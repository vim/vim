/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9cmds.c: Dealing with compiled function expressions
 */

#define USING_FLOAT_STUFF
#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

// When not generating protos this is included in proto.h
#ifdef PROTO
# include "vim9.h"
#endif

/*
 * Generate code for any ppconst entries.
 */
    int
generate_ppconst(cctx_T *cctx, ppconst_T *ppconst)
{
    int	    i;
    int	    ret = OK;
    int	    save_skip = cctx->ctx_skip;

    cctx->ctx_skip = SKIP_NOT;
    for (i = 0; i < ppconst->pp_used; ++i)
	if (generate_tv_PUSH(cctx, &ppconst->pp_tv[i]) == FAIL)
	    ret = FAIL;
    ppconst->pp_used = 0;
    cctx->ctx_skip = save_skip;
    return ret;
}

/*
 * Check that the last item of "ppconst" is a bool, if there is an item.
 */
    static int
check_ppconst_bool(ppconst_T *ppconst)
{
    if (ppconst->pp_used > 0)
    {
	typval_T    *tv = &ppconst->pp_tv[ppconst->pp_used - 1];
	where_T	    where = WHERE_INIT;

	return check_typval_type(&t_bool, tv, where);
    }
    return OK;
}

/*
 * Clear ppconst constants.  Used when failing.
 */
    void
clear_ppconst(ppconst_T *ppconst)
{
    int	    i;

    for (i = 0; i < ppconst->pp_used; ++i)
	clear_tv(&ppconst->pp_tv[i]);
    ppconst->pp_used = 0;
}

/*
 * Compile getting a member from a list/dict/string/blob.  Stack has the
 * indexable value and the index or the two indexes of a slice.
 * "keeping_dict" is used for dict[func](arg) to pass dict to func.
 */
    int
compile_member(int is_slice, int *keeping_dict, cctx_T *cctx)
{
    type2_T	*typep;
    garray_T	*stack = &cctx->ctx_type_stack;
    vartype_T	vartype;
    type_T	*idxtype;

    // We can index a list, dict and blob.  If we don't know the type
    // we can use the index value type.  If we still don't know use an "ANY"
    // instruction.
    // TODO: what about the decl type?
    typep = (((type2_T *)stack->ga_data) + stack->ga_len - (is_slice ? 3 : 2));
    vartype = typep->type_curr->tt_type;
    idxtype = (((type2_T *)stack->ga_data) + stack->ga_len - 1)->type_curr;
    // If the index is a string, the variable must be a Dict.
    if ((typep->type_curr == &t_any || typep->type_curr == &t_unknown)
						       && idxtype == &t_string)
	vartype = VAR_DICT;
    if (vartype == VAR_STRING || vartype == VAR_LIST || vartype == VAR_BLOB)
    {
	if (need_type(idxtype, &t_number, -1, 0, cctx, FALSE, FALSE) == FAIL)
	    return FAIL;
	if (is_slice)
	{
	    idxtype = get_type_on_stack(cctx, 1);
	    if (need_type(idxtype, &t_number, -2, 0, cctx,
							 FALSE, FALSE) == FAIL)
		return FAIL;
	}
    }

    if (vartype == VAR_DICT)
    {
	if (is_slice)
	{
	    emsg(_(e_cannot_slice_dictionary));
	    return FAIL;
	}
	if (typep->type_curr->tt_type == VAR_DICT)
	{
	    typep->type_curr = typep->type_curr->tt_member;
	    if (typep->type_curr == &t_unknown)
		// empty dict was used
		typep->type_curr = &t_any;
	    if (typep->type_decl->tt_type == VAR_DICT)
	    {
		typep->type_decl = typep->type_decl->tt_member;
		if (typep->type_decl == &t_unknown)
		    // empty dict was used
		    typep->type_decl = &t_any;
	    }
	    else
		typep->type_decl = typep->type_curr;
	}
	else
	{
	    if (need_type(typep->type_curr, &t_dict_any, -2, 0, cctx,
							 FALSE, FALSE) == FAIL)
		return FAIL;
	    typep->type_curr = &t_any;
	    typep->type_decl = &t_any;
	}
	if (may_generate_2STRING(-1, FALSE, cctx) == FAIL)
	    return FAIL;
	if (generate_instr_drop(cctx, ISN_MEMBER, 1) == FAIL)
	    return FAIL;
	if (keeping_dict != NULL)
	    *keeping_dict = TRUE;
    }
    else if (vartype == VAR_STRING)
    {
	typep->type_curr = &t_string;
	typep->type_decl = &t_string;
	if ((is_slice
		? generate_instr_drop(cctx, ISN_STRSLICE, 2)
		: generate_instr_drop(cctx, ISN_STRINDEX, 1)) == FAIL)
	    return FAIL;
    }
    else if (vartype == VAR_BLOB)
    {
	if (is_slice)
	{
	    typep->type_curr = &t_blob;
	    typep->type_decl = &t_blob;
	    if (generate_instr_drop(cctx, ISN_BLOBSLICE, 2) == FAIL)
		return FAIL;
	}
	else
	{
	    typep->type_curr = &t_number;
	    typep->type_decl = &t_number;
	    if (generate_instr_drop(cctx, ISN_BLOBINDEX, 1) == FAIL)
		return FAIL;
	}
    }
    else if (vartype == VAR_LIST || typep->type_curr == &t_any
					     || typep->type_curr == &t_unknown)
    {
	if (is_slice)
	{
	    if (generate_instr_drop(cctx,
		     vartype == VAR_LIST ?  ISN_LISTSLICE : ISN_ANYSLICE,
							    2) == FAIL)
		return FAIL;
	}
	else
	{
	    if (typep->type_curr->tt_type == VAR_LIST)
	    {
		typep->type_curr = typep->type_curr->tt_member;
		if (typep->type_curr == &t_unknown)
		    // empty list was used
		    typep->type_curr = &t_any;
		if (typep->type_decl->tt_type == VAR_LIST)
		{
		    typep->type_decl = typep->type_decl->tt_member;
		    if (typep->type_decl == &t_unknown)
			// empty list was used
			typep->type_decl = &t_any;
		}
		else
			typep->type_decl = typep->type_curr;
	    }
	    if (generate_instr_drop(cctx,
			vartype == VAR_LIST ?  ISN_LISTINDEX : ISN_ANYINDEX, 1)
								       == FAIL)
		return FAIL;
	}
    }
    else
    {
	switch (vartype)
	{
	    case VAR_FUNC:
	    case VAR_PARTIAL:
		emsg(_(e_cannot_index_a_funcref));
		break;
	    case VAR_BOOL:
	    case VAR_SPECIAL:
	    case VAR_JOB:
	    case VAR_CHANNEL:
	    case VAR_INSTR:
	    case VAR_UNKNOWN:
	    case VAR_ANY:
	    case VAR_VOID:
		emsg(_(e_cannot_index_special_variable));
		break;
	    default:
		emsg(_(e_string_list_dict_or_blob_required));
	}
	return FAIL;
    }
    return OK;
}

/*
 * Generate an instruction to load script-local variable "name", without the
 * leading "s:".
 * Also finds imported variables.
 */
    int
compile_load_scriptvar(
	cctx_T *cctx,
	char_u *name,	    // variable NUL terminated
	char_u *start,	    // start of variable
	char_u **end,	    // end of variable, may be NULL
	int    error)	    // when TRUE may give error
{
    scriptitem_T    *si;
    int		    idx;
    imported_T	    *import;

    if (!SCRIPT_ID_VALID(current_sctx.sc_sid))
	return FAIL;
    si = SCRIPT_ITEM(current_sctx.sc_sid);
    idx = get_script_item_idx(current_sctx.sc_sid, name, 0, cctx);
    if (idx == -1 || si->sn_version != SCRIPT_VERSION_VIM9)
    {
	// variable is not in sn_var_vals: old style script.
	return generate_OLDSCRIPT(cctx, ISN_LOADS, name, current_sctx.sc_sid,
								       &t_any);
    }
    if (idx >= 0)
    {
	svar_T		*sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;

	generate_VIM9SCRIPT(cctx, ISN_LOADSCRIPT,
					current_sctx.sc_sid, idx, sv->sv_type);
	return OK;
    }

    import = end == NULL ? NULL : find_imported(name, 0, cctx);
    if (import != NULL)
    {
	char_u	*p = skipwhite(*end);
	char_u	*exp_name;
	int	cc;
	ufunc_T	*ufunc;
	type_T	*type;

	// Need to lookup the member.
	if (*p != '.')
	{
	    semsg(_(e_expected_dot_after_name_str), start);
	    return FAIL;
	}
	++p;
	if (VIM_ISWHITE(*p))
	{
	    emsg(_(e_no_white_space_allowed_after_dot));
	    return FAIL;
	}

	// isolate one name
	exp_name = p;
	while (eval_isnamec(*p))
	    ++p;
	cc = *p;
	*p = NUL;

	idx = find_exported(import->imp_sid, exp_name, &ufunc, &type,
								   cctx, TRUE);
	*p = cc;
	p = skipwhite(p);
	*end = p;

	if (idx < 0)
	{
	    if (ufunc != NULL)
	    {
		// function call or function reference
		generate_PUSHFUNC(cctx, ufunc->uf_name, NULL);
		return OK;
	    }
	    return FAIL;
	}

	generate_VIM9SCRIPT(cctx, ISN_LOADSCRIPT,
		import->imp_sid,
		idx,
		type);
	return OK;
    }

    if (error)
	semsg(_(e_item_not_found_str), name);
    return FAIL;
}

    static int
generate_funcref(cctx_T *cctx, char_u *name)
{
    ufunc_T *ufunc = find_func(name, FALSE, cctx);

    if (ufunc == NULL)
	return FAIL;

    // Need to compile any default values to get the argument types.
    if (func_needs_compiling(ufunc, COMPILE_TYPE(ufunc))
	    && compile_def_function(ufunc, TRUE, COMPILE_TYPE(ufunc), NULL)
								       == FAIL)
	return FAIL;
    return generate_PUSHFUNC(cctx, ufunc->uf_name, ufunc->uf_func_type);
}

/*
 * Compile a variable name into a load instruction.
 * "end" points to just after the name.
 * "is_expr" is TRUE when evaluating an expression, might be a funcref.
 * When "error" is FALSE do not give an error when not found.
 */
    int
