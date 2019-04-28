/* vi:set ts=8 sts=4 sw=4 noet:
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
#define USING_FLOAT_STUFF

#include "vim.h"

static void add_msg_hist(char_u *s, int len, int attr);
static void hit_return_msg(void);
static void msg_home_replace_attr(char_u *fname, int attr);
static void msg_puts_attr_len(char *str, int maxlen, int attr);
static void msg_puts_display(char_u *str, int maxlen, int attr, int recurse);
static void msg_scroll_up(void);
static void inc_msg_scrolled(void);
static void store_sb_text(char_u **sb_str, char_u *s, int attr, int *sb_col, int finish);
static void t_puts(int *t_col, char_u *t_s, char_u *s, int attr);
static void msg_puts_printf(char_u *str, int maxlen);
static int do_more_prompt(int typed_char);
static void msg_screen_putchar(int c, int attr);
static int  msg_check_screen(void);
static void redir_write(char_u *s, int maxlen);
#ifdef FEAT_CON_DIALOG
static char_u *msg_show_console_dialog(char_u *message, char_u *buttons, int dfltbutton);
static int	confirm_msg_used = FALSE;	/* displaying confirm_msg */
static char_u	*confirm_msg = NULL;		/* ":confirm" message */
static char_u	*confirm_msg_tail;		/* tail of confirm_msg */
#endif
#ifdef FEAT_JOB_CHANNEL
static int emsg_to_channel_log = FALSE;
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

static FILE *verbose_fd = NULL;
static int  verbose_did_open = FALSE;

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
 *		    more-prompt is to be given.  -1 when not set.
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
msg(char *s)
{
    return msg_attr_keep(s, 0, FALSE);
}

/*
 * Like msg() but keep it silent when 'verbosefile' is set.
 */
    int
verb_msg(char *s)
{
    int		n;

    verbose_enter();
    n = msg_attr_keep(s, 0, FALSE);
    verbose_leave();

    return n;
}

    int
msg_attr(char *s, int attr)
{
    return msg_attr_keep(s, attr, FALSE);
}

    int
msg_attr_keep(
    char	*s,
    int		attr,
    int		keep)	    /* TRUE: set keep_msg if it doesn't scroll */
{
    static int	entered = 0;
    int		retval;
    char_u	*buf = NULL;

    /* Skip messages not matching ":filter pattern".
     * Don't filter when there is an error. */
    if (!emsg_on_display && message_filtered((char_u *)s))
	return TRUE;

#ifdef FEAT_EVAL
    if (attr == 0)
	set_vim_var_string(VV_STATUSMSG, (char_u *)s, -1);
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
    if ((char_u *)s != keep_msg
	    || (*s != '<'
		&& last_msg_hist != NULL
		&& last_msg_hist->msg != NULL
		&& STRCMP(s, last_msg_hist->msg)))
	add_msg_hist((char_u *)s, -1, attr);

#ifdef FEAT_JOB_CHANNEL
    if (emsg_to_channel_log)
	/* Write message in the channel log. */
	ch_log(NULL, "ERROR: %s", (char *)s);
#endif

    /* When displaying keep_msg, don't let msg_start() free it, caller must do
     * that. */
    if ((char_u *)s == keep_msg)
	keep_msg = NULL;

    /* Truncate the message if needed. */
    msg_start();
    buf = msg_strtrunc((char_u *)s, FALSE);
    if (buf != NULL)
	s = (char *)buf;

    msg_outtrans_attr((char_u *)s, attr);
    msg_clr_eos();
    retval = msg_end();

    if (keep && retval && vim_strsize((char_u *)s)
			    < (int)(Rows - cmdline_row - 1) * Columns + sc_col)
	set_keep_msg((char_u *)s, 0);

    vim_free(buf);
    --entered;
    return retval;
}

/*
 * Truncate a string such that it can be printed without causing a scroll.
 * Returns an allocated string or NULL when no truncating is done.
 */
    char_u *
msg_strtrunc(
    char_u	*s,
    int		force)	    /* always truncate */
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
	    if (enc_utf8)
		/* may have up to 18 bytes per cell (6 per char, up to two
		 * composing chars) */
		len = (room + 2) * 18;
	    else if (enc_dbcs == DBCS_JPNU)
		/* may have up to 2 bytes per cell for euc-jp */
		len = (room + 2) * 2;
	    else
		len = room + 2;
	    buf = alloc(len);
	    if (buf != NULL)
		trunc_string(s, buf, room, len);
	}
    }
    return buf;
}

/*
 * Truncate a string "s" to "buf" with cell width "room".
 * "s" and "buf" may be equal.
 */
    void
trunc_string(
    char_u	*s,
    char_u	*buf,
    int		room_in,
    int		buflen)
{
    size_t	room = room_in - 3; /* "..." takes 3 chars */
    size_t	half;
    size_t	len = 0;
    int		e;
    int		i;
    int		n;

    if (room_in < 3)
	room = 0;
    half = room / 2;

    /* First part: Start of the string. */
    for (e = 0; len < half && e < buflen; ++e)
    {
	if (s[e] == NUL)
	{
	    /* text fits without truncating! */
	    buf[e] = NUL;
	    return;
	}
	n = ptr2cells(s + e);
	if (len + n > half)
	    break;
	len += n;
	buf[e] = s[e];
	if (has_mbyte)
	    for (n = (*mb_ptr2len)(s + e); --n > 0; )
	    {
		if (++e == buflen)
		    break;
		buf[e] = s[e];
	    }
    }

    /* Last part: End of the string. */
    i = e;
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
		half = half - utf_head_off(s, s + half - 1) - 1;
	    while (half > 0 && utf_iscomposing(utf_ptr2char(s + half)));
	    n = ptr2cells(s + half);
	    if (len + n > room || half == 0)
		break;
	    len += n;
	    i = (int)half;
	}
    }
    else
    {
	for (i = (int)STRLEN(s); len + (n = ptr2cells(s + i - 1)) <= room; --i)
	    len += n;
    }


    if (i <= e + 3)
    {
	/* text fits without truncating */
	if (s != buf)
	{
	    len = STRLEN(s);
	    if (len >= (size_t)buflen)
		len = buflen - 1;
	    len = len - e + 1;
	    if (len < 1)
		buf[e - 1] = NUL;
	    else
		mch_memmove(buf + e, s + e, len);
	}
    }
    else if (e + 3 < buflen)
    {
	/* set the middle and copy the last part */
	mch_memmove(buf + e, "...", (size_t)3);
	len = STRLEN(s + i) + 1;
	if (len >= (size_t)buflen - e - 3)
	    len = buflen - e - 3 - 1;
	mch_memmove(buf + e + 3, s + i, len);
	buf[e + 3 + len - 1] = NUL;
    }
    else
    {
	/* can't fit in the "...", just truncate it */
	buf[e - 1] = NUL;
    }
}

/*
 * Automatic prototype generation does not understand this function.
 * Note: Caller of smsg() and smsg_attr() must check the resulting string is
 * shorter than IOSIZE!!!
 */
#ifndef PROTO

int vim_snprintf(char *str, size_t str_m, const char *fmt, ...);

    int
# ifdef __BORLANDC__
_RTLENTRYF
# endif
smsg(const char *s, ...)
{
    va_list arglist;

    va_start(arglist, s);
    vim_vsnprintf((char *)IObuff, IOSIZE, s, arglist);
    va_end(arglist);
    return msg((char *)IObuff);
}

    int
# ifdef __BORLANDC__
_RTLENTRYF
# endif
smsg_attr(int attr, const char *s, ...)
{
    va_list arglist;

    va_start(arglist, s);
    vim_vsnprintf((char *)IObuff, IOSIZE, s, arglist);
    va_end(arglist);
    return msg_attr((char *)IObuff, attr);
}

    int
# ifdef __BORLANDC__
_RTLENTRYF
# endif
smsg_attr_keep(int attr, const char *s, ...)
{
    va_list arglist;

    va_start(arglist, s);
    vim_vsnprintf((char *)IObuff, IOSIZE, s, arglist);
    va_end(arglist);
    return msg_attr_keep((char *)IObuff, attr, TRUE);
}

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
reset_last_sourcing(void)
{
    VIM_CLEAR(last_sourcing_name);
    last_sourcing_lnum = 0;
}

/*
 * Return TRUE if "sourcing_name" differs from "last_sourcing_name".
 */
    static int
other_sourcing_name(void)
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
get_emsg_source(void)
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
get_emsg_lnum(void)
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
msg_source(int attr)
{
    char_u	*p;

    ++no_wait_return;
    p = get_emsg_source();
    if (p != NULL)
    {
	msg_attr((char *)p, attr);
	vim_free(p);
    }
    p = get_emsg_lnum();
    if (p != NULL)
    {
	msg_attr((char *)p, HL_ATTR(HLF_N));
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
emsg_not_now(void)
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

#if defined(FEAT_EVAL) || defined(PROTO)
static garray_T ignore_error_list = GA_EMPTY;

    void
ignore_error_for_testing(char_u *error)
{
    if (ignore_error_list.ga_itemsize == 0)
	ga_init2(&ignore_error_list, sizeof(char_u *), 1);

    if (STRCMP("RESET", error) == 0)
	ga_clear_strings(&ignore_error_list);
    else
	ga_add_string(&ignore_error_list, error);
}

    static int
ignore_error(char_u *msg)
{
    int i;

    for (i = 0; i < ignore_error_list.ga_len; ++i)
	if (strstr((char *)msg,
		  (char *)((char_u **)(ignore_error_list.ga_data))[i]) != NULL)
	    return TRUE;
    return FALSE;
}
#endif

#if !defined(HAVE_STRERROR) || defined(PROTO)
/*
 * Replacement for perror() that behaves more or less like emsg() was called.
 * v:errmsg will be set and called_emsg will be set.
 */
    void
do_perror(char *msg)
{
    perror(msg);
    ++emsg_silent;
    emsg(msg);
    --emsg_silent;
}
#endif

/*
 * emsg_core() - display an error message
 *
 * Rings the bell, if appropriate, and calls message() to do the real work
 * When terminal not initialized (yet) mch_errmsg(..) is used.
 *
 * Return TRUE if wait_return not called.
 * Note: caller must check 'emsg_not_now()' before calling this.
 */
    static int
emsg_core(char_u *s)
{
    int		attr;
    char_u	*p;
    int		r;
#ifdef FEAT_EVAL
    int		ignore = FALSE;
    int		severe;
#endif

#ifdef FEAT_EVAL
    /* When testing some errors are turned into a normal message. */
    if (ignore_error(s))
	/* don't call msg() if it results in a dialog */
	return msg_use_printf() ? FALSE : msg((char *)s);
#endif

    called_emsg = TRUE;

#ifdef FEAT_EVAL
    /* If "emsg_severe" is TRUE: When an error exception is to be thrown,
     * prefer this message over previous messages for the same command. */
    severe = emsg_severe;
    emsg_severe = FALSE;
#endif

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
		++did_emsg;
	    return TRUE;
	}

	/* set "v:errmsg", also when using ":silent! cmd" */
	set_vim_var_string(VV_ERRMSG, s, -1);
#endif

	/*
	 * When using ":silent! cmd" ignore error messages.
	 * But do write it to the redirection file.
	 */
	if (emsg_silent != 0)
	{
	    if (emsg_noredir == 0)
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
	    }
#ifdef FEAT_JOB_CHANNEL
	    ch_log(NULL, "ERROR: %s", (char *)s);
#endif
	    return TRUE;
	}

	ex_exitval = 1;

	/* Reset msg_silent, an error causes messages to be switched back on.
	 */
	msg_silent = 0;
	cmd_silent = FALSE;

	if (global_busy)		/* break :global command */
	    ++global_busy;

	if (p_eb)
	    beep_flush();		/* also includes flush_buffers() */
	else
	    flush_buffers(FLUSH_MINIMAL);  // flush internal buffers
	++did_emsg;			   // flag for DoOneCmd()
#ifdef FEAT_EVAL
	did_uncaught_emsg = TRUE;
#endif
    }

    emsg_on_display = TRUE;	/* remember there is an error message */
    ++msg_scroll;		/* don't overwrite a previous message */
    attr = HL_ATTR(HLF_E);	/* set highlight mode for error messages */
    if (msg_scrolled != 0)
	need_wait_return = TRUE;    /* needed in case emsg() is called after
				     * wait_return has reset need_wait_return
				     * and a redraw is expected because
				     * msg_scrolled is non-zero */

