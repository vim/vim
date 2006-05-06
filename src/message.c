/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * message.c: functions for displaying messages on the command line
 */

#define MESSAGE_FILE		/* don't include prototype for smsg() */

#include "vim.h"

static int other_sourcing_name __ARGS((void));
static char_u *get_emsg_source __ARGS((void));
static char_u *get_emsg_lnum __ARGS((void));
static void add_msg_hist __ARGS((char_u *s, int len, int attr));
static void hit_return_msg __ARGS((void));
static void msg_home_replace_attr __ARGS((char_u *fname, int attr));
#ifdef FEAT_MBYTE
static char_u *screen_puts_mbyte __ARGS((char_u *s, int l, int attr));
#endif
static void msg_puts_attr_len __ARGS((char_u *str, int maxlen, int attr));
static void msg_puts_display __ARGS((char_u *str, int maxlen, int attr, int recurse));
static void msg_scroll_up __ARGS((void));
static void inc_msg_scrolled __ARGS((void));
static void store_sb_text __ARGS((char_u **sb_str, char_u *s, int attr, int *sb_col, int finish));
static void t_puts __ARGS((int *t_col, char_u *t_s, char_u *s, int attr));
static void msg_puts_printf __ARGS((char_u *str, int maxlen));
static int do_more_prompt __ARGS((int typed_char));
static void msg_screen_putchar __ARGS((int c, int attr));
static int  msg_check_screen __ARGS((void));
static void redir_write __ARGS((char_u *s, int maxlen));
static void verbose_write __ARGS((char_u *s, int maxlen));
#ifdef FEAT_CON_DIALOG
static char_u *msg_show_console_dialog __ARGS((char_u *message, char_u *buttons, int dfltbutton));
static int	confirm_msg_used = FALSE;	/* displaying confirm_msg */
static char_u	*confirm_msg = NULL;		/* ":confirm" message */
static char_u	*confirm_msg_tail;		/* tail of confirm_msg */
#endif

struct msg_hist
{
    struct msg_hist	*next;
    char_u		*msg;
    int			attr;
};

static struct msg_hist *first_msg_hist = NULL;
static struct msg_hist *last_msg_hist = NULL;
static int msg_hist_len = 0;
static int msg_hist_off = FALSE;	/* don't add messages to history */

/*
 * When writing messages to the screen, there are many different situations.
 * A number of variables is used to remember the current state:
 * msg_didany	    TRUE when messages were written since the last time the
 *		    user reacted to a prompt.
 *		    Reset: After hitting a key for the hit-return prompt,
 *		    hitting <CR> for the command line or input().
 *		    Set: When any message is written to the screen.
 * msg_didout	    TRUE when something was written to the current line.
 *		    Reset: When advancing to the next line, when the current
 *		    text can be overwritten.
 *		    Set: When any message is written to the screen.
 * msg_nowait	    No extra delay for the last drawn message.
 *		    Used in normal_cmd() before the mode message is drawn.
 * emsg_on_display  There was an error message recently.  Indicates that there
 *		    should be a delay before redrawing.
 * msg_scroll	    The next message should not overwrite the current one.
 * msg_scrolled	    How many lines the screen has been scrolled (because of
 *		    messages).  Used in update_screen() to scroll the screen
 *		    back.  Incremented each time the screen scrolls a line.
 * msg_scrolled_ign  TRUE when msg_scrolled is non-zero and msg_puts_attr()
 *		    writes something without scrolling should not make
 *		    need_wait_return to be set.  This is a hack to make ":ts"
 *		    work without an extra prompt.
 * lines_left	    Number of lines available for messages before the
 *		    more-prompt is to be given.
 * need_wait_return TRUE when the hit-return prompt is needed.
 *		    Reset: After giving the hit-return prompt, when the user
 *		    has answered some other prompt.
 *		    Set: When the ruler or typeahead display is overwritten,
 *		    scrolling the screen for some message.
 * keep_msg	    Message to be displayed after redrawing the screen, in
 *		    main_loop().
 *		    This is an allocated string or NULL when not used.
 */

/*
 * msg(s) - displays the string 's' on the status line
 * When terminal not initialized (yet) mch_errmsg(..) is used.
 * return TRUE if wait_return not called
 */
    int
msg(s)
    char_u	*s;
{
    return msg_attr_keep(s, 0, FALSE);
}

#if defined(FEAT_EVAL) || defined(FEAT_X11) || defined(USE_XSMP) \
    || defined(PROTO)
/*
 * Like msg() but keep it silent when 'verbosefile' is set.
 */
    int
verb_msg(s)
    char_u	*s;
{
    int		n;

    verbose_enter();
    n = msg_attr_keep(s, 0, FALSE);
    verbose_leave();

    return n;
}
#endif

    int
msg_attr(s, attr)
    char_u	*s;
    int		attr;
{
    return msg_attr_keep(s, attr, FALSE);
}

    int
msg_attr_keep(s, attr, keep)
    char_u	*s;
    int		attr;
    int		keep;	    /* TRUE: set keep_msg if it doesn't scroll */
{
    static int	entered = 0;
    int		retval;
    char_u	*buf = NULL;

#ifdef FEAT_EVAL
    if (attr == 0)
	set_vim_var_string(VV_STATUSMSG, s, -1);
#endif

    /*
     * It is possible that displaying a messages causes a problem (e.g.,
     * when redrawing the window), which causes another message, etc..	To
     * break this loop, limit the recursiveness to 3 levels.
     */
    if (entered >= 3)
	return TRUE;
    ++entered;

    /* Add message to history (unless it's a repeated kept message or a
     * truncated message) */
    if (s != keep_msg
	    || (*s != '<'
		&& last_msg_hist != NULL
		&& last_msg_hist->msg != NULL
		&& STRCMP(s, last_msg_hist->msg)))
	add_msg_hist(s, -1, attr);

    /* When displaying keep_msg, don't let msg_start() free it, caller must do
     * that. */
    if (s == keep_msg)
	keep_msg = NULL;

    /* Truncate the message if needed. */
    msg_start();
    buf = msg_strtrunc(s, FALSE);
    if (buf != NULL)
	s = buf;

    msg_outtrans_attr(s, attr);
    msg_clr_eos();
    retval = msg_end();

    if (keep && retval && vim_strsize(s) < (int)(Rows - cmdline_row - 1)
							   * Columns + sc_col)
	set_keep_msg(s, 0);

    vim_free(buf);
    --entered;
    return retval;
}

/*
 * Truncate a string such that it can be printed without causing a scroll.
 * Returns an allocated string or NULL when no truncating is done.
 */
    char_u *
msg_strtrunc(s, force)
    char_u	*s;
    int		force;	    /* always truncate */
{
    char_u	*buf = NULL;
    int		len;
    int		room;

    /* May truncate message to avoid a hit-return prompt */
    if ((!msg_scroll && !need_wait_return && shortmess(SHM_TRUNCALL)
			       && !exmode_active && msg_silent == 0) || force)
    {
	len = vim_strsize(s);
	if (msg_scrolled != 0)
	    /* Use all the columns. */
	    room = (int)(Rows - msg_row) * Columns - 1;
	else
	    /* Use up to 'showcmd' column. */
	    room = (int)(Rows - msg_row - 1) * Columns + sc_col - 1;
	if (len > room && room > 0)
	{
#ifdef FEAT_MBYTE
	    if (enc_utf8)
		/* may have up to 18 bytes per cell (6 per char, up to two
		 * composing chars) */
		buf = alloc((room + 2) * 18);
	    else if (enc_dbcs == DBCS_JPNU)
		/* may have up to 2 bytes per cell for euc-jp */
		buf = alloc((room + 2) * 2);
	    else
#endif
		buf = alloc(room + 2);
	    if (buf != NULL)
		trunc_string(s, buf, room);
	}
    }
    return buf;
}

/*
 * Truncate a string "s" to "buf" with cell width "room".
 * "s" and "buf" may be equal.
 */
    void
trunc_string(s, buf, room)
    char_u	*s;
    char_u	*buf;
    int		room;
{
    int		half;
    int		len;
    int		e;
    int		i;
    int		n;

    room -= 3;
    half = room / 2;
    len = 0;

    /* First part: Start of the string. */
    for (e = 0; len < half; ++e)
    {
	if (s[e] == NUL)
	{
	    /* text fits without truncating! */
	    buf[e] = NUL;
	    return;
	}
	n = ptr2cells(s + e);
	if (len + n >= half)
	    break;
	len += n;
	buf[e] = s[e];
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    for (n = (*mb_ptr2len)(s + e); --n > 0; )
	    {
		++e;
		buf[e] = s[e];
	    }
#endif
    }

    /* Last part: End of the string. */
    i = e;
#ifdef FEAT_MBYTE
    if (enc_dbcs != 0)
    {
	/* For DBCS going backwards in a string is slow, but
	 * computing the cell width isn't too slow: go forward
	 * until the rest fits. */
	n = vim_strsize(s + i);
	while (len + n > room)
	{
	    n -= ptr2cells(s + i);
	    i += (*mb_ptr2len)(s + i);
	}
    }
    else if (enc_utf8)
    {
	/* For UTF-8 we can go backwards easily. */
	half = i = (int)STRLEN(s);
	for (;;)
	{
	    do
		half = half - (*mb_head_off)(s, s + half - 1) - 1;
	    while (utf_iscomposing(utf_ptr2char(s + half)) && half > 0);
	    n = ptr2cells(s + half);
	    if (len + n > room)
		break;
	    len += n;
	    i = half;
	}
    }
    else
#endif
    {
	for (i = (int)STRLEN(s); len + (n = ptr2cells(s + i - 1)) <= room; --i)
	    len += n;
    }

    /* Set the middle and copy the last part. */
    mch_memmove(buf + e, "...", (size_t)3);
    mch_memmove(buf + e + 3, s + i, STRLEN(s + i) + 1);
}

/*
 * Automatic prototype generation does not understand this function.
 * Note: Caller of smgs() and smsg_attr() must check the resulting string is
 * shorter than IOSIZE!!!
 */
#ifndef PROTO
# ifndef HAVE_STDARG_H

int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg __ARGS((char_u *, long, long, long,
			long, long, long, long, long, long, long));
int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg_attr __ARGS((int, char_u *, long, long, long,
			long, long, long, long, long, long, long));

int vim_snprintf __ARGS((char *, size_t, char *, long, long, long,
				   long, long, long, long, long, long, long));

/*
 * smsg(str, arg, ...) is like using sprintf(buf, str, arg, ...) and then
 * calling msg(buf).
 * The buffer used is IObuff, the message is truncated at IOSIZE.
 */

