/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * eval.c: User defined function support
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)
/* function flags */
#define FC_ABORT    0x01	/* abort function on error */
#define FC_RANGE    0x02	/* function accepts range */
#define FC_DICT	    0x04	/* Dict function, uses "self" */
#define FC_CLOSURE  0x08	/* closure, uses outer scope variables */
#define FC_DELETED  0x10	/* :delfunction used while uf_refcount > 0 */
#define FC_REMOVED  0x20	/* function redefined while uf_refcount > 0 */

/* From user function to hashitem and back. */
#define UF2HIKEY(fp) ((fp)->uf_name)
#define HIKEY2UF(p)  ((ufunc_T *)(p - offsetof(ufunc_T, uf_name)))
#define HI2UF(hi)     HIKEY2UF((hi)->hi_key)

#define FUNCARG(fp, j)	((char_u **)(fp->uf_args.ga_data))[j]
#define FUNCLINE(fp, j)	((char_u **)(fp->uf_lines.ga_data))[j]

/*
 * All user-defined functions are found in this hashtable.
 */
static hashtab_T	func_hashtab;

/* Used by get_func_tv() */
static garray_T funcargs = GA_EMPTY;

/* pointer to funccal for currently active function */
funccall_T *current_funccal = NULL;

/* Pointer to list of previously used funccal, still around because some
 * item in it is still being used. */
funccall_T *previous_funccal = NULL;

static char *e_funcexts = N_("E122: Function %s already exists, add ! to replace it");
static char *e_funcdict = N_("E717: Dictionary entry already exists");
static char *e_funcref = N_("E718: Funcref required");
static char *e_nofunc = N_("E130: Unknown function: %s");

#ifdef FEAT_PROFILE
static void func_do_profile(ufunc_T *fp);
static void prof_sort_list(FILE *fd, ufunc_T **sorttab, int st_len, char *title, int prefer_self);
static void prof_func_line(FILE *fd, int count, proftime_T *total, proftime_T *self, int prefer_self);
static int
# ifdef __BORLANDC__
    _RTLENTRYF
# endif
	prof_total_cmp(const void *s1, const void *s2);
static int
# ifdef __BORLANDC__
    _RTLENTRYF
# endif
	prof_self_cmp(const void *s1, const void *s2);
#endif
static void funccal_unref(funccall_T *fc, ufunc_T *fp, int force);

    void
func_init()
{
    hash_init(&func_hashtab);
}

/*
 * Get function arguments.
 */
    static int
get_function_args(
    char_u	**argp,
    char_u	endchar,
    garray_T	*newargs,
    int		*varargs,
    int		skip)
{
    int		mustend = FALSE;
    char_u	*arg = *argp;
    char_u	*p = arg;
    int		c;
    int		i;

    if (newargs != NULL)
	ga_init2(newargs, (int)sizeof(char_u *), 3);

    if (varargs != NULL)
	*varargs = FALSE;

    /*
     * Isolate the arguments: "arg1, arg2, ...)"
     */
    while (*p != endchar)
    {
	if (p[0] == '.' && p[1] == '.' && p[2] == '.')
	{
	    if (varargs != NULL)
		*varargs = TRUE;
	    p += 3;
	    mustend = TRUE;
	}
	else
	{
	    arg = p;
	    while (ASCII_ISALNUM(*p) || *p == '_')
		++p;
	    if (arg == p || isdigit(*arg)
		    || (p - arg == 9 && STRNCMP(arg, "firstline", 9) == 0)
		    || (p - arg == 8 && STRNCMP(arg, "lastline", 8) == 0))
	    {
		if (!skip)
		    EMSG2(_("E125: Illegal argument: %s"), arg);
		break;
	    }
	    if (newargs != NULL && ga_grow(newargs, 1) == FAIL)
		goto err_ret;
	    if (newargs != NULL)
	    {
		c = *p;
		*p = NUL;
		arg = vim_strsave(arg);
		if (arg == NULL)
		{
		    *p = c;
		    goto err_ret;
		}

		/* Check for duplicate argument name. */
		for (i = 0; i < newargs->ga_len; ++i)
		    if (STRCMP(((char_u **)(newargs->ga_data))[i], arg) == 0)
		    {
			EMSG2(_("E853: Duplicate argument name: %s"), arg);
			vim_free(arg);
			goto err_ret;
		    }
		((char_u **)(newargs->ga_data))[newargs->ga_len] = arg;
		newargs->ga_len++;

		*p = c;
	    }
	    if (*p == ',')
		++p;
	    else
		mustend = TRUE;
	}
	p = skipwhite(p);
	if (mustend && *p != endchar)
	{
	    if (!skip)
		EMSG2(_(e_invarg2), *argp);
	    break;
	}
    }
    if (*p != endchar)
	goto err_ret;
    ++p;	/* skip "endchar" */

    *argp = p;
    return OK;

err_ret:
    if (newargs != NULL)
	ga_clear_strings(newargs);
    return FAIL;
}

/*
 * Register function "fp" as using "current_funccal" as its scope.
 */
    static int
register_closure(ufunc_T *fp)
{
    if (fp->uf_scoped == current_funccal)
	/* no change */
	return OK;
    funccal_unref(fp->uf_scoped, fp, FALSE);
    fp->uf_scoped = current_funccal;
    current_funccal->fc_refcount++;

    if (ga_grow(&current_funccal->fc_funcs, 1) == FAIL)
	return FAIL;
    ((ufunc_T **)current_funccal->fc_funcs.ga_data)
	[current_funccal->fc_funcs.ga_len++] = fp;
    return OK;
}

/*
 * Parse a lambda expression and get a Funcref from "*arg".
 * Return OK or FAIL.  Returns NOTDONE for dict or {expr}.
 */
    int
get_lambda_tv(char_u **arg, typval_T *rettv, int evaluate)
{
    garray_T	newargs;
    garray_T	newlines;
    garray_T	*pnewargs;
    ufunc_T	*fp = NULL;
    int		varargs;
    int		ret;
    char_u	*start = skipwhite(*arg + 1);
    char_u	*s, *e;
    static int	lambda_no = 0;
    int		*old_eval_lavars = eval_lavars_used;
    int		eval_lavars = FALSE;

    ga_init(&newargs);
    ga_init(&newlines);

    /* First, check if this is a lambda expression. "->" must exist. */
    ret = get_function_args(&start, '-', NULL, NULL, TRUE);
    if (ret == FAIL || *start != '>')
	return NOTDONE;

    /* Parse the arguments again. */
    if (evaluate)
	pnewargs = &newargs;
    else
	pnewargs = NULL;
    *arg = skipwhite(*arg + 1);
    ret = get_function_args(arg, '-', pnewargs, &varargs, FALSE);
    if (ret == FAIL || **arg != '>')
	goto errret;

    /* Set up a flag for checking local variables and arguments. */
    if (evaluate)
	eval_lavars_used = &eval_lavars;

    /* Get the start and the end of the expression. */
    *arg = skipwhite(*arg + 1);
    s = *arg;
    ret = skip_expr(arg);
    if (ret == FAIL)
	goto errret;
    e = *arg;
    *arg = skipwhite(*arg);
    if (**arg != '}')
	goto errret;
    ++*arg;

    if (evaluate)
    {
	int	    len, flags = 0;
	char_u	    *p;
	char_u	    name[20];
	partial_T   *pt;

	sprintf((char*)name, "<lambda>%d", ++lambda_no);

	fp = (ufunc_T *)alloc_clear((unsigned)(sizeof(ufunc_T) + STRLEN(name)));
	if (fp == NULL)
	    goto errret;
	pt = (partial_T *)alloc_clear((unsigned)sizeof(partial_T));
	if (pt == NULL)
	{
	    vim_free(fp);
	    goto errret;
	}

	ga_init2(&newlines, (int)sizeof(char_u *), 1);
	if (ga_grow(&newlines, 1) == FAIL)
	    goto errret;

	/* Add "return " before the expression. */
	len = 7 + e - s + 1;
	p = (char_u *)alloc(len);
	if (p == NULL)
	    goto errret;
	((char_u **)(newlines.ga_data))[newlines.ga_len++] = p;
	STRCPY(p, "return ");
	vim_strncpy(p + 7, s, e - s);

	fp->uf_refcount = 1;
	STRCPY(fp->uf_name, name);
	hash_add(&func_hashtab, UF2HIKEY(fp));
	fp->uf_args = newargs;
	fp->uf_lines = newlines;
	if (current_funccal != NULL && eval_lavars)
	{
	    flags |= FC_CLOSURE;
	    if (register_closure(fp) == FAIL)
		goto errret;
	}
	else
	    fp->uf_scoped = NULL;

#ifdef FEAT_PROFILE
	if (prof_def_func())
	    func_do_profile(fp);
#endif
	fp->uf_varargs = TRUE;
	fp->uf_flags = flags;
	fp->uf_calls = 0;
	fp->uf_script_ID = current_SID;

	pt->pt_func = fp;
	pt->pt_refcount = 1;
	rettv->vval.v_partial = pt;
	rettv->v_type = VAR_PARTIAL;
    }

    eval_lavars_used = old_eval_lavars;
    return OK;

errret:
    ga_clear_strings(&newargs);
    ga_clear_strings(&newlines);
    vim_free(fp);
    eval_lavars_used = old_eval_lavars;
    return FAIL;
}

/*
 * Check if "name" is a variable of type VAR_FUNC.  If so, return the function
 * name it contains, otherwise return "name".
 * If "partialp" is not NULL, and "name" is of type VAR_PARTIAL also set
 * "partialp".
 */
    char_u *
deref_func_name(char_u *name, int *lenp, partial_T **partialp, int no_autoload)
{
    dictitem_T	*v;
    int		cc;
    char_u	*s;

    if (partialp != NULL)
	*partialp = NULL;

    cc = name[*lenp];
    name[*lenp] = NUL;
    v = find_var(name, NULL, no_autoload);
    name[*lenp] = cc;
    if (v != NULL && v->di_tv.v_type == VAR_FUNC)
    {
	if (v->di_tv.vval.v_string == NULL)
	{
	    *lenp = 0;
	    return (char_u *)"";	/* just in case */
	}
	s = v->di_tv.vval.v_string;
	*lenp = (int)STRLEN(s);
	return s;
    }

    if (v != NULL && v->di_tv.v_type == VAR_PARTIAL)
    {
	partial_T *pt = v->di_tv.vval.v_partial;

	if (pt == NULL)
	{
	    *lenp = 0;
	    return (char_u *)"";	/* just in case */
	}
	if (partialp != NULL)
	    *partialp = pt;
	s = partial_name(pt);
	*lenp = (int)STRLEN(s);
	return s;
    }

    return name;
}

/*
 * Give an error message with a function name.  Handle <SNR> things.
 * "ermsg" is to be passed without translation, use N_() instead of _().
 */
    static void
emsg_funcname(char *ermsg, char_u *name)
{
    char_u	*p;

    if (*name == K_SPECIAL)
	p = concat_str((char_u *)"<SNR>", name + 3);
    else
	p = name;
    EMSG2(_(ermsg), p);
    if (p != name)
	vim_free(p);
}

/*
 * Allocate a variable for the result of a function.
 * Return OK or FAIL.
 */
    int
get_func_tv(
    char_u	*name,		/* name of the function */
    int		len,		/* length of "name" */
    typval_T	*rettv,
    char_u	**arg,		/* argument, pointing to the '(' */
    linenr_T	firstline,	/* first line of range */
    linenr_T	lastline,	/* last line of range */
    int		*doesrange,	/* return: function handled range */
    int		evaluate,
    partial_T	*partial,	/* for extra arguments */
    dict_T	*selfdict)	/* Dictionary for "self" */
{
    char_u	*argp;
    int		ret = OK;
    typval_T	argvars[MAX_FUNC_ARGS + 1];	/* vars for arguments */
    int		argcount = 0;		/* number of arguments found */

    /*
     * Get the arguments.
     */
    argp = *arg;
    while (argcount < MAX_FUNC_ARGS - (partial == NULL ? 0 : partial->pt_argc))
    {
	argp = skipwhite(argp + 1);	    /* skip the '(' or ',' */
	if (*argp == ')' || *argp == ',' || *argp == NUL)
	    break;
	if (eval1(&argp, &argvars[argcount], evaluate) == FAIL)
	{
	    ret = FAIL;
	    break;
	}
	++argcount;
	if (*argp != ',')
	    break;
    }
    if (*argp == ')')
	++argp;
    else
	ret = FAIL;

    if (ret == OK)
    {
	int		i = 0;

	if (get_vim_var_nr(VV_TESTING))
	{
	    /* Prepare for calling test_garbagecollect_now(), need to know
	     * what variables are used on the call stack. */
	    if (funcargs.ga_itemsize == 0)
		ga_init2(&funcargs, (int)sizeof(typval_T *), 50);
	    for (i = 0; i < argcount; ++i)
		if (ga_grow(&funcargs, 1) == OK)
		    ((typval_T **)funcargs.ga_data)[funcargs.ga_len++] =
								  &argvars[i];
	}

	ret = call_func(name, len, rettv, argcount, argvars, NULL,
		 firstline, lastline, doesrange, evaluate, partial, selfdict);

	funcargs.ga_len -= i;
    }
    else if (!aborting())
    {
	if (argcount == MAX_FUNC_ARGS)
	    emsg_funcname(N_("E740: Too many arguments for function %s"), name);
	else
	    emsg_funcname(N_("E116: Invalid arguments for function %s"), name);
    }

    while (--argcount >= 0)
	clear_tv(&argvars[argcount]);

    *arg = skipwhite(argp);
    return ret;
}

#define FLEN_FIXED 40

/*
 * Return TRUE if "p" starts with "<SID>" or "s:".
 * Only works if eval_fname_script() returned non-zero for "p"!
 */
    static int
eval_fname_sid(char_u *p)
{
    return (*p == 's' || TOUPPER_ASC(p[2]) == 'I');
}

