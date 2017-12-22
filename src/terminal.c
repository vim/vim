/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Terminal window support, see ":help :terminal".
 *
 * There are three parts:
 * 1. Generic code for all systems.
 *    Uses libvterm for the terminal emulator.
 * 2. The MS-Windows implementation.
 *    Uses winpty.
 * 3. The Unix-like implementation.
 *    Uses pseudo-tty's (pty's).
 *
 * For each terminal one VTerm is constructed.  This uses libvterm.  A copy of
 * this library is in the libvterm directory.
 *
 * When a terminal window is opened, a job is started that will be connected to
 * the terminal emulator.
 *
 * If the terminal window has keyboard focus, typed keys are converted to the
 * terminal encoding and writing to the job over a channel.
 *
 * If the job produces output, it is written to the terminal emulator.  The
 * terminal emulator invokes callbacks when its screen content changes.  The
 * line range is stored in tl_dirty_row_start and tl_dirty_row_end.  Once in a
 * while, if the terminal window is visible, the screen contents is drawn.
 *
 * When the job ends the text is put in a buffer.  Redrawing then happens from
 * that buffer, attributes come from the scrollback buffer tl_scrollback.
 * When the buffer is changed it is turned into a normal buffer, the attributes
 * in tl_scrollback are no longer used.
 *
 * TODO:
 * - When using 'termguicolors' still use the 16 ANSI colors as-is.  Helps for
 *   a job that uses 16 colors while Vim is using > 256.
 * - in GUI vertical split causes problems.  Cursor is flickering. (Hirohito
 *   Higashi, 2017 Sep 19)
 * - Shift-Tab does not work.
 * - after resizing windows overlap. (Boris Staletic, #2164)
 * - Redirecting output does not work on MS-Windows, Test_terminal_redir_file()
 *   is disabled.
 * - cursor blinks in terminal on widows with a timer. (xtal8, #2142)
 * - When closing gvim with an active terminal buffer, the dialog suggests
 *   saving the buffer.  Should say something else. (Manas Thakur, #2215)
 *   Also: #2223
 * - Termdebug does not work when Vim build with mzscheme.  gdb hangs.
 * - MS-Windows GUI: WinBar has  tearoff item
 * - Adding WinBar to terminal window doesn't display, text isn't shifted down.
 * - MS-Windows GUI: still need to type a key after shell exits?  #1924
 * - After executing a shell command the status line isn't redraw.
 * - What to store in a session file?  Shell at the prompt would be OK to
 *   restore, but others may not.  Open the window and let the user start the
 *   command?
 * - implement term_setsize()
 * - add test for giving error for invalid 'termsize' value.
 * - support minimal size when 'termsize' is "rows*cols".
 * - support minimal size when 'termsize' is empty?
 * - GUI: when using tabs, focus in terminal, click on tab does not work.
 * - GUI: when 'confirm' is set and trying to exit Vim, dialog offers to save
 *   changes to "!shell".
 *   (justrajdeep, 2017 Aug 22)
 * - Redrawing is slow with Athena and Motif.  Also other GUI? (Ramel Eshed)
 * - For the GUI fill termios with default values, perhaps like pangoterm:
 *   http://bazaar.launchpad.net/~leonerd/pangoterm/trunk/view/head:/main.c#L134
 * - if the job in the terminal does not support the mouse, we can use the
 *   mouse in the Terminal window for copy/paste.
 * - when 'encoding' is not utf-8, or the job is using another encoding, setup
 *   conversions.
 * - In the GUI use a terminal emulator for :!cmd.  Make the height the same as
 *   the window and position it higher up when it gets filled, so it looks like
 *   the text scrolls up.
 * - Copy text in the vterm to the Vim buffer once in a while, so that
 *   completion works.
 * - add an optional limit for the scrollback size.  When reaching it remove
 *   10% at the start.
 */

#include "vim.h"

#if defined(FEAT_TERMINAL) || defined(PROTO)

#ifndef MIN
# define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
# define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#include "libvterm/include/vterm.h"

/* This is VTermScreenCell without the characters, thus much smaller. */
typedef struct {
  VTermScreenCellAttrs	attrs;
  char			width;
  VTermColor		fg, bg;
} cellattr_T;

typedef struct sb_line_S {
    int		sb_cols;	/* can differ per line */
    cellattr_T	*sb_cells;	/* allocated */
    cellattr_T	sb_fill_attr;	/* for short line */
} sb_line_T;

/* typedef term_T in structs.h */
struct terminal_S {
    term_T	*tl_next;

    VTerm	*tl_vterm;
    job_T	*tl_job;
    buf_T	*tl_buffer;

    /* Set when setting the size of a vterm, reset after redrawing. */
    int		tl_vterm_size_changed;

    /* used when tl_job is NULL and only a pty was created */
    int		tl_tty_fd;
    char_u	*tl_tty_in;
    char_u	*tl_tty_out;

    int		tl_normal_mode; /* TRUE: Terminal-Normal mode */
    int		tl_channel_closed;
    int		tl_finish;	/* 'c' for ++close, 'o' for ++open */
    char_u	*tl_opencmd;
    char_u	*tl_eof_chars;

#ifdef WIN3264
    void	*tl_winpty_config;
    void	*tl_winpty;
#endif

    /* last known vterm size */
    int		tl_rows;
    int		tl_cols;
    /* vterm size does not follow window size */
    int		tl_rows_fixed;
    int		tl_cols_fixed;

    char_u	*tl_title; /* NULL or allocated */
    char_u	*tl_status_text; /* NULL or allocated */

    /* Range of screen rows to update.  Zero based. */
    int		tl_dirty_row_start; /* MAX_ROW if nothing dirty */
    int		tl_dirty_row_end;   /* row below last one to update */

    garray_T	tl_scrollback;
    int		tl_scrollback_scrolled;
    cellattr_T	tl_default_color;

    VTermPos	tl_cursor_pos;
    int		tl_cursor_visible;
    int		tl_cursor_blink;
    int		tl_cursor_shape;  /* 1: block, 2: underline, 3: bar */
    char_u	*tl_cursor_color; /* NULL or allocated */

    int		tl_using_altscreen;
};

#define TMODE_ONCE 1	    /* CTRL-\ CTRL-N used */
#define TMODE_LOOP 2	    /* CTRL-W N used */

/*
 * List of all active terminals.
 */
static term_T *first_term = NULL;

/* Terminal active in terminal_loop(). */
static term_T *in_terminal_loop = NULL;

#define MAX_ROW 999999	    /* used for tl_dirty_row_end to update all rows */
#define KEY_BUF_LEN 200

/*
 * Functions with separate implementation for MS-Windows and Unix-like systems.
 */
static int term_and_job_init(term_T *term, typval_T *argvar, jobopt_T *opt);
static int create_pty_only(term_T *term, jobopt_T *opt);
static void term_report_winsize(term_T *term, int rows, int cols);
static void term_free_vterm(term_T *term);

/* The character that we know (or assume) that the terminal expects for the
 * backspace key. */
static int term_backspace_char = BS;

/* "Terminal" highlight group colors. */
static int term_default_cterm_fg = -1;
static int term_default_cterm_bg = -1;

/**************************************
 * 1. Generic code for all systems.
 */

/*
 * Determine the terminal size from 'termsize' and the current window.
 * Assumes term->tl_rows and term->tl_cols are zero.
 */
    static void
set_term_and_win_size(term_T *term)
{
    if (*curwin->w_p_tms != NUL)
    {
	char_u *p = vim_strchr(curwin->w_p_tms, 'x') + 1;

	term->tl_rows = atoi((char *)curwin->w_p_tms);
	term->tl_cols = atoi((char *)p);
    }
    if (term->tl_rows == 0)
	term->tl_rows = curwin->w_height;
    else
    {
	win_setheight_win(term->tl_rows, curwin);
	term->tl_rows_fixed = TRUE;
    }
    if (term->tl_cols == 0)
	term->tl_cols = curwin->w_width;
    else
    {
	win_setwidth_win(term->tl_cols, curwin);
	term->tl_cols_fixed = TRUE;
    }
}

/*
 * Initialize job options for a terminal job.
 * Caller may overrule some of them.
 */
    static void
init_job_options(jobopt_T *opt)
{
    clear_job_options(opt);

    opt->jo_mode = MODE_RAW;
    opt->jo_out_mode = MODE_RAW;
    opt->jo_err_mode = MODE_RAW;
    opt->jo_set = JO_MODE | JO_OUT_MODE | JO_ERR_MODE;
}

/*
 * Set job options mandatory for a terminal job.
 */
    static void
setup_job_options(jobopt_T *opt, int rows, int cols)
{
    if (!(opt->jo_set & JO_OUT_IO))
    {
	/* Connect stdout to the terminal. */
	opt->jo_io[PART_OUT] = JIO_BUFFER;
	opt->jo_io_buf[PART_OUT] = curbuf->b_fnum;
	opt->jo_modifiable[PART_OUT] = 0;
	opt->jo_set |= JO_OUT_IO + JO_OUT_BUF + JO_OUT_MODIFIABLE;
    }

    if (!(opt->jo_set & JO_ERR_IO))
    {
	/* Connect stderr to the terminal. */
	opt->jo_io[PART_ERR] = JIO_BUFFER;
	opt->jo_io_buf[PART_ERR] = curbuf->b_fnum;
	opt->jo_modifiable[PART_ERR] = 0;
	opt->jo_set |= JO_ERR_IO + JO_ERR_BUF + JO_ERR_MODIFIABLE;
    }

    opt->jo_pty = TRUE;
    if ((opt->jo_set2 & JO2_TERM_ROWS) == 0)
	opt->jo_term_rows = rows;
    if ((opt->jo_set2 & JO2_TERM_COLS) == 0)
	opt->jo_term_cols = cols;
}

/*
 * Start a terminal window and return its buffer.
 * Returns NULL when failed.
 */
    static buf_T *
term_start(typval_T *argvar, jobopt_T *opt, int forceit)
{
    exarg_T	split_ea;
    win_T	*old_curwin = curwin;
    term_T	*term;
    buf_T	*old_curbuf = NULL;
    int		res;
    buf_T	*newbuf;

    if (check_restricted() || check_secure())
	return NULL;

    if ((opt->jo_set & (JO_IN_IO + JO_OUT_IO + JO_ERR_IO))
					 == (JO_IN_IO + JO_OUT_IO + JO_ERR_IO)
	|| (!(opt->jo_set & JO_OUT_IO) && (opt->jo_set & JO_OUT_BUF))
	|| (!(opt->jo_set & JO_ERR_IO) && (opt->jo_set & JO_ERR_BUF)))
    {
	EMSG(_(e_invarg));
	return NULL;
    }

    term = (term_T *)alloc_clear(sizeof(term_T));
    if (term == NULL)
	return NULL;
    term->tl_dirty_row_end = MAX_ROW;
    term->tl_cursor_visible = TRUE;
    term->tl_cursor_shape = VTERM_PROP_CURSORSHAPE_BLOCK;
    term->tl_finish = opt->jo_term_finish;
    ga_init2(&term->tl_scrollback, sizeof(sb_line_T), 300);

    vim_memset(&split_ea, 0, sizeof(split_ea));
    if (opt->jo_curwin)
    {
	/* Create a new buffer in the current window. */
	if (!can_abandon(curbuf, forceit))
	{
	    no_write_message();
	    vim_free(term);
	    return NULL;
	}
	if (do_ecmd(0, NULL, NULL, &split_ea, ECMD_ONE,
		     ECMD_HIDE + (forceit ? ECMD_FORCEIT : 0), curwin) == FAIL)
	{
	    vim_free(term);
	    return NULL;
	}
    }
    else if (opt->jo_hidden)
    {
	buf_T *buf;

	/* Create a new buffer without a window. Make it the current buffer for
	 * a moment to be able to do the initialisations. */
	buf = buflist_new((char_u *)"", NULL, (linenr_T)0,
							 BLN_NEW | BLN_LISTED);
	if (buf == NULL || ml_open(buf) == FAIL)
	{
	    vim_free(term);
	    return NULL;
	}
	old_curbuf = curbuf;
	--curbuf->b_nwindows;
	curbuf = buf;
	curwin->w_buffer = buf;
	++curbuf->b_nwindows;
    }
    else
    {
	/* Open a new window or tab. */
	split_ea.cmdidx = CMD_new;
	split_ea.cmd = (char_u *)"new";
	split_ea.arg = (char_u *)"";
	if (opt->jo_term_rows > 0 && !(cmdmod.split & WSP_VERT))
	{
	    split_ea.line2 = opt->jo_term_rows;
	    split_ea.addr_count = 1;
	}
	if (opt->jo_term_cols > 0 && (cmdmod.split & WSP_VERT))
	{
	    split_ea.line2 = opt->jo_term_cols;
	    split_ea.addr_count = 1;
	}

	ex_splitview(&split_ea);
	if (curwin == old_curwin)
	{
	    /* split failed */
	    vim_free(term);
	    return NULL;
	}
    }
    term->tl_buffer = curbuf;
    curbuf->b_term = term;

    if (!opt->jo_hidden)
    {
	/* only one size was taken care of with :new, do the other one */
	if (opt->jo_term_rows > 0 && (cmdmod.split & WSP_VERT))
	    win_setheight(opt->jo_term_rows);
	if (opt->jo_term_cols > 0 && !(cmdmod.split & WSP_VERT))
	    win_setwidth(opt->jo_term_cols);
    }

    /* Link the new terminal in the list of active terminals. */
    term->tl_next = first_term;
    first_term = term;

    if (opt->jo_term_name != NULL)
	curbuf->b_ffname = vim_strsave(opt->jo_term_name);
    else
    {
	int	i;
	size_t	len;
	char_u	*cmd, *p;

	if (argvar->v_type == VAR_STRING)
	{
	    cmd = argvar->vval.v_string;
	    if (cmd == NULL)
		cmd = (char_u *)"";
	    else if (STRCMP(cmd, "NONE") == 0)
		cmd = (char_u *)"pty";
	}
	else if (argvar->v_type != VAR_LIST
		|| argvar->vval.v_list == NULL
		|| argvar->vval.v_list->lv_len < 1
		|| (cmd = get_tv_string_chk(
			       &argvar->vval.v_list->lv_first->li_tv)) == NULL)
	    cmd = (char_u*)"";

	len = STRLEN(cmd) + 10;
	p = alloc((int)len);

	for (i = 0; p != NULL; ++i)
	{
	    /* Prepend a ! to the command name to avoid the buffer name equals
	     * the executable, otherwise ":w!" would overwrite it. */
	    if (i == 0)
		vim_snprintf((char *)p, len, "!%s", cmd);
	    else
		vim_snprintf((char *)p, len, "!%s (%d)", cmd, i);
	    if (buflist_findname(p) == NULL)
	    {
		vim_free(curbuf->b_ffname);
		curbuf->b_ffname = p;
		break;
	    }
	}
    }
    curbuf->b_fname = curbuf->b_ffname;

    if (opt->jo_term_opencmd != NULL)
	term->tl_opencmd = vim_strsave(opt->jo_term_opencmd);

    if (opt->jo_eof_chars != NULL)
	term->tl_eof_chars = vim_strsave(opt->jo_eof_chars);

    set_string_option_direct((char_u *)"buftype", -1,
				  (char_u *)"terminal", OPT_FREE|OPT_LOCAL, 0);

    /* Mark the buffer as not modifiable. It can only be made modifiable after
     * the job finished. */
    curbuf->b_p_ma = FALSE;

    set_term_and_win_size(term);
    setup_job_options(opt, term->tl_rows, term->tl_cols);

    /* System dependent: setup the vterm and maybe start the job in it. */
    if (argvar->v_type == VAR_STRING
	    && argvar->vval.v_string != NULL
	    && STRCMP(argvar->vval.v_string, "NONE") == 0)
	res = create_pty_only(term, opt);
    else
	res = term_and_job_init(term, argvar, opt);

    newbuf = curbuf;
    if (res == OK)
    {
	/* Get and remember the size we ended up with.  Update the pty. */
	vterm_get_size(term->tl_vterm, &term->tl_rows, &term->tl_cols);
	term_report_winsize(term, term->tl_rows, term->tl_cols);

	/* Make sure we don't get stuck on sending keys to the job, it leads to
	 * a deadlock if the job is waiting for Vim to read. */
	channel_set_nonblock(term->tl_job->jv_channel, PART_IN);

#ifdef FEAT_AUTOCMD
	++curbuf->b_locked;
	apply_autocmds(EVENT_BUFWINENTER, NULL, NULL, FALSE, curbuf);
	--curbuf->b_locked;
#endif

	if (old_curbuf != NULL)
	{
	    --curbuf->b_nwindows;
	    curbuf = old_curbuf;
	    curwin->w_buffer = curbuf;
	    ++curbuf->b_nwindows;
	}
    }
    else
    {
	buf_T *buf = curbuf;

	free_terminal(curbuf);
	if (old_curbuf != NULL)
	{
	    --curbuf->b_nwindows;
	    curbuf = old_curbuf;
	    curwin->w_buffer = curbuf;
	    ++curbuf->b_nwindows;
	}

	/* Wiping out the buffer will also close the window and call
	 * free_terminal(). */
	do_buffer(DOBUF_WIPE, DOBUF_FIRST, FORWARD, buf->b_fnum, TRUE);
	return NULL;
    }
    return newbuf;
}

/*
 * ":terminal": open a terminal window and execute a job in it.
 */
    void
ex_terminal(exarg_T *eap)
{
    typval_T	argvar[2];
    jobopt_T	opt;
    char_u	*cmd;
    char_u	*tofree = NULL;

    init_job_options(&opt);

    cmd = eap->arg;
    while (*cmd && *cmd == '+' && *(cmd + 1) == '+')
    {
	char_u  *p, *ep;

	cmd += 2;
	p = skiptowhite(cmd);
	ep = vim_strchr(cmd, '=');
	if (ep != NULL && ep < p)
	    p = ep;

	if ((int)(p - cmd) == 5 && STRNICMP(cmd, "close", 5) == 0)
	    opt.jo_term_finish = 'c';
	else if ((int)(p - cmd) == 4 && STRNICMP(cmd, "open", 4) == 0)
	    opt.jo_term_finish = 'o';
	else if ((int)(p - cmd) == 6 && STRNICMP(cmd, "curwin", 6) == 0)
	    opt.jo_curwin = 1;
	else if ((int)(p - cmd) == 6 && STRNICMP(cmd, "hidden", 6) == 0)
	    opt.jo_hidden = 1;
	else if ((int)(p - cmd) == 4 && STRNICMP(cmd, "rows", 4) == 0
		&& ep != NULL && isdigit(ep[1]))
	{
	    opt.jo_set2 |= JO2_TERM_ROWS;
	    opt.jo_term_rows = atoi((char *)ep + 1);
	    p = skiptowhite(cmd);
	}
	else if ((int)(p - cmd) == 4 && STRNICMP(cmd, "cols", 4) == 0
		&& ep != NULL && isdigit(ep[1]))
	{
	    opt.jo_set2 |= JO2_TERM_COLS;
	    opt.jo_term_cols = atoi((char *)ep + 1);
	    p = skiptowhite(cmd);
	}
	else if ((int)(p - cmd) == 3 && STRNICMP(cmd, "eof", 3) == 0
								 && ep != NULL)
	{
	    char_u *buf = NULL;
	    char_u *keys;

	    p = skiptowhite(cmd);
	    *p = NUL;
	    keys = replace_termcodes(ep + 1, &buf, TRUE, TRUE, TRUE);
	    opt.jo_set2 |= JO2_EOF_CHARS;
	    opt.jo_eof_chars = vim_strsave(keys);
	    vim_free(buf);
	    *p = ' ';
	}
	else
	{
	    if (*p)
		*p = NUL;
	    EMSG2(_("E181: Invalid attribute: %s"), cmd);
	    return;
	}
	cmd = skipwhite(p);
    }
    if (*cmd == NUL)
	/* Make a copy of 'shell', an autocommand may change the option. */
	tofree = cmd = vim_strsave(p_sh);

    if (eap->addr_count > 0)
    {
	/* Write lines from current buffer to the job. */
	opt.jo_set |= JO_IN_IO | JO_IN_BUF | JO_IN_TOP | JO_IN_BOT;
	opt.jo_io[PART_IN] = JIO_BUFFER;
	opt.jo_io_buf[PART_IN] = curbuf->b_fnum;
	opt.jo_in_top = eap->line1;
	opt.jo_in_bot = eap->line2;
    }

    argvar[0].v_type = VAR_STRING;
    argvar[0].vval.v_string = cmd;
    argvar[1].v_type = VAR_UNKNOWN;
    term_start(argvar, &opt, eap->forceit);
    vim_free(tofree);
    vim_free(opt.jo_eof_chars);
}

/*
 * Free the scrollback buffer for "term".
 */
    static void
free_scrollback(term_T *term)
{
    int i;

    for (i = 0; i < term->tl_scrollback.ga_len; ++i)
	vim_free(((sb_line_T *)term->tl_scrollback.ga_data + i)->sb_cells);
    ga_clear(&term->tl_scrollback);
}

/*
 * Free a terminal and everything it refers to.
 * Kills the job if there is one.
 * Called when wiping out a buffer.
 */
    void
free_terminal(buf_T *buf)
{
    term_T	*term = buf->b_term;
    term_T	*tp;

    if (term == NULL)
	return;
    if (first_term == term)
	first_term = term->tl_next;
    else
	for (tp = first_term; tp->tl_next != NULL; tp = tp->tl_next)
	    if (tp->tl_next == term)
	    {
		tp->tl_next = term->tl_next;
		break;
	    }

    if (term->tl_job != NULL)
    {
	if (term->tl_job->jv_status != JOB_ENDED
		&& term->tl_job->jv_status != JOB_FINISHED
	        && term->tl_job->jv_status != JOB_FAILED)
	    job_stop(term->tl_job, NULL, "kill");
	job_unref(term->tl_job);
    }

    free_scrollback(term);

    term_free_vterm(term);
    vim_free(term->tl_title);
    vim_free(term->tl_status_text);
    vim_free(term->tl_opencmd);
    vim_free(term->tl_eof_chars);
    vim_free(term->tl_cursor_color);
    vim_free(term);
    buf->b_term = NULL;
    if (in_terminal_loop == term)
	in_terminal_loop = NULL;
}

/*
 * Write job output "msg[len]" to the vterm.
 */
    static void
term_write_job_output(term_T *term, char_u *msg, size_t len)
{
    VTerm	*vterm = term->tl_vterm;

    vterm_input_write(vterm, (char *)msg, len);

    /* this invokes the damage callbacks */
    vterm_screen_flush_damage(vterm_obtain_screen(vterm));
}

    static void
update_cursor(term_T *term, int redraw)
{
    if (term->tl_normal_mode)
	return;
    setcursor();
    if (redraw)
    {
	if (term->tl_buffer == curbuf && term->tl_cursor_visible)
	    cursor_on();
	out_flush();
#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    gui_update_cursor(FALSE, FALSE);
	    gui_mch_flush();
	}
#endif
    }
}

/*
 * Invoked when "msg" output from a job was received.  Write it to the terminal
 * of "buffer".
 */
    void
write_to_term(buf_T *buffer, char_u *msg, channel_T *channel)
{
    size_t	len = STRLEN(msg);
    term_T	*term = buffer->b_term;

    if (term->tl_vterm == NULL)
    {
	ch_log(channel, "NOT writing %d bytes to terminal", (int)len);
	return;
    }
    ch_log(channel, "writing %d bytes to terminal", (int)len);
    term_write_job_output(term, msg, len);

    /* In Terminal-Normal mode we are displaying the buffer, not the terminal
     * contents, thus no screen update is needed. */
    if (!term->tl_normal_mode)
    {
	/* TODO: only update once in a while. */
	ch_log(term->tl_job->jv_channel, "updating screen");
	if (buffer == curbuf)
	{
	    update_screen(0);
	    update_cursor(term, TRUE);
	}
	else
	    redraw_after_callback(TRUE);
    }
}

/*
 * Send a mouse position and click to the vterm
 */
    static int
term_send_mouse(VTerm *vterm, int button, int pressed)
{
    VTermModifier   mod = VTERM_MOD_NONE;

    vterm_mouse_move(vterm, mouse_row - W_WINROW(curwin),
					    mouse_col - curwin->w_wincol, mod);
    if (button != 0)
	vterm_mouse_button(vterm, button, pressed, mod);
    return TRUE;
}

/*
 * Convert typed key "c" into bytes to send to the job.
 * Return the number of bytes in "buf".
 */
    static int
term_convert_key(term_T *term, int c, char *buf)
{
    VTerm	    *vterm = term->tl_vterm;
    VTermKey	    key = VTERM_KEY_NONE;
    VTermModifier   mod = VTERM_MOD_NONE;
    int		    other = FALSE;

    switch (c)
    {
	/* don't use VTERM_KEY_ENTER, it may do an unwanted conversion */

				/* don't use VTERM_KEY_BACKSPACE, it always
				 * becomes 0x7f DEL */
	case K_BS:		c = term_backspace_char; break;

	case ESC:		key = VTERM_KEY_ESCAPE; break;
	case K_DEL:		key = VTERM_KEY_DEL; break;
	case K_DOWN:		key = VTERM_KEY_DOWN; break;
	case K_S_DOWN:		mod = VTERM_MOD_SHIFT;
				key = VTERM_KEY_DOWN; break;
	case K_END:		key = VTERM_KEY_END; break;
	case K_S_END:		mod = VTERM_MOD_SHIFT;
				key = VTERM_KEY_END; break;
	case K_C_END:		mod = VTERM_MOD_CTRL;
				key = VTERM_KEY_END; break;
	case K_F10:		key = VTERM_KEY_FUNCTION(10); break;
	case K_F11:		key = VTERM_KEY_FUNCTION(11); break;
	case K_F12:		key = VTERM_KEY_FUNCTION(12); break;
	case K_F1:		key = VTERM_KEY_FUNCTION(1); break;
	case K_F2:		key = VTERM_KEY_FUNCTION(2); break;
	case K_F3:		key = VTERM_KEY_FUNCTION(3); break;
	case K_F4:		key = VTERM_KEY_FUNCTION(4); break;
	case K_F5:		key = VTERM_KEY_FUNCTION(5); break;
	case K_F6:		key = VTERM_KEY_FUNCTION(6); break;
	case K_F7:		key = VTERM_KEY_FUNCTION(7); break;
	case K_F8:		key = VTERM_KEY_FUNCTION(8); break;
	case K_F9:		key = VTERM_KEY_FUNCTION(9); break;
	case K_HOME:		key = VTERM_KEY_HOME; break;
	case K_S_HOME:		mod = VTERM_MOD_SHIFT;
				key = VTERM_KEY_HOME; break;
	case K_C_HOME:		mod = VTERM_MOD_CTRL;
				key = VTERM_KEY_HOME; break;
	case K_INS:		key = VTERM_KEY_INS; break;
	case K_K0:		key = VTERM_KEY_KP_0; break;
	case K_K1:		key = VTERM_KEY_KP_1; break;
	case K_K2:		key = VTERM_KEY_KP_2; break;
	case K_K3:		key = VTERM_KEY_KP_3; break;
	case K_K4:		key = VTERM_KEY_KP_4; break;
	case K_K5:		key = VTERM_KEY_KP_5; break;
	case K_K6:		key = VTERM_KEY_KP_6; break;
	case K_K7:		key = VTERM_KEY_KP_7; break;
	case K_K8:		key = VTERM_KEY_KP_8; break;
	case K_K9:		key = VTERM_KEY_KP_9; break;
	case K_KDEL:		key = VTERM_KEY_DEL; break; /* TODO */
	case K_KDIVIDE:		key = VTERM_KEY_KP_DIVIDE; break;
	case K_KEND:		key = VTERM_KEY_KP_1; break; /* TODO */
	case K_KENTER:		key = VTERM_KEY_KP_ENTER; break;
	case K_KHOME:		key = VTERM_KEY_KP_7; break; /* TODO */
	case K_KINS:		key = VTERM_KEY_KP_0; break; /* TODO */
	case K_KMINUS:		key = VTERM_KEY_KP_MINUS; break;
	case K_KMULTIPLY:	key = VTERM_KEY_KP_MULT; break;
	case K_KPAGEDOWN:	key = VTERM_KEY_KP_3; break; /* TODO */
	case K_KPAGEUP:		key = VTERM_KEY_KP_9; break; /* TODO */
	case K_KPLUS:		key = VTERM_KEY_KP_PLUS; break;
	case K_KPOINT:		key = VTERM_KEY_KP_PERIOD; break;
	case K_LEFT:		key = VTERM_KEY_LEFT; break;
	case K_S_LEFT:		mod = VTERM_MOD_SHIFT;
				key = VTERM_KEY_LEFT; break;
	case K_C_LEFT:		mod = VTERM_MOD_CTRL;
				key = VTERM_KEY_LEFT; break;
	case K_PAGEDOWN:	key = VTERM_KEY_PAGEDOWN; break;
	case K_PAGEUP:		key = VTERM_KEY_PAGEUP; break;
	case K_RIGHT:		key = VTERM_KEY_RIGHT; break;
	case K_S_RIGHT:		mod = VTERM_MOD_SHIFT;
				key = VTERM_KEY_RIGHT; break;
	case K_C_RIGHT:		mod = VTERM_MOD_CTRL;
				key = VTERM_KEY_RIGHT; break;
	case K_UP:		key = VTERM_KEY_UP; break;
	case K_S_UP:		mod = VTERM_MOD_SHIFT;
				key = VTERM_KEY_UP; break;
	case TAB:		key = VTERM_KEY_TAB; break;

	case K_MOUSEUP:		other = term_send_mouse(vterm, 5, 1); break;
	case K_MOUSEDOWN:	other = term_send_mouse(vterm, 4, 1); break;
	case K_MOUSELEFT:	/* TODO */ return 0;
	case K_MOUSERIGHT:	/* TODO */ return 0;

	case K_LEFTMOUSE:
	case K_LEFTMOUSE_NM:	other = term_send_mouse(vterm, 1, 1); break;
	case K_LEFTDRAG:	other = term_send_mouse(vterm, 1, 1); break;
	case K_LEFTRELEASE:
	case K_LEFTRELEASE_NM:	other = term_send_mouse(vterm, 1, 0); break;
	case K_MOUSEMOVE:	other = term_send_mouse(vterm, 0, 0); break;
	case K_MIDDLEMOUSE:	other = term_send_mouse(vterm, 2, 1); break;
	case K_MIDDLEDRAG:	other = term_send_mouse(vterm, 2, 1); break;
	case K_MIDDLERELEASE:	other = term_send_mouse(vterm, 2, 0); break;
	case K_RIGHTMOUSE:	other = term_send_mouse(vterm, 3, 1); break;
	case K_RIGHTDRAG:	other = term_send_mouse(vterm, 3, 1); break;
	case K_RIGHTRELEASE:	other = term_send_mouse(vterm, 3, 0); break;
	case K_X1MOUSE:		/* TODO */ return 0;
	case K_X1DRAG:		/* TODO */ return 0;
	case K_X1RELEASE:	/* TODO */ return 0;
	case K_X2MOUSE:		/* TODO */ return 0;
	case K_X2DRAG:		/* TODO */ return 0;
	case K_X2RELEASE:	/* TODO */ return 0;

	case K_IGNORE:		return 0;
	case K_NOP:		return 0;
	case K_UNDO:		return 0;
	case K_HELP:		return 0;
	case K_XF1:		key = VTERM_KEY_FUNCTION(1); break;
	case K_XF2:		key = VTERM_KEY_FUNCTION(2); break;
	case K_XF3:		key = VTERM_KEY_FUNCTION(3); break;
	case K_XF4:		key = VTERM_KEY_FUNCTION(4); break;
	case K_SELECT:		return 0;
#ifdef FEAT_GUI
	case K_VER_SCROLLBAR:	return 0;
	case K_HOR_SCROLLBAR:	return 0;
#endif
#ifdef FEAT_GUI_TABLINE
	case K_TABLINE:		return 0;
	case K_TABMENU:		return 0;
#endif
#ifdef FEAT_NETBEANS_INTG
	case K_F21:		key = VTERM_KEY_FUNCTION(21); break;
#endif
#ifdef FEAT_DND
	case K_DROP:		return 0;
#endif
#ifdef FEAT_AUTOCMD
	case K_CURSORHOLD:	return 0;
#endif
	case K_PS:		vterm_keyboard_start_paste(vterm);
				other = TRUE;
				break;
	case K_PE:		vterm_keyboard_end_paste(vterm);
				other = TRUE;
				break;
    }

    /*
     * Convert special keys to vterm keys:
     * - Write keys to vterm: vterm_keyboard_key()
     * - Write output to channel.
     * TODO: use mod_mask
     */
    if (key != VTERM_KEY_NONE)
	/* Special key, let vterm convert it. */
	vterm_keyboard_key(vterm, key, mod);
    else if (!other)
	/* Normal character, let vterm convert it. */
	vterm_keyboard_unichar(vterm, c, mod);

    /* Read back the converted escape sequence. */
    return (int)vterm_output_read(vterm, buf, KEY_BUF_LEN);
}

/*
 * Return TRUE if the job for "term" is still running.
 */
    int
term_job_running(term_T *term)
{
    /* Also consider the job finished when the channel is closed, to avoid a
     * race condition when updating the title. */
    return term != NULL
	&& term->tl_job != NULL
	&& channel_is_open(term->tl_job->jv_channel)
	&& (term->tl_job->jv_status == JOB_STARTED
		|| term->tl_job->jv_channel->ch_keep_open);
}

/*
 * Return TRUE if "term" has an active channel and used ":term NONE".
 */
    int
term_none_open(term_T *term)
{
    /* Also consider the job finished when the channel is closed, to avoid a
     * race condition when updating the title. */
    return term != NULL
	&& term->tl_job != NULL
	&& channel_is_open(term->tl_job->jv_channel)
	&& term->tl_job->jv_channel->ch_keep_open;
}

/*
 * Add the last line of the scrollback buffer to the buffer in the window.
 */
    static void
add_scrollback_line_to_buffer(term_T *term, char_u *text, int len)
{
    buf_T	*buf = term->tl_buffer;
    int		empty = (buf->b_ml.ml_flags & ML_EMPTY);
    linenr_T	lnum = buf->b_ml.ml_line_count;

#ifdef WIN3264
    if (!enc_utf8 && enc_codepage > 0)
    {
	WCHAR   *ret = NULL;
	int	length = 0;

	MultiByteToWideChar_alloc(CP_UTF8, 0, (char*)text, len + 1,
							   &ret, &length);
	if (ret != NULL)
	{
	    WideCharToMultiByte_alloc(enc_codepage, 0,
				      ret, length, (char **)&text, &len, 0, 0);
	    vim_free(ret);
	    ml_append_buf(term->tl_buffer, lnum, text, len, FALSE);
	    vim_free(text);
	}
    }
    else
#endif
	ml_append_buf(term->tl_buffer, lnum, text, len + 1, FALSE);
    if (empty)
    {
	/* Delete the empty line that was in the empty buffer. */
	curbuf = buf;
	ml_delete(1, FALSE);
	curbuf = curwin->w_buffer;
    }
}

    static void
cell2cellattr(const VTermScreenCell *cell, cellattr_T *attr)
{
    attr->width = cell->width;
    attr->attrs = cell->attrs;
    attr->fg = cell->fg;
    attr->bg = cell->bg;
}

    static int
equal_celattr(cellattr_T *a, cellattr_T *b)
{
    /* Comparing the colors should be sufficient. */
    return a->fg.red == b->fg.red
	&& a->fg.green == b->fg.green
	&& a->fg.blue == b->fg.blue
	&& a->bg.red == b->bg.red
	&& a->bg.green == b->bg.green
	&& a->bg.blue == b->bg.blue;
}


/*
 * Add the current lines of the terminal to scrollback and to the buffer.
 * Called after the job has ended and when switching to Terminal-Normal mode.
 */
    static void
move_terminal_to_buffer(term_T *term)
{
    win_T	    *wp;
    int		    len;
    int		    lines_skipped = 0;
    VTermPos	    pos;
    VTermScreenCell cell;
    cellattr_T	    fill_attr, new_fill_attr;
    cellattr_T	    *p;
    VTermScreen	    *screen;

    if (term->tl_vterm == NULL)
	return;
    screen = vterm_obtain_screen(term->tl_vterm);
    fill_attr = new_fill_attr = term->tl_default_color;

    for (pos.row = 0; pos.row < term->tl_rows; ++pos.row)
    {
	len = 0;
	for (pos.col = 0; pos.col < term->tl_cols; ++pos.col)
	    if (vterm_screen_get_cell(screen, pos, &cell) != 0
						       && cell.chars[0] != NUL)
	    {
		len = pos.col + 1;
		new_fill_attr = term->tl_default_color;
	    }
	    else
		/* Assume the last attr is the filler attr. */
		cell2cellattr(&cell, &new_fill_attr);

	if (len == 0 && equal_celattr(&new_fill_attr, &fill_attr))
	    ++lines_skipped;
	else
	{
	    while (lines_skipped > 0)
	    {
		/* Line was skipped, add an empty line. */
		--lines_skipped;
		if (ga_grow(&term->tl_scrollback, 1) == OK)
		{
		    sb_line_T *line = (sb_line_T *)term->tl_scrollback.ga_data
						  + term->tl_scrollback.ga_len;

		    line->sb_cols = 0;
		    line->sb_cells = NULL;
		    line->sb_fill_attr = fill_attr;
		    ++term->tl_scrollback.ga_len;

		    add_scrollback_line_to_buffer(term, (char_u *)"", 0);
		}
	    }

	    if (len == 0)
		p = NULL;
	    else
		p = (cellattr_T *)alloc((int)sizeof(cellattr_T) * len);
	    if ((p != NULL || len == 0)
				     && ga_grow(&term->tl_scrollback, 1) == OK)
	    {
		garray_T    ga;
		int	    width;
		sb_line_T   *line = (sb_line_T *)term->tl_scrollback.ga_data
						  + term->tl_scrollback.ga_len;

		ga_init2(&ga, 1, 100);
		for (pos.col = 0; pos.col < len; pos.col += width)
		{
		    if (vterm_screen_get_cell(screen, pos, &cell) == 0)
		    {
			width = 1;
			vim_memset(p + pos.col, 0, sizeof(cellattr_T));
			if (ga_grow(&ga, 1) == OK)
			    ga.ga_len += utf_char2bytes(' ',
					     (char_u *)ga.ga_data + ga.ga_len);
		    }
		    else
		    {
			width = cell.width;

			cell2cellattr(&cell, &p[pos.col]);

			if (ga_grow(&ga, MB_MAXBYTES) == OK)
			{
			    int	    i;
			    int	    c;

			    for (i = 0; (c = cell.chars[i]) > 0 || i == 0; ++i)
				ga.ga_len += utf_char2bytes(c == NUL ? ' ' : c,
					     (char_u *)ga.ga_data + ga.ga_len);
			}
		    }
		}
		line->sb_cols = len;
		line->sb_cells = p;
		line->sb_fill_attr = new_fill_attr;
		fill_attr = new_fill_attr;
		++term->tl_scrollback.ga_len;

		if (ga_grow(&ga, 1) == FAIL)
		    add_scrollback_line_to_buffer(term, (char_u *)"", 0);
		else
		{
		    *((char_u *)ga.ga_data + ga.ga_len) = NUL;
		    add_scrollback_line_to_buffer(term, ga.ga_data, ga.ga_len);
		}
		ga_clear(&ga);
	    }
	    else
		vim_free(p);
	}
    }

    /* Obtain the current background color. */
    vterm_state_get_default_colors(vterm_obtain_state(term->tl_vterm),
		       &term->tl_default_color.fg, &term->tl_default_color.bg);

    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer == term->tl_buffer)
	{
	    wp->w_cursor.lnum = term->tl_buffer->b_ml.ml_line_count;
	    wp->w_cursor.col = 0;
	    wp->w_valid = 0;
	    if (wp->w_cursor.lnum >= wp->w_height)
	    {
		linenr_T min_topline = wp->w_cursor.lnum - wp->w_height + 1;

		if (wp->w_topline < min_topline)
		    wp->w_topline = min_topline;
	    }
	    redraw_win_later(wp, NOT_VALID);
	}
    }
}

    static void
