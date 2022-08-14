/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9cmds.c: Dealing with commands of a compiled function
 */

#define USING_FLOAT_STUFF
#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

// When not generating protos this is included in proto.h
#ifdef PROTO
# include "vim9.h"
#endif

/*
 * Get the index of the current instruction.
 * This compensates for a preceding ISN_CMDMOD and ISN_PROF_START.
 */
    static int
current_instr_idx(cctx_T *cctx)
{
    garray_T	*instr = &cctx->ctx_instr;
    int		idx = instr->ga_len;

    while (idx > 0)
    {
	if (cctx->ctx_has_cmdmod && ((isn_T *)instr->ga_data)[idx - 1]
						       .isn_type == ISN_CMDMOD)
	{
	    --idx;
	    continue;
	}
#ifdef FEAT_PROFILE
	if (((isn_T *)instr->ga_data)[idx - 1].isn_type == ISN_PROF_START)
	{
	    --idx;
	    continue;
	}
#endif
	if (((isn_T *)instr->ga_data)[idx - 1].isn_type == ISN_DEBUG)
	{
	    --idx;
	    continue;
	}
	break;
    }
    return idx;
}
/*
 * Remove local variables above "new_top".
 */
    static void
unwind_locals(cctx_T *cctx, int new_top)
{
    if (cctx->ctx_locals.ga_len > new_top)
    {
	int	idx;
	lvar_T	*lvar;

	for (idx = new_top; idx < cctx->ctx_locals.ga_len; ++idx)
	{
	    lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;
	    vim_free(lvar->lv_name);
	}
    }
    cctx->ctx_locals.ga_len = new_top;
}

/*
 * Free all local variables.
 */
    void
free_locals(cctx_T *cctx)
{
    unwind_locals(cctx, 0);
    ga_clear(&cctx->ctx_locals);
}


/*
 * Check if "name" can be "unlet".
 */
    int
check_vim9_unlet(char_u *name)
{
    if (name[1] != ':' || vim_strchr((char_u *)"gwtb", *name) == NULL)
    {
	// "unlet s:var" is allowed in legacy script.
	if (*name == 's' && !script_is_vim9())
	    return OK;
	semsg(_(e_cannot_unlet_str), name);
	return FAIL;
    }
    return OK;
}

/*
 * Callback passed to ex_unletlock().
 */
    static int
compile_unlet(
    lval_T  *lvp,
    char_u  *name_end,
    exarg_T *eap,
    int	    deep UNUSED,
    void    *coookie)
{
    cctx_T	*cctx = coookie;
    char_u	*p = lvp->ll_name;
    int		cc = *name_end;
    int		ret = OK;

    if (cctx->ctx_skip == SKIP_YES)
	return OK;

    *name_end = NUL;
    if (*p == '$')
    {
	// :unlet $ENV_VAR
	ret = generate_UNLET(cctx, ISN_UNLETENV, p + 1, eap->forceit);
    }
    else if (vim_strchr(p, '.') != NULL || vim_strchr(p, '[') != NULL)
    {
	lhs_T	    lhs;

	// This is similar to assigning: lookup the list/dict, compile the
	// idx/key.  Then instead of storing the value unlet the item.
	// unlet {list}[idx]
	// unlet {dict}[key]  dict.key
	//
	// Figure out the LHS type and other properties.
	//
	ret = compile_lhs(p, &lhs, CMD_unlet, FALSE, FALSE, 0, cctx);

	// Use the info in "lhs" to unlet the item at the index in the
	// list or dict.
	if (ret == OK)
	{
	    if (!lhs.lhs_has_index)
	    {
		semsg(_(e_cannot_unlet_imported_item_str), p);
		ret = FAIL;
	    }
	    else
		ret = compile_assign_unlet(p, &lhs, FALSE, &t_void, cctx);
	}

	vim_free(lhs.lhs_name);
    }
    else if (check_vim9_unlet(p) == FAIL)
    {
	ret = FAIL;
    }
    else
    {
	// Normal name.  Only supports g:, w:, t: and b: namespaces.
	ret = generate_UNLET(cctx, ISN_UNLET, p, eap->forceit);
    }

    *name_end = cc;
    return ret;
}

/*
 * Callback passed to ex_unletlock().
 */
    static int
compile_lock_unlock(
    lval_T  *lvp,
    char_u  *name_end,
    exarg_T *eap,
    int	    deep,
    void    *coookie)
{
    cctx_T	*cctx = coookie;
    int		cc = *name_end;
    char_u	*p = lvp->ll_name;
    int		ret = OK;
    size_t	len;
    char_u	*buf;
    isntype_T	isn = ISN_EXEC;
    char	*cmd = eap->cmdidx == CMD_lockvar ? "lockvar" : "unlockvar";

    if (cctx->ctx_skip == SKIP_YES)
	return OK;

    if (*p == NUL)
    {
	semsg(_(e_argument_required_for_str), cmd);
	return FAIL;
    }

    // Cannot use :lockvar and :unlockvar on local variables.
    if (p[1] != ':')
    {
	char_u *end = find_name_end(p, NULL, NULL, FNE_CHECK_START);

	if (lookup_local(p, end - p, NULL, cctx) == OK)
	{
	    char_u *s = p;

	    if (*end != '.' && *end != '[')
	    {
		emsg(_(e_cannot_lock_unlock_local_variable));
		return FAIL;
	    }

	    // For "d.member" put the local variable on the stack, it will be
	    // passed to ex_lockvar() indirectly.
	    if (compile_load(&s, end, cctx, FALSE, FALSE) == FAIL)
		return FAIL;
	    isn = ISN_LOCKUNLOCK;
	}
    }

    // Checking is done at runtime.
    *name_end = NUL;
    len = name_end - p + 20;
    buf = alloc(len);
    if (buf == NULL)
	ret = FAIL;
    else
    {
	if (deep < 0)
	    vim_snprintf((char *)buf, len, "%s! %s", cmd, p);
	else
	    vim_snprintf((char *)buf, len, "%s %d %s", cmd, deep, p);
	ret = generate_EXEC_copy(cctx, isn, buf);

	vim_free(buf);
	*name_end = cc;
    }
    return ret;
}

/*
 * compile "unlet var", "lock var" and "unlock var"
 * "arg" points to "var".
 */
    char_u *
compile_unletlock(char_u *arg, exarg_T *eap, cctx_T *cctx)
{
    int	    deep = 0;
    char_u  *p = arg;

    if (eap->cmdidx != CMD_unlet)
    {
	if (eap->forceit)
	    deep = -1;
	else if (vim_isdigit(*p))
	{
	    deep = getdigits(&p);
	    p = skipwhite(p);
	}
	else
	    deep = 2;
    }

    ex_unletlock(eap, p, deep, GLV_NO_AUTOLOAD | GLV_COMPILING,
	    eap->cmdidx == CMD_unlet ? compile_unlet : compile_lock_unlock,
	    cctx);
    return eap->nextcmd == NULL ? (char_u *)"" : eap->nextcmd;
}

/*
 * generate a jump to the ":endif"/":endfor"/":endwhile"/":finally"/":endtry".
 */
    static int
compile_jump_to_end(endlabel_T **el, jumpwhen_T when, cctx_T *cctx)
{
    garray_T	*instr = &cctx->ctx_instr;
    endlabel_T  *endlabel = ALLOC_CLEAR_ONE(endlabel_T);

    if (endlabel == NULL)
	return FAIL;
    endlabel->el_next = *el;
    *el = endlabel;
    endlabel->el_end_label = instr->ga_len;

    generate_JUMP(cctx, when, 0);
    return OK;
}

    static void
compile_fill_jump_to_end(endlabel_T **el, int jump_where, cctx_T *cctx)
{
    garray_T	*instr = &cctx->ctx_instr;

    while (*el != NULL)
    {
	endlabel_T  *cur = (*el);
	isn_T	    *isn;

	isn = ((isn_T *)instr->ga_data) + cur->el_end_label;
	isn->isn_arg.jump.jump_where = jump_where;
	*el = cur->el_next;
	vim_free(cur);
    }
}

    static void
compile_free_jump_to_end(endlabel_T **el)
{
    while (*el != NULL)
    {
	endlabel_T  *cur = (*el);

	*el = cur->el_next;
	vim_free(cur);
    }
}

/*
 * Create a new scope and set up the generic items.
 */
    static scope_T *
new_scope(cctx_T *cctx, scopetype_T type)
{
    scope_T *scope = ALLOC_CLEAR_ONE(scope_T);

    if (scope == NULL)
	return NULL;
    scope->se_outer = cctx->ctx_scope;
    cctx->ctx_scope = scope;
    scope->se_type = type;
    scope->se_local_count = cctx->ctx_locals.ga_len;
    return scope;
}

/*
 * Free the current scope and go back to the outer scope.
 */
    void
