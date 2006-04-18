/* vi:set ts=8 sts=4 sw=4:
 *
 * CSCOPE support for Vim added by Andy Kahn <kahn@zk3.dec.com>
 * Ported to Win32 by Sergey Khorev <sergey.khorev@gmail.com>
 *
 * The basic idea/structure of cscope for Vim was borrowed from Nvi.  There
 * might be a few lines of code that look similar to what Nvi has.
 *
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#if defined(FEAT_CSCOPE) || defined(PROTO)

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(UNIX)
# include <sys/wait.h>
#else
    /* not UNIX, must be WIN32 */
# include "vimio.h"
# include <fcntl.h>
# include <process.h>
# define STDIN_FILENO    0
# define STDOUT_FILENO   1
# define STDERR_FILENO   2
# define pipe(fds) _pipe(fds, 256, O_TEXT|O_NOINHERIT)
#endif
#include "if_cscope.h"

static void	    cs_usage_msg __ARGS((csid_e x));
static int	    cs_add __ARGS((exarg_T *eap));
static void	    cs_stat_emsg __ARGS((char *fname));
static int	    cs_add_common __ARGS((char *, char *, char *));
static int	    cs_check_for_connections __ARGS((void));
static int	    cs_check_for_tags __ARGS((void));
static int	    cs_cnt_connections __ARGS((void));
static void	    cs_reading_emsg __ARGS((int idx));
static int	    cs_cnt_matches __ARGS((int idx));
static char *	    cs_create_cmd __ARGS((char *csoption, char *pattern));
static int	    cs_create_connection __ARGS((int i));
static void	    do_cscope_general __ARGS((exarg_T *eap, int make_split));
#ifdef FEAT_QUICKFIX
static void	    cs_file_results __ARGS((FILE *, int *));
#endif
static void	    cs_fill_results __ARGS((char *, int , int *, char ***,
			char ***, int *));
static int	    cs_find __ARGS((exarg_T *eap));
static int	    cs_find_common __ARGS((char *opt, char *pat, int, int, int));
static int	    cs_help __ARGS((exarg_T *eap));
static void	    cs_init __ARGS((void));
static void	    clear_csinfo __ARGS((int i));
static int	    cs_insert_filelist __ARGS((char *, char *, char *,
			struct stat *));
static int	    cs_kill __ARGS((exarg_T *eap));
static void	    cs_kill_execute __ARGS((int, char *));
static cscmd_T *    cs_lookup_cmd __ARGS((exarg_T *eap));
static char *	    cs_make_vim_style_matches __ARGS((char *, char *,
			char *, char *));
static char *	    cs_manage_matches __ARGS((char **, char **, int, mcmd_e));
static char *	    cs_parse_results __ARGS((int cnumber, char *buf, int bufsize, char **context, char **linenumber, char **search));
static char *	    cs_pathcomponents __ARGS((char *path));
static void	    cs_print_tags_priv __ARGS((char **, char **, int));
static int	    cs_read_prompt __ARGS((int ));
static void	    cs_release_csp __ARGS((int, int freefnpp));
static int	    cs_reset __ARGS((exarg_T *eap));
static char *	    cs_resolve_file __ARGS((int, char *));
static int	    cs_show __ARGS((exarg_T *eap));


static csinfo_T	    csinfo[CSCOPE_MAX_CONNECTIONS];
static cscmd_T	    cs_cmds[] =
{
    { "add",	cs_add,
		N_("Add a new database"),     "add file|dir [pre-path] [flags]", 0 },
    { "find",	cs_find,
		N_("Query for a pattern"),    FIND_USAGE, 1 },
    { "help",	cs_help,
		N_("Show this message"),      "help", 0 },
    { "kill",	cs_kill,
		N_("Kill a connection"),      "kill #", 0 },
    { "reset",	cs_reset,
		N_("Reinit all connections"), "reset", 0 },
    { "show",	cs_show,
		N_("Show connections"),       "show", 0 },
    { NULL }
};

    static void
cs_usage_msg(x)
    csid_e x;
{
    (void)EMSG2(_("E560: Usage: cs[cope] %s"), cs_cmds[(int)x].usage);
}

/*
 * PRIVATE: do_cscope_general
 *
 * find the command, print help if invalid, and the then call the
 * corresponding command function,
 * called from do_cscope and do_scscope
 */
    static void
do_cscope_general(eap, make_split)
    exarg_T	*eap;
    int		make_split; /* whether to split window */
{
    cscmd_T *cmdp;

    cs_init();
    if ((cmdp = cs_lookup_cmd(eap)) == NULL)
    {
	cs_help(eap);
	return;
    }

#ifdef FEAT_WINDOWS
    if (make_split)
    {
	if (!cmdp->cansplit)
	{
	    (void)MSG_PUTS(_("This cscope command does not support splitting the window.\n"));
	    return;
	}
	postponed_split = -1;
	postponed_split_flags = cmdmod.split;
    }
#endif

    cmdp->func(eap);

#ifdef FEAT_WINDOWS
    postponed_split_flags = 0;
#endif
}

/*
 * PUBLIC: do_cscope
 */
    void
do_cscope(eap)
    exarg_T *eap;
{
    do_cscope_general(eap, FALSE);
}

/*
 * PUBLIC: do_scscope
 *
 * same as do_cscope, but splits window, too.
 */
    void
do_scscope(eap)
    exarg_T *eap;
{
    do_cscope_general(eap, TRUE);
}

/*
 * PUBLIC: do_cstag
 *
 */
    void
do_cstag(eap)
    exarg_T *eap;
{
    int ret = FALSE;

    cs_init();

    if (eap->arg == NULL || strlen((const char *)(eap->arg)) == 0)
    {
	(void)EMSG(_("E562: Usage: cstag <ident>"));
	return;
    }

    switch (p_csto)
    {
    case 0 :
	if (cs_check_for_connections())
	{
	    ret = cs_find_common("g", (char *)(eap->arg), eap->forceit, FALSE,
				 FALSE);
	    if (ret == FALSE)
	    {
		cs_free_tags();
		if (msg_col)
		    msg_putchar('\n');

		if (cs_check_for_tags())
		    ret = do_tag(eap->arg, DT_JUMP, 0, eap->forceit, FALSE);
	    }
	}
	else if (cs_check_for_tags())
	{
	    ret = do_tag(eap->arg, DT_JUMP, 0, eap->forceit, FALSE);
	}
	break;
    case 1 :
	if (cs_check_for_tags())
	{
	    ret = do_tag(eap->arg, DT_JUMP, 0, eap->forceit, FALSE);
	    if (ret == FALSE)
	    {
		if (msg_col)
		    msg_putchar('\n');

		if (cs_check_for_connections())
		{
		    ret = cs_find_common("g", (char *)(eap->arg), eap->forceit,
					 FALSE, FALSE);
		    if (ret == FALSE)
			cs_free_tags();
		}
	    }
	}
	else if (cs_check_for_connections())
	{
	    ret = cs_find_common("g", (char *)(eap->arg), eap->forceit, FALSE,
				 FALSE);
	    if (ret == FALSE)
		cs_free_tags();
	}
	break;
    default :
	break;
    }

    if (!ret)
    {
	(void)EMSG(_("E257: cstag: tag not found"));
#if defined(FEAT_WINDOWS) && defined(FEAT_QUICKFIX)
	g_do_tagpreview = 0;
#endif
    }

} /* do_cscope */


/*
 * PUBLIC: cs_find
 *
 * this simulates a vim_fgets(), but for cscope, returns the next line
 * from the cscope output.  should only be called from find_tags()
 *
 * returns TRUE if eof, FALSE otherwise
 */
    int
cs_fgets(buf, size)
    char_u	*buf;
    int		size;
{
    char *p;

    if ((p = cs_manage_matches(NULL, NULL, -1, Get)) == NULL)
	return TRUE;

    if ((int)strlen(p) > size)
    {
	strncpy((char *)buf, p, size - 1);
	buf[size] = '\0';
    }
    else
	(void)strcpy((char *)buf, p);

    return FALSE;
} /* cs_fgets */


