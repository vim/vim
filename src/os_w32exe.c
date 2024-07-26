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
 * Windows GUI/Console: main program (EXE) entry point:
 *
 * Ron Aaron <ronaharon@yahoo.com> wrote this and the DLL support code.
 * Adapted by Ken Takata.
 */
#include "vim.h"

// cproto doesn't create a prototype for VimMain()
#ifdef VIMDLL
__declspec(dllimport)
#endif
int VimMain(int argc, char **argv);

#ifdef VIMDLL
# define SaveInst(hInst)    // Do nothing
#else
void SaveInst(HINSTANCE hInst);
#endif

#ifdef FEAT_GUI
    int WINAPI
wWinMain(
    HINSTANCE	hInstance,
    HINSTANCE	hPrevInst UNUSED,
    LPWSTR	lpszCmdLine UNUSED,
    int		nCmdShow UNUSED)
{
    SaveInst(hInstance);
    return VimMain(0, NULL);
}
#else
    int
wmain(int argc UNUSED, wchar_t **argv UNUSED)
{
    SaveInst(GetModuleHandleW(NULL));
    return VimMain(0, NULL);
}
#endif

#ifdef USE_OWNSTARTUP
// Use our own entry point and don't use the default CRT startup code to
// reduce the size of (g)vim.exe.  This works only when VIMDLL is defined.
//
// For MSVC, the /GS- compiler option is needed to avoid the undefined symbol
// error.  (It disables the security check. However, it affects only this
// function and doesn't have any effect on Vim itself.)
// For MinGW, the -nostdlib compiler option and the --entry linker option are
// needed.
# ifdef FEAT_GUI
    void WINAPI
wWinMainCRTStartup(void)
{
    VimMain(0, NULL);
}
# else
    void
wmainCRTStartup(void)
{
    VimMain(0, NULL);
}
# endif
#endif	// USE_OWNSTARTUP
