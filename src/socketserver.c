/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * socketserver.c: Socketserver clientserver functionality
 */

#include "vim.h"

#ifdef FEAT_SOCKETSERVER

/*
 * Any message sent to a Vim server or received from a Vim server is a simple
 * JSON object with the following fields:
 * {
 *   "type": "expr"|"keystrokes"|"notify"|"reply" -- The type of message
 *
 *   ?"enc": string -- Encoding to be used, default to 'encoding'
 *
 *   "str": string -- What to execute for the command or contents of result
 *
 *   ?"code": number -- Return code for expression
 *
 *   ?"sender": string -- Address of client that sent command if it is a server.
 * }
 */

/*
 * Represents a reply from a server2client call. Each client that calls a
 * server2client call to us has its own ss_reply_T. Each time a client sends
 * data using server2client, Vim creates a ss_reply_T if it doesn't exist and
 * adds the string to the array. When remote_read is called, the server id is
 * used to find the specific ss_reply_T, and a single string is popped from the
 * array.
 */
typedef struct
{
    char_u *sender;
    garray_T strings;
} ss_reply_T;

static channel_T    *server_channel = NULL;
static char_u	    *server_address = NULL;
static bool	    server_is_unix = false;
static garray_T	    server_replies;

static channel_T    *client_channels = NULL;

#define FOR_ALL_CLIENTS(ch) \
    for (ch = client_channels; ch != NULL; ch = ch->ch_ss_next)

static void	    socketserver_cleanup(void);
static char_u	    *socketserver_get_path_from_name(char_u *name);
static void	    socketserver_accept(channel_T *channel);
static void	    socketserver_close(channel_T *channel);
static ss_reply_T   *socketserver_add_reply(char_u *sender);

/*
 * Start the socketserver using the given name. Returns OK on success and FAIL
 * on failure.
 */
    int
socketserver_start(char_u *name, bool quiet)
{
    char_u	*address;
    int	    	port;
    bool	is_unix = false;
    channel_T	*channel;

    if (server_channel != NULL)
	return OK;

    if (channel_parse_address(name, (char *)address_buf, ADDRESS_BUFSIZE,
		&port, &is_unix, true, quiet) == FAIL)
	return FAIL;
    address = address_buf;

    if (is_unix)
	channel = channel_listen_unix((char *)address, NULL, false);
    else
	channel = channel_listen((char *)address, port, NULL);

    if (channel == NULL)
	return FAIL;

    channel->ch_socketserver = true;
    channel->ch_ss_accept_cb = socketserver_accept;
    channel->ch_ss_close_cb = socketserver_close;

    server_channel = channel;
    server_address = vim_strsave(name);
    server_is_unix = is_unix;

    vim_free(serverName);
    serverName = vim_strsave(name);
# ifdef FEAT_EVAL
    set_vim_var_string(VV_SEND_SERVER, serverName, -1);
# endif

    ga_init2(&server_replies, sizeof(ss_reply_T), 2);

    ch_log(NULL, "socketserver: started server at %s", name);

    return OK;
}

/*
 * Stop running the socketserver if it is. Note that this does not stop Vim from
 * becoming a client.
 */
    void
socketserver_stop(void)
{
    if (server_channel == NULL)
	return;

    channel_close(server_channel, false);
    channel_clear(server_channel);

    if (server_is_unix)
    {
	char_u *path = server_address;

	if (STRNCMP(path, "unix:", 5) == 0)
	    path += 5;
	mch_remove(path);
    }
    
    socketserver_cleanup();

    ch_log(NULL, "socketserver: shutting down server");
}

    static void
socketserver_cleanup(void)
{
    server_channel = NULL;
    vim_free(server_address);
    server_address = NULL;

    // Free all replies
    for (int i = 0; i < server_replies.ga_len; i++)
    {
	ss_reply_T *reply = (ss_reply_T *)server_replies.ga_data + i;

	vim_free(reply->sender);
	ga_clear_strings(&reply->strings);
    }
    ga_clear(&server_replies);
}

/*
 * If "name" is a pathless name such as "VIM", search known directories for the
 * socket named "name", and return the alloc'ed path to it. If "name" starts
 * with a '/', './' or '../', then a copy of "name" is returned. Returns NULL
 * on failure or if no socket was found.
 */
    static char_u *
