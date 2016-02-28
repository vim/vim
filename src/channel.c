/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Implements communication through a socket or any file handle.
 */

#include "vim.h"

#if defined(FEAT_CHANNEL) || defined(PROTO)

/* TRUE when netbeans is running with a GUI. */
#ifdef FEAT_GUI
# define CH_HAS_GUI (gui.in_use || gui.starting)
#endif

/* Note: when making changes here also adjust configure.in. */
#ifdef WIN32
/* WinSock API is separated from C API, thus we can't use read(), write(),
 * errno... */
# define SOCK_ERRNO errno = WSAGetLastError()
# undef ECONNREFUSED
# define ECONNREFUSED WSAECONNREFUSED
# undef EWOULDBLOCK
# define EWOULDBLOCK WSAEWOULDBLOCK
# undef EINPROGRESS
# define EINPROGRESS WSAEINPROGRESS
# ifdef EINTR
#  undef EINTR
# endif
# define EINTR WSAEINTR
# define sock_write(sd, buf, len) send((SOCKET)sd, buf, len, 0)
# define sock_read(sd, buf, len) recv((SOCKET)sd, buf, len, 0)
# define sock_close(sd) closesocket((SOCKET)sd)
#else
# include <netdb.h>
# include <netinet/in.h>

# include <sys/socket.h>
# ifdef HAVE_LIBGEN_H
#  include <libgen.h>
# endif
# define SOCK_ERRNO
# define sock_write(sd, buf, len) write(sd, buf, len)
# define sock_read(sd, buf, len) read(sd, buf, len)
# define sock_close(sd) close(sd)
# define fd_read(fd, buf, len) read(fd, buf, len)
# define fd_write(sd, buf, len) write(sd, buf, len)
# define fd_close(sd) close(sd)
#endif

/* Whether a redraw is needed for appending a line to a buffer. */
static int channel_need_redraw = FALSE;


#ifdef WIN32
    static int
fd_read(sock_T fd, char *buf, size_t len)
{
    HANDLE h = (HANDLE)fd;
    DWORD nread;

    if (!ReadFile(h, buf, (DWORD)len, &nread, NULL))
	return -1;
    return (int)nread;
}

    static int
fd_write(sock_T fd, char *buf, size_t len)
{
    HANDLE h = (HANDLE)fd;
    DWORD nwrite;

    if (!WriteFile(h, buf, (DWORD)len, &nwrite, NULL))
	return -1;
    return (int)nwrite;
}

    static void
fd_close(sock_T fd)
{
    HANDLE h = (HANDLE)fd;

    CloseHandle(h);
}
#endif

/* Log file opened with ch_logfile(). */
static FILE *log_fd = NULL;
#ifdef FEAT_RELTIME
static proftime_T log_start;
#endif

    void
ch_logfile(FILE *file)
{
    if (log_fd != NULL)
	fclose(log_fd);
    log_fd = file;
    if (log_fd != NULL)
    {
	fprintf(log_fd, "==== start log session ====\n");
#ifdef FEAT_RELTIME
	profile_start(&log_start);
#endif
    }
}

    int
ch_log_active()
{
    return log_fd != NULL;
}

    static void
ch_log_lead(char *what, channel_T *ch)
{
    if (log_fd != NULL)
    {
#ifdef FEAT_RELTIME
	proftime_T log_now;

	profile_start(&log_now);
	profile_sub(&log_now, &log_start);
	fprintf(log_fd, "%s ", profile_msg(&log_now));
#endif
	if (ch != NULL)
	    fprintf(log_fd, "%son %d: ", what, ch->ch_id);
	else
	    fprintf(log_fd, "%s: ", what);
    }
}

    void
ch_log(channel_T *ch, char *msg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch);
	fputs(msg, log_fd);
	fputc('\n', log_fd);
	fflush(log_fd);
    }
}

    static void
ch_logn(channel_T *ch, char *msg, int nr)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch);
	fprintf(log_fd, msg, nr);
	fputc('\n', log_fd);
	fflush(log_fd);
    }
}

    void
ch_logs(channel_T *ch, char *msg, char *name)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch);
	fprintf(log_fd, msg, name);
	fputc('\n', log_fd);
	fflush(log_fd);
    }
}

    static void
ch_logsn(channel_T *ch, char *msg, char *name, int nr)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch);
	fprintf(log_fd, msg, name, nr);
	fputc('\n', log_fd);
	fflush(log_fd);
    }
}

    static void
ch_error(channel_T *ch, char *msg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("ERR ", ch);
	fputs(msg, log_fd);
	fputc('\n', log_fd);
	fflush(log_fd);
    }
}

    static void
ch_errorn(channel_T *ch, char *msg, int nr)
{
    if (log_fd != NULL)
    {
	ch_log_lead("ERR ", ch);
	fprintf(log_fd, msg, nr);
	fputc('\n', log_fd);
	fflush(log_fd);
    }
}

    static void
ch_errors(channel_T *ch, char *msg, char *arg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("ERR ", ch);
	fprintf(log_fd, msg, arg);
	fputc('\n', log_fd);
	fflush(log_fd);
    }
}

#ifdef _WIN32
# undef PERROR
# define PERROR(msg) (void)emsg3((char_u *)"%s: %s", \
	(char_u *)msg, (char_u *)strerror_win32(errno))

    static char *
strerror_win32(int eno)
{
    static LPVOID msgbuf = NULL;
    char_u *ptr;

    if (msgbuf)
	LocalFree(msgbuf);
    FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER |
	FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	eno,
	MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
	(LPTSTR) &msgbuf,
	0,
	NULL);
    /* chomp \r or \n */
    for (ptr = (char_u *)msgbuf; *ptr; ptr++)
	switch (*ptr)
	{
	    case '\r':
		STRMOVE(ptr, ptr + 1);
		ptr--;
		break;
	    case '\n':
		if (*(ptr + 1) == '\0')
		    *ptr = '\0';
		else
		    *ptr = ' ';
		break;
	}
    return msgbuf;
}
#endif

/*
 * The list of all allocated channels.
 */
static channel_T *first_channel = NULL;
static int next_ch_id = 0;

/*
 * Allocate a new channel.  The refcount is set to 1.
 * The channel isn't actually used until it is opened.
 * Returns NULL if out of memory.
 */
    channel_T *
add_channel(void)
{
    int		part;
    channel_T	*channel = (channel_T *)alloc_clear((int)sizeof(channel_T));

    if (channel == NULL)
	return NULL;

    channel->ch_id = next_ch_id++;
    ch_log(channel, "Created channel");

#ifdef CHANNEL_PIPES
    for (part = PART_SOCK; part <= PART_IN; ++part)
#else
    part = PART_SOCK;
#endif
    {
	channel->ch_part[part].ch_fd = INVALID_FD;
#ifdef FEAT_GUI_X11
	channel->ch_part[part].ch_inputHandler = (XtInputId)NULL;
#endif
#ifdef FEAT_GUI_GTK
	channel->ch_part[part].ch_inputHandler = 0;
#endif
	channel->ch_part[part].ch_timeout = 2000;
    }

    if (first_channel != NULL)
    {
	first_channel->ch_prev = channel;
	channel->ch_next = first_channel;
    }
    first_channel = channel;

    channel->ch_refcount = 1;
    return channel;
}

/*
 * Called when the refcount of a channel is zero.
 * Return TRUE if "channel" has a callback and the associated job wasn't
 * killed.
 */
    static int
channel_still_useful(channel_T *channel)
{
    int has_sock_msg;
#ifdef CHANNEL_PIPES
    int	has_out_msg;
    int	has_err_msg;
#endif

    /* If the job was killed the channel is not expected to work anymore. */
    if (channel->ch_job_killed && channel->ch_job == NULL)
	return FALSE;

    /* If there is a close callback it may still need to be invoked. */
    if (channel->ch_close_cb != NULL)
	return TRUE;

    /* If there is no callback then nobody can get readahead.  If the fd is
     * closed and there is no readahead then the callback won't be called. */
    has_sock_msg = channel->ch_part[PART_SOCK].ch_fd != INVALID_FD
	          || channel->ch_part[PART_SOCK].ch_head.rq_next != NULL
		  || channel->ch_part[PART_SOCK].ch_json_head.jq_next != NULL;
#ifdef CHANNEL_PIPES
    has_out_msg = channel->ch_part[PART_OUT].ch_fd != INVALID_FD
		  || channel->ch_part[PART_OUT].ch_head.rq_next != NULL
		  || channel->ch_part[PART_OUT].ch_json_head.jq_next != NULL;
    has_err_msg = channel->ch_part[PART_ERR].ch_fd != INVALID_FD
		  || channel->ch_part[PART_ERR].ch_head.rq_next != NULL
		  || channel->ch_part[PART_ERR].ch_json_head.jq_next != NULL;
#endif
    return (channel->ch_callback != NULL && (has_sock_msg
#ifdef CHANNEL_PIPES
		|| has_out_msg || has_err_msg
#endif
		))
#ifdef CHANNEL_PIPES
	    || (channel->ch_part[PART_OUT].ch_callback != NULL && has_out_msg)
	    || (channel->ch_part[PART_ERR].ch_callback != NULL && has_err_msg)
#endif
	    ;
}

