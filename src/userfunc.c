/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * userfunc.c: User defined function support
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * All user-defined functions are found in this hashtable.
 */
static hashtab_T	func_hashtab;

// Used by get_func_tv()
static garray_T funcargs = GA_EMPTY;

// pointer to funccal for currently active function
static funccall_T *current_funccal = NULL;

// Pointer to list of previously used funccal, still around because some
// item in it is still being used.
static funccall_T *previous_funccal = NULL;

static void funccal_unref(funccall_T *fc, ufunc_T *fp, int force);
static void func_clear(ufunc_T *fp, int force);
static int func_free(ufunc_T *fp, int force);

    void
func_init()
{
    hash_init(&func_hashtab);
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Return the function hash table
 */
    hashtab_T *
func_tbl_get(void)
{
    return &func_hashtab;
}
#endif

/*
 * Get one function argument.
 * If "argtypes" is not NULL also get the type: "arg: type" (:def function).
 * If "types_optional" is TRUE a missing type is OK, use "any".
 * If "evalarg" is not NULL use it to check for an already declared name.
 * Return a pointer to after the type.
 * When something is wrong return "arg".
 */
    static char_u *
one_function_arg(
	char_u	    *arg,
	garray_T    *newargs,
	garray_T    *argtypes,
	int	    types_optional,
	evalarg_T   *evalarg,
	int	    is_vararg,
	int	    skip)
{
    char_u	*p = arg;
    char_u	*arg_copy = NULL;
    int		is_underscore = FALSE;

    while (ASCII_ISALNUM(*p) || *p == '_')
	++p;
    if (arg == p || isdigit(*arg)
	    || (argtypes == NULL
		&& ((p - arg == 9 && STRNCMP(arg, "firstline", 9) == 0)
		    || (p - arg == 8 && STRNCMP(arg, "lastline", 8) == 0))))
    {
	if (!skip)
	    semsg(_(e_illegal_argument_str), arg);
	return arg;
    }

    // Vim9 script: cannot use script var name for argument. In function: also
    // check local vars and arguments.
    if (!skip && argtypes != NULL && check_defined(arg, p - arg,
		    evalarg == NULL ? NULL : evalarg->eval_cctx, TRUE) == FAIL)
	return arg;

    if (newargs != NULL && ga_grow(newargs, 1) == FAIL)
	return arg;
    if (newargs != NULL)
    {
	int	c;
	int	i;

	c = *p;
	*p = NUL;
	arg_copy = vim_strsave(arg);
	if (arg_copy == NULL)
	{
	    *p = c;
	    return arg;
	}
	is_underscore = arg_copy[0] == '_' && arg_copy[1] == NUL;
	if (argtypes == NULL || !is_underscore)
	    // Check for duplicate argument name.
	    for (i = 0; i < newargs->ga_len; ++i)
		if (STRCMP(((char_u **)(newargs->ga_data))[i], arg_copy) == 0)
		{
		    semsg(_(e_duplicate_argument_name_str), arg_copy);
		    vim_free(arg_copy);
		    return arg;
		}
	((char_u **)(newargs->ga_data))[newargs->ga_len] = arg_copy;
	newargs->ga_len++;

	*p = c;
    }

    // get any type from "arg: type"
    if (argtypes != NULL && (skip || ga_grow(argtypes, 1) == OK))
    {
	char_u *type = NULL;

	if (VIM_ISWHITE(*p) && *skipwhite(p) == ':')
	{
	    semsg(_(e_no_white_space_allowed_before_colon_str),
					    arg_copy == NULL ? arg : arg_copy);
	    p = skipwhite(p);
	}
	if (*p == ':')
	{
	    ++p;
	    if (!skip && !VIM_ISWHITE(*p))
	    {
		semsg(_(e_white_space_required_after_str_str), ":", p - 1);
		return arg;
	    }
	    type = skipwhite(p);
	    p = skip_type(type, TRUE);
	    if (!skip)
		type = vim_strnsave(type, p - type);
	}
	else if (*skipwhite(p) != '=' && !types_optional && !is_underscore)
	{
	    semsg(_(e_missing_argument_type_for_str),
					    arg_copy == NULL ? arg : arg_copy);
	    return arg;
	}
	if (!skip)
	{
	    if (type == NULL && types_optional)
		// lambda arguments default to "any" type
		type = vim_strsave((char_u *)
					    (is_vararg ? "list<any>" : "any"));
	    ((char_u **)argtypes->ga_data)[argtypes->ga_len++] = type;
	}
    }

    return p;
}

/*
 * Handle line continuation in function arguments or body.
 * Get a next line, store it in "eap" if appropriate and use "line_to_free" to
 * handle freeing the line later.
 */
    static char_u *
get_function_line(
	exarg_T		*eap,
	char_u		**line_to_free,
	int		indent,
	getline_opt_T	getline_options)
{
    char_u *theline;

    if (eap->getline == NULL)
	theline = getcmdline(':', 0L, indent, 0);
    else
	theline = eap->getline(':', eap->cookie, indent, getline_options);
    if (theline != NULL)
    {
	if (*eap->cmdlinep == *line_to_free)
	    *eap->cmdlinep = theline;
	vim_free(*line_to_free);
	*line_to_free = theline;
    }

    return theline;
}

/*
 * Get function arguments.
 * "argp" should point to just after the "(", possibly to white space.
 * "argp" is advanced just after "endchar".
 */
    static int
get_function_args(
    char_u	**argp,
    char_u	endchar,
    garray_T	*newargs,
    garray_T	*argtypes,	// NULL unless using :def
    int		types_optional,	// types optional if "argtypes" is not NULL
    evalarg_T	*evalarg,	// context or NULL
    int		*varargs,
    garray_T	*default_args,
    int		skip,
    exarg_T	*eap,
    char_u	**line_to_free)
{
    int		mustend = FALSE;
    char_u	*arg;
    char_u	*p;
    int		c;
    int		any_default = FALSE;
    char_u	*expr;
    char_u	*whitep = *argp;

    if (newargs != NULL)
	ga_init2(newargs, (int)sizeof(char_u *), 3);
    if (argtypes != NULL)
	ga_init2(argtypes, (int)sizeof(char_u *), 3);
    if (!skip && default_args != NULL)
	ga_init2(default_args, (int)sizeof(char_u *), 3);

    if (varargs != NULL)
	*varargs = FALSE;

    /*
     * Isolate the arguments: "arg1, arg2, ...)"
     */
    arg = skipwhite(*argp);
    p = arg;
    while (*p != endchar)
    {
	while (eap != NULL && eap->getline != NULL
			 && (*p == NUL || (VIM_ISWHITE(*whitep) && *p == '#')))
	{
	    // End of the line, get the next one.
	    char_u *theline = get_function_line(eap, line_to_free, 0,
							  GETLINE_CONCAT_CONT);

	    if (theline == NULL)
		break;
	    whitep = (char_u *)" ";
	    p = skipwhite(theline);
	}

	if (mustend && *p != endchar)
	{
	    if (!skip)
		semsg(_(e_invalid_argument_str), *argp);
	    goto err_ret;
	}
	if (*p == endchar)
	    break;

	if (p[0] == '.' && p[1] == '.' && p[2] == '.')
	{
	    if (varargs != NULL)
		*varargs = TRUE;
	    p += 3;
	    mustend = TRUE;

	    if (argtypes != NULL)
	    {
		// ...name: list<type>
		if (!eval_isnamec1(*p))
		{
		    if (!skip)
			emsg(_(e_missing_name_after_dots));
		    goto err_ret;
		}

		arg = p;
		p = one_function_arg(p, newargs, argtypes, types_optional,
							  evalarg, TRUE, skip);
		if (p == arg)
		    break;
		if (*skipwhite(p) == '=')
		{
		    emsg(_(e_cannot_use_default_for_variable_arguments));
		    break;
		}
	    }
	}
	else
	{
	    char_u *np;

	    arg = p;
	    p = one_function_arg(p, newargs, argtypes, types_optional,
							 evalarg, FALSE, skip);
	    if (p == arg)
		break;

	    // Recognize " = expr" but not " == expr".  A lambda can have
	    // "(a = expr" but "(a == expr" and "(a =~ expr" are not a lambda.
	    np = skipwhite(p);
	    if (*np == '=' && np[1] != '=' && np[1] != '~'
						       && default_args != NULL)
	    {
		typval_T	rettv;

		// find the end of the expression (doesn't evaluate it)
		any_default = TRUE;
		p = skipwhite(p) + 1;
		whitep = p;
		p = skipwhite(p);
		expr = p;
		if (eval1(&p, &rettv, NULL) != FAIL)
		{
		    if (!skip)
		    {
			if (ga_grow(default_args, 1) == FAIL)
			    goto err_ret;

			// trim trailing whitespace
			while (p > expr && VIM_ISWHITE(p[-1]))
			    p--;
			c = *p;
			*p = NUL;
			expr = vim_strsave(expr);
			if (expr == NULL)
			{
			    *p = c;
			    goto err_ret;
			}
			((char_u **)(default_args->ga_data))
						 [default_args->ga_len] = expr;
			default_args->ga_len++;
			*p = c;
		    }
		}
		else
		    mustend = TRUE;
	    }
	    else if (any_default)
	    {
		emsg(_(e_non_default_argument_follows_default_argument));
		goto err_ret;
	    }

	    if (VIM_ISWHITE(*p) && *skipwhite(p) == ',')
	    {
		// Be tolerant when skipping
		if (!skip)
		{
		    semsg(_(e_no_white_space_allowed_before_str_str), ",", p);
		    goto err_ret;
		}
		p = skipwhite(p);
	    }
	    if (*p == ',')
	    {
		++p;
		// Don't give this error when skipping, it makes the "->" not
		// found in "{k,v -> x}" and give a confusing error.
		// Allow missing space after comma in legacy functions.
		if (!skip && argtypes != NULL
				      && !IS_WHITE_OR_NUL(*p) && *p != endchar)
		{
		    semsg(_(e_white_space_required_after_str_str), ",", p - 1);
		    goto err_ret;
		}
	    }
	    else
		mustend = TRUE;
	}
	whitep = p;
	p = skipwhite(p);
    }

    if (*p != endchar)
	goto err_ret;
    ++p;	// skip "endchar"

    *argp = p;
    return OK;

err_ret:
    if (newargs != NULL)
	ga_clear_strings(newargs);
    if (!skip && default_args != NULL)
	ga_clear_strings(default_args);
    return FAIL;
}

/*
 * Parse the argument types, filling "fp->uf_arg_types".
 * Return OK or FAIL.
 */
    static int
parse_argument_types(ufunc_T *fp, garray_T *argtypes, int varargs)
{
    int len = 0;

    ga_init2(&fp->uf_type_list, sizeof(type_T *), 10);
    if (argtypes->ga_len > 0)
    {
	// When "varargs" is set the last name/type goes into uf_va_name
	// and uf_va_type.
	len = argtypes->ga_len - (varargs ? 1 : 0);

	if (len > 0)
	    fp->uf_arg_types = ALLOC_CLEAR_MULT(type_T *, len);
	if (fp->uf_arg_types != NULL)
	{
	    int	i;
	    type_T	*type;

	    for (i = 0; i < len; ++ i)
	    {
		char_u *p = ((char_u **)argtypes->ga_data)[i];

		if (p == NULL)
		    // will get the type from the default value
		    type = &t_unknown;
		else
		    type = parse_type(&p, &fp->uf_type_list, TRUE);
		if (type == NULL)
		    return FAIL;
		fp->uf_arg_types[i] = type;
	    }
	}
    }

    if (varargs)
    {
	char_u *p;

	// Move the last argument "...name: type" to uf_va_name and
	// uf_va_type.
	fp->uf_va_name = ((char_u **)fp->uf_args.ga_data)
					      [fp->uf_args.ga_len - 1];
	--fp->uf_args.ga_len;
	p = ((char_u **)argtypes->ga_data)[len];
	if (p == NULL)
	    // TODO: get type from default value
	    fp->uf_va_type = &t_list_any;
	else
	{
	    fp->uf_va_type = parse_type(&p, &fp->uf_type_list, TRUE);
	    if (fp->uf_va_type != NULL && fp->uf_va_type->tt_type != VAR_LIST)
	    {
		semsg(_(e_variable_arguments_type_must_be_list_str),
					  ((char_u **)argtypes->ga_data)[len]);
		return FAIL;
	    }
	}
	if (fp->uf_va_type == NULL)
	    return FAIL;
    }

    return OK;
}

    static int
parse_return_type(ufunc_T *fp, char_u *ret_type)
{
    if (ret_type == NULL)
	fp->uf_ret_type = &t_void;
    else
    {
	char_u *p = ret_type;

	fp->uf_ret_type = parse_type(&p, &fp->uf_type_list, TRUE);
	if (fp->uf_ret_type == NULL)
	{
	    fp->uf_ret_type = &t_void;
	    return FAIL;
	}
    }
    return OK;
}

/*
 * Register function "fp" as using "current_funccal" as its scope.
 */
    static int
register_closure(ufunc_T *fp)
{
    if (fp->uf_scoped == current_funccal)
	// no change
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

    static void
set_ufunc_name(ufunc_T *fp, char_u *name)
{
    // Add a type cast to avoid a warning for an overflow, the uf_name[] array
    // actually extends beyond the struct.
    STRCPY((void *)fp->uf_name, name);

    if (name[0] == K_SPECIAL)
    {
	fp->uf_name_exp = alloc(STRLEN(name) + 3);
	if (fp->uf_name_exp != NULL)
	{
	    STRCPY(fp->uf_name_exp, "<SNR>");
	    STRCAT(fp->uf_name_exp, fp->uf_name + 3);
	}
    }
}

/*
 * Get a name for a lambda.  Returned in static memory.
 */
    char_u *
get_lambda_name(void)
{
    static char_u   name[30];
    static int	    lambda_no = 0;

    sprintf((char*)name, "<lambda>%d", ++lambda_no);
    return name;
}

#if defined(FEAT_LUA) || defined(PROTO)
/*
 * Registers a native C callback which can be called from Vim script.
 * Returns the name of the Vim script function.
 */
    char_u *
register_cfunc(cfunc_T cb, cfunc_free_T cb_free, void *state)
{
    char_u	*name = get_lambda_name();
    ufunc_T	*fp;

    fp = alloc_clear(offsetof(ufunc_T, uf_name) + STRLEN(name) + 1);
    if (fp == NULL)
	return NULL;

    fp->uf_def_status = UF_NOT_COMPILED;
    fp->uf_refcount = 1;
    fp->uf_varargs = TRUE;
    fp->uf_flags = FC_CFUNC | FC_LAMBDA;
    fp->uf_calls = 0;
    fp->uf_script_ctx = current_sctx;
    fp->uf_cb = cb;
    fp->uf_cb_free = cb_free;
    fp->uf_cb_state = state;

    set_ufunc_name(fp, name);
    hash_add(&func_hashtab, UF2HIKEY(fp));

    return name;
}
#endif

/*
 * Skip over "->" or "=>" after the arguments of a lambda.
 * If ": type" is found make "ret_type" point to "type".
 * If "white_error" is not NULL check for correct use of white space and set
 * "white_error" to TRUE if there is an error.
 * Return NULL if no valid arrow found.
 */
    static char_u *
skip_arrow(
	char_u	*start,
	int	equal_arrow,
	char_u	**ret_type,
	int	*white_error)
{
    char_u  *s = start;
    char_u  *bef = start - 2; // "start" points to > of ->

    if (equal_arrow)
    {
	if (*s == ':')
	{
	    if (white_error != NULL && !VIM_ISWHITE(s[1]))
	    {
		*white_error = TRUE;
		semsg(_(e_white_space_required_after_str_str), ":", s);
		return NULL;
	    }
	    s = skipwhite(s + 1);
	    *ret_type = s;
	    s = skip_type(s, TRUE);
	    if (s == *ret_type)
	    {
		emsg(_(e_missing_return_type));
		return NULL;
	    }
	}
	bef = s;
	s = skipwhite(s);
	if (*s != '=')
	    return NULL;
	++s;
    }
    if (*s != '>')
	return NULL;
    if (white_error != NULL && ((!VIM_ISWHITE(*bef) && *bef != '{')
		|| !IS_WHITE_OR_NUL(s[1])))
    {
	*white_error = TRUE;
	semsg(_(e_white_space_required_before_and_after_str_at_str),
					       equal_arrow ? "=>" : "->", bef);
	return NULL;
    }
    return skipwhite(s + 1);
}

/*
 * Check if "*cmd" points to a function command and if so advance "*cmd" and
 * return TRUE.
 * Otherwise return FALSE;
 * Do not consider "function(" to be a command.
 */
    static int
is_function_cmd(char_u **cmd)
{
    char_u *p = *cmd;

    if (checkforcmd(&p, "function", 2))
    {
	if (*p == '(')
	    return FALSE;
	*cmd = p;
	return TRUE;
    }
    return FALSE;
}

/*
 * Called when defining a function: The context may be needed for script
 * variables declared in a block that is visible now but not when the function
 * is compiled or called later.
 */
    static void
function_using_block_scopes(ufunc_T *fp, cstack_T *cstack)
{
    if (cstack != NULL && cstack->cs_idx >= 0)
    {
	int	    count = cstack->cs_idx + 1;
	int	    i;

	fp->uf_block_ids = ALLOC_MULT(int, count);
	if (fp->uf_block_ids != NULL)
	{
	    mch_memmove(fp->uf_block_ids, cstack->cs_block_id,
							  sizeof(int) * count);
	    fp->uf_block_depth = count;
	}

	// Set flag in each block to indicate a function was defined.  This
	// is used to keep the variable when leaving the block, see
	// hide_script_var().
	for (i = 0; i <= cstack->cs_idx; ++i)
	    cstack->cs_flags[i] |= CSF_FUNC_DEF;
    }
}

/*
 * Read the body of a function, put every line in "newlines".
 * This stops at "}", "endfunction" or "enddef".
 * "newlines" must already have been initialized.
 * "eap->cmdidx" is CMD_function, CMD_def or CMD_block;
 */
    static int
get_function_body(
	exarg_T	    *eap,
	garray_T    *newlines,
	char_u	    *line_arg_in,
	char_u	    **line_to_free)
{
    linenr_T	sourcing_lnum_top = SOURCING_LNUM;
    linenr_T	sourcing_lnum_off;
    int		saved_wait_return = need_wait_return;
    char_u	*line_arg = line_arg_in;
    int		vim9_function = eap->cmdidx == CMD_def
						   || eap->cmdidx == CMD_block;
#define MAX_FUNC_NESTING 50
    char	nesting_def[MAX_FUNC_NESTING];
    char	nesting_inline[MAX_FUNC_NESTING];
    int		nesting = 0;
    getline_opt_T getline_options;
    int		indent = 2;
    char_u	*skip_until = NULL;
    int		ret = FAIL;
    int		is_heredoc = FALSE;
    int		heredoc_concat_len = 0;
    garray_T	heredoc_ga;
    char_u	*heredoc_trimmed = NULL;

    ga_init2(&heredoc_ga, 1, 500);

    // Detect having skipped over comment lines to find the return
    // type.  Add NULL lines to keep the line count correct.
    sourcing_lnum_off = get_sourced_lnum(eap->getline, eap->cookie);
    if (SOURCING_LNUM < sourcing_lnum_off)
    {
	sourcing_lnum_off -= SOURCING_LNUM;
	if (ga_grow(newlines, sourcing_lnum_off) == FAIL)
	    goto theend;
	while (sourcing_lnum_off-- > 0)
	    ((char_u **)(newlines->ga_data))[newlines->ga_len++] = NULL;
    }

    nesting_def[0] = vim9_function;
    nesting_inline[0] = eap->cmdidx == CMD_block;
    getline_options = vim9_function
				? GETLINE_CONCAT_CONTBAR : GETLINE_CONCAT_CONT;
    for (;;)
    {
	char_u	*theline;
	char_u	*p;
	char_u	*arg;

	if (KeyTyped)
	{
	    msg_scroll = TRUE;
	    saved_wait_return = FALSE;
	}
	need_wait_return = FALSE;

	if (line_arg != NULL)
	{
	    // Use eap->arg, split up in parts by line breaks.
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
	    theline = get_function_line(eap, line_to_free, indent,
							      getline_options);
	}
	if (KeyTyped)
	    lines_left = Rows - 1;
	if (theline == NULL)
	{
	    // Use the start of the function for the line number.
	    SOURCING_LNUM = sourcing_lnum_top;
	    if (skip_until != NULL)
		semsg(_(e_missing_heredoc_end_marker_str), skip_until);
	    else if (nesting_inline[nesting])
		emsg(_(e_missing_end_block));
	    else if (eap->cmdidx == CMD_def)
		emsg(_(e_missing_enddef));
	    else
		emsg(_(e_missing_endfunction));
	    goto theend;
	}

	// Detect line continuation: SOURCING_LNUM increased more than one.
	sourcing_lnum_off = get_sourced_lnum(eap->getline, eap->cookie);
	if (SOURCING_LNUM < sourcing_lnum_off)
	    sourcing_lnum_off -= SOURCING_LNUM;
	else
	    sourcing_lnum_off = 0;

	if (skip_until != NULL)
	{
	    // Don't check for ":endfunc"/":enddef" between
	    // * ":append" and "."
	    // * ":python <<EOF" and "EOF"
	    // * ":let {var-name} =<< [trim] {marker}" and "{marker}"
	    if (heredoc_trimmed == NULL
		    || (is_heredoc && skipwhite(theline) == theline)
		    || STRNCMP(theline, heredoc_trimmed,
						 STRLEN(heredoc_trimmed)) == 0)
	    {
		if (heredoc_trimmed == NULL)
		    p = theline;
		else if (is_heredoc)
		    p = skipwhite(theline) == theline
				 ? theline : theline + STRLEN(heredoc_trimmed);
		else
		    p = theline + STRLEN(heredoc_trimmed);
		if (STRCMP(p, skip_until) == 0)
		{
		    VIM_CLEAR(skip_until);
		    VIM_CLEAR(heredoc_trimmed);
		    getline_options = vim9_function
				? GETLINE_CONCAT_CONTBAR : GETLINE_CONCAT_CONT;
		    is_heredoc = FALSE;

		    if (heredoc_concat_len > 0)
		    {
			// Replace the starting line with all the concatenated
			// lines.
			ga_concat(&heredoc_ga, theline);
			vim_free(((char_u **)(newlines->ga_data))[
						      heredoc_concat_len - 1]);
			((char_u **)(newlines->ga_data))[
				  heredoc_concat_len - 1] = heredoc_ga.ga_data;
			ga_init(&heredoc_ga);
			heredoc_concat_len = 0;
			theline += STRLEN(theline);  // skip the "EOF"
		    }
		}
	    }
	}
	else
	{
	    int	    c;
	    char_u  *end;

	    // skip ':' and blanks
	    for (p = theline; VIM_ISWHITE(*p) || *p == ':'; ++p)
		;

	    // Check for "endfunction", "enddef" or "}".
	    // When a ":" follows it must be a dict key; "enddef: value,"
	    if (nesting_inline[nesting]
		    ? *p == '}'
		    : (checkforcmd(&p, nesting_def[nesting]
						? "enddef" : "endfunction", 4)
			&& *p != ':'))
	    {
		if (nesting-- == 0)
		{
		    char_u *nextcmd = NULL;

		    if (*p == '|' || *p == '}')
			nextcmd = p + 1;
		    else if (line_arg != NULL && *skipwhite(line_arg) != NUL)
			nextcmd = line_arg;
		    else if (*p != NUL && *p != (vim9_function ? '#' : '"')
					   && (vim9_function || p_verbose > 0))
		    {
			SOURCING_LNUM = sourcing_lnum_top
							+ newlines->ga_len + 1;
			if (eap->cmdidx == CMD_def)
			    semsg(_(e_text_found_after_str_str), "enddef", p);
			else
			    give_warning2((char_u *)
				   _("W22: Text found after :endfunction: %s"),
				   p, TRUE);
		    }
		    if (nextcmd != NULL && *skipwhite(nextcmd) != NUL)
		    {
			// Another command follows. If the line came from "eap"
			// we can simply point into it, otherwise we need to
			// change "eap->cmdlinep".
			eap->nextcmd = nextcmd;
			if (*line_to_free != NULL
					    && *eap->cmdlinep != *line_to_free)
			{
			    vim_free(*eap->cmdlinep);
			    *eap->cmdlinep = *line_to_free;
			    *line_to_free = NULL;
			}
		    }
		    break;
		}
	    }

	    // Check for mismatched "endfunc" or "enddef".
	    // We don't check for "def" inside "func" thus we also can't check
	    // for "enddef".
	    // We continue to find the end of the function, although we might
	    // not find it.
	    else if (nesting_def[nesting])
	    {
		if (checkforcmd(&p, "endfunction", 4) && *p != ':')
		    emsg(_(e_mismatched_endfunction));
	    }
	    else if (eap->cmdidx == CMD_def && checkforcmd(&p, "enddef", 4))
		emsg(_(e_mismatched_enddef));

	    // Increase indent inside "if", "while", "for" and "try", decrease
	    // at "end".
	    if (indent > 2 && (*p == '}' || STRNCMP(p, "end", 3) == 0))
		indent -= 2;
	    else if (STRNCMP(p, "if", 2) == 0
		    || STRNCMP(p, "wh", 2) == 0
		    || STRNCMP(p, "for", 3) == 0
		    || STRNCMP(p, "try", 3) == 0)
		indent += 2;

	    // Check for defining a function inside this function.
	    // Only recognize "def" inside "def", not inside "function",
	    // For backwards compatibility, see Test_function_python().
	    c = *p;
	    if (is_function_cmd(&p)
		    || (eap->cmdidx == CMD_def && checkforcmd(&p, "def", 3)))
	    {
		if (*p == '!')
		    p = skipwhite(p + 1);
		p += eval_fname_script(p);
		vim_free(trans_function_name(&p, NULL, TRUE, 0, NULL,
								  NULL, NULL));
		if (*skipwhite(p) == '(')
		{
		    if (nesting == MAX_FUNC_NESTING - 1)
			emsg(_(e_function_nesting_too_deep));
		    else
		    {
			++nesting;
			nesting_def[nesting] = (c == 'd');
			nesting_inline[nesting] = FALSE;
			indent += 2;
		    }
		}
	    }

	    if (nesting_def[nesting] ? *p != '#' : *p != '"')
	    {
		// Not a comment line: check for nested inline function.
		end = p + STRLEN(p) - 1;
		while (end > p && VIM_ISWHITE(*end))
		    --end;
		if (end > p + 1 && *end == '{' && VIM_ISWHITE(end[-1]))
		{
		    int	    is_block;

		    // check for trailing "=> {": start of an inline function
		    --end;
		    while (end > p && VIM_ISWHITE(*end))
			--end;
		    is_block = end > p + 2 && end[-1] == '=' && end[0] == '>';
		    if (!is_block)
		    {
			char_u *s = p;

			// check for line starting with "au" for :autocmd or
			// "com" for :command, these can use a {} block
			is_block = checkforcmd_noparen(&s, "autocmd", 2)
				      || checkforcmd_noparen(&s, "command", 3);
		    }

		    if (is_block)
		    {
			if (nesting == MAX_FUNC_NESTING - 1)
			    emsg(_(e_function_nesting_too_deep));
			else
			{
			    ++nesting;
			    nesting_def[nesting] = TRUE;
			    nesting_inline[nesting] = TRUE;
			    indent += 2;
			}
		    }
		}
	    }

	    // Check for ":append", ":change", ":insert".  Not for :def.
	    p = skip_range(p, FALSE, NULL);
	    if (!vim9_function
		&& ((p[0] == 'a' && (!ASCII_ISALPHA(p[1]) || p[1] == 'p'))
		    || (p[0] == 'c'
			&& (!ASCII_ISALPHA(p[1]) || (p[1] == 'h'
				&& (!ASCII_ISALPHA(p[2]) || (p[2] == 'a'
					&& (STRNCMP(&p[3], "nge", 3) != 0
					    || !ASCII_ISALPHA(p[6])))))))
		    || (p[0] == 'i'
			&& (!ASCII_ISALPHA(p[1]) || (p[1] == 'n'
				&& (!ASCII_ISALPHA(p[2])
				    || (p[2] == 's'
					&& (!ASCII_ISALPHA(p[3])
						|| p[3] == 'e'))))))))
		skip_until = vim_strsave((char_u *)".");

	    // Check for ":python <<EOF", ":tcl <<EOF", etc.
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
		// ":python <<" continues until a dot, like ":append"
		p = skipwhite(arg + 2);
		if (STRNCMP(p, "trim", 4) == 0)
		{
		    // Ignore leading white space.
		    p = skipwhite(p + 4);
		    heredoc_trimmed = vim_strnsave(theline,
						 skipwhite(theline) - theline);
		}
		if (*p == NUL)
		    skip_until = vim_strsave((char_u *)".");
		else
		    skip_until = vim_strnsave(p, skiptowhite(p) - p);
		getline_options = GETLINE_NONE;
		is_heredoc = TRUE;
		if (eap->cmdidx == CMD_def)
		    heredoc_concat_len = newlines->ga_len + 1;
	    }

	    // Check for ":cmd v =<< [trim] EOF"
	    //       and ":cmd [a, b] =<< [trim] EOF"
	    //       and "lines =<< [trim] EOF" for Vim9
	    // Where "cmd" can be "let", "var", "final" or "const".
	    arg = skipwhite(skiptowhite(p));
	    if (*arg == '[')
		arg = vim_strchr(arg, ']');
	    if (arg != NULL)
	    {
		int found = (eap->cmdidx == CMD_def && arg[0] == '='
					     && arg[1] == '<' && arg[2] =='<');

		if (!found)
		    // skip over the argument after "cmd"
		    arg = skipwhite(skiptowhite(arg));
		if (found || (arg[0] == '=' && arg[1] == '<' && arg[2] =='<'
			&& (checkforcmd(&p, "let", 2)
			    || checkforcmd(&p, "var", 3)
			    || checkforcmd(&p, "final", 5)
			    || checkforcmd(&p, "const", 5))))
		{
		    p = skipwhite(arg + 3);
		    if (STRNCMP(p, "trim", 4) == 0)
		    {
			// Ignore leading white space.
			p = skipwhite(p + 4);
			heredoc_trimmed = vim_strnsave(theline,
						 skipwhite(theline) - theline);
		    }
		    skip_until = vim_strnsave(p, skiptowhite(p) - p);
		    getline_options = GETLINE_NONE;
		    is_heredoc = TRUE;
		}
	    }
	}

	// Add the line to the function.
	if (ga_grow(newlines, 1 + sourcing_lnum_off) == FAIL)
	    goto theend;

	if (heredoc_concat_len > 0)
	{
	    // For a :def function "python << EOF" concatenates all the lines,
	    // to be used for the instruction later.
	    ga_concat(&heredoc_ga, theline);
	    ga_concat(&heredoc_ga, (char_u *)"\n");
	    p = vim_strsave((char_u *)"");
	}
	else
	{
	    // Copy the line to newly allocated memory.  get_one_sourceline()
	    // allocates 250 bytes per line, this saves 80% on average.  The
	    // cost is an extra alloc/free.
	    p = vim_strsave(theline);
	}
	if (p == NULL)
	    goto theend;
	((char_u **)(newlines->ga_data))[newlines->ga_len++] = p;

	// Add NULL lines for continuation lines, so that the line count is
	// equal to the index in the growarray.
	while (sourcing_lnum_off-- > 0)
	    ((char_u **)(newlines->ga_data))[newlines->ga_len++] = NULL;

	// Check for end of eap->arg.
	if (line_arg != NULL && *line_arg == NUL)
	    line_arg = NULL;
    }

    // Return OK when no error was detected.
    if (!did_emsg)
	ret = OK;

theend:
    vim_free(skip_until);
    vim_free(heredoc_trimmed);
    vim_free(heredoc_ga.ga_data);
    need_wait_return |= saved_wait_return;
    return ret;
}

/*
 * Handle the body of a lambda.  *arg points to the "{", process statements
 * until the matching "}".
 * When not evaluating "newargs" is NULL.
 * When successful "rettv" is set to a funcref.
 */
    static int
lambda_function_body(
	char_u	    **arg,
	typval_T    *rettv,
	evalarg_T   *evalarg,
	garray_T    *newargs,
	garray_T    *argtypes,
	int	    varargs,
	garray_T    *default_args,
	char_u	    *ret_type)
{
    int		evaluate = (evalarg->eval_flags & EVAL_EVALUATE);
    garray_T	*gap = &evalarg->eval_ga;
    garray_T	*freegap = &evalarg->eval_freega;
    ufunc_T	*ufunc = NULL;
    exarg_T	eap;
    garray_T	newlines;
    char_u	*cmdline = NULL;
    int		ret = FAIL;
    char_u	*line_to_free = NULL;
    partial_T	*pt;
    char_u	*name;
    int		lnum_save = -1;
    linenr_T	sourcing_lnum_top = SOURCING_LNUM;

    if (!ends_excmd2(*arg, skipwhite(*arg + 1)))
    {
	semsg(_(e_trailing_characters_str), *arg + 1);
	return FAIL;
    }

    CLEAR_FIELD(eap);
    eap.cmdidx = CMD_block;
    eap.forceit = FALSE;
    eap.cmdlinep = &cmdline;
    eap.skip = !evaluate;
    if (evalarg->eval_cctx != NULL)
	fill_exarg_from_cctx(&eap, evalarg->eval_cctx);
    else
    {
	eap.getline = evalarg->eval_getline;
	eap.cookie = evalarg->eval_cookie;
    }

    ga_init2(&newlines, (int)sizeof(char_u *), 10);
    if (get_function_body(&eap, &newlines, NULL, &line_to_free) == FAIL)
    {
	if (cmdline != line_to_free)
	    vim_free(cmdline);
	goto erret;
    }

    // When inside a lambda must add the function lines to evalarg.eval_ga.
    evalarg->eval_break_count += newlines.ga_len;
    if (gap->ga_itemsize > 0)
    {
	int	idx;
	char_u	*last;
	size_t  plen;
	char_u  *pnl;

	for (idx = 0; idx < newlines.ga_len; ++idx)
	{
	    char_u  *p = skipwhite(((char_u **)newlines.ga_data)[idx]);

	    if (ga_grow(gap, 1) == FAIL || ga_grow(freegap, 1) == FAIL)
		goto erret;

	    // Going to concatenate the lines after parsing.  For an empty or
	    // comment line use an empty string.
	    // Insert NL characters at the start of each line, the string will
	    // be split again later in .get_lambda_tv().
	    if (*p == NUL || vim9_comment_start(p))
		p = (char_u *)"";
	    plen = STRLEN(p);
	    pnl = vim_strnsave((char_u *)"\n", plen + 1);
	    if (pnl != NULL)
		mch_memmove(pnl + 1, p, plen + 1);
	    ((char_u **)gap->ga_data)[gap->ga_len++] = pnl;
	    ((char_u **)freegap->ga_data)[freegap->ga_len++] = pnl;
	}
	if (ga_grow(gap, 1) == FAIL || ga_grow(freegap, 1) == FAIL)
	    goto erret;
	if (eap.nextcmd != NULL)
	    // more is following after the "}", which was skipped
	    last = cmdline;
	else
	    // nothing is following the "}"
	    last = (char_u *)"}";
	plen = STRLEN(last);
	pnl = vim_strnsave((char_u *)"\n", plen + 1);
	if (pnl != NULL)
	    mch_memmove(pnl + 1, last, plen + 1);
	((char_u **)gap->ga_data)[gap->ga_len++] = pnl;
	((char_u **)freegap->ga_data)[freegap->ga_len++] = pnl;
    }

    if (eap.nextcmd != NULL)
    {
	garray_T *tfgap = &evalarg->eval_tofree_ga;

	// Something comes after the "}".
	*arg = eap.nextcmd;

	// "arg" points into cmdline, need to keep the line and free it later.
	if (ga_grow(tfgap, 1) == OK)
	{
	    ((char_u **)(tfgap->ga_data))[tfgap->ga_len++] = cmdline;
	    evalarg->eval_using_cmdline = TRUE;
	    if (cmdline == line_to_free)
		line_to_free = NULL;
	}
    }
    else
	*arg = (char_u *)"";

    if (!evaluate)
    {
	ret = OK;
	goto erret;
    }

    name = get_lambda_name();
    ufunc = alloc_clear(offsetof(ufunc_T, uf_name) + STRLEN(name) + 1);
    if (ufunc == NULL)
	goto erret;
    set_ufunc_name(ufunc, name);
    if (hash_add(&func_hashtab, UF2HIKEY(ufunc)) == FAIL)
	goto erret;
    ufunc->uf_flags = FC_LAMBDA;
    ufunc->uf_refcount = 1;
    ufunc->uf_args = *newargs;
    newargs->ga_data = NULL;
    ufunc->uf_def_args = *default_args;
    default_args->ga_data = NULL;
    ufunc->uf_func_type = &t_func_any;

    // error messages are for the first function line
    lnum_save = SOURCING_LNUM;
    SOURCING_LNUM = sourcing_lnum_top;

    // parse argument types
    if (parse_argument_types(ufunc, argtypes, varargs) == FAIL)
    {
	SOURCING_LNUM = lnum_save;
	goto erret;
    }

    // parse the return type, if any
    if (parse_return_type(ufunc, ret_type) == FAIL)
	goto erret;

    pt = ALLOC_CLEAR_ONE(partial_T);
    if (pt == NULL)
	goto erret;
    pt->pt_func = ufunc;
    pt->pt_refcount = 1;

    ufunc->uf_lines = newlines;
    newlines.ga_data = NULL;
    if (sandbox)
	ufunc->uf_flags |= FC_SANDBOX;
    if (!ASCII_ISUPPER(*ufunc->uf_name))
	ufunc->uf_flags |= FC_VIM9;
    ufunc->uf_script_ctx = current_sctx;
    ufunc->uf_script_ctx_version = current_sctx.sc_version;
    ufunc->uf_script_ctx.sc_lnum += sourcing_lnum_top;
    set_function_type(ufunc);

    function_using_block_scopes(ufunc, evalarg->eval_cstack);

    rettv->vval.v_partial = pt;
    rettv->v_type = VAR_PARTIAL;
    ufunc = NULL;
    ret = OK;

erret:
    if (lnum_save >= 0)
	SOURCING_LNUM = lnum_save;
    vim_free(line_to_free);
    ga_clear_strings(&newlines);
    if (newargs != NULL)
	ga_clear_strings(newargs);
    ga_clear_strings(default_args);
    if (ufunc != NULL)
    {
	func_clear(ufunc, TRUE);
	func_free(ufunc, TRUE);
    }
    return ret;
}

/*
 * Parse a lambda expression and get a Funcref from "*arg" into "rettv".
 * "arg" points to the { in "{arg -> expr}" or the ( in "(arg) => expr"
 * When "types_optional" is TRUE optionally take argument types.
 * Return OK or FAIL.  Returns NOTDONE for dict or {expr}.
 */
    int
get_lambda_tv(
	char_u	    **arg,
	typval_T    *rettv,
	int	    types_optional,
	evalarg_T   *evalarg)
{
    int		evaluate = evalarg != NULL
				      && (evalarg->eval_flags & EVAL_EVALUATE);
    garray_T	newargs;
    garray_T	newlines;
    garray_T	*pnewargs;
    garray_T	argtypes;
    garray_T	default_args;
    ufunc_T	*fp = NULL;
    partial_T   *pt = NULL;
    int		varargs;
    char_u	*ret_type = NULL;
    int		ret;
    char_u	*s;
    char_u	*start, *end;
    int		*old_eval_lavars = eval_lavars_used;
    int		eval_lavars = FALSE;
    char_u	*tofree1 = NULL;
    char_u	*tofree2 = NULL;
    int		equal_arrow = **arg == '(';
    int		white_error = FALSE;
    int		called_emsg_start = called_emsg;

    if (equal_arrow && !in_vim9script())
	return NOTDONE;

    ga_init(&newargs);
    ga_init(&newlines);

    // First, check if this is really a lambda expression. "->" or "=>" must
    // be found after the arguments.
    s = *arg + 1;
    ret = get_function_args(&s, equal_arrow ? ')' : '-', NULL,
	    types_optional ? &argtypes : NULL, types_optional, evalarg,
					NULL, &default_args, TRUE, NULL, NULL);
    if (ret == FAIL || skip_arrow(s, equal_arrow, &ret_type, NULL) == NULL)
    {
	if (types_optional)
	    ga_clear_strings(&argtypes);
	return called_emsg == called_emsg_start ? NOTDONE : FAIL;
    }

    // Parse the arguments for real.
    if (evaluate)
	pnewargs = &newargs;
    else
	pnewargs = NULL;
    *arg += 1;
    ret = get_function_args(arg, equal_arrow ? ')' : '-', pnewargs,
	    types_optional ? &argtypes : NULL, types_optional, evalarg,
					    &varargs, &default_args,
					    FALSE, NULL, NULL);
    if (ret == FAIL
		  || (s = skip_arrow(*arg, equal_arrow, &ret_type,
		equal_arrow || in_vim9script() ? &white_error : NULL)) == NULL)
    {
	if (types_optional)
	    ga_clear_strings(&argtypes);
	ga_clear_strings(&newargs);
	return white_error ? FAIL : NOTDONE;
    }
    *arg = s;

    // Skipping over linebreaks may make "ret_type" invalid, make a copy.
    if (ret_type != NULL)
    {
	ret_type = vim_strsave(ret_type);
	tofree2 = ret_type;
    }

    // Set up a flag for checking local variables and arguments.
    if (evaluate)
	eval_lavars_used = &eval_lavars;

    *arg = skipwhite_and_linebreak(*arg, evalarg);

    // Recognize "{" as the start of a function body.
    if (equal_arrow && **arg == '{')
    {
	if (evalarg == NULL)
	    // cannot happen?
	    goto theend;
	if (lambda_function_body(arg, rettv, evalarg, pnewargs,
			   types_optional ? &argtypes : NULL, varargs,
			   &default_args, ret_type) == FAIL)
	    goto errret;
	goto theend;
    }
    if (default_args.ga_len > 0)
    {
	emsg(_(e_cannot_use_default_values_in_lambda));
	goto errret;
    }

    // Get the start and the end of the expression.
    start = *arg;
    ret = skip_expr_concatenate(arg, &start, &end, evalarg);
    if (ret == FAIL)
	goto errret;
    if (evalarg != NULL)
    {
	// avoid that the expression gets freed when another line break follows
	tofree1 = evalarg->eval_tofree;
	evalarg->eval_tofree = NULL;
    }

    if (!equal_arrow)
    {
	*arg = skipwhite_and_linebreak(*arg, evalarg);
	if (**arg != '}')
	{
	    semsg(_(e_expected_right_curly_str), *arg);
	    goto errret;
	}
	++*arg;
    }

    if (evaluate)
    {
	int	    len;
	int	    flags = FC_LAMBDA;
	char_u	    *p;
	char_u	    *line_end;
	char_u	    *name = get_lambda_name();

	fp = alloc_clear(offsetof(ufunc_T, uf_name) + STRLEN(name) + 1);
	if (fp == NULL)
	    goto errret;
	fp->uf_def_status = UF_NOT_COMPILED;
	pt = ALLOC_CLEAR_ONE(partial_T);
	if (pt == NULL)
	    goto errret;

	ga_init2(&newlines, (int)sizeof(char_u *), 1);
	if (ga_grow(&newlines, 1) == FAIL)
	    goto errret;

	// If there are line breaks, we need to split up the string.
	line_end = vim_strchr(start, '\n');
	if (line_end == NULL || line_end > end)
	    line_end = end;

	// Add "return " before the expression (or the first line).
	len = 7 + (int)(line_end - start) + 1;
	p = alloc(len);
	if (p == NULL)
	    goto errret;
	((char_u **)(newlines.ga_data))[newlines.ga_len++] = p;
	STRCPY(p, "return ");
	vim_strncpy(p + 7, start, line_end - start);

	if (line_end != end)
	{
	    // Add more lines, split by line breaks.  Thus is used when a
	    // lambda with { cmds } is encountered.
	    while (*line_end == '\n')
	    {
		if (ga_grow(&newlines, 1) == FAIL)
		    goto errret;
		start = line_end + 1;
		line_end = vim_strchr(start, '\n');
		if (line_end == NULL)
		    line_end = end;
		((char_u **)(newlines.ga_data))[newlines.ga_len++] =
					 vim_strnsave(start, line_end - start);
	    }
	}

	if (strstr((char *)p + 7, "a:") == NULL)
	    // No a: variables are used for sure.
	    flags |= FC_NOARGS;

	fp->uf_refcount = 1;
	set_ufunc_name(fp, name);
	fp->uf_args = newargs;
	ga_init(&fp->uf_def_args);
	if (types_optional)
	{
	    if (parse_argument_types(fp, &argtypes,
					   in_vim9script() && varargs) == FAIL)
		goto errret;
	    if (ret_type != NULL)
	    {
		fp->uf_ret_type = parse_type(&ret_type,
						      &fp->uf_type_list, TRUE);
		if (fp->uf_ret_type == NULL)
		    goto errret;
	    }
	    else
		fp->uf_ret_type = &t_unknown;
	}

	fp->uf_lines = newlines;
	if (current_funccal != NULL && eval_lavars)
	{
	    flags |= FC_CLOSURE;
	    if (register_closure(fp) == FAIL)
		goto errret;
	}

#ifdef FEAT_PROFILE
	if (prof_def_func())
	    func_do_profile(fp);
#endif
	if (sandbox)
	    flags |= FC_SANDBOX;
	// In legacy script a lambda can be called with more args than
	// uf_args.ga_len.  In Vim9 script "...name" has to be used.
	fp->uf_varargs = !in_vim9script() || varargs;
	fp->uf_flags = flags;
	fp->uf_calls = 0;
	fp->uf_script_ctx = current_sctx;
	fp->uf_script_ctx.sc_lnum += SOURCING_LNUM - newlines.ga_len + 1;

	function_using_block_scopes(fp, evalarg->eval_cstack);

	pt->pt_func = fp;
	pt->pt_refcount = 1;
	rettv->vval.v_partial = pt;
	rettv->v_type = VAR_PARTIAL;

	hash_add(&func_hashtab, UF2HIKEY(fp));
    }

theend:
    eval_lavars_used = old_eval_lavars;
    if (evalarg != NULL && evalarg->eval_tofree == NULL)
	evalarg->eval_tofree = tofree1;
    else
	vim_free(tofree1);
    vim_free(tofree2);
    if (types_optional)
	ga_clear_strings(&argtypes);

    return OK;

errret:
    ga_clear_strings(&newargs);
    ga_clear_strings(&newlines);
    ga_clear_strings(&default_args);
    if (types_optional)
    {
	ga_clear_strings(&argtypes);
	if (fp != NULL)
	    vim_free(fp->uf_arg_types);
    }
    vim_free(fp);
    vim_free(pt);
    if (evalarg != NULL && evalarg->eval_tofree == NULL)
	evalarg->eval_tofree = tofree1;
    else
	vim_free(tofree1);
    vim_free(tofree2);
    eval_lavars_used = old_eval_lavars;
    return FAIL;
}

/*
 * Check if "name" is a variable of type VAR_FUNC.  If so, return the function
 * name it contains, otherwise return "name".
 * If "partialp" is not NULL, and "name" is of type VAR_PARTIAL also set
 * "partialp".
 * If "type" is not NULL and a Vim9 script-local variable is found look up the
 * type of the variable.
 * If "found_var" is not NULL and a variable was found set it to TRUE.
 */
    char_u *
deref_func_name(
	char_u	    *name,
	int	    *lenp,
	partial_T   **partialp,
	type_T	    **type,
	int	    no_autoload,
	int	    *found_var)
{
    dictitem_T	*v;
    typval_T	*tv = NULL;
    int		cc;
    char_u	*s = NULL;
    hashtab_T	*ht;
    int		did_type = FALSE;

    if (partialp != NULL)
	*partialp = NULL;

    cc = name[*lenp];
    name[*lenp] = NUL;

    v = find_var_also_in_script(name, &ht, no_autoload);
    name[*lenp] = cc;
    if (v != NULL)
    {
	tv = &v->di_tv;
    }
    else if (in_vim9script() || STRNCMP(name, "s:", 2) == 0)
    {
	imported_T  *import;
	char_u	    *p = name;
	int	    len = *lenp;

	if (STRNCMP(name, "s:", 2) == 0)
	{
	    p = name + 2;
	    len -= 2;
	}
	import = find_imported(p, len, NULL);

	// imported function from another script
	if (import != NULL)
	{
	    name[len] = NUL;
	    semsg(_(e_cannot_use_str_itself_it_is_imported), name);
	    name[len] = cc;
	    *lenp = 0;
	    return (char_u *)"";	// just in case
	}
    }

    if (tv != NULL)
    {
	if (found_var != NULL)
	    *found_var = TRUE;
	if (tv->v_type == VAR_FUNC)
	{
	    if (tv->vval.v_string == NULL)
	    {
		*lenp = 0;
		return (char_u *)"";	// just in case
	    }
	    s = tv->vval.v_string;
	    *lenp = (int)STRLEN(s);
	}

	if (tv->v_type == VAR_PARTIAL)
	{
	    partial_T *pt = tv->vval.v_partial;

	    if (pt == NULL)
	    {
		*lenp = 0;
		return (char_u *)"";	// just in case
	    }
	    if (partialp != NULL)
		*partialp = pt;
	    s = partial_name(pt);
	    *lenp = (int)STRLEN(s);
	}

	if (s != NULL)
	{
	    if (!did_type && type != NULL && ht == get_script_local_ht())
	    {
		svar_T  *sv = find_typval_in_script(tv, 0);

		if (sv != NULL)
		    *type = sv->sv_type;
	    }
	    return s;
	}
    }

    return name;
}

/*
 * Give an error message with a function name.  Handle <SNR> things.
 * "ermsg" is to be passed without translation, use N_() instead of _().
 */
    void
emsg_funcname(char *ermsg, char_u *name)
{
    char_u	*p;

    if (*name == K_SPECIAL)
	p = concat_str((char_u *)"<SNR>", name + 3);
    else
	p = name;
    semsg(_(ermsg), p);
    if (p != name)
	vim_free(p);
}

/*
 * Allocate a variable for the result of a function.
 * Return OK or FAIL.
 */
    int
get_func_tv(
    char_u	*name,		// name of the function
    int		len,		// length of "name" or -1 to use strlen()
    typval_T	*rettv,
    char_u	**arg,		// argument, pointing to the '('
    evalarg_T	*evalarg,	// for line continuation
    funcexe_T	*funcexe)	// various values
{
    char_u	*argp;
    int		ret = OK;
    typval_T	argvars[MAX_FUNC_ARGS + 1];	// vars for arguments
    int		argcount = 0;		// number of arguments found
    int		vim9script = in_vim9script();

    /*
     * Get the arguments.
     */
    argp = *arg;
    while (argcount < MAX_FUNC_ARGS - (funcexe->fe_partial == NULL ? 0
					       : funcexe->fe_partial->pt_argc))
    {
	// skip the '(' or ',' and possibly line breaks
	argp = skipwhite_and_linebreak(argp + 1, evalarg);

	if (*argp == ')' || *argp == ',' || *argp == NUL)
	    break;
	if (eval1(&argp, &argvars[argcount], evalarg) == FAIL)
	{
	    ret = FAIL;
	    break;
	}
	++argcount;
	// The comma should come right after the argument, but this wasn't
	// checked previously, thus only enforce it in Vim9 script.
	if (vim9script)
	{
	    if (*argp != ',' && *skipwhite(argp) == ',')
	    {
		semsg(_(e_no_white_space_allowed_before_str_str), ",", argp);
		ret = FAIL;
		break;
	    }
	}
	else
	    argp = skipwhite(argp);
	if (*argp != ',')
	    break;
	if (vim9script && !IS_WHITE_OR_NUL(argp[1]))
	{
	    semsg(_(e_white_space_required_after_str_str), ",", argp);
	    ret = FAIL;
	    break;
	}
    }
    argp = skipwhite_and_linebreak(argp, evalarg);
    if (*argp == ')')
	++argp;
    else
	ret = FAIL;

    if (ret == OK)
    {
	int	i = 0;
	int	did_emsg_before = did_emsg;

	if (get_vim_var_nr(VV_TESTING))
	{
	    // Prepare for calling test_garbagecollect_now(), need to know
	    // what variables are used on the call stack.
	    if (funcargs.ga_itemsize == 0)
		ga_init2(&funcargs, (int)sizeof(typval_T *), 50);
	    for (i = 0; i < argcount; ++i)
		if (ga_grow(&funcargs, 1) == OK)
		    ((typval_T **)funcargs.ga_data)[funcargs.ga_len++] =
								  &argvars[i];
	}

	ret = call_func(name, len, rettv, argcount, argvars, funcexe);
	if (in_vim9script() && did_emsg > did_emsg_before)
	{
	    // An error in a builtin function does not return FAIL, but we do
	    // want to abort further processing if an error was given.
	    ret = FAIL;
	    clear_tv(rettv);
	}

	funcargs.ga_len -= i;
    }
    else if (!aborting())
    {
	if (argcount == MAX_FUNC_ARGS)
	    emsg_funcname(e_too_many_arguments_for_function_str_2, name);
	else
	    emsg_funcname(e_invalid_arguments_for_function_str, name);
    }

    while (--argcount >= 0)
	clear_tv(&argvars[argcount]);

    if (in_vim9script())
	*arg = argp;
    else
	*arg = skipwhite(argp);
    return ret;
}

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
    char_u *
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
	if (eval_fname_sid(name))	// "<SID>" or "s:"
	{
	    if (current_sctx.sc_sid <= 0)
		*error = FCERR_SCRIPT;
	    else
	    {
		sprintf((char *)fname_buf + 3, "%ld_",
						    (long)current_sctx.sc_sid);
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
	    fname = alloc(i + STRLEN(name + llen) + 1);
	    if (fname == NULL)
		*error = FCERR_OTHER;
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
 * Find a function "name" in script "sid".
 */
    static ufunc_T *
find_func_with_sid(char_u *name, int sid)
{
    hashitem_T	*hi;
    char_u	buffer[200];

    buffer[0] = K_SPECIAL;
    buffer[1] = KS_EXTRA;
    buffer[2] = (int)KE_SNR;
    vim_snprintf((char *)buffer + 3, sizeof(buffer) - 3, "%ld_%s",
							      (long)sid, name);
    hi = hash_find(&func_hashtab, buffer);
    if (!HASHITEM_EMPTY(hi))
	return HI2UF(hi);

    return NULL;
}

/*
 * Find a function by name, return pointer to it in ufuncs.
 * When "is_global" is true don't find script-local or imported functions.
 * Return NULL for unknown function.
 */
    ufunc_T *
find_func_even_dead(char_u *name, int is_global, cctx_T *cctx UNUSED)
{
    hashitem_T	*hi;
    ufunc_T	*func;

    if (!is_global)
    {
	int	find_script_local = in_vim9script() && eval_isnamec1(*name)
					   && (name[1] != ':' || *name == 's');

	if (find_script_local)
	{
	    // Find script-local function before global one.
	    func = find_func_with_sid(name[0] == 's' && name[1] == ':'
				       ? name + 2 : name, current_sctx.sc_sid);
	    if (func != NULL)
		return func;
	}
    }

    hi = hash_find(&func_hashtab,
				STRNCMP(name, "g:", 2) == 0 ? name + 2 : name);
    if (!HASHITEM_EMPTY(hi))
	return HI2UF(hi);

    return NULL;
}

/*
 * Find a function by name, return pointer to it in ufuncs.
 * "cctx" is passed in a :def function to find imported functions.
 * Return NULL for unknown or dead function.
 */
    ufunc_T *
find_func(char_u *name, int is_global, cctx_T *cctx)
{
    ufunc_T	*fp = find_func_even_dead(name, is_global, cctx);

    if (fp != NULL && (fp->uf_flags & FC_DEAD) == 0)
	return fp;
    return NULL;
}

/*
 * Return TRUE if "ufunc" is a global function.
 */
    int
func_is_global(ufunc_T *ufunc)
{
    return ufunc->uf_name[0] != K_SPECIAL;
}

/*
 * Copy the function name of "fp" to buffer "buf".
 * "buf" must be able to hold the function name plus three bytes.
 * Takes care of script-local function names.
 */
    static void
cat_func_name(char_u *buf, ufunc_T *fp)
{
    if (!func_is_global(fp))
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
 * Free "fc".
 */
    static void
free_funccal(funccall_T *fc)
{
    int	i;

    for (i = 0; i < fc->fc_funcs.ga_len; ++i)
    {
	ufunc_T *fp = ((ufunc_T **)(fc->fc_funcs.ga_data))[i];

	// When garbage collecting a funccall_T may be freed before the
	// function that references it, clear its uf_scoped field.
	// The function may have been redefined and point to another
	// funccall_T, don't clear it then.
	if (fp != NULL && fp->uf_scoped == fc)
	    fp->uf_scoped = NULL;
    }
    ga_clear(&fc->fc_funcs);

    func_ptr_unref(fc->func);
    vim_free(fc);
}

/*
 * Free "fc" and what it contains.
 * Can be called only when "fc" is kept beyond the period of it called,
 * i.e. after cleanup_function_call(fc).
 */
   static void
free_funccal_contents(funccall_T *fc)
{
    listitem_T	*li;

    // Free all l: variables.
    vars_clear(&fc->l_vars.dv_hashtab);

    // Free all a: variables.
    vars_clear(&fc->l_avars.dv_hashtab);

    // Free the a:000 variables.
    FOR_ALL_LIST_ITEMS(&fc->l_varlist, li)
	clear_tv(&li->li_tv);

    free_funccal(fc);
}

/*
 * Handle the last part of returning from a function: free the local hashtable.
 * Unless it is still in use by a closure.
 */
    static void
cleanup_function_call(funccall_T *fc)
{
    int	may_free_fc = fc->fc_refcount <= 0;
    int	free_fc = TRUE;

    current_funccal = fc->caller;

    // Free all l: variables if not referred.
    if (may_free_fc && fc->l_vars.dv_refcount == DO_NOT_FREE_CNT)
	vars_clear(&fc->l_vars.dv_hashtab);
    else
	free_fc = FALSE;

    // If the a:000 list and the l: and a: dicts are not referenced and
    // there is no closure using it, we can free the funccall_T and what's
    // in it.
    if (may_free_fc && fc->l_avars.dv_refcount == DO_NOT_FREE_CNT)
	vars_clear_ext(&fc->l_avars.dv_hashtab, FALSE);
    else
    {
	int	    todo;
	hashitem_T  *hi;
	dictitem_T  *di;

	free_fc = FALSE;

	// Make a copy of the a: variables, since we didn't do that above.
	todo = (int)fc->l_avars.dv_hashtab.ht_used;
	for (hi = fc->l_avars.dv_hashtab.ht_array; todo > 0; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		di = HI2DI(hi);
		copy_tv(&di->di_tv, &di->di_tv);
	    }
	}
    }

    if (may_free_fc && fc->l_varlist.lv_refcount == DO_NOT_FREE_CNT)
	fc->l_varlist.lv_first = NULL;
    else
    {
	listitem_T *li;

	free_fc = FALSE;

	// Make a copy of the a:000 items, since we didn't do that above.
	FOR_ALL_LIST_ITEMS(&fc->l_varlist, li)
	    copy_tv(&li->li_tv, &li->li_tv);
    }

    if (free_fc)
	free_funccal(fc);
    else
    {
	static int made_copy = 0;

	// "fc" is still in use.  This can happen when returning "a:000",
	// assigning "l:" to a global variable or defining a closure.
	// Link "fc" in the list for garbage collection later.
	fc->caller = previous_funccal;
	previous_funccal = fc;

	if (want_garbage_collect)
	    // If garbage collector is ready, clear count.
	    made_copy = 0;
	else if (++made_copy >= (int)((4096 * 1024) / sizeof(*fc)))
	{
	    // We have made a lot of copies, worth 4 Mbyte.  This can happen
	    // when repetitively calling a function that creates a reference to
	    // itself somehow.  Call the garbage collector soon to avoid using
	    // too much memory.
	    made_copy = 0;
	    want_garbage_collect = TRUE;
	}
    }
}

/*
 * Return TRUE if "name" is a numbered function, ignoring a "g:" prefix.
 */
    static int
numbered_function(char_u *name)
{
    return isdigit(*name)
	    || (name[0] == 'g' && name[1] == ':' && isdigit(name[2]));
}

/*
 * There are two kinds of function names:
 * 1. ordinary names, function defined with :function or :def
 * 2. numbered functions and lambdas
 * For the first we only count the name stored in func_hashtab as a reference,
 * using function() does not count as a reference, because the function is
 * looked up by name.
 */
    int
func_name_refcount(char_u *name)
{
    return numbered_function(name) || *name == '<';
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
		free_funccal_contents(fc);
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
    hashitem_T	*hi;

    // Return if it was already virtually deleted.
    if (fp->uf_flags & FC_DEAD)
	return FALSE;

    hi = hash_find(&func_hashtab, UF2HIKEY(fp));
    if (!HASHITEM_EMPTY(hi))
    {
	// When there is a def-function index do not actually remove the
	// function, so we can find the index when defining the function again.
	// Do remove it when it's a copy.
	if (fp->uf_def_status == UF_COMPILED && (fp->uf_flags & FC_COPY) == 0)
	{
	    fp->uf_flags |= FC_DEAD;
	    return FALSE;
	}
	hash_remove(&func_hashtab, hi);
	fp->uf_flags |= FC_DELETED;
	return TRUE;
    }
    return FALSE;
}

    static void
func_clear_items(ufunc_T *fp)
{
    ga_clear_strings(&(fp->uf_args));
    ga_clear_strings(&(fp->uf_def_args));
    ga_clear_strings(&(fp->uf_lines));
    VIM_CLEAR(fp->uf_arg_types);
    VIM_CLEAR(fp->uf_block_ids);
    VIM_CLEAR(fp->uf_va_name);
    clear_type_list(&fp->uf_type_list);

    // Increment the refcount of this function to avoid it being freed
    // recursively when the partial is freed.
    fp->uf_refcount += 3;
    partial_unref(fp->uf_partial);
    fp->uf_partial = NULL;
    fp->uf_refcount -= 3;

#ifdef FEAT_LUA
    if (fp->uf_cb_free != NULL)
    {
	fp->uf_cb_free(fp->uf_cb_state);
	fp->uf_cb_free = NULL;
    }

    fp->uf_cb_state = NULL;
    fp->uf_cb = NULL;
#endif
#ifdef FEAT_PROFILE
    VIM_CLEAR(fp->uf_tml_count);
    VIM_CLEAR(fp->uf_tml_total);
    VIM_CLEAR(fp->uf_tml_self);
#endif
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

    // clear this function
    func_clear_items(fp);
    funccal_unref(fp->uf_scoped, fp, force);
    unlink_def_function(fp);
}

/*
 * Free a function and remove it from the list of functions.  Does not free
 * what a function contains, call func_clear() first.
 * When "force" is TRUE we are exiting.
 * Returns OK when the function was actually freed.
 */
    static int
func_free(ufunc_T *fp, int force)
{
    // Only remove it when not done already, otherwise we would remove a newer
    // version of the function with the same name.
    if ((fp->uf_flags & (FC_DELETED | FC_REMOVED)) == 0)
	func_remove(fp);

    if ((fp->uf_flags & FC_DEAD) == 0 || force)
    {
	if (fp->uf_dfunc_idx > 0)
	    unlink_def_function(fp);
	VIM_CLEAR(fp->uf_name_exp);
	vim_free(fp);
	return OK;
    }
    return FAIL;
}

/*
 * Free all things that a function contains and free the function itself.
 * When "force" is TRUE we are exiting.
 */
    void
func_clear_free(ufunc_T *fp, int force)
{
    func_clear(fp, force);
    if (force || fp->uf_dfunc_idx == 0 || func_name_refcount(fp->uf_name)
						   || (fp->uf_flags & FC_COPY))
	func_free(fp, force);
    else
	fp->uf_flags |= FC_DEAD;
}

/*
 * Copy already defined function "lambda" to a new function with name "global".
 * This is for when a compiled function defines a global function.
 */
    int
copy_func(char_u *lambda, char_u *global, ectx_T *ectx)
{
    ufunc_T *ufunc = find_func_even_dead(lambda, TRUE, NULL);
    ufunc_T *fp = NULL;

    if (ufunc == NULL)
    {
	semsg(_(e_lambda_function_not_found_str), lambda);
	return FAIL;
    }

    fp = find_func(global, TRUE, NULL);
    if (fp != NULL)
    {
	// TODO: handle ! to overwrite
	semsg(_(e_function_str_already_exists_add_bang_to_replace), global);
	return FAIL;
    }

    fp = alloc_clear(offsetof(ufunc_T, uf_name) + STRLEN(global) + 1);
    if (fp == NULL)
	return FAIL;

    fp->uf_varargs = ufunc->uf_varargs;
    fp->uf_flags = (ufunc->uf_flags & ~FC_VIM9) | FC_COPY;
    fp->uf_def_status = ufunc->uf_def_status;
    fp->uf_dfunc_idx = ufunc->uf_dfunc_idx;
    if (ga_copy_strings(&ufunc->uf_args, &fp->uf_args) == FAIL
	    || ga_copy_strings(&ufunc->uf_def_args, &fp->uf_def_args)
								    == FAIL
	    || ga_copy_strings(&ufunc->uf_lines, &fp->uf_lines) == FAIL)
	goto failed;

    fp->uf_name_exp = ufunc->uf_name_exp == NULL ? NULL
					 : vim_strsave(ufunc->uf_name_exp);
    if (ufunc->uf_arg_types != NULL)
    {
	fp->uf_arg_types = ALLOC_MULT(type_T *, fp->uf_args.ga_len);
	if (fp->uf_arg_types == NULL)
	    goto failed;
	mch_memmove(fp->uf_arg_types, ufunc->uf_arg_types,
				    sizeof(type_T *) * fp->uf_args.ga_len);
    }
    if (ufunc->uf_va_name != NULL)
    {
	fp->uf_va_name = vim_strsave(ufunc->uf_va_name);
	if (fp->uf_va_name == NULL)
	    goto failed;
    }
    fp->uf_ret_type = ufunc->uf_ret_type;

    fp->uf_refcount = 1;
    STRCPY(fp->uf_name, global);
    hash_add(&func_hashtab, UF2HIKEY(fp));

    // the referenced dfunc_T is now used one more time
    link_def_function(fp);

    // Create a partial to store the context of the function where it was
    // instantiated.  Only needs to be done once.  Do this on the original
    // function, "dfunc->df_ufunc" will point to it.
    if ((ufunc->uf_flags & FC_CLOSURE) && ufunc->uf_partial == NULL)
    {
	partial_T   *pt = ALLOC_CLEAR_ONE(partial_T);

	if (pt == NULL)
	    goto failed;
	if (fill_partial_and_closure(pt, ufunc, ectx) == FAIL)
	{
            vim_free(pt);
	    goto failed;
	}
	ufunc->uf_partial = pt;
	--pt->pt_refcount;  // not actually referenced here
    }

    return OK;

failed:
    func_clear_free(fp, TRUE);
    return FAIL;
}

static int	funcdepth = 0;

/*
 * Increment the function call depth count.
 * Return FAIL when going over 'maxfuncdepth'.
 * Otherwise return OK, must call funcdepth_decrement() later!
 */
    int
funcdepth_increment(void)
{
    if (funcdepth >= p_mfd)
    {
	emsg(_(e_function_call_depth_is_higher_than_macfuncdepth));
	return FAIL;
    }
    ++funcdepth;
    return OK;
}

    void
funcdepth_decrement(void)
{
    --funcdepth;
}

/*
 * Get the current function call depth.
 */
    int
funcdepth_get(void)
{
    return funcdepth;
}

/*
 * Restore the function call depth.  This is for cases where there is no
 * guarantee funcdepth_decrement() can be called exactly the same number of
 * times as funcdepth_increment().
 */
    void
funcdepth_restore(int depth)
{
    funcdepth = depth;
}

/*
 * Call a user function.
 */
    static void
call_user_func(
    ufunc_T	*fp,		// pointer to function
    int		argcount,	// nr of args
    typval_T	*argvars,	// arguments
    typval_T	*rettv,		// return value
    funcexe_T	*funcexe,	// context
    dict_T	*selfdict)	// Dictionary for "self"
{
    sctx_T	save_current_sctx;
    int		using_sandbox = FALSE;
    funccall_T	*fc;
    int		save_did_emsg;
    int		default_arg_err = FALSE;
    dictitem_T	*v;
    int		fixvar_idx = 0;	// index in fixvar[]
    int		i;
    int		ai;
    int		islambda = FALSE;
    char_u	numbuf[NUMBUFLEN];
    char_u	*name;
    typval_T	*tv_to_free[MAX_FUNC_ARGS];
    int		tv_to_free_len = 0;
#ifdef FEAT_PROFILE
    profinfo_T	profile_info;
#endif
    ESTACK_CHECK_DECLARATION

#ifdef FEAT_PROFILE
    CLEAR_FIELD(profile_info);
#endif

    // If depth of calling is getting too high, don't execute the function.
    if (funcdepth_increment() == FAIL)
    {
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = -1;
	return;
    }

    line_breakcheck();		// check for CTRL-C hit

    fc = ALLOC_CLEAR_ONE(funccall_T);
    if (fc == NULL)
	return;
    fc->caller = current_funccal;
    current_funccal = fc;
    fc->func = fp;
    fc->rettv = rettv;
    fc->level = ex_nesting_level;
    // Check if this function has a breakpoint.
    fc->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name, (linenr_T)0);
    fc->dbg_tick = debug_tick;
    // Set up fields for closure.
    ga_init2(&fc->fc_funcs, sizeof(ufunc_T *), 1);
    func_ptr_ref(fp);

    if (fp->uf_def_status != UF_NOT_COMPILED)
    {
#ifdef FEAT_PROFILE
	ufunc_T *caller = fc->caller == NULL ? NULL : fc->caller->func;
#endif
	// Execute the function, possibly compiling it first.
#ifdef FEAT_PROFILE
	if (do_profiling == PROF_YES)
	    profile_may_start_func(&profile_info, fp, caller);
#endif
	call_def_function(fp, argcount, argvars, funcexe->fe_partial, rettv);
	funcdepth_decrement();
#ifdef FEAT_PROFILE
	if (do_profiling == PROF_YES && (fp->uf_profiling
				  || (caller != NULL && caller->uf_profiling)))
	    profile_may_end_func(&profile_info, fp, caller);
#endif
	current_funccal = fc->caller;
	free_funccal(fc);
	return;
    }

    islambda = fp->uf_flags & FC_LAMBDA;

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
	// Set l:self to "selfdict".  Use "name" to avoid a warning from
	// some compiler that checks the destination size.
	v = &fc->fixvar[fixvar_idx++].var;
	name = v->di_key;
	STRCPY(name, "self");
	v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
	hash_add(&fc->l_vars.dv_hashtab, DI2HIKEY(v));
	v->di_tv.v_type = VAR_DICT;
	v->di_tv.v_lock = 0;
	v->di_tv.vval.v_dict = selfdict;
	++selfdict->dv_refcount;
    }

    /*
     * Init a: variables, unless none found (in lambda).
     * Set a:0 to "argcount" less number of named arguments, if >= 0.
     * Set a:000 to a list with room for the "..." arguments.
     */
    init_var_dict(&fc->l_avars, &fc->l_avars_var, VAR_SCOPE);
    if ((fp->uf_flags & FC_NOARGS) == 0)
	add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "0",
				(varnumber_T)(argcount >= fp->uf_args.ga_len
				    ? argcount - fp->uf_args.ga_len : 0));
    fc->l_avars.dv_lock = VAR_FIXED;
    if ((fp->uf_flags & FC_NOARGS) == 0)
    {
	// Use "name" to avoid a warning from some compiler that checks the
	// destination size.
	v = &fc->fixvar[fixvar_idx++].var;
	name = v->di_key;
	STRCPY(name, "000");
	v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
	hash_add(&fc->l_avars.dv_hashtab, DI2HIKEY(v));
	v->di_tv.v_type = VAR_LIST;
	v->di_tv.v_lock = VAR_FIXED;
	v->di_tv.vval.v_list = &fc->l_varlist;
    }
    CLEAR_FIELD(fc->l_varlist);
    fc->l_varlist.lv_refcount = DO_NOT_FREE_CNT;
    fc->l_varlist.lv_lock = VAR_FIXED;

    /*
     * Set a:firstline to "firstline" and a:lastline to "lastline".
     * Set a:name to named arguments.
     * Set a:N to the "..." arguments.
     * Skipped when no a: variables used (in lambda).
     */
    if ((fp->uf_flags & FC_NOARGS) == 0)
    {
	add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "firstline",
					   (varnumber_T)funcexe->fe_firstline);
	add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "lastline",
					    (varnumber_T)funcexe->fe_lastline);
    }
    for (i = 0; i < argcount || i < fp->uf_args.ga_len; ++i)
    {
	int	    addlocal = FALSE;
	typval_T    def_rettv;
	int	    isdefault = FALSE;

	ai = i - fp->uf_args.ga_len;
	if (ai < 0)
	{
	    // named argument a:name
	    name = FUNCARG(fp, i);
	    if (islambda)
		addlocal = TRUE;

	    // evaluate named argument default expression
	    isdefault = ai + fp->uf_def_args.ga_len >= 0
		       && (i >= argcount || (argvars[i].v_type == VAR_SPECIAL
				   && argvars[i].vval.v_number == VVAL_NONE));
	    if (isdefault)
	    {
		char_u	    *default_expr = NULL;

		def_rettv.v_type = VAR_NUMBER;
		def_rettv.vval.v_number = -1;

		default_expr = ((char_u **)(fp->uf_def_args.ga_data))
						 [ai + fp->uf_def_args.ga_len];
		if (eval1(&default_expr, &def_rettv, &EVALARG_EVALUATE) == FAIL)
		{
		    default_arg_err = 1;
		    break;
		}
	    }
	}
	else
	{
	    if ((fp->uf_flags & FC_NOARGS) != 0)
		// Bail out if no a: arguments used (in lambda).
		break;

	    // "..." argument a:1, a:2, etc.
	    sprintf((char *)numbuf, "%d", ai + 1);
	    name = numbuf;
	}
	if (fixvar_idx < FIXVAR_CNT && STRLEN(name) <= VAR_SHORT_LEN)
	{
	    v = &fc->fixvar[fixvar_idx++].var;
	    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
	    STRCPY(v->di_key, name);
	}
	else
	{
	    v = dictitem_alloc(name);
	    if (v == NULL)
		break;
	    v->di_flags |= DI_FLAGS_RO | DI_FLAGS_FIX;
	}

	// Note: the values are copied directly to avoid alloc/free.
	// "argvars" must have VAR_FIXED for v_lock.
	v->di_tv = isdefault ? def_rettv : argvars[i];
	v->di_tv.v_lock = VAR_FIXED;

	if (isdefault)
	    // Need to free this later, no matter where it's stored.
	    tv_to_free[tv_to_free_len++] = &v->di_tv;

	if (addlocal)
	{
	    // Named arguments should be accessed without the "a:" prefix in
	    // lambda expressions.  Add to the l: dict.
	    copy_tv(&v->di_tv, &v->di_tv);
	    hash_add(&fc->l_vars.dv_hashtab, DI2HIKEY(v));
	}
	else
	    hash_add(&fc->l_avars.dv_hashtab, DI2HIKEY(v));

	if (ai >= 0 && ai < MAX_FUNC_ARGS)
	{
	    listitem_T *li = &fc->l_listitems[ai];

	    li->li_tv = argvars[i];
	    li->li_tv.v_lock = VAR_FIXED;
	    list_append(&fc->l_varlist, li);
	}
    }

    // Don't redraw while executing the function.
    ++RedrawingDisabled;

    if (fp->uf_flags & FC_SANDBOX)
    {
	using_sandbox = TRUE;
	++sandbox;
    }

    estack_push_ufunc(fp, 1);
    ESTACK_CHECK_SETUP
    if (p_verbose >= 12)
    {
	++no_wait_return;
	verbose_enter_scroll();

	smsg(_("calling %s"), SOURCING_NAME);
	if (p_verbose >= 14)
	{
	    char_u	buf[MSG_BUF_LEN];
	    char_u	numbuf2[NUMBUFLEN];
	    char_u	*tofree;
	    char_u	*s;

	    msg_puts("(");
	    for (i = 0; i < argcount; ++i)
	    {
		if (i > 0)
		    msg_puts(", ");
		if (argvars[i].v_type == VAR_NUMBER)
		    msg_outnum((long)argvars[i].vval.v_number);
		else
		{
		    // Do not want errors such as E724 here.
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
			msg_puts((char *)s);
			vim_free(tofree);
		    }
		}
	    }
	    msg_puts(")");
	}
	msg_puts("\n");   // don't overwrite this either

	verbose_leave_scroll();
	--no_wait_return;
    }
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	profile_may_start_func(&profile_info, fp,
				 fc->caller == NULL ? NULL : fc->caller->func);
