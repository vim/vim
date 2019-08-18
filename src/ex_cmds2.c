/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * ex_cmds2.c: some more functions for command line commands
 */

#include "vim.h"
#include "version.h"

static void	cmd_source(char_u *fname, exarg_T *eap);

#if defined(FEAT_EVAL) || defined(PROTO)
# if defined(FEAT_TIMERS) || defined(PROTO)
static timer_T	*first_timer = NULL;
static long	last_timer_id = 0;

/*
 * Return time left until "due".  Negative if past "due".
 */
    long
proftime_time_left(proftime_T *due, proftime_T *now)
{
#  ifdef MSWIN
    LARGE_INTEGER fr;

    if (now->QuadPart > due->QuadPart)
	return 0;
    QueryPerformanceFrequency(&fr);
    return (long)(((double)(due->QuadPart - now->QuadPart)
		   / (double)fr.QuadPart) * 1000);
#  else
    if (now->tv_sec > due->tv_sec)
	return 0;
    return (due->tv_sec - now->tv_sec) * 1000
	+ (due->tv_usec - now->tv_usec) / 1000;
#  endif
}

/*
 * Insert a timer in the list of timers.
 */
    static void
insert_timer(timer_T *timer)
{
    timer->tr_next = first_timer;
    timer->tr_prev = NULL;
    if (first_timer != NULL)
	first_timer->tr_prev = timer;
    first_timer = timer;
    did_add_timer = TRUE;
}

/*
 * Take a timer out of the list of timers.
 */
    static void
remove_timer(timer_T *timer)
{
    if (timer->tr_prev == NULL)
	first_timer = timer->tr_next;
    else
	timer->tr_prev->tr_next = timer->tr_next;
    if (timer->tr_next != NULL)
	timer->tr_next->tr_prev = timer->tr_prev;
}

    static void
free_timer(timer_T *timer)
{
    free_callback(&timer->tr_callback);
    vim_free(timer);
}

/*
 * Create a timer and return it.  NULL if out of memory.
 * Caller should set the callback.
 */
    timer_T *
create_timer(long msec, int repeat)
{
    timer_T	*timer = ALLOC_CLEAR_ONE(timer_T);
    long	prev_id = last_timer_id;

    if (timer == NULL)
	return NULL;
    if (++last_timer_id <= prev_id)
	/* Overflow!  Might cause duplicates... */
	last_timer_id = 0;
    timer->tr_id = last_timer_id;
    insert_timer(timer);
    if (repeat != 0)
	timer->tr_repeat = repeat - 1;
    timer->tr_interval = msec;

    profile_setlimit(msec, &timer->tr_due);
    return timer;
}

/*
 * Invoke the callback of "timer".
 */
    static void
timer_callback(timer_T *timer)
{
    typval_T	rettv;
    typval_T	argv[2];

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = (varnumber_T)timer->tr_id;
    argv[1].v_type = VAR_UNKNOWN;

    call_callback(&timer->tr_callback, -1, &rettv, 1, argv);
    clear_tv(&rettv);
}

/*
 * Call timers that are due.
 * Return the time in msec until the next timer is due.
 * Returns -1 if there are no pending timers.
 */
    long
check_due_timer(void)
{
    timer_T	*timer;
    timer_T	*timer_next;
    long	this_due;
    long	next_due = -1;
    proftime_T	now;
    int		did_one = FALSE;
    int		need_update_screen = FALSE;
    long	current_id = last_timer_id;

    /* Don't run any timers while exiting or dealing with an error. */
    if (exiting || aborting())
	return next_due;

    profile_start(&now);
    for (timer = first_timer; timer != NULL && !got_int; timer = timer_next)
    {
	timer_next = timer->tr_next;

	if (timer->tr_id == -1 || timer->tr_firing || timer->tr_paused)
	    continue;
	this_due = proftime_time_left(&timer->tr_due, &now);
	if (this_due <= 1)
	{
	    /* Save and restore a lot of flags, because the timer fires while
	     * waiting for a character, which might be halfway a command. */
	    int save_timer_busy = timer_busy;
	    int save_vgetc_busy = vgetc_busy;
	    int save_did_emsg = did_emsg;
	    int save_called_emsg = called_emsg;
	    int save_must_redraw = must_redraw;
	    int save_trylevel = trylevel;
	    int save_did_throw = did_throw;
	    int save_ex_pressedreturn = get_pressedreturn();
	    int save_may_garbage_collect = may_garbage_collect;
	    except_T *save_current_exception = current_exception;
	    vimvars_save_T vvsave;

	    /* Create a scope for running the timer callback, ignoring most of
	     * the current scope, such as being inside a try/catch. */
	    timer_busy = timer_busy > 0 || vgetc_busy > 0;
	    vgetc_busy = 0;
	    called_emsg = FALSE;
	    did_emsg = FALSE;
	    did_uncaught_emsg = FALSE;
	    must_redraw = 0;
	    trylevel = 0;
	    did_throw = FALSE;
	    current_exception = NULL;
	    may_garbage_collect = FALSE;
	    save_vimvars(&vvsave);

	    timer->tr_firing = TRUE;
	    timer_callback(timer);
	    timer->tr_firing = FALSE;

	    timer_next = timer->tr_next;
	    did_one = TRUE;
	    timer_busy = save_timer_busy;
	    vgetc_busy = save_vgetc_busy;
	    if (did_uncaught_emsg)
		++timer->tr_emsg_count;
	    did_emsg = save_did_emsg;
	    called_emsg = save_called_emsg;
	    trylevel = save_trylevel;
	    did_throw = save_did_throw;
	    current_exception = save_current_exception;
	    restore_vimvars(&vvsave);
	    if (must_redraw != 0)
		need_update_screen = TRUE;
	    must_redraw = must_redraw > save_must_redraw
					      ? must_redraw : save_must_redraw;
	    set_pressedreturn(save_ex_pressedreturn);
	    may_garbage_collect = save_may_garbage_collect;

	    /* Only fire the timer again if it repeats and stop_timer() wasn't
	     * called while inside the callback (tr_id == -1). */
	    if (timer->tr_repeat != 0 && timer->tr_id != -1
		    && timer->tr_emsg_count < 3)
	    {
		profile_setlimit(timer->tr_interval, &timer->tr_due);
		this_due = proftime_time_left(&timer->tr_due, &now);
		if (this_due < 1)
		    this_due = 1;
		if (timer->tr_repeat > 0)
		    --timer->tr_repeat;
	    }
	    else
	    {
		this_due = -1;
		remove_timer(timer);
		free_timer(timer);
	    }
	}
	if (this_due > 0 && (next_due == -1 || next_due > this_due))
	    next_due = this_due;
    }

    if (did_one)
	redraw_after_callback(need_update_screen);

#ifdef FEAT_BEVAL_TERM
    if (bevalexpr_due_set)
    {
	this_due = proftime_time_left(&bevalexpr_due, &now);
	if (this_due <= 1)
	{
	    bevalexpr_due_set = FALSE;
	    if (balloonEval == NULL)
	    {
		balloonEval = ALLOC_CLEAR_ONE(BalloonEval);
		balloonEvalForTerm = TRUE;
	    }
	    if (balloonEval != NULL)
	    {
		general_beval_cb(balloonEval, 0);
		setcursor();
		out_flush();
	    }
	}
	else if (next_due == -1 || next_due > this_due)
	    next_due = this_due;
    }
#endif
#ifdef FEAT_TERMINAL
    /* Some terminal windows may need their buffer updated. */
    next_due = term_check_timers(next_due, &now);
#endif

    return current_id != last_timer_id ? 1 : next_due;
}

/*
 * Find a timer by ID.  Returns NULL if not found;
 */
    timer_T *
find_timer(long id)
{
    timer_T *timer;

    if (id >= 0)
    {
	for (timer = first_timer; timer != NULL; timer = timer->tr_next)
	    if (timer->tr_id == id)
		return timer;
    }
    return NULL;
}


/*
 * Stop a timer and delete it.
 */
    void
stop_timer(timer_T *timer)
{
    if (timer->tr_firing)
	/* Free the timer after the callback returns. */
	timer->tr_id = -1;
    else
    {
	remove_timer(timer);
	free_timer(timer);
    }
}

    void
stop_all_timers(void)
{
    timer_T *timer;
    timer_T *timer_next;

    for (timer = first_timer; timer != NULL; timer = timer_next)
    {
	timer_next = timer->tr_next;
	stop_timer(timer);
    }
}

    void
add_timer_info(typval_T *rettv, timer_T *timer)
{
    list_T	*list = rettv->vval.v_list;
    dict_T	*dict = dict_alloc();
    dictitem_T	*di;
    long	remaining;
    proftime_T	now;

    if (dict == NULL)
	return;
    list_append_dict(list, dict);

    dict_add_number(dict, "id", timer->tr_id);
    dict_add_number(dict, "time", (long)timer->tr_interval);

    profile_start(&now);
    remaining = proftime_time_left(&timer->tr_due, &now);
    dict_add_number(dict, "remaining", (long)remaining);

    dict_add_number(dict, "repeat",
		    (long)(timer->tr_repeat < 0 ? -1 : timer->tr_repeat + 1));
    dict_add_number(dict, "paused", (long)(timer->tr_paused));

    di = dictitem_alloc((char_u *)"callback");
    if (di != NULL)
    {
	if (dict_add(dict, di) == FAIL)
	    vim_free(di);
	else
	    put_callback(&timer->tr_callback, &di->di_tv);
    }
}

    void
add_timer_info_all(typval_T *rettv)
{
    timer_T *timer;

    for (timer = first_timer; timer != NULL; timer = timer->tr_next)
	if (timer->tr_id != -1)
	    add_timer_info(rettv, timer);
}

/*
 * Mark references in partials of timers.
 */
    int
set_ref_in_timer(int copyID)
{
    int		abort = FALSE;
    timer_T	*timer;
    typval_T	tv;

    for (timer = first_timer; !abort && timer != NULL; timer = timer->tr_next)
    {
	if (timer->tr_callback.cb_partial != NULL)
	{
	    tv.v_type = VAR_PARTIAL;
	    tv.vval.v_partial = timer->tr_callback.cb_partial;
	}
	else
	{
	    tv.v_type = VAR_FUNC;
	    tv.vval.v_string = timer->tr_callback.cb_name;
	}
	abort = abort || set_ref_in_item(&tv, copyID, NULL, NULL);
    }
    return abort;
}

#  if defined(EXITFREE) || defined(PROTO)
    void
timer_free_all()
{
    timer_T *timer;

    while (first_timer != NULL)
    {
	timer = first_timer;
	remove_timer(timer);
	free_timer(timer);
    }
}
#  endif
# endif

#endif

/*
 * If 'autowrite' option set, try to write the file.
 * Careful: autocommands may make "buf" invalid!
 *
 * return FAIL for failure, OK otherwise
 */
    int
autowrite(buf_T *buf, int forceit)
{
    int		r;
    bufref_T	bufref;

    if (!(p_aw || p_awa) || !p_write
#ifdef FEAT_QUICKFIX
	    /* never autowrite a "nofile" or "nowrite" buffer */
	    || bt_dontwrite(buf)
#endif
	    || (!forceit && buf->b_p_ro) || buf->b_ffname == NULL)
	return FAIL;
    set_bufref(&bufref, buf);
    r = buf_write_all(buf, forceit);

    /* Writing may succeed but the buffer still changed, e.g., when there is a
     * conversion error.  We do want to return FAIL then. */
    if (bufref_valid(&bufref) && bufIsChanged(buf))
	r = FAIL;
    return r;
}

/*
 * Flush all buffers, except the ones that are readonly or are never written.
 */
    void
autowrite_all(void)
{
    buf_T	*buf;

    if (!(p_aw || p_awa) || !p_write)
	return;
    FOR_ALL_BUFFERS(buf)
	if (bufIsChanged(buf) && !buf->b_p_ro && !bt_dontwrite(buf))
	{
	    bufref_T	bufref;

	    set_bufref(&bufref, buf);

	    (void)buf_write_all(buf, FALSE);

	    /* an autocommand may have deleted the buffer */
	    if (!bufref_valid(&bufref))
		buf = firstbuf;
	}
}

