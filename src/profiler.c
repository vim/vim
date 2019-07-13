/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * profiler.c: vim script profiler
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)
# if defined(FEAT_PROFILE) || defined(FEAT_RELTIME) || defined(PROTO)
/*
 * Store the current time in "tm".
 */
    void
profile_start(proftime_T *tm)
{
# ifdef MSWIN
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

# ifdef MSWIN
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
# ifdef MSWIN
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

# ifdef MSWIN
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
#  ifdef MSWIN
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
# ifdef MSWIN
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

# ifdef MSWIN
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
# ifdef MSWIN
    tm->QuadPart = 0;
# else
    tm->tv_usec = 0;
    tm->tv_sec = 0;
# endif
}

# endif  /* FEAT_PROFILE || FEAT_RELTIME */

#if defined(FEAT_SYN_HL) && defined(FEAT_RELTIME) && defined(FEAT_FLOAT) && defined(FEAT_PROFILE)
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
# ifdef MSWIN
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
static proftime_T prof_wait_time;

/*
 * Add the time "tm2" to "tm".
 */
    void
profile_add(proftime_T *tm, proftime_T *tm2)
{
# ifdef MSWIN
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
#ifdef MSWIN
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
# ifdef MSWIN
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
# ifdef MSWIN
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
	emsg(_("E750: First use \":profile start {fname}\""));
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
	    semsg(_(e_notopen), profile_fname);
	else
	{
	    script_dump_profile(fd);
	    func_dump_profile(fd);
	    fclose(fd);
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

    void
prof_sort_list(
    FILE	*fd,
    ufunc_T	**sorttab,
    int		st_len,
    char	*title,
    int		prefer_self)	/* when equal print only self time */
{
    int		i;
    ufunc_T	*fp;

    fprintf(fd, "FUNCTIONS SORTED ON %s TIME\n", title);
    fprintf(fd, "count  total (s)   self (s)  function\n");
    for (i = 0; i < 20 && i < st_len; ++i)
    {
	fp = sorttab[i];
	prof_func_line(fd, fp->uf_tm_count, &fp->uf_tm_total, &fp->uf_tm_self,
								 prefer_self);
	if (fp->uf_name[0] == K_SPECIAL)
	    fprintf(fd, " <SNR>%s()\n", fp->uf_name + 3);
	else
	    fprintf(fd, " %s()\n", fp->uf_name);
    }
    fprintf(fd, "\n");
}

/*
 * Print the count and times for one function or function line.
 */
    void
prof_func_line(
    FILE	*fd,
    int		count,
    proftime_T	*total,
    proftime_T	*self,
    int		prefer_self)	/* when equal print only self time */
{
    if (count > 0)
    {
	fprintf(fd, "%5d ", count);
	if (prefer_self && profile_equal(total, self))
	    fprintf(fd, "           ");
	else
	    fprintf(fd, "%s ", profile_msg(total));
	if (!prefer_self && profile_equal(total, self))
	    fprintf(fd, "           ");
	else
	    fprintf(fd, "%s ", profile_msg(self));
    }
    else
	fprintf(fd, "                            ");
}

/*
 * Compare function for total time sorting.
 */
    int
prof_total_cmp(const void *s1, const void *s2)
{
    ufunc_T	*p1, *p2;

    p1 = *(ufunc_T **)s1;
    p2 = *(ufunc_T **)s2;
    return profile_cmp(&p1->uf_tm_total, &p2->uf_tm_total);
}

/*
 * Compare function for self time sorting.
 */
    int
prof_self_cmp(const void *s1, const void *s2)
{
    ufunc_T	*p1, *p2;

    p1 = *(ufunc_T **)s1;
    p2 = *(ufunc_T **)s2;
    return profile_cmp(&p1->uf_tm_self, &p2->uf_tm_self);
}

/*
 * Start profiling function "fp".
 */
    void
func_do_profile(ufunc_T *fp)
{
    int		len = fp->uf_lines.ga_len;

    if (!fp->uf_prof_initialized)
    {
	if (len == 0)
	    len = 1;  /* avoid getting error for allocating zero bytes */
	fp->uf_tm_count = 0;
	profile_zero(&fp->uf_tm_self);
	profile_zero(&fp->uf_tm_total);
	if (fp->uf_tml_count == NULL)
	    fp->uf_tml_count = ALLOC_CLEAR_MULT(int, len);
	if (fp->uf_tml_total == NULL)
	    fp->uf_tml_total = ALLOC_CLEAR_MULT(proftime_T, len);
	if (fp->uf_tml_self == NULL)
	    fp->uf_tml_self = ALLOC_CLEAR_MULT(proftime_T, len);
	fp->uf_tml_idx = -1;
	if (fp->uf_tml_count == NULL || fp->uf_tml_total == NULL
						    || fp->uf_tml_self == NULL)
	    return;	    /* out of memory */
	fp->uf_prof_initialized = TRUE;
    }

    fp->uf_profiling = TRUE;
}

/*
 * Prepare profiling for entering a child or something else that is not
 * counted for the script/function itself.
 * Should always be called in pair with prof_child_exit().
 */
    void
prof_child_enter(
    proftime_T *tm)	/* place to store waittime */
{
    funccall_T *fc = get_current_funccal();

    if (fc != NULL && fc->func->uf_profiling)
	profile_start(&fc->prof_child);
    script_prof_save(tm);
}

/*
 * Take care of time spent in a child.
 * Should always be called after prof_child_enter().
 */
    void
prof_child_exit(
    proftime_T *tm)	/* where waittime was stored */
{
    funccall_T *fc = get_current_funccal();

    if (fc != NULL && fc->func->uf_profiling)
    {
	profile_end(&fc->prof_child);
	profile_sub_wait(tm, &fc->prof_child); /* don't count waiting time */
	profile_add(&fc->func->uf_tm_children, &fc->prof_child);
	profile_add(&fc->func->uf_tml_children, &fc->prof_child);
    }
    script_prof_restore(tm);
}

/*
 * Called when starting to read a function line.
 * "sourcing_lnum" must be correct!
 * When skipping lines it may not actually be executed, but we won't find out
 * until later and we need to store the time now.
 */
    void
func_line_start(void *cookie)
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;

    if (fp->uf_profiling && sourcing_lnum >= 1
				      && sourcing_lnum <= fp->uf_lines.ga_len)
    {
	fp->uf_tml_idx = sourcing_lnum - 1;
	/* Skip continuation lines. */
	while (fp->uf_tml_idx > 0 && FUNCLINE(fp, fp->uf_tml_idx) == NULL)
	    --fp->uf_tml_idx;
	fp->uf_tml_execed = FALSE;
	profile_start(&fp->uf_tml_start);
	profile_zero(&fp->uf_tml_children);
	profile_get_wait(&fp->uf_tml_wait);
    }
}

/*
 * Called when actually executing a function line.
 */
    void
func_line_exec(void *cookie)
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;

    if (fp->uf_profiling && fp->uf_tml_idx >= 0)
	fp->uf_tml_execed = TRUE;
}

/*
 * Called when done with a function line.
 */
    void
func_line_end(void *cookie)
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;

    if (fp->uf_profiling && fp->uf_tml_idx >= 0)
    {
	if (fp->uf_tml_execed)
	{
	    ++fp->uf_tml_count[fp->uf_tml_idx];
	    profile_end(&fp->uf_tml_start);
	    profile_sub_wait(&fp->uf_tml_wait, &fp->uf_tml_start);
	    profile_add(&fp->uf_tml_total[fp->uf_tml_idx], &fp->uf_tml_start);
	    profile_self(&fp->uf_tml_self[fp->uf_tml_idx], &fp->uf_tml_start,
							&fp->uf_tml_children);
	}
	fp->uf_tml_idx = -1;
    }
}
# endif /* FEAT_PROFILE */

#endif