socketserver_get_path_from_name(char_u *name)
{
    char_u	    *buf;
    const char_u    *known_dirs[] = {
	mch_getenv("XDG_RUNTIME_DIR"),
	mch_getenv("TMPDIR"),
	(char_u *)"/tmp"
    };

    if (name == NULL)
	return NULL;

    // Ignore if name is a path
    if (name[0] == '/' || STRNCMP(name, "./", 2) == 0 ||
	    STRNCMP(name, "../", 3) == 0)
	return vim_strsave(name);

    buf = alloc(MAXPATHL);

    if (buf == NULL)
	return NULL;

    for (size_t i = 0; i < ARRAY_LENGTH(known_dirs); i++)
    {
	const char_u *dir = known_dirs[i];

	if (dir == NULL)
	    continue;
	else if (STRCMP(dir, "/tmp") == 0 ||
		(known_dirs[1] != NULL && STRCMP(dir, known_dirs[1]) == 0))
	    vim_snprintf((char *)buf, MAXPATHL, "%s/vim-%lu/%s", dir,
		    (unsigned long int)getuid(), name);
	else
	    vim_snprintf((char *)buf, MAXPATHL, "%s/vim/%s", dir, name);

	if (mch_access((char *)buf, F_OK) != 0)
	{
	    if (server_address != NULL && STRCMP(buf, server_address) == 0)
		// Can't connect to itself
		break;
	    return buf;
	}
    }

    vim_free(buf);
    return NULL;
}

/*
 * Callback for when client channel is closed
 */
    static void
socketserver_client_close(channel_T *channel)
{
    if (channel == client_channels)
	client_channels = channel->ch_ss_next;
    if (channel->ch_ss_prev != NULL)
	channel->ch_ss_prev->ch_ss_next = channel->ch_ss_next;
    if (channel->ch_ss_next != NULL)
	channel->ch_ss_next->ch_ss_prev = channel->ch_ss_prev;

    ch_log(NULL, "socketserver: client channel closed");
}

/*
 * Callback for when server channel accepted new client.
 */
    static void
socketserver_accept(channel_T *channel)
{
    channel->ch_socketserver = true;
    channel->ch_ss_close_cb = socketserver_client_close;

    channel->ch_ss_next = client_channels;
    channel->ch_ss_prev = NULL;
    client_channels = channel;

    // We will read the command from the client later in the input loop.
    ch_log(NULL, "socketserver: accepted new client");
}

/*
 * Callback for when server channel is closed
 */
    static void
socketserver_close(channel_T *channel)
{
    socketserver_cleanup();

    ch_log(NULL, "socketserver: server channel closed");
}

/*
 * Mark references to socketserver channels
 */
    int
set_ref_in_socketserver_channel(int copyID)
{
    bool	abort = false;
    channel_T	*channel;
    typval_T	tv;

    tv.v_type = VAR_CHANNEL;
    tv.vval.v_channel = server_channel;
    abort = abort || set_ref_in_item(&tv, copyID, NULL, NULL, NULL);

    FOR_ALL_CLIENTS(channel)
    {
	if (abort)
	    break;

	tv.v_type = VAR_CHANNEL;
	tv.vval.v_channel = channel;
	abort = abort || set_ref_in_item(&tv, copyID, NULL, NULL, NULL);
    }
    return abort;
}

/*
 * Execute the JSON message represented by "dict".
 */
    static void
