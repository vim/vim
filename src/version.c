/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef AMIGA
# include <time.h>	/* for time() */
#endif

/*
 * Vim originated from Stevie version 3.6 (Fish disk 217) by GRWalter (Fred)
 * It has been changed beyond recognition since then.
 *
 * Differences between version 6.x and 7.x can be found with ":help version7".
 * Differences between version 5.x and 6.x can be found with ":help version6".
 * Differences between version 4.x and 5.x can be found with ":help version5".
 * Differences between version 3.0 and 4.x can be found with ":help version4".
 * All the remarks about older versions have been removed, they are not very
 * interesting.
 */

#include "version.h"

char		*Version = VIM_VERSION_SHORT;
static char	*mediumVersion = VIM_VERSION_MEDIUM;

#if defined(HAVE_DATE_TIME) || defined(PROTO)
# if (defined(VMS) && defined(VAXC)) || defined(PROTO)
char	longVersion[sizeof(VIM_VERSION_LONG_DATE) + sizeof(__DATE__)
						      + sizeof(__TIME__) + 3];
    void
make_version()
{
    /*
     * Construct the long version string.  Necessary because
     * VAX C can't catenate strings in the preprocessor.
     */
    strcpy(longVersion, VIM_VERSION_LONG_DATE);
    strcat(longVersion, __DATE__);
    strcat(longVersion, " ");
    strcat(longVersion, __TIME__);
    strcat(longVersion, ")");
}
# else
char	*longVersion = VIM_VERSION_LONG_DATE __DATE__ " " __TIME__ ")";
# endif
#else
char	*longVersion = VIM_VERSION_LONG;
#endif

static void version_msg __ARGS((char *s));