/* VARARGS */
    int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg(s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
    char_u	*s;
    long	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
    return smsg_attr(0, s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

/* VARARGS */
    int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg_attr(attr, s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
    int		attr;
    char_u	*s;
    long	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
    vim_snprintf((char *)IObuff, IOSIZE, (char *)s,
				     a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    return msg_attr(IObuff, attr);
}

# else /* HAVE_STDARG_H */

int vim_snprintf(char *str, size_t str_m, char *fmt, ...);

    int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg(char_u *s, ...)
{
    va_list arglist;

    va_start(arglist, s);
    vim_vsnprintf((char *)IObuff, IOSIZE, (char *)s, arglist, NULL);
    va_end(arglist);
    return msg(IObuff);
}

    int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg_attr(int attr, char_u *s, ...)
{
    va_list arglist;

    va_start(arglist, s);
    vim_vsnprintf((char *)IObuff, IOSIZE, (char *)s, arglist, NULL);
    va_end(arglist);
    return msg_attr(IObuff, attr);
}

# endif /* HAVE_STDARG_H */
#endif

/*
 * Remember the last sourcing name/lnum used in an error message, so that it
 * isn't printed each time when it didn't change.
 */
static int	last_sourcing_lnum = 0;
static char_u   *last_sourcing_name = NULL;

/*
 * Reset the last used sourcing name/lnum.  Makes sure it is displayed again
 * for the next error message;
 */
    void
reset_last_sourcing()
{
    vim_free(last_sourcing_name);
    last_sourcing_name = NULL;
    last_sourcing_lnum = 0;
}

/*
 * Return TRUE if "sourcing_name" differs from "last_sourcing_name".
 */
    static int
other_sourcing_name()
{
    if (sourcing_name != NULL)
    {
	if (last_sourcing_name != NULL)
	    return STRCMP(sourcing_name, last_sourcing_name) != 0;
	return TRUE;
    }
    return FALSE;
}

/*
 * Get the message about the source, as used for an error message.
 * Returns an allocated string with room for one more character.
 * Returns NULL when no message is to be given.
 */
    static char_u *
get_emsg_source()
{
    char_u	*Buf, *p;

    if (sourcing_name != NULL && other_sourcing_name())
    {
	p = (char_u *)_("Error detected while processing %s:");
	Buf = alloc((unsigned)(STRLEN(sourcing_name) + STRLEN(p)));
	if (Buf != NULL)
	    sprintf((char *)Buf, (char *)p, sourcing_name);
	return Buf;
    }
    return NULL;
}

/*
 * Get the message about the source lnum, as used for an error message.
 * Returns an allocated string with room for one more character.
 * Returns NULL when no message is to be given.
 */
    static char_u *
get_emsg_lnum()
{
    char_u	*Buf, *p;

    /* lnum is 0 when executing a command from the command line
     * argument, we don't want a line number then */
    if (sourcing_name != NULL
	    && (other_sourcing_name() || sourcing_lnum != last_sourcing_lnum)
	    && sourcing_lnum != 0)
    {
	p = (char_u *)_("line %4ld:");
	Buf = alloc((unsigned)(STRLEN(p) + 20));
	if (Buf != NULL)
	    sprintf((char *)Buf, (char *)p, (long)sourcing_lnum);
	return Buf;
    }
    return NULL;
}

/*
 * Display name and line number for the source of an error.
 * Remember the file name and line number, so that for the next error the info
 * is only displayed if it changed.
 */
    void
msg_source(attr)
    int		attr;
{
    char_u	*p;

    ++no_wait_return;
    p = get_emsg_source();
    if (p != NULL)
    {
	msg_attr(p, attr);
	vim_free(p);
    }
    p = get_emsg_lnum();
    if (p != NULL)
    {
	msg_attr(p, hl_attr(HLF_N));
	vim_free(p);
	last_sourcing_lnum = sourcing_lnum;  /* only once for each line */
    }

    /* remember the last sourcing name printed, also when it's empty */
    if (sourcing_name == NULL || other_sourcing_name())
    {
	vim_free(last_sourcing_name);
	if (sourcing_name == NULL)
	    last_sourcing_name = NULL;
	else
	    last_sourcing_name = vim_strsave(sourcing_name);
    }
    --no_wait_return;
}

/*
 * Return TRUE if not giving error messages right now:
 * If "emsg_off" is set: no error messages at the moment.
 * If "msg" is in 'debug': do error message but without side effects.
 * If "emsg_skip" is set: never do error messages.
 */
    int
emsg_not_now()
{
    if ((emsg_off > 0 && vim_strchr(p_debug, 'm') == NULL
					  && vim_strchr(p_debug, 't') == NULL)
#ifdef FEAT_EVAL
	    || emsg_skip > 0
#endif
	    )
	return TRUE;
    return FALSE;
}

/*
 * emsg() - display an error message
 *
 * Rings the bell, if appropriate, and calls message() to do the real work
 * When terminal not initialized (yet) mch_errmsg(..) is used.
 *
 * return TRUE if wait_return not called
 */
    int
emsg(s)
    char_u	*s;
{
    int		attr;
    char_u	*p;
#ifdef FEAT_EVAL
    int		ignore = FALSE;
    int		severe;
#endif

    called_emsg = TRUE;
    ex_exitval = 1;

    /*
     * If "emsg_severe" is TRUE: When an error exception is to be thrown,
     * prefer this message over previous messages for the same command.
     */
#ifdef FEAT_EVAL
    severe = emsg_severe;
    emsg_severe = FALSE;
#endif

    /* Skip this if not giving error messages at the moment. */
    if (emsg_not_now())
	return TRUE;

    if (!emsg_off || vim_strchr(p_debug, 't') != NULL)
    {
#ifdef FEAT_EVAL
	/*
	 * Cause a throw of an error exception if appropriate.  Don't display
	 * the error message in this case.  (If no matching catch clause will
	 * be found, the message will be displayed later on.)  "ignore" is set
	 * when the message should be ignored completely (used for the
	 * interrupt message).
	 */
	if (cause_errthrow(s, severe, &ignore) == TRUE)
	{
	    if (!ignore)
		did_emsg = TRUE;
	    return TRUE;
	}

	/* set "v:errmsg", also when using ":silent! cmd" */
	set_vim_var_string(VV_ERRMSG, s, -1);
#endif

	/*
	 * When using ":silent! cmd" ignore error messsages.
	 * But do write it to the redirection file.
	 */
	if (emsg_silent != 0)
	{
	    msg_start();
	    p = get_emsg_source();
	    if (p != NULL)
	    {
		STRCAT(p, "\n");
		redir_write(p, -1);
		vim_free(p);
	    }
	    p = get_emsg_lnum();
	    if (p != NULL)
	    {
		STRCAT(p, "\n");
		redir_write(p, -1);
		vim_free(p);
	    }
	    redir_write(s, -1);
	    return TRUE;
	}

	/* Reset msg_silent, an error causes messages to be switched back on. */
	msg_silent = 0;
	cmd_silent = FALSE;

	if (global_busy)		/* break :global command */
	    ++global_busy;

	if (p_eb)
	    beep_flush();		/* also includes flush_buffers() */
	else
	    flush_buffers(FALSE);	/* flush internal buffers */
	did_emsg = TRUE;		/* flag for DoOneCmd() */
    }

    emsg_on_display = TRUE;	/* remember there is an error message */
    ++msg_scroll;		/* don't overwrite a previous message */
    attr = hl_attr(HLF_E);	/* set highlight mode for error messages */
    if (msg_scrolled != 0)
	need_wait_return = TRUE;    /* needed in case emsg() is called after
				     * wait_return has reset need_wait_return
				     * and a redraw is expected because
				     * msg_scrolled is non-zero */

    /*
     * Display name and line number for the source of the error.
     */
    msg_source(attr);

    /*
     * Display the error message itself.
     */
    msg_nowait = FALSE;			/* wait for this msg */
    return msg_attr(s, attr);
}

/*
 * Print an error message with one "%s" and one string argument.
 */
    int
emsg2(s, a1)
    char_u *s, *a1;
{
    return emsg3(s, a1, NULL);
}

/* emsg3() and emsgn() are in misc2.c to avoid warnings for the prototypes. */

    void
emsg_invreg(name)
    int	    name;
{
    EMSG2(_("E354: Invalid register name: '%s'"), transchar(name));
}

/*
 * Like msg(), but truncate to a single line if p_shm contains 't', or when
 * "force" is TRUE.  This truncates in another way as for normal messages.
 * Careful: The string may be changed by msg_may_trunc()!
 * Returns a pointer to the printed message, if wait_return() not called.
 */
    char_u *
msg_trunc_attr(s, force, attr)
    char_u	*s;
    int		force;
    int		attr;
{
    int		n;

    /* Add message to history before truncating */
    add_msg_hist(s, -1, attr);

    s = msg_may_trunc(force, s);

    msg_hist_off = TRUE;
    n = msg_attr(s, attr);
    msg_hist_off = FALSE;

    if (n)
	return s;
    return NULL;
}

/*
 * Check if message "s" should be truncated at the start (for filenames).
 * Return a pointer to where the truncated message starts.
 * Note: May change the message by replacing a character with '<'.
 */
    char_u *
msg_may_trunc(force, s)
    int		force;
    char_u	*s;
{
    int		n;
    int		room;

    room = (int)(Rows - cmdline_row - 1) * Columns + sc_col - 1;
    if ((force || (shortmess(SHM_TRUNC) && !exmode_active))
	    && (n = (int)STRLEN(s) - room) > 0)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    int	size = vim_strsize(s);

	    /* There may be room anyway when there are multibyte chars. */
	    if (size <= room)
		return s;

	    for (n = 0; size >= room; )
	    {
		size -= (*mb_ptr2cells)(s + n);
		n += (*mb_ptr2len)(s + n);
	    }
	    --n;
	}
#endif
	s += n;
	*s = '<';
    }
    return s;
}

    static void
add_msg_hist(s, len, attr)
    char_u	*s;
    int		len;		/* -1 for undetermined length */
    int		attr;
{
    struct msg_hist *p;

    if (msg_hist_off || msg_silent != 0)
	return;

    /* Don't let the message history get too big */
    while (msg_hist_len > MAX_MSG_HIST_LEN)
	(void)delete_first_msg();

    /* allocate an entry and add the message at the end of the history */
    p = (struct msg_hist *)alloc((int)sizeof(struct msg_hist));
    if (p != NULL)
    {
	if (len < 0)
	    len = (int)STRLEN(s);
	/* remove leading and trailing newlines */
	while (len > 0 && *s == '\n')
	{
	    ++s;
	    --len;
	}
	while (len > 0 && s[len - 1] == '\n')
	    --len;
	p->msg = vim_strnsave(s, len);
	p->next = NULL;
	p->attr = attr;
	if (last_msg_hist != NULL)
	    last_msg_hist->next = p;
	last_msg_hist = p;
	if (first_msg_hist == NULL)
	    first_msg_hist = last_msg_hist;
	++msg_hist_len;
    }
}

/*
 * Delete the first (oldest) message from the history.
 * Returns FAIL if there are no messages.
 */
    int
delete_first_msg()
{
    struct msg_hist *p;

    if (msg_hist_len <= 0)
	return FAIL;
    p = first_msg_hist;
    first_msg_hist = p->next;
    vim_free(p->msg);
    vim_free(p);
    --msg_hist_len;
    return OK;
}

/*
 * ":messages" command.
 */
/*ARGSUSED*/
    void
ex_messages(eap)
    exarg_T	*eap;
{
    struct msg_hist *p;
    char_u	    *s;

    msg_hist_off = TRUE;

    s = mch_getenv((char_u *)"LANG");
    if (s != NULL && *s != NUL)
	msg_attr((char_u *)
		_("Messages maintainer: Bram Moolenaar <Bram@vim.org>"),
		hl_attr(HLF_T));

    for (p = first_msg_hist; p != NULL; p = p->next)
	if (p->msg != NULL)
	    msg_attr(p->msg, p->attr);

    msg_hist_off = FALSE;
}

#if defined(FEAT_CON_DIALOG) || defined(FIND_REPLACE_DIALOG) || defined(PROTO)
/*
 * Call this after prompting the user.  This will avoid a hit-return message
 * and a delay.
 */
    void
msg_end_prompt()
{
    need_wait_return = FALSE;
    emsg_on_display = FALSE;
    cmdline_row = msg_row;
    msg_col = 0;
    msg_clr_eos();
}
#endif

/*
 * wait for the user to hit a key (normally a return)
 * if 'redraw' is TRUE, clear and redraw the screen
 * if 'redraw' is FALSE, just redraw the screen
 * if 'redraw' is -1, don't redraw at all
 */
    void
wait_return(redraw)
    int		redraw;
{
    int		c;
    int		oldState;
    int		tmpState;
    int		had_got_int;

    if (redraw == TRUE)
	must_redraw = CLEAR;

    /* If using ":silent cmd", don't wait for a return.  Also don't set
     * need_wait_return to do it later. */
    if (msg_silent != 0)
	return;

/*
 * With the global command (and some others) we only need one return at the
 * end. Adjust cmdline_row to avoid the next message overwriting the last one.
 * When inside vgetc(), we can't wait for a typed character at all.
 */
    if (vgetc_busy > 0)
	return;
    if (no_wait_return)
    {
	need_wait_return = TRUE;
	if (!exmode_active)
	    cmdline_row = msg_row;
	return;
    }

    redir_off = TRUE;		/* don't redirect this message */
    oldState = State;
    if (quit_more)
    {
	c = CAR;		/* just pretend CR was hit */
	quit_more = FALSE;
	got_int = FALSE;
    }
    else if (exmode_active)
    {
	MSG_PUTS(" ");		/* make sure the cursor is on the right line */
	c = CAR;		/* no need for a return in ex mode */
	got_int = FALSE;
    }
    else
    {
	/* Make sure the hit-return prompt is on screen when 'guioptions' was
	 * just changed. */
	screenalloc(FALSE);

	State = HITRETURN;
#ifdef FEAT_MOUSE
	setmouse();
#endif
#ifdef USE_ON_FLY_SCROLL
	dont_scroll = TRUE;		/* disallow scrolling here */
#endif
	hit_return_msg();

	do
	{
	    /* Remember "got_int", if it is set vgetc() probably returns a
	     * CTRL-C, but we need to loop then. */
	    had_got_int = got_int;

	    /* Don't do mappings here, we put the character back in the
	     * typeahead buffer. */
	    ++no_mapping;
	    ++allow_keys;
	    c = safe_vgetc();
	    if (had_got_int && !global_busy)
		got_int = FALSE;
	    --no_mapping;
	    --allow_keys;

#ifdef FEAT_CLIPBOARD
	    /* Strange way to allow copying (yanking) a modeless selection at
	     * the hit-enter prompt.  Use CTRL-Y, because the same is used in
	     * Cmdline-mode and it's harmless when there is no selection. */
	    if (c == Ctrl_Y && clip_star.state == SELECT_DONE)
	    {
		clip_copy_modeless_selection(TRUE);
		c = K_IGNORE;
	    }
#endif
	    /*
	     * Allow scrolling back in the messages.
	     * Also accept scroll-down commands when messages fill the screen,
	     * to avoid that typing one 'j' too many makes the messages
	     * disappear.
	     */
	    if (p_more && !p_cp)
	    {
		if (c == 'b' || c == 'k' || c == 'u' || c == 'g' || c == K_UP)
		{
		    /* scroll back to show older messages */
		    do_more_prompt(c);
		    if (quit_more)
		    {
			c = CAR;		/* just pretend CR was hit */
			quit_more = FALSE;
			got_int = FALSE;
		    }
		    else
		    {
			c = K_IGNORE;
			hit_return_msg();
		    }
		}
		else if (msg_scrolled > Rows - 2
				     && (c == 'j' || c == K_DOWN || c == 'd'))
		    c = K_IGNORE;
	    }
	} while ((had_got_int && c == Ctrl_C)
				|| c == K_IGNORE
#ifdef FEAT_GUI
				|| c == K_VER_SCROLLBAR || c == K_HOR_SCROLLBAR
#endif
#ifdef FEAT_MOUSE
				|| c == K_LEFTDRAG   || c == K_LEFTRELEASE
				|| c == K_MIDDLEDRAG || c == K_MIDDLERELEASE
				|| c == K_RIGHTDRAG  || c == K_RIGHTRELEASE
				|| c == K_MOUSEDOWN  || c == K_MOUSEUP
				|| (!mouse_has(MOUSE_RETURN)
				    && mouse_row < msg_row
				    && (c == K_LEFTMOUSE
					|| c == K_MIDDLEMOUSE
					|| c == K_RIGHTMOUSE
					|| c == K_X1MOUSE
					|| c == K_X2MOUSE))
#endif
				);
	ui_breakcheck();
#ifdef FEAT_MOUSE
	/*
	 * Avoid that the mouse-up event causes visual mode to start.
	 */
	if (c == K_LEFTMOUSE || c == K_MIDDLEMOUSE || c == K_RIGHTMOUSE
					  || c == K_X1MOUSE || c == K_X2MOUSE)
	    (void)jump_to_mouse(MOUSE_SETPOS, NULL, 0);
	else
#endif
	    if (vim_strchr((char_u *)"\r\n ", c) == NULL && c != Ctrl_C)
	{
	    /* Put the character back in the typeahead buffer.  Don't use the
	     * stuff buffer, because lmaps wouldn't work. */
	    ins_char_typebuf(c);
	    do_redraw = TRUE;	    /* need a redraw even though there is
				       typeahead */
	}
    }
    redir_off = FALSE;

    /*
     * If the user hits ':', '?' or '/' we get a command line from the next
     * line.
     */
    if (c == ':' || c == '?' || c == '/')
    {
	if (!exmode_active)
	    cmdline_row = msg_row;
	skip_redraw = TRUE;	    /* skip redraw once */
	do_redraw = FALSE;
    }

    /*
     * If the window size changed set_shellsize() will redraw the screen.
     * Otherwise the screen is only redrawn if 'redraw' is set and no ':'
     * typed.
     */
    tmpState = State;
    State = oldState;		    /* restore State before set_shellsize */
#ifdef FEAT_MOUSE
    setmouse();
#endif
    msg_check();

#if defined(UNIX) || defined(VMS)
    /*
     * When switching screens, we need to output an extra newline on exit.
     */
    if (swapping_screen() && !termcap_active)
	newline_on_exit = TRUE;
#endif

    need_wait_return = FALSE;
    did_wait_return = TRUE;
    emsg_on_display = FALSE;	/* can delete error message now */
    lines_left = -1;		/* reset lines_left at next msg_start() */
    reset_last_sourcing();
    if (keep_msg != NULL && vim_strsize(keep_msg) >=
				  (Rows - cmdline_row - 1) * Columns + sc_col)
    {
	vim_free(keep_msg);
	keep_msg = NULL;	    /* don't redisplay message, it's too long */
    }

    if (tmpState == SETWSIZE)	    /* got resize event while in vgetc() */
    {
	starttermcap();		    /* start termcap before redrawing */
	shell_resized();
    }
    else if (!skip_redraw
	    && (redraw == TRUE || (msg_scrolled != 0 && redraw != -1)))
    {
	starttermcap();		    /* start termcap before redrawing */
	redraw_later(VALID);
    }
}

/*
 * Write the hit-return prompt.
 */
    static void
hit_return_msg()
{
    int		save_p_more = p_more;

    p_more = FALSE;	/* don't want see this message when scrolling back */
    if (msg_didout)	/* start on a new line */
	msg_putchar('\n');
    if (got_int)
	MSG_PUTS(_("Interrupt: "));

    MSG_PUTS_ATTR(_("Press ENTER or type command to continue"), hl_attr(HLF_R));
    if (!msg_use_printf())
	msg_clr_eos();
    p_more = save_p_more;
}

/*
 * Set "keep_msg" to "s".  Free the old value and check for NULL pointer.
 */
    void
set_keep_msg(s, attr)
    char_u	*s;
    int		attr;
{
    vim_free(keep_msg);
    if (s != NULL && msg_silent == 0)
	keep_msg = vim_strsave(s);
    else
	keep_msg = NULL;
    keep_msg_more = FALSE;
    keep_msg_attr = attr;
}

#if defined(FEAT_TERMRESPONSE) || defined(PROTO)
/*
 * If there currently is a message being displayed, set "keep_msg" to it, so
 * that it will be displayed again after redraw.
 */
    void
set_keep_msg_from_hist()
{
    if (keep_msg == NULL && last_msg_hist != NULL && msg_scrolled == 0
							  && (State & NORMAL))
	set_keep_msg(last_msg_hist->msg, last_msg_hist->attr);
}
#endif

/*
 * Prepare for outputting characters in the command line.
 */
    void
msg_start()
{
    int		did_return = FALSE;

    vim_free(keep_msg);
    keep_msg = NULL;			/* don't display old message now */
    if (!msg_scroll && full_screen)	/* overwrite last message */
    {
	msg_row = cmdline_row;
	msg_col =
#ifdef FEAT_RIGHTLEFT
	    cmdmsg_rl ? Columns - 1 :
#endif
	    0;
    }
    else if (msg_didout)		    /* start message on next line */
    {
	msg_putchar('\n');
	did_return = TRUE;
	if (exmode_active != EXMODE_NORMAL)
	    cmdline_row = msg_row;
    }
    if (!msg_didany || lines_left < 0)
	msg_starthere();
    if (msg_silent == 0)
    {
	msg_didout = FALSE;		    /* no output on current line yet */
	cursor_off();
    }

    /* when redirecting, may need to start a new line. */
    if (!did_return)
	redir_write((char_u *)"\n", -1);
}

/*
 * Note that the current msg position is where messages start.
 */
    void
msg_starthere()
{
    lines_left = cmdline_row;
    msg_didany = FALSE;
}

    void
msg_putchar(c)
    int		c;
{
    msg_putchar_attr(c, 0);
}

    void
msg_putchar_attr(c, attr)
    int		c;
    int		attr;
{
#ifdef FEAT_MBYTE
    char_u	buf[MB_MAXBYTES + 1];
#else
    char_u	buf[4];
#endif

    if (IS_SPECIAL(c))
    {
	buf[0] = K_SPECIAL;
	buf[1] = K_SECOND(c);
	buf[2] = K_THIRD(c);
	buf[3] = NUL;
    }
    else
    {
#ifdef FEAT_MBYTE
	buf[(*mb_char2bytes)(c, buf)] = NUL;
#else
	buf[0] = c;
	buf[1] = NUL;
#endif
    }
    msg_puts_attr(buf, attr);
}

    void
msg_outnum(n)
    long	n;
{
    char_u	buf[20];

    sprintf((char *)buf, "%ld", n);
    msg_puts(buf);
}

    void
msg_home_replace(fname)
    char_u	*fname;
{
    msg_home_replace_attr(fname, 0);
}

#if defined(FEAT_FIND_ID) || defined(PROTO)
    void
msg_home_replace_hl(fname)
    char_u	*fname;
{
    msg_home_replace_attr(fname, hl_attr(HLF_D));
}
#endif

    static void
msg_home_replace_attr(fname, attr)
    char_u  *fname;
    int	    attr;
{
    char_u	*name;

    name = home_replace_save(NULL, fname);
    if (name != NULL)
	msg_outtrans_attr(name, attr);
    vim_free(name);
}

/*
 * Output 'len' characters in 'str' (including NULs) with translation
 * if 'len' is -1, output upto a NUL character.
 * Use attributes 'attr'.
 * Return the number of characters it takes on the screen.
 */
    int
msg_outtrans(str)
    char_u	    *str;
{
    return msg_outtrans_attr(str, 0);
}

    int
msg_outtrans_attr(str, attr)
    char_u	*str;
    int		attr;
{
    return msg_outtrans_len_attr(str, (int)STRLEN(str), attr);
}

    int
msg_outtrans_len(str, len)
    char_u	*str;
    int		len;
{
    return msg_outtrans_len_attr(str, len, 0);
}

/*
 * Output one character at "p".  Return pointer to the next character.
 * Handles multi-byte characters.
 */
    char_u *
msg_outtrans_one(p, attr)
    char_u	*p;
    int		attr;
{
#ifdef FEAT_MBYTE
    int		l;

    if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
    {
	msg_outtrans_len_attr(p, l, attr);
	return p + l;
    }
#endif
    msg_puts_attr(transchar_byte(*p), attr);
    return p + 1;
}

    int
msg_outtrans_len_attr(msgstr, len, attr)
    char_u	*msgstr;
    int		len;
    int		attr;
{
    int		retval = 0;
    char_u	*str = msgstr;
    char_u	*plain_start = msgstr;
    char_u	*s;
#ifdef FEAT_MBYTE
    int		mb_l;
    int		c;
#endif

    /* if MSG_HIST flag set, add message to history */
    if (attr & MSG_HIST)
    {
	add_msg_hist(str, len, attr);
	attr &= ~MSG_HIST;
    }

#ifdef FEAT_MBYTE
    /* If the string starts with a composing character first draw a space on
     * which the composing char can be drawn. */
    if (enc_utf8 && utf_iscomposing(utf_ptr2char(msgstr)))
	msg_puts_attr((char_u *)" ", attr);
#endif

    /*
     * Go over the string.  Special characters are translated and printed.
     * Normal characters are printed several at a time.
     */
    while (--len >= 0)
    {
#ifdef FEAT_MBYTE
	if (enc_utf8)
	    /* Don't include composing chars after the end. */
	    mb_l = utfc_ptr2len_len(str, len + 1);
	else if (has_mbyte)
	    mb_l = (*mb_ptr2len)(str);
	else
	    mb_l = 1;
	if (has_mbyte && mb_l > 1)
	{
	    c = (*mb_ptr2char)(str);
	    if (vim_isprintc(c))
		/* printable multi-byte char: count the cells. */
		retval += (*mb_ptr2cells)(str);
	    else
	    {
		/* unprintable multi-byte char: print the printable chars so
		 * far and the translation of the unprintable char. */
		if (str > plain_start)
		    msg_puts_attr_len(plain_start, (int)(str - plain_start),
									attr);
		plain_start = str + mb_l;
		msg_puts_attr(transchar(c), attr == 0 ? hl_attr(HLF_8) : attr);
		retval += char2cells(c);
	    }
	    len -= mb_l - 1;
	    str += mb_l;
	}
	else
#endif
	{
	    s = transchar_byte(*str);
	    if (s[1] != NUL)
	    {
		/* unprintable char: print the printable chars so far and the
		 * translation of the unprintable char. */
		if (str > plain_start)
		    msg_puts_attr_len(plain_start, (int)(str - plain_start),
									attr);
		plain_start = str + 1;
		msg_puts_attr(s, attr == 0 ? hl_attr(HLF_8) : attr);
	    }
	    retval += ptr2cells(str);
	    ++str;
	}
    }

    if (str > plain_start)
	/* print the printable chars at the end */
	msg_puts_attr_len(plain_start, (int)(str - plain_start), attr);

    return retval;
}

#if defined(FEAT_QUICKFIX) || defined(PROTO)
    void
msg_make(arg)
    char_u  *arg;
{
    int	    i;
    static char_u *str = (char_u *)"eeffoc", *rs = (char_u *)"Plon#dqg#vxjduB";

    arg = skipwhite(arg);
    for (i = 5; *arg && i >= 0; --i)
	if (*arg++ != str[i])
	    break;
    if (i < 0)
    {
	msg_putchar('\n');
	for (i = 0; rs[i]; ++i)
	    msg_putchar(rs[i] - 3);
    }
}
#endif

/*
 * Output the string 'str' upto a NUL character.
 * Return the number of characters it takes on the screen.
 *
 * If K_SPECIAL is encountered, then it is taken in conjunction with the
 * following character and shown as <F1>, <S-Up> etc.  Any other character
 * which is not printable shown in <> form.
 * If 'from' is TRUE (lhs of a mapping), a space is shown as <Space>.
 * If a character is displayed in one of these special ways, is also
 * highlighted (its highlight name is '8' in the p_hl variable).
 * Otherwise characters are not highlighted.
 * This function is used to show mappings, where we want to see how to type
 * the character/string -- webb
 */
    int
msg_outtrans_special(strstart, from)
    char_u	*strstart;
    int		from;	/* TRUE for lhs of a mapping */
{
    char_u	*str = strstart;
    int		retval = 0;
    char_u	*string;
    int		attr;
    int		len;

    attr = hl_attr(HLF_8);
    while (*str != NUL)
    {
	/* Leading and trailing spaces need to be displayed in <> form. */
	if ((str == strstart || str[1] == NUL) && *str == ' ')
	{
	    string = (char_u *)"<Space>";
	    ++str;
	}
	else
	    string = str2special(&str, from);
	len = vim_strsize(string);
	/* Highlight special keys */
	msg_puts_attr(string, len > 1
#ifdef FEAT_MBYTE
		&& (*mb_ptr2len)(string) <= 1
#endif
		? attr : 0);
	retval += len;
    }
    return retval;
}

/*
 * Return the printable string for the key codes at "*sp".
 * Used for translating the lhs or rhs of a mapping to printable chars.
 * Advances "sp" to the next code.
 */
    char_u *
str2special(sp, from)
    char_u	**sp;
    int		from;	/* TRUE for lhs of mapping */
{
    int			c;
    static char_u	buf[7];
    char_u		*str = *sp;
    int			modifiers = 0;
    int			special = FALSE;

#ifdef FEAT_MBYTE
    if (has_mbyte)
    {
	char_u	*p;

	/* Try to un-escape a multi-byte character.  Return the un-escaped
	 * string if it is a multi-byte character. */
	p = mb_unescape(sp);
	if (p != NULL)
	    return p;
    }
#endif

    c = *str;
    if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
    {
	if (str[1] == KS_MODIFIER)
	{
	    modifiers = str[2];
	    str += 3;
	    c = *str;
	}
	if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
	{
	    c = TO_SPECIAL(str[1], str[2]);
	    str += 2;
	    if (c == K_ZERO)	/* display <Nul> as ^@ */
		c = NUL;
	}
	if (IS_SPECIAL(c) || modifiers)	/* special key */
	    special = TRUE;
    }
    *sp = str + 1;

#ifdef FEAT_MBYTE
    /* For multi-byte characters check for an illegal byte. */
    if (has_mbyte && MB_BYTE2LEN(*str) > (*mb_ptr2len)(str))
    {
	transchar_nonprint(buf, c);
	return buf;
    }
#endif

    /* Make unprintable characters in <> form, also <M-Space> and <Tab>.
     * Use <Space> only for lhs of a mapping. */
    if (special || char2cells(c) > 1 || (from && c == ' '))
	return get_special_key_name(c, modifiers);
    buf[0] = c;
    buf[1] = NUL;
    return buf;
}

/*
 * Translate a key sequence into special key names.
 */
    void
str2specialbuf(sp, buf, len)
    char_u	*sp;
    char_u	*buf;
    int		len;
{
    char_u	*s;

    *buf = NUL;
    while (*sp)
    {
	s = str2special(&sp, FALSE);
	if ((int)(STRLEN(s) + STRLEN(buf)) < len)
	    STRCAT(buf, s);
    }
}

/*
 * print line for :print or :list command
 */
    void
msg_prt_line(s, list)
    char_u	*s;
    int		list;
{
    int		c;
    int		col = 0;
    int		n_extra = 0;
    int		c_extra = 0;
    char_u	*p_extra = NULL;	    /* init to make SASC shut up */
    int		n;
    int		attr= 0;
    char_u	*trail = NULL;
#ifdef FEAT_MBYTE
    int		l;
    char_u	buf[MB_MAXBYTES + 1];
#endif

    if (curwin->w_p_list)
	list = TRUE;

    /* find start of trailing whitespace */
    if (list && lcs_trail)
    {
	trail = s + STRLEN(s);
	while (trail > s && vim_iswhite(trail[-1]))
	    --trail;
    }

    /* output a space for an empty line, otherwise the line will be
     * overwritten */
    if (*s == NUL && !(list && lcs_eol != NUL))
	msg_putchar(' ');

    while (!got_int)
    {
	if (n_extra)
	{
	    --n_extra;
	    if (c_extra)
		c = c_extra;
	    else
		c = *p_extra++;
	}
#ifdef FEAT_MBYTE
	else if (has_mbyte && (l = (*mb_ptr2len)(s)) > 1)
	{
	    col += (*mb_ptr2cells)(s);
	    mch_memmove(buf, s, (size_t)l);
	    buf[l] = NUL;
	    msg_puts_attr(buf, attr);
	    s += l;
	    continue;
	}
#endif
	else
	{
	    attr = 0;
	    c = *s++;
	    if (c == TAB && (!list || lcs_tab1))
	    {
		/* tab amount depends on current column */
		n_extra = curbuf->b_p_ts - col % curbuf->b_p_ts - 1;
		if (!list)
		{
		    c = ' ';
		    c_extra = ' ';
		}
		else
		{
		    c = lcs_tab1;
		    c_extra = lcs_tab2;
		    attr = hl_attr(HLF_8);
		}
	    }
	    else if (c == NUL && list && lcs_eol != NUL)
	    {
		p_extra = (char_u *)"";
		c_extra = NUL;
		n_extra = 1;
		c = lcs_eol;
		attr = hl_attr(HLF_AT);
		--s;
	    }
	    else if (c != NUL && (n = byte2cells(c)) > 1)
	    {
		n_extra = n - 1;
		p_extra = transchar_byte(c);
		c_extra = NUL;
		c = *p_extra++;
	    }
	    else if (c == ' ' && trail != NULL && s > trail)
	    {
		c = lcs_trail;
		attr = hl_attr(HLF_8);
	    }
	}

	if (c == NUL)
	    break;

	msg_putchar_attr(c, attr);
	col++;
    }
    msg_clr_eos();
}

#ifdef FEAT_MBYTE
/*
 * Use screen_puts() to output one multi-byte character.
 * Return the pointer "s" advanced to the next character.
 */
    static char_u *
screen_puts_mbyte(s, l, attr)
    char_u	*s;
    int		l;
    int		attr;
{
    int		cw;

    msg_didout = TRUE;		/* remember that line is not empty */
    cw = (*mb_ptr2cells)(s);
    if (cw > 1 && (
#ifdef FEAT_RIGHTLEFT
		cmdmsg_rl ? msg_col <= 1 :
#endif
		msg_col == Columns - 1))
    {
	/* Doesn't fit, print a highlighted '>' to fill it up. */
	msg_screen_putchar('>', hl_attr(HLF_AT));
	return s;
    }

    screen_puts_len(s, l, msg_row, msg_col, attr);
#ifdef FEAT_RIGHTLEFT
    if (cmdmsg_rl)
    {
	msg_col -= cw;
	if (msg_col == 0)
	{
	    msg_col = Columns;
	    ++msg_row;
	}
    }
    else
#endif
    {
	msg_col += cw;
	if (msg_col >= Columns)
	{
	    msg_col = 0;
	    ++msg_row;
	}
    }
    return s + l;
}
#endif

/*
 * Output a string to the screen at position msg_row, msg_col.
 * Update msg_row and msg_col for the next message.
 */
    void
msg_puts(s)
    char_u	*s;
{
    msg_puts_attr(s, 0);
}

    void
msg_puts_title(s)
    char_u	*s;
{
    msg_puts_attr(s, hl_attr(HLF_T));
}

/*
 * Show a message in such a way that it always fits in the line.  Cut out a
 * part in the middle and replace it with "..." when necessary.
 * Does not handle multi-byte characters!
 */
    void
msg_puts_long_attr(longstr, attr)
    char_u	*longstr;
    int		attr;
{
    msg_puts_long_len_attr(longstr, (int)STRLEN(longstr), attr);
}

    void
msg_puts_long_len_attr(longstr, len, attr)
    char_u	*longstr;
    int		len;
    int		attr;
{
    int		slen = len;
    int		room;

    room = Columns - msg_col;
    if (len > room && room >= 20)
    {
	slen = (room - 3) / 2;
	msg_outtrans_len_attr(longstr, slen, attr);
	msg_puts_attr((char_u *)"...", hl_attr(HLF_8));
    }
    msg_outtrans_len_attr(longstr + len - slen, slen, attr);
}

/*
 * Basic function for writing a message with highlight attributes.
 */
    void
msg_puts_attr(s, attr)
    char_u	*s;
    int		attr;
{
    msg_puts_attr_len(s, -1, attr);
}

/*
 * Like msg_puts_attr(), but with a maximum length "maxlen" (in bytes).
 * When "maxlen" is -1 there is no maximum length.
 * When "maxlen" is >= 0 the message is not put in the history.
 */
    static void
msg_puts_attr_len(str, maxlen, attr)
    char_u	*str;
    int		maxlen;
    int		attr;
{
    /*
     * If redirection is on, also write to the redirection file.
     */
    redir_write(str, maxlen);

    /*
     * Don't print anything when using ":silent cmd".
     */
    if (msg_silent != 0)
	return;

    /* if MSG_HIST flag set, add message to history */
    if ((attr & MSG_HIST) && maxlen < 0)
    {
	add_msg_hist(str, -1, attr);
	attr &= ~MSG_HIST;
    }

    /*
     * When writing something to the screen after it has scrolled, requires a
     * wait-return prompt later.  Needed when scrolling, resetting
     * need_wait_return after some prompt, and then outputting something
     * without scrolling
     */
    if (msg_scrolled != 0 && !msg_scrolled_ign)
	need_wait_return = TRUE;
    msg_didany = TRUE;		/* remember that something was outputted */

    /*
     * If there is no valid screen, use fprintf so we can see error messages.
     * If termcap is not active, we may be writing in an alternate console
     * window, cursor positioning may not work correctly (window size may be
     * different, e.g. for Win32 console) or we just don't know where the
     * cursor is.
     */
    if (msg_use_printf())
	msg_puts_printf(str, maxlen);
    else
	msg_puts_display(str, maxlen, attr, FALSE);
}

/*
 * The display part of msg_puts_attr_len().
 * May be called recursively to display scroll-back text.
 */
    static void
msg_puts_display(str, maxlen, attr, recurse)
    char_u	*str;
    int		maxlen;
    int		attr;
    int		recurse;
{
    char_u	*s = str;
    char_u	*t_s = str;	/* string from "t_s" to "s" is still todo */
    int		t_col = 0;	/* screen cells todo, 0 when "t_s" not used */
#ifdef FEAT_MBYTE
    int		l;
    int		cw;
#endif
    char_u	*sb_str = str;
    int		sb_col = msg_col;
    int		wrap;

    did_wait_return = FALSE;
    while (*s != NUL && (maxlen < 0 || (int)(s - str) < maxlen))
    {
	/*
	 * We are at the end of the screen line when:
	 * - When outputting a newline.
	 * - When outputting a character in the last column.
	 */
	if (!recurse && msg_row >= Rows - 1 && (*s == '\n' || (
#ifdef FEAT_RIGHTLEFT
		    cmdmsg_rl
		    ? (
			msg_col <= 1
			|| (*s == TAB && msg_col <= 7)
# ifdef FEAT_MBYTE
			|| (has_mbyte && (*mb_ptr2cells)(s) > 1 && msg_col <= 2)
# endif
		      )
		    :
#endif
		      (msg_col + t_col >= Columns - 1
		       || (*s == TAB && msg_col + t_col >= ((Columns - 1) & ~7))
# ifdef FEAT_MBYTE
		       || (has_mbyte && (*mb_ptr2cells)(s) > 1
					    && msg_col + t_col >= Columns - 2)
# endif
		      ))))
	{
	    /*
	     * The screen is scrolled up when at the last row (some terminals
	     * scroll automatically, some don't.  To avoid problems we scroll
	     * ourselves).
	     */
	    if (t_col > 0)
		/* output postponed text */
		t_puts(&t_col, t_s, s, attr);

	    /* When no more prompt an no more room, truncate here */
	    if (msg_no_more && lines_left == 0)
		break;

	    /* Scroll the screen up one line. */
	    msg_scroll_up();

	    msg_row = Rows - 2;
	    if (msg_col >= Columns)	/* can happen after screen resize */
		msg_col = Columns - 1;

	    /* Display char in last column before showing more-prompt. */
	    if (*s >= ' '
#ifdef FEAT_RIGHTLEFT
		    && !cmdmsg_rl
#endif
	       )
	    {
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    if (enc_utf8 && maxlen >= 0)
			/* avoid including composing chars after the end */
			l = utfc_ptr2len_len(s, (int)((str + maxlen) - s));
		    else
			l = (*mb_ptr2len)(s);
		    s = screen_puts_mbyte(s, l, attr);
		}
		else
#endif
		    msg_screen_putchar(*s++, attr);
	    }

	    if (p_more)
		/* store text for scrolling back */
		store_sb_text(&sb_str, s, attr, &sb_col, TRUE);

	    inc_msg_scrolled();
	    need_wait_return = TRUE; /* may need wait_return in main() */
	    if (must_redraw < VALID)
		must_redraw = VALID;
	    redraw_cmdline = TRUE;
	    if (cmdline_row > 0 && !exmode_active)
		--cmdline_row;

	    /*
	     * If screen is completely filled and 'more' is set then wait
	     * for a character.
	     */
	    if (p_more && --lines_left == 0 && State != HITRETURN
					    && !msg_no_more && !exmode_active)
	    {
#ifdef FEAT_CON_DIALOG
		if (do_more_prompt(NUL))
		    s = confirm_msg_tail;
#else
		(void)do_more_prompt(NUL);
#endif
		if (quit_more)
		    return;
	    }

	    /* When we displayed a char in last column need to check if there
	     * is still more. */
	    if (*s >= ' '
#ifdef FEAT_RIGHTLEFT
		    && !cmdmsg_rl
#endif
	       )
		continue;
	}

	wrap = *s == '\n'
		    || msg_col + t_col >= Columns
#ifdef FEAT_MBYTE
		    || (has_mbyte && (*mb_ptr2cells)(s) > 1
					    && msg_col + t_col >= Columns - 1)
#endif
		    ;
	if (t_col > 0 && (wrap || *s == '\r' || *s == '\b'
						 || *s == '\t' || *s == BELL))
	    /* output any postponed text */
	    t_puts(&t_col, t_s, s, attr);

	if (wrap && p_more && !recurse)
	    /* store text for scrolling back */
	    store_sb_text(&sb_str, s, attr, &sb_col, TRUE);

	if (*s == '\n')		    /* go to next line */
	{
	    msg_didout = FALSE;	    /* remember that line is empty */
#ifdef FEAT_RIGHTLEFT
	    if (cmdmsg_rl)
		msg_col = Columns - 1;
	    else
#endif
		msg_col = 0;
	    if (++msg_row >= Rows)  /* safety check */
		msg_row = Rows - 1;
	}
	else if (*s == '\r')	    /* go to column 0 */
	{
	    msg_col = 0;
	}
	else if (*s == '\b')	    /* go to previous char */
	{
	    if (msg_col)
		--msg_col;
	}
	else if (*s == TAB)	    /* translate Tab into spaces */
	{
	    do
		msg_screen_putchar(' ', attr);
	    while (msg_col & 7);
	}
	else if (*s == BELL)	    /* beep (from ":sh") */
	    vim_beep();
	else
	{
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		cw = (*mb_ptr2cells)(s);
		if (enc_utf8 && maxlen >= 0)
		    /* avoid including composing chars after the end */
		    l = utfc_ptr2len_len(s, (int)((str + maxlen) - s));
		else
		    l = (*mb_ptr2len)(s);
	    }
	    else
	    {
		cw = 1;
		l = 1;
	    }
#endif
	    /* When drawing from right to left or when a double-wide character
	     * doesn't fit, draw a single character here.  Otherwise collect
	     * characters and draw them all at once later. */
#if defined(FEAT_RIGHTLEFT) || defined(FEAT_MBYTE)
	    if (
# ifdef FEAT_RIGHTLEFT
		    cmdmsg_rl
#  ifdef FEAT_MBYTE
		    ||
#  endif
# endif
# ifdef FEAT_MBYTE
		    (cw > 1 && msg_col + t_col >= Columns - 1)
# endif
		    )
	    {
# ifdef FEAT_MBYTE
		if (l > 1)
		    s = screen_puts_mbyte(s, l, attr) - 1;
		else
# endif
		    msg_screen_putchar(*s, attr);
	    }
	    else
#endif
	    {
		/* postpone this character until later */
		if (t_col == 0)
		    t_s = s;
#ifdef FEAT_MBYTE
		t_col += cw;
		s += l - 1;
#else
		++t_col;
#endif
	    }
	}
	++s;
    }

    /* output any postponed text */
    if (t_col > 0)
	t_puts(&t_col, t_s, s, attr);
    if (p_more && !recurse)
	store_sb_text(&sb_str, s, attr, &sb_col, FALSE);

    msg_check();
}