/*
 * Return TRUE if buffer was changed and cannot be abandoned.
 * For flags use the CCGD_ values.
 */
    int
check_changed(buf_T *buf, int flags)
{
    int		forceit = (flags & CCGD_FORCEIT);
    bufref_T	bufref;

    set_bufref(&bufref, buf);

    if (       !forceit
	    && bufIsChanged(buf)
	    && ((flags & CCGD_MULTWIN) || buf->b_nwindows <= 1)
	    && (!(flags & CCGD_AW) || autowrite(buf, forceit) == FAIL))
    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	if ((p_confirm || cmdmod.confirm) && p_write)
	{
	    buf_T	*buf2;
	    int		count = 0;

	    if (flags & CCGD_ALLBUF)
		FOR_ALL_BUFFERS(buf2)
		    if (bufIsChanged(buf2)
				     && (buf2->b_ffname != NULL
# ifdef FEAT_BROWSE
					 || cmdmod.browse
# endif
					))
			++count;
	    if (!bufref_valid(&bufref))
		/* Autocommand deleted buffer, oops!  It's not changed now. */
		return FALSE;

	    dialog_changed(buf, count > 1);

	    if (!bufref_valid(&bufref))
		/* Autocommand deleted buffer, oops!  It's not changed now. */
		return FALSE;
	    return bufIsChanged(buf);
	}
#endif
	if (flags & CCGD_EXCMD)
	    no_write_message();
	else
	    no_write_message_nobang(curbuf);
	return TRUE;
    }
    return FALSE;
}

#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG) || defined(PROTO)

#if defined(FEAT_BROWSE) || defined(PROTO)
/*
 * When wanting to write a file without a file name, ask the user for a name.
 */
    void
browse_save_fname(buf_T *buf)
{
    if (buf->b_fname == NULL)
    {
	char_u *fname;

	fname = do_browse(BROWSE_SAVE, (char_u *)_("Save As"),
						 NULL, NULL, NULL, NULL, buf);
	if (fname != NULL)
	{
	    if (setfname(buf, fname, NULL, TRUE) == OK)
		buf->b_flags |= BF_NOTEDITED;
	    vim_free(fname);
	}
    }
}
#endif

/*
 * Ask the user what to do when abandoning a changed buffer.
 * Must check 'write' option first!
 */
    void
dialog_changed(
    buf_T	*buf,
    int		checkall)	/* may abandon all changed buffers */
{
    char_u	buff[DIALOG_MSG_SIZE];
    int		ret;
    buf_T	*buf2;
    exarg_T     ea;

    dialog_msg(buff, _("Save changes to \"%s\"?"), buf->b_fname);
    if (checkall)
	ret = vim_dialog_yesnoallcancel(VIM_QUESTION, NULL, buff, 1);
    else
	ret = vim_dialog_yesnocancel(VIM_QUESTION, NULL, buff, 1);

    // Init ea pseudo-structure, this is needed for the check_overwrite()
    // function.
    vim_memset(&ea, 0, sizeof(ea));

    if (ret == VIM_YES)
    {
#ifdef FEAT_BROWSE
	/* May get file name, when there is none */
	browse_save_fname(buf);
#endif
	if (buf->b_fname != NULL && check_overwrite(&ea, buf,
				    buf->b_fname, buf->b_ffname, FALSE) == OK)
	    /* didn't hit Cancel */
	    (void)buf_write_all(buf, FALSE);
    }
    else if (ret == VIM_NO)
    {
	unchanged(buf, TRUE, FALSE);
    }
    else if (ret == VIM_ALL)
    {
	/*
	 * Write all modified files that can be written.
	 * Skip readonly buffers, these need to be confirmed
	 * individually.
	 */
	FOR_ALL_BUFFERS(buf2)
	{
	    if (bufIsChanged(buf2)
		    && (buf2->b_ffname != NULL
#ifdef FEAT_BROWSE
			|| cmdmod.browse
#endif
			)
		    && !buf2->b_p_ro)
	    {
		bufref_T bufref;

		set_bufref(&bufref, buf2);
#ifdef FEAT_BROWSE
		/* May get file name, when there is none */
		browse_save_fname(buf2);
#endif
		if (buf2->b_fname != NULL && check_overwrite(&ea, buf2,
				  buf2->b_fname, buf2->b_ffname, FALSE) == OK)
		    /* didn't hit Cancel */
		    (void)buf_write_all(buf2, FALSE);

		/* an autocommand may have deleted the buffer */
		if (!bufref_valid(&bufref))
		    buf2 = firstbuf;
	    }
	}
    }
    else if (ret == VIM_DISCARDALL)
    {
	/*
	 * mark all buffers as unchanged
	 */
	FOR_ALL_BUFFERS(buf2)
	    unchanged(buf2, TRUE, FALSE);
    }
}
#endif

/*
 * Return TRUE if the buffer "buf" can be abandoned, either by making it
 * hidden, autowriting it or unloading it.
 */
    int
can_abandon(buf_T *buf, int forceit)
{
    return (	   buf_hide(buf)
		|| !bufIsChanged(buf)
		|| buf->b_nwindows > 1
		|| autowrite(buf, forceit) == OK
		|| forceit);
}

/*
 * Add a buffer number to "bufnrs", unless it's already there.
 */
    static void
add_bufnum(int *bufnrs, int *bufnump, int nr)
{
    int i;

    for (i = 0; i < *bufnump; ++i)
	if (bufnrs[i] == nr)
	    return;
    bufnrs[*bufnump] = nr;
    *bufnump = *bufnump + 1;
}

/*
 * Return TRUE if any buffer was changed and cannot be abandoned.
 * That changed buffer becomes the current buffer.
 * When "unload" is TRUE the current buffer is unloaded instead of making it
 * hidden.  This is used for ":q!".
 */
    int
check_changed_any(
    int		hidden,		/* Only check hidden buffers */
    int		unload)
{
    int		ret = FALSE;
    buf_T	*buf;
    int		save;
    int		i;
    int		bufnum = 0;
    int		bufcount = 0;
    int		*bufnrs;
    tabpage_T   *tp;
    win_T	*wp;

    /* Make a list of all buffers, with the most important ones first. */
    FOR_ALL_BUFFERS(buf)
	++bufcount;

    if (bufcount == 0)
	return FALSE;

    bufnrs = ALLOC_MULT(int, bufcount);
    if (bufnrs == NULL)
	return FALSE;

    /* curbuf */
    bufnrs[bufnum++] = curbuf->b_fnum;

    /* buffers in current tab */
    FOR_ALL_WINDOWS(wp)
	if (wp->w_buffer != curbuf)
	    add_bufnum(bufnrs, &bufnum, wp->w_buffer->b_fnum);

    /* buffers in other tabs */
    FOR_ALL_TABPAGES(tp)
	if (tp != curtab)
	    for (wp = tp->tp_firstwin; wp != NULL; wp = wp->w_next)
		add_bufnum(bufnrs, &bufnum, wp->w_buffer->b_fnum);

    /* any other buffer */
    FOR_ALL_BUFFERS(buf)
	add_bufnum(bufnrs, &bufnum, buf->b_fnum);

    for (i = 0; i < bufnum; ++i)
    {
	buf = buflist_findnr(bufnrs[i]);
	if (buf == NULL)
	    continue;
	if ((!hidden || buf->b_nwindows == 0) && bufIsChanged(buf))
	{
	    bufref_T bufref;

	    set_bufref(&bufref, buf);
#ifdef FEAT_TERMINAL
	    if (term_job_running(buf->b_term))
	    {
		if (term_try_stop_job(buf) == FAIL)
		    break;
	    }
	    else
#endif
	    /* Try auto-writing the buffer.  If this fails but the buffer no
	     * longer exists it's not changed, that's OK. */
	    if (check_changed(buf, (p_awa ? CCGD_AW : 0)
				 | CCGD_MULTWIN
				 | CCGD_ALLBUF) && bufref_valid(&bufref))
		break;	    /* didn't save - still changes */
	}
    }

    if (i >= bufnum)
	goto theend;

    /* Get here if "buf" cannot be abandoned. */
    ret = TRUE;
    exiting = FALSE;
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
    /*
     * When ":confirm" used, don't give an error message.
     */
    if (!(p_confirm || cmdmod.confirm))
#endif
    {
	/* There must be a wait_return for this message, do_buffer()
	 * may cause a redraw.  But wait_return() is a no-op when vgetc()
	 * is busy (Quit used from window menu), then make sure we don't
	 * cause a scroll up. */
	if (vgetc_busy > 0)
	{
	    msg_row = cmdline_row;
	    msg_col = 0;
	    msg_didout = FALSE;
	}
	if (
#ifdef FEAT_TERMINAL
		term_job_running(buf->b_term)
		    ? semsg(_("E947: Job still running in buffer \"%s\""),
								  buf->b_fname)
		    :
#endif
		semsg(_("E162: No write since last change for buffer \"%s\""),
		    buf_spname(buf) != NULL ? buf_spname(buf) : buf->b_fname))
	{
	    save = no_wait_return;
	    no_wait_return = FALSE;
	    wait_return(FALSE);
	    no_wait_return = save;
	}
    }

    /* Try to find a window that contains the buffer. */
    if (buf != curbuf)
	FOR_ALL_TAB_WINDOWS(tp, wp)
	    if (wp->w_buffer == buf)
	    {
		bufref_T bufref;

		set_bufref(&bufref, buf);

		goto_tabpage_win(tp, wp);

		// Paranoia: did autocmd wipe out the buffer with changes?
		if (!bufref_valid(&bufref))
		    goto theend;
		goto buf_found;
	    }
buf_found:

    /* Open the changed buffer in the current window. */
    if (buf != curbuf)
	set_curbuf(buf, unload ? DOBUF_UNLOAD : DOBUF_GOTO);

theend:
    vim_free(bufnrs);
    return ret;
}

/*
 * return FAIL if there is no file name, OK if there is one
 * give error message for FAIL
 */
    int
check_fname(void)
{
    if (curbuf->b_ffname == NULL)
    {
	emsg(_(e_noname));
	return FAIL;
    }
    return OK;
}

/*
 * flush the contents of a buffer, unless it has no file name
 *
 * return FAIL for failure, OK otherwise
 */
    int
buf_write_all(buf_T *buf, int forceit)
{
    int	    retval;
    buf_T	*old_curbuf = curbuf;

    retval = (buf_write(buf, buf->b_ffname, buf->b_fname,
				   (linenr_T)1, buf->b_ml.ml_line_count, NULL,
						  FALSE, forceit, TRUE, FALSE));
    if (curbuf != old_curbuf)
    {
	msg_source(HL_ATTR(HLF_W));
	msg(_("Warning: Entered other buffer unexpectedly (check autocommands)"));
    }
    return retval;
}

/*
 * ":argdo", ":windo", ":bufdo", ":tabdo", ":cdo", ":ldo", ":cfdo" and ":lfdo"
 */
    void
