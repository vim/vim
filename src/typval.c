/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * typval.c: functions that deal with a typval
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Allocate memory for a variable type-value, and make it empty (0 or NULL
 * value).
 */
    typval_T *
alloc_tv(void)
{
    return ALLOC_CLEAR_ONE(typval_T);
}

/*
 * Allocate memory for a variable type-value, and assign a string to it.
 * The string "s" must have been allocated, it is consumed.
 * Return NULL for out of memory, the variable otherwise.
 */
    typval_T *
alloc_string_tv(char_u *s)
{
    typval_T	*rettv;

    rettv = alloc_tv();
    if (rettv != NULL)
    {
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = s;
    }
    else
	vim_free(s);
    return rettv;
}

/*
 * Free the memory for a variable type-value.
 */
    void
free_tv(typval_T *varp)
{
    if (varp != NULL)
    {
	switch (varp->v_type)
	{
	    case VAR_FUNC:
		func_unref(varp->vval.v_string);
		// FALLTHROUGH
	    case VAR_STRING:
		vim_free(varp->vval.v_string);
		break;
	    case VAR_PARTIAL:
		partial_unref(varp->vval.v_partial);
		break;
	    case VAR_BLOB:
		blob_unref(varp->vval.v_blob);
		break;
	    case VAR_LIST:
		list_unref(varp->vval.v_list);
		break;
	    case VAR_DICT:
		dict_unref(varp->vval.v_dict);
		break;
	    case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
		job_unref(varp->vval.v_job);
		break;
#endif
	    case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
		channel_unref(varp->vval.v_channel);
		break;
#endif
	    case VAR_NUMBER:
	    case VAR_FLOAT:
	    case VAR_ANY:
	    case VAR_UNKNOWN:
	    case VAR_VOID:
	    case VAR_BOOL:
	    case VAR_SPECIAL:
		break;
	}
	vim_free(varp);
    }
}

/*
 * Free the memory for a variable value and set the value to NULL or 0.
 */
    void
clear_tv(typval_T *varp)
{
    if (varp != NULL)
    {
	switch (varp->v_type)
	{
	    case VAR_FUNC:
		func_unref(varp->vval.v_string);
		// FALLTHROUGH
	    case VAR_STRING:
		VIM_CLEAR(varp->vval.v_string);
		break;
	    case VAR_PARTIAL:
		partial_unref(varp->vval.v_partial);
		varp->vval.v_partial = NULL;
		break;
	    case VAR_BLOB:
		blob_unref(varp->vval.v_blob);
		varp->vval.v_blob = NULL;
		break;
	    case VAR_LIST:
		list_unref(varp->vval.v_list);
		varp->vval.v_list = NULL;
		break;
	    case VAR_DICT:
		dict_unref(varp->vval.v_dict);
		varp->vval.v_dict = NULL;
		break;
	    case VAR_NUMBER:
	    case VAR_BOOL:
	    case VAR_SPECIAL:
		varp->vval.v_number = 0;
		break;
	    case VAR_FLOAT:
#ifdef FEAT_FLOAT
		varp->vval.v_float = 0.0;
		break;
#endif
	    case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
		job_unref(varp->vval.v_job);
		varp->vval.v_job = NULL;
#endif
		break;
	    case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
		channel_unref(varp->vval.v_channel);
		varp->vval.v_channel = NULL;
#endif
	    case VAR_UNKNOWN:
	    case VAR_ANY:
	    case VAR_VOID:
		break;
	}
	varp->v_lock = 0;
    }
}

/*
 * Set the value of a variable to NULL without freeing items.
 */
    void
init_tv(typval_T *varp)
{
    if (varp != NULL)
	CLEAR_POINTER(varp);
}

    static varnumber_T
tv_get_bool_or_number_chk(typval_T *varp, int *denote, int want_bool)
{
    varnumber_T	n = 0L;

    switch (varp->v_type)
    {
	case VAR_NUMBER:
	    if (in_vim9script() && want_bool && varp->vval.v_number != 0
						   && varp->vval.v_number != 1)
	    {
		semsg(_(e_using_number_as_bool_nr), varp->vval.v_number);
		break;
	    }
	    return varp->vval.v_number;
	case VAR_FLOAT:
#ifdef FEAT_FLOAT
	    emsg(_("E805: Using a Float as a Number"));
	    break;
#endif
	case VAR_FUNC:
	case VAR_PARTIAL:
	    emsg(_("E703: Using a Funcref as a Number"));
	    break;
	case VAR_STRING:
	    if (in_vim9script())
	    {
		emsg_using_string_as(varp, !want_bool);
		break;
	    }
	    if (varp->vval.v_string != NULL)
		vim_str2nr(varp->vval.v_string, NULL, NULL,
					    STR2NR_ALL, &n, NULL, 0, FALSE);
	    return n;
	case VAR_LIST:
	    emsg(_("E745: Using a List as a Number"));
	    break;
	case VAR_DICT:
	    emsg(_("E728: Using a Dictionary as a Number"));
	    break;
	case VAR_BOOL:
	case VAR_SPECIAL:
	    if (!want_bool && in_vim9script())
	    {
		if (varp->v_type == VAR_BOOL)
		    emsg(_(e_using_bool_as_number));
		else
		    emsg(_("E611: Using a Special as a Number"));
		break;
	    }
	    return varp->vval.v_number == VVAL_TRUE ? 1 : 0;
	case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
	    emsg(_("E910: Using a Job as a Number"));
	    break;
#endif
	case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    emsg(_("E913: Using a Channel as a Number"));
	    break;
#endif
	case VAR_BLOB:
	    emsg(_("E974: Using a Blob as a Number"));
	    break;
	case VAR_UNKNOWN:
	case VAR_ANY:
	case VAR_VOID:
	    internal_error_no_abort("tv_get_number(UNKNOWN)");
	    break;
    }
    if (denote == NULL)		// useful for values that must be unsigned
	n = -1;
    else
	*denote = TRUE;
    return n;
}

