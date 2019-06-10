/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Implementation of popup windows.  See ":help popup".
 */

#include "vim.h"

#ifdef FEAT_TEXT_PROP

typedef struct {
    char	*pp_name;
    poppos_T	pp_val;
} poppos_entry_T;

static poppos_entry_T poppos_entries[] = {
    {"botleft", POPPOS_BOTLEFT},
    {"topleft", POPPOS_TOPLEFT},
    {"botright", POPPOS_BOTRIGHT},
    {"topright", POPPOS_TOPRIGHT},
    {"center", POPPOS_CENTER}
};

/*
 * Get option value for "key", which is "line" or "col".
 * Handles "cursor+N" and "cursor-N".
 */
    static int
popup_options_one(dict_T *dict, char_u *key)
{
    dictitem_T	*di;
    char_u	*val;
    char_u	*s;
    char_u	*endp;
    int		n = 0;

    di = dict_find(dict, key, -1);
    if (di == NULL)
	return 0;

    val = tv_get_string(&di->di_tv);
    if (STRNCMP(val, "cursor", 6) != 0)
	return dict_get_number_check(dict, key);

    setcursor_mayforce(TRUE);
    s = val + 6;
    if (*s != NUL)
    {
	endp = s;
	if (*skipwhite(s) == '+' || *skipwhite(s) == '-')
	    n = strtol((char *)s, (char **)&endp, 10);
	if (endp != NULL && *skipwhite(endp) != NUL)
	{
	    semsg(_(e_invexpr2), val);
	    return 0;
	}
    }

    if (STRCMP(key, "line") == 0)
	n = screen_screenrow() + 1 + n;
    else // "col"
	n = screen_screencol() + 1 + n;

    if (n < 1)
	n = 1;
    return n;
}

    static void
get_pos_options(win_T *wp, dict_T *dict)
{
    char_u	*str;
    int		nr;

    nr = popup_options_one(dict, (char_u *)"line");
    if (nr > 0)
	wp->w_wantline = nr;
    nr = popup_options_one(dict, (char_u *)"col");
    if (nr > 0)
	wp->w_wantcol = nr;

    wp->w_popup_fixed = dict_get_number(dict, (char_u *)"fixed") != 0;

    str = dict_get_string(dict, (char_u *)"pos", FALSE);
    if (str != NULL)
    {
	for (nr = 0;
		nr < (int)(sizeof(poppos_entries) / sizeof(poppos_entry_T));
									  ++nr)
	    if (STRCMP(str, poppos_entries[nr].pp_name) == 0)
	    {
		wp->w_popup_pos = poppos_entries[nr].pp_val;
		nr = -1;
		break;
	    }
	if (nr != -1)
	    semsg(_(e_invarg2), str);
    }
}

    static void
get_padding_border(dict_T *dict, int *array, char *name, int max_val)
{
    dictitem_T	*di;

    vim_memset(array, 0, sizeof(int) * 4);
    di = dict_find(dict, (char_u *)name, -1);
    if (di != NULL)
    {
	if (di->di_tv.v_type != VAR_LIST)
	    emsg(_(e_listreq));
	else
	{
	    list_T	*list = di->di_tv.vval.v_list;
	    listitem_T	*li;
	    int		i;
	    int		nr;

	    for (i = 0; i < 4; ++i)
		array[i] = 1;
	    if (list != NULL)
		for (i = 0, li = list->lv_first; i < 4 && i < list->lv_len;
							 ++i, li = li->li_next)
		{
		    nr = (int)tv_get_number(&li->li_tv);
		    if (nr >= 0)
			array[i] = nr > max_val ? max_val : nr;
		}
	}
    }
}

/*
 * Used when popup options contain "moved": set default moved values.
 */
    static void
set_moved_values(win_T *wp)
{
    wp->w_popup_curwin = curwin;
    wp->w_popup_lnum = curwin->w_cursor.lnum;
    wp->w_popup_mincol = curwin->w_cursor.col;
    wp->w_popup_maxcol = curwin->w_cursor.col;
}

/*
 * Used when popup options contain "moved" with "word" or "WORD".
 */
    static void
set_moved_columns(win_T *wp, int flags)
{
    char_u	*ptr;
    int		len = find_ident_under_cursor(&ptr, flags | FIND_NOERROR);

    if (len > 0)
    {
	wp->w_popup_mincol = (int)(ptr - ml_get_curline());
	wp->w_popup_maxcol = wp->w_popup_mincol + len - 1;
    }
}

/*
 * Go through the options in "dict" and apply them to buffer "buf" displayed in
 * popup window "wp".
 */
    static void
