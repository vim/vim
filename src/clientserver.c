/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * clientserver.c: functions for Client Server functionality
 */

#include "vim.h"

#if defined(FEAT_CLIENTSERVER) || defined(PROTO)

#ifdef FEAT_SOCKETSERVER
# include <sys/socket.h>
# include "sys/un.h"
#endif

static void cmdsrv_main(int *argc, char **argv, char_u *serverName_arg, char_u **serverStr);
static char_u *serverMakeName(char_u *arg, char *cmd);

/*
 * Replace termcodes such as <CR> and insert as key presses if there is room.
 */
    void
server_to_input_buf(char_u *str)
{
    char_u      *ptr = NULL;
    char_u      *cpo_save = p_cpo;

    // Set 'cpoptions' the way we want it.
    //    B set - backslashes are *not* treated specially
    //    k set - keycodes are *not* reverse-engineered
    //    < unset - <Key> sequences *are* interpreted
    //  The last but one parameter of replace_termcodes() is TRUE so that the
    //  <lt> sequence is recognised - needed for a real backslash.
    p_cpo = (char_u *)"Bk";
    str = replace_termcodes(str, &ptr, 0, REPTERM_DO_LT, NULL);
    p_cpo = cpo_save;

    if (*ptr != NUL)	// trailing CTRL-V results in nothing
    {
	/*
	 * Add the string to the input stream.
	 * Can't use add_to_input_buf() here, we now have K_SPECIAL bytes.
	 *
	 * First clear typed characters from the typeahead buffer, there could
	 * be half a mapping there.  Then append to the existing string, so
	 * that multiple commands from a client are concatenated.
	 */
	if (typebuf.tb_maplen < typebuf.tb_len)
	    del_typebuf(typebuf.tb_len - typebuf.tb_maplen, typebuf.tb_maplen);
	(void)ins_typebuf(str, REMAP_NONE, typebuf.tb_len, TRUE, FALSE);

	// Let input_available() know we inserted text in the typeahead
	// buffer.
	typebuf_was_filled = TRUE;
    }
    vim_free(ptr);
}

/*
 * Evaluate an expression that the client sent to a string.
 */
    char_u *
eval_client_expr_to_string(char_u *expr)
{
    char_u	*res;
    int		save_dbl = debug_break_level;
    int		save_ro = redir_off;
    funccal_entry_T funccal_entry;
    int		did_save_funccal = FALSE;

#if defined(FEAT_EVAL)
    ch_log(NULL, "eval_client_expr_to_string(\"%s\")", expr);
#endif

    // Evaluate the expression at the toplevel, don't use variables local to
    // the calling function. Except when in debug mode.
    if (!debug_mode)
    {
	save_funccal(&funccal_entry);
	did_save_funccal = TRUE;
    }

     // Disable debugging, otherwise Vim hangs, waiting for "cont" to be
     // typed.
    debug_break_level = -1;
    redir_off = 0;
    // Do not display error message, otherwise Vim hangs, waiting for "cont"
    // to be typed.  Do generate errors so that try/catch works.
    ++emsg_silent;

    res = eval_to_string(expr, TRUE, FALSE);

    debug_break_level = save_dbl;
    redir_off = save_ro;
    --emsg_silent;
    if (emsg_silent < 0)
	emsg_silent = 0;
    if (did_save_funccal)
	restore_funccal();

    // A client can tell us to redraw, but not to display the cursor, so do
    // that here.
    setcursor();
    out_flush_cursor(FALSE, FALSE);

    return res;
}

/*
 * Evaluate a command or expression sent to ourselves.
 */
    int
sendToLocalVim(char_u *cmd, int asExpr, char_u **result)
{
    if (asExpr)
    {
	char_u *ret;

	ret = eval_client_expr_to_string(cmd);
	if (result != NULL)
	{
	    if (ret == NULL)
	    {
		char	*err = _(e_invalid_expression_received);
		size_t	len = STRLEN(cmd) + STRLEN(err) + 5;
		char_u	*msg;

		msg = alloc(len);
		if (msg != NULL)
		    vim_snprintf((char *)msg, len, "%s: \"%s\"", err, cmd);
		*result = msg;
	    }
	    else
		*result = ret;
	}
	else
	    vim_free(ret);
	return ret == NULL ? -1 : 0;
    }
    server_to_input_buf(cmd);
    return 0;
}

/*
 * If conversion is needed, convert "data" from "client_enc" to 'encoding' and
 * return an allocated string.  Otherwise return "data".
 * "*tofree" is set to the result when it needs to be freed later.
 */
    char_u *