socketserver_exec(channel_T *channel, dict_T *message)
{
    dictitem_T	*di;
    char_u	*type;
    char_u	*enc = p_enc;
    char_u	*str = NULL;
    char_u	*sender = NULL;
    char_u	*to_free;
    char_u	*to_free2;

    di = dict_find(message, (char_u *)"type", -1);
    if (di == NULL || di->di_tv.v_type != VAR_STRING)
	return;
    else
	type = di->di_tv.vval.v_string;

    di = dict_find(message, (char_u *)"enc", -1);
    if (di != NULL && di->di_tv.v_type == VAR_STRING)
	enc = di->di_tv.vval.v_string;

    di = dict_find(message, (char_u *)"str", -1);
    if (di != NULL && di->di_tv.v_type == VAR_STRING)
	str = di->di_tv.vval.v_string;

    di = dict_find(message, (char_u *)"sender", -1);
    if (di != NULL && di->di_tv.v_type == VAR_STRING)
    {
	sender = di->di_tv.vval.v_string;

	// Save in global
	vim_free(client_socket);
	client_socket = vim_strsave(sender);
    }

    ch_log(NULL, "socketserver_exec(): encoding: %s, result: %s",
	    enc, str == NULL ? (char_u *)"(null)" : str);

    if (STRCMP(type, "expr") == 0)
    {
	// Evaluate expression and send back reply
	typval_T    tv;
	dict_T	    *dict;
	char_u	    *result;
	int	    code;
	char_u	    *buf;

	dict = dict_alloc();
	if (dict == NULL)
	    return;

	str = serverConvert(enc, str, &to_free);
	result = eval_client_expr_to_string(str);
	code = result == NULL ? -1 : 0;

	dict_add_string(dict, "type", (char_u *)"reply");
	if (result != NULL)
	{
	    dict_add_string(dict, "str", result);
	    vim_free(result);
	}
	else
	    // Error occured, return error message
	    dict_add_string(dict, "str",
		    (char_u *)_(e_invalid_expression_received));

	dict_add_number(dict, "code", code);
	dict_add_string(dict, "enc", p_enc);

	tv.v_type = VAR_DICT;
	tv.vval.v_dict = dict;

	buf = json_encode(&tv, JSON_NL);

	if (buf != NULL)
	{
	    emsg_silent++;
	    channel_send(channel, PART_SOCK, buf, STRLEN(buf),
		    "socketserver_exec");
	    emsg_silent--;
	}
	vim_free(buf);

	dict_unref(dict);
	vim_free(to_free);
    }
    else if (STRCMP(type, "keystrokes") == 0)
    {
	// Execute keystrokes
	str = serverConvert(enc, str, &to_free);
	server_to_input_buf(str);
	vim_free(to_free);
    }
    else if (STRCMP(type, "notify") == 0)
    {
	// Notification, execute autocommands and save the reply for later use
	if (sender != NULL && str != NULL && enc != NULL)
	{
	    ss_reply_T *reply;

	    str = serverConvert(enc, str, &to_free);
	    sender = serverConvert(enc, sender, &to_free2);

	    reply = socketserver_add_reply(sender);

	    if (reply != NULL)
		ga_copy_string(&reply->strings, str);

	    apply_autocmds(EVENT_REMOTEREPLY, sender, str, TRUE, curbuf);

	    vim_free(to_free);
	    vim_free(to_free2);
	}
    }
    else
	ch_error(NULL, "socketserver: unknown command type '%s'", type);
}

    static int
socketserver_get_message(channel_T *channel, typval_T **tv)
{
    jsonq_T *head = &channel->ch_part[PART_SOCK].ch_json_head;
    jsonq_T *json_msg = head->jq_next;

    if (json_msg == NULL)
    {
	// Check the readahead buffer
	channel_parse_json(channel, PART_SOCK, true);
	json_msg = head->jq_next;
    }
    if (json_msg == NULL)
	return FAIL;
    *tv = json_msg->jq_value;
    remove_json_node(head, json_msg);

    if ((*tv)->v_type != VAR_DICT)
    {
	ch_error(NULL, "socketserver: message is not a JSON object");
	return FAIL;
    }
    return OK;
}

/*
 * Parse any commands in the queue and execute them.
 */
    void
socketserver_parse_messages(void)
{
    channel_T	*channel;
    typval_T	*tv;
    
    FOR_ALL_CLIENTS(channel)
    {
	// Get the JSON message if there is any from the queue for this channel.
	if (socketserver_get_message(channel, &tv) == FAIL)
	    continue;

	socketserver_exec(channel, tv->vval.v_dict);
	free_tv(tv);
    }
}


/*
 * Poll until there is something to read on "channel". Also handle other
 * socketserver channels in the meantime. If "channel" is NULL, then poll all
 * channels once then exit.
 *
 * Return OK on success and FAIL on failure or timeout.
 */
    static int