/*
 * PUBLIC: cs_free_tags
 *
 * called only from do_tag(), when popping the tag stack
 */
    void
cs_free_tags()
{
    cs_manage_matches(NULL, NULL, -1, Free);
}


/*
 * PUBLIC: cs_print_tags
 *
 * called from do_tag()
 */
    void
cs_print_tags()
{
    cs_manage_matches(NULL, NULL, -1, Print);
}


/*
 * "cscope_connection([{num} , {dbpath} [, {prepend}]])" function
 *
 *		Checks for the existence of a |cscope| connection.  If no
 *		parameters are specified, then the function returns:
 *
 *		0, if cscope was not available (not compiled in), or if there
 *		are no cscope connections; or
 *		1, if there is at least one cscope connection.
 *
 *		If parameters are specified, then the value of {num}
 *		determines how existence of a cscope connection is checked:
 *
 *		{num}	Description of existence check
 *		-----	------------------------------
 *		0	Same as no parameters (e.g., "cscope_connection()").
 *		1	Ignore {prepend}, and use partial string matches for
 *			{dbpath}.
 *		2	Ignore {prepend}, and use exact string matches for
 *			{dbpath}.
 *		3	Use {prepend}, use partial string matches for both
 *			{dbpath} and {prepend}.
 *		4	Use {prepend}, use exact string matches for both
 *			{dbpath} and {prepend}.
 *
 *		Note: All string comparisons are case sensitive!
 */
#if defined(FEAT_EVAL) || defined(PROTO)
    int
cs_connection(num, dbpath, ppath)
    int num;
    char_u *dbpath;
    char_u *ppath;
{
    int i;

    if (num < 0 || num > 4 || (num > 0 && !dbpath))
	return FALSE;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (!csinfo[i].fname)
	    continue;

	if (num == 0)
	    return TRUE;

	switch (num)
	{
	case 1:
	    if (strstr(csinfo[i].fname, (char *)dbpath))
		return TRUE;
	    break;
	case 2:
	    if (strcmp(csinfo[i].fname, (char *)dbpath) == 0)
		return TRUE;
	    break;
	case 3:
	    if (strstr(csinfo[i].fname, (char *)dbpath)
		    && ((!ppath && !csinfo[i].ppath)
			|| (ppath
			    && csinfo[i].ppath
			    && strstr(csinfo[i].ppath, (char *)ppath))))
		return TRUE;
	    break;
	case 4:
	    if ((strcmp(csinfo[i].fname, (char *)dbpath) == 0)
		    && ((!ppath && !csinfo[i].ppath)
			|| (ppath
			    && csinfo[i].ppath
			    && (strcmp(csinfo[i].ppath, (char *)ppath) == 0))))
		return TRUE;
	    break;
	}
    }

    return FALSE;
} /* cs_connection */
#endif


/*
 * PRIVATE functions
 ****************************************************************************/

/*
 * PRIVATE: cs_add
 *
 * add cscope database or a directory name (to look for cscope.out)
 * the the cscope connection list
 *
 * MAXPATHL 256
 */
/* ARGSUSED */
    static int
cs_add(eap)
    exarg_T *eap;
{
    char *fname, *ppath, *flags = NULL;

    if ((fname = strtok((char *)NULL, (const char *)" ")) == NULL)
    {
	cs_usage_msg(Add);
	return CSCOPE_FAILURE;
    }
    if ((ppath = strtok((char *)NULL, (const char *)" ")) != NULL)
	flags = strtok((char *)NULL, (const char *)" ");

    return cs_add_common(fname, ppath, flags);
}

    static void
cs_stat_emsg(fname)
    char *fname;
{
    char *stat_emsg = _("E563: stat(%s) error: %d");
    char *buf = (char *)alloc((unsigned)strlen(stat_emsg) + MAXPATHL + 10);

    if (buf != NULL)
    {
	(void)sprintf(buf, stat_emsg, fname, errno);
	(void)EMSG(buf);
	vim_free(buf);
    }
    else
	(void)EMSG(_("E563: stat error"));
}


/*
 * PRIVATE: cs_add_common
 *
 * the common routine to add a new cscope connection.  called by
 * cs_add() and cs_reset().  i really don't like to do this, but this
 * routine uses a number of goto statements.
 */
    static int
cs_add_common(arg1, arg2, flags)
    char *arg1;	    /* filename - may contain environment variables */
    char *arg2;	    /* prepend path - may contain environment variables */
    char *flags;
{
    struct stat statbuf;
    int		ret;
    char	*fname = NULL;
    char	*fname2 = NULL;
    char	*ppath = NULL;
    int		i;

    /* get the filename (arg1), expand it, and try to stat it */
    if ((fname = (char *)alloc(MAXPATHL + 1)) == NULL)
	goto add_err;

    expand_env((char_u *)arg1, (char_u *)fname, MAXPATHL);
    ret = stat(fname, &statbuf);
    if (ret < 0)
    {
staterr:
	if (p_csverbose)
	    cs_stat_emsg(fname);
	goto add_err;
    }

    /* get the prepend path (arg2), expand it, and try to stat it */
    if (arg2 != NULL)
    {
	struct stat statbuf2;

	if ((ppath = (char *)alloc(MAXPATHL + 1)) == NULL)
	    goto add_err;

	expand_env((char_u *)arg2, (char_u *)ppath, MAXPATHL);
	ret = stat(ppath, &statbuf2);
	if (ret < 0)
	    goto staterr;
    }

    /* if filename is a directory, append the cscope database name to it */
    if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
    {
	fname2 = (char *)alloc((unsigned)(strlen(CSCOPE_DBFILE) + strlen(fname) + 2));
	if (fname2 == NULL)
	    goto add_err;

	while (fname[strlen(fname)-1] == '/'
#ifdef WIN32
		|| fname[strlen(fname)-1] == '\\'
#endif
		)
	{
	    fname[strlen(fname)-1] = '\0';
	    if (strlen(fname) == 0)
		break;
	}
	if (fname[0] == '\0')
	    (void)sprintf(fname2, "/%s", CSCOPE_DBFILE);
	else
	    (void)sprintf(fname2, "%s/%s", fname, CSCOPE_DBFILE);

	ret = stat(fname2, &statbuf);
	if (ret < 0)
	{
	    if (p_csverbose)
		cs_stat_emsg(fname2);
	    goto add_err;
	}

	i = cs_insert_filelist(fname2, ppath, flags, &statbuf);
    }
#if defined(UNIX)
    else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode))
#else
	/* substitute define S_ISREG from os_unix.h */
    else if (((statbuf.st_mode) & S_IFMT) == S_IFREG)
#endif
    {
	i = cs_insert_filelist(fname, ppath, flags, &statbuf);
    }
    else
    {
	if (p_csverbose)
	    (void)EMSG2(
		_("E564: %s is not a directory or a valid cscope database"),
		fname);
	goto add_err;
    }

    if (i != -1)
    {
	if (cs_create_connection(i) == CSCOPE_FAILURE
		|| cs_read_prompt(i) == CSCOPE_FAILURE)
	{
	    cs_release_csp(i, TRUE);
	    goto add_err;
	}

	if (p_csverbose)
	{
	    msg_clr_eos();
	    (void)smsg_attr(hl_attr(HLF_R),
			    (char_u *)_("Added cscope database %s"),
			    csinfo[i].fname);
	}
    }

    vim_free(fname);
    vim_free(fname2);
    vim_free(ppath);
    return CSCOPE_SUCCESS;

add_err:
    vim_free(fname2);
    vim_free(fname);
    vim_free(ppath);
    return CSCOPE_FAILURE;
} /* cs_add_common */


    static int
cs_check_for_connections()
{
    return (cs_cnt_connections() > 0);
} /* cs_check_for_connections */


    static int
cs_check_for_tags()
{
    return (p_tags[0] != NUL && curbuf->b_p_tags != NULL);
} /* cs_check_for_tags */


/*
 * PRIVATE: cs_cnt_connections
 *
 * count the number of cscope connections
 */
    static int
