/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * tabpanel.c:
 */

#include "vim.h"

#if defined(FEAT_TABPANEL)

static void do_by_tplmode(int tplmode, int col_start, int col_end,
	int *pcurtab_row, int *ptabpagenr);

// set pcurtab_row. don't redraw tabpanel.
#define TPLMODE_GET_CURTAB_ROW	0
// set ptabpagenr. don't redraw tabpanel.
#define TPLMODE_GET_TABPAGENR	1
// redraw tabpanel.
#define TPLMODE_REDRAW		2

#define TPL_FILLCHAR		' '

#define VERT_LEN		1

// tpl_align's values
#define ALIGN_LEFT		0
#define ALIGN_RIGHT		1

static char_u *opt_name = (char_u *)"tabpanel";
static int opt_scope = OPT_LOCAL;
static int tpl_align = ALIGN_LEFT;
static int tpl_columns = 20;
static int tpl_is_vert = FALSE;

typedef struct {
    win_T   *wp;
    win_T   *cwp;
    char_u  *user_defined;
    int	maxrow;
    int	offsetrow;
    int	*prow;
    int	*pcol;
    int	attr;
    int	col_start;
    int	col_end;
} tabpanel_T;

    int
tabpanelopt_changed(void)
{
    char_u	*p;
    int		new_align = ALIGN_LEFT;
    int		new_columns = 20;
    int		new_is_vert = FALSE;

    p = p_tplo;
    while (*p != NUL)
    {
	if (STRNCMP(p, "align:", 6) == 0)
	{
	    p += 6;
	    if (STRNCMP(p, "left", 4) == 0)
	    {
		p += 4;
		new_align = ALIGN_LEFT;
	    }
	    else if (STRNCMP(p, "right", 5) == 0)
	    {
		p += 5;
		new_align = ALIGN_RIGHT;
	    }
	    else
		return FAIL;
	}
	else if (STRNCMP(p, "columns:", 8) == 0 && VIM_ISDIGIT(p[8]))
	{
	    p += 8;
	    new_columns = getdigits(&p);
	}
	else if (STRNCMP(p, "vert", 4) == 0)
	{
	    p += 4;
	    new_is_vert = TRUE;
	}

	if (*p != ',' && *p != NUL)
	    return FAIL;
	if (*p == ',')
	    ++p;
    }

    tpl_align = new_align;
    tpl_columns = new_columns;
    tpl_is_vert = new_is_vert;

    shell_new_columns();
    return OK;
}

/*
 * Return the width of tabpanel.
 */
    int
tabpanel_width(void)
{
    switch (p_stpl)
    {
	case 0:
	    return 0;
	case 1:
	    if (first_tabpage->tp_next == NULL)
		return 0;
    }
    if (Columns < tpl_columns)
	return 0;
    else
	return tpl_columns;
}

/*
 * Return the offset of a window considering the width of tabpanel.
 */
    int
tabpanel_leftcol(void)
{
    return tpl_align == ALIGN_RIGHT ? 0 : tabpanel_width();
}

/*
 * draw the tabpanel.
 */
    void
