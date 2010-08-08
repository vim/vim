/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * move.c: Functions for moving the cursor and scrolling text.
 *
 * There are two ways to move the cursor:
 * 1. Move the cursor directly, the text is scrolled to keep the cursor in the
 *    window.
 * 2. Scroll the text, the cursor is moved into the text visible in the
 *    window.
 * The 'scrolloff' option makes this a bit complicated.
 */

#include "vim.h"

static void comp_botline __ARGS((win_T *wp));
static int scrolljump_value __ARGS((void));
static int check_top_offset __ARGS((void));
static void curs_rows __ARGS((win_T *wp, int do_botline));
static void validate_botline_win __ARGS((win_T *wp));
static void validate_cheight __ARGS((void));

typedef struct
{
    linenr_T	    lnum;	/* line number */
#ifdef FEAT_DIFF
    int		    fill;	/* filler lines */
#endif
    int		    height;	/* height of added line */
} lineoff_T;

static void topline_back __ARGS((lineoff_T *lp));
static void botline_forw __ARGS((lineoff_T *lp));
#ifdef FEAT_DIFF
static void botline_topline __ARGS((lineoff_T *lp));
static void topline_botline __ARGS((lineoff_T *lp));
static void max_topfill __ARGS((void));
#endif

/*
 * Compute wp->w_botline for the current wp->w_topline.  Can be called after
 * wp->w_topline changed.
 */
    static void
comp_botline(wp)
    win_T	*wp;
{
    int		n;
    linenr_T	lnum;
    int		done;
#ifdef FEAT_FOLDING
    linenr_T    last;
    int		folded;
#endif

    /*
     * If w_cline_row is valid, start there.
     * Otherwise have to start at w_topline.
     */
    check_cursor_moved(wp);
    if (wp->w_valid & VALID_CROW)
    {
	lnum = wp->w_cursor.lnum;
	done = wp->w_cline_row;
    }
    else
    {
	lnum = wp->w_topline;
	done = 0;
    }

    for ( ; lnum <= wp->w_buffer->b_ml.ml_line_count; ++lnum)
    {
#ifdef FEAT_FOLDING
	last = lnum;
	folded = FALSE;
	if (hasFoldingWin(wp, lnum, NULL, &last, TRUE, NULL))
	{
	    n = 1;
	    folded = TRUE;
	}
	else
#endif
#ifdef FEAT_DIFF
	    if (lnum == wp->w_topline)
		n = plines_win_nofill(wp, lnum, TRUE) + wp->w_topfill;
	    else
#endif
		n = plines_win(wp, lnum, TRUE);
	if (
#ifdef FEAT_FOLDING
		lnum <= wp->w_cursor.lnum && last >= wp->w_cursor.lnum
#else
		lnum == wp->w_cursor.lnum
#endif
	   )
	{
	    wp->w_cline_row = done;
	    wp->w_cline_height = n;
#ifdef FEAT_FOLDING
	    wp->w_cline_folded = folded;
#endif
	    wp->w_valid |= (VALID_CROW|VALID_CHEIGHT);
	}
	if (done + n > wp->w_height)
	    break;
	done += n;
#ifdef FEAT_FOLDING
	lnum = last;
#endif
    }

    /* wp->w_botline is the line that is just below the window */
    wp->w_botline = lnum;
    wp->w_valid |= VALID_BOTLINE|VALID_BOTLINE_AP;

    set_empty_rows(wp, done);
}

/*
 * Update curwin->w_topline and redraw if necessary.
 * Used to update the screen before printing a message.
 */
    void
update_topline_redraw()
{
    update_topline();
    if (must_redraw)
	update_screen(0);
}

/*
 * Update curwin->w_topline to move the cursor onto the screen.
 */
    void
update_topline()
{
    long	line_count;
    int		halfheight;
    int		n;
    linenr_T	old_topline;
#ifdef FEAT_DIFF
    int		old_topfill;
#endif
#ifdef FEAT_FOLDING
    linenr_T	lnum;
#endif
    int		check_topline = FALSE;
    int		check_botline = FALSE;
#ifdef FEAT_MOUSE
    int		save_so = p_so;
#endif

    if (!screen_valid(TRUE))
	return;

    check_cursor_moved(curwin);
    if (curwin->w_valid & VALID_TOPLINE)
	return;

#ifdef FEAT_MOUSE
    /* When dragging with the mouse, don't scroll that quickly */
    if (mouse_dragging > 0)
	p_so = mouse_dragging - 1;
#endif

    old_topline = curwin->w_topline;
#ifdef FEAT_DIFF
    old_topfill = curwin->w_topfill;
#endif

    /*
     * If the buffer is empty, always set topline to 1.
     */
    if (bufempty())		/* special case - file is empty */
    {
	if (curwin->w_topline != 1)
	    redraw_later(NOT_VALID);
	curwin->w_topline = 1;
	curwin->w_botline = 2;
	curwin->w_valid |= VALID_BOTLINE|VALID_BOTLINE_AP;
#ifdef FEAT_SCROLLBIND
	curwin->w_scbind_pos = 1;
#endif
    }

    /*
     * If the cursor is above or near the top of the window, scroll the window
     * to show the line the cursor is in, with 'scrolloff' context.
     */
    else
    {
	if (curwin->w_topline > 1)
	{
	    /* If the cursor is above topline, scrolling is always needed.
	     * If the cursor is far below topline and there is no folding,
	     * scrolling down is never needed. */
	    if (curwin->w_cursor.lnum < curwin->w_topline)
		check_topline = TRUE;
	    else if (check_top_offset())
		check_topline = TRUE;
	}
#ifdef FEAT_DIFF
	    /* Check if there are more filler lines than allowed. */
	if (!check_topline && curwin->w_topfill > diff_check_fill(curwin,
							   curwin->w_topline))
	    check_topline = TRUE;
#endif

	if (check_topline)
	{
	    halfheight = curwin->w_height / 2 - 1;
	    if (halfheight < 2)
		halfheight = 2;

#ifdef FEAT_FOLDING
	    if (hasAnyFolding(curwin))
	    {
		/* Count the number of logical lines between the cursor and
		 * topline + p_so (approximation of how much will be
		 * scrolled). */
		n = 0;
		for (lnum = curwin->w_cursor.lnum;
				      lnum < curwin->w_topline + p_so; ++lnum)
		{
		    ++n;
		    /* stop at end of file or when we know we are far off */
		    if (lnum >= curbuf->b_ml.ml_line_count || n >= halfheight)
			break;
		    (void)hasFolding(lnum, NULL, &lnum);
		}
	    }
	    else
#endif
		n = curwin->w_topline + p_so - curwin->w_cursor.lnum;

	    /* If we weren't very close to begin with, we scroll to put the
	     * cursor in the middle of the window.  Otherwise put the cursor
	     * near the top of the window. */
	    if (n >= halfheight)
		scroll_cursor_halfway(FALSE);
	    else
	    {
		scroll_cursor_top(scrolljump_value(), FALSE);
		check_botline = TRUE;
	    }
	}

	else
	{
#ifdef FEAT_FOLDING
	    /* Make sure topline is the first line of a fold. */
	    (void)hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif
	    check_botline = TRUE;
	}
    }

    /*
     * If the cursor is below the bottom of the window, scroll the window
     * to put the cursor on the window.
     * When w_botline is invalid, recompute it first, to avoid a redraw later.
     * If w_botline was approximated, we might need a redraw later in a few
     * cases, but we don't want to spend (a lot of) time recomputing w_botline
     * for every small change.
     */
    if (check_botline)
    {
	if (!(curwin->w_valid & VALID_BOTLINE_AP))
	    validate_botline();

	if (curwin->w_botline <= curbuf->b_ml.ml_line_count)
	{
	    if (curwin->w_cursor.lnum < curwin->w_botline)
	    {
	      if (((long)curwin->w_cursor.lnum
					     >= (long)curwin->w_botline - p_so
#ifdef FEAT_FOLDING
			|| hasAnyFolding(curwin)
#endif
			))
	      {
		lineoff_T	loff;

		/* Cursor is (a few lines) above botline, check if there are
		 * 'scrolloff' window lines below the cursor.  If not, need to
		 * scroll. */
		n = curwin->w_empty_rows;
		loff.lnum = curwin->w_cursor.lnum;
#ifdef FEAT_FOLDING
		/* In a fold go to its last line. */
		(void)hasFolding(loff.lnum, NULL, &loff.lnum);
#endif
#ifdef FEAT_DIFF
		loff.fill = 0;
		n += curwin->w_filler_rows;
#endif
		loff.height = 0;
		while (loff.lnum < curwin->w_botline
#ifdef FEAT_DIFF
			&& (loff.lnum + 1 < curwin->w_botline || loff.fill == 0)
#endif
			)
		{
		    n += loff.height;
		    if (n >= p_so)
			break;
		    botline_forw(&loff);
		}
		if (n >= p_so)
		    /* sufficient context, no need to scroll */
		    check_botline = FALSE;
	      }
	      else
		  /* sufficient context, no need to scroll */
		  check_botline = FALSE;
	    }
	    if (check_botline)
	    {
#ifdef FEAT_FOLDING
		if (hasAnyFolding(curwin))
		{
		    /* Count the number of logical lines between the cursor and
		     * botline - p_so (approximation of how much will be
		     * scrolled). */
		    line_count = 0;
		    for (lnum = curwin->w_cursor.lnum;
				     lnum >= curwin->w_botline - p_so; --lnum)
		    {
			++line_count;
			/* stop at end of file or when we know we are far off */
			if (lnum <= 0 || line_count > curwin->w_height + 1)
			    break;
			(void)hasFolding(lnum, &lnum, NULL);
		    }
		}
		else
#endif
		    line_count = curwin->w_cursor.lnum - curwin->w_botline
								   + 1 + p_so;
		if (line_count <= curwin->w_height + 1)
		    scroll_cursor_bot(scrolljump_value(), FALSE);
		else
		    scroll_cursor_halfway(FALSE);
	    }
	}
    }
    curwin->w_valid |= VALID_TOPLINE;

    /*
     * Need to redraw when topline changed.
     */
    if (curwin->w_topline != old_topline
#ifdef FEAT_DIFF
	    || curwin->w_topfill != old_topfill
#endif
	    )
    {
	dollar_vcol = 0;
	if (curwin->w_skipcol != 0)
	{
	    curwin->w_skipcol = 0;
	    redraw_later(NOT_VALID);
	}
	else
	    redraw_later(VALID);
	/* May need to set w_skipcol when cursor in w_topline. */
	if (curwin->w_cursor.lnum == curwin->w_topline)
	    validate_cursor();
    }

#ifdef FEAT_MOUSE
    p_so = save_so;
#endif
}

