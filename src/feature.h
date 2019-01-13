/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
/*
 * feature.h: Defines for optional code and preferences
 *
 * Edit this file to include/exclude parts of Vim, before compiling.
 * The only other file that may be edited is Makefile, it contains machine
 * specific options.
 *
 * To include specific options, change the "#if*" and "#endif" into comments,
 * or uncomment the "#define".
 * To exclude specific options, change the "#define" into a comment.
 */

/*
 * When adding a new feature:
 * - Add a #define below.
 * - Add a message in the table above ex_version().
 * - Add a string to f_has().
 * - Add a feature to ":help feature-list" in doc/eval.txt.
 * - Add feature to ":help +feature-list" in doc/various.txt.
 * - Add comment for the documentation of commands that use the feature.
 */

/*
 * Basic choices:
 * ==============
 *
 * +tiny		almost no features enabled, not even multiple windows
 * +small		few features enabled, as basic as possible
 * +normal		A default selection of features enabled
 * +big			many features enabled, as rich as possible.
 * +huge		all possible features enabled.
 *
 * When +small is used, +tiny is also included.  +normal implies +small, etc.
 */

/*
 * Uncomment one of these to override the default.  For unix use a configure
 * argument, see Makefile.
 */
#if !defined(FEAT_TINY) && !defined(FEAT_SMALL) && !defined(FEAT_NORMAL) \
	&& !defined(FEAT_BIG) && !defined(FEAT_HUGE)
/* #define FEAT_TINY */
/* #define FEAT_SMALL */
/* #define FEAT_NORMAL */
/* #define FEAT_BIG */
/* #define FEAT_HUGE */
#endif

/*
 * For Unix, Mac and Win32 use +huge by default.  These days CPUs are fast and
 * Memory is cheap.
 * Use +big for older systems: Other MS-Windows and VMS.
 * Otherwise use +normal
 */
#if !defined(FEAT_TINY) && !defined(FEAT_SMALL) && !defined(FEAT_NORMAL) \
	&& !defined(FEAT_BIG) && !defined(FEAT_HUGE)
# if defined(UNIX) || defined(WIN3264) || defined(MACOS_X)
#  define FEAT_HUGE
# else
#  if defined(MSWIN) || defined(VMS) || defined(AMIGA)
#   define FEAT_BIG
#  else
#   define FEAT_NORMAL
#  endif
# endif
#endif

/*
 * Each feature implies including the "smaller" ones.
 */
#ifdef FEAT_HUGE
# define FEAT_BIG
#endif
#ifdef FEAT_BIG
# define FEAT_NORMAL
#endif
#ifdef FEAT_NORMAL
# define FEAT_SMALL
#endif
#ifdef FEAT_SMALL
# define FEAT_TINY
#endif

/*
 * Optional code (see ":help +feature-list")
 * =============
 */

/*
 * These features used to be optional but are now always enabled.
 * +windows		Multiple windows.  Without this there is no help
 *			window and no status lines.
 * +vertsplit		Vertically split windows.
 */

/*
 * +cmdhist		Command line history.
 */
#ifdef FEAT_SMALL
# define FEAT_CMDHIST
#endif

/*
 * Message history is fixed at 200 message, 20 for the tiny version.
 */
#ifdef FEAT_SMALL
# define MAX_MSG_HIST_LEN 200
#else
# define MAX_MSG_HIST_LEN 20
#endif

/*
 * +jumplist		Jumplist, CTRL-O and CTRL-I commands.
 */
#ifdef FEAT_SMALL
# define FEAT_JUMPLIST
#endif

/* the cmdline-window requires FEAT_CMDHIST */
#if defined(FEAT_CMDHIST)
# define FEAT_CMDWIN
#endif

/*
 * +folding		Fold lines.
 */
#ifdef FEAT_NORMAL
# define FEAT_FOLDING
#endif

/*
 * +digraphs		Digraphs.
 *			In insert mode and on the command line you will be
 *			able to use digraphs. The CTRL-K command will work.
 *			Define OLD_DIGRAPHS to get digraphs compatible with
 *			Vim 5.x.  The new ones are from RFC 1345.
 */
#ifdef FEAT_NORMAL
# define FEAT_DIGRAPHS
/* #define OLD_DIGRAPHS */
#endif

/*
 * +langmap		'langmap' option.  Only useful when you put your
 *			keyboard in a special language mode, e.g. for typing
 *			greek.
 */
#ifdef FEAT_BIG
# define FEAT_LANGMAP
#endif

/*
 * +keymap		'keymap' option.  Allows you to map typed keys in
 *			Insert mode for a special language.
 */
#ifdef FEAT_BIG
# define FEAT_KEYMAP
#endif

/*
 * +localmap		Mappings and abbreviations local to a buffer.
 */
#ifdef FEAT_NORMAL
# define FEAT_LOCALMAP
#endif

/*
 * +insert_expand	CTRL-N/CTRL-P/CTRL-X in insert mode. Takes about
 *			4Kbyte of code.
 */
#ifdef FEAT_NORMAL
# define FEAT_INS_EXPAND
#endif

/*
 * +cmdline_compl	completion of mappings/abbreviations in cmdline mode.
 *			Takes a few Kbyte of code.
 */
#ifdef FEAT_NORMAL
# define FEAT_CMDL_COMPL
#endif

#ifdef FEAT_NORMAL
# define VIM_BACKTICK		/* internal backtick expansion */
#endif

/*
 * +visual		Visual mode - now always included.
 * +visualextra		Extra features for Visual mode (mostly block operators).
 *			Now always included.
 */

/*
 * +virtualedit		'virtualedit' option and its implementation
 */
#ifdef FEAT_NORMAL
# define FEAT_VIRTUALEDIT
#endif

