/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 *
 * See the end of the file for implementation and maintenance notes.
 */

/*
 * Text properties implementation.  See ":help text-properties".
 */

#include "vim.h"

#if defined(FEAT_PROP_POPUP)

/*
 * In a hashtable item "hi_key" points to "pt_name" in a proptype_T.
 * This avoids adding a pointer to the hashtable item.
 * PT2HIKEY() converts a proptype pointer to a hashitem key pointer.
 * HIKEY2PT() converts a hashitem key pointer to a proptype pointer.
 * HI2PT() converts a hashitem pointer to a proptype pointer.
 */
#define PT2HIKEY(p)  ((p)->pt_name)
#define HIKEY2PT(p)   ((proptype_T *)((p) - offsetof(proptype_T, pt_name)))
#define HI2PT(hi)      HIKEY2PT((hi)->hi_key)

// The global text property types.
static hashtab_T *global_proptypes = NULL;
static proptype_T **global_proparray = NULL;

// The last used text property type ID.
static int proptype_id = 0;

// A counter used to assign virtual text IDs.
static int vt_id_counter = 0;

// These are used when sorting properties.
static textprop_T *text_prop_compare_props;
static buf_T	  *text_prop_compare_buf;

// A structure used to match a property.
typedef struct criteria_S
{
    int  id;            // Set to -MAXCOL if not used.
    int  num_type_ids;
    int	 *type_ids;
    int  flags;         // Set to -1 if not used.
    bool both;          // Should be false if id=-MAXCOL or num_type_ids=0
} criteria_T;

// Details of one or more property removal operations.
typedef struct
{
    int      removed_count;
    linenr_T first_changed;
    linenr_T last_changed;
} prop_remove_info_T;

// Display text associated with a property.
// The "ref_count" is used to track occasions when virtual text is (briefly)
// multiply referenced.
typedef struct vtext_S
{
    int8_t  ref_count;   // A reference count (< 10)
    char_u  text[1];     // The text, actually longer
} vtext_T;

// Code indicating if and how a property hase been modified.
typedef enum
{
    PC_UNMODIFIED,
    PC_MODIFIED,
    PC_RQ_DELETE,
} prop_mod_T;

#define PROP_COUNT_SIZE sizeof(uint16_t)
#define COPY_PROPS TRUE
#define NO_PROPS FALSE
#define WILL_NOT_CHANGE FALSE
#define DO_ALL TRUE
#define MATCH_ALL_PROPS NULL
#define NOT_DELETING FALSE
#define BORROW_VALUE FALSE
#define INSERT_IN_ORDER TRUE
#define UNSORTED FALSE

#define ERR_FAIL(report) (report, FAIL)
#define ERR_RET(report, retval) (report, retval)

static int text_prop_compare_p(const void *s1, const void *s2);
static bool um_detach(unpacked_memline_T *umemline);

/*
 * Calculate the byte length of a property.
 *
 * This is the actual length, ignoring the trailing NUL (which is included in
 * tp_len for properties that continue on the next line).
 */
    static int
prop_length(const textprop_T *prop)
{
    if (PROP_IS_VTEXT(prop))
	return 0;

    colnr_T prop_len = prop->tp_len;
    return prop->tp_flags & TP_FLAG_CONT_NEXT ? prop_len - 1 : prop_len;
}
/*
 * Test if the second property continues the first property.
 */
    static bool
properties_continue(
    const textprop_T *first_prop,
    const textprop_T *second_prop)
{
    bool matches = first_prop->tp_id == second_prop->tp_id;
    matches = matches && first_prop->tp_type == second_prop->tp_type;
    if (matches)
    {
	int first_flags = first_prop->tp_flags & TP_FLAG_CONT_BITS;
	int second_flags = second_prop->tp_flags & TP_FLAG_CONT_BITS;

	if (first_flags & TP_FLAG_CONT_NEXT && second_flags & TP_FLAG_CONT_PREV)
	{
	    // Tweak each property's "don't care' flags so we can make a simple
	    // equality comparison.
	    first_flags |= TP_FLAG_CONT_PREV;
	    second_flags |= TP_FLAG_CONT_NEXT;
	    return first_flags == second_flags;
	}
    }
    return FALSE;
}

/*
 * Test if the second property continues the first property.
 * This is an argument reversed version of properties_continue.
 */
    static bool
rev_properties_continue(
    const textprop_T *second_prop, const textprop_T *first_prop)
{
    return properties_continue(first_prop, second_prop);
}

/*
 * Clear the TP_FLAG_CONT_NEXT flag bit for a textprop_T.
 */
    static void
clear_cont_next(textprop_T *prop)
{
    if (prop->tp_flags & TP_FLAG_CONT_NEXT && prop->tp_len > 0)
	prop->tp_len -= 1;

    prop->tp_flags &= ~TP_FLAG_CONT_NEXT;
}

/*
 * Clear the TP_FLAG_CONT_PREV flag bit for a textprop_T.
 */
    static void
clear_cont_prev(textprop_T *prop)
{
    prop->tp_flags &= ~TP_FLAG_CONT_PREV;
}

/*
 * Free the virtual text in a detached unpacked memline properties array.
 */
    void
um_free_detached_props_vtext(textprop_T *props, int count)
{
    for (int i = 0; i < count; i++)
	free(props[i].tp_text);
}

/*
 * Free memory used by an unpacked memline, leaving it in a (temporarily)
 * unusable state. The "props" array is not freed if "free_props" is set.
 */
    static void
um_free(unpacked_memline_T *umemline)
{
    if (umemline->detached)
    {
	free(umemline->text);
	umemline->text = NULL;
	um_free_detached_props_vtext(umemline->props, umemline->prop_count);
    }
    free(umemline->props);
    if (umemline->next != NULL)
    {
	um_free(umemline->next);
	VIM_CLEAR(umemline->next);
    }
    if (umemline->prev != NULL)
    {
	um_free(umemline->prev);
	VIM_CLEAR(umemline->prev);
    }
    umemline->props = NULL;
    umemline->prop_size = 0;
    umemline->prop_count = 0;
    umemline->lnum = 0;
    umemline->detached = FALSE;
}

/*
 * Extract an unpacked memline's props array, taking ownership of it.
 *
 * The caller becomes responsible for freeing the props array. The
 * unpacked_memline_T becomes UNLOADED.
 */
    textprop_T *
um_extract_props(unpacked_memline_T *umemline)
{
    textprop_T *props = umemline->props;
    umemline->props = NULL;
    umemline->prop_count = 0;
    umemline->prop_size = 0;
    um_free(umemline);
    return props;
}

/*
 * Sort an unpacked memline's properties.
 */
    void
um_sort_props(unpacked_memline_T *umemline)
{
    if (umemline->props != NULL)
    {
	text_prop_compare_buf = umemline->buf;
	qsort(
	    (void *)umemline->props,
	    umemline->prop_count,
	    sizeof(textprop_T),
	    text_prop_compare_p);
    }
}

/*
 * Pack an unpacked memline into a newly allocated memline buffer.
 *
 * Return NULL is if memory cannot be allocated.
 */
    char_u *
um_pack(unpacked_memline_T *umemline, int *packed_length)
{
    // Work out the space required.
    *packed_length = 0;
    colnr_T memline_size = umemline->text_size;
    uint16_t live_props = 0;
    for (int i = 0; i < umemline->prop_count; i++)
    {
	textprop_T *prop = &umemline->props[i];
	if ((prop->tp_flags & TP_FLAG_DELETED) != 0)
	    continue;
	live_props += 1;
	if ((prop->tp_flags & TP_FLAG_VTEXT_BITS) != 0)
	    memline_size += prop->tp_len;
    }
    if (live_props)
    {
	memline_size += PROP_COUNT_SIZE;
	memline_size += live_props * sizeof(textprop_T);
    }

    // Allocate a new memline buffer.
    char_u *new_memline = alloc(memline_size);
    if (new_memline == NULL)
    {
	um_abort(umemline);
	return NULL;
    }
    *packed_length = memline_size;

    // Pack the buffer text part.
    mch_memmove(new_memline, umemline->text, umemline->text_size);
    free(umemline->text);
    umemline->text = new_memline;

    if (live_props > 0)
    {
	// Pack the properties and any associated virtual text.
	char_u *count_dest = new_memline + umemline->text_size;
	mch_memmove(count_dest, &live_props, PROP_COUNT_SIZE);
	char_u *prop_dest = count_dest + PROP_COUNT_SIZE;
	char_u *vtext_dest = prop_dest + live_props * sizeof(textprop_T);

	for (int i = 0; i < umemline->prop_count; i++)
	{
	    textprop_T *prop = &umemline->props[i];
	    if ((prop->tp_flags & TP_FLAG_DELETED) != 0)
		continue;

	    if ((prop->tp_flags & TP_FLAG_VTEXT_BITS) != 0)
	    {
		mch_memmove(vtext_dest, prop->tp_text, prop->tp_len);
		free(prop->tp_text);

		prop->tp_text_offset = vtext_dest - count_dest;
		mch_memmove(prop_dest, prop, sizeof(textprop_T));

		prop->tp_text = vtext_dest;
		vtext_dest += prop->tp_len;
	    }
	    else
	    {
		prop->tp_text_offset = 0;
		mch_memmove(prop_dest, prop, sizeof(textprop_T));
	    }
	    prop_dest += sizeof(textprop_T);
	}
    }
    umemline->detached = FALSE;

    return new_memline;
}

/*
 * Store changes to an unpacked memline.
 *
 * This creates a new memline buffer and packs it with the text and any
 * properties. Allocated text is discarded and the unpacked memline is nolonger
 * in the detached state.
 */
    static void
um_store_changes(unpacked_memline_T *umemline)
{
    if (umemline->prev != NULL)
    {
	um_store_changes(umemline->prev);
	um_free(umemline->prev);
	VIM_CLEAR(umemline->prev);
    }
    if (umemline->next != NULL)
    {
	um_store_changes(umemline->next);
	um_free(umemline->next);
	VIM_CLEAR(umemline->next);
    }

    if (!umemline->detached)
	return;

    // Pack up a new memline.
    int memline_size = 0;
    char_u *new_memline = um_pack(umemline, &memline_size);
    if (new_memline == NULL)
	return;

    // Replace the memline with the new one. If the "text_changed" flag is set
    // then use ml_replace_len() so that change tracking triggers are fired.
    // Otherwise update the memline directly.
    if (umemline->text_changed)
    {
	buf_T *saved_buf = curbuf;
	curbuf = umemline->buf;
	ml_replace_len(
	    umemline->lnum, new_memline, memline_size,
	    ML_PROPS_INCLUDED, ML_TAKE_OWNERSHIP_OF_LINE);
	curbuf = saved_buf;
    }
    else
    {
	buf_T *buf = umemline->buf;
	ml_get_buf(buf, umemline->lnum, WILL_NOT_CHANGE);
	if (buf->b_ml.ml_flags & (ML_ALLOCATED | ML_LINE_DIRTY))
	    vim_free(buf->b_ml.ml_line_ptr);
	buf->b_ml.ml_line_ptr = new_memline;
	buf->b_ml.ml_flags = buf->b_ml.ml_flags | ML_LINE_DIRTY;
	buf->b_ml.ml_line_len = memline_size;
	buf->b_ml.ml_line_textlen = umemline->text_size;
    }
}

/*
 * Make extra space, if required, to store additional properties.
 *
 * This will detach the unpacked memline. If any memorry allocation failure
 * occurs, the unpacked memline becomes unusable and hae "lnum" set to zero.
 */
    static bool
um_add_space_for_props(unpacked_memline_T *umemline, int extra_props)
{
    if (!umemline->detached && !um_detach(umemline))
	return FALSE;

    size_t prop_size = MAX(
	umemline->prop_size, umemline->prop_count + extra_props);
    if (umemline->prop_size >= prop_size)
	return TRUE;

    textprop_T *new_array = ALLOC_MULT(textprop_T, prop_size);
    if (new_array == NULL)
    {
	um_abort(umemline);
	return FALSE;
    }
    mch_memmove(
	new_array, umemline->props, umemline->prop_count * sizeof(textprop_T));
    free(umemline->props);
    umemline->props = new_array;
    umemline->prop_size = (uint16_t)prop_size;
    return TRUE;
}

/*
 * Common code for going to a specific line.
 */
    static bool
