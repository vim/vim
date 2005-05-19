/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * ex_cmds2.c: some more functions for command line commands
 */

#if defined(WIN32) && defined(FEAT_CSCOPE)
# include <io.h>
#endif

#include "vim.h"

#if defined(WIN32) && defined(FEAT_CSCOPE)
# include <fcntl.h>
#endif

#include "version.h"

static void	cmd_source __ARGS((char_u *fname, exarg_T *eap));

#ifdef FEAT_EVAL
/* Growarray to store info about already sourced scripts.
 * For Unix also store the dev/ino, so that we don't have to stat() each
 * script when going through the list. */
typedef struct scriptitem_S
{
    char_u	*sn_name;
# ifdef UNIX
    int		sn_dev;
    ino_t	sn_ino;
# endif
# ifdef FEAT_PROFILE
    int		sn_prof_on;	/* TRUE when script is/was profiled */
    int		sn_pr_force;	/* forceit: profile functions in this script */
    proftime_T	sn_pr_child;	/* time set when going into first child */
    int		sn_pr_nest;	/* nesting for sn_pr_child */
    /* profiling the script as a whole */
    int		sn_pr_count;	/* nr of times sourced */
    proftime_T	sn_pr_total;	/* time spend in script + children */
    proftime_T	sn_pr_self;	/* time spend in script itself */
    proftime_T	sn_pr_start;	/* time at script start */
    proftime_T	sn_pr_children; /* time in children after script start */
    /* profiling the script per line */
    garray_T	sn_prl_ga;	/* things stored for every line */
    proftime_T	sn_prl_start;	/* start time for current line */
    proftime_T	sn_prl_children; /* time spent in children for this line */
    proftime_T	sn_prl_wait;	/* wait start time for current line */
    int		sn_prl_idx;	/* index of line being timed; -1 if none */
    int		sn_prl_execed;	/* line being timed was executed */
# endif
} scriptitem_T;

static garray_T script_items = {0, 0, sizeof(scriptitem_T), 4, NULL};
#define SCRIPT_ITEM(id) (((scriptitem_T *)script_items.ga_data)[(id) - 1])

# ifdef FEAT_PROFILE
/* Struct used in sn_prl_ga for every line of a script. */
typedef struct sn_prl_S
{
    int		snp_count;	/* nr of times line was executed */
    proftime_T	sn_prl_total;	/* time spend in a line + children */
    proftime_T	sn_prl_self;	/* time spend in a line itself */
} sn_prl_T;

#  define PRL_ITEM(si, idx)	(((sn_prl_T *)(si)->sn_prl_ga.ga_data)[(idx)])
# endif
#endif

#if defined(FEAT_EVAL) || defined(PROTO)
static int debug_greedy = FALSE;	/* batch mode debugging: don't save
					   and restore typeahead. */

/*
 * do_debug(): Debug mode.
 * Repeatedly get Ex commands, until told to continue normal execution.
 */
    void
do_debug(cmd)
    char_u	*cmd;
{
    int		save_msg_scroll = msg_scroll;
    int		save_State = State;
    int		save_did_emsg = did_emsg;
    int		save_cmd_silent = cmd_silent;
    int		save_msg_silent = msg_silent;
    int		save_emsg_silent = emsg_silent;
    int		save_redir_off = redir_off;
    tasave_T	typeaheadbuf;
# ifdef FEAT_EX_EXTRA
    int		save_ex_normal_busy;
# endif
    int		n;
    char_u	*cmdline = NULL;
    char_u	*p;
    char	*tail = NULL;
    static int	last_cmd = 0;
#define CMD_CONT	1
#define CMD_NEXT	2
#define CMD_STEP	3
#define CMD_FINISH	4
#define CMD_QUIT	5
#define CMD_INTERRUPT	6

#ifdef ALWAYS_USE_GUI
    /* Can't do this when there is no terminal for input/output. */
    if (!gui.in_use)
    {
	/* Break as soon as possible. */
	debug_break_level = 9999;
	return;
    }
#endif

    /* Make sure we are in raw mode and start termcap mode.  Might have side
     * effects... */
    settmode(TMODE_RAW);
    starttermcap();

    ++RedrawingDisabled;	/* don't redisplay the window */
    ++no_wait_return;		/* don't wait for return */
    did_emsg = FALSE;		/* don't use error from debugged stuff */
    cmd_silent = FALSE;		/* display commands */
    msg_silent = FALSE;		/* display messages */
    emsg_silent = FALSE;	/* display error messages */
    redir_off = TRUE;		/* don't redirect debug commands */

    State = NORMAL;
#ifdef FEAT_SNIFF
    want_sniff_request = 0;    /* No K_SNIFF wanted */
#endif

    if (!debug_did_msg)
	MSG(_("Entering Debug mode.  Type \"cont\" to continue."));
    if (sourcing_name != NULL)
	msg(sourcing_name);
    if (sourcing_lnum != 0)
	smsg((char_u *)_("line %ld: %s"), (long)sourcing_lnum, cmd);
    else
	smsg((char_u *)_("cmd: %s"), cmd);

    /*
     * Repeat getting a command and executing it.
     */
    for (;;)
    {
	msg_scroll = TRUE;
	need_wait_return = FALSE;
#ifdef FEAT_SNIFF
	ProcessSniffRequests();
#endif
	/* Save the current typeahead buffer and replace it with an empty one.
	 * This makes sure we get input from the user here and don't interfere
	 * with the commands being executed.  Reset "ex_normal_busy" to avoid
	 * the side effects of using ":normal". Save the stuff buffer and make
	 * it empty. */
# ifdef FEAT_EX_EXTRA
	save_ex_normal_busy = ex_normal_busy;
	ex_normal_busy = 0;
# endif
	if (!debug_greedy)
	    save_typeahead(&typeaheadbuf);

	cmdline = getcmdline_prompt('>', NULL, 0);

	if (!debug_greedy)
	    restore_typeahead(&typeaheadbuf);
# ifdef FEAT_EX_EXTRA
	ex_normal_busy = save_ex_normal_busy;
# endif

	cmdline_row = msg_row;
	if (cmdline != NULL)
	{
	    /* If this is a debug command, set "last_cmd".
	     * If not, reset "last_cmd".
	     * For a blank line use previous command. */
	    p = skipwhite(cmdline);
	    if (*p != NUL)
	    {
		switch (*p)
		{
		    case 'c': last_cmd = CMD_CONT;
			      tail = "ont";
			      break;
		    case 'n': last_cmd = CMD_NEXT;
			      tail = "ext";
			      break;
		    case 's': last_cmd = CMD_STEP;
			      tail = "tep";
			      break;
		    case 'f': last_cmd = CMD_FINISH;
			      tail = "inish";
			      break;
		    case 'q': last_cmd = CMD_QUIT;
			      tail = "uit";
			      break;
		    case 'i': last_cmd = CMD_INTERRUPT;
			      tail = "nterrupt";
			      break;
		    default: last_cmd = 0;
		}
		if (last_cmd != 0)
		{
		    /* Check that the tail matches. */
		    ++p;
		    while (*p != NUL && *p == *tail)
		    {
			++p;
			++tail;
		    }
		    if (ASCII_ISALPHA(*p))
			last_cmd = 0;
		}
	    }

	    if (last_cmd != 0)
	    {
		/* Execute debug command: decided where to break next and
		 * return. */
		switch (last_cmd)
		{
		    case CMD_CONT:
			debug_break_level = -1;
			break;
		    case CMD_NEXT:
			debug_break_level = ex_nesting_level;
			break;
		    case CMD_STEP:
			debug_break_level = 9999;
			break;
		    case CMD_FINISH:
			debug_break_level = ex_nesting_level - 1;
			break;
		    case CMD_QUIT:
			got_int = TRUE;
			debug_break_level = -1;
			break;
		    case CMD_INTERRUPT:
			got_int = TRUE;
			debug_break_level = 9999;
			/* Do not repeat ">interrupt" cmd, continue stepping. */
			last_cmd = CMD_STEP;
			break;
		}
		break;
	    }

	    /* don't debug this command */
	    n = debug_break_level;
	    debug_break_level = -1;
	    (void)do_cmdline(cmdline, getexline, NULL,
						DOCMD_VERBOSE|DOCMD_EXCRESET);
	    debug_break_level = n;

	    vim_free(cmdline);
	}
	lines_left = Rows - 1;
    }
    vim_free(cmdline);

    --RedrawingDisabled;
    --no_wait_return;
    redraw_all_later(NOT_VALID);
    need_wait_return = FALSE;
    msg_scroll = save_msg_scroll;
    lines_left = Rows - 1;
    State = save_State;
    did_emsg = save_did_emsg;
    cmd_silent = save_cmd_silent;
    msg_silent = save_msg_silent;
    emsg_silent = save_emsg_silent;
    redir_off = save_redir_off;

    /* Only print the message again when typing a command before coming back
     * here. */
    debug_did_msg = TRUE;
}

/*
 * ":debug".
 */
    void
ex_debug(eap)
    exarg_T	*eap;
{
    int		debug_break_level_save = debug_break_level;

    debug_break_level = 9999;
    do_cmdline_cmd(eap->arg);
    debug_break_level = debug_break_level_save;
}

static char_u	*debug_breakpoint_name = NULL;
static linenr_T	debug_breakpoint_lnum;

/*
 * When debugging or a breakpoint is set on a skipped command, no debug prompt
 * is shown by do_one_cmd().  This situation is indicated by debug_skipped, and
 * debug_skipped_name is then set to the source name in the breakpoint case.  If
 * a skipped command decides itself that a debug prompt should be displayed, it
 * can do so by calling dbg_check_skipped().
 */
static int	debug_skipped;
static char_u	*debug_skipped_name;

/*
 * Go to debug mode when a breakpoint was encountered or "ex_nesting_level" is
 * at or below the break level.  But only when the line is actually
 * executed.  Return TRUE and set breakpoint_name for skipped commands that
 * decide to execute something themselves.
 * Called from do_one_cmd() before executing a command.
 */
    void
dbg_check_breakpoint(eap)
    exarg_T	*eap;
{
    char_u	*p;

    debug_skipped = FALSE;
    if (debug_breakpoint_name != NULL)
    {
	if (!eap->skip)
	{
	    /* replace K_SNR with "<SNR>" */
	    if (debug_breakpoint_name[0] == K_SPECIAL
		    && debug_breakpoint_name[1] == KS_EXTRA
		    && debug_breakpoint_name[2] == (int)KE_SNR)
		p = (char_u *)"<SNR>";
	    else
		p = (char_u *)"";
	    smsg((char_u *)_("Breakpoint in \"%s%s\" line %ld"),
		    p,
		    debug_breakpoint_name + (*p == NUL ? 0 : 3),
		    (long)debug_breakpoint_lnum);
	    debug_breakpoint_name = NULL;
	    do_debug(eap->cmd);
	}
	else
	{
	    debug_skipped = TRUE;
	    debug_skipped_name = debug_breakpoint_name;
	    debug_breakpoint_name = NULL;
	}
    }
    else if (ex_nesting_level <= debug_break_level)
    {
	if (!eap->skip)
	    do_debug(eap->cmd);
	else
	{
	    debug_skipped = TRUE;
	    debug_skipped_name = NULL;
	}
    }
}

/*
 * Go to debug mode if skipped by dbg_check_breakpoint() because eap->skip was
 * set.  Return TRUE when the debug mode is entered this time.
 */
    int
dbg_check_skipped(eap)
    exarg_T	*eap;
{
    int		prev_got_int;

    if (debug_skipped)
    {
	/*
	 * Save the value of got_int and reset it.  We don't want a previous
	 * interruption cause flushing the input buffer.
	 */
	prev_got_int = got_int;
	got_int = FALSE;
	debug_breakpoint_name = debug_skipped_name;
	/* eap->skip is TRUE */
	eap->skip = FALSE;
	(void)dbg_check_breakpoint(eap);
	eap->skip = TRUE;
	got_int |= prev_got_int;
	return TRUE;
    }
    return FALSE;
}

/*
 * The list of breakpoints: dbg_breakp.
 * This is a grow-array of structs.
 */
struct debuggy
{
    int		dbg_nr;		/* breakpoint number */
    int		dbg_type;	/* DBG_FUNC or DBG_FILE */
    char_u	*dbg_name;	/* function or file name */
    regprog_T	*dbg_prog;	/* regexp program */
    linenr_T	dbg_lnum;	/* line number in function or file */
    int		dbg_forceit;	/* ! used */
};

static garray_T dbg_breakp = {0, 0, sizeof(struct debuggy), 4, NULL};
#define BREAKP(idx)		(((struct debuggy *)dbg_breakp.ga_data)[idx])
#define DEBUGGY(gap, idx)	(((struct debuggy *)gap->ga_data)[idx])
static int last_breakp = 0;	/* nr of last defined breakpoint */

#ifdef FEAT_PROFILE
/* Profiling uses file and func names similar to breakpoints. */
static garray_T prof_ga = {0, 0, sizeof(struct debuggy), 4, NULL};
#endif
#define DBG_FUNC	1
#define DBG_FILE	2

static int dbg_parsearg __ARGS((char_u *arg, garray_T *gap));
static linenr_T debuggy_find __ARGS((int file,char_u *fname, linenr_T after, garray_T *gap, int *fp));

/*
 * Parse the arguments of ":profile", ":breakadd" or ":breakdel" and put them
 * in the entry just after the last one in dbg_breakp.  Note that "dbg_name"
 * is allocated.
 * Returns FAIL for failure.
 */
    static int
dbg_parsearg(arg, gap)
    char_u	*arg;
    garray_T	*gap;	    /* either &dbg_breakp or &prof_ga */
{
    char_u	*p = arg;
    char_u	*q;
    struct debuggy *bp;
    int		here = FALSE;

    if (ga_grow(gap, 1) == FAIL)
	return FAIL;
    bp = &DEBUGGY(gap, gap->ga_len);

    /* Find "func" or "file". */
    if (STRNCMP(p, "func", 4) == 0)
	bp->dbg_type = DBG_FUNC;
    else if (STRNCMP(p, "file", 4) == 0)
	bp->dbg_type = DBG_FILE;
    else if (
#ifdef FEAT_PROFILE
	    gap != &prof_ga &&
#endif
	    STRNCMP(p, "here", 4) == 0)
    {
	if (curbuf->b_ffname == NULL)
	{
	    EMSG(_(e_noname));
	    return FAIL;
	}
	bp->dbg_type = DBG_FILE;
	here = TRUE;
    }
    else
    {
	EMSG2(_(e_invarg2), p);
	return FAIL;
    }
    p = skipwhite(p + 4);

    /* Find optional line number. */
    if (here)
	bp->dbg_lnum = curwin->w_cursor.lnum;
    else if (
#ifdef FEAT_PROFILE
	    gap != &prof_ga &&
#endif
	    VIM_ISDIGIT(*p))
    {
	bp->dbg_lnum = getdigits(&p);
	p = skipwhite(p);
    }
    else
	bp->dbg_lnum = 0;

    /* Find the function or file name.  Don't accept a function name with (). */
    if ((!here && *p == NUL)
	    || (here && *p != NUL)
	    || (bp->dbg_type == DBG_FUNC && strstr((char *)p, "()") != NULL))
    {
	EMSG2(_(e_invarg2), arg);
	return FAIL;
    }

    if (bp->dbg_type == DBG_FUNC)
	bp->dbg_name = vim_strsave(p);
    else if (here)
	bp->dbg_name = vim_strsave(curbuf->b_ffname);
    else
    {
	/* Expand the file name in the same way as do_source().  This means
	 * doing it twice, so that $DIR/file gets expanded when $DIR is
	 * "~/dir". */
#ifdef RISCOS
	q = mch_munge_fname(p);
#else
	q = expand_env_save(p);
#endif
	if (q == NULL)
	    return FAIL;
#ifdef RISCOS
	p = mch_munge_fname(q);
#else
	p = expand_env_save(q);
#endif
	vim_free(q);
	if (p == NULL)
	    return FAIL;
	if (*p != '*')
	{
	    bp->dbg_name = fix_fname(p);
	    vim_free(p);
	}
	else
	    bp->dbg_name = p;
#ifdef MACOS_CLASSIC
	if (bp->dbg_name != NULL)
	    slash_n_colon_adjust(bp->dbg_name);
#endif
    }

    if (bp->dbg_name == NULL)
	return FAIL;
    return OK;
}

/*
 * ":breakadd".
 */
    void
ex_breakadd(eap)
    exarg_T	*eap;
{
    struct debuggy *bp;
    char_u	*pat;
    garray_T	*gap;

    gap = &dbg_breakp;
#ifdef FEAT_PROFILE
    if (eap->cmdidx == CMD_profile)
	gap = &prof_ga;
#endif

    if (dbg_parsearg(eap->arg, gap) == OK)
    {
	bp = &DEBUGGY(gap, gap->ga_len);
	bp->dbg_forceit = eap->forceit;

	pat = file_pat_to_reg_pat(bp->dbg_name, NULL, NULL, FALSE);
	if (pat != NULL)
	{
	    bp->dbg_prog = vim_regcomp(pat, RE_MAGIC + RE_STRING);
	    vim_free(pat);
	}
	if (pat == NULL || bp->dbg_prog == NULL)
	    vim_free(bp->dbg_name);
	else
	{
	    if (bp->dbg_lnum == 0)	/* default line number is 1 */
		bp->dbg_lnum = 1;
#ifdef FEAT_PROFILE
	    if (eap->cmdidx != CMD_profile)
#endif
	    {
		DEBUGGY(gap, gap->ga_len).dbg_nr = ++last_breakp;
		++debug_tick;
	    }
	    ++gap->ga_len;
	}
    }
}

/*
 * ":debuggreedy".
 */
    void
ex_debuggreedy(eap)
    exarg_T	*eap;
{
    if (eap->addr_count == 0 || eap->line2 != 0)
	debug_greedy = TRUE;
    else
	debug_greedy = FALSE;
}

/*
 * ":breakdel".
 */
    void
ex_breakdel(eap)
    exarg_T	*eap;
{
    struct debuggy *bp, *bpi;
    int		nr;
    int		todel = -1;
    int		i;
    linenr_T	best_lnum = 0;

    if (vim_isdigit(*eap->arg))
    {
	/* ":breakdel {nr}" */
	nr = atol((char *)eap->arg);
	for (i = 0; i < dbg_breakp.ga_len; ++i)
	    if (BREAKP(i).dbg_nr == nr)
	    {
		todel = i;
		break;
	    }
    }
    else
    {
	/* ":breakdel {func|file} [lnum] {name}" */
	if (dbg_parsearg(eap->arg, &dbg_breakp) == FAIL)
	    return;
	bp = &BREAKP(dbg_breakp.ga_len);
	for (i = 0; i < dbg_breakp.ga_len; ++i)
	{
	    bpi = &BREAKP(i);
	    if (bp->dbg_type == bpi->dbg_type
		    && STRCMP(bp->dbg_name, bpi->dbg_name) == 0
		    && (bp->dbg_lnum == bpi->dbg_lnum
			|| (bp->dbg_lnum == 0
			    && (best_lnum == 0
				|| bpi->dbg_lnum < best_lnum))))
	    {
		todel = i;
		best_lnum = bpi->dbg_lnum;
	    }
	}
	vim_free(bp->dbg_name);
    }

    if (todel < 0)
	EMSG2(_("E161: Breakpoint not found: %s"), eap->arg);
    else
    {
	vim_free(BREAKP(todel).dbg_name);
	vim_free(BREAKP(todel).dbg_prog);
	--dbg_breakp.ga_len;
	if (todel < dbg_breakp.ga_len)
	    mch_memmove(&BREAKP(todel), &BREAKP(todel + 1),
		    (dbg_breakp.ga_len - todel) * sizeof(struct debuggy));
	++debug_tick;
    }
}

/*
 * ":breaklist".
 */
/*ARGSUSED*/
    void
ex_breaklist(eap)
    exarg_T	*eap;
{
    struct debuggy *bp;
    int		i;

    if (dbg_breakp.ga_len == 0)
	MSG(_("No breakpoints defined"));
    else
	for (i = 0; i < dbg_breakp.ga_len; ++i)
	{
	    bp = &BREAKP(i);
	    smsg((char_u *)_("%3d  %s %s  line %ld"),
		    bp->dbg_nr,
		    bp->dbg_type == DBG_FUNC ? "func" : "file",
		    bp->dbg_name,
		    (long)bp->dbg_lnum);
	}
}

/*
 * Find a breakpoint for a function or sourced file.
 * Returns line number at which to break; zero when no matching breakpoint.
 */
    linenr_T
