/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * MSDOS Machine-dependent things.
 */

#include "os_dos.h"		/* common MS-DOS and Win32 stuff */

#define BINARY_FILE_IO
#define USE_EXE_NAME		/* use argv[0] for $VIM */
#define SYNC_DUP_CLOSE		/* sync() a file with dup() and close() */
#define USE_TERM_CONSOLE
#ifdef DJGPP
# include <fcntl.h>		/* defines _USE_LFN */
# define USE_LONG_FNAME _USE_LFN    /* decide at run time */
# define USE_FNAME_CASE
# define HAVE_PUTENV
# define HAVE_STDARG_H
#else
# define SHORT_FNAME		/* always 8.3 file name */
#endif
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_FCNTL_H
#define HAVE_STRCSPN
#define HAVE_STRICMP
#define HAVE_STRFTIME		/* guessed */
#define HAVE_STRNICMP
#define HAVE_MEMSET
#define HAVE_QSORT
#define HAVE_ST_MODE		/* have stat.st_mode */
#if defined(__DATE__) && defined(__TIME__)
# define HAVE_DATE_TIME
#endif
#define BREAKCHECK_SKIP	    1	/* call mch_breakcheck() each time, it's fast */
#define HAVE_AVAIL_MEM

/*
 * Borland C++ 3.1 doesn't have _RTLENTRYF
 */
#ifdef __BORLANDC__
# if __BORLANDC__ < 0x450
#  define _RTLENTRYF
# endif
#endif

#define FNAME_ILLEGAL "\"*?><|" /* illegal characters in a file name */

#include <dos.h>
#include <dir.h>
#include <time.h>

#ifdef DJGPP
# include <unistd.h>
# define HAVE_LOCALE_H
# define setlocale(c, p)    djgpp_setlocale()
#endif

#ifndef DJGPP
typedef long off_t;
#endif

/*
 * Try several directories to put the temp files.
 */
#define TEMPDIRNAMES	"$TMP", "$TEMP", "c:\\TMP", "c:\\TEMP", ""
#define TEMPNAMELEN	128

#ifndef DFLT_MAXMEM
# define DFLT_MAXMEM	256		/* use up to 256Kbyte for buffer */
#endif
#ifndef DFLT_MAXMEMTOT
# define DFLT_MAXMEMTOT	0		/* decide in set_init */
#endif

#ifdef DJGPP
# define BASENAMELEN  (_USE_LFN?250:8)	/* length of base of file name */
#else
# define BASENAMELEN	    8		/* length of base of file name */
#endif

/* codes for msdos mouse event */
#define MSDOS_MOUSE_LEFT	0x01
#define MSDOS_MOUSE_RIGHT	0x02
#define MSDOS_MOUSE_MIDDLE	0x04

#ifdef DJGPP
int mch_rename(const char *OldFile, const char *NewFile);
#else
# define mch_rename(src, dst) rename(src, dst)
#endif

#ifdef DJGPP
# define vim_mkdir(x, y) mkdir((char *)(x), y)
#else
# define vim_mkdir(x, y) mkdir((char *)(x))
#endif
#define mch_rmdir(x) rmdir((char *)(x))

#define mch_setenv(name, val, x) setenv(name, val, x)