ex_listdo(exarg_T *eap)
{
    int		i;
    win_T	*wp;
    tabpage_T	*tp;
    buf_T	*buf = curbuf;
    int		next_fnum = 0;
#if defined(FEAT_SYN_HL)
    char_u	*save_ei = NULL;
#endif
    char_u	*p_shm_save;
#ifdef FEAT_QUICKFIX
    int		qf_size = 0;
    int		qf_idx;
#endif

#ifndef FEAT_QUICKFIX
    if (eap->cmdidx == CMD_cdo || eap->cmdidx == CMD_ldo ||
	    eap->cmdidx == CMD_cfdo || eap->cmdidx == CMD_lfdo)
    {
	ex_ni(eap);
	return;
    }
#endif

#if defined(FEAT_SYN_HL)
    if (eap->cmdidx != CMD_windo && eap->cmdidx != CMD_tabdo)
    {
	/* Don't do syntax HL autocommands.  Skipping the syntax file is a
	 * great speed improvement. */
	save_ei = au_event_disable(",Syntax");

	for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	    buf->b_flags &= ~BF_SYN_SET;
	buf = curbuf;
    }
#endif
#ifdef FEAT_CLIPBOARD
    start_global_changes();
#endif

    if (eap->cmdidx == CMD_windo
	    || eap->cmdidx == CMD_tabdo
	    || buf_hide(curbuf)
	    || !check_changed(curbuf, CCGD_AW
				    | (eap->forceit ? CCGD_FORCEIT : 0)
				    | CCGD_EXCMD))
    {
	i = 0;
	/* start at the eap->line1 argument/window/buffer */
	wp = firstwin;
	tp = first_tabpage;
	switch (eap->cmdidx)
	{
	    case CMD_windo:
		for ( ; wp != NULL && i + 1 < eap->line1; wp = wp->w_next)
		    i++;
		break;
	    case CMD_tabdo:
		for( ; tp != NULL && i + 1 < eap->line1; tp = tp->tp_next)
		    i++;
		break;
	    case CMD_argdo:
		i = eap->line1 - 1;
		break;
	    default:
		break;
	}
	/* set pcmark now */
	if (eap->cmdidx == CMD_bufdo)
	{
	    /* Advance to the first listed buffer after "eap->line1". */
	    for (buf = firstbuf; buf != NULL && (buf->b_fnum < eap->line1
					  || !buf->b_p_bl); buf = buf->b_next)
		if (buf->b_fnum > eap->line2)
		{
		    buf = NULL;
		    break;
		}
	    if (buf != NULL)
		goto_buffer(eap, DOBUF_FIRST, FORWARD, buf->b_fnum);
	}
#ifdef FEAT_QUICKFIX
	else if (eap->cmdidx == CMD_cdo || eap->cmdidx == CMD_ldo
		|| eap->cmdidx == CMD_cfdo || eap->cmdidx == CMD_lfdo)
	{
	    qf_size = qf_get_valid_size(eap);
	    if (qf_size <= 0 || eap->line1 > qf_size)
		buf = NULL;
	    else
	    {
		ex_cc(eap);

		buf = curbuf;
		i = eap->line1 - 1;
		if (eap->addr_count <= 0)
		    /* default is all the quickfix/location list entries */
		    eap->line2 = qf_size;
	    }
	}
#endif
	else
	    setpcmark();
	listcmd_busy = TRUE;	    /* avoids setting pcmark below */

	while (!got_int && buf != NULL)
	{
	    if (eap->cmdidx == CMD_argdo)
	    {
		/* go to argument "i" */
		if (i == ARGCOUNT)
		    break;
		/* Don't call do_argfile() when already there, it will try
		 * reloading the file. */
		if (curwin->w_arg_idx != i || !editing_arg_idx(curwin))
		{
		    /* Clear 'shm' to avoid that the file message overwrites
		     * any output from the command. */
		    p_shm_save = vim_strsave(p_shm);
		    set_option_value((char_u *)"shm", 0L, (char_u *)"", 0);
		    do_argfile(eap, i);
		    set_option_value((char_u *)"shm", 0L, p_shm_save, 0);
		    vim_free(p_shm_save);
		}
		if (curwin->w_arg_idx != i)
		    break;
	    }
	    else if (eap->cmdidx == CMD_windo)
	    {
		/* go to window "wp" */
		if (!win_valid(wp))
		    break;
		win_goto(wp);
		if (curwin != wp)
		    break;  /* something must be wrong */
		wp = curwin->w_next;
	    }
	    else if (eap->cmdidx == CMD_tabdo)
	    {
		/* go to window "tp" */
		if (!valid_tabpage(tp))
		    break;
		goto_tabpage_tp(tp, TRUE, TRUE);
		tp = tp->tp_next;
	    }
	    else if (eap->cmdidx == CMD_bufdo)
	    {
		/* Remember the number of the next listed buffer, in case
		 * ":bwipe" is used or autocommands do something strange. */
		next_fnum = -1;
		for (buf = curbuf->b_next; buf != NULL; buf = buf->b_next)
		    if (buf->b_p_bl)
		    {
			next_fnum = buf->b_fnum;
			break;
		    }
	    }

	    ++i;

	    /* execute the command */
	    do_cmdline(eap->arg, eap->getline, eap->cookie,
						DOCMD_VERBOSE + DOCMD_NOWAIT);

	    if (eap->cmdidx == CMD_bufdo)
	    {
		/* Done? */
		if (next_fnum < 0 || next_fnum > eap->line2)
		    break;
		/* Check if the buffer still exists. */
		FOR_ALL_BUFFERS(buf)
		    if (buf->b_fnum == next_fnum)
			break;
		if (buf == NULL)
		    break;

		/* Go to the next buffer.  Clear 'shm' to avoid that the file
		 * message overwrites any output from the command. */
		p_shm_save = vim_strsave(p_shm);
		set_option_value((char_u *)"shm", 0L, (char_u *)"", 0);
		goto_buffer(eap, DOBUF_FIRST, FORWARD, next_fnum);
		set_option_value((char_u *)"shm", 0L, p_shm_save, 0);
		vim_free(p_shm_save);

		/* If autocommands took us elsewhere, quit here. */
		if (curbuf->b_fnum != next_fnum)
		    break;
	    }

#ifdef FEAT_QUICKFIX
	    if (eap->cmdidx == CMD_cdo || eap->cmdidx == CMD_ldo
		    || eap->cmdidx == CMD_cfdo || eap->cmdidx == CMD_lfdo)
	    {
		if (i >= qf_size || i >= eap->line2)
		    break;

		qf_idx = qf_get_cur_idx(eap);

		ex_cnext(eap);

		/* If jumping to the next quickfix entry fails, quit here */
		if (qf_get_cur_idx(eap) == qf_idx)
		    break;
	    }
#endif

	    if (eap->cmdidx == CMD_windo)
	    {
		validate_cursor();	/* cursor may have moved */

		/* required when 'scrollbind' has been set */
		if (curwin->w_p_scb)
		    do_check_scrollbind(TRUE);
	    }

	    if (eap->cmdidx == CMD_windo || eap->cmdidx == CMD_tabdo)
		if (i+1 > eap->line2)
		    break;
	    if (eap->cmdidx == CMD_argdo && i >= eap->line2)
		break;
	}
	listcmd_busy = FALSE;
    }

#if defined(FEAT_SYN_HL)
    if (save_ei != NULL)
    {
	buf_T		*bnext;
	aco_save_T	aco;

	au_event_restore(save_ei);

	for (buf = firstbuf; buf != NULL; buf = bnext)
	{
	    bnext = buf->b_next;
	    if (buf->b_nwindows > 0 && (buf->b_flags & BF_SYN_SET))
	    {
		buf->b_flags &= ~BF_SYN_SET;

		// buffer was opened while Syntax autocommands were disabled,
		// need to trigger them now.
		if (buf == curbuf)
		    apply_autocmds(EVENT_SYNTAX, curbuf->b_p_syn,
					       curbuf->b_fname, TRUE, curbuf);
		else
		{
		    aucmd_prepbuf(&aco, buf);
		    apply_autocmds(EVENT_SYNTAX, buf->b_p_syn,
						      buf->b_fname, TRUE, buf);
		    aucmd_restbuf(&aco);
		}

		// start over, in case autocommands messed things up.
		bnext = firstbuf;
	    }
	}
    }
#endif
#ifdef FEAT_CLIPBOARD
    end_global_changes();
#endif
}

#ifdef FEAT_EVAL
/*
 * ":compiler[!] {name}"
 */
    void
ex_compiler(exarg_T *eap)
{
    char_u	*buf;
    char_u	*old_cur_comp = NULL;
    char_u	*p;

    if (*eap->arg == NUL)
    {
	/* List all compiler scripts. */
	do_cmdline_cmd((char_u *)"echo globpath(&rtp, 'compiler/*.vim')");
					/* ) keep the indenter happy... */
    }
    else
    {
	buf = alloc(STRLEN(eap->arg) + 14);
	if (buf != NULL)
	{
	    if (eap->forceit)
	    {
		/* ":compiler! {name}" sets global options */
		do_cmdline_cmd((char_u *)
				   "command -nargs=* CompilerSet set <args>");
	    }
	    else
	    {
		/* ":compiler! {name}" sets local options.
		 * To remain backwards compatible "current_compiler" is always
		 * used.  A user's compiler plugin may set it, the distributed
		 * plugin will then skip the settings.  Afterwards set
		 * "b:current_compiler" and restore "current_compiler".
		 * Explicitly prepend "g:" to make it work in a function. */
		old_cur_comp = get_var_value((char_u *)"g:current_compiler");
		if (old_cur_comp != NULL)
		    old_cur_comp = vim_strsave(old_cur_comp);
		do_cmdline_cmd((char_u *)
			      "command -nargs=* CompilerSet setlocal <args>");
	    }
	    do_unlet((char_u *)"g:current_compiler", TRUE);
	    do_unlet((char_u *)"b:current_compiler", TRUE);

	    sprintf((char *)buf, "compiler/%s.vim", eap->arg);
	    if (source_runtime(buf, DIP_ALL) == FAIL)
		semsg(_("E666: compiler not supported: %s"), eap->arg);
	    vim_free(buf);

	    do_cmdline_cmd((char_u *)":delcommand CompilerSet");

	    /* Set "b:current_compiler" from "current_compiler". */
	    p = get_var_value((char_u *)"g:current_compiler");
	    if (p != NULL)
		set_internal_string_var((char_u *)"b:current_compiler", p);

	    /* Restore "current_compiler" for ":compiler {name}". */
	    if (!eap->forceit)
	    {
		if (old_cur_comp != NULL)
		{
		    set_internal_string_var((char_u *)"g:current_compiler",
								old_cur_comp);
		    vim_free(old_cur_comp);
		}
		else
		    do_unlet((char_u *)"g:current_compiler", TRUE);
	    }
	}
    }
}
#endif

/*
 * ":runtime [what] {name}"
 */
    void
ex_runtime(exarg_T *eap)
{
    char_u  *arg = eap->arg;
    char_u  *p = skiptowhite(arg);
    int	    len = (int)(p - arg);
    int	    flags = eap->forceit ? DIP_ALL : 0;

    if (STRNCMP(arg, "START", len) == 0)
    {
	flags += DIP_START + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "OPT", len) == 0)
    {
	flags += DIP_OPT + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "PACK", len) == 0)
    {
	flags += DIP_START + DIP_OPT + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "ALL", len) == 0)
    {
	flags += DIP_START + DIP_OPT;
	arg = skipwhite(arg + len);
    }

    source_runtime(arg, flags);
}

    static void
source_callback(char_u *fname, void *cookie UNUSED)
{
    (void)do_source(fname, FALSE, DOSO_NONE);
}

/*
 * Find the file "name" in all directories in "path" and invoke
 * "callback(fname, cookie)".
 * "name" can contain wildcards.
 * When "flags" has DIP_ALL: source all files, otherwise only the first one.
 * When "flags" has DIP_DIR: find directories instead of files.
 * When "flags" has DIP_ERR: give an error message if there is no match.
 *
 * return FAIL when no file could be sourced, OK otherwise.
 */
    int