dbg_find_breakpoint(file, fname, after)
    int		file;	    /* TRUE for a file, FALSE for a function */
    char_u	*fname;	    /* file or function name */
    linenr_T	after;	    /* after this line number */
{
    return debuggy_find(file, fname, after, &dbg_breakp, NULL);
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Return TRUE if profiling is on for a function or sourced file.
 */
    int
has_profiling(file, fname, fp)
    int		file;	    /* TRUE for a file, FALSE for a function */
    char_u	*fname;	    /* file or function name */
    int		*fp;	    /* return: forceit */
{
    return (debuggy_find(file, fname, (linenr_T)0, &prof_ga, fp)
							      != (linenr_T)0);
}
#endif

/*
 * Common code for dbg_find_breakpoint() and has_profiling().
 */
    static linenr_T
debuggy_find(file, fname, after, gap, fp)
    int		file;	    /* TRUE for a file, FALSE for a function */
    char_u	*fname;	    /* file or function name */
    linenr_T	after;	    /* after this line number */
    garray_T	*gap;	    /* either &dbg_breakp or &prof_ga */
    int		*fp;	    /* if not NULL: return forceit */
{
    struct debuggy *bp;
    int		i;
    linenr_T	lnum = 0;
    regmatch_T	regmatch;
    char_u	*name = fname;
    int		prev_got_int;

    /* Return quickly when there are no breakpoints. */
    if (gap->ga_len == 0)
	return (linenr_T)0;

    /* Replace K_SNR in function name with "<SNR>". */
    if (!file && fname[0] == K_SPECIAL)
    {
	name = alloc((unsigned)STRLEN(fname) + 3);
	if (name == NULL)
	    name = fname;
	else
	{
	    STRCPY(name, "<SNR>");
	    STRCPY(name + 5, fname + 3);
	}
    }

    for (i = 0; i < gap->ga_len; ++i)
    {
	/* Skip entries that are not useful or are for a line that is beyond
	 * an already found breakpoint. */
	bp = &DEBUGGY(gap, i);
	if (((bp->dbg_type == DBG_FILE) == file && (
#ifdef FEAT_PROFILE
		gap == &prof_ga ||
#endif
		(bp->dbg_lnum > after && (lnum == 0 || bp->dbg_lnum < lnum)))))
	{
	    regmatch.regprog = bp->dbg_prog;
	    regmatch.rm_ic = FALSE;
	    /*
	     * Save the value of got_int and reset it.  We don't want a
	     * previous interruption cancel matching, only hitting CTRL-C
	     * while matching should abort it.
	     */
	    prev_got_int = got_int;
	    got_int = FALSE;
	    if (vim_regexec(&regmatch, name, (colnr_T)0))
	    {
		lnum = bp->dbg_lnum;
		if (fp != NULL)
		    *fp = bp->dbg_forceit;
	    }
	    got_int |= prev_got_int;
	}
    }
    if (name != fname)
	vim_free(name);

    return lnum;
}

/*
 * Called when a breakpoint was encountered.
 */
    void
dbg_breakpoint(name, lnum)
    char_u	*name;
    linenr_T	lnum;
{
    /* We need to check if this line is actually executed in do_one_cmd() */
    debug_breakpoint_name = name;
    debug_breakpoint_lnum = lnum;
}


# if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Functions for profiling.
 */
static void script_do_profile __ARGS((scriptitem_T *si));
static void script_dump_profile __ARGS((FILE *fd));
static proftime_T prof_wait_time;

/*
 * Set the time in "tm" to zero.
 */
    void
profile_zero(tm)
    proftime_T *tm;
{
# ifdef WIN3264
    tm->QuadPart = 0;
# else
    tm->tv_usec = 0;
    tm->tv_sec = 0;
# endif
}

/*
 * Store the current time in "tm".
 */
    void
profile_start(tm)
    proftime_T *tm;
{
# ifdef WIN3264
    QueryPerformanceCounter(tm);
# else
    gettimeofday(tm, NULL);
# endif
}

/*
 * Compute the elapsed time from "tm" till now and store in "tm".
 */
    void
profile_end(tm)
    proftime_T *tm;
{
    proftime_T now;

# ifdef WIN3264
    QueryPerformanceCounter(&now);
    tm->QuadPart = now.QuadPart - tm->QuadPart;
# else
    gettimeofday(&now, NULL);
    tm->tv_usec = now.tv_usec - tm->tv_usec;
    tm->tv_sec = now.tv_sec - tm->tv_sec;
    if (tm->tv_usec < 0)
    {
	tm->tv_usec += 1000000;
	--tm->tv_sec;
    }
# endif
}

/*
 * Subtract the time "tm2" from "tm".
 */
    void
profile_sub(tm, tm2)
    proftime_T *tm, *tm2;
{
# ifdef WIN3264
    tm->QuadPart -= tm2->QuadPart;
# else
    tm->tv_usec -= tm2->tv_usec;
    tm->tv_sec -= tm2->tv_sec;
    if (tm->tv_usec < 0)
    {
	tm->tv_usec += 1000000;
	--tm->tv_sec;
    }
# endif
}

/*
 * Add the time "tm2" to "tm".
 */
    void
profile_add(tm, tm2)
    proftime_T *tm, *tm2;
{
# ifdef WIN3264
    tm->QuadPart += tm2->QuadPart;
# else
    tm->tv_usec += tm2->tv_usec;
    tm->tv_sec += tm2->tv_sec;
    if (tm->tv_usec >= 1000000)
    {
	tm->tv_usec -= 1000000;
	++tm->tv_sec;
    }
# endif
}

/*
 * Get the current waittime.
 */
    void
profile_get_wait(tm)
    proftime_T *tm;
{
    *tm = prof_wait_time;
}

/*
 * Subtract the passed waittime since "tm" from "tma".
 */
    void
profile_sub_wait(tm, tma)
    proftime_T *tm, *tma;
{
    proftime_T tm3 = prof_wait_time;

    profile_sub(&tm3, tm);
    profile_sub(tma, &tm3);
}

/*
 * Return TRUE if "tm1" and "tm2" are equal.
 */
    int
profile_equal(tm1, tm2)
    proftime_T *tm1, *tm2;
{
# ifdef WIN3264
    return (tm1->QuadPart == tm2->QuadPart);
# else
    return (tm1->tv_usec == tm2->tv_usec && tm1->tv_sec == tm2->tv_sec);
# endif
}

/*
 * Return <0, 0 or >0 if "tm1" < "tm2", "tm1" == "tm2" or "tm1" > "tm2"
 */
    int
profile_cmp(tm1, tm2)
    proftime_T *tm1, *tm2;
{
# ifdef WIN3264
    return (int)(tm2->QuadPart - tm1->QuadPart);
# else
    if (tm1->tv_sec == tm2->tv_sec)
	return tm2->tv_usec - tm1->tv_usec;
    return tm2->tv_sec - tm1->tv_sec;
# endif
}

/*
 * Return a string that represents a time.
 * Uses a static buffer!
 */
    char *
profile_msg(tm)
    proftime_T *tm;
{
    static char buf[50];

# ifdef WIN3264
    LARGE_INTEGER   fr;

    QueryPerformanceFrequency(&fr);
    sprintf(buf, "%10.6lf", (double)tm->QuadPart / (double)fr.QuadPart);
# else
    sprintf(buf, "%3ld.%06ld", (long)tm->tv_sec, (long)tm->tv_usec);
#endif
    return buf;
}

static char_u	*profile_fname = NULL;

/*
 * ":profile cmd args"
 */
    void
ex_profile(eap)
    exarg_T	*eap;
{
    char_u	*e;
    int		len;

    e = skiptowhite(eap->arg);
    len = e - eap->arg;
    e = skipwhite(e);

    if (len == 5 && STRNCMP(eap->arg, "start", 5) == 0 && *e != NUL)
    {
	vim_free(profile_fname);
	profile_fname = vim_strsave(e);
	do_profiling = TRUE;
	profile_zero(&prof_wait_time);
	set_vim_var_nr(VV_PROFILING, 1L);
    }
    else if (!do_profiling)
	EMSG(_("E750: First use :profile start <fname>"));
    else
    {
	/* The rest is similar to ":breakadd". */
	ex_breakadd(eap);
    }
}

/*
 * Dump the profiling info.
 */
    void
profile_dump()
{
    FILE	*fd;

    if (profile_fname != NULL)
    {
	fd = fopen((char *)profile_fname, "w");
	if (fd == NULL)
	    EMSG2(_(e_notopen), profile_fname);
	else
	{
	    script_dump_profile(fd);
	    func_dump_profile(fd);
	    fclose(fd);
	}
    }
}

/*
 * Start profiling script "fp".
 */
    static void
script_do_profile(si)
    scriptitem_T    *si;
{
    si->sn_pr_count = 0;
    profile_zero(&si->sn_pr_total);
    profile_zero(&si->sn_pr_self);

    ga_init2(&si->sn_prl_ga, sizeof(sn_prl_T), 100);
    si->sn_prl_idx = -1;
    si->sn_prof_on = TRUE;
    si->sn_pr_nest = 0;
}

/*
 * save time when starting to invoke another script or function.
 */
    void
script_prof_save(tm)
    proftime_T	*tm;	    /* place to store wait time */
{
    scriptitem_T    *si;

    if (current_SID > 0 && current_SID <= script_items.ga_len)
    {
	si = &SCRIPT_ITEM(current_SID);
	if (si->sn_prof_on && si->sn_pr_nest++ == 0)
	    profile_start(&si->sn_pr_child);
    }
    profile_get_wait(tm);
}

/*
 * Count time spent in children after invoking another script or function.
 */
    void
script_prof_restore(tm)
    proftime_T	*tm;
{
    scriptitem_T    *si;

    if (current_SID > 0 && current_SID <= script_items.ga_len)
    {
	si = &SCRIPT_ITEM(current_SID);
	if (si->sn_prof_on && --si->sn_pr_nest == 0)
	{
	    profile_end(&si->sn_pr_child);
	    profile_sub_wait(tm, &si->sn_pr_child); /* don't count wait time */
	    profile_add(&si->sn_pr_children, &si->sn_pr_child);
	    profile_add(&si->sn_prl_children, &si->sn_pr_child);
	}
    }
}

static proftime_T inchar_time;

/*
 * Called when starting to wait for the user to type a character.
 */
    void
prof_inchar_enter()
{
    profile_start(&inchar_time);
}

/*
 * Called when finished waiting for the user to type a character.
 */
    void
prof_inchar_exit()
{
    profile_end(&inchar_time);
    profile_add(&prof_wait_time, &inchar_time);
}

/*
 * Dump the profiling results for all scripts in file "fd".
 */
    static void
script_dump_profile(fd)
    FILE    *fd;
{
    int		    id;
    scriptitem_T    *si;
    int		    i;
    FILE	    *sfd;
    sn_prl_T	    *pp;

    for (id = 1; id <= script_items.ga_len; ++id)
    {
	si = &SCRIPT_ITEM(id);
	if (si->sn_prof_on)
	{
	    fprintf(fd, "SCRIPT  %s\n", si->sn_name);
	    if (si->sn_pr_count == 1)
		fprintf(fd, "Sourced 1 time\n");
	    else
		fprintf(fd, "Sourced %d times\n", si->sn_pr_count);
	    fprintf(fd, "Total time: %s\n", profile_msg(&si->sn_pr_total));
	    fprintf(fd, " Self time: %s\n", profile_msg(&si->sn_pr_self));
	    fprintf(fd, "\n");
	    fprintf(fd, "count  total (s)   self (s)\n");

	    sfd = fopen((char *)si->sn_name, "r");
	    if (sfd == NULL)
		fprintf(fd, "Cannot open file!\n");
	    else
	    {
		for (i = 0; i < si->sn_prl_ga.ga_len; ++i)
		{
		    if (vim_fgets(IObuff, IOSIZE, sfd))
			break;
		    pp = &PRL_ITEM(si, i);
		    if (pp->snp_count > 0)
		    {
			fprintf(fd, "%5d ", pp->snp_count);
			if (profile_equal(&pp->sn_prl_total, &pp->sn_prl_self))
			    fprintf(fd, "           ");
			else
			    fprintf(fd, "%s ", profile_msg(&pp->sn_prl_total));
			fprintf(fd, "%s ", profile_msg(&pp->sn_prl_self));
		    }
		    else
			fprintf(fd, "                            ");
		    fprintf(fd, "%s", IObuff);
		}
		fclose(sfd);
	    }
	    fprintf(fd, "\n");
	}
    }
}

/*
 * Return TRUE when a function defined in the current script should be
 * profiled.
 */
    int
prof_def_func()
{
    scriptitem_T    *si = &SCRIPT_ITEM(current_SID);

    return si->sn_pr_force;
}

# endif
#endif

/*
 * If 'autowrite' option set, try to write the file.
 * Careful: autocommands may make "buf" invalid!
 *
 * return FAIL for failure, OK otherwise
 */
    int
autowrite(buf, forceit)
    buf_T	*buf;
    int		forceit;
{
    if (!(p_aw || p_awa) || !p_write
#ifdef FEAT_QUICKFIX
	/* never autowrite a "nofile" or "nowrite" buffer */
	|| bt_dontwrite(buf)
#endif
	|| (!forceit && buf->b_p_ro) || buf->b_ffname == NULL)
	return FAIL;
    return buf_write_all(buf, forceit);
}

/*
 * flush all buffers, except the ones that are readonly
 */
    void
autowrite_all()
{
    buf_T	*buf;

    if (!(p_aw || p_awa) || !p_write)
	return;
    for (buf = firstbuf; buf; buf = buf->b_next)
	if (bufIsChanged(buf) && !buf->b_p_ro)
	{
	    (void)buf_write_all(buf, FALSE);
#ifdef FEAT_AUTOCMD
	    /* an autocommand may have deleted the buffer */
	    if (!buf_valid(buf))
		buf = firstbuf;
#endif
	}
}

/*
 * return TRUE if buffer was changed and cannot be abandoned.
 */
/*ARGSUSED*/
    int
check_changed(buf, checkaw, mult_win, forceit, allbuf)
    buf_T	*buf;
    int		checkaw;	/* do autowrite if buffer was changed */
    int		mult_win;	/* check also when several wins for the buf */
    int		forceit;
    int		allbuf;		/* may write all buffers */
{
    if (       !forceit
	    && bufIsChanged(buf)
	    && (mult_win || buf->b_nwindows <= 1)
	    && (!checkaw || autowrite(buf, forceit) == FAIL))
    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	if ((p_confirm || cmdmod.confirm) && p_write)
	{
	    buf_T	*buf2;
	    int		count = 0;

	    if (allbuf)
		for (buf2 = firstbuf; buf2 != NULL; buf2 = buf2->b_next)
		    if (bufIsChanged(buf2)
				     && (buf2->b_ffname != NULL
# ifdef FEAT_BROWSE
					 || cmdmod.browse
# endif
					))
			++count;
# ifdef FEAT_AUTOCMD
	    if (!buf_valid(buf))
		/* Autocommand deleted buffer, oops!  It's not changed now. */
		return FALSE;
# endif
	    dialog_changed(buf, count > 1);
# ifdef FEAT_AUTOCMD
	    if (!buf_valid(buf))
		/* Autocommand deleted buffer, oops!  It's not changed now. */
		return FALSE;
# endif
	    return bufIsChanged(buf);
	}
#endif
	EMSG(_(e_nowrtmsg));
	return TRUE;
    }
    return FALSE;
}

#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG) || defined(PROTO)

#if defined(FEAT_BROWSE) || defined(PROTO)
/*
 * When wanting to write a file without a file name, ask the user for a name.
 */
    void
browse_save_fname(buf)
    buf_T	*buf;
{
    if (buf->b_fname == NULL)
    {
	char_u *fname;

	fname = do_browse(BROWSE_SAVE, (char_u *)_("Save As"),
						 NULL, NULL, NULL, NULL, buf);
	if (fname != NULL)
	{
	    if (setfname(buf, fname, NULL, TRUE) == OK)
		buf->b_flags |= BF_NOTEDITED;
	    vim_free(fname);
	}
    }
}
#endif

/*
 * Ask the user what to do when abondoning a changed buffer.
 * Must check 'write' option first!
 */
    void
dialog_changed(buf, checkall)
    buf_T	*buf;
    int		checkall;	/* may abandon all changed buffers */
{
    char_u	buff[IOSIZE];
    int		ret;
    buf_T	*buf2;

    dialog_msg(buff, _("Save changes to \"%.*s\"?"),
			(buf->b_fname != NULL) ?
			buf->b_fname : (char_u *)_("Untitled"));
    if (checkall)
	ret = vim_dialog_yesnoallcancel(VIM_QUESTION, NULL, buff, 1);
    else
	ret = vim_dialog_yesnocancel(VIM_QUESTION, NULL, buff, 1);

    if (ret == VIM_YES)
    {
#ifdef FEAT_BROWSE
	/* May get file name, when there is none */
	browse_save_fname(buf);
#endif
	if (buf->b_fname != NULL)   /* didn't hit Cancel */
	    (void)buf_write_all(buf, FALSE);
    }
    else if (ret == VIM_NO)
    {
	unchanged(buf, TRUE);
    }
    else if (ret == VIM_ALL)
    {
	/*
	 * Write all modified files that can be written.
	 * Skip readonly buffers, these need to be confirmed
	 * individually.
	 */
	for (buf2 = firstbuf; buf2 != NULL; buf2 = buf2->b_next)
	{
	    if (bufIsChanged(buf2)
		    && (buf2->b_ffname != NULL
#ifdef FEAT_BROWSE
			|| cmdmod.browse
#endif
			)
		    && !buf2->b_p_ro)
	    {
#ifdef FEAT_BROWSE
		/* May get file name, when there is none */
		browse_save_fname(buf2);
#endif
		if (buf2->b_fname != NULL)   /* didn't hit Cancel */
		    (void)buf_write_all(buf2, FALSE);
#ifdef FEAT_AUTOCMD
		/* an autocommand may have deleted the buffer */
		if (!buf_valid(buf2))
		    buf2 = firstbuf;
#endif
	    }
	}
    }
    else if (ret == VIM_DISCARDALL)
    {
	/*
	 * mark all buffers as unchanged
	 */
	for (buf2 = firstbuf; buf2 != NULL; buf2 = buf2->b_next)
	    unchanged(buf2, TRUE);
    }
}
#endif

/*
 * Return TRUE if the buffer "buf" can be abandoned, either by making it
 * hidden, autowriting it or unloading it.
 */
    int
can_abandon(buf, forceit)
    buf_T	*buf;
    int		forceit;
{
    return (	   P_HID(buf)
		|| !bufIsChanged(buf)
		|| buf->b_nwindows > 1
		|| autowrite(buf, forceit) == OK
		|| forceit);
}

/*
 * Return TRUE if any buffer was changed and cannot be abandoned.
 * That changed buffer becomes the current buffer.
 */
    int
check_changed_any(hidden)
    int		hidden;		/* Only check hidden buffers */
{
    buf_T	*buf;
    int		save;
#ifdef FEAT_WINDOWS
    win_T	*wp;
#endif

    for (;;)
    {
	/* check curbuf first: if it was changed we can't abandon it */
	if (!hidden && curbufIsChanged())
	    buf = curbuf;
	else
	{
	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
		if ((!hidden || buf->b_nwindows == 0) && bufIsChanged(buf))
		    break;
	}
	if (buf == NULL)    /* No buffers changed */
	    return FALSE;

	if (check_changed(buf, p_awa, TRUE, FALSE, TRUE) && buf_valid(buf))
	    break;	    /* didn't save - still changes */
    }

    exiting = FALSE;
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
    /*
     * When ":confirm" used, don't give an error message.
     */
    if (!(p_confirm || cmdmod.confirm))
#endif
    {
	/* There must be a wait_return for this message, do_buffer()
	 * may cause a redraw.  But wait_return() is a no-op when vgetc()
	 * is busy (Quit used from window menu), then make sure we don't
	 * cause a scroll up. */
	if (vgetc_busy)
	{
	    msg_row = cmdline_row;
	    msg_col = 0;
	    msg_didout = FALSE;
	}
	if (EMSG2(_("E162: No write since last change for buffer \"%s\""),
		    buf_spname(buf) != NULL ? (char_u *)buf_spname(buf) :
		    buf->b_fname))
	{
	    save = no_wait_return;
	    no_wait_return = FALSE;
	    wait_return(FALSE);
	    no_wait_return = save;
	}
    }

#ifdef FEAT_WINDOWS
    /* Try to find a window that contains the buffer. */
    if (buf != curbuf)
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	    if (wp->w_buffer == buf)
	    {
		win_goto(wp);
# ifdef FEAT_AUTOCMD
		/* Paranoia: did autocms wipe out the buffer with changes? */
		if (!buf_valid(buf))
		    return TRUE;
# endif
		break;
	    }
#endif

    /* Open the changed buffer in the current window. */
    if (buf != curbuf)
	set_curbuf(buf, DOBUF_GOTO);

    return TRUE;
}

/*
 * return FAIL if there is no file name, OK if there is one
 * give error message for FAIL
 */
    int
check_fname()
{
    if (curbuf->b_ffname == NULL)
    {
	EMSG(_(e_noname));
	return FAIL;
    }
    return OK;
}

/*
 * flush the contents of a buffer, unless it has no file name
 *
 * return FAIL for failure, OK otherwise
 */
    int
buf_write_all(buf, forceit)
    buf_T	*buf;
    int		forceit;
{
    int	    retval;
#ifdef FEAT_AUTOCMD
    buf_T	*old_curbuf = curbuf;
#endif

    retval = (buf_write(buf, buf->b_ffname, buf->b_fname,
				   (linenr_T)1, buf->b_ml.ml_line_count, NULL,
						  FALSE, forceit, TRUE, FALSE));
#ifdef FEAT_AUTOCMD
    if (curbuf != old_curbuf)
    {
	msg_source(hl_attr(HLF_W));
	MSG(_("Warning: Entered other buffer unexpectedly (check autocommands)"));
    }
#endif
    return retval;
}

/*
 * Code to handle the argument list.
 */

static char_u	*do_one_arg __ARGS((char_u *str));
static int	do_arglist __ARGS((char_u *str, int what, int after));
static void	alist_check_arg_idx __ARGS((void));
static int	editing_arg_idx __ARGS((win_T *win));
#ifdef FEAT_LISTCMDS
static int	alist_add_list __ARGS((int count, char_u **files, int after));
#endif
#define AL_SET	1
#define AL_ADD	2
#define AL_DEL	3

/*
 * Isolate one argument, taking backticks.
 * Changes the argument in-place, puts a NUL after it.  Backticks remain.
 * Return a pointer to the start of the next argument.
 */
    static char_u *
do_one_arg(str)
    char_u *str;
{
    char_u	*p;
    int		inbacktick;

    inbacktick = FALSE;
    for (p = str; *str; ++str)
    {
	/* When the backslash is used for escaping the special meaning of a
	 * character we need to keep it until wildcard expansion. */
	if (rem_backslash(str))
	{
	    *p++ = *str++;
	    *p++ = *str;
	}
	else
	{
	    /* An item ends at a space not in backticks */
	    if (!inbacktick && vim_isspace(*str))
		break;
	    if (*str == '`')
		inbacktick ^= TRUE;
	    *p++ = *str;
	}
    }
    str = skipwhite(str);
    *p = NUL;

    return str;
}

/*
 * Separate the arguments in "str" and return a list of pointers in the
 * growarray "gap".
 */
    int
get_arglist(gap, str)
    garray_T	*gap;
    char_u	*str;
{
    ga_init2(gap, (int)sizeof(char_u *), 20);
    while (*str != NUL)
    {
	if (ga_grow(gap, 1) == FAIL)
	{
	    ga_clear(gap);
	    return FAIL;
	}
	((char_u **)gap->ga_data)[gap->ga_len++] = str;

	/* Isolate one argument, change it in-place, put a NUL after it. */
	str = do_one_arg(str);
    }
    return OK;
}

#if defined(FEAT_QUICKFIX) || (defined(FEAT_SYN_HL) && defined(FEAT_MBYTE)) \
	|| defined(PROTO)
/*
 * Parse a list of arguments (file names), expand them and return in
 * "fnames[fcountp]".
 * Return FAIL or OK.
 */
    int
get_arglist_exp(str, fcountp, fnamesp)
    char_u	*str;
    int		*fcountp;
    char_u	***fnamesp;
{
    garray_T	ga;
    int		i;

    if (get_arglist(&ga, str) == FAIL)
	return FAIL;
    i = gen_expand_wildcards(ga.ga_len, (char_u **)ga.ga_data,
				       fcountp, fnamesp, EW_FILE|EW_NOTFOUND);
    ga_clear(&ga);
    return i;
}
#endif

#if defined(FEAT_GUI) || defined(FEAT_CLIENTSERVER) || defined(PROTO)
/*
 * Redefine the argument list.
 */
    void
set_arglist(str)
    char_u	*str;
{
    do_arglist(str, AL_SET, 0);
}
#endif

/*
 * "what" == AL_SET: Redefine the argument list to 'str'.
 * "what" == AL_ADD: add files in 'str' to the argument list after "after".
 * "what" == AL_DEL: remove files in 'str' from the argument list.
 *
 * Return FAIL for failure, OK otherwise.
 */
/*ARGSUSED*/
    static int
do_arglist(str, what, after)
    char_u	*str;
    int		what;
    int		after;		/* 0 means before first one */
{
    garray_T	new_ga;
    int		exp_count;
    char_u	**exp_files;
    int		i;
#ifdef FEAT_LISTCMDS
    char_u	*p;
    int		match;
#endif

    /*
     * Collect all file name arguments in "new_ga".
     */
    if (get_arglist(&new_ga, str) == FAIL)
	return FAIL;

#ifdef FEAT_LISTCMDS
    if (what == AL_DEL)
    {
	regmatch_T	regmatch;
	int		didone;

	/*
	 * Delete the items: use each item as a regexp and find a match in the
	 * argument list.
	 */
#ifdef CASE_INSENSITIVE_FILENAME
	regmatch.rm_ic = TRUE;		/* Always ignore case */
#else
	regmatch.rm_ic = FALSE;		/* Never ignore case */
#endif
	for (i = 0; i < new_ga.ga_len && !got_int; ++i)
	{
	    p = ((char_u **)new_ga.ga_data)[i];
	    p = file_pat_to_reg_pat(p, NULL, NULL, FALSE);
	    if (p == NULL)
		break;
	    regmatch.regprog = vim_regcomp(p, p_magic ? RE_MAGIC : 0);
	    if (regmatch.regprog == NULL)
	    {
		vim_free(p);
		break;
	    }

	    didone = FALSE;
	    for (match = 0; match < ARGCOUNT; ++match)
		if (vim_regexec(&regmatch, alist_name(&ARGLIST[match]),
								  (colnr_T)0))
		{
		    didone = TRUE;
		    vim_free(ARGLIST[match].ae_fname);
		    mch_memmove(ARGLIST + match, ARGLIST + match + 1,
			    (ARGCOUNT - match - 1) * sizeof(aentry_T));
		    --ALIST(curwin)->al_ga.ga_len;
		    if (curwin->w_arg_idx > match)
			--curwin->w_arg_idx;
		    --match;
		}

	    vim_free(regmatch.regprog);
	    vim_free(p);
	    if (!didone)
		EMSG2(_(e_nomatch2), ((char_u **)new_ga.ga_data)[i]);
	}
	ga_clear(&new_ga);
    }
    else
#endif
    {
	i = expand_wildcards(new_ga.ga_len, (char_u **)new_ga.ga_data,
		&exp_count, &exp_files, EW_DIR|EW_FILE|EW_ADDSLASH|EW_NOTFOUND);
	ga_clear(&new_ga);
	if (i == FAIL)
	    return FAIL;
	if (exp_count == 0)
	{
	    EMSG(_(e_nomatch));
	    return FAIL;
	}

#ifdef FEAT_LISTCMDS
	if (what == AL_ADD)
	{
	    (void)alist_add_list(exp_count, exp_files, after);
	    vim_free(exp_files);
	}
	else /* what == AL_SET */
#endif
	    alist_set(ALIST(curwin), exp_count, exp_files, FALSE, NULL, 0);
    }

    alist_check_arg_idx();

    return OK;
}

/*
 * Check the validity of the arg_idx for each other window.
 */
    static void
alist_check_arg_idx()
{
#ifdef FEAT_WINDOWS
    win_T	*win;

    for (win = firstwin; win != NULL; win = win->w_next)
	if (win->w_alist == curwin->w_alist)
	    check_arg_idx(win);
#else
    check_arg_idx(curwin);
#endif
}

/*
 * Return TRUE if window "win" is editing then file at the current argument
 * index.
 */
    static int
editing_arg_idx(win)
    win_T	*win;
{
    return !(win->w_arg_idx >= WARGCOUNT(win)
		|| (win->w_buffer->b_fnum
				      != WARGLIST(win)[win->w_arg_idx].ae_fnum
		    && (win->w_buffer->b_ffname == NULL
			 || !(fullpathcmp(
				 alist_name(&WARGLIST(win)[win->w_arg_idx]),
				win->w_buffer->b_ffname, TRUE) & FPC_SAME))));
}

/*
 * Check if window "win" is editing the w_arg_idx file in its argument list.
 */
    void
check_arg_idx(win)
    win_T	*win;
{
    if (WARGCOUNT(win) > 1 && !editing_arg_idx(win))
    {
	/* We are not editing the current entry in the argument list.
	 * Set "arg_had_last" if we are editing the last one. */
	win->w_arg_idx_invalid = TRUE;
	if (win->w_arg_idx != WARGCOUNT(win) - 1
		&& arg_had_last == FALSE
#ifdef FEAT_WINDOWS
		&& ALIST(win) == &global_alist
#endif
		&& GARGCOUNT > 0
		&& win->w_arg_idx < GARGCOUNT
		&& (win->w_buffer->b_fnum == GARGLIST[GARGCOUNT - 1].ae_fnum
		    || (win->w_buffer->b_ffname != NULL
			&& (fullpathcmp(alist_name(&GARGLIST[GARGCOUNT - 1]),
				win->w_buffer->b_ffname, TRUE) & FPC_SAME))))
	    arg_had_last = TRUE;
    }
    else
    {
	/* We are editing the current entry in the argument list.
	 * Set "arg_had_last" if it's also the last one */
	win->w_arg_idx_invalid = FALSE;
	if (win->w_arg_idx == WARGCOUNT(win) - 1
#ifdef FEAT_WINDOWS
		&& win->w_alist == &global_alist
#endif
		)
	    arg_had_last = TRUE;
    }
}

/*
 * ":args", ":argslocal" and ":argsglobal".
 */
    void
ex_args(eap)
    exarg_T	*eap;
{
    int		i;

    if (eap->cmdidx != CMD_args)
    {
#if defined(FEAT_WINDOWS) && defined(FEAT_LISTCMDS)
	alist_unlink(ALIST(curwin));
	if (eap->cmdidx == CMD_argglobal)
	    ALIST(curwin) = &global_alist;
	else /* eap->cmdidx == CMD_arglocal */
	    alist_new();
#else
	ex_ni(eap);
	return;
#endif
    }

    if (!ends_excmd(*eap->arg))
    {
	/*
	 * ":args file ..": define new argument list, handle like ":next"
	 * Also for ":argslocal file .." and ":argsglobal file ..".
	 */
	ex_next(eap);
    }
    else
#if defined(FEAT_WINDOWS) && defined(FEAT_LISTCMDS)
	if (eap->cmdidx == CMD_args)
#endif
    {
	/*
	 * ":args": list arguments.
	 */
	if (ARGCOUNT > 0)
	{
	    /* Overwrite the command, for a short list there is no scrolling
	     * required and no wait_return(). */
	    gotocmdline(TRUE);
	    for (i = 0; i < ARGCOUNT; ++i)
	    {
		if (i == curwin->w_arg_idx)
		    msg_putchar('[');
		msg_outtrans(alist_name(&ARGLIST[i]));
		if (i == curwin->w_arg_idx)
		    msg_putchar(']');
		msg_putchar(' ');
	    }
	}
    }
#if defined(FEAT_WINDOWS) && defined(FEAT_LISTCMDS)
    else if (eap->cmdidx == CMD_arglocal)
    {
	garray_T	*gap = &curwin->w_alist->al_ga;

	/*
	 * ":argslocal": make a local copy of the global argument list.
	 */
	if (ga_grow(gap, GARGCOUNT) == OK)
	    for (i = 0; i < GARGCOUNT; ++i)
		if (GARGLIST[i].ae_fname != NULL)
		{
		    AARGLIST(curwin->w_alist)[gap->ga_len].ae_fname =
					    vim_strsave(GARGLIST[i].ae_fname);
		    AARGLIST(curwin->w_alist)[gap->ga_len].ae_fnum =
							  GARGLIST[i].ae_fnum;
		    ++gap->ga_len;
		}
    }
#endif
}

/*
 * ":previous", ":sprevious", ":Next" and ":sNext".
 */
    void
ex_previous(eap)
    exarg_T	*eap;
{
    /* If past the last one already, go to the last one. */
    if (curwin->w_arg_idx - (int)eap->line2 >= ARGCOUNT)
	do_argfile(eap, ARGCOUNT - 1);
    else
	do_argfile(eap, curwin->w_arg_idx - (int)eap->line2);
}

/*
 * ":rewind", ":first", ":sfirst" and ":srewind".
 */
    void
ex_rewind(eap)
    exarg_T	*eap;
{
    do_argfile(eap, 0);
}

/*
 * ":last" and ":slast".
 */
    void
ex_last(eap)
    exarg_T	*eap;
{
    do_argfile(eap, ARGCOUNT - 1);
}

/*
 * ":argument" and ":sargument".
 */
    void
ex_argument(eap)
    exarg_T	*eap;
{
    int		i;

    if (eap->addr_count > 0)
	i = eap->line2 - 1;
    else
	i = curwin->w_arg_idx;
    do_argfile(eap, i);
}

/*
 * Edit file "argn" of the argument lists.
 */
    void
do_argfile(eap, argn)
    exarg_T	*eap;
    int		argn;
{
    int		other;
    char_u	*p;
    int		old_arg_idx = curwin->w_arg_idx;

    if (argn < 0 || argn >= ARGCOUNT)
    {
	if (ARGCOUNT <= 1)
	    EMSG(_("E163: There is only one file to edit"));
	else if (argn < 0)
	    EMSG(_("E164: Cannot go before first file"));
	else
	    EMSG(_("E165: Cannot go beyond last file"));
    }
    else
    {
	setpcmark();
#ifdef FEAT_GUI
	need_mouse_correct = TRUE;
#endif

#ifdef FEAT_WINDOWS
	if (*eap->cmd == 's')	    /* split window first */
	{
	    if (win_split(0, 0) == FAIL)
		return;
# ifdef FEAT_SCROLLBIND
	    curwin->w_p_scb = FALSE;
# endif
	}
	else
#endif
	{
	    /*
	     * if 'hidden' set, only check for changed file when re-editing
	     * the same buffer
	     */
	    other = TRUE;
	    if (P_HID(curbuf))
	    {
		p = fix_fname(alist_name(&ARGLIST[argn]));
		other = otherfile(p);
		vim_free(p);
	    }
	    if ((!P_HID(curbuf) || !other)
		  && check_changed(curbuf, TRUE, !other, eap->forceit, FALSE))
		return;
	}

	curwin->w_arg_idx = argn;
	if (argn == ARGCOUNT - 1
#ifdef FEAT_WINDOWS
		&& curwin->w_alist == &global_alist
#endif
	   )
	    arg_had_last = TRUE;

	/* Edit the file; always use the last known line number.
	 * When it fails (e.g. Abort for already edited file) restore the
	 * argument index. */
	if (do_ecmd(0, alist_name(&ARGLIST[curwin->w_arg_idx]), NULL,
		      eap, ECMD_LAST,
		      (P_HID(curwin->w_buffer) ? ECMD_HIDE : 0) +
				   (eap->forceit ? ECMD_FORCEIT : 0)) == FAIL)
	    curwin->w_arg_idx = old_arg_idx;
	/* like Vi: set the mark where the cursor is in the file. */
	else if (eap->cmdidx != CMD_argdo)
	    setmark('\'');
    }
}

