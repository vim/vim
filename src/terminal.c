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
 * - ":term NONE" does not work in MS-Windows.
 * - test for writing lines to terminal job does not work on MS-Windows
 * - implement term_setsize()
 * - add test for giving error for invalid 'termsize' value.
 * - support minimal size when 'termsize' is "rows*cols".
 * - support minimal size when 'termsize' is empty?
 * - GUI: when using tabs, focus in terminal, click on tab does not work.
 * - GUI: when 'confirm' is set and trying to exit Vim, dialog offers to save
 *   changes to "!shell".
 *   (justrajdeep, 2017 Aug 22)
 * - command argument with spaces doesn't work #1999
 *       :terminal ls dir\ with\ spaces
 * - implement job options when starting a terminal.  Allow:
 *	"in_io", "in_top", "in_bot", "in_name", "in_buf"
	"out_io", "out_name", "out_buf", "out_modifiable", "out_msg"
	"err_io", "err_name", "err_buf", "err_modifiable", "err_msg"
 *   Check that something is connected to the terminal.
 *   Test: "cat" reading from a file or buffer
 *	   "ls" writing stdout to a file or buffer
 *	   shell writing stderr to a file or buffer
 * - For the GUI fill termios with default values, perhaps like pangoterm:
 *   http://bazaar.launchpad.net/~leonerd/pangoterm/trunk/view/head:/main.c#L134
 * - if the job in the terminal does not support the mouse, we can use the
 *   mouse in the Terminal window for copy/paste.
 * - when 'encoding' is not utf-8, or the job is using another encoding, setup
 *   conversions.
 * - In the GUI use a terminal emulator for :!cmd.
 * - Copy text in the vterm to the Vim buffer once in a while, so that
 *   completion works.
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
} sb_line_T;

/* typedef term_T in structs.h */
struct terminal_S {
    term_T	*tl_next;

    VTerm	*tl_vterm;
    job_T	*tl_job;
    buf_T	*tl_buffer;

    /* used when tl_job is NULL and only a pty was created */
    int		tl_tty_fd;
    char_u	*tl_tty_name;

    int		tl_normal_mode; /* TRUE: Terminal-Normal mode */
    int		tl_channel_closed;
    int		tl_finish;	/* 'c' for ++close, 'o' for ++open */
    char_u	*tl_opencmd;

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
    int		tl_dirty_row_start; /* -1 if nothing dirty */
    int		tl_dirty_row_end;   /* row below last one to update */

    garray_T	tl_scrollback;
    int		tl_scrollback_scrolled;

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

/* The characters that we know (or assume) that the terminal expects for the
 * backspace and enter keys. */
static int term_backspace_char = BS;
static int term_enter_char = CAR;
static int term_nl_does_cr = FALSE;


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

    opt->jo_io[PART_OUT] = JIO_BUFFER;
    opt->jo_io[PART_ERR] = JIO_BUFFER;
    opt->jo_set |= JO_OUT_IO + JO_ERR_IO;

    opt->jo_modifiable[PART_OUT] = 0;
    opt->jo_modifiable[PART_ERR] = 0;
    opt->jo_set |= JO_OUT_MODIFIABLE + JO_ERR_MODIFIABLE;

    opt->jo_set |= JO_OUT_BUF + JO_ERR_BUF;
}

/*
 * Set job options mandatory for a terminal job.
 */
    static void
setup_job_options(jobopt_T *opt, int rows, int cols)
{
    opt->jo_io_buf[PART_OUT] = curbuf->b_fnum;
    opt->jo_io_buf[PART_ERR] = curbuf->b_fnum;
    opt->jo_pty = TRUE;
    if ((opt->jo_set2 & JO2_TERM_ROWS) == 0)
	opt->jo_term_rows = rows;
    if ((opt->jo_set2 & JO2_TERM_COLS) == 0)
	opt->jo_term_cols = cols;
}

    static void
