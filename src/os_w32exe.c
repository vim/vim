/* vi:set ts=8 sts=4 sw=4:
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
 * Ron Aaron <ronaharon@yahoo.com> wrote this and  the DLL support code.
 */
#include "vim.h"

#ifdef __MINGW32__
# ifndef _cdecl
#  define _cdecl
# endif
#endif

/* cproto doesn't create a prototype for main() */
int _cdecl
#if defined(FEAT_GUI_W32)
VimMain
#else
    main
#endif
	__ARGS((int argc, char **argv));
static int (_cdecl *pmain)(int, char **);

#ifndef PROTO
#ifdef FEAT_GUI
#ifndef VIMDLL
void _cdecl SaveInst(HINSTANCE hInst);
#endif
static void (_cdecl *pSaveInst)(HINSTANCE);
#endif

/*ARGSUSED*/
    int WINAPI
WinMain(
    HINSTANCE	hInstance,
    HINSTANCE	hPrevInst,
    LPSTR	lpszCmdLine,
    int		nCmdShow)
{
    int		argc = 0;
    char	**argv;
    char	*tofree;
    char	prog[256];
#ifdef VIMDLL
    char	*p;
    HANDLE	hLib;
#endif

    /* Ron: added full path name so that the $VIM variable will get set to our
     * startup path (so the .vimrc file can be found w/o a VIM env. var.) */
    GetModuleFileName(NULL, prog, 255);

    argc = get_cmd_args(prog, (char *)lpszCmdLine, &argv, &tofree);
    if (argc == 0)
    {
	MessageBox(0, "Could not allocate memory for command line.",
							      "VIM Error", 0);
	return 0;
    }

#ifdef DYNAMIC_GETTEXT
    /* Initialize gettext library */
    dyn_libintl_init(NULL);
#endif

#ifdef VIMDLL
    // LoadLibrary - get name of dll to load in here:
    p = strrchr(prog, '\\');
    if (p != NULL)
    {
# ifdef DEBUG
	strcpy(p+1, "vim32d.dll");
# else
	strcpy(p+1, "vim32.dll");
# endif
    }
    hLib = LoadLibrary(prog);
    if (hLib == NULL)
    {
	MessageBox(0, _("Could not load vim32.dll!"), _("VIM Error"), 0);
	goto errout;
    }
    // fix up the function pointers
# ifdef FEAT_GUI
    pSaveInst = GetProcAddress(hLib, (LPCSTR)2);
# endif
    pmain = GetProcAddress(hLib, (LPCSTR)1);
    if (pmain == NULL)
    {
	MessageBox(0, _("Could not fix up function pointers to the DLL!"),
							    _("VIM Error"),0);
	goto errout;
    }
#else
# ifdef FEAT_GUI
    pSaveInst = SaveInst;
# endif
    pmain =
# if defined(FEAT_GUI_W32)
    //&& defined(__MINGW32__)
	VimMain
# else
	main
# endif
	;
#endif
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

#ifdef VIMDLL
    FreeLibrary(hLib);
errout:
#endif
    free(argv);
    free(tofree);
#ifdef FEAT_MBYTE
    free_cmd_argsW();
#endif

    return 0;
}
#endif