/*
 * ":next", and commands that behave like it.
 */
    void
ex_next(eap)
    exarg_T	*eap;
{
    int		i;

    /*
     * check for changed buffer now, if this fails the argument list is not
     * redefined.
     */
    if (       P_HID(curbuf)
	    || eap->cmdidx == CMD_snext
	    || !check_changed(curbuf, TRUE, FALSE, eap->forceit, FALSE))
    {
	if (*eap->arg != NUL)		    /* redefine file list */
	{
	    if (do_arglist(eap->arg, AL_SET, 0) == FAIL)
		return;
	    i = 0;
	}
	else
	    i = curwin->w_arg_idx + (int)eap->line2;
	do_argfile(eap, i);
    }
}

#ifdef FEAT_LISTCMDS
/*
 * ":argedit"
 */
    void
ex_argedit(eap)
    exarg_T	*eap;
{
    int		fnum;
    int		i;
    char_u	*s;

    /* Add the argument to the buffer list and get the buffer number. */
    fnum = buflist_add(eap->arg, BLN_LISTED);

    /* Check if this argument is already in the argument list. */
    for (i = 0; i < ARGCOUNT; ++i)
	if (ARGLIST[i].ae_fnum == fnum)
	    break;
    if (i == ARGCOUNT)
    {
	/* Can't find it, add it to the argument list. */
	s = vim_strsave(eap->arg);
	if (s == NULL)
	    return;
	i = alist_add_list(1, &s,
	       eap->addr_count > 0 ? (int)eap->line2 : curwin->w_arg_idx + 1);
	if (i < 0)
	    return;
	curwin->w_arg_idx = i;
    }

    alist_check_arg_idx();

    /* Edit the argument. */
    do_argfile(eap, i);
}

/*
 * ":argadd"
 */
    void
ex_argadd(eap)
    exarg_T	*eap;
{
    do_arglist(eap->arg, AL_ADD,
	       eap->addr_count > 0 ? (int)eap->line2 : curwin->w_arg_idx + 1);
#ifdef FEAT_TITLE
    maketitle();
#endif
}

/*
 * ":argdelete"
 */
    void
ex_argdelete(eap)
    exarg_T	*eap;
{
    int		i;
    int		n;

    if (eap->addr_count > 0)
    {
	/* ":1,4argdel": Delete all arguments in the range. */
	if (eap->line2 > ARGCOUNT)
	    eap->line2 = ARGCOUNT;
	n = eap->line2 - eap->line1 + 1;
	if (*eap->arg != NUL || n <= 0)
	    EMSG(_(e_invarg));
	else
	{
	    for (i = eap->line1; i <= eap->line2; ++i)
		vim_free(ARGLIST[i - 1].ae_fname);
	    mch_memmove(ARGLIST + eap->line1 - 1, ARGLIST + eap->line2,
			(size_t)((ARGCOUNT - eap->line2) * sizeof(aentry_T)));
	    ALIST(curwin)->al_ga.ga_len -= n;
	    if (curwin->w_arg_idx >= eap->line2)
		curwin->w_arg_idx -= n;
	    else if (curwin->w_arg_idx > eap->line1)
		curwin->w_arg_idx = eap->line1;
	}
    }
    else if (*eap->arg == NUL)
	EMSG(_(e_argreq));
    else
	do_arglist(eap->arg, AL_DEL, 0);
#ifdef FEAT_TITLE
    maketitle();
#endif
}

/*
 * ":argdo", ":windo", ":bufdo"
 */
    void
ex_listdo(eap)
    exarg_T	*eap;
{
    int		i;
#ifdef FEAT_WINDOWS
    win_T	*win;
#endif
    buf_T	*buf;
    int		next_fnum = 0;
#if defined(FEAT_AUTOCMD) && defined(FEAT_SYN_HL)
    char_u	*save_ei = NULL;
#endif
    char_u	*p_shm_save;

#ifndef FEAT_WINDOWS
    if (eap->cmdidx == CMD_windo)
    {
	ex_ni(eap);
	return;
    }
#endif

#if defined(FEAT_AUTOCMD) && defined(FEAT_SYN_HL)
    if (eap->cmdidx != CMD_windo)
	/* Don't do syntax HL autocommands.  Skipping the syntax file is a
	 * great speed improvement. */
	save_ei = au_event_disable(",Syntax");
#endif

    if (eap->cmdidx == CMD_windo
	    || P_HID(curbuf)
	    || !check_changed(curbuf, TRUE, FALSE, eap->forceit, FALSE))
    {
	/* start at the first argument/window/buffer */
	i = 0;
#ifdef FEAT_WINDOWS
	win = firstwin;
#endif
	/* set pcmark now */
	if (eap->cmdidx == CMD_bufdo)
	    goto_buffer(eap, DOBUF_FIRST, FORWARD, 0);
	else
	    setpcmark();
	listcmd_busy = TRUE;	    /* avoids setting pcmark below */

	while (!got_int)
	{
	    if (eap->cmdidx == CMD_argdo)
	    {
		/* go to argument "i" */
		if (i == ARGCOUNT)
		    break;
		/* Don't call do_argfile() when already there, it will try
		 * reloading the file. */
		if (curwin->w_arg_idx != i || !editing_arg_idx(curwin))
		{
		    /* Clear 'shm' to avoid that the file message overwrites
		     * any output from the command. */
		    p_shm_save = vim_strsave(p_shm);
		    set_option_value((char_u *)"shm", 0L, (char_u *)"", 0);
		    do_argfile(eap, i);
		    set_option_value((char_u *)"shm", 0L, p_shm_save, 0);
		    vim_free(p_shm_save);
		}
		if (curwin->w_arg_idx != i)
		    break;
		++i;
	    }
#ifdef FEAT_WINDOWS
	    else if (eap->cmdidx == CMD_windo)
	    {
		/* go to window "win" */
		if (!win_valid(win))
		    break;
		win_goto(win);
		win = win->w_next;
	    }
#endif
	    else if (eap->cmdidx == CMD_bufdo)
	    {
		/* Remember the number of the next listed buffer, in case
		 * ":bwipe" is used or autocommands do something strange. */
		next_fnum = -1;
		for (buf = curbuf->b_next; buf != NULL; buf = buf->b_next)
		    if (buf->b_p_bl)
		    {
			next_fnum = buf->b_fnum;
			break;
		    }
	    }

	    /* execute the command */
	    do_cmdline(eap->arg, eap->getline, eap->cookie,
						DOCMD_VERBOSE + DOCMD_NOWAIT);

	    if (eap->cmdidx == CMD_bufdo)
	    {
		/* Done? */
		if (next_fnum < 0)
		    break;
		/* Check if the buffer still exists. */
		for (buf = firstbuf; buf != NULL; buf = buf->b_next)
		    if (buf->b_fnum == next_fnum)
			break;
		if (buf == NULL)
		    break;

		/* Go to the next buffer.  Clear 'shm' to avoid that the file
		 * message overwrites any output from the command. */
		p_shm_save = vim_strsave(p_shm);
		set_option_value((char_u *)"shm", 0L, (char_u *)"", 0);
		goto_buffer(eap, DOBUF_FIRST, FORWARD, next_fnum);
		set_option_value((char_u *)"shm", 0L, p_shm_save, 0);
		vim_free(p_shm_save);

		/* If autocommands took us elsewhere, quit here */
		if (curbuf->b_fnum != next_fnum)
		    break;
	    }

	    if (eap->cmdidx == CMD_windo)
	    {
		validate_cursor();	/* cursor may have moved */
#ifdef FEAT_SCROLLBIND
		/* required when 'scrollbind' has been set */
		if (curwin->w_p_scb)
		    do_check_scrollbind(TRUE);
#endif
	    }
	}
	listcmd_busy = FALSE;
    }

#if defined(FEAT_AUTOCMD) && defined(FEAT_SYN_HL)
    if (save_ei != NULL)
    {
	au_event_restore(save_ei);
	apply_autocmds(EVENT_SYNTAX, curbuf->b_p_syn,
					       curbuf->b_fname, TRUE, curbuf);
    }
#endif
}

/*
 * Add files[count] to the arglist of the current window after arg "after".
 * The file names in files[count] must have been allocated and are taken over.
 * Files[] itself is not taken over.
 * Returns index of first added argument.  Returns -1 when failed (out of mem).
 */
    static int
alist_add_list(count, files, after)
    int		count;
    char_u	**files;
    int		after;	    /* where to add: 0 = before first one */
{
    int		i;

    if (ga_grow(&ALIST(curwin)->al_ga, count) == OK)
    {
	if (after < 0)
	    after = 0;
	if (after > ARGCOUNT)
	    after = ARGCOUNT;
	if (after < ARGCOUNT)
	    mch_memmove(&(ARGLIST[after + count]), &(ARGLIST[after]),
				       (ARGCOUNT - after) * sizeof(aentry_T));
	for (i = 0; i < count; ++i)
	{
	    ARGLIST[after + i].ae_fname = files[i];
	    ARGLIST[after + i].ae_fnum = buflist_add(files[i], BLN_LISTED);
	}
	ALIST(curwin)->al_ga.ga_len += count;
	if (curwin->w_arg_idx >= after)
	    ++curwin->w_arg_idx;
	return after;
    }

    for (i = 0; i < count; ++i)
	vim_free(files[i]);
    return -1;
}

#endif /* FEAT_LISTCMDS */

#ifdef FEAT_EVAL
/*
 * ":compiler[!] {name}"
 */
    void
ex_compiler(eap)
    exarg_T	*eap;
{
    char_u	*buf;
    char_u	*old_cur_comp = NULL;
    char_u	*p;

    if (*eap->arg == NUL)
    {
	/* List all compiler scripts. */
	do_cmdline_cmd((char_u *)"echo globpath(&rtp, 'compiler/*.vim')");
					/* ) keep the indenter happy... */
    }
    else
    {
	buf = alloc((unsigned)(STRLEN(eap->arg) + 14));
	if (buf != NULL)
	{
	    if (eap->forceit)
	    {
		/* ":compiler! {name}" sets global options */
		do_cmdline_cmd((char_u *)
				   "command -nargs=* CompilerSet set <args>");
	    }
	    else
	    {
		/* ":compiler! {name}" sets local options.
		 * To remain backwards compatible "current_compiler" is always
		 * used.  A user's compiler plugin may set it, the distributed
		 * plugin will then skip the settings.  Afterwards set
		 * "b:current_compiler" and restore "current_compiler". */
		old_cur_comp = get_var_value((char_u *)"current_compiler");
		if (old_cur_comp != NULL)
		    old_cur_comp = vim_strsave(old_cur_comp);
		do_cmdline_cmd((char_u *)
			      "command -nargs=* CompilerSet setlocal <args>");
	    }
	    do_unlet((char_u *)"current_compiler", TRUE);
	    do_unlet((char_u *)"b:current_compiler", TRUE);

	    sprintf((char *)buf, "compiler/%s.vim", eap->arg);
	    if (cmd_runtime(buf, TRUE) == FAIL)
		EMSG2(_("E666: compiler not supported: %s"), eap->arg);
	    vim_free(buf);

	    do_cmdline_cmd((char_u *)":delcommand CompilerSet");

	    /* Set "b:current_compiler" from "current_compiler". */
	    p = get_var_value((char_u *)"current_compiler");
	    if (p != NULL)
		set_internal_string_var((char_u *)"b:current_compiler", p);

	    /* Restore "current_compiler" for ":compiler {name}". */
	    if (!eap->forceit)
	    {
		if (old_cur_comp != NULL)
		{
		    set_internal_string_var((char_u *)"current_compiler",
								old_cur_comp);
		    vim_free(old_cur_comp);
		}
		else
		    do_unlet((char_u *)"current_compiler", TRUE);
	    }
	}
    }
}
#endif

/*
 * ":runtime {name}"
 */
    void
ex_runtime(eap)
    exarg_T	*eap;
{
    cmd_runtime(eap->arg, eap->forceit);
}

static void source_callback __ARGS((char_u *fname, void *cookie));

/*ARGSUSED*/
    static void
source_callback(fname, cookie)
    char_u	*fname;
    void	*cookie;
{
    (void)do_source(fname, FALSE, FALSE);
}

/*
 * Source the file "name" from all directories in 'runtimepath'.
 * "name" can contain wildcards.
 * When "all" is TRUE, source all files, otherwise only the first one.
 * return FAIL when no file could be sourced, OK otherwise.
 */
    int
cmd_runtime(name, all)
    char_u	*name;
    int		all;
{
    return do_in_runtimepath(name, all, source_callback, NULL);
}

/*
 * Find "name" in 'runtimepath'.  When found, invoke the callback function for
 * it: callback(fname, "cookie")
 * When "all" is TRUE repeat for all matches, otherwise only the first one is
 * used.
 * Returns OK when at least one match found, FAIL otherwise.
 */
    int
do_in_runtimepath(name, all, callback, cookie)
    char_u	*name;
    int		all;
    void	(*callback)__ARGS((char_u *fname, void *ck));
    void	*cookie;
{
    char_u	*rtp;
    char_u	*np;
    char_u	*buf;
    char_u	*rtp_copy;
    char_u	*tail;
    int		num_files;
    char_u	**files;
    int		i;
    int		did_one = FALSE;
#ifdef AMIGA
    struct Process	*proc = (struct Process *)FindTask(0L);
    APTR		save_winptr = proc->pr_WindowPtr;

    /* Avoid a requester here for a volume that doesn't exist. */
    proc->pr_WindowPtr = (APTR)-1L;
#endif

    /* Make a copy of 'runtimepath'.  Invoking the callback may change the
     * value. */
    rtp_copy = vim_strsave(p_rtp);
    buf = alloc(MAXPATHL);
    if (buf != NULL && rtp_copy != NULL)
    {
	if (p_verbose > 1)
	    smsg((char_u *)_("Searching for \"%s\" in \"%s\""),
						 (char *)name, (char *)p_rtp);

	/* Loop over all entries in 'runtimepath'. */
	rtp = rtp_copy;
	while (*rtp != NUL && (all || !did_one))
	{
	    /* Copy the path from 'runtimepath' to buf[]. */
	    copy_option_part(&rtp, buf, MAXPATHL, ",");
	    if (STRLEN(buf) + STRLEN(name) + 2 < MAXPATHL)
	    {
		add_pathsep(buf);
		tail = buf + STRLEN(buf);

		/* Loop over all patterns in "name" */
		np = name;
		while (*np != NUL && (all || !did_one))
		{
		    /* Append the pattern from "name" to buf[]. */
		    copy_option_part(&np, tail, (int)(MAXPATHL - (tail - buf)),
								       "\t ");

		    if (p_verbose > 2)
			smsg((char_u *)_("Searching for \"%s\""), buf);

		    /* Expand wildcards, invoke the callback for each match. */
		    if (gen_expand_wildcards(1, &buf, &num_files, &files,
							       EW_FILE) == OK)
		    {
			for (i = 0; i < num_files; ++i)
			{
			    (*callback)(files[i], cookie);
			    did_one = TRUE;
			    if (!all)
				break;
			}
			FreeWild(num_files, files);
		    }
		}
	    }
	}
    }
    vim_free(buf);
    vim_free(rtp_copy);
    if (p_verbose > 0 && !did_one)
	smsg((char_u *)_("not found in 'runtimepath': \"%s\""), name);

#ifdef AMIGA
    proc->pr_WindowPtr = save_winptr;
#endif

    return did_one ? OK : FAIL;
}

#if defined(FEAT_EVAL) && defined(FEAT_AUTOCMD)
/*
 * ":options"
 */
/*ARGSUSED*/
    void
ex_options(eap)
    exarg_T	*eap;
{
    cmd_source((char_u *)SYS_OPTWIN_FILE, NULL);
}
#endif

/*
 * ":source {fname}"
 */
    void
ex_source(eap)
    exarg_T	*eap;
{
#ifdef FEAT_BROWSE
    if (cmdmod.browse)
    {
	char_u *fname = NULL;

	fname = do_browse(0, (char_u *)_("Source Vim script"), eap->arg,
				      NULL, NULL, BROWSE_FILTER_MACROS, NULL);
	if (fname != NULL)
	{
	    cmd_source(fname, eap);
	    vim_free(fname);
	}
    }
    else
#endif
	cmd_source(eap->arg, eap);
}

    static void
cmd_source(fname, eap)
    char_u	*fname;
    exarg_T	*eap;
{
    if (*fname == NUL)
	EMSG(_(e_argreq));

    /* ":source!" read vi commands */
    else if (eap != NULL && eap->forceit)
	/* Need to execute the commands directly when:
	 * - ":g" command busy
	 * - after ":argdo", ":windo" or ":bufdo"
	 * - another command follows
	 * - inside a loop
	 */
	openscript(fname, global_busy || listcmd_busy || eap->nextcmd != NULL
#ifdef FEAT_EVAL
						 || eap->cstack->cs_idx >= 0
#endif
						 );

    /* ":source" read ex commands */
    else if (do_source(fname, FALSE, FALSE) == FAIL)
	EMSG2(_(e_notopen), fname);
}

/*
 * ":source" and associated commands.
 */
/*
 * Structure used to store info for each sourced file.
 * It is shared between do_source() and getsourceline().
 * This is required, because it needs to be handed to do_cmdline() and
 * sourcing can be done recursively.
 */
struct source_cookie
{
    FILE	*fp;		/* opened file for sourcing */
    char_u      *nextline;      /* if not NULL: line that was read ahead */
    int		finished;	/* ":finish" used */
#if defined (USE_CRNL) || defined (USE_CR)
    int		fileformat;	/* EOL_UNKNOWN, EOL_UNIX or EOL_DOS */
    int		error;		/* TRUE if LF found after CR-LF */
#endif
#ifdef FEAT_EVAL
    linenr_T	breakpoint;	/* next line with breakpoint or zero */
    char_u	*fname;		/* name of sourced file */
    int		dbg_tick;	/* debug_tick when breakpoint was set */
    int		level;		/* top nesting level of sourced file */
#endif
#ifdef FEAT_MBYTE
    vimconv_T	conv;		/* type of conversion */
#endif
};

#ifdef FEAT_EVAL
/*
 * Return the address holding the next breakpoint line for a source cookie.
 */
    linenr_T *
source_breakpoint(cookie)
    void *cookie;
{
    return &((struct source_cookie *)cookie)->breakpoint;
}

/*
 * Return the address holding the debug tick for a source cookie.
 */
    int *
source_dbg_tick(cookie)
    void *cookie;
{
    return &((struct source_cookie *)cookie)->dbg_tick;
}

/*
 * Return the nesting level for a source cookie.
 */
    int
source_level(cookie)
    void *cookie;
{
    return ((struct source_cookie *)cookie)->level;
}
#endif

static char_u *get_one_sourceline __ARGS((struct source_cookie *sp));

#if defined(WIN32) && defined(FEAT_CSCOPE)
static FILE *fopen_noinh_readbin __ARGS((char *filename));

/*
 * Special function to open a file without handle inheritance.
 */
    static FILE *
fopen_noinh_readbin(filename)
    char    *filename;
{
    int	fd_tmp = mch_open(filename, O_RDONLY | O_BINARY | O_NOINHERIT, 0);

    if (fd_tmp == -1)
	return NULL;
    return fdopen(fd_tmp, READBIN);
}
#endif


/*
 * do_source: Read the file "fname" and execute its lines as EX commands.
 *
 * This function may be called recursively!
 *
 * return FAIL if file could not be opened, OK otherwise
 */
    int
