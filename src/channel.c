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
# define fd_read(fd, buf, len, timeout) read(fd, buf, len)
# define fd_write(sd, buf, len) write(sd, buf, len)
# define fd_close(sd) close(sd)
#endif

#ifdef FEAT_GUI_W32
extern HWND s_hwnd;			/* Gvim's Window handle */
#endif

#ifdef WIN32
    static int
fd_read(sock_T fd, char_u *buf, size_t len, int timeout)
{
    HANDLE h = (HANDLE)fd;
    DWORD nread;

    if (!ReadFile(h, buf, (DWORD)len, &nread, NULL))
	return -1;
    return (int)nread;
}

    static int
fd_write(sock_T fd, char_u *buf, size_t len)
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

    void
ch_logfile(FILE *file)
{
    if (log_fd != NULL)
	fclose(log_fd);
    log_fd = file;
    if (log_fd != NULL)
	fprintf(log_fd, "==== start log session ====\n");
}

    static void
ch_log_lead(char *what, channel_T *ch)
{
    if (log_fd != NULL)
    {
	if (ch != NULL)
	    fprintf(log_fd, "%son %d: ", what, ch->ch_id);
	else
	    fprintf(log_fd, "%s: ", what);
    }
}

    static void
ch_log(channel_T *ch, char *msg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch);
	fputs(msg, log_fd);
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
	fflush(log_fd);
    }
}

    static void
ch_logs(channel_T *ch, char *msg, char *name)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch);
	fprintf(log_fd, msg, name);
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
    int		which;
    channel_T	*channel = (channel_T *)alloc_clear((int)sizeof(channel_T));

    if (channel == NULL)
	return NULL;

    channel->ch_id = next_ch_id++;
    ch_log(channel, "Opening channel\n");

#ifdef CHANNEL_PIPES
    for (which = CHAN_SOCK; which <= CHAN_IN; ++which)
#else
    which = CHAN_SOCK;
#endif
    {
	channel->ch_pfd[which].ch_fd = CHAN_FD_INVALID;
#ifdef FEAT_GUI_X11
	channel->ch_pfd[which].ch_inputHandler = (XtInputId)NULL;
#endif
#ifdef FEAT_GUI_GTK
	channel->ch_pfd[which].ch_inputHandler = 0;
#endif
#ifdef FEAT_GUI_W32
	channel->ch_pfd[which].ch_inputHandler = -1;
#endif
    }

    channel->ch_timeout = 2000;

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
 * Close a channel and free all its resources.
 */
    void
channel_free(channel_T *channel)
{
    channel_close(channel);
    if (channel->ch_next != NULL)
	channel->ch_next->ch_prev = channel->ch_prev;
    if (channel->ch_prev == NULL)
	first_channel = channel->ch_next;
    else
	channel->ch_prev->ch_next = channel->ch_next;
    vim_free(channel);
}

#if defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK)
    static channel_T *
channel_from_id(int id)
{
    channel_T *channel;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
	if (channel->ch_id == id)
	    return channel;
    return NULL;
}
#endif

#if defined(FEAT_GUI) || defined(PROTO)

#if defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK)
    static void
channel_read_netbeans(int id)
{
    channel_T *channel = channel_from_id(id);

    if (channel == NULL)
	ch_errorn(NULL, "Channel %d not found", id);
    else
	channel_read(channel, -1, "messageFromNetbeans");
}
#endif

/*
 * Read a command from netbeans.
 */
#ifdef FEAT_GUI_X11
    static void
messageFromNetbeans(XtPointer clientData,
		    int *unused1 UNUSED,
		    XtInputId *unused2 UNUSED)
{
    channel_read_netbeans((int)(long)clientData);
}
#endif

#ifdef FEAT_GUI_GTK
    static void
messageFromNetbeans(gpointer clientData,
		    gint unused1 UNUSED,
		    GdkInputCondition unused2 UNUSED)
{
    channel_read_netbeans((int)(long)clientData);
}
#endif

    static void