do_in_path(
    char_u	*path,
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    char_u	*rtp;
    char_u	*np;
    char_u	*buf;
    char_u	*rtp_copy;
    char_u	*tail;
    int		num_files;
    char_u	**files;
    int		i;
    int		did_one = FALSE;
#ifdef AMIGA
    struct Process	*proc = (struct Process *)FindTask(0L);
    APTR		save_winptr = proc->pr_WindowPtr;

    /* Avoid a requester here for a volume that doesn't exist. */
    proc->pr_WindowPtr = (APTR)-1L;
#endif

    /* Make a copy of 'runtimepath'.  Invoking the callback may change the
     * value. */
    rtp_copy = vim_strsave(path);
    buf = alloc(MAXPATHL);
    if (buf != NULL && rtp_copy != NULL)
    {
	if (p_verbose > 1 && name != NULL)
	{
	    verbose_enter();
	    smsg(_("Searching for \"%s\" in \"%s\""),
						 (char *)name, (char *)path);
	    verbose_leave();
	}

	/* Loop over all entries in 'runtimepath'. */
	rtp = rtp_copy;
	while (*rtp != NUL && ((flags & DIP_ALL) || !did_one))
	{
	    size_t buflen;

	    /* Copy the path from 'runtimepath' to buf[]. */
	    copy_option_part(&rtp, buf, MAXPATHL, ",");
	    buflen = STRLEN(buf);

	    /* Skip after or non-after directories. */
	    if (flags & (DIP_NOAFTER | DIP_AFTER))
	    {
		int is_after = buflen >= 5
				     && STRCMP(buf + buflen - 5, "after") == 0;

		if ((is_after && (flags & DIP_NOAFTER))
			|| (!is_after && (flags & DIP_AFTER)))
		    continue;
	    }

	    if (name == NULL)
	    {
		(*callback)(buf, (void *) &cookie);
		if (!did_one)
		    did_one = (cookie == NULL);
	    }
	    else if (buflen + STRLEN(name) + 2 < MAXPATHL)
	    {
		add_pathsep(buf);
		tail = buf + STRLEN(buf);

		/* Loop over all patterns in "name" */
		np = name;
		while (*np != NUL && ((flags & DIP_ALL) || !did_one))
		{
		    /* Append the pattern from "name" to buf[]. */
		    copy_option_part(&np, tail, (int)(MAXPATHL - (tail - buf)),
								       "\t ");

		    if (p_verbose > 2)
		    {
			verbose_enter();
			smsg(_("Searching for \"%s\""), buf);
			verbose_leave();
		    }

		    /* Expand wildcards, invoke the callback for each match. */
		    if (gen_expand_wildcards(1, &buf, &num_files, &files,
				  (flags & DIP_DIR) ? EW_DIR : EW_FILE) == OK)
		    {
			for (i = 0; i < num_files; ++i)
			{
			    (*callback)(files[i], cookie);
			    did_one = TRUE;
			    if (!(flags & DIP_ALL))
				break;
			}
			FreeWild(num_files, files);
		    }
		}
	    }
	}
    }
    vim_free(buf);
    vim_free(rtp_copy);
    if (!did_one && name != NULL)
    {
	char *basepath = path == p_rtp ? "runtimepath" : "packpath";

	if (flags & DIP_ERR)
	    semsg(_(e_dirnotf), basepath, name);
	else if (p_verbose > 0)
	{
	    verbose_enter();
	    smsg(_("not found in '%s': \"%s\""), basepath, name);
	    verbose_leave();
	}
    }

#ifdef AMIGA
    proc->pr_WindowPtr = save_winptr;
#endif

    return did_one ? OK : FAIL;
}

/*
 * Find "name" in "path".  When found, invoke the callback function for
 * it: callback(fname, "cookie")
 * When "flags" has DIP_ALL repeat for all matches, otherwise only the first
 * one is used.
 * Returns OK when at least one match found, FAIL otherwise.
 *
 * If "name" is NULL calls callback for each entry in "path". Cookie is
 * passed by reference in this case, setting it to NULL indicates that callback
 * has done its job.
 */
    static int
do_in_path_and_pp(
    char_u	*path,
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    int		done = FAIL;
    char_u	*s;
    int		len;
    char	*start_dir = "pack/*/start/*/%s";
    char	*opt_dir = "pack/*/opt/*/%s";

    if ((flags & DIP_NORTP) == 0)
	done = do_in_path(path, name, flags, callback, cookie);

    if ((done == FAIL || (flags & DIP_ALL)) && (flags & DIP_START))
    {
	len = (int)(STRLEN(start_dir) + STRLEN(name));
	s = alloc(len);
	if (s == NULL)
	    return FAIL;
	vim_snprintf((char *)s, len, start_dir, name);
	done = do_in_path(p_pp, s, flags, callback, cookie);
	vim_free(s);
    }

    if ((done == FAIL || (flags & DIP_ALL)) && (flags & DIP_OPT))
    {
	len = (int)(STRLEN(opt_dir) + STRLEN(name));
	s = alloc(len);
	if (s == NULL)
	    return FAIL;
	vim_snprintf((char *)s, len, opt_dir, name);
	done = do_in_path(p_pp, s, flags, callback, cookie);
	vim_free(s);
    }

    return done;
}

/*
 * Just like do_in_path_and_pp(), using 'runtimepath' for "path".
 */
    int
do_in_runtimepath(
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    return do_in_path_and_pp(p_rtp, name, flags, callback, cookie);
}

/*
 * Source the file "name" from all directories in 'runtimepath'.
 * "name" can contain wildcards.
 * When "flags" has DIP_ALL: source all files, otherwise only the first one.
 *
 * return FAIL when no file could be sourced, OK otherwise.
 */
    int
source_runtime(char_u *name, int flags)
{
    return source_in_path(p_rtp, name, flags);
}

/*
 * Just like source_runtime(), but use "path" instead of 'runtimepath'.
 */
    int
source_in_path(char_u *path, char_u *name, int flags)
{
    return do_in_path_and_pp(path, name, flags, source_callback, NULL);
}


#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Expand wildcards in "pat" and invoke do_source() for each match.
 */
    static void
source_all_matches(char_u *pat)
{
    int	    num_files;
    char_u  **files;
    int	    i;

    if (gen_expand_wildcards(1, &pat, &num_files, &files, EW_FILE) == OK)
    {
	for (i = 0; i < num_files; ++i)
	    (void)do_source(files[i], FALSE, DOSO_NONE);
	FreeWild(num_files, files);
    }
}

/*
 * Add the package directory to 'runtimepath'.
 */
    static int
add_pack_dir_to_rtp(char_u *fname)
{
    char_u  *p4, *p3, *p2, *p1, *p;
    char_u  *entry;
    char_u  *insp = NULL;
    int	    c;
    char_u  *new_rtp;
    int	    keep;
    size_t  oldlen;
    size_t  addlen;
    size_t  new_rtp_len;
    char_u  *afterdir = NULL;
    size_t  afterlen = 0;
    char_u  *after_insp = NULL;
    char_u  *ffname = NULL;
    size_t  fname_len;
    char_u  *buf = NULL;
    char_u  *rtp_ffname;
    int	    match;
    int	    retval = FAIL;

    p4 = p3 = p2 = p1 = get_past_head(fname);
    for (p = p1; *p; MB_PTR_ADV(p))
	if (vim_ispathsep_nocolon(*p))
	{
	    p4 = p3; p3 = p2; p2 = p1; p1 = p;
	}

    /* now we have:
     * rtp/pack/name/start/name
     *    p4   p3   p2    p1
     *
     * find the part up to "pack" in 'runtimepath' */
    c = *++p4; /* append pathsep in order to expand symlink */
    *p4 = NUL;
    ffname = fix_fname(fname);
    *p4 = c;
    if (ffname == NULL)
	return FAIL;

    // Find "ffname" in "p_rtp", ignoring '/' vs '\' differences.
    // Also stop at the first "after" directory.
    fname_len = STRLEN(ffname);
    buf = alloc(MAXPATHL);
    if (buf == NULL)
	goto theend;
    for (entry = p_rtp; *entry != NUL; )
    {
	char_u *cur_entry = entry;

	copy_option_part(&entry, buf, MAXPATHL, ",");
	if (insp == NULL)
	{
	    add_pathsep(buf);
	    rtp_ffname = fix_fname(buf);
	    if (rtp_ffname == NULL)
		goto theend;
	    match = vim_fnamencmp(rtp_ffname, ffname, fname_len) == 0;
	    vim_free(rtp_ffname);
	    if (match)
		// Insert "ffname" after this entry (and comma).
		insp = entry;
	}

	if ((p = (char_u *)strstr((char *)buf, "after")) != NULL
		&& p > buf
		&& vim_ispathsep(p[-1])
		&& (vim_ispathsep(p[5]) || p[5] == NUL || p[5] == ','))
	{
	    if (insp == NULL)
		// Did not find "ffname" before the first "after" directory,
		// insert it before this entry.
		insp = cur_entry;
	    after_insp = cur_entry;
	    break;
	}
    }

    if (insp == NULL)
	// Both "fname" and "after" not found, append at the end.
	insp = p_rtp + STRLEN(p_rtp);

    // check if rtp/pack/name/start/name/after exists
    afterdir = concat_fnames(fname, (char_u *)"after", TRUE);
    if (afterdir != NULL && mch_isdir(afterdir))
	afterlen = STRLEN(afterdir) + 1; // add one for comma

    oldlen = STRLEN(p_rtp);
    addlen = STRLEN(fname) + 1; // add one for comma
    new_rtp = alloc(oldlen + addlen + afterlen + 1); // add one for NUL
    if (new_rtp == NULL)
	goto theend;

    // We now have 'rtp' parts: {keep}{keep_after}{rest}.
    // Create new_rtp, first: {keep},{fname}
    keep = (int)(insp - p_rtp);
    mch_memmove(new_rtp, p_rtp, keep);
    new_rtp_len = keep;
    if (*insp == NUL)
	new_rtp[new_rtp_len++] = ',';  // add comma before
    mch_memmove(new_rtp + new_rtp_len, fname, addlen - 1);
    new_rtp_len += addlen - 1;
    if (*insp != NUL)
	new_rtp[new_rtp_len++] = ',';  // add comma after

    if (afterlen > 0 && after_insp != NULL)
    {
	int keep_after = (int)(after_insp - p_rtp);

	// Add to new_rtp: {keep},{fname}{keep_after},{afterdir}
	mch_memmove(new_rtp + new_rtp_len, p_rtp + keep,
							keep_after - keep);
	new_rtp_len += keep_after - keep;
	mch_memmove(new_rtp + new_rtp_len, afterdir, afterlen - 1);
	new_rtp_len += afterlen - 1;
	new_rtp[new_rtp_len++] = ',';
	keep = keep_after;
    }

    if (p_rtp[keep] != NUL)
	// Append rest: {keep},{fname}{keep_after},{afterdir}{rest}
	mch_memmove(new_rtp + new_rtp_len, p_rtp + keep, oldlen - keep + 1);
    else
	new_rtp[new_rtp_len] = NUL;

    if (afterlen > 0 && after_insp == NULL)
    {
	// Append afterdir when "after" was not found:
	// {keep},{fname}{rest},{afterdir}
	STRCAT(new_rtp, ",");
	STRCAT(new_rtp, afterdir);
    }

    set_option_value((char_u *)"rtp", 0L, new_rtp, 0);
    vim_free(new_rtp);
    retval = OK;

theend:
    vim_free(buf);
    vim_free(ffname);
    vim_free(afterdir);
    return retval;
}

/*
 * Load scripts in "plugin" and "ftdetect" directories of the package.
 */
    static int
load_pack_plugin(char_u *fname)
{
    static char *plugpat = "%s/plugin/**/*.vim";
    static char *ftpat = "%s/ftdetect/*.vim";
    int		len;
    char_u	*ffname = fix_fname(fname);
    char_u	*pat = NULL;
    int		retval = FAIL;

    if (ffname == NULL)
	return FAIL;
    len = (int)STRLEN(ffname) + (int)STRLEN(ftpat);
    pat = alloc(len);
    if (pat == NULL)
	goto theend;
    vim_snprintf((char *)pat, len, plugpat, ffname);
    source_all_matches(pat);

    {
	char_u *cmd = vim_strsave((char_u *)"g:did_load_filetypes");

	/* If runtime/filetype.vim wasn't loaded yet, the scripts will be
	 * found when it loads. */
	if (cmd != NULL && eval_to_number(cmd) > 0)
	{
	    do_cmdline_cmd((char_u *)"augroup filetypedetect");
	    vim_snprintf((char *)pat, len, ftpat, ffname);
	    source_all_matches(pat);
	    do_cmdline_cmd((char_u *)"augroup END");
	}
	vim_free(cmd);
    }
    vim_free(pat);
    retval = OK;

theend:
    vim_free(ffname);
    return retval;
}