#ifdef FEAT_JOB_CHANNEL
    emsg_to_channel_log = TRUE;
#endif
    /*
     * Display name and line number for the source of the error.
     */
    msg_source(attr);

    /*
     * Display the error message itself.
     */
    msg_nowait = FALSE;			/* wait for this msg */
    r = msg_attr((char *)s, attr);

#ifdef FEAT_JOB_CHANNEL
    emsg_to_channel_log = FALSE;
#endif
    return r;
}

/*
 * Print an error message.
 */
    int
emsg(char *s)
{
    /* Skip this if not giving error messages at the moment. */
    if (!emsg_not_now())
	return emsg_core((char_u *)s);
    return TRUE;		/* no error messages at the moment */
}

#ifndef PROTO  // manual proto with __attribute__
/*
 * Print an error message with format string and variable arguments.
 * Note: caller must not pass 'IObuff' as 1st argument.
 */
    int
semsg(const char *s, ...)
{
    /* Skip this if not giving error messages at the moment. */
    if (!emsg_not_now())
    {
	va_list ap;

	va_start(ap, s);
	vim_vsnprintf((char *)IObuff, IOSIZE, s, ap);
	va_end(ap);
	return emsg_core(IObuff);
    }
    return TRUE;		/* no error messages at the moment */
}
#endif

/*
 * Same as emsg(...), but abort on error when ABORT_ON_INTERNAL_ERROR is
 * defined. It is used for internal errors only, so that they can be
 * detected when fuzzing vim.
 */
    void
iemsg(char *s)
{
    if (!emsg_not_now())
	emsg_core((char_u *)s);
#ifdef ABORT_ON_INTERNAL_ERROR
    abort();
#endif
}

#ifndef PROTO  // manual proto with __attribute__
/*
 * Same as semsg(...) but abort on error when ABORT_ON_INTERNAL_ERROR is
 * defined. It is used for internal errors only, so that they can be
 * detected when fuzzing vim.
 * Note: caller must not pass 'IObuff' as 1st argument.
 */
    void
siemsg(const char *s, ...)
{
    if (!emsg_not_now())
    {
	va_list ap;

	va_start(ap, s);
	vim_vsnprintf((char *)IObuff, IOSIZE, s, ap);
	va_end(ap);
	emsg_core(IObuff);
    }
# ifdef ABORT_ON_INTERNAL_ERROR
    abort();
# endif
}
#endif

/*
 * Give an "Internal error" message.
 */
    void
internal_error(char *where)
{
    siemsg(_(e_intern2), where);
}

/* emsg3() and emsgn() are in misc2.c to avoid warnings for the prototypes. */

    void
emsg_invreg(int name)
{
    semsg(_("E354: Invalid register name: '%s'"), transchar(name));
}

/*
 * Like msg(), but truncate to a single line if p_shm contains 't', or when
 * "force" is TRUE.  This truncates in another way as for normal messages.
 * Careful: The string may be changed by msg_may_trunc()!
 * Returns a pointer to the printed message, if wait_return() not called.
 */
    char *
msg_trunc_attr(char *s, int force, int attr)
{
    int		n;
    char	*ts;

    /* Add message to history before truncating */
    add_msg_hist((char_u *)s, -1, attr);

    ts = (char *)msg_may_trunc(force, (char_u *)s);

    msg_hist_off = TRUE;
    n = msg_attr(ts, attr);
    msg_hist_off = FALSE;

    if (n)
	return ts;
    return NULL;
}

/*
 * Check if message "s" should be truncated at the start (for filenames).
 * Return a pointer to where the truncated message starts.
 * Note: May change the message by replacing a character with '<'.
 */
    char_u *
msg_may_trunc(int force, char_u *s)
{
    int		n;
    int		room;

    room = (int)(Rows - cmdline_row - 1) * Columns + sc_col - 1;
    if ((force || (shortmess(SHM_TRUNC) && !exmode_active))
	    && (n = (int)STRLEN(s) - room) > 0)
    {
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
	s += n;
	*s = '<';
    }
    return s;
}

    static void
add_msg_hist(
    char_u	*s,
    int		len,		/* -1 for undetermined length */
    int		attr)
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
delete_first_msg(void)
{
    struct msg_hist *p;

    if (msg_hist_len <= 0)
	return FAIL;
    p = first_msg_hist;
    first_msg_hist = p->next;
    if (first_msg_hist == NULL)
	last_msg_hist = NULL;  /* history is empty */
    vim_free(p->msg);
    vim_free(p);
    --msg_hist_len;
    return OK;
}

/*
 * ":messages" command.
 */
    void
ex_messages(exarg_T *eap)
{
    struct msg_hist *p;
    char_u	    *s;
    int		    c = 0;

    if (STRCMP(eap->arg, "clear") == 0)
    {
	int keep = eap->addr_count == 0 ? 0 : eap->line2;

	while (msg_hist_len > keep)
	    (void)delete_first_msg();
	return;
    }

    if (*eap->arg != NUL)
    {
	emsg(_(e_invarg));
	return;
    }

    msg_hist_off = TRUE;

    p = first_msg_hist;
    if (eap->addr_count != 0)
    {
	/* Count total messages */
	for (; p != NULL && !got_int; p = p->next)
	    c++;

	c -= eap->line2;

	/* Skip without number of messages specified */
	for (p = first_msg_hist; p != NULL && !got_int && c > 0;
						    p = p->next, c--);
    }

    if (p == first_msg_hist)
    {
	s = mch_getenv((char_u *)"LANG");
	if (s != NULL && *s != NUL)
	    // The next comment is extracted by xgettext and put in po file for
	    // translators to read.
	    msg_attr(
		    // Translator: Please replace the name and email address
		    // with the appropriate text for your translation.
		    _("Messages maintainer: Bram Moolenaar <Bram@vim.org>"),
		    HL_ATTR(HLF_T));
    }

    /* Display what was not skipped. */
    for (; p != NULL && !got_int; p = p->next)
	if (p->msg != NULL)
	    msg_attr((char *)p->msg, p->attr);

    msg_hist_off = FALSE;
}

#if defined(FEAT_CON_DIALOG) || defined(FIND_REPLACE_DIALOG) || defined(PROTO)
/*
 * Call this after prompting the user.  This will avoid a hit-return message
 * and a delay.
 */
    void
msg_end_prompt(void)
{
    need_wait_return = FALSE;
    emsg_on_display = FALSE;
    cmdline_row = msg_row;
    msg_col = 0;
    msg_clr_eos();
    lines_left = -1;
}
#endif

/*
 * Wait for the user to hit a key (normally Enter).
 * If "redraw" is TRUE, clear and redraw the screen.
 * If "redraw" is FALSE, just redraw the screen.
 * If "redraw" is -1, don't redraw at all.
 */
    void