/*
 * In a script change <SID>name() and s:name() to K_SNR 123_name().
 * Change <SNR>123_name() to K_SNR 123_name().
 * Use "fname_buf[FLEN_FIXED + 1]" when it fits, otherwise allocate memory
 * (slow).
 */
    static char_u *
fname_trans_sid(char_u *name, char_u *fname_buf, char_u **tofree, int *error)
{
    int		llen;
    char_u	*fname;
    int		i;

    llen = eval_fname_script(name);
    if (llen > 0)
    {
	fname_buf[0] = K_SPECIAL;
	fname_buf[1] = KS_EXTRA;
	fname_buf[2] = (int)KE_SNR;
	i = 3;
	if (eval_fname_sid(name))	/* "<SID>" or "s:" */
	{
	    if (current_SID <= 0)
		*error = ERROR_SCRIPT;
	    else
	    {
		sprintf((char *)fname_buf + 3, "%ld_", (long)current_SID);
		i = (int)STRLEN(fname_buf);
	    }
	}
	if (i + STRLEN(name + llen) < FLEN_FIXED)
	{
	    STRCPY(fname_buf + i, name + llen);
	    fname = fname_buf;
	}
	else
	{
	    fname = alloc((unsigned)(i + STRLEN(name + llen) + 1));
	    if (fname == NULL)
		*error = ERROR_OTHER;
	    else
	    {
		*tofree = fname;
		mch_memmove(fname, fname_buf, (size_t)i);
		STRCPY(fname + i, name + llen);
	    }
	}
    }
    else
	fname = name;
    return fname;
}

/*
 * Find a function by name, return pointer to it in ufuncs.
 * Return NULL for unknown function.
 */
    ufunc_T *
find_func(char_u *name)
{
    hashitem_T	*hi;

    hi = hash_find(&func_hashtab, name);
    if (!HASHITEM_EMPTY(hi))
	return HI2UF(hi);
    return NULL;
}

/*
 * Copy the function name of "fp" to buffer "buf".
 * "buf" must be able to hold the function name plus three bytes.
 * Takes care of script-local function names.
 */
    static void
cat_func_name(char_u *buf, ufunc_T *fp)
{
    if (fp->uf_name[0] == K_SPECIAL)
    {
	STRCPY(buf, "<SNR>");
	STRCAT(buf, fp->uf_name + 3);
    }
    else
	STRCPY(buf, fp->uf_name);
}

/*
 * Add a number variable "name" to dict "dp" with value "nr".
 */
    static void
add_nr_var(
    dict_T	*dp,
    dictitem_T	*v,
    char	*name,
    varnumber_T nr)
{
    STRCPY(v->di_key, name);
    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    hash_add(&dp->dv_hashtab, DI2HIKEY(v));
    v->di_tv.v_type = VAR_NUMBER;
    v->di_tv.v_lock = VAR_FIXED;
    v->di_tv.vval.v_number = nr;
}

/*
 * Free "fc" and what it contains.
 */
   static void
free_funccal(
    funccall_T	*fc,
    int		free_val)  /* a: vars were allocated */
{
    listitem_T	*li;
    int		i;

    for (i = 0; i < fc->fc_funcs.ga_len; ++i)
    {
	ufunc_T	    *fp = ((ufunc_T **)(fc->fc_funcs.ga_data))[i];

	/* When garbage collecting a funccall_T may be freed before the
	 * function that references it, clear its uf_scoped field.
	 * The function may have been redefined and point to another
	 * funccall_T, don't clear it then. */
	if (fp != NULL && fp->uf_scoped == fc)
	    fp->uf_scoped = NULL;
    }
    ga_clear(&fc->fc_funcs);

    /* The a: variables typevals may not have been allocated, only free the
     * allocated variables. */
    vars_clear_ext(&fc->l_avars.dv_hashtab, free_val);

    /* free all l: variables */
    vars_clear(&fc->l_vars.dv_hashtab);

    /* Free the a:000 variables if they were allocated. */
    if (free_val)
	for (li = fc->l_varlist.lv_first; li != NULL; li = li->li_next)
	    clear_tv(&li->li_tv);

    func_ptr_unref(fc->func);
    vim_free(fc);
}

/*
 * Handle the last part of returning from a function: free the local hashtable.
 * Unless it is still in use by a closure.
 */
    static void
cleanup_function_call(funccall_T *fc)
{
    current_funccal = fc->caller;

    /* If the a:000 list and the l: and a: dicts are not referenced and there
     * is no closure using it, we can free the funccall_T and what's in it. */
    if (fc->l_varlist.lv_refcount == DO_NOT_FREE_CNT
	    && fc->l_vars.dv_refcount == DO_NOT_FREE_CNT
	    && fc->l_avars.dv_refcount == DO_NOT_FREE_CNT
	    && fc->fc_refcount <= 0)
    {
	free_funccal(fc, FALSE);
    }
    else
    {
	hashitem_T	*hi;
	listitem_T	*li;
	int		todo;
	dictitem_T	*v;

	/* "fc" is still in use.  This can happen when returning "a:000",
	 * assigning "l:" to a global variable or defining a closure.
	 * Link "fc" in the list for garbage collection later. */
	fc->caller = previous_funccal;
	previous_funccal = fc;

	/* Make a copy of the a: variables, since we didn't do that above. */
	todo = (int)fc->l_avars.dv_hashtab.ht_used;
	for (hi = fc->l_avars.dv_hashtab.ht_array; todo > 0; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		v = HI2DI(hi);
		copy_tv(&v->di_tv, &v->di_tv);
	    }
	}

	/* Make a copy of the a:000 items, since we didn't do that above. */
	for (li = fc->l_varlist.lv_first; li != NULL; li = li->li_next)
	    copy_tv(&li->li_tv, &li->li_tv);
    }
}

/*
 * Call a user function.
 */
    static void
call_user_func(
    ufunc_T	*fp,		/* pointer to function */
    int		argcount,	/* nr of args */
    typval_T	*argvars,	/* arguments */
    typval_T	*rettv,		/* return value */
    linenr_T	firstline,	/* first line of range */
    linenr_T	lastline,	/* last line of range */
    dict_T	*selfdict)	/* Dictionary for "self" */
{
    char_u	*save_sourcing_name;
    linenr_T	save_sourcing_lnum;
    scid_T	save_current_SID;
    funccall_T	*fc;
    int		save_did_emsg;
    static int	depth = 0;
    dictitem_T	*v;
    int		fixvar_idx = 0;	/* index in fixvar[] */
    int		i;
    int		ai;
    int		islambda = FALSE;
    char_u	numbuf[NUMBUFLEN];
    char_u	*name;
    size_t	len;
#ifdef FEAT_PROFILE
    proftime_T	wait_start;
    proftime_T	call_start;
    int		started_profiling = FALSE;
#endif

    /* If depth of calling is getting too high, don't execute the function */
    if (depth >= p_mfd)
    {
	EMSG(_("E132: Function call depth is higher than 'maxfuncdepth'"));
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = -1;
	return;
    }
    ++depth;

    line_breakcheck();		/* check for CTRL-C hit */

    fc = (funccall_T *)alloc(sizeof(funccall_T));
    fc->caller = current_funccal;
    current_funccal = fc;
    fc->func = fp;
    fc->rettv = rettv;
    rettv->vval.v_number = 0;
    fc->linenr = 0;
    fc->returned = FALSE;
    fc->level = ex_nesting_level;
    /* Check if this function has a breakpoint. */
    fc->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name, (linenr_T)0);
    fc->dbg_tick = debug_tick;
    /* Set up fields for closure. */
    fc->fc_refcount = 0;
    fc->fc_copyID = 0;
    ga_init2(&fc->fc_funcs, sizeof(ufunc_T *), 1);
    func_ptr_ref(fp);

    if (STRNCMP(fp->uf_name, "<lambda>", 8) == 0)
	islambda = TRUE;

    /*
     * Note about using fc->fixvar[]: This is an array of FIXVAR_CNT variables
     * with names up to VAR_SHORT_LEN long.  This avoids having to alloc/free
     * each argument variable and saves a lot of time.
     */
    /*
     * Init l: variables.
     */
    init_var_dict(&fc->l_vars, &fc->l_vars_var, VAR_DEF_SCOPE);
    if (selfdict != NULL)
    {
	/* Set l:self to "selfdict".  Use "name" to avoid a warning from
	 * some compiler that checks the destination size. */
	v = &fc->fixvar[fixvar_idx++].var;
	name = v->di_key;
	STRCPY(name, "self");
	v->di_flags = DI_FLAGS_RO + DI_FLAGS_FIX;
	hash_add(&fc->l_vars.dv_hashtab, DI2HIKEY(v));
	v->di_tv.v_type = VAR_DICT;
	v->di_tv.v_lock = 0;
	v->di_tv.vval.v_dict = selfdict;
	++selfdict->dv_refcount;
    }

    /*
     * Init a: variables.
     * Set a:0 to "argcount".
     * Set a:000 to a list with room for the "..." arguments.
     */
    init_var_dict(&fc->l_avars, &fc->l_avars_var, VAR_SCOPE);
    add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "0",
				(varnumber_T)(argcount - fp->uf_args.ga_len));
    /* Use "name" to avoid a warning from some compiler that checks the
     * destination size. */
    v = &fc->fixvar[fixvar_idx++].var;
    name = v->di_key;
    STRCPY(name, "000");
    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    hash_add(&fc->l_avars.dv_hashtab, DI2HIKEY(v));
    v->di_tv.v_type = VAR_LIST;
    v->di_tv.v_lock = VAR_FIXED;
    v->di_tv.vval.v_list = &fc->l_varlist;
    vim_memset(&fc->l_varlist, 0, sizeof(list_T));
    fc->l_varlist.lv_refcount = DO_NOT_FREE_CNT;
    fc->l_varlist.lv_lock = VAR_FIXED;

    /*
     * Set a:firstline to "firstline" and a:lastline to "lastline".
     * Set a:name to named arguments.
     * Set a:N to the "..." arguments.
     */
    add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "firstline",
						      (varnumber_T)firstline);
    add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "lastline",
						       (varnumber_T)lastline);
    for (i = 0; i < argcount; ++i)
    {
	int	    addlocal = FALSE;

	ai = i - fp->uf_args.ga_len;
	if (ai < 0)
	{
	    /* named argument a:name */
	    name = FUNCARG(fp, i);
	    if (islambda)
		addlocal = TRUE;
	}
	else
	{
	    /* "..." argument a:1, a:2, etc. */
	    sprintf((char *)numbuf, "%d", ai + 1);
	    name = numbuf;
	}
	if (fixvar_idx < FIXVAR_CNT && STRLEN(name) <= VAR_SHORT_LEN)
	{
	    v = &fc->fixvar[fixvar_idx++].var;
	    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
	}
	else
	{
	    v = (dictitem_T *)alloc((unsigned)(sizeof(dictitem_T)
							     + STRLEN(name)));
	    if (v == NULL)
		break;
	    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX | DI_FLAGS_ALLOC;
	}
	STRCPY(v->di_key, name);

	/* Note: the values are copied directly to avoid alloc/free.
	 * "argvars" must have VAR_FIXED for v_lock. */
	v->di_tv = argvars[i];
	v->di_tv.v_lock = VAR_FIXED;

	if (addlocal)
	{
	    /* Named arguments should be accessed without the "a:" prefix in
	     * lambda expressions.  Add to the l: dict. */
	    copy_tv(&v->di_tv, &v->di_tv);
	    hash_add(&fc->l_vars.dv_hashtab, DI2HIKEY(v));
	}
	else
	    hash_add(&fc->l_avars.dv_hashtab, DI2HIKEY(v));

	if (ai >= 0 && ai < MAX_FUNC_ARGS)
	{
	    list_append(&fc->l_varlist, &fc->l_listitems[ai]);
	    fc->l_listitems[ai].li_tv = argvars[i];
	    fc->l_listitems[ai].li_tv.v_lock = VAR_FIXED;
	}
    }

    /* Don't redraw while executing the function. */
    ++RedrawingDisabled;
    save_sourcing_name = sourcing_name;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 1;
    /* need space for function name + ("function " + 3) or "[number]" */
    len = (save_sourcing_name == NULL ? 0 : STRLEN(save_sourcing_name))
						   + STRLEN(fp->uf_name) + 20;
    sourcing_name = alloc((unsigned)len);
    if (sourcing_name != NULL)
    {
	if (save_sourcing_name != NULL
			  && STRNCMP(save_sourcing_name, "function ", 9) == 0)
	    sprintf((char *)sourcing_name, "%s[%d]..",
				 save_sourcing_name, (int)save_sourcing_lnum);
	else
	    STRCPY(sourcing_name, "function ");
	cat_func_name(sourcing_name + STRLEN(sourcing_name), fp);

	if (p_verbose >= 12)
	{
	    ++no_wait_return;
	    verbose_enter_scroll();

	    smsg((char_u *)_("calling %s"), sourcing_name);
	    if (p_verbose >= 14)
	    {
		char_u	buf[MSG_BUF_LEN];
		char_u	numbuf2[NUMBUFLEN];
		char_u	*tofree;
		char_u	*s;

		msg_puts((char_u *)"(");
		for (i = 0; i < argcount; ++i)
		{
		    if (i > 0)
			msg_puts((char_u *)", ");
		    if (argvars[i].v_type == VAR_NUMBER)
			msg_outnum((long)argvars[i].vval.v_number);
		    else
		    {
			/* Do not want errors such as E724 here. */
			++emsg_off;
			s = tv2string(&argvars[i], &tofree, numbuf2, 0);
			--emsg_off;
			if (s != NULL)
			{
			    if (vim_strsize(s) > MSG_BUF_CLEN)
			    {
				trunc_string(s, buf, MSG_BUF_CLEN, MSG_BUF_LEN);
				s = buf;
			    }
			    msg_puts(s);
			    vim_free(tofree);
			}
		    }
		}
		msg_puts((char_u *)")");
	    }
	    msg_puts((char_u *)"\n");   /* don't overwrite this either */

	    verbose_leave_scroll();
	    --no_wait_return;
	}
    }
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
    {
	if (!fp->uf_profiling && has_profiling(FALSE, fp->uf_name, NULL))
	{
	    started_profiling = TRUE;
	    func_do_profile(fp);
	}
	if (fp->uf_profiling
		    || (fc->caller != NULL && fc->caller->func->uf_profiling))
	{
	    ++fp->uf_tm_count;
	    profile_start(&call_start);
	    profile_zero(&fp->uf_tm_children);
	}
	script_prof_save(&wait_start);
    }
