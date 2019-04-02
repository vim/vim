/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI support by Robert Webb
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * Windows GUI: main program (EXE) entry point:
 *
 * Ron Aaron <ronaharon@yahoo.com> wrote this and the (now deleted) DLL support
 * code.
 */
#include "vim.h"

#ifdef __MINGW32__
# ifndef _cdecl
#  define _cdecl
# endif
#endif

// cproto doesn't create a prototype for VimMain()
int _cdecl VimMain(int argc, char **argv);
#ifdef FEAT_GUI
void _cdecl SaveInst(HINSTANCE hInst);
#endif

#ifndef PROTO
    int WINAPI
WinMain(
    HINSTANCE	hInstance,
    HINSTANCE	hPrevInst UNUSED,
    LPSTR	lpszCmdLine UNUSED,
    int		nCmdShow UNUSED)
{
    int		argc = 0;
    char	**argv = NULL;

# ifdef FEAT_GUI
    SaveInst(hInstance);
# endif
    VimMain(argc, argv);

    return 0;
}
#endif