cs_cnt_connections()
{
    short i;
    short cnt = 0;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (csinfo[i].fname != NULL)
	    cnt++;
    }
    return cnt;
} /* cs_cnt_connections */

    static void
cs_reading_emsg(idx)
    int idx;	/* connection index */
{
    EMSGN(_("E262: error reading cscope connection %ld"), idx);
}

#define	CSREAD_BUFSIZE	2048
/*
 * PRIVATE: cs_cnt_matches
 *
 * count the number of matches for a given cscope connection.
 */
    static int
cs_cnt_matches(idx)
    int idx;
{
    char *stok;
    char *buf;
    int nlines;

    buf = (char *)alloc(CSREAD_BUFSIZE);
    if (buf == NULL)
	return 0;
    for (;;)
    {
	if (!fgets(buf, CSREAD_BUFSIZE, csinfo[idx].fr_fp))
	{
	    if (feof(csinfo[idx].fr_fp))
		errno = EIO;

	    cs_reading_emsg(idx);

	    vim_free(buf);
	    return -1;
	}

	/*
	 * If the database is out of date, or there's some other problem,
	 * cscope will output error messages before the number-of-lines output.
	 * Display/discard any output that doesn't match what we want.
	 */
	if ((stok = strtok(buf, (const char *)" ")) == NULL)
	    continue;
	if (strcmp((const char *)stok, "cscope:"))
	    continue;

	if ((stok = strtok(NULL, (const char *)" ")) == NULL)
	    continue;
	nlines = atoi(stok);
	if (nlines < 0)
	{
	    nlines = 0;
	    break;
	}

	if ((stok = strtok(NULL, (const char *)" ")) == NULL)
	    continue;
	if (strncmp((const char *)stok, "lines", 5))
	    continue;

	break;
    }

    vim_free(buf);
    return nlines;
} /* cs_cnt_matches */


/*
 * PRIVATE: cs_create_cmd
 *
 * Creates the actual cscope command query from what the user entered.
 */
    static char *
cs_create_cmd(csoption, pattern)
    char *csoption;
    char *pattern;
{
    char *cmd;
    short search;

    switch (csoption[0])
    {
    case '0' : case 's' :
	search = 0;
	break;
    case '1' : case 'g' :
	search = 1;
	break;
    case '2' : case 'd' :
	search = 2;
	break;
    case '3' : case 'c' :
	search = 3;
	break;
    case '4' : case 't' :
	search = 4;
	break;
    case '6' : case 'e' :
	search = 6;
	break;
    case '7' : case 'f' :
	search = 7;
	break;
    case '8' : case 'i' :
	search = 8;
	break;
    default :
	(void)EMSG(_("E561: unknown cscope search type"));
	cs_usage_msg(Find);
	return NULL;
    }

    if ((cmd = (char *)alloc((unsigned)(strlen(pattern) + 2))) == NULL)
	return NULL;

    (void)sprintf(cmd, "%d%s", search, pattern);

    return cmd;
} /* cs_create_cmd */


/*
 * PRIVATE: cs_create_connection
 *
 * This piece of code was taken/adapted from nvi.  do we need to add
 * the BSD license notice?
 */
    static int
cs_create_connection(i)
    int i;
{
    int to_cs[2], from_cs[2], len;
    char *prog, *cmd, *ppath = NULL;
#ifndef UNIX
    int in_save, out_save, err_save;
    long_i ph;
# ifdef FEAT_GUI
    HWND activewnd = NULL;
    HWND consolewnd = NULL;
# endif
#endif

    /*
     * Cscope reads from to_cs[0] and writes to from_cs[1]; vi reads from
     * from_cs[0] and writes to to_cs[1].
     */
    to_cs[0] = to_cs[1] = from_cs[0] = from_cs[1] = -1;
    if (pipe(to_cs) < 0 || pipe(from_cs) < 0)
    {
	(void)EMSG(_("E566: Could not create cscope pipes"));
err_closing:
	if (to_cs[0] != -1)
	    (void)close(to_cs[0]);
	if (to_cs[1] != -1)
	    (void)close(to_cs[1]);
	if (from_cs[0] != -1)
	    (void)close(from_cs[0]);
	if (from_cs[1] != -1)
	    (void)close(from_cs[1]);
	return CSCOPE_FAILURE;
    }

#if defined(UNIX)
    switch (csinfo[i].pid = fork())
    {
    case -1:
	(void)EMSG(_("E622: Could not fork for cscope"));
	goto err_closing;
    case 0:				/* child: run cscope. */
#else
	in_save = dup(STDIN_FILENO);
	out_save = dup(STDOUT_FILENO);
	err_save = dup(STDERR_FILENO);
#endif
	if (dup2(to_cs[0], STDIN_FILENO) == -1)
	    PERROR("cs_create_connection 1");
	if (dup2(from_cs[1], STDOUT_FILENO) == -1)
	    PERROR("cs_create_connection 2");
	if (dup2(from_cs[1], STDERR_FILENO) == -1)
	    PERROR("cs_create_connection 3");

	/* close unused */
#if defined(UNIX)
	(void)close(to_cs[1]);
	(void)close(from_cs[0]);
#else
	/* On win32 we must close opposite ends because we are the parent */
	(void)close(to_cs[0]);
	to_cs[0] = -1;
	(void)close(from_cs[1]);
	from_cs[1] = -1;
#endif
	/* expand the cscope exec for env var's */
	if ((prog = (char *)alloc(MAXPATHL + 1)) == NULL)
	{
#ifdef UNIX
	    return CSCOPE_FAILURE;
#else
	    goto err_closing;
#endif
	}
	expand_env((char_u *)p_csprg, (char_u *)prog, MAXPATHL);

	/* alloc space to hold the cscope command */
	len = (int)(strlen(prog) + strlen(csinfo[i].fname) + 32);
	if (csinfo[i].ppath)
	{
	    /* expand the prepend path for env var's */
	    if ((ppath = (char *)alloc(MAXPATHL + 1)) == NULL)
	    {
		vim_free(prog);
#ifdef UNIX
		return CSCOPE_FAILURE;
#else
		goto err_closing;
#endif
	    }
	    expand_env((char_u *)csinfo[i].ppath, (char_u *)ppath, MAXPATHL);

	    len += (int)strlen(ppath);
	}

	if (csinfo[i].flags)
	    len += (int)strlen(csinfo[i].flags);

	if ((cmd = (char *)alloc(len)) == NULL)
	{
	    vim_free(prog);
	    vim_free(ppath);
#ifdef UNIX
	    return CSCOPE_FAILURE;
#else
	    goto err_closing;
#endif
	}

	/* run the cscope command; is there execl for non-unix systems? */
#if defined(UNIX)
	(void)sprintf(cmd, "exec %s -dl -f %s", prog, csinfo[i].fname);
#else
	(void)sprintf(cmd, "%s -dl -f %s", prog, csinfo[i].fname);
#endif
	if (csinfo[i].ppath != NULL)
	{
	    (void)strcat(cmd, " -P");
	    (void)strcat(cmd, csinfo[i].ppath);
	}
	if (csinfo[i].flags != NULL)
	{
	    (void)strcat(cmd, " ");
	    (void)strcat(cmd, csinfo[i].flags);
	}
# ifdef UNIX
      /* on Win32 we still need prog */
	vim_free(prog);
# endif
	vim_free(ppath);

#if defined(UNIX)
	if (execl("/bin/sh", "sh", "-c", cmd, NULL) == -1)
	    PERROR(_("cs_create_connection exec failed"));

	exit(127);
	/* NOTREACHED */
    default:	/* parent. */
#else
# ifdef FEAT_GUI
	activewnd = GetForegroundWindow(); /* on win9x cscope steals focus */
	/* Dirty hack to hide annoying console window */
	if (AllocConsole())
	{
	    char *title;
	    title = (char *)alloc(1024);
	    if (title == NULL)
		FreeConsole();
	    else
	    {
		GetConsoleTitle(title, 1024); /* save for future restore */
		SetConsoleTitle(
		    "GVIMCS{5499421B-CBEF-45b0-85EF-38167FDEA5C5}GVIMCS");
		Sleep(40); /* as stated in MS KB we must wait 40 ms */
		consolewnd = FindWindow(NULL,
			"GVIMCS{5499421B-CBEF-45b0-85EF-38167FDEA5C5}GVIMCS");
		if (consolewnd != NULL)
		    ShowWindow(consolewnd, SW_HIDE);
		SetConsoleTitle(title);
		vim_free(title);
	    }
	}
# endif
	/* May be use &shell, &shellquote etc */
# ifdef __BORLANDC__
	/* BCC 5.5 uses a different function name for spawnlp */
	ph = (long_i)spawnlp(P_NOWAIT, prog, cmd, NULL);
# else
	ph = (long_i)_spawnlp(_P_NOWAIT, prog, cmd, NULL);
# endif
	vim_free(prog);
	vim_free(cmd);
# ifdef FEAT_GUI
	/* Dirty hack part two */
	if (activewnd != NULL)
	    /* restoring focus */
	    SetForegroundWindow(activewnd);
	if (consolewnd != NULL)
	    FreeConsole();

# endif
	if (ph == -1)
	{
	    PERROR(_("cs_create_connection exec failed"));
	    (void)EMSG(_("E623: Could not spawn cscope process"));
	    goto err_closing;
	}
	/* else */
	csinfo[i].pid = 0;
	csinfo[i].hProc = (HANDLE)ph;

#endif /* !UNIX */
	/*
	 * Save the file descriptors for later duplication, and
	 * reopen as streams.
	 */
	if ((csinfo[i].to_fp = fdopen(to_cs[1], "w")) == NULL)
	    PERROR(_("cs_create_connection: fdopen for to_fp failed"));
	if ((csinfo[i].fr_fp = fdopen(from_cs[0], "r")) == NULL)
	    PERROR(_("cs_create_connection: fdopen for fr_fp failed"));

#if defined(UNIX)
	/* close unused */
	(void)close(to_cs[0]);
	(void)close(from_cs[1]);

	break;
    }
#else
	/* restore stdhandles */
    dup2(in_save, STDIN_FILENO);
    dup2(out_save, STDOUT_FILENO);
    dup2(err_save, STDERR_FILENO);
    close(in_save);
    close(out_save);
    close(err_save);
#endif
    return CSCOPE_SUCCESS;
} /* cs_create_connection */