channel_gui_register_one(channel_T *channel, int which)
{
# ifdef FEAT_GUI_X11
    /* Tell notifier we are interested in being called
     * when there is input on the editor connection socket. */
    if (channel->ch_pfd[which].ch_inputHandler == (XtInputId)NULL)
	channel->ch_pfd[which].ch_inputHandler = XtAppAddInput(
		(XtAppContext)app_context,
		channel->ch_pfd[which].ch_fd,
		(XtPointer)(XtInputReadMask + XtInputExceptMask),
		messageFromNetbeans,
		(XtPointer)(long)channel->ch_id);
# else
#  ifdef FEAT_GUI_GTK
    /* Tell gdk we are interested in being called when there
     * is input on the editor connection socket. */
    if (channel->ch_pfd[which].ch_inputHandler == 0)
	channel->ch_pfd[which].ch_inputHandler = gdk_input_add(
		(gint)channel->ch_pfd[which].ch_fd,
		(GdkInputCondition)
			     ((int)GDK_INPUT_READ + (int)GDK_INPUT_EXCEPTION),
		messageFromNetbeans,
		(gpointer)(long)channel->ch_id);
#  else
#   ifdef FEAT_GUI_W32
    /* Tell Windows we are interested in receiving message when there
     * is input on the editor connection socket.  */
    if (channel->ch_pfd[which].ch_inputHandler == -1)
	channel->ch_pfd[which].ch_inputHandler = WSAAsyncSelect(
		channel->ch_pfd[which].ch_fd,
		s_hwnd, WM_NETBEANS, FD_READ);
#   endif
#  endif
# endif
}

    void
channel_gui_register(channel_T *channel)
{
    if (!CH_HAS_GUI)
	return;

    if (channel->CH_SOCK != CHAN_FD_INVALID)
	channel_gui_register_one(channel, CHAN_SOCK);
# ifdef CHANNEL_PIPES
    if (channel->CH_OUT != CHAN_FD_INVALID)
	channel_gui_register_one(channel, CHAN_OUT);
    if (channel->CH_ERR != CHAN_FD_INVALID)
	channel_gui_register_one(channel, CHAN_ERR);
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
    int	    which;

#ifdef CHANNEL_PIPES
    for (which = CHAN_SOCK; which < CHAN_IN; ++which)
#else
    which = CHAN_SOCK;
#endif
    {
# ifdef FEAT_GUI_X11
	if (channel->ch_pfd[which].ch_inputHandler != (XtInputId)NULL)
	{
	    XtRemoveInput(channel->ch_pfd[which].ch_inputHandler);
	    channel->ch_pfd[which].ch_inputHandler = (XtInputId)NULL;
	}
# else
#  ifdef FEAT_GUI_GTK
	if (channel->ch_pfd[which].ch_inputHandler != 0)
	{
	    gdk_input_remove(channel->ch_pfd[which].ch_inputHandler);
	    channel->ch_pfd[which].ch_inputHandler = 0;
	}
#  else
#   ifdef FEAT_GUI_W32
	if (channel->ch_pfd[which].ch_inputHandler == 0)
	{
	    WSAAsyncSelect(channel->ch_pfd[which].ch_fd, s_hwnd, 0, 0);
	    channel->ch_pfd[which].ch_inputHandler = -1;
	}
#   endif
#  endif
# endif
    }
}

#endif

/*
 * Open a socket channel to "hostname":"port".
 * Returns the channel for success.
 * Returns NULL for failure.
 */
    channel_T *