/*
 * Scroll the screen up one line for displaying the next message line.
 */
    static void
msg_scroll_up()
{
#ifdef FEAT_GUI
    /* Remove the cursor before scrolling, ScreenLines[] is going
     * to become invalid. */
    if (gui.in_use)
	gui_undraw_cursor();
#endif
    /* scrolling up always works */
    screen_del_lines(0, 0, 1, (int)Rows, TRUE, NULL);

    if (!can_clear((char_u *)" "))
    {
	/* Scrolling up doesn't result in the right background.  Set the
	 * background here.  It's not efficient, but avoids that we have to do
	 * it all over the code. */
	screen_fill((int)Rows - 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);

	/* Also clear the last char of the last but one line if it was not
	 * cleared before to avoid a scroll-up. */
	if (ScreenAttrs[LineOffset[Rows - 2] + Columns - 1] == (sattr_T)-1)
	    screen_fill((int)Rows - 2, (int)Rows - 1,
				 (int)Columns - 1, (int)Columns, ' ', ' ', 0);
    }
}

/*
 * Increment "msg_scrolled".
 */
    static void
inc_msg_scrolled()
{
#ifdef FEAT_EVAL
    if (*get_vim_var_str(VV_SCROLLSTART) == NUL)
    {
	char_u	    *p = sourcing_name;
	char_u	    *tofree = NULL;
	int	    len;

	/* v:scrollstart is empty, set it to the script/function name and line
	 * number */
	if (p == NULL)
	    p = (char_u *)_("Unknown");
	else
	{
	    len = (int)STRLEN(p) + 40;
	    tofree = alloc(len);
	    if (tofree != NULL)
	    {
		vim_snprintf((char *)tofree, len, _("%s line %ld"),
						      p, (long)sourcing_lnum);
		p = tofree;
	    }
	}
	set_vim_var_string(VV_SCROLLSTART, p, -1);
	vim_free(tofree);
    }
#endif
    ++msg_scrolled;
}