/*
 * Get the number value of a variable.
 * If it is a String variable, uses vim_str2nr().
 * For incompatible types, return 0.
 * tv_get_number_chk() is similar to tv_get_number(), but informs the
 * caller of incompatible types: it sets *denote to TRUE if "denote"
 * is not NULL or returns -1 otherwise.
 */
    varnumber_T
tv_get_number(typval_T *varp)
{
    int		error = FALSE;

    return tv_get_number_chk(varp, &error);	// return 0L on error
}

    varnumber_T
tv_get_number_chk(typval_T *varp, int *denote)
{
    return tv_get_bool_or_number_chk(varp, denote, FALSE);
}

/*
 * Get the boolean value of "varp".  This is like tv_get_number_chk(),
 * but in Vim9 script accepts Number (0 and 1) and Bool/Special.
 */
    varnumber_T
tv_get_bool(typval_T *varp)
{
    return tv_get_bool_or_number_chk(varp, NULL, TRUE);
}

/*
 * Get the boolean value of "varp".  This is like tv_get_number_chk(),
 * but in Vim9 script accepts Number and Bool.
 */
    varnumber_T
tv_get_bool_chk(typval_T *varp, int *denote)
{
    return tv_get_bool_or_number_chk(varp, denote, TRUE);
}

#ifdef FEAT_FLOAT
    float_T
tv_get_float(typval_T *varp)
{
    switch (varp->v_type)
    {
	case VAR_NUMBER:
	    return (float_T)(varp->vval.v_number);
	case VAR_FLOAT:
	    return varp->vval.v_float;
	case VAR_FUNC:
	case VAR_PARTIAL:
	    emsg(_("E891: Using a Funcref as a Float"));
	    break;
	case VAR_STRING:
	    emsg(_("E892: Using a String as a Float"));
	    break;
	case VAR_LIST:
	    emsg(_("E893: Using a List as a Float"));
	    break;
	case VAR_DICT:
	    emsg(_("E894: Using a Dictionary as a Float"));
	    break;
	case VAR_BOOL:
	    emsg(_("E362: Using a boolean value as a Float"));
	    break;
	case VAR_SPECIAL:
	    emsg(_("E907: Using a special value as a Float"));
	    break;
	case VAR_JOB:
# ifdef FEAT_JOB_CHANNEL
	    emsg(_("E911: Using a Job as a Float"));
	    break;
# endif
	case VAR_CHANNEL:
# ifdef FEAT_JOB_CHANNEL
	    emsg(_("E914: Using a Channel as a Float"));
	    break;
# endif
	case VAR_BLOB:
	    emsg(_("E975: Using a Blob as a Float"));
	    break;
	case VAR_UNKNOWN:
	case VAR_ANY:
	case VAR_VOID:
	    internal_error_no_abort("tv_get_float(UNKNOWN)");
	    break;
    }
    return 0;
}
#endif

/*
 * Get the string value of a variable.
 * If it is a Number variable, the number is converted into a string.
 * tv_get_string() uses a single, static buffer.  YOU CAN ONLY USE IT ONCE!
 * tv_get_string_buf() uses a given buffer.
 * If the String variable has never been set, return an empty string.
 * Never returns NULL;
 * tv_get_string_chk() and tv_get_string_buf_chk() are similar, but return
 * NULL on error.
 */
    char_u *
tv_get_string(typval_T *varp)
{
    static char_u   mybuf[NUMBUFLEN];

    return tv_get_string_buf(varp, mybuf);
}

    char_u *
tv_get_string_buf(typval_T *varp, char_u *buf)
{
    char_u	*res =  tv_get_string_buf_chk(varp, buf);

    return res != NULL ? res : (char_u *)"";
}

/*
 * Careful: This uses a single, static buffer.  YOU CAN ONLY USE IT ONCE!
 */
    char_u *
tv_get_string_chk(typval_T *varp)
{
    static char_u   mybuf[NUMBUFLEN];

    return tv_get_string_buf_chk(varp, mybuf);
}

    char_u *
tv_get_string_buf_chk(typval_T *varp, char_u *buf)
{
    switch (varp->v_type)
    {
	case VAR_NUMBER:
	    vim_snprintf((char *)buf, NUMBUFLEN, "%lld",
					    (varnumber_T)varp->vval.v_number);
	    return buf;
	case VAR_FUNC:
	case VAR_PARTIAL:
	    emsg(_("E729: using Funcref as a String"));
	    break;
	case VAR_LIST:
	    emsg(_("E730: using List as a String"));
	    break;
	case VAR_DICT:
	    emsg(_("E731: using Dictionary as a String"));
	    break;
	case VAR_FLOAT:
#ifdef FEAT_FLOAT
	    emsg(_(e_float_as_string));
	    break;
#endif
	case VAR_STRING:
	    if (varp->vval.v_string != NULL)
		return varp->vval.v_string;
	    return (char_u *)"";
	case VAR_BOOL:
	case VAR_SPECIAL:
	    STRCPY(buf, get_var_special_name(varp->vval.v_number));
	    return buf;
        case VAR_BLOB:
	    emsg(_("E976: using Blob as a String"));
	    break;
	case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
	    {
		job_T *job = varp->vval.v_job;
		char  *status;

		if (job == NULL)
		    return (char_u *)"no process";
		status = job->jv_status == JOB_FAILED ? "fail"
				: job->jv_status >= JOB_ENDED ? "dead"
				: "run";
# ifdef UNIX
		vim_snprintf((char *)buf, NUMBUFLEN,
			    "process %ld %s", (long)job->jv_pid, status);
# elif defined(MSWIN)
		vim_snprintf((char *)buf, NUMBUFLEN,
			    "process %ld %s",
			    (long)job->jv_proc_info.dwProcessId,
			    status);
# else
		// fall-back
		vim_snprintf((char *)buf, NUMBUFLEN, "process ? %s", status);
# endif
		return buf;
	    }
#endif
	    break;
	case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    {
		channel_T *channel = varp->vval.v_channel;
		char      *status = channel_status(channel, -1);

		if (channel == NULL)
		    vim_snprintf((char *)buf, NUMBUFLEN, "channel %s", status);
		else
		    vim_snprintf((char *)buf, NUMBUFLEN,
				     "channel %d %s", channel->ch_id, status);
		return buf;
	    }
#endif
	    break;
	case VAR_UNKNOWN:
	case VAR_ANY:
	case VAR_VOID:
	    emsg(_(e_inval_string));
	    break;
    }
    return NULL;
}

