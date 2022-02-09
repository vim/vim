/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * drawline.c: Functions for drawing window lines on the screen.
 * This is the middle level, drawscreen. is the higher level and screen.c the
 * lower level.
 */

#include "vim.h"

#ifdef FEAT_SYN_HL
/*
 * Advance **color_cols and return TRUE when there are columns to draw.
 */
    static int
advance_color_col(int vcol, int **color_cols)
{
    while (**color_cols >= 0 && vcol > **color_cols)
	++*color_cols;
    return (**color_cols >= 0);
}
#endif

#ifdef FEAT_SYN_HL
/*
 * Used when 'cursorlineopt' contains "screenline": compute the margins between
 * which the highlighting is used.
 */
    static void
margin_columns_win(win_T *wp, int *left_col, int *right_col)
{
    // cache previous calculations depending on w_virtcol
    static int saved_w_virtcol;
    static win_T *prev_wp;
    static int prev_left_col;
    static int prev_right_col;
    static int prev_col_off;

    int cur_col_off = win_col_off(wp);
    int	width1;
    int	width2;

    if (saved_w_virtcol == wp->w_virtcol
	    && prev_wp == wp && prev_col_off == cur_col_off)
    {
	*right_col = prev_right_col;
	*left_col = prev_left_col;
	return;
    }

    width1 = wp->w_width - cur_col_off;
    width2 = width1 + win_col_off2(wp);

    *left_col = 0;
    *right_col = width1;

    if (wp->w_virtcol >= (colnr_T)width1)
	*right_col = width1 + ((wp->w_virtcol - width1) / width2 + 1) * width2;
    if (wp->w_virtcol >= (colnr_T)width1 && width2 > 0)
	*left_col = (wp->w_virtcol - width1) / width2 * width2 + width1;

    // cache values
    prev_left_col = *left_col;
    prev_right_col = *right_col;
    prev_wp = wp;
    saved_w_virtcol = wp->w_virtcol;
    prev_col_off = cur_col_off;
}
#endif

#ifdef FEAT_SIGNS
/*
 * Return TRUE if CursorLineSign highlight is to be used.
 */
    static int
use_cursor_line_sign(win_T *wp, linenr_T lnum)
{
    return wp->w_p_cul
	    && lnum == wp->w_cursor.lnum
	    && (wp->w_p_culopt_flags & CULOPT_NBR);
}

/*
 * Get information needed to display the sign in line 'lnum' in window 'wp'.
 * If 'nrcol' is TRUE, the sign is going to be displayed in the number column.
 * Otherwise the sign is going to be displayed in the sign column.
 */
    static void
get_sign_display_info(
	int		nrcol,
	win_T		*wp,
	linenr_T	lnum,
	sign_attrs_T	*sattr,
	int		wcr_attr,
	int		row,
	int		startrow,
	int		filler_lines UNUSED,
	int		filler_todo UNUSED,
	int		*c_extrap,
	int		*c_finalp,
	char_u		*extra,
	char_u		**pp_extra,
	int		*n_extrap,
	int		*char_attrp)
{
    int	text_sign;
# ifdef FEAT_SIGN_ICONS
    int	icon_sign;
# endif

    // Draw two cells with the sign value or blank.
    *c_extrap = ' ';
    *c_finalp = NUL;
    if (nrcol)
	*n_extrap = number_width(wp) + 1;
    else
    {
	if (use_cursor_line_sign(wp, lnum))
	    *char_attrp = hl_combine_attr(wcr_attr, HL_ATTR(HLF_CLS));
	else
	    *char_attrp = hl_combine_attr(wcr_attr, HL_ATTR(HLF_SC));
	*n_extrap = 2;
    }

    if (row == startrow
#ifdef FEAT_DIFF
	    + filler_lines && filler_todo <= 0
#endif
       )
    {
	text_sign = (sattr->sat_text != NULL) ? sattr->sat_typenr : 0;
# ifdef FEAT_SIGN_ICONS
	icon_sign = (sattr->sat_icon != NULL) ? sattr->sat_typenr : 0;
	if (gui.in_use && icon_sign != 0)
	{
	    // Use the image in this position.
	    if (nrcol)
	    {
		*c_extrap = NUL;
		sprintf((char *)extra, "%-*c ", number_width(wp), SIGN_BYTE);
		*pp_extra = extra;
		*n_extrap = (int)STRLEN(*pp_extra);
	    }
	    else
		*c_extrap = SIGN_BYTE;
#  ifdef FEAT_NETBEANS_INTG
	    if (netbeans_active() && (buf_signcount(wp->w_buffer, lnum) > 1))
	    {
		if (nrcol)
		{
		    *c_extrap = NUL;
		    sprintf((char *)extra, "%-*c ", number_width(wp),
							MULTISIGN_BYTE);
		    *pp_extra = extra;
		    *n_extrap = (int)STRLEN(*pp_extra);
		}
		else
		    *c_extrap = MULTISIGN_BYTE;
	    }
#  endif
	    *c_finalp = NUL;
	    *char_attrp = icon_sign;
	}
	else
# endif
	    if (text_sign != 0)
	    {
		*pp_extra = sattr->sat_text;
		if (*pp_extra != NULL)
		{
		    if (nrcol)
		    {
			int n, width = number_width(wp) - 2;

			for (n = 0; n < width; n++)
			    extra[n] = ' ';
			extra[n] = 0;
			STRCAT(extra, *pp_extra);
			STRCAT(extra, " ");
			*pp_extra = extra;
		    }
		    *c_extrap = NUL;
		    *c_finalp = NUL;
		    *n_extrap = (int)STRLEN(*pp_extra);
		}

		if (use_cursor_line_sign(wp, lnum) && sattr->sat_culhl > 0)
		    *char_attrp = sattr->sat_culhl;
		else
		    *char_attrp = sattr->sat_texthl;
	    }
    }
}
#endif

#ifdef FEAT_PROP_POPUP
static textprop_T	*current_text_props = NULL;
static buf_T		*current_buf = NULL;

    static int
text_prop_compare(const void *s1, const void *s2)
{
    int  idx1, idx2;
    proptype_T  *pt1, *pt2;
    colnr_T col1, col2;

    idx1 = *(int *)s1;
    idx2 = *(int *)s2;
    pt1 = text_prop_type_by_id(current_buf, current_text_props[idx1].tp_type);
    pt2 = text_prop_type_by_id(current_buf, current_text_props[idx2].tp_type);
    if (pt1 == pt2)
	return 0;
    if (pt1 == NULL)
	return -1;
    if (pt2 == NULL)
	return 1;
    if (pt1->pt_priority != pt2->pt_priority)
	return pt1->pt_priority > pt2->pt_priority ? 1 : -1;
    col1 = current_text_props[idx1].tp_col;
    col2 = current_text_props[idx2].tp_col;
    return col1 == col2 ? 0 : col1 > col2 ? 1 : -1;
}
#endif

/*
 * Display line "lnum" of window 'wp' on the screen.
 * Start at row "startrow", stop when "endrow" is reached.
 * wp->w_virtcol needs to be valid.
 *
 * Return the number of last row the line occupies.
 */
    int
