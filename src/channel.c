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
typedef struct readqueue queue_T;

typedef struct {
    sock_T    ch_fd;	/* the socket, -1 for a closed channel */
    int	      ch_idx;	/* used by channel_poll_setup() */
    queue_T   ch_head;	/* dummy node, header for circular queue */

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

    char_u    *ch_callback;	/* function to call when a msg is not handled */
    char_u    *ch_req_callback;	/* function to call for current request */

    int	      ch_json_mode;
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

/*
 * Add a new channel slot, return the index.
 * The channel isn't actually used into ch_fd is set >= 0;
 * Returns -1 if all channels are in use.
 */
    static int
add_channel(void)
{
    int		idx;
    channel_T	*new_channels;

    if (channels != NULL)
	for (idx = 0; idx < channel_count; ++idx)
	    if (channels[idx].ch_fd < 0)
		/* re-use a closed channel slot */
		return idx;
    if (channel_count == MAX_OPEN_CHANNELS)
	return -1;
    new_channels = (channel_T *)alloc(sizeof(channel_T) * (channel_count + 1));
    if (new_channels == NULL)
	return -1;
    if (channels != NULL)
	mch_memmove(new_channels, channels, sizeof(channel_T) * channel_count);
    channels = new_channels;
    (void)vim_memset(&channels[channel_count], 0, sizeof(channel_T));

    channels[channel_count].ch_fd = (sock_T)-1;
#ifdef FEAT_GUI_X11
    channels[channel_count].ch_inputHandler = (XtInputId)NULL;
#endif
#ifdef FEAT_GUI_GTK
    channels[channel_count].ch_inputHandler = 0;
#endif
#ifdef FEAT_GUI_W32
    channels[channel_count].ch_inputHandler = -1;
#endif

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
channel_open(char *hostname, int port_in, void (*close_cb)(void))
{
    int			sd;
    struct sockaddr_in	server;
    struct hostent *	host;
#ifdef WIN32
    u_short		port = port_in;
#else
    int			port = port_in;
#endif
    int			idx;

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

    /* Connect to server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)))
    {
	SOCK_ERRNO;
	CHERROR("channel_open: Connect failed with errno %d\n", errno);
	if (errno == ECONNREFUSED)
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
	else
	{
	    CHERROR("Cannot connect to port\n", "");
	    PERROR(_("E902: Cannot connect to port"));
	    sock_close(sd);
	    return -1;
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
 * Set the json mode of channel "idx" to TRUE or FALSE.
 */
    void
channel_set_json_mode(int idx, int json_mode)
{
    channels[idx].ch_json_mode = json_mode;
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
 * Set the callback for channel "idx" for the next response.
 */
    void
channel_set_req_callback(int idx, char_u *callback)
{
    vim_free(channels[idx].ch_req_callback);
    channels[idx].ch_req_callback = callback == NULL
					       ? NULL : vim_strsave(callback);
}

/*
 * Decode JSON "msg", which must have the form "[expr1, expr2, expr3]".
 * Put "expr1" in "tv1".
 * Put "expr2" in "tv2".
 * Put "expr3" in "tv3". If "tv3" is NULL there is no "expr3".
 *
 * Return OK or FAIL.
 */
    int
channel_decode_json(char_u *msg, typval_T *tv1, typval_T *tv2, typval_T *tv3)
{
    js_read_T	reader;
    typval_T	listtv;

    reader.js_buf = msg;
    reader.js_eof = TRUE;
    reader.js_used = 0;
    json_decode(&reader, &listtv);

    if (listtv.v_type == VAR_LIST)
    {
	list_T *list = listtv.vval.v_list;

	if (list->lv_len == 2 || (tv3 != NULL && list->lv_len == 3))
	{
	    /* Move the item from the list and then change the type to avoid the
	     * item being freed. */
	    *tv1 = list->lv_first->li_tv;
	    list->lv_first->li_tv.v_type = VAR_NUMBER;
	    *tv2 = list->lv_first->li_next->li_tv;
	    list->lv_first->li_next->li_tv.v_type = VAR_NUMBER;
	    if (tv3 != NULL)
	    {
		if (list->lv_len == 3)
		{
		    *tv3 = list->lv_last->li_tv;
		    list->lv_last->li_tv.v_type = VAR_NUMBER;
		}
		else
		    tv3->v_type = VAR_UNKNOWN;
	    }
	    list_unref(list);
	    return OK;
	}
    }

    /* give error message? */
    clear_tv(&listtv);
    return FAIL;
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

	if (is_eval && arg3->v_type != VAR_NUMBER)
	{
	    if (p_verbose > 2)
		EMSG("E904: third argument for eval must be a number");
	}
	else
	{
	    typval_T	*tv = eval_expr(arg, NULL);
	    typval_T	err_tv;
	    char_u	*json;

	    if (is_eval)
	    {
		if (tv == NULL)
		{
		    err_tv.v_type = VAR_STRING;
		    err_tv.vval.v_string = (char_u *)"ERROR";
		    tv = &err_tv;
		}
		json = json_encode_nr_expr(arg3->vval.v_number, tv);
		channel_send(idx, json, "eval");
		vim_free(json);
	    }
	    free_tv(tv);
	}
    }
    else if (p_verbose > 2)
	EMSG2("E905: received unknown command: %s", cmd);
}

/*
 * Invoke a callback for channel "idx" if needed.
 */
    static void
may_invoke_callback(int idx)
{
    char_u	*msg;
    typval_T	typetv;
    typval_T	argv[3];
    typval_T	arg3;
    char_u	*cmd = NULL;
    int		seq_nr = -1;
    int		ret = OK;

    if (channel_peek(idx) == NULL)
	return;

    /* Concatenate everything into one buffer.
     * TODO: only read what the callback will use.
     * TODO: avoid multiple allocations. */
    while (channel_collapse(idx) == OK)
	;
    msg = channel_get(idx);

    if (channels[idx].ch_json_mode)
    {
	ret = channel_decode_json(msg, &typetv, &argv[1], &arg3);
	if (ret == OK)
	{
	    /* TODO: error if arg3 is set when it shouldn't? */
	    if (typetv.v_type == VAR_STRING)
		cmd = typetv.vval.v_string;
	    else if (typetv.v_type == VAR_NUMBER)
		seq_nr = typetv.vval.v_number;
	}
    }
    else
    {
	argv[1].v_type = VAR_STRING;
	argv[1].vval.v_string = msg;
    }

    if (ret == OK)
    {
	if (cmd != NULL)
	{
	    channel_exe_cmd(idx, cmd, &argv[1], &arg3);
	}
	else if (channels[idx].ch_req_callback != NULL && seq_nr != 0)
	{
	    /* TODO: check the sequence number */
	    /* invoke the one-time callback */
	    invoke_callback(idx, channels[idx].ch_req_callback, argv);
	    channels[idx].ch_req_callback = NULL;
	}
	else if (channels[idx].ch_callback != NULL)
	{
	    /* invoke the channel callback */
	    invoke_callback(idx, channels[idx].ch_callback, argv);
	}
	/* else: drop the message */

	if (channels[idx].ch_json_mode)
	{
	    clear_tv(&typetv);
	    clear_tv(&argv[1]);
	    clear_tv(&arg3);
	}
    }

    vim_free(msg);
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
    channel_T		*channel = &channels[idx];

    if (channel->ch_fd >= 0)
    {
	sock_close(channel->ch_fd);
	channel->ch_fd = -1;
#ifdef FEAT_GUI
	channel_gui_unregister(idx);
#endif
	vim_free(channel->ch_callback);
	channel->ch_callback = NULL;
    }
}

/*
 * Store "buf[len]" on channel "idx".
 * Returns OK or FAIL.
 */
    int
channel_save(int idx, char_u *buf, int len)
{
    queue_T *node;
    queue_T *head = &channels[idx].ch_head;

    node = (queue_T *)alloc(sizeof(queue_T));
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

    if (head->next == NULL)   /* initialize circular queue */
    {
	head->next = head;
	head->prev = head;
    }

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
    queue_T *head = &channels[idx].ch_head;

    if (head->next == head || head->next == NULL)
	return NULL;
    return head->next->buffer;
}

/*
 * Return the first buffer from the channel and remove it.
 * The caller must free it.
 * Returns NULL if there is nothing.
 */
    char_u *
channel_get(int idx)
{
    queue_T *head = &channels[idx].ch_head;
    queue_T *node;
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
 * Collapses the first and second buffer in the channel "idx".
 * Returns FAIL if that is not possible.
 */
    int
channel_collapse(int idx)
{
    queue_T *head = &channels[idx].ch_head;
    queue_T *node = head->next;
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
 * Clear the read buffer on channel "idx".
 */
    void
channel_clear(int idx)
{
    queue_T *head = &channels[idx].ch_head;
    queue_T *node = head->next;
    queue_T *next;

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
 */
    static int
channel_wait(int fd, int timeout)
{
#ifdef HAVE_SELECT
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
 * Read from channel "idx".  Blocks until there is something to read or the
 * timeout expires.
 * Returns what was read in allocated memory.
 * Returns NULL in case of error or timeout.
 */
    char_u *
channel_read_block(int idx)
{
    if (channel_peek(idx) == NULL)
    {
	/* Wait for up to 2 seconds.
	 * TODO: use timeout set on the channel. */
	if (channel_wait(channels[idx].ch_fd, 2000) == FAIL)
	    return NULL;
	channel_read(idx);
    }

    /* Concatenate everything into one buffer.
     * TODO: avoid multiple allocations. */
    while (channel_collapse(idx) == OK)
	;

    return channel_get(idx);
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
 * Invoked from the main loop when it's save to execute received commands.
 */
    void
channel_parse_messages(void)
{
    int	    i;

    for (i = 0; i < channel_count; ++i)
	may_invoke_callback(i);
}

#endif /* FEAT_CHANNEL */
