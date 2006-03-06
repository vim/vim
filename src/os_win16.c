/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * os_win16.c
 *
 * Win16 (Windows 3.1x) system-dependent routines.
 * Carved brutally from os_win32.c by Vince Negri <vn@aslnet.co.uk>
 */
#ifdef __BORLANDC__
# pragma warn -par
# pragma warn -ucp
# pragma warn -use
# pragma warn -aus
# pragma warn -obs
#endif

#include "vimio.h"
#include "vim.h"

#include <fcntl.h>
#include <dos.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <process.h>

#undef chdir
#include <direct.h>
#include <shellapi.h>	/* required for FindExecutable() */


/* Record all output and all keyboard & mouse input */
/* #define MCH_WRITE_DUMP */

#ifdef MCH_WRITE_DUMP
FILE* fdDump = NULL;
#endif


/*
 * When generating prototypes for Win32 on Unix, these lines make the syntax
 * errors disappear.  They do not need to be correct.
 */
#ifdef PROTO
typedef int HANDLE;
typedef int SMALL_RECT;
typedef int COORD;
typedef int SHORT;
typedef int WORD;
typedef int DWORD;
typedef int BOOL;
typedef int LPSTR;
typedef int LPTSTR;
typedef int KEY_EVENT_RECORD;
typedef int MOUSE_EVENT_RECORD;
# define WINAPI
typedef int CONSOLE_CURSOR_INFO;
typedef char * LPCSTR;
# define WINBASEAPI
typedef int INPUT_RECORD;
# define _cdecl
#endif

#ifdef __BORLANDC__
/* being a more ANSI compliant compiler, BorlandC doesn't define _stricoll:
 * but it does in BC 5.02! */
# if __BORLANDC__ < 0x502
int _stricoll(char *a, char *b);
# endif
#endif

/* cproto doesn't create a prototype for main() */
int _cdecl
VimMain
__ARGS((int argc, char **argv));
static int (_cdecl *pmain)(int, char **);

#ifndef PROTO
void _cdecl SaveInst(HINSTANCE hInst);
static void (_cdecl *pSaveInst)(HINSTANCE);

int WINAPI
WinMain(
    HINSTANCE	hInstance,
    HINSTANCE	hPrevInst,
    LPSTR	lpszCmdLine,
    int		nCmdShow)
{
    int		argc;
    char	**argv;
    char	*tofree;
    char	prog[256];

    /*
     * Ron: added full path name so that the $VIM variable will get set to our
     * startup path (so the .vimrc file can be found w/o a VIM env. var.)
     * Remove the ".exe" extension, and find the 1st non-space.
     */
    GetModuleFileName(hInstance, prog, 255);
    if (*prog != NUL)
	exe_name = FullName_save((char_u *)prog, FALSE);

    /* Separate the command line into arguments. */
    argc = get_cmd_args(prog, (char *)lpszCmdLine, &argv, &tofree);
    if (argc == 0)
    {
	/* Error message? */
	return 0;
    }

    pSaveInst = SaveInst;
    pmain = VimMain;
    pSaveInst(hInstance);
    pmain(argc, argv);

    free(argv);
    free(tofree);

    return 0;
}
#endif






#ifdef FEAT_MOUSE

/*
 * For the GUI the mouse handling is in gui_w32.c.
 */
    void
mch_setmouse(
    int on)
{
}
#endif /* FEAT_MOUSE */



/*
 * GUI version of mch_init().
 */
    void
mch_init()
{
    extern int _fmode;


    /* Let critical errors result in a failure, not in a dialog box.  Required
     * for the timestamp test to work on removed floppies. */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    _fmode = O_BINARY;		/* we do our own CR-LF translation */

    /* Specify window size.  Is there a place to get the default from? */
    Rows = 25;
    Columns = 80;


    set_option_value((char_u *)"grepprg", 0, (char_u *)"grep -n", 0);

#ifdef FEAT_CLIPBOARD
    clip_init(TRUE);

    /*
     * Vim's own clipboard format recognises whether the text is char, line,
     * or rectangular block.  Only useful for copying between two Vims.
     * "VimClipboard" was used for previous versions, using the first
     * character to specify MCHAR, MLINE or MBLOCK.
     */
    clip_star.format = RegisterClipboardFormat("VimClipboard2");
    clip_star.format_raw = RegisterClipboardFormat("VimRawBytes");
#endif
}



/*
 * Do we have an interactive window?
 */
    int
mch_check_win(
    int argc,
    char **argv)
{
    int		i;

    return OK;	    /* GUI always has a tty */
}