socketserver_wait(channel_T *channel, int timeout)
{
    while (true)
    {
	int		ret;
	channel_T	*ch;
#ifdef HAVE_SELECT
	fd_set		rfds;
	struct timeval  tv;
	int		maxfd;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	FD_ZERO(&rfds);

	if (channel != NULL)
	{
	    if (channel->CH_SOCK_FD == INVALID_FD)
		// Shouldn't happen
		return FAIL;

	    maxfd = channel->CH_SOCK_FD;
	    FD_SET(channel->CH_SOCK_FD, &rfds);
	}
	if (server_channel != NULL && server_channel->CH_SOCK_FD != INVALID_FD)
	{
	    FD_SET(server_channel->CH_SOCK_FD, &rfds);
	    if (server_channel->CH_SOCK_FD > maxfd)
		maxfd = server_channel->CH_SOCK_FD;
	}

	FOR_ALL_CLIENTS(ch)
	{
	    if (ch->CH_SOCK_FD != INVALID_FD)
	    {
		FD_SET(ch->CH_SOCK_FD, &rfds);
		if (maxfd < (int)ch->CH_SOCK_FD)
		    maxfd = (int)ch->CH_SOCK_FD;
	    }
	}

	ret = select(maxfd + 1, &rfds, NULL, NULL, &tv);

# ifdef EINTR
	if (ret == -1 && errno == EINTR)
	    continue;
# endif

	if (ret > 0)
	{
	    if (server_channel != NULL
		    && server_channel->CH_SOCK_FD != INVALID_FD
		    && FD_ISSET(server_channel->CH_SOCK_FD, &rfds))
		channel_check(server_channel, PART_SOCK);

	    FOR_ALL_CLIENTS(ch)
		if (ch->CH_SOCK_FD != INVALID_FD
			&& FD_ISSET(ch->CH_SOCK_FD, &rfds))
		    channel_check(ch, PART_SOCK);

	    socketserver_parse_messages();

	    if (channel == NULL)
		return OK;

	    if (channel->CH_SOCK_FD != INVALID_FD
		    && FD_ISSET(channel->CH_SOCK_FD, &rfds))
	    {
		channel_check(channel, PART_SOCK);
		return OK;
	    }
	    continue;
	}
#else
	struct pollfd   fds[MAX_OPEN_CHANNELS + 1];
	int		nfd = 0;

	if (channel != NULL)
	{
	    if (channel->CH_SOCK_FD == INVALID_FD)
		// Shouldn't happen
		return FAIL;

	    fds[nfd].fd = channel->CH_SOCK_FD;
	    fds[nfd++].fd = POLLIN;
	}
	if (server_channel != NULL && server_channel->CH_SOCK_FD != INVALID_FD)
	{
	    fds[nfd].fd = server_channel->CH_SOCK_FD;
	    fds[nfd++].fd = POLLIN;
	}

	FOR_ALL_CLIENTS(ch)
	    if (ch->CH_SOCK_FD != INVALID_FD)
	    {
		fds[nfd].fd = channel->CH_SOCK_FD;
		fds[nfd].fd = POLLIN;
		ch->ch_part[PART_SOCK].ch_poll_idx = nfd;
		nfd++;
	    }

	ret = poll(fds, nfd, timeout);

# ifdef EINTR
	if (ret == -1 && errno == EINTR)
	    continue;
# endif

	if (ret > 0)
	{
	    if (server_channel != NULL
		    && server_channel->CH_SOCK_FD != INVALID_FD
		    && fds[1].revents & POLLIN)
		channel_check(server_channel, PART_SOCK);

	    FOR_ALL_CLIENTS(ch)
		if (ch->CH_SOCK_FD != INVALID_FD
			&& fds[ch->ch_part[PART_SOCK].ch_poll_idx]
			.revents & POLLIN)
		    channel_check(ch, PART_SOCK);

	    socketserver_parse_messages();

	    if (channel == NULL)
		return OK;

	    if (channel->CH_SOCK_FD != INVALID_FD
		    && fds[0].revents & POLLIN)
	    {
		channel_check(channel, PART_SOCK);
		return OK;
	    }
	    continue;
	}
#endif
	break;
    }
    return FAIL;
}

    static channel_T *
