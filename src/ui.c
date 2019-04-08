/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * ui.c: functions that handle the user interface.
 * 1. Keyboard input stuff, and a bit of windowing stuff.  These are called
 *    before the machine specific stuff (mch_*) so that we can call the GUI
 *    stuff instead if the GUI is running.
 * 2. Clipboard stuff.
 * 3. Input buffer stuff.
 */

#include "vim.h"

#ifdef FEAT_CYGWIN_WIN32_CLIPBOARD
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include "winclip.pro"
#endif

    void
ui_write(char_u *s, int len)
{
#ifdef FEAT_GUI
    if (gui.in_use && !gui.dying && !gui.starting)
    {
	gui_write(s, len);
	if (p_wd)
	    gui_wait_for_chars(p_wd, typebuf.tb_change_cnt);
	return;
    }
#endif
#ifndef NO_CONSOLE
    /* Don't output anything in silent mode ("ex -s") unless 'verbose' set */
    if (!(silent_mode && p_verbose == 0))
    {
#if !defined(MSWIN)
	char_u	*tofree = NULL;

	if (output_conv.vc_type != CONV_NONE)
	{
	    /* Convert characters from 'encoding' to 'termencoding'. */
	    tofree = string_convert(&output_conv, s, &len);
	    if (tofree != NULL)
		s = tofree;
	}
#endif

	mch_write(s, len);

# if !defined(MSWIN)
	if (output_conv.vc_type != CONV_NONE)
	    vim_free(tofree);
# endif
    }
#endif
}

#if defined(UNIX) || defined(VMS) || defined(PROTO) || defined(MSWIN)
/*
 * When executing an external program, there may be some typed characters that
 * are not consumed by it.  Give them back to ui_inchar() and they are stored
 * here for the next call.
 */
static char_u *ta_str = NULL;
static int ta_off;	/* offset for next char to use when ta_str != NULL */
static int ta_len;	/* length of ta_str when it's not NULL*/

    void
ui_inchar_undo(char_u *s, int len)
{
    char_u  *new;
    int	    newlen;

    newlen = len;
    if (ta_str != NULL)
	newlen += ta_len - ta_off;
    new = alloc(newlen);
    if (new != NULL)
    {
	if (ta_str != NULL)
	{
	    mch_memmove(new, ta_str + ta_off, (size_t)(ta_len - ta_off));
	    mch_memmove(new + ta_len - ta_off, s, (size_t)len);
	    vim_free(ta_str);
	}
	else
	    mch_memmove(new, s, (size_t)len);
	ta_str = new;
	ta_len = newlen;
	ta_off = 0;
    }
}
#endif

/*
 * ui_inchar(): low level input function.
 * Get characters from the keyboard.
 * Return the number of characters that are available.
 * If "wtime" == 0 do not wait for characters.
 * If "wtime" == -1 wait forever for characters.
 * If "wtime" > 0 wait "wtime" milliseconds for a character.
 *
 * "tb_change_cnt" is the value of typebuf.tb_change_cnt if "buf" points into
 * it.  When typebuf.tb_change_cnt changes (e.g., when a message is received
 * from a remote client) "buf" can no longer be used.  "tb_change_cnt" is NULL
 * otherwise.
 */
    int
ui_inchar(
    char_u	*buf,
    int		maxlen,
    long	wtime,	    /* don't use "time", MIPS cannot handle it */
    int		tb_change_cnt)
{
    int		retval = 0;

#if defined(FEAT_GUI) && (defined(UNIX) || defined(VMS))
    /*
     * Use the typeahead if there is any.
     */
    if (ta_str != NULL)
    {
	if (maxlen >= ta_len - ta_off)
	{
	    mch_memmove(buf, ta_str + ta_off, (size_t)ta_len);
	    VIM_CLEAR(ta_str);
	    return ta_len;
	}
	mch_memmove(buf, ta_str + ta_off, (size_t)maxlen);
	ta_off += maxlen;
	return maxlen;
    }
#endif

#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES && wtime != 0)
	prof_inchar_enter();
#endif

#ifdef NO_CONSOLE_INPUT
    /* Don't wait for character input when the window hasn't been opened yet.
     * Do try reading, this works when redirecting stdin from a file.
     * Must return something, otherwise we'll loop forever.  If we run into
     * this very often we probably got stuck, exit Vim. */
    if (no_console_input())
    {
	static int count = 0;

# ifndef NO_CONSOLE
	retval = mch_inchar(buf, maxlen, wtime, tb_change_cnt);
	if (retval > 0 || typebuf_changed(tb_change_cnt) || wtime >= 0)
	    goto theend;
# endif
	if (wtime == -1 && ++count == 1000)
	    read_error_exit();
	buf[0] = CAR;
	retval = 1;
	goto theend;
    }
#endif

    /* If we are going to wait for some time or block... */
    if (wtime == -1 || wtime > 100L)
    {
	/* ... allow signals to kill us. */
	(void)vim_handle_signal(SIGNAL_UNBLOCK);

	/* ... there is no need for CTRL-C to interrupt something, don't let
	 * it set got_int when it was mapped. */
	if ((mapped_ctrl_c | curbuf->b_mapped_ctrl_c) & get_real_state())
	    ctrl_c_interrupts = FALSE;
    }

    /*
     * Here we call gui_inchar() or mch_inchar(), the GUI or machine-dependent
     * input function.  The functionality they implement is like this:
     *
     * while (not timed out)
     * {
     *    handle-resize;
     *    parse-queued-messages;
     *    if (waited for 'updatetime')
     *       trigger-cursorhold;
     *    ui_wait_for_chars_or_timer()
     *    if (character available)
     *      break;
     * }
     *
     * ui_wait_for_chars_or_timer() does:
     *
     * while (not timed out)
     * {
     *     if (any-timer-triggered)
     *        invoke-timer-callback;
     *     wait-for-character();
     *     if (character available)
     *        break;
     * }
     *
     * wait-for-character() does:
     * while (not timed out)
     * {
     *     Wait for event;
     *     if (something on channel)
     *        read/write channel;
     *     else if (resized)
     *        handle_resize();
     *     else if (system event)
     *        deal-with-system-event;
     *     else if (character available)
     *        break;
     * }
     *
     */

#ifdef FEAT_GUI
    if (gui.in_use)
	retval = gui_inchar(buf, maxlen, wtime, tb_change_cnt);
#endif
#ifndef NO_CONSOLE
# ifdef FEAT_GUI
    else
# endif
	retval = mch_inchar(buf, maxlen, wtime, tb_change_cnt);
#endif

    if (wtime == -1 || wtime > 100L)
	/* block SIGHUP et al. */
	(void)vim_handle_signal(SIGNAL_BLOCK);

    ctrl_c_interrupts = TRUE;

#ifdef NO_CONSOLE_INPUT
theend:
#endif
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES && wtime != 0)
	prof_inchar_exit();
#endif
    return retval;
}

#if defined(UNIX) || defined(FEAT_GUI) || defined(PROTO)
/*
 * Common code for mch_inchar() and gui_inchar(): Wait for a while or
 * indefinitely until characters are available, dealing with timers and
 * messages on channels.
 *
 * "buf" may be NULL if the available characters are not to be returned, only
 * check if they are available.
 *
 * Return the number of characters that are available.
 * If "wtime" == 0 do not wait for characters.
 * If "wtime" == n wait a short time for characters.
 * If "wtime" == -1 wait forever for characters.
 */
    int
inchar_loop(
    char_u	*buf,
    int		maxlen,
    long	wtime,	    // don't use "time", MIPS cannot handle it
    int		tb_change_cnt,
    int		(*wait_func)(long wtime, int *interrupted, int ignore_input),
    int		(*resize_func)(int check_only))
{
    int		len;
    int		interrupted = FALSE;
    int		did_call_wait_func = FALSE;
    int		did_start_blocking = FALSE;
    long	wait_time;
    long	elapsed_time = 0;
#ifdef ELAPSED_FUNC
    elapsed_T	start_tv;

    ELAPSED_INIT(start_tv);
#endif

    /* repeat until we got a character or waited long enough */
    for (;;)
    {
	/* Check if window changed size while we were busy, perhaps the ":set
	 * columns=99" command was used. */
	if (resize_func != NULL)
	    resize_func(FALSE);

#ifdef MESSAGE_QUEUE
	// Only process messages when waiting.
	if (wtime != 0)
	{
	    parse_queued_messages();
	    // If input was put directly in typeahead buffer bail out here.
	    if (typebuf_changed(tb_change_cnt))
		return 0;
	}
#endif
	if (wtime < 0 && did_start_blocking)
	    // blocking and already waited for p_ut
	    wait_time = -1;
	else
	{
	    if (wtime >= 0)
		wait_time = wtime;
	    else
		// going to block after p_ut
		wait_time = p_ut;
#ifdef ELAPSED_FUNC
	    elapsed_time = ELAPSED_FUNC(start_tv);
#endif
	    wait_time -= elapsed_time;

	    // If the waiting time is now zero or less, we timed out.  However,
	    // loop at least once to check for characters and events.  Matters
	    // when "wtime" is zero.
	    if (wait_time <= 0 && did_call_wait_func)
	    {
		if (wtime >= 0)
		    // no character available within "wtime"
		    return 0;

		// No character available within 'updatetime'.
		did_start_blocking = TRUE;
		if (trigger_cursorhold() && maxlen >= 3
					    && !typebuf_changed(tb_change_cnt))
		{
		    // Put K_CURSORHOLD in the input buffer or return it.
		    if (buf == NULL)
		    {
			char_u	ibuf[3];

			ibuf[0] = CSI;
			ibuf[1] = KS_EXTRA;
			ibuf[2] = (int)KE_CURSORHOLD;
			add_to_input_buf(ibuf, 3);
		    }
		    else
		    {
			buf[0] = K_SPECIAL;
			buf[1] = KS_EXTRA;
			buf[2] = (int)KE_CURSORHOLD;
		    }
		    return 3;
		}

		// There is no character available within 'updatetime' seconds:
		// flush all the swap files to disk.  Also done when
		// interrupted by SIGWINCH.
		before_blocking();
		continue;
	    }
	}

#ifdef FEAT_JOB_CHANNEL
	if (wait_time < 0 || wait_time > 100L)
	{
	    // Checking if a job ended requires polling.  Do this at least
	    // every 100 msec.
	    if (has_pending_job())
		wait_time = 100L;

	    // If there is readahead then parse_queued_messages() timed out and
	    // we should call it again soon.
	    if (channel_any_readahead())
		wait_time = 10L;
	}
#endif
#ifdef FEAT_BEVAL_GUI
	if (p_beval && wait_time > 100L)
	    // The 'balloonexpr' may indirectly invoke a callback while waiting
	    // for a character, need to check often.
	    wait_time = 100L;
#endif

	// Wait for a character to be typed or another event, such as the winch
	// signal or an event on the monitored file descriptors.
	did_call_wait_func = TRUE;
	if (wait_func(wait_time, &interrupted, FALSE))
	{
	    // If input was put directly in typeahead buffer bail out here.
	    if (typebuf_changed(tb_change_cnt))
		return 0;

	    // We might have something to return now.
	    if (buf == NULL)
		// "buf" is NULL, we were just waiting, not actually getting
		// input.
		return input_available();

	    len = read_from_input_buf(buf, (long)maxlen);
	    if (len > 0)
		return len;
	    continue;
	}
	// Timed out or interrupted with no character available.

#ifndef ELAPSED_FUNC
	// estimate the elapsed time
	elapsed_time += wait_time;
#endif

	if ((resize_func != NULL && resize_func(TRUE))
#if defined(FEAT_CLIENTSERVER) && defined(UNIX)
		|| server_waiting()
#endif
#ifdef MESSAGE_QUEUE
		|| interrupted
#endif
		|| wait_time > 0
		|| (wtime < 0 && !did_start_blocking))
	    // no character available, but something to be done, keep going
	    continue;

	// no character available or interrupted, return zero
	break;
    }
    return 0;
}
#endif

#if defined(FEAT_TIMERS) || defined(PROTO)
/*
 * Wait for a timer to fire or "wait_func" to return non-zero.
 * Returns OK when something was read.
 * Returns FAIL when it timed out or was interrupted.
 */
    int
