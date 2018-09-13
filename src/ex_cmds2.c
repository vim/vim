/* vi:set ts=8 sts=4 sw=4 noet:
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

#include "vim.h"
#include "version.h"

static void	cmd_source(char_u *fname, exarg_T *eap);

#ifdef FEAT_EVAL
/* Growarray to store info about already sourced scripts.
 * For Unix also store the dev/ino, so that we don't have to stat() each
 * script when going through the list. */
typedef struct scriptitem_S
{
    char_u	*sn_name;
# ifdef UNIX
    int		sn_dev_valid;
    dev_t	sn_dev;
    ino_t	sn_ino;
# endif
# ifdef FEAT_PROFILE
    int		sn_prof_on;	/* TRUE when script is/was profiled */
    int		sn_pr_force;	/* forceit: profile functions in this script */
    proftime_T	sn_pr_child;	/* time set when going into first child */
    int		sn_pr_nest;	/* nesting for sn_pr_child */
    /* profiling the script as a whole */
    int		sn_pr_count;	/* nr of times sourced */
    proftime_T	sn_pr_total;	/* time spent in script + children */
    proftime_T	sn_pr_self;	/* time spent in script itself */
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
    proftime_T	sn_prl_total;	/* time spent in a line + children */
    proftime_T	sn_prl_self;	/* time spent in a line itself */
} sn_prl_T;

#  define PRL_ITEM(si, idx)	(((sn_prl_T *)(si)->sn_prl_ga.ga_data)[(idx)])
# endif
#endif

#if defined(FEAT_EVAL) || defined(PROTO)
static int debug_greedy = FALSE;	/* batch mode debugging: don't save
					   and restore typeahead. */
static int get_maxbacktrace_level(void);
static void do_setdebugtracelevel(char_u *arg);
static void do_checkbacktracelevel(void);
static void do_showbacktrace(char_u *cmd);

static char_u *debug_oldval = NULL;	/* old and newval for debug expressions */
static char_u *debug_newval = NULL;
static int     debug_expr   = 0;        /* use debug_expr */

    int
has_watchexpr(void)
{
    return debug_expr;
}

/*
 * do_debug(): Debug mode.
 * Repeatedly get Ex commands, until told to continue normal execution.
 */
    void
do_debug(char_u *cmd)
{
    int		save_msg_scroll = msg_scroll;
    int		save_State = State;
    int		save_did_emsg = did_emsg;
    int		save_cmd_silent = cmd_silent;
    int		save_msg_silent = msg_silent;
    int		save_emsg_silent = emsg_silent;
    int		save_redir_off = redir_off;
    tasave_T	typeaheadbuf;
    int		typeahead_saved = FALSE;
    int		save_ignore_script = 0;
    int		save_ex_normal_busy;
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
#define CMD_BACKTRACE	7
#define CMD_FRAME	8
#define CMD_UP		9
#define CMD_DOWN	10

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
    debug_mode = TRUE;

    if (!debug_did_msg)
	MSG(_("Entering Debug mode.  Type \"cont\" to continue."));
    if (debug_oldval != NULL)
    {
	smsg((char_u *)_("Oldval = \"%s\""), debug_oldval);
	vim_free(debug_oldval);
	debug_oldval = NULL;
    }
    if (debug_newval != NULL)
    {
	smsg((char_u *)_("Newval = \"%s\""), debug_newval);
	vim_free(debug_newval);
	debug_newval = NULL;
    }
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

	/* Save the current typeahead buffer and replace it with an empty one.
	 * This makes sure we get input from the user here and don't interfere
	 * with the commands being executed.  Reset "ex_normal_busy" to avoid
	 * the side effects of using ":normal". Save the stuff buffer and make
	 * it empty. Set ignore_script to avoid reading from script input. */
	save_ex_normal_busy = ex_normal_busy;
	ex_normal_busy = 0;
	if (!debug_greedy)
	{
	    save_typeahead(&typeaheadbuf);
	    typeahead_saved = TRUE;
	    save_ignore_script = ignore_script;
	    ignore_script = TRUE;
	}

	vim_free(cmdline);
	cmdline = getcmdline_prompt('>', NULL, 0, EXPAND_NOTHING, NULL);

	if (typeahead_saved)
	{
	    restore_typeahead(&typeaheadbuf);
	    ignore_script = save_ignore_script;
	}
	ex_normal_busy = save_ex_normal_busy;

	cmdline_row = msg_row;
	msg_starthere();
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
		    case 'f':
			      last_cmd = 0;
			      if (p[1] == 'r')
			      {
				  last_cmd = CMD_FRAME;
				  tail = "rame";
			      }
			      else
			      {
				  last_cmd = CMD_FINISH;
				  tail = "inish";
			      }
			      break;
		    case 'q': last_cmd = CMD_QUIT;
			      tail = "uit";
			      break;
		    case 'i': last_cmd = CMD_INTERRUPT;
			      tail = "nterrupt";
			      break;
		    case 'b': last_cmd = CMD_BACKTRACE;
			      if (p[1] == 't')
				  tail = "t";
			      else
				  tail = "acktrace";
			      break;
		    case 'w': last_cmd = CMD_BACKTRACE;
			      tail = "here";
			      break;
		    case 'u': last_cmd = CMD_UP;
			      tail = "p";
			      break;
		    case 'd': last_cmd = CMD_DOWN;
			      tail = "own";
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
		    if (ASCII_ISALPHA(*p) && last_cmd != CMD_FRAME)
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
		    case CMD_BACKTRACE:
			do_showbacktrace(cmd);
			continue;
		    case CMD_FRAME:
			if (*p == NUL)
			{
			    do_showbacktrace(cmd);
			}
			else
			{
			    p = skipwhite(p);
			    do_setdebugtracelevel(p);
			}
			continue;
		    case CMD_UP:
			debug_backtrace_level++;
			do_checkbacktracelevel();
			continue;
		    case CMD_DOWN:
			debug_backtrace_level--;
			do_checkbacktracelevel();
			continue;
		}
		/* Going out reset backtrace_level */
		debug_backtrace_level = 0;
		break;
	    }

	    /* don't debug this command */
	    n = debug_break_level;
	    debug_break_level = -1;
	    (void)do_cmdline(cmdline, getexline, NULL,
						DOCMD_VERBOSE|DOCMD_EXCRESET);
	    debug_break_level = n;
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
    debug_mode = FALSE;
    did_emsg = save_did_emsg;
    cmd_silent = save_cmd_silent;
    msg_silent = save_msg_silent;
    emsg_silent = save_emsg_silent;
    redir_off = save_redir_off;

    /* Only print the message again when typing a command before coming back
     * here. */
    debug_did_msg = TRUE;
}

    static int
get_maxbacktrace_level(void)
{
    char	*p, *q;
    int		maxbacktrace = 0;

    if (sourcing_name != NULL)
    {
	p = (char *)sourcing_name;
	while ((q = strstr(p, "..")) != NULL)
	{
	    p = q + 2;
	    maxbacktrace++;
	}
    }
    return maxbacktrace;
}

    static void
do_setdebugtracelevel(char_u *arg)
{
    int level;

    level = atoi((char *)arg);
    if (*arg == '+' || level < 0)
	debug_backtrace_level += level;
    else
	debug_backtrace_level = level;

    do_checkbacktracelevel();
}

    static void
do_checkbacktracelevel(void)
{
    if (debug_backtrace_level < 0)
    {
	debug_backtrace_level = 0;
	MSG(_("frame is zero"));
    }
    else
    {
	int max = get_maxbacktrace_level();

	if (debug_backtrace_level > max)
	{
	    debug_backtrace_level = max;
	    smsg((char_u *)_("frame at highest level: %d"), max);
	}
    }
}

    static void
do_showbacktrace(char_u *cmd)
{
    char    *cur;
    char    *next;
    int	    i = 0;
    int	    max = get_maxbacktrace_level();

    if (sourcing_name != NULL)
    {
	cur = (char *)sourcing_name;
	while (!got_int)
	{
	    next = strstr(cur, "..");
	    if (next != NULL)
		*next = NUL;
	    if (i == max - debug_backtrace_level)
		smsg((char_u *)"->%d %s", max - i, cur);
	    else
		smsg((char_u *)"  %d %s", max - i, cur);
	    ++i;
	    if (next == NULL)
		break;
	    *next = '.';
	    cur = next + 2;
	}
    }
    if (sourcing_lnum != 0)
       smsg((char_u *)_("line %ld: %s"), (long)sourcing_lnum, cmd);
    else
       smsg((char_u *)_("cmd: %s"), cmd);
}

/*
 * ":debug".
 */
    void
ex_debug(exarg_T *eap)
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
dbg_check_breakpoint(exarg_T *eap)
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
dbg_check_skipped(exarg_T *eap)
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
    int		dbg_type;	/* DBG_FUNC, DBG_FILE or DBG_EXPR */
    char_u	*dbg_name;	/* function, expression or file name */
    regprog_T	*dbg_prog;	/* regexp program */
    linenr_T	dbg_lnum;	/* line number in function or file */
    int		dbg_forceit;	/* ! used */
#ifdef FEAT_EVAL
    typval_T    *dbg_val;       /* last result of watchexpression */
#endif
    int		dbg_level;      /* stored nested level for expr */
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
#define DBG_EXPR	3

static int dbg_parsearg(char_u *arg, garray_T *gap);
static linenr_T debuggy_find(int file,char_u *fname, linenr_T after, garray_T *gap, int *fp);

/*
 * Parse the arguments of ":profile", ":breakadd" or ":breakdel" and put them
 * in the entry just after the last one in dbg_breakp.  Note that "dbg_name"
 * is allocated.
 * Returns FAIL for failure.
 */
    static int
dbg_parsearg(
    char_u	*arg,
    garray_T	*gap)	    /* either &dbg_breakp or &prof_ga */
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
    else if (
#ifdef FEAT_PROFILE
	    gap != &prof_ga &&
#endif
	    STRNCMP(p, "expr", 4) == 0)
	bp->dbg_type = DBG_EXPR;
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
    else if (bp->dbg_type == DBG_EXPR)
    {
	bp->dbg_name = vim_strsave(p);
	if (bp->dbg_name != NULL)
	    bp->dbg_val = eval_expr(bp->dbg_name, NULL);
    }
    else
    {
	/* Expand the file name in the same way as do_source().  This means
	 * doing it twice, so that $DIR/file gets expanded when $DIR is
	 * "~/dir". */
	q = expand_env_save(p);
	if (q == NULL)
	    return FAIL;
	p = expand_env_save(q);
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
    }

    if (bp->dbg_name == NULL)
	return FAIL;
    return OK;
}

/*
 * ":breakadd".  Also used for ":profile".
 */
    void
ex_breakadd(exarg_T *eap)
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

	if (bp->dbg_type != DBG_EXPR)
	{
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
	else
	{
	    /* DBG_EXPR */
	    DEBUGGY(gap, gap->ga_len++).dbg_nr = ++last_breakp;
	    ++debug_tick;
	}
    }
}

/*
 * ":debuggreedy".
 */
    void
ex_debuggreedy(exarg_T *eap)
{
    if (eap->addr_count == 0 || eap->line2 != 0)
	debug_greedy = TRUE;
    else
	debug_greedy = FALSE;
}

/*
 * ":breakdel" and ":profdel".
 */
    void