do_source(fname, check_other, is_vimrc)
    char_u	*fname;
    int		check_other;	    /* check for .vimrc and _vimrc */
    int		is_vimrc;	    /* call vimrc_found() when file exists */
{
    struct source_cookie    cookie;
    char_u		    *save_sourcing_name;
    linenr_T		    save_sourcing_lnum;
    char_u		    *p;
    char_u		    *fname_exp;
    int			    retval = FAIL;
#ifdef FEAT_EVAL
    scid_T		    save_current_SID;
    static scid_T	    last_current_SID = 0;
    void		    *save_funccalp;
    int			    save_debug_break_level = debug_break_level;
    scriptitem_T	    *si = NULL;
# ifdef UNIX
    struct stat		    st;
    int			    stat_ok;
# endif
#endif
#ifdef STARTUPTIME
    struct timeval	    tv_rel;
    struct timeval	    tv_start;
#endif
#ifdef FEAT_PROFILE
    proftime_T		    wait_start;
#endif

#ifdef RISCOS
    p = mch_munge_fname(fname);
#else
    p = expand_env_save(fname);
#endif
    if (p == NULL)
	return retval;
    fname_exp = fix_fname(p);
    vim_free(p);
    if (fname_exp == NULL)
	return retval;
#ifdef MACOS_CLASSIC
    slash_n_colon_adjust(fname_exp);
#endif
    if (mch_isdir(fname_exp))
    {
	smsg((char_u *)_("Cannot source a directory: \"%s\""), fname);
	goto theend;
    }

#if defined(WIN32) && defined(FEAT_CSCOPE)
    cookie.fp = fopen_noinh_readbin((char *)fname_exp);
#else
    cookie.fp = mch_fopen((char *)fname_exp, READBIN);
#endif
    if (cookie.fp == NULL && check_other)
    {
	/*
	 * Try again, replacing file name ".vimrc" by "_vimrc" or vice versa,
	 * and ".exrc" by "_exrc" or vice versa.
	 */
	p = gettail(fname_exp);
	if ((*p == '.' || *p == '_')
		&& (STRICMP(p + 1, "vimrc") == 0
		    || STRICMP(p + 1, "gvimrc") == 0
		    || STRICMP(p + 1, "exrc") == 0))
	{
	    if (*p == '_')
		*p = '.';
	    else
		*p = '_';
#if defined(WIN32) && defined(FEAT_CSCOPE)
	    cookie.fp = fopen_noinh_readbin((char *)fname_exp);
#else
	    cookie.fp = mch_fopen((char *)fname_exp, READBIN);
#endif
	}
    }

    if (cookie.fp == NULL)
    {
	if (p_verbose > 0)
	{
	    if (sourcing_name == NULL)
		smsg((char_u *)_("could not source \"%s\""), fname);
	    else
		smsg((char_u *)_("line %ld: could not source \"%s\""),
							sourcing_lnum, fname);
	}
	goto theend;
    }

    /*
     * The file exists.
     * - In verbose mode, give a message.
     * - For a vimrc file, may want to set 'compatible', call vimrc_found().
     */
    if (p_verbose > 1)
    {
	if (sourcing_name == NULL)
	    smsg((char_u *)_("sourcing \"%s\""), fname);
	else
	    smsg((char_u *)_("line %ld: sourcing \"%s\""),
							sourcing_lnum, fname);
    }
    if (is_vimrc)
	vimrc_found();

#ifdef USE_CRNL
    /* If no automatic file format: Set default to CR-NL. */
    if (*p_ffs == NUL)
	cookie.fileformat = EOL_DOS;
    else
	cookie.fileformat = EOL_UNKNOWN;
    cookie.error = FALSE;
#endif

#ifdef USE_CR
    /* If no automatic file format: Set default to CR. */
    if (*p_ffs == NUL)
	cookie.fileformat = EOL_MAC;
    else
	cookie.fileformat = EOL_UNKNOWN;
    cookie.error = FALSE;
#endif

    cookie.nextline = NULL;
    cookie.finished = FALSE;

#ifdef FEAT_EVAL
    /*
     * Check if this script has a breakpoint.
     */
    cookie.breakpoint = dbg_find_breakpoint(TRUE, fname_exp, (linenr_T)0);
    cookie.fname = fname_exp;
    cookie.dbg_tick = debug_tick;

    cookie.level = ex_nesting_level;
#endif
#ifdef FEAT_MBYTE
    cookie.conv.vc_type = CONV_NONE;		/* no conversion */

    /* Try reading the first few bytes to check for a UTF-8 BOM. */
    {
	char_u	    buf[3];

	if (fread((char *)buf, sizeof(char_u), (size_t)3, cookie.fp)
								  == (size_t)3
		&& buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
	    /* Found BOM, setup conversion and skip over it. */
	    convert_setup(&cookie.conv, (char_u *)"utf-8", p_enc);
	else
	    /* No BOM found, rewind. */
	    fseek(cookie.fp, 0L, SEEK_SET);
    }
#endif

    /*
     * Keep the sourcing name/lnum, for recursive calls.
     */
    save_sourcing_name = sourcing_name;
    sourcing_name = fname_exp;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 0;

#ifdef STARTUPTIME
    time_push(&tv_rel, &tv_start);
#endif

#ifdef FEAT_EVAL
# ifdef FEAT_PROFILE
    if (do_profiling)
	prof_child_enter(&wait_start);		/* entering a child now */
# endif

    /* Don't use local function variables, if called from a function.
     * Also starts profiling timer for nested script. */
    save_funccalp = save_funccal();

    /*
     * Check if this script was sourced before to finds its SID.
     * If it's new, generate a new SID.
     */
    save_current_SID = current_SID;
# ifdef UNIX
    stat_ok = (mch_stat((char *)fname_exp, &st) >= 0);
# endif
    for (current_SID = script_items.ga_len; current_SID > 0; --current_SID)
    {
	si = &SCRIPT_ITEM(current_SID);
	if (si->sn_name != NULL
		&& (
# ifdef UNIX
		    /* Compare dev/ino when possible, it catches symbolic
		     * links.  Also compare file names, the inode may change
		     * when the file was edited. */
		    ((stat_ok && si->sn_dev != -1)
			&& (si->sn_dev == st.st_dev
			    && si->sn_ino == st.st_ino)) ||
# endif
		fnamecmp(si->sn_name, fname_exp) == 0))
	    break;
    }
    if (current_SID == 0)
    {
	current_SID = ++last_current_SID;
	if (ga_grow(&script_items, (int)(current_SID - script_items.ga_len))
								      == FAIL)
	    goto almosttheend;
	while (script_items.ga_len < current_SID)
	{
	    ++script_items.ga_len;
	    SCRIPT_ITEM(script_items.ga_len).sn_name = NULL;
# ifdef FEAT_PROFILE
	    SCRIPT_ITEM(script_items.ga_len).sn_prof_on = FALSE;
# endif
	}
	si = &SCRIPT_ITEM(current_SID);
	si->sn_name = fname_exp;
	fname_exp = NULL;
# ifdef UNIX
	if (stat_ok)
	{
	    si->sn_dev = st.st_dev;
	    si->sn_ino = st.st_ino;
	}
	else
	    si->sn_dev = -1;
# endif

	/* Allocate the local script variables to use for this script. */
	new_script_vars(current_SID);
    }

# ifdef FEAT_PROFILE
    if (do_profiling)
    {
	int	forceit;

	/* Check if we do profiling for this script. */
	if (!si->sn_prof_on && has_profiling(TRUE, si->sn_name, &forceit))
	{
	    script_do_profile(si);
	    si->sn_pr_force = forceit;
	}
	if (si->sn_prof_on)
	{
	    ++si->sn_pr_count;
	    profile_start(&si->sn_pr_start);
	    profile_zero(&si->sn_pr_children);
	}
    }
# endif
#endif

    /*
     * Call do_cmdline, which will call getsourceline() to get the lines.
     */
    do_cmdline(NULL, getsourceline, (void *)&cookie,
				     DOCMD_VERBOSE|DOCMD_NOWAIT|DOCMD_REPEAT);

    retval = OK;

#ifdef FEAT_PROFILE
    if (do_profiling)
    {
	/* Get "si" again, "script_items" may have been reallocated. */
	si = &SCRIPT_ITEM(current_SID);
	if (si->sn_prof_on)
	{
	    profile_end(&si->sn_pr_start);
	    profile_sub_wait(&wait_start, &si->sn_pr_start);
	    profile_add(&si->sn_pr_total, &si->sn_pr_start);
	    profile_add(&si->sn_pr_self, &si->sn_pr_start);
	    profile_sub(&si->sn_pr_self, &si->sn_pr_children);
	}
    }
#endif

    if (got_int)
	EMSG(_(e_interr));
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    if (p_verbose > 1)
    {
	smsg((char_u *)_("finished sourcing %s"), fname);
	if (sourcing_name != NULL)
	    smsg((char_u *)_("continuing in %s"), sourcing_name);
    }
#ifdef STARTUPTIME
    vim_snprintf(IObuff, IOSIZE, "sourcing %s", fname);
    time_msg(IObuff, &tv_start);
    time_pop(&tv_rel);
#endif

#ifdef FEAT_EVAL
    /*
     * After a "finish" in debug mode, need to break at first command of next
     * sourced file.
     */
    if (save_debug_break_level > ex_nesting_level
	    && debug_break_level == ex_nesting_level)
	++debug_break_level;
#endif

#ifdef FEAT_EVAL
almosttheend:
    current_SID = save_current_SID;
    restore_funccal(save_funccalp);
# ifdef FEAT_PROFILE
    if (do_profiling)
	prof_child_exit(&wait_start);		/* leaving a child now */
# endif
#endif
    fclose(cookie.fp);
    vim_free(cookie.nextline);
#ifdef FEAT_MBYTE
    convert_setup(&cookie.conv, NULL, NULL);
#endif

theend:
    vim_free(fname_exp);
    return retval;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * ":scriptnames"
 */
/*ARGSUSED*/
    void
ex_scriptnames(eap)
    exarg_T	*eap;
{
    int i;

    for (i = 1; i <= script_items.ga_len && !got_int; ++i)
	if (SCRIPT_ITEM(i).sn_name != NULL)
	    smsg((char_u *)"%3d: %s", i, SCRIPT_ITEM(i).sn_name);
}

# if defined(BACKSLASH_IN_FILENAME) || defined(PROTO)
/*
 * Fix slashes in the list of script names for 'shellslash'.
 */
    void
scriptnames_slash_adjust()
{
    int i;

    for (i = 1; i <= script_items.ga_len; ++i)
	if (SCRIPT_ITEM(i).sn_name != NULL)
	    slash_adjust(SCRIPT_ITEM(i).sn_name);
}
# endif

/*
 * Get a pointer to a script name.  Used for ":verbose set".
 */
    char_u *
get_scriptname(id)
    scid_T	id;
{
    if (id == SID_MODELINE)
	return (char_u *)"modeline";
    if (id == SID_CMDARG)
	return (char_u *)"--cmd argument";
    if (id == SID_CARG)
	return (char_u *)"-c argument";
    if (id == SID_ENV)
	return (char_u *)"environment variable";
    return SCRIPT_ITEM(id).sn_name;
}

#endif

#if defined(USE_CR) || defined(PROTO)

# if defined(__MSL__) && (__MSL__ >= 22)
/*
 * Newer version of the Metrowerks library handle DOS and UNIX files
 * without help.
 * Test with earlier versions, MSL 2.2 is the library supplied with
 * Codewarrior Pro 2.
 */
    char *
fgets_cr(s, n, stream)
    char	*s;
    int		n;
    FILE	*stream;
{
    return fgets(s, n, stream);
}
# else
/*
 * Version of fgets() which also works for lines ending in a <CR> only
 * (Macintosh format).
 * For older versions of the Metrowerks library.
 * At least CodeWarrior 9 needed this code.
 */
    char *
fgets_cr(s, n, stream)
    char	*s;
    int		n;
    FILE	*stream;
{
    int	c = 0;
    int char_read = 0;

    while (!feof(stream) && c != '\r' && c != '\n' && char_read < n - 1)
    {
	c = fgetc(stream);
	s[char_read++] = c;
	/* If the file is in DOS format, we need to skip a NL after a CR.  I
	 * thought it was the other way around, but this appears to work... */
	if (c == '\n')
	{
	    c = fgetc(stream);
	    if (c != '\r')
		ungetc(c, stream);
	}
    }

    s[char_read] = 0;
    if (char_read == 0)
	return NULL;

    if (feof(stream) && char_read == 1)
	return NULL;

    return s;
}
# endif
#endif

/*
 * Get one full line from a sourced file.
 * Called by do_cmdline() when it's called from do_source().
 *
 * Return a pointer to the line in allocated memory.
 * Return NULL for end-of-file or some error.
 */
/* ARGSUSED */
    char_u *
getsourceline(c, cookie, indent)
    int		c;		/* not used */
    void	*cookie;
    int		indent;		/* not used */
{
    struct source_cookie *sp = (struct source_cookie *)cookie;
    char_u		*line;
    char_u		*p, *s;

#ifdef FEAT_EVAL
    /* If breakpoints have been added/deleted need to check for it. */
    if (sp->dbg_tick < debug_tick)
    {
	sp->breakpoint = dbg_find_breakpoint(TRUE, sp->fname, sourcing_lnum);
	sp->dbg_tick = debug_tick;
    }
# ifdef FEAT_PROFILE
    if (do_profiling)
	script_line_end();
# endif
#endif
    /*
     * Get current line.  If there is a read-ahead line, use it, otherwise get
     * one now.
     */
    if (sp->finished)
	line = NULL;
    else if (sp->nextline == NULL)
	line = get_one_sourceline(sp);
    else
    {
	line = sp->nextline;
	sp->nextline = NULL;
	++sourcing_lnum;
    }
#ifdef FEAT_PROFILE
    if (line != NULL && do_profiling)
	script_line_start();
#endif

    /* Only concatenate lines starting with a \ when 'cpoptions' doesn't
     * contain the 'C' flag. */
    if (line != NULL && (vim_strchr(p_cpo, CPO_CONCAT) == NULL))
    {
	/* compensate for the one line read-ahead */
	--sourcing_lnum;
	for (;;)
	{
	    sp->nextline = get_one_sourceline(sp);
	    if (sp->nextline == NULL)
		break;
	    p = skipwhite(sp->nextline);
	    if (*p != '\\')
		break;
	    s = alloc((int)(STRLEN(line) + STRLEN(p)));
	    if (s == NULL)	/* out of memory */
		break;
	    STRCPY(s, line);
	    STRCAT(s, p + 1);
	    vim_free(line);
	    line = s;
	    vim_free(sp->nextline);
	}
    }

#ifdef FEAT_MBYTE
    if (line != NULL && sp->conv.vc_type != CONV_NONE)
    {
	/* Convert the encoding of the script line. */
	s = string_convert(&sp->conv, line, NULL);
	if (s != NULL)
	{
	    vim_free(line);
	    line = s;
	}
    }
#endif

#ifdef FEAT_EVAL
    /* Did we encounter a breakpoint? */
    if (sp->breakpoint != 0 && sp->breakpoint <= sourcing_lnum)
    {
	dbg_breakpoint(sp->fname, sourcing_lnum);
	/* Find next breakpoint. */
	sp->breakpoint = dbg_find_breakpoint(TRUE, sp->fname, sourcing_lnum);
	sp->dbg_tick = debug_tick;
    }
#endif

    return line;
}

    static char_u *
get_one_sourceline(sp)
    struct source_cookie    *sp;
{
    garray_T		ga;
    int			len;
    int			c;
    char_u		*buf;
#ifdef USE_CRNL
    int			has_cr;		/* CR-LF found */
#endif
#ifdef USE_CR
    char_u		*scan;
#endif
    int			have_read = FALSE;

    /* use a growarray to store the sourced line */
    ga_init2(&ga, 1, 250);

    /*
     * Loop until there is a finished line (or end-of-file).
     */
    sourcing_lnum++;
    for (;;)
    {
	/* make room to read at least 120 (more) characters */
	if (ga_grow(&ga, 120) == FAIL)
	    break;
	buf = (char_u *)ga.ga_data;

#ifdef USE_CR
	if (sp->fileformat == EOL_MAC)
	{
	    if (fgets_cr((char *)buf + ga.ga_len, ga.ga_maxlen - ga.ga_len,
							      sp->fp) == NULL)
		break;
	}
	else
#endif
	    if (fgets((char *)buf + ga.ga_len, ga.ga_maxlen - ga.ga_len,
							      sp->fp) == NULL)
		break;
	len = ga.ga_len + (int)STRLEN(buf + ga.ga_len);
#ifdef USE_CRNL
	/* Ignore a trailing CTRL-Z, when in Dos mode.	Only recognize the
	 * CTRL-Z by its own, or after a NL. */
	if (	   (len == 1 || (len >= 2 && buf[len - 2] == '\n'))
		&& sp->fileformat == EOL_DOS
		&& buf[len - 1] == Ctrl_Z)
	{
	    buf[len - 1] = NUL;
	    break;
	}
#endif

#ifdef USE_CR
	/* If the read doesn't stop on a new line, and there's
	 * some CR then we assume a Mac format */
	if (sp->fileformat == EOL_UNKNOWN)
	{
	    if (buf[len - 1] != '\n' && vim_strchr(buf, '\r') != NULL)
		sp->fileformat = EOL_MAC;
	    else
		sp->fileformat = EOL_UNIX;
	}

	if (sp->fileformat == EOL_MAC)
	{
	    scan = vim_strchr(buf, '\r');

	    if (scan != NULL)
	    {
		*scan = '\n';
		if (*(scan + 1) != 0)
		{
		    *(scan + 1) = 0;
		    fseek(sp->fp, (long)(scan - buf - len + 1), SEEK_CUR);
		}
	    }
	    len = STRLEN(buf);
	}
#endif

	have_read = TRUE;
	ga.ga_len = len;

	/* If the line was longer than the buffer, read more. */
	if (ga.ga_maxlen - ga.ga_len == 1 && buf[len - 1] != '\n')
	    continue;

	if (len >= 1 && buf[len - 1] == '\n')	/* remove trailing NL */
	{
#ifdef USE_CRNL
	    has_cr = (len >= 2 && buf[len - 2] == '\r');
	    if (sp->fileformat == EOL_UNKNOWN)
	    {
		if (has_cr)
		    sp->fileformat = EOL_DOS;
		else
		    sp->fileformat = EOL_UNIX;
	    }

	    if (sp->fileformat == EOL_DOS)
	    {
		if (has_cr)	    /* replace trailing CR */
		{
		    buf[len - 2] = '\n';
		    --len;
		    --ga.ga_len;
		}
		else	    /* lines like ":map xx yy^M" will have failed */
		{
		    if (!sp->error)
		    {
			msg_source(hl_attr(HLF_W));
			EMSG(_("W15: Warning: Wrong line separator, ^M may be missing"));
		    }
		    sp->error = TRUE;
		    sp->fileformat = EOL_UNIX;
		}
	    }
#endif
	    /* The '\n' is escaped if there is an odd number of ^V's just
	     * before it, first set "c" just before the 'V's and then check
	     * len&c parities (is faster than ((len-c)%2 == 0)) -- Acevedo */
	    for (c = len - 2; c >= 0 && buf[c] == Ctrl_V; c--)
		;
	    if ((len & 1) != (c & 1))	/* escaped NL, read more */
	    {
		sourcing_lnum++;
		continue;
	    }

	    buf[len - 1] = NUL;		/* remove the NL */
	}

	/*
	 * Check for ^C here now and then, so recursive :so can be broken.
	 */
	line_breakcheck();
	break;
    }

    if (have_read)
	return (char_u *)ga.ga_data;

    vim_free(ga.ga_data);
    return NULL;
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Called when starting to read a script line.
 * "sourcing_lnum" must be correct!
 * When skipping lines it may not actually be executed, but we won't find out
 * until later and we need to store the time now.
 */
    void
script_line_start()
{
    scriptitem_T    *si;
    sn_prl_T	    *pp;

    if (current_SID <= 0 || current_SID > script_items.ga_len)
	return;
    si = &SCRIPT_ITEM(current_SID);
    if (si->sn_prof_on && sourcing_lnum >= 1)
    {
	/* Grow the array before starting the timer, so that the time spend
	 * here isn't counted. */
	ga_grow(&si->sn_prl_ga, (int)(sourcing_lnum - si->sn_prl_ga.ga_len));
	si->sn_prl_idx = sourcing_lnum - 1;
	while (si->sn_prl_ga.ga_len <= si->sn_prl_idx
		&& si->sn_prl_ga.ga_len < si->sn_prl_ga.ga_maxlen)
	{
	    /* Zero counters for a line that was not used before. */
	    pp = &PRL_ITEM(si, si->sn_prl_ga.ga_len);
	    pp->snp_count = 0;
	    profile_zero(&pp->sn_prl_total);
	    profile_zero(&pp->sn_prl_self);
	    ++si->sn_prl_ga.ga_len;
	}
	si->sn_prl_execed = FALSE;
	profile_start(&si->sn_prl_start);
	profile_zero(&si->sn_prl_children);
	profile_get_wait(&si->sn_prl_wait);
    }
}

/*
 * Called when actually executing a function line.
 */
    void
script_line_exec()
{
    scriptitem_T    *si;

    if (current_SID <= 0 || current_SID > script_items.ga_len)
	return;
    si = &SCRIPT_ITEM(current_SID);
    if (si->sn_prof_on && si->sn_prl_idx >= 0)
	si->sn_prl_execed = TRUE;
}

/*
 * Called when done with a function line.
 */
    void
script_line_end()
{
    scriptitem_T    *si;
    sn_prl_T	    *pp;

    if (current_SID <= 0 || current_SID > script_items.ga_len)
	return;
    si = &SCRIPT_ITEM(current_SID);
    if (si->sn_prof_on && si->sn_prl_idx >= 0
				     && si->sn_prl_idx < si->sn_prl_ga.ga_len)
    {
	if (si->sn_prl_execed)
	{
	    pp = &PRL_ITEM(si, si->sn_prl_idx);
	    ++pp->snp_count;
	    profile_end(&si->sn_prl_start);
	    profile_sub_wait(&si->sn_prl_wait, &si->sn_prl_start);
	    profile_add(&pp->sn_prl_self, &si->sn_prl_start);
	    profile_add(&pp->sn_prl_total, &si->sn_prl_start);
	    profile_sub(&pp->sn_prl_self, &si->sn_prl_children);
	}
	si->sn_prl_idx = -1;
    }
}
#endif

/*
 * ":scriptencoding": Set encoding conversion for a sourced script.
 * Without the multi-byte feature it's simply ignored.
 */
/*ARGSUSED*/
    void
ex_scriptencoding(eap)
    exarg_T	*eap;
{
#ifdef FEAT_MBYTE
    struct source_cookie	*sp;
    char_u			*name;

    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	EMSG(_("E167: :scriptencoding used outside of a sourced file"));
	return;
    }

    if (*eap->arg != NUL)
    {
	name = enc_canonize(eap->arg);
	if (name == NULL)	/* out of memory */
	    return;
    }
    else
	name = eap->arg;

    /* Setup for conversion from the specified encoding to 'encoding'. */
    sp = (struct source_cookie *)getline_cookie(eap->getline, eap->cookie);
    convert_setup(&sp->conv, name, p_enc);

    if (name != eap->arg)
	vim_free(name);
#endif
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * ":finish": Mark a sourced file as finished.
 */
    void
ex_finish(eap)
    exarg_T	*eap;
{
    if (getline_equal(eap->getline, eap->cookie, getsourceline))
	do_finish(eap, FALSE);
    else
	EMSG(_("E168: :finish used outside of a sourced file"));
}

/*
 * Mark a sourced file as finished.  Possibly makes the ":finish" pending.
 * Also called for a pending finish at the ":endtry" or after returning from
 * an extra do_cmdline().  "reanimate" is used in the latter case.
 */
    void
do_finish(eap, reanimate)
    exarg_T	*eap;
    int		reanimate;
{
    int		idx;

    if (reanimate)
	((struct source_cookie *)getline_cookie(eap->getline,
					      eap->cookie))->finished = FALSE;

    /*
     * Cleanup (and inactivate) conditionals, but stop when a try conditional
     * not in its finally clause (which then is to be executed next) is found.
     * In this case, make the ":finish" pending for execution at the ":endtry".
     * Otherwise, finish normally.
     */
    idx = cleanup_conditionals(eap->cstack, 0, TRUE);
    if (idx >= 0)
    {
	eap->cstack->cs_pending[idx] = CSTP_FINISH;
	report_make_pending(CSTP_FINISH, NULL);
    }
    else
	((struct source_cookie *)getline_cookie(eap->getline,
					       eap->cookie))->finished = TRUE;
}


/*
 * Return TRUE when a sourced file had the ":finish" command: Don't give error
 * message for missing ":endif".
 * Return FALSE when not sourcing a file.
 */
    int
source_finished(getline, cookie)
    char_u	*(*getline) __ARGS((int, void *, int));
    void	*cookie;
{
    return (getline_equal(getline, cookie, getsourceline)
	    && ((struct source_cookie *)getline_cookie(
						 getline, cookie))->finished);
}
#endif

#if defined(FEAT_LISTCMDS) || defined(PROTO)
/*
 * ":checktime [buffer]"
 */
    void
ex_checktime(eap)
    exarg_T	*eap;
{
    buf_T	*buf;
    int		save_no_check_timestamps = no_check_timestamps;

    no_check_timestamps = 0;
    if (eap->addr_count == 0)	/* default is all buffers */
	check_timestamps(FALSE);
    else
    {
	buf = buflist_findnr((int)eap->line2);
	if (buf != NULL)	/* cannot happen? */
	    (void)buf_check_timestamp(buf, FALSE);
    }
    no_check_timestamps = save_no_check_timestamps;
}
#endif

#if defined(FEAT_PRINTER) || defined(PROTO)
/*
 * Printing code (Machine-independent.)
 * To implement printing on a platform, the following functions must be
 * defined:
 *
 * int mch_print_init(prt_settings_T *psettings, char_u *jobname, int forceit)
 * Called once.  Code should display printer dialogue (if appropriate) and
 * determine printer font and margin settings.  Reset has_color if the printer
 * doesn't support colors at all.
 * Returns FAIL to abort.
 *
 * int mch_print_begin(prt_settings_T *settings)
 * Called to start the print job.
 * Return FALSE to abort.
 *
 * int mch_print_begin_page(char_u *msg)
 * Called at the start of each page.
 * "msg" indicates the progress of the print job, can be NULL.
 * Return FALSE to abort.
 *
 * int mch_print_end_page()
 * Called at the end of each page.
 * Return FALSE to abort.
 *
 * int mch_print_blank_page()
 * Called to generate a blank page for collated, duplex, multiple copy
 * document.  Return FALSE to abort.
 *
 * void mch_print_end(prt_settings_T *psettings)
 * Called at normal end of print job.
 *
 * void mch_print_cleanup()
 * Called if print job ends normally or is abandoned. Free any memory, close
 * devices and handles.  Also called when mch_print_begin() fails, but not
 * when mch_print_init() fails.
 *
 * void mch_print_set_font(int Bold, int Italic, int Underline);
 * Called whenever the font style changes.
 *
 * void mch_print_set_bg(long bgcol);
 * Called to set the background color for the following text. Parameter is an
 * RGB value.
 *
 * void mch_print_set_fg(long fgcol);
 * Called to set the foreground color for the following text. Parameter is an
 * RGB value.
 *
 * mch_print_start_line(int margin, int page_line)
 * Sets the current position at the start of line "page_line".
 * If margin is TRUE start in the left margin (for header and line number).
 *
 * int mch_print_text_out(char_u *p, int len);
 * Output one character of text p[len] at the current position.
 * Return TRUE if there is no room for another character in the same line.
 *
 * Note that the generic code has no idea of margins. The machine code should
 * simply make the page look smaller!  The header and the line numbers are
 * printed in the margin.
 */

#ifdef FEAT_SYN_HL
static const long_u  cterm_color_8[8] =
{
    (long_u)0x000000L, (long_u)0xff0000L, (long_u)0x00ff00L, (long_u)0xffff00L,
    (long_u)0x0000ffL, (long_u)0xff00ffL, (long_u)0x00ffffL, (long_u)0xffffffL
};

static const long_u  cterm_color_16[16] =
{
    (long_u)0x000000L, (long_u)0x0000c0L, (long_u)0x008000L, (long_u)0x004080L,
    (long_u)0xc00000L, (long_u)0xc000c0L, (long_u)0x808000L, (long_u)0xc0c0c0L,
    (long_u)0x808080L, (long_u)0x6060ffL, (long_u)0x00ff00L, (long_u)0x00ffffL,
    (long_u)0xff8080L, (long_u)0xff40ffL, (long_u)0xffff00L, (long_u)0xffffffL
};

static int		current_syn_id;
#endif

#define PRCOLOR_BLACK	(long_u)0
#define PRCOLOR_WHITE	(long_u)0xFFFFFFL

static int	curr_italic;
static int	curr_bold;
static int	curr_underline;
static long_u	curr_bg;
static long_u	curr_fg;
static int	page_count;

/*
 * These values determine the print position on a page.
 */
typedef struct
{
    int		lead_spaces;	    /* remaining spaces for a TAB */
    int		print_pos;	    /* virtual column for computing TABs */
    colnr_T	column;		    /* byte column */
    linenr_T	file_line;	    /* line nr in the buffer */
    long_u	bytes_printed;	    /* bytes printed so far */
    int		ff;		    /* seen form feed character */
} prt_pos_T;

#ifdef FEAT_SYN_HL
static long_u darken_rgb __ARGS((long_u rgb));
static long_u prt_get_term_color __ARGS((int colorindex));
#endif
static void prt_set_fg __ARGS((long_u fg));
static void prt_set_bg __ARGS((long_u bg));
static void prt_set_font __ARGS((int bold, int italic, int underline));
static void prt_line_number __ARGS((prt_settings_T *psettings, int page_line, linenr_T lnum));
static void prt_header __ARGS((prt_settings_T *psettings, int pagenum, linenr_T lnum));
static void prt_message __ARGS((char_u *s));
static colnr_T hardcopy_line __ARGS((prt_settings_T *psettings, int page_line, prt_pos_T *ppos));
static void prt_get_attr __ARGS((int hl_id, prt_text_attr_T* pattr, int modec));

#ifdef FEAT_SYN_HL
/*
 * If using a dark background, the colors will probably be too bright to show
 * up well on white paper, so reduce their brightness.
 */
    static long_u
darken_rgb(rgb)
    long_u	rgb;
{
    return	((rgb >> 17) << 16)
	    +	(((rgb & 0xff00) >> 9) << 8)
	    +	((rgb & 0xff) >> 1);
}

    static long_u
prt_get_term_color(colorindex)
    int	    colorindex;
{
    /* TODO: Should check for xterm with 88 or 256 colors. */
    if (t_colors > 8)
	return cterm_color_16[colorindex % 16];
    return cterm_color_8[colorindex % 8];
}

    static void
prt_get_attr(hl_id, pattr, modec)
    int			hl_id;
    prt_text_attr_T	*pattr;
    int			modec;
{
    int     colorindex;
    long_u  fg_color;
    long_u  bg_color;
    char    *color;

    pattr->bold = (highlight_has_attr(hl_id, HL_BOLD, modec) != NULL);
    pattr->italic = (highlight_has_attr(hl_id, HL_ITALIC, modec) != NULL);
    pattr->underline = (highlight_has_attr(hl_id, HL_UNDERLINE, modec) != NULL);
    pattr->undercurl = (highlight_has_attr(hl_id, HL_UNDERCURL, modec) != NULL);

# ifdef FEAT_GUI
    if (gui.in_use)
    {
	bg_color = highlight_gui_color_rgb(hl_id, FALSE);
	if (bg_color == PRCOLOR_BLACK)
	    bg_color = PRCOLOR_WHITE;

	fg_color = highlight_gui_color_rgb(hl_id, TRUE);
    }
    else
# endif
    {
	bg_color = PRCOLOR_WHITE;

	color = (char *)highlight_color(hl_id, (char_u *)"fg", modec);
	if (color == NULL)
	    colorindex = 0;
	else
	    colorindex = atoi(color);

	if (colorindex >= 0 && colorindex < t_colors)
	    fg_color = prt_get_term_color(colorindex);
	else
	    fg_color = PRCOLOR_BLACK;
    }

    if (fg_color == PRCOLOR_WHITE)
	fg_color = PRCOLOR_BLACK;
    else if (*p_bg == 'd')
	fg_color = darken_rgb(fg_color);

    pattr->fg_color = fg_color;
    pattr->bg_color = bg_color;
}
#endif /* FEAT_SYN_HL */

    static void
prt_set_fg(fg)
    long_u fg;
{
    if (fg != curr_fg)
    {
	curr_fg = fg;
	mch_print_set_fg(fg);
    }
}

    static void
prt_set_bg(bg)
    long_u bg;
{
    if (bg != curr_bg)
    {
	curr_bg = bg;
	mch_print_set_bg(bg);
    }
}

    static void
prt_set_font(bold, italic, underline)
    int		bold;
    int		italic;
    int		underline;
{
    if (curr_bold != bold
	    || curr_italic != italic
	    || curr_underline != underline)
    {
	curr_underline = underline;
	curr_italic = italic;
	curr_bold = bold;
	mch_print_set_font(bold, italic, underline);
    }
}

/*
 * Print the line number in the left margin.
 */
    static void
prt_line_number(psettings, page_line, lnum)
    prt_settings_T *psettings;
    int		page_line;
    linenr_T	lnum;
{
    int		i;
    char_u	tbuf[20];

    prt_set_fg(psettings->number.fg_color);
    prt_set_bg(psettings->number.bg_color);
    prt_set_font(psettings->number.bold, psettings->number.italic, psettings->number.underline);
    mch_print_start_line(TRUE, page_line);

    /* Leave two spaces between the number and the text; depends on
     * PRINT_NUMBER_WIDTH. */
    sprintf((char *)tbuf, "%6ld", (long)lnum);
    for (i = 0; i < 6; i++)
	(void)mch_print_text_out(&tbuf[i], 1);

#ifdef FEAT_SYN_HL
    if (psettings->do_syntax)
	/* Set colors for next character. */
	current_syn_id = -1;
    else
#endif
    {
	/* Set colors and font back to normal. */
	prt_set_fg(PRCOLOR_BLACK);
	prt_set_bg(PRCOLOR_WHITE);
	prt_set_font(FALSE, FALSE, FALSE);
    }
}

static linenr_T printer_page_num;

    int
get_printer_page_num()
{
    return printer_page_num;
}

/*
 * Get the currently effective header height.
 */
    int
prt_header_height()
{
    if (printer_opts[OPT_PRINT_HEADERHEIGHT].present)
	return printer_opts[OPT_PRINT_HEADERHEIGHT].number;
    return 2;
}

/*
 * Return TRUE if using a line number for printing.
 */
    int
prt_use_number()
{
    return (printer_opts[OPT_PRINT_NUMBER].present
	    && TOLOWER_ASC(printer_opts[OPT_PRINT_NUMBER].string[0]) == 'y');
}

/*
 * Return the unit used in a margin item in 'printoptions'.
 * Returns PRT_UNIT_NONE if not recognized.
 */
    int
prt_get_unit(idx)
    int		idx;
{
    int		u = PRT_UNIT_NONE;
    int		i;
    static char *(units[4]) = PRT_UNIT_NAMES;

    if (printer_opts[idx].present)
	for (i = 0; i < 4; ++i)
	    if (STRNICMP(printer_opts[idx].string, units[i], 2) == 0)
	    {
		u = i;
		break;
	    }
    return u;
}

/*
 * Print the page header.
 */
/*ARGSUSED*/
    static void
prt_header(psettings, pagenum, lnum)
    prt_settings_T  *psettings;
    int		pagenum;
    linenr_T	lnum;
{
    int		width = psettings->chars_per_line;
    int		page_line;
    char_u	*tbuf;
    char_u	*p;
#ifdef FEAT_MBYTE
    int		l;
#endif

    /* Also use the space for the line number. */
    if (prt_use_number())
	width += PRINT_NUMBER_WIDTH;

    tbuf = alloc(width + IOSIZE);
    if (tbuf == NULL)
	return;

#ifdef FEAT_STL_OPT
    if (*p_header != NUL)
    {
	linenr_T	tmp_lnum, tmp_topline, tmp_botline;

	/*
	 * Need to (temporarily) set current line number and first/last line
	 * number on the 'window'.  Since we don't know how long the page is,
	 * set the first and current line number to the top line, and guess
	 * that the page length is 64.
	 */
	tmp_lnum = curwin->w_cursor.lnum;
	tmp_topline = curwin->w_topline;
	tmp_botline = curwin->w_botline;
	curwin->w_cursor.lnum = lnum;
	curwin->w_topline = lnum;
	curwin->w_botline = lnum + 63;
	printer_page_num = pagenum;

	build_stl_str_hl(curwin, tbuf, (size_t)(width + IOSIZE),
						  p_header, ' ', width, NULL);

	/* Reset line numbers */
	curwin->w_cursor.lnum = tmp_lnum;
	curwin->w_topline = tmp_topline;
	curwin->w_botline = tmp_botline;
    }
    else
#endif
	sprintf((char *)tbuf, _("Page %d"), pagenum);

    prt_set_fg(PRCOLOR_BLACK);
    prt_set_bg(PRCOLOR_WHITE);
    prt_set_font(TRUE, FALSE, FALSE);

    /* Use a negative line number to indicate printing in the top margin. */
    page_line = 0 - prt_header_height();
    mch_print_start_line(TRUE, page_line);
    for (p = tbuf; *p != NUL; )
    {
	if (mch_print_text_out(p,
#ifdef FEAT_MBYTE
		(l = (*mb_ptr2len_check)(p))
#else
		1
#endif
		    ))
	{
	    ++page_line;
	    if (page_line >= 0) /* out of room in header */
		break;
	    mch_print_start_line(TRUE, page_line);
	}
#ifdef FEAT_MBYTE
	p += l;
#else
	p++;
#endif
    }

    vim_free(tbuf);

#ifdef FEAT_SYN_HL
    if (psettings->do_syntax)
	/* Set colors for next character. */
	current_syn_id = -1;
    else
#endif
    {
	/* Set colors and font back to normal. */
	prt_set_fg(PRCOLOR_BLACK);
	prt_set_bg(PRCOLOR_WHITE);
	prt_set_font(FALSE, FALSE, FALSE);
    }
}

/*
 * Display a print status message.
 */
    static void
prt_message(s)
    char_u	*s;
{
    screen_fill((int)Rows - 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
    screen_puts(s, (int)Rows - 1, 0, hl_attr(HLF_R));
    out_flush();
}

    void
ex_hardcopy(eap)
    exarg_T	*eap;
{
    linenr_T		lnum;
    int			collated_copies, uncollated_copies;
    prt_settings_T	settings;
    long_u		bytes_to_print = 0;
    int			page_line;
    int			jobsplit;
    int			id;

    memset(&settings, 0, sizeof(prt_settings_T));
    settings.has_color = TRUE;

# ifdef FEAT_POSTSCRIPT
    if (*eap->arg == '>')
    {
	char_u	*errormsg = NULL;

	/* Expand things like "%.ps". */
	if (expand_filename(eap, eap->cmdlinep, &errormsg) == FAIL)
	{
	    if (errormsg != NULL)
		EMSG(errormsg);
	    return;
	}
	settings.outfile = skipwhite(eap->arg + 1);
    }
    else if (*eap->arg != NUL)
	settings.arguments = eap->arg;
# endif

    /*
     * Initialise for printing.  Ask the user for settings, unless forceit is
     * set.
     * The mch_print_init() code should set up margins if applicable. (It may
     * not be a real printer - for example the engine might generate HTML or
     * PS.)
     */
    if (mch_print_init(&settings,
			curbuf->b_fname == NULL
			    ? (char_u *)buf_spname(curbuf)
			    : curbuf->b_sfname == NULL
				? curbuf->b_fname
				: curbuf->b_sfname,
			eap->forceit) == FAIL)
	return;

#ifdef FEAT_SYN_HL
# ifdef  FEAT_GUI
    if (gui.in_use)
	settings.modec = 'g';
    else
# endif
	if (t_colors > 1)
	    settings.modec = 'c';
	else
	    settings.modec = 't';

    if (!syntax_present(curbuf))
	settings.do_syntax = FALSE;
    else if (printer_opts[OPT_PRINT_SYNTAX].present
	    && TOLOWER_ASC(printer_opts[OPT_PRINT_SYNTAX].string[0]) != 'a')
	settings.do_syntax =
	       (TOLOWER_ASC(printer_opts[OPT_PRINT_SYNTAX].string[0]) == 'y');
    else
	settings.do_syntax = settings.has_color;
#endif

    /* Set up printing attributes for line numbers */
    settings.number.fg_color = PRCOLOR_BLACK;
    settings.number.bg_color = PRCOLOR_WHITE;
    settings.number.bold = FALSE;
    settings.number.italic = TRUE;
    settings.number.underline = FALSE;
#ifdef FEAT_SYN_HL
    /*
     * Syntax highlighting of line numbers.
     */
    if (prt_use_number() && settings.do_syntax)
    {
	id = syn_name2id((char_u *)"LineNr");
	if (id > 0)
	    id = syn_get_final_id(id);

	prt_get_attr(id, &settings.number, settings.modec);
    }
#endif /* FEAT_SYN_HL */

    /*
     * Estimate the total lines to be printed
     */
    for (lnum = eap->line1; lnum <= eap->line2; lnum++)
	bytes_to_print += (long_u)STRLEN(skipwhite(ml_get(lnum)));
    if (bytes_to_print == 0)
    {
	MSG(_("No text to be printed"));
	goto print_fail_no_begin;
    }

    /* Set colors and font to normal. */
    curr_bg = (long_u)0xffffffffL;
    curr_fg = (long_u)0xffffffffL;
    curr_italic = MAYBE;
    curr_bold = MAYBE;
    curr_underline = MAYBE;

    prt_set_fg(PRCOLOR_BLACK);
    prt_set_bg(PRCOLOR_WHITE);
    prt_set_font(FALSE, FALSE, FALSE);
#ifdef FEAT_SYN_HL
    current_syn_id = -1;
#endif

    jobsplit = (printer_opts[OPT_PRINT_JOBSPLIT].present
	   && TOLOWER_ASC(printer_opts[OPT_PRINT_JOBSPLIT].string[0]) == 'y');

    if (!mch_print_begin(&settings))
	goto print_fail_no_begin;

    /*
     * Loop over collated copies: 1 2 3, 1 2 3, ...
     */
    page_count = 0;
    for (collated_copies = 0;
	    collated_copies < settings.n_collated_copies;
	    collated_copies++)
    {
	prt_pos_T	prtpos;		/* current print position */
	prt_pos_T	page_prtpos;	/* print position at page start */
	int		side;

	memset(&page_prtpos, 0, sizeof(prt_pos_T));
	page_prtpos.file_line = eap->line1;
	prtpos = page_prtpos;

	if (jobsplit && collated_copies > 0)
	{
	    /* Splitting jobs: Stop a previous job and start a new one. */
	    mch_print_end(&settings);
	    if (!mch_print_begin(&settings))
		goto print_fail_no_begin;
	}

	/*
	 * Loop over all pages in the print job: 1 2 3 ...
	 */
	for (page_count = 0; prtpos.file_line <= eap->line2; ++page_count)
	{
	    /*
	     * Loop over uncollated copies: 1 1 1, 2 2 2, 3 3 3, ...
	     * For duplex: 12 12 12 34 34 34, ...
	     */
	    for (uncollated_copies = 0;
		    uncollated_copies < settings.n_uncollated_copies;
		    uncollated_copies++)
	    {
		/* Set the print position to the start of this page. */
		prtpos = page_prtpos;

		/*
		 * Do front and rear side of a page.
		 */
		for (side = 0; side <= settings.duplex; ++side)
		{
		    /*
		     * Print one page.
		     */

		    /* Check for interrupt character every page. */
		    ui_breakcheck();
		    if (got_int || settings.user_abort)
			goto print_fail;

		    sprintf((char *)IObuff, _("Printing page %d (%d%%)"),
			    page_count + 1 + side,
			    prtpos.bytes_printed > 1000000
				? (int)(prtpos.bytes_printed /
						       (bytes_to_print / 100))
				: (int)((prtpos.bytes_printed * 100)
							   / bytes_to_print));
		    if (!mch_print_begin_page(IObuff))
			goto print_fail;

		    if (settings.n_collated_copies > 1)
			sprintf((char *)IObuff + STRLEN(IObuff),
				_(" Copy %d of %d"),
				collated_copies + 1,
				settings.n_collated_copies);
		    prt_message(IObuff);

		    /*
		     * Output header if required
		     */
		    if (prt_header_height() > 0)
			prt_header(&settings, page_count + 1 + side,
							prtpos.file_line);

		    for (page_line = 0; page_line < settings.lines_per_page;
								  ++page_line)
		    {
			prtpos.column = hardcopy_line(&settings,
							   page_line, &prtpos);
			if (prtpos.column == 0)
			{
			    /* finished a file line */
			    prtpos.bytes_printed +=
				  STRLEN(skipwhite(ml_get(prtpos.file_line)));
			    if (++prtpos.file_line > eap->line2)
				break; /* reached the end */
			}
			else if (prtpos.ff)
			{
			    /* Line had a formfeed in it - start new page but
			     * stay on the current line */
			    break;
			}
		    }

		    if (!mch_print_end_page())
			goto print_fail;
		    if (prtpos.file_line > eap->line2)
			break; /* reached the end */
		}

		/*
		 * Extra blank page for duplexing with odd number of pages and
		 * more copies to come.
		 */
		if (prtpos.file_line > eap->line2 && settings.duplex
								 && side == 0
		    && uncollated_copies + 1 < settings.n_uncollated_copies)
		{
		    if (!mch_print_blank_page())
			goto print_fail;
		}
	    }
	    if (settings.duplex && prtpos.file_line <= eap->line2)
		++page_count;

	    /* Remember the position where the next page starts. */
	    page_prtpos = prtpos;
	}

	vim_snprintf((char *)IObuff, IOSIZE, _("Printed: %s"),
							    settings.jobname);
	prt_message(IObuff);
    }

print_fail:
    if (got_int || settings.user_abort)
    {
	sprintf((char *)IObuff, "%s", _("Printing aborted"));
	prt_message(IObuff);
    }
    mch_print_end(&settings);

print_fail_no_begin:
    mch_print_cleanup();
}

/*
 * Print one page line.
 * Return the next column to print, or zero if the line is finished.
 */
    static colnr_T
hardcopy_line(psettings, page_line, ppos)
    prt_settings_T	*psettings;
    int			page_line;
    prt_pos_T		*ppos;
{
    colnr_T	col;
    char_u	*line;
    int		need_break = FALSE;
    int		outputlen;
    int		tab_spaces;
    long_u	print_pos;
#ifdef FEAT_SYN_HL
    prt_text_attr_T attr;
    int		id;
#endif

    if (ppos->column == 0 || ppos->ff)
    {
	print_pos = 0;
	tab_spaces = 0;
	if (!ppos->ff && prt_use_number())
	    prt_line_number(psettings, page_line, ppos->file_line);
	ppos->ff = FALSE;
    }
    else
    {
	/* left over from wrap halfway a tab */
	print_pos = ppos->print_pos;
	tab_spaces = ppos->lead_spaces;
    }

    mch_print_start_line(0, page_line);
    line = ml_get(ppos->file_line);

    /*
     * Loop over the columns until the end of the file line or right margin.
     */
    for (col = ppos->column; line[col] != NUL && !need_break; col += outputlen)
    {
	outputlen = 1;
#ifdef FEAT_MBYTE
	if (has_mbyte && (outputlen = (*mb_ptr2len_check)(line + col)) < 1)
	    outputlen = 1;
#endif
#ifdef FEAT_SYN_HL
	/*
	 * syntax highlighting stuff.
	 */
	if (psettings->do_syntax)
	{
	    id = syn_get_id(ppos->file_line, col, 1, NULL);
	    if (id > 0)
		id = syn_get_final_id(id);
	    else
		id = 0;
	    /* Get the line again, a multi-line regexp may invalidate it. */
	    line = ml_get(ppos->file_line);

	    if (id != current_syn_id)
	    {
		current_syn_id = id;
		prt_get_attr(id, &attr, psettings->modec);
		prt_set_font(attr.bold, attr.italic, attr.underline);
		prt_set_fg(attr.fg_color);
		prt_set_bg(attr.bg_color);
	    }
	}
#endif /* FEAT_SYN_HL */

	/*
	 * Appropriately expand any tabs to spaces.
	 */
	if (line[col] == TAB || tab_spaces != 0)
	{
	    if (tab_spaces == 0)
		tab_spaces = curbuf->b_p_ts - (print_pos % curbuf->b_p_ts);

	    while (tab_spaces > 0)
	    {
		need_break = mch_print_text_out((char_u *)" ", 1);
		print_pos++;
		tab_spaces--;
		if (need_break)
		    break;
	    }
	    /* Keep the TAB if we didn't finish it. */
	    if (need_break && tab_spaces > 0)
		break;
	}
	else if (line[col] == FF
		&& printer_opts[OPT_PRINT_FORMFEED].present
		&& TOLOWER_ASC(printer_opts[OPT_PRINT_FORMFEED].string[0])
								       == 'y')
	{
	    ppos->ff = TRUE;
	    need_break = 1;
	}
	else
	{
	    need_break = mch_print_text_out(line + col, outputlen);
#ifdef FEAT_MBYTE
	    if (has_mbyte)
		print_pos += (*mb_ptr2cells)(line + col);
	    else
#endif
		print_pos++;
	}
    }

    ppos->lead_spaces = tab_spaces;
    ppos->print_pos = print_pos;

    /*
     * Start next line of file if we clip lines, or have reached end of the
     * line, unless we are doing a formfeed.
     */
    if (!ppos->ff
	    && (line[col] == NUL
		|| (printer_opts[OPT_PRINT_WRAP].present
		    && TOLOWER_ASC(printer_opts[OPT_PRINT_WRAP].string[0])
								     == 'n')))
	return 0;
    return col;
}

# if defined(FEAT_POSTSCRIPT) || defined(PROTO)

/*
 * PS printer stuff.
 *
 * Sources of information to help maintain the PS printing code:
 *
 * 1. PostScript Language Reference, 3rd Edition,
 *      Addison-Wesley, 1999, ISBN 0-201-37922-8
 * 2. PostScript Language Program Design,
 *      Addison-Wesley, 1988, ISBN 0-201-14396-8
 * 3. PostScript Tutorial and Cookbook,
 *      Addison Wesley, 1985, ISBN 0-201-10179-3
 * 4. PostScript Language Document Structuring Conventions Specification,
 *    version 3.0,
 *      Adobe Technote 5001, 25th September 1992
 * 5. PostScript Printer Description File Format Specification, Version 4.3,
 *      Adobe technote 5003, 9th February 1996
 * 6. Adobe Font Metrics File Format Specification, Version 4.1,
 *      Adobe Technote 5007, 7th October 1998
 * 7. Adobe CMap and CIDFont Files Specification, Version 1.0,
 *      Adobe Technote 5014, 8th October 1996
 * 8. Adobe CJKV Character Collections and CMaps for CID-Keyed Fonts,
 *      Adoboe Technote 5094, 8th September, 2001
 * 9. CJKV Information Processing, 2nd Edition,
 *      O'Reilly, 2002, ISBN 1-56592-224-7
 *
 * Some of these documents can be found in PDF form on Adobe's web site -
 * http://www.adobe.com
 */

#define NUM_ELEMENTS(arr)   (sizeof(arr)/sizeof((arr)[0]))

#define PRT_PS_DEFAULT_DPI	    (72)    /* Default user space resolution */
#define PRT_PS_DEFAULT_FONTSIZE     (10)
#define PRT_PS_DEFAULT_BUFFER_SIZE  (80)

struct prt_mediasize_S
{
    char	*name;
    float	width;		/* width and height in points for portrait */
    float	height;
};

#define PRT_MEDIASIZE_LEN  (sizeof(prt_mediasize) / sizeof(struct prt_mediasize_S))

static struct prt_mediasize_S prt_mediasize[] =
{
    {"A4",		595.0,  842.0},
    {"letter",		612.0,  792.0},
    {"10x14",		720.0, 1008.0},
    {"A3",		842.0, 1191.0},
    {"A5",		420.0,  595.0},
    {"B4",		729.0, 1032.0},
    {"B5",		516.0,  729.0},
    {"executive",	522.0,  756.0},
    {"folio",		595.0,  935.0},
    {"ledger",	       1224.0,  792.0},   /* Yes, it is wider than taller! */
    {"legal",		612.0, 1008.0},
    {"quarto",		610.0,  780.0},
    {"statement",	396.0,  612.0},
    {"tabloid",		792.0, 1224.0}
};

/* PS font names, must be in Roman, Bold, Italic, Bold-Italic order */
struct prt_ps_font_S
{
    int		wx;
    int		uline_offset;
    int		uline_width;
    int		bbox_min_y;
    int		bbox_max_y;
    char	*(ps_fontname[4]);
};

#define PRT_PS_FONT_ROMAN	(0)
#define PRT_PS_FONT_BOLD	(1)
#define PRT_PS_FONT_OBLIQUE	(2)
#define PRT_PS_FONT_BOLDOBLIQUE (3)

/* Standard font metrics for Courier family */
static struct prt_ps_font_S prt_ps_courier_font =
{
    600,
    -100, 50,
    -250, 805,
    {"Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique"}
};

#ifdef FEAT_MBYTE
/* Generic font metrics for multi-byte fonts */
static struct prt_ps_font_S prt_ps_mb_font =
{
    1000,
    -100, 50,
    -250, 805,
    {NULL, NULL, NULL, NULL}
};
#endif

/* Pointer to current font set being used */
static struct prt_ps_font_S* prt_ps_font;

/* Structures to map user named encoding and mapping to PS equivalents for
 * building CID font name */
struct prt_ps_encoding_S
{
    char	*encoding;
    char	*cmap_encoding;
    int		needs_charset;
};

struct prt_ps_charset_S
{
    char	*charset;
    char	*cmap_charset;
    int		has_charset;
};

#ifdef FEAT_MBYTE

#define CS_JIS_C_1978   (0x01)
#define CS_JIS_X_1983   (0x02)
#define CS_JIS_X_1990   (0x04)
#define CS_NEC          (0x08)
#define CS_MSWINDOWS    (0x10)
#define CS_CP932        (0x20)
#define CS_KANJITALK6   (0x40)
#define CS_KANJITALK7   (0x80)

/* Japanese encodings and charsets */
static struct prt_ps_encoding_S j_encodings[] =
{
    {"iso-2022-jp", NULL,       (CS_JIS_C_1978|CS_JIS_X_1983|CS_JIS_X_1990|
                                                                    CS_NEC)},
    {"euc-jp",      "EUC",      (CS_JIS_C_1978|CS_JIS_X_1983|CS_JIS_X_1990)},
    {"sjis",        "RKSJ",     (CS_JIS_C_1978|CS_JIS_X_1983|CS_MSWINDOWS|
                                                CS_KANJITALK6|CS_KANJITALK7)},
    {"cp932",       "RKSJ",     CS_JIS_X_1983},
    {"ucs-2",       "UCS2",     CS_JIS_X_1990},
    {"utf-8",       "UTF8" ,    CS_JIS_X_1990}
};
static struct prt_ps_charset_S j_charsets[] =
{
    {"JIS_C_1978",  "78",       CS_JIS_C_1978},
    {"JIS_X_1983",  NULL,       CS_JIS_X_1983},
    {"JIS_X_1990",  "Hojo",     CS_JIS_X_1990},
    {"NEC",         "Ext",      CS_NEC},
    {"MSWINDOWS",   "90ms",     CS_MSWINDOWS},
    {"CP932",       "90ms",     CS_JIS_X_1983},
    {"KANJITALK6",  "83pv",     CS_KANJITALK6},
    {"KANJITALK7",  "90pv",     CS_KANJITALK7}
};

#define CS_GB_2312_80       (0x01)
#define CS_GBT_12345_90     (0x02)
#define CS_GBK2K            (0x04)
#define CS_SC_MAC           (0x08)
#define CS_GBT_90_MAC       (0x10)
#define CS_GBK              (0x20)
#define CS_SC_ISO10646      (0x40)

/* Simplified Chinese encodings and charsets */
static struct prt_ps_encoding_S sc_encodings[] =
{
    {"iso-2022",    NULL,       (CS_GB_2312_80|CS_GBT_12345_90)},
    {"gb18030",     NULL,       CS_GBK2K},
    {"euc-cn",      "EUC",      (CS_GB_2312_80|CS_GBT_12345_90|CS_SC_MAC|
                                                                CS_GBT_90_MAC)},
    {"gbk",         "EUC",      CS_GBK},
    {"ucs-2",       "UCS2",     CS_SC_ISO10646},
    {"utf-8",       "UTF8",     CS_SC_ISO10646}
};
static struct prt_ps_charset_S sc_charsets[] =
{
    {"GB_2312-80",  "GB",       CS_GB_2312_80},
    {"GBT_12345-90","GBT",      CS_GBT_12345_90},
    {"MAC",         "GBpc",     CS_SC_MAC},
    {"GBT-90_MAC",  "GBTpc",    CS_GBT_90_MAC},
    {"GBK",         "GBK",      CS_GBK},
    {"GB18030",     "GBK2K",    CS_GBK2K},
    {"ISO10646",    "UniGB",    CS_SC_ISO10646}
};

#define CS_CNS_PLANE_1      (0x01)
#define CS_CNS_PLANE_2      (0x02)
#define CS_CNS_PLANE_1_2    (0x04)
#define CS_B5               (0x08)
#define CS_ETEN             (0x10)
#define CS_HK_GCCS          (0x20)
#define CS_HK_SCS           (0x40)
#define CS_HK_SCS_ETEN      (0x80)
#define CS_MTHKL            (0x100)
#define CS_MTHKS            (0x200)
#define CS_DLHKL            (0x400)
#define CS_DLHKS            (0x800)
#define CS_TC_ISO10646      (0x1000)

/* Traditional Chinese encodings and charsets */
static struct prt_ps_encoding_S tc_encodings[] =
{
    {"iso-2022",    NULL,       (CS_CNS_PLANE_1|CS_CNS_PLANE_2)},
    {"euc-tw",      "EUC",      CS_CNS_PLANE_1_2},
    {"big5",        "B5",       (CS_B5|CS_ETEN|CS_HK_GCCS|CS_HK_SCS|
                                    CS_HK_SCS_ETEN|CS_MTHKL|CS_MTHKS|CS_DLHKL|
                                                                    CS_DLHKS)},
    {"cp950",       "B5",       CS_B5},
    {"ucs-2",       "UCS2",     CS_TC_ISO10646},
    {"utf-8",       "UTF8",     CS_TC_ISO10646},
    {"utf-16",      "UTF16",    CS_TC_ISO10646},
    {"utf-32",      "UTF32",    CS_TC_ISO10646}
};
static struct prt_ps_charset_S tc_charsets[] =
{
    {"CNS_1992_1",  "CNS1",     CS_CNS_PLANE_1},
    {"CNS_1992_2",  "CNS2",     CS_CNS_PLANE_2},
    {"CNS_1993",    "CNS",      CS_CNS_PLANE_1_2},
    {"BIG5",        NULL,       CS_B5},
    {"CP950",       NULL,       CS_B5},
    {"ETEN",        "ETen",     CS_ETEN},
    {"HK_GCCS",     "HKgccs",   CS_HK_GCCS},
    {"SCS",         "HKscs",    CS_HK_SCS},
    {"SCS_ETEN",    "ETHK",     CS_HK_SCS_ETEN},
    {"MTHKL",       "HKm471",   CS_MTHKL},
    {"MTHKS",       "HKm314",   CS_MTHKS},
    {"DLHKL",       "HKdla",    CS_DLHKL},
    {"DLHKS",       "HKdlb",    CS_DLHKS},
    {"ISO10646",    "UniCNS",   CS_TC_ISO10646}
};

#define CS_KR_X_1992        (0x01)
#define CS_KR_MAC           (0x02)
#define CS_KR_X_1992_MS     (0x04)
#define CS_KR_ISO10646      (0x08)

/* Korean encodings and charsets */
static struct prt_ps_encoding_S k_encodings[] =
{
    {"iso-2022-kr", NULL,       CS_KR_X_1992},
    {"euc-kr",      "EUC",      (CS_KR_X_1992|CS_KR_MAC)},
    {"johab",       "Johab",    CS_KR_X_1992},
    {"cp1361",      "Johab",    CS_KR_X_1992},
    {"uhc",         "UHC",      CS_KR_X_1992_MS},
    {"cp949",       "UHC",      CS_KR_X_1992_MS},
    {"ucs-2",       "UCS2",     CS_KR_ISO10646},
    {"utf-8",       "UTF8",     CS_KR_ISO10646}
};
static struct prt_ps_charset_S k_charsets[] =
{
    {"KS_X_1992",   "KSC",      CS_KR_X_1992},
    {"CP1361",      "KSC",      CS_KR_X_1992},
    {"MAC",         "KSCpc",    CS_KR_MAC},
    {"MSWINDOWS",   "KSCms",    CS_KR_X_1992_MS},
    {"CP949",       "KSCms",    CS_KR_X_1992_MS},
    {"WANSUNG",     "KSCms",    CS_KR_X_1992_MS},
    {"ISO10646",    "UniKS",    CS_KR_ISO10646}
};

/* Collections of encodings and charsets for multi-byte printing */
struct prt_ps_mbfont_S
{
    int                         num_encodings;
    struct prt_ps_encoding_S    *encodings;
    int                         num_charsets;
    struct prt_ps_charset_S     *charsets;
    char                        *ascii_enc;
    char                        *defcs;
};

static struct prt_ps_mbfont_S prt_ps_mbfonts[] =
{
    {
        NUM_ELEMENTS(j_encodings),
        j_encodings,
        NUM_ELEMENTS(j_charsets),
        j_charsets,
        "jis_roman",
        "JIS_X_1983"
    },
    {
        NUM_ELEMENTS(sc_encodings),
        sc_encodings,
        NUM_ELEMENTS(sc_charsets),
        sc_charsets,
        "gb_roman",
        "GB_2312-80"
    },
    {
        NUM_ELEMENTS(tc_encodings),
        tc_encodings,
        NUM_ELEMENTS(tc_charsets),
        tc_charsets,
        "cns_roman",
        "BIG5"
    },
    {
        NUM_ELEMENTS(k_encodings),
        k_encodings,
        NUM_ELEMENTS(k_charsets),
        k_charsets,
        "ks_roman",
        "KS_X_1992"
    }
};
#endif /* FEAT_MBYTE */

struct prt_ps_resource_S
{
    char_u  name[64];
    char_u  filename[MAXPATHL + 1];
    int     type;
    char_u  title[256];
    char_u  version[256];
};

/* Types of PS resource file currently used */
#define PRT_RESOURCE_TYPE_PROCSET   (0)
#define PRT_RESOURCE_TYPE_ENCODING  (1)
#define PRT_RESOURCE_TYPE_CMAP      (2)

/* The PS prolog file version number has to match - if the prolog file is
 * updated, increment the number in the file and here.  Version checking was
 * added as of VIM 6.2.
 * The CID prolog file version number behaves as per PS prolog.
 * Table of VIM and prolog versions:
 *
 * VIM      Prolog  CIDProlog
 * 6.2      1.3
 * 7.0      1.4	    1.0
 */
#define PRT_PROLOG_VERSION  ((char_u *)"1.4")
#define PRT_CID_PROLOG_VERSION  ((char_u *)"1.0")

/* String versions of PS resource types - indexed by constants above so don't
 * re-order!
 */
static char *prt_resource_types[] =
{
    "procset",
    "encoding",
    "cmap"
};

/* Strings to look for in a PS resource file */
#define PRT_RESOURCE_HEADER	    "%!PS-Adobe-"
#define PRT_RESOURCE_RESOURCE	    "Resource-"
#define PRT_RESOURCE_PROCSET	    "ProcSet"
#define PRT_RESOURCE_ENCODING	    "Encoding"
#define PRT_RESOURCE_CMAP           "CMap"


/* Data for table based DSC comment recognition, easy to extend if VIM needs to
 * read more comments. */
#define PRT_DSC_MISC_TYPE           (-1)
#define PRT_DSC_TITLE_TYPE          (1)
#define PRT_DSC_VERSION_TYPE        (2)
#define PRT_DSC_ENDCOMMENTS_TYPE    (3)

#define PRT_DSC_TITLE	            "%%Title:"
#define PRT_DSC_VERSION	            "%%Version:"
#define PRT_DSC_ENDCOMMENTS         "%%EndComments:"

struct prt_dsc_comment_S
{
    char	*string;
    int		len;
    int		type;
};

struct prt_dsc_line_S
{
    int		type;
    char_u	*string;
    int		len;
};


#define SIZEOF_CSTR(s)      (sizeof(s) - 1)
struct prt_dsc_comment_S prt_dsc_table[] =
{
    {PRT_DSC_TITLE,       SIZEOF_CSTR(PRT_DSC_TITLE),     PRT_DSC_TITLE_TYPE},
    {PRT_DSC_VERSION,     SIZEOF_CSTR(PRT_DSC_VERSION),
							PRT_DSC_VERSION_TYPE},
    {PRT_DSC_ENDCOMMENTS, SIZEOF_CSTR(PRT_DSC_ENDCOMMENTS),
						     PRT_DSC_ENDCOMMENTS_TYPE}
};

static void prt_write_file_raw_len __ARGS((char_u *buffer, int bytes));
static void prt_write_file __ARGS((char_u *buffer));
static void prt_write_file_len __ARGS((char_u *buffer, int bytes));
static void prt_write_string __ARGS((char *s));
static void prt_write_int __ARGS((int i));
static void prt_write_boolean __ARGS((int b));
static void prt_def_font __ARGS((char *new_name, char *encoding, int height, char *font));
static void prt_real_bits __ARGS((double real, int precision, int *pinteger, int *pfraction));
static void prt_write_real __ARGS((double val, int prec));
static void prt_def_var __ARGS((char *name, double value, int prec));
static void prt_flush_buffer __ARGS((void));
static void prt_resource_name __ARGS((char_u *filename, void *cookie));
static int prt_find_resource __ARGS((char *name, struct prt_ps_resource_S *resource));
static int prt_open_resource __ARGS((struct prt_ps_resource_S *resource));
static int prt_check_resource __ARGS((struct prt_ps_resource_S *resource, char_u *version));
static void prt_dsc_start __ARGS((void));
static void prt_dsc_noarg __ARGS((char *comment));
static void prt_dsc_textline __ARGS((char *comment, char *text));
static void prt_dsc_text __ARGS((char *comment, char *text));
static void prt_dsc_ints __ARGS((char *comment, int count, int *ints));
static void prt_dsc_requirements __ARGS((int duplex, int tumble, int collate, int color, int num_copies));
static void prt_dsc_docmedia __ARGS((char *paper_name, double width, double height, double weight, char *colour, char *type));
static void prt_dsc_resources __ARGS((char *comment, char *type, char *strings));
static void prt_dsc_font_resource __ARGS((char *resource, struct prt_ps_font_S *ps_font));
static float to_device_units __ARGS((int idx, double physsize, int def_number));
static void prt_page_margins __ARGS((double width, double height, double *left, double *right, double *top, double *bottom));
static void prt_font_metrics __ARGS((int font_scale));
static int prt_get_cpl __ARGS((void));
static int prt_get_lpp __ARGS((void));
static int prt_add_resource __ARGS((struct prt_ps_resource_S *resource));
static int prt_resfile_next_line __ARGS((void));
static int prt_resfile_strncmp __ARGS((int offset, char *string, int len));
static int prt_resfile_skip_nonws __ARGS((int offset));
static int prt_resfile_skip_ws __ARGS((int offset));
static int prt_next_dsc __ARGS((struct prt_dsc_line_S *p_dsc_line));
#ifdef FEAT_MBYTE
static int prt_build_cid_fontname __ARGS((int font, char_u *name, int name_len));
static void prt_def_cidfont __ARGS((char *new_name, int height, char *cidfont));
static int prt_match_encoding __ARGS((char *p_encoding, struct prt_ps_mbfont_S *p_cmap, struct prt_ps_encoding_S **pp_mbenc));
static int prt_match_charset __ARGS((char *p_charset, struct prt_ps_mbfont_S *p_cmap, struct prt_ps_charset_S **pp_mbchar));
#endif

/*
 * Variables for the output PostScript file.
 */
static FILE *prt_ps_fd;
static int prt_file_error;
static char_u *prt_ps_file_name = NULL;

/*
 * Various offsets and dimensions in default PostScript user space (points).
 * Used for text positioning calculations
 */
static float prt_page_width;
static float prt_page_height;
static float prt_left_margin;
static float prt_right_margin;
static float prt_top_margin;
static float prt_bottom_margin;
static float prt_line_height;
static float prt_first_line_height;
static float prt_char_width;
static float prt_number_width;
static float prt_bgcol_offset;
static float prt_pos_x_moveto = 0.0;
static float prt_pos_y_moveto = 0.0;

/*
 * Various control variables used to decide when and how to change the
 * PostScript graphics state.
 */
static int prt_need_moveto;
static int prt_do_moveto;
static int prt_need_font;
static int prt_font;
static int prt_need_underline;
static int prt_underline;
static int prt_do_underline;
static int prt_need_fgcol;
static int prt_fgcol;
static int prt_need_bgcol;
static int prt_do_bgcol;
static int prt_bgcol;
static int prt_new_bgcol;
static int prt_attribute_change;
static float prt_text_run;
static int prt_page_num;
static int prt_bufsiz;

/*
 * Variables controlling physical printing.
 */
static int prt_media;
static int prt_portrait;
static int prt_num_copies;
static int prt_duplex;
static int prt_tumble;
static int prt_collate;

/*
 * Buffers used when generating PostScript output
 */
static char_u prt_line_buffer[257];
static garray_T prt_ps_buffer;

# ifdef FEAT_MBYTE
static int prt_do_conv;
static vimconv_T prt_conv;

static int prt_out_mbyte;
static int prt_custom_cmap;
static char prt_cmap[80];
static int prt_use_courier;
static int prt_in_ascii;
static int prt_half_width;
static char *prt_ascii_encoding;
static char_u prt_hexchar[] = "0123456789abcdef";
# endif

    static void
prt_write_file_raw_len(buffer, bytes)
    char_u	*buffer;
    int		bytes;
{
    if (!prt_file_error
	    && fwrite(buffer, sizeof(char_u), bytes, prt_ps_fd)
							     != (size_t)bytes)
    {
	EMSG(_("E455: Error writing to PostScript output file"));
	prt_file_error = TRUE;
    }
}

    static void
prt_write_file(buffer)
    char_u	*buffer;
{
    prt_write_file_len(buffer, STRLEN(buffer));
}

    static void
prt_write_file_len(buffer, bytes)
    char_u	*buffer;
    int		bytes;
{
#ifdef EBCDIC
    ebcdic2ascii(buffer, bytes);
#endif
    prt_write_file_raw_len(buffer, bytes);
}

/*
 * Write a string.
 */
    static void
prt_write_string(s)
    char	*s;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer), "%s", s);
    prt_write_file(prt_line_buffer);
}

/*
 * Write an int and a space.
 */
    static void
prt_write_int(i)
    int		i;
{
    sprintf((char *)prt_line_buffer, "%d ", i);
    prt_write_file(prt_line_buffer);
}

/*
 * Write a boolean and a space.
 */
    static void
prt_write_boolean(b)
    int		b;
{
    sprintf((char *)prt_line_buffer, "%s ", (b ? "T" : "F"));
    prt_write_file(prt_line_buffer);
}

/*
 * Write PostScript to re-encode and define the font.
 */
    static void
prt_def_font(new_name, encoding, height, font)
    char	*new_name;
    char	*encoding;
    int		height;
    char	*font;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
			  "/_%s /VIM-%s /%s ref\n", new_name, encoding, font);
    prt_write_file(prt_line_buffer);
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
        sprintf((char *)prt_line_buffer, "/%s %d %f /_%s sffs\n",
		       new_name, height, 500./prt_ps_courier_font.wx, new_name);
    else