ui_wait_for_chars_or_timer(
    long    wtime,
    int	    (*wait_func)(long wtime, int *interrupted, int ignore_input),
    int	    *interrupted,
    int	    ignore_input)
{
    int	    due_time;
    long    remaining = wtime;
    int	    tb_change_cnt = typebuf.tb_change_cnt;
# ifdef FEAT_JOB_CHANNEL
    int	    brief_wait = FALSE;
# endif

    // When waiting very briefly don't trigger timers.
    if (wtime >= 0 && wtime < 10L)
	return wait_func(wtime, NULL, ignore_input);

    while (wtime < 0 || remaining > 0)
    {
	// Trigger timers and then get the time in wtime until the next one is
	// due.  Wait up to that time.
	due_time = check_due_timer();
	if (typebuf.tb_change_cnt != tb_change_cnt)
	{
	    /* timer may have used feedkeys() */
	    return FAIL;
	}
	if (due_time <= 0 || (wtime > 0 && due_time > remaining))
	    due_time = remaining;
# ifdef FEAT_JOB_CHANNEL
	if ((due_time < 0 || due_time > 10L)
#  ifdef FEAT_GUI
		&& !gui.in_use
#  endif
		&& (has_pending_job() || channel_any_readahead()))
	{
	    // There is a pending job or channel, should return soon in order
	    // to handle them ASAP.  Do check for input briefly.
	    due_time = 10L;
	    brief_wait = TRUE;
	}
# endif
	if (wait_func(due_time, interrupted, ignore_input))
	    return OK;
	if ((interrupted != NULL && *interrupted)
# ifdef FEAT_JOB_CHANNEL
		|| brief_wait
# endif
		)
	    // Nothing available, but need to return so that side effects get
	    // handled, such as handling a message on a channel.
	    return FAIL;
	if (wtime > 0)
	    remaining -= due_time;
    }
    return FAIL;
}
#endif

/*
 * Return non-zero if a character is available.
 */
    int
ui_char_avail(void)
{
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui_mch_update();
	return input_available();
    }
#endif
#ifndef NO_CONSOLE
# ifdef NO_CONSOLE_INPUT
    if (no_console_input())
	return 0;
# endif
    return mch_char_avail();
#else
    return 0;
#endif
}

/*
 * Delay for the given number of milliseconds.	If ignoreinput is FALSE then we
 * cancel the delay if a key is hit.
 */
    void
ui_delay(long msec, int ignoreinput)
{
#ifdef FEAT_GUI
    if (gui.in_use && !ignoreinput)
	gui_wait_for_chars(msec, typebuf.tb_change_cnt);
    else
#endif
	mch_delay(msec, ignoreinput);
}

/*
 * If the machine has job control, use it to suspend the program,
 * otherwise fake it by starting a new shell.
 * When running the GUI iconify the window.
 */
    void
ui_suspend(void)
{
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui_mch_iconify();
	return;
    }
#endif
    mch_suspend();
}

#if !defined(UNIX) || !defined(SIGTSTP) || defined(PROTO) || defined(__BEOS__)
/*
 * When the OS can't really suspend, call this function to start a shell.
 * This is never called in the GUI.
 */
    void
suspend_shell(void)
{
    if (*p_sh == NUL)
	emsg(_(e_shellempty));
    else
    {
	msg_puts(_("new shell started\n"));
	do_shell(NULL, 0);
    }
}
#endif

/*
 * Try to get the current Vim shell size.  Put the result in Rows and Columns.
 * Use the new sizes as defaults for 'columns' and 'lines'.
 * Return OK when size could be determined, FAIL otherwise.
 */
    int
ui_get_shellsize(void)
{
    int	    retval;

#ifdef FEAT_GUI
    if (gui.in_use)
	retval = gui_get_shellsize();
    else
#endif
	retval = mch_get_shellsize();

    check_shellsize();

    /* adjust the default for 'lines' and 'columns' */
    if (retval == OK)
    {
	set_number_default("lines", Rows);
	set_number_default("columns", Columns);
    }
    return retval;
}

/*
 * Set the size of the Vim shell according to Rows and Columns, if possible.
 * The gui_set_shellsize() or mch_set_shellsize() function will try to set the
 * new size.  If this is not possible, it will adjust Rows and Columns.
 */
    void
ui_set_shellsize(
    int		mustset UNUSED)	/* set by the user */
{
#ifdef FEAT_GUI
    if (gui.in_use)
	gui_set_shellsize(mustset, TRUE, RESIZE_BOTH);
    else
#endif
	mch_set_shellsize();
}

/*
 * Called when Rows and/or Columns changed.  Adjust scroll region and mouse
 * region.
 */
    void
ui_new_shellsize(void)
{
    if (full_screen && !exiting)
    {
#ifdef FEAT_GUI
	if (gui.in_use)
	    gui_new_shellsize();
	else
#endif
	    mch_new_shellsize();
    }
}

#if ((defined(FEAT_EVAL) || defined(FEAT_TERMINAL)) \
	    && (defined(FEAT_GUI) \
		|| defined(MSWIN) \
		|| (defined(HAVE_TGETENT) && defined(FEAT_TERMRESPONSE)))) \
	|| defined(PROTO)
/*
 * Get the window position in pixels, if possible.
 * Return FAIL when not possible.
 */
    int
ui_get_winpos(int *x, int *y, varnumber_T timeout)
{
# ifdef FEAT_GUI
    if (gui.in_use)
	return gui_mch_get_winpos(x, y);
# endif
# if defined(MSWIN) && (!defined(FEAT_GUI) || defined(VIMDLL))
    return mch_get_winpos(x, y);
# else
#  if defined(HAVE_TGETENT) && defined(FEAT_TERMRESPONSE)
    return term_get_winpos(x, y, timeout);
#  else
    return FAIL;
#  endif
# endif
}
#endif

    void
ui_breakcheck(void)
{
    ui_breakcheck_force(FALSE);
}

/*
 * When "force" is true also check when the terminal is not in raw mode.
 * This is useful to read input on channels.
 */
    void
ui_breakcheck_force(int force)
{
    static int	recursive = FALSE;
    int		save_updating_screen = updating_screen;

    // We could be called recursively if stderr is redirected, calling
    // fill_input_buf() calls settmode() when stdin isn't a tty.  settmode()
    // calls vgetorpeek() which calls ui_breakcheck() again.
    if (recursive)
	return;
    recursive = TRUE;

    // We do not want gui_resize_shell() to redraw the screen here.
    ++updating_screen;

#ifdef FEAT_GUI
    if (gui.in_use)
	gui_mch_update();
    else
#endif
	mch_breakcheck(force);

    if (save_updating_screen)
	updating_screen = TRUE;
    else
	reset_updating_screen(FALSE);

    recursive = FALSE;
}

/*****************************************************************************
 * Functions for copying and pasting text between applications.
 * This is always included in a GUI version, but may also be included when the
 * clipboard and mouse is available to a terminal version such as xterm.
 * Note: there are some more functions in ops.c that handle selection stuff.
 *
 * Also note that the majority of functions here deal with the X 'primary'
 * (visible - for Visual mode use) selection, and only that. There are no
 * versions of these for the 'clipboard' selection, as Visual mode has no use
 * for them.
 */

#if defined(FEAT_CLIPBOARD) || defined(PROTO)

/*
 * Selection stuff using Visual mode, for cutting and pasting text to other
 * windows.
 */

/*
 * Call this to initialise the clipboard.  Pass it FALSE if the clipboard code
 * is included, but the clipboard can not be used, or TRUE if the clipboard can
 * be used.  Eg unix may call this with FALSE, then call it again with TRUE if
 * the GUI starts.
 */
    void
clip_init(int can_use)
{
    VimClipboard *cb;

    cb = &clip_star;
    for (;;)
    {
	cb->available  = can_use;
	cb->owned      = FALSE;
	cb->start.lnum = 0;
	cb->start.col  = 0;
	cb->end.lnum   = 0;
	cb->end.col    = 0;
	cb->state      = SELECT_CLEARED;

	if (cb == &clip_plus)
	    break;
	cb = &clip_plus;
    }
}

/*
 * Check whether the VIsual area has changed, and if so try to become the owner
 * of the selection, and free any old converted selection we may still have
 * lying around.  If the VIsual mode has ended, make a copy of what was
 * selected so we can still give it to others.	Will probably have to make sure
 * this is called whenever VIsual mode is ended.
 */
    void
clip_update_selection(VimClipboard *clip)
{
    pos_T	    start, end;

    /* If visual mode is only due to a redo command ("."), then ignore it */
    if (!redo_VIsual_busy && VIsual_active && (State & NORMAL))
    {
	if (LT_POS(VIsual, curwin->w_cursor))
	{
	    start = VIsual;
	    end = curwin->w_cursor;
	    if (has_mbyte)
		end.col += (*mb_ptr2len)(ml_get_cursor()) - 1;
	}
	else
	{
	    start = curwin->w_cursor;
	    end = VIsual;
	}
	if (!EQUAL_POS(clip->start, start)
		|| !EQUAL_POS(clip->end, end)
		|| clip->vmode != VIsual_mode)
	{
	    clip_clear_selection(clip);
	    clip->start = start;
	    clip->end = end;
	    clip->vmode = VIsual_mode;
	    clip_free_selection(clip);
	    clip_own_selection(clip);
	    clip_gen_set_selection(clip);
	}
    }
}

    void
clip_own_selection(VimClipboard *cbd)
{
    /*
     * Also want to check somehow that we are reading from the keyboard rather
     * than a mapping etc.
     */
#ifdef FEAT_X11
    /* Always own the selection, we might have lost it without being
     * notified, e.g. during a ":sh" command. */
    if (cbd->available)
    {
	int was_owned = cbd->owned;

	cbd->owned = (clip_gen_own_selection(cbd) == OK);
	if (!was_owned && (cbd == &clip_star || cbd == &clip_plus))
	{
	    /* May have to show a different kind of highlighting for the
	     * selected area.  There is no specific redraw command for this,
	     * just redraw all windows on the current buffer. */
	    if (cbd->owned
		    && (get_real_state() == VISUAL
					    || get_real_state() == SELECTMODE)
		    && (cbd == &clip_star ? clip_isautosel_star()
						      : clip_isautosel_plus())
		    && HL_ATTR(HLF_V) != HL_ATTR(HLF_VNC))
		redraw_curbuf_later(INVERTED_ALL);
	}
    }
#else
    /* Only own the clipboard when we didn't own it yet. */
    if (!cbd->owned && cbd->available)
	cbd->owned = (clip_gen_own_selection(cbd) == OK);
#endif
}

    void
clip_lose_selection(VimClipboard *cbd)
{
#ifdef FEAT_X11
    int	    was_owned = cbd->owned;
#endif
    int     visual_selection = FALSE;

    if (cbd == &clip_star || cbd == &clip_plus)
	visual_selection = TRUE;

    clip_free_selection(cbd);
    cbd->owned = FALSE;
    if (visual_selection)
	clip_clear_selection(cbd);
    clip_gen_lose_selection(cbd);
#ifdef FEAT_X11
    if (visual_selection)
    {
	/* May have to show a different kind of highlighting for the selected
	 * area.  There is no specific redraw command for this, just redraw all
	 * windows on the current buffer. */
	if (was_owned
		&& (get_real_state() == VISUAL
					    || get_real_state() == SELECTMODE)
		&& (cbd == &clip_star ?
				clip_isautosel_star() : clip_isautosel_plus())
		&& HL_ATTR(HLF_V) != HL_ATTR(HLF_VNC))
	{
	    update_curbuf(INVERTED_ALL);
	    setcursor();
	    cursor_on();
	    out_flush_cursor(TRUE, FALSE);
	}
    }
#endif
}

    static void
clip_copy_selection(VimClipboard *clip)
{
    if (VIsual_active && (State & NORMAL) && clip->available)
    {
	clip_update_selection(clip);
	clip_free_selection(clip);
	clip_own_selection(clip);
	if (clip->owned)
	    clip_get_selection(clip);
	clip_gen_set_selection(clip);
    }
}

/*
 * Save and restore clip_unnamed before doing possibly many changes. This
 * prevents accessing the clipboard very often which might slow down Vim
 * considerably.
 */
static int global_change_count = 0; /* if set, inside a start_global_changes */
static int clipboard_needs_update = FALSE; /* clipboard needs to be updated */
static int clip_did_set_selection = TRUE;

/*
 * Save clip_unnamed and reset it.
 */
    void
start_global_changes(void)
{
    if (++global_change_count > 1)
	return;
    clip_unnamed_saved = clip_unnamed;
    clipboard_needs_update = FALSE;

    if (clip_did_set_selection)
    {
	clip_unnamed = FALSE;
	clip_did_set_selection = FALSE;
    }
}

/*
 * Return TRUE if setting the clipboard was postponed, it already contains the
 * right text.
 */
    int
is_clipboard_needs_update()
{
    return clipboard_needs_update;
}

/*
 * Restore clip_unnamed and set the selection when needed.
 */
    void
end_global_changes(void)
{
    if (--global_change_count > 0)
	/* recursive */
	return;
    if (!clip_did_set_selection)
    {
	clip_did_set_selection = TRUE;
	clip_unnamed = clip_unnamed_saved;
	clip_unnamed_saved = FALSE;
	if (clipboard_needs_update)
	{
	    /* only store something in the clipboard,
	     * if we have yanked anything to it */
	    if (clip_unnamed & CLIP_UNNAMED)
	    {
		clip_own_selection(&clip_star);
		clip_gen_set_selection(&clip_star);
	    }
	    if (clip_unnamed & CLIP_UNNAMED_PLUS)
	    {
		clip_own_selection(&clip_plus);
		clip_gen_set_selection(&clip_plus);
	    }
	}
    }
    clipboard_needs_update = FALSE;
}