/*
 * Close a channel and free all its resources if there is no further action
 * possible, there is no callback to be invoked or the associated job was
 * killed.
 * Return TRUE if the channel was freed.
 */
    int
channel_may_free(channel_T *channel)
{
    if (!channel_still_useful(channel))
    {
	channel_free(channel);
	return TRUE;
    }
    return FALSE;
}

/*
 * Close a channel and free all its resources.
 */
    void
channel_free(channel_T *channel)
{
    channel_close(channel, TRUE);
    channel_clear(channel);
    ch_log(channel, "Freeing channel");
    if (channel->ch_next != NULL)
	channel->ch_next->ch_prev = channel->ch_prev;
    if (channel->ch_prev == NULL)
	first_channel = channel->ch_next;
    else
	channel->ch_prev->ch_next = channel->ch_next;
    vim_free(channel);
}

#if defined(FEAT_GUI) || defined(PROTO)

#if defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK)
    static void
channel_read_fd(int fd)
{
    channel_T	*channel;
    int		part;

    channel = channel_fd2channel(fd, &part);
    if (channel == NULL)
	ch_errorn(NULL, "Channel for fd %d not found", fd);
    else
	channel_read(channel, part, "messageFromNetbeans");
}
#endif

/*
 * Read a command from netbeans.
 * TODO: instead of channel ID use the FD.
 */
#ifdef FEAT_GUI_X11
    static void
messageFromNetbeans(XtPointer clientData,
		    int *unused1 UNUSED,
		    XtInputId *unused2 UNUSED)
{
    channel_read_fd((int)(long)clientData);
}
#endif

#ifdef FEAT_GUI_GTK
# if GTK_CHECK_VERSION(3,0,0)
    static gboolean
messageFromNetbeans(GIOChannel *unused1 UNUSED,
		    GIOCondition unused2 UNUSED,
		    gpointer clientData)
{
    channel_read_fd(GPOINTER_TO_INT(clientData));
    return TRUE; /* Return FALSE instead in case the event source is to
		  * be removed after this function returns. */
}
# else
    static void
messageFromNetbeans(gpointer clientData,
		    gint unused1 UNUSED,
		    GdkInputCondition unused2 UNUSED)
{
    channel_read_fd((int)(long)clientData);
}
# endif
#endif

    static void
channel_gui_register_one(channel_T *channel, int part)
{
# ifdef FEAT_GUI_X11
    /* Tell notifier we are interested in being called
     * when there is input on the editor connection socket. */
    if (channel->ch_part[part].ch_inputHandler == (XtInputId)NULL)
	channel->ch_part[part].ch_inputHandler = XtAppAddInput(
		(XtAppContext)app_context,
		channel->ch_part[part].ch_fd,
		(XtPointer)(XtInputReadMask + XtInputExceptMask),
		messageFromNetbeans,
		(XtPointer)(long)channel->ch_part[part].ch_fd);
# else
#  ifdef FEAT_GUI_GTK
    /* Tell gdk we are interested in being called when there
     * is input on the editor connection socket. */
    if (channel->ch_part[part].ch_inputHandler == 0)
#   if GTK_CHECK_VERSION(3,0,0)
    {
	GIOChannel *chnnl = g_io_channel_unix_new(
		(gint)channel->ch_part[part].ch_fd);

	channel->ch_part[part].ch_inputHandler = g_io_add_watch(
		chnnl,
		G_IO_IN|G_IO_HUP|G_IO_ERR|G_IO_PRI,
		messageFromNetbeans,
		GINT_TO_POINTER(channel->ch_part[part].ch_fd));

	g_io_channel_unref(chnnl);
    }
#   else
	channel->ch_part[part].ch_inputHandler = gdk_input_add(
		(gint)channel->ch_part[part].ch_fd,
		(GdkInputCondition)
			     ((int)GDK_INPUT_READ + (int)GDK_INPUT_EXCEPTION),
		messageFromNetbeans,
		(gpointer)(long)channel->ch_part[part].ch_fd);
#   endif
#  endif
# endif
}

    void
channel_gui_register(channel_T *channel)
{
    if (!CH_HAS_GUI)
	return;

    if (channel->CH_SOCK_FD != INVALID_FD)
	channel_gui_register_one(channel, PART_SOCK);
# ifdef CHANNEL_PIPES
    if (channel->CH_OUT_FD != INVALID_FD)
	channel_gui_register_one(channel, PART_OUT);
    if (channel->CH_ERR_FD != INVALID_FD)
	channel_gui_register_one(channel, PART_ERR);
# endif
}

/*
 * Register any of our file descriptors with the GUI event handling system.
 * Called when the GUI has started.
 */
    void
channel_gui_register_all(void)
{
    channel_T *channel;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
	channel_gui_register(channel);
}

    static void
channel_gui_unregister(channel_T *channel)
{
    int	    part;

#ifdef CHANNEL_PIPES
    for (part = PART_SOCK; part < PART_IN; ++part)
#else
    part = PART_SOCK;
#endif
    {
# ifdef FEAT_GUI_X11
	if (channel->ch_part[part].ch_inputHandler != (XtInputId)NULL)
	{
	    XtRemoveInput(channel->ch_part[part].ch_inputHandler);
	    channel->ch_part[part].ch_inputHandler = (XtInputId)NULL;
	}
# else
#  ifdef FEAT_GUI_GTK
	if (channel->ch_part[part].ch_inputHandler != 0)
	{
#   if GTK_CHECK_VERSION(3,0,0)
	    g_source_remove(channel->ch_part[part].ch_inputHandler);
#   else
	    gdk_input_remove(channel->ch_part[part].ch_inputHandler);
#   endif
	    channel->ch_part[part].ch_inputHandler = 0;
	}
#  endif
# endif
    }
}

#endif

static char *e_cannot_connect = N_("E902: Cannot connect to port");

/*
 * Open a socket channel to "hostname":"port".
 * "waittime" is the time in msec to wait for the connection.
 * When negative wait forever.
 * Returns the channel for success.
 * Returns NULL for failure.
 */
    channel_T *
