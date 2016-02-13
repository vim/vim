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
# define sock_write(sd, buf, len) send(sd, buf, len, 0)
# define sock_read(sd, buf, len) recv(sd, buf, len, 0)
# define sock_close(sd) closesocket(sd)
# define sleep(t) Sleep(t*1000) /* WinAPI Sleep() accepts milliseconds */
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
#endif

#ifdef FEAT_GUI_W32
extern HWND s_hwnd;			/* Gvim's Window handle */
#endif

/*
 * Information about all channels.
 * There can be gaps for closed channels, they will be reused later.
 */
static channel_T *channels = NULL;
static int channel_count = 0;

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
ch_log_lead(char *what, int ch_idx)
{
    if (log_fd != NULL)
    {
	if (ch_idx >= 0)
	    fprintf(log_fd, "%son %d: ", what, ch_idx);
	else
	    fprintf(log_fd, "%s: ", what);
    }
}

    static void
ch_log(int ch_idx, char *msg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch_idx);
	fputs(msg, log_fd);
	fflush(log_fd);
    }
}

    static void
ch_logn(int ch_idx, char *msg, int nr)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch_idx);
	fprintf(log_fd, msg, nr);
	fflush(log_fd);
    }
}

    static void
ch_logs(int ch_idx, char *msg, char *name)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch_idx);
	fprintf(log_fd, msg, name);
	fflush(log_fd);
    }
}

    static void
ch_logsn(int ch_idx, char *msg, char *name, int nr)
{
    if (log_fd != NULL)
    {
	ch_log_lead("", ch_idx);
	fprintf(log_fd, msg, name, nr);
	fflush(log_fd);
    }
}

    static void
ch_error(int ch_idx, char *msg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("ERR ", ch_idx);
	fputs(msg, log_fd);
	fflush(log_fd);
    }
}

    static void
ch_errorn(int ch_idx, char *msg, int nr)
{
    if (log_fd != NULL)
    {
	ch_log_lead("ERR ", ch_idx);
	fprintf(log_fd, msg, nr);
	fflush(log_fd);
    }
}

    static void