channel_open(char *hostname, int port_in, int waittime, void (*close_cb)(void))
{
    int			sd;
    struct sockaddr_in	server;
    struct hostent *	host;
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
	ch_error(NULL, "Cannot allocate channel.\n");
	EMSG(_("E897: All channels are in use"));
	return NULL;
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
	ch_error(NULL, "in socket() in channel_open().\n");
	PERROR("E898: socket() in channel_open()");
	channel_free(channel);
	return NULL;
    }

    /* Get the server internet address and put into addr structure */
    /* fill in the socket address structure and connect to server */
    vim_memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((host = gethostbyname(hostname)) == NULL)
    {
	ch_error(NULL, "in gethostbyname() in channel_open()\n");
	PERROR("E901: gethostbyname() in channel_open()");
	sock_close(sd);
	channel_free(channel);
	return NULL;
    }
    memcpy((char *)&server.sin_addr, host->h_addr, host->h_length);

    if (waittime >= 0)
    {
	/* Make connect non-blocking. */
	if (
#ifdef _WIN32
	    ioctlsocket(sd, FIONBIO, &val) < 0
#else
	    fcntl(sd, F_SETFL, O_NONBLOCK) < 0
#endif
	   )
	{
	    SOCK_ERRNO;
	    ch_errorn(NULL, "channel_open: Connect failed with errno %d\n",
								       errno);
	    sock_close(sd);
	    channel_free(channel);
	    return NULL;
	}
    }

    /* Try connecting to the server. */
    ch_logsn(NULL, "Connecting to %s port %d", hostname, port);
    ret = connect(sd, (struct sockaddr *)&server, sizeof(server));
    SOCK_ERRNO;
    if (ret < 0)
    {
	if (errno != EWOULDBLOCK
#ifdef EINPROGRESS
		    && errno != EINPROGRESS
#endif
		)
	{
	    ch_errorn(NULL, "channel_open: Connect failed with errno %d\n",
								       errno);
	    PERROR(_("E902: Cannot connect to port"));
	    sock_close(sd);
	    channel_free(channel);
	    return NULL;
	}
    }

    if (waittime >= 0 && ret < 0)
    {
	struct timeval	tv;
	fd_set		wfds;

	FD_ZERO(&wfds);
	FD_SET(sd, &wfds);
	tv.tv_sec = waittime / 1000;
	tv.tv_usec = (waittime % 1000) * 1000;
	ret = select((int)sd + 1, NULL, &wfds, NULL, &tv);
	if (ret < 0)
	{
	    SOCK_ERRNO;
	    ch_errorn(NULL, "channel_open: Connect failed with errno %d\n",
								       errno);
	    PERROR(_("E902: Cannot connect to port"));
	    sock_close(sd);
	    channel_free(channel);
	    return NULL;
	}
	if (!FD_ISSET(sd, &wfds))
	{
	    /* don't give an error, we just timed out. */
	    sock_close(sd);
	    channel_free(channel);
	    return NULL;
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

    /* Only retry for netbeans.  TODO: can we use a waittime instead? */
    if (errno == ECONNREFUSED && close_cb != NULL)
    {
	sock_close(sd);
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
	    SOCK_ERRNO;
	    ch_log(NULL, "socket() retry in channel_open()\n");
	    PERROR("E900: socket() retry in channel_open()");
	    channel_free(channel);
	    return NULL;
	}
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)))
	{
	    int retries = 36;
	    int success = FALSE;

	    SOCK_ERRNO;
	    while (retries-- && ((errno == ECONNREFUSED)
						     || (errno == EINTR)))
	    {
		ch_log(NULL, "retrying...\n");
		mch_delay(3000L, TRUE);
		ui_breakcheck();
		if (got_int)
		{
		    errno = EINTR;
		    break;
		}
		if (connect(sd, (struct sockaddr *)&server,
						     sizeof(server)) == 0)
		{
		    success = TRUE;
		    break;
		}
		SOCK_ERRNO;
	    }
	    if (!success)
	    {
		/* Get here when the server can't be found. */
		ch_error(NULL, "Cannot connect to port after retry\n");
		PERROR(_("E899: Cannot connect to port after retry2"));
		sock_close(sd);
		channel_free(channel);
		return NULL;
	    }
	}
    }

    channel->CH_SOCK = (sock_T)sd;
    channel->ch_close_cb = close_cb;

#ifdef FEAT_GUI
    channel_gui_register(channel);
#endif

    return channel;
}

#if defined(CHANNEL_PIPES) || defined(PROTO)
    void
channel_set_pipes(channel_T *channel, sock_T in, sock_T out, sock_T err)
{
    channel->CH_IN = in;
    channel->CH_OUT = out;
    channel->CH_ERR = err;
}
#endif

    void
channel_set_job(channel_T *channel, job_T *job)
{
    channel->ch_job = job;
}

/*
 * Set the json mode of channel "channel" to "ch_mode".
 */
    void
channel_set_json_mode(channel_T *channel, ch_mode_T ch_mode)
{
    channel->ch_mode = ch_mode;
}

/*
 * Set the read timeout of channel "channel".
 */
    void
channel_set_timeout(channel_T *channel, int timeout)
{
    channel->ch_timeout = timeout;
}

/*
 * Set the callback for channel "channel".
 */
    void
channel_set_callback(channel_T *channel, char_u *callback)
{
    vim_free(channel->ch_callback);
    channel->ch_callback = vim_strsave(callback);
}

/*
 * Set the callback for channel "channel" for the response with "id".
 */
    void
channel_set_req_callback(channel_T *channel, char_u *callback, int id)
{
    cbq_T *head = &channel->ch_cb_head;
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
    /* If an echo command was used the cursor needs to be put back where
     * it belongs. */
    setcursor();
    cursor_on();
    out_flush();
}

/*
 * Return the first buffer from the channel and remove it.
 * The caller must free it.
 * Returns NULL if there is nothing.
 */
    char_u *