ex_breakdel(exarg_T *eap)
{
    struct debuggy *bp, *bpi;
    int		nr;
    int		todel = -1;
    int		del_all = FALSE;
    int		i;
    linenr_T	best_lnum = 0;
    garray_T	*gap;

    gap = &dbg_breakp;
    if (eap->cmdidx == CMD_profdel)
    {
#ifdef FEAT_PROFILE
	gap = &prof_ga;
#else
	ex_ni(eap);
	return;
#endif
    }

    if (vim_isdigit(*eap->arg))
    {
	/* ":breakdel {nr}" */
	nr = atol((char *)eap->arg);
	for (i = 0; i < gap->ga_len; ++i)
	    if (DEBUGGY(gap, i).dbg_nr == nr)
	    {
		todel = i;
		break;
	    }
    }
    else if (*eap->arg == '*')
    {
	todel = 0;
	del_all = TRUE;
    }
    else
    {
	/* ":breakdel {func|file|expr} [lnum] {name}" */
	if (dbg_parsearg(eap->arg, gap) == FAIL)
	    return;
	bp = &DEBUGGY(gap, gap->ga_len);
	for (i = 0; i < gap->ga_len; ++i)
	{
	    bpi = &DEBUGGY(gap, i);
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
	while (gap->ga_len > 0)
	{
	    vim_free(DEBUGGY(gap, todel).dbg_name);
#ifdef FEAT_EVAL
	    if (DEBUGGY(gap, todel).dbg_type == DBG_EXPR
		    && DEBUGGY(gap, todel).dbg_val != NULL)
		free_tv(DEBUGGY(gap, todel).dbg_val);
#endif
	    vim_regfree(DEBUGGY(gap, todel).dbg_prog);
	    --gap->ga_len;
	    if (todel < gap->ga_len)
		mch_memmove(&DEBUGGY(gap, todel), &DEBUGGY(gap, todel + 1),
			      (gap->ga_len - todel) * sizeof(struct debuggy));
#ifdef FEAT_PROFILE
	    if (eap->cmdidx == CMD_breakdel)
#endif
		++debug_tick;
	    if (!del_all)
		break;
	}

	/* If all breakpoints were removed clear the array. */
	if (gap->ga_len == 0)
	    ga_clear(gap);
    }
}

/*
 * ":breaklist".
 */
    void
ex_breaklist(exarg_T *eap UNUSED)
{
    struct debuggy *bp;
    int		i;

    if (dbg_breakp.ga_len == 0)
	MSG(_("No breakpoints defined"));
    else
	for (i = 0; i < dbg_breakp.ga_len; ++i)
	{
	    bp = &BREAKP(i);
	    if (bp->dbg_type == DBG_FILE)
		home_replace(NULL, bp->dbg_name, NameBuff, MAXPATHL, TRUE);
	    if (bp->dbg_type != DBG_EXPR)
		smsg((char_u *)_("%3d  %s %s  line %ld"),
		    bp->dbg_nr,
		    bp->dbg_type == DBG_FUNC ? "func" : "file",
		    bp->dbg_type == DBG_FUNC ? bp->dbg_name : NameBuff,
		    (long)bp->dbg_lnum);
	    else
		smsg((char_u *)_("%3d  expr %s"),
		    bp->dbg_nr, bp->dbg_name);
	}
}

/*
 * Find a breakpoint for a function or sourced file.
 * Returns line number at which to break; zero when no matching breakpoint.
 */
    linenr_T
dbg_find_breakpoint(
    int		file,	    /* TRUE for a file, FALSE for a function */
    char_u	*fname,	    /* file or function name */
    linenr_T	after)	    /* after this line number */
{
    return debuggy_find(file, fname, after, &dbg_breakp, NULL);
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Return TRUE if profiling is on for a function or sourced file.
 */
    int
has_profiling(
    int		file,	    /* TRUE for a file, FALSE for a function */
    char_u	*fname,	    /* file or function name */
    int		*fp)	    /* return: forceit */
{
    return (debuggy_find(file, fname, (linenr_T)0, &prof_ga, fp)
							      != (linenr_T)0);
}
#endif

/*
 * Common code for dbg_find_breakpoint() and has_profiling().
 */
    static linenr_T
debuggy_find(
    int		file,	    /* TRUE for a file, FALSE for a function */
    char_u	*fname,	    /* file or function name */
    linenr_T	after,	    /* after this line number */
    garray_T	*gap,	    /* either &dbg_breakp or &prof_ga */
    int		*fp)	    /* if not NULL: return forceit */
{
    struct debuggy *bp;
    int		i;
    linenr_T	lnum = 0;
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
	if (((bp->dbg_type == DBG_FILE) == file &&
		bp->dbg_type != DBG_EXPR && (
#ifdef FEAT_PROFILE
		gap == &prof_ga ||
#endif
		(bp->dbg_lnum > after && (lnum == 0 || bp->dbg_lnum < lnum)))))
	{
	    /*
	     * Save the value of got_int and reset it.  We don't want a
	     * previous interruption cancel matching, only hitting CTRL-C
	     * while matching should abort it.
	     */
	    prev_got_int = got_int;
	    got_int = FALSE;
	    if (vim_regexec_prog(&bp->dbg_prog, FALSE, name, (colnr_T)0))
	    {
		lnum = bp->dbg_lnum;
		if (fp != NULL)
		    *fp = bp->dbg_forceit;
	    }
	    got_int |= prev_got_int;
	}
#ifdef FEAT_EVAL
	else if (bp->dbg_type == DBG_EXPR)
	{
	    typval_T *tv;
	    int	      line = FALSE;

	    prev_got_int = got_int;
	    got_int = FALSE;

	    tv = eval_expr(bp->dbg_name, NULL);
	    if (tv != NULL)
	    {
		if (bp->dbg_val == NULL)
		{
		    debug_oldval = typval_tostring(NULL);
		    bp->dbg_val = tv;
		    debug_newval = typval_tostring(bp->dbg_val);
		    line = TRUE;
		}
		else
		{
		    if (typval_compare(tv, bp->dbg_val, TYPE_EQUAL,
							     TRUE, FALSE) == OK
			    && tv->vval.v_number == FALSE)
		    {
			typval_T *v;

			line = TRUE;
			debug_oldval = typval_tostring(bp->dbg_val);
			/* Need to evaluate again, typval_compare() overwrites
			 * "tv". */
			v = eval_expr(bp->dbg_name, NULL);
			debug_newval = typval_tostring(v);
			free_tv(bp->dbg_val);
			bp->dbg_val = v;
		    }
		    free_tv(tv);
		}
	    }
	    else if (bp->dbg_val != NULL)
	    {
		debug_oldval = typval_tostring(bp->dbg_val);
		debug_newval = typval_tostring(NULL);
		free_tv(bp->dbg_val);
		bp->dbg_val = NULL;
		line = TRUE;
	    }

	    if (line)
	    {
		lnum = after > 0 ? after : 1;
		break;
	    }

	    got_int |= prev_got_int;
	}
#endif
    }
    if (name != fname)
	vim_free(name);

    return lnum;
}

/*
 * Called when a breakpoint was encountered.
 */
    void
dbg_breakpoint(char_u *name, linenr_T lnum)
{
    /* We need to check if this line is actually executed in do_one_cmd() */
    debug_breakpoint_name = name;
    debug_breakpoint_lnum = lnum;
}


# if defined(FEAT_PROFILE) || defined(FEAT_RELTIME) || defined(PROTO)
/*
 * Store the current time in "tm".
 */
    void
profile_start(proftime_T *tm)
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
profile_end(proftime_T *tm)
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
profile_sub(proftime_T *tm, proftime_T *tm2)
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
 * Return a string that represents the time in "tm".
 * Uses a static buffer!
 */
    char *
profile_msg(proftime_T *tm)
{
    static char buf[50];

# ifdef WIN3264
    LARGE_INTEGER   fr;

    QueryPerformanceFrequency(&fr);
    sprintf(buf, "%10.6lf", (double)tm->QuadPart / (double)fr.QuadPart);
# else
    sprintf(buf, "%3ld.%06ld", (long)tm->tv_sec, (long)tm->tv_usec);
# endif
    return buf;
}

# if defined(FEAT_FLOAT) || defined(PROTO)
/*
 * Return a float that represents the time in "tm".
 */
    float_T
profile_float(proftime_T *tm)
{
#  ifdef WIN3264
    LARGE_INTEGER   fr;

    QueryPerformanceFrequency(&fr);
    return (float_T)tm->QuadPart / (float_T)fr.QuadPart;
#  else
    return (float_T)tm->tv_sec + (float_T)tm->tv_usec / 1000000.0;
#  endif
}
# endif

/*
 * Put the time "msec" past now in "tm".
 */
    void
profile_setlimit(long msec, proftime_T *tm)
{
    if (msec <= 0)   /* no limit */
	profile_zero(tm);
    else
    {
# ifdef WIN3264
	LARGE_INTEGER   fr;

	QueryPerformanceCounter(tm);
	QueryPerformanceFrequency(&fr);
	tm->QuadPart += (LONGLONG)((double)msec / 1000.0 * (double)fr.QuadPart);
# else
	long	    usec;

	gettimeofday(tm, NULL);
	usec = (long)tm->tv_usec + (long)msec * 1000;
	tm->tv_usec = usec % 1000000L;
	tm->tv_sec += usec / 1000000L;
# endif
    }
}

/*
 * Return TRUE if the current time is past "tm".
 */
    int
profile_passed_limit(proftime_T *tm)
{
    proftime_T	now;

# ifdef WIN3264
    if (tm->QuadPart == 0)  /* timer was not set */
	return FALSE;
    QueryPerformanceCounter(&now);
    return (now.QuadPart > tm->QuadPart);
# else
    if (tm->tv_sec == 0)    /* timer was not set */
	return FALSE;
    gettimeofday(&now, NULL);
    return (now.tv_sec > tm->tv_sec
	    || (now.tv_sec == tm->tv_sec && now.tv_usec > tm->tv_usec));
# endif
}

/*
 * Set the time in "tm" to zero.
 */
    void
profile_zero(proftime_T *tm)
{
# ifdef WIN3264
    tm->QuadPart = 0;
# else
    tm->tv_usec = 0;
    tm->tv_sec = 0;
# endif
}

# endif  /* FEAT_PROFILE || FEAT_RELTIME */

# if defined(FEAT_TIMERS) || defined(PROTO)
static timer_T	*first_timer = NULL;
static long	last_timer_id = 0;

    long
proftime_time_left(proftime_T *due, proftime_T *now)
{
#  ifdef WIN3264
    LARGE_INTEGER fr;

    if (now->QuadPart > due->QuadPart)
	return 0;
    QueryPerformanceFrequency(&fr);
    return (long)(((double)(due->QuadPart - now->QuadPart)
		   / (double)fr.QuadPart) * 1000);
#  else
    if (now->tv_sec > due->tv_sec)
	return 0;
    return (due->tv_sec - now->tv_sec) * 1000
	+ (due->tv_usec - now->tv_usec) / 1000;
#  endif
}

/*
 * Insert a timer in the list of timers.
 */
    static void
insert_timer(timer_T *timer)
{
    timer->tr_next = first_timer;
    timer->tr_prev = NULL;
    if (first_timer != NULL)
	first_timer->tr_prev = timer;
    first_timer = timer;
    did_add_timer = TRUE;
}

/*
 * Take a timer out of the list of timers.
 */
    static void
remove_timer(timer_T *timer)
{
    if (timer->tr_prev == NULL)
	first_timer = timer->tr_next;
    else
	timer->tr_prev->tr_next = timer->tr_next;
    if (timer->tr_next != NULL)
	timer->tr_next->tr_prev = timer->tr_prev;
}

    static void
free_timer(timer_T *timer)
{
    free_callback(timer->tr_callback, timer->tr_partial);
    vim_free(timer);
}

/*
 * Create a timer and return it.  NULL if out of memory.
 * Caller should set the callback.
 */
    timer_T *
create_timer(long msec, int repeat)
{
    timer_T	*timer = (timer_T *)alloc_clear(sizeof(timer_T));
    long	prev_id = last_timer_id;

    if (timer == NULL)
	return NULL;
    if (++last_timer_id <= prev_id)
	/* Overflow!  Might cause duplicates... */
	last_timer_id = 0;
    timer->tr_id = last_timer_id;
    insert_timer(timer);
    if (repeat != 0)
	timer->tr_repeat = repeat - 1;
    timer->tr_interval = msec;

    profile_setlimit(msec, &timer->tr_due);
    return timer;
}

/*
 * Invoke the callback of "timer".
 */
    static void
timer_callback(timer_T *timer)
{
    typval_T	rettv;
    int		dummy;
    typval_T	argv[2];

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = (varnumber_T)timer->tr_id;
    argv[1].v_type = VAR_UNKNOWN;

    call_func(timer->tr_callback, (int)STRLEN(timer->tr_callback),
			&rettv, 1, argv, NULL, 0L, 0L, &dummy, TRUE,
			timer->tr_partial, NULL);
    clear_tv(&rettv);
}

/*
 * Call timers that are due.
 * Return the time in msec until the next timer is due.
 * Returns -1 if there are no pending timers.
 */
    long
check_due_timer(void)
{
    timer_T	*timer;
    timer_T	*timer_next;
    long	this_due;
    long	next_due = -1;
    proftime_T	now;
    int		did_one = FALSE;
    int		need_update_screen = FALSE;
    long	current_id = last_timer_id;

    /* Don't run any timers while exiting or dealing with an error. */
    if (exiting || aborting())
	return next_due;

    profile_start(&now);
    for (timer = first_timer; timer != NULL && !got_int; timer = timer_next)
    {
	timer_next = timer->tr_next;

	if (timer->tr_id == -1 || timer->tr_firing || timer->tr_paused)
	    continue;
	this_due = proftime_time_left(&timer->tr_due, &now);
	if (this_due <= 1)
	{
	    /* Save and restore a lot of flags, because the timer fires while
	     * waiting for a character, which might be halfway a command. */
	    int save_timer_busy = timer_busy;
	    int save_vgetc_busy = vgetc_busy;
	    int save_did_emsg = did_emsg;
	    int save_called_emsg = called_emsg;
	    int	save_must_redraw = must_redraw;
	    int	save_trylevel = trylevel;
	    int save_did_throw = did_throw;
	    int save_ex_pressedreturn = get_pressedreturn();
	    except_T *save_current_exception = current_exception;
	    vimvars_save_T vvsave;

	    /* Create a scope for running the timer callback, ignoring most of
	     * the current scope, such as being inside a try/catch. */
	    timer_busy = timer_busy > 0 || vgetc_busy > 0;
	    vgetc_busy = 0;
	    called_emsg = FALSE;
	    did_emsg = FALSE;
	    did_uncaught_emsg = FALSE;
	    must_redraw = 0;
	    trylevel = 0;
	    did_throw = FALSE;
	    current_exception = NULL;
	    save_vimvars(&vvsave);

	    timer->tr_firing = TRUE;
	    timer_callback(timer);
	    timer->tr_firing = FALSE;

	    timer_next = timer->tr_next;
	    did_one = TRUE;
	    timer_busy = save_timer_busy;
	    vgetc_busy = save_vgetc_busy;
	    if (did_uncaught_emsg)
		++timer->tr_emsg_count;
	    did_emsg = save_did_emsg;
	    called_emsg = save_called_emsg;
	    trylevel = save_trylevel;
	    did_throw = save_did_throw;
	    current_exception = save_current_exception;
	    restore_vimvars(&vvsave);
	    if (must_redraw != 0)
		need_update_screen = TRUE;
	    must_redraw = must_redraw > save_must_redraw
					      ? must_redraw : save_must_redraw;
	    set_pressedreturn(save_ex_pressedreturn);

	    /* Only fire the timer again if it repeats and stop_timer() wasn't
	     * called while inside the callback (tr_id == -1). */
	    if (timer->tr_repeat != 0 && timer->tr_id != -1
		    && timer->tr_emsg_count < 3)
	    {
		profile_setlimit(timer->tr_interval, &timer->tr_due);
		this_due = proftime_time_left(&timer->tr_due, &now);
		if (this_due < 1)
		    this_due = 1;
		if (timer->tr_repeat > 0)
		    --timer->tr_repeat;
	    }
	    else
	    {
		this_due = -1;
		remove_timer(timer);
		free_timer(timer);
	    }
	}
	if (this_due > 0 && (next_due == -1 || next_due > this_due))
	    next_due = this_due;
    }

    if (did_one)
	redraw_after_callback(need_update_screen);

#ifdef FEAT_BEVAL_TERM
    if (bevalexpr_due_set)
    {
	this_due = proftime_time_left(&bevalexpr_due, &now);
	if (this_due <= 1)
	{
	    bevalexpr_due_set = FALSE;
	    if (balloonEval == NULL)
	    {
		balloonEval = (BalloonEval *)alloc_clear(sizeof(BalloonEval));
		balloonEvalForTerm = TRUE;
	    }
	    if (balloonEval != NULL)
		general_beval_cb(balloonEval, 0);
	}
	else if (next_due == -1 || next_due > this_due)
	    next_due = this_due;
    }
#endif
#ifdef FEAT_TERMINAL
    /* Some terminal windows may need their buffer updated. */
    next_due = term_check_timers(next_due, &now);
#endif

    return current_id != last_timer_id ? 1 : next_due;
}

/*
 * Find a timer by ID.  Returns NULL if not found;
 */
    timer_T *
find_timer(long id)
{
    timer_T *timer;

    if (id >= 0)
    {
	for (timer = first_timer; timer != NULL; timer = timer->tr_next)
	    if (timer->tr_id == id)
		return timer;
    }
    return NULL;
}


/*
 * Stop a timer and delete it.
 */
    void
stop_timer(timer_T *timer)
{
    if (timer->tr_firing)
	/* Free the timer after the callback returns. */
	timer->tr_id = -1;
    else
    {
	remove_timer(timer);
	free_timer(timer);
    }
}

    void
stop_all_timers(void)
{
    timer_T *timer;
    timer_T *timer_next;

    for (timer = first_timer; timer != NULL; timer = timer_next)
    {
	timer_next = timer->tr_next;
	stop_timer(timer);
    }
}

    void
add_timer_info(typval_T *rettv, timer_T *timer)
{
    list_T	*list = rettv->vval.v_list;
    dict_T	*dict = dict_alloc();
    dictitem_T	*di;
    long	remaining;
    proftime_T	now;

    if (dict == NULL)
	return;
    list_append_dict(list, dict);

    dict_add_number(dict, "id", timer->tr_id);
    dict_add_number(dict, "time", (long)timer->tr_interval);

    profile_start(&now);
    remaining = proftime_time_left(&timer->tr_due, &now);
    dict_add_number(dict, "remaining", (long)remaining);

    dict_add_number(dict, "repeat",
		    (long)(timer->tr_repeat < 0 ? -1 : timer->tr_repeat + 1));
    dict_add_number(dict, "paused", (long)(timer->tr_paused));

    di = dictitem_alloc((char_u *)"callback");
    if (di != NULL)
    {
	if (dict_add(dict, di) == FAIL)
	    vim_free(di);
	else if (timer->tr_partial != NULL)
	{
	    di->di_tv.v_type = VAR_PARTIAL;
	    di->di_tv.vval.v_partial = timer->tr_partial;
	    ++timer->tr_partial->pt_refcount;
	}
	else
	{
	    di->di_tv.v_type = VAR_FUNC;
	    di->di_tv.vval.v_string = vim_strsave(timer->tr_callback);
	}
    }
}

    void
add_timer_info_all(typval_T *rettv)
{
    timer_T *timer;

    for (timer = first_timer; timer != NULL; timer = timer->tr_next)
	if (timer->tr_id != -1)
	    add_timer_info(rettv, timer);
}

/*
 * Mark references in partials of timers.
 */
    int
set_ref_in_timer(int copyID)
{
    int		abort = FALSE;
    timer_T	*timer;
    typval_T	tv;

    for (timer = first_timer; timer != NULL; timer = timer->tr_next)
    {
	if (timer->tr_partial != NULL)
	{
	    tv.v_type = VAR_PARTIAL;
	    tv.vval.v_partial = timer->tr_partial;
	}
	else
	{
	    tv.v_type = VAR_FUNC;
	    tv.vval.v_string = timer->tr_callback;
	}
	abort = abort || set_ref_in_item(&tv, copyID, NULL, NULL);
    }
    return abort;
}

#  if defined(EXITFREE) || defined(PROTO)
    void
timer_free_all()
{
    timer_T *timer;

    while (first_timer != NULL)
    {
	timer = first_timer;
	remove_timer(timer);
	free_timer(timer);
    }
}
#  endif
# endif

#if defined(FEAT_SYN_HL) && defined(FEAT_RELTIME) && defined(FEAT_FLOAT)
# if defined(HAVE_MATH_H)
#  include <math.h>
# endif

/*
 * Divide the time "tm" by "count" and store in "tm2".
 */
    void
profile_divide(proftime_T *tm, int count, proftime_T *tm2)
{
    if (count == 0)
	profile_zero(tm2);
    else
    {
# ifdef WIN3264
	tm2->QuadPart = tm->QuadPart / count;
# else
	double usec = (tm->tv_sec * 1000000.0 + tm->tv_usec) / count;

	tm2->tv_sec = floor(usec / 1000000.0);
	tm2->tv_usec = vim_round(usec - (tm2->tv_sec * 1000000.0));
# endif
    }
}
#endif

# if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Functions for profiling.
 */
static void script_do_profile(scriptitem_T *si);
static void script_dump_profile(FILE *fd);
static proftime_T prof_wait_time;

/*
 * Add the time "tm2" to "tm".
 */
    void
profile_add(proftime_T *tm, proftime_T *tm2)
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
 * Add the "self" time from the total time and the children's time.
 */
    void
profile_self(proftime_T *self, proftime_T *total, proftime_T *children)
{
    /* Check that the result won't be negative.  Can happen with recursive
     * calls. */
#ifdef WIN3264
    if (total->QuadPart <= children->QuadPart)
	return;
#else
    if (total->tv_sec < children->tv_sec
	    || (total->tv_sec == children->tv_sec
		&& total->tv_usec <= children->tv_usec))
	return;
#endif
    profile_add(self, total);
    profile_sub(self, children);
}

/*
 * Get the current waittime.
 */
    void
profile_get_wait(proftime_T *tm)
{
    *tm = prof_wait_time;
}

/*
 * Subtract the passed waittime since "tm" from "tma".
 */
    void
profile_sub_wait(proftime_T *tm, proftime_T *tma)
{
    proftime_T tm3 = prof_wait_time;

    profile_sub(&tm3, tm);
    profile_sub(tma, &tm3);
}

/*
 * Return TRUE if "tm1" and "tm2" are equal.
 */
    int
profile_equal(proftime_T *tm1, proftime_T *tm2)
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
profile_cmp(const proftime_T *tm1, const proftime_T *tm2)
{
# ifdef WIN3264
    return (int)(tm2->QuadPart - tm1->QuadPart);
# else
    if (tm1->tv_sec == tm2->tv_sec)
	return tm2->tv_usec - tm1->tv_usec;
    return tm2->tv_sec - tm1->tv_sec;
# endif
}

static char_u	*profile_fname = NULL;
static proftime_T pause_time;

/*
 * ":profile cmd args"
 */
    void
ex_profile(exarg_T *eap)
{
    char_u	*e;
    int		len;

    e = skiptowhite(eap->arg);
    len = (int)(e - eap->arg);
    e = skipwhite(e);

    if (len == 5 && STRNCMP(eap->arg, "start", 5) == 0 && *e != NUL)
    {
	vim_free(profile_fname);
	profile_fname = expand_env_save_opt(e, TRUE);
	do_profiling = PROF_YES;
	profile_zero(&prof_wait_time);
	set_vim_var_nr(VV_PROFILING, 1L);
    }
    else if (do_profiling == PROF_NONE)
	EMSG(_("E750: First use \":profile start {fname}\""));
    else if (STRCMP(eap->arg, "pause") == 0)
    {
	if (do_profiling == PROF_YES)
	    profile_start(&pause_time);
	do_profiling = PROF_PAUSED;
    }
    else if (STRCMP(eap->arg, "continue") == 0)
    {
	if (do_profiling == PROF_PAUSED)
	{
	    profile_end(&pause_time);
	    profile_add(&prof_wait_time, &pause_time);
	}
	do_profiling = PROF_YES;
    }
    else
    {
	/* The rest is similar to ":breakadd". */
	ex_breakadd(eap);
    }
}

/* Command line expansion for :profile. */
static enum
{
    PEXP_SUBCMD,	/* expand :profile sub-commands */
    PEXP_FUNC		/* expand :profile func {funcname} */
} pexpand_what;

static char *pexpand_cmds[] = {
			"start",
#define PROFCMD_START	0
			"pause",
#define PROFCMD_PAUSE	1
			"continue",
#define PROFCMD_CONTINUE 2
			"func",
#define PROFCMD_FUNC	3
			"file",
#define PROFCMD_FILE	4
			NULL
#define PROFCMD_LAST	5
};

/*
 * Function given to ExpandGeneric() to obtain the profile command
 * specific expansion.
 */
    char_u *
get_profile_name(expand_T *xp UNUSED, int idx)
{
    switch (pexpand_what)
    {
    case PEXP_SUBCMD:
	return (char_u *)pexpand_cmds[idx];
    /* case PEXP_FUNC: TODO */
    default:
	return NULL;
    }
}

/*
 * Handle command line completion for :profile command.
 */
    void
set_context_in_profile_cmd(expand_T *xp, char_u *arg)
{
    char_u	*end_subcmd;

    /* Default: expand subcommands. */
    xp->xp_context = EXPAND_PROFILE;
    pexpand_what = PEXP_SUBCMD;
    xp->xp_pattern = arg;

    end_subcmd = skiptowhite(arg);
    if (*end_subcmd == NUL)
	return;

    if (end_subcmd - arg == 5 && STRNCMP(arg, "start", 5) == 0)
    {
	xp->xp_context = EXPAND_FILES;
	xp->xp_pattern = skipwhite(end_subcmd);
	return;
    }

    /* TODO: expand function names after "func" */
    xp->xp_context = EXPAND_NOTHING;
}

/*
 * Dump the profiling info.
 */
    void
profile_dump(void)
{
    FILE	*fd;

    if (profile_fname != NULL)
    {
	fd = mch_fopen((char *)profile_fname, "w");
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
script_do_profile(scriptitem_T *si)
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
 * Save time when starting to invoke another script or function.
 */
    void
script_prof_save(
    proftime_T	*tm)	    /* place to store wait time */
{
    scriptitem_T    *si;

    if (current_sctx.sc_sid > 0 && current_sctx.sc_sid <= script_items.ga_len)
    {
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
	if (si->sn_prof_on && si->sn_pr_nest++ == 0)
	    profile_start(&si->sn_pr_child);
    }
    profile_get_wait(tm);
}

/*
 * Count time spent in children after invoking another script or function.
 */
    void
script_prof_restore(proftime_T *tm)
{
    scriptitem_T    *si;

    if (current_sctx.sc_sid > 0 && current_sctx.sc_sid <= script_items.ga_len)
    {
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
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
prof_inchar_enter(void)
{
    profile_start(&inchar_time);
}

/*
 * Called when finished waiting for the user to type a character.
 */
    void
prof_inchar_exit(void)
{
    profile_end(&inchar_time);
    profile_add(&prof_wait_time, &inchar_time);
}

/*
 * Dump the profiling results for all scripts in file "fd".
 */
    static void
script_dump_profile(FILE *fd)
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

	    sfd = mch_fopen((char *)si->sn_name, "r");
	    if (sfd == NULL)
		fprintf(fd, "Cannot open file!\n");
	    else
	    {
		/* Keep going till the end of file, so that trailing
		 * continuation lines are listed. */
		for (i = 0; ; ++i)
		{
		    if (vim_fgets(IObuff, IOSIZE, sfd))
			break;
		    /* When a line has been truncated, append NL, taking care
		     * of multi-byte characters . */
		    if (IObuff[IOSIZE - 2] != NUL && IObuff[IOSIZE - 2] != NL)
		    {
			int n = IOSIZE - 2;
# ifdef FEAT_MBYTE
			if (enc_utf8)
			{
			    /* Move to the first byte of this char.
			     * utf_head_off() doesn't work, because it checks
			     * for a truncated character. */
			    while (n > 0 && (IObuff[n] & 0xc0) == 0x80)
				--n;
			}
			else if (has_mbyte)
			    n -= mb_head_off(IObuff, IObuff + n);
# endif
			IObuff[n] = NL;
			IObuff[n + 1] = NUL;
		    }
		    if (i < si->sn_prl_ga.ga_len
				     && (pp = &PRL_ITEM(si, i))->snp_count > 0)
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
prof_def_func(void)
{
    if (current_sctx.sc_sid > 0)
	return SCRIPT_ITEM(current_sctx.sc_sid).sn_pr_force;
    return FALSE;
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
autowrite(buf_T *buf, int forceit)
{
    int		r;
    bufref_T	bufref;

    if (!(p_aw || p_awa) || !p_write
#ifdef FEAT_QUICKFIX
	    /* never autowrite a "nofile" or "nowrite" buffer */
	    || bt_dontwrite(buf)
#endif
	    || (!forceit && buf->b_p_ro) || buf->b_ffname == NULL)
	return FAIL;
    set_bufref(&bufref, buf);
    r = buf_write_all(buf, forceit);

    /* Writing may succeed but the buffer still changed, e.g., when there is a
     * conversion error.  We do want to return FAIL then. */
    if (bufref_valid(&bufref) && bufIsChanged(buf))
	r = FAIL;
    return r;
}

/*
 * Flush all buffers, except the ones that are readonly or are never written.
 */
    void
autowrite_all(void)
{
    buf_T	*buf;

    if (!(p_aw || p_awa) || !p_write)
	return;
    FOR_ALL_BUFFERS(buf)
	if (bufIsChanged(buf) && !buf->b_p_ro && !bt_dontwrite(buf))
	{
	    bufref_T	bufref;

	    set_bufref(&bufref, buf);

	    (void)buf_write_all(buf, FALSE);

	    /* an autocommand may have deleted the buffer */
	    if (!bufref_valid(&bufref))
		buf = firstbuf;
	}
}

/*
 * Return TRUE if buffer was changed and cannot be abandoned.
 * For flags use the CCGD_ values.
 */
    int
check_changed(buf_T *buf, int flags)
{
    int		forceit = (flags & CCGD_FORCEIT);
    bufref_T	bufref;

    set_bufref(&bufref, buf);

    if (       !forceit
	    && bufIsChanged(buf)
	    && ((flags & CCGD_MULTWIN) || buf->b_nwindows <= 1)
	    && (!(flags & CCGD_AW) || autowrite(buf, forceit) == FAIL))
    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	if ((p_confirm || cmdmod.confirm) && p_write)
	{
	    buf_T	*buf2;
	    int		count = 0;

	    if (flags & CCGD_ALLBUF)
		FOR_ALL_BUFFERS(buf2)
		    if (bufIsChanged(buf2)
				     && (buf2->b_ffname != NULL
# ifdef FEAT_BROWSE
					 || cmdmod.browse
# endif
					))
			++count;
	    if (!bufref_valid(&bufref))
		/* Autocommand deleted buffer, oops!  It's not changed now. */
		return FALSE;

	    dialog_changed(buf, count > 1);

	    if (!bufref_valid(&bufref))
		/* Autocommand deleted buffer, oops!  It's not changed now. */
		return FALSE;
	    return bufIsChanged(buf);
	}
#endif
	if (flags & CCGD_EXCMD)
	    no_write_message();
	else
	    no_write_message_nobang(curbuf);
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
browse_save_fname(buf_T *buf)
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
 * Ask the user what to do when abandoning a changed buffer.
 * Must check 'write' option first!
 */
    void
dialog_changed(
    buf_T	*buf,
    int		checkall)	/* may abandon all changed buffers */
{
    char_u	buff[DIALOG_MSG_SIZE];
    int		ret;
    buf_T	*buf2;
    exarg_T     ea;

    dialog_msg(buff, _("Save changes to \"%s\"?"), buf->b_fname);
    if (checkall)
	ret = vim_dialog_yesnoallcancel(VIM_QUESTION, NULL, buff, 1);
    else
	ret = vim_dialog_yesnocancel(VIM_QUESTION, NULL, buff, 1);

    /* Init ea pseudo-structure, this is needed for the check_overwrite()
     * function. */
    ea.append = ea.forceit = FALSE;

    if (ret == VIM_YES)
    {
#ifdef FEAT_BROWSE
	/* May get file name, when there is none */
	browse_save_fname(buf);
#endif
	if (buf->b_fname != NULL && check_overwrite(&ea, buf,
				    buf->b_fname, buf->b_ffname, FALSE) == OK)
	    /* didn't hit Cancel */
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
	FOR_ALL_BUFFERS(buf2)
	{
	    if (bufIsChanged(buf2)
		    && (buf2->b_ffname != NULL
#ifdef FEAT_BROWSE
			|| cmdmod.browse
#endif
			)
		    && !buf2->b_p_ro)
	    {
		bufref_T bufref;

		set_bufref(&bufref, buf2);
#ifdef FEAT_BROWSE
		/* May get file name, when there is none */
		browse_save_fname(buf2);
#endif
		if (buf2->b_fname != NULL && check_overwrite(&ea, buf2,
				  buf2->b_fname, buf2->b_ffname, FALSE) == OK)
		    /* didn't hit Cancel */
		    (void)buf_write_all(buf2, FALSE);

		/* an autocommand may have deleted the buffer */
		if (!bufref_valid(&bufref))
		    buf2 = firstbuf;
	    }
	}
    }
    else if (ret == VIM_DISCARDALL)
    {
	/*
	 * mark all buffers as unchanged
	 */
	FOR_ALL_BUFFERS(buf2)
	    unchanged(buf2, TRUE);
    }
}
#endif

/*
 * Return TRUE if the buffer "buf" can be abandoned, either by making it
 * hidden, autowriting it or unloading it.
 */
    int
can_abandon(buf_T *buf, int forceit)
{
    return (	   buf_hide(buf)
		|| !bufIsChanged(buf)
		|| buf->b_nwindows > 1
		|| autowrite(buf, forceit) == OK
		|| forceit);
}

static void add_bufnum(int *bufnrs, int *bufnump, int nr);

/*
 * Add a buffer number to "bufnrs", unless it's already there.
 */
    static void
add_bufnum(int *bufnrs, int *bufnump, int nr)
{
    int i;

    for (i = 0; i < *bufnump; ++i)
	if (bufnrs[i] == nr)
	    return;
    bufnrs[*bufnump] = nr;
    *bufnump = *bufnump + 1;
}

/*
 * Return TRUE if any buffer was changed and cannot be abandoned.
 * That changed buffer becomes the current buffer.
 * When "unload" is TRUE the current buffer is unloaded instead of making it
 * hidden.  This is used for ":q!".
 */
    int
check_changed_any(
    int		hidden,		/* Only check hidden buffers */
    int		unload)
{
    int		ret = FALSE;
    buf_T	*buf;
    int		save;
    int		i;
    int		bufnum = 0;
    int		bufcount = 0;
    int		*bufnrs;
    tabpage_T   *tp;
    win_T	*wp;

    /* Make a list of all buffers, with the most important ones first. */
    FOR_ALL_BUFFERS(buf)
	++bufcount;

    if (bufcount == 0)
	return FALSE;

    bufnrs = (int *)alloc(sizeof(int) * bufcount);
    if (bufnrs == NULL)
	return FALSE;

    /* curbuf */
    bufnrs[bufnum++] = curbuf->b_fnum;

    /* buffers in current tab */
    FOR_ALL_WINDOWS(wp)
	if (wp->w_buffer != curbuf)
	    add_bufnum(bufnrs, &bufnum, wp->w_buffer->b_fnum);

    /* buffers in other tabs */
    FOR_ALL_TABPAGES(tp)
	if (tp != curtab)
	    for (wp = tp->tp_firstwin; wp != NULL; wp = wp->w_next)
		add_bufnum(bufnrs, &bufnum, wp->w_buffer->b_fnum);

    /* any other buffer */
    FOR_ALL_BUFFERS(buf)
	add_bufnum(bufnrs, &bufnum, buf->b_fnum);

    for (i = 0; i < bufnum; ++i)
    {
	buf = buflist_findnr(bufnrs[i]);
	if (buf == NULL)
	    continue;
	if ((!hidden || buf->b_nwindows == 0) && bufIsChanged(buf))
	{
	    bufref_T bufref;

	    set_bufref(&bufref, buf);
#ifdef FEAT_TERMINAL
	    if (term_job_running(buf->b_term))
	    {
		if (term_try_stop_job(buf) == FAIL)
		    break;
	    }
	    else
#endif
	    /* Try auto-writing the buffer.  If this fails but the buffer no
	    * longer exists it's not changed, that's OK. */
	    if (check_changed(buf, (p_awa ? CCGD_AW : 0)
				 | CCGD_MULTWIN
				 | CCGD_ALLBUF) && bufref_valid(&bufref))
		break;	    /* didn't save - still changes */
	}
    }

    if (i >= bufnum)
	goto theend;

    /* Get here if "buf" cannot be abandoned. */
    ret = TRUE;
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
	if (vgetc_busy > 0)
	{
	    msg_row = cmdline_row;
	    msg_col = 0;
	    msg_didout = FALSE;
	}
	if (
#ifdef FEAT_TERMINAL
		term_job_running(buf->b_term)
		    ? EMSG2(_("E947: Job still running in buffer \"%s\""),
								  buf->b_fname)
		    :
#endif
		EMSG2(_("E162: No write since last change for buffer \"%s\""),
		    buf_spname(buf) != NULL ? buf_spname(buf) : buf->b_fname))
	{
	    save = no_wait_return;
	    no_wait_return = FALSE;
	    wait_return(FALSE);
	    no_wait_return = save;
	}
    }

    /* Try to find a window that contains the buffer. */
    if (buf != curbuf)
	FOR_ALL_TAB_WINDOWS(tp, wp)
	    if (wp->w_buffer == buf)
	    {
		bufref_T bufref;

		set_bufref(&bufref, buf);

		goto_tabpage_win(tp, wp);

		/* Paranoia: did autocms wipe out the buffer with changes? */
		if (!bufref_valid(&bufref))
		    goto theend;
		goto buf_found;
	    }
buf_found:

    /* Open the changed buffer in the current window. */
    if (buf != curbuf)
	set_curbuf(buf, unload ? DOBUF_UNLOAD : DOBUF_GOTO);

theend:
    vim_free(bufnrs);
    return ret;
}

/*
 * return FAIL if there is no file name, OK if there is one
 * give error message for FAIL
 */
    int
check_fname(void)
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
buf_write_all(buf_T *buf, int forceit)
{
    int	    retval;
    buf_T	*old_curbuf = curbuf;

    retval = (buf_write(buf, buf->b_ffname, buf->b_fname,
				   (linenr_T)1, buf->b_ml.ml_line_count, NULL,
						  FALSE, forceit, TRUE, FALSE));
    if (curbuf != old_curbuf)
    {
	msg_source(HL_ATTR(HLF_W));
	MSG(_("Warning: Entered other buffer unexpectedly (check autocommands)"));
    }
    return retval;
}

/*
 * Code to handle the argument list.
 */

static char_u	*do_one_arg(char_u *str);
static int	do_arglist(char_u *str, int what, int after, int will_edit);
static void	alist_check_arg_idx(void);
static int	editing_arg_idx(win_T *win);
static void	alist_add_list(int count, char_u **files, int after, int will_edit);
#define AL_SET	1
#define AL_ADD	2
#define AL_DEL	3

/*
 * Isolate one argument, taking backticks.
 * Changes the argument in-place, puts a NUL after it.  Backticks remain.
 * Return a pointer to the start of the next argument.
 */
    static char_u *
do_one_arg(char_u *str)
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
    static int
get_arglist(garray_T *gap, char_u *str, int escaped)
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

	/* If str is escaped, don't handle backslashes or spaces */
	if (!escaped)
	    return OK;

	/* Isolate one argument, change it in-place, put a NUL after it. */
	str = do_one_arg(str);
    }
    return OK;
}

#if defined(FEAT_QUICKFIX) || defined(FEAT_SYN_HL) || defined(PROTO)
/*
 * Parse a list of arguments (file names), expand them and return in
 * "fnames[fcountp]".  When "wig" is TRUE, removes files matching 'wildignore'.
 * Return FAIL or OK.
 */
    int
get_arglist_exp(
    char_u	*str,
    int		*fcountp,
    char_u	***fnamesp,
    int		wig)
{
    garray_T	ga;
    int		i;

    if (get_arglist(&ga, str, TRUE) == FAIL)
	return FAIL;
    if (wig == TRUE)
	i = expand_wildcards(ga.ga_len, (char_u **)ga.ga_data,
					fcountp, fnamesp, EW_FILE|EW_NOTFOUND);
    else
	i = gen_expand_wildcards(ga.ga_len, (char_u **)ga.ga_data,
					fcountp, fnamesp, EW_FILE|EW_NOTFOUND);

    ga_clear(&ga);
    return i;
}
#endif

/*
 * Redefine the argument list.
 */
    void
set_arglist(char_u *str)
{
    do_arglist(str, AL_SET, 0, FALSE);
}

/*
 * "what" == AL_SET: Redefine the argument list to 'str'.
 * "what" == AL_ADD: add files in 'str' to the argument list after "after".
 * "what" == AL_DEL: remove files in 'str' from the argument list.
 *
 * Return FAIL for failure, OK otherwise.
 */
    static int
do_arglist(
    char_u	*str,
    int		what,
    int		after UNUSED,	// 0 means before first one
    int		will_edit)	// will edit added argument
{
    garray_T	new_ga;
    int		exp_count;
    char_u	**exp_files;
    int		i;
    char_u	*p;
    int		match;
    int		arg_escaped = TRUE;

    /*
     * Set default argument for ":argadd" command.
     */
    if (what == AL_ADD && *str == NUL)
    {
	if (curbuf->b_ffname == NULL)
	    return FAIL;
	str = curbuf->b_fname;
	arg_escaped = FALSE;
    }

    /*
     * Collect all file name arguments in "new_ga".
     */
    if (get_arglist(&new_ga, str, arg_escaped) == FAIL)
	return FAIL;

    if (what == AL_DEL)
    {
	regmatch_T	regmatch;
	int		didone;

	/*
	 * Delete the items: use each item as a regexp and find a match in the
	 * argument list.
	 */
	regmatch.rm_ic = p_fic;	/* ignore case when 'fileignorecase' is set */
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

	    vim_regfree(regmatch.regprog);
	    vim_free(p);
	    if (!didone)
		EMSG2(_(e_nomatch2), ((char_u **)new_ga.ga_data)[i]);
	}
	ga_clear(&new_ga);
    }
    else
    {
	i = expand_wildcards(new_ga.ga_len, (char_u **)new_ga.ga_data,
		&exp_count, &exp_files, EW_DIR|EW_FILE|EW_ADDSLASH|EW_NOTFOUND);
	ga_clear(&new_ga);
	if (i == FAIL || exp_count == 0)
	{
	    EMSG(_(e_nomatch));
	    return FAIL;
	}

	if (what == AL_ADD)
	{
	    alist_add_list(exp_count, exp_files, after, will_edit);
	    vim_free(exp_files);
	}
	else /* what == AL_SET */
	    alist_set(ALIST(curwin), exp_count, exp_files, will_edit, NULL, 0);
    }

    alist_check_arg_idx();

    return OK;
}

/*
 * Check the validity of the arg_idx for each other window.
 */
    static void
alist_check_arg_idx(void)
{
    win_T	*win;
    tabpage_T	*tp;

    FOR_ALL_TAB_WINDOWS(tp, win)
	if (win->w_alist == curwin->w_alist)
	    check_arg_idx(win);
}

/*
 * Return TRUE if window "win" is editing the file at the current argument
 * index.
 */
    static int
editing_arg_idx(win_T *win)
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
check_arg_idx(win_T *win)
{
    if (WARGCOUNT(win) > 1 && !editing_arg_idx(win))
    {
	/* We are not editing the current entry in the argument list.
	 * Set "arg_had_last" if we are editing the last one. */
	win->w_arg_idx_invalid = TRUE;
	if (win->w_arg_idx != WARGCOUNT(win) - 1
		&& arg_had_last == FALSE
		&& ALIST(win) == &global_alist
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
					      && win->w_alist == &global_alist)
	    arg_had_last = TRUE;
    }
}

/*
 * ":args", ":argslocal" and ":argsglobal".
 */
    void
ex_args(exarg_T *eap)
{
    int		i;

    if (eap->cmdidx != CMD_args)
    {
	alist_unlink(ALIST(curwin));
	if (eap->cmdidx == CMD_argglobal)
	    ALIST(curwin) = &global_alist;
	else /* eap->cmdidx == CMD_arglocal */
	    alist_new();
    }

    if (!ends_excmd(*eap->arg))
    {
	/*
	 * ":args file ..": define new argument list, handle like ":next"
	 * Also for ":argslocal file .." and ":argsglobal file ..".
	 */
	ex_next(eap);
    }
    else if (eap->cmdidx == CMD_args)
    {
	/*
	 * ":args": list arguments.
	 */
	if (ARGCOUNT > 0)
	{
	    char_u **items = (char_u **)alloc(sizeof(char_u *) * ARGCOUNT);

	    if (items != NULL)
	    {
		/* Overwrite the command, for a short list there is no
		 * scrolling required and no wait_return(). */
		gotocmdline(TRUE);

		for (i = 0; i < ARGCOUNT; ++i)
		    items[i] = alist_name(&ARGLIST[i]);
		list_in_columns(items, ARGCOUNT, curwin->w_arg_idx);
		vim_free(items);
	    }
	}
    }
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
}

/*
 * ":previous", ":sprevious", ":Next" and ":sNext".
 */
    void
ex_previous(exarg_T *eap)
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
ex_rewind(exarg_T *eap)
{
    do_argfile(eap, 0);
}

/*
 * ":last" and ":slast".
 */
    void
ex_last(exarg_T *eap)
{
    do_argfile(eap, ARGCOUNT - 1);
}

/*
 * ":argument" and ":sargument".
 */
    void
ex_argument(exarg_T *eap)
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
do_argfile(exarg_T *eap, int argn)
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

	/* split window or create new tab page first */
	if (*eap->cmd == 's' || cmdmod.tab != 0)
	{
	    if (win_split(0, 0) == FAIL)
		return;
	    RESET_BINDING(curwin);
	}
	else
	{
	    /*
	     * if 'hidden' set, only check for changed file when re-editing
	     * the same buffer
	     */
	    other = TRUE;
	    if (buf_hide(curbuf))
	    {
		p = fix_fname(alist_name(&ARGLIST[argn]));
		other = otherfile(p);
		vim_free(p);
	    }
	    if ((!buf_hide(curbuf) || !other)
		  && check_changed(curbuf, CCGD_AW
					 | (other ? 0 : CCGD_MULTWIN)
					 | (eap->forceit ? CCGD_FORCEIT : 0)
					 | CCGD_EXCMD))
		return;
	}

	curwin->w_arg_idx = argn;
	if (argn == ARGCOUNT - 1 && curwin->w_alist == &global_alist)
	    arg_had_last = TRUE;

	/* Edit the file; always use the last known line number.
	 * When it fails (e.g. Abort for already edited file) restore the
	 * argument index. */
	if (do_ecmd(0, alist_name(&ARGLIST[curwin->w_arg_idx]), NULL,
		      eap, ECMD_LAST,
		      (buf_hide(curwin->w_buffer) ? ECMD_HIDE : 0)
			 + (eap->forceit ? ECMD_FORCEIT : 0), curwin) == FAIL)
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
ex_next(exarg_T *eap)
{
    int		i;

    /*
     * check for changed buffer now, if this fails the argument list is not
     * redefined.
     */
    if (       buf_hide(curbuf)
	    || eap->cmdidx == CMD_snext
	    || !check_changed(curbuf, CCGD_AW
				    | (eap->forceit ? CCGD_FORCEIT : 0)
				    | CCGD_EXCMD))
    {
	if (*eap->arg != NUL)		    /* redefine file list */
	{
	    if (do_arglist(eap->arg, AL_SET, 0, TRUE) == FAIL)
		return;
	    i = 0;
	}
	else
	    i = curwin->w_arg_idx + (int)eap->line2;
	do_argfile(eap, i);
    }
}

/*
 * ":argedit"
 */
    void
ex_argedit(exarg_T *eap)
{
    int i = eap->addr_count ? (int)eap->line2 : curwin->w_arg_idx + 1;
    // Whether curbuf will be reused, curbuf->b_ffname will be set.
    int curbuf_is_reusable = curbuf_reusable();

    if (do_arglist(eap->arg, AL_ADD, i, TRUE) == FAIL)
	return;
#ifdef FEAT_TITLE
    maketitle();
#endif

    if (curwin->w_arg_idx == 0
	    && (curbuf->b_ml.ml_flags & ML_EMPTY)
	    && (curbuf->b_ffname == NULL || curbuf_is_reusable))
	i = 0;
    /* Edit the argument. */
    if (i < ARGCOUNT)
	do_argfile(eap, i);
}

/*
 * ":argadd"
 */
    void
ex_argadd(exarg_T *eap)
{
    do_arglist(eap->arg, AL_ADD,
	       eap->addr_count > 0 ? (int)eap->line2 : curwin->w_arg_idx + 1,
	       FALSE);
#ifdef FEAT_TITLE
    maketitle();
#endif
}

/*
 * ":argdelete"
 */
    void
ex_argdelete(exarg_T *eap)
{
    int		i;
    int		n;

    if (eap->addr_count > 0)
    {
	/* ":1,4argdel": Delete all arguments in the range. */
	if (eap->line2 > ARGCOUNT)
	    eap->line2 = ARGCOUNT;
	n = eap->line2 - eap->line1 + 1;
	if (*eap->arg != NUL)
	    /* Can't have both a range and an argument. */
	    EMSG(_(e_invarg));
	else if (n <= 0)
	{
	    /* Don't give an error for ":%argdel" if the list is empty. */
	    if (eap->line1 != 1 || eap->line2 != 0)
		EMSG(_(e_invrange));
	}
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
	    if (ARGCOUNT == 0)
		curwin->w_arg_idx = 0;
	    else if (curwin->w_arg_idx >= ARGCOUNT)
		curwin->w_arg_idx = ARGCOUNT - 1;
	}
    }
    else if (*eap->arg == NUL)
	EMSG(_(e_argreq));
    else
	do_arglist(eap->arg, AL_DEL, 0, FALSE);
#ifdef FEAT_TITLE
    maketitle();
#endif
}

/*
 * ":argdo", ":windo", ":bufdo", ":tabdo", ":cdo", ":ldo", ":cfdo" and ":lfdo"
 */
    void
ex_listdo(exarg_T *eap)
{
    int		i;
    win_T	*wp;
    tabpage_T	*tp;
    buf_T	*buf = curbuf;
    int		next_fnum = 0;
#if defined(FEAT_SYN_HL)
    char_u	*save_ei = NULL;
#endif
    char_u	*p_shm_save;
#ifdef FEAT_QUICKFIX
    int		qf_size = 0;
    int		qf_idx;
#endif

#ifndef FEAT_QUICKFIX
    if (eap->cmdidx == CMD_cdo || eap->cmdidx == CMD_ldo ||
	    eap->cmdidx == CMD_cfdo || eap->cmdidx == CMD_lfdo)
    {
	ex_ni(eap);
	return;
    }
#endif

#if defined(FEAT_SYN_HL)
    if (eap->cmdidx != CMD_windo && eap->cmdidx != CMD_tabdo)
	/* Don't do syntax HL autocommands.  Skipping the syntax file is a
	 * great speed improvement. */
	save_ei = au_event_disable(",Syntax");
#endif
#ifdef FEAT_CLIPBOARD
    start_global_changes();
#endif

    if (eap->cmdidx == CMD_windo
	    || eap->cmdidx == CMD_tabdo
	    || buf_hide(curbuf)
	    || !check_changed(curbuf, CCGD_AW
				    | (eap->forceit ? CCGD_FORCEIT : 0)
				    | CCGD_EXCMD))
    {
	i = 0;
	/* start at the eap->line1 argument/window/buffer */
	wp = firstwin;
	tp = first_tabpage;
	switch (eap->cmdidx)
	{
	    case CMD_windo:
		for ( ; wp != NULL && i + 1 < eap->line1; wp = wp->w_next)
		    i++;
		break;
	    case CMD_tabdo:
		for( ; tp != NULL && i + 1 < eap->line1; tp = tp->tp_next)
		    i++;
		break;
	    case CMD_argdo:
		i = eap->line1 - 1;
		break;
	    default:
		break;
	}
	/* set pcmark now */
	if (eap->cmdidx == CMD_bufdo)
	{
	    /* Advance to the first listed buffer after "eap->line1". */
	    for (buf = firstbuf; buf != NULL && (buf->b_fnum < eap->line1
					  || !buf->b_p_bl); buf = buf->b_next)
		if (buf->b_fnum > eap->line2)
		{
		    buf = NULL;
		    break;
		}
	    if (buf != NULL)
		goto_buffer(eap, DOBUF_FIRST, FORWARD, buf->b_fnum);
	}
#ifdef FEAT_QUICKFIX
	else if (eap->cmdidx == CMD_cdo || eap->cmdidx == CMD_ldo
		|| eap->cmdidx == CMD_cfdo || eap->cmdidx == CMD_lfdo)
	{
	    qf_size = qf_get_size(eap);
	    if (qf_size <= 0 || eap->line1 > qf_size)
		buf = NULL;
	    else
	    {
		ex_cc(eap);

		buf = curbuf;
		i = eap->line1 - 1;
		if (eap->addr_count <= 0)
		    /* default is all the quickfix/location list entries */
		    eap->line2 = qf_size;
	    }
	}
#endif
	else
	    setpcmark();
	listcmd_busy = TRUE;	    /* avoids setting pcmark below */

	while (!got_int && buf != NULL)
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
	    }
	    else if (eap->cmdidx == CMD_windo)
	    {
		/* go to window "wp" */
		if (!win_valid(wp))
		    break;
		win_goto(wp);
		if (curwin != wp)
		    break;  /* something must be wrong */
		wp = curwin->w_next;
	    }
	    else if (eap->cmdidx == CMD_tabdo)
	    {
		/* go to window "tp" */
		if (!valid_tabpage(tp))
		    break;
		goto_tabpage_tp(tp, TRUE, TRUE);
		tp = tp->tp_next;
	    }
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

	    ++i;

	    /* execute the command */
	    do_cmdline(eap->arg, eap->getline, eap->cookie,
						DOCMD_VERBOSE + DOCMD_NOWAIT);

	    if (eap->cmdidx == CMD_bufdo)
	    {
		/* Done? */
		if (next_fnum < 0 || next_fnum > eap->line2)
		    break;
		/* Check if the buffer still exists. */
		FOR_ALL_BUFFERS(buf)
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

		/* If autocommands took us elsewhere, quit here. */
		if (curbuf->b_fnum != next_fnum)
		    break;
	    }

#ifdef FEAT_QUICKFIX
	    if (eap->cmdidx == CMD_cdo || eap->cmdidx == CMD_ldo
		    || eap->cmdidx == CMD_cfdo || eap->cmdidx == CMD_lfdo)
	    {
		if (i >= qf_size || i >= eap->line2)
		    break;

		qf_idx = qf_get_cur_idx(eap);

		ex_cnext(eap);

		/* If jumping to the next quickfix entry fails, quit here */
		if (qf_get_cur_idx(eap) == qf_idx)
		    break;
	    }
#endif

	    if (eap->cmdidx == CMD_windo)
	    {
		validate_cursor();	/* cursor may have moved */

		/* required when 'scrollbind' has been set */
		if (curwin->w_p_scb)
		    do_check_scrollbind(TRUE);
	    }

	    if (eap->cmdidx == CMD_windo || eap->cmdidx == CMD_tabdo)
		if (i+1 > eap->line2)
		    break;
	    if (eap->cmdidx == CMD_argdo && i >= eap->line2)
		break;
	}
	listcmd_busy = FALSE;
    }

#if defined(FEAT_SYN_HL)
    if (save_ei != NULL)
    {
	au_event_restore(save_ei);
	apply_autocmds(EVENT_SYNTAX, curbuf->b_p_syn,
					       curbuf->b_fname, TRUE, curbuf);
    }
#endif
#ifdef FEAT_CLIPBOARD
    end_global_changes();
#endif
}

/*
 * Add files[count] to the arglist of the current window after arg "after".
 * The file names in files[count] must have been allocated and are taken over.
 * Files[] itself is not taken over.
 */
    static void
alist_add_list(
    int		count,
    char_u	**files,
    int		after,	    // where to add: 0 = before first one
    int		will_edit)  // will edit adding argument
{
    int		i;
    int		old_argcount = ARGCOUNT;

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
	    int flags = BLN_LISTED | (will_edit ? BLN_CURBUF : 0);

	    ARGLIST[after + i].ae_fname = files[i];
	    ARGLIST[after + i].ae_fnum = buflist_add(files[i], flags);
	}
	ALIST(curwin)->al_ga.ga_len += count;
	if (old_argcount > 0 && curwin->w_arg_idx >= after)
	    curwin->w_arg_idx += count;
	return;
    }

    for (i = 0; i < count; ++i)
	vim_free(files[i]);
}

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)
/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * argedit and argdelete commands.
 */
    char_u *