/*
 * Called when Visual mode is ended: update the selection.
 */
    void
clip_auto_select(void)
{
    if (clip_isautosel_star())
	clip_copy_selection(&clip_star);
    if (clip_isautosel_plus())
	clip_copy_selection(&clip_plus);
}

/*
 * Return TRUE if automatic selection of Visual area is desired for the *
 * register.
 */
    int
clip_isautosel_star(void)
{
    return (
#ifdef FEAT_GUI
	    gui.in_use ? (vim_strchr(p_go, GO_ASEL) != NULL) :
#endif
	    clip_autoselect_star);
}

/*
 * Return TRUE if automatic selection of Visual area is desired for the +
 * register.
 */
    int
clip_isautosel_plus(void)
{
    return (
#ifdef FEAT_GUI
	    gui.in_use ? (vim_strchr(p_go, GO_ASELPLUS) != NULL) :
#endif
	    clip_autoselect_plus);
}


/*
 * Stuff for general mouse selection, without using Visual mode.
 */

static void clip_invert_area(int, int, int, int, int how);
static void clip_invert_rectangle(int row, int col, int height, int width, int invert);
static void clip_get_word_boundaries(VimClipboard *, int, int);
static int  clip_get_line_end(int);
static void clip_update_modeless_selection(VimClipboard *, int, int,
						    int, int);

/* flags for clip_invert_area() */
#define CLIP_CLEAR	1
#define CLIP_SET	2
#define CLIP_TOGGLE	3

/*
 * Start, continue or end a modeless selection.  Used when editing the
 * command-line and in the cmdline window.
 */
    void
clip_modeless(int button, int is_click, int is_drag)
{
    int		repeat;

    repeat = ((clip_star.mode == SELECT_MODE_CHAR
		|| clip_star.mode == SELECT_MODE_LINE)
					      && (mod_mask & MOD_MASK_2CLICK))
	    || (clip_star.mode == SELECT_MODE_WORD
					     && (mod_mask & MOD_MASK_3CLICK));
    if (is_click && button == MOUSE_RIGHT)
    {
	/* Right mouse button: If there was no selection, start one.
	 * Otherwise extend the existing selection. */
	if (clip_star.state == SELECT_CLEARED)
	    clip_start_selection(mouse_col, mouse_row, FALSE);
	clip_process_selection(button, mouse_col, mouse_row, repeat);
    }
    else if (is_click)
	clip_start_selection(mouse_col, mouse_row, repeat);
    else if (is_drag)
    {
	/* Don't try extending a selection if there isn't one.  Happens when
	 * button-down is in the cmdline and them moving mouse upwards. */
	if (clip_star.state != SELECT_CLEARED)
	    clip_process_selection(button, mouse_col, mouse_row, repeat);
    }
    else /* release */
	clip_process_selection(MOUSE_RELEASE, mouse_col, mouse_row, FALSE);
}

/*
 * Compare two screen positions ala strcmp()
 */
    static int
clip_compare_pos(
    int		row1,
    int		col1,
    int		row2,
    int		col2)
{
    if (row1 > row2) return(1);
    if (row1 < row2) return(-1);
    if (col1 > col2) return(1);
    if (col1 < col2) return(-1);
    return(0);
}

/*
 * Start the selection
 */
    void
clip_start_selection(int col, int row, int repeated_click)
{
    VimClipboard	*cb = &clip_star;

    if (cb->state == SELECT_DONE)
	clip_clear_selection(cb);

    row = check_row(row);
    col = check_col(col);
    col = mb_fix_col(col, row);

    cb->start.lnum  = row;
    cb->start.col   = col;
    cb->end	    = cb->start;
    cb->origin_row  = (short_u)cb->start.lnum;
    cb->state	    = SELECT_IN_PROGRESS;

    if (repeated_click)
    {
	if (++cb->mode > SELECT_MODE_LINE)
	    cb->mode = SELECT_MODE_CHAR;
    }
    else
	cb->mode = SELECT_MODE_CHAR;

#ifdef FEAT_GUI
    /* clear the cursor until the selection is made */
    if (gui.in_use)
	gui_undraw_cursor();
#endif

    switch (cb->mode)
    {
	case SELECT_MODE_CHAR:
	    cb->origin_start_col = cb->start.col;
	    cb->word_end_col = clip_get_line_end((int)cb->start.lnum);
	    break;

	case SELECT_MODE_WORD:
	    clip_get_word_boundaries(cb, (int)cb->start.lnum, cb->start.col);
	    cb->origin_start_col = cb->word_start_col;
	    cb->origin_end_col	 = cb->word_end_col;

	    clip_invert_area((int)cb->start.lnum, cb->word_start_col,
			    (int)cb->end.lnum, cb->word_end_col, CLIP_SET);
	    cb->start.col = cb->word_start_col;
	    cb->end.col   = cb->word_end_col;
	    break;

	case SELECT_MODE_LINE:
	    clip_invert_area((int)cb->start.lnum, 0, (int)cb->start.lnum,
			    (int)Columns, CLIP_SET);
	    cb->start.col = 0;
	    cb->end.col   = Columns;
	    break;
    }

    cb->prev = cb->start;

#ifdef DEBUG_SELECTION
    printf("Selection started at (%u,%u)\n", cb->start.lnum, cb->start.col);
#endif
}

/*
 * Continue processing the selection
 */
    void
clip_process_selection(
    int		button,
    int		col,
    int		row,
    int_u	repeated_click)
{
    VimClipboard	*cb = &clip_star;
    int			diff;
    int			slen = 1;	/* cursor shape width */

    if (button == MOUSE_RELEASE)
    {
	/* Check to make sure we have something selected */
	if (cb->start.lnum == cb->end.lnum && cb->start.col == cb->end.col)
	{
#ifdef FEAT_GUI
	    if (gui.in_use)
		gui_update_cursor(FALSE, FALSE);
#endif
	    cb->state = SELECT_CLEARED;
	    return;
	}

#ifdef DEBUG_SELECTION
	printf("Selection ended: (%u,%u) to (%u,%u)\n", cb->start.lnum,
		cb->start.col, cb->end.lnum, cb->end.col);
#endif
	if (clip_isautosel_star()
		|| (
#ifdef FEAT_GUI
		    gui.in_use ? (vim_strchr(p_go, GO_ASELML) != NULL) :
#endif
		    clip_autoselectml))
	    clip_copy_modeless_selection(FALSE);
#ifdef FEAT_GUI
	if (gui.in_use)
	    gui_update_cursor(FALSE, FALSE);
#endif

	cb->state = SELECT_DONE;
	return;
    }

    row = check_row(row);
    col = check_col(col);
    col = mb_fix_col(col, row);

    if (col == (int)cb->prev.col && row == cb->prev.lnum && !repeated_click)
	return;

    /*
     * When extending the selection with the right mouse button, swap the
     * start and end if the position is before half the selection
     */
    if (cb->state == SELECT_DONE && button == MOUSE_RIGHT)
    {
	/*
	 * If the click is before the start, or the click is inside the
	 * selection and the start is the closest side, set the origin to the
	 * end of the selection.
	 */
	if (clip_compare_pos(row, col, (int)cb->start.lnum, cb->start.col) < 0
		|| (clip_compare_pos(row, col,
					   (int)cb->end.lnum, cb->end.col) < 0
		    && (((cb->start.lnum == cb->end.lnum
			    && cb->end.col - col > col - cb->start.col))
			|| ((diff = (cb->end.lnum - row) -
						   (row - cb->start.lnum)) > 0
			    || (diff == 0 && col < (int)(cb->start.col +
							 cb->end.col) / 2)))))
	{
	    cb->origin_row = (short_u)cb->end.lnum;
	    cb->origin_start_col = cb->end.col - 1;
	    cb->origin_end_col = cb->end.col;
	}
	else
	{
	    cb->origin_row = (short_u)cb->start.lnum;
	    cb->origin_start_col = cb->start.col;
	    cb->origin_end_col = cb->start.col;
	}
	if (cb->mode == SELECT_MODE_WORD && !repeated_click)
	    cb->mode = SELECT_MODE_CHAR;
    }

    /* set state, for when using the right mouse button */
    cb->state = SELECT_IN_PROGRESS;

#ifdef DEBUG_SELECTION
    printf("Selection extending to (%d,%d)\n", row, col);
#endif

    if (repeated_click && ++cb->mode > SELECT_MODE_LINE)
	cb->mode = SELECT_MODE_CHAR;

    switch (cb->mode)
    {
	case SELECT_MODE_CHAR:
	    /* If we're on a different line, find where the line ends */
	    if (row != cb->prev.lnum)
		cb->word_end_col = clip_get_line_end(row);

	    /* See if we are before or after the origin of the selection */
	    if (clip_compare_pos(row, col, cb->origin_row,
						   cb->origin_start_col) >= 0)
	    {
		if (col >= (int)cb->word_end_col)
		    clip_update_modeless_selection(cb, cb->origin_row,
			    cb->origin_start_col, row, (int)Columns);
		else
		{
		    if (has_mbyte && mb_lefthalve(row, col))
			slen = 2;
		    clip_update_modeless_selection(cb, cb->origin_row,
			    cb->origin_start_col, row, col + slen);
		}
	    }
	    else
	    {
		if (has_mbyte
			&& mb_lefthalve(cb->origin_row, cb->origin_start_col))
		    slen = 2;
		if (col >= (int)cb->word_end_col)
		    clip_update_modeless_selection(cb, row, cb->word_end_col,
			    cb->origin_row, cb->origin_start_col + slen);
		else
		    clip_update_modeless_selection(cb, row, col,
			    cb->origin_row, cb->origin_start_col + slen);
	    }
	    break;

	case SELECT_MODE_WORD:
	    /* If we are still within the same word, do nothing */
	    if (row == cb->prev.lnum && col >= (int)cb->word_start_col
		    && col < (int)cb->word_end_col && !repeated_click)
		return;

	    /* Get new word boundaries */
	    clip_get_word_boundaries(cb, row, col);

	    /* Handle being after the origin point of selection */
	    if (clip_compare_pos(row, col, cb->origin_row,
		    cb->origin_start_col) >= 0)
		clip_update_modeless_selection(cb, cb->origin_row,
			cb->origin_start_col, row, cb->word_end_col);
	    else
		clip_update_modeless_selection(cb, row, cb->word_start_col,
			cb->origin_row, cb->origin_end_col);
	    break;

	case SELECT_MODE_LINE:
	    if (row == cb->prev.lnum && !repeated_click)
		return;

	    if (clip_compare_pos(row, col, cb->origin_row,
		    cb->origin_start_col) >= 0)
		clip_update_modeless_selection(cb, cb->origin_row, 0, row,
			(int)Columns);
	    else
		clip_update_modeless_selection(cb, row, 0, cb->origin_row,
			(int)Columns);
	    break;
    }

    cb->prev.lnum = row;
    cb->prev.col  = col;

#ifdef DEBUG_SELECTION
	printf("Selection is: (%u,%u) to (%u,%u)\n", cb->start.lnum,
		cb->start.col, cb->end.lnum, cb->end.col);
#endif
}

# if defined(FEAT_GUI) || defined(PROTO)
/*
 * Redraw part of the selection if character at "row,col" is inside of it.
 * Only used for the GUI.
 */
    void
clip_may_redraw_selection(int row, int col, int len)
{
    int		start = col;
    int		end = col + len;

    if (clip_star.state != SELECT_CLEARED
	    && row >= clip_star.start.lnum
	    && row <= clip_star.end.lnum)
    {
	if (row == clip_star.start.lnum && start < (int)clip_star.start.col)
	    start = clip_star.start.col;
	if (row == clip_star.end.lnum && end > (int)clip_star.end.col)
	    end = clip_star.end.col;
	if (end > start)
	    clip_invert_area(row, start, row, end, 0);
    }
}
# endif

/*
 * Called from outside to clear selected region from the display
 */
    void
clip_clear_selection(VimClipboard *cbd)
{

    if (cbd->state == SELECT_CLEARED)
	return;

    clip_invert_area((int)cbd->start.lnum, cbd->start.col, (int)cbd->end.lnum,
						     cbd->end.col, CLIP_CLEAR);
    cbd->state = SELECT_CLEARED;
}

/*
 * Clear the selection if any lines from "row1" to "row2" are inside of it.
 */
    void
clip_may_clear_selection(int row1, int row2)
{
    if (clip_star.state == SELECT_DONE
	    && row2 >= clip_star.start.lnum
	    && row1 <= clip_star.end.lnum)
	clip_clear_selection(&clip_star);
}

/*
 * Called before the screen is scrolled up or down.  Adjusts the line numbers
 * of the selection.  Call with big number when clearing the screen.
 */
    void