serverConvert(
    char_u *client_enc UNUSED,
    char_u *data,
    char_u **tofree)
{
    char_u	*res = data;

    *tofree = NULL;
    if (client_enc == NULL || p_enc == NULL)
	return res;

    vimconv_T	vimconv;

    vimconv.vc_type = CONV_NONE;
    if (convert_setup(&vimconv, client_enc, p_enc) != FAIL
	    && vimconv.vc_type != CONV_NONE)
    {
	res = string_convert(&vimconv, data, NULL);
	if (res == NULL)
	    res = data;
	else
	    *tofree = res;
    }
    convert_setup(&vimconv, NULL, NULL);
    return res;
}
#endif

#if (defined(FEAT_CLIENTSERVER) && !defined(NO_VIM_MAIN)) || defined(PROTO)

/*
 * Common code for the X command server and the Win32 command server.
 */

static char_u *build_drop_cmd(int filec, char **filev, int tabs, int sendReply);

/*
 * Do the client-server stuff, unless "--servername ''" was used.
 */
    void
exec_on_server(mparm_T *parmp)
{
    int made_name = FALSE;

    if (parmp->serverName_arg != NULL && *parmp->serverName_arg == NUL)
	return;

# ifdef MSWIN
    // Initialise the client/server messaging infrastructure.
    serverInitMessaging();
# endif

#ifdef FEAT_SOCKETSERVER
    // If servername is specified and we are using sockets, always init the
    // sockt server. We may need to receive replies back to us. If --serverlist
    // is passed, the socket server will be uninitialized before listing
    // sockets then initialized after. This is so we don't add our own socket
    // in the list. This does not happen in serverlist().
    if ((parmp->serverArg || parmp->serverName_arg != NULL) &&
	    clientserver_method == CLIENTSERVER_METHOD_SOCKET)
    {
	parmp->servername = serverMakeName(parmp->serverName_arg,
		parmp->argv[0]);
	if (socket_server_init(parmp->servername) == OK)
	    TIME_MSG("initialize socket server");
	made_name = TRUE;
    }
#endif

    /*
     * When a command server argument was found, execute it.  This may
     * exit Vim when it was successful.  Otherwise it's executed further
     * on.  Remember the encoding used here in "serverStrEnc".
     */
    if (parmp->serverArg)
    {
	cmdsrv_main(&parmp->argc, parmp->argv,
		parmp->serverName_arg, &parmp->serverStr);
	parmp->serverStrEnc = vim_strsave(p_enc);
    }

    // If we're still running, get the name to register ourselves.
    // On Win32 can register right now, for X11 need to setup the
    // clipboard first, it's further down.
    if (!made_name && parmp->servername == NULL)
	parmp->servername = serverMakeName(parmp->serverName_arg,
		parmp->argv[0]);
# ifdef MSWIN
    if (parmp->servername != NULL)
    {
	serverSetName(parmp->servername);
	vim_free(parmp->servername);
    }
# endif
}
/*
 * Prepare for running as a Vim server.
 */
    void
prepare_server(mparm_T *parmp)
{
# if defined(FEAT_X11) || defined(FEAT_SOCKETSERVER)
    /*
     * Register for remote command execution with :serversend and --remote
     * unless there was a -X or a --servername '' on the command line.
     * Only register nongui-vim's with an explicit --servername argument,
     * or when compiling with autoservername.
     * When running as root --servername is also required.
     */

    if (
#  ifdef FEAT_X11
	    X_DISPLAY != NULL &&
#  endif

	    parmp->servername != NULL && (
#  if defined(FEAT_AUTOSERVERNAME) || defined(FEAT_GUI)
		(
#   if defined(FEAT_AUTOSERVERNAME)
		    1
#   else
		    gui.in_use
#   endif
#   ifdef UNIX
		 && getuid() != ROOT_UID
#   endif
		) ||
#  endif
		parmp->serverName_arg != NULL))
    {
#  ifdef FEAT_SOCKETSERVER
	if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	{
	    if (socket_server_init(parmp->servername) == OK)
		TIME_MSG("initialize socket server");
	}
#  endif
#  ifdef FEAT_X11
	if (clientserver_method == CLIENTSERVER_METHOD_X11)
	{
	    (void)serverRegisterName(X_DISPLAY, parmp->servername);
	    TIME_MSG("register x11 server name");
	}
#  endif
	vim_free(parmp->servername);
    }
#ifdef FEAT_X11
    else
	serverDelayedStartName = parmp->servername;
#endif
# endif

    /*
     * Execute command ourselves if we're here because the send failed (or
     * else we would have exited above).
     */
    if (parmp->serverStr != NULL)
    {
	char_u *p;

	server_to_input_buf(serverConvert(parmp->serverStrEnc,
						       parmp->serverStr, &p));
	vim_free(p);
    }
}

    static void