#endif

    save_current_SID = current_SID;
    current_SID = fp->uf_script_ID;
    save_did_emsg = did_emsg;
    did_emsg = FALSE;

    /* call do_cmdline() to execute the lines */
    do_cmdline(NULL, get_func_line, (void *)fc,
				     DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT);

    --RedrawingDisabled;

    /* when the function was aborted because of an error, return -1 */
    if ((did_emsg && (fp->uf_flags & FC_ABORT)) || rettv->v_type == VAR_UNKNOWN)
    {
	clear_tv(rettv);
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = -1;
    }

#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES && (fp->uf_profiling
		    || (fc->caller != NULL && fc->caller->func->uf_profiling)))
    {
	profile_end(&call_start);
	profile_sub_wait(&wait_start, &call_start);
	profile_add(&fp->uf_tm_total, &call_start);
	profile_self(&fp->uf_tm_self, &call_start, &fp->uf_tm_children);
	if (fc->caller != NULL && fc->caller->func->uf_profiling)
	{
	    profile_add(&fc->caller->func->uf_tm_children, &call_start);
	    profile_add(&fc->caller->func->uf_tml_children, &call_start);
	}
	if (started_profiling)
	    // make a ":profdel func" stop profiling the function
	    fp->uf_profiling = FALSE;
    }
#endif

    /* when being verbose, mention the return value */
    if (p_verbose >= 12)
    {
	++no_wait_return;
	verbose_enter_scroll();

	if (aborting())
	    smsg((char_u *)_("%s aborted"), sourcing_name);
	else if (fc->rettv->v_type == VAR_NUMBER)
	    smsg((char_u *)_("%s returning #%ld"), sourcing_name,
					       (long)fc->rettv->vval.v_number);
	else
	{
	    char_u	buf[MSG_BUF_LEN];
	    char_u	numbuf2[NUMBUFLEN];
	    char_u	*tofree;
	    char_u	*s;

	    /* The value may be very long.  Skip the middle part, so that we
	     * have some idea how it starts and ends. smsg() would always
	     * truncate it at the end. Don't want errors such as E724 here. */
	    ++emsg_off;
	    s = tv2string(fc->rettv, &tofree, numbuf2, 0);
	    --emsg_off;
	    if (s != NULL)
	    {
		if (vim_strsize(s) > MSG_BUF_CLEN)
		{
		    trunc_string(s, buf, MSG_BUF_CLEN, MSG_BUF_LEN);
		    s = buf;
		}
		smsg((char_u *)_("%s returning %s"), sourcing_name, s);
		vim_free(tofree);
	    }
	}
	msg_puts((char_u *)"\n");   /* don't overwrite this either */

	verbose_leave_scroll();
	--no_wait_return;
    }

    vim_free(sourcing_name);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    current_SID = save_current_SID;
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	script_prof_restore(&wait_start);
#endif

    if (p_verbose >= 12 && sourcing_name != NULL)
    {
	++no_wait_return;
	verbose_enter_scroll();

	smsg((char_u *)_("continuing in %s"), sourcing_name);
	msg_puts((char_u *)"\n");   /* don't overwrite this either */

	verbose_leave_scroll();
	--no_wait_return;
    }

    did_emsg |= save_did_emsg;
    --depth;

    cleanup_function_call(fc);
}

/*
 * Unreference "fc": decrement the reference count and free it when it
 * becomes zero.  "fp" is detached from "fc".
 * When "force" is TRUE we are exiting.
 */
    static void
funccal_unref(funccall_T *fc, ufunc_T *fp, int force)
{
    funccall_T	**pfc;
    int		i;

    if (fc == NULL)
	return;

    if (--fc->fc_refcount <= 0 && (force || (
		fc->l_varlist.lv_refcount == DO_NOT_FREE_CNT
		&& fc->l_vars.dv_refcount == DO_NOT_FREE_CNT
		&& fc->l_avars.dv_refcount == DO_NOT_FREE_CNT)))
	for (pfc = &previous_funccal; *pfc != NULL; pfc = &(*pfc)->caller)
	{
	    if (fc == *pfc)
	    {
		*pfc = fc->caller;
		free_funccal(fc, TRUE);
		return;
	    }
	}
    for (i = 0; i < fc->fc_funcs.ga_len; ++i)
	if (((ufunc_T **)(fc->fc_funcs.ga_data))[i] == fp)
	    ((ufunc_T **)(fc->fc_funcs.ga_data))[i] = NULL;
}

/*
 * Remove the function from the function hashtable.  If the function was
 * deleted while it still has references this was already done.
 * Return TRUE if the entry was deleted, FALSE if it wasn't found.
 */
    static int
func_remove(ufunc_T *fp)
{
    hashitem_T	*hi = hash_find(&func_hashtab, UF2HIKEY(fp));

    if (!HASHITEM_EMPTY(hi))
    {
	hash_remove(&func_hashtab, hi);
	return TRUE;
    }
    return FALSE;
}

/*
 * Free all things that a function contains.  Does not free the function
 * itself, use func_free() for that.
 * When "force" is TRUE we are exiting.
 */
    static void
func_clear(ufunc_T *fp, int force)
{
    if (fp->uf_cleared)
	return;
    fp->uf_cleared = TRUE;

    /* clear this function */
    ga_clear_strings(&(fp->uf_args));
    ga_clear_strings(&(fp->uf_lines));
#ifdef FEAT_PROFILE
    vim_free(fp->uf_tml_count);
    vim_free(fp->uf_tml_total);
    vim_free(fp->uf_tml_self);
#endif
    funccal_unref(fp->uf_scoped, fp, force);
}

/*
 * Free a function and remove it from the list of functions.  Does not free
 * what a function contains, call func_clear() first.
 */
    static void
func_free(ufunc_T *fp)
{
    /* only remove it when not done already, otherwise we would remove a newer
     * version of the function */
    if ((fp->uf_flags & (FC_DELETED | FC_REMOVED)) == 0)
	func_remove(fp);

    vim_free(fp);
}

/*
 * Free all things that a function contains and free the function itself.
 * When "force" is TRUE we are exiting.
 */
    static void
func_clear_free(ufunc_T *fp, int force)
{
    func_clear(fp, force);
    func_free(fp);
}

/*
 * There are two kinds of function names:
 * 1. ordinary names, function defined with :function
 * 2. numbered functions and lambdas
 * For the first we only count the name stored in func_hashtab as a reference,
 * using function() does not count as a reference, because the function is
 * looked up by name.
 */
    static int
func_name_refcount(char_u *name)
{
    return isdigit(*name) || *name == '<';
}

#if defined(EXITFREE) || defined(PROTO)
    void
free_all_functions(void)
{
    hashitem_T	*hi;
    ufunc_T	*fp;
    long_u	skipped = 0;
    long_u	todo = 1;
    long_u	used;

    /* Clean up the call stack. */
    while (current_funccal != NULL)
    {
	clear_tv(current_funccal->rettv);
	cleanup_function_call(current_funccal);
    }

    /* First clear what the functions contain.  Since this may lower the
     * reference count of a function, it may also free a function and change
     * the hash table. Restart if that happens. */
    while (todo > 0)
    {
	todo = func_hashtab.ht_used;
	for (hi = func_hashtab.ht_array; todo > 0; ++hi)
	    if (!HASHITEM_EMPTY(hi))
	    {
		/* Only free functions that are not refcounted, those are
		 * supposed to be freed when no longer referenced. */
		fp = HI2UF(hi);
		if (func_name_refcount(fp->uf_name))
		    ++skipped;
		else
		{
		    used = func_hashtab.ht_used;
		    func_clear(fp, TRUE);
		    if (used != func_hashtab.ht_used)
		    {
			skipped = 0;
			break;
		    }
		}
		--todo;
	    }
    }

    /* Now actually free the functions.  Need to start all over every time,
     * because func_free() may change the hash table. */
    skipped = 0;
    while (func_hashtab.ht_used > skipped)
    {
	todo = func_hashtab.ht_used;
	for (hi = func_hashtab.ht_array; todo > 0; ++hi)
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		/* Only free functions that are not refcounted, those are
		 * supposed to be freed when no longer referenced. */
		fp = HI2UF(hi);
		if (func_name_refcount(fp->uf_name))
		    ++skipped;
		else
		{
		    func_free(fp);
		    skipped = 0;
		    break;
		}
	    }
    }
    if (skipped == 0)
	hash_clear(&func_hashtab);
}
#endif

/*
 * Return TRUE if "name" looks like a builtin function name: starts with a
 * lower case letter and doesn't contain AUTOLOAD_CHAR.
 * "len" is the length of "name", or -1 for NUL terminated.
 */
    static int
builtin_function(char_u *name, int len)
{
    char_u *p;

    if (!ASCII_ISLOWER(name[0]))
	return FALSE;
    p = vim_strchr(name, AUTOLOAD_CHAR);
    return p == NULL || (len > 0 && p > name + len);
}

    int
func_call(
    char_u	*name,
    typval_T	*args,
    partial_T	*partial,
    dict_T	*selfdict,
    typval_T	*rettv)
{
    listitem_T	*item;
    typval_T	argv[MAX_FUNC_ARGS + 1];
    int		argc = 0;
    int		dummy;
    int		r = 0;

    for (item = args->vval.v_list->lv_first; item != NULL;
							 item = item->li_next)
    {
	if (argc == MAX_FUNC_ARGS - (partial == NULL ? 0 : partial->pt_argc))
	{
	    EMSG(_("E699: Too many arguments"));
	    break;
	}
	/* Make a copy of each argument.  This is needed to be able to set
	 * v_lock to VAR_FIXED in the copy without changing the original list.
	 */
	copy_tv(&item->li_tv, &argv[argc++]);
    }

    if (item == NULL)
	r = call_func(name, (int)STRLEN(name), rettv, argc, argv, NULL,
				 curwin->w_cursor.lnum, curwin->w_cursor.lnum,
					     &dummy, TRUE, partial, selfdict);

    /* Free the arguments. */
    while (argc > 0)
	clear_tv(&argv[--argc]);

    return r;
}

/*
 * Call a function with its resolved parameters
 *
 * "argv_func", when not NULL, can be used to fill in arguments only when the
 * invoked function uses them.  It is called like this:
 *   new_argcount = argv_func(current_argcount, argv, called_func_argcount)
 *
 * Return FAIL when the function can't be called,  OK otherwise.
 * Also returns OK when an error was encountered while executing the function.
 */
    int