#endif
	vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
			     "/%s %d /_%s ffs\n", new_name, height, new_name);
    prt_write_file(prt_line_buffer);
}

#ifdef FEAT_MBYTE
/*
 * Write a line to define the CID font.
 */
    static void
prt_def_cidfont(new_name, height, cidfont)
    char	*new_name;
    int		height;
    char	*cidfont;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
	      "/_%s /%s[/%s] vim_composefont\n", new_name, prt_cmap, cidfont);
    prt_write_file(prt_line_buffer);
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
			     "/%s %d /_%s ffs\n", new_name, height, new_name);
    prt_write_file(prt_line_buffer);
}

/*
 * Write a line to define a duplicate of a CID font
 */
    static void
prt_dup_cidfont(original_name, new_name)
    char	*original_name;
    char	*new_name;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
				       "/%s %s d\n", new_name, original_name);
    prt_write_file(prt_line_buffer);
}
#endif

/*
 * Convert a real value into an integer and fractional part as integers, with
 * the fractional part being in the range [0,10^precision).  The fractional part
 * is also rounded based on the precision + 1'th fractional digit.
 */
    static void
prt_real_bits(real, precision, pinteger, pfraction)
    double      real;
    int		precision;
    int		*pinteger;
    int		*pfraction;
{
    int     i;
    int     integer;
    float   fraction;

    integer = (int)real;
    fraction = (float)(real - integer);
    if (real < (double)integer)
	fraction = -fraction;
    for (i = 0; i < precision; i++)
	fraction *= 10.0;

    *pinteger = integer;
    *pfraction = (int)(fraction + 0.5);
}

