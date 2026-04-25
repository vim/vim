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
 *   "str": string -- What to execute for the command or contents of result
 *
 *   ?"code": number -- Return code for expression
 *
 *   ?"sender": string -- Address of client that sent command if it is a server.
 *
 *   ?"wait": bool -- Used by --remote-wait since the client is not a server.
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
    char_u *sender; // Includes "type:" prefix if any
    garray_T strings;
} ss_reply_T;

static channel_T    *server_channel = NULL;
static char_u	    *server_address = NULL; // Includes "type:" prefix if any
static bool	    server_is_unix = false;
static char_u	    *server_addr_cwd = NULL; // CWD when server was started, used
					  // to handle relative file paths.
static garray_T	    server_replies;

static channel_T    *client_channels = NULL;

#define FOR_ALL_CLIENTS(ch) \
    for (ch = client_channels; ch != NULL; ch = ch->ch_ss_next)

static void	    socketserver_cleanup(void);
static char_u	    *socketserver_create_path(char_u *name, bool quiet);
static char_u	    *socketserver_get_path(char_u *name, bool new, bool quiet);
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
    int		port;
    bool	is_unix = false;
    channel_T	*channel;
    char_u	*buf;
    char_u	dirbuf[MAXPATHL];

    if (server_channel != NULL)
	return OK;

    if (STRNICMP(name, "channel:", 8) == 0)
    {
	address = channel_parse_address(name + 8, &port, &is_unix, true, quiet);

	if (address == NULL)
	    return FAIL;
    }
    else
    {
	address = socketserver_create_path(name, quiet);
	if (address == NULL)
	    return FAIL;
	is_unix = true;
    }

    if (is_unix)
	channel = channel_listen_unix((char *)address, NULL, false);
    else
	channel = channel_listen(port, NULL);

    if (channel == NULL)
    {
	vim_free(address);
	return FAIL;
    }

    channel->ch_socketserver = true;
    channel->ch_ss_accept_cb = socketserver_accept;
    channel->ch_ss_close_cb = socketserver_close;

    server_channel = channel;
    server_is_unix = is_unix;

    VIM_CLEAR(serverName);

    if (STRNICMP(name, "channel:", 8) == 0)
	buf = vim_strsave(name);
    else
    {
	buf = alloc(MAXPATHL + 1);
	if (buf != NULL)
	{
	    buf[0] = NUL;
	    mch_FullName(address, buf, MAXPATHL, false);
	}
    }
    vim_free(address);

    if (buf != NULL)
    {
	server_address = vim_strsave(buf);
	serverName = buf;
	set_vim_var_string(VV_SEND_SERVER, serverName, -1);
    }

    vim_free(server_addr_cwd);
    if (mch_dirname(dirbuf, sizeof(dirbuf)) == OK)
	server_addr_cwd = vim_strsave(dirbuf);
    else
	server_addr_cwd = NULL;

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

    socketserver_cleanup();

    ch_log(NULL, "socketserver: shutting down server");
}

/*
 * Cleanup server stuff. Do not close client channels, as those are not part of
 * the actual server.
 */
    static void