set_terminal_mode(term_T *term, int normal_mode)
{
    term->tl_normal_mode = normal_mode;
    vim_free(term->tl_status_text);
    term->tl_status_text = NULL;
    if (term->tl_buffer == curbuf)
	maketitle();
}

/*
 * Called after the job if finished and Terminal mode is not active:
 * Move the vterm contents into the scrollback buffer and free the vterm.
 */
    static void
cleanup_vterm(term_T *term)
{
    if (term->tl_finish != 'c')
	move_terminal_to_buffer(term);
    term_free_vterm(term);
    set_terminal_mode(term, FALSE);
}

/*
 * Switch from Terminal-Job mode to Terminal-Normal mode.
 * Suspends updating the terminal window.
 */
    static void
term_enter_normal_mode(void)
{
    term_T *term = curbuf->b_term;

    /* Append the current terminal contents to the buffer. */
    move_terminal_to_buffer(term);

    set_terminal_mode(term, TRUE);

    /* Move the window cursor to the position of the cursor in the
     * terminal. */
    curwin->w_cursor.lnum = term->tl_scrollback_scrolled
					     + term->tl_cursor_pos.row + 1;
    check_cursor();
    coladvance(term->tl_cursor_pos.col);

    /* Display the same lines as in the terminal. */
    curwin->w_topline = term->tl_scrollback_scrolled + 1;
}