/*
 * PRIVATE: cs_find
 *
 * query cscope using command line interface.  parse the output and use tselect
 * to allow choices.  like Nvi, creates a pipe to send to/from query/cscope.
 *
 * returns TRUE if we jump to a tag or abort, FALSE if not.
 */
    static int
cs_find(eap)
    exarg_T *eap;
{
    char *opt, *pat;

    if (cs_check_for_connections() == FALSE)
    {
	(void)EMSG(_("E567: no cscope connections"));
	return FALSE;
    }

    if ((opt = strtok((char *)NULL, (const char *)" ")) == NULL)
    {
	cs_usage_msg(Find);
	return FALSE;
    }

    pat = opt + strlen(opt) + 1;
    if (pat == NULL || (pat != NULL && pat[0] == '\0'))
    {
	cs_usage_msg(Find);
	return FALSE;
    }

    return cs_find_common(opt, pat, eap->forceit, TRUE,
			  eap->cmdidx == CMD_lcscope);
} /* cs_find */


/*
 * PRIVATE: cs_find_common
 *
 * common code for cscope find, shared by cs_find() and do_cstag()
 */
    static int
cs_find_common(opt, pat, forceit, verbose, use_ll)
    char *opt;
    char *pat;
    int forceit;
    int verbose;
    int	use_ll;
{
    int i;
    char *cmd;
    char **matches, **contexts;
    int nummatches[CSCOPE_MAX_CONNECTIONS], totmatches, matched;
#ifdef FEAT_QUICKFIX
    char cmdletter;
    char *qfpos;
#endif

    /* create the actual command to send to cscope */
    cmd = cs_create_cmd(opt, pat);
    if (cmd == NULL)
	return FALSE;

    /* send query to all open connections, then count the total number
     * of matches so we can alloc matchesp all in one swell foop
     */
    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	nummatches[i] = 0;
    totmatches = 0;
    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (csinfo[i].fname == NULL)
	    continue;

	/* send cmd to cscope */
	(void)fprintf(csinfo[i].to_fp, "%s\n", cmd);
	(void)fflush(csinfo[i].to_fp);

	nummatches[i] = cs_cnt_matches(i);

	if (nummatches[i] > -1)
	    totmatches += nummatches[i];

	if (nummatches[i] == 0)
	    (void)cs_read_prompt(i);
    }
    vim_free(cmd);

    if (totmatches == 0)
    {
	char *nf = _("E259: no matches found for cscope query %s of %s");
	char *buf;

	if (!verbose)
	    return FALSE;

	buf = (char *)alloc((unsigned)(strlen(opt) + strlen(pat) + strlen(nf)));
	if (buf == NULL)
	    (void)EMSG(nf);
	else
	{
	    sprintf(buf, nf, opt, pat);
	    (void)EMSG(buf);
	    vim_free(buf);
	}
	return FALSE;
    }

#ifdef FEAT_QUICKFIX
    /* get cmd letter */
    switch (opt[0])
    {
    case '0' :
	cmdletter = 's';
	break;
    case '1' :
	cmdletter = 'g';
	break;
    case '2' :
	cmdletter = 'd';
	break;
    case '3' :
	cmdletter = 'c';
	break;
    case '4' :
	cmdletter = 't';
	break;
    case '6' :
	cmdletter = 'e';
	break;
    case '7' :
	cmdletter = 'f';
	break;
    case '8' :
	cmdletter = 'i';
	break;
    default :
	cmdletter = opt[0];
    }

    qfpos = (char *)vim_strchr(p_csqf, cmdletter);
    if (qfpos != NULL)
    {
	qfpos++;
	/* next symbol must be + or - */
	if (strchr(CSQF_FLAGS, *qfpos) == NULL)
	{
	    char *nf = _("E469: invalid cscopequickfix flag %c for %c");
	    char *buf = (char *)alloc((unsigned)strlen(nf));

	    /* strlen will be enough because we use chars */
	    if (buf != NULL)
	    {
		sprintf(buf, nf, *qfpos, *(qfpos-1));
		(void)EMSG(buf);
		vim_free(buf);
	    }
	    return FALSE;
	}
    }
    if (qfpos != NULL && *qfpos != '0' && totmatches > 0)
    {
	/* fill error list */
	FILE *f;
	char_u *tmp = vim_tempname('c');
	qf_info_T   *qi = NULL;
	win_T	    *wp = NULL;

	f = mch_fopen((char *)tmp, "w");
	cs_file_results(f, nummatches);
	fclose(f);
	if (use_ll)	    /* Use location list */
	    wp = curwin;
	/* '-' starts a new error list */
	if (qf_init(wp, tmp, (char_u *)"%f%*\\t%l%*\\t%m", *qfpos == '-') > 0)
	{
# ifdef FEAT_WINDOWS
	    if (postponed_split != 0)
	    {
		win_split(postponed_split > 0 ? postponed_split : 0,
						       postponed_split_flags);
#  ifdef FEAT_SCROLLBIND
		curwin->w_p_scb = FALSE;
#  endif
		postponed_split = 0;
	    }
# endif
	    if (use_ll)
		/*
		 * In the location list window, use the displayed location
		 * list. Otherwise, use the location list for the window.
		 */
		qi = (bt_quickfix(wp->w_buffer) && wp->w_llist_ref != NULL) ?
				    wp->w_llist_ref : wp->w_llist;
	    qf_jump(qi, 0, 0, forceit);
	}
	mch_remove(tmp);
	vim_free(tmp);
	return TRUE;
    }
    else
#endif /* FEAT_QUICKFIX */
    {
	/* read output */
	cs_fill_results((char *)pat, totmatches, nummatches, &matches,
							 &contexts, &matched);
	if (matches == NULL)
	    return FALSE;

	(void)cs_manage_matches(matches, contexts, matched, Store);

	return do_tag((char_u *)pat, DT_CSCOPE, 0, forceit, verbose);
    }

} /* cs_find_common */