win_line(
    win_T	*wp,
    linenr_T	lnum,
    int		startrow,
    int		endrow,
    int		nochange UNUSED,	// not updating for changed text
    int		number_only)		// only update the number column
{
    int		col = 0;		// visual column on screen
    unsigned	off;			// offset in ScreenLines/ScreenAttrs
    int		c = 0;			// init for GCC
    long	vcol = 0;		// virtual column (for tabs)
#ifdef FEAT_LINEBREAK
    long	vcol_sbr = -1;		// virtual column after showbreak
#endif
    long	vcol_prev = -1;		// "vcol" of previous character
    char_u	*line;			// current line
    char_u	*ptr;			// current position in "line"
    int		row;			// row in the window, excl w_winrow
    int		screen_row;		// row on the screen, incl w_winrow

    char_u	extra[21];		// "%ld " and 'fdc' must fit in here
    int		n_extra = 0;		// number of extra chars
    char_u	*p_extra = NULL;	// string of extra chars, plus NUL
    char_u	*p_extra_free = NULL;   // p_extra needs to be freed
    int		c_extra = NUL;		// extra chars, all the same
    int		c_final = NUL;		// final char, mandatory if set
    int		extra_attr = 0;		// attributes when n_extra != 0
#if defined(FEAT_LINEBREAK) && defined(FEAT_PROP_POPUP)
    int		in_linebreak = FALSE;	// n_extra set for showing linebreak
#endif
    static char_u *at_end_str = (char_u *)""; // used for p_extra when
					// displaying eol at end-of-line
    int		lcs_eol_one = wp->w_lcs_chars.eol; // eol until it's been used
    int		lcs_prec_todo = wp->w_lcs_chars.prec;
					// prec until it's been used

    // saved "extra" items for when draw_state becomes WL_LINE (again)
    int		saved_n_extra = 0;
    char_u	*saved_p_extra = NULL;
    int		saved_c_extra = 0;
    int		saved_c_final = 0;
    int		saved_char_attr = 0;

    int		n_attr = 0;		// chars with special attr
    int		saved_attr2 = 0;	// char_attr saved for n_attr
    int		n_attr3 = 0;		// chars with overruling special attr
    int		saved_attr3 = 0;	// char_attr saved for n_attr3

    int		n_skip = 0;		// nr of chars to skip for 'nowrap'

    int		fromcol = -10;		// start of inverting
    int		tocol = MAXCOL;		// end of inverting
    int		fromcol_prev = -2;	// start of inverting after cursor
    int		noinvcur = FALSE;	// don't invert the cursor
    int		lnum_in_visual_area = FALSE;
    pos_T	pos;
    long	v;

    int		char_attr = 0;		// attributes for next character
    int		attr_pri = FALSE;	// char_attr has priority
    int		area_highlighting = FALSE; // Visual or incsearch highlighting
					   // in this line
    int		vi_attr = 0;		// attributes for Visual and incsearch
					// highlighting
    int		wcr_attr = 0;		// attributes from 'wincolor'
    int		win_attr = 0;		// background for whole window, except
					// margins and "~" lines.
    int		area_attr = 0;		// attributes desired by highlighting
    int		search_attr = 0;	// attributes desired by 'hlsearch'
#ifdef FEAT_SYN_HL
    int		vcol_save_attr = 0;	// saved attr for 'cursorcolumn'
    int		syntax_attr = 0;	// attributes desired by syntax
    int		prev_syntax_col = -1;	// column of prev_syntax_attr
    int		prev_syntax_attr = 0;	// syntax_attr at prev_syntax_col
    int		has_syntax = FALSE;	// this buffer has syntax highl.
    int		save_did_emsg;
    int		draw_color_col = FALSE;	// highlight colorcolumn
    int		*color_cols = NULL;	// pointer to according columns array
#endif
    int		eol_hl_off = 0;		// 1 if highlighted char after EOL
#ifdef FEAT_PROP_POPUP
    int		text_prop_count;
    int		text_prop_next = 0;	// next text property to use
    textprop_T	*text_props = NULL;
    int		*text_prop_idxs = NULL;
    int		text_props_active = 0;
    proptype_T  *text_prop_type = NULL;
    int		text_prop_attr = 0;
    int		text_prop_combine = FALSE;
#endif
#ifdef FEAT_SPELL
    int		has_spell = FALSE;	// this buffer has spell checking
    int		can_spell;
# define SPWORDLEN 150
    char_u	nextline[SPWORDLEN * 2];// text with start of the next line
    int		nextlinecol = 0;	// column where nextline[] starts
    int		nextline_idx = 0;	// index in nextline[] where next line
					// starts
    int		spell_attr = 0;		// attributes desired by spelling
    int		word_end = 0;		// last byte with same spell_attr
    static linenr_T  checked_lnum = 0;	// line number for "checked_col"
    static int	checked_col = 0;	// column in "checked_lnum" up to which
					// there are no spell errors
    static int	cap_col = -1;		// column to check for Cap word
    static linenr_T capcol_lnum = 0;	// line number where "cap_col" used
    int		cur_checked_col = 0;	// checked column for current line
#endif
    int		extra_check = 0;	// has extra highlighting
    int		multi_attr = 0;		// attributes desired by multibyte
    int		mb_l = 1;		// multi-byte byte length
    int		mb_c = 0;		// decoded multi-byte character
    int		mb_utf8 = FALSE;	// screen char is UTF-8 char
    int		u8cc[MAX_MCO];		// composing UTF-8 chars
#if defined(FEAT_DIFF) || defined(FEAT_SIGNS)
    int		filler_lines = 0;	// nr of filler lines to be drawn
    int		filler_todo = 0;	// nr of filler lines still to do + 1
#endif
#ifdef FEAT_DIFF
    hlf_T	diff_hlf = (hlf_T)0;	// type of diff highlighting
    int		change_start = MAXCOL;	// first col of changed area
    int		change_end = -1;	// last col of changed area
#endif
    colnr_T	trailcol = MAXCOL;	// start of trailing spaces
    colnr_T	leadcol = 0;		// start of leading spaces
    int		in_multispace = FALSE;	// in multiple consecutive spaces
    int		multispace_pos = 0;	// position in lcs-multispace string
#ifdef FEAT_LINEBREAK
    int		need_showbreak = FALSE; // overlong line, skipping first x
					// chars
#endif
#if defined(FEAT_SIGNS) || defined(FEAT_QUICKFIX) \
	|| defined(FEAT_SYN_HL) || defined(FEAT_DIFF)
# define LINE_ATTR
    int		line_attr = 0;		// attribute for the whole line
    int		line_attr_save;
#endif
#ifdef FEAT_SIGNS
    int		sign_present = FALSE;
    sign_attrs_T sattr;
    int		num_attr = 0;		// attribute for the number column
#endif
#ifdef FEAT_ARABIC
    int		prev_c = 0;		// previous Arabic character
    int		prev_c1 = 0;		// first composing char for prev_c
#endif
#if defined(LINE_ATTR)
    int		did_line_attr = 0;
#endif
#ifdef FEAT_TERMINAL
    int		get_term_attr = FALSE;
#endif
#ifdef FEAT_SYN_HL
    int		cul_attr = 0;		// set when 'cursorline' active

    // 'cursorlineopt' has "screenline" and cursor is in this line
    int		cul_screenline = FALSE;

    // margin columns for the screen line, needed for when 'cursorlineopt'
    // contains "screenline"
    int		left_curline_col = 0;
    int		right_curline_col = 0;
#endif

    // draw_state: items that are drawn in sequence:
#define WL_START	0		// nothing done yet
#ifdef FEAT_CMDWIN
# define WL_CMDLINE	WL_START + 1	// cmdline window column
#else
# define WL_CMDLINE	WL_START
#endif
#ifdef FEAT_FOLDING
# define WL_FOLD	WL_CMDLINE + 1	// 'foldcolumn'
#else
# define WL_FOLD	WL_CMDLINE
#endif
#ifdef FEAT_SIGNS
# define WL_SIGN	WL_FOLD + 1	// column for signs
#else
# define WL_SIGN	WL_FOLD		// column for signs
#endif
#define WL_NR		WL_SIGN + 1	// line number
#ifdef FEAT_LINEBREAK
# define WL_BRI		WL_NR + 1	// 'breakindent'
#else
# define WL_BRI		WL_NR
#endif
#if defined(FEAT_LINEBREAK) || defined(FEAT_DIFF)
# define WL_SBR		WL_BRI + 1	// 'showbreak' or 'diff'
#else
# define WL_SBR		WL_BRI
#endif
#define WL_LINE		WL_SBR + 1	// text in the line
    int		draw_state = WL_START;	// what to draw next
#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
    int		feedback_col = 0;
    int		feedback_old_attr = -1;
#endif
    int		screen_line_flags = 0;

#if defined(FEAT_CONCEAL) || defined(FEAT_SEARCH_EXTRA)
    int		match_conc	= 0;	// cchar for match functions
    int		on_last_col     = FALSE;
#endif
#ifdef FEAT_CONCEAL
    int		syntax_flags	= 0;
    int		syntax_seqnr	= 0;
    int		prev_syntax_id	= 0;
    int		conceal_attr	= HL_ATTR(HLF_CONCEAL);
    int		is_concealing	= FALSE;
    int		boguscols	= 0;	// nonexistent columns added to force
					// wrapping
    int		vcol_off	= 0;	// offset for concealed characters
    int		did_wcol	= FALSE;
    int		old_boguscols   = 0;
# define VCOL_HLC (vcol - vcol_off)
# define FIX_FOR_BOGUSCOLS \
    { \
	n_extra += vcol_off; \
	vcol -= vcol_off; \
	vcol_off = 0; \
	col -= boguscols; \
	old_boguscols = boguscols; \
	boguscols = 0; \
    }
#else
# define VCOL_HLC (vcol)
#endif

    if (startrow > endrow)		// past the end already!
	return startrow;

    row = startrow;
    screen_row = row + W_WINROW(wp);

    if (!number_only)
    {
	// To speed up the loop below, set extra_check when there is linebreak,
	// trailing white space and/or syntax processing to be done.
#ifdef FEAT_LINEBREAK
	extra_check = wp->w_p_lbr;
#endif
#ifdef FEAT_SYN_HL
	if (syntax_present(wp) && !wp->w_s->b_syn_error
# ifdef SYN_TIME_LIMIT
		&& !wp->w_s->b_syn_slow
# endif
	   )
	{
	    // Prepare for syntax highlighting in this line.  When there is an
	    // error, stop syntax highlighting.
	    save_did_emsg = did_emsg;
	    did_emsg = FALSE;
	    syntax_start(wp, lnum);
	    if (did_emsg)
		wp->w_s->b_syn_error = TRUE;
	    else
	    {
		did_emsg = save_did_emsg;
#ifdef SYN_TIME_LIMIT
		if (!wp->w_s->b_syn_slow)
#endif
		{
		    has_syntax = TRUE;
		    extra_check = TRUE;
		}
	    }
	}

	// Check for columns to display for 'colorcolumn'.
	color_cols = wp->w_p_cc_cols;
	if (color_cols != NULL)
	    draw_color_col = advance_color_col(VCOL_HLC, &color_cols);
#endif

#ifdef FEAT_TERMINAL
	if (term_show_buffer(wp->w_buffer))
	{
	    extra_check = TRUE;
	    get_term_attr = TRUE;
	    win_attr = term_get_attr(wp, lnum, -1);
	}
#endif

#ifdef FEAT_SPELL
	if (wp->w_p_spell
		&& *wp->w_s->b_p_spl != NUL
		&& wp->w_s->b_langp.ga_len > 0
		&& *(char **)(wp->w_s->b_langp.ga_data) != NULL)
	{
	    // Prepare for spell checking.
	    has_spell = TRUE;
	    extra_check = TRUE;

	    // Get the start of the next line, so that words that wrap to the
	    // next line are found too: "et<line-break>al.".
	    // Trick: skip a few chars for C/shell/Vim comments
	    nextline[SPWORDLEN] = NUL;
	    if (lnum < wp->w_buffer->b_ml.ml_line_count)
	    {
		line = ml_get_buf(wp->w_buffer, lnum + 1, FALSE);
		spell_cat_line(nextline + SPWORDLEN, line, SPWORDLEN);
	    }

	    // When a word wrapped from the previous line the start of the
	    // current line is valid.
	    if (lnum == checked_lnum)
		cur_checked_col = checked_col;
	    checked_lnum = 0;

	    // When there was a sentence end in the previous line may require a
	    // word starting with capital in this line.  In line 1 always check
	    // the first word.
	    if (lnum != capcol_lnum)
		cap_col = -1;
	    if (lnum == 1)
		cap_col = 0;
	    capcol_lnum = 0;
	}
#endif

	// handle Visual active in this window
	if (VIsual_active && wp->w_buffer == curwin->w_buffer)
	{
	    pos_T	*top, *bot;

	    if (LTOREQ_POS(curwin->w_cursor, VIsual))
	    {
		// Visual is after curwin->w_cursor
		top = &curwin->w_cursor;
		bot = &VIsual;
	    }
	    else
	    {
		// Visual is before curwin->w_cursor
		top = &VIsual;
		bot = &curwin->w_cursor;
	    }
	    lnum_in_visual_area = (lnum >= top->lnum && lnum <= bot->lnum);
	    if (VIsual_mode == Ctrl_V)
	    {
		// block mode
		if (lnum_in_visual_area)
		{
		    fromcol = wp->w_old_cursor_fcol;
		    tocol = wp->w_old_cursor_lcol;
		}
	    }
	    else
	    {
		// non-block mode
		if (lnum > top->lnum && lnum <= bot->lnum)
		    fromcol = 0;
		else if (lnum == top->lnum)
		{
		    if (VIsual_mode == 'V')	// linewise
			fromcol = 0;
		    else
		    {
			getvvcol(wp, top, (colnr_T *)&fromcol, NULL, NULL);
			if (gchar_pos(top) == NUL)
			    tocol = fromcol + 1;
		    }
		}
		if (VIsual_mode != 'V' && lnum == bot->lnum)
		{
		    if (*p_sel == 'e' && bot->col == 0 && bot->coladd == 0)
		    {
			fromcol = -10;
			tocol = MAXCOL;
		    }
		    else if (bot->col == MAXCOL)
			tocol = MAXCOL;
		    else
		    {
			pos = *bot;
			if (*p_sel == 'e')
			    getvvcol(wp, &pos, (colnr_T *)&tocol, NULL, NULL);
			else
			{
			    getvvcol(wp, &pos, NULL, NULL, (colnr_T *)&tocol);
			    ++tocol;
			}
		    }
		}
	    }

	    // Check if the character under the cursor should not be inverted
	    if (!highlight_match && lnum == curwin->w_cursor.lnum
								&& wp == curwin
#ifdef FEAT_GUI
		    && !gui.in_use
#endif
		    )
		noinvcur = TRUE;

	    // if inverting in this line set area_highlighting
	    if (fromcol >= 0)
	    {
		area_highlighting = TRUE;
		vi_attr = HL_ATTR(HLF_V);
#if defined(FEAT_CLIPBOARD) && defined(FEAT_X11)
		if ((clip_star.available && !clip_star.owned
						      && clip_isautosel_star())
			|| (clip_plus.available && !clip_plus.owned
						     && clip_isautosel_plus()))
		    vi_attr = HL_ATTR(HLF_VNC);
#endif
	    }
	}

	// handle 'incsearch' and ":s///c" highlighting
	else if (highlight_match
		&& wp == curwin
		&& lnum >= curwin->w_cursor.lnum
		&& lnum <= curwin->w_cursor.lnum + search_match_lines)
	{
	    if (lnum == curwin->w_cursor.lnum)
		getvcol(curwin, &(curwin->w_cursor),
					      (colnr_T *)&fromcol, NULL, NULL);
	    else
		fromcol = 0;
	    if (lnum == curwin->w_cursor.lnum + search_match_lines)
	    {
		pos.lnum = lnum;
		pos.col = search_match_endcol;
		getvcol(curwin, &pos, (colnr_T *)&tocol, NULL, NULL);
	    }
	    else
		tocol = MAXCOL;
	    // do at least one character; happens when past end of line
	    if (fromcol == tocol && search_match_endcol)
		tocol = fromcol + 1;
	    area_highlighting = TRUE;
	    vi_attr = HL_ATTR(HLF_I);
	}
    }

#ifdef FEAT_DIFF
    filler_lines = diff_check(wp, lnum);
    if (filler_lines < 0)
    {
	if (filler_lines == -1)
	{
	    if (diff_find_change(wp, lnum, &change_start, &change_end))
		diff_hlf = HLF_ADD;	// added line
	    else if (change_start == 0)
		diff_hlf = HLF_TXD;	// changed text
	    else
		diff_hlf = HLF_CHD;	// changed line
	}
	else
	    diff_hlf = HLF_ADD;		// added line
	filler_lines = 0;
	area_highlighting = TRUE;
    }
    if (lnum == wp->w_topline)
	filler_lines = wp->w_topfill;
    filler_todo = filler_lines;
#endif

#ifdef FEAT_SIGNS
    sign_present = buf_get_signattrs(wp, lnum, &sattr);
    if (sign_present)
	num_attr = sattr.sat_numhl;
#endif

#ifdef LINE_ATTR
# ifdef FEAT_SIGNS
    // If this line has a sign with line highlighting set line_attr.
    if (sign_present)
	line_attr = sattr.sat_linehl;
# endif
# if defined(FEAT_QUICKFIX)
    // Highlight the current line in the quickfix window.
    if (bt_quickfix(wp->w_buffer) && qf_current_entry(wp) == lnum)
	line_attr = HL_ATTR(HLF_QFL);
# endif
    if (line_attr != 0)
	area_highlighting = TRUE;
#endif

    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
    ptr = line;

#ifdef FEAT_SPELL
    if (has_spell && !number_only)
    {
	// For checking first word with a capital skip white space.
	if (cap_col == 0)
	    cap_col = getwhitecols(line);

	// To be able to spell-check over line boundaries copy the end of the
	// current line into nextline[].  Above the start of the next line was
	// copied to nextline[SPWORDLEN].
	if (nextline[SPWORDLEN] == NUL)
	{
	    // No next line or it is empty.
	    nextlinecol = MAXCOL;
	    nextline_idx = 0;
	}
	else
	{
	    v = (long)STRLEN(line);
	    if (v < SPWORDLEN)
	    {
		// Short line, use it completely and append the start of the
		// next line.
		nextlinecol = 0;
		mch_memmove(nextline, line, (size_t)v);
		STRMOVE(nextline + v, nextline + SPWORDLEN);
		nextline_idx = v + 1;
	    }
	    else
	    {
		// Long line, use only the last SPWORDLEN bytes.
		nextlinecol = v - SPWORDLEN;
		mch_memmove(nextline, line + nextlinecol, SPWORDLEN);
		nextline_idx = SPWORDLEN + 1;
	    }
	}
    }
#endif

    if (wp->w_p_list)
    {
	if (wp->w_lcs_chars.space
		|| wp->w_lcs_chars.multispace != NULL
		|| wp->w_lcs_chars.trail
		|| wp->w_lcs_chars.lead
		|| wp->w_lcs_chars.nbsp)
	    extra_check = TRUE;

	// find start of trailing whitespace
	if (wp->w_lcs_chars.trail)
	{
	    trailcol = (colnr_T)STRLEN(ptr);
	    while (trailcol > (colnr_T)0 && VIM_ISWHITE(ptr[trailcol - 1]))
		--trailcol;
	    trailcol += (colnr_T) (ptr - line);
	}
	// find end of leading whitespace
	if (wp->w_lcs_chars.lead)
	{
	    leadcol = 0;
	    while (VIM_ISWHITE(ptr[leadcol]))
		++leadcol;
	    if (ptr[leadcol] == NUL)
		// in a line full of spaces all of them are treated as trailing
		leadcol = (colnr_T)0;
	    else
		// keep track of the first column not filled with spaces
		leadcol += (colnr_T) (ptr - line) + 1;
	}
    }

    wcr_attr = get_wcr_attr(wp);
    if (wcr_attr != 0)
    {
	win_attr = wcr_attr;
	area_highlighting = TRUE;
    }

#ifdef FEAT_PROP_POPUP
    if (WIN_IS_POPUP(wp))
	screen_line_flags |= SLF_POPUP;
#endif

    // 'nowrap' or 'wrap' and a single line that doesn't fit: Advance to the
    // first character to be displayed.
    if (wp->w_p_wrap)
	v = wp->w_skipcol;
    else
	v = wp->w_leftcol;
    if (v > 0 && !number_only)
    {
	char_u	*prev_ptr = ptr;

	while (vcol < v && *ptr != NUL)
	{
	    c = win_lbr_chartabsize(wp, line, ptr, (colnr_T)vcol, NULL);
	    vcol += c;
	    prev_ptr = ptr;
	    MB_PTR_ADV(ptr);
	}

	// When:
	// - 'cuc' is set, or
	// - 'colorcolumn' is set, or
	// - 'virtualedit' is set, or
	// - the visual mode is active,
	// the end of the line may be before the start of the displayed part.
	if (vcol < v && (
#ifdef FEAT_SYN_HL
	     wp->w_p_cuc || draw_color_col ||
#endif
	     virtual_active() ||
	     (VIsual_active && wp->w_buffer == curwin->w_buffer)))
	    vcol = v;

	// Handle a character that's not completely on the screen: Put ptr at
	// that character but skip the first few screen characters.
	if (vcol > v)
	{
	    vcol -= c;
	    ptr = prev_ptr;
	    // If the character fits on the screen, don't need to skip it.
	    // Except for a TAB.
	    if (( (*mb_ptr2cells)(ptr) >= c || *ptr == TAB) && col == 0)
	       n_skip = v - vcol;
	}

	// Adjust for when the inverted text is before the screen,
	// and when the start of the inverted text is before the screen.
	if (tocol <= vcol)
	    fromcol = 0;
	else if (fromcol >= 0 && fromcol < vcol)
	    fromcol = vcol;

#ifdef FEAT_LINEBREAK
	// When w_skipcol is non-zero, first line needs 'showbreak'
	if (wp->w_p_wrap)
	    need_showbreak = TRUE;
#endif
#ifdef FEAT_SPELL
	// When spell checking a word we need to figure out the start of the
	// word and if it's badly spelled or not.
	if (has_spell)
	{
	    int		len;
	    colnr_T	linecol = (colnr_T)(ptr - line);
	    hlf_T	spell_hlf = HLF_COUNT;

	    pos = wp->w_cursor;
	    wp->w_cursor.lnum = lnum;
	    wp->w_cursor.col = linecol;
	    len = spell_move_to(wp, FORWARD, TRUE, TRUE, &spell_hlf);

	    // spell_move_to() may call ml_get() and make "line" invalid
	    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
	    ptr = line + linecol;

	    if (len == 0 || (int)wp->w_cursor.col > ptr - line)
	    {
		// no bad word found at line start, don't check until end of a
		// word
		spell_hlf = HLF_COUNT;
		word_end = (int)(spell_to_word_end(ptr, wp) - line + 1);
	    }
	    else
	    {
		// bad word found, use attributes until end of word
		word_end = wp->w_cursor.col + len + 1;

		// Turn index into actual attributes.
		if (spell_hlf != HLF_COUNT)
		    spell_attr = highlight_attr[spell_hlf];
	    }
	    wp->w_cursor = pos;

# ifdef FEAT_SYN_HL
	    // Need to restart syntax highlighting for this line.
	    if (has_syntax)
		syntax_start(wp, lnum);
# endif
	}
#endif
    }

    // Correct highlighting for cursor that can't be disabled.
    // Avoids having to check this for each character.
    if (fromcol >= 0)
    {
	if (noinvcur)
	{
	    if ((colnr_T)fromcol == wp->w_virtcol)
	    {
		// highlighting starts at cursor, let it start just after the
		// cursor
		fromcol_prev = fromcol;
		fromcol = -1;
	    }
	    else if ((colnr_T)fromcol < wp->w_virtcol)
		// restart highlighting after the cursor
		fromcol_prev = wp->w_virtcol;
	}
	if (fromcol >= tocol)
	    fromcol = -1;
    }

#ifdef FEAT_SEARCH_EXTRA
    if (!number_only)
    {
	v = (long)(ptr - line);
	area_highlighting |= prepare_search_hl_line(wp, lnum, (colnr_T)v,
					      &line, &screen_search_hl,
					      &search_attr);
	ptr = line + v; // "line" may have been updated
    }
#endif

#ifdef FEAT_SYN_HL
    // Cursor line highlighting for 'cursorline' in the current window.
    if (wp->w_p_cul && lnum == wp->w_cursor.lnum)
    {
	// Do not show the cursor line in the text when Visual mode is active,
	// because it's not clear what is selected then.  Do update
	// w_last_cursorline.
	if (!(wp == curwin && VIsual_active)
					 && wp->w_p_culopt_flags != CULOPT_NBR)
	{
	    cul_screenline = (wp->w_p_wrap
				   && (wp->w_p_culopt_flags & CULOPT_SCRLINE));

	    // Only set line_attr here when "screenline" is not present in
	    // 'cursorlineopt'.  Otherwise it's done later.
	    if (!cul_screenline)
	    {
		cul_attr = HL_ATTR(HLF_CUL);
# ifdef FEAT_SIGNS
		// Combine the 'cursorline' and sign highlighting, depending on
		// the sign priority.
		if (sign_present && sattr.sat_linehl > 0)
		{
		    if (sattr.sat_priority >= 100)
			line_attr = hl_combine_attr(cul_attr, line_attr);
		    else
			line_attr = hl_combine_attr(line_attr, cul_attr);
		}
		else
# endif
		    line_attr = cul_attr;
		wp->w_last_cursorline = wp->w_cursor.lnum;
	    }
	    else
	    {
		line_attr_save = line_attr;
		wp->w_last_cursorline = 0;
		margin_columns_win(wp, &left_curline_col, &right_curline_col);
	    }
	    area_highlighting = TRUE;
	}
	else
	    wp->w_last_cursorline = wp->w_cursor.lnum;
    }
#endif

#ifdef FEAT_PROP_POPUP
    {
	char_u *prop_start;

	text_prop_count = get_text_props(wp->w_buffer, lnum,
							   &prop_start, FALSE);
	if (text_prop_count > 0)
	{
	    // Make a copy of the properties, so that they are properly
	    // aligned.
	    text_props = ALLOC_MULT(textprop_T, text_prop_count);
	    if (text_props != NULL)
		mch_memmove(text_props, prop_start,
					 text_prop_count * sizeof(textprop_T));

	    // Allocate an array for the indexes.
	    text_prop_idxs = ALLOC_MULT(int, text_prop_count);
	    area_highlighting = TRUE;
	    extra_check = TRUE;
	}
    }
#endif

    off = (unsigned)(current_ScreenLine - ScreenLines);
    col = 0;

#ifdef FEAT_RIGHTLEFT
    if (wp->w_p_rl)
    {
	// Rightleft window: process the text in the normal direction, but put
	// it in current_ScreenLine[] from right to left.  Start at the
	// rightmost column of the window.
	col = wp->w_width - 1;
	off += col;
	screen_line_flags |= SLF_RIGHTLEFT;
    }
#endif

    // Repeat for the whole displayed line.
    for (;;)
    {
#if defined(FEAT_CONCEAL) || defined(FEAT_SEARCH_EXTRA)
	int has_match_conc = 0;	// match wants to conceal
#endif
#ifdef FEAT_CONCEAL
	int did_decrement_ptr = FALSE;
#endif
	// Skip this quickly when working on the text.
	if (draw_state != WL_LINE)
	{
#ifdef FEAT_SYN_HL
	    if (cul_screenline)
	    {
		cul_attr = 0;
		line_attr = line_attr_save;
	    }
#endif

#ifdef FEAT_CMDWIN
	    if (draw_state == WL_CMDLINE - 1 && n_extra == 0)
	    {
		draw_state = WL_CMDLINE;
		if (cmdwin_type != 0 && wp == curwin)
		{
		    // Draw the cmdline character.
		    n_extra = 1;
		    c_extra = cmdwin_type;
		    c_final = NUL;
		    char_attr = hl_combine_attr(wcr_attr, HL_ATTR(HLF_AT));
		}
	    }
#endif

#ifdef FEAT_FOLDING
	    if (draw_state == WL_FOLD - 1 && n_extra == 0)
	    {
		int fdc = compute_foldcolumn(wp, 0);

		draw_state = WL_FOLD;
		if (fdc > 0)
		{
		    // Draw the 'foldcolumn'.  Allocate a buffer, "extra" may
		    // already be in use.
		    vim_free(p_extra_free);
		    p_extra_free = alloc(MAX_MCO * fdc + 1);
		    if (p_extra_free != NULL)
		    {
			n_extra = (int)fill_foldcolumn(p_extra_free, wp,
								  FALSE, lnum);
			p_extra_free[n_extra] = NUL;
			p_extra = p_extra_free;
			c_extra = NUL;
			c_final = NUL;
			if (use_cursor_line_sign(wp, lnum))
			    char_attr =
				   hl_combine_attr(wcr_attr, HL_ATTR(HLF_CLF));
			else
			    char_attr =
				    hl_combine_attr(wcr_attr, HL_ATTR(HLF_FC));
		    }
		}
	    }
#endif

#ifdef FEAT_SIGNS
	    if (draw_state == WL_SIGN - 1 && n_extra == 0)
	    {
		draw_state = WL_SIGN;
		// Show the sign column when there are any signs in this
		// buffer or when using Netbeans.
		if (signcolumn_on(wp))
		    get_sign_display_info(FALSE, wp, lnum, &sattr, wcr_attr,
			    row, startrow, filler_lines, filler_todo, &c_extra,
			    &c_final, extra, &p_extra, &n_extra, &char_attr);
	    }
#endif

	    if (draw_state == WL_NR - 1 && n_extra == 0)
	    {
		draw_state = WL_NR;
		// Display the absolute or relative line number. After the
		// first fill with blanks when the 'n' flag isn't in 'cpo'
		if ((wp->w_p_nu || wp->w_p_rnu)
			&& (row == startrow
#ifdef FEAT_DIFF
			    + filler_lines
#endif
			    || vim_strchr(p_cpo, CPO_NUMCOL) == NULL))
		{
#ifdef FEAT_SIGNS
		    // If 'signcolumn' is set to 'number' and a sign is present
		    // in 'lnum', then display the sign instead of the line
		    // number.
		    if ((*wp->w_p_scl == 'n' && *(wp->w_p_scl + 1) == 'u')
			    && sign_present)
			get_sign_display_info(TRUE, wp, lnum, &sattr, wcr_attr,
				row, startrow, filler_lines, filler_todo,
				&c_extra, &c_final, extra, &p_extra, &n_extra,
				&char_attr);
		    else
#endif
		    {
		      // Draw the line number (empty space after wrapping).
		      if (row == startrow
#ifdef FEAT_DIFF
			    + filler_lines
#endif
			    )
		      {
			long num;
			char *fmt = "%*ld ";

			if (wp->w_p_nu && !wp->w_p_rnu)
			    // 'number' + 'norelativenumber'
			    num = (long)lnum;
			else
			{
			    // 'relativenumber', don't use negative numbers
			    num = labs((long)get_cursor_rel_lnum(wp, lnum));
			    if (num == 0 && wp->w_p_nu && wp->w_p_rnu)
			    {
				// 'number' + 'relativenumber'
				num = lnum;
				fmt = "%-*ld ";
			    }
			}

			sprintf((char *)extra, fmt,
						number_width(wp), num);
			if (wp->w_skipcol > 0)
			    for (p_extra = extra; *p_extra == ' '; ++p_extra)
				*p_extra = '-';
#ifdef FEAT_RIGHTLEFT
			if (wp->w_p_rl)		    // reverse line numbers
			{
			    char_u	*p1, *p2;
			    int		t;

			    // like rl_mirror(), but keep the space at the end
			    p2 = skipwhite(extra);
			    p2 = skiptowhite(p2) - 1;
			    for (p1 = skipwhite(extra); p1 < p2; ++p1, --p2)
			    {
				t = *p1;
				*p1 = *p2;
				*p2 = t;
			    }
			}
#endif
			p_extra = extra;
			c_extra = NUL;
			c_final = NUL;
		      }
		      else
		      {
			c_extra = ' ';
			c_final = NUL;
		      }
		      n_extra = number_width(wp) + 1;
		      char_attr = hl_combine_attr(wcr_attr, HL_ATTR(HLF_N));
#ifdef FEAT_SYN_HL
		      // When 'cursorline' is set highlight the line number of
		      // the current line differently.
		      // When 'cursorlineopt' has "screenline" only highlight
		      // the line number itself.
		      // TODO: Can we use CursorLine instead of CursorLineNr
		      // when CursorLineNr isn't set?
		      if (wp->w_p_cul
			      && lnum == wp->w_cursor.lnum
			      && (wp->w_p_culopt_flags & CULOPT_NBR)
			      && (row == startrow
				  || wp->w_p_culopt_flags & CULOPT_LINE))
			char_attr = hl_combine_attr(wcr_attr, HL_ATTR(HLF_CLN));
#endif
		      if (wp->w_p_rnu && lnum < wp->w_cursor.lnum
						      && HL_ATTR(HLF_LNA) != 0)
			  // Use LineNrAbove
			  char_attr = hl_combine_attr(wcr_attr,
							     HL_ATTR(HLF_LNA));
		      if (wp->w_p_rnu && lnum > wp->w_cursor.lnum
						      && HL_ATTR(HLF_LNB) != 0)
			  // Use LineNrBelow
			  char_attr = hl_combine_attr(wcr_attr,
							     HL_ATTR(HLF_LNB));
		    }
#ifdef FEAT_SIGNS
		    if (num_attr)
			char_attr = num_attr;
#endif
		}
	    }

#ifdef FEAT_LINEBREAK
	    if (wp->w_briopt_sbr && draw_state == WL_BRI - 1
			    && n_extra == 0 && *get_showbreak_value(wp) != NUL)
		// draw indent after showbreak value
		draw_state = WL_BRI;
	    else if (wp->w_briopt_sbr && draw_state == WL_SBR && n_extra == 0)
		// After the showbreak, draw the breakindent
		draw_state = WL_BRI - 1;

	    // draw 'breakindent': indent wrapped text accordingly
	    if (draw_state == WL_BRI - 1 && n_extra == 0)
	    {
		draw_state = WL_BRI;
		// if need_showbreak is set, breakindent also applies
		if (wp->w_p_bri && n_extra == 0
					 && (row != startrow || need_showbreak)
# ifdef FEAT_DIFF
			&& filler_lines == 0
# endif
		   )
		{
		    char_attr = 0;
# ifdef FEAT_DIFF
		    if (diff_hlf != (hlf_T)0)
			char_attr = HL_ATTR(diff_hlf);
# endif
		    p_extra = NULL;
		    c_extra = ' ';
		    c_final = NUL;
		    n_extra = get_breakindent_win(wp,
				       ml_get_buf(wp->w_buffer, lnum, FALSE));
		    if (row == startrow)
		    {
			n_extra -= win_col_off2(wp);
			if (n_extra < 0)
			    n_extra = 0;
		    }
		    if (wp->w_skipcol > 0 && wp->w_p_wrap && wp->w_briopt_sbr)
			need_showbreak = FALSE;
		    // Correct end of highlighted area for 'breakindent',
		    // required when 'linebreak' is also set.
		    if (tocol == vcol)
			tocol += n_extra;
		}
	    }
#endif

#if defined(FEAT_LINEBREAK) || defined(FEAT_DIFF)
	    if (draw_state == WL_SBR - 1 && n_extra == 0)
	    {
		char_u *sbr;

		draw_state = WL_SBR;
# ifdef FEAT_DIFF
		if (filler_todo > 0)
		{
		    // Draw "deleted" diff line(s).
		    if (char2cells(fill_diff) > 1)
		    {
			c_extra = '-';
			c_final = NUL;
		    }
		    else
		    {
			c_extra = fill_diff;
			c_final = NUL;
		    }
#  ifdef FEAT_RIGHTLEFT
		    if (wp->w_p_rl)
			n_extra = col + 1;
		    else
#  endif
			n_extra = wp->w_width - col;
		    char_attr = HL_ATTR(HLF_DED);
		}
# endif
# ifdef FEAT_LINEBREAK
		sbr = get_showbreak_value(wp);
		if (*sbr != NUL && need_showbreak)
		{
		    // Draw 'showbreak' at the start of each broken line.
		    p_extra = sbr;
		    c_extra = NUL;
		    c_final = NUL;
		    n_extra = (int)STRLEN(sbr);
		    if (wp->w_skipcol == 0 || !wp->w_p_wrap)
			need_showbreak = FALSE;
		    vcol_sbr = vcol + MB_CHARLEN(sbr);
		    // Correct end of highlighted area for 'showbreak',
		    // required when 'linebreak' is also set.
		    if (tocol == vcol)
			tocol += n_extra;
		    // combine 'showbreak' with 'wincolor'
		    char_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_AT));
#  ifdef FEAT_SYN_HL
		    // combine 'showbreak' with 'cursorline'
		    if (cul_attr != 0)
			char_attr = hl_combine_attr(char_attr, cul_attr);
#  endif
		}
# endif
	    }
#endif

	    if (draw_state == WL_LINE - 1 && n_extra == 0)
	    {
		draw_state = WL_LINE;
		if (saved_n_extra)
		{
		    // Continue item from end of wrapped line.
		    n_extra = saved_n_extra;
		    c_extra = saved_c_extra;
		    c_final = saved_c_final;
		    p_extra = saved_p_extra;
		    char_attr = saved_char_attr;
		}
		else
		    char_attr = win_attr;
	    }
	}
