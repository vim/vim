/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * edit.c: functions for Insert mode
 */

#include "vim.h"

#ifdef FEAT_INS_EXPAND
/*
 * definitions used for CTRL-X submode
 */
#define CTRL_X_WANT_IDENT	0x100

#define CTRL_X_NOT_DEFINED_YET	1
#define CTRL_X_SCROLL		2
#define CTRL_X_WHOLE_LINE	3
#define CTRL_X_FILES		4
#define CTRL_X_TAGS		(5 + CTRL_X_WANT_IDENT)
#define CTRL_X_PATH_PATTERNS	(6 + CTRL_X_WANT_IDENT)
#define CTRL_X_PATH_DEFINES	(7 + CTRL_X_WANT_IDENT)
#define CTRL_X_FINISHED		8
#define CTRL_X_DICTIONARY	(9 + CTRL_X_WANT_IDENT)
#define CTRL_X_THESAURUS	(10 + CTRL_X_WANT_IDENT)
#define CTRL_X_CMDLINE		11

#define CHECK_KEYS_TIME		30

#define CTRL_X_MSG(i) ctrl_x_msgs[(i) & ~CTRL_X_WANT_IDENT]

static char *ctrl_x_msgs[] =
{
    N_(" Keyword completion (^N^P)"), /* ctrl_x_mode == 0, ^P/^N compl. */
    N_(" ^X mode (^E^Y^L^]^F^I^K^D^V^N^P)"),
    /* Scroll has it's own msgs, in it's place there is the msg for local
     * ctrl_x_mode = 0 (eg continue_status & CONT_LOCAL)  -- Acevedo */
    N_(" Keyword Local completion (^N^P)"),
    N_(" Whole line completion (^L^N^P)"),
    N_(" File name completion (^F^N^P)"),
    N_(" Tag completion (^]^N^P)"),
    N_(" Path pattern completion (^N^P)"),
    N_(" Definition completion (^D^N^P)"),
    NULL,
    N_(" Dictionary completion (^K^N^P)"),
    N_(" Thesaurus completion (^T^N^P)"),
    N_(" Command-line completion (^V^N^P)")
};

static char_u e_hitend[] = N_("Hit end of paragraph");

/*
 * Structure used to store one match for insert completion.
 */
struct Completion
{
    struct Completion	*next;
    struct Completion	*prev;
    char_u		*str;	  /* matched text */
    char_u		*fname;	  /* file containing the match */
    int			original; /* ORIGINAL_TEXT, CONT_S_IPOS or FREE_FNAME */
    int			number;	  /* sequence number */
};

/* the original text when the expansion begun */
#define ORIGINAL_TEXT	(1)
#define FREE_FNAME	(2)

/*
 * All the current matches are stored in a list.
 * "first_match" points to the start of the list.
 * "curr_match" points to the currently selected entry.
 * "shown_match" is different from curr_match during ins_compl_get_exp().
 */
static struct Completion    *first_match = NULL;
static struct Completion    *curr_match = NULL;
static struct Completion    *shown_match = NULL;

static int		    started_completion = FALSE;
static int		    completion_matches = 0;
static char_u		    *complete_pat = NULL;
static int		    complete_direction = FORWARD;
static int		    shown_direction = FORWARD;
static int		    completion_pending = FALSE;
static pos_T		    initial_pos;
static colnr_T		    complete_col = 0;	/* column where the text starts
						   that is being completed */
static int		    save_sm;
static char_u		    *original_text = NULL;  /* text before completion */
static int		    continue_mode = 0;
static expand_T		    complete_xp;

static int  ins_compl_add __ARGS((char_u *str, int len, char_u *, int dir, int reuse));
static void ins_compl_add_matches __ARGS((int num_matches, char_u **matches, int dir));
static int  ins_compl_make_cyclic __ARGS((void));
static void ins_compl_dictionaries __ARGS((char_u *dict, char_u *pat, int dir, int flags, int thesaurus));
static void ins_compl_free __ARGS((void));
static void ins_compl_clear __ARGS((void));
static void ins_compl_prep __ARGS((int c));
static buf_T *ins_compl_next_buf __ARGS((buf_T *buf, int flag));
static int  ins_compl_get_exp __ARGS((pos_T *ini, int dir));
static void ins_compl_delete __ARGS((void));
static void ins_compl_insert __ARGS((void));
static int  ins_compl_next __ARGS((int allow_get_expansion));
static int  ins_complete __ARGS((int c));
static int  quote_meta __ARGS((char_u *dest, char_u *str, int len));
#endif /* FEAT_INS_EXPAND */

#define BACKSPACE_CHAR		    1
#define BACKSPACE_WORD		    2
#define BACKSPACE_WORD_NOT_SPACE    3
#define BACKSPACE_LINE		    4

static void ins_redraw __ARGS((void));
static void ins_ctrl_v __ARGS((void));
static void undisplay_dollar __ARGS((void));
static void insert_special __ARGS((int, int, int));
static void check_auto_format __ARGS((int));
static void redo_literal __ARGS((int c));
static void start_arrow __ARGS((pos_T *end_insert_pos));
static void stop_insert __ARGS((pos_T *end_insert_pos, int esc));
static int  echeck_abbr __ARGS((int));
static void replace_push_off __ARGS((int c));
static int  replace_pop __ARGS((void));
static void replace_join __ARGS((int off));
static void replace_pop_ins __ARGS((void));
#ifdef FEAT_MBYTE
static void mb_replace_pop_ins __ARGS((int cc));
#endif
static void replace_flush __ARGS((void));
static void replace_do_bs __ARGS((void));
#ifdef FEAT_CINDENT
static int cindent_on __ARGS((void));
#endif
static void ins_reg __ARGS((void));
static void ins_ctrl_g __ARGS((void));
static int  ins_esc __ARGS((long *count, int cmdchar));
#ifdef FEAT_RIGHTLEFT
static void ins_ctrl_ __ARGS((void));
#endif
#ifdef FEAT_VISUAL
static int ins_start_select __ARGS((int c));
#endif
static void ins_shift __ARGS((int c, int lastc));
static void ins_del __ARGS((void));
static int  ins_bs __ARGS((int c, int mode, int *inserted_space_p));
#ifdef FEAT_MOUSE
static void ins_mouse __ARGS((int c));
static void ins_mousescroll __ARGS((int up));
#endif
static void ins_left __ARGS((void));
static void ins_home __ARGS((int c));
static void ins_end __ARGS((int c));
static void ins_s_left __ARGS((void));
static void ins_right __ARGS((void));
static void ins_s_right __ARGS((void));
static void ins_up __ARGS((int startcol));
static void ins_pageup __ARGS((void));
static void ins_down __ARGS((int startcol));
static void ins_pagedown __ARGS((void));
#ifdef FEAT_DND
static void ins_drop __ARGS((void));
#endif
static int  ins_tab __ARGS((void));
static int  ins_eol __ARGS((int c));
#ifdef FEAT_DIGRAPHS
static int  ins_digraph __ARGS((void));
#endif
static int  ins_copychar __ARGS((linenr_T lnum));
#ifdef FEAT_SMARTINDENT
static void ins_try_si __ARGS((int c));
#endif
static colnr_T get_nolist_virtcol __ARGS((void));

static colnr_T	Insstart_textlen;	/* length of line when insert started */
static colnr_T	Insstart_blank_vcol;	/* vcol for first inserted blank */

static char_u	*last_insert = NULL;	/* the text of the previous insert,
					   K_SPECIAL and CSI are escaped */
static int	last_insert_skip; /* nr of chars in front of previous insert */
static int	new_insert_skip;  /* nr of chars in front of current insert */

#ifdef FEAT_CINDENT
static int	can_cindent;		/* may do cindenting on this line */
#endif

static int	old_indent = 0;		/* for ^^D command in insert mode */

#ifdef FEAT_RIGHTLEFT
int		revins_on;		/* reverse insert mode on */
int		revins_chars;		/* how much to skip after edit */
int		revins_legal;		/* was the last char 'legal'? */
int		revins_scol;		/* start column of revins session */
#endif

#if defined(FEAT_MBYTE) && defined(MACOS_CLASSIC)
static short	previous_script = smRoman;
#endif

static int	ins_need_undo;		/* call u_save() before inserting a
					   char.  Set when edit() is called.
					   after that arrow_used is used. */

static int	did_add_space = FALSE;	/* auto_format() added an extra space
					   under the cursor */

/*
 * edit(): Start inserting text.
 *
 * "cmdchar" can be:
 * 'i'	normal insert command
 * 'a'	normal append command
 * 'R'	replace command
 * 'r'	"r<CR>" command: insert one <CR>.  Note: count can be > 1, for redo,
 *	but still only one <CR> is inserted.  The <Esc> is not used for redo.
 * 'g'	"gI" command.
 * 'V'	"gR" command for Virtual Replace mode.
 * 'v'	"gr" command for single character Virtual Replace mode.
 *
 * This function is not called recursively.  For CTRL-O commands, it returns
 * and lets the caller handle the Normal-mode command.
 *
 * Return TRUE if a CTRL-O command caused the return (insert mode pending).
 */
    int
edit(cmdchar, startln, count)
    int		cmdchar;
    int		startln;	/* if set, insert at start of line */
    long	count;
{
    int		c = 0;
    char_u	*ptr;
    int		lastc;
    colnr_T	mincol;
    static linenr_T o_lnum = 0;
    static int	o_eol = FALSE;
    int		i;
    int		did_backspace = TRUE;	    /* previous char was backspace */
#ifdef FEAT_CINDENT
    int		line_is_white = FALSE;	    /* line is empty before insert */
#endif
    linenr_T	old_topline = 0;	    /* topline before insertion */
#ifdef FEAT_DIFF
    int		old_topfill = -1;
#endif
    int		inserted_space = FALSE;     /* just inserted a space */
    int		replaceState = REPLACE;
    int		did_restart_edit = restart_edit;

    /* sleep before redrawing, needed for "CTRL-O :" that results in an
     * error message */
    check_for_delay(TRUE);

#ifdef HAVE_SANDBOX
    /* Don't allow inserting in the sandbox. */
    if (sandbox != 0)
    {
	EMSG(_(e_sandbox));
	return FALSE;
    }
#endif

#ifdef FEAT_INS_EXPAND
    ins_compl_clear();	    /* clear stuff for CTRL-X mode */
#endif

#ifdef FEAT_MOUSE
    /*
     * When doing a paste with the middle mouse button, Insstart is set to
     * where the paste started.
     */
    if (where_paste_started.lnum != 0)
	Insstart = where_paste_started;
    else
#endif
    {
	Insstart = curwin->w_cursor;
	if (startln)
	    Insstart.col = 0;
    }
    Insstart_textlen = linetabsize(ml_get_curline());
    Insstart_blank_vcol = MAXCOL;
    if (!did_ai)
	ai_col = 0;

    if (cmdchar != NUL && restart_edit == 0)
    {
	ResetRedobuff();
	AppendNumberToRedobuff(count);
#ifdef FEAT_VREPLACE
	if (cmdchar == 'V' || cmdchar == 'v')
	{
	    /* "gR" or "gr" command */
	    AppendCharToRedobuff('g');
	    AppendCharToRedobuff((cmdchar == 'v') ? 'r' : 'R');
	}
	else
#endif
	{
	    AppendCharToRedobuff(cmdchar);
	    if (cmdchar == 'g')		    /* "gI" command */
		AppendCharToRedobuff('I');
	    else if (cmdchar == 'r')	    /* "r<CR>" command */
		count = 1;		    /* insert only one <CR> */
	}
    }

    if (cmdchar == 'R')
    {
#ifdef FEAT_FKMAP
	if (p_fkmap && p_ri)
	{
	    beep_flush();
	    EMSG(farsi_text_3);	    /* encoded in Farsi */
	    State = INSERT;
	}
	else
#endif
	State = REPLACE;
    }
#ifdef FEAT_VREPLACE
    else if (cmdchar == 'V' || cmdchar == 'v')
    {
	State = VREPLACE;
	replaceState = VREPLACE;
	orig_line_count = curbuf->b_ml.ml_line_count;
	vr_lines_changed = 1;
    }
#endif
    else
	State = INSERT;

    stop_insert_mode = FALSE;

    /*
     * Need to recompute the cursor position, it might move when the cursor is
     * on a TAB or special character.
     */
    curs_columns(TRUE);

    /*
     * Enable langmap or IME, indicated by 'iminsert'.
     * Note that IME may enabled/disabled without us noticing here, thus the
     * 'iminsert' value may not reflect what is actually used.  It is updated
     * when hitting <Esc>.
     */
    if (curbuf->b_p_iminsert == B_IMODE_LMAP)
	State |= LANGMAP;
#ifdef USE_IM_CONTROL
    im_set_active(curbuf->b_p_iminsert == B_IMODE_IM);
#endif

#if defined(FEAT_MBYTE) && defined(MACOS_CLASSIC)
    KeyScript(previous_script);
#endif

#ifdef FEAT_MOUSE
    setmouse();
#endif
#ifdef FEAT_CMDL_INFO
    clear_showcmd();
#endif
#ifdef FEAT_RIGHTLEFT
    /* there is no reverse replace mode */
    revins_on = (State == INSERT && p_ri);
    if (revins_on)
	undisplay_dollar();
    revins_chars = 0;
    revins_legal = 0;
    revins_scol = -1;
#endif

    /*
     * Handle restarting Insert mode.
     * Don't do this for "CTRL-O ." (repeat an insert): we get here with
     * restart_edit non-zero, and something in the stuff buffer.
     */
    if (restart_edit != 0 && stuff_empty())
    {
#ifdef FEAT_MOUSE
	/*
	 * After a paste we consider text typed to be part of the insert for
	 * the pasted text. You can backspace over the pasted text too.
	 */
	if (where_paste_started.lnum)
	    arrow_used = FALSE;
	else
#endif
	    arrow_used = TRUE;
	restart_edit = 0;

	/*
	 * If the cursor was after the end-of-line before the CTRL-O and it is
	 * now at the end-of-line, put it after the end-of-line (this is not
	 * correct in very rare cases).
	 * Also do this if curswant is greater than the current virtual
	 * column.  Eg after "^O$" or "^O80|".
	 */
	validate_virtcol();
	update_curswant();
	if (((o_eol && curwin->w_cursor.lnum == o_lnum)
		    || curwin->w_curswant > curwin->w_virtcol)
		&& *(ptr = ml_get_curline() + curwin->w_cursor.col) != NUL)
	{
	    if (ptr[1] == NUL)
		++curwin->w_cursor.col;
#ifdef FEAT_MBYTE
	    else if (has_mbyte)
	    {
		i = (*mb_ptr2len_check)(ptr);
		if (ptr[i] == NUL)
		    curwin->w_cursor.col += i;
	    }
#endif
	}
	o_eol = FALSE;
    }
    else
	arrow_used = FALSE;

    /* we are in insert mode now, don't need to start it anymore */
    need_start_insertmode = FALSE;

    /* Need to save the line for undo before inserting the first char. */
    ins_need_undo = TRUE;

#ifdef FEAT_MOUSE
    where_paste_started.lnum = 0;
#endif
#ifdef FEAT_CINDENT
    can_cindent = TRUE;
#endif
#ifdef FEAT_FOLDING
    /* The cursor line is not in a closed fold, unless 'insertmode' is set or
     * restarting. */
    if (!p_im && did_restart_edit == 0)
	foldOpenCursor();
#endif

    /*
     * If 'showmode' is set, show the current (insert/replace/..) mode.
     * A warning message for changing a readonly file is given here, before
     * actually changing anything.  It's put after the mode, if any.
     */
    i = 0;
    if (p_smd)
	i = showmode();

    if (!p_im && did_restart_edit == 0)
	change_warning(i + 1);

#ifdef CURSOR_SHAPE
    ui_cursor_shape();		/* may show different cursor shape */
#endif
#ifdef FEAT_DIGRAPHS
    do_digraph(-1);		/* clear digraphs */
#endif

/*
 * Get the current length of the redo buffer, those characters have to be
 * skipped if we want to get to the inserted characters.
 */
    ptr = get_inserted();
    if (ptr == NULL)
	new_insert_skip = 0;
    else
    {
	new_insert_skip = (int)STRLEN(ptr);
	vim_free(ptr);
    }

    old_indent = 0;

    /*
     * Main loop in Insert mode: repeat until Insert mode is left.
     */
    for (;;)
    {
#ifdef FEAT_RIGHTLEFT
	if (!revins_legal)
	    revins_scol = -1;	    /* reset on illegal motions */
	else
	    revins_legal = 0;
#endif
	if (arrow_used)	    /* don't repeat insert when arrow key used */
	    count = 0;

	if (stop_insert_mode)
	{
	    /* ":stopinsert" used or 'insertmode' reset */
	    count = 0;
	    goto doESCkey;
	}

	/* set curwin->w_curswant for next K_DOWN or K_UP */
	if (!arrow_used)
	    curwin->w_set_curswant = TRUE;

	/* If there is no typeahead may check for timestamps (e.g., for when a
	 * menu invoked a shell command). */
	if (stuff_empty())
	{
	    did_check_timestamps = FALSE;
	    if (need_check_timestamps)
		check_timestamps(FALSE);
	}

	/*
	 * When emsg() was called msg_scroll will have been set.
	 */
	msg_scroll = FALSE;

#ifdef FEAT_GUI
	/* When 'mousefocus' is set a mouse movement may have taken us to
	 * another window.  "need_mouse_correct" may then be set because of an
	 * autocommand. */
	if (need_mouse_correct)
	    gui_mouse_correct();
#endif

#ifdef FEAT_FOLDING
	/* Open fold at the cursor line, according to 'foldopen'. */
	if (fdo_flags & FDO_INSERT)
	    foldOpenCursor();
	/* Close folds where the cursor isn't, according to 'foldclose' */
	if (!char_avail())
	    foldCheckClose();
#endif

	/*
	 * If we inserted a character at the last position of the last line in
	 * the window, scroll the window one line up. This avoids an extra
	 * redraw.
	 * This is detected when the cursor column is smaller after inserting
	 * something.
	 * Don't do this when the topline changed already, it has
	 * already been adjusted (by insertchar() calling open_line())).
	 */
	if (curbuf->b_mod_set
		&& curwin->w_p_wrap
		&& !did_backspace
		&& curwin->w_topline == old_topline
#ifdef FEAT_DIFF
		&& curwin->w_topfill == old_topfill
#endif
		)
	{
	    mincol = curwin->w_wcol;
	    validate_cursor_col();

	    if ((int)curwin->w_wcol < (int)mincol - curbuf->b_p_ts
		    && curwin->w_wrow == W_WINROW(curwin)
						 + curwin->w_height - 1 - p_so
		    && (curwin->w_cursor.lnum != curwin->w_topline
#ifdef FEAT_DIFF
			|| curwin->w_topfill > 0
#endif
		    ))
	    {
#ifdef FEAT_DIFF
		if (curwin->w_topfill > 0)
		    --curwin->w_topfill;
		else
#endif
#ifdef FEAT_FOLDING
		if (hasFolding(curwin->w_topline, NULL, &old_topline))
		    set_topline(curwin, old_topline + 1);
		else
#endif
		    set_topline(curwin, curwin->w_topline + 1);
	    }
	}

	/* May need to adjust w_topline to show the cursor. */
	update_topline();

	did_backspace = FALSE;

	validate_cursor();		/* may set must_redraw */

	/*
	 * Redraw the display when no characters are waiting.
	 * Also shows mode, ruler and positions cursor.
	 */
	ins_redraw();

#ifdef FEAT_SCROLLBIND
	if (curwin->w_p_scb)
	    do_check_scrollbind(TRUE);
#endif

	update_curswant();
	old_topline = curwin->w_topline;
#ifdef FEAT_DIFF
	old_topfill = curwin->w_topfill;
#endif

#ifdef USE_ON_FLY_SCROLL
	dont_scroll = FALSE;		/* allow scrolling here */
#endif

	/*
	 * Get a character for Insert mode.
	 */
	lastc = c;			/* remember previous char for CTRL-D */
	c = safe_vgetc();

#ifdef FEAT_RIGHTLEFT
	if (p_hkmap && KeyTyped)
	    c = hkmap(c);		/* Hebrew mode mapping */
#endif
#ifdef FEAT_FKMAP
	if (p_fkmap && KeyTyped)
	    c = fkmap(c);		/* Farsi mode mapping */
#endif

#ifdef FEAT_INS_EXPAND
	/* Prepare for or stop CTRL-X mode.  This doesn't do completion, but
	 * it does fix up the text when finishing completion. */
	ins_compl_prep(c);
#endif

	/* CTRL-\ CTRL-N goes to Normal mode, CTRL-\ CTRL-G goes to mode
	 * selected with 'insertmode'.  */
	if (c == Ctrl_BSL)
	{
	    /* may need to redraw when no more chars available now */
	    ins_redraw();
	    ++no_mapping;
	    ++allow_keys;
	    c = safe_vgetc();
	    --no_mapping;
	    --allow_keys;
	    if (c != Ctrl_N && c != Ctrl_G)	/* it's something else */
	    {
		vungetc(c);
		c = Ctrl_BSL;
	    }
	    else if (c == Ctrl_G && p_im)
		continue;
	    else
	    {
		count = 0;
		goto doESCkey;
	    }
	}

#ifdef FEAT_DIGRAPHS
	c = do_digraph(c);
#endif

#ifdef FEAT_INS_EXPAND
	if ((c == Ctrl_V || c == Ctrl_Q) && ctrl_x_mode == CTRL_X_CMDLINE)
	    goto docomplete;
#endif
	if (c == Ctrl_V || c == Ctrl_Q)
	{
	    ins_ctrl_v();
	    c = Ctrl_V;	/* pretend CTRL-V is last typed character */
	    continue;
	}

#ifdef FEAT_CINDENT
	if (cindent_on()
# ifdef FEAT_INS_EXPAND
		&& ctrl_x_mode == 0
# endif
	   )
	{
	    /* A key name preceded by a bang means this key is not to be
	     * inserted.  Skip ahead to the re-indenting below.
	     * A key name preceded by a star means that indenting has to be
	     * done before inserting the key. */
	    line_is_white = inindent(0);
	    if (in_cinkeys(c, '!', line_is_white))
		goto force_cindent;
	    if (can_cindent && in_cinkeys(c, '*', line_is_white)
							&& stop_arrow() == OK)
		do_c_expr_indent();
	}
#endif

#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	    switch (c)
	    {
		case K_LEFT:	c = K_RIGHT; break;
		case K_S_LEFT:	c = K_S_RIGHT; break;
		case K_C_LEFT:	c = K_C_RIGHT; break;
		case K_RIGHT:	c = K_LEFT; break;
		case K_S_RIGHT: c = K_S_LEFT; break;
		case K_C_RIGHT: c = K_C_LEFT; break;
	    }
#endif

#ifdef FEAT_VISUAL
	/*
	 * If 'keymodel' contains "startsel", may start selection.  If it
	 * does, a CTRL-O and c will be stuffed, we need to get these
	 * characters.
	 */
	if (ins_start_select(c))
	    continue;
#endif

	/*
	 * The big switch to handle a character in insert mode.
	 */
	switch (c)
	{
	/* toggle insert/replace mode */
	case K_INS:
	case K_KINS:
#ifdef FEAT_FKMAP
	    if (p_fkmap && p_ri)
	    {
		beep_flush();
		EMSG(farsi_text_3);	/* encoded in Farsi */
		break;
	    }
#endif
	    if (State & REPLACE_FLAG)
		State = INSERT | (State & LANGMAP);
	    else
		State = replaceState | (State & LANGMAP);
	    AppendCharToRedobuff(K_INS);
	    showmode();
#ifdef CURSOR_SHAPE
	    ui_cursor_shape();		/* may show different cursor shape */
#endif
	    break;

#ifdef FEAT_INS_EXPAND
	/* Enter CTRL-X mode */
	case Ctrl_X:
	    /* CTRL-X after CTRL-V CTRL-X doesn't do anything, so that CTRL-X
	     * CTRL-V works like CTRL-N */
	    if (ctrl_x_mode != CTRL_X_CMDLINE)
	    {
		/* if the next ^X<> won't ADD nothing, then reset
		 * continue_status */
		if (continue_status & CONT_N_ADDS)
		    continue_status = (continue_status | CONT_INTRPT);
		else
		    continue_status = 0;
		/* We're not sure which CTRL-X mode it will be yet */
		ctrl_x_mode = CTRL_X_NOT_DEFINED_YET;
		edit_submode = (char_u *)_(CTRL_X_MSG(ctrl_x_mode));
		edit_submode_pre = NULL;
		showmode();
	    }
	    break;
#endif

	/* end of Select mode mapping - ignore */
	case K_SELECT:
	    break;

	/* suspend when 'insertmode' set */
	case Ctrl_Z:
	    if (!p_im)
		goto normalchar;	/* insert CTRL-Z as normal char */
	    stuffReadbuff((char_u *)":st\r");
	    c = Ctrl_O;
	    /*FALLTHROUGH*/

	/* execute one command */
	case Ctrl_O:
	    if (echeck_abbr(Ctrl_O + ABBR_OFF))
		break;
	    count = 0;
#ifdef FEAT_VREPLACE
	    if (State & VREPLACE_FLAG)
		restart_edit = 'V';
	    else
#endif
		if (State & REPLACE_FLAG)
		restart_edit = 'R';
	    else
		restart_edit = 'I';
#ifdef FEAT_VIRTUALEDIT
	    if (virtual_active())
		o_eol = FALSE;	    /* cursor always keeps its column */
	    else
#endif
		o_eol = (gchar_cursor() == NUL);
	    goto doESCkey;

#ifdef FEAT_SNIFF
	case K_SNIFF:
	    stuffcharReadbuff(K_SNIFF);
	    goto doESCkey;
#endif

	/* Hitting the help key in insert mode is like <ESC> <Help> */
	case K_HELP:
	case K_F1:
	case K_XF1:
	    stuffcharReadbuff(K_HELP);
	    if (p_im)
		need_start_insertmode = TRUE;
	    goto doESCkey;

#ifdef FEAT_NETBEANS_INTG
	case K_F21:
	    ++no_mapping;		/* don't map the next key hits */
	    i = safe_vgetc();
	    --no_mapping;
	    netbeans_keycommand(i);
	    break;
#endif

	/* an escape ends input mode */
	case ESC:
	    if (echeck_abbr(ESC + ABBR_OFF))
		break;
	    /*FALLTHROUGH*/

	case Ctrl_C:
#ifdef FEAT_CMDWIN
	    if (c == Ctrl_C && cmdwin_type != 0)
	    {
		/* Close the cmdline window. */
		cmdwin_result = K_IGNORE;
		got_int = FALSE; /* don't stop executing autocommands et al. */
		goto doESCkey;
	    }
#endif

#ifdef UNIX
do_intr:
#endif
	    /* when 'insertmode' set, and not halfway a mapping, don't leave
	     * Insert mode */
	    if (goto_im())
	    {
		if (got_int)
		{
		    (void)vgetc();		/* flush all buffers */
		    got_int = FALSE;
		}
		else
		    vim_beep();
		break;
	    }
doESCkey:
	    /*
	     * This is the ONLY return from edit()!
	     */
	    /* Always update o_lnum, so that a "CTRL-O ." that adds a line
	     * still puts the cursor back after the inserted text. */
	    if (o_eol && gchar_cursor() == NUL)
		o_lnum = curwin->w_cursor.lnum;

	    if (ins_esc(&count, cmdchar))
		return (c == Ctrl_O);
	    continue;

	/*
	 * Insert the previously inserted text.
	 * For ^@ the trailing ESC will end the insert, unless there is an
	 * error.
	 */
	case K_ZERO:
	case NUL:
	case Ctrl_A:
	    if (stuff_inserted(NUL, 1L, (c == Ctrl_A)) == FAIL
						   && c != Ctrl_A && !p_im)
		goto doESCkey;		/* quit insert mode */
	    inserted_space = FALSE;
	    break;

	/* insert the contents of a register */
	case Ctrl_R:
	    ins_reg();
	    auto_format(FALSE, TRUE);
	    inserted_space = FALSE;
	    break;

	case Ctrl_G:
	    ins_ctrl_g();
	    break;

	case Ctrl_HAT:
	    if (map_to_exists_mode((char_u *)"", LANGMAP))
	    {
		/* ":lmap" mappings exists, Toggle use of ":lmap" mappings. */
		if (State & LANGMAP)
		{
		    curbuf->b_p_iminsert = B_IMODE_NONE;
		    State &= ~LANGMAP;
		}
		else
		{
		    curbuf->b_p_iminsert = B_IMODE_LMAP;
		    State |= LANGMAP;
#ifdef USE_IM_CONTROL
		    im_set_active(FALSE);
#endif
		}
	    }
#ifdef USE_IM_CONTROL
	    else
	    {
		/* There are no ":lmap" mappings, toggle IM */
		if (im_get_status())
		{
		    curbuf->b_p_iminsert = B_IMODE_NONE;
		    im_set_active(FALSE);
		}
		else
		{
		    curbuf->b_p_iminsert = B_IMODE_IM;
		    State &= ~LANGMAP;
		    im_set_active(TRUE);
		}
	    }
#endif
	    set_iminsert_global();
	    showmode();
#ifdef FEAT_GUI
	    /* may show different cursor shape or color */
	    if (gui.in_use)
		gui_update_cursor(TRUE, FALSE);
#endif
#if defined(FEAT_WINDOWS) && defined(FEAT_KEYMAP)
	    /* Show/unshow value of 'keymap' in status lines. */
	    status_redraw_curbuf();
#endif
	    break;

#ifdef FEAT_RIGHTLEFT
	case Ctrl__:
	    if (!p_ari)
		goto normalchar;
	    ins_ctrl_();
	    break;
#endif

	/* Make indent one shiftwidth smaller. */
	case Ctrl_D:
#if defined(FEAT_INS_EXPAND) && defined(FEAT_FIND_ID)
	    if (ctrl_x_mode == CTRL_X_PATH_DEFINES)
		goto docomplete;
#endif
	    /* FALLTHROUGH */

	/* Make indent one shiftwidth greater. */
	case Ctrl_T:
# ifdef FEAT_INS_EXPAND
	    if (c == Ctrl_T && ctrl_x_mode == CTRL_X_THESAURUS)
	    {
		if (*curbuf->b_p_tsr == NUL && *p_tsr == NUL)
		{
		    ctrl_x_mode = 0;
		    msg_attr((char_u *)_("'thesaurus' option is empty"),
			     hl_attr(HLF_E));
		    if (emsg_silent == 0)
		    {
			vim_beep();
			setcursor();
			out_flush();
			ui_delay(2000L, FALSE);
		    }
		    break;
		}
		goto docomplete;
	    }
# endif
	    ins_shift(c, lastc);
	    auto_format(FALSE, TRUE);
	    inserted_space = FALSE;
	    break;

	/* delete character under the cursor */
	case K_DEL:
	case K_KDEL:
	    ins_del();
	    auto_format(FALSE, TRUE);
	    break;

	/* delete character before the cursor */
	case K_BS:
	case Ctrl_H:
	    did_backspace = ins_bs(c, BACKSPACE_CHAR, &inserted_space);
	    auto_format(FALSE, TRUE);
	    break;

	/* delete word before the cursor */
	case Ctrl_W:
	    did_backspace = ins_bs(c, BACKSPACE_WORD, &inserted_space);
	    auto_format(FALSE, TRUE);
	    break;

	/* delete all inserted text in current line */
	case Ctrl_U:
	    did_backspace = ins_bs(c, BACKSPACE_LINE, &inserted_space);
	    auto_format(FALSE, TRUE);
	    inserted_space = FALSE;
	    break;

#ifdef FEAT_MOUSE
	case K_LEFTMOUSE:
	case K_LEFTMOUSE_NM:
	case K_LEFTDRAG:
	case K_LEFTRELEASE:
	case K_LEFTRELEASE_NM:
	case K_MIDDLEMOUSE:
	case K_MIDDLEDRAG:
	case K_MIDDLERELEASE:
	case K_RIGHTMOUSE:
	case K_RIGHTDRAG:
	case K_RIGHTRELEASE:
	case K_X1MOUSE:
	case K_X1DRAG:
	case K_X1RELEASE:
	case K_X2MOUSE:
	case K_X2DRAG:
	case K_X2RELEASE:
	    ins_mouse(c);
	    break;

	/* Default action for scroll wheel up: scroll up */
	case K_MOUSEDOWN:
	    ins_mousescroll(FALSE);
	    break;

	/* Default action for scroll wheel down: scroll down */
	case K_MOUSEUP:
	    ins_mousescroll(TRUE);
	    break;
#endif

	case K_IGNORE:
	    break;

#ifdef FEAT_GUI
	case K_VER_SCROLLBAR:
	    ins_scroll();
	    break;

	case K_HOR_SCROLLBAR:
	    ins_horscroll();
	    break;
#endif

	case K_HOME:
	case K_KHOME:
	case K_XHOME:
	case K_S_HOME:
	case K_C_HOME:
	    ins_home(c);
	    break;

	case K_END:
	case K_KEND:
	case K_XEND:
	case K_S_END:
	case K_C_END:
	    ins_end(c);
	    break;

	case K_LEFT:
	    ins_left();
	    break;

	case K_S_LEFT:
	case K_C_LEFT:
	    ins_s_left();
	    break;

	case K_RIGHT:
	    ins_right();
	    break;

	case K_S_RIGHT:
	case K_C_RIGHT:
	    ins_s_right();
	    break;

	case K_UP:
	    ins_up(FALSE);
	    break;

	case K_S_UP:
	case K_PAGEUP:
	case K_KPAGEUP:
	    ins_pageup();
	    break;

	case K_DOWN:
	    ins_down(FALSE);
	    break;

	case K_S_DOWN:
	case K_PAGEDOWN:
	case K_KPAGEDOWN:
	    ins_pagedown();
	    break;

#ifdef FEAT_DND
	case K_DROP:
	    ins_drop();
	    break;
#endif

	/* When <S-Tab> isn't mapped, use it like a normal TAB */
	case K_S_TAB:
	    c = TAB;
	    /* FALLTHROUGH */

	/* TAB or Complete patterns along path */
	case TAB:
#if defined(FEAT_INS_EXPAND) && defined(FEAT_FIND_ID)
	    if (ctrl_x_mode == CTRL_X_PATH_PATTERNS)
		goto docomplete;
#endif
	    inserted_space = FALSE;
	    if (ins_tab())
		goto normalchar;	/* insert TAB as a normal char */
	    auto_format(FALSE, TRUE);
	    break;

	case K_KENTER:
	    c = CAR;
	    /* FALLTHROUGH */
	case CAR:
	case NL:
#if defined(FEAT_WINDOWS) && defined(FEAT_QUICKFIX)
	    /* In a quickfix window a <CR> jumps to the error under the
	     * cursor. */
	    if (bt_quickfix(curbuf) && c == CAR)
	    {
		do_cmdline_cmd((char_u *)".cc");
		break;
	    }
#endif
#ifdef FEAT_CMDWIN
	    if (cmdwin_type != 0)
	    {
		/* Execute the command in the cmdline window. */
		cmdwin_result = CAR;
		goto doESCkey;
	    }
#endif
	    if (ins_eol(c) && !p_im)
		goto doESCkey;	    /* out of memory */
	    auto_format(FALSE, FALSE);
	    inserted_space = FALSE;
	    break;

#if defined(FEAT_DIGRAPHS) || defined (FEAT_INS_EXPAND)
	case Ctrl_K:
# ifdef FEAT_INS_EXPAND
	    if (ctrl_x_mode == CTRL_X_DICTIONARY)
	    {
		if (*curbuf->b_p_dict == NUL && *p_dict == NUL)
		{
		    ctrl_x_mode = 0;
		    msg_attr((char_u *)_("'dictionary' option is empty"),
			     hl_attr(HLF_E));
		    if (emsg_silent == 0)
		    {
			vim_beep();
			setcursor();
			out_flush();
			ui_delay(2000L, FALSE);
		    }
		    break;
		}
		goto docomplete;
	    }
# endif
# ifdef FEAT_DIGRAPHS
	    c = ins_digraph();
	    if (c == NUL)
		break;
# endif
	    goto normalchar;
#endif /* FEAT_DIGRAPHS || FEAT_INS_EXPAND */

#ifdef FEAT_INS_EXPAND
	case Ctrl_RSB:		/* Tag name completion after ^X */
	    if (ctrl_x_mode != CTRL_X_TAGS)
		goto normalchar;
	    goto docomplete;

	case Ctrl_F:		/* File name completion after ^X */
	    if (ctrl_x_mode != CTRL_X_FILES)
		goto normalchar;
	    goto docomplete;
#endif

	case Ctrl_L:		/* Whole line completion after ^X */
#ifdef FEAT_INS_EXPAND
	    if (ctrl_x_mode != CTRL_X_WHOLE_LINE)
#endif
	    {
		/* CTRL-L with 'insertmode' set: Leave Insert mode */
		if (p_im)
		{
		    if (echeck_abbr(Ctrl_L + ABBR_OFF))
			break;
		    goto doESCkey;
		}
		goto normalchar;
	    }
#ifdef FEAT_INS_EXPAND
	    /* FALLTHROUGH */

	/* Do previous/next pattern completion */
	case Ctrl_P:
	case Ctrl_N:
	    /* if 'complete' is empty then plain ^P is no longer special,
	     * but it is under other ^X modes */
	    if (*curbuf->b_p_cpt == NUL
		    && ctrl_x_mode != 0
		    && !(continue_status & CONT_LOCAL))
		goto normalchar;

docomplete:
	    if (ins_complete(c) == FAIL)
		continue_status = 0;
	    break;
#endif /* FEAT_INS_EXPAND */

	case Ctrl_Y:		/* copy from previous line or scroll down */
	case Ctrl_E:		/* copy from next line	   or scroll up */
#ifdef FEAT_INS_EXPAND
	    if (ctrl_x_mode == CTRL_X_SCROLL)
	    {
		if (c == Ctrl_Y)
		    scrolldown_clamp();
		else
		    scrollup_clamp();
		redraw_later(VALID);
	    }
	    else
#endif
	    {
		c = ins_copychar(curwin->w_cursor.lnum
						 + (c == Ctrl_Y ? -1 : 1));
		if (c != NUL)
		{
		    long	tw_save;

		    /* The character must be taken literally, insert like it
		     * was typed after a CTRL-V, and pretend 'textwidth'
		     * wasn't set.  Digits, 'o' and 'x' are special after a
		     * CTRL-V, don't use it for these. */
		    if (c < 256 && !isalnum(c))
			AppendToRedobuff((char_u *)CTRL_V_STR);	/* CTRL-V */
		    tw_save = curbuf->b_p_tw;
		    curbuf->b_p_tw = -1;
		    insert_special(c, TRUE, FALSE);
		    curbuf->b_p_tw = tw_save;
#ifdef FEAT_RIGHTLEFT
		    revins_chars++;
		    revins_legal++;
#endif
		    c = Ctrl_V;	/* pretend CTRL-V is last character */
		    auto_format(FALSE, TRUE);
		}
	    }
	    break;

	  default:
#ifdef UNIX
	    if (c == intr_char)		/* special interrupt char */
		goto do_intr;
#endif

	    /*
	     * Insert a nomal character.
	     */
normalchar:
#ifdef FEAT_SMARTINDENT
	    /* Try to perform smart-indenting. */
	    ins_try_si(c);
#endif

	    if (c == ' ')
	    {
		inserted_space = TRUE;
#ifdef FEAT_CINDENT
		if (inindent(0))
		    can_cindent = FALSE;
#endif
		if (Insstart_blank_vcol == MAXCOL
			&& curwin->w_cursor.lnum == Insstart.lnum)
		    Insstart_blank_vcol = get_nolist_virtcol();
	    }

	    if (vim_iswordc(c) || !echeck_abbr(
#ifdef FEAT_MBYTE
			/* Add ABBR_OFF for characters above 0x100, this is
			 * what check_abbr() expects. */
			(has_mbyte && c >= 0x100) ? (c + ABBR_OFF) :
#endif
			c))
	    {
		insert_special(c, FALSE, FALSE);
#ifdef FEAT_RIGHTLEFT
		revins_legal++;
		revins_chars++;
#endif
	    }

	    auto_format(FALSE, TRUE);

#ifdef FEAT_FOLDING
	    /* When inserting a character the cursor line must never be in a
	     * closed fold. */
	    foldOpenCursor();
#endif
	    break;
	}   /* end of switch (c) */

	/* If the cursor was moved we didn't just insert a space */
	if (arrow_used)
	    inserted_space = FALSE;

#ifdef FEAT_CINDENT
	if (can_cindent && cindent_on()
# ifdef FEAT_INS_EXPAND
		&& ctrl_x_mode == 0
# endif
	   )
	{
force_cindent:
	    /*
	     * Indent now if a key was typed that is in 'cinkeys'.
	     */
	    if (in_cinkeys(c, ' ', line_is_white))
	    {
		if (stop_arrow() == OK)
		    /* re-indent the current line */
		    do_c_expr_indent();
	    }
	}
#endif /* FEAT_CINDENT */

    }	/* for (;;) */
    /* NOTREACHED */
}