drop_scope(cctx_T *cctx)
{
    scope_T *scope = cctx->ctx_scope;

    if (scope == NULL)
    {
	iemsg("calling drop_scope() without a scope");
	return;
    }
    cctx->ctx_scope = scope->se_outer;
    switch (scope->se_type)
    {
	case IF_SCOPE:
	    compile_free_jump_to_end(&scope->se_u.se_if.is_end_label); break;
	case FOR_SCOPE:
	    compile_free_jump_to_end(&scope->se_u.se_for.fs_end_label); break;
	case WHILE_SCOPE:
	    compile_free_jump_to_end(&scope->se_u.se_while.ws_end_label); break;
	case TRY_SCOPE:
	    compile_free_jump_to_end(&scope->se_u.se_try.ts_end_label); break;
	case NO_SCOPE:
	case BLOCK_SCOPE:
	    break;
    }
    vim_free(scope);
}

    static int
misplaced_cmdmod(cctx_T *cctx)
{
    garray_T	*instr = &cctx->ctx_instr;

    if (cctx->ctx_has_cmdmod
	    && ((isn_T *)instr->ga_data)[instr->ga_len - 1].isn_type
								 == ISN_CMDMOD)
    {
	emsg(_(e_misplaced_command_modifier));
	return TRUE;
    }
    return FALSE;
}

/*
 * compile "if expr"
 *
 * "if expr" Produces instructions:
 *	EVAL expr		Push result of "expr"
 *	JUMP_IF_FALSE end
 *	... body ...
 * end:
 *
 * "if expr | else" Produces instructions:
 *	EVAL expr		Push result of "expr"
 *	JUMP_IF_FALSE else
 *	... body ...
 *	JUMP_ALWAYS end
 * else:
 *	... body ...
 * end:
 *
 * "if expr1 | elseif expr2 | else" Produces instructions:
 *	EVAL expr		Push result of "expr"
 *	JUMP_IF_FALSE elseif
 *	... body ...
 *	JUMP_ALWAYS end
 * elseif:
 *	EVAL expr		Push result of "expr"
 *	JUMP_IF_FALSE else
 *	... body ...
 *	JUMP_ALWAYS end
 * else:
 *	... body ...
 * end:
 */
    char_u *
compile_if(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    garray_T	*instr = &cctx->ctx_instr;
    int		instr_count = instr->ga_len;
    scope_T	*scope;
    skip_T	skip_save = cctx->ctx_skip;
    ppconst_T	ppconst;

    CLEAR_FIELD(ppconst);
    if (compile_expr1(&p, cctx, &ppconst) == FAIL)
    {
	clear_ppconst(&ppconst);
	return NULL;
    }
    if (!ends_excmd2(arg, skipwhite(p)))
    {
	semsg(_(e_trailing_characters_str), p);
	return NULL;
    }
    if (cctx->ctx_skip == SKIP_YES)
	clear_ppconst(&ppconst);
    else if (instr->ga_len == instr_count && ppconst.pp_used == 1)
    {
	int error = FALSE;
	int v;

	// The expression results in a constant.
	v = tv_get_bool_chk(&ppconst.pp_tv[0], &error);
	clear_ppconst(&ppconst);
	if (error)
	    return NULL;
	cctx->ctx_skip = v ? SKIP_NOT : SKIP_YES;
    }
    else
    {
	// Not a constant, generate instructions for the expression.
	cctx->ctx_skip = SKIP_UNKNOWN;
	if (generate_ppconst(cctx, &ppconst) == FAIL)
	    return NULL;
	if (bool_on_stack(cctx) == FAIL)
	    return NULL;
    }

    // CMDMOD_REV must come before the jump
    generate_undo_cmdmods(cctx);

    scope = new_scope(cctx, IF_SCOPE);
    if (scope == NULL)
	return NULL;
    scope->se_skip_save = skip_save;
    // "is_had_return" will be reset if any block does not end in :return
    scope->se_u.se_if.is_had_return = TRUE;

    if (cctx->ctx_skip == SKIP_UNKNOWN)
    {
	// "where" is set when ":elseif", "else" or ":endif" is found
	scope->se_u.se_if.is_if_label = instr->ga_len;
	generate_JUMP(cctx, JUMP_IF_FALSE, 0);
    }
    else
	scope->se_u.se_if.is_if_label = -1;

#ifdef FEAT_PROFILE
    if (cctx->ctx_compile_type == CT_PROFILE && cctx->ctx_skip == SKIP_YES
						      && skip_save != SKIP_YES)
    {
	// generated a profile start, need to generate a profile end, since it
	// won't be done after returning
	cctx->ctx_skip = SKIP_NOT;
	generate_instr(cctx, ISN_PROF_END);
	cctx->ctx_skip = SKIP_YES;
    }
#endif

    return p;
}

    char_u *