call_func(
    char_u	*funcname,	/* name of the function */
    int		len,		/* length of "name" */
    typval_T	*rettv,		/* return value goes here */
    int		argcount_in,	/* number of "argvars" */
    typval_T	*argvars_in,	/* vars for arguments, must have "argcount"
				   PLUS ONE elements! */
    int		(* argv_func)(int, typval_T *, int),
				/* function to fill in argvars */
    linenr_T	firstline,	/* first line of range */
    linenr_T	lastline,	/* last line of range */
    int		*doesrange,	/* return: function handled range */
    int		evaluate,
    partial_T	*partial,	/* optional, can be NULL */
    dict_T	*selfdict_in)	/* Dictionary for "self" */
{
    int		ret = FAIL;
    int		error = ERROR_NONE;
    int		i;
    ufunc_T	*fp;
    char_u	fname_buf[FLEN_FIXED + 1];
    char_u	*tofree = NULL;
    char_u	*fname;
    char_u	*name;
    int		argcount = argcount_in;
    typval_T	*argvars = argvars_in;
    dict_T	*selfdict = selfdict_in;
    typval_T	argv[MAX_FUNC_ARGS + 1]; /* used when "partial" is not NULL */
    int		argv_clear = 0;

    /* Make a copy of the name, if it comes from a funcref variable it could
     * be changed or deleted in the called function. */
    name = vim_strnsave(funcname, len);
    if (name == NULL)
	return ret;

    fname = fname_trans_sid(name, fname_buf, &tofree, &error);

    *doesrange = FALSE;

    if (partial != NULL)
    {
	/* When the function has a partial with a dict and there is a dict
	 * argument, use the dict argument.  That is backwards compatible.
	 * When the dict was bound explicitly use the one from the partial. */
	if (partial->pt_dict != NULL
		&& (selfdict_in == NULL || !partial->pt_auto))
	    selfdict = partial->pt_dict;
	if (error == ERROR_NONE && partial->pt_argc > 0)
	{
	    for (argv_clear = 0; argv_clear < partial->pt_argc; ++argv_clear)
		copy_tv(&partial->pt_argv[argv_clear], &argv[argv_clear]);
	    for (i = 0; i < argcount_in; ++i)
		argv[i + argv_clear] = argvars_in[i];
	    argvars = argv;
	    argcount = partial->pt_argc + argcount_in;
	}
    }


    /*
     * Execute the function if executing and no errors were detected.
     */
    if (!evaluate)
    {
	// Not evaluating, which means the return value is unknown.  This
	// matters for giving error messages.
	rettv->v_type = VAR_UNKNOWN;
    }
    else if (error == ERROR_NONE)
    {
	char_u *rfname = fname;

	/* Ignore "g:" before a function name. */
	if (fname[0] == 'g' && fname[1] == ':')
	    rfname = fname + 2;

	rettv->v_type = VAR_NUMBER;	/* default rettv is number zero */
	rettv->vval.v_number = 0;
	error = ERROR_UNKNOWN;

	if (!builtin_function(rfname, -1))
	{
	    /*
	     * User defined function.
	     */
	    if (partial != NULL && partial->pt_func != NULL)
		fp = partial->pt_func;
	    else
		fp = find_func(rfname);

	    /* Trigger FuncUndefined event, may load the function. */
	    if (fp == NULL
		    && apply_autocmds(EVENT_FUNCUNDEFINED,
						     rfname, rfname, TRUE, NULL)
		    && !aborting())
	    {
		/* executed an autocommand, search for the function again */
		fp = find_func(rfname);
	    }
	    /* Try loading a package. */
	    if (fp == NULL && script_autoload(rfname, TRUE) && !aborting())
	    {
		/* loaded a package, search for the function again */
		fp = find_func(rfname);
	    }

	    if (fp != NULL && (fp->uf_flags & FC_DELETED))
		error = ERROR_DELETED;
	    else if (fp != NULL)
	    {
		if (argv_func != NULL)
		    argcount = argv_func(argcount, argvars, fp->uf_args.ga_len);

		if (fp->uf_flags & FC_RANGE)
		    *doesrange = TRUE;
		if (argcount < fp->uf_args.ga_len)
		    error = ERROR_TOOFEW;
		else if (!fp->uf_varargs && argcount > fp->uf_args.ga_len)
		    error = ERROR_TOOMANY;
		else if ((fp->uf_flags & FC_DICT) && selfdict == NULL)
		    error = ERROR_DICT;
		else
		{
		    int did_save_redo = FALSE;
		    save_redo_T	save_redo;

		    /*
		     * Call the user function.
		     * Save and restore search patterns, script variables and
		     * redo buffer.
		     */
		    save_search_patterns();
#ifdef FEAT_INS_EXPAND
		    if (!ins_compl_active())
#endif
		    {
			saveRedobuff(&save_redo);
			did_save_redo = TRUE;
		    }
		    ++fp->uf_calls;
		    call_user_func(fp, argcount, argvars, rettv,
					       firstline, lastline,
				  (fp->uf_flags & FC_DICT) ? selfdict : NULL);
		    if (--fp->uf_calls <= 0 && fp->uf_refcount <= 0)
			/* Function was unreferenced while being used, free it
			 * now. */
			func_clear_free(fp, FALSE);
		    if (did_save_redo)
			restoreRedobuff(&save_redo);
		    restore_search_patterns();
		    error = ERROR_NONE;
		}
	    }
	}
	else
	{
	    /*
	     * Find the function name in the table, call its implementation.
	     */
	    error = call_internal_func(fname, argcount, argvars, rettv);
	}
	/*
	 * The function call (or "FuncUndefined" autocommand sequence) might
	 * have been aborted by an error, an interrupt, or an explicitly thrown
	 * exception that has not been caught so far.  This situation can be
	 * tested for by calling aborting().  For an error in an internal
	 * function or for the "E132" error in call_user_func(), however, the
	 * throw point at which the "force_abort" flag (temporarily reset by
	 * emsg()) is normally updated has not been reached yet. We need to
	 * update that flag first to make aborting() reliable.
	 */
	update_force_abort();
    }
    if (error == ERROR_NONE)
	ret = OK;

    /*
     * Report an error unless the argument evaluation or function call has been
     * cancelled due to an aborting error, an interrupt, or an exception.
     */
    if (!aborting())
    {
	switch (error)
	{
	    case ERROR_UNKNOWN:
		    emsg_funcname(N_("E117: Unknown function: %s"), name);
		    break;
	    case ERROR_DELETED:
		    emsg_funcname(N_("E933: Function was deleted: %s"), name);
		    break;
	    case ERROR_TOOMANY:
		    emsg_funcname((char *)e_toomanyarg, name);
		    break;
	    case ERROR_TOOFEW:
		    emsg_funcname(N_("E119: Not enough arguments for function: %s"),
									name);
		    break;
	    case ERROR_SCRIPT:
		    emsg_funcname(N_("E120: Using <SID> not in a script context: %s"),
									name);
		    break;
	    case ERROR_DICT:
		    emsg_funcname(N_("E725: Calling dict function without Dictionary: %s"),
									name);
		    break;
	}
    }

    while (argv_clear > 0)
	clear_tv(&argv[--argv_clear]);
    vim_free(tofree);
    vim_free(name);

    return ret;
}

/*
 * List the head of the function: "name(arg1, arg2)".
 */
    static void
list_func_head(ufunc_T *fp, int indent)
{
    int		j;

    msg_start();
    if (indent)
	MSG_PUTS("   ");
    MSG_PUTS("function ");
    if (fp->uf_name[0] == K_SPECIAL)
    {
	MSG_PUTS_ATTR("<SNR>", HL_ATTR(HLF_8));
	msg_puts(fp->uf_name + 3);
    }
    else
	msg_puts(fp->uf_name);
    msg_putchar('(');
    for (j = 0; j < fp->uf_args.ga_len; ++j)
    {
	if (j)
	    MSG_PUTS(", ");
	msg_puts(FUNCARG(fp, j));
    }
    if (fp->uf_varargs)
    {
	if (j)
	    MSG_PUTS(", ");
	MSG_PUTS("...");
    }
    msg_putchar(')');
    if (fp->uf_flags & FC_ABORT)
	MSG_PUTS(" abort");
    if (fp->uf_flags & FC_RANGE)
	MSG_PUTS(" range");
    if (fp->uf_flags & FC_DICT)
	MSG_PUTS(" dict");
    if (fp->uf_flags & FC_CLOSURE)
	MSG_PUTS(" closure");
    msg_clr_eos();
    if (p_verbose > 0)
	last_set_msg(fp->uf_script_ID);
}

/*
 * Get a function name, translating "<SID>" and "<SNR>".
 * Also handles a Funcref in a List or Dictionary.
 * Returns the function name in allocated memory, or NULL for failure.
 * flags:
 * TFN_INT:	    internal function name OK
 * TFN_QUIET:	    be quiet
 * TFN_NO_AUTOLOAD: do not use script autoloading
 * TFN_NO_DEREF:    do not dereference a Funcref
 * Advances "pp" to just after the function name (if no error).
 */
    char_u *
trans_function_name(
    char_u	**pp,
    int		skip,		/* only find the end, don't evaluate */
    int		flags,
    funcdict_T	*fdp,		/* return: info about dictionary used */
    partial_T	**partial)	/* return: partial of a FuncRef */
{
    char_u	*name = NULL;
    char_u	*start;
    char_u	*end;
    int		lead;
    char_u	sid_buf[20];
    int		len;
    lval_T	lv;

    if (fdp != NULL)
	vim_memset(fdp, 0, sizeof(funcdict_T));
    start = *pp;

    /* Check for hard coded <SNR>: already translated function ID (from a user
     * command). */
    if ((*pp)[0] == K_SPECIAL && (*pp)[1] == KS_EXTRA
						   && (*pp)[2] == (int)KE_SNR)
    {
	*pp += 3;
	len = get_id_len(pp) + 3;
	return vim_strnsave(start, len);
    }

    /* A name starting with "<SID>" or "<SNR>" is local to a script.  But
     * don't skip over "s:", get_lval() needs it for "s:dict.func". */
    lead = eval_fname_script(start);
    if (lead > 2)
	start += lead;

    /* Note that TFN_ flags use the same values as GLV_ flags. */
    end = get_lval(start, NULL, &lv, FALSE, skip, flags | GLV_READ_ONLY,
					      lead > 2 ? 0 : FNE_CHECK_START);
    if (end == start)
    {
	if (!skip)
	    EMSG(_("E129: Function name required"));
	goto theend;
    }
    if (end == NULL || (lv.ll_tv != NULL && (lead > 2 || lv.ll_range)))
    {
	/*
	 * Report an invalid expression in braces, unless the expression
	 * evaluation has been cancelled due to an aborting error, an
	 * interrupt, or an exception.
	 */
	if (!aborting())
	{
	    if (end != NULL)
		EMSG2(_(e_invarg2), start);
	}
	else
	    *pp = find_name_end(start, NULL, NULL, FNE_INCL_BR);
	goto theend;
    }

    if (lv.ll_tv != NULL)
    {
	if (fdp != NULL)
	{
	    fdp->fd_dict = lv.ll_dict;
	    fdp->fd_newkey = lv.ll_newkey;
	    lv.ll_newkey = NULL;
	    fdp->fd_di = lv.ll_di;
	}
	if (lv.ll_tv->v_type == VAR_FUNC && lv.ll_tv->vval.v_string != NULL)
	{
	    name = vim_strsave(lv.ll_tv->vval.v_string);
	    *pp = end;
	}
	else if (lv.ll_tv->v_type == VAR_PARTIAL
					  && lv.ll_tv->vval.v_partial != NULL)
	{
	    name = vim_strsave(partial_name(lv.ll_tv->vval.v_partial));
	    *pp = end;
	    if (partial != NULL)
		*partial = lv.ll_tv->vval.v_partial;
	}
	else
	{
	    if (!skip && !(flags & TFN_QUIET) && (fdp == NULL
			     || lv.ll_dict == NULL || fdp->fd_newkey == NULL))
		EMSG(_(e_funcref));
	    else
		*pp = end;
	    name = NULL;
	}
	goto theend;
    }

    if (lv.ll_name == NULL)
    {
	/* Error found, but continue after the function name. */
	*pp = end;
	goto theend;
    }

    /* Check if the name is a Funcref.  If so, use the value. */
    if (lv.ll_exp_name != NULL)
    {
	len = (int)STRLEN(lv.ll_exp_name);
	name = deref_func_name(lv.ll_exp_name, &len, partial,
						     flags & TFN_NO_AUTOLOAD);
	if (name == lv.ll_exp_name)
	    name = NULL;
    }
    else if (!(flags & TFN_NO_DEREF))
    {
	len = (int)(end - *pp);
	name = deref_func_name(*pp, &len, partial, flags & TFN_NO_AUTOLOAD);
	if (name == *pp)
	    name = NULL;
    }
    if (name != NULL)
    {
	name = vim_strsave(name);
	*pp = end;
	if (STRNCMP(name, "<SNR>", 5) == 0)
	{
	    /* Change "<SNR>" to the byte sequence. */
	    name[0] = K_SPECIAL;
	    name[1] = KS_EXTRA;
	    name[2] = (int)KE_SNR;
	    mch_memmove(name + 3, name + 5, STRLEN(name + 5) + 1);
	}
	goto theend;
    }

    if (lv.ll_exp_name != NULL)
    {
	len = (int)STRLEN(lv.ll_exp_name);
	if (lead <= 2 && lv.ll_name == lv.ll_exp_name
					 && STRNCMP(lv.ll_name, "s:", 2) == 0)
	{
	    /* When there was "s:" already or the name expanded to get a
	     * leading "s:" then remove it. */
	    lv.ll_name += 2;
	    len -= 2;
	    lead = 2;
	}
    }
    else
    {
	/* skip over "s:" and "g:" */
	if (lead == 2 || (lv.ll_name[0] == 'g' && lv.ll_name[1] == ':'))
	    lv.ll_name += 2;
	len = (int)(end - lv.ll_name);
    }

    /*
     * Copy the function name to allocated memory.
     * Accept <SID>name() inside a script, translate into <SNR>123_name().
     * Accept <SNR>123_name() outside a script.
     */
    if (skip)
	lead = 0;	/* do nothing */
    else if (lead > 0)
    {
	lead = 3;
	if ((lv.ll_exp_name != NULL && eval_fname_sid(lv.ll_exp_name))
						       || eval_fname_sid(*pp))
	{
	    /* It's "s:" or "<SID>" */
	    if (current_SID <= 0)
	    {
		EMSG(_(e_usingsid));
		goto theend;
	    }
	    sprintf((char *)sid_buf, "%ld_", (long)current_SID);
	    lead += (int)STRLEN(sid_buf);
	}
    }
    else if (!(flags & TFN_INT) && builtin_function(lv.ll_name, len))
    {
	EMSG2(_("E128: Function name must start with a capital or \"s:\": %s"),
								       start);
	goto theend;
    }
    if (!skip && !(flags & TFN_QUIET) && !(flags & TFN_NO_DEREF))
    {
	char_u *cp = vim_strchr(lv.ll_name, ':');

	if (cp != NULL && cp < end)
	{
	    EMSG2(_("E884: Function name cannot contain a colon: %s"), start);
	    goto theend;
	}
    }

    name = alloc((unsigned)(len + lead + 1));
    if (name != NULL)
    {
	if (lead > 0)
	{
	    name[0] = K_SPECIAL;
	    name[1] = KS_EXTRA;
	    name[2] = (int)KE_SNR;
	    if (lead > 3)	/* If it's "<SID>" */
		STRCPY(name + 3, sid_buf);
	}
	mch_memmove(name + lead, lv.ll_name, (size_t)len);
	name[lead + len] = NUL;
    }
    *pp = end;

theend:
    clear_lval(&lv);
    return name;
}

/*
 * ":function"
 */
    void
