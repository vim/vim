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

#if defined(FEAT_PROP_POPUP)

typedef struct {
    string_T	pp_name;
    poppos_T	pp_val;
} poppos_entry_T;

// Snapshot of the popup's drawn rectangle.  Used to redraw what becomes
// exposed when the popup moves, resizes, hides or closes.  "active" is TRUE
// when the popup is an opacity popup that contributes to the blended
// background; non-opacity popups rely on popup_mask instead.
typedef struct {
    int		active;
    int		winrow;
    int		wincol;
    int		height;
    int		width;
    int		leftoff;
    int		zindex;
} popup_area_T;

// Snapshot of the screen cells under an opacity popup's padding, captured
// before win_update() overwrites them so the padding can be re-blended
// against the original background.  "lines" is NULL when no snapshot was
// taken; popup_free_saved_screen() releases all owned buffers.
typedef struct {
    schar_T	*lines;
    int		*attrs;
    u8char_T	*linesuc;
    int		start_row;
    int		start_col;
    int		rows;
    int		cols;
} popup_saved_screen_T;

// Snapshot of popup style/option fields used by popup_setoptions() to detect
// which option changes require a redraw or reposition.
typedef struct {
    linenr_T	firstline;
    int		blend;
    int		flags;
    int		zindex;
    char_u	*scrollbar_highlight;
    char_u	*thumb_highlight;
    char_u	*border_highlight[4];
} popup_style_snapshot_T;

// Snapshot of popup layout fields used by popup_adjust_position() to detect
// whether the position or size changed and the popup mask must be refreshed.
typedef struct {
    int		winrow;
    int		wincol;
    int		width;
    int		height;
    int		leftcol;
    int		leftoff;
    int		has_scrollbar;
    int		topoff;
    int		bottomoff;
    int		leftclip;
    int		rightclip;
} popup_layout_T;

static poppos_entry_T poppos_entries[] = {
    {STR_LITERAL_INIT("botleft"), POPPOS_BOTLEFT},
    {STR_LITERAL_INIT("topleft"), POPPOS_TOPLEFT},
    {STR_LITERAL_INIT("botright"), POPPOS_BOTRIGHT},
    {STR_LITERAL_INIT("topright"), POPPOS_TOPRIGHT},
    {STR_LITERAL_INIT("center"), POPPOS_CENTER}
};

#ifdef HAS_MESSAGE_WINDOW
// Window used for ":echowindow"
static win_T *message_win = NULL;

// Time used for the next ":echowindow" message in msec.
static int  message_win_time = 3000;

// Flag set when a message is added to the message window, timer is started
// when the message window is drawn.  This might be after pressing Enter at the
// hit-enter prompt.
static int    start_message_win_timer = FALSE;

static void may_start_message_win_timer(win_T *wp);
#endif

static int popup_on_cmdline = FALSE;

static void popup_adjust_position(win_T *wp);
static bool popup_area_changed(win_T *wp, popup_area_T *area);
static void popup_redraw_exposed_area(popup_area_T *area);
static void popup_save_area(win_T *wp, popup_area_T *area);
static void popup_free_saved_screen(popup_saved_screen_T *saved_screen);
static void popup_save_padding_screen(win_T *wp,
	popup_saved_screen_T *saved_screen);
static bool popup_layout_changed(win_T *wp, popup_layout_T *layout);
static bool popup_style_changed(win_T *wp, popup_style_snapshot_T *style);
static void popup_save_style(win_T *wp, popup_style_snapshot_T *style);
static void popup_save_layout(win_T *wp, popup_layout_T *layout);
static void redraw_under_popup_area(int winrow, int wincol, int height,
	int width, int leftoff);
static void redraw_overlapped_opacity_popups(int winrow, int wincol,
	int height, int width, int leftoff, int zindex);

/*
 * Get option value for "key", which is "line" or "col".
 * Handles "cursor+N" and "cursor-N".
 * Returns MAXCOL if the entry is not present.
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
	return MAXCOL;

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
	    semsg(_(e_invalid_expression_str), val);
	    return 0;
	}
    }

    if (STRCMP(key, "line") == 0)
	n = screen_screenrow() + 1 + n;
    else // "col"
	n = screen_screencol() + 1 + n;

    // Zero means "not set", use -1 instead.
    if (n == 0)
	n = -1;
    return n;
}

    static int
set_padding_border(dict_T *dict, int *array, char *name, int max_val)
{
    dictitem_T	*di;

    di = dict_find(dict, (char_u *)name, -1);
    if (di == NULL)
	return OK;

    if (di->di_tv.v_type != VAR_LIST)
    {
	emsg(_(e_list_required));
	return FAIL;
    }

    list_T	*list = di->di_tv.vval.v_list;
    listitem_T	*li;
    int		i;
    int		nr;

    for (i = 0; i < 4; ++i)
	array[i] = 1;
    if (list == NULL)
	return OK;

    CHECK_LIST_MATERIALIZE(list);
    for (i = 0, li = list->lv_first; i < 4 && i < list->lv_len;
	    ++i, li = li->li_next)
    {
	nr = (int)tv_get_number(&li->li_tv);
	if (nr >= 0)
	    array[i] = nr > max_val ? max_val : nr;
    }

    return OK;
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

    if (len <= 0)
	return;

    wp->w_popup_mincol = (int)(ptr - ml_get_curline());
    wp->w_popup_maxcol = wp->w_popup_mincol + len - 1;
}

/*
 * Used when popup options contain "mousemoved": set default moved values.
 */
    static void
set_mousemoved_values(win_T *wp)
{
    wp->w_popup_mouse_row = mouse_row;
    wp->w_popup_mouse_mincol = mouse_col;
    wp->w_popup_mouse_maxcol = mouse_col;
}

    static void
update_popup_uses_mouse_move(void)
{
    popup_uses_mouse_move = FALSE;
    if (!popup_visible)
	return;

    win_T *wp;

    FOR_ALL_POPUPWINS(wp)
	if (wp->w_popup_mouse_row != 0)
	{
	    popup_uses_mouse_move = TRUE;
	    return;
	}
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	if (wp->w_popup_mouse_row != 0)
	{
	    popup_uses_mouse_move = TRUE;
	    return;
	}
}

/*
 * Used when popup options contain "moved" with "word" or "WORD".
 */
    static void
set_mousemoved_columns(win_T *wp, int flags)
{
    win_T	*textwp;
    char_u	*text;
    int		col;
    pos_T	pos;
    colnr_T	mcol;

    if (find_word_under_cursor(mouse_row, mouse_col, TRUE, flags,
				  &textwp, &pos.lnum, &text, NULL, &col) != OK)
	return;

    // convert text column to mouse column
    pos.col = col;
    pos.coladd = 0;
    getvcol(textwp, &pos, &mcol, NULL, NULL, 0);
    wp->w_popup_mouse_mincol = mcol;

    pos.col = col + (colnr_T)STRLEN(text) - 1;
    getvcol(textwp, &pos, NULL, NULL, &mcol, 0);
    wp->w_popup_mouse_maxcol = mcol;
    vim_free(text);
}

/*
 * Return TRUE if "row"/"col" is on the border of the popup.
 * The values are relative to the top-left corner.
 */
    int
popup_on_border(win_T *wp, int row, int col)
{
    return (row == 0 && wp->w_popup_border[0] > 0)
	    || (row == popup_height(wp) - 1 && wp->w_popup_border[2] > 0)
	    || (col == 0 && wp->w_popup_border[3] > 0)
	    || (col == popup_width(wp) - 1 && wp->w_popup_border[1] > 0);
}

/*
 * Return TRUE and close the popup if "row"/"col" is on the "X" button of the
 * popup and w_popup_close is POPCLOSE_BUTTON.
 * The values are relative to the top-left corner.
 * Caller should check the left mouse button was clicked.
 * Return TRUE if the popup was closed.
 */
    int
popup_close_if_on_X(win_T *wp, int row, int col)
{
    if (wp->w_popup_close == POPCLOSE_BUTTON
	    && row == 0 && col == popup_width(wp) - 1)
    {
	popup_close_for_mouse_click(wp);
	return TRUE;
    }
    return FALSE;
}

// Values set when dragging a popup window starts.
static int drag_start_row;
static int drag_start_col;
static int drag_start_wantline;
static int drag_start_wantcol;
static int drag_on_resize_handle;

/*
 * Mouse down on border of popup window: start dragging it.
 * Uses mouse_col and mouse_row.
 */
    void
popup_start_drag(win_T *wp, int row, int col)
{
    drag_start_row = mouse_row;
    drag_start_col = mouse_col;
    if (wp->w_wantline <= 0)
	drag_start_wantline = wp->w_winrow + 1;
    else
	drag_start_wantline = wp->w_wantline;
    if (wp->w_wantcol == 0)
	drag_start_wantcol = wp->w_wincol + 1;
    else
	drag_start_wantcol = wp->w_wantcol;

    // Stop centering the popup
    if (wp->w_popup_pos == POPPOS_CENTER)
	wp->w_popup_pos = POPPOS_TOPLEFT;

    drag_on_resize_handle = wp->w_popup_border[1] > 0
			    && wp->w_popup_border[2] > 0
			    && row == popup_height(wp) - 1
			    && col == popup_width(wp) - 1;

    if (wp->w_popup_pos != POPPOS_TOPLEFT && drag_on_resize_handle)
    {
	if (wp->w_popup_pos == POPPOS_TOPRIGHT
		|| wp->w_popup_pos == POPPOS_BOTRIGHT)
	    wp->w_wantcol = wp->w_wincol + 1;
	if (wp->w_popup_pos == POPPOS_BOTLEFT)
	    wp->w_wantline = wp->w_winrow + 1;
	wp->w_popup_pos = POPPOS_TOPLEFT;
    }
}

/*
 * Mouse moved while dragging a popup window: adjust the window popup position
 * or resize.
 */
    void
popup_drag(win_T *wp)
{
    // The popup may be closed before dragging stops.
    if (!win_valid_popup(wp))
	return;

    if ((wp->w_popup_flags & POPF_RESIZE) && drag_on_resize_handle)
    {
	int width_inc = mouse_col - drag_start_col;
	int height_inc = mouse_row - drag_start_row;

	if (width_inc != 0)
	{
	    int width = wp->w_width + width_inc;

	    if (width < 1)
		width = 1;
	    wp->w_minwidth = width;
	    wp->w_maxwidth = width;
	    drag_start_col = mouse_col;
	}

	if (height_inc != 0)
	{
	    int height = wp->w_height + height_inc;

	    if (height < 1)
		height = 1;
	    wp->w_minheight = height;
	    wp->w_maxheight = height;
	    drag_start_row = mouse_row;
	}

	popup_adjust_position(wp);
	return;
    }

    if (!(wp->w_popup_flags & (POPF_DRAG | POPF_DRAGALL)))
	return;
    wp->w_wantline = drag_start_wantline + (mouse_row - drag_start_row);
    if (wp->w_wantline < 1)
	wp->w_wantline = 1;
    if (wp->w_wantline > Rows)
	wp->w_wantline = Rows;
    wp->w_wantcol = drag_start_wantcol + (mouse_col - drag_start_col);
    if (wp->w_wantcol < 1)
	wp->w_wantcol = 1;
    if (wp->w_wantcol > Columns)
	wp->w_wantcol = Columns;

    popup_adjust_position(wp);
}

/*
 * Set w_firstline to match the current "wp->w_topline".
 */
    void
popup_set_firstline(win_T *wp)
{
    int	    height = wp->w_height;

    wp->w_firstline = wp->w_topline;
    popup_adjust_position(wp);

    // we don't want the popup to get smaller, decrement the first line
    // until it doesn't
    while (wp->w_firstline > 1 && wp->w_height < height)
    {
	--wp->w_firstline;
	popup_adjust_position(wp);
    }
}

/*
 * Return TRUE if the position is in the popup window scrollbar.
 */
    int
popup_is_in_scrollbar(win_T *wp, int row, int col)
{
    return wp->w_has_scrollbar
	&& row >= wp->w_popup_border[0]
	&& row < popup_height(wp) - wp->w_popup_border[2]
	&& col == popup_width(wp) - wp->w_popup_border[1] - 1;
}


/*
 * Handle a click in a popup window, if it is in the scrollbar.
 */
    void
popup_handle_scrollbar_click(win_T *wp, int row, int col)
{
    if (!popup_is_in_scrollbar(wp, row, col))
	return;

    int	    height = popup_height(wp);
    int	    new_topline = wp->w_topline;

    if (row >= height / 2)
    {
	// Click in lower half, scroll down.
	if (wp->w_topline < wp->w_buffer->b_ml.ml_line_count)
	    ++new_topline;
    }
    else if (wp->w_topline > 1)
	// click on upper half, scroll up.
	--new_topline;

    if (new_topline == wp->w_topline)
	return;

    set_topline(wp, new_topline);
    if (wp == curwin)
    {
	if (wp->w_cursor.lnum < wp->w_topline)
	{
	    wp->w_cursor.lnum = wp->w_topline;
	    check_cursor();
	}
	else if (wp->w_cursor.lnum >= wp->w_botline)
	{
	    wp->w_cursor.lnum = wp->w_botline - 1;
	    check_cursor();
	}
    }
    popup_set_firstline(wp);
    redraw_win_later(wp, UPD_NOT_VALID);
}

#if defined(FEAT_TIMERS)
/*
 * Add a timer to "wp" with "time".
 * If "close" is true use popup_close(), otherwise popup_hide().
 */
    static void
popup_add_timeout(win_T *wp, int time, int close)
{
    char_u	    cbbuf[50];
    char_u	    *ptr = cbbuf;
    typval_T	    tv;

    vim_snprintf((char *)cbbuf, sizeof(cbbuf),
		close ? "(_) => popup_close(%d)" : "(_) => popup_hide(%d)",
		wp->w_id);
    if (get_lambda_tv_and_compile(&ptr, &tv, FALSE, &EVALARG_EVALUATE) != OK)
	return;

    wp->w_popup_timer = create_timer(time, 0);
    callback_T cb = get_callback(&tv);
    if (cb.cb_name != NULL && !cb.cb_free_name)
    {
	cb.cb_name = vim_strsave(cb.cb_name);
	cb.cb_free_name = TRUE;
    }
    wp->w_popup_timer->tr_callback = cb;
    clear_tv(&tv);
}
#endif

    static poppos_T
get_pos_entry(dict_T *d, int give_error)
{
    char_u  *str = dict_get_string(d, "pos", FALSE);
    int	    nr;

    if (str == NULL)
	return POPPOS_NONE;

    for (nr = 0; nr < (int)ARRAY_LENGTH(poppos_entries); ++nr)
	if (STRCMP(str, poppos_entries[nr].pp_name.string) == 0)
	    return poppos_entries[nr].pp_val;

    if (give_error)
	semsg(_(e_invalid_argument_str), str);
    return POPPOS_NONE;
}

/*
 * Shared between popup_create() and f_popup_move().
 */
    static void
apply_move_options(win_T *wp, dict_T *d)
{
    int		nr;
    char_u	*str;
    dictitem_T	*di;

    if ((nr = dict_get_number_def(d, "minwidth", -1)) >= 0)
	wp->w_minwidth = nr;
    if ((nr = dict_get_number_def(d, "minheight", -1)) >= 0)
	wp->w_minheight = nr;
    if ((nr = dict_get_number_def(d, "maxwidth", -1)) >= 0)
	wp->w_maxwidth = nr;
    if ((nr = dict_get_number_def(d, "maxheight", -1)) >= 0)
	wp->w_maxheight = nr;

    nr = popup_options_one(d, (char_u *)"line");
    if (nr != MAXCOL)
	wp->w_wantline = nr;
    nr = popup_options_one(d, (char_u *)"col");
    if (nr != MAXCOL)
	wp->w_wantcol = nr;


    nr = dict_get_bool(d, "fixed", -1);
    if (nr != -1)
	wp->w_popup_fixed = nr != 0;

    {
	poppos_T ppt = get_pos_entry(d, TRUE);

	if (ppt != POPPOS_NONE)
	    wp->w_popup_pos = ppt;
    }

    str = dict_get_string(d, "textprop", FALSE);
    if (str != NULL)
    {
	wp->w_popup_prop_type = 0;
	if (*str != NUL)
	{
	    wp->w_popup_prop_win = curwin;
	    di = dict_find(d, (char_u *)"textpropwin", -1);
	    if (di != NULL)
	    {
		wp->w_popup_prop_win = find_win_by_nr_or_id(&di->di_tv);
		if (!win_valid_any_tab(wp->w_popup_prop_win))
		    wp->w_popup_prop_win = curwin;
	    }

	    nr = find_prop_type_id(str, wp->w_popup_prop_win->w_buffer);
	    if (nr <= 0)
		nr = find_prop_type_id(str, NULL);
	    if (nr <= 0)
		semsg(_(e_invalid_argument_str), str);
	    else
		wp->w_popup_prop_type = nr;
	}
    }

    di = dict_find(d, (char_u *)"textpropid", -1);
    if (di != NULL)
	wp->w_popup_prop_id = dict_get_number(d, "textpropid");
}

/*
 * Handle "moved" and "mousemoved" arguments.
 */
    static void
handle_moved_argument(win_T *wp, dictitem_T *di, int mousemoved)
{
    if (di->di_tv.v_type == VAR_STRING && di->di_tv.vval.v_string != NULL)
    {
	char_u  *s = di->di_tv.vval.v_string;
	int	flags = 0;

	if (STRCMP(s, "word") == 0)
	    flags = FIND_IDENT | FIND_STRING;
	else if (STRCMP(s, "WORD") == 0)
	    flags = FIND_STRING;
	else if (STRCMP(s, "expr") == 0)
	    flags = FIND_IDENT | FIND_STRING | FIND_EVAL;
	else if (STRCMP(s, "any") != 0)
	    semsg(_(e_invalid_argument_str), s);
	if (flags != 0)
	{
	    if (mousemoved)
		set_mousemoved_columns(wp, flags);
	    else
		set_moved_columns(wp, flags);
	}
    }
    else if (di->di_tv.v_type == VAR_LIST
	    && di->di_tv.vval.v_list != NULL
	    && (di->di_tv.vval.v_list->lv_len == 2
	     || di->di_tv.vval.v_list->lv_len == 3))
    {
	list_T	    *l = di->di_tv.vval.v_list;
	listitem_T  *li;
	int	    mincol;
	int	    maxcol;

	CHECK_LIST_MATERIALIZE(l);
	li = l->lv_first;
	if (l->lv_len == 3)
	{
	    varnumber_T nr = tv_get_number(&l->lv_first->li_tv);

	    // Three numbers, might be from popup_getoptions().
	    if (mousemoved)
		wp->w_popup_mouse_row = nr;
	    else
		wp->w_popup_lnum = nr;
	    li = li->li_next;
	    if (nr == 0)
		wp->w_popup_curwin = NULL;
	}

	mincol = tv_get_number(&li->li_tv);
	maxcol = tv_get_number(&li->li_next->li_tv);
	if (mousemoved)
	{
	    wp->w_popup_mouse_mincol = mincol;
	    wp->w_popup_mouse_maxcol = maxcol;
	}
	else
	{
	    wp->w_popup_mincol = mincol;
	    wp->w_popup_maxcol = maxcol;
	}
    }
    else
	semsg(_(e_invalid_argument_str), tv_get_string(&di->di_tv));
}

    static void
check_highlight(dict_T *dict, char *name, char_u **pval)
{
    dictitem_T  *di;
    char_u	*str;

    di = dict_find(dict, (char_u *)name, -1);
    if (di == NULL)
	return;

    if (di->di_tv.v_type != VAR_STRING)
	semsg(_(e_invalid_value_for_argument_str), name);
    else
    {
	str = tv_get_string(&di->di_tv);
	if (*str != NUL)
	    *pval = vim_strsave(str);
    }
}

/*
 * Scroll to show the line with the cursor.
 */
    static void
popup_show_curline(win_T *wp)
{
    if (wp->w_cursor.lnum < wp->w_topline)
	wp->w_topline = wp->w_cursor.lnum;
    else if (wp->w_cursor.lnum >= wp->w_botline
					      && (wp->w_valid & VALID_BOTLINE))
    {
	wp->w_topline = wp->w_cursor.lnum - wp->w_height + 1;
	if (wp->w_topline < 1)
	    wp->w_topline = 1;
	else if (wp->w_topline > wp->w_buffer->b_ml.ml_line_count)
	    wp->w_topline = wp->w_buffer->b_ml.ml_line_count;
	while (wp->w_topline < wp->w_cursor.lnum
		&& wp->w_topline < wp->w_buffer->b_ml.ml_line_count
		&& plines_m_win(wp, wp->w_topline, wp->w_cursor.lnum,
					    wp->w_height + 1) > wp->w_height)
	    ++wp->w_topline;
    }

    // Don't let "firstline" cause a scroll.
    if (wp->w_firstline > 0)
	wp->w_firstline = wp->w_topline;
}

/*
 * Get the sign group name for window "wp".
 * Returns a pointer to a static buffer, overwritten on the next call.
 */
    static char_u *
popup_get_sign_name(win_T *wp)
{
    static char    buf[30];

    vim_snprintf(buf, sizeof(buf), "popup-%d", wp->w_id);
    return (char_u *)buf;
}

/*
 * Highlight the line with the cursor.
 * Also scrolls the text to put the cursor line in view.
 */
    static void
popup_highlight_curline(win_T *wp)
{
    int	    sign_id = 0;
    char_u  *sign_name = popup_get_sign_name(wp);

    buf_delete_signs(wp->w_buffer, (char_u *)"PopUpMenu");

    if ((wp->w_popup_flags & POPF_CURSORLINE) != 0)
    {
	popup_show_curline(wp);

	if (!sign_exists_by_name(sign_name))
	{
	    char *linehl = "PopupSelected";
	    sign_define_by_name(sign_name, NULL, (char_u *)linehl, NULL, NULL, NULL,
		    NULL, SIGN_DEF_PRIO);
	}

	sign_place(&sign_id, (char_u *)"PopUpMenu", sign_name,
			       wp->w_buffer, wp->w_cursor.lnum, SIGN_DEF_PRIO);
	redraw_win_later(wp, UPD_NOT_VALID);
    }
    else
	sign_undefine_by_name(sign_name, FALSE);
    wp->w_popup_last_curline = wp->w_cursor.lnum;
}

/*
 * Shared between popup_create() and f_popup_setoptions().
 */
    static int