apply_options(win_T *wp, buf_T *buf UNUSED, dict_T *dict)
{
    int		nr;
    char_u	*str;
    dictitem_T	*di;
    int		i;

    wp->w_minwidth = dict_get_number(dict, (char_u *)"minwidth");
    wp->w_minheight = dict_get_number(dict, (char_u *)"minheight");
    wp->w_maxwidth = dict_get_number(dict, (char_u *)"maxwidth");
    wp->w_maxheight = dict_get_number(dict, (char_u *)"maxheight");

    get_pos_options(wp, dict);

    wp->w_zindex = dict_get_number(dict, (char_u *)"zindex");
    if (wp->w_zindex < 1)
	wp->w_zindex = POPUPWIN_DEFAULT_ZINDEX;
    if (wp->w_zindex > 32000)
	wp->w_zindex = 32000;

# if defined(FEAT_TIMERS)
    // Add timer to close the popup after some time.
    nr = dict_get_number(dict, (char_u *)"time");
    if (nr > 0)
    {
	char_u	    cbbuf[50];
	char_u	    *ptr = cbbuf;
	typval_T    tv;

	vim_snprintf((char *)cbbuf, sizeof(cbbuf),
					   "{_ -> popup_close(%d)}", wp->w_id);
	if (get_lambda_tv(&ptr, &tv, TRUE) == OK)
	{
	    wp->w_popup_timer = create_timer(nr, 0);
	    wp->w_popup_timer->tr_callback = get_callback(&tv);
	    clear_tv(&tv);
	}
    }
# endif

    // Option values resulting in setting an option.
    str = dict_get_string(dict, (char_u *)"highlight", FALSE);
    if (str != NULL)
	set_string_option_direct_in_win(wp, (char_u *)"wincolor", -1,
						   str, OPT_FREE|OPT_LOCAL, 0);

    di = dict_find(dict, (char_u *)"wrap", -1);
    if (di != NULL)
    {
	nr = dict_get_number(dict, (char_u *)"wrap");
	wp->w_p_wrap = nr != 0;
    }

    di = dict_find(dict, (char_u *)"callback", -1);
    if (di != NULL)
    {
	callback_T	callback = get_callback(&di->di_tv);

	if (callback.cb_name != NULL)
	    set_callback(&wp->w_close_cb, &callback);
    }

    di = dict_find(dict, (char_u *)"filter", -1);
    if (di != NULL)
    {
	callback_T	callback = get_callback(&di->di_tv);

	if (callback.cb_name != NULL)
	    set_callback(&wp->w_filter_cb, &callback);
    }

    get_padding_border(dict, wp->w_popup_padding, "padding", 999);
    get_padding_border(dict, wp->w_popup_border, "border", 1);

    for (i = 0; i < 4; ++i)
	VIM_CLEAR(wp->w_border_highlight[i]);
    di = dict_find(dict, (char_u *)"borderhighlight", -1);
    if (di != NULL)
    {
	if (di->di_tv.v_type != VAR_LIST)
	    emsg(_(e_listreq));
	else
	{
	    list_T	*list = di->di_tv.vval.v_list;
	    listitem_T	*li;

	    if (list != NULL)
		for (i = 0, li = list->lv_first; i < 4 && i < list->lv_len;
							 ++i, li = li->li_next)
		{
		    str = tv_get_string(&li->li_tv);
		    if (*str != NUL)
			wp->w_border_highlight[i] = vim_strsave(str);
		}
	    if (list->lv_len == 1 && wp->w_border_highlight[0] != NULL)
		for (i = 1; i < 4; ++i)
			wp->w_border_highlight[i] =
					vim_strsave(wp->w_border_highlight[0]);
	}
    }

    for (i = 0; i < 8; ++i)
	wp->w_border_char[i] = 0;
    di = dict_find(dict, (char_u *)"borderchars", -1);
    if (di != NULL)
    {
	if (di->di_tv.v_type != VAR_LIST)
	    emsg(_(e_listreq));
	else
	{
	    list_T	*list = di->di_tv.vval.v_list;
	    listitem_T	*li;

	    if (list != NULL)
		for (i = 0, li = list->lv_first; i < 8 && i < list->lv_len;
							 ++i, li = li->li_next)
		{
		    str = tv_get_string(&li->li_tv);
		    if (*str != NUL)
			wp->w_border_char[i] = mb_ptr2char(str);
		}
	    if (list->lv_len == 1)
		for (i = 1; i < 8; ++i)
		    wp->w_border_char[i] = wp->w_border_char[0];
	    if (list->lv_len == 2)
	    {
		for (i = 4; i < 8; ++i)
		    wp->w_border_char[i] = wp->w_border_char[1];
		for (i = 1; i < 4; ++i)
		    wp->w_border_char[i] = wp->w_border_char[0];
	    }
	}
    }

    di = dict_find(dict, (char_u *)"moved", -1);
    if (di != NULL)
    {
	set_moved_values(wp);
	if (di->di_tv.v_type == VAR_STRING && di->di_tv.vval.v_string != NULL)
	{
	    char_u  *s = di->di_tv.vval.v_string;
	    int	    flags = 0;

	    if (STRCMP(s, "word") == 0)
		flags = FIND_IDENT | FIND_STRING;
	    else if (STRCMP(s, "WORD") == 0)
		flags = FIND_STRING;
	    else if (STRCMP(s, "any") != 0)
		semsg(_(e_invarg2), s);
	    if (flags != 0)
		set_moved_columns(wp, flags);
	}
	else if (di->di_tv.v_type == VAR_LIST
		&& di->di_tv.vval.v_list != NULL
		&& di->di_tv.vval.v_list->lv_len == 2)
	{
	    list_T *l = di->di_tv.vval.v_list;

	    wp->w_popup_mincol = tv_get_number(&l->lv_first->li_tv);
	    wp->w_popup_maxcol = tv_get_number(&l->lv_first->li_next->li_tv);
	}
	else
	    semsg(_(e_invarg2), tv_get_string(&di->di_tv));
    }

    popup_mask_refresh = TRUE;
}