#ifdef FEAT_SYN_HL
	if (cul_screenline && draw_state == WL_LINE
		&& vcol >= left_curline_col
		&& vcol < right_curline_col)
	{
	    cul_attr = HL_ATTR(HLF_CUL);
	    line_attr = cul_attr;
	}
#endif

	// When still displaying '$' of change command, stop at cursor.
	// When only displaying the (relative) line number and that's done,
	// stop here.
	if (((dollar_vcol >= 0 && wp == curwin
		   && lnum == wp->w_cursor.lnum && vcol >= (long)wp->w_virtcol)
		|| (number_only && draw_state > WL_NR))
#ifdef FEAT_DIFF
				   && filler_todo <= 0
#endif
		)
	{
	    screen_line(screen_row, wp->w_wincol, col, -wp->w_width,
							    screen_line_flags);
	    // Pretend we have finished updating the window.  Except when
	    // 'cursorcolumn' is set.
#ifdef FEAT_SYN_HL
	    if (wp->w_p_cuc)
		row = wp->w_cline_row + wp->w_cline_height;
	    else
#endif
		row = wp->w_height;
	    break;
	}

	if (draw_state == WL_LINE && (area_highlighting || extra_check))
	{
	    // handle Visual or match highlighting in this line
	    if (vcol == fromcol
		    || (has_mbyte && vcol + 1 == fromcol && n_extra == 0
			&& (*mb_ptr2cells)(ptr) > 1)
		    || ((int)vcol_prev == fromcol_prev
			&& vcol_prev < vcol	// not at margin
			&& vcol < tocol))
		area_attr = vi_attr;		// start highlighting
	    else if (area_attr != 0
		    && (vcol == tocol
			|| (noinvcur && (colnr_T)vcol == wp->w_virtcol)))
		area_attr = 0;			// stop highlighting

#ifdef FEAT_SEARCH_EXTRA
	    if (!n_extra)
	    {
		// Check for start/end of 'hlsearch' and other matches.
		// After end, check for start/end of next match.
		// When another match, have to check for start again.
		v = (long)(ptr - line);
		search_attr = update_search_hl(wp, lnum, (colnr_T)v, &line,
				      &screen_search_hl, &has_match_conc,
				      &match_conc, did_line_attr, lcs_eol_one,
				      &on_last_col);
		ptr = line + v;  // "line" may have been changed

		// Do not allow a conceal over EOL otherwise EOL will be missed
		// and bad things happen.
		if (*ptr == NUL)
		    has_match_conc = 0;
	    }
#endif

#ifdef FEAT_DIFF
	    if (diff_hlf != (hlf_T)0)
	    {
		if (diff_hlf == HLF_CHD && ptr - line >= change_start
							      && n_extra == 0)
		    diff_hlf = HLF_TXD;		// changed text
		if (diff_hlf == HLF_TXD && ptr - line > change_end
							      && n_extra == 0)
		    diff_hlf = HLF_CHD;		// changed line
		line_attr = HL_ATTR(diff_hlf);
		if (wp->w_p_cul && lnum == wp->w_cursor.lnum
			&& wp->w_p_culopt_flags != CULOPT_NBR
			&& (!cul_screenline || (vcol >= left_curline_col
						&& vcol <= right_curline_col)))
		    line_attr = hl_combine_attr(
					  line_attr, HL_ATTR(HLF_CUL));
	    }
#endif

#ifdef FEAT_PROP_POPUP
	    if (text_props != NULL)
	    {
		int pi;
		int bcol = (int)(ptr - line);

		if (n_extra > 0
# ifdef FEAT_LINEBREAK
			&& !in_linebreak
# endif
			)
		    --bcol;  // still working on the previous char, e.g. Tab

		// Check if any active property ends.
		for (pi = 0; pi < text_props_active; ++pi)
		{
		    int tpi = text_prop_idxs[pi];

		    if (bcol >= text_props[tpi].tp_col - 1
						  + text_props[tpi].tp_len)
		    {
			if (pi + 1 < text_props_active)
			    mch_memmove(text_prop_idxs + pi,
					text_prop_idxs + pi + 1,
					sizeof(int)
					 * (text_props_active - (pi + 1)));
			--text_props_active;
			--pi;
# ifdef FEAT_LINEBREAK
			// not exactly right but should work in most cases
			if (in_linebreak && syntax_attr == text_prop_attr)
			    syntax_attr = 0;
# endif
		    }
		}

# ifdef FEAT_LINEBREAK
		if (n_extra > 0 && in_linebreak)
		    // not on the next char yet, don't start another prop
		    --bcol;
# endif
		// Add any text property that starts in this column.
		while (text_prop_next < text_prop_count
			   && bcol >= text_props[text_prop_next].tp_col - 1)
		{
		    if (bcol <= text_props[text_prop_next].tp_col - 1
					   + text_props[text_prop_next].tp_len)
			text_prop_idxs[text_props_active++] = text_prop_next;
		    ++text_prop_next;
		}

		text_prop_attr = 0;
		text_prop_combine = FALSE;
		text_prop_type = NULL;
		if (text_props_active > 0)
		{
		    // Sort the properties on priority and/or starting last.
		    // Then combine the attributes, highest priority last.
		    current_text_props = text_props;
		    current_buf = wp->w_buffer;
		    qsort((void *)text_prop_idxs, (size_t)text_props_active,
					       sizeof(int), text_prop_compare);

		    for (pi = 0; pi < text_props_active; ++pi)
		    {
			int	    tpi = text_prop_idxs[pi];
			proptype_T  *pt = text_prop_type_by_id(
					wp->w_buffer, text_props[tpi].tp_type);

			if (pt != NULL && pt->pt_hl_id > 0)
			{
			    int pt_attr = syn_id2attr(pt->pt_hl_id);

			    text_prop_type = pt;
			    text_prop_attr =
				      hl_combine_attr(text_prop_attr, pt_attr);
			    text_prop_combine = pt->pt_flags & PT_FLAG_COMBINE;
			}
		    }
		}
	    }
#endif

#ifdef FEAT_SYN_HL
	    if (extra_check && n_extra == 0)
	    {
		syntax_attr = 0;
# ifdef FEAT_TERMINAL
		if (get_term_attr)
		    syntax_attr = term_get_attr(wp, lnum, vcol);
# endif
		// Get syntax attribute.
		if (has_syntax)
		{
		    // Get the syntax attribute for the character.  If there
		    // is an error, disable syntax highlighting.
		    save_did_emsg = did_emsg;
		    did_emsg = FALSE;

		    v = (long)(ptr - line);
		    if (v == prev_syntax_col)
			// at same column again
			syntax_attr = prev_syntax_attr;
		    else
		    {
# ifdef FEAT_SPELL
			can_spell = TRUE;
# endif
			syntax_attr = get_syntax_attr((colnr_T)v,
# ifdef FEAT_SPELL
						has_spell ? &can_spell :
# endif
						NULL, FALSE);
			prev_syntax_col = v;
			prev_syntax_attr = syntax_attr;
		    }

		    if (did_emsg)
		    {
			wp->w_s->b_syn_error = TRUE;
			has_syntax = FALSE;
			syntax_attr = 0;
		    }
		    else
			did_emsg = save_did_emsg;
# ifdef SYN_TIME_LIMIT
		    if (wp->w_s->b_syn_slow)
			has_syntax = FALSE;
# endif

		    // Need to get the line again, a multi-line regexp may
		    // have made it invalid.
		    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
		    ptr = line + v;
# ifdef FEAT_CONCEAL
		    // no concealing past the end of the line, it interferes
		    // with line highlighting
		    if (*ptr == NUL)
			syntax_flags = 0;
		    else
			syntax_flags = get_syntax_info(&syntax_seqnr);
# endif
		}
	    }
# ifdef FEAT_PROP_POPUP
	    // Combine text property highlight into syntax highlight.
	    if (text_prop_type != NULL)
	    {
		if (text_prop_combine)
		    syntax_attr = hl_combine_attr(syntax_attr, text_prop_attr);
		else
		    syntax_attr = text_prop_attr;
	    }
# endif
#endif

	    // Decide which of the highlight attributes to use.
	    attr_pri = TRUE;
#ifdef LINE_ATTR
	    if (area_attr != 0)
	    {
		char_attr = hl_combine_attr(line_attr, area_attr);
		if (!highlight_match)
		    // let search highlight show in Visual area if possible
		    char_attr = hl_combine_attr(search_attr, char_attr);
# ifdef FEAT_SYN_HL
		char_attr = hl_combine_attr(syntax_attr, char_attr);
# endif
	    }
	    else if (search_attr != 0)
	    {
		char_attr = hl_combine_attr(line_attr, search_attr);
# ifdef FEAT_SYN_HL
		char_attr = hl_combine_attr(syntax_attr, char_attr);
# endif
	    }
	    else if (line_attr != 0 && ((fromcol == -10 && tocol == MAXCOL)
				|| vcol < fromcol || vcol_prev < fromcol_prev
				|| vcol >= tocol))
	    {
		// Use line_attr when not in the Visual or 'incsearch' area
		// (area_attr may be 0 when "noinvcur" is set).
# ifdef FEAT_SYN_HL
		char_attr = hl_combine_attr(syntax_attr, line_attr);
# else
		char_attr = line_attr;
# endif
		attr_pri = FALSE;
	    }
#else
	    if (area_attr != 0)
		char_attr = area_attr;
	    else if (search_attr != 0)
		char_attr = search_attr;
#endif
	    else
	    {
		attr_pri = FALSE;
#ifdef FEAT_SYN_HL
		char_attr = syntax_attr;
#else
		char_attr = 0;
#endif
	    }
	}

	// combine attribute with 'wincolor'
	if (win_attr != 0)
	{
	    if (char_attr == 0)
		char_attr = win_attr;
	    else
		char_attr = hl_combine_attr(win_attr, char_attr);
	}

	// Get the next character to put on the screen.

	// The "p_extra" points to the extra stuff that is inserted to
	// represent special characters (non-printable stuff) and other
	// things.  When all characters are the same, c_extra is used.
	// If c_final is set, it will compulsorily be used at the end.
	// "p_extra" must end in a NUL to avoid mb_ptr2len() reads past
	// "p_extra[n_extra]".
	// For the '$' of the 'list' option, n_extra == 1, p_extra == "".
	if (n_extra > 0)
	{
	    if (c_extra != NUL || (n_extra == 1 && c_final != NUL))
	    {
		c = (n_extra == 1 && c_final != NUL) ? c_final : c_extra;
		mb_c = c;	// doesn't handle non-utf-8 multi-byte!
		if (enc_utf8 && utf_char2len(c) > 1)
		{
		    mb_utf8 = TRUE;
		    u8cc[0] = 0;
		    c = 0xc0;
		}
		else
		    mb_utf8 = FALSE;
	    }
	    else
	    {
		c = *p_extra;
		if (has_mbyte)
		{
		    mb_c = c;
		    if (enc_utf8)
		    {
			// If the UTF-8 character is more than one byte:
			// Decode it into "mb_c".
			mb_l = utfc_ptr2len(p_extra);
			mb_utf8 = FALSE;
			if (mb_l > n_extra)
			    mb_l = 1;
			else if (mb_l > 1)
			{
			    mb_c = utfc_ptr2char(p_extra, u8cc);
			    mb_utf8 = TRUE;
			    c = 0xc0;
			}
		    }
		    else
		    {
			// if this is a DBCS character, put it in "mb_c"
			mb_l = MB_BYTE2LEN(c);
			if (mb_l >= n_extra)
			    mb_l = 1;
			else if (mb_l > 1)
			    mb_c = (c << 8) + p_extra[1];
		    }
		    if (mb_l == 0)  // at the NUL at end-of-line
			mb_l = 1;

		    // If a double-width char doesn't fit display a '>' in the
		    // last column.
		    if ((
# ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? (col <= 0) :
# endif
				    (col >= wp->w_width - 1))
			    && (*mb_char2cells)(mb_c) == 2)
		    {
			c = '>';
			mb_c = c;
			mb_l = 1;
			mb_utf8 = FALSE;
			multi_attr = HL_ATTR(HLF_AT);
#ifdef FEAT_SYN_HL
			if (cul_attr)
			    multi_attr = hl_combine_attr(multi_attr, cul_attr);
#endif
			multi_attr = hl_combine_attr(win_attr, multi_attr);

			// put the pointer back to output the double-width
			// character at the start of the next line.
			++n_extra;
			--p_extra;
		    }
		    else
		    {
			n_extra -= mb_l - 1;
			p_extra += mb_l - 1;
		    }
		}
		++p_extra;
	    }
	    --n_extra;
#if defined(FEAT_LINEBREAK) && defined(FEAT_PROP_POPUP)
	    if (n_extra <= 0)
		in_linebreak = FALSE;
#endif
	}
	else
	{
#ifdef FEAT_LINEBREAK
	    int c0;
#endif
	    VIM_CLEAR(p_extra_free);

	    // Get a character from the line itself.
	    c = *ptr;
#ifdef FEAT_LINEBREAK
	    c0 = *ptr;
#endif
	    if (has_mbyte)
	    {
		mb_c = c;
		if (enc_utf8)
		{
		    // If the UTF-8 character is more than one byte: Decode it
		    // into "mb_c".
		    mb_l = utfc_ptr2len(ptr);
		    mb_utf8 = FALSE;
		    if (mb_l > 1)
		    {
			mb_c = utfc_ptr2char(ptr, u8cc);
			// Overlong encoded ASCII or ASCII with composing char
			// is displayed normally, except a NUL.
			if (mb_c < 0x80)
			{
			    c = mb_c;
#ifdef FEAT_LINEBREAK
			    c0 = mb_c;
#endif
			}
			mb_utf8 = TRUE;

			// At start of the line we can have a composing char.
			// Draw it as a space with a composing char.
			if (utf_iscomposing(mb_c))
			{
			    int i;

			    for (i = Screen_mco - 1; i > 0; --i)
				u8cc[i] = u8cc[i - 1];
			    u8cc[0] = mb_c;
			    mb_c = ' ';
			}
		    }

		    if ((mb_l == 1 && c >= 0x80)
			    || (mb_l >= 1 && mb_c == 0)
			    || (mb_l > 1 && (!vim_isprintc(mb_c))))
		    {
			// Illegal UTF-8 byte: display as <xx>.
			// Non-BMP character : display as ? or fullwidth ?.
			transchar_hex(extra, mb_c);
# ifdef FEAT_RIGHTLEFT
			if (wp->w_p_rl)		// reverse
			    rl_mirror(extra);
# endif
			p_extra = extra;
			c = *p_extra;
			mb_c = mb_ptr2char_adv(&p_extra);
			mb_utf8 = (c >= 0x80);
			n_extra = (int)STRLEN(p_extra);
			c_extra = NUL;
			c_final = NUL;
			if (area_attr == 0 && search_attr == 0)
			{
			    n_attr = n_extra + 1;
			    extra_attr = hl_combine_attr(
						     win_attr, HL_ATTR(HLF_8));
			    saved_attr2 = char_attr; // save current attr
			}
		    }
		    else if (mb_l == 0)  // at the NUL at end-of-line
			mb_l = 1;
#ifdef FEAT_ARABIC
		    else if (p_arshape && !p_tbidi && ARABIC_CHAR(mb_c))
		    {
			// Do Arabic shaping.
			int	pc, pc1, nc;
			int	pcc[MAX_MCO];

			// The idea of what is the previous and next
			// character depends on 'rightleft'.
			if (wp->w_p_rl)
			{
			    pc = prev_c;
			    pc1 = prev_c1;
			    nc = utf_ptr2char(ptr + mb_l);
			    prev_c1 = u8cc[0];
			}
			else
			{
			    pc = utfc_ptr2char(ptr + mb_l, pcc);
			    nc = prev_c;
			    pc1 = pcc[0];
			}
			prev_c = mb_c;

			mb_c = arabic_shape(mb_c, &c, &u8cc[0], pc, pc1, nc);
		    }
		    else
			prev_c = mb_c;
#endif
		}
		else	// enc_dbcs
		{
		    mb_l = MB_BYTE2LEN(c);
		    if (mb_l == 0)  // at the NUL at end-of-line
			mb_l = 1;
		    else if (mb_l > 1)
		    {
			// We assume a second byte below 32 is illegal.
			// Hopefully this is OK for all double-byte encodings!
			if (ptr[1] >= 32)
			    mb_c = (c << 8) + ptr[1];
			else
			{
			    if (ptr[1] == NUL)
			    {
				// head byte at end of line
				mb_l = 1;
				transchar_nonprint(wp->w_buffer, extra, c);
			    }
			    else
			    {
				// illegal tail byte
				mb_l = 2;
				STRCPY(extra, "XX");
			    }
			    p_extra = extra;
			    n_extra = (int)STRLEN(extra) - 1;
			    c_extra = NUL;
			    c_final = NUL;
			    c = *p_extra++;
			    if (area_attr == 0 && search_attr == 0)
			    {
				n_attr = n_extra + 1;
				extra_attr = hl_combine_attr(
						     win_attr, HL_ATTR(HLF_8));
				saved_attr2 = char_attr; // save current attr
			    }
			    mb_c = c;
			}
		    }
		}
		// If a double-width char doesn't fit display a '>' in the
		// last column; the character is displayed at the start of the
		// next line.
		if ((
# ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? (col <= 0) :
# endif
				(col >= wp->w_width - 1))
			&& (*mb_char2cells)(mb_c) == 2)
		{
		    c = '>';
		    mb_c = c;
		    mb_utf8 = FALSE;
		    mb_l = 1;
		    multi_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_AT));
		    // Put pointer back so that the character will be
		    // displayed at the start of the next line.
		    --ptr;
#ifdef FEAT_CONCEAL
		    did_decrement_ptr = TRUE;
#endif
		}
		else if (*ptr != NUL)
		    ptr += mb_l - 1;

		// If a double-width char doesn't fit at the left side display
		// a '<' in the first column.  Don't do this for unprintable
		// characters.
		if (n_skip > 0 && mb_l > 1 && n_extra == 0)
		{
		    n_extra = 1;
		    c_extra = MB_FILLER_CHAR;
		    c_final = NUL;
		    c = ' ';
		    if (area_attr == 0 && search_attr == 0)
		    {
			n_attr = n_extra + 1;
			extra_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_AT));
			saved_attr2 = char_attr; // save current attr
		    }
		    mb_c = c;
		    mb_utf8 = FALSE;
		    mb_l = 1;
		}

	    }
	    ++ptr;

	    if (extra_check)
	    {
#ifdef FEAT_SPELL
		// Check spelling (unless at the end of the line).
		// Only do this when there is no syntax highlighting, the
		// @Spell cluster is not used or the current syntax item
		// contains the @Spell cluster.
		v = (long)(ptr - line);
		if (has_spell && v >= word_end && v > cur_checked_col)
		{
		    spell_attr = 0;
		    if (c != 0 && (
# ifdef FEAT_SYN_HL
				!has_syntax ||
# endif
				can_spell))
		    {
			char_u	*prev_ptr, *p;
			int	len;
			hlf_T	spell_hlf = HLF_COUNT;

			if (has_mbyte)
			{
			    prev_ptr = ptr - mb_l;
			    v -= mb_l - 1;
			}
			else
			    prev_ptr = ptr - 1;

			// Use nextline[] if possible, it has the start of the
			// next line concatenated.
			if ((prev_ptr - line) - nextlinecol >= 0)
			    p = nextline + (prev_ptr - line) - nextlinecol;
			else
			    p = prev_ptr;
			cap_col -= (int)(prev_ptr - line);
			len = spell_check(wp, p, &spell_hlf, &cap_col,
								    nochange);
			word_end = v + len;

			// In Insert mode only highlight a word that
			// doesn't touch the cursor.
			if (spell_hlf != HLF_COUNT
				&& (State & INSERT) != 0
				&& wp->w_cursor.lnum == lnum
				&& wp->w_cursor.col >=
						    (colnr_T)(prev_ptr - line)
				&& wp->w_cursor.col < (colnr_T)word_end)
			{
			    spell_hlf = HLF_COUNT;
			    spell_redraw_lnum = lnum;
			}

			if (spell_hlf == HLF_COUNT && p != prev_ptr
				       && (p - nextline) + len > nextline_idx)
			{
			    // Remember that the good word continues at the
			    // start of the next line.
			    checked_lnum = lnum + 1;
			    checked_col = (int)((p - nextline)
							 + len - nextline_idx);
			}

			// Turn index into actual attributes.
			if (spell_hlf != HLF_COUNT)
			    spell_attr = highlight_attr[spell_hlf];

			if (cap_col > 0)
			{
			    if (p != prev_ptr
				   && (p - nextline) + cap_col >= nextline_idx)
			    {
				// Remember that the word in the next line
				// must start with a capital.
				capcol_lnum = lnum + 1;
				cap_col = (int)((p - nextline) + cap_col
							       - nextline_idx);
			    }
			    else
				// Compute the actual column.
				cap_col += (int)(prev_ptr - line);
			}
		    }
		}
		if (spell_attr != 0)
		{
		    if (!attr_pri)
			char_attr = hl_combine_attr(char_attr, spell_attr);
		    else
			char_attr = hl_combine_attr(spell_attr, char_attr);
		}
#endif
#ifdef FEAT_LINEBREAK
		// Found last space before word: check for line break.
		if (wp->w_p_lbr && c0 == c
				  && VIM_ISBREAK(c) && !VIM_ISBREAK((int)*ptr))
		{
		    int	    mb_off = has_mbyte ? (*mb_head_off)(line, ptr - 1)
									   : 0;
		    char_u  *p = ptr - (mb_off + 1);

		    // TODO: is passing p for start of the line OK?
		    n_extra = win_lbr_chartabsize(wp, line, p, (colnr_T)vcol,
								    NULL) - 1;

		    // We have just drawn the showbreak value, no need to add
		    // space for it again.
		    if (vcol == vcol_sbr)
		    {
			n_extra -= MB_CHARLEN(get_showbreak_value(wp));
			if (n_extra < 0)
			    n_extra = 0;
		    }
		    if (on_last_col && c != TAB)
			// Do not continue search/match highlighting over the
			// line break, but for TABs the highlighting should
			// include the complete width of the character
			search_attr = 0;

		    if (c == TAB && n_extra + col > wp->w_width)
# ifdef FEAT_VARTABS
			n_extra = tabstop_padding(vcol, wp->w_buffer->b_p_ts,
					      wp->w_buffer->b_p_vts_array) - 1;
# else
			n_extra = (int)wp->w_buffer->b_p_ts
				       - vcol % (int)wp->w_buffer->b_p_ts - 1;
# endif

		    c_extra = mb_off > 0 ? MB_FILLER_CHAR : ' ';
		    c_final = NUL;
# if defined(FEAT_PROP_POPUP)
		    if (n_extra > 0 && c != TAB)
			in_linebreak = TRUE;
# endif
		    if (VIM_ISWHITE(c))
		    {
# ifdef FEAT_CONCEAL
			if (c == TAB)
			    // See "Tab alignment" below.
			    FIX_FOR_BOGUSCOLS;
# endif
			if (!wp->w_p_list)
			    c = ' ';
		    }
		}
#endif

		in_multispace = c == ' '
		    && ((ptr > line + 1 && ptr[-2] == ' ') || *ptr == ' ');
		if (!in_multispace)
		    multispace_pos = 0;

		// 'list': Change char 160 to 'nbsp' and space to 'space'
		// setting in 'listchars'.  But not when the character is
		// followed by a composing character (use mb_l to check that).
		if (wp->w_p_list
			&& ((((c == 160 && mb_l == 1)
			      || (mb_utf8
				  && ((mb_c == 160 && mb_l == 2)
				      || (mb_c == 0x202f && mb_l == 3))))
			     && wp->w_lcs_chars.nbsp)
			    || (c == ' '
				&& mb_l == 1
				&& (wp->w_lcs_chars.space
				    || (in_multispace
					&& wp->w_lcs_chars.multispace != NULL))
				&& ptr - line >= leadcol
				&& ptr - line <= trailcol)))
		{
		    if (in_multispace && wp->w_lcs_chars.multispace != NULL)
		    {
			c = wp->w_lcs_chars.multispace[multispace_pos++];
			if (wp->w_lcs_chars.multispace[multispace_pos] == NUL)
			    multispace_pos = 0;
		    }
		    else
			c = (c == ' ') ? wp->w_lcs_chars.space
					: wp->w_lcs_chars.nbsp;
		    if (area_attr == 0 && search_attr == 0)
		    {
			n_attr = 1;
			extra_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_8));
			saved_attr2 = char_attr; // save current attr
		    }
		    mb_c = c;
		    if (enc_utf8 && utf_char2len(c) > 1)
		    {
			mb_utf8 = TRUE;
			u8cc[0] = 0;
			c = 0xc0;
		    }
		    else
			mb_utf8 = FALSE;
		}

		if ((trailcol != MAXCOL && ptr > line + trailcol && c == ' ')
			|| (leadcol != 0 && ptr < line + leadcol && c == ' '))
		{
		    c = (ptr > line + trailcol) ? wp->w_lcs_chars.trail
							: wp->w_lcs_chars.lead;
		    if (!attr_pri)
		    {
			n_attr = 1;
			extra_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_8));
			saved_attr2 = char_attr; // save current attr
		    }
		    mb_c = c;
		    if (enc_utf8 && utf_char2len(c) > 1)
		    {
			mb_utf8 = TRUE;
			u8cc[0] = 0;
			c = 0xc0;
		    }
		    else
			mb_utf8 = FALSE;
		}
	    }

	    // Handling of non-printable characters.
	    if (!vim_isprintc(c))
	    {
		// when getting a character from the file, we may have to
		// turn it into something else on the way to putting it
		// into "ScreenLines".
		if (c == TAB && (!wp->w_p_list || wp->w_lcs_chars.tab1))
		{
		    int tab_len = 0;
		    long vcol_adjusted = vcol; // removed showbreak length
#ifdef FEAT_LINEBREAK
		    char_u *sbr = get_showbreak_value(wp);

		    // only adjust the tab_len, when at the first column
		    // after the showbreak value was drawn
		    if (*sbr != NUL && vcol == vcol_sbr && wp->w_p_wrap)
			vcol_adjusted = vcol - MB_CHARLEN(sbr);
#endif
		    // tab amount depends on current column
#ifdef FEAT_VARTABS
		    tab_len = tabstop_padding(vcol_adjusted,
					      wp->w_buffer->b_p_ts,
					      wp->w_buffer->b_p_vts_array) - 1;
#else
		    tab_len = (int)wp->w_buffer->b_p_ts
			       - vcol_adjusted % (int)wp->w_buffer->b_p_ts - 1;
#endif

#ifdef FEAT_LINEBREAK
		    if (!wp->w_p_lbr || !wp->w_p_list)
#endif
			// tab amount depends on current column
			n_extra = tab_len;
#ifdef FEAT_LINEBREAK
		    else
		    {
			char_u	*p;
			int	len;
			int	i;
			int	saved_nextra = n_extra;

# ifdef FEAT_CONCEAL
			if (vcol_off > 0)
			    // there are characters to conceal
			    tab_len += vcol_off;

			// boguscols before FIX_FOR_BOGUSCOLS macro from above
			if (wp->w_p_list && wp->w_lcs_chars.tab1
							&& old_boguscols > 0
							&& n_extra > tab_len)
			    tab_len += n_extra - tab_len;
# endif
			// If n_extra > 0, it gives the number of chars, to
			// use for a tab, else we need to calculate the width
			// for a tab.
			len = (tab_len * mb_char2len(wp->w_lcs_chars.tab2));
			if (wp->w_lcs_chars.tab3)
			    len += mb_char2len(wp->w_lcs_chars.tab3);
			if (n_extra > 0)
			    len += n_extra - tab_len;
			c = wp->w_lcs_chars.tab1;
			p = alloc(len + 1);
			if (p == NULL)
			    n_extra = 0;
			else
			{
			    vim_memset(p, ' ', len);
			    p[len] = NUL;
			    vim_free(p_extra_free);
			    p_extra_free = p;
			    for (i = 0; i < tab_len; i++)
			    {
				int lcs = wp->w_lcs_chars.tab2;

				if (*p == NUL)
				{
				    tab_len = i;
				    break;
				}

				// if tab3 is given, use it for the last char
				if (wp->w_lcs_chars.tab3 && i == tab_len - 1)
				    lcs = wp->w_lcs_chars.tab3;
				p += mb_char2bytes(lcs, p);
				n_extra += mb_char2len(lcs)
						  - (saved_nextra > 0 ? 1 : 0);
			    }
			    p_extra = p_extra_free;
# ifdef FEAT_CONCEAL
			    // n_extra will be increased by FIX_FOX_BOGUSCOLS
			    // macro below, so need to adjust for that here
			    if (vcol_off > 0)
				n_extra -= vcol_off;
# endif
			}
		    }
#endif
#ifdef FEAT_CONCEAL
		    {
			int vc_saved = vcol_off;

			// Tab alignment should be identical regardless of
			// 'conceallevel' value. So tab compensates of all
			// previous concealed characters, and thus resets
			// vcol_off and boguscols accumulated so far in the
			// line. Note that the tab can be longer than
			// 'tabstop' when there are concealed characters.
			FIX_FOR_BOGUSCOLS;

			// Make sure, the highlighting for the tab char will be
			// correctly set further below (effectively reverts the
			// FIX_FOR_BOGSUCOLS macro).
			if (n_extra == tab_len + vc_saved && wp->w_p_list
						&& wp->w_lcs_chars.tab1)
			    tab_len += vc_saved;
		    }
#endif
		    mb_utf8 = FALSE;	// don't draw as UTF-8
		    if (wp->w_p_list)
		    {
			c = (n_extra == 0 && wp->w_lcs_chars.tab3)
							? wp->w_lcs_chars.tab3
							: wp->w_lcs_chars.tab1;
#ifdef FEAT_LINEBREAK
			if (wp->w_p_lbr)
			    c_extra = NUL; // using p_extra from above
			else
#endif
			    c_extra = wp->w_lcs_chars.tab2;
			c_final = wp->w_lcs_chars.tab3;
			n_attr = tab_len + 1;
			extra_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_8));
			saved_attr2 = char_attr; // save current attr
			mb_c = c;
			if (enc_utf8 && utf_char2len(c) > 1)
			{
			    mb_utf8 = TRUE;
			    u8cc[0] = 0;
			    c = 0xc0;
			}
		    }
		    else
		    {
			c_final = NUL;
			c_extra = ' ';
			c = ' ';
		    }
		}
		else if (c == NUL
			&& (wp->w_p_list
			    || ((fromcol >= 0 || fromcol_prev >= 0)
				&& tocol > vcol
				&& VIsual_mode != Ctrl_V
				&& (
# ifdef FEAT_RIGHTLEFT
				    wp->w_p_rl ? (col >= 0) :
# endif
				    (col < wp->w_width))
				&& !(noinvcur
				    && lnum == wp->w_cursor.lnum
				    && (colnr_T)vcol == wp->w_virtcol)))
			&& lcs_eol_one > 0)
		{
		    // Display a '$' after the line or highlight an extra
		    // character if the line break is included.
#if defined(FEAT_DIFF) || defined(LINE_ATTR)
		    // For a diff line the highlighting continues after the
		    // "$".
		    if (
# ifdef FEAT_DIFF
			    diff_hlf == (hlf_T)0
#  ifdef LINE_ATTR
			    &&
#  endif
# endif
# ifdef LINE_ATTR
			    line_attr == 0
# endif
		       )
#endif
		    {
			// In virtualedit, visual selections may extend
			// beyond end of line.
			if (area_highlighting && virtual_active()
				&& tocol != MAXCOL && vcol < tocol)
			    n_extra = 0;
			else
			{
			    p_extra = at_end_str;
			    n_extra = 1;
			    c_extra = NUL;
			    c_final = NUL;
			}
		    }
		    if (wp->w_p_list && wp->w_lcs_chars.eol > 0)
			c = wp->w_lcs_chars.eol;
		    else
			c = ' ';
		    lcs_eol_one = -1;
		    --ptr;	    // put it back at the NUL
		    if (!attr_pri)
		    {
			extra_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_AT));
			n_attr = 1;
		    }
		    mb_c = c;
		    if (enc_utf8 && utf_char2len(c) > 1)
		    {
			mb_utf8 = TRUE;
			u8cc[0] = 0;
			c = 0xc0;
		    }
		    else
			mb_utf8 = FALSE;	// don't draw as UTF-8
		}
		else if (c != NUL)
		{
		    p_extra = transchar_buf(wp->w_buffer, c);
		    if (n_extra == 0)
			n_extra = byte2cells(c) - 1;
#ifdef FEAT_RIGHTLEFT
		    if ((dy_flags & DY_UHEX) && wp->w_p_rl)
			rl_mirror(p_extra);	// reverse "<12>"
#endif
		    c_extra = NUL;
		    c_final = NUL;
#ifdef FEAT_LINEBREAK
		    if (wp->w_p_lbr)
		    {
			char_u *p;

			c = *p_extra;
			p = alloc(n_extra + 1);
			vim_memset(p, ' ', n_extra);
			STRNCPY(p, p_extra + 1, STRLEN(p_extra) - 1);
			p[n_extra] = NUL;
			vim_free(p_extra_free);
			p_extra_free = p_extra = p;
		    }
		    else
#endif
		    {
			n_extra = byte2cells(c) - 1;
			c = *p_extra++;
		    }
		    if (!attr_pri)
		    {
			n_attr = n_extra + 1;
			extra_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_8));
			saved_attr2 = char_attr; // save current attr
		    }
		    mb_utf8 = FALSE;	// don't draw as UTF-8
		}
		else if (VIsual_active
			 && (VIsual_mode == Ctrl_V
			     || VIsual_mode == 'v')
			 && virtual_active()
			 && tocol != MAXCOL
			 && vcol < tocol
			 && (
#ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? (col >= 0) :
#endif
			    (col < wp->w_width)))
		{
		    c = ' ';
		    --ptr;	    // put it back at the NUL
		}