/*
 * To be able to scroll back at the "more" and "hit-enter" prompts we need to
 * store the displayed text and remember where screen lines start.
 */
typedef struct msgchunk_S msgchunk_T;
struct msgchunk_S
{
    msgchunk_T	*sb_next;
    msgchunk_T	*sb_prev;
    char	sb_eol;		/* TRUE when line ends after this text */
    int		sb_msg_col;	/* column in which text starts */
    int		sb_attr;	/* text attributes */
    char_u	sb_text[1];	/* text to be displayed, actually longer */
};

static msgchunk_T *last_msgchunk = NULL; /* last displayed text */

static msgchunk_T *msg_sb_start __ARGS((msgchunk_T *mps));
static msgchunk_T *disp_sb_line __ARGS((int row, msgchunk_T *smp));

static int do_clear_sb_text = FALSE;	/* clear text on next msg */

/*
 * Store part of a printed message for displaying when scrolling back.
 */
    static void
store_sb_text(sb_str, s, attr, sb_col, finish)
    char_u	**sb_str;	/* start of string */
    char_u	*s;		/* just after string */
    int		attr;
    int		*sb_col;
    int		finish;		/* line ends */
{
    msgchunk_T	*mp;

    if (do_clear_sb_text)
    {
	clear_sb_text();
	do_clear_sb_text = FALSE;
    }

    if (s > *sb_str)
    {
	mp = (msgchunk_T *)alloc((int)(sizeof(msgchunk_T) + (s - *sb_str)));
	if (mp != NULL)
	{
	    mp->sb_eol = finish;
	    mp->sb_msg_col = *sb_col;
	    mp->sb_attr = attr;
	    vim_strncpy(mp->sb_text, *sb_str, s - *sb_str);

	    if (last_msgchunk == NULL)
	    {
		last_msgchunk = mp;
		mp->sb_prev = NULL;
	    }
	    else
	    {
		mp->sb_prev = last_msgchunk;
		last_msgchunk->sb_next = mp;
		last_msgchunk = mp;
	    }
	    mp->sb_next = NULL;
	}
    }
    else if (finish && last_msgchunk != NULL)
	last_msgchunk->sb_eol = TRUE;

    *sb_str = s;
    *sb_col = 0;
}

/*
 * Finished showing messages, clear the scroll-back text on the next message.
 */
    void
may_clear_sb_text()
{
    do_clear_sb_text = TRUE;
}

/*
 * Clear any text remembered for scrolling back.
 * Called when redrawing the screen.
 */
    void
clear_sb_text()
{
    msgchunk_T	*mp;

    while (last_msgchunk != NULL)
    {
	mp = last_msgchunk->sb_prev;
	vim_free(last_msgchunk);
	last_msgchunk = mp;
    }
}

/*
 * "g<" command.
 */
    void
show_sb_text()
{
    msgchunk_T	*mp;

    /* Only show somethign if there is more than one line, otherwise it looks
     * weird, typing a command without output results in one line. */
    mp = msg_sb_start(last_msgchunk);
    if (mp == NULL || mp->sb_prev == NULL)
	vim_beep();
    else
    {
	do_more_prompt('G');
	wait_return(FALSE);
    }
}

/*
 * Move to the start of screen line in already displayed text.
 */
    static msgchunk_T *
msg_sb_start(mps)
    msgchunk_T *mps;
{
    msgchunk_T *mp = mps;

    while (mp != NULL && mp->sb_prev != NULL && !mp->sb_prev->sb_eol)
	mp = mp->sb_prev;
    return mp;
}

/*
 * Display a screen line from previously displayed text at row "row".
 * Returns a pointer to the text for the next line (can be NULL).
 */
    static msgchunk_T *
disp_sb_line(row, smp)
    int		row;
    msgchunk_T	*smp;
{
    msgchunk_T	*mp = smp;
    char_u	*p;

    for (;;)
    {
	msg_row = row;
	msg_col = mp->sb_msg_col;
	p = mp->sb_text;
	if (*p == '\n')	    /* don't display the line break */
	    ++p;
	msg_puts_display(p, -1, mp->sb_attr, TRUE);
	if (mp->sb_eol || mp->sb_next == NULL)
	    break;
	mp = mp->sb_next;
    }
    return mp->sb_next;
}

/*
 * Output any postponed text for msg_puts_attr_len().
 */
    static void