/*
 * Add lines to the popup from a list of strings.
 */
    static void
add_popup_strings(buf_T *buf, list_T *l)
{
    listitem_T  *li;
    linenr_T    lnum = 0;
    char_u	*p;

    for (li = l->lv_first; li != NULL; li = li->li_next)
	if (li->li_tv.v_type == VAR_STRING)
	{
	    p = li->li_tv.vval.v_string;
	    ml_append_buf(buf, lnum++,
			       p == NULL ? (char_u *)"" : p, (colnr_T)0, TRUE);
	}
}

/*
 * Add lines to the popup from a list of dictionaries.
 */
    static void
add_popup_dicts(buf_T *buf, list_T *l)
{
    listitem_T  *li;
    listitem_T  *pli;
    linenr_T    lnum = 0;
    char_u	*p;
    dict_T	*dict;

    // first add the text lines
    for (li = l->lv_first; li != NULL; li = li->li_next)
    {
	if (li->li_tv.v_type != VAR_DICT)
	{
	    emsg(_(e_dictreq));
	    return;
	}
	dict = li->li_tv.vval.v_dict;
	p = dict == NULL ? NULL
			      : dict_get_string(dict, (char_u *)"text", FALSE);
	ml_append_buf(buf, lnum++,
			       p == NULL ? (char_u *)"" : p, (colnr_T)0, TRUE);
    }

    // add the text properties
    lnum = 1;
    for (li = l->lv_first; li != NULL; li = li->li_next, ++lnum)
    {
	dictitem_T	*di;
	list_T		*plist;

	dict = li->li_tv.vval.v_dict;
	di = dict_find(dict, (char_u *)"props", -1);
	if (di != NULL)
	{
	    if (di->di_tv.v_type != VAR_LIST)
	    {
		emsg(_(e_listreq));
		return;
	    }
	    plist = di->di_tv.vval.v_list;
	    if (plist != NULL)
	    {
		for (pli = plist->lv_first; pli != NULL; pli = pli->li_next)
		{
		    if (pli->li_tv.v_type != VAR_DICT)
		    {
			emsg(_(e_dictreq));
			return;
		    }
		    dict = pli->li_tv.vval.v_dict;
		    if (dict != NULL)
		    {
			int col = dict_get_number(dict, (char_u *)"col");

			prop_add_common( lnum, col, dict, buf, NULL);
		    }
		}
	    }
	}
    }
}

/*
 * Adjust the position and size of the popup to fit on the screen.
 */
    void
