/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#if defined(MSDOS) || defined(WIN32) || defined(_WIN64)
# include <io.h>		/* for close() and dup() */
#endif

#define EXTERN
#include "vim.h"

#ifdef SPAWNO
# include <spawno.h>		/* special MSDOS swapping library */
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef __CYGWIN__
# ifndef WIN32
#  include <sys/cygwin.h>	/* for cygwin_conv_to_posix_path() */
# endif
# include <limits.h>
#endif

#if defined(UNIX) || defined(VMS)
static int file_owned __ARGS((char *fname));
#endif
static void mainerr __ARGS((int, char_u *));
static void main_msg __ARGS((char *s));
static void usage __ARGS((void));
static int get_number_arg __ARGS((char_u *p, int *idx, int def));
static void main_start_gui __ARGS((void));
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
static void check_swap_exists_action __ARGS((void));
#endif
#ifdef FEAT_CLIENTSERVER
static void cmdsrv_main __ARGS((int *argc, char **argv, char_u *serverName_arg, char_u **serverStr));
static char_u *serverMakeName __ARGS((char_u *arg, char *cmd));
#endif


#ifdef STARTUPTIME
static FILE *time_fd = NULL;
#endif

#define FEAT_PRECOMMANDS

/*
 * Different types of error messages.
 */
static char *(main_errors[]) =
{
    N_("Unknown option"),
#define ME_UNKNOWN_OPTION	0
    N_("Too many edit arguments"),
#define ME_TOO_MANY_ARGS	1
    N_("Argument missing after"),
#define ME_ARG_MISSING		2
    N_("Garbage after option"),
#define ME_GARBAGE		3
    N_("Too many \"+command\", \"-c command\" or \"--cmd command\" arguments"),
#define ME_EXTRA_CMD		4
    N_("Invalid argument for"),
#define ME_INVALID_ARG		5
};

/* Maximum number of commands from + or -c options */
#define MAX_ARG_CMDS 10

#ifndef PROTO	    /* don't want a prototype for main() */
    int
