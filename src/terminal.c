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
 * For a terminal one VTerm is constructed.  This uses libvterm.  A copy of
 * that library is in the libvterm directory.
 *
 * The VTerm invokes callbacks when its screen contents changes.  The line
 * range is stored in tl_dirty_row_start and tl_dirty_row_end.  Once in a
 * while, if the terminal window is visible, the screen contents is drawn.
 *
 * If the terminal window has keyboard focus, typed keys are converted to the
 * terminal encoding and writting to the job over a channel.
 *
 * If the job produces output, it is written to the VTerm.
 * This will result in screen updates.
 *
 * TODO:
 * - pressing Enter sends two CR and/or NL characters to "bash -i"?
 *   Passing Enter as NL seems to work.
 * - set buffer options to be scratch, hidden, nomodifiable, etc.
 * - set buffer name to command, add (1) to avoid duplicates.
 * - If [command] is not given the 'shell' option is used.
 * - if the job ends, write "-- JOB ENDED --" in the terminal
 * - when closing window and job ended, delete the terminal
 * - when closing window and job has not ended, make terminal hidden?
 * - Use a pty for I/O with the job.
 * - Windows implementation:
 *   (WiP): https://github.com/mattn/vim/tree/terminal
 *	src/os_win32.c  mch_open_terminal()
     Using winpty ?
 * - command line completion for :terminal
 * - support fixed size when 'termsize' is "rowsXcols".
 * - support minimal size when 'termsize' is "rows*cols".
 * - support minimal size when 'termsize' is empty.
 * - implement ":buf {term-buf-name}"
 * - implement term_list()			list of buffers with a terminal
 * - implement term_getsize(buf)
 * - implement term_setsize(buf)
 * - implement term_sendkeys(buf, keys)		send keystrokes to a terminal
 * - implement term_wait(buf)			wait for screen to be updated
 * - implement term_scrape(buf, row)		inspect terminal screen
 * - implement term_open(command, options)	open terminal window
 * - implement term_getjob(buf)
 * - implement 'termkey'
 */

#include "vim.h"

#ifdef FEAT_TERMINAL

#include "libvterm/include/vterm.h"

/* typedef term_T in structs.h */
struct terminal_S {
    term_T	*tl_next;

    VTerm	*tl_vterm;
    job_T	*tl_job;
    buf_T	*tl_buffer;

    /* Range of screen rows to update.  Zero based. */
    int		tl_dirty_row_start; /* -1 if nothing dirty */
    int		tl_dirty_row_end;   /* row below last one to update */

    pos_T	tl_cursor;
};

#define MAX_ROW 999999	    /* used for tl_dirty_row_end to update all rows */

/*
 * List of all active terminals.
 */
static term_T *first_term = NULL;

static int handle_damage(VTermRect rect, void *user);
static int handle_moverect(VTermRect dest, VTermRect src, void *user);
static int handle_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
static int handle_resize(int rows, int cols, void *user);

static VTermScreenCallbacks screen_callbacks = {
  handle_damage,	/* damage */
  handle_moverect,	/* moverect */
  handle_movecursor,	/* movecursor */
  NULL,			/* settermprop */
  NULL,			/* bell */
  handle_resize,	/* resize */
  NULL,			/* sb_pushline */
  NULL			/* sb_popline */
};

/*
 * ":terminal": open a terminal window and execute a job in it.
 */
    void