/*
 * Returns TRUE if the current window contains a terminal and we are in
 * Terminal-Normal mode.
 */
    int
term_in_normal_mode(void)
{
    term_T *term = curbuf->b_term;

    return term != NULL && term->tl_normal_mode;
}

/*
 * Switch from Terminal-Normal mode to Terminal-Job mode.
 * Restores updating the terminal window.
 */
    void
term_enter_job_mode()
{
    term_T	*term = curbuf->b_term;
    sb_line_T	*line;
    garray_T	*gap;

    /* Remove the terminal contents from the scrollback and the buffer. */
    gap = &term->tl_scrollback;
    while (curbuf->b_ml.ml_line_count > term->tl_scrollback_scrolled
							    && gap->ga_len > 0)
    {
	ml_delete(curbuf->b_ml.ml_line_count, FALSE);
	line = (sb_line_T *)gap->ga_data + gap->ga_len - 1;
	vim_free(line->sb_cells);
	--gap->ga_len;
    }
    check_cursor();

    set_terminal_mode(term, FALSE);

    if (term->tl_channel_closed)
	cleanup_vterm(term);
    redraw_buf_and_status_later(curbuf, NOT_VALID);
}

/*
 * Get a key from the user without mapping.
 * Note: while waiting a terminal may be closed and freed if the channel is
 * closed and ++close was used.
 * Uses terminal mode mappings.
 */
    static int
term_vgetc()
{
    int c;
    int save_State = State;

    State = TERMINAL;
    got_int = FALSE;
#ifdef WIN3264
    ctrl_break_was_pressed = FALSE;
#endif
    c = vgetc();
    got_int = FALSE;
    State = save_State;
    return c;
}

/*
 * Get the part that is connected to the tty. Normally this is PART_IN, but
 * when writing buffer lines to the job it can be another.  This makes it
 * possible to do "1,5term vim -".
 */
    static ch_part_T
get_tty_part(term_T *term)
{
#ifdef UNIX
    ch_part_T	parts[3] = {PART_IN, PART_OUT, PART_ERR};
    int		i;

    for (i = 0; i < 3; ++i)
    {
	int fd = term->tl_job->jv_channel->ch_part[parts[i]].ch_fd;

	if (isatty(fd))
	    return parts[i];
    }
#endif
    return PART_IN;
}

/*
 * Send keys to terminal.
 * Return FAIL when the key needs to be handled in Normal mode.
 * Return OK when the key was dropped or sent to the terminal.
 */
    int