/* used for "cookie" of add_pack_plugin() */
static int APP_ADD_DIR;
static int APP_LOAD;
static int APP_BOTH;

    static void
add_pack_plugin(char_u *fname, void *cookie)
{
    if (cookie != &APP_LOAD)
    {
	char_u	*buf = alloc(MAXPATHL);
	char_u	*p;
	int	found = FALSE;

	if (buf == NULL)
	    return;
	p = p_rtp;
	while (*p != NUL)
	{
	    copy_option_part(&p, buf, MAXPATHL, ",");
	    if (pathcmp((char *)buf, (char *)fname, -1) == 0)
	    {
		found = TRUE;
		break;
	    }
	}
	vim_free(buf);
	if (!found)
	    /* directory is not yet in 'runtimepath', add it */
	    if (add_pack_dir_to_rtp(fname) == FAIL)
		return;
    }

    if (cookie != &APP_ADD_DIR)
	load_pack_plugin(fname);
}

/*
 * Add all packages in the "start" directory to 'runtimepath'.
 */
    void
add_pack_start_dirs(void)
{
    do_in_path(p_pp, (char_u *)"pack/*/start/*", DIP_ALL + DIP_DIR,
					       add_pack_plugin, &APP_ADD_DIR);
}

/*
 * Load plugins from all packages in the "start" directory.
 */
    void
load_start_packages(void)
{
    did_source_packages = TRUE;
    do_in_path(p_pp, (char_u *)"pack/*/start/*", DIP_ALL + DIP_DIR,
						  add_pack_plugin, &APP_LOAD);
}

/*
 * ":packloadall"
 * Find plugins in the package directories and source them.
 */
    void
ex_packloadall(exarg_T *eap)
{
    if (!did_source_packages || eap->forceit)
    {
	/* First do a round to add all directories to 'runtimepath', then load
	 * the plugins. This allows for plugins to use an autoload directory
	 * of another plugin. */
	add_pack_start_dirs();
	load_start_packages();
    }
}

/*
 * ":packadd[!] {name}"
 */
    void
ex_packadd(exarg_T *eap)
{
    static char *plugpat = "pack/*/%s/%s";
    int		len;
    char	*pat;
    int		round;
    int		res = OK;

    /* Round 1: use "start", round 2: use "opt". */
    for (round = 1; round <= 2; ++round)
    {
	/* Only look under "start" when loading packages wasn't done yet. */
	if (round == 1 && did_source_packages)
	    continue;

	len = (int)STRLEN(plugpat) + (int)STRLEN(eap->arg) + 5;
	pat = alloc(len);
	if (pat == NULL)
	    return;
	vim_snprintf(pat, len, plugpat, round == 1 ? "start" : "opt", eap->arg);
	/* The first round don't give a "not found" error, in the second round
	 * only when nothing was found in the first round. */
	res = do_in_path(p_pp, (char_u *)pat,
		DIP_ALL + DIP_DIR + (round == 2 && res == FAIL ? DIP_ERR : 0),
		add_pack_plugin, eap->forceit ? &APP_ADD_DIR : &APP_BOTH);
	vim_free(pat);
    }
}
#endif

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * ":options"
 */
    void
ex_options(
    exarg_T	*eap UNUSED)
{
    vim_setenv((char_u *)"OPTWIN_CMD",
	    (char_u *)(cmdmod.tab ? "tab"
		: (cmdmod.split & WSP_VERT) ? "vert" : ""));
    cmd_source((char_u *)SYS_OPTWIN_FILE, NULL);
}
#endif

#if defined(FEAT_PYTHON3) || defined(FEAT_PYTHON) || defined(PROTO)

# if (defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)) || defined(PROTO)
/*
 * Detect Python 3 or 2, and initialize 'pyxversion'.
 */
    void
init_pyxversion(void)
{
    if (p_pyx == 0)
    {
	if (python3_enabled(FALSE))
	    p_pyx = 3;
	else if (python_enabled(FALSE))
	    p_pyx = 2;
    }
}
# endif

/*
 * Does a file contain one of the following strings at the beginning of any
 * line?
 * "#!(any string)python2"  => returns 2
 * "#!(any string)python3"  => returns 3
 * "# requires python 2.x"  => returns 2
 * "# requires python 3.x"  => returns 3
 * otherwise return 0.
 */
    static int
requires_py_version(char_u *filename)
{
    FILE    *file;
    int	    requires_py_version = 0;
    int	    i, lines;

    lines = (int)p_mls;
    if (lines < 0)
	lines = 5;

    file = mch_fopen((char *)filename, "r");
    if (file != NULL)
    {
	for (i = 0; i < lines; i++)
	{
	    if (vim_fgets(IObuff, IOSIZE, file))
		break;
	    if (i == 0 && IObuff[0] == '#' && IObuff[1] == '!')
	    {
		/* Check shebang. */
		if (strstr((char *)IObuff + 2, "python2") != NULL)
		{
		    requires_py_version = 2;
		    break;
		}
		if (strstr((char *)IObuff + 2, "python3") != NULL)
		{
		    requires_py_version = 3;
		    break;
		}
	    }
	    IObuff[21] = '\0';
	    if (STRCMP("# requires python 2.x", IObuff) == 0)
	    {
		requires_py_version = 2;
		break;
	    }
	    if (STRCMP("# requires python 3.x", IObuff) == 0)
	    {
		requires_py_version = 3;
		break;
	    }
	}
	fclose(file);
    }
    return requires_py_version;
}


/*
 * Source a python file using the requested python version.
 */
    static void
source_pyx_file(exarg_T *eap, char_u *fname)
{
    exarg_T ex;
    int	    v = requires_py_version(fname);

# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
    init_pyxversion();
# endif
    if (v == 0)
    {
# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
	/* user didn't choose a preference, 'pyx' is used */
	v = p_pyx;
# elif defined(FEAT_PYTHON)
	v = 2;
# elif defined(FEAT_PYTHON3)
	v = 3;
# endif
    }

    /*
     * now source, if required python version is not supported show
     * unobtrusive message.
     */
    if (eap == NULL)
	vim_memset(&ex, 0, sizeof(ex));
    else
	ex = *eap;
    ex.arg = fname;
    ex.cmd = (char_u *)(v == 2 ? "pyfile" : "pyfile3");

    if (v == 2)
    {
# ifdef FEAT_PYTHON
	ex_pyfile(&ex);
# else
	vim_snprintf((char *)IObuff, IOSIZE,
		_("W20: Required python version 2.x not supported, ignoring file: %s"),
		fname);
	msg((char *)IObuff);
# endif
	return;
    }
    else
    {
# ifdef FEAT_PYTHON3
	ex_py3file(&ex);
# else
	vim_snprintf((char *)IObuff, IOSIZE,
		_("W21: Required python version 3.x not supported, ignoring file: %s"),
		fname);
	msg((char *)IObuff);
# endif
	return;
    }
}

/*
 * ":pyxfile {fname}"
 */
    void
ex_pyxfile(exarg_T *eap)
{
    source_pyx_file(eap, eap->arg);
}

/*
 * ":pyx"
 */
    void
ex_pyx(exarg_T *eap)
{
# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
    init_pyxversion();
    if (p_pyx == 2)
	ex_python(eap);
    else
	ex_py3(eap);
# elif defined(FEAT_PYTHON)
    ex_python(eap);
# elif defined(FEAT_PYTHON3)
    ex_py3(eap);
# endif
}

/*
 * ":pyxdo"
 */
    void
ex_pyxdo(exarg_T *eap)
{
# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
    init_pyxversion();
    if (p_pyx == 2)
	ex_pydo(eap);
    else
	ex_py3do(eap);
# elif defined(FEAT_PYTHON)
    ex_pydo(eap);
# elif defined(FEAT_PYTHON3)
    ex_py3do(eap);
# endif
}

#endif

/*
 * ":source {fname}"
 */
    void
ex_source(exarg_T *eap)
{
#ifdef FEAT_BROWSE
    if (cmdmod.browse)
    {
	char_u *fname = NULL;

	fname = do_browse(0, (char_u *)_("Source Vim script"), eap->arg,
				      NULL, NULL,
				      (char_u *)_(BROWSE_FILTER_MACROS), NULL);
	if (fname != NULL)
	{
	    cmd_source(fname, eap);
	    vim_free(fname);
	}
    }
    else
#endif
	cmd_source(eap->arg, eap);
}

    static void
cmd_source(char_u *fname, exarg_T *eap)
{
    if (*fname == NUL)
	emsg(_(e_argreq));

    else if (eap != NULL && eap->forceit)
	/* ":source!": read Normal mode commands
	 * Need to execute the commands directly.  This is required at least
	 * for:
	 * - ":g" command busy
	 * - after ":argdo", ":windo" or ":bufdo"
	 * - another command follows
	 * - inside a loop
	 */
	openscript(fname, global_busy || listcmd_busy || eap->nextcmd != NULL
#ifdef FEAT_EVAL
						 || eap->cstack->cs_idx >= 0
#endif
						 );

    /* ":source" read ex commands */
    else if (do_source(fname, FALSE, DOSO_NONE) == FAIL)
	semsg(_(e_notopen), fname);
}

/*
 * ":source" and associated commands.
 */
/*
 * Structure used to store info for each sourced file.
 * It is shared between do_source() and getsourceline().
 * This is required, because it needs to be handed to do_cmdline() and
 * sourcing can be done recursively.
 */
struct source_cookie
{
    FILE	*fp;		// opened file for sourcing
    char_u	*nextline;	// if not NULL: line that was read ahead
    linenr_T	sourcing_lnum;	// line number of the source file
    int		finished;	// ":finish" used
#ifdef USE_CRNL
    int		fileformat;	// EOL_UNKNOWN, EOL_UNIX or EOL_DOS
    int		error;		// TRUE if LF found after CR-LF
#endif
#ifdef FEAT_EVAL
    linenr_T	breakpoint;	// next line with breakpoint or zero
    char_u	*fname;		// name of sourced file
    int		dbg_tick;	// debug_tick when breakpoint was set
    int		level;		// top nesting level of sourced file
#endif
    vimconv_T	conv;		// type of conversion
};

#ifdef FEAT_EVAL
/*
 * Return the address holding the next breakpoint line for a source cookie.
 */
    linenr_T *
source_breakpoint(void *cookie)
{
    return &((struct source_cookie *)cookie)->breakpoint;
}

/*
 * Return the address holding the debug tick for a source cookie.
 */
    int *
source_dbg_tick(void *cookie)
{
    return &((struct source_cookie *)cookie)->dbg_tick;
}

/*
 * Return the nesting level for a source cookie.
 */
    int
source_level(void *cookie)
{
    return ((struct source_cookie *)cookie)->level;
}
#endif

static char_u *get_one_sourceline(struct source_cookie *sp);

#if (defined(MSWIN) && defined(FEAT_CSCOPE)) || defined(HAVE_FD_CLOEXEC)
# define USE_FOPEN_NOINH
/*
 * Special function to open a file without handle inheritance.
 * When possible the handle is closed on exec().
 */
    static FILE *
fopen_noinh_readbin(char *filename)
{
# ifdef MSWIN
    int	fd_tmp = mch_open(filename, O_RDONLY | O_BINARY | O_NOINHERIT, 0);
# else
    int	fd_tmp = mch_open(filename, O_RDONLY, 0);
# endif

    if (fd_tmp == -1)
	return NULL;

# ifdef HAVE_FD_CLOEXEC
    {
	int fdflags = fcntl(fd_tmp, F_GETFD);
	if (fdflags >= 0 && (fdflags & FD_CLOEXEC) == 0)
	    (void)fcntl(fd_tmp, F_SETFD, fdflags | FD_CLOEXEC);
    }
# endif

    return fdopen(fd_tmp, READBIN);
}
#endif

/*
 * do_source: Read the file "fname" and execute its lines as EX commands.
 *
 * This function may be called recursively!
 *
 * return FAIL if file could not be opened, OK otherwise
 */
    int
