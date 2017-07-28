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
 * that library is in the libvterm directory.
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
 * TODO:
 * - To set BS correctly, check get_stty(); Pass the fd of the pty.
 * - include functions from #1871
 * - do not store terminal buffer in viminfo.  Or prefix term:// ?
 * - Add a scrollback buffer (contains lines to scroll off the top).
 *   Can use the buf_T lines, store attributes somewhere else?
 * - When the job ends:
 *   - Write "-- JOB ENDED --" in the terminal.
 *   - Put the terminal contents in the scrollback buffer.
 *   - Free the terminal emulator.
 *   - Display the scrollback buffer (but with attributes).
 *     Make the buffer not modifiable, drop attributes when making changes.
 *   - Need an option or argument to drop the window+buffer right away, to be
 *     used for a shell or Vim.
 * - add a character in :ls output
 * - when closing window and job has not ended, make terminal hidden?
 * - don't allow exiting Vim when a terminal is still running a job
 * - use win_del_lines() to make scroll-up efficient.
 * - add test for giving error for invalid 'termsize' value.
 * - support minimal size when 'termsize' is "rows*cols".
 * - support minimal size when 'termsize' is empty?
 * - implement "term" for job_start(): more job options when starting a
 *   terminal.
 * - implement term_list()			list of buffers with a terminal
 * - implement term_getsize(buf)
 * - implement term_setsize(buf)
 * - implement term_sendkeys(buf, keys)		send keystrokes to a terminal
 * - implement term_wait(buf)			wait for screen to be updated
 * - implement term_scrape(buf, row)		inspect terminal screen
 * - implement term_open(command, options)	open terminal window
 * - implement term_getjob(buf)
 * - when 'encoding' is not utf-8, or the job is using another encoding, setup
 *   conversions.
 * - In the GUI use a terminal emulator for :!cmd.
 */

#include "vim.h"

#ifdef FEAT_TERMINAL

#ifdef WIN3264
# define MIN(x,y) (x < y ? x : y)
# define MAX(x,y) (x > y ? x : y)
#endif

#include "libvterm/include/vterm.h"

/* typedef term_T in structs.h */
struct terminal_S {
    term_T	*tl_next;

#ifdef WIN3264
    void	*tl_winpty_config;
    void	*tl_winpty;
#endif
    VTerm	*tl_vterm;
    job_T	*tl_job;
    buf_T	*tl_buffer;

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

    pos_T	tl_cursor;
    int		tl_cursor_visible;
};

/*
 * List of all active terminals.
 */
static term_T *first_term = NULL;


#define MAX_ROW 999999	    /* used for tl_dirty_row_end to update all rows */
#define KEY_BUF_LEN 200

/*
 * Functions with separate implementation for MS-Windows and Unix-like systems.
 */
static int term_and_job_init(term_T *term, int rows, int cols, char_u *cmd);
static void term_report_winsize(term_T *term, int rows, int cols);
static void term_free(term_T *term);

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
 * ":terminal": open a terminal window and execute a job in it.
 */
    void