/*
 * Return the scrolljump value to use for the current window.
 * When 'scrolljump' is positive use it as-is.
 * When 'scrolljump' is negative use it as a percentage of the window height.
 */
    static int
scrolljump_value()
{
    if (p_sj >= 0)
	return (int)p_sj;
    return (curwin->w_height * -p_sj) / 100;
}

/*
 * Return TRUE when there are not 'scrolloff' lines above the cursor for the
 * current window.
 */
    static int
check_top_offset()
{
    lineoff_T	loff;
    int		n;

    if (curwin->w_cursor.lnum < curwin->w_topline + p_so
#ifdef FEAT_FOLDING
		    || hasAnyFolding(curwin)
#endif
	    )
    {
	loff.lnum = curwin->w_cursor.lnum;
#ifdef FEAT_DIFF
	loff.fill = 0;
	n = curwin->w_topfill;	    /* always have this context */
#else
	n = 0;
#endif
	/* Count the visible screen lines above the cursor line. */
	while (n < p_so)
	{
	    topline_back(&loff);
	    /* Stop when included a line above the window. */
	    if (loff.lnum < curwin->w_topline
#ifdef FEAT_DIFF
		    || (loff.lnum == curwin->w_topline && loff.fill > 0)
#endif
		    )
		break;
	    n += loff.height;
	}
	if (n < p_so)
	    return TRUE;
    }
    return FALSE;
}

    void
update_curswant()
{
    if (curwin->w_set_curswant)
    {
	validate_virtcol();
	curwin->w_curswant = curwin->w_virtcol;
	curwin->w_set_curswant = FALSE;
    }
}

/*
 * Check if the cursor has moved.  Set the w_valid flag accordingly.
 */
    void
check_cursor_moved(wp)
    win_T	*wp;
{
    if (wp->w_cursor.lnum != wp->w_valid_cursor.lnum)
    {
	wp->w_valid &= ~(VALID_WROW|VALID_WCOL|VALID_VIRTCOL
				     |VALID_CHEIGHT|VALID_CROW|VALID_TOPLINE);
	wp->w_valid_cursor = wp->w_cursor;
	wp->w_valid_leftcol = wp->w_leftcol;
    }
    else if (wp->w_cursor.col != wp->w_valid_cursor.col
	     || wp->w_leftcol != wp->w_valid_leftcol
#ifdef FEAT_VIRTUALEDIT
	     || wp->w_cursor.coladd != wp->w_valid_cursor.coladd
#endif
	     )
    {
	wp->w_valid &= ~(VALID_WROW|VALID_WCOL|VALID_VIRTCOL);
	wp->w_valid_cursor.col = wp->w_cursor.col;
	wp->w_valid_leftcol = wp->w_leftcol;
#ifdef FEAT_VIRTUALEDIT
	wp->w_valid_cursor.coladd = wp->w_cursor.coladd;
#endif
    }
}

/*
 * Call this function when some window settings have changed, which require
 * the cursor position, botline and topline to be recomputed and the window to
 * be redrawn.  E.g, when changing the 'wrap' option or folding.
 */
    void
changed_window_setting()
{
    changed_window_setting_win(curwin);
}

    void
changed_window_setting_win(wp)
    win_T	*wp;
{
    wp->w_lines_valid = 0;
    changed_line_abv_curs_win(wp);
    wp->w_valid &= ~(VALID_BOTLINE|VALID_BOTLINE_AP|VALID_TOPLINE);
    redraw_win_later(wp, NOT_VALID);
}

/*
 * Set wp->w_topline to a certain number.
 */
    void
set_topline(wp, lnum)
    win_T	*wp;
    linenr_T	lnum;
{
#ifdef FEAT_FOLDING
    /* go to first of folded lines */
    (void)hasFoldingWin(wp, lnum, &lnum, NULL, TRUE, NULL);
#endif
    /* Approximate the value of w_botline */
    wp->w_botline += lnum - wp->w_topline;
    wp->w_topline = lnum;
#ifdef FEAT_AUTOCMD
    wp->w_topline_was_set = TRUE;
#endif
#ifdef FEAT_DIFF
    wp->w_topfill = 0;
#endif
    wp->w_valid &= ~(VALID_WROW|VALID_CROW|VALID_BOTLINE|VALID_TOPLINE);
    /* Don't set VALID_TOPLINE here, 'scrolloff' needs to be checked. */
    redraw_later(VALID);
}

/*
 * Call this function when the length of the cursor line (in screen
 * characters) has changed, and the change is before the cursor.
 * Need to take care of w_botline separately!
 */
    void
changed_cline_bef_curs()
{
    curwin->w_valid &= ~(VALID_WROW|VALID_WCOL|VALID_VIRTCOL
						|VALID_CHEIGHT|VALID_TOPLINE);
}

    void
changed_cline_bef_curs_win(wp)
    win_T	*wp;
{
    wp->w_valid &= ~(VALID_WROW|VALID_WCOL|VALID_VIRTCOL
						|VALID_CHEIGHT|VALID_TOPLINE);
}

/*
 * Call this function when the length of a line (in screen characters) above
 * the cursor have changed.
 * Need to take care of w_botline separately!
 */
    void
changed_line_abv_curs()
{
    curwin->w_valid &= ~(VALID_WROW|VALID_WCOL|VALID_VIRTCOL|VALID_CROW
						|VALID_CHEIGHT|VALID_TOPLINE);
}

    void
changed_line_abv_curs_win(wp)
    win_T	*wp;
{
    wp->w_valid &= ~(VALID_WROW|VALID_WCOL|VALID_VIRTCOL|VALID_CROW
						|VALID_CHEIGHT|VALID_TOPLINE);
}

/*
 * Make sure the value of curwin->w_botline is valid.
 */
    void
validate_botline()
{
    if (!(curwin->w_valid & VALID_BOTLINE))
	comp_botline(curwin);
}

/*
 * Make sure the value of wp->w_botline is valid.
 */
    static void
validate_botline_win(wp)
    win_T	*wp;
{
    if (!(wp->w_valid & VALID_BOTLINE))
	comp_botline(wp);
}

/*
 * Mark curwin->w_botline as invalid (because of some change in the buffer).
 */
    void
invalidate_botline()
{
    curwin->w_valid &= ~(VALID_BOTLINE|VALID_BOTLINE_AP);
}

    void
invalidate_botline_win(wp)
    win_T	*wp;
{
    wp->w_valid &= ~(VALID_BOTLINE|VALID_BOTLINE_AP);
}

    void
approximate_botline_win(wp)
    win_T	*wp;
{
    wp->w_valid &= ~VALID_BOTLINE;
}

/*
 * Return TRUE if curwin->w_wrow and curwin->w_wcol are valid.
 */
    int
cursor_valid()
{
    check_cursor_moved(curwin);
    return ((curwin->w_valid & (VALID_WROW|VALID_WCOL)) ==
						      (VALID_WROW|VALID_WCOL));
}

/*
 * Validate cursor position.  Makes sure w_wrow and w_wcol are valid.
 * w_topline must be valid, you may need to call update_topline() first!
 */
    void
validate_cursor()
{
    check_cursor_moved(curwin);
    if ((curwin->w_valid & (VALID_WCOL|VALID_WROW)) != (VALID_WCOL|VALID_WROW))
	curs_columns(TRUE);
}

#if defined(FEAT_GUI) || defined(PROTO)
/*
 * validate w_cline_row.
 */
    void
validate_cline_row()
{
    /*
     * First make sure that w_topline is valid (after moving the cursor).
     */
    update_topline();
    check_cursor_moved(curwin);
    if (!(curwin->w_valid & VALID_CROW))
	curs_rows(curwin, FALSE);
}
#endif

/*
 * Compute wp->w_cline_row and wp->w_cline_height, based on the current value
 * of wp->w_topline.
 *
 * Returns OK when cursor is in the window, FAIL when it isn't.
 */
    static void
curs_rows(wp, do_botline)
    win_T	*wp;
    int		do_botline;		/* also compute w_botline */
{
    linenr_T	lnum;
    int		i;
    int		all_invalid;
    int		valid;
#ifdef FEAT_FOLDING
    long	fold_count;
#endif

    /* Check if wp->w_lines[].wl_size is invalid */
    all_invalid = (!redrawing()
			|| wp->w_lines_valid == 0
			|| wp->w_lines[0].wl_lnum > wp->w_topline);
    i = 0;
    wp->w_cline_row = 0;
    for (lnum = wp->w_topline; lnum < wp->w_cursor.lnum; ++i)
    {
	valid = FALSE;
	if (!all_invalid && i < wp->w_lines_valid)
	{
	    if (wp->w_lines[i].wl_lnum < lnum || !wp->w_lines[i].wl_valid)
		continue;		/* skip changed or deleted lines */
	    if (wp->w_lines[i].wl_lnum == lnum)
	    {
#ifdef FEAT_FOLDING
		/* Check for newly inserted lines below this row, in which
		 * case we need to check for folded lines. */
		if (!wp->w_buffer->b_mod_set
			|| wp->w_lines[i].wl_lastlnum < wp->w_cursor.lnum
			|| wp->w_buffer->b_mod_top
					     > wp->w_lines[i].wl_lastlnum + 1)
#endif
		valid = TRUE;
	    }
	    else if (wp->w_lines[i].wl_lnum > lnum)
		--i;			/* hold at inserted lines */
	}
	if (valid
#ifdef FEAT_DIFF
		&& (lnum != wp->w_topline || !wp->w_p_diff)
#endif
		)
	{
#ifdef FEAT_FOLDING
	    lnum = wp->w_lines[i].wl_lastlnum + 1;
	    /* Cursor inside folded lines, don't count this row */
	    if (lnum > wp->w_cursor.lnum)
		break;
#else
	    ++lnum;
#endif
	    wp->w_cline_row += wp->w_lines[i].wl_size;
	}
	else
	{
#ifdef FEAT_FOLDING
	    fold_count = foldedCount(wp, lnum, NULL);
	    if (fold_count)
	    {
		lnum += fold_count;
		if (lnum > wp->w_cursor.lnum)
		    break;
		++wp->w_cline_row;
	    }
	    else
#endif
#ifdef FEAT_DIFF
		if (lnum == wp->w_topline)
		    wp->w_cline_row += plines_win_nofill(wp, lnum++, TRUE)
							      + wp->w_topfill;
		else
#endif
		    wp->w_cline_row += plines_win(wp, lnum++, TRUE);
	}
    }

    check_cursor_moved(wp);
    if (!(wp->w_valid & VALID_CHEIGHT))
    {
	if (all_invalid
		|| i == wp->w_lines_valid
		|| (i < wp->w_lines_valid
		    && (!wp->w_lines[i].wl_valid
			|| wp->w_lines[i].wl_lnum != wp->w_cursor.lnum)))
	{
#ifdef FEAT_DIFF
	    if (wp->w_cursor.lnum == wp->w_topline)
		wp->w_cline_height = plines_win_nofill(wp, wp->w_cursor.lnum,
							TRUE) + wp->w_topfill;
	    else
#endif
		wp->w_cline_height = plines_win(wp, wp->w_cursor.lnum, TRUE);
#ifdef FEAT_FOLDING
	    wp->w_cline_folded = hasFoldingWin(wp, wp->w_cursor.lnum,
						      NULL, NULL, TRUE, NULL);
#endif
	}
	else if (i > wp->w_lines_valid)
	{
	    /* a line that is too long to fit on the last screen line */
	    wp->w_cline_height = 0;
#ifdef FEAT_FOLDING
	    wp->w_cline_folded = hasFoldingWin(wp, wp->w_cursor.lnum,
						      NULL, NULL, TRUE, NULL);
#endif
	}
	else
	{
	    wp->w_cline_height = wp->w_lines[i].wl_size;
#ifdef FEAT_FOLDING
	    wp->w_cline_folded = wp->w_lines[i].wl_folded;
#endif
	}
    }

    wp->w_valid |= VALID_CROW|VALID_CHEIGHT;

    /* validate botline too, if update_screen doesn't do it */
    if (do_botline && all_invalid)
	validate_botline_win(wp);
}

