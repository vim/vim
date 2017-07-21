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
 * 2. The MS-Windows implementation.
 *    Uses a hidden console for the terminal emulator.
 * 3. The Unix-like implementation.
 *    Uses libvterm for the terminal emulator.
 *
 * When a terminal window is opened, a job is started that will be connected to
 * the terminal emulator.
 *
 * If the terminal window has keyboard focus, typed keys are converted to the
 * terminal encoding and writting to the job over a channel.
 *
 * If the job produces output, it is written to the terminal emulator.  The
 * terminal emulator invokes callbacks when its screen content changes.  The
 * line range is stored in tl_dirty_row_start and tl_dirty_row_end.  Once in a
 * while, if the terminal window is visible, the screen contents is drawn.
 *
 * TODO:
 * - set buffer options to be scratch, hidden, nomodifiable, etc.
 * - set buffer name to command, add (1) to avoid duplicates.
 * - If [command] is not given the 'shell' option is used.
 * - Add a scrollback buffer (contains lines to scroll off the top).
 *   Can use the buf_T lines, store attributes somewhere else?
 * - When the job ends:
 *   - Write "-- JOB ENDED --" in the terminal.
 *   - Put the terminal contents in the scrollback buffer.
 *   - Free the terminal emulator.
 *   - Display the scrollback buffer (but with attributes).
 *     Make the buffer not modifiable, drop attributes when making changes.
 * - when closing window and job has not ended, make terminal hidden?
 * - Use a pty for I/O with the job.
 * - Windows implementation:
 *   (WiP): https://github.com/mattn/vim/tree/terminal
 *	src/os_win32.c  mch_open_terminal()
 *   Using winpty ?
 * - use win_del_lines() to make scroll-up efficient.
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

#ifdef WIN3264
# define MIN(x,y) (x < y ? x : y)
# define MAX(x,y) (x > y ? x : y)
#endif

#include "libvterm/include/vterm.h"

/* typedef term_T in structs.h */
struct terminal_S {
    term_T	*tl_next;

#ifdef WIN3264
    /* console handle? */
    void	*wp_config;
    void	*wp_pty;
#endif
    VTerm	*tl_vterm;
    job_T	*tl_job;
    buf_T	*tl_buffer;

    /* Range of screen rows to update.  Zero based. */
    int		tl_dirty_row_start; /* -1 if nothing dirty */
    int		tl_dirty_row_end;   /* row below last one to update */

    pos_T	tl_cursor;
};

#define MAX_ROW 999999	    /* used for tl_dirty_row_end to update all rows */
#define KEY_BUF_LEN 200

/* Functions implemented for MS-Windows and Unix-like systems. */
static int term_init(term_T *term, int rows, int cols, char_u *cmd);
static void term_free(term_T *term);
static void term_write_job_output(term_T *term, char_u *msg, size_t len);
static int term_convert_key(int c, char *buf);
static void term_update_lines(win_T *wp);

/*
 * List of all active terminals.
 */
static term_T *first_term = NULL;

/**************************************
 * 1. Generic code for all systems.
 */

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
    term_T	*term;

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

    term_init(term, rows, cols, eap->arg);

    if (term->tl_job == NULL)
    {
	first_term = term->tl_next;
	curbuf->b_term = NULL;
	/* Wiping out the buffer will also close the window. */
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
	if (term->tl_job->jv_status != JOB_ENDED)
	    job_stop(term->tl_job, NULL, "kill");
	job_unref(term->tl_job);
    }

    term_free(term);
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
    term_T	*term = buffer->b_term;

    ch_logn(channel, "writing %d bytes to terminal", (int)len);
    term_write_job_output(term, msg, len);

    /* TODO: only update once in a while. */
    update_screen(0);
    setcursor();
    out_flush();
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

    for (;;)
    {
	/* TODO: skip screen update when handling a sequence of keys. */
	update_screen(0);
	setcursor();
	out_flush();
	++no_mapping;
	++allow_keys;
	got_int = FALSE;
	c = vgetc();
	--no_mapping;
	--allow_keys;

	/* Catch keys that need to be handled as in Normal mode. */
	switch (c)
	{
	    case Ctrl_W:
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
						     (char_u *)buf, len, NULL);
    }
}

/*
 * Called to update the window that contains the terminal.
 */
    void
term_update_window(win_T *wp)
{
    term_update_lines(wp);
}

    static void