ch_errors(int ch_idx, char *msg, char *arg)
{
    if (log_fd != NULL)
    {
	ch_log_lead("ERR ", ch_idx);
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

    static void
init_channel(int ch_idx)
{
    channel_T	*ch;

    ch = &channels[ch_idx];
    (void)vim_memset(ch, 0, sizeof(channel_T));

    ch->ch_sock = (sock_T)-1;
#ifdef CHANNEL_PIPES
    ch->ch_in = -1;
    ch->ch_out = -1;
    ch->ch_err = -1;
#endif
#ifdef FEAT_GUI_X11
    ch->ch_inputHandler = (XtInputId)NULL;
#endif
#ifdef FEAT_GUI_GTK
    ch->ch_inputHandler = 0;
#endif
#ifdef FEAT_GUI_W32
    ch->ch_inputHandler = -1;
#endif
    /* initialize circular queues */
    ch->ch_head.next = &ch->ch_head;
    ch->ch_head.prev = &ch->ch_head;
    ch->ch_cb_head.next = &ch->ch_cb_head;
    ch->ch_cb_head.prev = &ch->ch_cb_head;
    ch->ch_json_head.next = &ch->ch_json_head;
    ch->ch_json_head.prev = &ch->ch_json_head;

    ch->ch_timeout = 2000;
}

/*
 * Add a new channel slot, return the index.
 * The channel isn't actually used into ch_sock is set >= 0;
 * Returns -1 if all channels are in use.
 */
    int
add_channel(void)
{
    int		ch_idx;

    if (channels != NULL)
    {
	for (ch_idx = 0; ch_idx < channel_count; ++ch_idx)
	    if (!channel_is_open(ch_idx))
	    {
		/* re-use a closed channel slot */
		init_channel(ch_idx);
		ch_log(ch_idx, "Opening channel (used before)\n");
		return ch_idx;
	    }
	if (channel_count == MAX_OPEN_CHANNELS)
	    return -1;
    }
    else
    {
	channels = (channel_T *)alloc((int)sizeof(channel_T)
							 * MAX_OPEN_CHANNELS);
	if (channels == NULL)
	    return -1;
    }
    init_channel(channel_count);
    ch_log(channel_count, "Opening new channel\n");
    return channel_count++;
}

#if defined(FEAT_GUI) || defined(PROTO)
/*
 * Read a command from netbeans.
 */
#ifdef FEAT_GUI_X11
    static void
messageFromNetbeans(XtPointer clientData,
		    int *unused1 UNUSED,
		    XtInputId *unused2 UNUSED)
{
    channel_read((int)(long)clientData, FALSE, "messageFromNetbeans");
}
#endif

#ifdef FEAT_GUI_GTK
    static void
messageFromNetbeans(gpointer clientData,
		    gint unused1 UNUSED,
		    GdkInputCondition unused2 UNUSED)
{
    channel_read((int)(long)clientData, FALSE, "messageFromNetbeans");
}
#endif

    static void
channel_gui_register(int ch_idx)
{
    channel_T	*channel = &channels[ch_idx];

    if (!CH_HAS_GUI)
	return;

    /* TODO: pipes */
# ifdef FEAT_GUI_X11
    /* tell notifier we are interested in being called
     * when there is input on the editor connection socket
     */
    if (channel->ch_inputHandler == (XtInputId)NULL)
	channel->ch_inputHandler =
	    XtAppAddInput((XtAppContext)app_context, channel->ch_sock,
			 (XtPointer)(XtInputReadMask + XtInputExceptMask),
				messageFromNetbeans, (XtPointer)(long)ch_idx);
# else
#  ifdef FEAT_GUI_GTK
    /*
     * Tell gdk we are interested in being called when there
     * is input on the editor connection socket
     */
    if (channel->ch_inputHandler == 0)
	channel->ch_inputHandler =
	    gdk_input_add((gint)channel->ch_sock, (GdkInputCondition)
			     ((int)GDK_INPUT_READ + (int)GDK_INPUT_EXCEPTION),
				 messageFromNetbeans, (gpointer)(long)ch_idx);
#  else
#   ifdef FEAT_GUI_W32
    /*
     * Tell Windows we are interested in receiving message when there
     * is input on the editor connection socket.
     */
    if (channel->ch_inputHandler == -1)
	channel->ch_inputHandler =
	    WSAAsyncSelect(channel->ch_sock, s_hwnd, WM_NETBEANS, FD_READ);
#   endif
#  endif
# endif
}

/*
 * Register any of our file descriptors with the GUI event handling system.
 * Called when the GUI has started.
 */
    void
channel_gui_register_all(void)
{
    int i;

    for (i = 0; i < channel_count; ++i)
	/* TODO: pipes */
	if (channels[i].ch_sock >= 0)
	    channel_gui_register(i);
}

    static void
channel_gui_unregister(int ch_idx)
{
    channel_T	*channel = &channels[ch_idx];

    /* TODO: pipes */
# ifdef FEAT_GUI_X11
    if (channel->ch_inputHandler != (XtInputId)NULL)
    {
	XtRemoveInput(channel->ch_inputHandler);
	channel->ch_inputHandler = (XtInputId)NULL;
    }
# else
#  ifdef FEAT_GUI_GTK
    if (channel->ch_inputHandler != 0)
    {
	gdk_input_remove(channel->ch_inputHandler);
	channel->ch_inputHandler = 0;
    }
#  else
#   ifdef FEAT_GUI_W32
    if (channel->ch_inputHandler == 0)
    {
	WSAAsyncSelect(channel->ch_sock, s_hwnd, 0, 0);
	channel->ch_inputHandler = -1;
    }
#   endif
#  endif
# endif
}

#endif

/*
 * Open a socket channel to "hostname":"port".
 * Returns the channel number for success.
 * Returns a negative number for failure.
 */
    int
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
    int			ch_idx;
    int			ret;

#ifdef WIN32
    channel_init_winsock();
#endif

    ch_idx = add_channel();
    if (ch_idx < 0)
    {
	ch_error(-1, "All channels are in use.\n");
	EMSG(_("E897: All channels are in use"));
	return -1;
    }

    if ((sd = (sock_T)socket(AF_INET, SOCK_STREAM, 0)) == (sock_T)-1)
    {
	ch_error(-1, "in socket() in channel_open().\n");
	PERROR("E898: socket() in channel_open()");
	return -1;
    }

    /* Get the server internet address and put into addr structure */
    /* fill in the socket address structure and connect to server */
    vim_memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((host = gethostbyname(hostname)) == NULL)
    {
	ch_error(-1, "in gethostbyname() in channel_open()\n");
	PERROR("E901: gethostbyname() in channel_open()");
	sock_close(sd);
	return -1;
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
	    ch_errorn(-1, "channel_open: Connect failed with errno %d\n",
								       errno);
	    sock_close(sd);
	    return -1;
	}
    }

    /* Try connecting to the server. */
    ch_logsn(-1, "Connecting to %s port %d", hostname, port);
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
	    ch_errorn(-1, "channel_open: Connect failed with errno %d\n",
								       errno);
	    PERROR(_("E902: Cannot connect to port"));
	    sock_close(sd);
	    return -1;
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
	    ch_errorn(-1, "channel_open: Connect failed with errno %d\n",
								       errno);
	    PERROR(_("E902: Cannot connect to port"));
	    sock_close(sd);
	    return -1;
	}
	if (!FD_ISSET(sd, &wfds))
	{
	    /* don't give an error, we just timed out. */
	    sock_close(sd);
	    return -1;
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
	if ((sd = (sock_T)socket(AF_INET, SOCK_STREAM, 0)) == (sock_T)-1)
	{
	    SOCK_ERRNO;
	    ch_log(-1, "socket() retry in channel_open()\n");
	    PERROR("E900: socket() retry in channel_open()");
	    return -1;
	}
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)))
	{
	    int retries = 36;
	    int success = FALSE;

	    SOCK_ERRNO;
	    while (retries-- && ((errno == ECONNREFUSED)
						     || (errno == EINTR)))
	    {
		ch_log(-1, "retrying...\n");
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
		ch_error(-1, "Cannot connect to port after retry\n");
		PERROR(_("E899: Cannot connect to port after retry2"));
		sock_close(sd);
		return -1;
	    }
	}
    }

    channels[ch_idx].ch_sock = sd;
    channels[ch_idx].ch_close_cb = close_cb;

