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

#if defined(FEAT_JOB_CHANNEL) || defined(PROTO)

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
ch_logfile(char_u *fname, char_u *opt)
{
    FILE   *file = NULL;

    if (log_fd != NULL)
	fclose(log_fd);

    if (*fname != NUL)
    {
	file = fopen((char *)fname, *opt == 'w' ? "w" : "a");
	if (file == NULL)
	{
	    EMSG2(_(e_notopen), fname);
	    return;
	}
    }
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

static int did_log_msg = TRUE;

    void
ch_log(channel_T *ch, char *msg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch);
	fputs(msg, log_fd);
	fputc('\n', log_fd);
	fflush(log_fd);
	did_log_msg = TRUE;
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
	did_log_msg = TRUE;
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
	did_log_msg = TRUE;
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
	did_log_msg = TRUE;
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
	did_log_msg = TRUE;
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
	did_log_msg = TRUE;
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
	did_log_msg = TRUE;
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

    for (part = PART_SOCK; part <= PART_IN; ++part)
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
    int	has_out_msg;
    int	has_err_msg;

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
    has_out_msg = channel->ch_part[PART_OUT].ch_fd != INVALID_FD
		  || channel->ch_part[PART_OUT].ch_head.rq_next != NULL
		  || channel->ch_part[PART_OUT].ch_json_head.jq_next != NULL;
    has_err_msg = channel->ch_part[PART_ERR].ch_fd != INVALID_FD
		  || channel->ch_part[PART_ERR].ch_head.rq_next != NULL
		  || channel->ch_part[PART_ERR].ch_json_head.jq_next != NULL;
    return (channel->ch_callback != NULL && (has_sock_msg
		|| has_out_msg || has_err_msg))
	    || (channel->ch_part[PART_OUT].ch_callback != NULL && has_out_msg)
	    || (channel->ch_part[PART_ERR].ch_callback != NULL && has_err_msg);
}

/*
 * Close a channel and free all its resources if there is no further action
 * possible, there is no callback to be invoked or the associated job was
 * killed.
 * Return TRUE if the channel was freed.
 */
    static int
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
 * Decrement the reference count on "channel" and maybe free it when it goes
 * down to zero.  Don't free it if there is a pending action.
 * Returns TRUE when the channel is no longer referenced.
 */
    int
