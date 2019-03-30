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
 * Ron Aaron <ronaharon@yahoo.com> wrote this and the DLL support code.
 */
#include "vim.h"

#ifdef __MINGW32__
# ifndef _cdecl
#  define _cdecl
# endif
#endif

/* cproto doesn't create a prototype for main() */
int _cdecl
#if defined(FEAT_GUI_MSWIN)
VimMain
#else
    main
#endif
	(int argc, char **argv);
static int (_cdecl *pmain)(int, char **);

#ifndef PROTO
#ifdef FEAT_GUI
void _cdecl SaveInst(HINSTANCE hInst);
static void (_cdecl *pSaveInst)(HINSTANCE);
#endif

    int WINAPI
WinMain(
    HINSTANCE	hInstance UNUSED,
    HINSTANCE	hPrevInst UNUSED,
    LPSTR	lpszCmdLine UNUSED,
    int		nCmdShow UNUSED)
{
    int		argc = 0;
    char	**argv = NULL;
#ifdef FEAT_GUI
    pSaveInst = SaveInst;
#endif
    pmain =
#if defined(FEAT_GUI_MSWIN)
    //&& defined(__MINGW32__)
	VimMain
#else
	main
#endif
	;
#ifdef FEAT_GUI
    pSaveInst(
#ifdef __MINGW32__
	    GetModuleHandle(NULL)
#else
	    hInstance
#endif
	    );
#endif
    pmain(argc, argv);

    free_cmd_argsW();

    return 0;
}
#endif
