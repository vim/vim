/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * popupmnu.c: Popup menu (PUM)
 */
#include "vim.h"

#if defined(FEAT_INS_EXPAND) || defined(PROTO)

static pumitem_T *pum_array = NULL;	/* items of displayed pum */
static int pum_size;			/* nr of items in "pum_array" */
static int pum_selected;		/* index of selected item or -1 */
static int pum_first = 0;		/* index of top item */

static int pum_height;			/* nr of displayed pum items */
static int pum_width;			/* width of displayed pum items */
static int pum_base_width;		/* width of pum items base */
static int pum_kind_width;		/* width of pum items kind column */
static int pum_extra_width;		/* width of extra stuff */
static int pum_scrollbar;		/* TRUE when scrollbar present */

static int pum_row;			/* top row of pum */
static int pum_col;			/* left column of pum */

static win_T *pum_window = NULL;
static int pum_win_row;
static int pum_win_height;
static int pum_win_col;
static int pum_win_wcol;
static int pum_win_width;

static int pum_do_redraw = FALSE;	/* do redraw anyway */

static int pum_set_selected(int n, int repeat);

#define PUM_DEF_HEIGHT 10
#define PUM_DEF_WIDTH  15

    static void
pum_compute_size(void)
{
    int	i;
    int	w;

    /* Compute the width of the widest match and the widest extra. */
    pum_base_width = 0;
    pum_kind_width = 0;
    pum_extra_width = 0;
    for (i = 0; i < pum_size; ++i)
    {
	w = vim_strsize(pum_array[i].pum_text);
	if (pum_base_width < w)
	    pum_base_width = w;
	if (pum_array[i].pum_kind != NULL)
	{
	    w = vim_strsize(pum_array[i].pum_kind) + 1;
	    if (pum_kind_width < w)
		pum_kind_width = w;
	}
	if (pum_array[i].pum_extra != NULL)
	{
	    w = vim_strsize(pum_array[i].pum_extra) + 1;
	    if (pum_extra_width < w)
		pum_extra_width = w;
	}
    }
}

/*
 * Show the popup menu with items "array[size]".
 * "array" must remain valid until pum_undisplay() is called!
 * When possible the leftmost character is aligned with cursor column.
 * The menu appears above the screen line "row" or at "row" + "height" - 1.
 */
    void
