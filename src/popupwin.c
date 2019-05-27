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

/*
 * Go through the options in "dict" and apply them to buffer "buf" displayed in
 * popup window "wp".
 */
    static void
apply_options(win_T *wp, buf_T *buf UNUSED, dict_T *dict)
{
    int	    nr;
    char_u  *str;

    wp->w_minwidth = dict_get_number(dict, (char_u *)"minwidth");
    wp->w_minheight = dict_get_number(dict, (char_u *)"minheight");
    wp->w_maxwidth = dict_get_number(dict, (char_u *)"maxwidth");
    wp->w_maxheight = dict_get_number(dict, (char_u *)"maxheight");

    wp->w_wantline = dict_get_number(dict, (char_u *)"line");
    wp->w_wantcol = dict_get_number(dict, (char_u *)"col");

    wp->w_zindex = dict_get_number(dict, (char_u *)"zindex");

#if defined(FEAT_TIMERS)
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
	    wp->w_popup_timer->tr_callback =
				  vim_strsave(partial_name(tv.vval.v_partial));
	    func_ref(wp->w_popup_timer->tr_callback);
	    wp->w_popup_timer->tr_partial = tv.vval.v_partial;
	}
    }
#endif

    str = dict_get_string(dict, (char_u *)"highlight", TRUE);
    if (str != NULL)
	set_string_option_direct_in_win(wp, (char_u *)"wincolor", -1,
						   str, OPT_FREE|OPT_LOCAL, 0);
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
    static void
popup_adjust_position(win_T *wp)
{
    // TODO: Compute the size and position properly.
    if (wp->w_wantline > 0)
	wp->w_winrow = wp->w_wantline - 1;
    else
	// TODO: better default
	wp->w_winrow = Rows > 5 ? Rows / 2 - 2 : 0;
    if (wp->w_winrow >= Rows)
	wp->w_winrow = Rows - 1;

    if (wp->w_wantcol > 0)
	wp->w_wincol = wp->w_wantcol - 1;
    else
	// TODO: better default
	wp->w_wincol = Columns > 20 ? Columns / 2 - 10 : 0;
    if (wp->w_wincol >= Columns - 3)
	wp->w_wincol = Columns - 3;

    // TODO: set width based on longest text line and the 'wrap' option
    wp->w_width = vim_strsize(ml_get_buf(wp->w_buffer, 1, FALSE));
    if (wp->w_minwidth > 0 && wp->w_width < wp->w_minwidth)
	wp->w_width = wp->w_minwidth;
    if (wp->w_maxwidth > 0 && wp->w_width > wp->w_maxwidth)
	wp->w_width = wp->w_maxwidth;
    if (wp->w_width > Columns - wp->w_wincol)
	wp->w_width = Columns - wp->w_wincol;

    if (wp->w_height <= 1)
	// TODO: adjust height for wrapped lines
	wp->w_height = wp->w_buffer->b_ml.ml_line_count;
    if (wp->w_minheight > 0 && wp->w_height < wp->w_minheight)
	wp->w_height = wp->w_minheight;
    if (wp->w_maxheight > 0 && wp->w_height > wp->w_maxheight)
	wp->w_height = wp->w_maxheight;
    if (wp->w_height > Rows - wp->w_winrow)
	wp->w_height = Rows - wp->w_winrow;
}

/*
 * popup_create({text}, {options})
 */
    void
f_popup_create(typval_T *argvars, typval_T *rettv)
{
    win_T   *wp;
    buf_T   *buf;
    dict_T  *d;
    int	    nr;

    // Check arguments look OK.
    if (!(argvars[0].v_type == VAR_STRING
		&& argvars[0].vval.v_string != NULL
		&& STRLEN(argvars[0].vval.v_string) > 0)
	&& !(argvars[0].v_type == VAR_LIST
	    && argvars[0].vval.v_list != NULL
	    && argvars[0].vval.v_list->lv_len > 0))
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
    wp->w_p_wrap = TRUE;  // 'wrap' is default on

    buf = buflist_new(NULL, NULL, (linenr_T)0, BLN_NEW|BLN_LISTED|BLN_DUMMY);
    if (buf == NULL)
	return;
    ml_open(buf);
    set_string_option_direct_in_buf(buf, (char_u *)"buftype", -1,
				     (char_u *)"popup", OPT_FREE|OPT_LOCAL, 0);
    set_string_option_direct_in_buf(buf, (char_u *)"bufhidden", -1,
				     (char_u *)"hide", OPT_FREE|OPT_LOCAL, 0);
    buf->b_p_ul = -1;	    // no undo
    buf->b_p_swf = FALSE;   // no swap file
    buf->b_p_bl = FALSE;    // unlisted buffer

    win_init_popup_win(wp, buf);

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

	if (l->lv_first->li_tv.v_type == VAR_STRING)
	    // list of strings
	    add_popup_strings(buf, l);
	else
	    // list of dictionaries
	    add_popup_dicts(buf, l);
    }

    // Delete the line of the empty buffer.
    curbuf = buf;
    ml_delete(buf->b_ml.ml_line_count, FALSE);
    curbuf = curwin->w_buffer;

    // Deal with options.
    apply_options(wp, buf, argvars[1].vval.v_dict);

    // set default values
    if (wp->w_zindex == 0)
	wp->w_zindex = 50;

    popup_adjust_position(wp);

    wp->w_vsep_width = 0;

    redraw_all_later(NOT_VALID);
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
 * popup_close({id})
 */
    void
f_popup_close(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id = (int)tv_get_number(argvars);

    popup_close(id);
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
	redraw_all_later(NOT_VALID);
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
	redraw_all_later(NOT_VALID);
    }
}

    static void
popup_free(win_T *wp)
{
    if (wp->w_winrow + wp->w_height >= cmdline_row)
	clear_cmdline = TRUE;
    win_free_popup(wp);
    redraw_all_later(NOT_VALID);
}

/*
 * Close a popup window by Window-id.
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

    void
ex_popupclear(exarg_T *eap UNUSED)
{
    close_all_popups();
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
    if ((nr = dict_get_number(d, (char_u *)"line")) > 0)
	wp->w_wantline = nr;
    if ((nr = dict_get_number(d, (char_u *)"col")) > 0)
	wp->w_wantcol = nr;
    // TODO: "pos"

    if (wp->w_winrow + wp->w_height >= cmdline_row)
	clear_cmdline = TRUE;
    popup_adjust_position(wp);
    redraw_all_later(NOT_VALID);
}

#endif // FEAT_TEXT_PROP