clip_scroll_selection(
    int	    rows)		/* negative for scroll down */
{
    int	    lnum;

    if (clip_star.state == SELECT_CLEARED)
	return;

    lnum = clip_star.start.lnum - rows;
    if (lnum <= 0)
	clip_star.start.lnum = 0;
    else if (lnum >= screen_Rows)	/* scrolled off of the screen */
	clip_star.state = SELECT_CLEARED;
    else
	clip_star.start.lnum = lnum;

    lnum = clip_star.end.lnum - rows;
    if (lnum < 0)			/* scrolled off of the screen */
	clip_star.state = SELECT_CLEARED;
    else if (lnum >= screen_Rows)
	clip_star.end.lnum = screen_Rows - 1;
    else
	clip_star.end.lnum = lnum;
}

/*
 * Invert a region of the display between a starting and ending row and column
 * Values for "how":
 * CLIP_CLEAR:  undo inversion
 * CLIP_SET:    set inversion
 * CLIP_TOGGLE: set inversion if pos1 < pos2, undo inversion otherwise.
 * 0: invert (GUI only).
 */
    static void
clip_invert_area(
    int		row1,
    int		col1,
    int		row2,
    int		col2,
    int		how)
{
    int		invert = FALSE;

    if (how == CLIP_SET)
	invert = TRUE;

    /* Swap the from and to positions so the from is always before */
    if (clip_compare_pos(row1, col1, row2, col2) > 0)
    {
	int tmp_row, tmp_col;

	tmp_row = row1;
	tmp_col = col1;
	row1	= row2;
	col1	= col2;
	row2	= tmp_row;
	col2	= tmp_col;
    }
    else if (how == CLIP_TOGGLE)
	invert = TRUE;

    /* If all on the same line, do it the easy way */
    if (row1 == row2)
    {
	clip_invert_rectangle(row1, col1, 1, col2 - col1, invert);
    }
    else
    {
	/* Handle a piece of the first line */
	if (col1 > 0)
	{
	    clip_invert_rectangle(row1, col1, 1, (int)Columns - col1, invert);
	    row1++;
	}

	/* Handle a piece of the last line */
	if (col2 < Columns - 1)
	{
	    clip_invert_rectangle(row2, 0, 1, col2, invert);
	    row2--;
	}

	/* Handle the rectangle thats left */
	if (row2 >= row1)
	    clip_invert_rectangle(row1, 0, row2 - row1 + 1, (int)Columns,
								      invert);
    }
}

/*
 * Invert or un-invert a rectangle of the screen.
 * "invert" is true if the result is inverted.
 */
    static void
clip_invert_rectangle(
    int		row,
    int		col,
    int		height,
    int		width,
    int		invert)
{
#ifdef FEAT_GUI
    if (gui.in_use)
	gui_mch_invert_rectangle(row, col, height, width);
    else
#endif
	screen_draw_rectangle(row, col, height, width, invert);
}

/*
 * Copy the currently selected area into the '*' register so it will be
 * available for pasting.
 * When "both" is TRUE also copy to the '+' register.
 */
    void
clip_copy_modeless_selection(int both UNUSED)
{
    char_u	*buffer;
    char_u	*bufp;
    int		row;
    int		start_col;
    int		end_col;
    int		line_end_col;
    int		add_newline_flag = FALSE;
    int		len;
    char_u	*p;
    int		row1 = clip_star.start.lnum;
    int		col1 = clip_star.start.col;
    int		row2 = clip_star.end.lnum;
    int		col2 = clip_star.end.col;

    /* Can't use ScreenLines unless initialized */
    if (ScreenLines == NULL)
	return;

    /*
     * Make sure row1 <= row2, and if row1 == row2 that col1 <= col2.
     */
    if (row1 > row2)
    {
	row = row1; row1 = row2; row2 = row;
	row = col1; col1 = col2; col2 = row;
    }
    else if (row1 == row2 && col1 > col2)
    {
	row = col1; col1 = col2; col2 = row;
    }
    /* correct starting point for being on right halve of double-wide char */
    p = ScreenLines + LineOffset[row1];
    if (enc_dbcs != 0)
	col1 -= (*mb_head_off)(p, p + col1);
    else if (enc_utf8 && p[col1] == 0)
	--col1;

    /* Create a temporary buffer for storing the text */
    len = (row2 - row1 + 1) * Columns + 1;
    if (enc_dbcs != 0)
	len *= 2;	/* max. 2 bytes per display cell */
    else if (enc_utf8)
	len *= MB_MAXBYTES;
    buffer = lalloc((long_u)len, TRUE);
    if (buffer == NULL)	    /* out of memory */
	return;

    /* Process each row in the selection */
    for (bufp = buffer, row = row1; row <= row2; row++)
    {
	if (row == row1)
	    start_col = col1;
	else
	    start_col = 0;

	if (row == row2)
	    end_col = col2;
	else
	    end_col = Columns;

	line_end_col = clip_get_line_end(row);

	/* See if we need to nuke some trailing whitespace */
	if (end_col >= Columns && (row < row2 || end_col > line_end_col))
	{
	    /* Get rid of trailing whitespace */
	    end_col = line_end_col;
	    if (end_col < start_col)
		end_col = start_col;

	    /* If the last line extended to the end, add an extra newline */
	    if (row == row2)
		add_newline_flag = TRUE;
	}

	/* If after the first row, we need to always add a newline */
	if (row > row1 && !LineWraps[row - 1])
	    *bufp++ = NL;

	if (row < screen_Rows && end_col <= screen_Columns)
	{
	    if (enc_dbcs != 0)
	    {
		int	i;

		p = ScreenLines + LineOffset[row];
		for (i = start_col; i < end_col; ++i)
		    if (enc_dbcs == DBCS_JPNU && p[i] == 0x8e)
		    {
			/* single-width double-byte char */
			*bufp++ = 0x8e;
			*bufp++ = ScreenLines2[LineOffset[row] + i];
		    }
		    else
		    {
			*bufp++ = p[i];
			if (MB_BYTE2LEN(p[i]) == 2)
			    *bufp++ = p[++i];
		    }
	    }
	    else if (enc_utf8)
	    {
		int	off;
		int	i;
		int	ci;

		off = LineOffset[row];
		for (i = start_col; i < end_col; ++i)
		{
		    /* The base character is either in ScreenLinesUC[] or
		     * ScreenLines[]. */
		    if (ScreenLinesUC[off + i] == 0)
			*bufp++ = ScreenLines[off + i];
		    else
		    {
			bufp += utf_char2bytes(ScreenLinesUC[off + i], bufp);
			for (ci = 0; ci < Screen_mco; ++ci)
			{
			    /* Add a composing character. */
			    if (ScreenLinesC[ci][off + i] == 0)
				break;
			    bufp += utf_char2bytes(ScreenLinesC[ci][off + i],
									bufp);
			}
		    }
		    /* Skip right halve of double-wide character. */
		    if (ScreenLines[off + i + 1] == 0)
			++i;
		}
	    }
	    else
	    {
		STRNCPY(bufp, ScreenLines + LineOffset[row] + start_col,
							 end_col - start_col);
		bufp += end_col - start_col;
	    }
	}
    }

    /* Add a newline at the end if the selection ended there */
    if (add_newline_flag)
	*bufp++ = NL;

    /* First cleanup any old selection and become the owner. */
    clip_free_selection(&clip_star);
    clip_own_selection(&clip_star);

    /* Yank the text into the '*' register. */
    clip_yank_selection(MCHAR, buffer, (long)(bufp - buffer), &clip_star);

    /* Make the register contents available to the outside world. */
    clip_gen_set_selection(&clip_star);

#ifdef FEAT_X11
    if (both)
    {
	/* Do the same for the '+' register. */
	clip_free_selection(&clip_plus);
	clip_own_selection(&clip_plus);
	clip_yank_selection(MCHAR, buffer, (long)(bufp - buffer), &clip_plus);
	clip_gen_set_selection(&clip_plus);
    }
#endif
    vim_free(buffer);
}

/*
 * Find the starting and ending positions of the word at the given row and
 * column.  Only white-separated words are recognized here.
 */
#define CHAR_CLASS(c)	(c <= ' ' ? ' ' : vim_iswordc(c))

    static void
clip_get_word_boundaries(VimClipboard *cb, int row, int col)
{
    int		start_class;
    int		temp_col;
    char_u	*p;
    int		mboff;

    if (row >= screen_Rows || col >= screen_Columns || ScreenLines == NULL)
	return;

    p = ScreenLines + LineOffset[row];
    /* Correct for starting in the right halve of a double-wide char */
    if (enc_dbcs != 0)
	col -= dbcs_screen_head_off(p, p + col);
    else if (enc_utf8 && p[col] == 0)
	--col;
    start_class = CHAR_CLASS(p[col]);

    temp_col = col;
    for ( ; temp_col > 0; temp_col--)
	if (enc_dbcs != 0
		   && (mboff = dbcs_screen_head_off(p, p + temp_col - 1)) > 0)
	    temp_col -= mboff;
	else if (CHAR_CLASS(p[temp_col - 1]) != start_class
		&& !(enc_utf8 && p[temp_col - 1] == 0))
	    break;
    cb->word_start_col = temp_col;

    temp_col = col;
    for ( ; temp_col < screen_Columns; temp_col++)
	if (enc_dbcs != 0 && dbcs_ptr2cells(p + temp_col) == 2)
	    ++temp_col;
	else if (CHAR_CLASS(p[temp_col]) != start_class
		&& !(enc_utf8 && p[temp_col] == 0))
	    break;
    cb->word_end_col = temp_col;
}

/*
 * Find the column position for the last non-whitespace character on the given
 * line.
 */
    static int
clip_get_line_end(int row)
{
    int	    i;

    if (row >= screen_Rows || ScreenLines == NULL)
	return 0;
    for (i = screen_Columns; i > 0; i--)
	if (ScreenLines[LineOffset[row] + i - 1] != ' ')
	    break;
    return i;
}

/*
 * Update the currently selected region by adding and/or subtracting from the
 * beginning or end and inverting the changed area(s).
 */
    static void
clip_update_modeless_selection(
    VimClipboard    *cb,
    int		    row1,
    int		    col1,
    int		    row2,
    int		    col2)
{
    /* See if we changed at the beginning of the selection */
    if (row1 != cb->start.lnum || col1 != (int)cb->start.col)
    {
	clip_invert_area(row1, col1, (int)cb->start.lnum, cb->start.col,
								 CLIP_TOGGLE);
	cb->start.lnum = row1;
	cb->start.col  = col1;
    }

    /* See if we changed at the end of the selection */
    if (row2 != cb->end.lnum || col2 != (int)cb->end.col)
    {
	clip_invert_area((int)cb->end.lnum, cb->end.col, row2, col2,
								 CLIP_TOGGLE);
	cb->end.lnum = row2;
	cb->end.col  = col2;
    }
}

    int
clip_gen_own_selection(VimClipboard *cbd)
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	return clip_mch_own_selection(cbd);
    else
# endif
	return clip_xterm_own_selection(cbd);
#else
    return clip_mch_own_selection(cbd);
#endif
}

    void
clip_gen_lose_selection(VimClipboard *cbd)
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	clip_mch_lose_selection(cbd);
    else
# endif
	clip_xterm_lose_selection(cbd);
#else
    clip_mch_lose_selection(cbd);
#endif
}

    void
clip_gen_set_selection(VimClipboard *cbd)
{
    if (!clip_did_set_selection)
    {
	/* Updating postponed, so that accessing the system clipboard won't
	 * hang Vim when accessing it many times (e.g. on a :g command). */
	if ((cbd == &clip_plus && (clip_unnamed_saved & CLIP_UNNAMED_PLUS))
		|| (cbd == &clip_star && (clip_unnamed_saved & CLIP_UNNAMED)))
	{
	    clipboard_needs_update = TRUE;
	    return;
	}
    }
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	clip_mch_set_selection(cbd);
    else
# endif
	clip_xterm_set_selection(cbd);
#else
    clip_mch_set_selection(cbd);
#endif
}

    void
clip_gen_request_selection(VimClipboard *cbd)
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	clip_mch_request_selection(cbd);
    else
# endif
	clip_xterm_request_selection(cbd);
#else
    clip_mch_request_selection(cbd);
#endif
}

#if (defined(FEAT_X11) && defined(USE_SYSTEM)) || defined(PROTO)
    int
clip_gen_owner_exists(VimClipboard *cbd UNUSED)
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI_GTK
    if (gui.in_use)
	return clip_gtk_owner_exists(cbd);
    else
# endif
	return clip_x11_owner_exists(cbd);
#else
    return TRUE;
#endif
}
#endif

#endif /* FEAT_CLIPBOARD */

/*****************************************************************************
 * Functions that handle the input buffer.
 * This is used for any GUI version, and the unix terminal version.
 *
 * For Unix, the input characters are buffered to be able to check for a
 * CTRL-C.  This should be done with signals, but I don't know how to do that
 * in a portable way for a tty in RAW mode.
 *
 * For the client-server code in the console the received keys are put in the
 * input buffer.
 */

#if defined(USE_INPUT_BUF) || defined(PROTO)

/*
 * Internal typeahead buffer.  Includes extra space for long key code
 * descriptions which would otherwise overflow.  The buffer is considered full
 * when only this extra space (or part of it) remains.
 */