cmdsrv_main(
    int		*argc,
    char	**argv,
    char_u	*serverName_arg,
    char_u	**serverStr)
{
    char_u	*res;
    int		i;
    char_u	*sname;
    int		ret = -1;
    int		didone = FALSE;
    int		exiterr = 0;
    char	**newArgV = argv + 1;
    int		newArgC = 1,
		Argc = *argc;
    int		argtype;
#define ARGTYPE_OTHER		0
#define ARGTYPE_EDIT		1
#define ARGTYPE_EDIT_WAIT	2
#define ARGTYPE_SEND		3
    int		silent = FALSE;
    int		tabs = FALSE;
#ifdef FEAT_SOCKETSERVER
    char_u	*receiver;
#endif
# ifdef MSWIN
    HWND	srv;
# elif defined(FEAT_X11)
    Window	srv;

    setup_term_clip();
# endif

    sname = serverMakeName(serverName_arg, argv[0]);
    if (sname == NULL)
	return;

    /*
     * Execute the command server related arguments and remove them
     * from the argc/argv array; We may have to return into main()
     */
    for (i = 1; i < Argc; i++)
    {
	res = NULL;
	if (STRCMP(argv[i], "--") == 0)	// end of option arguments
	{
	    for (; i < *argc; i++)
	    {
		*newArgV++ = argv[i];
		newArgC++;
	    }
	    break;
	}

	if (STRICMP(argv[i], "--remote-send") == 0)
	    argtype = ARGTYPE_SEND;
	else if (STRNICMP(argv[i], "--remote", 8) == 0)
	{
	    char	*p = argv[i] + 8;

	    argtype = ARGTYPE_EDIT;
	    while (*p != NUL)
	    {
		if (STRNICMP(p, "-wait", 5) == 0)
		{
		    argtype = ARGTYPE_EDIT_WAIT;
		    p += 5;
		}
		else if (STRNICMP(p, "-silent", 7) == 0)
		{
		    silent = TRUE;
		    p += 7;
		}
		else if (STRNICMP(p, "-tab", 4) == 0)
		{
		    tabs = TRUE;
		    p += 4;
		}
		else
		{
		    argtype = ARGTYPE_OTHER;
		    break;
		}
	    }
	}
	else
	    argtype = ARGTYPE_OTHER;

	if (argtype != ARGTYPE_OTHER)
	{
	    if (i == *argc - 1)
		mainerr_arg_missing((char_u *)argv[i]);
	    if (argtype == ARGTYPE_SEND)
	    {
		*serverStr = (char_u *)argv[i + 1];
		i++;
	    }
	    else
	    {
		*serverStr = build_drop_cmd(*argc - i - 1, argv + i + 1,
					  tabs, argtype == ARGTYPE_EDIT_WAIT);
		if (*serverStr == NULL)
		{
		    // Probably out of memory, exit.
		    didone = TRUE;
		    exiterr = 1;
		    break;
		}
		Argc = i;
	    }

#ifdef FEAT_SOCKETSERVER
	    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
		ret = socket_server_send(
			sname, *serverStr, NULL, &receiver,
			0, -1, silent);
#endif
#ifdef FEAT_X11
	    if (clientserver_method == CLIENTSERVER_METHOD_X11)
	    {
		if (xterm_dpy == NULL)
		{
		    mch_errmsg(_("No display"));
		    ret = -1;
		}
		else
		    ret = serverSendToVim(xterm_dpy, sname, *serverStr,
			    NULL, &srv, 0, 0, 0, silent);
	    }
#endif
#ifdef MSWIN
	    // Win32 always works?
	    ret = serverSendToVim(sname, *serverStr, NULL, &srv, 0, 0, silent);
# endif
	    if (ret < 0)
	    {
		if (argtype == ARGTYPE_SEND)
		{
		    // Failed to send, abort.
		    mch_errmsg(_(": Send failed.\n"));
		    didone = TRUE;
		    exiterr = 1;
		}
		else if (!silent)
		    // Let vim start normally.
		    mch_errmsg(_(": Send failed. Trying to execute locally\n"));
		break;
	    }

# ifdef FEAT_GUI_MSWIN
	    // Guess that when the server name starts with "g" it's a GUI
	    // server, which we can bring to the foreground here.
	    // Foreground() in the server doesn't work very well.
	    if (argtype != ARGTYPE_SEND && TOUPPER_ASC(*sname) == 'G')
		SetForegroundWindow(srv);
# endif

	    /*
	     * For --remote-wait: Wait until the server did edit each
	     * file.  Also detect that the server no longer runs.
	     */
	    if (argtype == ARGTYPE_EDIT_WAIT)
	    {
		int	numFiles = *argc - i - 1;
		char_u  *done = alloc(numFiles);
# ifdef FEAT_GUI_MSWIN
		NOTIFYICONDATA ni;
		int	count = 0;
		extern HWND message_window;
# endif

		if (numFiles > 0 && argv[i + 1][0] == '+')
		    // Skip "+cmd" argument, don't wait for it to be edited.
		    --numFiles;

# ifdef FEAT_GUI_MSWIN
		ni.cbSize = sizeof(ni);
		ni.hWnd = message_window;
		ni.uID = 0;
		ni.uFlags = NIF_ICON|NIF_TIP;
		ni.hIcon = LoadIcon((HINSTANCE)GetModuleHandle(0), "IDR_VIM");
		sprintf(ni.szTip, _("%d of %d edited"), count, numFiles);
		Shell_NotifyIcon(NIM_ADD, &ni);
# endif

		// Wait for all files to unload in remote
		vim_memset(done, 0, numFiles);
		while (memchr(done, 0, numFiles) != NULL)
		{
		    char_u  *p = NULL;
		    int	    j;
# ifdef MSWIN
		    p = serverGetReply(srv, NULL, TRUE, TRUE, 0);
		    if (p == NULL)
			break;
# else
#  ifdef FEAT_SOCKETSERVER
		    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET
			    && socket_server_read_reply(receiver, &p, -1) == FAIL)
			    break;
#  endif
#  ifdef FEAT_X11
		    if (clientserver_method == CLIENTSERVER_METHOD_X11
			    && serverReadReply(xterm_dpy, srv, &p, TRUE, -1) < 0)
			    break;
#   endif
		    if (p == NULL)
			break;
# endif
		    j = atoi((char *)p);
		    vim_free(p);
		    if (j >= 0 && j < numFiles)
		    {
# ifdef FEAT_GUI_MSWIN
			++count;
			sprintf(ni.szTip, _("%d of %d edited"),
							     count, numFiles);
			Shell_NotifyIcon(NIM_MODIFY, &ni);
# endif
			done[j] = 1;
		    }
		}
# ifdef FEAT_GUI_MSWIN
		Shell_NotifyIcon(NIM_DELETE, &ni);
# endif
		vim_free(done);
	    }
	}
	else if (STRICMP(argv[i], "--remote-expr") == 0)
	{
	    if (i == *argc - 1)
		mainerr_arg_missing((char_u *)argv[i]);
# ifdef MSWIN
	    // Win32 always works?
	    if (serverSendToVim(sname, (char_u *)argv[i + 1],
						  &res, NULL, 1, 0, FALSE) < 0)
# else
#  ifdef FEAT_SOCKETSERVER
	    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	    {
		if (!socket_server_valid())
		    mch_errmsg(_("Socket server not online:"
				"Send expression failed"));
		else if (socket_server_send(sname, (char_u *)argv[i + 1],
			    &res, NULL, 1, 0, FALSE) < 0)
		    goto expr_fail;
	    }
#  endif
#  ifdef FEAT_X11
	    if (clientserver_method == CLIENTSERVER_METHOD_X11)
	    {
		if (xterm_dpy == NULL)
		    mch_errmsg(_("No display: Send expression failed.\n"));
		else if (serverSendToVim(xterm_dpy, sname,
			    (char_u *)argv[i + 1], &res,
			    NULL, 1, 0, 1, FALSE) < 0)
		    goto expr_fail;
	    }
#  endif
	    if (FALSE)
# endif
	    {
# if !defined(MSWIN)
expr_fail:
# endif
		if (res != NULL && *res != NUL)
		{
		    // Output error from remote
		    mch_errmsg((char *)res);
		    VIM_CLEAR(res);
		}
		mch_errmsg(_(": Send expression failed.\n"));
	    }
	}
	else if (STRICMP(argv[i], "--serverlist") == 0)
	{
# ifdef MSWIN
	    // Win32 always works?
	    res = serverGetVimNames();
# else
#  ifdef FEAT_SOCKETSERVER
	    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	    {
		int was_init = socket_server_valid();

		// Don't want to add ourselves to the list. So shutdown the
		// server before listing then startup back again.
		socket_server_uninit();
		res = socket_server_list_sockets();

		if (was_init)
		    socket_server_init(NULL);
	    }
#  endif
#  ifdef FEAT_X11
	    if (clientserver_method == CLIENTSERVER_METHOD_X11 &&
		    xterm_dpy != NULL)
		res = serverGetVimNames(xterm_dpy);
#  endif
# endif
	    if (did_emsg)
		mch_errmsg("\n");
	}
	else if (STRICMP(argv[i], "--servername") == 0)
	{
	    // Already processed. Take it out of the command line
	    i++;
	    continue;
	}
	else
	{
	    *newArgV++ = argv[i];
	    newArgC++;
	    continue;
	}
	didone = TRUE;
	if (res != NULL && *res != NUL)
	{
	    mch_msg((char *)res);
	    if (res[STRLEN(res) - 1] != '\n')
		mch_msg("\n");
	}
	vim_free(res);
    }

    if (didone)
    {
#ifdef FEAT_SOCKETSERVER
	socket_server_uninit();
#endif
	display_errors();	// display any collected messages
	exit(exiterr);	// Mission accomplished - get out
    }

    // Return back into main()
    *argc = newArgC;
    vim_free(sname);
}