draw_tabpanel(void)
{
    int saved_KeyTyped = KeyTyped;
    int saved_got_int = got_int;
    int maxwidth = tabpanel_width();
    int vs_attr = HL_ATTR(HLF_C);
    int curtab_row = 0;
    int vsrow = 0;
    int is_right = tpl_align == ALIGN_RIGHT;

    if (maxwidth == 0)
	return;

    // Reset got_int to avoid build_stl_str_hl() isn't evaluated.
    got_int = FALSE;

    if (tpl_is_vert)
    {
	if (is_right)
	{
	    // draw main contents in tabpanel
	    do_by_tplmode(TPLMODE_GET_CURTAB_ROW, VERT_LEN,
		    maxwidth - VERT_LEN, &curtab_row, NULL);
	    do_by_tplmode(TPLMODE_REDRAW, VERT_LEN, maxwidth, &curtab_row,
		    NULL);
	    // draw vert separator in tabpanel
	    for (vsrow = 0; vsrow < Rows; vsrow++)
		screen_putchar(curwin->w_fill_chars.tpl_vert, vsrow,
			topframe->fr_width, vs_attr);
	}
	else
	{
	    // draw main contents in tabpanel
	    do_by_tplmode(TPLMODE_GET_CURTAB_ROW, 0, maxwidth - VERT_LEN,
		    &curtab_row, NULL);
	    do_by_tplmode(TPLMODE_REDRAW, 0, maxwidth - VERT_LEN,
		    &curtab_row, NULL);
	    // draw vert separator in tabpanel
	    for (vsrow = 0; vsrow < Rows; vsrow++)
		screen_putchar(curwin->w_fill_chars.tpl_vert, vsrow,
			maxwidth - VERT_LEN, vs_attr);
	}
    }
    else
    {
	do_by_tplmode(TPLMODE_GET_CURTAB_ROW, 0, maxwidth, &curtab_row, NULL);
	do_by_tplmode(TPLMODE_REDRAW, 0, maxwidth, &curtab_row, NULL);
    }

    got_int |= saved_got_int;

    // A user function may reset KeyTyped, restore it.
    KeyTyped = saved_KeyTyped;

    redraw_tabpanel = FALSE;
}

/*
 * Return tabpagenr when clicking and dragging in tabpanel.
 */
    int
get_tabpagenr_on_tabpanel(void)
{
    int		maxwidth = tabpanel_width();
    int		curtab_row = 0;
    int		tabpagenr = 0;

    if (maxwidth == 0)
	return -1;

    do_by_tplmode(TPLMODE_GET_CURTAB_ROW, 0, maxwidth, &curtab_row, NULL);
    do_by_tplmode(TPLMODE_GET_TABPAGENR, 0, maxwidth, &curtab_row,
	    &tabpagenr);

    return tabpagenr;
}

/*
 * Fill tailing area between {start_row} and {end_row - 1}.
 */
    static void
screen_fill_tailing_area(
	int	tplmode,
	int	row_start,
	int	row_end,
	int	col_start,
	int	col_end,
	int	attr)
{
    int is_right = tpl_align == ALIGN_RIGHT;
    if (tplmode == TPLMODE_REDRAW)
	screen_fill(row_start, row_end,
		(is_right ? topframe->fr_width : 0) + col_start,
		(is_right ? topframe->fr_width : 0) + col_end,
		TPL_FILLCHAR, TPL_FILLCHAR, attr);
}

/*
 * screen_puts_len() for tabpanel.
 */
    static void