#if defined(FEAT_JOB_CHANNEL) || defined(FEAT_CLIENTSERVER)
   /*
    * NetBeans stuffs debugger commands into the input buffer.
    * This requires a larger buffer...
    * (Madsen) Go with this for remote input as well ...
    */
# define INBUFLEN 4096
#else
# define INBUFLEN 250
#endif

static char_u	inbuf[INBUFLEN + MAX_KEY_CODE_LEN];
static int	inbufcount = 0;	    /* number of chars in inbuf[] */

/*
 * vim_is_input_buf_full(), vim_is_input_buf_empty(), add_to_input_buf(), and
 * trash_input_buf() are functions for manipulating the input buffer.  These
 * are used by the gui_* calls when a GUI is used to handle keyboard input.
 */

    int
vim_is_input_buf_full(void)
{
    return (inbufcount >= INBUFLEN);
}

    int
vim_is_input_buf_empty(void)
{
    return (inbufcount == 0);
}

#if defined(FEAT_OLE) || defined(PROTO)
    int
vim_free_in_input_buf(void)
{
    return (INBUFLEN - inbufcount);
}
#endif

#if defined(FEAT_GUI_GTK) || defined(PROTO)
    int
vim_used_in_input_buf(void)
{
    return inbufcount;
}
#endif

/*
 * Return the current contents of the input buffer and make it empty.
 * The returned pointer must be passed to set_input_buf() later.
 */
    char_u *
get_input_buf(void)
{
    garray_T	*gap;

    /* We use a growarray to store the data pointer and the length. */
    gap = (garray_T *)alloc((unsigned)sizeof(garray_T));
    if (gap != NULL)
    {
	/* Add one to avoid a zero size. */
	gap->ga_data = alloc((unsigned)inbufcount + 1);
	if (gap->ga_data != NULL)
	    mch_memmove(gap->ga_data, inbuf, (size_t)inbufcount);
	gap->ga_len = inbufcount;
    }
    trash_input_buf();
    return (char_u *)gap;
}

/*
 * Restore the input buffer with a pointer returned from get_input_buf().
 * The allocated memory is freed, this only works once!
 */
    void
set_input_buf(char_u *p)
{
    garray_T	*gap = (garray_T *)p;

    if (gap != NULL)
    {
	if (gap->ga_data != NULL)
	{
	    mch_memmove(inbuf, gap->ga_data, gap->ga_len);
	    inbufcount = gap->ga_len;
	    vim_free(gap->ga_data);
	}
	vim_free(gap);
    }
}

/*
 * Add the given bytes to the input buffer
 * Special keys start with CSI.  A real CSI must have been translated to
 * CSI KS_EXTRA KE_CSI.  K_SPECIAL doesn't require translation.
 */
    void
add_to_input_buf(char_u *s, int len)
{
    if (inbufcount + len > INBUFLEN + MAX_KEY_CODE_LEN)
	return;	    /* Shouldn't ever happen! */

#ifdef FEAT_HANGULIN
    if ((State & (INSERT|CMDLINE)) && hangul_input_state_get())
	if ((len = hangul_input_process(s, len)) == 0)
	    return;
#endif

    while (len--)
	inbuf[inbufcount++] = *s++;
}

/*
 * Add "str[len]" to the input buffer while escaping CSI bytes.
 */
    void
add_to_input_buf_csi(char_u *str, int len)
{
    int		i;
    char_u	buf[2];

    for (i = 0; i < len; ++i)
    {
	add_to_input_buf(str + i, 1);
	if (str[i] == CSI)
	{
	    /* Turn CSI into K_CSI. */
	    buf[0] = KS_EXTRA;
	    buf[1] = (int)KE_CSI;
	    add_to_input_buf(buf, 2);
	}
    }
}

#if defined(FEAT_HANGULIN) || defined(PROTO)
    void
push_raw_key(char_u *s, int len)
{
    char_u *tmpbuf;
    char_u *inp = s;

    /* use the conversion result if possible */
    tmpbuf = hangul_string_convert(s, &len);
    if (tmpbuf != NULL)
	inp = tmpbuf;

    for (; len--; inp++)
    {
	inbuf[inbufcount++] = *inp;
	if (*inp == CSI)
	{
	    /* Turn CSI into K_CSI. */
	    inbuf[inbufcount++] = KS_EXTRA;
	    inbuf[inbufcount++] = (int)KE_CSI;
	}
    }
    vim_free(tmpbuf);
}
#endif

/* Remove everything from the input buffer.  Called when ^C is found */
    void
trash_input_buf(void)
{
    inbufcount = 0;
}

/*
 * Read as much data from the input buffer as possible up to maxlen, and store
 * it in buf.
 */
    int
read_from_input_buf(char_u *buf, long maxlen)
{
    if (inbufcount == 0)	/* if the buffer is empty, fill it */
	fill_input_buf(TRUE);
    if (maxlen > inbufcount)
	maxlen = inbufcount;
    mch_memmove(buf, inbuf, (size_t)maxlen);
    inbufcount -= maxlen;
    if (inbufcount)
	mch_memmove(inbuf, inbuf + maxlen, (size_t)inbufcount);
    return (int)maxlen;
}

    void
fill_input_buf(int exit_on_error UNUSED)
{
#if defined(UNIX) || defined(VMS) || defined(MACOS_X)
    int		len;
    int		try;
    static int	did_read_something = FALSE;
    static char_u *rest = NULL;	    /* unconverted rest of previous read */
    static int	restlen = 0;
    int		unconverted;
#endif

#ifdef FEAT_GUI
    if (gui.in_use
# ifdef NO_CONSOLE_INPUT
    /* Don't use the GUI input when the window hasn't been opened yet.
     * We get here from ui_inchar() when we should try reading from stdin. */
	    && !no_console_input()
# endif
       )
    {
	gui_mch_update();
	return;
    }
#endif
#if defined(UNIX) || defined(VMS) || defined(MACOS_X)
    if (vim_is_input_buf_full())
	return;
    /*
     * Fill_input_buf() is only called when we really need a character.
     * If we can't get any, but there is some in the buffer, just return.
     * If we can't get any, and there isn't any in the buffer, we give up and
     * exit Vim.
     */
# ifdef __BEOS__
    /*
     * On the BeBox version (for now), all input is secretly performed within
     * beos_select() which is called from RealWaitForChar().
     */
    while (!vim_is_input_buf_full() && RealWaitForChar(read_cmd_fd, 0, NULL))
	    ;
    len = inbufcount;
    inbufcount = 0;
# else

    if (rest != NULL)
    {
	/* Use remainder of previous call, starts with an invalid character
	 * that may become valid when reading more. */
	if (restlen > INBUFLEN - inbufcount)
	    unconverted = INBUFLEN - inbufcount;
	else
	    unconverted = restlen;
	mch_memmove(inbuf + inbufcount, rest, unconverted);
	if (unconverted == restlen)
	    VIM_CLEAR(rest);
	else
	{
	    restlen -= unconverted;
	    mch_memmove(rest, rest + unconverted, restlen);
	}
	inbufcount += unconverted;
    }
    else
	unconverted = 0;

    len = 0;	/* to avoid gcc warning */
    for (try = 0; try < 100; ++try)
    {
	size_t readlen = (size_t)((INBUFLEN - inbufcount)
			    / input_conv.vc_factor);
#  ifdef VMS
	len = vms_read((char *)inbuf + inbufcount, readlen);
#  else
	len = read(read_cmd_fd, (char *)inbuf + inbufcount, readlen);
#  endif

	if (len > 0 || got_int)
	    break;
	/*
	 * If reading stdin results in an error, continue reading stderr.
	 * This helps when using "foo | xargs vim".
	 */
	if (!did_read_something && !isatty(read_cmd_fd) && read_cmd_fd == 0)
	{
	    int m = cur_tmode;

	    /* We probably set the wrong file descriptor to raw mode.  Switch
	     * back to cooked mode, use another descriptor and set the mode to
	     * what it was. */
	    settmode(TMODE_COOK);
#ifdef HAVE_DUP
	    /* Use stderr for stdin, also works for shell commands. */
	    close(0);
	    vim_ignored = dup(2);
#else
	    read_cmd_fd = 2;	/* read from stderr instead of stdin */
#endif
	    settmode(m);
	}
	if (!exit_on_error)
	    return;
    }
# endif
    if (len <= 0 && !got_int)
	read_error_exit();
    if (len > 0)
	did_read_something = TRUE;
    if (got_int)
    {
	/* Interrupted, pretend a CTRL-C was typed. */
	inbuf[0] = 3;
	inbufcount = 1;
    }
    else
    {
	/*
	 * May perform conversion on the input characters.
	 * Include the unconverted rest of the previous call.
	 * If there is an incomplete char at the end it is kept for the next
	 * time, reading more bytes should make conversion possible.
	 * Don't do this in the unlikely event that the input buffer is too
	 * small ("rest" still contains more bytes).
	 */
	if (input_conv.vc_type != CONV_NONE)
	{
	    inbufcount -= unconverted;
	    len = convert_input_safe(inbuf + inbufcount,
				     len + unconverted, INBUFLEN - inbufcount,
				       rest == NULL ? &rest : NULL, &restlen);
	}
	while (len-- > 0)
	{
	    /*
	     * if a CTRL-C was typed, remove it from the buffer and set got_int
	     */
	    if (inbuf[inbufcount] == 3 && ctrl_c_interrupts)
	    {
		/* remove everything typed before the CTRL-C */
		mch_memmove(inbuf, inbuf + inbufcount, (size_t)(len + 1));
		inbufcount = 0;
		got_int = TRUE;
	    }
	    ++inbufcount;
	}
    }
#endif /* UNIX or VMS*/
}
#endif /* defined(UNIX) || defined(FEAT_GUI) || defined(VMS) */

/*
 * Exit because of an input read error.
 */
    void
read_error_exit(void)
{
    if (silent_mode)	/* Normal way to exit for "ex -s" */
	getout(0);
    STRCPY(IObuff, _("Vim: Error reading input, exiting...\n"));
    preserve_exit();
}

#if defined(CURSOR_SHAPE) || defined(PROTO)
/*
 * May update the shape of the cursor.
 */
    void
ui_cursor_shape_forced(int forced)
{
# ifdef FEAT_GUI
    if (gui.in_use)
	gui_update_cursor_later();
    else
# endif
	term_cursor_mode(forced);

# ifdef MCH_CURSOR_SHAPE
    mch_update_cursor();
# endif

# ifdef FEAT_CONCEAL
    conceal_check_cursor_line();
# endif
}

    void
ui_cursor_shape(void)
{
    ui_cursor_shape_forced(FALSE);
}
#endif

/*
 * Check bounds for column number
 */
    int
check_col(int col)
{
    if (col < 0)
	return 0;
    if (col >= (int)screen_Columns)
	return (int)screen_Columns - 1;
    return col;
}

/*
 * Check bounds for row number
 */
    int
check_row(int row)
{
    if (row < 0)
	return 0;
    if (row >= (int)screen_Rows)
	return (int)screen_Rows - 1;
    return row;
}

/*
 * Stuff for the X clipboard.  Shared between VMS and Unix.
 */

#if defined(FEAT_XCLIPBOARD) || defined(FEAT_GUI_X11) || defined(PROTO)
# include <X11/Xatom.h>
# include <X11/Intrinsic.h>

/*
 * Open the application context (if it hasn't been opened yet).
 * Used for Motif and Athena GUI and the xterm clipboard.
 */
    void
open_app_context(void)
{
    if (app_context == NULL)
    {
	XtToolkitInitialize();
	app_context = XtCreateApplicationContext();
    }
}

static Atom	vim_atom;	/* Vim's own special selection format */
static Atom	vimenc_atom;	/* Vim's extended selection format */
static Atom	utf8_atom;
static Atom	compound_text_atom;
static Atom	text_atom;
static Atom	targets_atom;
static Atom	timestamp_atom;	/* Used to get a timestamp */

    void
x11_setup_atoms(Display *dpy)
{
    vim_atom	       = XInternAtom(dpy, VIM_ATOM_NAME,   False);
    vimenc_atom	       = XInternAtom(dpy, VIMENC_ATOM_NAME,False);
    utf8_atom	       = XInternAtom(dpy, "UTF8_STRING",   False);
    compound_text_atom = XInternAtom(dpy, "COMPOUND_TEXT", False);
    text_atom	       = XInternAtom(dpy, "TEXT",	   False);
    targets_atom       = XInternAtom(dpy, "TARGETS",	   False);
    clip_star.sel_atom = XA_PRIMARY;
    clip_plus.sel_atom = XInternAtom(dpy, "CLIPBOARD",	   False);
    timestamp_atom     = XInternAtom(dpy, "TIMESTAMP",	   False);
}

/*
 * X Selection stuff, for cutting and pasting text to other windows.
 */

static Boolean	clip_x11_convert_selection_cb(Widget w, Atom *sel_atom, Atom *target, Atom *type, XtPointer *value, long_u *length, int *format);
static void clip_x11_lose_ownership_cb(Widget w, Atom *sel_atom);
static void clip_x11_notify_cb(Widget w, Atom *sel_atom, Atom *target);