channel_open(
	char *hostname,
	int port_in,
	int waittime,
	void (*nb_close_cb)(void))
{
    int			sd = -1;
    struct sockaddr_in	server;
    struct hostent	*host;
#ifdef WIN32
    u_short		port = port_in;
    u_long		val = 1;
#else
    int			port = port_in;
    struct timeval	start_tv;
#endif
    channel_T		*channel;
    int			ret;

#ifdef WIN32
    channel_init_winsock();
#endif

    channel = add_channel();
    if (channel == NULL)
    {
	ch_error(NULL, "Cannot allocate channel.");
	return NULL;
    }

    /* Get the server internet address and put into addr structure */
    /* fill in the socket address structure and connect to server */
    vim_memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((host = gethostbyname(hostname)) == NULL)
    {
	ch_error(channel, "in gethostbyname() in channel_open()");
	PERROR("E901: gethostbyname() in channel_open()");
	channel_free(channel);
	return NULL;
    }
    memcpy((char *)&server.sin_addr, host->h_addr, host->h_length);

    /* On Mac and Solaris a zero timeout almost never works.  At least wait
     * one millisecond. Let's do it for all systems, because we don't know why
     * this is needed. */
    if (waittime == 0)
	waittime = 1;

    /*
     * For Unix we need to call connect() again after connect() failed.
     * On Win32 one time is sufficient.
     */
    while (TRUE)
    {
	if (sd >= 0)
	    sock_close(sd);
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
	{
	    ch_error(channel, "in socket() in channel_open().");
	    PERROR("E898: socket() in channel_open()");
	    channel_free(channel);
	    return NULL;
	}

	if (waittime >= 0)
	{
	    /* Make connect() non-blocking. */
	    if (
#ifdef _WIN32
		ioctlsocket(sd, FIONBIO, &val) < 0
#else
		fcntl(sd, F_SETFL, O_NONBLOCK) < 0
#endif
	       )
	    {
		SOCK_ERRNO;
		ch_errorn(channel,
			 "channel_open: Connect failed with errno %d", errno);
		sock_close(sd);
		channel_free(channel);
		return NULL;
	    }
	}

	/* Try connecting to the server. */
	ch_logsn(channel, "Connecting to %s port %d", hostname, port);
	ret = connect(sd, (struct sockaddr *)&server, sizeof(server));

	SOCK_ERRNO;
	if (ret < 0)
	{
	    if (errno != EWOULDBLOCK
		    && errno != ECONNREFUSED
#ifdef EINPROGRESS
		    && errno != EINPROGRESS
#endif
		    )
	    {
		ch_errorn(channel,
			"channel_open: Connect failed with errno %d", errno);
		PERROR(_(e_cannot_connect));
		sock_close(sd);
		channel_free(channel);
		return NULL;
	    }
	}

	/* If we don't block and connect() failed then try using select() to
	 * wait for the connection to be made. */
	if (waittime >= 0 && ret < 0)
	{
	    struct timeval	tv;
	    fd_set		rfds;
	    fd_set		wfds;
	    int			so_error = 0;
	    socklen_t		so_error_len = sizeof(so_error);

	    FD_ZERO(&rfds);
	    FD_SET(sd, &rfds);
	    FD_ZERO(&wfds);
	    FD_SET(sd, &wfds);

	    tv.tv_sec = waittime / 1000;
	    tv.tv_usec = (waittime % 1000) * 1000;
#ifndef WIN32
	    gettimeofday(&start_tv, NULL);
#endif
	    ch_logn(channel,
		    "Waiting for connection (waittime %d msec)...", waittime);
	    ret = select((int)sd + 1, &rfds, &wfds, NULL, &tv);

	    if (ret < 0)
	    {
		SOCK_ERRNO;
		ch_errorn(channel,
			"channel_open: Connect failed with errno %d", errno);
		PERROR(_(e_cannot_connect));
		sock_close(sd);
		channel_free(channel);
		return NULL;
	    }

	    /* On Win32: select() is expected to work and wait for up to the
	     * waittime for the socket to be open.
	     * On Linux-like systems: See socket(7) for the behavior
	     * After putting the socket in non-blocking mode, connect() will
	     * return EINPROGRESS, select() will not wait (as if writing is
	     * possible), need to use getsockopt() to check if the socket is
	     * actually connect.
	     * We detect an failure to connect when both read and write fds
	     * are set.  Use getsockopt() to find out what kind of failure. */
	    if (FD_ISSET(sd, &rfds) && FD_ISSET(sd, &wfds))
	    {
		ret = getsockopt(sd,
			    SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
		if (ret < 0 || (so_error != 0
			&& so_error != EWOULDBLOCK
			&& so_error != ECONNREFUSED
#ifdef EINPROGRESS
			&& so_error != EINPROGRESS
#endif
			))
		{
		    ch_errorn(channel,
			    "channel_open: Connect failed with errno %d",
			    so_error);
		    PERROR(_(e_cannot_connect));
		    sock_close(sd);
		    channel_free(channel);
		    return NULL;
		}
	    }

	    if (!FD_ISSET(sd, &wfds) || so_error != 0)
	    {
#ifndef WIN32
		struct  timeval end_tv;
		long    elapsed_msec;

		gettimeofday(&end_tv, NULL);
		elapsed_msec = (end_tv.tv_sec - start_tv.tv_sec) * 1000
				 + (end_tv.tv_usec - start_tv.tv_usec) / 1000;
		if (waittime > 1 && elapsed_msec < waittime)
		{
		    /* The port isn't ready but we also didn't get an error.
		     * This happens when the server didn't open the socket
		     * yet.  Wait a bit and try again. */
		    mch_delay(waittime < 50 ? (long)waittime : 50L, TRUE);
		    ui_breakcheck();
		    if (!got_int)
		    {
			/* reduce the waittime by the elapsed time and the 50
			 * msec delay (or a bit more) */
			waittime -= elapsed_msec;
			if (waittime > 50)
			    waittime -= 50;
			else
			    waittime = 1;
			continue;
		    }
		    /* we were interrupted, behave as if timed out */
		}
#endif
		/* We timed out. */
		ch_error(channel, "Connection timed out");
		sock_close(sd);
		channel_free(channel);
		return NULL;
	    }

	    ch_log(channel, "Connection made");
	    break;
	}
    }

    if (waittime >= 0)
    {
#ifdef _WIN32
	val = 0;
	ioctlsocket(sd, FIONBIO, &val);
#else
	(void)fcntl(sd, F_SETFL, 0);
#endif
    }

    channel->CH_SOCK_FD = (sock_T)sd;
    channel->ch_nb_close_cb = nb_close_cb;

#ifdef FEAT_GUI
    channel_gui_register(channel);
#endif

    return channel;
}

#if defined(CHANNEL_PIPES) || defined(PROTO)
    void
channel_set_pipes(channel_T *channel, sock_T in, sock_T out, sock_T err)
{
    channel->CH_IN_FD = in;
    channel->CH_OUT_FD = out;
    channel->CH_ERR_FD = err;
}
#endif

/*
 * Sets the job the channel is associated with.
 * This does not keep a refcount, when the job is freed ch_job is cleared.
 */
    void
channel_set_job(channel_T *channel, job_T *job)
{
    channel->ch_job = job;
}

/*
 * Find a buffer matching "name" or create a new one.
 */
    static buf_T *
find_buffer(char_u *name)
{
    buf_T *buf = NULL;
    buf_T *save_curbuf = curbuf;

    if (name != NULL && *name != NUL)
	buf = buflist_findname(name);
    if (buf == NULL)
    {
	buf = buflist_new(name == NULL || *name == NUL ? NULL : name,
					       NULL, (linenr_T)0, BLN_LISTED);
	buf_copy_options(buf, BCO_ENTER);
#ifdef FEAT_QUICKFIX
	clear_string_option(&buf->b_p_bt);
	buf->b_p_bt = vim_strsave((char_u *)"nofile");
	clear_string_option(&buf->b_p_bh);
	buf->b_p_bh = vim_strsave((char_u *)"hide");
#endif
	curbuf = buf;
	ml_open(curbuf);
	ml_replace(1, (char_u *)"Reading from channel output...", TRUE);
	changed_bytes(1, 0);
	curbuf = save_curbuf;
    }

    return buf;
}

/*
 * Set various properties from an "opt" argument.
 */
    void
channel_set_options(channel_T *channel, jobopt_T *opt)
{
    int    part;
    char_u **cbp;

    if (opt->jo_set & JO_MODE)
	for (part = PART_SOCK; part <= PART_IN; ++part)
	    channel->ch_part[part].ch_mode = opt->jo_mode;
    if (opt->jo_set & JO_IN_MODE)
	channel->ch_part[PART_IN].ch_mode = opt->jo_in_mode;
    if (opt->jo_set & JO_OUT_MODE)
	channel->ch_part[PART_OUT].ch_mode = opt->jo_out_mode;
    if (opt->jo_set & JO_ERR_MODE)
	channel->ch_part[PART_ERR].ch_mode = opt->jo_err_mode;

    if (opt->jo_set & JO_TIMEOUT)
	for (part = PART_SOCK; part <= PART_IN; ++part)
	    channel->ch_part[part].ch_timeout = opt->jo_timeout;
    if (opt->jo_set & JO_OUT_TIMEOUT)
	channel->ch_part[PART_OUT].ch_timeout = opt->jo_out_timeout;
    if (opt->jo_set & JO_ERR_TIMEOUT)
	channel->ch_part[PART_ERR].ch_timeout = opt->jo_err_timeout;

    if (opt->jo_set & JO_CALLBACK)
    {
	cbp = &channel->ch_callback;
	vim_free(*cbp);
	if (opt->jo_callback != NULL && *opt->jo_callback != NUL)
	    *cbp = vim_strsave(opt->jo_callback);
	else
	    *cbp = NULL;
    }
    if (opt->jo_set & JO_OUT_CALLBACK)
    {
	cbp = &channel->ch_part[PART_OUT].ch_callback;
	vim_free(*cbp);
	if (opt->jo_out_cb != NULL && *opt->jo_out_cb != NUL)
	    *cbp = vim_strsave(opt->jo_out_cb);
	else
	    *cbp = NULL;
    }
    if (opt->jo_set & JO_ERR_CALLBACK)
    {
	cbp = &channel->ch_part[PART_ERR].ch_callback;
	vim_free(*cbp);
	if (opt->jo_err_cb != NULL && *opt->jo_err_cb != NUL)
	    *cbp = vim_strsave(opt->jo_err_cb);
	else
	    *cbp = NULL;
    }
    if (opt->jo_set & JO_CLOSE_CALLBACK)
    {
	cbp = &channel->ch_close_cb;
	vim_free(*cbp);
	if (opt->jo_close_cb != NULL && *opt->jo_close_cb != NUL)
	    *cbp = vim_strsave(opt->jo_close_cb);
	else
	    *cbp = NULL;
    }

    if ((opt->jo_set & JO_OUT_IO) && opt->jo_io[PART_OUT] == JIO_BUFFER)
    {
	/* writing output to a buffer. Force mode to NL. */
	channel->ch_part[PART_OUT].ch_mode = MODE_NL;
	channel->ch_part[PART_OUT].ch_buffer =
				       find_buffer(opt->jo_io_name[PART_OUT]);
	ch_logs(channel, "writing to buffer '%s'",
		      (char *)channel->ch_part[PART_OUT].ch_buffer->b_ffname);
    }
}

/*
 * Set the callback for "channel"/"part" for the response with "id".
 */
    void
channel_set_req_callback(
	channel_T *channel,
	int part,
	char_u *callback,
	int id)
{
    cbq_T *head = &channel->ch_part[part].ch_cb_head;
    cbq_T *item = (cbq_T *)alloc((int)sizeof(cbq_T));

    if (item != NULL)
    {
	item->cq_callback = vim_strsave(callback);
	item->cq_seq_nr = id;
	item->cq_prev = head->cq_prev;
	head->cq_prev = item;
	item->cq_next = NULL;
	if (item->cq_prev == NULL)
	    head->cq_next = item;
	else
	    item->cq_prev->cq_next = item;
    }
}

/*
 * Invoke the "callback" on channel "channel".
 */
    static void
invoke_callback(channel_T *channel, char_u *callback, typval_T *argv)
{
    typval_T	rettv;
    int		dummy;

    argv[0].v_type = VAR_CHANNEL;
    argv[0].vval.v_channel = channel;

    call_func(callback, (int)STRLEN(callback),
			     &rettv, 2, argv, 0L, 0L, &dummy, TRUE, NULL);
    clear_tv(&rettv);

    /* If an echo command was used the cursor needs to be put back where
     * it belongs. If highlighting was changed a redraw is needed. */
    update_screen(0);
    setcursor();
    cursor_on();
    out_flush();
#ifdef FEAT_GUI
    gui_update_cursor(TRUE, FALSE);
    gui_mch_flush();
#endif
}

/*
 * Return the first buffer from channel "channel"/"part" and remove it.
 * The caller must free it.
 * Returns NULL if there is nothing.
 */
    char_u *
channel_get(channel_T *channel, int part)
{
    readq_T *head = &channel->ch_part[part].ch_head;
    readq_T *node = head->rq_next;
    char_u *p;

    if (node == NULL)
	return NULL;
    /* dispose of the node but keep the buffer */
    p = node->rq_buffer;
    head->rq_next = node->rq_next;
    if (node->rq_next == NULL)
	head->rq_prev = NULL;
    else
	node->rq_next->rq_prev = NULL;
    vim_free(node);
    return p;
}

/*
 * Returns the whole buffer contents concatenated for "channel"/"part".
 */
    static char_u *
channel_get_all(channel_T *channel, int part)
{
    /* Concatenate everything into one buffer.
     * TODO: avoid multiple allocations. */
    while (channel_collapse(channel, part) == OK)
	;
    return channel_get(channel, part);
}

/*
 * Collapses the first and second buffer for "channel"/"part".
 * Returns FAIL if that is not possible.
 */
    int
channel_collapse(channel_T *channel, int part)
{
    readq_T *head = &channel->ch_part[part].ch_head;
    readq_T *node = head->rq_next;
    char_u  *p;

    if (node == NULL || node->rq_next == NULL)
	return FAIL;

    p = alloc((unsigned)(STRLEN(node->rq_buffer)
				     + STRLEN(node->rq_next->rq_buffer) + 1));
    if (p == NULL)
	return FAIL;	    /* out of memory */
    STRCPY(p, node->rq_buffer);
    STRCAT(p, node->rq_next->rq_buffer);
    vim_free(node->rq_next->rq_buffer);
    node->rq_next->rq_buffer = p;

    /* dispose of the node and its buffer */
    head->rq_next = node->rq_next;
    head->rq_next->rq_prev = NULL;
    vim_free(node->rq_buffer);
    vim_free(node);
    return OK;
}

/*
 * Store "buf[len]" on "channel"/"part".
 * Returns OK or FAIL.
 */
    static int
channel_save(channel_T *channel, int part, char_u *buf, int len)
{
    readq_T *node;
    readq_T *head = &channel->ch_part[part].ch_head;
    char_u  *p;
    int	    i;

    node = (readq_T *)alloc(sizeof(readq_T));
    if (node == NULL)
	return FAIL;	    /* out of memory */
    node->rq_buffer = alloc(len + 1);
    if (node->rq_buffer == NULL)
    {
	vim_free(node);
	return FAIL;	    /* out of memory */
    }

    if (channel->ch_part[part].ch_mode == MODE_NL)
    {
	/* Drop any CR before a NL. */
	p = node->rq_buffer;
	for (i = 0; i < len; ++i)
	    if (buf[i] != CAR || i + 1 >= len || buf[i + 1] != NL)
		*p++ = buf[i];
	*p = NUL;
    }
    else
    {
	mch_memmove(node->rq_buffer, buf, len);
	node->rq_buffer[len] = NUL;
    }

    /* append node to the tail of the queue */
    node->rq_next = NULL;
    node->rq_prev = head->rq_prev;
    if (head->rq_prev == NULL)
	head->rq_next = node;
    else
	head->rq_prev->rq_next = node;
    head->rq_prev = node;

    if (log_fd != NULL)
    {
	ch_log_lead("RECV ", channel);
	fprintf(log_fd, "'");
	if (fwrite(buf, len, 1, log_fd) != 1)
	    return FAIL;
	fprintf(log_fd, "'\n");
    }
    return OK;
}

/*
 * Use the read buffer of "channel"/"part" and parse a JSON messages that is
 * complete.  The messages are added to the queue.
 * Return TRUE if there is more to read.
 */
    static int
channel_parse_json(channel_T *channel, int part)
{
    js_read_T	reader;
    typval_T	listtv;
    jsonq_T	*item;
    jsonq_T	*head = &channel->ch_part[part].ch_json_head;
    int		ret;

    if (channel_peek(channel, part) == NULL)
	return FALSE;

    /* TODO: make reader work properly */
    /* reader.js_buf = channel_peek(channel, part); */
    reader.js_buf = channel_get_all(channel, part);
    reader.js_used = 0;
    reader.js_fill = NULL;
    /* reader.js_fill = channel_fill; */
    reader.js_cookie = channel;
    ret = json_decode(&reader, &listtv,
		     channel->ch_part[part].ch_mode == MODE_JS ? JSON_JS : 0);
    if (ret == OK)
    {
	/* Only accept the response when it is a list with at least two
	 * items. */
	if (listtv.v_type != VAR_LIST || listtv.vval.v_list->lv_len < 2)
	{
	    /* TODO: give error */
	    clear_tv(&listtv);
	}
	else
	{
	    item = (jsonq_T *)alloc((unsigned)sizeof(jsonq_T));
	    if (item == NULL)
		clear_tv(&listtv);
	    else
	    {
		item->jq_value = alloc_tv();
		if (item->jq_value == NULL)
		{
		    vim_free(item);
		    clear_tv(&listtv);
		}
		else
		{
		    *item->jq_value = listtv;
		    item->jq_prev = head->jq_prev;
		    head->jq_prev = item;
		    item->jq_next = NULL;
		    if (item->jq_prev == NULL)
			head->jq_next = item;
		    else
			item->jq_prev->jq_next = item;
		}
	    }
	}
    }

    /* Put the unread part back into the channel.
     * TODO: insert in front */
    if (reader.js_buf[reader.js_used] != NUL)
    {
	channel_save(channel, part, reader.js_buf + reader.js_used,
		(int)(reader.js_end - reader.js_buf) - reader.js_used);
	ret = TRUE;
    }
    else
	ret = FALSE;

    vim_free(reader.js_buf);
    return ret;
}

/*
 * Remove "node" from the queue that it is in.  Does not free it.
 */
    static void
remove_cb_node(cbq_T *head, cbq_T *node)
{
    if (node->cq_prev == NULL)
	head->cq_next = node->cq_next;
    else
	node->cq_prev->cq_next = node->cq_next;
    if (node->cq_next == NULL)
	head->cq_prev = node->cq_prev;
    else
	node->cq_next->cq_prev = node->cq_prev;
}

/*
 * Remove "node" from the queue that it is in and free it.
 * Caller should have freed or used node->jq_value.
 */
    static void
remove_json_node(jsonq_T *head, jsonq_T *node)
{
    if (node->jq_prev == NULL)
	head->jq_next = node->jq_next;
    else
	node->jq_prev->jq_next = node->jq_next;
    if (node->jq_next == NULL)
	head->jq_prev = node->jq_prev;
    else
	node->jq_next->jq_prev = node->jq_prev;
    vim_free(node);
}

/*
 * Get a message from the JSON queue for channel "channel".
 * When "id" is positive it must match the first number in the list.
 * When "id" is zero or negative jut get the first message.  But not the one
 * with id ch_block_id.
 * Return OK when found and return the value in "rettv".
 * Return FAIL otherwise.
 */
    static int
channel_get_json(channel_T *channel, int part, int id, typval_T **rettv)
{
    jsonq_T   *head = &channel->ch_part[part].ch_json_head;
    jsonq_T   *item = head->jq_next;

    while (item != NULL)
    {
	list_T	    *l = item->jq_value->vval.v_list;
	typval_T    *tv = &l->lv_first->li_tv;

	if ((id > 0 && tv->v_type == VAR_NUMBER && tv->vval.v_number == id)
	      || (id <= 0 && (tv->v_type != VAR_NUMBER
		 || tv->vval.v_number == 0
		 || tv->vval.v_number != channel->ch_part[part].ch_block_id)))
	{
	    *rettv = item->jq_value;
	    remove_json_node(head, item);
	    return OK;
	}
	item = item->jq_next;
    }
    return FAIL;
}

#define CH_JSON_MAX_ARGS 4

/*
 * Execute a command received over "channel"/"part"
 * "argv[0]" is the command string.
 * "argv[1]" etc. have further arguments, type is VAR_UNKNOWN if missing.
 */
    static void
channel_exe_cmd(channel_T *channel, int part, typval_T *argv)
{
    char_u  *cmd = argv[0].vval.v_string;
    char_u  *arg;
    int	    options = channel->ch_part[part].ch_mode == MODE_JS ? JSON_JS : 0;

    if (argv[1].v_type != VAR_STRING)
    {
	ch_error(channel, "received command with non-string argument");
	if (p_verbose > 2)
	    EMSG("E903: received command with non-string argument");
	return;
    }
    arg = argv[1].vval.v_string;
    if (arg == NULL)
	arg = (char_u *)"";

    if (STRCMP(cmd, "ex") == 0)
    {
	do_cmdline_cmd(arg);
    }
    else if (STRCMP(cmd, "normal") == 0)
    {
	exarg_T ea;

	ea.arg = arg;
	ea.addr_count = 0;
	ea.forceit = TRUE; /* no mapping */
	ex_normal(&ea);
    }
    else if (STRCMP(cmd, "redraw") == 0)
    {
	exarg_T ea;

	ea.forceit = *arg != NUL;
	ex_redraw(&ea);
	showruler(FALSE);
	setcursor();
	out_flush();
#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    gui_update_cursor(FALSE, FALSE);
	    gui_mch_flush();
	}
#endif
    }
    else if (STRCMP(cmd, "expr") == 0 || STRCMP(cmd, "call") == 0)
    {
	int is_call = cmd[0] == 'c';
	int id_idx = is_call ? 3 : 2;

	if (argv[id_idx].v_type != VAR_UNKNOWN
					 && argv[id_idx].v_type != VAR_NUMBER)
	{
	    ch_error(channel, "last argument for expr/call must be a number");
	    if (p_verbose > 2)
		EMSG("E904: last argument for expr/call must be a number");
	}
	else if (is_call && argv[2].v_type != VAR_LIST)
	{
	    ch_error(channel, "third argument for call must be a list");
	    if (p_verbose > 2)
		EMSG("E904: third argument for call must be a list");
	}
	else
	{
	    typval_T	*tv;
	    typval_T	res_tv;
	    typval_T	err_tv;
	    char_u	*json = NULL;

	    /* Don't pollute the display with errors. */
	    ++emsg_skip;
	    if (!is_call)
		tv = eval_expr(arg, NULL);
	    else if (func_call(arg, &argv[2], NULL, &res_tv) == OK)
		tv = &res_tv;
	    else
		tv = NULL;

	    if (argv[id_idx].v_type == VAR_NUMBER)
	    {
		int id = argv[id_idx].vval.v_number;

		if (tv != NULL)
		    json = json_encode_nr_expr(id, tv, options);
		if (tv == NULL || (json != NULL && *json == NUL))
		{
		    /* If evaluation failed or the result can't be encoded
		     * then return the string "ERROR". */
		    vim_free(json);
		    free_tv(tv);
		    err_tv.v_type = VAR_STRING;
		    err_tv.vval.v_string = (char_u *)"ERROR";
		    tv = &err_tv;
		    json = json_encode_nr_expr(id, tv, options);
		}
		if (json != NULL)
		{
		    channel_send(channel,
				 part == PART_SOCK ? PART_SOCK : PART_IN,
				 json, (char *)cmd);
		    vim_free(json);
		}
	    }
	    --emsg_skip;
	    if (tv == &res_tv)
		clear_tv(tv);
	    else if (tv != &err_tv)
		free_tv(tv);
	}
    }
    else if (p_verbose > 2)
    {
	ch_errors(channel, "Receved unknown command: %s", (char *)cmd);
	EMSG2("E905: received unknown command: %s", cmd);
    }
}