do_source(
    char_u	*fname,
    int		check_other,	    /* check for .vimrc and _vimrc */
    int		is_vimrc)	    /* DOSO_ value */
{
    struct source_cookie    cookie;
    char_u		    *save_sourcing_name;
    linenr_T		    save_sourcing_lnum;
    char_u		    *p;
    char_u		    *fname_exp;
    char_u		    *firstline = NULL;
    int			    retval = FAIL;
#ifdef FEAT_EVAL
    sctx_T		    save_current_sctx;
    static scid_T	    last_current_SID = 0;
    static int		    last_current_SID_seq = 0;
    funccal_entry_T	    funccalp_entry;
    int			    save_debug_break_level = debug_break_level;
    scriptitem_T	    *si = NULL;
# ifdef UNIX
    stat_T		    st;
    int			    stat_ok;
# endif
#endif
#ifdef STARTUPTIME
    struct timeval	    tv_rel;
    struct timeval	    tv_start;
#endif
#ifdef FEAT_PROFILE
    proftime_T		    wait_start;
#endif
    int			    trigger_source_post = FALSE;

    p = expand_env_save(fname);
    if (p == NULL)
	return retval;
    fname_exp = fix_fname(p);
    vim_free(p);
    if (fname_exp == NULL)
	return retval;
    if (mch_isdir(fname_exp))
    {
	smsg(_("Cannot source a directory: \"%s\""), fname);
	goto theend;
    }

    /* Apply SourceCmd autocommands, they should get the file and source it. */
    if (has_autocmd(EVENT_SOURCECMD, fname_exp, NULL)
	    && apply_autocmds(EVENT_SOURCECMD, fname_exp, fname_exp,
							       FALSE, curbuf))
    {
#ifdef FEAT_EVAL
	retval = aborting() ? FAIL : OK;
#else
	retval = OK;
#endif
	if (retval == OK)
	    // Apply SourcePost autocommands.
	    apply_autocmds(EVENT_SOURCEPOST, fname_exp, fname_exp,
								FALSE, curbuf);
	goto theend;
    }

    /* Apply SourcePre autocommands, they may get the file. */
    apply_autocmds(EVENT_SOURCEPRE, fname_exp, fname_exp, FALSE, curbuf);

#ifdef USE_FOPEN_NOINH
    cookie.fp = fopen_noinh_readbin((char *)fname_exp);
#else
    cookie.fp = mch_fopen((char *)fname_exp, READBIN);
#endif
    if (cookie.fp == NULL && check_other)
    {
	/*
	 * Try again, replacing file name ".vimrc" by "_vimrc" or vice versa,
	 * and ".exrc" by "_exrc" or vice versa.
	 */
	p = gettail(fname_exp);
	if ((*p == '.' || *p == '_')
		&& (STRICMP(p + 1, "vimrc") == 0
		    || STRICMP(p + 1, "gvimrc") == 0
		    || STRICMP(p + 1, "exrc") == 0))
	{
	    if (*p == '_')
		*p = '.';
	    else
		*p = '_';
#ifdef USE_FOPEN_NOINH
	    cookie.fp = fopen_noinh_readbin((char *)fname_exp);
#else
	    cookie.fp = mch_fopen((char *)fname_exp, READBIN);
#endif
	}
    }

    if (cookie.fp == NULL)
    {
	if (p_verbose > 0)
	{
	    verbose_enter();
	    if (sourcing_name == NULL)
		smsg(_("could not source \"%s\""), fname);
	    else
		smsg(_("line %ld: could not source \"%s\""),
							sourcing_lnum, fname);
	    verbose_leave();
	}
	goto theend;
    }

    /*
     * The file exists.
     * - In verbose mode, give a message.
     * - For a vimrc file, may want to set 'compatible', call vimrc_found().
     */
    if (p_verbose > 1)
    {
	verbose_enter();
	if (sourcing_name == NULL)
	    smsg(_("sourcing \"%s\""), fname);
	else
	    smsg(_("line %ld: sourcing \"%s\""),
							sourcing_lnum, fname);
	verbose_leave();
    }
    if (is_vimrc == DOSO_VIMRC)
	vimrc_found(fname_exp, (char_u *)"MYVIMRC");
    else if (is_vimrc == DOSO_GVIMRC)
	vimrc_found(fname_exp, (char_u *)"MYGVIMRC");

#ifdef USE_CRNL
    /* If no automatic file format: Set default to CR-NL. */
    if (*p_ffs == NUL)
	cookie.fileformat = EOL_DOS;
    else
	cookie.fileformat = EOL_UNKNOWN;
    cookie.error = FALSE;
#endif

    cookie.nextline = NULL;
    cookie.sourcing_lnum = 0;
    cookie.finished = FALSE;

#ifdef FEAT_EVAL
    /*
     * Check if this script has a breakpoint.
     */
    cookie.breakpoint = dbg_find_breakpoint(TRUE, fname_exp, (linenr_T)0);
    cookie.fname = fname_exp;
    cookie.dbg_tick = debug_tick;

    cookie.level = ex_nesting_level;
#endif

    /*
     * Keep the sourcing name/lnum, for recursive calls.
     */
    save_sourcing_name = sourcing_name;
    sourcing_name = fname_exp;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 0;

#ifdef STARTUPTIME
    if (time_fd != NULL)
	time_push(&tv_rel, &tv_start);
#endif

#ifdef FEAT_EVAL
# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	prof_child_enter(&wait_start);		/* entering a child now */
# endif

    /* Don't use local function variables, if called from a function.
     * Also starts profiling timer for nested script. */
    save_funccal(&funccalp_entry);

    save_current_sctx = current_sctx;
    current_sctx.sc_lnum = 0;
    current_sctx.sc_version = 1;

    // Check if this script was sourced before to finds its SID.
    // If it's new, generate a new SID.
    // Always use a new sequence number.
    current_sctx.sc_seq = ++last_current_SID_seq;
# ifdef UNIX
    stat_ok = (mch_stat((char *)fname_exp, &st) >= 0);
# endif
    for (current_sctx.sc_sid = script_items.ga_len; current_sctx.sc_sid > 0;
							 --current_sctx.sc_sid)
    {
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
	if (si->sn_name != NULL
		&& (
# ifdef UNIX
		    /* Compare dev/ino when possible, it catches symbolic
		     * links.  Also compare file names, the inode may change
		     * when the file was edited. */
		    ((stat_ok && si->sn_dev_valid)
			&& (si->sn_dev == st.st_dev
			    && si->sn_ino == st.st_ino)) ||
# endif
		fnamecmp(si->sn_name, fname_exp) == 0))
	    break;
    }
    if (current_sctx.sc_sid == 0)
    {
	current_sctx.sc_sid = ++last_current_SID;
	if (ga_grow(&script_items,
		     (int)(current_sctx.sc_sid - script_items.ga_len)) == FAIL)
	    goto almosttheend;
	while (script_items.ga_len < current_sctx.sc_sid)
	{
	    ++script_items.ga_len;
	    SCRIPT_ITEM(script_items.ga_len).sn_name = NULL;
# ifdef FEAT_PROFILE
	    SCRIPT_ITEM(script_items.ga_len).sn_prof_on = FALSE;
# endif
	}
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
	si->sn_name = fname_exp;
	fname_exp = vim_strsave(si->sn_name);  // used for autocmd
# ifdef UNIX
	if (stat_ok)
	{
	    si->sn_dev_valid = TRUE;
	    si->sn_dev = st.st_dev;
	    si->sn_ino = st.st_ino;
	}
	else
	    si->sn_dev_valid = FALSE;
# endif

	/* Allocate the local script variables to use for this script. */
	new_script_vars(current_sctx.sc_sid);
    }

# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
    {
	int	forceit;

	/* Check if we do profiling for this script. */
	if (!si->sn_prof_on && has_profiling(TRUE, si->sn_name, &forceit))
	{
	    script_do_profile(si);
	    si->sn_pr_force = forceit;
	}
	if (si->sn_prof_on)
	{
	    ++si->sn_pr_count;
	    profile_start(&si->sn_pr_start);
	    profile_zero(&si->sn_pr_children);
	}
    }
# endif
#endif

    cookie.conv.vc_type = CONV_NONE;		/* no conversion */

    /* Read the first line so we can check for a UTF-8 BOM. */
    firstline = getsourceline(0, (void *)&cookie, 0, TRUE);
    if (firstline != NULL && STRLEN(firstline) >= 3 && firstline[0] == 0xef
			      && firstline[1] == 0xbb && firstline[2] == 0xbf)
    {
	/* Found BOM; setup conversion, skip over BOM and recode the line. */
	convert_setup(&cookie.conv, (char_u *)"utf-8", p_enc);
	p = string_convert(&cookie.conv, firstline + 3, NULL);
	if (p == NULL)
	    p = vim_strsave(firstline + 3);
	if (p != NULL)
	{
	    vim_free(firstline);
	    firstline = p;
	}
    }

    /*
     * Call do_cmdline, which will call getsourceline() to get the lines.
     */
    do_cmdline(firstline, getsourceline, (void *)&cookie,
				     DOCMD_VERBOSE|DOCMD_NOWAIT|DOCMD_REPEAT);
    retval = OK;

#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
    {
	/* Get "si" again, "script_items" may have been reallocated. */
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
	if (si->sn_prof_on)
	{
	    profile_end(&si->sn_pr_start);
	    profile_sub_wait(&wait_start, &si->sn_pr_start);
	    profile_add(&si->sn_pr_total, &si->sn_pr_start);
	    profile_self(&si->sn_pr_self, &si->sn_pr_start,
							 &si->sn_pr_children);
	}
    }
#endif

    if (got_int)
	emsg(_(e_interr));
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    if (p_verbose > 1)
    {
	verbose_enter();
	smsg(_("finished sourcing %s"), fname);
	if (sourcing_name != NULL)
	    smsg(_("continuing in %s"), sourcing_name);
	verbose_leave();
    }
#ifdef STARTUPTIME
    if (time_fd != NULL)
    {
	vim_snprintf((char *)IObuff, IOSIZE, "sourcing %s", fname);
	time_msg((char *)IObuff, &tv_start);
	time_pop(&tv_rel);
    }
#endif

    if (!got_int)
	trigger_source_post = TRUE;

#ifdef FEAT_EVAL
    /*
     * After a "finish" in debug mode, need to break at first command of next
     * sourced file.
     */
    if (save_debug_break_level > ex_nesting_level
	    && debug_break_level == ex_nesting_level)
	++debug_break_level;
#endif

#ifdef FEAT_EVAL
almosttheend:
    current_sctx = save_current_sctx;
    restore_funccal();
# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	prof_child_exit(&wait_start);		/* leaving a child now */
# endif
#endif
    fclose(cookie.fp);
    vim_free(cookie.nextline);
    vim_free(firstline);
    convert_setup(&cookie.conv, NULL, NULL);

    if (trigger_source_post)
	apply_autocmds(EVENT_SOURCEPOST, fname_exp, fname_exp, FALSE, curbuf);

theend:
    vim_free(fname_exp);
    return retval;
}

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * ":scriptnames"
 */
    void
ex_scriptnames(exarg_T *eap)
{
    int i;

    if (eap->addr_count > 0)
    {
	// :script {scriptId}: edit the script
	if (eap->line2 < 1 || eap->line2 > script_items.ga_len)
	    emsg(_(e_invarg));
	else
	{
	    eap->arg = SCRIPT_ITEM(eap->line2).sn_name;
	    do_exedit(eap, NULL);
	}
	return;
    }

    for (i = 1; i <= script_items.ga_len && !got_int; ++i)
	if (SCRIPT_ITEM(i).sn_name != NULL)
	{
	    home_replace(NULL, SCRIPT_ITEM(i).sn_name,
						    NameBuff, MAXPATHL, TRUE);
	    smsg("%3d: %s", i, NameBuff);
	}
}

# if defined(BACKSLASH_IN_FILENAME) || defined(PROTO)
/*
 * Fix slashes in the list of script names for 'shellslash'.
 */
    void
scriptnames_slash_adjust(void)
{
    int i;

    for (i = 1; i <= script_items.ga_len; ++i)
	if (SCRIPT_ITEM(i).sn_name != NULL)
	    slash_adjust(SCRIPT_ITEM(i).sn_name);
}
# endif