channel_unref(channel_T *channel)
{
    if (channel != NULL && --channel->ch_refcount <= 0)
	return channel_may_free(channel);
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
    if (!CH_HAS_GUI)
	return;

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

    static void
channel_gui_register(channel_T *channel)
{
    if (channel->CH_SOCK_FD != INVALID_FD)
	channel_gui_register_one(channel, PART_SOCK);
    if (channel->CH_OUT_FD != INVALID_FD)
	channel_gui_register_one(channel, PART_OUT);
    if (channel->CH_ERR_FD != INVALID_FD)
	channel_gui_register_one(channel, PART_ERR);
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
channel_gui_unregister_one(channel_T *channel, int part)
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

    static void
channel_gui_unregister(channel_T *channel)
{
    int	    part;

    for (part = PART_SOCK; part < PART_IN; ++part)
	channel_gui_unregister_one(channel, part);
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
	long	elapsed_msec = 0;
	int	waitnow;

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

	if (ret == 0)
	    /* The connection could be established. */
	    break;

	SOCK_ERRNO;
	if (waittime < 0 || (errno != EWOULDBLOCK
		&& errno != ECONNREFUSED
#ifdef EINPROGRESS
		&& errno != EINPROGRESS
#endif
		))
	{
	    ch_errorn(channel,
			 "channel_open: Connect failed with errno %d", errno);
	    PERROR(_(e_cannot_connect));
	    sock_close(sd);
	    channel_free(channel);
	    return NULL;
	}

	/* Limit the waittime to 50 msec.  If it doesn't work within this
	 * time we close the socket and try creating it again. */
	waitnow = waittime > 50 ? 50 : waittime;

	/* If connect() didn't finish then try using select() to wait for the
	 * connection to be made. For Win32 always use select() to wait. */
#ifndef WIN32
	if (errno != ECONNREFUSED)
#endif
	{
	    struct timeval	tv;
	    fd_set		rfds;
	    fd_set		wfds;
#ifndef WIN32
	    int			so_error = 0;
	    socklen_t		so_error_len = sizeof(so_error);
	    struct timeval	start_tv;
	    struct timeval	end_tv;
#endif
	    FD_ZERO(&rfds);
	    FD_SET(sd, &rfds);
	    FD_ZERO(&wfds);
	    FD_SET(sd, &wfds);

	    tv.tv_sec = waitnow / 1000;
	    tv.tv_usec = (waitnow % 1000) * 1000;
#ifndef WIN32
	    gettimeofday(&start_tv, NULL);
#endif
	    ch_logn(channel,
		    "Waiting for connection (waiting %d msec)...", waitnow);
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

#ifdef WIN32
	    /* On Win32: select() is expected to work and wait for up to
	     * "waitnow" msec for the socket to be open. */
	    if (FD_ISSET(sd, &wfds))
		break;
	    elapsed_msec = waitnow;
	    if (waittime > 1 && elapsed_msec < waittime)
	    {
		waittime -= elapsed_msec;
		continue;
	    }
#else
	    /* On Linux-like systems: See socket(7) for the behavior
	     * After putting the socket in non-blocking mode, connect() will
	     * return EINPROGRESS, select() will not wait (as if writing is
	     * possible), need to use getsockopt() to check if the socket is
	     * actually able to connect.
	     * We detect a failure to connect when either read and write fds
	     * are set.  Use getsockopt() to find out what kind of failure. */
	    if (FD_ISSET(sd, &rfds) || FD_ISSET(sd, &wfds))
	    {
		ret = getsockopt(sd,
			      SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
		if (ret < 0 || (so_error != 0
			&& so_error != EWOULDBLOCK
			&& so_error != ECONNREFUSED
# ifdef EINPROGRESS
			&& so_error != EINPROGRESS
# endif
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

	    if (FD_ISSET(sd, &wfds) && so_error == 0)
		/* Did not detect an error, connection is established. */
		break;

	    gettimeofday(&end_tv, NULL);
	    elapsed_msec = (end_tv.tv_sec - start_tv.tv_sec) * 1000
			     + (end_tv.tv_usec - start_tv.tv_usec) / 1000;
#endif
	}

#ifndef WIN32
	if (waittime > 1 && elapsed_msec < waittime)
	{
	    /* The port isn't ready but we also didn't get an error.
	     * This happens when the server didn't open the socket
	     * yet.  Select() may return early, wait until the remaining
	     * "waitnow"  and try again. */
	    waitnow -= elapsed_msec;
	    waittime -= elapsed_msec;
	    if (waitnow > 0)
	    {
		mch_delay((long)waitnow, TRUE);
		ui_breakcheck();
		waittime -= waitnow;
	    }
	    if (!got_int)
	    {
		if (waittime <= 0)
		    /* give it one more try */
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
    channel_gui_register_one(channel, PART_SOCK);
#endif

    return channel;
}

/*
 * Implements ch_open().
 */
    channel_T *
channel_open_func(typval_T *argvars)
{
    char_u	*address;
    char_u	*p;
    char	*rest;
    int		port;
    jobopt_T    opt;
    channel_T	*channel;

    address = get_tv_string(&argvars[0]);
    if (argvars[1].v_type != VAR_UNKNOWN
	 && (argvars[1].v_type != VAR_DICT || argvars[1].vval.v_dict == NULL))
    {
	EMSG(_(e_invarg));
	return NULL;
    }

    /* parse address */
    p = vim_strchr(address, ':');
    if (p == NULL)
    {
	EMSG2(_(e_invarg2), address);
	return NULL;
    }
    *p++ = NUL;
    port = strtol((char *)p, &rest, 10);
    if (*address == NUL || port <= 0 || *rest != NUL)
    {
	p[-1] = ':';
	EMSG2(_(e_invarg2), address);
	return NULL;
    }

    /* parse options */
    clear_job_options(&opt);
    opt.jo_mode = MODE_JSON;
    opt.jo_timeout = 2000;
    if (get_job_options(&argvars[1], &opt,
	      JO_MODE_ALL + JO_CB_ALL + JO_WAITTIME + JO_TIMEOUT_ALL) == FAIL)
	return NULL;
    if (opt.jo_timeout < 0)
    {
	EMSG(_(e_invarg));
	return NULL;
    }

    channel = channel_open((char *)address, port, opt.jo_waittime, NULL);
    if (channel != NULL)
    {
	opt.jo_set = JO_ALL;
	channel_set_options(channel, &opt);
    }
    return channel;
}

    static void
may_close_part(sock_T *fd)
{
    if (*fd != INVALID_FD)
    {
	fd_close(*fd);
	*fd = INVALID_FD;
    }
}

    void
channel_set_pipes(channel_T *channel, sock_T in, sock_T out, sock_T err)
{
    if (in != INVALID_FD)
    {
	may_close_part(&channel->CH_IN_FD);
	channel->CH_IN_FD = in;
    }
    if (out != INVALID_FD)
    {
# if defined(FEAT_GUI)
	channel_gui_unregister_one(channel, PART_OUT);
# endif
	may_close_part(&channel->CH_OUT_FD);
	channel->CH_OUT_FD = out;
# if defined(FEAT_GUI)
	channel_gui_register_one(channel, PART_OUT);
# endif
    }
    if (err != INVALID_FD)
    {
# if defined(FEAT_GUI)
	channel_gui_unregister_one(channel, PART_ERR);
# endif
	may_close_part(&channel->CH_ERR_FD);
	channel->CH_ERR_FD = err;
# if defined(FEAT_GUI)
	channel_gui_register_one(channel, PART_ERR);
# endif
    }
}

/*
 * Sets the job the channel is associated with and associated options.
 * This does not keep a refcount, when the job is freed ch_job is cleared.
 */
    void
channel_set_job(channel_T *channel, job_T *job, jobopt_T *options)
{
    channel->ch_job = job;

    channel_set_options(channel, options);

    if (job->jv_in_buf != NULL)
    {
	chanpart_T *in_part = &channel->ch_part[PART_IN];

	in_part->ch_buffer = job->jv_in_buf;
	ch_logs(channel, "reading from buffer '%s'",
					(char *)in_part->ch_buffer->b_ffname);
	if (options->jo_set & JO_IN_TOP)
	{
	    if (options->jo_in_top == 0 && !(options->jo_set & JO_IN_BOT))
	    {
		/* Special mode: send last-but-one line when appending a line
		 * to the buffer. */
		in_part->ch_buffer->b_write_to_channel = TRUE;
		in_part->ch_buf_top =
				   in_part->ch_buffer->b_ml.ml_line_count + 1;
	    }
	    else
		in_part->ch_buf_top = options->jo_in_top;
	}
	else
	    in_part->ch_buf_top = 1;
	if (options->jo_set & JO_IN_BOT)
	    in_part->ch_buf_bot = options->jo_in_bot;
	else
	    in_part->ch_buf_bot = in_part->ch_buffer->b_ml.ml_line_count;
    }
}

/*
 * Find a buffer matching "name" or create a new one.
 */
    static buf_T *
find_buffer(char_u *name, int err)
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
	curbuf = buf;
#ifdef FEAT_QUICKFIX
	set_option_value((char_u *)"bt", 0L, (char_u *)"nofile", OPT_LOCAL);
	set_option_value((char_u *)"bh", 0L, (char_u *)"hide", OPT_LOCAL);
#endif
	if (curbuf->b_ml.ml_mfp == NULL)
	    ml_open(curbuf);
	ml_replace(1, (char_u *)(err ? "Reading from channel error..."
				   : "Reading from channel output..."), TRUE);
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
    int		part;
    char_u	**cbp;
    partial_T	**pp;

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
	pp = &channel->ch_partial;
	vim_free(*cbp);
	partial_unref(*pp);
	if (opt->jo_callback != NULL && *opt->jo_callback != NUL)
	    *cbp = vim_strsave(opt->jo_callback);
	else
	    *cbp = NULL;
	*pp = opt->jo_partial;
	if (*pp != NULL)
	    ++(*pp)->pt_refcount;
    }
    if (opt->jo_set & JO_OUT_CALLBACK)
    {
	cbp = &channel->ch_part[PART_OUT].ch_callback;
	pp = &channel->ch_part[PART_OUT].ch_partial;
	vim_free(*cbp);
	partial_unref(*pp);
	if (opt->jo_out_cb != NULL && *opt->jo_out_cb != NUL)
	    *cbp = vim_strsave(opt->jo_out_cb);
	else
	    *cbp = NULL;
	*pp = opt->jo_out_partial;
	if (*pp != NULL)
	    ++(*pp)->pt_refcount;
    }
    if (opt->jo_set & JO_ERR_CALLBACK)
    {
	cbp = &channel->ch_part[PART_ERR].ch_callback;
	pp = &channel->ch_part[PART_ERR].ch_partial;
	vim_free(*cbp);
	partial_unref(*pp);
	if (opt->jo_err_cb != NULL && *opt->jo_err_cb != NUL)
	    *cbp = vim_strsave(opt->jo_err_cb);
	else
	    *cbp = NULL;
	*pp = opt->jo_err_partial;
	if (*pp != NULL)
	    ++(*pp)->pt_refcount;
    }
    if (opt->jo_set & JO_CLOSE_CALLBACK)
    {
	cbp = &channel->ch_close_cb;
	pp = &channel->ch_close_partial;
	vim_free(*cbp);
	partial_unref(*pp);
	if (opt->jo_close_cb != NULL && *opt->jo_close_cb != NUL)
	    *cbp = vim_strsave(opt->jo_close_cb);
	else
	    *cbp = NULL;
	*pp = opt->jo_err_partial;
	if (*pp != NULL)
	    ++(*pp)->pt_refcount;
    }

    if ((opt->jo_set & JO_OUT_IO) && opt->jo_io[PART_OUT] == JIO_BUFFER)
    {
	/* writing output to a buffer. Default mode is NL. */
	if (!(opt->jo_set & JO_OUT_MODE))
	    channel->ch_part[PART_OUT].ch_mode = MODE_NL;
	if (opt->jo_set & JO_OUT_BUF)
	    channel->ch_part[PART_OUT].ch_buffer =
				     buflist_findnr(opt->jo_io_buf[PART_OUT]);
	else
	    channel->ch_part[PART_OUT].ch_buffer =
				find_buffer(opt->jo_io_name[PART_OUT], FALSE);
	ch_logs(channel, "writing out to buffer '%s'",
		      (char *)channel->ch_part[PART_OUT].ch_buffer->b_ffname);
    }

    if ((opt->jo_set & JO_ERR_IO) && (opt->jo_io[PART_ERR] == JIO_BUFFER
	 || (opt->jo_io[PART_ERR] == JIO_OUT && (opt->jo_set & JO_OUT_IO)
				       && opt->jo_io[PART_OUT] == JIO_BUFFER)))
    {
	/* writing err to a buffer. Default mode is NL. */
	if (!(opt->jo_set & JO_ERR_MODE))
	    channel->ch_part[PART_ERR].ch_mode = MODE_NL;
	if (opt->jo_io[PART_ERR] == JIO_OUT)
	    channel->ch_part[PART_ERR].ch_buffer =
					 channel->ch_part[PART_OUT].ch_buffer;
	else if (opt->jo_set & JO_ERR_BUF)
	    channel->ch_part[PART_ERR].ch_buffer =
				     buflist_findnr(opt->jo_io_buf[PART_ERR]);
	else
	    channel->ch_part[PART_ERR].ch_buffer =
				 find_buffer(opt->jo_io_name[PART_ERR], TRUE);
	ch_logs(channel, "writing err to buffer '%s'",
		      (char *)channel->ch_part[PART_ERR].ch_buffer->b_ffname);
    }
}

/*
 * Set the callback for "channel"/"part" for the response with "id".
 */
    void
channel_set_req_callback(
	channel_T   *channel,
	int	    part,
	char_u	    *callback,
	partial_T   *partial,
	int	    id)
{
    cbq_T *head = &channel->ch_part[part].ch_cb_head;
    cbq_T *item = (cbq_T *)alloc((int)sizeof(cbq_T));

    if (item != NULL)
    {
	item->cq_callback = vim_strsave(callback);
	item->cq_partial = partial;
	if (partial != NULL)
	    ++partial->pt_refcount;
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

    static void
write_buf_line(buf_T *buf, linenr_T lnum, channel_T *channel)
{
    char_u  *line = ml_get_buf(buf, lnum, FALSE);
    int	    len = (int)STRLEN(line);
    char_u  *p;

    /* TODO: check if channel can be written to, do not block on write */
    if ((p = alloc(len + 2)) == NULL)
	return;
    STRCPY(p, line);
    p[len] = NL;
    p[len + 1] = NUL;
    channel_send(channel, PART_IN, p, "write_buf_line()");
    vim_free(p);
}

/*
 * Write any lines to the input channel.
 */
    void
channel_write_in(channel_T *channel)
{
    chanpart_T *in_part = &channel->ch_part[PART_IN];
    linenr_T    lnum;
    buf_T	*buf = in_part->ch_buffer;
    int		written = 0;

    if (buf == NULL)
	return;
    if (!buf_valid(buf) || buf->b_ml.ml_mfp == NULL)
    {
	/* buffer was wiped out or unloaded */
	in_part->ch_buffer = NULL;
	return;
    }
    if (in_part->ch_fd == INVALID_FD)
	/* pipe was closed */
	return;

    for (lnum = in_part->ch_buf_top; lnum <= in_part->ch_buf_bot
				   && lnum <= buf->b_ml.ml_line_count; ++lnum)
    {
	write_buf_line(buf, lnum, channel);
	++written;
    }

    if (written == 1)
	ch_logn(channel, "written line %d to channel", (int)lnum - 1);
    else if (written > 1)
	ch_logn(channel, "written %d lines to channel", written);

    in_part->ch_buf_top = lnum;
}

/*
 * Write appended lines above the last one in "buf" to the channel.
 */
    void
channel_write_new_lines(buf_T *buf)
{
    channel_T	*channel;
    int		found_one = FALSE;

    /* There could be more than one channel for the buffer, loop over all of
     * them. */
    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
	chanpart_T  *in_part = &channel->ch_part[PART_IN];
	linenr_T    lnum;
	int	    written = 0;

	if (in_part->ch_buffer == buf)
	{
	    if (in_part->ch_fd == INVALID_FD)
		/* pipe was closed */
		continue;
	    found_one = TRUE;
	    for (lnum = in_part->ch_buf_bot; lnum < buf->b_ml.ml_line_count;
								       ++lnum)
	    {
		write_buf_line(buf, lnum, channel);
		++written;
	    }

	    if (written == 1)
		ch_logn(channel, "written line %d to channel", (int)lnum - 1);
	    else if (written > 1)
		ch_logn(channel, "written %d lines to channel", written);

	    in_part->ch_buf_bot = lnum;
	}
    }
    if (!found_one)
	buf->b_write_to_channel = FALSE;
}

/*
 * Invoke the "callback" on channel "channel".
 */
    static void
invoke_callback(channel_T *channel, char_u *callback, partial_T *partial,
							       typval_T *argv)
{
    typval_T	rettv;
    int		dummy;

    argv[0].v_type = VAR_CHANNEL;
    argv[0].vval.v_channel = channel;

    call_func(callback, (int)STRLEN(callback),
			&rettv, 2, argv, 0L, 0L, &dummy, TRUE, partial, NULL);
    clear_tv(&rettv);

    redraw_after_callback();
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
channel_save(channel_T *channel, int part, char_u *buf, int len, char *lead)
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

    if (log_fd != NULL && lead != NULL)
    {
	ch_log_lead(lead, channel);
	fprintf(log_fd, "'");
	if (fwrite(buf, len, 1, log_fd) != 1)
	    return FAIL;
	fprintf(log_fd, "'\n");
    }
    return OK;
}

/*
 * Use the read buffer of "channel"/"part" and parse a JSON message that is
 * complete.  The messages are added to the queue.
 * Return TRUE if there is more to read.
 */
    static int
channel_parse_json(channel_T *channel, int part)
{
    js_read_T	reader;
    typval_T	listtv;
    jsonq_T	*item;
    chanpart_T	*chanpart = &channel->ch_part[part];
    jsonq_T	*head = &chanpart->ch_json_head;
    int		status;
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

    /* When a message is incomplete we wait for a short while for more to
     * arrive.  After the delay drop the input, otherwise a truncated string
     * or list will make us hang.  */
    status = json_decode(&reader, &listtv,
		     chanpart->ch_mode == MODE_JS ? JSON_JS : 0);
    if (status == OK)
    {
	/* Only accept the response when it is a list with at least two
	 * items. */
	if (listtv.v_type != VAR_LIST || listtv.vval.v_list->lv_len < 2)
	{
	    if (listtv.v_type != VAR_LIST)
		ch_error(channel, "Did not receive a list, discarding");
	    else
		ch_errorn(channel, "Expected list with two items, got %d",
						  listtv.vval.v_list->lv_len);
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

    if (status == OK)
	chanpart->ch_waiting = FALSE;
    else if (status == MAYBE)
    {
	if (!chanpart->ch_waiting)
	{
	    /* First time encountering incomplete message, set a deadline of
	     * 100 msec. */
	    ch_log(channel, "Incomplete message - wait for more");
	    reader.js_used = 0;
	    chanpart->ch_waiting = TRUE;
#ifdef WIN32
	    chanpart->ch_deadline = GetTickCount() + 100L;
#else
	    gettimeofday(&chanpart->ch_deadline, NULL);
	    chanpart->ch_deadline.tv_usec += 100 * 1000;
	    if (chanpart->ch_deadline.tv_usec > 1000 * 1000)
	    {
		chanpart->ch_deadline.tv_usec -= 1000 * 1000;
		++chanpart->ch_deadline.tv_sec;
	    }
#endif
	}
	else
	{
	    int timeout;
#ifdef WIN32
	    timeout = GetTickCount() > chanpart->ch_deadline;
#else
	    {
		struct timeval now_tv;

		gettimeofday(&now_tv, NULL);
		timeout = now_tv.tv_sec > chanpart->ch_deadline.tv_sec
		      || (now_tv.tv_sec == chanpart->ch_deadline.tv_sec
			   && now_tv.tv_usec > chanpart->ch_deadline.tv_usec);
	    }
#endif
	    if (timeout)
	    {
		status = FAIL;
		chanpart->ch_waiting = FALSE;
	    }
	    else
	    {
		reader.js_used = 0;
		ch_log(channel, "still waiting on incomplete message");
	    }
	}
    }

    if (status == FAIL)
    {
	ch_error(channel, "Decoding failed - discarding input");
	ret = FALSE;
	chanpart->ch_waiting = FALSE;
    }
    else if (reader.js_buf[reader.js_used] != NUL)
    {
	/* Put the unread part back into the channel.
	 * TODO: insert in front */
	channel_save(channel, part, reader.js_buf + reader.js_used,
		(int)(reader.js_end - reader.js_buf) - reader.js_used, NULL);
	ret = status == MAYBE ? FALSE: TRUE;
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
	    if (tv->v_type == VAR_NUMBER)
		ch_logn(channel, "Getting JSON message %d", tv->vval.v_number);
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
	ch_logs(channel, "Executing ex command '%s'", (char *)arg);
	do_cmdline_cmd(arg);
    }
    else if (STRCMP(cmd, "normal") == 0)
    {
	exarg_T ea;

	ch_logs(channel, "Executing normal command '%s'", (char *)arg);
	ea.arg = arg;
	ea.addr_count = 0;
	ea.forceit = TRUE; /* no mapping */
	ex_normal(&ea);
    }
    else if (STRCMP(cmd, "redraw") == 0)
    {
	exarg_T ea;

	ch_log(channel, "redraw");
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
	    {
		ch_logs(channel, "Evaluating expression '%s'", (char *)arg);
		tv = eval_expr(arg, NULL);
	    }
	    else
	    {
		ch_logs(channel, "Calling '%s'", (char *)arg);
		if (func_call(arg, &argv[2], NULL, NULL, &res_tv) == OK)
		    tv = &res_tv;
		else
		    tv = NULL;
	    }

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

    static void
invoke_one_time_callback(
	channel_T   *channel,
	cbq_T	    *cbhead,
	cbq_T	    *item,
	typval_T    *argv)
{
    ch_logs(channel, "Invoking one-time callback %s",
						   (char *)item->cq_callback);
    /* Remove the item from the list first, if the callback
     * invokes ch_close() the list will be cleared. */
    remove_cb_node(cbhead, item);
    invoke_callback(channel, item->cq_callback, item->cq_partial, argv);
    vim_free(item->cq_callback);
    partial_unref(item->cq_partial);
    vim_free(item);
}

    static void
append_to_buffer(buf_T *buffer, char_u *msg, channel_T *channel)
{
    buf_T	*save_curbuf = curbuf;
    linenr_T    lnum = buffer->b_ml.ml_line_count;
    int		save_write_to = buffer->b_write_to_channel;

    /* If the buffer is also used as input insert above the last
     * line. Don't write these lines. */
    if (save_write_to)
    {
	--lnum;
	buffer->b_write_to_channel = FALSE;
    }

    /* Append to the buffer */
    ch_logn(channel, "appending line %d to buffer", (int)lnum + 1);

    curbuf = buffer;
    u_sync(TRUE);
    /* ignore undo failure, undo is not very useful here */
    ignored = u_save(lnum, lnum + 1);

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
		    && (save_write_to
			? wp->w_cursor.lnum == lnum + 1
			: (wp->w_cursor.lnum == lnum
			    && wp->w_cursor.col == 0)))
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

    if (save_write_to)
    {
	channel_T *ch;

	/* Find channels reading from this buffer and adjust their
	 * next-to-read line number. */
	buffer->b_write_to_channel = TRUE;
	for (ch = first_channel; ch != NULL; ch = ch->ch_next)
	{
	    chanpart_T  *in_part = &ch->ch_part[PART_IN];

	    if (in_part->ch_buffer == buffer)
		in_part->ch_buf_bot = buffer->b_ml.ml_line_count;
	}
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
    cbq_T	*cbhead = &channel->ch_part[part].ch_cb_head;
    cbq_T	*cbitem;
    char_u	*callback = NULL;
    partial_T	*partial = NULL;
    buf_T	*buffer = NULL;

    if (channel->ch_nb_close_cb != NULL)
	/* this channel is handled elsewhere (netbeans) */
	return FALSE;

    /* Use a message-specific callback, part callback or channel callback */
    for (cbitem = cbhead->cq_next; cbitem != NULL; cbitem = cbitem->cq_next)
	if (cbitem->cq_seq_nr == 0)
	    break;
    if (cbitem != NULL)
    {
	callback = cbitem->cq_callback;
	partial = cbitem->cq_partial;
    }
    else if (channel->ch_part[part].ch_callback != NULL)
    {
	callback = channel->ch_part[part].ch_callback;
	partial = channel->ch_part[part].ch_partial;
    }
    else
    {
	callback = channel->ch_callback;
	partial = channel->ch_partial;
    }

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
	    /* ["cmd", arg] or ["cmd", arg, arg] or ["cmd", arg, arg, arg] */
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

	if (msg == NULL)
	    return FALSE; /* out of memory (and avoids Coverity warning) */

	argv[1].v_type = VAR_STRING;
	argv[1].vval.v_string = msg;
    }

    if (seq_nr > 0)
    {
	int	done = FALSE;

	/* invoke the one-time callback with the matching nr */
	for (cbitem = cbhead->cq_next; cbitem != NULL; cbitem = cbitem->cq_next)
	    if (cbitem->cq_seq_nr == seq_nr)
	    {
		invoke_one_time_callback(channel, cbhead, cbitem, argv);
		done = TRUE;
		break;
	    }
	if (!done)
	    ch_logn(channel, "Dropping message %d without callback", seq_nr);
    }
    else if (callback != NULL || buffer != NULL)
    {
	if (buffer != NULL)
	{
	    if (msg == NULL)
		/* JSON or JS mode: re-encode the message. */
		msg = json_encode(listtv, ch_mode);
	    if (msg != NULL)
		append_to_buffer(buffer, msg, channel);
	}

	if (callback != NULL)
	{
	    if (cbitem != NULL)
		invoke_one_time_callback(channel, cbhead, cbitem, argv);
	    else
	    {
		/* invoke the channel callback */
		ch_logs(channel, "Invoking channel callback %s",
							    (char *)callback);
		invoke_callback(channel, callback, partial, argv);
	    }
	}
    }
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
			  || channel->CH_IN_FD != INVALID_FD);
}

/*
 * Return TRUE when channel "channel" is open for reading or writing.
 * Also returns FALSE for invalid "channel".
 */
    int
channel_is_open(channel_T *channel)
{
    return channel != NULL && (channel->CH_SOCK_FD != INVALID_FD
			  || channel->CH_IN_FD != INVALID_FD
			  || channel->CH_OUT_FD != INVALID_FD
			  || channel->CH_ERR_FD != INVALID_FD);
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
    may_close_part(&channel->CH_IN_FD);
    may_close_part(&channel->CH_OUT_FD);
    may_close_part(&channel->CH_ERR_FD);

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
			   &rettv, 1, argv, 0L, 0L, &dummy, TRUE,
			   channel->ch_close_partial, NULL);
	  clear_tv(&rettv);
	  --channel->ch_refcount;

	  /* the callback is only called once */
	  vim_free(channel->ch_close_cb);
	  channel->ch_close_cb = NULL;
	  partial_unref(channel->ch_close_partial);
	  channel->ch_close_partial = NULL;
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
	partial_unref(node->cq_partial);
	vim_free(node);
    }

    while (json_head->jq_next != NULL)
    {
	free_tv(json_head->jq_next->jq_value);
	remove_json_node(json_head, json_head->jq_next);
    }

    vim_free(channel->ch_part[part].ch_callback);
    channel->ch_part[part].ch_callback = NULL;
    partial_unref(channel->ch_part[part].ch_partial);
    channel->ch_part[part].ch_partial = NULL;
}

/*
 * Clear all the read buffers on "channel".
 */
    void
channel_clear(channel_T *channel)
{
    ch_log(channel, "Clearing channel");
    channel_clear_one(channel, PART_SOCK);
    channel_clear_one(channel, PART_OUT);
    channel_clear_one(channel, PART_ERR);
    vim_free(channel->ch_callback);
    channel->ch_callback = NULL;
    partial_unref(channel->ch_partial);
    channel->ch_partial = NULL;
    vim_free(channel->ch_close_cb);
    channel->ch_close_cb = NULL;
    partial_unref(channel->ch_close_partial);
    channel->ch_close_partial = NULL;
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
	channel_save(channel, part, buf, len, "RECV ");
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
					 (int)STRLEN(DETACH_MSG_RAW), "PUT ");

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
	if (fd == INVALID_FD)
	    return NULL;
	if (channel_wait(channel, fd, timeout) == FAIL)
	{
	    ch_log(channel, "Timed out");
	    return NULL;
	}
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
	int	    timeout_arg,
	int	    id,
	typval_T    **rettv)
{
    int		more;
    sock_T	fd;
    int		timeout;
    chanpart_T	*chanpart = &channel->ch_part[part];

    ch_log(channel, "Reading JSON");
    if (id != -1)
	chanpart->ch_block_id = id;
    for (;;)
    {
	more = channel_parse_json(channel, part);

	/* search for messsage "id" */
	if (channel_get_json(channel, part, id, rettv) == OK)
	{
	    chanpart->ch_block_id = 0;
	    return OK;
	}

	if (!more)
	{
	    /* Handle any other messages in the queue.  If done some more
	     * messages may have arrived. */
	    if (channel_parse_messages())
		continue;

	    /* Wait for up to the timeout.  If there was an incomplete message
	     * use the deadline for that. */
	    timeout = timeout_arg;
	    if (chanpart->ch_waiting)
	    {
#ifdef WIN32
		timeout = chanpart->ch_deadline - GetTickCount() + 1;
#else
		{
		    struct timeval now_tv;

		    gettimeofday(&now_tv, NULL);
		    timeout = (chanpart->ch_deadline.tv_sec
						       - now_tv.tv_sec) * 1000
			+ (chanpart->ch_deadline.tv_usec
						     - now_tv.tv_usec) / 1000
			+ 1;
		}
#endif
		if (timeout < 0)
		{
		    /* Something went wrong, channel_parse_json() didn't
		     * discard message.  Cancel waiting. */
		    chanpart->ch_waiting = FALSE;
		    timeout = timeout_arg;
		}
		else if (timeout > timeout_arg)
		    timeout = timeout_arg;
	    }
	    fd = chanpart->ch_fd;
	    if (fd == INVALID_FD || channel_wait(channel, fd, timeout) == FAIL)
	    {
		if (timeout == timeout_arg)
		{
		    if (fd != INVALID_FD)
			ch_log(channel, "Timed out");
		    break;
		}
	    }
	    else
		channel_read(channel, part, "channel_read_json_block");
	}
    }
    chanpart->ch_block_id = 0;
    return FAIL;
}

/*
 * Common for ch_read() and ch_readraw().
 */
    void
common_channel_read(typval_T *argvars, typval_T *rettv, int raw)
{
    channel_T	*channel;
    int		part;
    jobopt_T	opt;
    int		mode;
    int		timeout;
    int		id = -1;
    typval_T	*listtv = NULL;

    /* return an empty string by default */
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    clear_job_options(&opt);
    if (get_job_options(&argvars[1], &opt, JO_TIMEOUT + JO_PART + JO_ID)
								      == FAIL)
	return;

    channel = get_channel_arg(&argvars[0], TRUE);
    if (channel != NULL)
    {
	if (opt.jo_set & JO_PART)
	    part = opt.jo_part;
	else
	    part = channel_part_read(channel);
	mode = channel_get_mode(channel, part);
	timeout = channel_get_timeout(channel, part);
	if (opt.jo_set & JO_TIMEOUT)
	    timeout = opt.jo_timeout;

	if (raw || mode == MODE_RAW || mode == MODE_NL)
	    rettv->vval.v_string = channel_read_block(channel, part, timeout);
	else
	{
	    if (opt.jo_set & JO_ID)
		id = opt.jo_id;
	    channel_read_json_block(channel, part, timeout, id, &listtv);
	    if (listtv != NULL)
	    {
		*rettv = *listtv;
		vim_free(listtv);
	    }
	    else
	    {
		rettv->v_type = VAR_SPECIAL;
		rettv->vval.v_number = VVAL_NONE;
	    }
	}
    }
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
	    for (part = PART_SOCK; part < PART_IN; ++part)
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
	/* check the socket and pipes */
	for (part = PART_SOCK; part <= PART_ERR; ++part)
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
	did_log_msg = TRUE;
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

/*
 * Common for "ch_sendexpr()" and "ch_sendraw()".
 * Returns the channel if the caller should read the response.
 * Sets "part_read" to the the read fd.
 * Otherwise returns NULL.
 */
    channel_T *
send_common(
	typval_T    *argvars,
	char_u	    *text,
	int	    id,
	int	    eval,
	jobopt_T    *opt,
	char	    *fun,
	int	    *part_read)
{
    channel_T	*channel;
    int		part_send;

    channel = get_channel_arg(&argvars[0], TRUE);
    if (channel == NULL)
	return NULL;
    part_send = channel_part_send(channel);
    *part_read = channel_part_read(channel);

    clear_job_options(opt);
    if (get_job_options(&argvars[2], opt, JO_CALLBACK + JO_TIMEOUT) == FAIL)
	return NULL;

    /* Set the callback. An empty callback means no callback and not reading
     * the response. With "ch_evalexpr()" and "ch_evalraw()" a callback is not
     * allowed. */
    if (opt->jo_callback != NULL && *opt->jo_callback != NUL)
    {
	if (eval)
	{
	    EMSG2(_("E917: Cannot use a callback with %s()"), fun);
	    return NULL;
	}
	channel_set_req_callback(channel, part_send,
				       opt->jo_callback, opt->jo_partial, id);
    }

    if (channel_send(channel, part_send, text, fun) == OK
						  && opt->jo_callback == NULL)
	return channel;
    return NULL;
}

/*
 * common for "ch_evalexpr()" and "ch_sendexpr()"
 */
    void
ch_expr_common(typval_T *argvars, typval_T *rettv, int eval)
{
    char_u	*text;
    typval_T	*listtv;
    channel_T	*channel;
    int		id;
    ch_mode_T	ch_mode;
    int		part_send;
    int		part_read;
    jobopt_T    opt;
    int		timeout;

    /* return an empty string by default */
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    channel = get_channel_arg(&argvars[0], TRUE);
    if (channel == NULL)
	return;
    part_send = channel_part_send(channel);

    ch_mode = channel_get_mode(channel, part_send);
    if (ch_mode == MODE_RAW || ch_mode == MODE_NL)
    {
	EMSG(_("E912: cannot use ch_evalexpr()/ch_sendexpr() with a raw or nl channel"));
	return;
    }

    id = ++channel->ch_last_msg_id;
    text = json_encode_nr_expr(id, &argvars[1],
					    ch_mode == MODE_JS ? JSON_JS : 0);
    if (text == NULL)
	return;

    channel = send_common(argvars, text, id, eval, &opt,
			    eval ? "ch_evalexpr" : "ch_sendexpr", &part_read);
    vim_free(text);
    if (channel != NULL && eval)
    {
	if (opt.jo_set & JO_TIMEOUT)
	    timeout = opt.jo_timeout;
	else
	    timeout = channel_get_timeout(channel, part_read);
	if (channel_read_json_block(channel, part_read, timeout, id, &listtv)
									== OK)
	{
	    list_T *list = listtv->vval.v_list;

	    /* Move the item from the list and then change the type to
	     * avoid the value being freed. */
	    *rettv = list->lv_last->li_tv;
	    list->lv_last->li_tv.v_type = VAR_NUMBER;
	    free_tv(listtv);
	}
    }
}

/*
 * common for "ch_evalraw()" and "ch_sendraw()"
 */
    void
ch_raw_common(typval_T *argvars, typval_T *rettv, int eval)
{
    char_u	buf[NUMBUFLEN];
    char_u	*text;
    channel_T	*channel;
    int		part_read;
    jobopt_T    opt;
    int		timeout;

    /* return an empty string by default */
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    text = get_tv_string_buf(&argvars[1], buf);
    channel = send_common(argvars, text, 0, eval, &opt,
			      eval ? "ch_evalraw" : "ch_sendraw", &part_read);
    if (channel != NULL && eval)
    {
	if (opt.jo_set & JO_TIMEOUT)
	    timeout = opt.jo_timeout;
	else
	    timeout = channel_get_timeout(channel, part_read);
	rettv->vval.v_string = channel_read_block(channel, part_read, timeout);
    }
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
	for (part = PART_SOCK; part < PART_IN; ++part)
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
	for (part = PART_SOCK; part < PART_IN; ++part)
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
	for (part = PART_SOCK; part < PART_IN; ++part)
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
	for (part = PART_SOCK; part < PART_IN; ++part)
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

    /* Only do this message when another message was given, otherwise we get
     * lots of them. */
    if (did_log_msg)
    {
	ch_log(NULL, "looking for messages on channels");
	did_log_msg = FALSE;
    }
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
	if (part < PART_ERR)
	    ++part;
	else
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
	for (part = PART_SOCK; part < PART_IN; ++part)
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
    if (channel->CH_SOCK_FD == INVALID_FD)
	return PART_IN;
    return PART_SOCK;
}

/*
 * Return the default "part" to read from for "channel".
 */
    int
channel_part_read(channel_T *channel)
{
    if (channel->CH_SOCK_FD == INVALID_FD)
	return PART_OUT;
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

    static int
handle_mode(typval_T *item, jobopt_T *opt, ch_mode_T *modep, int jo)
{
    char_u	*val = get_tv_string(item);

    opt->jo_set |= jo;
    if (STRCMP(val, "nl") == 0)
	*modep = MODE_NL;
    else if (STRCMP(val, "raw") == 0)
	*modep = MODE_RAW;
    else if (STRCMP(val, "js") == 0)
	*modep = MODE_JS;
    else if (STRCMP(val, "json") == 0)
	*modep = MODE_JSON;
    else
    {
	EMSG2(_(e_invarg2), val);
	return FAIL;
    }
    return OK;
}

    static int
handle_io(typval_T *item, int part, jobopt_T *opt)
{
    char_u	*val = get_tv_string(item);

    opt->jo_set |= JO_OUT_IO << (part - PART_OUT);
    if (STRCMP(val, "null") == 0)
	opt->jo_io[part] = JIO_NULL;
    else if (STRCMP(val, "pipe") == 0)
	opt->jo_io[part] = JIO_PIPE;
    else if (STRCMP(val, "file") == 0)
	opt->jo_io[part] = JIO_FILE;
    else if (STRCMP(val, "buffer") == 0)
	opt->jo_io[part] = JIO_BUFFER;
    else if (STRCMP(val, "out") == 0 && part == PART_ERR)
	opt->jo_io[part] = JIO_OUT;
    else
    {
	EMSG2(_(e_invarg2), val);
	return FAIL;
    }
    return OK;
}

    void
clear_job_options(jobopt_T *opt)
{
    vim_memset(opt, 0, sizeof(jobopt_T));
}

/*
 * Get the PART_ number from the first character of an option name.
 */
    static int
part_from_char(int c)
{
    return c == 'i' ? PART_IN : c == 'o' ? PART_OUT: PART_ERR;
}

/*
 * Get the option entries from the dict in "tv", parse them and put the result
 * in "opt".
 * Only accept options in "supported".
 * If an option value is invalid return FAIL.
 */
    int
get_job_options(typval_T *tv, jobopt_T *opt, int supported)
{
    typval_T	*item;
    char_u	*val;
    dict_T	*dict;
    int		todo;
    hashitem_T	*hi;
    int		part;

    opt->jo_set = 0;
    if (tv->v_type == VAR_UNKNOWN)
	return OK;
    if (tv->v_type != VAR_DICT)
    {
	EMSG(_(e_invarg));
	return FAIL;
    }
    dict = tv->vval.v_dict;
    if (dict == NULL)
	return OK;

    todo = (int)dict->dv_hashtab.ht_used;
    for (hi = dict->dv_hashtab.ht_array; todo > 0; ++hi)
	if (!HASHITEM_EMPTY(hi))
	{
	    item = &dict_lookup(hi)->di_tv;

	    if (STRCMP(hi->hi_key, "mode") == 0)
	    {
		if (!(supported & JO_MODE))
		    break;
		if (handle_mode(item, opt, &opt->jo_mode, JO_MODE) == FAIL)
		    return FAIL;
	    }
	    else if (STRCMP(hi->hi_key, "in_mode") == 0)
	    {
		if (!(supported & JO_IN_MODE))
		    break;
		if (handle_mode(item, opt, &opt->jo_in_mode, JO_IN_MODE)
								      == FAIL)
		    return FAIL;
	    }
	    else if (STRCMP(hi->hi_key, "out_mode") == 0)
	    {
		if (!(supported & JO_OUT_MODE))
		    break;
		if (handle_mode(item, opt, &opt->jo_out_mode, JO_OUT_MODE)
								      == FAIL)
		    return FAIL;
	    }
	    else if (STRCMP(hi->hi_key, "err_mode") == 0)
	    {
		if (!(supported & JO_ERR_MODE))
		    break;
		if (handle_mode(item, opt, &opt->jo_err_mode, JO_ERR_MODE)
								      == FAIL)
		    return FAIL;
	    }
	    else if (STRCMP(hi->hi_key, "in_io") == 0
		    || STRCMP(hi->hi_key, "out_io") == 0
		    || STRCMP(hi->hi_key, "err_io") == 0)
	    {
		if (!(supported & JO_OUT_IO))
		    break;
		if (handle_io(item, part_from_char(*hi->hi_key), opt) == FAIL)
		    return FAIL;
	    }
	    else if (STRCMP(hi->hi_key, "in_name") == 0
		    || STRCMP(hi->hi_key, "out_name") == 0
		    || STRCMP(hi->hi_key, "err_name") == 0)
	    {
		part = part_from_char(*hi->hi_key);

		if (!(supported & JO_OUT_IO))
		    break;
		opt->jo_set |= JO_OUT_NAME << (part - PART_OUT);
		opt->jo_io_name[part] =
		       get_tv_string_buf_chk(item, opt->jo_io_name_buf[part]);
	    }
	    else if (STRCMP(hi->hi_key, "in_buf") == 0
		    || STRCMP(hi->hi_key, "out_buf") == 0
		    || STRCMP(hi->hi_key, "err_buf") == 0)
	    {
		part = part_from_char(*hi->hi_key);

		if (!(supported & JO_OUT_IO))
		    break;
		opt->jo_set |= JO_OUT_BUF << (part - PART_OUT);
		opt->jo_io_buf[part] = get_tv_number(item);
		if (opt->jo_io_buf[part] <= 0)
		{
		    EMSG2(_(e_invarg2), get_tv_string(item));
		    return FAIL;
		}
		if (buflist_findnr(opt->jo_io_buf[part]) == NULL)
		{
		    EMSGN(_(e_nobufnr), (long)opt->jo_io_buf[part]);
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "in_top") == 0
		    || STRCMP(hi->hi_key, "in_bot") == 0)
	    {
		linenr_T *lp;

		if (!(supported & JO_OUT_IO))
		    break;
		if (hi->hi_key[3] == 't')
		{
		    lp = &opt->jo_in_top;
		    opt->jo_set |= JO_IN_TOP;
		}
		else
		{
		    lp = &opt->jo_in_bot;
		    opt->jo_set |= JO_IN_BOT;
		}
		*lp = get_tv_number(item);
		if (*lp < 0)
		{
		    EMSG2(_(e_invarg2), get_tv_string(item));
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "channel") == 0)
	    {
		if (!(supported & JO_OUT_IO))
		    break;
		opt->jo_set |= JO_CHANNEL;
		if (item->v_type != VAR_CHANNEL)
		{
		    EMSG2(_(e_invarg2), "channel");
		    return FAIL;
		}
		opt->jo_channel = item->vval.v_channel;
	    }
	    else if (STRCMP(hi->hi_key, "callback") == 0)
	    {
		if (!(supported & JO_CALLBACK))
		    break;
		opt->jo_set |= JO_CALLBACK;
		opt->jo_callback = get_callback(item, &opt->jo_partial);
		if (opt->jo_callback == NULL)
		{
		    EMSG2(_(e_invarg2), "callback");
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "out_cb") == 0)
	    {
		if (!(supported & JO_OUT_CALLBACK))
		    break;
		opt->jo_set |= JO_OUT_CALLBACK;
		opt->jo_out_cb = get_callback(item, &opt->jo_out_partial);
		if (opt->jo_out_cb == NULL)
		{
		    EMSG2(_(e_invarg2), "out_cb");
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "err_cb") == 0)
	    {
		if (!(supported & JO_ERR_CALLBACK))
		    break;
		opt->jo_set |= JO_ERR_CALLBACK;
		opt->jo_err_cb = get_callback(item, &opt->jo_err_partial);
		if (opt->jo_err_cb == NULL)
		{
		    EMSG2(_(e_invarg2), "err_cb");
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "close_cb") == 0)
	    {
		if (!(supported & JO_CLOSE_CALLBACK))
		    break;
		opt->jo_set |= JO_CLOSE_CALLBACK;
		opt->jo_close_cb = get_callback(item, &opt->jo_close_partial);
		if (opt->jo_close_cb == NULL)
		{
		    EMSG2(_(e_invarg2), "close_cb");
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "waittime") == 0)
	    {
		if (!(supported & JO_WAITTIME))
		    break;
		opt->jo_set |= JO_WAITTIME;
		opt->jo_waittime = get_tv_number(item);
	    }
	    else if (STRCMP(hi->hi_key, "timeout") == 0)
	    {
		if (!(supported & JO_TIMEOUT))
		    break;
		opt->jo_set |= JO_TIMEOUT;
		opt->jo_timeout = get_tv_number(item);
	    }
	    else if (STRCMP(hi->hi_key, "out_timeout") == 0)
	    {
		if (!(supported & JO_OUT_TIMEOUT))
		    break;
		opt->jo_set |= JO_OUT_TIMEOUT;
		opt->jo_out_timeout = get_tv_number(item);
	    }
	    else if (STRCMP(hi->hi_key, "err_timeout") == 0)
	    {
		if (!(supported & JO_ERR_TIMEOUT))
		    break;
		opt->jo_set |= JO_ERR_TIMEOUT;
		opt->jo_err_timeout = get_tv_number(item);
	    }
	    else if (STRCMP(hi->hi_key, "part") == 0)
	    {
		if (!(supported & JO_PART))
		    break;
		opt->jo_set |= JO_PART;
		val = get_tv_string(item);
		if (STRCMP(val, "err") == 0)
		    opt->jo_part = PART_ERR;
		else
		{
		    EMSG2(_(e_invarg2), val);
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "id") == 0)
	    {
		if (!(supported & JO_ID))
		    break;
		opt->jo_set |= JO_ID;
		opt->jo_id = get_tv_number(item);
	    }
	    else if (STRCMP(hi->hi_key, "stoponexit") == 0)
	    {
		if (!(supported & JO_STOPONEXIT))
		    break;
		opt->jo_set |= JO_STOPONEXIT;
		opt->jo_stoponexit = get_tv_string_buf_chk(item,
							     opt->jo_soe_buf);
		if (opt->jo_stoponexit == NULL)
		{
		    EMSG2(_(e_invarg2), "stoponexit");
		    return FAIL;
		}
	    }
	    else if (STRCMP(hi->hi_key, "exit_cb") == 0)
	    {
		if (!(supported & JO_EXIT_CB))
		    break;
		opt->jo_set |= JO_EXIT_CB;
		if (item->v_type == VAR_PARTIAL && item->vval.v_partial != NULL)
		{
		    opt->jo_exit_partial = item->vval.v_partial;
		    opt->jo_exit_cb = item->vval.v_partial->pt_name;
		}
		else
		    opt->jo_exit_cb = get_tv_string_buf_chk(
						       item, opt->jo_ecb_buf);
		if (opt->jo_exit_cb == NULL)
		{
		    EMSG2(_(e_invarg2), "exit_cb");
		    return FAIL;
		}
	    }
	    else
		break;
	    --todo;
	}
    if (todo > 0)
    {
	EMSG2(_(e_invarg2), hi->hi_key);
	return FAIL;
    }

    return OK;
}

/*
 * Get the channel from the argument.
 * Returns NULL if the handle is invalid.
 */
    channel_T *
get_channel_arg(typval_T *tv, int check_open)
{
    channel_T *channel = NULL;

    if (tv->v_type == VAR_JOB)
    {
	if (tv->vval.v_job != NULL)
	    channel = tv->vval.v_job->jv_channel;
    }
    else if (tv->v_type == VAR_CHANNEL)
    {
	channel = tv->vval.v_channel;
    }
    else
    {
	EMSG2(_(e_invarg2), get_tv_string(tv));
	return NULL;
    }

    if (check_open && (channel == NULL || !channel_is_open(channel)))
    {
	EMSG(_("E906: not an open channel"));
	return NULL;
    }
    return channel;
}

static job_T *first_job = NULL;

    static void
job_free(job_T *job)
{
    ch_log(job->jv_channel, "Freeing job");
    if (job->jv_channel != NULL)
    {
	/* The link from the channel to the job doesn't count as a reference,
	 * thus don't decrement the refcount of the job.  The reference from
	 * the job to the channel does count the refrence, decrement it and
	 * NULL the reference.  We don't set ch_job_killed, unreferencing the
	 * job doesn't mean it stops running. */
	job->jv_channel->ch_job = NULL;
	channel_unref(job->jv_channel);
    }
    mch_clear_job(job);

    if (job->jv_next != NULL)
	job->jv_next->jv_prev = job->jv_prev;
    if (job->jv_prev == NULL)
	first_job = job->jv_next;
    else
	job->jv_prev->jv_next = job->jv_next;

    vim_free(job->jv_stoponexit);
    vim_free(job->jv_exit_cb);
    partial_unref(job->jv_exit_partial);
    vim_free(job);
}

    void
job_unref(job_T *job)
{
    if (job != NULL && --job->jv_refcount <= 0)
    {
	/* Do not free the job when it has not ended yet and there is a
	 * "stoponexit" flag or an exit callback. */
	if (job->jv_status != JOB_STARTED
		|| (job->jv_stoponexit == NULL && job->jv_exit_cb == NULL))
	{
	    job_free(job);
	}
	else if (job->jv_channel != NULL)
	{
	    /* Do remove the link to the channel, otherwise it hangs
	     * around until Vim exits. See job_free() for refcount. */
	    job->jv_channel->ch_job = NULL;
	    channel_unref(job->jv_channel);
	    job->jv_channel = NULL;
	}
    }
}

/*
 * Allocate a job.  Sets the refcount to one and sets options default.
 */
    static job_T *
job_alloc(void)
{
    job_T *job;

    job = (job_T *)alloc_clear(sizeof(job_T));
    if (job != NULL)
    {
	job->jv_refcount = 1;
	job->jv_stoponexit = vim_strsave((char_u *)"term");

	if (first_job != NULL)
	{
	    first_job->jv_prev = job;
	    job->jv_next = first_job;
	}
	first_job = job;
    }
    return job;
}

    void
job_set_options(job_T *job, jobopt_T *opt)
{
    if (opt->jo_set & JO_STOPONEXIT)
    {
	vim_free(job->jv_stoponexit);
	if (opt->jo_stoponexit == NULL || *opt->jo_stoponexit == NUL)
	    job->jv_stoponexit = NULL;
	else
	    job->jv_stoponexit = vim_strsave(opt->jo_stoponexit);
    }
    if (opt->jo_set & JO_EXIT_CB)
    {
	vim_free(job->jv_exit_cb);
	partial_unref(job->jv_exit_partial);
	if (opt->jo_exit_cb == NULL || *opt->jo_exit_cb == NUL)
	{
	    job->jv_exit_cb = NULL;
	    job->jv_exit_partial = NULL;
	}
	else
	{
	    job->jv_exit_cb = vim_strsave(opt->jo_exit_cb);
	    job->jv_exit_partial = opt->jo_exit_partial;
	    if (job->jv_exit_partial != NULL)
		++job->jv_exit_partial->pt_refcount;
	}
    }
}

/*
 * Called when Vim is exiting: kill all jobs that have the "stoponexit" flag.
 */
    void
job_stop_on_exit()
{
    job_T	*job;

    for (job = first_job; job != NULL; job = job->jv_next)
	if (job->jv_status == JOB_STARTED && job->jv_stoponexit != NULL)
	    mch_stop_job(job, job->jv_stoponexit);
}

/*
 * Called once in a while: check if any jobs with an "exit_cb" have ended.
 */
    void
job_check_ended(void)
{
    static time_t   last_check = 0;
    time_t	    now;
    job_T	    *job;
    job_T	    *next;

    /* Only do this once in 10 seconds. */
    now = time(NULL);
    if (last_check + 10 < now)
    {
	last_check = now;
	for (job = first_job; job != NULL; job = next)
	{
	    next = job->jv_next;
	    if (job->jv_status == JOB_STARTED && job->jv_exit_cb != NULL)
		job_status(job); /* may free "job" */
	}
    }
}

/*
 * "job_start()" function
 */
    job_T *
job_start(typval_T *argvars)
{
    job_T	*job;
    char_u	*cmd = NULL;
#if defined(UNIX)
# define USE_ARGV
    char	**argv = NULL;
    int		argc = 0;
#else
    garray_T	ga;
#endif
    jobopt_T	opt;
    int		part;

    job = job_alloc();
    if (job == NULL)
	return NULL;

    job->jv_status = JOB_FAILED;

    /* Default mode is NL. */
    clear_job_options(&opt);
    opt.jo_mode = MODE_NL;
    if (get_job_options(&argvars[1], &opt,
	    JO_MODE_ALL + JO_CB_ALL + JO_TIMEOUT_ALL
			    + JO_STOPONEXIT + JO_EXIT_CB + JO_OUT_IO) == FAIL)
	return job;

    /* Check that when io is "file" that there is a file name. */
    for (part = PART_OUT; part <= PART_IN; ++part)
	if ((opt.jo_set & (JO_OUT_IO << (part - PART_OUT)))
		&& opt.jo_io[part] == JIO_FILE
		&& (!(opt.jo_set & (JO_OUT_NAME << (part - PART_OUT)))
		    || *opt.jo_io_name[part] == NUL))
	{
	    EMSG(_("E920: _io file requires _name to be set"));
	    return job;
	}

    if ((opt.jo_set & JO_IN_IO) && opt.jo_io[PART_IN] == JIO_BUFFER)
    {
	buf_T *buf = NULL;

	/* check that we can find the buffer before starting the job */
	if (opt.jo_set & JO_IN_BUF)
	{
	    buf = buflist_findnr(opt.jo_io_buf[PART_IN]);
	    if (buf == NULL)
		EMSGN(_(e_nobufnr), (long)opt.jo_io_buf[PART_IN]);
	}
	else if (!(opt.jo_set & JO_IN_NAME))
	{
	    EMSG(_("E915: in_io buffer requires in_buf or in_name to be set"));
	}
	else
	    buf = buflist_find_by_name(opt.jo_io_name[PART_IN], FALSE);
	if (buf == NULL)
	    return job;
	if (buf->b_ml.ml_mfp == NULL)
	{
	    char_u	numbuf[NUMBUFLEN];
	    char_u	*s;

	    if (opt.jo_set & JO_IN_BUF)
	    {
		sprintf((char *)numbuf, "%d", opt.jo_io_buf[PART_IN]);
		s = numbuf;
	    }
	    else
		s = opt.jo_io_name[PART_IN];
	    EMSG2(_("E918: buffer must be loaded: %s"), s);
	    return job;
	}
	job->jv_in_buf = buf;
    }

    job_set_options(job, &opt);

#ifndef USE_ARGV
    ga_init2(&ga, (int)sizeof(char*), 20);
#endif

    if (argvars[0].v_type == VAR_STRING)
    {
	/* Command is a string. */
	cmd = argvars[0].vval.v_string;
#ifdef USE_ARGV
	if (mch_parse_cmd(cmd, FALSE, &argv, &argc) == FAIL)
	    return job;
	argv[argc] = NULL;
#endif
    }
    else if (argvars[0].v_type != VAR_LIST
	    || argvars[0].vval.v_list == NULL
	    || argvars[0].vval.v_list->lv_len < 1)
    {
	EMSG(_(e_invarg));
	return job;
    }
    else
    {
	list_T	    *l = argvars[0].vval.v_list;
	listitem_T  *li;
	char_u	    *s;

#ifdef USE_ARGV
	/* Pass argv[] to mch_call_shell(). */
	argv = (char **)alloc(sizeof(char *) * (l->lv_len + 1));
	if (argv == NULL)
	    return job;
#endif
	for (li = l->lv_first; li != NULL; li = li->li_next)
	{
	    s = get_tv_string_chk(&li->li_tv);
	    if (s == NULL)
		goto theend;
#ifdef USE_ARGV
	    argv[argc++] = (char *)s;
#else
	    /* Only escape when needed, double quotes are not always allowed. */
	    if (li != l->lv_first && vim_strpbrk(s, (char_u *)" \t\"") != NULL)
	    {
# ifdef WIN32
		int old_ssl = p_ssl;

		/* This is using CreateProcess, not cmd.exe.  Always use
		 * double quote and backslashes. */
		p_ssl = 0;
# endif
		s = vim_strsave_shellescape(s, FALSE, TRUE);
# ifdef WIN32
		p_ssl = old_ssl;
# endif
		if (s == NULL)
		    goto theend;
		ga_concat(&ga, s);
		vim_free(s);
	    }
	    else
		ga_concat(&ga, s);
	    if (li->li_next != NULL)
		ga_append(&ga, ' ');
#endif
	}
#ifdef USE_ARGV
	argv[argc] = NULL;
#else
	cmd = ga.ga_data;
#endif
    }

#ifdef USE_ARGV
    if (ch_log_active())
    {
	garray_T    ga;
	int	    i;

	ga_init2(&ga, (int)sizeof(char), 200);
	for (i = 0; i < argc; ++i)
	{
	    if (i > 0)
		ga_concat(&ga, (char_u *)"  ");
	    ga_concat(&ga, (char_u *)argv[i]);
	}
	ch_logs(NULL, "Starting job: %s", (char *)ga.ga_data);
	ga_clear(&ga);
    }
    mch_start_job(argv, job, &opt);
#else
    ch_logs(NULL, "Starting job: %s", (char *)cmd);
    mch_start_job((char *)cmd, job, &opt);
#endif

    /* If the channel is reading from a buffer, write lines now. */
    if (job->jv_channel != NULL)
	channel_write_in(job->jv_channel);

theend:
#ifdef USE_ARGV
    vim_free(argv);
#else
    vim_free(ga.ga_data);
#endif
    return job;
}

/*
 * Get the status of "job" and invoke the exit callback when needed.
 * The returned string is not allocated.
 */
    char *
job_status(job_T *job)
{
    char	*result;

    if (job->jv_status == JOB_ENDED)
	/* No need to check, dead is dead. */
	result = "dead";
    else if (job->jv_status == JOB_FAILED)
	result = "fail";
    else
    {
	result = mch_job_status(job);
	if (job->jv_status == JOB_ENDED)
	    ch_log(job->jv_channel, "Job ended");
	if (job->jv_status == JOB_ENDED && job->jv_exit_cb != NULL)
	{
	    typval_T	argv[3];
	    typval_T	rettv;
	    int		dummy;

	    /* invoke the exit callback; make sure the refcount is > 0 */
	    ++job->jv_refcount;
	    argv[0].v_type = VAR_JOB;
	    argv[0].vval.v_job = job;
	    argv[1].v_type = VAR_NUMBER;
	    argv[1].vval.v_number = job->jv_exitval;
	    call_func(job->jv_exit_cb, (int)STRLEN(job->jv_exit_cb),
			   &rettv, 2, argv, 0L, 0L, &dummy, TRUE,
			   job->jv_exit_partial, NULL);
	    clear_tv(&rettv);
	    --job->jv_refcount;
	}
	if (job->jv_status == JOB_ENDED && job->jv_refcount == 0)
	{
	    /* The job was already unreferenced, now that it ended it can be
	     * freed. Careful: caller must not use "job" after this! */
	    job_free(job);
	}
    }
    return result;
}

/*
 * Implementation of job_info().
 */
    void
job_info(job_T *job, dict_T *dict)
{
    dictitem_T	*item;
    varnumber_T	nr;

    dict_add_nr_str(dict, "status", 0L, (char_u *)job_status(job));

    item = dictitem_alloc((char_u *)"channel");
    if (item == NULL)
	return;
    item->di_tv.v_lock = 0;
    item->di_tv.v_type = VAR_CHANNEL;
    item->di_tv.vval.v_channel = job->jv_channel;
    if (job->jv_channel != NULL)
	++job->jv_channel->ch_refcount;
    if (dict_add(dict, item) == FAIL)
	dictitem_free(item);

#ifdef UNIX
    nr = job->jv_pid;
#else
    nr = job->jv_proc_info.dwProcessId;
#endif
    dict_add_nr_str(dict, "process", nr, NULL);

    dict_add_nr_str(dict, "exitval", job->jv_exitval, NULL);
    dict_add_nr_str(dict, "exit_cb", 0L, job->jv_exit_cb);
    dict_add_nr_str(dict, "stoponexit", 0L, job->jv_stoponexit);
}

    int
job_stop(job_T *job, typval_T *argvars)
{
    char_u *arg;

    if (argvars[1].v_type == VAR_UNKNOWN)
	arg = (char_u *)"";
    else
    {
	arg = get_tv_string_chk(&argvars[1]);
	if (arg == NULL)
	{
	    EMSG(_(e_invarg));
	    return 0;
	}
    }
    ch_logs(job->jv_channel, "Stopping job with '%s'", (char *)arg);
    if (mch_stop_job(job, arg) == FAIL)
	return 0;

    /* Assume that "hup" does not kill the job. */
    if (job->jv_channel != NULL && STRCMP(arg, "hup") != 0)
	job->jv_channel->ch_job_killed = TRUE;

    /* We don't try freeing the job, obviously the caller still has a
     * reference to it. */
    return 1;
}

#endif /* FEAT_JOB_CHANNEL */