apply_general_options(win_T *wp, dict_T *dict)
{
    dictitem_T	*di;
    int		nr;
    char_u	*str;

    // TODO: flip

    di = dict_find(dict, (char_u *)"firstline", -1);
    if (di != NULL)
    {
	wp->w_firstline = dict_get_number(dict, "firstline");
	if (wp->w_firstline < 0)
	    wp->w_firstline = -1;
    }

    nr = dict_get_bool(dict, "scrollbar", -1);
    if (nr != -1)
	wp->w_want_scrollbar = nr;

    str = dict_get_string(dict, "title", FALSE);
    if (str != NULL)
    {
	vim_free(wp->w_popup_title);
	wp->w_popup_title = vim_strsave(str);
    }

    nr = dict_get_bool(dict, "wrap", -1);
    if (nr != -1)
	wp->w_p_wrap = nr != 0;

    nr = dict_get_bool(dict, "drag", -1);
    if (nr != -1)
    {
	if (nr)
	    wp->w_popup_flags |= POPF_DRAG;
	else
	    wp->w_popup_flags &= ~POPF_DRAG;
    }
    nr = dict_get_bool(dict, "dragall", -1);
    if (nr != -1)
    {
	if (nr)
	    wp->w_popup_flags |= POPF_DRAGALL;
	else
	    wp->w_popup_flags &= ~POPF_DRAGALL;
    }

    nr = dict_get_bool(dict, "posinvert", -1);
    if (nr != -1)
    {
	if (nr)
	    wp->w_popup_flags |= POPF_POSINVERT;
	else
	    wp->w_popup_flags &= ~POPF_POSINVERT;
    }

    nr = dict_get_bool(dict, "clipwindow", -1);
    if (nr != -1)
    {
	if (nr)
	    wp->w_popup_flags |= POPF_CLIPWINDOW;
	else
	    wp->w_popup_flags &= ~POPF_CLIPWINDOW;
    }

    nr = dict_get_bool(dict, "resize", -1);
    if (nr != -1)
    {
	if (nr)
	    wp->w_popup_flags |= POPF_RESIZE;
	else
	    wp->w_popup_flags &= ~POPF_RESIZE;
    }

    di = dict_find(dict, (char_u *)"opacity", -1);
    if (di != NULL)
    {
	nr = dict_get_number(dict, "opacity");
	if (nr == 0)
	{
	    // opacity: 0, fully transparent
	    wp->w_popup_flags |= POPF_OPACITY;
	    wp->w_popup_blend = 100;
	}
	else if (nr > 0 && nr < 100)
	{
	    // opacity: 1-99, partially transparent
	    // Convert to blend (0=opaque, 100=transparent)
	    wp->w_popup_flags |= POPF_OPACITY;
	    wp->w_popup_blend = 100 - nr;
	}
	else if (nr == 100)
	{
	    // Fully opaque, same as no opacity set.
	    wp->w_popup_flags &= ~POPF_OPACITY;
	    wp->w_popup_blend = 0;
	}
	else
	{
	    wp->w_popup_flags &= ~POPF_OPACITY;
	    wp->w_popup_blend = 0;
	}
    }

    di = dict_find(dict, (char_u *)"close", -1);
    if (di != NULL)
    {
	int ok = TRUE;

	if (di->di_tv.v_type == VAR_STRING && di->di_tv.vval.v_string != NULL)
	{
	    char_u  *s = di->di_tv.vval.v_string;

	    if (STRCMP(s, "none") == 0)
		wp->w_popup_close = POPCLOSE_NONE;
	    else if (STRCMP(s, "button") == 0)
		wp->w_popup_close = POPCLOSE_BUTTON;
	    else if (STRCMP(s, "click") == 0)
		wp->w_popup_close = POPCLOSE_CLICK;
	    else
		ok = FALSE;
	}
	else
	    ok = FALSE;
	if (!ok)
	    semsg(_(e_invalid_value_for_argument_str_str), "close", tv_get_string(&di->di_tv));
    }

    str = dict_get_string(dict, "highlight", FALSE);
    if (str != NULL)
    {
	char *errmsg = update_wincolor(wp, str);

	if (errmsg == NULL)
	    set_string_option_direct_in_win(wp, (char_u *)"wincolor", -1,
		    str, OPT_FREE|OPT_LOCAL, 0);
	else
	    emsg(_(errmsg));
    }

    str = dict_get_string(dict, "highlights", FALSE);
    if (str != NULL)
    {
	char *errmsg = update_winhighlight(wp, str);

	if (errmsg == NULL)
	    set_string_option_direct_in_win(wp, (char_u *)"winhighlight", -1,
		    str, OPT_FREE|OPT_LOCAL, 0);
	else
	    emsg(_(errmsg));
    }

    if (set_padding_border(dict, wp->w_popup_padding, "padding", 999) == FAIL ||
	set_padding_border(dict, wp->w_popup_border, "border", 1) == FAIL)
	return FAIL;

    di = dict_find(dict, (char_u *)"borderhighlight", -1);
    if (di != NULL)
    {
	if (di->di_tv.v_type != VAR_LIST || di->di_tv.vval.v_list == NULL)
	{
	    emsg(_(e_list_required));
	    return FAIL;
	}
	else
	{
	    list_T	*list = di->di_tv.vval.v_list;
	    listitem_T	*li;
	    int		i;

	    CHECK_LIST_MATERIALIZE(list);
	    wp->w_border_highlight_isset = true;
	    // Clear all highlights if list is empty
	    if (list->lv_len == 0)
	    {
		for (i = 0; i < 4; ++i)
		{
		    vim_free(wp->w_border_highlight[i]);
		    wp->w_border_highlight[i] = NULL;
		}
	    }
	    else
	    {
		for (i = 0, li = list->lv_first; i < 4 && i < list->lv_len;
						     ++i, li = li->li_next)
		{
		    str = tv_get_string(&li->li_tv);
		    if (*str != NUL)
		    {
			vim_free(wp->w_border_highlight[i]);
			wp->w_border_highlight[i] = vim_strsave(str);
		    }
		}
		if (list->lv_len == 1 && wp->w_border_highlight[0] != NULL)
		    for (i = 1; i < 4; ++i)
		    {
			vim_free(wp->w_border_highlight[i]);
			wp->w_border_highlight[i] =
					vim_strsave(wp->w_border_highlight[0]);
		    }
	    }
	}
    }

    di = dict_find(dict, (char_u *)"borderchars", -1);
    if (di != NULL)
    {
	if (di->di_tv.v_type != VAR_LIST)
	{
	    emsg(_(e_list_required));
	    return FAIL;
	}
	else
	{
	    list_T	*list = di->di_tv.vval.v_list;
	    listitem_T	*li;
	    int		i;

	    if (list != NULL)
	    {
		CHECK_LIST_MATERIALIZE(list);
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
    }

    check_highlight(dict, "scrollbarhighlight", &wp->w_scrollbar_highlight);
    check_highlight(dict, "thumbhighlight", &wp->w_thumb_highlight);

    di = dict_find(dict, (char_u *)"zindex", -1);
    if (di != NULL)
    {
	wp->w_zindex = dict_get_number(dict, "zindex");
	if (wp->w_zindex < 1)
	    wp->w_zindex = POPUPWIN_DEFAULT_ZINDEX;
	if (wp->w_zindex > 32000)
	    wp->w_zindex = 32000;
    }

    di = dict_find(dict, (char_u *)"mask", -1);
    if (di != NULL)
    {
	int ok = FALSE;

	if (di->di_tv.v_type == VAR_LIST && di->di_tv.vval.v_list != NULL)
	{
	    listitem_T *li;

	    ok = TRUE;
	    FOR_ALL_LIST_ITEMS(di->di_tv.vval.v_list, li)
	    {
		if (li->li_tv.v_type != VAR_LIST
			|| li->li_tv.vval.v_list == NULL
			|| li->li_tv.vval.v_list->lv_len != 4)
		{
		    ok = FALSE;
		    break;
		}
		else
		    CHECK_LIST_MATERIALIZE(li->li_tv.vval.v_list);
	    }
	}
	if (ok)
	{
	    wp->w_popup_mask = di->di_tv.vval.v_list;
	    ++wp->w_popup_mask->lv_refcount;
	    VIM_CLEAR(wp->w_popup_mask_cells);
	}
	else
	{
	    semsg(_(e_invalid_value_for_argument_str), "mask");
	    return FAIL;
	}
    }

#if defined(FEAT_TIMERS)
    // Add timer to close the popup after some time.
    nr = dict_get_number(dict, "time");
    if (nr > 0)
	popup_add_timeout(wp, nr, TRUE);
#endif

    di = dict_find(dict, (char_u *)"moved", -1);
    if (di != NULL)
    {
	set_moved_values(wp);
	handle_moved_argument(wp, di, FALSE);
    }

    di = dict_find(dict, (char_u *)"mousemoved", -1);
    if (di != NULL)
    {
	set_mousemoved_values(wp);
	handle_moved_argument(wp, di, TRUE);
    }

    nr = dict_get_bool(dict, "cursorline", -1);
    if (nr != -1)
    {
	if (nr != 0)
	    wp->w_popup_flags |= POPF_CURSORLINE;
	else
	    wp->w_popup_flags &= ~POPF_CURSORLINE;
    }

    di = dict_find(dict, (char_u *)"filter", -1);
    if (di != NULL)
    {
	callback_T	callback = get_callback(&di->di_tv);

	if (callback.cb_name != NULL)
	{
	    free_callback(&wp->w_filter_cb);
	    set_callback(&wp->w_filter_cb, &callback);
	}
    }
    nr = dict_get_bool(dict, "mapping", -1);
    if (nr != -1)
    {
	if (nr)
	    wp->w_popup_flags |= POPF_MAPPING;
	else
	    wp->w_popup_flags &= ~POPF_MAPPING;
    }

    str = dict_get_string(dict, "filtermode", FALSE);
    if (str != NULL)
    {
	if (STRCMP(str, "a") == 0)
	    wp->w_filter_mode = MODE_ALL;
	else
	    wp->w_filter_mode = mode_str2flags(str);
    }

    di = dict_find(dict, (char_u *)"callback", -1);
    if (di == NULL)
	return OK;

    callback_T	callback = get_callback(&di->di_tv);
    if (callback.cb_name == NULL)
	return OK;

    free_callback(&wp->w_close_cb);
    set_callback(&wp->w_close_cb, &callback);
    return OK;
}

/*
 * Go through the options in "dict" and apply them to popup window "wp".
 * "create" is TRUE when creating a new popup window.
 */
    static int
apply_options(win_T *wp, dict_T *dict, int create)
{
    int		nr;

    apply_move_options(wp, dict);

    if (create)
	set_string_option_direct_in_win(wp, (char_u *)"signcolumn", -1,
					(char_u *)"no", OPT_FREE|OPT_LOCAL, 0);

    if (apply_general_options(wp, dict) == FAIL)
	return FAIL;

    nr = dict_get_bool(dict, "hidden", FALSE);
    if (nr > 0)
	wp->w_popup_flags |= POPF_HIDDEN | POPF_HIDDEN_FORCE;

    // when "firstline" and "cursorline" are both set and the cursor would be
    // above or below the displayed lines, move the cursor to "firstline".
    if (wp->w_firstline > 0 && (wp->w_popup_flags & POPF_CURSORLINE))
    {
	if (wp->w_firstline > wp->w_buffer->b_ml.ml_line_count)
	    wp->w_cursor.lnum = wp->w_buffer->b_ml.ml_line_count;
	else if (wp->w_cursor.lnum < wp->w_firstline
		|| wp->w_cursor.lnum >= wp->w_firstline + wp->w_height)
	    wp->w_cursor.lnum = wp->w_firstline;
	wp->w_topline = wp->w_firstline;
	wp->w_valid &= ~VALID_BOTLINE;
    }

    if (create)
	popup_mask_refresh = TRUE;
    popup_highlight_curline(wp);

    return OK;
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

    FOR_ALL_LIST_ITEMS(l, li)
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
    FOR_ALL_LIST_ITEMS(l, li)
    {
	if (li->li_tv.v_type != VAR_DICT)
	{
	    semsg(_(e_argument_1_list_item_nr_dictionary_required), lnum + 1);
	    return;
	}
	dict = li->li_tv.vval.v_dict;
	p = dict == NULL ? NULL : dict_get_string(dict, "text", FALSE);
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
		emsg(_(e_list_required));
		return;
	    }
	    plist = di->di_tv.vval.v_list;
	    if (plist != NULL)
	    {
		FOR_ALL_LIST_ITEMS(plist, pli)
		{
		    if (pli->li_tv.v_type != VAR_DICT)
		    {
			emsg(_(e_dictionary_required));
			return;
		    }
		    dict = pli->li_tv.vval.v_dict;
		    if (dict != NULL)
		    {
			int col = dict_get_number(dict, "col");

			prop_add_common(lnum, col, dict, buf, NULL);
		    }
		}
	    }
	}
    }
}

/*
 * Get the padding plus border at the top, adjusted to 1 if there is a title.
 */
    int
popup_top_extra(win_T *wp)
{
    int	extra = wp->w_popup_border[0] + wp->w_popup_padding[0];

    if (extra == 0 && wp->w_popup_title != NULL && *wp->w_popup_title != NUL)
	return 1;
    return extra;
}

/*
 * Get the padding plus border at the left.
 */
    int
popup_left_extra(win_T *wp)
{
    return wp->w_popup_border[3] + wp->w_popup_padding[3];
}

/*
 * Return the height of popup window "wp", including border and padding.
 */
    int
popup_height(win_T *wp)
{
    return wp->w_height
	    + popup_top_extra(wp)
	    + wp->w_popup_padding[2] + wp->w_popup_border[2];
}

/*
 * Return the width of popup window "wp", including border, padding and
 * scrollbar.
 */
    int
popup_width(win_T *wp)
{
    // w_leftcol is how many columns of the core are left of the screen
    // w_popup_rightoff is how many columns of the core are right of the screen
    return wp->w_width + wp->w_leftcol
	    + popup_extra_width(wp)
	    + wp->w_popup_rightoff;
}

/*
 * Return the extra width of popup window "wp": border, padding and scrollbar.
 */
    int
popup_extra_width(win_T *wp)
{
    return wp->w_popup_padding[3] + wp->w_popup_border[3]
	    + wp->w_popup_padding[1] + wp->w_popup_border[1]
	    + wp->w_has_scrollbar;
}

/*
 * Return the host window used to clip popup "wp" when POPF_CLIPWINDOW is set,
 * or NULL when no clipping should be applied (option off, or the host window
 * is no longer valid).  The textprop window is used as the host; popups not
 * anchored to a textprop are not clipped.
 */
    static win_T *
popup_get_clipwin(win_T *wp)
{
    if (!(wp->w_popup_flags & POPF_CLIPWINDOW))
	return NULL;
    if (win_valid(wp->w_popup_prop_win))
	return wp->w_popup_prop_win;
    return NULL;
}

// Per-popup clip geometry derived from w_popup_{top,bottom}off and
// w_popup_{left,right}clip.  Filled by popup_compute_clip().
//
//   *_extra        : original border+padding at each edge.
//   clip_*_content : how many *content* rows/cols are clipped at each edge
//                    (border/padding is consumed first; the rest comes off
//                    w_height/w_width).  >= 0.
//   eff_*_extra    : 0 when that edge is clipped (border+padding gone),
//                    otherwise the original *_extra.
//   eff_border[],
//   eff_padding[]  : per-edge border/padding sizes (indexed [top,right,bot,left]
//                    matching wp->w_popup_border / wp->w_popup_padding).  At a
//                    clipped edge they collapse to 0; elsewhere they keep the
//                    original size.  Drawing code can replace
//                    `wp->w_popup_border[N] > 0 && wp->w_popup_*clip == 0`
//                    with a single `cl.eff_border[N] > 0` test.
//   eff_height     : drawn extent = eff_top_extra + visible content + eff_bot_extra.
//   eff_width      : drawn extent = eff_left_extra + visible content + eff_right_extra
//                    (does NOT include w_leftcol or scrollbar; see callers).
typedef struct {
    int top_extra;
    int bot_extra;
    int left_extra;
    int right_extra;

    int clip_top_content;
    int clip_bot_content;
    int clip_left_content;
    int clip_right_content;

    int eff_top_extra;
    int eff_bot_extra;
    int eff_left_extra;
    int eff_right_extra;

    int eff_border[4];
    int eff_padding[4];

    int eff_height;
    int eff_width;
} popup_clip_T;

    static void
popup_compute_clip(win_T *wp, popup_clip_T *cl)
{
    int h, w;

    cl->top_extra = popup_top_extra(wp);
    cl->bot_extra = wp->w_popup_padding[2] + wp->w_popup_border[2];
    cl->left_extra = wp->w_popup_border[3] + wp->w_popup_padding[3];
    cl->right_extra = wp->w_popup_border[1] + wp->w_popup_padding[1];

    cl->clip_top_content = wp->w_popup_topoff - cl->top_extra;
    if (cl->clip_top_content < 0)
	cl->clip_top_content = 0;
    cl->clip_bot_content = wp->w_popup_bottomoff - cl->bot_extra;
    if (cl->clip_bot_content < 0)
	cl->clip_bot_content = 0;
    cl->clip_left_content = wp->w_popup_leftclip - cl->left_extra;
    if (cl->clip_left_content < 0)
	cl->clip_left_content = 0;
    cl->clip_right_content = wp->w_popup_rightclip - cl->right_extra;
    if (cl->clip_right_content < 0)
	cl->clip_right_content = 0;

    cl->eff_top_extra = wp->w_popup_topoff > 0 ? 0 : cl->top_extra;
    cl->eff_bot_extra = wp->w_popup_bottomoff > 0 ? 0 : cl->bot_extra;
    cl->eff_left_extra = wp->w_popup_leftclip > 0 ? 0 : cl->left_extra;
    cl->eff_right_extra = wp->w_popup_rightclip > 0 ? 0 : cl->right_extra;

    cl->eff_border[0] = wp->w_popup_topoff > 0 ? 0 : wp->w_popup_border[0];
    cl->eff_border[1] = wp->w_popup_rightclip > 0 ? 0 : wp->w_popup_border[1];
    cl->eff_border[2] = wp->w_popup_bottomoff > 0 ? 0 : wp->w_popup_border[2];
    cl->eff_border[3] = wp->w_popup_leftclip > 0 ? 0 : wp->w_popup_border[3];

    cl->eff_padding[0] = wp->w_popup_topoff > 0 ? 0 : wp->w_popup_padding[0];
    cl->eff_padding[1] = wp->w_popup_rightclip > 0 ? 0 : wp->w_popup_padding[1];
    cl->eff_padding[2] = wp->w_popup_bottomoff > 0 ? 0 : wp->w_popup_padding[2];
    cl->eff_padding[3] = wp->w_popup_leftclip > 0 ? 0 : wp->w_popup_padding[3];

    h = wp->w_height - cl->clip_top_content - cl->clip_bot_content;
    if (h < 0)
	h = 0;
    cl->eff_height = cl->eff_top_extra + h + cl->eff_bot_extra;

    w = wp->w_width - cl->clip_left_content - cl->clip_right_content;
    if (w < 0)
	w = 0;
    cl->eff_width = cl->eff_left_extra + w + cl->eff_right_extra;
}

// Snapshot of the popup window geometry that update_popups() temporarily
// mutates so that win_update() draws within the host-window clip rectangle.
// Saved before the clip is applied, restored after win_update() returns so
// callers continue to see the popup's logical geometry.
// Field names omit the "w_" prefix to avoid clashing with struct-field
// macros like w_p_wrap (= w_onebuf_opt.wo_wrap).
typedef struct {
    int		height;
    int		width;
    int		winrow;
    int		wincol;
    int		leftcol;
    int		p_wrap;
    linenr_T	topline;
} popup_geom_save_T;

    static void
popup_geom_save(win_T *wp, popup_geom_save_T *sv)
{
    sv->height  = wp->w_height;
    sv->width   = wp->w_width;
    sv->winrow  = wp->w_winrow;
    sv->wincol  = wp->w_wincol;
    sv->leftcol = wp->w_leftcol;
    sv->p_wrap  = wp->w_p_wrap;
    sv->topline = wp->w_topline;
}

    static void
popup_geom_restore(win_T *wp, popup_geom_save_T *sv)
{
    wp->w_p_wrap  = sv->p_wrap;
    wp->w_leftcol = sv->leftcol;
    wp->w_wincol  = sv->wincol;
    wp->w_winrow  = sv->winrow;
    wp->w_topline = sv->topline;
    wp->w_width   = sv->width;
    wp->w_height  = sv->height;
}

/*
 * Compute a screen row for a textprop that has scrolled above the host
 * window's top.  textpos2screenpos() cannot return a row above topline, so
 * compute the virtual column directly from the prop's *own* line and then
 * extrapolate a (possibly-negative) row by counting how many buffer lines
 * lie between the prop and topline.  The popup_topoff clip path turns the
 * negative row into a top-clip animation as the prop rolls off the top edge.
 *
 * Probing at topline with the prop's tp_col would inherit topline's tab
 * stops / multi-byte widths, so the popup's wincol would jitter every time
 * a wider/narrower line scrolled into the topmost position.
 */
    static void
popup_screenpos_above_top(
	win_T	    *prop_win,
	pos_T	    *pos,
	linenr_T    prop_lnum,
	int	    *screen_row,
	int	    *screen_scol,
	int	    *screen_ccol,
	int	    *screen_ecol)
{
    pos_T   probe = *pos;
    colnr_T scol = 0, ccol = 0, ecol = 0;
    int	    coloff;

    probe.lnum = prop_lnum;
    getvcol(prop_win, &probe, &scol, &ccol, &ecol, 0);
    coloff = (int)win_col_off(prop_win) - (int)prop_win->w_leftcol
					+ prop_win->w_wincol + 1;
    *screen_scol = (int)scol + coloff;
    *screen_ccol = (int)ccol + coloff;
    *screen_ecol = (int)ecol + coloff;
    *screen_row = prop_win->w_winrow + 1
				 - (int)(prop_win->w_topline - prop_lnum);
}

/*
 * Hide popup "wp" because its anchoring textprop is no longer reachable.
 * Marks the popup as POPF_HIDDEN (no-op when already hidden) and schedules a
 * redraw of the host window so any leftover decorations are cleared.
 */
    static void
popup_hide_for_textprop(win_T *wp)
{
    if ((wp->w_popup_flags & POPF_HIDDEN) != 0)
	return;
    wp->w_popup_flags |= POPF_HIDDEN;
    if (win_valid(wp->w_popup_prop_win))
	redraw_win_later(wp->w_popup_prop_win, UPD_SOME_VALID);
}

/*
 * For "clipwindow" popups: search the lines above prop_win->w_topline for the
 * popup's anchoring textprop and report whether one was found.  When
 * "max_reach" is > 0, only the last "max_reach" lines before topline are
 * scanned; pass 0 to scan all lines from line 1.  Returns false when the
 * popup is not "clipwindow", topline is already at line 1, or no prop matches.
 */
    static bool
popup_find_prop_above_top(
	win_T	    *wp,
	win_T	    *prop_win,
	int	    max_reach,
	textprop_T  *prop,
	linenr_T    *found_lnum)
{
    linenr_T	first;

    if (!(wp->w_popup_flags & POPF_CLIPWINDOW) || prop_win->w_topline <= 1)
	return false;

    first = max_reach > 0 ? prop_win->w_topline - max_reach : 1;
    if (first < 1)
	first = 1;
    return find_prop_in_lines(prop_win,
		    wp->w_popup_prop_type, wp->w_popup_prop_id,
		    prop, found_lnum, first, prop_win->w_topline - 1);
}

/*
 * Compute and assign w_popup_topoff/bottomoff/leftclip/rightclip from the
 * host (textprop) window's content rectangle when POPF_CLIPWINDOW is set.
 * The popup's logical geometry (w_winrow, w_height, w_width) is preserved;
 * only the *off/clip fields record how much of each edge falls outside.
 * Returns true when the popup has scrolled completely past one of the host
 * edges, in which case the caller must hide it.
 */
    static bool
popup_compute_clipwindow_offsets(win_T *wp)
{
    win_T   *cw = popup_get_clipwin(wp);
    int	    extra_h, extra_w;
    int	    popup_top, popup_bottom, popup_left, popup_right;
    int	    total_h, total_w;

    if (cw == NULL)
	return false;

    extra_h = popup_top_extra(wp)
		    + wp->w_popup_padding[2] + wp->w_popup_border[2];
    extra_w = popup_extra_width(wp);

    popup_top = wp->w_winrow;
    popup_bottom = wp->w_winrow + wp->w_height + extra_h;
    popup_left = wp->w_wincol;
    popup_right = wp->w_wincol + wp->w_width + extra_w;
    total_h = wp->w_height + extra_h;
    total_w = wp->w_width + extra_w;

    if (popup_top < cw->w_winrow)
	wp->w_popup_topoff = cw->w_winrow - popup_top;
    if (popup_bottom > cw->w_winrow + cw->w_height)
	wp->w_popup_bottomoff = popup_bottom - (cw->w_winrow + cw->w_height);
    if (popup_left < cw->w_wincol)
	wp->w_popup_leftclip = cw->w_wincol - popup_left;
    if (popup_right > cw->w_wincol + cw->w_width)
	wp->w_popup_rightclip = popup_right - (cw->w_wincol + cw->w_width);

    return wp->w_popup_topoff >= total_h
	|| wp->w_popup_bottomoff >= total_h
	|| wp->w_popup_leftclip >= total_w
	|| wp->w_popup_rightclip >= total_w;
}

/*
 * Mutate "wp"'s window geometry so win_update() draws only the rows/columns
 * that fit within the host-window clip rectangle for "clipwindow" popups.
 * The caller must save the original geometry with popup_geom_save() before
 * this call and restore it with popup_geom_restore() after win_update().
 *
 * Vertical clip: shrink w_height by the clipped content rows; advance
 * w_topline and w_winrow when rows are cut off the top so the first visible
 * content row lands on the host's top edge.
 *
 * Horizontal clip: when the right side is clipped, just shrink w_width.
 * When the left side is clipped, advance w_leftcol so the hidden buffer
 * columns scroll off and shift w_wincol so the first visible column lands on
 * the host's left edge.  Disable wrap so the transient w_width reduction does
 * not reflow wrapped lines: the popup's logical width is unchanged, we just
 * want to truncate cells that fall outside the host at draw time.
 */
    static void
popup_apply_winupdate_clip(win_T *wp, popup_clip_T *cl)
{
    if (wp->w_popup_topoff > 0 || wp->w_popup_bottomoff > 0)
    {
	wp->w_height -= cl->clip_top_content + cl->clip_bot_content;
	if (wp->w_height < 0)
	    wp->w_height = 0;
	if (cl->clip_top_content > 0)
	{
	    wp->w_topline += cl->clip_top_content;
	    wp->w_winrow += cl->clip_top_content;
	}
    }
    if (wp->w_popup_leftclip > 0 || wp->w_popup_rightclip > 0)
    {
	if (cl->clip_left_content > 0 || cl->clip_right_content > 0)
	    wp->w_p_wrap = FALSE;
	if (cl->clip_right_content > 0)
	{
	    wp->w_width -= cl->clip_right_content;
	    if (wp->w_width < 0)
		wp->w_width = 0;
	}
	if (cl->clip_left_content > 0)
	{
	    wp->w_leftcol += cl->clip_left_content;
	    wp->w_wincol += cl->clip_left_content;
	    wp->w_width -= cl->clip_left_content;
	    if (wp->w_width < 0)
		wp->w_width = 0;
	}
    }
}

/*
 * Adjust the position and size of the popup to fit on the screen.
 */
    static void
