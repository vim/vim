/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/* Before Including the MacOS specific files,
 * lets set the OPAQUE_TOOLBOX_STRUCTS to 0 so we
 * can access the internal structures.
 * (Until fully Carbon compliant)
 * TODO: Can we remove this? (Dany)
 */
#if 0
# define OPAQUE_TOOLBOX_STRUCTS 0
#endif

/*
 * Macintosh machine-dependent things.
 *
 * Include the Mac header files, unless also compiling with X11 (the header
 * files have many conflicts).
 */
#ifndef FEAT_X11
# include <QuickDraw.h>
# include <ToolUtils.h>
# include <LowMem.h>
# include <Scrap.h>
# include <Sound.h>
# include <TextUtils.h>
# include <Memory.h>
# include <OSUtils.h>
# include <Files.h>
# ifdef FEAT_MBYTE
#  include <Script.h>
# endif
#endif

/*
 * Unix interface
 */
#if defined(__MWERKS__) /* for CodeWarrior */
# include <unistd.h>
# include <utsname.h>
# include <unix.h>
#endif
#if defined(__APPLE_CC__) /* for Project Builder and ... */
# include <unistd.h>
#endif
/* Get stat.h or something similar. Comment: How come some OS get in in vim.h */
#if defined(__MWERKS__)
# include <stat.h>
#endif
#if defined(__APPLE_CC__)
# include <sys/stat.h>
#endif
#if defined(__MRC__) || defined(__SC__) /* for Apple MPW Compilers */
/* There's no stat.h for MPW? */
# ifdef powerc
#  pragma options align=power
# endif
  struct stat
  {
    UInt32 st_mtime;
    UInt32 st_mode;
    UInt32 st_size;
  };
# ifdef powerc
#  pragma options align=reset
# endif
#endif
#if defined(__APPLE_CC__) /* && defined(HAVE_CURSE) */
/* The curses.h from MacOS X provides by default some BACKWARD compatibilty
 * definition which can cause us problem later on. So we undefine a few of them. */
# include <curses.h>
# undef reg
# undef ospeed
/* OK defined to 0 in MacOS X 10.2 curses!  Remove it, we define it to be 1. */
# undef OK
#endif
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef MACOS_X
# include <dirent.h>
#endif

/*
 * Incompatibility checks
 */

/* Got problem trying to use shared library in 68k */
#if !defined(__POWERPC__) && !defined(__i386__) && defined(FEAT_PYTHON)
# undef FEAT_PYTHON
# warning Auto-disabling Python. Not yet supported in 68k.
#endif

#if !defined(__POWERPC__) && !defined(__ppc__) && !defined(__i386__)
# if !__option(enumsalwaysint)
#  error "You must compile with enums always int!"
# endif
# if defined(__MWERKS__) && !defined(__fourbyteints__)
#  error "You must compile the project with 4-byte ints"
/* MPW ints are always 4 byte long */
# endif
#endif

/*
 * MacOS specific #define
 */

/* This will go away when CMD_KEY fully tested */
#define USE_CMD_KEY
/* On MacOS X use the / not the : */
/* TODO: Should file such as ~/.vimrc reside instead in
 *       ~/Library/Vim or ~/Library/Preferences/org.vim.vim/ ? (Dany)
 */
/* When compiled under MacOS X (including CARBON version)
 * we use the Unix File path style.  Also when UNIX is defined. */
#if defined(UNIX) || (defined(TARGET_API_MAC_OSX) && TARGET_API_MAC_OSX)
# undef COLON_AS_PATHSEP
# define USE_UNIXFILENAME
#else
# define COLON_AS_PATHSEP
# define DONT_ADD_PATHSEP_TO_DIR
#endif


/*
 * Generic Vim #define
 */

#define FEAT_SOURCE_FFS
#define FEAT_SOURCE_FF_MAC

#define USE_EXE_NAME		    /* to find  $VIM */
#define CASE_INSENSITIVE_FILENAME   /* ignore case when comparing file names */
#define SPACE_IN_FILENAME
#define BREAKCHECK_SKIP	   32	    /* call mch_breakcheck() each time, it's
				       quite fast. Did I forgot to update the
				       comment */


#define USE_FNAME_CASE		/* make ":e os_Mac.c" open the file in its
				   original case, as "os_mac.c" */
#define BINARY_FILE_IO
#define EOL_DEFAULT EOL_MAC
#ifndef MACOS_X_UNIX		/* I hope that switching these two lines */
# define USE_CR			/* does what I want -- BNF */
# define NO_CONSOLE		/* don't include console mode */
#endif
#define HAVE_AVAIL_MEM