pum_display(
    pumitem_T	*array,
    int		size,
    int		selected)	/* index of initially selected item, none if
				   out of range */
{
    int		def_width;
    int		max_width;
    int		context_lines;
    int		cursor_col;
    int		above_row;
    int		below_row;
    int		redo_count = 0;
#if defined(FEAT_QUICKFIX)
    win_T	*pvwin;
#endif

    do
    {
	def_width = p_pw;
	above_row = 0;
	below_row = cmdline_row;

	/* Pretend the pum is already there to avoid that must_redraw is set
	 * when 'cuc' is on. */
	pum_array = (pumitem_T *)1;
	validate_cursor_col();
	pum_array = NULL;

	// Remember the essential parts of the window position and size, so we
	// can decide when to reposition the popup menu.
	pum_window = curwin;
	pum_win_row = curwin->w_wrow + W_WINROW(curwin);
	pum_win_height = curwin->w_height;
	pum_win_col = curwin->w_wincol;
	pum_win_wcol = curwin->w_wcol;
	pum_win_width = curwin->w_width;

#if defined(FEAT_QUICKFIX)
	FOR_ALL_WINDOWS(pvwin)
	    if (pvwin->w_p_pvw)
		break;
	if (pvwin != NULL)
	{
	    if (W_WINROW(pvwin) < W_WINROW(curwin))
		above_row = W_WINROW(pvwin) + pvwin->w_height;
	    else if (W_WINROW(pvwin) > W_WINROW(curwin) + curwin->w_height)
		below_row = W_WINROW(pvwin);
	}
#endif

	/*
	 * Figure out the size and position of the pum.
	 */
	if (size < PUM_DEF_HEIGHT)
	    pum_height = size;
	else
	    pum_height = PUM_DEF_HEIGHT;
	if (p_ph > 0 && pum_height > p_ph)
	    pum_height = p_ph;

	/* Put the pum below "pum_win_row" if possible.  If there are few lines
	 * decide on where there is more room. */
	if (pum_win_row + 2 >= below_row - pum_height
		      && pum_win_row - above_row > (below_row - above_row) / 2)
	{
	    /* pum above "pum_win_row" */

	    /* Leave two lines of context if possible */
	    if (curwin->w_wrow - curwin->w_cline_row >= 2)
		context_lines = 2;
	    else
		context_lines = curwin->w_wrow - curwin->w_cline_row;

	    if (pum_win_row >= size + context_lines)
	    {
		pum_row = pum_win_row - size - context_lines;
		pum_height = size;
	    }
	    else
	    {
		pum_row = 0;
		pum_height = pum_win_row - context_lines;
	    }
	    if (p_ph > 0 && pum_height > p_ph)
	    {
		pum_row += pum_height - p_ph;
		pum_height = p_ph;
	    }
	}
	else
	{
	    /* pum below "pum_win_row" */

	    /* Leave two lines of context if possible */
	    if (curwin->w_cline_row
				+ curwin->w_cline_height - curwin->w_wrow >= 3)
		context_lines = 3;
	    else
		context_lines = curwin->w_cline_row
				    + curwin->w_cline_height - curwin->w_wrow;

	    pum_row = pum_win_row + context_lines;
	    if (size > below_row - pum_row)
		pum_height = below_row - pum_row;
	    else
		pum_height = size;
	    if (p_ph > 0 && pum_height > p_ph)
		pum_height = p_ph;
	}

	/* don't display when we only have room for one line */
	if (pum_height < 1 || (pum_height == 1 && size > 1))
	    return;

#if defined(FEAT_QUICKFIX)
	// If there is a preview window at the above avoid drawing over it.
	// Do keep at least 10 entries.
	if (pvwin != NULL && pum_row < above_row && pum_height > 10)
	{
	    if (pum_win_row - above_row < 10)
	    {
		pum_row = pum_win_row - 10;
		pum_height = 10;
	    }
	    else
	    {
		pum_row = above_row;
		pum_height = pum_win_row - above_row;
	    }
	}
#endif

	pum_array = array;
	pum_size = size;
	pum_compute_size();
	max_width = pum_base_width;

	/* Calculate column */
#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	    cursor_col = curwin->w_wincol + curwin->w_width
							  - curwin->w_wcol - 1;
	else
#endif
	    cursor_col = curwin->w_wincol + curwin->w_wcol;

	/* if there are more items than room we need a scrollbar */
	if (pum_height < size)
	{
	    pum_scrollbar = 1;
	    ++max_width;
	}
	else
	    pum_scrollbar = 0;

	if (def_width < max_width)
	    def_width = max_width;

	if (((cursor_col < Columns - p_pw
					   || cursor_col < Columns - max_width)
#ifdef FEAT_RIGHTLEFT
		    && !curwin->w_p_rl)
	       || (curwin->w_p_rl
			       && (cursor_col > p_pw || cursor_col > max_width)
#endif
	   ))
	{
	    /* align pum with "cursor_col" */
	    pum_col = cursor_col;

	    /* start with the maximum space available */
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		pum_width = pum_col - pum_scrollbar + 1;
	    else
#endif
		pum_width = Columns - pum_col - pum_scrollbar;

	    if (pum_width > max_width + pum_kind_width + pum_extra_width + 1
						&& pum_width > p_pw)
	    {
		/* the width is more than needed for the items, make it
		 * narrower */
		pum_width = max_width + pum_kind_width + pum_extra_width + 1;
		if (pum_width < p_pw)
		    pum_width = p_pw;
	    }
	    else if (((cursor_col > p_pw || cursor_col > max_width)
#ifdef FEAT_RIGHTLEFT
			&& !curwin->w_p_rl)
		|| (curwin->w_p_rl && (cursor_col < Columns - p_pw
			|| cursor_col < Columns - max_width)
#endif
		    ))
	    {
		/* align pum edge with "cursor_col" */
#ifdef FEAT_RIGHTLEFT
		if (curwin->w_p_rl
			&& W_ENDCOL(curwin) < max_width + pum_scrollbar + 1)
		{
		    pum_col = cursor_col + max_width + pum_scrollbar + 1;
		    if (pum_col >= Columns)
			pum_col = Columns - 1;
		}
		else if (!curwin->w_p_rl)
#endif
		{
		    if (curwin->w_wincol > Columns - max_width - pum_scrollbar
							  && max_width <= p_pw)
		    {
			/* use full width to end of the screen */
			pum_col = Columns - max_width - pum_scrollbar;
			if (pum_col < 0)
			    pum_col = 0;
		    }
		}

#ifdef FEAT_RIGHTLEFT
		if (curwin->w_p_rl)
		    pum_width = pum_col - pum_scrollbar + 1;
		else
#endif
		    pum_width = Columns - pum_col - pum_scrollbar;

		if (pum_width < p_pw)
		{
		    pum_width = p_pw;
#ifdef FEAT_RIGHTLEFT
		    if (curwin->w_p_rl)
		    {
			if (pum_width > pum_col)
			    pum_width = pum_col;
		    }
		    else
#endif
		    {
			if (pum_width >= Columns - pum_col)
			    pum_width = Columns - pum_col - 1;
		    }
		}
		else if (pum_width > max_width + pum_kind_width
							  + pum_extra_width + 1
			    && pum_width > p_pw)
		{
		    pum_width = max_width + pum_kind_width
							 + pum_extra_width + 1;
		    if (pum_width < p_pw)
			pum_width = p_pw;
		}
	    }

	}
	else if (Columns < def_width)
	{
	    /* not enough room, will use what we have */
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		pum_col = Columns - 1;
	    else
#endif
		pum_col = 0;
	    pum_width = Columns - 1;
	}
	else
	{
	    if (max_width > p_pw)
		max_width = p_pw;	/* truncate */
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		pum_col = max_width - 1;
	    else
#endif
		pum_col = Columns - max_width;
	    pum_width = max_width - pum_scrollbar;
	}

	/* Set selected item and redraw.  If the window size changed need to
	 * redo the positioning.  Limit this to two times, when there is not
	 * much room the window size will keep changing. */
    } while (pum_set_selected(selected, redo_count) && ++redo_count <= 2);
}