compile_load(
	char_u **arg,
	char_u *end_arg,
	cctx_T	*cctx,
	int	is_expr,
	int	error)
{
    type_T	*type;
    char_u	*name = NULL;
    char_u	*end = end_arg;
    int		res = FAIL;
    int		prev_called_emsg = called_emsg;

    if (*(*arg + 1) == ':')
    {
	if (end <= *arg + 2)
	{
	    isntype_T  isn_type;

	    // load dictionary of namespace
	    switch (**arg)
	    {
		case 'g': isn_type = ISN_LOADGDICT; break;
		case 'w': isn_type = ISN_LOADWDICT; break;
		case 't': isn_type = ISN_LOADTDICT; break;
		case 'b': isn_type = ISN_LOADBDICT; break;
		default:
		    semsg(_(e_namespace_not_supported_str), *arg);
		    goto theend;
	    }
	    if (generate_instr_type(cctx, isn_type, &t_dict_any) == NULL)
		goto theend;
	    res = OK;
	}
	else
	{
	    isntype_T  isn_type = ISN_DROP;

	    // load namespaced variable
	    name = vim_strnsave(*arg + 2, end - (*arg + 2));
	    if (name == NULL)
		return FAIL;

	    switch (**arg)
	    {
		case 'v': res = generate_LOADV(cctx, name, error);
			  break;
		case 's': if (is_expr && ASCII_ISUPPER(*name)
				       && find_func(name, FALSE, cctx) != NULL)
			      res = generate_funcref(cctx, name);
			  else
			      res = compile_load_scriptvar(cctx, name,
							    NULL, &end, error);
			  break;
		case 'g': if (vim_strchr(name, AUTOLOAD_CHAR) == NULL)
			  {
			      if (is_expr && ASCII_ISUPPER(*name)
				       && find_func(name, FALSE, cctx) != NULL)
				  res = generate_funcref(cctx, name);
			      else
				  isn_type = ISN_LOADG;
			  }
			  else
			  {
			      isn_type = ISN_LOADAUTO;
			      vim_free(name);
			      name = vim_strnsave(*arg, end - *arg);
			      if (name == NULL)
				  return FAIL;
			  }
			  break;
		case 'w': isn_type = ISN_LOADW; break;
		case 't': isn_type = ISN_LOADT; break;
		case 'b': isn_type = ISN_LOADB; break;
		default:  // cannot happen, just in case
			  semsg(_(e_namespace_not_supported_str), *arg);
			  goto theend;
	    }
	    if (isn_type != ISN_DROP)
	    {
		// Global, Buffer-local, Window-local and Tabpage-local
		// variables can be defined later, thus we don't check if it
		// exists, give an error at runtime.
		res = generate_LOAD(cctx, isn_type, 0, name, &t_any);
	    }
	}
    }
    else
    {
	size_t	    len = end - *arg;
	int	    idx;
	int	    gen_load = FALSE;
	int	    gen_load_outer = 0;

	name = vim_strnsave(*arg, end - *arg);
	if (name == NULL)
	    return FAIL;

	if (vim_strchr(name, AUTOLOAD_CHAR) != NULL)
	{
	    script_autoload(name, FALSE);
	    res = generate_LOAD(cctx, ISN_LOADAUTO, 0, name, &t_any);
	}
	else if (arg_exists(*arg, len, &idx, &type, &gen_load_outer, cctx)
									 == OK)
	{
	    if (gen_load_outer == 0)
		gen_load = TRUE;
	}
	else
	{
	    lvar_T lvar;

	    if (lookup_local(*arg, len, &lvar, cctx) == OK)
	    {
		type = lvar.lv_type;
		idx = lvar.lv_idx;
		if (lvar.lv_from_outer != 0)
		    gen_load_outer = lvar.lv_from_outer;
		else
		    gen_load = TRUE;
	    }
	    else
	    {
		// "var" can be script-local even without using "s:" if it
		// already exists in a Vim9 script or when it's imported.
		if (script_var_exists(*arg, len, cctx) == OK
			|| find_imported(name, 0, cctx) != NULL)
		   res = compile_load_scriptvar(cctx, name, *arg, &end, FALSE);

		// When evaluating an expression and the name starts with an
		// uppercase letter it can be a user defined function.
		// generate_funcref() will fail if the function can't be found.
		if (res == FAIL && is_expr && ASCII_ISUPPER(*name))
		    res = generate_funcref(cctx, name);
	    }
	}
	if (gen_load)
	    res = generate_LOAD(cctx, ISN_LOAD, idx, NULL, type);
	if (gen_load_outer > 0)
	{
	    res = generate_LOADOUTER(cctx, idx, gen_load_outer, type);
	    cctx->ctx_outer_used = TRUE;
	}
    }

    *arg = end;

theend:
    if (res == FAIL && error && called_emsg == prev_called_emsg)
	semsg(_(e_variable_not_found_str), name);
    vim_free(name);
    return res;
}

/*
 * Compile a string in a ISN_PUSHS instruction into an ISN_INSTR.
 * Returns FAIL if compilation fails.
 */
    static int
compile_string(isn_T *isn, cctx_T *cctx)
{
    char_u	*s = isn->isn_arg.string;
    garray_T	save_ga = cctx->ctx_instr;
    int		expr_res;
    int		trailing_error;
    int		instr_count;
    isn_T	*instr = NULL;

    // Remove the string type from the stack.
    --cctx->ctx_type_stack.ga_len;

    // Temporarily reset the list of instructions so that the jump labels are
    // correct.
    cctx->ctx_instr.ga_len = 0;
    cctx->ctx_instr.ga_maxlen = 0;
    cctx->ctx_instr.ga_data = NULL;
    expr_res = compile_expr0(&s, cctx);
    s = skipwhite(s);
    trailing_error = *s != NUL;

    if (expr_res == FAIL || trailing_error
				       || GA_GROW_FAILS(&cctx->ctx_instr, 1))
    {
	if (trailing_error)
	    semsg(_(e_trailing_characters_str), s);
	clear_instr_ga(&cctx->ctx_instr);
	cctx->ctx_instr = save_ga;
	++cctx->ctx_type_stack.ga_len;
	return FAIL;
    }

    // Move the generated instructions into the ISN_INSTR instruction, then
    // restore the list of instructions.
    instr_count = cctx->ctx_instr.ga_len;
    instr = cctx->ctx_instr.ga_data;
    instr[instr_count].isn_type = ISN_FINISH;

    cctx->ctx_instr = save_ga;
    vim_free(isn->isn_arg.string);
    isn->isn_type = ISN_INSTR;
    isn->isn_arg.instr = instr;
    return OK;
}

/*
 * Compile the argument expressions.
 * "arg" points to just after the "(" and is advanced to after the ")"
 */
    static int
compile_arguments(char_u **arg, cctx_T *cctx, int *argcount, int is_searchpair)
{
    char_u  *p = *arg;
    char_u  *whitep = *arg;
    int	    must_end = FALSE;
    int	    instr_count;

    for (;;)
    {
	if (may_get_next_line(whitep, &p, cctx) == FAIL)
	    goto failret;
	if (*p == ')')
	{
	    *arg = p + 1;
	    return OK;
	}
	if (must_end)
	{
	    semsg(_(e_missing_comma_before_argument_str), p);
	    return FAIL;
	}

	instr_count = cctx->ctx_instr.ga_len;
	if (compile_expr0(&p, cctx) == FAIL)
	    return FAIL;
	++*argcount;

	if (is_searchpair && *argcount == 5
		&& cctx->ctx_instr.ga_len == instr_count + 1)
	{
	    isn_T *isn = ((isn_T *)cctx->ctx_instr.ga_data) + instr_count;

	    // {skip} argument of searchpair() can be compiled if not empty
	    if (isn->isn_type == ISN_PUSHS && *isn->isn_arg.string != NUL)
		compile_string(isn, cctx);
	}

	if (*p != ',' && *skipwhite(p) == ',')
	{
	    semsg(_(e_no_white_space_allowed_before_str_str), ",", p);
	    p = skipwhite(p);
	}
	if (*p == ',')
	{
	    ++p;
	    if (*p != NUL && !VIM_ISWHITE(*p))
		semsg(_(e_white_space_required_after_str_str), ",", p - 1);
	}
	else
	    must_end = TRUE;
	whitep = p;
	p = skipwhite(p);
    }
failret:
    emsg(_(e_missing_closing_paren));
    return FAIL;
}

/*
 * Compile a function call:  name(arg1, arg2)
 * "arg" points to "name", "arg + varlen" to the "(".
 * "argcount_init" is 1 for "value->method()"
 * Instructions:
 *	EVAL arg1
 *	EVAL arg2
 *	BCALL / DCALL / UCALL
 */
    static int