popup_adjust_position(win_T *wp)
{
    linenr_T	lnum;
    int		wrapped = 0;
    int		maxwidth;
    int		maxwidth_no_scrollbar;
    int		width_with_scrollbar = 0;
    int		used_maxwidth = FALSE;
    int		margin_width = 0;
    int		maxspace;
    int		center_vert = FALSE;
    int		center_hor = FALSE;
    int		allow_adjust_left = !wp->w_popup_fixed;
    int		top_extra = popup_top_extra(wp);
    int		right_extra = wp->w_popup_border[1] + wp->w_popup_padding[1]
		    + (wp->w_popup_shadow ? 2 : 0);
    int		bot_extra = wp->w_popup_border[2] + wp->w_popup_padding[2]
		    + wp->w_popup_shadow;
    int		left_extra = wp->w_popup_border[3] + wp->w_popup_padding[3];
    int		extra_height = top_extra + bot_extra;
    int		extra_width = left_extra + right_extra;
    int		w_height_before_limit;
    popup_layout_T org_layout;
    int		minwidth, minheight;
    int		maxheight = Rows;
    int		wantline = wp->w_wantline;  // adjusted for textprop
    int		wantcol = wp->w_wantcol;    // adjusted for textprop
    int		use_wantcol = wantcol != 0;
    int		adjust_height_for_top_aligned = FALSE;

    popup_save_layout(wp, &org_layout);

    wp->w_winrow = 0;
    wp->w_wincol = 0;
    wp->w_leftcol = 0;
    wp->w_popup_leftoff = 0;
    wp->w_popup_rightoff = 0;
    wp->w_popup_topoff = 0;
    wp->w_popup_bottomoff = 0;
    wp->w_popup_leftclip = 0;
    wp->w_popup_rightclip = 0;

    // May need to update the "cursorline" highlighting, which may also change
    // "topline"
    if (wp->w_popup_last_curline != wp->w_cursor.lnum)
	popup_highlight_curline(wp);

    if (wp->w_popup_prop_type > 0 && win_valid(wp->w_popup_prop_win))
    {
	win_T	    *prop_win = wp->w_popup_prop_win;
	textprop_T  prop;
	linenr_T    prop_lnum;
	pos_T	    pos;
	int	    screen_row;
	int	    screen_scol;
	int	    screen_ccol;
	int	    screen_ecol;

	// Popup window is positioned relative to a text property.  With
	// "clipwindow", keep the popup visible while the textprop has just
	// scrolled above the host's top: extrapolate a negative screen_row
	// from a prop above topline so the top-clip path can roll the popup
	// off the top edge.  Unhiding is done in check_popup_unhidden().
	bool prop_above_top = false;
	if (!find_visible_prop(prop_win,
				wp->w_popup_prop_type, wp->w_popup_prop_id,
				&prop, &prop_lnum))
	{
	    if (popup_find_prop_above_top(wp, prop_win, 0,
							    &prop, &prop_lnum))
		prop_above_top = true;
	    else
	    {
		popup_hide_for_textprop(wp);
		return;
	    }
	}

	// Compute the desired position from the position of the text
	// property.  Use "wantline" and "wantcol" as offsets.
	pos.lnum = prop_lnum;
	pos.col = prop.tp_col;
	if (wp->w_popup_pos == POPPOS_TOPLEFT
		|| wp->w_popup_pos == POPPOS_BOTLEFT)
	    pos.col += prop.tp_len - 1;
	if (prop_above_top)
	    popup_screenpos_above_top(prop_win, &pos, prop_lnum, &screen_row,
				     &screen_scol, &screen_ccol, &screen_ecol);
	else
	    textpos2screenpos(prop_win, &pos, &screen_row,
				     &screen_scol, &screen_ccol, &screen_ecol);

	if (screen_scol == 0)
	{
	    // position is off screen, make the width zero to hide it.
	    wp->w_width = 0;
	    return;
	}
	if (wp->w_popup_pos == POPPOS_TOPLEFT
		|| wp->w_popup_pos == POPPOS_TOPRIGHT)
	    // below the text
	    wantline = screen_row + wantline + 1;
	else
	    // above the text
	    wantline = screen_row + wantline - 1;
	center_vert = FALSE;
	if (wp->w_popup_pos == POPPOS_TOPLEFT
		|| wp->w_popup_pos == POPPOS_BOTLEFT)
	    // right of the text
	    wantcol = screen_ecol + wantcol;
	else
	    // left of the text
	    wantcol = screen_scol + wantcol - 2;
	use_wantcol = TRUE;
    }
    else
    {
	// If no line was specified default to vertical centering.
	if (wantline == 0)
	    center_vert = TRUE;
	else if (wantline < 0)
	    // If "wantline" is negative it actually means zero.
	    wantline = 0;
	if (wantcol < 0)
	    // If "wantcol" is negative it actually means zero.
	    wantcol = 0;
    }

    if (wp->w_popup_pos == POPPOS_CENTER)
    {
	// center after computing the size
	center_vert = TRUE;
	center_hor = TRUE;
    }
    else
    {
	if (wantline > 0 && (wp->w_popup_pos == POPPOS_TOPLEFT
					|| wp->w_popup_pos == POPPOS_TOPRIGHT))
	{
	    wp->w_winrow = wantline - 1;
	    if (wp->w_winrow >= Rows)
		wp->w_winrow = Rows - 1;
	}
	if (wp->w_popup_pos == POPPOS_BOTTOM)
	{
	    // Assume that each buffer line takes one screen line, and one line
	    // for the top border.  First make sure cmdline_row is valid,
	    // calling update_screen() will set it only later.
	    compute_cmdrow();
	    wp->w_winrow = MAX(cmdline_row
				    - wp->w_buffer->b_ml.ml_line_count - 1, 0);
	}

	if (!use_wantcol)
	    center_hor = TRUE;
	else if (wantcol > 0 && (wp->w_popup_pos == POPPOS_TOPLEFT
		|| wp->w_popup_pos == POPPOS_BOTLEFT))
	{
	    wp->w_wincol = wantcol - 1;
	    // Need to see at least one character of content plus the right
	    // border/padding/shadow after the decoration.
	    if (wp->w_wincol > firstwin->w_wincol + topframe->fr_width
						- left_extra - right_extra - 1)
		wp->w_wincol = firstwin->w_wincol + topframe->fr_width
						- left_extra - right_extra - 1;
	}
    }

    // Keep the popup out of the tabpanel area so the available width is
    // computed correctly below.
    if (wp->w_wincol < firstwin->w_wincol)
	wp->w_wincol = firstwin->w_wincol;

    // When centering or right aligned, use maximum width.
    // When left aligned use the space available, but shift to the left when we
    // hit the right of the screen.
    // Reserve room for the right border/padding/shadow so the popup fits.
    maxspace = firstwin->w_wincol + topframe->fr_width
					- wp->w_wincol - left_extra - right_extra;
    maxwidth = maxspace;
    if (wp->w_maxwidth > 0 && maxwidth > wp->w_maxwidth)
    {
	allow_adjust_left = FALSE;
	maxwidth = wp->w_maxwidth;
    }

    if (wp->w_p_nu || wp->w_p_rnu)
	margin_width = number_width(wp) + 1;
#ifdef FEAT_FOLDING
    margin_width += wp->w_p_fdc;
#endif
#ifdef FEAT_SIGNS
    if (signcolumn_on(wp))
	margin_width += 2;
#endif
    if (margin_width >= maxwidth)
	margin_width = maxwidth - 1;

    minwidth = wp->w_minwidth;
    minheight = wp->w_minheight;
#ifdef FEAT_TERMINAL
    // A terminal popup initially does not have content, use a default minimal
    // width of 20 characters and height of 5 lines.
    if (wp->w_buffer->b_term != NULL)
    {
	if (minwidth == 0)
	    minwidth = 20;
	if (minheight == 0)
	    minheight = 5;
    }
#endif

    if (wp->w_maxheight > 0)
	maxheight = wp->w_maxheight;
    else if (wp->w_popup_pos == POPPOS_BOTTOM)
	maxheight = cmdline_row - 1;

    // start at the desired first line
    if (wp->w_firstline > 0)
    {
	// If firstline is beyond the buffer content, reset it to auto-position.
	// This can happen when the popup was scrolled and then the buffer
	// content was changed to have fewer lines.
	if (wp->w_firstline > wp->w_buffer->b_ml.ml_line_count)
	    wp->w_firstline = 0;
	else
	    wp->w_topline = wp->w_firstline;
    }
    if (wp->w_topline < 1)
	wp->w_topline = 1;
    else if (wp->w_topline > wp->w_buffer->b_ml.ml_line_count)
	wp->w_topline = wp->w_buffer->b_ml.ml_line_count;

    // Compute width based on longest text line and the 'wrap' option.
    // Use a minimum width of one, so that something shows when there is no
    // text.
    // When "firstline" is -1 then start with the last buffer line and go
    // backwards.
    // TODO: more accurate wrapping
    wp->w_width = 1;
    // Pre-scan every buffer line to find the widest one, so the popup width
    // stays stable when scrolling changes which lines are visible.
    {
	linenr_T ln;
	int saved_w_width = wp->w_width;

	if (wp->w_width < maxwidth)
	    wp->w_width = maxwidth;
	for (ln = 1; ln <= wp->w_buffer->b_ml.ml_line_count; ++ln)
	{
	    int len = linetabsize(wp, ln) + margin_width;

	    if (wp->w_maxwidth > 0 && len > wp->w_maxwidth)
		len = wp->w_maxwidth;
	    if (saved_w_width < len)
		saved_w_width = len;
	    if (wp->w_maxwidth > 0 && saved_w_width >= wp->w_maxwidth)
		break;
	}
	wp->w_width = saved_w_width;
    }
    if (wp->w_firstline < 0)
	lnum = wp->w_buffer->b_ml.ml_line_count;
    else
	lnum = wp->w_topline;
    while (lnum >= 1 && lnum <= wp->w_buffer->b_ml.ml_line_count)
    {
	int len;
	int w_width = wp->w_width;

	// Count Tabs for what they are worth and compute the length based on
	// the maximum width (matters when 'showbreak' is set).
	// "margin_width" is added to "len" where it matters.
	if (wp->w_width < maxwidth)
	    wp->w_width = maxwidth;
	len = linetabsize(wp, lnum);
	wp->w_width = w_width;

	if (len + margin_width > maxwidth
		&& allow_adjust_left
		&& (wp->w_popup_pos == POPPOS_TOPLEFT
		    || wp->w_popup_pos == POPPOS_BOTLEFT))
	{
	    // adjust leftwise to fit text on screen
	    int shift_by = len + margin_width - maxwidth;

	    if (shift_by > wp->w_wincol)
	    {
		int truncate_shift = shift_by - wp->w_wincol;

		shift_by -= truncate_shift;
	    }

	    // When wrapping is enabled and maxwidth is explicitly set,
	    // don't shift beyond maxwidth - let the text wrap instead.
	    if (wp->w_p_wrap && wp->w_maxwidth > 0
				    && maxwidth + shift_by > wp->w_maxwidth)
		shift_by = wp->w_maxwidth - maxwidth;

	    if (shift_by > 0)
	    {
		wp->w_wincol -= shift_by;
		maxwidth += shift_by;
	    }
	    wp->w_width = maxwidth;
	}
	if (wp->w_p_wrap)
	{
	    while (len + margin_width > maxwidth)
	    {
		++wrapped;
		len -= maxwidth - margin_width;
		wp->w_width = maxwidth;
		used_maxwidth = TRUE;
	    }
	}
	if (wp->w_width < len + margin_width)
	{
	    wp->w_width = len + margin_width;
	    if (wp->w_maxwidth > 0 && wp->w_width > wp->w_maxwidth)
		wp->w_width = wp->w_maxwidth;
	}

	if (wp->w_firstline < 0)
	    --lnum;
	else
	    ++lnum;

	// do not use the width of lines we're not going to show
	if (maxheight > 0
		   && (wp->w_firstline >= 0
			       ? lnum - wp->w_topline
			       : wp->w_buffer->b_ml.ml_line_count - lnum)
		       + wrapped >= maxheight)
	    break;
    }

    if (wp->w_firstline < 0)
	wp->w_topline = lnum + 1;

    wp->w_has_scrollbar = wp->w_want_scrollbar
	   && (wp->w_topline > 1 || lnum <= wp->w_buffer->b_ml.ml_line_count);
#ifdef FEAT_TERMINAL
    if (wp->w_buffer->b_term != NULL && !term_is_finished(wp->w_buffer))
	// Terminal window with running job never has a scrollbar, adjusts to
	// window height.
	wp->w_has_scrollbar = false;
#endif
    maxwidth_no_scrollbar = maxwidth;
    if (wp->w_has_scrollbar)
    {
	++right_extra;
	++extra_width;
	// make space for the scrollbar if needed, when lines wrap and when
	// applying minwidth
	if (maxwidth + right_extra >= maxspace
		&& (used_maxwidth || (minwidth > 0 && wp->w_width < minwidth)))
	    maxwidth -= wp->w_popup_padding[1] + 1;
    }

    if (wp->w_popup_title != NULL && *wp->w_popup_title != NUL)
    {
	int title_len = vim_strsize(wp->w_popup_title) + 2 - extra_width;

	if (minwidth < title_len)
	    minwidth = title_len;
    }

    if (minwidth > 0 && wp->w_width < minwidth)
	wp->w_width = minwidth;
    if (wp->w_width > maxwidth)
    {
	if (wp->w_width > maxspace && !wp->w_p_wrap)
	    // some columns cut off on the right
	    wp->w_popup_rightoff = wp->w_width - maxspace;

	// If the window doesn't fit because 'minwidth' is set then the
	// scrollbar is at the far right of the screen, use the size without
	// the scrollbar.
	if (wp->w_has_scrollbar && wp->w_minwidth > 0)
	{
	    int off = wp->w_width - maxwidth;
	    extra_width -= MIN(off, right_extra);
	    wp->w_width = maxwidth_no_scrollbar;
	}
	else
	{
	    wp->w_width = maxwidth;

	    // when adding a scrollbar below need to adjust the width
	    width_with_scrollbar = maxwidth_no_scrollbar - right_extra;
	}
    }
    if (center_hor)
    {
	wp->w_wincol = firstwin->w_wincol
		    + (topframe->fr_width - wp->w_width - extra_width) / 2;
	if (wp->w_wincol < firstwin->w_wincol)
	    wp->w_wincol = firstwin->w_wincol;
    }
    else if (wp->w_popup_pos == POPPOS_BOTRIGHT
	    || wp->w_popup_pos == POPPOS_TOPRIGHT)
    {
	int leftoff = wantcol - (wp->w_width + extra_width);

	// Right aligned: move to the right if needed.
	// No truncation, because that would change the height.
	if (leftoff >= 0)
	    wp->w_wincol = leftoff;
	else if (wp->w_popup_fixed)
	{
	    // "col" specifies the right edge, but popup doesn't fit, skip some
	    // columns when displaying the window, minus left border and
	    // padding.
	    if (-leftoff > left_extra)
		wp->w_leftcol = -leftoff - left_extra;
	    wp->w_width -= wp->w_leftcol;
	    wp->w_popup_leftoff = -leftoff;
	    if (wp->w_width < 0)
		wp->w_width = 0;
	}
    }

    if (wp->w_p_wrap || (!wp->w_popup_fixed
			    && (wp->w_popup_pos == POPPOS_TOPLEFT
				|| wp->w_popup_pos == POPPOS_BOTLEFT)))
    {
	int want_col = 0;

	// try to show the right border and any scrollbar
	want_col = left_extra + wp->w_width + right_extra;
	if (want_col > 0 && wp->w_wincol > 0
					 && wp->w_wincol + want_col >= firstwin->w_wincol + topframe->fr_width)
	{
	    wp->w_wincol = firstwin->w_wincol + topframe->fr_width - want_col;
	    if (wp->w_wincol < 0)
		wp->w_wincol = 0;
	}
    }

    wp->w_height = wp->w_buffer->b_ml.ml_line_count - wp->w_topline
								 + 1 + wrapped;
    if (minheight > 0 && wp->w_height < minheight)
	wp->w_height = minheight;
    if (maxheight > 0 && wp->w_height > maxheight)
	wp->w_height = maxheight;
    w_height_before_limit = wp->w_height;
    if (wp->w_height > Rows - wp->w_winrow)
	wp->w_height = Rows - wp->w_winrow;

    if (center_vert)
    {
	wp->w_winrow = (Rows - wp->w_height - extra_height) / 2;
	if (wp->w_winrow < 0)
	    wp->w_winrow = 0;
    }
    else if (wp->w_popup_pos == POPPOS_BOTRIGHT
		|| wp->w_popup_pos == POPPOS_BOTLEFT)
    {
	if ((wp->w_height + extra_height) <= wantline
		|| (wp->w_popup_flags & POPF_CLIPWINDOW))
	    // bottom aligned: may move down.  With "clipwindow" the popup
	    // keeps its natural position even if it overflows the screen,
	    // because the clip logic handles the overflow.
	    wp->w_winrow = wantline - (wp->w_height + extra_height);
	else if (wantline * 2 >= Rows || !(wp->w_popup_flags & POPF_POSINVERT))
	{
	    // Bottom aligned but does not fit, and less space on the other
	    // side or "posinvert" is off: reduce height.
	    wp->w_winrow = 0;
	    wp->w_height = wantline - extra_height;
	}
	else
	{
	    // Not enough space and more space on the other side: make top
	    // aligned.
	    wp->w_winrow = (wantline < 0 ? 0 : wantline) + 1;
	    adjust_height_for_top_aligned = TRUE;
	}
    }
    else if (wp->w_popup_pos == POPPOS_TOPRIGHT
		|| wp->w_popup_pos == POPPOS_TOPLEFT)
    {

	int check_height = (wp->w_popup_flags & POPF_INFO) ? wp->w_height
						    : w_height_before_limit;
	if (wp != popup_dragwin
		&& wantline + (check_height + extra_height) - 1 > Rows
		&& wantline * 2 > Rows
		&& (wp->w_popup_flags & POPF_POSINVERT))
	{
	    // top aligned and not enough space below but there is space above:
	    // make bottom aligned and recompute the height
	    wp->w_height = w_height_before_limit;
	    wp->w_winrow = wantline - 2 - wp->w_height - extra_height;
	    if (wp->w_popup_flags & POPF_INFO)
		wp->w_winrow += extra_height + 2;
	    if (wp->w_winrow < 0)
	    {
		wp->w_height += wp->w_winrow;
		wp->w_winrow = 0;
	    }
	}
	else
	{
	    wp->w_winrow = wantline - 1;
	    adjust_height_for_top_aligned = TRUE;
	}
    }

    if (adjust_height_for_top_aligned && wp->w_want_scrollbar
			  && wp->w_winrow + wp->w_height + extra_height > Rows)
    {
	// Bottom of the popup goes below the last line, reduce the height and
	// add a scrollbar.
	wp->w_height = Rows - wp->w_winrow - extra_height;
#ifdef FEAT_TERMINAL
	if (wp->w_buffer->b_term == NULL || term_is_finished(wp->w_buffer))
#endif
	{
	    wp->w_has_scrollbar = true;
	    if (width_with_scrollbar > 0)
		wp->w_width = width_with_scrollbar;
	}
    }

    // make sure w_winrow is valid
    if (wp->w_winrow >= Rows)
	wp->w_winrow = Rows - 1;
    else if (wp->w_winrow < 0 && !(wp->w_popup_flags & POPF_CLIPWINDOW))
	wp->w_winrow = 0;

    if (wp->w_wincol + wp->w_width + extra_width
				    > firstwin->w_wincol + topframe->fr_width)
	wp->w_wincol = firstwin->w_wincol + topframe->fr_width
						- wp->w_width - extra_width;
    if (wp->w_wincol < firstwin->w_wincol)
	wp->w_wincol = firstwin->w_wincol;
    if (wp->w_wincol < 0)
	wp->w_wincol = 0;
    // If the popup is wider than the available area (e.g. minwidth larger than
    // the work area between tabpanels), clip the content width so the right
    // border/padding/shadow stays visible instead of being pushed off the
    // screen or into the tabpanel.
    if (wp->w_wincol + wp->w_width + extra_width
				    > firstwin->w_wincol + topframe->fr_width)
    {
	int avail = firstwin->w_wincol + topframe->fr_width
						- wp->w_wincol - extra_width;
	wp->w_width = avail > 0 ? avail : 0;
    }

    // Same for the bottom edge: shift up so the border/padding/shadow stays
    // on screen, and clip the height if the popup is taller than the screen.
    // For "clipwindow" popups the host-window clip below handles overflow, so
    // skip these screen-edge clamps -- otherwise a synthesised negative
    // w_winrow (popup partially above the host's top edge) would be snapped
    // back to 0 and defeat the top-clip animation.
    if (!(wp->w_popup_flags & POPF_CLIPWINDOW))
    {
	if (wp->w_winrow + wp->w_height + extra_height > Rows)
	    wp->w_winrow = Rows - wp->w_height - extra_height;
	if (wp->w_winrow < 0)
	    wp->w_winrow = 0;
	if (wp->w_winrow + wp->w_height + extra_height > Rows)
	{
	    int avail = Rows - wp->w_winrow - extra_height;
	    wp->w_height = avail > 0 ? avail : 0;
	}
    }

    if (wp->w_height != org_layout.height)
	win_comp_scroll(wp);

    // Confine the popup to its host window for "clipwindow".  The popup's
    // logical geometry stays untouched; only w_popup_topoff/bottomoff/
    // leftclip/rightclip record how many rows/columns of each edge fall
    // outside the host so the drawing code can skip them.  When the popup
    // has fully scrolled past one of the host edges, hide it instead of
    // leaving stray decorations behind.
    if (popup_compute_clipwindow_offsets(wp))
    {
	popup_hide_for_textprop(wp);
	return;
    }

    wp->w_popup_last_changedtick = CHANGEDTICK(wp->w_buffer);
    if (win_valid(wp->w_popup_prop_win))
    {
	wp->w_popup_prop_changedtick =
				   CHANGEDTICK(wp->w_popup_prop_win->w_buffer);
	wp->w_popup_prop_topline = wp->w_popup_prop_win->w_topline;
	wp->w_popup_prop_winrow = wp->w_popup_prop_win->w_winrow;
	wp->w_popup_prop_wincol = wp->w_popup_prop_win->w_wincol;
	wp->w_popup_prop_width = wp->w_popup_prop_win->w_width;
	wp->w_popup_prop_winheight = wp->w_popup_prop_win->w_height;
    }

    // Need to update popup_mask if the position or size changed.
    // And redraw windows and statuslines that were behind the popup.
    if (popup_layout_changed(wp, &org_layout))
    {
	redraw_win_later(wp, UPD_NOT_VALID);
	if (wp->w_popup_flags & POPF_ON_CMDLINE)
	    clear_cmdline = TRUE;
	popup_mask_refresh = TRUE;
    }
}

typedef enum
{
    TYPE_NORMAL,
    TYPE_ATCURSOR,
    TYPE_BEVAL,
    TYPE_NOTIFICATION,
    TYPE_MESSAGE_WIN,	// similar to TYPE_NOTIFICATION
    TYPE_DIALOG,
    TYPE_MENU,
    TYPE_PREVIEW,	// preview window
    TYPE_INFO		// popup menu info
} create_type_T;

/*
 * Return TRUE if "type" is TYPE_NOTIFICATION or TYPE_MESSAGE_WIN.
 */
    static int
popup_is_notification(create_type_T type)
{
    return type == TYPE_NOTIFICATION || type == TYPE_MESSAGE_WIN;
}

/*
 * Make "buf" empty and set the contents to "text".
 * Used by popup_create() and popup_settext().
 */
    static void
popup_set_buffer_text(buf_T *buf, typval_T text)
{
    int	    lnum;

    // Clear the buffer, then replace the lines.
    curbuf = buf;
    for (lnum = buf->b_ml.ml_line_count; lnum > 0; --lnum)
	ml_delete(lnum);
    curbuf = curwin->w_buffer;

    // Add text to the buffer.
    if (text.v_type == VAR_STRING)
    {
	char_u *s = text.vval.v_string;

	// just a string
	ml_append_buf(buf, 0, s == NULL ? (char_u *)"" : s, (colnr_T)0, TRUE);
    }
    else
    {
	list_T *l = text.vval.v_list;

	if (l != NULL && l->lv_len > 0)
	{
	    if (l->lv_first == &range_list_item)
		emsg(_(e_using_number_as_string));
	    else if (l->lv_first->li_tv.v_type == VAR_STRING)
		// list of strings
		add_popup_strings(buf, l);
	    else
		// list of dictionaries
		add_popup_dicts(buf, l);
	}
    }

    // delete the line that was in the empty buffer
    curbuf = buf;
    ml_delete(buf->b_ml.ml_line_count);
    curbuf = curwin->w_buffer;
}

#define SET_BORDER_CHARS(a0, a1, a2, a3, a4, a5, a6, a7)    \
    do {						    \
	if (wp != NULL)					    \
	{						    \
	    wp->w_border_char[0] = (a0);		    \
	    wp->w_border_char[1] = (a1);		    \
	    wp->w_border_char[2] = (a2);		    \
	    wp->w_border_char[3] = (a3);		    \
	    wp->w_border_char[4] = (a4);		    \
	    wp->w_border_char[5] = (a5);		    \
	    wp->w_border_char[6] = (a6);		    \
	    wp->w_border_char[7] = (a7);		    \
	}						    \
    } while (0)

/*
 * Parse the 'previewpopup' or 'completepopup' option and apply the values to
 * window "wp" if it is not NULL.
 * Return FAIL if the parsing fails.
 */
    static int
parse_popup_option(win_T *wp, int is_preview)
{
    char_u *p =
#ifdef FEAT_QUICKFIX
	!is_preview ? p_cpp :
#endif
	p_pvp;
    int	    border_enabled = FALSE;

    if (wp != NULL)
	wp->w_popup_flags &= ~POPF_INFO_MENU;

    for ( ; *p != NUL; p += (*p == ',' ? 1 : 0))
    {
	char_u	*e, *dig;
	char_u	*s = p;
	int	x;

	e = vim_strchr(p, ':');
	if (e == NULL || e[1] == NUL)
	    return FAIL;

	p = vim_strchr(e, ',');
	if (p == NULL)
	    p = e + STRLEN(e);
	dig = e + 1;
	x = getdigits(&dig);

	// Note: Keep this in sync with p_popup_option_values.
	if (STRNCMP(s, "height:", 7) == 0)
	{
	    if (dig != p)
		return FAIL;
	    if (wp != NULL)
	    {
		if (is_preview)
		    wp->w_minheight = x;
		wp->w_maxheight = x;
	    }
	}
	else if (STRNCMP(s, "width:", 6) == 0)
	{
	    if (dig != p)
		return FAIL;
	    if (wp != NULL)
	    {
		if (is_preview)
		    wp->w_minwidth = x;
		wp->w_maxwidth = x;
		wp->w_maxwidth_opt = x;
	    }
	}
	else if (STRNCMP(s, "highlight:", 10) == 0)
	{
	    if (wp != NULL)
	    {
		char *errmsg;
		int c = *p;

		*p = NUL;

		errmsg = update_wincolor(wp, s + 10);
		if (errmsg == NULL)
		    set_string_option_direct_in_win(wp, (char_u *)"wincolor",
			    -1, s + 10, OPT_FREE|OPT_LOCAL, 0);
		else
		    emsg(_(errmsg));

		*p = c;
	    }
	}
	else if (STRNCMP(s, "borderhighlight:", 16) == 0)
	{
	    char_u	*arg = s + 16;

	    if (*arg == NUL || *arg == ',')
		return FAIL;
	    if (wp != NULL)
	    {
		for (int i = 0; i < 4; ++i)
		{
		    VIM_CLEAR(wp->w_border_highlight[i]);
		    wp->w_border_highlight[i] = vim_strnsave(arg, p - arg);
		}
	    }
	}
	else if (STRNCMP(s, "border:", 7) == 0)
	{
	    char_u	*arg = s + 7;
	    int		i;
	    int		token_len = p - arg;
	    char_u	*token;
	    // Use box-drawing characters only when 'encoding' is "utf-8" and
	    // 'ambiwidth' is "single".
	    int		can_use_box_chars = (enc_utf8 && *p_ambw == 's');

	    if (token_len == 0
			|| (STRNCMP(arg, "off", 3) == 0 && arg + 3 == p))
	    {
		if (wp != NULL)
		{
		    for (i = 0; i < 4; ++i)
			wp->w_popup_border[i] = 0;
		    SET_BORDER_CHARS(0, 0, 0, 0, 0, 0, 0, 0);
		    // only show the X for close when there is a border
		    wp->w_popup_close = POPCLOSE_NONE;
		}
		continue;
	    }

	    token = vim_strnsave(arg, token_len);
	    if (token == NULL)
		return FAIL;

	    if ((can_use_box_chars && (STRCMP(token, "single") == 0
			    || STRCMP(token, "double") == 0
			    || STRCMP(token, "on") == 0
			    || STRCMP(token, "round") == 0))
		    || STRCMP(token, "ascii") == 0
		    || (STRNCMP(token, "custom:", 7) == 0))
	    {
		if (STRCMP(token, "single") == 0)
		    SET_BORDER_CHARS(0x2500, 0x2502, 0x2500, 0x2502, // ─ │ ─ │
			    0x250c, 0x2510, 0x2518, 0x2514); // ┌ ┐ ┘ └
		else if (STRCMP(token, "double") == 0)
		    SET_BORDER_CHARS(0x2550, 0x2551, 0x2550, 0x2551, // ═ ║ ═ ║
			    0x2554, 0x2557, 0x255D, 0x255A); // ╔ ╗ ╝  ╚
		else if (STRCMP(token, "round") == 0)
		    SET_BORDER_CHARS(0x2500, 0x2502, 0x2500, 0x2502, // ─ │ ─ │
			    0x256d, 0x256e, 0x256f, 0x2570); // ╭ ╮ ╯ ╰
		else if (STRCMP(token, "on") == 0)
		    SET_BORDER_CHARS(0, 0, 0, 0, 0, 0, 0, 0);
		else if (STRCMP(token, "ascii") == 0)
		    SET_BORDER_CHARS('-', '|', '-', '|', '+', '+', '+', '+');
		else if (STRNCMP(token, "custom:", 7) == 0)
		{
		    char_u	*q = token + 7;
		    int		out[8];
		    int		failed = FALSE;

		    SET_BORDER_CHARS(0, 0, 0, 0, 0, 0, 0, 0);

		    for (i = 0; i < 8 && !failed; i++)
		    {
			if (*q == NUL)
			    failed = TRUE;
			else
			{
			    out[i] = mb_ptr2char(q);
			    mb_ptr2char_adv(&q);
			    if (i < 7)
			    {
				if (*q != ';')
				    failed = TRUE; // must be semicolon
				q++;
			    }
			}
		    }
		    if (failed || *q != NUL) // must end exactly after the 8th char
		    {
			vim_free(token);
			return FAIL;
		    }
		    SET_BORDER_CHARS(out[0], out[1], out[2], out[3], out[4],
			    out[5], out[6], out[7]);
		}
	    }
	    else
	    {
		vim_free(token);
		return FAIL;
	    }

	    if (wp != NULL)
	    {
		for (i = 0; i < 4; ++i)
		    wp->w_popup_border[i] = 1;
	    }
	    border_enabled = TRUE;

	    vim_free(token);
	}
	else if (STRNCMP(s, "close:", 6) == 0)
	{
	    char_u	*arg = s + 6;
	    int		on = STRNCMP(arg, "on", 2) == 0 && arg + 2 == p;
	    int		off = STRNCMP(arg, "off", 3) == 0 && arg + 3 == p;

	    if (!on && !off)
		return FAIL;
	    on = on && mouse_has(MOUSE_INSERT) && (border_enabled || is_preview);
	    if (wp != NULL)
		wp->w_popup_close = on ? POPCLOSE_BUTTON : POPCLOSE_NONE;
	}
	else if (STRNCMP(s, "resize:", 7) == 0)
	{
	    char_u	*arg = s + 7;
	    int		on = STRNCMP(arg, "on", 2) == 0 && arg + 2 == p;
	    int		off = STRNCMP(arg, "off", 3) == 0 && arg + 3 == p;

	    if (!on && !off)
		return FAIL;
	    if (wp != NULL)
	    {
		if (on && mouse_has(MOUSE_INSERT))
		    wp->w_popup_flags |= POPF_RESIZE;
		else
		    wp->w_popup_flags &= ~POPF_RESIZE;
	    }
	}
	else if (STRNCMP(s, "shadow:", 7) == 0)
	{
	    char_u	*arg = s + 7;
	    int		on = STRNCMP(arg, "on", 2) == 0 && arg + 2 == p;
	    int		off = STRNCMP(arg, "off", 3) == 0 && arg + 3 == p;

	    if (!on && !off)
		return FAIL;
	    if (wp != NULL)
		wp->w_popup_shadow = on ? 1 : 0;
	}
	else if (STRNCMP(s, "align:", 6) == 0)
	{
	    // Note: Keep this in sync with p_popup_option_align_values.
	    char_u	*arg = s + 6;
	    int		item = STRNCMP(arg, "item", 4) == 0 && arg + 4 == p;
	    int		menu = STRNCMP(arg, "menu", 4) == 0 && arg + 4 == p;

	    if (!menu && !item)
		return FAIL;
	    if (wp != NULL && menu)
		wp->w_popup_flags |= POPF_INFO_MENU;
	}
	else if (STRNCMP(s, "opacity:", 8) == 0)
	{
	    if (dig != p || x < 0 || x > 100)
		return FAIL;
	    if (wp != NULL)
	    {
		if (x < 100)
		    wp->w_popup_flags |= POPF_OPACITY;
		else
		    wp->w_popup_flags &= ~POPF_OPACITY;
		wp->w_popup_blend = 100 - x;
	    }
	}
	else
	    return FAIL;
    }
    return OK;
}