/*
 * Redraw the popup menu, using "pum_first" and "pum_selected".
 */
    void
pum_redraw(void)
{
    int		row = pum_row;
    int		col;
    int		attr_norm = highlight_attr[HLF_PNI];
    int		attr_select = highlight_attr[HLF_PSI];
    int		attr_scroll = highlight_attr[HLF_PSB];
    int		attr_thumb = highlight_attr[HLF_PST];
    int		attr;
    int		i;
    int		idx;
    char_u	*s;
    char_u	*p = NULL;
    int		totwidth, width, w;
    int		thumb_pos = 0;
    int		thumb_heigth = 1;
    int		round;
    int		n;

    /* Never display more than we have */
    if (pum_first > pum_size - pum_height)
	pum_first = pum_size - pum_height;

    if (pum_scrollbar)
    {
	thumb_heigth = pum_height * pum_height / pum_size;
	if (thumb_heigth == 0)
	    thumb_heigth = 1;
	thumb_pos = (pum_first * (pum_height - thumb_heigth)
			    + (pum_size - pum_height) / 2)
						    / (pum_size - pum_height);
    }

    for (i = 0; i < pum_height; ++i)
    {
	idx = i + pum_first;
	attr = (idx == pum_selected) ? attr_select : attr_norm;

	/* prepend a space if there is room */
#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	{
	    if (pum_col < curwin->w_wincol + curwin->w_width - 1)
		screen_putchar(' ', row, pum_col + 1, attr);
	}
	else
#endif
	    if (pum_col > 0)
		screen_putchar(' ', row, pum_col - 1, attr);

	/* Display each entry, use two spaces for a Tab.
	 * Do this 3 times: For the main text, kind and extra info */
	col = pum_col;
	totwidth = 0;
	for (round = 1; round <= 3; ++round)
	{
	    width = 0;
	    s = NULL;
	    switch (round)
	    {
		case 1: p = pum_array[idx].pum_text; break;
		case 2: p = pum_array[idx].pum_kind; break;
		case 3: p = pum_array[idx].pum_extra; break;
	    }
	    if (p != NULL)
		for ( ; ; MB_PTR_ADV(p))
		{
		    if (s == NULL)
			s = p;
		    w = ptr2cells(p);
		    if (*p == NUL || *p == TAB || totwidth + w > pum_width)
		    {
			/* Display the text that fits or comes before a Tab.
			 * First convert it to printable characters. */
			char_u	*st;
			int	saved = *p;

			if (saved != NUL)
			    *p = NUL;
			st = transstr(s);
			if (saved != NUL)
			    *p = saved;
#ifdef FEAT_RIGHTLEFT
			if (curwin->w_p_rl)
			{
			    if (st != NULL)
			    {
				char_u	*rt = reverse_text(st);

				if (rt != NULL)
				{
				    char_u	*rt_start = rt;
				    int		size;

				    size = vim_strsize(rt);
				    if (size > pum_width)
				    {
					do
					{
					    size -= has_mbyte
						    ? (*mb_ptr2cells)(rt) : 1;
					    MB_PTR_ADV(rt);
					} while (size > pum_width);

					if (size < pum_width)
					{
					    /* Most left character requires
					     * 2-cells but only 1 cell is
					     * available on screen.  Put a
					     * '<' on the left of the pum
					     * item */
					    *(--rt) = '<';
					    size++;
					}
				    }
				    screen_puts_len(rt, (int)STRLEN(rt),
						   row, col - size + 1, attr);
				    vim_free(rt_start);
				}
				vim_free(st);
			    }
			    col -= width;
			}
			else
#endif
			{
			    if (st != NULL)
			    {
				screen_puts_len(st, (int)STRLEN(st), row, col,
									attr);
				vim_free(st);
			    }
			    col += width;
			}

			if (*p != TAB)
			    break;

			/* Display two spaces for a Tab. */
#ifdef FEAT_RIGHTLEFT
			if (curwin->w_p_rl)
			{
			    screen_puts_len((char_u *)"  ", 2, row, col - 1,
									attr);
			    col -= 2;
			}
			else
#endif
			{
			    screen_puts_len((char_u *)"  ", 2, row, col, attr);
			    col += 2;
			}
			totwidth += 2;
			s = NULL;	    /* start text at next char */
			width = 0;
		    }
		    else
			width += w;
		}

	    if (round > 1)
		n = pum_kind_width + 1;
	    else
		n = 1;

	    /* Stop when there is nothing more to display. */
	    if (round == 3
		    || (round == 2 && pum_array[idx].pum_extra == NULL)
		    || (round == 1 && pum_array[idx].pum_kind == NULL
					  && pum_array[idx].pum_extra == NULL)
		    || pum_base_width + n >= pum_width)
		break;
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
	    {
		screen_fill(row, row + 1, pum_col - pum_base_width - n + 1,
						    col + 1, ' ', ' ', attr);
		col = pum_col - pum_base_width - n + 1;
	    }
	    else
#endif
	    {
		screen_fill(row, row + 1, col, pum_col + pum_base_width + n,
							      ' ', ' ', attr);
		col = pum_col + pum_base_width + n;
	    }
	    totwidth = pum_base_width + n;
	}

#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	    screen_fill(row, row + 1, pum_col - pum_width + 1, col + 1, ' ',
								    ' ', attr);
	else
