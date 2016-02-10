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

/*
 * Change the zero to 1 to enable debugging.
 * This will write a file "channel_debug.log".
 */
#if 0
# define CHERROR(fmt, arg) cherror(fmt, arg)
# define CHLOG(idx, send, buf) chlog(idx, send, buf)
# define CHFILE "channel_debug.log"

static void cherror(char *fmt, char *arg);
static void chlog(int send, char_u *buf);
#else
# define CHERROR(fmt, arg)
# define CHLOG(idx, send, buf)
#endif

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

struct readqueue
{
    char_u		*buffer;
    struct readqueue	*next;
    struct readqueue	*prev;
};
typedef struct readqueue readq_T;

struct jsonqueue
{
    typval_T		*value;
    struct jsonqueue	*next;
    struct jsonqueue	*prev;
};
typedef struct jsonqueue jsonq_T;

struct cbqueue
{
    char_u		*callback;
    int			seq_nr;
    struct cbqueue	*next;
    struct cbqueue	*prev;
};
typedef struct cbqueue cbq_T;

typedef struct {
    sock_T    ch_fd;	/* the socket, -1 for a closed channel */
    int	      ch_idx;	/* used by channel_poll_setup() */
    readq_T   ch_head;	/* dummy node, header for circular queue */

    int	      ch_error;	/* When TRUE an error was reported.  Avoids giving
			 * pages full of error messages when the other side
			 * has exited, only mention the first error until the
			 * connection works again. */
#ifdef FEAT_GUI_X11
    XtInputId ch_inputHandler;  /* Cookie for input */
#endif
#ifdef FEAT_GUI_GTK
    gint      ch_inputHandler;	/* Cookie for input */
#endif
#ifdef WIN32
    int       ch_inputHandler;	/* simply ret.value of WSAAsyncSelect() */
#endif

    void      (*ch_close_cb)(void); /* callback for when channel is closed */

    int	      ch_block_id;	/* ID that channel_read_json_block() is
				   waiting for */
    char_u    *ch_callback;	/* function to call when a msg is not handled */
    cbq_T     ch_cb_head;	/* dummy node for pre-request callbacks */

    ch_mode_T ch_mode;
    jsonq_T   ch_json_head;	/* dummy node, header for circular queue */

    int       ch_timeout;	/* request timeout in msec */
} channel_T;

/*
 * Information about all channels.
 * There can be gaps for closed channels, they will be reused later.
 */
static channel_T *channels = NULL;
static int channel_count = 0;

/*
 * TODO: open debug file when desired.
 */
FILE *debugfd = NULL;

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
 * Add a new channel slot, return the index.
 * The channel isn't actually used into ch_fd is set >= 0;
 * Returns -1 if all channels are in use.
 */
    static int
add_channel(void)
{
    int		idx;
    channel_T	*ch;

    if (channels != NULL)
    {
	for (idx = 0; idx < channel_count; ++idx)
	    if (channels[idx].ch_fd < 0)
		/* re-use a closed channel slot */
		return idx;
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

    ch = &channels[channel_count];
    (void)vim_memset(ch, 0, sizeof(channel_T));

    ch->ch_fd = (sock_T)-1;
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
    channel_read((int)(long)clientData);
}
#endif

#ifdef FEAT_GUI_GTK
    static void
messageFromNetbeans(gpointer clientData,
		    gint unused1 UNUSED,
		    GdkInputCondition unused2 UNUSED)
{
    channel_read((int)(long)clientData);
}
#endif

    static void
channel_gui_register(int idx)
{
    channel_T	*channel = &channels[idx];

    if (!CH_HAS_GUI)
	return;

# ifdef FEAT_GUI_X11
    /* tell notifier we are interested in being called
     * when there is input on the editor connection socket
     */
    if (channel->ch_inputHandler == (XtInputId)NULL)
	channel->ch_inputHandler =
	    XtAppAddInput((XtAppContext)app_context, channel->ch_fd,
			 (XtPointer)(XtInputReadMask + XtInputExceptMask),
				   messageFromNetbeans, (XtPointer)(long)idx);
# else
#  ifdef FEAT_GUI_GTK
    /*
     * Tell gdk we are interested in being called when there
     * is input on the editor connection socket
     */
    if (channel->ch_inputHandler == 0)
	channel->ch_inputHandler =
	    gdk_input_add((gint)channel->ch_fd, (GdkInputCondition)
			     ((int)GDK_INPUT_READ + (int)GDK_INPUT_EXCEPTION),
				    messageFromNetbeans, (gpointer)(long)idx);
#  else
#   ifdef FEAT_GUI_W32
    /*
     * Tell Windows we are interested in receiving message when there
     * is input on the editor connection socket.
     */
    if (channel->ch_inputHandler == -1)
	channel->ch_inputHandler =
	    WSAAsyncSelect(channel->ch_fd, s_hwnd, WM_NETBEANS, FD_READ);
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
	if (channels[i].ch_fd >= 0)
	    channel_gui_register(i);
}

    static void
channel_gui_unregister(int idx)
{
    channel_T	*channel = &channels[idx];

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
	WSAAsyncSelect(channel->ch_fd, s_hwnd, 0, 0);
	channel->ch_inputHandler = -1;
    }
#   endif
#  endif
# endif
}