compile_call(
	char_u	    **arg,
	size_t	    varlen,
	cctx_T	    *cctx,
	ppconst_T   *ppconst,
	int	    argcount_init)
{
    char_u	*name = *arg;
    char_u	*p;
    int		argcount = argcount_init;
    char_u	namebuf[100];
    char_u	fname_buf[FLEN_FIXED + 1];
    char_u	*tofree = NULL;
    int		error = FCERR_NONE;
    ufunc_T	*ufunc = NULL;
    int		res = FAIL;
    int		is_autoload;
    int		is_searchpair;

    // We can evaluate "has('name')" at compile time.
    // We always evaluate "exists_compiled()" at compile time.
    if ((varlen == 3 && STRNCMP(*arg, "has", 3) == 0)
	    || (varlen == 15 && STRNCMP(*arg, "exists_compiled", 6) == 0))
    {
	char_u	    *s = skipwhite(*arg + varlen + 1);
	typval_T    argvars[2];
	int	    is_has = **arg == 'h';

	argvars[0].v_type = VAR_UNKNOWN;
	if (*s == '"')
	    (void)eval_string(&s, &argvars[0], TRUE);
	else if (*s == '\'')
	    (void)eval_lit_string(&s, &argvars[0], TRUE);
	s = skipwhite(s);
	if (*s == ')' && argvars[0].v_type == VAR_STRING
	       && ((is_has && !dynamic_feature(argvars[0].vval.v_string))
		    || !is_has))
	{
	    typval_T	*tv = &ppconst->pp_tv[ppconst->pp_used];

	    *arg = s + 1;
	    argvars[1].v_type = VAR_UNKNOWN;
	    tv->v_type = VAR_NUMBER;
	    tv->vval.v_number = 0;
	    if (is_has)
		f_has(argvars, tv);
	    else
		f_exists(argvars, tv);
	    clear_tv(&argvars[0]);
	    ++ppconst->pp_used;
	    return OK;
	}
	clear_tv(&argvars[0]);
	if (!is_has)
	{
	    emsg(_(e_argument_of_exists_compiled_must_be_literal_string));
	    return FAIL;
	}
    }

    if (generate_ppconst(cctx, ppconst) == FAIL)
	return FAIL;

    if (varlen >= sizeof(namebuf))
    {
	semsg(_(e_name_too_long_str), name);
	return FAIL;
    }
    vim_strncpy(namebuf, *arg, varlen);
    name = fname_trans_sid(namebuf, fname_buf, &tofree, &error);

    // We handle the "skip" argument of searchpair() and searchpairpos()
    // differently.
    is_searchpair = (varlen == 6 && STRNCMP(*arg, "search", 6) == 0)
	         || (varlen == 9 && STRNCMP(*arg, "searchpos", 9) == 0)
	        || (varlen == 10 && STRNCMP(*arg, "searchpair", 10) == 0)
	        || (varlen == 13 && STRNCMP(*arg, "searchpairpos", 13) == 0);

    *arg = skipwhite(*arg + varlen + 1);
    if (compile_arguments(arg, cctx, &argcount, is_searchpair) == FAIL)
	goto theend;

    is_autoload = vim_strchr(name, AUTOLOAD_CHAR) != NULL;
    if (ASCII_ISLOWER(*name) && name[1] != ':' && !is_autoload)
    {
	int	    idx;

	// builtin function
	idx = find_internal_func(name);
	if (idx >= 0)
	{
	    if (STRCMP(name, "flatten") == 0)
	    {
		emsg(_(e_cannot_use_flatten_in_vim9_script));
		goto theend;
	    }

	    if (STRCMP(name, "add") == 0 && argcount == 2)
	    {
		type_T	    *type = get_type_on_stack(cctx, 1);

		// add() can be compiled to instructions if we know the type
		if (type->tt_type == VAR_LIST)
		{
		    // inline "add(list, item)" so that the type can be checked
		    res = generate_LISTAPPEND(cctx);
		    idx = -1;
		}
		else if (type->tt_type == VAR_BLOB)
		{
		    // inline "add(blob, nr)" so that the type can be checked
		    res = generate_BLOBAPPEND(cctx);
		    idx = -1;
		}
	    }

	    if (idx >= 0)
		res = generate_BCALL(cctx, idx, argcount, argcount_init == 1);
	}
	else
	    semsg(_(e_unknown_function_str), namebuf);
	goto theend;
    }

    // An argument or local variable can be a function reference, this
    // overrules a function name.
    if (lookup_local(namebuf, varlen, NULL, cctx) == FAIL
	    && arg_exists(namebuf, varlen, NULL, NULL, NULL, cctx) != OK)
    {
	// If we can find the function by name generate the right call.
	// Skip global functions here, a local funcref takes precedence.
	ufunc = find_func(name, FALSE, cctx);
	if (ufunc != NULL && !func_is_global(ufunc))
	{
	    res = generate_CALL(cctx, ufunc, argcount);
	    goto theend;
	}
    }

    // If the name is a variable, load it and use PCALL.
    // Not for g:Func(), we don't know if it is a variable or not.
    // Not for eome#Func(), it will be loaded later.
    p = namebuf;
    if (STRNCMP(namebuf, "g:", 2) != 0 && !is_autoload
	    && compile_load(&p, namebuf + varlen, cctx, FALSE, FALSE) == OK)
    {
	type_T	    *type = get_type_on_stack(cctx, 0);

	res = generate_PCALL(cctx, argcount, namebuf, type, FALSE);
	goto theend;
    }

    // If we can find a global function by name generate the right call.
    if (ufunc != NULL)
    {
	res = generate_CALL(cctx, ufunc, argcount);
	goto theend;
    }

    // A global function may be defined only later.  Need to figure out at
    // runtime.  Also handles a FuncRef at runtime.
    if (STRNCMP(namebuf, "g:", 2) == 0 || is_autoload)
	res = generate_UCALL(cctx, name, argcount);
    else
	semsg(_(e_unknown_function_str), namebuf);

theend:
    vim_free(tofree);
    return res;
}

// like NAMESPACE_CHAR but with 'a' and 'l'.
#define VIM9_NAMESPACE_CHAR	(char_u *)"bgstvw"

/*
 * Find the end of a variable or function name.  Unlike find_name_end() this
 * does not recognize magic braces.
 * When "use_namespace" is TRUE recognize "b:", "s:", etc.
 * Return a pointer to just after the name.  Equal to "arg" if there is no
 * valid name.
 */
    char_u *
to_name_end(char_u *arg, int use_namespace)
{
    char_u	*p;

    // Quick check for valid starting character.
    if (!eval_isnamec1(*arg))
	return arg;

    for (p = arg + 1; *p != NUL && eval_isnamec(*p); MB_PTR_ADV(p))
	// Include a namespace such as "s:var" and "v:var".  But "n:" is not
	// and can be used in slice "[n:]".
	if (*p == ':' && (p != arg + 1
			     || !use_namespace
			     || vim_strchr(VIM9_NAMESPACE_CHAR, *arg) == NULL))
	    break;
    return p;
}

/*
 * Like to_name_end() but also skip over a list or dict constant.
 * Also accept "<SNR>123_Func".
 * This intentionally does not handle line continuation.
 */
    char_u *
to_name_const_end(char_u *arg)
{
    char_u	*p = arg;
    typval_T	rettv;

    if (STRNCMP(p, "<SNR>", 5) == 0)
	p = skipdigits(p + 5);
    p = to_name_end(p, TRUE);
    if (p == arg && *arg == '[')
    {

	// Can be "[1, 2, 3]->Func()".
	if (eval_list(&p, &rettv, NULL, FALSE) == FAIL)
	    p = arg;
    }
    return p;
}

/*
 * parse a list: [expr, expr]
 * "*arg" points to the '['.
 * ppconst->pp_is_const is set if all items are a constant.
 */
    static int
compile_list(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    char_u	*p = skipwhite(*arg + 1);
    char_u	*whitep = *arg + 1;
    int		count = 0;
    int		is_const;
    int		is_all_const = TRUE;	// reset when non-const encountered

    for (;;)
    {
	if (may_get_next_line(whitep, &p, cctx) == FAIL)
	{
	    semsg(_(e_missing_end_of_list_rsb_str), *arg);
	    return FAIL;
	}
	if (*p == ',')
	{
	    semsg(_(e_no_white_space_allowed_before_str_str), ",", p);
	    return FAIL;
	}
	if (*p == ']')
	{
	    ++p;
	    break;
	}
	if (compile_expr0_ext(&p, cctx, &is_const) == FAIL)
	    return FAIL;
	if (!is_const)
	    is_all_const = FALSE;
	++count;
	if (*p == ',')
	{
	    ++p;
	    if (*p != ']' && !IS_WHITE_OR_NUL(*p))
	    {
		semsg(_(e_white_space_required_after_str_str), ",", p - 1);
		return FAIL;
	    }
	}
	whitep = p;
	p = skipwhite(p);
    }
    *arg = p;

    ppconst->pp_is_const = is_all_const;
    return generate_NEWLIST(cctx, count);
}

/*
 * Parse a lambda: "(arg, arg) => expr"
 * "*arg" points to the '('.
 * Returns OK/FAIL when a lambda is recognized, NOTDONE if it's not a lambda.
 */
    static int
compile_lambda(char_u **arg, cctx_T *cctx)
{
    int		r;
    typval_T	rettv;
    ufunc_T	*ufunc;
    evalarg_T	evalarg;

    init_evalarg(&evalarg);
    evalarg.eval_flags = EVAL_EVALUATE;
    evalarg.eval_cctx = cctx;

    // Get the funcref in "rettv".
    r = get_lambda_tv(arg, &rettv, TRUE, &evalarg);
    if (r != OK)
    {
	clear_evalarg(&evalarg, NULL);
	return r;
    }

    // "rettv" will now be a partial referencing the function.
    ufunc = rettv.vval.v_partial->pt_func;
    ++ufunc->uf_refcount;
    clear_tv(&rettv);

    // Compile it here to get the return type.  The return type is optional,
    // when it's missing use t_unknown.  This is recognized in
    // compile_return().
    if (ufunc->uf_ret_type->tt_type == VAR_VOID)
	ufunc->uf_ret_type = &t_unknown;
    compile_def_function(ufunc, FALSE, cctx->ctx_compile_type, cctx);

    // When the outer function is compiled for profiling or debugging, the
    // lambda may be called without profiling or debugging.  Compile it here in
    // the right context.
    if (cctx->ctx_compile_type == CT_DEBUG
#ifdef FEAT_PROFILE
	    || cctx->ctx_compile_type == CT_PROFILE
#endif
       )
	compile_def_function(ufunc, FALSE, CT_NONE, cctx);

    // The last entry in evalarg.eval_tofree_ga is a copy of the last line and
    // "*arg" may point into it.  Point into the original line to avoid a
    // dangling pointer.
    if (evalarg.eval_using_cmdline)
    {
	garray_T    *gap = &evalarg.eval_tofree_ga;
	size_t	    off = *arg - ((char_u **)gap->ga_data)[gap->ga_len - 1];

	*arg = ((char_u **)cctx->ctx_ufunc->uf_lines.ga_data)[cctx->ctx_lnum]
									 + off;
    }

    clear_evalarg(&evalarg, NULL);

    if (ufunc->uf_def_status == UF_COMPILED)
    {
	// The return type will now be known.
	set_function_type(ufunc);

	// The function reference count will be 1.  When the ISN_FUNCREF
	// instruction is deleted the reference count is decremented and the
	// function is freed.
	return generate_FUNCREF(cctx, ufunc);
    }

    func_ptr_unref(ufunc);
    return FAIL;
}

/*
 * Get a lambda and compile it.  Uses Vim9 syntax.
 */
    int
get_lambda_tv_and_compile(
	char_u	    **arg,
	typval_T    *rettv,
	int	    types_optional,
	evalarg_T   *evalarg)
{
    int		r;
    ufunc_T	*ufunc;
    int		save_sc_version = current_sctx.sc_version;

    // Get the funcref in "rettv".
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;
    r = get_lambda_tv(arg, rettv, types_optional, evalarg);
    current_sctx.sc_version = save_sc_version;
    if (r != OK)
	return r;

    // "rettv" will now be a partial referencing the function.
    ufunc = rettv->vval.v_partial->pt_func;

    // Compile it here to get the return type.  The return type is optional,
    // when it's missing use t_unknown.  This is recognized in
    // compile_return().
    if (ufunc->uf_ret_type == NULL || ufunc->uf_ret_type->tt_type == VAR_VOID)
	ufunc->uf_ret_type = &t_unknown;
    compile_def_function(ufunc, FALSE, CT_NONE, NULL);

    if (ufunc->uf_def_status == UF_COMPILED)
    {
	// The return type will now be known.
	set_function_type(ufunc);
	return OK;
    }
    clear_tv(rettv);
    return FAIL;
}

/*
 * parse a dict: {key: val, [key]: val}
 * "*arg" points to the '{'.
 * ppconst->pp_is_const is set if all item values are a constant.
 */
    static int