um_goto_line_common(
	unpacked_memline_T *umemline,
	linenr_T	   lnum,
	int		   extra_props,
	bool		   copy_props)
{
    if (umemline->buf == NULL)
	return FALSE;

    if (lnum == umemline->lnum && lnum > 0)
	return um_add_space_for_props(umemline, extra_props);

    um_store_changes(umemline);
    if (lnum == 0)
	goto fail;

    buf_T *buf = umemline->buf;
    memline_T *ml = &umemline->buf->b_ml;
    if (lnum < 1 || lnum > ml->ml_line_count)
	goto fail;

    size_t text_size = ml_get_buf_len(buf, lnum) + 1;
    size_t memline_size = ml->ml_line_len;
    uint16_t prop_count = 0;
    char_u *count_ptr = buf->b_ml.ml_line_ptr + text_size;
    size_t prop_part_len = memline_size - text_size;

    if (prop_part_len != 0)
    {
	if (prop_part_len < PROP_COUNT_SIZE + sizeof(textprop_T))
	    goto corrupted;

	mch_memmove(&prop_count, count_ptr, PROP_COUNT_SIZE);
    }

    size_t prop_size = MAX(umemline->prop_size, prop_count + extra_props);
    if (prop_size > umemline->prop_size)
    {
	free(umemline->props);
	umemline->props = ALLOC_MULT(textprop_T, prop_size);
	if (umemline->props == NULL)
	    goto fail;
    }

    umemline->prop_size = (uint16_t)prop_size;
    umemline->lnum = lnum;
    umemline->detached = FALSE;
    umemline->text_size = (colnr_T)text_size;
    umemline->text = buf->b_ml.ml_line_ptr;
    umemline->prop_count = 0;

    if (copy_props && prop_count > 0)
    {
	// Calculation of max_valid_vtext_offset allows space for NUL.
	umemline->prop_count = prop_count;
	char_u *memline_end = umemline->text + memline_size;
	size_t max_valid_vtext_offset = memline_end - count_ptr - 2;
	char_u *props_start = count_ptr + PROP_COUNT_SIZE;
	size_t props_size = sizeof(textprop_T) * prop_count;
	if (memline_end - props_start < (long)props_size)
	    goto corrupted;

	mch_memmove( umemline->props, props_start, props_size);
	for (int i = 0; i < prop_count; i++)
	{
	    textprop_T *prop = &umemline->props[i];
	    if (prop->tp_flags & TP_FLAG_VTEXT_BITS)
	    {
		if (prop->tp_text_offset > (colnr_T)max_valid_vtext_offset)
		    goto corrupted;  // NUL char beyond end of memline!
		prop->tp_text = count_ptr + prop->tp_text_offset;
	    }
	    else
	    {
		prop->tp_text = NULL;
	    }
	}
    }
    return TRUE;

corrupted:
    iemsg(e_text_property_info_corrupted);

fail:
    um_abort(umemline);
    return FALSE;
}

/*
 * Move an unpacked memline to a given line.
 *
 * If the given line is not the current line then any changes are first
 * 'written' back to the buffer's memline.
 *
 * Return TRUE if successful. Return FALSE if lnum == 0 or a falure occurred,
 * in which case unpacked memline is closed ("buf" == NULL).
 */
    bool
um_goto_line(unpacked_memline_T *umemline, linenr_T lnum, int extra_props)
{
    return um_goto_line_common(umemline, lnum, extra_props, COPY_PROPS);
}

/*
 * Open an unpacked memline for a buffer.
 *
 * The unpacked memline is empty, with "lnum" set to zero, waiting
 * for a call to um_goto_line.
 */
    unpacked_memline_T
um_open(buf_T *buf)
{
    unpacked_memline_T umemline;

    vim_memset(&umemline, 0, sizeof(unpacked_memline_T));
    umemline.buf = buf;
    return umemline;
}

/*
 * Open an unpacked memline for a buffer and goto a line.
 *
 * If it was not possible to goto the requested line then the unpacked memline
 * is set to a closed state.
 */
    unpacked_memline_T
um_open_at(buf_T *buf, linenr_T lnum, int extra_props)
{
    unpacked_memline_T umemline = um_open(buf);
    um_goto_line(&umemline, lnum, extra_props);
    return umemline;
}

/*
 * Open an unpacked memline for a buffer, goto a line and detach.
 */
    unpacked_memline_T
um_open_at_detached(buf_T *buf, linenr_T lnum, int extra_props)
{
    unpacked_memline_T umemline = um_open_at(buf, lnum, extra_props);
    if (umemline.buf != NULL)
	um_detach(&umemline);
    return umemline;
}

/*
 * Open an unpacked memline for a buffer, goto a line and detach, but do not
 * copy the properties. Space will be allocated for "prop_count" properties.
 */
    unpacked_memline_T
um_open_at_no_props(buf_T *buf, linenr_T lnum, int prop_count)
{
    unpacked_memline_T umemline = um_open(buf);
    um_goto_line_common(&umemline, lnum, prop_count, NO_PROPS);
    if (umemline.buf != NULL)
	um_detach(&umemline);
    return umemline;
}

/*
 * Close an open unpacked memline.
 *
 * This ensures than any changes get 'written' back to the buffer's memline.
 * The unpacked memline should not be used after this.
 */
    void
um_close(unpacked_memline_T *umemline)
{
    if (umemline->lnum > 0)
	um_goto_line(umemline, 0, 0);
}

/*
 * Close an unpacked memline without saving.
 *
 * All memory is freed and the "buf" is set to NULL to indicate the closed
 * state. Use this for error conditions or to close without writaing changes
 * back to the buffer's memline.
 */
    void
um_abort(unpacked_memline_T *umemline)
{
    if (umemline->buf != NULL)
    {
	um_free(umemline);
	umemline->buf = NULL;
    }
}

/*
 * Set the text for an unpacked memline.
 *
 * The provided "text" must be a pointer to allocated memory. The unpacked
 * memline takes ownership of this allocated memory. The unpacked memline
 * becomes detached.
 *
 * This returns false if the unpacked memline could not be detached. If already
 * detached then this call cannot fail.
 */
    bool
um_set_text(unpacked_memline_T *umemline, char_u *text)
{
    if (!umemline->detached && !um_detach(umemline))
	return FALSE;

    free(umemline->text);
    umemline->text = text;
    umemline->text_size = (colnr_T)(STRLEN(text) + 1);
    umemline->text_changed = TRUE;
    return TRUE;
}

    static bool
um_add_prop_common(
    unpacked_memline_T *umemline, const textprop_T *prop, bool sorted)
{
    if (!umemline->detached && !um_detach(umemline))
	return FALSE;

    if (umemline->prop_size == umemline->prop_count)
    {
	if (!um_add_space_for_props(umemline, 1))
	{
	    return FALSE;
	}
    }

    if (!sorted)
    {
	umemline->props[umemline->prop_count++] = *prop;
	return TRUE;
    }

    colnr_T sort_col = prop->tp_col;
    if (PROP_IS_VTEXT(prop) != 0 && PROP_IS_ABOVE(prop))
	sort_col = 0;

    int i = 0;
    for (i = 0; i < umemline->prop_count; i++)
    {
	textprop_T *cur_prop = &umemline->props[i];
	colnr_T col = cur_prop->tp_col;
	if (PROP_IS_VTEXT(cur_prop) != 0 && PROP_IS_ABOVE(cur_prop))
	    col = 0;

	if (col >= sort_col)
	    break;
    }

    if (i < umemline->prop_count)
	mch_memmove(
	    &umemline->props[i + 1], &umemline->props[i],
	    sizeof(textprop_T) * (umemline->prop_count - i));
    umemline->props[i] = *prop;
    umemline->prop_count += 1;

    return TRUE;
}

/*
 * Add a property to an unpacked memline.
 *
 * The property is inserted based on its start column, with special rules
 * for virtual text properies. The unpacked memline becomes detached.
 *
 * TODO:
 *     It would be good to document why this sorted order is important.
 *     It is propbably critical to make screen drawing work correctly, even
 *     though that also involves lots of property sorting.
 */
    static bool
um_add_prop(unpacked_memline_T *umemline, const textprop_T *prop)
{
    return um_add_prop_common(umemline, prop, INSERT_IN_ORDER);
}

/*
 * Add a property to an unpacked memline without regard to insertion order.
 *
 * This is used for special cases, such as during line joining.
 */
    static bool
um_add_prop_unsorted(unpacked_memline_T *umemline, const textprop_T *prop)
{
    return um_add_prop_common(umemline, prop, UNSORTED);
}

/*
 * Create a neighbouring unpacked memline.
 */
    static unpacked_memline_T *
um_create_neighbour(unpacked_memline_T *umemline, linenr_T lnum)
{
    unpacked_memline_T *neighbour = ALLOC_ONE(unpacked_memline_T);
    if (neighbour == NULL)
	return NULL;

    *neighbour = um_open_at_detached(umemline->buf, lnum, 0);
    if (neighbour->buf == NULL)
    {
	um_abort(neighbour);
	free(neighbour);
	return NULL;
    }
    return neighbour;
}

/*
 * Find and adjust the continuation for a property's neighbour.
 */
    static void
um_adjust_continuation(
	unpacked_memline_T *umemline,
	const textprop_T *the_prop,
	bool (*match_continuation)(const textprop_T *, const textprop_T *),
	void (*modify)(textprop_T *))
{
    for (int i = 0; i < umemline->prop_count; i++)
    {
	textprop_T *prop = &umemline->props[i];
	if (match_continuation(prop, the_prop))
	{
	    modify(prop);
	    return;
	}
    }
}

/*
 * Adjust properties on a neighbouring lines for a property deletion.
 */
    static void
um_adjust_for_deletion(unpacked_memline_T *umemline, int index)
{
    if (!um_detach(umemline))
	return;

    textprop_T *prop = &umemline->props[index];
    const bool deleting = (umemline->adjust_flags & ML_DEL_DELETING_LINE) != 0;
    const bool fix_prev = !(umemline->adjust_flags & ML_DEL_NO_ADJ_PREV);
    const bool fix_next = !(umemline->adjust_flags & ML_DEL_NO_ADJ_NEXT);
    const bool cont_prev = (prop->tp_flags & TP_FLAG_CONT_PREV) != 0;
    const bool cont_next = (prop->tp_flags & TP_FLAG_CONT_NEXT) != 0;

    if (fix_prev && cont_prev && (!deleting || !cont_next))
    {
	if (umemline->prev == NULL)
	{
	    umemline->prev = um_create_neighbour(umemline, umemline->lnum - 1);
	    if (umemline->prev == NULL)
		goto fail;
	}
	um_adjust_continuation(
	    umemline->prev, prop, properties_continue, clear_cont_next);
    }
    if (fix_next && cont_next && (!deleting || !cont_prev))
    {
	if (umemline->next == NULL)
	{
	    umemline->next = um_create_neighbour(umemline, umemline->lnum + 1);
	    if (umemline->next == NULL)
		goto fail;
	}
	um_adjust_continuation(
	    umemline->next, prop, rev_properties_continue, clear_cont_prev);
    }
    return;

fail:
    um_abort(umemline);
}

/*
 * Mark the property at a given index as deleted.
 */
    void
um_delete_prop(unpacked_memline_T *umemline, int index)
{
    if (!um_detach(umemline))
	return;

    textprop_T *prop = &umemline->props[index];
    prop->tp_flags |= TP_FLAG_DELETED;
    if (PROP_IS_VTEXT(prop))
    {
	free(prop->tp_text);
	prop->tp_text = NULL;
	prop->tp_len = 0;
    }
    um_adjust_for_deletion(umemline, index);
}

/*
 * Detach an an unpacked memline from its memline by allocating the text
 * and all virtual text of the properties.
 *
 * Returns OK on success or if already detached and FAIL if any memory
 * allocation fails, in which case the unpacked memline is left in an unusable
 * state.
 */
    static bool
um_detach(unpacked_memline_T *umemline)
{
    if (umemline->detached)
	return TRUE;

    int i = 0;
    if (umemline->text != NULL)
    {
	umemline->text = vim_strsave(umemline->text);
    }
    else
    {
	umemline->text = vim_strsave((char_u *)"");
	umemline->text_size = 1;
    }
    if (umemline->text == NULL)
	goto fail;

    for (i = 0; i < umemline->prop_count; i++)
    {
	textprop_T *prop = &umemline->props[i];
	if (prop->tp_text != NULL)
	{
	    prop->tp_text = vim_strsave(prop->tp_text);
	    if (prop->tp_text == NULL)
		goto fail;
	}
    }
    umemline->detached = TRUE;
    return TRUE;

fail:
    free(umemline->text);
    for (int j = 0; j < i; j++)
    {
	free(umemline->props[j].tp_text);
    }
    return FALSE;
}

/*
 * Reverse the order of the properties.
 */
    void
um_reverse_props(unpacked_memline_T *umemline)
{
    textprop_T *props = umemline->props;

    for (int i = 0, j = umemline->prop_count - 1; i < j; i++, j--)
    {
	if (i != j)
	{
	    textprop_T temp_prop = props[i];
	    props[i] = props[j];
	    props[j] = temp_prop;
	}
    }
}

/*
 * Make a copy of a virtual text string and clean TABs, etc.
 */
    static char_u *
copy_and_clean_virtual_text(const char_u *text)
{
    char_u *virt_text = vim_strsave(text);
    if (virt_text == NULL)
	return NULL;

    // Change any control character (TAB, newline, etc.) to a space to make
    // it simpler to compute the size.
    for (char_u *p = virt_text; *p != NUL; MB_PTR_ADV(p))
	if (*p < ' ')
	    *p = ' ';
    return virt_text;
}

/*
 * Test if a property matches a given set of criteria.
 */
    static bool
prop_matches(
    const textprop_T *prop,
    const criteria_T *criteria)
{
    bool matches_id = FALSE;
    bool matches_type = FALSE;

    // Find a match for either ID, a type or both depending on whether 'both'
    // is set.
    if (criteria->id != -MAXCOL)
    {
	matches_id = prop->tp_id == criteria->id;
	if (criteria->both && !matches_id)
	    return FALSE;

    }
    if (criteria->num_type_ids > 0)
    {
	matches_type = FALSE;
	for (int idx = 0; !matches_type && idx < criteria->num_type_ids; ++idx)
	    matches_type = prop->tp_type == criteria->type_ids[idx];
    }
    if (criteria->both)
    {
	if (!(matches_id && matches_type))
	    return FALSE;
    }
    else
    {
	if (!(matches_id || matches_type))
	    return FALSE;
    }

    if (criteria->flags != -1)
	return prop->tp_flags == criteria->flags;

    return TRUE;
}