#endif

    save_current_sctx = current_sctx;
    current_sctx = fp->uf_script_ctx;
    save_did_emsg = did_emsg;
    did_emsg = FALSE;

    if (default_arg_err && (fp->uf_flags & FC_ABORT))
	did_emsg = TRUE;
    else if (islambda)
    {
	char_u *p = *(char_u **)fp->uf_lines.ga_data + 7;

	// A Lambda always has the command "return {expr}".  It is much faster
	// to evaluate {expr} directly.
	++ex_nesting_level;
	(void)eval1(&p, rettv, &EVALARG_EVALUATE);
	--ex_nesting_level;
    }
    else
	// call do_cmdline() to execute the lines
	do_cmdline(NULL, get_func_line, (void *)fc,
				     DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT);

    --RedrawingDisabled;

    // when the function was aborted because of an error, return -1
    if ((did_emsg && (fp->uf_flags & FC_ABORT)) || rettv->v_type == VAR_UNKNOWN)
    {
	clear_tv(rettv);
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = -1;
    }

#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
    {
	ufunc_T *caller = fc->caller == NULL ? NULL : fc->caller->func;

	if (fp->uf_profiling || (caller != NULL && caller->uf_profiling))
	    profile_may_end_func(&profile_info, fp, caller);
    }
#endif

    // when being verbose, mention the return value
    if (p_verbose >= 12)
    {
	++no_wait_return;
	verbose_enter_scroll();

	if (aborting())
	    smsg(_("%s aborted"), SOURCING_NAME);
	else if (fc->rettv->v_type == VAR_NUMBER)
	    smsg(_("%s returning #%ld"), SOURCING_NAME,
					       (long)fc->rettv->vval.v_number);
	else
	{
	    char_u	buf[MSG_BUF_LEN];
	    char_u	numbuf2[NUMBUFLEN];
	    char_u	*tofree;
	    char_u	*s;

	    // The value may be very long.  Skip the middle part, so that we
	    // have some idea how it starts and ends. smsg() would always
	    // truncate it at the end. Don't want errors such as E724 here.
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
		smsg(_("%s returning %s"), SOURCING_NAME, s);
		vim_free(tofree);
	    }
	}
	msg_puts("\n");   // don't overwrite this either

	verbose_leave_scroll();
	--no_wait_return;
    }

    ESTACK_CHECK_NOW
    estack_pop();
    current_sctx = save_current_sctx;
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	script_prof_restore(&profile_info.pi_wait_start);
#endif
    if (using_sandbox)
	--sandbox;

    if (p_verbose >= 12 && SOURCING_NAME != NULL)
    {
	++no_wait_return;
	verbose_enter_scroll();

	smsg(_("continuing in %s"), SOURCING_NAME);
	msg_puts("\n");   // don't overwrite this either

	verbose_leave_scroll();
	--no_wait_return;
    }

    did_emsg |= save_did_emsg;
    funcdepth_decrement();
    for (i = 0; i < tv_to_free_len; ++i)
	clear_tv(tv_to_free[i]);
    cleanup_function_call(fc);
}