compile_dict(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    garray_T	*instr = &cctx->ctx_instr;
    int		count = 0;
    dict_T	*d = dict_alloc();
    dictitem_T	*item;
    char_u	*whitep = *arg + 1;
    char_u	*p;
    int		is_const;
    int		is_all_const = TRUE;	// reset when non-const encountered

    if (d == NULL)
	return FAIL;
    if (generate_ppconst(cctx, ppconst) == FAIL)
	return FAIL;
    for (;;)
    {
	char_u	    *key = NULL;

	if (may_get_next_line(whitep, arg, cctx) == FAIL)
	{
	    *arg = NULL;
	    goto failret;
	}

	if (**arg == '}')
	    break;

	if (**arg == '[')
	{
	    isn_T	*isn;

	    // {[expr]: value} uses an evaluated key.
	    *arg = skipwhite(*arg + 1);
	    if (compile_expr0(arg, cctx) == FAIL)
		return FAIL;
	    isn = ((isn_T *)instr->ga_data) + instr->ga_len - 1;
	    if (isn->isn_type == ISN_PUSHNR)
	    {
		char buf[NUMBUFLEN];

		// Convert to string at compile time.
		vim_snprintf(buf, NUMBUFLEN, "%lld", isn->isn_arg.number);
		isn->isn_type = ISN_PUSHS;
		isn->isn_arg.string = vim_strsave((char_u *)buf);
	    }
	    if (isn->isn_type == ISN_PUSHS)
		key = isn->isn_arg.string;
	    else if (may_generate_2STRING(-1, FALSE, cctx) == FAIL)
		return FAIL;
	    *arg = skipwhite(*arg);
	    if (**arg != ']')
	    {
		emsg(_(e_missing_matching_bracket_after_dict_key));
		return FAIL;
	    }
	    ++*arg;
	}
	else
	{
	    // {"name": value},
	    // {'name': value},
	    // {name: value} use "name" as a literal key
	    key = get_literal_key(arg);
	    if (key == NULL)
		return FAIL;
	    if (generate_PUSHS(cctx, &key) == FAIL)
		return FAIL;
	}

	// Check for duplicate keys, if using string keys.
	if (key != NULL)
	{
	    item = dict_find(d, key, -1);
	    if (item != NULL)
	    {
		semsg(_(e_duplicate_key_in_dicitonary), key);
		goto failret;
	    }
	    item = dictitem_alloc(key);
	    if (item != NULL)
	    {
		item->di_tv.v_type = VAR_UNKNOWN;
		item->di_tv.v_lock = 0;
		if (dict_add(d, item) == FAIL)
		    dictitem_free(item);
	    }
	}

	if (**arg != ':')
	{
	    if (*skipwhite(*arg) == ':')
		semsg(_(e_no_white_space_allowed_before_str_str), ":", *arg);
	    else
		semsg(_(e_missing_colon_in_dictionary), *arg);
	    return FAIL;
	}
	whitep = *arg + 1;
	if (!IS_WHITE_OR_NUL(*whitep))
	{
	    semsg(_(e_white_space_required_after_str_str), ":", *arg);
	    return FAIL;
	}

	if (may_get_next_line(whitep, arg, cctx) == FAIL)
	{
	    *arg = NULL;
	    goto failret;
	}

	if (compile_expr0_ext(arg, cctx, &is_const) == FAIL)
	    return FAIL;
	if (!is_const)
	    is_all_const = FALSE;
	++count;

	whitep = *arg;
	if (may_get_next_line(whitep, arg, cctx) == FAIL)
	{
	    *arg = NULL;
	    goto failret;
	}
	if (**arg == '}')
	    break;
	if (**arg != ',')
	{
	    semsg(_(e_missing_comma_in_dictionary), *arg);
	    goto failret;
	}
	if (IS_WHITE_OR_NUL(*whitep))
	{
	    semsg(_(e_no_white_space_allowed_before_str_str), ",", whitep);
	    return FAIL;
	}
	whitep = *arg + 1;
	if (!IS_WHITE_OR_NUL(*whitep))
	{
	    semsg(_(e_white_space_required_after_str_str), ",", *arg);
	    return FAIL;
	}
	*arg = skipwhite(whitep);
    }

    *arg = *arg + 1;

    // Allow for following comment, after at least one space.
    p = skipwhite(*arg);
    if (VIM_ISWHITE(**arg) && vim9_comment_start(p))
	*arg += STRLEN(*arg);

    dict_unref(d);
    ppconst->pp_is_const = is_all_const;
    return generate_NEWDICT(cctx, count);

failret:
    if (*arg == NULL)
    {
	semsg(_(e_missing_dict_end), _("[end of lines]"));
	*arg = (char_u *)"";
    }
    dict_unref(d);
    return FAIL;
}

/*
 * Compile "&option".
 */
    static int
compile_get_option(char_u **arg, cctx_T *cctx)
{
    typval_T	rettv;
    char_u	*start = *arg;
    int		ret;

    // parse the option and get the current value to get the type.
    rettv.v_type = VAR_UNKNOWN;
    ret = eval_option(arg, &rettv, TRUE);
    if (ret == OK)
    {
	// include the '&' in the name, eval_option() expects it.
	char_u	*name = vim_strnsave(start, *arg - start);
	type_T	*type = rettv.v_type == VAR_BOOL ? &t_bool
			  : rettv.v_type == VAR_NUMBER ? &t_number : &t_string;

	ret = generate_LOAD(cctx, ISN_LOADOPT, 0, name, type);
	vim_free(name);
    }
    clear_tv(&rettv);

    return ret;
}

/*
 * Compile "$VAR".
 */
    static int
compile_get_env(char_u **arg, cctx_T *cctx)
{
    char_u	*start = *arg;
    int		len;
    int		ret;
    char_u	*name;

    ++*arg;
    len = get_env_len(arg);
    if (len == 0)
    {
	semsg(_(e_syntax_error_at_str), start);
	return FAIL;
    }

    // include the '$' in the name, eval_env_var() expects it.
    name = vim_strnsave(start, len + 1);
    ret = generate_LOAD(cctx, ISN_LOADENV, 0, name, &t_string);
    vim_free(name);
    return ret;
}

/*
 * Compile "@r".
 */
    static int
compile_get_register(char_u **arg, cctx_T *cctx)
{
    int		ret;

    ++*arg;
    if (**arg == NUL)
    {
	semsg(_(e_syntax_error_at_str), *arg - 1);
	return FAIL;
    }
    if (!valid_yank_reg(**arg, FALSE))
    {
	emsg_invreg(**arg);
	return FAIL;
    }
    ret = generate_LOAD(cctx, ISN_LOADREG, **arg, NULL, &t_string);
    ++*arg;
    return ret;
}

/*
 * Apply leading '!', '-' and '+' to constant "rettv".
 * When "numeric_only" is TRUE do not apply '!'.
 */
    static int
apply_leader(typval_T *rettv, int numeric_only, char_u *start, char_u **end)
{
    char_u *p = *end;

    // this works from end to start
    while (p > start)
    {
	--p;
	if (*p == '-' || *p == '+')
	{
	    // only '-' has an effect, for '+' we only check the type
#ifdef FEAT_FLOAT
	    if (rettv->v_type == VAR_FLOAT)
	    {
		if (*p == '-')
		    rettv->vval.v_float = -rettv->vval.v_float;
	    }
	    else
#endif
	    {
		varnumber_T	val;
		int		error = FALSE;

		// tv_get_number_chk() accepts a string, but we don't want that
		// here
		if (check_not_string(rettv) == FAIL)
		    return FAIL;
		val = tv_get_number_chk(rettv, &error);
		clear_tv(rettv);
		if (error)
		    return FAIL;
		if (*p == '-')
		    val = -val;
		rettv->v_type = VAR_NUMBER;
		rettv->vval.v_number = val;
	    }
	}
	else if (numeric_only)
	{
	    ++p;
	    break;
	}
	else if (*p == '!')
	{
	    int v = tv2bool(rettv);

	    // '!' is permissive in the type.
	    clear_tv(rettv);
	    rettv->v_type = VAR_BOOL;
	    rettv->vval.v_number = v ? VVAL_FALSE : VVAL_TRUE;
	}
    }
    *end = p;
    return OK;
}

/*
 * Recognize v: variables that are constants and set "rettv".
 */
    static void
get_vim_constant(char_u **arg, typval_T *rettv)
{
    if (STRNCMP(*arg, "v:true", 6) == 0)
    {
	rettv->v_type = VAR_BOOL;
	rettv->vval.v_number = VVAL_TRUE;
	*arg += 6;
    }
    else if (STRNCMP(*arg, "v:false", 7) == 0)
    {
	rettv->v_type = VAR_BOOL;
	rettv->vval.v_number = VVAL_FALSE;
	*arg += 7;
    }
    else if (STRNCMP(*arg, "v:null", 6) == 0)
    {
	rettv->v_type = VAR_SPECIAL;
	rettv->vval.v_number = VVAL_NULL;
	*arg += 6;
    }
    else if (STRNCMP(*arg, "v:none", 6) == 0)
    {
	rettv->v_type = VAR_SPECIAL;
	rettv->vval.v_number = VVAL_NONE;
	*arg += 6;
    }
}

    exprtype_T
get_compare_type(char_u *p, int *len, int *type_is)
{
    exprtype_T	type = EXPR_UNKNOWN;
    int		i;

    switch (p[0])
    {
	case '=':   if (p[1] == '=')
			type = EXPR_EQUAL;
		    else if (p[1] == '~')
			type = EXPR_MATCH;
		    break;
	case '!':   if (p[1] == '=')
			type = EXPR_NEQUAL;
		    else if (p[1] == '~')
			type = EXPR_NOMATCH;
		    break;
	case '>':   if (p[1] != '=')
		    {
			type = EXPR_GREATER;
			*len = 1;
		    }
		    else
			type = EXPR_GEQUAL;
		    break;
	case '<':   if (p[1] != '=')
		    {
			type = EXPR_SMALLER;
			*len = 1;
		    }
		    else
			type = EXPR_SEQUAL;
		    break;
	case 'i':   if (p[1] == 's')
		    {
			// "is" and "isnot"; but not a prefix of a name
			if (p[2] == 'n' && p[3] == 'o' && p[4] == 't')
			    *len = 5;
			i = p[*len];
			if (!isalnum(i) && i != '_')
			{
			    type = *len == 2 ? EXPR_IS : EXPR_ISNOT;
			    *type_is = TRUE;
			}
		    }
		    break;
    }
    return type;
}

/*
 * Skip over an expression, ignoring most errors.
 */
    void
skip_expr_cctx(char_u **arg, cctx_T *cctx)
{
    evalarg_T	evalarg;

    init_evalarg(&evalarg);
    evalarg.eval_cctx = cctx;
    skip_expr(arg, &evalarg);
    clear_evalarg(&evalarg, NULL);
}

/*
 * Check that the top of the type stack has a type that can be used as a
 * condition.  Give an error and return FAIL if not.
 */
    int