/*
 * Find a property type by name, return the hashitem.
 * Returns NULL if the item can't be found.
 */
    static hashitem_T *
find_prop_type_hi(char_u *name, buf_T *buf)
{
    hashtab_T	*ht;
    hashitem_T	*hi;

    if (*name == NUL)
	return NULL;
    if (buf == NULL)
	ht = global_proptypes;
    else
	ht = buf->b_proptypes;

    if (ht == NULL)
	return NULL;
    hi = hash_find(ht, name);
    if (HASHITEM_EMPTY(hi))
	return NULL;
    return hi;
}

/*
 * Like find_prop_type_hi() but return the property type.
 */
    static proptype_T *
find_prop_type(char_u *name, buf_T *buf)
{
    hashitem_T	*hi = find_prop_type_hi(name, buf);

    if (hi == NULL)
	return NULL;
    return HI2PT(hi);
}

/*
 * Get the prop type ID of "name".
 * When not found return zero.
 */
    int
find_prop_type_id(char_u *name, buf_T *buf)
{
    proptype_T *pt = find_prop_type(name, buf);

    if (pt == NULL)
	return 0;
    return pt->pt_id;
}

/*
 * Lookup a property type by name.  First in "buf" and when not found in the
 * global types.
 * When not found gives an error message and returns NULL.
 */
    static proptype_T *
lookup_prop_type(char_u *name, buf_T *buf)
{
    proptype_T *type = find_prop_type(name, buf);

    if (type == NULL)
	type = find_prop_type(name, NULL);
    if (type == NULL)
	semsg(_(e_property_type_str_does_not_exist), name);
    return type;
}

/*
 * Get an optional "bufnr" item from the dict in "arg".
 * When the argument is not used or "bufnr" is not present then "buf" is
 * unchanged.
 * If "bufnr" is valid or not present return OK.
 * When "arg" is not a dict or "bufnr" is invalid return FAIL.
 */
    static int
get_bufnr_from_arg(typval_T *arg, buf_T **buf)
{
    dictitem_T	*di;

    if (arg->v_type != VAR_DICT)
	return ERR_FAIL(emsg(_(e_dictionary_required)));
    if (arg->vval.v_dict == NULL)
	return OK;  // NULL dict is like an empty dict
    di = dict_find(arg->vval.v_dict, (char_u *)"bufnr", -1);
    if (di != NULL && (di->di_tv.v_type != VAR_NUMBER
					      || di->di_tv.vval.v_number != 0))
    {
	*buf = get_buf_arg(&di->di_tv);
	if (*buf == NULL)
	    return FAIL;
    }
    return OK;
}

/*
 * Remove or clean up matching properties from a line.
 *
 * If "do_all" is not set then only the first matching property is processed.
 * If "deleting" is set then Vim is in the process of deleting the line and
 * this function does not modify the line's contents, although it may modify
 * those of the preceding and following lines.
 */
    int
remove_props_from_line(
    buf_T                   *buf,
    linenr_T                lnum,
    const struct criteria_S *criteria,
    bool                     do_all,
    bool                     deleting,
    int                      flags)
{
    int  removed_count = 0;

    unpacked_memline_T umemline = um_open(buf);
    umemline.adjust_flags = flags;
    if (deleting)
	umemline.adjust_flags |= ML_DEL_DELETING_LINE;

    um_goto_line(&umemline, lnum, 0);
    for (int i = 0; i < umemline.prop_count; i++)
    {
	textprop_T *prop = &umemline.props[i];
	if (criteria != NULL && !prop_matches(prop, criteria))
	    continue;

	// A matching property has been found, which means the line may
	// need to be copied so modifications can be made.
	if (!deleting)
	{
	    um_delete_prop(&umemline, i);
	    removed_count += 1;
	}
	else
	{
	    um_adjust_for_deletion(&umemline, i);
	}
	if (!do_all)
	    break;
    }

    um_close(&umemline);
    return removed_count;
}

/*
 * Remove matching properties over a given line range.
 * If the "criteria" pointer is NULL then all properties are be removed.
 */
    static prop_remove_info_T
remove_props(
    buf_T *buf,
    const int start,
    const int end,
    const criteria_T *criteria,
    const bool do_all)
{
    prop_remove_info_T info = {0, 0, 0};

    for (linenr_T lnum = start; lnum <= end; ++lnum)
    {
	int count = remove_props_from_line(
		buf, lnum, criteria, do_all, NOT_DELETING, 0);
	if (count)
	{
	    if (info.first_changed == 0)
		info.first_changed = lnum;
	    info.last_changed = lnum;
	    info.removed_count += count;
	}
    }
    return info;
}

/*
 * prop_add({lnum}, {col}, {props})
 */
    void
f_prop_add(typval_T *argvars, typval_T *rettv)
{
    linenr_T	start_lnum;
    colnr_T	start_col;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_number_arg(argvars, 1) == FAIL
		|| check_for_dict_arg(argvars, 2) == FAIL))
	return;

    start_lnum = tv_get_number(&argvars[0]);
    start_col = tv_get_number(&argvars[1]);
    if (check_for_dict_arg(argvars, 2) == FAIL)
	return;

    rettv->vval.v_number = prop_add_common(start_lnum, start_col,
				 argvars[2].vval.v_dict, curbuf, &argvars[2]);
}

/*
 * Attach a text property 'type_name' to the text starting
 * at [start_lnum, start_col] and ending at [end_lnum, end_col] in
 * the buffer "buf" and assign identifier "id".
 *
 * The "text" is the propery's virtual text and when not NULL "start_lnum" must
 * equal "end_lnum". Also the "id" will be set to a unique (for the buffer)
 * negative value.
 */
    static int
prop_add_one(
	buf_T		*buf,
	char_u		*type_name,
	int		*id,
	char_u		*text_arg,
	int		text_padding_left,
	int		text_flags,
	linenr_T	start_lnum,
	linenr_T	end_lnum,
	colnr_T		start_col,
	colnr_T		end_col)
{
    proptype_T	    *type = lookup_prop_type(type_name, buf);

    if (type == NULL)
	return FAIL;
    if (start_lnum < 1 || start_lnum > buf->b_ml.ml_line_count)
	return ERR_FAIL(semsg(_(e_invalid_line_number_nr), (long)start_lnum));
    if (end_lnum < start_lnum || end_lnum > buf->b_ml.ml_line_count)
	return ERR_FAIL(semsg(_(e_invalid_line_number_nr), (long)end_lnum));
    if (buf->b_ml.ml_mfp == NULL)
	return ERR_FAIL(emsg(_(e_cannot_add_text_property_to_unloaded_buffer)));

    char_u *virt_text = NULL;
    if (text_arg != NULL)
    {
	if ((virt_text = copy_and_clean_virtual_text(text_arg)) == NULL)
	    goto fail;
	*id = --vt_id_counter;
    }

    unpacked_memline_T umemline = um_open(buf);
    for (linenr_T lnum = start_lnum; lnum <= end_lnum; ++lnum)
    {
	if (!um_goto_line(&umemline, lnum, 1))
	    goto fail;

	size_t	textlen = umemline.text_size;
	colnr_T col = lnum == start_lnum ? start_col : 1;
	if (col - 1 > (colnr_T)textlen && !(col == 0 && virt_text != NULL))
	{
	    semsg(_(e_invalid_column_number_nr), (long)start_col);
	    goto fail;
	}

	long    length = lnum == end_lnum
		       ? end_col - col : (int)textlen - col + 1;
	length = MAX(0, MIN(length, (long)textlen));

	if (virt_text != NULL)
	{
	    length = (long)(STRLEN(virt_text) + 1);
	    if (col == 0)
	    {
		col = MAXCOL;  // This ensures correct property sorting.
	    }
	}

	textprop_T tmp_prop;
	tmp_prop.tp_col = col;
	tmp_prop.tp_len = length;
	tmp_prop.tp_id = *id;
	tmp_prop.tp_type = type->pt_id;
	tmp_prop.tp_flags = text_flags
			    | (lnum > start_lnum ? TP_FLAG_CONT_PREV : 0)
			    | (lnum < end_lnum ? TP_FLAG_CONT_NEXT : 0)
			    | ((type->pt_flags & PT_FLAG_INS_START_INCL)
						     ? TP_FLAG_START_INCL : 0);
	tmp_prop.tp_padleft = text_padding_left;
	tmp_prop.tp_text = virt_text;
	if (!um_add_prop(&umemline, &tmp_prop))
	    goto fail;
    }

    um_close(&umemline);
    changed_line_display_buf(buf);
    changed_lines_buf(buf, start_lnum, end_lnum + 1, 0);
    return OK;

fail:
    free(virt_text);
    um_abort(&umemline);
    return FAIL;
}

/*
 * prop_add_list()
 * First argument specifies the text property:
 *   {'type': <str>, 'id': <num>, 'bufnr': <num>}
 * Second argument is a List where each item is a List with the following
 * entries: [lnum, start_col, end_col]
 */
    void
f_prop_add_list(typval_T *argvars, typval_T *rettv UNUSED)
{
    dict_T	*dict;
    char_u	*type_name;
    buf_T	*buf = curbuf;
    int		id = 0;
    listitem_T	*li;
    list_T	*pos_list;
    linenr_T	start_lnum;
    colnr_T	start_col;
    linenr_T	end_lnum;
    colnr_T	end_col;
    int		error = FALSE;
    int		prev_did_emsg = did_emsg;

    if (check_for_dict_arg(argvars, 0) == FAIL
	    || check_for_list_arg(argvars, 1) == FAIL)
	return;

    if (check_for_nonnull_list_arg(argvars, 1) == FAIL)
	return;

    dict = argvars[0].vval.v_dict;
    if (dict == NULL || !dict_has_key(dict, "type"))
    {
	emsg(_(e_missing_property_type_name));
	return;
    }
    type_name = dict_get_string(dict, "type", FALSE);

    if (dict_has_key(dict, "id"))
    {
	vimlong_T x;
	x = dict_get_number(dict, "id");
	if (x > INT_MAX || x  <= INT_MIN)
	{
	    semsg(_(e_val_too_large), dict_get_string(dict, "id", FALSE));
	    return;
	}
	id = (int)x;
    }

    if (get_bufnr_from_arg(&argvars[0], &buf) == FAIL)
	return;

    // This must be done _before_ we start adding properties because property
    // changes trigger buffer (memline) reorganisation, which needs this flag
    // to be correctly set.
    buf->b_has_textprop = TRUE;  // this is never reset
    FOR_ALL_LIST_ITEMS(argvars[1].vval.v_list, li)
    {
	if (li->li_tv.v_type != VAR_LIST || li->li_tv.vval.v_list == NULL)
	{
	    emsg(_(e_list_required));
	    return;
	}

	pos_list = li->li_tv.vval.v_list;
	start_lnum = list_find_nr(pos_list, 0L, &error);
	if (!error)
	    start_col = list_find_nr(pos_list, 1L, &error);
	if (!error)
	    end_lnum = list_find_nr(pos_list, 2L, &error);
	if (!error)
	    end_col = list_find_nr(pos_list, 3L, &error);
	int this_id = id;
	if (!error && pos_list->lv_len > 4)
	    this_id = list_find_nr(pos_list, 4L, &error);
	if (error || start_lnum <= 0 || start_col <= 0
		  || end_lnum <= 0 || end_col <= 0)
	{
	    if (prev_did_emsg == did_emsg)
		emsg(_(e_invalid_argument));
	    return;
	}
	if (prop_add_one(buf, type_name, &this_id, NULL, 0, 0,
			     start_lnum, end_lnum, start_col, end_col) == FAIL)
	    return;
    }

    redraw_buf_later(buf, UPD_VALID);
}

/*
 * Shared between prop_add() and popup_create().
 * "dict_arg" is the function argument of a dict containing "bufnr".
 * it is NULL for popup_create().
 * Returns the "id" used for "text" or zero.
 */
    int
