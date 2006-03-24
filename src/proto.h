/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * proto.h: include the (automatically generated) function prototypes
 */

/*
 * Don't include these while generating prototypes.  Prevents problems when
 * files are missing.
 */
#if !defined(PROTO) && !defined(NOPROTO)

/*
 * Machine-dependent routines.
 */
/* avoid errors in function prototypes */
# if !defined(FEAT_X11) && !defined(FEAT_GUI_GTK)
#  define Display int
#  define Widget int
# endif
# ifndef FEAT_GUI_GTK
#  define GdkEvent int
#  define GdkEventKey int
# endif
# ifndef FEAT_X11
#  define XImage int
# endif

# ifdef AMIGA
#  include "os_amiga.pro"
# endif
# if defined(UNIX) || defined(__EMX__) || defined(VMS)
#  include "os_unix.pro"
# endif
# if defined(MSDOS) || defined(WIN16)
#  include "os_msdos.pro"
# endif
# ifdef WIN16
   typedef LPSTR LPWSTR;
   typedef LPCSTR LPCWSTR;
   typedef int LPBOOL;
#  include "os_win16.pro"
#  include "os_mswin.pro"
# endif
# ifdef WIN3264
#  include "os_win32.pro"
#  include "os_mswin.pro"
#  if (defined(__GNUC__) && !defined(__MINGW32__)) \
	|| (defined(__BORLANDC__) && __BORLANDC__ < 0x502)
extern int _stricoll __ARGS((char *a, char *b));
#  endif
# endif
# ifdef VMS
#  include "os_vms.pro"
# endif
# ifdef __BEOS__
#  include "os_beos.pro"
# endif
# ifdef RISCOS
#  include "os_riscos.pro"
# endif
# ifdef __QNX__
#  include "os_qnx.pro"
# endif

# include "buffer.pro"
# include "charset.pro"
# ifdef FEAT_CSCOPE
#  include "if_cscope.pro"
# endif
# include "diff.pro"
# include "digraph.pro"
# include "edit.pro"
# include "eval.pro"
# include "ex_cmds.pro"
# include "ex_cmds2.pro"
# include "ex_docmd.pro"
# include "ex_eval.pro"
# include "ex_getln.pro"
# include "fileio.pro"
# include "fold.pro"
# include "getchar.pro"
# ifdef FEAT_HANGULIN
#  include "hangulin.pro"
# endif
# include "hardcopy.pro"
# include "hashtab.pro"
# include "main.pro"
# include "mark.pro"
# include "memfile.pro"
# include "memline.pro"
# ifdef FEAT_MENU
#  include "menu.pro"
# endif

# if !defined MESSAGE_FILE || defined(HAVE_STDARG_H)
    /* These prototypes cannot be produced automatically and conflict with
     * the old-style prototypes in message.c. */
int
#  ifdef __BORLANDC__
_RTLENTRYF
#  endif
smsg __ARGS((char_u *, ...));
int
#  ifdef __BORLANDC__
_RTLENTRYF
#  endif
smsg_attr __ARGS((int, char_u *, ...));
int
#  ifdef __BORLANDC__
_RTLENTRYF
#  endif
vim_snprintf __ARGS((char *, size_t, char *, ...));
#  if defined(HAVE_STDARG_H)
int vim_vsnprintf(char *str, size_t str_m, char *fmt, va_list ap, typval_T *tvs);
#  endif
# endif

# include "message.pro"
# include "misc1.pro"
# include "misc2.pro"
#ifndef HAVE_STRPBRK	    /* not generated automatically from misc2.c */
char_u *vim_strpbrk __ARGS((char_u *s, char_u *charset));
#endif
#ifndef HAVE_QSORT
/* Use our own qsort(), don't define the prototype when not used. */
void qsort __ARGS((void *base, size_t elm_count, size_t elm_size, int (*cmp)(const void *, const void *)));
#endif
# include "move.pro"
# if defined(FEAT_MBYTE) || defined(FEAT_XIM) || defined(FEAT_KEYMAP) \
	|| defined(FEAT_POSTSCRIPT)