/*
 * Parse the 'previewpopup' option and apply the values to window "wp" if it
 * is not NULL.
 * Return FAIL if the parsing fails.
 */
    int
parse_previewpopup(win_T *wp)
{
    return parse_popup_option(wp, TRUE);
}

/*
 * Parse the 'completepopup' option and apply the values to window "wp" if it
 * is not NULL.
 * Return FAIL if the parsing fails.
 */
    int
parse_completepopup(win_T *wp)
{
    return parse_popup_option(wp, FALSE);
}

/*
 * Set w_wantline and w_wantcol for the cursor position in the current window.
 * Keep at least "width" columns from the right of the screen.
 */
    void
popup_set_wantpos_cursor(win_T *wp, int width, dict_T *d)
{
    poppos_T ppt = POPPOS_NONE;

    if (d != NULL)
	ppt = get_pos_entry(d, FALSE);

    setcursor_mayforce(TRUE);
    if (ppt == POPPOS_TOPRIGHT || ppt == POPPOS_TOPLEFT)
    {
	wp->w_wantline = curwin->w_winrow + curwin->w_wrow + 2;
    }
    else
    {
	wp->w_wantline = curwin->w_winrow + curwin->w_wrow;
	if (wp->w_wantline == 0)  // cursor in first line
	{
	    wp->w_wantline = 2;
	    wp->w_popup_pos = ppt == POPPOS_BOTRIGHT
					    ? POPPOS_TOPRIGHT : POPPOS_TOPLEFT;
	}
    }

    wp->w_wantcol = curwin->w_wincol + curwin->w_wcol + 1;
    if (wp->w_wantcol > Columns - width)
    {
	wp->w_wantcol = Columns - width;
	if (wp->w_wantcol < 1)
	    wp->w_wantcol = 1;
    }

    popup_adjust_position(wp);
}

/*
 * Set w_wantline and w_wantcol for the a given screen position.
 * Caller must take care of running into the window border.
 */
    void
popup_set_wantpos_rowcol(win_T *wp, int row, int col)
{
    wp->w_wantline = row;
    wp->w_wantcol = col;
    popup_adjust_position(wp);
}

/*
 * Add a border and lef&right padding.
 */
    static void
add_border_left_right_padding(win_T *wp)
{
    int i;

    for (i = 0; i < 4; ++i)
    {
	wp->w_popup_border[i] = 1;
	wp->w_popup_padding[i] = (i & 1) ? 1 : 0;
    }
}

#ifdef FEAT_TERMINAL
/*
 * Return TRUE if there is any popup window with a terminal buffer.
 */
    static int
popup_terminal_exists(void)
{
    win_T	*wp;
    tabpage_T	*tp;

    FOR_ALL_POPUPWINS(wp)
	if (wp->w_buffer->b_term != NULL)
	    return TRUE;
    FOR_ALL_TABPAGES(tp)
	FOR_ALL_POPUPWINS_IN_TAB(tp, wp)
	    if (wp->w_buffer->b_term != NULL)
		return TRUE;
    return FALSE;
}
#endif

/*
 * Mark all popup windows in the current tab and global for redrawing.
 */
    void
popup_redraw_all(void)
{
    win_T	*wp;

    FOR_ALL_POPUPWINS(wp)
	wp->w_redr_type = UPD_NOT_VALID;
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	wp->w_redr_type = UPD_NOT_VALID;
}

/*
 * Return TRUE if any visible popup window needs a redraw.
 */
    int
popup_need_redraw(void)
{
    win_T	*wp;

    FOR_ALL_POPUPWINS(wp)
	if ((wp->w_popup_flags & POPF_HIDDEN) == 0 && wp->w_redr_type != 0)
	    return TRUE;
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	if ((wp->w_popup_flags & POPF_HIDDEN) == 0 && wp->w_redr_type != 0)
	    return TRUE;
    return FALSE;
}

/*
 * Set the color for a notification window.
 */
    static void
popup_update_color(win_T *wp, create_type_T type)
{
    char    *hiname = type == TYPE_MESSAGE_WIN
				       ? "MessageWindow" : "PopupNotification";
    char    *errmsg;

    errmsg = update_wincolor(wp, (char_u *)hiname);
    if (errmsg == NULL)
	set_string_option_direct_in_win(wp, (char_u *)"wincolor", -1,
		(char_u *)hiname, OPT_FREE|OPT_LOCAL, 0);
    else
	emsg(_(errmsg));
}

/*
 * popup_create({text}, {options})
 * popup_atcursor({text}, {options})
 * etc.
 * When creating a preview or info popup "argvars" and "rettv" are NULL.
 */
    static win_T *
popup_create(typval_T *argvars, typval_T *rettv, create_type_T type)
{
    win_T	*wp;
    tabpage_T	*tp = NULL;
    int		tabnr = 0;
    int		new_buffer;
    buf_T	*buf = NULL;
    dict_T	*d = NULL;
    int		i;

    if (argvars != NULL)
    {
	if (in_vim9script()
		&& (check_for_string_or_number_or_list_arg(argvars, 0) == FAIL
		    || check_for_dict_arg(argvars, 1) == FAIL))
	    return NULL;

	// Check that arguments look OK.
	if (argvars[0].v_type == VAR_NUMBER)
	{
	    buf = buflist_findnr(argvars[0].vval.v_number);
	    if (buf == NULL)
	    {
		semsg(_(e_buffer_nr_does_not_exist), argvars[0].vval.v_number);
		return NULL;
	    }
#ifdef FEAT_TERMINAL
	    if (buf->b_term != NULL && popup_terminal_exists())
	    {
		emsg(_(e_cannot_open_second_popup_with_terminal));
		return NULL;
	    }
#endif
	}
	else if (!(argvars[0].v_type == VAR_STRING
			&& argvars[0].vval.v_string != NULL)
		    && !(argvars[0].v_type == VAR_LIST
			&& argvars[0].vval.v_list != NULL))
	{
	    emsg(_(e_buffer_number_text_or_list_required));
	    return NULL;
	}
	if (check_for_nonnull_dict_arg(argvars, 1) == FAIL)
	    return NULL;
	d = argvars[1].vval.v_dict;
    }

    if (d != NULL)
    {
	if (dict_has_key(d, "tabpage"))
	    tabnr = (int)dict_get_number(d, "tabpage");
	else if (popup_is_notification(type))
	    tabnr = -1;  // notifications are global by default
	else
	    tabnr = 0;
	if (tabnr > 0)
	{
	    tp = find_tabpage(tabnr);
	    if (tp == NULL)
	    {
		semsg(_(e_tabpage_not_found_nr), tabnr);
		return NULL;
	    }
	}
    }
    else if (popup_is_notification(type))
	tabnr = -1;  // show on all tabs

    if (buf != NULL && buf->b_locked_split)
    {
	// disallow opening a popup to a closing buffer, which like splitting,
	// can result in more windows displaying it
	emsg(_(e_cannot_open_a_popup_window_to_a_closing_buffer));
	return NULL;
    }

    // Create the window and buffer.
    wp = win_alloc_popup_win();
    if (wp == NULL)
	return NULL;
    if (rettv != NULL)
	rettv->vval.v_number = wp->w_id;
    wp->w_popup_pos = POPPOS_TOPLEFT;
    wp->w_popup_flags = POPF_IS_POPUP | POPF_MAPPING | POPF_POSINVERT;

    if (buf != NULL)
    {
	// use existing buffer
	new_buffer = FALSE;
	win_init_popup_win(wp, buf);
	set_local_options_default(wp, FALSE);
	swap_exists_action = SEA_READONLY;
	buffer_ensure_loaded(buf);
	swap_exists_action = SEA_NONE;
    }
    else
    {
	// create a new buffer associated with the popup
	new_buffer = TRUE;
	buf = buflist_new(NULL, NULL, (linenr_T)0, BLN_NEW|BLN_DUMMY|BLN_REUSE);
	if (buf == NULL)
	{
	    win_free_popup(wp);
	    return NULL;
	}
	ml_open(buf);

	win_init_popup_win(wp, buf);

	set_local_options_default(wp, TRUE);
	set_string_option_direct_in_buf(buf, (char_u *)"buftype", -1,
				     (char_u *)"popup", OPT_FREE|OPT_LOCAL, 0);
	set_string_option_direct_in_buf(buf, (char_u *)"bufhidden", -1,
				      (char_u *)"wipe", OPT_FREE|OPT_LOCAL, 0);
	buf->b_p_ul = -1;	// no undo
	buf->b_p_swf = FALSE;   // no swap file
	buf->b_p_bl = FALSE;    // unlisted buffer
	buf->b_locked = TRUE;	// prevent deleting the buffer

	// Avoid that 'buftype' is reset when this buffer is entered.
	buf->b_p_initialized = true;
    }
    wp->w_p_wrap = TRUE;	// 'wrap' is default on
    wp->w_p_so = 0;		// 'scrolloff' zero
    wp->w_p_sop = 0;		// 'scrolloffpad' zero

    if (tp != NULL)
    {
	// popup on specified tab page
	wp->w_next = tp->tp_first_popupwin;
	tp->tp_first_popupwin = wp;
    }
    else if (tabnr == 0)
    {
	// popup on current tab page
	wp->w_next = curtab->tp_first_popupwin;
	curtab->tp_first_popupwin = wp;
    }
    else // (tabnr < 0)
    {
	win_T *prev = first_popupwin;

	// Global popup: add at the end, so that it gets displayed on top of
	// older ones with the same zindex. Matters for notifications.
	if (first_popupwin == NULL)
	    first_popupwin = wp;
	else
	{
	    while (prev->w_next != NULL)
		prev = prev->w_next;
	    prev->w_next = wp;
	}
    }

    if (new_buffer && argvars != NULL)
	popup_set_buffer_text(buf, argvars[0]);

    if (type == TYPE_ATCURSOR || type == TYPE_PREVIEW)
    {
	wp->w_popup_pos = POPPOS_BOTLEFT;
    }
    if (type == TYPE_ATCURSOR)
    {
	popup_set_wantpos_cursor(wp, 0, d);
	set_moved_values(wp);
	set_moved_columns(wp, FIND_STRING);
    }

    if (type == TYPE_BEVAL)
    {
	wp->w_popup_pos = POPPOS_BOTLEFT;

	// by default use the mouse position
	wp->w_wantline = mouse_row;
	if (wp->w_wantline <= 0)  // mouse on first line
	{
	    wp->w_wantline = 2;
	    wp->w_popup_pos = POPPOS_TOPLEFT;
	}
	wp->w_wantcol = mouse_col + 1;
	set_mousemoved_values(wp);
	set_mousemoved_columns(wp, FIND_IDENT + FIND_STRING + FIND_EVAL);
    }

    // set default values
    wp->w_zindex = POPUPWIN_DEFAULT_ZINDEX;
    wp->w_popup_close = POPCLOSE_NONE;

    if (popup_is_notification(type))
    {
	win_T  *twp, *nextwin;
	int	height = buf->b_ml.ml_line_count + 3;

	// Try to not overlap with another global popup.  Guess we need 3
	// more screen lines than buffer lines.
	wp->w_wantline = 1;
	for (twp = first_popupwin; twp != NULL; twp = nextwin)
	{
	    nextwin = twp->w_next;
	    if (twp != wp
		    && twp->w_zindex == POPUPWIN_NOTIFICATION_ZINDEX
		    && twp->w_winrow <= wp->w_wantline - 1 + height
		    && twp->w_winrow + popup_height(twp) > wp->w_wantline - 1)
	    {
		// move to below this popup and restart the loop to check for
		// overlap with other popups
		wp->w_wantline = twp->w_winrow + popup_height(twp) + 1;
		nextwin = first_popupwin;
	    }
	}
	if (wp->w_wantline + height > Rows)
	{
	    // can't avoid overlap, put on top in the hope that message goes
	    // away soon.
	    wp->w_wantline = 1;
	}

	wp->w_wantcol = 10;
	wp->w_zindex = POPUPWIN_NOTIFICATION_ZINDEX;
	wp->w_minwidth = 20;
	wp->w_popup_flags |= POPF_DRAG;
	wp->w_popup_close = POPCLOSE_CLICK;
	for (i = 0; i < 4; ++i)
	    wp->w_popup_border[i] = 1;
	wp->w_popup_padding[1] = 1;
	wp->w_popup_padding[3] = 1;

	popup_update_color(wp, type);
    }

    if (type == TYPE_DIALOG || type == TYPE_MENU)
    {
	wp->w_popup_pos = POPPOS_CENTER;
	wp->w_zindex = POPUPWIN_DIALOG_ZINDEX;
	wp->w_popup_flags |= POPF_DRAG;
	wp->w_popup_flags &= ~POPF_MAPPING;
	add_border_left_right_padding(wp);
    }

    if (type == TYPE_MENU)
    {
	typval_T	tv;
	callback_T	callback;

	tv.v_type = VAR_STRING;
	tv.vval.v_string = (char_u *)"popup_filter_menu";
	callback = get_callback(&tv);
	if (callback.cb_name != NULL)
	{
	    set_callback(&wp->w_filter_cb, &callback);
	}

	wp->w_p_wrap = 0;
	wp->w_popup_flags |= POPF_CURSORLINE;
    }

    for (i = 0; i < 4; ++i)
	VIM_CLEAR(wp->w_border_highlight[i]);
    for (i = 0; i < 8; ++i)
	wp->w_border_char[i] = 0;

    if (type == TYPE_PREVIEW)
    {
	if (mouse_has(MOUSE_INSERT))
	{
	    wp->w_popup_flags |= POPF_DRAG | POPF_RESIZE;
	    wp->w_popup_close = POPCLOSE_BUTTON;
	}
	for (i = 0; i < 4; ++i)
	    wp->w_popup_border[i] = 1;
	parse_previewpopup(wp);
	popup_set_wantpos_cursor(wp, wp->w_minwidth, d);
    }

#ifdef FEAT_QUICKFIX
    if (type == TYPE_INFO)
    {
	wp->w_popup_pos = POPPOS_TOPLEFT;
	if (mouse_has(MOUSE_INSERT))
	{
	    wp->w_popup_flags |= POPF_DRAG | POPF_RESIZE;
	    wp->w_popup_close = POPCLOSE_BUTTON;
	}
	add_border_left_right_padding(wp);
	parse_completepopup(wp);
    }
#endif

    wp->w_want_scrollbar = 1;
    wp->w_popup_fixed = 0;
    wp->w_filter_mode = MODE_ALL;

    if (d != NULL)
    {
	// Deal with options.
	if (apply_options(wp, d, TRUE) == FAIL)
	{
	    (void)popup_close(wp->w_id, FALSE);
	    return NULL;
	}
    }

#ifdef FEAT_TIMERS
    if (popup_is_notification(type) && wp->w_popup_timer == NULL)
	popup_add_timeout(wp, 3000, type == TYPE_NOTIFICATION);
#endif

    popup_adjust_position(wp);

    wp->w_vsep_width = 0;

    redraw_all_later(UPD_NOT_VALID);
    popup_mask_refresh = TRUE;

#ifdef FEAT_TERMINAL
    // When running a terminal in the popup it becomes the current window.
    if (buf->b_term != NULL)
	win_enter(wp, FALSE);
#endif

    return wp;
}

/*
 * popup_clear()
 */
    void
f_popup_clear(typval_T *argvars, typval_T *rettv UNUSED)
{
    int force = FALSE;

    if (in_vim9script() && check_for_opt_bool_arg(argvars, 0) == FAIL)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
	force = (int)tv_get_bool(&argvars[0]);
    close_all_popups(force);
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
 * popup_beval({text}, {options})
 */
    void
f_popup_beval(typval_T *argvars, typval_T *rettv)
{
    popup_create(argvars, rettv, TYPE_BEVAL);
}

/*
 * Invoke the close callback for window "wp" with value "result".
 * Careful: The callback may make "wp" invalid!
 */
    static void
invoke_popup_callback(win_T *wp, typval_T *result)
{
    typval_T	rettv;
    typval_T	argv[3];

    rettv.v_type = VAR_UNKNOWN;

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

    call_callback(&wp->w_close_cb, -1, &rettv, 2, argv);
    if (result != NULL)
	clear_tv(&argv[1]);
    clear_tv(&rettv);
}

/*
 * Make "prevwin" the current window, unless it's equal to "wp".
 * Otherwise make "firstwin" the current window.
 */
    static void
back_to_prevwin(win_T *wp)
{
    if (win_valid(prevwin) && wp != prevwin)
	win_enter(prevwin, FALSE);
    else
	win_enter(firstwin, FALSE);
}

/*
 * Close popup "wp" and invoke any close callback for it.
 * Careful: callback function might have freed the popup window already
 */
    static void
popup_close_and_callback(win_T *wp, typval_T *arg)
{
    int id;

    if (!win_valid(wp))
       return;

    id = wp->w_id;

#ifdef FEAT_TERMINAL
    if (wp == curwin && curbuf->b_term != NULL)
    {
	win_T *owp;

	// Closing popup window with a terminal: put focus back on the first
	// that works:
	// - another popup window with a terminal
	// - the previous window
	// - the first one.
	FOR_ALL_POPUPWINS(owp)
	    if (owp != curwin && owp->w_buffer->b_term != NULL)
		break;
	if (owp != NULL)
	    win_enter(owp, FALSE);
	else
	{
	    FOR_ALL_POPUPWINS_IN_TAB(curtab, owp)
		if (owp != curwin && owp->w_buffer->b_term != NULL)
		    break;
	    if (owp != NULL)
		win_enter(owp, FALSE);
	    else
		back_to_prevwin(wp);
	}
    }
#endif

    // Just in case a check higher up is missing.
    if (wp == curwin && ERROR_IF_POPUP_WINDOW)
    {
	// To avoid getting stuck when win_execute() does something that causes
	// an error, stop calling the filter callback.
	free_callback(&wp->w_filter_cb);

	return;
    }

    CHECK_CURBUF;
    if (wp->w_close_cb.cb_name != NULL)
	// Careful: This may make "wp" invalid.
	invoke_popup_callback(wp, arg);

    popup_close(id, FALSE);
    CHECK_CURBUF;
}

    void
popup_close_with_retval(win_T *wp, int retval)
{
    typval_T res;

    res.v_type = VAR_NUMBER;
    res.vval.v_number = retval;
    popup_close_and_callback(wp, &res);
}

/*
 * Close popup "wp" because of a mouse click.
 */
    void
popup_close_for_mouse_click(win_T *wp)
{
    popup_close_with_retval(wp, -2);
}

    static void
check_mouse_moved(win_T *wp, win_T *mouse_wp)
{
    // Close the popup when all if these are true:
    // - the mouse is not on this popup
    // - "mousemoved" was used
    // - the mouse is no longer on the same screen row or the mouse column is
    //   outside of the relevant text
    if (wp != mouse_wp
	    && wp->w_popup_mouse_row != 0
	    && (wp->w_popup_mouse_row != mouse_row
		|| mouse_col < wp->w_popup_mouse_mincol
		|| mouse_col > wp->w_popup_mouse_maxcol))
    {
	// Careful: this makes "wp" invalid.
	popup_close_with_retval(wp, -2);
    }
}

/*
 * Called when the mouse moved: may close a popup with "mousemoved".
 */
    void
popup_handle_mouse_moved(void)
{
    win_T   *wp, *nextwp;
    win_T   *mouse_wp;
    int	    row = mouse_row;
    int	    col = mouse_col;

    // find the window where the mouse is in
    mouse_wp = mouse_find_win(&row, &col, FIND_POPUP);

    for (wp = first_popupwin; wp != NULL; wp = nextwp)
    {
	nextwp = wp->w_next;
	check_mouse_moved(wp, mouse_wp);
    }
    for (wp = curtab->tp_first_popupwin; wp != NULL; wp = nextwp)
    {
	nextwp = wp->w_next;
	check_mouse_moved(wp, mouse_wp);
    }
}

/*
 * In a filter: check if the typed key is a mouse event that is used for
 * dragging the popup.
 */
    static void
filter_handle_drag(win_T *wp, int c, typval_T *rettv)
{
    int	row = mouse_row;
    int	col = mouse_col;

    if ((wp->w_popup_flags & (POPF_DRAG | POPF_DRAGALL))
	    && is_mouse_key(c)
	    && (wp == popup_dragwin
			  || wp == mouse_find_win(&row, &col, FIND_POPUP)))
	// do not consume the key, allow for dragging the popup
	rettv->vval.v_number = 0;
}

/*
 * popup_filter_menu({id}, {key})
 */
    void
f_popup_filter_menu(typval_T *argvars, typval_T *rettv)
{
    int		id;
    win_T	*wp;
    char_u	*key;
    typval_T	res;
    int		c;
    linenr_T	old_lnum;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_string_arg(argvars, 1) == FAIL))
	return;

    id = tv_get_number(&argvars[0]);
    wp = win_id2wp(id);
    key = tv_get_string(&argvars[1]);
    // If the popup has been closed do not consume the key.
    if (wp == NULL)
	return;

    c = *key;
    if (c == K_SPECIAL && key[1] != NUL)
	c = TO_SPECIAL(key[1], key[2]);

    // consume all keys until done
    rettv->v_type = VAR_BOOL;
    rettv->vval.v_number = VVAL_TRUE;
    res.v_type = VAR_NUMBER;

    old_lnum = wp->w_cursor.lnum;
    if (c == 'k' || c == 'K' || c == K_UP || c == Ctrl_P)
    {
	if (wp->w_cursor.lnum > 1)
	    --wp->w_cursor.lnum;
	else
	    wp->w_cursor.lnum = wp->w_buffer->b_ml.ml_line_count;
    }
    if (c == 'j' || c == 'J' || c == K_DOWN || c == Ctrl_N)
    {
	if (wp->w_cursor.lnum < wp->w_buffer->b_ml.ml_line_count)
	    ++wp->w_cursor.lnum;
	else
	    wp->w_cursor.lnum = 1;
    }
    if (old_lnum != wp->w_cursor.lnum)
    {
	// caller will call popup_highlight_curline()
	return;
    }

    if (c == 'x' || c == 'X' || c == ESC || c == Ctrl_C)
    {
	// Cancelled, invoke callback with -1
	res.vval.v_number = -1;
	popup_close_and_callback(wp, &res);
	return;
    }
    if (c == ' ' || c == K_KENTER || c == CAR || c == NL)
    {
	// Invoke callback with current index.
	res.vval.v_number = wp->w_cursor.lnum;
	popup_close_and_callback(wp, &res);
	return;
    }

    filter_handle_drag(wp, c, rettv);
}

/*
 * popup_filter_yesno({id}, {key})
 */
    void
f_popup_filter_yesno(typval_T *argvars, typval_T *rettv)
{
    int		id;
    win_T	*wp;
    char_u	*key;
    typval_T	res;
    int		c;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_string_arg(argvars, 1) == FAIL))
	return;

    id = tv_get_number(&argvars[0]);
    wp = win_id2wp(id);
    key = tv_get_string(&argvars[1]);
    // If the popup has been closed don't consume the key.
    if (wp == NULL)
	return;

    c = *key;
    if (c == CAR && need_wait_return)
	return;
    if (c == K_SPECIAL && key[1] != NUL)
	c = TO_SPECIAL(key[1], key[2]);

    // consume all keys until done
    rettv->v_type = VAR_BOOL;
    rettv->vval.v_number = VVAL_TRUE;

    if (c == 'y' || c == 'Y')
	res.vval.v_number = 1;
    else if (c == 'n' || c == 'N' || c == 'x' || c == 'X' || c == ESC)
	res.vval.v_number = 0;
    else
    {
	filter_handle_drag(wp, c, rettv);
	return;
    }

    // Invoke callback
    res.v_type = VAR_NUMBER;
    popup_close_and_callback(wp, &res);
}

/*
 * popup_dialog({text}, {options})
 */
    void
f_popup_dialog(typval_T *argvars, typval_T *rettv)
{
    popup_create(argvars, rettv, TYPE_DIALOG);
}

/*
 * popup_menu({text}, {options})
 */
    void
f_popup_menu(typval_T *argvars, typval_T *rettv)
{
    popup_create(argvars, rettv, TYPE_MENU);
}

/*
 * popup_notification({text}, {options})
 */
    void
f_popup_notification(typval_T *argvars, typval_T *rettv)
{
    popup_create(argvars, rettv, TYPE_NOTIFICATION);
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

    if (wp != NULL && !WIN_IS_POPUP(wp))
    {
	semsg(_(e_window_nr_is_not_popup_window), id);
	return NULL;
    }
    return wp;
}