wait_return(int redraw)
{
    int		c;
    int		oldState;
    int		tmpState;
    int		had_got_int;
    int		save_reg_recording;
    FILE	*save_scriptout;

    if (redraw == TRUE)
	must_redraw = CLEAR;

    /* If using ":silent cmd", don't wait for a return.  Also don't set
     * need_wait_return to do it later. */
    if (msg_silent != 0)
	return;

    /*
     * When inside vgetc(), we can't wait for a typed character at all.
     * With the global command (and some others) we only need one return at
     * the end. Adjust cmdline_row to avoid the next message overwriting the
     * last one.
     */
    if (vgetc_busy > 0)
	return;
    need_wait_return = TRUE;
    if (no_wait_return)
    {
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
	msg_puts(" ");		/* make sure the cursor is on the right line */
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
	cmdline_row = msg_row;

	/* Avoid the sequence that the user types ":" at the hit-return prompt
	 * to start an Ex command, but the file-changed dialog gets in the
	 * way. */
	if (need_check_timestamps)
	    check_timestamps(FALSE);

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

	    /* Temporarily disable Recording. If Recording is active, the
	     * character will be recorded later, since it will be added to the
	     * typebuf after the loop */
	    save_reg_recording = reg_recording;
	    save_scriptout = scriptout;
	    reg_recording = 0;
	    scriptout = NULL;
	    c = safe_vgetc();
	    if (had_got_int && !global_busy)
		got_int = FALSE;
	    --no_mapping;
	    --allow_keys;
	    reg_recording = save_reg_recording;
	    scriptout = save_scriptout;

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
		if (c == 'b' || c == 'k' || c == 'u' || c == 'g'
						|| c == K_UP || c == K_PAGEUP)
		{
		    if (msg_scrolled > Rows)
			/* scroll back to show older messages */
			do_more_prompt(c);
		    else
		    {
			msg_didout = FALSE;
			c = K_IGNORE;
			msg_col =
#ifdef FEAT_RIGHTLEFT
			    cmdmsg_rl ? Columns - 1 :
#endif
			    0;
		    }
		    if (quit_more)
		    {
			c = CAR;		/* just pretend CR was hit */
			quit_more = FALSE;
			got_int = FALSE;
		    }
		    else if (c != K_IGNORE)
		    {
			c = K_IGNORE;
			hit_return_msg();
		    }
		}
		else if (msg_scrolled > Rows - 2
			 && (c == 'j' || c == 'd' || c == 'f'
					   || c == K_DOWN || c == K_PAGEDOWN))
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
				|| c == K_MOUSELEFT  || c == K_MOUSERIGHT
				|| c == K_MOUSEDOWN  || c == K_MOUSEUP
				|| c == K_MOUSEMOVE
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
#ifdef FEAT_TERMINAL
	skip_term_loop = TRUE;
#endif
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
	VIM_CLEAR(keep_msg);	    /* don't redisplay message, it's too long */

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
hit_return_msg(void)
{
    int		save_p_more = p_more;

    p_more = FALSE;	/* don't want see this message when scrolling back */
    if (msg_didout)	/* start on a new line */
	msg_putchar('\n');
    if (got_int)
	msg_puts(_("Interrupt: "));

    msg_puts_attr(_("Press ENTER or type command to continue"), HL_ATTR(HLF_R));
    if (!msg_use_printf())
	msg_clr_eos();
    p_more = save_p_more;
}

/*
 * Set "keep_msg" to "s".  Free the old value and check for NULL pointer.
 */
    void
set_keep_msg(char_u *s, int attr)
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
set_keep_msg_from_hist(void)
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
msg_start(void)
{
    int		did_return = FALSE;

    if (!msg_silent)
	VIM_CLEAR(keep_msg);

#ifdef FEAT_EVAL
    if (need_clr_eos)
    {
	/* Halfway an ":echo" command and getting an (error) message: clear
	 * any text from the command. */
	need_clr_eos = FALSE;
	msg_clr_eos();
    }
#endif

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
msg_starthere(void)
{
    lines_left = cmdline_row;
    msg_didany = FALSE;
}

    void
msg_putchar(int c)
{
    msg_putchar_attr(c, 0);
}

    void
msg_putchar_attr(int c, int attr)
{
    char_u	buf[MB_MAXBYTES + 1];

    if (IS_SPECIAL(c))
    {
	buf[0] = K_SPECIAL;
	buf[1] = K_SECOND(c);
	buf[2] = K_THIRD(c);
	buf[3] = NUL;
    }
    else
	buf[(*mb_char2bytes)(c, buf)] = NUL;
    msg_puts_attr((char *)buf, attr);
}

    void
msg_outnum(long n)
{
    char	buf[20];

    sprintf(buf, "%ld", n);
    msg_puts(buf);
}

    void
msg_home_replace(char_u *fname)
{
    msg_home_replace_attr(fname, 0);
}

#if defined(FEAT_FIND_ID) || defined(PROTO)
    void
msg_home_replace_hl(char_u *fname)
{
    msg_home_replace_attr(fname, HL_ATTR(HLF_D));
}
#endif

    static void
msg_home_replace_attr(char_u *fname, int attr)
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
msg_outtrans(char_u *str)
{
    return msg_outtrans_attr(str, 0);
}

    int
msg_outtrans_attr(char_u *str, int attr)
{
    return msg_outtrans_len_attr(str, (int)STRLEN(str), attr);
}

    int
msg_outtrans_len(char_u *str, int len)
{
    return msg_outtrans_len_attr(str, len, 0);
}

/*
 * Output one character at "p".  Return pointer to the next character.
 * Handles multi-byte characters.
 */
    char_u *
msg_outtrans_one(char_u *p, int attr)
{
    int		l;

    if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
    {
	msg_outtrans_len_attr(p, l, attr);
	return p + l;
    }
    msg_puts_attr((char *)transchar_byte(*p), attr);
    return p + 1;
}

    int
msg_outtrans_len_attr(char_u *msgstr, int len, int attr)
{
    int		retval = 0;
    char_u	*str = msgstr;
    char_u	*plain_start = msgstr;
    char_u	*s;
    int		mb_l;
    int		c;

    /* if MSG_HIST flag set, add message to history */
    if (attr & MSG_HIST)
    {
	add_msg_hist(str, len, attr);
	attr &= ~MSG_HIST;
    }

    /* If the string starts with a composing character first draw a space on
     * which the composing char can be drawn. */
    if (enc_utf8 && utf_iscomposing(utf_ptr2char(msgstr)))
	msg_puts_attr(" ", attr);

    /*
     * Go over the string.  Special characters are translated and printed.
     * Normal characters are printed several at a time.
     */
    while (--len >= 0)
    {
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
		    msg_puts_attr_len((char *)plain_start,
					       (int)(str - plain_start), attr);
		plain_start = str + mb_l;
		msg_puts_attr((char *)transchar(c),
					    attr == 0 ? HL_ATTR(HLF_8) : attr);
		retval += char2cells(c);
	    }
	    len -= mb_l - 1;
	    str += mb_l;
	}
	else
	{
	    s = transchar_byte(*str);
	    if (s[1] != NUL)
	    {
		/* unprintable char: print the printable chars so far and the
		 * translation of the unprintable char. */
		if (str > plain_start)
		    msg_puts_attr_len((char *)plain_start,
					       (int)(str - plain_start), attr);
		plain_start = str + 1;
		msg_puts_attr((char *)s, attr == 0 ? HL_ATTR(HLF_8) : attr);
		retval += (int)STRLEN(s);
	    }
	    else
		++retval;
	    ++str;
	}
    }

    if (str > plain_start)
	/* print the printable chars at the end */
	msg_puts_attr_len((char *)plain_start, (int)(str - plain_start), attr);

    return retval;
}

#if defined(FEAT_QUICKFIX) || defined(PROTO)
    void
msg_make(char_u *arg)
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
msg_outtrans_special(
    char_u	*strstart,
    int		from,	// TRUE for lhs of a mapping
    int		maxlen) // screen columns, 0 for unlimeted
{
    char_u	*str = strstart;
    int		retval = 0;
    char	*text;
    int		attr;
    int		len;

    attr = HL_ATTR(HLF_8);
    while (*str != NUL)
    {
	/* Leading and trailing spaces need to be displayed in <> form. */
	if ((str == strstart || str[1] == NUL) && *str == ' ')
	{
	    text = "<Space>";
	    ++str;
	}
	else
	    text = (char *)str2special(&str, from);
	len = vim_strsize((char_u *)text);
	if (maxlen > 0 && retval + len >= maxlen)
	    break;
	/* Highlight special keys */
	msg_puts_attr(text, len > 1
		&& (*mb_ptr2len)((char_u *)text) <= 1 ? attr : 0);
	retval += len;
    }
    return retval;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return the lhs or rhs of a mapping, with the key codes turned into printable
 * strings, in an allocated string.
 */
    char_u *
str2special_save(
    char_u  *str,
    int	    is_lhs)  /* TRUE for lhs, FALSE for rhs */
{
    garray_T	ga;
    char_u	*p = str;

    ga_init2(&ga, 1, 40);
    while (*p != NUL)
	ga_concat(&ga, str2special(&p, is_lhs));
    ga_append(&ga, NUL);
    return (char_u *)ga.ga_data;
}
#endif

/*
 * Return the printable string for the key codes at "*sp".
 * Used for translating the lhs or rhs of a mapping to printable chars.
 * Advances "sp" to the next code.
 */
    char_u *
str2special(
    char_u	**sp,
    int		from)	/* TRUE for lhs of mapping */
{
    int			c;
    static char_u	buf[7];
    char_u		*str = *sp;
    int			modifiers = 0;
    int			special = FALSE;

    if (has_mbyte)
    {
	char_u	*p;

	/* Try to un-escape a multi-byte character.  Return the un-escaped
	 * string if it is a multi-byte character. */
	p = mb_unescape(sp);
	if (p != NULL)
	    return p;
    }

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
	}
	if (IS_SPECIAL(c) || modifiers)	/* special key */
	    special = TRUE;
    }

    if (has_mbyte && !IS_SPECIAL(c))
    {
	int len = (*mb_ptr2len)(str);

	/* For multi-byte characters check for an illegal byte. */
	if (has_mbyte && MB_BYTE2LEN(*str) > len)
	{
	    transchar_nonprint(buf, c);
	    *sp = str + 1;
	    return buf;
	}
	/* Since 'special' is TRUE the multi-byte character 'c' will be
	 * processed by get_special_key_name() */
	c = (*mb_ptr2char)(str);
	*sp = str + len;
    }
    else
	*sp = str + 1;

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
str2specialbuf(char_u *sp, char_u *buf, int len)
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
msg_prt_line(char_u *s, int list)
{
    int		c;
    int		col = 0;
    int		n_extra = 0;
    int		c_extra = 0;
    int		c_final = 0;
    char_u	*p_extra = NULL;	    /* init to make SASC shut up */
    int		n;
    int		attr = 0;
    char_u	*trail = NULL;
    int		l;
    char_u	buf[MB_MAXBYTES + 1];

    if (curwin->w_p_list)
	list = TRUE;

    /* find start of trailing whitespace */
    if (list && lcs_trail)
    {
	trail = s + STRLEN(s);
	while (trail > s && VIM_ISWHITE(trail[-1]))
	    --trail;
    }

    /* output a space for an empty line, otherwise the line will be
     * overwritten */
    if (*s == NUL && !(list && lcs_eol != NUL))
	msg_putchar(' ');

    while (!got_int)
    {
	if (n_extra > 0)
	{
	    --n_extra;
	    if (n_extra == 0 && c_final)
		c = c_final;
	    else if (c_extra)
		c = c_extra;
	    else
		c = *p_extra++;
	}
	else if (has_mbyte && (l = (*mb_ptr2len)(s)) > 1)
	{
	    col += (*mb_ptr2cells)(s);
	    if (lcs_nbsp != NUL && list
		    && (mb_ptr2char(s) == 160
			|| mb_ptr2char(s) == 0x202f))
	    {
		mb_char2bytes(lcs_nbsp, buf);
		buf[(*mb_ptr2len)(buf)] = NUL;
	    }
	    else
	    {
		mch_memmove(buf, s, (size_t)l);
		buf[l] = NUL;
	    }
	    msg_puts((char *)buf);
	    s += l;
	    continue;
	}
	else
	{
	    attr = 0;
	    c = *s++;
	    if (c == TAB && (!list || lcs_tab1))
	    {
		/* tab amount depends on current column */
#ifdef FEAT_VARTABS
		n_extra = tabstop_padding(col, curbuf->b_p_ts,
						    curbuf->b_p_vts_array) - 1;
#else
		n_extra = curbuf->b_p_ts - col % curbuf->b_p_ts - 1;
#endif
		if (!list)
		{
		    c = ' ';
		    c_extra = ' ';
		    c_final = NUL;
		}
		else
		{
		    c = (n_extra == 0 && lcs_tab3) ? lcs_tab3 : lcs_tab1;
		    c_extra = lcs_tab2;
		    c_final = lcs_tab3;
		    attr = HL_ATTR(HLF_8);
		}
	    }
	    else if (c == 160 && list && lcs_nbsp != NUL)
	    {
		c = lcs_nbsp;
		attr = HL_ATTR(HLF_8);
	    }
	    else if (c == NUL && list && lcs_eol != NUL)
	    {
		p_extra = (char_u *)"";
		c_extra = NUL;
		c_final = NUL;
		n_extra = 1;
		c = lcs_eol;
		attr = HL_ATTR(HLF_AT);
		--s;
	    }
	    else if (c != NUL && (n = byte2cells(c)) > 1)
	    {
		n_extra = n - 1;
		p_extra = transchar_byte(c);
		c_extra = NUL;
		c_final = NUL;
		c = *p_extra++;
		/* Use special coloring to be able to distinguish <hex> from
		 * the same in plain text. */
		attr = HL_ATTR(HLF_8);
	    }
	    else if (c == ' ' && trail != NULL && s > trail)
	    {
		c = lcs_trail;
		attr = HL_ATTR(HLF_8);
	    }
	    else if (c == ' ' && list && lcs_space != NUL)
	    {
		c = lcs_space;
		attr = HL_ATTR(HLF_8);
	    }
	}

	if (c == NUL)
	    break;

	msg_putchar_attr(c, attr);
	col++;
    }
    msg_clr_eos();
}

/*
 * Use screen_puts() to output one multi-byte character.
 * Return the pointer "s" advanced to the next character.
 */
    static char_u *
screen_puts_mbyte(char_u *s, int l, int attr)
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
	msg_screen_putchar('>', HL_ATTR(HLF_AT));
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

/*
 * Output a string to the screen at position msg_row, msg_col.
 * Update msg_row and msg_col for the next message.
 */
    void
msg_puts(char *s)
{
    msg_puts_attr(s, 0);
}

    void
msg_puts_title(char *s)
{
    msg_puts_attr(s, HL_ATTR(HLF_T));
}

/*
 * Show a message in such a way that it always fits in the line.  Cut out a
 * part in the middle and replace it with "..." when necessary.
 * Does not handle multi-byte characters!
 */
    void
msg_outtrans_long_attr(char_u *longstr, int attr)
{
    msg_outtrans_long_len_attr(longstr, (int)STRLEN(longstr), attr);
}

    void
msg_outtrans_long_len_attr(char_u *longstr, int len, int attr)
{
    int		slen = len;
    int		room;

    room = Columns - msg_col;
    if (len > room && room >= 20)
    {
	slen = (room - 3) / 2;
	msg_outtrans_len_attr(longstr, slen, attr);
	msg_puts_attr("...", HL_ATTR(HLF_8));
    }
    msg_outtrans_len_attr(longstr + len - slen, slen, attr);
}

/*
 * Basic function for writing a message with highlight attributes.
 */
    void
msg_puts_attr(char *s, int attr)
{
    msg_puts_attr_len(s, -1, attr);
}

/*
 * Like msg_puts_attr(), but with a maximum length "maxlen" (in bytes).
 * When "maxlen" is -1 there is no maximum length.
 * When "maxlen" is >= 0 the message is not put in the history.
 */
    static void
msg_puts_attr_len(char *str, int maxlen, int attr)
{
    /*
     * If redirection is on, also write to the redirection file.
     */
    redir_write((char_u *)str, maxlen);

    /*
     * Don't print anything when using ":silent cmd".
     */
    if (msg_silent != 0)
	return;

    /* if MSG_HIST flag set, add message to history */
    if ((attr & MSG_HIST) && maxlen < 0)
    {
	add_msg_hist((char_u *)str, -1, attr);
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
	msg_puts_printf((char_u *)str, maxlen);
    else
	msg_puts_display((char_u *)str, maxlen, attr, FALSE);
}

/*
 * The display part of msg_puts_attr_len().
 * May be called recursively to display scroll-back text.
 */
    static void
msg_puts_display(
    char_u	*str,
    int		maxlen,
    int		attr,
    int		recurse)
{
    char_u	*s = str;
    char_u	*t_s = str;	/* string from "t_s" to "s" is still todo */
    int		t_col = 0;	/* screen cells todo, 0 when "t_s" not used */
    int		l;
    int		cw;
    char_u	*sb_str = str;
    int		sb_col = msg_col;
    int		wrap;
    int		did_last_char;

    did_wait_return = FALSE;
    while ((maxlen < 0 || (int)(s - str) < maxlen) && *s != NUL)
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
		      || (has_mbyte && (*mb_ptr2cells)(s) > 1 && msg_col <= 2))
		    :
#endif
		      (msg_col + t_col >= Columns - 1
		       || (*s == TAB && msg_col + t_col >= ((Columns - 1) & ~7))
		       || (has_mbyte && (*mb_ptr2cells)(s) > 1
					 && msg_col + t_col >= Columns - 2)))))
	{
	    /*
	     * The screen is scrolled up when at the last row (some terminals
	     * scroll automatically, some don't.  To avoid problems we scroll
	     * ourselves).
	     */
	    if (t_col > 0)
		/* output postponed text */
		t_puts(&t_col, t_s, s, attr);

	    /* When no more prompt and no more room, truncate here */
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
		    msg_screen_putchar(*s++, attr);
		did_last_char = TRUE;
	    }
	    else
		did_last_char = FALSE;

	    if (p_more)
		/* store text for scrolling back */
		store_sb_text(&sb_str, s, attr, &sb_col, TRUE);

	    inc_msg_scrolled();
	    need_wait_return = TRUE; /* may need wait_return in main() */
	    redraw_cmdline = TRUE;
	    if (cmdline_row > 0 && !exmode_active)
		--cmdline_row;

	    /*
	     * If screen is completely filled and 'more' is set then wait
	     * for a character.
	     */
	    if (lines_left > 0)
		--lines_left;
	    if (p_more && lines_left == 0 && State != HITRETURN
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
	    if (did_last_char)
		continue;
	}

	wrap = *s == '\n'
		    || msg_col + t_col >= Columns
		    || (has_mbyte && (*mb_ptr2cells)(s) > 1
					    && msg_col + t_col >= Columns - 1);
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
	else if (*s == BELL)		/* beep (from ":sh") */
	    vim_beep(BO_SH);
	else
	{
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

	    /* When drawing from right to left or when a double-wide character
	     * doesn't fit, draw a single character here.  Otherwise collect
	     * characters and draw them all at once later. */
	    if (
# ifdef FEAT_RIGHTLEFT
		    cmdmsg_rl ||
# endif
		    (cw > 1 && msg_col + t_col >= Columns - 1))
	    {
		if (l > 1)
		    s = screen_puts_mbyte(s, l, attr) - 1;
		else
		    msg_screen_putchar(*s, attr);
	    }
	    else
	    {
		/* postpone this character until later */
		if (t_col == 0)
		    t_s = s;
		t_col += cw;
		s += l - 1;
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
 * Return TRUE when ":filter pattern" was used and "msg" does not match
 * "pattern".
 */
    int
message_filtered(char_u *msg)
{
    int match;

    if (cmdmod.filter_regmatch.regprog == NULL)
	return FALSE;
    match = vim_regexec(&cmdmod.filter_regmatch, msg, (colnr_T)0);
    return cmdmod.filter_force ? match : !match;
}

/*
 * Scroll the screen up one line for displaying the next message line.
 */
    static void
msg_scroll_up(void)
{
#ifdef FEAT_GUI
    /* Remove the cursor before scrolling, ScreenLines[] is going
     * to become invalid. */
    if (gui.in_use)
	gui_undraw_cursor();
#endif
    /* scrolling up always works */
    mch_disable_flush();
    screen_del_lines(0, 0, 1, (int)Rows, TRUE, 0, NULL);
    mch_enable_flush();

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
inc_msg_scrolled(void)
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
    if (must_redraw < VALID)
	must_redraw = VALID;
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

static msgchunk_T *msg_sb_start(msgchunk_T *mps);

typedef enum {
    SB_CLEAR_NONE = 0,
    SB_CLEAR_ALL,
    SB_CLEAR_CMDLINE_BUSY,
    SB_CLEAR_CMDLINE_DONE
} sb_clear_T;

/* When to clear text on next msg. */
static sb_clear_T do_clear_sb_text = SB_CLEAR_NONE;

/*
 * Store part of a printed message for displaying when scrolling back.
 */
    static void
store_sb_text(
    char_u	**sb_str,	/* start of string */
    char_u	*s,		/* just after string */
    int		attr,
    int		*sb_col,
    int		finish)		/* line ends */
{
    msgchunk_T	*mp;

    if (do_clear_sb_text == SB_CLEAR_ALL
	    || do_clear_sb_text == SB_CLEAR_CMDLINE_DONE)
    {
	clear_sb_text(do_clear_sb_text == SB_CLEAR_ALL);
	do_clear_sb_text = SB_CLEAR_NONE;
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
may_clear_sb_text(void)
{
    do_clear_sb_text = SB_CLEAR_ALL;
}

/*
 * Starting to edit the command line, do not clear messages now.
 */
    void
sb_text_start_cmdline(void)
{
    do_clear_sb_text = SB_CLEAR_CMDLINE_BUSY;
    msg_sb_eol();
}

/*
 * Ending to edit the command line.  Clear old lines but the last one later.
 */
    void
sb_text_end_cmdline(void)
{
    do_clear_sb_text = SB_CLEAR_CMDLINE_DONE;
}

/*
 * Clear any text remembered for scrolling back.
 * When "all" is FALSE keep the last line.
 * Called when redrawing the screen.
 */
    void
clear_sb_text(int all)
{
    msgchunk_T	*mp;
    msgchunk_T	**lastp;

    if (all)
	lastp = &last_msgchunk;
    else
    {
	if (last_msgchunk == NULL)
	    return;
	lastp = &last_msgchunk->sb_prev;
    }

    while (*lastp != NULL)
    {
	mp = (*lastp)->sb_prev;
	vim_free(*lastp);
	*lastp = mp;
    }
}

/*
 * "g<" command.
 */
    void
show_sb_text(void)
{
    msgchunk_T	*mp;

    /* Only show something if there is more than one line, otherwise it looks
     * weird, typing a command without output results in one line. */
    mp = msg_sb_start(last_msgchunk);
    if (mp == NULL || mp->sb_prev == NULL)
	vim_beep(BO_MESS);
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
msg_sb_start(msgchunk_T *mps)
{
    msgchunk_T *mp = mps;

    while (mp != NULL && mp->sb_prev != NULL && !mp->sb_prev->sb_eol)
	mp = mp->sb_prev;
    return mp;
}

/*
 * Mark the last message chunk as finishing the line.
 */
    void
msg_sb_eol(void)
{
    if (last_msgchunk != NULL)
	last_msgchunk->sb_eol = TRUE;
}

/*
 * Display a screen line from previously displayed text at row "row".
 * Returns a pointer to the text for the next line (can be NULL).
 */
    static msgchunk_T *
disp_sb_line(int row, msgchunk_T *smp)
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
t_puts(
    int		*t_col,
    char_u	*t_s,
    char_u	*s,
    int		attr)
{
    /* output postponed text */
    msg_didout = TRUE;		/* remember that line is not empty */
    screen_puts_len(t_s, (int)(s - t_s), msg_row, msg_col, attr);
    msg_col += *t_col;
    *t_col = 0;
    /* If the string starts with a composing character don't increment the
     * column position for it. */
    if (enc_utf8 && utf_iscomposing(utf_ptr2char(t_s)))
	--msg_col;
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
msg_use_printf(void)
{
    return (!msg_check_screen()
#if defined(MSWIN) && (!defined(FEAT_GUI_MSWIN) || defined(VIMDLL))
# ifdef VIMDLL
	    || (!gui.in_use && !termcap_active)
# else
	    || !termcap_active
# endif
#endif
	    || (swapping_screen() && !termcap_active)
	       );
}

/*
 * Print a message when there is no valid screen.
 */
    static void
msg_puts_printf(char_u *str, int maxlen)
{
    char_u	*s = str;
    char_u	*buf = NULL;
    char_u	*p = s;

#ifdef MSWIN
    if (!(silent_mode && p_verbose == 0))
	mch_settmode(TMODE_COOK);	/* handle CR and NL correctly */
#endif
    while ((maxlen < 0 || (int)(s - str) < maxlen) && *s != NUL)
    {
	if (!(silent_mode && p_verbose == 0))
	{
	    // NL --> CR NL translation (for Unix, not for "--version")
	    if (*s == NL)
	    {
		int n = (int)(s - p);

		buf = alloc(n + 3);
		memcpy(buf, p, n);
		if (!info_message)
		    buf[n++] = CAR;
		buf[n++] = NL;
		buf[n++] = NUL;
		if (info_message)   // informative message, not an error
		    mch_msg((char *)buf);
		else
		    mch_errmsg((char *)buf);
		vim_free(buf);
		p = s + 1;
	    }
	}

	// primitive way to compute the current column
#ifdef FEAT_RIGHTLEFT
	if (cmdmsg_rl)
	{
	    if (*s == CAR || *s == NL)
		msg_col = Columns - 1;
	    else
		--msg_col;
	}
	else
#endif
	{
	    if (*s == CAR || *s == NL)
		msg_col = 0;
	    else
		++msg_col;
	}
	++s;
    }

    if (*p != NUL && !(silent_mode && p_verbose == 0))
    {
	int c = -1;

	if (maxlen > 0 && STRLEN(p) > (size_t)maxlen)
	{
	    c = p[maxlen];
	    p[maxlen] = 0;
	}
	if (info_message)
	    mch_msg((char *)p);
	else
	    mch_errmsg((char *)p);
	if (c != -1)
	    p[maxlen] = c;
    }

    msg_didout = TRUE;	    // assume that line is not empty

#ifdef MSWIN
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
do_more_prompt(int typed_char)
{
    static int	entered = FALSE;
    int		used_typed_char = typed_char;
    int		oldState = State;
    int		c;
#ifdef FEAT_CON_DIALOG
    int		retval = FALSE;
#endif
    int		toscroll;
    msgchunk_T	*mp_last = NULL;
    msgchunk_T	*mp;
    int		i;

    /* We get called recursively when a timer callback outputs a message. In
     * that case don't show another prompt. Also when at the hit-Enter prompt
     * and nothing was typed. */
    if (entered || (State == HITRETURN && typed_char == 0))
	return FALSE;
    entered = TRUE;

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

	toscroll = 0;
	switch (c)
	{
	case BS:		/* scroll one line back */
	case K_BS:
	case 'k':
	case K_UP:
	    toscroll = -1;
	    break;

	case CAR:		/* one extra line */
	case NL:
	case 'j':
	case K_DOWN:
	    toscroll = 1;
	    break;

	case 'u':		/* Up half a page */
	    toscroll = -(Rows / 2);
	    break;

	case 'd':		/* Down half a page */
	    toscroll = Rows / 2;
	    break;

	case 'b':		/* one page back */
	case K_PAGEUP:
	    toscroll = -(Rows - 1);
	    break;

	case ' ':		/* one extra page */
	case 'f':
	case K_PAGEDOWN:
	case K_LEFTMOUSE:
	    toscroll = Rows - 1;
	    break;

	case 'g':		/* all the way back to the start */
	    toscroll = -999999;
	    break;

	case 'G':		/* all the way to the end */
	    toscroll = 999999;
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
#ifdef FEAT_TERMINAL
		skip_term_loop = TRUE;
#endif
		cmdline_row = Rows - 1;		/* put ':' on this line */
		skip_redraw = TRUE;		/* skip redraw once */
		need_wait_return = FALSE;	/* don't wait in main() */
	    }
	    /* FALLTHROUGH */
	case 'q':		/* quit */
	case Ctrl_C:
	case ESC:
#ifdef FEAT_CON_DIALOG
	    if (confirm_msg_used)
	    {
		/* Jump to the choices of the dialog. */
		retval = TRUE;
	    }
	    else
#endif
	    {
		got_int = TRUE;
		quit_more = TRUE;
	    }
	    /* When there is some more output (wrapping line) display that
	     * without another prompt. */
	    lines_left = Rows - 1;
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

	if (toscroll != 0)
	{
	    if (toscroll < 0)
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
		    for (i = 0; i > toscroll; --i)
		    {
			if (mp == NULL || mp->sb_prev == NULL)
			    break;
			mp = msg_sb_start(mp->sb_prev);
			if (mp_last == NULL)
			    mp_last = msg_sb_start(last_msgchunk);
			else
			    mp_last = msg_sb_start(mp_last->sb_prev);
		    }

		    if (toscroll == -1 && screen_ins_lines(0, 0, 1,
						     (int)Rows, 0, NULL) == OK)
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
		    toscroll = 0;
		}
	    }
	    else
	    {
		/* First display any text that we scrolled back. */
		while (toscroll > 0 && mp_last != NULL)
		{
		    /* scroll up, display line at bottom */
		    msg_scroll_up();
		    inc_msg_scrolled();
		    screen_fill((int)Rows - 2, (int)Rows - 1, 0,
						   (int)Columns, ' ', ' ', 0);
		    mp_last = disp_sb_line((int)Rows - 2, mp_last);
		    --toscroll;
		}
	    }

	    if (toscroll <= 0)
	    {
		/* displayed the requested text, more prompt again */
		screen_fill((int)Rows - 1, (int)Rows, 0,
						   (int)Columns, ' ', ' ', 0);
		msg_moremsg(FALSE);
		continue;
	    }

	    /* display more text, return to caller */
	    lines_left = toscroll;
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

    entered = FALSE;
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

#if defined(MSWIN) && (!defined(FEAT_GUI_MSWIN) || defined(VIMDLL))
    static void
mch_errmsg_c(char *str)
{
    int	    len = (int)STRLEN(str);
    DWORD   nwrite = 0;
    DWORD   mode = 0;
    HANDLE  h = GetStdHandle(STD_ERROR_HANDLE);

    if (GetConsoleMode(h, &mode) && enc_codepage >= 0
	    && (int)GetConsoleCP() != enc_codepage)
    {
	WCHAR	*w = enc_to_utf16((char_u *)str, &len);

	WriteConsoleW(h, w, len, &nwrite, NULL);
	vim_free(w);
    }
    else
    {
	fprintf(stderr, "%s", str);
    }
}
#endif

/*
 * Give an error message.  To be used when the screen hasn't been initialized
 * yet.  When stderr can't be used, collect error messages until the GUI has
 * started and they can be displayed in a message box.
 */
    void
mch_errmsg(char *str)
{
#if !defined(MSWIN) || defined(FEAT_GUI_MSWIN)
    int		len;
#endif

#if (defined(UNIX) || defined(FEAT_GUI)) && (!defined(ALWAYS_USE_GUI) || !defined(VIMDLL))
    /* On Unix use stderr if it's a tty.
     * When not going to start the GUI also use stderr.
     * On Mac, when started from Finder, stderr is the console. */
    if (
# ifdef UNIX
#  ifdef MACOS_X
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

#if defined(MSWIN) && (!defined(FEAT_GUI_MSWIN) || defined(VIMDLL))
# ifdef VIMDLL
    if (!(gui.in_use || gui.starting))
# endif
    {
	mch_errmsg_c(str);
	return;
    }
#endif

#if !defined(MSWIN) || defined(FEAT_GUI_MSWIN)
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
# ifdef UNIX
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
# endif
	--len;		/* don't count the NUL at the end */
	error_ga.ga_len += len;
    }
#endif
}

#if defined(MSWIN) && (!defined(FEAT_GUI_MSWIN) || defined(VIMDLL))
    static void
mch_msg_c(char *str)
{
    int	    len = (int)STRLEN(str);
    DWORD   nwrite = 0;
    DWORD   mode;
    HANDLE  h = GetStdHandle(STD_OUTPUT_HANDLE);


    if (GetConsoleMode(h, &mode) && enc_codepage >= 0
	    && (int)GetConsoleCP() != enc_codepage)
    {
	WCHAR	*w = enc_to_utf16((char_u *)str, &len);

	WriteConsoleW(h, w, len, &nwrite, NULL);
	vim_free(w);
    }
    else
    {
	printf("%s", str);
    }
}
#endif

/*
 * Give a message.  To be used when the screen hasn't been initialized yet.
 * When there is no tty, collect messages until the GUI has started and they
 * can be displayed in a message box.
 */
    void
mch_msg(char *str)
{
#if (defined(UNIX) || defined(FEAT_GUI)) && (!defined(ALWAYS_USE_GUI) || !defined(VIMDLL))
    /* On Unix use stdout if we have a tty.  This allows "vim -h | more" and
     * uses mch_errmsg() when started from the desktop.
     * When not going to start the GUI also use stdout.
     * On Mac, when started from Finder, stderr is the console. */
    if (
# ifdef UNIX
#  ifdef MACOS_X
	    (isatty(2) && strcmp("/dev/console", ttyname(2)) != 0)
#  else
	    isatty(2)
#   endif
#  ifdef FEAT_GUI
	    ||
#  endif
# endif
# ifdef FEAT_GUI
	    !(gui.in_use || gui.starting)
# endif
	    )
    {
	printf("%s", str);
	return;
    }
#endif

#if defined(MSWIN) && (!defined(FEAT_GUI_MSWIN) || defined(VIMDLL))
# ifdef VIMDLL
    if (!(gui.in_use || gui.starting))
# endif
    {
	mch_msg_c(str);
	return;
    }
#endif
#if !defined(MSWIN) || defined(FEAT_GUI_MSWIN)
    mch_errmsg(str);
#endif
}
#endif /* USE_MCH_ERRMSG */

/*
 * Put a character on the screen at the current message position and advance
 * to the next position.  Only for printable ASCII!
 */
    static void
msg_screen_putchar(int c, int attr)
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
msg_moremsg(int full)
{
    int		attr;
    char_u	*s = (char_u *)_("-- More --");

    attr = HL_ATTR(HLF_M);
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
repeat_message(void)
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
	if (msg_row == Rows - 1)
	{
	    /* Avoid drawing the "hit-enter" prompt below the previous one,
	     * overwrite it.  Esp. useful when regaining focus and a
	     * FocusGained autocmd exists but didn't draw anything. */
	    msg_didout = FALSE;
	    msg_col = 0;
	    msg_clr_eos();
	}
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
msg_check_screen(void)
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
msg_clr_eos(void)
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
msg_clr_eos_force(void)
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
msg_clr_cmdline(void)
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
msg_end(void)
{
    /*
     * If the string is larger than the window,
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
msg_check(void)
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
redir_write(char_u *str, int maxlen)
{
    char_u	*s = str;
    static int	cur_col = 0;

    /* Don't do anything for displaying prompts and the like. */
    if (redir_off)
	return;

    /* If 'verbosefile' is set prepare for writing in that file. */
    if (*p_vfile != NUL && verbose_fd == NULL)
	verbose_open();

    if (redirecting())
    {
	/* If the string doesn't start with CR or NL, go to msg_col */
	if (*s != '\n' && *s != '\r')
	{
	    while (cur_col < msg_col)
	    {
#ifdef FEAT_EVAL
		if (redir_execute)
		    execute_redir_str((char_u *)" ", -1);
		else if (redir_reg)
		    write_reg_contents(redir_reg, (char_u *)" ", -1, TRUE);
		else if (redir_vname)
		    var_redir_str((char_u *)" ", -1);
		else
#endif
		    if (redir_fd != NULL)
		    fputs(" ", redir_fd);
		if (verbose_fd != NULL)
		    fputs(" ", verbose_fd);
		++cur_col;
	    }
	}

#ifdef FEAT_EVAL
	if (redir_execute)
	    execute_redir_str(s, maxlen);
	else if (redir_reg)
	    write_reg_contents(redir_reg, s, maxlen, TRUE);
	else if (redir_vname)
	    var_redir_str(s, maxlen);
#endif

	/* Write and adjust the current column. */
	while (*s != NUL && (maxlen < 0 || (int)(s - str) < maxlen))
	{
#ifdef FEAT_EVAL
	    if (!redir_reg && !redir_vname && !redir_execute)
#endif
		if (redir_fd != NULL)
		    putc(*s, redir_fd);
	    if (verbose_fd != NULL)
		putc(*s, verbose_fd);
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

    int
redirecting(void)
{
    return redir_fd != NULL || *p_vfile != NUL
#ifdef FEAT_EVAL
			  || redir_reg || redir_vname || redir_execute
#endif
				       ;
}

/*
 * Before giving verbose message.
 * Must always be called paired with verbose_leave()!
 */
    void
verbose_enter(void)
{
    if (*p_vfile != NUL)
	++msg_silent;
}

/*
 * After giving verbose message.
 * Must always be called paired with verbose_enter()!
 */
    void
verbose_leave(void)
{
    if (*p_vfile != NUL)
	if (--msg_silent < 0)
	    msg_silent = 0;
}

/*
 * Like verbose_enter() and set msg_scroll when displaying the message.
 */
    void
verbose_enter_scroll(void)
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
verbose_leave_scroll(void)
{
    if (*p_vfile != NUL)
    {
	if (--msg_silent < 0)
	    msg_silent = 0;
    }
    else
	cmdline_row = msg_row;
}

/*
 * Called when 'verbosefile' is set: stop writing to the file.
 */
    void
verbose_stop(void)
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
verbose_open(void)
{
    if (verbose_fd == NULL && !verbose_did_open)
    {
	/* Only give the error message once. */
	verbose_did_open = TRUE;

	verbose_fd = mch_fopen((char *)p_vfile, "a");
	if (verbose_fd == NULL)
	{
	    semsg(_(e_notopen), p_vfile);
	    return FAIL;
	}
    }
    return OK;
}

/*
 * Give a warning message (for searching).
 * Use 'w' highlighting and may repeat the message after redrawing
 */
    void
give_warning(char_u *message, int hl)
{
    /* Don't do this for ":silent". */
    if (msg_silent != 0)
	return;

    /* Don't want a hit-enter prompt here. */
    ++no_wait_return;

#ifdef FEAT_EVAL
    set_vim_var_string(VV_WARNINGMSG, message, -1);
#endif
    VIM_CLEAR(keep_msg);
    if (hl)
	keep_msg_attr = HL_ATTR(HLF_W);
    else
	keep_msg_attr = 0;
    if (msg_attr((char *)message, keep_msg_attr) && msg_scrolled == 0)
	set_keep_msg(message, keep_msg_attr);
    msg_didout = FALSE;	    /* overwrite this message */
    msg_nowait = TRUE;	    /* don't wait for this message */
    msg_col = 0;

    --no_wait_return;
}

#if defined(FEAT_EVAL) || defined(PROTO)
    void
give_warning2(char_u *message, char_u *a1, int hl)
{
    vim_snprintf((char *)IObuff, IOSIZE, (char *)message, a1);
    give_warning(IObuff, hl);
}
#endif

/*
 * Advance msg cursor to column "col".
 */
    void
msg_advance(int col)
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
    int
do_dialog(
    int		type UNUSED,
    char_u	*title UNUSED,
    char_u	*message,
    char_u	*buttons,
    int		dfltbutton,
    char_u	*textfield UNUSED,	/* IObuff for inputdialog(), NULL
					   otherwise */
    int		ex_cmd)	    /* when TRUE pressing : accepts default and starts
			       Ex command */
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
							   textfield, ex_cmd);
	/* avoid a hit-enter prompt without clearing the cmdline */
	need_wait_return = FALSE;
	emsg_on_display = FALSE;
	cmdline_row = msg_row;

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
		if (c == ':' && ex_cmd)
		{
		    retval = dfltbutton;
		    ins_char_typebuf(':');
		    break;
		}

		/* Make the character lowercase, as chars in "hotkeys" are. */
		c = MB_TOLOWER(c);
		retval = 1;
		for (i = 0; hotkeys[i]; ++i)
		{
		    if (has_mbyte)
		    {
			if ((*mb_ptr2char)(hotkeys + i) == c)
			    break;
			i += (*mb_ptr2len)(hotkeys + i) - 1;
		    }
		    else
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

/*
 * Copy one character from "*from" to "*to", taking care of multi-byte
 * characters.  Return the length of the character in bytes.
 */
    static int
copy_char(
    char_u	*from,
    char_u	*to,
    int		lowercase)	/* make character lower case */
{
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
msg_show_console_dialog(
    char_u	*message,
    char_u	*buttons,
    int		dfltbutton)
{
    int		len = 0;
#define HOTK_LEN (has_mbyte ? MB_MAXBYTES : 1)
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
		    if (has_mbyte)
			hotkp += STRLEN(hotkp);
		    else
			++hotkp;
		    hotkp[copy_char(r + 1, hotkp, TRUE)] = NUL;
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
			hotkp[copy_char(r, hotkp, TRUE)] = NUL;
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
	    MB_PTR_ADV(r);
	}

	if (copy)
	{
	    *msgp++ = ':';
	    *msgp++ = ' ';
	    *msgp = NUL;
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

	    /* Define first default hotkey.  Keep the hotkey string NUL
	     * terminated to avoid reading past the end. */
	    hotkp[copy_char(buttons, hotkp, TRUE)] = NUL;

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
display_confirm_msg(void)
{
    /* avoid that 'q' at the more prompt truncates the message here */
    ++confirm_msg_used;
    if (confirm_msg != NULL)
	msg_puts_attr((char *)confirm_msg, HL_ATTR(HLF_M));
    --confirm_msg_used;
}

#endif /* FEAT_CON_DIALOG */

#if defined(FEAT_CON_DIALOG) || defined(FEAT_GUI_DIALOG)

    int
vim_dialog_yesno(
    int		type,
    char_u	*title,
    char_u	*message,
    int		dflt)
{
    if (do_dialog(type,
		title == NULL ? (char_u *)_("Question") : title,
		message,
		(char_u *)_("&Yes\n&No"), dflt, NULL, FALSE) == 1)
	return VIM_YES;
    return VIM_NO;
}

    int
vim_dialog_yesnocancel(
    int		type,
    char_u	*title,
    char_u	*message,
    int		dflt)
{
    switch (do_dialog(type,
		title == NULL ? (char_u *)_("Question") : title,
		message,
		(char_u *)_("&Yes\n&No\n&Cancel"), dflt, NULL, FALSE))
    {
	case 1: return VIM_YES;
	case 2: return VIM_NO;
    }
    return VIM_CANCEL;
}

    int
vim_dialog_yesnoallcancel(
    int		type,
    char_u	*title,
    char_u	*message,
    int		dflt)
{
    switch (do_dialog(type,
		title == NULL ? (char_u *)"Question" : title,
		message,
		(char_u *)_("&Yes\n&No\nSave &All\n&Discard All\n&Cancel"),
							   dflt, NULL, FALSE))
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
do_browse(
    int		flags,		/* BROWSE_SAVE and BROWSE_DIR */
    char_u	*title,		/* title for the window */
    char_u	*dflt,		/* default file name (may include directory) */
    char_u	*ext,		/* extension added */
    char_u	*initdir,	/* initial directory, NULL for current dir or
				   when using path from "dflt" */
    char_u	*filter,	/* file name filter */
    buf_T	*buf)		/* buffer to read/write for */
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
#  if defined(FEAT_GUI_GTK) || defined(MSWIN)
	    /* For systems that have a directory dialog. */
	    fname = gui_mch_browsedir(title, initdir);
#  else
	    /* Generic solution for selecting a directory: select a file and
	     * remove the file name. */
	    fname = gui_mch_browse(0, title, dflt, ext, initdir, (char_u *)"");
#  endif
#  if !defined(FEAT_GUI_GTK)
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
			       title, dflt, ext, initdir, (char_u *)_(filter));

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
	emsg(_("E338: Sorry, no file browser in console mode"));
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

#if defined(FEAT_EVAL)
static char *e_printf = N_("E766: Insufficient arguments for printf()");

/*
 * Get number argument from "idxp" entry in "tvs".  First entry is 1.
 */
    static varnumber_T
tv_nr(typval_T *tvs, int *idxp)
{
    int		idx = *idxp - 1;
    varnumber_T	n = 0;
    int		err = FALSE;

    if (tvs[idx].v_type == VAR_UNKNOWN)
	emsg(_(e_printf));
    else
    {
	++*idxp;
	n = tv_get_number_chk(&tvs[idx], &err);
	if (err)
	    n = 0;
    }
    return n;
}

/*
 * Get string argument from "idxp" entry in "tvs".  First entry is 1.
 * If "tofree" is NULL tv_get_string_chk() is used.  Some types (e.g. List)
 * are not converted to a string.
 * If "tofree" is not NULL echo_string() is used.  All types are converted to
 * a string with the same format as ":echo".  The caller must free "*tofree".
 * Returns NULL for an error.
 */
    static char *
tv_str(typval_T *tvs, int *idxp, char_u **tofree)
{
    int		    idx = *idxp - 1;
    char	    *s = NULL;
    static char_u   numbuf[NUMBUFLEN];

    if (tvs[idx].v_type == VAR_UNKNOWN)
	emsg(_(e_printf));
    else
    {
	++*idxp;
	if (tofree != NULL)
	    s = (char *)echo_string(&tvs[idx], tofree, numbuf, get_copyID());
	else
	    s = (char *)tv_get_string_chk(&tvs[idx]);
    }
    return s;
}

# ifdef FEAT_FLOAT
/*
 * Get float argument from "idxp" entry in "tvs".  First entry is 1.
 */
    static double
tv_float(typval_T *tvs, int *idxp)
{
    int		idx = *idxp - 1;
    double	f = 0;

    if (tvs[idx].v_type == VAR_UNKNOWN)
	emsg(_(e_printf));
    else
    {
	++*idxp;
	if (tvs[idx].v_type == VAR_FLOAT)
	    f = tvs[idx].vval.v_float;
	else if (tvs[idx].v_type == VAR_NUMBER)
	    f = (double)tvs[idx].vval.v_number;
	else
	    emsg(_("E807: Expected Float argument for printf()"));
    }
    return f;
}
# endif
#endif

#ifdef FEAT_FLOAT
/*
 * Return the representation of infinity for printf() function:
 * "-inf", "inf", "+inf", " inf", "-INF", "INF", "+INF" or " INF".
 */
    static const char *
infinity_str(int positive,
	     char fmt_spec,
	     int force_sign,
	     int space_for_positive)
{
    static const char *table[] =
    {
	"-inf", "inf", "+inf", " inf",
	"-INF", "INF", "+INF", " INF"
    };
    int idx = positive * (1 + force_sign + force_sign * space_for_positive);

    if (ASCII_ISUPPER(fmt_spec))
	idx += 4;
    return table[idx];
}
#endif

/*
 * This code was included to provide a portable vsnprintf() and snprintf().
 * Some systems may provide their own, but we always use this one for
 * consistency.
 *
 * This code is based on snprintf.c - a portable implementation of snprintf
 * by Mark Martinec <mark.martinec@ijs.si>, Version 2.2, 2000-10-06.
 * Included with permission.  It was heavily modified to fit in Vim.
 * The original code, including useful comments, can be found here:
 *	http://www.ijs.si/software/snprintf/
 *
 * This snprintf() only supports the following conversion specifiers:
 * s, c, d, u, o, x, X, p  (and synonyms: i, D, U, O - see below)
 * with flags: '-', '+', ' ', '0' and '#'.
 * An asterisk is supported for field width as well as precision.
 *
 * Limited support for floating point was added: 'f', 'F', 'e', 'E', 'g', 'G'.
 *
 * Length modifiers 'h' (short int) and 'l' (long int) and 'll' (long long int)
 * are supported.
 *
 * The locale is not used, the string is used as a byte string.  This is only
 * relevant for double-byte encodings where the second byte may be '%'.
 *
 * It is permitted for "str_m" to be zero, and it is permitted to specify NULL
 * pointer for resulting string argument if "str_m" is zero (as per ISO C99).
 *
 * The return value is the number of characters which would be generated
 * for the given input, excluding the trailing NUL. If this value
 * is greater or equal to "str_m", not all characters from the result
 * have been stored in str, output bytes beyond the ("str_m"-1) -th character
 * are discarded. If "str_m" is greater than zero it is guaranteed
 * the resulting string will be NUL-terminated.
 */

/*
 * When va_list is not supported we only define vim_snprintf().
 *
 * vim_vsnprintf_typval() can be invoked with either "va_list" or a list of
 * "typval_T".  When the latter is not used it must be NULL.
 */

/* When generating prototypes all of this is skipped, cproto doesn't
 * understand this. */
#ifndef PROTO

/* Like vim_vsnprintf() but append to the string. */
    int
vim_snprintf_add(char *str, size_t str_m, const char *fmt, ...)
{
    va_list	ap;
    int		str_l;
    size_t	len = STRLEN(str);
    size_t	space;

    if (str_m <= len)
	space = 0;
    else
	space = str_m - len;
    va_start(ap, fmt);
    str_l = vim_vsnprintf(str + len, space, fmt, ap);
    va_end(ap);
    return str_l;
}

    int
vim_snprintf(char *str, size_t str_m, const char *fmt, ...)
{
    va_list	ap;
    int		str_l;

    va_start(ap, fmt);
    str_l = vim_vsnprintf(str, str_m, fmt, ap);
    va_end(ap);
    return str_l;
}

    int
vim_vsnprintf(
    char	*str,
    size_t	str_m,
    const char	*fmt,
    va_list	ap)
{
    return vim_vsnprintf_typval(str, str_m, fmt, ap, NULL);
}

    int
vim_vsnprintf_typval(
    char	*str,
    size_t	str_m,
    const char	*fmt,
    va_list	ap,
    typval_T	*tvs)
{
    size_t	str_l = 0;
    const char	*p = fmt;
    int		arg_idx = 1;

    if (p == NULL)
	p = "";
    while (*p != NUL)
    {
	if (*p != '%')
	{
	    char    *q = strchr(p + 1, '%');
	    size_t  n = (q == NULL) ? STRLEN(p) : (size_t)(q - p);

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
# if defined(FEAT_FLOAT)
#  define TMP_LEN 350	/* On my system 1e308 is the biggest number possible.
			 * That sounds reasonable to use as the maximum
			 * printable. */
# elif defined(FEAT_NUM64)
#  define TMP_LEN 66
# else
#  define TMP_LEN 34
# endif
	    char    tmp[TMP_LEN];

	    /* string address in case of string argument */
	    const char  *str_arg = NULL;

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

	    /* buffer for 's' and 'S' specs */
	    char_u  *tofree = NULL;


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
# if defined(FEAT_EVAL)
		    tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
			va_arg(ap, int);
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
# if defined(FEAT_EVAL)
			tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
			    va_arg(ap, int);
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
# ifdef FEAT_NUM64
		    length_modifier = 'L';
# else
		    length_modifier = 'l';	/* treat it as a single 'l' */
# endif
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

# if defined(FEAT_EVAL) && defined(FEAT_NUM64)
	    switch (fmt_spec)
	    {
		case 'd': case 'u': case 'o': case 'x': case 'X':
		    if (tvs != NULL && length_modifier == '\0')
			length_modifier = 'L';
	    }
# endif

	    /* get parameter value, do initial processing */
	    switch (fmt_spec)
	    {
		/* '%' and 'c' behave similar to 's' regarding flags and field
		 * widths */
	    case '%':
	    case 'c':
	    case 's':
	    case 'S':
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
# if defined(FEAT_EVAL)
			    tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
				va_arg(ap, int);
			/* standard demands unsigned char */
			uchar_arg = (unsigned char)j;
			str_arg = (char *)&uchar_arg;
			break;
		    }

		case 's':
		case 'S':
		    str_arg =
# if defined(FEAT_EVAL)
				tvs != NULL ? tv_str(tvs, &arg_idx, &tofree) :
# endif
				    va_arg(ap, char *);
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
			/* Don't put the #if inside memchr(), it can be a
			 * macro. */
			/* memchr on HP does not like n > 2^31  !!! */
			char *q = memchr(str_arg, '\0',
				  precision <= (size_t)0x7fffffffL ? precision
						       : (size_t)0x7fffffffL);
			str_arg_l = (q == NULL) ? precision
						      : (size_t)(q - str_arg);
		    }
		    if (fmt_spec == 'S')
		    {
			if (min_field_width != 0)
			    min_field_width += STRLEN(str_arg)
				     - mb_string2cells((char_u *)str_arg, -1);
			if (precision)
			{
			    char_u *p1 = (char_u *)str_arg;
			    size_t i;

			    for (i = 0; i < precision && *p1; i++)
				p1 += mb_ptr2len(p1);

			    str_arg_l = precision = p1 - (char_u *)str_arg;
			}
		    }
		    break;

		default:
		    break;
		}
		break;

	    case 'd': case 'u':
	    case 'b': case 'B':
	    case 'o':
	    case 'x': case 'X':
	    case 'p':
		{
		    /* NOTE: the u, b, o, x, X and p conversion specifiers
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

# ifdef FEAT_NUM64
		    /* only defined for length modifier ll */
		    varnumber_T llong_arg = 0;
		    uvarnumber_T ullong_arg = 0;
# endif

		    /* only defined for b conversion */
		    uvarnumber_T bin_arg = 0;

		    /* pointer argument value -only defined for p
		     * conversion */
		    void *ptr_arg = NULL;

		    if (fmt_spec == 'p')
		    {
			length_modifier = '\0';
			ptr_arg =
# if defined(FEAT_EVAL)
				 tvs != NULL ? (void *)tv_str(tvs, &arg_idx,
									NULL) :
# endif
					va_arg(ap, void *);
			if (ptr_arg != NULL)
			    arg_sign = 1;
		    }
		    else if (fmt_spec == 'b' || fmt_spec == 'B')
		    {
			bin_arg =
# if defined(FEAT_EVAL)
				    tvs != NULL ?
					   (uvarnumber_T)tv_nr(tvs, &arg_idx) :
# endif
					va_arg(ap, uvarnumber_T);
			if (bin_arg != 0)
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
# if defined(FEAT_EVAL)
					tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
					    va_arg(ap, int);
			    if (int_arg > 0)
				arg_sign =  1;
			    else if (int_arg < 0)
				arg_sign = -1;
			    break;
			case 'l':
			    long_arg =
# if defined(FEAT_EVAL)
					tvs != NULL ? tv_nr(tvs, &arg_idx) :
# endif
					    va_arg(ap, long int);
			    if (long_arg > 0)
				arg_sign =  1;
			    else if (long_arg < 0)
				arg_sign = -1;
			    break;
# ifdef FEAT_NUM64
			case 'L':
			    llong_arg =
#  if defined(FEAT_EVAL)
					tvs != NULL ? tv_nr(tvs, &arg_idx) :
#  endif
					    va_arg(ap, varnumber_T);
			    if (llong_arg > 0)
				arg_sign =  1;
			    else if (llong_arg < 0)
				arg_sign = -1;
			    break;
# endif
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
# if defined(FEAT_EVAL)
					    tvs != NULL ? (unsigned)
							tv_nr(tvs, &arg_idx) :
# endif
						va_arg(ap, unsigned int);
				if (uint_arg != 0)
				    arg_sign = 1;
				break;
			    case 'l':
				ulong_arg =
# if defined(FEAT_EVAL)
					    tvs != NULL ? (unsigned long)
							tv_nr(tvs, &arg_idx) :
# endif
						va_arg(ap, unsigned long int);
				if (ulong_arg != 0)
				    arg_sign = 1;
				break;
# ifdef FEAT_NUM64
			    case 'L':
				ullong_arg =
#  if defined(FEAT_EVAL)
					    tvs != NULL ? (uvarnumber_T)
							tv_nr(tvs, &arg_idx) :
#  endif
						va_arg(ap, uvarnumber_T);
				if (ullong_arg != 0)
				    arg_sign = 1;
				break;
# endif
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
				     && (fmt_spec == 'b' || fmt_spec == 'B'
				      || fmt_spec == 'x' || fmt_spec == 'X') )
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
			 * empty (d, i, u, b, B, o, x, X, p).   */
		    }
		    else
		    {
			char	f[6];
			int	f_l = 0;

			/* construct a simple format string for sprintf */
			f[f_l++] = '%';
			if (!length_modifier)
			    ;
			else if (length_modifier == 'L')
			{
# ifdef FEAT_NUM64
#  ifdef MSWIN
			    f[f_l++] = 'I';
			    f[f_l++] = '6';
			    f[f_l++] = '4';
#  else
			    f[f_l++] = 'l';
			    f[f_l++] = 'l';
#  endif
# else
			    f[f_l++] = 'l';
# endif
			}
			else
			    f[f_l++] = length_modifier;
			f[f_l++] = fmt_spec;
			f[f_l++] = '\0';

			if (fmt_spec == 'p')
			    str_arg_l += sprintf(tmp + str_arg_l, f, ptr_arg);
			else if (fmt_spec == 'b' || fmt_spec == 'B')
			{
			    char	    b[8 * sizeof(uvarnumber_T)];
			    size_t	    b_l = 0;
			    uvarnumber_T    bn = bin_arg;

			    do
			    {
				b[sizeof(b) - ++b_l] = '0' + (bn & 0x1);
				bn >>= 1;
			    }
			    while (bn != 0);

			    memcpy(tmp + str_arg_l, b + sizeof(b) - b_l, b_l);
			    str_arg_l += b_l;
			}
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
# ifdef FEAT_NUM64
			    case 'L': str_arg_l += sprintf(
					       tmp + str_arg_l, f, llong_arg);
				      break;
# endif
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
# ifdef FEAT_NUM64
			    case 'L': str_arg_l += sprintf(
					      tmp + str_arg_l, f, ullong_arg);
				      break;
# endif
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

# ifdef FEAT_FLOAT
	    case 'f':
	    case 'F':
	    case 'e':
	    case 'E':
	    case 'g':
	    case 'G':
		{
		    /* Floating point. */
		    double	f;
		    double	abs_f;
		    char	format[40];
		    int		l;
		    int		remove_trailing_zeroes = FALSE;

		    f =
#  if defined(FEAT_EVAL)
			tvs != NULL ? tv_float(tvs, &arg_idx) :
#  endif
			    va_arg(ap, double);
		    abs_f = f < 0 ? -f : f;

		    if (fmt_spec == 'g' || fmt_spec == 'G')
		    {
			/* Would be nice to use %g directly, but it prints
			 * "1.0" as "1", we don't want that. */
			if ((abs_f >= 0.001 && abs_f < 10000000.0)
							      || abs_f == 0.0)
			    fmt_spec = ASCII_ISUPPER(fmt_spec) ? 'F' : 'f';
			else
			    fmt_spec = fmt_spec == 'g' ? 'e' : 'E';
			remove_trailing_zeroes = TRUE;
		    }

		    if ((fmt_spec == 'f' || fmt_spec == 'F') &&
#  ifdef VAX
			    abs_f > 1.0e38
#  else
			    abs_f > 1.0e307
#  endif
			    )
		    {
			/* Avoid a buffer overflow */
			STRCPY(tmp, infinity_str(f > 0.0, fmt_spec,
					      force_sign, space_for_positive));
			str_arg_l = STRLEN(tmp);
			zero_padding = 0;
		    }
		    else
		    {
			if (isnan(f))
			{
			    /* Not a number: nan or NAN */
			    STRCPY(tmp, ASCII_ISUPPER(fmt_spec) ? "NAN"
								      : "nan");
			    str_arg_l = 3;
			    zero_padding = 0;
			}
			else if (isinf(f))
			{
			    STRCPY(tmp, infinity_str(f > 0.0, fmt_spec,
					      force_sign, space_for_positive));
			    str_arg_l = STRLEN(tmp);
			    zero_padding = 0;
			}
			else
			{
			    /* Regular float number */
			    format[0] = '%';
			    l = 1;
			    if (force_sign)
				format[l++] = space_for_positive ? ' ' : '+';
			    if (precision_specified)
			    {
				size_t max_prec = TMP_LEN - 10;

				/* Make sure we don't get more digits than we
				 * have room for. */
				if ((fmt_spec == 'f' || fmt_spec == 'F')
								&& abs_f > 1.0)
				    max_prec -= (size_t)log10(abs_f);
				if (precision > max_prec)
				    precision = max_prec;
				l += sprintf(format + l, ".%d", (int)precision);
			    }
			    format[l] = fmt_spec == 'F' ? 'f' : fmt_spec;
			    format[l + 1] = NUL;

			    str_arg_l = sprintf(tmp, format, f);
			}

			if (remove_trailing_zeroes)
			{
			    int i;
			    char *tp;

			    /* Using %g or %G: remove superfluous zeroes. */
			    if (fmt_spec == 'f' || fmt_spec == 'F')
				tp = tmp + str_arg_l - 1;
			    else
			    {
				tp = (char *)vim_strchr((char_u *)tmp,
						 fmt_spec == 'e' ? 'e' : 'E');
				if (tp != NULL)
				{
				    /* Remove superfluous '+' and leading
				     * zeroes from the exponent. */
				    if (tp[1] == '+')
				    {
					/* Change "1.0e+07" to "1.0e07" */
					STRMOVE(tp + 1, tp + 2);
					--str_arg_l;
				    }
				    i = (tp[1] == '-') ? 2 : 1;
				    while (tp[i] == '0')
				    {
					/* Change "1.0e07" to "1.0e7" */
					STRMOVE(tp + i, tp + i + 1);
					--str_arg_l;
				    }
				    --tp;
				}
			    }

			    if (tp != NULL && !precision_specified)
				/* Remove trailing zeroes, but keep the one
				 * just after a dot. */
				while (tp > tmp + 2 && *tp == '0'
							     && tp[-1] != '.')
				{
				    STRMOVE(tp, tp + 1);
				    --tp;
				    --str_arg_l;
				}
			}
			else
			{
			    char *tp;

			    /* Be consistent: some printf("%e") use 1.0e+12
			     * and some 1.0e+012.  Remove one zero in the last
			     * case. */
			    tp = (char *)vim_strchr((char_u *)tmp,
						 fmt_spec == 'e' ? 'e' : 'E');
			    if (tp != NULL && (tp[1] == '+' || tp[1] == '-')
					  && tp[2] == '0'
					  && vim_isdigit(tp[3])
					  && vim_isdigit(tp[4]))
			    {
				STRMOVE(tp + 2, tp + 3);
				--str_arg_l;
			    }
			}
		    }
		    if (zero_padding && min_field_width > str_arg_l
					      && (tmp[0] == '-' || force_sign))
		    {
			/* padding 0's should be inserted after the sign */
			number_of_zeros_to_pad = min_field_width - str_arg_l;
			zero_padding_insertion_ind = 1;
		    }
		    str_arg = tmp;
		    break;
		}
# endif

	    default:
		/* unrecognized conversion specifier, keep format string
		 * as-is */
		zero_padding = 0;  /* turn zero padding off for non-numeric
				      conversion */
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
					     (size_t)pn > avail ? avail
								: (size_t)pn);
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
					     (size_t)zn > avail ? avail
								: (size_t)zn);
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
			size_t avail = str_m - str_l;

			vim_memset(str + str_l, '0',
					     (size_t)zn > avail ? avail
								: (size_t)zn);
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
				(size_t)sn > avail ? avail : (size_t)sn);
		    }
		    str_l += sn;
		}
	    }

	    /* insert right padding */
	    if (justify_left)
	    {
		/* right blank padding to the field width */
		int pn = (int)(min_field_width
				      - (str_arg_l + number_of_zeros_to_pad));

		if (pn > 0)
		{
		    if (str_l < str_m)
		    {
			size_t avail = str_m - str_l;

			vim_memset(str + str_l, ' ',
					     (size_t)pn > avail ? avail
								: (size_t)pn);
		    }
		    str_l += pn;
		}
	    }
	    vim_free(tofree);
	}
    }

    if (str_m > 0)
    {
	/* make sure the string is nul-terminated even at the expense of
	 * overwriting the last character (shouldn't happen, but just in case)
	 * */
	str[str_l <= str_m - 1 ? str_l : str_m - 1] = '\0';
    }

    if (tvs != NULL && tvs[arg_idx - 1].v_type != VAR_UNKNOWN)
	emsg(_("E767: Too many arguments to printf()"));

    /* Return the number of characters formatted (excluding trailing nul
     * character), that is, the number of characters that would have been
     * written to the buffer if it were large enough. */
    return (int)str_l;
}

#endif /* PROTO */