/*
 * Invoke a callback for "channel"/"part" if needed.
 * Return TRUE when a message was handled, there might be another one.
 */
    static int
may_invoke_callback(channel_T *channel, int part)
{
    char_u	*msg = NULL;
    typval_T	*listtv = NULL;
    typval_T	argv[CH_JSON_MAX_ARGS];
    int		seq_nr = -1;
    ch_mode_T	ch_mode = channel->ch_part[part].ch_mode;
    char_u	*callback = NULL;
    buf_T	*buffer = NULL;

    if (channel->ch_nb_close_cb != NULL)
	/* this channel is handled elsewhere (netbeans) */
	return FALSE;

    if (channel->ch_part[part].ch_callback != NULL)
	callback = channel->ch_part[part].ch_callback;
    else
	callback = channel->ch_callback;

    buffer = channel->ch_part[part].ch_buffer;
    if (buffer != NULL && !buf_valid(buffer))
    {
	/* buffer was wiped out */
	channel->ch_part[part].ch_buffer = NULL;
	buffer = NULL;
    }

    if (ch_mode == MODE_JSON || ch_mode == MODE_JS)
    {
	listitem_T	*item;
	int		argc = 0;

	/* Get any json message in the queue. */
	if (channel_get_json(channel, part, -1, &listtv) == FAIL)
	{
	    /* Parse readahead, return when there is still no message. */
	    channel_parse_json(channel, part);
	    if (channel_get_json(channel, part, -1, &listtv) == FAIL)
		return FALSE;
	}

	for (item = listtv->vval.v_list->lv_first;
			    item != NULL && argc < CH_JSON_MAX_ARGS;
						    item = item->li_next)
	    argv[argc++] = item->li_tv;
	while (argc < CH_JSON_MAX_ARGS)
	    argv[argc++].v_type = VAR_UNKNOWN;

	if (argv[0].v_type == VAR_STRING)
	{
	    char_u	*cmd = argv[0].vval.v_string;

	    /* ["cmd", arg] or ["cmd", arg, arg] or ["cmd", arg, arg, arg] */
	    ch_logs(channel, "Executing %s command", (char *)cmd);
	    channel_exe_cmd(channel, part, argv);
	    free_tv(listtv);
	    return TRUE;
	}

	if (argv[0].v_type != VAR_NUMBER)
	{
	    ch_error(channel,
		      "Dropping message with invalid sequence number type");
	    free_tv(listtv);
	    return FALSE;
	}
	seq_nr = argv[0].vval.v_number;
    }
    else if (channel_peek(channel, part) == NULL)
    {
	/* nothing to read on RAW or NL channel */
	return FALSE;
    }
    else
    {
	/* If there is no callback or buffer drop the message. */
	if (callback == NULL && buffer == NULL)
	{
	    while ((msg = channel_get(channel, part)) != NULL)
	    {
		ch_logs(channel, "Dropping message '%s'", (char *)msg);
		vim_free(msg);
	    }
	    return FALSE;
	}

	if (ch_mode == MODE_NL)
	{
	    char_u  *nl;
	    char_u  *buf;

	    /* See if we have a message ending in NL in the first buffer.  If
	     * not try to concatenate the first and the second buffer. */
	    while (TRUE)
	    {
		buf = channel_peek(channel, part);
		nl = vim_strchr(buf, NL);
		if (nl != NULL)
		    break;
		if (channel_collapse(channel, part) == FAIL)
		    return FALSE; /* incomplete message */
	    }
	    if (nl[1] == NUL)
	    {
		/* get the whole buffer, drop the NL */
		msg = channel_get(channel, part);
		*nl = NUL;
	    }
	    else
	    {
		/* Copy the message into allocated memory and remove it from
		 * the buffer. */
		msg = vim_strnsave(buf, (int)(nl - buf));
		mch_memmove(buf, nl + 1, STRLEN(nl + 1) + 1);
	    }
	}
	else
	    /* For a raw channel we don't know where the message ends, just
	     * get everything we have. */
	    msg = channel_get_all(channel, part);

	argv[1].v_type = VAR_STRING;
	argv[1].vval.v_string = msg;
    }

    if (seq_nr > 0)
    {
	cbq_T	*head = &channel->ch_part[part].ch_cb_head;
	cbq_T	*item = head->cq_next;
	int	done = FALSE;

	/* invoke the one-time callback with the matching nr */
	while (item != NULL)
	{
	    if (item->cq_seq_nr == seq_nr)
	    {
		ch_logs(channel, "Invoking one-time callback %s",
						   (char *)item->cq_callback);
		/* Remove the item from the list first, if the callback
		 * invokes ch_close() the list will be cleared. */
		remove_cb_node(head, item);
		invoke_callback(channel, item->cq_callback, argv);
		vim_free(item->cq_callback);
		vim_free(item);
		done = TRUE;
		break;
	    }
	    item = item->cq_next;
	}
	if (!done)
	    ch_logn(channel, "Dropping message %d without callback", seq_nr);
    }
    else if (callback != NULL || buffer != NULL)
    {
	if (buffer != NULL)
	{
	    buf_T	*save_curbuf = curbuf;
	    linenr_T	lnum = buffer->b_ml.ml_line_count;

	    /* Append to the buffer */
	    ch_logn(channel, "appending line %d to buffer", (int)lnum + 1);

	    curbuf = buffer;
	    u_sync(TRUE);
	    u_save(lnum, lnum + 1);

	    ml_append(lnum, msg, 0, FALSE);
	    appended_lines_mark(lnum, 1L);
	    curbuf = save_curbuf;

	    if (buffer->b_nwindows > 0)
	    {
		win_T	*wp;
		win_T	*save_curwin;

		FOR_ALL_WINDOWS(wp)
		{
		    if (wp->w_buffer == buffer
			    && wp->w_cursor.lnum == lnum
			    && wp->w_cursor.col == 0)
		    {
			++wp->w_cursor.lnum;
			save_curwin = curwin;
			curwin = wp;
			curbuf = curwin->w_buffer;
			scroll_cursor_bot(0, FALSE);
			curwin = save_curwin;
			curbuf = curwin->w_buffer;
		    }
		}
		redraw_buf_later(buffer, VALID);
		channel_need_redraw = TRUE;
	    }
	}
	if (callback != NULL)
	{
	    /* invoke the channel callback */
	    ch_logs(channel, "Invoking channel callback %s", (char *)callback);
	    invoke_callback(channel, callback, argv);
	}
    }
    else if (msg != NULL)
	ch_logs(channel, "Dropping message '%s'", (char *)msg);
    else
	ch_log(channel, "Dropping message");

    if (listtv != NULL)
	free_tv(listtv);
    vim_free(msg);

    return TRUE;
}