ex_function(exarg_T *eap)
{
    char_u	*theline;
    char_u	*line_to_free = NULL;
    int		j;
    int		c;
    int		saved_did_emsg;
    int		saved_wait_return = need_wait_return;
    char_u	*name = NULL;
    char_u	*p;
    char_u	*arg;
    char_u	*line_arg = NULL;
    garray_T	newargs;
    garray_T	newlines;
    int		varargs = FALSE;
    int		flags = 0;
    ufunc_T	*fp;
    int		overwrite = FALSE;
    int		indent;
    int		nesting;
    char_u	*skip_until = NULL;
    dictitem_T	*v;
    funcdict_T	fudi;
    static int	func_nr = 0;	    /* number for nameless function */
    int		paren;
    hashtab_T	*ht;
    int		todo;
    hashitem_T	*hi;
    int		sourcing_lnum_off;

    /*
     * ":function" without argument: list functions.
     */
    if (ends_excmd(*eap->arg))
    {
	if (!eap->skip)
	{
	    todo = (int)func_hashtab.ht_used;
	    for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;
		    fp = HI2UF(hi);
		    if (!func_name_refcount(fp->uf_name))
			list_func_head(fp, FALSE);
		}
	    }
	}
	eap->nextcmd = check_nextcmd(eap->arg);
	return;
    }

    /*
     * ":function /pat": list functions matching pattern.
     */
    if (*eap->arg == '/')
    {
	p = skip_regexp(eap->arg + 1, '/', TRUE, NULL);
	if (!eap->skip)
	{
	    regmatch_T	regmatch;

	    c = *p;
	    *p = NUL;
	    regmatch.regprog = vim_regcomp(eap->arg + 1, RE_MAGIC);
	    *p = c;
	    if (regmatch.regprog != NULL)
	    {
		regmatch.rm_ic = p_ic;

		todo = (int)func_hashtab.ht_used;
		for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
		{
		    if (!HASHITEM_EMPTY(hi))
		    {
			--todo;
			fp = HI2UF(hi);
			if (!isdigit(*fp->uf_name)
				    && vim_regexec(&regmatch, fp->uf_name, 0))
			    list_func_head(fp, FALSE);
		    }
		}
		vim_regfree(regmatch.regprog);
	    }
	}
	if (*p == '/')
	    ++p;
	eap->nextcmd = check_nextcmd(p);
	return;
    }

    /*
     * Get the function name.  There are these situations:
     * func	    normal function name
     *		    "name" == func, "fudi.fd_dict" == NULL
     * dict.func    new dictionary entry
     *		    "name" == NULL, "fudi.fd_dict" set,
     *		    "fudi.fd_di" == NULL, "fudi.fd_newkey" == func
     * dict.func    existing dict entry with a Funcref
     *		    "name" == func, "fudi.fd_dict" set,
     *		    "fudi.fd_di" set, "fudi.fd_newkey" == NULL
     * dict.func    existing dict entry that's not a Funcref
     *		    "name" == NULL, "fudi.fd_dict" set,
     *		    "fudi.fd_di" set, "fudi.fd_newkey" == NULL
     * s:func	    script-local function name
     * g:func	    global function name, same as "func"
     */
    p = eap->arg;
    name = trans_function_name(&p, eap->skip, TFN_NO_AUTOLOAD, &fudi, NULL);
    paren = (vim_strchr(p, '(') != NULL);
    if (name == NULL && (fudi.fd_dict == NULL || !paren) && !eap->skip)
    {
	/*
	 * Return on an invalid expression in braces, unless the expression
	 * evaluation has been cancelled due to an aborting error, an
	 * interrupt, or an exception.
	 */
	if (!aborting())
	{
	    if (!eap->skip && fudi.fd_newkey != NULL)
		EMSG2(_(e_dictkey), fudi.fd_newkey);
	    vim_free(fudi.fd_newkey);
	    return;
	}
	else
	    eap->skip = TRUE;
    }

    /* An error in a function call during evaluation of an expression in magic
     * braces should not cause the function not to be defined. */
    saved_did_emsg = did_emsg;
    did_emsg = FALSE;

    /*
     * ":function func" with only function name: list function.
     */
    if (!paren)
    {
	if (!ends_excmd(*skipwhite(p)))
	{
	    EMSG(_(e_trailing));
	    goto ret_free;
	}
	eap->nextcmd = check_nextcmd(p);
	if (eap->nextcmd != NULL)
	    *p = NUL;
	if (!eap->skip && !got_int)
	{
	    fp = find_func(name);
	    if (fp != NULL)
	    {
		list_func_head(fp, TRUE);
		for (j = 0; j < fp->uf_lines.ga_len && !got_int; ++j)
		{
		    if (FUNCLINE(fp, j) == NULL)
			continue;
		    msg_putchar('\n');
		    msg_outnum((long)(j + 1));
		    if (j < 9)
			msg_putchar(' ');
		    if (j < 99)
			msg_putchar(' ');
		    msg_prt_line(FUNCLINE(fp, j), FALSE);
		    out_flush();	/* show a line at a time */
		    ui_breakcheck();
		}
		if (!got_int)
		{
		    msg_putchar('\n');
		    msg_puts((char_u *)"   endfunction");
		}
	    }
	    else
		emsg_funcname(N_("E123: Undefined function: %s"), name);
	}
	goto ret_free;
    }

    /*
     * ":function name(arg1, arg2)" Define function.
     */
    p = skipwhite(p);
    if (*p != '(')
    {
	if (!eap->skip)
	{
	    EMSG2(_("E124: Missing '(': %s"), eap->arg);
	    goto ret_free;
	}
	/* attempt to continue by skipping some text */
	if (vim_strchr(p, '(') != NULL)
	    p = vim_strchr(p, '(');
    }
    p = skipwhite(p + 1);

    ga_init2(&newlines, (int)sizeof(char_u *), 3);

    if (!eap->skip)
    {
	/* Check the name of the function.  Unless it's a dictionary function
	 * (that we are overwriting). */
	if (name != NULL)
	    arg = name;
	else
	    arg = fudi.fd_newkey;
	if (arg != NULL && (fudi.fd_di == NULL
				     || (fudi.fd_di->di_tv.v_type != VAR_FUNC
				 && fudi.fd_di->di_tv.v_type != VAR_PARTIAL)))
	{
	    if (*arg == K_SPECIAL)
		j = 3;
	    else
		j = 0;
	    while (arg[j] != NUL && (j == 0 ? eval_isnamec1(arg[j])
						      : eval_isnamec(arg[j])))
		++j;
	    if (arg[j] != NUL)
		emsg_funcname((char *)e_invarg2, arg);
	}
	/* Disallow using the g: dict. */
	if (fudi.fd_dict != NULL && fudi.fd_dict->dv_scope == VAR_DEF_SCOPE)
	    EMSG(_("E862: Cannot use g: here"));
    }

    if (get_function_args(&p, ')', &newargs, &varargs, eap->skip) == FAIL)
	goto errret_2;

    /* find extra arguments "range", "dict", "abort" and "closure" */
    for (;;)
    {
	p = skipwhite(p);
	if (STRNCMP(p, "range", 5) == 0)
	{
	    flags |= FC_RANGE;
	    p += 5;
	}
	else if (STRNCMP(p, "dict", 4) == 0)
	{
	    flags |= FC_DICT;
	    p += 4;
	}
	else if (STRNCMP(p, "abort", 5) == 0)
	{
	    flags |= FC_ABORT;
	    p += 5;
	}
	else if (STRNCMP(p, "closure", 7) == 0)
	{
	    flags |= FC_CLOSURE;
	    p += 7;
	    if (current_funccal == NULL)
	    {
		emsg_funcname(N_("E932: Closure function should not be at top level: %s"),
			name == NULL ? (char_u *)"" : name);
		goto erret;
	    }
	}
	else
	    break;
    }

    /* When there is a line break use what follows for the function body.
     * Makes 'exe "func Test()\n...\nendfunc"' work. */
    if (*p == '\n')
	line_arg = p + 1;
    else if (*p != NUL && *p != '"' && !eap->skip && !did_emsg)
	EMSG(_(e_trailing));

    /*
     * Read the body of the function, until ":endfunction" is found.
     */
    if (KeyTyped)
    {
	/* Check if the function already exists, don't let the user type the
	 * whole function before telling him it doesn't work!  For a script we
	 * need to skip the body to be able to find what follows. */
	if (!eap->skip && !eap->forceit)
	{
	    if (fudi.fd_dict != NULL && fudi.fd_newkey == NULL)
		EMSG(_(e_funcdict));
	    else if (name != NULL && find_func(name) != NULL)
		emsg_funcname(e_funcexts, name);
	}

	if (!eap->skip && did_emsg)
	    goto erret;

	msg_putchar('\n');	    /* don't overwrite the function name */
	cmdline_row = msg_row;
    }

    indent = 2;
    nesting = 0;
    for (;;)
    {
	if (KeyTyped)
	{
	    msg_scroll = TRUE;
	    saved_wait_return = FALSE;
	}
	need_wait_return = FALSE;
	sourcing_lnum_off = sourcing_lnum;

	if (line_arg != NULL)
	{
	    /* Use eap->arg, split up in parts by line breaks. */
	    theline = line_arg;
	    p = vim_strchr(theline, '\n');
	    if (p == NULL)
		line_arg += STRLEN(line_arg);
	    else
	    {
		*p = NUL;
		line_arg = p + 1;
	    }
	}
	else
	{
	    vim_free(line_to_free);
	    if (eap->getline == NULL)
		theline = getcmdline(':', 0L, indent);
	    else
		theline = eap->getline(':', eap->cookie, indent);
	    line_to_free = theline;
	}
	if (KeyTyped)
	    lines_left = Rows - 1;
	if (theline == NULL)
	{
	    EMSG(_("E126: Missing :endfunction"));
	    goto erret;
	}

	/* Detect line continuation: sourcing_lnum increased more than one. */
	if (sourcing_lnum > sourcing_lnum_off + 1)
	    sourcing_lnum_off = sourcing_lnum - sourcing_lnum_off - 1;
	else
	    sourcing_lnum_off = 0;

	if (skip_until != NULL)
	{
	    /* between ":append" and "." and between ":python <<EOF" and "EOF"
	     * don't check for ":endfunc". */
	    if (STRCMP(theline, skip_until) == 0)
		VIM_CLEAR(skip_until);
	}
	else
	{
	    /* skip ':' and blanks*/
	    for (p = theline; VIM_ISWHITE(*p) || *p == ':'; ++p)
		;

	    /* Check for "endfunction". */
	    if (checkforcmd(&p, "endfunction", 4) && nesting-- == 0)
	    {
		char_u *nextcmd = NULL;

		if (*p == '|')
		    nextcmd = p + 1;
		else if (line_arg != NULL && *skipwhite(line_arg) != NUL)
		    nextcmd = line_arg;
		else if (*p != NUL && *p != '"' && p_verbose > 0)
		    give_warning2(
			 (char_u *)_("W22: Text found after :endfunction: %s"),
			 p, TRUE);
		if (nextcmd != NULL)
		{
		    /* Another command follows. If the line came from "eap" we
		     * can simply point into it, otherwise we need to change
		     * "eap->cmdlinep". */
		    eap->nextcmd = nextcmd;
		    if (line_to_free != NULL)
		    {
			vim_free(*eap->cmdlinep);
			*eap->cmdlinep = line_to_free;
			line_to_free = NULL;
		    }
		}
		break;
	    }

	    /* Increase indent inside "if", "while", "for" and "try", decrease
	     * at "end". */
	    if (indent > 2 && STRNCMP(p, "end", 3) == 0)
		indent -= 2;
	    else if (STRNCMP(p, "if", 2) == 0
		    || STRNCMP(p, "wh", 2) == 0
		    || STRNCMP(p, "for", 3) == 0
		    || STRNCMP(p, "try", 3) == 0)
		indent += 2;

	    /* Check for defining a function inside this function. */
	    if (checkforcmd(&p, "function", 2))
	    {
		if (*p == '!')
		    p = skipwhite(p + 1);
		p += eval_fname_script(p);
		vim_free(trans_function_name(&p, TRUE, 0, NULL, NULL));
		if (*skipwhite(p) == '(')
		{
		    ++nesting;
		    indent += 2;
		}
	    }

	    /* Check for ":append", ":change", ":insert". */
	    p = skip_range(p, NULL);
	    if ((p[0] == 'a' && (!ASCII_ISALPHA(p[1]) || p[1] == 'p'))
		    || (p[0] == 'c'
			&& (!ASCII_ISALPHA(p[1]) || (p[1] == 'h'
				&& (!ASCII_ISALPHA(p[2]) || (p[2] == 'a'
					&& (STRNCMP(&p[3], "nge", 3) != 0
					    || !ASCII_ISALPHA(p[6])))))))
		    || (p[0] == 'i'
			&& (!ASCII_ISALPHA(p[1]) || (p[1] == 'n'
				&& (!ASCII_ISALPHA(p[2]) || (p[2] == 's'))))))
		skip_until = vim_strsave((char_u *)".");

	    /* Check for ":python <<EOF", ":tcl <<EOF", etc. */
	    arg = skipwhite(skiptowhite(p));
	    if (arg[0] == '<' && arg[1] =='<'
		    && ((p[0] == 'p' && p[1] == 'y'
				    && (!ASCII_ISALNUM(p[2]) || p[2] == 't'
					|| ((p[2] == '3' || p[2] == 'x')
						   && !ASCII_ISALPHA(p[3]))))
			|| (p[0] == 'p' && p[1] == 'e'
				    && (!ASCII_ISALPHA(p[2]) || p[2] == 'r'))
			|| (p[0] == 't' && p[1] == 'c'
				    && (!ASCII_ISALPHA(p[2]) || p[2] == 'l'))
			|| (p[0] == 'l' && p[1] == 'u' && p[2] == 'a'
				    && !ASCII_ISALPHA(p[3]))
			|| (p[0] == 'r' && p[1] == 'u' && p[2] == 'b'
				    && (!ASCII_ISALPHA(p[3]) || p[3] == 'y'))
			|| (p[0] == 'm' && p[1] == 'z'
				    && (!ASCII_ISALPHA(p[2]) || p[2] == 's'))
			))
	    {
		/* ":python <<" continues until a dot, like ":append" */
		p = skipwhite(arg + 2);
		if (*p == NUL)
		    skip_until = vim_strsave((char_u *)".");
		else
		    skip_until = vim_strsave(p);
	    }
	}

	/* Add the line to the function. */
	if (ga_grow(&newlines, 1 + sourcing_lnum_off) == FAIL)
	    goto erret;

	/* Copy the line to newly allocated memory.  get_one_sourceline()
	 * allocates 250 bytes per line, this saves 80% on average.  The cost
	 * is an extra alloc/free. */
	p = vim_strsave(theline);
	if (p == NULL)
	    goto erret;
	((char_u **)(newlines.ga_data))[newlines.ga_len++] = p;

	/* Add NULL lines for continuation lines, so that the line count is
	 * equal to the index in the growarray.   */
	while (sourcing_lnum_off-- > 0)
	    ((char_u **)(newlines.ga_data))[newlines.ga_len++] = NULL;

	/* Check for end of eap->arg. */
	if (line_arg != NULL && *line_arg == NUL)
	    line_arg = NULL;
    }

    /* Don't define the function when skipping commands or when an error was
     * detected. */
    if (eap->skip || did_emsg)
	goto erret;

    /*
     * If there are no errors, add the function
     */
    if (fudi.fd_dict == NULL)
    {
	v = find_var(name, &ht, FALSE);
	if (v != NULL && v->di_tv.v_type == VAR_FUNC)
	{
	    emsg_funcname(N_("E707: Function name conflicts with variable: %s"),
									name);
	    goto erret;
	}

	fp = find_func(name);
	if (fp != NULL)
	{
	    if (!eap->forceit)
	    {
		emsg_funcname(e_funcexts, name);
		goto erret;
	    }
	    if (fp->uf_calls > 0)
	    {
		emsg_funcname(N_("E127: Cannot redefine function %s: It is in use"),
									name);
		goto erret;
	    }
	    if (fp->uf_refcount > 1)
	    {
		/* This function is referenced somewhere, don't redefine it but
		 * create a new one. */
		--fp->uf_refcount;
		fp->uf_flags |= FC_REMOVED;
		fp = NULL;
		overwrite = TRUE;
	    }
	    else
	    {
		/* redefine existing function */
		ga_clear_strings(&(fp->uf_args));
		ga_clear_strings(&(fp->uf_lines));
		VIM_CLEAR(name);
	    }
	}
    }
    else
    {
	char	numbuf[20];

	fp = NULL;
	if (fudi.fd_newkey == NULL && !eap->forceit)
	{
	    EMSG(_(e_funcdict));
	    goto erret;
	}
	if (fudi.fd_di == NULL)
	{
	    /* Can't add a function to a locked dictionary */
	    if (tv_check_lock(fudi.fd_dict->dv_lock, eap->arg, FALSE))
		goto erret;
	}
	    /* Can't change an existing function if it is locked */
	else if (tv_check_lock(fudi.fd_di->di_tv.v_lock, eap->arg, FALSE))
	    goto erret;

	/* Give the function a sequential number.  Can only be used with a
	 * Funcref! */
	vim_free(name);
	sprintf(numbuf, "%d", ++func_nr);
	name = vim_strsave((char_u *)numbuf);
	if (name == NULL)
	    goto erret;
    }

    if (fp == NULL)
    {
	if (fudi.fd_dict == NULL && vim_strchr(name, AUTOLOAD_CHAR) != NULL)
	{
	    int	    slen, plen;
	    char_u  *scriptname;

	    /* Check that the autoload name matches the script name. */
	    j = FAIL;
	    if (sourcing_name != NULL)
	    {
		scriptname = autoload_name(name);
		if (scriptname != NULL)
		{
		    p = vim_strchr(scriptname, '/');
		    plen = (int)STRLEN(p);
		    slen = (int)STRLEN(sourcing_name);
		    if (slen > plen && fnamecmp(p,
					    sourcing_name + slen - plen) == 0)
			j = OK;
		    vim_free(scriptname);
		}
	    }
	    if (j == FAIL)
	    {
		EMSG2(_("E746: Function name does not match script file name: %s"), name);
		goto erret;
	    }
	}

	fp = (ufunc_T *)alloc_clear((unsigned)(sizeof(ufunc_T) + STRLEN(name)));
	if (fp == NULL)
	    goto erret;

	if (fudi.fd_dict != NULL)
	{
	    if (fudi.fd_di == NULL)
	    {
		/* add new dict entry */
		fudi.fd_di = dictitem_alloc(fudi.fd_newkey);
		if (fudi.fd_di == NULL)
		{
		    vim_free(fp);
		    goto erret;
		}
		if (dict_add(fudi.fd_dict, fudi.fd_di) == FAIL)
		{
		    vim_free(fudi.fd_di);
		    vim_free(fp);
		    goto erret;
		}
	    }
	    else
		/* overwrite existing dict entry */
		clear_tv(&fudi.fd_di->di_tv);
	    fudi.fd_di->di_tv.v_type = VAR_FUNC;
	    fudi.fd_di->di_tv.vval.v_string = vim_strsave(name);

	    /* behave like "dict" was used */
	    flags |= FC_DICT;
	}

	/* insert the new function in the function list */
	STRCPY(fp->uf_name, name);
	if (overwrite)
	{
	    hi = hash_find(&func_hashtab, name);
	    hi->hi_key = UF2HIKEY(fp);
	}
	else if (hash_add(&func_hashtab, UF2HIKEY(fp)) == FAIL)
	{
	    vim_free(fp);
	    goto erret;
	}
	fp->uf_refcount = 1;
    }
    fp->uf_args = newargs;
    fp->uf_lines = newlines;
    if ((flags & FC_CLOSURE) != 0)
    {
	if (register_closure(fp) == FAIL)
	    goto erret;
    }
    else
	fp->uf_scoped = NULL;