/*
 * PRIVATE: cs_help
 *
 * print help
 */
/* ARGSUSED */
    static int
cs_help(eap)
    exarg_T *eap;
{
    cscmd_T *cmdp = cs_cmds;

    (void)MSG_PUTS(_("cscope commands:\n"));
    while (cmdp->name != NULL)
    {
	(void)smsg((char_u *)_("%-5s: %-30s (Usage: %s)"),
				      cmdp->name, _(cmdp->help), cmdp->usage);
	if (strcmp(cmdp->name, "find") == 0)
	    MSG_PUTS(FIND_HELP);
	cmdp++;
    }

    wait_return(TRUE);
    return 0;
} /* cs_help */


/*
 * PRIVATE: cs_init
 *
 * initialize cscope structure if not already
 */
    static void
cs_init()
{
    short i;
    static int init_already = FALSE;

    if (init_already)
	return;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	clear_csinfo(i);

    init_already = TRUE;
} /* cs_init */

    static void
clear_csinfo(i)
    int	    i;
{
    csinfo[i].fname  = NULL;
    csinfo[i].ppath  = NULL;
    csinfo[i].flags  = NULL;
#if defined(UNIX)
    csinfo[i].st_dev = (dev_t)0;
    csinfo[i].st_ino = (ino_t)0;
#else
    csinfo[i].nVolume = 0;
    csinfo[i].nIndexHigh = 0;
    csinfo[i].nIndexLow = 0;
#endif
    csinfo[i].pid    = -1;
    csinfo[i].fr_fp  = NULL;
    csinfo[i].to_fp  = NULL;
#if defined(WIN32)
    csinfo[i].hProc = NULL;
#endif
}

#ifndef UNIX
static char *GetWin32Error __ARGS((void));

    static char *
GetWin32Error()
{
    char *msg = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
	    NULL, GetLastError(), 0, (LPSTR)&msg, 0, NULL);
    if (msg != NULL)
    {
	/* remove trailing \r\n */
	char *pcrlf = strstr(msg, "\r\n");
	if (pcrlf != NULL)
	    *pcrlf = '\0';
    }
    return msg;
}
#endif

/*
 * PRIVATE: cs_insert_filelist
 *
 * insert a new cscope database filename into the filelist
 */
/*ARGSUSED*/
    static int
cs_insert_filelist(fname, ppath, flags, sb)
    char *fname;
    char *ppath;
    char *flags;
    struct stat *sb;
{
    short	i, j;
#ifndef UNIX
    HANDLE	hFile;
    BY_HANDLE_FILE_INFORMATION bhfi;

    vim_memset(&bhfi, 0, sizeof(bhfi));
    /* On windows 9x GetFileInformationByHandle doesn't work, so skip it */
    if (!mch_windows95())
    {
	hFile = CreateFile(fname, FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
	    if (p_csverbose)
	    {
		char *cant_msg = _("E625: cannot open cscope database: %s");
		char *winmsg = GetWin32Error();

		if (winmsg != NULL)
		{
		    (void)EMSG2(cant_msg, winmsg);
		    LocalFree(winmsg);
		}
		else
		    /* subst filename if can't get error text */
		    (void)EMSG2(cant_msg, fname);
	    }
	    return -1;
	}
	if (!GetFileInformationByHandle(hFile, &bhfi))
	{
	    CloseHandle(hFile);
	    if (p_csverbose)
		(void)EMSG(_("E626: cannot get cscope database information"));
	    return -1;
	}
	CloseHandle(hFile);
    }
#endif

    i = -1; /* can be set to the index of an empty item in csinfo */
    for (j = 0; j < CSCOPE_MAX_CONNECTIONS; j++)
    {
	if (csinfo[j].fname != NULL
#if defined(UNIX)
	    && csinfo[j].st_dev == sb->st_dev && csinfo[j].st_ino == sb->st_ino
#else
	    /* compare pathnames first */
	    && ((fullpathcmp(csinfo[j].fname, fname, FALSE) & FPC_SAME)
		/* if not Windows 9x, test index file atributes too */
		|| (!mch_windows95()
		    && csinfo[j].nVolume == bhfi.dwVolumeSerialNumber
		    && csinfo[j].nIndexHigh == bhfi.nFileIndexHigh
		    && csinfo[j].nIndexLow == bhfi.nFileIndexLow))
#endif
	    )
	{
	    if (p_csverbose)
		(void)EMSG(_("E568: duplicate cscope database not added"));
	    return -1;
	}

	if (csinfo[j].fname == NULL && i == -1)
	    i = j; /* remember first empty entry */
    }

    if (i == -1)
    {
	if (p_csverbose)
	    (void)EMSG(_("E569: maximum number of cscope connections reached"));
	return -1;
    }

    if ((csinfo[i].fname = (char *)alloc((unsigned)strlen(fname)+1)) == NULL)
	return -1;

    (void)strcpy(csinfo[i].fname, (const char *)fname);

    if (ppath != NULL)
    {
	if ((csinfo[i].ppath = (char *)alloc((unsigned)strlen(ppath) + 1)) == NULL)
	{
	    vim_free(csinfo[i].fname);
	    csinfo[i].fname = NULL;
	    return -1;
	}
	(void)strcpy(csinfo[i].ppath, (const char *)ppath);
    } else
	csinfo[i].ppath = NULL;

    if (flags != NULL)
    {
	if ((csinfo[i].flags = (char *)alloc((unsigned)strlen(flags) + 1)) == NULL)
	{
	    vim_free(csinfo[i].fname);
	    vim_free(csinfo[i].ppath);
	    csinfo[i].fname = NULL;
	    csinfo[i].ppath = NULL;
	    return -1;
	}
	(void)strcpy(csinfo[i].flags, (const char *)flags);
    } else
	csinfo[i].flags = NULL;

#if defined(UNIX)
    csinfo[i].st_dev = sb->st_dev;
    csinfo[i].st_ino = sb->st_ino;

#else
    csinfo[i].nVolume = bhfi.dwVolumeSerialNumber;
    csinfo[i].nIndexLow = bhfi.nFileIndexLow;
    csinfo[i].nIndexHigh = bhfi.nFileIndexHigh;
#endif
    return i;
} /* cs_insert_filelist */


/*
 * PRIVATE: cs_lookup_cmd
 *
 * find cscope command in command table
 */
    static cscmd_T *
cs_lookup_cmd(eap)
    exarg_T *eap;
{
    cscmd_T *cmdp;
    char *stok;
    size_t len;

    if (eap->arg == NULL)
	return NULL;

    if ((stok = strtok((char *)(eap->arg), (const char *)" ")) == NULL)
	return NULL;

    len = strlen(stok);
    for (cmdp = cs_cmds; cmdp->name != NULL; ++cmdp)
    {
	if (strncmp((const char *)(stok), cmdp->name, len) == 0)
	    return (cmdp);
    }
    return NULL;
} /* cs_lookup_cmd */


/*
 * PRIVATE: cs_kill
 *
 * nuke em
 */
/* ARGSUSED */
    static int
cs_kill(eap)
    exarg_T *eap;
{
    char *stok;
    short i;

    if ((stok = strtok((char *)NULL, (const char *)" ")) == NULL)
    {
	cs_usage_msg(Kill);
	return CSCOPE_FAILURE;
    }

    /* only single digit positive and negative integers are allowed */
    if ((strlen(stok) < 2 && VIM_ISDIGIT((int)(stok[0])))
	    || (strlen(stok) < 3 && stok[0] == '-'
					      && VIM_ISDIGIT((int)(stok[1]))))
	i = atoi(stok);
    else
    {
	/* It must be part of a name.  We will try to find a match
	 * within all the names in the csinfo data structure
	 */
	for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	{
	    if (csinfo[i].fname != NULL && strstr(csinfo[i].fname, stok))
		break;
	}
    }

    if ((i >= CSCOPE_MAX_CONNECTIONS || i < -1 || csinfo[i].fname == NULL)
	    && i != -1)
    {
	if (p_csverbose)
	    (void)EMSG2(_("E261: cscope connection %s not found"), stok);
    }
    else
    {
	if (i == -1)
	{
	    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	    {
		if (csinfo[i].fname)
		    cs_kill_execute(i, csinfo[i].fname);
	    }
	}
	else
	    cs_kill_execute(i, stok);
    }

    return 0;
} /* cs_kill */