get_arglist_name(expand_T *xp UNUSED, int idx)
{
    if (idx >= ARGCOUNT)
	return NULL;

    return alist_name(&ARGLIST[idx]);
}
#endif


#ifdef FEAT_EVAL
/*
 * ":compiler[!] {name}"
 */
    void
ex_compiler(exarg_T *eap)
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
		 * "b:current_compiler" and restore "current_compiler".
		 * Explicitly prepend "g:" to make it work in a function. */
		old_cur_comp = get_var_value((char_u *)"g:current_compiler");
		if (old_cur_comp != NULL)
		    old_cur_comp = vim_strsave(old_cur_comp);
		do_cmdline_cmd((char_u *)
			      "command -nargs=* CompilerSet setlocal <args>");
	    }
	    do_unlet((char_u *)"g:current_compiler", TRUE);
	    do_unlet((char_u *)"b:current_compiler", TRUE);

	    sprintf((char *)buf, "compiler/%s.vim", eap->arg);
	    if (source_runtime(buf, DIP_ALL) == FAIL)
		EMSG2(_("E666: compiler not supported: %s"), eap->arg);
	    vim_free(buf);

	    do_cmdline_cmd((char_u *)":delcommand CompilerSet");

	    /* Set "b:current_compiler" from "current_compiler". */
	    p = get_var_value((char_u *)"g:current_compiler");
	    if (p != NULL)
		set_internal_string_var((char_u *)"b:current_compiler", p);

	    /* Restore "current_compiler" for ":compiler {name}". */
	    if (!eap->forceit)
	    {
		if (old_cur_comp != NULL)
		{
		    set_internal_string_var((char_u *)"g:current_compiler",
								old_cur_comp);
		    vim_free(old_cur_comp);
		}
		else
		    do_unlet((char_u *)"g:current_compiler", TRUE);
	    }
	}
    }
}
#endif