ex_terminal(exarg_T *eap)
{
    exarg_T	split_ea;
    win_T	*old_curwin = curwin;
    term_T	*term;
    char_u	*cmd = eap->arg;

    if (check_restricted() || check_secure())
	return;

    term = (term_T *)alloc_clear(sizeof(term_T));
    if (term == NULL)
	return;
    term->tl_dirty_row_end = MAX_ROW;
    term->tl_cursor_visible = TRUE;

    /* Open a new window or tab. */
    vim_memset(&split_ea, 0, sizeof(split_ea));
    split_ea.cmdidx = CMD_new;
    split_ea.cmd = (char_u *)"new";
    split_ea.arg = (char_u *)"";
    ex_splitview(&split_ea);
    if (curwin == old_curwin)
    {
	/* split failed */
	vim_free(term);
	return;
    }
    term->tl_buffer = curbuf;
    curbuf->b_term = term;

    /* Link the new terminal in the list of active terminals. */
    term->tl_next = first_term;
    first_term = term;

    if (cmd == NULL || *cmd == NUL)
	cmd = p_sh;

    if (buflist_findname(cmd) == NULL)
	curbuf->b_ffname = vim_strsave(cmd);
    else
    {
	int	i;
	size_t	len = STRLEN(cmd) + 10;
	char_u	*p = alloc((int)len);

	for (i = 1; p != NULL; ++i)
	{
	    vim_snprintf((char *)p, len, "%s (%d)", cmd, i);
	    if (buflist_findname(p) == NULL)
	    {
		curbuf->b_ffname = p;
		break;
	    }
	}
    }
    curbuf->b_fname = curbuf->b_ffname;

    /* Mark the buffer as changed, so that it's not easy to abandon the job. */
    curbuf->b_changed = TRUE;
    curbuf->b_p_ma = FALSE;
    set_string_option_direct((char_u *)"buftype", -1,
				  (char_u *)"terminal", OPT_FREE|OPT_LOCAL, 0);

    set_term_and_win_size(term);

    /* System dependent: setup the vterm and start the job in it. */
    if (term_and_job_init(term, term->tl_rows, term->tl_cols, cmd) == OK)
    {
	/* store the size we ended up with */
	vterm_get_size(term->tl_vterm, &term->tl_rows, &term->tl_cols);
    }
    else
    {
	free_terminal(term);
	curbuf->b_term = NULL;

	/* Wiping out the buffer will also close the window and call
	 * free_terminal(). */
	do_buffer(DOBUF_WIPE, DOBUF_CURRENT, FORWARD, 0, TRUE);
    }

    /* TODO: Setup pty, see mch_call_shell(). */
}

/*
 * Free a terminal and everything it refers to.
 * Kills the job if there is one.
 * Called when wiping out a buffer.
 */
    void
free_terminal(term_T *term)
{
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
				      && term->tl_job->jv_status != JOB_FAILED)
	    job_stop(term->tl_job, NULL, "kill");
	job_unref(term->tl_job);
    }

    term_free(term);
    vim_free(term->tl_title);
    vim_free(term->tl_status_text);
    vim_free(term);
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
	    /* Convert NL to CR-NL, that appears to work best. */
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
    setcursor();
    if (redraw && term->tl_buffer == curbuf)
    {
	if (term->tl_cursor_visible)
	    cursor_on();
	out_flush();
#ifdef FEAT_GUI
	if (gui.in_use && term->tl_cursor_visible)
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

    ch_logn(channel, "writing %d bytes to terminal", (int)len);
    term_write_job_output(term, msg, len);

    /* TODO: only update once in a while. */
    update_screen(0);
    update_cursor(term, TRUE);
}

/*
 * Convert typed key "c" into bytes to send to the job.
 * Return the number of bytes in "buf".
 */
    static int