#endif

/*
 * Open a channel to "hostname":"port".
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
    int			idx;
    int			ret;

#ifdef WIN32
    channel_init_winsock();
#endif

    idx = add_channel();
    if (idx < 0)
    {
	CHERROR("All channels are in use\n", "");
	EMSG(_("E897: All channels are in use"));
	return -1;
    }

    if ((sd = (sock_T)socket(AF_INET, SOCK_STREAM, 0)) == (sock_T)-1)
    {
	CHERROR("error in socket() in channel_open()\n", "");
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
	CHERROR("error in gethostbyname() in channel_open()\n", "");
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
	    CHERROR("channel_open: Connect failed with errno %d\n", errno);
	    sock_close(sd);
	    return -1;
	}
    }

    /* Try connecting to the server. */
    ret = connect(sd, (struct sockaddr *)&server, sizeof(server));
    SOCK_ERRNO;
    if (ret < 0)
    {
	if (errno != EWOULDBLOCK && errno != EINPROGRESS)
	{
	    CHERROR("channel_open: Connect failed with errno %d\n", errno);
	    CHERROR("Cannot connect to port\n", "");
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
	    CHERROR("channel_open: Connect failed with errno %d\n", errno);
	    CHERROR("Cannot connect to port\n", "");
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
	    CHERROR("socket() retry in channel_open()\n", "");
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
		CHERROR("retrying...\n", "");
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
		CHERROR("Cannot connect to port after retry\n", "");
		PERROR(_("E899: Cannot connect to port after retry2"));
		sock_close(sd);
		return -1;
	    }
	}
    }

    channels[idx].ch_fd = sd;
    channels[idx].ch_close_cb = close_cb;

#ifdef FEAT_GUI
    channel_gui_register(idx);
#endif

    return idx;
}

/*
 * Set the json mode of channel "idx" to "ch_mode".
 */
    void
channel_set_json_mode(int idx, ch_mode_T ch_mode)
{
    channels[idx].ch_mode = ch_mode;
}

/*
 * Set the read timeout of channel "idx".
 */
    void
channel_set_timeout(int idx, int timeout)
{
    channels[idx].ch_timeout = timeout;
}

/*
 * Set the callback for channel "idx".
 */
    void
channel_set_callback(int idx, char_u *callback)
{
    vim_free(channels[idx].ch_callback);
    channels[idx].ch_callback = vim_strsave(callback);
}

/*
 * Set the callback for channel "idx" for the response with "id".
 */
    void