/*
 * Turn a typeval into a string.  Similar to tv_get_string_buf() but uses
 * string() on Dict, List, etc.
 */
    char_u *
tv_stringify(typval_T *varp, char_u *buf)
{
    if (varp->v_type == VAR_LIST
	    || varp->v_type == VAR_DICT
	    || varp->v_type == VAR_BLOB
	    || varp->v_type == VAR_FUNC
	    || varp->v_type == VAR_PARTIAL
	    || varp->v_type == VAR_FLOAT)
    {
	typval_T tmp;

	f_string(varp, &tmp);
	tv_get_string_buf(&tmp, buf);
	clear_tv(varp);
	*varp = tmp;
	return tmp.vval.v_string;
    }
    return tv_get_string_buf(varp, buf);
}

/*
 * Return TRUE if typeval "tv" and its value are set to be locked (immutable).
 * Also give an error message, using "name" or _("name") when use_gettext is
 * TRUE.
 */
    int
tv_check_lock(typval_T *tv, char_u *name, int use_gettext)
{
    int	lock = 0;

    switch (tv->v_type)
    {
	case VAR_BLOB:
	    if (tv->vval.v_blob != NULL)
		lock = tv->vval.v_blob->bv_lock;
	    break;
	case VAR_LIST:
	    if (tv->vval.v_list != NULL)
		lock = tv->vval.v_list->lv_lock;
	    break;
	case VAR_DICT:
	    if (tv->vval.v_dict != NULL)
		lock = tv->vval.v_dict->dv_lock;
	    break;
	default:
	    break;
    }
    return value_check_lock(tv->v_lock, name, use_gettext)
		   || (lock != 0 && value_check_lock(lock, name, use_gettext));
}

/*
 * Copy the values from typval_T "from" to typval_T "to".
 * When needed allocates string or increases reference count.
 * Does not make a copy of a list, blob or dict but copies the reference!
 * It is OK for "from" and "to" to point to the same item.  This is used to
 * make a copy later.
 */
    void
copy_tv(typval_T *from, typval_T *to)
{
    to->v_type = from->v_type;
    to->v_lock = 0;
    switch (from->v_type)
    {
	case VAR_NUMBER:
	case VAR_BOOL:
	case VAR_SPECIAL:
	    to->vval.v_number = from->vval.v_number;
	    break;
	case VAR_FLOAT:
#ifdef FEAT_FLOAT
	    to->vval.v_float = from->vval.v_float;
	    break;
#endif
	case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
	    to->vval.v_job = from->vval.v_job;
	    if (to->vval.v_job != NULL)
		++to->vval.v_job->jv_refcount;
	    break;
#endif
	case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    to->vval.v_channel = from->vval.v_channel;
	    if (to->vval.v_channel != NULL)
		++to->vval.v_channel->ch_refcount;
	    break;
#endif
	case VAR_STRING:
	case VAR_FUNC:
	    if (from->vval.v_string == NULL)
		to->vval.v_string = NULL;
	    else
	    {
		to->vval.v_string = vim_strsave(from->vval.v_string);
		if (from->v_type == VAR_FUNC)
		    func_ref(to->vval.v_string);
	    }
	    break;
	case VAR_PARTIAL:
	    if (from->vval.v_partial == NULL)
		to->vval.v_partial = NULL;
	    else
	    {
		to->vval.v_partial = from->vval.v_partial;
		++to->vval.v_partial->pt_refcount;
	    }
	    break;
	case VAR_BLOB:
	    if (from->vval.v_blob == NULL)
		to->vval.v_blob = NULL;
	    else
	    {
		to->vval.v_blob = from->vval.v_blob;
		++to->vval.v_blob->bv_refcount;
	    }
	    break;
	case VAR_LIST:
	    if (from->vval.v_list == NULL)
		to->vval.v_list = NULL;
	    else
	    {
		to->vval.v_list = from->vval.v_list;
		++to->vval.v_list->lv_refcount;
	    }
	    break;
	case VAR_DICT:
	    if (from->vval.v_dict == NULL)
		to->vval.v_dict = NULL;
	    else
	    {
		to->vval.v_dict = from->vval.v_dict;
		++to->vval.v_dict->dv_refcount;
	    }
	    break;
	case VAR_UNKNOWN:
	case VAR_ANY:
	case VAR_VOID:
	    internal_error_no_abort("copy_tv(UNKNOWN)");
	    break;
    }
}

/*
 * Compare "typ1" and "typ2".  Put the result in "typ1".
 */
    int