/*
 * +cmdline_info	'showcmd' and 'ruler' options.
 */
#ifdef FEAT_NORMAL
# define FEAT_CMDL_INFO
#endif

/*
 * +linebreak		'showbreak', 'breakat'  and 'linebreak' options.
 *			Also 'numberwidth'.
 */
#ifdef FEAT_NORMAL
# define FEAT_LINEBREAK
#endif

/*
 * +extra_search	'hlsearch' and 'incsearch' options.
 */
#ifdef FEAT_NORMAL
# define FEAT_SEARCH_EXTRA
#endif

/*
 * +quickfix		Quickfix commands.
 */
#ifdef FEAT_NORMAL
# define FEAT_QUICKFIX
#endif

/*
 * +file_in_path	"gf" and "<cfile>" commands.
 */
#ifdef FEAT_NORMAL
# define FEAT_SEARCHPATH
#endif

/*
 * +find_in_path	"[I" ":isearch" "^W^I", ":checkpath", etc.
 */
#ifdef FEAT_NORMAL
# ifdef FEAT_SEARCHPATH	/* FEAT_SEARCHPATH is required */
#  define FEAT_FIND_ID
# endif
#endif

/*
 * +path_extra		up/downwards searching in 'path' and 'tags'.
 */
#ifdef FEAT_NORMAL
# define FEAT_PATH_EXTRA
#endif

/*
 * +rightleft		Right-to-left editing/typing support.
 *
 * Disabled for EBCDIC as it requires multibyte.
 */
#if defined(FEAT_BIG) && !defined(DISABLE_RIGHTLEFT) && !defined(EBCDIC)
# define FEAT_RIGHTLEFT
#endif

/*
 * +farsi		Farsi (Persian language) Keymap support.
 *			Requires FEAT_RIGHTLEFT.
 *
 * Disabled for EBCDIC as it requires multibyte.
 */
#if defined(FEAT_BIG) && !defined(DISABLE_FARSI) && !defined(EBCDIC)
# define FEAT_FKMAP
#endif
#ifdef FEAT_FKMAP
# ifndef FEAT_RIGHTLEFT
#   define FEAT_RIGHTLEFT
# endif
#endif

/*
 * +arabic		Arabic keymap and shaping support.
 *			Requires FEAT_RIGHTLEFT
 *
 * Disabled for EBCDIC as it requires multibyte.
 */
#if defined(FEAT_BIG) && !defined(DISABLE_ARABIC) && VIM_SIZEOF_INT >= 4 && !defined(EBCDIC)
# define FEAT_ARABIC
#endif
#ifdef FEAT_ARABIC
# ifndef FEAT_RIGHTLEFT
#   define FEAT_RIGHTLEFT
# endif
#endif

/*
 * +emacs_tags		When FEAT_EMACS_TAGS defined: Include support for
 *			emacs style TAGS file.
 */
#ifdef FEAT_BIG
# define FEAT_EMACS_TAGS
#endif

/*
 * +tag_binary		Can use a binary search for the tags file.
 *
 * Disabled for EBCDIC:
 * On z/OS Unix we have the problem that /bin/sort sorts ASCII instead of
 * EBCDIC.  With this binary search doesn't work, as VIM expects a tag file
 * sorted by character values.  I'm not sure how to fix this. Should we really
 * do a EBCDIC to ASCII conversion for this??
 */
#if !defined(EBCDIC)
# define FEAT_TAG_BINS
#endif

/*
 * +tag_old_static	Old style static tags: "file:tag  file  ..".  Slows
 *			down tag searching a bit.
 */
#ifdef FEAT_NORMAL
# define FEAT_TAG_OLDSTATIC
#endif

/*
 * +tag_any_white	Allow any white space to separate the fields in a tags
 *			file.  When not defined, only a TAB is allowed.
 */
/* #define FEAT_TAG_ANYWHITE */

/*
 * +cscope		Unix only: Cscope support.
 */
#if defined(UNIX) && defined(FEAT_BIG) && !defined(FEAT_CSCOPE) && !defined(MACOS_X)
# define FEAT_CSCOPE
#endif

/*
 * +eval		Built-in script language and expression evaluation,
 *			":let", ":if", etc.
 * +float		Floating point variables.
 * +num64		64-bit Number.
 */
#ifdef FEAT_NORMAL
# define FEAT_EVAL
# if defined(HAVE_FLOAT_FUNCS) || defined(WIN3264) || defined(MACOS_X)
#  define FEAT_FLOAT
# endif
# if defined(HAVE_STDINT_H) || defined(WIN3264) || (VIM_SIZEOF_LONG >= 8)
#  define FEAT_NUM64
# endif
#endif

#ifdef FEAT_EVAL
# define HAVE_SANDBOX
#endif

/*
 * +profile		Profiling for functions and scripts.
 */
#if defined(FEAT_HUGE) \
	&& defined(FEAT_EVAL) \
	&& ((defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)) \
		|| defined(WIN3264))
# define FEAT_PROFILE
#endif

/*
 * +reltime		reltime() function
 */
#if defined(FEAT_NORMAL) \
	&& defined(FEAT_EVAL) \
	&& ((defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)) \
		|| defined(WIN3264))
# define FEAT_RELTIME
#endif

/*
 * +timers		timer_start()
 */
#if defined(FEAT_RELTIME) && (defined(UNIX) || defined(WIN32))
# define FEAT_TIMERS
#endif

/*
 * +textobjects		Text objects: "vaw", "das", etc.
 */
#if defined(FEAT_NORMAL) && defined(FEAT_EVAL)
# define FEAT_TEXTOBJ
#endif

/*
 *			Insert mode completion with 'completefunc'.
 */
#if defined(FEAT_INS_EXPAND) && defined(FEAT_EVAL)
# define FEAT_COMPL_FUNC
#endif