screen_puts_len_for_tabpanel(
	int	    tplmode,
	char_u	    *p,
	int	    len,
	int	    attr,
	tabpanel_T  *pargs)
{
    int		j, k;
    int		chlen;
    int		chcells;
    char_u	buf[IOSIZE];
    char_u	*temp;

    for (j = 0; j < len;)
    {
	if (tplmode != TPLMODE_GET_CURTAB_ROW
		&& pargs->maxrow <= *pargs->prow - pargs->offsetrow)
	    break;

	if (p[j] == '\n' || p[j] == '\r')
	{
	    // fill the tailing area of current row.
	    if (*pargs->prow - pargs->offsetrow >= 0
		    && *pargs->prow - pargs->offsetrow < pargs->maxrow)
		screen_fill_tailing_area(tplmode,
			*pargs->prow - pargs->offsetrow,
			*pargs->prow - pargs->offsetrow + 1,
			*pargs->pcol, pargs->col_end, attr);
	    (*pargs->prow)++;
	    *pargs->pcol = pargs->col_start;
	    j++;
	}
	else
	{
	    if (has_mbyte)
		chlen = (*mb_ptr2len)(p + j);
	    else
		chlen = (int)STRLEN(p + j);

	    for (k = 0; k < chlen; k++)
		buf[k] = p[j + k];
	    buf[chlen] = NUL;
	    j += chlen;

	    // Make all characters printable.
	    temp = transstr(buf);
	    if (temp != NULL)
	    {
		vim_strncpy(buf, temp, sizeof(buf) - 1);
		vim_free(temp);
	    }

	    if (has_mbyte)
		chcells = (*mb_ptr2cells)(buf);
	    else
		chcells = 1;

	    if (pargs->col_end < (*pargs->pcol) + chcells)
	    {
		// fill the tailing area of current row.
		if (*pargs->prow - pargs->offsetrow >= 0
			&& *pargs->prow - pargs->offsetrow < pargs->maxrow)
		    screen_fill_tailing_area(tplmode,
			    *pargs->prow - pargs->offsetrow,
			    *pargs->prow - pargs->offsetrow + 1,
			    *pargs->pcol, pargs->col_end, attr);
		*pargs->pcol = pargs->col_end;

		if (pargs->col_end < chcells)
		    break;
	    }

	    if (*pargs->pcol + chcells <= pargs->col_end)
	    {
		int off = (tpl_align == ALIGN_RIGHT)
			? topframe->fr_width
			: 0;
		if (TPLMODE_REDRAW == tplmode
			&& (*pargs->prow - pargs->offsetrow >= 0
			&& *pargs->prow - pargs->offsetrow < pargs->maxrow))
		    screen_puts(buf, *pargs->prow - pargs->offsetrow,
			    *pargs->pcol + off, attr);
		*pargs->pcol += chcells;
	    }
	}
    }
}

/*
 * default tabpanel drawing behavior if 'tabpanel' option is empty.
 */
    static void
draw_tabpanel_default(int tplmode, tabpanel_T *pargs)
{
    int		modified;
    int		wincount;
    int		len = 0;
    char_u	buf[2] = { NUL, NUL };

    modified = FALSE;
    for (wincount = 0; pargs->wp != NULL;
	    pargs->wp = pargs->wp->w_next, ++wincount)
	if (bufIsChanged(pargs->wp->w_buffer))
	    modified = TRUE;

    if (modified || wincount > 1)
    {
	if (wincount > 1)
	{
	    vim_snprintf((char *)NameBuff, MAXPATHL, "%d", wincount);
	    len = (int)STRLEN(NameBuff);
	    screen_puts_len_for_tabpanel(tplmode, NameBuff, len,
#if defined(FEAT_SYN_HL)
		    hl_combine_attr(pargs->attr, HL_ATTR(HLF_T)),
#else
		    pargs->attr,
#endif
		    pargs);
	}
	if (modified)
	{
	    buf[0] = '+';
	    screen_puts_len_for_tabpanel(tplmode, buf, 1, pargs->attr, pargs);
	}

	buf[0] = TPL_FILLCHAR;
	screen_puts_len_for_tabpanel(tplmode, buf, 1, pargs->attr, pargs);
    }

    get_trans_bufname(pargs->cwp->w_buffer);
    shorten_dir(NameBuff);
    len = (int)STRLEN(NameBuff);
    screen_puts_len_for_tabpanel(tplmode, NameBuff, len, pargs->attr, pargs);

    // fill the tailing area of current row.
    if (*pargs->prow - pargs->offsetrow >= 0
	    && *pargs->prow - pargs->offsetrow < pargs->maxrow)
	screen_fill_tailing_area(tplmode, *pargs->prow - pargs->offsetrow,
		*pargs->prow - pargs->offsetrow + 1,
		*pargs->pcol, pargs->col_end, pargs->attr);
    *pargs->pcol = pargs->col_end;
}

/*
 * default tabpanel drawing behavior if 'tabpanel' option is NOT empty.
 */
    static void