/*
 * Get a pointer to a script name.  Used for ":verbose set".
 */
    char_u *
get_scriptname(scid_T id)
{
    if (id == SID_MODELINE)
	return (char_u *)_("modeline");
    if (id == SID_CMDARG)
	return (char_u *)_("--cmd argument");
    if (id == SID_CARG)
	return (char_u *)_("-c argument");
    if (id == SID_ENV)
	return (char_u *)_("environment variable");
    if (id == SID_ERROR)
	return (char_u *)_("error handler");
    return SCRIPT_ITEM(id).sn_name;
}

# if defined(EXITFREE) || defined(PROTO)
    void
free_scriptnames(void)
{
    int			i;

    for (i = script_items.ga_len; i > 0; --i)
	vim_free(SCRIPT_ITEM(i).sn_name);
    ga_clear(&script_items);
}
# endif

#endif

    linenr_T
get_sourced_lnum(char_u *(*fgetline)(int, void *, int, int), void *cookie)
{
    return fgetline == getsourceline
			? ((struct source_cookie *)cookie)->sourcing_lnum
			: sourcing_lnum;
}

/*
 * Get one full line from a sourced file.
 * Called by do_cmdline() when it's called from do_source().
 *
 * Return a pointer to the line in allocated memory.
 * Return NULL for end-of-file or some error.
 */
    char_u *
getsourceline(int c UNUSED, void *cookie, int indent UNUSED, int do_concat)
{
    struct source_cookie *sp = (struct source_cookie *)cookie;
    char_u		*line;
    char_u		*p;

#ifdef FEAT_EVAL
    /* If breakpoints have been added/deleted need to check for it. */
    if (sp->dbg_tick < debug_tick)
    {
	sp->breakpoint = dbg_find_breakpoint(TRUE, sp->fname, sourcing_lnum);
	sp->dbg_tick = debug_tick;
    }
# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	script_line_end();
# endif
#endif

    // Set the current sourcing line number.
    sourcing_lnum = sp->sourcing_lnum + 1;

    /*
     * Get current line.  If there is a read-ahead line, use it, otherwise get
     * one now.
     */
    if (sp->finished)
	line = NULL;
    else if (sp->nextline == NULL)
	line = get_one_sourceline(sp);
    else
    {
	line = sp->nextline;
	sp->nextline = NULL;
	++sp->sourcing_lnum;
    }
#ifdef FEAT_PROFILE
    if (line != NULL && do_profiling == PROF_YES)
	script_line_start();
#endif

    /* Only concatenate lines starting with a \ when 'cpoptions' doesn't
     * contain the 'C' flag. */
    if (line != NULL && do_concat && vim_strchr(p_cpo, CPO_CONCAT) == NULL)
    {
	/* compensate for the one line read-ahead */
	--sp->sourcing_lnum;

	// Get the next line and concatenate it when it starts with a
	// backslash. We always need to read the next line, keep it in
	// sp->nextline.
	/* Also check for a comment in between continuation lines: "\ */
	sp->nextline = get_one_sourceline(sp);
	if (sp->nextline != NULL
		&& (*(p = skipwhite(sp->nextline)) == '\\'
			      || (p[0] == '"' && p[1] == '\\' && p[2] == ' ')))
	{
	    garray_T    ga;

	    ga_init2(&ga, (int)sizeof(char_u), 400);
	    ga_concat(&ga, line);
	    if (*p == '\\')
		ga_concat(&ga, p + 1);
	    for (;;)
	    {
		vim_free(sp->nextline);
		sp->nextline = get_one_sourceline(sp);
		if (sp->nextline == NULL)
		    break;
		p = skipwhite(sp->nextline);
		if (*p == '\\')
		{
		    // Adjust the growsize to the current length to speed up
		    // concatenating many lines.
		    if (ga.ga_len > 400)
		    {
			if (ga.ga_len > 8000)
			    ga.ga_growsize = 8000;
			else
			    ga.ga_growsize = ga.ga_len;
		    }
		    ga_concat(&ga, p + 1);
		}
		else if (p[0] != '"' || p[1] != '\\' || p[2] != ' ')
		    break;
	    }
	    ga_append(&ga, NUL);
	    vim_free(line);
	    line = ga.ga_data;
	}
    }

    if (line != NULL && sp->conv.vc_type != CONV_NONE)
    {
	char_u	*s;

	/* Convert the encoding of the script line. */
	s = string_convert(&sp->conv, line, NULL);
	if (s != NULL)
	{
	    vim_free(line);
	    line = s;
	}
    }

#ifdef FEAT_EVAL
    /* Did we encounter a breakpoint? */
    if (sp->breakpoint != 0 && sp->breakpoint <= sourcing_lnum)
    {
	dbg_breakpoint(sp->fname, sourcing_lnum);
	/* Find next breakpoint. */
	sp->breakpoint = dbg_find_breakpoint(TRUE, sp->fname, sourcing_lnum);
	sp->dbg_tick = debug_tick;
    }
#endif

    return line;
}

    static char_u *
get_one_sourceline(struct source_cookie *sp)
{
    garray_T		ga;
    int			len;
    int			c;
    char_u		*buf;
#ifdef USE_CRNL
    int			has_cr;		/* CR-LF found */
#endif
    int			have_read = FALSE;

    /* use a growarray to store the sourced line */
    ga_init2(&ga, 1, 250);

    /*
     * Loop until there is a finished line (or end-of-file).
     */
    ++sp->sourcing_lnum;
    for (;;)
    {
	/* make room to read at least 120 (more) characters */
	if (ga_grow(&ga, 120) == FAIL)
	    break;
	buf = (char_u *)ga.ga_data;

	if (fgets((char *)buf + ga.ga_len, ga.ga_maxlen - ga.ga_len,
							      sp->fp) == NULL)
	    break;
	len = ga.ga_len + (int)STRLEN(buf + ga.ga_len);
#ifdef USE_CRNL
	/* Ignore a trailing CTRL-Z, when in Dos mode.	Only recognize the
	 * CTRL-Z by its own, or after a NL. */
	if (	   (len == 1 || (len >= 2 && buf[len - 2] == '\n'))
		&& sp->fileformat == EOL_DOS
		&& buf[len - 1] == Ctrl_Z)
	{
	    buf[len - 1] = NUL;
	    break;
	}
#endif

	have_read = TRUE;
	ga.ga_len = len;

	/* If the line was longer than the buffer, read more. */
	if (ga.ga_maxlen - ga.ga_len == 1 && buf[len - 1] != '\n')
	    continue;

	if (len >= 1 && buf[len - 1] == '\n')	/* remove trailing NL */
	{
#ifdef USE_CRNL
	    has_cr = (len >= 2 && buf[len - 2] == '\r');
	    if (sp->fileformat == EOL_UNKNOWN)
	    {
		if (has_cr)
		    sp->fileformat = EOL_DOS;
		else
		    sp->fileformat = EOL_UNIX;
	    }

	    if (sp->fileformat == EOL_DOS)
	    {
		if (has_cr)	    /* replace trailing CR */
		{
		    buf[len - 2] = '\n';
		    --len;
		    --ga.ga_len;
		}
		else	    /* lines like ":map xx yy^M" will have failed */
		{
		    if (!sp->error)
		    {
			msg_source(HL_ATTR(HLF_W));
			emsg(_("W15: Warning: Wrong line separator, ^M may be missing"));
		    }
		    sp->error = TRUE;
		    sp->fileformat = EOL_UNIX;
		}
	    }
#endif
	    /* The '\n' is escaped if there is an odd number of ^V's just
	     * before it, first set "c" just before the 'V's and then check
	     * len&c parities (is faster than ((len-c)%2 == 0)) -- Acevedo */
	    for (c = len - 2; c >= 0 && buf[c] == Ctrl_V; c--)
		;
	    if ((len & 1) != (c & 1))	/* escaped NL, read more */
	    {
		++sp->sourcing_lnum;
		continue;
	    }

	    buf[len - 1] = NUL;		/* remove the NL */
	}

	/*
	 * Check for ^C here now and then, so recursive :so can be broken.
	 */
	line_breakcheck();
	break;
    }

    if (have_read)
	return (char_u *)ga.ga_data;

    vim_free(ga.ga_data);
    return NULL;
}

/*
 * ":scriptencoding": Set encoding conversion for a sourced script.
 */
    void
ex_scriptencoding(exarg_T *eap)
{
    struct source_cookie	*sp;
    char_u			*name;

    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_("E167: :scriptencoding used outside of a sourced file"));
	return;
    }

    if (*eap->arg != NUL)
    {
	name = enc_canonize(eap->arg);
	if (name == NULL)	/* out of memory */
	    return;
    }
    else
	name = eap->arg;

    /* Setup for conversion from the specified encoding to 'encoding'. */
    sp = (struct source_cookie *)getline_cookie(eap->getline, eap->cookie);
    convert_setup(&sp->conv, name, p_enc);

    if (name != eap->arg)
	vim_free(name);
}

/*
 * ":scriptversion": Set Vim script version for a sourced script.
 */
    void
ex_scriptversion(exarg_T *eap UNUSED)
{
#ifdef FEAT_EVAL
    int		nr;

    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_("E984: :scriptversion used outside of a sourced file"));
	return;
    }

    nr = getdigits(&eap->arg);
    if (nr == 0 || *eap->arg != NUL)
	emsg(_(e_invarg));
    else if (nr > 3)
	semsg(_("E999: scriptversion not supported: %d"), nr);
    else
	current_sctx.sc_version = nr;
#endif
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * ":finish": Mark a sourced file as finished.
 */
    void
ex_finish(exarg_T *eap)
{
    if (getline_equal(eap->getline, eap->cookie, getsourceline))
	do_finish(eap, FALSE);
    else
	emsg(_("E168: :finish used outside of a sourced file"));
}

/*
 * Mark a sourced file as finished.  Possibly makes the ":finish" pending.
 * Also called for a pending finish at the ":endtry" or after returning from
 * an extra do_cmdline().  "reanimate" is used in the latter case.
 */
    void
do_finish(exarg_T *eap, int reanimate)
{
    int		idx;

    if (reanimate)
	((struct source_cookie *)getline_cookie(eap->getline,
					      eap->cookie))->finished = FALSE;

    /*
     * Cleanup (and inactivate) conditionals, but stop when a try conditional
     * not in its finally clause (which then is to be executed next) is found.
     * In this case, make the ":finish" pending for execution at the ":endtry".
     * Otherwise, finish normally.
     */
    idx = cleanup_conditionals(eap->cstack, 0, TRUE);
    if (idx >= 0)
    {
	eap->cstack->cs_pending[idx] = CSTP_FINISH;
	report_make_pending(CSTP_FINISH, NULL);
    }
    else
	((struct source_cookie *)getline_cookie(eap->getline,
					       eap->cookie))->finished = TRUE;
}


/*
 * Return TRUE when a sourced file had the ":finish" command: Don't give error
 * message for missing ":endif".
 * Return FALSE when not sourcing a file.
 */
    int
source_finished(
    char_u	*(*fgetline)(int, void *, int, int),
    void	*cookie)
{
    return (getline_equal(fgetline, cookie, getsourceline)
	    && ((struct source_cookie *)getline_cookie(
						fgetline, cookie))->finished);
}
#endif

/*
 * ":checktime [buffer]"
 */
    void
ex_checktime(exarg_T *eap)
{
    buf_T	*buf;
    int		save_no_check_timestamps = no_check_timestamps;

    no_check_timestamps = 0;
    if (eap->addr_count == 0)	/* default is all buffers */
	check_timestamps(FALSE);
    else
    {
	buf = buflist_findnr((int)eap->line2);
	if (buf != NULL)	/* cannot happen? */
	    (void)buf_check_timestamp(buf, FALSE);
    }
    no_check_timestamps = save_no_check_timestamps;
}

#if (defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
	&& (defined(FEAT_EVAL) || defined(FEAT_MULTI_LANG))