/*
 * Check the argument count for user function "fp".
 * Return FCERR_UNKNOWN if OK, FCERR_TOOFEW or FCERR_TOOMANY otherwise.
 */
    int
check_user_func_argcount(ufunc_T *fp, int argcount)
{
    int regular_args = fp->uf_args.ga_len;

    if (argcount < regular_args - fp->uf_def_args.ga_len)
	return FCERR_TOOFEW;
    else if (!has_varargs(fp) && argcount > regular_args)
	return FCERR_TOOMANY;
    return FCERR_UNKNOWN;
}

/*
 * Call a user function after checking the arguments.
 */
    int
call_user_func_check(
	ufunc_T	    *fp,
	int	    argcount,
	typval_T    *argvars,
	typval_T    *rettv,
	funcexe_T   *funcexe,
	dict_T	    *selfdict)
{
    int error;

    if (fp->uf_flags & FC_RANGE && funcexe->fe_doesrange != NULL)
	*funcexe->fe_doesrange = TRUE;
    error = check_user_func_argcount(fp, argcount);
    if (error != FCERR_UNKNOWN)
	return error;
    if ((fp->uf_flags & FC_DICT) && selfdict == NULL)
	error = FCERR_DICT;
    else
    {
	int		did_save_redo = FALSE;
	save_redo_T	save_redo;

	/*
	 * Call the user function.
	 * Save and restore search patterns, script variables and
	 * redo buffer.
	 */
	save_search_patterns();
	if (!ins_compl_active())
	{
	    saveRedobuff(&save_redo);
	    did_save_redo = TRUE;
	}
	++fp->uf_calls;
	call_user_func(fp, argcount, argvars, rettv, funcexe,
				   (fp->uf_flags & FC_DICT) ? selfdict : NULL);
	if (--fp->uf_calls <= 0 && fp->uf_refcount <= 0)
	    // Function was unreferenced while being used, free it now.
	    func_clear_free(fp, FALSE);
	if (did_save_redo)
	    restoreRedobuff(&save_redo);
	restore_search_patterns();
	error = FCERR_NONE;
    }
    return error;
}