popup_adjust_position(win_T *wp)
{
    linenr_T	lnum;
    int		wrapped = 0;
    int		maxwidth;
    int		center_vert = FALSE;
    int		center_hor = FALSE;
    int		allow_adjust_left = !wp->w_popup_fixed;
    int		top_extra = wp->w_popup_border[0] + wp->w_popup_padding[0];
    int		right_extra = wp->w_popup_border[1] + wp->w_popup_padding[1];
    int		bot_extra = wp->w_popup_border[2] + wp->w_popup_padding[2];
    int		left_extra = wp->w_popup_border[3] + wp->w_popup_padding[3];
    int		extra_height = top_extra + bot_extra;
    int		extra_width = left_extra + right_extra;
    int		org_winrow = wp->w_winrow;
    int		org_wincol = wp->w_wincol;
    int		org_width = wp->w_width;
    int		org_height = wp->w_height;

    wp->w_winrow = 0;
    wp->w_wincol = 0;
    if (wp->w_popup_pos == POPPOS_CENTER)
    {
	// center after computing the size
	center_vert = TRUE;
	center_hor = TRUE;
    }
    else
    {
	if (wp->w_wantline == 0)
	    center_vert = TRUE;
	else if (wp->w_popup_pos == POPPOS_TOPLEFT
		|| wp->w_popup_pos == POPPOS_TOPRIGHT)
	{
	    wp->w_winrow = wp->w_wantline - 1;
	    if (wp->w_winrow >= Rows)
		wp->w_winrow = Rows - 1;
	}

	if (wp->w_wantcol == 0)
	    center_hor = TRUE;
	else if (wp->w_popup_pos == POPPOS_TOPLEFT
		|| wp->w_popup_pos == POPPOS_BOTLEFT)
	{
	    wp->w_wincol = wp->w_wantcol - 1;
	    if (wp->w_wincol >= Columns - 3)
		wp->w_wincol = Columns - 3;
	}
    }

    // When centering or right aligned, use maximum width.
    // When left aligned use the space available, but shift to the left when we
    // hit the right of the screen.
    maxwidth = Columns - wp->w_wincol;
    if (wp->w_maxwidth > 0 && maxwidth > wp->w_maxwidth)
    {
	allow_adjust_left = FALSE;
	maxwidth = wp->w_maxwidth;
    }

    // Compute width based on longest text line and the 'wrap' option.
    // TODO: more accurate wrapping
    wp->w_width = 0;
    for (lnum = 1; lnum <= wp->w_buffer->b_ml.ml_line_count; ++lnum)
    {
	int len = vim_strsize(ml_get_buf(wp->w_buffer, lnum, FALSE));

	if (wp->w_p_wrap)
	{
	    while (len > maxwidth)
	    {
		++wrapped;
		len -= maxwidth;
		wp->w_width = maxwidth;
	    }
	}
	else if (len > maxwidth
		&& allow_adjust_left
		&& (wp->w_popup_pos == POPPOS_TOPLEFT
		    || wp->w_popup_pos == POPPOS_BOTLEFT))
	{
	    // adjust leftwise to fit text on screen
	    int shift_by = ( len - maxwidth );

	    if ( shift_by > wp->w_wincol )
	    {
		int truncate_shift = shift_by - wp->w_wincol;
		len -= truncate_shift;
		shift_by -= truncate_shift;
	    }

	    wp->w_wincol -= shift_by;
	    maxwidth += shift_by;
	    wp->w_width = maxwidth;
	}
	if (wp->w_width < len)
	    wp->w_width = len;
    }

    if (wp->w_minwidth > 0 && wp->w_width < wp->w_minwidth)
	wp->w_width = wp->w_minwidth;
    if (wp->w_width > maxwidth)
	wp->w_width = maxwidth;
    if (center_hor)
	wp->w_wincol = (Columns - wp->w_width) / 2;
    else if (wp->w_popup_pos == POPPOS_BOTRIGHT
	    || wp->w_popup_pos == POPPOS_TOPRIGHT)
    {
	// Right aligned: move to the right if needed.
	// No truncation, because that would change the height.
	if (wp->w_width + extra_width < wp->w_wantcol)
	    wp->w_wincol = wp->w_wantcol - (wp->w_width + extra_width);
    }

    wp->w_height = wp->w_buffer->b_ml.ml_line_count + wrapped;
    if (wp->w_minheight > 0 && wp->w_height < wp->w_minheight)
	wp->w_height = wp->w_minheight;
    if (wp->w_maxheight > 0 && wp->w_height > wp->w_maxheight)
	wp->w_height = wp->w_maxheight;
    if (wp->w_height > Rows - wp->w_winrow)
	wp->w_height = Rows - wp->w_winrow;

    if (center_vert)
	wp->w_winrow = (Rows - wp->w_height) / 2;
    else if (wp->w_popup_pos == POPPOS_BOTRIGHT
	    || wp->w_popup_pos == POPPOS_BOTLEFT)
    {
	if ((wp->w_height + extra_height) <= wp->w_wantline)
	    // bottom aligned: may move down
	    wp->w_winrow = wp->w_wantline - (wp->w_height + extra_height);
	else
	    // not enough space, make top aligned
	    wp->w_winrow = wp->w_wantline + 1;
    }

    wp->w_popup_last_changedtick = CHANGEDTICK(wp->w_buffer);

    // Need to update popup_mask if the position or size changed.
    // And redraw windows that were behind the popup.
    if (org_winrow != wp->w_winrow
	    || org_wincol != wp->w_wincol
	    || org_width != wp->w_width
	    || org_height != wp->w_height)
    {
	// TODO: redraw only windows that were below the popup.
	redraw_all_later(NOT_VALID);
	popup_mask_refresh = TRUE;
    }
}