/*
 * +user_commands	Allow the user to define his own commands.
 */
#ifdef FEAT_NORMAL
# define FEAT_USR_CMDS
#endif

/*
 * +printer		":hardcopy" command
 * +postscript		Printing uses PostScript file output.
 */
#if defined(FEAT_NORMAL) && (defined(MSWIN) || defined(FEAT_EVAL)) \
	&& !defined(AMIGA)
# define FEAT_PRINTER
#endif
#if defined(FEAT_PRINTER) && ((defined(MSWIN) && defined(MSWINPS)) \
	|| (!defined(MSWIN) && defined(FEAT_EVAL)))
# define FEAT_POSTSCRIPT
#endif

/*
 * +modify_fname	modifiers for file name.  E.g., "%:p:h".
 */
#ifdef FEAT_NORMAL
# define FEAT_MODIFY_FNAME
#endif

/*
 * +diff		Displaying diffs in a nice way.
 *			Requires +windows and +autocmd.
 */
#if defined(FEAT_NORMAL)
# define FEAT_DIFF
#endif

/*
 * +title		'title' and 'icon' options
 * +statusline		'statusline', 'rulerformat' and special format of
 *			'titlestring' and 'iconstring' options.
 * +byte_offset		'%o' in 'statusline' and builtin functions line2byte()
 *			and byte2line().
 *			Note: Required for Macintosh.
 */
#if defined(FEAT_NORMAL)
# define FEAT_TITLE
#endif

#ifdef FEAT_NORMAL
# define FEAT_STL_OPT
# ifndef FEAT_CMDL_INFO
#  define FEAT_CMDL_INFO	/* 'ruler' is required for 'statusline' */
# endif
#endif

#ifdef FEAT_NORMAL
# define FEAT_BYTEOFF
#endif

/*
 * +wildignore		'wildignore' and 'backupskip' options
 *			Needed for Unix to make "crontab -e" work.
 */
#if defined(FEAT_NORMAL) || defined(UNIX)
# define FEAT_WILDIGN
#endif

/*
 * +wildmenu		'wildmenu' option
 */
#if defined(FEAT_NORMAL)
# define FEAT_WILDMENU
#endif

/*
 * +viminfo		reading/writing the viminfo file. Takes about 8Kbyte
 *			of code.
 * VIMINFO_FILE		Location of user .viminfo file (should start with $).
 * VIMINFO_FILE2	Location of alternate user .viminfo file.
 */
#ifdef FEAT_NORMAL
# define FEAT_VIMINFO
/* #define VIMINFO_FILE	"$HOME/foo/.viminfo" */
/* #define VIMINFO_FILE2 "~/bar/.viminfo" */
#endif

/*
 * +syntax		syntax highlighting.  When using this, it's a good
 *			idea to have +autocmd and +eval too.
 */
#if defined(FEAT_NORMAL) || defined(PROTO)
# define FEAT_SYN_HL
#endif

/*
 * +conceal		'conceal' option.  Needs syntax highlighting
 *			as this is how the concealed text is defined.
 */
#if defined(FEAT_BIG) && defined(FEAT_SYN_HL)
# define FEAT_CONCEAL
#endif

/*
 * +textprop		Text properties
 */
#if defined(FEAT_EVAL) && defined(FEAT_SYN_HL)
# define FEAT_TEXT_PROP
#endif

/*
 * +spell		spell checking
 *
 * Disabled for EBCDIC: * Doesn't work (SIGSEGV).
 */
#if (defined(FEAT_NORMAL) || defined(PROTO)) && !defined(EBCDIC)
# define FEAT_SPELL
#endif

/*
 * +builtin_terms	Choose one out of the following four:
 *
 * NO_BUILTIN_TCAPS	Do not include any builtin termcap entries (used only
 *			with HAVE_TGETENT defined).
 *
 * (nothing)		Machine specific termcap entries will be included.
 *
 * SOME_BUILTIN_TCAPS	Include most useful builtin termcap entries (used only
 *			with NO_BUILTIN_TCAPS not defined).
 *			This is the default.
 *
 * ALL_BUILTIN_TCAPS	Include all builtin termcap entries
 *			(used only with NO_BUILTIN_TCAPS not defined).
 */
#ifdef HAVE_TGETENT
/* #define NO_BUILTIN_TCAPS */
#endif

#if !defined(NO_BUILTIN_TCAPS)
# ifdef FEAT_BIG
#  define ALL_BUILTIN_TCAPS
# else
#  define SOME_BUILTIN_TCAPS		/* default */
# endif
#endif

/*
 * +lispindent		lisp indenting (From Eric Fischer).
 * +cindent		C code indenting (From Eric Fischer).
 * +smartindent		smart C code indenting when the 'si' option is set.
 *
 * These two need to be defined when making prototypes.
 */
#if defined(FEAT_NORMAL) || defined(PROTO)
# define FEAT_LISP
#endif

#if defined(FEAT_NORMAL) || defined(PROTO)
# define FEAT_CINDENT
#endif

#ifdef FEAT_NORMAL
# define FEAT_SMARTINDENT
#endif

/*
 * +comments		'comments' option.
 */
#ifdef FEAT_NORMAL
# define FEAT_COMMENTS
#endif

/*
 * +cryptv		Encryption (by Mohsin Ahmed <mosh@sasi.com>).
 */
#if defined(FEAT_NORMAL) && !defined(FEAT_CRYPT) || defined(PROTO)
# define FEAT_CRYPT
#endif

/*
 * +mksession		":mksession" command.
 *			Requires +windows and +vertsplit.
 */
#if defined(FEAT_NORMAL)
# define FEAT_SESSION
#endif

/*
 * +multi_lang		Multi language support. ":menutrans", ":language", etc.
 * +gettext		Message translations (requires +multi_lang)
 *			(only when "lang" archive unpacked)
 */