static char *(features[]) =
{
#ifdef AMIGA		/* only for Amiga systems */
# ifdef FEAT_ARP
	"+ARP",
# else
	"-ARP",
# endif
#endif
#ifdef FEAT_ARABIC
	"+arabic",
#else
	"-arabic",
#endif
#ifdef FEAT_AUTOCMD
	"+autocmd",
#else
	"-autocmd",
#endif
#ifdef FEAT_BEVAL
	"+balloon_eval",
#else
	"-balloon_eval",
#endif
#ifdef FEAT_BROWSE
	"+browse",
#else
	"-browse",
#endif
#ifdef NO_BUILTIN_TCAPS
	"-builtin_terms",
#endif
#ifdef SOME_BUILTIN_TCAPS
	"+builtin_terms",
#endif
#ifdef ALL_BUILTIN_TCAPS
	"++builtin_terms",
#endif
#ifdef FEAT_BYTEOFF
	"+byte_offset",
#else
	"-byte_offset",
#endif
#ifdef FEAT_CINDENT
	"+cindent",
#else
	"-cindent",
#endif
#ifdef FEAT_CLIENTSERVER
	"+clientserver",
#else
	"-clientserver",
#endif
#ifdef FEAT_CLIPBOARD
	"+clipboard",
#else
	"-clipboard",
#endif
#ifdef FEAT_CMDL_COMPL
	"+cmdline_compl",
#else
	"-cmdline_compl",
#endif
#ifdef FEAT_CMDHIST
	"+cmdline_hist",
#else
	"-cmdline_hist",
#endif
#ifdef FEAT_CMDL_INFO
	"+cmdline_info",
#else
	"-cmdline_info",
#endif
#ifdef FEAT_COMMENTS
	"+comments",
#else
	"-comments",
#endif
#ifdef FEAT_CRYPT
	"+cryptv",
#else
	"-cryptv",
#endif
#ifdef FEAT_CSCOPE
	"+cscope",
#else
	"-cscope",
#endif
#ifdef CURSOR_SHAPE
	"+cursorshape",
#else
	"-cursorshape",
#endif
#if defined(FEAT_CON_DIALOG) && defined(FEAT_GUI_DIALOG)
	"+dialog_con_gui",
#else
# if defined(FEAT_CON_DIALOG)
	"+dialog_con",
# else
#  if defined(FEAT_GUI_DIALOG)
	"+dialog_gui",
#  else
	"-dialog",
#  endif
# endif
#endif
#ifdef FEAT_DIFF
	"+diff",
#else
	"-diff",
#endif
#ifdef FEAT_DIGRAPHS
	"+digraphs",
#else
	"-digraphs",
#endif
#ifdef FEAT_DND
	"+dnd",
#else
	"-dnd",
#endif
#ifdef EBCDIC
	"+ebcdic",
#else
	"-ebcdic",
#endif
#ifdef FEAT_EMACS_TAGS
	"+emacs_tags",
#else
	"-emacs_tags",
#endif
#ifdef FEAT_EVAL
	"+eval",
#else
	"-eval",
#endif
#ifdef FEAT_EX_EXTRA
	"+ex_extra",
#else
	"-ex_extra",
#endif
#ifdef FEAT_SEARCH_EXTRA
	"+extra_search",
#else
	"-extra_search",
#endif
#ifdef FEAT_FKMAP
	"+farsi",
#else
	"-farsi",
#endif
#ifdef FEAT_SEARCHPATH
	"+file_in_path",
#else
	"-file_in_path",
#endif
#ifdef FEAT_FIND_ID
	"+find_in_path",
#else
	"-find_in_path",
#endif
#ifdef FEAT_FLOAT
	"+float",
#else
	"-float",
#endif
#ifdef FEAT_FOLDING
	"+folding",
#else
	"-folding",
#endif
#ifdef FEAT_FOOTER
	"+footer",
#else
	"-footer",
#endif
	    /* only interesting on Unix systems */
#if !defined(USE_SYSTEM) && defined(UNIX)
	"+fork()",
#endif
#ifdef FEAT_GETTEXT
# ifdef DYNAMIC_GETTEXT
	"+gettext/dyn",
# else
	"+gettext",
# endif
#else
	"-gettext",
#endif
#ifdef FEAT_HANGULIN
	"+hangul_input",
#else
	"-hangul_input",
#endif
#if (defined(HAVE_ICONV_H) && defined(USE_ICONV)) || defined(DYNAMIC_ICONV)
# ifdef DYNAMIC_ICONV
	"+iconv/dyn",
# else
	"+iconv",
# endif
#else
	"-iconv",
#endif
#ifdef FEAT_INS_EXPAND
	"+insert_expand",
#else
	"-insert_expand",
#endif
#ifdef FEAT_JUMPLIST
	"+jumplist",
#else
	"-jumplist",
#endif
#ifdef FEAT_KEYMAP
	"+keymap",
#else
	"-keymap",
#endif
#ifdef FEAT_LANGMAP
	"+langmap",
#else
	"-langmap",
#endif
#ifdef FEAT_LIBCALL
	"+libcall",
#else
	"-libcall",
#endif
#ifdef FEAT_LINEBREAK
	"+linebreak",
#else
	"-linebreak",
#endif
#ifdef FEAT_LISP
	"+lispindent",
#else
	"-lispindent",
#endif
#ifdef FEAT_LISTCMDS
	"+listcmds",
#else
	"-listcmds",
#endif
#ifdef FEAT_LOCALMAP
	"+localmap",
#else
	"-localmap",
#endif
#ifdef FEAT_MENU
	"+menu",
#else
	"-menu",
#endif
#ifdef FEAT_SESSION
	"+mksession",
#else
	"-mksession",
#endif
#ifdef FEAT_MODIFY_FNAME
	"+modify_fname",
#else
	"-modify_fname",
#endif
#ifdef FEAT_MOUSE
	"+mouse",
#  ifdef FEAT_MOUSESHAPE
	"+mouseshape",
#  else
	"-mouseshape",
#  endif
# else
	"-mouse",
#endif
#if defined(UNIX) || defined(VMS)
# ifdef FEAT_MOUSE_DEC
	"+mouse_dec",
# else
	"-mouse_dec",
# endif
# ifdef FEAT_MOUSE_GPM
	"+mouse_gpm",
# else
	"-mouse_gpm",
# endif
# ifdef FEAT_MOUSE_JSB
	"+mouse_jsbterm",
# else
	"-mouse_jsbterm",
# endif
# ifdef FEAT_MOUSE_NET
	"+mouse_netterm",
# else
	"-mouse_netterm",
# endif
# ifdef FEAT_SYSMOUSE
	"+mouse_sysmouse",
# else
	"-mouse_sysmouse",
# endif
# ifdef FEAT_MOUSE_XTERM
	"+mouse_xterm",
# else
	"-mouse_xterm",
# endif
#endif
#ifdef __QNX__
# ifdef FEAT_MOUSE_PTERM
	"+mouse_pterm",
# else
	"-mouse_pterm",
# endif
#endif
#ifdef FEAT_MBYTE_IME
# ifdef DYNAMIC_IME
	"+multi_byte_ime/dyn",
# else
	"+multi_byte_ime",
# endif
#else
# ifdef FEAT_MBYTE
	"+multi_byte",
# else
	"-multi_byte",
# endif
#endif
#ifdef FEAT_MULTI_LANG
	"+multi_lang",
#else
	"-multi_lang",
#endif
#ifdef FEAT_MZSCHEME
# ifdef DYNAMIC_MZSCHEME
	"+mzscheme/dyn",
# else
	"+mzscheme",
# endif
#else
	"-mzscheme",
#endif
#ifdef FEAT_NETBEANS_INTG
	"+netbeans_intg",
#else
	"-netbeans_intg",
#endif
#ifdef FEAT_GUI_W32
# ifdef FEAT_OLE
	"+ole",
# else
	"-ole",
# endif
#endif
#ifdef FEAT_OSFILETYPE
	"+osfiletype",
#else
	"-osfiletype",
#endif
#ifdef FEAT_PATH_EXTRA
	"+path_extra",
#else
	"-path_extra",
#endif
#ifdef FEAT_PERL
# ifdef DYNAMIC_PERL
	"+perl/dyn",
# else
	"+perl",
# endif
#else
	"-perl",
#endif
#ifdef FEAT_PRINTER
# ifdef FEAT_POSTSCRIPT
	"+postscript",
# else
	"-postscript",
# endif
	"+printer",
#else
	"-printer",
#endif
#ifdef FEAT_PROFILE
	"+profile",
#else
	"-profile",
#endif
#ifdef FEAT_PYTHON
# ifdef DYNAMIC_PYTHON
	"+python/dyn",
# else
	"+python",
# endif
#else
	"-python",
#endif
#ifdef FEAT_QUICKFIX
	"+quickfix",
#else
	"-quickfix",
#endif
#ifdef FEAT_RELTIME
	"+reltime",
#else
	"-reltime",
#endif
#ifdef FEAT_RIGHTLEFT
	"+rightleft",
#else
	"-rightleft",
#endif
#ifdef FEAT_RUBY
# ifdef DYNAMIC_RUBY
	"+ruby/dyn",
# else
	"+ruby",
# endif
#else
	"-ruby",
#endif
#ifdef FEAT_SCROLLBIND
	"+scrollbind",
#else
	"-scrollbind",
#endif
#ifdef FEAT_SIGNS
	"+signs",
#else
	"-signs",
#endif
#ifdef FEAT_SMARTINDENT
	"+smartindent",
#else
	"-smartindent",
#endif
#ifdef FEAT_SNIFF
	"+sniff",
#else
	"-sniff",
#endif
#ifdef FEAT_STL_OPT
	"+statusline",
#else
	"-statusline",
#endif
#ifdef FEAT_SUN_WORKSHOP
	"+sun_workshop",
#else
	"-sun_workshop",
#endif
#ifdef FEAT_SYN_HL
	"+syntax",
#else
	"-syntax",
#endif
	    /* only interesting on Unix systems */
#if defined(USE_SYSTEM) && (defined(UNIX) || defined(__EMX__))
	"+system()",
#endif
#ifdef FEAT_TAG_BINS
	"+tag_binary",
#else
	"-tag_binary",
#endif
#ifdef FEAT_TAG_OLDSTATIC
	"+tag_old_static",
#else
	"-tag_old_static",
#endif
#ifdef FEAT_TAG_ANYWHITE
	"+tag_any_white",
#else
	"-tag_any_white",
#endif
#ifdef FEAT_TCL
# ifdef DYNAMIC_TCL
	"+tcl/dyn",
# else
	"+tcl",
# endif
#else
	"-tcl",
#endif
#if defined(UNIX) || defined(__EMX__)
/* only Unix (or OS/2 with EMX!) can have terminfo instead of termcap */
# ifdef TERMINFO
	"+terminfo",
# else
	"-terminfo",
# endif
#else		    /* unix always includes termcap support */
# ifdef HAVE_TGETENT
	"+tgetent",
# else
	"-tgetent",
# endif
#endif
#ifdef FEAT_TERMRESPONSE
	"+termresponse",
#else
	"-termresponse",
#endif
#ifdef FEAT_TEXTOBJ
	"+textobjects",
#else
	"-textobjects",
#endif
#ifdef FEAT_TITLE
	"+title",
#else
	"-title",
#endif
#ifdef FEAT_TOOLBAR
	"+toolbar",
#else
	"-toolbar",
#endif
#ifdef FEAT_USR_CMDS
	"+user_commands",
#else
	"-user_commands",
#endif
#ifdef FEAT_VERTSPLIT
	"+vertsplit",
#else
	"-vertsplit",
#endif
#ifdef FEAT_VIRTUALEDIT
	"+virtualedit",
#else
	"-virtualedit",
#endif
#ifdef FEAT_VISUAL
	"+visual",
# ifdef FEAT_VISUALEXTRA
	"+visualextra",
# else
	"-visualextra",
# endif
#else
	"-visual",
#endif
#ifdef FEAT_VIMINFO
	"+viminfo",
#else
	"-viminfo",
#endif
#ifdef FEAT_VREPLACE
	"+vreplace",
#else
	"-vreplace",
#endif
#ifdef FEAT_WILDIGN
	"+wildignore",
#else
	"-wildignore",
#endif
#ifdef FEAT_WILDMENU
	"+wildmenu",
#else
	"-wildmenu",
#endif
#ifdef FEAT_WINDOWS
	"+windows",
#else
	"-windows",
#endif
#ifdef FEAT_WRITEBACKUP
	"+writebackup",
#else
	"-writebackup",
#endif
#if defined(UNIX) || defined(VMS)
# ifdef FEAT_X11
	"+X11",
# else
	"-X11",
# endif
#endif
#ifdef FEAT_XFONTSET
	"+xfontset",
#else
	"-xfontset",
#endif
#ifdef FEAT_XIM
	"+xim",
#else
	"-xim",
#endif
#if defined(UNIX) || defined(VMS)
# ifdef USE_XSMP_INTERACT
	"+xsmp_interact",
# else
#  ifdef USE_XSMP
	"+xsmp",
#  else
	"-xsmp",
#  endif
# endif
# ifdef FEAT_XCLIPBOARD
	"+xterm_clipboard",
# else
	"-xterm_clipboard",
# endif
#endif
#ifdef FEAT_XTERM_SAVE
	"+xterm_save",
#else
	"-xterm_save",
#endif
#ifdef WIN3264
# ifdef FEAT_XPM_W32
	"+xpm_w32",
# else
	"-xpm_w32",
# endif
#endif
	NULL
};