typedef enum
{
    TYPE_NORMAL,
    TYPE_ATCURSOR
} create_type_T;

/*
 * popup_create({text}, {options})
 * popup_atcursor({text}, {options})
 * When called from f_popup_atcursor() "type" is TYPE_ATCURSOR.
 */
    static void
popup_create(typval_T *argvars, typval_T *rettv, create_type_T type)
{
    win_T   *wp;
    buf_T   *buf;
    dict_T  *d;
    int	    nr;

    // Check arguments look OK.
    if (!(argvars[0].v_type == VAR_STRING && argvars[0].vval.v_string != NULL)
	&& !(argvars[0].v_type == VAR_LIST && argvars[0].vval.v_list != NULL))
    {
	emsg(_(e_listreq));
	return;
    }
    if (argvars[1].v_type != VAR_DICT || argvars[1].vval.v_dict == NULL)
    {
	emsg(_(e_dictreq));
	return;
    }
    d = argvars[1].vval.v_dict;

    // Create the window and buffer.
    wp = win_alloc_popup_win();
    if (wp == NULL)
	return;
    rettv->vval.v_number = wp->w_id;
    wp->w_popup_pos = POPPOS_TOPLEFT;

    buf = buflist_new(NULL, NULL, (linenr_T)0, BLN_NEW|BLN_LISTED|BLN_DUMMY);
    if (buf == NULL)
	return;
    ml_open(buf);

    win_init_popup_win(wp, buf);

    set_local_options_default(wp);
    set_string_option_direct_in_buf(buf, (char_u *)"buftype", -1,
				     (char_u *)"popup", OPT_FREE|OPT_LOCAL, 0);
    set_string_option_direct_in_buf(buf, (char_u *)"bufhidden", -1,
				     (char_u *)"hide", OPT_FREE|OPT_LOCAL, 0);
    buf->b_p_ul = -1;	    // no undo
    buf->b_p_swf = FALSE;   // no swap file
    buf->b_p_bl = FALSE;    // unlisted buffer
    buf->b_locked = TRUE;
    wp->w_p_wrap = TRUE;  // 'wrap' is default on

    // Avoid that 'buftype' is reset when this buffer is entered.
    buf->b_p_initialized = TRUE;

    nr = (int)dict_get_number(d, (char_u *)"tab");
    if (nr == 0)
    {
	// popup on current tab
	wp->w_next = curtab->tp_first_popupwin;
	curtab->tp_first_popupwin = wp;
    }
    else if (nr < 0)
    {
	// global popup
	wp->w_next = first_popupwin;
	first_popupwin = wp;
    }
    else
	// TODO: find tab page "nr"
	emsg("Not implemented yet");

    // Add text to the buffer.
    if (argvars[0].v_type == VAR_STRING)
    {
	// just a string
	ml_append_buf(buf, 0, argvars[0].vval.v_string, (colnr_T)0, TRUE);
    }
    else
    {
	list_T *l = argvars[0].vval.v_list;

	if (l->lv_len > 0)
	{
	    if (l->lv_first->li_tv.v_type == VAR_STRING)
		// list of strings
		add_popup_strings(buf, l);
	    else
		// list of dictionaries
		add_popup_dicts(buf, l);
	}
    }

    // Delete the line of the empty buffer.
    curbuf = buf;
    ml_delete(buf->b_ml.ml_line_count, FALSE);
    curbuf = curwin->w_buffer;

    if (type == TYPE_ATCURSOR)
    {
	wp->w_popup_pos = POPPOS_BOTLEFT;
	setcursor_mayforce(TRUE);
	wp->w_wantline = screen_screenrow();
	if (wp->w_wantline == 0)  // cursor in first line
	{
	    wp->w_wantline = 2;
	    wp->w_popup_pos = POPPOS_TOPLEFT;
	}
	wp->w_wantcol = screen_screencol() + 1;
	set_moved_values(wp);
	set_moved_columns(wp, FIND_STRING);
    }

    // set default values
    wp->w_zindex = POPUPWIN_DEFAULT_ZINDEX;

    // Deal with options.
    apply_options(wp, buf, argvars[1].vval.v_dict);

    popup_adjust_position(wp);

    wp->w_vsep_width = 0;

    redraw_all_later(NOT_VALID);
    popup_mask_refresh = TRUE;
}

/*
 * popup_clear()
 */
    void
f_popup_clear(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    close_all_popups();
}

/*
 * popup_create({text}, {options})
 */
    void
f_popup_create(typval_T *argvars, typval_T *rettv)
{
    popup_create(argvars, rettv, TYPE_NORMAL);
}