#ifndef HAVE_CONFIG_H
/* #define SYNC_DUP_CLOSE	   sync() a file with dup() and close() */
# define HAVE_STRING_H
# define HAVE_STRCSPN
# define HAVE_MEMSET
# define USE_TMPNAM		/* use tmpnam() instead of mktemp() */
# define HAVE_FCNTL_H
# define HAVE_QSORT
# define HAVE_ST_MODE		/* have stat.st_mode */

# if defined(__DATE__) && defined(__TIME__)
#  define HAVE_DATE_TIME
# endif
# define HAVE_STRFTIME
#endif

/*
 * Names for the EXRC, HELP and temporary files.
 * Some of these may have been defined in the makefile.
 */

#ifndef SYS_VIMRC_FILE
# ifdef COLON_AS_PATHSEP
#  define SYS_VIMRC_FILE "$VIM:vimrc"
# else
#  define SYS_VIMRC_FILE "$VIM/vimrc"
# endif
#endif
#ifndef SYS_GVIMRC_FILE
# ifdef COLON_AS_PATHSEP
#  define SYS_GVIMRC_FILE "$VIM:gvimrc"
# else
#  define SYS_GVIMRC_FILE "$VIM/gvimrc"
# endif
#endif
#ifndef SYS_MENU_FILE
# ifdef COLON_AS_PATHSEP
#  define SYS_MENU_FILE	"$VIMRUNTIME:menu.vim"
# else
#  define SYS_MENU_FILE	"$VIMRUNTIME/menu.vim"
# endif
#endif
#ifndef SYS_OPTWIN_FILE
# ifdef COLON_AS_PATHSEP
#  define SYS_OPTWIN_FILE "$VIMRUNTIME:optwin.vim"
# else
#  define SYS_OPTWIN_FILE "$VIMRUNTIME/optwin.vim"
# endif
#endif
#ifndef EVIM_FILE
# ifdef COLON_AS_PATHSEP
#  define EVIM_FILE	"$VIMRUNTIME:evim.vim"
# else
#  define EVIM_FILE	"$VIMRUNTIME/evim.vim"
# endif
#endif

#ifdef FEAT_GUI
# ifndef USR_GVIMRC_FILE
#  ifdef COLON_AS_PATHSEP
#   define USR_GVIMRC_FILE "$VIM:.gvimrc"
#  else
#   define USR_GVIMRC_FILE "~/.gvimrc"
#  endif
# endif
# ifndef GVIMRC_FILE
#  define GVIMRC_FILE	"_gvimrc"
# endif
#endif
#ifndef USR_VIMRC_FILE
# ifdef COLON_AS_PATHSEP
#  define USR_VIMRC_FILE	"$VIM:.vimrc"
# else
#  define USR_VIMRC_FILE	"~/.vimrc"
# endif
#endif

#ifndef USR_EXRC_FILE
# ifdef COLON_AS_PATHSEP
#  define USR_EXRC_FILE	"$VIM:.exrc"
# else
#  define USR_EXRC_FILE	"~/.exrc"
# endif
#endif

#ifndef VIMRC_FILE
# define VIMRC_FILE	"_vimrc"
#endif

#ifndef EXRC_FILE
# define EXRC_FILE	"_exrc"
#endif

#ifndef DFLT_HELPFILE
# ifdef COLON_AS_PATHSEP
#  define DFLT_HELPFILE	"$VIMRUNTIME:doc:help.txt"
# else
#  define DFLT_HELPFILE	"$VIMRUNTIME/doc/help.txt"
# endif
#endif

#ifndef FILETYPE_FILE
# define FILETYPE_FILE	"filetype.vim"
#endif
#ifndef FTPLUGIN_FILE
# define FTPLUGIN_FILE	"ftplugin.vim"
#endif
#ifndef INDENT_FILE
# define INDENT_FILE	"indent.vim"
#endif
#ifndef FTOFF_FILE
# define FTOFF_FILE	"ftoff.vim"
#endif
#ifndef FTPLUGOF_FILE
# define FTPLUGOF_FILE	"ftplugof.vim"
#endif
#ifndef INDOFF_FILE
# define INDOFF_FILE	"indoff.vim"
#endif

#ifndef SYNTAX_FNAME
# ifdef COLON_AS_PATHSEP
#  define SYNTAX_FNAME	"$VIMRUNTIME:syntax:%s.vim"
# else
#  define SYNTAX_FNAME	"$VIMRUNTIME/syntax/%s.vim"
# endif
#endif

#ifdef FEAT_VIMINFO
# ifndef VIMINFO_FILE
#  ifdef COLON_AS_PATHSEP
#   define VIMINFO_FILE	"$VIM:viminfo"
#  else
#   define VIMINFO_FILE	"~/.viminfo"
#  endif
# endif
#endif /* FEAT_VIMINFO */