/*
 * Return TRUE when channel "channel" is open for writing to.
 * Also returns FALSE or invalid "channel".
 */
    int
channel_can_write_to(channel_T *channel)
{
    return channel != NULL && (channel->CH_SOCK_FD != INVALID_FD
#ifdef CHANNEL_PIPES
			  || channel->CH_IN_FD != INVALID_FD
#endif
			  );
}

/*
 * Return TRUE when channel "channel" is open for reading or writing.
 * Also returns FALSE for invalid "channel".
 */
    int
channel_is_open(channel_T *channel)
{
    return channel != NULL && (channel->CH_SOCK_FD != INVALID_FD
#ifdef CHANNEL_PIPES
			  || channel->CH_IN_FD != INVALID_FD
			  || channel->CH_OUT_FD != INVALID_FD
			  || channel->CH_ERR_FD != INVALID_FD
#endif
			  );
}

/*
 * Return a string indicating the status of the channel.
 */
    char *
channel_status(channel_T *channel)
{
    if (channel == NULL)
	 return "fail";
    if (channel_is_open(channel))
	 return "open";
    return "closed";
}

/*
 * Close channel "channel".
 * Trigger the close callback if "invoke_close_cb" is TRUE.
 * Does not clear the buffers.
 */
    void
channel_close(channel_T *channel, int invoke_close_cb)
{
    ch_log(channel, "Closing channel");

#ifdef FEAT_GUI
    channel_gui_unregister(channel);
#endif

    if (channel->CH_SOCK_FD != INVALID_FD)
    {
	sock_close(channel->CH_SOCK_FD);
	channel->CH_SOCK_FD = INVALID_FD;
    }
#if defined(CHANNEL_PIPES)
    if (channel->CH_IN_FD != INVALID_FD)
    {
	fd_close(channel->CH_IN_FD);
	channel->CH_IN_FD = INVALID_FD;
    }
    if (channel->CH_OUT_FD != INVALID_FD)
    {
	fd_close(channel->CH_OUT_FD);
	channel->CH_OUT_FD = INVALID_FD;
    }
    if (channel->CH_ERR_FD != INVALID_FD)
    {
	fd_close(channel->CH_ERR_FD);
	channel->CH_ERR_FD = INVALID_FD;
    }
#endif

    if (invoke_close_cb && channel->ch_close_cb != NULL)
    {
	  typval_T	argv[1];
	  typval_T	rettv;
	  int		dummy;

	  /* invoke the close callback; increment the refcount to avoid it
	   * being freed halfway */
	  ch_logs(channel, "Invoking close callback %s",
						(char *)channel->ch_close_cb);
	  argv[0].v_type = VAR_CHANNEL;
	  argv[0].vval.v_channel = channel;
	  ++channel->ch_refcount;
	  call_func(channel->ch_close_cb, (int)STRLEN(channel->ch_close_cb),
				 &rettv, 1, argv, 0L, 0L, &dummy, TRUE, NULL);
	  clear_tv(&rettv);
	  --channel->ch_refcount;

	  /* the callback is only called once */
	  vim_free(channel->ch_close_cb);
	  channel->ch_close_cb = NULL;
    }

    channel->ch_nb_close_cb = NULL;
}