/*
 * popup_atcursor({text}, {options})
 */
    void
f_popup_atcursor(typval_T *argvars, typval_T *rettv)
{
    popup_create(argvars, rettv, TYPE_ATCURSOR);
}

/*
 * Find the popup window with window-ID "id".
 * If the popup window does not exist NULL is returned.
 * If the window is not a popup window, and error message is given.
 */
    static win_T *
find_popup_win(int id)
{
    win_T *wp = win_id2wp(id);

    if (wp != NULL && !bt_popup(wp->w_buffer))
    {
	semsg(_("E993: window %d is not a popup window"), id);
	return NULL;
    }
    return wp;
}

/*
 * Return TRUE if there any popups that are not hidden.
 */
    int
popup_any_visible(void)
{
    win_T *wp;

    for (wp = first_popupwin; wp != NULL; wp = wp->w_next)
	if ((wp->w_popup_flags & POPF_HIDDEN) == 0)
	    return TRUE;
    for (wp = curtab->tp_first_popupwin; wp != NULL; wp = wp->w_next)
	if ((wp->w_popup_flags & POPF_HIDDEN) == 0)
	    return TRUE;
    return FALSE;
}

/*
 * Invoke the close callback for window "wp" with value "result".
 * Careful: The callback may make "wp" invalid!
 */
    static void
invoke_popup_callback(win_T *wp, typval_T *result)
{
    typval_T	rettv;
    int		dummy;
    typval_T	argv[3];

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = (varnumber_T)wp->w_id;

    if (result != NULL && result->v_type != VAR_UNKNOWN)
	copy_tv(result, &argv[1]);
    else
    {
	argv[1].v_type = VAR_NUMBER;
	argv[1].vval.v_number = 0;
    }

    argv[2].v_type = VAR_UNKNOWN;

    call_callback(&wp->w_close_cb, -1,
			    &rettv, 2, argv, NULL, 0L, 0L, &dummy, TRUE, NULL);
    if (result != NULL)
	clear_tv(&argv[1]);
    clear_tv(&rettv);
}

/*
 * Close popup "wp" and invoke any close callback for it.
 */
    static void
popup_close_and_callback(win_T *wp, typval_T *arg)
{
    int id = wp->w_id;

    if (wp->w_close_cb.cb_name != NULL)
	// Careful: This may make "wp" invalid.
	invoke_popup_callback(wp, arg);

    popup_close(id);
}

/*
 * popup_close({id})
 */
    void
f_popup_close(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id = (int)tv_get_number(argvars);
    win_T	*wp = find_popup_win(id);

    if (wp != NULL)
	popup_close_and_callback(wp, &argvars[1]);
}

/*
 * popup_hide({id})
 */
    void
f_popup_hide(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id = (int)tv_get_number(argvars);
    win_T	*wp = find_popup_win(id);

    if (wp != NULL && (wp->w_popup_flags & POPF_HIDDEN) == 0)
    {
	wp->w_popup_flags |= POPF_HIDDEN;
	--wp->w_buffer->b_nwindows;
	redraw_all_later(NOT_VALID);
	popup_mask_refresh = TRUE;
    }
}

/*
 * popup_show({id})
 */
    void
f_popup_show(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id = (int)tv_get_number(argvars);
    win_T	*wp = find_popup_win(id);

    if (wp != NULL && (wp->w_popup_flags & POPF_HIDDEN) != 0)
    {
	wp->w_popup_flags &= ~POPF_HIDDEN;
	++wp->w_buffer->b_nwindows;
	redraw_all_later(NOT_VALID);
	popup_mask_refresh = TRUE;
    }
}

    static void
popup_free(win_T *wp)
{
    wp->w_buffer->b_locked = FALSE;
    if (wp->w_winrow + wp->w_height >= cmdline_row)
	clear_cmdline = TRUE;
    win_free_popup(wp);
    redraw_all_later(NOT_VALID);
    popup_mask_refresh = TRUE;
}

/*
 * Close a popup window by Window-id.
 * Does not invoke the callback.
 */
    void
popup_close(int id)
{
    win_T	*wp;
    tabpage_T	*tp;
    win_T	*prev = NULL;

    // go through global popups
    for (wp = first_popupwin; wp != NULL; prev = wp, wp = wp->w_next)
	if (wp->w_id == id)
	{
	    if (prev == NULL)
		first_popupwin = wp->w_next;
	    else
		prev->w_next = wp->w_next;
	    popup_free(wp);
	    return;
	}

    // go through tab-local popups
    FOR_ALL_TABPAGES(tp)
	popup_close_tabpage(tp, id);
}