#ifndef DFLT_BDIR
# define DFLT_BDIR	"."	/* default for 'backupdir' */
#endif

#ifndef DFLT_DIR
# define DFLT_DIR	"."	/* default for 'directory' */
#endif

#ifndef DFLT_VDIR
# ifdef COLON_AS_PATHSEP
#  define DFLT_VDIR	"$VIM:vimfiles:view"	/* default for 'viewdir' */
# else
#  define DFLT_VDIR	"$VIM/vimfiles/view"	/* default for 'viewdir' */
# endif
#endif

#define DFLT_ERRORFILE		"errors.err"

#ifndef DFLT_RUNTIMEPATH
# ifdef COLON_AS_PATHSEP
#  define DFLT_RUNTIMEPATH	"$VIM:vimfiles,$VIMRUNTIME,$VIM:vimfiles:after"
# else
#  define DFLT_RUNTIMEPATH	"~/.vim,$VIM/vimfiles,$VIMRUNTIME,$VIM/vimfiles/after,~/.vim/after"
# endif
#endif

/*
 * Macintosh has plenty of memory, use large buffers
 */
#define CMDBUFFSIZE 1024	/* size of the command processing buffer */

#if defined(MACOS_X_UNIX)
# define MAXPATHL	1024
# define BASENAMELEN	(MAXNAMLEN - 5)	/* length of base of filename */
#else
# define MAXPATHL	256		/* Limited by the Pascal Strings */
# define BASENAMELEN	(32-5-1)	/* length of base of filename */
#endif

#ifndef DFLT_MAXMEM
# define DFLT_MAXMEM	512	/* use up to  512 Kbyte for buffer */
#endif

#ifndef DFLT_MAXMEMTOT
# define DFLT_MAXMEMTOT	2048	/* use up to 2048 Kbyte for Vim */
#endif

#define WILDCHAR_LIST "*?[{`$"

/**************/
#define mch_rename(src, dst) rename(src, dst)
#define mch_remove(x) unlink((char *)(x))
#ifndef mch_getenv
# if defined(__MRC__) || defined(__SC__)
#  define mch_getenv(name)  ((char_u *)getenv((char *)(name)))
#  define mch_setenv(name, val, x) setenv((name), (val))
# elif defined(__APPLE_CC__)
#  define mch_getenv(name)  ((char_u *)getenv((char *)(name)))
/*# define mch_setenv(name, val, x) setenv((name), (val)) */ /* Obsoleted by Dany on Oct 30, 2001 */
#  define mch_setenv(name, val, x) setenv(name, val, x)
# else
  /* vim_getenv() is in pty.c */
#  define USE_VIMPTY_GETENV
#  define mch_getenv(x) vimpty_getenv(x)
#  define mch_setenv(name, val, x) setenv(name, val, x)
# endif
#endif

#ifndef HAVE_CONFIG_H
# ifdef __APPLE_CC__
/* Assuming compiling for MacOS X */
/* Trying to take advantage of the prebinding */
#  define HAVE_TGETENT
#  define OSPEED_EXTERN
#  define UP_BC_PC_EXTERN
# endif
#endif

/* Some "prep work" definition to be able to compile the MacOS X
 * version with os_unix.x instead of os_mac.c. Based on the result
 * of ./configure for console MacOS X.
 */

#ifdef MACOS_X_UNIX
# define SIGPROTOARG	(int)
# define SIGDEFARG(s)	(s) int s;
# define SIGDUMMYARG	0
# undef  HAVE_AVAIL_MEM
# ifndef HAVE_CONFIG_H
#  define RETSIGTYPE void
#  define SIGRETURN  return
/*# define USE_SYSTEM */  /* Output ship do debugger :(, but ot compile */
#  define HAVE_SYS_WAIT_H 1 /* Attempt */
#  define HAVE_TERMIOS_H 1
#  define SYS_SELECT_WITH_SYS_TIME 1
#  define HAVE_SELECT 1
#  define HAVE_SYS_SELECT_H 1
#  define HAVE_PUTENV
#  define HAVE_SETENV
#  define HAVE_RENAME
# endif
# define mch_chdir(s) chdir(s)
#endif

#if defined(MACOS_X) && !defined(HAVE_CONFIG_H)
# define HAVE_PUTENV
#endif

/* A Mac constant causing big problem to syntax highlighting */
#define UNKNOWN_CREATOR '\?\?\?\?'

/*
 * for debugging
 */
#ifdef MACOS_X
# ifdef _DEBUG
#  define TRACE			Trace
   void Trace(char *fmt, ...);
# else
#  define TRACE			1 ? (void)0 : printf
# endif
#endif

#ifdef MACOS_CLASSIC
#  define TRACE			1 ? (int)0 : printf
#endif