/*
 * Build a ":drop" command to send to a Vim server.
 */
    static char_u *
build_drop_cmd(
    int		filec,
    char	**filev,
    int		tabs,		// Use ":tab drop" instead of ":drop".
    int		sendReply)
{
    garray_T	ga;
    int		i;
    char_u	*inicmd = NULL;
    char_u	*p;
    char_u	*cdp;
    char_u	*cwd;
    // reset wildignore temporarily
    const char *wig[] =
    { "<CR><C-\\><C-N>:let g:_wig=&wig|set wig=",
      "<C-\\><C-N>:let &wig=g:_wig|unlet g:_wig<CR>"};

    if (filec > 0 && filev[0][0] == '+')
    {
	inicmd = (char_u *)filev[0] + 1;
	filev++;
	filec--;
    }
    // Check if we have at least one argument.
    if (filec <= 0)
	mainerr_arg_missing((char_u *)filev[-1]);

    // Temporarily cd to the current directory to handle relative file names.
    cwd = alloc(MAXPATHL);
    if (cwd == NULL)
	return NULL;
    if (mch_dirname(cwd, MAXPATHL) != OK)
    {
	vim_free(cwd);
	return NULL;
    }
    cdp = vim_strsave_escaped_ext(cwd,
#ifdef BACKSLASH_IN_FILENAME
	    (char_u *)"",  // rem_backslash() will tell what chars to escape
#else
	    PATH_ESC_CHARS,
#endif
	    '\\', TRUE);
    vim_free(cwd);
    if (cdp == NULL)
	return NULL;
    ga_init2(&ga, 1, 100);
    ga_concat(&ga, (char_u *)"<C-\\><C-N>:cd ");
    ga_concat(&ga, cdp);
    // reset wildignorecase temporarily
    ga_concat(&ga, (char_u *)wig[0]);

    // Call inputsave() so that a prompt for an encryption key works.
    ga_concat(&ga, (char_u *)
	    "<CR><C-\\><C-N>:if exists('*inputsave')|call inputsave()|endif|");
    if (tabs)
	ga_concat(&ga, (char_u *)"tab ");
    ga_concat(&ga, (char_u *)"drop");
    for (i = 0; i < filec; i++)
    {
	// On Unix the shell has already expanded the wildcards, don't want to
	// do it again in the Vim server.  On MS-Windows only escape
	// non-wildcard characters.
	p = vim_strsave_escaped((char_u *)filev[i],
#ifdef UNIX
		PATH_ESC_CHARS
#else
		(char_u *)" \t%#"
#endif
		);
	if (p == NULL)
	{
	    vim_free(ga.ga_data);
	    return NULL;
	}
	ga_concat(&ga, (char_u *)" ");
	ga_concat(&ga, p);
	vim_free(p);
    }
    ga_concat(&ga, (char_u *)
		  "|if exists('*inputrestore')|call inputrestore()|endif<CR>");

    // The :drop commands goes to Insert mode when 'insertmode' is set, use
    // CTRL-\ CTRL-N again.
    ga_concat(&ga, (char_u *)"<C-\\><C-N>");

    // Switch back to the correct current directory (prior to temporary path
    // switch) unless 'autochdir' is set, in which case it will already be
    // correct after the :drop command. With line breaks and spaces:
    //  if !exists('+acd') || !&acd
    //    if haslocaldir()
    //	    cd -
    //      lcd -
    //    elseif getcwd() ==# 'current path'
    //      cd -
    //    endif
    //  endif
    ga_concat(&ga, (char_u *)":if !exists('+acd')||!&acd|if haslocaldir()|");
#ifdef MSWIN
    // in case :set shellslash is set, need to normalize the directory separators
    // '/' is not valid in a filename so replacing '/' by '\\' should be safe
    ga_concat(&ga, (char_u *)"cd -|lcd -|elseif getcwd()->tr('/','\\') ==# '");
#else
    ga_concat(&ga, (char_u *)"cd -|lcd -|elseif getcwd() ==# '");
#endif
    ga_concat(&ga, cdp);
    ga_concat(&ga, (char_u *)"'|cd -|endif|endif<CR>");
    vim_free(cdp);
    // reset wildignorecase
    ga_concat(&ga, (char_u *)wig[1]);

    if (sendReply)
	ga_concat(&ga, (char_u *)":call SetupRemoteReplies()<CR>");
    ga_concat(&ga, (char_u *)":");
    if (inicmd != NULL)
    {
	// Can't use <CR> after "inicmd", because a "startinsert" would cause
	// the following commands to be inserted as text.  Use a "|",
	// hopefully "inicmd" does allow this...
	ga_concat(&ga, inicmd);
	ga_concat(&ga, (char_u *)"|");
    }
    // Bring the window to the foreground, goto Insert mode when 'im' set and
    // clear command line.
    ga_concat(&ga, (char_u *)"cal foreground()|if &im|star|en|redr|f<CR>");
    ga_append(&ga, NUL);
    return ga.ga_data;
}

