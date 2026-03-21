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

channel_T   *server_channel = NULL;
char_u	    *server_address = NULL;
bool	    server_is_unix = false;
channel_T   *client_channels = NULL;

#define FOR_ALL_CLIENTS(ch) \
    for (ch = client_channels; ch != NULL; ch = ch->ch_ss_next)

static void socketserver_accept(channel_T *channel);
static void socketserver_close(channel_T *channel);

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

    if (channel_parse_address(name, (char *)IObuff, IOSIZE, &port,
		&is_unix, true, quiet) == FAIL)
	return FAIL;
    address = IObuff;

    if (is_unix)
	channel = channel_listen_unix((char *)address, NULL);
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

    serverName = vim_strsave(name);
# ifdef FEAT_EVAL
    set_vim_var_string(VV_SEND_SERVER, serverName, -1);
# endif

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
    
    server_channel = NULL;
    vim_free(server_address);
    server_address = NULL;

    ch_log(NULL, "socketserver: shutting down server");
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
    server_channel = NULL;
    vim_free(server_address);
    server_address = NULL;

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
    /* char_u	*to_free2; */

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
 * socketserver channels in the meantime. Return OK on success and FAIL on
 * failure or timeout.
 */
    static int
socketserver_wait(channel_T *channel, int timeout)
{
    // TODO: support poll() and mswin
    while (true)
    {
#ifdef HAVE_SELECT
	channel_T	*ch;
	fd_set		rfds;
	fd_set		wfds;
	struct timeval  tv;
	int		maxfd;
	int		ret;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	maxfd = channel->CH_SOCK_FD;
	FD_SET(channel->CH_SOCK_FD, &rfds);
	if (server_channel != NULL)
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

		if (channel_check_write_queue(channel, PART_SOCK))
		    FD_SET(ch->CH_SOCK_FD, &wfds);
	    }
	}

	ret = select(maxfd + 1, &rfds, &wfds, NULL, &tv);

# ifdef EINTR
	if (ret == -1 && errno == EINTR)
	    continue;
# endif

	if (ret > 0)
	{
	    FOR_ALL_CLIENTS(ch)
	    {
		if (FD_ISSET(ch->CH_SOCK_FD, &rfds))
		    channel_check(ch, PART_SOCK);

		if (FD_ISSET(ch->CH_SOCK_FD, &wfds))
		    channel_write_input(channel);
	    }

	    socketserver_parse_messages();

	    if (FD_ISSET(channel->CH_SOCK_FD, &rfds))
	    {
		channel_check(channel, PART_SOCK);
		return OK;
	    }
	    continue;
	}
	break;
#endif
    }
    return FAIL;
}

/*
 * Send command to address "name". Returns 0 for OK, -1 on error.
 */
    int
socketserver_send(
	char_u *name,
        char_u *str,
	char_u **result,
	int is_expr,
	int timeout,
	int silent)
{
    int		port;
    bool	is_unix;
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

    if (channel_parse_address(name, (char *)IObuff, IOSIZE, &port, &is_unix,
		false, false) == FAIL)
	return FAIL;

    if (is_unix)
	channel = channel_open_unix((char *)IObuff, NULL);
    else
	channel = channel_open((char *)IObuff, port, 1000, NULL);

    if (channel == NULL)
	return -1;

    dict = dict_alloc();
    if (dict == NULL)
	goto fail;

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
	goto fail;
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

    if (result != NULL)
    {
	di = dict_find(dict, (char_u *)"str", -1);
	if (di != NULL && di->di_tv.v_type == VAR_STRING)
	    *result = vim_strsave(di->di_tv.vval.v_string);
	else
	{
	    dict_unref(dict);
	    goto fail;
	}
    }

    di = dict_find(dict, (char_u *)"code", -1);
    if (di != NULL && di->di_tv.v_type == VAR_NUMBER)
	rcode = di->di_tv.vval.v_number;

    free_tv(resp_tv);

    return rcode;
fail:
    channel_close(channel, false);
    channel_clear(channel);
    return -1;
}

#endif // FEAT_SOCKETSERVER
