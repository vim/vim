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
int (_cdecl *pmain)(int, char **);

#ifdef FEAT_MBYTE
/* The commandline arguments in UCS2. */
static DWORD	nArgsW = 0;
static LPWSTR	*ArglistW = NULL;
static int	global_argc;
static char	**global_argv;

static int	used_file_argc = 0;	/* last argument in global_argv[] used
					   for the argument list. */
static int	*used_file_indexes = NULL; /* indexes in global_argv[] for
					      command line arguments added to
					      the argument list */
static int	used_file_count = 0;	/* nr of entries in used_file_indexes */
static int	used_file_literal = FALSE;  /* take file names literally */
static int	used_file_full_path = FALSE;  /* file name was full path */
static int	used_alist_count = 0;
#endif

#ifndef PROTO
#ifdef FEAT_GUI
#ifndef VIMDLL
void _cdecl SaveInst(HINSTANCE hInst);
#endif
void (_cdecl *pSaveInst)(HINSTANCE);
#endif

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

    /* Separate the command line into arguments.  Use the Unicode functions
     * when possible. When 'encoding' is later changed these are used to
     * recode the arguments. */
#ifdef FEAT_MBYTE
    ArglistW = CommandLineToArgvW(GetCommandLineW(), &nArgsW);
    if (ArglistW != NULL)
    {
	argv = malloc((nArgsW + 1) * sizeof(char *));
	if (argv != NULL)
	{
	    int		i;

	    argv[argc] = NULL;
	    argc = nArgsW;
	    for (i = 0; i < argc; ++i)
	    {
		int	len;

		WideCharToMultiByte_alloc(GetACP(), 0,
				ArglistW[i], wcslen(ArglistW[i]) + 1,
				(LPSTR *)&argv[i], &len, 0, 0);
		if (argv[i] == NULL)
		{
		    while (i > 0)
			free(argv[--i]);
		    free(argv);
		    argc = 0;
		}
	    }
	}
    }

    if (argc == 0)
#endif
    {
	argc = get_cmd_args(prog, (char *)lpszCmdLine, &argv, &tofree);
	if (argc == 0)
	{
	    MessageBox(0, "Could not allocate memory for command line.",
								  "VIM Error", 0);
	    return 0;
	}
    }

#ifdef FEAT_MBYTE
    global_argc = argc;
    global_argv = argv;
    used_file_indexes = malloc(argc * sizeof(int));
#endif

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
    if (ArglistW != NULL)
	GlobalFree(ArglistW);
#endif

    return 0;
}
#endif

#ifdef FEAT_MBYTE
/*
 * Remember "name" is an argument that was added to the argument list.
 * This avoids that we have to re-parse the argument list when fix_arg_enc()
 * is called.
 */
    void
used_file_arg(name, literal, full_path)
    char	*name;
    int		literal;
    int		full_path;
{
    int		i;

    if (used_file_indexes == NULL)
	return;
    for (i = used_file_argc + 1; i < global_argc; ++i)
	if (STRCMP(global_argv[i], name) == 0)
	{
	    used_file_argc = i;
	    used_file_indexes[used_file_count++] = i;
	    break;
	}
    used_file_literal = literal;
    used_file_full_path = full_path;
}

/*
 * Remember the length of the argument list as it was.  If it changes then we
 * leave it alone when 'encoding' is set.
 */
    void
set_alist_count(void)
{
    used_alist_count = GARGCOUNT;
}

/*
 * Fix the encoding of the command line arguments.  Invoked when 'encoding'
 * has been changed while starting up.  Use the UCS-2 command line arguments
 * and convert them to 'encoding'.
 */
    void
fix_arg_enc()
{
    int		i;
    int		idx;
    char_u	*str;

    /* Safety checks:
     * - if argument count differs between the wide and non-wide argument
     *   list, something must be wrong.
     * - the file name arguments must have been located.
     * - the length of the argument list wasn't changed by the user.
     */
    if (global_argc != (int)nArgsW
	    || ArglistW == NULL
	    || used_file_indexes == NULL
	    || used_file_count == 0
	    || used_alist_count != GARGCOUNT)
	return;

    /* Clear the argument list.  Make room for the new arguments. */
    alist_clear(&global_alist);
    if (ga_grow(&global_alist.al_ga, used_file_count) == FAIL)
	return;	    /* out of memory */

    for (i = 0; i < used_file_count; ++i)
    {
	idx = used_file_indexes[i];
	str = ucs2_to_enc(ArglistW[idx], NULL);
	if (str != NULL)
	    alist_add(&global_alist, str, used_file_literal ? 2 : 0);
    }

    if (!used_file_literal)
    {
	/* Now expand wildcards in the arguments. */
	/* Temporarily add '(' and ')' to 'isfname'.  These are valid
	 * filename characters but are excluded from 'isfname' to make
	 * "gf" work on a file name in parenthesis (e.g.: see vim.h). */
	do_cmdline_cmd((char_u *)":let SaVe_ISF = &isf|set isf+=(,)");
	alist_expand();
	do_cmdline_cmd((char_u *)":let &isf = SaVe_ISF|unlet SaVe_ISF");
    }

    /* If wildcard expansion failed, we are editing the first file of the
     * arglist and there is no file name: Edit the first argument now. */
    if (curwin->w_arg_idx == 0 && curbuf->b_fname == NULL)
    {
	do_cmdline_cmd((char_u *)":rewind");
	if (GARGCOUNT == 1 && used_file_full_path)
	    (void)vim_chdirfile(alist_name(&GARGLIST[0]));
    }
}
#endif