#ifdef FEAT_NORMAL
# define FEAT_MULTI_LANG
#endif
#if defined(HAVE_GETTEXT) && defined(FEAT_MULTI_LANG) \
	&& (defined(HAVE_LOCALE_H) || defined(X_LOCALE))
# define FEAT_GETTEXT
#endif

/*
 * +multi_byte		Generic multi-byte character handling.
 *			Now always enabled.
 */
#if !defined(FEAT_MBYTE)
# define FEAT_MBYTE
#endif
#if VIM_SIZEOF_INT < 4 && !defined(PROTO)
	Error: Vim only works with 32 bit int or larger
#endif

/* Define this if you want to use 16 bit Unicode only, reduces memory used for
 * the screen structures. */
/* #define UNICODE16 */

/*
 * +multi_byte_ime	Win32 IME input method.  Requires +multi_byte.
 *			Only for far-east Windows, so IME can be used to input
 *			chars.  Not tested much!
 */
#if defined(FEAT_GUI_W32) && !defined(FEAT_MBYTE_IME)
/* #define FEAT_MBYTE_IME */
# endif

/* Use iconv() when it's available. */
#if (defined(HAVE_ICONV_H) && defined(HAVE_ICONV)) || defined(DYNAMIC_ICONV)
# define USE_ICONV
#endif

/*
 * +xim			X Input Method.  For entering special languages like
 *			chinese and Japanese.
 * +hangul_input	Internal Hangul input method.  Must be included
 *			through configure: "--enable-hangulin"
 * Both are for Unix and VMS only.
 */
#ifndef FEAT_XIM
/* #define FEAT_XIM */
#endif

#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
# define USE_XIM 1		/* needed for GTK include files */
#endif

#ifdef FEAT_HANGULIN
# define HANGUL_DEFAULT_KEYBOARD 2	/* 2 or 3 bulsik keyboard */
# define ESC_CHG_TO_ENG_MODE		/* if defined, when ESC pressed,
					 * turn to english mode
					 */
# if defined(FEAT_XIM) && !defined(LINT)
	Error: You should select only ONE of XIM and HANGUL INPUT
# endif
#endif
#if defined(FEAT_HANGULIN) || defined(FEAT_XIM)
/* # define X_LOCALE */			/* for OS with incomplete locale
					   support, like old linux versions. */
#endif

/*
 * +xfontset		X fontset support.  For outputting wide characters.
 */
#ifndef FEAT_XFONTSET
# if defined(HAVE_X11) && !defined(FEAT_GUI_GTK)
#  define FEAT_XFONTSET
# else
/* #  define FEAT_XFONTSET */
# endif
#endif

/*
 * +libcall		libcall() function
 */
/* Using dlopen() also requires dlsym() to be available. */
#if defined(HAVE_DLOPEN) && defined(HAVE_DLSYM)
# define USE_DLOPEN
#endif
#if defined(FEAT_EVAL) && (defined(WIN3264) || ((defined(UNIX) || defined(VMS)) \
	&& (defined(USE_DLOPEN) || defined(HAVE_SHL_LOAD))))
# define FEAT_LIBCALL
#endif

/*
 * +menu		":menu" command
 */
#ifdef FEAT_NORMAL
# define FEAT_MENU
# ifdef FEAT_GUI_W32
#  define FEAT_TEAROFF
# endif
#endif

/*
 * popup menu in a terminal
 */
#if defined(FEAT_MENU) && !defined(ALWAYS_USE_GUI) && defined(FEAT_INS_EXPAND)
# define FEAT_TERM_POPUP_MENU
#endif

/* There are two ways to use XPM. */
#if (defined(HAVE_XM_XPMP_H) && defined(FEAT_GUI_MOTIF)) \
		|| defined(HAVE_X11_XPM_H)
# define HAVE_XPM 1
#endif

/*
 * +toolbar		Include code for a toolbar (for the Win32 GUI, GTK
 *			always has it).  But only if menus are enabled.
 */
#if defined(FEAT_NORMAL) && defined(FEAT_MENU) \
	&& (defined(FEAT_GUI_GTK) \
		|| defined(FEAT_GUI_MSWIN) \
		|| ((defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)) \
			&& defined(HAVE_XPM)) \
		|| defined(FEAT_GUI_PHOTON))
# define FEAT_TOOLBAR
#endif


#if defined(FEAT_TOOLBAR) && !defined(FEAT_MENU)
# define FEAT_MENU
#endif

/*
 * GUI tabline
 */
#if defined(FEAT_NORMAL) \
    && (defined(FEAT_GUI_GTK) \
	|| (defined(FEAT_GUI_MOTIF) && defined(HAVE_XM_NOTEBOOK_H)) \
	|| defined(FEAT_GUI_MAC) \
	|| (defined(FEAT_GUI_MSWIN) \
	    && (!defined(_MSC_VER) || _MSC_VER > 1020)))
# define FEAT_GUI_TABLINE
#endif

/*
 * +browse		":browse" command.
 *			or just the ":browse" command modifier
 */
#if defined(FEAT_NORMAL)
# define FEAT_BROWSE_CMD
# if defined(FEAT_GUI_MSWIN) || defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA) || defined(FEAT_GUI_GTK) || defined(FEAT_GUI_PHOTON) || defined(FEAT_GUI_MAC)
#  define FEAT_BROWSE
# endif
#endif

/*
 * On some systems, when we compile with the GUI, we always use it.  On Mac
 * there is no terminal version, and on Windows we can't figure out how to
 * fork one off with :gui.
 */
#if defined(FEAT_GUI_MSWIN) || (defined(FEAT_GUI_MAC) && !defined(MACOS_X_DARWIN))
# define ALWAYS_USE_GUI
#endif