#if defined(LINE_ATTR)
		else if ((
# ifdef FEAT_DIFF
			    diff_hlf != (hlf_T)0 ||
# endif
# ifdef FEAT_TERMINAL
			    win_attr != 0 ||
# endif
			    line_attr != 0
			) && (
# ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? (col >= 0) :
# endif
			    (col
# ifdef FEAT_CONCEAL
				- boguscols
# endif
					    < wp->w_width)))
		{
		    // Highlight until the right side of the window
		    c = ' ';
		    --ptr;	    // put it back at the NUL

		    // Remember we do the char for line highlighting.
		    ++did_line_attr;

		    // don't do search HL for the rest of the line
		    if (line_attr != 0 && char_attr == search_attr
					&& (did_line_attr > 1
					    || (wp->w_p_list &&
						wp->w_lcs_chars.eol > 0)))
			char_attr = line_attr;
# ifdef FEAT_DIFF
		    if (diff_hlf == HLF_TXD)
		    {
			diff_hlf = HLF_CHD;
			if (vi_attr == 0 || char_attr != vi_attr)
			{
			    char_attr = HL_ATTR(diff_hlf);
			    if (wp->w_p_cul && lnum == wp->w_cursor.lnum
				    && wp->w_p_culopt_flags != CULOPT_NBR
				    && (!cul_screenline
					|| (vcol >= left_curline_col
						 && vcol <= right_curline_col)))
				char_attr = hl_combine_attr(
					  char_attr, HL_ATTR(HLF_CUL));
			}
		    }
# endif
# ifdef FEAT_TERMINAL
		    if (win_attr != 0)
		    {
			char_attr = win_attr;
			if (wp->w_p_cul && lnum == wp->w_cursor.lnum
				    && wp->w_p_culopt_flags != CULOPT_NBR)
			{
			    if (!cul_screenline || (vcol >= left_curline_col
						  && vcol <= right_curline_col))
				char_attr = hl_combine_attr(
					      char_attr, HL_ATTR(HLF_CUL));
			}
			else if (line_attr)
			    char_attr = hl_combine_attr(char_attr, line_attr);
		    }
# endif
		}