channel_get(channel_T *channel)
{
    readq_T *head = &channel->ch_head;
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
 * Returns the whole buffer contents concatenated.
 */
    static char_u *
channel_get_all(channel_T *channel)
{
    /* Concatenate everything into one buffer.
     * TODO: avoid multiple allocations. */
    while (channel_collapse(channel) == OK)
	;
    return channel_get(channel);
}

/*
 * Collapses the first and second buffer in the channel "channel".
 * Returns FAIL if that is not possible.
 */
    int
channel_collapse(channel_T *channel)
{
    readq_T *head = &channel->ch_head;
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
 * Use the read buffer of channel "channel" and parse a JSON messages that is
 * complete.  The messages are added to the queue.
 * Return TRUE if there is more to read.
 */
    static int
channel_parse_json(channel_T *channel)
{
    js_read_T	reader;
    typval_T	listtv;
    jsonq_T	*item;
    jsonq_T	*head = &channel->ch_json_head;
    int		ret;

    if (channel_peek(channel) == NULL)
	return FALSE;

    /* TODO: make reader work properly */
    /* reader.js_buf = channel_peek(channel); */
    reader.js_buf = channel_get_all(channel);
    reader.js_used = 0;
    reader.js_fill = NULL;
    /* reader.js_fill = channel_fill; */
    reader.js_cookie = channel;
    ret = json_decode(&reader, &listtv,
				   channel->ch_mode == MODE_JS ? JSON_JS : 0);
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
	channel_save(channel, reader.js_buf + reader.js_used,
		(int)(reader.js_end - reader.js_buf) - reader.js_used);
	ret = TRUE;
    }
    else
	ret = FALSE;

    vim_free(reader.js_buf);
    return ret;
}

/*
 * Remove "node" from the queue that it is in and free it.
 * Also frees the contained callback name.
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
    vim_free(node->cq_callback);
    vim_free(node);
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
channel_get_json(channel_T *channel, int id, typval_T **rettv)
{
    jsonq_T   *head = &channel->ch_json_head;
    jsonq_T   *item = head->jq_next;

    while (item != NULL)
    {
	list_T	    *l = item->jq_value->vval.v_list;
	typval_T    *tv = &l->lv_first->li_tv;

	if ((id > 0 && tv->v_type == VAR_NUMBER && tv->vval.v_number == id)
	      || (id <= 0 && (tv->v_type != VAR_NUMBER
			       || tv->vval.v_number == 0
			       || tv->vval.v_number != channel->ch_block_id)))
	{
	    *rettv = item->jq_value;
	    remove_json_node(head, item);
	    return OK;
	}
	item = item->jq_next;
    }
    return FAIL;
}

/*
 * Execute a command received over channel "channel".
 * "cmd" is the command string, "arg2" the second argument.
 * "arg3" is the third argument, NULL if missing.
 */
    static void
channel_exe_cmd(channel_T *channel, char_u *cmd, typval_T *arg2, typval_T *arg3)
{
    char_u *arg;

    if (arg2->v_type != VAR_STRING)
    {
	if (p_verbose > 2)
	    EMSG("E903: received ex command with non-string argument");
	return;
    }
    arg = arg2->vval.v_string;
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
    else if (STRCMP(cmd, "expr") == 0 || STRCMP(cmd, "eval") == 0)
    {
	int is_eval = cmd[1] == 'v';

	if (is_eval && (arg3 == NULL || arg3->v_type != VAR_NUMBER))
	{
	    if (p_verbose > 2)
		EMSG("E904: third argument for eval must be a number");
	}
	else
	{
	    typval_T	*tv;
	    typval_T	err_tv;
	    char_u	*json = NULL;
	    int		options = channel->ch_mode == MODE_JS ? JSON_JS : 0;

	    /* Don't pollute the display with errors. */
	    ++emsg_skip;
	    tv = eval_expr(arg, NULL);
	    if (is_eval)
	    {
		if (tv != NULL)
		    json = json_encode_nr_expr(arg3->vval.v_number, tv,
								     options);
		if (tv == NULL || (json != NULL && *json == NUL))
		{
		    /* If evaluation failed or the result can't be encoded
		     * then return the string "ERROR". */
		    vim_free(json);
		    free_tv(tv);
		    err_tv.v_type = VAR_STRING;
		    err_tv.vval.v_string = (char_u *)"ERROR";
		    tv = &err_tv;
		    json = json_encode_nr_expr(arg3->vval.v_number, tv,
								     options);
		}
		if (json != NULL)
		{
		    channel_send(channel, json, "eval");
		    vim_free(json);
		}
	    }
	    --emsg_skip;
	    if (tv != &err_tv)
		free_tv(tv);
	}
    }
    else if (p_verbose > 2)
	EMSG2("E905: received unknown command: %s", cmd);
}

/*
 * Invoke a callback for channel "channel" if needed.
 * Return OK when a message was handled, there might be another one.
 */
    static int
may_invoke_callback(channel_T *channel)
{
    char_u	*msg = NULL;
    typval_T	*listtv = NULL;
    list_T	*list;
    typval_T	*typetv;
    typval_T	argv[3];
    int		seq_nr = -1;
    ch_mode_T	ch_mode = channel->ch_mode;

    if (channel->ch_close_cb != NULL)
	/* this channel is handled elsewhere (netbeans) */
	return FALSE;

    if (ch_mode != MODE_RAW)
    {
	/* Get any json message in the queue. */
	if (channel_get_json(channel, -1, &listtv) == FAIL)
	{
	    /* Parse readahead, return when there is still no message. */
	    channel_parse_json(channel);
	    if (channel_get_json(channel, -1, &listtv) == FAIL)
		return FALSE;
	}

	list = listtv->vval.v_list;
	argv[1] = list->lv_first->li_next->li_tv;
	typetv = &list->lv_first->li_tv;
	if (typetv->v_type == VAR_STRING)
	{
	    typval_T	*arg3 = NULL;
	    char_u	*cmd = typetv->vval.v_string;

	    /* ["cmd", arg] or ["cmd", arg, arg] */
	    if (list->lv_len == 3)
		arg3 = &list->lv_last->li_tv;
	    ch_logs(channel, "Executing %s command", (char *)cmd);
	    channel_exe_cmd(channel, cmd, &argv[1], arg3);
	    free_tv(listtv);
	    return TRUE;
	}

	if (typetv->v_type != VAR_NUMBER)
	{
	    ch_error(channel,
		      "Dropping message with invalid sequence number type\n");
	    free_tv(listtv);
	    return FALSE;
	}
	seq_nr = typetv->vval.v_number;
    }
    else if (channel_peek(channel) == NULL)
    {
	/* nothing to read on raw channel */
	return FALSE;
    }
    else
    {
	/* If there is no callback, don't do anything. */
	if (channel->ch_callback == NULL)
	    return FALSE;

	/* For a raw channel we don't know where the message ends, just get
	 * everything. */
	msg = channel_get_all(channel);
	argv[1].v_type = VAR_STRING;
	argv[1].vval.v_string = msg;
    }

    if (seq_nr > 0)
    {
	cbq_T	*head = &channel->ch_cb_head;
	cbq_T	*item = head->cq_next;
	int	done = FALSE;

	/* invoke the one-time callback with the matching nr */
	while (item != NULL)
	{
	    if (item->cq_seq_nr == seq_nr)
	    {
		ch_log(channel, "Invoking one-time callback\n");
		invoke_callback(channel, item->cq_callback, argv);
		remove_cb_node(head, item);
		done = TRUE;
		break;
	    }
	    item = item->cq_next;
	}
	if (!done)
	    ch_log(channel, "Dropping message without callback\n");
    }
    else if (channel->ch_callback != NULL)
    {
	/* invoke the channel callback */
	ch_log(channel, "Invoking channel callback\n");
	invoke_callback(channel, channel->ch_callback, argv);
    }
    else
	ch_log(channel, "Dropping message\n");

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
    return channel != NULL && (channel->CH_SOCK != CHAN_FD_INVALID
#ifdef CHANNEL_PIPES
			  || channel->CH_IN != CHAN_FD_INVALID
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
    return channel != NULL && (channel->CH_SOCK != CHAN_FD_INVALID
#ifdef CHANNEL_PIPES
			  || channel->CH_IN != CHAN_FD_INVALID
			  || channel->CH_OUT != CHAN_FD_INVALID
			  || channel->CH_ERR != CHAN_FD_INVALID
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
 * This does not trigger the close callback.
 */
    void
channel_close(channel_T *channel)
{
    ch_log(channel, "Closing channel");

#ifdef FEAT_GUI
    channel_gui_unregister(channel);
#endif

    if (channel->CH_SOCK != CHAN_FD_INVALID)
    {
	sock_close(channel->CH_SOCK);
	channel->CH_SOCK = CHAN_FD_INVALID;
    }
#if defined(CHANNEL_PIPES)
    if (channel->CH_IN != CHAN_FD_INVALID)
    {
	fd_close(channel->CH_IN);
	channel->CH_IN = CHAN_FD_INVALID;
    }
    if (channel->CH_OUT != CHAN_FD_INVALID)
    {
	fd_close(channel->CH_OUT);
	channel->CH_OUT = CHAN_FD_INVALID;
    }
    if (channel->CH_ERR != CHAN_FD_INVALID)
    {
	fd_close(channel->CH_ERR);
	channel->CH_ERR = CHAN_FD_INVALID;
    }
#endif

    channel->ch_close_cb = NULL;
    channel_clear(channel);
}

/*
 * Store "buf[len]" on channel "channel".
 * Returns OK or FAIL.
 */
    int
channel_save(channel_T *channel, char_u *buf, int len)
{
    readq_T *node;
    readq_T *head = &channel->ch_head;

    node = (readq_T *)alloc(sizeof(readq_T));
    if (node == NULL)
	return FAIL;	    /* out of memory */
    node->rq_buffer = alloc(len + 1);
    if (node->rq_buffer == NULL)
    {
	vim_free(node);
	return FAIL;	    /* out of memory */
    }
    mch_memmove(node->rq_buffer, buf, (size_t)len);
    node->rq_buffer[len] = NUL;

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
 * Return the first buffer from the channel without removing it.
 * Returns NULL if there is nothing.
 */
    char_u *
channel_peek(channel_T *channel)
{
    readq_T *head = &channel->ch_head;

    if (head->rq_next == NULL)
	return NULL;
    return head->rq_next->rq_buffer;
}

/*
 * Clear the read buffer on channel "channel".
 */
    void
channel_clear(channel_T *channel)
{
    jsonq_T *json_head = &channel->ch_json_head;
    cbq_T   *cb_head = &channel->ch_cb_head;

    while (channel_peek(channel) != NULL)
	vim_free(channel_get(channel));

    while (cb_head->cq_next != NULL)
	remove_cb_node(cb_head, cb_head->cq_next);

    while (json_head->jq_next != NULL)
    {
	free_tv(json_head->jq_next->jq_value);
	remove_json_node(json_head, json_head->jq_next);
    }

    vim_free(channel->ch_callback);
    channel->ch_callback = NULL;
}

#if defined(EXITFREE) || defined(PROTO)
    void
channel_free_all(void)
{
    channel_T *channel;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
	channel_clear(channel);
}
#endif


/* Sent when the channel is found closed when reading. */
#define DETACH_MSG "\"DETACH\"\n"

/* Buffer size for reading incoming messages. */
#define MAXMSGSIZE 4096

/*
 * Check for reading from "fd" with "timeout" msec.
 * Return FAIL when there is nothing to read.
 * Always returns OK for FEAT_GUI_W32.
 */
    static int
channel_wait(channel_T *channel, sock_T fd, int timeout)
{
#if defined(HAVE_SELECT) && !defined(FEAT_GUI_W32)
    struct timeval	tval;
    fd_set		rfds;
    int			ret;

    if (timeout > 0)
	ch_logn(channel, "Waiting for %d msec\n", timeout);


# ifdef WIN32
    if (channel->CH_SOCK == CHAN_FD_INVALID)
    {
	DWORD	nread;
	int	diff;
	DWORD	deadline = GetTickCount() + timeout;

	/* reading from a pipe, not a socket */
	while (TRUE)
	{
	    if (PeekNamedPipe(fd, NULL, 0, NULL, &nread, NULL) && nread > 0)
		return OK;
	    diff = deadline - GetTickCount();
	    if (diff < 0)
		break;
	    /* Wait for 5 msec.
	     * TODO: increase the sleep time when looping more often */
	    Sleep(5);
	}
	return FAIL;
    }
#endif

    FD_ZERO(&rfds);
    FD_SET((int)fd, &rfds);
    tval.tv_sec = timeout / 1000;
    tval.tv_usec = (timeout % 1000) * 1000;
    for (;;)
    {
	ret = select((int)fd + 1, &rfds, NULL, NULL, &tval);
# ifdef EINTR
	if (ret == -1 && errno == EINTR)
	    continue;
# endif
	if (ret <= 0)
	{
	    ch_log(channel, "Nothing to read\n");
	    return FAIL;
	}
	break;
    }
#else
# ifdef HAVE_POLL
    struct pollfd	fds;

    if (timeout > 0)
	ch_logn(channel, "Waiting for %d msec\n", timeout);
    fds.fd = fd;
    fds.events = POLLIN;
    if (poll(&fds, 1, timeout) <= 0)
    {
	ch_log(channel, "Nothing to read\n");
	return FAIL;
    }
# endif
#endif
    return OK;
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
 * Get the file descriptor to read from, either the socket or stdout.
 * TODO: should have a way to read stderr.
 */
    static sock_T
get_read_fd(channel_T *channel)
{
    if (channel->CH_SOCK != CHAN_FD_INVALID)
	return channel->CH_SOCK;
#if defined(CHANNEL_PIPES)
    if (channel->CH_OUT != CHAN_FD_INVALID)
	return channel->CH_OUT;
#endif
    ch_error(channel, "channel_read() called while socket is closed\n");
    return CHAN_FD_INVALID;
}

/*
 * Read from channel "channel" for as long as there is something to read.
 * "which" is CHAN_SOCK, CHAN_OUT or CHAN_ERR.  When -1 use CHAN_SOCK or
 * CHAN_OUT, the one that is open.
 * The data is put in the read queue.
 */
    void
channel_read(channel_T *channel, int which, char *func)
{
    static char_u	*buf = NULL;
    int			len = 0;
    int			readlen = 0;
    sock_T		fd;
    int			use_socket = FALSE;

    if (which < 0)
	fd = get_read_fd(channel);
    else
	fd = channel->ch_pfd[which].ch_fd;
    if (fd == CHAN_FD_INVALID)
	return;
    use_socket = fd == channel->CH_SOCK;

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
	    len = sock_read(fd, buf, MAXMSGSIZE);
	else
	    len = fd_read(fd, buf, MAXMSGSIZE, channel->ch_timeout);
	if (len <= 0)
	    break;	/* error or nothing more to read */

	/* Store the read message in the queue. */
	channel_save(channel, buf, len);
	readlen += len;
	if (len < MAXMSGSIZE)
	    break;	/* did read everything that's available */
    }
#ifdef FEAT_GUI_W32
    if (use_socket && len == SOCKET_ERROR)
    {
	/* For Win32 GUI channel_wait() always returns OK and we handle the
	 * situation that there is nothing to read here.
	 * TODO: how about a timeout? */
	if (WSAGetLastError() == WSAEWOULDBLOCK)
	    return;
    }
#endif

    /* Reading a socket disconnection (readlen == 0), or a socket error. */
    if (readlen <= 0)
    {
	/* Queue a "DETACH" netbeans message in the command queue in order to
	 * terminate the netbeans session later. Do not end the session here
	 * directly as we may be running in the context of a call to
	 * netbeans_parse_messages():
	 *	netbeans_parse_messages
	 *	    -> autocmd triggered while processing the netbeans cmd
	 *		-> ui_breakcheck
	 *		    -> gui event loop or select loop
	 *			-> channel_read()
	 */
	ch_errors(channel, "%s(): Cannot read\n", func);
	channel_save(channel, (char_u *)DETACH_MSG, (int)STRLEN(DETACH_MSG));

	/* TODO: When reading from stdout is not possible, should we try to
	 * keep stdin and stderr open?  Probably not, assume the other side
	 * has died. */
	channel_close(channel);
	if (channel->ch_close_cb != NULL)
	    (*channel->ch_close_cb)();

	if (len < 0)
	{
	    ch_error(channel, "channel_read(): cannot read from channel\n");
	    PERROR(_("E896: read from channel"));
	}
    }

#if defined(CH_HAS_GUI) && defined(FEAT_GUI_GTK)
    /* signal the main loop that there is something to read */
    if (CH_HAS_GUI && gtk_main_level() > 0)
	gtk_main_quit();
#endif
}

/*
 * Read from raw channel "channel".  Blocks until there is something to read or
 * the timeout expires.
 * Returns what was read in allocated memory.
 * Returns NULL in case of error or timeout.
 */
    char_u *
channel_read_block(channel_T *channel)
{
    ch_log(channel, "Reading raw\n");
    if (channel_peek(channel) == NULL)
    {
	sock_T fd = get_read_fd(channel);

	/* TODO: read both out and err if they are different */
	ch_log(channel, "No readahead\n");
	/* Wait for up to the channel timeout. */
	if (fd == CHAN_FD_INVALID
		|| channel_wait(channel, fd, channel->ch_timeout) == FAIL)
	    return NULL;
	channel_read(channel, -1, "channel_read_block");
    }

    /* TODO: only get the first message */
    ch_log(channel, "Returning readahead\n");
    return channel_get_all(channel);
}

/*
 * Read one JSON message with ID "id" from channel "channel" and store the
 * result in "rettv".
 * Blocks until the message is received or the timeout is reached.
 */
    int
channel_read_json_block(channel_T *channel, int id, typval_T **rettv)
{
    int		more;
    sock_T	fd;

    ch_log(channel, "Reading JSON\n");
    channel->ch_block_id = id;
    for (;;)
    {
	more = channel_parse_json(channel);

	/* search for messsage "id" */
	if (channel_get_json(channel, id, rettv) == OK)
	{
	    channel->ch_block_id = 0;
	    return OK;
	}

	if (!more)
	{
	    /* Handle any other messages in the queue.  If done some more
	     * messages may have arrived. */
	    if (channel_parse_messages())
		continue;

	    /* Wait for up to the channel timeout. */
	    fd = get_read_fd(channel);
	    if (fd == CHAN_FD_INVALID
		    || channel_wait(channel, fd, channel->ch_timeout) == FAIL)
		break;
	    channel_read(channel, -1, "channel_read_json_block");
	}
    }
    channel->ch_block_id = 0;
    return FAIL;
}

# if defined(WIN32) || defined(PROTO)
/*
 * Lookup the channel from the socket.  Set "which" to the fd index.
 * Returns NULL when the socket isn't found.
 */
    channel_T *
channel_fd2channel(sock_T fd, int *whichp)
{
    channel_T	*channel;
    int		i;

    if (fd != CHAN_FD_INVALID)
	for (channel = first_channel; channel != NULL;
						   channel = channel->ch_next)
	{
#  ifdef CHANNEL_PIPES
	    for (i = CHAN_SOCK; i < CHAN_IN; ++i)
#  else
	    i = CHAN_SOCK;
#  endif
		if (channel->ch_pfd[i].ch_fd == fd)
		{
		    *whichp = i;
		    return channel;
		}
	}
    return NULL;
}
# endif

/*
 * Write "buf" (NUL terminated string) to channel "channel".
 * When "fun" is not NULL an error message might be given.
 * Return FAIL or OK.
 */
    int
channel_send(channel_T *channel, char_u *buf, char *fun)
{
    int		len = (int)STRLEN(buf);
    int		res;
    sock_T	fd = CHAN_FD_INVALID;
    int		use_socket = FALSE;

    if (channel->CH_SOCK != CHAN_FD_INVALID)
    {
	fd = channel->CH_SOCK;
	use_socket = TRUE;
    }
#if defined(CHANNEL_PIPES)
    else if (channel->CH_IN != CHAN_FD_INVALID)
	fd = channel->CH_IN;
#endif
    if (fd == CHAN_FD_INVALID)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    ch_errors(channel, "%s(): write while not connected\n", fun);
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

    if (use_socket)
	res = sock_write(fd, buf, len);
    else
	res = fd_write(fd, buf, len);
    if (res != len)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    ch_errors(channel, "%s(): write failed\n", fun);
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
    int		which;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (which = CHAN_SOCK; which < CHAN_IN; ++which)
#  else
	which = CHAN_SOCK;
#  endif
	{
	    if (channel->ch_pfd[which].ch_fd != CHAN_FD_INVALID)
	    {
		channel->ch_pfd[which].ch_poll_idx = nfd;
		fds[nfd].fd = channel->ch_pfd[which].ch_fd;
		fds[nfd].events = POLLIN;
		nfd++;
	    }
	    else
		channel->ch_pfd[which].ch_poll_idx = -1;
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
    int		which;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (which = CHAN_SOCK; which < CH_IN; ++which)
#  else
	which = CHAN_SOCK;
#  endif
	{
	    int idx = channel->ch_pfd[which].ch_poll_idx;

	    if (ret > 0 && idx != -1 && fds[idx].revents & POLLIN)
	    {
		channel_read(channel, which, "channel_poll_check");
		--ret;
	    }
	}
    }

    return ret;
}
# endif /* UNIX && !HAVE_SELECT */

# if (!defined(FEAT_GUI_W32) && defined(HAVE_SELECT)) || defined(PROTO)
/*
 * The type of "rfds" is hidden to avoid problems with the function proto.
 */
    int
channel_select_setup(int maxfd_in, void *rfds_in)
{
    int		maxfd = maxfd_in;
    channel_T	*channel;
    fd_set	*rfds = rfds_in;
    int		which;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (which = CHAN_SOCK; which < CHAN_IN; ++which)
#  else
	which = CHAN_SOCK;
#  endif
	{
	    sock_T fd = channel->ch_pfd[which].ch_fd;

	    if (fd != CHAN_FD_INVALID)
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
    int		which;

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
#  ifdef CHANNEL_PIPES
	for (which = CHAN_SOCK; which < CHAN_IN; ++which)
#  else
	which = CHAN_SOCK;
#  endif
	{
	    sock_T fd = channel->ch_pfd[which].ch_fd;

	    if (ret > 0 && fd != CHAN_FD_INVALID && FD_ISSET(fd, rfds))
	    {
		channel_read(channel, which, "channel_select_check");
		--ret;
	    }
	}
    }

    return ret;
}
# endif /* !FEAT_GUI_W32 && HAVE_SELECT */

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

    while (channel != NULL)
    {
	/* Increase the refcount, in case the handler causes the channel to be
	 * unreferenced or closed. */
	++channel->ch_refcount;
	r = may_invoke_callback(channel);
	if (channel_unref(channel))
	    /* channel was freed, start over */
	    channel = first_channel;

	if (r == OK)
	{
	    channel = first_channel;  /* something was done, start over */
	    ret = TRUE;
	}
	else
	    channel = channel->ch_next;
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

    for (channel = first_channel; channel != NULL; channel = channel->ch_next)
    {
	jsonq_T *head = &channel->ch_json_head;
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
    return abort;
}

/*
 * Return the mode of channel "channel".
 * If "channel" is invalid returns MODE_JSON.
 */
    ch_mode_T
channel_get_mode(channel_T *channel)
{
    if (channel == NULL)
	return MODE_JSON;
    return channel->ch_mode;
}

#endif /* FEAT_CHANNEL */