typval_compare(
    typval_T	*typ1,   // first operand
    typval_T	*typ2,   // second operand
    exptype_T	type,    // operator
    int		ic)      // ignore case
{
    int		i;
    varnumber_T	n1, n2;
    char_u	*s1, *s2;
    char_u	buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    int		type_is = type == EXPR_IS || type == EXPR_ISNOT;

    if (type_is && typ1->v_type != typ2->v_type)
    {
	// For "is" a different type always means FALSE, for "notis"
	// it means TRUE.
	n1 = (type == EXPR_ISNOT);
    }
    else if (typ1->v_type == VAR_BLOB || typ2->v_type == VAR_BLOB)
    {
	if (type_is)
	{
	    n1 = (typ1->v_type == typ2->v_type
			    && typ1->vval.v_blob == typ2->vval.v_blob);
	    if (type == EXPR_ISNOT)
		n1 = !n1;
	}
	else if (typ1->v_type != typ2->v_type
		|| (type != EXPR_EQUAL && type != EXPR_NEQUAL))
	{
	    if (typ1->v_type != typ2->v_type)
		emsg(_("E977: Can only compare Blob with Blob"));
	    else
		emsg(_(e_invalblob));
	    clear_tv(typ1);
	    return FAIL;
	}
	else
	{
	    // Compare two Blobs for being equal or unequal.
	    n1 = blob_equal(typ1->vval.v_blob, typ2->vval.v_blob);
	    if (type == EXPR_NEQUAL)
		n1 = !n1;
	}
    }
    else if (typ1->v_type == VAR_LIST || typ2->v_type == VAR_LIST)
    {
	if (type_is)
	{
	    n1 = (typ1->v_type == typ2->v_type
			    && typ1->vval.v_list == typ2->vval.v_list);
	    if (type == EXPR_ISNOT)
		n1 = !n1;
	}
	else if (typ1->v_type != typ2->v_type
		|| (type != EXPR_EQUAL && type != EXPR_NEQUAL))
	{
	    if (typ1->v_type != typ2->v_type)
		emsg(_("E691: Can only compare List with List"));
	    else
		emsg(_("E692: Invalid operation for List"));
	    clear_tv(typ1);
	    return FAIL;
	}
	else
	{
	    // Compare two Lists for being equal or unequal.
	    n1 = list_equal(typ1->vval.v_list, typ2->vval.v_list,
							    ic, FALSE);
	    if (type == EXPR_NEQUAL)
		n1 = !n1;
	}
    }

    else if (typ1->v_type == VAR_DICT || typ2->v_type == VAR_DICT)
    {
	if (type_is)
	{
	    n1 = (typ1->v_type == typ2->v_type
			    && typ1->vval.v_dict == typ2->vval.v_dict);
	    if (type == EXPR_ISNOT)
		n1 = !n1;
	}
	else if (typ1->v_type != typ2->v_type
		|| (type != EXPR_EQUAL && type != EXPR_NEQUAL))
	{
	    if (typ1->v_type != typ2->v_type)
		emsg(_("E735: Can only compare Dictionary with Dictionary"));
	    else
		emsg(_("E736: Invalid operation for Dictionary"));
	    clear_tv(typ1);
	    return FAIL;
	}
	else
	{
	    // Compare two Dictionaries for being equal or unequal.
	    n1 = dict_equal(typ1->vval.v_dict, typ2->vval.v_dict,
							    ic, FALSE);
	    if (type == EXPR_NEQUAL)
		n1 = !n1;
	}
    }

    else if (typ1->v_type == VAR_FUNC || typ2->v_type == VAR_FUNC
	|| typ1->v_type == VAR_PARTIAL || typ2->v_type == VAR_PARTIAL)
    {
	if (type != EXPR_EQUAL && type != EXPR_NEQUAL
		&& type != EXPR_IS && type != EXPR_ISNOT)
	{
	    emsg(_("E694: Invalid operation for Funcrefs"));
	    clear_tv(typ1);
	    return FAIL;
	}
	if ((typ1->v_type == VAR_PARTIAL
					&& typ1->vval.v_partial == NULL)
		|| (typ2->v_type == VAR_PARTIAL
					&& typ2->vval.v_partial == NULL))
	    // When both partials are NULL, then they are equal.
	    // Otherwise they are not equal.
	    n1 = (typ1->vval.v_partial == typ2->vval.v_partial);
	else if (type_is)
	{
	    if (typ1->v_type == VAR_FUNC && typ2->v_type == VAR_FUNC)
		// strings are considered the same if their value is
		// the same
		n1 = tv_equal(typ1, typ2, ic, FALSE);
	    else if (typ1->v_type == VAR_PARTIAL
					&& typ2->v_type == VAR_PARTIAL)
		n1 = (typ1->vval.v_partial == typ2->vval.v_partial);
	    else
		n1 = FALSE;
	}
	else
	    n1 = tv_equal(typ1, typ2, ic, FALSE);
	if (type == EXPR_NEQUAL || type == EXPR_ISNOT)
	    n1 = !n1;
    }

#ifdef FEAT_FLOAT
    // If one of the two variables is a float, compare as a float.
    // When using "=~" or "!~", always compare as string.
    else if ((typ1->v_type == VAR_FLOAT || typ2->v_type == VAR_FLOAT)
	    && type != EXPR_MATCH && type != EXPR_NOMATCH)
    {
	float_T f1, f2;

	f1 = tv_get_float(typ1);
	f2 = tv_get_float(typ2);
	n1 = FALSE;
	switch (type)
	{
	    case EXPR_IS:
	    case EXPR_EQUAL:    n1 = (f1 == f2); break;
	    case EXPR_ISNOT:
	    case EXPR_NEQUAL:   n1 = (f1 != f2); break;
	    case EXPR_GREATER:  n1 = (f1 > f2); break;
	    case EXPR_GEQUAL:   n1 = (f1 >= f2); break;
	    case EXPR_SMALLER:  n1 = (f1 < f2); break;
	    case EXPR_SEQUAL:   n1 = (f1 <= f2); break;
	    case EXPR_UNKNOWN:
	    case EXPR_MATCH:
	    default:  break;  // avoid gcc warning
	}
    }
#endif

    // If one of the two variables is a number, compare as a number.
    // When using "=~" or "!~", always compare as string.
    else if ((typ1->v_type == VAR_NUMBER || typ2->v_type == VAR_NUMBER)
	    && type != EXPR_MATCH && type != EXPR_NOMATCH)
    {
	n1 = tv_get_number(typ1);
	n2 = tv_get_number(typ2);
	switch (type)
	{
	    case EXPR_IS:
	    case EXPR_EQUAL:    n1 = (n1 == n2); break;
	    case EXPR_ISNOT:
	    case EXPR_NEQUAL:   n1 = (n1 != n2); break;
	    case EXPR_GREATER:  n1 = (n1 > n2); break;
	    case EXPR_GEQUAL:   n1 = (n1 >= n2); break;
	    case EXPR_SMALLER:  n1 = (n1 < n2); break;
	    case EXPR_SEQUAL:   n1 = (n1 <= n2); break;
	    case EXPR_UNKNOWN:
	    case EXPR_MATCH:
	    default:  break;  // avoid gcc warning
	}
    }
    else
    {
	s1 = tv_get_string_buf(typ1, buf1);
	s2 = tv_get_string_buf(typ2, buf2);
	if (type != EXPR_MATCH && type != EXPR_NOMATCH)
	    i = ic ? MB_STRICMP(s1, s2) : STRCMP(s1, s2);
	else
	    i = 0;
	n1 = FALSE;
	switch (type)
	{
	    case EXPR_IS:
	    case EXPR_EQUAL:    n1 = (i == 0); break;
	    case EXPR_ISNOT:
	    case EXPR_NEQUAL:   n1 = (i != 0); break;
	    case EXPR_GREATER:  n1 = (i > 0); break;
	    case EXPR_GEQUAL:   n1 = (i >= 0); break;
	    case EXPR_SMALLER:  n1 = (i < 0); break;
	    case EXPR_SEQUAL:   n1 = (i <= 0); break;

	    case EXPR_MATCH:
	    case EXPR_NOMATCH:
		    n1 = pattern_match(s2, s1, ic);
		    if (type == EXPR_NOMATCH)
			n1 = !n1;
		    break;

	    default:  break;  // avoid gcc warning
	}
    }
    clear_tv(typ1);
    if (in_vim9script())
    {
	typ1->v_type = VAR_BOOL;
	typ1->vval.v_number = n1 ? VVAL_TRUE : VVAL_FALSE;
    }
    else
    {
	typ1->v_type = VAR_NUMBER;
	typ1->vval.v_number = n1;
    }

    return OK;
}

    char_u *