socketserver_get_channel(char_u *name)
{
    int		port;
    bool	is_unix;
    channel_T	*channel;

    if (channel_parse_address(name, (char *)address_buf,
		ADDRESS_BUFSIZE, &port, &is_unix, false, false) == FAIL)
	return NULL;

    if (is_unix)
	channel = channel_open_unix((char *)address_buf, NULL);
    else
	channel =  channel_open((char *)address_buf, port, 1000, NULL);

    if (channel == NULL)
	semsg(_(e_socket_server_failed_connecting), name);

    return channel;
}

/*
 * Send command to address "name". Returns 0 for OK, -1 on error.
 */
    int
socketserver_send(
	char_u *name,
        char_u *str,
	char_u **result,
	bool is_expr,
	int timeout,
	bool silent)
{
    int		rcode = -1;
    channel_T	*channel;
    dict_T	*dict;
    dictitem_T	*di;
    typval_T	tv;
    typval_T	*resp_tv = NULL;
    char_u	*buf;

    // Execute locally if target is ourselves
    if (serverName != NULL && STRICMP(name, serverName) == 0)
	return sendToLocalVim(str, is_expr, result);

    channel = socketserver_get_channel(name);
    if (channel == NULL)
	return -1;

    dict = dict_alloc();
    if (dict == NULL)
	goto exit;

    dict_add_string(dict, "type", (char_u *)(is_expr ? "expr" : "keystrokes"));
    dict_add_string(dict, "enc", p_enc);
    dict_add_string(dict, "str", str);

    // Tell server who we are so it can save our socket path internally for
    // later use with server2client. Only do this if we are actually a server.
    if (server_address != NULL)
	dict_add_string(dict, "sender", server_address);

    tv.v_type = VAR_DICT;
    tv.vval.v_dict = dict;

    buf = json_encode(&tv, JSON_NL);
    if (buf == NULL
	    || channel_send(channel, PART_SOCK, buf, STRLEN(buf),
		"socketserver_send") == FAIL)
    {
	dict_unref(dict);
	vim_free(buf);
	goto exit;
    }
    vim_free(buf);
    dict_unref(dict);

    if (!is_expr)
	// Exit, we aren't waiting for a response
	return 0;

    if (timeout == 0)
	timeout = 1000;

    // To handle recursive calls, we must handle any socketserver channels as
    // well.
    while (socketserver_wait(channel, timeout) == OK)
	if (socketserver_get_message(channel, &resp_tv) == OK)
	    break;

    if (resp_tv == NULL)
	return -1;

    dict = resp_tv->vval.v_dict;

    di = dict_find(dict, (char_u *)"type", -1);
    if (di == NULL || di->di_tv.v_type != VAR_STRING ||
	    STRCMP(di->di_tv.vval.v_string, "reply") != 0)
    {
	ch_error(NULL, "socketserver: unknown reply type");
	goto exit;
    }

    if (result != NULL)
    {
	di = dict_find(dict, (char_u *)"str", -1);
	if (di != NULL && di->di_tv.v_type == VAR_STRING)
	    *result = vim_strsave(di->di_tv.vval.v_string);
	else
	{
	    free_tv(resp_tv);
	    goto exit;
	}
    }

    di = dict_find(dict, (char_u *)"code", -1);
    if (di != NULL && di->di_tv.v_type == VAR_NUMBER)
	rcode = di->di_tv.vval.v_number;

    free_tv(resp_tv);

exit:
    channel_close(channel, false);
    channel_clear(channel);
    return rcode;
}

   static ss_reply_T *
socketserver_get_reply(char_u *sender, int *index)
{
    for (int i = 0; i < server_replies.ga_len; i++)
    {
	ss_reply_T *reply = ((ss_reply_T *)server_replies.ga_data) + i;

	if (STRCMP(reply->sender, sender) == 0)
	{
	    if (index != NULL)
		*index = i;
	    return reply;
	}
    }
    return NULL;
}

/*
 * Add reply to list of replies. Returns a pointer to the ss_reply_T that was
 * initialized or was found.
 */
    static ss_reply_T *