#  include "mbyte.pro"
# endif
# include "normal.pro"
# include "ops.pro"
# include "option.pro"
# include "popupmnu.pro"
# ifdef FEAT_QUICKFIX
#  include "quickfix.pro"
# endif
# include "regexp.pro"
# include "screen.pro"
# include "search.pro"
# include "spell.pro"
# include "syntax.pro"
# include "tag.pro"
# include "term.pro"
# if defined(HAVE_TGETENT) && (defined(AMIGA) || defined(VMS))
#  include "termlib.pro"
# endif
# include "ui.pro"
# include "undo.pro"
# include "version.pro"
# include "window.pro"

# ifdef FEAT_MZSCHEME
#  include "if_mzsch.pro"
# endif

# ifdef FEAT_PYTHON
#  include "if_python.pro"
# endif

# ifdef FEAT_TCL
#  include "if_tcl.pro"
# endif

# ifdef FEAT_RUBY
#  include "if_ruby.pro"
# endif

# ifdef FEAT_GUI
#  include "gui.pro"
#  if defined(UNIX) || defined(MACOS)
#   include "pty.pro"
#  endif
#  if !defined(HAVE_SETENV) && !defined(HAVE_PUTENV) && !defined(VMS)
extern int putenv __ARGS((const char *string));		/* from pty.c */
#   ifdef USE_VIMPTY_GETENV
extern char_u *vimpty_getenv __ARGS((const char_u *string));	/* from pty.c */
#   endif
#  endif
#  ifdef FEAT_GUI_W16
#   include "gui_w16.pro"
#  endif
    /* Ugly solution for "BalloonEval" not being defined while it's used in
     * the prototypes. */
#  ifndef FEAT_BEVAL
#   define BalloonEval int
#  endif
#  ifdef FEAT_GUI_W32
#   include "gui_w32.pro"
#  endif
#  ifdef FEAT_GUI_GTK
#   include "gui_gtk.pro"
#   include "gui_gtk_x11.pro"
#  endif
#  ifdef FEAT_GUI_MOTIF
#   include "gui_motif.pro"
#   include "gui_xmdlg.pro"
#  endif
#  ifdef FEAT_GUI_ATHENA
#   include "gui_athena.pro"
#   ifdef FEAT_BROWSE
extern char *vim_SelFile __ARGS((Widget toplevel, char *prompt, char *init_path, int (*show_entry)(), int x, int y, guicolor_T fg, guicolor_T bg, guicolor_T scroll_fg, guicolor_T scroll_bg));
#   endif
#  endif
#  ifdef FEAT_GUI_MAC
#   include "gui_mac.pro"
#  endif
#  ifdef FEAT_GUI_X11
#   include "gui_x11.pro"
#  endif
#  ifdef RISCOS
#   include "gui_riscos.pro"
#  endif
#  ifdef FEAT_GUI_PHOTON
#   include "gui_photon.pro"
#  endif
#  ifdef FEAT_SUN_WORKSHOP
#   include "workshop.pro"
#  endif
#  ifdef FEAT_NETBEANS_INTG
#   include "netbeans.pro"
#  endif
# endif	/* FEAT_GUI */

# ifdef FEAT_OLE
#  include "if_ole.pro"
# endif
# if defined(FEAT_CLIENTSERVER) && defined(FEAT_X11)
#  include "if_xcmdsrv.pro"
# endif

/*
 * The perl include files pollute the namespace, therefore proto.h must be
 * included before the perl include files.  But then CV is not defined, which
 * is used in if_perl.pro.  To get around this, the perl prototype files are
 * not included here for the perl files.  Use a dummy define for CV for the
 * other files.
 */
#if defined(FEAT_PERL) && !defined(IN_PERL_FILE)
# define CV void
# ifdef __BORLANDC__
  #pragma option -pc
# endif
# include "if_perl.pro"
# ifdef __BORLANDC__
  #pragma option -p.
# endif
# include "if_perlsfio.pro"
#endif

#ifdef MACOS_CONVERT
# include "os_mac_conv.pro"
#endif

#ifdef __BORLANDC__
# define _PROTO_H
#endif
#endif /* !PROTO && !NOPROTO */