channel_set_req_callback(int idx, char_u *callback, int id)
{
    cbq_T *cbhead = &channels[idx].ch_cb_head;
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
 * Invoke the "callback" on channel "idx".
 */
    static void
invoke_callback(int idx, char_u *callback, typval_T *argv)
{
    typval_T	rettv;
    int		dummy;

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = idx;

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
channel_get(int idx)
{
    readq_T *head = &channels[idx].ch_head;
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
channel_get_all(int idx)
{
    /* Concatenate everything into one buffer.
     * TODO: avoid multiple allocations. */
    while (channel_collapse(idx) == OK)
	;
    return channel_get(idx);
}

/*
 * Collapses the first and second buffer in the channel "idx".
 * Returns FAIL if that is not possible.
 */
    int
channel_collapse(int idx)
{
    readq_T *head = &channels[idx].ch_head;
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
 * Execute a command received over channel "idx".
 * "cmd" is the command string, "arg2" the second argument.
 * "arg3" is the third argument, NULL if missing.
 */
    static void
channel_exe_cmd(int idx, char_u *cmd, typval_T *arg2, typval_T *arg3)
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
	    channel_T	*channel = &channels[idx];
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
		    channel_send(idx, json, "eval");
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
 * Invoke a callback for channel "idx" if needed.
 * Return OK when a message was handled, there might be another one.
 */
    static int
may_invoke_callback(int idx)
{
    char_u	*msg = NULL;
    typval_T	*listtv = NULL;
    list_T	*list;
    typval_T	*typetv;
    typval_T	argv[3];
    int		seq_nr = -1;
    channel_T	*channel = &channels[idx];
    ch_mode_T	ch_mode = channel->ch_mode;

    if (channel->ch_close_cb != NULL)
	/* this channel is handled elsewhere (netbeans) */
	return FALSE;

    if (ch_mode != MODE_RAW)
    {
	/* Get any json message in the queue. */
	if (channel_get_json(idx, -1, &listtv) == FAIL)
	{
	    /* Parse readahead, return when there is still no message. */
	    channel_parse_json(idx);
	    if (channel_get_json(idx, -1, &listtv) == FAIL)
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
	    channel_exe_cmd(idx, cmd, &argv[1], arg3);
	    clear_tv(listtv);
	    return TRUE;
	}

	if (typetv->v_type != VAR_NUMBER)
	{
	    /* TODO: give error */
	    clear_tv(listtv);
	    return FALSE;
	}
	seq_nr = typetv->vval.v_number;
    }
    else if (channel_peek(idx) == NULL)
    {
	/* nothing to read on raw channel */
	return FALSE;
    }
    else
    {
	/* For a raw channel we don't know where the message ends, just get
	 * everything. */
	msg = channel_get_all(idx);
	argv[1].v_type = VAR_STRING;
	argv[1].vval.v_string = msg;
    }

    if (seq_nr > 0)
    {
	cbq_T *cbhead = &channel->ch_cb_head;
	cbq_T *cbitem = cbhead->next;

	/* invoke the one-time callback with the matching nr */
	while (cbitem != cbhead)
	{
	    if (cbitem->seq_nr == seq_nr)
	    {
		invoke_callback(idx, cbitem->callback, argv);
		remove_cb_node(cbitem);
		break;
	    }
	    cbitem = cbitem->next;
	}
    }
    else if (channel->ch_callback != NULL)
    {
	/* invoke the channel callback */
	invoke_callback(idx, channel->ch_callback, argv);
    }
    /* else: drop the message TODO: give error */

    if (listtv != NULL)
	clear_tv(listtv);
    vim_free(msg);

    return TRUE;
}

/*
 * Return TRUE when channel "idx" is open.
 * Also returns FALSE or invalid "idx".
 */
    int
channel_is_open(int idx)
{
    return idx >= 0 && idx < channel_count && channels[idx].ch_fd >= 0;
}

/*
 * Close channel "idx".
 * This does not trigger the close callback.
 */
    void
channel_close(int idx)
{
    channel_T	*channel = &channels[idx];
    jsonq_T	*jhead;
    cbq_T	*cbhead;

    if (channel->ch_fd >= 0)
    {
	sock_close(channel->ch_fd);
	channel->ch_fd = -1;
	channel->ch_close_cb = NULL;
#ifdef FEAT_GUI
	channel_gui_unregister(idx);
#endif
	vim_free(channel->ch_callback);
	channel->ch_callback = NULL;
	channel->ch_timeout = 2000;

	while (channel_peek(idx) != NULL)
	    vim_free(channel_get(idx));

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
}

/*
 * Store "buf[len]" on channel "idx".
 * Returns OK or FAIL.
 */
    int
channel_save(int idx, char_u *buf, int len)
{
    readq_T *node;
    readq_T *head = &channels[idx].ch_head;

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

    if (debugfd != NULL)
    {
	fprintf(debugfd, "RECV on %d: ", idx);
	if (fwrite(buf, len, 1, debugfd) != 1)
	    return FAIL;
	fprintf(debugfd, "\n");
    }
    return OK;
}

/*
 * Return the first buffer from the channel without removing it.
 * Returns NULL if there is nothing.
 */
    char_u *
channel_peek(int idx)
{
    readq_T *head = &channels[idx].ch_head;

    if (head->next == head || head->next == NULL)
	return NULL;
    return head->next->buffer;
}

/*
 * Clear the read buffer on channel "idx".
 */
    void
channel_clear(int idx)
{
    readq_T *head = &channels[idx].ch_head;
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
channel_wait(int fd, int timeout)
{
#if defined(HAVE_SELECT) && !defined(FEAT_GUI_W32)
    struct timeval	tval;
    fd_set		rfds;
    int			ret;

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
	    return FAIL;
	break;
    }
#else
# ifdef HAVE_POLL
    struct pollfd	fds;

    fds.fd = fd;
    fds.events = POLLIN;
    if (poll(&fds, 1, timeout) <= 0)
	return FAIL;
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
 * Read from channel "idx" for as long as there is something to read.
 * The data is put in the read queue.
 */
    void
channel_read(int idx)
{
    static char_u	*buf = NULL;
    int			len = 0;
    int			readlen = 0;
    channel_T		*channel = &channels[idx];

    if (channel->ch_fd < 0)
    {
	CHLOG(idx, FALSE, "channel_read() called while socket is closed\n");
	return;
    }

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
	if (channel_wait(channel->ch_fd, 0) == FAIL)
	    break;
	len = sock_read(channel->ch_fd, buf, MAXMSGSIZE);
	if (len <= 0)
	    break;	/* error or nothing more to read */

	/* Store the read message in the queue. */
	channel_save(idx, buf, len);
	readlen += len;
	if (len < MAXMSGSIZE)
	    break;	/* did read everything that's available */
    }
#ifdef FEAT_GUI_W32
    if (len == SOCKET_ERROR)
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
	channel_save(idx, (char_u *)DETACH_MSG, (int)STRLEN(DETACH_MSG));

	channel_close(idx);
	if (channel->ch_close_cb != NULL)
	    (*channel->ch_close_cb)();

	if (len < 0)
	{
	    /* Todo: which channel? */
	    CHERROR("%s(): cannot from channel\n", "channel_read");
	    PERROR(_("E896: read from channel"));
	}
    }

#if defined(CH_HAS_GUI) && defined(FEAT_GUI_GTK)
    if (CH_HAS_GUI && gtk_main_level() > 0)
	gtk_main_quit();
#endif
}

/*
 * Read from raw channel "idx".  Blocks until there is something to read or
 * the timeout expires.
 * Returns what was read in allocated memory.
 * Returns NULL in case of error or timeout.
 */
    char_u *
channel_read_block(int idx)
{
    if (channel_peek(idx) == NULL)
    {
	/* Wait for up to the channel timeout. */
	if (channel_wait(channels[idx].ch_fd, channels[idx].ch_timeout) == FAIL)
	    return NULL;
	channel_read(idx);
    }

    return channel_get_all(idx);
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
	    if (channel->ch_fd < 0 || channel_wait(channel->ch_fd,
						 channel->ch_timeout) == FAIL)
		break;
	    channel_read(ch_idx);
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
channel_socket2idx(sock_T fd)
{
    int i;

    if (fd >= 0)
	for (i = 0; i < channel_count; ++i)
	    if (channels[i].ch_fd == fd)
		return i;
    return -1;
}
# endif

/*
 * Write "buf" (NUL terminated string) to channel "idx".
 * When "fun" is not NULL an error message might be given.
 * Return FAIL or OK.
 */
    int
channel_send(int idx, char_u *buf, char *fun)
{
    channel_T	*channel = &channels[idx];
    int		len = (int)STRLEN(buf);

    if (channel->ch_fd < 0)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    CHERROR("    %s(): write while not connected\n", fun);
	    EMSG2("E630: %s(): write while not connected", fun);
	}
	channel->ch_error = TRUE;
	return FAIL;
    }

    if (sock_write(channel->ch_fd, buf, len) != len)
    {
	if (!channel->ch_error && fun != NULL)
	{
	    CHERROR("    %s(): write failed\n", fun);
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
	if (channels[i].ch_fd >= 0)
	{
	    channels[i].ch_idx = nfd;
	    fds[nfd].fd = channels[i].ch_fd;
	    fds[nfd].events = POLLIN;
	    nfd++;
	}
	else
	    channels[i].ch_idx = -1;

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
	if (ret > 0 && channels[i].ch_idx != -1
				 && fds[channels[i].ch_idx].revents & POLLIN)
	{
	    channel_read(i);
	    --ret;
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
	if (channels[i].ch_fd >= 0)
	{
	    FD_SET(channels[i].ch_fd, rfds);
	    if (maxfd < channels[i].ch_fd)
		maxfd = channels[i].ch_fd;
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
	if (ret > 0 && channels[i].ch_fd >= 0
				       && FD_ISSET(channels[i].ch_fd, rfds))
	{
	    channel_read(i);
	    --ret;
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
 * Return the mode of channel "idx".
 * If "idx" is invalid returns MODE_JSON.
 */
    ch_mode_T
channel_get_mode(int idx)
{
    if (idx < 0 || idx >= channel_count)
	return MODE_JSON;
    return channels[idx].ch_mode;
}

#endif /* FEAT_CHANNEL */
