/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Win32 (Windows NT and Windows 95) machine-dependent things.
 */

#include "os_dos.h"		/* common MS-DOS and Win32 stuff */
#ifndef __CYGWIN__
/* cproto fails on missing include files */
# ifndef PROTO
#  include <direct.h>		/* for _mkdir() */
# endif
#endif

/* Stop the VC2005 compiler from nagging. */
#if _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#define BINARY_FILE_IO
#define USE_EXE_NAME		/* use argv[0] for $VIM */
#define SYNC_DUP_CLOSE		/* sync() a file with dup() and close() */
#define USE_TERM_CONSOLE
#ifndef HAVE_STRING_H
# define HAVE_STRING_H
#endif
#ifndef HAVE_MATH_H
# define HAVE_MATH_H
#endif
#define HAVE_STRCSPN
#ifndef __GNUC__
#define HAVE_STRICMP
#define HAVE_STRNICMP
#endif
#ifndef HAVE_STRFTIME
# define HAVE_STRFTIME		/* guessed */
#endif
#define HAVE_MEMSET
#ifndef HAVE_LOCALE_H
# define HAVE_LOCALE_H 1
#endif
#ifndef HAVE_FCNTL_H
# define HAVE_FCNTL_H
#endif
#define HAVE_QSORT
#define HAVE_ST_MODE		/* have stat.st_mode */

#define FEAT_SHORTCUT		/* resolve shortcuts */

#if (!defined(__BORLANDC__) || __BORLANDC__ >= 0x550) \
	&& (!defined(_MSC_VER) || _MSC_VER > 1020)
/*
 * Access Control List (actually security info).
 * Borland has the acl stuff only in version 5.5 and later.
 * MSVC in 5.0, not in 4.2, don't know about 4.3.
 */
# define HAVE_ACL
#endif

#define USE_FNAME_CASE		/* adjust case of file names */
#if !defined(FEAT_CLIPBOARD) && defined(FEAT_MOUSE)
# define FEAT_CLIPBOARD		/* include clipboard support */
#endif
#if defined(__DATE__) && defined(__TIME__)
# define HAVE_DATE_TIME
#endif
#ifndef FEAT_GUI_W32		/* GUI works different */
# define BREAKCHECK_SKIP    1	/* call mch_breakcheck() each time, it's fast */
#endif

#define HAVE_TOTAL_MEM

#define HAVE_PUTENV		/* at least Bcc 5.2 and MSC have it */

#ifdef FEAT_GUI_W32
# define NO_CONSOLE		/* don't included console-only code */
#endif

/* toupper() is not really broken, but it's very slow.	Probably because of
 * using Unicode characters on Windows NT */
#define BROKEN_TOUPPER

#define FNAME_ILLEGAL "\"*?><|" /* illegal characters in a file name */

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#ifndef STRICT
# define STRICT
#endif
#ifndef COBJMACROS
# define COBJMACROS	/* For OLE: Enable "friendlier" access to objects */
#endif
#ifndef PROTO
# include <windows.h>
# ifndef SM_CXPADDEDBORDER
#  define SM_CXPADDEDBORDER     92
# endif
#endif

/*
 * Win32 has plenty of memory, use large buffers
 */
#define CMDBUFFSIZE 1024	/* size of the command processing buffer */

/* _MAX_PATH is only 260 (stdlib.h), but we want more for the 'path' option,
 * thus use a larger number. */
#define MAXPATHL	1024

#ifndef BASENAMELEN
# define BASENAMELEN	(_MAX_PATH - 5)	/* length of base of file name */
#endif

#define TEMPNAMELEN	_MAX_PATH	/* length of temp file name path */

#ifndef DFLT_MAXMEM
# define DFLT_MAXMEM	(2*1024)    /* use up to 2 Mbyte for a buffer */
#endif

#ifndef DFLT_MAXMEMTOT
# define DFLT_MAXMEMTOT	(5*1024)    /* use up to 5 Mbyte for Vim */
#endif

/*
 * Reparse Point
 */
#ifndef FILE_ATTRIBUTE_REPARSE_POINT
# define FILE_ATTRIBUTE_REPARSE_POINT	0x00000400
#endif
#ifndef IO_REPARSE_TAG_MOUNT_POINT
# define IO_REPARSE_TAG_MOUNT_POINT	0xA0000003
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
# define IO_REPARSE_TAG_SYMLINK		0xA000000C
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
    /* Support for __try / __except.  All versions of MSVC and Borland C are
     * expected to have this.  Any other compilers that support it? */
# define HAVE_TRY_EXCEPT 1
# include <malloc.h>		/* for _resetstkoflw() */
# if defined(_MSC_VER) && (_MSC_VER >= 1300)
#  define RESETSTKOFLW _resetstkoflw
# else
#  define RESETSTKOFLW myresetstkoflw
#  define MYRESETSTKOFLW
# endif
#endif

/*
 * Some simple debugging macros that look and behave a lot like their
 * namesakes in MFC.
 */

#ifdef _DEBUG

# if defined(_MSC_VER)	&&  (_MSC_VER >= 1000)
   /* Use the new debugging tools in Visual C++ 4.x */
#  include <crtdbg.h>
#  define ASSERT(f) _ASSERT(f)
# else
#  include <assert.h>
#  define ASSERT(f) assert(f)
# endif

# define TRACE			Trace
# define TRACE0(sz)		Trace(_T("%s"), _T(sz))
# define TRACE1(sz, p1)		Trace(_T(sz), p1)
# define TRACE2(sz, p1, p2)	Trace(_T(sz), p1, p2)
# define TRACE3(sz, p1, p2, p3) Trace(_T(sz), p1, p2, p3)
# define TRACE4(sz, p1, p2, p3, p4) Trace(_T(sz), p1, p2, p3, p4)

/* In debug version, writes trace messages to debug stream */
void __cdecl
Trace(char *pszFormat, ...);

#else /* !_DEBUG */

  /* These macros should all compile away to nothing */
# define ASSERT(f)		((void)0)
# define TRACE			1 ? (void)0 : printf
# define TRACE0(sz)
# define TRACE1(sz, p1)
# define TRACE2(sz, p1, p2)
# define TRACE3(sz, p1, p2, p3)
# define TRACE4(sz, p1, p2, p3, p4)

#endif /* !_DEBUG */


#define ASSERT_POINTER(p, type) \
    ASSERT(((p) != NULL)  &&  IsValidAddress((p), sizeof(type), FALSE))

#define ASSERT_NULL_OR_POINTER(p, type) \
    ASSERT(((p) == NULL)  ||  IsValidAddress((p), sizeof(type), FALSE))

#ifndef HAVE_SETENV
# define HAVE_SETENV
#endif
#define mch_getenv(x) (char_u *)getenv((char *)(x))
#ifdef __BORLANDC__
# define vim_mkdir(x, y) mkdir(x)
#else
# define vim_mkdir(x, y) mch_mkdir(x)
#endif

/* Enable common dialogs input unicode from IME if possible. */
#ifdef FEAT_MBYTE
# define pDispatchMessage DispatchMessageW
# define pGetMessage GetMessageW
# define pIsDialogMessage IsDialogMessageW
# define pPeekMessage PeekMessageW
#else
# define pDispatchMessage DispatchMessage
# define pGetMessage GetMessage
# define pIsDialogMessage IsDialogMessage
# define pPeekMessage PeekMessage
#endif