static funccal_entry_T *funccal_stack = NULL;

/*
 * Save the current function call pointer, and set it to NULL.
 * Used when executing autocommands and for ":source".
 */
    void
save_funccal(funccal_entry_T *entry)
{
    entry->top_funccal = current_funccal;
    entry->next = funccal_stack;
    funccal_stack = entry;
    current_funccal = NULL;
}

    void
restore_funccal(void)
{
    if (funccal_stack == NULL)
	iemsg("INTERNAL: restore_funccal()");
    else
    {
	current_funccal = funccal_stack->top_funccal;
	funccal_stack = funccal_stack->next;
    }
}

    funccall_T *
get_current_funccal(void)
{
    return current_funccal;
}

/*
 * Mark all functions of script "sid" as deleted.
 */
    void
delete_script_functions(int sid)
{
    hashitem_T	*hi;
    ufunc_T	*fp;
    long_u	todo = 1;
    char_u	buf[30];
    size_t	len;

    buf[0] = K_SPECIAL;
    buf[1] = KS_EXTRA;
    buf[2] = (int)KE_SNR;
    sprintf((char *)buf + 3, "%d_", sid);
    len = STRLEN(buf);

    while (todo > 0)
    {
	todo = func_hashtab.ht_used;
	for (hi = func_hashtab.ht_array; todo > 0; ++hi)
	    if (!HASHITEM_EMPTY(hi))
	    {
		fp = HI2UF(hi);
		if (STRNCMP(fp->uf_name, buf, len) == 0)
		{
		    int changed = func_hashtab.ht_changed;

		    fp->uf_flags |= FC_DEAD;

		    if (fp->uf_calls > 0)
		    {
			// Function is executing, don't free it but do remove
			// it from the hashtable.
			if (func_remove(fp))
			    fp->uf_refcount--;
		    }
		    else
		    {
			func_clear(fp, TRUE);
			// When clearing a function another function can be
			// cleared as a side effect.  When that happens start
			// over.
			if (changed != func_hashtab.ht_changed)
			    break;
		    }
		}
		--todo;
	    }
    }
}