/*
 * Property callback to get a timestamp for XtOwnSelection.
 */
    static void
clip_x11_timestamp_cb(
    Widget	w,
    XtPointer	n UNUSED,
    XEvent	*event,
    Boolean	*cont UNUSED)
{
    Atom	    actual_type;
    int		    format;
    unsigned  long  nitems, bytes_after;
    unsigned char   *prop=NULL;
    XPropertyEvent  *xproperty=&event->xproperty;

    /* Must be a property notify, state can't be Delete (True), has to be
     * one of the supported selection types. */
    if (event->type != PropertyNotify || xproperty->state
	    || (xproperty->atom != clip_star.sel_atom
				    && xproperty->atom != clip_plus.sel_atom))
	return;

    if (XGetWindowProperty(xproperty->display, xproperty->window,
	  xproperty->atom, 0, 0, False, timestamp_atom, &actual_type, &format,
						&nitems, &bytes_after, &prop))
	return;

    if (prop)
	XFree(prop);

    /* Make sure the property type is "TIMESTAMP" and it's 32 bits. */
    if (actual_type != timestamp_atom || format != 32)
	return;

    /* Get the selection, using the event timestamp. */
    if (XtOwnSelection(w, xproperty->atom, xproperty->time,
	    clip_x11_convert_selection_cb, clip_x11_lose_ownership_cb,
	    clip_x11_notify_cb) == OK)
    {
	/* Set the "owned" flag now, there may have been a call to
	 * lose_ownership_cb in between. */
	if (xproperty->atom == clip_plus.sel_atom)
	    clip_plus.owned = TRUE;
	else
	    clip_star.owned = TRUE;
    }
}

    void
x11_setup_selection(Widget w)
{
    XtAddEventHandler(w, PropertyChangeMask, False,
	    /*(XtEventHandler)*/clip_x11_timestamp_cb, (XtPointer)NULL);
}

    static void
clip_x11_request_selection_cb(
    Widget	w UNUSED,
    XtPointer	success,
    Atom	*sel_atom,
    Atom	*type,
    XtPointer	value,
    long_u	*length,
    int		*format)
{
    int		motion_type = MAUTO;
    long_u	len;
    char_u	*p;
    char	**text_list = NULL;
    VimClipboard	*cbd;
    char_u	*tmpbuf = NULL;

    if (*sel_atom == clip_plus.sel_atom)
	cbd = &clip_plus;
    else
	cbd = &clip_star;

    if (value == NULL || *length == 0)
    {
	clip_free_selection(cbd);	/* nothing received, clear register */
	*(int *)success = FALSE;
	return;
    }
    p = (char_u *)value;
    len = *length;
    if (*type == vim_atom)
    {
	motion_type = *p++;
	len--;
    }

    else if (*type == vimenc_atom)
    {
	char_u		*enc;
	vimconv_T	conv;
	int		convlen;

	motion_type = *p++;
	--len;

	enc = p;
	p += STRLEN(p) + 1;
	len -= p - enc;

	/* If the encoding of the text is different from 'encoding', attempt
	 * converting it. */
	conv.vc_type = CONV_NONE;
	convert_setup(&conv, enc, p_enc);
	if (conv.vc_type != CONV_NONE)
	{
	    convlen = len;	/* Need to use an int here. */
	    tmpbuf = string_convert(&conv, p, &convlen);
	    len = convlen;
	    if (tmpbuf != NULL)
		p = tmpbuf;
	    convert_setup(&conv, NULL, NULL);
	}
    }

    else if (*type == compound_text_atom
	    || *type == utf8_atom
	    || (enc_dbcs != 0 && *type == text_atom))
    {
	XTextProperty	text_prop;
	int		n_text = 0;
	int		status;

	text_prop.value = (unsigned char *)value;
	text_prop.encoding = *type;
	text_prop.format = *format;
	text_prop.nitems = len;
#if defined(X_HAVE_UTF8_STRING)
	if (*type == utf8_atom)
	    status = Xutf8TextPropertyToTextList(X_DISPLAY, &text_prop,
							 &text_list, &n_text);
	else
#endif
	    status = XmbTextPropertyToTextList(X_DISPLAY, &text_prop,
							 &text_list, &n_text);
	if (status != Success || n_text < 1)
	{
	    *(int *)success = FALSE;
	    return;
	}
	p = (char_u *)text_list[0];
	len = STRLEN(p);
    }
    clip_yank_selection(motion_type, p, (long)len, cbd);

    if (text_list != NULL)
	XFreeStringList(text_list);
    vim_free(tmpbuf);
    XtFree((char *)value);
    *(int *)success = TRUE;
}

    void
clip_x11_request_selection(
    Widget	myShell,
    Display	*dpy,
    VimClipboard	*cbd)
{
    XEvent	event;
    Atom	type;
    static int	success;
    int		i;
    time_t	start_time;
    int		timed_out = FALSE;

    for (i = 0; i < 6; i++)
    {
	switch (i)
	{
	    case 0:  type = vimenc_atom;	break;
	    case 1:  type = vim_atom;		break;
	    case 2:  type = utf8_atom;		break;
	    case 3:  type = compound_text_atom; break;
	    case 4:  type = text_atom;		break;
	    default: type = XA_STRING;
	}
	if (type == utf8_atom
# if defined(X_HAVE_UTF8_STRING)
		&& !enc_utf8
# endif
		)
	    /* Only request utf-8 when 'encoding' is utf8 and
	     * Xutf8TextPropertyToTextList is available. */
	    continue;
	success = MAYBE;
	XtGetSelectionValue(myShell, cbd->sel_atom, type,
	    clip_x11_request_selection_cb, (XtPointer)&success, CurrentTime);

	/* Make sure the request for the selection goes out before waiting for
	 * a response. */
	XFlush(dpy);

	/*
	 * Wait for result of selection request, otherwise if we type more
	 * characters, then they will appear before the one that requested the
	 * paste!  Don't worry, we will catch up with any other events later.
	 */
	start_time = time(NULL);
	while (success == MAYBE)
	{
	    if (XCheckTypedEvent(dpy, PropertyNotify, &event)
		    || XCheckTypedEvent(dpy, SelectionNotify, &event)
		    || XCheckTypedEvent(dpy, SelectionRequest, &event))
	    {
		/* This is where clip_x11_request_selection_cb() should be
		 * called.  It may actually happen a bit later, so we loop
		 * until "success" changes.
		 * We may get a SelectionRequest here and if we don't handle
		 * it we hang.  KDE klipper does this, for example.
		 * We need to handle a PropertyNotify for large selections. */
		XtDispatchEvent(&event);
		continue;
	    }

	    /* Time out after 2 to 3 seconds to avoid that we hang when the
	     * other process doesn't respond.  Note that the SelectionNotify
	     * event may still come later when the selection owner comes back
	     * to life and the text gets inserted unexpectedly.  Don't know
	     * why that happens or how to avoid that :-(. */
	    if (time(NULL) > start_time + 2)
	    {
		timed_out = TRUE;
		break;
	    }

	    /* Do we need this?  Probably not. */
	    XSync(dpy, False);

	    /* Wait for 1 msec to avoid that we eat up all CPU time. */
	    ui_delay(1L, TRUE);
	}

	if (success == TRUE)
	    return;

	/* don't do a retry with another type after timing out, otherwise we
	 * hang for 15 seconds. */
	if (timed_out)
	    break;
    }

    /* Final fallback position - use the X CUT_BUFFER0 store */
    yank_cut_buffer0(dpy, cbd);
}

    static Boolean
clip_x11_convert_selection_cb(
    Widget	w UNUSED,
    Atom	*sel_atom,
    Atom	*target,
    Atom	*type,
    XtPointer	*value,
    long_u	*length,
    int		*format)
{
    static char_u   *save_result = NULL;
    static long_u   save_length = 0;
    char_u	    *string;
    int		    motion_type;
    VimClipboard    *cbd;
    int		    i;

    if (*sel_atom == clip_plus.sel_atom)
	cbd = &clip_plus;
    else
	cbd = &clip_star;

    if (!cbd->owned)
	return False;	    /* Shouldn't ever happen */

    /* requestor wants to know what target types we support */
    if (*target == targets_atom)
    {
	static Atom array[7];

	*value = (XtPointer)array;
	i = 0;
	array[i++] = targets_atom;
	array[i++] = vimenc_atom;
	array[i++] = vim_atom;
	if (enc_utf8)
	    array[i++] = utf8_atom;
	array[i++] = XA_STRING;
	array[i++] = text_atom;
	array[i++] = compound_text_atom;

	*type = XA_ATOM;
	/* This used to be: *format = sizeof(Atom) * 8; but that caused
	 * crashes on 64 bit machines. (Peter Derr) */
	*format = 32;
	*length = i;
	return True;
    }

    if (       *target != XA_STRING
	    && *target != vimenc_atom
	    && (*target != utf8_atom || !enc_utf8)
	    && *target != vim_atom
	    && *target != text_atom
	    && *target != compound_text_atom)
	return False;

    clip_get_selection(cbd);
    motion_type = clip_convert_selection(&string, length, cbd);
    if (motion_type < 0)
	return False;

    /* For our own format, the first byte contains the motion type */
    if (*target == vim_atom)
	(*length)++;

    /* Our own format with encoding: motion 'encoding' NUL text */
    if (*target == vimenc_atom)
	*length += STRLEN(p_enc) + 2;

    if (save_length < *length || save_length / 2 >= *length)
	*value = XtRealloc((char *)save_result, (Cardinal)*length + 1);
    else
	*value = save_result;
    if (*value == NULL)
    {
	vim_free(string);
	return False;
    }
    save_result = (char_u *)*value;
    save_length = *length;

    if (*target == XA_STRING || (*target == utf8_atom && enc_utf8))
    {
	mch_memmove(save_result, string, (size_t)(*length));
	*type = *target;
    }
    else if (*target == compound_text_atom || *target == text_atom)
    {
	XTextProperty	text_prop;
	char		*string_nt = (char *)save_result;
	int		conv_result;

	/* create NUL terminated string which XmbTextListToTextProperty wants */
	mch_memmove(string_nt, string, (size_t)*length);
	string_nt[*length] = NUL;
	conv_result = XmbTextListToTextProperty(X_DISPLAY, (char **)&string_nt,
					   1, XCompoundTextStyle, &text_prop);
	if (conv_result != Success)
	{
	    vim_free(string);
	    return False;
	}
	*value = (XtPointer)(text_prop.value);	/*    from plain text */
	*length = text_prop.nitems;
	*type = compound_text_atom;
	XtFree((char *)save_result);
	save_result = (char_u *)*value;
	save_length = *length;
    }
    else if (*target == vimenc_atom)
    {
	int l = STRLEN(p_enc);

	save_result[0] = motion_type;
	STRCPY(save_result + 1, p_enc);
	mch_memmove(save_result + l + 2, string, (size_t)(*length - l - 2));
	*type = vimenc_atom;
    }
    else
    {
	save_result[0] = motion_type;
	mch_memmove(save_result + 1, string, (size_t)(*length - 1));
	*type = vim_atom;
    }
    *format = 8;	    /* 8 bits per char */
    vim_free(string);
    return True;
}

    static void
clip_x11_lose_ownership_cb(Widget w UNUSED, Atom *sel_atom)
{
    if (*sel_atom == clip_plus.sel_atom)
	clip_lose_selection(&clip_plus);
    else
	clip_lose_selection(&clip_star);
}

    void
clip_x11_lose_selection(Widget myShell, VimClipboard *cbd)
{
    XtDisownSelection(myShell, cbd->sel_atom,
				XtLastTimestampProcessed(XtDisplay(myShell)));
}

    static void
clip_x11_notify_cb(Widget w UNUSED, Atom *sel_atom UNUSED, Atom *target UNUSED)
{
    /* To prevent automatically freeing the selection value. */
}

    int
clip_x11_own_selection(Widget myShell, VimClipboard *cbd)
{
    /* When using the GUI we have proper timestamps, use the one of the last
     * event.  When in the console we don't get events (the terminal gets
     * them), Get the time by a zero-length append, clip_x11_timestamp_cb will
     * be called with the current timestamp.  */
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	if (XtOwnSelection(myShell, cbd->sel_atom,
	       XtLastTimestampProcessed(XtDisplay(myShell)),
	       clip_x11_convert_selection_cb, clip_x11_lose_ownership_cb,
	       clip_x11_notify_cb) == False)
	    return FAIL;
    }
    else
#endif
    {
	if (!XChangeProperty(XtDisplay(myShell), XtWindow(myShell),
		  cbd->sel_atom, timestamp_atom, 32, PropModeAppend, NULL, 0))
	    return FAIL;
    }
    /* Flush is required in a terminal as nothing else is doing it. */
    XFlush(XtDisplay(myShell));
    return OK;
}

/*
 * Send the current selection to the clipboard.  Do nothing for X because we
 * will fill in the selection only when requested by another app.
 */
    void
clip_x11_set_selection(VimClipboard *cbd UNUSED)
{
}