send_keys_to_term(term_T *term, int c, int typed)
{
    char	msg[KEY_BUF_LEN];
    size_t	len;
    static int	mouse_was_outside = FALSE;
    int		dragging_outside = FALSE;

    /* Catch keys that need to be handled as in Normal mode. */
    switch (c)
    {
	case NUL:
	case K_ZERO:
	    if (typed)
		stuffcharReadbuff(c);
	    return FAIL;

	case K_IGNORE:
	    return FAIL;

	case K_LEFTDRAG:
	case K_MIDDLEDRAG:
	case K_RIGHTDRAG:
	case K_X1DRAG:
	case K_X2DRAG:
	    dragging_outside = mouse_was_outside;
	    /* FALLTHROUGH */
	case K_LEFTMOUSE:
	case K_LEFTMOUSE_NM:
	case K_LEFTRELEASE:
	case K_LEFTRELEASE_NM:
	case K_MOUSEMOVE:
	case K_MIDDLEMOUSE:
	case K_MIDDLERELEASE:
	case K_RIGHTMOUSE:
	case K_RIGHTRELEASE:
	case K_X1MOUSE:
	case K_X1RELEASE:
	case K_X2MOUSE:
	case K_X2RELEASE:

	case K_MOUSEUP:
	case K_MOUSEDOWN:
	case K_MOUSELEFT:
	case K_MOUSERIGHT:
	    if (mouse_row < W_WINROW(curwin)
		    || mouse_row >= (W_WINROW(curwin) + curwin->w_height)
		    || mouse_col < curwin->w_wincol
		    || mouse_col >= W_ENDCOL(curwin)
		    || dragging_outside)
	    {
		/* click or scroll outside the current window or on status line
		 * or vertical separator */
		if (typed)
		{
		    stuffcharReadbuff(c);
		    mouse_was_outside = TRUE;
		}
		return FAIL;
	    }
    }
    if (typed)
	mouse_was_outside = FALSE;

    /* Convert the typed key to a sequence of bytes for the job. */
    len = term_convert_key(term, c, msg);
    if (len > 0)
	/* TODO: if FAIL is returned, stop? */
	channel_send(term->tl_job->jv_channel, get_tty_part(term),
						(char_u *)msg, (int)len, NULL);

    return OK;
}

    static void
position_cursor(win_T *wp, VTermPos *pos)
{
    wp->w_wrow = MIN(pos->row, MAX(0, wp->w_height - 1));
    wp->w_wcol = MIN(pos->col, MAX(0, wp->w_width - 1));
    wp->w_valid |= (VALID_WCOL|VALID_WROW);
}

/*
 * Handle CTRL-W "": send register contents to the job.
 */
    static void
term_paste_register(int prev_c UNUSED)
{
    int		c;
    list_T	*l;
    listitem_T	*item;
    long	reglen = 0;
    int		type;

#ifdef FEAT_CMDL_INFO
    if (add_to_showcmd(prev_c))
    if (add_to_showcmd('"'))
	out_flush();
#endif
    c = term_vgetc();
#ifdef FEAT_CMDL_INFO
    clear_showcmd();
#endif
    if (!term_use_loop())
	/* job finished while waiting for a character */
	return;

    /* CTRL-W "= prompt for expression to evaluate. */
    if (c == '=' && get_expr_register() != '=')
	return;
    if (!term_use_loop())
	/* job finished while waiting for a character */
	return;

    l = (list_T *)get_reg_contents(c, GREG_LIST);
    if (l != NULL)
    {
	type = get_reg_type(c, &reglen);
	for (item = l->lv_first; item != NULL; item = item->li_next)
	{
	    char_u *s = get_tv_string(&item->li_tv);
#ifdef WIN3264
	    char_u *tmp = s;

	    if (!enc_utf8 && enc_codepage > 0)
	    {
		WCHAR   *ret = NULL;
		int	length = 0;

		MultiByteToWideChar_alloc(enc_codepage, 0, (char *)s,
						(int)STRLEN(s), &ret, &length);
		if (ret != NULL)
		{
		    WideCharToMultiByte_alloc(CP_UTF8, 0,
				    ret, length, (char **)&s, &length, 0, 0);
		    vim_free(ret);
		}
	    }
#endif
	    channel_send(curbuf->b_term->tl_job->jv_channel, PART_IN,
						      s, (int)STRLEN(s), NULL);
#ifdef WIN3264
	    if (tmp != s)
		vim_free(s);
#endif

	    if (item->li_next != NULL || type == MLINE)
		channel_send(curbuf->b_term->tl_job->jv_channel, PART_IN,
						      (char_u *)"\r", 1, NULL);
	}
	list_free(l);
    }
}

#if defined(FEAT_GUI) || defined(PROTO)
/*
 * Return TRUE when the cursor of the terminal should be displayed.
 */
    int
terminal_is_active()
{
    return in_terminal_loop != NULL;
}

    cursorentry_T *
term_get_cursor_shape(guicolor_T *fg, guicolor_T *bg)
{
    term_T		 *term = in_terminal_loop;
    static cursorentry_T entry;

    vim_memset(&entry, 0, sizeof(entry));
    entry.shape = entry.mshape =
	term->tl_cursor_shape == VTERM_PROP_CURSORSHAPE_UNDERLINE ? SHAPE_HOR :
	term->tl_cursor_shape == VTERM_PROP_CURSORSHAPE_BAR_LEFT ? SHAPE_VER :
	SHAPE_BLOCK;
    entry.percentage = 20;
    if (term->tl_cursor_blink)
    {
	entry.blinkwait = 700;
	entry.blinkon = 400;
	entry.blinkoff = 250;
    }
    *fg = gui.back_pixel;
    if (term->tl_cursor_color == NULL)
	*bg = gui.norm_pixel;
    else
	*bg = color_name2handle(term->tl_cursor_color);
    entry.name = "n";
    entry.used_for = SHAPE_CURSOR;

    return &entry;
}
#endif

static int did_change_cursor = FALSE;

    static void
may_set_cursor_props(term_T *term)
{
#ifdef FEAT_GUI
    /* For the GUI the cursor properties are obtained with
     * term_get_cursor_shape(). */
    if (gui.in_use)
	return;
#endif
    if (in_terminal_loop == term)
    {
	did_change_cursor = TRUE;
	if (term->tl_cursor_color != NULL)
	    term_cursor_color(term->tl_cursor_color);
	else
	    term_cursor_color((char_u *)"");
	term_cursor_shape(term->tl_cursor_shape, term->tl_cursor_blink);
    }
}

    static void
may_restore_cursor_props(void)
{
#ifdef FEAT_GUI
    if (gui.in_use)
	return;
#endif
    if (did_change_cursor)
    {
	did_change_cursor = FALSE;
	term_cursor_color((char_u *)"");
	/* this will restore the initial cursor style, if possible */
	ui_cursor_shape_forced(TRUE);
    }
}

/*
 * Returns TRUE if the current window contains a terminal and we are sending
 * keys to the job.
 */
    int
term_use_loop(void)
{
    term_T *term = curbuf->b_term;

    return term != NULL
	&& !term->tl_normal_mode
	&& term->tl_vterm != NULL
	&& term_job_running(term);
}

/*
 * Wait for input and send it to the job.
 * When "blocking" is TRUE wait for a character to be typed.  Otherwise return
 * when there is no more typahead.
 * Return when the start of a CTRL-W command is typed or anything else that
 * should be handled as a Normal mode command.
 * Returns OK if a typed character is to be handled in Normal mode, FAIL if
 * the terminal was closed.
 */
    int
terminal_loop(int blocking)
{
    int		c;
    int		termkey = 0;
    int		ret;
#ifdef UNIX
    int		tty_fd = curbuf->b_term->tl_job->jv_channel
				 ->ch_part[get_tty_part(curbuf->b_term)].ch_fd;
#endif

    /* Remember the terminal we are sending keys to.  However, the terminal
     * might be closed while waiting for a character, e.g. typing "exit" in a
     * shell and ++close was used.  Therefore use curbuf->b_term instead of a
     * stored reference. */
    in_terminal_loop = curbuf->b_term;

    if (*curwin->w_p_tk != NUL)
	termkey = string_to_key(curwin->w_p_tk, TRUE);
    position_cursor(curwin, &curbuf->b_term->tl_cursor_pos);
    may_set_cursor_props(curbuf->b_term);

    while (blocking || vpeekc() != NUL)
    {
	/* TODO: skip screen update when handling a sequence of keys. */
	/* Repeat redrawing in case a message is received while redrawing. */
	while (must_redraw != 0)
	    if (update_screen(0) == FAIL)
		break;
	update_cursor(curbuf->b_term, FALSE);

	c = term_vgetc();
	if (!term_use_loop())
	{
	    /* Job finished while waiting for a character.  Push back the
	     * received character. */
	    if (c != K_IGNORE)
		vungetc(c);
	    break;
	}
	if (c == K_IGNORE)
	    continue;

#ifdef UNIX
	/*
	 * The shell or another program may change the tty settings.  Getting
	 * them for every typed character is a bit of overhead, but it's needed
	 * for the first character typed, e.g. when Vim starts in a shell.
	 */
	if (isatty(tty_fd))
	{
	    ttyinfo_T info;

	    /* Get the current backspace character of the pty. */
	    if (get_tty_info(tty_fd, &info) == OK)
		term_backspace_char = info.backspace;
	}
#endif

#ifdef WIN3264
	/* On Windows winpty handles CTRL-C, don't send a CTRL_C_EVENT.
	 * Use CTRL-BREAK to kill the job. */
	if (ctrl_break_was_pressed)
	    mch_signal_job(curbuf->b_term->tl_job, (char_u *)"kill");
#endif
	/* Was either CTRL-W (termkey) or CTRL-\ pressed? */
	if (c == (termkey == 0 ? Ctrl_W : termkey) || c == Ctrl_BSL)
	{
	    int	    prev_c = c;

#ifdef FEAT_CMDL_INFO
	    if (add_to_showcmd(c))
		out_flush();
#endif
	    c = term_vgetc();
#ifdef FEAT_CMDL_INFO
	    clear_showcmd();
#endif
	    if (!term_use_loop())
		/* job finished while waiting for a character */
		break;

	    if (prev_c == Ctrl_BSL)
	    {
		if (c == Ctrl_N)
		{
		    /* CTRL-\ CTRL-N : go to Terminal-Normal mode. */
		    term_enter_normal_mode();
		    ret = FAIL;
		    goto theend;
		}
		/* Send both keys to the terminal. */
		send_keys_to_term(curbuf->b_term, prev_c, TRUE);
	    }
	    else if (c == Ctrl_C)
	    {
		/* "CTRL-W CTRL-C" or 'termkey' CTRL-C: end the job */
		mch_signal_job(curbuf->b_term->tl_job, (char_u *)"kill");
	    }
	    else if (termkey == 0 && c == '.')
	    {
		/* "CTRL-W .": send CTRL-W to the job */
		c = Ctrl_W;
	    }
	    else if (c == 'N')
	    {
		/* CTRL-W N : go to Terminal-Normal mode. */
		term_enter_normal_mode();
		ret = FAIL;
		goto theend;
	    }
	    else if (c == '"')
	    {
		term_paste_register(prev_c);
		continue;
	    }
	    else if (termkey == 0 || c != termkey)
	    {
		stuffcharReadbuff(Ctrl_W);
		stuffcharReadbuff(c);
		ret = OK;
		goto theend;
	    }
	}
# ifdef WIN3264
	if (!enc_utf8 && has_mbyte && c >= 0x80)
	{
	    WCHAR   wc;
	    char_u  mb[3];

	    mb[0] = (unsigned)c >> 8;
	    mb[1] = c;
	    if (MultiByteToWideChar(GetACP(), 0, (char*)mb, 2, &wc, 1) > 0)
		c = wc;
	}
# endif
	if (send_keys_to_term(curbuf->b_term, c, TRUE) != OK)
	{
	    ret = OK;
	    goto theend;
	}
    }
    ret = FAIL;

theend:
    in_terminal_loop = NULL;
    may_restore_cursor_props();
    return ret;
}

/*
 * Called when a job has finished.
 * This updates the title and status, but does not close the vterm, because
 * there might still be pending output in the channel.
 */
    void
term_job_ended(job_T *job)
{
    term_T *term;
    int	    did_one = FALSE;

    for (term = first_term; term != NULL; term = term->tl_next)
	if (term->tl_job == job)
	{
	    vim_free(term->tl_title);
	    term->tl_title = NULL;
	    vim_free(term->tl_status_text);
	    term->tl_status_text = NULL;
	    redraw_buf_and_status_later(term->tl_buffer, VALID);
	    did_one = TRUE;
	}
    if (did_one)
	redraw_statuslines();
    if (curbuf->b_term != NULL)
    {
	if (curbuf->b_term->tl_job == job)
	    maketitle();
	update_cursor(curbuf->b_term, TRUE);
    }
}

    static void
may_toggle_cursor(term_T *term)
{
    if (in_terminal_loop == term)
    {
	if (term->tl_cursor_visible)
	    cursor_on();
	else
	    cursor_off();
    }
}

/*
 * Reverse engineer the RGB value into a cterm color index.
 * First color is 1.  Return 0 if no match found (default color).
 */
    static int
color2index(VTermColor *color, int fg, int *boldp)
{
    int red = color->red;
    int blue = color->blue;
    int green = color->green;

    if (color->ansi_index != VTERM_ANSI_INDEX_NONE)
    {
	/* First 16 colors and default: use the ANSI index, because these
	 * colors can be redefined. */
	if (t_colors >= 16)
	    return color->ansi_index;
	switch (color->ansi_index)
	{
	    case  0: return 0;
	    case  1: return lookup_color( 0, fg, boldp) + 1;
	    case  2: return lookup_color( 4, fg, boldp) + 1; /* dark red */
	    case  3: return lookup_color( 2, fg, boldp) + 1; /* dark green */
	    case  4: return lookup_color( 6, fg, boldp) + 1; /* brown */
	    case  5: return lookup_color( 1, fg, boldp) + 1; /* dark blue*/
	    case  6: return lookup_color( 5, fg, boldp) + 1; /* dark magenta */
	    case  7: return lookup_color( 3, fg, boldp) + 1; /* dark cyan */
	    case  8: return lookup_color( 8, fg, boldp) + 1; /* light grey */
	    case  9: return lookup_color(12, fg, boldp) + 1; /* dark grey */
	    case 10: return lookup_color(20, fg, boldp) + 1; /* red */
	    case 11: return lookup_color(16, fg, boldp) + 1; /* green */
	    case 12: return lookup_color(24, fg, boldp) + 1; /* yellow */
	    case 13: return lookup_color(14, fg, boldp) + 1; /* blue */
	    case 14: return lookup_color(22, fg, boldp) + 1; /* magenta */
	    case 15: return lookup_color(18, fg, boldp) + 1; /* cyan */
	    case 16: return lookup_color(26, fg, boldp) + 1; /* white */
	}
    }

    if (t_colors >= 256)
    {
	if (red == blue && red == green)
	{
	    /* 24-color greyscale plus white and black */
	    static int cutoff[23] = {
		    0x0D, 0x17, 0x21, 0x2B, 0x35, 0x3F, 0x49, 0x53, 0x5D, 0x67,
		    0x71, 0x7B, 0x85, 0x8F, 0x99, 0xA3, 0xAD, 0xB7, 0xC1, 0xCB,
		    0xD5, 0xDF, 0xE9};
	    int i;

	    if (red < 5)
		return 17; /* 00/00/00 */
	    if (red > 245) /* ff/ff/ff */
		return 232;
	    for (i = 0; i < 23; ++i)
		if (red < cutoff[i])
		    return i + 233;
	    return 256;
	}
	{
	    static int cutoff[5] = {0x2F, 0x73, 0x9B, 0xC3, 0xEB};
	    int ri, gi, bi;

	    /* 216-color cube */
	    for (ri = 0; ri < 5; ++ri)
		if (red < cutoff[ri])
		    break;
	    for (gi = 0; gi < 5; ++gi)
		if (green < cutoff[gi])
		    break;
	    for (bi = 0; bi < 5; ++bi)
		if (blue < cutoff[bi])
		    break;
	    return 17 + ri * 36 + gi * 6 + bi;
	}
    }
    return 0;
}