#if defined(EXITFREE) || defined(PROTO)
    void
free_all_functions(void)
{
    hashitem_T	*hi;
    ufunc_T	*fp;
    long_u	skipped = 0;
    long_u	todo = 1;
    int		changed;

    // Clean up the current_funccal chain and the funccal stack.
    while (current_funccal != NULL)
    {
	clear_tv(current_funccal->rettv);
	cleanup_function_call(current_funccal);
	if (current_funccal == NULL && funccal_stack != NULL)
	    restore_funccal();
    }

    // First clear what the functions contain.  Since this may lower the
    // reference count of a function, it may also free a function and change
    // the hash table. Restart if that happens.
    while (todo > 0)
    {
	todo = func_hashtab.ht_used;
	for (hi = func_hashtab.ht_array; todo > 0; ++hi)
	    if (!HASHITEM_EMPTY(hi))
	    {
		// clear the def function index now
		fp = HI2UF(hi);
		fp->uf_flags &= ~FC_DEAD;
		fp->uf_def_status = UF_NOT_COMPILED;

		// Only free functions that are not refcounted, those are
		// supposed to be freed when no longer referenced.
		if (func_name_refcount(fp->uf_name))
		    ++skipped;
		else
		{
		    changed = func_hashtab.ht_changed;
		    func_clear(fp, TRUE);
		    if (changed != func_hashtab.ht_changed)
		    {
			skipped = 0;
			break;
		    }
		}
		--todo;
	    }
    }

    // Now actually free the functions.  Need to start all over every time,
    // because func_free() may change the hash table.
    skipped = 0;
    while (func_hashtab.ht_used > skipped)
    {
	todo = func_hashtab.ht_used;
	for (hi = func_hashtab.ht_array; todo > 0; ++hi)
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		// Only free functions that are not refcounted, those are
		// supposed to be freed when no longer referenced.
		fp = HI2UF(hi);
		if (func_name_refcount(fp->uf_name))
		    ++skipped;
		else
		{
		    if (func_free(fp, FALSE) == OK)
		    {
			skipped = 0;
			break;
		    }
		    // did not actually free it
		    ++skipped;
		}
	    }
    }
    if (skipped == 0)
	hash_clear(&func_hashtab);

    free_def_functions();
}
#endif

/*
 * Return TRUE if "name" looks like a builtin function name: starts with a
 * lower case letter and doesn't contain AUTOLOAD_CHAR or ':'.
 * "len" is the length of "name", or -1 for NUL terminated.
 */
    int
builtin_function(char_u *name, int len)
{
    char_u *p;

    if (!ASCII_ISLOWER(name[0]) || name[1] == ':')
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
    list_T	*l = args->vval.v_list;
    listitem_T	*item;
    typval_T	argv[MAX_FUNC_ARGS + 1];
    int		argc = 0;
    int		r = 0;

    CHECK_LIST_MATERIALIZE(l);
    FOR_ALL_LIST_ITEMS(l, item)
    {
	if (argc == MAX_FUNC_ARGS - (partial == NULL ? 0 : partial->pt_argc))
	{
	    emsg(_(e_too_many_arguments));
	    break;
	}
	// Make a copy of each argument.  This is needed to be able to set
	// v_lock to VAR_FIXED in the copy without changing the original list.
	copy_tv(&item->li_tv, &argv[argc++]);
    }

    if (item == NULL)
    {
	funcexe_T funcexe;

	CLEAR_FIELD(funcexe);
	funcexe.fe_firstline = curwin->w_cursor.lnum;
	funcexe.fe_lastline = curwin->w_cursor.lnum;
	funcexe.fe_evaluate = TRUE;
	funcexe.fe_partial = partial;
	funcexe.fe_selfdict = selfdict;
	r = call_func(name, -1, rettv, argc, argv, &funcexe);
    }

    // Free the arguments.
    while (argc > 0)
	clear_tv(&argv[--argc]);

    return r;
}

static int callback_depth = 0;

    int
get_callback_depth(void)
{
    return callback_depth;
}

/*
 * Invoke call_func() with a callback.
 * Returns FAIL if the callback could not be called.
 */
    int
call_callback(
    callback_T	*callback,
    int		len,		// length of "name" or -1 to use strlen()
    typval_T	*rettv,		// return value goes here
    int		argcount,	// number of "argvars"
    typval_T	*argvars)	// vars for arguments, must have "argcount"
				// PLUS ONE elements!
{
    funcexe_T	funcexe;
    int		ret;

    if (callback->cb_name == NULL || *callback->cb_name == NUL)
	return FAIL;
    CLEAR_FIELD(funcexe);
    funcexe.fe_evaluate = TRUE;
    funcexe.fe_partial = callback->cb_partial;
    ++callback_depth;
    ret = call_func(callback->cb_name, len, rettv, argcount, argvars, &funcexe);
    --callback_depth;

    // When a :def function was called that uses :try an error would be turned
    // into an exception.  Need to give the error here.
    if (need_rethrow && current_exception != NULL && trylevel == 0)
    {
	need_rethrow = FALSE;
	handle_did_throw();
    }

    return ret;
}

/*
 * call the 'callback' function and return the result as a number.
 * Returns -2 when calling the function fails.  Uses argv[0] to argv[argc - 1]
 * for the function arguments. argv[argc] should have type VAR_UNKNOWN.
 */
    varnumber_T
call_callback_retnr(
    callback_T	*callback,
    int		argcount,	// number of "argvars"
    typval_T	*argvars)	// vars for arguments, must have "argcount"
				// PLUS ONE elements!
{
    typval_T	rettv;
    varnumber_T	retval;

    if (call_callback(callback, 0, &rettv, argcount, argvars) == FAIL)
	return -2;

    retval = tv_get_number_chk(&rettv, NULL);
    clear_tv(&rettv);
    return retval;
}

/*
 * Give an error message for the result of a function.
 * Nothing if "error" is FCERR_NONE.
 */
    void
user_func_error(int error, char_u *name, funcexe_T *funcexe)
{
    switch (error)
    {
	case FCERR_UNKNOWN:
		if (funcexe->fe_found_var)
		    semsg(_(e_not_callable_type_str), name);
		else
		    emsg_funcname(e_unknown_function_str, name);
		break;
	case FCERR_NOTMETHOD:
		emsg_funcname(
			N_(e_cannot_use_function_as_method_str), name);
		break;
	case FCERR_DELETED:
		emsg_funcname(e_function_was_deleted_str, name);
		break;
	case FCERR_TOOMANY:
		emsg_funcname(e_too_many_arguments_for_function_str, name);
		break;
	case FCERR_TOOFEW:
		emsg_funcname(e_not_enough_arguments_for_function_str, name);
		break;
	case FCERR_SCRIPT:
		emsg_funcname(
		    e_using_sid_not_in_script_context_str, name);
		break;
	case FCERR_DICT:
		emsg_funcname(e_calling_dict_function_without_dictionary_str,
									 name);
		break;
    }
}

/*
 * Call a function with its resolved parameters
 *
 * Return FAIL when the function can't be called,  OK otherwise.
 * Also returns OK when an error was encountered while executing the function.
 */
    int