#if (defined(FEAT_X11) && defined(FEAT_XCLIPBOARD) && defined(USE_SYSTEM)) \
	|| defined(PROTO)
    int
clip_x11_owner_exists(VimClipboard *cbd)
{
    return XGetSelectionOwner(X_DISPLAY, cbd->sel_atom) != None;
}
#endif
#endif

#if defined(FEAT_XCLIPBOARD) || defined(FEAT_GUI_X11) \
    || defined(FEAT_GUI_GTK) || defined(PROTO)
/*
 * Get the contents of the X CUT_BUFFER0 and put it in "cbd".
 */
    void
yank_cut_buffer0(Display *dpy, VimClipboard *cbd)
{
    int		nbytes = 0;
    char_u	*buffer = (char_u *)XFetchBuffer(dpy, &nbytes, 0);

    if (nbytes > 0)
    {
	int  done = FALSE;

	/* CUT_BUFFER0 is supposed to be always latin1.  Convert to 'enc' when
	 * using a multi-byte encoding.  Conversion between two 8-bit
	 * character sets usually fails and the text might actually be in
	 * 'enc' anyway. */
	if (has_mbyte)
	{
	    char_u	*conv_buf;
	    vimconv_T	vc;

	    vc.vc_type = CONV_NONE;
	    if (convert_setup(&vc, (char_u *)"latin1", p_enc) == OK)
	    {
		conv_buf = string_convert(&vc, buffer, &nbytes);
		if (conv_buf != NULL)
		{
		    clip_yank_selection(MCHAR, conv_buf, (long)nbytes, cbd);
		    vim_free(conv_buf);
		    done = TRUE;
		}
		convert_setup(&vc, NULL, NULL);
	    }
	}
	if (!done)  /* use the text without conversion */
	    clip_yank_selection(MCHAR, buffer, (long)nbytes, cbd);
	XFree((void *)buffer);
	if (p_verbose > 0)
	{
	    verbose_enter();
	    verb_msg(_("Used CUT_BUFFER0 instead of empty selection"));
	    verbose_leave();
	}
    }
}
#endif

#if defined(FEAT_MOUSE) || defined(PROTO)

/*
 * Move the cursor to the specified row and column on the screen.
 * Change current window if necessary.	Returns an integer with the
 * CURSOR_MOVED bit set if the cursor has moved or unset otherwise.
 *
 * The MOUSE_FOLD_CLOSE bit is set when clicked on the '-' in a fold column.
 * The MOUSE_FOLD_OPEN bit is set when clicked on the '+' in a fold column.
 *
 * If flags has MOUSE_FOCUS, then the current window will not be changed, and
 * if the mouse is outside the window then the text will scroll, or if the
 * mouse was previously on a status line, then the status line may be dragged.
 *
 * If flags has MOUSE_MAY_VIS, then VIsual mode will be started before the
 * cursor is moved unless the cursor was on a status line.
 * This function returns one of IN_UNKNOWN, IN_BUFFER, IN_STATUS_LINE or
 * IN_SEP_LINE depending on where the cursor was clicked.
 *
 * If flags has MOUSE_MAY_STOP_VIS, then Visual mode will be stopped, unless
 * the mouse is on the status line of the same window.
 *
 * If flags has MOUSE_DID_MOVE, nothing is done if the mouse didn't move since
 * the last call.
 *
 * If flags has MOUSE_SETPOS, nothing is done, only the current position is
 * remembered.
 */
    int
jump_to_mouse(
    int		flags,
    int		*inclusive,	/* used for inclusive operator, can be NULL */
    int		which_button)	/* MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE */
{
    static int	on_status_line = 0;	/* #lines below bottom of window */
    static int	on_sep_line = 0;	/* on separator right of window */
#ifdef FEAT_MENU
    static int  in_winbar = FALSE;
#endif
    static int	prev_row = -1;
    static int	prev_col = -1;
    static win_T *dragwin = NULL;	/* window being dragged */
    static int	did_drag = FALSE;	/* drag was noticed */

    win_T	*wp, *old_curwin;
    pos_T	old_cursor;
    int		count;
    int		first;
    int		row = mouse_row;
    int		col = mouse_col;
#ifdef FEAT_FOLDING
    int		mouse_char;
#endif

    mouse_past_bottom = FALSE;
    mouse_past_eol = FALSE;

    if (flags & MOUSE_RELEASED)
    {
	/* On button release we may change window focus if positioned on a
	 * status line and no dragging happened. */
	if (dragwin != NULL && !did_drag)
	    flags &= ~(MOUSE_FOCUS | MOUSE_DID_MOVE);
	dragwin = NULL;
	did_drag = FALSE;
    }

    if ((flags & MOUSE_DID_MOVE)
	    && prev_row == mouse_row
	    && prev_col == mouse_col)
    {
retnomove:
	/* before moving the cursor for a left click which is NOT in a status
	 * line, stop Visual mode */
	if (on_status_line)
	    return IN_STATUS_LINE;
	if (on_sep_line)
	    return IN_SEP_LINE;
#ifdef FEAT_MENU
	if (in_winbar)
	{
	    /* A quick second click may arrive as a double-click, but we use it
	     * as a second click in the WinBar. */
	    if ((mod_mask & MOD_MASK_MULTI_CLICK) && !(flags & MOUSE_RELEASED))
	    {
		wp = mouse_find_win(&row, &col);
		if (wp == NULL)
		    return IN_UNKNOWN;
		winbar_click(wp, col);
	    }
	    return IN_OTHER_WIN | MOUSE_WINBAR;
	}
#endif
	if (flags & MOUSE_MAY_STOP_VIS)
	{
	    end_visual_mode();
	    redraw_curbuf_later(INVERTED);	/* delete the inversion */
	}
#if defined(FEAT_CMDWIN) && defined(FEAT_CLIPBOARD)
	/* Continue a modeless selection in another window. */
	if (cmdwin_type != 0 && row < curwin->w_winrow)
	    return IN_OTHER_WIN;
#endif
	return IN_BUFFER;
    }

    prev_row = mouse_row;
    prev_col = mouse_col;

    if (flags & MOUSE_SETPOS)
	goto retnomove;				/* ugly goto... */

#ifdef FEAT_FOLDING
    /* Remember the character under the mouse, it might be a '-' or '+' in the
     * fold column. */
    if (row >= 0 && row < Rows && col >= 0 && col <= Columns
						       && ScreenLines != NULL)
	mouse_char = ScreenLines[LineOffset[row] + col];
    else
	mouse_char = ' ';
#endif

    old_curwin = curwin;
    old_cursor = curwin->w_cursor;

    if (!(flags & MOUSE_FOCUS))
    {
	if (row < 0 || col < 0)			/* check if it makes sense */
	    return IN_UNKNOWN;

	/* find the window where the row is in */
	wp = mouse_find_win(&row, &col);
	if (wp == NULL)
	    return IN_UNKNOWN;
	dragwin = NULL;

#ifdef FEAT_MENU
	if (row == -1)
	{
	    /* A click in the window toolbar does not enter another window or
	     * change Visual highlighting. */
	    winbar_click(wp, col);
	    in_winbar = TRUE;
	    return IN_OTHER_WIN | MOUSE_WINBAR;
	}
	in_winbar = FALSE;
#endif

	/*
	 * winpos and height may change in win_enter()!
	 */
	if (row >= wp->w_height)		/* In (or below) status line */
	{
	    on_status_line = row - wp->w_height + 1;
	    dragwin = wp;
	}
	else
	    on_status_line = 0;
	if (col >= wp->w_width)		/* In separator line */
	{
	    on_sep_line = col - wp->w_width + 1;
	    dragwin = wp;
	}
	else
	    on_sep_line = 0;

	/* The rightmost character of the status line might be a vertical
	 * separator character if there is no connecting window to the right. */
	if (on_status_line && on_sep_line)
	{
	    if (stl_connected(wp))
		on_sep_line = 0;
	    else
		on_status_line = 0;
	}

	/* Before jumping to another buffer, or moving the cursor for a left
	 * click, stop Visual mode. */
	if (VIsual_active
		&& (wp->w_buffer != curwin->w_buffer
		    || (!on_status_line && !on_sep_line
#ifdef FEAT_FOLDING
			&& (
# ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? col < wp->w_width - wp->w_p_fdc :
# endif
			    col >= wp->w_p_fdc
# ifdef FEAT_CMDWIN
				  + (cmdwin_type == 0 && wp == curwin ? 0 : 1)
# endif
			    )
#endif
			&& (flags & MOUSE_MAY_STOP_VIS))))
	{
	    end_visual_mode();
	    redraw_curbuf_later(INVERTED);	/* delete the inversion */
	}
#ifdef FEAT_CMDWIN
	if (cmdwin_type != 0 && wp != curwin)
	{
	    /* A click outside the command-line window: Use modeless
	     * selection if possible.  Allow dragging the status lines. */
	    on_sep_line = 0;
# ifdef FEAT_CLIPBOARD
	    if (on_status_line)
		return IN_STATUS_LINE;
	    return IN_OTHER_WIN;
# else
	    row = 0;
	    col += wp->w_wincol;
	    wp = curwin;
# endif
	}
#endif
	/* Only change window focus when not clicking on or dragging the
	 * status line.  Do change focus when releasing the mouse button
	 * (MOUSE_FOCUS was set above if we dragged first). */
	if (dragwin == NULL || (flags & MOUSE_RELEASED))
	    win_enter(wp, TRUE);		/* can make wp invalid! */

	if (curwin != old_curwin)
	{
#ifdef CHECK_DOUBLE_CLICK
	    /* set topline, to be able to check for double click ourselves */
	    set_mouse_topline(curwin);
#endif
#ifdef FEAT_TERMINAL
	    /* when entering a terminal window may change state */
	    term_win_entered();
#endif
	}
	if (on_status_line)			/* In (or below) status line */
	{
	    /* Don't use start_arrow() if we're in the same window */
	    if (curwin == old_curwin)
		return IN_STATUS_LINE;
	    else
		return IN_STATUS_LINE | CURSOR_MOVED;
	}
	if (on_sep_line)			/* In (or below) status line */
	{
	    /* Don't use start_arrow() if we're in the same window */
	    if (curwin == old_curwin)
		return IN_SEP_LINE;
	    else
		return IN_SEP_LINE | CURSOR_MOVED;
	}

	curwin->w_cursor.lnum = curwin->w_topline;
#ifdef FEAT_GUI
	/* remember topline, needed for double click */
	gui_prev_topline = curwin->w_topline;
# ifdef FEAT_DIFF
	gui_prev_topfill = curwin->w_topfill;
# endif
#endif
    }
    else if (on_status_line && which_button == MOUSE_LEFT)
    {
	if (dragwin != NULL)
	{
	    /* Drag the status line */
	    count = row - dragwin->w_winrow - dragwin->w_height + 1
							     - on_status_line;
	    win_drag_status_line(dragwin, count);
	    did_drag |= count;
	}
	return IN_STATUS_LINE;			/* Cursor didn't move */
    }
    else if (on_sep_line && which_button == MOUSE_LEFT)
    {
	if (dragwin != NULL)
	{
	    /* Drag the separator column */
	    count = col - dragwin->w_wincol - dragwin->w_width + 1
								- on_sep_line;
	    win_drag_vsep_line(dragwin, count);
	    did_drag |= count;
	}
	return IN_SEP_LINE;			/* Cursor didn't move */
    }
#ifdef FEAT_MENU
    else if (in_winbar)
    {
	/* After a click on the window toolbar don't start Visual mode. */
	return IN_OTHER_WIN | MOUSE_WINBAR;
    }
#endif
    else /* keep_window_focus must be TRUE */
    {
	/* before moving the cursor for a left click, stop Visual mode */
	if (flags & MOUSE_MAY_STOP_VIS)
	{
	    end_visual_mode();
	    redraw_curbuf_later(INVERTED);	/* delete the inversion */
	}

#if defined(FEAT_CMDWIN) && defined(FEAT_CLIPBOARD)
	/* Continue a modeless selection in another window. */
	if (cmdwin_type != 0 && row < curwin->w_winrow)
	    return IN_OTHER_WIN;
#endif

	row -= W_WINROW(curwin);
	col -= curwin->w_wincol;

	/*
	 * When clicking beyond the end of the window, scroll the screen.
	 * Scroll by however many rows outside the window we are.
	 */
	if (row < 0)
	{
	    count = 0;
	    for (first = TRUE; curwin->w_topline > 1; )
	    {
#ifdef FEAT_DIFF
		if (curwin->w_topfill < diff_check(curwin, curwin->w_topline))
		    ++count;
		else
#endif
		    count += plines(curwin->w_topline - 1);
		if (!first && count > -row)
		    break;
		first = FALSE;
#ifdef FEAT_FOLDING
		(void)hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif
#ifdef FEAT_DIFF
		if (curwin->w_topfill < diff_check(curwin, curwin->w_topline))
		    ++curwin->w_topfill;
		else
#endif
		{
		    --curwin->w_topline;
#ifdef FEAT_DIFF
		    curwin->w_topfill = 0;
#endif
		}
	    }
#ifdef FEAT_DIFF
	    check_topfill(curwin, FALSE);
#endif
	    curwin->w_valid &=
		      ~(VALID_WROW|VALID_CROW|VALID_BOTLINE|VALID_BOTLINE_AP);
	    redraw_later(VALID);
	    row = 0;
	}
	else if (row >= curwin->w_height)
	{
	    count = 0;
	    for (first = TRUE; curwin->w_topline < curbuf->b_ml.ml_line_count; )
	    {
#ifdef FEAT_DIFF
		if (curwin->w_topfill > 0)
		    ++count;
		else
#endif
		    count += plines(curwin->w_topline);
		if (!first && count > row - curwin->w_height + 1)
		    break;
		first = FALSE;
#ifdef FEAT_FOLDING
		if (hasFolding(curwin->w_topline, NULL, &curwin->w_topline)
			&& curwin->w_topline == curbuf->b_ml.ml_line_count)
		    break;
#endif
#ifdef FEAT_DIFF
		if (curwin->w_topfill > 0)
		    --curwin->w_topfill;
		else
#endif
		{
		    ++curwin->w_topline;
#ifdef FEAT_DIFF
		    curwin->w_topfill =
				   diff_check_fill(curwin, curwin->w_topline);
#endif
		}
	    }
#ifdef FEAT_DIFF
	    check_topfill(curwin, FALSE);
#endif
	    redraw_later(VALID);
	    curwin->w_valid &=
		      ~(VALID_WROW|VALID_CROW|VALID_BOTLINE|VALID_BOTLINE_AP);
	    row = curwin->w_height - 1;
	}
	else if (row == 0)
	{
	    /* When dragging the mouse, while the text has been scrolled up as
	     * far as it goes, moving the mouse in the top line should scroll
	     * the text down (done later when recomputing w_topline). */
	    if (mouse_dragging > 0
		    && curwin->w_cursor.lnum
				       == curwin->w_buffer->b_ml.ml_line_count
		    && curwin->w_cursor.lnum == curwin->w_topline)
		curwin->w_valid &= ~(VALID_TOPLINE);
	}
    }

#ifdef FEAT_FOLDING
    /* Check for position outside of the fold column. */
    if (
# ifdef FEAT_RIGHTLEFT
	    curwin->w_p_rl ? col < curwin->w_width - curwin->w_p_fdc :
# endif
	    col >= curwin->w_p_fdc
#  ifdef FEAT_CMDWIN
				+ (cmdwin_type == 0 ? 0 : 1)
#  endif
       )
	mouse_char = ' ';
#endif

    /* compute the position in the buffer line from the posn on the screen */
    if (mouse_comp_pos(curwin, &row, &col, &curwin->w_cursor.lnum))
	mouse_past_bottom = TRUE;

    /* Start Visual mode before coladvance(), for when 'sel' != "old" */
    if ((flags & MOUSE_MAY_VIS) && !VIsual_active)
    {
	check_visual_highlight();
	VIsual = old_cursor;
	VIsual_active = TRUE;
	VIsual_reselect = TRUE;
	/* if 'selectmode' contains "mouse", start Select mode */
	may_start_select('o');
	setmouse();
	if (p_smd && msg_silent == 0)
	    redraw_cmdline = TRUE;	/* show visual mode later */
    }

    curwin->w_curswant = col;
    curwin->w_set_curswant = FALSE;	/* May still have been TRUE */
    if (coladvance(col) == FAIL)	/* Mouse click beyond end of line */
    {
	if (inclusive != NULL)
	    *inclusive = TRUE;
	mouse_past_eol = TRUE;
    }
    else if (inclusive != NULL)
	*inclusive = FALSE;

    count = IN_BUFFER;
    if (curwin != old_curwin || curwin->w_cursor.lnum != old_cursor.lnum
	    || curwin->w_cursor.col != old_cursor.col)
	count |= CURSOR_MOVED;		/* Cursor has moved */

#ifdef FEAT_FOLDING
    if (mouse_char == '+')
	count |= MOUSE_FOLD_OPEN;
    else if (mouse_char != ' ')
	count |= MOUSE_FOLD_CLOSE;
#endif

    return count;
}