static int included_patches[] =
{   /* Add new patch number below this line */
/**/
    22,
/**/
    21,
/**/
    20,
/**/
    19,
/**/
    18,
/**/
    17,
/**/
    16,
/**/
    15,
/**/
    14,
/**/
    13,
/**/
    12,
/**/
    11,
/**/
    10,
/**/
    9,
/**/
    8,
/**/
    7,
/**/
    6,
/**/
    5,
/**/
    4,
/**/
    3,
/**/
    2,
/**/
    1,
/**/
    0
};

    int
highest_patch()
{
    int		i;
    int		h = 0;

    for (i = 0; included_patches[i] != 0; ++i)
	if (included_patches[i] > h)
	    h = included_patches[i];
    return h;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return TRUE if patch "n" has been included.
 */
    int
has_patch(n)
    int		n;
{
    int		i;

    for (i = 0; included_patches[i] != 0; ++i)
	if (included_patches[i] == n)
	    return TRUE;
    return FALSE;
}
#endif

    void
ex_version(eap)
    exarg_T	*eap;
{
    /*
     * Ignore a ":version 9.99" command.
     */
    if (*eap->arg == NUL)
    {
	msg_putchar('\n');
	list_version();
    }
}

    void
list_version()
{
    int		i;
    int		first;
    char	*s = "";

    /*
     * When adding features here, don't forget to update the list of
     * internal variables in eval.c!
     */
    MSG(longVersion);
#ifdef WIN3264
# ifdef FEAT_GUI_W32
#  if defined(_MSC_VER) && (_MSC_VER <= 1010)
    /* Only MS VC 4.1 and earlier can do Win32s */
    MSG_PUTS(_("\nMS-Windows 16/32-bit GUI version"));
#  else
#   ifdef _WIN64
    MSG_PUTS(_("\nMS-Windows 64-bit GUI version"));
#   else
    MSG_PUTS(_("\nMS-Windows 32-bit GUI version"));
#   endif
#  endif
    if (gui_is_win32s())
	MSG_PUTS(_(" in Win32s mode"));
# ifdef FEAT_OLE
    MSG_PUTS(_(" with OLE support"));
# endif
# else
#  ifdef _WIN64
    MSG_PUTS(_("\nMS-Windows 64-bit console version"));
#  else
    MSG_PUTS(_("\nMS-Windows 32-bit console version"));
#  endif
# endif
#endif
#ifdef WIN16
    MSG_PUTS(_("\nMS-Windows 16-bit version"));
#endif
#ifdef MSDOS
# ifdef DJGPP
    MSG_PUTS(_("\n32-bit MS-DOS version"));
# else
    MSG_PUTS(_("\n16-bit MS-DOS version"));
# endif
#endif
#ifdef MACOS
# ifdef MACOS_X
#  ifdef MACOS_X_UNIX
    MSG_PUTS(_("\nMacOS X (unix) version"));
#  else
    MSG_PUTS(_("\nMacOS X version"));
#  endif
#else
    MSG_PUTS(_("\nMacOS version"));
# endif
#endif

#ifdef RISCOS
    MSG_PUTS(_("\nRISC OS version"));
#endif
#ifdef VMS
    MSG_PUTS(_("\nOpenVMS version"));
# ifdef HAVE_PATHDEF
    if (*compiled_arch != NUL)
    {
	MSG_PUTS(" - ");
	MSG_PUTS(compiled_arch);
    }
# endif

#endif

    /* Print the list of patch numbers if there is at least one. */
    /* Print a range when patches are consecutive: "1-10, 12, 15-40, 42-45" */
    if (included_patches[0] != 0)
    {
	MSG_PUTS(_("\nIncluded patches: "));
	first = -1;
	/* find last one */
	for (i = 0; included_patches[i] != 0; ++i)
	    ;
	while (--i >= 0)
	{
	    if (first < 0)
		first = included_patches[i];
	    if (i == 0 || included_patches[i - 1] != included_patches[i] + 1)
	    {
		MSG_PUTS(s);
		s = ", ";
		msg_outnum((long)first);
		if (first != included_patches[i])
		{
		    MSG_PUTS("-");
		    msg_outnum((long)included_patches[i]);
		}
		first = -1;
	    }
	}
    }

#ifdef MODIFIED_BY
    MSG_PUTS("\n");
    MSG_PUTS(_("Modified by "));
    MSG_PUTS(MODIFIED_BY);
#endif

#ifdef HAVE_PATHDEF
    if (*compiled_user != NUL || *compiled_sys != NUL)
    {
	MSG_PUTS(_("\nCompiled "));
	if (*compiled_user != NUL)
	{
	    MSG_PUTS(_("by "));
	    MSG_PUTS(compiled_user);
	}
	if (*compiled_sys != NUL)
	{
	    MSG_PUTS("@");
	    MSG_PUTS(compiled_sys);
	}
    }
#endif

#ifdef FEAT_HUGE
    MSG_PUTS(_("\nHuge version "));
#else
# ifdef FEAT_BIG
    MSG_PUTS(_("\nBig version "));
# else
#  ifdef FEAT_NORMAL
    MSG_PUTS(_("\nNormal version "));
#  else
#   ifdef FEAT_SMALL
    MSG_PUTS(_("\nSmall version "));
#   else
    MSG_PUTS(_("\nTiny version "));
#   endif
#  endif
# endif
#endif
#ifndef FEAT_GUI
    MSG_PUTS(_("without GUI."));
#else
# ifdef FEAT_GUI_GTK
#  ifdef FEAT_GUI_GNOME
#   ifdef HAVE_GTK2
    MSG_PUTS(_("with GTK2-GNOME GUI."));
#   else
    MSG_PUTS(_("with GTK-GNOME GUI."));
#   endif
#  else
#   ifdef HAVE_GTK2
    MSG_PUTS(_("with GTK2 GUI."));
#   else
    MSG_PUTS(_("with GTK GUI."));
#   endif
#  endif
# else
#  ifdef FEAT_GUI_MOTIF
    MSG_PUTS(_("with X11-Motif GUI."));
#  else
#   ifdef FEAT_GUI_ATHENA
#    ifdef FEAT_GUI_NEXTAW
    MSG_PUTS(_("with X11-neXtaw GUI."));
#    else
    MSG_PUTS(_("with X11-Athena GUI."));
#    endif
#   else
#     ifdef FEAT_GUI_PHOTON
    MSG_PUTS(_("with Photon GUI."));
#     else
#      if defined(MSWIN)
    MSG_PUTS(_("with GUI."));
#      else
#	if defined (TARGET_API_MAC_CARBON) && TARGET_API_MAC_CARBON
    MSG_PUTS(_("with Carbon GUI."));
#	else
#	 if defined (TARGET_API_MAC_OSX) && TARGET_API_MAC_OSX
    MSG_PUTS(_("with Cocoa GUI."));
#	 else
#	  if defined (MACOS)
    MSG_PUTS(_("with (classic) GUI."));
#	  endif
#	 endif
#	endif
#      endif
#    endif
#   endif
#  endif
# endif
#endif
    version_msg(_("  Features included (+) or not (-):\n"));

    /* print all the features */
    for (i = 0; features[i] != NULL; ++i)
    {
	version_msg(features[i]);
	if (msg_col > 0)
	    version_msg(" ");
    }

    version_msg("\n");
#ifdef SYS_VIMRC_FILE
    version_msg(_("   system vimrc file: \""));
    version_msg(SYS_VIMRC_FILE);
    version_msg("\"\n");
#endif
#ifdef USR_VIMRC_FILE
    version_msg(_("     user vimrc file: \""));
    version_msg(USR_VIMRC_FILE);
    version_msg("\"\n");
#endif
#ifdef USR_VIMRC_FILE2
    version_msg(_(" 2nd user vimrc file: \""));
    version_msg(USR_VIMRC_FILE2);
    version_msg("\"\n");
#endif
#ifdef USR_VIMRC_FILE3
    version_msg(_(" 3rd user vimrc file: \""));
    version_msg(USR_VIMRC_FILE3);
    version_msg("\"\n");
#endif
#ifdef USR_EXRC_FILE
    version_msg(_("      user exrc file: \""));
    version_msg(USR_EXRC_FILE);
    version_msg("\"\n");
#endif
#ifdef USR_EXRC_FILE2
    version_msg(_("  2nd user exrc file: \""));
    version_msg(USR_EXRC_FILE2);
    version_msg("\"\n");
#endif
#ifdef FEAT_GUI
# ifdef SYS_GVIMRC_FILE
    version_msg(_("  system gvimrc file: \""));
    version_msg(SYS_GVIMRC_FILE);
    version_msg("\"\n");
# endif
    version_msg(_("    user gvimrc file: \""));
    version_msg(USR_GVIMRC_FILE);
    version_msg("\"\n");
# ifdef USR_GVIMRC_FILE2
    version_msg(_("2nd user gvimrc file: \""));
    version_msg(USR_GVIMRC_FILE2);
    version_msg("\"\n");
# endif
# ifdef USR_GVIMRC_FILE3
    version_msg(_("3rd user gvimrc file: \""));
    version_msg(USR_GVIMRC_FILE3);
    version_msg("\"\n");
# endif
#endif
#ifdef FEAT_GUI
# ifdef SYS_MENU_FILE
    version_msg(_("    system menu file: \""));
    version_msg(SYS_MENU_FILE);
    version_msg("\"\n");
# endif
#endif
#ifdef HAVE_PATHDEF
    if (*default_vim_dir != NUL)
    {
	version_msg(_("  fall-back for $VIM: \""));
	version_msg((char *)default_vim_dir);
	version_msg("\"\n");
    }
    if (*default_vimruntime_dir != NUL)
    {
	version_msg(_(" f-b for $VIMRUNTIME: \""));
	version_msg((char *)default_vimruntime_dir);
	version_msg("\"\n");
    }
    version_msg(_("Compilation: "));
    version_msg((char *)all_cflags);
    version_msg("\n");
#ifdef VMS
    if (*compiler_version != NUL)
    {
	version_msg(_("Compiler: "));
	version_msg((char *)compiler_version);
	version_msg("\n");
    }
#endif
    version_msg(_("Linking: "));
    version_msg((char *)all_lflags);
#endif
#ifdef DEBUG
    version_msg("\n");
    version_msg(_("  DEBUG BUILD"));
#endif
}

/*
 * Output a string for the version message.  If it's going to wrap, output a
 * newline, unless the message is too long to fit on the screen anyway.
 */
    static void
version_msg(s)
    char	*s;
{
    int		len = (int)STRLEN(s);

    if (!got_int && len < (int)Columns && msg_col + len >= (int)Columns
								&& *s != '\n')
	msg_putchar('\n');
    if (!got_int)
	MSG_PUTS(s);
}

static void do_intro_line __ARGS((int row, char_u *mesg, int add_version, int attr));

/*
 * Give an introductory message about Vim.
 * Only used when starting Vim on an empty file, without a file name.
 * Or with the ":intro" command (for Sven :-).
 */
    void
intro_message(colon)
    int		colon;		/* TRUE for ":intro" */
{
    int		i;
    int		row;
    int		blanklines;
    int		sponsor;
    char	*p;
    static char	*(lines[]) =
    {
	N_("VIM - Vi IMproved"),
	"",
	N_("version "),
	N_("by Bram Moolenaar et al."),
#ifdef MODIFIED_BY
	" ",
#endif
	N_("Vim is open source and freely distributable"),
	"",
	N_("Help poor children in Uganda!"),
	N_("type  :help iccf<Enter>       for information "),
	"",
	N_("type  :q<Enter>               to exit         "),
	N_("type  :help<Enter>  or  <F1>  for on-line help"),
	N_("type  :help version7<Enter>   for version info"),
	NULL,
	"",
	N_("Running in Vi compatible mode"),
	N_("type  :set nocp<Enter>        for Vim defaults"),
	N_("type  :help cp-default<Enter> for info on this"),
    };
#ifdef FEAT_GUI
    static char	*(gui_lines[]) =
    {
	NULL,
	NULL,
	NULL,
	NULL,
#ifdef MODIFIED_BY
	NULL,
#endif
	NULL,
	NULL,
	NULL,
	N_("menu  Help->Orphans           for information    "),
	NULL,
	N_("Running modeless, typed text is inserted"),
	N_("menu  Edit->Global Settings->Toggle Insert Mode  "),
	N_("                              for two modes      "),
	NULL,
	NULL,
	NULL,
	N_("menu  Edit->Global Settings->Toggle Vi Compatible"),
	N_("                              for Vim defaults   "),
    };
#endif

    /* blanklines = screen height - # message lines */
    blanklines = (int)Rows - ((sizeof(lines) / sizeof(char *)) - 1);
    if (!p_cp)
	blanklines += 4;  /* add 4 for not showing "Vi compatible" message */
#if defined(WIN3264) && !defined(FEAT_GUI_W32)
    if (mch_windows95())
	blanklines -= 3;  /* subtract 3 for showing "Windows 95" message */
#endif

#ifdef FEAT_WINDOWS
    /* Don't overwrite a statusline.  Depends on 'cmdheight'. */
    if (p_ls > 1)
	blanklines -= Rows - topframe->fr_height;
#endif
    if (blanklines < 0)
	blanklines = 0;

    /* Show the sponsor and register message one out of four times, the Uganda
     * message two out of four times. */
    sponsor = (int)time(NULL);
    sponsor = ((sponsor & 2) == 0) - ((sponsor & 4) == 0);

    /* start displaying the message lines after half of the blank lines */
    row = blanklines / 2;
    if ((row >= 2 && Columns >= 50) || colon)
    {
	for (i = 0; i < (int)(sizeof(lines) / sizeof(char *)); ++i)
	{
	    p = lines[i];
#ifdef FEAT_GUI
	    if (p_im && gui.in_use && gui_lines[i] != NULL)
		p = gui_lines[i];
#endif
	    if (p == NULL)
	    {
		if (!p_cp)
		    break;
		continue;
	    }
	    if (sponsor != 0)
	    {
		if (strstr(p, "children") != NULL)
		    p = sponsor < 0
			? N_("Sponsor Vim development!")
			: N_("Become a registered Vim user!");
		else if (strstr(p, "iccf") != NULL)
		    p = sponsor < 0
			? N_("type  :help sponsor<Enter>    for information ")
			: N_("type  :help register<Enter>   for information ");
		else if (strstr(p, "Orphans") != NULL)
		    p = N_("menu  Help->Sponsor/Register  for information    ");
	    }
	    if (*p != NUL)
		do_intro_line(row, (char_u *)_(p), i == 2, 0);
	    ++row;
	}
#if defined(WIN3264) && !defined(FEAT_GUI_W32)
	if (mch_windows95())
	{
	    do_intro_line(++row,
		    (char_u *)_("WARNING: Windows 95/98/ME detected"),
							FALSE, hl_attr(HLF_E));
	    do_intro_line(++row,
		(char_u *)_("type  :help windows95<Enter>  for info on this"),
								    FALSE, 0);
	}
#endif
    }

    /* Make the wait-return message appear just below the text. */
    if (colon)
	msg_row = row;
}

    static void
do_intro_line(row, mesg, add_version, attr)
    int		row;
    char_u	*mesg;
    int		add_version;
    int		attr;
{
    char_u	vers[20];
    int		col;
    char_u	*p;
    int		l;
    int		clen;
#ifdef MODIFIED_BY
# define MODBY_LEN 150
    char_u	modby[MODBY_LEN];

    if (*mesg == ' ')
    {
	vim_strncpy(modby, (char_u *)_("Modified by "), MODBY_LEN - 1);
	l = STRLEN(modby);
	vim_strncpy(modby + l, (char_u *)MODIFIED_BY, MODBY_LEN - l - 1);
	mesg = modby;
    }
#endif

    /* Center the message horizontally. */
    col = vim_strsize(mesg);
    if (add_version)
    {
	STRCPY(vers, mediumVersion);
	if (highest_patch())
	{
	    /* Check for 9.9x or 9.9xx, alpha/beta version */
	    if (isalpha((int)mediumVersion[3]))
	    {
		if (isalpha((int)mediumVersion[4]))
		    sprintf((char *)vers + 5, ".%d%s", highest_patch(),
							   mediumVersion + 5);
		else
		    sprintf((char *)vers + 4, ".%d%s", highest_patch(),
							   mediumVersion + 4);
	    }
	    else
		sprintf((char *)vers + 3, ".%d", highest_patch());
	}
	col += (int)STRLEN(vers);
    }
    col = (Columns - col) / 2;
    if (col < 0)
	col = 0;

    /* Split up in parts to highlight <> items differently. */
    for (p = mesg; *p != NUL; p += l)
    {
	clen = 0;
	for (l = 0; p[l] != NUL
			 && (l == 0 || (p[l] != '<' && p[l - 1] != '>')); ++l)
	{
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		clen += ptr2cells(p + l);
		l += (*mb_ptr2len)(p + l) - 1;
	    }
	    else
#endif
		clen += byte2cells(p[l]);
	}
	screen_puts_len(p, l, row, col, *p == '<' ? hl_attr(HLF_8) : attr);
	col += clen;
    }

    /* Add the version number to the version line. */
    if (add_version)
	screen_puts(vers, row, col, 0);
}

/*
 * ":intro": clear screen, display intro screen and wait for return.
 */
/*ARGSUSED*/
    void
ex_intro(eap)
    exarg_T	*eap;
{
    screenclear();
    intro_message(TRUE);
    wait_return(TRUE);
}