position_cursor(win_T *wp, VTermPos *pos)
{
    wp->w_wrow = MIN(pos->row, MAX(0, wp->w_height - 1));
    wp->w_wcol = MIN(pos->col, MAX(0, wp->w_width - 1));
}

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
 * Write job output "msg[len]" to the terminal.
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
	    p += utf_ptr2len_len(p, len - (p - msg));
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
    vterm_screen_flush_damage(vterm_obtain_screen(vterm));
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
	VTermPos oldpos,
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
    win_T	*wp;

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
 * Called to update the window that contains the terminal.
 */
    static void
term_update_lines(win_T *wp)
{
    int		vterm_rows;
    int		vterm_cols;
    VTerm	*vterm = wp->w_buffer->b_term->tl_vterm;
    VTermScreen *screen = vterm_obtain_screen(vterm);
    VTermPos	pos;

    vterm_get_size(vterm, &vterm_rows, &vterm_cols);

    /* TODO: Only redraw what changed. */
    for (pos.row = 0; pos.row < wp->w_height; ++pos.row)
    {
	int off = screen_get_current_line_off();

	for (pos.col = 0; pos.col < wp->w_width && pos.col < vterm_cols; ++pos.col)
	{
	    ScreenLines[off + pos.col] = ' ';
	    ScreenLinesUC[off + pos.col] = 0x00;
	}

	if (pos.row < vterm_rows)
	{
	    for (pos.col = 0; pos.col < wp->w_width && pos.col < vterm_cols;)
	    {
		VTermScreenCell cell;
		int c;

		vterm_screen_get_cell(screen, pos, &cell);
		/* TODO: use cell.attrs and colors */
		c = cell.chars[0];
		if (c != NUL)
		{
#if defined(FEAT_MBYTE)
		    if (enc_utf8 && c >= 0x80)
			ScreenLinesUC[off] = c;
		    else
			ScreenLines[off] = c;
#else
		    ScreenLines[off] = c;
#endif
		    ScreenAttrs[off] = 0;
		}

		pos.col += cell.width;
		off += cell.width;
	    }
	}

	screen_line(wp->w_winrow + pos.row, wp->w_wincol,
		MIN(wp->w_width, vterm_cols), wp->w_width, FALSE);
    }
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
	case K_BS:		key = VTERM_KEY_BACKSPACE; break;
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
    return vterm_output_read(vterm, buf, KEY_BUF_LEN);
}

    static void
setup_job_options(jobopt_T *opt)
{
    clear_job_options(opt);
    opt->jo_mode = MODE_RAW;
    opt->jo_out_mode = MODE_RAW;
    opt->jo_err_mode = MODE_RAW;
    opt->jo_set = JO_MODE | JO_OUT_MODE | JO_ERR_MODE;
    opt->jo_io[PART_OUT] = JIO_BUFFER;
    opt->jo_io[PART_ERR] = JIO_BUFFER;
    opt->jo_set |= JO_OUT_IO + (JO_OUT_IO << (PART_ERR - PART_OUT));
    opt->jo_io_buf[PART_OUT] = curbuf->b_fnum;
    opt->jo_io_buf[PART_ERR] = curbuf->b_fnum;
    opt->jo_set |= JO_OUT_BUF + (JO_OUT_BUF << (PART_ERR - PART_OUT));
}

#ifdef WIN3264

#define WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN 1ul
#define WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN 2ull