/*
 * PRIVATE: cs_kill_execute
 *
 * Actually kills a specific cscope connection.
 */
    static void
cs_kill_execute(i, cname)
    int i;	    /* cscope table index */
    char *cname;    /* cscope database name */
{
    if (p_csverbose)
    {
	msg_clr_eos();
	(void)smsg_attr(hl_attr(HLF_R) | MSG_HIST,
		(char_u *)_("cscope connection %s closed"), cname);
    }
    cs_release_csp(i, TRUE);
}


/*
 * PRIVATE: cs_make_vim_style_matches
 *
 * convert the cscope output into into a ctags style entry (as might be found
 * in a ctags tags file).  there's one catch though: cscope doesn't tell you
 * the type of the tag you are looking for.  for example, in Darren Hiebert's
 * ctags (the one that comes with vim), #define's use a line number to find the
 * tag in a file while function definitions use a regexp search pattern.
 *
 * i'm going to always use the line number because cscope does something
 * quirky (and probably other things i don't know about):
 *
 *     if you have "#  define" in your source file, which is
 *     perfectly legal, cscope thinks you have "#define".  this
 *     will result in a failed regexp search. :(
 *
 * besides, even if this particular case didn't happen, the search pattern
 * would still have to be modified to escape all the special regular expression
 * characters to comply with ctags formatting.
 */
    static char *
cs_make_vim_style_matches(fname, slno, search, tagstr)
    char *fname;
    char *slno;
    char *search;
    char *tagstr;
{
    /* vim style is ctags:
     *
     *	    <tagstr>\t<filename>\t<linenum_or_search>"\t<extra>
     *
     * but as mentioned above, we'll always use the line number and
     * put the search pattern (if one exists) as "extra"
     *
     * buf is used as part of vim's method of handling tags, and
     * (i think) vim frees it when you pop your tags and get replaced
     * by new ones on the tag stack.
     */
    char *buf;
    int amt;

    if (search != NULL)
    {
	amt = (int)(strlen(fname) + strlen(slno) + strlen(tagstr) + strlen(search)+6);
	if ((buf = (char *)alloc(amt)) == NULL)
	    return NULL;

	(void)sprintf(buf, "%s\t%s\t%s;\"\t%s", tagstr, fname, slno, search);
    }
    else
    {
	amt = (int)(strlen(fname) + strlen(slno) + strlen(tagstr) + 5);
	if ((buf = (char *)alloc(amt)) == NULL)
	    return NULL;

	(void)sprintf(buf, "%s\t%s\t%s;\"", tagstr, fname, slno);
    }

    return buf;
} /* cs_make_vim_style_matches */


/*
 * PRIVATE: cs_manage_matches
 *
 * this is kind of hokey, but i don't see an easy way round this..
 *
 * Store: keep a ptr to the (malloc'd) memory of matches originally
 * generated from cs_find().  the matches are originally lines directly
 * from cscope output, but transformed to look like something out of a
 * ctags.  see cs_make_vim_style_matches for more details.
 *
 * Get: used only from cs_fgets(), this simulates a vim_fgets() to return
 * the next line from the cscope output.  it basically keeps track of which
 * lines have been "used" and returns the next one.
 *
 * Free: frees up everything and resets
 *
 * Print: prints the tags
 */
    static char *
cs_manage_matches(matches, contexts, totmatches, cmd)
    char **matches;
    char **contexts;
    int totmatches;
    mcmd_e cmd;
{
    static char **mp = NULL;
    static char **cp = NULL;
    static int cnt = -1;
    static int next = -1;
    char *p = NULL;

    switch (cmd)
    {
    case Store:
	assert(matches != NULL);
	assert(totmatches > 0);
	if (mp != NULL || cp != NULL)
	    (void)cs_manage_matches(NULL, NULL, -1, Free);
	mp = matches;
	cp = contexts;
	cnt = totmatches;
	next = 0;
	break;
    case Get:
	if (next >= cnt)
	    return NULL;

	p = mp[next];
	next++;
	break;
    case Free:
	if (mp != NULL)
	{
	    if (cnt > 0)
		while (cnt--)
		{
		    vim_free(mp[cnt]);
		    if (cp != NULL)
			vim_free(cp[cnt]);
		}
	    vim_free(mp);
	    vim_free(cp);
	}
	mp = NULL;
	cp = NULL;
	cnt = 0;
	next = 0;
	break;
    case Print:
	cs_print_tags_priv(mp, cp, cnt);
	break;
    default:	/* should not reach here */
	(void)EMSG(_("E570: fatal error in cs_manage_matches"));
	return NULL;
    }

    return p;
} /* cs_manage_matches */


/*
 * PRIVATE: cs_parse_results
 *
 * parse cscope output
 */
    static char *
cs_parse_results(cnumber, buf, bufsize, context, linenumber, search)
    int cnumber;
    char *buf;
    int bufsize;
    char **context;
    char **linenumber;
    char **search;
{
    int ch;
    char *p;
    char *name;

    if (fgets(buf, bufsize, csinfo[cnumber].fr_fp) == NULL)
    {
	if (feof(csinfo[cnumber].fr_fp))
	    errno = EIO;

	cs_reading_emsg(cnumber);

	return NULL;
    }

	/* If the line's too long for the buffer, discard it. */
    if ((p = strchr(buf, '\n')) == NULL)
    {
	while ((ch = getc(csinfo[cnumber].fr_fp)) != EOF && ch != '\n')
	    ;
	return NULL;
    }
    *p = '\0';

    /*
     * cscope output is in the following format:
     *
     *	<filename> <context> <line number> <pattern>
     */
    if ((name = strtok((char *)buf, (const char *)" ")) == NULL)
	return NULL;
    if ((*context = strtok(NULL, (const char *)" ")) == NULL)
	return NULL;
    if ((*linenumber = strtok(NULL, (const char *)" ")) == NULL)
	return NULL;
    *search = *linenumber + strlen(*linenumber) + 1;	/* +1 to skip \0 */

    /* --- nvi ---
     * If the file is older than the cscope database, that is,
     * the database was built since the file was last modified,
     * or there wasn't a search string, use the line number.
     */
    if (strcmp(*search, "<unknown>") == 0)
	*search = NULL;

    name = cs_resolve_file(cnumber, name);
    return name;
}

#ifdef FEAT_QUICKFIX
/*
 * PRIVATE: cs_file_results
 *
 * write cscope find results to file
 */
    static void
cs_file_results(f, nummatches_a)
    FILE *f;
    int *nummatches_a;
{
    int i, j;
    char *buf;
    char *search, *slno;
    char *fullname;
    char *cntx;
    char *context;

    buf = (char *)alloc(CSREAD_BUFSIZE);
    if (buf == NULL)
	return;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (nummatches_a[i] < 1)
	    continue;

	for (j = 0; j < nummatches_a[i]; j++)
	{
	   if ((fullname = cs_parse_results(i, buf, CSREAD_BUFSIZE, &cntx,
			   &slno, &search)) == NULL)
	       continue;

	   context = (char *)alloc((unsigned)strlen(cntx)+5);
	   if (context==NULL)
	       continue;

	   if (strcmp(cntx, "<global>")==0)
	       strcpy(context, "<<global>>");
	   else
	       sprintf(context, "<<%s>>", cntx);

	   if (search==NULL)
	       fprintf(f, "%s\t%s\t%s\n", fullname, slno, context);
	   else
	       fprintf(f, "%s\t%s\t%s %s\n", fullname, slno, context, search);

	   vim_free(context);
	   vim_free(fullname);
	} /* for all matches */

	(void)cs_read_prompt(i);

    } /* for all cscope connections */
    vim_free(buf);
}
#endif