term_convert_key(int c, char *buf)
{
    VTerm	    *vterm = curbuf->b_term->tl_vterm;
    VTermKey	    key = VTERM_KEY_NONE;
    VTermModifier   mod = VTERM_MOD_NONE;

    switch (c)
    {
	case CAR:		key = VTERM_KEY_ENTER; break;
	case ESC:		key = VTERM_KEY_ESCAPE; break;
				/* VTERM_KEY_BACKSPACE becomes 0x7f DEL */
	case K_BS:		c = BS; break;
	case K_DEL:		key = VTERM_KEY_DEL; break;
	case K_DOWN:		key = VTERM_KEY_DOWN; break;
	case K_END:		key = VTERM_KEY_END; break;
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
	case K_PAGEDOWN:	key = VTERM_KEY_PAGEDOWN; break;
	case K_PAGEUP:		key = VTERM_KEY_PAGEUP; break;
	case K_RIGHT:		key = VTERM_KEY_RIGHT; break;
	case K_UP:		key = VTERM_KEY_UP; break;
	case TAB:		key = VTERM_KEY_TAB; break;

	case K_MOUSEUP:		/* TODO */ break;
	case K_MOUSEDOWN:	/* TODO */ break;
	case K_MOUSELEFT:	/* TODO */ break;
	case K_MOUSERIGHT:	/* TODO */ break;

	case K_LEFTMOUSE:	/* TODO */ break;
	case K_LEFTMOUSE_NM:	/* TODO */ break;
	case K_LEFTDRAG:	/* TODO */ break;
	case K_LEFTRELEASE:	/* TODO */ break;
	case K_LEFTRELEASE_NM:	/* TODO */ break;
	case K_MIDDLEMOUSE:	/* TODO */ break;
	case K_MIDDLEDRAG:	/* TODO */ break;
	case K_MIDDLERELEASE:	/* TODO */ break;
	case K_RIGHTMOUSE:	/* TODO */ break;
	case K_RIGHTDRAG:	/* TODO */ break;
	case K_RIGHTRELEASE:	/* TODO */ break;
	case K_X1MOUSE:		/* TODO */ break;
	case K_X1DRAG:		/* TODO */ break;
	case K_X1RELEASE:	/* TODO */ break;
	case K_X2MOUSE:		/* TODO */ break;
	case K_X2DRAG:		/* TODO */ break;
	case K_X2RELEASE:	/* TODO */ break;

        /* TODO: handle all special keys and modifiers that terminal_loop()
	 * does not handle. */
    }

    /*
     * Convert special keys to vterm keys:
     * - Write keys to vterm: vterm_keyboard_key()
     * - Write output to channel.
     */
    if (key != VTERM_KEY_NONE)
	/* Special key, let vterm convert it. */
	vterm_keyboard_key(vterm, key, mod);
    else
	/* Normal character, let vterm convert it. */
	vterm_keyboard_unichar(vterm, c, mod);

    /* Read back the converted escape sequence. */
    return (int)vterm_output_read(vterm, buf, KEY_BUF_LEN);
}

/*
 * Get a key from the user without mapping.
 * TODO: use terminal mode mappings.
 */
    static int
term_vgetc()
{
    int c;

    ++no_mapping;
    ++allow_keys;
    got_int = FALSE;
    c = vgetc();
    --no_mapping;
    --allow_keys;
    return c;
}

/*
 * Wait for input and send it to the job.
 * Return when the start of a CTRL-W command is typed or anything else that
 * should be handled as a Normal mode command.
 */
    void
terminal_loop(void)
{
    char	buf[KEY_BUF_LEN];
    int		c;
    size_t	len;
    static int	mouse_was_outside = FALSE;
    int		dragging_outside = FALSE;
    int		termkey = 0;

    if (*curwin->w_p_tk != NUL)
	termkey = string_to_key(curwin->w_p_tk, TRUE);

    for (;;)
    {
	/* TODO: skip screen update when handling a sequence of keys. */
	update_screen(0);
	update_cursor(curbuf->b_term, FALSE);
	c = term_vgetc();

	if (c == (termkey == 0 ? Ctrl_W : termkey))
	{
#ifdef FEAT_CMDL_INFO
	    if (add_to_showcmd(c))
		out_flush();
#endif
	    c = term_vgetc();
#ifdef FEAT_CMDL_INFO
	    clear_showcmd();
#endif

	    if (termkey == 0 && c == '.')
		/* "CTRL-W .": send CTRL-W to the job */
		c = Ctrl_W;
	    else if (termkey == 0 || c != termkey)
	    {
		stuffcharReadbuff(Ctrl_W);
		stuffcharReadbuff(c);
		return;
	    }
	}

	/* Catch keys that need to be handled as in Normal mode. */
	switch (c)
	{
	    case NUL:
	    case K_ZERO:
		stuffcharReadbuff(c);
		return;

	    case K_IGNORE: continue;

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
		if (mouse_row < W_WINROW(curwin)
			|| mouse_row >= (W_WINROW(curwin) + curwin->w_height)
			|| mouse_col < W_WINCOL(curwin)
			|| mouse_col >= W_ENDCOL(curwin)
			|| dragging_outside)
		{
		    /* click outside the current window */
		    stuffcharReadbuff(c);
		    mouse_was_outside = TRUE;
		    return;
		}
	}
	mouse_was_outside = FALSE;

	/* Convert the typed key to a sequence of bytes for the job. */
	len = term_convert_key(c, buf);
	if (len > 0)
	    /* TODO: if FAIL is returned, stop? */
	    channel_send(curbuf->b_term->tl_job->jv_channel, PART_IN,
						     (char_u *)buf, (int)len, NULL);
    }
}

