/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			this file by Vince Negri
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vimrun.c - Tiny Win32 program to safely run an external command in a
 *	      DOS console.
 *	      This program is required to avoid that typing CTRL-C in the DOS
 *	      console kills Vim.  Now it only kills vimrun.
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef __CYGWIN__
# include <conio.h>
#endif

#ifdef __BORLANDC__
extern char *
#ifdef _RTLDLL
__import
#endif
_oscmd;
# define _kbhit kbhit
# define _getch getch
#else
# ifdef __MINGW32__
#  ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
# else
#  ifdef __CYGWIN__
#   ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   define _getch getchar
#  else
extern char *_acmdln;
#  endif
# endif
#endif

    int
main(void)
{
    const char	*p;
    int		retval;
    int		inquote = 0;
    int		silent = 0;

#ifdef __BORLANDC__
    p = _oscmd;
#else
# if defined(__MINGW32__) || defined(__CYGWIN__)
    p = (const char *)GetCommandLine();
# else
    p = _acmdln;
# endif
#endif
    /*
     * Skip the executable name, which might be in "".
     */
    while (*p)
    {
	if (*p == '"')
	    inquote = !inquote;
	else if (!inquote && *p == ' ')
	{
	    ++p;
	    break;
	}
	++p;
    }

    /*
     * "-s" argument: don't wait for a key hit.
     */
    if (p[0] == '-' && p[1] == 's' && p[2] == ' ')
    {
	silent = 1;
	p += 3;
	while (*p == ' ')
	    ++p;
    }

    /* Print the command, including quotes and redirection. */
    puts(p);

    /*
     * Do it!
     */
    retval = system(p);

    if (retval == -1)
	perror("vimrun system(): ");
    else if (retval != 0)
	printf("shell returned %d\n", retval);

    if (!silent)
    {
	puts("Hit any key to close this window...");

#ifndef __CYGWIN__
	while (_kbhit())
	    (void)_getch();
#endif
	(void)_getch();
    }

    return retval;
}