/*
 * Validate curwin->w_virtcol only.
 */
    void
validate_virtcol()
{
    validate_virtcol_win(curwin);
}

/*
 * Validate wp->w_virtcol only.
 */
    void
validate_virtcol_win(wp)
    win_T	*wp;
{
    check_cursor_moved(wp);
    if (!(wp->w_valid & VALID_VIRTCOL))
    {
	getvvcol(wp, &wp->w_cursor, NULL, &(wp->w_virtcol), NULL);
	wp->w_valid |= VALID_VIRTCOL;
#ifdef FEAT_SYN_HL
	if (wp->w_p_cuc
# ifdef FEAT_INS_EXPAND
		&& !pum_visible()
# endif
		)
	    redraw_win_later(wp, SOME_VALID);
#endif
    }
}

/*
 * Validate curwin->w_cline_height only.
 */
    static void
validate_cheight()
{
    check_cursor_moved(curwin);
    if (!(curwin->w_valid & VALID_CHEIGHT))
    {
#ifdef FEAT_DIFF
	if (curwin->w_cursor.lnum == curwin->w_topline)
	    curwin->w_cline_height = plines_nofill(curwin->w_cursor.lnum)
							  + curwin->w_topfill;
	else
#endif
	    curwin->w_cline_height = plines(curwin->w_cursor.lnum);
#ifdef FEAT_FOLDING
	curwin->w_cline_folded = hasFolding(curwin->w_cursor.lnum, NULL, NULL);
#endif
	curwin->w_valid |= VALID_CHEIGHT;
    }
}

/*
 * Validate w_wcol and w_virtcol only.
 */
    void
validate_cursor_col()
{
    colnr_T off;
    colnr_T col;
    int     width;

    validate_virtcol();
    if (!(curwin->w_valid & VALID_WCOL))
    {
	col = curwin->w_virtcol;
	off = curwin_col_off();
	col += off;
	width = W_WIDTH(curwin) - off + curwin_col_off2();

	/* long line wrapping, adjust curwin->w_wrow */
	if (curwin->w_p_wrap
		&& col >= (colnr_T)W_WIDTH(curwin)
		&& width > 0)
	    /* use same formula as what is used in curs_columns() */
	    col -= ((col - W_WIDTH(curwin)) / width + 1) * width;
	if (col > (int)curwin->w_leftcol)
	    col -= curwin->w_leftcol;
	else
	    col = 0;
	curwin->w_wcol = col;

	curwin->w_valid |= VALID_WCOL;
    }
}

/*
 * Compute offset of a window, occupied by absolute or relative line number,
 * fold column and sign column (these don't move when scrolling horizontally).
 */
    int
win_col_off(wp)
    win_T	*wp;
{
    return (((wp->w_p_nu || wp->w_p_rnu) ? number_width(wp) + 1 : 0)
#ifdef FEAT_CMDWIN
	    + (cmdwin_type == 0 || wp != curwin ? 0 : 1)
#endif
#ifdef FEAT_FOLDING
	    + wp->w_p_fdc
#endif
#ifdef FEAT_SIGNS
	    + (
# ifdef FEAT_NETBEANS_INTG
		/* show glyph gutter in netbeans */
		netbeans_active() ||
# endif
		wp->w_buffer->b_signlist != NULL ? 2 : 0)
#endif
	   );
}

    int
curwin_col_off()
{
    return win_col_off(curwin);
}

/*
 * Return the difference in column offset for the second screen line of a
 * wrapped line.  It's 8 if 'number' or 'relativenumber' is on and 'n' is in
 * 'cpoptions'.
 */
    int
win_col_off2(wp)
    win_T	*wp;
{
    if ((wp->w_p_nu || wp->w_p_rnu) && vim_strchr(p_cpo, CPO_NUMCOL) != NULL)
	return number_width(wp) + 1;
    return 0;
}

    int
curwin_col_off2()
{
    return win_col_off2(curwin);
}

/*
 * compute curwin->w_wcol and curwin->w_virtcol.
 * Also updates curwin->w_wrow and curwin->w_cline_row.
 * Also updates curwin->w_leftcol.
 */
    void
curs_columns(scroll)
    int		scroll;		/* when TRUE, may scroll horizontally */
{
    int		diff;
    int		extra;		/* offset for first screen line */
    int		off_left, off_right;
    int		n;
    int		p_lines;
    int		width = 0;
    int		textwidth;
    int		new_leftcol;
    colnr_T	startcol;
    colnr_T	endcol;
    colnr_T	prev_skipcol;

    /*
     * First make sure that w_topline is valid (after moving the cursor).
     */
    update_topline();

    /*
     * Next make sure that w_cline_row is valid.
     */
    if (!(curwin->w_valid & VALID_CROW))
	curs_rows(curwin, FALSE);

    /*
     * Compute the number of virtual columns.
     */
#ifdef FEAT_FOLDING
    if (curwin->w_cline_folded)
	/* In a folded line the cursor is always in the first column */
	startcol = curwin->w_virtcol = endcol = curwin->w_leftcol;
    else
#endif
	getvvcol(curwin, &curwin->w_cursor,
				&startcol, &(curwin->w_virtcol), &endcol);

    /* remove '$' from change command when cursor moves onto it */
    if (startcol > dollar_vcol)
	dollar_vcol = 0;

    extra = curwin_col_off();
    curwin->w_wcol = curwin->w_virtcol + extra;
    endcol += extra;

    /*
     * Now compute w_wrow, counting screen lines from w_cline_row.
     */
    curwin->w_wrow = curwin->w_cline_row;

    textwidth = W_WIDTH(curwin) - extra;
    if (textwidth <= 0)
    {
	/* No room for text, put cursor in last char of window. */
	curwin->w_wcol = W_WIDTH(curwin) - 1;
	curwin->w_wrow = curwin->w_height - 1;
    }
    else if (curwin->w_p_wrap
#ifdef FEAT_VERTSPLIT
	    && curwin->w_width != 0
#endif
	    )
    {
	width = textwidth + curwin_col_off2();

	/* long line wrapping, adjust curwin->w_wrow */
	if (curwin->w_wcol >= W_WIDTH(curwin))
	{
	    /* this same formula is used in validate_cursor_col() */
	    n = (curwin->w_wcol - W_WIDTH(curwin)) / width + 1;
	    curwin->w_wcol -= n * width;
	    curwin->w_wrow += n;

#ifdef FEAT_LINEBREAK
	    /* When cursor wraps to first char of next line in Insert
	     * mode, the 'showbreak' string isn't shown, backup to first
	     * column */
	    if (*p_sbr && *ml_get_cursor() == NUL
		    && curwin->w_wcol == (int)vim_strsize(p_sbr))
		curwin->w_wcol = 0;
#endif
	}
    }

    /* No line wrapping: compute curwin->w_leftcol if scrolling is on and line
     * is not folded.
     * If scrolling is off, curwin->w_leftcol is assumed to be 0 */
    else if (scroll
#ifdef FEAT_FOLDING
	    && !curwin->w_cline_folded
#endif
	    )
    {
	/*
	 * If Cursor is left of the screen, scroll rightwards.
	 * If Cursor is right of the screen, scroll leftwards
	 * If we get closer to the edge than 'sidescrolloff', scroll a little
	 * extra
	 */
	off_left = (int)startcol - (int)curwin->w_leftcol - p_siso;
	off_right = (int)endcol - (int)(curwin->w_leftcol + W_WIDTH(curwin)
								- p_siso) + 1;
	if (off_left < 0 || off_right > 0)
	{
	    if (off_left < 0)
		diff = -off_left;
	    else
		diff = off_right;

	    /* When far off or not enough room on either side, put cursor in
	     * middle of window. */
	    if (p_ss == 0 || diff >= textwidth / 2 || off_right >= off_left)
		new_leftcol = curwin->w_wcol - extra - textwidth / 2;
	    else
	    {
		if (diff < p_ss)
		    diff = p_ss;
		if (off_left < 0)
		    new_leftcol = curwin->w_leftcol - diff;
		else
		    new_leftcol = curwin->w_leftcol + diff;
	    }
	    if (new_leftcol < 0)
		new_leftcol = 0;
	    if (new_leftcol != (int)curwin->w_leftcol)
	    {
		curwin->w_leftcol = new_leftcol;
		/* screen has to be redrawn with new curwin->w_leftcol */
		redraw_later(NOT_VALID);
	    }
	}
	curwin->w_wcol -= curwin->w_leftcol;
    }
    else if (curwin->w_wcol > (int)curwin->w_leftcol)
	curwin->w_wcol -= curwin->w_leftcol;
    else
	curwin->w_wcol = 0;

#ifdef FEAT_DIFF
    /* Skip over filler lines.  At the top use w_topfill, there
     * may be some filler lines above the window. */
    if (curwin->w_cursor.lnum == curwin->w_topline)
	curwin->w_wrow += curwin->w_topfill;
    else
	curwin->w_wrow += diff_check_fill(curwin, curwin->w_cursor.lnum);
#endif

    prev_skipcol = curwin->w_skipcol;

    p_lines = 0;
    if ((curwin->w_wrow >= curwin->w_height
		|| ((prev_skipcol > 0
			|| curwin->w_wrow + p_so >= curwin->w_height)
		    && (p_lines =
#ifdef FEAT_DIFF
			plines_win_nofill
#else
			plines_win
#endif
			(curwin, curwin->w_cursor.lnum, FALSE))
						    - 1 >= curwin->w_height))
	    && curwin->w_height != 0
	    && curwin->w_cursor.lnum == curwin->w_topline
	    && width > 0
#ifdef FEAT_VERTSPLIT
	    && curwin->w_width != 0
#endif
	    )
    {
	/* Cursor past end of screen.  Happens with a single line that does
	 * not fit on screen.  Find a skipcol to show the text around the
	 * cursor.  Avoid scrolling all the time. compute value of "extra":
	 * 1: Less than "p_so" lines above
	 * 2: Less than "p_so" lines below
	 * 3: both of them */
	extra = 0;
	if (curwin->w_skipcol + p_so * width > curwin->w_virtcol)
	    extra = 1;
	/* Compute last display line of the buffer line that we want at the
	 * bottom of the window. */
	if (p_lines == 0)
	    p_lines = plines_win(curwin, curwin->w_cursor.lnum, FALSE);
	--p_lines;
	if (p_lines > curwin->w_wrow + p_so)
	    n = curwin->w_wrow + p_so;
	else
	    n = p_lines;
	if ((colnr_T)n >= curwin->w_height + curwin->w_skipcol / width)
	    extra += 2;

	if (extra == 3 || p_lines < p_so * 2)
	{
	    /* not enough room for 'scrolloff', put cursor in the middle */
	    n = curwin->w_virtcol / width;
	    if (n > curwin->w_height / 2)
		n -= curwin->w_height / 2;
	    else
		n = 0;
	    /* don't skip more than necessary */
	    if (n > p_lines - curwin->w_height + 1)
		n = p_lines - curwin->w_height + 1;
	    curwin->w_skipcol = n * width;
	}
	else if (extra == 1)
	{
	    /* less then 'scrolloff' lines above, decrease skipcol */
	    extra = (curwin->w_skipcol + p_so * width - curwin->w_virtcol
				     + width - 1) / width;
	    if (extra > 0)
	    {
		if ((colnr_T)(extra * width) > curwin->w_skipcol)
		    extra = curwin->w_skipcol / width;
		curwin->w_skipcol -= extra * width;
	    }
	}
	else if (extra == 2)
	{
	    /* less then 'scrolloff' lines below, increase skipcol */
	    endcol = (n - curwin->w_height + 1) * width;
	    while (endcol > curwin->w_virtcol)
		endcol -= width;
	    if (endcol > curwin->w_skipcol)
		curwin->w_skipcol = endcol;
	}

	curwin->w_wrow -= curwin->w_skipcol / width;
	if (curwin->w_wrow >= curwin->w_height)
	{
	    /* small window, make sure cursor is in it */
	    extra = curwin->w_wrow - curwin->w_height + 1;
	    curwin->w_skipcol += extra * width;
	    curwin->w_wrow -= extra;
	}

	extra = ((int)prev_skipcol - (int)curwin->w_skipcol) / width;
	if (extra > 0)
	    win_ins_lines(curwin, 0, extra, FALSE, FALSE);
	else if (extra < 0)
	    win_del_lines(curwin, 0, -extra, FALSE, FALSE);
    }
    else
	curwin->w_skipcol = 0;
    if (prev_skipcol != curwin->w_skipcol)
	redraw_later(NOT_VALID);

    /* Redraw when w_row changes and 'relativenumber' is set */
    if (((curwin->w_valid & VALID_WROW) == 0 && (curwin->w_p_rnu
#ifdef FEAT_SYN_HL
	/* or when w_row changes and 'cursorline' is set. */
						|| curwin->w_p_cul
#endif
	))
#ifdef FEAT_SYN_HL
	/* or when w_virtcol changes and 'cursorcolumn' is set */
	|| (curwin->w_p_cuc && (curwin->w_valid & VALID_VIRTCOL) == 0)
#endif
	)
# ifdef FEAT_INS_EXPAND
	    if (!pum_visible())
# endif
		redraw_later(SOME_VALID);

    curwin->w_valid |= VALID_WCOL|VALID_WROW|VALID_VIRTCOL;
}