bool_on_stack(cctx_T *cctx)
{
    type_T	*type;

    type = get_type_on_stack(cctx, 0);
    if (type == &t_bool)
	return OK;

    if (type == &t_any
	    || type == &t_unknown
	    || type == &t_number
	    || type == &t_number_bool)
	// Number 0 and 1 are OK to use as a bool.  "any" could also be a bool.
	// This requires a runtime type check.
	return generate_COND2BOOL(cctx);

    return need_type(type, &t_bool, -1, 0, cctx, FALSE, FALSE);
}

/*
 * Give the "white on both sides" error, taking the operator from "p[len]".
 */
    void
error_white_both(char_u *op, int len)
{
    char_u	buf[10];

    vim_strncpy(buf, op, len);
    semsg(_(e_white_space_required_before_and_after_str_at_str), buf, op);
}

/*
 * Compile code to apply '-', '+' and '!'.
 * When "numeric_only" is TRUE do not apply '!'.
 */
    static int
compile_leader(cctx_T *cctx, int numeric_only, char_u *start, char_u **end)
{
    char_u	*p = *end;

    // this works from end to start
    while (p > start)
    {
	--p;
	while (VIM_ISWHITE(*p))
	    --p;
	if (*p == '-' || *p == '+')
	{
	    int		negate = *p == '-';
	    isn_T	*isn;
	    type_T	*type;

	    type = get_type_on_stack(cctx, 0);
	    if (type != &t_float && need_type(type, &t_number,
					    -1, 0, cctx, FALSE, FALSE) == FAIL)
		return FAIL;

	    while (p > start && (p[-1] == '-' || p[-1] == '+'))
	    {
		--p;
		if (*p == '-')
		    negate = !negate;
	    }
	    // only '-' has an effect, for '+' we only check the type
	    if (negate)
	    {
		isn = generate_instr(cctx, ISN_NEGATENR);
		if (isn == NULL)
		    return FAIL;
	    }
	}
	else if (numeric_only)
	{
	    ++p;
	    break;
	}
	else
	{
	    int  invert = *p == '!';

	    while (p > start && (p[-1] == '!' || VIM_ISWHITE(p[-1])))
	    {
		if (p[-1] == '!')
		    invert = !invert;
		--p;
	    }
	    if (generate_2BOOL(cctx, invert, -1) == FAIL)
		return FAIL;
	}
    }
    *end = p;
    return OK;
}

/*
 * Compile "(expression)": recursive!
 * Return FAIL/OK.
 */
    static int
compile_parenthesis(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    int	    ret;
    char_u  *p = *arg + 1;

    if (may_get_next_line_error(p, arg, cctx) == FAIL)
	return FAIL;
    if (ppconst->pp_used <= PPSIZE - 10)
    {
	ret = compile_expr1(arg, cctx, ppconst);
    }
    else
    {
	// Not enough space in ppconst, flush constants.
	if (generate_ppconst(cctx, ppconst) == FAIL)
	    return FAIL;
	ret = compile_expr0(arg, cctx);
    }
    if (may_get_next_line_error(*arg, arg, cctx) == FAIL)
	return FAIL;
    if (**arg == ')')
	++*arg;
    else if (ret == OK)
    {
	emsg(_(e_missing_closing_paren));
	ret = FAIL;
    }
    return ret;
}

/*
 * Compile whatever comes after "name" or "name()".
 * Advances "*arg" only when something was recognized.
 */
    static int
compile_subscript(
	char_u **arg,
	cctx_T *cctx,
	char_u *start_leader,
	char_u **end_leader,
	ppconst_T *ppconst)
{
    char_u	*name_start = *end_leader;
    int		keeping_dict = FALSE;

    for (;;)
    {
	char_u *p = skipwhite(*arg);

	if (*p == NUL || (VIM_ISWHITE(**arg) && vim9_comment_start(p)))
	{
	    char_u *next = peek_next_line_from_context(cctx);

	    // If a following line starts with "->{" or "->X" advance to that
	    // line, so that a line break before "->" is allowed.
	    // Also if a following line starts with ".x".
	    if (next != NULL &&
		    ((next[0] == '-' && next[1] == '>'
				 && (next[2] == '{'
				       || ASCII_ISALPHA(*skipwhite(next + 2))))
		    || (next[0] == '.' && eval_isdictc(next[1]))))
	    {
		next = next_line_from_context(cctx, TRUE);
		if (next == NULL)
		    return FAIL;
		*arg = next;
		p = skipwhite(*arg);
	    }
	}

	// Do not skip over white space to find the "(", "execute 'x' (expr)"
	// is not a function call.
	if (**arg == '(')
	{
	    type_T	*type;
	    int		argcount = 0;

	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;
	    ppconst->pp_is_const = FALSE;

	    // funcref(arg)
	    type = get_type_on_stack(cctx, 0);

	    *arg = skipwhite(p + 1);
	    if (compile_arguments(arg, cctx, &argcount, FALSE) == FAIL)
		return FAIL;
	    if (generate_PCALL(cctx, argcount, name_start, type, TRUE) == FAIL)
		return FAIL;
	    if (keeping_dict)
	    {
		keeping_dict = FALSE;
		if (generate_instr(cctx, ISN_CLEARDICT) == NULL)
		    return FAIL;
	    }
	}
	else if (*p == '-' && p[1] == '>')
	{
	    char_u *pstart = p;

	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;
	    ppconst->pp_is_const = FALSE;

	    // something->method()
	    // Apply the '!', '-' and '+' first:
	    //   -1.0->func() works like (-1.0)->func()
	    if (compile_leader(cctx, TRUE, start_leader, end_leader) == FAIL)
		return FAIL;

	    p += 2;
	    *arg = skipwhite(p);
	    // No line break supported right after "->".
	    if (**arg == '(')
	    {
		int	    argcount = 1;
		garray_T    *stack = &cctx->ctx_type_stack;
		int	    type_idx_start = stack->ga_len;
		type_T	    *type;
		int	    expr_isn_start = cctx->ctx_instr.ga_len;
		int	    expr_isn_end;
		int	    arg_isn_count;

		// Funcref call:  list->(Refs[2])(arg)
		// or lambda:	  list->((arg) => expr)(arg)
		//
		// Fist compile the function expression.
		if (compile_parenthesis(arg, cctx, ppconst) == FAIL)
		    return FAIL;

		// Remember the next instruction index, where the instructions
		// for arguments are being written.
		expr_isn_end = cctx->ctx_instr.ga_len;

		// Compile the arguments.
		if (**arg != '(')
		{
		    if (*skipwhite(*arg) == '(')
			emsg(_(e_no_white_space_allowed_before_parenthesis));
		    else
			semsg(_(e_missing_parenthesis_str), *arg);
		    return FAIL;
		}
		*arg = skipwhite(*arg + 1);
		if (compile_arguments(arg, cctx, &argcount, FALSE) == FAIL)
		    return FAIL;

		// Move the instructions for the arguments to before the
		// instructions of the expression and move the type of the
		// expression after the argument types.  This is what ISN_PCALL
		// expects.
		arg_isn_count = cctx->ctx_instr.ga_len - expr_isn_end;
		if (arg_isn_count > 0)
		{
		    int	    expr_isn_count = expr_isn_end - expr_isn_start;
		    isn_T   *isn = ALLOC_MULT(isn_T, expr_isn_count);
		    type_T  *decl_type;
		    type2_T  *typep;

		    if (isn == NULL)
			return FAIL;
		    mch_memmove(isn, ((isn_T *)cctx->ctx_instr.ga_data)
							      + expr_isn_start,
					       sizeof(isn_T) * expr_isn_count);
		    mch_memmove(((isn_T *)cctx->ctx_instr.ga_data)
							      + expr_isn_start,
			     ((isn_T *)cctx->ctx_instr.ga_data) + expr_isn_end,
						sizeof(isn_T) * arg_isn_count);
		    mch_memmove(((isn_T *)cctx->ctx_instr.ga_data)
					      + expr_isn_start + arg_isn_count,
					  isn, sizeof(isn_T) * expr_isn_count);
		    vim_free(isn);

		    typep = ((type2_T *)stack->ga_data) + type_idx_start;
		    type = typep->type_curr;
		    decl_type = typep->type_decl;
		    mch_memmove(((type2_T *)stack->ga_data) + type_idx_start,
			      ((type2_T *)stack->ga_data) + type_idx_start + 1,
			      sizeof(type2_T)
				       * (stack->ga_len - type_idx_start - 1));
		    typep = ((type2_T *)stack->ga_data) + stack->ga_len - 1;
		    typep->type_curr = type;
		    typep->type_decl = decl_type;
		}

		type = get_type_on_stack(cctx, 0);
		if (generate_PCALL(cctx, argcount, p - 2, type, FALSE) == FAIL)
		    return FAIL;
	    }
	    else
	    {
		// method call:  list->method()
		p = *arg;
		if (!eval_isnamec1(*p))
		{
		    semsg(_(e_trailing_characters_str), pstart);
		    return FAIL;
		}
		if (ASCII_ISALPHA(*p) && p[1] == ':')
		    p += 2;
		for ( ; eval_isnamec(*p); ++p)
		    ;
		if (*p != '(')
		{
		    semsg(_(e_missing_parenthesis_str), *arg);
		    return FAIL;
		}
		if (compile_call(arg, p - *arg, cctx, ppconst, 1) == FAIL)
		    return FAIL;
	    }
	    if (keeping_dict)
	    {
		keeping_dict = FALSE;
		if (generate_instr(cctx, ISN_CLEARDICT) == NULL)
		    return FAIL;
	    }
	}
	else if (**arg == '[')
	{
	    int		is_slice = FALSE;

	    // list index: list[123]
	    // dict member: dict[key]
	    // string index: text[123]
	    // blob index: blob[123]
	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;
	    ppconst->pp_is_const = FALSE;

	    ++p;
	    if (may_get_next_line_error(p, arg, cctx) == FAIL)
		return FAIL;
	    if (**arg == ':')
	    {
		// missing first index is equal to zero
		generate_PUSHNR(cctx, 0);
	    }
	    else
	    {
		if (compile_expr0(arg, cctx) == FAIL)
		    return FAIL;
		if (**arg == ':')
		{
		    semsg(_(e_white_space_required_before_and_after_str_at_str),
								    ":", *arg);
		    return FAIL;
		}
		if (may_get_next_line_error(*arg, arg, cctx) == FAIL)
		    return FAIL;
		*arg = skipwhite(*arg);
	    }
	    if (**arg == ':')
	    {
		is_slice = TRUE;
		++*arg;
		if (!IS_WHITE_OR_NUL(**arg) && **arg != ']')
		{
		    semsg(_(e_white_space_required_before_and_after_str_at_str),
								    ":", *arg);
		    return FAIL;
		}
		if (may_get_next_line_error(*arg, arg, cctx) == FAIL)
		    return FAIL;
		if (**arg == ']')
		    // missing second index is equal to end of string
		    generate_PUSHNR(cctx, -1);
		else
		{
		    if (compile_expr0(arg, cctx) == FAIL)
			return FAIL;
		    if (may_get_next_line_error(*arg, arg, cctx) == FAIL)
			return FAIL;
		    *arg = skipwhite(*arg);
		}
	    }

	    if (**arg != ']')
	    {
		emsg(_(e_missing_closing_square_brace));
		return FAIL;
	    }
	    *arg = *arg + 1;

	    if (keeping_dict)
	    {
		keeping_dict = FALSE;
		if (generate_instr(cctx, ISN_CLEARDICT) == NULL)
		    return FAIL;
	    }
	    if (compile_member(is_slice, &keeping_dict, cctx) == FAIL)
		return FAIL;
	}
	else if (*p == '.' && p[1] != '.')
	{
	    // dictionary member: dict.name
	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;
	    ppconst->pp_is_const = FALSE;

	    *arg = p + 1;
	    if (IS_WHITE_OR_NUL(**arg))
	    {
		emsg(_(e_missing_name_after_dot));
		return FAIL;
	    }
	    p = *arg;
	    if (eval_isdictc(*p))
		while (eval_isnamec(*p))
		    MB_PTR_ADV(p);
	    if (p == *arg)
	    {
		semsg(_(e_syntax_error_at_str), *arg);
		return FAIL;
	    }
	    if (keeping_dict && generate_instr(cctx, ISN_CLEARDICT) == NULL)
		return FAIL;
	    if (generate_STRINGMEMBER(cctx, *arg, p - *arg) == FAIL)
		return FAIL;
	    keeping_dict = TRUE;
	    *arg = p;
	}
	else
	    break;
    }

    // Turn "dict.Func" into a partial for "Func" bound to "dict".
    // This needs to be done at runtime to be able to check the type.
    if (keeping_dict && generate_instr(cctx, ISN_USEDICT) == NULL)
	return FAIL;

    return OK;
}