/*
 * Convert the attributes of a vterm cell into an attribute index.
 */
    static int
cell2attr(VTermScreenCellAttrs cellattrs, VTermColor cellfg, VTermColor cellbg)
{
    int attr = 0;

    if (cellattrs.bold)
	attr |= HL_BOLD;
    if (cellattrs.underline)
	attr |= HL_UNDERLINE;
    if (cellattrs.italic)
	attr |= HL_ITALIC;
    if (cellattrs.strike)
	attr |= HL_STRIKETHROUGH;
    if (cellattrs.reverse)
	attr |= HL_INVERSE;

#ifdef FEAT_GUI
    if (gui.in_use)
    {
	guicolor_T fg, bg;

	fg = gui_mch_get_rgb_color(cellfg.red, cellfg.green, cellfg.blue);
	bg = gui_mch_get_rgb_color(cellbg.red, cellbg.green, cellbg.blue);
	return get_gui_attr_idx(attr, fg, bg);
    }
    else
#endif
#ifdef FEAT_TERMGUICOLORS
    if (p_tgc)
    {
	guicolor_T fg, bg;

	fg = gui_get_rgb_color_cmn(cellfg.red, cellfg.green, cellfg.blue);
	bg = gui_get_rgb_color_cmn(cellbg.red, cellbg.green, cellbg.blue);

	return get_tgc_attr_idx(attr, fg, bg);
    }
    else
#endif
    {
	int bold = MAYBE;
	int fg = color2index(&cellfg, TRUE, &bold);
	int bg = color2index(&cellbg, FALSE, &bold);

	/* Use the "Terminal" highlighting for the default colors. */
	if ((fg == 0 || bg == 0) && t_colors >= 16)
	{
	    if (fg == 0 && term_default_cterm_fg >= 0)
		fg = term_default_cterm_fg + 1;
	    if (bg == 0 && term_default_cterm_bg >= 0)
		bg = term_default_cterm_bg + 1;
	}

	/* with 8 colors set the bold attribute to get a bright foreground */
	if (bold == TRUE)
	    attr |= HL_BOLD;
	return get_cterm_attr_idx(attr, fg, bg);
    }
    return 0;
}

    static int
handle_damage(VTermRect rect, void *user)
{
    term_T *term = (term_T *)user;

    term->tl_dirty_row_start = MIN(term->tl_dirty_row_start, rect.start_row);
    term->tl_dirty_row_end = MAX(term->tl_dirty_row_end, rect.end_row);
    redraw_buf_later(term->tl_buffer, NOT_VALID);
    return 1;
}

    static int
handle_moverect(VTermRect dest, VTermRect src, void *user)
{
    term_T	*term = (term_T *)user;

    /* Scrolling up is done much more efficiently by deleting lines instead of
     * redrawing the text. */
    if (dest.start_col == src.start_col
	    && dest.end_col == src.end_col
	    && dest.start_row < src.start_row)
    {
	win_T	    *wp;
	VTermColor  fg, bg;
	VTermScreenCellAttrs attr;
	int	    clear_attr;

	/* Set the color to clear lines with. */
	vterm_state_get_default_colors(vterm_obtain_state(term->tl_vterm),
								     &fg, &bg);
	vim_memset(&attr, 0, sizeof(attr));
	clear_attr = cell2attr(attr, fg, bg);

	FOR_ALL_WINDOWS(wp)
	{
	    if (wp->w_buffer == term->tl_buffer)
		win_del_lines(wp, dest.start_row,
				 src.start_row - dest.start_row, FALSE, FALSE,
				 clear_attr);
	}
    }

    term->tl_dirty_row_start = MIN(term->tl_dirty_row_start, dest.start_row);
    term->tl_dirty_row_end = MIN(term->tl_dirty_row_end, dest.end_row);

    redraw_buf_later(term->tl_buffer, NOT_VALID);
    return 1;
}

    static int
handle_movecursor(
	VTermPos pos,
	VTermPos oldpos UNUSED,
	int visible,
	void *user)
{
    term_T	*term = (term_T *)user;
    win_T	*wp;

    term->tl_cursor_pos = pos;
    term->tl_cursor_visible = visible;

    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer == term->tl_buffer)
	    position_cursor(wp, &pos);
    }
    if (term->tl_buffer == curbuf && !term->tl_normal_mode)
    {
	may_toggle_cursor(term);
	update_cursor(term, term->tl_cursor_visible);
    }

    return 1;
}

    static int
handle_settermprop(
	VTermProp prop,
	VTermValue *value,
	void *user)
{
    term_T	*term = (term_T *)user;

    switch (prop)
    {
	case VTERM_PROP_TITLE:
	    vim_free(term->tl_title);
	    /* a blank title isn't useful, make it empty, so that "running" is
	     * displayed */
	    if (*skipwhite((char_u *)value->string) == NUL)
		term->tl_title = NULL;
#ifdef WIN3264
	    else if (!enc_utf8 && enc_codepage > 0)
	    {
		WCHAR   *ret = NULL;
		int	length = 0;

		MultiByteToWideChar_alloc(CP_UTF8, 0,
			(char*)value->string, (int)STRLEN(value->string),
								&ret, &length);
		if (ret != NULL)
		{
		    WideCharToMultiByte_alloc(enc_codepage, 0,
					ret, length, (char**)&term->tl_title,
					&length, 0, 0);
		    vim_free(ret);
		}
	    }
#endif
	    else
		term->tl_title = vim_strsave((char_u *)value->string);
	    vim_free(term->tl_status_text);
	    term->tl_status_text = NULL;
	    if (term == curbuf->b_term)
		maketitle();
	    break;

	case VTERM_PROP_CURSORVISIBLE:
	    term->tl_cursor_visible = value->boolean;
	    may_toggle_cursor(term);
	    out_flush();
	    break;

	case VTERM_PROP_CURSORBLINK:
	    term->tl_cursor_blink = value->boolean;
	    may_set_cursor_props(term);
	    break;

	case VTERM_PROP_CURSORSHAPE:
	    term->tl_cursor_shape = value->number;
	    may_set_cursor_props(term);
	    break;

	case VTERM_PROP_CURSORCOLOR:
	    vim_free(term->tl_cursor_color);
	    if (*value->string == NUL)
		term->tl_cursor_color = NULL;
	    else
		term->tl_cursor_color = vim_strsave((char_u *)value->string);
	    may_set_cursor_props(term);
	    break;

	case VTERM_PROP_ALTSCREEN:
	    /* TODO: do anything else? */
	    term->tl_using_altscreen = value->boolean;
	    break;

	default:
	    break;
    }
    /* Always return 1, otherwise vterm doesn't store the value internally. */
    return 1;
}

/*
 * The job running in the terminal resized the terminal.
 */
    static int
handle_resize(int rows, int cols, void *user)
{
    term_T	*term = (term_T *)user;
    win_T	*wp;

    term->tl_rows = rows;
    term->tl_cols = cols;
    if (term->tl_vterm_size_changed)
	/* Size was set by vterm_set_size(), don't set the window size. */
	term->tl_vterm_size_changed = FALSE;
    else
    {
	FOR_ALL_WINDOWS(wp)
	{
	    if (wp->w_buffer == term->tl_buffer)
	    {
		win_setheight_win(rows, wp);
		win_setwidth_win(cols, wp);
	    }
	}
	redraw_buf_later(term->tl_buffer, NOT_VALID);
    }
    return 1;
}

/*
 * Handle a line that is pushed off the top of the screen.
 */
    static int
handle_pushline(int cols, const VTermScreenCell *cells, void *user)
{
    term_T	*term = (term_T *)user;

    /* TODO: Limit the number of lines that are stored. */
    if (ga_grow(&term->tl_scrollback, 1) == OK)
    {
	cellattr_T	*p = NULL;
	int		len = 0;
	int		i;
	int		c;
	int		col;
	sb_line_T	*line;
	garray_T	ga;
	cellattr_T	fill_attr = term->tl_default_color;

	/* do not store empty cells at the end */
	for (i = 0; i < cols; ++i)
	    if (cells[i].chars[0] != 0)
		len = i + 1;
	    else
		cell2cellattr(&cells[i], &fill_attr);

	ga_init2(&ga, 1, 100);
	if (len > 0)
	    p = (cellattr_T *)alloc((int)sizeof(cellattr_T) * len);
	if (p != NULL)
	{
	    for (col = 0; col < len; col += cells[col].width)
	    {
		if (ga_grow(&ga, MB_MAXBYTES) == FAIL)
		{
		    ga.ga_len = 0;
		    break;
		}
		for (i = 0; (c = cells[col].chars[i]) > 0 || i == 0; ++i)
		    ga.ga_len += utf_char2bytes(c == NUL ? ' ' : c,
					     (char_u *)ga.ga_data + ga.ga_len);
		cell2cellattr(&cells[col], &p[col]);
	    }
	}
	if (ga_grow(&ga, 1) == FAIL)
	    add_scrollback_line_to_buffer(term, (char_u *)"", 0);
	else
	{
	    *((char_u *)ga.ga_data + ga.ga_len) = NUL;
	    add_scrollback_line_to_buffer(term, ga.ga_data, ga.ga_len);
	}
	ga_clear(&ga);

	line = (sb_line_T *)term->tl_scrollback.ga_data
						  + term->tl_scrollback.ga_len;
	line->sb_cols = len;
	line->sb_cells = p;
	line->sb_fill_attr = fill_attr;
	++term->tl_scrollback.ga_len;
	++term->tl_scrollback_scrolled;
    }
    return 0; /* ignored */
}

static VTermScreenCallbacks screen_callbacks = {
  handle_damage,	/* damage */
  handle_moverect,	/* moverect */
  handle_movecursor,	/* movecursor */
  handle_settermprop,	/* settermprop */
  NULL,			/* bell */
  handle_resize,	/* resize */
  handle_pushline,	/* sb_pushline */
  NULL			/* sb_popline */
};

/*
 * Called when a channel has been closed.
 * If this was a channel for a terminal window then finish it up.
 */
    void
term_channel_closed(channel_T *ch)
{
    term_T *term;
    int	    did_one = FALSE;

    for (term = first_term; term != NULL; term = term->tl_next)
	if (term->tl_job == ch->ch_job)
	{
	    term->tl_channel_closed = TRUE;
	    did_one = TRUE;

	    vim_free(term->tl_title);
	    term->tl_title = NULL;
	    vim_free(term->tl_status_text);
	    term->tl_status_text = NULL;

	    /* Unless in Terminal-Normal mode: clear the vterm. */
	    if (!term->tl_normal_mode)
	    {
		int	fnum = term->tl_buffer->b_fnum;

		cleanup_vterm(term);

		if (term->tl_finish == 'c')
		{
		    aco_save_T	aco;

		    /* ++close or term_finish == "close" */
		    ch_log(NULL, "terminal job finished, closing window");
		    aucmd_prepbuf(&aco, term->tl_buffer);
		    do_bufdel(DOBUF_WIPE, (char_u *)"", 1, fnum, fnum, FALSE);
		    aucmd_restbuf(&aco);
		    break;
		}
		if (term->tl_finish == 'o' && term->tl_buffer->b_nwindows == 0)
		{
		    char buf[50];

		    /* TODO: use term_opencmd */
		    ch_log(NULL, "terminal job finished, opening window");
		    vim_snprintf(buf, sizeof(buf),
			    term->tl_opencmd == NULL
				    ? "botright sbuf %d"
				    : (char *)term->tl_opencmd, fnum);
		    do_cmdline_cmd((char_u *)buf);
		}
		else
		    ch_log(NULL, "terminal job finished");
	    }

	    redraw_buf_and_status_later(term->tl_buffer, NOT_VALID);
	}
    if (did_one)
    {
	redraw_statuslines();

	/* Need to break out of vgetc(). */
	ins_char_typebuf(K_IGNORE);
	typebuf_was_filled = TRUE;

	term = curbuf->b_term;
	if (term != NULL)
	{
	    if (term->tl_job == ch->ch_job)
		maketitle();
	    update_cursor(term, term->tl_cursor_visible);
	}
    }
}

/*
 * Called to update a window that contains an active terminal.
 * Returns FAIL when there is no terminal running in this window or in
 * Terminal-Normal mode.
 */
    int