/*
 * Scroll the current window down by "line_count" logical lines.  "CTRL-Y"
 */
    void
scrolldown(line_count, byfold)
    long	line_count;
    int		byfold UNUSED;	/* TRUE: count a closed fold as one line */
{
    long	done = 0;	/* total # of physical lines done */
    int		wrow;
    int		moved = FALSE;

#ifdef FEAT_FOLDING
    linenr_T	first;

    /* Make sure w_topline is at the first of a sequence of folded lines. */
    (void)hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif
    validate_cursor();		/* w_wrow needs to be valid */
    while (line_count-- > 0)
    {
#ifdef FEAT_DIFF
	if (curwin->w_topfill < diff_check(curwin, curwin->w_topline)
		&& curwin->w_topfill < curwin->w_height - 1)
	{
	    ++curwin->w_topfill;
	    ++done;
	}
	else
#endif
	{
	    if (curwin->w_topline == 1)
		break;
	    --curwin->w_topline;
#ifdef FEAT_DIFF
	    curwin->w_topfill = 0;
#endif
#ifdef FEAT_FOLDING
	    /* A sequence of folded lines only counts for one logical line */
	    if (hasFolding(curwin->w_topline, &first, NULL))
	    {
		++done;
		if (!byfold)
		    line_count -= curwin->w_topline - first - 1;
		curwin->w_botline -= curwin->w_topline - first;
		curwin->w_topline = first;
	    }
	    else
#endif
#ifdef FEAT_DIFF
		done += plines_nofill(curwin->w_topline);
#else
		done += plines(curwin->w_topline);
#endif
	}
	--curwin->w_botline;		/* approximate w_botline */
	invalidate_botline();
    }
    curwin->w_wrow += done;		/* keep w_wrow updated */
    curwin->w_cline_row += done;	/* keep w_cline_row updated */

#ifdef FEAT_DIFF
    if (curwin->w_cursor.lnum == curwin->w_topline)
	curwin->w_cline_row = 0;
    check_topfill(curwin, TRUE);
#endif

    /*
     * Compute the row number of the last row of the cursor line
     * and move the cursor onto the displayed part of the window.
     */
    wrow = curwin->w_wrow;
    if (curwin->w_p_wrap
#ifdef FEAT_VERTSPLIT
		&& curwin->w_width != 0
#endif
	    )
    {
	validate_virtcol();
	validate_cheight();
	wrow += curwin->w_cline_height - 1 -
	    curwin->w_virtcol / W_WIDTH(curwin);
    }
    while (wrow >= curwin->w_height && curwin->w_cursor.lnum > 1)
    {
#ifdef FEAT_FOLDING
	if (hasFolding(curwin->w_cursor.lnum, &first, NULL))
	{
	    --wrow;
	    if (first == 1)
		curwin->w_cursor.lnum = 1;
	    else
		curwin->w_cursor.lnum = first - 1;
	}
	else
#endif
	    wrow -= plines(curwin->w_cursor.lnum--);
	curwin->w_valid &=
	      ~(VALID_WROW|VALID_WCOL|VALID_CHEIGHT|VALID_CROW|VALID_VIRTCOL);
	moved = TRUE;
    }
    if (moved)
    {
#ifdef FEAT_FOLDING
	/* Move cursor to first line of closed fold. */
	foldAdjustCursor();
#endif
	coladvance(curwin->w_curswant);
    }
}

/*
 * Scroll the current window up by "line_count" logical lines.  "CTRL-E"
 */
    void
scrollup(line_count, byfold)
    long	line_count;
    int		byfold UNUSED;	/* TRUE: count a closed fold as one line */
{
#if defined(FEAT_FOLDING) || defined(FEAT_DIFF)
    linenr_T	lnum;

    if (
# ifdef FEAT_FOLDING
	    (byfold && hasAnyFolding(curwin))
#  ifdef FEAT_DIFF
	    ||
#  endif
# endif
# ifdef FEAT_DIFF
	    curwin->w_p_diff
# endif
	    )
    {
	/* count each sequence of folded lines as one logical line */
	lnum = curwin->w_topline;
	while (line_count--)
	{
# ifdef FEAT_DIFF
	    if (curwin->w_topfill > 0)
		--curwin->w_topfill;
	    else
# endif
	    {
# ifdef FEAT_FOLDING
		if (byfold)
		    (void)hasFolding(lnum, NULL, &lnum);
# endif
		if (lnum >= curbuf->b_ml.ml_line_count)
		    break;
		++lnum;
# ifdef FEAT_DIFF
		curwin->w_topfill = diff_check_fill(curwin, lnum);
# endif
	    }
	}
	/* approximate w_botline */
	curwin->w_botline += lnum - curwin->w_topline;
	curwin->w_topline = lnum;
    }
    else
#endif
    {
	curwin->w_topline += line_count;
	curwin->w_botline += line_count;	/* approximate w_botline */
    }

    if (curwin->w_topline > curbuf->b_ml.ml_line_count)
	curwin->w_topline = curbuf->b_ml.ml_line_count;
    if (curwin->w_botline > curbuf->b_ml.ml_line_count + 1)
	curwin->w_botline = curbuf->b_ml.ml_line_count + 1;

#ifdef FEAT_DIFF
    check_topfill(curwin, FALSE);
#endif

#ifdef FEAT_FOLDING
    if (hasAnyFolding(curwin))
	/* Make sure w_topline is at the first of a sequence of folded lines. */
	(void)hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif

    curwin->w_valid &= ~(VALID_WROW|VALID_CROW|VALID_BOTLINE);
    if (curwin->w_cursor.lnum < curwin->w_topline)
    {
	curwin->w_cursor.lnum = curwin->w_topline;
	curwin->w_valid &=
	      ~(VALID_WROW|VALID_WCOL|VALID_CHEIGHT|VALID_CROW|VALID_VIRTCOL);
	coladvance(curwin->w_curswant);
    }
}

#ifdef FEAT_DIFF
/*
 * Don't end up with too many filler lines in the window.
 */
    void
check_topfill(wp, down)
    win_T	*wp;
    int		down;	/* when TRUE scroll down when not enough space */
{
    int		n;

    if (wp->w_topfill > 0)
    {
	n = plines_win_nofill(wp, wp->w_topline, TRUE);
	if (wp->w_topfill + n > wp->w_height)
	{
	    if (down && wp->w_topline > 1)
	    {
		--wp->w_topline;
		wp->w_topfill = 0;
	    }
	    else
	    {
		wp->w_topfill = wp->w_height - n;
		if (wp->w_topfill < 0)
		    wp->w_topfill = 0;
	    }
	}
    }
}

/*
 * Use as many filler lines as possible for w_topline.  Make sure w_topline
 * is still visible.
 */
    static void