/*
 * Return the first buffer from "channel"/"part" without removing it.
 * Returns NULL if there is nothing.
 */
    char_u *
channel_peek(channel_T *channel, int part)
{
    readq_T *head = &channel->ch_part[part].ch_head;

    if (head->rq_next == NULL)
	return NULL;
    return head->rq_next->rq_buffer;
}

/*
 * Clear the read buffer on "channel"/"part".
 */
    static void
channel_clear_one(channel_T *channel, int part)
{
    jsonq_T *json_head = &channel->ch_part[part].ch_json_head;
    cbq_T   *cb_head = &channel->ch_part[part].ch_cb_head;

    while (channel_peek(channel, part) != NULL)
	vim_free(channel_get(channel, part));

    while (cb_head->cq_next != NULL)
    {
	cbq_T *node = cb_head->cq_next;

	remove_cb_node(cb_head, node);
	vim_free(node->cq_callback);
	vim_free(node);
    }

    while (json_head->jq_next != NULL)
    {
	free_tv(json_head->jq_next->jq_value);
	remove_json_node(json_head, json_head->jq_next);
    }

    vim_free(channel->ch_part[part].ch_callback);
    channel->ch_part[part].ch_callback = NULL;
}

/*
 * Clear all the read buffers on "channel".
 */
    void
channel_clear(channel_T *channel)
{
    ch_log(channel, "Clearing channel");
    channel_clear_one(channel, PART_SOCK);
#ifdef CHANNEL_PIPES
    channel_clear_one(channel, PART_OUT);
    channel_clear_one(channel, PART_ERR);
#endif
    vim_free(channel->ch_callback);
    channel->ch_callback = NULL;
    vim_free(channel->ch_close_cb);
    channel->ch_close_cb = NULL;
}

#if defined(EXITFREE) || defined(PROTO)
    void
channel_free_all(void)
{
    channel_T *channel;

    ch_log(NULL, "channel_free_all()");
    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
	channel_clear(channel);
}
#endif


/* Sent when the channel is found closed when reading. */
#define DETACH_MSG_RAW "DETACH\n"

/* Buffer size for reading incoming messages. */
#define MAXMSGSIZE 4096

/*
 * Check for reading from "fd" with "timeout" msec.
 * Return FAIL when there is nothing to read.
 */
    static int