#endif
	    screen_fill(row, row + 1, col, pum_col + pum_width, ' ', ' ',
									attr);
	if (pum_scrollbar > 0)
	{
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		screen_putchar(' ', row, pum_col - pum_width,
			i >= thumb_pos && i < thumb_pos + thumb_heigth
						  ? attr_thumb : attr_scroll);
	    else
#endif
		screen_putchar(' ', row, pum_col + pum_width,
			i >= thumb_pos && i < thumb_pos + thumb_heigth
						  ? attr_thumb : attr_scroll);
	}

	++row;
    }
}

/*
 * Set the index of the currently selected item.  The menu will scroll when
 * necessary.  When "n" is out of range don't scroll.
 * This may be repeated when the preview window is used:
 * "repeat" == 0: open preview window normally
 * "repeat" == 1: open preview window but don't set the size
 * "repeat" == 2: don't open preview window
 * Returns TRUE when the window was resized and the location of the popup menu
 * must be recomputed.
 */
    static int
pum_set_selected(int n, int repeat)
{
    int	    resized = FALSE;
    int	    context = pum_height / 2;

    pum_selected = n;

    if (pum_selected >= 0 && pum_selected < pum_size)
    {
	if (pum_first > pum_selected - 4)
	{
	    /* scroll down; when we did a jump it's probably a PageUp then
	     * scroll a whole page */
	    if (pum_first > pum_selected - 2)
	    {
		pum_first -= pum_height - 2;
		if (pum_first < 0)
		    pum_first = 0;
		else if (pum_first > pum_selected)
		    pum_first = pum_selected;
	    }
	    else
		pum_first = pum_selected;
	}
	else if (pum_first < pum_selected - pum_height + 5)
	{
	    /* scroll up; when we did a jump it's probably a PageDown then
	     * scroll a whole page */
	    if (pum_first < pum_selected - pum_height + 1 + 2)
	    {
		pum_first += pum_height - 2;
		if (pum_first < pum_selected - pum_height + 1)
		    pum_first = pum_selected - pum_height + 1;
	    }
	    else
		pum_first = pum_selected - pum_height + 1;
	}

	/* Give a few lines of context when possible. */
	if (context > 3)
	    context = 3;
	if (pum_height > 2)
	{
	    if (pum_first > pum_selected - context)
	    {
		/* scroll down */
		pum_first = pum_selected - context;
		if (pum_first < 0)
		    pum_first = 0;
	    }
	    else if (pum_first < pum_selected + context - pum_height + 1)
	    {
		/* scroll up */
		pum_first = pum_selected + context - pum_height + 1;
	    }
	}

#if defined(FEAT_QUICKFIX)
	/*
	 * Show extra info in the preview window if there is something and
	 * 'completeopt' contains "preview".
	 * Skip this when tried twice already.
	 * Skip this also when there is not much room.
	 * NOTE: Be very careful not to sync undo!
	 */
	if (pum_array[pum_selected].pum_info != NULL
		&& Rows > 10
		&& repeat <= 1
		&& vim_strchr(p_cot, 'p') != NULL)
	{
	    win_T	*curwin_save = curwin;
	    tabpage_T   *curtab_save = curtab;
	    int		res = OK;

	    /* Open a preview window.  3 lines by default.  Prefer
	     * 'previewheight' if set and smaller. */
	    g_do_tagpreview = 3;
	    if (p_pvh > 0 && p_pvh < g_do_tagpreview)
		g_do_tagpreview = p_pvh;
	    ++RedrawingDisabled;
	    /* Prevent undo sync here, if an autocommand syncs undo weird
	     * things can happen to the undo tree. */
	    ++no_u_sync;
	    resized = prepare_tagpreview(FALSE);
	    --no_u_sync;
	    --RedrawingDisabled;
	    g_do_tagpreview = 0;

	    if (curwin->w_p_pvw)
	    {
		if (!resized
			&& curbuf->b_nwindows == 1
			&& curbuf->b_fname == NULL
			&& curbuf->b_p_bt[0] == 'n' && curbuf->b_p_bt[2] == 'f'
			&& curbuf->b_p_bh[0] == 'w')
		{
		    /* Already a "wipeout" buffer, make it empty. */
		    while (!BUFEMPTY())
			ml_delete((linenr_T)1, FALSE);
		}
		else
		{
		    /* Don't want to sync undo in the current buffer. */
		    ++no_u_sync;
		    res = do_ecmd(0, NULL, NULL, NULL, ECMD_ONE, 0, NULL);
		    --no_u_sync;
		    if (res == OK)
		    {
			/* Edit a new, empty buffer. Set options for a "wipeout"
			 * buffer. */
			set_option_value((char_u *)"swf", 0L, NULL, OPT_LOCAL);
			set_option_value((char_u *)"bt", 0L,
					       (char_u *)"nofile", OPT_LOCAL);
			set_option_value((char_u *)"bh", 0L,
						 (char_u *)"wipe", OPT_LOCAL);
			set_option_value((char_u *)"diff", 0L,
							     NULL, OPT_LOCAL);
		    }
		}
		if (res == OK)
		{
		    char_u	*p, *e;
		    linenr_T	lnum = 0;

		    for (p = pum_array[pum_selected].pum_info; *p != NUL; )
		    {
			e = vim_strchr(p, '\n');
			if (e == NULL)
			{
			    ml_append(lnum++, p, 0, FALSE);
			    break;
			}
			else
			{
			    *e = NUL;
			    ml_append(lnum++, p, (int)(e - p + 1), FALSE);
			    *e = '\n';
			    p = e + 1;
			}
		    }

		    /* Increase the height of the preview window to show the
		     * text, but no more than 'previewheight' lines. */
		    if (repeat == 0)
		    {
			if (lnum > p_pvh)
			    lnum = p_pvh;
			if (curwin->w_height < lnum)
			{
			    win_setheight((int)lnum);
			    resized = TRUE;
			}
		    }

		    curbuf->b_changed = 0;
		    curbuf->b_p_ma = FALSE;
		    curwin->w_cursor.lnum = 1;
		    curwin->w_cursor.col = 0;

		    if ((curwin != curwin_save && win_valid(curwin_save))
			    || (curtab != curtab_save
						&& valid_tabpage(curtab_save)))
		    {
			if (curtab != curtab_save && valid_tabpage(curtab_save))
			    goto_tabpage_tp(curtab_save, FALSE, FALSE);

			/* When the first completion is done and the preview
			 * window is not resized, skip the preview window's
			 * status line redrawing. */
			if (ins_compl_active() && !resized)
			    curwin->w_redr_status = FALSE;

			/* Return cursor to where we were */
			validate_cursor();
			redraw_later(SOME_VALID);

			/* When the preview window was resized we need to
			 * update the view on the buffer.  Only go back to
			 * the window when needed, otherwise it will always be
			 * redraw. */
			if (resized)
			{
			    ++no_u_sync;
			    win_enter(curwin_save, TRUE);
			    --no_u_sync;
			    update_topline();
			}

			/* Update the screen before drawing the popup menu.
			 * Enable updating the status lines. */
			pum_do_redraw = TRUE;
			update_screen(0);
			pum_do_redraw = FALSE;

			if (!resized && win_valid(curwin_save))
			{
			    ++no_u_sync;
			    win_enter(curwin_save, TRUE);
			    --no_u_sync;
			}

			/* May need to update the screen again when there are
			 * autocommands involved. */
			pum_do_redraw = TRUE;
			update_screen(0);
			pum_do_redraw = FALSE;
		    }
		}
	    }
	}
#endif
    }

    if (!resized)
	pum_redraw();

    return resized;
}