max_topfill()
{
    int		n;

    n = plines_nofill(curwin->w_topline);
    if (n >= curwin->w_height)
	curwin->w_topfill = 0;
    else
    {
	curwin->w_topfill = diff_check_fill(curwin, curwin->w_topline);
	if (curwin->w_topfill + n > curwin->w_height)
	    curwin->w_topfill = curwin->w_height - n;
    }
}
#endif

#if defined(FEAT_INS_EXPAND) || defined(PROTO)
/*
 * Scroll the screen one line down, but don't do it if it would move the
 * cursor off the screen.
 */
    void
scrolldown_clamp()
{
    int		end_row;
#ifdef FEAT_DIFF
    int		can_fill = (curwin->w_topfill
				< diff_check_fill(curwin, curwin->w_topline));
#endif

    if (curwin->w_topline <= 1
#ifdef FEAT_DIFF
	    && !can_fill
#endif
	    )
	return;

    validate_cursor();	    /* w_wrow needs to be valid */

    /*
     * Compute the row number of the last row of the cursor line
     * and make sure it doesn't go off the screen. Make sure the cursor
     * doesn't go past 'scrolloff' lines from the screen end.
     */
    end_row = curwin->w_wrow;
#ifdef FEAT_DIFF
    if (can_fill)
	++end_row;
    else
	end_row += plines_nofill(curwin->w_topline - 1);
#else
    end_row += plines(curwin->w_topline - 1);
#endif
    if (curwin->w_p_wrap
#ifdef FEAT_VERTSPLIT
		&& curwin->w_width != 0
#endif
	    )
    {
	validate_cheight();
	validate_virtcol();
	end_row += curwin->w_cline_height - 1 -
	    curwin->w_virtcol / W_WIDTH(curwin);
    }
    if (end_row < curwin->w_height - p_so)
    {
#ifdef FEAT_DIFF
	if (can_fill)
	{
	    ++curwin->w_topfill;
	    check_topfill(curwin, TRUE);
	}
	else
	{
	    --curwin->w_topline;
	    curwin->w_topfill = 0;
	}
#else
	--curwin->w_topline;
#endif
#ifdef FEAT_FOLDING
	hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif
	--curwin->w_botline;	    /* approximate w_botline */
	curwin->w_valid &= ~(VALID_WROW|VALID_CROW|VALID_BOTLINE);
    }
}

/*
 * Scroll the screen one line up, but don't do it if it would move the cursor
 * off the screen.
 */
    void
scrollup_clamp()
{
    int	    start_row;

    if (curwin->w_topline == curbuf->b_ml.ml_line_count
#ifdef FEAT_DIFF
	    && curwin->w_topfill == 0
#endif
	    )
	return;

    validate_cursor();	    /* w_wrow needs to be valid */

    /*
     * Compute the row number of the first row of the cursor line
     * and make sure it doesn't go off the screen. Make sure the cursor
     * doesn't go before 'scrolloff' lines from the screen start.
     */
#ifdef FEAT_DIFF
    start_row = curwin->w_wrow - plines_nofill(curwin->w_topline)
							  - curwin->w_topfill;
#else
    start_row = curwin->w_wrow - plines(curwin->w_topline);
#endif
    if (curwin->w_p_wrap
#ifdef FEAT_VERTSPLIT
		&& curwin->w_width != 0
#endif
	    )
    {
	validate_virtcol();
	start_row -= curwin->w_virtcol / W_WIDTH(curwin);
    }
    if (start_row >= p_so)
    {
#ifdef FEAT_DIFF
	if (curwin->w_topfill > 0)
	    --curwin->w_topfill;
	else
#endif
	{
#ifdef FEAT_FOLDING
	    (void)hasFolding(curwin->w_topline, NULL, &curwin->w_topline);
#endif
	    ++curwin->w_topline;
	}
	++curwin->w_botline;		/* approximate w_botline */
	curwin->w_valid &= ~(VALID_WROW|VALID_CROW|VALID_BOTLINE);
    }
}
#endif /* FEAT_INS_EXPAND */

/*
 * Add one line above "lp->lnum".  This can be a filler line, a closed fold or
 * a (wrapped) text line.  Uses and sets "lp->fill".
 * Returns the height of the added line in "lp->height".
 * Lines above the first one are incredibly high: MAXCOL.
 */
    static void
topline_back(lp)
    lineoff_T	*lp;
{
#ifdef FEAT_DIFF
    if (lp->fill < diff_check_fill(curwin, lp->lnum))
    {
	/* Add a filler line. */
	++lp->fill;
	lp->height = 1;
    }
    else
#endif
    {
	--lp->lnum;
#ifdef FEAT_DIFF
	lp->fill = 0;
#endif
	if (lp->lnum < 1)
	    lp->height = MAXCOL;
	else
#ifdef FEAT_FOLDING
	    if (hasFolding(lp->lnum, &lp->lnum, NULL))
	    /* Add a closed fold */
	    lp->height = 1;
	else
#endif
	{
#ifdef FEAT_DIFF
	    lp->height = plines_nofill(lp->lnum);
#else
	    lp->height = plines(lp->lnum);
#endif
	}
    }
}

/*
 * Add one line below "lp->lnum".  This can be a filler line, a closed fold or
 * a (wrapped) text line.  Uses and sets "lp->fill".
 * Returns the height of the added line in "lp->height".
 * Lines below the last one are incredibly high.
 */
    static void
botline_forw(lp)
    lineoff_T	*lp;
{
#ifdef FEAT_DIFF
    if (lp->fill < diff_check_fill(curwin, lp->lnum + 1))
    {
	/* Add a filler line. */
	++lp->fill;
	lp->height = 1;
    }
    else
#endif
    {
	++lp->lnum;
#ifdef FEAT_DIFF
	lp->fill = 0;
#endif
	if (lp->lnum > curbuf->b_ml.ml_line_count)
	    lp->height = MAXCOL;
	else
#ifdef FEAT_FOLDING
	    if (hasFolding(lp->lnum, NULL, &lp->lnum))
	    /* Add a closed fold */
	    lp->height = 1;
	else
#endif
	{
#ifdef FEAT_DIFF
	    lp->height = plines_nofill(lp->lnum);
#else
	    lp->height = plines(lp->lnum);
#endif
	}
    }
}

#ifdef FEAT_DIFF
/*
 * Switch from including filler lines below lp->lnum to including filler
 * lines above loff.lnum + 1.  This keeps pointing to the same line.
 * When there are no filler lines nothing changes.
 */
    static void
botline_topline(lp)
    lineoff_T	*lp;
{
    if (lp->fill > 0)
    {
	++lp->lnum;
	lp->fill = diff_check_fill(curwin, lp->lnum) - lp->fill + 1;
    }
}

/*
 * Switch from including filler lines above lp->lnum to including filler
 * lines below loff.lnum - 1.  This keeps pointing to the same line.
 * When there are no filler lines nothing changes.
 */
    static void
topline_botline(lp)
    lineoff_T	*lp;
{
    if (lp->fill > 0)
    {
	lp->fill = diff_check_fill(curwin, lp->lnum) - lp->fill + 1;
	--lp->lnum;
    }
}
#endif

/*
 * Recompute topline to put the cursor at the top of the window.
 * Scroll at least "min_scroll" lines.
 * If "always" is TRUE, always set topline (for "zt").
 */
    void
scroll_cursor_top(min_scroll, always)
    int		min_scroll;
    int		always;
{
    int		scrolled = 0;
    int		extra = 0;
    int		used;
    int		i;
    linenr_T	top;		/* just above displayed lines */
    linenr_T	bot;		/* just below displayed lines */
    linenr_T	old_topline = curwin->w_topline;
#ifdef FEAT_DIFF
    linenr_T	old_topfill = curwin->w_topfill;
#endif
    linenr_T	new_topline;
    int		off = p_so;

#ifdef FEAT_MOUSE
    if (mouse_dragging > 0)
	off = mouse_dragging - 1;
#endif

    /*
     * Decrease topline until:
     * - it has become 1
     * - (part of) the cursor line is moved off the screen or
     * - moved at least 'scrolljump' lines and
     * - at least 'scrolloff' lines above and below the cursor
     */
    validate_cheight();
    used = curwin->w_cline_height;
    if (curwin->w_cursor.lnum < curwin->w_topline)
	scrolled = used;

#ifdef FEAT_FOLDING
    if (hasFolding(curwin->w_cursor.lnum, &top, &bot))
    {
	--top;
	++bot;
    }
    else
#endif
    {
	top = curwin->w_cursor.lnum - 1;
	bot = curwin->w_cursor.lnum + 1;
    }
    new_topline = top + 1;

#ifdef FEAT_DIFF
    /* count filler lines of the cursor window as context */
    i = diff_check_fill(curwin, curwin->w_cursor.lnum);
    used += i;
    extra += i;
#endif

    /*
     * Check if the lines from "top" to "bot" fit in the window.  If they do,
     * set new_topline and advance "top" and "bot" to include more lines.
     */
    while (top > 0)
    {
#ifdef FEAT_FOLDING
	if (hasFolding(top, &top, NULL))
	    /* count one logical line for a sequence of folded lines */
	    i = 1;
	else
#endif
	    i = plines(top);
	used += i;
	if (extra + i <= off && bot < curbuf->b_ml.ml_line_count)
	{
#ifdef FEAT_FOLDING
	    if (hasFolding(bot, NULL, &bot))
		/* count one logical line for a sequence of folded lines */
		++used;
	    else
#endif
		used += plines(bot);
	}
	if (used > curwin->w_height)
	    break;
	if (top < curwin->w_topline)
	    scrolled += i;

	/*
	 * If scrolling is needed, scroll at least 'sj' lines.
	 */
	if ((new_topline >= curwin->w_topline || scrolled > min_scroll)
		&& extra >= off)
	    break;

	extra += i;
	new_topline = top;
	--top;
	++bot;
    }

    /*
     * If we don't have enough space, put cursor in the middle.
     * This makes sure we get the same position when using "k" and "j"
     * in a small window.
     */
    if (used > curwin->w_height)
	scroll_cursor_halfway(FALSE);
    else
    {
	/*
	 * If "always" is FALSE, only adjust topline to a lower value, higher
	 * value may happen with wrapping lines
	 */
	if (new_topline < curwin->w_topline || always)
	    curwin->w_topline = new_topline;
	if (curwin->w_topline > curwin->w_cursor.lnum)
	    curwin->w_topline = curwin->w_cursor.lnum;
#ifdef FEAT_DIFF
	curwin->w_topfill = diff_check_fill(curwin, curwin->w_topline);
	if (curwin->w_topfill > 0 && extra > off)
	{
	    curwin->w_topfill -= extra - off;
	    if (curwin->w_topfill < 0)
		curwin->w_topfill = 0;
	}
	check_topfill(curwin, FALSE);
#endif
	if (curwin->w_topline != old_topline
#ifdef FEAT_DIFF
		|| curwin->w_topfill != old_topfill
#endif
		)
	    curwin->w_valid &=
		      ~(VALID_WROW|VALID_CROW|VALID_BOTLINE|VALID_BOTLINE_AP);
	curwin->w_valid |= VALID_TOPLINE;
    }
}