term_start(typval_T *argvar, jobopt_T *opt, int forceit)
{
    exarg_T	split_ea;
    win_T	*old_curwin = curwin;
    term_T	*term;
    buf_T	*old_curbuf = NULL;
    int		res;

    if (check_restricted() || check_secure())
	return;

    term = (term_T *)alloc_clear(sizeof(term_T));
    if (term == NULL)
	return;
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
	    return;
	}
	if (do_ecmd(0, NULL, NULL, &split_ea, ECMD_ONE,
		     ECMD_HIDE + (forceit ? ECMD_FORCEIT : 0), curwin) == FAIL)
	{
	    vim_free(term);
	    return;
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
	    return;
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
	    return;
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
		|| argvar->vval.v_list->lv_len < 1)
	    cmd = (char_u*)"";
	else
	    cmd = get_tv_string_chk(&argvar->vval.v_list->lv_first->li_tv);

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
		curbuf->b_ffname = p;
		break;
	    }
	}
    }
    curbuf->b_fname = curbuf->b_ffname;

    if (opt->jo_term_opencmd != NULL)
	term->tl_opencmd = vim_strsave(opt->jo_term_opencmd);

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

    if (res == OK)
    {
	/* Get and remember the size we ended up with.  Update the pty. */
	vterm_get_size(term->tl_vterm, &term->tl_rows, &term->tl_cols);
	term_report_winsize(term, term->tl_rows, term->tl_cols);

	/* Make sure we don't get stuck on sending keys to the job, it leads to
	 * a deadlock if the job is waiting for Vim to read. */
	channel_set_nonblock(term->tl_job->jv_channel, PART_IN);

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
    }
}

/*
 * ":terminal": open a terminal window and execute a job in it.
 */
    void
ex_terminal(exarg_T *eap)
{
    typval_T	argvar;
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
	else
	{
	    if (*p)
		*p = NUL;
	    EMSG2(_("E181: Invalid attribute: %s"), cmd);
	    return;
	}
	cmd = skipwhite(p);
    }
    if (cmd == NULL || *cmd == NUL)
	/* Make a copy, an autocommand may set 'shell'. */
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

    argvar.v_type = VAR_STRING;
    argvar.vval.v_string = cmd;
    term_start(&argvar, &opt, eap->forceit);
    vim_free(tofree);
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
    char_u	*p;
    size_t	done;
    size_t	len_now;

    if (term_nl_does_cr)
	vterm_input_write(vterm, (char *)msg, len);
    else
	/* need to convert NL to CR-NL */
	for (done = 0; done < len; done += len_now)
	{
	    for (p = msg + done; p < msg + len; )
	    {
		if (*p == NL)
		    break;
		p += utf_ptr2len_len(p, (int)(len - (p - msg)));
	    }
	    len_now = p - msg - done;
	    vterm_input_write(vterm, (char *)msg + done, len_now);
	    if (p < msg + len && *p == NL)
	    {
		vterm_input_write(vterm, "\r\n", 2);
		++len_now;
	    }
	}

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
	    gui_update_cursor(FALSE, FALSE);
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
					    mouse_col - W_WINCOL(curwin), mod);
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
    int		    mouse = FALSE;

    switch (c)
    {
	case CAR:		c = term_enter_char; break;
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

	case K_MOUSEUP:		mouse = term_send_mouse(vterm, 5, 1); break;
	case K_MOUSEDOWN:	mouse = term_send_mouse(vterm, 4, 1); break;
	case K_MOUSELEFT:	/* TODO */ return 0;
	case K_MOUSERIGHT:	/* TODO */ return 0;

	case K_LEFTMOUSE:
	case K_LEFTMOUSE_NM:	mouse = term_send_mouse(vterm, 1, 1); break;
	case K_LEFTDRAG:	mouse = term_send_mouse(vterm, 1, 1); break;
	case K_LEFTRELEASE:
	case K_LEFTRELEASE_NM:	mouse = term_send_mouse(vterm, 1, 0); break;
	case K_MIDDLEMOUSE:	mouse = term_send_mouse(vterm, 2, 1); break;
	case K_MIDDLEDRAG:	mouse = term_send_mouse(vterm, 2, 1); break;
	case K_MIDDLERELEASE:	mouse = term_send_mouse(vterm, 2, 0); break;
	case K_RIGHTMOUSE:	mouse = term_send_mouse(vterm, 3, 1); break;
	case K_RIGHTDRAG:	mouse = term_send_mouse(vterm, 3, 1); break;
	case K_RIGHTRELEASE:	mouse = term_send_mouse(vterm, 3, 0); break;
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
	case K_PS:		vterm_keyboard_start_paste(vterm); return 0;
	case K_PE:		vterm_keyboard_end_paste(vterm); return 0;
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
    else if (!mouse)
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
    cellattr_T	    *p;
    VTermScreen	    *screen;

    if (term->tl_vterm == NULL)
	return;
    screen = vterm_obtain_screen(term->tl_vterm);
    for (pos.row = 0; pos.row < term->tl_rows; ++pos.row)
    {
	len = 0;
	for (pos.col = 0; pos.col < term->tl_cols; ++pos.col)
	    if (vterm_screen_get_cell(screen, pos, &cell) != 0
						       && cell.chars[0] != NUL)
		len = pos.col + 1;

	if (len == 0)
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
		    ++term->tl_scrollback.ga_len;

		    add_scrollback_line_to_buffer(term, (char_u *)"", 0);
		}
	    }

	    p = (cellattr_T *)alloc((int)sizeof(cellattr_T) * len);
	    if (p != NULL && ga_grow(&term->tl_scrollback, 1) == OK)
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

			p[pos.col].width = cell.width;
			p[pos.col].attrs = cell.attrs;
			p[pos.col].fg = cell.fg;
			p[pos.col].bg = cell.bg;

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
 * TODO: use terminal mode mappings.
 */
    static int