/*
 * Make our basic server name: use the specified "arg" if given, otherwise use
 * the tail of the command "cmd" we were started with.
 * Return the name in allocated memory.  This doesn't include a serial number.
 */
    static char_u *
serverMakeName(char_u *arg, char *cmd)
{
    char_u *p;

    if (arg != NULL && *arg != NUL)
    {
#ifdef FEAT_SOCKETSERVER
	// If we are using a socket server, we want to preserve the original
	// name if it is a path, else uppercase it if its just a generic name.
	if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	{
	    if (arg[0] == '/' || STRNCMP(arg, "./", 2) == 0 ||
		    STRNCMP(arg, "../", 3) == 0)
		p = vim_strsave(arg);
	    else
		p = vim_strsave_up(arg);
	}
	else
	    p = vim_strsave_up(arg);
#else
	p = vim_strsave_up(arg);
#endif
    }
    else
    {
	p = vim_strsave_up(gettail((char_u *)cmd));
	// Remove .exe or .bat from the name.
	if (p != NULL && vim_strchr(p, '.') != NULL)
	    *vim_strchr(p, '.') = NUL;
    }
    return p;
}
#endif // FEAT_CLIENTSERVER

#if defined(FEAT_CLIENTSERVER) && defined(FEAT_X11)
    static void