# ifdef VIMDLL
_export
# endif
# ifdef FEAT_GUI_MSWIN
#  ifdef __BORLANDC__
_cdecl
#  endif
VimMain
# else
main
# endif
(argc, argv)
    int		argc;
    char	**argv;
{
    char_u	*initstr;		/* init string from environment */
    char_u	*term = NULL;		/* specified terminal name */
    char_u	*fname = NULL;		/* file name from command line */
    char_u	*tagname = NULL;	/* tag from -t option */
    char_u	*use_vimrc = NULL;	/* vimrc from -u option */
#ifdef FEAT_QUICKFIX
    char_u	*use_ef = NULL;		/* 'errorfile' from -q option */
#endif
#ifdef FEAT_CRYPT
    int		ask_for_key = FALSE;    /* -x argument */
#endif
    int		n_commands = 0;		/* no. of commands from + or -c */
    char_u	*commands[MAX_ARG_CMDS]; /* commands from + or -c option */
#ifdef FEAT_PRECOMMANDS
    int		p_commands = 0;		/* no. of commands from --cmd */
    char_u	*pre_commands[MAX_ARG_CMDS]; /* commands from --cmd option */
#endif
    int		no_swap_file = FALSE;   /* "-n" option used */
    int		c;
    int		i;
    char_u	*p = NULL;
    int		bin_mode = FALSE;	/* -b option used */
#ifdef FEAT_EVAL
    int		use_debug_break_level = -1;
#endif
#ifdef FEAT_WINDOWS
    int		window_count = -1;	/* number of windows to use */
    int		arg_idx;		/* index in argument list */
    int		vert_windows = MAYBE;   /* "-O" used instead of "-o" */
#endif
    int		had_minmin = FALSE;	/* found "--" option */
    int		argv_idx;		/* index in argv[n][] */
    int		want_full_screen = TRUE;
    int		want_argument;		/* option with argument */
#define EDIT_NONE   0	    /* no edit type yet */
#define EDIT_FILE   1	    /* file name argument[s] given, use argument list */
#define EDIT_STDIN  2	    /* read file from stdin */
#define EDIT_TAG    3	    /* tag name argument given, use tagname */
#define EDIT_QF	    4	    /* start in quickfix mode */
    int		edit_type = EDIT_NONE;  /* type of editing to do */
#ifdef FEAT_DIFF
    int		diff_mode = FALSE;	/* start with 'diff' set */
#endif
    int		evim_mode = FALSE;	/* started as "evim" */
    int		stdout_isatty;		/* is stdout a terminal? */
    int		input_isatty;		/* is active input a terminal? */
#ifdef MSWIN
    int		full_path = FALSE;
#endif
#ifdef FEAT_CLIENTSERVER
    char_u	*serverStr = NULL;
    char_u	*serverName_arg = NULL;	/* cmdline arg for server name */
    int		serverArg = FALSE;	/* TRUE when argument for a server */
    char_u	*servername = NULL;	/* allocated name for our server */
#endif
#if (!defined(UNIX) && !defined(__EMX__)) || defined(ARCHIE)
    int		literal = FALSE;	/* don't expand file names */
#endif

# ifdef NBDEBUG
    nbdebug_log_init("SPRO_GVIM_DEBUG", "SPRO_GVIM_DLEVEL");
    nbdebug_wait(WT_ENV | WT_WAIT | WT_STOP, "SPRO_GVIM_WAIT", 20);
# endif

    /*
     * Do any system-specific initialisations.  These can NOT use IObuff or
     * NameBuff.  Thus emsg2() cannot be called!
     */
    mch_early_init();

#ifdef FEAT_TCL
    vim_tcl_init(argv[0]);
#endif

#ifdef MEM_PROFILE
    atexit(vim_mem_profile_dump);
#endif

#ifdef STARTUPTIME
    time_fd = fopen(STARTUPTIME, "a");
    TIME_MSG("--- VIM STARTING ---");
#endif

#ifdef __EMX__
    _wildcard(&argc, &argv);
#endif

#ifdef FEAT_MBYTE
    (void)mb_init();	/* init mb_bytelen_tab[] to ones */
#endif

#ifdef __QNXNTO__
    qnx_init();		/* PhAttach() for clipboard, (and gui) */
#endif

#ifdef MAC_OS_CLASSIC
    /* Macintosh needs this before any memory is allocated. */
    gui_prepare(&argc, argv);	/* Prepare for possibly starting GUI sometime */
    TIME_MSG("GUI prepared");
#endif

    /* Init the table of Normal mode commands. */
    init_normal_cmds();

#if defined(HAVE_DATE_TIME) && defined(VMS) && defined(VAXC)
    make_version();
#endif

    /*
     * Allocate space for the generic buffers (needed for set_init_1() and
     * EMSG2()).
     */
    if ((IObuff = alloc(IOSIZE)) == NULL
	    || (NameBuff = alloc(MAXPATHL)) == NULL)
	mch_exit(0);

    TIME_MSG("Allocated generic buffers");

#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    /*
     * Setup to use the current locale (for ctype() and many other things).
     * NOTE: Translated messages with encodings other than latin1 will not
     * work until set_init_1() has been called!
     */
    setlocale(LC_ALL, "");

# ifdef FEAT_GETTEXT
    {
	int	mustfree = FALSE;

#  ifdef DYNAMIC_GETTEXT
	/* Initialize the gettext library */
	dyn_libintl_init(NULL);
#  endif
	/* expand_env() doesn't work yet, because chartab[] is not initialized
	 * yet, call vim_getenv() directly */
	p = vim_getenv((char_u *)"VIMRUNTIME", &mustfree);
	if (p != NULL && *p != NUL)
	{
	    STRCPY(NameBuff, p);
	    STRCAT(NameBuff, "/lang");
	    bindtextdomain(VIMPACKAGE, (char *)NameBuff);
	}
	if (mustfree)
	    vim_free(p);
	textdomain(VIMPACKAGE);
    }
# endif
    TIME_MSG("locale set");
#endif

#ifdef FEAT_GUI
    gui.dofork = TRUE;		    /* default is to use fork() */
#endif

#if defined(FEAT_XCLIPBOARD) || defined(FEAT_CLIENTSERVER)
    /*
     * Get the name of the display, before gui_prepare() removes it from
     * argv[].  Used for the xterm-clipboard display.
     *
     * Also find the --server... arguments
     */
    for (i = 1; i < argc; i++)
    {
	if (STRCMP(argv[i], "--") == 0)
	    break;
# ifdef FEAT_XCLIPBOARD
	else if (STRICMP(argv[i], "-display") == 0
#  ifdef FEAT_GUI_GTK
		|| STRICMP(argv[i], "--display") == 0
#  endif
		)
	{
	    if (i == argc - 1)
		mainerr_arg_missing((char_u *)argv[i]);
	    xterm_display = argv[++i];
	}
# endif
# ifdef FEAT_CLIENTSERVER
	else if (STRICMP(argv[i], "--servername") == 0)
	{
	    if (i == argc - 1)
		mainerr_arg_missing((char_u *)argv[i]);
	    serverName_arg = (char_u *)argv[++i];
	}
	else if (STRICMP(argv[i], "--serverlist") == 0
		 || STRICMP(argv[i], "--remote-send") == 0
		 || STRICMP(argv[i], "--remote-expr") == 0
		 || STRICMP(argv[i], "--remote") == 0
		 || STRICMP(argv[i], "--remote-silent") == 0)
	    serverArg = TRUE;
	else if (STRICMP(argv[i], "--remote-wait") == 0
		|| STRICMP(argv[i], "--remote-wait-silent") == 0)
	{
	    serverArg = TRUE;
#ifdef FEAT_GUI
	    /* don't fork() when starting the GUI to edit the files ourself */
	    gui.dofork = FALSE;
#endif
	}
# endif
# ifdef FEAT_GUI_GTK
	else if (STRICMP(argv[i], "--socketid") == 0)
	{
	    unsigned int    socket_id;
	    int		    count;

	    if (i == argc - 1)
		mainerr_arg_missing((char_u *)argv[i]);
	    if (STRNICMP(argv[i+1], "0x", 2) == 0)
		count = sscanf(&(argv[i + 1][2]), "%x", &socket_id);
	    else
		count = sscanf(argv[i+1], "%u", &socket_id);
	    if (count != 1)
		mainerr(ME_INVALID_ARG, (char_u *)argv[i]);
	    else
		gtk_socket_id = socket_id;
	    i++;
	}
	else if (STRICMP(argv[i], "--echo-wid") == 0)
	    echo_wid_arg = TRUE;
# endif
    }
#endif

#ifdef FEAT_SUN_WORKSHOP
    findYourself(argv[0]);
#endif
#if defined(FEAT_GUI) && !defined(MAC_OS_CLASSIC)
    gui_prepare(&argc, argv);	/* Prepare for possibly starting GUI sometime */
    TIME_MSG("GUI prepared");
#endif

#ifdef FEAT_CLIPBOARD
    clip_init(FALSE);		/* Initialise clipboard stuff */
    TIME_MSG("clipboard setup");
#endif

    /*
     * Check if we have an interactive window.
     * On the Amiga: If there is no window, we open one with a newcli command
     * (needed for :! to * work). mch_check_win() will also handle the -d or
     * -dev argument.
     */
    stdout_isatty = (mch_check_win(argc, argv) != FAIL);
    TIME_MSG("window checked");

    /*
     * Allocate the first window and buffer. Can't do much without it.
     */
    win_alloc_first();

    init_yank();		/* init yank buffers */

    /* Init the argument list to empty. */
    alist_init(&global_alist);

    /*
     * Set the default values for the options.
     * NOTE: Non-latin1 translated messages are working only after this,
     * because this is where "has_mbyte" will be set, which is used by
     * msg_outtrans_len_attr().
     * First find out the home directory, needed to expand "~" in options.
     */
    init_homedir();		/* find real value of $HOME */
    set_init_1();
    TIME_MSG("inits 1");

#ifdef FEAT_EVAL
    set_lang_var();		/* set v:lang and v:ctype */
#endif

#ifdef FEAT_CLIENTSERVER
    /*
     * Do the client-server stuff, unless "--servername ''" was used.
     */
    if (serverName_arg == NULL || *serverName_arg != NUL)
    {
# ifdef WIN32
	/* Initialise the client/server messaging infrastructure. */
	serverInitMessaging();
# endif

	/*
	 * When a command server argument was found, execute it.  This may
	 * exit Vim when it was successful.
	 */
	if (serverArg)
	    cmdsrv_main(&argc, argv, serverName_arg, &serverStr);

	/* If we're still running, get the name to register ourselves.
	 * On Win32 can register right now, for X11 need to setup the
	 * clipboard first, it's further down. */
	servername = serverMakeName(serverName_arg, argv[0]);
# ifdef WIN32
	if (servername != NULL)
	{
	    serverSetName(servername);
	    vim_free(servername);
	}
# endif
    }
#endif

    /*
     * Check for: [r][e][g][vi|vim|view][diff][ex[im]]
     * If the executable name starts with "r" we disable shell commands.
     * If the next character is "e" we run in Easy mode.
     * If the next character is "g" we run the GUI version.
     * If the next characters are "view" we start in readonly mode.
     * If the next characters are "diff" or "vimdiff" we start in diff mode.
     * If the next characters are "ex" we start in Ex mode.  If it's followed
     * by "im" use improved Ex mode.
     */
    initstr = gettail((char_u *)argv[0]);

#ifdef MACOS_X_UNIX
    /* An issue has been seen when launching Vim in such a way that
     * $PWD/$ARGV[0] or $ARGV[0] is not the absolute path to the
     * executable or a symbolic link of it. Until this issue is resolved
     * we prohibit the GUI from being used.
     */
    if (STRCMP(initstr, argv[0]) == 0)
	disallow_gui = TRUE;
#endif

#ifdef FEAT_EVAL
    set_vim_var_string(VV_PROGNAME, initstr, -1);
#endif

    /* TODO: On MacOS X default to gui if argv[0] ends in:
     *       /vim.app/Contents/MacOS/Vim */

    if (TOLOWER_ASC(initstr[0]) == 'r')
    {
	restricted = TRUE;
	++initstr;
    }

    /* Avoid using evim mode for "editor". */
    if (TOLOWER_ASC(initstr[0]) == 'e'
	    && (TOLOWER_ASC(initstr[1]) == 'v'
					   || TOLOWER_ASC(initstr[1]) == 'g'))
    {
#ifdef FEAT_GUI
	gui.starting = TRUE;
#endif
	evim_mode = TRUE;
	++initstr;
    }

    if (TOLOWER_ASC(initstr[0]) == 'g')
    {
	main_start_gui();
#ifdef FEAT_GUI
	++initstr;
#endif
    }

    if (STRNICMP(initstr, "view", 4) == 0)
    {
	readonlymode = TRUE;
	curbuf->b_p_ro = TRUE;
	p_uc = 10000;			/* don't update very often */
	initstr += 4;
    }
    else if (STRNICMP(initstr, "vim", 3) == 0)
	initstr += 3;

    /* Catch "[r][g]vimdiff" and "[r][g]viewdiff". */
    if (STRICMP(initstr, "diff") == 0)
    {
#ifdef FEAT_DIFF
	diff_mode = TRUE;
#else
	mch_errmsg(_("This Vim was not compiled with the diff feature."));
	mch_errmsg("\n");
	mch_exit(2);
#endif
    }

    if (STRNICMP(initstr, "ex", 2) == 0)
    {
	if (STRNICMP(initstr + 2, "im", 2) == 0)
	    exmode_active = EXMODE_VIM;
	else
	    exmode_active = EXMODE_NORMAL;
	change_compatible(TRUE);	/* set 'compatible' */
    }

    initstr = gettail((char_u *)argv[0]);
    ++argv;
    --argc;

    /*
     * Process the command line arguments.
     */
    argv_idx = 1;	    /* active option letter is argv[0][argv_idx] */
    while (argc > 0)
    {
	/*
	 * "+" or "+{number}" or "+/{pat}" or "+{command}" argument.
	 */
	if (argv[0][0] == '+' && !had_minmin)
	{
	    if (n_commands >= MAX_ARG_CMDS)
		mainerr(ME_EXTRA_CMD, NULL);
	    argv_idx = -1;	    /* skip to next argument */
	    if (argv[0][1] == NUL)
		commands[n_commands++] = (char_u *)"$";
	    else
		commands[n_commands++] = (char_u *)&(argv[0][1]);
	}

	/*
	 * Optional argument.
	 */
	else if (argv[0][0] == '-' && !had_minmin)
	{
	    want_argument = FALSE;
	    c = argv[0][argv_idx++];
#ifdef VMS
	    /*
	     * VMS only uses upper case command lines.  Interpret "-X" as "-x"
	     * and "-/X" as "-X".
	     */
	    if (c == '/')
	    {
		c = argv[0][argv_idx++];
		c = TOUPPER_ASC(c);
	    }
	    else
		c = TOLOWER_ASC(c);
#endif
	    switch (c)
	    {
	    case NUL:		/* "vim -"  read from stdin */
				/* "ex -" silent mode */
		if (exmode_active)
		    silent_mode = TRUE;
		else
		{
		    if (edit_type != EDIT_NONE)
			mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
		    edit_type = EDIT_STDIN;
		    read_cmd_fd = 2;	/* read from stderr instead of stdin */
		}
		argv_idx = -1;		/* skip to next argument */
		break;

	    case '-':		/* "--" don't take any more options */
				/* "--help" give help message */
				/* "--version" give version message */
				/* "--literal" take files literally */
				/* "--nofork" don't fork */
				/* "--noplugin[s]" skip plugins */
				/* "--cmd <cmd>" execute cmd before vimrc */
		if (STRICMP(argv[0] + argv_idx, "help") == 0)
		    usage();
		else if (STRICMP(argv[0] + argv_idx, "version") == 0)
		{
		    Columns = 80;	/* need to init Columns */
		    info_message = TRUE; /* use mch_msg(), not mch_errmsg() */
		    list_version();
		    msg_putchar('\n');
		    msg_didout = FALSE;
		    mch_exit(0);
		}
		else if (STRNICMP(argv[0] + argv_idx, "literal", 7) == 0)
		{
#if (!defined(UNIX) && !defined(__EMX__)) || defined(ARCHIE)
		    literal = TRUE;
#endif
		}
		else if (STRNICMP(argv[0] + argv_idx, "nofork", 6) == 0)
		{
#ifdef FEAT_GUI
		    gui.dofork = FALSE;	/* don't fork() when starting GUI */
#endif
		}
		else if (STRNICMP(argv[0] + argv_idx, "noplugin", 8) == 0)
		    p_lpl = FALSE;
#ifdef FEAT_PRECOMMANDS
		else if (STRNICMP(argv[0] + argv_idx, "cmd", 3) == 0)
		{
		    want_argument = TRUE;
		    argv_idx += 3;
		}
#endif
#ifdef FEAT_CLIENTSERVER
		else if (STRNICMP(argv[0] + argv_idx, "serverlist", 10) == 0)
		    ; /* already processed -- no arg */
		else if (STRNICMP(argv[0] + argv_idx, "servername", 10) == 0
		       || STRNICMP(argv[0] + argv_idx, "serversend", 10) == 0)
		{
		    /* already processed -- snatch the following arg */
		    if (argc > 1)
		    {
			--argc;
			++argv;
		    }
		}
#endif
#ifdef FEAT_GUI_GTK
		else if (STRNICMP(argv[0] + argv_idx, "socketid", 8) == 0)
		{
		    /* already processed -- snatch the following arg */
		    if (argc > 1)
		    {
			--argc;
			++argv;
		    }
		}
		else if (STRNICMP(argv[0] + argv_idx, "echo-wid", 8) == 0)
		{
		    /* already processed, skip */
		}
#endif
		else
		{
		    if (argv[0][argv_idx])
			mainerr(ME_UNKNOWN_OPTION, (char_u *)argv[0]);
		    had_minmin = TRUE;
		}
		if (!want_argument)
		    argv_idx = -1;	/* skip to next argument */
		break;

	    case 'A':		/* "-A" start in Arabic mode */
#ifdef FEAT_ARABIC
		set_option_value((char_u *)"arabic", 1L, NULL, 0);
#else
		mch_errmsg(_(e_noarabic));
		mch_exit(2);
#endif
		break;

	    case 'b':		/* "-b" binary mode */
		bin_mode = TRUE;    /* postpone to after reading .exrc files */
		break;

	    case 'C':		/* "-C"  Compatible */
		change_compatible(TRUE);
		break;

	    case 'e':		/* "-e" Ex mode */
		exmode_active = EXMODE_NORMAL;
		break;

	    case 'E':		/* "-E" Improved Ex mode */
		exmode_active = EXMODE_VIM;
		break;

	    case 'f':		/* "-f"  GUI: run in foreground.  Amiga: open
				window directly, not with newcli */
#ifdef FEAT_GUI
		gui.dofork = FALSE;	/* don't fork() when starting GUI */
#endif
		break;

	    case 'g':		/* "-g" start GUI */
		main_start_gui();
		break;

	    case 'F':		/* "-F" start in Farsi mode: rl + fkmap set */
#ifdef FEAT_FKMAP
		curwin->w_p_rl = p_fkmap = TRUE;
#else
		mch_errmsg(_(e_nofarsi));
		mch_exit(2);
#endif
		break;

	    case 'h':		/* "-h" give help message */
#ifdef FEAT_GUI_GNOME
		/* Tell usage() to exit for "gvim". */
		gui.starting = FALSE;
#endif
		usage();
		break;

	    case 'H':		/* "-H" start in Hebrew mode: rl + hkmap set */
#ifdef FEAT_RIGHTLEFT
		curwin->w_p_rl = p_hkmap = TRUE;
#else
		mch_errmsg(_(e_nohebrew));
		mch_exit(2);
#endif
		break;

	    case 'l':		/* "-l" lisp mode, 'lisp' and 'showmatch' on */
#ifdef FEAT_LISP
		set_option_value((char_u *)"lisp", 1L, NULL, 0);
		p_sm = TRUE;
#endif
		break;

#ifdef TARGET_API_MAC_OSX
		/* For some reason on MacOS X, an argument like:
		   -psn_0_10223617 is passed in when invoke from Finder
		   or with the 'open' command */
	    case 'p':
		argv_idx = -1; /* bypass full -psn */
		main_start_gui();
		break;
#endif
	    case 'M':		/* "-M"  no changes or writing of files */
		reset_modifiable();
		/* FALLTRHOUGH */

	    case 'm':		/* "-m"  no writing of files */
		p_write = FALSE;
		break;

	    case 'y':		/* "-y"  easy mode */
#ifdef FEAT_GUI
		gui.starting = TRUE;	/* start GUI a bit later */
#endif
		evim_mode = TRUE;
		break;

	    case 'N':		/* "-N"  Nocompatible */
		change_compatible(FALSE);
		break;

	    case 'n':		/* "-n" no swap file */
		no_swap_file = TRUE;
		break;

	    case 'o':		/* "-o[N]" open N horizontal split windows */
#ifdef FEAT_WINDOWS
		/* default is 0: open window for each file */
		window_count = get_number_arg((char_u *)argv[0], &argv_idx, 0);
		vert_windows = FALSE;
#endif
		break;

		case 'O':	/* "-O[N]" open N vertical split windows */
#if defined(FEAT_VERTSPLIT) && defined(FEAT_WINDOWS)
		/* default is 0: open window for each file */
		window_count = get_number_arg((char_u *)argv[0], &argv_idx, 0);
		vert_windows = TRUE;
#endif
		break;

#ifdef FEAT_QUICKFIX
	    case 'q':		/* "-q" QuickFix mode */
		if (edit_type != EDIT_NONE)
		    mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
		edit_type = EDIT_QF;
		if (argv[0][argv_idx])		/* "-q{errorfile}" */
		{
		    use_ef = (char_u *)argv[0] + argv_idx;
		    argv_idx = -1;
		}
		else if (argc > 1)		/* "-q {errorfile}" */
		    want_argument = TRUE;
		break;
#endif

	    case 'R':		/* "-R" readonly mode */
		readonlymode = TRUE;
		curbuf->b_p_ro = TRUE;
		p_uc = 10000;			/* don't update very often */
		break;

	    case 'r':		/* "-r" recovery mode */
	    case 'L':		/* "-L" recovery mode */
		recoverymode = 1;
		break;

	    case 's':
		if (exmode_active)	/* "-s" silent (batch) mode */
		    silent_mode = TRUE;
		else		/* "-s {scriptin}" read from script file */
		    want_argument = TRUE;
		break;

	    case 't':		/* "-t {tag}" or "-t{tag}" jump to tag */
		if (edit_type != EDIT_NONE)
		    mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
		edit_type = EDIT_TAG;
		if (argv[0][argv_idx])		/* "-t{tag}" */
		{
		    tagname = (char_u *)argv[0] + argv_idx;
		    argv_idx = -1;
		}
		else				/* "-t {tag}" */
		    want_argument = TRUE;
		break;

#ifdef FEAT_EVAL
	    case 'D':		/* "-D"		Debugging */
		use_debug_break_level = 9999;
		break;
#endif
#ifdef FEAT_DIFF
	    case 'd':		/* "-d"		'diff' */
# ifdef AMIGA
		/* check for "-dev {device}" */
		if (argv[0][argv_idx] == 'e' && argv[0][argv_idx + 1] == 'v')
		    want_argument = TRUE;
		else
# endif
		    diff_mode = TRUE;
		break;
#endif
	    case 'V':		/* "-V{N}"	Verbose level */
		/* default is 10: a little bit verbose */
		p_verbose = get_number_arg((char_u *)argv[0], &argv_idx, 10);
		break;

	    case 'v':		/* "-v"  Vi-mode (as if called "vi") */
		exmode_active = 0;
#ifdef FEAT_GUI
		gui.starting = FALSE;	/* don't start GUI */
#endif
		break;

	    case 'w':		/* "-w{number}"	set window height */
				/* "-w {scriptout}"	write to script */
		if (vim_isdigit(((char_u *)argv[0])[argv_idx]))
		{
		    argv_idx = -1;
		    break;			/* not implemented, ignored */
		}
		want_argument = TRUE;
		break;

#ifdef FEAT_CRYPT
	    case 'x':		/* "-x"  encrypted reading/writing of files */
		ask_for_key = TRUE;
		break;
#endif

	    case 'X':		/* "-X"  don't connect to X server */
#if (defined(UNIX) || defined(VMS)) && defined(FEAT_X11)
		x_no_connect = TRUE;
#endif
		break;

	    case 'Z':		/* "-Z"  restricted mode */
		restricted = TRUE;
		break;

	    case 'c':		/* "-c {command}" execute command */
	    case 'S':		/* "-S {file}" execute Vim script */
	    case 'i':		/* "-i {viminfo}" use for viminfo */
#ifndef FEAT_DIFF
	    case 'd':		/* "-d {device}" device (for Amiga) */
#endif
	    case 'T':		/* "-T {terminal}" terminal name */
	    case 'u':		/* "-u {vimrc}" vim inits file */
	    case 'U':		/* "-U {gvimrc}" gvim inits file */
	    case 'W':		/* "-W {scriptout}" overwrite */
#ifdef FEAT_GUI_W32
	    case 'P':		/* "-P {parent title}" MDI parent */
#endif
		want_argument = TRUE;
		break;

	    default:
		mainerr(ME_UNKNOWN_OPTION, (char_u *)argv[0]);
	    }

	    /*
	     * Handle options with argument.
	     */
	    if (want_argument)
	    {
		/*
		 * Check for garbage immediately after the option letter.
		 */
		if (argv[0][argv_idx] != NUL)
		    mainerr(ME_GARBAGE, (char_u *)argv[0]);

		--argc;
		if (argc < 1 && c != 'S')
		    mainerr_arg_missing((char_u *)argv[0]);
		++argv;
		argv_idx = -1;

		switch (c)
		{
		case 'c':	/* "-c {command}" execute command */
		case 'S':	/* "-S {file}" execute Vim script */
		    if (n_commands >= MAX_ARG_CMDS)
			mainerr(ME_EXTRA_CMD, NULL);
		    if (c == 'S')
		    {
			char	*a;

			if (argc < 1)
			    /* "-S" without argument: use default session file
			     * name. */
			    a = SESSION_FILE;
			else if (argv[0][0] == '-')
			{
			    /* "-S" followed by another option: use default
			     * session file name. */
			    a = SESSION_FILE;
			    ++argc;
			    --argv;
			}
			else
			    a = argv[0];
			p = alloc((unsigned)(STRLEN(a) + 4));
			if (p == NULL)
			    mch_exit(2);
			sprintf((char *)p, "so %s", a);
			commands[n_commands++] = p;
		    }
		    else
			commands[n_commands++] = (char_u *)argv[0];
		    break;

#ifdef FEAT_PRECOMMANDS
		case '-':	/* "--cmd {command}" execute command */
		    if (p_commands >= MAX_ARG_CMDS)
			mainerr(ME_EXTRA_CMD, NULL);
		    pre_commands[p_commands++] = (char_u *)argv[0];
		    break;
#endif

	    /*	case 'd':   -d {device} is handled in mch_check_win() for the
	     *		    Amiga */

#ifdef FEAT_QUICKFIX
		case 'q':	/* "-q {errorfile}" QuickFix mode */
		    use_ef = (char_u *)argv[0];
		    break;
#endif

		case 'i':	/* "-i {viminfo}" use for viminfo */
		    use_viminfo = (char_u *)argv[0];
		    break;

		case 's':	/* "-s {scriptin}" read from script file */
		    if (scriptin[0] != NULL)
		    {
scripterror:
			mch_errmsg(_("Attempt to open script file again: \""));
			mch_errmsg(argv[-1]);
			mch_errmsg(" ");
			mch_errmsg(argv[0]);
			mch_errmsg("\"\n");
			mch_exit(2);
		    }
		    if ((scriptin[0] = mch_fopen(argv[0], READBIN)) == NULL)
		    {
			mch_errmsg(_("Cannot open for reading: \""));
			mch_errmsg(argv[0]);
			mch_errmsg("\"\n");
			mch_exit(2);
		    }
		    if (save_typebuf() == FAIL)
			mch_exit(2);	/* out of memory */
		    break;

		case 't':	/* "-t {tag}" */
		    tagname = (char_u *)argv[0];
		    break;

		case 'T':	/* "-T {terminal}" terminal name */
		    /*
		     * The -T term option is always available and when
		     * HAVE_TERMLIB is supported it overrides the environment
		     * variable TERM.
		     */
#ifdef FEAT_GUI
		    if (term_is_gui((char_u *)argv[0]))
			gui.starting = TRUE;	/* start GUI a bit later */
		    else
#endif
			term = (char_u *)argv[0];
		    break;

		case 'u':	/* "-u {vimrc}" vim inits file */
		    use_vimrc = (char_u *)argv[0];
		    break;

		case 'U':	/* "-U {gvimrc}" gvim inits file */
#ifdef FEAT_GUI
		    use_gvimrc = (char_u *)argv[0];
#endif
		    break;

		case 'w':	/* "-w {scriptout}" append to script file */
		case 'W':	/* "-W {scriptout}" overwrite script file */
		    if (scriptout != NULL)
			goto scripterror;
		    if ((scriptout = mch_fopen(argv[0],
				    c == 'w' ? APPENDBIN : WRITEBIN)) == NULL)
		    {
			mch_errmsg(_("Cannot open for script output: \""));
			mch_errmsg(argv[0]);
			mch_errmsg("\"\n");
			mch_exit(2);
		    }
		    break;

#ifdef FEAT_GUI_W32
		case 'P':		/* "-P {parent title}" MDI parent */
		    gui_mch_set_parent(argv[0]);
		    break;
#endif
		}
	    }
	}

	/*
	 * File name argument.
	 */
	else
	{
	    argv_idx = -1;	    /* skip to next argument */

	    /* Check for only one type of editing. */
	    if (edit_type != EDIT_NONE && edit_type != EDIT_FILE)
		mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
	    edit_type = EDIT_FILE;

#ifdef MSWIN
	    /* Remember if the argument was a full path before changing
	     * slashes to backslashes. */
	    if (argv[0][0] != NUL && argv[0][1] == ':' && argv[0][2] == '\\')
		full_path = TRUE;
#endif

	    /* Add the file to the global argument list. */
	    if (ga_grow(&global_alist.al_ga, 1) == FAIL
		    || (p = vim_strsave((char_u *)argv[0])) == NULL)
		mch_exit(2);
#ifdef FEAT_DIFF
	    if (diff_mode && mch_isdir(p) && GARGCOUNT > 0
				      && !mch_isdir(alist_name(&GARGLIST[0])))
	    {
		char_u	    *r;

		r = concat_fnames(p, gettail(alist_name(&GARGLIST[0])), TRUE);
		if (r != NULL)
		{
		    vim_free(p);
		    p = r;
		}
	    }
#endif
#if defined(__CYGWIN32__) && !defined(WIN32)
	    /*
	     * If vim is invoked by non-Cygwin tools, convert away any
	     * DOS paths, so things like .swp files are created correctly.
	     * Look for evidence of non-Cygwin paths before we bother.
	     * This is only for when using the Unix files.
	     */
	    if (strpbrk(p, "\\:") != NULL)
	    {
		char posix_path[PATH_MAX];

		cygwin_conv_to_posix_path(p, posix_path);
		vim_free(p);
		p = vim_strsave(posix_path);
		if (p == NULL)
		    mch_exit(2);
	    }
#endif
	    alist_add(&global_alist, p,
#if (!defined(UNIX) && !defined(__EMX__)) || defined(ARCHIE)
		    literal ? 2 : 0	/* add buffer number after expanding */
#else
		    2		/* add buffer number now and use curbuf */
#endif
		    );
	}

	/*
	 * If there are no more letters after the current "-", go to next
	 * argument.  argv_idx is set to -1 when the current argument is to be
	 * skipped.
	 */
	if (argv_idx <= 0 || argv[0][argv_idx] == NUL)
	{
	    --argc;
	    ++argv;
	    argv_idx = 1;
	}
    }
    TIME_MSG("parsing arguments");

    /*
     * On some systems, when we compile with the GUI, we always use it.  On Mac
     * there is no terminal version, and on Windows we can't figure out how to
     * fork one off with :gui.
     */
#ifdef ALWAYS_USE_GUI
    gui.starting = TRUE;
#else
# if defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK)
    /*
     * Check if the GUI can be started.  Reset gui.starting if not.
     * Don't know about other systems, stay on the safe side and don't check.
     */
    if (gui.starting && gui_init_check() == FAIL)
    {
	gui.starting = FALSE;

	/* When running "evim" or "gvim -y" we need the menus, exit if we
	 * don't have them. */
	if (evim_mode)
	    mch_exit(1);
    }
# endif
#endif

    /* "-b" argument used.  Check before expanding file names, because for
     * Win32 this makes us edit a shortcut file itself, instead of the file it
     * links to. */
    if (bin_mode)
    {
	set_options_bin(curbuf->b_p_bin, 1, 0);
	curbuf->b_p_bin = 1;	    /* binary file I/O */
    }

    if (GARGCOUNT > 0)
    {
#if (!defined(UNIX) && !defined(__EMX__)) || defined(ARCHIE)
	/*
	 * Expand wildcards in file names.
	 */
	if (!literal)
	{
	    /* Temporarily add '(' and ')' to 'isfname'.  These are valid
	     * filename characters but are excluded from 'isfname' to make
	     * "gf" work on a file name in parenthesis (e.g.: see vim.h). */
	    do_cmdline_cmd((char_u *)":set isf+=(,)");
	    alist_expand();
	    do_cmdline_cmd((char_u *)":set isf&");
	}
#endif
	fname = alist_name(&GARGLIST[0]);
    }
    if (GARGCOUNT > 1)
	printf(_("%d files to edit\n"), GARGCOUNT);
#ifdef MSWIN
    else if (GARGCOUNT == 1 && full_path)
    {
	/*
	 * If there is one filename, fully qualified, we have very probably
	 * been invoked from explorer, so change to the file's directory.
	 * Hint: to avoid this when typing a command use a forward slash.
	 * If the cd fails, it doesn't matter.
	 */
	(void)vim_chdirfile(fname);
    }
#endif
    TIME_MSG("expanding arguments");

#ifdef FEAT_DIFF
    if (diff_mode)
    {
	if (window_count == -1)
	    window_count = 0;		/* open up to 3 files in a window */
	if (vert_windows == MAYBE)
	    vert_windows = TRUE;	/* use vertical split */
    }
#endif

    ++RedrawingDisabled;

    /*
     * When listing swap file names, don't do cursor positioning et. al.
     */
    if (recoverymode && fname == NULL)
	want_full_screen = FALSE;

    /*
     * When certain to start the GUI, don't check capabilities of terminal.
     * For GTK we can't be sure, but when started from the desktop it doesn't
     * make sense to try using a terminal.
     */
#if defined(ALWAYS_USE_GUI) || defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK)
    if (gui.starting
# ifdef FEAT_GUI_GTK
	    && !isatty(2)
# endif
	    )
	want_full_screen = FALSE;
#endif

#if defined(FEAT_GUI_MAC) && defined(MACOS_X_UNIX)
    /* When the GUI is started from Finder, need to display messages in a
     * message box.  isatty(2) returns TRUE anyway, thus we need to check the
     * name to know we're not started from a terminal. */
    if (gui.starting && (!isatty(2) || strcmp("/dev/console", ttyname(2)) == 0))
	want_full_screen = FALSE;
#endif

    /*
     * mch_init() sets up the terminal (window) for use.  This must be
     * done after resetting full_screen, otherwise it may move the cursor
     * (MSDOS).
     * Note that we may use mch_exit() before mch_init()!
     */
    mch_init();
    TIME_MSG("shell init");

#ifdef USE_XSMP
    /*
     * For want of anywhere else to do it, try to connect to xsmp here.
     * Fitting it in after gui_mch_init, but before gui_init (via termcapinit).
     * Hijacking -X 'no X connection' to also disable XSMP connection as that
     * has a similar delay upon failure.
     * Only try if SESSION_MANAGER is set to something non-null.
     */
    if (!x_no_connect)
    {
	p = (char_u *)getenv("SESSION_MANAGER");
	if (p != NULL && *p != NUL)
	{
	    xsmp_init();
	    TIME_MSG("xsmp init");
	}
    }
#endif

    /*
     * Print a warning if stdout is not a terminal.
     * When starting in Ex mode and commands come from a file, set Silent mode.
     */
    input_isatty = mch_input_isatty();
    if (exmode_active)
    {
	if (!input_isatty)
	    silent_mode = TRUE;
    }
    else if (want_full_screen && (!stdout_isatty || !input_isatty)
#ifdef FEAT_GUI
	    /* don't want the delay when started from the desktop */
	    && !gui.starting
#endif
	    )
    {
	if (!stdout_isatty)
	    mch_errmsg(_("Vim: Warning: Output is not to a terminal\n"));
	if (!input_isatty)
	    mch_errmsg(_("Vim: Warning: Input is not from a terminal\n"));
	out_flush();
	if (scriptin[0] == NULL)
	    ui_delay(2000L, TRUE);
	TIME_MSG("Warning delay");
    }

    if (want_full_screen)
    {
	termcapinit(term);	/* set terminal name and get terminal
				   capabilities (will set full_screen) */
	screen_start();		/* don't know where cursor is now */
	TIME_MSG("Termcap init");
    }

    /*
     * Set the default values for the options that use Rows and Columns.
     */
    ui_get_shellsize();		/* inits Rows and Columns */
#ifdef FEAT_NETBEANS_INTG
    if (usingNetbeans)
	Columns += 2;		/* leave room for glyph gutter */
#endif
    firstwin->w_height = Rows - p_ch;
    topframe->fr_height = Rows - p_ch;
#ifdef FEAT_VERTSPLIT
    firstwin->w_width = Columns;
    topframe->fr_width = Columns;
#endif
#ifdef FEAT_DIFF
    /* Set the 'diff' option now, so that it can be checked for in a .vimrc
     * file.  There is no buffer yet though. */
    if (diff_mode)
	diff_win_options(firstwin, FALSE);
#endif

    cmdline_row = Rows - p_ch;
    msg_row = cmdline_row;
    screenalloc(FALSE);		/* allocate screen buffers */
    set_init_2();
    TIME_MSG("inits 2");

    msg_scroll = TRUE;
    no_wait_return = TRUE;

    init_mappings();		/* set up initial mappings */

    init_highlight(TRUE, FALSE); /* set the default highlight groups */
    TIME_MSG("init highlight");
#ifdef CURSOR_SHAPE
    parse_shape_opt(SHAPE_CURSOR); /* set cursor shapes from 'guicursor' */
#endif
#ifdef FEAT_MOUSESHAPE
    parse_shape_opt(SHAPE_MOUSE);  /* set mouse shapes from 'mouseshape' */
#endif
#ifdef FEAT_PRINTER
    parse_list_options(p_popt, printer_opts, OPT_PRINT_NUM_OPTIONS);
#endif

#ifdef FEAT_EVAL
    /* Set the break level after the terminal is initialized. */
    debug_break_level = use_debug_break_level;
#endif

#ifdef FEAT_PRECOMMANDS
    if (p_commands > 0)
    {
	curwin->w_cursor.lnum = 0; /* just in case.. */
	sourcing_name = (char_u *)_("pre-vimrc command line");
# ifdef FEAT_EVAL
	current_SID = SID_CMDARG;
# endif
	for (i = 0; i < p_commands; ++i)
	    do_cmdline_cmd(pre_commands[i]);
	sourcing_name = NULL;
# ifdef FEAT_EVAL
	current_SID = 0;
# endif
    }
#endif

    /*
     * For "evim" source evim.vim first of all, so that the user can overrule
     * any things he doesn't like.
     */
    if (evim_mode)
    {
	(void)do_source((char_u *)EVIM_FILE, FALSE, FALSE);
	TIME_MSG("source evim file");
    }

    /*
     * If -u option given, use only the initializations from that file and
     * nothing else.
     */
    if (use_vimrc != NULL)
    {
	if (STRCMP(use_vimrc, "NONE") == 0 || STRCMP(use_vimrc, "NORC") == 0)
	{
#ifdef FEAT_GUI
	    if (use_gvimrc == NULL)	    /* don't load gvimrc either */
		use_gvimrc = use_vimrc;
#endif
	    if (use_vimrc[2] == 'N')
		p_lpl = FALSE;		    /* don't load plugins either */
	}
	else
	{
	    if (do_source(use_vimrc, FALSE, FALSE) != OK)
		EMSG2(_("E282: Cannot read from \"%s\""), use_vimrc);
	}
    }
    else if (!silent_mode)
    {
#ifdef AMIGA
	struct Process	*proc = (struct Process *)FindTask(0L);
	APTR		save_winptr = proc->pr_WindowPtr;

	/* Avoid a requester here for a volume that doesn't exist. */
	proc->pr_WindowPtr = (APTR)-1L;
#endif

	/*
	 * Get system wide defaults, if the file name is defined.
	 */
#ifdef SYS_VIMRC_FILE
	(void)do_source((char_u *)SYS_VIMRC_FILE, FALSE, FALSE);
#endif

	/*
	 * Try to read initialization commands from the following places:
	 * - environment variable VIMINIT
	 * - user vimrc file (s:.vimrc for Amiga, ~/.vimrc otherwise)
	 * - second user vimrc file ($VIM/.vimrc for Dos)
	 * - environment variable EXINIT
	 * - user exrc file (s:.exrc for Amiga, ~/.exrc otherwise)
	 * - second user exrc file ($VIM/.exrc for Dos)
	 * The first that exists is used, the rest is ignored.
	 */
	if (process_env((char_u *)"VIMINIT", TRUE) != OK)
	{
	    if (do_source((char_u *)USR_VIMRC_FILE, TRUE, TRUE) == FAIL
#ifdef USR_VIMRC_FILE2
		&& do_source((char_u *)USR_VIMRC_FILE2, TRUE, TRUE) == FAIL
#endif
#ifdef USR_VIMRC_FILE3
		&& do_source((char_u *)USR_VIMRC_FILE3, TRUE, TRUE) == FAIL
#endif
		&& process_env((char_u *)"EXINIT", FALSE) == FAIL
		&& do_source((char_u *)USR_EXRC_FILE, FALSE, FALSE) == FAIL)
	    {
#ifdef USR_EXRC_FILE2
		(void)do_source((char_u *)USR_EXRC_FILE2, FALSE, FALSE);
#endif
	    }
	}

	/*
	 * Read initialization commands from ".vimrc" or ".exrc" in current
	 * directory.  This is only done if the 'exrc' option is set.
	 * Because of security reasons we disallow shell and write commands
	 * now, except for unix if the file is owned by the user or 'secure'
	 * option has been reset in environmet of global ".exrc" or ".vimrc".
	 * Only do this if VIMRC_FILE is not the same as USR_VIMRC_FILE or
	 * SYS_VIMRC_FILE.
	 */
	if (p_exrc)
	{
#if defined(UNIX) || defined(VMS)
	    /* If ".vimrc" file is not owned by user, set 'secure' mode. */
	    if (!file_owned(VIMRC_FILE))
#endif
		secure = p_secure;

	    i = FAIL;
	    if (fullpathcmp((char_u *)USR_VIMRC_FILE,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#ifdef USR_VIMRC_FILE2
		    && fullpathcmp((char_u *)USR_VIMRC_FILE2,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#endif
#ifdef USR_VIMRC_FILE3
		    && fullpathcmp((char_u *)USR_VIMRC_FILE3,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#endif
#ifdef SYS_VIMRC_FILE
		    && fullpathcmp((char_u *)SYS_VIMRC_FILE,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#endif
				)
		i = do_source((char_u *)VIMRC_FILE, TRUE, TRUE);

	    if (i == FAIL)
	    {
#if defined(UNIX) || defined(VMS)
		/* if ".exrc" is not owned by user set 'secure' mode */
		if (!file_owned(EXRC_FILE))
		    secure = p_secure;
		else
		    secure = 0;
#endif
		if (	   fullpathcmp((char_u *)USR_EXRC_FILE,
				      (char_u *)EXRC_FILE, FALSE) != FPC_SAME
#ifdef USR_EXRC_FILE2
			&& fullpathcmp((char_u *)USR_EXRC_FILE2,
				      (char_u *)EXRC_FILE, FALSE) != FPC_SAME
#endif
				)
		    (void)do_source((char_u *)EXRC_FILE, FALSE, FALSE);
	    }
	}
	if (secure == 2)
	    need_wait_return = TRUE;
	secure = 0;
#ifdef AMIGA
	proc->pr_WindowPtr = save_winptr;
#endif
    }
    TIME_MSG("sourcing vimrc file(s)");

#ifdef FEAT_EVAL
    /*
     * Read all the plugin files.
     * Only when compiled with +eval, since most plugins need it.
     */
    if (p_lpl)
    {
	cmd_runtime((char_u *)"plugin/*.vim", TRUE);
	TIME_MSG("loading plugins");
    }
#endif

    /*
     * Recovery mode without a file name: List swap files.
     * This uses the 'dir' option, therefore it must be after the
     * initializations.
     */
    if (recoverymode && fname == NULL)
    {
	recover_names(NULL, TRUE, 0);
	mch_exit(0);
    }

    /*
     * Set a few option defaults after reading .vimrc files:
     * 'title' and 'icon', Unix: 'shellpipe' and 'shellredir'.
     */
    set_init_3();
    TIME_MSG("inits 3");

    /*
     * "-n" argument: Disable swap file by setting 'updatecount' to 0.
     * Note that this overrides anything from a vimrc file.
     */
    if (no_swap_file)
	p_uc = 0;

#ifdef FEAT_FKMAP
    if (curwin->w_p_rl && p_altkeymap)
    {
	p_hkmap = FALSE;	/* Reset the Hebrew keymap mode */
# ifdef FEAT_ARABIC
	curwin->w_p_arab = FALSE; /* Reset the Arabic keymap mode */
# endif
	p_fkmap = TRUE;		/* Set the Farsi keymap mode */
    }
#endif

#ifdef FEAT_GUI
    if (gui.starting)
    {
#if defined(UNIX) || defined(VMS)
	/* When something caused a message from a vimrc script, need to output
	 * an extra newline before the shell prompt. */
	if (did_emsg || msg_didout)
	    putchar('\n');
#endif

	gui_start();		/* will set full_screen to TRUE */
	TIME_MSG("starting GUI");

	/* When running "evim" or "gvim -y" we need the menus, exit if we
	 * don't have them. */
	if (!gui.in_use && evim_mode)
	    mch_exit(1);
    }
#endif

#ifdef SPAWNO		/* special MSDOS swapping library */
    init_SPAWNO("", SWAP_ANY);
#endif

#ifdef FEAT_VIMINFO
    /*
     * Read in registers, history etc, but not marks, from the viminfo file
     */
    if (*p_viminfo != NUL)
    {
	read_viminfo(NULL, TRUE, FALSE, FALSE);
	TIME_MSG("reading viminfo");
    }
#endif

#ifdef FEAT_QUICKFIX
    /*
     * "-q errorfile": Load the error file now.
     * If the error file can't be read, exit before doing anything else.
     */
    if (edit_type == EDIT_QF)
    {
	if (use_ef != NULL)
	    set_string_option_direct((char_u *)"ef", -1, use_ef, OPT_FREE);
	if (qf_init(p_ef, p_efm, TRUE) < 0)
	{
	    out_char('\n');
	    mch_exit(3);
	}
	TIME_MSG("reading errorfile");
    }
#endif

    /*
     * Start putting things on the screen.
     * Scroll screen down before drawing over it
     * Clear screen now, so file message will not be cleared.
     */
    starting = NO_BUFFERS;
    no_wait_return = FALSE;
    if (!exmode_active)
	msg_scroll = FALSE;

#ifdef FEAT_GUI
    /*
     * This seems to be required to make callbacks to be called now, instead
     * of after things have been put on the screen, which then may be deleted
     * when getting a resize callback.
     * For the Mac this handles putting files dropped on the Vim icon to
     * global_alist.
     */
    if (gui.in_use)
    {
# ifdef FEAT_SUN_WORKSHOP
	if (!usingSunWorkShop)
# endif
	    gui_wait_for_chars(50L);
	TIME_MSG("GUI delay");
    }
#endif

#if defined(FEAT_GUI_PHOTON) && defined(FEAT_CLIPBOARD)
    qnx_clip_init();
#endif

#ifdef FEAT_XCLIPBOARD
    /* Start using the X clipboard, unless the GUI was started. */
# ifdef FEAT_GUI
    if (!gui.in_use)
# endif
    {
	setup_term_clip();
	TIME_MSG("setup clipboard");
    }
#endif

#if defined(FEAT_CLIENTSERVER) && defined(FEAT_X11)
    /*
     * Register for remote command execution with :serversend and --remote
     * unless there was a -X or a --servername '' on the command line.
     * Only register nongui-vim's with an explicit --servername argument.
     */
    if (X_DISPLAY != NULL && servername != NULL && (
# ifdef FEAT_GUI
		gui.in_use ||
# endif
		serverName_arg != NULL))
    {
	(void)serverRegisterName(X_DISPLAY, servername);
	vim_free(servername);
	TIME_MSG("register server name");
    }
    else
	serverDelayedStartName = servername;
#endif

#ifdef FEAT_CLIENTSERVER
    /*
     * Execute command ourselves if we're here because the send failed (or
     * else we would have exited above).
     */
    if (serverStr != NULL)
	server_to_input_buf(serverStr);
#endif

    /*
     * If "-" argument given: Read file from stdin.
     * Do this before starting Raw mode, because it may change things that the
     * writing end of the pipe doesn't like, e.g., in case stdin and stderr
     * are the same terminal: "cat | vim -".
     * Using autocommands here may cause trouble...
     */
    if (edit_type == EDIT_STDIN && !recoverymode)
    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	/* When getting the ATTENTION prompt here, use a dialog */
	swap_exists_action = SEA_DIALOG;
#endif
	no_wait_return = TRUE;
	i = msg_didany;
	set_buflisted(TRUE);
	(void)open_buffer(TRUE, NULL);	/* create memfile and read file */
	no_wait_return = FALSE;
	msg_didany = i;
	TIME_MSG("reading stdin");
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	check_swap_exists_action();
#endif
#if !(defined(AMIGA) || defined(MACOS))
	/*
	 * Close stdin and dup it from stderr.  Required for GPM to work
	 * properly, and for running external commands.
	 * Is there any other system that cannot do this?
	 */
	close(0);
	dup(2);
#endif
    }

#if defined(UNIX) || defined(VMS)
    /* When switching screens and something caused a message from a vimrc
     * script, need to output an extra newline on exit. */
    if ((did_emsg || msg_didout) && *T_TI != NUL)
	newline_on_exit = TRUE;
#endif

    /*
     * When done something that is not allowed or error message call
     * wait_return.  This must be done before starttermcap(), because it may
     * switch to another screen. It must be done after settmode(TMODE_RAW),
     * because we want to react on a single key stroke.
     * Call settmode and starttermcap here, so the T_KS and T_TI may be
     * defined by termcapinit and redifined in .exrc.
     */
    settmode(TMODE_RAW);
    TIME_MSG("setting raw mode");

    if (need_wait_return || msg_didany)
    {
	wait_return(TRUE);
	TIME_MSG("waiting for return");
    }

    starttermcap();	    /* start termcap if not done by wait_return() */
    TIME_MSG("start termcap");

#ifdef FEAT_MOUSE
    setmouse();				/* may start using the mouse */
#endif
    if (scroll_region)
	scroll_region_reset();		/* In case Rows changed */

    scroll_start();

    /*
     * Don't clear the screen when starting in Ex mode, unless using the GUI.
     */
    if (exmode_active
#ifdef FEAT_GUI
			&& !gui.in_use
#endif
					)
	must_redraw = CLEAR;
    else
    {
	screenclear();			/* clear screen */
	TIME_MSG("clearing screen");
    }

#ifdef FEAT_CRYPT
    if (ask_for_key)
    {
	(void)get_crypt_key(TRUE, TRUE);
	TIME_MSG("getting crypt key");
    }
#endif

    no_wait_return = TRUE;

#ifdef FEAT_WINDOWS
    /*
     * Create the number of windows that was requested.
     */
    if (window_count == -1)	/* was not set */
	window_count = 1;
    if (window_count == 0)
	window_count = GARGCOUNT;
    if (window_count > 1)
    {
	/* Don't change the windows if there was a command in .vimrc that
	 * already split some windows */
	if (vert_windows == MAYBE)
	    vert_windows = FALSE;
	if (firstwin->w_next == NULL)
	{
	    window_count = make_windows(window_count, vert_windows);
	    TIME_MSG("making windows");
	}
	else
	    window_count = win_count();
    }
    else
	window_count = 1;
#endif

    if (recoverymode)			/* do recover */
    {
	msg_scroll = TRUE;		/* scroll message up */
	ml_recover();
	if (curbuf->b_ml.ml_mfp == NULL) /* failed */
	    getout(1);
	do_modelines();			/* do modelines */
    }
    else
    {
	/*
	 * Open a buffer for windows that don't have one yet.
	 * Commands in the .vimrc might have loaded a file or split the window.
	 * Watch out for autocommands that delete a window.
	 */
#ifdef FEAT_AUTOCMD
	/*
	 * Don't execute Win/Buf Enter/Leave autocommands here
	 */
	++autocmd_no_enter;
	++autocmd_no_leave;
#endif
#ifdef FEAT_WINDOWS
	for (curwin = firstwin; curwin != NULL; curwin = W_NEXT(curwin))
#endif
	{
	    curbuf = curwin->w_buffer;
	    if (curbuf->b_ml.ml_mfp == NULL)
	    {
#ifdef FEAT_FOLDING
		/* Set 'foldlevel' to 'foldlevelstart' if it's not negative. */
		if (p_fdls >= 0)
		    curwin->w_p_fdl = p_fdls;
#endif
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
		/* When getting the ATTENTION prompt here, use a dialog */
		swap_exists_action = SEA_DIALOG;
#endif
		set_buflisted(TRUE);
		(void)open_buffer(FALSE, NULL); /* create memfile, read file */

#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
		check_swap_exists_action();
#endif
#ifdef FEAT_AUTOCMD
		curwin = firstwin;	    /* start again */
#endif
	    }
#ifdef FEAT_WINDOWS
	    ui_breakcheck();
	    if (got_int)
	    {
		(void)vgetc();	/* only break the file loading, not the rest */
		break;
	    }
#endif
	}
#ifdef FEAT_AUTOCMD
	--autocmd_no_enter;
	--autocmd_no_leave;
#endif
#ifdef FEAT_WINDOWS
	curwin = firstwin;
	curbuf = curwin->w_buffer;
#endif
    }
    TIME_MSG("opening buffers");

    /* Ex starts at last line of the file */
    if (exmode_active)
	curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;

#ifdef FEAT_AUTOCMD
    apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
    TIME_MSG("BufEnter autocommands");
#endif
    setpcmark();

#ifdef FEAT_QUICKFIX
    /*
     * When started with "-q errorfile" jump to first error now.
     */
    if (edit_type == EDIT_QF)
    {
	qf_jump(0, 0, FALSE);
	TIME_MSG("jump to first error");
    }
#endif

#ifdef FEAT_WINDOWS
    /*
     * If opened more than one window, start editing files in the other
     * windows.  Make_windows() has already opened the windows.
     */
# ifdef FEAT_AUTOCMD
    /*
     * Don't execute Win/Buf Enter/Leave autocommands here
     */
    ++autocmd_no_enter;
    ++autocmd_no_leave;
# endif
    arg_idx = 1;
    for (i = 1; i < window_count; ++i)
    {
	if (curwin->w_next == NULL)	    /* just checking */
	    break;
	win_enter(curwin->w_next, FALSE);

	/* Only open the file if there is no file in this window yet (that can
	 * happen when .vimrc contains ":sall") */
	if (curbuf == firstwin->w_buffer || curbuf->b_ffname == NULL)
	{
	    curwin->w_arg_idx = arg_idx;
	    /* edit file from arg list, if there is one */
	    (void)do_ecmd(0, arg_idx < GARGCOUNT
			  ? alist_name(&GARGLIST[arg_idx]) : NULL,
			  NULL, NULL, ECMD_LASTL, ECMD_HIDE);
	    if (arg_idx == GARGCOUNT - 1)
		arg_had_last = TRUE;
	    ++arg_idx;
	}
	ui_breakcheck();
	if (got_int)
	{
	    (void)vgetc();	/* only break the file loading, not the rest */
	    break;
	}
    }
# ifdef FEAT_AUTOCMD
    --autocmd_no_enter;
# endif
    win_enter(firstwin, FALSE);		/* back to first window */
# ifdef FEAT_AUTOCMD
    --autocmd_no_leave;
# endif
    TIME_MSG("editing files in windows");
    if (window_count > 1)
	win_equal(curwin, FALSE, 'b');	/* adjust heights */
#endif /* FEAT_WINDOWS */

#ifdef FEAT_DIFF
    if (diff_mode)
    {
	win_T	*wp;

	/* set options in each window for "vimdiff". */
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	    diff_win_options(wp, TRUE);
    }
#endif

    /*
     * Shorten any of the filenames, but only when absolute.
     */
    shorten_fnames(FALSE);

    /*
     * Need to jump to the tag before executing the '-c command'.
     * Makes "vim -c '/return' -t main" work.
     */
    if (tagname != NULL)
    {
	STRCPY(IObuff, "ta ");

	STRNCAT(IObuff, tagname, IOSIZE - 4);
	IObuff[IOSIZE - 1] = NUL;
	do_cmdline_cmd(IObuff);
	TIME_MSG("jumping to tag");
    }

    if (n_commands > 0)
    {
	/*
	 * We start commands on line 0, make "vim +/pat file" match a
	 * pattern on line 1.
	 */
	msg_scroll = TRUE;
	if (tagname == NULL)
	    curwin->w_cursor.lnum = 0;
	sourcing_name = (char_u *)"command line";
#ifdef FEAT_EVAL
	current_SID = SID_CARG;
#endif
	for (i = 0; i < n_commands; ++i)
	    do_cmdline_cmd(commands[i]);
	sourcing_name = NULL;
#ifdef FEAT_EVAL
	current_SID = 0;
#endif
	if (curwin->w_cursor.lnum == 0)
	    curwin->w_cursor.lnum = 1;

	if (!exmode_active)
	    msg_scroll = FALSE;

#ifdef FEAT_QUICKFIX
	/* When started with "-q errorfile" jump to first error again. */
	if (edit_type == EDIT_QF)
	    qf_jump(0, 0, FALSE);
#endif
	TIME_MSG("executing command arguments");
    }

    RedrawingDisabled = 0;
    redraw_all_later(NOT_VALID);
    no_wait_return = FALSE;
    starting = 0;

    /* start in insert mode */
    if (p_im)
	need_start_insertmode = TRUE;

#ifdef FEAT_AUTOCMD
    apply_autocmds(EVENT_VIMENTER, NULL, NULL, FALSE, curbuf);
    TIME_MSG("VimEnter autocommands");
#endif

#if defined(FEAT_DIFF) && defined(FEAT_SCROLLBIND)
    /* When a startup script or session file setup for diff'ing and
     * scrollbind, sync the scrollbind now. */
    if (curwin->w_p_diff && curwin->w_p_scb)
    {
	update_topline();
	check_scrollbind((linenr_T)0, 0L);
	TIME_MSG("diff scrollbinding");
    }
#endif

#if defined(WIN3264) && !defined(FEAT_GUI_W32)
    mch_set_winsize_now();	    /* Allow winsize changes from now on */
#endif

    /* If ":startinsert" command used, stuff a dummy command to be able to
     * call normal_cmd(), which will then start Insert mode. */
    if (restart_edit != 0)
	stuffcharReadbuff(K_IGNORE);

#ifdef FEAT_NETBEANS_INTG
    if (usingNetbeans)
	/* Tell the client that it can start sending commands. */
	netbeans_startup_done();
#endif

    TIME_MSG("before starting main loop");

    /*
     * Call the main command loop.  This never returns.
     */
    main_loop(FALSE);

    return 0;
}
#endif /* PROTO */

/*
 * Main loop: Execute Normal mode commands until exiting Vim.
 * Also used to handle commands in the command-line window, until the window
 * is closed.
 */
    void
main_loop(cmdwin)
    int		cmdwin;	/* TRUE when working in the command-line window */
{
    oparg_T	oa;	/* operator arguments */

#if defined(FEAT_X11) && defined(FEAT_XCLIPBOARD)
    /* Setup to catch a terminating error from the X server.  Just ignore
     * it, restore the state and continue.  This might not always work
     * properly, but at least we don't exit unexpectedly when the X server
     * exists while Vim is running in a console. */
    if (!cmdwin && SETJMP(x_jump_env))
    {
	State = NORMAL;
# ifdef FEAT_VISUAL
	VIsual_active = FALSE;
# endif
	got_int = TRUE;
	need_wait_return = FALSE;
	global_busy = FALSE;
	exmode_active = 0;
	skip_redraw = FALSE;
	RedrawingDisabled = 0;
	no_wait_return = 0;
# ifdef FEAT_EVAL
	emsg_skip = 0;
# endif
	emsg_off = 0;
# ifdef FEAT_MOUSE
	setmouse();
# endif
	settmode(TMODE_RAW);
	starttermcap();
	scroll_start();
	redraw_later_clear();
    }
#endif

    clear_oparg(&oa);
    while (!cmdwin
#ifdef FEAT_CMDWIN
	    || cmdwin_result == 0
#endif
	    )
    {
	if (stuff_empty())
	{
	    did_check_timestamps = FALSE;
	    if (need_check_timestamps)
		check_timestamps(FALSE);
	    if (need_wait_return)	/* if wait_return still needed ... */
		wait_return(FALSE);	/* ... call it now */
	    if (need_start_insertmode && goto_im()
#ifdef FEAT_VISUAL
		    && !VIsual_active
#endif
		    )
	    {
		need_start_insertmode = FALSE;
		stuffReadbuff((char_u *)"i");	/* start insert mode next */
		/* skip the fileinfo message now, because it would be shown
		 * after insert mode finishes! */
		need_fileinfo = FALSE;
	    }
	}
	if (got_int && !global_busy)
	{
	    if (!quit_more)
		(void)vgetc();		/* flush all buffers */
	    got_int = FALSE;
	}
	if (!exmode_active)
	    msg_scroll = FALSE;
	quit_more = FALSE;

	/*
	 * If skip redraw is set (for ":" in wait_return()), don't redraw now.
	 * If there is nothing in the stuff_buffer or do_redraw is TRUE,
	 * update cursor and redraw.
	 */
	if (skip_redraw || exmode_active)
	    skip_redraw = FALSE;
	else if (do_redraw || stuff_empty())
	{
#if defined(FEAT_FOLDING) && defined(FEAT_VISUAL)
	    /* Include a closed fold completely in the Visual area. */
	    foldAdjustVisual();
#endif
#ifdef FEAT_FOLDING
	    /*
	     * When 'foldclose' is set, apply 'foldlevel' to folds that don't
	     * contain the cursor.
	     * When 'foldopen' is "all", open the fold(s) under the cursor.
	     * This may mark the window for redrawing.
	     */
	    if (hasAnyFolding(curwin) && !char_avail())
	    {
		foldCheckClose();
		if (fdo_flags & FDO_ALL)
		    foldOpenCursor();
	    }
#endif

	    /*
	     * Before redrawing, make sure w_topline is correct, and w_leftcol
	     * if lines don't wrap, and w_skipcol if lines wrap.
	     */
	    update_topline();
	    validate_cursor();

#ifdef FEAT_VISUAL
	    if (VIsual_active)
		update_curbuf(INVERTED);/* update inverted part */
	    else
#endif
		if (must_redraw)
		update_screen(0);
	    else if (redraw_cmdline || clear_cmdline)
		showmode();
#ifdef FEAT_WINDOWS
	    redraw_statuslines();
#endif
#ifdef FEAT_TITLE
	    if (need_maketitle)
		maketitle();
#endif
	    /* display message after redraw */
	    if (keep_msg != NULL)
	    {
		char_u *p;

		/* msg_attr_keep() will set keep_msg to NULL, must free the
		 * string here. */
		p = keep_msg;
		msg_attr(p, keep_msg_attr);
		vim_free(p);
	    }
	    if (need_fileinfo)		/* show file info after redraw */
	    {
		fileinfo(FALSE, TRUE, FALSE);
		need_fileinfo = FALSE;
	    }

	    emsg_on_display = FALSE;	/* can delete error message now */
	    did_emsg = FALSE;
	    msg_didany = FALSE;		/* reset lines_left in msg_start() */
	    showruler(FALSE);

	    setcursor();
	    cursor_on();

	    do_redraw = FALSE;
	}
#ifdef FEAT_GUI
	if (need_mouse_correct)
	    gui_mouse_correct();
#endif

	/*
	 * Update w_curswant if w_set_curswant has been set.
	 * Postponed until here to avoid computing w_virtcol too often.
	 */
	update_curswant();

	/*
	 * If we're invoked as ex, do a round of ex commands.
	 * Otherwise, get and execute a normal mode command.
	 */
	if (exmode_active)
	    do_exmode(exmode_active == EXMODE_VIM);
	else
	    normal_cmd(&oa, TRUE);
    }
}


#if defined(USE_XSMP) || defined(FEAT_GUI_MSWIN) || defined(PROTO)
/*
 * Exit, but leave behind swap files for modified buffers.
 */
    void
getout_preserve_modified(exitval)
    int		exitval;
{
    ml_close_notmod();		    /* close all not-modified buffers */
    ml_sync_all(FALSE, FALSE);	    /* preserve all swap files */
    ml_close_all(FALSE);	    /* close all memfiles, without deleting */
    getout(exitval);		    /* exit Vim properly */
}
#endif


/* Exit properly */
    void
getout(exitval)
    int		exitval;
{
#ifdef FEAT_AUTOCMD
    buf_T	*buf;
    win_T	*wp;
#endif

    exiting = TRUE;

    /* Position the cursor on the last screen line, below all the text */
#ifdef FEAT_GUI
    if (!gui.in_use)
#endif
	windgoto((int)Rows - 1, 0);

#ifdef FEAT_GUI
    msg_didany = FALSE;
#endif

#ifdef FEAT_AUTOCMD
    /* Trigger BufWinLeave for all windows, but only once per buffer. */
    for (wp = firstwin; wp != NULL; )
    {
	buf = wp->w_buffer;
	if (buf->b_changedtick != -1)
	{
	    apply_autocmds(EVENT_BUFWINLEAVE, buf->b_fname, buf->b_fname,
								  FALSE, buf);
	    buf->b_changedtick = -1;	/* note that we did it already */
	    wp = firstwin;		/* restart, window may be closed */
	}
	else
	    wp = wp->w_next;
    }
    /* Trigger BufUnload for buffers that are loaded */
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (buf->b_ml.ml_mfp != NULL)
	{
	    apply_autocmds(EVENT_BUFUNLOAD, buf->b_fname, buf->b_fname,
								  FALSE, buf);
	    if (!buf_valid(buf))	/* autocmd may delete the buffer */
		break;
	}
    apply_autocmds(EVENT_VIMLEAVEPRE, NULL, NULL, FALSE, curbuf);
#endif

#ifdef FEAT_VIMINFO
    if (*p_viminfo != NUL)
	/* Write out the registers, history, marks etc, to the viminfo file */
	write_viminfo(NULL, FALSE);
#endif

#ifdef FEAT_AUTOCMD
    apply_autocmds(EVENT_VIMLEAVE, NULL, NULL, FALSE, curbuf);
#endif

    if (did_emsg
#ifdef FEAT_GUI
	    || (gui.in_use && msg_didany && p_verbose > 0)
#endif
	    )
    {
	/* give the user a chance to read the (error) message */
	no_wait_return = FALSE;
	wait_return(FALSE);
    }

#ifdef FEAT_AUTOCMD
    /* Position the cursor again, the autocommands may have moved it */
# ifdef FEAT_GUI
    if (!gui.in_use)
# endif
	windgoto((int)Rows - 1, 0);
#endif

#ifdef FEAT_TCL
    tcl_end();
#endif
#ifdef FEAT_RUBY
    ruby_end();
#endif
#ifdef FEAT_PYTHON
    python_end();
#endif
#ifdef FEAT_PERL
    perl_end();
#endif
#if defined(USE_ICONV) && defined(DYNAMIC_ICONV)
    iconv_end();
#endif
#ifdef FEAT_NETBEANS_INTG
    netbeans_end();
#endif

    mch_exit(exitval);
}

/*
 * Get a (optional) count for a Vim argument.
 */
    static int
get_number_arg(p, idx, def)
    char_u	*p;	    /* pointer to argument */
    int		*idx;	    /* index in argument, is incremented */
    int		def;	    /* default value */
{
    if (vim_isdigit(p[*idx]))
    {
	def = atoi((char *)&(p[*idx]));
	while (vim_isdigit(p[*idx]))
	    *idx = *idx + 1;
    }
    return def;
}

/*
 * Setup to start using the GUI.  Exit with an error when not available.
 */
    static void
main_start_gui()
{
#ifdef FEAT_GUI
    gui.starting = TRUE;	/* start GUI a bit later */
#else
    mch_errmsg(_(e_nogvim));
    mch_errmsg("\n");
    mch_exit(2);
#endif
}

/*
 * Get an evironment variable, and execute it as Ex commands.
 * Returns FAIL if the environment variable was not executed, OK otherwise.
 */
    int
process_env(env, is_viminit)
    char_u	*env;
    int		is_viminit; /* when TRUE, called for VIMINIT */
{
    char_u	*initstr;
    char_u	*save_sourcing_name;
    linenr_T	save_sourcing_lnum;
#ifdef FEAT_EVAL
    scid_T	save_sid;
#endif

    if ((initstr = mch_getenv(env)) != NULL && *initstr != NUL)
    {
	if (is_viminit)
	    vimrc_found();
	save_sourcing_name = sourcing_name;
	save_sourcing_lnum = sourcing_lnum;
	sourcing_name = env;
	sourcing_lnum = 0;
#ifdef FEAT_EVAL
	save_sid = current_SID;
	current_SID = SID_ENV;
#endif
	do_cmdline_cmd(initstr);
	sourcing_name = save_sourcing_name;
	sourcing_lnum = save_sourcing_lnum;
#ifdef FEAT_EVAL
	current_SID = save_sid;;
#endif
	return OK;
    }
    return FAIL;
}

#if defined(UNIX) || defined(VMS)
/*
 * Return TRUE if we are certain the user owns the file "fname".
 * Used for ".vimrc" and ".exrc".
 * Use both stat() and lstat() for extra security.
 */
    static int
file_owned(fname)
    char	*fname;
{
    struct stat s;
# ifdef UNIX
    uid_t	uid = getuid();
# else	 /* VMS */
    uid_t	uid = ((getgid() << 16) | getuid());
# endif

    return !(mch_stat(fname, &s) != 0 || s.st_uid != uid
# ifdef HAVE_LSTAT
	    || mch_lstat(fname, &s) != 0 || s.st_uid != uid
# endif
	    );
}
#endif

/*
 * Give an error message main_errors["n"] and exit.
 */
    static void
mainerr(n, str)
    int		n;	/* one of the ME_ defines */
    char_u	*str;	/* extra argument or NULL */
{
#if defined(UNIX) || defined(__EMX__) || defined(VMS)
    reset_signals();		/* kill us with CTRL-C here, if you like */
#endif

    mch_errmsg(longVersion);
    mch_errmsg("\n");
    mch_errmsg(_(main_errors[n]));
    if (str != NULL)
    {
	mch_errmsg(": \"");
	mch_errmsg((char *)str);
	mch_errmsg("\"");
    }
    mch_errmsg(_("\nMore info with: \"vim -h\"\n"));

    mch_exit(1);
}

    void
mainerr_arg_missing(str)
    char_u	*str;
{
    mainerr(ME_ARG_MISSING, str);
}

/*
 * print a message with three spaces prepended and '\n' appended.
 */
    static void
main_msg(s)
    char *s;
{
    mch_msg("   ");
    mch_msg(s);
    mch_msg("\n");
}

/*
 * Print messages for "vim -h" or "vim --help" and exit.
 */
    static void
usage()
{
    int		i;
    static char	*(use[]) =
    {
	N_("[file ..]       edit specified file(s)"),
	N_("-               read text from stdin"),
	N_("-t tag          edit file where tag is defined"),
#ifdef FEAT_QUICKFIX
	N_("-q [errorfile]  edit file with first error")
#endif
    };

#if defined(UNIX) || defined(__EMX__) || defined(VMS)
    reset_signals();		/* kill us with CTRL-C here, if you like */
#endif

    mch_msg(longVersion);
    mch_msg(_("\n\nusage:"));
    for (i = 0; ; ++i)
    {
	mch_msg(_(" vim [arguments] "));
	mch_msg(_(use[i]));
	if (i == (sizeof(use) / sizeof(char_u *)) - 1)
	    break;
	mch_msg(_("\n   or:"));
    }

    mch_msg(_("\n\nArguments:\n"));
    main_msg(_("--\t\t\tOnly file names after this"));
#if (!defined(UNIX) && !defined(__EMX__)) || defined(ARCHIE)
    main_msg(_("--literal\t\tDon't expand wildcards"));
#endif
#ifdef FEAT_OLE
    main_msg(_("-register\t\tRegister this gvim for OLE"));
    main_msg(_("-unregister\t\tUnregister gvim for OLE"));
#endif
#ifdef FEAT_GUI
    main_msg(_("-g\t\t\tRun using GUI (like \"gvim\")"));
    main_msg(_("-f  or  --nofork\tForeground: Don't fork when starting GUI"));
#endif
    main_msg(_("-v\t\t\tVi mode (like \"vi\")"));
    main_msg(_("-e\t\t\tEx mode (like \"ex\")"));
    main_msg(_("-s\t\t\tSilent (batch) mode (only for \"ex\")"));
#ifdef FEAT_DIFF
    main_msg(_("-d\t\t\tDiff mode (like \"vimdiff\")"));
#endif
    main_msg(_("-y\t\t\tEasy mode (like \"evim\", modeless)"));
    main_msg(_("-R\t\t\tReadonly mode (like \"view\")"));
    main_msg(_("-Z\t\t\tRestricted mode (like \"rvim\")"));
    main_msg(_("-m\t\t\tModifications (writing files) not allowed"));
    main_msg(_("-M\t\t\tModifications in text not allowed"));
    main_msg(_("-b\t\t\tBinary mode"));
#ifdef FEAT_LISP
    main_msg(_("-l\t\t\tLisp mode"));
#endif
    main_msg(_("-C\t\t\tCompatible with Vi: 'compatible'"));
    main_msg(_("-N\t\t\tNot fully Vi compatible: 'nocompatible'"));
    main_msg(_("-V[N]\t\tVerbose level"));
    main_msg(_("-D\t\t\tDebugging mode"));
    main_msg(_("-n\t\t\tNo swap file, use memory only"));
    main_msg(_("-r\t\t\tList swap files and exit"));
    main_msg(_("-r (with file name)\tRecover crashed session"));
    main_msg(_("-L\t\t\tSame as -r"));
#ifdef AMIGA
    main_msg(_("-f\t\t\tDon't use newcli to open window"));
    main_msg(_("-dev <device>\t\tUse <device> for I/O"));
#endif
#ifdef FEAT_ARABIC
    main_msg(_("-A\t\t\tstart in Arabic mode"));
#endif
#ifdef FEAT_RIGHTLEFT
    main_msg(_("-H\t\t\tStart in Hebrew mode"));
#endif
#ifdef FEAT_FKMAP
    main_msg(_("-F\t\t\tStart in Farsi mode"));
#endif
    main_msg(_("-T <terminal>\tSet terminal type to <terminal>"));
    main_msg(_("-u <vimrc>\t\tUse <vimrc> instead of any .vimrc"));
#ifdef FEAT_GUI
    main_msg(_("-U <gvimrc>\t\tUse <gvimrc> instead of any .gvimrc"));
#endif
    main_msg(_("--noplugin\t\tDon't load plugin scripts"));
    main_msg(_("-o[N]\t\tOpen N windows (default: one for each file)"));
    main_msg(_("-O[N]\t\tLike -o but split vertically"));
    main_msg(_("+\t\t\tStart at end of file"));
    main_msg(_("+<lnum>\t\tStart at line <lnum>"));
#ifdef FEAT_PRECOMMANDS
    main_msg(_("--cmd <command>\tExecute <command> before loading any vimrc file"));
#endif
    main_msg(_("-c <command>\t\tExecute <command> after loading the first file"));
    main_msg(_("-S <session>\t\tSource file <session> after loading the first file"));
    main_msg(_("-s <scriptin>\tRead Normal mode commands from file <scriptin>"));
    main_msg(_("-w <scriptout>\tAppend all typed commands to file <scriptout>"));
    main_msg(_("-W <scriptout>\tWrite all typed commands to file <scriptout>"));
#ifdef FEAT_CRYPT
    main_msg(_("-x\t\t\tEdit encrypted files"));
#endif
#if (defined(UNIX) || defined(VMS)) && defined(FEAT_X11)
# if defined(FEAT_GUI_X11) && !defined(FEAT_GUI_GTK)
    main_msg(_("-display <display>\tConnect vim to this particular X-server"));
# endif
    main_msg(_("-X\t\t\tDo not connect to X server"));
#endif
#ifdef FEAT_CLIENTSERVER
    main_msg(_("--remote <files>\tEdit <files> in a Vim server if possible"));
    main_msg(_("--remote-silent <files>  Same, don't complain if there is no server"));
    main_msg(_("--remote-wait <files>  As --remote but wait for files to have been edited"));
    main_msg(_("--remote-wait-silent <files>  Same, don't complain if there is no server"));
    main_msg(_("--remote-send <keys>\tSend <keys> to a Vim server and exit"));
    main_msg(_("--remote-expr <expr>\tEvaluate <expr> in a Vim server and print result"));
    main_msg(_("--serverlist\t\tList available Vim server names and exit"));
    main_msg(_("--servername <name>\tSend to/become the Vim server <name>"));
#endif
#ifdef FEAT_VIMINFO
    main_msg(_("-i <viminfo>\t\tUse <viminfo> instead of .viminfo"));
#endif
    main_msg(_("-h  or  --help\tPrint Help (this message) and exit"));
    main_msg(_("--version\t\tPrint version information and exit"));

#ifdef FEAT_GUI_X11
# ifdef FEAT_GUI_MOTIF
    mch_msg(_("\nArguments recognised by gvim (Motif version):\n"));
# else
#  ifdef FEAT_GUI_ATHENA
#   ifdef FEAT_GUI_NEXTAW
    mch_msg(_("\nArguments recognised by gvim (neXtaw version):\n"));
#   else
    mch_msg(_("\nArguments recognised by gvim (Athena version):\n"));
#   endif
#  endif
# endif
    main_msg(_("-display <display>\tRun vim on <display>"));
    main_msg(_("-iconic\t\tStart vim iconified"));
# if 0
    main_msg(_("-name <name>\t\tUse resource as if vim was <name>"));
    mch_msg(_("\t\t\t  (Unimplemented)\n"));
# endif
    main_msg(_("-background <color>\tUse <color> for the background (also: -bg)"));
    main_msg(_("-foreground <color>\tUse <color> for normal text (also: -fg)"));
    main_msg(_("-font <font>\t\tUse <font> for normal text (also: -fn)"));
    main_msg(_("-boldfont <font>\tUse <font> for bold text"));
    main_msg(_("-italicfont <font>\tUse <font> for italic text"));
    main_msg(_("-geometry <geom>\tUse <geom> for initial geometry (also: -geom)"));
    main_msg(_("-borderwidth <width>\tUse a border width of <width> (also: -bw)"));
    main_msg(_("-scrollbarwidth <width>  Use a scrollbar width of <width> (also: -sw)"));
# ifdef FEAT_GUI_ATHENA
    main_msg(_("-menuheight <height>\tUse a menu bar height of <height> (also: -mh)"));
# endif
    main_msg(_("-reverse\t\tUse reverse video (also: -rv)"));
    main_msg(_("+reverse\t\tDon't use reverse video (also: +rv)"));
    main_msg(_("-xrm <resource>\tSet the specified resource"));
#endif /* FEAT_GUI_X11 */
#if defined(FEAT_GUI) && defined(RISCOS)
    mch_msg(_("\nArguments recognised by gvim (RISC OS version):\n"));
    main_msg(_("--columns <number>\tInitial width of window in columns"));
    main_msg(_("--rows <number>\tInitial height of window in rows"));
#endif
#ifdef FEAT_GUI_GTK
    mch_msg(_("\nArguments recognised by gvim (GTK+ version):\n"));
    main_msg(_("-font <font>\t\tUse <font> for normal text (also: -fn)"));
    main_msg(_("-geometry <geom>\tUse <geom> for initial geometry (also: -geom)"));
    main_msg(_("-reverse\t\tUse reverse video (also: -rv)"));
    main_msg(_("-display <display>\tRun vim on <display> (also: --display)"));
# ifdef HAVE_GTK2
    main_msg(_("--role <role>\tSet a unique role to identify the main window"));
# endif
    main_msg(_("--socketid <xid>\tOpen Vim inside another GTK widget"));
#endif
#ifdef FEAT_GUI_W32
    main_msg(_("-P <parent title>\tOpen Vim inside parent application"));
#endif

#ifdef FEAT_GUI_GNOME
    /* Gnome gives extra messages for --help if we continue, but not for -h. */
    if (gui.starting)
	mch_msg("\n");
    else
#endif
	mch_exit(0);
}

#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
/*
 * Check the result of the ATTENTION dialog:
 * When "Quit" selected, exit Vim.
 * When "Recover" selected, recover the file.
 */
    static void
check_swap_exists_action()
{
    if (swap_exists_action == SEA_QUIT)
	getout(1);
    handle_swap_exists(NULL);
}
#endif

#if defined(STARTUPTIME) || defined(PROTO)
static void time_diff __ARGS((struct timeval *then, struct timeval *now));

static struct timeval	prev_timeval;

/*
 * Save the previous time before doing something that could nest.
 * set "*tv_rel" to the time elapsed so far.
 */
    void
time_push(tv_rel, tv_start)
    void	*tv_rel, *tv_start;
{
    *((struct timeval *)tv_rel) = prev_timeval;
    gettimeofday(&prev_timeval, NULL);
    ((struct timeval *)tv_rel)->tv_usec = prev_timeval.tv_usec
					- ((struct timeval *)tv_rel)->tv_usec;
    ((struct timeval *)tv_rel)->tv_sec = prev_timeval.tv_sec
					 - ((struct timeval *)tv_rel)->tv_sec;
    if (((struct timeval *)tv_rel)->tv_usec < 0)
    {
	((struct timeval *)tv_rel)->tv_usec += 1000000;
	--((struct timeval *)tv_rel)->tv_sec;
    }
    *(struct timeval *)tv_start = prev_timeval;
}

/*
 * Compute the previous time after doing something that could nest.
 * Subtract "*tp" from prev_timeval;
 * Note: The arguments are (void *) to avoid trouble with systems that don't
 * have struct timeval.
 */
    void
time_pop(tp)
    void	*tp;	/* actually (struct timeval *) */
{
    prev_timeval.tv_usec -= ((struct timeval *)tp)->tv_usec;
    prev_timeval.tv_sec -= ((struct timeval *)tp)->tv_sec;
    if (prev_timeval.tv_usec < 0)
    {
	prev_timeval.tv_usec += 1000000;
	--prev_timeval.tv_sec;
    }
}

    static void
time_diff(then, now)
    struct timeval	*then;
    struct timeval	*now;
{
    long	usec;
    long	msec;

    usec = now->tv_usec - then->tv_usec;
    msec = (now->tv_sec - then->tv_sec) * 1000L + usec / 1000L,
    usec = usec % 1000L;
    fprintf(time_fd, "%03ld.%03ld", msec, usec >= 0 ? usec : usec + 1000L);
}

    void
time_msg(msg, tv_start)
    char	*msg;
    void	*tv_start;  /* only for do_source: start time; actually
			       (struct timeval *) */
{
    static struct timeval	start;
    struct timeval		now;

    if (time_fd != NULL)
    {
	if (strstr(msg, "STARTING") != NULL)
	{
	    gettimeofday(&start, NULL);
	    prev_timeval = start;
	    fprintf(time_fd, "\n\ntimes in msec\n");
	    fprintf(time_fd, " clock   self+sourced   self:  sourced script\n");
	    fprintf(time_fd, " clock   elapsed:              other lines\n\n");
	}
	gettimeofday(&now, NULL);
	time_diff(&start, &now);
	if (((struct timeval *)tv_start) != NULL)
	{
	    fprintf(time_fd, "  ");
	    time_diff(((struct timeval *)tv_start), &now);
	}
	fprintf(time_fd, "  ");
	time_diff(&prev_timeval, &now);
	prev_timeval = now;
	fprintf(time_fd, ": %s\n", msg);
    }
}

# ifdef WIN3264
/*
 * Windows doesn't have gettimeofday(), although it does have struct timeval.
 */
    int
gettimeofday(struct timeval *tv, char *dummy)
{
    long t = clock();
    tv->tv_sec = t / CLOCKS_PER_SEC;
    tv->tv_usec = (t - tv->tv_sec * CLOCKS_PER_SEC) * 1000000 / CLOCKS_PER_SEC;
    return 0;
}
# endif

#endif

#if defined(FEAT_CLIENTSERVER) || defined(PROTO)

/*
 * Common code for the X command server and the Win32 command server.
 */

static char_u *build_drop_cmd __ARGS((int filec, char **filev, int sendReply));

    static void
cmdsrv_main(argc, argv, serverName_arg, serverStr)
    int		*argc;
    char	**argv;
    char_u	*serverName_arg;
    char_u	**serverStr;
{
    char_u	*res;
    int		i;
    char_u	*sname;
    int		ret;
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
# ifndef FEAT_X11
    HWND	srv;
# else
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
	if (STRCMP(argv[i], "--") == 0)	/* end of options */
	{
	    for (; i < *argc; i++)
	    {
		*newArgV++ = argv[i];
		newArgC++;
	    }
	    break;
	}

	if (STRICMP(argv[i], "--remote") == 0)
	    argtype = ARGTYPE_EDIT;
	else if (STRICMP(argv[i], "--remote-silent") == 0)
	{
	    argtype = ARGTYPE_EDIT;
	    silent = TRUE;
	}
	else if (STRICMP(argv[i], "--remote-wait") == 0)
	    argtype = ARGTYPE_EDIT_WAIT;
	else if (STRICMP(argv[i], "--remote-wait-silent") == 0)
	{
	    argtype = ARGTYPE_EDIT_WAIT;
	    silent = TRUE;
	}
	else if (STRICMP(argv[i], "--remote-send") == 0)
	    argtype = ARGTYPE_SEND;
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
						argtype == ARGTYPE_EDIT_WAIT);
		if (*serverStr == NULL)
		{
		    /* Probably out of memory, exit. */
		    didone = TRUE;
		    exiterr = 1;
		    break;
		}
		Argc = i;
	    }
# ifdef FEAT_X11
	    if (xterm_dpy == NULL)
	    {
		mch_errmsg(_("No display"));
		ret = -1;
	    }
	    else
		ret = serverSendToVim(xterm_dpy, sname, *serverStr,
						    NULL, &srv, 0, 0, silent);
# else
	    /* Win32 always works? */
	    ret = serverSendToVim(sname, *serverStr, NULL, &srv, 0, silent);
# endif
	    if (ret < 0)
	    {
		if (argtype == ARGTYPE_SEND)
		{
		    /* Failed to send, abort. */
		    mch_errmsg(_(": Send failed.\n"));
		    didone = TRUE;
		    exiterr = 1;
		}
		else if (!silent)
		    /* Let vim start normally.  */
		    mch_errmsg(_(": Send failed. Trying to execute locally\n"));
		break;
	    }

# ifdef FEAT_GUI_W32
	    /* Guess that when the server name starts with "g" it's a GUI
	     * server, which we can bring to the foreground here.
	     * Foreground() in the server doesn't work very well. */
	    if (argtype != ARGTYPE_SEND && TOUPPER_ASC(*sname) == 'G')
		SetForegroundWindow(srv);
# endif

	    /*
	     * For --remote-wait: Wait until the server did edit each
	     * file.  Also detect that the server no longer runs.
	     */
	    if (ret >= 0 && argtype == ARGTYPE_EDIT_WAIT)
	    {
		int	numFiles = *argc - i - 1;
		int	j;
		char_u  *done = alloc(numFiles);
		char_u  *p;
# ifdef FEAT_GUI_W32
		NOTIFYICONDATA ni;
		int	count = 0;
		extern HWND message_window;
# endif

		if (numFiles > 0 && argv[i + 1][0] == '+')
		    /* Skip "+cmd" argument, don't wait for it to be edited. */
		    --numFiles;

# ifdef FEAT_GUI_W32
		ni.cbSize = sizeof(ni);
		ni.hWnd = message_window;
		ni.uID = 0;
		ni.uFlags = NIF_ICON|NIF_TIP;
		ni.hIcon = LoadIcon((HINSTANCE)GetModuleHandle(0), "IDR_VIM");
		sprintf(ni.szTip, _("%d of %d edited"), count, numFiles);
		Shell_NotifyIcon(NIM_ADD, &ni);
# endif

		/* Wait for all files to unload in remote */
		memset(done, 0, numFiles);
		while (memchr(done, 0, numFiles) != NULL)
		{
# ifdef WIN32
		    p = serverGetReply(srv, NULL, TRUE, TRUE);
		    if (p == NULL)
			break;
# else
		    if (serverReadReply(xterm_dpy, srv, &p, TRUE) < 0)
			break;
# endif
		    j = atoi((char *)p);
		    if (j >= 0 && j < numFiles)
		    {
# ifdef FEAT_GUI_W32
			++count;
			sprintf(ni.szTip, _("%d of %d edited"),
							     count, numFiles);
			Shell_NotifyIcon(NIM_MODIFY, &ni);
# endif
			done[j] = 1;
		    }
		}
# ifdef FEAT_GUI_W32
		Shell_NotifyIcon(NIM_DELETE, &ni);
# endif
	    }
	}
	else if (STRICMP(argv[i], "--remote-expr") == 0)
	{
	    if (i == *argc - 1)
		mainerr_arg_missing((char_u *)argv[i]);
# ifdef WIN32
	    /* Win32 always works? */
	    if (serverSendToVim(sname, (char_u *)argv[i + 1],
						    &res, NULL, 1, FALSE) < 0)
# else
	    if (xterm_dpy == NULL)
		mch_errmsg(_("No display: Send expression failed.\n"));
	    else if (serverSendToVim(xterm_dpy, sname, (char_u *)argv[i + 1],
						 &res, NULL, 1, 1, FALSE) < 0)
# endif
	    {
		if (res != NULL && *res != NUL)
		{
		    /* Output error from remote */
		    mch_errmsg((char *)res);
		    vim_free(res);
		    res = NULL;
		}
		mch_errmsg(_(": Send expression failed.\n"));
	    }
	}
	else if (STRICMP(argv[i], "--serverlist") == 0)
	{
# ifdef WIN32
	    /* Win32 always works? */
	    res = serverGetVimNames();
# else
	    if (xterm_dpy != NULL)
		res = serverGetVimNames(xterm_dpy);
# endif
	    if (called_emsg)
		mch_errmsg("\n");
	}
	else if (STRICMP(argv[i], "--servername") == 0)
	{
	    /* Alredy processed. Take it out of the command line */
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
	display_errors();	/* display any collected messages */
	exit(exiterr);	/* Mission accomplished - get out */
    }

    /* Return back into main() */
    *argc = newArgC;
    vim_free(sname);
}