/*
 * Set w_empty_rows and w_filler_rows for window "wp", having used up "used"
 * screen lines for text lines.
 */
    void
set_empty_rows(wp, used)
    win_T	*wp;
    int		used;
{
#ifdef FEAT_DIFF
    wp->w_filler_rows = 0;
#endif
    if (used == 0)
	wp->w_empty_rows = 0;	/* single line that doesn't fit */
    else
    {
	wp->w_empty_rows = wp->w_height - used;
#ifdef FEAT_DIFF
	if (wp->w_botline <= wp->w_buffer->b_ml.ml_line_count)
	{
	    wp->w_filler_rows = diff_check_fill(wp, wp->w_botline);
	    if (wp->w_empty_rows > wp->w_filler_rows)
		wp->w_empty_rows -= wp->w_filler_rows;
	    else
	    {
		wp->w_filler_rows = wp->w_empty_rows;
		wp->w_empty_rows = 0;
	    }
	}
#endif
    }
}

/*
 * Recompute topline to put the cursor at the bottom of the window.
 * Scroll at least "min_scroll" lines.
 * If "set_topbot" is TRUE, set topline and botline first (for "zb").
 * This is messy stuff!!!
 */
    void
scroll_cursor_bot(min_scroll, set_topbot)
    int		min_scroll;
    int		set_topbot;
{
    int		used;
    int		scrolled = 0;
    int		extra = 0;
    int		i;
    linenr_T	line_count;
    linenr_T	old_topline = curwin->w_topline;
    lineoff_T	loff;
    lineoff_T	boff;
#ifdef FEAT_DIFF
    int		old_topfill = curwin->w_topfill;
    int		fill_below_window;
#endif
    linenr_T	old_botline = curwin->w_botline;
    linenr_T	old_valid = curwin->w_valid;
    int		old_empty_rows = curwin->w_empty_rows;
    linenr_T	cln;		    /* Cursor Line Number */

    cln = curwin->w_cursor.lnum;
    if (set_topbot)
    {
	used = 0;
	curwin->w_botline = cln + 1;
#ifdef FEAT_DIFF
	loff.fill = 0;
#endif
	for (curwin->w_topline = curwin->w_botline;
		curwin->w_topline > 1;
		curwin->w_topline = loff.lnum)
	{
	    loff.lnum = curwin->w_topline;
	    topline_back(&loff);
	    if (loff.height == MAXCOL || used + loff.height > curwin->w_height)
		break;
	    used += loff.height;
#ifdef FEAT_DIFF
	    curwin->w_topfill = loff.fill;
#endif
	}
	set_empty_rows(curwin, used);
	curwin->w_valid |= VALID_BOTLINE|VALID_BOTLINE_AP;
	if (curwin->w_topline != old_topline
#ifdef FEAT_DIFF
		|| curwin->w_topfill != old_topfill
#endif
		)
	    curwin->w_valid &= ~(VALID_WROW|VALID_CROW);
    }
    else
	validate_botline();

    /* The lines of the cursor line itself are always used. */
#ifdef FEAT_DIFF
    used = plines_nofill(cln);
#else
    validate_cheight();
    used = curwin->w_cline_height;
#endif

    /* If the cursor is below botline, we will at least scroll by the height
     * of the cursor line.  Correct for empty lines, which are really part of
     * botline. */
    if (cln >= curwin->w_botline)
    {
	scrolled = used;
	if (cln == curwin->w_botline)
	    scrolled -= curwin->w_empty_rows;
    }

    /*
     * Stop counting lines to scroll when
     * - hitting start of the file
     * - scrolled nothing or at least 'sj' lines
     * - at least 'so' lines below the cursor
     * - lines between botline and cursor have been counted
     */
#ifdef FEAT_FOLDING
    if (!hasFolding(curwin->w_cursor.lnum, &loff.lnum, &boff.lnum))
#endif
    {
	loff.lnum = cln;
	boff.lnum = cln;
    }
#ifdef FEAT_DIFF
    loff.fill = 0;
    boff.fill = 0;
    fill_below_window = diff_check_fill(curwin, curwin->w_botline)
						      - curwin->w_filler_rows;
#endif

    while (loff.lnum > 1)
    {
	/* Stop when scrolled nothing or at least "min_scroll", found "extra"
	 * context for 'scrolloff' and counted all lines below the window. */
	if ((((scrolled <= 0 || scrolled >= min_scroll)
			&& extra >= (
#ifdef FEAT_MOUSE
			    mouse_dragging > 0 ? mouse_dragging - 1 :
#endif
			    p_so))
		    || boff.lnum + 1 > curbuf->b_ml.ml_line_count)
		&& loff.lnum <= curwin->w_botline
#ifdef FEAT_DIFF
		&& (loff.lnum < curwin->w_botline
		    || loff.fill >= fill_below_window)
#endif
		)
	    break;

	/* Add one line above */
	topline_back(&loff);
	if (loff.height == MAXCOL)
	    used = MAXCOL;
	else
	    used += loff.height;
	if (used > curwin->w_height)
	    break;
	if (loff.lnum >= curwin->w_botline
#ifdef FEAT_DIFF
		&& (loff.lnum > curwin->w_botline
		    || loff.fill <= fill_below_window)
#endif
		)
	{
	    /* Count screen lines that are below the window. */
	    scrolled += loff.height;
	    if (loff.lnum == curwin->w_botline
#ifdef FEAT_DIFF
			    && boff.fill == 0
#endif
		    )
		scrolled -= curwin->w_empty_rows;
	}

	if (boff.lnum < curbuf->b_ml.ml_line_count)
	{
	    /* Add one line below */
	    botline_forw(&boff);
	    used += boff.height;
	    if (used > curwin->w_height)
		break;
	    if (extra < (
#ifdef FEAT_MOUSE
			mouse_dragging > 0 ? mouse_dragging - 1 :
#endif
			p_so) || scrolled < min_scroll)
	    {
		extra += boff.height;
		if (boff.lnum >= curwin->w_botline
#ifdef FEAT_DIFF
			|| (boff.lnum + 1 == curwin->w_botline
			    && boff.fill > curwin->w_filler_rows)
#endif
		   )
		{
		    /* Count screen lines that are below the window. */
		    scrolled += boff.height;
		    if (boff.lnum == curwin->w_botline
#ifdef FEAT_DIFF
			    && boff.fill == 0
#endif
			    )
			scrolled -= curwin->w_empty_rows;
		}
	    }
	}
    }

    /* curwin->w_empty_rows is larger, no need to scroll */
    if (scrolled <= 0)
	line_count = 0;
    /* more than a screenfull, don't scroll but redraw */
    else if (used > curwin->w_height)
	line_count = used;
    /* scroll minimal number of lines */
    else
    {
	line_count = 0;
#ifdef FEAT_DIFF
	boff.fill = curwin->w_topfill;
#endif
	boff.lnum = curwin->w_topline - 1;
	for (i = 0; i < scrolled && boff.lnum < curwin->w_botline; )
	{
	    botline_forw(&boff);
	    i += boff.height;
	    ++line_count;
	}
	if (i < scrolled)	/* below curwin->w_botline, don't scroll */
	    line_count = 9999;
    }

    /*
     * Scroll up if the cursor is off the bottom of the screen a bit.
     * Otherwise put it at 1/2 of the screen.
     */
    if (line_count >= curwin->w_height && line_count > min_scroll)
	scroll_cursor_halfway(FALSE);
    else
	scrollup(line_count, TRUE);

    /*
     * If topline didn't change we need to restore w_botline and w_empty_rows
     * (we changed them).
     * If topline did change, update_screen() will set botline.
     */
    if (curwin->w_topline == old_topline && set_topbot)
    {
	curwin->w_botline = old_botline;
	curwin->w_empty_rows = old_empty_rows;
	curwin->w_valid = old_valid;
    }
    curwin->w_valid |= VALID_TOPLINE;
}

/*
 * Recompute topline to put the cursor halfway the window
 * If "atend" is TRUE, also put it halfway at the end of the file.
 */
    void
scroll_cursor_halfway(atend)
    int		atend;
{
    int		above = 0;
    linenr_T	topline;
#ifdef FEAT_DIFF
    int		topfill = 0;
#endif
    int		below = 0;
    int		used;
    lineoff_T	loff;
    lineoff_T	boff;

    loff.lnum = boff.lnum = curwin->w_cursor.lnum;
#ifdef FEAT_FOLDING
    (void)hasFolding(loff.lnum, &loff.lnum, &boff.lnum);
#endif
#ifdef FEAT_DIFF
    used = plines_nofill(loff.lnum);
    loff.fill = 0;
    boff.fill = 0;
#else
    used = plines(loff.lnum);
#endif
    topline = loff.lnum;
    while (topline > 1)
    {
	if (below <= above)	    /* add a line below the cursor first */
	{
	    if (boff.lnum < curbuf->b_ml.ml_line_count)
	    {
		botline_forw(&boff);
		used += boff.height;
		if (used > curwin->w_height)
		    break;
		below += boff.height;
	    }
	    else
	    {
		++below;	    /* count a "~" line */
		if (atend)
		    ++used;
	    }
	}

	if (below > above)	    /* add a line above the cursor */
	{
	    topline_back(&loff);
	    if (loff.height == MAXCOL)
		used = MAXCOL;
	    else
		used += loff.height;
	    if (used > curwin->w_height)
		break;
	    above += loff.height;
	    topline = loff.lnum;
#ifdef FEAT_DIFF
	    topfill = loff.fill;
#endif
	}
    }
#ifdef FEAT_FOLDING
    if (!hasFolding(topline, &curwin->w_topline, NULL))
#endif
	curwin->w_topline = topline;
#ifdef FEAT_DIFF
    curwin->w_topfill = topfill;
    check_topfill(curwin, FALSE);
#endif
    curwin->w_valid &= ~(VALID_WROW|VALID_CROW|VALID_BOTLINE|VALID_BOTLINE_AP);
    curwin->w_valid |= VALID_TOPLINE;
}

/*
 * Correct the cursor position so that it is in a part of the screen at least
 * 'so' lines from the top and bottom, if possible.
 * If not possible, put it at the same position as scroll_cursor_halfway().
 * When called topline must be valid!
 */
    void