typval_tostring(typval_T *arg)
{
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];
    char_u	*ret = NULL;

    if (arg == NULL)
	return vim_strsave((char_u *)"(does not exist)");
    ret = tv2string(arg, &tofree, numbuf, 0);
    // Make a copy if we have a value but it's not in allocated memory.
    if (ret != NULL && tofree == NULL)
	ret = vim_strsave(ret);
    return ret;
}

/*
 * Return TRUE if typeval "tv" is locked: Either that value is locked itself
 * or it refers to a List or Dictionary that is locked.
 */
    int
tv_islocked(typval_T *tv)
{
    return (tv->v_lock & VAR_LOCKED)
	|| (tv->v_type == VAR_LIST
		&& tv->vval.v_list != NULL
		&& (tv->vval.v_list->lv_lock & VAR_LOCKED))
	|| (tv->v_type == VAR_DICT
		&& tv->vval.v_dict != NULL
		&& (tv->vval.v_dict->dv_lock & VAR_LOCKED));
}

    static int
func_equal(
    typval_T *tv1,
    typval_T *tv2,
    int	     ic)	    // ignore case
{
    char_u	*s1, *s2;
    dict_T	*d1, *d2;
    int		a1, a2;
    int		i;

    // empty and NULL function name considered the same
    s1 = tv1->v_type == VAR_FUNC ? tv1->vval.v_string
					   : partial_name(tv1->vval.v_partial);
    if (s1 != NULL && *s1 == NUL)
	s1 = NULL;
    s2 = tv2->v_type == VAR_FUNC ? tv2->vval.v_string
					   : partial_name(tv2->vval.v_partial);
    if (s2 != NULL && *s2 == NUL)
	s2 = NULL;
    if (s1 == NULL || s2 == NULL)
    {
	if (s1 != s2)
	    return FALSE;
    }
    else if (STRCMP(s1, s2) != 0)
	return FALSE;

    // empty dict and NULL dict is different
    d1 = tv1->v_type == VAR_FUNC ? NULL : tv1->vval.v_partial->pt_dict;
    d2 = tv2->v_type == VAR_FUNC ? NULL : tv2->vval.v_partial->pt_dict;
    if (d1 == NULL || d2 == NULL)
    {
	if (d1 != d2)
	    return FALSE;
    }
    else if (!dict_equal(d1, d2, ic, TRUE))
	return FALSE;

    // empty list and no list considered the same
    a1 = tv1->v_type == VAR_FUNC ? 0 : tv1->vval.v_partial->pt_argc;
    a2 = tv2->v_type == VAR_FUNC ? 0 : tv2->vval.v_partial->pt_argc;
    if (a1 != a2)
	return FALSE;
    for (i = 0; i < a1; ++i)
	if (!tv_equal(tv1->vval.v_partial->pt_argv + i,
		      tv2->vval.v_partial->pt_argv + i, ic, TRUE))
	    return FALSE;

    return TRUE;
}

/*
 * Return TRUE if "tv1" and "tv2" have the same value.
 * Compares the items just like "==" would compare them, but strings and
 * numbers are different.  Floats and numbers are also different.
 */
    int