channel_wait(channel_T *channel, sock_T fd, int timeout)
{
    if (timeout > 0)
	ch_logn(channel, "Waiting for up to %d msec", timeout);

# ifdef WIN32
    if (fd != channel->CH_SOCK_FD)
    {
	DWORD	nread;
	int	diff;
	DWORD	deadline = GetTickCount() + timeout;

	/* reading from a pipe, not a socket */
	while (TRUE)
	{
	    if (PeekNamedPipe((HANDLE)fd, NULL, 0, NULL, &nread, NULL)
								 && nread > 0)
		return OK;
	    diff = deadline - GetTickCount();
	    if (diff <= 0)
		break;
	    /* Wait for 5 msec.
	     * TODO: increase the sleep time when looping more often */
	    Sleep(5);
	}
    }
    else
#endif
    {
#if defined(HAVE_SELECT)
	struct timeval	tval;
	fd_set		rfds;
	int			ret;

	FD_ZERO(&rfds);
	FD_SET((int)fd, &rfds);
	tval.tv_sec = timeout / 1000;
	tval.tv_usec = (timeout % 1000) * 1000;
	for (;;)
	{
	    ret = select((int)fd + 1, &rfds, NULL, NULL, &tval);
# ifdef EINTR
	    SOCK_ERRNO;
	    if (ret == -1 && errno == EINTR)
		continue;
# endif
	    if (ret > 0)
		return OK;
	    break;
	}
#else
	struct pollfd	fds;

	fds.fd = fd;
	fds.events = POLLIN;
	if (poll(&fds, 1, timeout) > 0)
	    return OK;
#endif
    }
    return FAIL;
}

/*
 * Return a unique ID to be used in a message.
 */
    int
channel_get_id(void)
{
    static int next_id = 1;

    return next_id++;
}

/*
 * Read from channel "channel" for as long as there is something to read.
 * "part" is PART_SOCK, PART_OUT or PART_ERR.
 * The data is put in the read queue.
 */
    void
channel_read(channel_T *channel, int part, char *func)
{
    static char_u	*buf = NULL;
    int			len = 0;
    int			readlen = 0;
    sock_T		fd;
    int			use_socket = FALSE;

    fd = channel->ch_part[part].ch_fd;
    if (fd == INVALID_FD)
    {
	ch_error(channel, "channel_read() called while socket is closed");
	return;
    }
    use_socket = fd == channel->CH_SOCK_FD;

    /* Allocate a buffer to read into. */
    if (buf == NULL)
    {
	buf = alloc(MAXMSGSIZE);
	if (buf == NULL)
	    return;	/* out of memory! */
    }

    /* Keep on reading for as long as there is something to read.
     * Use select() or poll() to avoid blocking on a message that is exactly
     * MAXMSGSIZE long. */
    for (;;)
    {
	if (channel_wait(channel, fd, 0) == FAIL)
	    break;
	if (use_socket)
	    len = sock_read(fd, (char *)buf, MAXMSGSIZE);
	else
	    len = fd_read(fd, (char *)buf, MAXMSGSIZE);
	if (len <= 0)
	    break;	/* error or nothing more to read */

	/* Store the read message in the queue. */
	channel_save(channel, part, buf, len);
	readlen += len;
	if (len < MAXMSGSIZE)
	    break;	/* did read everything that's available */
    }

    /* Reading a disconnection (readlen == 0), or an error. */
    if (readlen <= 0)
    {
	/* Do not give an error message, most likely the other end just
	 * exited. */
	ch_errors(channel, "%s(): Cannot read from channel", func);

	/* Queue a "DETACH" netbeans message in the command queue in order to
	 * terminate the netbeans session later. Do not end the session here
	 * directly as we may be running in the context of a call to
	 * netbeans_parse_messages():
	 *	netbeans_parse_messages
	 *	    -> autocmd triggered while processing the netbeans cmd
	 *		-> ui_breakcheck
	 *		    -> gui event loop or select loop
	 *			-> channel_read()
	 * Don't send "DETACH" for a JS or JSON channel.
	 */
	if (channel->ch_part[part].ch_mode == MODE_RAW
				 || channel->ch_part[part].ch_mode == MODE_NL)
	    channel_save(channel, part, (char_u *)DETACH_MSG_RAW,
						 (int)STRLEN(DETACH_MSG_RAW));

	/* TODO: When reading from stdout is not possible, should we try to
	 * keep stdin and stderr open?  Probably not, assume the other side
	 * has died. */
	channel_close(channel, TRUE);
	if (channel->ch_nb_close_cb != NULL)
	    (*channel->ch_nb_close_cb)();
    }

#if defined(CH_HAS_GUI) && defined(FEAT_GUI_GTK)
    /* signal the main loop that there is something to read */
    if (CH_HAS_GUI && gtk_main_level() > 0)
	gtk_main_quit();
#endif
}

/*
 * Read from RAW or NL "channel"/"part".  Blocks until there is something to
 * read or the timeout expires.
 * Returns what was read in allocated memory.
 * Returns NULL in case of error or timeout.
 */
    char_u *
channel_read_block(channel_T *channel, int part, int timeout)
{
    char_u	*buf;
    char_u	*msg;
    ch_mode_T	mode = channel->ch_part[part].ch_mode;
    sock_T	fd = channel->ch_part[part].ch_fd;
    char_u	*nl;

    ch_logsn(channel, "Blocking %s read, timeout: %d msec",
				    mode == MODE_RAW ? "RAW" : "NL", timeout);

    while (TRUE)
    {
	buf = channel_peek(channel, part);
	if (buf != NULL && (mode == MODE_RAW
			 || (mode == MODE_NL && vim_strchr(buf, NL) != NULL)))
	    break;
	if (buf != NULL && channel_collapse(channel, part) == OK)
	    continue;

	/* Wait for up to the channel timeout. */
	if (fd == INVALID_FD
		|| channel_wait(channel, fd, timeout) == FAIL)
	    return NULL;
	channel_read(channel, part, "channel_read_block");
    }

    if (mode == MODE_RAW)
    {
	msg = channel_get_all(channel, part);
    }
    else
    {
	nl = vim_strchr(buf, NL);
	if (nl[1] == NUL)
	{
	    /* get the whole buffer */
	    msg = channel_get(channel, part);
	    *nl = NUL;
	}
	else
	{
	    /* Copy the message into allocated memory and remove it from the
	     * buffer. */
	    msg = vim_strnsave(buf, (int)(nl - buf));
	    mch_memmove(buf, nl + 1, STRLEN(nl + 1) + 1);
	}
    }
    if (log_fd != NULL)
	ch_logn(channel, "Returning %d bytes", (int)STRLEN(msg));
    return msg;
}

/*
 * Read one JSON message with ID "id" from "channel"/"part" and store the
 * result in "rettv".
 * When "id" is -1 accept any message;
 * Blocks until the message is received or the timeout is reached.
 */
    int
channel_read_json_block(
	channel_T   *channel,
	int	    part,
	int	    timeout,
	int	    id,
	typval_T    **rettv)
{
    int		more;
    sock_T	fd;

    ch_log(channel, "Reading JSON");
    if (id != -1)
	channel->ch_part[part].ch_block_id = id;
    for (;;)
    {
	more = channel_parse_json(channel, part);

	/* search for messsage "id" */
	if (channel_get_json(channel, part, id, rettv) == OK)
	{
	    channel->ch_part[part].ch_block_id = 0;
	    return OK;
	}

	if (!more)
	{
	    /* Handle any other messages in the queue.  If done some more
	     * messages may have arrived. */
	    if (channel_parse_messages())
		continue;

	    /* Wait for up to the timeout. */
	    fd = channel->ch_part[part].ch_fd;
	    if (fd == INVALID_FD || channel_wait(channel, fd, timeout) == FAIL)
		break;
	    channel_read(channel, part, "channel_read_json_block");
	}
    }
    channel->ch_part[part].ch_block_id = 0;
    return FAIL;
}

# if defined(WIN32) || defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK) \
	|| defined(PROTO)
/*
 * Lookup the channel from the socket.  Set "partp" to the fd index.
 * Returns NULL when the socket isn't found.
 */
    channel_T *
channel_fd2channel(sock_T fd, int *partp)
{
    channel_T	*channel;
    int		part;

    if (fd != INVALID_FD)
	for (channel = first_channel; channel != NULL;
						   channel = channel->ch_next)
	{
#  ifdef CHANNEL_PIPES
	    for (part = PART_SOCK; part < PART_IN; ++part)
#  else
	    part = PART_SOCK;
#  endif
		if (channel->ch_part[part].ch_fd == fd)
		{
		    *partp = part;
		    return channel;
		}
	}
    return NULL;
}
# endif

# if defined(WIN32) || defined(PROTO)
/*
 * Check the channels for anything that is ready to be read.
 * The data is put in the read queue.
 */
    void