compile_elseif(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    garray_T	*instr = &cctx->ctx_instr;
    int		instr_count;
    isn_T	*isn;
    scope_T	*scope = cctx->ctx_scope;
    ppconst_T	ppconst;
    skip_T	save_skip = cctx->ctx_skip;

    if (scope == NULL || scope->se_type != IF_SCOPE)
    {
	emsg(_(e_elseif_without_if));
	return NULL;
    }
    unwind_locals(cctx, scope->se_local_count);
    if (!cctx->ctx_had_return)
	scope->se_u.se_if.is_had_return = FALSE;

    if (cctx->ctx_skip == SKIP_NOT)
    {
	// previous block was executed, this one and following will not
	cctx->ctx_skip = SKIP_YES;
	scope->se_u.se_if.is_seen_skip_not = TRUE;
    }
    if (scope->se_u.se_if.is_seen_skip_not)
    {
	// A previous block was executed, skip over expression and bail out.
	// Do not count the "elseif" for profiling and cmdmod
	instr->ga_len = current_instr_idx(cctx);

	skip_expr_cctx(&p, cctx);
	return p;
    }

    if (cctx->ctx_skip == SKIP_UNKNOWN)
    {
	int	    moved_cmdmod = FALSE;
	int	    saved_debug = FALSE;
	isn_T	    debug_isn;

	// Move any CMDMOD instruction to after the jump
	if (((isn_T *)instr->ga_data)[instr->ga_len - 1].isn_type == ISN_CMDMOD)
	{
	    if (GA_GROW_FAILS(instr, 1))
		return NULL;
	    ((isn_T *)instr->ga_data)[instr->ga_len] =
				  ((isn_T *)instr->ga_data)[instr->ga_len - 1];
	    --instr->ga_len;
	    moved_cmdmod = TRUE;
	}

	// Remove the already generated ISN_DEBUG, it is written below the
	// ISN_FOR instruction.
	if (cctx->ctx_compile_type == CT_DEBUG && instr->ga_len > 0
		&& ((isn_T *)instr->ga_data)[instr->ga_len - 1]
							.isn_type == ISN_DEBUG)
	{
	    --instr->ga_len;
	    debug_isn = ((isn_T *)instr->ga_data)[instr->ga_len];
	    saved_debug = TRUE;
	}

	if (compile_jump_to_end(&scope->se_u.se_if.is_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	    return NULL;
	// previous "if" or "elseif" jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_if.is_if_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;

	if (moved_cmdmod)
	    ++instr->ga_len;

	if (saved_debug)
	{
	    // move the debug instruction here
	    if (GA_GROW_FAILS(instr, 1))
		return NULL;
	    ((isn_T *)instr->ga_data)[instr->ga_len] = debug_isn;
	    ++instr->ga_len;
	}
    }

    // compile "expr"; if we know it evaluates to FALSE skip the block
    CLEAR_FIELD(ppconst);
    if (cctx->ctx_skip == SKIP_YES)
    {
	cctx->ctx_skip = SKIP_UNKNOWN;
#ifdef FEAT_PROFILE
	if (cctx->ctx_compile_type == CT_PROFILE)
	    // the previous block was skipped, need to profile this line
	    generate_instr(cctx, ISN_PROF_START);
#endif
	if (cctx->ctx_compile_type == CT_DEBUG)
	    // the previous block was skipped, may want to debug this line
	    generate_instr_debug(cctx);
    }

    instr_count = instr->ga_len;
    if (compile_expr1(&p, cctx, &ppconst) == FAIL)
    {
	clear_ppconst(&ppconst);
	return NULL;
    }
    cctx->ctx_skip = save_skip;
    if (!ends_excmd2(arg, skipwhite(p)))
    {
	clear_ppconst(&ppconst);
	semsg(_(e_trailing_characters_str), p);
	return NULL;
    }
    if (scope->se_skip_save == SKIP_YES)
	clear_ppconst(&ppconst);
    else if (instr->ga_len == instr_count && ppconst.pp_used == 1)
    {
	int error = FALSE;
	int v;

	// The expression result is a constant.
	v = tv_get_bool_chk(&ppconst.pp_tv[0], &error);
	if (error)
	{
	    clear_ppconst(&ppconst);
	    return NULL;
	}
	cctx->ctx_skip = v ? SKIP_NOT : SKIP_YES;
	clear_ppconst(&ppconst);
	scope->se_u.se_if.is_if_label = -1;
    }
    else
    {
	// Not a constant, generate instructions for the expression.
	cctx->ctx_skip = SKIP_UNKNOWN;
	if (generate_ppconst(cctx, &ppconst) == FAIL)
	    return NULL;
	if (bool_on_stack(cctx) == FAIL)
	    return NULL;

	// CMDMOD_REV must come before the jump
	generate_undo_cmdmods(cctx);

	// "where" is set when ":elseif", "else" or ":endif" is found
	scope->se_u.se_if.is_if_label = instr->ga_len;
	generate_JUMP(cctx, JUMP_IF_FALSE, 0);
    }

    return p;
}

    char_u *
compile_else(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*isn;
    scope_T	*scope = cctx->ctx_scope;

    if (scope == NULL || scope->se_type != IF_SCOPE)
    {
	emsg(_(e_else_without_if));
	return NULL;
    }
    unwind_locals(cctx, scope->se_local_count);
    if (!cctx->ctx_had_return)
	scope->se_u.se_if.is_had_return = FALSE;
    scope->se_u.se_if.is_seen_else = TRUE;

#ifdef FEAT_PROFILE
    if (cctx->ctx_compile_type == CT_PROFILE)
    {
	if (cctx->ctx_skip == SKIP_NOT
		&& ((isn_T *)instr->ga_data)[instr->ga_len - 1]
						   .isn_type == ISN_PROF_START)
	    // the previous block was executed, do not count "else" for
	    // profiling
	    --instr->ga_len;
	if (cctx->ctx_skip == SKIP_YES && !scope->se_u.se_if.is_seen_skip_not)
	{
	    // the previous block was not executed, this one will, do count the
	    // "else" for profiling
	    cctx->ctx_skip = SKIP_NOT;
	    generate_instr(cctx, ISN_PROF_END);
	    generate_instr(cctx, ISN_PROF_START);
	    cctx->ctx_skip = SKIP_YES;
	}
    }
#endif

    if (!scope->se_u.se_if.is_seen_skip_not && scope->se_skip_save != SKIP_YES)
    {
	// jump from previous block to the end, unless the else block is empty
	if (cctx->ctx_skip == SKIP_UNKNOWN)
	{
	    if (!cctx->ctx_had_return
		    && compile_jump_to_end(&scope->se_u.se_if.is_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
		return NULL;
	}

	if (cctx->ctx_skip == SKIP_UNKNOWN)
	{
	    if (scope->se_u.se_if.is_if_label >= 0)
	    {
		// previous "if" or "elseif" jumps here
		isn = ((isn_T *)instr->ga_data) + scope->se_u.se_if.is_if_label;
		isn->isn_arg.jump.jump_where = instr->ga_len;
		scope->se_u.se_if.is_if_label = -1;
	    }
	}

	if (cctx->ctx_skip != SKIP_UNKNOWN)
	    cctx->ctx_skip = cctx->ctx_skip == SKIP_YES ? SKIP_NOT : SKIP_YES;
    }

    return p;
}

    char_u *
compile_endif(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    ifscope_T	*ifscope;
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*isn;

    if (misplaced_cmdmod(cctx))
	return NULL;

    if (scope == NULL || scope->se_type != IF_SCOPE)
    {
	emsg(_(e_endif_without_if));
	return NULL;
    }
    ifscope = &scope->se_u.se_if;
    unwind_locals(cctx, scope->se_local_count);
    if (!cctx->ctx_had_return)
	ifscope->is_had_return = FALSE;

    if (scope->se_u.se_if.is_if_label >= 0)
    {
	// previous "if" or "elseif" jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_if.is_if_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
    }
    // Fill in the "end" label in jumps at the end of the blocks.
    compile_fill_jump_to_end(&ifscope->is_end_label, instr->ga_len, cctx);

#ifdef FEAT_PROFILE
    // even when skipping we count the endif as executed, unless the block it's
    // in is skipped
    if (cctx->ctx_compile_type == CT_PROFILE && cctx->ctx_skip == SKIP_YES
					    && scope->se_skip_save != SKIP_YES)
    {
	cctx->ctx_skip = SKIP_NOT;
	generate_instr(cctx, ISN_PROF_START);
    }
#endif
    cctx->ctx_skip = scope->se_skip_save;

    // If all the blocks end in :return and there is an :else then the
    // had_return flag is set.
    cctx->ctx_had_return = ifscope->is_had_return && ifscope->is_seen_else;

    drop_scope(cctx);
    return arg;
}

/*
 * Compile "for var in expr":
 *
 * Produces instructions:
 *       PUSHNR -1
 *       STORE loop-idx		Set index to -1
 *       EVAL expr		result of "expr" on top of stack
 * top:  FOR loop-idx, end	Increment index, use list on bottom of stack
 *				- if beyond end, jump to "end"
 *				- otherwise get item from list and push it
 *       STORE var		Store item in "var"
 *       ... body ...
 *       JUMP top		Jump back to repeat
 * end:	 DROP			Drop the result of "expr"
 *
 * Compile "for [var1, var2] in expr" - as above, but instead of "STORE var":
 *	 UNPACK 2		Split item in 2
 *       STORE var1		Store item in "var1"
 *       STORE var2		Store item in "var2"
 */
    char_u *
compile_for(char_u *arg_start, cctx_T *cctx)
{
    char_u	*arg;
    char_u	*arg_end;
    char_u	*name = NULL;
    char_u	*p;
    char_u	*wp;
    int		var_count = 0;
    int		var_list = FALSE;
    int		semicolon = FALSE;
    size_t	varlen;
    garray_T	*instr = &cctx->ctx_instr;
    scope_T	*scope;
    lvar_T	*loop_lvar;	// loop iteration variable
    lvar_T	*var_lvar;	// variable for "var"
    type_T	*vartype;
    type_T	*item_type = &t_any;
    int		idx;
    int		prev_lnum = cctx->ctx_prev_lnum;

    p = skip_var_list(arg_start, TRUE, &var_count, &semicolon, FALSE);
    if (p == NULL)
	return NULL;
    if (var_count == 0)
	var_count = 1;
    else
	var_list = TRUE;  // can also be a list of one variable

    // consume "in"
    wp = p;
    if (may_get_next_line_error(wp, &p, cctx) == FAIL)
	return NULL;
    if (STRNCMP(p, "in", 2) != 0 || !IS_WHITE_OR_NUL(p[2]))
    {
	if (*p == ':' && wp != p)
	    semsg(_(e_no_white_space_allowed_before_colon_str), p);
	else
	    emsg(_(e_missing_in_after_for));
	return NULL;
    }
    wp = p + 2;
    if (may_get_next_line_error(wp, &p, cctx) == FAIL)
	return NULL;

    // Find the already generated ISN_DEBUG to get the line number for the
    // instruction written below the ISN_FOR instruction.
    if (cctx->ctx_compile_type == CT_DEBUG && instr->ga_len > 0
	    && ((isn_T *)instr->ga_data)[instr->ga_len - 1]
							.isn_type == ISN_DEBUG)
    {
	prev_lnum = ((isn_T *)instr->ga_data)[instr->ga_len - 1]
						 .isn_arg.debug.dbg_break_lnum;
    }

    scope = new_scope(cctx, FOR_SCOPE);
    if (scope == NULL)
	return NULL;

    // Reserve a variable to store the loop iteration counter and initialize it
    // to -1.
    loop_lvar = reserve_local(cctx, (char_u *)"", 0, FALSE, &t_number);
    if (loop_lvar == NULL)
    {
	// out of memory
	drop_scope(cctx);
	return NULL;
    }
    generate_STORENR(cctx, loop_lvar->lv_idx, -1);

    // compile "expr", it remains on the stack until "endfor"
    arg = p;
    if (compile_expr0(&arg, cctx) == FAIL)
    {
	drop_scope(cctx);
	return NULL;
    }
    arg_end = arg;

    if (cctx->ctx_skip != SKIP_YES)
    {
	// If we know the type of "var" and it is not a supported type we can
	// give an error now.
	vartype = get_type_on_stack(cctx, 0);
	if (vartype->tt_type != VAR_LIST
		&& vartype->tt_type != VAR_STRING
		&& vartype->tt_type != VAR_BLOB
		&& vartype->tt_type != VAR_ANY
		&& vartype->tt_type != VAR_UNKNOWN)
	{
	    semsg(_(e_for_loop_on_str_not_supported),
					       vartype_name(vartype->tt_type));
	    drop_scope(cctx);
	    return NULL;
	}

	if (vartype->tt_type == VAR_STRING)
	    item_type = &t_string;
	else if (vartype->tt_type == VAR_BLOB)
	    item_type = &t_number;
	else if (vartype->tt_type == VAR_LIST
				     && vartype->tt_member->tt_type != VAR_ANY)
	{
	    if (!var_list)
		item_type = vartype->tt_member;
	    else if (vartype->tt_member->tt_type == VAR_LIST
			  && vartype->tt_member->tt_member->tt_type != VAR_ANY)
		item_type = vartype->tt_member->tt_member;
	}

	// CMDMOD_REV must come before the FOR instruction.
	generate_undo_cmdmods(cctx);

	// "for_end" is set when ":endfor" is found
	scope->se_u.se_for.fs_top_label = current_instr_idx(cctx);

	if (cctx->ctx_compile_type == CT_DEBUG)
	{
	    int		save_prev_lnum = cctx->ctx_prev_lnum;
	    isn_T	*isn;

	    // Add ISN_DEBUG here, before deciding to end the loop.  There will
	    // be another ISN_DEBUG before the next instruction.
	    // Use the prev_lnum from the ISN_DEBUG instruction removed above.
	    // Increment the variable count so that the loop variable can be
	    // inspected.
	    cctx->ctx_prev_lnum = prev_lnum;
	    isn = generate_instr_debug(cctx);
	    ++isn->isn_arg.debug.dbg_var_names_len;
	    cctx->ctx_prev_lnum = save_prev_lnum;
	}

	generate_FOR(cctx, loop_lvar->lv_idx);

	arg = arg_start;
	if (var_list)
	{
	    generate_UNPACK(cctx, var_count, semicolon);
	    arg = skipwhite(arg + 1);	// skip white after '['

	    // drop the list item
	    --cctx->ctx_type_stack.ga_len;

	    // add type of the items
	    for (idx = 0; idx < var_count; ++idx)
	    {
		type_T *type = (semicolon && idx == 0) ? vartype : item_type;

		if (push_type_stack(cctx, type) == FAIL)
		{
		    drop_scope(cctx);
		    return NULL;
		}
	    }
	}

	for (idx = 0; idx < var_count; ++idx)
	{
	    assign_dest_T	dest = dest_local;
	    int			opt_flags = 0;
	    int			vimvaridx = -1;
	    type_T		*type = &t_any;
	    type_T		*lhs_type = &t_any;
	    where_T		where = WHERE_INIT;

	    p = skip_var_one(arg, FALSE);
	    varlen = p - arg;
	    name = vim_strnsave(arg, varlen);
	    if (name == NULL)
		goto failed;
	    if (*p == ':')
	    {
		p = skipwhite(p + 1);
		lhs_type = parse_type(&p, cctx->ctx_type_list, TRUE);
	    }

	    if (get_var_dest(name, &dest, CMD_for, &opt_flags,
					      &vimvaridx, &type, cctx) == FAIL)
		goto failed;
	    if (dest != dest_local)
	    {
		if (generate_store_var(cctx, dest, opt_flags, vimvaridx,
						     0, 0, type, name) == FAIL)
		    goto failed;
	    }
	    else if (varlen == 1 && *arg == '_')
	    {
		// Assigning to "_": drop the value.
		if (generate_instr_drop(cctx, ISN_DROP, 1) == NULL)
		    goto failed;
	    }
	    else
	    {
		// Script var is not supported.
		if (STRNCMP(name, "s:", 2) == 0)
		{
		    emsg(_(e_cannot_use_script_variable_in_for_loop));
		    goto failed;
		}

		if (!valid_varname(arg, (int)varlen, FALSE))
		    goto failed;
		if (lookup_local(arg, varlen, NULL, cctx) == OK)
		{
		    semsg(_(e_variable_already_declared), arg);
		    goto failed;
		}

		// Reserve a variable to store "var".
		where.wt_index = var_list ? idx + 1 : 0;
		where.wt_variable = TRUE;
		if (lhs_type == &t_any)
		    lhs_type = item_type;
		else if (item_type != &t_unknown
			&& need_type_where(item_type, lhs_type, -1,
					    where, cctx, FALSE, FALSE) == FAIL)
		    goto failed;
		var_lvar = reserve_local(cctx, arg, varlen, TRUE, lhs_type);
		if (var_lvar == NULL)
		    // out of memory or used as an argument
		    goto failed;

		if (semicolon && idx == var_count - 1)
		    var_lvar->lv_type = vartype;
		generate_STORE(cctx, ISN_STORE, var_lvar->lv_idx, NULL);
	    }

	    if (*p == ',' || *p == ';')
		++p;
	    arg = skipwhite(p);
	    vim_free(name);
	}
    }

    return arg_end;

failed:
    vim_free(name);
    drop_scope(cctx);
    return NULL;
}

/*
 * compile "endfor"
 */
    char_u *
compile_endfor(char_u *arg, cctx_T *cctx)
{
    garray_T	*instr = &cctx->ctx_instr;
    scope_T	*scope = cctx->ctx_scope;
    forscope_T	*forscope;
    isn_T	*isn;

    if (misplaced_cmdmod(cctx))
	return NULL;

    if (scope == NULL || scope->se_type != FOR_SCOPE)
    {
	emsg(_(e_endfor_without_for));
	return NULL;
    }
    forscope = &scope->se_u.se_for;
    cctx->ctx_scope = scope->se_outer;
    if (cctx->ctx_skip != SKIP_YES)
    {
	unwind_locals(cctx, scope->se_local_count);

	// At end of ":for" scope jump back to the FOR instruction.
	generate_JUMP(cctx, JUMP_ALWAYS, forscope->fs_top_label);

	// Fill in the "end" label in the FOR statement so it can jump here.
	// In debug mode an ISN_DEBUG was inserted.
	isn = ((isn_T *)instr->ga_data) + forscope->fs_top_label
				+ (cctx->ctx_compile_type == CT_DEBUG ? 1 : 0);
	isn->isn_arg.forloop.for_end = instr->ga_len;

	// Fill in the "end" label any BREAK statements
	compile_fill_jump_to_end(&forscope->fs_end_label, instr->ga_len, cctx);

	// Below the ":for" scope drop the "expr" list from the stack.
	if (generate_instr_drop(cctx, ISN_DROP, 1) == NULL)
	    return NULL;
    }

    vim_free(scope);

    return arg;
}

/*
 * compile "while expr"
 *
 * Produces instructions:
 * top:  EVAL expr		Push result of "expr"
 *       JUMP_IF_FALSE end	jump if false
 *       ... body ...
 *       JUMP top		Jump back to repeat
 * end:
 *
 */
    char_u *
compile_while(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    scope_T	*scope;

    scope = new_scope(cctx, WHILE_SCOPE);
    if (scope == NULL)
	return NULL;

    // "endwhile" jumps back here, one before when profiling or using cmdmods
    scope->se_u.se_while.ws_top_label = current_instr_idx(cctx);

    // compile "expr"
    if (compile_expr0(&p, cctx) == FAIL)
	return NULL;

    if (!ends_excmd2(arg, skipwhite(p)))
    {
	semsg(_(e_trailing_characters_str), p);
	return NULL;
    }

    if (cctx->ctx_skip != SKIP_YES)
    {
	if (bool_on_stack(cctx) == FAIL)
	    return FAIL;

	// CMDMOD_REV must come before the jump
	generate_undo_cmdmods(cctx);

	// "while_end" is set when ":endwhile" is found
	if (compile_jump_to_end(&scope->se_u.se_while.ws_end_label,
						  JUMP_IF_FALSE, cctx) == FAIL)
	    return FAIL;
    }

    return p;
}

/*
 * compile "endwhile"
 */
    char_u *
compile_endwhile(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    garray_T	*instr = &cctx->ctx_instr;

    if (misplaced_cmdmod(cctx))
	return NULL;
    if (scope == NULL || scope->se_type != WHILE_SCOPE)
    {
	emsg(_(e_endwhile_without_while));
	return NULL;
    }
    cctx->ctx_scope = scope->se_outer;
    if (cctx->ctx_skip != SKIP_YES)
    {
	unwind_locals(cctx, scope->se_local_count);

#ifdef FEAT_PROFILE
	// count the endwhile before jumping
	may_generate_prof_end(cctx, cctx->ctx_lnum);
#endif

	// At end of ":for" scope jump back to the FOR instruction.
	generate_JUMP(cctx, JUMP_ALWAYS, scope->se_u.se_while.ws_top_label);

	// Fill in the "end" label in the WHILE statement so it can jump here.
	// And in any jumps for ":break"
	compile_fill_jump_to_end(&scope->se_u.se_while.ws_end_label,
							  instr->ga_len, cctx);
    }

    vim_free(scope);

    return arg;
}

/*
 * compile "continue"
 */
    char_u *
compile_continue(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    int		try_scopes = 0;
    int		loop_label;

    for (;;)
    {
	if (scope == NULL)
	{
	    emsg(_(e_continue_without_while_or_for));
	    return NULL;
	}
	if (scope->se_type == FOR_SCOPE)
	{
	    loop_label = scope->se_u.se_for.fs_top_label;
	    break;
	}
	if (scope->se_type == WHILE_SCOPE)
	{
	    loop_label = scope->se_u.se_while.ws_top_label;
	    break;
	}
	if (scope->se_type == TRY_SCOPE)
	    ++try_scopes;
	scope = scope->se_outer;
    }

    if (try_scopes > 0)
	// Inside one or more try/catch blocks we first need to jump to the
	// "finally" or "endtry" to cleanup.
	generate_TRYCONT(cctx, try_scopes, loop_label);
    else
	// Jump back to the FOR or WHILE instruction.
	generate_JUMP(cctx, JUMP_ALWAYS, loop_label);

    return arg;
}

/*
 * compile "break"
 */
    char_u *
compile_break(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    int		try_scopes = 0;
    endlabel_T	**el;

    for (;;)
    {
	if (scope == NULL)
	{
	    emsg(_(e_break_without_while_or_for));
	    return NULL;
	}
	if (scope->se_type == FOR_SCOPE)
	{
	    el = &scope->se_u.se_for.fs_end_label;
	    break;
	}
	if (scope->se_type == WHILE_SCOPE)
	{
	    el = &scope->se_u.se_while.ws_end_label;
	    break;
	}
	if (scope->se_type == TRY_SCOPE)
	    ++try_scopes;
	scope = scope->se_outer;
    }

    if (try_scopes > 0)
	// Inside one or more try/catch blocks we first need to jump to the
	// "finally" or "endtry" to cleanup.  Then come to the next JUMP
	// intruction, which we don't know the index of yet.
	generate_TRYCONT(cctx, try_scopes, cctx->ctx_instr.ga_len + 1);

    // Jump to the end of the FOR or WHILE loop.  The instruction index will be
    // filled in later.
    if (compile_jump_to_end(el, JUMP_ALWAYS, cctx) == FAIL)
	return FAIL;

    return arg;
}

/*
 * compile "{" start of block
 */
    char_u *
compile_block(char_u *arg, cctx_T *cctx)
{
    if (new_scope(cctx, BLOCK_SCOPE) == NULL)
	return NULL;
    return skipwhite(arg + 1);
}

/*
 * compile end of block: drop one scope
 */
    void
compile_endblock(cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;

    cctx->ctx_scope = scope->se_outer;
    unwind_locals(cctx, scope->se_local_count);
    vim_free(scope);
}

/*
 * Compile "try".
 * Creates a new scope for the try-endtry, pointing to the first catch and
 * finally.
 * Creates another scope for the "try" block itself.
 * TRY instruction sets up exception handling at runtime.
 *
 *	"try"
 *	    TRY -> catch1, -> finally  push trystack entry
 *	    ... try block
 *	"throw {exception}"
 *	    EVAL {exception}
 *	    THROW		create exception
 *	    ... try block
 *	" catch {expr}"
 *	    JUMP -> finally
 * catch1:  PUSH exception
 *	    EVAL {expr}
 *	    MATCH
 *	    JUMP nomatch -> catch2
 *	    CATCH   remove exception
 *	    ... catch block
 *	" catch"
 *	    JUMP -> finally
 * catch2:  CATCH   remove exception
 *	    ... catch block
 *	" finally"
 * finally:
 *	    ... finally block
 *	" endtry"
 *	    ENDTRY  pop trystack entry, may rethrow
 */
    char_u *
compile_try(char_u *arg, cctx_T *cctx)
{
    garray_T	*instr = &cctx->ctx_instr;
    scope_T	*try_scope;
    scope_T	*scope;

    if (misplaced_cmdmod(cctx))
	return NULL;

    // scope that holds the jumps that go to catch/finally/endtry
    try_scope = new_scope(cctx, TRY_SCOPE);
    if (try_scope == NULL)
	return NULL;

    if (cctx->ctx_skip != SKIP_YES)
    {
	isn_T	*isn;

	// "try_catch" is set when the first ":catch" is found or when no catch
	// is found and ":finally" is found.
	// "try_finally" is set when ":finally" is found
	// "try_endtry" is set when ":endtry" is found
	try_scope->se_u.se_try.ts_try_label = instr->ga_len;
	if ((isn = generate_instr(cctx, ISN_TRY)) == NULL)
	    return NULL;
	isn->isn_arg.tryref.try_ref = ALLOC_CLEAR_ONE(tryref_T);
	if (isn->isn_arg.tryref.try_ref == NULL)
	    return NULL;
    }

    // scope for the try block itself
    scope = new_scope(cctx, BLOCK_SCOPE);
    if (scope == NULL)
	return NULL;

    return arg;
}

/*
 * Compile "catch {expr}".
 */
    char_u *
compile_catch(char_u *arg, cctx_T *cctx UNUSED)
{
    scope_T	*scope = cctx->ctx_scope;
    garray_T	*instr = &cctx->ctx_instr;
    char_u	*p;
    isn_T	*isn;

    if (misplaced_cmdmod(cctx))
	return NULL;

    // end block scope from :try or :catch
    if (scope != NULL && scope->se_type == BLOCK_SCOPE)
	compile_endblock(cctx);
    scope = cctx->ctx_scope;

    // Error if not in a :try scope
    if (scope == NULL || scope->se_type != TRY_SCOPE)
    {
	emsg(_(e_catch_without_try));
	return NULL;
    }

    if (scope->se_u.se_try.ts_caught_all)
    {
	emsg(_(e_catch_unreachable_after_catch_all));
	return NULL;
    }
    if (!cctx->ctx_had_return)
	scope->se_u.se_try.ts_no_return = TRUE;

    if (cctx->ctx_skip != SKIP_YES)
    {
#ifdef FEAT_PROFILE
	// the profile-start should be after the jump
	if (cctx->ctx_compile_type == CT_PROFILE
		&& instr->ga_len > 0
		&& ((isn_T *)instr->ga_data)[instr->ga_len - 1]
						   .isn_type == ISN_PROF_START)
	    --instr->ga_len;
#endif
	// Jump from end of previous block to :finally or :endtry
	if (compile_jump_to_end(&scope->se_u.se_try.ts_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	    return NULL;

	// End :try or :catch scope: set value in ISN_TRY instruction
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_try_label;
	if (isn->isn_arg.tryref.try_ref->try_catch == 0)
	    isn->isn_arg.tryref.try_ref->try_catch = instr->ga_len;
	if (scope->se_u.se_try.ts_catch_label != 0)
	{
	    // Previous catch without match jumps here
	    isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_catch_label;
	    isn->isn_arg.jump.jump_where = instr->ga_len;
	}
#ifdef FEAT_PROFILE
	if (cctx->ctx_compile_type == CT_PROFILE)
	{
	    // a "throw" that jumps here needs to be counted
	    generate_instr(cctx, ISN_PROF_END);
	    // the "catch" is also counted
	    generate_instr(cctx, ISN_PROF_START);
	}
#endif
	if (cctx->ctx_compile_type == CT_DEBUG)
	    generate_instr_debug(cctx);
    }

    p = skipwhite(arg);
    if (ends_excmd2(arg, p))
    {
	scope->se_u.se_try.ts_caught_all = TRUE;
	scope->se_u.se_try.ts_catch_label = 0;
    }
    else
    {
	char_u *end;
	char_u *pat;
	char_u *tofree = NULL;
	int	dropped = 0;
	int	len;

	// Push v:exception, push {expr} and MATCH
	generate_instr_type(cctx, ISN_PUSHEXC, &t_string);

	end = skip_regexp_ex(p + 1, *p, TRUE, &tofree, &dropped, NULL);
	if (*end != *p)
	{
	    semsg(_(e_separator_mismatch_str), p);
	    vim_free(tofree);
	    return NULL;
	}
	if (tofree == NULL)
	    len = (int)(end - (p + 1));
	else
	    len = (int)(end - tofree);
	pat = vim_strnsave(tofree == NULL ? p + 1 : tofree, len);
	vim_free(tofree);
	p += len + 2 + dropped;
	if (pat == NULL)
	    return NULL;
	if (generate_PUSHS(cctx, &pat) == FAIL)
	    return NULL;

	if (generate_COMPARE(cctx, EXPR_MATCH, FALSE) == FAIL)
	    return NULL;

	scope->se_u.se_try.ts_catch_label = instr->ga_len;
	if (generate_JUMP(cctx, JUMP_IF_FALSE, 0) == FAIL)
	    return NULL;
    }

    if (cctx->ctx_skip != SKIP_YES && generate_instr(cctx, ISN_CATCH) == NULL)
	return NULL;

    if (new_scope(cctx, BLOCK_SCOPE) == NULL)
	return NULL;
    return p;
}

    char_u *
compile_finally(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*isn;
    int		this_instr;

    if (misplaced_cmdmod(cctx))
	return NULL;

    // end block scope from :try or :catch
    if (scope != NULL && scope->se_type == BLOCK_SCOPE)
	compile_endblock(cctx);
    scope = cctx->ctx_scope;

    // Error if not in a :try scope
    if (scope == NULL || scope->se_type != TRY_SCOPE)
    {
	emsg(_(e_finally_without_try));
	return NULL;
    }

    if (cctx->ctx_skip != SKIP_YES)
    {
	// End :catch or :finally scope: set value in ISN_TRY instruction
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_try_label;
	if (isn->isn_arg.tryref.try_ref->try_finally != 0)
	{
	    emsg(_(e_multiple_finally));
	    return NULL;
	}

	this_instr = instr->ga_len;
#ifdef FEAT_PROFILE
	if (cctx->ctx_compile_type == CT_PROFILE
		&& ((isn_T *)instr->ga_data)[this_instr - 1]
						   .isn_type == ISN_PROF_START)
	{
	    // jump to the profile start of the "finally"
	    --this_instr;

	    // jump to the profile end above it
	    if (this_instr > 0 && ((isn_T *)instr->ga_data)[this_instr - 1]
						     .isn_type == ISN_PROF_END)
		--this_instr;
	}
#endif

	// Fill in the "end" label in jumps at the end of the blocks.
	compile_fill_jump_to_end(&scope->se_u.se_try.ts_end_label,
							     this_instr, cctx);

	// If there is no :catch then an exception jumps to :finally.
	if (isn->isn_arg.tryref.try_ref->try_catch == 0)
	    isn->isn_arg.tryref.try_ref->try_catch = this_instr;
	isn->isn_arg.tryref.try_ref->try_finally = this_instr;
	if (scope->se_u.se_try.ts_catch_label != 0)
	{
	    // Previous catch without match jumps here
	    isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_catch_label;
	    isn->isn_arg.jump.jump_where = this_instr;
	    scope->se_u.se_try.ts_catch_label = 0;
	}
	scope->se_u.se_try.ts_has_finally = TRUE;
	if (generate_instr(cctx, ISN_FINALLY) == NULL)
	    return NULL;
    }

    return arg;
}

    char_u *
compile_endtry(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*try_isn;

    if (misplaced_cmdmod(cctx))
	return NULL;

    // end block scope from :catch or :finally
    if (scope != NULL && scope->se_type == BLOCK_SCOPE)
	compile_endblock(cctx);
    scope = cctx->ctx_scope;

    // Error if not in a :try scope
    if (scope == NULL || scope->se_type != TRY_SCOPE)
    {
	if (scope == NULL)
	    emsg(_(e_endtry_without_try));
	else if (scope->se_type == WHILE_SCOPE)
	    emsg(_(e_missing_endwhile));
	else if (scope->se_type == FOR_SCOPE)
	    emsg(_(e_missing_endfor));
	else
	    emsg(_(e_missing_endif));
	return NULL;
    }

    try_isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_try_label;
    if (cctx->ctx_skip != SKIP_YES)
    {
	if (try_isn->isn_arg.tryref.try_ref->try_catch == 0
			  && try_isn->isn_arg.tryref.try_ref->try_finally == 0)
	{
	    emsg(_(e_missing_catch_or_finally));
	    return NULL;
	}

#ifdef FEAT_PROFILE
	if (cctx->ctx_compile_type == CT_PROFILE
		&& ((isn_T *)instr->ga_data)[instr->ga_len - 1]
						.isn_type == ISN_PROF_START)
	    // move the profile start after "endtry" so that it's not counted when
	    // the exception is rethrown.
	    --instr->ga_len;
#endif

	// Fill in the "end" label in jumps at the end of the blocks, if not
	// done by ":finally".
	compile_fill_jump_to_end(&scope->se_u.se_try.ts_end_label,
							  instr->ga_len, cctx);

	if (scope->se_u.se_try.ts_catch_label != 0)
	{
	    // Last catch without match jumps here
	    isn_T *isn = ((isn_T *)instr->ga_data)
					   + scope->se_u.se_try.ts_catch_label;
	    isn->isn_arg.jump.jump_where = instr->ga_len;
	}
    }

    // If there is a finally clause that ends in return then we will return.
    // If one of the blocks didn't end in "return" or we did not catch all
    // exceptions reset the had_return flag.
    if (!(scope->se_u.se_try.ts_has_finally && cctx->ctx_had_return)
	    && (scope->se_u.se_try.ts_no_return
		|| !scope->se_u.se_try.ts_caught_all))
	cctx->ctx_had_return = FALSE;

    compile_endblock(cctx);

    if (cctx->ctx_skip != SKIP_YES)
    {
	// End :catch or :finally scope: set instruction index in ISN_TRY
	// instruction
	try_isn->isn_arg.tryref.try_ref->try_endtry = instr->ga_len;
	if (generate_instr(cctx, ISN_ENDTRY) == NULL)
	    return NULL;
#ifdef FEAT_PROFILE
	if (cctx->ctx_compile_type == CT_PROFILE)
	    generate_instr(cctx, ISN_PROF_START);
#endif
    }
    return arg;
}

/*
 * compile "throw {expr}"
 */
    char_u *
compile_throw(char_u *arg, cctx_T *cctx UNUSED)
{
    char_u *p = skipwhite(arg);

    if (compile_expr0(&p, cctx) == FAIL)
	return NULL;
    if (cctx->ctx_skip == SKIP_YES)
	return p;
    if (may_generate_2STRING(-1, FALSE, cctx) == FAIL)
	return NULL;
    if (generate_instr_drop(cctx, ISN_THROW, 1) == NULL)
	return NULL;

    return p;
}

    char_u *
compile_eval(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    int		name_only;
    long	lnum = SOURCING_LNUM;

    // find_ex_command() will consider a variable name an expression, assuming
    // that something follows on the next line.  Check that something actually
    // follows, otherwise it's probably a misplaced command.
    name_only = cmd_is_name_only(arg);

    if (compile_expr0(&p, cctx) == FAIL)
	return NULL;

    if (name_only && lnum == SOURCING_LNUM)
    {
	semsg(_(e_expression_without_effect_str), arg);
	return NULL;
    }

    // drop the result
    generate_instr_drop(cctx, ISN_DROP, 1);

    return skipwhite(p);
}

/*
 * compile "echo expr"
 * compile "echomsg expr"
 * compile "echoerr expr"
 * compile "echoconsole expr"
 * compile "execute expr"
 */
    char_u *
compile_mult_expr(char_u *arg, int cmdidx, cctx_T *cctx)
{
    char_u	*p = arg;
    char_u	*prev = arg;
    char_u	*expr_start;
    int		count = 0;
    int		start_ctx_lnum = cctx->ctx_lnum;
    type_T	*type;

    for (;;)
    {
	if (ends_excmd2(prev, p))
	    break;
	expr_start = p;
	if (compile_expr0(&p, cctx) == FAIL)
	    return NULL;

	if (cctx->ctx_skip != SKIP_YES)
	{
	    // check for non-void type
	    type = get_type_on_stack(cctx, 0);
	    if (type->tt_type == VAR_VOID)
	    {
		semsg(_(e_expression_does_not_result_in_value_str), expr_start);
		return NULL;
	    }
	}

	++count;
	prev = p;
	p = skipwhite(p);
    }

    if (count > 0)
    {
	long save_lnum = cctx->ctx_lnum;

	// Use the line number where the command started.
	cctx->ctx_lnum = start_ctx_lnum;

	if (cmdidx == CMD_echo || cmdidx == CMD_echon)
	    generate_ECHO(cctx, cmdidx == CMD_echo, count);
	else if (cmdidx == CMD_execute)
	    generate_MULT_EXPR(cctx, ISN_EXECUTE, count);
	else if (cmdidx == CMD_echomsg)
	    generate_MULT_EXPR(cctx, ISN_ECHOMSG, count);
	else if (cmdidx == CMD_echoconsole)
	    generate_MULT_EXPR(cctx, ISN_ECHOCONSOLE, count);
	else
	    generate_MULT_EXPR(cctx, ISN_ECHOERR, count);

	cctx->ctx_lnum = save_lnum;
    }
    return p;
}

/*
 * If "eap" has a range that is not a constant generate an ISN_RANGE
 * instruction to compute it and return OK.
 * Otherwise return FAIL, the caller must deal with any range.
 */
    static int
compile_variable_range(exarg_T *eap, cctx_T *cctx)
{
    char_u *range_end = skip_range(eap->cmd, TRUE, NULL);
    char_u *p = skipdigits(eap->cmd);

    if (p == range_end)
	return FAIL;
    return generate_RANGE(cctx, vim_strnsave(eap->cmd, range_end - eap->cmd));
}

/*
 * :put r
 * :put ={expr}
 */
    char_u *
compile_put(char_u *arg, exarg_T *eap, cctx_T *cctx)
{
    char_u	*line = arg;
    linenr_T	lnum;
    char	*errormsg;
    int		above = eap->forceit;

    eap->regname = *line;

    if (eap->regname == '=')
    {
	char_u *p = skipwhite(line + 1);

	if (compile_expr0(&p, cctx) == FAIL)
	    return NULL;
	line = p;
    }
    else if (eap->regname != NUL)
	++line;

    if (compile_variable_range(eap, cctx) == OK)
    {
	lnum = above ? LNUM_VARIABLE_RANGE_ABOVE : LNUM_VARIABLE_RANGE;
    }
    else
    {
	// Either no range or a number.
	// "errormsg" will not be set because the range is ADDR_LINES.
	if (parse_cmd_address(eap, &errormsg, FALSE) == FAIL)
	    // cannot happen
	    return NULL;
	if (eap->addr_count == 0)
	    lnum = -1;
	else
	    lnum = eap->line2;
	if (above)
	    --lnum;
    }

    generate_PUT(cctx, eap->regname, lnum);
    return line;
}

/*
 * A command that is not compiled, execute with legacy code.
 */
    char_u *
compile_exec(char_u *line_arg, exarg_T *eap, cctx_T *cctx)
{
    char_u	*line = line_arg;
    char_u	*p;
    int		has_expr = FALSE;
    char_u	*nextcmd = (char_u *)"";
    char_u	*tofree = NULL;
    char_u	*cmd_arg = NULL;

    if (cctx->ctx_skip == SKIP_YES)
	goto theend;

    // If there was a prececing command modifier, drop it and include it in the
    // EXEC command.
    if (cctx->ctx_has_cmdmod)
    {
	garray_T	*instr = &cctx->ctx_instr;
	isn_T		*isn = ((isn_T *)instr->ga_data) + instr->ga_len - 1;

	if (isn->isn_type == ISN_CMDMOD)
	{
	    vim_regfree(isn->isn_arg.cmdmod.cf_cmdmod
					       ->cmod_filter_regmatch.regprog);
	    vim_free(isn->isn_arg.cmdmod.cf_cmdmod);
	    --instr->ga_len;
	    cctx->ctx_has_cmdmod = FALSE;
	}
    }

    if (eap->cmdidx >= 0 && eap->cmdidx < CMD_SIZE)
    {
	long	argt = eap->argt;
	int	usefilter = FALSE;

	has_expr = argt & (EX_XFILE | EX_EXPAND);

	// If the command can be followed by a bar, find the bar and truncate
	// it, so that the following command can be compiled.
	// The '|' is overwritten with a NUL, it is put back below.
	if ((eap->cmdidx == CMD_write || eap->cmdidx == CMD_read)
							   && *eap->arg == '!')
	    // :w !filter or :r !filter or :r! filter
	    usefilter = TRUE;
	if ((argt & EX_TRLBAR) && !usefilter)
	{
	    eap->argt = argt;
	    separate_nextcmd(eap, TRUE);
	    if (eap->nextcmd != NULL)
		nextcmd = eap->nextcmd;
	}
	else if (eap->cmdidx == CMD_wincmd)
	{
	    p = eap->arg;
	    if (*p != NUL)
		++p;
	    if (*p == 'g' || *p == Ctrl_G)
		++p;
	    p = skipwhite(p);
	    if (*p == '|')
	    {
		*p = NUL;
		nextcmd = p + 1;
	    }
	}
	else if (eap->cmdidx == CMD_command || eap->cmdidx == CMD_autocmd)
	{
	    // If there is a trailing '{' read lines until the '}'
	    p = eap->arg + STRLEN(eap->arg) - 1;
	    while (p > eap->arg && VIM_ISWHITE(*p))
		--p;
	    if (*p == '{')
	    {
		exarg_T ea;
		int	flags = 0;  // unused
		int	start_lnum = SOURCING_LNUM;

		CLEAR_FIELD(ea);
		ea.arg = eap->arg;
		fill_exarg_from_cctx(&ea, cctx);
		(void)may_get_cmd_block(&ea, p, &tofree, &flags);
		if (tofree != NULL)
		{
		    *p = NUL;
		    line = concat_str(line, tofree);
		    if (line == NULL)
			goto theend;
		    vim_free(tofree);
		    tofree = line;
		    SOURCING_LNUM = start_lnum;
		}
	    }
	}
    }

    if (eap->cmdidx == CMD_syntax && STRNCMP(eap->arg, "include ", 8) == 0)
    {
	// expand filename in "syntax include [@group] filename"
	has_expr = TRUE;
	eap->arg = skipwhite(eap->arg + 7);
	if (*eap->arg == '@')
	    eap->arg = skiptowhite(eap->arg);
    }

    if ((eap->cmdidx == CMD_global || eap->cmdidx == CMD_vglobal)
						       && STRLEN(eap->arg) > 4)
    {
	int delim = *eap->arg;

	p = skip_regexp_ex(eap->arg + 1, delim, TRUE, NULL, NULL, NULL);
	if (*p == delim)
	    cmd_arg = p + 1;
    }

    if (eap->cmdidx == CMD_folddoopen || eap->cmdidx == CMD_folddoclosed)
	cmd_arg = eap->arg;

    if (cmd_arg != NULL)
    {
	exarg_T nea;

	CLEAR_FIELD(nea);
	nea.cmd = cmd_arg;
	p = find_ex_command(&nea, NULL, lookup_scriptitem, NULL);
	if (nea.cmdidx < CMD_SIZE)
	{
	    has_expr = excmd_get_argt(nea.cmdidx) & (EX_XFILE | EX_EXPAND);
	    if (has_expr)
		eap->arg = skiptowhite(eap->arg);
	}
    }

    if (has_expr && (p = (char_u *)strstr((char *)eap->arg, "`=")) != NULL)
    {
	int	count = 0;
	char_u	*start = skipwhite(line);

	// :cmd xxx`=expr1`yyy`=expr2`zzz
	// PUSHS ":cmd xxx"
	// eval expr1
	// PUSHS "yyy"
	// eval expr2
	// PUSHS "zzz"
	// EXECCONCAT 5
	for (;;)
	{
	    if (p > start)
	    {
		char_u *val = vim_strnsave(start, p - start);

		generate_PUSHS(cctx, &val);
		++count;
	    }
	    p += 2;
	    if (compile_expr0(&p, cctx) == FAIL)
		return NULL;
	    may_generate_2STRING(-1, TRUE, cctx);
	    ++count;
	    p = skipwhite(p);
	    if (*p != '`')
	    {
		emsg(_(e_missing_backtick));
		return NULL;
	    }
	    start = p + 1;

	    p = (char_u *)strstr((char *)start, "`=");
	    if (p == NULL)
	    {
		if (*skipwhite(start) != NUL)
		{
		    char_u *val = vim_strsave(start);

		    generate_PUSHS(cctx, &val);
		    ++count;
		}
		break;
	    }
	}
	generate_EXECCONCAT(cctx, count);
    }
    else
	generate_EXEC_copy(cctx, ISN_EXEC, line);

theend:
    if (*nextcmd != NUL)
    {
	// the parser expects a pointer to the bar, put it back
	--nextcmd;
	*nextcmd = '|';
    }
    vim_free(tofree);

    return nextcmd;
}

/*
 * A script command with heredoc, e.g.
 *	ruby << EOF
 *	   command
 *	EOF
 * Has been turned into one long line with NL characters by
 * get_function_body():
 *	ruby << EOF<NL>   command<NL>EOF
 */
    char_u *
compile_script(char_u *line, cctx_T *cctx)
{
    if (cctx->ctx_skip != SKIP_YES)
    {
	isn_T	*isn;

	if ((isn = generate_instr(cctx, ISN_EXEC_SPLIT)) == NULL)
	    return NULL;
	isn->isn_arg.string = vim_strsave(line);
    }
    return (char_u *)"";
}


/*
 * :s/pat/repl/
 */
    char_u *
compile_substitute(char_u *arg, exarg_T *eap, cctx_T *cctx)
{
    char_u  *cmd = eap->arg;
    char_u  *expr = (char_u *)strstr((char *)cmd, "\\=");

    if (expr != NULL)
    {
	int delimiter = *cmd++;

	// There is a \=expr, find it in the substitute part.
	cmd = skip_regexp_ex(cmd, delimiter, magic_isset(), NULL, NULL, NULL);
	if (cmd[0] == delimiter && cmd[1] == '\\' && cmd[2] == '=')
	{
	    garray_T	save_ga = cctx->ctx_instr;
	    char_u	*end;
	    int		expr_res;
	    int		trailing_error;
	    int		instr_count;
	    isn_T	*instr;
	    isn_T	*isn;

	    cmd += 3;
	    end = skip_substitute(cmd, delimiter);

	    // Temporarily reset the list of instructions so that the jump
	    // labels are correct.
	    cctx->ctx_instr.ga_len = 0;
	    cctx->ctx_instr.ga_maxlen = 0;
	    cctx->ctx_instr.ga_data = NULL;
	    expr_res = compile_expr0(&cmd, cctx);
	    if (end[-1] == NUL)
		end[-1] = delimiter;
	    cmd = skipwhite(cmd);
	    trailing_error = *cmd != delimiter && *cmd != NUL;

	    if (expr_res == FAIL || trailing_error
				       || GA_GROW_FAILS(&cctx->ctx_instr, 1))
	    {
		if (trailing_error)
		    semsg(_(e_trailing_characters_str), cmd);
		clear_instr_ga(&cctx->ctx_instr);
		cctx->ctx_instr = save_ga;
		return NULL;
	    }

	    // Move the generated instructions into the ISN_SUBSTITUTE
	    // instructions, then restore the list of instructions before
	    // adding the ISN_SUBSTITUTE instruction.
	    instr_count = cctx->ctx_instr.ga_len;
	    instr = cctx->ctx_instr.ga_data;
	    instr[instr_count].isn_type = ISN_FINISH;

	    cctx->ctx_instr = save_ga;
	    if ((isn = generate_instr(cctx, ISN_SUBSTITUTE)) == NULL)
	    {
		int idx;

		for (idx = 0; idx < instr_count; ++idx)
		    delete_instr(instr + idx);
		vim_free(instr);
		return NULL;
	    }
	    isn->isn_arg.subs.subs_cmd = vim_strsave(arg);
	    isn->isn_arg.subs.subs_instr = instr;

	    // skip over flags
	    if (*end == '&')
		++end;
	    while (ASCII_ISALPHA(*end) || *end == '#')
		++end;
	    return end;
	}
    }

    return compile_exec(arg, eap, cctx);
}

    char_u *
compile_redir(char_u *line, exarg_T *eap, cctx_T *cctx)
{
    char_u  *arg = eap->arg;
    lhs_T   *lhs = &cctx->ctx_redir_lhs;

    if (lhs->lhs_name != NULL)
    {
	if (STRNCMP(arg, "END", 3) == 0)
	{
	    if (lhs->lhs_append)
	    {
		// First load the current variable value.
		if (compile_load_lhs_with_index(lhs, lhs->lhs_whole,
								 cctx) == FAIL)
		    return NULL;
	    }

	    // Gets the redirected text and put it on the stack, then store it
	    // in the variable.
	    generate_instr_type(cctx, ISN_REDIREND, &t_string);

	    if (lhs->lhs_append)
		generate_CONCAT(cctx, 2);

	    if (lhs->lhs_has_index)
	    {
		// Use the info in "lhs" to store the value at the index in the
		// list or dict.
		if (compile_assign_unlet(lhs->lhs_whole, lhs, TRUE,
						      &t_string, cctx) == FAIL)
		    return NULL;
	    }
	    else if (generate_store_lhs(cctx, lhs, -1, FALSE) == FAIL)
		return NULL;

	    VIM_CLEAR(lhs->lhs_name);
	    VIM_CLEAR(lhs->lhs_whole);
	    return arg + 3;
	}
	emsg(_(e_cannot_nest_redir));
	return NULL;
    }

    if (arg[0] == '=' && arg[1] == '>')
    {
	int	    append = FALSE;

	// redirect to a variable is compiled
	arg += 2;
	if (*arg == '>')
	{
	    ++arg;
	    append = TRUE;
	}
	arg = skipwhite(arg);

	if (compile_assign_lhs(arg, lhs, CMD_redir,
					 FALSE, FALSE, FALSE, 1, cctx) == FAIL)
	    return NULL;
	if (need_type(&t_string, lhs->lhs_member_type,
					    -1, 0, cctx, FALSE, FALSE) == FAIL)
	    return NULL;
	generate_instr(cctx, ISN_REDIRSTART);
	lhs->lhs_append = append;
	if (lhs->lhs_has_index)
	{
	    lhs->lhs_whole = vim_strnsave(arg, lhs->lhs_varlen_total);
	    if (lhs->lhs_whole == NULL)
		return NULL;
	}

	return arg + lhs->lhs_varlen_total;
    }

    // other redirects are handled like at script level
    return compile_exec(line, eap, cctx);
}

#if defined(FEAT_QUICKFIX) || defined(PROTO)
    char_u *
compile_cexpr(char_u *line, exarg_T *eap, cctx_T *cctx)
{
    isn_T	*isn;
    char_u	*p;

    isn = generate_instr(cctx, ISN_CEXPR_AUCMD);
    if (isn == NULL)
	return NULL;
    isn->isn_arg.number = eap->cmdidx;

    p = eap->arg;
    if (compile_expr0(&p, cctx) == FAIL)
	return NULL;

    isn = generate_instr(cctx, ISN_CEXPR_CORE);
    if (isn == NULL)
	return NULL;
    isn->isn_arg.cexpr.cexpr_ref = ALLOC_ONE(cexprref_T);
    if (isn->isn_arg.cexpr.cexpr_ref == NULL)
	return NULL;
    isn->isn_arg.cexpr.cexpr_ref->cer_cmdidx = eap->cmdidx;
    isn->isn_arg.cexpr.cexpr_ref->cer_forceit = eap->forceit;
    isn->isn_arg.cexpr.cexpr_ref->cer_cmdline = vim_strsave(skipwhite(line));

    return p;
}
#endif

/*
 * Compile "return [expr]".
 * When "legacy" is TRUE evaluate [expr] with legacy syntax
 */
    char_u *
compile_return(char_u *arg, int check_return_type, int legacy, cctx_T *cctx)
{
    char_u	*p = arg;
    type_T	*stack_type;

    if (*p != NUL && *p != '|' && *p != '\n')
    {
	// For a lambda, "return expr" is always used, also when "expr" results
	// in a void.
	if (cctx->ctx_ufunc->uf_ret_type->tt_type == VAR_VOID
		&& (cctx->ctx_ufunc->uf_flags & FC_LAMBDA) == 0)
	{
	    emsg(_(e_returning_value_in_function_without_return_type));
	    return NULL;
	}
	if (legacy)
	{
	    int save_flags = cmdmod.cmod_flags;

	    generate_LEGACY_EVAL(cctx, p);
	    if (need_type(&t_any, cctx->ctx_ufunc->uf_ret_type, -1,
						0, cctx, FALSE, FALSE) == FAIL)
		return NULL;
	    cmdmod.cmod_flags |= CMOD_LEGACY;
	    (void)skip_expr(&p, NULL);
	    cmdmod.cmod_flags = save_flags;
	}
	else
	{
	    // compile return argument into instructions
	    if (compile_expr0(&p, cctx) == FAIL)
		return NULL;
	}

	if (cctx->ctx_skip != SKIP_YES)
	{
	    // "check_return_type" with uf_ret_type set to &t_unknown is used
	    // for an inline function without a specified return type.  Set the
	    // return type here.
	    stack_type = get_type_on_stack(cctx, 0);
	    if ((check_return_type && (cctx->ctx_ufunc->uf_ret_type == NULL
				|| cctx->ctx_ufunc->uf_ret_type == &t_unknown))
		    || (!check_return_type
				&& cctx->ctx_ufunc->uf_ret_type == &t_unknown))
	    {
		cctx->ctx_ufunc->uf_ret_type = stack_type;
	    }
	    else
	    {
		if (need_type(stack_type, cctx->ctx_ufunc->uf_ret_type, -1,
						0, cctx, FALSE, FALSE) == FAIL)
		    return NULL;
	    }
	}
    }
    else
    {
	// "check_return_type" cannot be TRUE, only used for a lambda which
	// always has an argument.
	if (cctx->ctx_ufunc->uf_ret_type->tt_type != VAR_VOID
		&& cctx->ctx_ufunc->uf_ret_type->tt_type != VAR_UNKNOWN)
	{
	    emsg(_(e_missing_return_value));
	    return NULL;
	}

	// No argument, return zero.
	generate_PUSHNR(cctx, 0);
    }

    // Undo any command modifiers.
    generate_undo_cmdmods(cctx);

    if (cctx->ctx_skip != SKIP_YES && generate_instr(cctx, ISN_RETURN) == NULL)
	return NULL;

    // "return val | endif" is possible
    return skipwhite(p);
}

/*
 * Check if the separator for a :global or :substitute command is OK.
 */
    int
check_global_and_subst(char_u *cmd, char_u *arg)
{
    if (arg == cmd + 1 && vim_strchr((char_u *)":-.", *arg) != NULL)
    {
	semsg(_(e_separator_not_supported_str), arg);
	return FAIL;
    }
    if (VIM_ISWHITE(cmd[1]))
    {
	semsg(_(e_no_white_space_allowed_before_separator_str), cmd);
	return FAIL;
    }
    return OK;
}


#endif  // defined(FEAT_EVAL)