/*
 * Build a ":drop" command to send to a Vim server.
 */
    static char_u *
build_drop_cmd(filec, filev, sendReply)
    int		filec;
    char	**filev;
    int		sendReply;
{
    garray_T	ga;
    int		i;
    char_u	*inicmd = NULL;
    char_u	*p;
    char_u	cwd[MAXPATHL];

    if (filec > 0 && filev[0][0] == '+')
    {
	inicmd = (char_u *)filev[0] + 1;
	filev++;
	filec--;
    }
    /* Check if we have at least one argument. */
    if (filec <= 0)
	mainerr_arg_missing((char_u *)filev[-1]);
    if (mch_dirname(cwd, MAXPATHL) != OK)
	return NULL;
    if ((p = vim_strsave_escaped_ext(cwd, PATH_ESC_CHARS, TRUE)) == NULL)
	return NULL;
    ga_init2(&ga, 1, 100);
    ga_concat(&ga, (char_u *)"<C-\\><C-N>:cd ");
    ga_concat(&ga, p);
    /* Call inputsave() so that a prompt for an encryption key works. */
    ga_concat(&ga, (char_u *)"<CR>:if exists('*inputsave')|call inputsave()|endif|drop");
    vim_free(p);
    for (i = 0; i < filec; i++)
    {
	/* On Unix the shell has already expanded the wildcards, don't want to
	 * do it again in the Vim server.  On MS-Windows only need to escape a
	 * space. */
	p = vim_strsave_escaped((char_u *)filev[i],
#ifdef UNIX
		PATH_ESC_CHARS
#else
		(char_u *)" "
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
    /* The :drop commands goes to Insert mode when 'insertmode' is set, use
     * CTRL-\ CTRL-N again. */
    ga_concat(&ga, (char_u *)"|if exists('*inputrestore')|call inputrestore()|endif<CR>");
    ga_concat(&ga, (char_u *)"<C-\\><C-N>:cd -");
    if (sendReply)
	ga_concat(&ga, (char_u *)"<CR>:call SetupRemoteReplies()");
    ga_concat(&ga, (char_u *)"<CR>:");
    if (inicmd != NULL)
    {
	/* Can't use <CR> after "inicmd", because an "startinsert" would cause
	 * the following commands to be inserted as text.  Use a "|",
	 * hopefully "inicmd" does allow this... */
	ga_concat(&ga, inicmd);
	ga_concat(&ga, (char_u *)"|");
    }
    /* Bring the window to the foreground, goto Insert mode when 'im' set and
     * clear command line. */
    ga_concat(&ga, (char_u *)"cal foreground()|if &im|star|en|ec<CR>");
    ga_append(&ga, NUL);
    return ga.ga_data;
}

/*
 * Replace termcodes such as <CR> and insert as key presses if there is room.
 */
    void
server_to_input_buf(str)
    char_u	*str;
{
    char_u      *ptr = NULL;
    char_u      *cpo_save = p_cpo;

    /* Set 'cpoptions' the way we want it.
     *    B set - backslashes are *not* treated specially
     *    k set - keycodes are *not* reverse-engineered
     *    < unset - <Key> sequences *are* interpreted
     *  The last parameter of replace_termcodes() is TRUE so that the <lt>
     *  sequence is recognised - needed for a real backslash.
     */
    p_cpo = (char_u *)"Bk";
    str = replace_termcodes((char_u *)str, &ptr, FALSE, TRUE);
    p_cpo = cpo_save;

    if (*ptr != NUL)	/* trailing CTRL-V results in nothing */
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

	/* Let input_available() know we inserted text in the typeahead
	 * buffer. */
	received_from_client = TRUE;
    }
    vim_free((char_u *)ptr);
}

/*
 * Evaluate an expression that the client sent to a string.
 * Handles disabling error messages and disables debugging, otherwise Vim
 * hangs, waiting for "cont" to be typed.
 */
    char_u *
eval_client_expr_to_string(expr)
    char_u *expr;
{
    char_u	*res;
    int		save_dbl = debug_break_level;
    int		save_ro = redir_off;

    debug_break_level = -1;
    redir_off = 0;
    ++emsg_skip;

    res = eval_to_string(expr, NULL);

    debug_break_level = save_dbl;
    redir_off = save_ro;
    --emsg_skip;

    return res;
}


/*
 * Make our basic server name: use the specified "arg" if given, otherwise use
 * the tail of the command "cmd" we were started with.
 * Return the name in allocated memory.  This doesn't include a serial number.
 */
    static char_u *
serverMakeName(arg, cmd)
    char_u	*arg;
    char	*cmd;
{
    char_u *p;

    if (arg != NULL && *arg != NUL)
	p = vim_strsave_up(arg);
    else
    {
	p = vim_strsave_up(gettail((char_u *)cmd));
	/* Remove .exe or .bat from the name. */
	if (p != NULL && vim_strchr(p, '.') != NULL)
	    *vim_strchr(p, '.') = NUL;
    }
    return p;
}
#endif /* FEAT_CLIENTSERVER */

/*
 * When FEAT_FKMAP is defined, also compile the Farsi source code.
 */
#if defined(FEAT_FKMAP) || defined(PROTO)
# include "farsi.c"
#endif

/*
 * When FEAT_ARABIC is defined, also compile the Arabic source code.
 */
#if defined(FEAT_ARABIC) || defined(PROTO)
# include "arabic.c"
#endif