t_puts(t_col, t_s, s, attr)
    int		*t_col;
    char_u	*t_s;
    char_u	*s;
    int		attr;
{
    /* output postponed text */
    msg_didout = TRUE;		/* remember that line is not empty */
    screen_puts_len(t_s, (int)(s - t_s), msg_row, msg_col, attr);
    msg_col += *t_col;
    *t_col = 0;
#ifdef FEAT_MBYTE
    /* If the string starts with a composing character don't increment the
     * column position for it. */
    if (enc_utf8 && utf_iscomposing(utf_ptr2char(t_s)))
	--msg_col;
#endif
    if (msg_col >= Columns)
    {
	msg_col = 0;
	++msg_row;
    }
}

/*
 * Returns TRUE when messages should be printed with mch_errmsg().
 * This is used when there is no valid screen, so we can see error messages.
 * If termcap is not active, we may be writing in an alternate console
 * window, cursor positioning may not work correctly (window size may be
 * different, e.g. for Win32 console) or we just don't know where the
 * cursor is.
 */
    int
msg_use_printf()
{
    return (!msg_check_screen()
#if defined(WIN3264) && !defined(FEAT_GUI_MSWIN)
	    || !termcap_active
#endif
	    || (swapping_screen() && !termcap_active)
	       );
}

/*
 * Print a message when there is no valid screen.
 */
    static void
msg_puts_printf(str, maxlen)
    char_u	*str;
    int		maxlen;
{
    char_u	*s = str;
    char_u	buf[4];
    char_u	*p;

#ifdef WIN3264
    if (!(silent_mode && p_verbose == 0))
	mch_settmode(TMODE_COOK);	/* handle '\r' and '\n' correctly */
#endif
    while (*s != NUL && (maxlen < 0 || (int)(s - str) < maxlen))
    {
	if (!(silent_mode && p_verbose == 0))
	{
	    /* NL --> CR NL translation (for Unix, not for "--version") */
	    /* NL --> CR translation (for Mac) */
	    p = &buf[0];
	    if (*s == '\n' && !info_message)
		*p++ = '\r';
#if defined(USE_CR) && !defined(MACOS_X_UNIX)
	    else
#endif
		*p++ = *s;
	    *p = '\0';
	    if (info_message)	/* informative message, not an error */
		mch_msg((char *)buf);
	    else
		mch_errmsg((char *)buf);
	}

	/* primitive way to compute the current column */
#ifdef FEAT_RIGHTLEFT
	if (cmdmsg_rl)
	{
	    if (*s == '\r' || *s == '\n')
		msg_col = Columns - 1;
	    else
		--msg_col;
	}
	else
#endif
	{
	    if (*s == '\r' || *s == '\n')
		msg_col = 0;
	    else
		++msg_col;
	}
	++s;
    }
    msg_didout = TRUE;	    /* assume that line is not empty */

#ifdef WIN3264
    if (!(silent_mode && p_verbose == 0))
	mch_settmode(TMODE_RAW);
#endif
}

/*
 * Show the more-prompt and handle the user response.
 * This takes care of scrolling back and displaying previously displayed text.
 * When at hit-enter prompt "typed_char" is the already typed character,
 * otherwise it's NUL.
 * Returns TRUE when jumping ahead to "confirm_msg_tail".
 */
    static int