call_func(
    char_u	*funcname,	// name of the function
    int		len,		// length of "name" or -1 to use strlen()
    typval_T	*rettv,		// return value goes here
    int		argcount_in,	// number of "argvars"
    typval_T	*argvars_in,	// vars for arguments, must have "argcount"
				// PLUS ONE elements!
    funcexe_T	*funcexe)	// more arguments
{
    int		ret = FAIL;
    int		error = FCERR_NONE;
    int		i;
    ufunc_T	*fp = NULL;
    char_u	fname_buf[FLEN_FIXED + 1];
    char_u	*tofree = NULL;
    char_u	*fname = NULL;
    char_u	*name = NULL;
    int		argcount = argcount_in;
    typval_T	*argvars = argvars_in;
    dict_T	*selfdict = funcexe->fe_selfdict;
    typval_T	argv[MAX_FUNC_ARGS + 1]; // used when "partial" or
					 // "funcexe->fe_basetv" is not NULL
    int		argv_clear = 0;
    int		argv_base = 0;
    partial_T	*partial = funcexe->fe_partial;
    type_T	check_type;
    type_T	*check_type_args[MAX_FUNC_ARGS];

    // Initialize rettv so that it is safe for caller to invoke clear_tv(rettv)
    // even when call_func() returns FAIL.
    rettv->v_type = VAR_UNKNOWN;

    if (partial != NULL)
	fp = partial->pt_func;
    if (fp == NULL)
    {
	// Make a copy of the name, if it comes from a funcref variable it
	// could be changed or deleted in the called function.
	name = len > 0 ? vim_strnsave(funcname, len) : vim_strsave(funcname);
	if (name == NULL)
	    return ret;

	fname = fname_trans_sid(name, fname_buf, &tofree, &error);
    }

    if (funcexe->fe_doesrange != NULL)
	*funcexe->fe_doesrange = FALSE;

    if (partial != NULL)
    {
	// When the function has a partial with a dict and there is a dict
	// argument, use the dict argument.  That is backwards compatible.
	// When the dict was bound explicitly use the one from the partial.
	if (partial->pt_dict != NULL && (selfdict == NULL || !partial->pt_auto))
	    selfdict = partial->pt_dict;
	if (error == FCERR_NONE && partial->pt_argc > 0)
	{
	    for (argv_clear = 0; argv_clear < partial->pt_argc; ++argv_clear)
	    {
		if (argv_clear + argcount_in >= MAX_FUNC_ARGS)
		{
		    error = FCERR_TOOMANY;
		    goto theend;
		}
		copy_tv(&partial->pt_argv[argv_clear], &argv[argv_clear]);
	    }
	    for (i = 0; i < argcount_in; ++i)
		argv[i + argv_clear] = argvars_in[i];
	    argvars = argv;
	    argcount = partial->pt_argc + argcount_in;

	    if (funcexe->fe_check_type != NULL
				  && funcexe->fe_check_type->tt_argcount != -1)
	    {
		// Now funcexe->fe_check_type is missing the added arguments,
		// make a copy of the type with the correction.
		check_type = *funcexe->fe_check_type;
		funcexe->fe_check_type = &check_type;
		check_type.tt_args = check_type_args;
		CLEAR_FIELD(check_type_args);
		for (i = 0; i < check_type.tt_argcount; ++i)
		    check_type_args[i + partial->pt_argc] =
							 check_type.tt_args[i];
		check_type.tt_argcount += partial->pt_argc;
		check_type.tt_min_argcount += partial->pt_argc;
	    }
	}
    }

    if (error == FCERR_NONE && funcexe->fe_check_type != NULL
						       && funcexe->fe_evaluate)
    {
	// Check that the argument types are OK for the types of the funcref.
	if (check_argument_types(funcexe->fe_check_type, argvars, argcount,
				     (name != NULL) ? name : funcname) == FAIL)
	    error = FCERR_OTHER;
    }

    if (error == FCERR_NONE && funcexe->fe_evaluate)
    {
	char_u *rfname = fname;
	int	is_global = FALSE;

	// Skip "g:" before a function name.
	if (fp == NULL && fname[0] == 'g' && fname[1] == ':')
	{
	    is_global = TRUE;
	    rfname = fname + 2;
	}

	rettv->v_type = VAR_NUMBER;	// default rettv is number zero
	rettv->vval.v_number = 0;
	error = FCERR_UNKNOWN;

	if (fp != NULL || !builtin_function(rfname, -1))
	{
	    /*
	     * User defined function.
	     */
	    if (fp == NULL)
		fp = find_func(rfname, is_global, NULL);

	    // Trigger FuncUndefined event, may load the function.
	    if (fp == NULL
		    && apply_autocmds(EVENT_FUNCUNDEFINED,
						    rfname, rfname, TRUE, NULL)
		    && !aborting())
	    {
		// executed an autocommand, search for the function again
		fp = find_func(rfname, is_global, NULL);
	    }
	    // Try loading a package.
	    if (fp == NULL && script_autoload(rfname, TRUE) && !aborting())
	    {
		// loaded a package, search for the function again
		fp = find_func(rfname, is_global, NULL);
	    }
	    if (fp == NULL)
	    {
		char_u *p = untrans_function_name(rfname);

		// If using Vim9 script try not local to the script.
		// Don't do this if the name starts with "s:".
		if (p != NULL && (funcname[0] != 's' || funcname[1] != ':'))
		    fp = find_func(p, is_global, NULL);
	    }

	    if (fp != NULL && (fp->uf_flags & FC_DELETED))
		error = FCERR_DELETED;
#ifdef FEAT_LUA
	    else if (fp != NULL && (fp->uf_flags & FC_CFUNC))
	    {
		cfunc_T cb = fp->uf_cb;

		error = (*cb)(argcount, argvars, rettv, fp->uf_cb_state);
	    }
#endif
	    else if (fp != NULL)
	    {
		if (funcexe->fe_argv_func != NULL)
		    // postponed filling in the arguments, do it now
		    argcount = funcexe->fe_argv_func(argcount, argvars,
					       argv_clear, fp->uf_args.ga_len);

		if (funcexe->fe_basetv != NULL)
		{
		    // Method call: base->Method()
		    mch_memmove(&argv[1], argvars, sizeof(typval_T) * argcount);
		    argv[0] = *funcexe->fe_basetv;
		    argcount++;
		    argvars = argv;
		    argv_base = 1;
		}

		error = call_user_func_check(fp, argcount, argvars, rettv,
							    funcexe, selfdict);
	    }
	}
	else if (funcexe->fe_basetv != NULL)
	{
	    /*
	     * expr->method(): Find the method name in the table, call its
	     * implementation with the base as one of the arguments.
	     */
	    error = call_internal_method(fname, argcount, argvars, rettv,
							   funcexe->fe_basetv);
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
    if (error == FCERR_NONE)
	ret = OK;

theend:
    /*
     * Report an error unless the argument evaluation or function call has been
     * cancelled due to an aborting error, an interrupt, or an exception.
     */
    if (!aborting())
    {
	user_func_error(error, (name != NULL) ? name : funcname, funcexe);
    }

    // clear the copies made from the partial
    while (argv_clear > 0)
	clear_tv(&argv[--argv_clear + argv_base]);

    vim_free(tofree);
    vim_free(name);

    return ret;
}

    char_u *
printable_func_name(ufunc_T *fp)
{
    return fp->uf_name_exp != NULL ? fp->uf_name_exp : fp->uf_name;
}

/*
 * List the head of the function: "function name(arg1, arg2)".
 */
    static void
list_func_head(ufunc_T *fp, int indent)
{
    int		j;

    msg_start();
    if (indent)
	msg_puts("   ");
    if (fp->uf_def_status != UF_NOT_COMPILED)
	msg_puts("def ");
    else
	msg_puts("function ");
    msg_puts((char *)printable_func_name(fp));
    msg_putchar('(');
    for (j = 0; j < fp->uf_args.ga_len; ++j)
    {
	if (j)
	    msg_puts(", ");
	msg_puts((char *)FUNCARG(fp, j));
	if (fp->uf_arg_types != NULL)
	{
	    char *tofree;

	    msg_puts(": ");
	    msg_puts(type_name(fp->uf_arg_types[j], &tofree));
	    vim_free(tofree);
	}
	if (j >= fp->uf_args.ga_len - fp->uf_def_args.ga_len)
	{
	    msg_puts(" = ");
	    msg_puts(((char **)(fp->uf_def_args.ga_data))
		       [j - fp->uf_args.ga_len + fp->uf_def_args.ga_len]);
	}
    }
    if (fp->uf_varargs)
    {
	if (j)
	    msg_puts(", ");
	msg_puts("...");
    }
    if (fp->uf_va_name != NULL)
    {
	if (j)
	    msg_puts(", ");
	msg_puts("...");
	msg_puts((char *)fp->uf_va_name);
	if (fp->uf_va_type != NULL)
	{
	    char *tofree;

	    msg_puts(": ");
	    msg_puts(type_name(fp->uf_va_type, &tofree));
	    vim_free(tofree);
	}
    }
    msg_putchar(')');

    if (fp->uf_def_status != UF_NOT_COMPILED)
    {
	if (fp->uf_ret_type != &t_void)
	{
	    char *tofree;

	    msg_puts(": ");
	    msg_puts(type_name(fp->uf_ret_type, &tofree));
	    vim_free(tofree);
	}
    }
    else if (fp->uf_flags & FC_ABORT)
	msg_puts(" abort");
    if (fp->uf_flags & FC_RANGE)
	msg_puts(" range");
    if (fp->uf_flags & FC_DICT)
	msg_puts(" dict");
    if (fp->uf_flags & FC_CLOSURE)
	msg_puts(" closure");
    msg_clr_eos();
    if (p_verbose > 0)
	last_set_msg(fp->uf_script_ctx);
}

/*
 * Get a function name, translating "<SID>" and "<SNR>".
 * Also handles a Funcref in a List or Dictionary.
 * Returns the function name in allocated memory, or NULL for failure.
 * Set "*is_global" to TRUE when the function must be global, unless
 * "is_global" is NULL.
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
    int		*is_global,
    int		skip,		// only find the end, don't evaluate
    int		flags,
    funcdict_T	*fdp,		// return: info about dictionary used
    partial_T	**partial,	// return: partial of a FuncRef
    type_T	**type)		// return: type of funcref if not NULL
{
    char_u	*name = NULL;
    char_u	*start;
    char_u	*end;
    int		lead;
    char_u	sid_buf[20];
    int		len;
    int		extra = 0;
    lval_T	lv;
    int		vim9script;

    if (fdp != NULL)
	CLEAR_POINTER(fdp);
    start = *pp;

    // Check for hard coded <SNR>: already translated function ID (from a user
    // command).
    if ((*pp)[0] == K_SPECIAL && (*pp)[1] == KS_EXTRA
						   && (*pp)[2] == (int)KE_SNR)
    {
	*pp += 3;
	len = get_id_len(pp) + 3;
	return vim_strnsave(start, len);
    }

    // A name starting with "<SID>" or "<SNR>" is local to a script.  But
    // don't skip over "s:", get_lval() needs it for "s:dict.func".
    lead = eval_fname_script(start);
    if (lead > 2)
	start += lead;

    // Note that TFN_ flags use the same values as GLV_ flags.
    end = get_lval(start, NULL, &lv, FALSE, skip, flags | GLV_READ_ONLY,
					      lead > 2 ? 0 : FNE_CHECK_START);
    if (end == start)
    {
	if (!skip)
	    emsg(_(e_function_name_required));
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
		semsg(_(e_invalid_argument_str), start);
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
		emsg(_(e_funcref_required));
	    else
		*pp = end;
	    name = NULL;
	}
	goto theend;
    }

    if (lv.ll_name == NULL)
    {
	// Error found, but continue after the function name.
	*pp = end;
	goto theend;
    }

    // Check if the name is a Funcref.  If so, use the value.
    if (lv.ll_exp_name != NULL)
    {
	len = (int)STRLEN(lv.ll_exp_name);
	name = deref_func_name(lv.ll_exp_name, &len, partial, type,
						flags & TFN_NO_AUTOLOAD, NULL);
	if (name == lv.ll_exp_name)
	    name = NULL;
    }
    else if (!(flags & TFN_NO_DEREF))
    {
	len = (int)(end - *pp);
	name = deref_func_name(*pp, &len, partial, type,
						flags & TFN_NO_AUTOLOAD, NULL);
	if (name == *pp)
	    name = NULL;
    }
    if (name != NULL)
    {
	name = vim_strsave(name);
	*pp = end;
	if (STRNCMP(name, "<SNR>", 5) == 0)
	{
	    // Change "<SNR>" to the byte sequence.
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
	    // When there was "s:" already or the name expanded to get a
	    // leading "s:" then remove it.
	    lv.ll_name += 2;
	    len -= 2;
	    lead = 2;
	}
    }
    else
    {
	// skip over "s:" and "g:"
	if (lead == 2 || (lv.ll_name[0] == 'g' && lv.ll_name[1] == ':'))
	{
	    if (is_global != NULL && lv.ll_name[0] == 'g')
		*is_global = TRUE;
	    lv.ll_name += 2;
	}
	len = (int)(end - lv.ll_name);
    }
    if (len <= 0)
    {
	if (!skip)
	    emsg(_(e_function_name_required));
	goto theend;
    }

    // In Vim9 script a user function is script-local by default, unless it
    // starts with a lower case character: dict.func().
    vim9script = ASCII_ISUPPER(*start) && in_vim9script();
    if (vim9script)
    {
	char_u *p;

	// SomeScript#func() is a global function.
	for (p = start; *p != NUL && *p != '('; ++p)
	    if (*p == AUTOLOAD_CHAR)
		vim9script = FALSE;
    }

    /*
     * Copy the function name to allocated memory.
     * Accept <SID>name() inside a script, translate into <SNR>123_name().
     * Accept <SNR>123_name() outside a script.
     */
    if (skip)
	lead = 0;	// do nothing
    else if (lead > 0 || vim9script)
    {
	if (!vim9script)
	    lead = 3;
	if (vim9script || (lv.ll_exp_name != NULL
					     && eval_fname_sid(lv.ll_exp_name))
						       || eval_fname_sid(*pp))
	{
	    // It's script-local, "s:" or "<SID>"
	    if (current_sctx.sc_sid <= 0)
	    {
		emsg(_(e_using_sid_not_in_script_context));
		goto theend;
	    }
	    sprintf((char *)sid_buf, "%ld_", (long)current_sctx.sc_sid);
	    if (vim9script)
		extra = 3 + (int)STRLEN(sid_buf);
	    else
		lead += (int)STRLEN(sid_buf);
	}
    }
    else if (!(flags & TFN_INT) && (builtin_function(lv.ll_name, len)
				   || (in_vim9script() && *lv.ll_name == '_')))
    {
	semsg(_(e_function_name_must_start_with_capital_or_s_str), start);
	goto theend;
    }
    if (!skip && !(flags & TFN_QUIET) && !(flags & TFN_NO_DEREF))
    {
	char_u *cp = vim_strchr(lv.ll_name, ':');

	if (cp != NULL && cp < end)
	{
	    semsg(_(e_function_name_cannot_contain_colon_str), start);
	    goto theend;
	}
    }

    name = alloc(len + lead + extra + 1);
    if (name != NULL)
    {
	if (!skip && (lead > 0 || vim9script))
	{
	    name[0] = K_SPECIAL;
	    name[1] = KS_EXTRA;
	    name[2] = (int)KE_SNR;
	    if (vim9script || lead > 3)	// If it's "<SID>"
		STRCPY(name + 3, sid_buf);
	}
	mch_memmove(name + lead + extra, lv.ll_name, (size_t)len);
	name[lead + extra + len] = NUL;
    }
    *pp = end;

theend:
    clear_lval(&lv);
    return name;
}

/*
 * Assuming "name" is the result of trans_function_name() and it was prefixed
 * to use the script-local name, return the unmodified name (points into
 * "name").  Otherwise return NULL.
 * This can be used to first search for a script-local function and fall back
 * to the global function if not found.
 */
    char_u *
untrans_function_name(char_u *name)
{
    char_u *p;

    if (*name == K_SPECIAL && in_vim9script())
    {
	p = vim_strchr(name, '_');
	if (p != NULL)
	    return p + 1;
    }
    return NULL;
}

/*
 * If the 'funcname' starts with "s:" or "<SID>", then expands it to the
 * current script ID and returns the expanded function name. The caller should
 * free the returned name. If not called from a script context or the function
 * name doesn't start with these prefixes, then returns NULL.
 * This doesn't check whether the script-local function exists or not.
 */
    char_u *
get_scriptlocal_funcname(char_u *funcname)
{
    char	sid_buf[25];
    int		off;
    char_u	*newname;

    if (funcname == NULL)
	return NULL;

    if (STRNCMP(funcname, "s:", 2) != 0
		&& STRNCMP(funcname, "<SID>", 5) != 0)
	// The function name is not a script-local function name
	return NULL;

    if (!SCRIPT_ID_VALID(current_sctx.sc_sid))
    {
	emsg(_(e_using_sid_not_in_script_context));
	return NULL;
    }
    // Expand s: prefix into <SNR>nr_<name>
    vim_snprintf(sid_buf, sizeof(sid_buf), "<SNR>%ld_",
	    (long)current_sctx.sc_sid);
    off = *funcname == 's' ? 2 : 5;
    newname = alloc(STRLEN(sid_buf) + STRLEN(funcname + off) + 1);
    if (newname == NULL)
	return NULL;
    STRCPY(newname, sid_buf);
    STRCAT(newname, funcname + off);

    return newname;
}

/*
 * Call trans_function_name(), except that a lambda is returned as-is.
 * Returns the name in allocated memory.
 */
    char_u *
save_function_name(
	char_u	    **name,
	int	    *is_global,
	int	    skip,
	int	    flags,
	funcdict_T  *fudi)
{
    char_u *p = *name;
    char_u *saved;

    if (STRNCMP(p, "<lambda>", 8) == 0)
    {
	p += 8;
	(void)getdigits(&p);
	saved = vim_strnsave(*name, p - *name);
	if (fudi != NULL)
	    CLEAR_POINTER(fudi);
    }
    else
	saved = trans_function_name(&p, is_global, skip,
						      flags, fudi, NULL, NULL);
    *name = p;
    return saved;
}

/*
 * List functions.  When "regmatch" is NULL all of then.
 * Otherwise functions matching "regmatch".
 */
    void
list_functions(regmatch_T *regmatch)
{
    int		changed = func_hashtab.ht_changed;
    long_u	todo = func_hashtab.ht_used;
    hashitem_T	*hi;

    for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    ufunc_T	*fp = HI2UF(hi);

	    --todo;
	    if ((fp->uf_flags & FC_DEAD) == 0
		    && (regmatch == NULL
			? !message_filtered(fp->uf_name)
			    && !func_name_refcount(fp->uf_name)
			: !isdigit(*fp->uf_name)
			    && vim_regexec(regmatch, fp->uf_name, 0)))
	    {
		list_func_head(fp, FALSE);
		if (changed != func_hashtab.ht_changed)
		{
		    emsg(_(e_function_list_was_modified));
		    return;
		}
	    }
	}
    }
}

/*
 * ":function" also supporting nested ":def".
 * When "name_arg" is not NULL this is a nested function, using "name_arg" for
 * the function name.
 * Returns a pointer to the function or NULL if no function defined.
 */
    ufunc_T *