#endif
	    }

#ifdef FEAT_CONCEAL
	    if (   wp->w_p_cole > 0
		&& (wp != curwin || lnum != wp->w_cursor.lnum ||
						       conceal_cursor_line(wp))
		&& ((syntax_flags & HL_CONCEAL) != 0 || has_match_conc > 0)
		&& !(lnum_in_visual_area
				    && vim_strchr(wp->w_p_cocu, 'v') == NULL))
	    {
		char_attr = conceal_attr;
		if ((prev_syntax_id != syntax_seqnr || has_match_conc > 1)
			&& (syn_get_sub_char() != NUL
				|| (has_match_conc && match_conc)
				|| wp->w_p_cole == 1)
			&& wp->w_p_cole != 3)
		{
		    // First time at this concealed item: display one
		    // character.
		    if (has_match_conc && match_conc)
			c = match_conc;
		    else if (syn_get_sub_char() != NUL)
			c = syn_get_sub_char();
		    else if (wp->w_lcs_chars.conceal != NUL)
			c = wp->w_lcs_chars.conceal;
		    else
			c = ' ';

		    prev_syntax_id = syntax_seqnr;

		    if (n_extra > 0)
			vcol_off += n_extra;
		    vcol += n_extra;
		    if (wp->w_p_wrap && n_extra > 0)
		    {
# ifdef FEAT_RIGHTLEFT
			if (wp->w_p_rl)
			{
			    col -= n_extra;
			    boguscols -= n_extra;
			}
			else
# endif
			{
			    boguscols += n_extra;
			    col += n_extra;
			}
		    }
		    n_extra = 0;
		    n_attr = 0;
		}
		else if (n_skip == 0)
		{
		    is_concealing = TRUE;
		    n_skip = 1;
		}
		mb_c = c;
		if (enc_utf8 && utf_char2len(c) > 1)
		{
		    mb_utf8 = TRUE;
		    u8cc[0] = 0;
		    c = 0xc0;
		}
		else
		    mb_utf8 = FALSE;	// don't draw as UTF-8
	    }
	    else
	    {
		prev_syntax_id = 0;
		is_concealing = FALSE;
	    }

	    if (n_skip > 0 && did_decrement_ptr)
		// not showing the '>', put pointer back to avoid getting stuck
		++ptr;