/*
 * +dialog_gui		Use GUI dialog.
 * +dialog_con		May use Console dialog.
 *			When none of these defined there is no dialog support.
 */
#ifdef FEAT_NORMAL
# if ((defined(FEAT_GUI_ATHENA) || defined(FEAT_GUI_MOTIF)) \
		&& defined(HAVE_X11_XPM_H)) \
	|| defined(FEAT_GUI_GTK) \
	|| defined(FEAT_GUI_PHOTON) \
	|| defined(FEAT_GUI_MSWIN) \
	|| defined(FEAT_GUI_MAC)
#  define FEAT_CON_DIALOG
#  define FEAT_GUI_DIALOG
# else
#  define FEAT_CON_DIALOG
# endif
#endif
#if !defined(FEAT_GUI_DIALOG) && (defined(FEAT_GUI_MOTIF) \
	|| defined(FEAT_GUI_ATHENA) || defined(FEAT_GUI_GTK) \
	|| defined(FEAT_GUI_W32))
/* need a dialog to show error messages when starting from the desktop */
# define FEAT_GUI_DIALOG
#endif
#if defined(FEAT_GUI_DIALOG) && \
	(defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA) \
	 || defined(FEAT_GUI_GTK) || defined(FEAT_GUI_MSWIN) \
	 || defined(FEAT_GUI_PHOTON) || defined(FEAT_GUI_MAC))
# define FEAT_GUI_TEXTDIALOG
# ifndef ALWAYS_USE_GUI
#  define FEAT_CON_DIALOG
# endif
#endif

/*
 * +termguicolors	'termguicolors' option.
 */
#if (defined(FEAT_BIG) && defined(FEAT_SYN_HL)) && !defined(ALWAYS_USE_GUI)
# define FEAT_TERMGUICOLORS
#endif

/* Mac specific thing: Codewarrior interface. */
#ifdef FEAT_GUI_MAC
# define FEAT_CW_EDITOR
#endif

/*
 * +vartabs		'vartabstop' and 'varsofttabstop' options.
 */
#ifdef FEAT_BIG
# define FEAT_VARTABS
#endif

/*
 * Preferences:
 * ============
 */

/*
 * +writebackup		'writebackup' is default on:
 *			Use a backup file while overwriting a file.  But it's
 *			deleted again when 'backup' is not set.  Changing this
 *			is strongly discouraged: You can lose all your
 *			changes when the computer crashes while writing the
 *			file.
 *			VMS note: It does work on VMS as well, but because of
 *			version handling it does not have any purpose.
 *			Overwrite will write to the new version.
 */
#ifndef VMS
# define FEAT_WRITEBACKUP
#endif

/*
 * +xterm_save		The t_ti and t_te entries for the builtin xterm will
 *			be set to save the screen when starting Vim and
 *			restoring it when exiting.
 */
/* #define FEAT_XTERM_SAVE */

/*
 * DEBUG		Output a lot of debugging garbage.
 */
/* #define DEBUG */

/*
 * STARTUPTIME		Time the startup process.  Writes a file with
 *			timestamps.
 */
#if defined(FEAT_NORMAL) \
	&& ((defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)) \
		|| defined(WIN3264))
# define STARTUPTIME 1
#endif

/*
 * MEM_PROFILE		Debugging of memory allocation and freeing.
 */
/* #define MEM_PROFILE */

/*
 * VIMRC_FILE		Name of the .vimrc file in current dir.
 */
/* #define VIMRC_FILE	".vimrc" */

/*
 * EXRC_FILE		Name of the .exrc file in current dir.
 */
/* #define EXRC_FILE	".exrc" */

/*
 * GVIMRC_FILE		Name of the .gvimrc file in current dir.
 */
/* #define GVIMRC_FILE	".gvimrc" */

/*
 * SESSION_FILE		Name of the default ":mksession" file.
 */
#define SESSION_FILE	"Session.vim"

/*
 * USR_VIMRC_FILE	Name of the user .vimrc file.
 * USR_VIMRC_FILE2	Name of alternate user .vimrc file.
 * USR_VIMRC_FILE3	Name of alternate user .vimrc file.
 */
/* #define USR_VIMRC_FILE	"~/foo/.vimrc" */
/* #define USR_VIMRC_FILE2	"~/bar/.vimrc" */
/* #define USR_VIMRC_FILE3	"$VIM/.vimrc" */

/*
 * VIM_DEFAULTS_FILE	Name of the defaults.vim script file
 */
/* #define VIM_DEFAULTS_FILE	"$VIMRUNTIME/defaults.vim" */

/*
 * EVIM_FILE		Name of the evim.vim script file
 */
/* #define EVIM_FILE		"$VIMRUNTIME/evim.vim" */

/*
 * USR_EXRC_FILE	Name of the user .exrc file.
 * USR_EXRC_FILE2	Name of the alternate user .exrc file.
 */
/* #define USR_EXRC_FILE	"~/foo/.exrc" */
/* #define USR_EXRC_FILE2	"~/bar/.exrc" */

/*
 * USR_GVIMRC_FILE	Name of the user .gvimrc file.
 * USR_GVIMRC_FILE2	Name of the alternate user .gvimrc file.
 */
/* #define USR_GVIMRC_FILE	"~/foo/.gvimrc" */
/* #define USR_GVIMRC_FILE2	"~/bar/.gvimrc" */
/* #define USR_GVIMRC_FILE3	"$VIM/.gvimrc" */

/*
 * SYS_VIMRC_FILE	Name of the system-wide .vimrc file.
 */
/* #define SYS_VIMRC_FILE	"/etc/vimrc" */

/*
 * SYS_GVIMRC_FILE	Name of the system-wide .gvimrc file.
 */
