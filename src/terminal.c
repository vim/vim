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
 * while, if the window is visible, the screen contents is drawn.
 *
 * If the terminal window has keyboard focus, typed keys are converted to the
 * terminal encoding and writting to the job over a channel.
 *
 * If the job produces output, it is written to the VTerm.
 * This will result in screen updates.
 *
 * TODO:
 * - free b_term when closing terminal.
 * - remove term from first_term list when closing terminal.
 * - set buffer options to be scratch, hidden, nomodifiable, etc.
 * - set buffer name to command, add (1) to avoid duplicates.
 * - if buffer is wiped, cleanup terminal, may stop job.
 * - if the job ends, write "-- JOB ENDED --" in the terminal
 * - command line completion (command name)
 * - support fixed size when 'termsize' is "rowsXcols".
 * - support minimal size when 'termsize' is "rows*cols".
 * - support minimal size when 'termsize' is empty.
 * - implement ":buf {term-buf-name}"
 * - implement term_getsize()
 * - implement term_setsize()
 * - implement term_sendkeys()		send keystrokes to a terminal
 * - implement term_wait()		wait for screen to be updated
 * - implement term_scrape()		inspect terminal screen
 * - implement term_open()		open terminal window
 * - implement term_getjob()
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

    /* TODO: setup channel to job */
    /* Setup pty, see mch_call_shell(). */
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

/* TODO: function to update the window.
 * Get the screen contents from vterm with vterm_screen_get_cell().
 * put in current_ScreenLine and call screen_line().
 */

/* TODO: function to wait for input and send it to the job.
 * Return when a CTRL-W command is typed that moves to another window.
 * Convert special keys to vterm keys:
 * - Write keys to vterm: vterm_keyboard_key()
 * - read the output (xterm escape sequences): vterm_output_read()
 * - Write output to channel.
 */

#endif /* FEAT_TERMINAL */