/*
 * Compute the position in the buffer line from the posn on the screen in
 * window "win".
 * Returns TRUE if the position is below the last line.
 */
    int
mouse_comp_pos(
    win_T	*win,
    int		*rowp,
    int		*colp,
    linenr_T	*lnump)
{
    int		col = *colp;
    int		row = *rowp;
    linenr_T	lnum;
    int		retval = FALSE;
    int		off;
    int		count;

#ifdef FEAT_RIGHTLEFT
    if (win->w_p_rl)
	col = win->w_width - 1 - col;
#endif

    lnum = win->w_topline;

    while (row > 0)
    {
#ifdef FEAT_DIFF
	/* Don't include filler lines in "count" */
	if (win->w_p_diff
# ifdef FEAT_FOLDING
		&& !hasFoldingWin(win, lnum, NULL, NULL, TRUE, NULL)
# endif
		)
	{
	    if (lnum == win->w_topline)
		row -= win->w_topfill;
	    else
		row -= diff_check_fill(win, lnum);
	    count = plines_win_nofill(win, lnum, TRUE);
	}
	else
#endif
	    count = plines_win(win, lnum, TRUE);
	if (count > row)
	    break;	/* Position is in this buffer line. */
#ifdef FEAT_FOLDING
	(void)hasFoldingWin(win, lnum, NULL, &lnum, TRUE, NULL);
#endif
	if (lnum == win->w_buffer->b_ml.ml_line_count)
	{
	    retval = TRUE;
	    break;		/* past end of file */
	}
	row -= count;
	++lnum;
    }

    if (!retval)
    {
	/* Compute the column without wrapping. */
	off = win_col_off(win) - win_col_off2(win);
	if (col < off)
	    col = off;
	col += row * (win->w_width - off);
	/* add skip column (for long wrapping line) */
	col += win->w_skipcol;
    }

    if (!win->w_p_wrap)
	col += win->w_leftcol;

    /* skip line number and fold column in front of the line */
    col -= win_col_off(win);
    if (col < 0)
    {
#ifdef FEAT_NETBEANS_INTG
	netbeans_gutter_click(lnum);
#endif
	col = 0;
    }

    *colp = col;
    *rowp = row;
    *lnump = lnum;
    return retval;
}

/*
 * Find the window at screen position "*rowp" and "*colp".  The positions are
 * updated to become relative to the top-left of the window.
 * Returns NULL when something is wrong.
 */
    win_T *
mouse_find_win(int *rowp, int *colp UNUSED)
{
    frame_T	*fp;
    win_T	*wp;

    fp = topframe;
    *rowp -= firstwin->w_winrow;
    for (;;)
    {
	if (fp->fr_layout == FR_LEAF)
	    break;
	if (fp->fr_layout == FR_ROW)
	{
	    for (fp = fp->fr_child; fp->fr_next != NULL; fp = fp->fr_next)
	    {
		if (*colp < fp->fr_width)
		    break;
		*colp -= fp->fr_width;
	    }
	}
	else    /* fr_layout == FR_COL */
	{
	    for (fp = fp->fr_child; fp->fr_next != NULL; fp = fp->fr_next)
	    {
		if (*rowp < fp->fr_height)
		    break;
		*rowp -= fp->fr_height;
	    }
	}
    }
    /* When using a timer that closes a window the window might not actually
     * exist. */
    FOR_ALL_WINDOWS(wp)
	if (wp == fp->fr_win)
	{
#ifdef FEAT_MENU
	    *rowp -= wp->w_winbar_height;
#endif
	    return wp;
	}
    return NULL;
}

#if defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_GTK) || defined(FEAT_GUI_MAC) \
	|| defined(FEAT_GUI_ATHENA) || defined(FEAT_GUI_MSWIN) \
	|| defined(FEAT_GUI_PHOTON) || defined(FEAT_TERM_POPUP_MENU) \
	|| defined(PROTO)
/*
 * Translate window coordinates to buffer position without any side effects
 */
    int
get_fpos_of_mouse(pos_T *mpos)
{
    win_T	*wp;
    int		row = mouse_row;
    int		col = mouse_col;

    if (row < 0 || col < 0)		/* check if it makes sense */
	return IN_UNKNOWN;

    /* find the window where the row is in */
    wp = mouse_find_win(&row, &col);
    if (wp == NULL)
	return IN_UNKNOWN;
    /*
     * winpos and height may change in win_enter()!
     */
    if (row >= wp->w_height)	/* In (or below) status line */
	return IN_STATUS_LINE;
    if (col >= wp->w_width)	/* In vertical separator line */
	return IN_SEP_LINE;

    if (wp != curwin)
	return IN_UNKNOWN;

    /* compute the position in the buffer line from the posn on the screen */
    if (mouse_comp_pos(curwin, &row, &col, &mpos->lnum))
	return IN_STATUS_LINE; /* past bottom */

    mpos->col = vcol2col(wp, mpos->lnum, col);

    if (mpos->col > 0)
	--mpos->col;
    mpos->coladd = 0;
    return IN_BUFFER;
}
#endif

#if defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_GTK) || defined(FEAT_GUI_MAC) \
	|| defined(FEAT_GUI_ATHENA) || defined(FEAT_GUI_MSWIN) \
	|| defined(FEAT_GUI_PHOTON) || defined(FEAT_BEVAL) \
	|| defined(FEAT_TERM_POPUP_MENU) || defined(PROTO)
/*
 * Convert a virtual (screen) column to a character column.
 * The first column is one.
 */
    int
vcol2col(win_T *wp, linenr_T lnum, int vcol)
{
    /* try to advance to the specified column */
    int		count = 0;
    char_u	*ptr;
    char_u	*line;

    line = ptr = ml_get_buf(wp->w_buffer, lnum, FALSE);
    while (count < vcol && *ptr != NUL)
    {
	count += win_lbr_chartabsize(wp, line, ptr, count, NULL);
	MB_PTR_ADV(ptr);
    }
    return (int)(ptr - line);
}
#endif

#endif /* FEAT_MOUSE */

#if defined(FEAT_GUI) || defined(MSWIN) || defined(PROTO)
/*
 * Called when focus changed.  Used for the GUI or for systems where this can
 * be done in the console (Win32).
 */
    void
ui_focus_change(
    int		in_focus)	/* TRUE if focus gained. */
{
    static time_t	last_time = (time_t)0;
    int			need_redraw = FALSE;

    /* When activated: Check if any file was modified outside of Vim.
     * Only do this when not done within the last two seconds (could get
     * several events in a row). */
    if (in_focus && last_time + 2 < time(NULL))
    {
	need_redraw = check_timestamps(
# ifdef FEAT_GUI
		gui.in_use
# else
		FALSE
# endif
		);
	last_time = time(NULL);
    }

    /*
     * Fire the focus gained/lost autocommand.
     */
    need_redraw |= apply_autocmds(in_focus ? EVENT_FOCUSGAINED
				: EVENT_FOCUSLOST, NULL, NULL, FALSE, curbuf);

    if (need_redraw)
    {
	/* Something was executed, make sure the cursor is put back where it
	 * belongs. */
	need_wait_return = FALSE;

	if (State & CMDLINE)
	    redrawcmdline();
	else if (State == HITRETURN || State == SETWSIZE || State == ASKMORE
		|| State == EXTERNCMD || State == CONFIRM || exmode_active)
	    repeat_message();
	else if ((State & NORMAL) || (State & INSERT))
	{
	    if (must_redraw != 0)
		update_screen(0);
	    setcursor();
	}
	cursor_on();	    /* redrawing may have switched it off */
	out_flush_cursor(FALSE, TRUE);
# ifdef FEAT_GUI
	if (gui.in_use)
	    gui_update_scrollbars(FALSE);
# endif
    }
#ifdef FEAT_TITLE
    /* File may have been changed from 'readonly' to 'noreadonly' */
    if (need_maketitle)
	maketitle();
#endif
}
#endif

#if defined(HAVE_INPUT_METHOD) || defined(PROTO)
/*
 * Save current Input Method status to specified place.
 */
    void
im_save_status(long *psave)
{
    /* Don't save when 'imdisable' is set or "xic" is NULL, IM is always
     * disabled then (but might start later).
     * Also don't save when inside a mapping, vgetc_im_active has not been set
     * then.
     * And don't save when the keys were stuffed (e.g., for a "." command).
     * And don't save when the GUI is running but our window doesn't have
     * input focus (e.g., when a find dialog is open). */
    if (!p_imdisable && KeyTyped && !KeyStuffed
# ifdef FEAT_XIM
	    && xic != NULL
# endif
# ifdef FEAT_GUI
	    && (!gui.in_use || gui.in_focus)
# endif
	)
    {
	/* Do save when IM is on, or IM is off and saved status is on. */
	if (vgetc_im_active)
	    *psave = B_IMODE_IM;
	else if (*psave == B_IMODE_IM)
	    *psave = B_IMODE_NONE;
    }
}
#endif