ex_terminal(exarg_T *eap)
{
    int		rows;
    int		cols;
    exarg_T	split_ea;
    win_T	*old_curwin = curwin;
    typval_T	argvars[2];
    term_T	*term;
    VTerm	*vterm;
    VTermScreen *screen;
    jobopt_T	opt;

    if (check_restricted() || check_secure())
	return;

    term = (term_T *)alloc_clear(sizeof(term_T));
    if (term == NULL)
	return;
    term->tl_dirty_row_end = MAX_ROW;

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
    term->tl_next = first_term;
    first_term = term;

    /* TODO: set buffer type, hidden, etc. */

    if (*curwin->w_p_tms != NUL)
    {
	char_u *p = vim_strchr(curwin->w_p_tms, 'x') + 1;

	rows = atoi((char *)curwin->w_p_tms);
	cols = atoi((char *)p);
	/* TODO: resize window if possible. */
    }
    else
    {
	rows = curwin->w_height;
	cols = curwin->w_width;
    }

    vterm = vterm_new(rows, cols);
    term->tl_vterm = vterm;
    screen = vterm_obtain_screen(vterm);
    vterm_screen_set_callbacks(screen, &screen_callbacks, term);
    /* TODO: depends on 'encoding'. */
    vterm_set_utf8(vterm, 1);
    /* Required to initialize most things. */
    vterm_screen_reset(screen, 1 /* hard */);

    /* By default NL means CR-NL. */
    vterm_input_write(vterm, "\x1b[20h", 5);

    argvars[0].v_type = VAR_STRING;
    argvars[0].vval.v_string = eap->arg;

    clear_job_options(&opt);
    opt.jo_mode = MODE_RAW;
    opt.jo_out_mode = MODE_RAW;
    opt.jo_err_mode = MODE_RAW;
    opt.jo_set = JO_MODE | JO_OUT_MODE | JO_ERR_MODE;
    opt.jo_io[PART_OUT] = JIO_BUFFER;
    opt.jo_io[PART_ERR] = JIO_BUFFER;
    opt.jo_set |= JO_OUT_IO + (JO_OUT_IO << (PART_ERR - PART_OUT));
    opt.jo_io_buf[PART_OUT] = curbuf->b_fnum;
    opt.jo_io_buf[PART_ERR] = curbuf->b_fnum;
    opt.jo_set |= JO_OUT_BUF + (JO_OUT_BUF << (PART_ERR - PART_OUT));

    term->tl_job = job_start(argvars, &opt);

    if (term->tl_job == NULL)
	/* Wiping out the buffer will also close the window. */
	do_buffer(DOBUF_WIPE, DOBUF_CURRENT, FORWARD, 0, TRUE);

    /* Setup pty, see mch_call_shell(). */
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
	if (term->tl_job->jv_status != JOB_ENDED)
	    job_stop(term->tl_job, NULL, "kill");
	job_unref(term->tl_job);
    }

    vterm_free(term->tl_vterm);
    vim_free(term);
}

/*
 * Invoked when "msg" output from a job was received.  Write it to the terminal
 * of "buffer".
 */
    void
write_to_term(buf_T *buffer, char_u *msg, channel_T *channel)
{
    size_t	len = STRLEN(msg);
    VTerm	*vterm = buffer->b_term->tl_vterm;

    ch_logn(channel, "writing %d bytes to terminal", (int)len);
    vterm_input_write(vterm, (char *)msg, len);
    vterm_screen_flush_damage(vterm_obtain_screen(vterm));

    /* TODO: only update once in a while. */
    update_screen(0);
    setcursor();
    out_flush();
}

/*
 * Called to update the window that contains the terminal.
 */
    void
term_update_window(win_T *wp)
{
    int vterm_rows;
    int vterm_cols;
    VTerm *vterm = wp->w_buffer->b_term->tl_vterm;
    VTermScreen *screen = vterm_obtain_screen(vterm);
    VTermPos pos;

    vterm_get_size(vterm, &vterm_rows, &vterm_cols);

    /* TODO: Only redraw what changed. */
    for (pos.row = 0; pos.row < wp->w_height; ++pos.row)
    {
	int off = screen_get_current_line_off();

	if (pos.row < vterm_rows)
	    for (pos.col = 0; pos.col < wp->w_width && pos.col < vterm_cols;
								     ++pos.col)
	    {
		VTermScreenCell cell;
		int c;

		vterm_screen_get_cell(screen, pos, &cell);
		/* TODO: use cell.attrs and colors */
		/* TODO: use cell.width */
		/* TODO: multi-byte chars */
		c = cell.chars[0];
		ScreenLines[off] = c == NUL ? ' ' : c;
		ScreenAttrs[off] = 0;
		++off;
	    }

	screen_line(wp->w_winrow + pos.row, wp->w_wincol, pos.col, wp->w_width,
									FALSE);
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
handle_moverect(VTermRect dest, VTermRect src, void *user)
{
    term_T	*term = (term_T *)user;

    /* TODO */
    redraw_buf_later(term->tl_buffer, NOT_VALID);
    return 1;
}

  static int
handle_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    term_T	*term = (term_T *)user;
    win_T	*wp;
    int		is_current = FALSE;

    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer == term->tl_buffer)
	{
	    /* TODO: limit to window size? */
	    wp->w_wrow = pos.row;
	    wp->w_wcol = pos.col;
	    if (wp == curwin)
		is_current = TRUE;
	}
    }

    if (is_current)
    {
	setcursor();
	out_flush();
    }

    return 1;
}

    static int