prop_add_common(
	linenr_T    start_lnum,
	colnr_T	    start_col,
	dict_T	    *dict,
	buf_T	    *default_buf,
	typval_T    *dict_arg)
{
    linenr_T	end_lnum;
    colnr_T	end_col;
    char_u	*type_name;
    buf_T	*buf = default_buf;
    int		id = 0;
    char_u	*text = NULL;
    int		text_padding_left = 0;
    int		flags = 0;

    if (dict == NULL || !dict_has_key(dict, "type"))
	return ERR_RET(emsg(
	    _(e_missing_property_type_name)), id);
    type_name = dict_get_string(dict, "type", FALSE);

    if (dict_has_key(dict, "end_lnum"))
    {
	end_lnum = dict_get_number(dict, "end_lnum");
	if (end_lnum < start_lnum)
	    return ERR_RET(semsg(
		_(e_invalid_value_for_argument_str), "end_lnum"), id);
    }
    else
	end_lnum = start_lnum;

    if (dict_has_key(dict, "length"))
    {
	long length = dict_get_number(dict, "length");

	if (length < 0 || end_lnum > start_lnum)
	    return ERR_RET(semsg(
		_(e_invalid_value_for_argument_str), "length"), id);
	end_col = start_col + length;
    }
    else if (dict_has_key(dict, "end_col"))
    {
	end_col = dict_get_number(dict, "end_col");
	if (end_col <= 0)
	    return ERR_RET(semsg(
		_(e_invalid_value_for_argument_str), "end_col"), id);
    }
    else if (start_lnum == end_lnum)
	end_col = start_col;
    else
	end_col = 1;

    if (dict_has_key(dict, "id"))
    {
	vimlong_T x;
	x = dict_get_number(dict, "id");
	if (x > INT_MAX || x  <= INT_MIN)
	    return ERR_RET(semsg(
		_(e_val_too_large), dict_get_string(dict, "id", FALSE)), id);
	id = (int)x;
    }

    if (dict_has_key(dict, "text"))
    {
	if (dict_has_key(dict, "length")
		|| dict_has_key(dict, "id")
		|| dict_has_key(dict, "end_col")
		|| dict_has_key(dict, "end_lnum"))
	    return ERR_RET(emsg(
		_(e_cannot_use_id_length_endcol_and_endlnum_with_text)), id);

	text = dict_get_string(dict, "text", BORROW_VALUE);
	if (text == NULL)
	    return id;

	end_col = start_col + 1;
	if (dict_has_key(dict, "text_align"))
	{
	    char_u *p = dict_get_string(dict, "text_align", FALSE);

	    if (p == NULL)
		return id;
	    if (start_col != 0)
		return ERR_RET(emsg(
		    _(e_can_only_use_text_align_when_column_is_zero)), id);
	    if (STRCMP(p, "right") == 0)
		flags |= TP_FLAG_ALIGN_RIGHT;
	    else if (STRCMP(p, "above") == 0)
		flags |= TP_FLAG_ALIGN_ABOVE;
	    else if (STRCMP(p, "below") == 0)
		flags |= TP_FLAG_ALIGN_BELOW;
	    else if (STRCMP(p, "after") == 0)
		flags |= TP_FLAG_ALIGN_AFTER;
	    else
		return ERR_RET(
		    semsg(
			_(e_invalid_value_for_argument_str_str),
			"text_align", p), id);
	}
	else
	{
	    if (start_col == 0)
		flags |= TP_FLAG_ALIGN_AFTER;
	    else
		flags |= TP_FLAG_INLINE;
	}

	if (dict_has_key(dict, "text_padding_left"))
	{
	    text_padding_left = dict_get_number(dict, "text_padding_left");
	    if (text_padding_left < 0)
		return ERR_RET(semsg(
		    _(e_argument_must_be_positive_str), "text_padding_left"),
		    id);
	}

	if (dict_has_key(dict, "text_wrap"))
	{
	    char_u *p = dict_get_string(dict, "text_wrap", FALSE);

	    if (p == NULL)
		return id;
	    if (STRCMP(p, "wrap") == 0)
		flags |= TP_FLAG_WRAP;
	    else if (STRCMP(p, "truncate") != 0)
		return ERR_RET(semsg(
		    _(e_invalid_value_for_argument_str_str), "text_wrap", p),
		    id);
	}
    }

    // Column must be 1 or more for a normal text property; when "text" is
    // present zero means it goes after, above or below the line.
    if (start_col < (text == NULL ? 1 : 0))
	return ERR_RET(semsg(
	    _(e_invalid_column_number_nr), (long)start_col), id);
    if (start_col > 0 && text_padding_left > 0)
	return ERR_RET(emsg(
	    _(e_can_only_use_left_padding_when_column_is_zero)), id);

    if (dict_arg != NULL && get_bufnr_from_arg(dict_arg, &buf) == FAIL)
	return id;

    // This must be done _before_ we add the property because property changes
    // trigger buffer (memline) reorganisation, which needs this flag to be
    // correctly set.
    buf->b_has_textprop = TRUE;  // this is never reset

    prop_add_one(
	buf, type_name, &id, text, text_padding_left, flags,
	start_lnum, end_lnum, start_col, end_col);
    text = NULL;

    redraw_buf_later(buf, UPD_VALID);
    return id;
}

/*
 * Test if a line in curbuf has a property with the given flags set.
 */
    int
has_prop_w_flags(linenr_T lnum, int flags)
{
    int result = FALSE;

    unpacked_memline_T umemline = um_open(curbuf);
    if (!um_goto_line(&umemline, lnum, 0))
	return FALSE;

    for (int i = 0; i < umemline.prop_count; i++)
    {
	if (umemline.props[i].tp_flags & flags)
	{
	    result = TRUE;
	    break;
	}
    }
    um_close(&umemline);
    return result;
}

/*
 * Get a read-only allocated copy of a line's text properties.
 *
 * The "count" is set to the number of properties. NULL is returned
 * if the line has no properties.
 *
 * Virtual text properties have "tp_text" pointers into the buffer's memline,
 * which will become invalid if the line gets modified.
 */
    textprop_T *
get_text_props_copy(buf_T *buf, linenr_T lnum, int *count)
{
    // Be quick when no text property types have been defined for the buffer.
    *count = 0;
    if (!buf->b_has_textprop || buf->b_ml.ml_mfp == NULL)
	return NULL;

    unpacked_memline_T umemline = um_open_at(buf, lnum, 0);
    if (umemline.buf == NULL || umemline.prop_count == 0)
	goto clean_and_return_null;

    *count = umemline.prop_count;
    textprop_T *props = um_extract_props(&umemline);
    um_abort(&umemline);
    return props;

clean_and_return_null:
    um_abort(&umemline);
    return NULL;
}

/*
 * Return the number of text properties with "above" or "below" alignment in
 * line "lnum".  A "right" aligned property also goes below after a "below" or
 * other "right" aligned property.
 */
    int
prop_count_above_below(buf_T *buf, linenr_T lnum)
{
    int		       result = 0;
    int		       next_right_goes_below = FALSE;
    unpacked_memline_T umemline = um_open(buf);

    if (!um_goto_line(&umemline, lnum, 0))
	return 0;

    for (int i = 0; i < umemline.prop_count; i++)
    {
	textprop_T *prop = &umemline.props[i];
	if (PROP_IS_FLOATING(prop) && text_prop_type_valid(buf, prop))
	{
	    if (PROP_IS_ABOVE_OR_BELOW(prop)
		|| (next_right_goes_below && PROP_IS_RIGHT(prop)))
	    {
		++result;
	    }
	    else if (PROP_IS_RIGHT(prop))
	    {
		next_right_goes_below = TRUE;
	    }
	}
    }
    um_close(&umemline);
    return result;
}

/*
 * Get the number of properties for a given line in a buffer.
 */
    static int
get_line_prop_count(buf_T *buf, linenr_T lnum)
{
    uint16_t prop_count = 0;
    size_t text_size = ml_get_buf_len(buf, lnum) + 1;
    if (buf->b_ml.ml_line_len - text_size >= PROP_COUNT_SIZE)
    {
	char_u *count_ptr = buf->b_ml.ml_line_ptr + text_size;
	mch_memmove(&prop_count, count_ptr, PROP_COUNT_SIZE);
    }
    return prop_count;
}

/*
 * Count text properties on line "lnum" in the current buffer.
 *
 * When "only_starting" is true only text properties starting in this line will
 * be considered. When "last_line" is FALSE then text properties after the line
 * are not counted.
 */
    int
count_props(linenr_T lnum, bool only_starting, bool last_line)
{
    if (!(only_starting || last_line))
	return get_line_prop_count(curbuf, lnum);

    int prop_count = 0;
    unpacked_memline_T umemline = um_open(curbuf);
    um_goto_line(&umemline, lnum, 0);
    prop_count = umemline.prop_count;
    for (int i = 0; i < umemline.prop_count; i++)
    {
	textprop_T *prop = &umemline.props[i];
	if ((only_starting && (prop->tp_flags & TP_FLAG_CONT_PREV))
		|| (!last_line && PROP_IS_FLOATING(prop)))
	    prop_count -= 1;
    }
    um_close(&umemline);
    return prop_count;
}

/*
 * Score for sorting on position of the text property: 0: above,
 * 1: after (default), 2: right, 3: below (comes last)
 */
    static int
text_prop_order(int flags)
{
    int align = flags & TP_FLAG_VTEXT_BITS;
    if (align == TP_FLAG_ALIGN_ABOVE)
	return 0;
    if (align == TP_FLAG_ALIGN_RIGHT)
	return 2;
    if (align == TP_FLAG_ALIGN_BELOW)
	return 3;
    return 1;
}

/*
 * Function passed to qsort() to sort text properties.
 * Return 1 if "s1" has priority over "s2", -1 if the other way around, zero if
 * both have the same priority.
 */
    static int
text_prop_compare_p(const void *s1, const void *s2)
{
    textprop_T	*tp1 = (textprop_T *)s1;
    textprop_T	*tp2 = (textprop_T *)s2;

    // property that inserts text has priority over one that doesn't
    if (PROP_IS_VTEXT(tp1) != PROP_IS_VTEXT(tp2))
	return PROP_IS_VTEXT(tp1) ? 1 : -1;

    if (PROP_IS_FLOATING(tp1) || PROP_IS_FLOATING(tp2))
    {
	int order1 = text_prop_order(tp1->tp_flags);
	int order2 = text_prop_order(tp2->tp_flags);

	// sort on order where it is added
	if (order1 != order2)
	    return order1 < order2 ? 1 : -1;
    }

    // check highest priority, defined by the type
    proptype_T *pt1 = text_prop_type_by_id(
	text_prop_compare_buf, tp1->tp_type);
    proptype_T *pt2 = text_prop_type_by_id(
	text_prop_compare_buf, tp2->tp_type);
    if (pt1 != pt2)
    {
	if (pt1 == NULL)
	    return -1;
	if (pt2 == NULL)
	    return 1;
	if (pt1->pt_priority != pt2->pt_priority)
	    return pt1->pt_priority > pt2->pt_priority ? 1 : -1;
    }

    // same priority, one that starts first wins
    colnr_T col1 = tp1->tp_col;
    colnr_T col2 = tp2->tp_col;
    if (col1 != col2)
	return col1 < col2 ? 1 : -1;

    // for a property with text the id can be used as tie breaker
    if (PROP_IS_VTEXT(tp1))
	return tp1->tp_id > tp2->tp_id ? 1 : -1;

    return 0;
}

/*
 * Function passed to qsort() to sort text properties.
 * Return 1 if "s1" has priority over "s2", -1 if the other way around, zero if
 * both have the same priority.
 */
    static int
text_prop_compare(const void *s1, const void *s2)
{
    int  idx1, idx2;
    textprop_T	*tp1, *tp2;

    idx1 = *(int *)s1;
    idx2 = *(int *)s2;
    tp1 = &text_prop_compare_props[idx1];
    tp2 = &text_prop_compare_props[idx2];
    return text_prop_compare_p(tp1, tp2);
}

/*
 * Sort "count" text properties using an array of indexes "idxs" into the list
 * of text props "props" for buffer "buf".
 */
    void
sort_text_props(
	buf_T	    *buf,
	textprop_T  *props,
	int	    *idxs,
	int	    count)
{
    text_prop_compare_buf = buf;
    text_prop_compare_props = props;
    qsort((void *)idxs, (size_t)count, sizeof(int), text_prop_compare);
}

/*
 * Find text property "type_id" in the visible lines of window "wp".
 * Match "id" when it is > 0.
 * Returns FAIL when not found.
 */
    int
find_visible_prop(
	win_T	    *wp,
	int	    type_id,
	int	    id,
	textprop_T  *prop,
	linenr_T    *found_lnum)
{
    // return when "type_id" no longer exists
    if (text_prop_type_by_id(wp->w_buffer, type_id) == NULL)
	return FAIL;

    // w_botline may not have been updated yet.
    validate_botline_win(wp);

    unpacked_memline_T umemline = um_open(wp->w_buffer);
    bool	       found = FALSE;
    for (linenr_T lnum = wp->w_topline; !found && lnum < wp->w_botline; ++lnum)
    {
	if (!um_goto_line(&umemline, lnum, 0))
	    return FAIL;

	for (int i = 0; i < umemline.prop_count; i++)
	{
	    textprop_T *tmp_prop = &umemline.props[i];
	    if (tmp_prop->tp_type == type_id && (id <= 0 || tmp_prop->tp_id == id))
	    {
		found = TRUE;
		*prop = *tmp_prop;
		*found_lnum = lnum;
		break;
	    }
	}
    }
    um_abort(&umemline);
    return found ? OK : FAIL;
}

/*
 * Function passed to qsort() for sorting proptype_T on pt_id.
 */
    static int
compare_pt(const void *s1, const void *s2)
{
    proptype_T	*tp1 = *(proptype_T **)s1;
    proptype_T	*tp2 = *(proptype_T **)s2;

    return tp1->pt_id == tp2->pt_id ? 0 : tp1->pt_id < tp2->pt_id ? -1 : 1;
}

    static proptype_T *