/*
 * Redraw for Insert mode.
 * This is postponed until getting the next character to make '$' in the 'cpo'
 * option work correctly.
 * Only redraw when there are no characters available.  This speeds up
 * inserting sequences of characters (e.g., for CTRL-R).
 */
    static void
ins_redraw()
{
    if (!char_avail())
    {
	if (must_redraw)
	    update_screen(0);
	else if (clear_cmdline || redraw_cmdline)
	    showmode();		/* clear cmdline and show mode */
	showruler(FALSE);
	setcursor();
	emsg_on_display = FALSE;	/* may remove error message now */
    }
}

/*
 * Handle a CTRL-V or CTRL-Q typed in Insert mode.
 */
    static void
ins_ctrl_v()
{
    int		c;

    /* may need to redraw when no more chars available now */
    ins_redraw();

    if (redrawing() && !char_avail())
	edit_putchar('^', TRUE);
    AppendToRedobuff((char_u *)CTRL_V_STR);	/* CTRL-V */

#ifdef FEAT_CMDL_INFO
    add_to_showcmd_c(Ctrl_V);
#endif

    c = get_literal();
#ifdef FEAT_CMDL_INFO
    clear_showcmd();
#endif
    insert_special(c, FALSE, TRUE);
#ifdef FEAT_RIGHTLEFT
    revins_chars++;
    revins_legal++;
#endif
}

/*
 * Put a character directly onto the screen.  It's not stored in a buffer.
 * Used while handling CTRL-K, CTRL-V, etc. in Insert mode.
 */
static int  pc_status;
#define PC_STATUS_UNSET	0	/* pc_bytes was not set */
#define PC_STATUS_RIGHT	1	/* right halve of double-wide char */
#define PC_STATUS_LEFT	2	/* left halve of double-wide char */
#define PC_STATUS_SET	3	/* pc_bytes was filled */
#ifdef FEAT_MBYTE
static char_u pc_bytes[MB_MAXBYTES + 1]; /* saved bytes */
#else
static char_u pc_bytes[2];		/* saved bytes */
#endif
static int  pc_attr;
static int  pc_row;
static int  pc_col;

    void
edit_putchar(c, highlight)
    int	    c;
    int	    highlight;
{
    int	    attr;

    if (ScreenLines != NULL)
    {
	update_topline();	/* just in case w_topline isn't valid */
	validate_cursor();
	if (highlight)
	    attr = hl_attr(HLF_8);
	else
	    attr = 0;
	pc_row = W_WINROW(curwin) + curwin->w_wrow;
	pc_col = W_WINCOL(curwin);
#if defined(FEAT_RIGHTLEFT) || defined(FEAT_MBYTE)
	pc_status = PC_STATUS_UNSET;
#endif
#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	{
	    pc_col += W_WIDTH(curwin) - 1 - curwin->w_wcol;
# ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		int fix_col = mb_fix_col(pc_col, pc_row);

		if (fix_col != pc_col)
		{
		    screen_putchar(' ', pc_row, fix_col, attr);
		    --curwin->w_wcol;
		    pc_status = PC_STATUS_RIGHT;
		}
	    }
# endif
	}
	else
#endif
	{
	    pc_col += curwin->w_wcol;
#ifdef FEAT_MBYTE
	    if (mb_lefthalve(pc_row, pc_col))
		pc_status = PC_STATUS_LEFT;
#endif
	}

	/* save the character to be able to put it back */
#if defined(FEAT_RIGHTLEFT) || defined(FEAT_MBYTE)
	if (pc_status == PC_STATUS_UNSET)
#endif
	{
	    screen_getbytes(pc_row, pc_col, pc_bytes, &pc_attr);
	    pc_status = PC_STATUS_SET;
	}
	screen_putchar(c, pc_row, pc_col, attr);
    }
}

/*
 * Undo the previous edit_putchar().
 */
    void
edit_unputchar()
{
    if (pc_status != PC_STATUS_UNSET && pc_row >= msg_scrolled)
    {
#if defined(FEAT_MBYTE)
	if (pc_status == PC_STATUS_RIGHT)
	    ++curwin->w_wcol;
	if (pc_status == PC_STATUS_RIGHT || pc_status == PC_STATUS_LEFT)
	    redrawWinline(curwin->w_cursor.lnum, FALSE);
	else
#endif
	    screen_puts(pc_bytes, pc_row - msg_scrolled, pc_col, pc_attr);
    }
}

/*
 * Called when p_dollar is set: display a '$' at the end of the changed text
 * Only works when cursor is in the line that changes.
 */
    void
display_dollar(col)
    colnr_T	col;
{
    colnr_T save_col;

    if (!redrawing())
	return;

    cursor_off();
    save_col = curwin->w_cursor.col;
    curwin->w_cursor.col = col;
#ifdef FEAT_MBYTE
    if (has_mbyte)
    {
	char_u *p;

	/* If on the last byte of a multi-byte move to the first byte. */
	p = ml_get_curline();
	curwin->w_cursor.col -= (*mb_head_off)(p, p + col);
    }
#endif
    curs_columns(FALSE);	    /* recompute w_wrow and w_wcol */
    if (curwin->w_wcol < W_WIDTH(curwin))
    {
	edit_putchar('$', FALSE);
	dollar_vcol = curwin->w_virtcol;
    }
    curwin->w_cursor.col = save_col;
}

/*
 * Call this function before moving the cursor from the normal insert position
 * in insert mode.
 */
    static void
undisplay_dollar()
{
    if (dollar_vcol)
    {
	dollar_vcol = 0;
	redrawWinline(curwin->w_cursor.lnum, FALSE);
    }
}

/*
 * Insert an indent (for <Tab> or CTRL-T) or delete an indent (for CTRL-D).
 * Keep the cursor on the same character.
 * type == INDENT_INC	increase indent (for CTRL-T or <Tab>)
 * type == INDENT_DEC	decrease indent (for CTRL-D)
 * type == INDENT_SET	set indent to "amount"
 * if round is TRUE, round the indent to 'shiftwidth' (only with _INC and _Dec).
 */
    void
change_indent(type, amount, round, replaced)
    int		type;
    int		amount;
    int		round;
    int		replaced;	/* replaced character, put on replace stack */
{
    int		vcol;
    int		last_vcol;
    int		insstart_less;		/* reduction for Insstart.col */
    int		new_cursor_col;
    int		i;
    char_u	*ptr;
    int		save_p_list;
    int		start_col;
    colnr_T	vc;
#ifdef FEAT_VREPLACE
    colnr_T	orig_col = 0;		/* init for GCC */
    char_u	*new_line, *orig_line = NULL;	/* init for GCC */

    /* VREPLACE mode needs to know what the line was like before changing */
    if (State & VREPLACE_FLAG)
    {
	orig_line = vim_strsave(ml_get_curline());  /* Deal with NULL below */
	orig_col = curwin->w_cursor.col;
    }
#endif

    /* for the following tricks we don't want list mode */
    save_p_list = curwin->w_p_list;
    curwin->w_p_list = FALSE;
    vc = getvcol_nolist(&curwin->w_cursor);
    vcol = vc;

    /*
     * For Replace mode we need to fix the replace stack later, which is only
     * possible when the cursor is in the indent.  Remember the number of
     * characters before the cursor if it's possible.
     */
    start_col = curwin->w_cursor.col;

    /* determine offset from first non-blank */
    new_cursor_col = curwin->w_cursor.col;
    beginline(BL_WHITE);
    new_cursor_col -= curwin->w_cursor.col;

    insstart_less = curwin->w_cursor.col;

    /*
     * If the cursor is in the indent, compute how many screen columns the
     * cursor is to the left of the first non-blank.
     */
    if (new_cursor_col < 0)
	vcol = get_indent() - vcol;

    if (new_cursor_col > 0)	    /* can't fix replace stack */
	start_col = -1;

    /*
     * Set the new indent.  The cursor will be put on the first non-blank.
     */
    if (type == INDENT_SET)
	(void)set_indent(amount, SIN_CHANGED);
    else
    {
#ifdef FEAT_VREPLACE
	int	save_State = State;

	/* Avoid being called recursively. */
	if (State & VREPLACE_FLAG)
	    State = INSERT;
#endif
	shift_line(type == INDENT_DEC, round, 1);
#ifdef FEAT_VREPLACE
	State = save_State;
#endif
    }
    insstart_less -= curwin->w_cursor.col;

    /*
     * Try to put cursor on same character.
     * If the cursor is at or after the first non-blank in the line,
     * compute the cursor column relative to the column of the first
     * non-blank character.
     * If we are not in insert mode, leave the cursor on the first non-blank.
     * If the cursor is before the first non-blank, position it relative
     * to the first non-blank, counted in screen columns.
     */
    if (new_cursor_col >= 0)
    {
	/*
	 * When changing the indent while the cursor is touching it, reset
	 * Insstart_col to 0.
	 */
	if (new_cursor_col == 0)
	    insstart_less = MAXCOL;
	new_cursor_col += curwin->w_cursor.col;
    }
    else if (!(State & INSERT))
	new_cursor_col = curwin->w_cursor.col;
    else
    {
	/*
	 * Compute the screen column where the cursor should be.
	 */
	vcol = get_indent() - vcol;
	curwin->w_virtcol = (vcol < 0) ? 0 : vcol;

	/*
	 * Advance the cursor until we reach the right screen column.
	 */
	vcol = last_vcol = 0;
	new_cursor_col = -1;
	ptr = ml_get_curline();
	while (vcol <= (int)curwin->w_virtcol)
	{
	    last_vcol = vcol;
#ifdef FEAT_MBYTE
	    if (has_mbyte && new_cursor_col >= 0)
		new_cursor_col += (*mb_ptr2len_check)(ptr + new_cursor_col);
	    else
#endif
		++new_cursor_col;
	    vcol += lbr_chartabsize(ptr + new_cursor_col, (colnr_T)vcol);
	}
	vcol = last_vcol;

	/*
	 * May need to insert spaces to be able to position the cursor on
	 * the right screen column.
	 */
	if (vcol != (int)curwin->w_virtcol)
	{
	    curwin->w_cursor.col = new_cursor_col;
	    i = (int)curwin->w_virtcol - vcol;
	    ptr = alloc(i + 1);
	    if (ptr != NULL)
	    {
		new_cursor_col += i;
		ptr[i] = NUL;
		while (--i >= 0)
		    ptr[i] = ' ';
		ins_str(ptr);
		vim_free(ptr);
	    }
	}

	/*
	 * When changing the indent while the cursor is in it, reset
	 * Insstart_col to 0.
	 */
	insstart_less = MAXCOL;
    }

    curwin->w_p_list = save_p_list;

    if (new_cursor_col <= 0)
	curwin->w_cursor.col = 0;
    else
	curwin->w_cursor.col = new_cursor_col;
    curwin->w_set_curswant = TRUE;
    changed_cline_bef_curs();

    /*
     * May have to adjust the start of the insert.
     */
    if (State & INSERT)
    {
	if (curwin->w_cursor.lnum == Insstart.lnum && Insstart.col != 0)
	{
	    if ((int)Insstart.col <= insstart_less)
		Insstart.col = 0;
	    else
		Insstart.col -= insstart_less;
	}
	if ((int)ai_col <= insstart_less)
	    ai_col = 0;
	else
	    ai_col -= insstart_less;
    }

    /*
     * For REPLACE mode, may have to fix the replace stack, if it's possible.
     * If the number of characters before the cursor decreased, need to pop a
     * few characters from the replace stack.
     * If the number of characters before the cursor increased, need to push a
     * few NULs onto the replace stack.
     */
    if (REPLACE_NORMAL(State) && start_col >= 0)
    {
	while (start_col > (int)curwin->w_cursor.col)
	{
	    replace_join(0);	    /* remove a NUL from the replace stack */
	    --start_col;
	}
	while (start_col < (int)curwin->w_cursor.col || replaced)
	{
	    replace_push(NUL);
	    if (replaced)
	    {
		replace_push(replaced);
		replaced = NUL;
	    }
	    ++start_col;
	}
    }

#ifdef FEAT_VREPLACE
    /*
     * For VREPLACE mode, we also have to fix the replace stack.  In this case
     * it is always possible because we backspace over the whole line and then
     * put it back again the way we wanted it.
     */
    if (State & VREPLACE_FLAG)
    {
	/* If orig_line didn't allocate, just return.  At least we did the job,
	 * even if you can't backspace. */
	if (orig_line == NULL)
	    return;

	/* Save new line */
	new_line = vim_strsave(ml_get_curline());
	if (new_line == NULL)
	    return;

	/* We only put back the new line up to the cursor */
	new_line[curwin->w_cursor.col] = NUL;

	/* Put back original line */
	ml_replace(curwin->w_cursor.lnum, orig_line, FALSE);
	curwin->w_cursor.col = orig_col;

	/* Backspace from cursor to start of line */
	backspace_until_column(0);

	/* Insert new stuff into line again */
	ins_bytes(new_line);

	vim_free(new_line);
    }
#endif
}

/*
 * Truncate the space at the end of a line.  This is to be used only in an
 * insert mode.  It handles fixing the replace stack for REPLACE and VREPLACE
 * modes.
 */
    void
truncate_spaces(line)
    char_u  *line;
{
    int	    i;

    /* find start of trailing white space */
    for (i = (int)STRLEN(line) - 1; i >= 0 && vim_iswhite(line[i]); i--)
    {
	if (State & REPLACE_FLAG)
	    replace_join(0);	    /* remove a NUL from the replace stack */
    }
    line[i + 1] = NUL;
}

#if defined(FEAT_VREPLACE) || defined(FEAT_INS_EXPAND) \
	|| defined(FEAT_COMMENTS) || defined(PROTO)
/*
 * Backspace the cursor until the given column.  Handles REPLACE and VREPLACE
 * modes correctly.  May also be used when not in insert mode at all.
 */
    void
backspace_until_column(col)
    int	    col;
{
    while ((int)curwin->w_cursor.col > col)
    {
	curwin->w_cursor.col--;
	if (State & REPLACE_FLAG)
	    replace_do_bs();
	else
	    (void)del_char(FALSE);
    }
}
#endif

#if defined(FEAT_INS_EXPAND) || defined(PROTO)
/*
 * Is the character 'c' a valid key to go to or keep us in CTRL-X mode?
 * This depends on the current mode.
 */
    int