/*
 * Write a real and a space.  Save bytes if real value has no fractional part!
 * We use prt_real_bits() as %f in sprintf uses the locale setting to decide
 * what decimal point character to use, but PS always requires a '.'.
 */
    static void
prt_write_real(val, prec)
    double	val;
    int		prec;
{
    int     integer;
    int     fraction;

    prt_real_bits(val, prec, &integer, &fraction);
    /* Emit integer part */
    sprintf((char *)prt_line_buffer, "%d", integer);
    prt_write_file(prt_line_buffer);
    /* Only emit fraction if necessary */
    if (fraction != 0)
    {
	/* Remove any trailing zeros */
	while ((fraction % 10) == 0)
	{
	    prec--;
	    fraction /= 10;
	}
	/* Emit fraction left padded with zeros */
	sprintf((char *)prt_line_buffer, ".%0*d", prec, fraction);
	prt_write_file(prt_line_buffer);
    }
    sprintf((char *)prt_line_buffer, " ");
    prt_write_file(prt_line_buffer);
}

/*
 * Write a line to define a numeric variable.
 */
    static void
prt_def_var(name, value, prec)
    char	*name;
    double	value;
    int		prec;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
								"/%s ", name);
    prt_write_file(prt_line_buffer);
    prt_write_real(value, prec);
    sprintf((char *)prt_line_buffer, "d\n");
    prt_write_file(prt_line_buffer);
}

/* Convert size from font space to user space at current font scale */
#define PRT_PS_FONT_TO_USER(scale, size)    ((size) * ((scale)/1000.0))

    static void
prt_flush_buffer()
{
    if (prt_ps_buffer.ga_len > 0)
    {
	/* Any background color must be drawn first */
	if (prt_do_bgcol && (prt_new_bgcol != PRCOLOR_WHITE))
	{
	    int     r, g, b;

	    if (prt_do_moveto)
	    {
		prt_write_real(prt_pos_x_moveto, 2);
		prt_write_real(prt_pos_y_moveto, 2);
		prt_write_string("m\n");
		prt_do_moveto = FALSE;
	    }

	    /* Size of rect of background color on which text is printed */
	    prt_write_real(prt_text_run, 2);
	    prt_write_real(prt_line_height, 2);

	    /* Lastly add the color of the background */
	    r = ((unsigned)prt_new_bgcol & 0xff0000) >> 16;
	    g = ((unsigned)prt_new_bgcol & 0xff00) >> 8;
	    b = prt_new_bgcol & 0xff;
	    prt_write_real(r / 255.0, 3);
	    prt_write_real(g / 255.0, 3);
	    prt_write_real(b / 255.0, 3);
	    prt_write_string("bg\n");
	}
	/* Draw underlines before the text as it makes it slightly easier to
	 * find the starting point.
	 */
	if (prt_do_underline)
	{
	    if (prt_do_moveto)
	    {
		prt_write_real(prt_pos_x_moveto, 2);
		prt_write_real(prt_pos_y_moveto, 2);
		prt_write_string("m\n");
		prt_do_moveto = FALSE;
	    }

            /* Underline length of text run */
	    prt_write_real(prt_text_run, 2);
	    prt_write_string("ul\n");
	}
	/* Draw the text
	 * Note: we write text out raw - EBCDIC conversion is handled in the
	 * PostScript world via the font encoding vector. */
#ifdef FEAT_MBYTE
        if (prt_out_mbyte)
            prt_write_string("<");
        else
#endif
            prt_write_string("(");
	prt_write_file_raw_len(prt_ps_buffer.ga_data, prt_ps_buffer.ga_len);
#ifdef FEAT_MBYTE
        if (prt_out_mbyte)
            prt_write_string(">");
        else
#endif
            prt_write_string(")");
	/* Add a moveto if need be and use the appropriate show procedure */
	if (prt_do_moveto)
	{
	    prt_write_real(prt_pos_x_moveto, 2);
	    prt_write_real(prt_pos_y_moveto, 2);
	    /* moveto and a show */
	    prt_write_string("ms\n");
	    prt_do_moveto = FALSE;
	}
	else /* Simple show */
	    prt_write_string("s\n");

	ga_clear(&prt_ps_buffer);
	ga_init2(&prt_ps_buffer, (int)sizeof(char), prt_bufsiz);
    }
}


    static void
prt_resource_name(filename, cookie)
    char_u  *filename;
    void    *cookie;
{
    char_u *resource_filename = cookie;

    if (STRLEN(filename) >= MAXPATHL)
	*resource_filename = NUL;
    else
	STRCPY(resource_filename, filename);
}

    static int
prt_find_resource(name, resource)
    char	*name;
    struct prt_ps_resource_S *resource;
{
    char_u	buffer[MAXPATHL + 1];

    STRCPY(resource->name, name);
    /* Look for named resource file in runtimepath */
    STRCPY(buffer, "print");
    add_pathsep(buffer);
    STRCAT(buffer, name);
    STRCAT(buffer, ".ps");
    resource->filename[0] = NUL;
    return (do_in_runtimepath(buffer, FALSE, prt_resource_name,
							   resource->filename)
	    && resource->filename[0] != NUL);
}

/* PS CR and LF characters have platform independent values */
#define PSLF  (0x0a)
#define PSCR  (0x0d)

/* Static buffer to read initial comments in a resource file, some can have a
 * couple of KB of comments! */
#define PRT_FILE_BUFFER_LEN (2048)
struct prt_resfile_buffer_S
{
    char_u  buffer[PRT_FILE_BUFFER_LEN];
    int     len;
    int     line_start;
    int     line_end;
};

static struct prt_resfile_buffer_S prt_resfile;

    static int
prt_resfile_next_line()
{
    int     index;

    /* Move to start of next line and then find end of line */
    index = prt_resfile.line_end + 1;
    while (index < prt_resfile.len)
    {
        if (prt_resfile.buffer[index] != PSLF && prt_resfile.buffer[index]
                                                                        != PSCR)
            break;
        index++;
    }
    prt_resfile.line_start = index;

    while (index < prt_resfile.len)
    {
        if (prt_resfile.buffer[index] == PSLF || prt_resfile.buffer[index]
                                                                        == PSCR)
            break;
        index++;
    }
    prt_resfile.line_end = index;

    return (index < prt_resfile.len);
}

    static int
prt_resfile_strncmp(offset, string, len)
    int     offset;
    char    *string;
    int     len;
{
    /* Force not equal if string is longer than remainder of line */
    if (len > (prt_resfile.line_end - (prt_resfile.line_start + offset)))
        return 1;

    return STRNCMP(&prt_resfile.buffer[prt_resfile.line_start + offset],
                                                                string, len);
}

    static int
prt_resfile_skip_nonws(offset)
    int     offset;
{
    int     index;

    index = prt_resfile.line_start + offset;
    while (index < prt_resfile.line_end)
    {
        if (isspace(prt_resfile.buffer[index]))
            return index - prt_resfile.line_start;
        index++;
    }
    return -1;
}

    static int
prt_resfile_skip_ws(offset)
    int     offset;
{
    int     index;

    index = prt_resfile.line_start + offset;
    while (index < prt_resfile.line_end)
    {
        if (!isspace(prt_resfile.buffer[index]))
            return index - prt_resfile.line_start;
        index++;
    }
    return -1;
}

/* prt_next_dsc() - returns detail on next DSC comment line found.  Returns true
 * if a DSC comment is found, else false */
    static int
prt_next_dsc(p_dsc_line)
    struct prt_dsc_line_S *p_dsc_line;
{
    int     comment;
    int     offset;

    /* Move to start of next line */
    if (!prt_resfile_next_line())
        return FALSE;

    /* DSC comments always start %% */
    if (prt_resfile_strncmp(0, "%%", 2) != 0)
        return FALSE;

    /* Find type of DSC comment */
    for (comment = 0; comment < NUM_ELEMENTS(prt_dsc_table); comment++)
        if (prt_resfile_strncmp(0, prt_dsc_table[comment].string,
                                            prt_dsc_table[comment].len) == 0)
            break;

    if (comment != NUM_ELEMENTS(prt_dsc_table))
    {
        /* Return type of comment */
        p_dsc_line->type = prt_dsc_table[comment].type;
        offset = prt_dsc_table[comment].len;
    }
    else
    {
        /* Unrecognised DSC comment, skip to ws after comment leader */
        p_dsc_line->type = PRT_DSC_MISC_TYPE;
        offset = prt_resfile_skip_nonws(0);
        if (offset == -1)
            return FALSE;
    }

    /* Skip ws to comment value */
    offset = prt_resfile_skip_ws(offset);
    if (offset == -1)
        return FALSE;

    p_dsc_line->string = &prt_resfile.buffer[prt_resfile.line_start + offset];
    p_dsc_line->len = prt_resfile.line_end - (prt_resfile.line_start + offset);

    return TRUE;
}

/* Improved hand crafted parser to get the type, title, and version number of a
 * PS resource file so the file details can be added to the DSC header comments.
 */
    static int
prt_open_resource(resource)
    struct prt_ps_resource_S *resource;
{
    int         offset;
    int         seen_all;
    int         seen_title;
    int         seen_version;
    FILE	*fd_resource;
    struct prt_dsc_line_S dsc_line;

    fd_resource = mch_fopen((char *)resource->filename, READBIN);
    if (fd_resource == NULL)
    {
	EMSG2(_("E624: Can't open file \"%s\""), resource->filename);
	return FALSE;
    }
    vim_memset(prt_resfile.buffer, NUL, PRT_FILE_BUFFER_LEN);

    /* Parse first line to ensure valid resource file */
    prt_resfile.len = fread((char *)prt_resfile.buffer, sizeof(char_u),
                                            PRT_FILE_BUFFER_LEN, fd_resource);
    if (ferror(fd_resource))
    {
	EMSG2(_("E457: Can't read PostScript resource file \"%s\""),
		resource->filename);
	fclose(fd_resource);
	return FALSE;
    }

    prt_resfile.line_end = -1;
    prt_resfile.line_start = 0;
    if (!prt_resfile_next_line())
        return FALSE;

    offset = 0;

    if (prt_resfile_strncmp(offset, PRT_RESOURCE_HEADER,
                                            STRLEN(PRT_RESOURCE_HEADER)) != 0)
    {
	EMSG2(_("E618: file \"%s\" is not a PostScript resource file"),
		resource->filename);
	fclose(fd_resource);
	return FALSE;
    }

    /* Skip over any version numbers and following ws */
    offset += STRLEN(PRT_RESOURCE_HEADER);
    offset = prt_resfile_skip_nonws(offset);
    if (offset == -1)
        return FALSE;
    offset = prt_resfile_skip_ws(offset);
    if (offset == -1)
        return FALSE;

    if (prt_resfile_strncmp(offset, PRT_RESOURCE_RESOURCE,
                                            STRLEN(PRT_RESOURCE_RESOURCE)) != 0)
    {
	EMSG2(_("E619: file \"%s\" is not a supported PostScript resource file"),
		resource->filename);
	fclose(fd_resource);
	return FALSE;
    }
    offset += STRLEN(PRT_RESOURCE_RESOURCE);

    /* Decide type of resource in the file */
    if (prt_resfile_strncmp(offset, PRT_RESOURCE_PROCSET,
                                            STRLEN(PRT_RESOURCE_PROCSET)) == 0)
	resource->type = PRT_RESOURCE_TYPE_PROCSET;
    else if (prt_resfile_strncmp(offset, PRT_RESOURCE_ENCODING,
                                            STRLEN(PRT_RESOURCE_ENCODING)) == 0)
	resource->type = PRT_RESOURCE_TYPE_ENCODING;
    else if (prt_resfile_strncmp(offset, PRT_RESOURCE_CMAP,
                                            STRLEN(PRT_RESOURCE_CMAP)) == 0)
	resource->type = PRT_RESOURCE_TYPE_CMAP;
    else
    {
	EMSG2(_("E619: file \"%s\" is not a supported PostScript resource file"),
		resource->filename);
	fclose(fd_resource);
	return FALSE;
    }

    /* Look for title and version of resource */
    resource->title[0] = '\0';
    resource->version[0] = '\0';
    seen_title = FALSE;
    seen_version = FALSE;
    seen_all = FALSE;
    while (!seen_all && prt_next_dsc(&dsc_line))
    {
        switch (dsc_line.type)
        {
        case PRT_DSC_TITLE_TYPE:
            STRNCPY(resource->title, dsc_line.string, dsc_line.len);
            resource->title[dsc_line.len] = '\0';
            seen_title = TRUE;
            if (seen_version)
                seen_all = TRUE;
            break;

        case PRT_DSC_VERSION_TYPE:
            STRNCPY(resource->version, dsc_line.string, dsc_line.len);
            resource->version[dsc_line.len] = '\0';
            seen_version = TRUE;
            if (seen_title)
                seen_all = TRUE;
            break;

        case PRT_DSC_ENDCOMMENTS_TYPE:
            /* Wont find title or resource after this comment, stop searching */
            seen_all = TRUE;
            break;

        case PRT_DSC_MISC_TYPE:
            /* Not interested in whatever comment this line had */
            break;
        }
    }

    if (!seen_title || !seen_version)
    {
	EMSG2(_("E619: file \"%s\" is not a supported PostScript resource file"),
		resource->filename);
	fclose(fd_resource);
	return FALSE;
    }

    fclose(fd_resource);

    return TRUE;
}

    static int
prt_check_resource(resource, version)
    struct prt_ps_resource_S *resource;
    char_u  *version;
{
    /* Version number m.n should match, the revision number does not matter */
    if (STRNCMP(resource->version, version, STRLEN(version)))
    {
	EMSG2(_("E621: \"%s\" resource file has wrong version"),
		resource->name);
	return FALSE;
    }

    /* Other checks to be added as needed */
    return TRUE;
}

    static void
prt_dsc_start()
{
    prt_write_string("%!PS-Adobe-3.0\n");
}

    static void
prt_dsc_noarg(comment)
    char	*comment;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
							 "%%%%%s\n", comment);
    prt_write_file(prt_line_buffer);
}

    static void
prt_dsc_textline(comment, text)
    char	*comment;
    char	*text;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
					       "%%%%%s: %s\n", comment, text);
    prt_write_file(prt_line_buffer);
}

    static void
prt_dsc_text(comment, text)
    char	*comment;
    char	*text;
{
    /* TODO - should scan 'text' for any chars needing escaping! */
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
					     "%%%%%s: (%s)\n", comment, text);
    prt_write_file(prt_line_buffer);
}

#define prt_dsc_atend(c)	prt_dsc_text((c), "atend")

    static void
prt_dsc_ints(comment, count, ints)
    char	*comment;
    int		count;
    int		*ints;
{
    int		i;

    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
							  "%%%%%s:", comment);
    prt_write_file(prt_line_buffer);

    for (i = 0; i < count; i++)
    {
	sprintf((char *)prt_line_buffer, " %d", ints[i]);
	prt_write_file(prt_line_buffer);
    }

    prt_write_string("\n");
}

    static void
prt_dsc_resources(comment, type, string)
    char	*comment;	/* if NULL add to previous */
    char	*type;
    char	*string;
{
    if (comment != NULL)
	vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
						 "%%%%%s: %s", comment, type);
    else
	vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
							    "%%%%+ %s", type);
    prt_write_file(prt_line_buffer);

    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
							     " %s\n", string);
    prt_write_file(prt_line_buffer);
}

    static void
prt_dsc_font_resource(resource, ps_font)
    char	*resource;
    struct prt_ps_font_S *ps_font;
{
    int     i;

    prt_dsc_resources(resource, "font",
                                    ps_font->ps_fontname[PRT_PS_FONT_ROMAN]);
    for (i = PRT_PS_FONT_BOLD ; i <= PRT_PS_FONT_BOLDOBLIQUE ; i++)
        if (ps_font->ps_fontname[i] != NULL)
            prt_dsc_resources(NULL, "font", ps_font->ps_fontname[i]);
}

    static void
prt_dsc_requirements(duplex, tumble, collate, color, num_copies)
    int		duplex;
    int		tumble;
    int		collate;
    int		color;
    int		num_copies;
{
    /* Only output the comment if we need to.
     * Note: tumble is ignored if we are not duplexing
     */
    if (!(duplex || collate || color || (num_copies > 1)))
	return;

    sprintf((char *)prt_line_buffer, "%%%%Requirements:");
    prt_write_file(prt_line_buffer);

    if (duplex)
    {
	prt_write_string(" duplex");
	if (tumble)
	    prt_write_string("(tumble)");
    }
    if (collate)
	prt_write_string(" collate");
    if (color)
	prt_write_string(" color");
    if (num_copies > 1)
    {
	prt_write_string(" numcopies(");
	/* Note: no space wanted so dont use prt_write_int() */
	sprintf((char *)prt_line_buffer, "%d", num_copies);
	prt_write_file(prt_line_buffer);
	prt_write_string(")");
    }
    prt_write_string("\n");
}

    static void