/*
 * popup_close({id})
 */
    void
f_popup_close(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id;
    win_T	*wp;

    if (in_vim9script() && check_for_number_arg(argvars, 0) == FAIL)
	return;

    id = (int)tv_get_number(argvars);
    if (
#ifdef FEAT_TERMINAL
	// if the popup contains a terminal it will become hidden
	curbuf->b_term == NULL &&
#endif
	    ERROR_IF_ANY_POPUP_WINDOW)
	return;

    wp = find_popup_win(id);
    if (wp != NULL)
	popup_close_and_callback(wp, &argvars[1]);
}

/*
 * Clear popup_mask entries for the cells covered by "wp" so that
 * screen_fill / screen_puts calls made before the next update_screen()
 * (e.g. msg_clr_eos triggered by a status message) are not silently
 * dropped by skip_for_popup().  Without this the popup's chars survive
 * on screen until may_update_popup_mask() runs and the affected cells
 * happen to be redrawn.
 */
    static void
popup_clear_mask_for(win_T *wp)
{
    int r, c;
    int row_start, col_start, row_end, col_end;

    if (popup_mask == NULL || !popup_visible)
	return;

    row_start = MAX(wp->w_winrow, 0);
    col_start = MAX(wp->w_wincol, 0);
    row_end = MIN(wp->w_winrow + popup_height(wp), (int)screen_Rows);
    col_end = MIN(wp->w_wincol + popup_width(wp), (int)screen_Columns);

    for (r = row_start; r < row_end; ++r)
	for (c = col_start; c < col_end; ++c)
	    popup_mask[r * screen_Columns + c] = 0;
}

    void
popup_hide(win_T *wp)
{
    popup_area_T	old_area;
    int			was_visible = (wp->w_popup_flags & POPF_HIDDEN) == 0;

#ifdef FEAT_TERMINAL
    if (error_if_term_popup_window())
	return;
#endif
    if ((wp->w_popup_flags & POPF_HIDDEN) != 0)
	return;

    popup_save_area(wp, &old_area);

    wp->w_popup_flags |= POPF_HIDDEN;
    // Do not decrement b_nwindows, we still reference the buffer.
    if (wp->w_winrow + popup_height(wp) >= cmdline_row)
	clear_cmdline = TRUE;

    if (was_visible)
	popup_clear_mask_for(wp);

    if (old_area.active)
	popup_redraw_exposed_area(&old_area);
    else
	redraw_all_later(UPD_NOT_VALID);

    status_redraw_all();
    if (!old_area.active)
	popup_mask_refresh = TRUE;
}

/*
 * popup_hide({id})
 */
    void
f_popup_hide(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id;
    win_T	*wp;

    if (in_vim9script() && check_for_number_arg(argvars, 0) == FAIL)
	return;

    id = (int)tv_get_number(argvars);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;

    popup_hide(wp);
    wp->w_popup_flags |= POPF_HIDDEN_FORCE;
}

    void
popup_show(win_T *wp)
{
    bool popup_active;

    if ((wp->w_popup_flags & POPF_HIDDEN) == 0)
	return;

    popup_active = (wp->w_popup_flags & POPF_OPACITY) && wp->w_popup_blend > 0;
    wp->w_popup_flags &= ~POPF_HIDDEN;
    if (popup_active)
    {
	wp->w_redr_type = UPD_NOT_VALID;
	wp->w_lines_valid = 0;
	if (must_redraw < UPD_VALID)
	    must_redraw = UPD_VALID;
    }
    else
    {
	redraw_all_later(UPD_NOT_VALID);
	popup_mask_refresh = TRUE;
    }
}

/*
 * popup_show({id})
 */
    void
f_popup_show(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id;
    win_T	*wp;

    rettv->v_type = VAR_NUMBER;
    rettv->vval.v_number = -1;

    if (in_vim9script() && check_for_number_arg(argvars, 0) == FAIL)
	return;

    id = (int)tv_get_number(argvars);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;

    wp->w_popup_flags &= ~POPF_HIDDEN_FORCE;
    popup_show(wp);
#ifdef FEAT_QUICKFIX
    if (wp->w_popup_flags & POPF_INFO)
	pum_position_info_popup(wp);
#endif

    rettv->vval.v_number = 0;
}

/*
 * popup_settext({id}, {text})
 */
    void
f_popup_settext(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		id;
    win_T	*wp;
    popup_area_T	old_area;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_string_or_list_arg(argvars, 1) == FAIL))
	return;

    id = (int)tv_get_number(&argvars[0]);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;

    popup_save_area(wp, &old_area);

    if (check_for_string_or_list_arg(argvars, 1) == FAIL)
	return;

    popup_set_buffer_text(wp->w_buffer, argvars[1]);

    // Redraw the popup window without triggering a full screen redraw.
    // Using redraw_win_later() with UPD_NOT_VALID would set the global
    // must_redraw, causing may_update_popup_mask() to refresh the mask and
    // redraw windows behind the popup, resulting in flickering.
    wp->w_redr_type = UPD_NOT_VALID;
    wp->w_lines_valid = 0;
    if (must_redraw < UPD_VALID)
	must_redraw = UPD_VALID;
    popup_adjust_position(wp);

    if (popup_area_changed(wp, &old_area))
	popup_redraw_exposed_area(&old_area);
}

/*
 * popup_setbuf({id}, {bufnr})
 */
    void
f_popup_setbuf(typval_T *argvars, typval_T *rettv UNUSED)
{
    int			id;
    win_T		*wp;
    buf_T		*buf;
    popup_area_T	old_area;

    rettv->v_type = VAR_BOOL;
    rettv->vval.v_number = VVAL_FALSE;

    if (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_buffer_arg(argvars, 1) == FAIL)
	return;

    id = (int)tv_get_number(&argvars[0]);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;

    buf = tv_get_buf_from_arg(&argvars[1]);

    if (buf == NULL)
	return;
#ifdef FEAT_TERMINAL
    if (buf->b_term != NULL && popup_terminal_exists())
    {
	emsg(_(e_cannot_open_second_popup_with_terminal));
	return;
    }
#endif

    if (wp->w_buffer != buf)
    {
	popup_save_area(wp, &old_area);

	wp->w_buffer->b_nwindows--;
	win_init_popup_win(wp, buf);
	set_local_options_default(wp, FALSE);
	swap_exists_action = SEA_READONLY;
	buffer_ensure_loaded(buf);
	swap_exists_action = SEA_NONE;
	redraw_win_later(wp, UPD_NOT_VALID);
	popup_adjust_position(wp);

	if (popup_area_changed(wp, &old_area))
	    popup_redraw_exposed_area(&old_area);
    }
    rettv->vval.v_number = VVAL_TRUE;
}

    static void
popup_free(win_T *wp)
{
    popup_area_T	old_area;
    int			was_visible = (wp->w_popup_flags & POPF_HIDDEN) == 0;

    popup_save_area(wp, &old_area);

    sign_undefine_by_name(popup_get_sign_name(wp), FALSE);
    wp->w_buffer->b_locked = FALSE;
    if (wp->w_winrow + popup_height(wp) >= cmdline_row)
	clear_cmdline = TRUE;

    if (was_visible)
	popup_clear_mask_for(wp);

    popup_redraw_exposed_area(&old_area);

    win_free_popup(wp);

#ifdef HAS_MESSAGE_WINDOW
    if (wp == message_win)
	message_win = NULL;
#endif

    if (!old_area.active)
	redraw_all_later(UPD_NOT_VALID);
    status_redraw_all();
    if (!old_area.active)
	popup_mask_refresh = TRUE;
}

    static void
error_for_popup_window(void)
{
    emsg(_(e_not_allowed_in_popup_window));
}

    int
error_if_popup_window(int also_with_term UNUSED)
{
    // win_execute() may set "curwin" to a popup window temporarily, but many
    // commands are disallowed then.  When a terminal runs in the popup most
    // things are allowed.  When a terminal is finished it can be closed.
    if (WIN_IS_POPUP(curwin)
#ifdef FEAT_TERMINAL
	    && (also_with_term || curbuf->b_term == NULL)
#endif
	    )
    {
	error_for_popup_window();
	return TRUE;
    }
    return FALSE;
}

/*
 * Close a popup window by Window-id.
 * Does not invoke the callback.
 * Return OK if the popup was closed, FAIL otherwise.
 */
    int
popup_close(int id, int force)
{
    win_T	*wp;
    tabpage_T	*tp;
    win_T	*prev = NULL;

    // go through global popups
    for (wp = first_popupwin; wp != NULL; prev = wp, wp = wp->w_next)
	if (wp->w_id == id)
	{
	    if (wp == curwin)
	    {
		if (!force)
		{
		    error_for_popup_window();
		    return FAIL;
		}
		back_to_prevwin(wp);
	    }
	    if (prev == NULL)
		first_popupwin = wp->w_next;
	    else
		prev->w_next = wp->w_next;
	    popup_free(wp);
	    return OK;
	}

    // go through tab-local popups
    FOR_ALL_TABPAGES(tp)
	if (popup_close_tabpage(tp, id, force) == OK)
	    return OK;
    return FAIL;
}

/*
 * Close a popup window with Window-id "id" in tabpage "tp".
 */
    int
popup_close_tabpage(tabpage_T *tp, int id, int force)
{
    win_T	*wp;
    win_T	**root = &tp->tp_first_popupwin;
    win_T	*prev = NULL;

    for (wp = *root; wp != NULL; prev = wp, wp = wp->w_next)
	if (wp->w_id == id)
	{
	    if (wp == curwin)
	    {
		if (!force)
		{
		    error_for_popup_window();
		    return FAIL;
		}
		back_to_prevwin(wp);
	    }

	    // Set curwin for tabpage to a valid window, in case we try
	    // accessing it later.
	    if (tp->tp_curwin == wp)
		tp->tp_curwin = tp->tp_firstwin;

	    if (prev == NULL)
		*root = wp->w_next;
	    else
		prev->w_next = wp->w_next;
	    popup_free(wp);
	    return OK;
	}
    return FAIL;
}

    void
close_all_popups(int force)
{
    if (!force && ERROR_IF_ANY_POPUP_WINDOW)
	return;
    while (first_popupwin != NULL)
	if (popup_close(first_popupwin->w_id, force) == FAIL)
	    return;
    while (curtab->tp_first_popupwin != NULL)
	if (popup_close(curtab->tp_first_popupwin->w_id, force) == FAIL)
	    return;
}

/*
 * Save the current popup area that may need to be restored later.
 */
    static void
popup_save_area(win_T *wp, popup_area_T *area)
{
    area->active = (wp->w_popup_flags & POPF_OPACITY) && wp->w_popup_blend > 0;
    area->winrow = wp->w_winrow;
    area->wincol = wp->w_wincol;
    area->height = popup_height(wp);
    area->width = popup_width(wp);
    area->leftoff = wp->w_popup_leftoff;
    area->zindex = wp->w_zindex;
}

/*
 * Save popup style-related fields that affect redraw/reposition decisions.
 */
    static void
popup_save_style(win_T *wp, popup_style_snapshot_T *style)
{
    int i;

    style->firstline = wp->w_firstline;
    style->blend = wp->w_popup_blend;
    style->flags = wp->w_popup_flags;
    style->zindex = wp->w_zindex;
    style->scrollbar_highlight = wp->w_scrollbar_highlight;
    style->thumb_highlight = wp->w_thumb_highlight;
    for (i = 0; i < 4; i++)
	style->border_highlight[i] = wp->w_border_highlight[i];
}

/*
 * Return true if style changes require at least a popup redraw.
 */
    static bool
popup_style_changed(win_T *wp, popup_style_snapshot_T *style)
{
    int i;

    if (style->firstline != wp->w_firstline
	    || style->flags != wp->w_popup_flags
	    || style->scrollbar_highlight != wp->w_scrollbar_highlight
	    || style->thumb_highlight != wp->w_thumb_highlight)
	return true;
    for (i = 0; i < 4; i++)
	if (style->border_highlight[i] != wp->w_border_highlight[i])
	    return true;
    return false;
}

/*
 * Save popup layout fields that affect mask refresh and local redraw.
 */
    static void
popup_save_layout(win_T *wp, popup_layout_T *layout)
{
    layout->winrow = wp->w_winrow;
    layout->wincol = wp->w_wincol;
    layout->width = wp->w_width;
    layout->height = wp->w_height;
    layout->leftcol = wp->w_leftcol;
    layout->leftoff = wp->w_popup_leftoff;
    layout->has_scrollbar = wp->w_has_scrollbar;
    layout->topoff = wp->w_popup_topoff;
    layout->bottomoff = wp->w_popup_bottomoff;
    layout->leftclip = wp->w_popup_leftclip;
    layout->rightclip = wp->w_popup_rightclip;
}

/*
 * Return true when the popup layout changed.
 */
    static bool
popup_layout_changed(win_T *wp, popup_layout_T *layout)
{
    return layout->winrow != wp->w_winrow
	|| layout->wincol != wp->w_wincol
	|| layout->leftcol != wp->w_leftcol
	|| layout->leftoff != wp->w_popup_leftoff
	|| layout->width != wp->w_width
	|| layout->height != wp->w_height
	|| layout->has_scrollbar != wp->w_has_scrollbar
	|| layout->topoff != wp->w_popup_topoff
	|| layout->bottomoff != wp->w_popup_bottomoff
	|| layout->leftclip != wp->w_popup_leftclip
	|| layout->rightclip != wp->w_popup_rightclip;
}

/*
 * Return true when the popup no longer covers the saved area.
 */
    static bool
popup_area_changed(win_T *wp, popup_area_T *area)
{
    return area->winrow != wp->w_winrow
	|| area->wincol != wp->w_wincol
	|| area->height != popup_height(wp)
	|| area->width != popup_width(wp)
	|| area->leftoff != wp->w_popup_leftoff;
}

/*
 * If "wp" is a visible opacity popup at or below "zindex" whose drawn area
 * overlaps the rectangle, mark it for full redraw so its blended background
 * is recomputed.
 */
    static void
mark_overlapped_opacity_popup(win_T *wp, int area_top, int area_bot,
	int area_left, int area_right, int zindex)
{
    if ((wp->w_popup_flags & POPF_HIDDEN)
	    || (wp->w_popup_flags & POPF_OPACITY) == 0
	    || wp->w_popup_blend == 0
	    || wp->w_zindex > zindex
	    || wp->w_winrow >= area_bot
	    || wp->w_winrow + popup_height(wp) <= area_top
	    || wp->w_wincol >= area_right
	    || wp->w_wincol + popup_width(wp) - wp->w_popup_leftoff
							      <= area_left)
	return;

    wp->w_redr_type = UPD_NOT_VALID;
    wp->w_lines_valid = 0;
}

/*
 * Mark lower or equal zindex opacity popups that overlap with a popup area
 * for redraw.  Their blended background may have included the old popup.
 */
    static void
redraw_overlapped_opacity_popups(int winrow, int wincol, int height, int width,
	int leftoff, int zindex)
{
    win_T	*wp;
    int		area_top = winrow;
    int		area_bot = winrow + height;
    int		area_left = wincol;
    int		area_right = wincol + width - leftoff;

    FOR_ALL_POPUPWINS(wp)
	mark_overlapped_opacity_popup(wp, area_top, area_bot, area_left,
		area_right, zindex);
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	mark_overlapped_opacity_popup(wp, area_top, area_bot, area_left,
		area_right, zindex);

    if (must_redraw < UPD_VALID)
	must_redraw = UPD_VALID;
}

/*
 * Replace pum_bg_* cells [left, right) of screen row "r" with the underlying
 * buffer text, so the pum's opacity padding shows the buffer through.  Cells
 * outside any window (or past the end of the buffer line) become spaces.
 *
 * Used after an opacity popup that overlapped pum_bg_* is dismissed: the
 * stale popup content held in pum_bg_* would otherwise leak through the
 * pum's opacity blend as a ghost.  This walks the buffer line and writes
 * the displayed character at each visual column directly into pum_bg_*.
 *
 * Limitations: handles plain text and tabs; folds, conceal, virtual text and
 * other rendering features fall back to a space, which still beats showing a
 * stale popup char.
 */
    static void
refill_pum_bg_row_from_buffer(int r, int left, int right)
{
    int		line_cp = r;
    int		col_cp = left;
    win_T	*wp;
    linenr_T	lnum;
    int		row_for_lnum;
    int		col_for_lnum = 0;
    char_u	*line;
    char_u	*p;
    int		win_text_col;
    int		screen_col;
    int		soff_base = (r - pum_bg_top) * pum_bg_cols;
    int		c;

    // Default: fill the range with spaces so trailing/empty cells render as
    // plain pum bg through opacity.
    for (c = left; c < right; ++c)
    {
	pum_bg_lines[soff_base + c] = ' ';
	if (pum_bg_attrs != NULL)
	    pum_bg_attrs[soff_base + c] = 0;
	if (enc_utf8 && pum_bg_linesUC != NULL)
	    pum_bg_linesUC[soff_base + c] = 0;
	if (enc_utf8)
	{
	    int k;
	    for (k = 0; k < MAX_MCO; ++k)
		if (pum_bg_linesC[k] != NULL)
		    pum_bg_linesC[k][soff_base + c] = 0;
	}
    }

    wp = mouse_find_win(&line_cp, &col_cp, IGNORE_POPUP);
    if (wp == NULL || line_cp < 0 || line_cp >= wp->w_height)
	return;
    if (wp->w_buffer == NULL || wp->w_buffer->b_ml.ml_mfp == NULL)
	return;

    // Compute the buffer line for this screen row.
    row_for_lnum = line_cp;
    if (mouse_comp_pos(wp, &row_for_lnum, &col_for_lnum, &lnum, NULL))
	return;	// past end of buffer
    if (lnum < 1 || lnum > wp->w_buffer->b_ml.ml_line_count)
	return;

    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
    if (line == NULL)
	return;

    // Walk the buffer line and write each displayed cell into pum_bg_*.
    // win_text_col is the screen column where the buffer text starts inside
    // the window (after sign/number/fold columns and horizontal scroll).
    win_text_col = wp->w_wincol + win_col_off(wp);
    if (!wp->w_p_wrap)
	win_text_col -= wp->w_leftcol;
    screen_col = win_text_col;
    p = line;
    while (*p != NUL && screen_col < right)
    {
	int char_cells;
	int byte_count;
	int soff = soff_base + screen_col;
	int in_range = (screen_col >= left && screen_col < right);

	if (*p == '\t')
	{
	    int ts = (int)wp->w_buffer->b_p_ts;
	    char_cells = ts > 0 ? ts - ((screen_col - win_text_col) % ts) : 1;
	    byte_count = 1;
	    if (in_range)
	    {
		pum_bg_lines[soff] = ' ';
		if (enc_utf8 && pum_bg_linesUC != NULL)
		    pum_bg_linesUC[soff] = 0;
	    }
	}
	else if (has_mbyte)
	{
	    char_cells = mb_ptr2cells(p);
	    byte_count = mb_ptr2len(p);
	    if (in_range)
	    {
		pum_bg_lines[soff] = *p;
		if (enc_utf8 && pum_bg_linesUC != NULL)
		    pum_bg_linesUC[soff] = (*p < 0x80) ? 0 : mb_ptr2char(p);
	    }
	}
	else
	{
	    char_cells = 1;
	    byte_count = 1;
	    if (in_range)
	    {
		pum_bg_lines[soff] = (*p < 0x20) ? ' ' : *p;
		if (enc_utf8 && pum_bg_linesUC != NULL)
		    pum_bg_linesUC[soff] = 0;
	    }
	}
	if (in_range && pum_bg_attrs != NULL)
	    pum_bg_attrs[soff] = 0;

	// For wide chars / tabs the trailing cells are zeroed already (by the
	// initial space fill we did above).  Just skip past them.
	p += byte_count;
	screen_col += char_cells;
    }
}

/*
 * Redraw what becomes exposed when an opacity popup moves, resizes or closes.
 */
    static void
popup_redraw_exposed_area(popup_area_T *area)
{
    if (!area->active)
	return;

    redraw_under_popup_area(area->winrow, area->wincol, area->height,
	    area->width, area->leftoff);
    redraw_overlapped_opacity_popups(area->winrow, area->wincol,
	    area->height, area->width, area->leftoff, area->zindex);

    // If the closing/moving popup overlapped the pum's saved background,
    // pum_bg_* still holds the dismissed popup's content.  When the pum
    // next blends opacity it would restore those stale chars at padding
    // cells, leaving a ghost.
    //
    // We can't re-snapshot via update_screen from here: the surrounding
    // update_screen has updating_screen set, so a nested call would no-op.
    // Instead, replace the overlapping pum_bg_* cells with the actual
    // underlying buffer text so the pum's opacity padding shows the buffer
    // through, just as it would if the popup had never been there.
    if (pum_bg_lines != NULL
	    && area->winrow < pum_bg_bot
	    && area->winrow + area->height > pum_bg_top)
    {
	int top = MAX(area->winrow, pum_bg_top);
	int bot = MIN(area->winrow + area->height, pum_bg_bot);
	int left = MAX(area->wincol, 0);
	int right = MIN(area->wincol + area->width, pum_bg_cols);
	int r;

	for (r = top; r < bot; ++r)
	    refill_pum_bg_row_from_buffer(r, left, right);
    }
}

/*
 * Release saved screen data used for opacity padding redraw.
 */
    static void
popup_free_saved_screen(popup_saved_screen_T *saved_screen)
{
    vim_free(saved_screen->lines);
    vim_free(saved_screen->attrs);
    vim_free(saved_screen->linesuc);
    CLEAR_POINTER(saved_screen);
}

/*
 * Save the screen area that opacity padding may need to blend against.
 * On entry "saved_screen" must be zero-initialised.  When the snapshot is
 * unavailable (no opacity popup, no padding, allocation failure) "lines" is
 * left NULL and callers should fall back to plain screen_fill().  The caller
 * must always release the snapshot with popup_free_saved_screen().
 */
    static void
popup_save_padding_screen(win_T *wp, popup_saved_screen_T *saved_screen)
{
    if (screen_opacity_popup == NULL
	    || (wp->w_popup_padding[0] == 0 && wp->w_popup_padding[1] == 0
		&& wp->w_popup_padding[2] == 0 && wp->w_popup_padding[3] == 0))
	return;

    saved_screen->start_row = wp->w_winrow + wp->w_popup_border[0];
    saved_screen->start_col = wp->w_wincol + wp->w_popup_border[3];
    saved_screen->rows = wp->w_popup_padding[0] + wp->w_height
						    + wp->w_popup_padding[2];
    saved_screen->cols = wp->w_popup_padding[3] + wp->w_width
						    + wp->w_popup_padding[1];

    // Include one column to the left to handle wide chars that overlap the
    // padding boundary.
    if (saved_screen->start_col > 0)
    {
	--saved_screen->start_col;
	++saved_screen->cols;
    }

    saved_screen->lines = ALLOC_MULT(schar_T,
				  saved_screen->rows * saved_screen->cols);
    saved_screen->attrs = ALLOC_MULT(int,
				  saved_screen->rows * saved_screen->cols);
    if (enc_utf8)
	saved_screen->linesuc = ALLOC_MULT(u8char_T,
				  saved_screen->rows * saved_screen->cols);

    if (saved_screen->lines == NULL || saved_screen->attrs == NULL)
	return;

    for (int r = 0; r < saved_screen->rows; r++)
    {
	int screen_row = saved_screen->start_row + r;

	if (screen_row >= 0 && screen_row < screen_Rows)
	    for (int c = 0; c < saved_screen->cols; c++)
	    {
		int screen_col = saved_screen->start_col + c;

		if (screen_col >= 0 && screen_col < screen_Columns)
		{
		    int off = LineOffset[screen_row] + screen_col;
		    int save_off = r * saved_screen->cols + c;

		    saved_screen->lines[save_off] = ScreenLines[off];
		    saved_screen->attrs[save_off] = ScreenAttrs[off];
		    if (enc_utf8 && saved_screen->linesuc != NULL)
			saved_screen->linesuc[save_off] = ScreenLinesUC[off];
		}
	    }
    }
}

/*
 * Force windows under a popup area to redraw.
 */
    static void
redraw_under_popup_area(int winrow, int wincol, int height, int width, int leftoff)
{
    int	    r;

    for (r = winrow; r < winrow + height && r < screen_Rows; ++r)
    {
	int	    c;
	win_T	    *prev_twp = NULL;

	if (r >= cmdline_row)
	{
	    clear_cmdline = TRUE;
	    continue;
	}

	for (c = wincol; c < wincol + width - leftoff && c < screen_Columns; ++c)
	{
	    int	    line_cp = r;
	    int	    col_cp = c;
	    win_T   *twp;

	    twp = mouse_find_win(&line_cp, &col_cp, IGNORE_POPUP);
	    if (twp != NULL && twp != prev_twp)
	    {
		prev_twp = twp;
		if (line_cp < twp->w_height)
		{
		    linenr_T lnum;

		    (void)mouse_comp_pos(twp, &line_cp, &col_cp, &lnum, NULL);
		    redrawWinline(twp, lnum);
		}
		else if (line_cp == twp->w_height)
		    twp->w_redr_status = true;
	    }
	}
    }
}

/*
 * popup_move({id}, {options})
 */
    void
f_popup_move(typval_T *argvars, typval_T *rettv UNUSED)
{
    dict_T	*dict;
    int		id;
    win_T	*wp;
    popup_area_T	old_area;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_dict_arg(argvars, 1) == FAIL))
	return;

    id = (int)tv_get_number(argvars);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;  // invalid {id}

    if (check_for_nonnull_dict_arg(argvars, 1) == FAIL)
	return;
    dict = argvars[1].vval.v_dict;

    popup_save_area(wp, &old_area);

    apply_move_options(wp, dict);

    if (wp->w_winrow + wp->w_height >= cmdline_row)
	clear_cmdline = TRUE;
    popup_adjust_position(wp);

    // Redraw the popup at the new position; for opaque popups, the
    // diff-based popup mask update in may_update_popup_mask() will handle
    // redrawing the affected lines in regular windows to clear the old
    // position.  Transparent popups don't participate in popup_mask, so
    // we need to manually mark the old area's lines for redraw.
    if (popup_area_changed(wp, &old_area))
    {
	redraw_win_later(wp, UPD_NOT_VALID);

	popup_redraw_exposed_area(&old_area);
    }
}

/*
 * popup_setoptions({id}, {options})
 */
    void