vim_is_ctrl_x_key(c)
    int	    c;
{
    /* Always allow ^R - let it's results then be checked */
    if (c == Ctrl_R)
	return TRUE;

    switch (ctrl_x_mode)
    {
	case 0:		    /* Not in any CTRL-X mode */
	    return (c == Ctrl_N || c == Ctrl_P || c == Ctrl_X);
	case CTRL_X_NOT_DEFINED_YET:
	    return (       c == Ctrl_X || c == Ctrl_Y || c == Ctrl_E
		    || c == Ctrl_L || c == Ctrl_F || c == Ctrl_RSB
		    || c == Ctrl_I || c == Ctrl_D || c == Ctrl_P
		    || c == Ctrl_N || c == Ctrl_T || c == Ctrl_V
		    || c == Ctrl_Q);
	case CTRL_X_SCROLL:
	    return (c == Ctrl_Y || c == Ctrl_E);
	case CTRL_X_WHOLE_LINE:
	    return (c == Ctrl_L || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_FILES:
	    return (c == Ctrl_F || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_DICTIONARY:
	    return (c == Ctrl_K || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_THESAURUS:
	    return (c == Ctrl_T || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_TAGS:
	    return (c == Ctrl_RSB || c == Ctrl_P || c == Ctrl_N);
#ifdef FEAT_FIND_ID
	case CTRL_X_PATH_PATTERNS:
	    return (c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_PATH_DEFINES:
	    return (c == Ctrl_D || c == Ctrl_P || c == Ctrl_N);
#endif
	case CTRL_X_CMDLINE:
	    return (c == Ctrl_V || c == Ctrl_Q || c == Ctrl_P || c == Ctrl_N
		    || c == Ctrl_X);
    }
    EMSG(_(e_internal));
    return FALSE;
}

/*
 * This is like ins_compl_add(), but if ic and inf are set, then the
 * case of the originally typed text is used, and the case of the completed
 * text is infered, ie this tries to work out what case you probably wanted
 * the rest of the word to be in -- webb
 */
    int
ins_compl_add_infercase(str, len, fname, dir, reuse)
    char_u	*str;
    int		len;
    char_u	*fname;
    int		dir;
    int		reuse;
{
    int		has_lower = FALSE;
    int		was_letter = FALSE;
    int		idx;

    if (p_ic && curbuf->b_p_inf && len < IOSIZE)
    {
	/* Infer case of completed part -- webb */
	/* Use IObuff, str would change text in buffer! */
	STRNCPY(IObuff, str, len);
	IObuff[len] = NUL;

	/* Rule 1: Were any chars converted to lower? */
	for (idx = 0; idx < completion_length; ++idx)
	{
	    if (islower(original_text[idx]))
	    {
		has_lower = TRUE;
		if (isupper(IObuff[idx]))
		{
		    /* Rule 1 is satisfied */
		    for (idx = completion_length; idx < len; ++idx)
			IObuff[idx] = TOLOWER_LOC(IObuff[idx]);
		    break;
		}
	    }
	}

	/*
	 * Rule 2: No lower case, 2nd consecutive letter converted to
	 * upper case.
	 */
	if (!has_lower)
	{
	    for (idx = 0; idx < completion_length; ++idx)
	    {
		if (was_letter && isupper(original_text[idx])
						      && islower(IObuff[idx]))
		{
		    /* Rule 2 is satisfied */
		    for (idx = completion_length; idx < len; ++idx)
			IObuff[idx] = TOUPPER_LOC(IObuff[idx]);
		    break;
		}
		was_letter = isalpha(original_text[idx]);
	    }
	}

	/* Copy the original case of the part we typed */
	STRNCPY(IObuff, original_text, completion_length);

	return ins_compl_add(IObuff, len, fname, dir, reuse);
    }
    return ins_compl_add(str, len, fname, dir, reuse);
}

/*
 * Add a match to the list of matches.
 * If the given string is already in the list of completions, then return
 * FAIL, otherwise add it to the list and return OK.  If there is an error,
 * maybe because alloc returns NULL, then RET_ERROR is returned -- webb.
 */
    static int
ins_compl_add(str, len, fname, dir, reuse)
    char_u	*str;
    int		len;
    char_u	*fname;
    int		dir;
    int		reuse;
{
    struct Completion *match;

    ui_breakcheck();
    if (got_int)
	return RET_ERROR;
    if (len < 0)
	len = (int)STRLEN(str);

    /*
     * If the same match is already present, don't add it.
     */
    if (first_match != NULL)
    {
	match = first_match;
	do
	{
	    if (    !(match->original & ORIGINAL_TEXT)
		    && STRNCMP(match->str, str, (size_t)len) == 0
		    && match->str[len] == NUL)
		return FAIL;
	    match = match->next;
	} while (match != NULL && match != first_match);
    }

    /*
     * Allocate a new match structure.
     * Copy the values to the new match structure.
     */
    match = (struct Completion *)alloc((unsigned)sizeof(struct Completion));
    if (match == NULL)
	return RET_ERROR;
    match->number = -1;
    if (reuse & ORIGINAL_TEXT)
    {
	match->number = 0;
	match->str = original_text;
    }
    else if ((match->str = vim_strnsave(str, len)) == NULL)
    {
	vim_free(match);
	return RET_ERROR;
    }
    /* match-fname is:
     * - curr_match->fname if it is a string equal to fname.
     * - a copy of fname, FREE_FNAME is set to free later THE allocated mem.
     * - NULL otherwise.	--Acevedo */
    if (fname && curr_match && curr_match->fname
	      && STRCMP(fname, curr_match->fname) == 0)
	match->fname = curr_match->fname;
    else if (fname && (match->fname = vim_strsave(fname)) != NULL)
	reuse |= FREE_FNAME;
    else
	match->fname = NULL;
    match->original = reuse;

    /*
     * Link the new match structure in the list of matches.
     */
    if (first_match == NULL)
	match->next = match->prev = NULL;
    else if (dir == FORWARD)
    {
	match->next = curr_match->next;
	match->prev = curr_match;
    }
    else	/* BACKWARD */
    {
	match->next = curr_match;
	match->prev = curr_match->prev;
    }
    if (match->next)
	match->next->prev = match;
    if (match->prev)
	match->prev->next = match;
    else	/* if there's nothing before, it is the first match */
	first_match = match;
    curr_match = match;

    return OK;
}

/*
 * Add an array of matches to the list of matches.
 * Frees matches[].
 */
    static void
ins_compl_add_matches(num_matches, matches, dir)
    int		num_matches;
    char_u	**matches;
    int		dir;
{
    int		i;
    int		add_r = OK;
    int		ldir = dir;

    for (i = 0; i < num_matches && add_r != RET_ERROR; i++)
	if ((add_r = ins_compl_add(matches[i], -1, NULL, ldir, 0)) == OK)
	    /* if dir was BACKWARD then honor it just once */
	    ldir = FORWARD;
    FreeWild(num_matches, matches);
}

/* Make the completion list cyclic.
 * Return the number of matches (excluding the original).
 */
    static int
ins_compl_make_cyclic()
{
    struct Completion *match;
    int	    count = 0;

    if (first_match != NULL)
    {
	/*
	 * Find the end of the list.
	 */
	match = first_match;
	/* there's always an entry for the original_text, it doesn't count. */
	while (match->next != NULL && match->next != first_match)
	{
	    match = match->next;
	    ++count;
	}
	match->next = first_match;
	first_match->prev = match;
    }
    return count;
}

#define DICT_FIRST	(1)	/* use just first element in "dict" */
#define DICT_EXACT	(2)	/* "dict" is the exact name of a file */
/*
 * Add any identifiers that match the given pattern to the list of
 * completions.
 */
    static void
ins_compl_dictionaries(dict, pat, dir, flags, thesaurus)
    char_u	*dict;
    char_u	*pat;
    int		dir;
    int		flags;
    int		thesaurus;
{
    char_u	*ptr;
    char_u	*buf;
    FILE	*fp;
    regmatch_T	regmatch;
    int		add_r;
    char_u	**files;
    int		count;
    int		i;
    int		save_p_scs;

    buf = alloc(LSIZE);
    /* If 'infercase' is set, don't use 'smartcase' here */
    save_p_scs = p_scs;
    if (curbuf->b_p_inf)
	p_scs = FALSE;
    regmatch.regprog = vim_regcomp(pat, p_magic ? RE_MAGIC : 0);
    /* ignore case depends on 'ignorecase', 'smartcase' and "pat" */
    regmatch.rm_ic = ignorecase(pat);
    while (buf != NULL && regmatch.regprog != NULL && *dict != NUL
		       && !got_int && !completion_interrupted)
    {
	/* copy one dictionary file name into buf */
	if (flags == DICT_EXACT)
	{
	    count = 1;
	    files = &dict;
	}
	else
	{
	    /* Expand wildcards in the dictionary name, but do not allow
	     * backticks (for security, the 'dict' option may have been set in
	     * a modeline). */
	    copy_option_part(&dict, buf, LSIZE, ",");
	    if (vim_strchr(buf, '`') != NULL
		    || expand_wildcards(1, &buf, &count, &files,
						     EW_FILE|EW_SILENT) != OK)
		count = 0;
	}

	for (i = 0; i < count && !got_int && !completion_interrupted; i++)
	{
	    fp = mch_fopen((char *)files[i], "r");  /* open dictionary file */
	    if (flags != DICT_EXACT)
	    {
		sprintf((char*)IObuff, _("Scanning dictionary: %s"),
							    (char *)files[i]);
		msg_trunc_attr(IObuff, TRUE, hl_attr(HLF_R));
	    }

	    if (fp != NULL)
	    {
		/*
		 * Read dictionary file line by line.
		 * Check each line for a match.
		 */
		while (!got_int && !completion_interrupted
				&& !vim_fgets(buf, LSIZE, fp))
		{
		    ptr = buf;
		    while (vim_regexec(&regmatch, buf, (colnr_T)(ptr - buf)))
		    {
			ptr = regmatch.startp[0];
			ptr = find_word_end(ptr);
			add_r = ins_compl_add_infercase(regmatch.startp[0],
					      (int)(ptr - regmatch.startp[0]),
							    files[i], dir, 0);
			if (thesaurus)
			{
			    char_u *wstart;

			    /*
			     * Add the other matches on the line
			     */
			    while (!got_int)
			    {
				/* Find start of the next word.  Skip white
				 * space and punctuation. */
				ptr = find_word_start(ptr);
				if (*ptr == NUL || *ptr == NL)
				    break;
				wstart = ptr;

				/* Find end of the word and add it. */
#ifdef FEAT_MBYTE
				if (has_mbyte)
				    /* Japanese words may have characters in
				     * different classes, only separate words
				     * with single-byte non-word characters. */
				    while (*ptr != NUL)
				    {
					int l = (*mb_ptr2len_check)(ptr);

					if (l < 2 && !vim_iswordc(*ptr))
					    break;
					ptr += l;
				    }
				else
#endif
				    ptr = find_word_end(ptr);
				add_r = ins_compl_add_infercase(wstart,
					(int)(ptr - wstart), files[i], dir, 0);
			    }
			}
			if (add_r == OK)
			    /* if dir was BACKWARD then honor it just once */
			    dir = FORWARD;
			else if (add_r == RET_ERROR)
			    break;
			/* avoid expensive call to vim_regexec() when at end
			 * of line */
			if (*ptr == '\n' || got_int)
			    break;
		    }
		    line_breakcheck();
		    ins_compl_check_keys();
		}
		fclose(fp);
	    }
	}
	if (flags != DICT_EXACT)
	    FreeWild(count, files);
	if (flags)
	    break;
    }
    p_scs = save_p_scs;
    vim_free(regmatch.regprog);
    vim_free(buf);
}

/*
 * Find the start of the next word.
 * Returns a pointer to the first char of the word.  Also stops at a NUL.
 */
    char_u *
find_word_start(ptr)
    char_u	*ptr;
{
#ifdef FEAT_MBYTE
    if (has_mbyte)
	while (*ptr != NUL && *ptr != '\n' && mb_get_class(ptr) <= 1)
	    ptr += (*mb_ptr2len_check)(ptr);
    else
#endif
	while (*ptr != NUL && *ptr != '\n' && !vim_iswordc(*ptr))
	    ++ptr;
    return ptr;
}

/*
 * Find the end of the word.  Assumes it starts inside a word.
 * Returns a pointer to just after the word.
 */
    char_u *
find_word_end(ptr)
    char_u	*ptr;
{
#ifdef FEAT_MBYTE
    int		start_class;

    if (has_mbyte)
    {
	start_class = mb_get_class(ptr);
	if (start_class > 1)
	    while (*ptr != NUL)
	    {
		ptr += (*mb_ptr2len_check)(ptr);
		if (mb_get_class(ptr) != start_class)
		    break;
	    }
    }
    else
#endif
	while (vim_iswordc(*ptr))
	    ++ptr;
    return ptr;
}

/*
 * Free the list of completions
 */
    static void
ins_compl_free()
{
    struct Completion *match;

    vim_free(complete_pat);
    complete_pat = NULL;

    if (first_match == NULL)
	return;
    curr_match = first_match;
    do
    {
	match = curr_match;
	curr_match = curr_match->next;
	vim_free(match->str);
	/* several entries may use the same fname, free it just once. */
	if (match->original & FREE_FNAME)
	    vim_free(match->fname);
	vim_free(match);
    } while (curr_match != NULL && curr_match != first_match);
    first_match = curr_match = NULL;
}

    static void
ins_compl_clear()
{
    continue_status = 0;
    started_completion = FALSE;
    completion_matches = 0;
    vim_free(complete_pat);
    complete_pat = NULL;
    save_sm = -1;
    edit_submode_extra = NULL;
}

/*
 * Prepare for Insert mode completion, or stop it.
 */
    static void
ins_compl_prep(c)
    int	    c;
{
    char_u	*ptr;
    char_u	*tmp_ptr;
    int		temp;
    int		want_cindent;

    /* Forget any previous 'special' messages if this is actually
     * a ^X mode key - bar ^R, in which case we wait to see what it gives us.
     */
    if (c != Ctrl_R && vim_is_ctrl_x_key(c))
	edit_submode_extra = NULL;

    /* Ignore end of Select mode mapping */
    if (c == K_SELECT)
	return;

    if (ctrl_x_mode == CTRL_X_NOT_DEFINED_YET)
    {
	/*
	 * We have just typed CTRL-X and aren't quite sure which CTRL-X mode
	 * it will be yet.  Now we decide.
	 */
	switch (c)
	{
	    case Ctrl_E:
	    case Ctrl_Y:
		ctrl_x_mode = CTRL_X_SCROLL;
		if (!(State & REPLACE_FLAG))
		    edit_submode = (char_u *)_(" (insert) Scroll (^E/^Y)");
		else
		    edit_submode = (char_u *)_(" (replace) Scroll (^E/^Y)");
		edit_submode_pre = NULL;
		showmode();
		break;
	    case Ctrl_L:
		ctrl_x_mode = CTRL_X_WHOLE_LINE;
		break;
	    case Ctrl_F:
		ctrl_x_mode = CTRL_X_FILES;
		break;
	    case Ctrl_K:
		ctrl_x_mode = CTRL_X_DICTIONARY;
		break;
	    case Ctrl_R:
		/* Simply allow ^R to happen without affecting ^X mode */
		break;
	    case Ctrl_T:
		ctrl_x_mode = CTRL_X_THESAURUS;
		break;
	    case Ctrl_RSB:
		ctrl_x_mode = CTRL_X_TAGS;
		break;
#ifdef FEAT_FIND_ID
	    case Ctrl_I:
	    case K_S_TAB:
		ctrl_x_mode = CTRL_X_PATH_PATTERNS;
		break;
	    case Ctrl_D:
		ctrl_x_mode = CTRL_X_PATH_DEFINES;
		break;
#endif
	    case Ctrl_V:
	    case Ctrl_Q:
		ctrl_x_mode = CTRL_X_CMDLINE;
		break;
	    case Ctrl_P:
	    case Ctrl_N:
		/* ^X^P means LOCAL expansion if nothing interrupted (eg we
		 * just started ^X mode, or there were enough ^X's to cancel
		 * the previous mode, say ^X^F^X^X^P or ^P^X^X^X^P, see below)
		 * do normal expansion when interrupting a different mode (say
		 * ^X^F^X^P or ^P^X^X^P, see below)
		 * nothing changes if interrupting mode 0, (eg, the flag
		 * doesn't change when going to ADDING mode  -- Acevedo */
		if (!(continue_status & CONT_INTRPT))
		    continue_status |= CONT_LOCAL;
		else if (continue_mode != 0)
		    continue_status &= ~CONT_LOCAL;
		/* FALLTHROUGH */
	    default:
		/* if we have typed at least 2 ^X's... for modes != 0, we set
		 * continue_status = 0 (eg, as if we had just started ^X mode)
		 * for mode 0, we set continue_mode to an impossible value, in
		 * both cases ^X^X can be used to restart the same mode
		 * (avoiding ADDING mode).   Undocumented feature:
		 * In a mode != 0 ^X^P and ^X^X^P start 'complete' and local
		 * ^P expansions respectively.	In mode 0 an extra ^X is
		 * needed since ^X^P goes to ADDING mode  -- Acevedo */
		if (c == Ctrl_X)
		{
		    if (continue_mode != 0)
			continue_status = 0;
		    else
			continue_mode = CTRL_X_NOT_DEFINED_YET;
		}
		ctrl_x_mode = 0;
		edit_submode = NULL;
		showmode();
		break;
	}
    }
    else if (ctrl_x_mode != 0)
    {
	/* We're already in CTRL-X mode, do we stay in it? */
	if (!vim_is_ctrl_x_key(c))
	{
	    if (ctrl_x_mode == CTRL_X_SCROLL)
		ctrl_x_mode = 0;
	    else
		ctrl_x_mode = CTRL_X_FINISHED;
	    edit_submode = NULL;
	}
	showmode();
    }

    if (started_completion || ctrl_x_mode == CTRL_X_FINISHED)
    {
	/* Show error message from attempted keyword completion (probably
	 * 'Pattern not found') until another key is hit, then go back to
	 * showing what mode we are in.
	 */
	showmode();
	if ((ctrl_x_mode == 0 && c != Ctrl_N && c != Ctrl_P && c != Ctrl_R)
		|| ctrl_x_mode == CTRL_X_FINISHED)
	{
	    /* Get here when we have finished typing a sequence of ^N and
	     * ^P or other completion characters in CTRL-X mode.  Free up
	     * memory that was used, and make sure we can redo the insert.
	     */
	    if (curr_match != NULL)
	    {
		/*
		 * If any of the original typed text has been changed,
		 * eg when ignorecase is set, we must add back-spaces to
		 * the redo buffer.  We add as few as necessary to delete
		 * just the part of the original text that has changed.
		 */
		ptr = curr_match->str;
		tmp_ptr = original_text;
		while (*tmp_ptr && *tmp_ptr == *ptr)
		{
		    ++tmp_ptr;
		    ++ptr;
		}
		for (temp = 0; tmp_ptr[temp]; ++temp)
		    AppendCharToRedobuff(K_BS);
		AppendToRedobuffLit(ptr);
	    }

#ifdef FEAT_CINDENT
	    want_cindent = (can_cindent && cindent_on());
#endif
	    /*
	     * When completing whole lines: fix indent for 'cindent'.
	     * Otherwise, break line if it's too long.
	     */
	    if (continue_mode == CTRL_X_WHOLE_LINE)
	    {
#ifdef FEAT_CINDENT
		/* re-indent the current line */
		if (want_cindent)
		{
		    do_c_expr_indent();
		    want_cindent = FALSE;	/* don't do it again */
		}
#endif
	    }
	    else
	    {
		/* put the cursor on the last char, for 'tw' formatting */
		curwin->w_cursor.col--;
		if (stop_arrow() == OK)
		    insertchar(NUL, 0, -1);
		curwin->w_cursor.col++;
	    }

	    auto_format(FALSE, TRUE);

	    ins_compl_free();
	    started_completion = FALSE;
	    completion_matches = 0;
	    msg_clr_cmdline();		/* necessary for "noshowmode" */
	    ctrl_x_mode = 0;
	    p_sm = save_sm;
	    if (edit_submode != NULL)
	    {
		edit_submode = NULL;
		showmode();
	    }

#ifdef FEAT_CINDENT
	    /*
	     * Indent now if a key was typed that is in 'cinkeys'.
	     */
	    if (want_cindent && in_cinkeys(KEY_COMPLETE, ' ', inindent(0)))
		do_c_expr_indent();
#endif
	}
    }

    /* reset continue_* if we left expansion-mode, if we stay they'll be
     * (re)set properly in ins_complete() */
    if (!vim_is_ctrl_x_key(c))
    {
	continue_status = 0;
	continue_mode = 0;
    }
}

/*
 * Loops through the list of windows, loaded-buffers or non-loaded-buffers
 * (depending on flag) starting from buf and looking for a non-scanned
 * buffer (other than curbuf).	curbuf is special, if it is called with
 * buf=curbuf then it has to be the first call for a given flag/expansion.
 *
 * Returns the buffer to scan, if any, otherwise returns curbuf -- Acevedo
 */
    static buf_T *
ins_compl_next_buf(buf, flag)
    buf_T	*buf;
    int		flag;
{
#ifdef FEAT_WINDOWS
    static win_T *wp;
#endif

    if (flag == 'w')		/* just windows */
    {
#ifdef FEAT_WINDOWS
	if (buf == curbuf)	/* first call for this flag/expansion */
	    wp = curwin;
	while ((wp = wp->w_next != NULL ? wp->w_next : firstwin) != curwin
		&& wp->w_buffer->b_scanned)
	    ;
	buf = wp->w_buffer;
#else
	buf = curbuf;
#endif
    }
    else
	/* 'b' (just loaded buffers), 'u' (just non-loaded buffers) or 'U'
	 * (unlisted buffers)
	 * When completing whole lines skip unloaded buffers. */
	while ((buf = buf->b_next != NULL ? buf->b_next : firstbuf) != curbuf
		&& ((flag == 'U'
			? buf->b_p_bl
			: (!buf->b_p_bl
			    || (buf->b_ml.ml_mfp == NULL) != (flag == 'u')))
		    || buf->b_scanned
		    || (buf->b_ml.ml_mfp == NULL
			&& ctrl_x_mode == CTRL_X_WHOLE_LINE)))
	    ;
    return buf;
}

/*
 * Get the next expansion(s) for the text starting at the initial curbuf
 * position "ini" and in the direction dir.
 * Return the total of matches or -1 if still unknown -- Acevedo
 */
    static int
ins_compl_get_exp(ini, dir)
    pos_T	*ini;
    int		dir;
{
    static pos_T	first_match_pos;
    static pos_T	last_match_pos;
    static char_u	*e_cpt = (char_u *)"";	/* curr. entry in 'complete' */
    static int		found_all = FALSE;	/* Found all matches. */
    static buf_T	*ins_buf = NULL;

    pos_T		*pos;
    char_u		**matches;
    int			save_p_scs;
    int			save_p_ws;
    int			save_p_ic;
    int			i;
    int			num_matches;
    int			len;
    int			found_new_match;
    int			type = ctrl_x_mode;
    char_u		*ptr;
    char_u		*tmp_ptr;
    char_u		*dict = NULL;
    int			dict_f = 0;
    struct Completion	*old_match;

    if (!started_completion)
    {
	for (ins_buf = firstbuf; ins_buf != NULL; ins_buf = ins_buf->b_next)
	    ins_buf->b_scanned = 0;
	found_all = FALSE;
	ins_buf = curbuf;
	e_cpt = continue_status & CONT_LOCAL ? (char_u *)"." : curbuf->b_p_cpt;
	last_match_pos = first_match_pos = *ini;
    }

    old_match = curr_match;		/* remember the last current match */
    pos = (dir == FORWARD) ? &last_match_pos : &first_match_pos;
    /* For ^N/^P loop over all the flags/windows/buffers in 'complete' */
    for (;;)
    {
	found_new_match = FAIL;

	/* For ^N/^P pick a new entry from e_cpt if started_completion is off,
	 * or if found_all says this entry is done.  For ^X^L only use the
	 * entries from 'complete' that look in loaded buffers. */
	if ((ctrl_x_mode == 0 || ctrl_x_mode == CTRL_X_WHOLE_LINE)
		&& (!started_completion || found_all))
	{
	    found_all = FALSE;
	    while (*e_cpt == ',' || *e_cpt == ' ')
		e_cpt++;
	    if (*e_cpt == '.' && !curbuf->b_scanned)
	    {
		ins_buf = curbuf;
		first_match_pos = *ini;
		/* So that ^N can match word immediately after cursor */
		if (ctrl_x_mode == 0)
		    dec(&first_match_pos);
		last_match_pos = first_match_pos;
		type = 0;
	    }
	    else if (vim_strchr((char_u *)"buwU", *e_cpt) != NULL
		 && (ins_buf = ins_compl_next_buf(ins_buf, *e_cpt)) != curbuf)
	    {
		/* Scan a buffer, but not the current one. */
		if (ins_buf->b_ml.ml_mfp != NULL)   /* loaded buffer */
		{
		    started_completion = TRUE;
		    first_match_pos.col = last_match_pos.col = 0;
		    first_match_pos.lnum = ins_buf->b_ml.ml_line_count + 1;
		    last_match_pos.lnum = 0;
		    type = 0;
		}
		else	/* unloaded buffer, scan like dictionary */
		{
		    found_all = TRUE;
		    if (ins_buf->b_fname == NULL)
			continue;
		    type = CTRL_X_DICTIONARY;
		    dict = ins_buf->b_fname;
		    dict_f = DICT_EXACT;
		}
		sprintf((char *)IObuff, _("Scanning: %s"),
			ins_buf->b_fname == NULL
			    ? buf_spname(ins_buf)
			    : ins_buf->b_sfname == NULL
				? (char *)ins_buf->b_fname
				: (char *)ins_buf->b_sfname);
		msg_trunc_attr(IObuff, TRUE, hl_attr(HLF_R));
	    }
	    else if (*e_cpt == NUL)
		break;
	    else
	    {
		if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
		    type = -1;
		else if (*e_cpt == 'k' || *e_cpt == 's')
		{
		    if (*e_cpt == 'k')
			type = CTRL_X_DICTIONARY;
		    else
			type = CTRL_X_THESAURUS;
		    if (*++e_cpt != ',' && *e_cpt != NUL)
		    {
			dict = e_cpt;
			dict_f = DICT_FIRST;
		    }
		}
#ifdef FEAT_FIND_ID
		else if (*e_cpt == 'i')
		    type = CTRL_X_PATH_PATTERNS;
		else if (*e_cpt == 'd')
		    type = CTRL_X_PATH_DEFINES;
#endif
		else if (*e_cpt == ']' || *e_cpt == 't')
		{
		    type = CTRL_X_TAGS;
		    sprintf((char*)IObuff, _("Scanning tags."));
		    msg_trunc_attr(IObuff, TRUE, hl_attr(HLF_R));
		}
		else
		    type = -1;

		/* in any case e_cpt is advanced to the next entry */
		(void)copy_option_part(&e_cpt, IObuff, IOSIZE, ",");

		found_all = TRUE;
		if (type == -1)
		    continue;
	    }
	}

	switch (type)
	{
	case -1:
	    break;
#ifdef FEAT_FIND_ID
	case CTRL_X_PATH_PATTERNS:
	case CTRL_X_PATH_DEFINES:
	    find_pattern_in_path(complete_pat, dir,
				 (int)STRLEN(complete_pat), FALSE, FALSE,
				 (type == CTRL_X_PATH_DEFINES
				  && !(continue_status & CONT_SOL))
				 ? FIND_DEFINE : FIND_ANY, 1L, ACTION_EXPAND,
				 (linenr_T)1, (linenr_T)MAXLNUM);
	    break;
#endif

	case CTRL_X_DICTIONARY:
	case CTRL_X_THESAURUS:
	    ins_compl_dictionaries(
		    dict ? dict
			 : (type == CTRL_X_THESAURUS
			     ? (*curbuf->b_p_tsr == NUL
				 ? p_tsr
				 : curbuf->b_p_tsr)
			     : (*curbuf->b_p_dict == NUL
				 ? p_dict
				 : curbuf->b_p_dict)),
			    complete_pat, dir,
				 dict ? dict_f : 0, type == CTRL_X_THESAURUS);
	    dict = NULL;
	    break;

	case CTRL_X_TAGS:
	    /* set p_ic according to p_ic, p_scs and pat for find_tags(). */
	    save_p_ic = p_ic;
	    p_ic = ignorecase(complete_pat);

	    /* Find up to TAG_MANY matches.  Avoids that an enourmous number
	     * of matches is found when complete_pat is empty */
	    if (find_tags(complete_pat, &num_matches, &matches,
		    TAG_REGEXP | TAG_NAMES | TAG_NOIC |
		    TAG_INS_COMP | (ctrl_x_mode ? TAG_VERBOSE : 0),
		    TAG_MANY, curbuf->b_ffname) == OK && num_matches > 0)
	    {
		ins_compl_add_matches(num_matches, matches, dir);
	    }
	    p_ic = save_p_ic;
	    break;

	case CTRL_X_FILES:
	    if (expand_wildcards(1, &complete_pat, &num_matches, &matches,
				  EW_FILE|EW_DIR|EW_ADDSLASH|EW_SILENT) == OK)
	    {

		/* May change home directory back to "~". */
		tilde_replace(complete_pat, num_matches, matches);
		ins_compl_add_matches(num_matches, matches, dir);
	    }
	    break;

	case CTRL_X_CMDLINE:
	    if (expand_cmdline(&complete_xp, complete_pat,
			(int)STRLEN(complete_pat),
					 &num_matches, &matches) == EXPAND_OK)
		ins_compl_add_matches(num_matches, matches, dir);
	    break;

	default:	/* normal ^P/^N and ^X^L */
	    /*
	     * If 'infercase' is set, don't use 'smartcase' here
	     */
	    save_p_scs = p_scs;
	    if (ins_buf->b_p_inf)
		p_scs = FALSE;
	    /*	buffers other than curbuf are scanned from the beginning or the
	     *	end but never from the middle, thus setting nowrapscan in this
	     *	buffers is a good idea, on the other hand, we always set
	     *	wrapscan for curbuf to avoid missing matches -- Acevedo,Webb */
	    save_p_ws = p_ws;
	    if (ins_buf != curbuf)
		p_ws = FALSE;
	    else if (*e_cpt == '.')
		p_ws = TRUE;
	    for (;;)
	    {
		int	reuse = 0;

		/* ctrl_x_mode == CTRL_X_WHOLE_LINE || word-wise search that has
		 * added a word that was at the beginning of the line */
		if (	ctrl_x_mode == CTRL_X_WHOLE_LINE
			|| (continue_status & CONT_SOL))
		    found_new_match = search_for_exact_line(ins_buf, pos,
							    dir, complete_pat);
		else
		    found_new_match = searchit(NULL, ins_buf, pos, dir,
				 complete_pat, 1L, SEARCH_KEEP + SEARCH_NFMSG,
								     RE_LAST);
		if (!started_completion)
		{
		    /* set started_completion even on fail */
		    started_completion = TRUE;
		    first_match_pos = *pos;
		    last_match_pos = *pos;
		}
		else if (first_match_pos.lnum == last_match_pos.lnum
			 && first_match_pos.col == last_match_pos.col)
		    found_new_match = FAIL;
		if (found_new_match == FAIL)
		{
		    if (ins_buf == curbuf)
			found_all = TRUE;
		    break;
		}

		/* when ADDING, the text before the cursor matches, skip it */
		if (	(continue_status & CONT_ADDING) && ins_buf == curbuf
			&& ini->lnum == pos->lnum
			&& ini->col  == pos->col)
		    continue;
		ptr = ml_get_buf(ins_buf, pos->lnum, FALSE) + pos->col;
		if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
		{
		    if (continue_status & CONT_ADDING)
		    {
			if (pos->lnum >= ins_buf->b_ml.ml_line_count)
			    continue;
			ptr = ml_get_buf(ins_buf, pos->lnum + 1, FALSE);
			if (!p_paste)
			    ptr = skipwhite(ptr);
		    }
		    len = (int)STRLEN(ptr);
		}
		else
		{
		    tmp_ptr = ptr;
		    if (continue_status & CONT_ADDING)
		    {
			tmp_ptr += completion_length;
			/* Skip if already inside a word. */
			if (vim_iswordp(tmp_ptr))
			    continue;
			/* Find start of next word. */
			tmp_ptr = find_word_start(tmp_ptr);
		    }
		    /* Find end of this word. */
		    tmp_ptr = find_word_end(tmp_ptr);
		    len = (int)(tmp_ptr - ptr);

		    if ((continue_status & CONT_ADDING)
						  && len == completion_length)
		    {
			if (pos->lnum < ins_buf->b_ml.ml_line_count)
			{
			    /* Try next line, if any. the new word will be
			     * "join" as if the normal command "J" was used.
			     * IOSIZE is always greater than
			     * completion_length, so the next STRNCPY always
			     * works -- Acevedo */
			    STRNCPY(IObuff, ptr, len);
			    ptr = ml_get_buf(ins_buf, pos->lnum + 1, FALSE);
			    tmp_ptr = ptr = skipwhite(ptr);
			    /* Find start of next word. */
			    tmp_ptr = find_word_start(tmp_ptr);
			    /* Find end of next word. */
			    tmp_ptr = find_word_end(tmp_ptr);
			    if (tmp_ptr > ptr)
			    {
				if (*ptr != ')' && IObuff[len-1] != TAB)
				{
				    if (IObuff[len-1] != ' ')
					IObuff[len++] = ' ';
				    /* IObuf =~ "\k.* ", thus len >= 2 */
				    if (p_js
					&& (IObuff[len-2] == '.'
					    || (vim_strchr(p_cpo, CPO_JOINSP)
								       == NULL
						&& (IObuff[len-2] == '?'
						    || IObuff[len-2] == '!'))))
					IObuff[len++] = ' ';
				}
				/* copy as much as posible of the new word */
				if (tmp_ptr - ptr >= IOSIZE - len)
				    tmp_ptr = ptr + IOSIZE - len - 1;
				STRNCPY(IObuff + len, ptr, tmp_ptr - ptr);
				len += (int)(tmp_ptr - ptr);
				reuse |= CONT_S_IPOS;
			    }
			    IObuff[len] = NUL;
			    ptr = IObuff;
			}
			if (len == completion_length)
			    continue;
		    }
		}
		if (ins_compl_add_infercase(ptr, len,
			    ins_buf == curbuf ?  NULL : ins_buf->b_sfname,
							  dir, reuse) != FAIL)
		{
		    found_new_match = OK;
		    break;
		}
	    }
	    p_scs = save_p_scs;
	    p_ws = save_p_ws;
	}
	/* check if curr_match has changed, (e.g. other type of expansion
	 * added somenthing) */
	if (curr_match != old_match)
	    found_new_match = OK;

	/* break the loop for specialized modes (use 'complete' just for the
	 * generic ctrl_x_mode == 0) or when we've found a new match */
	if ((ctrl_x_mode != 0 && ctrl_x_mode != CTRL_X_WHOLE_LINE)
		|| found_new_match != FAIL)
	    break;

	/* Mark a buffer scanned when it has been scanned completely */
	if (type == 0 || type == CTRL_X_PATH_PATTERNS)
	    ins_buf->b_scanned = TRUE;

	started_completion = FALSE;
    }
    started_completion = TRUE;

    if ((ctrl_x_mode == 0 || ctrl_x_mode == CTRL_X_WHOLE_LINE)
	    && *e_cpt == NUL)		/* Got to end of 'complete' */
	found_new_match = FAIL;

    i = -1;		/* total of matches, unknown */
    if (found_new_match == FAIL
	    || (ctrl_x_mode != 0 && ctrl_x_mode != CTRL_X_WHOLE_LINE))
	i = ins_compl_make_cyclic();

    /* If several matches were added (FORWARD) or the search failed and has
     * just been made cyclic then we have to move curr_match to the next or
     * previous entry (if any) -- Acevedo */
    curr_match = dir == FORWARD ? old_match->next : old_match->prev;
    if (curr_match == NULL)
	curr_match = old_match;
    return i;
}

/* Delete the old text being completed. */
    static void
ins_compl_delete()
{
    int	    i;

    /*
     * In insert mode: Delete the typed part.
     * In replace mode: Put the old characters back, if any.
     */
    i = complete_col + (continue_status & CONT_ADDING ? completion_length : 0);
    backspace_until_column(i);
    changed_cline_bef_curs();
}

/* Insert the new text being completed. */
    static void
ins_compl_insert()
{
    ins_bytes(shown_match->str + curwin->w_cursor.col - complete_col);
}

/*
 * Fill in the next completion in the current direction.
 * If allow_get_expansion is TRUE, then we may call ins_compl_get_exp() to get
 * more completions.  If it is FALSE, then we just do nothing when there are
 * no more completions in a given direction.  The latter case is used when we
 * are still in the middle of finding completions, to allow browsing through
 * the ones found so far.
 * Return the total number of matches, or -1 if still unknown -- webb.
 *
 * curr_match is currently being used by ins_compl_get_exp(), so we use
 * shown_match here.
 *
 * Note that this function may be called recursively once only.  First with
 * allow_get_expansion TRUE, which calls ins_compl_get_exp(), which in turn
 * calls this function with allow_get_expansion FALSE.
 */
    static int
ins_compl_next(allow_get_expansion)
    int	    allow_get_expansion;
{
    int	    num_matches = -1;
    int	    i;

    if (allow_get_expansion)
    {
	/* Delete old text to be replaced */
	ins_compl_delete();
    }
    completion_pending = FALSE;
    if (shown_direction == FORWARD && shown_match->next != NULL)
	shown_match = shown_match->next;
    else if (shown_direction == BACKWARD && shown_match->prev != NULL)
	shown_match = shown_match->prev;
    else
    {
	completion_pending = TRUE;
	if (allow_get_expansion)
	{
	    num_matches = ins_compl_get_exp(&initial_pos, complete_direction);
	    if (completion_pending)
	    {
		if (complete_direction == shown_direction)
		    shown_match = curr_match;
	    }
	}
	else
	    return -1;
    }

    /* Insert the text of the new completion */
    ins_compl_insert();

    if (!allow_get_expansion)
    {
	/* Display the current match. */
	update_screen(0);

	/* Delete old text to be replaced, since we're still searching and
	 * don't want to match ourselves!  */
	ins_compl_delete();
    }

    /*
     * Show the file name for the match (if any)
     * Truncate the file name to avoid a wait for return.
     */
    if (shown_match->fname != NULL)
    {
	STRCPY(IObuff, "match in file ");
	i = (vim_strsize(shown_match->fname) + 16) - sc_col;
	if (i <= 0)
	    i = 0;
	else
	    STRCAT(IObuff, "<");
	STRCAT(IObuff, shown_match->fname + i);
	msg(IObuff);
	redraw_cmdline = FALSE;	    /* don't overwrite! */
    }

    return num_matches;
}

/*
 * Call this while finding completions, to check whether the user has hit a key
 * that should change the currently displayed completion, or exit completion
 * mode.  Also, when completion_pending is TRUE, show a completion as soon as
 * possible. -- webb
 */
    void
ins_compl_check_keys()
{
    static int	count = 0;

    int	    c;

    /* Don't check when reading keys from a script.  That would break the test
     * scripts */
    if (using_script())
	return;

    /* Only do this at regular intervals */
    if (++count < CHECK_KEYS_TIME)
	return;
    count = 0;

    ++no_mapping;
    c = vpeekc_any();
    --no_mapping;
    if (c != NUL)
    {
	if (vim_is_ctrl_x_key(c) && c != Ctrl_X && c != Ctrl_R)
	{
	    c = safe_vgetc();	/* Eat the character */
	    if (c == Ctrl_P || c == Ctrl_L)
		shown_direction = BACKWARD;
	    else
		shown_direction = FORWARD;
	    (void)ins_compl_next(FALSE);
	}
	else if (c != Ctrl_R)
	    completion_interrupted = TRUE;
    }
    if (completion_pending && !got_int)
	(void)ins_compl_next(FALSE);
}

/*
 * Do Insert mode completion.
 * Called when character "c" was typed, which has a meaning for completion.
 * Returns OK if completion was done, FAIL if something failed (out of mem).
 */
    static int
ins_complete(c)
    int		    c;
{
    char_u	    *line;
    char_u	    *tmp_ptr = NULL;		/* init for gcc */
    int		    temp = 0;

    if (c == Ctrl_P || c == Ctrl_L)
	complete_direction = BACKWARD;
    else
	complete_direction = FORWARD;
    if (!started_completion)
    {
	/* First time we hit ^N or ^P (in a row, I mean) */

	/* Turn off 'sm' so we don't show matches with ^X^L */
	save_sm = p_sm;
	p_sm = FALSE;

	did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
	did_si = FALSE;
	can_si = FALSE;
	can_si_back = FALSE;
#endif
	if (stop_arrow() == FAIL)
	    return FAIL;

	line = ml_get(curwin->w_cursor.lnum);
	complete_col = curwin->w_cursor.col;

	/* if this same ctrl_x_mode has been interrupted use the text from
	 * "initial_pos" to the cursor as a pattern to add a new word instead
	 * of expand the one before the cursor, in word-wise if "initial_pos"
	 * is not in the same line as the cursor then fix it (the line has
	 * been split because it was longer than 'tw').  if SOL is set then
	 * skip the previous pattern, a word at the beginning of the line has
	 * been inserted, we'll look for that  -- Acevedo. */
	if ((continue_status & CONT_INTRPT) && continue_mode == ctrl_x_mode)
	{
	    /*
	     * it is a continued search
	     */
	    continue_status &= ~CONT_INTRPT;	/* remove INTRPT */
	    if (ctrl_x_mode == 0 || ctrl_x_mode == CTRL_X_PATH_PATTERNS
					|| ctrl_x_mode == CTRL_X_PATH_DEFINES)
	    {
		if (initial_pos.lnum != curwin->w_cursor.lnum)
		{
		    /* line (probably) wrapped, set initial_pos to the first
		     * non_blank in the line, if it is not a wordchar include
		     * it to get a better pattern, but then we don't want the
		     * "\\<" prefix, check it bellow */
		    tmp_ptr = skipwhite(line);
		    initial_pos.col = (colnr_T) (tmp_ptr - line);
		    initial_pos.lnum = curwin->w_cursor.lnum;
		    continue_status &= ~CONT_SOL;    /* clear SOL if present */
		}
		else
		{
		    /* S_IPOS was set when we inserted a word that was at the
		     * beginning of the line, which means that we'll go to SOL
		     * mode but first we need to redefine initial_pos */
		    if (continue_status & CONT_S_IPOS)
		    {
			continue_status |= CONT_SOL;
			initial_pos.col = (colnr_T) (skipwhite(line + completion_length +
						    initial_pos.col) - line);
		    }
		    tmp_ptr = line + initial_pos.col;
		}
		temp = curwin->w_cursor.col - (int)(tmp_ptr - line);
		/* IObuf is used to add a "word from the next line" would we
		 * have enough space?  just being paranoic */
#define	MIN_SPACE 75
		if (temp > (IOSIZE - MIN_SPACE))
		{
		    continue_status &= ~CONT_SOL;
		    temp = (IOSIZE - MIN_SPACE);
		    tmp_ptr = line + curwin->w_cursor.col - temp;
		}
		continue_status |= CONT_ADDING | CONT_N_ADDS;
		if (temp < 1)
		    continue_status &= CONT_LOCAL;
	    }
	    else if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
		continue_status = CONT_ADDING | CONT_N_ADDS;
	    else
		continue_status = 0;
	}
	else
	    continue_status &= CONT_LOCAL;

	if (!(continue_status & CONT_ADDING))	/* normal expansion */
	{
	    continue_mode = ctrl_x_mode;
	    if (ctrl_x_mode != 0)	/* Remove LOCAL if ctrl_x_mode != 0 */
		continue_status = 0;
	    continue_status |= CONT_N_ADDS;
	    initial_pos = curwin->w_cursor;
	    temp = (int)complete_col;
	    tmp_ptr = line;
	}

	/* Work out completion pattern and original text -- webb */
	if (ctrl_x_mode == 0 || (ctrl_x_mode & CTRL_X_WANT_IDENT))
	{
	    if (       (continue_status & CONT_SOL)
		    || ctrl_x_mode == CTRL_X_PATH_DEFINES)
	    {
		if (!(continue_status & CONT_ADDING))
		{
		    while (--temp >= 0 && vim_isIDc(line[temp]))
			;
		    tmp_ptr += ++temp;
		    temp = complete_col - temp;
		}
		if (p_ic)
		    complete_pat = str_foldcase(tmp_ptr, temp);
		else
		    complete_pat = vim_strnsave(tmp_ptr, temp);
		if (complete_pat == NULL)
		    return FAIL;
	    }
	    else if (continue_status & CONT_ADDING)
	    {
		char_u	    *prefix = (char_u *)"\\<";

		/* we need 3 extra chars, 1 for the NUL and
		 * 2 >= strlen(prefix)	-- Acevedo */
		complete_pat = alloc(quote_meta(NULL, tmp_ptr, temp) + 3);
		if (complete_pat == NULL)
		    return FAIL;
		if (!vim_iswordp(tmp_ptr)
			|| (tmp_ptr > line
			    && (
#ifdef FEAT_MBYTE
				vim_iswordp(mb_prevptr(line, tmp_ptr))
#else
				vim_iswordc(*(tmp_ptr - 1))
#endif
				)))
		    prefix = (char_u *)"";
		STRCPY((char *)complete_pat, prefix);
		(void)quote_meta(complete_pat + STRLEN(prefix), tmp_ptr, temp);
	    }
	    else if (
#ifdef FEAT_MBYTE
		    --temp < 0 || !vim_iswordp(mb_prevptr(line,
							     line + temp + 1))
#else
		    --temp < 0 || !vim_iswordc(line[temp])
#endif
		    )
	    {
		/* Match any word of at least two chars */
		complete_pat = vim_strsave((char_u *)"\\<\\k\\k");
		if (complete_pat == NULL)
		    return FAIL;
		tmp_ptr += complete_col;
		temp = 0;
	    }
	    else
	    {
#ifdef FEAT_MBYTE
		/* Search the point of change class of multibyte character
		 * or not a word single byte character backward.  */
		if (has_mbyte)
		{
		    int base_class;
		    int head_off;

		    temp -= (*mb_head_off)(line, line + temp);
		    base_class = mb_get_class(line + temp);
		    while (--temp >= 0)
		    {
			head_off = (*mb_head_off)(line, line + temp);
			if (base_class != mb_get_class(line + temp - head_off))
			    break;
			temp -= head_off;
		    }
		}
		else
#endif
		    while (--temp >= 0 && vim_iswordc(line[temp]))
			;
		tmp_ptr += ++temp;
		if ((temp = (int)complete_col - temp) == 1)
		{
		    /* Only match word with at least two chars -- webb
		     * there's no need to call quote_meta,
		     * alloc(7) is enough  -- Acevedo
		     */
		    complete_pat = alloc(7);
		    if (complete_pat == NULL)
			return FAIL;
		    STRCPY((char *)complete_pat, "\\<");
		    (void)quote_meta(complete_pat + 2, tmp_ptr, 1);
		    STRCAT((char *)complete_pat, "\\k");
		}
		else
		{
		    complete_pat = alloc(quote_meta(NULL, tmp_ptr, temp) + 3);
		    if (complete_pat == NULL)
			return FAIL;
		    STRCPY((char *)complete_pat, "\\<");
		    (void)quote_meta(complete_pat + 2, tmp_ptr, temp);
		}
	    }
	}
	else if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
	{
	    tmp_ptr = skipwhite(line);
	    temp = (int)complete_col - (int)(tmp_ptr - line);
	    if (temp < 0)	/* cursor in indent: empty pattern */
		temp = 0;
	    if (p_ic)
		complete_pat = str_foldcase(tmp_ptr, temp);
	    else
		complete_pat = vim_strnsave(tmp_ptr, temp);
	    if (complete_pat == NULL)
		return FAIL;
	}
	else if (ctrl_x_mode == CTRL_X_FILES)
	{
	    while (--temp >= 0 && vim_isfilec(line[temp]))
		;
	    tmp_ptr += ++temp;
	    temp = (int)complete_col - temp;
	    complete_pat = addstar(tmp_ptr, temp, EXPAND_FILES);
	    if (complete_pat == NULL)
		return FAIL;
	}
	else if (ctrl_x_mode == CTRL_X_CMDLINE)
	{
	    complete_pat = vim_strnsave(line, complete_col);
	    if (complete_pat == NULL)
		return FAIL;
	    set_cmd_context(&complete_xp, complete_pat,
				     (int)STRLEN(complete_pat), complete_col);
	    if (complete_xp.xp_context == EXPAND_UNSUCCESSFUL
		    || complete_xp.xp_context == EXPAND_NOTHING)
		return FAIL;
	    temp = (int)(complete_xp.xp_pattern - complete_pat);
	    tmp_ptr = line + temp;
	    temp = complete_col - temp;
	}
	complete_col = (colnr_T) (tmp_ptr - line);

	if (continue_status & CONT_ADDING)
	{
	    edit_submode_pre = (char_u *)_(" Adding");
	    if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
	    {
		/* Insert a new line, keep indentation but ignore 'comments' */
#ifdef FEAT_COMMENTS
		char_u *old = curbuf->b_p_com;

		curbuf->b_p_com = (char_u *)"";
#endif
		initial_pos.lnum = curwin->w_cursor.lnum;
		initial_pos.col = complete_col;
		ins_eol('\r');
#ifdef FEAT_COMMENTS
		curbuf->b_p_com = old;
#endif
		tmp_ptr = (char_u *)"";
		temp = 0;
		complete_col = curwin->w_cursor.col;
	    }
	}
	else
	{
	    edit_submode_pre = NULL;
	    initial_pos.col = complete_col;
	}

	if (continue_status & CONT_LOCAL)
	    edit_submode = (char_u *)_(ctrl_x_msgs[2]);
	else
	    edit_submode = (char_u *)_(CTRL_X_MSG(ctrl_x_mode));

	completion_length = temp;

	/* Always add completion for the original text.  Note that
	 * "original_text" itself (not a copy) is added, it will be freed when
	 * the list of matches is freed. */
	if ((original_text = vim_strnsave(tmp_ptr, temp)) == NULL
	    || ins_compl_add(original_text, -1, NULL, 0, ORIGINAL_TEXT) != OK)
	{
	    vim_free(complete_pat);
	    complete_pat = NULL;
	    vim_free(original_text);
	    return FAIL;
	}

	/* showmode might reset the internal line pointers, so it must
	 * be called before line = ml_get(), or when this address is no
	 * longer needed.  -- Acevedo.
	 */
	edit_submode_extra = (char_u *)_("-- Searching...");
	edit_submode_highl = HLF_COUNT;
	showmode();
	edit_submode_extra = NULL;
	out_flush();
    }

    shown_match = curr_match;
    shown_direction = complete_direction;

    /*
     * Find next match.
     */
    temp = ins_compl_next(TRUE);

    if (temp > 1)	/* all matches have been found */
	completion_matches = temp;
    curr_match = shown_match;
    complete_direction = shown_direction;
    completion_interrupted = FALSE;

    /* eat the ESC to avoid leaving insert mode */
    if (got_int && !global_busy)
    {
	(void)vgetc();
	got_int = FALSE;
    }

    /* we found no match if the list has only the original_text-entry */
    if (first_match == first_match->next)
    {
	edit_submode_extra = (continue_status & CONT_ADDING)
			&& completion_length > 1
			     ? (char_u *)_(e_hitend) : (char_u *)_(e_patnotf);
	edit_submode_highl = HLF_E;
	/* remove N_ADDS flag, so next ^X<> won't try to go to ADDING mode,
	 * because we couldn't expand anything at first place, but if we used
	 * ^P, ^N, ^X^I or ^X^D we might want to add-expand a single-char-word
	 * (such as M in M'exico) if not tried already.  -- Acevedo */
	if (	   completion_length > 1
		|| (continue_status & CONT_ADDING)
		|| (ctrl_x_mode != 0
		    && ctrl_x_mode != CTRL_X_PATH_PATTERNS
		    && ctrl_x_mode != CTRL_X_PATH_DEFINES))
	    continue_status &= ~CONT_N_ADDS;
    }

    if (curr_match->original & CONT_S_IPOS)
	continue_status |= CONT_S_IPOS;
    else
	continue_status &= ~CONT_S_IPOS;

    if (edit_submode_extra == NULL)
    {
	if (curr_match->original & ORIGINAL_TEXT)
	{
	    edit_submode_extra = (char_u *)_("Back at original");
	    edit_submode_highl = HLF_W;
	}
	else if (continue_status & CONT_S_IPOS)
	{
	    edit_submode_extra = (char_u *)_("Word from other line");
	    edit_submode_highl = HLF_COUNT;
	}
	else if (curr_match->next == curr_match->prev)
	{
	    edit_submode_extra = (char_u *)_("The only match");
	    edit_submode_highl = HLF_COUNT;
	}
	else
	{
	    /* Update completion sequence number when needed. */
	    if (curr_match->number == -1)
	    {
		int		    number = 0;
		struct Completion   *match;

		if (complete_direction == FORWARD)
		{
		    /* search backwards for the first valid (!= -1) number.
		     * This should normally succeed already at the first loop
		     * cycle, so it's fast! */
		    for (match = curr_match->prev; match != NULL
				 && match != first_match; match = match->prev)
			if (match->number != -1)
			{
			    number = match->number;
			    break;
			}
		    if (match != NULL)
			/* go up and assign all numbers which are not assigned
			 * yet */
			for (match = match->next; match
				  && match->number == -1; match = match->next)
			    match->number = ++number;
		}
		else /* BACKWARD */
		{
		    /* search forwards (upwards) for the first valid (!= -1)
		     * number.  This should normally succeed already at the
		     * first loop cycle, so it's fast! */
		    for (match = curr_match->next; match != NULL
				 && match != first_match; match = match->next)
			if (match->number != -1)
			{
			    number = match->number;
			    break;
			}
		    if (match != NULL)
			/* go down and assign all numbers which are not
			 * assigned yet */
			for (match = match->prev; match
				  && match->number == -1; match = match->prev)
			    match->number = ++number;
		}
	    }

	    /* The match should always have a sequnce number now, this is just
	     * a safety check. */
	    if (curr_match->number != -1)
	    {
		/* Space for 10 text chars. + 2x10-digit no.s */
		static char_u match_ref[31];

		if (completion_matches > 0)
		    sprintf((char *)IObuff, _("match %d of %d"),
				      curr_match->number, completion_matches);
		else
		    sprintf((char *)IObuff, _("match %d"), curr_match->number);
		STRNCPY(match_ref, IObuff, 30 );
		match_ref[30] = '\0';
		edit_submode_extra = match_ref;
		edit_submode_highl = HLF_R;
		if (dollar_vcol)
		    curs_columns(FALSE);
	    }
	}
    }

    /* Show a message about what (completion) mode we're in. */
    showmode();
    if (edit_submode_extra != NULL)
    {
	if (!p_smd)
	    msg_attr(edit_submode_extra,
		    edit_submode_highl < HLF_COUNT
		    ? hl_attr(edit_submode_highl) : 0);
    }
    else
	msg_clr_cmdline();	/* necessary for "noshowmode" */

    return OK;
}

/*
 * Looks in the first "len" chars. of "src" for search-metachars.
 * If dest is not NULL the chars. are copied there quoting (with
 * a backslash) the metachars, and dest would be NUL terminated.
 * Returns the length (needed) of dest
 */
    static int
quote_meta(dest, src, len)
    char_u	*dest;
    char_u	*src;
    int		len;
{
    int	m;

    for (m = len; --len >= 0; src++)
    {
	switch (*src)
	{
	    case '.':
	    case '*':
	    case '[':
		if (ctrl_x_mode == CTRL_X_DICTIONARY
					   || ctrl_x_mode == CTRL_X_THESAURUS)
		    break;
	    case '~':
		if (!p_magic)	/* quote these only if magic is set */
		    break;
	    case '\\':
		if (ctrl_x_mode == CTRL_X_DICTIONARY
					   || ctrl_x_mode == CTRL_X_THESAURUS)
		    break;
	    case '^':		/* currently it's not needed. */
	    case '$':
		m++;
		if (dest != NULL)
		    *dest++ = '\\';
		break;
	}
	if (dest != NULL)
	    *dest++ = *src;
#ifdef FEAT_MBYTE
	/* Copy remaining bytes of a multibyte character. */
	if (has_mbyte)
	{
	    int i, mb_len;

	    mb_len = (*mb_ptr2len_check)(src) - 1;
	    if (mb_len > 0 && len >= mb_len)
		for (i = 0; i < mb_len; ++i)
		{
		    --len;
		    ++src;
		    if (dest != NULL)
			*dest++ = *src;
		}
	}
#endif
    }
    if (dest != NULL)
	*dest = NUL;

    return m;
}
#endif /* FEAT_INS_EXPAND */

/*
 * Next character is interpreted literally.
 * A one, two or three digit decimal number is interpreted as its byte value.
 * If one or two digits are entered, the next character is given to vungetc().
 * For Unicode a character > 255 may be returned.
 */
    int
get_literal()
{
    int		cc;
    int		nc;
    int		i;
    int		hex = FALSE;
    int		octal = FALSE;
#ifdef FEAT_MBYTE
    int		unicode = 0;
#endif

    if (got_int)
	return Ctrl_C;

#ifdef FEAT_GUI
    /*
     * In GUI there is no point inserting the internal code for a special key.
     * It is more useful to insert the string "<KEY>" instead.	This would
     * probably be useful in a text window too, but it would not be
     * vi-compatible (maybe there should be an option for it?) -- webb
     */
    if (gui.in_use)
	++allow_keys;
#endif
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif
    ++no_mapping;		/* don't map the next key hits */
    cc = 0;
    i = 0;
    for (;;)
    {
	do
	    nc = safe_vgetc();
	while (nc == K_IGNORE || nc == K_VER_SCROLLBAR
						    || nc == K_HOR_SCROLLBAR);
#ifdef FEAT_CMDL_INFO
	if (!(State & CMDLINE)
# ifdef FEAT_MBYTE
		&& MB_BYTE2LEN_CHECK(nc) == 1
# endif
	   )
	    add_to_showcmd(nc);
#endif
	if (nc == 'x' || nc == 'X')
	    hex = TRUE;
	else if (nc == 'o' || nc == 'O')
	    octal = TRUE;
#ifdef FEAT_MBYTE
	else if (nc == 'u' || nc == 'U')
	    unicode = nc;
#endif
	else
	{
	    if (hex
#ifdef FEAT_MBYTE
		    || unicode != 0
#endif
		    )
	    {
		if (!vim_isxdigit(nc))
		    break;
		cc = cc * 16 + hex2nr(nc);
	    }
	    else if (octal)
	    {
		if (nc < '0' || nc > '7')
		    break;
		cc = cc * 8 + nc - '0';
	    }
	    else
	    {
		if (!VIM_ISDIGIT(nc))
		    break;
		cc = cc * 10 + nc - '0';
	    }

	    ++i;
	}

	if (cc > 255
#ifdef FEAT_MBYTE
		&& unicode == 0
#endif
		)
	    cc = 255;		/* limit range to 0-255 */
	nc = 0;

	if (hex)		/* hex: up to two chars */
	{
	    if (i >= 2)
		break;
	}
#ifdef FEAT_MBYTE
	else if (unicode)	/* Unicode: up to four or eight chars */
	{
	    if ((unicode == 'u' && i >= 4) || (unicode == 'U' && i >= 8))
		break;
	}
#endif
	else if (i >= 3)	/* decimal or octal: up to three chars */
	    break;
    }
    if (i == 0)	    /* no number entered */
    {
	if (nc == K_ZERO)   /* NUL is stored as NL */
	{
	    cc = '\n';
	    nc = 0;
	}
	else
	{
	    cc = nc;
	    nc = 0;
	}
    }

    if (cc == 0)	/* NUL is stored as NL */
	cc = '\n';

    --no_mapping;
#ifdef FEAT_GUI
    if (gui.in_use)
	--allow_keys;
#endif
    if (nc)
	vungetc(nc);
    got_int = FALSE;	    /* CTRL-C typed after CTRL-V is not an interrupt */
    return cc;
}

/*
 * Insert character, taking care of special keys and mod_mask
 */
    static void
insert_special(c, allow_modmask, ctrlv)
    int	    c;
    int	    allow_modmask;
    int	    ctrlv;	    /* c was typed after CTRL-V */
{
    char_u  *p;
    int	    len;

    /*
     * Special function key, translate into "<Key>". Up to the last '>' is
     * inserted with ins_str(), so as not to replace characters in replace
     * mode.
     * Only use mod_mask for special keys, to avoid things like <S-Space>,
     * unless 'allow_modmask' is TRUE.
     */
#ifdef MACOS
    /* Command-key never produces a normal key */
    if (mod_mask & MOD_MASK_CMD)
	allow_modmask = TRUE;
#endif
    if (IS_SPECIAL(c) || (mod_mask && allow_modmask))
    {
	p = get_special_key_name(c, mod_mask);
	len = (int)STRLEN(p);
	c = p[len - 1];
	if (len > 2)
	{
	    if (stop_arrow() == FAIL)
		return;
	    p[len - 1] = NUL;
	    ins_str(p);
	    AppendToRedobuffLit(p);
	    ctrlv = FALSE;
	}
    }
    if (stop_arrow() == OK)
	insertchar(c, ctrlv ? INSCHAR_CTRLV : 0, -1);
}

/*
 * Special characters in this context are those that need processing other
 * than the simple insertion that can be performed here. This includes ESC
 * which terminates the insert, and CR/NL which need special processing to
 * open up a new line. This routine tries to optimize insertions performed by
 * the "redo", "undo" or "put" commands, so it needs to know when it should
 * stop and defer processing to the "normal" mechanism.
 * '0' and '^' are special, because they can be followed by CTRL-D.
 */
#ifdef EBCDIC
# define ISSPECIAL(c)	((c) < ' ' || (c) == '0' || (c) == '^')
#else
# define ISSPECIAL(c)	((c) < ' ' || (c) >= DEL || (c) == '0' || (c) == '^')
#endif

#ifdef FEAT_MBYTE
# define WHITECHAR(cc) (vim_iswhite(cc) && (!enc_utf8 || !utf_iscomposing(utf_ptr2char(ml_get_cursor() + 1))))
#else
# define WHITECHAR(cc) vim_iswhite(cc)
#endif

    void
insertchar(c, flags, second_indent)
    int		c;			/* character to insert or NUL */
    int		flags;			/* INSCHAR_FORMAT, etc. */
    int		second_indent;		/* indent for second line if >= 0 */
{
    int		haveto_redraw = FALSE;
    int		textwidth;
#ifdef FEAT_COMMENTS
    colnr_T	leader_len;
    char_u	*p;
    int		no_leader = FALSE;
    int		do_comments = (flags & INSCHAR_DO_COM);
#endif
    int		fo_white_par;
    int		first_line = TRUE;
    int		fo_ins_blank;
#ifdef FEAT_MBYTE
    int		fo_multibyte;
#endif
    int		save_char = NUL;
    int		cc;

    textwidth = comp_textwidth(flags & INSCHAR_FORMAT);
    fo_ins_blank = has_format_option(FO_INS_BLANK);
#ifdef FEAT_MBYTE
    fo_multibyte = has_format_option(FO_MBYTE_BREAK);
#endif
    fo_white_par = has_format_option(FO_WHITE_PAR);

    /*
     * Try to break the line in two or more pieces when:
     * - Always do this if we have been called to do formatting only.
     * - Always do this when 'formatoptions' has the 'a' flag and the line
     *   ends in white space.
     * - Otherwise:
     *	 - Don't do this if inserting a blank
     *	 - Don't do this if an existing character is being replaced, unless
     *	   we're in VREPLACE mode.
     *	 - Do this if the cursor is not on the line where insert started
     *	 or - 'formatoptions' doesn't have 'l' or the line was not too long
     *	       before the insert.
     *	    - 'formatoptions' doesn't have 'b' or a blank was inserted at or
     *	      before 'textwidth'
     */
    if (textwidth
	    && ((flags & INSCHAR_FORMAT)
		|| (!vim_iswhite(c)
		    && !((State & REPLACE_FLAG)
#ifdef FEAT_VREPLACE
			&& !(State & VREPLACE_FLAG)
#endif
			&& *ml_get_cursor() != NUL)
		    && (curwin->w_cursor.lnum != Insstart.lnum
			|| ((!has_format_option(FO_INS_LONG)
				|| Insstart_textlen <= (colnr_T)textwidth)
			    && (!fo_ins_blank
				|| Insstart_blank_vcol <= (colnr_T)textwidth
			    ))))))
    {
	/*
	 * When 'ai' is off we don't want a space under the cursor to be
	 * deleted.  Replace it with an 'x' temporarily.
	 */
	if (!curbuf->b_p_ai)
	{
	    cc = gchar_cursor();
	    if (vim_iswhite(cc))
	    {
		save_char = cc;
		pchar_cursor('x');
	    }
	}

	/*
	 * Repeat breaking lines, until the current line is not too long.
	 */
	while (!got_int)
	{
	    int		startcol;		/* Cursor column at entry */
	    int		wantcol;		/* column at textwidth border */
	    int		foundcol;		/* column for start of spaces */
	    int		end_foundcol = 0;	/* column for start of word */
	    colnr_T	len;
	    colnr_T	virtcol;
#ifdef FEAT_VREPLACE
	    int		orig_col = 0;
	    char_u	*saved_text = NULL;
#endif
	    colnr_T	col;

	    virtcol = get_nolist_virtcol();
	    if (virtcol < (colnr_T)textwidth)
		break;

#ifdef FEAT_COMMENTS
	    if (no_leader)
		do_comments = FALSE;
	    else if (!(flags & INSCHAR_FORMAT)
					   && has_format_option(FO_WRAP_COMS))
		do_comments = TRUE;

	    /* Don't break until after the comment leader */
	    if (do_comments)
		leader_len = get_leader_len(ml_get_curline(), NULL, FALSE);
	    else
		leader_len = 0;

	    /* If the line doesn't start with a comment leader, then don't
	     * start one in a following broken line.  Avoids that a %word
	     * moved to the start of the next line causes all following lines
	     * to start with %. */
	    if (leader_len == 0)
		no_leader = TRUE;
#endif
	    if (!(flags & INSCHAR_FORMAT)
#ifdef FEAT_COMMENTS
		    && leader_len == 0
#endif
		    && !has_format_option(FO_WRAP))

	    {
		textwidth = 0;
		break;
	    }
	    if ((startcol = curwin->w_cursor.col) == 0)
		break;

	    /* find column of textwidth border */
	    coladvance((colnr_T)textwidth);
	    wantcol = curwin->w_cursor.col;

	    curwin->w_cursor.col = startcol - 1;
#ifdef FEAT_MBYTE
	    /* Correct cursor for multi-byte character. */
	    if (has_mbyte)
		mb_adjust_cursor();
#endif
	    foundcol = 0;

	    /*
	     * Find position to break at.
	     * Stop at first entered white when 'formatoptions' has 'v'
	     */
	    while ((!fo_ins_blank && !has_format_option(FO_INS_VI))
			|| curwin->w_cursor.lnum != Insstart.lnum
			|| curwin->w_cursor.col >= Insstart.col)
	    {
		cc = gchar_cursor();
		if (WHITECHAR(cc))
		{
		    /* remember position of blank just before text */
		    end_foundcol = curwin->w_cursor.col;

		    /* find start of sequence of blanks */
		    while (curwin->w_cursor.col > 0 && WHITECHAR(cc))
		    {
			dec_cursor();
			cc = gchar_cursor();
		    }
		    if (curwin->w_cursor.col == 0 && WHITECHAR(cc))
			break;		/* only spaces in front of text */
#ifdef FEAT_COMMENTS
		    /* Don't break until after the comment leader */
		    if (curwin->w_cursor.col < leader_len)
			break;
#endif
		    if (has_format_option(FO_ONE_LETTER))
		    {
			/* do not break after one-letter words */
			if (curwin->w_cursor.col == 0)
			    break;	/* one-letter word at begin */

			col = curwin->w_cursor.col;
			dec_cursor();
			cc = gchar_cursor();

			if (WHITECHAR(cc))
			    continue;	/* one-letter, continue */
			curwin->w_cursor.col = col;
		    }
#ifdef FEAT_MBYTE
		    if (has_mbyte)
			foundcol = curwin->w_cursor.col
				       + (*mb_ptr2len_check)(ml_get_cursor());
		    else
#endif
			foundcol = curwin->w_cursor.col + 1;
		    if (curwin->w_cursor.col < (colnr_T)wantcol)
			break;
		}
#ifdef FEAT_MBYTE
		else if (cc >= 0x100 && fo_multibyte
				  && curwin->w_cursor.col <= (colnr_T)wantcol)
		{
		    /* Break after or before a multi-byte character. */
		    foundcol = curwin->w_cursor.col;
		    if (curwin->w_cursor.col < (colnr_T)wantcol)
			foundcol += (*mb_char2len)(cc);
		    end_foundcol = foundcol;
		    break;
		}
#endif
		if (curwin->w_cursor.col == 0)
		    break;
		dec_cursor();
	    }

	    if (foundcol == 0)		/* no spaces, cannot break line */
	    {
		curwin->w_cursor.col = startcol;
		break;
	    }

	    /* Going to break the line, remove any "$" now. */
	    undisplay_dollar();

	    /*
	     * Offset between cursor position and line break is used by replace
	     * stack functions.  VREPLACE does not use this, and backspaces
	     * over the text instead.
	     */
#ifdef FEAT_VREPLACE
	    if (State & VREPLACE_FLAG)
		orig_col = startcol;	/* Will start backspacing from here */
	    else
#endif
		replace_offset = startcol - end_foundcol - 1;

	    /*
	     * adjust startcol for spaces that will be deleted and
	     * characters that will remain on top line
	     */
	    curwin->w_cursor.col = foundcol;
	    while (cc = gchar_cursor(), WHITECHAR(cc))
		inc_cursor();
	    startcol -= curwin->w_cursor.col;
	    if (startcol < 0)
		startcol = 0;

#ifdef FEAT_VREPLACE
	    if (State & VREPLACE_FLAG)
	    {
		/*
		 * In VREPLACE mode, we will backspace over the text to be
		 * wrapped, so save a copy now to put on the next line.
		 */
		saved_text = vim_strsave(ml_get_cursor());
		curwin->w_cursor.col = orig_col;
		if (saved_text == NULL)
		    break;	/* Can't do it, out of memory */
		saved_text[startcol] = NUL;

		/* Backspace over characters that will move to the next line */
		if (!fo_white_par)
		    backspace_until_column(foundcol);
	    }
	    else
#endif
	    {
		/* put cursor after pos. to break line */
		if (!fo_white_par)
		    curwin->w_cursor.col = foundcol;
	    }

	    /*
	     * Split the line just before the margin.
	     * Only insert/delete lines, but don't really redraw the window.
	     */
	    open_line(FORWARD, OPENLINE_DELSPACES + OPENLINE_MARKFIX
		    + (fo_white_par ? OPENLINE_KEEPTRAIL : 0)
#ifdef FEAT_COMMENTS
		    + (do_comments ? OPENLINE_DO_COM : 0)
#endif
		    , old_indent);
	    old_indent = 0;

	    replace_offset = 0;
	    if (first_line)
	    {
		if (second_indent < 0 && has_format_option(FO_Q_NUMBER))
		    second_indent = get_number_indent(curwin->w_cursor.lnum -1);
		if (second_indent >= 0)
		{
#ifdef FEAT_VREPLACE
		    if (State & VREPLACE_FLAG)
			change_indent(INDENT_SET, second_indent, FALSE, NUL);
		    else
#endif
			(void)set_indent(second_indent, SIN_CHANGED);
		}
		first_line = FALSE;
	    }

#ifdef FEAT_VREPLACE
	    if (State & VREPLACE_FLAG)
	    {
		/*
		 * In VREPLACE mode we have backspaced over the text to be
		 * moved, now we re-insert it into the new line.
		 */
		ins_bytes(saved_text);
		vim_free(saved_text);
	    }
	    else
#endif
	    {
		/*
		 * Check if cursor is not past the NUL off the line, cindent
		 * may have added or removed indent.
		 */
		curwin->w_cursor.col += startcol;
		len = (colnr_T)STRLEN(ml_get_curline());
		if (curwin->w_cursor.col > len)
		    curwin->w_cursor.col = len;
	    }

	    haveto_redraw = TRUE;
#ifdef FEAT_CINDENT
	    can_cindent = TRUE;
#endif
	    /* moved the cursor, don't autoindent or cindent now */
	    did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
	    did_si = FALSE;
	    can_si = FALSE;
	    can_si_back = FALSE;
#endif
	    line_breakcheck();
	}

	if (save_char)			/* put back space after cursor */
	    pchar_cursor(save_char);

	if (c == NUL)			/* formatting only */
	    return;
	if (haveto_redraw)
	{
	    update_topline();
	    redraw_curbuf_later(VALID);
	}
    }
    if (c == NUL)	    /* only formatting was wanted */
	return;

#ifdef FEAT_COMMENTS
    /* Check whether this character should end a comment. */
    if (did_ai && (int)c == end_comment_pending)
    {
	char_u  *line;
	char_u	lead_end[COM_MAX_LEN];	    /* end-comment string */
	int	middle_len, end_len;
	int	i;

	/*
	 * Need to remove existing (middle) comment leader and insert end
	 * comment leader.  First, check what comment leader we can find.
	 */
	i = get_leader_len(line = ml_get_curline(), &p, FALSE);
	if (i > 0 && vim_strchr(p, COM_MIDDLE) != NULL)	/* Just checking */
	{
	    /* Skip middle-comment string */
	    while (*p && p[-1] != ':')	/* find end of middle flags */
		++p;
	    middle_len = copy_option_part(&p, lead_end, COM_MAX_LEN, ",");
	    /* Don't count trailing white space for middle_len */
	    while (middle_len > 0 && vim_iswhite(lead_end[middle_len - 1]))
		--middle_len;

	    /* Find the end-comment string */
	    while (*p && p[-1] != ':')	/* find end of end flags */
		++p;
	    end_len = copy_option_part(&p, lead_end, COM_MAX_LEN, ",");

	    /* Skip white space before the cursor */
	    i = curwin->w_cursor.col;
	    while (--i >= 0 && vim_iswhite(line[i]))
		;
	    i++;

	    /* Skip to before the middle leader */
	    i -= middle_len;

	    /* Check some expected things before we go on */
	    if (i >= 0 && lead_end[end_len - 1] == end_comment_pending)
	    {
		/* Backspace over all the stuff we want to replace */
		backspace_until_column(i);

		/*
		 * Insert the end-comment string, except for the last
		 * character, which will get inserted as normal later.
		 */
		ins_bytes_len(lead_end, end_len - 1);
	    }
	}
    }
    end_comment_pending = NUL;
#endif

    did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif

    /*
     * If there's any pending input, grab up to INPUT_BUFLEN at once.
     * This speeds up normal text input considerably.
     * Don't do this when 'cindent' or 'indentexpr' is set, because we might
     * need to re-indent at a ':', or any other character (but not what
     * 'paste' is set)..
     */
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = FALSE;		/* allow scrolling here */
#endif

    if (       !ISSPECIAL(c)
#ifdef FEAT_MBYTE
	    && (!has_mbyte || (*mb_char2len)(c) == 1)
#endif
	    && vpeekc() != NUL
	    && !(State & REPLACE_FLAG)
#ifdef FEAT_CINDENT
	    && !cindent_on()
#endif
#ifdef FEAT_RIGHTLEFT
	    && !p_ri
#endif
	       )
    {
#define INPUT_BUFLEN 100
	char_u		buf[INPUT_BUFLEN + 1];
	int		i;
	colnr_T		virtcol = 0;

	buf[0] = c;
	i = 1;
	if (textwidth)
	    virtcol = get_nolist_virtcol();
	/*
	 * Stop the string when:
	 * - no more chars available
	 * - finding a special character (command key)
	 * - buffer is full
	 * - running into the 'textwidth' boundary
	 * - need to check for abbreviation: A non-word char after a word-char
	 */
	while (	   (c = vpeekc()) != NUL
		&& !ISSPECIAL(c)
#ifdef FEAT_MBYTE
		&& (!has_mbyte || MB_BYTE2LEN_CHECK(c) == 1)
#endif
		&& i < INPUT_BUFLEN
		&& (textwidth == 0
		    || (virtcol += byte2cells(buf[i - 1])) < (colnr_T)textwidth)
		&& !(!no_abbr && !vim_iswordc(c) && vim_iswordc(buf[i - 1])))
	{
#ifdef FEAT_RIGHTLEFT
	    c = vgetc();
	    if (p_hkmap && KeyTyped)
		c = hkmap(c);		    /* Hebrew mode mapping */
# ifdef FEAT_FKMAP
	    if (p_fkmap && KeyTyped)
		c = fkmap(c);		    /* Farsi mode mapping */
# endif
	    buf[i++] = c;
#else
	    buf[i++] = vgetc();
#endif
	}

#ifdef FEAT_DIGRAPHS
	do_digraph(-1);			/* clear digraphs */
	do_digraph(buf[i-1]);		/* may be the start of a digraph */
#endif
	buf[i] = NUL;
	ins_str(buf);
	if (flags & INSCHAR_CTRLV)
	{
	    redo_literal(*buf);
	    i = 1;
	}
	else
	    i = 0;
	if (buf[i] != NUL)
	    AppendToRedobuffLit(buf + i);
    }
    else
    {
#ifdef FEAT_MBYTE
	if (has_mbyte && (cc = (*mb_char2len)(c)) > 1)
	{
	    char_u	buf[MB_MAXBYTES + 1];

	    (*mb_char2bytes)(c, buf);
	    buf[cc] = NUL;
	    ins_char_bytes(buf, cc);
	    AppendCharToRedobuff(c);
	}
	else
#endif
	{
	    ins_char(c);
	    if (flags & INSCHAR_CTRLV)
		redo_literal(c);
	    else
		AppendCharToRedobuff(c);
	}
    }
}

/*
 * Called after inserting or deleting text: When 'formatoptions' includes the
 * 'a' flag format from the current line until the end of the paragraph.
 * Keep the cursor at the same position relative to the text.
 * The caller must have saved the cursor line for undo, following ones will be
 * saved here.
 */
    void
auto_format(trailblank, prev_line)
    int		trailblank;	/* when TRUE also format with trailing blank */
    int		prev_line;	/* may start in previous line */
{
    pos_T	pos;
    colnr_T	len;
    char_u	*old;
    char_u	*new, *pnew;
    int		wasatend;

    if (!has_format_option(FO_AUTO))
	return;

    pos = curwin->w_cursor;
    old = ml_get_curline();

    /* may remove added space */
    check_auto_format(FALSE);

    /* Don't format in Insert mode when the cursor is on a trailing blank, the
     * user might insert normal text next.  Also skip formatting when "1" is
     * in 'formatoptions' and there is a single character before the cursor.
     * Otherwise the line would be broken and when typing another non-white
     * next they are not joined back together. */
    wasatend = (pos.col == STRLEN(old));
    if (*old != NUL && !trailblank && wasatend)
    {
	dec_cursor();
	if (!WHITECHAR(gchar_cursor())
		&& curwin->w_cursor.col > 0
		&& has_format_option(FO_ONE_LETTER))
	    dec_cursor();
	if (WHITECHAR(gchar_cursor()))
	{
	    curwin->w_cursor = pos;
	    return;
	}
	curwin->w_cursor = pos;
    }

#ifdef FEAT_COMMENTS
    /* With the 'c' flag in 'formatoptions' and 't' missing: only format
     * comments. */
    if (has_format_option(FO_WRAP_COMS) && !has_format_option(FO_WRAP)
				     && get_leader_len(old, NULL, FALSE) == 0)
	return;
#endif

    /*
     * May start formatting in a previous line, so that after "x" a word is
     * moved to the previous line if it fits there now.  Only when this is not
     * the start of a paragraph.
     */
    if (prev_line && !paragraph_start(curwin->w_cursor.lnum))
    {
	--curwin->w_cursor.lnum;
	if (u_save_cursor() == FAIL)
	    return;
    }

    /*
     * Do the formatting and restore the cursor position.  "saved_cursor" will
     * be adjusted for the text formatting.
     */
    saved_cursor = pos;
    format_lines((linenr_T)-1);
    curwin->w_cursor = saved_cursor;
    saved_cursor.lnum = 0;

    if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
    {
	/* "cannot happen" */
	curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	coladvance((colnr_T)MAXCOL);
    }
    else
	check_cursor_col();

    /* Insert mode: If the cursor is now after the end of the line while it
     * previously wasn't, the line was broken.  Because of the rule above we
     * need to add a space when 'w' is in 'formatoptions' to keep a paragraph
     * formatted. */
    if (!wasatend && has_format_option(FO_WHITE_PAR))
    {
	new = ml_get_curline();
	len = STRLEN(new);
	if (curwin->w_cursor.col == len)
	{
	    pnew = vim_strnsave(new, len + 2);
	    pnew[len] = ' ';
	    pnew[len + 1] = NUL;
	    ml_replace(curwin->w_cursor.lnum, pnew, FALSE);
	    /* remove the space later */
	    did_add_space = TRUE;
	}
	else
	    /* may remove added space */
	    check_auto_format(FALSE);
    }

    check_cursor();
}

/*
 * When an extra space was added to continue a paragraph for auto-formatting,
 * delete it now.  The space must be under the cursor, just after the insert
 * position.
 */
    static void
check_auto_format(end_insert)
    int		end_insert;	    /* TRUE when ending Insert mode */
{
    int		c = ' ';

    if (did_add_space)
    {
	if (!WHITECHAR(gchar_cursor()))
	    /* Somehow the space was removed already. */
	    did_add_space = FALSE;
	else
	{
	    if (!end_insert)
	    {
		inc_cursor();
		c = gchar_cursor();
		dec_cursor();
	    }
	    if (c != NUL)
	    {
		/* The space is no longer at the end of the line, delete it. */
		del_char(FALSE);
		did_add_space = FALSE;
	    }
	}
    }
}

/*
 * Find out textwidth to be used for formatting:
 *	if 'textwidth' option is set, use it
 *	else if 'wrapmargin' option is set, use W_WIDTH(curwin) - 'wrapmargin'
 *	if invalid value, use 0.
 *	Set default to window width (maximum 79) for "gq" operator.
 */
    int
comp_textwidth(ff)
    int		ff;	/* force formatting (for "Q" command) */
{
    int		textwidth;

    textwidth = curbuf->b_p_tw;
    if (textwidth == 0 && curbuf->b_p_wm)
    {
	/* The width is the window width minus 'wrapmargin' minus all the
	 * things that add to the margin. */
	textwidth = W_WIDTH(curwin) - curbuf->b_p_wm;
#ifdef FEAT_CMDWIN
	if (cmdwin_type != 0)
	    textwidth -= 1;
#endif
#ifdef FEAT_FOLDING
	textwidth -= curwin->w_p_fdc;
#endif
#ifdef FEAT_SIGNS
	if (curwin->w_buffer->b_signlist != NULL
# ifdef FEAT_NETBEANS_INTG
			    || usingNetbeans
# endif
		    )
	    textwidth -= 1;
#endif
	if (curwin->w_p_nu)
	    textwidth -= 8;
    }
    if (textwidth < 0)
	textwidth = 0;
    if (ff && textwidth == 0)
    {
	textwidth = W_WIDTH(curwin) - 1;
	if (textwidth > 79)
	    textwidth = 79;
    }
    return textwidth;
}

/*
 * Put a character in the redo buffer, for when just after a CTRL-V.
 */
    static void
redo_literal(c)
    int	    c;
{
    char_u	buf[10];

    /* Only digits need special treatment.  Translate them into a string of
     * three digits. */
    if (VIM_ISDIGIT(c))
    {
	sprintf((char *)buf, "%03d", c);
	AppendToRedobuff(buf);
    }
    else
	AppendCharToRedobuff(c);
}

/*
 * start_arrow() is called when an arrow key is used in insert mode.
 * It resembles hitting the <ESC> key.
 */
    static void
start_arrow(end_insert_pos)
    pos_T    *end_insert_pos;
{
    if (!arrow_used)	    /* something has been inserted */
    {
	AppendToRedobuff(ESC_STR);
	stop_insert(end_insert_pos, FALSE);
	arrow_used = TRUE;	/* this means we stopped the current insert */
    }
}

/*
 * stop_arrow() is called before a change is made in insert mode.
 * If an arrow key has been used, start a new insertion.
 * Returns FAIL if undo is impossible, shouldn't insert then.
 */
    int
stop_arrow()
{
    if (arrow_used)
    {
	if (u_save_cursor() == OK)
	{
	    arrow_used = FALSE;
	    ins_need_undo = FALSE;
	}
	Insstart = curwin->w_cursor;	/* new insertion starts here */
	Insstart_textlen = linetabsize(ml_get_curline());
	ai_col = 0;
#ifdef FEAT_VREPLACE
	if (State & VREPLACE_FLAG)
	{
	    orig_line_count = curbuf->b_ml.ml_line_count;
	    vr_lines_changed = 1;
	}
#endif
	ResetRedobuff();
	AppendToRedobuff((char_u *)"1i");   /* pretend we start an insertion */
    }
    else if (ins_need_undo)
    {
	if (u_save_cursor() == OK)
	    ins_need_undo = FALSE;
    }

#ifdef FEAT_FOLDING
    /* Always open fold at the cursor line when inserting something. */
    foldOpenCursor();
#endif

    return (arrow_used || ins_need_undo ? FAIL : OK);
}

/*
 * do a few things to stop inserting
 */
    static void
stop_insert(end_insert_pos, esc)
    pos_T    *end_insert_pos;	/* where insert ended */
    int	    esc;		/* called by ins_esc() */
{
    int	    cc;

    stop_redo_ins();
    replace_flush();		/* abandon replace stack */

    /*
     * save the inserted text for later redo with ^@
     */
    vim_free(last_insert);
    last_insert = get_inserted();
    last_insert_skip = new_insert_skip;

    if (!arrow_used)
    {
	/* Auto-format now.  It may seem strange to do this when stopping an
	 * insertion (or moving the cursor), but it's required when appending
	 * a line and having it end in a space.  But only do it when something
	 * was actually inserted, otherwise undo won't work. */
	if (!ins_need_undo)
	{
	    /* When the cursor is at the end of the line after a space the
	     * formatting will move it to the following word.  Avoid that by
	     * moving the cursor onto the space. */
	    cc = 'x';
	    if (curwin->w_cursor.col > 0 && gchar_cursor() == NUL)
	    {
		dec_cursor();
		cc = gchar_cursor();
		if (!vim_iswhite(cc))
		    inc_cursor();
	    }

	    auto_format(TRUE, FALSE);

	    if (vim_iswhite(cc) && gchar_cursor() != NUL)
		inc_cursor();
	}

	/* If a space was inserted for auto-formatting, remove it now. */
	check_auto_format(TRUE);

	/* If we just did an auto-indent, remove the white space from the end
	 * of the line, and put the cursor back.  */
	if (did_ai && esc)
	{
	    if (gchar_cursor() == NUL && curwin->w_cursor.col > 0)
		--curwin->w_cursor.col;
	    while (cc = gchar_cursor(), vim_iswhite(cc))
		(void)del_char(TRUE);
	    if (cc != NUL)
		++curwin->w_cursor.col;	/* put cursor back on the NUL */

#ifdef FEAT_VISUAL
	    /* <C-S-Right> may have started Visual mode, adjust the position for
	     * deleted characters. */
	    if (VIsual_active && VIsual.lnum == curwin->w_cursor.lnum)
	    {
		cc = STRLEN(ml_get_curline());
		if (VIsual.col > (colnr_T)cc)
		{
		    VIsual.col = cc;
# ifdef FEAT_VIRTUALEDIT
		    VIsual.coladd = 0;
# endif
		}
	    }
#endif
	}
    }
    did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif

    /* set '[ and '] to the inserted text */
    curbuf->b_op_start = Insstart;
    curbuf->b_op_end = *end_insert_pos;
}

/*
 * Set the last inserted text to a single character.
 * Used for the replace command.
 */
    void
set_last_insert(c)
    int		c;
{
    char_u	*s;

    vim_free(last_insert);
#ifdef FEAT_MBYTE
    last_insert = alloc(MB_MAXBYTES * 3 + 5);
#else
    last_insert = alloc(6);
#endif
    if (last_insert != NULL)
    {
	s = last_insert;
	/* Use the CTRL-V only when entering a special char */
	if (c < ' ' || c == DEL)
	    *s++ = Ctrl_V;
	s = add_char2buf(c, s);
	*s++ = ESC;
	*s++ = NUL;
	last_insert_skip = 0;
    }
}

/*
 * Add character "c" to buffer "s".  Escape the special meaning of K_SPECIAL
 * and CSI.  Handle multi-byte characters.
 * Returns a pointer to after the added bytes.
 */
    char_u *
add_char2buf(c, s)
    int		c;
    char_u	*s;
{
#ifdef FEAT_MBYTE
    char_u	temp[MB_MAXBYTES];
    int		i;
    int		len;

    len = (*mb_char2bytes)(c, temp);
    for (i = 0; i < len; ++i)
    {
	c = temp[i];
#endif
	/* Need to escape K_SPECIAL and CSI like in the typeahead buffer. */
	if (c == K_SPECIAL)
	{
	    *s++ = K_SPECIAL;
	    *s++ = KS_SPECIAL;
	    *s++ = KE_FILLER;
	}
#ifdef FEAT_GUI
	else if (c == CSI)
	{
	    *s++ = CSI;
	    *s++ = KS_EXTRA;
	    *s++ = (int)KE_CSI;
	}
#endif
	else
	    *s++ = c;
#ifdef FEAT_MBYTE
    }
#endif
    return s;
}

/*
 * move cursor to start of line
 * if flags & BL_WHITE	move to first non-white
 * if flags & BL_SOL	move to first non-white if startofline is set,
 *			    otherwise keep "curswant" column
 * if flags & BL_FIX	don't leave the cursor on a NUL.
 */
    void
beginline(flags)
    int		flags;
{
    if ((flags & BL_SOL) && !p_sol)
	coladvance(curwin->w_curswant);
    else
    {
	curwin->w_cursor.col = 0;
#ifdef FEAT_VIRTUALEDIT
	curwin->w_cursor.coladd = 0;
#endif

	if (flags & (BL_WHITE | BL_SOL))
	{
	    char_u  *ptr;

	    for (ptr = ml_get_curline(); vim_iswhite(*ptr)
			       && !((flags & BL_FIX) && ptr[1] == NUL); ++ptr)
		++curwin->w_cursor.col;
	}
	curwin->w_set_curswant = TRUE;
    }
}

/*
 * oneright oneleft cursor_down cursor_up
 *
 * Move one char {right,left,down,up}.
 * Doesn't move onto the NUL past the end of the line.
 * Return OK when successful, FAIL when we hit a line of file boundary.
 */

    int
oneright()
{
    char_u	*ptr;
#ifdef FEAT_MBYTE
    int		l;
#endif

#ifdef FEAT_VIRTUALEDIT
    if (virtual_active())
    {
	pos_T	prevpos = curwin->w_cursor;

	/* Adjust for multi-wide char (excluding TAB) */
	ptr = ml_get_cursor();
	coladvance(getviscol() + ((*ptr != TAB && vim_isprintc(
#ifdef FEAT_MBYTE
			    (*mb_ptr2char)(ptr)
#else
			    *ptr
#endif
			    ))
		    ? ptr2cells(ptr) : 1));
	curwin->w_set_curswant = TRUE;
	/* Return OK if the cursor moved, FAIL otherwise (at window edge). */
	return (prevpos.col != curwin->w_cursor.col
		    || prevpos.coladd != curwin->w_cursor.coladd) ? OK : FAIL;
    }
#endif

    ptr = ml_get_cursor();
#ifdef FEAT_MBYTE
    if (has_mbyte && (l = (*mb_ptr2len_check)(ptr)) > 1)
    {
	/* The character under the cursor is a multi-byte character, move
	 * several bytes right, but don't end up on the NUL. */
	if (ptr[l] == NUL)
	    return FAIL;
	curwin->w_cursor.col += l;
    }
    else
#endif
    {
	if (*ptr++ == NUL || *ptr == NUL)
	    return FAIL;
	++curwin->w_cursor.col;
    }

    curwin->w_set_curswant = TRUE;
    return OK;
}

    int
oneleft()
{
#ifdef FEAT_VIRTUALEDIT
    if (virtual_active())
    {
	int width;
	int v = getviscol();

	if (v == 0)
	    return FAIL;

# ifdef FEAT_LINEBREAK
	/* We might get stuck on 'showbreak', skip over it. */
	width = 1;
	for (;;)
	{
	    coladvance(v - width);
	    /* getviscol() is slow, skip it when 'showbreak' is empty and
	     * there are no multi-byte characters */
	    if ((*p_sbr == NUL
#  ifdef FEAT_MBYTE
			&& !has_mbyte
#  endif
			) || getviscol() < v)
		break;
	    ++width;
	}
# else
	coladvance(v - 1);
# endif

	if (curwin->w_cursor.coladd == 1)
	{
	    char_u *ptr;

	    /* Adjust for multi-wide char (not a TAB) */
	    ptr = ml_get_cursor();
	    if (*ptr != TAB && vim_isprintc(
#  ifdef FEAT_MBYTE
			    (*mb_ptr2char)(ptr)
#  else
			    *ptr
#  endif
			    ) && ptr2cells(ptr) > 1)
		curwin->w_cursor.coladd = 0;
	}

	curwin->w_set_curswant = TRUE;
	return OK;
    }
#endif

    if (curwin->w_cursor.col == 0)
	return FAIL;

    curwin->w_set_curswant = TRUE;
    --curwin->w_cursor.col;

#ifdef FEAT_MBYTE
    /* if the character on the left of the current cursor is a multi-byte
     * character, move to its first byte */
    if (has_mbyte)
	mb_adjust_cursor();
#endif
    return OK;
}

    int
cursor_up(n, upd_topline)
    long	n;
    int		upd_topline;	    /* When TRUE: update topline */
{
    linenr_T	lnum;

    if (n > 0)
    {
	lnum = curwin->w_cursor.lnum;
	if (lnum <= 1)
	    return FAIL;
	if (n >= lnum)
	    lnum = 1;
	else
#ifdef FEAT_FOLDING
	    if (hasAnyFolding(curwin))
	{
	    /*
	     * Count each sequence of folded lines as one logical line.
	     */
	    /* go to the the start of the current fold */
	    (void)hasFolding(lnum, &lnum, NULL);

	    while (n--)
	    {
		/* move up one line */
		--lnum;
		if (lnum <= 1)
		    break;
		/* If we entered a fold, move to the beginning, unless in
		 * Insert mode or when 'foldopen' contains "all": it will open
		 * in a moment. */
		if (n > 0 || !((State & INSERT) || (fdo_flags & FDO_ALL)))
		    (void)hasFolding(lnum, &lnum, NULL);
	    }
	    if (lnum < 1)
		lnum = 1;
	}
	else
#endif
	    lnum -= n;
	curwin->w_cursor.lnum = lnum;
    }

    /* try to advance to the column we want to be at */
    coladvance(curwin->w_curswant);

    if (upd_topline)
	update_topline();	/* make sure curwin->w_topline is valid */

    return OK;
}

/*
 * Cursor down a number of logical lines.
 */
    int
cursor_down(n, upd_topline)
    long	n;
    int		upd_topline;	    /* When TRUE: update topline */
{
    linenr_T	lnum;

    if (n > 0)
    {
	lnum = curwin->w_cursor.lnum;
#ifdef FEAT_FOLDING
	/* Move to last line of fold, will fail if it's the end-of-file. */
	(void)hasFolding(lnum, NULL, &lnum);
#endif
	if (lnum >= curbuf->b_ml.ml_line_count)
	    return FAIL;
	if (lnum + n >= curbuf->b_ml.ml_line_count)
	    lnum = curbuf->b_ml.ml_line_count;
	else
#ifdef FEAT_FOLDING
	if (hasAnyFolding(curwin))
	{
	    linenr_T	last;

	    /* count each sequence of folded lines as one logical line */
	    while (n--)
	    {
		if (hasFolding(lnum, NULL, &last))
		    lnum = last + 1;
		else
		    ++lnum;
		if (lnum >= curbuf->b_ml.ml_line_count)
		    break;
	    }
	    if (lnum > curbuf->b_ml.ml_line_count)
		lnum = curbuf->b_ml.ml_line_count;
	}
	else
#endif
	    lnum += n;
	curwin->w_cursor.lnum = lnum;
    }

    /* try to advance to the column we want to be at */
    coladvance(curwin->w_curswant);

    if (upd_topline)
	update_topline();	/* make sure curwin->w_topline is valid */

    return OK;
}

/*
 * Stuff the last inserted text in the read buffer.
 * Last_insert actually is a copy of the redo buffer, so we
 * first have to remove the command.
 */
    int
stuff_inserted(c, count, no_esc)
    int	    c;		/* Command character to be inserted */
    long    count;	/* Repeat this many times */
    int	    no_esc;	/* Don't add an ESC at the end */
{
    char_u	*esc_ptr;
    char_u	*ptr;
    char_u	*last_ptr;
    char_u	last = NUL;

    ptr = get_last_insert();
    if (ptr == NULL)
    {
	EMSG(_(e_noinstext));
	return FAIL;
    }

    /* may want to stuff the command character, to start Insert mode */
    if (c != NUL)
	stuffcharReadbuff(c);
    if ((esc_ptr = (char_u *)vim_strrchr(ptr, ESC)) != NULL)
	*esc_ptr = NUL;	    /* remove the ESC */

    /* when the last char is either "0" or "^" it will be quoted if no ESC
     * comes after it OR if it will inserted more than once and "ptr"
     * starts with ^D.	-- Acevedo
     */
    last_ptr = (esc_ptr ? esc_ptr : ptr + STRLEN(ptr)) - 1;
    if (last_ptr >= ptr && (*last_ptr == '0' || *last_ptr == '^')
	    && (no_esc || (*ptr == Ctrl_D && count > 1)))
    {
	last = *last_ptr;
	*last_ptr = NUL;
    }

    do
    {
	stuffReadbuff(ptr);
	/* a trailing "0" is inserted as "<C-V>048", "^" as "<C-V>^" */
	if (last)
	    stuffReadbuff((char_u *)(last == '0'
			? IF_EB("\026\060\064\070", CTRL_V_STR "xf0")
			: IF_EB("\026^", CTRL_V_STR "^")));
    }
    while (--count > 0);

    if (last)
	*last_ptr = last;

    if (esc_ptr != NULL)
	*esc_ptr = ESC;	    /* put the ESC back */

    /* may want to stuff a trailing ESC, to get out of Insert mode */
    if (!no_esc)
	stuffcharReadbuff(ESC);

    return OK;
}

    char_u *
get_last_insert()
{
    if (last_insert == NULL)
	return NULL;
    return last_insert + last_insert_skip;
}

/*
 * Get last inserted string, and remove trailing <Esc>.
 * Returns pointer to allocated memory (must be freed) or NULL.
 */
    char_u *
get_last_insert_save()
{
    char_u	*s;
    int		len;

    if (last_insert == NULL)
	return NULL;
    s = vim_strsave(last_insert + last_insert_skip);
    if (s != NULL)
    {
	len = (int)STRLEN(s);
	if (len > 0 && s[len - 1] == ESC)	/* remove trailing ESC */
	    s[len - 1] = NUL;
    }
    return s;
}

/*
 * Check the word in front of the cursor for an abbreviation.
 * Called when the non-id character "c" has been entered.
 * When an abbreviation is recognized it is removed from the text and
 * the replacement string is inserted in typebuf.tb_buf[], followed by "c".
 */
    static int
echeck_abbr(c)
    int c;
{
    /* Don't check for abbreviation in paste mode, when disabled and just
     * after moving around with cursor keys. */
    if (p_paste || no_abbr || arrow_used)
	return FALSE;

    return check_abbr(c, ml_get_curline(), curwin->w_cursor.col,
		curwin->w_cursor.lnum == Insstart.lnum ? Insstart.col : 0);
}

/*
 * replace-stack functions
 *
 * When replacing characters, the replaced characters are remembered for each
 * new character.  This is used to re-insert the old text when backspacing.
 *
 * There is a NUL headed list of characters for each character that is
 * currently in the file after the insertion point.  When BS is used, one NUL
 * headed list is put back for the deleted character.
 *
 * For a newline, there are two NUL headed lists.  One contains the characters
 * that the NL replaced.  The extra one stores the characters after the cursor
 * that were deleted (always white space).
 *
 * Replace_offset is normally 0, in which case replace_push will add a new
 * character at the end of the stack.  If replace_offset is not 0, that many
 * characters will be left on the stack above the newly inserted character.
 */

char_u	*replace_stack = NULL;
long	replace_stack_nr = 0;	    /* next entry in replace stack */
long	replace_stack_len = 0;	    /* max. number of entries */

    void
replace_push(c)
    int	    c;	    /* character that is replaced (NUL is none) */
{
    char_u  *p;

    if (replace_stack_nr < replace_offset)	/* nothing to do */
	return;
    if (replace_stack_len <= replace_stack_nr)
    {
	replace_stack_len += 50;
	p = lalloc(sizeof(char_u) * replace_stack_len, TRUE);
	if (p == NULL)	    /* out of memory */
	{
	    replace_stack_len -= 50;
	    return;
	}
	if (replace_stack != NULL)
	{
	    mch_memmove(p, replace_stack,
				 (size_t)(replace_stack_nr * sizeof(char_u)));
	    vim_free(replace_stack);
	}
	replace_stack = p;
    }
    p = replace_stack + replace_stack_nr - replace_offset;
    if (replace_offset)
	mch_memmove(p + 1, p, (size_t)(replace_offset * sizeof(char_u)));
    *p = c;
    ++replace_stack_nr;
}

/*
 * call replace_push(c) with replace_offset set to the first NUL.
 */
    static void
replace_push_off(c)
    int	    c;
{
    char_u	*p;

    p = replace_stack + replace_stack_nr;
    for (replace_offset = 1; replace_offset < replace_stack_nr;
							     ++replace_offset)
	if (*--p == NUL)
	    break;
    replace_push(c);
    replace_offset = 0;
}

/*
 * Pop one item from the replace stack.
 * return -1 if stack empty
 * return replaced character or NUL otherwise
 */
    static int
replace_pop()
{
    if (replace_stack_nr == 0)
	return -1;
    return (int)replace_stack[--replace_stack_nr];
}

/*
 * Join the top two items on the replace stack.  This removes to "off"'th NUL
 * encountered.
 */
    static void
replace_join(off)
    int	    off;	/* offset for which NUL to remove */
{
    int	    i;

    for (i = replace_stack_nr; --i >= 0; )
	if (replace_stack[i] == NUL && off-- <= 0)
	{
	    --replace_stack_nr;
	    mch_memmove(replace_stack + i, replace_stack + i + 1,
					      (size_t)(replace_stack_nr - i));
	    return;
	}
}

/*
 * Pop bytes from the replace stack until a NUL is found, and insert them
 * before the cursor.  Can only be used in REPLACE or VREPLACE mode.
 */
    static void
replace_pop_ins()
{
    int	    cc;
    int	    oldState = State;

    State = NORMAL;			/* don't want REPLACE here */
    while ((cc = replace_pop()) > 0)
    {
#ifdef FEAT_MBYTE
	mb_replace_pop_ins(cc);
#else
	ins_char(cc);
#endif
	dec_cursor();
    }
    State = oldState;
}

#ifdef FEAT_MBYTE
/*
 * Insert bytes popped from the replace stack. "cc" is the first byte.  If it
 * indicates a multi-byte char, pop the other bytes too.
 */
    static void
mb_replace_pop_ins(cc)
    int		cc;
{
    int		n;
    char_u	buf[MB_MAXBYTES];
    int		i;
    int		c;

    if (has_mbyte && (n = MB_BYTE2LEN(cc)) > 1)
    {
	buf[0] = cc;
	for (i = 1; i < n; ++i)
	    buf[i] = replace_pop();
	ins_bytes_len(buf, n);
    }
    else
	ins_char(cc);

    if (enc_utf8)
	/* Handle composing chars. */
	for (;;)
	{
	    c = replace_pop();
	    if (c == -1)	    /* stack empty */
		break;
	    if ((n = MB_BYTE2LEN(c)) == 1)
	    {
		/* Not a multi-byte char, put it back. */
		replace_push(c);
		break;
	    }
	    else
	    {
		buf[0] = c;
		for (i = 1; i < n; ++i)
		    buf[i] = replace_pop();
		if (utf_iscomposing(utf_ptr2char(buf)))
		    ins_bytes_len(buf, n);
		else
		{
		    /* Not a composing char, put it back. */
		    for (i = n - 1; i >= 0; --i)
			replace_push(buf[i]);
		    break;
		}
	    }
	}
}
#endif

/*
 * make the replace stack empty
 * (called when exiting replace mode)
 */
    static void
replace_flush()
{
    vim_free(replace_stack);
    replace_stack = NULL;
    replace_stack_len = 0;
    replace_stack_nr = 0;
}

/*
 * Handle doing a BS for one character.
 * cc < 0: replace stack empty, just move cursor
 * cc == 0: character was inserted, delete it
 * cc > 0: character was replaced, put cc (first byte of original char) back
 * and check for more characters to be put back
 */
    static void
replace_do_bs()
{
    int		cc;
#ifdef FEAT_VREPLACE
    int		orig_len = 0;
    int		ins_len;
    int		orig_vcols = 0;
    colnr_T	start_vcol;
    char_u	*p;
    int		i;
    int		vcol;
#endif

    cc = replace_pop();
    if (cc > 0)
    {
#ifdef FEAT_VREPLACE
	if (State & VREPLACE_FLAG)
	{
	    /* Get the number of screen cells used by the character we are
	     * going to delete. */
	    getvcol(curwin, &curwin->w_cursor, NULL, &start_vcol, NULL);
	    orig_vcols = chartabsize(ml_get_cursor(), start_vcol);
	}
#endif
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    del_char(FALSE);
# ifdef FEAT_VREPLACE
	    if (State & VREPLACE_FLAG)
		orig_len = STRLEN(ml_get_cursor());
# endif
	    replace_push(cc);
	}
	else
#endif
	{
	    pchar_cursor(cc);
#ifdef FEAT_VREPLACE
	    if (State & VREPLACE_FLAG)
		orig_len = STRLEN(ml_get_cursor()) - 1;
#endif
	}
	replace_pop_ins();

#ifdef FEAT_VREPLACE
	if (State & VREPLACE_FLAG)
	{
	    /* Get the number of screen cells used by the inserted characters */
	    p = ml_get_cursor();
	    ins_len = STRLEN(p) - orig_len;
	    vcol = start_vcol;
	    for (i = 0; i < ins_len; ++i)
	    {
		vcol += chartabsize(p + i, vcol);
#ifdef FEAT_MBYTE
		i += (*mb_ptr2len_check)(p) - 1;
#endif
	    }
	    vcol -= start_vcol;

	    /* Delete spaces that were inserted after the cursor to keep the
	     * text aligned. */
	    curwin->w_cursor.col += ins_len;
	    while (vcol > orig_vcols && gchar_cursor() == ' ')
	    {
		del_char(FALSE);
		++orig_vcols;
	    }
	    curwin->w_cursor.col -= ins_len;
	}
#endif

	/* mark the buffer as changed and prepare for displaying */
	changed_bytes(curwin->w_cursor.lnum, curwin->w_cursor.col);
    }
    else if (cc == 0)
	(void)del_char(FALSE);
}

#ifdef FEAT_CINDENT
/*
 * Return TRUE if C-indenting is on.
 */
    static int
cindent_on()
{
    return (!p_paste && (curbuf->b_p_cin
# ifdef FEAT_EVAL
		    || *curbuf->b_p_inde != NUL
# endif
		    ));
}
#endif

#if defined(FEAT_LISP) || defined(FEAT_CINDENT) || defined(PROTO)
/*
 * Re-indent the current line, based on the current contents of it and the
 * surrounding lines. Fixing the cursor position seems really easy -- I'm very
 * confused what all the part that handles Control-T is doing that I'm not.
 * "get_the_indent" should be get_c_indent, get_expr_indent or get_lisp_indent.
 */

    void
fixthisline(get_the_indent)
    int (*get_the_indent) __ARGS((void));
{
    change_indent(INDENT_SET, get_the_indent(), FALSE, 0);
    if (linewhite(curwin->w_cursor.lnum))
	did_ai = TRUE;	    /* delete the indent if the line stays empty */
}

    void
fix_indent()
{
    if (p_paste)
	return;
# ifdef FEAT_LISP
    if (curbuf->b_p_lisp && curbuf->b_p_ai)
	fixthisline(get_lisp_indent);
# endif
# if defined(FEAT_LISP) && defined(FEAT_CINDENT)
    else
# endif
# ifdef FEAT_CINDENT
	if (cindent_on())
	    do_c_expr_indent();
# endif
}

#endif

#ifdef FEAT_CINDENT
/*
 * return TRUE if 'cinkeys' contains the key "keytyped",
 * when == '*':	    Only if key is preceded with '*'	(indent before insert)
 * when == '!':	    Only if key is prededed with '!'	(don't insert)
 * when == ' ':	    Only if key is not preceded with '*'(indent afterwards)
 *
 * "keytyped" can have a few special values:
 * KEY_OPEN_FORW
 * KEY_OPEN_BACK
 * KEY_COMPLETE	    just finished completion.
 *
 * If line_is_empty is TRUE accept keys with '0' before them.
 */
    int
in_cinkeys(keytyped, when, line_is_empty)
    int		keytyped;
    int		when;
    int		line_is_empty;
{
    char_u	*look;
    int		try_match;
    int		try_match_word;
    char_u	*p;
    char_u	*line;
    int		icase;
    int		i;

#ifdef FEAT_EVAL
    if (*curbuf->b_p_inde != NUL)
	look = curbuf->b_p_indk;	/* 'indentexpr' set: use 'indentkeys' */
    else
#endif
	look = curbuf->b_p_cink;	/* 'indentexpr' empty: use 'cinkeys' */
    while (*look)
    {
	/*
	 * Find out if we want to try a match with this key, depending on
	 * 'when' and a '*' or '!' before the key.
	 */
	switch (when)
	{
	    case '*': try_match = (*look == '*'); break;
	    case '!': try_match = (*look == '!'); break;
	     default: try_match = (*look != '*'); break;
	}
	if (*look == '*' || *look == '!')
	    ++look;

	/*
	 * If there is a '0', only accept a match if the line is empty.
	 * But may still match when typing last char of a word.
	 */
	if (*look == '0')
	{
	    try_match_word = try_match;
	    if (!line_is_empty)
		try_match = FALSE;
	    ++look;
	}
	else
	    try_match_word = FALSE;

	/*
	 * does it look like a control character?
	 */
	if (*look == '^'
#ifdef EBCDIC
		&& (Ctrl_chr(look[1]) != 0)
#else
		&& look[1] >= '?' && look[1] <= '_'
#endif
		)
	{
	    if (try_match && keytyped == Ctrl_chr(look[1]))
		return TRUE;
	    look += 2;
	}
	/*
	 * 'o' means "o" command, open forward.
	 * 'O' means "O" command, open backward.
	 */
	else if (*look == 'o')
	{
	    if (try_match && keytyped == KEY_OPEN_FORW)
		return TRUE;
	    ++look;
	}
	else if (*look == 'O')
	{
	    if (try_match && keytyped == KEY_OPEN_BACK)
		return TRUE;
	    ++look;
	}

	/*
	 * 'e' means to check for "else" at start of line and just before the
	 * cursor.
	 */
	else if (*look == 'e')
	{
	    if (try_match && keytyped == 'e' && curwin->w_cursor.col >= 4)
	    {
		p = ml_get_curline();
		if (skipwhite(p) == p + curwin->w_cursor.col - 4 &&
			STRNCMP(p + curwin->w_cursor.col - 4, "else", 4) == 0)
		    return TRUE;
	    }
	    ++look;
	}

	/*
	 * ':' only causes an indent if it is at the end of a label or case
	 * statement, or when it was before typing the ':' (to fix
	 * class::method for C++).
	 */
	else if (*look == ':')
	{
	    if (try_match && keytyped == ':')
	    {
		p = ml_get_curline();
		if (cin_iscase(p) || cin_isscopedecl(p) || cin_islabel(30))
		    return TRUE;
		if (curwin->w_cursor.col > 2
			&& p[curwin->w_cursor.col - 1] == ':'
			&& p[curwin->w_cursor.col - 2] == ':')
		{
		    p[curwin->w_cursor.col - 1] = ' ';
		    i = (cin_iscase(p) || cin_isscopedecl(p)
							  || cin_islabel(30));
		    p = ml_get_curline();
		    p[curwin->w_cursor.col - 1] = ':';
		    if (i)
			return TRUE;
		}
	    }
	    ++look;
	}


	/*
	 * Is it a key in <>, maybe?
	 */
	else if (*look == '<')
	{
	    if (try_match)
	    {
		/*
		 * make up some named keys <o>, <O>, <e>, <0>, <>>, <<>, <*>,
		 * <:> and <!> so that people can re-indent on o, O, e, 0, <,
		 * >, *, : and ! keys if they really really want to.
		 */
		if (vim_strchr((char_u *)"<>!*oOe0:", look[1]) != NULL
						       && keytyped == look[1])
		    return TRUE;

		if (keytyped == get_special_key_code(look + 1))
		    return TRUE;
	    }
	    while (*look && *look != '>')
		look++;
	    while (*look == '>')
		look++;
	}

	/*
	 * Is it a word: "=word"?
	 */
	else if (*look == '=' && look[1] != ',' && look[1] != NUL)
	{
	    ++look;
	    if (*look == '~')
	    {
		icase = TRUE;
		++look;
	    }
	    else
		icase = FALSE;
	    p = vim_strchr(look, ',');
	    if (p == NULL)
		p = look + STRLEN(look);
	    if ((try_match || try_match_word)
		    && curwin->w_cursor.col >= (colnr_T)(p - look))
	    {
		int		match = FALSE;

#ifdef FEAT_INS_EXPAND
		if (keytyped == KEY_COMPLETE)
		{
		    char_u	*s;

		    /* Just completed a word, check if it starts with "look".
		     * search back for the start of a word. */
		    line = ml_get_curline();
# ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			char_u	*n;

			for (s = line + curwin->w_cursor.col; s > line; s = n)
			{
			    n = mb_prevptr(line, s);
			    if (!vim_iswordp(n))
				break;
			}
		    }
		    else
# endif
			for (s = line + curwin->w_cursor.col; s > line; --s)
			    if (!vim_iswordc(s[-1]))
				break;
		    if (s + (p - look) <= line + curwin->w_cursor.col
			    && (icase
				? MB_STRNICMP(s, look, p - look)
				: STRNCMP(s, look, p - look)) == 0)
			match = TRUE;
		}
		else
#endif
		    /* TODO: multi-byte */
		    if (keytyped == (int)p[-1] || (icase && keytyped < 256
			 && TOLOWER_LOC(keytyped) == TOLOWER_LOC((int)p[-1])))
		{
		    line = ml_get_cursor();
		    if ((curwin->w_cursor.col == (colnr_T)(p - look)
				|| !vim_iswordc(line[-(p - look) - 1]))
			    && (icase
				? MB_STRNICMP(line - (p - look), look, p - look)
				: STRNCMP(line - (p - look), look, p - look))
									 == 0)
			match = TRUE;
		}
		if (match && try_match_word && !try_match)
		{
		    /* "0=word": Check if there are only blanks before the
		     * word. */
		    line = ml_get_curline();
		    if ((int)(skipwhite(line) - line) !=
				     (int)(curwin->w_cursor.col - (p - look)))
			match = FALSE;
		}
		if (match)
		    return TRUE;
	    }
	    look = p;
	}

	/*
	 * ok, it's a boring generic character.
	 */
	else
	{
	    if (try_match && *look == keytyped)
		return TRUE;
	    ++look;
	}

	/*
	 * Skip over ", ".
	 */
	look = skip_to_option_part(look);
    }
    return FALSE;
}
#endif /* FEAT_CINDENT */

#if defined(FEAT_RIGHTLEFT) || defined(PROTO)
/*
 * Map Hebrew keyboard when in hkmap mode.
 */
    int
hkmap(c)
    int c;
{
    if (p_hkmapp)   /* phonetic mapping, by Ilya Dogolazky */
    {
	enum {hALEF=0, BET, GIMEL, DALET, HEI, VAV, ZAIN, HET, TET, IUD,
	    KAFsofit, hKAF, LAMED, MEMsofit, MEM, NUNsofit, NUN, SAMEH, AIN,
	    PEIsofit, PEI, ZADIsofit, ZADI, KOF, RESH, hSHIN, TAV};
	static char_u map[26] =
	    {(char_u)hALEF/*a*/, (char_u)BET  /*b*/, (char_u)hKAF    /*c*/,
	     (char_u)DALET/*d*/, (char_u)-1   /*e*/, (char_u)PEIsofit/*f*/,
	     (char_u)GIMEL/*g*/, (char_u)HEI  /*h*/, (char_u)IUD     /*i*/,
	     (char_u)HET  /*j*/, (char_u)KOF  /*k*/, (char_u)LAMED   /*l*/,
	     (char_u)MEM  /*m*/, (char_u)NUN  /*n*/, (char_u)SAMEH   /*o*/,
	     (char_u)PEI  /*p*/, (char_u)-1   /*q*/, (char_u)RESH    /*r*/,
	     (char_u)ZAIN /*s*/, (char_u)TAV  /*t*/, (char_u)TET     /*u*/,
	     (char_u)VAV  /*v*/, (char_u)hSHIN/*w*/, (char_u)-1      /*x*/,
	     (char_u)AIN  /*y*/, (char_u)ZADI /*z*/};

	if (c == 'N' || c == 'M' || c == 'P' || c == 'C' || c == 'Z')
	    return (int)(map[CharOrd(c)] - 1 + p_aleph);
							    /* '-1'='sofit' */
	else if (c == 'x')
	    return 'X';
	else if (c == 'q')
	    return '\''; /* {geresh}={'} */
	else if (c == 246)
	    return ' ';  /* \"o --> ' ' for a german keyboard */
	else if (c == 228)
	    return ' ';  /* \"a --> ' '      -- / --	       */
	else if (c == 252)
	    return ' ';  /* \"u --> ' '      -- / --	       */
#ifdef EBCDIC
	else if (islower(c))
#else
	/* NOTE: islower() does not do the right thing for us on Linux so we
	 * do this the same was as 5.7 and previous, so it works correctly on
	 * all systems.  Specifically, the e.g. Delete and Arrow keys are
	 * munged and won't work if e.g. searching for Hebrew text.
	 */
	else if (c >= 'a' && c <= 'z')
#endif
	    return (int)(map[CharOrdLow(c)] + p_aleph);
	else
	    return c;
    }
    else
    {
	switch (c)
	{
	    case '`':	return ';';
	    case '/':	return '.';
	    case '\'':	return ',';
	    case 'q':	return '/';
	    case 'w':	return '\'';

			/* Hebrew letters - set offset from 'a' */
	    case ',':	c = '{'; break;
	    case '.':	c = 'v'; break;
	    case ';':	c = 't'; break;
	    default: {
			 static char str[] = "zqbcxlsjphmkwonu ydafe rig";

#ifdef EBCDIC
			 /* see note about islower() above */
			 if (!islower(c))
#else
			 if (c < 'a' || c > 'z')
#endif
			     return c;
			 c = str[CharOrdLow(c)];
			 break;
		     }
	}

	return (int)(CharOrdLow(c) + p_aleph);
    }
}
#endif

    static void
ins_reg()
{
    int		need_redraw = FALSE;
    int		regname;
    int		literally = 0;

    /*
     * If we are going to wait for a character, show a '"'.
     */
    pc_status = PC_STATUS_UNSET;
    if (redrawing() && !char_avail())
    {
	/* may need to redraw when no more chars available now */
	ins_redraw();

	edit_putchar('"', TRUE);
#ifdef FEAT_CMDL_INFO
	add_to_showcmd_c(Ctrl_R);
#endif
    }

#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif

    /*
     * Don't map the register name. This also prevents the mode message to be
     * deleted when ESC is hit.
     */
    ++no_mapping;
    regname = safe_vgetc();
#ifdef FEAT_LANGMAP
    LANGMAP_ADJUST(regname, TRUE);
#endif
    if (regname == Ctrl_R || regname == Ctrl_O || regname == Ctrl_P)
    {
	/* Get a third key for literal register insertion */
	literally = regname;
#ifdef FEAT_CMDL_INFO
	add_to_showcmd_c(literally);
#endif
	regname = safe_vgetc();
#ifdef FEAT_LANGMAP
	LANGMAP_ADJUST(regname, TRUE);
#endif
    }
    --no_mapping;

#ifdef FEAT_EVAL
    /*
     * Don't call u_sync() while getting the expression,
     * evaluating it or giving an error message for it!
     */
    ++no_u_sync;
    if (regname == '=')
    {
#ifdef USE_IM_CONTROL
	int	im_on = im_get_status();
#endif
	regname = get_expr_register();
#ifdef USE_IM_CONTROL
	/* Restore the Input Method. */
	if (im_on)
	    im_set_active(TRUE);
#endif
    }
    if (regname == NUL)
	need_redraw = TRUE;	/* remove the '"' */
    else
    {
#endif
	if (literally == Ctrl_O || literally == Ctrl_P)
	{
	    /* Append the command to the redo buffer. */
	    AppendCharToRedobuff(Ctrl_R);
	    AppendCharToRedobuff(literally);
	    AppendCharToRedobuff(regname);

	    do_put(regname, BACKWARD, 1L,
		 (literally == Ctrl_P ? PUT_FIXINDENT : 0) | PUT_CURSEND);
	}
	else if (insert_reg(regname, literally) == FAIL)
	{
	    vim_beep();
	    need_redraw = TRUE;	/* remove the '"' */
	}
#ifdef FEAT_EVAL
    }
    --no_u_sync;
#endif
#ifdef FEAT_CMDL_INFO
    clear_showcmd();
#endif

    /* If the inserted register is empty, we need to remove the '"' */
    if (need_redraw || stuff_empty())
	edit_unputchar();
}

/*
 * CTRL-G commands in Insert mode.
 */
    static void
ins_ctrl_g()
{
    int		c;

#ifdef FEAT_INS_EXPAND
    /* Right after CTRL-X the cursor will be after the ruler. */
    setcursor();
#endif

    /*
     * Don't map the second key. This also prevents the mode message to be
     * deleted when ESC is hit.
     */
    ++no_mapping;
    c = safe_vgetc();
    --no_mapping;
    switch (c)
    {
	/* CTRL-G k and CTRL-G <Up>: cursor up to Insstart.col */
	case K_UP:
	case Ctrl_K:
	case 'k': ins_up(TRUE);
		  break;

	/* CTRL-G j and CTRL-G <Down>: cursor down to Insstart.col */
	case K_DOWN:
	case Ctrl_J:
	case 'j': ins_down(TRUE);
		  break;

	/* CTRL-G u: start new undoable edit */
	case 'u': u_sync();
		  ins_need_undo = TRUE;
		  break;

	/* Unknown CTRL-G command, reserved for future expansion. */
	default:  vim_beep();
    }
}

/*
 * Handle ESC in insert mode.
 * Returns TRUE when leaving insert mode, FALSE when going to repeat the
 * insert.
 */
    static int
ins_esc(count, cmdchar)
    long	*count;
    int		cmdchar;
{
    int		temp;
    static int	disabled_redraw = FALSE;

#if defined(FEAT_HANGULIN)
# if defined(ESC_CHG_TO_ENG_MODE)
    hangul_input_state_set(0);
# endif
    if (composing_hangul)
    {
	push_raw_key(composing_hangul_buffer, 2);
	composing_hangul = 0;
    }
#endif
#if defined(FEAT_MBYTE) && defined(MACOS_CLASSIC)
    previous_script = GetScriptManagerVariable(smKeyScript);
    KeyScript(smKeyRoman); /* or smKeySysScript */
#endif

    temp = curwin->w_cursor.col;
    if (disabled_redraw)
    {
	--RedrawingDisabled;
	disabled_redraw = FALSE;
    }
    if (!arrow_used)
    {
	/*
	 * Don't append the ESC for "r<CR>" and "grx".
	 */
	if (cmdchar != 'r' && cmdchar != 'v')
	    AppendToRedobuff(ESC_STR);

	/*
	 * Repeating insert may take a long time.  Check for
	 * interrupt now and then.
	 */
	if (*count > 0)
	{
	    line_breakcheck();
	    if (got_int)
		*count = 0;
	}

	if (--*count > 0)	/* repeat what was typed */
	{
	    (void)start_redo_ins();
	    if (cmdchar == 'r' || cmdchar == 'v')
		stuffReadbuff(ESC_STR);	/* no ESC in redo buffer */
	    ++RedrawingDisabled;
	    disabled_redraw = TRUE;
	    return FALSE;	/* repeat the insert */
	}
	stop_insert(&curwin->w_cursor, TRUE);
	undisplay_dollar();
    }

    /* When an autoindent was removed, curswant stays after the
     * indent */
    if (restart_edit == NUL && (colnr_T)temp == curwin->w_cursor.col)
	curwin->w_set_curswant = TRUE;

    /* Remember the last Insert position in the '^ mark. */
    if (!cmdmod.keepjumps)
	curbuf->b_last_insert = curwin->w_cursor;

    /*
     * The cursor should end up on the last inserted character.
     */
    if ((curwin->w_cursor.col != 0
#ifdef FEAT_VIRTUALEDIT
		|| curwin->w_cursor.coladd > 0
#endif
	) && (restart_edit == NUL
		|| (gchar_cursor() == NUL
#ifdef FEAT_VISUAL
		    && !VIsual_active
#endif
		    ))
#ifdef FEAT_RIGHTLEFT
	    && !revins_on
#endif
				      )
    {
#ifdef FEAT_VIRTUALEDIT
	if (curwin->w_cursor.coladd > 0 || ve_flags == VE_ALL)
	{
	    oneleft();
	    if (restart_edit != NUL)
		++curwin->w_cursor.coladd;
	}
	else
#endif
	{
	    --curwin->w_cursor.col;
#ifdef FEAT_MBYTE
	    /* Correct cursor for multi-byte character. */
	    if (has_mbyte)
		mb_adjust_cursor();
#endif
	}
    }

#ifdef USE_IM_CONTROL
    /* Disable IM to allow typing English directly for Normal mode commands.
     * When ":lmap" is enabled don't change 'iminsert' (IM can be enabled as
     * well). */
    if (!(State & LANGMAP))
	im_save_status(&curbuf->b_p_iminsert);
    im_set_active(FALSE);
#endif

    State = NORMAL;
    /* need to position cursor again (e.g. when on a TAB ) */
    changed_cline_bef_curs();

#ifdef FEAT_MOUSE
    setmouse();
#endif
#ifdef CURSOR_SHAPE
    ui_cursor_shape();		/* may show different cursor shape */
#endif

    /*
     * When recording or for CTRL-O, need to display the new mode.
     * Otherwise remove the mode message.
     */
    if (Recording || restart_edit != NUL)
	showmode();
    else if (p_smd)
	MSG("");

    return TRUE;	    /* exit Insert mode */
}

#ifdef FEAT_RIGHTLEFT
/*
 * Toggle language: hkmap and revins_on.
 * Move to end of reverse inserted text.
 */
    static void
ins_ctrl_()
{
    if (revins_on && revins_chars && revins_scol >= 0)
    {
	while (gchar_cursor() != NUL && revins_chars--)
	    ++curwin->w_cursor.col;
    }
    p_ri = !p_ri;
    revins_on = (State == INSERT && p_ri);
    if (revins_on)
    {
	revins_scol = curwin->w_cursor.col;
	revins_legal++;
	revins_chars = 0;
	undisplay_dollar();
    }
    else
	revins_scol = -1;
#ifdef FEAT_FKMAP
    if (p_altkeymap)
    {
	/*
	 * to be consistent also for redo command, using '.'
	 * set arrow_used to true and stop it - causing to redo
	 * characters entered in one mode (normal/reverse insert).
	 */
	arrow_used = TRUE;
	(void)stop_arrow();
	p_fkmap = curwin->w_p_rl ^ p_ri;
	if (p_fkmap && p_ri)
	    State = INSERT;
    }
    else
#endif
	p_hkmap = curwin->w_p_rl ^ p_ri;    /* be consistent! */
    showmode();
}
#endif

#ifdef FEAT_VISUAL
/*
 * If 'keymodel' contains "startsel", may start selection.
 * Returns TRUE when a CTRL-O and other keys stuffed.
 */
    static int
ins_start_select(c)
    int		c;
{
    if (km_startsel)
	switch (c)
	{
	    case K_KHOME:
	    case K_XHOME:
	    case K_KEND:
	    case K_XEND:
	    case K_PAGEUP:
	    case K_KPAGEUP:
	    case K_PAGEDOWN:
	    case K_KPAGEDOWN:
# ifdef MACOS
	    case K_LEFT:
	    case K_RIGHT:
	    case K_UP:
	    case K_DOWN:
	    case K_END:
	    case K_HOME:
# endif
		if (!(mod_mask & MOD_MASK_SHIFT))
		    break;
		/* FALLTHROUGH */
	    case K_S_LEFT:
	    case K_S_RIGHT:
	    case K_S_UP:
	    case K_S_DOWN:
	    case K_S_END:
	    case K_S_HOME:
		/* Start selection right away, the cursor can move with
		 * CTRL-O when beyond the end of the line. */
		start_selection();

		/* Execute the key in (insert) Select mode. */
		stuffcharReadbuff(Ctrl_O);
		if (mod_mask)
		{
		    char_u	    buf[4];

		    buf[0] = K_SPECIAL;
		    buf[1] = KS_MODIFIER;
		    buf[2] = mod_mask;
		    buf[3] = NUL;
		    stuffReadbuff(buf);
		}
		stuffcharReadbuff(c);
		return TRUE;
	}
    return FALSE;
}
#endif

/*
 * If the cursor is on an indent, ^T/^D insert/delete one
 * shiftwidth.	Otherwise ^T/^D behave like a "<<" or ">>".
 * Always round the indent to 'shiftwith', this is compatible
 * with vi.  But vi only supports ^T and ^D after an
 * autoindent, we support it everywhere.
 */
    static void
ins_shift(c, lastc)
    int	    c;
    int	    lastc;
{
    if (stop_arrow() == FAIL)
	return;
    AppendCharToRedobuff(c);

    /*
     * 0^D and ^^D: remove all indent.
     */
    if ((lastc == '0' || lastc == '^') && curwin->w_cursor.col)
    {
	--curwin->w_cursor.col;
	(void)del_char(FALSE);		/* delete the '^' or '0' */
	/* In Replace mode, restore the characters that '^' or '0' replaced. */
	if (State & REPLACE_FLAG)
	    replace_pop_ins();
	if (lastc == '^')
	    old_indent = get_indent();	/* remember curr. indent */
	change_indent(INDENT_SET, 0, TRUE, 0);
    }
    else
	change_indent(c == Ctrl_D ? INDENT_DEC : INDENT_INC, 0, TRUE, 0);

    if (did_ai && *skipwhite(ml_get_curline()) != NUL)
	did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
#ifdef FEAT_CINDENT
    can_cindent = FALSE;	/* no cindenting after ^D or ^T */
#endif
}

    static void
ins_del()
{
    int	    temp;

    if (stop_arrow() == FAIL)
	return;
    if (gchar_cursor() == NUL)		/* delete newline */
    {
	temp = curwin->w_cursor.col;
	if (!can_bs(BS_EOL)		/* only if "eol" included */
		|| u_save((linenr_T)(curwin->w_cursor.lnum - 1),
		    (linenr_T)(curwin->w_cursor.lnum + 2)) == FAIL
		|| do_join(FALSE) == FAIL)
	    vim_beep();
	else
	    curwin->w_cursor.col = temp;
    }
    else if (del_char(FALSE) == FAIL)	/* delete char under cursor */
	vim_beep();
    did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
    AppendCharToRedobuff(K_DEL);
}

/*
 * Handle Backspace, delete-word and delete-line in Insert mode.
 * Return TRUE when backspace was actually used.
 */
    static int
ins_bs(c, mode, inserted_space_p)
    int		c;
    int		mode;
    int		*inserted_space_p;
{
    linenr_T	lnum;
    int		cc;
    int		temp = 0;	    /* init for GCC */
    colnr_T	mincol;
    int		did_backspace = FALSE;
    int		in_indent;
    int		oldState;
#ifdef FEAT_MBYTE
    int		p1, p2;
#endif

    /*
     * can't delete anything in an empty file
     * can't backup past first character in buffer
     * can't backup past starting point unless 'backspace' > 1
     * can backup to a previous line if 'backspace' == 0
     */
    if (       bufempty()
	    || (
#ifdef FEAT_RIGHTLEFT
		!revins_on &&
#endif
		((curwin->w_cursor.lnum == 1 && curwin->w_cursor.col == 0)
		    || (!can_bs(BS_START)
			&& (arrow_used
			    || (curwin->w_cursor.lnum == Insstart.lnum
				&& curwin->w_cursor.col <= Insstart.col)))
		    || (!can_bs(BS_INDENT) && !arrow_used && ai_col > 0
					 && curwin->w_cursor.col <= ai_col)
		    || (!can_bs(BS_EOL) && curwin->w_cursor.col == 0))))
    {
	vim_beep();
	return FALSE;
    }

    if (stop_arrow() == FAIL)
	return FALSE;
    in_indent = inindent(0);
#ifdef FEAT_CINDENT
    if (in_indent)
	can_cindent = FALSE;
#endif
#ifdef FEAT_COMMENTS
    end_comment_pending = NUL;	/* After BS, don't auto-end comment */
#endif
#ifdef FEAT_RIGHTLEFT
    if (revins_on)	    /* put cursor after last inserted char */
	inc_cursor();
#endif

#ifdef FEAT_VIRTUALEDIT
    /* Virtualedit:
     *	BACKSPACE_CHAR eats a virtual space
     *	BACKSPACE_WORD eats all coladd
     *	BACKSPACE_LINE eats all coladd and keeps going
     */
    if (curwin->w_cursor.coladd > 0)
    {
	if (mode == BACKSPACE_CHAR)
	{
	    --curwin->w_cursor.coladd;
	    return TRUE;
	}
	if (mode == BACKSPACE_WORD)
	{
	    curwin->w_cursor.coladd = 0;
	    return TRUE;
	}
	curwin->w_cursor.coladd = 0;
    }
#endif

    /*
     * delete newline!
     */
    if (curwin->w_cursor.col == 0)
    {
	lnum = Insstart.lnum;
	if (curwin->w_cursor.lnum == Insstart.lnum
#ifdef FEAT_RIGHTLEFT
			|| revins_on
#endif
				    )
	{
	    if (u_save((linenr_T)(curwin->w_cursor.lnum - 2),
			       (linenr_T)(curwin->w_cursor.lnum + 1)) == FAIL)
		return FALSE;
	    --Insstart.lnum;
	    Insstart.col = MAXCOL;
	}
	/*
	 * In replace mode:
	 * cc < 0: NL was inserted, delete it
	 * cc >= 0: NL was replaced, put original characters back
	 */
	cc = -1;
	if (State & REPLACE_FLAG)
	    cc = replace_pop();	    /* returns -1 if NL was inserted */
	/*
	 * In replace mode, in the line we started replacing, we only move the
	 * cursor.
	 */
	if ((State & REPLACE_FLAG) && curwin->w_cursor.lnum <= lnum)
	{
	    dec_cursor();
	}
	else
	{
#ifdef FEAT_VREPLACE
	    if (!(State & VREPLACE_FLAG)
				   || curwin->w_cursor.lnum > orig_line_count)
#endif
	    {
		temp = gchar_cursor();	/* remember current char */
		--curwin->w_cursor.lnum;
		(void)do_join(FALSE);
		if (temp == NUL && gchar_cursor() != NUL)
		    inc_cursor();
	    }
#ifdef FEAT_VREPLACE
	    else
		dec_cursor();
#endif

	    /*
	     * In REPLACE mode we have to put back the text that was replaced
	     * by the NL. On the replace stack is first a NUL-terminated
	     * sequence of characters that were deleted and then the
	     * characters that NL replaced.
	     */
	    if (State & REPLACE_FLAG)
	    {
		/*
		 * Do the next ins_char() in NORMAL state, to
		 * prevent ins_char() from replacing characters and
		 * avoiding showmatch().
		 */
		oldState = State;
		State = NORMAL;
		/*
		 * restore characters (blanks) deleted after cursor
		 */
		while (cc > 0)
		{
		    temp = curwin->w_cursor.col;
#ifdef FEAT_MBYTE
		    mb_replace_pop_ins(cc);
#else
		    ins_char(cc);
#endif
		    curwin->w_cursor.col = temp;
		    cc = replace_pop();
		}
		/* restore the characters that NL replaced */
		replace_pop_ins();
		State = oldState;
	    }
	}
	did_ai = FALSE;
    }
    else
    {
	/*
	 * Delete character(s) before the cursor.
	 */
#ifdef FEAT_RIGHTLEFT
	if (revins_on)		/* put cursor on last inserted char */
	    dec_cursor();
#endif
	mincol = 0;
						/* keep indent */
	if (mode == BACKSPACE_LINE && curbuf->b_p_ai
#ifdef FEAT_RIGHTLEFT
		&& !revins_on
#endif
			    )
	{
	    temp = curwin->w_cursor.col;
	    beginline(BL_WHITE);
	    if (curwin->w_cursor.col < (colnr_T)temp)
		mincol = curwin->w_cursor.col;
	    curwin->w_cursor.col = temp;
	}

	/*
	 * Handle deleting one 'shiftwidth' or 'softtabstop'.
	 */
	if (	   mode == BACKSPACE_CHAR
		&& ((p_sta && in_indent)
		    || (curbuf->b_p_sts
			&& (*(ml_get_cursor() - 1) == TAB
			    || (*(ml_get_cursor() - 1) == ' '
				&& (!*inserted_space_p
				    || arrow_used))))))
	{
	    int		ts;
	    colnr_T	vcol;
	    colnr_T	want_vcol;
	    int		extra = 0;

	    *inserted_space_p = FALSE;
	    if (p_sta)
		ts = curbuf->b_p_sw;
	    else
		ts = curbuf->b_p_sts;
	    /* Compute the virtual column where we want to be.  Since
	     * 'showbreak' may get in the way, need to get the last column of
	     * the previous character. */
	    getvcol(curwin, &curwin->w_cursor, &vcol, NULL, NULL);
	    dec_cursor();
	    getvcol(curwin, &curwin->w_cursor, NULL, NULL, &want_vcol);
	    inc_cursor();
	    want_vcol = (want_vcol / ts) * ts;

	    /* delete characters until we are at or before want_vcol */
	    while (vcol > want_vcol
		    && (cc = *(ml_get_cursor() - 1), vim_iswhite(cc)))
	    {
		dec_cursor();
		getvcol(curwin, &curwin->w_cursor, &vcol, NULL, NULL);
		if (State & REPLACE_FLAG)
		{
		    /* Don't delete characters before the insert point when in
		     * Replace mode */
		    if (curwin->w_cursor.lnum != Insstart.lnum
			    || curwin->w_cursor.col >= Insstart.col)
		    {
#if 0	/* what was this for?  It causes problems when sw != ts. */
			if (State == REPLACE && (int)vcol < want_vcol)
			{
			    (void)del_char(FALSE);
			    extra = 2;	/* don't pop too much */
			}
			else
#endif
			    replace_do_bs();
		    }
		}
		else
		    (void)del_char(FALSE);
	    }

	    /* insert extra spaces until we are at want_vcol */
	    while (vcol < want_vcol)
	    {
		/* Remember the first char we inserted */
		if (curwin->w_cursor.lnum == Insstart.lnum
				   && curwin->w_cursor.col < Insstart.col)
		    Insstart.col = curwin->w_cursor.col;

#ifdef FEAT_VREPLACE
		if (State & VREPLACE_FLAG)
		    ins_char(' ');
		else
#endif
		{
		    ins_str((char_u *)" ");
		    if ((State & REPLACE_FLAG) && extra <= 1)
		    {
			if (extra)
			    replace_push_off(NUL);
			else
			    replace_push(NUL);
		    }
		    if (extra == 2)
			extra = 1;
		}
		getvcol(curwin, &curwin->w_cursor, &vcol, NULL, NULL);
	    }
	}

	/*
	 * Delete upto starting point, start of line or previous word.
	 */
	else do
	{
#ifdef FEAT_RIGHTLEFT
	    if (!revins_on) /* put cursor on char to be deleted */
#endif
		dec_cursor();

	    /* start of word? */
	    if (mode == BACKSPACE_WORD && !vim_isspace(gchar_cursor()))
	    {
		mode = BACKSPACE_WORD_NOT_SPACE;
		temp = vim_iswordc(gchar_cursor());
	    }
	    /* end of word? */
	    else if (mode == BACKSPACE_WORD_NOT_SPACE
		    && (vim_isspace(cc = gchar_cursor())
			    || vim_iswordc(cc) != temp))
	    {
#ifdef FEAT_RIGHTLEFT
		if (!revins_on)
#endif
		    inc_cursor();
#ifdef FEAT_RIGHTLEFT
		else if (State & REPLACE_FLAG)
		    dec_cursor();
#endif
		break;
	    }
	    if (State & REPLACE_FLAG)
		replace_do_bs();
	    else
	    {
#ifdef FEAT_MBYTE
		if (enc_utf8 && p_deco)
		    (void)utfc_ptr2char(ml_get_cursor(), &p1, &p2);
#endif
		(void)del_char(FALSE);
#ifdef FEAT_MBYTE
		/*
		 * If p1 or p2 is non-zero, there are combining characters we
		 * need to take account of.  Don't back up before the base
		 * character.
		 */
		if (enc_utf8 && p_deco && (p1 != NUL || p2 != NUL))
		    inc_cursor();
#endif
#ifdef FEAT_RIGHTLEFT
		if (revins_chars)
		{
		    revins_chars--;
		    revins_legal++;
		}
		if (revins_on && gchar_cursor() == NUL)
		    break;
#endif
	    }
	    /* Just a single backspace?: */
	    if (mode == BACKSPACE_CHAR)
		break;
	} while (
#ifdef FEAT_RIGHTLEFT
		revins_on ||
#endif
		(curwin->w_cursor.col > mincol
		 && (curwin->w_cursor.lnum != Insstart.lnum
		     || curwin->w_cursor.col != Insstart.col)));
	did_backspace = TRUE;
    }
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
    if (curwin->w_cursor.col <= 1)
	did_ai = FALSE;
    /*
     * It's a little strange to put backspaces into the redo
     * buffer, but it makes auto-indent a lot easier to deal
     * with.
     */
    AppendCharToRedobuff(c);

    /* If deleted before the insertion point, adjust it */
    if (curwin->w_cursor.lnum == Insstart.lnum
				       && curwin->w_cursor.col < Insstart.col)
	Insstart.col = curwin->w_cursor.col;

    /* vi behaviour: the cursor moves backward but the character that
     *		     was there remains visible
     * Vim behaviour: the cursor moves backward and the character that
     *		      was there is erased from the screen.
     * We can emulate the vi behaviour by pretending there is a dollar
     * displayed even when there isn't.
     *  --pkv Sun Jan 19 01:56:40 EST 2003 */
    if (vim_strchr(p_cpo, CPO_BACKSPACE) != NULL && dollar_vcol == 0)
	dollar_vcol = curwin->w_virtcol;

    return did_backspace;
}

#ifdef FEAT_MOUSE
    static void
ins_mouse(c)
    int	    c;
{
    pos_T	tpos;

# ifdef FEAT_GUI
    /* When GUI is active, also move/paste when 'mouse' is empty */
    if (!gui.in_use)
# endif
	if (!mouse_has(MOUSE_INSERT))
	    return;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (do_mouse(NULL, c, BACKWARD, 1L, 0))
    {
	start_arrow(&tpos);
# ifdef FEAT_CINDENT
	can_cindent = TRUE;
# endif
    }

#ifdef FEAT_WINDOWS
    /* redraw status lines (in case another window became active) */
    redraw_statuslines();
#endif
}

    static void
ins_mousescroll(up)
    int		up;
{
    pos_T	tpos;
# if defined(FEAT_GUI) && defined(FEAT_WINDOWS)
    win_T	*old_curwin;
# endif

    tpos = curwin->w_cursor;

# if defined(FEAT_GUI) && defined(FEAT_WINDOWS)
    old_curwin = curwin;

    /* Currently the mouse coordinates are only known in the GUI. */
    if (gui.in_use && mouse_row >= 0 && mouse_col >= 0)
    {
	int row, col;

	row = mouse_row;
	col = mouse_col;

	/* find the window at the pointer coordinates */
	curwin = mouse_find_win(&row, &col);
	curbuf = curwin->w_buffer;
    }
    if (curwin == old_curwin)
# endif
	undisplay_dollar();

    if (mod_mask & (MOD_MASK_SHIFT | MOD_MASK_CTRL))
	scroll_redraw(up, (long)(curwin->w_botline - curwin->w_topline));
    else
	scroll_redraw(up, 3L);

# if defined(FEAT_GUI) && defined(FEAT_WINDOWS)
    curwin->w_redr_status = TRUE;

    curwin = old_curwin;
    curbuf = curwin->w_buffer;
# endif

    if (!equalpos(curwin->w_cursor, tpos))
    {
	start_arrow(&tpos);
# ifdef FEAT_CINDENT
	can_cindent = TRUE;
# endif
    }
}
#endif

#ifdef FEAT_GUI
    void
ins_scroll()
{
    pos_T	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (gui_do_scroll())
    {
	start_arrow(&tpos);
# ifdef FEAT_CINDENT
	can_cindent = TRUE;
# endif
    }
}

    void
ins_horscroll()
{
    pos_T	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (gui_do_horiz_scroll())
    {
	start_arrow(&tpos);
# ifdef FEAT_CINDENT
	can_cindent = TRUE;
# endif
    }
}
#endif

    static void
ins_left()
{
    pos_T	tpos;

#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_HOR) && KeyTyped)
	foldOpenCursor();
#endif
    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (oneleft() == OK)
    {
	start_arrow(&tpos);
#ifdef FEAT_RIGHTLEFT
	/* If exit reversed string, position is fixed */
	if (revins_scol != -1 && (int)curwin->w_cursor.col >= revins_scol)
	    revins_legal++;
	revins_chars++;
#endif
    }

    /*
     * if 'whichwrap' set for cursor in insert mode may go to
     * previous line
     */
    else if (vim_strchr(p_ww, '[') != NULL && curwin->w_cursor.lnum > 1)
    {
	start_arrow(&tpos);
	--(curwin->w_cursor.lnum);
	coladvance((colnr_T)MAXCOL);
	curwin->w_set_curswant = TRUE;	/* so we stay at the end */
    }
    else
	vim_beep();
}

    static void
ins_home(c)
    int		c;
{
    pos_T	tpos;

#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_HOR) && KeyTyped)
	foldOpenCursor();
#endif
    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (c == K_C_HOME)
	curwin->w_cursor.lnum = 1;
    curwin->w_cursor.col = 0;
#ifdef FEAT_VIRTUALEDIT
    curwin->w_cursor.coladd = 0;
#endif
    curwin->w_curswant = 0;
    start_arrow(&tpos);
}

    static void
ins_end(c)
    int		c;
{
    pos_T	tpos;

#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_HOR) && KeyTyped)
	foldOpenCursor();
#endif
    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (c == K_C_END)
	curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
    coladvance((colnr_T)MAXCOL);
    curwin->w_curswant = MAXCOL;

    start_arrow(&tpos);
}

    static void
ins_s_left()
{
#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_HOR) && KeyTyped)
	foldOpenCursor();
#endif
    undisplay_dollar();
    if (curwin->w_cursor.lnum > 1 || curwin->w_cursor.col > 0)
    {
	start_arrow(&curwin->w_cursor);
	(void)bck_word(1L, FALSE, FALSE);
	curwin->w_set_curswant = TRUE;
    }
    else
	vim_beep();
}

    static void
ins_right()
{
#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_HOR) && KeyTyped)
	foldOpenCursor();
#endif
    undisplay_dollar();
    if (gchar_cursor() != NUL || virtual_active()
	    )
    {
	start_arrow(&curwin->w_cursor);
	curwin->w_set_curswant = TRUE;
#ifdef FEAT_VIRTUALEDIT
	if (virtual_active())
	    oneright();
	else
#endif
	{
#ifdef FEAT_MBYTE
	    if (has_mbyte)
		curwin->w_cursor.col += (*mb_ptr2len_check)(ml_get_cursor());
	    else
#endif
		++curwin->w_cursor.col;
	}

#ifdef FEAT_RIGHTLEFT
	revins_legal++;
	if (revins_chars)
	    revins_chars--;
#endif
    }
    /* if 'whichwrap' set for cursor in insert mode, may move the
     * cursor to the next line */
    else if (vim_strchr(p_ww, ']') != NULL
	    && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
    {
	start_arrow(&curwin->w_cursor);
	curwin->w_set_curswant = TRUE;
	++curwin->w_cursor.lnum;
	curwin->w_cursor.col = 0;
    }
    else
	vim_beep();
}

    static void
ins_s_right()
{
#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_HOR) && KeyTyped)
	foldOpenCursor();
#endif
    undisplay_dollar();
    if (curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count
	    || gchar_cursor() != NUL)
    {
	start_arrow(&curwin->w_cursor);
	(void)fwd_word(1L, FALSE, 0);
	curwin->w_set_curswant = TRUE;
    }
    else
	vim_beep();
}

    static void
ins_up(startcol)
    int		startcol;	/* when TRUE move to Insstart.col */
{
    pos_T	tpos;
    linenr_T	old_topline = curwin->w_topline;
#ifdef FEAT_DIFF
    int		old_topfill = curwin->w_topfill;
#endif

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (cursor_up(1L, TRUE) == OK)
    {
	if (startcol)
	    coladvance(getvcol_nolist(&Insstart));
	if (old_topline != curwin->w_topline
#ifdef FEAT_DIFF
		|| old_topfill != curwin->w_topfill
#endif
		)
	    redraw_later(VALID);
	start_arrow(&tpos);
#ifdef FEAT_CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

    static void
ins_pageup()
{
    pos_T	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (onepage(BACKWARD, 1L) == OK)
    {
	start_arrow(&tpos);
#ifdef FEAT_CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

    static void
ins_down(startcol)
    int		startcol;	/* when TRUE move to Insstart.col */
{
    pos_T	tpos;
    linenr_T	old_topline = curwin->w_topline;
#ifdef FEAT_DIFF
    int		old_topfill = curwin->w_topfill;
#endif

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (cursor_down(1L, TRUE) == OK)
    {
	if (startcol)
	    coladvance(getvcol_nolist(&Insstart));
	if (old_topline != curwin->w_topline
#ifdef FEAT_DIFF
		|| old_topfill != curwin->w_topfill
#endif
		)
	    redraw_later(VALID);
	start_arrow(&tpos);
#ifdef FEAT_CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

    static void
ins_pagedown()
{
    pos_T	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (onepage(FORWARD, 1L) == OK)
    {
	start_arrow(&tpos);
#ifdef FEAT_CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

#ifdef FEAT_DND
    static void
ins_drop()
{
    do_put('~', BACKWARD, 1L, PUT_CURSEND);
}
#endif

/*
 * Handle TAB in Insert or Replace mode.
 * Return TRUE when the TAB needs to be inserted like a normal character.
 */
    static int
ins_tab()
{
    int		ind;
    int		i;
    int		temp;

    if (Insstart_blank_vcol == MAXCOL && curwin->w_cursor.lnum == Insstart.lnum)
	Insstart_blank_vcol = get_nolist_virtcol();
    if (echeck_abbr(TAB + ABBR_OFF))
	return FALSE;

    ind = inindent(0);
#ifdef FEAT_CINDENT
    if (ind)
	can_cindent = FALSE;
#endif

    /*
     * When nothing special, insert TAB like a normal character
     */
    if (!curbuf->b_p_et
	    && !(p_sta && ind && curbuf->b_p_ts != curbuf->b_p_sw)
	    && curbuf->b_p_sts == 0)
	return TRUE;

    if (stop_arrow() == FAIL)
	return TRUE;

    did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
    AppendToRedobuff((char_u *)"\t");

    if (p_sta && ind)		/* insert tab in indent, use 'shiftwidth' */
	temp = (int)curbuf->b_p_sw;
    else if (curbuf->b_p_sts > 0) /* use 'softtabstop' when set */
	temp = (int)curbuf->b_p_sts;
    else			/* otherwise use 'tabstop' */
	temp = (int)curbuf->b_p_ts;
    temp -= get_nolist_virtcol() % temp;

    /*
     * Insert the first space with ins_char().	It will delete one char in
     * replace mode.  Insert the rest with ins_str(); it will not delete any
     * chars.  For VREPLACE mode, we use ins_char() for all characters.
     */
    ins_char(' ');
    while (--temp > 0)
    {
#ifdef FEAT_VREPLACE
	if (State & VREPLACE_FLAG)
	    ins_char(' ');
	else
#endif
	{
	    ins_str((char_u *)" ");
	    if (State & REPLACE_FLAG)	    /* no char replaced */
		replace_push(NUL);
	}
    }

    /*
     * When 'expandtab' not set: Replace spaces by TABs where possible.
     */
    if (!curbuf->b_p_et && (curbuf->b_p_sts || (p_sta && ind)))
    {
	char_u		*ptr;
#ifdef FEAT_VREPLACE
	char_u		*saved_line = NULL;	/* init for GCC */
	pos_T		pos;
#endif
	pos_T		fpos;
	pos_T		*cursor;
	colnr_T		want_vcol, vcol;
	int		change_col = -1;
	int		save_list = curwin->w_p_list;

	/*
	 * Get the current line.  For VREPLACE mode, don't make real changes
	 * yet, just work on a copy of the line.
	 */
#ifdef FEAT_VREPLACE
	if (State & VREPLACE_FLAG)
	{
	    pos = curwin->w_cursor;
	    cursor = &pos;
	    saved_line = vim_strsave(ml_get_curline());
	    if (saved_line == NULL)
		return FALSE;
	    ptr = saved_line + pos.col;
	}
	else
#endif
	{
	    ptr = ml_get_cursor();
	    cursor = &curwin->w_cursor;
	}

	/* When 'L' is not in 'cpoptions' a tab always takes up 'ts' spaces. */
	if (vim_strchr(p_cpo, CPO_LISTWM) == NULL)
	    curwin->w_p_list = FALSE;

	/* Find first white before the cursor */
	fpos = curwin->w_cursor;
	while (fpos.col > 0 && vim_iswhite(ptr[-1]))
	{
	    --fpos.col;
	    --ptr;
	}

	/* In Replace mode, don't change characters before the insert point. */
	if ((State & REPLACE_FLAG)
		&& fpos.lnum == Insstart.lnum
		&& fpos.col < Insstart.col)
	{
	    ptr += Insstart.col - fpos.col;
	    fpos.col = Insstart.col;
	}

	/* compute virtual column numbers of first white and cursor */
	getvcol(curwin, &fpos, &vcol, NULL, NULL);
	getvcol(curwin, cursor, &want_vcol, NULL, NULL);

	/* Use as many TABs as possible.  Beware of 'showbreak' and
	 * 'linebreak' adding extra virtual columns. */
	while (vim_iswhite(*ptr))
	{
	    i = lbr_chartabsize((char_u *)"\t", vcol);
	    if (vcol + i > want_vcol)
		break;
	    if (*ptr != TAB)
	    {
		*ptr = TAB;
		if (change_col < 0)
		{
		    change_col = fpos.col;  /* Column of first change */
		    /* May have to adjust Insstart */
		    if (fpos.lnum == Insstart.lnum && fpos.col < Insstart.col)
			Insstart.col = fpos.col;
		}
	    }
	    ++fpos.col;
	    ++ptr;
	    vcol += i;
	}

	if (change_col >= 0)
	{
	    int repl_off = 0;

	    /* Skip over the spaces we need. */
	    while (vcol < want_vcol && *ptr == ' ')
	    {
		vcol += lbr_chartabsize(ptr, vcol);
		++ptr;
		++repl_off;
	    }
	    if (vcol > want_vcol)
	    {
		/* Must have a char with 'showbreak' just before it. */
		--ptr;
		--repl_off;
	    }
	    fpos.col += repl_off;

	    /* Delete following spaces. */
	    i = cursor->col - fpos.col;
	    if (i > 0)
	    {
		mch_memmove(ptr, ptr + i, STRLEN(ptr + i) + 1);
		/* correct replace stack. */
		if ((State & REPLACE_FLAG)
#ifdef FEAT_VREPLACE
			&& !(State & VREPLACE_FLAG)
#endif
			)
		    for (temp = i; --temp >= 0; )
			replace_join(repl_off);
	    }
	    cursor->col -= i;

#ifdef FEAT_VREPLACE
	    /*
	     * In VREPLACE mode, we haven't changed anything yet.  Do it now by
	     * backspacing over the changed spacing and then inserting the new
	     * spacing.
	     */
	    if (State & VREPLACE_FLAG)
	    {
		/* Backspace from real cursor to change_col */
		backspace_until_column(change_col);

		/* Insert each char in saved_line from changed_col to
		 * ptr-cursor */
		ins_bytes_len(saved_line + change_col,
						    cursor->col - change_col);
	    }
#endif
	}

#ifdef FEAT_VREPLACE
	if (State & VREPLACE_FLAG)
	    vim_free(saved_line);
#endif
	curwin->w_p_list = save_list;
    }

    return FALSE;
}

/*
 * Handle CR or NL in insert mode.
 * Return TRUE when out of memory or can't undo.
 */
    static int
ins_eol(c)
    int		c;
{
    int	    i;

    if (echeck_abbr(c + ABBR_OFF))
	return FALSE;
    if (stop_arrow() == FAIL)
	return TRUE;
    undisplay_dollar();

    /*
     * Strange Vi behaviour: In Replace mode, typing a NL will not delete the
     * character under the cursor.  Only push a NUL on the replace stack,
     * nothing to put back when the NL is deleted.
     */
    if ((State & REPLACE_FLAG)
#ifdef FEAT_VREPLACE
	    && !(State & VREPLACE_FLAG)
#endif
	    )
	replace_push(NUL);

    /*
     * In VREPLACE mode, a NL replaces the rest of the line, and starts
     * replacing the next line, so we push all of the characters left on the
     * line onto the replace stack.  This is not done here though, it is done
     * in open_line().
     */

#ifdef FEAT_RIGHTLEFT
# ifdef FEAT_FKMAP
    if (p_altkeymap && p_fkmap)
	fkmap(NL);
# endif
    /* NL in reverse insert will always start in the end of
     * current line. */
    if (revins_on)
	curwin->w_cursor.col += (colnr_T)STRLEN(ml_get_cursor());
#endif

    AppendToRedobuff(NL_STR);
    i = open_line(FORWARD,
#ifdef FEAT_COMMENTS
	    has_format_option(FO_RET_COMS) ? OPENLINE_DO_COM :
#endif
	    0, old_indent);
    old_indent = 0;
#ifdef FEAT_CINDENT
    can_cindent = TRUE;
#endif

    return (!i);
}

#ifdef FEAT_DIGRAPHS
/*
 * Handle digraph in insert mode.
 * Returns character still to be inserted, or NUL when nothing remaining to be
 * done.
 */
    static int
ins_digraph()
{
    int	    c;
    int	    cc;

    pc_status = PC_STATUS_UNSET;
    if (redrawing() && !char_avail())
    {
	/* may need to redraw when no more chars available now */
	ins_redraw();

	edit_putchar('?', TRUE);
#ifdef FEAT_CMDL_INFO
	add_to_showcmd_c(Ctrl_K);
#endif
    }

#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif

    /* don't map the digraph chars. This also prevents the
     * mode message to be deleted when ESC is hit */
    ++no_mapping;
    ++allow_keys;
    c = safe_vgetc();
    --no_mapping;
    --allow_keys;
    if (IS_SPECIAL(c) || mod_mask)	    /* special key */
    {
#ifdef FEAT_CMDL_INFO
	clear_showcmd();
#endif
	insert_special(c, TRUE, FALSE);
	return NUL;
    }
    if (c != ESC)
    {
	if (redrawing() && !char_avail())
	{
	    /* may need to redraw when no more chars available now */
	    ins_redraw();

	    if (char2cells(c) == 1)
	    {
		/* first remove the '?', otherwise it's restored when typing
		 * an ESC next */
		edit_unputchar();
		ins_redraw();
		edit_putchar(c, TRUE);
	    }
#ifdef FEAT_CMDL_INFO
	    add_to_showcmd_c(c);
#endif
	}
	++no_mapping;
	++allow_keys;
	cc = safe_vgetc();
	--no_mapping;
	--allow_keys;
	if (cc != ESC)
	{
	    AppendToRedobuff((char_u *)CTRL_V_STR);
	    c = getdigraph(c, cc, TRUE);
#ifdef FEAT_CMDL_INFO
	    clear_showcmd();
#endif
	    return c;
	}
    }
    edit_unputchar();
#ifdef FEAT_CMDL_INFO
    clear_showcmd();
#endif
    return NUL;
}
#endif

/*
 * Handle CTRL-E and CTRL-Y in Insert mode: copy char from other line.
 * Returns the char to be inserted, or NUL if none found.
 */
    static int
ins_copychar(lnum)
    linenr_T	lnum;
{
    int	    c;
    int	    temp;
    char_u  *ptr, *prev_ptr;

    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count)
    {
	vim_beep();
	return NUL;
    }

    /* try to advance to the cursor column */
    temp = 0;
    ptr = ml_get(lnum);
    prev_ptr = ptr;
    validate_virtcol();
    while ((colnr_T)temp < curwin->w_virtcol && *ptr != NUL)
    {
	prev_ptr = ptr;
	temp += lbr_chartabsize_adv(&ptr, (colnr_T)temp);
    }
    if ((colnr_T)temp > curwin->w_virtcol)
	ptr = prev_ptr;

#ifdef FEAT_MBYTE
    c = (*mb_ptr2char)(ptr);
#else
    c = *ptr;
#endif
    if (c == NUL)
	vim_beep();
    return c;
}

#ifdef FEAT_SMARTINDENT
/*
 * Try to do some very smart auto-indenting.
 * Used when inserting a "normal" character.
 */
    static void
ins_try_si(c)
    int	    c;
{
    pos_T	*pos, old_pos;
    char_u	*ptr;
    int		i;
    int		temp;

    /*
     * do some very smart indenting when entering '{' or '}'
     */
    if (((did_si || can_si_back) && c == '{') || (can_si && c == '}'))
    {
	/*
	 * for '}' set indent equal to indent of line containing matching '{'
	 */
	if (c == '}' && (pos = findmatch(NULL, '{')) != NULL)
	{
	    old_pos = curwin->w_cursor;
	    /*
	     * If the matching '{' has a ')' immediately before it (ignoring
	     * white-space), then line up with the start of the line
	     * containing the matching '(' if there is one.  This handles the
	     * case where an "if (..\n..) {" statement continues over multiple
	     * lines -- webb
	     */
	    ptr = ml_get(pos->lnum);
	    i = pos->col;
	    if (i > 0)		/* skip blanks before '{' */
		while (--i > 0 && vim_iswhite(ptr[i]))
		    ;
	    curwin->w_cursor.lnum = pos->lnum;
	    curwin->w_cursor.col = i;
	    if (ptr[i] == ')' && (pos = findmatch(NULL, '(')) != NULL)
		curwin->w_cursor = *pos;
	    i = get_indent();
	    curwin->w_cursor = old_pos;
#ifdef FEAT_VREPLACE
	    if (State & VREPLACE_FLAG)
		change_indent(INDENT_SET, i, FALSE, NUL);
	    else
#endif
		(void)set_indent(i, SIN_CHANGED);
	}
	else if (curwin->w_cursor.col > 0)
	{
	    /*
	     * when inserting '{' after "O" reduce indent, but not
	     * more than indent of previous line
	     */
	    temp = TRUE;
	    if (c == '{' && can_si_back && curwin->w_cursor.lnum > 1)
	    {
		old_pos = curwin->w_cursor;
		i = get_indent();
		while (curwin->w_cursor.lnum > 1)
		{
		    ptr = skipwhite(ml_get(--(curwin->w_cursor.lnum)));

		    /* ignore empty lines and lines starting with '#'. */
		    if (*ptr != '#' && *ptr != NUL)
			break;
		}
		if (get_indent() >= i)
		    temp = FALSE;
		curwin->w_cursor = old_pos;
	    }
	    if (temp)
		shift_line(TRUE, FALSE, 1);
	}
    }

    /*
     * set indent of '#' always to 0
     */
    if (curwin->w_cursor.col > 0 && can_si && c == '#')
    {
	/* remember current indent for next line */
	old_indent = get_indent();
	(void)set_indent(0, SIN_CHANGED);
    }

    /* Adjust ai_col, the char at this position can be deleted. */
    if (ai_col > curwin->w_cursor.col)
	ai_col = curwin->w_cursor.col;
}
#endif

/*
 * Get the value that w_virtcol would have when 'list' is off.
 * Unless 'cpo' contains the 'L' flag.
 */
    static colnr_T
get_nolist_virtcol()
{
    if (curwin->w_p_list && vim_strchr(p_cpo, CPO_LISTWM) == NULL)
	return getvcol_nolist(&curwin->w_cursor);
    validate_virtcol();
    return curwin->w_virtcol;
}