tv_equal(
    typval_T *tv1,
    typval_T *tv2,
    int	     ic,	    // ignore case
    int	     recursive)	    // TRUE when used recursively
{
    char_u	buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    char_u	*s1, *s2;
    static int  recursive_cnt = 0;	    // catch recursive loops
    int		r;
    static int	tv_equal_recurse_limit;

    // Catch lists and dicts that have an endless loop by limiting
    // recursiveness to a limit.  We guess they are equal then.
    // A fixed limit has the problem of still taking an awful long time.
    // Reduce the limit every time running into it. That should work fine for
    // deeply linked structures that are not recursively linked and catch
    // recursiveness quickly.
    if (!recursive)
	tv_equal_recurse_limit = 1000;
    if (recursive_cnt >= tv_equal_recurse_limit)
    {
	--tv_equal_recurse_limit;
	return TRUE;
    }

    // For VAR_FUNC and VAR_PARTIAL compare the function name, bound dict and
    // arguments.
    if ((tv1->v_type == VAR_FUNC
		|| (tv1->v_type == VAR_PARTIAL && tv1->vval.v_partial != NULL))
	    && (tv2->v_type == VAR_FUNC
		|| (tv2->v_type == VAR_PARTIAL && tv2->vval.v_partial != NULL)))
    {
	++recursive_cnt;
	r = func_equal(tv1, tv2, ic);
	--recursive_cnt;
	return r;
    }

    if (tv1->v_type != tv2->v_type)
	return FALSE;

    switch (tv1->v_type)
    {
	case VAR_LIST:
	    ++recursive_cnt;
	    r = list_equal(tv1->vval.v_list, tv2->vval.v_list, ic, TRUE);
	    --recursive_cnt;
	    return r;

	case VAR_DICT:
	    ++recursive_cnt;
	    r = dict_equal(tv1->vval.v_dict, tv2->vval.v_dict, ic, TRUE);
	    --recursive_cnt;
	    return r;

	case VAR_BLOB:
	    return blob_equal(tv1->vval.v_blob, tv2->vval.v_blob);

	case VAR_NUMBER:
	case VAR_BOOL:
	case VAR_SPECIAL:
	    return tv1->vval.v_number == tv2->vval.v_number;

	case VAR_STRING:
	    s1 = tv_get_string_buf(tv1, buf1);
	    s2 = tv_get_string_buf(tv2, buf2);
	    return ((ic ? MB_STRICMP(s1, s2) : STRCMP(s1, s2)) == 0);

	case VAR_FLOAT:
#ifdef FEAT_FLOAT
	    return tv1->vval.v_float == tv2->vval.v_float;
#endif
	case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
	    return tv1->vval.v_job == tv2->vval.v_job;
#endif
	case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    return tv1->vval.v_channel == tv2->vval.v_channel;
#endif

	case VAR_PARTIAL:
	    return tv1->vval.v_partial == tv2->vval.v_partial;

	case VAR_FUNC:
	    return tv1->vval.v_string == tv2->vval.v_string;

	case VAR_UNKNOWN:
	case VAR_ANY:
	case VAR_VOID:
	    break;
    }

    // VAR_UNKNOWN can be the result of a invalid expression, let's say it
    // does not equal anything, not even itself.
    return FALSE;
}

/*
 * Get an option value.
 * "arg" points to the '&' or '+' before the option name.
 * "arg" is advanced to character after the option name.
 * Return OK or FAIL.
 */
    int
eval_option(
    char_u	**arg,
    typval_T	*rettv,	// when NULL, only check if option exists
    int		evaluate)
{
    char_u	*option_end;
    long	numval;
    char_u	*stringval;
    int		opt_type;
    int		c;
    int		working = (**arg == '+');    // has("+option")
    int		ret = OK;
    int		opt_flags;

    // Isolate the option name and find its value.
    option_end = find_option_end(arg, &opt_flags);
    if (option_end == NULL)
    {
	if (rettv != NULL)
	    semsg(_("E112: Option name missing: %s"), *arg);
	return FAIL;
    }

    if (!evaluate)
    {
	*arg = option_end;
	return OK;
    }

    c = *option_end;
    *option_end = NUL;
    opt_type = get_option_value(*arg, &numval,
			       rettv == NULL ? NULL : &stringval, opt_flags);

    if (opt_type == -3)			// invalid name
    {
	if (rettv != NULL)
	    semsg(_(e_unknown_option), *arg);
	ret = FAIL;
    }
    else if (rettv != NULL)
    {
	if (opt_type == -2)		// hidden string option
	{
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = NULL;
	}
	else if (opt_type == -1)	// hidden number option
	{
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = 0;
	}
	else if (opt_type == 1)		// number option
	{
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = numval;
	}
	else				// string option
	{
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = stringval;
	}
    }
    else if (working && (opt_type == -2 || opt_type == -1))
	ret = FAIL;

    *option_end = c;		    // put back for error messages
    *arg = option_end;

    return ret;
}

/*
 * Allocate a variable for a number constant.  Also deals with "0z" for blob.
 * Return OK or FAIL.
 */
    int