#ifdef FEAT_GUI
    channel_gui_register(ch_idx);
#endif

    return ch_idx;
}

#if defined(CHANNEL_PIPES) || defined(PROTO)
    void
channel_set_pipes(int ch_idx, int in, int out, int err)
{
    channel_T *channel = &channels[ch_idx];

    channel->ch_in = in;
    channel->ch_out = out;
    channel->ch_err = err;
}
#endif

    void
channel_set_job(int ch_idx, job_T *job)
{
    channels[ch_idx].ch_job = job;
}

/*
 * Set the json mode of channel "ch_idx" to "ch_mode".
 */
    void
channel_set_json_mode(int ch_idx, ch_mode_T ch_mode)
{
    channels[ch_idx].ch_mode = ch_mode;
}

/*
 * Set the read timeout of channel "ch_idx".
 */
    void
channel_set_timeout(int ch_idx, int timeout)
{
    channels[ch_idx].ch_timeout = timeout;
}

/*
 * Set the callback for channel "ch_idx".
 */
    void
channel_set_callback(int ch_idx, char_u *callback)
{
    vim_free(channels[ch_idx].ch_callback);
    channels[ch_idx].ch_callback = vim_strsave(callback);
}

/*
 * Set the callback for channel "ch_idx" for the response with "id".
 */
    void
channel_set_req_callback(int ch_idx, char_u *callback, int id)
{
    cbq_T *cbhead = &channels[ch_idx].ch_cb_head;
    cbq_T *item = (cbq_T *)alloc((int)sizeof(cbq_T));

    if (item != NULL)
    {
	item->callback = vim_strsave(callback);
	item->seq_nr = id;
	item->prev = cbhead->prev;
	cbhead->prev = item;
	item->next = cbhead;
	item->prev->next = item;
    }
}

/*
 * Invoke the "callback" on channel "ch_idx".
 */
    static void