/*
 * Close a popup window with Window-id "id" in tabpage "tp".
 */
    void
popup_close_tabpage(tabpage_T *tp, int id)
{
    win_T	*wp;
    win_T	**root = &tp->tp_first_popupwin;
    win_T	*prev = NULL;

    for (wp = *root; wp != NULL; prev = wp, wp = wp->w_next)
	if (wp->w_id == id)
	{
	    if (prev == NULL)
		*root = wp->w_next;
	    else
		prev->w_next = wp->w_next;
	    popup_free(wp);
	    return;
	}
}

    void
close_all_popups(void)
{
    while (first_popupwin != NULL)
	popup_close(first_popupwin->w_id);
    while (curtab->tp_first_popupwin != NULL)
	popup_close(curtab->tp_first_popupwin->w_id);
}

/*
 * popup_move({id}, {options})
 */
    void
f_popup_move(typval_T *argvars, typval_T *rettv UNUSED)
{
    dict_T	*d;
    int		nr;
    int		id = (int)tv_get_number(argvars);
    win_T	*wp = find_popup_win(id);

    if (wp == NULL)
	return;  // invalid {id}

    if (argvars[1].v_type != VAR_DICT || argvars[1].vval.v_dict == NULL)
    {
	emsg(_(e_dictreq));
	return;
    }
    d = argvars[1].vval.v_dict;

    if ((nr = dict_get_number(d, (char_u *)"minwidth")) > 0)
	wp->w_minwidth = nr;
    if ((nr = dict_get_number(d, (char_u *)"minheight")) > 0)
	wp->w_minheight = nr;
    if ((nr = dict_get_number(d, (char_u *)"maxwidth")) > 0)
	wp->w_maxwidth = nr;
    if ((nr = dict_get_number(d, (char_u *)"maxheight")) > 0)
	wp->w_maxheight = nr;
    get_pos_options(wp, d);

    if (wp->w_winrow + wp->w_height >= cmdline_row)
	clear_cmdline = TRUE;
    popup_adjust_position(wp);
}

/*
 * popup_getpos({id})
 */
    void
f_popup_getpos(typval_T *argvars, typval_T *rettv)
{
    dict_T	*dict;
    int		id = (int)tv_get_number(argvars);
    win_T	*wp = find_popup_win(id);
    int		top_extra;
    int		left_extra;

    if (rettv_dict_alloc(rettv) == OK)
    {
	if (wp == NULL)
	    return;  // invalid {id}
	top_extra = wp->w_popup_border[0] + wp->w_popup_padding[0];
	left_extra = wp->w_popup_border[3] + wp->w_popup_padding[3];

	dict = rettv->vval.v_dict;

	dict_add_number(dict, "line", wp->w_winrow + 1);
	dict_add_number(dict, "col", wp->w_wincol + 1);
	dict_add_number(dict, "width", wp->w_width + left_extra + wp->w_popup_border[1] + wp->w_popup_padding[1]);
	dict_add_number(dict, "height", wp->w_height + top_extra + wp->w_popup_border[2] + wp->w_popup_padding[2]);

	dict_add_number(dict, "core_line", wp->w_winrow + 1 + top_extra);
	dict_add_number(dict, "core_col", wp->w_wincol + 1 + left_extra);
	dict_add_number(dict, "core_width", wp->w_width);
	dict_add_number(dict, "core_height", wp->w_height);

	dict_add_number(dict, "visible",
		      win_valid(wp) && (wp->w_popup_flags & POPF_HIDDEN) == 0);
    }
}

/*
 * popup_getoptions({id})
 */
    void
f_popup_getoptions(typval_T *argvars, typval_T *rettv)
{
    dict_T	*dict;
    int		id = (int)tv_get_number(argvars);
    win_T	*wp = find_popup_win(id);
    int		i;

    if (rettv_dict_alloc(rettv) == OK)
    {
	if (wp == NULL)
	    return;

	dict = rettv->vval.v_dict;
	dict_add_number(dict, "line", wp->w_wantline);
	dict_add_number(dict, "col", wp->w_wantcol);
	dict_add_number(dict, "minwidth", wp->w_minwidth);
	dict_add_number(dict, "minheight", wp->w_minheight);
	dict_add_number(dict, "maxheight", wp->w_maxheight);
	dict_add_number(dict, "maxwidth", wp->w_maxwidth);
	dict_add_number(dict, "zindex", wp->w_zindex);
	dict_add_number(dict, "fixed", wp->w_popup_fixed);

	for (i = 0; i < (int)(sizeof(poppos_entries) / sizeof(poppos_entry_T));
									   ++i)
	    if (wp->w_popup_pos == poppos_entries[i].pp_val)
	    {
		dict_add_string(dict, "pos",
					  (char_u *)poppos_entries[i].pp_name);
		break;
	    }

# if defined(FEAT_TIMERS)
	dict_add_number(dict, "time", wp->w_popup_timer != NULL
				 ?  (long)wp->w_popup_timer->tr_interval : 0L);
# endif
    }
}

    int