cursor_correct()
{
    int		above = 0;	    /* screen lines above topline */
    linenr_T	topline;
    int		below = 0;	    /* screen lines below botline */
    linenr_T	botline;
    int		above_wanted, below_wanted;
    linenr_T	cln;		    /* Cursor Line Number */
    int		max_off;

    /*
     * How many lines we would like to have above/below the cursor depends on
     * whether the first/last line of the file is on screen.
     */
    above_wanted = p_so;
    below_wanted = p_so;
#ifdef FEAT_MOUSE
    if (mouse_dragging > 0)
    {
	above_wanted = mouse_dragging - 1;
	below_wanted = mouse_dragging - 1;
    }
#endif
    if (curwin->w_topline == 1)
    {
	above_wanted = 0;
	max_off = curwin->w_height / 2;
	if (below_wanted > max_off)
	    below_wanted = max_off;
    }
    validate_botline();
    if (curwin->w_botline == curbuf->b_ml.ml_line_count + 1
#ifdef FEAT_MOUSE
	    && mouse_dragging == 0
#endif
	    )
    {
	below_wanted = 0;
	max_off = (curwin->w_height - 1) / 2;
	if (above_wanted > max_off)
	    above_wanted = max_off;
    }

    /*
     * If there are sufficient file-lines above and below the cursor, we can
     * return now.
     */
    cln = curwin->w_cursor.lnum;
    if (cln >= curwin->w_topline + above_wanted
	    && cln < curwin->w_botline - below_wanted
#ifdef FEAT_FOLDING
	    && !hasAnyFolding(curwin)
#endif
	    )
	return;

    /*
     * Narrow down the area where the cursor can be put by taking lines from
     * the top and the bottom until:
     * - the desired context lines are found
     * - the lines from the top is past the lines from the bottom
     */
    topline = curwin->w_topline;
    botline = curwin->w_botline - 1;
#ifdef FEAT_DIFF
    /* count filler lines as context */
    above = curwin->w_topfill;
    below = curwin->w_filler_rows;
#endif
    while ((above < above_wanted || below < below_wanted) && topline < botline)
    {
	if (below < below_wanted && (below <= above || above >= above_wanted))
	{
#ifdef FEAT_FOLDING
	    if (hasFolding(botline, &botline, NULL))
		++below;
	    else
#endif
		below += plines(botline);
	    --botline;
	}
	if (above < above_wanted && (above < below || below >= below_wanted))
	{
#ifdef FEAT_FOLDING
	    if (hasFolding(topline, NULL, &topline))
		++above;
	    else
#endif
#ifndef FEAT_DIFF
		above += plines(topline);
#else
		above += plines_nofill(topline);

	    /* Count filler lines below this line as context. */
	    if (topline < botline)
		above += diff_check_fill(curwin, topline + 1);
#endif
	    ++topline;
	}
    }
    if (topline == botline || botline == 0)
	curwin->w_cursor.lnum = topline;
    else if (topline > botline)
	curwin->w_cursor.lnum = botline;
    else
    {
	if (cln < topline && curwin->w_topline > 1)
	{
	    curwin->w_cursor.lnum = topline;
	    curwin->w_valid &=
			    ~(VALID_WROW|VALID_WCOL|VALID_CHEIGHT|VALID_CROW);
	}
	if (cln > botline && curwin->w_botline <= curbuf->b_ml.ml_line_count)
	{
	    curwin->w_cursor.lnum = botline;
	    curwin->w_valid &=
			    ~(VALID_WROW|VALID_WCOL|VALID_CHEIGHT|VALID_CROW);
	}
    }
    curwin->w_valid |= VALID_TOPLINE;
}

static void get_scroll_overlap __ARGS((lineoff_T *lp, int dir));

/*
 * move screen 'count' pages up or down and update screen
 *
 * return FAIL for failure, OK otherwise
 */
    int
onepage(dir, count)
    int		dir;
    long	count;
{
    long	n;
    int		retval = OK;
    lineoff_T	loff;
    linenr_T	old_topline = curwin->w_topline;

    if (curbuf->b_ml.ml_line_count == 1)    /* nothing to do */
    {
	beep_flush();
	return FAIL;
    }

    for ( ; count > 0; --count)
    {
	validate_botline();
	/*
	 * It's an error to move a page up when the first line is already on
	 * the screen.	It's an error to move a page down when the last line
	 * is on the screen and the topline is 'scrolloff' lines from the
	 * last line.
	 */
	if (dir == FORWARD
		? ((curwin->w_topline >= curbuf->b_ml.ml_line_count - p_so)
		    && curwin->w_botline > curbuf->b_ml.ml_line_count)
		: (curwin->w_topline == 1
#ifdef FEAT_DIFF
		    && curwin->w_topfill ==
				    diff_check_fill(curwin, curwin->w_topline)
#endif
		    ))
	{
	    beep_flush();
	    retval = FAIL;
	    break;
	}

#ifdef FEAT_DIFF
	loff.fill = 0;
#endif
	if (dir == FORWARD)
	{
	    if (firstwin == lastwin && p_window > 0 && p_window < Rows - 1)
	    {
		/* Vi compatible scrolling */
		if (p_window <= 2)
		    ++curwin->w_topline;
		else
		    curwin->w_topline += p_window - 2;
		if (curwin->w_topline > curbuf->b_ml.ml_line_count)
		    curwin->w_topline = curbuf->b_ml.ml_line_count;
		curwin->w_cursor.lnum = curwin->w_topline;
	    }
	    else if (curwin->w_botline > curbuf->b_ml.ml_line_count)
	    {
		/* at end of file */
		curwin->w_topline = curbuf->b_ml.ml_line_count;
#ifdef FEAT_DIFF
		curwin->w_topfill = 0;
#endif
		curwin->w_valid &= ~(VALID_WROW|VALID_CROW);
	    }
	    else
	    {
		/* For the overlap, start with the line just below the window
		 * and go upwards. */
		loff.lnum = curwin->w_botline;
#ifdef FEAT_DIFF
		loff.fill = diff_check_fill(curwin, loff.lnum)
						      - curwin->w_filler_rows;
#endif
		get_scroll_overlap(&loff, -1);
		curwin->w_topline = loff.lnum;
#ifdef FEAT_DIFF
		curwin->w_topfill = loff.fill;
		check_topfill(curwin, FALSE);
#endif
		curwin->w_cursor.lnum = curwin->w_topline;
		curwin->w_valid &= ~(VALID_WCOL|VALID_CHEIGHT|VALID_WROW|
				   VALID_CROW|VALID_BOTLINE|VALID_BOTLINE_AP);
	    }
	}
	else	/* dir == BACKWARDS */
	{
#ifdef FEAT_DIFF
	    if (curwin->w_topline == 1)
	    {
		/* Include max number of filler lines */
		max_topfill();
		continue;
	    }
#endif
	    if (firstwin == lastwin && p_window > 0 && p_window < Rows - 1)
	    {
		/* Vi compatible scrolling (sort of) */
		if (p_window <= 2)
		    --curwin->w_topline;
		else
		    curwin->w_topline -= p_window - 2;
		if (curwin->w_topline < 1)
		    curwin->w_topline = 1;
		curwin->w_cursor.lnum = curwin->w_topline + p_window - 1;
		if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
		    curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
		continue;
	    }

	    /* Find the line at the top of the window that is going to be the
	     * line at the bottom of the window.  Make sure this results in
	     * the same line as before doing CTRL-F. */
	    loff.lnum = curwin->w_topline - 1;
#ifdef FEAT_DIFF
	    loff.fill = diff_check_fill(curwin, loff.lnum + 1)
							  - curwin->w_topfill;
#endif
	    get_scroll_overlap(&loff, 1);

	    if (loff.lnum >= curbuf->b_ml.ml_line_count)
	    {
		loff.lnum = curbuf->b_ml.ml_line_count;
#ifdef FEAT_DIFF
		loff.fill = 0;
	    }
	    else
	    {
		botline_topline(&loff);
#endif
	    }
	    curwin->w_cursor.lnum = loff.lnum;

	    /* Find the line just above the new topline to get the right line
	     * at the bottom of the window. */
	    n = 0;
	    while (n <= curwin->w_height && loff.lnum >= 1)
	    {
		topline_back(&loff);
		if (loff.height == MAXCOL)
		    n = MAXCOL;
		else
		    n += loff.height;
	    }
	    if (loff.lnum < 1)			/* at begin of file */
	    {
		curwin->w_topline = 1;
#ifdef FEAT_DIFF
		max_topfill();
#endif
		curwin->w_valid &= ~(VALID_WROW|VALID_CROW|VALID_BOTLINE);
	    }
	    else
	    {
		/* Go two lines forward again. */
#ifdef FEAT_DIFF
		topline_botline(&loff);
#endif
		botline_forw(&loff);
		botline_forw(&loff);
#ifdef FEAT_DIFF
		botline_topline(&loff);
#endif
#ifdef FEAT_FOLDING
		/* We're at the wrong end of a fold now. */
		(void)hasFolding(loff.lnum, &loff.lnum, NULL);
#endif

		/* Always scroll at least one line.  Avoid getting stuck on
		 * very long lines. */
		if (loff.lnum >= curwin->w_topline
#ifdef FEAT_DIFF
			&& (loff.lnum > curwin->w_topline
			    || loff.fill >= curwin->w_topfill)
#endif
			)
		{
#ifdef FEAT_DIFF
		    /* First try using the maximum number of filler lines.  If
		     * that's not enough, backup one line. */
		    loff.fill = curwin->w_topfill;
		    if (curwin->w_topfill < diff_check_fill(curwin,
							   curwin->w_topline))
			max_topfill();
		    if (curwin->w_topfill == loff.fill)
#endif
		    {
			--curwin->w_topline;
#ifdef FEAT_DIFF
			curwin->w_topfill = 0;
#endif
		    }
		    comp_botline(curwin);
		    curwin->w_cursor.lnum = curwin->w_botline - 1;
		    curwin->w_valid &= ~(VALID_WCOL|VALID_CHEIGHT|
			    VALID_WROW|VALID_CROW);
		}
		else
		{
		    curwin->w_topline = loff.lnum;
#ifdef FEAT_DIFF
		    curwin->w_topfill = loff.fill;
		    check_topfill(curwin, FALSE);
#endif
		    curwin->w_valid &= ~(VALID_WROW|VALID_CROW|VALID_BOTLINE);
		}
	    }
	}
    }
#ifdef FEAT_FOLDING
    foldAdjustCursor();
#endif
    cursor_correct();
    if (retval == OK)
	beginline(BL_SOL | BL_FIX);
    curwin->w_valid &= ~(VALID_WCOL|VALID_WROW|VALID_VIRTCOL);

    /*
     * Avoid the screen jumping up and down when 'scrolloff' is non-zero.
     * But make sure we scroll at least one line (happens with mix of long
     * wrapping lines and non-wrapping line).
     */
    if (retval == OK && dir == FORWARD && check_top_offset())
    {
	scroll_cursor_top(1, FALSE);
	if (curwin->w_topline <= old_topline
				  && old_topline < curbuf->b_ml.ml_line_count)
	{
	    curwin->w_topline = old_topline + 1;
#ifdef FEAT_FOLDING
	    (void)hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif
	}
    }

    redraw_later(VALID);
    return retval;
}