#endif // FEAT_CONCEAL
	}

#ifdef FEAT_CONCEAL
	// In the cursor line and we may be concealing characters: correct
	// the cursor column when we reach its position.
	if (!did_wcol && draw_state == WL_LINE
		&& wp == curwin && lnum == wp->w_cursor.lnum
		&& conceal_cursor_line(wp)
		&& (int)wp->w_virtcol <= vcol + n_skip)
	{
# ifdef FEAT_RIGHTLEFT
	    if (wp->w_p_rl)
		wp->w_wcol = wp->w_width - col + boguscols - 1;
	    else
# endif
		wp->w_wcol = col - boguscols;
	    wp->w_wrow = row;
	    did_wcol = TRUE;
	    curwin->w_valid |= VALID_WCOL|VALID_WROW|VALID_VIRTCOL;
# ifdef FEAT_PROP_POPUP
	    curwin->w_flags &= ~(WFLAG_WCOL_OFF_ADDED | WFLAG_WROW_OFF_ADDED);
# endif
	}
#endif

	// Don't override visual selection highlighting.
	if (n_attr > 0
		&& draw_state == WL_LINE
		&& !attr_pri)
	{
#ifdef LINE_ATTR
	    if (line_attr)
		char_attr = hl_combine_attr(extra_attr, line_attr);
	    else
#endif
		char_attr = extra_attr;
	}