term_update_window(win_T *wp)
{
    term_T	*term = wp->w_buffer->b_term;
    VTerm	*vterm;
    VTermScreen *screen;
    VTermState	*state;
    VTermPos	pos;

    if (term == NULL || term->tl_vterm == NULL || term->tl_normal_mode)
	return FAIL;

    vterm = term->tl_vterm;
    screen = vterm_obtain_screen(vterm);
    state = vterm_obtain_state(vterm);

    if (wp->w_redr_type >= SOME_VALID)
    {
	term->tl_dirty_row_start = 0;
	term->tl_dirty_row_end = MAX_ROW;
    }

    /*
     * If the window was resized a redraw will be triggered and we get here.
     * Adjust the size of the vterm unless 'termsize' specifies a fixed size.
     */
    if ((!term->tl_rows_fixed && term->tl_rows != wp->w_height)
	    || (!term->tl_cols_fixed && term->tl_cols != wp->w_width))
    {
	int	rows = term->tl_rows_fixed ? term->tl_rows : wp->w_height;
	int	cols = term->tl_cols_fixed ? term->tl_cols : wp->w_width;
	win_T	*twp;

	FOR_ALL_WINDOWS(twp)
	{
	    /* When more than one window shows the same terminal, use the
	     * smallest size. */
	    if (twp->w_buffer == term->tl_buffer)
	    {
		if (!term->tl_rows_fixed && rows > twp->w_height)
		    rows = twp->w_height;
		if (!term->tl_cols_fixed && cols > twp->w_width)
		    cols = twp->w_width;
	    }
	}

	term->tl_vterm_size_changed = TRUE;
	vterm_set_size(vterm, rows, cols);
	ch_log(term->tl_job->jv_channel, "Resizing terminal to %d lines",
									 rows);
	term_report_winsize(term, rows, cols);
    }

    /* The cursor may have been moved when resizing. */
    vterm_state_get_cursorpos(state, &pos);
    position_cursor(wp, &pos);

    for (pos.row = term->tl_dirty_row_start; pos.row < term->tl_dirty_row_end
					  && pos.row < wp->w_height; ++pos.row)
    {
	int off = screen_get_current_line_off();
	int max_col = MIN(wp->w_width, term->tl_cols);

	if (pos.row < term->tl_rows)
	{
	    for (pos.col = 0; pos.col < max_col; )
	    {
		VTermScreenCell cell;
		int		c;

		if (vterm_screen_get_cell(screen, pos, &cell) == 0)
		    vim_memset(&cell, 0, sizeof(cell));

		c = cell.chars[0];
		if (c == NUL)
		{
		    ScreenLines[off] = ' ';
		    if (enc_utf8)
			ScreenLinesUC[off] = NUL;
		}
		else
		{
		    if (enc_utf8)
		    {
			int i;

			/* composing chars */
			for (i = 0; i < Screen_mco
				      && i + 1 < VTERM_MAX_CHARS_PER_CELL; ++i)
			{
			    ScreenLinesC[i][off] = cell.chars[i + 1];
			    if (cell.chars[i + 1] == 0)
				break;
			}
			if (c >= 0x80 || (Screen_mco > 0
						 && ScreenLinesC[0][off] != 0))
			{
			    ScreenLines[off] = ' ';
			    ScreenLinesUC[off] = c;
			}
			else
			{
			    ScreenLines[off] = c;
			    ScreenLinesUC[off] = NUL;
			}
		    }
#ifdef WIN3264
		    else if (has_mbyte && c >= 0x80)
		    {
			char_u	mb[MB_MAXBYTES+1];
			WCHAR	wc = c;

			if (WideCharToMultiByte(GetACP(), 0, &wc, 1,
						       (char*)mb, 2, 0, 0) > 1)
			{
			    ScreenLines[off] = mb[0];
			    ScreenLines[off + 1] = mb[1];
			    cell.width = mb_ptr2cells(mb);
			}
			else
			    ScreenLines[off] = c;
		    }
#endif
		    else
			ScreenLines[off] = c;
		}
		ScreenAttrs[off] = cell2attr(cell.attrs, cell.fg, cell.bg);

		++pos.col;
		++off;
		if (cell.width == 2)
		{
		    if (enc_utf8)
			ScreenLinesUC[off] = NUL;

		    /* don't set the second byte to NUL for a DBCS encoding, it
		     * has been set above */
		    if (enc_utf8 || !has_mbyte)
			ScreenLines[off] = NUL;

		    ++pos.col;
		    ++off;
		}
	    }
	}
	else
	    pos.col = 0;

	screen_line(wp->w_winrow + pos.row, wp->w_wincol,
						  pos.col, wp->w_width, FALSE);
    }
    term->tl_dirty_row_start = MAX_ROW;
    term->tl_dirty_row_end = 0;

    return OK;
}

/*
 * Return TRUE if "wp" is a terminal window where the job has finished.
 */
    int
term_is_finished(buf_T *buf)
{
    return buf->b_term != NULL && buf->b_term->tl_vterm == NULL;
}

/*
 * Return TRUE if "wp" is a terminal window where the job has finished or we
 * are in Terminal-Normal mode, thus we show the buffer contents.
 */
    int
term_show_buffer(buf_T *buf)
{
    term_T *term = buf->b_term;

    return term != NULL && (term->tl_vterm == NULL || term->tl_normal_mode);
}

/*
 * The current buffer is going to be changed.  If there is terminal
 * highlighting remove it now.
 */
    void
term_change_in_curbuf(void)
{
    term_T *term = curbuf->b_term;

    if (term_is_finished(curbuf) && term->tl_scrollback.ga_len > 0)
    {
	free_scrollback(term);
	redraw_buf_later(term->tl_buffer, NOT_VALID);

	/* The buffer is now like a normal buffer, it cannot be easily
	 * abandoned when changed. */
	set_string_option_direct((char_u *)"buftype", -1,
					  (char_u *)"", OPT_FREE|OPT_LOCAL, 0);
    }
}

/*
 * Get the screen attribute for a position in the buffer.
 * Use a negative "col" to get the filler background color.
 */
    int
term_get_attr(buf_T *buf, linenr_T lnum, int col)
{
    term_T	*term = buf->b_term;
    sb_line_T	*line;
    cellattr_T	*cellattr;

    if (lnum > term->tl_scrollback.ga_len)
	cellattr = &term->tl_default_color;
    else
    {
	line = (sb_line_T *)term->tl_scrollback.ga_data + lnum - 1;
	if (col < 0 || col >= line->sb_cols)
	    cellattr = &line->sb_fill_attr;
	else
	    cellattr = line->sb_cells + col;
    }
    return cell2attr(cellattr->attrs, cellattr->fg, cellattr->bg);
}

static VTermColor ansi_table[16] = {
  {  0,   0,   0,  1}, /* black */
  {224,   0,   0,  2}, /* dark red */
  {  0, 224,   0,  3}, /* dark green */
  {224, 224,   0,  4}, /* dark yellow / brown */
  {  0,   0, 224,  5}, /* dark blue */
  {224,   0, 224,  6}, /* dark magenta */
  {  0, 224, 224,  7}, /* dark cyan */
  {224, 224, 224,  8}, /* light grey */

  {128, 128, 128,  9}, /* dark grey */
  {255,  64,  64, 10}, /* light red */
  { 64, 255,  64, 11}, /* light green */
  {255, 255,  64, 12}, /* yellow */
  { 64,  64, 255, 13}, /* light blue */
  {255,  64, 255, 14}, /* light magenta */
  { 64, 255, 255, 15}, /* light cyan */
  {255, 255, 255, 16}, /* white */
};

static int cube_value[] = {
    0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF
};

static int grey_ramp[] = {
    0x08, 0x12, 0x1C, 0x26, 0x30, 0x3A, 0x44, 0x4E, 0x58, 0x62, 0x6C, 0x76,
    0x80, 0x8A, 0x94, 0x9E, 0xA8, 0xB2, 0xBC, 0xC6, 0xD0, 0xDA, 0xE4, 0xEE
};

/*
 * Convert a cterm color number 0 - 255 to RGB.
 * This is compatible with xterm.
 */
    static void
cterm_color2rgb(int nr, VTermColor *rgb)
{
    int idx;

    if (nr < 16)
    {
	*rgb = ansi_table[nr];
    }
    else if (nr < 232)
    {
	/* 216 color cube */
	idx = nr - 16;
	rgb->blue  = cube_value[idx      % 6];
	rgb->green = cube_value[idx / 6  % 6];
	rgb->red   = cube_value[idx / 36 % 6];
	rgb->ansi_index = VTERM_ANSI_INDEX_NONE;
    }
    else if (nr < 256)
    {
	/* 24 grey scale ramp */
	idx = nr - 232;
	rgb->blue  = grey_ramp[idx];
	rgb->green = grey_ramp[idx];
	rgb->red   = grey_ramp[idx];
	rgb->ansi_index = VTERM_ANSI_INDEX_NONE;
    }
}

/*
 * Create a new vterm and initialize it.
 */
    static void
create_vterm(term_T *term, int rows, int cols)
{
    VTerm	    *vterm;
    VTermScreen	    *screen;
    VTermValue	    value;
    VTermColor	    *fg, *bg;
    int		    fgval, bgval;
    int		    id;

    vterm = vterm_new(rows, cols);
    term->tl_vterm = vterm;
    screen = vterm_obtain_screen(vterm);
    vterm_screen_set_callbacks(screen, &screen_callbacks, term);
    /* TODO: depends on 'encoding'. */
    vterm_set_utf8(vterm, 1);

    vim_memset(&term->tl_default_color.attrs, 0, sizeof(VTermScreenCellAttrs));
    term->tl_default_color.width = 1;
    fg = &term->tl_default_color.fg;
    bg = &term->tl_default_color.bg;

    /* Vterm uses a default black background.  Set it to white when
     * 'background' is "light". */
    if (*p_bg == 'l')
    {
	fgval = 0;
	bgval = 255;
    }
    else
    {
	fgval = 255;
	bgval = 0;
    }
    fg->red = fg->green = fg->blue = fgval;
    bg->red = bg->green = bg->blue = bgval;
    fg->ansi_index = bg->ansi_index = VTERM_ANSI_INDEX_DEFAULT;

    /* The "Terminal" highlight group overrules the defaults. */
    id = syn_name2id((char_u *)"Terminal");

    /* Use the actual color for the GUI and when 'termguicolors' is set. */
#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
    if (0
# ifdef FEAT_GUI
	    || gui.in_use
# endif
# ifdef FEAT_TERMGUICOLORS
	    || p_tgc
# endif
       )
    {
	guicolor_T	fg_rgb = INVALCOLOR;
	guicolor_T	bg_rgb = INVALCOLOR;

	if (id != 0)
	    syn_id2colors(id, &fg_rgb, &bg_rgb);

# ifdef FEAT_GUI
	if (gui.in_use)
	{
	    if (fg_rgb == INVALCOLOR)
		fg_rgb = gui.norm_pixel;
	    if (bg_rgb == INVALCOLOR)
		bg_rgb = gui.back_pixel;
	}
#  ifdef FEAT_TERMGUICOLORS
	else
#  endif
# endif
# ifdef FEAT_TERMGUICOLORS
	{
	    if (fg_rgb == INVALCOLOR)
		fg_rgb = cterm_normal_fg_gui_color;
	    if (bg_rgb == INVALCOLOR)
		bg_rgb = cterm_normal_bg_gui_color;
	}
# endif
	if (fg_rgb != INVALCOLOR)
	{
	    long_u rgb = GUI_MCH_GET_RGB(fg_rgb);

	    fg->red = (unsigned)(rgb >> 16);
	    fg->green = (unsigned)(rgb >> 8) & 255;
	    fg->blue = (unsigned)rgb & 255;
	}
	if (bg_rgb != INVALCOLOR)
	{
	    long_u rgb = GUI_MCH_GET_RGB(bg_rgb);

	    bg->red = (unsigned)(rgb >> 16);
	    bg->green = (unsigned)(rgb >> 8) & 255;
	    bg->blue = (unsigned)rgb & 255;
	}
    }
    else
#endif
    if (id != 0 && t_colors >= 16)
    {
	if (term_default_cterm_fg >= 0)
	    cterm_color2rgb(term_default_cterm_fg, fg);
	if (term_default_cterm_bg >= 0)
	    cterm_color2rgb(term_default_cterm_bg, bg);
    }
    else
    {
#if defined(WIN3264) && !defined(FEAT_GUI_W32)
	int tmp;
#endif

	/* In an MS-Windows console we know the normal colors. */
	if (cterm_normal_fg_color > 0)
	{
	    cterm_color2rgb(cterm_normal_fg_color - 1, fg);
# if defined(WIN3264) && !defined(FEAT_GUI_W32)
	    tmp = fg->red;
	    fg->red = fg->blue;
	    fg->blue = tmp;
# endif
	}
# ifdef FEAT_TERMRESPONSE
	else
	    term_get_fg_color(&fg->red, &fg->green, &fg->blue);
# endif

	if (cterm_normal_bg_color > 0)
	{
	    cterm_color2rgb(cterm_normal_bg_color - 1, bg);
# if defined(WIN3264) && !defined(FEAT_GUI_W32)
	    tmp = bg->red;
	    bg->red = bg->blue;
	    bg->blue = tmp;
# endif
	}
# ifdef FEAT_TERMRESPONSE
	else
	    term_get_bg_color(&bg->red, &bg->green, &bg->blue);
# endif
    }

    vterm_state_set_default_colors(vterm_obtain_state(vterm), fg, bg);

    /* Required to initialize most things. */
    vterm_screen_reset(screen, 1 /* hard */);

    /* Allow using alternate screen. */
    vterm_screen_enable_altscreen(screen, 1);

    /* For unix do not use a blinking cursor.  In an xterm this causes the
     * cursor to blink if it's blinking in the xterm.
     * For Windows we respect the system wide setting. */
#ifdef WIN3264
    if (GetCaretBlinkTime() == INFINITE)
	value.boolean = 0;
    else
	value.boolean = 1;
#else
    value.boolean = 0;
#endif
    vterm_state_set_termprop(vterm_obtain_state(vterm),
					       VTERM_PROP_CURSORBLINK, &value);
}

/*
 * Return the text to show for the buffer name and status.
 */
    char_u *
term_get_status_text(term_T *term)
{
    if (term->tl_status_text == NULL)
    {
	char_u *txt;
	size_t len;

	if (term->tl_normal_mode)
	{
	    if (term_job_running(term))
		txt = (char_u *)_("Terminal");
	    else
		txt = (char_u *)_("Terminal-finished");
	}
	else if (term->tl_title != NULL)
	    txt = term->tl_title;
	else if (term_none_open(term))
	    txt = (char_u *)_("active");
	else if (term_job_running(term))
	    txt = (char_u *)_("running");
	else
	    txt = (char_u *)_("finished");
	len = 9 + STRLEN(term->tl_buffer->b_fname) + STRLEN(txt);
	term->tl_status_text = alloc((int)len);
	if (term->tl_status_text != NULL)
	    vim_snprintf((char *)term->tl_status_text, len, "%s [%s]",
						term->tl_buffer->b_fname, txt);
    }
    return term->tl_status_text;
}

/*
 * Mark references in jobs of terminals.
 */
    int
set_ref_in_term(int copyID)
{
    int		abort = FALSE;
    term_T	*term;
    typval_T	tv;

    for (term = first_term; term != NULL; term = term->tl_next)
	if (term->tl_job != NULL)
	{
	    tv.v_type = VAR_JOB;
	    tv.vval.v_job = term->tl_job;
	    abort = abort || set_ref_in_item(&tv, copyID, NULL, NULL);
	}
    return abort;
}

/*
 * Cache "Terminal" highlight group colors.
 */
    void
set_terminal_default_colors(int cterm_fg, int cterm_bg)
{
    term_default_cterm_fg = cterm_fg - 1;
    term_default_cterm_bg = cterm_bg - 1;
}

/*
 * Get the buffer from the first argument in "argvars".
 * Returns NULL when the buffer is not for a terminal window.
 */
    static buf_T *
term_get_buf(typval_T *argvars)
{
    buf_T *buf;

    (void)get_tv_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = get_buf_tv(&argvars[0], FALSE);
    --emsg_off;
    if (buf == NULL || buf->b_term == NULL)
	return NULL;
    return buf;
}