/*
 * Decide how much overlap to use for page-up or page-down scrolling.
 * This is symmetric, so that doing both keeps the same lines displayed.
 * Three lines are examined:
 *
 *  before CTRL-F	    after CTRL-F / before CTRL-B
 *     etc.			l1
 *  l1 last but one line	------------
 *  l2 last text line		l2 top text line
 *  -------------		l3 second text line
 *  l3				   etc.
 */
    static void
get_scroll_overlap(lp, dir)
    lineoff_T	*lp;
    int		dir;
{
    int		h1, h2, h3, h4;
    int		min_height = curwin->w_height - 2;
    lineoff_T	loff0, loff1, loff2;

#ifdef FEAT_DIFF
    if (lp->fill > 0)
	lp->height = 1;
    else
	lp->height = plines_nofill(lp->lnum);
#else
    lp->height = plines(lp->lnum);
#endif
    h1 = lp->height;
    if (h1 > min_height)
	return;		/* no overlap */

    loff0 = *lp;
    if (dir > 0)
	botline_forw(lp);
    else
	topline_back(lp);
    h2 = lp->height;
    if (h2 + h1 > min_height)
    {
	*lp = loff0;	/* no overlap */
	return;
    }

    loff1 = *lp;
    if (dir > 0)
	botline_forw(lp);
    else
	topline_back(lp);
    h3 = lp->height;
    if (h3 + h2 > min_height)
    {
	*lp = loff0;	/* no overlap */
	return;
    }

    loff2 = *lp;
    if (dir > 0)
	botline_forw(lp);
    else
	topline_back(lp);
    h4 = lp->height;
    if (h4 + h3 + h2 > min_height || h3 + h2 + h1 > min_height)
	*lp = loff1;	/* 1 line overlap */
    else
	*lp = loff2;	/* 2 lines overlap */
    return;
}

/* #define KEEP_SCREEN_LINE */
/*
 * Scroll 'scroll' lines up or down.
 */
    void
halfpage(flag, Prenum)
    int		flag;
    linenr_T	Prenum;
{
    long	scrolled = 0;
    int		i;
    int		n;
    int		room;

    if (Prenum)
	curwin->w_p_scr = (Prenum > curwin->w_height) ?
						curwin->w_height : Prenum;
    n = (curwin->w_p_scr <= curwin->w_height) ?
				    curwin->w_p_scr : curwin->w_height;

    validate_botline();
    room = curwin->w_empty_rows;
#ifdef FEAT_DIFF
    room += curwin->w_filler_rows;
#endif
    if (flag)
    {
	/*
	 * scroll the text up
	 */
	while (n > 0 && curwin->w_botline <= curbuf->b_ml.ml_line_count)
	{
#ifdef FEAT_DIFF
	    if (curwin->w_topfill > 0)
	    {
		i = 1;
		if (--n < 0 && scrolled > 0)
		    break;
		--curwin->w_topfill;
	    }
	    else
#endif
	    {
#ifdef FEAT_DIFF
		i = plines_nofill(curwin->w_topline);
#else
		i = plines(curwin->w_topline);
#endif
		n -= i;
		if (n < 0 && scrolled > 0)
		    break;
#ifdef FEAT_FOLDING
		(void)hasFolding(curwin->w_topline, NULL, &curwin->w_topline);
#endif
		++curwin->w_topline;
#ifdef FEAT_DIFF
		curwin->w_topfill = diff_check_fill(curwin, curwin->w_topline);
#endif

#ifndef KEEP_SCREEN_LINE
		if (curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
		{
		    ++curwin->w_cursor.lnum;
		    curwin->w_valid &=
				    ~(VALID_VIRTCOL|VALID_CHEIGHT|VALID_WCOL);
		}
#endif
	    }
	    curwin->w_valid &= ~(VALID_CROW|VALID_WROW);
	    scrolled += i;

	    /*
	     * Correct w_botline for changed w_topline.
	     * Won't work when there are filler lines.
	     */
#ifdef FEAT_DIFF
	    if (curwin->w_p_diff)
		curwin->w_valid &= ~(VALID_BOTLINE|VALID_BOTLINE_AP);
	    else
#endif
	    {
		room += i;
		do
		{
		    i = plines(curwin->w_botline);
		    if (i > room)
			break;
#ifdef FEAT_FOLDING
		    (void)hasFolding(curwin->w_botline, NULL,
							  &curwin->w_botline);
#endif
		    ++curwin->w_botline;
		    room -= i;
		} while (curwin->w_botline <= curbuf->b_ml.ml_line_count);
	    }
	}

#ifndef KEEP_SCREEN_LINE
	/*
	 * When hit bottom of the file: move cursor down.
	 */
	if (n > 0)
	{
# ifdef FEAT_FOLDING
	    if (hasAnyFolding(curwin))
	    {
		while (--n >= 0
			&& curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
		{
		    (void)hasFolding(curwin->w_cursor.lnum, NULL,
						      &curwin->w_cursor.lnum);
		    ++curwin->w_cursor.lnum;
		}
	    }
	    else
# endif
		curwin->w_cursor.lnum += n;
	    check_cursor_lnum();
	}
#else
	/* try to put the cursor in the same screen line */
	while ((curwin->w_cursor.lnum < curwin->w_topline || scrolled > 0)
			     && curwin->w_cursor.lnum < curwin->w_botline - 1)
	{
	    scrolled -= plines(curwin->w_cursor.lnum);
	    if (scrolled < 0 && curwin->w_cursor.lnum >= curwin->w_topline)
		break;
# ifdef FEAT_FOLDING
	    (void)hasFolding(curwin->w_cursor.lnum, NULL,
						      &curwin->w_cursor.lnum);
# endif
	    ++curwin->w_cursor.lnum;
	}
#endif
    }
    else
    {
	/*
	 * scroll the text down
	 */
	while (n > 0 && curwin->w_topline > 1)
	{
#ifdef FEAT_DIFF
	    if (curwin->w_topfill < diff_check_fill(curwin, curwin->w_topline))
	    {
		i = 1;
		if (--n < 0 && scrolled > 0)
		    break;
		++curwin->w_topfill;
	    }
	    else
#endif
	    {
#ifdef FEAT_DIFF
		i = plines_nofill(curwin->w_topline - 1);
#else
		i = plines(curwin->w_topline - 1);
#endif
		n -= i;
		if (n < 0 && scrolled > 0)
		    break;
		--curwin->w_topline;
#ifdef FEAT_FOLDING
		(void)hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif
#ifdef FEAT_DIFF
		curwin->w_topfill = 0;
#endif
	    }
	    curwin->w_valid &= ~(VALID_CROW|VALID_WROW|
					      VALID_BOTLINE|VALID_BOTLINE_AP);
	    scrolled += i;
#ifndef KEEP_SCREEN_LINE
	    if (curwin->w_cursor.lnum > 1)
	    {
		--curwin->w_cursor.lnum;
		curwin->w_valid &= ~(VALID_VIRTCOL|VALID_CHEIGHT|VALID_WCOL);
	    }
#endif
	}
#ifndef KEEP_SCREEN_LINE
	/*
	 * When hit top of the file: move cursor up.
	 */
	if (n > 0)
	{
	    if (curwin->w_cursor.lnum <= (linenr_T)n)
		curwin->w_cursor.lnum = 1;
	    else
# ifdef FEAT_FOLDING
	    if (hasAnyFolding(curwin))
	    {
		while (--n >= 0 && curwin->w_cursor.lnum > 1)
		{
		    --curwin->w_cursor.lnum;
		    (void)hasFolding(curwin->w_cursor.lnum,
						&curwin->w_cursor.lnum, NULL);
		}
	    }
	    else
# endif
		curwin->w_cursor.lnum -= n;
	}
#else
	/* try to put the cursor in the same screen line */
	scrolled += n;	    /* move cursor when topline is 1 */
	while (curwin->w_cursor.lnum > curwin->w_topline
	      && (scrolled > 0 || curwin->w_cursor.lnum >= curwin->w_botline))
	{
	    scrolled -= plines(curwin->w_cursor.lnum - 1);
	    if (scrolled < 0 && curwin->w_cursor.lnum < curwin->w_botline)
		break;
	    --curwin->w_cursor.lnum;
# ifdef FEAT_FOLDING
	    foldAdjustCursor();
# endif
	}
#endif
    }
# ifdef FEAT_FOLDING
    /* Move cursor to first line of closed fold. */
    foldAdjustCursor();
# endif
#ifdef FEAT_DIFF
    check_topfill(curwin, !flag);
#endif
    cursor_correct();
    beginline(BL_SOL | BL_FIX);
    redraw_later(VALID);
}

#if defined(FEAT_CURSORBIND) || defined(PROTO)
    void
do_check_cursorbind()
{
    linenr_T	line = curwin->w_cursor.lnum;
    colnr_T	col =  curwin->w_cursor.col;
    win_T	*old_curwin = curwin;
    buf_T	*old_curbuf = curbuf;
# ifdef FEAT_VISUAL
    int		old_VIsual_select = VIsual_select;
    int		old_VIsual_active = VIsual_active;
# endif

    /*
     * loop through the cursorbound windows
     */
# ifdef FEAT_VISUAL
    VIsual_select = VIsual_active = 0;
# endif
    for (curwin = firstwin; curwin; curwin = curwin->w_next)
    {
	curbuf = curwin->w_buffer;
	/* skip original window  and windows with 'noscrollbind' */
	if (curwin != old_curwin && curwin->w_p_crb)
	{
# ifdef FEAT_DIFF
	    if (curwin->w_p_diff)
		curwin->w_cursor.lnum
			= diff_get_corresponding_line(old_curbuf,
						      line,
						      curbuf,
						      curwin->w_cursor.lnum);
	    else
# endif
		curwin->w_cursor.lnum = line;
	    curwin->w_cursor.col = col;

	    /* Make sure the cursor is in a valid position. */
	    check_cursor();
# ifdef FEAT_MBYTE
	    /* Correct cursor for multi-byte character. */
	    if (has_mbyte)
		mb_adjust_cursor();
# endif

	    redraw_later(VALID);
	    update_topline();
# ifdef FEAT_WINDOWS
	    curwin->w_redr_status = TRUE;
# endif
	}
    }

    /*
     * reset current-window
     */
# ifdef FEAT_VISUAL
    VIsual_select = old_VIsual_select;
    VIsual_active = old_VIsual_active;
# endif
    curwin = old_curwin;
    curbuf = old_curbuf;
}
#endif /* FEAT_CURSORBIND */