make_connection(void)
{
    if (X_DISPLAY == NULL
# ifdef FEAT_GUI
	    && !gui.in_use
# endif
	    )
    {
	x_force_connect = TRUE;
	setup_term_clip();
	x_force_connect = FALSE;
    }
}

    static int
check_connection(void)
{
    make_connection();
    if (X_DISPLAY == NULL)
    {
	emsg(_(e_no_connection_to_x_server));
	return FAIL;
    }
    return OK;
}
#endif

#ifdef FEAT_CLIENTSERVER
    static void
remote_common(typval_T *argvars, typval_T *rettv, int expr)
{
    char_u	*server_name;
    char_u	*keys;
    char_u	*r = NULL;
    char_u	buf[NUMBUFLEN];
    int		timeout = 0;
# ifdef MSWIN
    HWND	w;
# else
#ifdef FEAT_X11
    Window	w;
#endif
#ifdef FEAT_SOCKETSERVER
    char_u	*client = NULL;
#endif
# endif

    if (check_restricted() || check_secure())
	return;

# ifdef FEAT_X11
    if (check_connection() == FAIL)
	return;
# endif
    if (argvars[2].v_type != VAR_UNKNOWN
	    && argvars[3].v_type != VAR_UNKNOWN)
	timeout = tv_get_number(&argvars[3]);

    server_name = tv_get_string_chk(&argvars[0]);
    if (server_name == NULL)
	return;		// type error; errmsg already given
    keys = tv_get_string_buf(&argvars[1], buf);
# ifdef MSWIN
    if (serverSendToVim(server_name, keys, &r, &w, expr, timeout, TRUE) < 0)
# else
#ifdef FEAT_SOCKETSERVER
    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	if (socket_server_send(server_name, keys, &r, &client, expr,
		    timeout * 1000, TRUE) < 0)
	    goto fail;
#endif
#ifdef FEAT_X11
    if (clientserver_method == CLIENTSERVER_METHOD_X11)
	if (serverSendToVim(X_DISPLAY, server_name, keys, &r, &w, expr, timeout,
		    0, TRUE) < 0)
	    goto fail;
#endif
# endif
#if !defined(MSWIN)
    if (FALSE)
    {
fail:
#else
    {
#endif
	if (r != NULL)
	{
	    emsg((char *)r);	// sending worked but evaluation failed
	    vim_free(r);
#ifdef FEAT_SOCKETSERVER
	    vim_free(client);
#endif
	}
	else
	    semsg(_(e_unable_to_send_to_str), server_name);
	return;
    }

    rettv->vval.v_string = r;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	dictitem_T	v;
#if defined(FEAT_SOCKETSERVER)
	struct sockaddr_un addr;
	char_u		str[sizeof(addr.sun_path)];
#else
	char_u		str[30];
#endif
	char_u		*idvar;

	idvar = tv_get_string_chk(&argvars[2]);
	if (idvar != NULL && *idvar != NUL)
	{
	    str[0] = NUL;
#ifdef MSWIN
	    sprintf((char *)str, PRINTF_HEX_LONG_U, (long_u)w);
#else
#ifdef FEAT_X11
	    if (clientserver_method == CLIENTSERVER_METHOD_X11)
		sprintf((char *)str, PRINTF_HEX_LONG_U, (long_u)w);
#endif
#ifdef FEAT_SOCKETSERVER
	    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
		vim_snprintf((char *)str, sizeof(addr.sun_path),
			"%s", client);
#endif
#endif
	    v.di_tv.v_type = VAR_STRING;
	    v.di_tv.vval.v_string = vim_strsave(str);
	    set_var(idvar, &v.di_tv, FALSE);
	    vim_free(v.di_tv.vval.v_string);
	}
    }
#ifdef FEAT_SOCKETSERVER
    vim_free(client);
#endif
}
#endif

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * "remote_expr()" function
 */
    void