/*
 * PRIVATE: cs_fill_results
 *
 * get parsed cscope output and calls cs_make_vim_style_matches to convert
 * into ctags format
 * When there are no matches sets "*matches_p" to NULL.
 */
    static void
cs_fill_results(tagstr, totmatches, nummatches_a, matches_p, cntxts_p, matched)
    char *tagstr;
    int totmatches;
    int *nummatches_a;
    char ***matches_p;
    char ***cntxts_p;
    int *matched;
{
    int i, j;
    char *buf;
    char *search, *slno;
    int totsofar = 0;
    char **matches = NULL;
    char **cntxts = NULL;
    char *fullname;
    char *cntx;

    assert(totmatches > 0);

    buf = (char *)alloc(CSREAD_BUFSIZE);
    if (buf == NULL)
	return;

    if ((matches = (char **)alloc(sizeof(char *) * totmatches)) == NULL)
	goto parse_out;
    if ((cntxts = (char **)alloc(sizeof(char *) * totmatches)) == NULL)
	goto parse_out;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (nummatches_a[i] < 1)
	    continue;

	for (j = 0; j < nummatches_a[i]; j++)
	{
	   if ((fullname = cs_parse_results(i, buf, CSREAD_BUFSIZE, &cntx,
			   &slno, &search)) == NULL)
		continue;

	    matches[totsofar] = cs_make_vim_style_matches(fullname, slno,
							  search, tagstr);

	    vim_free(fullname);

	    if (strcmp(cntx, "<global>") == 0)
		cntxts[totsofar] = NULL;
	    else
		/* note: if vim_strsave returns NULL, then the context
		 * will be "<global>", which is misleading.
		 */
		cntxts[totsofar] = (char *)vim_strsave((char_u *)cntx);

	    if (matches[totsofar] != NULL)
		totsofar++;

	} /* for all matches */

	(void)cs_read_prompt(i);

    } /* for all cscope connections */

parse_out:
    if (totsofar == 0)
    {
	/* No matches, free the arrays and return NULL in "*matches_p". */
	vim_free(matches);
	matches = NULL;
	vim_free(cntxts);
	cntxts = NULL;
    }
    *matched = totsofar;
    *matches_p = matches;
    *cntxts_p = cntxts;

    vim_free(buf);
} /* cs_fill_results */


/* get the requested path components */
    static char *
cs_pathcomponents(path)
    char	*path;
{
    int		i;
    char	*s;

    if (p_cspc == 0)
	return path;

    s = path + strlen(path) - 1;
    for (i = 0; i < p_cspc; ++i)
	while (s > path && *--s != '/'
#ifdef WIN32
		&& *--s != '\\'
#endif
		)
	    ;
    if ((s > path && *s == '/')
#ifdef WIN32
	|| (s > path && *s == '\\')
#endif
	    )
	++s;
    return s;
}

/*
 * PRIVATE: cs_print_tags_priv
 *
 * called from cs_manage_matches()
 */
    static void
cs_print_tags_priv(matches, cntxts, num_matches)
    char **matches;
    char **cntxts;
    int num_matches;
{
    char	*buf = NULL;
    int		bufsize = 0; /* Track available bufsize */
    int		newsize = 0;
    char	*ptag;
    char	*fname, *lno, *extra, *tbuf;
    int		i, idx, num;
    char	*globalcntx = "GLOBAL";
    char	*cntxformat = " <<%s>>";
    char	*context;
    char	*cstag_msg = _("Cscope tag: %s");
    char	*csfmt_str = "%4d %6s  ";

    assert (num_matches > 0);

    if ((tbuf = (char *)alloc((unsigned)strlen(matches[0]) + 1)) == NULL)
	return;

    strcpy(tbuf, matches[0]);
    ptag = strtok(tbuf, "\t");

    newsize = (int)(strlen(cstag_msg) + strlen(ptag));
    buf = (char *)alloc(newsize);
    if (buf != NULL)
    {
	bufsize = newsize;
	(void)sprintf(buf, cstag_msg, ptag);
	MSG_PUTS_ATTR(buf, hl_attr(HLF_T));
    }

    vim_free(tbuf);

    MSG_PUTS_ATTR(_("\n   #   line"), hl_attr(HLF_T));    /* strlen is 7 */
    msg_advance(msg_col + 2);
    MSG_PUTS_ATTR(_("filename / context / line\n"), hl_attr(HLF_T));

    num = 1;
    for (i = 0; i < num_matches; i++)
    {
	idx = i;

	/* if we really wanted to, we could avoid this malloc and strcpy
	 * by parsing matches[i] on the fly and placing stuff into buf
	 * directly, but that's too much of a hassle
	 */
	if ((tbuf = (char *)alloc((unsigned)strlen(matches[idx]) + 1)) == NULL)
	    continue;
	(void)strcpy(tbuf, matches[idx]);

	if ((fname = strtok(tbuf, (const char *)"\t")) == NULL)
	    continue;
	if ((fname = strtok(NULL, (const char *)"\t")) == NULL)
	    continue;
	if ((lno = strtok(NULL, (const char *)"\t")) == NULL)
	{
	    /* if NULL, then no "extra", although in cscope's case, there
	     * should always be "extra".
	     */
	    extra = NULL;
	}

	extra = lno + strlen(lno) + 1;

	lno[strlen(lno)-2] = '\0';  /* ignore ;" at the end */

	/* hopefully 'num' (num of matches) will be less than 10^16 */
	newsize = (int)(strlen(csfmt_str) + 16 + strlen(lno));
	if (bufsize < newsize)
	{
	    buf = (char *)vim_realloc(buf, newsize);
	    if (buf == NULL)
		bufsize = 0;
	    else
		bufsize = newsize;
	}
	if (buf != NULL)
	{
	    /* csfmt_str = "%4d %6s  "; */
	    (void)sprintf(buf, csfmt_str, num, lno);
	    MSG_PUTS_ATTR(buf, hl_attr(HLF_CM));
	}
	MSG_PUTS_LONG_ATTR(cs_pathcomponents(fname), hl_attr(HLF_CM));

	/* compute the required space for the context */
	if (cntxts[idx] != NULL)
	    context = cntxts[idx];
	else
	    context = globalcntx;
	newsize = (int)(strlen(context) + strlen(cntxformat));

	if (bufsize < newsize)
	{
	    buf = (char *)vim_realloc(buf, newsize);
	    if (buf == NULL)
		bufsize = 0;
	    else
		bufsize = newsize;
	}
	if (buf != NULL)
	{
	    (void)sprintf(buf, cntxformat, context);

	    /* print the context only if it fits on the same line */
	    if (msg_col + (int)strlen(buf) >= (int)Columns)
		msg_putchar('\n');
	    msg_advance(12);
	    MSG_PUTS_LONG(buf);
	    msg_putchar('\n');
	}
	if (extra != NULL)
	{
	    msg_advance(13);
	    MSG_PUTS_LONG(extra);
	}

	vim_free(tbuf); /* only after printing extra due to strtok use */

	if (msg_col)
	    msg_putchar('\n');

	ui_breakcheck();
	if (got_int)
	{
	    got_int = FALSE;	/* don't print any more matches */
	    break;
	}

	num++;
    } /* for all matches */

    vim_free(buf);
} /* cs_print_tags_priv */


/*
 * PRIVATE: cs_read_prompt
 *
 * read a cscope prompt (basically, skip over the ">> ")
 */
    static int