/* #define SYS_GVIMRC_FILE	"/etc/gvimrc" */

/*
 * DFLT_HELPFILE	Name of the help file.
 */
/* # define DFLT_HELPFILE	"$VIMRUNTIME/doc/help.txt.gz" */

/*
 * File names for:
 * FILETYPE_FILE	switch on file type detection
 * FTPLUGIN_FILE	switch on loading filetype plugin files
 * INDENT_FILE		switch on loading indent files
 * FTOFF_FILE		switch off file type detection
 * FTPLUGOF_FILE	switch off loading settings files
 * INDOFF_FILE		switch off loading indent files
 */
/* # define FILETYPE_FILE	"filetype.vim" */
/* # define FTPLUGIN_FILE	"ftplugin.vim" */
/* # define INDENT_FILE		"indent.vim" */
/* # define FTOFF_FILE		"ftoff.vim" */
/* # define FTPLUGOF_FILE	"ftplugof.vim" */
/* # define INDOFF_FILE		"indoff.vim" */

/*
 * SYS_MENU_FILE	Name of the default menu.vim file.
 */
/* # define SYS_MENU_FILE	"$VIMRUNTIME/menu.vim" */

/*
 * SYS_OPTWIN_FILE	Name of the default optwin.vim file.
 */
#ifndef SYS_OPTWIN_FILE
# define SYS_OPTWIN_FILE	"$VIMRUNTIME/optwin.vim"
#endif

/*
 * SYNTAX_FNAME		Name of a syntax file, where %s is the syntax name.
 */
/* #define SYNTAX_FNAME	"/foo/%s.vim" */

/*
 * RUNTIME_DIRNAME	Generic name for the directory of the runtime files.
 */
#ifndef RUNTIME_DIRNAME
# define RUNTIME_DIRNAME "runtime"
#endif

/*
 * RUNTIME_GLOBAL	Comma-separated list of directory names for global Vim
 *			runtime directories.
 *			Don't define this if the preprocessor can't handle
 *			string concatenation.
 *			Also set by "--with-global-runtime" configure argument.
 */
/* #define RUNTIME_GLOBAL "/etc/vim" */

/*
 * RUNTIME_GLOBAL_AFTER	Comma-separated list of directory names for global Vim
 *			runtime after directories.
 *			Don't define this if the preprocessor can't handle
 *			string concatenation.
 *			Also set by "--with-global-runtime" configure argument.
 */
/* #define RUNTIME_GLOBAL_AFTER "/etc/vim/after" */

/*
 * MODIFIED_BY		Name of who modified Vim.  Required when distributing
 *			a modified version of Vim.
 *			Also from the "--with-modified-by" configure argument.
 */
/* #define MODIFIED_BY "John Doe" */

/*
 * Machine dependent:
 * ==================
 */

/*
 * +fork		Unix only: fork() support (detected by configure)
 * +system		Use system() instead of fork/exec for starting a
 *			shell.  Doesn't work for the GUI!
 */
/* #define USE_SYSTEM */

/*
 * +X11			Unix only.  Include code for xterm title saving and X
 *			clipboard.  Only works if HAVE_X11 is also defined.
 */
#if (defined(FEAT_NORMAL) || defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA))
# define WANT_X11
#endif

/*
 * XSMP - X11 Session Management Protocol
 * It may be preferred to disable this if the GUI supports it (e.g.,
 * GNOME/KDE) and implement save-yourself etc. through that, but it may also
 * be cleaner to have all SM-aware vims do the same thing (libSM does not
 * depend upon X11).
 * If your GUI wants to support SM itself, change this ifdef.
 * I'm assuming that any X11 implementation will cope with this for now.
 */
#if defined(HAVE_X11) && defined(WANT_X11) && defined(HAVE_X11_SM_SMLIB_H)
# define USE_XSMP
#endif
#if defined(USE_XSMP_INTERACT) && !defined(USE_XSMP)
# undef USE_XSMP_INTERACT
#endif

/*
 * +mouse_xterm		Unix only: Include code for xterm mouse handling.
 * +mouse_dec		idem, for Dec mouse handling.
 * +mouse_jsbterm	idem, for Jsbterm mouse handling.
 * +mouse_netterm	idem, for Netterm mouse handling.
 * (none)		MS-DOS mouse support.
 * +mouse_gpm		Unix only: Include code for Linux console mouse
 *			handling.
 * +mouse_pterm		PTerm mouse support for QNX
 * +mouse_sgr		Unix only: Include code for for SGR-styled mouse.
 * +mouse_sysmouse	Unix only: Include code for FreeBSD and DragonFly
 *			console mouse handling.
 * +mouse_urxvt		Unix only: Include code for for urxvt mosue handling.
 * +mouse		Any mouse support (any of the above enabled).
 */
/* OS/2 and Amiga console have no mouse support */
#if !defined(AMIGA)
# ifdef FEAT_NORMAL
#  define FEAT_MOUSE_XTERM
# endif
# ifdef FEAT_BIG
#  define FEAT_MOUSE_NET
# endif
# ifdef FEAT_BIG
#  define FEAT_MOUSE_DEC
# endif
# ifdef FEAT_BIG
#  define FEAT_MOUSE_URXVT
# endif
# ifdef FEAT_BIG
#  define FEAT_MOUSE_SGR
# endif
# if defined(FEAT_NORMAL) && defined(WIN3264)
#  define DOS_MOUSE
# endif
# if defined(FEAT_NORMAL) && defined(__QNX__)
#  define FEAT_MOUSE_PTERM
# endif
#endif

/*
 * Note: Only one of the following may be defined:
 * FEAT_MOUSE_GPM
 * FEAT_SYSMOUSE
 * FEAT_MOUSE_JSB
 * FEAT_MOUSE_PTERM
 */