eval_number(
	char_u	    **arg,
	typval_T    *rettv,
	int	    evaluate,
	int	    want_string UNUSED)
{
    int		len;
#ifdef FEAT_FLOAT
    char_u	*p;
    int		get_float = FALSE;

    // We accept a float when the format matches
    // "[0-9]\+\.[0-9]\+\([eE][+-]\?[0-9]\+\)\?".  This is very
    // strict to avoid backwards compatibility problems.
    // With script version 2 and later the leading digit can be
    // omitted.
    // Don't look for a float after the "." operator, so that
    // ":let vers = 1.2.3" doesn't fail.
    if (**arg == '.')
	p = *arg;
    else
	p = skipdigits(*arg + 1);
    if (!want_string && p[0] == '.' && vim_isdigit(p[1]))
    {
	get_float = TRUE;
	p = skipdigits(p + 2);
	if (*p == 'e' || *p == 'E')
	{
	    ++p;
	    if (*p == '-' || *p == '+')
		++p;
	    if (!vim_isdigit(*p))
		get_float = FALSE;
	    else
		p = skipdigits(p + 1);
	}
	if (ASCII_ISALPHA(*p) || *p == '.')
	    get_float = FALSE;
    }
    if (get_float)
    {
	float_T	f;

	*arg += string2float(*arg, &f);
	if (evaluate)
	{
	    rettv->v_type = VAR_FLOAT;
	    rettv->vval.v_float = f;
	}
    }
    else
#endif
    if (**arg == '0' && ((*arg)[1] == 'z' || (*arg)[1] == 'Z'))
    {
	char_u  *bp;
	blob_T  *blob = NULL;  // init for gcc

	// Blob constant: 0z0123456789abcdef
	if (evaluate)
	    blob = blob_alloc();
	for (bp = *arg + 2; vim_isxdigit(bp[0]); bp += 2)
	{
	    if (!vim_isxdigit(bp[1]))
	    {
		if (blob != NULL)
		{
		    emsg(_("E973: Blob literal should have an even number of hex characters"));
		    ga_clear(&blob->bv_ga);
		    VIM_CLEAR(blob);
		}
		return FAIL;
	    }
	    if (blob != NULL)
		ga_append(&blob->bv_ga,
			     (hex2nr(*bp) << 4) + hex2nr(*(bp+1)));
	    if (bp[2] == '.' && vim_isxdigit(bp[3]))
		++bp;
	}
	if (blob != NULL)
	    rettv_blob_set(rettv, blob);
	*arg = bp;
    }
    else
    {
	varnumber_T	n;

	// decimal, hex or octal number
	vim_str2nr(*arg, NULL, &len, current_sctx.sc_version >= 4
		      ? STR2NR_NO_OCT + STR2NR_QUOTE
		      : STR2NR_ALL, &n, NULL, 0, TRUE);
	if (len == 0)
	{
	    semsg(_(e_invexpr2), *arg);
	    return FAIL;
	}
	*arg += len;
	if (evaluate)
	{
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = n;
	}
    }
    return OK;
}

/*
 * Allocate a variable for a string constant.
 * Return OK or FAIL.
 */
    int
eval_string(char_u **arg, typval_T *rettv, int evaluate)
{
    char_u	*p;
    char_u	*end;
    int		extra = 0;
    int		len;

    // Find the end of the string, skipping backslashed characters.
    for (p = *arg + 1; *p != NUL && *p != '"'; MB_PTR_ADV(p))
    {
	if (*p == '\\' && p[1] != NUL)
	{
	    ++p;
	    // A "\<x>" form occupies at least 4 characters, and produces up
	    // to 21 characters (3 * 6 for the char and 3 for a modifier):
	    // reserve space for 18 extra.
	    // Each byte in the char could be encoded as K_SPECIAL K_EXTRA x.
	    if (*p == '<')
		extra += 18;
	}
    }

    if (*p != '"')
    {
	semsg(_("E114: Missing quote: %s"), *arg);
	return FAIL;
    }

    // If only parsing, set *arg and return here
    if (!evaluate)
    {
	*arg = p + 1;
	return OK;
    }

    // Copy the string into allocated memory, handling backslashed
    // characters.
    rettv->v_type = VAR_STRING;
    len = (int)(p - *arg + extra);
    rettv->vval.v_string = alloc(len);
    if (rettv->vval.v_string == NULL)
	return FAIL;
    end = rettv->vval.v_string;

    for (p = *arg + 1; *p != NUL && *p != '"'; )
    {
	if (*p == '\\')
	{
	    switch (*++p)
	    {
		case 'b': *end++ = BS; ++p; break;
		case 'e': *end++ = ESC; ++p; break;
		case 'f': *end++ = FF; ++p; break;
		case 'n': *end++ = NL; ++p; break;
		case 'r': *end++ = CAR; ++p; break;
		case 't': *end++ = TAB; ++p; break;

		case 'X': // hex: "\x1", "\x12"
		case 'x':
		case 'u': // Unicode: "\u0023"
		case 'U':
			  if (vim_isxdigit(p[1]))
			  {
			      int	n, nr;
			      int	c = toupper(*p);

			      if (c == 'X')
				  n = 2;
			      else if (*p == 'u')
				  n = 4;
			      else
				  n = 8;
			      nr = 0;
			      while (--n >= 0 && vim_isxdigit(p[1]))
			      {
				  ++p;
				  nr = (nr << 4) + hex2nr(*p);
			      }
			      ++p;
			      // For "\u" store the number according to
			      // 'encoding'.
			      if (c != 'X')
				  end += (*mb_char2bytes)(nr, end);
			      else
				  *end++ = nr;
			  }
			  break;

			  // octal: "\1", "\12", "\123"
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7': *end = *p++ - '0';
			  if (*p >= '0' && *p <= '7')
			  {
			      *end = (*end << 3) + *p++ - '0';
			      if (*p >= '0' && *p <= '7')
				  *end = (*end << 3) + *p++ - '0';
			  }
			  ++end;
			  break;

			  // Special key, e.g.: "\<C-W>"
		case '<':
			  {
			      int flags = FSK_KEYCODE | FSK_IN_STRING;

			      if (p[1] != '*')
				  flags |= FSK_SIMPLIFY;
			      extra = trans_special(&p, end, flags, NULL);
			      if (extra != 0)
			      {
				  end += extra;
				  if (end >= rettv->vval.v_string + len)
				      iemsg("eval_string() used more space than allocated");
				  break;
			      }
			  }
			  // FALLTHROUGH

		default:  MB_COPY_CHAR(p, end);
			  break;
	    }
	}
	else
	    MB_COPY_CHAR(p, end);
    }
    *end = NUL;
    if (*p != NUL) // just in case
	++p;
    *arg = p;

    return OK;
}