draw_tabpanel_userdefined(int tplmode, tabpanel_T *pargs)
{
    char_u	*p;
    int		p_crb_save;
    char_u	buf[IOSIZE];
    stl_hlrec_T *hltab;
    stl_hlrec_T *tabtab;
    int		curattr;
    int		n;

    // Temporarily reset 'cursorbind', we don't want a side effect from moving
    // the cursor away and back.
    p_crb_save = pargs->cwp->w_p_crb;
    pargs->cwp->w_p_crb = FALSE;

    // Make a copy, because the statusline may include a function call that
    // might change the option value and free the memory.
    p = vim_strsave(pargs->user_defined);

    build_stl_str_hl(pargs->cwp, buf, sizeof(buf),
	    p, opt_name, opt_scope,
	    TPL_FILLCHAR, pargs->col_end - pargs->col_start, &hltab, &tabtab);

    vim_free(p);
    pargs->cwp->w_p_crb = p_crb_save;

    curattr = pargs->attr;
    p = buf;
    for (n = 0; hltab[n].start != NULL; n++)
    {
	screen_puts_len_for_tabpanel(tplmode, p, (int)(hltab[n].start - p),
		curattr, pargs);
	p = hltab[n].start;
	if (hltab[n].userhl == 0)
	    curattr = pargs->attr;
	else if (hltab[n].userhl < 0)
	    curattr = syn_id2attr(-hltab[n].userhl);
#ifdef FEAT_TERMINAL
	else if (pargs->wp != NULL && pargs->wp != curwin
		&& bt_terminal(pargs->wp->w_buffer)
		&& pargs->wp->w_status_height != 0)
	    curattr = highlight_stltermnc[hltab[n].userhl - 1];
	else if (pargs->wp != NULL && bt_terminal(pargs->wp->w_buffer)
		&& pargs->wp->w_status_height != 0)
	    curattr = highlight_stlterm[hltab[n].userhl - 1];
#endif
	else if (pargs->wp != NULL && pargs->wp != curwin
		&& pargs->wp->w_status_height != 0)
	    curattr = highlight_stlnc[hltab[n].userhl - 1];
	else
	    curattr = highlight_user[hltab[n].userhl - 1];
    }
    screen_puts_len_for_tabpanel(tplmode, p, (int)STRLEN(p), curattr, pargs);

    // fill the tailing area of current row.
    if (*pargs->prow - pargs->offsetrow >= 0
	    && *pargs->prow - pargs->offsetrow < pargs->maxrow)
	screen_fill_tailing_area(tplmode, *pargs->prow - pargs->offsetrow,
		*pargs->prow - pargs->offsetrow + 1, *pargs->pcol,
		pargs->col_end, curattr);
    *pargs->pcol = pargs->col_end;
}

    static char_u *
starts_with_percent_and_bang(tabpanel_T *pargs)
{
    int		len = 0;
    char_u	*usefmt = p_tpl;
    int		did_emsg_before = did_emsg;

    if (usefmt == NULL)
	return NULL;

    len = (int)STRLEN(usefmt);

    if (len == 0)
	return NULL;

#ifdef FEAT_EVAL
    // if "fmt" was set insecurely it needs to be evaluated in the sandbox
    int	use_sandbox = was_set_insecurely(curwin, opt_name, opt_scope);

    // When the format starts with "%!" then evaluate it as an expression and
    // use the result as the actual format string.
    if (len > 1 && usefmt[0] == '%' && usefmt[1] == '!')
    {
	typval_T	tv;
	char_u		*p = NULL;

	tv.v_type = VAR_NUMBER;
	tv.vval.v_number = pargs->cwp->w_id;
	set_var((char_u *)"g:tabpanel_winid", &tv, FALSE);

	p = eval_to_string_safe(usefmt + 2, use_sandbox, FALSE, FALSE);
	if (p != NULL)
	    usefmt = p;

	do_unlet((char_u *)"g:tabpanel_winid", TRUE);

	if (did_emsg > did_emsg_before)
	{
	    usefmt = NULL;
	    set_string_option_direct(opt_name, -1, (char_u *)"",
		    OPT_FREE | opt_scope, SID_ERROR);
	}
    }
#endif

    return usefmt;
}