invoke_callback(int ch_idx, char_u *callback, typval_T *argv)
{
    typval_T	rettv;
    int		dummy;

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = ch_idx;

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
channel_get(int ch_idx)
{
    readq_T *head = &channels[ch_idx].ch_head;
    readq_T *node;
    char_u *p;

    if (head->next == head || head->next == NULL)
	return NULL;
    node = head->next;
    /* dispose of the node but keep the buffer */
    p = node->buffer;
    head->next = node->next;
    node->next->prev = node->prev;
    vim_free(node);
    return p;
}

/*
 * Returns the whole buffer contents concatenated.
 */
    static char_u *
channel_get_all(int ch_idx)
{
    /* Concatenate everything into one buffer.
     * TODO: avoid multiple allocations. */
    while (channel_collapse(ch_idx) == OK)
	;
    return channel_get(ch_idx);
}

/*
 * Collapses the first and second buffer in the channel "ch_idx".
 * Returns FAIL if that is not possible.
 */
    int
channel_collapse(int ch_idx)
{
    readq_T *head = &channels[ch_idx].ch_head;
    readq_T *node = head->next;
    char_u  *p;

    if (node == head || node == NULL || node->next == head)
	return FAIL;

    p = alloc((unsigned)(STRLEN(node->buffer)
					   + STRLEN(node->next->buffer) + 1));
    if (p == NULL)
	return FAIL;	    /* out of memory */
    STRCPY(p, node->buffer);
    STRCAT(p, node->next->buffer);
    vim_free(node->next->buffer);
    node->next->buffer = p;

    /* dispose of the node and buffer */
    head->next = node->next;
    node->next->prev = node->prev;
    vim_free(node->buffer);
    vim_free(node);
    return OK;
}

/*
 * Use the read buffer of channel "ch_idx" and parse a JSON messages that is
 * complete.  The messages are added to the queue.
 * Return TRUE if there is more to read.
 */
    static int
channel_parse_json(int ch_idx)
{
    js_read_T	reader;
    typval_T	listtv;
    jsonq_T	*item;
    channel_T	*channel = &channels[ch_idx];
    jsonq_T	*head = &channel->ch_json_head;
    int		ret;

    if (channel_peek(ch_idx) == NULL)
	return FALSE;

    /* TODO: make reader work properly */
    /* reader.js_buf = channel_peek(ch_idx); */
    reader.js_buf = channel_get_all(ch_idx);
    reader.js_used = 0;
    reader.js_fill = NULL;
    /* reader.js_fill = channel_fill; */
    reader.js_cookie = &ch_idx;
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
		item->value = alloc_tv();
		if (item->value == NULL)
		{
		    vim_free(item);
		    clear_tv(&listtv);
		}
		else
		{
		    *item->value = listtv;
		    item->prev = head->prev;
		    head->prev = item;
		    item->next = head;
		    item->prev->next = item;
		}
	    }
	}
    }

    /* Put the unread part back into the channel.
     * TODO: insert in front */
    if (reader.js_buf[reader.js_used] != NUL)
    {
	channel_save(ch_idx, reader.js_buf + reader.js_used,
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
remove_cb_node(cbq_T *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    vim_free(node->callback);
    vim_free(node);
}

/*
 * Remove "node" from the queue that it is in and free it.
 * Caller should have freed or used node->value.
 */
    static void
remove_json_node(jsonq_T *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    vim_free(node);
}

/*
 * Get a message from the JSON queue for channel "ch_idx".
 * When "id" is positive it must match the first number in the list.
 * When "id" is zero or negative jut get the first message.  But not the one
 * with id ch_block_id.
 * Return OK when found and return the value in "rettv".
 * Return FAIL otherwise.
 */
    static int
channel_get_json(int ch_idx, int id, typval_T **rettv)
{
    channel_T *channel = &channels[ch_idx];
    jsonq_T   *head = &channel->ch_json_head;
    jsonq_T   *item = head->next;

    while (item != head)
    {
	list_T	    *l = item->value->vval.v_list;
	typval_T    *tv = &l->lv_first->li_tv;

	if ((id > 0 && tv->v_type == VAR_NUMBER && tv->vval.v_number == id)
	      || (id <= 0 && (tv->v_type != VAR_NUMBER
			       || tv->vval.v_number == 0
			       || tv->vval.v_number != channel->ch_block_id)))
	{
	    *rettv = item->value;
	    remove_json_node(item);
	    return OK;
	}
	item = item->next;
    }
    return FAIL;
}

/*
 * Execute a command received over channel "ch_idx".
 * "cmd" is the command string, "arg2" the second argument.
 * "arg3" is the third argument, NULL if missing.
 */
    static void
channel_exe_cmd(int ch_idx, char_u *cmd, typval_T *arg2, typval_T *arg3)
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
	    channel_T	*channel = &channels[ch_idx];
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
		    err_tv.v_type = VAR_STRING;
		    err_tv.vval.v_string = (char_u *)"ERROR";
		    tv = &err_tv;
		    json = json_encode_nr_expr(arg3->vval.v_number, tv,
								     options);
		}
		if (json != NULL)
		{
		    channel_send(ch_idx, json, "eval");
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
 * Invoke a callback for channel "ch_idx" if needed.
 * Return OK when a message was handled, there might be another one.
 */
    static int
may_invoke_callback(int ch_idx)
{
    char_u	*msg = NULL;
    typval_T	*listtv = NULL;
    list_T	*list;
    typval_T	*typetv;
    typval_T	argv[3];
    int		seq_nr = -1;
    channel_T	*channel = &channels[ch_idx];
    ch_mode_T	ch_mode = channel->ch_mode;

    if (channel->ch_close_cb != NULL)
	/* this channel is handled elsewhere (netbeans) */
	return FALSE;

    if (ch_mode != MODE_RAW)
    {
	/* Get any json message in the queue. */
	if (channel_get_json(ch_idx, -1, &listtv) == FAIL)
	{
	    /* Parse readahead, return when there is still no message. */
	    channel_parse_json(ch_idx);
	    if (channel_get_json(ch_idx, -1, &listtv) == FAIL)
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
	    ch_logs(ch_idx, "Executing %s command", (char *)cmd);
	    channel_exe_cmd(ch_idx, cmd, &argv[1], arg3);
	    clear_tv(listtv);
	    return TRUE;
	}

	if (typetv->v_type != VAR_NUMBER)
	{
	    ch_error(ch_idx,
		      "Dropping message with invalid sequence number type\n");
	    clear_tv(listtv);
	    return FALSE;
	}
	seq_nr = typetv->vval.v_number;
    }
    else if (channel_peek(ch_idx) == NULL)
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
	msg = channel_get_all(ch_idx);
	argv[1].v_type = VAR_STRING;
	argv[1].vval.v_string = msg;
    }

    if (seq_nr > 0)
    {
	cbq_T	*cbhead = &channel->ch_cb_head;
	cbq_T	*cbitem = cbhead->next;
	int	done = FALSE;

	/* invoke the one-time callback with the matching nr */
	while (cbitem != cbhead)
	{
	    if (cbitem->seq_nr == seq_nr)
	    {
		ch_log(ch_idx, "Invoking one-time callback\n");
		invoke_callback(ch_idx, cbitem->callback, argv);
		remove_cb_node(cbitem);
		done = TRUE;
		break;
	    }
	    cbitem = cbitem->next;
	}
	if (!done)
	    ch_log(ch_idx, "Dropping message without callback\n");
    }
    else if (channel->ch_callback != NULL)
    {
	/* invoke the channel callback */
	ch_log(ch_idx, "Invoking channel callback\n");
	invoke_callback(ch_idx, channel->ch_callback, argv);
    }
    else
	ch_log(ch_idx, "Dropping message\n");

    if (listtv != NULL)
	clear_tv(listtv);
    vim_free(msg);

    return TRUE;
}

/*
 * Return TRUE when channel "ch_idx" is open for writing to.
 * Also returns FALSE or invalid "ch_idx".
 */
    int
channel_can_write_to(int ch_idx)
{
    return ch_idx >= 0 && ch_idx < channel_count
		  && (channels[ch_idx].ch_sock >= 0
#ifdef CHANNEL_PIPES
			  || channels[ch_idx].ch_in >= 0
#endif
			  );
}

/*
 * Return TRUE when channel "ch_idx" is open for reading or writing.
 * Also returns FALSE or invalid "ch_idx".
 */
    int
channel_is_open(int ch_idx)
{
    return ch_idx >= 0 && ch_idx < channel_count
		  && (channels[ch_idx].ch_sock >= 0
#ifdef CHANNEL_PIPES
			  || channels[ch_idx].ch_in >= 0
			  || channels[ch_idx].ch_out >= 0
			  || channels[ch_idx].ch_err >= 0
#endif
			  );
}

/*
 * Close channel "ch_idx".
 * This does not trigger the close callback.
 */
    void
channel_close(int ch_idx)
{
    channel_T	*channel = &channels[ch_idx];
    jsonq_T	*jhead;
    cbq_T	*cbhead;

    if (channel->ch_sock >= 0)
    {
	sock_close(channel->ch_sock);
	channel->ch_sock = -1;
	channel->ch_close_cb = NULL;
#ifdef FEAT_GUI
	channel_gui_unregister(ch_idx);
#endif
	vim_free(channel->ch_callback);
	channel->ch_callback = NULL;
	channel->ch_timeout = 2000;

	while (channel_peek(ch_idx) != NULL)
	    vim_free(channel_get(ch_idx));

	cbhead = &channel->ch_cb_head;
	while (cbhead->next != cbhead)
	    remove_cb_node(cbhead->next);

	jhead = &channel->ch_json_head;
	while (jhead->next != jhead)
	{
	    clear_tv(jhead->next->value);
	    remove_json_node(jhead->next);
	}
    }
#if defined(CHANNEL_PIPES)
    if (channel->ch_in >= 0)
    {
	close(channel->ch_in);
	channel->ch_in = -1;
    }
    if (channel->ch_out >= 0)
    {
	close(channel->ch_out);
	channel->ch_out = -1;
    }
    if (channel->ch_err >= 0)
    {
	close(channel->ch_err);
	channel->ch_err = -1;
    }
#endif
}

/*
 * Store "buf[len]" on channel "ch_idx".
 * Returns OK or FAIL.
 */
    int
channel_save(int ch_idx, char_u *buf, int len)
{
    readq_T *node;
    readq_T *head = &channels[ch_idx].ch_head;

    node = (readq_T *)alloc(sizeof(readq_T));
    if (node == NULL)
	return FAIL;	    /* out of memory */
    node->buffer = alloc(len + 1);
    if (node->buffer == NULL)
    {
	vim_free(node);
	return FAIL;	    /* out of memory */
    }
    mch_memmove(node->buffer, buf, (size_t)len);
    node->buffer[len] = NUL;

    /* insert node at tail of queue */
    node->next = head;
    node->prev = head->prev;
    head->prev->next = node;
    head->prev = node;

    if (log_fd != NULL)
    {
	ch_log_lead("RECV ", ch_idx);
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
channel_peek(int ch_idx)
{
    readq_T *head = &channels[ch_idx].ch_head;

    if (head->next == head || head->next == NULL)
	return NULL;
    return head->next->buffer;
}

/*
 * Clear the read buffer on channel "ch_idx".
 */
    void
channel_clear(int ch_idx)
{
    readq_T *head = &channels[ch_idx].ch_head;
    readq_T *node = head->next;
    readq_T *next;

    while (node != NULL && node != head)
    {
	next = node->next;
	vim_free(node->buffer);
	vim_free(node);
	if (next == head)
	{
	    head->next = head;
	    head->prev = head;
	    break;
	}
	node = next;
    }
}

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
channel_wait(int ch_idx, int fd, int timeout)
{
#if defined(HAVE_SELECT) && !defined(FEAT_GUI_W32)
    struct timeval	tval;
    fd_set		rfds;
    int			ret;

    if (timeout > 0)
	ch_logn(ch_idx, "Waiting for %d msec\n", timeout);
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tval.tv_sec = timeout / 1000;
    tval.tv_usec = (timeout % 1000) * 1000;
    for (;;)
    {
	ret = select(fd + 1, &rfds, NULL, NULL, &tval);
# ifdef EINTR
	if (ret == -1 && errno == EINTR)
	    continue;
# endif
	if (ret <= 0)
	{
	    ch_log(ch_idx, "Nothing to read\n");
	    return FAIL;
	}
	break;
    }
#else
# ifdef HAVE_POLL
    struct pollfd	fds;

    if (timeout > 0)
	ch_logn(ch_idx, "Waiting for %d msec\n", timeout);
    fds.fd = fd;
    fds.events = POLLIN;
    if (poll(&fds, 1, timeout) <= 0)
    {
	ch_log(ch_idx, "Nothing to read\n");
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
 */
    static int
get_read_fd(int ch_idx, int use_stderr)
{
    channel_T		*channel = &channels[ch_idx];

    if (channel->ch_sock >= 0)
	return channel->ch_sock;
#if defined(CHANNEL_PIPES)
    if (!use_stderr && channel->ch_out >= 0)
	return channel->ch_out;
    if (use_stderr && channel->ch_err >= 0)
	return channel->ch_err;
#endif
    ch_error(ch_idx, "channel_read() called while socket is closed\n");
    return -1;
}

/*
 * Read from channel "ch_idx" for as long as there is something to read.
 * The data is put in the read queue.
 */
    void
channel_read(int ch_idx, int use_stderr, char *func)
{
    channel_T		*channel = &channels[ch_idx];
    static char_u	*buf = NULL;
    int			len = 0;
    int			readlen = 0;
    int			fd;
    int			use_socket = FALSE;

    fd = get_read_fd(ch_idx, use_stderr);
    if (fd < 0)
	return;
    use_socket = channel->ch_sock >= 0;

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
	if (channel_wait(ch_idx, fd, 0) == FAIL)
	    break;
	if (use_socket)
	    len = sock_read(fd, buf, MAXMSGSIZE);
	else
	    len = read(fd, buf, MAXMSGSIZE);
	if (len <= 0)
	    break;	/* error or nothing more to read */

	/* Store the read message in the queue. */
	channel_save(ch_idx, buf, len);
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
	ch_errors(ch_idx, "%s(): Cannot read\n", func);
	channel_save(ch_idx, (char_u *)DETACH_MSG, (int)STRLEN(DETACH_MSG));

	if (use_socket)
	{
	    channel_close(ch_idx);
	    if (channel->ch_close_cb != NULL)
		(*channel->ch_close_cb)();
	}
#if defined(CHANNEL_PIPES)
	else
	{
	    close(fd);
	    channel->ch_out = -1;
	}
#endif

	if (len < 0)
	{
	    ch_error(ch_idx, "channel_read(): cannot read from channel\n");
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
 * Read from raw channel "ch_idx".  Blocks until there is something to read or
 * the timeout expires.
 * Returns what was read in allocated memory.
 * Returns NULL in case of error or timeout.
 */
    char_u *
channel_read_block(int ch_idx)
{
    ch_log(ch_idx, "Reading raw\n");
    if (channel_peek(ch_idx) == NULL)
    {
	int fd = get_read_fd(ch_idx, FALSE);

	ch_log(ch_idx, "No readahead\n");
	/* Wait for up to the channel timeout. */
	if (fd < 0 || channel_wait(ch_idx, fd,
					 channels[ch_idx].ch_timeout) == FAIL)
	    return NULL;
	channel_read(ch_idx, FALSE, "channel_read_block");
    }

    /* TODO: only get the first message */
    ch_log(ch_idx, "Returning readahead\n");
    return channel_get_all(ch_idx);
}

/*
 * Read one JSON message from channel "ch_idx" with ID "id" and store the
 * result in "rettv".
 * Blocks until the message is received or the timeout is reached.
 */
    int
channel_read_json_block(int ch_idx, int id, typval_T **rettv)
{
    int		more;
    channel_T	*channel = &channels[ch_idx];
    int		fd;

    ch_log(ch_idx, "Reading JSON\n");
    channel->ch_block_id = id;
    for (;;)
    {
	more = channel_parse_json(ch_idx);

	/* search for messsage "id" */
	if (channel_get_json(ch_idx, id, rettv) == OK)
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
	    fd = get_read_fd(ch_idx, FALSE);
	    if (fd < 0 || channel_wait(ch_idx, fd, channel->ch_timeout) == FAIL)
		break;
	    channel_read(ch_idx, FALSE, "channel_read_json_block");
	}
    }
    channel->ch_block_id = 0;
    return FAIL;
}

# if defined(WIN32) || defined(PROTO)
/*
 * Lookup the channel index from the socket.
 * Returns -1 when the socket isn't found.
 */
    int
channel_fd2idx(sock_T fd)
{
    int i;

    if (fd >= 0)
	for (i = 0; i < channel_count; ++i)
	    if (channels[i].ch_sock == fd
#  if defined(CHANNEL_PIPES)
		    || channels[i].ch_out == fd
		    || channels[i].ch_err == fd
#  endif
		    )
		return i;
    return -1;
}
# endif

/*
 * Write "buf" (NUL terminated string) to channel "ch_idx".
 * When "fun" is not NULL an error message might be given.
 * Return FAIL or OK.
 */
    int
channel_send(int ch_idx, char_u *buf, char *fun)
{
    channel_T	*channel = &channels[ch_idx];
    int		len = (int)STRLEN(buf);
    int		res;
    int		fd;
    int		use_socket = FALSE;

    if (channel->ch_sock >= 0)
    {
	fd = channel->ch_sock;
	use_socket = TRUE;
    }
#if defined(CHANNEL_PIPES)
    else if (channel->ch_in >= 0)
	fd = channel->ch_in;
#endif
    if (fd < 0)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    ch_errors(ch_idx, "%s(): write while not connected\n", fun);
	    EMSG2("E630: %s(): write while not connected", fun);
	}
	channel->ch_error = TRUE;
	return FAIL;
    }

    if (log_fd != NULL)
    {
	ch_log_lead("SEND ", ch_idx);
	fprintf(log_fd, "'");
	ignored = fwrite(buf, len, 1, log_fd);
	fprintf(log_fd, "'\n");
	fflush(log_fd);
    }

    if (use_socket)
	res = sock_write(fd, buf, len);
    else
	res = write(fd, buf, len);
    if (res != len)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    ch_errors(ch_idx, "%s(): write failed\n", fun);
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
    int nfd = nfd_in;
    int i;
    struct pollfd *fds = fds_in;

    for (i = 0; i < channel_count; ++i)
    {
	if (channels[i].ch_sock >= 0)
	{
	    channels[i].ch_sock_idx = nfd;
	    fds[nfd].fd = channels[i].ch_sock;
	    fds[nfd].events = POLLIN;
	    nfd++;
	}
	else
	    channels[i].ch_sock_idx = -1;

#  ifdef CHANNEL_PIPES
	if (channels[i].ch_out >= 0)
	{
	    channels[i].ch_out_idx = nfd;
	    fds[nfd].fd = channels[i].ch_out;
	    fds[nfd].events = POLLIN;
	    nfd++;
	}
	else
	    channels[i].ch_out_idx = -1;

	if (channels[i].ch_err >= 0)
	{
	    channels[i].ch_err_idx = nfd;
	    fds[nfd].fd = channels[i].ch_err;
	    fds[nfd].events = POLLIN;
	    nfd++;
	}
	else
	    channels[i].ch_err_idx = -1;
#  endif
    }

    return nfd;
}

/*
 * The type of "fds" is hidden to avoid problems with the function proto.
 */
    int
channel_poll_check(int ret_in, void *fds_in)
{
    int ret = ret_in;
    int i;
    struct pollfd *fds = fds_in;

    for (i = 0; i < channel_count; ++i)
    {
	if (ret > 0 && channels[i].ch_sock_idx != -1
			     && fds[channels[i].ch_sock_idx].revents & POLLIN)
	{
	    channel_read(i, FALSE, "channel_poll_check");
	    --ret;
	}
#  ifdef CHANNEL_PIPES
	if (ret > 0 && channels[i].ch_out_idx != -1
			       && fds[channels[i].ch_out_idx].revents & POLLIN)
	{
	    channel_read(i, FALSE, "channel_poll_check");
	    --ret;
	}
	if (ret > 0 && channels[i].ch_err_idx != -1
			       && fds[channels[i].ch_err_idx].revents & POLLIN)
	{
	    channel_read(i, TRUE, "channel_poll_check");
	    --ret;
	}
#  endif
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
    int	    maxfd = maxfd_in;
    int	    i;
    fd_set  *rfds = rfds_in;

    for (i = 0; i < channel_count; ++i)
    {
	if (channels[i].ch_sock >= 0)
	{
	    FD_SET(channels[i].ch_sock, rfds);
	    if (maxfd < channels[i].ch_sock)
		maxfd = channels[i].ch_sock;
	}
#  ifdef CHANNEL_PIPES
	if (channels[i].ch_out >= 0)
	{
	    FD_SET(channels[i].ch_out, rfds);
	    if (maxfd < channels[i].ch_out)
		maxfd = channels[i].ch_out;
	}
	if (channels[i].ch_err >= 0)
	{
	    FD_SET(channels[i].ch_err, rfds);
	    if (maxfd < channels[i].ch_err)
		maxfd = channels[i].ch_err;
	}
#  endif
    }

    return maxfd;
}

/*
 * The type of "rfds" is hidden to avoid problems with the function proto.
 */
    int
channel_select_check(int ret_in, void *rfds_in)
{
    int	    ret = ret_in;
    int	    i;
    fd_set  *rfds = rfds_in;

    for (i = 0; i < channel_count; ++i)
    {
	if (ret > 0 && channels[i].ch_sock >= 0
				       && FD_ISSET(channels[i].ch_sock, rfds))
	{
	    channel_read(i, FALSE, "channel_select_check");
	    --ret;
	}
#  ifdef CHANNEL_PIPES
	if (ret > 0 && channels[i].ch_out >= 0
				       && FD_ISSET(channels[i].ch_out, rfds))
	{
	    channel_read(i, FALSE, "channel_select_check");
	    --ret;
	}
	if (ret > 0 && channels[i].ch_err >= 0
				       && FD_ISSET(channels[i].ch_err, rfds))
	{
	    channel_read(i, TRUE, "channel_select_check");
	    --ret;
	}
#  endif
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
    int	    i;
    int	    ret = FALSE;

    for (i = 0; i < channel_count; ++i)
	while (may_invoke_callback(i) == OK)
	{
	    i = 0;  /* start over */
	    ret = TRUE;
	}
    return ret;
}

/*
 * Mark references to lists used in channels.
 */
    int
set_ref_in_channel(int copyID)
{
    int	    i;
    int	    abort = FALSE;

    for (i = 0; i < channel_count; ++i)
    {
	jsonq_T *head = &channels[i].ch_json_head;
	jsonq_T *item = head->next;

	while (item != head)
	{
	    list_T	*l = item->value->vval.v_list;

	    if (l->lv_copyID != copyID)
	    {
		l->lv_copyID = copyID;
		abort = abort || set_ref_in_list(l, copyID, NULL);
	    }
	    item = item->next;
	}
    }
    return abort;
}

/*
 * Return the mode of channel "ch_idx".
 * If "ch_idx" is invalid returns MODE_JSON.
 */
    ch_mode_T
channel_get_mode(int ch_idx)
{
    if (ch_idx < 0 || ch_idx >= channel_count)
	return MODE_JSON;
    return channels[ch_idx].ch_mode;
}

#endif /* FEAT_CHANNEL */