find_type_by_id(hashtab_T *ht, proptype_T ***array, int id)
{
    int low = 0;
    int high;

    if (ht == NULL || ht->ht_used == 0)
	return NULL;

    // Make the lookup faster by creating an array with pointers to
    // hashtable entries, sorted on pt_id.
    if (*array == NULL)
    {
	long	    todo;
	hashitem_T  *hi;
	int	    i = 0;

	*array = ALLOC_MULT(proptype_T *, ht->ht_used);
	if (*array == NULL)
	    return NULL;
	todo = (long)ht->ht_used;
	FOR_ALL_HASHTAB_ITEMS(ht, hi, todo)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		(*array)[i++] = HI2PT(hi);
		--todo;
	    }
	}
	qsort((void *)*array, ht->ht_used, sizeof(proptype_T *), compare_pt);
    }

    // binary search in the sorted array
    high = ht->ht_used;
    while (high > low)
    {
	int m = (high + low) / 2;

	if ((*array)[m]->pt_id == id)
	    return (*array)[m];
	if ((*array)[m]->pt_id > id)
	    high = m;
	else
	    low = m + 1;
    }
    return NULL;
}

/*
 * Fill 'dict' with text properties in 'prop'.
 */
    static void
prop_fill_dict(dict_T *dict, textprop_T *prop, buf_T *buf)
{
    proptype_T *pt;
    int buflocal = TRUE;

    dict_add_number(dict, "col", PROP_IS_FLOATING(prop) ? 0 : prop->tp_col);
    if (prop->tp_text == NULL)
    {
	dict_add_number(dict, "length", prop->tp_len);
	dict_add_number(dict, "id", prop->tp_id);
    }
    dict_add_number(dict, "start", !(prop->tp_flags & TP_FLAG_CONT_PREV));
    dict_add_number(dict, "end", !(prop->tp_flags & TP_FLAG_CONT_NEXT));

    pt = find_type_by_id(buf->b_proptypes, &buf->b_proparray, prop->tp_type);
    if (pt == NULL)
    {
	pt = find_type_by_id(global_proptypes, &global_proparray,
								prop->tp_type);
	buflocal = FALSE;
    }
    if (pt != NULL)
	dict_add_string(dict, "type", pt->pt_name);

    if (buflocal)
	dict_add_number(dict, "type_bufnr", buf->b_fnum);
    else
	dict_add_number(dict, "type_bufnr", 0);
    if (prop->tp_text != NULL)
    {
	dict_add_string(dict, "text", prop->tp_text);

	char_u *text_align = NULL;
	if (PROP_IS_RIGHT(prop))
	    text_align = (char_u *)"right";
	else if (PROP_IS_ABOVE(prop))
	    text_align = (char_u *)"above";
	else if (PROP_IS_BELOW(prop))
	    text_align = (char_u *)"below";
	if (text_align != NULL)
	    dict_add_string(dict, "text_align", text_align);

	if (prop->tp_flags & TP_FLAG_WRAP)
	    dict_add_string(dict, "text_wrap", (char_u *)"wrap");
	if (prop->tp_padleft != 0)
	    dict_add_number(dict, "text_padding_left", prop->tp_padleft);
    }
}

/*
 * Find a property type by ID in "buf" or globally.
 * Returns NULL if not found.
 */
    proptype_T *
text_prop_type_by_id(buf_T *buf, int id)
{
    proptype_T *type;

    type = find_type_by_id(buf->b_proptypes, &buf->b_proparray, id);
    if (type == NULL)
	type = find_type_by_id(global_proptypes, &global_proparray, id);
    return type;
}

/*
 * Return TRUE if "prop" is a valid text property type.
 */
    int
text_prop_type_valid(buf_T *buf, textprop_T *prop)
{
    return text_prop_type_by_id(buf, prop->tp_type) != NULL;
}

/*
 * prop_clear({lnum} [, {lnum_end} [, {bufnr}]])
 */
    void
f_prop_clear(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_opt_number_arg(argvars, 1) == FAIL
		|| (argvars[1].v_type != VAR_UNKNOWN
		    && check_for_opt_dict_arg(argvars, 2) == FAIL)))
	return;

    const linenr_T start = tv_get_number(&argvars[0]);
    linenr_T end = start;
    buf_T    *buf = curbuf;
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	end = tv_get_number(&argvars[1]);
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    if (get_bufnr_from_arg(&argvars[2], &buf) == FAIL)
		return;
	}
    }
    if (start < 1 || end < 1)
    {
	emsg(_(e_invalid_range));
	return;
    }

    prop_remove_info_T info = remove_props(
	buf, start, end, MATCH_ALL_PROPS, DO_ALL);
    if (info.removed_count > 0)
	redraw_buf_later(buf, UPD_NOT_VALID);
}

/*
 * prop_find({props} [, {direction}])
 */
    void
f_prop_find(typval_T *argvars, typval_T *rettv)
{
    pos_T       *cursor = &curwin->w_cursor;
    dict_T      *dict;
    buf_T       *buf = curbuf;
    dictitem_T  *di;
    int		lnum_start;
    int		start_pos_has_prop = 0;
    int		seen_end = FALSE;
    int		id = 0;
    int		id_found = FALSE;
    int		type_id = -1;
    int		skipstart = FALSE;
    int		lnum = -1;
    int		col = -1;
    int		dir = FORWARD;    // FORWARD == 1, BACKWARD == -1
    int		both;

    if (in_vim9script()
	    && (check_for_dict_arg(argvars, 0) == FAIL
		|| check_for_opt_string_arg(argvars, 1) == FAIL))
	return;

    if (check_for_nonnull_dict_arg(argvars, 0) == FAIL)
	return;
    dict = argvars[0].vval.v_dict;

    if (get_bufnr_from_arg(&argvars[0], &buf) == FAIL)
	return;
    if (buf->b_ml.ml_mfp == NULL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	char_u      *dir_s = tv_get_string(&argvars[1]);

	if (*dir_s == 'b')
	    dir = BACKWARD;
	else if (*dir_s != 'f')
	{
	    emsg(_(e_invalid_argument));
	    return;
	}
    }

    di = dict_find(dict, (char_u *)"lnum", -1);
    if (di != NULL)
	lnum = tv_get_number(&di->di_tv);

    di = dict_find(dict, (char_u *)"col", -1);
    if (di != NULL)
	col = tv_get_number(&di->di_tv);

    if (lnum == -1)
    {
	lnum = cursor->lnum;
	col = cursor->col + 1;
    }
    else if (col == -1)
	col = 1;

    if (lnum < 1 || lnum > buf->b_ml.ml_line_count)
    {
	emsg(_(e_invalid_range));
	return;
    }

    skipstart = dict_get_bool(dict, "skipstart", 0);

    if (dict_has_key(dict, "id"))
    {
	id = dict_get_number(dict, "id");
	id_found = TRUE;
    }
    if (dict_has_key(dict, "type"))
    {
	char_u	    *name = dict_get_string(dict, "type", FALSE);
	proptype_T  *type = lookup_prop_type(name, buf);

	if (type == NULL)
	    return;
	type_id = type->pt_id;
    }
    both = dict_get_bool(dict, "both", FALSE);
    if (!id_found && type_id == -1)
    {
	emsg(_(e_need_at_least_one_of_id_or_type));
	return;
    }
    if (both && (!id_found || type_id == -1))
    {
	emsg(_(e_need_id_and_type_or_types_with_both));
	return;
    }

    lnum_start = lnum;

    if (rettv_dict_alloc(rettv) == FAIL)
	return;

    int                delta = dir == BACKWARD ? -1 : 1;
    unpacked_memline_T umemline = um_open(buf);
    while (1)
    {
	um_goto_line(&umemline, lnum, 0);

	int	 count = umemline.prop_count;
	unsigned start = dir == BACKWARD ? count - 1 : 0;

	for (int i = start; i >= 0 && i < count; i += delta)
	{
	    textprop_T *prop = &umemline.props[i];

	    // For the very first line try to find the first property before or
	    // after `col`, depending on the search direction.
	    if (lnum == lnum_start)
	    {
		if (dir == BACKWARD)
		{
		    if (prop->tp_col > col)
			continue;
		}
		else if (prop->tp_col + prop->tp_len - (prop->tp_len != 0) < col)
		    continue;
	    }
	    if (both ? prop->tp_id == id && prop->tp_type == type_id
		     : (id_found && prop->tp_id == id)
						    || prop->tp_type == type_id)
	    {
		// Check if the starting position has text props.
		if (lnum_start == lnum
			&& col >= prop->tp_col
			&& (col <= prop->tp_col + prop->tp_len
							 - (prop->tp_len != 0)))
		    start_pos_has_prop = 1;

		// The property was not continued from last line, it starts on
		// this line.
		int prop_start = !(prop->tp_flags & TP_FLAG_CONT_PREV);

		// The property does not continue on the next line, it ends on
		// this line.
		int prop_end = !(prop->tp_flags & TP_FLAG_CONT_NEXT);
		if (!prop_start && prop_end && dir == FORWARD)
		    seen_end = 1;

		// Skip lines without the start flag.
		if (!prop_start)
		{
		    // Always search backwards for start when search started
		    // on a prop and we're not skipping.
		    if (start_pos_has_prop && !skipstart)
		    {
			dir = BACKWARD;
			delta = -1;
		    }
		    continue;
		}

		// If skipstart is true, skip the prop at start pos (even if
		// continued from another line).
		if (start_pos_has_prop && skipstart && !seen_end)
		{
		    start_pos_has_prop = 0;
		    continue;
		}

		prop_fill_dict(rettv->vval.v_dict, prop, buf);
		dict_add_number(rettv->vval.v_dict, "lnum", lnum);

		return;
	    }
	}

	if (dir == FORWARD)
	{
	    if (lnum >= buf->b_ml.ml_line_count)
		break;
	}
	else
	{
	    if (lnum <= 1)
		break;
	}
	lnum += delta;
    }
    um_abort(&umemline);
}

/*
 * Returns TRUE if 'type_or_id' is in the 'types_or_ids' list.
 */
    static int
prop_type_or_id_in_list(int *types_or_ids, int len, int type_or_id)
{
    int i;

    for (i = 0; i < len; i++)
	if (types_or_ids[i] == type_or_id)
	    return TRUE;

    return FALSE;
}

/*
 * Return all the text properties in line 'lnum' in buffer 'buf' in 'retlist'.
 * If 'prop_types' is not NULL, then return only the text properties with
 * matching property type in the 'prop_types' array.
 * If 'prop_ids' is not NULL, then return only the text properties with
 * an identifier in the 'props_ids' array.
 * If 'add_lnum' is TRUE, then add the line number also to the text property
 * dictionary.
 */
    static void
get_props_in_line(
	buf_T		*buf,
	linenr_T	lnum,
	int		*prop_types,
	int		prop_types_len,
	int		*prop_ids,
	int		prop_ids_len,
	list_T		*retlist,
	int		add_lnum)
{
    unpacked_memline_T umemline = um_open(buf);
    um_goto_line(&umemline, lnum, 0);
    for (int i = 0; i < umemline.prop_count; i++)
    {
	textprop_T *prop = &umemline.props[i];
	if ((prop_types == NULL
		    || prop_type_or_id_in_list(prop_types, prop_types_len,
			prop->tp_type))
		&& (prop_ids == NULL
		    || prop_type_or_id_in_list(prop_ids, prop_ids_len,
								 prop->tp_id)))
	{
	    dict_T *d = dict_alloc();

	    if (d == NULL)
		break;
	    prop_fill_dict(d, prop, buf);
	    if (add_lnum)
		dict_add_number(d, "lnum", lnum);
	    list_append_dict(retlist, d);
	}
    }
    um_close(&umemline);
}

/*
 * Convert a List of property type names into an array of property type
 * identifiers. Returns a pointer to the allocated array. Returns NULL on
 * error. 'num_types' is set to the number of returned property types.
 */
    static int *
get_prop_types_from_names(list_T *l, buf_T *buf, int *num_types)
{
    int		*prop_types;
    listitem_T	*li;
    int		i;
    char_u	*name;
    proptype_T	*type;

    *num_types = 0;

    prop_types = ALLOC_MULT(int, list_len(l));
    if (prop_types == NULL)
	return NULL;

    i = 0;
    FOR_ALL_LIST_ITEMS(l, li)
    {
	if (li->li_tv.v_type != VAR_STRING)
	{
	    emsg(_(e_string_required));
	    goto errret;
	}
	name = li->li_tv.vval.v_string;
	if (name == NULL)
	    goto errret;

	type = lookup_prop_type(name, buf);
	if (type == NULL)
	    goto errret;
	prop_types[i++] = type->pt_id;
    }

    *num_types = i;
    return prop_types;

errret:
    VIM_CLEAR(prop_types);
    return NULL;
}

/*
 * Convert a List of property identifiers into an array of property
 * identifiers.  Returns a pointer to the allocated array. Returns NULL on
 * error. 'num_ids' is set to the number of returned property identifiers.
 */
    static int *
get_prop_ids_from_list(list_T *l, int *num_ids)
{
    int		*prop_ids;
    listitem_T	*li;
    int		i = 0;
    int		id;
    int		error;

    *num_ids = 0;

    prop_ids = ALLOC_MULT(int, list_len(l));
    if (prop_ids == NULL)
	return NULL;

    CHECK_LIST_MATERIALIZE(l);
    FOR_ALL_LIST_ITEMS(l, li)
    {
	error = FALSE;
	id = tv_get_number_chk(&li->li_tv, &error);
	if (error)
	    goto errret;

	prop_ids[i++] = id;
    }

    *num_ids = i;
    return prop_ids;

errret:
    VIM_CLEAR(prop_ids);
    return NULL;
}

/*
 * prop_list({lnum} [, {bufnr}])
 */
    void