/*
 * do something by tplmode for drawing tabpanel.
 */
    static void
do_by_tplmode(
	int	tplmode,
	int	col_start,
	int	col_end,
	int	*pcurtab_row,
	int	*ptabpagenr)
{
    int		attr_tplf = HL_ATTR(HLF_TPLF);
    int		attr_tpls = HL_ATTR(HLF_TPLS);
    int		attr_tpl = HL_ATTR(HLF_TPL);
    int		col = col_start;
    int		row = 0;
    tabpage_T	*tp = NULL;
    typval_T	v;
    tabpanel_T	args;

    args.maxrow = Rows;
    args.offsetrow = 0;
    args.col_start = col_start;
    args.col_end = col_end;

    if (tplmode != TPLMODE_GET_CURTAB_ROW && args.maxrow > 0)
	while (args.offsetrow + args.maxrow <= *pcurtab_row)
	    args.offsetrow += args.maxrow;

    tp = first_tabpage;

    for (row = 0; tp != NULL; row++)
    {
	if (tplmode != TPLMODE_GET_CURTAB_ROW
		&& args.maxrow <= row - args.offsetrow)
	    break;

	col = col_start;

	v.v_type = VAR_NUMBER;
	v.vval.v_number = tabpage_index(tp);
	set_var((char_u *)"g:actual_curtabpage", &v, TRUE);

	if (tp->tp_topframe == topframe)
	{
	    args.attr = attr_tpls;
	    if (tplmode == TPLMODE_GET_CURTAB_ROW)
	    {
		*pcurtab_row = row;
		break;
	    }
	}
	else
	    args.attr = attr_tpl;

	if (tp == curtab)
	{
	    args.cwp = curwin;
	    args.wp = firstwin;
	}
	else
	{
	    args.cwp = tp->tp_curwin;
	    args.wp = tp->tp_firstwin;
	}

	char_u *usefmt = starts_with_percent_and_bang(&args);
	if (usefmt != NULL)
	{
	    char_u	buf[IOSIZE];
	    char_u	*p = usefmt;
	    size_t	i = 0;

	    while (p[i] != NUL)
	    {
		while (p[i] == '\n' || p[i] == '\r')
		{
		    // fill the tailing area of current row.
		    if (row - args.offsetrow >= 0
			    && row - args.offsetrow < args.maxrow)
			screen_fill_tailing_area(tplmode,
				row - args.offsetrow,
				row - args.offsetrow + 1,
				col, args.col_end, args.attr);
		    row++;
		    col = col_start;
		    p++;
		}

		while (p[i] != '\n' && p[i] != '\r' && p[i] != NUL)
		{
		    if (i + 1 >= sizeof(buf))
			break;
		    buf[i] = p[i];
		    i++;
		}
		buf[i] = NUL;

		args.user_defined = buf;
		args.prow = &row;
		args.pcol = &col;
		draw_tabpanel_userdefined(tplmode, &args);
		// p_tpl could have been freed in build_stl_str_hl()
		if (p_tpl == NULL || *p_tpl == NUL)
		{
		    usefmt = NULL;
		    break;
		}

		p += i;
		i = 0;
	    }
	    if (usefmt != p_tpl)
		VIM_CLEAR(usefmt);
	}
	else
	{
	    args.user_defined = NULL;
	    args.prow = &row;
	    args.pcol = &col;
	    draw_tabpanel_default(tplmode, &args);
	}

	do_unlet((char_u *)"g:actual_curtabpage", TRUE);

	tp = tp->tp_next;

	if ((tplmode == TPLMODE_GET_TABPAGENR)
		&& (mouse_row <= (row - args.offsetrow)))
	{
	    *ptabpagenr = v.vval.v_number;
	    break;
	}
    }

    // fill the area of TabPanelFill.
    screen_fill_tailing_area(tplmode, row - args.offsetrow, args.maxrow,
	    args.col_start, args.col_end, attr_tplf);
}

#endif // FEAT_TABPANEL