/*
 * Called when a job has finished.
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

/*
 * Return TRUE if the job for "buf" is still running.
 */
    static int
term_job_running(term_T *term)
{
    return term->tl_job != NULL && term->tl_job->jv_status == JOB_STARTED;
}

    static void
position_cursor(win_T *wp, VTermPos *pos)
{
    wp->w_wrow = MIN(pos->row, MAX(0, wp->w_height - 1));
    wp->w_wcol = MIN(pos->col, MAX(0, wp->w_width - 1));
}

    static void
may_toggle_cursor(term_T *term)
{
    if (curbuf == term->tl_buffer)
    {
	if (term->tl_cursor_visible)
	    cursor_on();
	else
	    cursor_off();
    }
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
handle_moverect(VTermRect dest UNUSED, VTermRect src UNUSED, void *user)
{
    term_T	*term = (term_T *)user;

    /* TODO */
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
    int		is_current = FALSE;

    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer == term->tl_buffer)
	{
	    position_cursor(wp, &pos);
	    if (wp == curwin)
		is_current = TRUE;
	}
    }

    term->tl_cursor_visible = visible;
    if (is_current)
    {
	may_toggle_cursor(term);
	update_cursor(term, TRUE);
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

static VTermScreenCallbacks screen_callbacks = {
  handle_damage,	/* damage */
  handle_moverect,	/* moverect */
  handle_movecursor,	/* movecursor */
  handle_settermprop,	/* settermprop */
  NULL,			/* bell */
  handle_resize,	/* resize */
  NULL,			/* sb_pushline */
  NULL			/* sb_popline */
};

/*
 * Reverse engineer the RGB value into a cterm color index.
 * First color is 1.  Return 0 if no match found.
 */
    static int
color2index(VTermColor *color)
{
    int red = color->red;
    int blue = color->blue;
    int green = color->green;

#if defined(WIN3264) && !defined(FEAT_GUI)
    int	tmp = red;
    red = blue;
    blue = tmp;
#endif

    if (red == 0)
    {
	if (green == 0)
	{
	    if (blue == 0)
		return 1; /* black */
	    if (blue == 224)
		return 5; /* blue */
	}
	else if (green == 224)
	{
	    if (blue == 0)
		return 3; /* green */
	    if (blue == 224)
		return 7; /* cyan */
	}
    }
    else if (red == 224)
    {
	if (green == 0)
	{
	    if (blue == 0)
		return 2; /* red */
	    if (blue == 224)
		return 6; /* magenta */
	}
	else if (green == 224)
	{
	    if (blue == 0)
		return 4; /* yellow */
	    if (blue == 224)
		return 8; /* white */
	}
    }
    else if (red == 128)
    {
	if (green == 128 && blue == 128)
	    return 9; /* high intensity black */
    }
    else if (red == 255)
    {
	if (green == 64)
	{
	    if (blue == 64)
		return 10;  /* high intensity red */
	    if (blue == 255)
		return 14;  /* high intensity magenta */
	}
	else if (green == 255)
	{
	    if (blue == 64)
		return 12;  /* high intensity yellow */
	    if (blue == 255)
		return 16;  /* high intensity white */
	}
    }
    else if (red == 64)
    {
	if (green == 64)
	{
	    if (blue == 255)
		return 13;  /* high intensity blue */
	}
	else if (green == 255)
	{
	    if (blue == 64)
		return 11;  /* high intensity green */
	    if (blue == 255)
		return 15;  /* high intensity cyan */
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
cell2attr(VTermScreenCell *cell)
{
    int attr = 0;

    if (cell->attrs.bold)
	attr |= HL_BOLD;
    if (cell->attrs.underline)
	attr |= HL_UNDERLINE;
    if (cell->attrs.italic)
	attr |= HL_ITALIC;
    if (cell->attrs.strike)
	attr |= HL_STANDOUT;
    if (cell->attrs.reverse)
	attr |= HL_INVERSE;
    if (cell->attrs.strike)
	attr |= HL_UNDERLINE;

#ifdef FEAT_GUI
    if (gui.in_use)
    {
	guicolor_T fg, bg;

	fg = gui_mch_get_rgb_color(cell->fg.red, cell->fg.green, cell->fg.blue);
	bg = gui_mch_get_rgb_color(cell->bg.red, cell->bg.green, cell->bg.blue);
	return get_gui_attr_idx(attr, fg, bg);
    }
    else
#endif
#ifdef FEAT_TERMGUICOLORS
    if (p_tgc)
    {
	guicolor_T fg, bg;

	fg = gui_get_rgb_color_cmn(cell->fg.red, cell->fg.green, cell->fg.blue);
	bg = gui_get_rgb_color_cmn(cell->bg.red, cell->bg.green, cell->bg.blue);

	return get_tgc_attr_idx(attr, fg, bg);
    }
    else
#endif
    {
	return get_cterm_attr_idx(attr, color2index(&cell->fg),
						       color2index(&cell->bg));
    }
    return 0;
}

/*
 * Called to update the window that contains the terminal.
 */
    void
term_update_window(win_T *wp)
{
    term_T	*term = wp->w_buffer->b_term;
    VTerm	*vterm = term->tl_vterm;
    VTermScreen *screen = vterm_obtain_screen(vterm);
    VTermState	*state = vterm_obtain_state(vterm);
    VTermPos	pos;

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
	ch_logn(term->tl_job->jv_channel, "Resizing terminal to %d lines",
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
#if defined(FEAT_MBYTE)
		    if (enc_utf8)
			ScreenLinesUC[off] = NUL;
#endif
		}
		else
		{
#if defined(FEAT_MBYTE)
		    if (enc_utf8 && c >= 0x80)
		    {
			ScreenLines[off] = ' ';
			ScreenLinesUC[off] = c;
		    }
		    else
		    {
			ScreenLines[off] = c;
			if (enc_utf8)
			    ScreenLinesUC[off] = NUL;
		    }
#else
		    ScreenLines[off] = c;
#endif
		}
		ScreenAttrs[off] = cell2attr(&cell);

		++pos.col;
		++off;
		if (cell.width == 2)
		{
		    ScreenLines[off] = NUL;
#if defined(FEAT_MBYTE)
		    if (enc_utf8)
			ScreenLinesUC[off] = NUL;
#endif
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
}

/*
 * Set job options common for Unix and MS-Windows.
 */
    static void
setup_job_options(jobopt_T *opt, int rows, int cols)
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

    opt->jo_io_buf[PART_OUT] = curbuf->b_fnum;
    opt->jo_io_buf[PART_ERR] = curbuf->b_fnum;
    opt->jo_pty = TRUE;
    opt->jo_set |= JO_OUT_BUF + JO_ERR_BUF;

    opt->jo_term_rows = rows;
    opt->jo_term_cols = cols;
}

/*
 * Create a new vterm and initialize it.
 */
    static void
create_vterm(term_T *term, int rows, int cols)
{
    VTerm	    *vterm;
    VTermScreen	    *screen;

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

	if (term->tl_title != NULL)
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

# ifdef WIN3264

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

/**************************************
 * 2. MS-Windows implementation.
 */

#define WINPTY_DLL "winpty.dll"

static HINSTANCE hWinPtyDLL = NULL;

    int
dyn_winpty_init(void)
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
	{NULL, NULL}
    };

    /* No need to initialize twice. */
    if (hWinPtyDLL)
	return 1;
    /* Load winpty.dll */
    hWinPtyDLL = vimLoadLib(WINPTY_DLL);
    if (!hWinPtyDLL)
    {
	EMSG2(_(e_loadlib), WINPTY_DLL);
	return 0;
    }
    for (i = 0; winpty_entry[i].name != NULL
					 && winpty_entry[i].ptr != NULL; ++i)
    {
	if ((*winpty_entry[i].ptr = (FARPROC)GetProcAddress(hWinPtyDLL,
					      winpty_entry[i].name)) == NULL)
	{
	    EMSG2(_(e_loadfunc), winpty_entry[i].name);
	    return 0;
	}
    }

    return 1;
}

/*
 * Create a new terminal of "rows" by "cols" cells.
 * Store a reference in "term".
 * Return OK or FAIL.
 */
    static int
term_and_job_init(term_T *term, int rows, int cols, char_u *cmd)
{
    WCHAR	    *p;
    channel_T	    *channel = NULL;
    job_T	    *job = NULL;
    jobopt_T	    opt;
    DWORD	    error;
    HANDLE	    jo = NULL, child_process_handle, child_thread_handle;
    void	    *winpty_err;
    void	    *spawn_config = NULL;

    if (!dyn_winpty_init())
	return FAIL;

    p = enc_to_utf16(cmd, NULL);
    if (p == NULL)
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

    winpty_config_set_initial_size(term->tl_winpty_config, cols, rows);
    term->tl_winpty = winpty_open(term->tl_winpty_config, &winpty_err);
    if (term->tl_winpty == NULL)
	goto failed;

    spawn_config = winpty_spawn_config_new(
	    WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN |
		WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN,
	    NULL,
	    p,
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

    if (!winpty_spawn(term->tl_winpty, spawn_config, &child_process_handle,
	    &child_thread_handle, &error, &winpty_err))
	goto failed;

    channel_set_pipes(channel,
	(sock_T) CreateFileW(
	    winpty_conin_name(term->tl_winpty),
	    GENERIC_WRITE, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T) CreateFileW(
	    winpty_conout_name(term->tl_winpty),
	    GENERIC_READ, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T) CreateFileW(
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
    vim_free(p);

    create_vterm(term, rows, cols);

    setup_job_options(&opt, rows, cols);
    channel_set_job(channel, job, &opt);

    job->jv_channel = channel;
    job->jv_proc_info.hProcess = child_process_handle;
    job->jv_proc_info.dwProcessId = GetProcessId(child_process_handle);
    job->jv_job_object = jo;
    job->jv_status = JOB_STARTED;
    ++job->jv_refcount;
    term->tl_job = job;

    return OK;

failed:
    if (spawn_config != NULL)
	winpty_spawn_config_free(spawn_config);
    vim_free(p);
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

/*
 * Free the terminal emulator part of "term".
 */
    static void
term_free(term_T *term)
{
    if (term->tl_winpty != NULL)
	winpty_free(term->tl_winpty);
    if (term->tl_winpty_config != NULL)
	winpty_config_free(term->tl_winpty_config);
    if (term->tl_vterm != NULL)
	vterm_free(term->tl_vterm);
}

/*
 * Request size to terminal.
 */
    static void
term_report_winsize(term_T *term, int rows, int cols)
{
    winpty_set_size(term->tl_winpty, cols, rows, NULL);
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
term_and_job_init(term_T *term, int rows, int cols, char_u *cmd)
{
    typval_T	argvars[2];
    jobopt_T	opt;

    create_vterm(term, rows, cols);

    argvars[0].v_type = VAR_STRING;
    argvars[0].vval.v_string = cmd;
    setup_job_options(&opt, rows, cols);
    term->tl_job = job_start(argvars, &opt);
    if (term->tl_job != NULL)
	++term->tl_job->jv_refcount;

    return term->tl_job != NULL
	&& term->tl_job->jv_channel != NULL
	&& term->tl_job->jv_status != JOB_FAILED ? OK : FAIL;
}

/*
 * Free the terminal emulator part of "term".
 */
    static void
term_free(term_T *term)
{
    if (term->tl_vterm != NULL)
	vterm_free(term->tl_vterm);
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
	    mch_stop_job(term->tl_job, (char_u *)"winch");
    }
}

# endif

#endif /* FEAT_TERMINAL */