f_prop_list(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;
    linenr_T	start_lnum;
    linenr_T	end_lnum;
    buf_T	*buf = curbuf;
    int		add_lnum = FALSE;
    int		*prop_types = NULL;
    int		prop_types_len = 0;
    int		*prop_ids = NULL;
    int		prop_ids_len = 0;
    list_T	*l;
    dictitem_T	*di;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_opt_dict_arg(argvars, 1) == FAIL))
	return;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    // default: get text properties on current line
    start_lnum = tv_get_number(&argvars[0]);
    end_lnum = start_lnum;
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	dict_T *d;

	if (check_for_dict_arg(argvars, 1) == FAIL)
	    return;
	d = argvars[1].vval.v_dict;

	if (get_bufnr_from_arg(&argvars[1], &buf) == FAIL)
	    return;

	if (d != NULL && (di = dict_find(d, (char_u *)"end_lnum", -1)) != NULL)
	{
	    if (di->di_tv.v_type != VAR_NUMBER)
	    {
		emsg(_(e_number_required));
		return;
	    }
	    end_lnum = tv_get_number(&di->di_tv);
	    if (end_lnum < 0)
		// negative end_lnum is used as an offset from the last buffer
		// line
		end_lnum = buf->b_ml.ml_line_count + end_lnum + 1;
	    else if (end_lnum > buf->b_ml.ml_line_count)
		end_lnum = buf->b_ml.ml_line_count;
	    add_lnum = TRUE;
	}
	if (d != NULL && (di = dict_find(d, (char_u *)"types", -1)) != NULL)
	{
	    if (di->di_tv.v_type != VAR_LIST)
	    {
		emsg(_(e_list_required));
		return;
	    }

	    l = di->di_tv.vval.v_list;
	    if (l != NULL && list_len(l) > 0)
	    {
		prop_types = get_prop_types_from_names(l, buf, &prop_types_len);
		if (prop_types == NULL)
		    return;
	    }
	}
	if (d != NULL && (di = dict_find(d, (char_u *)"ids", -1)) != NULL)
	{
	    if (di->di_tv.v_type != VAR_LIST)
	    {
		emsg(_(e_list_required));
		goto errret;
	    }

	    l = di->di_tv.vval.v_list;
	    if (l != NULL && list_len(l) > 0)
	    {
		prop_ids = get_prop_ids_from_list(l, &prop_ids_len);
		if (prop_ids == NULL)
		    goto errret;
	    }
	}
    }
    if (start_lnum < 1 || start_lnum > buf->b_ml.ml_line_count
		|| end_lnum < 1 || end_lnum < start_lnum)
	emsg(_(e_invalid_range));
    else
	for (lnum = start_lnum; lnum <= end_lnum; lnum++)
	    get_props_in_line(buf, lnum, prop_types, prop_types_len,
		    prop_ids, prop_ids_len,
		    rettv->vval.v_list, add_lnum);

errret:
    VIM_CLEAR(prop_types);
    VIM_CLEAR(prop_ids);
}

/*
 * prop_remove({props} [, {lnum} [, {lnum_end}]])
 */
    void
f_prop_remove(typval_T *argvars, typval_T *rettv)
{
    linenr_T           start = 1;
    linenr_T           end = 0;
    dict_T             *dict;
    buf_T              *buf = curbuf;
    int	               do_all;
    criteria_T         criteria = {-MAXCOL, 0, NULL, -1, FALSE};
    prop_remove_info_T info;

    rettv->vval.v_number = 0;

    if (in_vim9script()
	    && (check_for_dict_arg(argvars, 0) == FAIL
		|| check_for_opt_number_arg(argvars, 1) == FAIL
		|| (argvars[1].v_type != VAR_UNKNOWN
		    && check_for_opt_number_arg(argvars, 2) == FAIL)))
	return;

    if (check_for_nonnull_dict_arg(argvars, 0) == FAIL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	start = tv_get_number(&argvars[1]);
	end = start;
	if (argvars[2].v_type != VAR_UNKNOWN)
	    end = tv_get_number(&argvars[2]);
	if (start < 1 || end < 1)
	{
	    emsg(_(e_invalid_range));
	    return;
	}
    }
    dict = argvars[0].vval.v_dict;
    if (get_bufnr_from_arg(&argvars[0], &buf) == FAIL)
	return;
    if (buf->b_ml.ml_mfp == NULL)
	return;

    if (end == 0)
	end = buf->b_ml.ml_line_count;
    else if (end > buf->b_ml.ml_line_count)
	end = buf->b_ml.ml_line_count;
    do_all = dict_get_bool(dict, "all", FALSE);
    criteria.both = dict_get_bool(dict, "both", FALSE);
    if (dict_has_key(dict, "id"))
	criteria.id = dict_get_number(dict, "id");

    // Do an early check that both 'type' and 'types' are not specified to
    // simplify later logic.
    if (dict_has_key(dict, "type") && dict_has_key(dict, "types"))
    {
	emsg(_(e_cannot_specify_both_type_and_types));
	return;
    }

    if (dict_has_key(dict, "type"))
    {
	char_u	    *name = dict_get_string(dict, "type", FALSE);
	proptype_T  *type = lookup_prop_type(name, buf);

	if (type == NULL)
	    return;

	criteria.type_ids = alloc(sizeof(int));
	if (criteria.type_ids == NULL)
	    return;

	criteria.type_ids[0] = type->pt_id;
	criteria.num_type_ids = 1;
    }
    else if (dict_has_key(dict, "types"))
    {
	typval_T types;
	listitem_T *li = NULL;
	int num_ids = 0;

	dict_get_tv(dict, "types", &types);
	if (types.v_type == VAR_LIST && types.vval.v_list->lv_len > 0)
	{
	    criteria.type_ids = alloc(sizeof(int) * types.vval.v_list->lv_len);

	    FOR_ALL_LIST_ITEMS(types.vval.v_list, li)
	    {
		proptype_T *prop_type;

		if (li->li_tv.v_type != VAR_STRING)
		    continue;

		prop_type = lookup_prop_type(li->li_tv.vval.v_string, buf);
		if (!prop_type)
		    goto cleanup_prop_remove;

		criteria.type_ids[num_ids++] = prop_type->pt_id;
	    }
	    criteria.num_type_ids = num_ids;
	}
    }
    if (criteria.id == -MAXCOL && criteria.num_type_ids == 0)
    {
	emsg(_(e_need_at_least_one_of_id_or_type));
	goto cleanup_prop_remove;
    }
    if (criteria.both && (criteria.id == -MAXCOL || criteria.num_type_ids == 0))
    {
	emsg(_(e_need_id_and_type_or_types_with_both));
	goto cleanup_prop_remove;
    }

    info = remove_props(buf, start, end, &criteria, do_all);
    if (info.first_changed > 0)
    {
	changed_line_display_buf(buf);
	changed_lines_buf(buf, info.first_changed, info.last_changed + 1, 0);
	redraw_buf_later(buf, UPD_VALID);
    }
    rettv->vval.v_number = info.removed_count;

cleanup_prop_remove:
    vim_free(criteria.type_ids);
}

/*
 * Common for f_prop_type_add() and f_prop_type_change().
 */
    static void
prop_type_set(typval_T *argvars, int add)
{
    char_u	*name;
    buf_T	*buf = NULL;
    dict_T	*dict;
    dictitem_T  *di;
    proptype_T	*prop;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_dict_arg(argvars, 1) == FAIL))
	return;

    name = tv_get_string(&argvars[0]);
    if (*name == NUL)
    {
	semsg(_(e_invalid_argument_str), "\"\"");
	return;
    }

    if (get_bufnr_from_arg(&argvars[1], &buf) == FAIL)
	return;
    dict = argvars[1].vval.v_dict;

    prop = find_prop_type(name, buf);
    if (add)
    {
	hashtab_T **htp;

	if (prop != NULL)
	{
	    semsg(_(e_property_type_str_already_defined), name);
	    return;
	}
	prop = alloc_clear(offsetof(proptype_T, pt_name) + STRLEN(name) + 1);
	if (prop == NULL)
	    return;
	STRCPY(prop->pt_name, name);
	prop->pt_id = ++proptype_id;
	prop->pt_flags = PT_FLAG_COMBINE;
	if (buf == NULL)
	{
	    htp = &global_proptypes;
	    VIM_CLEAR(global_proparray);
	}
	else
	{
	    htp = &buf->b_proptypes;
	    VIM_CLEAR(buf->b_proparray);
	}
	if (*htp == NULL)
	{
	    *htp = ALLOC_ONE(hashtab_T);
	    if (*htp == NULL)
	    {
		vim_free(prop);
		return;
	    }
	    hash_init(*htp);
	}
	hash_add(*htp, PT2HIKEY(prop), "prop type");
    }
    else
    {
	if (prop == NULL)
	{
	    semsg(_(e_property_type_str_does_not_exist), name);
	    return;
	}
    }

    if (dict != NULL)
    {
	di = dict_find(dict, (char_u *)"highlight", -1);
	if (di != NULL)
	{
	    char_u	*highlight;
	    int		hl_id = 0;

	    highlight = dict_get_string(dict, "highlight", FALSE);
	    if (highlight != NULL && *highlight != NUL)
		hl_id = syn_name2id(highlight);
	    if (hl_id <= 0)
	    {
		semsg(_(e_unknown_highlight_group_name_str),
			highlight == NULL ? (char_u *)"" : highlight);
		return;
	    }
	    prop->pt_hl_id = hl_id;
	}

	di = dict_find(dict, (char_u *)"combine", -1);
	if (di != NULL)
	{
	    if (tv_get_bool(&di->di_tv))
		prop->pt_flags |= PT_FLAG_COMBINE;
	    else
		prop->pt_flags &= ~PT_FLAG_COMBINE;
	}

	di = dict_find(dict, (char_u *)"override", -1);
	if (di != NULL)
	{
	    if (tv_get_bool(&di->di_tv))
		prop->pt_flags |= PT_FLAG_OVERRIDE;
	    else
		prop->pt_flags &= ~PT_FLAG_OVERRIDE;
	}

	di = dict_find(dict, (char_u *)"priority", -1);
	if (di != NULL)
	    prop->pt_priority = tv_get_number(&di->di_tv);

	di = dict_find(dict, (char_u *)"start_incl", -1);
	if (di != NULL)
	{
	    if (tv_get_bool(&di->di_tv))
		prop->pt_flags |= PT_FLAG_INS_START_INCL;
	    else
		prop->pt_flags &= ~PT_FLAG_INS_START_INCL;
	}

	di = dict_find(dict, (char_u *)"end_incl", -1);
	if (di != NULL)
	{
	    if (tv_get_bool(&di->di_tv))
		prop->pt_flags |= PT_FLAG_INS_END_INCL;
	    else
		prop->pt_flags &= ~PT_FLAG_INS_END_INCL;
	}
    }
}

/*
 * prop_type_add({name}, {props})
 */
    void
f_prop_type_add(typval_T *argvars, typval_T *rettv UNUSED)
{
    prop_type_set(argvars, TRUE);
}

/*
 * prop_type_change({name}, {props})
 */
    void
f_prop_type_change(typval_T *argvars, typval_T *rettv UNUSED)
{
    prop_type_set(argvars, FALSE);
}

/*
 * prop_type_delete({name} [, {bufnr}])
 */
    void
f_prop_type_delete(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u	*name;
    buf_T	*buf = NULL;
    hashitem_T	*hi;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_opt_dict_arg(argvars, 1) == FAIL))
	return;

    name = tv_get_string(&argvars[0]);
    if (*name == NUL)
    {
	semsg(_(e_invalid_argument_str), "\"\"");
	return;
    }

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (get_bufnr_from_arg(&argvars[1], &buf) == FAIL)
	    return;
    }

    hi = find_prop_type_hi(name, buf);
    if (hi == NULL)
	return;

    hashtab_T	*ht;
    proptype_T	*prop = HI2PT(hi);

    if (buf == NULL)
    {
	ht = global_proptypes;
	VIM_CLEAR(global_proparray);
    }
    else
    {
	ht = buf->b_proptypes;
	VIM_CLEAR(buf->b_proparray);
    }
    hash_remove(ht, hi, "prop type delete");
    vim_free(prop);

    // currently visible text properties will disappear
    redraw_all_later(UPD_CLEAR);
    changed_window_setting_buf(buf == NULL ? curbuf : buf);
}

/*
 * prop_type_get({name} [, {props}])
 */
    void
f_prop_type_get(typval_T *argvars, typval_T *rettv)
{
    char_u *name;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_opt_dict_arg(argvars, 1) == FAIL))
	return;

    name = tv_get_string(&argvars[0]);
    if (*name == NUL)
    {
	semsg(_(e_invalid_argument_str), "\"\"");
	return;
    }

    if (rettv_dict_alloc(rettv) == FAIL)
	return;

    proptype_T  *prop = NULL;
    buf_T	    *buf = NULL;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (get_bufnr_from_arg(&argvars[1], &buf) == FAIL)
	    return;
    }

    prop = find_prop_type(name, buf);
    if (prop == NULL)
	return;

    dict_T *d = rettv->vval.v_dict;

    if (prop->pt_hl_id > 0)
	dict_add_string(d, "highlight", syn_id2name(prop->pt_hl_id));
    dict_add_number(d, "priority", prop->pt_priority);
    dict_add_number(d, "combine",
	    (prop->pt_flags & PT_FLAG_COMBINE) ? 1 : 0);
    dict_add_number(d, "start_incl",
	    (prop->pt_flags & PT_FLAG_INS_START_INCL) ? 1 : 0);
    dict_add_number(d, "end_incl",
	    (prop->pt_flags & PT_FLAG_INS_END_INCL) ? 1 : 0);
    if (buf != NULL)
	dict_add_number(d, "bufnr", buf->b_fnum);
}

    static void