cs_read_prompt(i)
    int i;
{
    int		ch;
    char	*buf = NULL; /* buffer for possible error message from cscope */
    int		bufpos = 0;
    char	*cs_emsg;
    int		maxlen;
    static char *eprompt = "Press the RETURN key to continue:";
    int		epromptlen = (int)strlen(eprompt);
    int		n;

    cs_emsg = _("E609: Cscope error: %s");
    /* compute maximum allowed len for Cscope error message */
    maxlen = (int)(IOSIZE - strlen(cs_emsg));

    for (;;)
    {
	while ((ch = getc(csinfo[i].fr_fp)) != EOF && ch != CSCOPE_PROMPT[0])
	    /* if there is room and char is printable */
	    if (bufpos < maxlen - 1 && vim_isprintc(ch))
	    {
		if (buf == NULL) /* lazy buffer allocation */
		    buf = (char *)alloc(maxlen);
		if (buf != NULL)
		{
		    /* append character to the message */
		    buf[bufpos++] = ch;
		    buf[bufpos] = NUL;
		    if (bufpos >= epromptlen
			    && strcmp(&buf[bufpos - epromptlen], eprompt) == 0)
		    {
			/* remove eprompt from buf */
			buf[bufpos - epromptlen] = NUL;

			/* print message to user */
			(void)EMSG2(cs_emsg, buf);

			/* send RETURN to cscope */
			(void)putc('\n', csinfo[i].to_fp);
			(void)fflush(csinfo[i].to_fp);

			/* clear buf */
			bufpos = 0;
			buf[bufpos] = NUL;
		    }
		}
	    }

	for (n = 0; n < (int)strlen(CSCOPE_PROMPT); ++n)
	{
	    if (n > 0)
		ch = getc(csinfo[i].fr_fp);
	    if (ch == EOF)
	    {
		PERROR("cs_read_prompt EOF");
		if (buf != NULL && buf[0] != NUL)
		    (void)EMSG2(cs_emsg, buf);
		else if (p_csverbose)
		    cs_reading_emsg(i); /* don't have additional information */
		cs_release_csp(i, TRUE);
		vim_free(buf);
		return CSCOPE_FAILURE;
	    }

	    if (ch != CSCOPE_PROMPT[n])
	    {
		ch = EOF;
		break;
	    }
	}

	if (ch == EOF)
	    continue;	    /* didn't find the prompt */
	break;		    /* did find the prompt */
    }

    vim_free(buf);
    return CSCOPE_SUCCESS;
}


/*
 * PRIVATE: cs_release_csp
 *
 * does the actual free'ing for the cs ptr with an optional flag of whether
 * or not to free the filename.  called by cs_kill and cs_reset.
 */
    static void
cs_release_csp(i, freefnpp)
    int i;
    int freefnpp;
{
#if defined(UNIX)
    int pstat;
#else
    /*
     * Trying to exit normally (not sure whether it is fit to UNIX cscope
     */
    if (csinfo[i].to_fp != NULL)
    {
	(void)fputs("q\n", csinfo[i].to_fp);
	(void)fflush(csinfo[i].to_fp);
    }
    /* give cscope chance to exit normally */
    if (csinfo[i].hProc != NULL
	    && WaitForSingleObject(csinfo[i].hProc, 1000) == WAIT_TIMEOUT)
	TerminateProcess(csinfo[i].hProc, 0);
#endif

    if (csinfo[i].fr_fp != NULL)
	(void)fclose(csinfo[i].fr_fp);
    if (csinfo[i].to_fp != NULL)
	(void)fclose(csinfo[i].to_fp);

    /*
     * Safety check: If the PID would be zero here, the entire X session would
     * be killed.  -1 and 1 are dangerous as well.
     */
#if defined(UNIX)
    if (csinfo[i].pid > 1)
    {
	kill(csinfo[i].pid, SIGTERM);
	(void)waitpid(csinfo[i].pid, &pstat, 0);
    }
#endif

    if (freefnpp)
    {
	vim_free(csinfo[i].fname);
	vim_free(csinfo[i].ppath);
	vim_free(csinfo[i].flags);
    }

    clear_csinfo(i);
} /* cs_release_csp */


/*
 * PRIVATE: cs_reset
 *
 * calls cs_kill on all cscope connections then reinits
 */
/* ARGSUSED */
    static int
cs_reset(eap)
    exarg_T *eap;
{
    char	**dblist = NULL, **pplist = NULL, **fllist = NULL;
    int	i;
    char buf[20]; /* for sprintf " (#%d)" */

    /* malloc our db and ppath list */
    dblist = (char **)alloc(CSCOPE_MAX_CONNECTIONS * sizeof(char *));
    pplist = (char **)alloc(CSCOPE_MAX_CONNECTIONS * sizeof(char *));
    fllist = (char **)alloc(CSCOPE_MAX_CONNECTIONS * sizeof(char *));
    if (dblist == NULL || pplist == NULL || fllist == NULL)
    {
	vim_free(dblist);
	vim_free(pplist);
	vim_free(fllist);
	return CSCOPE_FAILURE;
    }

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	dblist[i] = csinfo[i].fname;
	pplist[i] = csinfo[i].ppath;
	fllist[i] = csinfo[i].flags;
	if (csinfo[i].fname != NULL)
	    cs_release_csp(i, FALSE);
    }

    /* rebuild the cscope connection list */
    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (dblist[i] != NULL)
	{
	    cs_add_common(dblist[i], pplist[i], fllist[i]);
	    if (p_csverbose)
	    {
		/* dont' use smsg_attr because want to display
		 * connection number in the same line as
		 * "Added cscope database..."
		 */
		sprintf(buf, " (#%d)", i);
		MSG_PUTS_ATTR(buf, hl_attr(HLF_R));
	    }
	}
	vim_free(dblist[i]);
	vim_free(pplist[i]);
	vim_free(fllist[i]);
    }
    vim_free(dblist);
    vim_free(pplist);
    vim_free(fllist);

    if (p_csverbose)
	MSG_ATTR(_("All cscope databases reset"), hl_attr(HLF_R) | MSG_HIST);
    return CSCOPE_SUCCESS;
} /* cs_reset */


/*
 * PRIVATE: cs_resolve_file
 *
 * construct the full pathname to a file found in the cscope database.
 * (Prepends ppath, if there is one and if it's not already prepended,
 * otherwise just uses the name found.)
 *
 * we need to prepend the prefix because on some cscope's (e.g., the one that
 * ships with Solaris 2.6), the output never has the prefix prepended.
 * contrast this with my development system (Digital Unix), which does.
 */
    static char *
cs_resolve_file(i, name)
    int i;
    char *name;
{
    char *fullname;
    int len;

    /*
     * ppath is freed when we destroy the cscope connection.
     * fullname is freed after cs_make_vim_style_matches, after it's been
     * copied into the tag buffer used by vim
     */
    len = (int)(strlen(name) + 2);
    if (csinfo[i].ppath != NULL)
	len += (int)strlen(csinfo[i].ppath);

    if ((fullname = (char *)alloc(len)) == NULL)
	return NULL;

    /*
     * note/example: this won't work if the cscope output already starts
     * "../.." and the prefix path is also "../..".  if something like this
     * happens, you are screwed up and need to fix how you're using cscope.
     */
    if (csinfo[i].ppath != NULL &&
	(strncmp(name, csinfo[i].ppath, strlen(csinfo[i].ppath)) != 0) &&
	(name[0] != '/')
#ifdef WIN32
	&& name[0] != '\\' && name[1] != ':'
#endif
	)
	(void)sprintf(fullname, "%s/%s", csinfo[i].ppath, name);
    else
	(void)sprintf(fullname, "%s", name);

    return fullname;
} /* cs_resolve_file */


/*
 * PRIVATE: cs_show
 *
 * show all cscope connections
 */
/* ARGSUSED */
    static int
cs_show(eap)
    exarg_T *eap;
{
    short i;
    if (cs_cnt_connections() == 0)
	MSG_PUTS(_("no cscope connections\n"));
    else
    {
	MSG_PUTS_ATTR(
	    _(" # pid    database name                       prepend path\n"),
	    hl_attr(HLF_T));
	for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	{
	    if (csinfo[i].fname == NULL)
		continue;

	    if (csinfo[i].ppath != NULL)
		(void)smsg((char_u *)"%2d %-5ld  %-34s  %-32s",
		    i, (long)csinfo[i].pid, csinfo[i].fname, csinfo[i].ppath);
	    else
		(void)smsg((char_u *)"%2d %-5ld  %-34s  <none>",
			   i, (long)csinfo[i].pid, csinfo[i].fname);
	}
    }

    wait_return(TRUE);
    return CSCOPE_SUCCESS;
} /* cs_show */

#endif	/* FEAT_CSCOPE */

/* the end */