/*
 * ":runtime [what] {name}"
 */
    void
ex_runtime(exarg_T *eap)
{
    char_u  *arg = eap->arg;
    char_u  *p = skiptowhite(arg);
    int	    len = (int)(p - arg);
    int	    flags = eap->forceit ? DIP_ALL : 0;

    if (STRNCMP(arg, "START", len) == 0)
    {
	flags += DIP_START + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "OPT", len) == 0)
    {
	flags += DIP_OPT + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "PACK", len) == 0)
    {
	flags += DIP_START + DIP_OPT + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "ALL", len) == 0)
    {
	flags += DIP_START + DIP_OPT;
	arg = skipwhite(arg + len);
    }

    source_runtime(arg, flags);
}

    static void
source_callback(char_u *fname, void *cookie UNUSED)
{
    (void)do_source(fname, FALSE, DOSO_NONE);
}

/*
 * Find the file "name" in all directories in "path" and invoke
 * "callback(fname, cookie)".
 * "name" can contain wildcards.
 * When "flags" has DIP_ALL: source all files, otherwise only the first one.
 * When "flags" has DIP_DIR: find directories instead of files.
 * When "flags" has DIP_ERR: give an error message if there is no match.
 *
 * return FAIL when no file could be sourced, OK otherwise.
 */
    int