list_types(hashtab_T *ht, list_T *l)
{
    long	todo;
    hashitem_T	*hi;

    todo = (long)ht->ht_used;
    FOR_ALL_HASHTAB_ITEMS(ht, hi, todo)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    proptype_T *prop = HI2PT(hi);

	    list_append_string(l, prop->pt_name, -1);
	    --todo;
	}
    }
}

/*
 * prop_type_list([{bufnr}])
 */
    void
f_prop_type_list(typval_T *argvars, typval_T *rettv)
{
    buf_T *buf = NULL;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    if (in_vim9script() && check_for_opt_dict_arg(argvars, 0) == FAIL)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	if (get_bufnr_from_arg(&argvars[0], &buf) == FAIL)
	    return;
    }
    if (buf == NULL)
    {
	if (global_proptypes != NULL)
	    list_types(global_proptypes, rettv->vval.v_list);
    }
    else if (buf->b_proptypes != NULL)
	list_types(buf->b_proptypes, rettv->vval.v_list);
}

/*
 * Free all property types in "ht".
 */
    static void
clear_ht_prop_types(hashtab_T *ht)
{
    long	todo;
    hashitem_T	*hi;

    if (ht == NULL)
	return;

    todo = (long)ht->ht_used;
    FOR_ALL_HASHTAB_ITEMS(ht, hi, todo)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    proptype_T *prop = HI2PT(hi);

	    vim_free(prop);
	    --todo;
	}
    }

    hash_clear(ht);
    vim_free(ht);
}

#if defined(EXITFREE)
/*
 * Free all global property types.
 */
    void
clear_global_prop_types(void)
{
    clear_ht_prop_types(global_proptypes);
    global_proptypes = NULL;
    VIM_CLEAR(global_proparray);
}
#endif

/*
 * Free all property types for "buf".
 */
    void
clear_buf_prop_types(buf_T *buf)
{
    clear_ht_prop_types(buf->b_proptypes);
    buf->b_proptypes = NULL;
    VIM_CLEAR(buf->b_proparray);
}

// Struct used to return two values from adjust_prop().
typedef struct
{
    int dirty;	    // if the property was changed
    int can_drop;   // whether after this change, the prop may be removed
} adjustres_T;

/*
 * Adjust a property to reflect truncation after and including "col".
 *
 * The "col" is 1-based.
 *
 * The caller must not invoke this for 'floating' virtual text properties.
 */
    static prop_mod_T
trim_trailing_bytes_1(
	textprop_T *prop,
	colnr_T    col,
	colnr_T    count UNUSED)
{
    if (PROP_IS_VTEXT(prop))
    {
	if (prop->tp_col >= col + 1)
	{
	    prop->tp_col = col + 1;
	    return PC_MODIFIED;
	}
	return PC_UNMODIFIED;
    }

    if (prop->tp_col >= col)
	return PC_RQ_DELETE;

    const colnr_T prop_end_col = prop->tp_col + prop_length(prop);
    if (col >= prop_end_col)
	return PC_UNMODIFIED;

    prop->tp_len -= prop_end_col - col;
    return PC_MODIFIED;
}

/*
 * Adjust a property to reflect removal of "count" leading bytes.
 *
 * The caller must not invoke this for virtual text properties.
 */
    static prop_mod_T
trim_leading_bytes_1(
	textprop_T *prop,
	colnr_T    col UNUSED,
	colnr_T    count)
{
    const colnr_T prop_end_col = prop->tp_col + prop_length(prop);

    if (PROP_IS_VTEXT(prop))
    {
	int prev_tp_col = prop->tp_col;
	prop->tp_col = MAX(0, prop->tp_col - count);
	return prop->tp_col != prev_tp_col ? PC_MODIFIED : PC_UNMODIFIED;
    }

    if (prop_end_col <= count + 1)
	return PC_RQ_DELETE;

    if (prop->tp_col > count)
    {
	prop->tp_col -= count;
    }
    else
    {
	prop->tp_len -= count - prop->tp_col + 1;
	prop->tp_col = 1;
    }
    return PC_MODIFIED;
}

/*
 * Adjust a property to reflect removal of "count" bytes starting at "col".
 *
 * The "col" is 1-based.
 *
 * The caller must not invoke this for 'floating' virtual text properties.
 */
    static prop_mod_T
remove_bytes_1(
	textprop_T *prop,
	colnr_T    col,
	colnr_T    count)
{
    const colnr_T last_prop_byte = prop->tp_col + prop_length(prop) - 1;
    colnr_T last_deleted_byte = col + count - 1;

    if (PROP_IS_VTEXT(prop))
    {
	if (prop->tp_col > col)
	{
	    if (last_deleted_byte >= prop->tp_col)
	    {
		return PC_RQ_DELETE;
	    }

	    if (prop->tp_col <= last_deleted_byte)
		prop->tp_col = col;
	    else
		prop->tp_col -= count;
	    return PC_MODIFIED;
	}
	return PC_UNMODIFIED;
    }

    if (last_prop_byte < col)
	// The property is before the first deleted byte.
	return PC_UNMODIFIED;

    if (prop->tp_col > last_deleted_byte)
    {
	// The property is after the last deleted byte.
	prop->tp_col -= count;
	return PC_MODIFIED;
    }

    if (prop->tp_col >= col && last_prop_byte <= last_deleted_byte)
	// All bytes of the property are deleted.
	return PC_RQ_DELETE;

    // The range of deleted bytes and the property partially overlap.
    if (last_deleted_byte <= last_prop_byte)
    {
	if (prop->tp_col <= col)
	    prop->tp_len -= count;
	else
	    prop->tp_len = last_prop_byte - last_deleted_byte;
    }
    if (col < prop->tp_col)
	prop->tp_col = col;
    return PC_MODIFIED;
}

/*
 * Adjust a property to reflect adding of "count" bytes after "col".
 *
 * The "col" is 1-based. When "col" is zero, bytes are inserted at the start of
 * the line.
 *
 * The caller must not invoke this for 'floating' virtual text properties.
 */
    static prop_mod_T
add_bytes_1(
	textprop_T *prop,
	colnr_T    col,
	colnr_T    count)
{
    if (PROP_IS_VTEXT(prop))
    {
	if (prop->tp_col > col)
	{
	    prop->tp_col += count;
	    return PC_MODIFIED;
	}
	return PC_UNMODIFIED;
    }

    const colnr_T prop_end_col = prop->tp_col + prop_length(prop);
    if (col >= prop_end_col)
	// Adding after the end of this property.
	return PC_UNMODIFIED;

    const proptype_T *prop_type = text_prop_type_by_id(curbuf, prop->tp_type);
    bool start_incl = prop_type != NULL
		    ? (prop_type->pt_flags & PT_FLAG_INS_START_INCL) != 0
		    : FALSE;
    bool end_incl = prop_type != NULL
		    ? (prop_type->pt_flags & PT_FLAG_INS_END_INCL) != 0
		    : FALSE;

    if (col == prop->tp_col - 1 && start_incl)
	// Adding at the start of this property with start_incl set.
	prop->tp_len += count;

    else if (col == prop_end_col - 1 && end_incl)
	// Adding at the end of this property with end_incl set.
	prop->tp_len += count;

    else if (col < prop->tp_col)
	// Adding before the start of this property.
	prop->tp_col += count;

    else if (col < prop_end_col - 1)
	// Adding within the bounds of this property.
	prop->tp_len += count;
    else
	return PC_UNMODIFIED;

    return PC_MODIFIED;
}

/*
 * Adjust properties on a given line.
 *
 * The provided "adjust" function performs that required adjustment for each
 * property, returning PC_UNMODIFIED, PC_MODIFIED or PC_RQ_DELETE as appropriate.
 *
 * The provided "flags" may have the APC_SAVE_FOR_UNDO bit set, in which case
 * "u_save" is invoked before any property changes are stored in the line and
 * then APC_SAVE_FOR_UNDO is cleared.
 */
    static void
perform_properties_adjustment(
	buf_T	   *buf,
	linenr_T   lnum,
	colnr_T    col,
	colnr_T    count,
	bool	   keep_empty,
	int	   *flags,
	prop_mod_T (*adjust)(textprop_T *, colnr_T, colnr_T))
{
    unpacked_memline_T umemline = um_open_at(buf, lnum, 0);
    if (umemline.buf == NULL)
	return;

    for (int i = 0; i < umemline.prop_count; i++)
    {
	textprop_T *prop = &umemline.props[i];
	if (PROP_IS_FLOATING(prop))
	    continue;

	int is_vtext = PROP_IS_VTEXT(prop);
	prop_mod_T result = adjust(prop, col, count);
	if (result != PC_UNMODIFIED && !um_detach(&umemline))
	    break;
	if (result == PC_RQ_DELETE)
	{
	    if (keep_empty && (prop->tp_flags & TP_FLAG_CONT_BITS) != 0)
	    {
		if (!is_vtext)
		    prop->tp_len = 0;
		prop->tp_col = 1;
		result = PC_MODIFIED;
	    }
	}
	if (result == PC_RQ_DELETE || result == PC_MODIFIED)
	{
	    if (*flags & APC_SAVE_FOR_UNDO)
	    {
		if (u_save(lnum - 1, lnum + 1) == FAIL)
		    // Cannot save for undo. No properties can be changed for
		    // this line.
		    return;
		*flags &= ~APC_SAVE_FOR_UNDO;
	    }
	}
	if (result == PC_RQ_DELETE)
	    um_delete_prop(&umemline, i);
    }

    um_close(&umemline);
}

/*
 * Adjust a line's properties to reflect truncation after and including "col".
 * If "keep_empty" is set then properties that continue on the next or previous
 * line are kept even they become empty.
 *
 * The "lnum" and "col" are 1-based.
 */
    void
prop_trim_trailing_bytes(
	buf_T	 *buf,
	linenr_T lnum,
	colnr_T  col,
	int	 *flags,
	bool	 keep_empty)
{
    perform_properties_adjustment(
	buf, lnum, col, 0, keep_empty, flags, trim_trailing_bytes_1);
}

/*
 * Adjust a line's properties to reflect removal of "count" leading bytes.
 */
    void
prop_trim_leading_bytes(
	buf_T	       *buf,
	const linenr_T lnum,
	const colnr_T  count,
	int	       *flags)
{
    perform_properties_adjustment(
	buf, lnum, 0, count, FALSE, flags, trim_leading_bytes_1);
}

/*
 * Adjust a line's properties to reflect reomving of "count" starting at "col".
 *
 * The "col" is 1-based.
 */
    void
prop_remove_bytes(
	buf_T	 *buf,
	linenr_T lnum,
	colnr_T  col,
	colnr_T  count,
	int	 *flags)
{
    perform_properties_adjustment(
	buf, lnum, col, count, FALSE, flags, remove_bytes_1);
}

/*
 * Adjust a line's properties to reflect adding of "count" bytes after "col".
 *
 * The "col" is 1-based. When "col" is zero, bytes are inserted at the start of
 * the line.
 */
    void
prop_add_bytes(
	buf_T	 *buf,
	linenr_T lnum,
	colnr_T  col,
	colnr_T  count,
	int	 *flags)
{
    perform_properties_adjustment(
	buf, lnum, col, count, FALSE, flags, add_bytes_1);
}

/*
 * Adjust the property for "added" bytes (can be negative) inserted at "col".
 *
 * Note that "col" is zero-based, while tp_col is one-based.
 * Only for the current buffer.
 * "flags" can have:
 * APC_SUBSTITUTE:	Text is replaced, not inserted.
 * APC_INDENT:		Text is inserted before virtual text prop
 */
    static adjustres_T