/*
 * "term_getaltscreen(buf)" function
 */
    void
f_term_getaltscreen(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);

    if (buf == NULL)
	return;
    rettv->vval.v_number = buf->b_term->tl_using_altscreen;
}

/*
 * "term_getattr(attr, name)" function
 */
    void
f_term_getattr(typval_T *argvars, typval_T *rettv)
{
    int	    attr;
    size_t  i;
    char_u  *name;

    static struct {
	char	    *name;
	int	    attr;
    } attrs[] = {
	{"bold",      HL_BOLD},
	{"italic",    HL_ITALIC},
	{"underline", HL_UNDERLINE},
	{"strike",    HL_STRIKETHROUGH},
	{"reverse",   HL_INVERSE},
    };

    attr = get_tv_number(&argvars[0]);
    name = get_tv_string_chk(&argvars[1]);
    if (name == NULL)
	return;

    for (i = 0; i < sizeof(attrs)/sizeof(attrs[0]); ++i)
	if (STRCMP(name, attrs[i].name) == 0)
	{
	    rettv->vval.v_number = (attr & attrs[i].attr) != 0 ? 1 : 0;
	    break;
	}
}

/*
 * "term_getcursor(buf)" function
 */
    void
f_term_getcursor(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);
    term_T	*term;
    list_T	*l;
    dict_T	*d;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    if (buf == NULL)
	return;
    term = buf->b_term;

    l = rettv->vval.v_list;
    list_append_number(l, term->tl_cursor_pos.row + 1);
    list_append_number(l, term->tl_cursor_pos.col + 1);

    d = dict_alloc();
    if (d != NULL)
    {
	dict_add_nr_str(d, "visible", term->tl_cursor_visible, NULL);
	dict_add_nr_str(d, "blink", blink_state_is_inverted()
		       ? !term->tl_cursor_blink : term->tl_cursor_blink, NULL);
	dict_add_nr_str(d, "shape", term->tl_cursor_shape, NULL);
	dict_add_nr_str(d, "color", 0L, term->tl_cursor_color == NULL
				       ? (char_u *)"" : term->tl_cursor_color);
	list_append_dict(l, d);
    }
}

/*
 * "term_getjob(buf)" function
 */
    void
f_term_getjob(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);

    rettv->v_type = VAR_JOB;
    rettv->vval.v_job = NULL;
    if (buf == NULL)
	return;

    rettv->vval.v_job = buf->b_term->tl_job;
    if (rettv->vval.v_job != NULL)
	++rettv->vval.v_job->jv_refcount;
}

    static int
get_row_number(typval_T *tv, term_T *term)
{
    if (tv->v_type == VAR_STRING
	    && tv->vval.v_string != NULL
	    && STRCMP(tv->vval.v_string, ".") == 0)
	return term->tl_cursor_pos.row;
    return (int)get_tv_number(tv) - 1;
}

/*
 * "term_getline(buf, row)" function
 */
    void
f_term_getline(typval_T *argvars, typval_T *rettv)
{
    buf_T	    *buf = term_get_buf(argvars);
    term_T	    *term;
    int		    row;

    rettv->v_type = VAR_STRING;
    if (buf == NULL)
	return;
    term = buf->b_term;
    row = get_row_number(&argvars[1], term);

    if (term->tl_vterm == NULL)
    {
	linenr_T lnum = row + term->tl_scrollback_scrolled + 1;

	/* vterm is finished, get the text from the buffer */
	if (lnum > 0 && lnum <= buf->b_ml.ml_line_count)
	    rettv->vval.v_string = vim_strsave(ml_get_buf(buf, lnum, FALSE));
    }
    else
    {
	VTermScreen	*screen = vterm_obtain_screen(term->tl_vterm);
	VTermRect	rect;
	int		len;
	char_u		*p;

	if (row < 0 || row >= term->tl_rows)
	    return;
	len = term->tl_cols * MB_MAXBYTES + 1;
	p = alloc(len);
	if (p == NULL)
	    return;
	rettv->vval.v_string = p;

	rect.start_col = 0;
	rect.end_col = term->tl_cols;
	rect.start_row = row;
	rect.end_row = row + 1;
	p[vterm_screen_get_text(screen, (char *)p, len, rect)] = NUL;
    }
}

/*
 * "term_getscrolled(buf)" function
 */
    void
f_term_getscrolled(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);

    if (buf == NULL)
	return;
    rettv->vval.v_number = buf->b_term->tl_scrollback_scrolled;
}

/*
 * "term_getsize(buf)" function
 */
    void
f_term_getsize(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);
    list_T	*l;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    if (buf == NULL)
	return;

    l = rettv->vval.v_list;
    list_append_number(l, buf->b_term->tl_rows);
    list_append_number(l, buf->b_term->tl_cols);
}

/*
 * "term_getstatus(buf)" function
 */
    void
f_term_getstatus(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);
    term_T	*term;
    char_u	val[100];

    rettv->v_type = VAR_STRING;
    if (buf == NULL)
	return;
    term = buf->b_term;

    if (term_job_running(term))
	STRCPY(val, "running");
    else
	STRCPY(val, "finished");
    if (term->tl_normal_mode)
	STRCAT(val, ",normal");
    rettv->vval.v_string = vim_strsave(val);
}

/*
 * "term_gettitle(buf)" function
 */
    void
f_term_gettitle(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);

    rettv->v_type = VAR_STRING;
    if (buf == NULL)
	return;

    if (buf->b_term->tl_title != NULL)
	rettv->vval.v_string = vim_strsave(buf->b_term->tl_title);
}

/*
 * "term_gettty(buf)" function
 */
    void
f_term_gettty(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);
    char_u	*p;
    int		num = 0;

    rettv->v_type = VAR_STRING;
    if (buf == NULL)
	return;
    if (argvars[1].v_type != VAR_UNKNOWN)
	num = get_tv_number(&argvars[1]);

    switch (num)
    {
	case 0:
	    if (buf->b_term->tl_job != NULL)
		p = buf->b_term->tl_job->jv_tty_out;
	    else
		p = buf->b_term->tl_tty_out;
	    break;
	case 1:
	    if (buf->b_term->tl_job != NULL)
		p = buf->b_term->tl_job->jv_tty_in;
	    else
		p = buf->b_term->tl_tty_in;
	    break;
	default:
	    EMSG2(_(e_invarg2), get_tv_string(&argvars[1]));
	    return;
    }
    if (p != NULL)
	rettv->vval.v_string = vim_strsave(p);
}

/*
 * "term_list()" function
 */
    void
f_term_list(typval_T *argvars UNUSED, typval_T *rettv)
{
    term_T	*tp;
    list_T	*l;

    if (rettv_list_alloc(rettv) == FAIL || first_term == NULL)
	return;

    l = rettv->vval.v_list;
    for (tp = first_term; tp != NULL; tp = tp->tl_next)
	if (tp != NULL && tp->tl_buffer != NULL)
	    if (list_append_number(l,
				   (varnumber_T)tp->tl_buffer->b_fnum) == FAIL)
		return;
}

/*
 * "term_scrape(buf, row)" function
 */
    void
f_term_scrape(typval_T *argvars, typval_T *rettv)
{
    buf_T	    *buf = term_get_buf(argvars);
    VTermScreen	    *screen = NULL;
    VTermPos	    pos;
    list_T	    *l;
    term_T	    *term;
    char_u	    *p;
    sb_line_T	    *line;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    if (buf == NULL)
	return;
    term = buf->b_term;

    l = rettv->vval.v_list;
    pos.row = get_row_number(&argvars[1], term);

    if (term->tl_vterm != NULL)
    {
	screen = vterm_obtain_screen(term->tl_vterm);
	p = NULL;
	line = NULL;
    }
    else
    {
	linenr_T	lnum = pos.row + term->tl_scrollback_scrolled;

	if (lnum < 0 || lnum >= term->tl_scrollback.ga_len)
	    return;
	p = ml_get_buf(buf, lnum + 1, FALSE);
	line = (sb_line_T *)term->tl_scrollback.ga_data + lnum;
    }

    for (pos.col = 0; pos.col < term->tl_cols; )
    {
	dict_T		*dcell;
	int		width;
	VTermScreenCellAttrs attrs;
	VTermColor	fg, bg;
	char_u		rgb[8];
	char_u		mbs[MB_MAXBYTES * VTERM_MAX_CHARS_PER_CELL + 1];
	int		off = 0;
	int		i;

	if (screen == NULL)
	{
	    cellattr_T	*cellattr;
	    int		len;

	    /* vterm has finished, get the cell from scrollback */
	    if (pos.col >= line->sb_cols)
		break;
	    cellattr = line->sb_cells + pos.col;
	    width = cellattr->width;
	    attrs = cellattr->attrs;
	    fg = cellattr->fg;
	    bg = cellattr->bg;
	    len = MB_PTR2LEN(p);
	    mch_memmove(mbs, p, len);
	    mbs[len] = NUL;
	    p += len;
	}
	else
	{
	    VTermScreenCell cell;
	    if (vterm_screen_get_cell(screen, pos, &cell) == 0)
		break;
	    for (i = 0; i < VTERM_MAX_CHARS_PER_CELL; ++i)
	    {
		if (cell.chars[i] == 0)
		    break;
		off += (*utf_char2bytes)((int)cell.chars[i], mbs + off);
	    }
	    mbs[off] = NUL;
	    width = cell.width;
	    attrs = cell.attrs;
	    fg = cell.fg;
	    bg = cell.bg;
	}
	dcell = dict_alloc();
	list_append_dict(l, dcell);

	dict_add_nr_str(dcell, "chars", 0, mbs);

	vim_snprintf((char *)rgb, 8, "#%02x%02x%02x",
				     fg.red, fg.green, fg.blue);
	dict_add_nr_str(dcell, "fg", 0, rgb);
	vim_snprintf((char *)rgb, 8, "#%02x%02x%02x",
				     bg.red, bg.green, bg.blue);
	dict_add_nr_str(dcell, "bg", 0, rgb);

	dict_add_nr_str(dcell, "attr",
				cell2attr(attrs, fg, bg), NULL);
	dict_add_nr_str(dcell, "width", width, NULL);

	++pos.col;
	if (width == 2)
	    ++pos.col;
    }
}

/*
 * "term_sendkeys(buf, keys)" function
 */
    void
f_term_sendkeys(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = term_get_buf(argvars);
    char_u	*msg;
    term_T	*term;

    rettv->v_type = VAR_UNKNOWN;
    if (buf == NULL)
	return;

    msg = get_tv_string_chk(&argvars[1]);
    if (msg == NULL)
	return;
    term = buf->b_term;
    if (term->tl_vterm == NULL)
	return;

    while (*msg != NUL)
    {
	send_keys_to_term(term, PTR2CHAR(msg), FALSE);
	msg += MB_CPTR2LEN(msg);
    }
}

/*
 * "term_start(command, options)" function
 */
    void
f_term_start(typval_T *argvars, typval_T *rettv)
{
    jobopt_T	opt;
    buf_T	*buf;

    init_job_options(&opt);
    if (argvars[1].v_type != VAR_UNKNOWN
	    && get_job_options(&argvars[1], &opt,
		JO_TIMEOUT_ALL + JO_STOPONEXIT
		    + JO_CALLBACK + JO_OUT_CALLBACK + JO_ERR_CALLBACK
		    + JO_EXIT_CB + JO_CLOSE_CALLBACK + JO_OUT_IO,
		JO2_TERM_NAME + JO2_TERM_FINISH + JO2_HIDDEN + JO2_TERM_OPENCMD
		    + JO2_TERM_COLS + JO2_TERM_ROWS + JO2_VERTICAL + JO2_CURWIN
		    + JO2_CWD + JO2_ENV + JO2_EOF_CHARS) == FAIL)
	return;

    if (opt.jo_vertical)
	cmdmod.split = WSP_VERT;
    buf = term_start(&argvars[0], &opt, FALSE);

    if (buf != NULL && buf->b_term != NULL)
	rettv->vval.v_number = buf->b_fnum;
}

/*
 * "term_wait" function
 */
    void
f_term_wait(typval_T *argvars, typval_T *rettv UNUSED)
{
    buf_T	*buf = term_get_buf(argvars);

    if (buf == NULL)
    {
	ch_log(NULL, "term_wait(): invalid argument");
	return;
    }
    if (buf->b_term->tl_job == NULL)
    {
	ch_log(NULL, "term_wait(): no job to wait for");
	return;
    }
    if (buf->b_term->tl_job->jv_channel == NULL)
	/* channel is closed, nothing to do */
	return;

    /* Get the job status, this will detect a job that finished. */
    if ((buf->b_term->tl_job->jv_channel == NULL
			     || !buf->b_term->tl_job->jv_channel->ch_keep_open)
	    && STRCMP(job_status(buf->b_term->tl_job), "dead") == 0)
    {
	/* The job is dead, keep reading channel I/O until the channel is
	 * closed. buf->b_term may become NULL if the terminal was closed while
	 * waiting. */
	ch_log(NULL, "term_wait(): waiting for channel to close");
	while (buf->b_term != NULL && !buf->b_term->tl_channel_closed)
	{
	    mch_check_messages();
	    parse_queued_messages();
	    if (!buf_valid(buf))
		/* If the terminal is closed when the channel is closed the
		 * buffer disappears. */
		break;
	    ui_delay(10L, FALSE);
	}
	mch_check_messages();
	parse_queued_messages();
    }
    else
    {
	long wait = 10L;

	mch_check_messages();
	parse_queued_messages();

	/* Wait for some time for any channel I/O. */
	if (argvars[1].v_type != VAR_UNKNOWN)
	    wait = get_tv_number(&argvars[1]);
	ui_delay(wait, TRUE);
	mch_check_messages();

	/* Flushing messages on channels is hopefully sufficient.
	 * TODO: is there a better way? */
	parse_queued_messages();
    }
}

/*
 * Called when a channel has sent all the lines to a terminal.
 * Send a CTRL-D to mark the end of the text.
 */
    void
term_send_eof(channel_T *ch)
{
    term_T	*term;

    for (term = first_term; term != NULL; term = term->tl_next)
	if (term->tl_job == ch->ch_job)
	{
	    if (term->tl_eof_chars != NULL)
	    {
		channel_send(ch, PART_IN, term->tl_eof_chars,
					(int)STRLEN(term->tl_eof_chars), NULL);
		channel_send(ch, PART_IN, (char_u *)"\r", 1, NULL);
	    }
# ifdef WIN3264
	    else
		/* Default: CTRL-D */
		channel_send(ch, PART_IN, (char_u *)"\004\r", 2, NULL);
# endif
	}
}

# if defined(WIN3264) || defined(PROTO)

/**************************************
 * 2. MS-Windows implementation.
 */

#  ifndef PROTO

#define WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN 1ul
#define WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN 2ull
#define WINPTY_MOUSE_MODE_FORCE         2