do_in_path(
    char_u	*path,
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
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
    rtp_copy = vim_strsave(path);
    buf = alloc(MAXPATHL);
    if (buf != NULL && rtp_copy != NULL)
    {
	if (p_verbose > 1 && name != NULL)
	{
	    verbose_enter();
	    smsg((char_u *)_("Searching for \"%s\" in \"%s\""),
						 (char *)name, (char *)path);
	    verbose_leave();
	}

	/* Loop over all entries in 'runtimepath'. */
	rtp = rtp_copy;
	while (*rtp != NUL && ((flags & DIP_ALL) || !did_one))
	{
	    size_t buflen;

	    /* Copy the path from 'runtimepath' to buf[]. */
	    copy_option_part(&rtp, buf, MAXPATHL, ",");
	    buflen = STRLEN(buf);

	    /* Skip after or non-after directories. */
	    if (flags & (DIP_NOAFTER | DIP_AFTER))
	    {
		int is_after = buflen >= 5
				     && STRCMP(buf + buflen - 5, "after") == 0;

		if ((is_after && (flags & DIP_NOAFTER))
			|| (!is_after && (flags & DIP_AFTER)))
		    continue;
	    }

	    if (name == NULL)
	    {
		(*callback)(buf, (void *) &cookie);
		if (!did_one)
		    did_one = (cookie == NULL);
	    }
	    else if (buflen + STRLEN(name) + 2 < MAXPATHL)
	    {
		add_pathsep(buf);
		tail = buf + STRLEN(buf);

		/* Loop over all patterns in "name" */
		np = name;
		while (*np != NUL && ((flags & DIP_ALL) || !did_one))
		{
		    /* Append the pattern from "name" to buf[]. */
		    copy_option_part(&np, tail, (int)(MAXPATHL - (tail - buf)),
								       "\t ");

		    if (p_verbose > 2)
		    {
			verbose_enter();
			smsg((char_u *)_("Searching for \"%s\""), buf);
			verbose_leave();
		    }

		    /* Expand wildcards, invoke the callback for each match. */
		    if (gen_expand_wildcards(1, &buf, &num_files, &files,
				  (flags & DIP_DIR) ? EW_DIR : EW_FILE) == OK)
		    {
			for (i = 0; i < num_files; ++i)
			{
			    (*callback)(files[i], cookie);
			    did_one = TRUE;
			    if (!(flags & DIP_ALL))
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
    if (!did_one && name != NULL)
    {
	char *basepath = path == p_rtp ? "runtimepath" : "packpath";

	if (flags & DIP_ERR)
	    EMSG3(_(e_dirnotf), basepath, name);
	else if (p_verbose > 0)
	{
	    verbose_enter();
	    smsg((char_u *)_("not found in '%s': \"%s\""), basepath, name);
	    verbose_leave();
	}
    }

#ifdef AMIGA
    proc->pr_WindowPtr = save_winptr;
#endif

    return did_one ? OK : FAIL;
}

/*
 * Find "name" in "path".  When found, invoke the callback function for
 * it: callback(fname, "cookie")
 * When "flags" has DIP_ALL repeat for all matches, otherwise only the first
 * one is used.
 * Returns OK when at least one match found, FAIL otherwise.
 *
 * If "name" is NULL calls callback for each entry in "path". Cookie is
 * passed by reference in this case, setting it to NULL indicates that callback
 * has done its job.
 */
    static int
do_in_path_and_pp(
    char_u	*path,
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    int		done = FAIL;
    char_u	*s;
    int		len;
    char	*start_dir = "pack/*/start/*/%s";
    char	*opt_dir = "pack/*/opt/*/%s";

    if ((flags & DIP_NORTP) == 0)
	done = do_in_path(path, name, flags, callback, cookie);

    if ((done == FAIL || (flags & DIP_ALL)) && (flags & DIP_START))
    {
	len = (int)(STRLEN(start_dir) + STRLEN(name));
	s = alloc(len);
	if (s == NULL)
	    return FAIL;
	vim_snprintf((char *)s, len, start_dir, name);
	done = do_in_path(p_pp, s, flags, callback, cookie);
	vim_free(s);
    }

    if ((done == FAIL || (flags & DIP_ALL)) && (flags & DIP_OPT))
    {
	len = (int)(STRLEN(opt_dir) + STRLEN(name));
	s = alloc(len);
	if (s == NULL)
	    return FAIL;
	vim_snprintf((char *)s, len, opt_dir, name);
	done = do_in_path(p_pp, s, flags, callback, cookie);
	vim_free(s);
    }

    return done;
}

/*
 * Just like do_in_path_and_pp(), using 'runtimepath' for "path".
 */
    int
do_in_runtimepath(
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    return do_in_path_and_pp(p_rtp, name, flags, callback, cookie);
}

/*
 * Source the file "name" from all directories in 'runtimepath'.
 * "name" can contain wildcards.
 * When "flags" has DIP_ALL: source all files, otherwise only the first one.
 *
 * return FAIL when no file could be sourced, OK otherwise.
 */
    int
source_runtime(char_u *name, int flags)
{
    return source_in_path(p_rtp, name, flags);
}

/*
 * Just like source_runtime(), but use "path" instead of 'runtimepath'.
 */
    int
source_in_path(char_u *path, char_u *name, int flags)
{
    return do_in_path_and_pp(path, name, flags, source_callback, NULL);
}


#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Expand wildcards in "pat" and invoke do_source() for each match.
 */
    static void
source_all_matches(char_u *pat)
{
    int	    num_files;
    char_u  **files;
    int	    i;

    if (gen_expand_wildcards(1, &pat, &num_files, &files, EW_FILE) == OK)
    {
	for (i = 0; i < num_files; ++i)
	    (void)do_source(files[i], FALSE, DOSO_NONE);
	FreeWild(num_files, files);
    }
}

/*
 * Add the package directory to 'runtimepath'.
 */
    static int
add_pack_dir_to_rtp(char_u *fname)
{
    char_u  *p4, *p3, *p2, *p1, *p;
    char_u  *entry;
    char_u  *insp = NULL;
    int	    c;
    char_u  *new_rtp;
    int	    keep;
    size_t  oldlen;
    size_t  addlen;
    size_t  new_rtp_len;
    char_u  *afterdir = NULL;
    size_t  afterlen = 0;
    char_u  *after_insp = NULL;
    char_u  *ffname = NULL;
    size_t  fname_len;
    char_u  *buf = NULL;
    char_u  *rtp_ffname;
    int	    match;
    int	    retval = FAIL;

    p4 = p3 = p2 = p1 = get_past_head(fname);
    for (p = p1; *p; MB_PTR_ADV(p))
	if (vim_ispathsep_nocolon(*p))
	{
	    p4 = p3; p3 = p2; p2 = p1; p1 = p;
	}

    /* now we have:
     * rtp/pack/name/start/name
     *    p4   p3   p2    p1
     *
     * find the part up to "pack" in 'runtimepath' */
    c = *++p4; /* append pathsep in order to expand symlink */
    *p4 = NUL;
    ffname = fix_fname(fname);
    *p4 = c;
    if (ffname == NULL)
	return FAIL;

    // Find "ffname" in "p_rtp", ignoring '/' vs '\' differences.
    // Also stop at the first "after" directory.
    fname_len = STRLEN(ffname);
    buf = alloc(MAXPATHL);
    if (buf == NULL)
	goto theend;
    for (entry = p_rtp; *entry != NUL; )
    {
	char_u *cur_entry = entry;

	copy_option_part(&entry, buf, MAXPATHL, ",");
	if (insp == NULL)
	{
	    add_pathsep(buf);
	    rtp_ffname = fix_fname(buf);
	    if (rtp_ffname == NULL)
		goto theend;
	    match = vim_fnamencmp(rtp_ffname, ffname, fname_len) == 0;
	    vim_free(rtp_ffname);
	    if (match)
		// Insert "ffname" after this entry (and comma).
		insp = entry;
	}

	if ((p = (char_u *)strstr((char *)buf, "after")) != NULL
		&& p > buf
		&& vim_ispathsep(p[-1])
		&& (vim_ispathsep(p[5]) || p[5] == NUL || p[5] == ','))
	{
	    if (insp == NULL)
		// Did not find "ffname" before the first "after" directory,
		// insert it before this entry.
		insp = cur_entry;
	    after_insp = cur_entry;
	    break;
	}
    }

    if (insp == NULL)
	// Both "fname" and "after" not found, append at the end.
	insp = p_rtp + STRLEN(p_rtp);

    // check if rtp/pack/name/start/name/after exists
    afterdir = concat_fnames(fname, (char_u *)"after", TRUE);
    if (afterdir != NULL && mch_isdir(afterdir))
	afterlen = STRLEN(afterdir) + 1; // add one for comma

    oldlen = STRLEN(p_rtp);
    addlen = STRLEN(fname) + 1; // add one for comma
    new_rtp = alloc((int)(oldlen + addlen + afterlen + 1)); // add one for NUL
    if (new_rtp == NULL)
	goto theend;

    // We now have 'rtp' parts: {keep}{keep_after}{rest}.
    // Create new_rtp, first: {keep},{fname}
    keep = (int)(insp - p_rtp);
    mch_memmove(new_rtp, p_rtp, keep);
    new_rtp_len = keep;
    if (*insp == NUL)
	new_rtp[new_rtp_len++] = ',';  // add comma before
    mch_memmove(new_rtp + new_rtp_len, fname, addlen - 1);
    new_rtp_len += addlen - 1;
    if (*insp != NUL)
	new_rtp[new_rtp_len++] = ',';  // add comma after

    if (afterlen > 0 && after_insp != NULL)
    {
	int keep_after = (int)(after_insp - p_rtp);

	// Add to new_rtp: {keep},{fname}{keep_after},{afterdir}
	mch_memmove(new_rtp + new_rtp_len, p_rtp + keep,
							keep_after - keep);
	new_rtp_len += keep_after - keep;
	mch_memmove(new_rtp + new_rtp_len, afterdir, afterlen - 1);
	new_rtp_len += afterlen - 1;
	new_rtp[new_rtp_len++] = ',';
	keep = keep_after;
    }

    if (p_rtp[keep] != NUL)
	// Append rest: {keep},{fname}{keep_after},{afterdir}{rest}
	mch_memmove(new_rtp + new_rtp_len, p_rtp + keep, oldlen - keep + 1);
    else
	new_rtp[new_rtp_len] = NUL;

    if (afterlen > 0 && after_insp == NULL)
    {
	// Append afterdir when "after" was not found:
	// {keep},{fname}{rest},{afterdir}
	STRCAT(new_rtp, ",");
	STRCAT(new_rtp, afterdir);
    }

    set_option_value((char_u *)"rtp", 0L, new_rtp, 0);
    vim_free(new_rtp);
    retval = OK;

theend:
    vim_free(buf);
    vim_free(ffname);
    vim_free(afterdir);
    return retval;
}

/*
 * Load scripts in "plugin" and "ftdetect" directories of the package.
 */
    static int
load_pack_plugin(char_u *fname)
{
    static char *plugpat = "%s/plugin/**/*.vim";
    static char *ftpat = "%s/ftdetect/*.vim";
    int		len;
    char_u	*ffname = fix_fname(fname);
    char_u	*pat = NULL;
    int		retval = FAIL;

    if (ffname == NULL)
	return FAIL;
    len = (int)STRLEN(ffname) + (int)STRLEN(ftpat);
    pat = alloc(len);
    if (pat == NULL)
	goto theend;
    vim_snprintf((char *)pat, len, plugpat, ffname);
    source_all_matches(pat);

    {
	char_u *cmd = vim_strsave((char_u *)"g:did_load_filetypes");

	/* If runtime/filetype.vim wasn't loaded yet, the scripts will be
	 * found when it loads. */
	if (cmd != NULL && eval_to_number(cmd) > 0)
	{
	    do_cmdline_cmd((char_u *)"augroup filetypedetect");
	    vim_snprintf((char *)pat, len, ftpat, ffname);
	    source_all_matches(pat);
	    do_cmdline_cmd((char_u *)"augroup END");
	}
	vim_free(cmd);
    }
    vim_free(pat);
    retval = OK;

theend:
    vim_free(ffname);
    return retval;
}

/* used for "cookie" of add_pack_plugin() */
static int APP_ADD_DIR;
static int APP_LOAD;
static int APP_BOTH;

    static void
add_pack_plugin(char_u *fname, void *cookie)
{
    if (cookie != &APP_LOAD)
    {
	char_u	*buf = alloc(MAXPATHL);
	char_u	*p;
	int	found = FALSE;

	if (buf == NULL)
	    return;
	p = p_rtp;
	while (*p != NUL)
	{
	    copy_option_part(&p, buf, MAXPATHL, ",");
	    if (pathcmp((char *)buf, (char *)fname, -1) == 0)
	    {
		found = TRUE;
		break;
	    }
	}
	vim_free(buf);
	if (!found)
	    /* directory is not yet in 'runtimepath', add it */
	    if (add_pack_dir_to_rtp(fname) == FAIL)
		return;
    }

    if (cookie != &APP_ADD_DIR)
	load_pack_plugin(fname);
}

/*
 * Add all packages in the "start" directory to 'runtimepath'.
 */
    void
add_pack_start_dirs(void)
{
    do_in_path(p_pp, (char_u *)"pack/*/start/*", DIP_ALL + DIP_DIR,
					       add_pack_plugin, &APP_ADD_DIR);
}

/*
 * Load plugins from all packages in the "start" directory.
 */
    void
load_start_packages(void)
{
    did_source_packages = TRUE;
    do_in_path(p_pp, (char_u *)"pack/*/start/*", DIP_ALL + DIP_DIR,
						  add_pack_plugin, &APP_LOAD);
}

/*
 * ":packloadall"
 * Find plugins in the package directories and source them.
 */
    void
ex_packloadall(exarg_T *eap)
{
    if (!did_source_packages || eap->forceit)
    {
	/* First do a round to add all directories to 'runtimepath', then load
	 * the plugins. This allows for plugins to use an autoload directory
	 * of another plugin. */
	add_pack_start_dirs();
	load_start_packages();
    }
}

/*
 * ":packadd[!] {name}"
 */
    void
ex_packadd(exarg_T *eap)
{
    static char *plugpat = "pack/*/%s/%s";
    int		len;
    char	*pat;
    int		round;
    int		res = OK;

    /* Round 1: use "start", round 2: use "opt". */
    for (round = 1; round <= 2; ++round)
    {
	/* Only look under "start" when loading packages wasn't done yet. */
	if (round == 1 && did_source_packages)
	    continue;

	len = (int)STRLEN(plugpat) + (int)STRLEN(eap->arg) + 5;
	pat = (char *)alloc(len);
	if (pat == NULL)
	    return;
	vim_snprintf(pat, len, plugpat, round == 1 ? "start" : "opt", eap->arg);
	/* The first round don't give a "not found" error, in the second round
	 * only when nothing was found in the first round. */
	res = do_in_path(p_pp, (char_u *)pat,
		DIP_ALL + DIP_DIR + (round == 2 && res == FAIL ? DIP_ERR : 0),
		add_pack_plugin, eap->forceit ? &APP_ADD_DIR : &APP_BOTH);
	vim_free(pat);
    }
}
#endif

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * ":options"
 */
    void
ex_options(
    exarg_T	*eap UNUSED)
{
    vim_setenv((char_u *)"OPTWIN_CMD", (char_u *)(cmdmod.tab ? "tab" : ""));
    cmd_source((char_u *)SYS_OPTWIN_FILE, NULL);
}
#endif

#if defined(FEAT_PYTHON3) || defined(FEAT_PYTHON) || defined(PROTO)

# if (defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)) || defined(PROTO)
/*
 * Detect Python 3 or 2, and initialize 'pyxversion'.
 */
    void
init_pyxversion(void)
{
    if (p_pyx == 0)
    {
	if (python3_enabled(FALSE))
	    p_pyx = 3;
	else if (python_enabled(FALSE))
	    p_pyx = 2;
    }
}
# endif

/*
 * Does a file contain one of the following strings at the beginning of any
 * line?
 * "#!(any string)python2"  => returns 2
 * "#!(any string)python3"  => returns 3
 * "# requires python 2.x"  => returns 2
 * "# requires python 3.x"  => returns 3
 * otherwise return 0.
 */
    static int
requires_py_version(char_u *filename)
{
    FILE    *file;
    int	    requires_py_version = 0;
    int	    i, lines;

    lines = (int)p_mls;
    if (lines < 0)
	lines = 5;

    file = mch_fopen((char *)filename, "r");
    if (file != NULL)
    {
	for (i = 0; i < lines; i++)
	{
	    if (vim_fgets(IObuff, IOSIZE, file))
		break;
	    if (i == 0 && IObuff[0] == '#' && IObuff[1] == '!')
	    {
		/* Check shebang. */
		if (strstr((char *)IObuff + 2, "python2") != NULL)
		{
		    requires_py_version = 2;
		    break;
		}
		if (strstr((char *)IObuff + 2, "python3") != NULL)
		{
		    requires_py_version = 3;
		    break;
		}
	    }
	    IObuff[21] = '\0';
	    if (STRCMP("# requires python 2.x", IObuff) == 0)
	    {
		requires_py_version = 2;
		break;
	    }
	    if (STRCMP("# requires python 3.x", IObuff) == 0)
	    {
		requires_py_version = 3;
		break;
	    }
	}
	fclose(file);
    }
    return requires_py_version;
}


/*
 * Source a python file using the requested python version.
 */
    static void
source_pyx_file(exarg_T *eap, char_u *fname)
{
    exarg_T ex;
    int	    v = requires_py_version(fname);

# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
    init_pyxversion();
# endif
    if (v == 0)
    {
# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
	/* user didn't choose a preference, 'pyx' is used */
	v = p_pyx;
# elif defined(FEAT_PYTHON)
	v = 2;
# elif defined(FEAT_PYTHON3)
	v = 3;
# endif
    }

    /*
     * now source, if required python version is not supported show
     * unobtrusive message.
     */
    if (eap == NULL)
	vim_memset(&ex, 0, sizeof(ex));
    else
	ex = *eap;
    ex.arg = fname;
    ex.cmd = (char_u *)(v == 2 ? "pyfile" : "pyfile3");

    if (v == 2)
    {
# ifdef FEAT_PYTHON
	ex_pyfile(&ex);
# else
	vim_snprintf((char *)IObuff, IOSIZE,
		_("W20: Required python version 2.x not supported, ignoring file: %s"),
		fname);
	MSG(IObuff);
# endif
	return;
    }
    else
    {
# ifdef FEAT_PYTHON3
	ex_py3file(&ex);
# else
	vim_snprintf((char *)IObuff, IOSIZE,
		_("W21: Required python version 3.x not supported, ignoring file: %s"),
		fname);
	MSG(IObuff);
# endif
	return;
    }
}

/*
 * ":pyxfile {fname}"
 */
    void
ex_pyxfile(exarg_T *eap)
{
    source_pyx_file(eap, eap->arg);
}

/*
 * ":pyx"
 */
    void
ex_pyx(exarg_T *eap)
{
# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
    init_pyxversion();
    if (p_pyx == 2)
	ex_python(eap);
    else
	ex_py3(eap);
# elif defined(FEAT_PYTHON)
    ex_python(eap);
# elif defined(FEAT_PYTHON3)
    ex_py3(eap);
# endif
}

/*
 * ":pyxdo"
 */
    void
ex_pyxdo(exarg_T *eap)
{
# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
    init_pyxversion();
    if (p_pyx == 2)
	ex_pydo(eap);
    else
	ex_py3do(eap);
# elif defined(FEAT_PYTHON)
    ex_pydo(eap);
# elif defined(FEAT_PYTHON3)
    ex_py3do(eap);
# endif
}

#endif

/*
 * ":source {fname}"
 */
    void
ex_source(exarg_T *eap)
{
#ifdef FEAT_BROWSE
    if (cmdmod.browse)
    {
	char_u *fname = NULL;

	fname = do_browse(0, (char_u *)_("Source Vim script"), eap->arg,
				      NULL, NULL,
				      (char_u *)_(BROWSE_FILTER_MACROS), NULL);
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
cmd_source(char_u *fname, exarg_T *eap)
{
    if (*fname == NUL)
	EMSG(_(e_argreq));

    else if (eap != NULL && eap->forceit)
	/* ":source!": read Normal mode commands
	 * Need to execute the commands directly.  This is required at least
	 * for:
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
    else if (do_source(fname, FALSE, DOSO_NONE) == FAIL)
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
#if defined(USE_CRNL) || defined(USE_CR)
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
source_breakpoint(void *cookie)
{
    return &((struct source_cookie *)cookie)->breakpoint;
}

/*
 * Return the address holding the debug tick for a source cookie.
 */
    int *
source_dbg_tick(void *cookie)
{
    return &((struct source_cookie *)cookie)->dbg_tick;
}

/*
 * Return the nesting level for a source cookie.
 */
    int
source_level(void *cookie)
{
    return ((struct source_cookie *)cookie)->level;
}
#endif

static char_u *get_one_sourceline(struct source_cookie *sp);

#if (defined(WIN32) && defined(FEAT_CSCOPE)) || defined(HAVE_FD_CLOEXEC)
# define USE_FOPEN_NOINH
static FILE *fopen_noinh_readbin(char *filename);

/*
 * Special function to open a file without handle inheritance.
 * When possible the handle is closed on exec().
 */
    static FILE *
fopen_noinh_readbin(char *filename)
{
# ifdef WIN32
    int	fd_tmp = mch_open(filename, O_RDONLY | O_BINARY | O_NOINHERIT, 0);
# else
    int	fd_tmp = mch_open(filename, O_RDONLY, 0);
# endif

    if (fd_tmp == -1)
	return NULL;

# ifdef HAVE_FD_CLOEXEC
    {
	int fdflags = fcntl(fd_tmp, F_GETFD);
	if (fdflags >= 0 && (fdflags & FD_CLOEXEC) == 0)
	    (void)fcntl(fd_tmp, F_SETFD, fdflags | FD_CLOEXEC);
    }
# endif

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
do_source(
    char_u	*fname,
    int		check_other,	    /* check for .vimrc and _vimrc */
    int		is_vimrc)	    /* DOSO_ value */
{
    struct source_cookie    cookie;
    char_u		    *save_sourcing_name;
    linenr_T		    save_sourcing_lnum;
    char_u		    *p;
    char_u		    *fname_exp;
    char_u		    *firstline = NULL;
    int			    retval = FAIL;
#ifdef FEAT_EVAL
    sctx_T		    save_current_sctx;
    static scid_T	    last_current_SID = 0;
    void		    *save_funccalp;
    int			    save_debug_break_level = debug_break_level;
    scriptitem_T	    *si = NULL;
# ifdef UNIX
    stat_T		    st;
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

    p = expand_env_save(fname);
    if (p == NULL)
	return retval;
    fname_exp = fix_fname(p);
    vim_free(p);
    if (fname_exp == NULL)
	return retval;
    if (mch_isdir(fname_exp))
    {
	smsg((char_u *)_("Cannot source a directory: \"%s\""), fname);
	goto theend;
    }

    /* Apply SourceCmd autocommands, they should get the file and source it. */
    if (has_autocmd(EVENT_SOURCECMD, fname_exp, NULL)
	    && apply_autocmds(EVENT_SOURCECMD, fname_exp, fname_exp,
							       FALSE, curbuf))
    {
#ifdef FEAT_EVAL
	retval = aborting() ? FAIL : OK;
#else
	retval = OK;
#endif
	goto theend;
    }

    /* Apply SourcePre autocommands, they may get the file. */
    apply_autocmds(EVENT_SOURCEPRE, fname_exp, fname_exp, FALSE, curbuf);

#ifdef USE_FOPEN_NOINH
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
#ifdef USE_FOPEN_NOINH
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
	    verbose_enter();
	    if (sourcing_name == NULL)
		smsg((char_u *)_("could not source \"%s\""), fname);
	    else
		smsg((char_u *)_("line %ld: could not source \"%s\""),
							sourcing_lnum, fname);
	    verbose_leave();
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
	verbose_enter();
	if (sourcing_name == NULL)
	    smsg((char_u *)_("sourcing \"%s\""), fname);
	else
	    smsg((char_u *)_("line %ld: sourcing \"%s\""),
							sourcing_lnum, fname);
	verbose_leave();
    }
    if (is_vimrc == DOSO_VIMRC)
	vimrc_found(fname_exp, (char_u *)"MYVIMRC");
    else if (is_vimrc == DOSO_GVIMRC)
	vimrc_found(fname_exp, (char_u *)"MYGVIMRC");

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

    /*
     * Keep the sourcing name/lnum, for recursive calls.
     */
    save_sourcing_name = sourcing_name;
    sourcing_name = fname_exp;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 0;

#ifdef STARTUPTIME
    if (time_fd != NULL)
	time_push(&tv_rel, &tv_start);
#endif

#ifdef FEAT_EVAL
# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	prof_child_enter(&wait_start);		/* entering a child now */
# endif

    /* Don't use local function variables, if called from a function.
     * Also starts profiling timer for nested script. */
    save_funccalp = save_funccal();

    /*
     * Check if this script was sourced before to finds its SID.
     * If it's new, generate a new SID.
     */
    save_current_sctx = current_sctx;
    current_sctx.sc_lnum = 0;
# ifdef UNIX
    stat_ok = (mch_stat((char *)fname_exp, &st) >= 0);
# endif
    for (current_sctx.sc_sid = script_items.ga_len; current_sctx.sc_sid > 0;
							 --current_sctx.sc_sid)
    {
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
	if (si->sn_name != NULL
		&& (
# ifdef UNIX
		    /* Compare dev/ino when possible, it catches symbolic
		     * links.  Also compare file names, the inode may change
		     * when the file was edited. */
		    ((stat_ok && si->sn_dev_valid)
			&& (si->sn_dev == st.st_dev
			    && si->sn_ino == st.st_ino)) ||
# endif
		fnamecmp(si->sn_name, fname_exp) == 0))
	    break;
    }
    if (current_sctx.sc_sid == 0)
    {
	current_sctx.sc_sid = ++last_current_SID;
	if (ga_grow(&script_items,
		     (int)(current_sctx.sc_sid - script_items.ga_len)) == FAIL)
	    goto almosttheend;
	while (script_items.ga_len < current_sctx.sc_sid)
	{
	    ++script_items.ga_len;
	    SCRIPT_ITEM(script_items.ga_len).sn_name = NULL;
# ifdef FEAT_PROFILE
	    SCRIPT_ITEM(script_items.ga_len).sn_prof_on = FALSE;
# endif
	}
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
	si->sn_name = fname_exp;
	fname_exp = NULL;
# ifdef UNIX
	if (stat_ok)
	{
	    si->sn_dev_valid = TRUE;
	    si->sn_dev = st.st_dev;
	    si->sn_ino = st.st_ino;
	}
	else
	    si->sn_dev_valid = FALSE;
# endif

	/* Allocate the local script variables to use for this script. */
	new_script_vars(current_sctx.sc_sid);
    }

# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
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

#ifdef FEAT_MBYTE
    cookie.conv.vc_type = CONV_NONE;		/* no conversion */

    /* Read the first line so we can check for a UTF-8 BOM. */
    firstline = getsourceline(0, (void *)&cookie, 0);
    if (firstline != NULL && STRLEN(firstline) >= 3 && firstline[0] == 0xef
			      && firstline[1] == 0xbb && firstline[2] == 0xbf)
    {
	/* Found BOM; setup conversion, skip over BOM and recode the line. */
	convert_setup(&cookie.conv, (char_u *)"utf-8", p_enc);
	p = string_convert(&cookie.conv, firstline + 3, NULL);
	if (p == NULL)
	    p = vim_strsave(firstline + 3);
	if (p != NULL)
	{
	    vim_free(firstline);
	    firstline = p;
	}
    }
#endif

    /*
     * Call do_cmdline, which will call getsourceline() to get the lines.
     */
    do_cmdline(firstline, getsourceline, (void *)&cookie,
				     DOCMD_VERBOSE|DOCMD_NOWAIT|DOCMD_REPEAT);
    retval = OK;

#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
    {
	/* Get "si" again, "script_items" may have been reallocated. */
	si = &SCRIPT_ITEM(current_sctx.sc_sid);
	if (si->sn_prof_on)
	{
	    profile_end(&si->sn_pr_start);
	    profile_sub_wait(&wait_start, &si->sn_pr_start);
	    profile_add(&si->sn_pr_total, &si->sn_pr_start);
	    profile_self(&si->sn_pr_self, &si->sn_pr_start,
							 &si->sn_pr_children);
	}
    }
#endif

    if (got_int)
	EMSG(_(e_interr));
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    if (p_verbose > 1)
    {
	verbose_enter();
	smsg((char_u *)_("finished sourcing %s"), fname);
	if (sourcing_name != NULL)
	    smsg((char_u *)_("continuing in %s"), sourcing_name);
	verbose_leave();
    }
#ifdef STARTUPTIME
    if (time_fd != NULL)
    {
	vim_snprintf((char *)IObuff, IOSIZE, "sourcing %s", fname);
	time_msg((char *)IObuff, &tv_start);
	time_pop(&tv_rel);
    }
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
    current_sctx = save_current_sctx;
    restore_funccal(save_funccalp);
# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	prof_child_exit(&wait_start);		/* leaving a child now */
# endif
#endif
    fclose(cookie.fp);
    vim_free(cookie.nextline);
    vim_free(firstline);
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
    void
ex_scriptnames(exarg_T *eap UNUSED)
{
    int i;

    for (i = 1; i <= script_items.ga_len && !got_int; ++i)
	if (SCRIPT_ITEM(i).sn_name != NULL)
	{
	    home_replace(NULL, SCRIPT_ITEM(i).sn_name,
						    NameBuff, MAXPATHL, TRUE);
	    smsg((char_u *)"%3d: %s", i, NameBuff);
	}
}

# if defined(BACKSLASH_IN_FILENAME) || defined(PROTO)
/*
 * Fix slashes in the list of script names for 'shellslash'.
 */
    void
scriptnames_slash_adjust(void)
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
get_scriptname(scid_T id)
{
    if (id == SID_MODELINE)
	return (char_u *)_("modeline");
    if (id == SID_CMDARG)
	return (char_u *)_("--cmd argument");
    if (id == SID_CARG)
	return (char_u *)_("-c argument");
    if (id == SID_ENV)
	return (char_u *)_("environment variable");
    if (id == SID_ERROR)
	return (char_u *)_("error handler");
    return SCRIPT_ITEM(id).sn_name;
}

# if defined(EXITFREE) || defined(PROTO)
    void
free_scriptnames(void)
{
    int			i;

    for (i = script_items.ga_len; i > 0; --i)
	vim_free(SCRIPT_ITEM(i).sn_name);
    ga_clear(&script_items);
}
# endif

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
fgets_cr(char *s, int n, FILE *stream)
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
fgets_cr(char *s, int n, FILE *stream)
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
    char_u *
getsourceline(int c UNUSED, void *cookie, int indent UNUSED)
{
    struct source_cookie *sp = (struct source_cookie *)cookie;
    char_u		*line;
    char_u		*p;

#ifdef FEAT_EVAL
    /* If breakpoints have been added/deleted need to check for it. */
    if (sp->dbg_tick < debug_tick)
    {
	sp->breakpoint = dbg_find_breakpoint(TRUE, sp->fname, sourcing_lnum);
	sp->dbg_tick = debug_tick;
    }
# ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
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
    if (line != NULL && do_profiling == PROF_YES)
	script_line_start();
#endif

    /* Only concatenate lines starting with a \ when 'cpoptions' doesn't
     * contain the 'C' flag. */
    if (line != NULL && (vim_strchr(p_cpo, CPO_CONCAT) == NULL))
    {
	/* compensate for the one line read-ahead */
	--sourcing_lnum;

	// Get the next line and concatenate it when it starts with a
	// backslash. We always need to read the next line, keep it in
	// sp->nextline.
	/* Also check for a comment in between continuation lines: "\ */
	sp->nextline = get_one_sourceline(sp);
	if (sp->nextline != NULL
		&& (*(p = skipwhite(sp->nextline)) == '\\'
			      || (p[0] == '"' && p[1] == '\\' && p[2] == ' ')))
	{
	    garray_T    ga;

	    ga_init2(&ga, (int)sizeof(char_u), 400);
	    ga_concat(&ga, line);
	    if (*p == '\\')
		ga_concat(&ga, p + 1);
	    for (;;)
	    {
		vim_free(sp->nextline);
		sp->nextline = get_one_sourceline(sp);
		if (sp->nextline == NULL)
		    break;
		p = skipwhite(sp->nextline);
		if (*p == '\\')
		{
		    // Adjust the growsize to the current length to speed up
		    // concatenating many lines.
		    if (ga.ga_len > 400)
		    {
			if (ga.ga_len > 8000)
			    ga.ga_growsize = 8000;
			else
			    ga.ga_growsize = ga.ga_len;
		    }
		    ga_concat(&ga, p + 1);
		}
		else if (p[0] != '"' || p[1] != '\\' || p[2] != ' ')
		    break;
	    }
	    ga_append(&ga, NUL);
	    vim_free(line);
	    line = ga.ga_data;
	}
    }

#ifdef FEAT_MBYTE
    if (line != NULL && sp->conv.vc_type != CONV_NONE)
    {
	char_u	*s;

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
get_one_sourceline(struct source_cookie *sp)
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
			msg_source(HL_ATTR(HLF_W));
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
script_line_start(void)
{
    scriptitem_T    *si;
    sn_prl_T	    *pp;

    if (current_sctx.sc_sid <= 0 || current_sctx.sc_sid > script_items.ga_len)
	return;
    si = &SCRIPT_ITEM(current_sctx.sc_sid);
    if (si->sn_prof_on && sourcing_lnum >= 1)
    {
	/* Grow the array before starting the timer, so that the time spent
	 * here isn't counted. */
	(void)ga_grow(&si->sn_prl_ga,
				  (int)(sourcing_lnum - si->sn_prl_ga.ga_len));
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
script_line_exec(void)
{
    scriptitem_T    *si;

    if (current_sctx.sc_sid <= 0 || current_sctx.sc_sid > script_items.ga_len)
	return;
    si = &SCRIPT_ITEM(current_sctx.sc_sid);
    if (si->sn_prof_on && si->sn_prl_idx >= 0)
	si->sn_prl_execed = TRUE;
}

/*
 * Called when done with a script line.
 */
    void
script_line_end(void)
{
    scriptitem_T    *si;
    sn_prl_T	    *pp;

    if (current_sctx.sc_sid <= 0 || current_sctx.sc_sid > script_items.ga_len)
	return;
    si = &SCRIPT_ITEM(current_sctx.sc_sid);
    if (si->sn_prof_on && si->sn_prl_idx >= 0
				     && si->sn_prl_idx < si->sn_prl_ga.ga_len)
    {
	if (si->sn_prl_execed)
	{
	    pp = &PRL_ITEM(si, si->sn_prl_idx);
	    ++pp->snp_count;
	    profile_end(&si->sn_prl_start);
	    profile_sub_wait(&si->sn_prl_wait, &si->sn_prl_start);
	    profile_add(&pp->sn_prl_total, &si->sn_prl_start);
	    profile_self(&pp->sn_prl_self, &si->sn_prl_start,
							&si->sn_prl_children);
	}
	si->sn_prl_idx = -1;
    }
}
#endif

/*
 * ":scriptencoding": Set encoding conversion for a sourced script.
 * Without the multi-byte feature it's simply ignored.
 */
    void
ex_scriptencoding(exarg_T *eap UNUSED)
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
ex_finish(exarg_T *eap)
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
do_finish(exarg_T *eap, int reanimate)
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
source_finished(
    char_u	*(*fgetline)(int, void *, int),
    void	*cookie)
{
    return (getline_equal(fgetline, cookie, getsourceline)
	    && ((struct source_cookie *)getline_cookie(
						fgetline, cookie))->finished);
}
#endif

/*
 * ":checktime [buffer]"
 */
    void
ex_checktime(exarg_T *eap)
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

#if (defined(HAVE_LOCALE_H) || defined(X_LOCALE)) \
	&& (defined(FEAT_EVAL) || defined(FEAT_MULTI_LANG))
# define HAVE_GET_LOCALE_VAL
static char_u *get_locale_val(int what);

    static char_u *
get_locale_val(int what)
{
    char_u	*loc;

    /* Obtain the locale value from the libraries. */
    loc = (char_u *)setlocale(what, NULL);

# ifdef WIN32
    if (loc != NULL)
    {
	char_u	*p;

	/* setocale() returns something like "LC_COLLATE=<name>;LC_..." when
	 * one of the values (e.g., LC_CTYPE) differs. */
	p = vim_strchr(loc, '=');
	if (p != NULL)
	{
	    loc = ++p;
	    while (*p != NUL)	/* remove trailing newline */
	    {
		if (*p < ' ' || *p == ';')
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
	    return (char_u *)mtable[i + 1];
    return name;
}
#endif

#if defined(FEAT_MULTI_LANG) || defined(PROTO)
/*
 * Obtain the current messages language.  Used to set the default for
 * 'helplang'.  May return NULL or an empty string.
 */
    char_u *
get_mess_lang(void)
{
    char_u *p;

# ifdef HAVE_GET_LOCALE_VAL
#  if defined(LC_MESSAGES)
    p = get_locale_val(LC_MESSAGES);
#  else
    /* This is necessary for Win32, where LC_MESSAGES is not defined and $LANG
     * may be set to the LCID number.  LC_COLLATE is the best guess, LC_TIME
     * and LC_MONETARY may be set differently for a Japanese working in the
     * US. */
    p = get_locale_val(LC_COLLATE);
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
static char_u *get_mess_env(void);

/*
 * Get the language used for messages from the environment.
 */
    static char_u *
get_mess_env(void)
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
# ifdef HAVE_GET_LOCALE_VAL
	    if (p == NULL || *p == NUL)
		p = get_locale_val(LC_CTYPE);
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
set_lang_var(void)
{
    char_u	*loc;

# ifdef HAVE_GET_LOCALE_VAL
    loc = get_locale_val(LC_CTYPE);
# else
    /* setlocale() not supported: use the default value */
    loc = (char_u *)"C";
# endif
    set_vim_var_string(VV_CTYPE, loc, -1);

    /* When LC_MESSAGES isn't defined use the value from $LC_MESSAGES, fall
     * back to LC_CTYPE if it's empty. */
# if defined(HAVE_GET_LOCALE_VAL) && defined(LC_MESSAGES)
    loc = get_locale_val(LC_MESSAGES);
# else
    loc = get_mess_env();
# endif
    set_vim_var_string(VV_LANG, loc, -1);

# ifdef HAVE_GET_LOCALE_VAL
    loc = get_locale_val(LC_TIME);
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
ex_language(exarg_T *eap)
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
    if ((*p == NUL || VIM_ISWHITE(*p)) && p - eap->arg >= 3)
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
	{
	    loc = setlocale(what, (char *)name);
#if defined(FEAT_FLOAT) && defined(LC_NUMERIC)
	    /* Make sure strtod() uses a decimal point, not a comma. */
	    setlocale(LC_NUMERIC, "C");
#endif
	}
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
	    /* Reset $LC_ALL, otherwise it would overrule everything. */
	    vim_setenv((char_u *)"LC_ALL", (char_u *)"");

	    if (what != LC_TIME)
	    {
		/* Tell gettext() what to translate to.  It apparently doesn't
		 * use the currently effective locale.  Also do this when
		 * FEAT_GETTEXT isn't defined, so that shell commands use this
		 * value. */
		if (what == LC_ALL)
		{
		    vim_setenv((char_u *)"LANG", name);

		    /* Clear $LANGUAGE because GNU gettext uses it. */
		    vim_setenv((char_u *)"LANGUAGE", (char_u *)"");
# ifdef WIN32
		    /* Apparently MS-Windows printf() may cause a crash when
		     * we give it 8-bit text while it's expecting text in the
		     * current locale.  This call avoids that. */
		    setlocale(LC_CTYPE, "C");
# endif
		}
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
	    }

# ifdef FEAT_EVAL
	    /* Set v:lang, v:lc_time and v:ctype to the final result. */
	    set_lang_var();
# endif
# ifdef FEAT_TITLE
	    maketitle();
# endif
	}
    }
}

# if defined(FEAT_CMDL_COMPL) || defined(PROTO)

static char_u	**locales = NULL;	/* Array of all available locales */

#  ifndef WIN32
static int	did_init_locales = FALSE;

/* Return an array of strings for all available locales + NULL for the
 * last element.  Return NULL in case of error. */
    static char_u **
find_locales(void)
{
    garray_T	locales_ga;
    char_u	*loc;

    /* Find all available locales by running command "locale -a".  If this
     * doesn't work we won't have completion. */
    char_u *locale_a = get_cmd_output((char_u *)"locale -a",
						    NULL, SHELL_SILENT, NULL);
    if (locale_a == NULL)
	return NULL;
    ga_init2(&locales_ga, sizeof(char_u *), 20);

    /* Transform locale_a string where each locale is separated by "\n"
     * into an array of locale strings. */
    loc = (char_u *)strtok((char *)locale_a, "\n");

    while (loc != NULL)
    {
	if (ga_grow(&locales_ga, 1) == FAIL)
	    break;
	loc = vim_strsave(loc);
	if (loc == NULL)
	    break;

	((char_u **)locales_ga.ga_data)[locales_ga.ga_len++] = loc;
	loc = (char_u *)strtok(NULL, "\n");
    }
    vim_free(locale_a);
    if (ga_grow(&locales_ga, 1) == FAIL)
    {
	ga_clear(&locales_ga);
	return NULL;
    }
    ((char_u **)locales_ga.ga_data)[locales_ga.ga_len] = NULL;
    return (char_u **)locales_ga.ga_data;
}
#  endif

/*
 * Lazy initialization of all available locales.
 */
    static void
init_locales(void)
{
#  ifndef WIN32
    if (!did_init_locales)
    {
	did_init_locales = TRUE;
	locales = find_locales();
    }
#  endif
}

#  if defined(EXITFREE) || defined(PROTO)
    void
free_locales(void)
{
    int			i;
    if (locales != NULL)
    {
	for (i = 0; locales[i] != NULL; i++)
	    vim_free(locales[i]);
	VIM_CLEAR(locales);
    }
}
#  endif

/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":language" command.
 */
    char_u *
get_lang_arg(expand_T *xp UNUSED, int idx)
{
    if (idx == 0)
	return (char_u *)"messages";
    if (idx == 1)
	return (char_u *)"ctype";
    if (idx == 2)
	return (char_u *)"time";

    init_locales();
    if (locales == NULL)
	return NULL;
    return locales[idx - 3];
}

/*
 * Function given to ExpandGeneric() to obtain the available locales.
 */
    char_u *
get_locales(expand_T *xp UNUSED, int idx)
{
    init_locales();
    if (locales == NULL)
	return NULL;
    return locales[idx];
}
# endif

#endif