/*
 * Undisplay the popup menu (later).
 */
    void
pum_undisplay(void)
{
    pum_array = NULL;
    redraw_all_later(NOT_VALID);
    redraw_tabline = TRUE;
    status_redraw_all();
}

/*
 * Clear the popup menu.  Currently only resets the offset to the first
 * displayed item.
 */
    void
pum_clear(void)
{
    pum_first = 0;
}

/*
 * Return TRUE if the popup menu is displayed.
 * Overruled when "pum_do_redraw" is set, used to redraw the status lines.
 */
    int
pum_visible(void)
{
    return !pum_do_redraw && pum_array != NULL;
}

/*
 * Reposition the popup menu to adjust for window layout changes.
 */
    void
pum_may_redraw(void)
{
    pumitem_T	*array = pum_array;
    int		len = pum_size;
    int		selected = pum_selected;

    if (!pum_visible())
	return;  // nothing to do

    if (pum_window != curwin
	    || (pum_win_row == curwin->w_wrow + W_WINROW(curwin)
		&& pum_win_height == curwin->w_height
		&& pum_win_col == curwin->w_wincol
		&& pum_win_width == curwin->w_width))
    {
	// window position didn't change, redraw in the same position
	pum_redraw();
    }
    else
    {
	int wcol = curwin->w_wcol;

	// Window layout changed, recompute the position.
	// Use the remembered w_wcol value, the cursor may have moved when a
	// completion was inserted, but we want the menu in the same position.
	pum_undisplay();
	curwin->w_wcol = pum_win_wcol;
	curwin->w_valid |= VALID_WCOL;
	pum_display(array, len, selected);
	curwin->w_wcol = wcol;
    }
}