term_vgetc()
{
    int c;

    ++no_mapping;
    ++allow_keys;
    got_int = FALSE;
#ifdef WIN3264
    ctrl_break_was_pressed = FALSE;
#endif
    c = vgetc();
    got_int = FALSE;
    --no_mapping;
    --allow_keys;
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
		    || mouse_col < W_WINCOL(curwin)
		    || mouse_col >= W_ENDCOL(curwin)
		    || dragging_outside)
	    {
		/* click or scroll outside the current window */
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
use_terminal_cursor()
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
 * Return when the start of a CTRL-W command is typed or anything else that
 * should be handled as a Normal mode command.
 * Returns OK if a typed character is to be handled in Normal mode, FAIL if
 * the terminal was closed.
 */
    int
terminal_loop(void)
{
    int		c;
    int		termkey = 0;
    int		ret;

    /* Remember the terminal we are sending keys to.  However, the terminal
     * might be closed while waiting for a character, e.g. typing "exit" in a
     * shell and ++close was used.  Therefore use curbuf->b_term instead of a
     * stored reference. */
    in_terminal_loop = curbuf->b_term;

    if (*curwin->w_p_tk != NUL)
	termkey = string_to_key(curwin->w_p_tk, TRUE);
    position_cursor(curwin, &curbuf->b_term->tl_cursor_pos);
    may_set_cursor_props(curbuf->b_term);

#ifdef UNIX
    {
	int part = get_tty_part(curbuf->b_term);
	int fd = curbuf->b_term->tl_job->jv_channel->ch_part[part].ch_fd;

	if (isatty(fd))
	{
	    ttyinfo_T info;

	    /* Get the current backspace and enter characters of the pty. */
	    if (get_tty_info(fd, &info) == OK)
	    {
		term_backspace_char = info.backspace;
		term_enter_char = info.enter;
		term_nl_does_cr = info.nl_does_cr;
	    }
	}
    }
#endif

    for (;;)
    {
	/* TODO: skip screen update when handling a sequence of keys. */
	/* Repeat redrawing in case a message is received while redrawing. */
	while (curwin->w_redr_type != 0)
	    update_screen(0);
	update_cursor(curbuf->b_term, FALSE);

	c = term_vgetc();
	if (!term_use_loop())
	    /* job finished while waiting for a character */
	    break;
	if (c == K_IGNORE)
	    continue;

#ifdef WIN3264
	/* On Windows winpty handles CTRL-C, don't send a CTRL_C_EVENT.
	 * Use CTRL-BREAK to kill the job. */
	if (ctrl_break_was_pressed)
	    mch_signal_job(curbuf->b_term->tl_job, (char_u *)"kill");
#endif

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
 * First color is 1.  Return 0 if no match found.
 */
    static int
color2index(VTermColor *color, int fg, int *boldp)
{
    int red = color->red;
    int blue = color->blue;
    int green = color->green;

    /* The argument for lookup_color() is for the color_names[] table. */
    if (red == 0)
    {
	if (green == 0)
	{
	    if (blue == 0)
		return lookup_color(0, fg, boldp) + 1; /* black */
	    if (blue == 224)
		return lookup_color(1, fg, boldp) + 1; /* dark blue */
	}
	else if (green == 224)
	{
	    if (blue == 0)
		return lookup_color(2, fg, boldp) + 1; /* dark green */
	    if (blue == 224)
		return lookup_color(3, fg, boldp) + 1; /* dark cyan */
	}
    }
    else if (red == 224)
    {
	if (green == 0)
	{
	    if (blue == 0)
		return lookup_color(4, fg, boldp) + 1; /* dark red */
	    if (blue == 224)
		return lookup_color(5, fg, boldp) + 1; /* dark magenta */
	}
	else if (green == 224)
	{
	    if (blue == 0)
		return lookup_color(6, fg, boldp) + 1; /* dark yellow / brown */
	    if (blue == 224)
		return lookup_color(8, fg, boldp) + 1; /* white / light grey */
	}
    }
    else if (red == 128)
    {
	if (green == 128 && blue == 128)
	    return lookup_color(12, fg, boldp) + 1; /* high intensity black / dark grey */
    }
    else if (red == 255)
    {
	if (green == 64)
	{
	    if (blue == 64)
		return lookup_color(20, fg, boldp) + 1;  /* light red */
	    if (blue == 255)
		return lookup_color(22, fg, boldp) + 1;  /* light magenta */
	}
	else if (green == 255)
	{
	    if (blue == 64)
		return lookup_color(24, fg, boldp) + 1;  /* yellow */
	    if (blue == 255)
		return lookup_color(26, fg, boldp) + 1;  /* white */
	}
    }
    else if (red == 64)
    {
	if (green == 64)
	{
	    if (blue == 255)
		return lookup_color(14, fg, boldp) + 1;  /* light blue */
	}
	else if (green == 255)
	{
	    if (blue == 64)
		return lookup_color(16, fg, boldp) + 1;  /* light green */
	    if (blue == 255)
		return lookup_color(18, fg, boldp) + 1;  /* light cyan */
	}
    }
    if (t_colors >= 256)
    {
	if (red == blue && red == green)
	{
	    /* 24-color greyscale */
	    static int cutoff[23] = {
		0x05, 0x10, 0x1B, 0x26, 0x31, 0x3C, 0x47, 0x52,
		0x5D, 0x68, 0x73, 0x7F, 0x8A, 0x95, 0xA0, 0xAB,
		0xB6, 0xC1, 0xCC, 0xD7, 0xE2, 0xED, 0xF9};
	    int i;

	    for (i = 0; i < 23; ++i)
		if (red < cutoff[i])
		    return i + 233;
	    return 256;
	}

	/* 216-color cube */
	return 17 + ((red + 25) / 0x33) * 36
		  + ((green + 25) / 0x33) * 6
		  + (blue + 25) / 0x33;
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
	attr |= HL_STANDOUT;
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
    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer == term->tl_buffer)
	{
	    win_setheight_win(rows, wp);
	    win_setwidth_win(cols, wp);
	}
    }

    redraw_buf_later(term->tl_buffer, NOT_VALID);
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

	/* do not store empty cells at the end */
	for (i = 0; i < cols; ++i)
	    if (cells[i].chars[0] != 0)
		len = i + 1;

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
		p[col].width = cells[col].width;
		p[col].attrs = cells[col].attrs;
		p[col].fg = cells[col].fg;
		p[col].bg = cells[col].bg;
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
		    /* ++close or term_finish == "close" */
		    ch_log(NULL, "terminal job finished, closing window");
		    curbuf = term->tl_buffer;
		    do_bufdel(DOBUF_WIPE, (char_u *)"", 1, fnum, fnum, FALSE);
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

	vterm_set_size(vterm, rows, cols);
	ch_log(term->tl_job->jv_channel, "Resizing terminal to %d lines",
									 rows);
	term_report_winsize(term, rows, cols);
    }

    /* The cursor may have been moved when resizing. */
    vterm_state_get_cursorpos(state, &pos);
    position_cursor(wp, &pos);

    /* TODO: Only redraw what changed. */
    for (pos.row = 0; pos.row < wp->w_height; ++pos.row)
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

		/* TODO: composing chars */
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
			if (c >= 0x80)
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
 */
    int
term_get_attr(buf_T *buf, linenr_T lnum, int col)
{
    term_T	*term = buf->b_term;
    sb_line_T	*line;
    cellattr_T	*cellattr;

    if (lnum > term->tl_scrollback.ga_len)
	return 0;
    line = (sb_line_T *)term->tl_scrollback.ga_data + lnum - 1;
    if (col >= line->sb_cols)
	return 0;
    cellattr = line->sb_cells + col;
    return cell2attr(cellattr->attrs, cellattr->fg, cellattr->bg);
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

    vterm = vterm_new(rows, cols);
    term->tl_vterm = vterm;
    screen = vterm_obtain_screen(vterm);
    vterm_screen_set_callbacks(screen, &screen_callbacks, term);
    /* TODO: depends on 'encoding'. */
    vterm_set_utf8(vterm, 1);

    /* Vterm uses a default black background.  Set it to white when
     * 'background' is "light". */
    if (*p_bg == 'l')
    {
	VTermColor	fg, bg;

	fg.red = fg.green = fg.blue = 0;
	bg.red = bg.green = bg.blue = 255;
	vterm_state_set_default_colors(vterm_obtain_state(vterm), &fg, &bg);
    }

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
	{"strike",    HL_STANDOUT},
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

    rettv->v_type = VAR_STRING;
    if (buf == NULL)
	return;
    if (buf->b_term->tl_job != NULL)
	p = buf->b_term->tl_job->jv_tty_name;
    else
	p = buf->b_term->tl_tty_name;
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
	msg += MB_PTR2LEN(msg);
    }
}

/*
 * "term_start(command, options)" function
 */
    void
f_term_start(typval_T *argvars, typval_T *rettv)
{
    jobopt_T	opt;

    init_job_options(&opt);
    /* TODO: allow more job options */
    if (argvars[1].v_type != VAR_UNKNOWN
	    && get_job_options(&argvars[1], &opt,
		JO_TIMEOUT_ALL + JO_STOPONEXIT
		    + JO_EXIT_CB + JO_CLOSE_CALLBACK,
		JO2_TERM_NAME + JO2_TERM_FINISH + JO2_HIDDEN + JO2_TERM_OPENCMD
		    + JO2_TERM_COLS + JO2_TERM_ROWS + JO2_VERTICAL + JO2_CURWIN
		    + JO2_CWD + JO2_ENV) == FAIL)
	return;

    if (opt.jo_vertical)
	cmdmod.split = WSP_VERT;
    term_start(&argvars[0], &opt, FALSE);

    if (curbuf->b_term != NULL)
	rettv->vval.v_number = curbuf->b_fnum;
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
	 * closed. */
	ch_log(NULL, "term_wait(): waiting for channel to close");
	while (buf->b_term != NULL && !buf->b_term->tl_channel_closed)
	{
	    mch_check_messages();
	    parse_queued_messages();
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

# if defined(WIN3264) || defined(PROTO)

/**************************************
 * 2. MS-Windows implementation.
 */

#  ifndef PROTO

#define WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN 1ul
#define WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN 2ull

void* (*winpty_config_new)(UINT64, void*);
void* (*winpty_open)(void*, void*);
void* (*winpty_spawn_config_new)(UINT64, void*, LPCWSTR, void*, void*, void*);
BOOL (*winpty_spawn)(void*, void*, HANDLE*, HANDLE*, DWORD*, void*);
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
	{"winpty_config_set_initial_size", (FARPROC*)&winpty_config_set_initial_size},
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
    channel_T	    *channel = NULL;
    job_T	    *job = NULL;
    DWORD	    error;
    HANDLE	    jo = NULL;
    HANDLE	    child_process_handle;
    HANDLE	    child_thread_handle;
    void	    *winpty_err;
    void	    *spawn_config = NULL;
    char	    buf[MAX_PATH];
    garray_T	    ga;
    char_u	    *cmd;

    if (dyn_winpty_init(TRUE) == FAIL)
	return FAIL;

    if (argvar->v_type == VAR_STRING)
	cmd = argvar->vval.v_string;
    else
    {
	ga_init2(&ga, (int)sizeof(char*), 20);
	if (win32_build_cmd(argvar->vval.v_list, &ga) == FAIL)
	    goto failed;
	cmd = ga.ga_data;
    }

    cmd_wchar = enc_to_utf16(cmd, NULL);
    if (cmd_wchar == NULL)
	return FAIL;

    job = job_alloc();
    if (job == NULL)
	goto failed;

    channel = add_channel();
    if (channel == NULL)
	goto failed;

    term->tl_winpty_config = winpty_config_new(0, &winpty_err);
    if (term->tl_winpty_config == NULL)
	goto failed;

    winpty_config_set_initial_size(term->tl_winpty_config,
						 term->tl_cols, term->tl_rows);
    term->tl_winpty = winpty_open(term->tl_winpty_config, &winpty_err);
    if (term->tl_winpty == NULL)
	goto failed;

    /* TODO: if the command is "NONE" only create a pty. */
    spawn_config = winpty_spawn_config_new(
	    WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN |
		WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN,
	    NULL,
	    cmd_wchar,
	    NULL,
	    NULL,
	    &winpty_err);
    if (spawn_config == NULL)
	goto failed;

    channel = add_channel();
    if (channel == NULL)
	goto failed;

    job = job_alloc();
    if (job == NULL)
	goto failed;

    /* TODO: when all lines are written and the fd is closed, the command
     * doesn't get EOF and hangs. */
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

    create_vterm(term, term->tl_rows, term->tl_cols);

    channel_set_job(channel, job, opt);
    job_set_options(job, opt);

    job->jv_channel = channel;
    job->jv_proc_info.hProcess = child_process_handle;
    job->jv_proc_info.dwProcessId = GetProcessId(child_process_handle);
    job->jv_job_object = jo;
    job->jv_status = JOB_STARTED;
    sprintf(buf, "winpty://%lu",
	    GetProcessId(winpty_agent_process(term->tl_winpty)));
    job->jv_tty_name = vim_strsave((char_u*)buf);
    ++job->jv_refcount;
    term->tl_job = job;

    return OK;

failed:
    if (argvar->v_type == VAR_LIST)
	vim_free(ga.ga_data);
    if (cmd_wchar != NULL)
	vim_free(cmd_wchar);
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
create_pty_only(term_T *term, jobopt_T *opt)
{
    /* TODO: implement this */
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

    /* TODO: if the command is "NONE" only create a pty. */
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
    int ret;

    create_vterm(term, term->tl_rows, term->tl_cols);

    term->tl_job = job_alloc();
    if (term->tl_job == NULL)
	return FAIL;
    ++term->tl_job->jv_refcount;

    /* behave like the job is already finished */
    term->tl_job->jv_status = JOB_FINISHED;

    ret = mch_create_pty_channel(term->tl_job, opt);

    return ret;
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