prt_dsc_docmedia(paper_name, width, height, weight, colour, type)
    char	*paper_name;
    double	width;
    double	height;
    double	weight;
    char	*colour;
    char	*type;
{
    vim_snprintf((char *)prt_line_buffer, sizeof(prt_line_buffer),
					"%%%%DocumentMedia: %s ", paper_name);
    prt_write_file(prt_line_buffer);
    prt_write_real(width, 2);
    prt_write_real(height, 2);
    prt_write_real(weight, 2);
    if (colour == NULL)
	prt_write_string("()");
    else
	prt_write_string(colour);
    prt_write_string(" ");
    if (type == NULL)
	prt_write_string("()");
    else
	prt_write_string(type);
    prt_write_string("\n");
}

    void
mch_print_cleanup()
{
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
    {
        int     i;

        /* Free off all CID font names created, but first clear duplicate
         * pointers to the same string (when the same font is used for more than
         * one style).
         */
        for (i = PRT_PS_FONT_ROMAN; i <= PRT_PS_FONT_BOLDOBLIQUE; i++)
        {
            if (prt_ps_mb_font.ps_fontname[i] != NULL)
                vim_free(prt_ps_mb_font.ps_fontname[i]);
            prt_ps_mb_font.ps_fontname[i] = NULL;
        }
    }

    if (prt_do_conv)
    {
	convert_setup(&prt_conv, NULL, NULL);
	prt_do_conv = FALSE;
    }
#endif
    if (prt_ps_fd != NULL)
    {
	fclose(prt_ps_fd);
	prt_ps_fd = NULL;
	prt_file_error = FALSE;
    }
    if (prt_ps_file_name != NULL)
    {
	vim_free(prt_ps_file_name);
	prt_ps_file_name = NULL;
    }
}

    static float
to_device_units(idx, physsize, def_number)
    int		idx;
    double	physsize;
    int		def_number;
{
    float	ret;
    int		u;
    int		nr;

    u = prt_get_unit(idx);
    if (u == PRT_UNIT_NONE)
    {
	u = PRT_UNIT_PERC;
	nr = def_number;
    }
    else
	nr = printer_opts[idx].number;

    switch (u)
    {
	case PRT_UNIT_INCH:
	    ret = (float)(nr * PRT_PS_DEFAULT_DPI);
	    break;
	case PRT_UNIT_MM:
	    ret = (float)(nr * PRT_PS_DEFAULT_DPI) / (float)25.4;
	    break;
	case PRT_UNIT_POINT:
	    ret = (float)nr;
	    break;
	case PRT_UNIT_PERC:
	default:
	    ret = (float)(physsize * nr) / 100;
	    break;
    }

    return ret;
}

/*
 * Calculate margins for given width and height from printoptions settings.
 */
    static void
prt_page_margins(width, height, left, right, top, bottom)
    double	width;
    double	height;
    double	*left;
    double	*right;
    double	*top;
    double	*bottom;
{
    *left   = to_device_units(OPT_PRINT_LEFT, width, 10);
    *right  = width - to_device_units(OPT_PRINT_RIGHT, width, 5);
    *top    = height - to_device_units(OPT_PRINT_TOP, height, 5);
    *bottom = to_device_units(OPT_PRINT_BOT, height, 5);
}

    static void
prt_font_metrics(font_scale)
    int		font_scale;
{
    prt_line_height = (float)font_scale;
    prt_char_width = (float)PRT_PS_FONT_TO_USER(font_scale, prt_ps_font->wx);
}


    static int
prt_get_cpl()
{
    if (prt_use_number())
    {
	prt_number_width = PRINT_NUMBER_WIDTH * prt_char_width;
#ifdef FEAT_MBYTE
        /* If we are outputting multi-byte characters then line numbers will be
         * printed with half width characters
         */
        if (prt_out_mbyte)
            prt_number_width /= 2;
#endif
	prt_left_margin += prt_number_width;
    }
    else
	prt_number_width = 0.0;

    return (int)((prt_right_margin - prt_left_margin) / prt_char_width);
}

#ifdef FEAT_MBYTE
    static int
prt_build_cid_fontname(font, name, name_len)
    int     font;
    char_u  *name;
    int     name_len;
{
    char    *fontname;

    fontname = (char *)alloc(name_len + 1);
    if (fontname == NULL)
        return FALSE;
    STRNCPY(fontname, name, name_len);
    fontname[name_len] = '\0';
    prt_ps_mb_font.ps_fontname[font] = fontname;

    return TRUE;
}
#endif

/*
 * Get number of lines of text that fit on a page (excluding the header).
 */
    static int
prt_get_lpp()
{
    int lpp;

    /*
     * Calculate offset to lower left corner of background rect based on actual
     * font height (based on its bounding box) and the line height, handling the
     * case where the font height can exceed the line height.
     */
    prt_bgcol_offset = (float)PRT_PS_FONT_TO_USER(prt_line_height,
					   prt_ps_font->bbox_min_y);
    if ((prt_ps_font->bbox_max_y - prt_ps_font->bbox_min_y) < 1000.0)
    {
	prt_bgcol_offset -= (float)PRT_PS_FONT_TO_USER(prt_line_height,
				(1000.0 - (prt_ps_font->bbox_max_y -
					    prt_ps_font->bbox_min_y)) / 2);
    }

    /* Get height for topmost line based on background rect offset. */
    prt_first_line_height = prt_line_height + prt_bgcol_offset;

    /* Calculate lpp */
    lpp = (int)((prt_top_margin - prt_bottom_margin) / prt_line_height);

    /* Adjust top margin if there is a header */
    prt_top_margin -= prt_line_height * prt_header_height();

    return lpp - prt_header_height();
}

#ifdef FEAT_MBYTE
    static int
prt_match_encoding(p_encoding, p_cmap, pp_mbenc)
    char			*p_encoding;
    struct prt_ps_mbfont_S	*p_cmap;
    struct prt_ps_encoding_S	**pp_mbenc;
{
    int				mbenc;
    int				enc_len;
    struct prt_ps_encoding_S	*p_mbenc;

    *pp_mbenc = NULL;
    /* Look for recognised encoding */
    enc_len = STRLEN(p_encoding);
    p_mbenc = p_cmap->encodings;
    for (mbenc = 0; mbenc < p_cmap->num_encodings; mbenc++)
    {
        if (STRNICMP(p_mbenc->encoding, p_encoding, enc_len) == 0)
        {
            *pp_mbenc = p_mbenc;
            return TRUE;
        }
        p_mbenc++;
    }
    return FALSE;
}

    static int
prt_match_charset(p_charset, p_cmap, pp_mbchar)
    char		    *p_charset;
    struct prt_ps_mbfont_S  *p_cmap;
    struct prt_ps_charset_S **pp_mbchar;
{
    int			    mbchar;
    int			    char_len;
    struct prt_ps_charset_S *p_mbchar;

    /* Look for recognised character set, using default if one is not given */
    if (*p_charset == NUL)
        p_charset = p_cmap->defcs;
    char_len = STRLEN(p_charset);
    p_mbchar = p_cmap->charsets;
    for (mbchar = 0; mbchar < p_cmap->num_charsets; mbchar++)
    {
        if (STRNICMP(p_mbchar->charset, p_charset, char_len) == 0)
        {
            *pp_mbchar = p_mbchar;
            return TRUE;
        }
        p_mbchar++;
    }
    return FALSE;
}
#endif

/*ARGSUSED*/
    int
mch_print_init(psettings, jobname, forceit)
    prt_settings_T *psettings;
    char_u	*jobname;
    int		forceit;
{
    int		i;
    char	*paper_name;
    int		paper_strlen;
    int		fontsize;
    char_u	*p;
    double      left;
    double      right;
    double      top;
    double      bottom;
#ifdef FEAT_MBYTE
    int         cmap;
    int         pmcs_len;
    char_u	*p_encoding;
    struct prt_ps_encoding_S *p_mbenc;
    struct prt_ps_encoding_S *p_mbenc_first;
    struct prt_ps_charset_S  *p_mbchar;
#endif

#if 0
    /*
     * TODO:
     * If "forceit" is false: pop up a dialog to select:
     *	- printer name
     *	- copies
     *	- collated/uncollated
     *	- duplex off/long side/short side
     *	- paper size
     *	- portrait/landscape
     *	- font size
     *
     * If "forceit" is true: use the default printer and settings
     */
    if (forceit)
	s_pd.Flags |= PD_RETURNDEFAULT;
#endif

    /*
     * Set up font and encoding.
     */
#ifdef FEAT_MBYTE
    p_encoding = enc_skip(p_penc);
    if (*p_encoding == NUL)
        p_encoding = enc_skip(p_enc);

    /* Look for recognised multi-byte coding, and if the charset is recognised.
     * This is to cope with the fact that various unicode encodings are
     * supported in more than one of CJK. */
    p_mbenc = NULL;
    p_mbenc_first = NULL;
    p_mbchar = NULL;
    for (cmap = 0; cmap < NUM_ELEMENTS(prt_ps_mbfonts); cmap++)
        if (prt_match_encoding((char *)p_encoding, &prt_ps_mbfonts[cmap],
								    &p_mbenc))
        {
            if (p_mbenc_first == NULL)
                p_mbenc_first = p_mbenc;
            if (prt_match_charset((char *)p_pmcs, &prt_ps_mbfonts[cmap],
								   &p_mbchar))
                break;
        }

    /* Use first encoding matched if no charset matched */
    if (p_mbchar == NULL && p_mbenc_first != NULL)
        p_mbenc = p_mbenc_first;

    prt_out_mbyte = (p_mbenc != NULL);
    if (prt_out_mbyte)
    {
        /* Build CMap name - will be same for all multi-byte fonts used */
        prt_cmap[0] = '\0';

        prt_custom_cmap = prt_out_mbyte && p_mbchar == NULL;

        if (!prt_custom_cmap)
        {
            /* Check encoding and character set are compatible */
            if ((p_mbenc->needs_charset&p_mbchar->has_charset) == 0)
            {
                EMSG(_("E673: Incompatible multi-byte encoding and character set."));
                return FALSE;
            }

            /* Add charset name if not empty */
            if (p_mbchar->cmap_charset != NULL)
            {
                STRCAT(prt_cmap, p_mbchar->cmap_charset);
                STRCAT(prt_cmap, "-");
            }
        }
        else
        {
            /* Add custom CMap character set name */
            pmcs_len = STRLEN(p_pmcs);
            if (pmcs_len == 0)
            {
                EMSG(_("E674: printmbcharset cannot be empty with multi-byte encoding."));
                return FALSE;
            }
            STRNCPY(prt_cmap, p_pmcs, STRLEN(p_pmcs));
            prt_cmap[pmcs_len] = '\0';
            STRCAT(prt_cmap, "-");
        }

        /* CMap name ends with (optional) encoding name and -H for horizontal */
        if (p_mbenc->cmap_encoding != NULL)
        {
            STRCAT(prt_cmap, p_mbenc->cmap_encoding);
            STRCAT(prt_cmap, "-");
        }
        STRCAT(prt_cmap, "H");

        if (!mbfont_opts[OPT_MBFONT_REGULAR].present)
        {
            EMSG(_("E675: No default font specified for multi-byte printing."));
            return FALSE;
        }

        /* Derive CID font names with fallbacks if not defined */
        if (!prt_build_cid_fontname(PRT_PS_FONT_ROMAN,
                                    mbfont_opts[OPT_MBFONT_REGULAR].string,
                                    mbfont_opts[OPT_MBFONT_REGULAR].strlen))
            return FALSE;
        if (mbfont_opts[OPT_MBFONT_BOLD].present)
            if (!prt_build_cid_fontname(PRT_PS_FONT_BOLD,
                                        mbfont_opts[OPT_MBFONT_BOLD].string,
                                        mbfont_opts[OPT_MBFONT_BOLD].strlen))
                return FALSE;
        if (mbfont_opts[OPT_MBFONT_OBLIQUE].present)
            if (!prt_build_cid_fontname(PRT_PS_FONT_OBLIQUE,
                                        mbfont_opts[OPT_MBFONT_OBLIQUE].string,
                                        mbfont_opts[OPT_MBFONT_OBLIQUE].strlen))
                return FALSE;
        if (mbfont_opts[OPT_MBFONT_BOLDOBLIQUE].present)
            if (!prt_build_cid_fontname(PRT_PS_FONT_BOLDOBLIQUE,
				   mbfont_opts[OPT_MBFONT_BOLDOBLIQUE].string,
				  mbfont_opts[OPT_MBFONT_BOLDOBLIQUE].strlen))
                return FALSE;

        /* Check if need to use Courier for ASCII code range, and if so pick up
         * the encoding to use */
        prt_use_courier = mbfont_opts[OPT_MBFONT_USECOURIER].present &&
            (TOLOWER_ASC(mbfont_opts[OPT_MBFONT_USECOURIER].string[0]) == 'y');
        if (prt_use_courier)
        {
            /* Use national ASCII variant unless ASCII wanted */
            if (mbfont_opts[OPT_MBFONT_ASCII].present &&
                (TOLOWER_ASC(mbfont_opts[OPT_MBFONT_ASCII].string[0]) == 'y'))
                prt_ascii_encoding = "ascii";
            else
                prt_ascii_encoding = prt_ps_mbfonts[cmap].ascii_enc;
        }

        prt_ps_font = &prt_ps_mb_font;
    }
    else
#endif
    {
#ifdef FEAT_MBYTE
        prt_use_courier = FALSE;
#endif
        prt_ps_font = &prt_ps_courier_font;
    }

    /*
     * Find the size of the paper and set the margins.
     */
    prt_portrait = (!printer_opts[OPT_PRINT_PORTRAIT].present
	   || TOLOWER_ASC(printer_opts[OPT_PRINT_PORTRAIT].string[0]) == 'y');
    if (printer_opts[OPT_PRINT_PAPER].present)
    {
	paper_name = (char *)printer_opts[OPT_PRINT_PAPER].string;
	paper_strlen = printer_opts[OPT_PRINT_PAPER].strlen;
    }
    else
    {
	paper_name = "A4";
	paper_strlen = 2;
    }
    for (i = 0; i < PRT_MEDIASIZE_LEN; ++i)
	if (STRLEN(prt_mediasize[i].name) == (unsigned)paper_strlen
		&& STRNICMP(prt_mediasize[i].name, paper_name,
							   paper_strlen) == 0)
	    break;
    if (i == PRT_MEDIASIZE_LEN)
	i = 0;
    prt_media = i;

    /*
     * Set PS pagesize based on media dimensions and print orientation.
     * Note: Media and page sizes have defined meanings in PostScript and should
     * be kept distinct.  Media is the paper (or transparency, or ...) that is
     * printed on, whereas the page size is the area that the PostScript
     * interpreter renders into.
     */
    if (prt_portrait)
    {
	prt_page_width = prt_mediasize[i].width;
	prt_page_height = prt_mediasize[i].height;
    }
    else
    {
	prt_page_width = prt_mediasize[i].height;
	prt_page_height = prt_mediasize[i].width;
    }

    /*
     * Set PS page margins based on the PS pagesize, not the mediasize - this
     * needs to be done before the cpl and lpp are calculated.
     */
    prt_page_margins(prt_page_width, prt_page_height, &left, &right, &top,
								    &bottom);
    prt_left_margin = (float)left;
    prt_right_margin = (float)right;
    prt_top_margin = (float)top;
    prt_bottom_margin = (float)bottom;

    /*
     * Set up the font size.
     */
    fontsize = PRT_PS_DEFAULT_FONTSIZE;
    for (p = p_pfn; (p = vim_strchr(p, ':')) != NULL; ++p)
	if (p[1] == 'h' && VIM_ISDIGIT(p[2]))
	    fontsize = atoi((char *)p + 2);
    prt_font_metrics(fontsize);

    /*
     * Return the number of characters per line, and lines per page for the
     * generic print code.
     */
    psettings->chars_per_line = prt_get_cpl();
    psettings->lines_per_page = prt_get_lpp();

    /* Catch margin settings that leave no space for output! */
    if (psettings->chars_per_line <= 0 || psettings->lines_per_page <= 0)
	return FAIL;

    /*
     * Sort out the number of copies to be printed.  PS by default will do
     * uncollated copies for you, so once we know how many uncollated copies are
     * wanted cache it away and lie to the generic code that we only want one
     * uncollated copy.
     */
    psettings->n_collated_copies = 1;
    psettings->n_uncollated_copies = 1;
    prt_num_copies = 1;
    prt_collate = (!printer_opts[OPT_PRINT_COLLATE].present
	    || TOLOWER_ASC(printer_opts[OPT_PRINT_COLLATE].string[0]) == 'y');
    if (prt_collate)
    {
	/* TODO: Get number of collated copies wanted. */
	psettings->n_collated_copies = 1;
    }
    else
    {
	/* TODO: Get number of uncollated copies wanted and update the cached
	 * count.
	 */
	prt_num_copies = 1;
    }

    psettings->jobname = jobname;

    /*
     * Set up printer duplex and tumble based on Duplex option setting - default
     * is long sided duplex printing (i.e. no tumble).
     */
    prt_duplex = TRUE;
    prt_tumble = FALSE;
    psettings->duplex = 1;
    if (printer_opts[OPT_PRINT_DUPLEX].present)
    {
	if (STRNICMP(printer_opts[OPT_PRINT_DUPLEX].string, "off", 3) == 0)
	{
	    prt_duplex = FALSE;
	    psettings->duplex = 0;
	}
	else if (STRNICMP(printer_opts[OPT_PRINT_DUPLEX].string, "short", 5)
									 == 0)
	    prt_tumble = TRUE;
    }

    /* For now user abort not supported */
    psettings->user_abort = 0;

    /* If the user didn't specify a file name, use a temp file. */
    if (psettings->outfile == NULL)
    {
	prt_ps_file_name = vim_tempname('p');
	if (prt_ps_file_name == NULL)
	{
	    EMSG(_(e_notmp));
	    return FAIL;
	}
	prt_ps_fd = mch_fopen((char *)prt_ps_file_name, WRITEBIN);
    }
    else
    {
	p = expand_env_save(psettings->outfile);
	if (p != NULL)
	{
	    prt_ps_fd = mch_fopen((char *)p, WRITEBIN);
	    vim_free(p);
	}
    }
    if (prt_ps_fd == NULL)
    {
	EMSG(_("E324: Can't open PostScript output file"));
	mch_print_cleanup();
	return FAIL;
    }

    prt_bufsiz = psettings->chars_per_line;
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
        prt_bufsiz *= 2;
#endif
    ga_init2(&prt_ps_buffer, (int)sizeof(char), prt_bufsiz);

    prt_page_num = 0;

    prt_attribute_change = FALSE;
    prt_need_moveto = FALSE;
    prt_need_font = FALSE;
    prt_need_fgcol = FALSE;
    prt_need_bgcol = FALSE;
    prt_need_underline = FALSE;

    prt_file_error = FALSE;

    return OK;
}

    static int
prt_add_resource(resource)
    struct prt_ps_resource_S *resource;
{
    FILE*	fd_resource;
    char_u	resource_buffer[512];
    size_t	bytes_read;

    fd_resource = mch_fopen((char *)resource->filename, READBIN);
    if (fd_resource == NULL)
    {
	EMSG2(_("E456: Can't open file \"%s\""), resource->filename);
	return FALSE;
    }
    prt_dsc_resources("BeginResource", prt_resource_types[resource->type],
						     (char *)resource->title);

    prt_dsc_textline("BeginDocument", (char *)resource->filename);

    for (;;)
    {
	bytes_read = fread((char *)resource_buffer, sizeof(char_u),
			   sizeof(resource_buffer), fd_resource);
	if (ferror(fd_resource))
	{
	    EMSG2(_("E457: Can't read PostScript resource file \"%s\""),
							    resource->filename);
	    fclose(fd_resource);
	    return FALSE;
	}
	if (bytes_read == 0)
	    break;
	prt_write_file_raw_len(resource_buffer, bytes_read);
	if (prt_file_error)
	{
	    fclose(fd_resource);
	    return FALSE;
	}
    }
    fclose(fd_resource);

    prt_dsc_noarg("EndDocument");

    prt_dsc_noarg("EndResource");

    return TRUE;
}

    int
mch_print_begin(psettings)
    prt_settings_T *psettings;
{
    time_t	now;
    int		bbox[4];
    char	*p_time;
    double      left;
    double      right;
    double      top;
    double      bottom;
    struct prt_ps_resource_S res_prolog;
    struct prt_ps_resource_S res_encoding;
    char	buffer[256];
    char_u      *p_encoding;
#ifdef FEAT_MBYTE
    struct prt_ps_resource_S res_cidfont;
    struct prt_ps_resource_S res_cmap;
#endif

    /*
     * PS DSC Header comments - no PS code!
     */
    prt_dsc_start();
    prt_dsc_textline("Title", (char *)psettings->jobname);
    if (!get_user_name((char_u *)buffer, 256))
        STRCPY(buffer, "Unknown");
    prt_dsc_textline("For", buffer);
    prt_dsc_textline("Creator", VIM_VERSION_LONG);
    /* Note: to ensure Clean8bit I don't think we can use LC_TIME */
    now = time(NULL);
    p_time = ctime(&now);
    /* Note: ctime() adds a \n so we have to remove it :-( */
    *(vim_strchr((char_u *)p_time, '\n')) = '\0';
    prt_dsc_textline("CreationDate", p_time);
    prt_dsc_textline("DocumentData", "Clean8Bit");
    prt_dsc_textline("Orientation", "Portrait");
    prt_dsc_atend("Pages");
    prt_dsc_textline("PageOrder", "Ascend");
    /* The bbox does not change with orientation - it is always in the default
     * user coordinate system!  We have to recalculate right and bottom
     * coordinates based on the font metrics for the bbox to be accurate. */
    prt_page_margins(prt_mediasize[prt_media].width,
					    prt_mediasize[prt_media].height,
					    &left, &right, &top, &bottom);
    bbox[0] = (int)left;
    if (prt_portrait)
    {
	/* In portrait printing the fixed point is the top left corner so we
	 * derive the bbox from that point.  We have the expected cpl chars
	 * across the media and lpp lines down the media.
	 */
	bbox[1] = (int)(top - (psettings->lines_per_page + prt_header_height())
							    * prt_line_height);
	bbox[2] = (int)(left + psettings->chars_per_line * prt_char_width
									+ 0.5);
	bbox[3] = (int)(top + 0.5);
    }
    else
    {
	/* In landscape printing the fixed point is the bottom left corner so we
	 * derive the bbox from that point.  We have lpp chars across the media
	 * and cpl lines up the media.
	 */
	bbox[1] = (int)bottom;
	bbox[2] = (int)(left + ((psettings->lines_per_page
			      + prt_header_height()) * prt_line_height) + 0.5);
	bbox[3] = (int)(bottom + psettings->chars_per_line * prt_char_width
									+ 0.5);
    }
    prt_dsc_ints("BoundingBox", 4, bbox);
    /* The media width and height does not change with landscape printing! */
    prt_dsc_docmedia(prt_mediasize[prt_media].name,
				prt_mediasize[prt_media].width,
				prt_mediasize[prt_media].height,
				(double)0, NULL, NULL);
    /* Define fonts needed */
#ifdef FEAT_MBYTE
    if (!prt_out_mbyte || prt_use_courier)
#endif
        prt_dsc_font_resource("DocumentNeededResources", &prt_ps_courier_font);
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
    {
        prt_dsc_font_resource((prt_use_courier ? NULL
                                 : "DocumentNeededResources"), &prt_ps_mb_font);
        if (!prt_custom_cmap)
            prt_dsc_resources(NULL, "cmap", prt_cmap);
    }
#endif

    /* Search for external resources VIM supplies */
    if (!prt_find_resource("prolog", &res_prolog))
    {
	EMSG(_("E456: Can't find PostScript resource file \"prolog.ps\""));
	return FALSE;
    }
    if (!prt_open_resource(&res_prolog))
	return FALSE;
    if (!prt_check_resource(&res_prolog, PRT_PROLOG_VERSION))
	return FALSE;
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
    {
        /* Look for required version of multi-byte printing procset */
        if (!prt_find_resource("cidfont", &res_cidfont))
        {
            EMSG(_("E456: Can't find PostScript resource file \"cidfont.ps\""));
            return FALSE;
        }
        if (!prt_open_resource(&res_cidfont))
            return FALSE;
        if (!prt_check_resource(&res_cidfont, PRT_CID_PROLOG_VERSION))
            return FALSE;
    }
#endif

    /* Find an encoding to use for printing.
     * Check 'printencoding'. If not set or not found, then use 'encoding'. If
     * that cannot be found then default to "latin1".
     * Note: VIM specific encoding header is always skipped.
     */
#ifdef FEAT_MBYTE
    if (!prt_out_mbyte)
    {
#endif
        p_encoding = enc_skip(p_penc);
        if (*p_encoding == NUL
                || !prt_find_resource((char *)p_encoding, &res_encoding))
        {
            /* 'printencoding' not set or not supported - find alternate */
#ifdef FEAT_MBYTE
            int		props;

            p_encoding = enc_skip(p_enc);
            props = enc_canon_props(p_encoding);
            if (!(props & ENC_8BIT)
                    || !prt_find_resource((char *)p_encoding, &res_encoding))
                /* 8-bit 'encoding' is not supported */
#endif
                {
                /* Use latin1 as default printing encoding */
                p_encoding = (char_u *)"latin1";
                if (!prt_find_resource((char *)p_encoding, &res_encoding))
                {
                    EMSG2(_("E456: Can't find PostScript resource file \"%s.ps\""),
                            p_encoding);
                    return FALSE;
                }
            }
        }
        if (!prt_open_resource(&res_encoding))
            return FALSE;
        /* For the moment there are no checks on encoding resource files to
         * perform */
#ifdef FEAT_MBYTE
    }
    else
    {
        p_encoding = enc_skip(p_penc);
        if (*p_encoding == NUL)
            p_encoding = enc_skip(p_enc);
        if (prt_use_courier)
        {
            /* Include ASCII range encoding vector */
            if (!prt_find_resource(prt_ascii_encoding, &res_encoding))
            {
                EMSG2(_("E456: Can't find PostScript resource file \"%s.ps\""),
							  prt_ascii_encoding);
                return FALSE;
            }
            if (!prt_open_resource(&res_encoding))
                return FALSE;
            /* For the moment there are no checks on encoding resource files to
             * perform */
        }
    }

    prt_conv.vc_type = CONV_NONE;
    if (!(enc_canon_props(p_enc) & enc_canon_props(p_encoding) & ENC_8BIT)) {
        /* Set up encoding conversion if required */
	if (FAIL == convert_setup(&prt_conv, p_enc, p_encoding))
	{
            EMSG2(_("E620: Unable to convert to print encoding \"%s\""),
		    p_encoding);
	    return FALSE;
	}
	prt_do_conv = TRUE;
    }
    prt_do_conv = prt_conv.vc_type != CONV_NONE;

    if (prt_out_mbyte && prt_custom_cmap)
    {
        /* Find user supplied CMap */
        if (!prt_find_resource(prt_cmap, &res_cmap))
        {
            EMSG2(_("E456: Can't find PostScript resource file \"%s.ps\""),
								    prt_cmap);
            return FALSE;
        }
        if (!prt_open_resource(&res_cmap))
            return FALSE;
    }
#endif

    /* List resources supplied */
    STRCPY(buffer, res_prolog.title);
    STRCAT(buffer, " ");
    STRCAT(buffer, res_prolog.version);
    prt_dsc_resources("DocumentSuppliedResources", "procset", buffer);
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
    {
        STRCPY(buffer, res_cidfont.title);
        STRCAT(buffer, " ");
        STRCAT(buffer, res_cidfont.version);
        prt_dsc_resources(NULL, "procset", buffer);

        if (prt_custom_cmap)
        {
            STRCPY(buffer, res_cmap.title);
            STRCAT(buffer, " ");
            STRCAT(buffer, res_cmap.version);
            prt_dsc_resources(NULL, "cmap", buffer);
        }
    }
    if (!prt_out_mbyte || prt_use_courier)
#endif
    {
        STRCPY(buffer, res_encoding.title);
        STRCAT(buffer, " ");
        STRCAT(buffer, res_encoding.version);
        prt_dsc_resources(NULL, "encoding", buffer);
    }
    prt_dsc_requirements(prt_duplex, prt_tumble, prt_collate,
#ifdef FEAT_SYN_HL
					psettings->do_syntax
#else
					0
#endif
					, prt_num_copies);
    prt_dsc_noarg("EndComments");

    /*
     * PS Document page defaults
     */
    prt_dsc_noarg("BeginDefaults");

    /* List font resources most likely common to all pages */
#ifdef FEAT_MBYTE
    if (!prt_out_mbyte || prt_use_courier)
#endif
        prt_dsc_font_resource("PageResources", &prt_ps_courier_font);
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
    {
        prt_dsc_font_resource((prt_use_courier ? NULL : "PageResources"),
                                                            &prt_ps_mb_font);
        if (!prt_custom_cmap)
            prt_dsc_resources(NULL, "cmap", prt_cmap);
    }
#endif

    /* Paper will be used for all pages */
    prt_dsc_textline("PageMedia", prt_mediasize[prt_media].name);

    prt_dsc_noarg("EndDefaults");

    /*
     * PS Document prolog inclusion - all required procsets.
     */
    prt_dsc_noarg("BeginProlog");

    /* Add required procsets - NOTE: order is important! */
    if (!prt_add_resource(&res_prolog))
	return FALSE;
#ifdef FEAT_MBYTE
    if (prt_out_mbyte)
    {
        /* Add CID font procset, and any user supplied CMap */
        if (!prt_add_resource(&res_cidfont))
            return FALSE;
        if (prt_custom_cmap && !prt_add_resource(&res_cmap))
            return FALSE;
    }
#endif

#ifdef FEAT_MBYTE
    if (!prt_out_mbyte || prt_use_courier)
#endif
        /* There will be only one Roman font encoding to be included in the PS
         * file. */
        if (!prt_add_resource(&res_encoding))
            return FALSE;

    prt_dsc_noarg("EndProlog");

    /*
     * PS Document setup - must appear after the prolog
     */
    prt_dsc_noarg("BeginSetup");

    /* Device setup - page size and number of uncollated copies */
    prt_write_int((int)prt_mediasize[prt_media].width);
    prt_write_int((int)prt_mediasize[prt_media].height);
    prt_write_int(0);
    prt_write_string("sps\n");
    prt_write_int(prt_num_copies);
    prt_write_string("nc\n");
    prt_write_boolean(prt_duplex);
    prt_write_boolean(prt_tumble);
    prt_write_string("dt\n");
    prt_write_boolean(prt_collate);
    prt_write_string("c\n");

    /* Font resource inclusion and definition */
#ifdef FEAT_MBYTE
    if (!prt_out_mbyte || prt_use_courier)
    {
        /* When using Courier for ASCII range when printing multi-byte, need to
         * pick up ASCII encoding to use with it. */
        if (prt_use_courier)
            p_encoding = (char_u *)prt_ascii_encoding;
#endif
        prt_dsc_resources("IncludeResource", "font",
                          prt_ps_courier_font.ps_fontname[PRT_PS_FONT_ROMAN]);
        prt_def_font("F0", (char *)p_encoding, (int)prt_line_height,
                     prt_ps_courier_font.ps_fontname[PRT_PS_FONT_ROMAN]);
        prt_dsc_resources("IncludeResource", "font",
                          prt_ps_courier_font.ps_fontname[PRT_PS_FONT_BOLD]);
        prt_def_font("F1", (char *)p_encoding, (int)prt_line_height,
                     prt_ps_courier_font.ps_fontname[PRT_PS_FONT_BOLD]);
        prt_dsc_resources("IncludeResource", "font",
                          prt_ps_courier_font.ps_fontname[PRT_PS_FONT_OBLIQUE]);
        prt_def_font("F2", (char *)p_encoding, (int)prt_line_height,
                     prt_ps_courier_font.ps_fontname[PRT_PS_FONT_OBLIQUE]);
        prt_dsc_resources("IncludeResource", "font",
                          prt_ps_courier_font.ps_fontname[PRT_PS_FONT_BOLDOBLIQUE]);
        prt_def_font("F3", (char *)p_encoding, (int)prt_line_height,
                     prt_ps_courier_font.ps_fontname[PRT_PS_FONT_BOLDOBLIQUE]);
#ifdef FEAT_MBYTE
    }
    if (prt_out_mbyte)
    {
        /* Define the CID fonts to be used in the job.  Typically CJKV fonts do
         * not have an italic form being a western style, so where no font is
         * defined for these faces VIM falls back to an existing face.
         * Note: if using Courier for the ASCII range then the printout will
         * have bold/italic/bolditalic regardless of the setting of printmbfont.
         */
        prt_dsc_resources("IncludeResource", "font",
                          prt_ps_mb_font.ps_fontname[PRT_PS_FONT_ROMAN]);
        if (!prt_custom_cmap)
            prt_dsc_resources("IncludeResource", "cmap", prt_cmap);
        prt_def_cidfont("CF0", (int)prt_line_height,
                        prt_ps_mb_font.ps_fontname[PRT_PS_FONT_ROMAN]);

        if (prt_ps_mb_font.ps_fontname[PRT_PS_FONT_BOLD] != NULL)
        {
            prt_dsc_resources("IncludeResource", "font",
                              prt_ps_mb_font.ps_fontname[PRT_PS_FONT_BOLD]);
            if (!prt_custom_cmap)
                prt_dsc_resources("IncludeResource", "cmap", prt_cmap);
            prt_def_cidfont("CF1", (int)prt_line_height,
                            prt_ps_mb_font.ps_fontname[PRT_PS_FONT_BOLD]);
        }
        else
            /* Use ROMAN for BOLD */
            prt_dup_cidfont("CF0", "CF1");

        if (prt_ps_mb_font.ps_fontname[PRT_PS_FONT_OBLIQUE] != NULL)
        {
            prt_dsc_resources("IncludeResource", "font",
                              prt_ps_mb_font.ps_fontname[PRT_PS_FONT_OBLIQUE]);
            if (!prt_custom_cmap)
                prt_dsc_resources("IncludeResource", "cmap", prt_cmap);
            prt_def_cidfont("CF2", (int)prt_line_height,
                            prt_ps_mb_font.ps_fontname[PRT_PS_FONT_OBLIQUE]);
        }
        else
            /* Use ROMAN for OBLIQUE */
            prt_dup_cidfont("CF0", "CF2");

        if (prt_ps_mb_font.ps_fontname[PRT_PS_FONT_BOLDOBLIQUE] != NULL)
        {
            prt_dsc_resources("IncludeResource", "font",
                              prt_ps_mb_font.ps_fontname[PRT_PS_FONT_BOLDOBLIQUE]);
            if (!prt_custom_cmap)
                prt_dsc_resources("IncludeResource", "cmap", prt_cmap);
            prt_def_cidfont("CF3", (int)prt_line_height,
                            prt_ps_mb_font.ps_fontname[PRT_PS_FONT_BOLDOBLIQUE]);
        }
        else
            /* Use BOLD for BOLDOBLIQUE */
            prt_dup_cidfont("CF1", "CF3");
    }
#endif

    /* Misc constant vars used for underlining and background rects */
    prt_def_var("UO", PRT_PS_FONT_TO_USER(prt_line_height,
						prt_ps_font->uline_offset), 2);
    prt_def_var("UW", PRT_PS_FONT_TO_USER(prt_line_height,
						 prt_ps_font->uline_width), 2);
    prt_def_var("BO", prt_bgcol_offset, 2);

    prt_dsc_noarg("EndSetup");

    /* Fail if any problems writing out to the PS file */
    return !prt_file_error;
}

    void