#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
	// XIM don't send preedit_start and preedit_end, but they send
	// preedit_changed and commit.  Thus Vim can't set "im_is_active", use
	// im_is_preediting() here.
	if (p_imst == IM_ON_THE_SPOT
		&& xic != NULL
		&& lnum == wp->w_cursor.lnum
		&& (State & INSERT)
		&& !p_imdisable
		&& im_is_preediting()
		&& draw_state == WL_LINE)
	{
	    colnr_T tcol;

	    if (preedit_end_col == MAXCOL)
		getvcol(curwin, &(wp->w_cursor), &tcol, NULL, NULL);
	    else
		tcol = preedit_end_col;
	    if ((long)preedit_start_col <= vcol && vcol < (long)tcol)
	    {
		if (feedback_old_attr < 0)
		{
		    feedback_col = 0;
		    feedback_old_attr = char_attr;
		}
		char_attr = im_get_feedback_attr(feedback_col);
		if (char_attr < 0)
		    char_attr = feedback_old_attr;
		feedback_col++;
	    }
	    else if (feedback_old_attr >= 0)
	    {
		char_attr = feedback_old_attr;
		feedback_old_attr = -1;
		feedback_col = 0;
	    }
	}
#endif
	// Handle the case where we are in column 0 but not on the first
	// character of the line and the user wants us to show us a
	// special character (via 'listchars' option "precedes:<char>".
	if (lcs_prec_todo != NUL
		&& wp->w_p_list
		&& (wp->w_p_wrap ?
		    (wp->w_skipcol > 0  && row == 0) :
		    wp->w_leftcol > 0)
#ifdef FEAT_DIFF
		&& filler_todo <= 0
#endif
		&& draw_state > WL_NR
		&& c != NUL)
	{
	    c = wp->w_lcs_chars.prec;
	    lcs_prec_todo = NUL;
	    if (has_mbyte && (*mb_char2cells)(mb_c) > 1)
	    {
		// Double-width character being overwritten by the "precedes"
		// character, need to fill up half the character.
		c_extra = MB_FILLER_CHAR;
		c_final = NUL;
		n_extra = 1;
		n_attr = 2;
		extra_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_AT));
	    }
	    mb_c = c;
	    if (enc_utf8 && utf_char2len(c) > 1)
	    {
		mb_utf8 = TRUE;
		u8cc[0] = 0;
		c = 0xc0;
	    }
	    else
		mb_utf8 = FALSE;	// don't draw as UTF-8
	    if (!attr_pri)
	    {
		saved_attr3 = char_attr; // save current attr
		char_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_AT));
		n_attr3 = 1;
	    }
	}

	// At end of the text line or just after the last character.
	if (c == NUL
#if defined(LINE_ATTR)
		|| did_line_attr == 1
#endif
		)
	{
#ifdef FEAT_SEARCH_EXTRA
	    // flag to indicate whether prevcol equals startcol of search_hl or
	    // one of the matches
	    int prevcol_hl_flag = get_prevcol_hl_flag(wp, &screen_search_hl,
					      (long)(ptr - line) - (c == NUL));
#endif
	    // Invert at least one char, used for Visual and empty line or
	    // highlight match at end of line. If it's beyond the last
	    // char on the screen, just overwrite that one (tricky!)  Not
	    // needed when a '$' was displayed for 'list'.
	    if (wp->w_lcs_chars.eol == lcs_eol_one
		    && ((area_attr != 0 && vcol == fromcol
			    && (VIsual_mode != Ctrl_V
				|| lnum == VIsual.lnum
				|| lnum == curwin->w_cursor.lnum)
			    && c == NUL)
#ifdef FEAT_SEARCH_EXTRA
			// highlight 'hlsearch' match at end of line
			|| (prevcol_hl_flag
# ifdef FEAT_SYN_HL
			    && !(wp->w_p_cul && lnum == wp->w_cursor.lnum
				    && !(wp == curwin && VIsual_active))
# endif
# ifdef FEAT_DIFF
			    && diff_hlf == (hlf_T)0
# endif
# if defined(LINE_ATTR)
			    && did_line_attr <= 1
# endif
			   )
#endif
		       ))
	    {
		int n = 0;

#ifdef FEAT_RIGHTLEFT
		if (wp->w_p_rl)
		{
		    if (col < 0)
			n = 1;
		}
		else
#endif
		{
		    if (col >= wp->w_width)
			n = -1;
		}
		if (n != 0)
		{
		    // At the window boundary, highlight the last character
		    // instead (better than nothing).
		    off += n;
		    col += n;
		}
		else
		{
		    // Add a blank character to highlight.
		    ScreenLines[off] = ' ';
		    if (enc_utf8)
			ScreenLinesUC[off] = 0;
		}
#ifdef FEAT_SEARCH_EXTRA
		if (area_attr == 0)
		{
		    // Use attributes from match with highest priority among
		    // 'search_hl' and the match list.
		    get_search_match_hl(wp, &screen_search_hl,
					       (long)(ptr - line), &char_attr);
		}
#endif
		ScreenAttrs[off] = char_attr;
#ifdef FEAT_RIGHTLEFT
		if (wp->w_p_rl)
		{
		    --col;
		    --off;
		}
		else
#endif
		{
		    ++col;
		    ++off;
		}
		++vcol;
		eol_hl_off = 1;
	    }
	}

	// At end of the text line.
	if (c == NUL)
	{
#ifdef FEAT_SYN_HL
	    // Highlight 'cursorcolumn' & 'colorcolumn' past end of the line.
	    if (wp->w_p_wrap)
		v = wp->w_skipcol;
	    else
		v = wp->w_leftcol;

	    // check if line ends before left margin
	    if (vcol < v + col - win_col_off(wp))
		vcol = v + col - win_col_off(wp);
#ifdef FEAT_CONCEAL
	    // Get rid of the boguscols now, we want to draw until the right
	    // edge for 'cursorcolumn'.
	    col -= boguscols;
	    boguscols = 0;
#endif

	    if (draw_color_col)
		draw_color_col = advance_color_col(VCOL_HLC, &color_cols);

	    if (((wp->w_p_cuc
		      && (int)wp->w_virtcol >= VCOL_HLC - eol_hl_off
		      && (int)wp->w_virtcol <
				   (long)wp->w_width * (row - startrow + 1) + v
		      && lnum != wp->w_cursor.lnum)
		    || draw_color_col
		    || win_attr != 0)
# ifdef FEAT_RIGHTLEFT
		    && !wp->w_p_rl
# endif
		    )
	    {
		int	rightmost_vcol = 0;
		int	i;

		if (wp->w_p_cuc)
		    rightmost_vcol = wp->w_virtcol;
		if (draw_color_col)
		    // determine rightmost colorcolumn to possibly draw
		    for (i = 0; color_cols[i] >= 0; ++i)
			if (rightmost_vcol < color_cols[i])
			    rightmost_vcol = color_cols[i];

		while (col < wp->w_width)
		{
		    ScreenLines[off] = ' ';
		    if (enc_utf8)
			ScreenLinesUC[off] = 0;
		    ++col;
		    if (draw_color_col)
			draw_color_col = advance_color_col(VCOL_HLC,
								 &color_cols);

		    if (wp->w_p_cuc && VCOL_HLC == (long)wp->w_virtcol)
			ScreenAttrs[off++] = HL_ATTR(HLF_CUC);
		    else if (draw_color_col && VCOL_HLC == *color_cols)
			ScreenAttrs[off++] = HL_ATTR(HLF_MC);
		    else
			ScreenAttrs[off++] = win_attr;

		    if (VCOL_HLC >= rightmost_vcol && win_attr == 0)
			break;

		    ++vcol;
		}
	    }
#endif

	    screen_line(screen_row, wp->w_wincol, col,
					  wp->w_width, screen_line_flags);
	    row++;

	    // Update w_cline_height and w_cline_folded if the cursor line was
	    // updated (saves a call to plines() later).
	    if (wp == curwin && lnum == curwin->w_cursor.lnum)
	    {
		curwin->w_cline_row = startrow;
		curwin->w_cline_height = row - startrow;
#ifdef FEAT_FOLDING
		curwin->w_cline_folded = FALSE;
#endif
		curwin->w_valid |= (VALID_CHEIGHT|VALID_CROW);
	    }

	    break;
	}

	// Show "extends" character from 'listchars' if beyond the line end and
	// 'list' is set.
	if (wp->w_lcs_chars.ext != NUL
		&& draw_state == WL_LINE
		&& wp->w_p_list
		&& !wp->w_p_wrap
#ifdef FEAT_DIFF
		&& filler_todo <= 0
#endif
		&& (
#ifdef FEAT_RIGHTLEFT
		    wp->w_p_rl ? col == 0 :
#endif
		    col == wp->w_width - 1)
		&& (*ptr != NUL
		    || (wp->w_p_list && lcs_eol_one > 0)
		    || (n_extra && (c_extra != NUL || *p_extra != NUL))))
	{
	    c = wp->w_lcs_chars.ext;
	    char_attr = hl_combine_attr(win_attr, HL_ATTR(HLF_AT));
	    mb_c = c;
	    if (enc_utf8 && utf_char2len(c) > 1)
	    {
		mb_utf8 = TRUE;
		u8cc[0] = 0;
		c = 0xc0;
	    }
	    else
		mb_utf8 = FALSE;
	}