# define HAVE_GET_LOCALE_VAL
    static char_u *
get_locale_val(int what)
{
    char_u	*loc;

    /* Obtain the locale value from the libraries. */
    loc = (char_u *)setlocale(what, NULL);

# ifdef MSWIN
    if (loc != NULL)
    {
	char_u	*p;

	/* setocale() returns something like "LC_COLLATE=<name>;LC_..." when
	 * one of the values (e.g., LC_CTYPE) differs. */
	p = vim_strchr(loc, '=');
	if (p != NULL)
	{
	    loc = ++p;
	    while (*p != NUL)	/* remove trailing newline */
	    {
		if (*p < ' ' || *p == ';')
		{
		    *p = NUL;
		    break;
		}
		++p;
	    }
	}
    }
# endif

    return loc;
}
#endif


#ifdef MSWIN
/*
 * On MS-Windows locale names are strings like "German_Germany.1252", but
 * gettext expects "de".  Try to translate one into another here for a few
 * supported languages.
 */
    static char_u *
gettext_lang(char_u *name)
{
    int		i;
    static char *(mtable[]) = {
			"afrikaans",	"af",
			"czech",	"cs",
			"dutch",	"nl",
			"german",	"de",
			"english_united kingdom", "en_GB",
			"spanish",	"es",
			"french",	"fr",
			"italian",	"it",
			"japanese",	"ja",
			"korean",	"ko",
			"norwegian",	"no",
			"polish",	"pl",
			"russian",	"ru",
			"slovak",	"sk",
			"swedish",	"sv",
			"ukrainian",	"uk",
			"chinese_china", "zh_CN",
			"chinese_taiwan", "zh_TW",
			NULL};

    for (i = 0; mtable[i] != NULL; i += 2)
	if (STRNICMP(mtable[i], name, STRLEN(mtable[i])) == 0)
	    return (char_u *)mtable[i + 1];
    return name;
}
#endif

#if defined(FEAT_MULTI_LANG) || defined(PROTO)
/*
 * Return TRUE when "lang" starts with a valid language name.
 * Rejects NULL, empty string, "C", "C.UTF-8" and others.
 */
    static int
is_valid_mess_lang(char_u *lang)
{
    return lang != NULL && ASCII_ISALPHA(lang[0]) && ASCII_ISALPHA(lang[1]);
}

/*
 * Obtain the current messages language.  Used to set the default for
 * 'helplang'.  May return NULL or an empty string.
 */
    char_u *
get_mess_lang(void)
{
    char_u *p;

# ifdef HAVE_GET_LOCALE_VAL
#  if defined(LC_MESSAGES)
    p = get_locale_val(LC_MESSAGES);
#  else
    /* This is necessary for Win32, where LC_MESSAGES is not defined and $LANG
     * may be set to the LCID number.  LC_COLLATE is the best guess, LC_TIME
     * and LC_MONETARY may be set differently for a Japanese working in the
     * US. */
    p = get_locale_val(LC_COLLATE);
#  endif
# else
    p = mch_getenv((char_u *)"LC_ALL");
    if (!is_valid_mess_lang(p))
    {
	p = mch_getenv((char_u *)"LC_MESSAGES");
	if (!is_valid_mess_lang(p))
	    p = mch_getenv((char_u *)"LANG");
    }
# endif
# ifdef MSWIN
    p = gettext_lang(p);
# endif
    return is_valid_mess_lang(p) ? p : NULL;
}
#endif

/* Complicated #if; matches with where get_mess_env() is used below. */
#if (defined(FEAT_EVAL) && !((defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
	    && defined(LC_MESSAGES))) \
	|| ((defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
		&& !defined(LC_MESSAGES))
/*
 * Get the language used for messages from the environment.
 */
    static char_u *
get_mess_env(void)
{
    char_u	*p;

    p = mch_getenv((char_u *)"LC_ALL");
    if (p == NULL || *p == NUL)
    {
	p = mch_getenv((char_u *)"LC_MESSAGES");
	if (p == NULL || *p == NUL)
	{
	    p = mch_getenv((char_u *)"LANG");
	    if (p != NULL && VIM_ISDIGIT(*p))
		p = NULL;		/* ignore something like "1043" */
# ifdef HAVE_GET_LOCALE_VAL
	    if (p == NULL || *p == NUL)
		p = get_locale_val(LC_CTYPE);
# endif
	}
    }
    return p;
}
#endif

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Set the "v:lang" variable according to the current locale setting.
 * Also do "v:lc_time"and "v:ctype".
 */
    void
set_lang_var(void)
{
    char_u	*loc;

# ifdef HAVE_GET_LOCALE_VAL
    loc = get_locale_val(LC_CTYPE);
# else
    /* setlocale() not supported: use the default value */
    loc = (char_u *)"C";
# endif
    set_vim_var_string(VV_CTYPE, loc, -1);

    /* When LC_MESSAGES isn't defined use the value from $LC_MESSAGES, fall
     * back to LC_CTYPE if it's empty. */
# if defined(HAVE_GET_LOCALE_VAL) && defined(LC_MESSAGES)
    loc = get_locale_val(LC_MESSAGES);
# else
    loc = get_mess_env();
# endif
    set_vim_var_string(VV_LANG, loc, -1);

# ifdef HAVE_GET_LOCALE_VAL
    loc = get_locale_val(LC_TIME);
# endif
    set_vim_var_string(VV_LC_TIME, loc, -1);
}
#endif

#if defined(HAVE_LOCALE_H) || defined(X_LOCALE) \
/*
 * ":language":  Set the language (locale).
 */
    void
ex_language(exarg_T *eap)
{
    char	*loc;
    char_u	*p;
    char_u	*name;
    int		what = LC_ALL;
    char	*whatstr = "";
#ifdef LC_MESSAGES
# define VIM_LC_MESSAGES LC_MESSAGES
#else
# define VIM_LC_MESSAGES 6789
#endif

    name = eap->arg;

    /* Check for "messages {name}", "ctype {name}" or "time {name}" argument.
     * Allow abbreviation, but require at least 3 characters to avoid
     * confusion with a two letter language name "me" or "ct". */
    p = skiptowhite(eap->arg);
    if ((*p == NUL || VIM_ISWHITE(*p)) && p - eap->arg >= 3)
    {
	if (STRNICMP(eap->arg, "messages", p - eap->arg) == 0)
	{
	    what = VIM_LC_MESSAGES;
	    name = skipwhite(p);
	    whatstr = "messages ";
	}
	else if (STRNICMP(eap->arg, "ctype", p - eap->arg) == 0)
	{
	    what = LC_CTYPE;
	    name = skipwhite(p);
	    whatstr = "ctype ";
	}
	else if (STRNICMP(eap->arg, "time", p - eap->arg) == 0)
	{
	    what = LC_TIME;
	    name = skipwhite(p);
	    whatstr = "time ";
	}
    }

    if (*name == NUL)
    {
#ifndef LC_MESSAGES
	if (what == VIM_LC_MESSAGES)
	    p = get_mess_env();
	else
#endif
	    p = (char_u *)setlocale(what, NULL);
	if (p == NULL || *p == NUL)
	    p = (char_u *)"Unknown";
	smsg(_("Current %slanguage: \"%s\""), whatstr, p);
    }
    else
    {
#ifndef LC_MESSAGES
	if (what == VIM_LC_MESSAGES)
	    loc = "";
	else
#endif
	{
	    loc = setlocale(what, (char *)name);
#if defined(FEAT_FLOAT) && defined(LC_NUMERIC)
	    /* Make sure strtod() uses a decimal point, not a comma. */
	    setlocale(LC_NUMERIC, "C");
#endif
	}
	if (loc == NULL)
	    semsg(_("E197: Cannot set language to \"%s\""), name);
	else
	{
#ifdef HAVE_NL_MSG_CAT_CNTR
	    /* Need to do this for GNU gettext, otherwise cached translations
	     * will be used again. */
	    extern int _nl_msg_cat_cntr;

	    ++_nl_msg_cat_cntr;
#endif
	    /* Reset $LC_ALL, otherwise it would overrule everything. */
	    vim_setenv((char_u *)"LC_ALL", (char_u *)"");

	    if (what != LC_TIME)
	    {
		/* Tell gettext() what to translate to.  It apparently doesn't
		 * use the currently effective locale.  Also do this when
		 * FEAT_GETTEXT isn't defined, so that shell commands use this
		 * value. */
		if (what == LC_ALL)
		{
		    vim_setenv((char_u *)"LANG", name);

		    /* Clear $LANGUAGE because GNU gettext uses it. */
		    vim_setenv((char_u *)"LANGUAGE", (char_u *)"");
# ifdef MSWIN
		    /* Apparently MS-Windows printf() may cause a crash when
		     * we give it 8-bit text while it's expecting text in the
		     * current locale.  This call avoids that. */
		    setlocale(LC_CTYPE, "C");
# endif
		}
		if (what != LC_CTYPE)
		{
		    char_u	*mname;
#ifdef MSWIN
		    mname = gettext_lang(name);
#else
		    mname = name;
#endif
		    vim_setenv((char_u *)"LC_MESSAGES", mname);
#ifdef FEAT_MULTI_LANG
		    set_helplang_default(mname);
#endif
		}
	    }

# ifdef FEAT_EVAL
	    /* Set v:lang, v:lc_time and v:ctype to the final result. */
	    set_lang_var();
# endif
# ifdef FEAT_TITLE
	    maketitle();
# endif
	}
    }
}

static char_u	**locales = NULL;	/* Array of all available locales */

# ifndef MSWIN
static int	did_init_locales = FALSE;

/*
 * Return an array of strings for all available locales + NULL for the
 * last element.  Return NULL in case of error.
 */
    static char_u **
find_locales(void)
{
    garray_T	locales_ga;
    char_u	*loc;

    /* Find all available locales by running command "locale -a".  If this
     * doesn't work we won't have completion. */
    char_u *locale_a = get_cmd_output((char_u *)"locale -a",
						    NULL, SHELL_SILENT, NULL);
    if (locale_a == NULL)
	return NULL;
    ga_init2(&locales_ga, sizeof(char_u *), 20);

    /* Transform locale_a string where each locale is separated by "\n"
     * into an array of locale strings. */
    loc = (char_u *)strtok((char *)locale_a, "\n");

    while (loc != NULL)
    {
	if (ga_grow(&locales_ga, 1) == FAIL)
	    break;
	loc = vim_strsave(loc);
	if (loc == NULL)
	    break;

	((char_u **)locales_ga.ga_data)[locales_ga.ga_len++] = loc;
	loc = (char_u *)strtok(NULL, "\n");
    }
    vim_free(locale_a);
    if (ga_grow(&locales_ga, 1) == FAIL)
    {
	ga_clear(&locales_ga);
	return NULL;
    }
    ((char_u **)locales_ga.ga_data)[locales_ga.ga_len] = NULL;
    return (char_u **)locales_ga.ga_data;
}
# endif

/*
 * Lazy initialization of all available locales.
 */
    static void
init_locales(void)
{
#  ifndef MSWIN
    if (!did_init_locales)
    {
	did_init_locales = TRUE;
	locales = find_locales();
    }
#  endif
}

#  if defined(EXITFREE) || defined(PROTO)
    void
free_locales(void)
{
    int			i;
    if (locales != NULL)
    {
	for (i = 0; locales[i] != NULL; i++)
	    vim_free(locales[i]);
	VIM_CLEAR(locales);
    }
}
#  endif

/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":language" command.
 */
    char_u *
get_lang_arg(expand_T *xp UNUSED, int idx)
{
    if (idx == 0)
	return (char_u *)"messages";
    if (idx == 1)
	return (char_u *)"ctype";
    if (idx == 2)
	return (char_u *)"time";

    init_locales();
    if (locales == NULL)
	return NULL;
    return locales[idx - 3];
}

/*
 * Function given to ExpandGeneric() to obtain the available locales.
 */
    char_u *
get_locales(expand_T *xp UNUSED, int idx)
{
    init_locales();
    if (locales == NULL)
	return NULL;
    return locales[idx];
}

#endif