#ifdef FEAT_PROFILE
    fp->uf_tml_count = NULL;
    fp->uf_tml_total = NULL;
    fp->uf_tml_self = NULL;
    fp->uf_profiling = FALSE;
    if (prof_def_func())
	func_do_profile(fp);
#endif
    fp->uf_varargs = varargs;
    fp->uf_flags = flags;
    fp->uf_calls = 0;
    fp->uf_script_ID = current_SID;
    goto ret_free;

erret:
    ga_clear_strings(&newargs);
errret_2:
    ga_clear_strings(&newlines);
ret_free:
    vim_free(skip_until);
    vim_free(line_to_free);
    vim_free(fudi.fd_newkey);
    vim_free(name);
    did_emsg |= saved_did_emsg;
    need_wait_return |= saved_wait_return;
}

/*
 * Return 5 if "p" starts with "<SID>" or "<SNR>" (ignoring case).
 * Return 2 if "p" starts with "s:".
 * Return 0 otherwise.
 */
    int
eval_fname_script(char_u *p)
{
    /* Use MB_STRICMP() because in Turkish comparing the "I" may not work with
     * the standard library function. */
    if (p[0] == '<' && (MB_STRNICMP(p + 1, "SID>", 4) == 0
				       || MB_STRNICMP(p + 1, "SNR>", 4) == 0))
	return 5;
    if (p[0] == 's' && p[1] == ':')
	return 2;
    return 0;
}

    int
translated_function_exists(char_u *name)
{
    if (builtin_function(name, -1))
	return find_internal_func(name) >= 0;
    return find_func(name) != NULL;
}

/*
 * Return TRUE if a function "name" exists.
 * If "no_defef" is TRUE, do not dereference a Funcref.
 */
    int
function_exists(char_u *name, int no_deref)
{
    char_u  *nm = name;
    char_u  *p;
    int	    n = FALSE;
    int	    flag;

    flag = TFN_INT | TFN_QUIET | TFN_NO_AUTOLOAD;
    if (no_deref)
	flag |= TFN_NO_DEREF;
    p = trans_function_name(&nm, FALSE, flag, NULL, NULL);
    nm = skipwhite(nm);

    /* Only accept "funcname", "funcname ", "funcname (..." and
     * "funcname(...", not "funcname!...". */
    if (p != NULL && (*nm == NUL || *nm == '('))
	n = translated_function_exists(p);
    vim_free(p);
    return n;
}

    char_u *
get_expanded_name(char_u *name, int check)
{
    char_u	*nm = name;
    char_u	*p;

    p = trans_function_name(&nm, FALSE, TFN_INT|TFN_QUIET, NULL, NULL);

    if (p != NULL && *nm == NUL)
	if (!check || translated_function_exists(p))
	    return p;

    vim_free(p);
    return NULL;
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Start profiling function "fp".
 */
    static void
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
	    fp->uf_tml_count = (int *)alloc_clear(
					       (unsigned)(sizeof(int) * len));
	if (fp->uf_tml_total == NULL)
	    fp->uf_tml_total = (proftime_T *)alloc_clear(
					 (unsigned)(sizeof(proftime_T) * len));
	if (fp->uf_tml_self == NULL)
	    fp->uf_tml_self = (proftime_T *)alloc_clear(
					 (unsigned)(sizeof(proftime_T) * len));
	fp->uf_tml_idx = -1;
	if (fp->uf_tml_count == NULL || fp->uf_tml_total == NULL
						    || fp->uf_tml_self == NULL)
	    return;	    /* out of memory */
	fp->uf_prof_initialized = TRUE;
    }

    fp->uf_profiling = TRUE;
}

/*
 * Dump the profiling results for all functions in file "fd".
 */
    void
func_dump_profile(FILE *fd)
{
    hashitem_T	*hi;
    int		todo;
    ufunc_T	*fp;
    int		i;
    ufunc_T	**sorttab;
    int		st_len = 0;

    todo = (int)func_hashtab.ht_used;
    if (todo == 0)
	return;     /* nothing to dump */

    sorttab = (ufunc_T **)alloc((unsigned)(sizeof(ufunc_T *) * todo));

    for (hi = func_hashtab.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    fp = HI2UF(hi);
	    if (fp->uf_prof_initialized)
	    {
		if (sorttab != NULL)
		    sorttab[st_len++] = fp;

		if (fp->uf_name[0] == K_SPECIAL)
		    fprintf(fd, "FUNCTION  <SNR>%s()\n", fp->uf_name + 3);
		else
		    fprintf(fd, "FUNCTION  %s()\n", fp->uf_name);
		if (fp->uf_tm_count == 1)
		    fprintf(fd, "Called 1 time\n");
		else
		    fprintf(fd, "Called %d times\n", fp->uf_tm_count);
		fprintf(fd, "Total time: %s\n", profile_msg(&fp->uf_tm_total));
		fprintf(fd, " Self time: %s\n", profile_msg(&fp->uf_tm_self));
		fprintf(fd, "\n");
		fprintf(fd, "count  total (s)   self (s)\n");

		for (i = 0; i < fp->uf_lines.ga_len; ++i)
		{
		    if (FUNCLINE(fp, i) == NULL)
			continue;
		    prof_func_line(fd, fp->uf_tml_count[i],
			     &fp->uf_tml_total[i], &fp->uf_tml_self[i], TRUE);
		    fprintf(fd, "%s\n", FUNCLINE(fp, i));
		}
		fprintf(fd, "\n");
	    }
	}
    }

    if (sorttab != NULL && st_len > 0)
    {
	qsort((void *)sorttab, (size_t)st_len, sizeof(ufunc_T *),
							      prof_total_cmp);
	prof_sort_list(fd, sorttab, st_len, "TOTAL", FALSE);
	qsort((void *)sorttab, (size_t)st_len, sizeof(ufunc_T *),
							      prof_self_cmp);
	prof_sort_list(fd, sorttab, st_len, "SELF", TRUE);
    }

    vim_free(sorttab);
}

    static void
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
    static void
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
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
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
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
prof_self_cmp(const void *s1, const void *s2)
{
    ufunc_T	*p1, *p2;

    p1 = *(ufunc_T **)s1;
    p2 = *(ufunc_T **)s2;
    return profile_cmp(&p1->uf_tm_self, &p2->uf_tm_self);
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
    funccall_T *fc = current_funccal;

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
    funccall_T *fc = current_funccal;

    if (fc != NULL && fc->func->uf_profiling)
    {
	profile_end(&fc->prof_child);
	profile_sub_wait(tm, &fc->prof_child); /* don't count waiting time */
	profile_add(&fc->func->uf_tm_children, &fc->prof_child);
	profile_add(&fc->func->uf_tml_children, &fc->prof_child);
    }
    script_prof_restore(tm);
}

#endif /* FEAT_PROFILE */

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)

/*
 * Function given to ExpandGeneric() to obtain the list of user defined
 * function names.
 */
    char_u *
get_user_func_name(expand_T *xp, int idx)
{
    static long_u	done;
    static hashitem_T	*hi;
    ufunc_T		*fp;

    if (idx == 0)
    {
	done = 0;
	hi = func_hashtab.ht_array;
    }
    if (done < func_hashtab.ht_used)
    {
	if (done++ > 0)
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	fp = HI2UF(hi);

	if ((fp->uf_flags & FC_DICT)
				|| STRNCMP(fp->uf_name, "<lambda>", 8) == 0)
	    return (char_u *)""; /* don't show dict and lambda functions */

	if (STRLEN(fp->uf_name) + 4 >= IOSIZE)
	    return fp->uf_name;	/* prevents overflow */

	cat_func_name(IObuff, fp);
	if (xp->xp_context != EXPAND_USER_FUNC)
	{
	    STRCAT(IObuff, "(");
	    if (!fp->uf_varargs && fp->uf_args.ga_len == 0)
		STRCAT(IObuff, ")");
	}
	return IObuff;
    }
    return NULL;
}

#endif /* FEAT_CMDL_COMPL */

/*
 * ":delfunction {name}"
 */
    void