f_popup_setoptions(typval_T *argvars, typval_T *rettv UNUSED)
{
    dict_T	*dict;
    int		id;
    win_T	*wp;
    popup_area_T	old_area;
    popup_style_snapshot_T old_style;
    int		need_redraw = FALSE;
    int		need_reposition = FALSE;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_dict_arg(argvars, 1) == FAIL))
	return;

    id = (int)tv_get_number(argvars);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;  // invalid {id}

    if (check_for_nonnull_dict_arg(argvars, 1) == FAIL)
	return;
    dict = argvars[1].vval.v_dict;
    popup_save_area(wp, &old_area);
    popup_save_style(wp, &old_style);

    (void)apply_options(wp, dict, FALSE);

    // Keep "firstline" sticky across popup_setoptions(): when it is set, any
    // property update should reapply it and restore the displayed top line.
    if (wp->w_firstline > 0
	    && wp->w_firstline <= wp->w_buffer->b_ml.ml_line_count)
	wp->w_topline = wp->w_firstline;

    // Check if visual options changed and redraw if needed
    if (old_style.zindex != wp->w_zindex)
    {
	need_redraw = TRUE;
	need_reposition = TRUE;
    }
    else if (popup_style_changed(wp, &old_style))
	need_redraw = TRUE;

    if (old_style.flags != wp->w_popup_flags)
	need_reposition = TRUE;

    if (need_reposition)
    {
	redraw_win_later(wp, UPD_NOT_VALID);
	popup_mask_refresh = TRUE;
    }
    else if (need_redraw)
    {
	// Only content changed (e.g. firstline, highlight): redraw the
	// popup window without updating the popup mask or triggering a
	// full screen redraw.  This avoids flickering of windows behind
	// the popup.
	wp->w_redr_type = UPD_NOT_VALID;
	wp->w_lines_valid = 0;
	if (must_redraw < UPD_VALID)
	    must_redraw = UPD_VALID;
    }

    // Force redraw if opacity value changed
    if (old_style.blend != wp->w_popup_blend)
    {
	redraw_win_later(wp, UPD_NOT_VALID);
	// Also redraw windows below the popup
	redraw_all_later(UPD_NOT_VALID);
	popup_mask_refresh = TRUE;
    }

    // Always recalculate popup position/size: other options like border,
    // close, padding may have changed without affecting w_popup_flags.
    // popup_adjust_position() only sets popup_mask_refresh when the
    // position or size actually changed.
    popup_adjust_position(wp);

    if (popup_area_changed(wp, &old_area))
	popup_redraw_exposed_area(&old_area);
}

/*
 * popup_getpos({id})
 */
    void
f_popup_getpos(typval_T *argvars, typval_T *rettv)
{
    dict_T	*dict;
    int		id;
    win_T	*wp;
    int		top_extra;
    int		left_extra;

    if (rettv_dict_alloc(rettv) == FAIL)
	return;

    if (in_vim9script() && check_for_number_arg(argvars, 0) == FAIL)
	return;

    id = (int)tv_get_number(argvars);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;  // invalid {id}
    top_extra = popup_top_extra(wp);
    left_extra = wp->w_popup_border[3] + wp->w_popup_padding[3];

    // we know how much space we need, avoid resizing halfway
    dict = rettv->vval.v_dict;
    hash_lock_size(&dict->dv_hashtab, 11);

    dict_add_number(dict, "line", wp->w_winrow + 1);
    dict_add_number(dict, "col", wp->w_wincol + 1);
    dict_add_number(dict, "width", wp->w_width + left_extra
	    + wp->w_popup_border[1] + wp->w_popup_padding[1]);
    dict_add_number(dict, "height", wp->w_height + top_extra
	    + wp->w_popup_border[2] + wp->w_popup_padding[2]);

    dict_add_number(dict, "core_line", wp->w_winrow + 1 + top_extra);
    dict_add_number(dict, "core_col", wp->w_wincol + 1 + left_extra);
    dict_add_number(dict, "core_width", wp->w_width);
    dict_add_number(dict, "core_height", wp->w_height);

    dict_add_number(dict, "scrollbar", wp->w_has_scrollbar);
    dict_add_number(dict, "firstline", wp->w_topline);
    dict_add_number(dict, "lastline", wp->w_botline - 1);
    dict_add_number(dict, "visible",
	    win_valid(wp) && (wp->w_popup_flags & POPF_HIDDEN) == 0);

    hash_unlock(&dict->dv_hashtab);
}

/*
 * popup_list()
 */
    void
f_popup_list(typval_T *argvars UNUSED, typval_T *rettv)
{
    win_T	*wp;
    tabpage_T	*tp;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    FOR_ALL_POPUPWINS(wp)
	list_append_number(rettv->vval.v_list, wp->w_id);
    FOR_ALL_TABPAGES(tp)
	FOR_ALL_POPUPWINS_IN_TAB(tp, wp)
	    list_append_number(rettv->vval.v_list, wp->w_id);
}

/*
 * popup_locate({row}, {col})
 */
    void
f_popup_locate(typval_T *argvars, typval_T *rettv)
{
    int		row;
    int		col;
    win_T	*wp;

    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_number_arg(argvars, 1) == FAIL))
	return;

    row = tv_get_number(&argvars[0]) - 1;
    col = tv_get_number(&argvars[1]) - 1;
    wp = mouse_find_win(&row, &col, FIND_POPUP);
    if (wp != NULL && WIN_IS_POPUP(wp))
	rettv->vval.v_number = wp->w_id;
}

/*
 * For popup_getoptions(): add a "border" or "padding" entry to "dict".
 */
    static void
get_padding_border(dict_T *dict, int *array, char *name)
{
    list_T  *list;
    int	    i;

    if (array[0] == 0 && array[1] == 0 && array[2] == 0 && array[3] == 0)
	return;

    list = list_alloc();
    if (list == NULL)
	return;

    dict_add_list(dict, name, list);
    if (array[0] != 1 || array[1] != 1 || array[2] != 1 || array[3] != 1)
	for (i = 0; i < 4; ++i)
	    list_append_number(list, array[i]);
}

/*
 * For popup_getoptions(): add a "borderhighlight" entry to "dict".
 */
    static void
get_borderhighlight(dict_T *dict, win_T *wp)
{
    list_T  *list;
    int	    i;

    for (i = 0; i < 4; ++i)
	if (wp->w_border_highlight[i] != NULL)
	    break;
    // Only include "borderhighlight" if it was explicitly set (even if empty)
    // or if at least one highlight is set.
    if (i == 4 && !wp->w_border_highlight_isset)
	return;

    list = list_alloc();
    if (list == NULL)
	return;

    dict_add_list(dict, "borderhighlight", list);
    // When all highlights are NULL (cleared to empty list), return empty list.
    if (i == 4)
	return;
    for (i = 0; i < 4; ++i)
	list_append_string(list, wp->w_border_highlight[i], -1);
}

/*
 * For popup_getoptions(): add a "borderchars" entry to "dict".
 */
    static void
get_borderchars(dict_T *dict, win_T *wp)
{
    list_T  *list;
    int	    i;
    char_u  buf[NUMBUFLEN];
    int	    len;

    for (i = 0; i < 8; ++i)
	if (wp->w_border_char[i] != 0)
	    break;
    if (i == 8)
	return;

    list = list_alloc();
    if (list == NULL)
	return;

    dict_add_list(dict, "borderchars", list);
    for (i = 0; i < 8; ++i)
    {
	len = mb_char2bytes(wp->w_border_char[i], buf);
	list_append_string(list, buf, len);
    }
}

/*
 * For popup_getoptions(): add a "moved" and "mousemoved" entry to "dict".
 */
    static void
get_moved_list(dict_T *dict, win_T *wp)
{
    list_T  *list;

    list = list_alloc();
    if (list != NULL)
    {
	dict_add_list(dict, "moved", list);
	list_append_number(list, wp->w_popup_lnum);
	list_append_number(list, wp->w_popup_mincol);
	list_append_number(list, wp->w_popup_maxcol);
    }
    list = list_alloc();
    if (list == NULL)
	return;

    dict_add_list(dict, "mousemoved", list);
    list_append_number(list, wp->w_popup_mouse_row);
    list_append_number(list, wp->w_popup_mouse_mincol);
    list_append_number(list, wp->w_popup_mouse_maxcol);
}

/*
 * popup_getoptions({id})
 */
    void
f_popup_getoptions(typval_T *argvars, typval_T *rettv)
{
    dict_T	*dict;
    int		id;
    win_T	*wp;
    tabpage_T	*tp;
    int		i;

    if (rettv_dict_alloc(rettv) == FAIL)
	return;

    if (in_vim9script() && check_for_number_arg(argvars, 0) == FAIL)
	return;

    id = (int)tv_get_number(argvars);
    wp = find_popup_win(id);
    if (wp == NULL)
	return;

    dict = rettv->vval.v_dict;
    dict_add_number(dict, "line", wp->w_wantline);
    dict_add_number(dict, "col", wp->w_wantcol);
    dict_add_number(dict, "minwidth", wp->w_minwidth);
    dict_add_number(dict, "minheight", wp->w_minheight);
    dict_add_number(dict, "maxheight", wp->w_maxheight);
    dict_add_number(dict, "maxwidth", wp->w_maxwidth);
    dict_add_number(dict, "firstline", wp->w_firstline);
    dict_add_number(dict, "scrollbar", wp->w_want_scrollbar);
    dict_add_number(dict, "zindex", wp->w_zindex);
    dict_add_number(dict, "fixed", wp->w_popup_fixed);
    if (wp->w_popup_prop_type && win_valid_any_tab(wp->w_popup_prop_win))
    {
	proptype_T *pt = text_prop_type_by_id(
		wp->w_popup_prop_win->w_buffer,
		wp->w_popup_prop_type);

	if (pt != NULL)
	    dict_add_string(dict, "textprop", pt->pt_name);
	dict_add_number(dict, "textpropwin", wp->w_popup_prop_win->w_id);
	dict_add_number(dict, "textpropid", wp->w_popup_prop_id);
    }
    dict_add_string(dict, "title", wp->w_popup_title);
    dict_add_number(dict, "wrap", wp->w_p_wrap);
    dict_add_number(dict, "drag", (wp->w_popup_flags & POPF_DRAG) != 0);
    dict_add_number(dict, "dragall",
	    (wp->w_popup_flags & POPF_DRAGALL) != 0);
    dict_add_number(dict, "mapping",
	    (wp->w_popup_flags & POPF_MAPPING) != 0);
    dict_add_number(dict, "resize", (wp->w_popup_flags & POPF_RESIZE) != 0);
    dict_add_number(dict, "posinvert",
	    (wp->w_popup_flags & POPF_POSINVERT) != 0);
    dict_add_number(dict, "clipwindow",
	    (wp->w_popup_flags & POPF_CLIPWINDOW) != 0);
    // Return opacity (0-100) by converting from internal blend value
    dict_add_number(dict, "opacity",
	    (wp->w_popup_flags & POPF_OPACITY) ? 100 - wp->w_popup_blend : 100);
    dict_add_number(dict, "cursorline",
	    (wp->w_popup_flags & POPF_CURSORLINE) != 0);
    dict_add_string(dict, "highlight", syn_id2name(hlf_get_id(wp, HLF_WIN)));
    dict_add_string(dict, "highlights", wp->w_p_whl);
    if (wp->w_scrollbar_highlight != NULL)
	dict_add_string(dict, "scrollbarhighlight",
		wp->w_scrollbar_highlight);
    if (wp->w_thumb_highlight != NULL)
	dict_add_string(dict, "thumbhighlight", wp->w_thumb_highlight);

    // find the tabpage that holds this popup
    i = 1;
    FOR_ALL_TABPAGES(tp)
    {
	win_T *twp;

	FOR_ALL_POPUPWINS_IN_TAB(tp, twp)
	    if (twp->w_id == id)
		break;
	if (twp != NULL)
	    break;
	++i;
    }
    if (tp == NULL)
	i = -1;  // must be global
    else if (tp == curtab)
	i = 0;
    dict_add_number(dict, "tabpage", i);

    get_padding_border(dict, wp->w_popup_padding, "padding");
    get_padding_border(dict, wp->w_popup_border, "border");
    get_borderhighlight(dict, wp);
    get_borderchars(dict, wp);
    get_moved_list(dict, wp);

    if (wp->w_filter_cb.cb_name != NULL)
	dict_add_callback(dict, "filter", &wp->w_filter_cb);
    if (wp->w_close_cb.cb_name != NULL)
	dict_add_callback(dict, "callback", &wp->w_close_cb);

    for (i = 0; i < (int)ARRAY_LENGTH(poppos_entries); ++i)
	if (wp->w_popup_pos == poppos_entries[i].pp_val)
	{
	    dict_add_string_len(dict, "pos",
		poppos_entries[i].pp_name.string,
		(int)poppos_entries[i].pp_name.length);
	    break;
	}

    if (wp->w_popup_close == POPCLOSE_BUTTON)
	dict_add_string_len(dict, "close", (char_u *)"button", STRLEN_LITERAL("button"));
    else if (wp->w_popup_close == POPCLOSE_CLICK)
	dict_add_string_len(dict, "close", (char_u *)"click", STRLEN_LITERAL("click"));
    else
	dict_add_string_len(dict, "close", (char_u *)"none", STRLEN_LITERAL("none"));

#if defined(FEAT_TIMERS)
    dict_add_number(dict, "time", wp->w_popup_timer != NULL
	    ?  (long)wp->w_popup_timer->tr_interval : 0L);
#endif
}

#if defined(FEAT_TERMINAL)
/*
 * Return TRUE if the current window is running a terminal in a popup window.
 * Return FALSE when the job has ended.
 */
    int
error_if_term_popup_window(void)
{
    if (WIN_IS_POPUP(curwin) && curbuf->b_term != NULL
					   && term_job_running(curbuf->b_term))
    {
	emsg(_(e_not_allowed_for_terminal_in_popup_window));
	return TRUE;
    }
    return FALSE;
}
#endif

/*
 * Reset all the "handled_flag" flags in global popup windows and popup windows
 * in the current tab page.
 * Each calling function should use a different flag, see the list at
 * POPUP_HANDLED_1.  This won't work with recursive calls though.
 */
    void
popup_reset_handled(int handled_flag)
{
    win_T *wp;

    FOR_ALL_POPUPWINS(wp)
	wp->w_popup_handled &= ~handled_flag;
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	wp->w_popup_handled &= ~handled_flag;
}

/*
 * Find the next visible popup where "handled_flag" is not set.
 * Must have called popup_reset_handled() first.
 * When "lowest" is TRUE find the popup with the lowest zindex, otherwise the
 * popup with the highest zindex.
 */
    win_T *
find_next_popup(int lowest, int handled_flag)
{
    win_T   *wp;
    win_T   *found_wp;
    int	    found_zindex;

    found_zindex = lowest ? INT_MAX : 0;
    found_wp = NULL;
    FOR_ALL_POPUPWINS(wp)
	if ((wp->w_popup_handled & handled_flag) == 0
		&& (wp->w_popup_flags & POPF_HIDDEN) == 0
		&& (lowest ? wp->w_zindex < found_zindex
		    : wp->w_zindex > found_zindex))
	{
	    found_zindex = wp->w_zindex;
	    found_wp = wp;
	}
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	if ((wp->w_popup_handled & handled_flag) == 0
		&& (wp->w_popup_flags & POPF_HIDDEN) == 0
		&& (lowest ? wp->w_zindex < found_zindex
		    : wp->w_zindex > found_zindex))
	{
	    found_zindex = wp->w_zindex;
	    found_wp = wp;
	}

    if (found_wp != NULL)
	found_wp->w_popup_handled |= handled_flag;
    return found_wp;
}

/*
 * Invoke the filter callback for window "wp" with typed character "c".
 * Uses the global "mod_mask" for modifiers.
 * Returns the return value of the filter or -1 for CTRL-C in the current
 * window.
 * Careful: The filter may make "wp" invalid!
 */
    static int
invoke_popup_filter(win_T *wp, int c)
{
    int		res;
    typval_T	rettv;
    typval_T	argv[3];
    char_u	buf[NUMBUFLEN];
    linenr_T	old_lnum = wp->w_cursor.lnum;
    int		prev_did_emsg = did_emsg;

    // Emergency exit: CTRL-C closes the popup.
    if (c == Ctrl_C)
    {
	int save_got_int = got_int;
	int was_curwin = wp == curwin;

	// Reset got_int to avoid the callback isn't called.
	got_int = FALSE;
	popup_close_with_retval(wp, -1);
	got_int |= save_got_int;

	// If the popup is the current window it probably fails to close.  Then
	// do not consume the key.
	if (was_curwin && wp == curwin)
	    return -1;
	return TRUE;
    }

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = (varnumber_T)wp->w_id;

    // Convert the number to a string, so that the function can use:
    //	    if a:c == "\<F2>"
    buf[special_to_buf(c, mod_mask, FALSE, buf)] = NUL;
    argv[1].v_type = VAR_STRING;
    argv[1].vval.v_string = vim_strsave(buf);

    argv[2].v_type = VAR_UNKNOWN;

    // NOTE: The callback might close the popup and make "wp" invalid.
    if (call_callback(&wp->w_filter_cb, -1, &rettv, 2, argv) == FAIL)
    {
	// Cannot call the function, close the popup to avoid that the filter
	// eats keys and the user is stuck.  Might as well eat the key.
	popup_close_with_retval(wp, -1);
	res = TRUE;
    }
    else
    {
	if (win_valid_popup(wp) && old_lnum != wp->w_cursor.lnum)
	    popup_highlight_curline(wp);

	// If an error message was given always return FALSE, so that keys are
	// not consumed and the user can type something.
	// If we get three errors in a row then close the popup.  Decrement the
	// error count by 1/10 if there are no errors, thus allowing up to 1 in
	// 10 calls to cause an error.
	if (win_valid_popup(wp) && did_emsg > prev_did_emsg)
	{
	    wp->w_filter_errors += 10;
	    if (wp->w_filter_errors >= 30)
		popup_close_with_retval(wp, -1);
	    res = FALSE;
	}
	else
	{
	    if (win_valid_popup(wp) && wp->w_filter_errors > 0)
		--wp->w_filter_errors;
	    res = tv_get_bool(&rettv);
	}
    }

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
    static int	recursive = FALSE;
    int		res = FALSE;
    win_T	*wp;
    int		save_KeyTyped = KeyTyped;
    int		state;
    int		was_must_redraw = must_redraw;

#ifdef FEAT_TERMINAL
    // Popup window with terminal always gets focus.
    if (popup_is_popup(curwin) && curbuf->b_term != NULL)
	return FALSE;
#endif

    if (recursive)
	return FALSE;
    recursive = TRUE;

    if (c == K_LEFTMOUSE)
    {
	int row = mouse_row;
	int col = mouse_col;

	wp = mouse_find_win(&row, &col, FIND_POPUP);
	if (wp != NULL && popup_close_if_on_X(wp, row, col))
	    res = TRUE;
    }

    popup_reset_handled(POPUP_HANDLED_2);
    state = get_real_state();
    while (res == FALSE
		     && (wp = find_next_popup(FALSE, POPUP_HANDLED_2)) != NULL)
	if (wp->w_filter_cb.cb_name != NULL
		&& (wp->w_filter_mode & state) != 0)
	    res = invoke_popup_filter(wp, c);

    if (must_redraw > was_must_redraw)
    {
	int save_got_int = got_int;

	// Reset got_int to avoid a function used in the statusline aborts.
	got_int = FALSE;
	redraw_after_callback(FALSE, FALSE);
	got_int |= save_got_int;
    }
    recursive = FALSE;
    KeyTyped = save_KeyTyped;

    // When interrupted return FALSE to avoid looping.
    return res == -1 ? FALSE : res;
}

/*
 * Return TRUE if there is a popup visible with a filter callback and the
 * "mapping" property off.
 */
    int
popup_no_mapping(void)
{
    int	    round;
    win_T   *wp;

    for (round = 1; round <= 2; ++round)
	for (wp = round == 1 ? first_popupwin : curtab->tp_first_popupwin;
						   wp != NULL; wp = wp->w_next)
	    if (wp->w_filter_cb.cb_name != NULL
		    && (wp->w_popup_flags & (POPF_HIDDEN | POPF_MAPPING)) == 0)
		return TRUE;
    return FALSE;
}

/*
 * Called when the cursor moved: check if any popup needs to be closed if the
 * cursor moved far enough.
 */
    void
popup_check_cursor_pos(void)
{
    win_T *wp;

    popup_reset_handled(POPUP_HANDLED_3);
    while ((wp = find_next_popup(TRUE, POPUP_HANDLED_3)) != NULL)
	if (wp->w_popup_curwin != NULL
		&& (curwin != wp->w_popup_curwin
		    || curwin->w_cursor.lnum != wp->w_popup_lnum
		    || curwin->w_cursor.col < wp->w_popup_mincol
		    || curwin->w_cursor.col > wp->w_popup_maxcol))
	    popup_close_with_retval(wp, -1);
}

/*
 * Update "w_popup_mask_cells".
 */
    static void
popup_update_mask(win_T *wp, int width, int height)
{
    listitem_T	*lio, *li;
    char_u	*cells;
    int		row, col;

    if (wp->w_popup_mask == NULL || width == 0 || height == 0)
    {
	VIM_CLEAR(wp->w_popup_mask_cells);
	return;
    }
    if (wp->w_popup_mask_cells != NULL
	    && wp->w_popup_mask_height == height
	    && wp->w_popup_mask_width == width)
	return;  // cache is still valid

    vim_free(wp->w_popup_mask_cells);
    if (width > 0 && (size_t)height > SIZE_MAX / (size_t)width)
    {
	wp->w_popup_mask_cells = NULL;
	return;
    }
    wp->w_popup_mask_cells = alloc_clear((size_t)width * height);
    if (wp->w_popup_mask_cells == NULL)
	return;
    cells = wp->w_popup_mask_cells;

    FOR_ALL_LIST_ITEMS(wp->w_popup_mask, lio)
    {
	int cols, cole;
	int lines, linee;

	li = lio->li_tv.vval.v_list->lv_first;
	cols = tv_get_number(&li->li_tv);
	if (cols < 0)
	    cols = width + cols + 1;
	if (cols <= 0)
	    cols = 1;
	li = li->li_next;
	cole = tv_get_number(&li->li_tv);
	if (cole < 0)
	    cole = width + cole + 1;
	if (cole > width)
	    cole = width;
	li = li->li_next;
	lines = tv_get_number(&li->li_tv);
	if (lines < 0)
	    lines = height + lines + 1;
	if (lines <= 0)
	    lines = 1;
	li = li->li_next;
	linee = tv_get_number(&li->li_tv);
	if (linee < 0)
	    linee = height + linee + 1;
	if (linee > height)
	    linee = height;

	for (row = lines - 1; row < linee; ++row)
	    for (col = cols - 1; col < cole; ++col)
		cells[row * width + col] = 1;
    }
}

/*
 * Return TRUE if "col" / "line" matches with an entry in w_popup_mask.
 * "col" and "line" are screen coordinates.
 */
    static int
popup_masked(win_T *wp, int width, int height, int screencol, int screenline)
{
    int col = screencol - wp->w_wincol + wp->w_popup_leftoff;
    int line = screenline - wp->w_winrow;

    return col >= 0 && col < width
	    && line >= 0 && line < height
	    && wp->w_popup_mask_cells[line * width + col];
}

/*
 * Set flags in popup_transparent[] for window "wp" to "val".
 */
    static void
update_popup_transparent(win_T *wp, int val)
{
    if (wp->w_popup_mask == NULL)
	return;

    int		width = popup_width(wp);
    int		height = popup_height(wp);
    listitem_T	*lio, *li;
    int		cols, cole;
    int		lines, linee;
    int		col, line;

    FOR_ALL_LIST_ITEMS(wp->w_popup_mask, lio)
    {
	li = lio->li_tv.vval.v_list->lv_first;
	cols = tv_get_number(&li->li_tv);
	if (cols < 0)
	    cols = width + cols + 1;
	li = li->li_next;
	cole = tv_get_number(&li->li_tv);
	if (cole < 0)
	    cole = width + cole + 1;
	li = li->li_next;
	lines = tv_get_number(&li->li_tv);
	if (lines < 0)
	    lines = height + lines + 1;
	li = li->li_next;
	linee = tv_get_number(&li->li_tv);
	if (linee < 0)
	    linee = height + linee + 1;

	--cols;
	cols -= wp->w_popup_leftoff;
	if (cols < 0)
	    cols = 0;
	cole -= wp->w_popup_leftoff;
	--lines;
	if (lines < 0)
	    lines = 0;
	for (line = lines; line < linee
		&& line + wp->w_winrow < screen_Rows; ++line)
	    for (col = cols; col < cole
		    && col + wp->w_wincol < screen_Columns; ++col)
		popup_transparent[(line + wp->w_winrow) * screen_Columns
		    + col + wp->w_wincol] = val;
    }
}

/*
 * Only called when popup window "wp" is hidden: If the window is positioned
 * next to a text property, and it is now visible, then  unhide the popup.
 * We don't check if visible popups become hidden, that is done in
 * popup_adjust_position().
 * Return TRUE if the popup became unhidden.
 */
    static int