/*
 * Allocate a variable for a 'str''ing' constant.
 * Return OK or FAIL.
 */
    int
eval_lit_string(char_u **arg, typval_T *rettv, int evaluate)
{
    char_u	*p;
    char_u	*str;
    int		reduce = 0;

    // Find the end of the string, skipping ''.
    for (p = *arg + 1; *p != NUL; MB_PTR_ADV(p))
    {
	if (*p == '\'')
	{
	    if (p[1] != '\'')
		break;
	    ++reduce;
	    ++p;
	}
    }

    if (*p != '\'')
    {
	semsg(_("E115: Missing quote: %s"), *arg);
	return FAIL;
    }

    // If only parsing return after setting "*arg"
    if (!evaluate)
    {
	*arg = p + 1;
	return OK;
    }

    // Copy the string into allocated memory, handling '' to ' reduction.
    str = alloc((p - *arg) - reduce);
    if (str == NULL)
	return FAIL;
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = str;

    for (p = *arg + 1; *p != NUL; )
    {
	if (*p == '\'')
	{
	    if (p[1] != '\'')
		break;
	    ++p;
	}
	MB_COPY_CHAR(p, str);
    }
    *str = NUL;
    *arg = p + 1;

    return OK;
}

/*
 * Return a string with the string representation of a variable.
 * If the memory is allocated "tofree" is set to it, otherwise NULL.
 * "numbuf" is used for a number.
 * Puts quotes around strings, so that they can be parsed back by eval().
 * May return NULL.
 */
    char_u *
tv2string(
    typval_T	*tv,
    char_u	**tofree,
    char_u	*numbuf,
    int		copyID)
{
    return echo_string_core(tv, tofree, numbuf, copyID, FALSE, TRUE, FALSE);
}

/*
 * Get the value of an environment variable.
 * "arg" is pointing to the '$'.  It is advanced to after the name.
 * If the environment variable was not set, silently assume it is empty.
 * Return FAIL if the name is invalid.
 */
    int
eval_env_var(char_u **arg, typval_T *rettv, int evaluate)
{
    char_u	*string = NULL;
    int		len;
    int		cc;
    char_u	*name;
    int		mustfree = FALSE;

    ++*arg;
    name = *arg;
    len = get_env_len(arg);
    if (evaluate)
    {
	if (len == 0)
	    return FAIL; // invalid empty name

	cc = name[len];
	name[len] = NUL;
	// first try vim_getenv(), fast for normal environment vars
	string = vim_getenv(name, &mustfree);
	if (string != NULL && *string != NUL)
	{
	    if (!mustfree)
		string = vim_strsave(string);
	}
	else
	{
	    if (mustfree)
		vim_free(string);

	    // next try expanding things like $VIM and ${HOME}
	    string = expand_env_save(name - 1);
	    if (string != NULL && *string == '$')
		VIM_CLEAR(string);
	}
	name[len] = cc;

	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = string;
    }

    return OK;
}

/*
 * Get the lnum from the first argument.
 * Also accepts ".", "$", etc., but that only works for the current buffer.
 * Returns -1 on error.
 */
    linenr_T
tv_get_lnum(typval_T *argvars)
{
    linenr_T	lnum = 0;

    if (argvars[0].v_type != VAR_STRING || !in_vim9script())
	lnum = (linenr_T)tv_get_number_chk(&argvars[0], NULL);
    if (lnum == 0)  // no valid number, try using arg like line()
    {
	int	fnum;
	pos_T	*fp = var2fpos(&argvars[0], TRUE, &fnum);

	if (fp != NULL)
	    lnum = fp->lnum;
    }
    return lnum;
}

/*
 * Get the lnum from the first argument.
 * Also accepts "$", then "buf" is used.
 * Returns 0 on error.
 */
    linenr_T
tv_get_lnum_buf(typval_T *argvars, buf_T *buf)
{
    if (argvars[0].v_type == VAR_STRING
	    && argvars[0].vval.v_string != NULL
	    && argvars[0].vval.v_string[0] == '$'
	    && buf != NULL)
	return buf->b_ml.ml_line_count;
    return (linenr_T)tv_get_number_chk(&argvars[0], NULL);
}

/*
 * Get buffer by number or pattern.
 */
    buf_T *
tv_get_buf(typval_T *tv, int curtab_only)
{
    char_u	*name = tv->vval.v_string;
    buf_T	*buf;

    if (tv->v_type == VAR_NUMBER)
	return buflist_findnr((int)tv->vval.v_number);
    if (tv->v_type != VAR_STRING)
	return NULL;
    if (name == NULL || *name == NUL)
	return curbuf;
    if (name[0] == '$' && name[1] == NUL)
	return lastbuf;

    buf = buflist_find_by_name(name, curtab_only);

    // If not found, try expanding the name, like done for bufexists().
    if (buf == NULL)
	buf = find_buffer(tv);

    return buf;
}

/*
 * Like tv_get_buf() but give an error message is the type is wrong.
 */
    buf_T *
tv_get_buf_from_arg(typval_T *tv)
{
    buf_T *buf;

    ++emsg_off;
    buf = tv_get_buf(tv, FALSE);
    --emsg_off;
    if (buf == NULL
	    && tv->v_type != VAR_NUMBER
	    && tv->v_type != VAR_STRING)
	// issue errmsg for type error
	(void)tv_get_number(tv);
    return buf;
}

#endif // FEAT_EVAL