define_function(exarg_T *eap, char_u *name_arg, char_u **line_to_free)
{
    int		j;
    int		c;
    int		saved_did_emsg;
    char_u	*name = name_arg;
    int		is_global = FALSE;
    char_u	*p;
    char_u	*arg;
    char_u	*whitep;
    char_u	*line_arg = NULL;
    garray_T	newargs;
    garray_T	argtypes;
    garray_T	default_args;
    garray_T	newlines;
    int		varargs = FALSE;
    int		flags = 0;
    char_u	*ret_type = NULL;
    ufunc_T	*fp = NULL;
    int		fp_allocated = FALSE;
    int		free_fp = FALSE;
    int		overwrite = FALSE;
    dictitem_T	*v;
    funcdict_T	fudi;
    static int	func_nr = 0;	    // number for nameless function
    int		paren;
    hashitem_T	*hi;
    linenr_T	sourcing_lnum_top;
    int		vim9script = in_vim9script();
    imported_T	*import = NULL;

    /*
     * ":function" without argument: list functions.
     */
    if (ends_excmd2(eap->cmd, eap->arg))
    {
	if (!eap->skip)
	    list_functions(NULL);
	set_nextcmd(eap, eap->arg);
	return NULL;
    }

    /*
     * ":function /pat": list functions matching pattern.
     */
    if (*eap->arg == '/')
    {
	p = skip_regexp(eap->arg + 1, '/', TRUE);
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
		list_functions(&regmatch);
		vim_regfree(regmatch.regprog);
	    }
	}
	if (*p == '/')
	    ++p;
	set_nextcmd(eap, p);
	return NULL;
    }

    ga_init(&newargs);
    ga_init(&argtypes);
    ga_init(&default_args);

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
    if (name_arg != NULL)
    {
	// nested function, argument is (args).
	paren = TRUE;
	CLEAR_FIELD(fudi);
    }
    else
    {
	name = save_function_name(&p, &is_global, eap->skip,
						       TFN_NO_AUTOLOAD, &fudi);
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
		    semsg(_(e_key_not_present_in_dictionary), fudi.fd_newkey);
		vim_free(fudi.fd_newkey);
		return NULL;
	    }
	    else
		eap->skip = TRUE;
	}
    }

    // An error in a function call during evaluation of an expression in magic
    // braces should not cause the function not to be defined.
    saved_did_emsg = did_emsg;
    did_emsg = FALSE;

    /*
     * ":function func" with only function name: list function.
     */
    if (!paren)
    {
	if (!ends_excmd(*skipwhite(p)))
	{
	    semsg(_(e_trailing_characters_str), p);
	    goto ret_free;
	}
	set_nextcmd(eap, p);
	if (eap->nextcmd != NULL)
	    *p = NUL;
	if (!eap->skip && !got_int)
	{
	    fp = find_func(name, is_global, NULL);
	    if (fp == NULL && ASCII_ISUPPER(*eap->arg))
	    {
		char_u *up = untrans_function_name(name);

		// With Vim9 script the name was made script-local, if not
		// found try again with the original name.
		if (up != NULL)
		    fp = find_func(up, FALSE, NULL);
	    }

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
		    out_flush();	// show a line at a time
		    ui_breakcheck();
		}
		if (!got_int)
		{
		    msg_putchar('\n');
		    if (fp->uf_def_status != UF_NOT_COMPILED)
			msg_puts("   enddef");
		    else
			msg_puts("   endfunction");
		}
	    }
	    else
		emsg_funcname(e_undefined_function_str, eap->arg);
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
	    semsg(_(e_missing_paren_str), eap->arg);
	    goto ret_free;
	}
	// attempt to continue by skipping some text
	if (vim_strchr(p, '(') != NULL)
	    p = vim_strchr(p, '(');
    }

    if ((vim9script || eap->cmdidx == CMD_def) && VIM_ISWHITE(p[-1]))
    {
	semsg(_(e_no_white_space_allowed_before_str_str), "(", p - 1);
	goto ret_free;
    }

    // In Vim9 script only global functions can be redefined.
    if (vim9script && eap->forceit && !is_global)
    {
	emsg(_(e_no_bang_allowed));
	goto ret_free;
    }

    ga_init2(&newlines, (int)sizeof(char_u *), 10);

    if (!eap->skip && name_arg == NULL)
    {
	// Check the name of the function.  Unless it's a dictionary function
	// (that we are overwriting).
	if (name != NULL)
	    arg = name;
	else
	    arg = fudi.fd_newkey;
	if (arg != NULL && (fudi.fd_di == NULL
				     || (fudi.fd_di->di_tv.v_type != VAR_FUNC
				 && fudi.fd_di->di_tv.v_type != VAR_PARTIAL)))
	{
	    char_u  *name_base = arg;
	    int	    i;

	    if (*arg == K_SPECIAL)
	    {
		name_base = vim_strchr(arg, '_');
		if (name_base == NULL)
		    name_base = arg + 3;
		else
		    ++name_base;
	    }
	    for (i = 0; name_base[i] != NUL && (i == 0
					? eval_isnamec1(name_base[i])
					: eval_isnamec(name_base[i])); ++i)
		;
	    if (name_base[i] != NUL)
		emsg_funcname(e_invalid_argument_str, arg);

	    // In Vim9 script a function cannot have the same name as a
	    // variable.
	    if (vim9script && *arg == K_SPECIAL
		&& eval_variable(name_base, (int)STRLEN(name_base), 0, NULL,
		    NULL, EVAL_VAR_NOAUTOLOAD + EVAL_VAR_IMPORT
						     + EVAL_VAR_NO_FUNC) == OK)
	    {
		semsg(_(e_redefining_script_item_str), name_base);
		goto ret_free;
	    }
	}
	// Disallow using the g: dict.
	if (fudi.fd_dict != NULL && fudi.fd_dict->dv_scope == VAR_DEF_SCOPE)
	{
	    emsg(_(e_cannot_use_g_here));
	    goto ret_free;
	}
    }

    // This may get more lines and make the pointers into the first line
    // invalid.
    ++p;
    if (get_function_args(&p, ')', &newargs,
			eap->cmdidx == CMD_def ? &argtypes : NULL, FALSE,
			 NULL, &varargs, &default_args, eap->skip,
			 eap, line_to_free) == FAIL)
	goto errret_2;
    whitep = p;

    if (eap->cmdidx == CMD_def)
    {
	// find the return type: :def Func(): type
	if (*skipwhite(p) == ':')
	{
	    if (*p != ':')
	    {
		semsg(_(e_no_white_space_allowed_before_colon_str), p);
		p = skipwhite(p);
	    }
	    else if (!IS_WHITE_OR_NUL(p[1]))
		semsg(_(e_white_space_required_after_str_str), ":", p);
	    ret_type = skipwhite(p + 1);
	    p = skip_type(ret_type, FALSE);
	    if (p > ret_type)
	    {
		ret_type = vim_strnsave(ret_type, p - ret_type);
		whitep = p;
		p = skipwhite(p);
	    }
	    else
	    {
		semsg(_(e_expected_type_str), ret_type);
		ret_type = NULL;
	    }
	}
	p = skipwhite(p);
    }
    else
	// find extra arguments "range", "dict", "abort" and "closure"
	for (;;)
	{
	    whitep = p;
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
		    emsg_funcname(e_closure_function_should_not_be_at_top_level,
			    name == NULL ? (char_u *)"" : name);
		    goto erret;
		}
	    }
	    else
		break;
	}

    // When there is a line break use what follows for the function body.
    // Makes 'exe "func Test()\n...\nendfunc"' work.
    if (*p == '\n')
	line_arg = p + 1;
    else if (*p != NUL
	    && !(*p == '"' && (!vim9script || eap->cmdidx == CMD_function)
						     && eap->cmdidx != CMD_def)
	    && !(VIM_ISWHITE(*whitep) && *p == '#'
				     && (vim9script || eap->cmdidx == CMD_def))
	    && !eap->skip
	    && !did_emsg)
	semsg(_(e_trailing_characters_str), p);

    /*
     * Read the body of the function, until "}", ":endfunction" or ":enddef" is
     * found.
     */
    if (KeyTyped)
    {
	// Check if the function already exists, don't let the user type the
	// whole function before telling him it doesn't work!  For a script we
	// need to skip the body to be able to find what follows.
	if (!eap->skip && !eap->forceit)
	{
	    if (fudi.fd_dict != NULL && fudi.fd_newkey == NULL)
		emsg(_(e_dictionary_entry_already_exists));
	    else if (name != NULL && find_func(name, is_global, NULL) != NULL)
		emsg_funcname(e_function_str_already_exists_add_bang_to_replace, name);
	}

	if (!eap->skip && did_emsg)
	    goto erret;

	msg_putchar('\n');	    // don't overwrite the function name
	cmdline_row = msg_row;
    }

    // Save the starting line number.
    sourcing_lnum_top = SOURCING_LNUM;

    // Do not define the function when getting the body fails and when
    // skipping.
    if (get_function_body(eap, &newlines, line_arg, line_to_free) == FAIL
	    || eap->skip)
	goto erret;

    /*
     * If there are no errors, add the function
     */
    if (fudi.fd_dict == NULL)
    {
	hashtab_T	*ht;

	v = find_var(name, &ht, TRUE);
	if (v != NULL && v->di_tv.v_type == VAR_FUNC)
	{
	    emsg_funcname(e_function_name_conflicts_with_variable_str, name);
	    goto erret;
	}

	fp = find_func_even_dead(name, is_global, NULL);
	if (vim9script)
	{
	    char_u *uname = untrans_function_name(name);

	    import = find_imported(uname == NULL ? name : uname, 0, NULL);
	}

	if (fp != NULL || import != NULL)
	{
	    int dead = fp != NULL && (fp->uf_flags & FC_DEAD);

	    // Function can be replaced with "function!" and when sourcing the
	    // same script again, but only once.
	    // A name that is used by an import can not be overruled.
	    if (import != NULL
		    || (!dead && !eap->forceit
			&& (fp->uf_script_ctx.sc_sid != current_sctx.sc_sid
			  || fp->uf_script_ctx.sc_seq == current_sctx.sc_seq)))
	    {
		SOURCING_LNUM = sourcing_lnum_top;
		if (vim9script)
		    emsg_funcname(e_name_already_defined_str, name);
		else
		    emsg_funcname(e_function_str_already_exists_add_bang_to_replace, name);
		goto erret;
	    }
	    if (fp->uf_calls > 0)
	    {
		emsg_funcname(
			    e_cannot_redefine_function_str_it_is_in_use, name);
		goto erret;
	    }
	    if (fp->uf_refcount > 1)
	    {
		// This function is referenced somewhere, don't redefine it but
		// create a new one.
		--fp->uf_refcount;
		fp->uf_flags |= FC_REMOVED;
		fp = NULL;
		overwrite = TRUE;
	    }
	    else
	    {
		char_u *exp_name = fp->uf_name_exp;

		// redefine existing function, keep the expanded name
		VIM_CLEAR(name);
		fp->uf_name_exp = NULL;
		func_clear_items(fp);
		fp->uf_name_exp = exp_name;
		fp->uf_flags &= ~FC_DEAD;
#ifdef FEAT_PROFILE
		fp->uf_profiling = FALSE;
		fp->uf_prof_initialized = FALSE;
#endif
		fp->uf_def_status = UF_NOT_COMPILED;
	    }
	}
    }
    else
    {
	char	numbuf[20];

	fp = NULL;
	if (fudi.fd_newkey == NULL && !eap->forceit)
	{
	    emsg(_(e_dictionary_entry_already_exists));
	    goto erret;
	}
	if (fudi.fd_di == NULL)
	{
	    // Can't add a function to a locked dictionary
	    if (value_check_lock(fudi.fd_dict->dv_lock, eap->arg, FALSE))
		goto erret;
	}
	    // Can't change an existing function if it is locked
	else if (value_check_lock(fudi.fd_di->di_tv.v_lock, eap->arg, FALSE))
	    goto erret;

	// Give the function a sequential number.  Can only be used with a
	// Funcref!
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

	    // Check that the autoload name matches the script name.
	    j = FAIL;
	    if (SOURCING_NAME != NULL)
	    {
		scriptname = autoload_name(name);
		if (scriptname != NULL)
		{
		    p = vim_strchr(scriptname, '/');
		    plen = (int)STRLEN(p);
		    slen = (int)STRLEN(SOURCING_NAME);
		    if (slen > plen && fnamecmp(p,
					    SOURCING_NAME + slen - plen) == 0)
			j = OK;
		    vim_free(scriptname);
		}
	    }
	    if (j == FAIL)
	    {
		linenr_T save_lnum = SOURCING_LNUM;

		SOURCING_LNUM = sourcing_lnum_top;
		semsg(_(e_function_name_does_not_match_script_file_name_str),
									 name);
		SOURCING_LNUM = save_lnum;
		goto erret;
	    }
	}

	fp = alloc_clear(offsetof(ufunc_T, uf_name) + STRLEN(name) + 1);
	if (fp == NULL)
	    goto erret;
	fp_allocated = TRUE;

	if (fudi.fd_dict != NULL)
	{
	    if (fudi.fd_di == NULL)
	    {
		// add new dict entry
		fudi.fd_di = dictitem_alloc(fudi.fd_newkey);
		if (fudi.fd_di == NULL)
		{
		    vim_free(fp);
		    fp = NULL;
		    goto erret;
		}
		if (dict_add(fudi.fd_dict, fudi.fd_di) == FAIL)
		{
		    vim_free(fudi.fd_di);
		    vim_free(fp);
		    fp = NULL;
		    goto erret;
		}
	    }
	    else
		// overwrite existing dict entry
		clear_tv(&fudi.fd_di->di_tv);
	    fudi.fd_di->di_tv.v_type = VAR_FUNC;
	    fudi.fd_di->di_tv.vval.v_string = vim_strsave(name);

	    // behave like "dict" was used
	    flags |= FC_DICT;
	}
    }
    fp->uf_args = newargs;
    fp->uf_def_args = default_args;
    fp->uf_ret_type = &t_any;
    fp->uf_func_type = &t_func_any;

    if (eap->cmdidx == CMD_def)
    {
	int	    lnum_save = SOURCING_LNUM;
	cstack_T    *cstack = eap->cstack;

	fp->uf_def_status = UF_TO_BE_COMPILED;

	// error messages are for the first function line
	SOURCING_LNUM = sourcing_lnum_top;

	// The function may use script variables from the context.
	function_using_block_scopes(fp, cstack);

	if (parse_argument_types(fp, &argtypes, varargs) == FAIL)
	{
	    SOURCING_LNUM = lnum_save;
	    free_fp = fp_allocated;
	    goto erret;
	}
	varargs = FALSE;

	// parse the return type, if any
	if (parse_return_type(fp, ret_type) == FAIL)
	{
	    SOURCING_LNUM = lnum_save;
	    free_fp = fp_allocated;
	    goto erret;
	}
	SOURCING_LNUM = lnum_save;
    }
    else
	fp->uf_def_status = UF_NOT_COMPILED;

    if (fp_allocated)
    {
	// insert the new function in the function list
	set_ufunc_name(fp, name);
	if (overwrite)
	{
	    hi = hash_find(&func_hashtab, name);
	    hi->hi_key = UF2HIKEY(fp);
	}
	else if (hash_add(&func_hashtab, UF2HIKEY(fp)) == FAIL)
	{
	    free_fp = TRUE;
	    goto erret;
	}
	fp->uf_refcount = 1;
    }

    fp->uf_lines = newlines;
    newlines.ga_data = NULL;
    if ((flags & FC_CLOSURE) != 0)
    {
	if (register_closure(fp) == FAIL)
	    goto erret;
    }
    else
	fp->uf_scoped = NULL;

#ifdef FEAT_PROFILE
    if (prof_def_func())
	func_do_profile(fp);
#endif
    fp->uf_varargs = varargs;
    if (sandbox)
	flags |= FC_SANDBOX;
    if (vim9script && !ASCII_ISUPPER(*fp->uf_name))
	flags |= FC_VIM9;
    fp->uf_flags = flags;
    fp->uf_calls = 0;
    fp->uf_cleared = FALSE;
    fp->uf_script_ctx = current_sctx;
    fp->uf_script_ctx_version = current_sctx.sc_version;
    fp->uf_script_ctx.sc_lnum += sourcing_lnum_top;
    if (is_export)
    {
	fp->uf_flags |= FC_EXPORT;
	// let ex_export() know the export worked.
	is_export = FALSE;
    }

    if (eap->cmdidx == CMD_def)
	set_function_type(fp);
    else if (fp->uf_script_ctx.sc_version == SCRIPT_VERSION_VIM9)
	// :func does not use Vim9 script syntax, even in a Vim9 script file
	fp->uf_script_ctx.sc_version = SCRIPT_VERSION_MAX;

    goto ret_free;

erret:
    ga_clear_strings(&newargs);
    ga_clear_strings(&default_args);
    if (fp != NULL)
    {
	ga_init(&fp->uf_args);
	ga_init(&fp->uf_def_args);
    }
errret_2:
    ga_clear_strings(&newlines);
    if (fp != NULL)
	VIM_CLEAR(fp->uf_arg_types);
    if (free_fp)
    {
	vim_free(fp);
	fp = NULL;
    }
ret_free:
    ga_clear_strings(&argtypes);
    vim_free(fudi.fd_newkey);
    if (name != name_arg)
	vim_free(name);
    vim_free(ret_type);
    did_emsg |= saved_did_emsg;

    return fp;
}

/*
 * ":function"
 */
    void
ex_function(exarg_T *eap)
{
    char_u *line_to_free = NULL;

    (void)define_function(eap, NULL, &line_to_free);
    vim_free(line_to_free);
}

/*
 * :defcompile - compile all :def functions in the current script that need to
 * be compiled.  Except dead functions.  Doesn't do profiling.
 */
    void
ex_defcompile(exarg_T *eap UNUSED)
{
    long	todo = (long)func_hashtab.ht_used;
    int		changed = func_hashtab.ht_changed;
    hashitem_T	*hi;
    ufunc_T	*ufunc;

    for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    ufunc = HI2UF(hi);
	    if (ufunc->uf_script_ctx.sc_sid == current_sctx.sc_sid
		    && ufunc->uf_def_status == UF_TO_BE_COMPILED
		    && (ufunc->uf_flags & FC_DEAD) == 0)
	    {
		(void)compile_def_function(ufunc, FALSE, CT_NONE, NULL);

		if (func_hashtab.ht_changed != changed)
		{
		    // a function has been added or removed, need to start over
		    todo = (long)func_hashtab.ht_used;
		    changed = func_hashtab.ht_changed;
		    hi = func_hashtab.ht_array;
		    --hi;
		}
	    }
	}
    }
}

/*
 * Return 5 if "p" starts with "<SID>" or "<SNR>" (ignoring case).
 * Return 2 if "p" starts with "s:".
 * Return 0 otherwise.
 */
    int
eval_fname_script(char_u *p)
{
    // Use MB_STRICMP() because in Turkish comparing the "I" may not work with
    // the standard library function.
    if (p[0] == '<' && (MB_STRNICMP(p + 1, "SID>", 4) == 0
				       || MB_STRNICMP(p + 1, "SNR>", 4) == 0))
	return 5;
    if (p[0] == 's' && p[1] == ':')
	return 2;
    return 0;
}

    int
translated_function_exists(char_u *name, int is_global)
{
    if (builtin_function(name, -1))
	return has_internal_func(name);
    return find_func(name, is_global, NULL) != NULL;
}

/*
 * Return TRUE when "ufunc" has old-style "..." varargs
 * or named varargs "...name: type".
 */
    int
has_varargs(ufunc_T *ufunc)
{
    return ufunc->uf_varargs || ufunc->uf_va_name != NULL;
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
    int	    is_global = FALSE;

    flag = TFN_INT | TFN_QUIET | TFN_NO_AUTOLOAD;
    if (no_deref)
	flag |= TFN_NO_DEREF;
    p = trans_function_name(&nm, &is_global, FALSE, flag, NULL, NULL, NULL);
    nm = skipwhite(nm);

    // Only accept "funcname", "funcname ", "funcname (..." and
    // "funcname(...", not "funcname!...".
    if (p != NULL && (*nm == NUL || *nm == '('))
	n = translated_function_exists(p, is_global);
    vim_free(p);
    return n;
}

#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3) || defined(PROTO)
    char_u *
get_expanded_name(char_u *name, int check)
{
    char_u	*nm = name;
    char_u	*p;
    int		is_global = FALSE;

    p = trans_function_name(&nm, &is_global, FALSE,
					  TFN_INT|TFN_QUIET, NULL, NULL, NULL);

    if (p != NULL && *nm == NUL
		       && (!check || translated_function_exists(p, is_global)))
	return p;

    vim_free(p);
    return NULL;
}
#endif

/*
 * Function given to ExpandGeneric() to obtain the list of user defined
 * function names.
 */
    char_u *