check_popup_unhidden(win_T *wp)
{
    if (wp->w_popup_prop_type > 0 && win_valid(wp->w_popup_prop_win))
    {
	textprop_T  prop;
	linenr_T    lnum;
	bool	    found = false;

	if ((wp->w_popup_flags & POPF_HIDDEN_FORCE) != 0)
	    return FALSE;
	if (find_visible_prop(wp->w_popup_prop_win,
				wp->w_popup_prop_type, wp->w_popup_prop_id,
						       &prop, &lnum))
	    found = true;
	// The textprop may have scrolled just above the host window's top.
	// Unhide the popup so popup_adjust_position() can roll it partially
	// onto the host's top edge via the top-clip path.  Limit the search
	// to the popup's own height so we do not resurrect a popup whose
	// prop is already further off-screen than the popup can extend.
	else if (popup_find_prop_above_top(wp, wp->w_popup_prop_win,
					    popup_height(wp), &prop, &lnum))
	    found = true;
	if (found)
	{
	    wp->w_popup_flags &= ~POPF_HIDDEN;
	    wp->w_popup_prop_topline = 0; // force repositioning
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 * Return TRUE if popup_adjust_position() needs to be called for "wp".
 * That is when the buffer in the popup was changed, or the popup is following
 * a textprop and the referenced buffer was changed.
 * Or when the cursor line changed and "cursorline" is set.
 */
    static int
popup_need_position_adjust(win_T *wp)
{
    if (wp->w_popup_last_changedtick != CHANGEDTICK(wp->w_buffer))
	return TRUE;
    if (win_valid(wp->w_popup_prop_win)
	    && (wp->w_popup_prop_changedtick
				 != CHANGEDTICK(wp->w_popup_prop_win->w_buffer)
	       || wp->w_popup_prop_topline != wp->w_popup_prop_win->w_topline
	       || wp->w_popup_prop_winrow != wp->w_popup_prop_win->w_winrow
	       || wp->w_popup_prop_wincol != wp->w_popup_prop_win->w_wincol
	       || wp->w_popup_prop_width != wp->w_popup_prop_win->w_width
	       || wp->w_popup_prop_winheight != wp->w_popup_prop_win->w_height))
	return TRUE;

    // May need to adjust the width if the cursor moved.
    return wp->w_cursor.lnum != wp->w_popup_last_curline;
}

// Cached array with max zindex of opacity popups covering each cell.
// Allocated in may_update_popup_mask() when opacity popups exist.
static short *opacity_zindex = NULL;
static int    opacity_zindex_rows = 0;
static int    opacity_zindex_cols = 0;

/*
 * Mark cells covered by opacity popup "wp" in opacity_zindex[].
 * Stores the maximum zindex so that lower popups can be suppressed too.
 */
    static void
popup_mark_opacity_zindex(win_T *wp)
{
    int	    width;
    int	    height;
    int	    r, c;

    if (!(wp->w_popup_flags & POPF_OPACITY) || wp->w_popup_blend <= 0
	    || (wp->w_popup_flags & POPF_HIDDEN))
	return;

    width = popup_width(wp);
    height = popup_height(wp);
    for (r = wp->w_winrow;
		       r < wp->w_winrow + height && r < screen_Rows; ++r)
	for (c = wp->w_wincol;
		 c < wp->w_wincol + width - wp->w_popup_leftoff
						&& c < screen_Columns; ++c)
	{
	    int off = r * screen_Columns + c;
	    if (wp->w_zindex > opacity_zindex[off])
		opacity_zindex[off] = wp->w_zindex;
	}
}

/*
 * Force background windows to redraw rows under an opacity popup.
 */
    static void
redraw_win_under_opacity_popup(win_T *wp)
{
    int	    height;
    int	    width;
    int	    r;

    if (!(wp->w_popup_flags & POPF_OPACITY) || wp->w_popup_blend <= 0
	    || (wp->w_popup_flags & POPF_HIDDEN))
	return;

    height = popup_height(wp);
    width = popup_width(wp);
    for (r = wp->w_winrow;
		       r < wp->w_winrow + height && r < screen_Rows; ++r)
    {
	int	    col;
	win_T	    *prev_twp = NULL;

	// Check across the full width of the popup to find all underlying
	// windows (e.g., when the popup spans a vertical split).
	for (col = wp->w_wincol;
		       col < wp->w_wincol + width && col < screen_Columns; ++col)
	{
	    int	    line_cp = r;
	    int	    col_cp = col;
	    win_T   *twp;

	    twp = mouse_find_win(&line_cp, &col_cp, IGNORE_POPUP);
	    if (twp != NULL && twp != prev_twp)
	    {
		prev_twp = twp;
		if (line_cp < twp->w_height)
		{
		    linenr_T lnum;

		    (void)mouse_comp_pos(twp, &line_cp, &col_cp, &lnum, NULL);
		    // Called from inside update_screen(); raising must_redraw
		    // would loop the outer redraw indefinitely.
		    redraw_win_range_now(twp, lnum, lnum);
		}
		else if (line_cp == twp->w_height)
		    // Status bar line: mark for redraw to prevent
		    // opacity blend accumulation.
		    twp->w_redr_status = true;
	    }
	}
    }
}


/*
 * Return TRUE if cell (row, col) is covered by a higher-zindex opacity popup.
 */
    int
popup_is_under_opacity(int row, int col)
{
    if (opacity_zindex == NULL
	    || row < 0 || row >= opacity_zindex_rows
	    || col < 0 || col >= opacity_zindex_cols)
	return FALSE;
    return opacity_zindex[row * opacity_zindex_cols + col] > screen_zindex;
}

/*
 * Return TRUE if cell (row, col) is covered by a lower-zindex opacity popup.
 */
    int
popup_is_over_opacity(int row, int col)
{
    win_T *wp;

    FOR_ALL_POPUPWINS(wp)
	if ((wp->w_popup_flags & POPF_OPACITY)
		&& wp->w_popup_blend > 0
		&& !(wp->w_popup_flags & POPF_HIDDEN)
		&& wp->w_zindex < screen_zindex
		&& row >= wp->w_winrow
		&& row < wp->w_winrow + popup_height(wp)
		&& col >= wp->w_wincol
		&& col < wp->w_wincol + popup_width(wp))
	    return TRUE;
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	if ((wp->w_popup_flags & POPF_OPACITY)
		&& wp->w_popup_blend > 0
		&& !(wp->w_popup_flags & POPF_HIDDEN)
		&& wp->w_zindex < screen_zindex
		&& row >= wp->w_winrow
		&& row < wp->w_winrow + popup_height(wp)
		&& col >= wp->w_wincol
		&& col < wp->w_wincol + popup_width(wp))
	    return TRUE;
    return FALSE;
}

/*
 * Return TRUE if any cell in row "row" from "start_col" to "end_col"
 * (exclusive) is covered by a higher-zindex opacity popup.
 */
    int
popup_is_under_opacity_range(int row, int start_col, int end_col)
{
    int col;

    if (opacity_zindex == NULL
	    || row < 0 || row >= opacity_zindex_rows)
	return FALSE;
    for (col = start_col; col < end_col && col < opacity_zindex_cols; ++col)
	if (opacity_zindex[row * opacity_zindex_cols + col] > screen_zindex)
	    return TRUE;
    return FALSE;
}

/*
 * Update "popup_mask" if needed.
 * Also recomputes the popup size and positions.
 * Also updates "popup_visible" and "popup_uses_mouse_move".
 * Also marks window lines for redrawing.
 */
    void
may_update_popup_mask(int type)
{
    win_T	*wp;
    short	*mask;
    int		line, col;
    int		redraw_all_popups = FALSE;
    int		redrawing_all_win;

    // Need to recompute when switching tabs.
    // Also recompute when the type is UPD_CLEAR or UPD_NOT_VALID, something
    // basic (such as the screen size) must have changed.
    if (popup_mask_tab != curtab || type >= UPD_NOT_VALID)
    {
	popup_mask_refresh = TRUE;
	redraw_all_popups = TRUE;
    }

    // Check if any popup window buffer has changed and if any popup connected
    // to a text property has become visible.
    FOR_ALL_POPUPWINS(wp)
	if (wp->w_popup_flags & POPF_HIDDEN)
	    popup_mask_refresh |= check_popup_unhidden(wp);
	else if (popup_need_position_adjust(wp))
	    popup_mask_refresh = TRUE;
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	if (wp->w_popup_flags & POPF_HIDDEN)
	    popup_mask_refresh |= check_popup_unhidden(wp);
	else if (popup_need_position_adjust(wp))
	    popup_mask_refresh = TRUE;

    // Force background windows to redraw rows under opacity popups.
    // Opacity popups don't participate in popup_mask, so their area
    // wouldn't normally be redrawn.  Without this, ScreenAttrs retains
    // blended values from the previous cycle, causing blend accumulation.
    // This must run every cycle, not just when popup_mask_refresh is set.
    //
    // Also build the opacity_zindex array used by screen_char() to suppress
    // output for cells under opacity popups during background draw.
    {
	int has_opacity = FALSE;

	FOR_ALL_POPUPWINS(wp)
	{
	    redraw_win_under_opacity_popup(wp);
	    if ((wp->w_popup_flags & POPF_OPACITY)
		    && wp->w_popup_blend > 0
		    && !(wp->w_popup_flags & POPF_HIDDEN))
		has_opacity = TRUE;
	}
	FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	{
	    redraw_win_under_opacity_popup(wp);
	    if ((wp->w_popup_flags & POPF_OPACITY)
		    && wp->w_popup_blend > 0
		    && !(wp->w_popup_flags & POPF_HIDDEN))
		has_opacity = TRUE;
	}

	if (!has_opacity)
	{
	    VIM_CLEAR(opacity_zindex);
	    opacity_zindex_rows = 0;
	    opacity_zindex_cols = 0;
	}
	else
	{
	    if (opacity_zindex_rows != screen_Rows
		    || opacity_zindex_cols != screen_Columns)
	    {
		vim_free(opacity_zindex);
		opacity_zindex = LALLOC_MULT(short,
					screen_Rows * screen_Columns);
		opacity_zindex_rows = screen_Rows;
		opacity_zindex_cols = screen_Columns;
	    }
	    if (opacity_zindex != NULL)
	    {
		vim_memset(opacity_zindex, 0,
		    (size_t)screen_Rows * screen_Columns * sizeof(short));

		FOR_ALL_POPUPWINS(wp)
		    popup_mark_opacity_zindex(wp);
		FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
		    popup_mark_opacity_zindex(wp);
	    }
	}
    }

    if (!popup_mask_refresh)
	return;

    // Need to update the mask, something has changed.
    popup_mask_refresh = FALSE;
    popup_mask_tab = curtab;
    popup_visible = FALSE;

    // If redrawing all windows, just update "popup_mask".
    // If redrawing only what is needed, update "popup_mask_next" and then
    // compare with "popup_mask" to see what changed.
    redrawing_all_win = TRUE;
    FOR_ALL_WINDOWS(wp)
	if (wp->w_redr_type < UPD_SOME_VALID)
	    redrawing_all_win = FALSE;
    if (redrawing_all_win)
	mask = popup_mask;
    else
	mask = popup_mask_next;
    vim_memset(mask, 0, (size_t)screen_Rows * screen_Columns * sizeof(short));

    // Find the window with the lowest zindex that hasn't been handled yet,
    // so that the window with a higher zindex overwrites the value in
    // popup_mask.
    popup_reset_handled(POPUP_HANDLED_4);
    while ((wp = find_next_popup(TRUE, POPUP_HANDLED_4)) != NULL)
    {
	int width;
	int height;

	popup_visible = TRUE;

	// Recompute the position if the text changed.  It may make the popup
	// hidden if it's attach to a text property that is no longer visible.
	if (redraw_all_popups || popup_need_position_adjust(wp))
	{
	    popup_adjust_position(wp);
	    if (wp->w_popup_flags & POPF_HIDDEN)
		continue;
	}

	width = popup_width(wp);
	// Match the drawn extent computed by update_popups so that cells
	// outside the clipped popup are not marked as popup-owned and the
	// background window can draw through them.
	if (wp->w_popup_topoff > 0 || wp->w_popup_bottomoff > 0)
	{
	    popup_clip_T cl;

	    popup_compute_clip(wp, &cl);
	    height = cl.eff_height;
	}
	else
	    height = popup_height(wp);
	popup_update_mask(wp, width, height);

	// Popup with partial transparency do not block lower layers from
	// drawing, so they don't participate in the popup_mask.
	// Fully opaque popups (blend == 0) still block lower layers.
	if ((wp->w_popup_flags & POPF_OPACITY) && wp->w_popup_blend > 0)
	    continue;

	{
	    int mask_start = wp->w_winrow + wp->w_popup_topoff;
	    int mask_end = mask_start + height;
	    int mask_col_start = wp->w_wincol + wp->w_popup_leftclip;
	    int mask_col_end = wp->w_wincol + width - wp->w_popup_leftoff
						    - wp->w_popup_rightclip;

	    for (line = mask_start;
			    line < mask_end && line < screen_Rows; ++line)
		for (col = mask_col_start;
		     col < mask_col_end && col < screen_Columns; ++col)
		    if (wp->w_zindex < POPUPMENU_ZINDEX
			    && pum_visible()
			    && pum_under_menu(line, col, FALSE))
			mask[line * screen_Columns + col] = POPUPMENU_ZINDEX;
		    else if (wp->w_popup_mask_cells == NULL
				    || !popup_masked(wp, width, height, col, line))
			mask[line * screen_Columns + col] = wp->w_zindex;
	}
    }

    // Only check which lines are to be updated if not already
    // updating all lines.
    if (mask == popup_mask_next)
    {
	int	    *plines_cache = ALLOC_CLEAR_MULT(int, Rows);
	win_T	    *prev_wp = NULL;

	for (line = 0; line < screen_Rows; ++line)
	{
	    int	    col_done = 0;

	    for (col = 0; col < screen_Columns; ++col)
	    {
		int off = line * screen_Columns + col;

		if (popup_mask[off] != popup_mask_next[off])
		{
		    popup_mask[off] = popup_mask_next[off];

		    if (line >= cmdline_row)
		    {
			// the command line needs to be cleared if text below
			// the popup is now visible.
			if (!msg_scrolled && popup_mask_next[off] == 0)
			    clear_cmdline = TRUE;
		    }
		    else if (col >= col_done)
		    {
			linenr_T	lnum;
			int		line_cp = line;
			int		col_cp = col;

			// The screen position "line" / "col" needs to be
			// redrawn.  Figure out what window that is and update
			// w_redraw_top and w_redraw_bot.  Only needs to be
			// done once for each window line.
			wp = mouse_find_win(&line_cp, &col_cp, IGNORE_POPUP);
			if (wp != NULL)
			{
#if defined(FEAT_TERMINAL)
			    // A terminal window needs to be redrawn.
			    if (bt_terminal(wp->w_buffer))
				redraw_win_later(wp, UPD_NOT_VALID);
			    else
#endif
			    {
				if (wp != prev_wp)
				{
				    vim_memset(plines_cache, 0,
							   sizeof(int) * Rows);
				    prev_wp = wp;
				}

				if (line_cp >= wp->w_height)
				    // In (or below) status line
				    wp->w_redr_status = true;
				else
				{
				    // compute the position in the buffer line
				    // from the position in the window
				    (void)mouse_comp_pos(wp, &line_cp, &col_cp,
							  &lnum, plines_cache);
				    redrawWinline(wp, lnum);
				}
			    }

			    // This line is going to be redrawn, no need to
			    // check until the right side of the window.
			    col_done = wp->w_wincol + wp->w_width - 1;
			}
		    }
		}
	    }
	}

	vim_free(plines_cache);
    }

    update_popup_uses_mouse_move();
}

/*
 * If the current window is a popup and something relevant changed, recompute
 * the position and size.
 */
    void
may_update_popup_position(void)
{
    if (popup_is_popup(curwin) && popup_need_position_adjust(curwin))
	popup_adjust_position(curwin);
}

#ifdef FEAT_PROP_POPUP
static schar_T *base_screenlines = NULL;
static int *base_screenattrs = NULL;
static u8char_T *base_screenlinesuc = NULL;
static int base_screen_rows = 0;
static int base_screen_cols = 0;

/*
 * Get the base screen cell saved before drawing opacity popups.
 * Returns TRUE if the cell is available.
 */
    int
popup_get_base_screen_cell(int row, int col, schar_T *linep, int *attrp,
							  u8char_T *ucp)
{
    if (base_screenlines == NULL || base_screenattrs == NULL)
	return FALSE;
    if (row < 0 || col < 0 || row >= base_screen_rows
						     || col >= base_screen_cols)
	return FALSE;

    int off = row * base_screen_cols + col;
    if (linep != NULL)
	*linep = base_screenlines[off];
    if (attrp != NULL)
	*attrp = base_screenattrs[off];
    if (ucp != NULL)
    {
	if (enc_utf8 && base_screenlinesuc != NULL)
	    *ucp = base_screenlinesuc[off];
	else
	    *ucp = 0;
    }
    return TRUE;
}

/*
 * Set the base screen cell saved before drawing opacity popups.
 * Used to update the snapshot after blending a layer.
 */
    void
popup_set_base_screen_cell(int row, int col, schar_T line, int attr, u8char_T uc)
{
    if (base_screenlines == NULL || base_screenattrs == NULL)
	return;
    if (row < 0 || col < 0 || row >= base_screen_rows
						     || col >= base_screen_cols)
	return;

    int off = row * base_screen_cols + col;
    base_screenlines[off] = line;
    base_screenattrs[off] = attr;
    if (enc_utf8 && base_screenlinesuc != NULL)
	base_screenlinesuc[off] = uc;
}
#endif

/*
 * Draw a single padding cell with opacity blending.
 * Restores background from saved data and blends with popup attribute.
 */
static void
draw_opacity_padding_cell(
	int		row,
	int		col,
	popup_saved_screen_T *saved_screen,
	int		pad_start_col,
	int		pad_end_col)
{
    int off = LineOffset[row] + col;
    int r = row - saved_screen->start_row;
    int c = col - saved_screen->start_col;

    if (r >= 0 && r < saved_screen->rows && c >= 0 && c < saved_screen->cols)
    {
	int save_off = r * saved_screen->cols + c;
	// If this is the second cell of a wide background character, blend
	// the wide character instead of overwriting it.
	if (enc_utf8 && saved_screen->linesuc != NULL)
	{
	    int base_col = col - 1;
	    int base_off = off - 1;
	    int base_save_off = save_off - 1;
	    int wide_prev = FALSE;

	    // Prefer current screen state for detecting a wide char, since the
	    // saved data may not contain a reliable right-half marker.
	    if (base_off >= 0)
	    {
		if (ScreenLinesUC != NULL
			&& ScreenLinesUC[base_off] != 0
			&& utf_char2cells(ScreenLinesUC[base_off]) == 2
			&& ScreenLines[off] == 0)
		    wide_prev = TRUE;
	    }
	    if (!wide_prev && save_off > 0)
	    {
		if (saved_screen->linesuc[save_off - 1] != 0
			&& utf_char2cells(saved_screen->linesuc[save_off - 1]) == 2
			&& saved_screen->lines[save_off] == 0)
		    wide_prev = TRUE;
	    }

	    if (wide_prev && base_col >= 0)
	    {
		// If the wide character starts outside the padding area, do not
		// overwrite it. Use the base screen cell if available.
		if (base_col < pad_start_col)
		{
		    if (ScreenLinesUC != NULL
			    && ScreenLinesUC[base_off] != 0
			    && utf_char2cells(ScreenLinesUC[base_off]) == 2)
		    {
			// The left half still has the wide char on screen.
			// Clear it to an unblended space: only the right half is
			// covered by the popup background.
			ScreenLines[base_off] = ' ';
			ScreenLinesUC[base_off] = 0;
			ScreenAttrs[base_off] = saved_screen->attrs[base_save_off];
			popup_set_base_screen_cell(row, base_col,
				ScreenLines[base_off],
				ScreenAttrs[base_off],
				ScreenLinesUC[base_off]);
			screen_char(base_off, row, base_col);

			// Draw padding in the right half.
			// Use left half's attr since the right half of a
			// wide char may have an unreliable attr value.
			ScreenLines[off] = ' ';
			ScreenAttrs[off] = saved_screen->attrs[base_save_off];
			if (enc_utf8)
			    ScreenLinesUC[off] = 0;
			int popup_attr_val =
					get_win_attr(screen_opacity_popup);
			int blend = screen_opacity_popup->w_popup_blend;
			ScreenAttrs[off] = hl_blend_attr(ScreenAttrs[off],
					popup_attr_val, blend, TRUE);
			popup_set_base_screen_cell(row, col,
				ScreenLines[off], ScreenAttrs[off],
				ScreenLinesUC[off]);
			screen_char(off, row, col);
			return;
		    }
		    // The content drawing cleared the left half to a
		    // space (wide char didn't fit at content edge),
		    // but the saved data has a wide char.  Restore it
		    // spanning both the content cell and padding cell.
		    if (base_save_off >= 0
			    && saved_screen->linesuc[base_save_off] != 0
			    && utf_char2cells(
				saved_screen->linesuc[base_save_off]) == 2
			    && ScreenLines[base_off] == ' '
			    && ScreenLinesUC[base_off] == 0)
		    {
			int popup_attr_val =
				    get_win_attr(screen_opacity_popup);
			int blend =
				    screen_opacity_popup->w_popup_blend;

			ScreenLines[base_off] =
				    saved_screen->lines[base_save_off];
			ScreenLinesUC[base_off] =
				    saved_screen->linesuc[base_save_off];
			ScreenAttrs[base_off] =
				    saved_screen->attrs[base_save_off];
			ScreenAttrs[base_off] = hl_blend_attr(
				    ScreenAttrs[base_off],
				    popup_attr_val, blend, TRUE);

			ScreenLines[off] = 0;
			ScreenLinesUC[off] = 0;
			ScreenAttrs[off] = ScreenAttrs[base_off];

			popup_set_base_screen_cell(row, base_col,
				    ScreenLines[base_off],
				    ScreenAttrs[base_off],
				    ScreenLinesUC[base_off]);
			popup_set_base_screen_cell(row, col,
				    ScreenLines[off],
				    ScreenAttrs[off],
				    ScreenLinesUC[off]);
			screen_char(base_off, row, base_col);
			return;
		    }

		    // Draw padding in the right half.
		    // Use left half's attr since the right half of a
		    // wide char may have an unreliable attr value.
		    ScreenLines[off] = ' ';
		    ScreenAttrs[off] = saved_screen->attrs[base_save_off];
		    if (enc_utf8 && ScreenLinesUC != NULL)
			ScreenLinesUC[off] = 0;
		    int popup_attr_val = get_win_attr(screen_opacity_popup);
		    int blend = screen_opacity_popup->w_popup_blend;
		    ScreenAttrs[off] = hl_blend_attr(ScreenAttrs[off],
				    popup_attr_val, blend, TRUE);
		    popup_set_base_screen_cell(row, col, ScreenLines[off],
					       ScreenAttrs[off], ScreenLinesUC[off]);
		    screen_char(off, row, col);
		    return;
		}

		// Base cell is inside the saved area, redraw the wide char.
		if (save_off > 0)
		{
		    ScreenLines[base_off] = saved_screen->lines[base_save_off];
		    ScreenAttrs[base_off] = saved_screen->attrs[base_save_off];
		    ScreenLines[off] = saved_screen->lines[save_off];
		    ScreenAttrs[off] = saved_screen->attrs[save_off];
		    ScreenLinesUC[base_off] = saved_screen->linesuc[base_save_off];
		    ScreenLinesUC[off] = saved_screen->linesuc[save_off];

		    int popup_attr_val = get_win_attr(screen_opacity_popup);
		    int blend = screen_opacity_popup->w_popup_blend;
		    ScreenAttrs[base_off] = hl_blend_attr(ScreenAttrs[base_off],
				    popup_attr_val, blend, TRUE);
		    ScreenAttrs[off] = ScreenAttrs[base_off];
		    popup_set_base_screen_cell(row, base_col, ScreenLines[base_off],
					       ScreenAttrs[base_off], ScreenLinesUC[base_off]);
		    popup_set_base_screen_cell(row, col, ScreenLines[off],
					       ScreenAttrs[off], ScreenLinesUC[off]);
		    screen_char(base_off, row, base_col);
		}
		return;
	    }
	}
	ScreenLines[off] = saved_screen->lines[save_off];
	ScreenAttrs[off] = saved_screen->attrs[save_off];
	if (enc_utf8 && saved_screen->linesuc != NULL)
	    ScreenLinesUC[off] = saved_screen->linesuc[save_off];

	// If the saved character is wide and would extend past the padding
	// area into the content area, replace with a space to avoid
	// corrupting the content.
	if (enc_utf8 && ScreenLinesUC[off] != 0
		&& utf_char2cells(ScreenLinesUC[off]) == 2
		&& col + 1 >= pad_end_col)
	{
	    ScreenLines[off] = ' ';
	    ScreenLinesUC[off] = 0;
	}

	int popup_attr_val = get_win_attr(screen_opacity_popup);
	int blend = screen_opacity_popup->w_popup_blend;
	ScreenAttrs[off] = hl_blend_attr(ScreenAttrs[off],
				popup_attr_val, blend, TRUE);
	popup_set_base_screen_cell(row, col, ScreenLines[off],
				   ScreenAttrs[off], ScreenLinesUC[off]);
	screen_char(off, row, col);
    }
}

/*
 * Fill a rectangular padding area with opacity blending.
 */
static void
fill_opacity_padding(
	int		start_row,
	int		end_row,
	int		start_col,
	int		end_col,
	popup_saved_screen_T *saved_screen)
{
    for (int pad_row = start_row; pad_row < end_row; pad_row++)
	for (int pad_col = start_col; pad_col < end_col; pad_col++)
	    draw_opacity_padding_cell(pad_row, pad_col, saved_screen,
		    start_col, end_col);
}

/*
 * Update popup windows.  They are drawn on top of normal windows.
 * "win_update" is called for each popup window, lowest zindex first.
 */
    void
update_popups(void (*win_update)(win_T *wp))
{
    win_T   *wp;
    int	    top_off;
    int	    left_extra;
    int	    total_width;
    int	    total_height;
    int	    top_padding;
    int	    popup_attr;
    int	    border_attr[4];
    int	    border_char[8];
    char_u  buf[MB_MAXBYTES];
    int	    row;
    int	    wincol;
    int	    padcol = 0;
    int	    padendcol = 0;
    int	    i;
    int	    sb_thumb_top = 0;
    int	    sb_thumb_height = 0;
    int	    attr_scroll = 0;
    int	    attr_thumb = 0;
    bool    override_success;

    // hide the cursor until redrawing is done.
    cursor_off();

    // Find the window with the lowest zindex that hasn't been updated yet,
    // so that the window with a higher zindex is drawn later, thus goes on
    // top.
    popup_reset_handled(POPUP_HANDLED_5);
#ifdef FEAT_PROP_POPUP
    if (base_screenlines != NULL)
    {
	vim_free(base_screenlines);
	base_screenlines = NULL;
    }
    if (base_screenattrs != NULL)
    {
	vim_free(base_screenattrs);
	base_screenattrs = NULL;
    }
    if (base_screenlinesuc != NULL)
    {
	vim_free(base_screenlinesuc);
	base_screenlinesuc = NULL;
    }
    base_screen_rows = 0;
    base_screen_cols = 0;
#endif
    while ((wp = find_next_popup(TRUE, POPUP_HANDLED_5)) != NULL)
    {
	int	    title_len = 0;
	int	    title_wincol;
	popup_clip_T cl;

	// Compute the clip geometry once per iteration; w_popup_*off/clip,
	// w_height, w_width, w_popup_border and w_popup_padding are stable
	// for the duration of this iteration (popup_apply_winupdate_clip()
	// mutates w_height/w_width temporarily but the result is restored
	// before any code below reads cl again).
	popup_compute_clip(wp, &cl);

	override_success = push_highlight_overrides(wp->w_hl, wp->w_hl_len);

	// This drawing uses the zindex of the popup window, so that it's on
	// top of the text but doesn't draw when another popup with higher
	// zindex is on top of the character.
	screen_zindex = wp->w_zindex;

	// Set popup with opacity context for screen drawing.
	// Only enable transparency rendering when blend > 0 (not fully opaque).
	if ((wp->w_popup_flags & POPF_OPACITY) && wp->w_popup_blend > 0)
	    screen_opacity_popup = wp;
	else
	    screen_opacity_popup = NULL;

#ifdef FEAT_PROP_POPUP
	if (screen_opacity_popup != NULL)
	{
	    if (base_screenlines != NULL)
	    {
		vim_free(base_screenlines);
		base_screenlines = NULL;
	    }
	    if (base_screenattrs != NULL)
	    {
		vim_free(base_screenattrs);
		base_screenattrs = NULL;
	    }
	    if (base_screenlinesuc != NULL)
	    {
		vim_free(base_screenlinesuc);
		base_screenlinesuc = NULL;
	    }

	    base_screen_rows = screen_Rows;
	    base_screen_cols = screen_Columns;
	    base_screenlines = ALLOC_MULT(schar_T,
				    base_screen_rows * base_screen_cols);
	    base_screenattrs = ALLOC_MULT(int,
				    base_screen_rows * base_screen_cols);
	    if (enc_utf8)
		base_screenlinesuc = ALLOC_MULT(u8char_T,
				    base_screen_rows * base_screen_cols);

	    if (base_screenlines != NULL && base_screenattrs != NULL)
	    {
		for (int r = 0; r < base_screen_rows; r++)
		{
		    int off = LineOffset[r];
		    int base_off = r * base_screen_cols;
		    for (int c = 0; c < base_screen_cols; c++)
		    {
			base_screenlines[base_off + c] = ScreenLines[off + c];
			base_screenattrs[base_off + c] = ScreenAttrs[off + c];
			if (enc_utf8 && base_screenlinesuc != NULL)
			    base_screenlinesuc[base_off + c] =
					ScreenLinesUC[off + c];
		    }
		}
	    }
	}
#endif

	// Save background ScreenLines for padding opacity before win_update()
	// overwrites them.
	popup_saved_screen_T saved_screen = { 0 };

	popup_save_padding_screen(wp, &saved_screen);

	// Set flags in popup_transparent[] for masked cells.
	update_popup_transparent(wp, 1);

	// adjust w_winrow and w_wincol for border and padding, since
	// win_update() doesn't handle them.
	top_off = popup_top_extra(wp);
	left_extra = wp->w_popup_padding[3] + wp->w_popup_border[3]
							- wp->w_popup_leftoff;
	if (wp->w_wincol + left_extra < 0)
	    left_extra = -wp->w_wincol;
	wp->w_winrow += top_off;
	wp->w_wincol += left_extra;

	// Draw the popup text, unless it's off screen.
	if (wp->w_winrow < screen_Rows && wp->w_wincol < screen_Columns)
	{
	    popup_geom_save_T saved;

	    popup_geom_save(wp, &saved);

	    // May need to update the "cursorline" highlighting, which may also
	    // change "topline"
	    if (wp->w_popup_last_curline != wp->w_cursor.lnum)
		popup_highlight_curline(wp);

	    // Clip the buffer's drawn extent to the host window when
	    // "clipwindow" is set.  The transient mutations are reverted by
	    // popup_geom_restore() so callers continue to see the popup's
	    // logical geometry via popup_getoptions/popup_getpos.
	    popup_apply_winupdate_clip(wp, &cl);

	    win_update(wp);

	    popup_geom_restore(wp, &saved);

	    // move the cursor into the visible lines, otherwise executing
	    // commands with win_execute() may cause the text to jump.
	    if (wp->w_cursor.lnum < wp->w_topline)
		wp->w_cursor.lnum = wp->w_topline;
	    else if (wp->w_cursor.lnum >= wp->w_botline)
		wp->w_cursor.lnum = wp->w_botline - 1;
	}

	wp->w_winrow -= top_off;
	wp->w_wincol -= left_extra;

	// "clipwindow" with top-clip shifts all popup decorations down so the
	// first visible row of the popup lands at the host window's top edge.
	// Apply the shift before drawing borders/padding/etc. and restore at
	// the end of this popup's iteration.
	wp->w_winrow += wp->w_popup_topoff;

	// Add offset for border and padding if not done already.
	if ((wp->w_flags & WFLAG_WCOL_OFF_ADDED) == 0)
	{
	    wp->w_wcol += left_extra;
	    wp->w_flags |= WFLAG_WCOL_OFF_ADDED;
	}
	if ((wp->w_flags & WFLAG_WROW_OFF_ADDED) == 0)
	{
	    wp->w_wrow += top_off;
	    wp->w_flags |= WFLAG_WROW_OFF_ADDED;
	}

	// When clipped by "clipwindow", drop the border/padding slot at the
	// clipped edge that we will not render, so the popup ends exactly on
	// the last visible content row (no empty trailing side-border row)
	// and starts on the first visible row when top-clipped.  When
	// unclipped, fall back to the full popup geometry (cl.eff_width
	// excludes w_leftcol and the scrollbar, which popup_width() folds in).
	if (wp->w_popup_leftclip > 0 || wp->w_popup_rightclip > 0)
	    total_width = cl.eff_width;
	else
	    total_width = popup_width(wp) - wp->w_popup_rightoff;
	if (total_width < 0)
	    total_width = 0;
	if (wp->w_popup_topoff > 0 || wp->w_popup_bottomoff > 0)
	    total_height = cl.eff_height;
	else
	    total_height = popup_height(wp);
	popup_attr = get_win_attr(wp);

	if (wp->w_winrow + total_height > cmdline_row)
	    wp->w_popup_flags |= POPF_ON_CMDLINE;
	else
	    wp->w_popup_flags &= ~POPF_ON_CMDLINE;

	// We can only use these line drawing characters when 'encoding' is
	// "utf-8" and 'ambiwidth' is "single".
	if (enc_utf8 && *p_ambw == 's')
	{
	    border_char[0] = border_char[2] = 0x2550;
	    border_char[1] = border_char[3] = 0x2551;
	    border_char[4] = 0x2554;
	    border_char[5] = 0x2557;
	    border_char[6] = (wp->w_popup_flags & POPF_RESIZE)
							     ? 0x21f2 : 0x255d;
	    border_char[7] = 0x255a;
	}
	else
	{
	    border_char[0] = border_char[2] = '-';
	    border_char[1] = border_char[3] = '|';
	    for (i = 4; i < 8; ++i)
		border_char[i] = '+';
	    if (wp->w_popup_flags & POPF_RESIZE)
		border_char[6] = '@';
	}
	for (i = 0; i < 8; ++i)
	    if (wp->w_border_char[i] != 0)
		border_char[i] = wp->w_border_char[i];

	for (i = 0; i < 4; ++i)
	{
	    border_attr[i] = popup_attr;
	    if (wp->w_border_highlight[i] != NULL)
		border_attr[i] = syn_name2attr(wp->w_border_highlight[i]);

	    // Apply blend to border attributes for popup with opacitys
	    if ((wp->w_popup_flags & POPF_OPACITY) && wp->w_popup_blend > 0)
		border_attr[i] = hl_blend_attr(0, border_attr[i],
					       wp->w_popup_blend, FALSE);
	}

	// Apply blend to popup_attr for padding areas
	if ((wp->w_popup_flags & POPF_OPACITY) && wp->w_popup_blend > 0)
	    popup_attr = hl_blend_attr(0, popup_attr, wp->w_popup_blend, FALSE);


	// Title goes on top of border or padding.
	title_wincol = wp->w_wincol + 1;
	if (wp->w_popup_title != NULL)
	{
	    title_len = vim_strsize(wp->w_popup_title);

	    // truncate the title if too long
	    if (title_len > total_width - 2)
	    {
		int	title_byte_len = (int)STRLEN(wp->w_popup_title);
		char_u  *title_text = alloc(title_byte_len + 1);

		if (title_text != NULL)
		{
		    trunc_string(wp->w_popup_title, title_text,
					  total_width - 2, title_byte_len + 1);
		    screen_puts(title_text, wp->w_winrow, title_wincol,
				  wp->w_popup_border[0] > 0
						? border_attr[0] : popup_attr);
		    vim_free(title_text);
		}

		title_len = total_width - 2;
	    }
	    else
		screen_puts(wp->w_popup_title, wp->w_winrow, title_wincol,
		      wp->w_popup_border[0] > 0 ? border_attr[0] : popup_attr);
	}

	wincol = wp->w_wincol - wp->w_popup_leftoff + wp->w_popup_leftclip;
	top_padding = cl.eff_padding[0];
	if (cl.eff_border[0] > 0)
	{
	    // top border; do not draw over the title
	    if (title_len > 0)
	    {
		screen_fill(wp->w_winrow, wp->w_winrow + 1,
			wincol < 0 ? 0 : wincol, title_wincol,
			cl.eff_border[3] != 0 && wp->w_popup_leftoff == 0
					     ? border_char[4] : border_char[0],
			border_char[0], border_attr[0]);
		screen_fill(wp->w_winrow, wp->w_winrow + 1,
			title_wincol + title_len, wincol + total_width,
			border_char[0], border_char[0], border_attr[0]);
	    }
	    else
	    {
		screen_fill(wp->w_winrow, wp->w_winrow + 1,
			wincol < 0 ? 0 : wincol, wincol + total_width,
			cl.eff_border[3] != 0 && wp->w_popup_leftoff == 0
					     ? border_char[4] : border_char[0],
			border_char[0], border_attr[0]);
	    }
	    if (cl.eff_border[1] > 0)
	    {
		buf[mb_char2bytes(border_char[5], buf)] = NUL;
		screen_puts(buf, wp->w_winrow,
			       wincol + total_width - 1, border_attr[1]);
	    }
	}
	else if (cl.eff_padding[0] == 0 && popup_top_extra(wp) > 0
						    && wp->w_popup_topoff == 0)
	    top_padding = 1;

	if (top_padding > 0 || wp->w_popup_padding[2] > 0)
	{
	    padcol = wincol + wp->w_popup_border[3];
	    padendcol = wp->w_wincol + total_width - wp->w_popup_border[1]
							- wp->w_has_scrollbar;
	    if (padcol < 0)
	    {
		padendcol += padcol;
		padcol = 0;
	    }
	}
	if (top_padding > 0)
	{
	    row = wp->w_winrow + wp->w_popup_border[0];
	    if (title_len > 0 && row == wp->w_winrow)
	    {
		// top padding and no border; do not draw over the title
		if (screen_opacity_popup != NULL && saved_screen.lines != NULL)
		{
		    // Left of title
		    fill_opacity_padding(row, row + 1, padcol, title_wincol,
			    &saved_screen);
		    // Right of title
		    fill_opacity_padding(row, row + 1,
			    title_wincol + title_len, padendcol, &saved_screen);
		}
		else
		{
		    screen_fill(row, row + 1, padcol, title_wincol,
							     ' ', ' ', popup_attr);
		    screen_fill(row, row + 1, title_wincol + title_len,
						  padendcol, ' ', ' ', popup_attr);
		}
		row += 1;
		top_padding -= 1;
	    }
	    // Draw remaining top padding rows
	    if (screen_opacity_popup != NULL && saved_screen.lines != NULL)
	    {
		fill_opacity_padding(row, row + top_padding, padcol, padendcol,
			&saved_screen);
	    }
	    else
	    {
		screen_fill(row, row + top_padding, padcol, padendcol,
							     ' ', ' ', popup_attr);
	    }
	}

	// Compute scrollbar thumb position and size.
	if (wp->w_has_scrollbar)
	{
	    linenr_T	linecount = wp->w_buffer->b_ml.ml_line_count;
	    int		height = wp->w_height;
	    int		last;

	    sb_thumb_height = ((linenr_T)height * height + linecount / 2)
								   / linecount;
	    if (wp->w_topline > 1 && sb_thumb_height == height)
		--sb_thumb_height;  // scrolled, no full thumb
	    if (sb_thumb_height == 0)
		sb_thumb_height = 1;
	    if (linecount <= wp->w_height || wp->w_height == 0)
		// it just fits, avoid divide by zero
		sb_thumb_top = 0;
	    else
		sb_thumb_top = (wp->w_topline - 1
				+ (linecount / wp->w_height) / 2)
				* (wp->w_height - sb_thumb_height)
						  / (linecount - wp->w_height);
	    if (wp->w_topline > 1 && sb_thumb_top == 0 && height > 1)
		sb_thumb_top = 1;  // show it's scrolled
	    last = total_height - top_off - wp->w_popup_border[2];
	    if (sb_thumb_top >= last)
		// show at least one character
		sb_thumb_top = last - 1;

	    if (wp->w_scrollbar_highlight != NULL)
		attr_scroll = syn_name2attr(wp->w_scrollbar_highlight);
	    else
		attr_scroll = highlight_attr[HLF_PSB];
	    if (wp->w_thumb_highlight != NULL)
		attr_thumb = syn_name2attr(wp->w_thumb_highlight);
	    else
		attr_thumb = highlight_attr[HLF_PST];
	}

	// The side-border loop spans the popup's drawn extent.  cl.eff_border
	// and cl.eff_padding collapse the clipped edges to 0 so the loop
	// covers the full visible area without leaving an empty trailing row.
	for (i = cl.eff_border[0]; i < total_height - cl.eff_border[2]; ++i)
	{
	    int	pad_left;
	    // left and right padding only needed next to the body
	    int do_padding =
		    i >= cl.eff_border[0] + cl.eff_padding[0]
		    && i < total_height - cl.eff_border[2] - cl.eff_padding[2];

	    row = wp->w_winrow + i;

	    // left border
	    if (cl.eff_border[3] > 0 && wincol >= 0)
	    {
		buf[mb_char2bytes(border_char[3], buf)] = NUL;
		screen_puts(buf, row, wincol, border_attr[3]);
	    }
	    if (do_padding && cl.eff_padding[3] > 0)
	    {
		int col = wincol + wp->w_popup_border[3];

		// left padding
		pad_left = wp->w_popup_padding[3];
		if (col < 0)
		{
		    pad_left += col;
		    col = 0;
		}
		if (pad_left > 0)
		{
		    if (screen_opacity_popup != NULL && saved_screen.lines != NULL)
			fill_opacity_padding(row, row + 1, col, col + pad_left,
				&saved_screen);
		    else
			screen_fill(row, row + 1, col, col + pad_left,
							 ' ', ' ', popup_attr);
		}
	    }
	    // scrollbar
	    if (wp->w_has_scrollbar)
	    {
		int line = i - top_off;
		int scroll_col = wp->w_wincol + total_width - 1
						       - wp->w_popup_border[1];

		if (line >= 0 && line < wp->w_height)
		    screen_putchar(' ', row, scroll_col,
			    line >= sb_thumb_top
				       && line < sb_thumb_top + sb_thumb_height
						  ? attr_thumb : attr_scroll);
		else
		    screen_putchar(' ', row, scroll_col, popup_attr);
	    }
	    // right border
	    if (cl.eff_border[1] > 0)
	    {
		buf[mb_char2bytes(border_char[1], buf)] = NUL;
		screen_puts(buf, row, wincol + total_width - 1, border_attr[1]);
	    }
	    // right padding
	    if (do_padding && cl.eff_padding[1] > 0)
	    {
		int pad_col_start = wincol + wp->w_popup_border[3]
			+ wp->w_popup_padding[3] + wp->w_width + wp->w_leftcol;
		int pad_col_end = pad_col_start + wp->w_popup_padding[1];

		if (screen_opacity_popup != NULL && saved_screen.lines != NULL)
		    fill_opacity_padding(row, row + 1, pad_col_start, pad_col_end,
			    &saved_screen);
		else
		    screen_fill(row, row + 1, pad_col_start, pad_col_end,
							     ' ', ' ', popup_attr);
	    }
	}

	// right shadow
	if (wp->w_popup_shadow)
	{
	    int col = wincol + total_width;
	    for (i = 0; i < total_height; ++i)
	    {
		row = wp->w_winrow + i + 1;
		put_shadow_char(row, col);
		put_shadow_char(row, col + 1);
	    }
	}

	if (cl.eff_padding[2] > 0)
	{
	    // bottom padding
	    row = wp->w_winrow + wp->w_popup_border[0]
				       + wp->w_popup_padding[0] + wp->w_height;
	    if (screen_opacity_popup != NULL && saved_screen.lines != NULL)
		fill_opacity_padding(row, row + wp->w_popup_padding[2],
			padcol, padendcol, &saved_screen);
	    else
		screen_fill(row, row + wp->w_popup_padding[2],
					   padcol, padendcol, ' ', ' ', popup_attr);
	}

	if (cl.eff_border[2] > 0)
	{
	    // bottom border
	    row = wp->w_winrow + total_height - 1;
	    screen_fill(row, row + 1,
		    wincol < 0 ? 0 : wincol,
		    wincol + total_width,
		    cl.eff_border[3] != 0 && wp->w_popup_leftoff == 0
					     ? border_char[7] : border_char[2],
		    border_char[2], border_attr[2]);
	    if (cl.eff_border[1] > 0)
	    {
		buf[mb_char2bytes(border_char[6], buf)] = NUL;
		screen_puts(buf, row, wincol + total_width - 1, border_attr[2]);
	    }
	}

	if (wp->w_popup_shadow && wp->w_popup_bottomoff == 0)
	{
	    // bottom shadow
	    row = wp->w_winrow + total_height;
	    for (int col = 2 + (wincol < 0 ? 0 : wincol);
		    col < wincol + total_width; col++)
		put_shadow_char(row, col);
	}

	if (wp->w_popup_close == POPCLOSE_BUTTON)
	{
	    // close button goes on top of anything at the top-right corner
	    buf[mb_char2bytes('X', buf)] = NUL;
	    screen_puts(buf, wp->w_winrow, wincol + total_width - 1,
		      wp->w_popup_border[0] > 0 ? border_attr[0] : popup_attr);
	}

	update_popup_transparent(wp, 0);

	popup_free_saved_screen(&saved_screen);

	// Clear popup with opacity context.
	screen_opacity_popup = NULL;

	// Back to the normal zindex.
	screen_zindex = 0;

#ifdef HAS_MESSAGE_WINDOW
	// if this was the message window popup may start the timer now
	may_start_message_win_timer(wp);
#endif

	if (override_success)
	    pop_highlight_overrides();

	// Undo the topoff shift applied before drawing the borders so the
	// next iteration sees the popup's logical winrow.
	wp->w_winrow -= wp->w_popup_topoff;
    }

#ifdef FEAT_PROP_POPUP
    VIM_CLEAR(base_screenlines);
    VIM_CLEAR(base_screenattrs);
    VIM_CLEAR(base_screenlinesuc);
    base_screen_rows = 0;
    base_screen_cols = 0;
#endif

#if defined(FEAT_SEARCH_EXTRA)
    // In case win_update() called start_search_hl().
    end_search_hl();
#endif
}

/*
 * Mark references in callbacks of one popup window.
 */
    static int
set_ref_in_one_popup(win_T *wp, int copyID)
{
    int		abort = FALSE;
    typval_T	tv;

    if (wp->w_close_cb.cb_partial != NULL)
    {
	tv.v_type = VAR_PARTIAL;
	tv.vval.v_partial = wp->w_close_cb.cb_partial;
	abort = abort || set_ref_in_item(&tv, copyID, NULL, NULL, NULL);
    }
    if (wp->w_filter_cb.cb_partial != NULL)
    {
	tv.v_type = VAR_PARTIAL;
	tv.vval.v_partial = wp->w_filter_cb.cb_partial;
	abort = abort || set_ref_in_item(&tv, copyID, NULL, NULL, NULL);
    }
    abort = abort || set_ref_in_list(wp->w_popup_mask, copyID);
    return abort;
}

/*
 * Set reference in callbacks of popup windows.
 */
    int
set_ref_in_popups(int copyID)
{
    int		abort = FALSE;
    win_T	*wp;
    tabpage_T	*tp;

    for (wp = first_popupwin; !abort && wp != NULL; wp = wp->w_next)
	abort = abort || set_ref_in_one_popup(wp, copyID);

    FOR_ALL_TABPAGES(tp)
    {
	for (wp = tp->tp_first_popupwin; !abort && wp != NULL; wp = wp->w_next)
	    abort = abort || set_ref_in_one_popup(wp, copyID);
	if (abort)
	    break;
    }
    return abort;
}

    int
popup_is_popup(win_T *wp)
{
    return wp->w_popup_flags != 0;
}

#if defined(FEAT_QUICKFIX)
/*
 * Find an existing popup used as the preview window, in the current tab page.
 * Return NULL if not found.
 */
    win_T *
popup_find_preview_window(void)
{
    win_T *wp;

    // Preview window popup is always local to tab page.
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	if (wp->w_p_pvw)
	    return wp;
    return NULL;
}

/*
 * Find an existing popup used as the info window, in the current tab page.
 * Return NULL if not found.
 */
    win_T *
popup_find_info_window(void)
{
    win_T *wp;

    // info window popup is always local to tab page.
    FOR_ALL_POPUPWINS_IN_TAB(curtab, wp)
	if (wp->w_popup_flags & POPF_INFO)
	    return wp;
    return NULL;
}
#endif

    void
f_popup_findecho(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef HAS_MESSAGE_WINDOW
    rettv->vval.v_number = message_win == NULL ? 0 : message_win->w_id;
#else
    rettv->vval.v_number = 0;
#endif
}

    void
f_popup_findinfo(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_QUICKFIX
    win_T   *wp = popup_find_info_window();

    rettv->vval.v_number = wp == NULL ? 0 : wp->w_id;
#else
    rettv->vval.v_number = 0;
#endif
}

    void
f_popup_findpreview(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_QUICKFIX
    win_T   *wp = popup_find_preview_window();

    rettv->vval.v_number = wp == NULL ? 0 : wp->w_id;
#else
    rettv->vval.v_number = 0;
#endif
}

#if defined(FEAT_QUICKFIX)
/*
 * Create a popup to be used as the preview or info window.
 * NOTE: this makes the popup the current window, so that the file can be
 * edited.  However, it must not remain to be the current window, the caller
 * must make sure of that.
 */
    int
popup_create_preview_window(int info)
{
    win_T *wp = popup_create(NULL, NULL, info ? TYPE_INFO : TYPE_PREVIEW);

    if (wp == NULL)
	return FAIL;
    if (info)
	wp->w_popup_flags |= POPF_INFO;
    else
	wp->w_p_pvw = TRUE;

    // Set the width to a reasonable value, so that w_topline can be computed.
    if (wp->w_minwidth > 0)
	wp->w_width = wp->w_minwidth;
    else if (wp->w_maxwidth > 0)
	wp->w_width = wp->w_maxwidth;
    else
	wp->w_width = curwin->w_width;

    // Will switch to another buffer soon, dummy one can be wiped.
    wp->w_buffer->b_locked = FALSE;

    win_enter(wp, FALSE);
    return OK;
}

/*
 * Close any preview popup.
 */
    void
popup_close_preview(void)
{
    win_T *wp = popup_find_preview_window();

    if (wp != NULL)
	popup_close_with_retval(wp, -1);
}

/*
 * Hide the info popup.
 */
    void
popup_hide_info(void)
{
    win_T *wp = popup_find_info_window();

    if (wp != NULL)
    {
	popup_on_cmdline = wp->w_popup_flags & POPF_ON_CMDLINE;
	popup_hide(wp);
	if (State & MODE_CMDLINE)
	    // Cmdline mode doesn't normally call update_screen(), so it's
	    // necessary to use pum_call_update_screen() here.
	    pum_call_update_screen();
    }
}

/*
 * Close any info popup.
 */
    void
popup_close_info(void)
{
    win_T *wp = popup_find_info_window();

    if (wp != NULL)
	popup_close_with_retval(wp, -1);
}
#endif

/*
 * Returns TRUE if a popup extends into the cmdline area.
 */
    int
popup_overlaps_cmdline(void)
{
    return popup_on_cmdline;
}

#if defined(HAS_MESSAGE_WINDOW)

/*
 * Get the message window.
 * Returns NULL if something failed.
 */
    win_T *
popup_get_message_win(void)
{
    if (message_win != NULL)
	return message_win;

    int i;

    message_win = popup_create(NULL, NULL, TYPE_MESSAGE_WIN);

    if (message_win == NULL)
	return NULL;

    // use the full screen width
    message_win->w_width = Columns;

    // position at bottom of screen
    message_win->w_popup_pos = POPPOS_BOTTOM;
    message_win->w_wantcol = 1;
    message_win->w_minwidth = 9999;
    message_win->w_firstline = -1;

    // no padding, border at the top
    for (i = 0; i < 4; ++i)
	message_win->w_popup_padding[i] = 0;
    for (i = 1; i < 4; ++i)
	message_win->w_popup_border[i] = 0;

    if (message_win->w_popup_timer != NULL)
	message_win->w_popup_timer->tr_keep = TRUE;
    return message_win;
}

/*
 * If the message window is not visible: show it
 * If the message window is visible: reset the timeout
 */
    void
popup_show_message_win(void)
{
    if (message_win == NULL)
	return;

    if ((message_win->w_popup_flags & POPF_HIDDEN) != 0)
    {
	// the highlight may have changed.
	popup_update_color(message_win, TYPE_MESSAGE_WIN);
	popup_show(message_win);
    }
    start_message_win_timer = TRUE;
}

    static void
may_start_message_win_timer(win_T *wp)
{
    if (wp == message_win && start_message_win_timer)
    {
	if (message_win->w_popup_timer != NULL)
	{
	    message_win->w_popup_timer->tr_interval = message_win_time;
	    timer_start(message_win->w_popup_timer);
	    message_win_time = 3000;
	}
	start_message_win_timer = FALSE;
    }
}

    int
popup_message_win_visible(void)
{
    return message_win != NULL
	&& (message_win->w_popup_flags & POPF_HIDDEN) == 0;
}

/*
 * If the message window is visible: hide it.
 */
    void
popup_hide_message_win(void)
{
    if (message_win != NULL)
	popup_hide(message_win);
}

// Values saved in start_echowindow() and restored in end_echowindow()
static int save_msg_didout = FALSE;
static int save_msg_col = 0;
// Values saved in end_echowindow() and restored in start_echowindow()
static int ew_msg_didout = FALSE;
static int ew_msg_col = 0;

/*
 * Invoked before outputting a message for ":echowindow".
 * "time_sec" is the display time, zero means using the default 3 sec.
 */
    void
start_echowindow(int time_sec)
{
    in_echowindow = TRUE;
    save_msg_didout = msg_didout;
    save_msg_col = msg_col;
    msg_didout = ew_msg_didout;
    msg_col = ew_msg_col;
    if (time_sec != 0)
	message_win_time = time_sec * 1000;
}

/*
 * Invoked after outputting a message for ":echowindow".
 */
    void
end_echowindow(void)
{
    in_echowindow = FALSE;

    if ((State & MODE_HITRETURN) == 0)
	// show the message window now
	redraw_cmd(FALSE);

    // do not overwrite messages
    ew_msg_didout = TRUE;
    ew_msg_col = msg_col == 0 ? 1 : msg_col;
    msg_didout = save_msg_didout;
    msg_col = save_msg_col;
}
#endif

/*
 * Close any popup for a text property associated with "win".
 * Return TRUE if a popup was closed.
 */
    int
popup_win_closed(win_T *win)
{
    int	    round;
    win_T   *wp;
    win_T   *next;
    int	    ret = FALSE;

    for (round = 1; round <= 2; ++round)
	for (wp = round == 1 ? first_popupwin : curtab->tp_first_popupwin;
							 wp != NULL; wp = next)
	{
	    next = wp->w_next;
	    if (wp->w_popup_prop_win == win)
	    {
		popup_close_with_retval(wp, -1);
		ret = TRUE;
	    }
	}
    return ret;
}

/*
 * Set the title of the popup window to the file name.
 */
    void
popup_set_title(win_T *wp)
{
    if (wp->w_buffer->b_fname == NULL)
	return;

    char_u	dirname[MAXPATHL];
    size_t	len;

    mch_dirname(dirname, MAXPATHL);
    shorten_buf_fname(wp->w_buffer, dirname, FALSE);

    vim_free(wp->w_popup_title);
    len = STRLEN(wp->w_buffer->b_fname) + 3;
    wp->w_popup_title = alloc(len);
    if (wp->w_popup_title != NULL)
	vim_snprintf((char *)wp->w_popup_title, len, " %s ",
		wp->w_buffer->b_fname);
    redraw_win_later(wp, UPD_VALID);
}

#if defined(FEAT_QUICKFIX)
/*
 * If there is a preview window, update the title.
 * Used after changing directory.
 */
    void
popup_update_preview_title(void)
{
    win_T *wp = popup_find_preview_window();

    if (wp != NULL)
	popup_set_title(wp);
}
#endif

#endif // FEAT_PROP_POPUP
