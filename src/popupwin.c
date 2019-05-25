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
    wp->w_maxwidth = dict_get_number(dict, (char_u *)"maxwidth");
    wp->w_maxheight = dict_get_number(dict, (char_u *)"maxheight");
    wp->w_winrow = dict_get_number(dict, (char_u *)"line");
    wp->w_wincol = dict_get_number(dict, (char_u *)"col");
    wp->w_zindex = dict_get_number(dict, (char_u *)"zindex");
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
    curbuf = buf;
    set_string_option_direct((char_u *)"buftype", -1,
				     (char_u *)"popup", OPT_FREE|OPT_LOCAL, 0);
    set_string_option_direct((char_u *)"bufhidden", -1,
				     (char_u *)"hide", OPT_FREE|OPT_LOCAL, 0);
    curbuf = curwin->w_buffer;
    buf->b_p_ul = -1;	    // no undo
    buf->b_p_swf = FALSE;   // no swap file
    buf->b_p_bl = FALSE;    // unlisted buffer

    win_init_popup_win(wp, buf);

    nr = (int)dict_get_number(d, (char_u *)"tab");
    if (nr == 0)
    {
	// popup on current tab
	wp->w_next = first_tab_popupwin;
	first_tab_popupwin = wp;
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
	// just a string
	ml_append_buf(buf, 0, argvars[0].vval.v_string, (colnr_T)0, TRUE);
    else if (argvars[0].vval.v_list->lv_first->li_tv.v_type == VAR_STRING)
    {
	listitem_T  *li;
	linenr_T    lnum = 0;
	char_u	    *p;

	// list of strings
	for (li = argvars[0].vval.v_list->lv_first; li != NULL;
							      li = li->li_next)
	    if (li->li_tv.v_type == VAR_STRING)
	    {
		p = li->li_tv.vval.v_string;
		ml_append_buf(buf, lnum++,
			       p == NULL ? (char_u *)"" : p, (colnr_T)0, TRUE);
	    }
    }
    else
	// TODO: handle a list of dictionaries
	emsg("Not implemented yet");

    // Delete the line of the empty buffer.
    curbuf = buf;
    ml_delete(buf->b_ml.ml_line_count, FALSE);
    curbuf = curwin->w_buffer;

    // Deal with options.
    apply_options(wp, buf, argvars[1].vval.v_dict);

    // set default values
    if (wp->w_zindex == 0)
	wp->w_zindex = 50;

    // TODO: Compute the size and position properly.

    // Default position is in middle of the screen, assuming a small popup
    if (wp->w_winrow == 0)
	wp->w_winrow = Rows > 5 ? Rows / 2 - 2 : 0;
    else
	--wp->w_winrow;  // option value is one-based
    if (wp->w_wincol == 0)
	wp->w_wincol = Columns > 20 ? Columns / 2 - 10 : 0;
    else
	--wp->w_wincol;  // option value is one-based


    // TODO: set width based on longest text line and the 'wrap' option
    wp->w_width = wp->w_maxwidth == 0 ? 20 : wp->w_maxwidth;
    if (wp->w_maxwidth > 0 && wp->w_width > wp->w_maxwidth)
	wp->w_width = wp->w_maxwidth;
    if (wp->w_width > Columns - wp->w_wincol)
	wp->w_width = Columns - wp->w_wincol;

    // TODO: adjust height for wrapped lines
    wp->w_height = buf->b_ml.ml_line_count;
    if (wp->w_maxheight > 0 && wp->w_height > wp->w_maxheight)
	wp->w_height = wp->w_maxheight;
    if (wp->w_height > Rows - wp->w_winrow)
	wp->w_height = Rows - wp->w_winrow;

    wp->w_vsep_width = 0;

    redraw_all_later(NOT_VALID);
}

/*
 * popup_close({id})
 */
    void
f_popup_close(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		nr = (int)tv_get_number(argvars);

    popup_close(nr);
}

    void
popup_close(int nr)
{
    win_T	*wp;
    win_T	*prev = NULL;

    for (wp = first_popupwin; wp != NULL; prev = wp, wp = wp->w_next)
	if (wp->w_id == nr)
	{
	    if (prev == NULL)
		first_popupwin = wp->w_next;
	    else
		prev->w_next = wp->w_next;
	    break;
	}

    if (wp == NULL)
    {
	prev = NULL;
	for (wp = first_tab_popupwin; wp != NULL; prev = wp, wp = wp->w_next)
	    if (wp->w_id == nr)
	    {
		if (prev == NULL)
		    first_tab_popupwin = wp->w_next;
		else
		    prev->w_next = wp->w_next;
		break;
	    }
    }
    if (wp != NULL)
    {
	win_free_popup(wp);
	redraw_all_later(NOT_VALID);
    }
}

    void
close_all_popups(void)
{
    while (first_popupwin != NULL)
	popup_close(first_popupwin->w_id);
    while (first_tab_popupwin != NULL)
	popup_close(first_tab_popupwin->w_id);
}

    void
ex_popupclear(exarg_T *eap UNUSED)
{
    close_all_popups();
}

#endif // FEAT_TEXT_PROP