handle_resize(int rows, int cols, void *user)
{
    term_T	*term = (term_T *)user;

    /* TODO: handle terminal resize. */
    redraw_buf_later(term->tl_buffer, NOT_VALID);
    return 1;
}

/* TODO: Use win_del_lines() to make scroll up efficient. */


/*
 * Wait for input and send it to the job.
 * Return when a CTRL-W command is typed that moves to another window.
 */
    void
terminal_loop(void)
{
    VTerm   *vterm = curbuf->b_term->tl_vterm;
    char    buf[200];

    for (;;)
    {
	int c;
	VTermKey key = VTERM_KEY_NONE;
	VTermModifier mod = VTERM_MOD_NONE;
	size_t len;

	update_screen(0);
	setcursor();
	out_flush();

	c = vgetc();
	switch (c)
	{
	    case Ctrl_W:
		stuffcharReadbuff(Ctrl_W);
		return;

	    /* TODO: which of these two should be used? */
#if 0
	    case CAR:		key = VTERM_KEY_ENTER; break;
#else
	    case CAR:		c = NL; break;
#endif
	    case ESC:		key = VTERM_KEY_ESCAPE; break;
	    case K_BS:		key = VTERM_KEY_BACKSPACE; break;
	    case K_DEL:		key = VTERM_KEY_DEL; break;
	    case K_DOWN:	key = VTERM_KEY_DOWN; break;
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
	    case K_HOME:	key = VTERM_KEY_HOME; break;
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
	    case K_KDEL:	key = VTERM_KEY_DEL; break; /* TODO */
	    case K_KDIVIDE:	key = VTERM_KEY_KP_DIVIDE; break;
	    case K_KEND:	key = VTERM_KEY_KP_1; break; /* TODO */
	    case K_KENTER:	key = VTERM_KEY_KP_ENTER; break;
	    case K_KHOME:	key = VTERM_KEY_KP_7; break; /* TODO */
	    case K_KINS:	key = VTERM_KEY_KP_0; break; /* TODO */
	    case K_KMINUS:	key = VTERM_KEY_KP_MINUS; break;
	    case K_KMULTIPLY:	key = VTERM_KEY_KP_MULT; break;
	    case K_KPAGEDOWN:	key = VTERM_KEY_KP_3; break; /* TODO */
	    case K_KPAGEUP:	key = VTERM_KEY_KP_9; break; /* TODO */
	    case K_KPLUS:	key = VTERM_KEY_KP_PLUS; break;
	    case K_KPOINT:	key = VTERM_KEY_KP_PERIOD; break;
	    case K_LEFT:	key = VTERM_KEY_LEFT; break;
	    case K_PAGEDOWN:	key = VTERM_KEY_PAGEDOWN; break;
	    case K_PAGEUP:	key = VTERM_KEY_PAGEUP; break;
	    case K_RIGHT:	key = VTERM_KEY_RIGHT; break;
	    case K_UP:		key = VTERM_KEY_UP; break;
	    case TAB:		key = VTERM_KEY_TAB; break;
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
	len = vterm_output_read(vterm, buf, sizeof(buf));

	/* TODO: if FAIL is returned, stop? */
	channel_send(curbuf->b_term->tl_job->jv_channel, PART_IN,
						     (char_u *)buf, len, NULL);
    }
}

#endif /* FEAT_TERMINAL */