#if defined(FEAT_NORMAL) && defined(HAVE_GPM)
# define FEAT_MOUSE_GPM
#endif

#if defined(FEAT_NORMAL) && defined(HAVE_SYSMOUSE)
# define FEAT_SYSMOUSE
#endif

/* urxvt is a small variation of mouse_xterm, and shares its code */
#if defined(FEAT_MOUSE_URXVT) && !defined(FEAT_MOUSE_XTERM)
# define FEAT_MOUSE_XTERM
#endif

/* sgr is a small variation of mouse_xterm, and shares its code */
#if defined(FEAT_MOUSE_SGR) && !defined(FEAT_MOUSE_XTERM)
# define FEAT_MOUSE_XTERM
#endif

/* Define FEAT_MOUSE when any of the above is defined or FEAT_GUI. */
#if !defined(FEAT_MOUSE_TTY) \
	&& (defined(FEAT_MOUSE_XTERM) \
	    || defined(FEAT_MOUSE_NET) \
	    || defined(FEAT_MOUSE_DEC) \
	    || defined(DOS_MOUSE) \
	    || defined(FEAT_MOUSE_GPM) \
	    || defined(FEAT_MOUSE_JSB) \
	    || defined(FEAT_MOUSE_PTERM) \
	    || defined(FEAT_SYSMOUSE) \
	    || defined(FEAT_MOUSE_URXVT) \
	    || defined(FEAT_MOUSE_SGR))
# define FEAT_MOUSE_TTY		/* include non-GUI mouse support */
#endif
#if !defined(FEAT_MOUSE) && (defined(FEAT_MOUSE_TTY) || defined(FEAT_GUI))
# define FEAT_MOUSE		/* include generic mouse support */
#endif

/*
 * +clipboard		Clipboard support.  Always used for the GUI.
 * +xterm_clipboard	Unix only: Include code for handling the clipboard
 *			in an xterm like in the GUI.
 */

#ifdef FEAT_CYGWIN_WIN32_CLIPBOARD
# define FEAT_CLIPBOARD
#endif

#ifdef FEAT_GUI
# ifndef FEAT_CLIPBOARD
#  define FEAT_CLIPBOARD
# endif
#endif

#if defined(FEAT_NORMAL) \
	&& (defined(UNIX) || defined(VMS)) \
	&& defined(WANT_X11) && defined(HAVE_X11)
# define FEAT_XCLIPBOARD
# ifndef FEAT_CLIPBOARD
#  define FEAT_CLIPBOARD
# endif
#endif

/*
 * +dnd		Drag'n'drop support.  Always used for the GTK+ GUI.
 */
#if defined(FEAT_CLIPBOARD) && defined(FEAT_GUI_GTK)
# define FEAT_DND
#endif

#if defined(FEAT_GUI_MSWIN) && defined(FEAT_SMALL)
# define MSWIN_FIND_REPLACE	/* include code for find/replace dialog */
# define MSWIN_FR_BUFSIZE 256
#endif

#if defined(FEAT_GUI_GTK) || defined(FEAT_GUI_MOTIF) \
	|| defined(MSWIN_FIND_REPLACE)
# define FIND_REPLACE_DIALOG 1
#endif

/*
 * +clientserver	Remote control via the remote_send() function
 *			and the --remote argument
 */
#if (defined(WIN32) || defined(FEAT_XCLIPBOARD)) && defined(FEAT_EVAL)
# define FEAT_CLIENTSERVER
#endif

/*
 * +autoservername	Automatically generate a servername for clientserver
 *			when --servername is not passed on the command line.
 */
#if defined(FEAT_CLIENTSERVER) && !defined(FEAT_AUTOSERVERNAME)
# ifdef WIN3264
    /* Always enabled on MS-Windows. */
#  define FEAT_AUTOSERVERNAME
# else
    /* Enable here if you don't use configure. */
/* # define FEAT_AUTOSERVERNAME */
# endif
#endif

/*
 * +termresponse	send t_RV to obtain terminal response.  Used for xterm
 *			to check if mouse dragging can be used and if term
 *			codes can be obtained.
 */
#if (defined(FEAT_NORMAL) || defined(FEAT_MOUSE)) && defined(HAVE_TGETENT)
# define FEAT_TERMRESPONSE
#endif

/*
 * cursor shape		Adjust the shape of the cursor to the mode.
 * mouse shape		Adjust the shape of the mouse pointer to the mode.
 */
#ifdef FEAT_NORMAL
/* MS-DOS console and Win32 console can change cursor shape */
# if defined(WIN3264) && !defined(FEAT_GUI_W32)
#  define MCH_CURSOR_SHAPE
# endif
# if defined(FEAT_GUI_W32) || defined(FEAT_GUI_MOTIF) \
	|| defined(FEAT_GUI_ATHENA) || defined(FEAT_GUI_GTK) \
	|| defined(FEAT_GUI_PHOTON)
#  define FEAT_MOUSESHAPE
# endif
#endif

/* GUI and some consoles can change the shape of the cursor.  The code is also
 * needed for the 'mouseshape' and 'concealcursor' options. */
#if defined(FEAT_GUI) \
	    || defined(MCH_CURSOR_SHAPE) \
	    || defined(FEAT_MOUSESHAPE) \
	    || defined(FEAT_CONCEAL) \
	    || (defined(UNIX) && defined(FEAT_NORMAL))
# define CURSOR_SHAPE
#endif

#if defined(FEAT_MZSCHEME) && (defined(FEAT_GUI_W32) || defined(FEAT_GUI_GTK)    \
	|| defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)	\
	|| defined(FEAT_GUI_MAC))
# define MZSCHEME_GUI_THREADS
#endif

/*
 * +ARP			Amiga only. Use arp.library, DOS 2.0 is not required.
 */