adjust_prop(
	textprop_T  *prop,
	colnr_T	    col,
	int	    added,
	int	    flags)
{
    proptype_T	*pt;
    bool	start_incl;
    bool	end_incl;
    adjustres_T res = {TRUE, FALSE};

    pt = text_prop_type_by_id(curbuf, prop->tp_type);
    if (PROP_IS_FLOATING(prop))
    {
	res.dirty = FALSE;
	return res;
    }

    if (PROP_IS_INLINE(prop))
    {
	if (added < 0 && col + 1 < prop->tp_col)
	{
	    if (col - added >= prop->tp_col)
	    {
		res.can_drop = TRUE;
		return res;
	    }
	}

	start_incl = pt != NULL && (prop->tp_flags & TP_FLAG_START_INCL) != 0;
	if (flags & APC_INDENT)
	    start_incl = FALSE;
	int col_pos = (start_incl && added >= 0) ? col + 2 : col + 1;
	if (added > 0)
	{
	    if (col_pos <= prop->tp_col)
	    {
		prop->tp_col += added;
		if (prop->tp_col < 1)
		    prop->tp_col = 1;
	    }
	}
	else if (added < 0)
	{
	    if (col_pos < prop->tp_col)
	    {
		prop->tp_col += added;
		if (prop->tp_col < 1)
		    prop->tp_col = 1;
	    }
	}
	else
	{
	    res.dirty = FALSE;
	}

	return res;
    }

    start_incl = (pt != NULL && (pt->pt_flags & PT_FLAG_INS_START_INCL))
				|| (flags & APC_SUBSTITUTE)
				|| (prop->tp_flags & TP_FLAG_CONT_PREV);
    end_incl = (pt != NULL && (pt->pt_flags & PT_FLAG_INS_END_INCL))
				|| (prop->tp_flags & TP_FLAG_CONT_NEXT);
    bool cont_next = (prop->tp_flags & TP_FLAG_CONT_NEXT) != 0;
    if (added > 0)
    {
	int col_adjust = (start_incl || (prop->tp_len == 0 && end_incl))
		       ? 1 : 0;
	if (col + 1 <= prop->tp_col - col_adjust)
	    // Change is entirely before the text property: Only shift
	    prop->tp_col += added;
	else if (col + 1 < prop->tp_col + prop->tp_len + end_incl)
	    // Insertion was inside text property
	    prop->tp_len += added;
    }
    else if (prop->tp_col > col + 1)
    {
	if (prop->tp_col + added < col + 1)
	{
	    prop->tp_len += (prop->tp_col - 1 - col) + added;
	    prop->tp_col = col + 1;
	    if (cont_next && prop->tp_len == 1)
		prop->tp_len = 0;
	    if (prop->tp_len <= 0)
	    {
		prop->tp_len = 0;
		res.can_drop = TRUE;
	    }
	}
	else
	    prop->tp_col += added;
    }
    else if (prop->tp_len > 0 && prop->tp_col + prop->tp_len > col)
    {
	int after = col - added - (prop->tp_col - 1 + prop->tp_len);

	prop->tp_len += after > 0 ? added + after : added;
	if (cont_next && prop->tp_len == 1)
	    prop->tp_len = 0;
	res.can_drop = prop->tp_len <= 0;
    }
    else
	res.dirty = FALSE;

    return res;
}

/*
 * Adjust the columns of text properties in line "lnum" after position "col" to
 * shift by "bytes_added" (can be negative).
 * Note that "col" is zero-based, while tp_col is one-based.
 * Only for the current buffer.
 * "flags" can have:
 * APC_SAVE_FOR_UNDO:	Call u_savesub() before making changes to the line.
 * APC_SUBSTITUTE:	Text is replaced, not inserted.
 * APC_INDENT:		Text is inserted before virtual text prop
 * Caller is expected to check b_has_textprop and "bytes_added" being non-zero.
 * Returns TRUE when props were changed.
 */
    int
adjust_prop_columns(
	linenr_T    lnum,
	colnr_T	    col,
	int	    bytes_added,
	int	    flags)
{
    if (text_prop_frozen > 0)
	return FALSE;

    unpacked_memline_T umemline = um_open_at(curbuf, lnum, 0);
    if (umemline.lnum == 0)
	return FALSE;

    int	dirty = FALSE;
    for (int ri = 0; ri < umemline.prop_count; ri++)
    {
	textprop_T  *prop = &umemline.props[ri];
	adjustres_T res = adjust_prop(prop, col, bytes_added, flags);
	if (res.dirty)
	{
	    if (!dirty)
	    {
		// Save for undo if requested and not done yet.
		if (flags & APC_SAVE_FOR_UNDO)
		    if (u_savesub(lnum) == FAIL)
			goto fail;

		if (!um_detach(&umemline))
		    goto fail;
		dirty = TRUE;
	    }
	}
	if (res.can_drop)
	    um_delete_prop(&umemline, ri);
    }
    um_close(&umemline);
    return dirty;

fail:
    um_abort(&umemline);
    return FALSE;
}

/*
 * Adjust text properties for a line that was split in two.
 * "lnum_props" is the line that has the properties from before the split.
 * "lnum_top" is the top line.
 * "kept" is the number of bytes kept in the first line, while
 * "deleted" is the number of bytes deleted.
 * "at_eol" is true if the split is after the end of the line.
 */
    void
adjust_props_for_split(
	linenr_T    lnum_props,
	linenr_T    lnum_top,
	int	    kept,
	int	    deleted,
	int	    at_eol)
{
    if (!curbuf->b_has_textprop)
	return;

    unpacked_memline_T umemline = um_open_at_detached(curbuf, lnum_props, 0);
    if (umemline.buf == NULL)
	goto fail2;

    unpacked_memline_T prev_umemline = um_open_at_no_props(
	curbuf, lnum_top, umemline.prop_count);
    unpacked_memline_T next_umemline = um_open_at_no_props(
	curbuf, lnum_top + 1, umemline.prop_count);
    if (next_umemline.buf == NULL || prev_umemline.buf == NULL)
	goto fail;

    // Keep the relevant ones in the first line, reducing the length if needed.
    // Copy the ones that include the split to the second line.
    // Move the ones after the split to the second line.
    int	skipped = kept + deleted;
    for (int i = 0; i < umemline.prop_count; i++)
    {
	textprop_T *prop = &umemline.props[i];
	proptype_T *pt;
	int	    start_incl, end_incl;
	int	    cont_prev, cont_next;
	bool	    is_vtext = PROP_IS_VTEXT(prop);
	bool	    is_floating = PROP_IS_FLOATING(prop);

	pt = text_prop_type_by_id(curbuf, prop->tp_type);
	start_incl = (pt != NULL && (pt->pt_flags & PT_FLAG_INS_START_INCL));
	end_incl = (pt != NULL && (pt->pt_flags & PT_FLAG_INS_END_INCL));

	if (PROP_IS_ABOVE(prop))
	{
	    // a text prop "above" behaves like it is on the first text column
	    cont_prev = 1 + !start_incl <= kept;
	    cont_next = skipped <= 1 - !end_incl;
	}
	else if (PROP_IS_FLOATING(prop))
	{
	    cont_prev = at_eol;
	    cont_next = !at_eol;
	}
	else
	{
	    cont_prev = prop->tp_col + !start_incl <= kept;
	    cont_next = skipped <= prop->tp_col + prop->tp_len - !end_incl;
	}
	// When a prop has text only one line gets it.
	if (PROP_IS_VTEXT(prop) && cont_next)
	    cont_prev = FALSE;

	if (cont_prev)
	{
	    textprop_T *p = &prev_umemline.props[prev_umemline.prop_count++];
	    *p = *prop;
	    prop->tp_text = NULL;  // V-text ownership is transferred.

	    if (!is_vtext && p->tp_col + p->tp_len >= kept)
		p->tp_len = kept - p->tp_col;
	    if (cont_next)
	    {
		p->tp_flags |= TP_FLAG_CONT_NEXT;
		if (!is_vtext)
		    p->tp_len += 1;
	    }
	}

	// Only add the property to the next line if the length is bigger than
	// zero.
	if (cont_next)
	{
	    textprop_T *p = &next_umemline.props[next_umemline.prop_count++];
	    *p = *prop;
	    prop->tp_text = NULL;  // V-text ownership is transferred.

	    if (!is_floating)
	    {
		if (p->tp_col > skipped)
		    p->tp_col -= skipped - 1;
		else
		{
		    if (!is_vtext)
			p->tp_len -= skipped - p->tp_col;
		    p->tp_col = 1;
		}
	    }
	    if (cont_prev)
		p->tp_flags |= TP_FLAG_CONT_PREV;
	}
    }

    um_close(&prev_umemline);
    um_close(&next_umemline);
    um_abort(&umemline);
    return;

fail:
    um_abort(&next_umemline);
    um_abort(&prev_umemline);
fail2:
    um_abort(&umemline);
}

/*
 * Prepend properties of joined line "lnum" to "new_props".
 */
    void
prepend_joined_props(
	unpacked_memline_T *umemline,
	linenr_T	   lnum,
	bool		   is_last_line,
	long		   col,
	int		   removed)
{

    if (umemline->buf == NULL)
	return;

    unpacked_memline_T r_umemline = um_open_at_detached(
	umemline->buf, lnum, 0);
    if (r_umemline.buf == NULL)
	goto fail;
    if (!um_add_space_for_props(umemline, r_umemline.prop_count))
	goto fail;

    // Add the line's properties in reverse order.
    textprop_T *props = umemline->props;
    for (int i = r_umemline.prop_count - 1; i >= 0; i--)
    {
	textprop_T *prop = &r_umemline.props[i];
	if (PROP_IS_FLOATING(prop) && !is_last_line)
	    continue;  // Drop floating text for leading lines.

	adjust_prop(prop, 0, -removed, 0); // Remove leading spaces
	adjust_prop(prop, -1, col, 0); // Make line start at its final column
	if (is_last_line || (prop->tp_flags & TP_FLAG_CONT_NEXT) == 0)
	{
	    um_add_prop_unsorted(umemline, prop);
	    prop->tp_text = NULL;  // Ownership has been transferred.
	}
	else
	{
	    // The copied property continues one we copied earlier. So find
	    // that one and adjust it.
	    bool found = FALSE;
	    for (int j = umemline->prop_count - 1; j >= 0; j--)
	    {
		textprop_T *op = &props[j];
		if ((op->tp_flags & TP_FLAG_CONT_PREV) == 0)
		    continue;
		if (op->tp_id != prop->tp_id || op->tp_type != prop->tp_type)
		    continue;

		// We have found the match.
		found = TRUE;
		if (op->tp_len == 0)
		{
		    if (op->tp_flags & TP_FLAG_CONT_NEXT)
			op->tp_len = prop->tp_len;
		    else
			op->tp_len = prop->tp_len - 1;
		}
		else
		{
		    op->tp_len += op->tp_col - prop->tp_col;
		}
		op->tp_col = prop->tp_col;
		if (!(prop->tp_flags & TP_FLAG_CONT_PREV))
		    op->tp_flags &= ~TP_FLAG_CONT_PREV;
		break;
	    }
	    if (!found)
	    {
		internal_error("text property above joined line not found");
		goto fail;
	    }
	}
    }
    um_abort(&r_umemline);
    return;

fail:
    um_abort(&r_umemline);
    um_abort(umemline);
}

#endif // FEAT_PROP_POPUP

/*
 * Implementation notes.
 *
 * There are 2 basic typs of text properties - text highlighting and virtual
 * text; both stored in a textprop_T structure.
 *
 * Text highlighting properties (conceptually) have a start and end column
 * ("tp_col" and "tp_len") within a line of buffer text.
 *
 * Virtual text properties have associated text strings that are displayed
 * relative to the "lnum". Virtual text properties can be considered to have 2
 * sub-types - inline and floating. Inline virtual text appears before a given
 * column ("tp_col") and floating text appears above, after or below the line
 * ("tp_col" is set to MAXCOL for sorting purposes only). The length of the
 * virtual text, including the trailing NUL is stored in "tp_len".
 *
 * Properties are stored with the text of the line they belong to in memlines.
 * The content of a memline looks like::
 *
 *     [line text] [0] [pc] [properties...] [virtual text...]
 *
 * Where:
 *     [0]	 is the terminating NUL character of the line's text.
 *     [pc]	 is a 16-bit counter for the number of properties.
 *
 * Lines without properties have no [pc], [properties...] or [virtual text...]
 * parts. The [pc] is never zero. The virtual text part can, of course, be zero
 * length. Each string in the [virtual text...] part is NUL terminated.
 * Virtual text properties store an offset to the text part, relative to where the
 * [pc] is stored.
 *
 * A memline is typically unpacked into an unpacked_memline_T structure in
 * order to process properties. A set of 'um_...()' functions are provided to
 * work with this structure.
 *
 * An unpacked_memline_T may be LOADED or DETACHED. A LOADED unpacked_memline_T
 * looks like::
 *
 *                [line text] [0] [pc] [properties...] [virtual text...]
 *		  ^				       ^       ^
 *     text-------'				       |       |
 *     ...					       |       |
 *     props[0].tp_text -> NULL			       |       |
 *     props[1].tp_text -------------------------------'       |
 *     ...						       |
 *     props[4].tp_text ---------------------------------------'
 *
 * Each virtual text propery has a pointer into the memline, as does the "text"
 * member.
 *
 * For a DETACHED unpacked_memline_T, all pointers into the memline are
 * replaced with allocated strings.
 *
 * In the event of any error, an unpacked_memline_T becomes CLOSED and the
 * following are all true.::
 *
 *     buf == NULL
 *     text == NULL
 *     props == NULL
 *     lnum == 0
 *     prop_count == 0
 *
 * Use (umemline.buf == NULL) to test for the CLOSED state.
 *
 * The main (public) um_...() functions are:
 *
 * - unpacked_memline_T um_open(*buf);
 * - unpacked_memline_T um_open_at(*buf, lnum, extra_props);
 * - unpacked_memline_T um_open_at_no_props(*buf, lnum, prop_count);
 * - unpacked_memline_T um_open_at_detached(*buf, lnum, extra_props);
 * - bool um_goto_line(*umemline, lnum, extra_props);
 * - void um_close(*umemline);
 * - void um_abort(*umemline);
 * - void um_delete_prop(*umemline, index);
 * - void um_reverse_props(*umemline);
 * - void um_sort_props(*umemline);
 * - void um_free_detached_props_vtext(*props, count);
 * - bool um_set_text(*umemline, *text);
 * - textprop_T *um_extract_props(*umemline);
 * - char_u *um_pack(*umemline, *packed_length);
 */