do_more_prompt(typed_char)
    int		typed_char;
{
    int		used_typed_char = typed_char;
    int		oldState = State;
    int		c;
#ifdef FEAT_CON_DIALOG
    int		retval = FALSE;
#endif
    int		scroll;
    msgchunk_T	*mp_last = NULL;
    msgchunk_T	*mp;
    int		i;

    if (typed_char == 'G')
    {
	/* "g<": Find first line on the last page. */
	mp_last = msg_sb_start(last_msgchunk);
	for (i = 0; i < Rows - 2 && mp_last != NULL
					     && mp_last->sb_prev != NULL; ++i)
	    mp_last = msg_sb_start(mp_last->sb_prev);
    }

    State = ASKMORE;
#ifdef FEAT_MOUSE
    setmouse();
#endif
    if (typed_char == NUL)
	msg_moremsg(FALSE);
    for (;;)
    {
	/*
	 * Get a typed character directly from the user.
	 */
	if (used_typed_char != NUL)
	{
	    c = used_typed_char;	/* was typed at hit-enter prompt */
	    used_typed_char = NUL;
	}
	else
	    c = get_keystroke();

#if defined(FEAT_MENU) && defined(FEAT_GUI)
	if (c == K_MENU)
	{
	    int idx = get_menu_index(current_menu, ASKMORE);

	    /* Used a menu.  If it starts with CTRL-Y, it must
	     * be a "Copy" for the clipboard.  Otherwise
	     * assume that we end */
	    if (idx == MENU_INDEX_INVALID)
		continue;
	    c = *current_menu->strings[idx];
	    if (c != NUL && current_menu->strings[idx][1] != NUL)
		ins_typebuf(current_menu->strings[idx] + 1,
				current_menu->noremap[idx], 0, TRUE,
						   current_menu->silent[idx]);
	}
#endif

	scroll = 0;
	switch (c)
	{
	case BS:		/* scroll one line back */
	case K_BS:
	case 'k':
	case K_UP:
	    scroll = -1;
	    break;

	case CAR:		/* one extra line */
	case NL:
	case 'j':
	case K_DOWN:
	    scroll = 1;
	    break;

	case 'u':		/* Up half a page */
	case K_PAGEUP:
	    scroll = -(Rows / 2);
	    break;

	case 'd':		/* Down half a page */
	    scroll = Rows / 2;
	    break;

	case 'b':		/* one page back */
	    scroll = -(Rows - 1);
	    break;

	case ' ':		/* one extra page */
	case K_PAGEDOWN:
	case K_LEFTMOUSE:
	    scroll = Rows - 1;
	    break;

	case 'g':		/* all the way back to the start */
	    scroll = -999999;
	    break;

	case 'G':		/* all the way to the end */
	    scroll = 999999;
	    lines_left = 999999;
	    break;

	case ':':		/* start new command line */
#ifdef FEAT_CON_DIALOG
	    if (!confirm_msg_used)
#endif
	    {
		/* Since got_int is set all typeahead will be flushed, but we
		 * want to keep this ':', remember that in a special way. */
		typeahead_noflush(':');
		cmdline_row = Rows - 1;		/* put ':' on this line */
		skip_redraw = TRUE;		/* skip redraw once */
		need_wait_return = FALSE;	/* don't wait in main() */
	    }
	    /*FALLTHROUGH*/
	case 'q':		/* quit */
	case Ctrl_C:
	case ESC:
#ifdef FEAT_CON_DIALOG
	    if (confirm_msg_used)
	    {
		/* Jump to the choices of the dialog. */
		retval = TRUE;
		lines_left = Rows - 1;
	    }
	    else
#endif
	    {
		got_int = TRUE;
		quit_more = TRUE;
	    }
	    break;

#ifdef FEAT_CLIPBOARD
	case Ctrl_Y:
	    /* Strange way to allow copying (yanking) a modeless
	     * selection at the more prompt.  Use CTRL-Y,
	     * because the same is used in Cmdline-mode and at the
	     * hit-enter prompt.  However, scrolling one line up
	     * might be expected... */
	    if (clip_star.state == SELECT_DONE)
		clip_copy_modeless_selection(TRUE);
	    continue;
#endif
	default:		/* no valid response */
	    msg_moremsg(TRUE);
	    continue;
	}

	if (scroll != 0)
	{
	    if (scroll < 0)
	    {
		/* go to start of last line */
		if (mp_last == NULL)
		    mp = msg_sb_start(last_msgchunk);
		else if (mp_last->sb_prev != NULL)
		    mp = msg_sb_start(mp_last->sb_prev);
		else
		    mp = NULL;

		/* go to start of line at top of the screen */
		for (i = 0; i < Rows - 2 && mp != NULL && mp->sb_prev != NULL;
									  ++i)
		    mp = msg_sb_start(mp->sb_prev);

		if (mp != NULL && mp->sb_prev != NULL)
		{
		    /* Find line to be displayed at top. */
		    for (i = 0; i > scroll; --i)
		    {
			if (mp == NULL || mp->sb_prev == NULL)
			    break;
			mp = msg_sb_start(mp->sb_prev);
			if (mp_last == NULL)
			    mp_last = msg_sb_start(last_msgchunk);
			else
			    mp_last = msg_sb_start(mp_last->sb_prev);
		    }

		    if (scroll == -1 && screen_ins_lines(0, 0, 1,
						       (int)Rows, NULL) == OK)
		    {
			/* display line at top */
			(void)disp_sb_line(0, mp);
		    }
		    else
		    {
			/* redisplay all lines */
			screenclear();
			for (i = 0; mp != NULL && i < Rows - 1; ++i)
			{
			    mp = disp_sb_line(i, mp);
			    ++msg_scrolled;
			}
		    }
		    scroll = 0;
		}
	    }
	    else
	    {
		/* First display any text that we scrolled back. */
		while (scroll > 0 && mp_last != NULL)
		{
		    /* scroll up, display line at bottom */
		    msg_scroll_up();
		    inc_msg_scrolled();
		    screen_fill((int)Rows - 2, (int)Rows - 1, 0,
						   (int)Columns, ' ', ' ', 0);
		    mp_last = disp_sb_line((int)Rows - 2, mp_last);
		    --scroll;
		}
	    }

	    if (scroll < 0 || (scroll == 0 && mp_last != NULL))
	    {
		/* displayed the requested text, more prompt again */
		screen_fill((int)Rows - 1, (int)Rows, 0,
						   (int)Columns, ' ', ' ', 0);
		msg_moremsg(FALSE);
		continue;
	    }

	    /* display more text, return to caller */
	    lines_left = scroll;
	}

	break;
    }

    /* clear the --more-- message */
    screen_fill((int)Rows - 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
    State = oldState;
#ifdef FEAT_MOUSE
    setmouse();
#endif
    if (quit_more)
    {
	msg_row = Rows - 1;
	msg_col = 0;
    }
#ifdef FEAT_RIGHTLEFT
    else if (cmdmsg_rl)
	msg_col = Columns - 1;
#endif

#ifdef FEAT_CON_DIALOG
    return retval;
#else
    return FALSE;
#endif
}

#if defined(USE_MCH_ERRMSG) || defined(PROTO)

#ifdef mch_errmsg
# undef mch_errmsg
#endif
#ifdef mch_msg
# undef mch_msg
#endif

/*
 * Give an error message.  To be used when the screen hasn't been initialized
 * yet.  When stderr can't be used, collect error messages until the GUI has
 * started and they can be displayed in a message box.
 */
    void
mch_errmsg(str)
    char	*str;
{
    int		len;

#if (defined(UNIX) || defined(FEAT_GUI)) && !defined(ALWAYS_USE_GUI)
    /* On Unix use stderr if it's a tty.
     * When not going to start the GUI also use stderr.
     * On Mac, when started from Finder, stderr is the console. */
    if (
# ifdef UNIX
#  ifdef MACOS_X_UNIX
	    (isatty(2) && strcmp("/dev/console", ttyname(2)) != 0)
#  else
	    isatty(2)
#  endif
#  ifdef FEAT_GUI
	    ||
#  endif
# endif
# ifdef FEAT_GUI
	    !(gui.in_use || gui.starting)
# endif
	    )
    {
	fprintf(stderr, "%s", str);
	return;
    }
#endif

    /* avoid a delay for a message that isn't there */
    emsg_on_display = FALSE;

    len = (int)STRLEN(str) + 1;
    if (error_ga.ga_growsize == 0)
    {
	error_ga.ga_growsize = 80;
	error_ga.ga_itemsize = 1;
    }
    if (ga_grow(&error_ga, len) == OK)
    {
	mch_memmove((char_u *)error_ga.ga_data + error_ga.ga_len,
							  (char_u *)str, len);
#ifdef UNIX
	/* remove CR characters, they are displayed */
	{
	    char_u	*p;

	    p = (char_u *)error_ga.ga_data + error_ga.ga_len;
	    for (;;)
	    {
		p = vim_strchr(p, '\r');
		if (p == NULL)
		    break;
		*p = ' ';
	    }
	}
#endif
	--len;		/* don't count the NUL at the end */
	error_ga.ga_len += len;
    }
}

/*
 * Give a message.  To be used when the screen hasn't been initialized yet.
 * When there is no tty, collect messages until the GUI has started and they
 * can be displayed in a message box.
 */
    void
mch_msg(str)
    char	*str;
{
#if (defined(UNIX) || defined(FEAT_GUI)) && !defined(ALWAYS_USE_GUI)
    /* On Unix use stdout if we have a tty.  This allows "vim -h | more" and
     * uses mch_errmsg() when started from the desktop.
     * When not going to start the GUI also use stdout.
     * On Mac, when started from Finder, stderr is the console. */
    if (
#  ifdef UNIX
#   ifdef MACOS_X_UNIX
	    (isatty(2) && strcmp("/dev/console", ttyname(2)) != 0)
#   else
	    isatty(2)
#    endif
#   ifdef FEAT_GUI
	    ||
#   endif
#  endif
#  ifdef FEAT_GUI
	    !(gui.in_use || gui.starting)
#  endif
	    )
    {
	printf("%s", str);
	return;
    }
# endif
    mch_errmsg(str);
}
#endif /* USE_MCH_ERRMSG */

/*
 * Put a character on the screen at the current message position and advance
 * to the next position.  Only for printable ASCII!
 */
    static void
msg_screen_putchar(c, attr)
    int		c;
    int		attr;
{
    msg_didout = TRUE;		/* remember that line is not empty */
    screen_putchar(c, msg_row, msg_col, attr);
#ifdef FEAT_RIGHTLEFT
    if (cmdmsg_rl)
    {
	if (--msg_col == 0)
	{
	    msg_col = Columns;
	    ++msg_row;
	}
    }
    else
#endif
    {
	if (++msg_col >= Columns)
	{
	    msg_col = 0;
	    ++msg_row;
	}
    }
}

    void
msg_moremsg(full)
    int	    full;
{
    int		attr;
    char_u	*s = (char_u *)_("-- More --");

    attr = hl_attr(HLF_M);
    screen_puts(s, (int)Rows - 1, 0, attr);
    if (full)
	screen_puts((char_u *)
		_(" SPACE/d/j: screen/page/line down, b/u/k: up, q: quit "),
		(int)Rows - 1, vim_strsize(s), attr);
}

/*
 * Repeat the message for the current mode: ASKMORE, EXTERNCMD, CONFIRM or
 * exmode_active.
 */
    void
repeat_message()
{
    if (State == ASKMORE)
    {
	msg_moremsg(TRUE);	/* display --more-- message again */
	msg_row = Rows - 1;
    }
#ifdef FEAT_CON_DIALOG
    else if (State == CONFIRM)
    {
	display_confirm_msg();	/* display ":confirm" message again */
	msg_row = Rows - 1;
    }
#endif
    else if (State == EXTERNCMD)
    {
	windgoto(msg_row, msg_col); /* put cursor back */
    }
    else if (State == HITRETURN || State == SETWSIZE)
    {
	hit_return_msg();
	msg_row = Rows - 1;
    }
}

/*
 * msg_check_screen - check if the screen is initialized.
 * Also check msg_row and msg_col, if they are too big it may cause a crash.
 * While starting the GUI the terminal codes will be set for the GUI, but the
 * output goes to the terminal.  Don't use the terminal codes then.
 */
    static int
msg_check_screen()
{
    if (!full_screen || !screen_valid(FALSE))
	return FALSE;

    if (msg_row >= Rows)
	msg_row = Rows - 1;
    if (msg_col >= Columns)
	msg_col = Columns - 1;
    return TRUE;
}

/*
 * Clear from current message position to end of screen.
 * Skip this when ":silent" was used, no need to clear for redirection.
 */
    void
msg_clr_eos()
{
    if (msg_silent == 0)
	msg_clr_eos_force();
}

/*
 * Clear from current message position to end of screen.
 * Note: msg_col is not updated, so we remember the end of the message
 * for msg_check().
 */
    void
msg_clr_eos_force()
{
    if (msg_use_printf())
    {
	if (full_screen)	/* only when termcap codes are valid */
	{
	    if (*T_CD)
		out_str(T_CD);	/* clear to end of display */
	    else if (*T_CE)
		out_str(T_CE);	/* clear to end of line */
	}
    }
    else
    {
#ifdef FEAT_RIGHTLEFT
	if (cmdmsg_rl)
	{
	    screen_fill(msg_row, msg_row + 1, 0, msg_col + 1, ' ', ' ', 0);
	    screen_fill(msg_row + 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
	}
	else
#endif
	{
	    screen_fill(msg_row, msg_row + 1, msg_col, (int)Columns,
								 ' ', ' ', 0);
	    screen_fill(msg_row + 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
	}
    }
}

/*
 * Clear the command line.
 */
    void
msg_clr_cmdline()
{
    msg_row = cmdline_row;
    msg_col = 0;
    msg_clr_eos_force();
}

/*
 * end putting a message on the screen
 * call wait_return if the message does not fit in the available space
 * return TRUE if wait_return not called.
 */
    int
msg_end()
{
    /*
     * if the string is larger than the window,
     * or the ruler option is set and we run into it,
     * we have to redraw the window.
     * Do not do this if we are abandoning the file or editing the command line.
     */
    if (!exiting && need_wait_return && !(State & CMDLINE))
    {
	wait_return(FALSE);
	return FALSE;
    }
    out_flush();
    return TRUE;
}

/*
 * If the written message runs into the shown command or ruler, we have to
 * wait for hit-return and redraw the window later.
 */
    void
msg_check()
{
    if (msg_row == Rows - 1 && msg_col >= sc_col)
    {
	need_wait_return = TRUE;
	redraw_cmdline = TRUE;
    }
}

/*
 * May write a string to the redirection file.
 * When "maxlen" is -1 write the whole string, otherwise up to "maxlen" bytes.
 */
    static void
redir_write(str, maxlen)
    char_u	*str;
    int		maxlen;
{
    char_u	*s = str;
    static int	cur_col = 0;

    /* Don't do anything for displaying prompts and the like. */
    if (redir_off)
	return;

    /*
     * If 'verbosefile' is set write message in that file.
     * Must come before the rest because of updating "msg_col".
     */
    if (*p_vfile != NUL)
	verbose_write(s, maxlen);

    if (redir_fd != NULL
#ifdef FEAT_EVAL
			  || redir_reg || redir_vname
#endif
				       )
    {
	/* If the string doesn't start with CR or NL, go to msg_col */
	if (*s != '\n' && *s != '\r')
	{
	    while (cur_col < msg_col)
	    {
#ifdef FEAT_EVAL
		if (redir_reg)
		    write_reg_contents(redir_reg, (char_u *)" ", -1, TRUE);
		else if (redir_vname)
		    var_redir_str((char_u *)" ", -1);
		else if (redir_fd)
#endif
		    fputs(" ", redir_fd);
		++cur_col;
	    }
	}

#ifdef FEAT_EVAL
	if (redir_reg)
	    write_reg_contents(redir_reg, s, maxlen, TRUE);
	if (redir_vname)
	    var_redir_str(s, maxlen);
#endif

	/* Adjust the current column */
	while (*s != NUL && (maxlen < 0 || (int)(s - str) < maxlen))
	{
#ifdef FEAT_EVAL
	    if (!redir_reg && !redir_vname && redir_fd != NULL)
#endif
		putc(*s, redir_fd);
	    if (*s == '\r' || *s == '\n')
		cur_col = 0;
	    else if (*s == '\t')
		cur_col += (8 - cur_col % 8);
	    else
		++cur_col;
	    ++s;
	}

	if (msg_silent != 0)	/* should update msg_col */
	    msg_col = cur_col;
    }
}

/*
 * Before giving verbose messsage.
 * Must always be called paired with verbose_leave()!
 */
    void
verbose_enter()
{
    if (*p_vfile != NUL)
	++msg_silent;
}

/*
 * After giving verbose message.
 * Must always be called paired with verbose_enter()!
 */
    void
verbose_leave()
{
    if (*p_vfile != NUL)
	if (--msg_silent < 0)
	    msg_silent = 0;
}

/*
 * Like verbose_enter() and set msg_scroll when displaying the message.
 */
    void
verbose_enter_scroll()
{
    if (*p_vfile != NUL)
	++msg_silent;
    else
	/* always scroll up, don't overwrite */
	msg_scroll = TRUE;
}

/*
 * Like verbose_leave() and set cmdline_row when displaying the message.
 */
    void
verbose_leave_scroll()
{
    if (*p_vfile != NUL)
    {
	if (--msg_silent < 0)
	    msg_silent = 0;
    }
    else
	cmdline_row = msg_row;
}

static FILE *verbose_fd = NULL;
static int  verbose_did_open = FALSE;

/*
 * Called when 'verbosefile' is set: stop writing to the file.
 */
    void
verbose_stop()
{
    if (verbose_fd != NULL)
    {
	fclose(verbose_fd);
	verbose_fd = NULL;
    }
    verbose_did_open = FALSE;
}

/*
 * Open the file 'verbosefile'.
 * Return FAIL or OK.
 */
    int
verbose_open()
{
    if (verbose_fd == NULL && !verbose_did_open)
    {
	/* Only give the error message once. */
	verbose_did_open = TRUE;

	verbose_fd = mch_fopen((char *)p_vfile, "a");
	if (verbose_fd == NULL)
	{
	    EMSG2(_(e_notopen), p_vfile);
	    return FAIL;
	}
    }
    return OK;
}

/*
 * Write a string to 'verbosefile'.
 * When "maxlen" is -1 write the whole string, otherwise up to "maxlen" bytes.
 */
    static void
verbose_write(str, maxlen)
    char_u	*str;
    int		maxlen;
{
    char_u	*s = str;
    static int	cur_col = 0;

    /* Open the file when called the first time. */
    if (verbose_fd == NULL)
	verbose_open();

    if (verbose_fd != NULL)
    {
	/* If the string doesn't start with CR or NL, go to msg_col */
	if (*s != '\n' && *s != '\r')
	{
	    while (cur_col < msg_col)
	    {
		fputs(" ", verbose_fd);
		++cur_col;
	    }
	}

	/* Adjust the current column */
	while (*s != NUL && (maxlen < 0 || (int)(s - str) < maxlen))
	{
	    putc(*s, verbose_fd);
	    if (*s == '\r' || *s == '\n')
		cur_col = 0;
	    else if (*s == '\t')
		cur_col += (8 - cur_col % 8);
	    else
		++cur_col;
	    ++s;
	}
    }
}

/*
 * Give a warning message (for searching).
 * Use 'w' highlighting and may repeat the message after redrawing
 */
    void
give_warning(message, hl)
    char_u  *message;
    int	    hl;
{
    /* Don't do this for ":silent". */
    if (msg_silent != 0)
	return;

    /* Don't want a hit-enter prompt here. */
    ++no_wait_return;

#ifdef FEAT_EVAL
    set_vim_var_string(VV_WARNINGMSG, message, -1);
#endif
    vim_free(keep_msg);
    keep_msg = NULL;
    if (hl)
	keep_msg_attr = hl_attr(HLF_W);
    else
	keep_msg_attr = 0;
    if (msg_attr(message, keep_msg_attr) && msg_scrolled == 0)
	set_keep_msg(message, keep_msg_attr);
    msg_didout = FALSE;	    /* overwrite this message */
    msg_nowait = TRUE;	    /* don't wait for this message */
    msg_col = 0;

    --no_wait_return;
}

/*
 * Advance msg cursor to column "col".
 */
    void
msg_advance(col)
    int	    col;
{
    if (msg_silent != 0)	/* nothing to advance to */
    {
	msg_col = col;		/* for redirection, may fill it up later */
	return;
    }
    if (col >= Columns)		/* not enough room */
	col = Columns - 1;
#ifdef FEAT_RIGHTLEFT
    if (cmdmsg_rl)
	while (msg_col > Columns - col)
	    msg_putchar(' ');
    else
#endif
	while (msg_col < col)
	    msg_putchar(' ');
}

#if defined(FEAT_CON_DIALOG) || defined(PROTO)
/*
 * Used for "confirm()" function, and the :confirm command prefix.
 * Versions which haven't got flexible dialogs yet, and console
 * versions, get this generic handler which uses the command line.
 *
 * type  = one of:
 *	   VIM_QUESTION, VIM_INFO, VIM_WARNING, VIM_ERROR or VIM_GENERIC
 * title = title string (can be NULL for default)
 * (neither used in console dialogs at the moment)
 *
 * Format of the "buttons" string:
 * "Button1Name\nButton2Name\nButton3Name"
 * The first button should normally be the default/accept
 * The second button should be the 'Cancel' button
 * Other buttons- use your imagination!
 * A '&' in a button name becomes a shortcut, so each '&' should be before a
 * different letter.
 */
/* ARGSUSED */
    int
do_dialog(type, title, message, buttons, dfltbutton, textfield)
    int		type;
    char_u	*title;
    char_u	*message;
    char_u	*buttons;
    int		dfltbutton;
    char_u	*textfield;	/* IObuff for inputdialog(), NULL otherwise */
{
    int		oldState;
    int		retval = 0;
    char_u	*hotkeys;
    int		c;
    int		i;

#ifndef NO_CONSOLE
    /* Don't output anything in silent mode ("ex -s") */
    if (silent_mode)
	return dfltbutton;   /* return default option */
#endif

#ifdef FEAT_GUI_DIALOG
    /* When GUI is running and 'c' not in 'guioptions', use the GUI dialog */
    if (gui.in_use && vim_strchr(p_go, GO_CONDIALOG) == NULL)
    {
	c = gui_mch_dialog(type, title, message, buttons, dfltbutton,
								   textfield);
	msg_end_prompt();

	/* Flush output to avoid that further messages and redrawing is done
	 * in the wrong order. */
	out_flush();
	gui_mch_update();

	return c;
    }
#endif

    oldState = State;
    State = CONFIRM;
#ifdef FEAT_MOUSE
    setmouse();
#endif

    /*
     * Since we wait for a keypress, don't make the
     * user press RETURN as well afterwards.
     */
    ++no_wait_return;
    hotkeys = msg_show_console_dialog(message, buttons, dfltbutton);

    if (hotkeys != NULL)
    {
	for (;;)
	{
	    /* Get a typed character directly from the user. */
	    c = get_keystroke();
	    switch (c)
	    {
	    case CAR:		/* User accepts default option */
	    case NL:
		retval = dfltbutton;
		break;
	    case Ctrl_C:	/* User aborts/cancels */
	    case ESC:
		retval = 0;
		break;
	    default:		/* Could be a hotkey? */
		if (c < 0)	/* special keys are ignored here */
		    continue;
		/* Make the character lowercase, as chars in "hotkeys" are. */
		c = MB_TOLOWER(c);
		retval = 1;
		for (i = 0; hotkeys[i]; ++i)
		{
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			if ((*mb_ptr2char)(hotkeys + i) == c)
			    break;
			i += (*mb_ptr2len)(hotkeys + i) - 1;
		    }
		    else
#endif
			if (hotkeys[i] == c)
			    break;
		    ++retval;
		}
		if (hotkeys[i])
		    break;
		/* No hotkey match, so keep waiting */
		continue;
	    }
	    break;
	}

	vim_free(hotkeys);
    }

    State = oldState;
#ifdef FEAT_MOUSE
    setmouse();
#endif
    --no_wait_return;
    msg_end_prompt();

    return retval;
}

static int copy_char __ARGS((char_u *from, char_u *to, int lowercase));

/*
 * Copy one character from "*from" to "*to", taking care of multi-byte
 * characters.  Return the length of the character in bytes.
 */
    static int
copy_char(from, to, lowercase)
    char_u	*from;
    char_u	*to;
    int		lowercase;	/* make character lower case */
{
#ifdef FEAT_MBYTE
    int		len;
    int		c;

    if (has_mbyte)
    {
	if (lowercase)
	{
	    c = MB_TOLOWER((*mb_ptr2char)(from));
	    return (*mb_char2bytes)(c, to);
	}
	else
	{
	    len = (*mb_ptr2len)(from);
	    mch_memmove(to, from, (size_t)len);
	    return len;
	}
    }
    else
#endif
    {
	if (lowercase)
	    *to = (char_u)TOLOWER_LOC(*from);
	else
	    *to = *from;
	return 1;
    }
}

/*
 * Format the dialog string, and display it at the bottom of
 * the screen. Return a string of hotkey chars (if defined) for
 * each 'button'. If a button has no hotkey defined, the first character of
 * the button is used.
 * The hotkeys can be multi-byte characters, but without combining chars.
 *
 * Returns an allocated string with hotkeys, or NULL for error.
 */
    static char_u *
msg_show_console_dialog(message, buttons, dfltbutton)
    char_u	*message;
    char_u	*buttons;
    int		dfltbutton;
{
    int		len = 0;
#ifdef FEAT_MBYTE
# define HOTK_LEN (has_mbyte ? MB_MAXBYTES : 1)
#else
# define HOTK_LEN 1
#endif
    int		lenhotkey = HOTK_LEN;	/* count first button */
    char_u	*hotk = NULL;
    char_u	*msgp = NULL;
    char_u	*hotkp = NULL;
    char_u	*r;
    int		copy;
#define HAS_HOTKEY_LEN 30
    char_u	has_hotkey[HAS_HOTKEY_LEN];
    int		first_hotkey = FALSE;	/* first char of button is hotkey */
    int		idx;

    has_hotkey[0] = FALSE;

    /*
     * First loop: compute the size of memory to allocate.
     * Second loop: copy to the allocated memory.
     */
    for (copy = 0; copy <= 1; ++copy)
    {
	r = buttons;
	idx = 0;
	while (*r)
	{
	    if (*r == DLG_BUTTON_SEP)
	    {
		if (copy)
		{
		    *msgp++ = ',';
		    *msgp++ = ' ';	    /* '\n' -> ', ' */

		    /* advance to next hotkey and set default hotkey */
#ifdef FEAT_MBYTE
		    if (has_mbyte)
			hotkp += (*mb_ptr2len)(hotkp);
		    else
#endif
			++hotkp;
		    (void)copy_char(r + 1, hotkp, TRUE);
		    if (dfltbutton)
			--dfltbutton;

		    /* If no hotkey is specified first char is used. */
		    if (idx < HAS_HOTKEY_LEN - 1 && !has_hotkey[++idx])
			first_hotkey = TRUE;
		}
		else
		{
		    len += 3;		    /* '\n' -> ', '; 'x' -> '(x)' */
		    lenhotkey += HOTK_LEN;  /* each button needs a hotkey */
		    if (idx < HAS_HOTKEY_LEN - 1)
			has_hotkey[++idx] = FALSE;
		}
	    }
	    else if (*r == DLG_HOTKEY_CHAR || first_hotkey)
	    {
		if (*r == DLG_HOTKEY_CHAR)
		    ++r;
		first_hotkey = FALSE;
		if (copy)
		{
		    if (*r == DLG_HOTKEY_CHAR)		/* '&&a' -> '&a' */
			*msgp++ = *r;
		    else
		    {
			/* '&a' -> '[a]' */
			*msgp++ = (dfltbutton == 1) ? '[' : '(';
			msgp += copy_char(r, msgp, FALSE);
			*msgp++ = (dfltbutton == 1) ? ']' : ')';

			/* redefine hotkey */
			(void)copy_char(r, hotkp, TRUE);
		    }
		}
		else
		{
		    ++len;	    /* '&a' -> '[a]' */
		    if (idx < HAS_HOTKEY_LEN - 1)
			has_hotkey[idx] = TRUE;
		}
	    }
	    else
	    {
		/* everything else copy literally */
		if (copy)
		    msgp += copy_char(r, msgp, FALSE);
	    }

	    /* advance to the next character */
	    mb_ptr_adv(r);
	}

	if (copy)
	{
	    *msgp++ = ':';
	    *msgp++ = ' ';
	    *msgp = NUL;
	    mb_ptr_adv(hotkp);
	    *hotkp = NUL;
	}
	else
	{
	    len += (int)(STRLEN(message)
			+ 2			/* for the NL's */
			+ STRLEN(buttons)
			+ 3);			/* for the ": " and NUL */
	    lenhotkey++;			/* for the NUL */

	    /* If no hotkey is specified first char is used. */
	    if (!has_hotkey[0])
	    {
		first_hotkey = TRUE;
		len += 2;		/* "x" -> "[x]" */
	    }

	    /*
	     * Now allocate and load the strings
	     */
	    vim_free(confirm_msg);
	    confirm_msg = alloc(len);
	    if (confirm_msg == NULL)
		return NULL;
	    *confirm_msg = NUL;
	    hotk = alloc(lenhotkey);
	    if (hotk == NULL)
		return NULL;

	    *confirm_msg = '\n';
	    STRCPY(confirm_msg + 1, message);

	    msgp = confirm_msg + 1 + STRLEN(message);
	    hotkp = hotk;

	    /* define first default hotkey */
	    (void)copy_char(buttons, hotkp, TRUE);

	    /* Remember where the choices start, displaying starts here when
	     * "hotkp" typed at the more prompt. */
	    confirm_msg_tail = msgp;
	    *msgp++ = '\n';
	}
    }

    display_confirm_msg();
    return hotk;
}

/*
 * Display the ":confirm" message.  Also called when screen resized.
 */
    void
display_confirm_msg()
{
    /* avoid that 'q' at the more prompt truncates the message here */
    ++confirm_msg_used;
    if (confirm_msg != NULL)
	msg_puts_attr(confirm_msg, hl_attr(HLF_M));
    --confirm_msg_used;
}

#endif /* FEAT_CON_DIALOG */

#if defined(FEAT_CON_DIALOG) || defined(FEAT_GUI_DIALOG)

    int
vim_dialog_yesno(type, title, message, dflt)
    int		type;
    char_u	*title;
    char_u	*message;
    int		dflt;
{
    if (do_dialog(type,
		title == NULL ? (char_u *)_("Question") : title,
		message,
		(char_u *)_("&Yes\n&No"), dflt, NULL) == 1)
	return VIM_YES;
    return VIM_NO;
}

    int
vim_dialog_yesnocancel(type, title, message, dflt)
    int		type;
    char_u	*title;
    char_u	*message;
    int		dflt;
{
    switch (do_dialog(type,
		title == NULL ? (char_u *)_("Question") : title,
		message,
		(char_u *)_("&Yes\n&No\n&Cancel"), dflt, NULL))
    {
	case 1: return VIM_YES;
	case 2: return VIM_NO;
    }
    return VIM_CANCEL;
}

    int
vim_dialog_yesnoallcancel(type, title, message, dflt)
    int		type;
    char_u	*title;
    char_u	*message;
    int		dflt;
{
    switch (do_dialog(type,
		title == NULL ? (char_u *)"Question" : title,
		message,
		(char_u *)_("&Yes\n&No\nSave &All\n&Discard All\n&Cancel"),
								  dflt, NULL))
    {
	case 1: return VIM_YES;
	case 2: return VIM_NO;
	case 3: return VIM_ALL;
	case 4: return VIM_DISCARDALL;
    }
    return VIM_CANCEL;
}

#endif /* FEAT_GUI_DIALOG || FEAT_CON_DIALOG */

#if defined(FEAT_BROWSE) || defined(PROTO)
/*
 * Generic browse function.  Calls gui_mch_browse() when possible.
 * Later this may pop-up a non-GUI file selector (external command?).
 */
    char_u *
do_browse(flags, title, dflt, ext, initdir, filter, buf)
    int		flags;		/* BROWSE_SAVE and BROWSE_DIR */
    char_u	*title;		/* title for the window */
    char_u	*dflt;		/* default file name (may include directory) */
    char_u	*ext;		/* extension added */
    char_u	*initdir;	/* initial directory, NULL for current dir or
				   when using path from "dflt" */
    char_u	*filter;	/* file name filter */
    buf_T	*buf;		/* buffer to read/write for */
{
    char_u		*fname;
    static char_u	*last_dir = NULL;    /* last used directory */
    char_u		*tofree = NULL;
    int			save_browse = cmdmod.browse;

    /* Must turn off browse to avoid that autocommands will get the
     * flag too!  */
    cmdmod.browse = FALSE;

    if (title == NULL || *title == NUL)
    {
	if (flags & BROWSE_DIR)
	    title = (char_u *)_("Select Directory dialog");
	else if (flags & BROWSE_SAVE)
	    title = (char_u *)_("Save File dialog");
	else
	    title = (char_u *)_("Open File dialog");
    }

    /* When no directory specified, use default file name, default dir, buffer
     * dir, last dir or current dir */
    if ((initdir == NULL || *initdir == NUL) && dflt != NULL && *dflt != NUL)
    {
	if (mch_isdir(dflt))		/* default file name is a directory */
	{
	    initdir = dflt;
	    dflt = NULL;
	}
	else if (gettail(dflt) != dflt)	/* default file name includes a path */
	{
	    tofree = vim_strsave(dflt);
	    if (tofree != NULL)
	    {
		initdir = tofree;
		*gettail(initdir) = NUL;
		dflt = gettail(dflt);
	    }
	}
    }

    if (initdir == NULL || *initdir == NUL)
    {
	/* When 'browsedir' is a directory, use it */
	if (STRCMP(p_bsdir, "last") != 0
		&& STRCMP(p_bsdir, "buffer") != 0
		&& STRCMP(p_bsdir, "current") != 0
		&& mch_isdir(p_bsdir))
	    initdir = p_bsdir;
	/* When saving or 'browsedir' is "buffer", use buffer fname */
	else if (((flags & BROWSE_SAVE) || *p_bsdir == 'b')
		&& buf != NULL && buf->b_ffname != NULL)
	{
	    if (dflt == NULL || *dflt == NUL)
		dflt = gettail(curbuf->b_ffname);
	    tofree = vim_strsave(curbuf->b_ffname);
	    if (tofree != NULL)
	    {
		initdir = tofree;
		*gettail(initdir) = NUL;
	    }
	}
	/* When 'browsedir' is "last", use dir from last browse */
	else if (*p_bsdir == 'l')
	    initdir = last_dir;
	/* When 'browsedir is "current", use current directory.  This is the
	 * default already, leave initdir empty. */
    }

# ifdef FEAT_GUI
    if (gui.in_use)		/* when this changes, also adjust f_has()! */
    {
	if (filter == NULL
#  ifdef FEAT_EVAL
		&& (filter = get_var_value((char_u *)"b:browsefilter")) == NULL
		&& (filter = get_var_value((char_u *)"g:browsefilter")) == NULL
#  endif
	)
	    filter = BROWSE_FILTER_DEFAULT;
	if (flags & BROWSE_DIR)
	{
#  if defined(HAVE_GTK2) || defined(WIN3264)
	    /* For systems that have a directory dialog. */
	    fname = gui_mch_browsedir(title, initdir);
#  else
	    /* Generic solution for selecting a directory: select a file and
	     * remove the file name. */
	    fname = gui_mch_browse(0, title, dflt, ext, initdir, (char_u *)"");
#  endif
#  if !defined(HAVE_GTK2)
	    /* Win32 adds a dummy file name, others return an arbitrary file
	     * name.  GTK+ 2 returns only the directory, */
	    if (fname != NULL && *fname != NUL && !mch_isdir(fname))
	    {
		/* Remove the file name. */
		char_u	    *tail = gettail_sep(fname);

		if (tail == fname)
		    *tail++ = '.';	/* use current dir */
		*tail = NUL;
	    }
#  endif
	}
	else
	    fname = gui_mch_browse(flags & BROWSE_SAVE,
					   title, dflt, ext, initdir, filter);

	/* We hang around in the dialog for a while, the user might do some
	 * things to our files.  The Win32 dialog allows deleting or renaming
	 * a file, check timestamps. */
	need_check_timestamps = TRUE;
	did_check_timestamps = FALSE;
    }
    else
# endif
    {
	/* TODO: non-GUI file selector here */
	EMSG(_("E338: Sorry, no file browser in console mode"));
	fname = NULL;
    }

    /* keep the directory for next time */
    if (fname != NULL)
    {
	vim_free(last_dir);
	last_dir = vim_strsave(fname);
	if (last_dir != NULL && !(flags & BROWSE_DIR))
	{
	    *gettail(last_dir) = NUL;
	    if (*last_dir == NUL)
	    {
		/* filename only returned, must be in current dir */
		vim_free(last_dir);
		last_dir = alloc(MAXPATHL);
		if (last_dir != NULL)
		    mch_dirname(last_dir, MAXPATHL);
	    }
	}
    }

    vim_free(tofree);
    cmdmod.browse = save_browse;

    return fname;
}
#endif

#if defined(HAVE_STDARG_H) && defined(FEAT_EVAL)
static char *e_printf = N_("E766: Insufficient arguments for printf()");

static long tv_nr __ARGS((typval_T *tvs, int *idxp));
static char *tv_str __ARGS((typval_T *tvs, int *idxp));

/*
 * Get number argument from "idxp" entry in "tvs".  First entry is 1.
 */
    static long
tv_nr(tvs, idxp)
    typval_T	*tvs;
    int		*idxp;
{
    int		idx = *idxp - 1;
    long	n = 0;
    int		err = FALSE;

    if (tvs[idx].v_type == VAR_UNKNOWN)
	EMSG(_(e_printf));
    else
    {
	++*idxp;
	n = get_tv_number_chk(&tvs[idx], &err);
	if (err)
	    n = 0;
    }
    return n;
}

/*
 * Get string argument from "idxp" entry in "tvs".  First entry is 1.
 * Returns NULL for an error.
 */
    static char *
tv_str(tvs, idxp)
    typval_T	*tvs;
    int		*idxp;
{
    int		idx = *idxp - 1;
    char	*s = NULL;

    if (tvs[idx].v_type == VAR_UNKNOWN)
	EMSG(_(e_printf));
    else
    {
	++*idxp;
	s = (char *)get_tv_string_chk(&tvs[idx]);
    }
    return s;
}
#endif

/*
 * This code was included to provide a portable vsnprintf() and snprintf().
 * Some systems may provide their own, but we always use this one for
 * consistency.
 *
 * This code is based on snprintf.c - a portable implementation of snprintf
 * by Mark Martinec <mark.martinec@ijs.si>, Version 2.2, 2000-10-06.
 * Included with permission.  It was heavely modified to fit in Vim.
 * The original code, including useful comments, can be found here:
 *	http://www.ijs.si/software/snprintf/
 *
 * This snprintf() only supports the following conversion specifiers:
 * s, c, d, u, o, x, X, p  (and synonyms: i, D, U, O - see below)
 * with flags: '-', '+', ' ', '0' and '#'.
 * An asterisk is supported for field width as well as precision.
 *
 * Length modifiers 'h' (short int) and 'l' (long int) are supported.
 * 'll' (long long int) is not supported.
 *
 * The locale is not used, the string is used as a byte string.  This is only
 * relevant for double-byte encodings where the second byte may be '%'.
 *
 * It is permitted for "str_m" to be zero, and it is permitted to specify NULL
 * pointer for resulting string argument if "str_m" is zero (as per ISO C99).
 *
 * The return value is the number of characters which would be generated
 * for the given input, excluding the trailing null. If this value
 * is greater or equal to "str_m", not all characters from the result
 * have been stored in str, output bytes beyond the ("str_m"-1) -th character
 * are discarded. If "str_m" is greater than zero it is guaranteed
 * the resulting string will be null-terminated.
 */

/*
 * When va_list is not supported we only define vim_snprintf().
 *
 * vim_vsnprintf() can be invoked with either "va_list" or a list of
 * "typval_T".  When the latter is not used it must be NULL.
 */

/* When generating prototypes all of this is skipped, cproto doesn't
 * understand this. */
#ifndef PROTO
# ifdef HAVE_STDARG_H
    int
vim_snprintf(char *str, size_t str_m, char *fmt, ...)
{
    va_list	ap;
    int		str_l;

    va_start(ap, fmt);
    str_l = vim_vsnprintf(str, str_m, fmt, ap, NULL);
    va_end(ap);
    return str_l;
}

    int
vim_vsnprintf(str, str_m, fmt, ap, tvs)
# else
    /* clumsy way to work around missing va_list */
#  define get_a_arg(i) (++i, i == 2 ? a1 : i == 3 ? a2 : i == 4 ? a3 : i == 5 ? a4 : i == 6 ? a5 : i == 7 ? a6 : i == 8 ? a7 : i == 9 ? a8 : i == 10 ? a9 : a10)

/* VARARGS */
    int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
vim_snprintf(str, str_m, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
# endif
    char	*str;
    size_t	str_m;
    char	*fmt;
# ifdef HAVE_STDARG_H
    va_list	ap;
    typval_T	*tvs;
# else
    long	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
# endif
{
    size_t	str_l = 0;
    char	*p = fmt;
    int		arg_idx = 1;

    if (p == NULL)
	p = "";
    while (*p != NUL)
    {
	if (*p != '%')
	{
	    char    *q = strchr(p + 1, '%');
	    size_t  n = (q == NULL) ? STRLEN(p) : (q - p);

	    /* Copy up to the next '%' or NUL without any changes. */
	    if (str_l < str_m)
	    {
		size_t avail = str_m - str_l;

		mch_memmove(str + str_l, p, n > avail ? avail : n);
	    }
	    p += n;
	    str_l += n;
	}
	else
	{
	    size_t  min_field_width = 0, precision = 0;
	    int	    zero_padding = 0, precision_specified = 0, justify_left = 0;
	    int	    alternate_form = 0, force_sign = 0;

	    /* If both the ' ' and '+' flags appear, the ' ' flag should be
	     * ignored. */
	    int	    space_for_positive = 1;

	    /* allowed values: \0, h, l, L */
	    char    length_modifier = '\0';

	    /* temporary buffer for simple numeric->string conversion */
	    char    tmp[32];

	    /* string address in case of string argument */
	    char    *str_arg;

	    /* natural field width of arg without padding and sign */
	    size_t  str_arg_l;

	    /* unsigned char argument value - only defined for c conversion.
	     * N.B. standard explicitly states the char argument for the c
	     * conversion is unsigned */
	    unsigned char uchar_arg;

	    /* number of zeros to be inserted for numeric conversions as
	     * required by the precision or minimal field width */
	    size_t  number_of_zeros_to_pad = 0;

	    /* index into tmp where zero padding is to be inserted */
	    size_t  zero_padding_insertion_ind = 0;

	    /* current conversion specifier character */
	    char    fmt_spec = '\0';

	    str_arg = NULL;
	    p++;  /* skip '%' */

	    /* parse flags */
	    while (*p == '0' || *p == '-' || *p == '+' || *p == ' '
						   || *p == '#' || *p == '\'')
	    {
		switch (*p)
		{
		    case '0': zero_padding = 1; break;
		    case '-': justify_left = 1; break;
		    case '+': force_sign = 1; space_for_positive = 0; break;
		    case ' ': force_sign = 1;
			      /* If both the ' ' and '+' flags appear, the ' '
			       * flag should be ignored */
			      break;
		    case '#': alternate_form = 1; break;
		    case '\'': break;
		}
		p++;
	    }
	    /* If the '0' and '-' flags both appear, the '0' flag should be
	     * ignored. */

	    /* parse field width */
	    if (*p == '*')
	    {
		int j;

		p++;
		j =
#ifndef HAVE_STDARG_H
		    get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
		    tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
			va_arg(ap, int);
#endif
		if (j >= 0)
		    min_field_width = j;
		else
		{
		    min_field_width = -j;
		    justify_left = 1;
		}
	    }
	    else if (VIM_ISDIGIT((int)(*p)))
	    {
		/* size_t could be wider than unsigned int; make sure we treat
		 * argument like common implementations do */
		unsigned int uj = *p++ - '0';

		while (VIM_ISDIGIT((int)(*p)))
		    uj = 10 * uj + (unsigned int)(*p++ - '0');
		min_field_width = uj;
	    }

	    /* parse precision */
	    if (*p == '.')
	    {
		p++;
		precision_specified = 1;
		if (*p == '*')
		{
		    int j;

		    j =
#ifndef HAVE_STDARG_H
			get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
			tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
			    va_arg(ap, int);
#endif
		    p++;
		    if (j >= 0)
			precision = j;
		    else
		    {
			precision_specified = 0;
			precision = 0;
		    }
		}
		else if (VIM_ISDIGIT((int)(*p)))
		{
		    /* size_t could be wider than unsigned int; make sure we
		     * treat argument like common implementations do */
		    unsigned int uj = *p++ - '0';

		    while (VIM_ISDIGIT((int)(*p)))
			uj = 10 * uj + (unsigned int)(*p++ - '0');
		    precision = uj;
		}
	    }

	    /* parse 'h', 'l' and 'll' length modifiers */
	    if (*p == 'h' || *p == 'l')
	    {
		length_modifier = *p;
		p++;
		if (length_modifier == 'l' && *p == 'l')
		{
		    /* double l = long long */
		    length_modifier = 'l';	/* treat it as a single 'l' */
		    p++;
		}
	    }
	    fmt_spec = *p;

	    /* common synonyms: */
	    switch (fmt_spec)
	    {
		case 'i': fmt_spec = 'd'; break;
		case 'D': fmt_spec = 'd'; length_modifier = 'l'; break;
		case 'U': fmt_spec = 'u'; length_modifier = 'l'; break;
		case 'O': fmt_spec = 'o'; length_modifier = 'l'; break;
		default: break;
	    }

	    /* get parameter value, do initial processing */
	    switch (fmt_spec)
	    {
		/* '%' and 'c' behave similar to 's' regarding flags and field
		 * widths */
	    case '%':
	    case 'c':
	    case 's':
		length_modifier = '\0';
		zero_padding = 0;    /* turn zero padding off for string
					conversions */
		str_arg_l = 1;
		switch (fmt_spec)
		{
		case '%':
		    str_arg = p;
		    break;

		case 'c':
		    {
			int j;

			j =
#ifndef HAVE_STDARG_H
			    get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
			    tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
				va_arg(ap, int);
#endif
			/* standard demands unsigned char */
			uchar_arg = (unsigned char)j;
			str_arg = (char *)&uchar_arg;
			break;
		    }

		case 's':
		    str_arg =
#ifndef HAVE_STDARG_H
				(char *)get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
				tvs != NULL ? tv_str(tvs, &arg_idx) :
# endif
				    va_arg(ap, char *);
#endif
		    if (str_arg == NULL)
		    {
			str_arg = "[NULL]";
			str_arg_l = 6;
		    }
		    /* make sure not to address string beyond the specified
		     * precision !!! */
		    else if (!precision_specified)
			str_arg_l = strlen(str_arg);
		    /* truncate string if necessary as requested by precision */
		    else if (precision == 0)
			str_arg_l = 0;
		    else
		    {
			/* memchr on HP does not like n > 2^31  !!! */
			char *q = memchr(str_arg, '\0',
#if SIZEOF_INT <= 2
				precision
#else
				precision <= (size_t)0x7fffffffL ? precision
						       : (size_t)0x7fffffffL
#endif
						       );
			str_arg_l = (q == NULL) ? precision : q - str_arg;
		    }
		    break;

		default:
		    break;
		}
		break;

	    case 'd': case 'u': case 'o': case 'x': case 'X': case 'p':
		{
		    /* NOTE: the u, o, x, X and p conversion specifiers
		     * imply the value is unsigned;  d implies a signed
		     * value */

		    /* 0 if numeric argument is zero (or if pointer is
		     * NULL for 'p'), +1 if greater than zero (or nonzero
		     * for unsigned arguments), -1 if negative (unsigned
		     * argument is never negative) */
		    int arg_sign = 0;

		    /* only defined for length modifier h, or for no
		     * length modifiers */
		    int int_arg = 0;
		    unsigned int uint_arg = 0;

		    /* only defined for length modifier l */
		    long int long_arg = 0;
		    unsigned long int ulong_arg = 0;

		    /* pointer argument value -only defined for p
		     * conversion */
		    void *ptr_arg = NULL;

		    if (fmt_spec == 'p')
		    {
			length_modifier = '\0';
			ptr_arg =
#ifndef HAVE_STDARG_H
				 (void *)get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
				 tvs != NULL ? (void *)tv_str(tvs, &arg_idx) :
# endif
					va_arg(ap, void *);
#endif
			if (ptr_arg != NULL)
			    arg_sign = 1;
		    }
		    else if (fmt_spec == 'd')
		    {
			/* signed */
			switch (length_modifier)
			{
			case '\0':
			case 'h':
			    /* char and short arguments are passed as int. */
			    int_arg =
#ifndef HAVE_STDARG_H
					get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
					tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
					    va_arg(ap, int);
#endif
			    if (int_arg > 0)
				arg_sign =  1;
			    else if (int_arg < 0)
				arg_sign = -1;
			    break;
			case 'l':
			    long_arg =
#ifndef HAVE_STDARG_H
					get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
					tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
					    va_arg(ap, long int);
#endif
			    if (long_arg > 0)
				arg_sign =  1;
			    else if (long_arg < 0)
				arg_sign = -1;
			    break;
			}
		    }
		    else
		    {
			/* unsigned */
			switch (length_modifier)
			{
			    case '\0':
			    case 'h':
				uint_arg =
#ifndef HAVE_STDARG_H
					    get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
					    tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
						va_arg(ap, unsigned int);
#endif
				if (uint_arg != 0)
				    arg_sign = 1;
				break;
			    case 'l':
				ulong_arg =
#ifndef HAVE_STDARG_H
					    get_a_arg(arg_idx);
#else
# if defined(FEAT_EVAL)
					    tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
						va_arg(ap, unsigned long int);
#endif
				if (ulong_arg != 0)
				    arg_sign = 1;
				break;
			}
		    }

		    str_arg = tmp;
		    str_arg_l = 0;

		    /* NOTE:
		     *   For d, i, u, o, x, and X conversions, if precision is
		     *   specified, the '0' flag should be ignored. This is so
		     *   with Solaris 2.6, Digital UNIX 4.0, HPUX 10, Linux,
		     *   FreeBSD, NetBSD; but not with Perl.
		     */
		    if (precision_specified)
			zero_padding = 0;
		    if (fmt_spec == 'd')
		    {
			if (force_sign && arg_sign >= 0)
			    tmp[str_arg_l++] = space_for_positive ? ' ' : '+';
			/* leave negative numbers for sprintf to handle, to
			 * avoid handling tricky cases like (short int)-32768 */
		    }
		    else if (alternate_form)
		    {
			if (arg_sign != 0
				     && (fmt_spec == 'x' || fmt_spec == 'X') )
			{
			    tmp[str_arg_l++] = '0';
			    tmp[str_arg_l++] = fmt_spec;
			}
			/* alternate form should have no effect for p
			 * conversion, but ... */
		    }

		    zero_padding_insertion_ind = str_arg_l;
		    if (!precision_specified)
			precision = 1;   /* default precision is 1 */
		    if (precision == 0 && arg_sign == 0)
		    {
			/* When zero value is formatted with an explicit
			 * precision 0, the resulting formatted string is
			 * empty (d, i, u, o, x, X, p).   */
		    }
		    else
		    {
			char	f[5];
			int	f_l = 0;

			/* construct a simple format string for sprintf */
			f[f_l++] = '%';
			if (!length_modifier)
			    ;
			else if (length_modifier == '2')
			{
			    f[f_l++] = 'l';
			    f[f_l++] = 'l';
			}
			else
			    f[f_l++] = length_modifier;
			f[f_l++] = fmt_spec;
			f[f_l++] = '\0';

			if (fmt_spec == 'p')
			    str_arg_l += sprintf(tmp + str_arg_l, f, ptr_arg);
			else if (fmt_spec == 'd')
			{
			    /* signed */
			    switch (length_modifier)
			    {
			    case '\0':
			    case 'h': str_arg_l += sprintf(
						 tmp + str_arg_l, f, int_arg);
				      break;
			    case 'l': str_arg_l += sprintf(
						tmp + str_arg_l, f, long_arg);
				      break;
			    }
			}
			else
			{
			    /* unsigned */
			    switch (length_modifier)
			    {
			    case '\0':
			    case 'h': str_arg_l += sprintf(
						tmp + str_arg_l, f, uint_arg);
				      break;
			    case 'l': str_arg_l += sprintf(
					       tmp + str_arg_l, f, ulong_arg);
				      break;
			    }
			}

			/* include the optional minus sign and possible
			 * "0x" in the region before the zero padding
			 * insertion point */
			if (zero_padding_insertion_ind < str_arg_l
				&& tmp[zero_padding_insertion_ind] == '-')
			    zero_padding_insertion_ind++;
			if (zero_padding_insertion_ind + 1 < str_arg_l
				&& tmp[zero_padding_insertion_ind]   == '0'
				&& (tmp[zero_padding_insertion_ind + 1] == 'x'
				 || tmp[zero_padding_insertion_ind + 1] == 'X'))
			    zero_padding_insertion_ind += 2;
		    }

		    {
			size_t num_of_digits = str_arg_l
						 - zero_padding_insertion_ind;

			if (alternate_form && fmt_spec == 'o'
				/* unless zero is already the first
				 * character */
				&& !(zero_padding_insertion_ind < str_arg_l
				    && tmp[zero_padding_insertion_ind] == '0'))
			{
			    /* assure leading zero for alternate-form
			     * octal numbers */
			    if (!precision_specified
					     || precision < num_of_digits + 1)
			    {
				/* precision is increased to force the
				 * first character to be zero, except if a
				 * zero value is formatted with an
				 * explicit precision of zero */
				precision = num_of_digits + 1;
				precision_specified = 1;
			    }
			}
			/* zero padding to specified precision? */
			if (num_of_digits < precision)
			    number_of_zeros_to_pad = precision - num_of_digits;
		    }
		    /* zero padding to specified minimal field width? */
		    if (!justify_left && zero_padding)
		    {
			int n = (int)(min_field_width - (str_arg_l
						    + number_of_zeros_to_pad));
			if (n > 0)
			    number_of_zeros_to_pad += n;
		    }
		    break;
		}

	    default:
		/* unrecognized conversion specifier, keep format string
		 * as-is */
		zero_padding = 0;  /* turn zero padding off for non-numeric
				      convers. */
		justify_left = 1;
		min_field_width = 0;		    /* reset flags */

		/* discard the unrecognized conversion, just keep *
		 * the unrecognized conversion character	  */
		str_arg = p;
		str_arg_l = 0;
		if (*p != NUL)
		    str_arg_l++;  /* include invalid conversion specifier
				     unchanged if not at end-of-string */
		break;
	    }

	    if (*p != NUL)
		p++;     /* step over the just processed conversion specifier */

	    /* insert padding to the left as requested by min_field_width;
	     * this does not include the zero padding in case of numerical
	     * conversions*/
	    if (!justify_left)
	    {
		/* left padding with blank or zero */
		int pn = (int)(min_field_width - (str_arg_l + number_of_zeros_to_pad));

		if (pn > 0)
		{
		    if (str_l < str_m)
		    {
			size_t avail = str_m - str_l;

			vim_memset(str + str_l, zero_padding ? '0' : ' ',
					     (size_t)pn > avail ? avail : pn);
		    }
		    str_l += pn;
		}
	    }

	    /* zero padding as requested by the precision or by the minimal
	     * field width for numeric conversions required? */
	    if (number_of_zeros_to_pad == 0)
	    {
		/* will not copy first part of numeric right now, *
		 * force it to be copied later in its entirety    */
		zero_padding_insertion_ind = 0;
	    }
	    else
	    {
		/* insert first part of numerics (sign or '0x') before zero
		 * padding */
		int zn = (int)zero_padding_insertion_ind;

		if (zn > 0)
		{
		    if (str_l < str_m)
		    {
			size_t avail = str_m - str_l;

			mch_memmove(str + str_l, str_arg,
					     (size_t)zn > avail ? avail : zn);
		    }
		    str_l += zn;
		}

		/* insert zero padding as requested by the precision or min
		 * field width */
		zn = (int)number_of_zeros_to_pad;
		if (zn > 0)
		{
		    if (str_l < str_m)
		    {
			size_t avail = str_m-str_l;

			vim_memset(str + str_l, '0',
					     (size_t)zn > avail ? avail : zn);
		    }
		    str_l += zn;
		}
	    }

	    /* insert formatted string
	     * (or as-is conversion specifier for unknown conversions) */
	    {
		int sn = (int)(str_arg_l - zero_padding_insertion_ind);

		if (sn > 0)
		{
		    if (str_l < str_m)
		    {
			size_t avail = str_m - str_l;

			mch_memmove(str + str_l,
				str_arg + zero_padding_insertion_ind,
				(size_t)sn > avail ? avail : sn);
		    }
		    str_l += sn;
		}
	    }

	    /* insert right padding */
	    if (justify_left)
	    {
		/* right blank padding to the field width */
		int pn = (int)(min_field_width - (str_arg_l + number_of_zeros_to_pad));

		if (pn > 0)
		{
		    if (str_l < str_m)
		    {
			size_t avail = str_m - str_l;

			vim_memset(str + str_l, ' ',
					     (size_t)pn > avail ? avail : pn);
		    }
		    str_l += pn;
		}
	    }
	}
    }

    if (str_m > 0)
    {
	/* make sure the string is nul-terminated even at the expense of
	 * overwriting the last character (shouldn't happen, but just in case)
	 * */
	str[str_l <= str_m - 1 ? str_l : str_m - 1] = '\0';
    }

#ifdef HAVE_STDARG_H
    if (tvs != NULL && tvs[arg_idx - 1].v_type != VAR_UNKNOWN)
	EMSG(_("E767: Too many arguments to printf()"));
#endif

    /* Return the number of characters formatted (excluding trailing nul
     * character), that is, the number of characters that would have been
     * written to the buffer if it were large enough. */
    return (int)str_l;
}

#endif /* PROTO */
