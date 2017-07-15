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
 */

#include "vim.h"

#ifdef FEAT_TERMINAL

#include "libvterm/include/vterm.h"

/* typedef term_T in structs.h */
struct terminal_S {
    term_T	*tl_next;

    VTerm	*tl_vterm;
    job_T	*tl_job;

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

    argvars[0].v_type = VAR_STRING;
    argvars[0].vval.v_string = eap->arg;
    argvars[1].v_type = VAR_UNKNOWN;
    term->tl_job = job_start(argvars);

    /* TODO: setup channels to/from job */
    /* Setup pty, see mch_call_shell(). */
}

    static int
handle_damage(VTermRect rect, void *user)
{
    term_T *term = (term_T *)user;

    term->tl_dirty_row_start = MIN(term->tl_dirty_row_start, rect.start_row);
    term->tl_dirty_row_end = MAX(term->tl_dirty_row_end, rect.end_row);
    return 1;
}

    static int
handle_moverect(VTermRect dest, VTermRect src, void *user)
{
    /* TODO */
    return 1;
}

  static int
handle_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    /* TODO: handle moving the cursor. */
    return 1;
}

    static int
handle_resize(int rows, int cols, void *user)
{
    /* TODO: handle terminal resize. */
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

/* TODO: function to read job output from the channel.
 * write to vterm: vterm_input_write()
 * This will invoke screen callbacks.
 * call vterm_screen_flush_damage()
 */

#endif /* FEAT_TERMINAL */