void* (*winpty_config_new)(UINT64, void*);
void* (*winpty_open)(void*, void*);
void* (*winpty_spawn_config_new)(UINT64, void*, LPCWSTR, void*, void*, void*);
BOOL (*winpty_spawn)(void*, void*, HANDLE*, HANDLE*, DWORD*, void*);
void (*winpty_config_set_mouse_mode)(void*, int);
void (*winpty_config_set_initial_size)(void*, int, int);
LPCWSTR (*winpty_conin_name)(void*);
LPCWSTR (*winpty_conout_name)(void*);
LPCWSTR (*winpty_conerr_name)(void*);
void (*winpty_free)(void*);
void (*winpty_config_free)(void*);
void (*winpty_spawn_config_free)(void*);
void (*winpty_error_free)(void*);
LPCWSTR (*winpty_error_msg)(void*);
BOOL (*winpty_set_size)(void*, int, int, void*);
HANDLE (*winpty_agent_process)(void*);

#define WINPTY_DLL "winpty.dll"

static HINSTANCE hWinPtyDLL = NULL;
#  endif

    static int
dyn_winpty_init(int verbose)
{
    int i;
    static struct
    {
	char	    *name;
	FARPROC	    *ptr;
    } winpty_entry[] =
    {
	{"winpty_conerr_name", (FARPROC*)&winpty_conerr_name},
	{"winpty_config_free", (FARPROC*)&winpty_config_free},
	{"winpty_config_new", (FARPROC*)&winpty_config_new},
	{"winpty_config_set_mouse_mode",
				      (FARPROC*)&winpty_config_set_mouse_mode},
	{"winpty_config_set_initial_size",
				    (FARPROC*)&winpty_config_set_initial_size},
	{"winpty_conin_name", (FARPROC*)&winpty_conin_name},
	{"winpty_conout_name", (FARPROC*)&winpty_conout_name},
	{"winpty_error_free", (FARPROC*)&winpty_error_free},
	{"winpty_free", (FARPROC*)&winpty_free},
	{"winpty_open", (FARPROC*)&winpty_open},
	{"winpty_spawn", (FARPROC*)&winpty_spawn},
	{"winpty_spawn_config_free", (FARPROC*)&winpty_spawn_config_free},
	{"winpty_spawn_config_new", (FARPROC*)&winpty_spawn_config_new},
	{"winpty_error_msg", (FARPROC*)&winpty_error_msg},
	{"winpty_set_size", (FARPROC*)&winpty_set_size},
	{"winpty_agent_process", (FARPROC*)&winpty_agent_process},
	{NULL, NULL}
    };

    /* No need to initialize twice. */
    if (hWinPtyDLL)
	return OK;
    /* Load winpty.dll, prefer using the 'winptydll' option, fall back to just
     * winpty.dll. */
    if (*p_winptydll != NUL)
	hWinPtyDLL = vimLoadLib((char *)p_winptydll);
    if (!hWinPtyDLL)
	hWinPtyDLL = vimLoadLib(WINPTY_DLL);
    if (!hWinPtyDLL)
    {
	if (verbose)
	    EMSG2(_(e_loadlib), *p_winptydll != NUL ? p_winptydll
						       : (char_u *)WINPTY_DLL);
	return FAIL;
    }
    for (i = 0; winpty_entry[i].name != NULL
					 && winpty_entry[i].ptr != NULL; ++i)
    {
	if ((*winpty_entry[i].ptr = (FARPROC)GetProcAddress(hWinPtyDLL,
					      winpty_entry[i].name)) == NULL)
	{
	    if (verbose)
		EMSG2(_(e_loadfunc), winpty_entry[i].name);
	    return FAIL;
	}
    }

    return OK;
}

/*
 * Create a new terminal of "rows" by "cols" cells.
 * Store a reference in "term".
 * Return OK or FAIL.
 */
    static int
term_and_job_init(
	term_T	    *term,
	typval_T    *argvar,
	jobopt_T    *opt)
{
    WCHAR	    *cmd_wchar = NULL;
    WCHAR	    *cwd_wchar = NULL;
    WCHAR	    *env_wchar = NULL;
    channel_T	    *channel = NULL;
    job_T	    *job = NULL;
    DWORD	    error;
    HANDLE	    jo = NULL;
    HANDLE	    child_process_handle;
    HANDLE	    child_thread_handle;
    void	    *winpty_err;
    void	    *spawn_config = NULL;
    garray_T	    ga_cmd, ga_env;
    char_u	    *cmd;

    if (dyn_winpty_init(TRUE) == FAIL)
	return FAIL;

    if (argvar->v_type == VAR_STRING)
	cmd = argvar->vval.v_string;
    else
    {
	ga_init2(&ga_cmd, (int)sizeof(char*), 20);
	if (win32_build_cmd(argvar->vval.v_list, &ga_cmd) == FAIL)
	    goto failed;
	cmd = ga_cmd.ga_data;
    }

    cmd_wchar = enc_to_utf16(cmd, NULL);
    if (cmd_wchar == NULL)
	return FAIL;
    if (opt->jo_cwd != NULL)
	cwd_wchar = enc_to_utf16(opt->jo_cwd, NULL);

    ga_init2(&ga_env, (int)sizeof(char*), 20);
    win32_build_env(opt->jo_env, &ga_env, TRUE);
    env_wchar = ga_env.ga_data;

    job = job_alloc();
    if (job == NULL)
	goto failed;

    channel = add_channel();
    if (channel == NULL)
	goto failed;

    term->tl_winpty_config = winpty_config_new(0, &winpty_err);
    if (term->tl_winpty_config == NULL)
	goto failed;

    winpty_config_set_mouse_mode(term->tl_winpty_config,
						    WINPTY_MOUSE_MODE_FORCE);
    winpty_config_set_initial_size(term->tl_winpty_config,
						 term->tl_cols, term->tl_rows);
    term->tl_winpty = winpty_open(term->tl_winpty_config, &winpty_err);
    if (term->tl_winpty == NULL)
	goto failed;

    spawn_config = winpty_spawn_config_new(
	    WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN |
		WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN,
	    NULL,
	    cmd_wchar,
	    cwd_wchar,
	    env_wchar,
	    &winpty_err);
    if (spawn_config == NULL)
	goto failed;

    channel = add_channel();
    if (channel == NULL)
	goto failed;

    job = job_alloc();
    if (job == NULL)
	goto failed;

    if (opt->jo_set & JO_IN_BUF)
	job->jv_in_buf = buflist_findnr(opt->jo_io_buf[PART_IN]);

    if (!winpty_spawn(term->tl_winpty, spawn_config, &child_process_handle,
	    &child_thread_handle, &error, &winpty_err))
	goto failed;

    channel_set_pipes(channel,
	(sock_T)CreateFileW(
	    winpty_conin_name(term->tl_winpty),
	    GENERIC_WRITE, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T)CreateFileW(
	    winpty_conout_name(term->tl_winpty),
	    GENERIC_READ, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T)CreateFileW(
	    winpty_conerr_name(term->tl_winpty),
	    GENERIC_READ, 0, NULL,
	    OPEN_EXISTING, 0, NULL));

    /* Write lines with CR instead of NL. */
    channel->ch_write_text_mode = TRUE;

    jo = CreateJobObject(NULL, NULL);
    if (jo == NULL)
	goto failed;

    if (!AssignProcessToJobObject(jo, child_process_handle))
    {
	/* Failed, switch the way to terminate process with TerminateProcess. */
	CloseHandle(jo);
	jo = NULL;
    }

    winpty_spawn_config_free(spawn_config);
    vim_free(cmd_wchar);
    vim_free(cwd_wchar);

    create_vterm(term, term->tl_rows, term->tl_cols);

    channel_set_job(channel, job, opt);
    job_set_options(job, opt);

    job->jv_channel = channel;
    job->jv_proc_info.hProcess = child_process_handle;
    job->jv_proc_info.dwProcessId = GetProcessId(child_process_handle);
    job->jv_job_object = jo;
    job->jv_status = JOB_STARTED;
    job->jv_tty_in = utf16_to_enc(
	    (short_u*)winpty_conin_name(term->tl_winpty), NULL);
    job->jv_tty_out = utf16_to_enc(
	    (short_u*)winpty_conout_name(term->tl_winpty), NULL);
    ++job->jv_refcount;
    term->tl_job = job;

    return OK;

failed:
    if (argvar->v_type == VAR_LIST)
	vim_free(ga_cmd.ga_data);
    vim_free(ga_env.ga_data);
    vim_free(cmd_wchar);
    vim_free(cwd_wchar);
    if (spawn_config != NULL)
	winpty_spawn_config_free(spawn_config);
    if (channel != NULL)
	channel_clear(channel);
    if (job != NULL)
    {
	job->jv_channel = NULL;
	job_cleanup(job);
    }
    term->tl_job = NULL;
    if (jo != NULL)
	CloseHandle(jo);
    if (term->tl_winpty != NULL)
	winpty_free(term->tl_winpty);
    term->tl_winpty = NULL;
    if (term->tl_winpty_config != NULL)
	winpty_config_free(term->tl_winpty_config);
    term->tl_winpty_config = NULL;
    if (winpty_err != NULL)
    {
	char_u *msg = utf16_to_enc(
				(short_u *)winpty_error_msg(winpty_err), NULL);

	EMSG(msg);
	winpty_error_free(winpty_err);
    }
    return FAIL;
}

    static int
create_pty_only(term_T *term, jobopt_T *options)
{
    HANDLE	    hPipeIn = INVALID_HANDLE_VALUE;
    HANDLE	    hPipeOut = INVALID_HANDLE_VALUE;
    char	    in_name[80], out_name[80];
    channel_T	    *channel = NULL;

    create_vterm(term, term->tl_rows, term->tl_cols);

    vim_snprintf(in_name, sizeof(in_name), "\\\\.\\pipe\\vim-%d-in-%d",
	    GetCurrentProcessId(),
	    curbuf->b_fnum);
    hPipeIn = CreateNamedPipe(in_name, PIPE_ACCESS_OUTBOUND,
	    PIPE_TYPE_MESSAGE | PIPE_NOWAIT,
	    PIPE_UNLIMITED_INSTANCES,
	    0, 0, NMPWAIT_NOWAIT, NULL);
    if (hPipeIn == INVALID_HANDLE_VALUE)
	goto failed;

    vim_snprintf(out_name, sizeof(out_name), "\\\\.\\pipe\\vim-%d-out-%d",
	    GetCurrentProcessId(),
	    curbuf->b_fnum);
    hPipeOut = CreateNamedPipe(out_name, PIPE_ACCESS_INBOUND,
	    PIPE_TYPE_MESSAGE | PIPE_NOWAIT,
	    PIPE_UNLIMITED_INSTANCES,
	    0, 0, 0, NULL);
    if (hPipeOut == INVALID_HANDLE_VALUE)
	goto failed;

    ConnectNamedPipe(hPipeIn, NULL);
    ConnectNamedPipe(hPipeOut, NULL);

    term->tl_job = job_alloc();
    if (term->tl_job == NULL)
	goto failed;
    ++term->tl_job->jv_refcount;

    /* behave like the job is already finished */
    term->tl_job->jv_status = JOB_FINISHED;

    channel = add_channel();
    if (channel == NULL)
	goto failed;
    term->tl_job->jv_channel = channel;
    channel->ch_keep_open = TRUE;
    channel->ch_named_pipe = TRUE;

    channel_set_pipes(channel,
	(sock_T)hPipeIn,
	(sock_T)hPipeOut,
	(sock_T)hPipeOut);
    channel_set_job(channel, term->tl_job, options);
    term->tl_job->jv_tty_in = vim_strsave((char_u*)in_name);
    term->tl_job->jv_tty_out = vim_strsave((char_u*)out_name);

    return OK;

failed:
    if (hPipeIn != NULL)
	CloseHandle(hPipeIn);
    if (hPipeOut != NULL)
	CloseHandle(hPipeOut);
    return FAIL;
}

/*
 * Free the terminal emulator part of "term".
 */
    static void
term_free_vterm(term_T *term)
{
    if (term->tl_winpty != NULL)
	winpty_free(term->tl_winpty);
    term->tl_winpty = NULL;
    if (term->tl_winpty_config != NULL)
	winpty_config_free(term->tl_winpty_config);
    term->tl_winpty_config = NULL;
    if (term->tl_vterm != NULL)
	vterm_free(term->tl_vterm);
    term->tl_vterm = NULL;
}

/*
 * Request size to terminal.
 */
    static void
term_report_winsize(term_T *term, int rows, int cols)
{
    if (term->tl_winpty)
	winpty_set_size(term->tl_winpty, cols, rows, NULL);
}

    int
terminal_enabled(void)
{
    return dyn_winpty_init(FALSE) == OK;
}

# else

/**************************************
 * 3. Unix-like implementation.
 */

/*
 * Create a new terminal of "rows" by "cols" cells.
 * Start job for "cmd".
 * Store the pointers in "term".
 * Return OK or FAIL.
 */
    static int
term_and_job_init(
	term_T	    *term,
	typval_T    *argvar,
	jobopt_T    *opt)
{
    create_vterm(term, term->tl_rows, term->tl_cols);

    term->tl_job = job_start(argvar, opt);
    if (term->tl_job != NULL)
	++term->tl_job->jv_refcount;

    return term->tl_job != NULL
	&& term->tl_job->jv_channel != NULL
	&& term->tl_job->jv_status != JOB_FAILED ? OK : FAIL;
}

    static int
create_pty_only(term_T *term, jobopt_T *opt)
{
    create_vterm(term, term->tl_rows, term->tl_cols);

    term->tl_job = job_alloc();
    if (term->tl_job == NULL)
	return FAIL;
    ++term->tl_job->jv_refcount;

    /* behave like the job is already finished */
    term->tl_job->jv_status = JOB_FINISHED;

    return mch_create_pty_channel(term->tl_job, opt);
}

/*
 * Free the terminal emulator part of "term".
 */
    static void
term_free_vterm(term_T *term)
{
    if (term->tl_vterm != NULL)
	vterm_free(term->tl_vterm);
    term->tl_vterm = NULL;
}

/*
 * Request size to terminal.
 */
    static void
term_report_winsize(term_T *term, int rows, int cols)
{
    /* Use an ioctl() to report the new window size to the job. */
    if (term->tl_job != NULL && term->tl_job->jv_channel != NULL)
    {
	int fd = -1;
	int part;

	for (part = PART_OUT; part < PART_COUNT; ++part)
	{
	    fd = term->tl_job->jv_channel->ch_part[part].ch_fd;
	    if (isatty(fd))
		break;
	}
	if (part < PART_COUNT && mch_report_winsize(fd, rows, cols) == OK)
	    mch_signal_job(term->tl_job, (char_u *)"winch");
    }
}

# endif

#endif /* FEAT_TERMINAL */