not_in_popup_window()
{
    if (bt_popup(curwin->w_buffer))
    {
	emsg(_("E994: Not allowed in a popup window"));
	return TRUE;
    }
    return FALSE;
}

/*
 * Reset all the POPF_HANDLED flags in global popup windows and popup windows
 * in the current tab.
 */
    void
popup_reset_handled()
{
    win_T *wp;

    for (wp = first_popupwin; wp != NULL; wp = wp->w_next)
	wp->w_popup_flags &= ~POPF_HANDLED;
    for (wp = curtab->tp_first_popupwin; wp != NULL; wp = wp->w_next)
	wp->w_popup_flags &= ~POPF_HANDLED;
}

/*
 * Find the next visible popup where POPF_HANDLED is not set.
 * Must have called popup_reset_handled() first.
 * When "lowest" is TRUE find the popup with the lowest zindex, otherwise the
 * popup with the highest zindex.
 */
    win_T *
find_next_popup(int lowest)
{
    win_T   *wp;
    win_T   *found_wp;
    int	    found_zindex;

    found_zindex = lowest ? INT_MAX : 0;
    found_wp = NULL;
    for (wp = first_popupwin; wp != NULL; wp = wp->w_next)
	if ((wp->w_popup_flags & (POPF_HANDLED|POPF_HIDDEN)) == 0
		&& (lowest ? wp->w_zindex < found_zindex
			   : wp->w_zindex > found_zindex))
	{
	    found_zindex = wp->w_zindex;
	    found_wp = wp;
	}
    for (wp = curtab->tp_first_popupwin; wp != NULL; wp = wp->w_next)
	if ((wp->w_popup_flags & (POPF_HANDLED|POPF_HIDDEN)) == 0
		&& (lowest ? wp->w_zindex < found_zindex
			   : wp->w_zindex > found_zindex))
	{
	    found_zindex = wp->w_zindex;
	    found_wp = wp;
	}

    if (found_wp != NULL)
	found_wp->w_popup_flags |= POPF_HANDLED;
    return found_wp;
}

/*
 * Invoke the filter callback for window "wp" with typed character "c".
 * Uses the global "mod_mask" for modifiers.
 * Returns the return value of the filter.
 * Careful: The filter may make "wp" invalid!
 */
    static int
invoke_popup_filter(win_T *wp, int c)
{
    int		res;
    typval_T	rettv;
    int		dummy;
    typval_T	argv[3];
    char_u	buf[NUMBUFLEN];

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = (varnumber_T)wp->w_id;

    // Convert the number to a string, so that the function can use:
    //	    if a:c == "\<F2>"
    buf[special_to_buf(c, mod_mask, TRUE, buf)] = NUL;
    argv[1].v_type = VAR_STRING;
    argv[1].vval.v_string = vim_strsave(buf);

    argv[2].v_type = VAR_UNKNOWN;

    call_callback(&wp->w_filter_cb, -1,
			    &rettv, 2, argv, NULL, 0L, 0L, &dummy, TRUE, NULL);
    res = tv_get_number(&rettv);
    vim_free(argv[1].vval.v_string);
    clear_tv(&rettv);
    return res;
}

/*
 * Called when "c" was typed: invoke popup filter callbacks.
 * Returns TRUE when the character was consumed,
 */
    int
popup_do_filter(int c)
{
    int		res = FALSE;
    win_T   *wp;

    popup_reset_handled();

    while (!res && (wp = find_next_popup(FALSE)) != NULL)
	if (wp->w_filter_cb.cb_name != NULL)
	    res = invoke_popup_filter(wp, c);

    return res;
}

/*
 * Called when the cursor moved: check if any popup needs to be closed if the
 * cursor moved far enough.
 */
    void
popup_check_cursor_pos()
{
    win_T *wp;
    typval_T tv;

    popup_reset_handled();
    while ((wp = find_next_popup(TRUE)) != NULL)
	if (wp->w_popup_curwin != NULL
		&& (curwin != wp->w_popup_curwin
		    || curwin->w_cursor.lnum != wp->w_popup_lnum
		    || curwin->w_cursor.col < wp->w_popup_mincol
		    || curwin->w_cursor.col > wp->w_popup_maxcol))
	{
	    tv.v_type = VAR_NUMBER;
	    tv.vval.v_number = -1;
	    popup_close_and_callback(wp, &tv);
	}
}

#endif // FEAT_TEXT_PROP