f_remote_expr(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

#ifdef FEAT_CLIENTSERVER
    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_string_arg(argvars, 1) == FAIL
		|| check_for_opt_string_arg(argvars, 2) == FAIL
		|| (argvars[2].v_type != VAR_UNKNOWN
		    && check_for_opt_number_arg(argvars, 3) == FAIL)))
	return;

    remote_common(argvars, rettv, TRUE);
#endif
}

/*
 * "remote_foreground()" function
 */
    void
f_remote_foreground(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_CLIENTSERVER
    if (in_vim9script() && check_for_string_arg(argvars, 0) == FAIL)
	return;

# ifdef MSWIN
    // On Win32 it's done in this application.
    {
	char_u	*server_name = tv_get_string_chk(&argvars[0]);

	if (server_name != NULL)
	    serverForeground(server_name);
    }
# else
    // Send a foreground() expression to the server.
    argvars[1].v_type = VAR_STRING;
    argvars[1].vval.v_string = vim_strsave((char_u *)"foreground()");
    argvars[2].v_type = VAR_UNKNOWN;
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    remote_common(argvars, rettv, TRUE);
    vim_free(argvars[1].vval.v_string);
# endif
#endif
}

    void
f_remote_peek(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_CLIENTSERVER
    dictitem_T	v;
    char_u	*s = NULL;
# ifdef MSWIN
    long_u	n = 0;
# endif
    char_u	*serverid;

    rettv->vval.v_number = -1;
    if (check_restricted() || check_secure())
	return;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_opt_string_arg(argvars, 1) == FAIL))
	return;

    serverid = tv_get_string_chk(&argvars[0]);
    if (serverid == NULL)
	return;		// type error; errmsg already given
# ifdef MSWIN
    sscanf((const char *)serverid, SCANF_HEX_LONG_U, &n);
    if (n == 0)
	rettv->vval.v_number = -1;
    else
    {
	s = serverGetReply((HWND)n, FALSE, FALSE, FALSE, 0);
	rettv->vval.v_number = (s != NULL);
    }
# else
#  ifdef FEAT_SOCKETSERVER
    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	rettv->vval.v_number = socket_server_peek_reply(serverid, &s);
#  endif
#  ifdef FEAT_X11
    if (clientserver_method == CLIENTSERVER_METHOD_X11)
    {
	if (check_connection() == FAIL)
	    return;

	rettv->vval.v_number = serverPeekReply(X_DISPLAY,
		serverStrToWin(serverid), &s);
    }
#  endif
# endif

    if (argvars[1].v_type != VAR_UNKNOWN && rettv->vval.v_number > 0)
    {
	char_u		*retvar;

	v.di_tv.v_type = VAR_STRING;
	v.di_tv.vval.v_string = vim_strsave(s);
	retvar = tv_get_string_chk(&argvars[1]);
	if (retvar != NULL)
	    set_var(retvar, &v.di_tv, FALSE);
	vim_free(v.di_tv.vval.v_string);
    }
#else
    rettv->vval.v_number = -1;
#endif
}

    void