/*
 * Return the height of the popup menu, the number of entries visible.
 * Only valid when pum_visible() returns TRUE!
 */
    int
pum_get_height(void)
{
    return pum_height;
}

# if defined(FEAT_BEVAL_TERM) || defined(FEAT_TERM_POPUP_MENU) || defined(PROTO)
    static void
pum_position_at_mouse(int min_width)
{
    if (Rows - mouse_row > pum_size)
    {
	/* Enough space below the mouse row. */
	pum_row = mouse_row + 1;
	if (pum_height > Rows - pum_row)
	    pum_height = Rows - pum_row;
    }
    else
    {
	/* Show above the mouse row, reduce height if it does not fit. */
	pum_row = mouse_row - pum_size;
	if (pum_row < 0)
	{
	    pum_height += pum_row;
	    pum_row = 0;
	}
    }
    if (Columns - mouse_col >= pum_base_width
	    || Columns - mouse_col > min_width)
	/* Enough space to show at mouse column. */
	pum_col = mouse_col;
    else
	/* Not enough space, right align with window. */
	pum_col = Columns - (pum_base_width > min_width
						 ? min_width : pum_base_width);

    pum_width = Columns - pum_col;
    if (pum_width > pum_base_width + 1)
	pum_width = pum_base_width + 1;

    // Do not redraw at cursor position.
    pum_window = NULL;
}

# endif

# if defined(FEAT_BEVAL_TERM) || defined(PROTO)
static pumitem_T *balloon_array = NULL;
static int balloon_arraysize;
static int balloon_mouse_row = 0;
static int balloon_mouse_col = 0;

#define BALLOON_MIN_WIDTH 50
#define BALLOON_MIN_HEIGHT 10

typedef struct {
    char_u	*start;
    int		bytelen;
    int		cells;
    int		indent;
} balpart_T;

/*
 * Split a string into parts to display in the balloon.
 * Aimed at output from gdb.  Attempts to split at white space, preserve quoted
 * strings and make a struct look good.
 * Resulting array is stored in "array" and returns the size of the array.
 */
    int