get_user_func_name(expand_T *xp, int idx)
{
    static long_u	done;
    static int		changed;
    static hashitem_T	*hi;
    ufunc_T		*fp;

    if (idx == 0)
    {
	done = 0;
	hi = func_hashtab.ht_array;
	changed = func_hashtab.ht_changed;
    }
    if (changed == func_hashtab.ht_changed && done < func_hashtab.ht_used)
    {
	if (done++ > 0)
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	fp = HI2UF(hi);

	// don't show dead, dict and lambda functions
	if ((fp->uf_flags & FC_DEAD) || (fp->uf_flags & FC_DICT)
				|| STRNCMP(fp->uf_name, "<lambda>", 8) == 0)
	    return (char_u *)"";

	if (STRLEN(fp->uf_name) + 4 >= IOSIZE)
	    return fp->uf_name;	// prevents overflow

	cat_func_name(IObuff, fp);
	if (xp->xp_context != EXPAND_USER_FUNC
				       && xp->xp_context != EXPAND_DISASSEMBLE)
	{
	    STRCAT(IObuff, "(");
	    if (!has_varargs(fp) && fp->uf_args.ga_len == 0)
		STRCAT(IObuff, ")");
	}
	return IObuff;
    }
    return NULL;
}

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
    int		is_global = FALSE;

    p = eap->arg;
    name = trans_function_name(&p, &is_global, eap->skip, 0, &fudi,
								   NULL, NULL);
    vim_free(fudi.fd_newkey);
    if (name == NULL)
    {
	if (fudi.fd_dict != NULL && !eap->skip)
	    emsg(_(e_funcref_required));
	return;
    }
    if (!ends_excmd(*skipwhite(p)))
    {
	vim_free(name);
	semsg(_(e_trailing_characters_str), p);
	return;
    }
    set_nextcmd(eap, p);
    if (eap->nextcmd != NULL)
	*p = NUL;

    if (numbered_function(name) && fudi.fd_dict == NULL)
    {
	if (!eap->skip)
	    semsg(_(e_invalid_argument_str), eap->arg);
	vim_free(name);
	return;
    }
    if (!eap->skip)
	fp = find_func(name, is_global, NULL);
    vim_free(name);

    if (!eap->skip)
    {
	if (fp == NULL)
	{
	    if (!eap->forceit)
		semsg(_(e_unknown_function_str), eap->arg);
	    return;
	}
	if (fp->uf_calls > 0)
	{
	    semsg(_(e_cannot_delete_function_str_it_is_in_use), eap->arg);
	    return;
	}
	if (fp->uf_flags & FC_VIM9)
	{
	    semsg(_(e_cannot_delete_vim9_script_function_str), eap->arg);
	    return;
	}

	if (fudi.fd_dict != NULL)
	{
	    // Delete the dict item that refers to the function, it will
	    // invoke func_unref() and possibly delete the function.
	    dictitem_remove(fudi.fd_dict, fudi.fd_di);
	}
	else
	{
	    // A normal function (not a numbered function or lambda) has a
	    // refcount of 1 for the entry in the hashtable.  When deleting
	    // it and the refcount is more than one, it should be kept.
	    // A numbered function and lambda should be kept if the refcount is
	    // one or more.
	    if (fp->uf_refcount > (func_name_refcount(fp->uf_name) ? 0 : 1))
	    {
		// Function is still referenced somewhere.  Don't free it but
		// do remove it from the hashtable.
		if (func_remove(fp))
		    fp->uf_refcount--;
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
    fp = find_func(name, FALSE, NULL);
    if (fp == NULL && numbered_function(name))
    {
#ifdef EXITFREE
	if (!entered_free_all_mem)
#endif
	    internal_error("func_unref()");
    }
    func_ptr_unref(fp);
}

/*
 * Unreference a Function: decrement the reference count and free it when it
 * becomes zero.
 * Also when it becomes one and uf_partial points to the function.
 */
    void
func_ptr_unref(ufunc_T *fp)
{
    if (fp != NULL && (--fp->uf_refcount <= 0
		|| (fp->uf_refcount == 1 && fp->uf_partial != NULL
					 && fp->uf_partial->pt_refcount <= 1
					 && fp->uf_partial->pt_func == fp)))
    {
	// Only delete it when it's not being used.  Otherwise it's done
	// when "uf_calls" becomes zero.
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
    fp = find_func(name, FALSE, NULL);
    if (fp != NULL)
	++fp->uf_refcount;
    else if (numbered_function(name))
	// Only give an error for a numbered function.
	// Fail silently, when named or lambda function isn't found.
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
    evalarg_T	evalarg;

    if (current_funccal == NULL)
    {
	emsg(_(e_return_not_inside_function));
	return;
    }

    init_evalarg(&evalarg);
    evalarg.eval_flags = eap->skip ? 0 : EVAL_EVALUATE;

    if (eap->skip)
	++emsg_skip;

    eap->nextcmd = NULL;
    if ((*arg != NUL && *arg != '|' && *arg != '\n')
				  && eval0(arg, &rettv, eap, &evalarg) != FAIL)
    {
	if (!eap->skip)
	    returning = do_return(eap, FALSE, TRUE, &rettv);
	else
	    clear_tv(&rettv);
    }
    // It's safer to return also on error.
    else if (!eap->skip)
    {
	// In return statement, cause_abort should be force_abort.
	update_force_abort();

	/*
	 * Return unless the expression evaluation has been cancelled due to an
	 * aborting error, an interrupt, or an exception.
	 */
	if (!aborting())
	    returning = do_return(eap, FALSE, TRUE, NULL);
    }

    // When skipping or the return gets pending, advance to the next command
    // in this line (!returning).  Otherwise, ignore the rest of the line.
    // Following lines will be ignored by get_func_line().
    if (returning)
	eap->nextcmd = NULL;
    else if (eap->nextcmd == NULL)	    // no argument
	set_nextcmd(eap, arg);

    if (eap->skip)
	--emsg_skip;
    clear_evalarg(&evalarg, eap);
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
    evalarg_T	evalarg;
    type_T	*type = NULL;
    int		found_var = FALSE;

    fill_evalarg_from_eap(&evalarg, eap, eap->skip);
    if (eap->skip)
    {
	// trans_function_name() doesn't work well when skipping, use eval0()
	// instead to skip to any following command, e.g. for:
	//   :if 0 | call dict.foo().bar() | endif
	++emsg_skip;
	if (eval0(eap->arg, &rettv, eap, &evalarg) != FAIL)
	    clear_tv(&rettv);
	--emsg_skip;
	clear_evalarg(&evalarg, eap);
	return;
    }

    tofree = trans_function_name(&arg, NULL, eap->skip, TFN_INT,
			      &fudi, &partial, in_vim9script() ? &type : NULL);
    if (fudi.fd_newkey != NULL)
    {
	// Still need to give an error message for missing key.
	semsg(_(e_key_not_present_in_dictionary), fudi.fd_newkey);
	vim_free(fudi.fd_newkey);
    }
    if (tofree == NULL)
	return;

    // Increase refcount on dictionary, it could get deleted when evaluating
    // the arguments.
    if (fudi.fd_dict != NULL)
	++fudi.fd_dict->dv_refcount;

    // If it is the name of a variable of type VAR_FUNC or VAR_PARTIAL use its
    // contents.  For VAR_PARTIAL get its partial, unless we already have one
    // from trans_function_name().
    len = (int)STRLEN(tofree);
    name = deref_func_name(tofree, &len, partial != NULL ? NULL : &partial,
	    in_vim9script() && type == NULL ? &type : NULL, FALSE, &found_var);

    // Skip white space to allow ":call func ()".  Not good, but required for
    // backward compatibility.
    startarg = skipwhite(arg);
    if (*startarg != '(')
    {
	semsg(_(e_missing_parenthesis_str), eap->arg);
	goto end;
    }
    if (in_vim9script() && startarg > arg)
    {
	semsg(_(e_no_white_space_allowed_before_str_str), "(", eap->arg);
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
	lnum = eap->line2;	// do it once, also with an invalid range
    }
    else
	lnum = eap->line1;
    for ( ; lnum <= eap->line2; ++lnum)
    {
	funcexe_T funcexe;

	if (!eap->skip && eap->addr_count > 0)
	{
	    if (lnum > curbuf->b_ml.ml_line_count)
	    {
		// If the function deleted lines or switched to another buffer
		// the line number may become invalid.
		emsg(_(e_invalid_range));
		break;
	    }
	    curwin->w_cursor.lnum = lnum;
	    curwin->w_cursor.col = 0;
	    curwin->w_cursor.coladd = 0;
	}
	arg = startarg;

	CLEAR_FIELD(funcexe);
	funcexe.fe_firstline = eap->line1;
	funcexe.fe_lastline = eap->line2;
	funcexe.fe_doesrange = &doesrange;
	funcexe.fe_evaluate = !eap->skip;
	funcexe.fe_partial = partial;
	funcexe.fe_selfdict = fudi.fd_dict;
	funcexe.fe_check_type = type;
	funcexe.fe_found_var = found_var;
	rettv.v_type = VAR_UNKNOWN;	// clear_tv() uses this
	if (get_func_tv(name, -1, &rettv, &arg, &evalarg, &funcexe) == FAIL)
	{
	    failed = TRUE;
	    break;
	}
	if (has_watchexpr())
	    dbg_check_breakpoint(eap);

	// Handle a function returning a Funcref, Dictionary or List.
	if (handle_subscript(&arg, NULL, &rettv,
			   eap->skip ? NULL : &EVALARG_EVALUATE, TRUE) == FAIL)
	{
	    failed = TRUE;
	    break;
	}

	clear_tv(&rettv);
	if (doesrange || eap->skip)
	    break;

	// Stop when immediately aborting on error, or when an interrupt
	// occurred or an exception was thrown but not caught.
	// get_func_tv() returned OK, so that the check for trailing
	// characters below is executed.
	if (aborting())
	    break;
    }
    if (eap->skip)
	--emsg_skip;
    clear_evalarg(&evalarg, eap);

    // When inside :try we need to check for following "| catch" or "| endtry".
    // Not when there was an error, but do check if an exception was thrown.
    if ((!aborting() || did_throw)
				  && (!failed || eap->cstack->cs_trylevel > 0))
    {
	// Check for trailing illegal characters and a following command.
	arg = skipwhite(arg);
	if (!ends_excmd2(eap->arg, arg))
	{
	    if (!failed && !aborting())
	    {
		emsg_severe = TRUE;
		semsg(_(e_trailing_characters_str), arg);
	    }
	}
	else
	    set_nextcmd(eap, arg);
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
    cstack_T	*cstack = eap->cstack;

    if (reanimate)
	// Undo the return.
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
	    // A pending return again gets pending.  "rettv" points to an
	    // allocated variable with the rettv of the original ":return"'s
	    // argument if present or is NULL else.
	    cstack->cs_rettv[idx] = rettv;
	else
	{
	    // When undoing a return in order to make it pending, get the stored
	    // return rettv.
	    if (reanimate)
		rettv = current_funccal->rettv;

	    if (rettv != NULL)
	    {
		// Store the value of the pending return.
		if ((cstack->cs_rettv[idx] = alloc_tv()) != NULL)
		    *(typval_T *)cstack->cs_rettv[idx] = *(typval_T *)rettv;
		else
		    emsg(_(e_out_of_memory));
	    }
	    else
		cstack->cs_rettv[idx] = NULL;

	    if (reanimate)
	    {
		// The pending return value could be overwritten by a ":return"
		// without argument in a finally clause; reset the default
		// return value.
		current_funccal->rettv->v_type = VAR_NUMBER;
		current_funccal->rettv->vval.v_number = 0;
	    }
	}
	report_make_pending(CSTP_RETURN, rettv);
    }
    else
    {
	current_funccal->returned = TRUE;

	// If the return is carried out now, store the return value.  For
	// a return immediately after reanimation, the value is already
	// there.
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
    int	    indent UNUSED,
    getline_opt_T options UNUSED)
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;
    char_u	*retval;
    garray_T	*gap;  // growarray with function lines

    // If breakpoints have been added/deleted need to check for it.
    if (fcp->dbg_tick != debug_tick)
    {
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name,
							       SOURCING_LNUM);
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
	// Skip NULL lines (continuation lines).
	while (fcp->linenr < gap->ga_len
			  && ((char_u **)(gap->ga_data))[fcp->linenr] == NULL)
	    ++fcp->linenr;
	if (fcp->linenr >= gap->ga_len)
	    retval = NULL;
	else
	{
	    retval = vim_strsave(((char_u **)(gap->ga_data))[fcp->linenr++]);
	    SOURCING_LNUM = fcp->linenr;
#ifdef FEAT_PROFILE
	    if (do_profiling == PROF_YES)
		func_line_start(cookie, SOURCING_LNUM);
#endif
	}
    }

    // Did we encounter a breakpoint?
    if (fcp->breakpoint != 0 && fcp->breakpoint <= SOURCING_LNUM)
    {
	dbg_breakpoint(fp->uf_name, SOURCING_LNUM);
	// Find next breakpoint.
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name,
							       SOURCING_LNUM);
	fcp->dbg_tick = debug_tick;
    }

    return retval;
}

/*
 * Return TRUE if the currently active function should be ended, because a
 * return was encountered or an error occurred.  Used inside a ":while".
 */
    int
func_has_ended(void *cookie)
{
    funccall_T  *fcp = (funccall_T *)cookie;

    // Ignore the "abort" flag if the abortion behavior has been changed due to
    // an error inside a try conditional.
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
	// Translate "s:func" to the stored function name.
	fname = fname_trans_sid(fname, fname_buf, &tofree, &error);
	fp = find_func(fname, FALSE, NULL);
	vim_free(tofree);
    }

    if (fp != NULL && (fp->uf_flags & FC_DICT))
    {
	partial_T	*pt = ALLOC_CLEAR_ONE(partial_T);

	if (pt != NULL)
	{
	    pt->pt_refcount = 1;
	    pt->pt_dict = selfdict;
	    pt->pt_auto = TRUE;
	    selfdict = NULL;
	    if (rettv->v_type == VAR_FUNC)
	    {
		// Just a function: Take over the function name and use
		// selfdict.
		pt->pt_name = rettv->vval.v_string;
	    }
	    else
	    {
		partial_T	*ret_pt = rettv->vval.v_partial;
		int		i;

		// Partial: copy the function name, use selfdict and copy
		// args.  Can't take over name or args, the partial might
		// be referenced elsewhere.
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
		    pt->pt_argv = ALLOC_MULT(typval_T, ret_pt->pt_argc);
		    if (pt->pt_argv == NULL)
			// out of memory: drop the arguments
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
	    free_funccal_contents(fc);
	    did_free = TRUE;
	    did_free_funccal = TRUE;
	}
	else
	    pfc = &(*pfc)->caller;
    }
    if (did_free_funccal)
	// When a funccal was freed some more items might be garbage
	// collected, so run again.
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
		// backtrace level overflow. reset to max
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
    if (current_funccal == NULL || current_funccal->l_vars.dv_refcount == 0)
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
    if (current_funccal == NULL || current_funccal->l_vars.dv_refcount == 0)
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
    if (current_funccal == NULL || current_funccal->l_vars.dv_refcount == 0)
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
    if (current_funccal == NULL || current_funccal->l_vars.dv_refcount == 0)
	return NULL;
    return &get_funccal()->l_avars_var;
}

/*
 * List function variables, if there is a function.
 */
    void
list_func_vars(int *first)
{
    if (current_funccal != NULL && current_funccal->l_vars.dv_refcount > 0)
	list_hashtable_vars(&current_funccal->l_vars.dv_hashtab,
							   "l:", FALSE, first);
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

    // Search in parent scope, which can be referenced from a lambda.
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

    // Search in parent scope which is possible to reference from lambda
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
    funccall_T	*fc;

    for (fc = previous_funccal; fc != NULL; fc = fc->caller)
    {
	fc->fc_copyID = copyID + 1;
	if (set_ref_in_ht(&fc->l_vars.dv_hashtab, copyID + 1, NULL)
		|| set_ref_in_ht(&fc->l_avars.dv_hashtab, copyID + 1, NULL)
		|| set_ref_in_list_items(&fc->l_varlist, copyID + 1, NULL))
	    return TRUE;
    }
    return FALSE;
}

    static int
set_ref_in_funccal(funccall_T *fc, int copyID)
{
    if (fc->fc_copyID != copyID)
    {
	fc->fc_copyID = copyID;
	if (set_ref_in_ht(&fc->l_vars.dv_hashtab, copyID, NULL)
		|| set_ref_in_ht(&fc->l_avars.dv_hashtab, copyID, NULL)
		|| set_ref_in_list_items(&fc->l_varlist, copyID, NULL)
		|| set_ref_in_func(NULL, fc->func, copyID))
	    return TRUE;
    }
    return FALSE;
}

/*
 * Set "copyID" in all local vars and arguments in the call stack.
 */
    int
set_ref_in_call_stack(int copyID)
{
    funccall_T		*fc;
    funccal_entry_T	*entry;

    for (fc = current_funccal; fc != NULL; fc = fc->caller)
	if (set_ref_in_funccal(fc, copyID))
	    return TRUE;

    // Also go through the funccal_stack.
    for (entry = funccal_stack; entry != NULL; entry = entry->next)
	for (fc = entry->top_funccal; fc != NULL; fc = fc->caller)
	    if (set_ref_in_funccal(fc, copyID))
		return TRUE;
    return FALSE;
}

/*
 * Set "copyID" in all functions available by name.
 */
    int
set_ref_in_functions(int copyID)
{
    int		todo;
    hashitem_T	*hi = NULL;
    ufunc_T	*fp;

    todo = (int)func_hashtab.ht_used;
    for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    fp = HI2UF(hi);
	    if (!func_name_refcount(fp->uf_name)
					  && set_ref_in_func(NULL, fp, copyID))
		return TRUE;
	}
    }
    return FALSE;
}

/*
 * Set "copyID" in all function arguments.
 */
    int
set_ref_in_func_args(int copyID)
{
    int i;

    for (i = 0; i < funcargs.ga_len; ++i)
	if (set_ref_in_item(((typval_T **)funcargs.ga_data)[i],
							  copyID, NULL, NULL))
	    return TRUE;
    return FALSE;
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
    int		error = FCERR_NONE;
    char_u	fname_buf[FLEN_FIXED + 1];
    char_u	*tofree = NULL;
    char_u	*fname;
    int		abort = FALSE;

    if (name == NULL && fp_in == NULL)
	return FALSE;

    if (fp_in == NULL)
    {
	fname = fname_trans_sid(name, fname_buf, &tofree, &error);
	fp = find_func(fname, FALSE, NULL);
    }
    if (fp != NULL)
    {
	for (fc = fp->uf_scoped; fc != NULL; fc = fc->func->uf_scoped)
	    abort = abort || set_ref_in_funccal(fc, copyID);
    }

    vim_free(tofree);
    return abort;
}

#endif // FEAT_EVAL