socketserver_cleanup(void)
{
#ifdef UNIX
    if (server_is_unix && server_address != NULL)
    {
	char_u *path = server_address;
	char_u dirbuf[MAXPATHL];

	if (STRNICMP(path, "channel:unix:", 13) == 0)
	    path += 13;
	else if (STRNICMP(path, "name:", 5) == 0)
	    path += 5;

	if (*path == '/')
	    mch_remove(path);
	// Go to the directory where the server was started. This is to handle
	// when Vim changes directories and the servername is a relative path.
	else if (server_addr_cwd != NULL
		&& mch_dirname(dirbuf, sizeof(dirbuf)) == OK)
	{
	    if (mch_chdir((char *)server_addr_cwd) == 0)
	    {
		mch_remove(path);
		mch_chdir((char *)dirbuf);
	    }
	}
    }
#endif

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
 * List available sockets that can be connected to, only in common directories
 * that Vim knows about. Vim instances with custom socket paths will not be
 * detected. Returns a newline separated string on success and NULL on failure.
 */
    char_u *
socketserver_list(void)
{
#ifdef MSWIN
    // Only support addresses on Windows
    return vim_strsave((char_u *)"");
#else
    garray_T	    str;
    string_T	    buf;
    string_T	    path;
    DIR		    *dirp;
    struct dirent   *dp;
    const char_u    *known_dirs[] = {
	mch_getenv("XDG_RUNTIME_DIR"),
	mch_getenv("TMPDIR"),
	(char_u *)"/tmp"
    };

    if ((buf.string = alloc(MAXPATHL)) == NULL)
	return NULL;
    if ((path.string = alloc(MAXPATHL)) == NULL)
    {
	vim_free(buf.string);
	return NULL;
    }
    buf.length = 0;
    path.length = 0;

    ga_init2(&str, 1, 100);

    for (size_t i = 0 ; i < ARRAY_LENGTH(known_dirs); i++)
    {
	const char_u *dir = known_dirs[i];

	if (dir == NULL)
	    continue;

	if (STRCMP(dir, "/tmp") == 0 ||
		(known_dirs[1] != NULL && STRCMP(dir, known_dirs[1]) == 0))
	    path.length = vim_snprintf_safelen((char *)path.string, MAXPATHL,
		    "%s/vim-%lu", dir, (unsigned long int)getuid());
	else
	    path.length = vim_snprintf_safelen((char *)path.string, MAXPATHL,
		    "%s/vim", dir);

	dirp = opendir((char *)path.string);
	if (dirp == NULL)
	    continue;

	// Loop through directory
	while ((dp = readdir(dirp)) != NULL)
	{
	    if (STRCMP(dp->d_name, ".") == 0 || STRCMP(dp->d_name, "..") == 0)
		continue;

	    buf.length = vim_snprintf_safelen((char *)buf.string, MAXPATHL,
		    "%s/%s", path.string, dp->d_name);

	    ga_concat_len(&str, (char_u *)dp->d_name,
		    buf.length - (path.length + 1));
	    ga_append(&str, '\n');
	}

	closedir(dirp);

	break;
    }

    vim_free(path.string);
    vim_free(buf.string);

    ga_append(&str, NUL);

    return str.ga_data;
#endif
}

/*
 * If "name" is a path (starts with a '/', './', or '../'), it is assumed to be
 * the path to the desired socket. If the socket path is already taken, append
 * an incrementing number to the path until we find a socket filename that can
 * be used. Returns alloced string or NULL on failure.
 */
    static char_u *
socketserver_create_path(char_u *name, bool quiet)
{
#ifdef MSWIN
    // Only support channel addresses on Windows
    if (STRNICMP(name, "channel:", 8) == 0 && STRLEN(name) > 8)
	return vim_strsave(name + 8);
    else
    {
	if (!quiet)
	    semsg(_(e_invalid_argument_str), name);
	return NULL;
    }
#else
    char_u  *buf = NULL;
    int	    buflen = STRLEN(name) + NUMBUFLEN;
    char_u  *path = NULL;

    if (STRNICMP(name, "channel:", 8) == 0 && STRLEN(name) > 8)
	return vim_strsave(name + 8);
    if (STRNICMP(name, "name:", 5) == 0)
	name += 5;

    for (int i = 0; i < 1000; i++)
    {
	if (buf != NULL)
	{
	    vim_snprintf((char *)buf, buflen, "%s%d", name, i);
	    path = socketserver_get_path(buf, true, quiet);
	}
	else
	    path = socketserver_get_path(name, true, quiet);

	if (path == NULL)
	{
	    if (buf == NULL)
	    {
		buf = alloc(buflen);
		if (buf == NULL)
		{
		    semsg(_(e_out_of_memory_allocating_nr_bytes), buflen);
		    return NULL;
		}
	    }
	    continue;
	}
	break;
    }
    vim_free(buf);

    return path;
#endif
}

/*
 * If "name" is a pathless name such as "VIM", search known directories for the
 * socket named "name", and return the alloc'ed path to it. If "name" starts
 * with a '/', './' or '../', then a copy of "name" is returned.
 *
 * If "name" starts with "channel:", then return the address part
 *
 * If "new" is true, then return a path if the name does not exist in the known
 * location.
 *
 * Returns path on success and NULL on failure.
 */
    static char_u *
socketserver_get_path(char_u *name, bool new UNUSED, bool quiet)
{
#ifdef MSWIN
    // Only support channel addresses on Windows
    if (STRNICMP(name, "channel:", 8) == 0 && STRLEN(name) > 8)
	return vim_strsave(name + 8);
    else
    {
	if (!quiet)
	    semsg(_(e_invalid_argument_str), name);
	return NULL;
    }
#else
    char_u	    *buf;
    bool	    res = false;
    channel_T	    *channel;
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

    if (STRNICMP(name, "channel:", 8) == 0 && STRLEN(name) > 8)
	return vim_strsave(name + 8);

    if (STRNICMP(name, "name:", 5) == 0)
	name += 5;

    if (vim_strchr(name, '/') != NULL)
    {
	if (!quiet)
	    semsg(_(e_socket_name_no_slashes), name);
	return NULL;
    }

    buf = alloc(MAXPATHL);

    if (buf == NULL)
	return NULL;

    for (size_t i = 0; i < ARRAY_LENGTH(known_dirs); i++)
    {
	const char_u	*dir = known_dirs[i];
	bool		got = false;

	if (dir == NULL)
	    continue;
	else if (STRCMP(dir, "/tmp") == 0 ||
		(known_dirs[1] != NULL && STRCMP(dir, known_dirs[1]) == 0))
	{
	    // "/tmp" or $TMPDIR, must suffix dir with uid
	    vim_snprintf((char *)buf, MAXPATHL, "%s/vim-%lu", dir,
		    (unsigned long int)getuid());
	    if (vim_mkdir(buf, 0700) == -1 && errno != EEXIST)
		continue;

	    vim_snprintf((char *)buf, MAXPATHL, "%s/vim-%lu/%s", dir,
		    (unsigned long int)getuid(), name);
	}
	else
	{
	    vim_snprintf((char *)buf, MAXPATHL, "%s/vim", dir);
	    if (vim_mkdir(buf, 0700) == -1 && errno != EEXIST)
		continue;

	    vim_snprintf((char *)buf, MAXPATHL, "%s/vim/%s", dir, name);
	}

	// If looking for a new socket path, and "buf" currently exists, check
	// if it is a dead socket, if it is then remove it.
	if (new)
	{
	    emsg_silent++;
	    channel = channel_open_unix((char *)buf, NULL);
	    emsg_silent--;

	    if (channel != NULL)
	    {
		channel_close(channel, false);
		channel_clear(channel);
	    }
	    else
	    {
		mch_remove(buf);
		got = true;
	    }
	}

	res = true;
	if (got || (!new && mch_access((char *)buf, F_OK) == 0))
	{
	    if (server_address != NULL && STRCMP(buf, server_address) == 0)
		// Can't connect to itself
		break;
	    return buf;
	}
	break;
    }

    if (!quiet)
    {
	if (!res)
	    semsg(_("Failed creating socket directory: %s"), strerror(errno));
	else
	    semsg(_(e_invalid_server_id_used_str), name);
    }
    vim_free(buf);
    return NULL;
#endif
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
    if (client_channels != NULL)
	client_channels->ch_ss_prev = channel;
    client_channels = channel;

    // We will read the command from the client later in the input loop.
    ch_log(NULL, "socketserver: accepted new client");
}

/*
 * Callback for when server channel is closed
 */
    static void
socketserver_close(channel_T *channel UNUSED)
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
    char_u	*str = NULL;
    char_u	*sender = NULL;
    char_u	idbuf[NUMBUFLEN];

    di = dict_find(message, (char_u *)"type", -1);
    if (di == NULL || di->di_tv.v_type != VAR_STRING)
	return;
    else
	type = di->di_tv.vval.v_string;

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

    di = dict_find(message, (char_u *)"wait", -1);
    if (di != NULL && di->di_tv.v_type == VAR_BOOL && di->di_tv.vval.v_number)
    {
	// Client is not a server, but still wants a response later. Save the
	// ID of the channel connection that we will use to send back a response,
	vim_snprintf((char *)idbuf, NUMBUFLEN, "remotewait:%d", channel->ch_id);

	sender = idbuf;
	vim_free(client_socket);
	client_socket = vim_strsave(sender);
    }

    ch_log(NULL, "socketserver_exec(): result: %s",
	    str == NULL ? (char_u *)"(null)" : str);

    if (STRCMP(type, "expr") == 0 && str != NULL)
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

	tv.v_type = VAR_DICT;
	tv.vval.v_dict = dict;

	buf = json_encode(&tv, JSON_NL);

	if (buf != NULL)
	{
	    emsg_silent++;
	    channel_send(channel, PART_SOCK, buf, (int)STRLEN(buf),
		    "socketserver_exec");
	    emsg_silent--;
	}
	vim_free(buf);
	dict_unref(dict);
    }
    else if (STRCMP(type, "keystrokes") == 0 && str != NULL)
    {
	// Execute keystrokes
	server_to_input_buf(str);
    }
    else if (STRCMP(type, "notify") == 0)
    {
	// Notification, execute autocommands and save the reply for later use
	if (sender != NULL && str != NULL)
	{
	    ss_reply_T *reply;

	    reply = socketserver_add_reply(sender);

	    if (reply != NULL)
		ga_copy_string(&reply->strings, str);

	    apply_autocmds(EVENT_REMOTEREPLY, sender, str, TRUE, curbuf);
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
	free_tv(*tv);
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
    typval_T	*tv;

    for (channel_T *ch = client_channels; ch != NULL;)
    {
	// Make sure to save next channel in case "ch" is freed. Not sure if
	// this can actually happen but be safe.
	channel_T *next = ch->ch_ss_next;

	// Get the JSON message if there is any from the queue for this channel.
	if (socketserver_get_message(ch, &tv) == FAIL)
	{
	    ch = next;
	    continue;
	}

	socketserver_exec(ch, tv->vval.v_dict);
	free_tv(tv);
	ch = next;
    }
}


/*
 * Poll until there is something to read on "channel". Also handle other
 * socketserver channels in the meantime. If "channel" is NULL, then poll all
 * channels once then exit. If "timeout" is -1, then wait forever unless
 * interrupted.
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
	int		maxfd = -1;

	if (timeout != -1)
	{
	    tv.tv_sec = timeout / 1000;
	    tv.tv_usec = (timeout % 1000) * 1000;
	}

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

	if (maxfd == -1)
	    return FAIL;

	ret = select(maxfd + 1, &rfds, NULL, NULL, timeout == -1 ? NULL : &tv);

# ifdef EINTR
	if (ret == -1 && errno == EINTR)
	{
	    if (got_int)
		break;
	    continue;
	}
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
	int		channel_idx = -1;
	int		server_idx = -1;

	if (channel != NULL)
	{
	    if (channel->CH_SOCK_FD == INVALID_FD)
		// Shouldn't happen
		return FAIL;

	    channel_idx = nfd;
	    fds[nfd].fd = channel->CH_SOCK_FD;
	    fds[nfd++].events = POLLIN;
	}
	if (server_channel != NULL && server_channel->CH_SOCK_FD != INVALID_FD)
	{
	    server_idx = nfd;
	    fds[nfd].fd = server_channel->CH_SOCK_FD;
	    fds[nfd++].events = POLLIN;
	}

	FOR_ALL_CLIENTS(ch)
	    if (ch->CH_SOCK_FD != INVALID_FD)
	    {
		fds[nfd].fd = ch->CH_SOCK_FD;
		fds[nfd].events = POLLIN;
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
		    && fds[server_idx].revents & POLLIN)
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
		    && fds[channel_idx].revents & POLLIN)
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

/*
 * Parse "name" and create or get the channel connection for it. If "wait" is
 * not NULL, then if the client name is a channel ID, then it will be set to
 * true. Returns NULL on
 * failure.
 */
    static channel_T *
socketserver_get_channel(char_u *name, bool quiet, bool *wait)
{
    char_u	*address;
    int		port;
    bool	is_unix;
    channel_T	*channel;

    if (STRNICMP(name, "channel:", 8) == 0)
    {
	address = channel_parse_address(name + 8, &port, &is_unix, true, quiet);

	if (address == NULL)
	    return NULL;
    }
    else if (STRNICMP(name, "remotewait:", 11) == 0)
    {
	// Channel ID name, find channel with that ID.
	int id;

	if (name[2] == NUL)
	    return NULL;
	id = strtol((char *)name + 11, NULL, 10);
	if (wait != NULL)
	    *wait = true;

	return channel_find(id);
    }
    else
    {
	address = socketserver_get_path(name, false, quiet);
	if (address == NULL)
	    return NULL;
	is_unix = true;
    }

    if (is_unix)
	channel = channel_open_unix((char *)address, NULL);
    else
	channel =  channel_open((char *)address, port, 1000, NULL);

    if (channel == NULL && !quiet)
	semsg(_(e_socket_server_failed_connecting), name);

    vim_free(address);

    return channel;
}

/*
 * Send command to address "name". If "ch" is not NULL, it is set to the channel
 * for the connection between us and the server, and the channel will not just
 * be closed immediately, this is used for --remote-wait. Returns 0 for OK, -1
 * on error.
 */
    int
socketserver_send(
	char_u *name,
	char_u *str,
	char_u **result,
	bool is_expr,
	int timeout,
	bool silent,
	channel_T **ch)
{
    int		rcode = -1;
    channel_T	*channel;
    dict_T	*dict;
    dictitem_T	*di;
    typval_T	tv;
    typval_T	*resp_tv = NULL;
    char_u	*buf;

    if (*name == NUL)
    {
	semsg(_(e_unable_to_send_to_str), name);
	return FAIL;
    }

    // Execute locally if target is ourselves
    if (serverName != NULL && STRICMP(name, serverName) == 0)
	return sendToLocalVim(str, is_expr, result);

    channel = socketserver_get_channel(name, silent, NULL);
    if (channel == NULL)
	return -1;

    dict = dict_alloc();
    if (dict == NULL)
	goto exit;

    dict_add_string(dict, "type", (char_u *)(is_expr ? "expr" : "keystrokes"));
    dict_add_string(dict, "str", str);

    // Tell server who we are so it can save our socket path internally for
    // later use with server2client. Only do this if we are actually a server.
    //
    // If we are not a server, then --remote-wait will not work. To handle this
    // case, we add "wait" to the JSON message set to true, so that the server
    // will create an internal address for our connection to it.
    if (server_address != NULL)
	dict_add_string(dict, "sender", server_address);
    else if (ch != NULL)
	dict_add_bool(dict, "wait", true);

    tv.v_type = VAR_DICT;
    tv.vval.v_dict = dict;

    buf = json_encode(&tv, JSON_NL);
    if (buf == NULL
	    || channel_send(channel, PART_SOCK, buf, (int)STRLEN(buf),
		"socketserver_send") == FAIL)
    {
	dict_unref(dict);
	vim_free(buf);
	goto exit;
    }
    vim_free(buf);
    dict_unref(dict);

    if (!is_expr)
    {
	// Exit, we aren't waiting for a response
	rcode = 0;
	if (ch != NULL)
	{
	    *ch = channel;

	    channel->ch_ss_next = client_channels;
	    channel->ch_ss_prev = NULL;
	    client_channels = channel;

	    return rcode;
	}
	goto exit;
    }

    if (timeout == 0)
	timeout = 1000;

    // To handle recursive calls, we must handle any socketserver channels as
    // well.
    while (socketserver_wait(channel, timeout) == OK)
	if (socketserver_get_message(channel, &resp_tv) == OK)
	    break;

    if (resp_tv == NULL)
	goto exit;

    dict = resp_tv->vval.v_dict;

    di = dict_find(dict, (char_u *)"type", -1);
    if (di == NULL || di->di_tv.v_type != VAR_STRING ||
	    STRCMP(di->di_tv.vval.v_string, "reply") != 0)
    {
	ch_error(NULL, "socketserver: unknown reply type");
	free_tv(resp_tv);
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
		    sizeof(ss_reply_T) * (len - index - 1));
	server_replies.ga_len--;
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
    bool	wait = false;

    if (*client == NUL)
    {
	semsg(_(e_invalid_server_id_used_str), client);
	return FAIL;
    }

    if (server_channel == NULL || server_address == NULL)
    {
	emsg(_(e_socket_server_not_online));
	return FAIL;
    }

    channel = socketserver_get_channel(client, false, &wait);
    if (channel == NULL)
	return FAIL;

    dict = dict_alloc();
    if (dict == NULL)
    {
	ret = FAIL;
	goto exit;
    }

    dict_add_string(dict, "type", (char_u *)"notify");
    dict_add_string(dict, "str", str);
    if (server_address != NULL)
	dict_add_string(dict, "sender", server_address);

    tv.v_type = VAR_DICT;
    tv.vval.v_dict = dict;

    buf = json_encode(&tv, JSON_NL);
    if (buf == NULL
	    || channel_send(channel, PART_SOCK, buf, (int)STRLEN(buf),
		"socketserver_send_reply") == FAIL)
	ret = FAIL;

    vim_free(buf);
    dict_unref(dict);

exit:
    // Don't want to close the channel if client is referenced by channel ID.
    // This allows --remote-wait to work with multiple files.
    if (!wait)
    {
	channel_close(channel, false);
	channel_clear(channel);
    }

    return ret;
}

/*
 * Wait for reply from "client" and place result in "str". Returns OK on success
 * and FAIL on failure. Timeout is in milliseconds
 */
    int
socketserver_read_reply(
	char_u *client,
	char_u **str,
	int timeout,
	bool remotewait)
{
    ss_reply_T	*reply = NULL;
    char_u	*actual;

    if (*client == NUL)
    {
	semsg(_(e_invalid_server_id_used_str), client);
	return FAIL;
    }

    if (!remotewait && (server_channel == NULL || server_address == NULL))
    {
	emsg(_(e_socket_server_not_online));
	return FAIL;
    }

    actual = socketserver_get_path(client, false, false);
    if (actual == NULL)
	return FAIL;

    while (true)
    {
	reply = socketserver_get_reply(actual, NULL);
	if (reply != NULL)
	    break;
	if (socketserver_wait(NULL, timeout) == FAIL)
	    break;
    }

    if (reply == NULL || reply->strings.ga_len == 0)
    {
	vim_free(actual);
	return FAIL;
    }

    // Consume the string
    *str = ((char_u **)reply->strings.ga_data)[0];

    if (reply->strings.ga_len > 1)
	mch_memmove((char_u **)reply->strings.ga_data,
		((char_u **)reply->strings.ga_data) + 1,
		sizeof(char_u *) * (reply->strings.ga_len - 1));
    reply->strings.ga_len--;

    if (reply->strings.ga_len < 1)
	// Last string removed, remove the reply
	socketserver_remove_reply(actual);

    vim_free(actual);

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
    ss_reply_T	*reply;
    char_u	*actual;

    if (*sender == NUL)
    {
	semsg(_(e_invalid_server_id_used_str), sender);
	return FAIL;
    }

    if (server_channel == NULL || server_address == NULL)
    {
	emsg(_(e_socket_server_not_online));
	return FAIL;
    }

    actual = socketserver_get_path(sender, false, false);
    if (actual == NULL)
	return FAIL;

    reply = socketserver_get_reply(actual, NULL);
    vim_free(actual);

    if (reply != NULL && reply->strings.ga_len > 0)
    {
	if (str != NULL)
	    *str = ((char_u **)reply->strings.ga_data)[0];
	return 1;
    }
    return 0;
}

#endif // FEAT_SOCKETSERVER