void* (*winpty_config_new)(int, void*);
void* (*winpty_open)(void*, void*);
void* (*winpty_spawn_config_new)(int, void*, LPCWSTR, void*, void*, void*);
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
term_init(term_T *term, int rows, int cols, char_u *cmd)
{
    WCHAR	    *p = enc_to_utf16(cmd, NULL);
    channel_T	    *channel = NULL;
    job_T	    *job = NULL;
    jobopt_T	    opt;
    DWORD	    error;
    HANDLE	    jo = NULL, child_process_handle, child_thread_handle;
    void	    *err;
    void	    *spawn_config;

    VTerm	    *vterm;
    VTermScreen	    *screen;

    if (!dyn_winpty_init())
	return FAIL;

    if (p == NULL)
	return FAIL;

    job = job_alloc();
    if (job == NULL)
	goto failed;

    channel = add_channel();
    if (channel == NULL)
	goto failed;

    term->wp_config = winpty_config_new(0, &err);
    if (term->wp_config == NULL)
	goto failed;

    winpty_config_set_initial_size(term->wp_config, cols, rows);
    term->wp_pty = winpty_open(term->wp_config, &err);
    if (term->wp_pty == NULL)
	goto failed;

    spawn_config = winpty_spawn_config_new(
	    WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN |
		WINPTY_SPAWN_FLAG_EXIT_AFTER_SHUTDOWN,
	    NULL,
	    p,
	    NULL,
	    NULL,
	    &err);
    if (spawn_config == NULL)
	goto failed;

    channel = add_channel();
    if (channel == NULL)
	goto failed;

    job = job_alloc();
    if (job == NULL)
	goto failed;

    if (!winpty_spawn(term->wp_pty, spawn_config, &child_process_handle,
	    &child_thread_handle, &error, &err))
	goto failed;

    channel_set_pipes(channel,
	(sock_T) CreateFileW(
	    winpty_conin_name(term->wp_pty),
	    GENERIC_WRITE, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T) CreateFileW(
	    winpty_conout_name(term->wp_pty),
	    GENERIC_READ, 0, NULL,
	    OPEN_EXISTING, 0, NULL),
	(sock_T) CreateFileW(
	    winpty_conerr_name(term->wp_pty),
	    GENERIC_READ, 0, NULL,
	    OPEN_EXISTING, 0, NULL));

    jo = CreateJobObject(NULL, NULL);
    if (jo == NULL)
	goto failed;

    if (!AssignProcessToJobObject(jo, child_process_handle))
	goto failed;

    winpty_spawn_config_free(spawn_config);

    vterm = vterm_new(rows, cols);

    term->tl_vterm = vterm;
    screen = vterm_obtain_screen(vterm);
    vterm_screen_set_callbacks(screen, &screen_callbacks, term);
    /* TODO: depends on 'encoding'. */
    vterm_set_utf8(vterm, 1);
    /* Required to initialize most things. */
    vterm_screen_reset(screen, 1 /* hard */);

    setup_job_options(&opt);
    channel_set_job(channel, job, &opt);

    job->jv_channel = channel;
    job->jv_proc_info.hProcess = child_process_handle;
    job->jv_proc_info.dwProcessId = GetProcessId(child_process_handle);
    job->jv_job_object = jo;
    job->jv_status = JOB_STARTED;
    term->tl_job = job;

    return OK;

failed:
    if (channel)
	channel_clear(channel);
    if (job)
	job_cleanup(job);
    if (jo)
	CloseHandle(jo);
    if (term->wp_pty)
	winpty_free(term->wp_pty);
    if (term->wp_config)
	winpty_config_free(term->wp_config);
    if (err)
    {
	char_u* msg = utf16_to_enc((short_u*) winpty_error_msg(err), NULL);
	EMSG(msg);
	winpty_error_free(err);
    }
    return FAIL;
}

/*
 * Free the terminal emulator part of "term".
 */
    static void
term_free(term_T *term)
{
    winpty_free(term->wp_pty);
    winpty_config_free(term->wp_config);
    vterm_free(term->tl_vterm);
}

#else

/**************************************
 * 3. Unix-like implementation.
 *
 * For a terminal one VTerm is constructed.  This uses libvterm.  A copy of
 * that library is in the libvterm directory.
 */
/*
 * Create a new terminal of "rows" by "cols" cells.
 * Store a reference in "term".
 * Return OK or FAIL.
 */
    static int
term_init(term_T *term, int rows, int cols, char_u *cmd)
{
    VTerm *vterm = vterm_new(rows, cols);
    VTermScreen *screen;
    typval_T	argvars[2];
    jobopt_T	opt;

    term->tl_vterm = vterm;
    screen = vterm_obtain_screen(vterm);
    vterm_screen_set_callbacks(screen, &screen_callbacks, term);
    /* TODO: depends on 'encoding'. */
    vterm_set_utf8(vterm, 1);
    /* Required to initialize most things. */
    vterm_screen_reset(screen, 1 /* hard */);

    argvars[0].v_type = VAR_STRING;
    argvars[0].vval.v_string = cmd;

    setup_job_options(&opt);
    term->tl_job = job_start(argvars, &opt);

    return term->tl_job != NULL ? OK : FAIL;
}

/*
 * Free the terminal emulator part of "term".
 */
    static void
term_free(term_T *term)
{
    vterm_free(term->tl_vterm);
}

#endif

#endif /* FEAT_TERMINAL */