channel_handle_events(void)
{
    channel_T	*channel;
    int		part;
    sock_T	fd;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	/* check the socket and pipes */
	for (part = PART_SOCK; part <= PART_ERR; ++part)
#  else
	/* only check the socket */
	part = PART_SOCK;
#  endif
	{
	    fd = channel->ch_part[part].ch_fd;
	    if (fd != INVALID_FD && channel_wait(channel, fd, 0) == OK)
		channel_read(channel, part, "channel_handle_events");
	}
    }
}
# endif

/*
 * Write "buf" (NUL terminated string) to "channel"/"part".
 * When "fun" is not NULL an error message might be given.
 * Return FAIL or OK.
 */
    int
channel_send(channel_T *channel, int part, char_u *buf, char *fun)
{
    int		len = (int)STRLEN(buf);
    int		res;
    sock_T	fd;

    fd = channel->ch_part[part].ch_fd;
    if (fd == INVALID_FD)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    ch_errors(channel, "%s(): write while not connected", fun);
	    EMSG2("E630: %s(): write while not connected", fun);
	}
	channel->ch_error = TRUE;
	return FAIL;
    }

    if (log_fd != NULL)
    {
	ch_log_lead("SEND ", channel);
	fprintf(log_fd, "'");
	ignored = (int)fwrite(buf, len, 1, log_fd);
	fprintf(log_fd, "'\n");
	fflush(log_fd);
    }

    if (part == PART_SOCK)
	res = sock_write(fd, (char *)buf, len);
    else
	res = fd_write(fd, (char *)buf, len);
    if (res != len)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    ch_errors(channel, "%s(): write failed", fun);
	    EMSG2("E631: %s(): write failed", fun);
	}
	channel->ch_error = TRUE;
	return FAIL;
    }

    channel->ch_error = FALSE;
    return OK;
}

# if (defined(UNIX) && !defined(HAVE_SELECT)) || defined(PROTO)
/*
 * Add open channels to the poll struct.
 * Return the adjusted struct index.
 * The type of "fds" is hidden to avoid problems with the function proto.
 */
    int
channel_poll_setup(int nfd_in, void *fds_in)
{
    int		nfd = nfd_in;
    channel_T	*channel;
    struct	pollfd *fds = fds_in;
    int		part;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (part = PART_SOCK; part < PART_IN; ++part)
#  else
	part = PART_SOCK;
#  endif
	{
	    if (channel->ch_part[part].ch_fd != INVALID_FD)
	    {
		channel->ch_part[part].ch_poll_idx = nfd;
		fds[nfd].fd = channel->ch_part[part].ch_fd;
		fds[nfd].events = POLLIN;
		nfd++;
	    }
	    else
		channel->ch_part[part].ch_poll_idx = -1;
	}
    }

    return nfd;
}

/*
 * The type of "fds" is hidden to avoid problems with the function proto.
 */
    int
channel_poll_check(int ret_in, void *fds_in)
{
    int		ret = ret_in;
    channel_T	*channel;
    struct	pollfd *fds = fds_in;
    int		part;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (part = PART_SOCK; part < PART_IN; ++part)
#  else
	part = PART_SOCK;
#  endif
	{
	    int idx = channel->ch_part[part].ch_poll_idx;

	    if (ret > 0 && idx != -1 && fds[idx].revents & POLLIN)
	    {
		channel_read(channel, part, "channel_poll_check");
		--ret;
	    }
	}
    }

    return ret;
}
# endif /* UNIX && !HAVE_SELECT */

# if (!defined(WIN32) && defined(HAVE_SELECT)) || defined(PROTO)
/*
 * The type of "rfds" is hidden to avoid problems with the function proto.
 */
    int
channel_select_setup(int maxfd_in, void *rfds_in)
{
    int		maxfd = maxfd_in;
    channel_T	*channel;
    fd_set	*rfds = rfds_in;
    int		part;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (part = PART_SOCK; part < PART_IN; ++part)
#  else
	part = PART_SOCK;
#  endif
	{
	    sock_T fd = channel->ch_part[part].ch_fd;

	    if (fd != INVALID_FD)
	    {
		FD_SET((int)fd, rfds);
		if (maxfd < (int)fd)
		    maxfd = (int)fd;
	    }
	}
    }

    return maxfd;
}

/*
 * The type of "rfds" is hidden to avoid problems with the function proto.
 */
    int
channel_select_check(int ret_in, void *rfds_in)
{
    int		ret = ret_in;
    channel_T	*channel;
    fd_set	*rfds = rfds_in;
    int		part;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (part = PART_SOCK; part < PART_IN; ++part)
#  else
	part = PART_SOCK;
#  endif
	{
	    sock_T fd = channel->ch_part[part].ch_fd;

	    if (ret > 0 && fd != INVALID_FD && FD_ISSET(fd, rfds))
	    {
		channel_read(channel, part, "channel_select_check");
		--ret;
	    }
	}
    }

    return ret;
}
# endif /* !WIN32 && HAVE_SELECT */

/*
 * Return TRUE if "channel" has JSON or other typeahead.
 */
    static int
channel_has_readahead(channel_T *channel, int part)
{
    ch_mode_T	ch_mode = channel->ch_part[part].ch_mode;

    if (ch_mode == MODE_JSON || ch_mode == MODE_JS)
    {
	jsonq_T   *head = &channel->ch_part[part].ch_json_head;
	jsonq_T   *item = head->jq_next;

	return item != NULL;
    }
    return channel_peek(channel, part) != NULL;
}

/*
 * Execute queued up commands.
 * Invoked from the main loop when it's safe to execute received commands.
 * Return TRUE when something was done.
 */
    int
channel_parse_messages(void)
{
    channel_T	*channel = first_channel;
    int		ret = FALSE;
    int		r;
    int		part = PART_SOCK;

    ch_log(NULL, "looking for messages on channels");
    while (channel != NULL)
    {
	if (channel->ch_refcount == 0 && !channel_still_useful(channel))
	{
	    /* channel is no longer useful, free it */
	    channel_free(channel);
	    channel = first_channel;
	    part = PART_SOCK;
	    continue;
	}
	if (channel->ch_part[part].ch_fd != INVALID_FD
		|| channel_has_readahead(channel, part))
	{
	    /* Increase the refcount, in case the handler causes the channel
	     * to be unreferenced or closed. */
	    ++channel->ch_refcount;
	    r = may_invoke_callback(channel, part);
	    if (r == OK)
		ret = TRUE;
	    if (channel_unref(channel) || r == OK)
	    {
		/* channel was freed or something was done, start over */
		channel = first_channel;
		part = PART_SOCK;
		continue;
	    }
	}
#ifdef CHANNEL_PIPES
	if (part < PART_ERR)
	    ++part;
	else
#endif
	{
	    channel = channel->ch_next;
	    part = PART_SOCK;
	}
    }

    if (channel_need_redraw && must_redraw)
    {
	channel_need_redraw = FALSE;
	update_screen(0);
	setcursor();
	cursor_on();
	out_flush();
    }

    return ret;
}

/*
 * Mark references to lists used in channels.
 */
    int
set_ref_in_channel(int copyID)
{
    int		abort = FALSE;
    channel_T	*channel;
    int		part;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#ifdef CHANNEL_PIPES
	for (part = PART_SOCK; part < PART_IN; ++part)
#else
	part = PART_SOCK;
#endif
	{
	    jsonq_T *head = &channel->ch_part[part].ch_json_head;
	    jsonq_T *item = head->jq_next;

	    while (item != NULL)
	    {
		list_T	*l = item->jq_value->vval.v_list;

		if (l->lv_copyID != copyID)
		{
		    l->lv_copyID = copyID;
		    abort = abort || set_ref_in_list(l, copyID, NULL);
		}
		item = item->jq_next;
	    }
	}
    }
    return abort;
}

/*
 * Return the "part" to write to for "channel".
 */
    int
channel_part_send(channel_T *channel)
{
#ifdef CHANNEL_PIPES
    if (channel->CH_SOCK_FD == INVALID_FD)
	return PART_IN;
#endif
    return PART_SOCK;
}

/*
 * Return the default "part" to read from for "channel".
 */
    int
channel_part_read(channel_T *channel)
{
#ifdef CHANNEL_PIPES
    if (channel->CH_SOCK_FD == INVALID_FD)
	return PART_OUT;
#endif
    return PART_SOCK;
}

/*
 * Return the mode of "channel"/"part"
 * If "channel" is invalid returns MODE_JSON.
 */
    ch_mode_T
channel_get_mode(channel_T *channel, int part)
{
    if (channel == NULL)
	return MODE_JSON;
    return channel->ch_part[part].ch_mode;
}

/*
 * Return the timeout of "channel"/"part"
 */
    int
channel_get_timeout(channel_T *channel, int part)
{
    return channel->ch_part[part].ch_timeout;
}

#endif /* FEAT_CHANNEL */