/*
 * Compile an expression at "*arg" and add instructions to "cctx->ctx_instr".
 * "arg" is advanced until after the expression, skipping white space.
 *
 * If the value is a constant "ppconst->pp_used" will be non-zero.
 * Before instructions are generated, any values in "ppconst" will generated.
 *
 * This is the compiling equivalent of eval1(), eval2(), etc.
 */

/*
 *  number		number constant
 *  0zFFFFFFFF		Blob constant
 *  "string"		string constant
 *  'string'		literal string constant
 *  &option-name	option value
 *  @r			register contents
 *  identifier		variable value
 *  function()		function call
 *  $VAR		environment variable
 *  (expression)	nested expression
 *  [expr, expr]	List
 *  {key: val, [key]: val}   Dictionary
 *
 *  Also handle:
 *  ! in front		logical NOT
 *  - in front		unary minus
 *  + in front		unary plus (ignored)
 *  trailing (arg)	funcref/partial call
 *  trailing []		subscript in String or List
 *  trailing .name	entry in Dictionary
 *  trailing ->name()	method call
 */
    static int
compile_expr8(
	char_u **arg,
	cctx_T *cctx,
	ppconst_T *ppconst)
{
    char_u	*start_leader, *end_leader;
    int		ret = OK;
    typval_T	*rettv = &ppconst->pp_tv[ppconst->pp_used];
    int		used_before = ppconst->pp_used;

    ppconst->pp_is_const = FALSE;

    /*
     * Skip '!', '-' and '+' characters.  They are handled later.
     */
    start_leader = *arg;
    if (eval_leader(arg, TRUE) == FAIL)
	return FAIL;
    end_leader = *arg;

    rettv->v_type = VAR_UNKNOWN;
    switch (**arg)
    {
	/*
	 * Number constant.
	 */
	case '0':	// also for blob starting with 0z
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '.':   if (eval_number(arg, rettv, TRUE, FALSE) == FAIL)
			return FAIL;
		    // Apply "-" and "+" just before the number now, right to
		    // left.  Matters especially when "->" follows.  Stops at
		    // '!'.
		    if (apply_leader(rettv, TRUE,
					    start_leader, &end_leader) == FAIL)
		    {
			clear_tv(rettv);
			return FAIL;
		    }
		    break;

	/*
	 * String constant: "string".
	 */
	case '"':   if (eval_string(arg, rettv, TRUE) == FAIL)
			return FAIL;
		    break;

	/*
	 * Literal string constant: 'str''ing'.
	 */
	case '\'':  if (eval_lit_string(arg, rettv, TRUE) == FAIL)
			return FAIL;
		    break;

	/*
	 * Constant Vim variable.
	 */
	case 'v':   get_vim_constant(arg, rettv);
		    ret = NOTDONE;
		    break;

	/*
	 * "true" constant
	 */
	case 't':   if (STRNCMP(*arg, "true", 4) == 0
						   && !eval_isnamec((*arg)[4]))
		    {
			*arg += 4;
			rettv->v_type = VAR_BOOL;
			rettv->vval.v_number = VVAL_TRUE;
		    }
		    else
			ret = NOTDONE;
		    break;

	/*
	 * "false" constant
	 */
	case 'f':   if (STRNCMP(*arg, "false", 5) == 0
						   && !eval_isnamec((*arg)[5]))
		    {
			*arg += 5;
			rettv->v_type = VAR_BOOL;
			rettv->vval.v_number = VVAL_FALSE;
		    }
		    else
			ret = NOTDONE;
		    break;

	/*
	 * "null" constant
	 */
	case 'n':   if (STRNCMP(*arg, "null", 4) == 0
						   && !eval_isnamec((*arg)[4]))
		    {
			*arg += 4;
			rettv->v_type = VAR_SPECIAL;
			rettv->vval.v_number = VVAL_NULL;
		    }
		    else
			ret = NOTDONE;
		    break;

	/*
	 * List: [expr, expr]
	 */
	case '[':   if (generate_ppconst(cctx, ppconst) == FAIL)
			return FAIL;
		    ret = compile_list(arg, cctx, ppconst);
		    break;

	/*
	 * Dictionary: {'key': val, 'key': val}
	 */
	case '{':   if (generate_ppconst(cctx, ppconst) == FAIL)
			return FAIL;
		    ret = compile_dict(arg, cctx, ppconst);
		    break;

	/*
	 * Option value: &name
	 */
	case '&':	if (generate_ppconst(cctx, ppconst) == FAIL)
			    return FAIL;
			ret = compile_get_option(arg, cctx);
			break;

	/*
	 * Environment variable: $VAR.
	 */
	case '$':	if (generate_ppconst(cctx, ppconst) == FAIL)
			    return FAIL;
			ret = compile_get_env(arg, cctx);
			break;

	/*
	 * Register contents: @r.
	 */
	case '@':	if (generate_ppconst(cctx, ppconst) == FAIL)
			    return FAIL;
			ret = compile_get_register(arg, cctx);
			break;
	/*
	 * nested expression: (expression).
	 * lambda: (arg, arg) => expr
	 * funcref: (arg, arg) => { statement }
	 */
	case '(':   // if compile_lambda returns NOTDONE then it must be (expr)
		    ret = compile_lambda(arg, cctx);
		    if (ret == NOTDONE)
			ret = compile_parenthesis(arg, cctx, ppconst);
		    break;

	default:    ret = NOTDONE;
		    break;
    }
    if (ret == FAIL)
	return FAIL;

    if (rettv->v_type != VAR_UNKNOWN && used_before == ppconst->pp_used)
    {
	if (cctx->ctx_skip == SKIP_YES)
	    clear_tv(rettv);
	else
	    // A constant expression can possibly be handled compile time,
	    // return the value instead of generating code.
	    ++ppconst->pp_used;
    }
    else if (ret == NOTDONE)
    {
	char_u	    *p;
	int	    r;

	if (!eval_isnamec1(**arg))
	{
	    if (!vim9_bad_comment(*arg))
	    {
		if (ends_excmd(*skipwhite(*arg)))
		    semsg(_(e_empty_expression_str), *arg);
		else
		    semsg(_(e_name_expected_str), *arg);
	    }
	    return FAIL;
	}

	// "name" or "name()"
	p = to_name_end(*arg, TRUE);
	if (p - *arg == (size_t)1 && **arg == '_')
	{
	    emsg(_(e_cannot_use_underscore_here));
	    return FAIL;
	}

	if (*p == '(')
	{
	    r = compile_call(arg, p - *arg, cctx, ppconst, 0);
	}
	else
	{
	    if (cctx->ctx_skip != SKIP_YES
				    && generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;
	    r = compile_load(arg, p, cctx, TRUE, TRUE);
	}
	if (r == FAIL)
	    return FAIL;
    }

    // Handle following "[]", ".member", etc.
    // Then deal with prefixed '-', '+' and '!', if not done already.
    if (compile_subscript(arg, cctx, start_leader, &end_leader,
							     ppconst) == FAIL)
	return FAIL;
    if (ppconst->pp_used > 0)
    {
	// apply the '!', '-' and '+' before the constant
	rettv = &ppconst->pp_tv[ppconst->pp_used - 1];
	if (apply_leader(rettv, FALSE, start_leader, &end_leader) == FAIL)
	    return FAIL;
	return OK;
    }
    if (compile_leader(cctx, FALSE, start_leader, &end_leader) == FAIL)
	return FAIL;
    return OK;
}

/*
 * <type>expr8: runtime type check / conversion
 */
    static int
compile_expr7(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    type_T *want_type = NULL;

    // Recognize <type>
    if (**arg == '<' && eval_isnamec1((*arg)[1]))
    {
	++*arg;
	want_type = parse_type(arg, cctx->ctx_type_list, TRUE);
	if (want_type == NULL)
	    return FAIL;

	if (**arg != '>')
	{
	    if (*skipwhite(*arg) == '>')
		semsg(_(e_no_white_space_allowed_before_str_str), ">", *arg);
	    else
		emsg(_(e_missing_gt));
	    return FAIL;
	}
	++*arg;
	if (may_get_next_line_error(*arg, arg, cctx) == FAIL)
	    return FAIL;
    }

    if (compile_expr8(arg, cctx, ppconst) == FAIL)
	return FAIL;

    if (want_type != NULL)
    {
	type_T	    *actual;
	where_T	    where = WHERE_INIT;

	generate_ppconst(cctx, ppconst);
	actual = get_type_on_stack(cctx, 0);
	if (check_type_maybe(want_type, actual, FALSE, where) != OK)
	{
	    if (need_type(actual, want_type, -1, 0, cctx, FALSE, FALSE)
								       == FAIL)
		return FAIL;
	}
    }

    return OK;
}

/*
 *	*	number multiplication
 *	/	number division
 *	%	number modulo
 */
    static int
compile_expr6(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    char_u	*op;
    char_u	*next;
    int		ppconst_used = ppconst->pp_used;

    // get the first expression
    if (compile_expr7(arg, cctx, ppconst) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no "*", "/" or "%" is following.
     */
    for (;;)
    {
	op = may_peek_next_line(cctx, *arg, &next);
	if (*op != '*' && *op != '/' && *op != '%')
	    break;
	if (next != NULL)
	{
	    *arg = next_line_from_context(cctx, TRUE);
	    op = skipwhite(*arg);
	}

	if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(op[1]))
	{
	    error_white_both(op, 1);
	    return FAIL;
	}
	if (may_get_next_line_error(op + 1, arg, cctx) == FAIL)
	    return FAIL;

	// get the second expression
	if (compile_expr7(arg, cctx, ppconst) == FAIL)
	    return FAIL;

	if (ppconst->pp_used == ppconst_used + 2
		&& ppconst->pp_tv[ppconst_used].v_type == VAR_NUMBER
		&& ppconst->pp_tv[ppconst_used + 1].v_type == VAR_NUMBER)
	{
	    typval_T	    *tv1 = &ppconst->pp_tv[ppconst_used];
	    typval_T	    *tv2 = &ppconst->pp_tv[ppconst_used + 1];
	    varnumber_T	    res = 0;
	    int		    failed = FALSE;

	    // both are numbers: compute the result
	    switch (*op)
	    {
		case '*': res = tv1->vval.v_number * tv2->vval.v_number;
			  break;
		case '/': res = num_divide(tv1->vval.v_number,
						  tv2->vval.v_number, &failed);
			  break;
		case '%': res = num_modulus(tv1->vval.v_number,
						  tv2->vval.v_number, &failed);
			  break;
	    }
	    if (failed)
		return FAIL;
	    tv1->vval.v_number = res;
	    --ppconst->pp_used;
	}
	else
	{
	    generate_ppconst(cctx, ppconst);
	    generate_two_op(cctx, op);
	}
    }

    return OK;
}

/*
 *      +	number addition or list/blobl concatenation
 *      -	number subtraction
 *      ..	string concatenation
 */
    static int
compile_expr5(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    char_u	*op;
    char_u	*next;
    int		oplen;
    int		ppconst_used = ppconst->pp_used;

    // get the first variable
    if (compile_expr6(arg, cctx, ppconst) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no "+", "-" or ".." is following.
     */
    for (;;)
    {
	op = may_peek_next_line(cctx, *arg, &next);
	if (*op != '+' && *op != '-' && !(*op == '.' && *(op + 1) == '.'))
	    break;
	if (op[0] == op[1] && *op != '.' && next)
	    // Finding "++" or "--" on the next line is a separate command.
	    // But ".." is concatenation.
	    break;
	oplen = (*op == '.' ? 2 : 1);
	if (next != NULL)
	{
	    *arg = next_line_from_context(cctx, TRUE);
	    op = skipwhite(*arg);
	}

	if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(op[oplen]))
	{
	    error_white_both(op, oplen);
	    return FAIL;
	}

	if (may_get_next_line_error(op + oplen, arg, cctx) == FAIL)
	    return FAIL;

	// get the second expression
	if (compile_expr6(arg, cctx, ppconst) == FAIL)
	    return FAIL;

	if (ppconst->pp_used == ppconst_used + 2
		&& (*op == '.'
		    ? (ppconst->pp_tv[ppconst_used].v_type == VAR_STRING
		    && ppconst->pp_tv[ppconst_used + 1].v_type == VAR_STRING)
		    : (ppconst->pp_tv[ppconst_used].v_type == VAR_NUMBER
		    && ppconst->pp_tv[ppconst_used + 1].v_type == VAR_NUMBER)))
	{
	    typval_T *tv1 = &ppconst->pp_tv[ppconst_used];
	    typval_T *tv2 = &ppconst->pp_tv[ppconst_used + 1];

	    // concat/subtract/add constant numbers
	    if (*op == '+')
		tv1->vval.v_number = tv1->vval.v_number + tv2->vval.v_number;
	    else if (*op == '-')
		tv1->vval.v_number = tv1->vval.v_number - tv2->vval.v_number;
	    else
	    {
		// concatenate constant strings
		char_u *s1 = tv1->vval.v_string;
		char_u *s2 = tv2->vval.v_string;
		size_t len1 = STRLEN(s1);

		tv1->vval.v_string = alloc((int)(len1 + STRLEN(s2) + 1));
		if (tv1->vval.v_string == NULL)
		{
		    clear_ppconst(ppconst);
		    return FAIL;
		}
		mch_memmove(tv1->vval.v_string, s1, len1);
		STRCPY(tv1->vval.v_string + len1, s2);
		vim_free(s1);
		vim_free(s2);
	    }
	    --ppconst->pp_used;
	}
	else
	{
	    generate_ppconst(cctx, ppconst);
	    ppconst->pp_is_const = FALSE;
	    if (*op == '.')
	    {
		if (may_generate_2STRING(-2, FALSE, cctx) == FAIL
			|| may_generate_2STRING(-1, FALSE, cctx) == FAIL)
		    return FAIL;
		generate_instr_drop(cctx, ISN_CONCAT, 1);
	    }
	    else
		generate_two_op(cctx, op);
	}
    }

    return OK;
}

/*
 * expr5a == expr5b
 * expr5a =~ expr5b
 * expr5a != expr5b
 * expr5a !~ expr5b
 * expr5a > expr5b
 * expr5a >= expr5b
 * expr5a < expr5b
 * expr5a <= expr5b
 * expr5a is expr5b
 * expr5a isnot expr5b
 *
 * Produces instructions:
 *	EVAL expr5a		Push result of "expr5a"
 *	EVAL expr5b		Push result of "expr5b"
 *	COMPARE			one of the compare instructions
 */
    static int
compile_expr4(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    exprtype_T	type = EXPR_UNKNOWN;
    char_u	*p;
    char_u	*next;
    int		len = 2;
    int		type_is = FALSE;
    int		ppconst_used = ppconst->pp_used;

    // get the first variable
    if (compile_expr5(arg, cctx, ppconst) == FAIL)
	return FAIL;

    p = may_peek_next_line(cctx, *arg, &next);
    type = get_compare_type(p, &len, &type_is);

    /*
     * If there is a comparative operator, use it.
     */
    if (type != EXPR_UNKNOWN)
    {
	int ic = FALSE;  // Default: do not ignore case

	if (next != NULL)
	{
	    *arg = next_line_from_context(cctx, TRUE);
	    p = skipwhite(*arg);
	}
	if (type_is && (p[len] == '?' || p[len] == '#'))
	{
	    semsg(_(e_invalid_expression_str), *arg);
	    return FAIL;
	}
	// extra question mark appended: ignore case
	if (p[len] == '?')
	{
	    ic = TRUE;
	    ++len;
	}
	// extra '#' appended: match case (ignored)
	else if (p[len] == '#')
	    ++len;
	// nothing appended: match case

	if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(p[len]))
	{
	    error_white_both(p, len);
	    return FAIL;
	}

	// get the second variable
	if (may_get_next_line_error(p + len, arg, cctx) == FAIL)
	    return FAIL;

	if (compile_expr5(arg, cctx, ppconst) == FAIL)
	    return FAIL;

	if (ppconst->pp_used == ppconst_used + 2)
	{
	    typval_T *	tv1 = &ppconst->pp_tv[ppconst->pp_used - 2];
	    typval_T	*tv2 = &ppconst->pp_tv[ppconst->pp_used - 1];
	    int		ret;

	    // Both sides are a constant, compute the result now.
	    // First check for a valid combination of types, this is more
	    // strict than typval_compare().
	    if (check_compare_types(type, tv1, tv2) == FAIL)
		ret = FAIL;
	    else
	    {
		ret = typval_compare(tv1, tv2, type, ic);
		tv1->v_type = VAR_BOOL;
		tv1->vval.v_number = tv1->vval.v_number
						      ? VVAL_TRUE : VVAL_FALSE;
		clear_tv(tv2);
		--ppconst->pp_used;
	    }
	    return ret;
	}

	generate_ppconst(cctx, ppconst);
	return generate_COMPARE(cctx, type, ic);
    }

    return OK;
}