ex_delfunction(exarg_T *eap)
{
    ufunc_T	*fp = NULL;
    char_u	*p;
    char_u	*name;
    funcdict_T	fudi;

    p = eap->arg;
    name = trans_function_name(&p, eap->skip, 0, &fudi, NULL);
    vim_free(fudi.fd_newkey);
    if (name == NULL)
    {
	if (fudi.fd_dict != NULL && !eap->skip)
	    EMSG(_(e_funcref));
	return;
    }
    if (!ends_excmd(*skipwhite(p)))
    {
	vim_free(name);
	EMSG(_(e_trailing));
	return;
    }
    eap->nextcmd = check_nextcmd(p);
    if (eap->nextcmd != NULL)
	*p = NUL;

    if (!eap->skip)
	fp = find_func(name);
    vim_free(name);

    if (!eap->skip)
    {
	if (fp == NULL)
	{
	    if (!eap->forceit)
		EMSG2(_(e_nofunc), eap->arg);
	    return;
	}
	if (fp->uf_calls > 0)
	{
	    EMSG2(_("E131: Cannot delete function %s: It is in use"), eap->arg);
	    return;
	}

	if (fudi.fd_dict != NULL)
	{
	    /* Delete the dict item that refers to the function, it will
	     * invoke func_unref() and possibly delete the function. */
	    dictitem_remove(fudi.fd_dict, fudi.fd_di);
	}
	else
	{
	    /* A normal function (not a numbered function or lambda) has a
	     * refcount of 1 for the entry in the hashtable.  When deleting
	     * it and the refcount is more than one, it should be kept.
	     * A numbered function and lambda should be kept if the refcount is
	     * one or more. */
	    if (fp->uf_refcount > (func_name_refcount(fp->uf_name) ? 0 : 1))
	    {
		/* Function is still referenced somewhere.  Don't free it but
		 * do remove it from the hashtable. */
		if (func_remove(fp))
		    fp->uf_refcount--;
		fp->uf_flags |= FC_DELETED;
	    }
	    else
		func_clear_free(fp, FALSE);
	}
    }
}

/*
 * Unreference a Function: decrement the reference count and free it when it
 * becomes zero.
 */
    void
func_unref(char_u *name)
{
    ufunc_T *fp = NULL;

    if (name == NULL || !func_name_refcount(name))
	return;
    fp = find_func(name);
    if (fp == NULL && isdigit(*name))
    {
#ifdef EXITFREE
	if (!entered_free_all_mem)
#endif
	    internal_error("func_unref()");
    }
    if (fp != NULL && --fp->uf_refcount <= 0)
    {
	/* Only delete it when it's not being used.  Otherwise it's done
	 * when "uf_calls" becomes zero. */
	if (fp->uf_calls == 0)
	    func_clear_free(fp, FALSE);
    }
}

/*
 * Unreference a Function: decrement the reference count and free it when it
 * becomes zero.
 */
    void
func_ptr_unref(ufunc_T *fp)
{
    if (fp != NULL && --fp->uf_refcount <= 0)
    {
	/* Only delete it when it's not being used.  Otherwise it's done
	 * when "uf_calls" becomes zero. */
	if (fp->uf_calls == 0)
	    func_clear_free(fp, FALSE);
    }
}

/*
 * Count a reference to a Function.
 */
    void
func_ref(char_u *name)
{
    ufunc_T *fp;

    if (name == NULL || !func_name_refcount(name))
	return;
    fp = find_func(name);
    if (fp != NULL)
	++fp->uf_refcount;
    else if (isdigit(*name))
	/* Only give an error for a numbered function.
	 * Fail silently, when named or lambda function isn't found. */
	internal_error("func_ref()");
}

/*
 * Count a reference to a Function.
 */
    void
func_ptr_ref(ufunc_T *fp)
{
    if (fp != NULL)
	++fp->uf_refcount;
}

/*
 * Return TRUE if items in "fc" do not have "copyID".  That means they are not
 * referenced from anywhere that is in use.
 */
    static int
can_free_funccal(funccall_T *fc, int copyID)
{
    return (fc->l_varlist.lv_copyID != copyID
	    && fc->l_vars.dv_copyID != copyID
	    && fc->l_avars.dv_copyID != copyID
	    && fc->fc_copyID != copyID);
}

/*
 * ":return [expr]"
 */
    void
ex_return(exarg_T *eap)
{
    char_u	*arg = eap->arg;
    typval_T	rettv;
    int		returning = FALSE;

    if (current_funccal == NULL)
    {
	EMSG(_("E133: :return not inside a function"));
	return;
    }

    if (eap->skip)
	++emsg_skip;

    eap->nextcmd = NULL;
    if ((*arg != NUL && *arg != '|' && *arg != '\n')
	    && eval0(arg, &rettv, &eap->nextcmd, !eap->skip) != FAIL)
    {
	if (!eap->skip)
	    returning = do_return(eap, FALSE, TRUE, &rettv);
	else
	    clear_tv(&rettv);
    }
    /* It's safer to return also on error. */
    else if (!eap->skip)
    {
	/* In return statement, cause_abort should be force_abort. */
	update_force_abort();

	/*
	 * Return unless the expression evaluation has been cancelled due to an
	 * aborting error, an interrupt, or an exception.
	 */
	if (!aborting())
	    returning = do_return(eap, FALSE, TRUE, NULL);
    }

    /* When skipping or the return gets pending, advance to the next command
     * in this line (!returning).  Otherwise, ignore the rest of the line.
     * Following lines will be ignored by get_func_line(). */
    if (returning)
	eap->nextcmd = NULL;
    else if (eap->nextcmd == NULL)	    /* no argument */
	eap->nextcmd = check_nextcmd(arg);

    if (eap->skip)
	--emsg_skip;
}

/*
 * ":1,25call func(arg1, arg2)"	function call.
 */
    void
ex_call(exarg_T *eap)
{
    char_u	*arg = eap->arg;
    char_u	*startarg;
    char_u	*name;
    char_u	*tofree;
    int		len;
    typval_T	rettv;
    linenr_T	lnum;
    int		doesrange;
    int		failed = FALSE;
    funcdict_T	fudi;
    partial_T	*partial = NULL;

    if (eap->skip)
    {
	/* trans_function_name() doesn't work well when skipping, use eval0()
	 * instead to skip to any following command, e.g. for:
	 *   :if 0 | call dict.foo().bar() | endif  */
	++emsg_skip;
	if (eval0(eap->arg, &rettv, &eap->nextcmd, FALSE) != FAIL)
	    clear_tv(&rettv);
	--emsg_skip;
	return;
    }

    tofree = trans_function_name(&arg, eap->skip, TFN_INT, &fudi, &partial);
    if (fudi.fd_newkey != NULL)
    {
	/* Still need to give an error message for missing key. */
	EMSG2(_(e_dictkey), fudi.fd_newkey);
	vim_free(fudi.fd_newkey);
    }
    if (tofree == NULL)
	return;

    /* Increase refcount on dictionary, it could get deleted when evaluating
     * the arguments. */
    if (fudi.fd_dict != NULL)
	++fudi.fd_dict->dv_refcount;

    /* If it is the name of a variable of type VAR_FUNC or VAR_PARTIAL use its
     * contents.  For VAR_PARTIAL get its partial, unless we already have one
     * from trans_function_name(). */
    len = (int)STRLEN(tofree);
    name = deref_func_name(tofree, &len,
				    partial != NULL ? NULL : &partial, FALSE);

    /* Skip white space to allow ":call func ()".  Not good, but required for
     * backward compatibility. */
    startarg = skipwhite(arg);
    rettv.v_type = VAR_UNKNOWN;	/* clear_tv() uses this */

    if (*startarg != '(')
    {
	EMSG2(_("E107: Missing parentheses: %s"), eap->arg);
	goto end;
    }

    /*
     * When skipping, evaluate the function once, to find the end of the
     * arguments.
     * When the function takes a range, this is discovered after the first
     * call, and the loop is broken.
     */
    if (eap->skip)
    {
	++emsg_skip;
	lnum = eap->line2;	/* do it once, also with an invalid range */
    }
    else
	lnum = eap->line1;
    for ( ; lnum <= eap->line2; ++lnum)
    {
	if (!eap->skip && eap->addr_count > 0)
	{
	    curwin->w_cursor.lnum = lnum;
	    curwin->w_cursor.col = 0;
#ifdef FEAT_VIRTUALEDIT
	    curwin->w_cursor.coladd = 0;
#endif
	}
	arg = startarg;
	if (get_func_tv(name, (int)STRLEN(name), &rettv, &arg,
		    eap->line1, eap->line2, &doesrange,
				   !eap->skip, partial, fudi.fd_dict) == FAIL)
	{
	    failed = TRUE;
	    break;
	}
	if (has_watchexpr())
	    dbg_check_breakpoint(eap);

	/* Handle a function returning a Funcref, Dictionary or List. */
	if (handle_subscript(&arg, &rettv, !eap->skip, TRUE) == FAIL)
	{
	    failed = TRUE;
	    break;
	}

	clear_tv(&rettv);
	if (doesrange || eap->skip)
	    break;

	/* Stop when immediately aborting on error, or when an interrupt
	 * occurred or an exception was thrown but not caught.
	 * get_func_tv() returned OK, so that the check for trailing
	 * characters below is executed. */
	if (aborting())
	    break;
    }
    if (eap->skip)
	--emsg_skip;

    if (!failed)
    {
	/* Check for trailing illegal characters and a following command. */
	if (!ends_excmd(*arg))
	{
	    emsg_severe = TRUE;
	    EMSG(_(e_trailing));
	}
	else
	    eap->nextcmd = check_nextcmd(arg);
    }

end:
    dict_unref(fudi.fd_dict);
    vim_free(tofree);
}

/*
 * Return from a function.  Possibly makes the return pending.  Also called
 * for a pending return at the ":endtry" or after returning from an extra
 * do_cmdline().  "reanimate" is used in the latter case.  "is_cmd" is set
 * when called due to a ":return" command.  "rettv" may point to a typval_T
 * with the return rettv.  Returns TRUE when the return can be carried out,
 * FALSE when the return gets pending.
 */
    int
do_return(
    exarg_T	*eap,
    int		reanimate,
    int		is_cmd,
    void	*rettv)
{
    int		idx;
    struct condstack *cstack = eap->cstack;

    if (reanimate)
	/* Undo the return. */
	current_funccal->returned = FALSE;

    /*
     * Cleanup (and inactivate) conditionals, but stop when a try conditional
     * not in its finally clause (which then is to be executed next) is found.
     * In this case, make the ":return" pending for execution at the ":endtry".
     * Otherwise, return normally.
     */
    idx = cleanup_conditionals(eap->cstack, 0, TRUE);
    if (idx >= 0)
    {
	cstack->cs_pending[idx] = CSTP_RETURN;

	if (!is_cmd && !reanimate)
	    /* A pending return again gets pending.  "rettv" points to an
	     * allocated variable with the rettv of the original ":return"'s
	     * argument if present or is NULL else. */
	    cstack->cs_rettv[idx] = rettv;
	else
	{
	    /* When undoing a return in order to make it pending, get the stored
	     * return rettv. */
	    if (reanimate)
		rettv = current_funccal->rettv;

	    if (rettv != NULL)
	    {
		/* Store the value of the pending return. */
		if ((cstack->cs_rettv[idx] = alloc_tv()) != NULL)
		    *(typval_T *)cstack->cs_rettv[idx] = *(typval_T *)rettv;
		else
		    EMSG(_(e_outofmem));
	    }
	    else
		cstack->cs_rettv[idx] = NULL;

	    if (reanimate)
	    {
		/* The pending return value could be overwritten by a ":return"
		 * without argument in a finally clause; reset the default
		 * return value. */
		current_funccal->rettv->v_type = VAR_NUMBER;
		current_funccal->rettv->vval.v_number = 0;
	    }
	}
	report_make_pending(CSTP_RETURN, rettv);
    }
    else
    {
	current_funccal->returned = TRUE;

	/* If the return is carried out now, store the return value.  For
	 * a return immediately after reanimation, the value is already
	 * there. */
	if (!reanimate && rettv != NULL)
	{
	    clear_tv(current_funccal->rettv);
	    *current_funccal->rettv = *(typval_T *)rettv;
	    if (!is_cmd)
		vim_free(rettv);
	}
    }

    return idx < 0;
}

/*
 * Free the variable with a pending return value.
 */
    void
discard_pending_return(void *rettv)
{
    free_tv((typval_T *)rettv);
}

/*
 * Generate a return command for producing the value of "rettv".  The result
 * is an allocated string.  Used by report_pending() for verbose messages.
 */
    char_u *
get_return_cmd(void *rettv)
{
    char_u	*s = NULL;
    char_u	*tofree = NULL;
    char_u	numbuf[NUMBUFLEN];

    if (rettv != NULL)
	s = echo_string((typval_T *)rettv, &tofree, numbuf, 0);
    if (s == NULL)
	s = (char_u *)"";

    STRCPY(IObuff, ":return ");
    STRNCPY(IObuff + 8, s, IOSIZE - 8);
    if (STRLEN(s) + 8 >= IOSIZE)
	STRCPY(IObuff + IOSIZE - 4, "...");
    vim_free(tofree);
    return vim_strsave(IObuff);
}

/*
 * Get next function line.
 * Called by do_cmdline() to get the next line.
 * Returns allocated string, or NULL for end of function.
 */
    char_u *
get_func_line(
    int	    c UNUSED,
    void    *cookie,
    int	    indent UNUSED)
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;
    char_u	*retval;
    garray_T	*gap;  /* growarray with function lines */

    /* If breakpoints have been added/deleted need to check for it. */
    if (fcp->dbg_tick != debug_tick)
    {
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name,
							       sourcing_lnum);
	fcp->dbg_tick = debug_tick;
    }
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	func_line_end(cookie);
#endif

    gap = &fp->uf_lines;
    if (((fp->uf_flags & FC_ABORT) && did_emsg && !aborted_in_try())
	    || fcp->returned)
	retval = NULL;
    else
    {
	/* Skip NULL lines (continuation lines). */
	while (fcp->linenr < gap->ga_len
			  && ((char_u **)(gap->ga_data))[fcp->linenr] == NULL)
	    ++fcp->linenr;
	if (fcp->linenr >= gap->ga_len)
	    retval = NULL;
	else
	{
	    retval = vim_strsave(((char_u **)(gap->ga_data))[fcp->linenr++]);
	    sourcing_lnum = fcp->linenr;
#ifdef FEAT_PROFILE
	    if (do_profiling == PROF_YES)
		func_line_start(cookie);
#endif
	}
    }

    /* Did we encounter a breakpoint? */
    if (fcp->breakpoint != 0 && fcp->breakpoint <= sourcing_lnum)
    {
	dbg_breakpoint(fp->uf_name, sourcing_lnum);
	/* Find next breakpoint. */
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name,
							       sourcing_lnum);
	fcp->dbg_tick = debug_tick;
    }

    return retval;
}