#if !defined(NO_ARP) && !defined(__amigaos4__)
# define FEAT_ARP
#endif

/*
 * +GUI_Athena		To compile Vim with or without the GUI (gvim) you have
 * +GUI_Motif		to edit the Makefile.
 */

/*
 * +ole			Win32 OLE automation: Use Makefile.ovc.
 */

/*
 * These features can only be included by using a configure argument.  See the
 * Makefile for a line to uncomment.
 * +lua			Lua interface: "--enable-luainterp"
 * +mzscheme		MzScheme interface: "--enable-mzscheme"
 * +perl		Perl interface: "--enable-perlinterp"
 * +python		Python interface: "--enable-pythoninterp"
 * +tcl			TCL interface: "--enable-tclinterp"
 * +sun_workshop	Sun Workshop integration
 * +netbeans_intg	Netbeans integration
 * +channel		Inter process communication
 */

/*
 * These features are automatically detected:
 * +terminfo
 * +tgetent
 */

/*
 * The Sun Workshop features currently only work with Motif.
 */
#if !defined(FEAT_GUI_MOTIF) && defined(FEAT_SUN_WORKSHOP)
# undef FEAT_SUN_WORKSHOP
#endif

/*
 * The Netbeans feature requires +eval.
 */
#if !defined(FEAT_EVAL) && defined(FEAT_NETBEANS_INTG)
# undef FEAT_NETBEANS_INTG
#endif

/*
 * The +channel feature requires +eval.
 */
#if !defined(FEAT_EVAL) && defined(FEAT_JOB_CHANNEL)
# undef FEAT_JOB_CHANNEL
#endif

/*
 * +terminal		":terminal" command.  Runs a terminal in a window.
 *			requires +channel
 */
#if defined(FEAT_TERMINAL) && !defined(FEAT_JOB_CHANNEL)
# undef FEAT_TERMINAL
#endif
#if defined(FEAT_TERMINAL) && !defined(CURSOR_SHAPE)
# define CURSOR_SHAPE
#endif

/*
 * +signs		Allow signs to be displayed to the left of text lines.
 *			Adds the ":sign" command.
 */
#if defined(FEAT_BIG) || defined(FEAT_SUN_WORKSHOP) \
	    || defined(FEAT_NETBEANS_INTG)
# define FEAT_SIGNS
# if ((defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)) \
		&& defined(HAVE_X11_XPM_H)) \
	|| defined(FEAT_GUI_GTK) \
	|| (defined(WIN32) && defined(FEAT_GUI))
#  define FEAT_SIGN_ICONS
# endif
#endif

/*
 * +balloon_eval	Allow balloon expression evaluation. Used with a
 *			debugger and for tooltips.
 *			Only for GUIs where it was implemented.
 */
#if (defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA) \
	|| defined(FEAT_GUI_GTK) || defined(FEAT_GUI_W32)) \
	&& (   ((defined(FEAT_TOOLBAR) || defined(FEAT_GUI_TABLINE)) \
		&& !defined(FEAT_GUI_GTK) && !defined(FEAT_GUI_W32)) \
	    || defined(FEAT_SUN_WORKSHOP) \
	    || defined(FEAT_NETBEANS_INTG) || defined(FEAT_EVAL))
# define FEAT_BEVAL_GUI
# if !defined(FEAT_XFONTSET) && !defined(FEAT_GUI_GTK) \
	&& !defined(FEAT_GUI_W32)
#  define FEAT_XFONTSET
# endif
#endif

#if defined(FEAT_BEVAL_GUI) && (defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA))
# define FEAT_BEVAL_TIP		/* balloon eval used for toolbar tooltip */
#endif

/*
 * +balloon_eval_term	Allow balloon expression evaluation in the terminal.
 */
#if defined(FEAT_HUGE) && defined(FEAT_TIMERS) && \
	(defined(UNIX) || (defined(WIN32) && !defined(FEAT_GUI_W32)))
# define FEAT_BEVAL_TERM
#endif

#if defined(FEAT_BEVAL_GUI) || defined(FEAT_BEVAL_TERM)
# define FEAT_BEVAL
#endif

/* both Motif and Athena are X11 and share some code */
#if defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)
# define FEAT_GUI_X11
#endif

#if defined(FEAT_SUN_WORKSHOP) || defined(FEAT_NETBEANS_INTG)
/*
 * The following features are (currently) only used by Sun Visual WorkShop 6
 * and NetBeans. These features could be used with other integrations with
 * debuggers so I've used separate feature defines.
 */
# if !defined(FEAT_MENU)
#  define FEAT_MENU
# endif
#endif

#if defined(FEAT_SUN_WORKSHOP)
/*
 *			Use an alternative method of X input for a secondary
 *			command input.
 */
# define ALT_X_INPUT

/*
 * +footer		Motif only: Add a message area at the bottom of the
 *			main window area.
 */
# define FEAT_FOOTER

#endif

/*
 * +autochdir		'autochdir' option.
 */
#if defined(FEAT_SUN_WORKSHOP) || defined(FEAT_NETBEANS_INTG) \
	    || defined(FEAT_BIG)
# define FEAT_AUTOCHDIR
#endif

/*
 * +persistent_undo	'undofile', 'undodir' options, :wundo and :rundo, and
 * implementation.
 */
#ifdef FEAT_NORMAL
# define FEAT_PERSISTENT_UNDO
#endif

/*
 * +filterpipe
 */
#if (defined(UNIX) && !defined(USE_SYSTEM)) \
	    || (defined(WIN3264) && defined(FEAT_GUI_W32))
# define FEAT_FILTERPIPE
#endif

/*
 * +vtp: Win32 virtual console.
 */
#if !defined(FEAT_GUI) && defined(WIN3264)
# define FEAT_VTP
#endif