/*
 * return process ID
 */
    long
mch_get_pid()
{
    return (long)GetCurrentTask();
}


/*
 * Specialised version of system().
 * This version proceeds as follows:
 *    1. Start the program with WinExec
 *    2. Wait for the module use count of the program to go to 0
 *	 (This is the best way of detecting the program has finished)
 */

    static int
mch_system(char *cmd, int options)
{
    DWORD		ret = 0;
    UINT		wShowWindow;
    UINT		h_module;
    MSG			msg;
    BOOL		again = TRUE;

    /*
     * It's nicer to run a filter command in a minimized window, but in
     */
    if (options & SHELL_DOOUT)
	wShowWindow = SW_SHOWMINIMIZED;
    else
	wShowWindow = SW_SHOWNORMAL;

    /* Now, run the command */
    h_module = WinExec((LPCSTR)cmd, wShowWindow);

    if (h_module < 32)
    {
	/*error*/
	ret = -h_module;
    }
    else
    {
	/* Wait for the command to terminate before continuing */
	while (GetModuleUsage((HINSTANCE)h_module) > 0 && again )
	{
	    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) && again )
	    {
		if(msg.message == WM_QUIT)

		{
		    PostQuitMessage(msg.wParam);
		    again = FALSE;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    }
	}
    }

    return ret;
}

/*
 * Either execute a command by calling the shell or start a new shell
 */
    int
mch_call_shell(
    char_u *cmd,
    int options)	    /* SHELL_, see vim.h */
{
    int		x;
    int		tmode = cur_tmode;

    out_flush();


#ifdef MCH_WRITE_DUMP
    if (fdDump)
    {
	fprintf(fdDump, "mch_call_shell(\"%s\", %d)\n", cmd, options);
	fflush(fdDump);
    }
#endif

    /*
     * Catch all deadly signals while running the external command, because a
     * CTRL-C, Ctrl-Break or illegal instruction  might otherwise kill us.
     */
    signal(SIGINT, SIG_IGN);
    signal(SIGILL, SIG_IGN);
    signal(SIGFPE, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGABRT, SIG_IGN);

    if (options & SHELL_COOKED)
	settmode(TMODE_COOK);	/* set to normal mode */

    if (cmd == NULL)
    {
	x = mch_system(p_sh, options);
    }
    else
    {
	/* we use "command" or "cmd" to start the shell; slow but easy */
	char_u *newcmd;

	newcmd = lalloc(
		STRLEN(p_sh) + STRLEN(p_shcf) + STRLEN(cmd) + 10, TRUE);
	if (newcmd != NULL)
	{
	    if (STRNICMP(cmd, "start ", 6) == 0)
	    {
		sprintf((char *)newcmd, "%s\0", cmd+6);
		if (WinExec((LPCSTR)newcmd, SW_SHOWNORMAL) > 31)
		    x = 0;
		else
		    x = -1;
	    }
	    else
	    {
		sprintf((char *)newcmd, "%s%s %s %s",
			"",
			p_sh,
			p_shcf,
			cmd);
		x = mch_system((char *)newcmd, options);
	    }
	    vim_free(newcmd);
	}
    }

    if (tmode == TMODE_RAW)
	settmode(TMODE_RAW);	/* set to raw mode */

    if (x && !(options & SHELL_SILENT) && !emsg_silent)
    {
	smsg(_("shell returned %d"), x);
	msg_putchar('\n');
    }
#ifdef FEAT_TITLE
    resettitle();
#endif

    signal(SIGINT, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGABRT, SIG_DFL);


    return x;
}


/*
 * Delay for half a second.
 */
    void
mch_delay(
    long    msec,
    int	    ignoreinput)
{
#ifdef MUST_FIX
    Sleep((int)msec);	    /* never wait for input */
#endif
}


/*
 * check for an "interrupt signal": CTRL-break or CTRL-C
 */
    void
mch_breakcheck()
{
    /* never used */
}


/*
 * How much memory is available?
 */
    long_u
mch_avail_mem(
    int special)
{
    return GetFreeSpace(0);
}


/*
 * Like rename(), returns 0 upon success, non-zero upon failure.
 * Should probably set errno appropriately when errors occur.
 */
    int
mch_rename(
    const char	*pszOldFile,
    const char	*pszNewFile)
{

    /*
     * No need to play tricks, this isn't rubbish like Windows 95 <g>
     */
    return rename(pszOldFile, pszNewFile);

}

/*
 * Get the default shell for the current hardware platform
 */
    char*
default_shell()
{
    char* psz = NULL;

    psz = "command.com";

    return psz;
}