#ifdef FEAT_SYN_HL
	// advance to the next 'colorcolumn'
	if (draw_color_col)
	    draw_color_col = advance_color_col(VCOL_HLC, &color_cols);

	// Highlight the cursor column if 'cursorcolumn' is set.  But don't
	// highlight the cursor position itself.
	// Also highlight the 'colorcolumn' if it is different than
	// 'cursorcolumn'
	// Also highlight the 'colorcolumn' if 'breakindent' and/or 'showbreak'
	// options are set
	vcol_save_attr = -1;
	if (((draw_state == WL_LINE ||
	     draw_state == WL_BRI ||
	     draw_state == WL_SBR) && !lnum_in_visual_area
		&& search_attr == 0 && area_attr == 0)
# ifdef FEAT_DIFF
			&& filler_todo <= 0
# endif
		)
	{
	    if (wp->w_p_cuc && VCOL_HLC == (long)wp->w_virtcol
						 && lnum != wp->w_cursor.lnum)
	    {
		vcol_save_attr = char_attr;
		char_attr = hl_combine_attr(char_attr, HL_ATTR(HLF_CUC));
	    }
	    else if (draw_color_col && VCOL_HLC == *color_cols)
	    {
		vcol_save_attr = char_attr;
		char_attr = hl_combine_attr(char_attr, HL_ATTR(HLF_MC));
	    }
	}
#endif

	// Store character to be displayed.
	// Skip characters that are left of the screen for 'nowrap'.
	vcol_prev = vcol;
	if (draw_state < WL_LINE || n_skip <= 0)
	{
	    // Store the character.
#if defined(FEAT_RIGHTLEFT)
	    if (has_mbyte && wp->w_p_rl && (*mb_char2cells)(mb_c) > 1)
	    {
		// A double-wide character is: put first half in left cell.
		--off;
		--col;
	    }
#endif
	    ScreenLines[off] = c;
	    if (enc_dbcs == DBCS_JPNU)
	    {
		if ((mb_c & 0xff00) == 0x8e00)
		    ScreenLines[off] = 0x8e;
		ScreenLines2[off] = mb_c & 0xff;
	    }
	    else if (enc_utf8)
	    {
		if (mb_utf8)
		{
		    int i;

		    ScreenLinesUC[off] = mb_c;
		    if ((c & 0xff) == 0)
			ScreenLines[off] = 0x80;   // avoid storing zero
		    for (i = 0; i < Screen_mco; ++i)
		    {
			ScreenLinesC[i][off] = u8cc[i];
			if (u8cc[i] == 0)
			    break;
		    }
		}
		else
		    ScreenLinesUC[off] = 0;
	    }
	    if (multi_attr)
	    {
		ScreenAttrs[off] = multi_attr;
		multi_attr = 0;
	    }
	    else
		ScreenAttrs[off] = char_attr;

	    if (has_mbyte && (*mb_char2cells)(mb_c) > 1)
	    {
		// Need to fill two screen columns.
		++off;
		++col;
		if (enc_utf8)
		    // UTF-8: Put a 0 in the second screen char.
		    ScreenLines[off] = 0;
		else
		    // DBCS: Put second byte in the second screen char.
		    ScreenLines[off] = mb_c & 0xff;
		if (draw_state > WL_NR
#ifdef FEAT_DIFF
			&& filler_todo <= 0
#endif
			)
		    ++vcol;
		// When "tocol" is halfway a character, set it to the end of
		// the character, otherwise highlighting won't stop.
		if (tocol == vcol)
		    ++tocol;
#ifdef FEAT_RIGHTLEFT
		if (wp->w_p_rl)
		{
		    // now it's time to backup one cell
		    --off;
		    --col;
		}
#endif
	    }
#ifdef FEAT_RIGHTLEFT
	    if (wp->w_p_rl)
	    {
		--off;
		--col;
	    }
	    else
#endif
	    {
		++off;
		++col;
	    }
	}
#ifdef FEAT_CONCEAL
	else if (wp->w_p_cole > 0 && is_concealing)
	{
	    --n_skip;
	    ++vcol_off;
	    if (n_extra > 0)
		vcol_off += n_extra;
	    if (wp->w_p_wrap)
	    {
		// Special voodoo required if 'wrap' is on.
		//
		// Advance the column indicator to force the line
		// drawing to wrap early. This will make the line
		// take up the same screen space when parts are concealed,
		// so that cursor line computations aren't messed up.
		//
		// To avoid the fictitious advance of 'col' causing
		// trailing junk to be written out of the screen line
		// we are building, 'boguscols' keeps track of the number
		// of bad columns we have advanced.
		if (n_extra > 0)
		{
		    vcol += n_extra;
# ifdef FEAT_RIGHTLEFT
		    if (wp->w_p_rl)
		    {
			col -= n_extra;
			boguscols -= n_extra;
		    }
		    else
# endif
		    {
			col += n_extra;
			boguscols += n_extra;
		    }
		    n_extra = 0;
		    n_attr = 0;
		}


		if (has_mbyte && (*mb_char2cells)(mb_c) > 1)
		{
		    // Need to fill two screen columns.
# ifdef FEAT_RIGHTLEFT
		    if (wp->w_p_rl)
		    {
			--boguscols;
			--col;
		    }
		    else
# endif
		    {
			++boguscols;
			++col;
		    }
		}

# ifdef FEAT_RIGHTLEFT
		if (wp->w_p_rl)
		{
		    --boguscols;
		    --col;
		}
		else
# endif
		{
		    ++boguscols;
		    ++col;
		}
	    }
	    else
	    {
		if (n_extra > 0)
		{
		    vcol += n_extra;
		    n_extra = 0;
		    n_attr = 0;
		}
	    }

	}
#endif // FEAT_CONCEAL
	else
	    --n_skip;

	// Only advance the "vcol" when after the 'number' or 'relativenumber'
	// column.
	if (draw_state > WL_NR
#ifdef FEAT_DIFF
		&& filler_todo <= 0
#endif
		)
	    ++vcol;

#ifdef FEAT_SYN_HL
	if (vcol_save_attr >= 0)
	    char_attr = vcol_save_attr;
#endif

	// restore attributes after "predeces" in 'listchars'
	if (draw_state > WL_NR && n_attr3 > 0 && --n_attr3 == 0)
	    char_attr = saved_attr3;

	// restore attributes after last 'listchars' or 'number' char
	if (n_attr > 0 && draw_state == WL_LINE && --n_attr == 0)
	    char_attr = saved_attr2;

	// At end of screen line and there is more to come: Display the line
	// so far.  If there is no more to display it is caught above.
	if ((
#ifdef FEAT_RIGHTLEFT
	    wp->w_p_rl ? (col < 0) :
#endif
				    (col >= wp->w_width))
		&& (draw_state != WL_LINE
		    || *ptr != NUL
#ifdef FEAT_DIFF
		    || filler_todo > 0
#endif
		    || (wp->w_p_list && wp->w_lcs_chars.eol != NUL
						&& p_extra != at_end_str)
		    || (n_extra != 0 && (c_extra != NUL || *p_extra != NUL)))
		)
	{
#ifdef FEAT_CONCEAL
	    screen_line(screen_row, wp->w_wincol, col - boguscols,
					  wp->w_width, screen_line_flags);
	    boguscols = 0;
#else
	    screen_line(screen_row, wp->w_wincol, col,
					  wp->w_width, screen_line_flags);
#endif
	    ++row;
	    ++screen_row;

	    // When not wrapping and finished diff lines, or when displayed
	    // '$' and highlighting until last column, break here.
	    if ((!wp->w_p_wrap
#ifdef FEAT_DIFF
		    && filler_todo <= 0
#endif
		    ) || lcs_eol_one == -1)
		break;

	    // When the window is too narrow draw all "@" lines.
	    if (draw_state != WL_LINE
#ifdef FEAT_DIFF
		    && filler_todo <= 0
#endif
		    )
	    {
		win_draw_end(wp, '@', ' ', TRUE, row, wp->w_height, HLF_AT);
		draw_vsep_win(wp, row);
		row = endrow;
	    }

	    // When line got too long for screen break here.
	    if (row == endrow)
	    {
		++row;
		break;
	    }

	    if (screen_cur_row == screen_row - 1
#ifdef FEAT_DIFF
		     && filler_todo <= 0
#endif
		     && wp->w_width == Columns)
	    {
		// Remember that the line wraps, used for modeless copy.
		LineWraps[screen_row - 1] = TRUE;

		// Special trick to make copy/paste of wrapped lines work with
		// xterm/screen: write an extra character beyond the end of
		// the line. This will work with all terminal types
		// (regardless of the xn,am settings).
		// Only do this on a fast tty.
		// Only do this if the cursor is on the current line
		// (something has been written in it).
		// Don't do this for the GUI.
		// Don't do this for double-width characters.
		// Don't do this for a window not at the right screen border.
		if (p_tf
#ifdef FEAT_GUI
			 && !gui.in_use
#endif
			 && !(has_mbyte
			     && ((*mb_off2cells)(LineOffset[screen_row],
				     LineOffset[screen_row] + screen_Columns)
									  == 2
				 || (*mb_off2cells)(LineOffset[screen_row - 1]
							+ (int)Columns - 2,
				     LineOffset[screen_row] + screen_Columns)
									== 2)))
		{
		    // First make sure we are at the end of the screen line,
		    // then output the same character again to let the
		    // terminal know about the wrap.  If the terminal doesn't
		    // auto-wrap, we overwrite the character.
		    if (screen_cur_col != wp->w_width)
			screen_char(LineOffset[screen_row - 1]
						      + (unsigned)Columns - 1,
					  screen_row - 1, (int)(Columns - 1));

		    // When there is a multi-byte character, just output a
		    // space to keep it simple.
		    if (has_mbyte && MB_BYTE2LEN(ScreenLines[LineOffset[
					screen_row - 1] + (Columns - 1)]) > 1)
			out_char(' ');
		    else
			out_char(ScreenLines[LineOffset[screen_row - 1]
							    + (Columns - 1)]);
		    // force a redraw of the first char on the next line
		    ScreenAttrs[LineOffset[screen_row]] = (sattr_T)-1;
		    screen_start();	// don't know where cursor is now
		}
	    }

	    col = 0;
	    off = (unsigned)(current_ScreenLine - ScreenLines);
#ifdef FEAT_RIGHTLEFT
	    if (wp->w_p_rl)
	    {
		col = wp->w_width - 1;	// col is not used if breaking!
		off += col;
	    }
#endif

	    // reset the drawing state for the start of a wrapped line
	    draw_state = WL_START;
	    saved_n_extra = n_extra;
	    saved_p_extra = p_extra;
	    saved_c_extra = c_extra;
	    saved_c_final = c_final;
#ifdef FEAT_SYN_HL
	    if (!(cul_screenline
# ifdef FEAT_DIFF
			&& diff_hlf == (hlf_T)0
# endif
		    ))
		saved_char_attr = char_attr;
	    else
#endif
		saved_char_attr = 0;
	    n_extra = 0;
	    lcs_prec_todo = wp->w_lcs_chars.prec;
#ifdef FEAT_LINEBREAK
# ifdef FEAT_DIFF
	    if (filler_todo <= 0)
# endif
		need_showbreak = TRUE;
#endif
#ifdef FEAT_DIFF
	    --filler_todo;
	    // When the filler lines are actually below the last line of the
	    // file, don't draw the line itself, break here.
	    if (filler_todo == 0 && wp->w_botfill)
		break;
#endif
	}

    }	// for every character in the line

#ifdef FEAT_SPELL
    // After an empty line check first word for capital.
    if (*skipwhite(line) == NUL)
    {
	capcol_lnum = lnum + 1;
	cap_col = 0;
    }
#endif
#ifdef FEAT_PROP_POPUP
    vim_free(text_props);
    vim_free(text_prop_idxs);
#endif

    vim_free(p_extra_free);
    return row;
}
