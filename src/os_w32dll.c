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
 * Windows GUI: main program (DLL) entry point:
 *
 * Ron Aaron <ronaharon@yahoo.com> wrote this and  the DLL support code.
 */
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE  hinstDLL, DWORD	fdwReason, LPVOID  lpvReserved)
{
    return TRUE;
}