#if defined(FEAT_PROFILE) || defined(PROTO)
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
#endif

/*
 * Return TRUE if the currently active function should be ended, because a
 * return was encountered or an error occurred.  Used inside a ":while".
 */
    int
func_has_ended(void *cookie)
{
    funccall_T  *fcp = (funccall_T *)cookie;

    /* Ignore the "abort" flag if the abortion behavior has been changed due to
     * an error inside a try conditional. */
    return (((fcp->func->uf_flags & FC_ABORT) && did_emsg && !aborted_in_try())
	    || fcp->returned);
}

/*
 * return TRUE if cookie indicates a function which "abort"s on errors.
 */
    int
func_has_abort(
    void    *cookie)
{
    return ((funccall_T *)cookie)->func->uf_flags & FC_ABORT;
}


/*
 * Turn "dict.Func" into a partial for "Func" bound to "dict".
 * Don't do this when "Func" is already a partial that was bound
 * explicitly (pt_auto is FALSE).
 * Changes "rettv" in-place.
 * Returns the updated "selfdict_in".
 */
    dict_T *
make_partial(dict_T *selfdict_in, typval_T *rettv)
{
    char_u	*fname;
    char_u	*tofree = NULL;
    ufunc_T	*fp;
    char_u	fname_buf[FLEN_FIXED + 1];
    int		error;
    dict_T	*selfdict = selfdict_in;

    if (rettv->v_type == VAR_PARTIAL && rettv->vval.v_partial->pt_func != NULL)
	fp = rettv->vval.v_partial->pt_func;
    else
    {
	fname = rettv->v_type == VAR_FUNC ? rettv->vval.v_string
					      : rettv->vval.v_partial->pt_name;
	/* Translate "s:func" to the stored function name. */
	fname = fname_trans_sid(fname, fname_buf, &tofree, &error);
	fp = find_func(fname);
	vim_free(tofree);
    }

    if (fp != NULL && (fp->uf_flags & FC_DICT))
    {
	partial_T	*pt = (partial_T *)alloc_clear(sizeof(partial_T));

	if (pt != NULL)
	{
	    pt->pt_refcount = 1;
	    pt->pt_dict = selfdict;
	    pt->pt_auto = TRUE;
	    selfdict = NULL;
	    if (rettv->v_type == VAR_FUNC)
	    {
		/* Just a function: Take over the function name and use
		 * selfdict. */
		pt->pt_name = rettv->vval.v_string;
	    }
	    else
	    {
		partial_T	*ret_pt = rettv->vval.v_partial;
		int		i;

		/* Partial: copy the function name, use selfdict and copy
		 * args.  Can't take over name or args, the partial might
		 * be referenced elsewhere. */
		if (ret_pt->pt_name != NULL)
		{
		    pt->pt_name = vim_strsave(ret_pt->pt_name);
		    func_ref(pt->pt_name);
		}
		else
		{
		    pt->pt_func = ret_pt->pt_func;
		    func_ptr_ref(pt->pt_func);
		}
		if (ret_pt->pt_argc > 0)
		{
		    pt->pt_argv = (typval_T *)alloc(
				      sizeof(typval_T) * ret_pt->pt_argc);
		    if (pt->pt_argv == NULL)
			/* out of memory: drop the arguments */
			pt->pt_argc = 0;
		    else
		    {
			pt->pt_argc = ret_pt->pt_argc;
			for (i = 0; i < pt->pt_argc; i++)
			    copy_tv(&ret_pt->pt_argv[i], &pt->pt_argv[i]);
		    }
		}
		partial_unref(ret_pt);
	    }
	    rettv->v_type = VAR_PARTIAL;
	    rettv->vval.v_partial = pt;
	}
    }
    return selfdict;
}

/*
 * Return the name of the executed function.
 */
    char_u *
func_name(void *cookie)
{
    return ((funccall_T *)cookie)->func->uf_name;
}

/*
 * Return the address holding the next breakpoint line for a funccall cookie.
 */
    linenr_T *
func_breakpoint(void *cookie)
{
    return &((funccall_T *)cookie)->breakpoint;
}

/*
 * Return the address holding the debug tick for a funccall cookie.
 */
    int *
func_dbg_tick(void *cookie)
{
    return &((funccall_T *)cookie)->dbg_tick;
}

/*
 * Return the nesting level for a funccall cookie.
 */
    int
func_level(void *cookie)
{
    return ((funccall_T *)cookie)->level;
}

/*
 * Return TRUE when a function was ended by a ":return" command.
 */
    int
current_func_returned(void)
{
    return current_funccal->returned;
}

/*
 * Save the current function call pointer, and set it to NULL.
 * Used when executing autocommands and for ":source".
 */
    void *
save_funccal(void)
{
    funccall_T *fc = current_funccal;

    current_funccal = NULL;
    return (void *)fc;
}

    void
restore_funccal(void *vfc)
{
    funccall_T *fc = (funccall_T *)vfc;

    current_funccal = fc;
}

    int
free_unref_funccal(int copyID, int testing)
{
    int		did_free = FALSE;
    int		did_free_funccal = FALSE;
    funccall_T	*fc, **pfc;

    for (pfc = &previous_funccal; *pfc != NULL; )
    {
	if (can_free_funccal(*pfc, copyID))
	{
	    fc = *pfc;
	    *pfc = fc->caller;
	    free_funccal(fc, TRUE);
	    did_free = TRUE;
	    did_free_funccal = TRUE;
	}
	else
	    pfc = &(*pfc)->caller;
    }
    if (did_free_funccal)
	/* When a funccal was freed some more items might be garbage
	 * collected, so run again. */
	(void)garbage_collect(testing);

    return did_free;
}

/*
 * Get function call environment based on backtrace debug level
 */
    static funccall_T *
get_funccal(void)
{
    int		i;
    funccall_T	*funccal;
    funccall_T	*temp_funccal;

    funccal = current_funccal;
    if (debug_backtrace_level > 0)
    {
	for (i = 0; i < debug_backtrace_level; i++)
	{
	    temp_funccal = funccal->caller;
	    if (temp_funccal)
		funccal = temp_funccal;
	    else
		/* backtrace level overflow. reset to max */
		debug_backtrace_level = i;
	}
    }
    return funccal;
}

/*
 * Return the hashtable used for local variables in the current funccal.
 * Return NULL if there is no current funccal.
 */
    hashtab_T *
get_funccal_local_ht()
{
    if (current_funccal == NULL)
	return NULL;
    return &get_funccal()->l_vars.dv_hashtab;
}

/*
 * Return the l: scope variable.
 * Return NULL if there is no current funccal.
 */
    dictitem_T *
get_funccal_local_var()
{
    if (current_funccal == NULL)
	return NULL;
    return &get_funccal()->l_vars_var;
}

/*
 * Return the hashtable used for argument in the current funccal.
 * Return NULL if there is no current funccal.
 */
    hashtab_T *
get_funccal_args_ht()
{
    if (current_funccal == NULL)
	return NULL;
    return &get_funccal()->l_avars.dv_hashtab;
}

/*
 * Return the a: scope variable.
 * Return NULL if there is no current funccal.
 */
    dictitem_T *
get_funccal_args_var()
{
    if (current_funccal == NULL)
	return NULL;
    return &get_funccal()->l_avars_var;
}

/*
 * Clear the current_funccal and return the old value.
 * Caller is expected to invoke restore_current_funccal().
 */
    void *
clear_current_funccal()
{
    funccall_T *f = current_funccal;

    current_funccal = NULL;
    return f;
}

    void
restore_current_funccal(void *f)
{
    current_funccal = f;
}

/*
 * List function variables, if there is a function.
 */
    void
list_func_vars(int *first)
{
    if (current_funccal != NULL)
	list_hashtable_vars(&current_funccal->l_vars.dv_hashtab,
						(char_u *)"l:", FALSE, first);
}

/*
 * If "ht" is the hashtable for local variables in the current funccal, return
 * the dict that contains it.
 * Otherwise return NULL.
 */
    dict_T *
get_current_funccal_dict(hashtab_T *ht)
{
    if (current_funccal != NULL
	    && ht == &current_funccal->l_vars.dv_hashtab)
	return &current_funccal->l_vars;
    return NULL;
}

/*
 * Search hashitem in parent scope.
 */
    hashitem_T *
find_hi_in_scoped_ht(char_u *name, hashtab_T **pht)
{
    funccall_T	*old_current_funccal = current_funccal;
    hashtab_T	*ht;
    hashitem_T	*hi = NULL;
    char_u	*varname;

    if (current_funccal == NULL || current_funccal->func->uf_scoped == NULL)
      return NULL;

    /* Search in parent scope which is possible to reference from lambda */
    current_funccal = current_funccal->func->uf_scoped;
    while (current_funccal != NULL)
    {
	ht = find_var_ht(name, &varname);
	if (ht != NULL && *varname != NUL)
	{
	    hi = hash_find(ht, varname);
	    if (!HASHITEM_EMPTY(hi))
	    {
		*pht = ht;
		break;
	    }
	}
	if (current_funccal == current_funccal->func->uf_scoped)
	    break;
	current_funccal = current_funccal->func->uf_scoped;
    }
    current_funccal = old_current_funccal;

    return hi;
}

/*
 * Search variable in parent scope.
 */
    dictitem_T *
find_var_in_scoped_ht(char_u *name, int no_autoload)
{
    dictitem_T	*v = NULL;
    funccall_T	*old_current_funccal = current_funccal;
    hashtab_T	*ht;
    char_u	*varname;

    if (current_funccal == NULL || current_funccal->func->uf_scoped == NULL)
	return NULL;

    /* Search in parent scope which is possible to reference from lambda */
    current_funccal = current_funccal->func->uf_scoped;
    while (current_funccal)
    {
	ht = find_var_ht(name, &varname);
	if (ht != NULL && *varname != NUL)
	{
	    v = find_var_in_ht(ht, *name, varname, no_autoload);
	    if (v != NULL)
		break;
	}
	if (current_funccal == current_funccal->func->uf_scoped)
	    break;
	current_funccal = current_funccal->func->uf_scoped;
    }
    current_funccal = old_current_funccal;

    return v;
}

/*
 * Set "copyID + 1" in previous_funccal and callers.
 */
    int
set_ref_in_previous_funccal(int copyID)
{
    int		abort = FALSE;
    funccall_T	*fc;

    for (fc = previous_funccal; fc != NULL; fc = fc->caller)
    {
	fc->fc_copyID = copyID + 1;
	abort = abort || set_ref_in_ht(&fc->l_vars.dv_hashtab, copyID + 1,
									NULL);
	abort = abort || set_ref_in_ht(&fc->l_avars.dv_hashtab, copyID + 1,
									NULL);
    }
    return abort;
}

    static int
set_ref_in_funccal(funccall_T *fc, int copyID)
{
    int abort = FALSE;

    if (fc->fc_copyID != copyID)
    {
	fc->fc_copyID = copyID;
	abort = abort || set_ref_in_ht(&fc->l_vars.dv_hashtab, copyID, NULL);
	abort = abort || set_ref_in_ht(&fc->l_avars.dv_hashtab, copyID, NULL);
	abort = abort || set_ref_in_func(NULL, fc->func, copyID);
    }
    return abort;
}

/*
 * Set "copyID" in all local vars and arguments in the call stack.
 */
    int
set_ref_in_call_stack(int copyID)
{
    int		abort = FALSE;
    funccall_T	*fc;

    for (fc = current_funccal; fc != NULL; fc = fc->caller)
	abort = abort || set_ref_in_funccal(fc, copyID);
    return abort;
}

/*
 * Set "copyID" in all functions available by name.
 */
    int
set_ref_in_functions(int copyID)
{
    int		todo;
    hashitem_T	*hi = NULL;
    int		abort = FALSE;
    ufunc_T	*fp;

    todo = (int)func_hashtab.ht_used;
    for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    fp = HI2UF(hi);
	    if (!func_name_refcount(fp->uf_name))
		abort = abort || set_ref_in_func(NULL, fp, copyID);
	}
    }
    return abort;
}

/*
 * Set "copyID" in all function arguments.
 */
    int
set_ref_in_func_args(int copyID)
{
    int i;
    int abort = FALSE;

    for (i = 0; i < funcargs.ga_len; ++i)
	abort = abort || set_ref_in_item(((typval_T **)funcargs.ga_data)[i],
							  copyID, NULL, NULL);
    return abort;
}

/*
 * Mark all lists and dicts referenced through function "name" with "copyID".
 * Returns TRUE if setting references failed somehow.
 */
    int
set_ref_in_func(char_u *name, ufunc_T *fp_in, int copyID)
{
    ufunc_T	*fp = fp_in;
    funccall_T	*fc;
    int		error = ERROR_NONE;
    char_u	fname_buf[FLEN_FIXED + 1];
    char_u	*tofree = NULL;
    char_u	*fname;
    int		abort = FALSE;

    if (name == NULL && fp_in == NULL)
	return FALSE;

    if (fp_in == NULL)
    {
	fname = fname_trans_sid(name, fname_buf, &tofree, &error);
	fp = find_func(fname);
    }
    if (fp != NULL)
    {
	for (fc = fp->uf_scoped; fc != NULL; fc = fc->func->uf_scoped)
	    abort = abort || set_ref_in_funccal(fc, copyID);
    }
    vim_free(tofree);
    return abort;
}

#endif /* FEAT_EVAL */