socketserver_add_reply(char_u *sender)
{
    ss_reply_T *reply;

    if (server_replies.ga_growsize == 0)
	ga_init2(&server_replies, sizeof(ss_reply_T), 1);

    reply = socketserver_get_reply(sender, NULL);

    if (reply == NULL && ga_grow(&server_replies, 1) == OK)
    {
	reply = ((ss_reply_T *)server_replies.ga_data) + server_replies.ga_len++;

	reply->sender = vim_strsave(sender);

	if (reply->sender == NULL)
	    return NULL;

	ga_init2(&reply->strings, sizeof(char_u *), 5);
    }

    return reply;
}

    static void
socketserver_remove_reply(char_u *sender)
{
    int index;
    ss_reply_T *reply = socketserver_get_reply(sender, &index);

    if (reply != NULL)
    {
	ss_reply_T  *arr = server_replies.ga_data;
	int	    len	 = server_replies.ga_len;

	// Free strings
	vim_free(reply->sender);
	ga_clear_strings(&reply->strings);

	// Move all elements after the removed reply forward by one
	if (len > 1)
	    mch_memmove(arr + index, arr + index + 1,
		    sizeof(ss_reply_T) * (len - index));
	reply->strings.ga_len--;
    }
}

/*
 * Send a string to "client" as a reply (notification). Returns OK on success
 * and FAIL on failure.
 */
    int
socketserver_send_reply(char_u *client, char_u *str)
{
    dict_T	*dict;
    channel_T	*channel;
    typval_T	tv;
    char_u	*buf;
    int		ret = OK;

    if (server_channel == NULL || server_address == NULL)
    {
	emsg(_(e_socket_server_not_online));
	return FAIL;
    }

    channel = socketserver_get_channel(client);
    if (channel == NULL)
	return FAIL;

    dict = dict_alloc();
    if (dict == NULL)
    {
	ret = FAIL;
	goto exit;
    }

    dict_add_string(dict, "type", (char_u *)"notify");
    dict_add_string(dict, "enc", p_enc);
    dict_add_string(dict, "str", str);
    dict_add_string(dict, "sender", str);

    tv.v_type = VAR_DICT;
    tv.vval.v_dict = dict;

    buf = json_encode(&tv, JSON_NL);
    if (buf == NULL
	    || channel_send(channel, PART_SOCK, buf, STRLEN(buf),
		"socketserver_send") == FAIL)
	ret = FAIL;

    vim_free(buf);
    dict_unref(dict);

exit:
    channel_close(channel, false);
    channel_clear(channel);

    return ret;
}

/*
 * Wait for reply from "client" and place result in "str". Returns OK on success
 * and FAIL on failure. Timeout is in milliseconds
 */
    int
socketserver_read_reply(char_u *client, char_u **str, int timeout)
{
    ss_reply_T *reply = NULL;

    if (server_channel == NULL || server_address == NULL)
    {
	emsg(_(e_socket_server_not_online));
	return FAIL;
    }

    while (true)
    {
	reply = socketserver_get_reply(client, NULL);
	if (reply != NULL)
	    break;
	socketserver_wait(NULL, timeout);
    }

    // Consume the string
    *str = ((char_u **)reply->strings.ga_data)[0];

    if (reply->strings.ga_len > 1)
	mch_memmove((char_u **)reply->strings.ga_data,
		((char_u **)reply->strings.ga_data) + 1,
		sizeof(ss_reply_T) * (reply->strings.ga_len - 1));
    reply->strings.ga_len--;

    if (reply->strings.ga_len < 1)
	// Last string removed, remove the reply
	socketserver_remove_reply(client);

    return OK;
}

/*
 * Check for any replies for "sender". Returns 1 if there is and places the
 * reply in "str" without consuming it (note that a copy is not created).
 * Returns 0 if otherwise and -1 on
 * error.
 */
    int
socketserver_peek_reply(char_u *sender, char_u **str)
{
    ss_reply_T *reply;

    if (server_channel == NULL || server_address == NULL)
    {
	emsg(_(e_socket_server_not_online));
	return FAIL;
    }

    reply = socketserver_get_reply(sender, NULL);
    if (reply != NULL && reply->strings.ga_len > 0)
    {
        if (str != NULL)
            *str = ((char_u **)reply->strings.ga_data)[0];
        return 1;
    }
    return 0;
}

#endif // FEAT_SOCKETSERVER