f_remote_read(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	*r = NULL;

#ifdef FEAT_CLIENTSERVER
    char_u	*serverid;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_opt_number_arg(argvars, 1) == FAIL))
	return;

    serverid = tv_get_string_chk(&argvars[0]);
    if (serverid != NULL && !check_restricted() && !check_secure())
    {
	int timeout = 0;
# ifdef MSWIN
	// The server's HWND is encoded in the 'id' parameter
	long_u		n = 0;
# endif

	if (argvars[1].v_type != VAR_UNKNOWN)
	    timeout = tv_get_number(&argvars[1]);

# ifdef MSWIN
	sscanf((char *)serverid, SCANF_HEX_LONG_U, &n);
	if (n != 0)
	    r = serverGetReply((HWND)n, FALSE, TRUE, TRUE, timeout);
	if (r == NULL)
	    emsg(_(e_unable_to_read_server_reply));
# else
#  ifdef FEAT_SOCKETSERVER
	if (clientserver_method == CLIENTSERVER_METHOD_SOCKET &&
		socket_server_read_reply(serverid, &r, timeout * 1000) == FAIL)
	    emsg(_(e_unable_to_read_server_reply));
#  endif
#  ifdef FEAT_X11
	if (clientserver_method == CLIENTSERVER_METHOD_X11 &&
		(check_connection() == FAIL
		|| serverReadReply(X_DISPLAY, serverStrToWin(serverid),
						       &r, FALSE, timeout) < 0))
	    emsg(_(e_unable_to_read_server_reply));
#  endif
# endif
    }
#endif
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = r;
}

/*
 * "remote_send()" function
 */
    void
f_remote_send(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

#ifdef FEAT_CLIENTSERVER
    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_string_arg(argvars, 1) == FAIL
		|| check_for_opt_string_arg(argvars, 2) == FAIL))
	return;

    remote_common(argvars, rettv, FALSE);
#endif
}

/*
 * "remote_startserver()" function
 */
    void
f_remote_startserver(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_CLIENTSERVER
    if (check_for_nonempty_string_arg(argvars, 0) == FAIL)
	return;

    if (serverName != NULL)
    {
	emsg(_(e_already_started_server));
	return;
    }

    char_u *server = tv_get_string_chk(&argvars[0]);
# ifdef MSWIN
    serverSetName(server);
# else
# ifdef FEAT_SOCKETSERVER
    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	socket_server_init(server);
# endif
# ifdef FEAT_X11
    if (clientserver_method == CLIENTSERVER_METHOD_X11 &&
	    check_connection() == OK)
	serverRegisterName(X_DISPLAY, server);
# endif
# endif

#else
    emsg(_(e_clientserver_feature_not_available));
#endif
}

    void
f_server2client(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_CLIENTSERVER
    char_u	buf[NUMBUFLEN];
    char_u	*server;
    char_u	*reply;

    rettv->vval.v_number = -1;
    if (check_restricted() || check_secure())
	return;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_string_arg(argvars, 1) == FAIL))
	return;

    server = tv_get_string_chk(&argvars[0]);
    reply = tv_get_string_buf_chk(&argvars[1], buf);
    if (server == NULL || reply == NULL)
	return;

#ifdef FEAT_SOCKETSERVER
    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET &&
	    socket_server_send_reply(server, reply) == FAIL)
	goto fail;
#endif

#ifdef FEAT_X11
    if (clientserver_method == CLIENTSERVER_METHOD_X11 &&
	    check_connection() == FAIL)
	return;

    if (clientserver_method == CLIENTSERVER_METHOD_X11 &&
	    serverSendReply(server, reply) < 0)
#endif
#ifdef MSWIN
    if (serverSendReply(server, reply) < 0)
#endif
#if defined(FEAT_SOCKETSERVER) && !defined(FEAT_X11) && !defined(MSWIN)
    if (FALSE)
#endif
    {
#ifdef FEAT_SOCKETSERVER
fail:
#endif
	emsg(_(e_unable_to_send_to_client));
	return;
    }
    rettv->vval.v_number = 0;
#else
    rettv->vval.v_number = -1;
#endif
}

    void
f_serverlist(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	*r = NULL;

#ifdef FEAT_CLIENTSERVER
# ifdef MSWIN
    r = serverGetVimNames();
# else
#  ifdef FEAT_SOCKETSERVER
    if (clientserver_method == CLIENTSERVER_METHOD_SOCKET)
	r = socket_server_list_sockets();
#  endif
#  ifdef FEAT_X11
    if (clientserver_method == CLIENTSERVER_METHOD_X11)
    {
    make_connection();
    if (X_DISPLAY != NULL)
	r = serverGetVimNames(X_DISPLAY);
    }
#  endif
# endif
#endif
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = r;
}
#endif