mch_print_end(psettings)
    prt_settings_T *psettings;
{
    prt_dsc_noarg("Trailer");

    /*
     * Output any info we don't know in toto until we finish
     */
    prt_dsc_ints("Pages", 1, &prt_page_num);

    prt_dsc_noarg("EOF");

    /* Write CTRL-D to close serial communication link if used.
     * NOTHING MUST BE WRITTEN AFTER THIS! */
    prt_write_file((char_u *)IF_EB("\004", "\067"));

    if (!prt_file_error && psettings->outfile == NULL
					&& !got_int && !psettings->user_abort)
    {
	/* Close the file first. */
	if (prt_ps_fd != NULL)
	{
	    fclose(prt_ps_fd);
	    prt_ps_fd = NULL;
	}
	prt_message((char_u *)_("Sending to printer..."));

	/* Not printing to a file: use 'printexpr' to print the file. */
	if (eval_printexpr(prt_ps_file_name, psettings->arguments) == FAIL)
	    EMSG(_("E365: Failed to print PostScript file"));
	else
	    prt_message((char_u *)_("Print job sent."));
    }

    mch_print_cleanup();
}

    int
mch_print_end_page()
{
    prt_flush_buffer();

    prt_write_string("re sp\n");

    prt_dsc_noarg("PageTrailer");

    return !prt_file_error;
}

/*ARGSUSED*/
    int
mch_print_begin_page(str)
    char_u	*str;
{
    int		page_num[2];

    prt_page_num++;

    page_num[0] = page_num[1] = prt_page_num;
    prt_dsc_ints("Page", 2, page_num);

    prt_dsc_noarg("BeginPageSetup");

    prt_write_string("sv\n0 g\n");
#ifdef FEAT_MBYTE
    prt_in_ascii = !prt_out_mbyte;
    if (prt_out_mbyte)
        prt_write_string("CF0 sf\n");
    else
#endif
        prt_write_string("F0 sf\n");
    prt_fgcol = PRCOLOR_BLACK;
    prt_bgcol = PRCOLOR_WHITE;
    prt_font = PRT_PS_FONT_ROMAN;

    /* Set up page transformation for landscape printing. */
    if (!prt_portrait)
    {
	prt_write_int(-((int)prt_mediasize[prt_media].width));
	prt_write_string("sl\n");
    }

    prt_dsc_noarg("EndPageSetup");

    /* We have reset the font attributes, force setting them again. */
    curr_bg = (long_u)0xffffffff;
    curr_fg = (long_u)0xffffffff;
    curr_bold = MAYBE;

    return !prt_file_error;
}

    int
mch_print_blank_page()
{
    return (mch_print_begin_page(NULL) ? (mch_print_end_page()) : FALSE);
}

static float prt_pos_x = 0;
static float prt_pos_y = 0;

    void
mch_print_start_line(margin, page_line)
    int		margin;
    int		page_line;
{
    prt_pos_x = prt_left_margin;
    if (margin)
	prt_pos_x -= prt_number_width;

    prt_pos_y = prt_top_margin - prt_first_line_height -
					page_line * prt_line_height;

    prt_attribute_change = TRUE;
    prt_need_moveto = TRUE;
#ifdef FEAT_MBYTE
    prt_half_width = FALSE;
#endif
}

/*ARGSUSED*/
    int
mch_print_text_out(p, len)
    char_u	*p;
    int		len;
{
    int		need_break;
    char_u	ch;
    char_u      ch_buff[8];
    float       char_width;
    float       next_pos;
#ifdef FEAT_MBYTE
    int         in_ascii;
    int         half_width;
#endif

    char_width = prt_char_width;

#ifdef FEAT_MBYTE
    /* Ideally VIM would create a rearranged CID font to combine a Roman and
     * CJKV font to do what VIM is doing here - use a Roman font for characters
     * in the ASCII range, and the origingal CID font for everything else.
     * The problem is that GhostScript still (as of 8.13) does not support
     * rearranged fonts even though they have been documented by Adobe for 7
     * years!  If they ever do, a lot of this code will disappear.
     */
    if (prt_use_courier)
    {
        in_ascii = (len == 1 && *p < 0x80);
        if (prt_in_ascii)
        {
            if (!in_ascii)
            {
                /* No longer in ASCII range - need to switch font */
                prt_in_ascii = FALSE;
                prt_need_font = TRUE;
                prt_attribute_change = TRUE;
            }
        }
        else if (in_ascii)
        {
            /* Now in ASCII range - need to switch font */
            prt_in_ascii = TRUE;
            prt_need_font = TRUE;
            prt_attribute_change = TRUE;
        }
    }
    if (prt_out_mbyte)
    {
        half_width = ((*mb_ptr2cells)(p) == 1);
        if (half_width)
            char_width /= 2;
        if (prt_half_width)
        {
            if (!half_width)
            {
                prt_half_width = FALSE;
                prt_pos_x += prt_char_width/4;
                prt_need_moveto = TRUE;
                prt_attribute_change = TRUE;
            }
        }
        else if (half_width)
        {
            prt_half_width = TRUE;
            prt_pos_x += prt_char_width/4;
            prt_need_moveto = TRUE;
            prt_attribute_change = TRUE;
        }
    }
#endif

    /* Output any required changes to the graphics state, after flushing any
     * text buffered so far.
     */
    if (prt_attribute_change)
    {
	prt_flush_buffer();
	/* Reset count of number of chars that will be printed */
	prt_text_run = 0;

	if (prt_need_moveto)
	{
	    prt_pos_x_moveto = prt_pos_x;
	    prt_pos_y_moveto = prt_pos_y;
	    prt_do_moveto = TRUE;

	    prt_need_moveto = FALSE;
	}
	if (prt_need_font)
	{
#ifdef FEAT_MBYTE
            if (!prt_in_ascii)
                prt_write_string("CF");
            else
#endif
                prt_write_string("F");
            prt_write_int(prt_font);
            prt_write_string("sf\n");
            prt_need_font = FALSE;
	}
	if (prt_need_fgcol)
	{
	    int     r, g, b;
	    r = ((unsigned)prt_fgcol & 0xff0000) >> 16;
	    g = ((unsigned)prt_fgcol & 0xff00) >> 8;
	    b = prt_fgcol & 0xff;

	    prt_write_real(r / 255.0, 3);
	    if (r == g && g == b)
		prt_write_string("g\n");
	    else
	    {
		prt_write_real(g / 255.0, 3);
		prt_write_real(b / 255.0, 3);
		prt_write_string("r\n");
	    }
	    prt_need_fgcol = FALSE;
	}

	if (prt_bgcol != PRCOLOR_WHITE)
	{
	    prt_new_bgcol = prt_bgcol;
	    if (prt_need_bgcol)
		prt_do_bgcol = TRUE;
	}
	else
	    prt_do_bgcol = FALSE;
	prt_need_bgcol = FALSE;

	if (prt_need_underline)
	    prt_do_underline = prt_underline;
	prt_need_underline = FALSE;

	prt_attribute_change = FALSE;
    }

#ifdef FEAT_MBYTE
    if (prt_do_conv)
    {
	/* Convert from multi-byte to 8-bit encoding */
	p = string_convert(&prt_conv, p, &len);
	if (p == NULL)
	    p = (char_u *)"";
    }

    if (prt_out_mbyte)
    {
        /* Multi-byte character strings are represented more efficiently as hex
         * strings when outputting clean 8 bit PS.
         */
        do
        {
           ch = prt_hexchar[(unsigned)(*p) >> 4];
           ga_append(&prt_ps_buffer, ch);
           ch = prt_hexchar[(*p) & 0xf];
           ga_append(&prt_ps_buffer, ch);
           p++;
        }
        while (--len);
    }
    else
#endif
    {
        /* Add next character to buffer of characters to output.
         * Note: One printed character may require several PS characters to
         * represent it, but we only count them as one printed character.
         */
        ch = *p;
        if (ch < 32 || ch == '(' || ch == ')' || ch == '\\')
        {
            /* Convert non-printing characters to either their escape or octal
             * sequence, ensures PS sent over a serial line does not interfere
             * with the comms protocol.  Note: For EBCDIC we need to write out
             * the escape sequences as ASCII codes!
	     * Note 2: Char codes < 32 are identical in EBCDIC and ASCII AFAIK!
	     */
            ga_append(&prt_ps_buffer, IF_EB('\\', 0134));
            switch (ch)
            {
                case BS:   ga_append(&prt_ps_buffer, IF_EB('b', 0142)); break;
                case TAB:  ga_append(&prt_ps_buffer, IF_EB('t', 0164)); break;
                case NL:   ga_append(&prt_ps_buffer, IF_EB('n', 0156)); break;
                case FF:   ga_append(&prt_ps_buffer, IF_EB('f', 0146)); break;
                case CAR:  ga_append(&prt_ps_buffer, IF_EB('r', 0162)); break;
                case '(':  ga_append(&prt_ps_buffer, IF_EB('(', 0050)); break;
                case ')':  ga_append(&prt_ps_buffer, IF_EB(')', 0051)); break;
                case '\\': ga_append(&prt_ps_buffer, IF_EB('\\', 0134)); break;

                default:
                           sprintf((char *)ch_buff, "%03o", (unsigned int)ch);
#ifdef EBCDIC
                           ebcdic2ascii(ch_buff, 3);
#endif
                           ga_append(&prt_ps_buffer, ch_buff[0]);
                           ga_append(&prt_ps_buffer, ch_buff[1]);
                           ga_append(&prt_ps_buffer, ch_buff[2]);
                           break;
            }
        }
        else
            ga_append(&prt_ps_buffer, ch);
    }

#ifdef FEAT_MBYTE
    /* Need to free any translated characters */
    if (prt_do_conv && (*p != NUL))
	vim_free(p);
#endif

    prt_text_run += char_width;
    prt_pos_x += char_width;

    /* The downside of fp - use relative error on right margin check */
    next_pos = prt_pos_x + prt_char_width;
    need_break = (next_pos > prt_right_margin) &&
                    ((next_pos - prt_right_margin) > (prt_right_margin*1e-5));

    if (need_break)
	prt_flush_buffer();

    return need_break;
}

    void
mch_print_set_font(iBold, iItalic, iUnderline)
    int		iBold;
    int		iItalic;
    int		iUnderline;
{
    int		font = 0;

    if (iBold)
	font |= 0x01;
    if (iItalic)
	font |= 0x02;

    if (font != prt_font)
    {
	prt_font = font;
	prt_attribute_change = TRUE;
	prt_need_font = TRUE;
    }
    if (prt_underline != iUnderline)
    {
	prt_underline = iUnderline;
	prt_attribute_change = TRUE;
	prt_need_underline = TRUE;
    }
}

    void
mch_print_set_bg(bgcol)
    long_u	bgcol;
{
    prt_bgcol = bgcol;
    prt_attribute_change = TRUE;
    prt_need_bgcol = TRUE;
}

    void
mch_print_set_fg(fgcol)
    long_u	fgcol;
{
    if (fgcol != (long_u)prt_fgcol)
    {
	prt_fgcol = fgcol;
	prt_attribute_change = TRUE;
	prt_need_fgcol = TRUE;
    }
}

# endif /*FEAT_POSTSCRIPT*/
#endif /*FEAT_PRINTER*/

#if (defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
	&& (defined(FEAT_EVAL) || defined(FEAT_MULTI_LANG))
static char *get_locale_val __ARGS((int what));

    static char *
get_locale_val(what)
    int		what;
{
    char	*loc;

    /* Obtain the locale value from the libraries.  For DJGPP this is
     * redefined and it doesn't use the arguments. */
    loc = setlocale(what, NULL);

# if defined(__BORLANDC__)
    if (loc != NULL)
    {
	char_u	*p;

	/* Borland returns something like "LC_CTYPE=<name>\n"
	 * Let's try to fix that bug here... */
	p = vim_strchr(loc, '=');
	if (p != NULL)
	{
	    loc = ++p;
	    while (*p != NUL)	/* remove trailing newline */
	    {
		if (*p < ' ')
		{
		    *p = NUL;
		    break;
		}
		++p;
	    }
	}
    }
# endif

    return loc;
}
#endif


#ifdef WIN32
/*
 * On MS-Windows locale names are strings like "German_Germany.1252", but
 * gettext expects "de".  Try to translate one into another here for a few
 * supported languages.
 */
    static char_u *
gettext_lang(char_u *name)
{
    int		i;
    static char *(mtable[]) = {
			"afrikaans",	"af",
			"czech",	"cs",
			"dutch",	"nl",
			"german",	"de",
			"english_united kingdom", "en_GB",
			"spanish",	"es",
			"french",	"fr",
			"italian",	"it",
			"japanese",	"ja",
			"korean",	"ko",
			"norwegian",	"no",
			"polish",	"pl",
			"russian",	"ru",
			"slovak",	"sk",
			"swedish",	"sv",
			"ukrainian",	"uk",
			"chinese_china", "zh_CN",
			"chinese_taiwan", "zh_TW",
			NULL};

    for (i = 0; mtable[i] != NULL; i += 2)
	if (STRNICMP(mtable[i], name, STRLEN(mtable[i])) == 0)
	    return mtable[i + 1];
    return name;
}
#endif

#if defined(FEAT_MULTI_LANG) || defined(PROTO)
/*
 * Obtain the current messages language.  Used to set the default for
 * 'helplang'.  May return NULL or an empty string.
 */
    char_u *
get_mess_lang()
{
    char_u *p;

# if (defined(HAVE_LOCALE_H) || defined(X_LOCALE))
#  if defined(LC_MESSAGES)
    p = (char_u *)get_locale_val(LC_MESSAGES);
#  else
    /* This is necessary for Win32, where LC_MESSAGES is not defined and $LANG
     * may be set to the LCID number. */
    p = (char_u *)get_locale_val(LC_ALL);
#  endif
# else
    p = mch_getenv((char_u *)"LC_ALL");
    if (p == NULL || *p == NUL)
    {
	p = mch_getenv((char_u *)"LC_MESSAGES");
	if (p == NULL || *p == NUL)
	    p = mch_getenv((char_u *)"LANG");
    }
# endif
# ifdef WIN32
    p = gettext_lang(p);
# endif
    return p;
}
#endif

/* Complicated #if; matches with where get_mess_env() is used below. */
#if (defined(FEAT_EVAL) && !((defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
	    && defined(LC_MESSAGES))) \
	|| ((defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
		&& (defined(FEAT_GETTEXT) || defined(FEAT_MBYTE)) \
		&& !defined(LC_MESSAGES))
static char_u *get_mess_env __ARGS((void));

/*
 * Get the language used for messages from the environment.
 */
    static char_u *
get_mess_env()
{
    char_u	*p;

    p = mch_getenv((char_u *)"LC_ALL");
    if (p == NULL || *p == NUL)
    {
	p = mch_getenv((char_u *)"LC_MESSAGES");
	if (p == NULL || *p == NUL)
	{
	    p = mch_getenv((char_u *)"LANG");
	    if (p != NULL && VIM_ISDIGIT(*p))
		p = NULL;		/* ignore something like "1043" */
# if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
	    if (p == NULL || *p == NUL)
		p = (char_u *)get_locale_val(LC_CTYPE);
# endif
	}
    }
    return p;
}
#endif

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Set the "v:lang" variable according to the current locale setting.
 * Also do "v:lc_time"and "v:ctype".
 */
    void
set_lang_var()
{
    char_u	*loc;

# if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    loc = (char_u *)get_locale_val(LC_CTYPE);
# else
    /* setlocale() not supported: use the default value */
    loc = (char_u *)"C";
# endif
    set_vim_var_string(VV_CTYPE, loc, -1);

    /* When LC_MESSAGES isn't defined use the value from $LC_MESSAGES, fall
     * back to LC_CTYPE if it's empty. */
# if (defined(HAVE_LOCALE_H) || defined(X_LOCALE)) && defined(LC_MESSAGES)
    loc = (char_u *)get_locale_val(LC_MESSAGES);
# else
    loc = get_mess_env();
# endif
    set_vim_var_string(VV_LANG, loc, -1);

# if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    loc = (char_u *)get_locale_val(LC_TIME);
# endif
    set_vim_var_string(VV_LC_TIME, loc, -1);
}
#endif

#if (defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
	&& (defined(FEAT_GETTEXT) || defined(FEAT_MBYTE))
/*
 * ":language":  Set the language (locale).
 */
    void
ex_language(eap)
    exarg_T	*eap;
{
    char	*loc;
    char_u	*p;
    char_u	*name;
    int		what = LC_ALL;
    char	*whatstr = "";
#ifdef LC_MESSAGES
# define VIM_LC_MESSAGES LC_MESSAGES
#else
# define VIM_LC_MESSAGES 6789
#endif

    name = eap->arg;

    /* Check for "messages {name}", "ctype {name}" or "time {name}" argument.
     * Allow abbreviation, but require at least 3 characters to avoid
     * confusion with a two letter language name "me" or "ct". */
    p = skiptowhite(eap->arg);
    if ((*p == NUL || vim_iswhite(*p)) && p - eap->arg >= 3)
    {
	if (STRNICMP(eap->arg, "messages", p - eap->arg) == 0)
	{
	    what = VIM_LC_MESSAGES;
	    name = skipwhite(p);
	    whatstr = "messages ";
	}
	else if (STRNICMP(eap->arg, "ctype", p - eap->arg) == 0)
	{
	    what = LC_CTYPE;
	    name = skipwhite(p);
	    whatstr = "ctype ";
	}
	else if (STRNICMP(eap->arg, "time", p - eap->arg) == 0)
	{
	    what = LC_TIME;
	    name = skipwhite(p);
	    whatstr = "time ";
	}
    }

    if (*name == NUL)
    {
#ifndef LC_MESSAGES
	if (what == VIM_LC_MESSAGES)
	    p = get_mess_env();
	else
#endif
	    p = (char_u *)setlocale(what, NULL);
	if (p == NULL || *p == NUL)
	    p = (char_u *)"Unknown";
	smsg((char_u *)_("Current %slanguage: \"%s\""), whatstr, p);
    }
    else
    {
#ifndef LC_MESSAGES
	if (what == VIM_LC_MESSAGES)
	    loc = "";
	else
#endif
	    loc = setlocale(what, (char *)name);
	if (loc == NULL)
	    EMSG2(_("E197: Cannot set language to \"%s\""), name);
	else
	{
#ifdef HAVE_NL_MSG_CAT_CNTR
	    /* Need to do this for GNU gettext, otherwise cached translations
	     * will be used again. */
	    extern int _nl_msg_cat_cntr;

	    ++_nl_msg_cat_cntr;
#endif
	    /* Reset $LC_ALL, otherwise it would overrule everyting. */
	    vim_setenv((char_u *)"LC_ALL", (char_u *)"");

	    if (what != LC_TIME)
	    {
		/* Tell gettext() what to translate to.  It apparently doesn't
		 * use the currently effective locale.  Also do this when
		 * FEAT_GETTEXT isn't defined, so that shell commands use this
		 * value. */
		if (what == LC_ALL)
		    vim_setenv((char_u *)"LANG", name);
		if (what != LC_CTYPE)
		{
		    char_u	*mname;
#ifdef WIN32
		    mname = gettext_lang(name);
#else
		    mname = name;
#endif
		    vim_setenv((char_u *)"LC_MESSAGES", mname);
#ifdef FEAT_MULTI_LANG
		    set_helplang_default(mname);
#endif
		}

		/* Set $LC_CTYPE, because it overrules $LANG, and
		 * gtk_set_locale() calls setlocale() again.  gnome_init()
		 * sets $LC_CTYPE to "en_US" (that's a bug!). */
		if (what != VIM_LC_MESSAGES)
		    vim_setenv((char_u *)"LC_CTYPE", name);
# ifdef FEAT_GUI_GTK
		/* Let GTK know what locale we're using.  Not sure this is
		 * really needed... */
		if (gui.in_use)
		    (void)gtk_set_locale();
# endif
	    }

# ifdef FEAT_EVAL
	    /* Set v:lang, v:lc_time and v:ctype to the final result. */
	    set_lang_var();
# endif
	}
    }
}

# if defined(FEAT_CMDL_COMPL) || defined(PROTO)
/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":language" command.
 */
/*ARGSUSED*/
    char_u *
get_lang_arg(xp, idx)
    expand_T	*xp;
    int		idx;
{
    if (idx == 0)
	return (char_u *)"messages";
    if (idx == 1)
	return (char_u *)"ctype";
    if (idx == 2)
	return (char_u *)"time";
    return NULL;
}
# endif

#endif