static int compile_expr3(char_u **arg,  cctx_T *cctx, ppconst_T *ppconst);

/*
 * Compile || or &&.
 */
    static int
compile_and_or(
	char_u **arg,
	cctx_T	*cctx,
	char	*op,
	ppconst_T *ppconst,
	int	ppconst_used UNUSED)
{
    char_u	*next;
    char_u	*p = may_peek_next_line(cctx, *arg, &next);
    int		opchar = *op;

    if (p[0] == opchar && p[1] == opchar)
    {
	garray_T	*instr = &cctx->ctx_instr;
	garray_T	end_ga;
	int		save_skip = cctx->ctx_skip;

	/*
	 * Repeat until there is no following "||" or "&&"
	 */
	ga_init2(&end_ga, sizeof(int), 10);
	while (p[0] == opchar && p[1] == opchar)
	{
	    long	start_lnum = SOURCING_LNUM;
	    long	save_sourcing_lnum;
	    int		start_ctx_lnum = cctx->ctx_lnum;
	    int		save_lnum;
	    int		const_used;
	    int		status;
	    jumpwhen_T	jump_when = opchar == '|'
				      ? JUMP_IF_COND_TRUE : JUMP_IF_COND_FALSE;

	    if (next != NULL)
	    {
		*arg = next_line_from_context(cctx, TRUE);
		p = skipwhite(*arg);
	    }

	    if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(p[2]))
	    {
		semsg(_(e_white_space_required_before_and_after_str_at_str),
									op, p);
		ga_clear(&end_ga);
		return FAIL;
	    }

	    save_sourcing_lnum = SOURCING_LNUM;
	    SOURCING_LNUM = start_lnum;
	    save_lnum = cctx->ctx_lnum;
	    cctx->ctx_lnum = start_ctx_lnum;

	    status = check_ppconst_bool(ppconst);
	    if (status != FAIL)
	    {
		// Use the last ppconst if possible.
		if (ppconst->pp_used > 0)
		{
		    typval_T	*tv = &ppconst->pp_tv[ppconst->pp_used - 1];
		    int		is_true = tv2bool(tv);

		    if ((is_true && opchar == '|')
						|| (!is_true && opchar == '&'))
		    {
			// For "false && expr" and "true || expr" the "expr"
			// does not need to be evaluated.
			cctx->ctx_skip = SKIP_YES;
			clear_tv(tv);
			tv->v_type = VAR_BOOL;
			tv->vval.v_number = is_true ? VVAL_TRUE : VVAL_FALSE;
		    }
		    else
		    {
			// For "true && expr" and "false || expr" only "expr"
			// needs to be evaluated.
			--ppconst->pp_used;
			jump_when = JUMP_NEVER;
		    }
		}
		else
		{
		    // Every part must evaluate to a bool.
		    status = bool_on_stack(cctx);
		}
	    }
	    if (status != FAIL)
		status = ga_grow(&end_ga, 1);
	    cctx->ctx_lnum = save_lnum;
	    if (status == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	    if (jump_when != JUMP_NEVER)
	    {
		if (cctx->ctx_skip != SKIP_YES)
		{
		    *(((int *)end_ga.ga_data) + end_ga.ga_len) = instr->ga_len;
		    ++end_ga.ga_len;
		}
		generate_JUMP(cctx, jump_when, 0);
	    }

	    // eval the next expression
	    SOURCING_LNUM = save_sourcing_lnum;
	    if (may_get_next_line_error(p + 2, arg, cctx) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	    const_used = ppconst->pp_used;
	    if ((opchar == '|' ? compile_expr3(arg, cctx, ppconst)
				  : compile_expr4(arg, cctx, ppconst)) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	    // "0 || 1" results in true, "1 && 0" results in false.
	    if (ppconst->pp_used == const_used + 1)
	    {
		typval_T	*tv = &ppconst->pp_tv[ppconst->pp_used - 1];

		if (tv->v_type == VAR_NUMBER
			 && (tv->vval.v_number == 1 || tv->vval.v_number == 0))
		{
		    tv->vval.v_number = tv->vval.v_number == 1
						      ? VVAL_TRUE : VVAL_FALSE;
		    tv->v_type = VAR_BOOL;
		}
	    }

	    p = may_peek_next_line(cctx, *arg, &next);
	}

	if (check_ppconst_bool(ppconst) == FAIL)
	{
	    ga_clear(&end_ga);
	    return FAIL;
	}

	if (cctx->ctx_skip != SKIP_YES && ppconst->pp_used == 0)
	    // Every part must evaluate to a bool.
	    if (bool_on_stack(cctx) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	if (end_ga.ga_len > 0)
	{
	    // Fill in the end label in all jumps.
	    generate_ppconst(cctx, ppconst);
	    while (end_ga.ga_len > 0)
	    {
		isn_T	*isn;

		--end_ga.ga_len;
		isn = ((isn_T *)instr->ga_data)
				  + *(((int *)end_ga.ga_data) + end_ga.ga_len);
		isn->isn_arg.jump.jump_where = instr->ga_len;
	    }
	}
	ga_clear(&end_ga);

	cctx->ctx_skip = save_skip;
    }

    return OK;
}

/*
 * expr4a && expr4a && expr4a	    logical AND
 *
 * Produces instructions:
 *	EVAL expr4a		Push result of "expr4a"
 *	COND2BOOL		convert to bool if needed
 *	JUMP_IF_COND_FALSE end
 *	EVAL expr4b		Push result of "expr4b"
 *	JUMP_IF_COND_FALSE end
 *	EVAL expr4c		Push result of "expr4c"
 * end:
 */
    static int
compile_expr3(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    int		ppconst_used = ppconst->pp_used;

    // get the first variable
    if (compile_expr4(arg, cctx, ppconst) == FAIL)
	return FAIL;

    // || and && work almost the same
    return compile_and_or(arg, cctx, "&&", ppconst, ppconst_used);
}

/*
 * expr3a || expr3b || expr3c	    logical OR
 *
 * Produces instructions:
 *	EVAL expr3a		Push result of "expr3a"
 *	COND2BOOL		convert to bool if needed
 *	JUMP_IF_COND_TRUE end
 *	EVAL expr3b		Push result of "expr3b"
 *	JUMP_IF_COND_TRUE end
 *	EVAL expr3c		Push result of "expr3c"
 * end:
 */
    static int
compile_expr2(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    int		ppconst_used = ppconst->pp_used;

    // eval the first expression
    if (compile_expr3(arg, cctx, ppconst) == FAIL)
	return FAIL;

    // || and && work almost the same
    return compile_and_or(arg, cctx, "||", ppconst, ppconst_used);
}

/*
 * Toplevel expression: expr2 ? expr1a : expr1b
 * Produces instructions:
 *	EVAL expr2		Push result of "expr2"
 *      JUMP_IF_FALSE alt	jump if false
 *      EVAL expr1a
 *      JUMP_ALWAYS end
 * alt:	EVAL expr1b
 * end:
 *
 * Toplevel expression: expr2 ?? expr1
 * Produces instructions:
 *	EVAL expr2		    Push result of "expr2"
 *      JUMP_AND_KEEP_IF_TRUE end   jump if true
 *      EVAL expr1
 * end:
 */
    int
compile_expr1(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
{
    char_u	*p;
    int		ppconst_used = ppconst->pp_used;
    char_u	*next;

    // Ignore all kinds of errors when not producing code.
    if (cctx->ctx_skip == SKIP_YES)
    {
	skip_expr_cctx(arg, cctx);
	return OK;
    }

    // Evaluate the first expression.
    if (compile_expr2(arg, cctx, ppconst) == FAIL)
	return FAIL;

    p = may_peek_next_line(cctx, *arg, &next);
    if (*p == '?')
    {
	int		op_falsy = p[1] == '?';
	garray_T	*instr = &cctx->ctx_instr;
	garray_T	*stack = &cctx->ctx_type_stack;
	int		alt_idx = instr->ga_len;
	int		end_idx = 0;
	isn_T		*isn;
	type_T		*type1 = NULL;
	int		has_const_expr = FALSE;
	int		const_value = FALSE;
	int		save_skip = cctx->ctx_skip;

	if (next != NULL)
	{
	    *arg = next_line_from_context(cctx, TRUE);
	    p = skipwhite(*arg);
	}

	if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(p[1 + op_falsy]))
	{
	    semsg(_(e_white_space_required_before_and_after_str_at_str),
						     op_falsy ? "??" : "?", p);
	    return FAIL;
	}

	if (ppconst->pp_used == ppconst_used + 1)
	{
	    // the condition is a constant, we know whether the ? or the :
	    // expression is to be evaluated.
	    has_const_expr = TRUE;
	    if (op_falsy)
		const_value = tv2bool(&ppconst->pp_tv[ppconst_used]);
	    else
	    {
		int error = FALSE;

		const_value = tv_get_bool_chk(&ppconst->pp_tv[ppconst_used],
								       &error);
		if (error)
		    return FAIL;
	    }
	    cctx->ctx_skip = save_skip == SKIP_YES ||
		 (op_falsy ? const_value : !const_value) ? SKIP_YES : SKIP_NOT;

	    if (op_falsy && cctx->ctx_skip == SKIP_YES)
		// "left ?? right" and "left" is truthy: produce "left"
		generate_ppconst(cctx, ppconst);
	    else
	    {
		clear_tv(&ppconst->pp_tv[ppconst_used]);
		--ppconst->pp_used;
	    }
	}
	else
	{
	    generate_ppconst(cctx, ppconst);
	    if (op_falsy)
		end_idx = instr->ga_len;
	    generate_JUMP(cctx, op_falsy
				   ? JUMP_AND_KEEP_IF_TRUE : JUMP_IF_FALSE, 0);
	    if (op_falsy)
		type1 = get_type_on_stack(cctx, -1);
	}

	// evaluate the second expression; any type is accepted
	if (may_get_next_line_error(p + 1 + op_falsy, arg, cctx) == FAIL)
	    return FAIL;
	if (compile_expr1(arg, cctx, ppconst) == FAIL)
	    return FAIL;

	if (!has_const_expr)
	{
	    generate_ppconst(cctx, ppconst);

	    if (!op_falsy)
	    {
		// remember the type and drop it
		type1 = get_type_on_stack(cctx, 0);
		--stack->ga_len;

		end_idx = instr->ga_len;
		generate_JUMP(cctx, JUMP_ALWAYS, 0);

		// jump here from JUMP_IF_FALSE
		isn = ((isn_T *)instr->ga_data) + alt_idx;
		isn->isn_arg.jump.jump_where = instr->ga_len;
	    }
	}

	if (!op_falsy)
	{
	    // Check for the ":".
	    p = may_peek_next_line(cctx, *arg, &next);
	    if (*p != ':')
	    {
		emsg(_(e_missing_colon_after_questionmark));
		return FAIL;
	    }
	    if (next != NULL)
	    {
		*arg = next_line_from_context(cctx, TRUE);
		p = skipwhite(*arg);
	    }

	    if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(p[1]))
	    {
		semsg(_(e_white_space_required_before_and_after_str_at_str),
								       ":", p);
		return FAIL;
	    }

	    // evaluate the third expression
	    if (has_const_expr)
		cctx->ctx_skip = save_skip == SKIP_YES || const_value
							 ? SKIP_YES : SKIP_NOT;
	    if (may_get_next_line_error(p + 1, arg, cctx) == FAIL)
		return FAIL;
	    if (compile_expr1(arg, cctx, ppconst) == FAIL)
		return FAIL;
	}

	if (!has_const_expr)
	{
	    type_T	**typep;

	    generate_ppconst(cctx, ppconst);
	    ppconst->pp_is_const = FALSE;

	    // If the types differ, the result has a more generic type.
	    typep = &((((type2_T *)stack->ga_data)
					      + stack->ga_len - 1)->type_curr);
	    common_type(type1, *typep, typep, cctx->ctx_type_list);

	    // jump here from JUMP_ALWAYS or JUMP_AND_KEEP_IF_TRUE
	    isn = ((isn_T *)instr->ga_data) + end_idx;
	    isn->isn_arg.jump.jump_where = instr->ga_len;
	}

	cctx->ctx_skip = save_skip;
    }
    return OK;
}

/*
 * Toplevel expression.
 * Sets "is_const" (if not NULL) to indicate the value is a constant.
 * Returns OK or FAIL.
 */
    int
compile_expr0_ext(char_u **arg,  cctx_T *cctx, int *is_const)
{
    ppconst_T	ppconst;

    CLEAR_FIELD(ppconst);
    if (compile_expr1(arg, cctx, &ppconst) == FAIL)
    {
	clear_ppconst(&ppconst);
	return FAIL;
    }
    if (is_const != NULL)
	*is_const = ppconst.pp_used > 0 || ppconst.pp_is_const;
    if (generate_ppconst(cctx, &ppconst) == FAIL)
	return FAIL;
    return OK;
}

/*
 * Toplevel expression.
 */
    int
compile_expr0(char_u **arg,  cctx_T *cctx)
{
    return compile_expr0_ext(arg, cctx, NULL);
}


#endif // defined(FEAT_EVAL)