split_message(char_u *mesg, pumitem_T **array)
{
    garray_T	ga;
    char_u	*p;
    balpart_T	*item;
    int		quoted = FALSE;
    int		height;
    int		line;
    int		item_idx;
    int		indent = 0;
    int		max_cells = 0;
    int		max_height = Rows / 2 - 2;
    int		long_item_count = 0;
    int		split_long_items = FALSE;

    ga_init2(&ga, sizeof(balpart_T), 20);
    p = mesg;

    while (*p != NUL)
    {
	if (ga_grow(&ga, 1) == FAIL)
	    goto failed;
	item = ((balpart_T *)ga.ga_data) + ga.ga_len;
	item->start = p;
	item->indent = indent;
	item->cells = indent * 2;
	++ga.ga_len;
	while (*p != NUL)
	{
	    if (*p == '"')
		quoted = !quoted;
	    else if (*p == '\\' && p[1] != NUL)
		++p;
	    else if (!quoted)
	    {
		if ((*p == ',' && p[1] == ' ') || *p == '{' || *p == '}')
		{
		    /* Looks like a good point to break. */
		    if (*p == '{')
			++indent;
		    else if (*p == '}' && indent > 0)
			--indent;
		    ++item->cells;
		    p = skipwhite(p + 1);
		    break;
		}
	    }
	    item->cells += ptr2cells(p);
	    p += MB_PTR2LEN(p);
	}
	item->bytelen = p - item->start;
	if (item->cells > max_cells)
	    max_cells = item->cells;
	long_item_count += (item->cells - 1) / BALLOON_MIN_WIDTH;
    }

    height = 2 + ga.ga_len;

    /* If there are long items and the height is below the limit: split lines */
    if (long_item_count > 0 && height + long_item_count <= max_height)
    {
	split_long_items = TRUE;
	height += long_item_count;
    }

    /* Limit to half the window height, it has to fit above or below the mouse
     * position. */
    if (height > max_height)
	height = max_height;
    *array = (pumitem_T *)alloc_clear((unsigned)sizeof(pumitem_T) * height);
    if (*array == NULL)
	goto failed;

    /* Add an empty line above and below, looks better. */
    (*array)->pum_text = vim_strsave((char_u *)"");
    (*array + height - 1)->pum_text = vim_strsave((char_u *)"");

    for (line = 1, item_idx = 0; line < height - 1; ++item_idx)
    {
	int	skip;
	int	thislen;
	int	copylen;
	int	ind;
	int	cells;

	item = ((balpart_T *)ga.ga_data) + item_idx;
	for (skip = 0; skip < item->bytelen; skip += thislen)
	{
	    if (split_long_items && item->cells >= BALLOON_MIN_WIDTH)
	    {
		cells = item->indent * 2;
		for (p = item->start + skip; p < item->start + item->bytelen;
							    p += MB_PTR2LEN(p))
		    if ((cells += ptr2cells(p)) > BALLOON_MIN_WIDTH)
			break;
		thislen = p - (item->start + skip);
	    }
	    else
		thislen = item->bytelen;

	    /* put indent at the start */
	    p = alloc(thislen + item->indent * 2 + 1);
	    for (ind = 0; ind < item->indent * 2; ++ind)
		p[ind] = ' ';

	    /* exclude spaces at the end of the string */
	    for (copylen = thislen; copylen > 0; --copylen)
		if (item->start[skip + copylen - 1] != ' ')
		    break;

	    vim_strncpy(p + ind, item->start + skip, copylen);
	    (*array)[line].pum_text = p;
	    item->indent = 0;  /* wrapped line has no indent */
	    ++line;
	}
    }
    ga_clear(&ga);
    return height;

failed:
    ga_clear(&ga);
    return 0;
}

    void
ui_remove_balloon(void)
{
    if (balloon_array != NULL)
    {
	pum_undisplay();
	while (balloon_arraysize > 0)
	    vim_free(balloon_array[--balloon_arraysize].pum_text);
	VIM_CLEAR(balloon_array);
    }
}

/*
 * Terminal version of a balloon, uses the popup menu code.
 */
    void
ui_post_balloon(char_u *mesg, list_T *list)
{
    ui_remove_balloon();

    if (mesg == NULL && list == NULL)
	return;
    if (list != NULL)
    {
	listitem_T  *li;
	int	    idx;

	balloon_arraysize = list->lv_len;
	balloon_array = (pumitem_T *)alloc_clear(
				   (unsigned)sizeof(pumitem_T) * list->lv_len);
	if (balloon_array == NULL)
	    return;
	for (idx = 0, li = list->lv_first; li != NULL; li = li->li_next, ++idx)
	{
	    char_u *text = get_tv_string_chk(&li->li_tv);

	    balloon_array[idx].pum_text = vim_strsave(
					   text == NULL ? (char_u *)"" : text);
	}
    }
    else
	balloon_arraysize = split_message(mesg, &balloon_array);

    if (balloon_arraysize > 0)
    {
	pum_array = balloon_array;
	pum_size = balloon_arraysize;
	pum_compute_size();
	pum_scrollbar = 0;
	pum_height = balloon_arraysize;

	pum_position_at_mouse(BALLOON_MIN_WIDTH);
	pum_selected = -1;
	pum_first = 0;
	pum_redraw();
    }
}

/*
 * Called when the mouse moved, may remove any displayed balloon.
 */
    void
ui_may_remove_balloon(void)
{
    if (mouse_row != balloon_mouse_row || mouse_col != balloon_mouse_col)
	ui_remove_balloon();
}
# endif

# if defined(FEAT_TERM_POPUP_MENU) || defined(PROTO)
/*
 * Select the pum entry at the mouse position.
 */
    static void
pum_select_mouse_pos(void)
{
    int idx = mouse_row - pum_row;

    if (idx < 0 || idx >= pum_size)
	pum_selected = -1;
    else if (*pum_array[idx].pum_text != NUL)
	pum_selected = idx;
}

/*
 * Execute the currently selected popup menu item.
 */
    static void
pum_execute_menu(vimmenu_T *menu, int mode)
{
    vimmenu_T   *mp;
    int		idx = 0;
    exarg_T	ea;

    for (mp = menu->children; mp != NULL; mp = mp->next)
	if ((mp->modes & mp->enabled & mode) && idx++ == pum_selected)
	{
	    vim_memset(&ea, 0, sizeof(ea));
	    execute_menu(&ea, mp, -1);
	    break;
	}
}

/*
 * Open the terminal version of the popup menu and don't return until it is
 * closed.
 */
    void
pum_show_popupmenu(vimmenu_T *menu)
{
    vimmenu_T   *mp;
    int		idx = 0;
    pumitem_T	*array;
#ifdef FEAT_BEVAL_TERM
    int		save_bevalterm = p_bevalterm;
#endif
    int		mode;

    pum_undisplay();
    pum_size = 0;
    mode = get_menu_mode_flag();

    for (mp = menu->children; mp != NULL; mp = mp->next)
	if (menu_is_separator(mp->dname)
		|| (mp->modes & mp->enabled & mode))
	    ++pum_size;

    array = (pumitem_T *)alloc_clear((unsigned)sizeof(pumitem_T) * pum_size);
    if (array == NULL)
	return;

    for (mp = menu->children; mp != NULL; mp = mp->next)
	if (menu_is_separator(mp->dname))
	    array[idx++].pum_text = (char_u *)"";
	else if (mp->modes & mp->enabled & mode)
	    array[idx++].pum_text = mp->dname;

    pum_array = array;
    pum_compute_size();
    pum_scrollbar = 0;
    pum_height = pum_size;
    pum_position_at_mouse(20);

    pum_selected = -1;
    pum_first = 0;
#  ifdef FEAT_BEVAL_TERM
    p_bevalterm = TRUE;  /* track mouse movement */
    mch_setmouse(TRUE);
#  endif

    for (;;)
    {
	int	c;

	pum_redraw();
	setcursor_mayforce(TRUE);
	out_flush();

	c = vgetc();
	if (c == ESC || c == Ctrl_C)
	    break;
	else if (c == CAR || c == NL)
	{
	    /* enter: select current item, if any, and close */
	    pum_execute_menu(menu, mode);
	    break;
	}
	else if (c == 'k' || c == K_UP || c == K_MOUSEUP)
	{
	    /* cursor up: select previous item */
	    while (pum_selected > 0)
	    {
		--pum_selected;
		if (*array[pum_selected].pum_text != NUL)
		    break;
	    }
	}
	else if (c == 'j' || c == K_DOWN || c == K_MOUSEDOWN)
	{
	    /* cursor down: select next item */
	    while (pum_selected < pum_size - 1)
	    {
		++pum_selected;
		if (*array[pum_selected].pum_text != NUL)
		    break;
	    }
	}
	else if (c == K_RIGHTMOUSE)
	{
	    /* Right mouse down: reposition the menu. */
	    vungetc(c);
	    break;
	}
	else if (c == K_LEFTDRAG || c == K_RIGHTDRAG || c == K_MOUSEMOVE)
	{
	    /* mouse moved: select item in the mouse row */
	    pum_select_mouse_pos();
	}
	else if (c == K_LEFTMOUSE || c == K_LEFTMOUSE_NM || c == K_RIGHTRELEASE)
	{
	    /* left mouse click: select clicked item, if any, and close;
	     * right mouse release: select clicked item, close if any */
	    pum_select_mouse_pos();
	    if (pum_selected >= 0)
	    {
		pum_execute_menu(menu, mode);
		break;
	    }
	    if (c == K_LEFTMOUSE || c == K_LEFTMOUSE_NM)
		break;
	}
    }

    vim_free(array);
    pum_undisplay();
#  ifdef FEAT_BEVAL_TERM
    p_bevalterm = save_bevalterm;
    mch_setmouse(TRUE);
#  endif
}

    void
pum_make_popup(char_u *path_name, int use_mouse_pos)
{
    vimmenu_T *menu;

    if (!use_mouse_pos)
    {
	/* Hack: set mouse position at the cursor so that the menu pops up
	 * around there. */
	mouse_row = curwin->w_winrow + curwin->w_wrow;
	mouse_col = curwin->w_wincol + curwin->w_wcol;
    }

    menu = gui_find_menu(path_name);
    if (menu != NULL)
	pum_show_popupmenu(menu);
}
# endif

#endif
