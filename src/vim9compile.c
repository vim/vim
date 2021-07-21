/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9compile.c: :def and dealing with instructions
 */

#define USING_FLOAT_STUFF
#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

#ifdef VMS
# include <float.h>
#endif

#define DEFINE_VIM9_GLOBALS
#include "vim9.h"

// values for ctx_skip
typedef enum {
    SKIP_NOT,		// condition is a constant, produce code
    SKIP_YES,		// condition is a constant, do NOT produce code
    SKIP_UNKNOWN	// condition is not a constant, produce code
} skip_T;

/*
 * Chain of jump instructions where the end label needs to be set.
 */
typedef struct endlabel_S endlabel_T;
struct endlabel_S {
    endlabel_T	*el_next;	    // chain end_label locations
    int		el_end_label;	    // instruction idx where to set end
};

/*
 * info specific for the scope of :if / elseif / else
 */
typedef struct {
    int		is_seen_else;
    int		is_seen_skip_not;   // a block was unconditionally executed
    int		is_had_return;	    // every block ends in :return
    int		is_if_label;	    // instruction idx at IF or ELSEIF
    endlabel_T	*is_end_label;	    // instructions to set end label
} ifscope_T;

/*
 * info specific for the scope of :while
 */
typedef struct {
    int		ws_top_label;	    // instruction idx at WHILE
    endlabel_T	*ws_end_label;	    // instructions to set end
} whilescope_T;

/*
 * info specific for the scope of :for
 */
typedef struct {
    int		fs_top_label;	    // instruction idx at FOR
    endlabel_T	*fs_end_label;	    // break instructions
} forscope_T;

/*
 * info specific for the scope of :try
 */
typedef struct {
    int		ts_try_label;	    // instruction idx at TRY
    endlabel_T	*ts_end_label;	    // jump to :finally or :endtry
    int		ts_catch_label;	    // instruction idx of last CATCH
    int		ts_caught_all;	    // "catch" without argument encountered
} tryscope_T;

typedef enum {
    NO_SCOPE,
    IF_SCOPE,
    WHILE_SCOPE,
    FOR_SCOPE,
    TRY_SCOPE,
    BLOCK_SCOPE
} scopetype_T;

/*
 * Info for one scope, pointed to by "ctx_scope".
 */
typedef struct scope_S scope_T;
struct scope_S {
    scope_T	*se_outer;	    // scope containing this one
    scopetype_T se_type;
    int		se_local_count;	    // ctx_locals.ga_len before scope
    skip_T	se_skip_save;	    // ctx_skip before the block
    union {
	ifscope_T	se_if;
	whilescope_T	se_while;
	forscope_T	se_for;
	tryscope_T	se_try;
    } se_u;
};

/*
 * Entry for "ctx_locals".  Used for arguments and local variables.
 */
typedef struct {
    char_u	*lv_name;
    type_T	*lv_type;
    int		lv_idx;		// index of the variable on the stack
    int		lv_from_outer;	// nesting level, using ctx_outer scope
    int		lv_const;	// when TRUE cannot be assigned to
    int		lv_arg;		// when TRUE this is an argument
} lvar_T;

// Destination for an assignment or ":unlet" with an index.
typedef enum {
    dest_local,
    dest_option,
    dest_env,
    dest_global,
    dest_buffer,
    dest_window,
    dest_tab,
    dest_vimvar,
    dest_script,
    dest_reg,
    dest_expr,
} assign_dest_T;

// Used by compile_lhs() to store information about the LHS of an assignment
// and one argument of ":unlet" with an index.
typedef struct {
    assign_dest_T   lhs_dest;	    // type of destination

    char_u	    *lhs_name;	    // allocated name excluding the last
				    // "[expr]" or ".name".
    size_t	    lhs_varlen;	    // length of the variable without
				    // "[expr]" or ".name"
    char_u	    *lhs_whole;	    // allocated name including the last
				    // "[expr]" or ".name" for :redir
    size_t	    lhs_varlen_total; // length of the variable including
				      // any "[expr]" or ".name"
    char_u	    *lhs_dest_end;  // end of the destination, including
				    // "[expr]" or ".name".

    int		    lhs_has_index;  // has "[expr]" or ".name"

    int		    lhs_new_local;  // create new local variable
    int		    lhs_opt_flags;  // for when destination is an option
    int		    lhs_vimvaridx;  // for when destination is a v:var

    lvar_T	    lhs_local_lvar; // used for existing local destination
    lvar_T	    lhs_arg_lvar;   // used for argument destination
    lvar_T	    *lhs_lvar;	    // points to destination lvar
    int		    lhs_scriptvar_sid;
    int		    lhs_scriptvar_idx;

    int		    lhs_has_type;   // type was specified
    type_T	    *lhs_type;
    type_T	    *lhs_member_type;

    int		    lhs_append;	    // used by ISN_REDIREND
} lhs_T;

/*
 * Context for compiling lines of Vim script.
 * Stores info about the local variables and condition stack.
 */
struct cctx_S {
    ufunc_T	*ctx_ufunc;	    // current function
    int		ctx_lnum;	    // line number in current function
    char_u	*ctx_line_start;    // start of current line or NULL
    garray_T	ctx_instr;	    // generated instructions

    int		ctx_prev_lnum;	    // line number below previous command, for
				    // debugging

    compiletype_T ctx_compile_type;

    garray_T	ctx_locals;	    // currently visible local variables

    int		ctx_has_closure;    // set to one if a closures was created in
				    // the function

    garray_T	ctx_imports;	    // imported items

    skip_T	ctx_skip;
    scope_T	*ctx_scope;	    // current scope, NULL at toplevel
    int		ctx_had_return;	    // last seen statement was "return"

    cctx_T	*ctx_outer;	    // outer scope for lambda or nested
				    // function
    int		ctx_outer_used;	    // var in ctx_outer was used

    garray_T	ctx_type_stack;	    // type of each item on the stack
    garray_T	*ctx_type_list;	    // list of pointers to allocated types

    int		ctx_has_cmdmod;	    // ISN_CMDMOD was generated

    lhs_T	ctx_redir_lhs;	    // LHS for ":redir => var", valid when
				    // lhs_name is not NULL
};

static void delete_def_function_contents(dfunc_T *dfunc, int mark_deleted);

/*
 * Lookup variable "name" in the local scope and return it in "lvar".
 * "lvar->lv_from_outer" is incremented accordingly.
 * If "lvar" is NULL only check if the variable can be found.
 * Return FAIL if not found.
 */
    static int
lookup_local(char_u *name, size_t len, lvar_T *lvar, cctx_T *cctx)
{
    int	    idx;
    lvar_T  *lvp;

    if (len == 0)
	return FAIL;

    // Find local in current function scope.
    for (idx = 0; idx < cctx->ctx_locals.ga_len; ++idx)
    {
	lvp = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;
	if (STRNCMP(name, lvp->lv_name, len) == 0
					       && STRLEN(lvp->lv_name) == len)
	{
	    if (lvar != NULL)
	    {
		*lvar = *lvp;
		lvar->lv_from_outer = 0;
	    }
	    return OK;
	}
    }

    // Find local in outer function scope.
    if (cctx->ctx_outer != NULL)
    {
	if (lookup_local(name, len, lvar, cctx->ctx_outer) == OK)
	{
	    if (lvar != NULL)
	    {
		cctx->ctx_outer_used = TRUE;
		++lvar->lv_from_outer;
	    }
	    return OK;
	}
    }

    return FAIL;
}

/*
 * Lookup an argument in the current function and an enclosing function.
 * Returns the argument index in "idxp"
 * Returns the argument type in "type"
 * Sets "gen_load_outer" to TRUE if found in outer scope.
 * Returns OK when found, FAIL otherwise.
 */
    static int
arg_exists(
	char_u	*name,
	size_t	len,
	int	*idxp,
	type_T	**type,
	int	*gen_load_outer,
	cctx_T	*cctx)
{
    int	    idx;
    char_u  *va_name;

    if (len == 0)
	return FAIL;
    for (idx = 0; idx < cctx->ctx_ufunc->uf_args_visible; ++idx)
    {
	char_u *arg = FUNCARG(cctx->ctx_ufunc, idx);

	if (STRNCMP(name, arg, len) == 0 && arg[len] == NUL)
	{
	    if (idxp != NULL)
	    {
		// Arguments are located above the frame pointer.  One further
		// if there is a vararg argument
		*idxp = idx - (cctx->ctx_ufunc->uf_args.ga_len
							    + STACK_FRAME_SIZE)
			      + (cctx->ctx_ufunc->uf_va_name != NULL ? -1 : 0);

		if (cctx->ctx_ufunc->uf_arg_types != NULL)
		    *type = cctx->ctx_ufunc->uf_arg_types[idx];
		else
		    *type = &t_any;
	    }
	    return OK;
	}
    }

    va_name = cctx->ctx_ufunc->uf_va_name;
    if (va_name != NULL
		    && STRNCMP(name, va_name, len) == 0 && va_name[len] == NUL)
    {
	if (idxp != NULL)
	{
	    // varargs is always the last argument
	    *idxp = -STACK_FRAME_SIZE - 1;
	    *type = cctx->ctx_ufunc->uf_va_type;
	}
	return OK;
    }

    if (cctx->ctx_outer != NULL)
    {
	// Lookup the name for an argument of the outer function.
	if (arg_exists(name, len, idxp, type, gen_load_outer, cctx->ctx_outer)
									 == OK)
	{
	    if (gen_load_outer != NULL)
		++*gen_load_outer;
	    return OK;
	}
    }

    return FAIL;
}

/*
 * Lookup a script-local variable in the current script, possibly defined in a
 * block that contains the function "cctx->ctx_ufunc".
 * "cctx" is NULL at the script level.
 * If "len" is <= 0 "name" must be NUL terminated.
 * Return NULL when not found.
 */
    static sallvar_T *
find_script_var(char_u *name, size_t len, cctx_T *cctx)
{
    scriptitem_T    *si = SCRIPT_ITEM(current_sctx.sc_sid);
    hashitem_T	    *hi;
    int		    cc;
    sallvar_T	    *sav;
    ufunc_T	    *ufunc;

    // Find the list of all script variables with the right name.
    if (len > 0)
    {
	cc = name[len];
	name[len] = NUL;
    }
    hi = hash_find(&si->sn_all_vars.dv_hashtab, name);
    if (len > 0)
	name[len] = cc;
    if (HASHITEM_EMPTY(hi))
	return NULL;

    sav = HI2SAV(hi);
    if (sav->sav_block_id == 0 || cctx == NULL)
	// variable defined in the script scope or not in a function.
	return sav;

    // Go over the variables with this name and find one that was visible
    // from the function.
    ufunc = cctx->ctx_ufunc;
    while (sav != NULL)
    {
	int idx;

	// Go over the blocks that this function was defined in.  If the
	// variable block ID matches it was visible to the function.
	for (idx = 0; idx < ufunc->uf_block_depth; ++idx)
	    if (ufunc->uf_block_ids[idx] == sav->sav_block_id)
		return sav;
	sav = sav->sav_next;
    }

    return NULL;
}

/*
 * Return TRUE if the script context is Vim9 script.
 */
    static int
script_is_vim9()
{
    return SCRIPT_ITEM(current_sctx.sc_sid)->sn_version == SCRIPT_VERSION_VIM9;
}

/*
 * Lookup a variable (without s: prefix) in the current script.
 * "cctx" is NULL at the script level.
 * Returns OK or FAIL.
 */
    static int
script_var_exists(char_u *name, size_t len, cctx_T *cctx)
{
    if (current_sctx.sc_sid <= 0)
	return FAIL;
    if (script_is_vim9())
    {
	// Check script variables that were visible where the function was
	// defined.
	if (find_script_var(name, len, cctx) != NULL)
	    return OK;
    }
    else
    {
	hashtab_T	*ht = &SCRIPT_VARS(current_sctx.sc_sid);
	dictitem_T	*di;
	int		cc;

	// Check script variables that are currently visible
	cc = name[len];
	name[len] = NUL;
	di = find_var_in_ht(ht, 0, name, TRUE);
	name[len] = cc;
	if (di != NULL)
	    return OK;
    }

    return FAIL;
}

/*
 * Return TRUE if "name" is a local variable, argument, script variable or
 * imported.
 */
    static int
variable_exists(char_u *name, size_t len, cctx_T *cctx)
{
    return (cctx != NULL
		&& (lookup_local(name, len, NULL, cctx) == OK
		    || arg_exists(name, len, NULL, NULL, NULL, cctx) == OK))
	    || script_var_exists(name, len, cctx) == OK
	    || find_imported(name, len, cctx) != NULL;
}

/*
 * Return TRUE if "name" is a local variable, argument, script variable,
 * imported or function.
 */
    static int
item_exists(char_u *name, size_t len, int cmd UNUSED, cctx_T *cctx)
{
    int	    is_global;
    char_u  *p;

    if (variable_exists(name, len, cctx))
	return TRUE;

    // This is similar to what is in lookup_scriptitem():
    // Find a function, so that a following "->" works.
    // Require "(" or "->" to follow, "Cmd" is a user command while "Cmd()" is
    // a function call.
    p = skipwhite(name + len);

    if (name[len] == '(' || (p[0] == '-' && p[1] == '>'))
    {
	// Do not check for an internal function, since it might also be a
	// valid command, such as ":split" versus "split()".
	// Skip "g:" before a function name.
	is_global = (name[0] == 'g' && name[1] == ':');
	return find_func(is_global ? name + 2 : name, is_global, cctx) != NULL;
    }
    return FALSE;
}

/*
 * Check if "p[len]" is already defined, either in script "import_sid" or in
 * compilation context "cctx".  "cctx" is NULL at the script level.
 * Does not check the global namespace.
 * If "is_arg" is TRUE the error message is for an argument name.
 * Return FAIL and give an error if it defined.
 */
    int
check_defined(char_u *p, size_t len, cctx_T *cctx, int is_arg)
{
    int		c = p[len];
    ufunc_T	*ufunc = NULL;

    // underscore argument is OK
    if (len == 1 && *p == '_')
	return OK;

    if (script_var_exists(p, len, cctx) == OK)
    {
	if (is_arg)
	    semsg(_(e_argument_already_declared_in_script_str), p);
	else
	    semsg(_(e_variable_already_declared_in_script_str), p);
	return FAIL;
    }

    p[len] = NUL;
    if ((cctx != NULL
		&& (lookup_local(p, len, NULL, cctx) == OK
		    || arg_exists(p, len, NULL, NULL, NULL, cctx) == OK))
	    || find_imported(p, len, cctx) != NULL
	    || (ufunc = find_func_even_dead(p, FALSE, cctx)) != NULL)
    {
	// A local or script-local function can shadow a global function.
	if (ufunc == NULL || ((ufunc->uf_flags & FC_DEAD) == 0
		    && (!func_is_global(ufunc)
					     || (p[0] == 'g' && p[1] == ':'))))
	{
	    if (is_arg)
		semsg(_(e_argument_name_shadows_existing_variable_str), p);
	    else
		semsg(_(e_name_already_defined_str), p);
	    p[len] = c;
	    return FAIL;
	}
    }
    p[len] = c;
    return OK;
}


/////////////////////////////////////////////////////////////////////
// Following generate_ functions expect the caller to call ga_grow().

#define RETURN_NULL_IF_SKIP(cctx) if (cctx->ctx_skip == SKIP_YES) return NULL
#define RETURN_OK_IF_SKIP(cctx) if (cctx->ctx_skip == SKIP_YES) return OK

/*
 * Generate an instruction without arguments.
 * Returns a pointer to the new instruction, NULL if failed.
 */
    static isn_T *
generate_instr(cctx_T *cctx, isntype_T isn_type)
{
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*isn;

    RETURN_NULL_IF_SKIP(cctx);
    if (ga_grow(instr, 1) == FAIL)
	return NULL;
    isn = ((isn_T *)instr->ga_data) + instr->ga_len;
    isn->isn_type = isn_type;
    isn->isn_lnum = cctx->ctx_lnum + 1;
    ++instr->ga_len;

    return isn;
}

/*
 * Generate an instruction without arguments.
 * "drop" will be removed from the stack.
 * Returns a pointer to the new instruction, NULL if failed.
 */
    static isn_T *
generate_instr_drop(cctx_T *cctx, isntype_T isn_type, int drop)
{
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_NULL_IF_SKIP(cctx);
    stack->ga_len -= drop;
    return generate_instr(cctx, isn_type);
}

/*
 * Generate instruction "isn_type" and put "type" on the type stack.
 */
    static isn_T *
generate_instr_type(cctx_T *cctx, isntype_T isn_type, type_T *type)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    if ((isn = generate_instr(cctx, isn_type)) == NULL)
	return NULL;

    if (ga_grow(stack, 1) == FAIL)
	return NULL;
    ((type_T **)stack->ga_data)[stack->ga_len] = type == NULL ? &t_any : type;
    ++stack->ga_len;

    return isn;
}

/*
 * Generate an ISN_DEBUG instruction.
 */
    static isn_T *
generate_instr_debug(cctx_T *cctx)
{
    isn_T	*isn;
    dfunc_T	*dfunc = ((dfunc_T *)def_functions.ga_data)
					       + cctx->ctx_ufunc->uf_dfunc_idx;

    if ((isn = generate_instr(cctx, ISN_DEBUG)) == NULL)
	return NULL;
    isn->isn_arg.debug.dbg_var_names_len = dfunc->df_var_names.ga_len;
    isn->isn_arg.debug.dbg_break_lnum = cctx->ctx_prev_lnum;
    return isn;
}

/*
 * If type at "offset" isn't already VAR_STRING then generate ISN_2STRING.
 * But only for simple types.
 * When "tolerant" is TRUE convert most types to string, e.g. a List.
 */
    static int
may_generate_2STRING(int offset, int tolerant, cctx_T *cctx)
{
    isn_T	*isn;
    isntype_T	isntype = ISN_2STRING;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	**type;

    RETURN_OK_IF_SKIP(cctx);
    type = ((type_T **)stack->ga_data) + stack->ga_len + offset;
    switch ((*type)->tt_type)
    {
	// nothing to be done
	case VAR_STRING: return OK;

	// conversion possible
	case VAR_SPECIAL:
	case VAR_BOOL:
	case VAR_NUMBER:
	case VAR_FLOAT:
			 break;

	// conversion possible (with runtime check)
	case VAR_ANY:
	case VAR_UNKNOWN:
			 isntype = ISN_2STRING_ANY;
			 break;

	// conversion possible when tolerant
	case VAR_LIST:
			 if (tolerant)
			 {
			     isntype = ISN_2STRING_ANY;
			     break;
			 }
			 // FALLTHROUGH

	// conversion not possible
	case VAR_VOID:
	case VAR_BLOB:
	case VAR_FUNC:
	case VAR_PARTIAL:
	case VAR_DICT:
	case VAR_JOB:
	case VAR_CHANNEL:
	case VAR_INSTR:
			 to_string_error((*type)->tt_type);
			 return FAIL;
    }

    *type = &t_string;
    if ((isn = generate_instr(cctx, isntype)) == NULL)
	return FAIL;
    isn->isn_arg.tostring.offset = offset;
    isn->isn_arg.tostring.tolerant = tolerant;

    return OK;
}

    static int
check_number_or_float(vartype_T type1, vartype_T type2, char_u *op)
{
    if (!((type1 == VAR_NUMBER || type1 == VAR_FLOAT || type1 == VAR_ANY)
	    && (type2 == VAR_NUMBER || type2 == VAR_FLOAT
							 || type2 == VAR_ANY)))
    {
	if (*op == '+')
	    emsg(_(e_wrong_argument_type_for_plus));
	else
	    semsg(_(e_char_requires_number_or_float_arguments), *op);
	return FAIL;
    }
    return OK;
}

    static int
generate_add_instr(
	cctx_T *cctx,
	vartype_T vartype,
	type_T *type1,
	type_T *type2)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    isn_T	*isn = generate_instr_drop(cctx,
		      vartype == VAR_NUMBER ? ISN_OPNR
		    : vartype == VAR_LIST ? ISN_ADDLIST
		    : vartype == VAR_BLOB ? ISN_ADDBLOB
#ifdef FEAT_FLOAT
		    : vartype == VAR_FLOAT ? ISN_OPFLOAT
#endif
		    : ISN_OPANY, 1);

    if (vartype != VAR_LIST && vartype != VAR_BLOB
	    && type1->tt_type != VAR_ANY
	    && type2->tt_type != VAR_ANY
	    && check_number_or_float(
			type1->tt_type, type2->tt_type, (char_u *)"+") == FAIL)
	return FAIL;

    if (isn != NULL)
	isn->isn_arg.op.op_type = EXPR_ADD;

    // When concatenating two lists with different member types the member type
    // becomes "any".
    if (vartype == VAR_LIST
	    && type1->tt_type == VAR_LIST && type2->tt_type == VAR_LIST
	    && type1->tt_member != type2->tt_member)
	(((type_T **)stack->ga_data)[stack->ga_len - 1]) = &t_list_any;

    return isn == NULL ? FAIL : OK;
}

/*
 * Get the type to use for an instruction for an operation on "type1" and
 * "type2".  If they are matching use a type-specific instruction. Otherwise
 * fall back to runtime type checking.
 */
    static vartype_T
operator_type(type_T *type1, type_T *type2)
{
    if (type1->tt_type == type2->tt_type
	    && (type1->tt_type == VAR_NUMBER
		|| type1->tt_type == VAR_LIST
#ifdef FEAT_FLOAT
		|| type1->tt_type == VAR_FLOAT
#endif
		|| type1->tt_type == VAR_BLOB))
	return type1->tt_type;
    return VAR_ANY;
}

/*
 * Generate an instruction with two arguments.  The instruction depends on the
 * type of the arguments.
 */
    static int
generate_two_op(cctx_T *cctx, char_u *op)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type1;
    type_T	*type2;
    vartype_T	vartype;
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);

    // Get the known type of the two items on the stack.
    type1 = ((type_T **)stack->ga_data)[stack->ga_len - 2];
    type2 = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    vartype = operator_type(type1, type2);

    switch (*op)
    {
	case '+':
		  if (generate_add_instr(cctx, vartype, type1, type2) == FAIL)
		      return FAIL;
		  break;

	case '-':
	case '*':
	case '/': if (check_number_or_float(type1->tt_type, type2->tt_type,
								   op) == FAIL)
		      return FAIL;
		  if (vartype == VAR_NUMBER)
		      isn = generate_instr_drop(cctx, ISN_OPNR, 1);
#ifdef FEAT_FLOAT
		  else if (vartype == VAR_FLOAT)
		      isn = generate_instr_drop(cctx, ISN_OPFLOAT, 1);
#endif
		  else
		      isn = generate_instr_drop(cctx, ISN_OPANY, 1);
		  if (isn != NULL)
		      isn->isn_arg.op.op_type = *op == '*'
				 ? EXPR_MULT : *op == '/'? EXPR_DIV : EXPR_SUB;
		  break;

	case '%': if ((type1->tt_type != VAR_ANY
					       && type1->tt_type != VAR_NUMBER)
			  || (type2->tt_type != VAR_ANY
					      && type2->tt_type != VAR_NUMBER))
		  {
		      emsg(_(e_percent_requires_number_arguments));
		      return FAIL;
		  }
		  isn = generate_instr_drop(cctx,
			      vartype == VAR_NUMBER ? ISN_OPNR : ISN_OPANY, 1);
		  if (isn != NULL)
		      isn->isn_arg.op.op_type = EXPR_REM;
		  break;
    }

    // correct type of result
    if (vartype == VAR_ANY)
    {
	type_T *type = &t_any;

#ifdef FEAT_FLOAT
	// float+number and number+float results in float
	if ((type1->tt_type == VAR_NUMBER || type1->tt_type == VAR_FLOAT)
		&& (type2->tt_type == VAR_NUMBER || type2->tt_type == VAR_FLOAT))
	    type = &t_float;
#endif
	((type_T **)stack->ga_data)[stack->ga_len - 1] = type;
    }

    return OK;
}

/*
 * Get the instruction to use for comparing "type1" with "type2"
 * Return ISN_DROP when failed.
 */
    static isntype_T
get_compare_isn(exprtype_T exprtype, vartype_T type1, vartype_T type2)
{
    isntype_T	isntype = ISN_DROP;

    if (type1 == VAR_UNKNOWN)
	type1 = VAR_ANY;
    if (type2 == VAR_UNKNOWN)
	type2 = VAR_ANY;

    if (type1 == type2)
    {
	switch (type1)
	{
	    case VAR_BOOL: isntype = ISN_COMPAREBOOL; break;
	    case VAR_SPECIAL: isntype = ISN_COMPARESPECIAL; break;
	    case VAR_NUMBER: isntype = ISN_COMPARENR; break;
	    case VAR_FLOAT: isntype = ISN_COMPAREFLOAT; break;
	    case VAR_STRING: isntype = ISN_COMPARESTRING; break;
	    case VAR_BLOB: isntype = ISN_COMPAREBLOB; break;
	    case VAR_LIST: isntype = ISN_COMPARELIST; break;
	    case VAR_DICT: isntype = ISN_COMPAREDICT; break;
	    case VAR_FUNC: isntype = ISN_COMPAREFUNC; break;
	    default: isntype = ISN_COMPAREANY; break;
	}
    }
    else if (type1 == VAR_ANY || type2 == VAR_ANY
	    || ((type1 == VAR_NUMBER || type1 == VAR_FLOAT)
	      && (type2 == VAR_NUMBER || type2 == VAR_FLOAT)))
	isntype = ISN_COMPAREANY;

    if ((exprtype == EXPR_IS || exprtype == EXPR_ISNOT)
	    && (isntype == ISN_COMPAREBOOL
	    || isntype == ISN_COMPARESPECIAL
	    || isntype == ISN_COMPARENR
	    || isntype == ISN_COMPAREFLOAT))
    {
	semsg(_(e_cannot_use_str_with_str),
		exprtype == EXPR_IS ? "is" : "isnot" , vartype_name(type1));
	return ISN_DROP;
    }
    if (isntype == ISN_DROP
	    || ((exprtype != EXPR_EQUAL && exprtype != EXPR_NEQUAL
		    && (type1 == VAR_BOOL || type1 == VAR_SPECIAL
		       || type2 == VAR_BOOL || type2 == VAR_SPECIAL)))
	    || ((exprtype != EXPR_EQUAL && exprtype != EXPR_NEQUAL
				 && exprtype != EXPR_IS && exprtype != EXPR_ISNOT
		    && (type1 == VAR_BLOB || type2 == VAR_BLOB
			|| type1 == VAR_LIST || type2 == VAR_LIST))))
    {
	semsg(_(e_cannot_compare_str_with_str),
		vartype_name(type1), vartype_name(type2));
	return ISN_DROP;
    }
    return isntype;
}

    int
check_compare_types(exprtype_T type, typval_T *tv1, typval_T *tv2)
{
    if (get_compare_isn(type, tv1->v_type, tv2->v_type) == ISN_DROP)
	return FAIL;
    return OK;
}

/*
 * Generate an ISN_COMPARE* instruction with a boolean result.
 */
    static int
generate_COMPARE(cctx_T *cctx, exprtype_T exprtype, int ic)
{
    isntype_T	isntype;
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    vartype_T	type1;
    vartype_T	type2;

    RETURN_OK_IF_SKIP(cctx);

    // Get the known type of the two items on the stack.  If they are matching
    // use a type-specific instruction. Otherwise fall back to runtime type
    // checking.
    type1 = ((type_T **)stack->ga_data)[stack->ga_len - 2]->tt_type;
    type2 = ((type_T **)stack->ga_data)[stack->ga_len - 1]->tt_type;
    isntype = get_compare_isn(exprtype, type1, type2);
    if (isntype == ISN_DROP)
	return FAIL;

    if ((isn = generate_instr(cctx, isntype)) == NULL)
	return FAIL;
    isn->isn_arg.op.op_type = exprtype;
    isn->isn_arg.op.op_ic = ic;

    // takes two arguments, puts one bool back
    if (stack->ga_len >= 2)
    {
	--stack->ga_len;
	((type_T **)stack->ga_data)[stack->ga_len - 1] = &t_bool;
    }

    return OK;
}

/*
 * Generate an ISN_2BOOL instruction.
 * "offset" is the offset in the type stack.
 */
    static int
generate_2BOOL(cctx_T *cctx, int invert, int offset)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_2BOOL)) == NULL)
	return FAIL;
    isn->isn_arg.tobool.invert = invert;
    isn->isn_arg.tobool.offset = offset;

    // type becomes bool
    ((type_T **)stack->ga_data)[stack->ga_len + offset] = &t_bool;

    return OK;
}

/*
 * Generate an ISN_COND2BOOL instruction.
 */
    static int
generate_COND2BOOL(cctx_T *cctx)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_COND2BOOL)) == NULL)
	return FAIL;

    // type becomes bool
    ((type_T **)stack->ga_data)[stack->ga_len - 1] = &t_bool;

    return OK;
}

    static int
generate_TYPECHECK(
	cctx_T	    *cctx,
	type_T	    *expected,
	int	    offset,
	int	    argidx)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_CHECKTYPE)) == NULL)
	return FAIL;
    isn->isn_arg.type.ct_type = alloc_type(expected);
    isn->isn_arg.type.ct_off = (int8_T)offset;
    isn->isn_arg.type.ct_arg_idx = (int8_T)argidx;

    // type becomes expected
    ((type_T **)stack->ga_data)[stack->ga_len + offset] = expected;

    return OK;
}

    static int
generate_SETTYPE(
	cctx_T	    *cctx,
	type_T	    *expected)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_SETTYPE)) == NULL)
	return FAIL;
    isn->isn_arg.type.ct_type = alloc_type(expected);
    return OK;
}

/*
 * Return TRUE if "actual" could be "expected" and a runtime typecheck is to be
 * used.  Return FALSE if the types will never match.
 */
    int
use_typecheck(type_T *actual, type_T *expected)
{
    if (actual->tt_type == VAR_ANY
	    || actual->tt_type == VAR_UNKNOWN
	    || (actual->tt_type == VAR_FUNC
		&& (expected->tt_type == VAR_FUNC
					   || expected->tt_type == VAR_PARTIAL)
		&& (actual->tt_member == &t_any || actual->tt_argcount < 0)
		&& ((actual->tt_member == &t_void)
					 == (expected->tt_member == &t_void))))
	return TRUE;
    if ((actual->tt_type == VAR_LIST || actual->tt_type == VAR_DICT)
				       && actual->tt_type == expected->tt_type)
	// This takes care of a nested list or dict.
	return use_typecheck(actual->tt_member, expected->tt_member);
    return FALSE;
}

/*
 * Check that
 * - "actual" matches "expected" type or
 * - "actual" is a type that can be "expected" type: add a runtime check; or
 * - return FAIL.
 * If "actual_is_const" is TRUE then the type won't change at runtime, do not
 * generate a TYPECHECK.
 */
    int
need_type(
	type_T	*actual,
	type_T	*expected,
	int	offset,
	int	arg_idx,
	cctx_T	*cctx,
	int	silent,
	int	actual_is_const)
{
    where_T where;

    if (expected == &t_bool && actual != &t_bool
					&& (actual->tt_flags & TTFLAG_BOOL_OK))
    {
	// Using "0", "1" or the result of an expression with "&&" or "||" as a
	// boolean is OK but requires a conversion.
	generate_2BOOL(cctx, FALSE, offset);
	return OK;
    }

    where.wt_index = arg_idx;
    where.wt_variable = FALSE;
    if (check_type(expected, actual, FALSE, where) == OK)
	return OK;

    // If the actual type can be the expected type add a runtime check.
    // If it's a constant a runtime check makes no sense.
    if ((!actual_is_const || actual == &t_any)
					    && use_typecheck(actual, expected))
    {
	generate_TYPECHECK(cctx, expected, offset, arg_idx);
	return OK;
    }

    if (!silent)
	arg_type_mismatch(expected, actual, arg_idx);
    return FAIL;
}

/*
 * Check that the top of the type stack has a type that can be used as a
 * condition.  Give an error and return FAIL if not.
 */
    static int
bool_on_stack(cctx_T *cctx)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type;

    type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    if (type == &t_bool)
	return OK;

    if (type == &t_any || type == &t_number || type == &t_number_bool)
	// Number 0 and 1 are OK to use as a bool.  "any" could also be a bool.
	// This requires a runtime type check.
	return generate_COND2BOOL(cctx);

    return need_type(type, &t_bool, -1, 0, cctx, FALSE, FALSE);
}

/*
 * Generate an ISN_PUSHNR instruction.
 */
    static int
generate_PUSHNR(cctx_T *cctx, varnumber_T number)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHNR, &t_number)) == NULL)
	return FAIL;
    isn->isn_arg.number = number;

    if (number == 0 || number == 1)
	// A 0 or 1 number can also be used as a bool.
	((type_T **)stack->ga_data)[stack->ga_len - 1] = &t_number_bool;
    return OK;
}

/*
 * Generate an ISN_PUSHBOOL instruction.
 */
    static int
generate_PUSHBOOL(cctx_T *cctx, varnumber_T number)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHBOOL, &t_bool)) == NULL)
	return FAIL;
    isn->isn_arg.number = number;

    return OK;
}

/*
 * Generate an ISN_PUSHSPEC instruction.
 */
    static int
generate_PUSHSPEC(cctx_T *cctx, varnumber_T number)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHSPEC, &t_special)) == NULL)
	return FAIL;
    isn->isn_arg.number = number;

    return OK;
}

#ifdef FEAT_FLOAT
/*
 * Generate an ISN_PUSHF instruction.
 */
    static int
generate_PUSHF(cctx_T *cctx, float_T fnumber)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHF, &t_float)) == NULL)
	return FAIL;
    isn->isn_arg.fnumber = fnumber;

    return OK;
}
#endif

/*
 * Generate an ISN_PUSHS instruction.
 * Consumes "str".
 */
    static int
generate_PUSHS(cctx_T *cctx, char_u *str)
{
    isn_T	*isn;

    if (cctx->ctx_skip == SKIP_YES)
    {
	vim_free(str);
	return OK;
    }
    if ((isn = generate_instr_type(cctx, ISN_PUSHS, &t_string)) == NULL)
	return FAIL;
    isn->isn_arg.string = str;

    return OK;
}

/*
 * Generate an ISN_PUSHCHANNEL instruction.
 * Consumes "channel".
 */
    static int
generate_PUSHCHANNEL(cctx_T *cctx, channel_T *channel)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHCHANNEL, &t_channel)) == NULL)
	return FAIL;
    isn->isn_arg.channel = channel;

    return OK;
}

/*
 * Generate an ISN_PUSHJOB instruction.
 * Consumes "job".
 */
    static int
generate_PUSHJOB(cctx_T *cctx, job_T *job)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHJOB, &t_channel)) == NULL)
	return FAIL;
    isn->isn_arg.job = job;

    return OK;
}

/*
 * Generate an ISN_PUSHBLOB instruction.
 * Consumes "blob".
 */
    static int
generate_PUSHBLOB(cctx_T *cctx, blob_T *blob)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHBLOB, &t_blob)) == NULL)
	return FAIL;
    isn->isn_arg.blob = blob;

    return OK;
}

/*
 * Generate an ISN_PUSHFUNC instruction with name "name".
 * Consumes "name".
 */
    static int
generate_PUSHFUNC(cctx_T *cctx, char_u *name, type_T *type)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_PUSHFUNC, type)) == NULL)
	return FAIL;
    isn->isn_arg.string = name == NULL ? NULL : vim_strsave(name);

    return OK;
}

/*
 * Generate an ISN_GETITEM instruction with "index".
 * "with_op" is TRUE for "+=" and other operators, the stack has the current
 * value below the list with values.
 */
    static int
generate_GETITEM(cctx_T *cctx, int index, int with_op)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type = ((type_T **)stack->ga_data)[stack->ga_len
							  - (with_op ? 2 : 1)];
    type_T	*item_type = &t_any;

    RETURN_OK_IF_SKIP(cctx);

    if (type->tt_type != VAR_LIST)
    {
	// cannot happen, caller has checked the type
	emsg(_(e_listreq));
	return FAIL;
    }
    item_type = type->tt_member;
    if ((isn = generate_instr(cctx, ISN_GETITEM)) == NULL)
	return FAIL;
    isn->isn_arg.getitem.gi_index = index;
    isn->isn_arg.getitem.gi_with_op = with_op;

    // add the item type to the type stack
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] = item_type;
    ++stack->ga_len;
    return OK;
}

/*
 * Generate an ISN_SLICE instruction with "count".
 */
    static int
generate_SLICE(cctx_T *cctx, int count)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_SLICE)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;
    return OK;
}

/*
 * Generate an ISN_CHECKLEN instruction with "min_len".
 */
    static int
generate_CHECKLEN(cctx_T *cctx, int min_len, int more_OK)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);

    if ((isn = generate_instr(cctx, ISN_CHECKLEN)) == NULL)
	return FAIL;
    isn->isn_arg.checklen.cl_min_len = min_len;
    isn->isn_arg.checklen.cl_more_OK = more_OK;

    return OK;
}

/*
 * Generate an ISN_STORE instruction.
 */
    static int
generate_STORE(cctx_T *cctx, isntype_T isn_type, int idx, char_u *name)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_drop(cctx, isn_type, 1)) == NULL)
	return FAIL;
    if (name != NULL)
	isn->isn_arg.string = vim_strsave(name);
    else
	isn->isn_arg.number = idx;

    return OK;
}

/*
 * Generate an ISN_STOREOUTER instruction.
 */
    static int
generate_STOREOUTER(cctx_T *cctx, int idx, int level)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_drop(cctx, ISN_STOREOUTER, 1)) == NULL)
	return FAIL;
    isn->isn_arg.outer.outer_idx = idx;
    isn->isn_arg.outer.outer_depth = level;

    return OK;
}

/*
 * Generate an ISN_STORENR instruction (short for ISN_PUSHNR + ISN_STORE)
 */
    static int
generate_STORENR(cctx_T *cctx, int idx, varnumber_T value)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_STORENR)) == NULL)
	return FAIL;
    isn->isn_arg.storenr.stnr_idx = idx;
    isn->isn_arg.storenr.stnr_val = value;

    return OK;
}

/*
 * Generate an ISN_STOREOPT instruction
 */
    static int
generate_STOREOPT(cctx_T *cctx, char_u *name, int opt_flags)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_drop(cctx, ISN_STOREOPT, 1)) == NULL)
	return FAIL;
    isn->isn_arg.storeopt.so_name = vim_strsave(name);
    isn->isn_arg.storeopt.so_flags = opt_flags;

    return OK;
}

/*
 * Generate an ISN_LOAD or similar instruction.
 */
    static int
generate_LOAD(
	cctx_T	    *cctx,
	isntype_T   isn_type,
	int	    idx,
	char_u	    *name,
	type_T	    *type)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, isn_type, type)) == NULL)
	return FAIL;
    if (name != NULL)
	isn->isn_arg.string = vim_strsave(name);
    else
	isn->isn_arg.number = idx;

    return OK;
}

/*
 * Generate an ISN_LOADOUTER instruction
 */
    static int
generate_LOADOUTER(
	cctx_T	    *cctx,
	int	    idx,
	int	    nesting,
	type_T	    *type)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_type(cctx, ISN_LOADOUTER, type)) == NULL)
	return FAIL;
    isn->isn_arg.outer.outer_idx = idx;
    isn->isn_arg.outer.outer_depth = nesting;

    return OK;
}

/*
 * Generate an ISN_LOADV instruction for v:var.
 */
    static int
generate_LOADV(
	cctx_T	    *cctx,
	char_u	    *name,
	int	    error)
{
    int	    di_flags;
    int	    vidx = find_vim_var(name, &di_flags);
    type_T  *type;

    RETURN_OK_IF_SKIP(cctx);
    if (vidx < 0)
    {
	if (error)
	    semsg(_(e_variable_not_found_str), name);
	return FAIL;
    }
    type = typval2type_vimvar(get_vim_var_tv(vidx), cctx->ctx_type_list);

    return generate_LOAD(cctx, ISN_LOADV, vidx, NULL, type);
}

/*
 * Generate an ISN_UNLET instruction.
 */
    static int
generate_UNLET(cctx_T *cctx, isntype_T isn_type, char_u *name, int forceit)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, isn_type)) == NULL)
	return FAIL;
    isn->isn_arg.unlet.ul_name = vim_strsave(name);
    isn->isn_arg.unlet.ul_forceit = forceit;

    return OK;
}

/*
 * Generate an ISN_LOCKCONST instruction.
 */
    static int
generate_LOCKCONST(cctx_T *cctx)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_LOCKCONST)) == NULL)
	return FAIL;
    return OK;
}

/*
 * Generate an ISN_LOADS instruction.
 */
    static int
generate_OLDSCRIPT(
	cctx_T	    *cctx,
	isntype_T   isn_type,
	char_u	    *name,
	int	    sid,
	type_T	    *type)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if (isn_type == ISN_LOADS)
	isn = generate_instr_type(cctx, isn_type, type);
    else
	isn = generate_instr_drop(cctx, isn_type, 1);
    if (isn == NULL)
	return FAIL;
    isn->isn_arg.loadstore.ls_name = vim_strsave(name);
    isn->isn_arg.loadstore.ls_sid = sid;

    return OK;
}

/*
 * Generate an ISN_LOADSCRIPT or ISN_STORESCRIPT instruction.
 */
    static int
generate_VIM9SCRIPT(
	cctx_T	    *cctx,
	isntype_T   isn_type,
	int	    sid,
	int	    idx,
	type_T	    *type)
{
    isn_T	*isn;
    scriptref_T	*sref;
    scriptitem_T *si = SCRIPT_ITEM(sid);

    RETURN_OK_IF_SKIP(cctx);
    if (isn_type == ISN_LOADSCRIPT)
	isn = generate_instr_type(cctx, isn_type, type);
    else
	isn = generate_instr_drop(cctx, isn_type, 1);
    if (isn == NULL)
	return FAIL;

    // This requires three arguments, which doesn't fit in an instruction, thus
    // we need to allocate a struct for this.
    sref = ALLOC_ONE(scriptref_T);
    if (sref == NULL)
	return FAIL;
    isn->isn_arg.script.scriptref = sref;
    sref->sref_sid = sid;
    sref->sref_idx = idx;
    sref->sref_seq = si->sn_script_seq;
    sref->sref_type = type;
    return OK;
}

/*
 * Generate an ISN_NEWLIST instruction.
 */
    static int
generate_NEWLIST(cctx_T *cctx, int count)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type;
    type_T	*member;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_NEWLIST)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;

    // get the member type from all the items on the stack.
    if (count == 0)
	member = &t_void;
    else
	member = get_member_type_from_stack(
	    ((type_T **)stack->ga_data) + stack->ga_len, count, 1,
							  cctx->ctx_type_list);
    type = get_list_type(member, cctx->ctx_type_list);

    // drop the value types
    stack->ga_len -= count;

    // add the list type to the type stack
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] = type;
    ++stack->ga_len;

    return OK;
}

/*
 * Generate an ISN_NEWDICT instruction.
 */
    static int
generate_NEWDICT(cctx_T *cctx, int count)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type;
    type_T	*member;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_NEWDICT)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;

    if (count == 0)
	member = &t_void;
    else
	member = get_member_type_from_stack(
	    ((type_T **)stack->ga_data) + stack->ga_len, count, 2,
							  cctx->ctx_type_list);
    type = get_dict_type(member, cctx->ctx_type_list);

    // drop the key and value types
    stack->ga_len -= 2 * count;

    // add the dict type to the type stack
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] = type;
    ++stack->ga_len;

    return OK;
}

/*
 * Generate an ISN_FUNCREF instruction.
 */
    static int
generate_FUNCREF(cctx_T *cctx, ufunc_T *ufunc)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_FUNCREF)) == NULL)
	return FAIL;
    isn->isn_arg.funcref.fr_func = ufunc->uf_dfunc_idx;
    cctx->ctx_has_closure = 1;

    // If the referenced function is a closure, it may use items further up in
    // the nested context, including this one.
    if (ufunc->uf_flags & FC_CLOSURE)
	cctx->ctx_ufunc->uf_flags |= FC_CLOSURE;

    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] =
	       ufunc->uf_func_type == NULL ? &t_func_any : ufunc->uf_func_type;
    ++stack->ga_len;

    return OK;
}

/*
 * Generate an ISN_NEWFUNC instruction.
 * "lambda_name" and "func_name" must be in allocated memory and will be
 * consumed.
 */
    static int
generate_NEWFUNC(cctx_T *cctx, char_u *lambda_name, char_u *func_name)
{
    isn_T	*isn;

    if (cctx->ctx_skip == SKIP_YES)
    {
	vim_free(lambda_name);
	vim_free(func_name);
	return OK;
    }
    if ((isn = generate_instr(cctx, ISN_NEWFUNC)) == NULL)
    {
	vim_free(lambda_name);
	vim_free(func_name);
	return FAIL;
    }
    isn->isn_arg.newfunc.nf_lambda = lambda_name;
    isn->isn_arg.newfunc.nf_global = func_name;

    return OK;
}

/*
 * Generate an ISN_DEF instruction: list functions
 */
    static int
generate_DEF(cctx_T *cctx, char_u *name, size_t len)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_DEF)) == NULL)
	return FAIL;
    if (len > 0)
    {
	isn->isn_arg.string = vim_strnsave(name, len);
	if (isn->isn_arg.string == NULL)
	    return FAIL;
    }
    return OK;
}

/*
 * Generate an ISN_JUMP instruction.
 */
    static int
generate_JUMP(cctx_T *cctx, jumpwhen_T when, int where)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_JUMP)) == NULL)
	return FAIL;
    isn->isn_arg.jump.jump_when = when;
    isn->isn_arg.jump.jump_where = where;

    if (when != JUMP_ALWAYS && stack->ga_len > 0)
	--stack->ga_len;

    return OK;
}

/*
 * Generate an ISN_JUMP_IF_ARG_SET instruction.
 */
    static int
generate_JUMP_IF_ARG_SET(cctx_T *cctx, int arg_off)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_JUMP_IF_ARG_SET)) == NULL)
	return FAIL;
    isn->isn_arg.jumparg.jump_arg_off = arg_off;
    // jump_where is set later
    return OK;
}

    static int
generate_FOR(cctx_T *cctx, int loop_idx)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_FOR)) == NULL)
	return FAIL;
    isn->isn_arg.forloop.for_idx = loop_idx;

    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    // type doesn't matter, will be stored next
    ((type_T **)stack->ga_data)[stack->ga_len] = &t_any;
    ++stack->ga_len;

    return OK;
}
/*
 * Generate an ISN_TRYCONT instruction.
 */
    static int
generate_TRYCONT(cctx_T *cctx, int levels, int where)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_TRYCONT)) == NULL)
	return FAIL;
    isn->isn_arg.trycont.tct_levels = levels;
    isn->isn_arg.trycont.tct_where = where;

    return OK;
}


/*
 * Generate an ISN_BCALL instruction.
 * "method_call" is TRUE for "value->method()"
 * Return FAIL if the number of arguments is wrong.
 */
    static int
generate_BCALL(cctx_T *cctx, int func_idx, int argcount, int method_call)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    int		argoff;
    type_T	**argtypes = NULL;
    type_T	*shuffled_argtypes[MAX_FUNC_ARGS];
    type_T	*maptype = NULL;

    RETURN_OK_IF_SKIP(cctx);
    argoff = check_internal_func(func_idx, argcount);
    if (argoff < 0)
	return FAIL;

    if (method_call && argoff > 1)
    {
	if ((isn = generate_instr(cctx, ISN_SHUFFLE)) == NULL)
	    return FAIL;
	isn->isn_arg.shuffle.shfl_item = argcount;
	isn->isn_arg.shuffle.shfl_up = argoff - 1;
    }

    if (argcount > 0)
    {
	// Check the types of the arguments.
	argtypes = ((type_T **)stack->ga_data) + stack->ga_len - argcount;
	if (method_call && argoff > 1)
	{
	    int i;

	    for (i = 0; i < argcount; ++i)
		shuffled_argtypes[i] = (i < argoff - 1)
			    ? argtypes[i + 1]
			    : (i == argoff - 1) ? argtypes[0] : argtypes[i];
	    argtypes = shuffled_argtypes;
	}
	if (internal_func_check_arg_types(argtypes, func_idx, argcount,
								 cctx) == FAIL)
	    return FAIL;
	if (internal_func_is_map(func_idx))
	    maptype = *argtypes;
    }

    if ((isn = generate_instr(cctx, ISN_BCALL)) == NULL)
	return FAIL;
    isn->isn_arg.bfunc.cbf_idx = func_idx;
    isn->isn_arg.bfunc.cbf_argcount = argcount;

    // Drop the argument types and push the return type.
    stack->ga_len -= argcount;
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] =
			  internal_func_ret_type(func_idx, argcount, argtypes);
    ++stack->ga_len;

    if (maptype != NULL && maptype->tt_member != NULL
					       && maptype->tt_member != &t_any)
	// Check that map() didn't change the item types.
	generate_TYPECHECK(cctx, maptype, -1, 1);

    return OK;
}

/*
 * Generate an ISN_LISTAPPEND instruction.  Works like add().
 * Argument count is already checked.
 */
    static int
generate_LISTAPPEND(cctx_T *cctx)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*list_type;
    type_T	*item_type;
    type_T	*expected;

    // Caller already checked that list_type is a list.
    list_type = ((type_T **)stack->ga_data)[stack->ga_len - 2];
    item_type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    expected = list_type->tt_member;
    if (need_type(item_type, expected, -1, 0, cctx, FALSE, FALSE) == FAIL)
	return FAIL;

    if (generate_instr(cctx, ISN_LISTAPPEND) == NULL)
	return FAIL;

    --stack->ga_len;	    // drop the argument
    return OK;
}

/*
 * Generate an ISN_BLOBAPPEND instruction.  Works like add().
 * Argument count is already checked.
 */
    static int
generate_BLOBAPPEND(cctx_T *cctx)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*item_type;

    // Caller already checked that blob_type is a blob.
    item_type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    if (need_type(item_type, &t_number, -1, 0, cctx, FALSE, FALSE) == FAIL)
	return FAIL;

    if (generate_instr(cctx, ISN_BLOBAPPEND) == NULL)
	return FAIL;

    --stack->ga_len;	    // drop the argument
    return OK;
}

/*
 * Return TRUE if "ufunc" should be compiled, taking into account whether
 * "profile" indicates profiling is to be done.
 */
    int
func_needs_compiling(ufunc_T *ufunc, compiletype_T compile_type)
{
    switch (ufunc->uf_def_status)
    {
	case UF_TO_BE_COMPILED:
	    return TRUE;

	case UF_COMPILED:
	{
	    dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;

	    switch (compile_type)
	    {
		case CT_PROFILE:
#ifdef FEAT_PROFILE
		    return dfunc->df_instr_prof == NULL;
#endif
		case CT_NONE:
		    return dfunc->df_instr == NULL;
		case CT_DEBUG:
		    return dfunc->df_instr_debug == NULL;
	    }
	}

	case UF_NOT_COMPILED:
	case UF_COMPILE_ERROR:
	case UF_COMPILING:
	    break;
    }
    return FALSE;
}

/*
 * Generate an ISN_DCALL or ISN_UCALL instruction.
 * Return FAIL if the number of arguments is wrong.
 */
    static int
generate_CALL(cctx_T *cctx, ufunc_T *ufunc, int pushed_argcount)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    int		regular_args = ufunc->uf_args.ga_len;
    int		argcount = pushed_argcount;

    RETURN_OK_IF_SKIP(cctx);
    if (argcount > regular_args && !has_varargs(ufunc))
    {
	semsg(_(e_toomanyarg), printable_func_name(ufunc));
	return FAIL;
    }
    if (argcount < regular_args - ufunc->uf_def_args.ga_len)
    {
	semsg(_(e_toofewarg), printable_func_name(ufunc));
	return FAIL;
    }

    if (ufunc->uf_def_status != UF_NOT_COMPILED
	    && ufunc->uf_def_status != UF_COMPILE_ERROR)
    {
	int		i;

	for (i = 0; i < argcount; ++i)
	{
	    type_T *expected;
	    type_T *actual;

	    actual = ((type_T **)stack->ga_data)[stack->ga_len - argcount + i];
	    if (actual == &t_special
			      && i >= regular_args - ufunc->uf_def_args.ga_len)
	    {
		// assume v:none used for default argument value
		continue;
	    }
	    if (i < regular_args)
	    {
		if (ufunc->uf_arg_types == NULL)
		    continue;
		expected = ufunc->uf_arg_types[i];
	    }
	    else if (ufunc->uf_va_type == NULL
					   || ufunc->uf_va_type == &t_list_any)
		// possibly a lambda or "...: any"
		expected = &t_any;
	    else
		expected = ufunc->uf_va_type->tt_member;
	    if (need_type(actual, expected, -argcount + i, i + 1, cctx,
							  TRUE, FALSE) == FAIL)
	    {
		arg_type_mismatch(expected, actual, i + 1);
		return FAIL;
	    }
	}
	if (func_needs_compiling(ufunc, COMPILE_TYPE(ufunc))
		&& compile_def_function(ufunc, ufunc->uf_ret_type == NULL,
					    COMPILE_TYPE(ufunc), NULL) == FAIL)
	    return FAIL;
    }
    if (ufunc->uf_def_status == UF_COMPILE_ERROR)
    {
	emsg_funcname(_(e_call_to_function_that_failed_to_compile_str),
							       ufunc->uf_name);
	return FAIL;
    }

    if ((isn = generate_instr(cctx,
		    ufunc->uf_def_status != UF_NOT_COMPILED ? ISN_DCALL
							 : ISN_UCALL)) == NULL)
	return FAIL;
    if (isn->isn_type == ISN_DCALL)
    {
	isn->isn_arg.dfunc.cdf_idx = ufunc->uf_dfunc_idx;
	isn->isn_arg.dfunc.cdf_argcount = argcount;
    }
    else
    {
	// A user function may be deleted and redefined later, can't use the
	// ufunc pointer, need to look it up again at runtime.
	isn->isn_arg.ufunc.cuf_name = vim_strsave(ufunc->uf_name);
	isn->isn_arg.ufunc.cuf_argcount = argcount;
    }

    stack->ga_len -= argcount; // drop the arguments
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    // add return value
    ((type_T **)stack->ga_data)[stack->ga_len] = ufunc->uf_ret_type;
    ++stack->ga_len;

    return OK;
}

/*
 * Generate an ISN_UCALL instruction when the function isn't defined yet.
 */
    static int
generate_UCALL(cctx_T *cctx, char_u *name, int argcount)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_UCALL)) == NULL)
	return FAIL;
    isn->isn_arg.ufunc.cuf_name = vim_strsave(name);
    isn->isn_arg.ufunc.cuf_argcount = argcount;

    stack->ga_len -= argcount; // drop the arguments
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    // add return value
    ((type_T **)stack->ga_data)[stack->ga_len] = &t_any;
    ++stack->ga_len;

    return OK;
}

/*
 * Generate an ISN_PCALL instruction.
 * "type" is the type of the FuncRef.
 */
    static int
generate_PCALL(
	cctx_T	*cctx,
	int	argcount,
	char_u	*name,
	type_T	*type,
	int	at_top)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*ret_type;

    RETURN_OK_IF_SKIP(cctx);

    if (type->tt_type == VAR_ANY)
	ret_type = &t_any;
    else if (type->tt_type == VAR_FUNC || type->tt_type == VAR_PARTIAL)
    {
	if (type->tt_argcount != -1)
	{
	    int	    varargs = (type->tt_flags & TTFLAG_VARARGS) ? 1 : 0;

	    if (argcount < type->tt_min_argcount - varargs)
	    {
		semsg(_(e_toofewarg), name);
		return FAIL;
	    }
	    if (!varargs && argcount > type->tt_argcount)
	    {
		semsg(_(e_toomanyarg), name);
		return FAIL;
	    }
	    if (type->tt_args != NULL)
	    {
		int i;

		for (i = 0; i < argcount; ++i)
		{
		    int	    offset = -argcount + i - (at_top ? 0 : 1);
		    type_T *actual = ((type_T **)stack->ga_data)[
						       stack->ga_len + offset];
		    type_T *expected;

		    if (varargs && i >= type->tt_argcount - 1)
			expected = type->tt_args[
					     type->tt_argcount - 1]->tt_member;
		    else if (i >= type->tt_min_argcount
						       && actual == &t_special)
			expected = &t_any;
		    else
			expected = type->tt_args[i];
		    if (need_type(actual, expected, offset, i + 1,
						    cctx, TRUE, FALSE) == FAIL)
		    {
			arg_type_mismatch(expected, actual, i + 1);
			return FAIL;
		    }
		}
	    }
	}
	ret_type = type->tt_member;
    }
    else
    {
	semsg(_(e_not_callable_type_str), name);
	return FAIL;
    }

    if ((isn = generate_instr(cctx, ISN_PCALL)) == NULL)
	return FAIL;
    isn->isn_arg.pfunc.cpf_top = at_top;
    isn->isn_arg.pfunc.cpf_argcount = argcount;

    stack->ga_len -= argcount; // drop the arguments

    // drop the funcref/partial, get back the return value
    ((type_T **)stack->ga_data)[stack->ga_len - 1] = ret_type;

    // If partial is above the arguments it must be cleared and replaced with
    // the return value.
    if (at_top && generate_instr(cctx, ISN_PCALL_END) == NULL)
	return FAIL;

    return OK;
}

/*
 * Generate an ISN_STRINGMEMBER instruction.
 */
    static int
generate_STRINGMEMBER(cctx_T *cctx, char_u *name, size_t len)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_STRINGMEMBER)) == NULL)
	return FAIL;
    isn->isn_arg.string = vim_strnsave(name, len);

    // check for dict type
    type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    if (type->tt_type != VAR_DICT && type != &t_any)
    {
	emsg(_(e_dictreq));
	return FAIL;
    }
    // change dict type to dict member type
    if (type->tt_type == VAR_DICT)
    {
	((type_T **)stack->ga_data)[stack->ga_len - 1] =
		      type->tt_member == &t_unknown ? &t_any : type->tt_member;
    }

    return OK;
}

/*
 * Generate an ISN_ECHO instruction.
 */
    static int
generate_ECHO(cctx_T *cctx, int with_white, int count)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr_drop(cctx, ISN_ECHO, count)) == NULL)
	return FAIL;
    isn->isn_arg.echo.echo_with_white = with_white;
    isn->isn_arg.echo.echo_count = count;

    return OK;
}

/*
 * Generate an ISN_EXECUTE/ISN_ECHOMSG/ISN_ECHOERR instruction.
 */
    static int
generate_MULT_EXPR(cctx_T *cctx, isntype_T isn_type, int count)
{
    isn_T	*isn;

    if ((isn = generate_instr_drop(cctx, isn_type, count)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;

    return OK;
}

/*
 * Generate an ISN_PUT instruction.
 */
    static int
generate_PUT(cctx_T *cctx, int regname, linenr_T lnum)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_PUT)) == NULL)
	return FAIL;
    isn->isn_arg.put.put_regname = regname;
    isn->isn_arg.put.put_lnum = lnum;
    return OK;
}

    static int
generate_EXEC(cctx_T *cctx, char_u *line)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_EXEC)) == NULL)
	return FAIL;
    isn->isn_arg.string = vim_strsave(line);
    return OK;
}

    static int
generate_LEGACY_EVAL(cctx_T *cctx, char_u *line)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_LEGACY_EVAL)) == NULL)
	return FAIL;
    isn->isn_arg.string = vim_strsave(line);

    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] = &t_any;
    ++stack->ga_len;

    return OK;
}

    static int
generate_EXECCONCAT(cctx_T *cctx, int count)
{
    isn_T	*isn;

    if ((isn = generate_instr_drop(cctx, ISN_EXECCONCAT, count)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;
    return OK;
}

/*
 * Generate ISN_RANGE.  Consumes "range".  Return OK/FAIL.
 */
    static int
generate_RANGE(cctx_T *cctx, char_u *range)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    if ((isn = generate_instr(cctx, ISN_RANGE)) == NULL)
	return FAIL;
    isn->isn_arg.string = range;

    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] = &t_number;
    ++stack->ga_len;
    return OK;
}

    static int
generate_UNPACK(cctx_T *cctx, int var_count, int semicolon)
{
    isn_T	*isn;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_UNPACK)) == NULL)
	return FAIL;
    isn->isn_arg.unpack.unp_count = var_count;
    isn->isn_arg.unpack.unp_semicolon = semicolon;
    return OK;
}

/*
 * Generate an instruction for any command modifiers.
 */
    static int
generate_cmdmods(cctx_T *cctx, cmdmod_T *cmod)
{
    isn_T	*isn;

    if (has_cmdmod(cmod))
    {
	cctx->ctx_has_cmdmod = TRUE;

	if ((isn = generate_instr(cctx, ISN_CMDMOD)) == NULL)
	    return FAIL;
	isn->isn_arg.cmdmod.cf_cmdmod = ALLOC_ONE(cmdmod_T);
	if (isn->isn_arg.cmdmod.cf_cmdmod == NULL)
	    return FAIL;
	mch_memmove(isn->isn_arg.cmdmod.cf_cmdmod, cmod, sizeof(cmdmod_T));
	// filter program now belongs to the instruction
	cmod->cmod_filter_regmatch.regprog = NULL;
    }

    return OK;
}

    static int
generate_undo_cmdmods(cctx_T *cctx)
{
    if (cctx->ctx_has_cmdmod && generate_instr(cctx, ISN_CMDMOD_REV) == NULL)
	return FAIL;
    cctx->ctx_has_cmdmod = FALSE;
    return OK;
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
 * Get the index of the current instruction.
 * This compenstates for a preceding ISN_CMDMOD and ISN_PROF_START.
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

#ifdef FEAT_PROFILE
    static void
may_generate_prof_end(cctx_T *cctx, int prof_lnum)
{
    if (cctx->ctx_compile_type == CT_PROFILE && prof_lnum >= 0)
	generate_instr(cctx, ISN_PROF_END);
}
#endif

/*
 * Reserve space for a local variable.
 * Return the variable or NULL if it failed.
 */
    static lvar_T *
reserve_local(
	cctx_T	*cctx,
	char_u	*name,
	size_t	len,
	int	isConst,
	type_T	*type)
{
    lvar_T  *lvar;
    dfunc_T *dfunc;

    if (arg_exists(name, len, NULL, NULL, NULL, cctx) == OK)
    {
	emsg_namelen(_(e_str_is_used_as_argument), name, (int)len);
	return NULL;
    }

    if (ga_grow(&cctx->ctx_locals, 1) == FAIL)
	return NULL;
    lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + cctx->ctx_locals.ga_len++;
    CLEAR_POINTER(lvar);

    // Every local variable uses the next entry on the stack.  We could re-use
    // the last ones when leaving a scope, but then variables used in a closure
    // might get overwritten.  To keep things simple do not re-use stack
    // entries.  This is less efficient, but memory is cheap these days.
    dfunc = ((dfunc_T *)def_functions.ga_data) + cctx->ctx_ufunc->uf_dfunc_idx;
    lvar->lv_idx = dfunc->df_var_names.ga_len;

    lvar->lv_name = vim_strnsave(name, len == 0 ? STRLEN(name) : len);
    lvar->lv_const = isConst;
    lvar->lv_type = type;

    // Remember the name for debugging.
    if (ga_grow(&dfunc->df_var_names, 1) == FAIL)
	return NULL;
    ((char_u **)dfunc->df_var_names.ga_data)[lvar->lv_idx] =
						    vim_strsave(lvar->lv_name);
    ++dfunc->df_var_names.ga_len;

    return lvar;
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
    static void
free_locals(cctx_T *cctx)
{
    unwind_locals(cctx, 0);
    ga_clear(&cctx->ctx_locals);
}

/*
 * If "check_writable" is ASSIGN_CONST give an error if the variable was
 * defined with :final or :const, if "check_writable" is ASSIGN_FINAL give an
 * error if the variable was defined with :const.
 */
    static int
check_item_writable(svar_T *sv, int check_writable, char_u *name)
{
    if ((check_writable == ASSIGN_CONST && sv->sv_const != 0)
	    || (check_writable == ASSIGN_FINAL
					      && sv->sv_const == ASSIGN_CONST))
    {
	semsg(_(e_cannot_change_readonly_variable_str), name);
	return FAIL;
    }
    return OK;
}

/*
 * Find "name" in script-local items of script "sid".
 * Pass "check_writable" to check_item_writable().
 * Returns the index in "sn_var_vals" if found.
 * If found but not in "sn_var_vals" returns -1.
 * If not found or the variable is not writable returns -2.
 */
    int
get_script_item_idx(int sid, char_u *name, int check_writable, cctx_T *cctx)
{
    hashtab_T	    *ht;
    dictitem_T	    *di;
    scriptitem_T    *si = SCRIPT_ITEM(sid);
    svar_T	    *sv;
    int		    idx;

    if (!SCRIPT_ID_VALID(sid))
	return -1;
    if (sid == current_sctx.sc_sid)
    {
	sallvar_T *sav = find_script_var(name, 0, cctx);

	if (sav == NULL)
	    return -2;
	idx = sav->sav_var_vals_idx;
	sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;
	if (check_item_writable(sv, check_writable, name) == FAIL)
	    return -2;
	return idx;
    }

    // First look the name up in the hashtable.
    ht = &SCRIPT_VARS(sid);
    di = find_var_in_ht(ht, 0, name, TRUE);
    if (di == NULL)
	return -2;

    // Now find the svar_T index in sn_var_vals.
    for (idx = 0; idx < si->sn_var_vals.ga_len; ++idx)
    {
	sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;
	if (sv->sv_tv == &di->di_tv)
	{
	    if (check_item_writable(sv, check_writable, name) == FAIL)
		return -2;
	    return idx;
	}
    }
    return -1;
}

/*
 * Find "name" in imported items of the current script or in "cctx" if not
 * NULL.
 */
    imported_T *
find_imported(char_u *name, size_t len, cctx_T *cctx)
{
    int		    idx;

    if (!SCRIPT_ID_VALID(current_sctx.sc_sid))
	return NULL;
    if (cctx != NULL)
	for (idx = 0; idx < cctx->ctx_imports.ga_len; ++idx)
	{
	    imported_T *import = ((imported_T *)cctx->ctx_imports.ga_data)
									 + idx;

	    if (len == 0 ? STRCMP(name, import->imp_name) == 0
			 : STRLEN(import->imp_name) == len
				  && STRNCMP(name, import->imp_name, len) == 0)
		return import;
	}

    return find_imported_in_script(name, len, current_sctx.sc_sid);
}

    imported_T *
find_imported_in_script(char_u *name, size_t len, int sid)
{
    scriptitem_T    *si;
    int		    idx;

    if (!SCRIPT_ID_VALID(sid))
	return NULL;
    si = SCRIPT_ITEM(sid);
    for (idx = 0; idx < si->sn_imports.ga_len; ++idx)
    {
	imported_T *import = ((imported_T *)si->sn_imports.ga_data) + idx;

	if (len == 0 ? STRCMP(name, import->imp_name) == 0
		     : STRLEN(import->imp_name) == len
				  && STRNCMP(name, import->imp_name, len) == 0)
	    return import;
    }
    return NULL;
}

/*
 * Free all imported variables.
 */
    static void
free_imported(cctx_T *cctx)
{
    int idx;

    for (idx = 0; idx < cctx->ctx_imports.ga_len; ++idx)
    {
	imported_T *import = ((imported_T *)cctx->ctx_imports.ga_data) + idx;

	vim_free(import->imp_name);
    }
    ga_clear(&cctx->ctx_imports);
}

/*
 * Return a pointer to the next line that isn't empty or only contains a
 * comment. Skips over white space.
 * Returns NULL if there is none.
 */
    char_u *
peek_next_line_from_context(cctx_T *cctx)
{
    int lnum = cctx->ctx_lnum;

    while (++lnum < cctx->ctx_ufunc->uf_lines.ga_len)
    {
	char_u *line = ((char_u **)cctx->ctx_ufunc->uf_lines.ga_data)[lnum];
	char_u *p;

	// ignore NULLs inserted for continuation lines
	if (line != NULL)
	{
	    p = skipwhite(line);
	    if (vim9_bad_comment(p))
		return NULL;
	    if (*p != NUL && !vim9_comment_start(p))
		return p;
	}
    }
    return NULL;
}

/*
 * Called when checking for a following operator at "arg".  When the rest of
 * the line is empty or only a comment, peek the next line.  If there is a next
 * line return a pointer to it and set "nextp".
 * Otherwise skip over white space.
 */
    static char_u *
may_peek_next_line(cctx_T *cctx, char_u *arg, char_u **nextp)
{
    char_u *p = skipwhite(arg);

    *nextp = NULL;
    if (*p == NUL || (VIM_ISWHITE(*arg) && vim9_comment_start(p)))
    {
	*nextp = peek_next_line_from_context(cctx);
	if (*nextp != NULL)
	    return *nextp;
    }
    return p;
}

/*
 * Get the next line of the function from "cctx".
 * Skips over empty lines.  Skips over comment lines if "skip_comment" is TRUE.
 * Returns NULL when at the end.
 */
    char_u *
next_line_from_context(cctx_T *cctx, int skip_comment)
{
    char_u	*line;

    do
    {
	++cctx->ctx_lnum;
	if (cctx->ctx_lnum >= cctx->ctx_ufunc->uf_lines.ga_len)
	{
	    line = NULL;
	    break;
	}
	line = ((char_u **)cctx->ctx_ufunc->uf_lines.ga_data)[cctx->ctx_lnum];
	cctx->ctx_line_start = line;
	SOURCING_LNUM = cctx->ctx_lnum + 1;
    } while (line == NULL || *skipwhite(line) == NUL
		     || (skip_comment && vim9_comment_start(skipwhite(line))));
    return line;
}

/*
 * Skip over white space at "whitep" and assign to "*arg".
 * If "*arg" is at the end of the line, advance to the next line.
 * Also when "whitep" points to white space and "*arg" is on a "#".
 * Return FAIL if beyond the last line, "*arg" is unmodified then.
 */
    static int
may_get_next_line(char_u *whitep, char_u **arg, cctx_T *cctx)
{
    *arg = skipwhite(whitep);
    if (vim9_bad_comment(*arg))
	return FAIL;
    if (**arg == NUL || (VIM_ISWHITE(*whitep) && vim9_comment_start(*arg)))
    {
	char_u *next = next_line_from_context(cctx, TRUE);

	if (next == NULL)
	    return FAIL;
	*arg = skipwhite(next);
    }
    return OK;
}

/*
 * Idem, and give an error when failed.
 */
    static int
may_get_next_line_error(char_u *whitep, char_u **arg, cctx_T *cctx)
{
    if (may_get_next_line(whitep, arg, cctx) == FAIL)
    {
	SOURCING_LNUM = cctx->ctx_lnum + 1;
	emsg(_(e_line_incomplete));
	return FAIL;
    }
    return OK;
}


// Structure passed between the compile_expr* functions to keep track of
// constants that have been parsed but for which no code was produced yet.  If
// possible expressions on these constants are applied at compile time.  If
// that is not possible, the code to push the constants needs to be generated
// before other instructions.
// Using 50 should be more than enough of 5 levels of ().
#define PPSIZE 50
typedef struct {
    typval_T	pp_tv[PPSIZE];	// stack of ppconst constants
    int		pp_used;	// active entries in pp_tv[]
    int		pp_is_const;	// all generated code was constants, used for a
				// list or dict with constant members
} ppconst_T;

static int compile_expr0_ext(char_u **arg,  cctx_T *cctx, int *is_const);
static int compile_expr0(char_u **arg,  cctx_T *cctx);
static int compile_expr1(char_u **arg,  cctx_T *cctx, ppconst_T *ppconst);

/*
 * Generate a PUSH instruction for "tv".
 * "tv" will be consumed or cleared.
 * Nothing happens if "tv" is NULL or of type VAR_UNKNOWN;
 */
    static int
generate_tv_PUSH(cctx_T *cctx, typval_T *tv)
{
    if (tv != NULL)
    {
	switch (tv->v_type)
	{
	    case VAR_UNKNOWN:
		break;
	    case VAR_BOOL:
		generate_PUSHBOOL(cctx, tv->vval.v_number);
		break;
	    case VAR_SPECIAL:
		generate_PUSHSPEC(cctx, tv->vval.v_number);
		break;
	    case VAR_NUMBER:
		generate_PUSHNR(cctx, tv->vval.v_number);
		break;
#ifdef FEAT_FLOAT
	    case VAR_FLOAT:
		generate_PUSHF(cctx, tv->vval.v_float);
		break;
#endif
	    case VAR_BLOB:
		generate_PUSHBLOB(cctx, tv->vval.v_blob);
		tv->vval.v_blob = NULL;
		break;
	    case VAR_STRING:
		generate_PUSHS(cctx, tv->vval.v_string);
		tv->vval.v_string = NULL;
		break;
	    default:
		iemsg("constant type not supported");
		clear_tv(tv);
		return FAIL;
	}
	tv->v_type = VAR_UNKNOWN;
    }
    return OK;
}

/*
 * Generate code for any ppconst entries.
 */
    static int
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
 * Check that the last item of "ppconst" is a bool.
 */
    static int
check_ppconst_bool(ppconst_T *ppconst)
{
    if (ppconst->pp_used > 0)
    {
	typval_T    *tv = &ppconst->pp_tv[ppconst->pp_used - 1];
	where_T	    where;

	where.wt_index = 0;
	where.wt_variable = FALSE;
	return check_typval_type(&t_bool, tv, where);
    }
    return OK;
}

/*
 * Clear ppconst constants.  Used when failing.
 */
    static void
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
 */
    static int
compile_member(int is_slice, cctx_T *cctx)
{
    type_T	**typep;
    garray_T	*stack = &cctx->ctx_type_stack;
    vartype_T	vartype;
    type_T	*idxtype;

    // We can index a list, dict and blob.  If we don't know the type
    // we can use the index value type.  If we still don't know use an "ANY"
    // instruction.
    typep = ((type_T **)stack->ga_data) + stack->ga_len
						  - (is_slice ? 3 : 2);
    vartype = (*typep)->tt_type;
    idxtype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    // If the index is a string, the variable must be a Dict.
    if (*typep == &t_any && idxtype == &t_string)
	vartype = VAR_DICT;
    if (vartype == VAR_STRING || vartype == VAR_LIST || vartype == VAR_BLOB)
    {
	if (need_type(idxtype, &t_number, -1, 0, cctx, FALSE, FALSE) == FAIL)
	    return FAIL;
	if (is_slice)
	{
	    idxtype = ((type_T **)stack->ga_data)[stack->ga_len - 2];
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
	if ((*typep)->tt_type == VAR_DICT)
	{
	    *typep = (*typep)->tt_member;
	    if (*typep == &t_unknown)
		// empty dict was used
		*typep = &t_any;
	}
	else
	{
	    if (need_type(*typep, &t_dict_any, -2, 0, cctx,
							 FALSE, FALSE) == FAIL)
		return FAIL;
	    *typep = &t_any;
	}
	if (may_generate_2STRING(-1, FALSE, cctx) == FAIL)
	    return FAIL;
	if (generate_instr_drop(cctx, ISN_MEMBER, 1) == FAIL)
	    return FAIL;
    }
    else if (vartype == VAR_STRING)
    {
	*typep = &t_string;
	if ((is_slice
		? generate_instr_drop(cctx, ISN_STRSLICE, 2)
		: generate_instr_drop(cctx, ISN_STRINDEX, 1)) == FAIL)
	    return FAIL;
    }
    else if (vartype == VAR_BLOB)
    {
	if (is_slice)
	{
	    *typep = &t_blob;
	    if (generate_instr_drop(cctx, ISN_BLOBSLICE, 2) == FAIL)
		return FAIL;
	}
	else
	{
	    *typep = &t_number;
	    if (generate_instr_drop(cctx, ISN_BLOBINDEX, 1) == FAIL)
		return FAIL;
	}
    }
    else if (vartype == VAR_LIST || *typep == &t_any)
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
	    if ((*typep)->tt_type == VAR_LIST)
	    {
		*typep = (*typep)->tt_member;
		if (*typep == &t_unknown)
		    // empty list was used
		    *typep = &t_any;
	    }
	    if (generate_instr_drop(cctx,
			vartype == VAR_LIST ?  ISN_LISTINDEX : ISN_ANYINDEX, 1)
								       == FAIL)
		return FAIL;
	}
    }
    else
    {
	emsg(_(e_string_list_dict_or_blob_required));
	return FAIL;
    }
    return OK;
}

/*
 * Generate an instruction to load script-local variable "name", without the
 * leading "s:".
 * Also finds imported variables.
 */
    static int
compile_load_scriptvar(
	cctx_T *cctx,
	char_u *name,	    // variable NUL terminated
	char_u *start,	    // start of variable
	char_u **end,	    // end of variable
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

    import = find_imported(name, 0, cctx);
    if (import != NULL)
    {
	if (import->imp_flags & IMP_FLAGS_STAR)
	{
	    char_u	*p = skipwhite(*end);
	    char_u	*exp_name;
	    int		cc;
	    ufunc_T	*ufunc;
	    type_T	*type;

	    // Used "import * as Name", need to lookup the member.
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
		if (*p == '(' && ufunc != NULL)
		{
		    generate_PUSHFUNC(cctx, ufunc->uf_name, import->imp_type);
		    return OK;
		}
		return FAIL;
	    }

	    generate_VIM9SCRIPT(cctx, ISN_LOADSCRIPT,
		    import->imp_sid,
		    idx,
		    type);
	}
	else if (import->imp_funcname != NULL)
	    generate_PUSHFUNC(cctx, import->imp_funcname, import->imp_type);
	else
	    generate_VIM9SCRIPT(cctx, ISN_LOADSCRIPT,
		    import->imp_sid,
		    import->imp_var_vals_idx,
		    import->imp_type);
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
    static int
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

    static void
clear_instr_ga(garray_T *gap)
{
    int idx;

    for (idx = 0; idx < gap->ga_len; ++idx)
	delete_instr(((isn_T *)gap->ga_data) + idx);
    ga_clear(gap);
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

    // Temporarily reset the list of instructions so that the jump labels are
    // correct.
    cctx->ctx_instr.ga_len = 0;
    cctx->ctx_instr.ga_maxlen = 0;
    cctx->ctx_instr.ga_data = NULL;
    expr_res = compile_expr0(&s, cctx);
    s = skipwhite(s);
    trailing_error = *s != NUL;

    if (expr_res == FAIL || trailing_error
				       || ga_grow(&cctx->ctx_instr, 1) == FAIL)
    {
	if (trailing_error)
	    semsg(_(e_trailing_arg), s);
	clear_instr_ga(&cctx->ctx_instr);
	cctx->ctx_instr = save_ga;
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

	if (is_searchpair && *argcount >= 5
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
    emsg(_(e_missing_close));
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

    // we can evaluate "has('name')" at compile time
    if (varlen == 3 && STRNCMP(*arg, "has", 3) == 0)
    {
	char_u	    *s = skipwhite(*arg + varlen + 1);
	typval_T    argvars[2];

	argvars[0].v_type = VAR_UNKNOWN;
	if (*s == '"')
	    (void)eval_string(&s, &argvars[0], TRUE);
	else if (*s == '\'')
	    (void)eval_lit_string(&s, &argvars[0], TRUE);
	s = skipwhite(s);
	if (*s == ')' && argvars[0].v_type == VAR_STRING
		&& !dynamic_feature(argvars[0].vval.v_string))
	{
	    typval_T	*tv = &ppconst->pp_tv[ppconst->pp_used];

	    *arg = s + 1;
	    argvars[1].v_type = VAR_UNKNOWN;
	    tv->v_type = VAR_NUMBER;
	    tv->vval.v_number = 0;
	    f_has(argvars, tv);
	    clear_tv(&argvars[0]);
	    ++ppconst->pp_used;
	    return OK;
	}
	clear_tv(&argvars[0]);
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
		garray_T    *stack = &cctx->ctx_type_stack;
		type_T	    *type = ((type_T **)stack->ga_data)[
							    stack->ga_len - 2];

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
	    semsg(_(e_unknownfunc), namebuf);
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
	garray_T    *stack = &cctx->ctx_type_stack;
	type_T	    *type = ((type_T **)stack->ga_data)[stack->ga_len - 1];

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
	semsg(_(e_unknownfunc), namebuf);

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
 * This intentionally does not handle line continuation.
 */
    char_u *
to_name_const_end(char_u *arg)
{
    char_u	*p = to_name_end(arg, TRUE);
    typval_T	rettv;

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
	    semsg(_(e_list_end), *arg);
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

    CLEAR_FIELD(evalarg);
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

#ifdef FEAT_PROFILE
    // When the outer function is compiled for profiling, the lambda may be
    // called without profiling.  Compile it here in the right context.
    if (cctx->ctx_compile_type == CT_PROFILE)
	compile_def_function(ufunc, FALSE, CT_NONE, cctx);
#endif

    // evalarg.eval_tofree_cmdline may have a copy of the last line and "*arg"
    // points into it.  Point to the original line to avoid a dangling pointer.
    if (evalarg.eval_tofree_cmdline != NULL)
    {
	size_t	off = *arg - evalarg.eval_tofree_cmdline;

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
	    if (generate_PUSHS(cctx, key) == FAIL)
		return FAIL;
	}

	// Check for duplicate keys, if using string keys.
	if (key != NULL)
	{
	    item = dict_find(d, key, -1);
	    if (item != NULL)
	    {
		semsg(_(e_duplicate_key), key);
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
		semsg(_(e_missing_dict_colon), *arg);
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
	    semsg(_(e_missing_dict_comma), *arg);
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
	semsg(_(e_syntax_error_at_str), start - 1);
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
    static void
skip_expr_cctx(char_u **arg, cctx_T *cctx)
{
    evalarg_T	evalarg;

    CLEAR_FIELD(evalarg);
    evalarg.eval_cctx = cctx;
    skip_expr(arg, &evalarg);
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
	    int	    negate = *p == '-';
	    isn_T   *isn;

	    // TODO: check type
	    while (p > start && (p[-1] == '-' || p[-1] == '+'))
	    {
		--p;
		if (*p == '-')
		    negate = !negate;
	    }
	    // only '-' has an effect, for '+' we only check the type
	    if (negate)
		isn = generate_instr(cctx, ISN_NEGATENR);
	    else
		isn = generate_instr(cctx, ISN_CHECKNR);
	    if (isn == NULL)
		return FAIL;
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
	emsg(_(e_missing_close));
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

	// Do not skip over white space to find the "(", "execute 'x' ()" is
	// not a function call.
	if (**arg == '(')
	{
	    garray_T    *stack = &cctx->ctx_type_stack;
	    type_T	*type;
	    int		argcount = 0;

	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;
	    ppconst->pp_is_const = FALSE;

	    // funcref(arg)
	    type = ((type_T **)stack->ga_data)[stack->ga_len - 1];

	    *arg = skipwhite(p + 1);
	    if (compile_arguments(arg, cctx, &argcount, FALSE) == FAIL)
		return FAIL;
	    if (generate_PCALL(cctx, argcount, name_start, type, TRUE) == FAIL)
		return FAIL;
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
			emsg(_(e_nowhitespace));
		    else
			semsg(_(e_missing_paren), *arg);
		    return FAIL;
		}
		*arg = skipwhite(*arg + 1);
		if (compile_arguments(arg, cctx, &argcount, FALSE) == FAIL)
		    return FAIL;

		// Move the instructions for the arguments to before the
		// instructions of the expression and move the type of the
		// expression after the argument types.  This is what ISN_PCALL
		// expects.
		stack = &cctx->ctx_type_stack;
		arg_isn_count = cctx->ctx_instr.ga_len - expr_isn_end;
		if (arg_isn_count > 0)
		{
		    int	    expr_isn_count = expr_isn_end - expr_isn_start;
		    isn_T   *isn = ALLOC_MULT(isn_T, expr_isn_count);

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

		    type = ((type_T **)stack->ga_data)[type_idx_start];
		    mch_memmove(((type_T **)stack->ga_data) + type_idx_start,
			      ((type_T **)stack->ga_data) + type_idx_start + 1,
			      sizeof(type_T *)
				       * (stack->ga_len - type_idx_start - 1));
		    ((type_T **)stack->ga_data)[stack->ga_len - 1] = type;
		}

		type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
		if (generate_PCALL(cctx, argcount, p - 2, type, FALSE) == FAIL)
		    return FAIL;
	    }
	    else
	    {
		// method call:  list->method()
		p = *arg;
		if (!eval_isnamec1(*p))
		{
		    semsg(_(e_trailing_arg), pstart);
		    return FAIL;
		}
		if (ASCII_ISALPHA(*p) && p[1] == ':')
		    p += 2;
		for ( ; eval_isnamec(*p); ++p)
		    ;
		if (*p != '(')
		{
		    semsg(_(e_missing_paren), *arg);
		    return FAIL;
		}
		if (compile_call(arg, p - *arg, cctx, ppconst, 1) == FAIL)
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
	    // TODO: more arguments
	    // TODO: recognize list or dict at runtime
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
		emsg(_(e_missbrac));
		return FAIL;
	    }
	    *arg = *arg + 1;

	    if (compile_member(is_slice, cctx) == FAIL)
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
	    if (generate_STRINGMEMBER(cctx, *arg, p - *arg) == FAIL)
		return FAIL;
	    *arg = p;
	}
	else
	    break;
    }

    // TODO - see handle_subscript():
    // Turn "dict.Func" into a partial for "Func" bound to "dict".
    // Don't do this when "Func" is already a partial that was bound
    // explicitly (pt_auto is FALSE).

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
compile_expr7(
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
	    if (generate_ppconst(cctx, ppconst) == FAIL)
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
 * <type>expr7: runtime type check / conversion
 */
    static int
compile_expr7t(char_u **arg, cctx_T *cctx, ppconst_T *ppconst)
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

    if (compile_expr7(arg, cctx, ppconst) == FAIL)
	return FAIL;

    if (want_type != NULL)
    {
	garray_T    *stack = &cctx->ctx_type_stack;
	type_T	    *actual;
	where_T	    where;

	generate_ppconst(cctx, ppconst);
	actual = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	where.wt_index = 0;
	where.wt_variable = FALSE;
	if (check_type(want_type, actual, FALSE, where) == FAIL)
	{
	    if (need_type(actual, want_type, -1, 0, cctx, FALSE, FALSE) == FAIL)
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
    if (compile_expr7t(arg, cctx, ppconst) == FAIL)
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
	if (compile_expr7t(arg, cctx, ppconst) == FAIL)
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
	    int		status;

	    if (next != NULL)
	    {
		*arg = next_line_from_context(cctx, TRUE);
		p = skipwhite(*arg);
	    }

	    if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(p[2]))
	    {
		semsg(_(e_white_space_required_before_and_after_str_at_str),
									op, p);
		return FAIL;
	    }

	    save_sourcing_lnum = SOURCING_LNUM;
	    SOURCING_LNUM = start_lnum;
	    save_lnum = cctx->ctx_lnum;
	    cctx->ctx_lnum = start_ctx_lnum;

	    status = check_ppconst_bool(ppconst);
	    if (status == OK)
	    {
		// TODO: use ppconst if the value is a constant
		generate_ppconst(cctx, ppconst);

		// Every part must evaluate to a bool.
		status = (bool_on_stack(cctx));
		if (status == OK)
		    status = ga_grow(&end_ga, 1);
	    }
	    cctx->ctx_lnum = save_lnum;
	    if (status == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	    *(((int *)end_ga.ga_data) + end_ga.ga_len) = instr->ga_len;
	    ++end_ga.ga_len;
	    generate_JUMP(cctx, opchar == '|'
				 ?  JUMP_IF_COND_TRUE : JUMP_IF_COND_FALSE, 0);

	    // eval the next expression
	    SOURCING_LNUM = save_sourcing_lnum;
	    if (may_get_next_line_error(p + 2, arg, cctx) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	    if ((opchar == '|' ? compile_expr3(arg, cctx, ppconst)
				  : compile_expr4(arg, cctx, ppconst)) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	    p = may_peek_next_line(cctx, *arg, &next);
	}

	if (check_ppconst_bool(ppconst) == FAIL)
	{
	    ga_clear(&end_ga);
	    return FAIL;
	}
	generate_ppconst(cctx, ppconst);

	// Every part must evaluate to a bool.
	if (bool_on_stack(cctx) == FAIL)
	{
	    ga_clear(&end_ga);
	    return FAIL;
	}

	// Fill in the end label in all jumps.
	while (end_ga.ga_len > 0)
	{
	    isn_T	*isn;

	    --end_ga.ga_len;
	    isn = ((isn_T *)instr->ga_data)
				  + *(((int *)end_ga.ga_data) + end_ga.ga_len);
	    isn->isn_arg.jump.jump_where = instr->ga_len;
	}
	ga_clear(&end_ga);
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
    static int
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
		type1 = ((type_T **)stack->ga_data)[stack->ga_len];
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
		--stack->ga_len;
		type1 = ((type_T **)stack->ga_data)[stack->ga_len];

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
		emsg(_(e_missing_colon));
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

	    // If the types differ, the result has a more generic type.
	    typep = ((type_T **)stack->ga_data) + stack->ga_len - 1;
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
    static int
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
    static int
compile_expr0(char_u **arg,  cctx_T *cctx)
{
    return compile_expr0_ext(arg, cctx, NULL);
}

/*
 * Compile "return [expr]".
 * When "legacy" is TRUE evaluate [expr] with legacy syntax
 */
    static char_u *
compile_return(char_u *arg, int check_return_type, int legacy, cctx_T *cctx)
{
    char_u	*p = arg;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*stack_type;

    if (*p != NUL && *p != '|' && *p != '\n')
    {
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
	    stack_type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    if ((check_return_type && (cctx->ctx_ufunc->uf_ret_type == NULL
				|| cctx->ctx_ufunc->uf_ret_type == &t_unknown
				|| cctx->ctx_ufunc->uf_ret_type == &t_any))
		    || (!check_return_type
				&& cctx->ctx_ufunc->uf_ret_type == &t_unknown))
	    {
		cctx->ctx_ufunc->uf_ret_type = stack_type;
	    }
	    else
	    {
		if (cctx->ctx_ufunc->uf_ret_type->tt_type == VAR_VOID
			&& stack_type->tt_type != VAR_VOID
			&& stack_type->tt_type != VAR_UNKNOWN)
		{
		    emsg(_(e_returning_value_in_function_without_return_type));
		    return NULL;
		}
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
 * Get a line from the compilation context, compatible with exarg_T getline().
 * Return a pointer to the line in allocated memory.
 * Return NULL for end-of-file or some error.
 */
    static char_u *
exarg_getline(
	int c UNUSED,
	void *cookie,
	int indent UNUSED,
	getline_opt_T options UNUSED)
{
    cctx_T  *cctx = (cctx_T *)cookie;
    char_u  *p;

    for (;;)
    {
	if (cctx->ctx_lnum >= cctx->ctx_ufunc->uf_lines.ga_len - 1)
	    return NULL;
	++cctx->ctx_lnum;
	p = ((char_u **)cctx->ctx_ufunc->uf_lines.ga_data)[cctx->ctx_lnum];
	// Comment lines result in NULL pointers, skip them.
	if (p != NULL)
	    return vim_strsave(p);
    }
}

    void
fill_exarg_from_cctx(exarg_T *eap, cctx_T *cctx)
{
    eap->getline = exarg_getline;
    eap->cookie = cctx;
}

/*
 * Compile a nested :def command.
 */
    static char_u *
compile_nested_function(exarg_T *eap, cctx_T *cctx)
{
    int		is_global = *eap->arg == 'g' && eap->arg[1] == ':';
    char_u	*name_start = eap->arg;
    char_u	*name_end = to_name_end(eap->arg, TRUE);
    char_u	*lambda_name;
    ufunc_T	*ufunc;
    int		r = FAIL;
    compiletype_T   compile_type;

    if (eap->forceit)
    {
	emsg(_(e_cannot_use_bang_with_nested_def));
	return NULL;
    }

    if (*name_start == '/')
    {
	name_end = skip_regexp(name_start + 1, '/', TRUE);
	if (*name_end == '/')
	    ++name_end;
	eap->nextcmd = check_nextcmd(name_end);
    }
    if (name_end == name_start || *skipwhite(name_end) != '(')
    {
	if (!ends_excmd2(name_start, name_end))
	{
	    semsg(_(e_invalid_command_str), eap->cmd);
	    return NULL;
	}

	// "def" or "def Name": list functions
	if (generate_DEF(cctx, name_start, name_end - name_start) == FAIL)
	    return NULL;
	return eap->nextcmd == NULL ? (char_u *)"" : eap->nextcmd;
    }

    // Only g:Func() can use a namespace.
    if (name_start[1] == ':' && !is_global)
    {
	semsg(_(e_namespace_not_supported_str), name_start);
	return NULL;
    }
    if (check_defined(name_start, name_end - name_start, cctx, FALSE) == FAIL)
	return NULL;

    eap->arg = name_end;
    fill_exarg_from_cctx(eap, cctx);

    eap->forceit = FALSE;
    lambda_name = vim_strsave(get_lambda_name());
    if (lambda_name == NULL)
	return NULL;
    ufunc = define_function(eap, lambda_name);

    if (ufunc == NULL)
    {
	r = eap->skip ? OK : FAIL;
	goto theend;
    }

    // copy over the block scope IDs before compiling
    if (!is_global && cctx->ctx_ufunc->uf_block_depth > 0)
    {
	int block_depth = cctx->ctx_ufunc->uf_block_depth;

	ufunc->uf_block_ids = ALLOC_MULT(int, block_depth);
	if (ufunc->uf_block_ids != NULL)
	{
	    mch_memmove(ufunc->uf_block_ids, cctx->ctx_ufunc->uf_block_ids,
						    sizeof(int) * block_depth);
	    ufunc->uf_block_depth = block_depth;
	}
    }

    compile_type = COMPILE_TYPE(ufunc);
#ifdef FEAT_PROFILE
    // If the outer function is profiled, also compile the nested function for
    // profiling.
    if (cctx->ctx_compile_type == CT_PROFILE)
	compile_type = CT_PROFILE;
#endif
    if (func_needs_compiling(ufunc, compile_type)
	    && compile_def_function(ufunc, TRUE, compile_type, cctx) == FAIL)
    {
	func_ptr_unref(ufunc);
	goto theend;
    }

#ifdef FEAT_PROFILE
    // When the outer function is compiled for profiling, the nested function
    // may be called without profiling.  Compile it here in the right context.
    if (compile_type == CT_PROFILE && func_needs_compiling(ufunc, CT_NONE))
	compile_def_function(ufunc, FALSE, CT_NONE, cctx);
#endif

    if (is_global)
    {
	char_u *func_name = vim_strnsave(name_start + 2,
						    name_end - name_start - 2);

	if (func_name == NULL)
	    r = FAIL;
	else
	{
	    r = generate_NEWFUNC(cctx, lambda_name, func_name);
	    lambda_name = NULL;
	}
    }
    else
    {
	// Define a local variable for the function reference.
	lvar_T	*lvar = reserve_local(cctx, name_start, name_end - name_start,
						    TRUE, ufunc->uf_func_type);

	if (lvar == NULL)
	    goto theend;
	if (generate_FUNCREF(cctx, ufunc) == FAIL)
	    goto theend;
	r = generate_STORE(cctx, ISN_STORE, lvar->lv_idx, NULL);
    }
    // TODO: warning for trailing text?

theend:
    vim_free(lambda_name);
    return r == FAIL ? NULL : (char_u *)"";
}

/*
 * Return the length of an assignment operator, or zero if there isn't one.
 */
    int
assignment_len(char_u *p, int *heredoc)
{
    if (*p == '=')
    {
	if (p[1] == '<' && p[2] == '<')
	{
	    *heredoc = TRUE;
	    return 3;
	}
	return 1;
    }
    if (vim_strchr((char_u *)"+-*/%", *p) != NULL && p[1] == '=')
	return 2;
    if (STRNCMP(p, "..=", 3) == 0)
	return 3;
    return 0;
}

/*
 * Generate the load instruction for "name".
 */
    static void
generate_loadvar(
	cctx_T		*cctx,
	assign_dest_T	dest,
	char_u		*name,
	lvar_T		*lvar,
	type_T		*type)
{
    switch (dest)
    {
	case dest_option:
	    // TODO: check the option exists
	    generate_LOAD(cctx, ISN_LOADOPT, 0, name, type);
	    break;
	case dest_global:
	    if (vim_strchr(name, AUTOLOAD_CHAR) == NULL)
		generate_LOAD(cctx, ISN_LOADG, 0, name + 2, type);
	    else
		generate_LOAD(cctx, ISN_LOADAUTO, 0, name, type);
	    break;
	case dest_buffer:
	    generate_LOAD(cctx, ISN_LOADB, 0, name + 2, type);
	    break;
	case dest_window:
	    generate_LOAD(cctx, ISN_LOADW, 0, name + 2, type);
	    break;
	case dest_tab:
	    generate_LOAD(cctx, ISN_LOADT, 0, name + 2, type);
	    break;
	case dest_script:
	    compile_load_scriptvar(cctx,
		    name + (name[1] == ':' ? 2 : 0), NULL, NULL, TRUE);
	    break;
	case dest_env:
	    // Include $ in the name here
	    generate_LOAD(cctx, ISN_LOADENV, 0, name, type);
	    break;
	case dest_reg:
	    generate_LOAD(cctx, ISN_LOADREG, name[1], NULL, &t_string);
	    break;
	case dest_vimvar:
	    generate_LOADV(cctx, name + 2, TRUE);
	    break;
	case dest_local:
	    if (lvar->lv_from_outer > 0)
		generate_LOADOUTER(cctx, lvar->lv_idx, lvar->lv_from_outer,
									 type);
	    else
		generate_LOAD(cctx, ISN_LOAD, lvar->lv_idx, NULL, type);
	    break;
	case dest_expr:
	    // list or dict value should already be on the stack.
	    break;
    }
}

/*
 * Skip over "[expr]" or ".member".
 * Does not check for any errors.
 */
    static char_u *
skip_index(char_u *start)
{
    char_u *p = start;

    if (*p == '[')
    {
	p = skipwhite(p + 1);
	(void)skip_expr(&p, NULL);
	p = skipwhite(p);
	if (*p == ']')
	    return p + 1;
	return p;
    }
    // if (*p == '.')
    return to_name_end(p + 1, TRUE);
}

    void
vim9_declare_error(char_u *name)
{
    char *scope = "";

    switch (*name)
    {
	case 'g': scope = _("global"); break;
	case 'b': scope = _("buffer"); break;
	case 'w': scope = _("window"); break;
	case 't': scope = _("tab"); break;
	case 'v': scope = "v:"; break;
	case '$': semsg(_(e_cannot_declare_an_environment_variable), name);
		  return;
	case '&': semsg(_(e_cannot_declare_an_option), name);
		  return;
	case '@': semsg(_(e_cannot_declare_a_register_str), name);
		  return;
	default: return;
    }
    semsg(_(e_cannot_declare_a_scope_variable), scope, name);
}

/*
 * For one assignment figure out the type of destination.  Return it in "dest".
 * When not recognized "dest" is not set.
 * For an option "opt_flags" is set.
 * For a v:var "vimvaridx" is set.
 * "type" is set to the destination type if known, unchanted otherwise.
 * Return FAIL if an error message was given.
 */
    static int
get_var_dest(
	char_u		*name,
	assign_dest_T	*dest,
	int		cmdidx,
	int		*opt_flags,
	int		*vimvaridx,
	type_T		**type,
	cctx_T		*cctx)
{
    char_u *p;

    if (*name == '&')
    {
	int		cc;
	long		numval;
	getoption_T	opt_type;

	*dest = dest_option;
	if (cmdidx == CMD_final || cmdidx == CMD_const)
	{
	    emsg(_(e_const_option));
	    return FAIL;
	}
	p = name;
	p = find_option_end(&p, opt_flags);
	if (p == NULL)
	{
	    // cannot happen?
	    emsg(_(e_unexpected_characters_in_assignment));
	    return FAIL;
	}
	cc = *p;
	*p = NUL;
	opt_type = get_option_value(skip_option_env_lead(name),
						    &numval, NULL, *opt_flags);
	*p = cc;
	switch (opt_type)
	{
	    case gov_unknown:
		    semsg(_(e_unknown_option), name);
		    return FAIL;
	    case gov_string:
	    case gov_hidden_string:
		    *type = &t_string;
		    break;
	    case gov_bool:
	    case gov_hidden_bool:
		    *type = &t_bool;
		    break;
	    case gov_number:
	    case gov_hidden_number:
		    *type = &t_number;
		    break;
	}
    }
    else if (*name == '$')
    {
	*dest = dest_env;
	*type = &t_string;
    }
    else if (*name == '@')
    {
	if (name[1] != '@'
			&& (!valid_yank_reg(name[1], FALSE) || name[1] == '.'))
	{
	    emsg_invreg(name[1]);
	    return FAIL;
	}
	*dest = dest_reg;
	*type = name[1] == '#' ? &t_number_or_string : &t_string;
    }
    else if (STRNCMP(name, "g:", 2) == 0)
    {
	*dest = dest_global;
    }
    else if (STRNCMP(name, "b:", 2) == 0)
    {
	*dest = dest_buffer;
    }
    else if (STRNCMP(name, "w:", 2) == 0)
    {
	*dest = dest_window;
    }
    else if (STRNCMP(name, "t:", 2) == 0)
    {
	*dest = dest_tab;
    }
    else if (STRNCMP(name, "v:", 2) == 0)
    {
	typval_T	*vtv;
	int		di_flags;

	*vimvaridx = find_vim_var(name + 2, &di_flags);
	if (*vimvaridx < 0)
	{
	    semsg(_(e_variable_not_found_str), name);
	    return FAIL;
	}
	// We use the current value of "sandbox" here, is that OK?
	if (var_check_ro(di_flags, name, FALSE))
	    return FAIL;
	*dest = dest_vimvar;
	vtv = get_vim_var_tv(*vimvaridx);
	*type = typval2type_vimvar(vtv, cctx->ctx_type_list);
    }
    return OK;
}

/*
 * Generate a STORE instruction for "dest", not being "dest_local".
 * Return FAIL when out of memory.
 */
    static int
generate_store_var(
	cctx_T		*cctx,
	assign_dest_T	dest,
	int		opt_flags,
	int		vimvaridx,
	int		scriptvar_idx,
	int		scriptvar_sid,
	type_T		*type,
	char_u		*name)
{
    switch (dest)
    {
	case dest_option:
	    return generate_STOREOPT(cctx, skip_option_env_lead(name),
								    opt_flags);
	case dest_global:
	    // include g: with the name, easier to execute that way
	    return generate_STORE(cctx, vim_strchr(name, AUTOLOAD_CHAR) == NULL
					? ISN_STOREG : ISN_STOREAUTO, 0, name);
	case dest_buffer:
	    // include b: with the name, easier to execute that way
	    return generate_STORE(cctx, ISN_STOREB, 0, name);
	case dest_window:
	    // include w: with the name, easier to execute that way
	    return generate_STORE(cctx, ISN_STOREW, 0, name);
	case dest_tab:
	    // include t: with the name, easier to execute that way
	    return generate_STORE(cctx, ISN_STORET, 0, name);
	case dest_env:
	    return generate_STORE(cctx, ISN_STOREENV, 0, name + 1);
	case dest_reg:
	    return generate_STORE(cctx, ISN_STOREREG,
					 name[1] == '@' ? '"' : name[1], NULL);
	case dest_vimvar:
	    return generate_STORE(cctx, ISN_STOREV, vimvaridx, NULL);
	case dest_script:
	    if (scriptvar_idx < 0)
		// "s:" may be included in the name.
		return generate_OLDSCRIPT(cctx, ISN_STORES, name,
							  scriptvar_sid, type);
	    return generate_VIM9SCRIPT(cctx, ISN_STORESCRIPT,
					   scriptvar_sid, scriptvar_idx, type);
	case dest_local:
	case dest_expr:
	    // cannot happen
	    break;
    }
    return FAIL;
}

    static int
generate_store_lhs(cctx_T *cctx, lhs_T *lhs, int instr_count)
{
    if (lhs->lhs_dest != dest_local)
	return generate_store_var(cctx, lhs->lhs_dest,
			    lhs->lhs_opt_flags, lhs->lhs_vimvaridx,
			    lhs->lhs_scriptvar_idx, lhs->lhs_scriptvar_sid,
			    lhs->lhs_type, lhs->lhs_name);

    if (lhs->lhs_lvar != NULL)
    {
	garray_T	*instr = &cctx->ctx_instr;
	isn_T		*isn = ((isn_T *)instr->ga_data) + instr->ga_len - 1;

	// optimization: turn "var = 123" from ISN_PUSHNR + ISN_STORE into
	// ISN_STORENR
	if (lhs->lhs_lvar->lv_from_outer == 0
		&& instr->ga_len == instr_count + 1
		&& isn->isn_type == ISN_PUSHNR)
	{
	    varnumber_T val = isn->isn_arg.number;
	    garray_T    *stack = &cctx->ctx_type_stack;

	    isn->isn_type = ISN_STORENR;
	    isn->isn_arg.storenr.stnr_idx = lhs->lhs_lvar->lv_idx;
	    isn->isn_arg.storenr.stnr_val = val;
	    if (stack->ga_len > 0)
		--stack->ga_len;
	}
	else if (lhs->lhs_lvar->lv_from_outer > 0)
	    generate_STOREOUTER(cctx, lhs->lhs_lvar->lv_idx,
						 lhs->lhs_lvar->lv_from_outer);
	else
	    generate_STORE(cctx, ISN_STORE, lhs->lhs_lvar->lv_idx, NULL);
    }
    return OK;
}

    static int
is_decl_command(int cmdidx)
{
    return cmdidx == CMD_let || cmdidx == CMD_var
				 || cmdidx == CMD_final || cmdidx == CMD_const;
}

/*
 * Figure out the LHS type and other properties for an assignment or one item
 * of ":unlet" with an index.
 * Returns OK or FAIL.
 */
    static int
compile_lhs(
	char_u	*var_start,
	lhs_T	*lhs,
	int	cmdidx,
	int	heredoc,
	int	oplen,
	cctx_T	*cctx)
{
    char_u	*var_end;
    int		is_decl = is_decl_command(cmdidx);

    CLEAR_POINTER(lhs);
    lhs->lhs_dest = dest_local;
    lhs->lhs_vimvaridx = -1;
    lhs->lhs_scriptvar_idx = -1;

    // "dest_end" is the end of the destination, including "[expr]" or
    // ".name".
    // "var_end" is the end of the variable/option/etc. name.
    lhs->lhs_dest_end = skip_var_one(var_start, FALSE);
    if (*var_start == '@')
	var_end = var_start + 2;
    else
    {
	// skip over the leading "&", "&l:", "&g:" and "$"
	var_end = skip_option_env_lead(var_start);
	var_end = to_name_end(var_end, TRUE);
    }

    // "a: type" is declaring variable "a" with a type, not dict "a:".
    if (is_decl && lhs->lhs_dest_end == var_start + 2
					       && lhs->lhs_dest_end[-1] == ':')
	--lhs->lhs_dest_end;
    if (is_decl && var_end == var_start + 2 && var_end[-1] == ':')
	--var_end;

    // compute the length of the destination without "[expr]" or ".name"
    lhs->lhs_varlen = var_end - var_start;
    lhs->lhs_varlen_total = lhs->lhs_varlen;
    lhs->lhs_name = vim_strnsave(var_start, lhs->lhs_varlen);
    if (lhs->lhs_name == NULL)
	return FAIL;

    if (lhs->lhs_dest_end > var_start + lhs->lhs_varlen)
	// Something follows after the variable: "var[idx]" or "var.key".
	lhs->lhs_has_index = TRUE;

    if (heredoc)
	lhs->lhs_type = &t_list_string;
    else
	lhs->lhs_type = &t_any;

    if (cctx->ctx_skip != SKIP_YES)
    {
	int	    declare_error = FALSE;

	if (get_var_dest(lhs->lhs_name, &lhs->lhs_dest, cmdidx,
				      &lhs->lhs_opt_flags, &lhs->lhs_vimvaridx,
						 &lhs->lhs_type, cctx) == FAIL)
	    return FAIL;
	if (lhs->lhs_dest != dest_local
				 && cmdidx != CMD_const && cmdidx != CMD_final)
	{
	    // Specific kind of variable recognized.
	    declare_error = is_decl;
	}
	else
	{
	    // No specific kind of variable recognized, just a name.
	    if (check_reserved_name(lhs->lhs_name) == FAIL)
		return FAIL;

	    if (lookup_local(var_start, lhs->lhs_varlen,
					     &lhs->lhs_local_lvar, cctx) == OK)
		lhs->lhs_lvar = &lhs->lhs_local_lvar;
	    else
	    {
		CLEAR_FIELD(lhs->lhs_arg_lvar);
		if (arg_exists(var_start, lhs->lhs_varlen,
			 &lhs->lhs_arg_lvar.lv_idx, &lhs->lhs_arg_lvar.lv_type,
			    &lhs->lhs_arg_lvar.lv_from_outer, cctx) == OK)
		{
		    if (is_decl)
		    {
			semsg(_(e_str_is_used_as_argument), lhs->lhs_name);
			return FAIL;
		    }
		    lhs->lhs_lvar = &lhs->lhs_arg_lvar;
		}
	    }
	    if (lhs->lhs_lvar != NULL)
	    {
		if (is_decl)
		{
		    semsg(_(e_variable_already_declared), lhs->lhs_name);
		    return FAIL;
		}
	    }
	    else
	    {
		int script_namespace = lhs->lhs_varlen > 1
				       && STRNCMP(var_start, "s:", 2) == 0;
		int script_var = (script_namespace
			? script_var_exists(var_start + 2, lhs->lhs_varlen - 2,
									  cctx)
			  : script_var_exists(var_start, lhs->lhs_varlen,
								  cctx)) == OK;
		imported_T  *import =
			       find_imported(var_start, lhs->lhs_varlen, cctx);

		if (script_namespace || script_var || import != NULL)
		{
		    char_u	*rawname = lhs->lhs_name
					   + (lhs->lhs_name[1] == ':' ? 2 : 0);

		    if (is_decl)
		    {
			if (script_namespace)
			    semsg(_(e_cannot_declare_script_variable_in_function),
								lhs->lhs_name);
			else
			    semsg(_(e_variable_already_declared_in_script_str),
								lhs->lhs_name);
			return FAIL;
		    }
		    else if (cctx->ctx_ufunc->uf_script_ctx_version
							 == SCRIPT_VERSION_VIM9
				    && script_namespace
				    && !script_var && import == NULL)
		    {
			semsg(_(e_unknown_variable_str), lhs->lhs_name);
			return FAIL;
		    }

		    lhs->lhs_dest = dest_script;

		    // existing script-local variables should have a type
		    lhs->lhs_scriptvar_sid = current_sctx.sc_sid;
		    if (import != NULL)
			lhs->lhs_scriptvar_sid = import->imp_sid;
		    if (SCRIPT_ID_VALID(lhs->lhs_scriptvar_sid))
		    {
			// Check writable only when no index follows.
			lhs->lhs_scriptvar_idx = get_script_item_idx(
					       lhs->lhs_scriptvar_sid, rawname,
			      lhs->lhs_has_index ? ASSIGN_FINAL : ASSIGN_CONST,
									 cctx);
			if (lhs->lhs_scriptvar_idx >= 0)
			{
			    scriptitem_T *si = SCRIPT_ITEM(
						       lhs->lhs_scriptvar_sid);
			    svar_T	 *sv =
					    ((svar_T *)si->sn_var_vals.ga_data)
						      + lhs->lhs_scriptvar_idx;
			    lhs->lhs_type = sv->sv_type;
			}
		    }
		}
		else if (check_defined(var_start, lhs->lhs_varlen, cctx, FALSE)
								       == FAIL)
		    return FAIL;
	    }
	}

	if (declare_error)
	{
	    vim9_declare_error(lhs->lhs_name);
	    return FAIL;
	}
    }

    // handle "a:name" as a name, not index "name" on "a"
    if (lhs->lhs_varlen > 1 || var_start[lhs->lhs_varlen] != ':')
	var_end = lhs->lhs_dest_end;

    if (lhs->lhs_dest != dest_option)
    {
	if (is_decl && *var_end == ':')
	{
	    char_u *p;

	    // parse optional type: "let var: type = expr"
	    if (!VIM_ISWHITE(var_end[1]))
	    {
		semsg(_(e_white_space_required_after_str_str), ":", var_end);
		return FAIL;
	    }
	    p = skipwhite(var_end + 1);
	    lhs->lhs_type = parse_type(&p, cctx->ctx_type_list, TRUE);
	    if (lhs->lhs_type == NULL)
		return FAIL;
	    lhs->lhs_has_type = TRUE;
	}
	else if (lhs->lhs_lvar != NULL)
	    lhs->lhs_type = lhs->lhs_lvar->lv_type;
    }

    if (oplen == 3 && !heredoc
		   && lhs->lhs_dest != dest_global
		   && !lhs->lhs_has_index
		   && lhs->lhs_type->tt_type != VAR_STRING
		   && lhs->lhs_type->tt_type != VAR_ANY)
    {
	emsg(_(e_can_only_concatenate_to_string));
	return FAIL;
    }

    if (lhs->lhs_lvar == NULL && lhs->lhs_dest == dest_local
						 && cctx->ctx_skip != SKIP_YES)
    {
	if (oplen > 1 && !heredoc)
	{
	    // +=, /=, etc. require an existing variable
	    semsg(_(e_cannot_use_operator_on_new_variable), lhs->lhs_name);
	    return FAIL;
	}
	if (!is_decl)
	{
	    semsg(_(e_unknown_variable_str), lhs->lhs_name);
	    return FAIL;
	}

	// Check the name is valid for a funcref.
	if ((lhs->lhs_type->tt_type == VAR_FUNC
				      || lhs->lhs_type->tt_type == VAR_PARTIAL)
		&& var_wrong_func_name(lhs->lhs_name, TRUE))
	    return FAIL;

	// New local variable.
	lhs->lhs_lvar = reserve_local(cctx, var_start, lhs->lhs_varlen,
		    cmdidx == CMD_final || cmdidx == CMD_const, lhs->lhs_type);
	if (lhs->lhs_lvar == NULL)
	    return FAIL;
	lhs->lhs_new_local = TRUE;
    }

    lhs->lhs_member_type = lhs->lhs_type;
    if (lhs->lhs_has_index)
    {
	// Something follows after the variable: "var[idx]" or "var.key".
	// TODO: should we also handle "->func()" here?
	if (is_decl)
	{
	    emsg(_(e_cannot_use_index_when_declaring_variable));
	    return FAIL;
	}

	if (var_start[lhs->lhs_varlen] == '['
					  || var_start[lhs->lhs_varlen] == '.')
	{
	    char_u	*after = var_start + lhs->lhs_varlen;
	    char_u	*p;

	    // Only the last index is used below, if there are others
	    // before it generate code for the expression.  Thus for
	    // "ll[1][2]" the expression is "ll[1]" and "[2]" is the index.
	    for (;;)
	    {
		p = skip_index(after);
		if (*p != '[' && *p != '.')
		{
		    lhs->lhs_varlen_total = p - var_start;
		    break;
		}
		after = p;
	    }
	    if (after > var_start + lhs->lhs_varlen)
	    {
		lhs->lhs_varlen = after - var_start;
		lhs->lhs_dest = dest_expr;
		// We don't know the type before evaluating the expression,
		// use "any" until then.
		lhs->lhs_type = &t_any;
	    }

	    if (lhs->lhs_type->tt_member == NULL)
		lhs->lhs_member_type = &t_any;
	    else
		lhs->lhs_member_type = lhs->lhs_type->tt_member;
	}
	else
	{
	    semsg("Not supported yet: %s", var_start);
	    return FAIL;
	}
    }
    return OK;
}

/*
 * Figure out the LHS and check a few errors.
 */
    static int
compile_assign_lhs(
	char_u	*var_start,
	lhs_T	*lhs,
	int	cmdidx,
	int	is_decl,
	int	heredoc,
	int	oplen,
	cctx_T	*cctx)
{
    if (compile_lhs(var_start, lhs, cmdidx, heredoc, oplen, cctx) == FAIL)
	return FAIL;

    if (!lhs->lhs_has_index && lhs->lhs_lvar == &lhs->lhs_arg_lvar)
    {
	semsg(_(e_cannot_assign_to_argument), lhs->lhs_name);
	return FAIL;
    }
    if (!is_decl && lhs->lhs_lvar != NULL
			   && lhs->lhs_lvar->lv_const && !lhs->lhs_has_index)
    {
	semsg(_(e_cannot_assign_to_constant), lhs->lhs_name);
	return FAIL;
    }
    return OK;
}

/*
 * For an assignment with an index, compile the "idx" in "var[idx]" or "key" in
 * "var.key".
 */
    static int
compile_assign_index(
	char_u	*var_start,
	lhs_T	*lhs,
	int	*range,
	cctx_T	*cctx)
{
    size_t	varlen = lhs->lhs_varlen;
    char_u	*p;
    int		r = OK;
    int		need_white_before = TRUE;
    int		empty_second;

    p = var_start + varlen;
    if (*p == '[')
    {
	p = skipwhite(p + 1);
	if (*p == ':')
	{
	    // empty first index, push zero
	    r = generate_PUSHNR(cctx, 0);
	    need_white_before = FALSE;
	}
	else
	    r = compile_expr0(&p, cctx);

	if (r == OK && *skipwhite(p) == ':')
	{
	    // unlet var[idx : idx]
	    // blob[idx : idx] = value
	    *range = TRUE;
	    p = skipwhite(p);
	    empty_second = *skipwhite(p + 1) == ']';
	    if ((need_white_before && !IS_WHITE_OR_NUL(p[-1]))
		    || (!empty_second && !IS_WHITE_OR_NUL(p[1])))
	    {
		semsg(_(e_white_space_required_before_and_after_str_at_str),
								      ":", p);
		return FAIL;
	    }
	    p = skipwhite(p + 1);
	    if (*p == ']')
		// empty second index, push "none"
		r = generate_PUSHSPEC(cctx, VVAL_NONE);
	    else
		r = compile_expr0(&p, cctx);
	}

	if (r == OK && *skipwhite(p) != ']')
	{
	    // this should not happen
	    emsg(_(e_missbrac));
	    r = FAIL;
	}
    }
    else // if (*p == '.')
    {
	char_u *key_end = to_name_end(p + 1, TRUE);
	char_u *key = vim_strnsave(p + 1, key_end - p - 1);

	r = generate_PUSHS(cctx, key);
    }
    return r;
}

/*
 * For a LHS with an index, load the variable to be indexed.
 */
    static int
compile_load_lhs(
	lhs_T	*lhs,
	char_u	*var_start,
	type_T	*rhs_type,
	cctx_T	*cctx)
{
    if (lhs->lhs_dest == dest_expr)
    {
	size_t	    varlen = lhs->lhs_varlen;
	int	    c = var_start[varlen];
	char_u	    *p = var_start;
	garray_T    *stack = &cctx->ctx_type_stack;

	// Evaluate "ll[expr]" of "ll[expr][idx]"
	var_start[varlen] = NUL;
	if (compile_expr0(&p, cctx) == OK && p != var_start + varlen)
	{
	    // this should not happen
	    emsg(_(e_missbrac));
	    var_start[varlen] = c;
	    return FAIL;
	}
	var_start[varlen] = c;

	lhs->lhs_type = stack->ga_len == 0 ? &t_void
			      : ((type_T **)stack->ga_data)[stack->ga_len - 1];
	// now we can properly check the type
	if (rhs_type != NULL && lhs->lhs_type->tt_member != NULL
		&& rhs_type != &t_void
		&& need_type(rhs_type, lhs->lhs_type->tt_member, -2, 0, cctx,
							 FALSE, FALSE) == FAIL)
	    return FAIL;
    }
    else
	generate_loadvar(cctx, lhs->lhs_dest, lhs->lhs_name,
						 lhs->lhs_lvar, lhs->lhs_type);
    return OK;
}

/*
 * Produce code for loading "lhs" and also take care of an index.
 * Return OK/FAIL.
 */
    static int
compile_load_lhs_with_index(lhs_T *lhs, char_u *var_start, cctx_T *cctx)
{
    compile_load_lhs(lhs, var_start, NULL, cctx);

    if (lhs->lhs_has_index)
    {
	int range = FALSE;

	// Get member from list or dict.  First compile the
	// index value.
	if (compile_assign_index(var_start, lhs, &range, cctx) == FAIL)
	    return FAIL;
	if (range)
	{
	    semsg(_(e_cannot_use_range_with_assignment_operator_str),
								    var_start);
	    return FAIL;
	}

	// Get the member.
	if (compile_member(FALSE, cctx) == FAIL)
	    return FAIL;
    }
    return OK;
}

/*
 * Assignment to a list or dict member, or ":unlet" for the item, using the
 * information in "lhs".
 * Returns OK or FAIL.
 */
    static int
compile_assign_unlet(
	char_u	*var_start,
	lhs_T	*lhs,
	int	is_assign,
	type_T	*rhs_type,
	cctx_T	*cctx)
{
    vartype_T	dest_type;
    garray_T    *stack = &cctx->ctx_type_stack;
    int		range = FALSE;

    if (compile_assign_index(var_start, lhs, &range, cctx) == FAIL)
	return FAIL;
    if (is_assign && range && lhs->lhs_type != &t_blob
						    && lhs->lhs_type != &t_any)
    {
	semsg(_(e_cannot_use_range_with_assignment_str), var_start);
	return FAIL;
    }

    if (lhs->lhs_type == &t_any)
    {
	// Index on variable of unknown type: check at runtime.
	dest_type = VAR_ANY;
    }
    else
    {
	dest_type = lhs->lhs_type->tt_type;
	if (dest_type == VAR_DICT && range)
	{
	    emsg(e_cannot_use_range_with_dictionary);
	    return FAIL;
	}
	if (dest_type == VAR_DICT
			      && may_generate_2STRING(-1, FALSE, cctx) == FAIL)
	    return FAIL;
	if (dest_type == VAR_LIST || dest_type == VAR_BLOB)
	{
	    type_T *type;

	    if (range)
	    {
		type = ((type_T **)stack->ga_data)[stack->ga_len - 2];
		if (need_type(type, &t_number,
					    -1, 0, cctx, FALSE, FALSE) == FAIL)
		return FAIL;
	    }
	    type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    if ((dest_type != VAR_BLOB && type != &t_special)
		    && need_type(type, &t_number,
					    -1, 0, cctx, FALSE, FALSE) == FAIL)
		return FAIL;
	}
    }

    // Load the dict or list.  On the stack we then have:
    // - value (for assignment, not for :unlet)
    // - index
    // - for [a : b] second index
    // - variable
    if (compile_load_lhs(lhs, var_start, rhs_type, cctx) == FAIL)
	return FAIL;

    if (dest_type == VAR_LIST || dest_type == VAR_DICT
			      || dest_type == VAR_BLOB || dest_type == VAR_ANY)
    {
	if (is_assign)
	{
	    if (range)
	    {
		if (generate_instr_drop(cctx, ISN_STORERANGE, 4) == NULL)
		    return FAIL;
	    }
	    else
	    {
		isn_T	*isn = generate_instr_drop(cctx, ISN_STOREINDEX, 3);

		if (isn == NULL)
		    return FAIL;
		isn->isn_arg.vartype = dest_type;
	    }
	}
	else if (range)
	{
	    if (generate_instr_drop(cctx, ISN_UNLETRANGE, 3) == NULL)
		return FAIL;
	}
	else
	{
	    if (generate_instr_drop(cctx, ISN_UNLETINDEX, 2) == NULL)
		return FAIL;
	}
    }
    else
    {
	emsg(_(e_indexable_type_required));
	return FAIL;
    }

    return OK;
}

/*
 * Compile declaration and assignment:
 * "let name"
 * "var name = expr"
 * "final name = expr"
 * "const name = expr"
 * "name = expr"
 * "arg" points to "name".
 * "++arg" and "--arg"
 * Return NULL for an error.
 * Return "arg" if it does not look like a variable list.
 */
    static char_u *
compile_assignment(char_u *arg, exarg_T *eap, cmdidx_T cmdidx, cctx_T *cctx)
{
    char_u	*var_start;
    char_u	*p;
    char_u	*end = arg;
    char_u	*ret = NULL;
    int		var_count = 0;
    int		var_idx;
    int		semicolon = 0;
    int		did_generate_slice = FALSE;
    garray_T	*instr = &cctx->ctx_instr;
    garray_T    *stack = &cctx->ctx_type_stack;
    char_u	*op;
    int		oplen = 0;
    int		heredoc = FALSE;
    int		incdec = FALSE;
    type_T	*rhs_type = &t_any;
    char_u	*sp;
    int		is_decl = is_decl_command(cmdidx);
    lhs_T	lhs;
    long	start_lnum = SOURCING_LNUM;

    // Skip over the "var" or "[var, var]" to get to any "=".
    p = skip_var_list(arg, TRUE, &var_count, &semicolon, TRUE);
    if (p == NULL)
	return *arg == '[' ? arg : NULL;

    if (var_count > 0 && is_decl)
    {
	// TODO: should we allow this, and figure out type inference from list
	// members?
	emsg(_(e_cannot_use_list_for_declaration));
	return NULL;
    }
    lhs.lhs_name = NULL;

    sp = p;
    p = skipwhite(p);
    op = p;
    oplen = assignment_len(p, &heredoc);

    if (var_count > 0 && oplen == 0)
	// can be something like "[1, 2]->func()"
	return arg;

    if (oplen > 0 && (!VIM_ISWHITE(*sp) || !IS_WHITE_OR_NUL(op[oplen])))
    {
	error_white_both(op, oplen);
	return NULL;
    }
    if (eap->cmdidx == CMD_increment || eap->cmdidx == CMD_decrement)
    {
	if (VIM_ISWHITE(eap->cmd[2]))
	{
	    semsg(_(e_no_white_space_allowed_after_str_str),
			 eap->cmdidx == CMD_increment ? "++" : "--", eap->cmd);
	    return NULL;
	}
	op = (char_u *)(eap->cmdidx == CMD_increment ? "+=" : "-=");
	oplen = 2;
	incdec = TRUE;
    }

    if (heredoc)
    {
	list_T	   *l;
	listitem_T *li;

	// [let] varname =<< [trim] {end}
	eap->getline = exarg_getline;
	eap->cookie = cctx;
	l = heredoc_get(eap, op + 3, FALSE);
	if (l == NULL)
	    return NULL;

	if (cctx->ctx_skip != SKIP_YES)
	{
	    // Push each line and the create the list.
	    FOR_ALL_LIST_ITEMS(l, li)
	    {
		generate_PUSHS(cctx, li->li_tv.vval.v_string);
		li->li_tv.vval.v_string = NULL;
	    }
	    generate_NEWLIST(cctx, l->lv_len);
	}
	list_free(l);
	p += STRLEN(p);
	end = p;
    }
    else if (var_count > 0)
    {
	char_u *wp;

	// for "[var, var] = expr" evaluate the expression here, loop over the
	// list of variables below.
	// A line break may follow the "=".

	wp = op + oplen;
	if (may_get_next_line_error(wp, &p, cctx) == FAIL)
	    return FAIL;
	if (compile_expr0(&p, cctx) == FAIL)
	    return NULL;
	end = p;

	if (cctx->ctx_skip != SKIP_YES)
	{
	    type_T	*stacktype;

	    stacktype = stack->ga_len == 0 ? &t_void
			      : ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    if (stacktype->tt_type == VAR_VOID)
	    {
		emsg(_(e_cannot_use_void_value));
		goto theend;
	    }
	    if (need_type(stacktype, &t_list_any, -1, 0, cctx,
							 FALSE, FALSE) == FAIL)
		goto theend;
	    // TODO: check the length of a constant list here
	    generate_CHECKLEN(cctx, semicolon ? var_count - 1 : var_count,
								    semicolon);
	    if (stacktype->tt_member != NULL)
		rhs_type = stacktype->tt_member;
	}
    }

    /*
     * Loop over variables in "[var, var] = expr".
     * For "var = expr" and "let var: type" this is done only once.
     */
    if (var_count > 0)
	var_start = skipwhite(arg + 1);  // skip over the "["
    else
	var_start = arg;
    for (var_idx = 0; var_idx == 0 || var_idx < var_count; var_idx++)
    {
	int	instr_count = -1;
	int	save_lnum;

	if (var_start[0] == '_' && !eval_isnamec(var_start[1]))
	{
	    // Ignore underscore in "[a, _, b] = list".
	    if (var_count > 0)
	    {
		var_start = skipwhite(var_start + 2);
		continue;
	    }
	    emsg(_(e_cannot_use_underscore_here));
	    goto theend;
	}
	vim_free(lhs.lhs_name);

	/*
	 * Figure out the LHS type and other properties.
	 */
	if (compile_assign_lhs(var_start, &lhs, cmdidx,
					is_decl, heredoc, oplen, cctx) == FAIL)
	    goto theend;
	if (!heredoc)
	{
	    if (cctx->ctx_skip == SKIP_YES)
	    {
		if (oplen > 0 && var_count == 0)
		{
		    // skip over the "=" and the expression
		    p = skipwhite(op + oplen);
		    (void)compile_expr0(&p, cctx);
		}
	    }
	    else if (oplen > 0)
	    {
		int	is_const = FALSE;
		char_u	*wp;

		// for "+=", "*=", "..=" etc. first load the current value
		if (*op != '='
			&& compile_load_lhs_with_index(&lhs, var_start,
								 cctx) == FAIL)
		    goto theend;

		// For "var = expr" evaluate the expression.
		if (var_count == 0)
		{
		    int	r;

		    // Compile the expression.
		    instr_count = instr->ga_len;
		    if (incdec)
		    {
			r = generate_PUSHNR(cctx, 1);
		    }
		    else
		    {
			// Temporarily hide the new local variable here, it is
			// not available to this expression.
			if (lhs.lhs_new_local)
			    --cctx->ctx_locals.ga_len;
			wp = op + oplen;
			if (may_get_next_line_error(wp, &p, cctx) == FAIL)
			{
			    if (lhs.lhs_new_local)
				++cctx->ctx_locals.ga_len;
			    goto theend;
			}
			r = compile_expr0_ext(&p, cctx, &is_const);
			if (lhs.lhs_new_local)
			    ++cctx->ctx_locals.ga_len;
			if (r == FAIL)
			    goto theend;
		    }
		}
		else if (semicolon && var_idx == var_count - 1)
		{
		    // For "[var; var] = expr" get the rest of the list
		    did_generate_slice = TRUE;
		    if (generate_SLICE(cctx, var_count - 1) == FAIL)
			goto theend;
		}
		else
		{
		    // For "[var, var] = expr" get the "var_idx" item from the
		    // list.
		    if (generate_GETITEM(cctx, var_idx, *op != '=') == FAIL)
			goto theend;
		}

		rhs_type = stack->ga_len == 0 ? &t_void
			      : ((type_T **)stack->ga_data)[stack->ga_len - 1];
		if (lhs.lhs_lvar != NULL && (is_decl || !lhs.lhs_has_type))
		{
		    if ((rhs_type->tt_type == VAR_FUNC
				|| rhs_type->tt_type == VAR_PARTIAL)
			    && !lhs.lhs_has_index
			    && var_wrong_func_name(lhs.lhs_name, TRUE))
			goto theend;

		    if (lhs.lhs_new_local && !lhs.lhs_has_type)
		    {
			if (rhs_type->tt_type == VAR_VOID)
			{
			    emsg(_(e_cannot_use_void_value));
			    goto theend;
			}
			else
			{
			    // An empty list or dict has a &t_unknown member,
			    // for a variable that implies &t_any.
			    if (rhs_type == &t_list_empty)
				lhs.lhs_lvar->lv_type = &t_list_any;
			    else if (rhs_type == &t_dict_empty)
				lhs.lhs_lvar->lv_type = &t_dict_any;
			    else if (rhs_type == &t_unknown)
				lhs.lhs_lvar->lv_type = &t_any;
			    else
				lhs.lhs_lvar->lv_type = rhs_type;
			}
		    }
		    else if (*op == '=')
		    {
			type_T *use_type = lhs.lhs_lvar->lv_type;

			// Without operator check type here, otherwise below.
			// Use the line number of the assignment.
			SOURCING_LNUM = start_lnum;
			if (lhs.lhs_has_index)
			    use_type = lhs.lhs_member_type;
			if (need_type(rhs_type, use_type, -1, 0, cctx,
						      FALSE, is_const) == FAIL)
			    goto theend;
		    }
		}
		else
		{
		    type_T *lhs_type = lhs.lhs_member_type;

		    // Special case: assigning to @# can use a number or a
		    // string.
		    if (lhs_type == &t_number_or_string
					    && rhs_type->tt_type == VAR_NUMBER)
			lhs_type = &t_number;
		    if (*p != '=' && need_type(rhs_type, lhs_type,
					    -1, 0, cctx, FALSE, FALSE) == FAIL)
		    goto theend;
		}
	    }
	    else if (cmdidx == CMD_final)
	    {
		emsg(_(e_final_requires_a_value));
		goto theend;
	    }
	    else if (cmdidx == CMD_const)
	    {
		emsg(_(e_const_requires_a_value));
		goto theend;
	    }
	    else if (!lhs.lhs_has_type || lhs.lhs_dest == dest_option)
	    {
		emsg(_(e_type_or_initialization_required));
		goto theend;
	    }
	    else
	    {
		// variables are always initialized
		if (ga_grow(instr, 1) == FAIL)
		    goto theend;
		switch (lhs.lhs_member_type->tt_type)
		{
		    case VAR_BOOL:
			generate_PUSHBOOL(cctx, VVAL_FALSE);
			break;
		    case VAR_FLOAT:
#ifdef FEAT_FLOAT
			generate_PUSHF(cctx, 0.0);
#endif
			break;
		    case VAR_STRING:
			generate_PUSHS(cctx, NULL);
			break;
		    case VAR_BLOB:
			generate_PUSHBLOB(cctx, blob_alloc());
			break;
		    case VAR_FUNC:
			generate_PUSHFUNC(cctx, NULL, &t_func_void);
			break;
		    case VAR_LIST:
			generate_NEWLIST(cctx, 0);
			break;
		    case VAR_DICT:
			generate_NEWDICT(cctx, 0);
			break;
		    case VAR_JOB:
			generate_PUSHJOB(cctx, NULL);
			break;
		    case VAR_CHANNEL:
			generate_PUSHCHANNEL(cctx, NULL);
			break;
		    case VAR_NUMBER:
		    case VAR_UNKNOWN:
		    case VAR_ANY:
		    case VAR_PARTIAL:
		    case VAR_VOID:
		    case VAR_INSTR:
		    case VAR_SPECIAL:  // cannot happen
			generate_PUSHNR(cctx, 0);
			break;
		}
	    }
	    if (var_count == 0)
		end = p;
	}

	// no need to parse more when skipping
	if (cctx->ctx_skip == SKIP_YES)
	    break;

	if (oplen > 0 && *op != '=')
	{
	    type_T	    *expected;
	    type_T	    *stacktype;

	    if (*op == '.')
		expected = &t_string;
	    else
		expected = lhs.lhs_member_type;
	    stacktype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    if (
#ifdef FEAT_FLOAT
		// If variable is float operation with number is OK.
		!(expected == &t_float && stacktype == &t_number) &&
#endif
		    need_type(stacktype, expected, -1, 0, cctx,
							 FALSE, FALSE) == FAIL)
		goto theend;

	    if (*op == '.')
	    {
		if (generate_instr_drop(cctx, ISN_CONCAT, 1) == NULL)
		    goto theend;
	    }
	    else if (*op == '+')
	    {
		if (generate_add_instr(cctx,
			    operator_type(lhs.lhs_member_type, stacktype),
				       lhs.lhs_member_type, stacktype) == FAIL)
		    goto theend;
	    }
	    else if (generate_two_op(cctx, op) == FAIL)
		goto theend;
	}

	// Use the line number of the assignment for store instruction.
	save_lnum = cctx->ctx_lnum;
	cctx->ctx_lnum = start_lnum - 1;

	if (lhs.lhs_has_index)
	{
	    // Use the info in "lhs" to store the value at the index in the
	    // list or dict.
	    if (compile_assign_unlet(var_start, &lhs, TRUE, rhs_type, cctx)
								       == FAIL)
	    {
		cctx->ctx_lnum = save_lnum;
		goto theend;
	    }
	}
	else
	{
	    if (is_decl && cmdidx == CMD_const && (lhs.lhs_dest == dest_script
						|| lhs.lhs_dest == dest_global
						|| lhs.lhs_dest == dest_local))
		// ":const var": lock the value, but not referenced variables
		generate_LOCKCONST(cctx);

	    if (is_decl
		    && (lhs.lhs_type->tt_type == VAR_DICT
					  || lhs.lhs_type->tt_type == VAR_LIST)
		    && lhs.lhs_type->tt_member != NULL
		    && lhs.lhs_type->tt_member != &t_any
		    && lhs.lhs_type->tt_member != &t_unknown)
		// Set the type in the list or dict, so that it can be checked,
		// also in legacy script.
		generate_SETTYPE(cctx, lhs.lhs_type);

	    if (generate_store_lhs(cctx, &lhs, instr_count) == FAIL)
	    {
		cctx->ctx_lnum = save_lnum;
		goto theend;
	    }
	}
	cctx->ctx_lnum = save_lnum;

	if (var_idx + 1 < var_count)
	    var_start = skipwhite(lhs.lhs_dest_end + 1);
    }

    // For "[var, var] = expr" drop the "expr" value.
    // Also for "[var, var; _] = expr".
    if (var_count > 0 && (!semicolon || !did_generate_slice))
    {
	if (generate_instr_drop(cctx, ISN_DROP, 1) == NULL)
	    goto theend;
    }

    ret = skipwhite(end);

theend:
    vim_free(lhs.lhs_name);
    return ret;
}

/*
 * Check for an assignment at "eap->cmd", compile it if found.
 * Return NOTDONE if there is none, FAIL for failure, OK if done.
 */
    static int
may_compile_assignment(exarg_T *eap, char_u **line, cctx_T *cctx)
{
    char_u  *pskip;
    char_u  *p;

    // Assuming the command starts with a variable or function name,
    // find what follows.
    // Skip over "var.member", "var[idx]" and the like.
    // Also "&opt = val", "$ENV = val" and "@r = val".
    pskip = (*eap->cmd == '&' || *eap->cmd == '$' || *eap->cmd == '@')
						 ? eap->cmd + 1 : eap->cmd;
    p = to_name_end(pskip, TRUE);
    if (p > eap->cmd && *p != NUL)
    {
	char_u *var_end;
	int	oplen;
	int	heredoc;

	if (eap->cmd[0] == '@')
	    var_end = eap->cmd + 2;
	else
	    var_end = find_name_end(pskip, NULL, NULL,
					FNE_CHECK_START | FNE_INCL_BR);
	oplen = assignment_len(skipwhite(var_end), &heredoc);
	if (oplen > 0)
	{
	    size_t len = p - eap->cmd;

	    // Recognize an assignment if we recognize the variable
	    // name:
	    // "g:var = expr"
	    // "local = expr"  where "local" is a local var.
	    // "script = expr"  where "script" is a script-local var.
	    // "import = expr"  where "import" is an imported var
	    // "&opt = expr"
	    // "$ENV = expr"
	    // "@r = expr"
	    if (*eap->cmd == '&'
		    || *eap->cmd == '$'
		    || *eap->cmd == '@'
		    || ((len) > 2 && eap->cmd[1] == ':')
		    || variable_exists(eap->cmd, len, cctx))
	    {
		*line = compile_assignment(eap->cmd, eap, CMD_SIZE, cctx);
		if (*line == NULL || *line == eap->cmd)
		    return FAIL;
		return OK;
	    }
	}
    }

    if (*eap->cmd == '[')
    {
	// [var, var] = expr
	*line = compile_assignment(eap->cmd, eap, CMD_SIZE, cctx);
	if (*line == NULL)
	    return FAIL;
	if (*line != eap->cmd)
	    return OK;
    }
    return NOTDONE;
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
	ret = compile_lhs(p, &lhs, CMD_unlet, FALSE, 0, cctx);

	// : unlet an indexed item
	if (!lhs.lhs_has_index)
	{
	    iemsg("called compile_lhs() without an index");
	    ret = FAIL;
	}
	else
	{
	    // Use the info in "lhs" to unlet the item at the index in the
	    // list or dict.
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
    int	    deep UNUSED,
    void    *coookie)
{
    cctx_T	*cctx = coookie;
    int		cc = *name_end;
    char_u	*p = lvp->ll_name;
    int		ret = OK;
    size_t	len;
    char_u	*buf;

    if (cctx->ctx_skip == SKIP_YES)
	return OK;

    // Cannot use :lockvar and :unlockvar on local variables.
    if (p[1] != ':')
    {
	char_u *end = skip_var_one(p, FALSE);

	if (lookup_local(p, end - p, NULL, cctx) == OK)
	{
	    emsg(_(e_cannot_lock_unlock_local_variable));
	    return FAIL;
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
	vim_snprintf((char *)buf, len, "%s %s",
		eap->cmdidx == CMD_lockvar ? "lockvar" : "unlockvar",
		p);
	ret = generate_EXEC(cctx, buf);

	vim_free(buf);
	*name_end = cc;
    }
    return ret;
}

/*
 * compile "unlet var", "lock var" and "unlock var"
 * "arg" points to "var".
 */
    static char_u *
compile_unletlock(char_u *arg, exarg_T *eap, cctx_T *cctx)
{
    ex_unletlock(eap, arg, 0, GLV_NO_AUTOLOAD | GLV_COMPILING,
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
    static void
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
    static char_u *
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
	semsg(_(e_trailing_arg), p);
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

    static char_u *
compile_elseif(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    garray_T	*instr = &cctx->ctx_instr;
    int		instr_count = instr->ga_len;
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
	int moved_cmdmod = FALSE;

	// Move any CMDMOD instruction to after the jump
	if (((isn_T *)instr->ga_data)[instr->ga_len - 1].isn_type == ISN_CMDMOD)
	{
	    if (ga_grow(instr, 1) == FAIL)
		return NULL;
	    ((isn_T *)instr->ga_data)[instr->ga_len] =
				  ((isn_T *)instr->ga_data)[instr->ga_len - 1];
	    --instr->ga_len;
	    moved_cmdmod = TRUE;
	}

	if (compile_jump_to_end(&scope->se_u.se_if.is_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	    return NULL;
	// previous "if" or "elseif" jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_if.is_if_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
	if (moved_cmdmod)
	    ++instr->ga_len;
    }

    // compile "expr"; if we know it evaluates to FALSE skip the block
    CLEAR_FIELD(ppconst);
    if (cctx->ctx_skip == SKIP_YES)
    {
	cctx->ctx_skip = SKIP_UNKNOWN;
#ifdef FEAT_PROFILE
	if (cctx->ctx_compile_type == CT_PROFILE)
	{
	    // the previous block was skipped, need to profile this line
	    generate_instr(cctx, ISN_PROF_START);
	    instr_count = instr->ga_len;
	}
#endif
	if (cctx->ctx_compile_type == CT_DEBUG)
	{
	    // the previous block was skipped, may want to debug this line
	    generate_instr_debug(cctx);
	    instr_count = instr->ga_len;
	}
    }
    if (compile_expr1(&p, cctx, &ppconst) == FAIL)
    {
	clear_ppconst(&ppconst);
	return NULL;
    }
    cctx->ctx_skip = save_skip;
    if (!ends_excmd2(arg, skipwhite(p)))
    {
	semsg(_(e_trailing_arg), p);
	return NULL;
    }
    if (scope->se_skip_save == SKIP_YES)
	clear_ppconst(&ppconst);
    else if (instr->ga_len == instr_count && ppconst.pp_used == 1)
    {
	int error = FALSE;
	int v;

	// The expression results in a constant.
	// TODO: how about nesting?
	v = tv_get_bool_chk(&ppconst.pp_tv[0], &error);
	if (error)
	    return NULL;
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

    static char_u *
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

    static char_u *
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
    static char_u *
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
    garray_T	*stack = &cctx->ctx_type_stack;
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
	    emsg(_(e_missing_in));
	return NULL;
    }
    wp = p + 2;
    if (may_get_next_line_error(wp, &p, cctx) == FAIL)
	return NULL;

    // Remove the already generated ISN_DEBUG, it is written below the ISN_FOR
    // instruction.
    if (cctx->ctx_compile_type == CT_DEBUG && instr->ga_len > 0
	    && ((isn_T *)instr->ga_data)[instr->ga_len - 1]
							.isn_type == ISN_DEBUG)
    {
	--instr->ga_len;
	prev_lnum = ((isn_T *)instr->ga_data)[instr->ga_len]
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

    // If we know the type of "var" and it is a not a supported type we can
    // give an error now.
    vartype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    if (vartype->tt_type != VAR_LIST && vartype->tt_type != VAR_STRING
		&& vartype->tt_type != VAR_BLOB && vartype->tt_type != VAR_ANY)
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
	    // TODO: should get the type for each lhs
	    item_type = vartype->tt_member->tt_member;
    }

    // CMDMOD_REV must come before the FOR instruction.
    generate_undo_cmdmods(cctx);

    // "for_end" is set when ":endfor" is found
    scope->se_u.se_for.fs_top_label = current_instr_idx(cctx);

    generate_FOR(cctx, loop_lvar->lv_idx);

    arg = arg_start;
    if (var_list)
    {
	generate_UNPACK(cctx, var_count, semicolon);
	arg = skipwhite(arg + 1);	// skip white after '['

	// the list item is replaced by a number of items
	if (ga_grow(stack, var_count - 1) == FAIL)
	{
	    drop_scope(cctx);
	    return NULL;
	}
	--stack->ga_len;
	for (idx = 0; idx < var_count; ++idx)
	{
	    ((type_T **)stack->ga_data)[stack->ga_len] =
				(semicolon && idx == 0) ? vartype : item_type;
	    ++stack->ga_len;
	}
    }

    for (idx = 0; idx < var_count; ++idx)
    {
	assign_dest_T	dest = dest_local;
	int		opt_flags = 0;
	int		vimvaridx = -1;
	type_T		*type = &t_any;
	type_T		*lhs_type = &t_any;
	where_T		where;

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

	// TODO: script var not supported?
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
	    if (lookup_local(arg, varlen, NULL, cctx) == OK)
	    {
		semsg(_(e_variable_already_declared), arg);
		goto failed;
	    }

	    if (STRNCMP(name, "s:", 2) == 0)
	    {
		semsg(_(e_cannot_declare_script_variable_in_function), name);
		goto failed;
	    }

	    // Reserve a variable to store "var".
	    where.wt_index = var_list ? idx + 1 : 0;
	    where.wt_variable = TRUE;
	    if (lhs_type == &t_any)
		lhs_type = item_type;
	    else if (item_type != &t_unknown
			&& (item_type == &t_any
			  ? need_type(item_type, lhs_type,
						     -1, 0, cctx, FALSE, FALSE)
			  : check_type(lhs_type, item_type, TRUE, where))
			== FAIL)
		goto failed;
	    var_lvar = reserve_local(cctx, arg, varlen, TRUE, lhs_type);
	    if (var_lvar == NULL)
		// out of memory or used as an argument
		goto failed;

	    if (semicolon && idx == var_count - 1)
		var_lvar->lv_type = vartype;
	    else
		var_lvar->lv_type = item_type;
	    generate_STORE(cctx, ISN_STORE, var_lvar->lv_idx, NULL);
	}

	if (*p == ',' || *p == ';')
	    ++p;
	arg = skipwhite(p);
	vim_free(name);
    }

    if (cctx->ctx_compile_type == CT_DEBUG)
    {
	int save_prev_lnum = cctx->ctx_prev_lnum;

	// Add ISN_DEBUG here, so that the loop variables can be inspected.
	// Use the prev_lnum from the ISN_DEBUG instruction removed above.
	cctx->ctx_prev_lnum = prev_lnum;
	generate_instr_debug(cctx);
	cctx->ctx_prev_lnum = save_prev_lnum;
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
    static char_u *
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
	emsg(_(e_for));
	return NULL;
    }
    forscope = &scope->se_u.se_for;
    cctx->ctx_scope = scope->se_outer;
    unwind_locals(cctx, scope->se_local_count);

    // At end of ":for" scope jump back to the FOR instruction.
    generate_JUMP(cctx, JUMP_ALWAYS, forscope->fs_top_label);

    // Fill in the "end" label in the FOR statement so it can jump here.
    isn = ((isn_T *)instr->ga_data) + forscope->fs_top_label;
    isn->isn_arg.forloop.for_end = instr->ga_len;

    // Fill in the "end" label any BREAK statements
    compile_fill_jump_to_end(&forscope->fs_end_label, instr->ga_len, cctx);

    // Below the ":for" scope drop the "expr" list from the stack.
    if (generate_instr_drop(cctx, ISN_DROP, 1) == NULL)
	return NULL;

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
    static char_u *
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
	semsg(_(e_trailing_arg), p);
	return NULL;
    }

    if (bool_on_stack(cctx) == FAIL)
	return FAIL;

    // CMDMOD_REV must come before the jump
    generate_undo_cmdmods(cctx);

    // "while_end" is set when ":endwhile" is found
    if (compile_jump_to_end(&scope->se_u.se_while.ws_end_label,
						  JUMP_IF_FALSE, cctx) == FAIL)
	return FAIL;

    return p;
}

/*
 * compile "endwhile"
 */
    static char_u *
compile_endwhile(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    garray_T	*instr = &cctx->ctx_instr;

    if (misplaced_cmdmod(cctx))
	return NULL;
    if (scope == NULL || scope->se_type != WHILE_SCOPE)
    {
	emsg(_(e_while));
	return NULL;
    }
    cctx->ctx_scope = scope->se_outer;
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

    vim_free(scope);

    return arg;
}

/*
 * compile "continue"
 */
    static char_u *
compile_continue(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    int		try_scopes = 0;
    int		loop_label;

    for (;;)
    {
	if (scope == NULL)
	{
	    emsg(_(e_continue));
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
    static char_u *
compile_break(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    endlabel_T	**el;

    for (;;)
    {
	if (scope == NULL)
	{
	    emsg(_(e_break));
	    return NULL;
	}
	if (scope->se_type == FOR_SCOPE || scope->se_type == WHILE_SCOPE)
	    break;
	scope = scope->se_outer;
    }

    // Jump to the end of the FOR or WHILE loop.
    if (scope->se_type == FOR_SCOPE)
	el = &scope->se_u.se_for.fs_end_label;
    else
	el = &scope->se_u.se_while.ws_end_label;
    if (compile_jump_to_end(el, JUMP_ALWAYS, cctx) == FAIL)
	return FAIL;

    return arg;
}

/*
 * compile "{" start of block
 */
    static char_u *
compile_block(char_u *arg, cctx_T *cctx)
{
    if (new_scope(cctx, BLOCK_SCOPE) == NULL)
	return NULL;
    return skipwhite(arg + 1);
}

/*
 * compile end of block: drop one scope
 */
    static void
compile_endblock(cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;

    cctx->ctx_scope = scope->se_outer;
    unwind_locals(cctx, scope->se_local_count);
    vim_free(scope);
}

/*
 * compile "try"
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
    static char_u *
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
	isn->isn_arg.try.try_ref = ALLOC_CLEAR_ONE(tryref_T);
	if (isn->isn_arg.try.try_ref == NULL)
	    return NULL;
    }

    // scope for the try block itself
    scope = new_scope(cctx, BLOCK_SCOPE);
    if (scope == NULL)
	return NULL;

    return arg;
}

/*
 * compile "catch {expr}"
 */
    static char_u *
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
	emsg(_(e_catch));
	return NULL;
    }

    if (scope->se_u.se_try.ts_caught_all)
    {
	emsg(_(e_catch_unreachable_after_catch_all));
	return NULL;
    }

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
	if (isn->isn_arg.try.try_ref->try_catch == 0)
	    isn->isn_arg.try.try_ref->try_catch = instr->ga_len;
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
	    return FAIL;
	}
	if (tofree == NULL)
	    len = (int)(end - (p + 1));
	else
	    len = (int)(end - tofree);
	pat = vim_strnsave(tofree == NULL ? p + 1 : tofree, len);
	vim_free(tofree);
	p += len + 2 + dropped;
	if (pat == NULL)
	    return FAIL;
	if (generate_PUSHS(cctx, pat) == FAIL)
	    return FAIL;

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

    static char_u *
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
	emsg(_(e_finally));
	return NULL;
    }

    // End :catch or :finally scope: set value in ISN_TRY instruction
    isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_try_label;
    if (isn->isn_arg.try.try_ref->try_finally != 0)
    {
	emsg(_(e_finally_dup));
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
    if (isn->isn_arg.try.try_ref->try_catch == 0)
	isn->isn_arg.try.try_ref->try_catch = this_instr;
    isn->isn_arg.try.try_ref->try_finally = this_instr;
    if (scope->se_u.se_try.ts_catch_label != 0)
    {
	// Previous catch without match jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_catch_label;
	isn->isn_arg.jump.jump_where = this_instr;
	scope->se_u.se_try.ts_catch_label = 0;
    }
    if (generate_instr(cctx, ISN_FINALLY) == NULL)
	return NULL;

    // TODO: set index in ts_finally_label jumps

    return arg;
}

    static char_u *
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
	    emsg(_(e_no_endtry));
	else if (scope->se_type == WHILE_SCOPE)
	    emsg(_(e_endwhile));
	else if (scope->se_type == FOR_SCOPE)
	    emsg(_(e_endfor));
	else
	    emsg(_(e_endif));
	return NULL;
    }

    try_isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_try_label;
    if (cctx->ctx_skip != SKIP_YES)
    {
	if (try_isn->isn_arg.try.try_ref->try_catch == 0
				      && try_isn->isn_arg.try.try_ref->try_finally == 0)
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

    compile_endblock(cctx);

    if (cctx->ctx_skip != SKIP_YES)
    {
	// End :catch or :finally scope: set instruction index in ISN_TRY
	// instruction
	try_isn->isn_arg.try.try_ref->try_endtry = instr->ga_len;
	if (cctx->ctx_skip != SKIP_YES
				   && generate_instr(cctx, ISN_ENDTRY) == NULL)
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
    static char_u *
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

    static char_u *
compile_eval(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    int		name_only;
    char_u	*alias;
    long	lnum = SOURCING_LNUM;

    // find_ex_command() will consider a variable name an expression, assuming
    // that something follows on the next line.  Check that something actually
    // follows, otherwise it's probably a misplaced command.
    get_name_len(&p, &alias, FALSE, FALSE);
    name_only = ends_excmd2(arg, skipwhite(p));
    vim_free(alias);

    p = arg;
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
 * compile "execute expr"
 */
    static char_u *
compile_mult_expr(char_u *arg, int cmdidx, cctx_T *cctx)
{
    char_u	*p = arg;
    char_u	*prev = arg;
    char_u	*expr_start;
    int		count = 0;
    int		start_ctx_lnum = cctx->ctx_lnum;
    garray_T	*stack = &cctx->ctx_type_stack;
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
	    type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
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
    static char_u *
compile_put(char_u *arg, exarg_T *eap, cctx_T *cctx)
{
    char_u	*line = arg;
    linenr_T	lnum;
    char	*errormsg;
    int		above = eap->forceit;

    eap->regname = *line;

    if (eap->regname == '=')
    {
	char_u *p = line + 1;

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
    static char_u *
compile_exec(char_u *line, exarg_T *eap, cctx_T *cctx)
{
    char_u	*p;
    int		has_expr = FALSE;
    char_u	*nextcmd = (char_u *)"";

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
	    separate_nextcmd(eap);
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
	{
	    eap->arg = p + 1;
	    has_expr = TRUE;
	}
    }

    if (eap->cmdidx == CMD_folddoopen || eap->cmdidx == CMD_folddoclosed)
    {
	// TODO: should only expand when appropriate for the command
	eap->arg = skiptowhite(eap->arg);
	has_expr = TRUE;
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
		generate_PUSHS(cctx, vim_strnsave(start, p - start));
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
		    generate_PUSHS(cctx, vim_strsave(start));
		    ++count;
		}
		break;
	    }
	}
	generate_EXECCONCAT(cctx, count);
    }
    else
	generate_EXEC(cctx, line);

theend:
    if (*nextcmd != NUL)
    {
	// the parser expects a pointer to the bar, put it back
	--nextcmd;
	*nextcmd = '|';
    }

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
    static char_u *
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
    static char_u *
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
				       || ga_grow(&cctx->ctx_instr, 1) == FAIL)
	    {
		if (trailing_error)
		    semsg(_(e_trailing_arg), cmd);
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

    static char_u *
compile_redir(char_u *line, exarg_T *eap, cctx_T *cctx)
{
    char_u *arg = eap->arg;
    lhs_T	*lhs = &cctx->ctx_redir_lhs;

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
		generate_instr_drop(cctx, ISN_CONCAT, 1);

	    if (lhs->lhs_has_index)
	    {
		// Use the info in "lhs" to store the value at the index in the
		// list or dict.
		if (compile_assign_unlet(lhs->lhs_whole, lhs, TRUE,
						      &t_string, cctx) == FAIL)
		    return NULL;
	    }
	    else if (generate_store_lhs(cctx, lhs, -1) == FAIL)
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
						FALSE, FALSE, 1, cctx) == FAIL)
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

#ifdef FEAT_QUICKFIX
    static char_u *
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
 * Add a function to the list of :def functions.
 * This sets "ufunc->uf_dfunc_idx" but the function isn't compiled yet.
 */
    static int
add_def_function(ufunc_T *ufunc)
{
    dfunc_T *dfunc;

    if (def_functions.ga_len == 0)
    {
	// The first position is not used, so that a zero uf_dfunc_idx means it
	// wasn't set.
	if (ga_grow(&def_functions, 1) == FAIL)
	    return FAIL;
	++def_functions.ga_len;
    }

    // Add the function to "def_functions".
    if (ga_grow(&def_functions, 1) == FAIL)
	return FAIL;
    dfunc = ((dfunc_T *)def_functions.ga_data) + def_functions.ga_len;
    CLEAR_POINTER(dfunc);
    dfunc->df_idx = def_functions.ga_len;
    ufunc->uf_dfunc_idx = dfunc->df_idx;
    dfunc->df_ufunc = ufunc;
    dfunc->df_name = vim_strsave(ufunc->uf_name);
    ga_init2(&dfunc->df_var_names, sizeof(char_u *), 10);
    ++dfunc->df_refcount;
    ++def_functions.ga_len;
    return OK;
}

/*
 * After ex_function() has collected all the function lines: parse and compile
 * the lines into instructions.
 * Adds the function to "def_functions".
 * When "check_return_type" is set then set ufunc->uf_ret_type to the type of
 * the return statement (used for lambda).  When uf_ret_type is already set
 * then check that it matches.
 * When "profiling" is true add ISN_PROF_START instructions.
 * "outer_cctx" is set for a nested function.
 * This can be used recursively through compile_lambda(), which may reallocate
 * "def_functions".
 * Returns OK or FAIL.
 */
    int
compile_def_function(
	ufunc_T		*ufunc,
	int		check_return_type,
	compiletype_T   compile_type,
	cctx_T		*outer_cctx)
{
    char_u	*line = NULL;
    char_u	*line_to_free = NULL;
    char_u	*p;
    char	*errormsg = NULL;	// error message
    cctx_T	cctx;
    garray_T	*instr;
    int		did_emsg_before = did_emsg;
    int		did_emsg_silent_before = did_emsg_silent;
    int		ret = FAIL;
    sctx_T	save_current_sctx = current_sctx;
    int		save_estack_compiling = estack_compiling;
    int		save_cmod_flags = cmdmod.cmod_flags;
    int		do_estack_push;
    int		new_def_function = FALSE;
#ifdef FEAT_PROFILE
    int		prof_lnum = -1;
#endif
    int		debug_lnum = -1;

    // When using a function that was compiled before: Free old instructions.
    // The index is reused.  Otherwise add a new entry in "def_functions".
    if (ufunc->uf_dfunc_idx > 0)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;
	isn_T	*instr_dest = NULL;

	switch (compile_type)
	{
	    case CT_PROFILE:
#ifdef FEAT_PROFILE
			    instr_dest = dfunc->df_instr_prof; break;
#endif
	    case CT_NONE:   instr_dest = dfunc->df_instr; break;
	    case CT_DEBUG:  instr_dest = dfunc->df_instr_debug; break;
	}
	if (instr_dest != NULL)
	    // Was compiled in this mode before: Free old instructions.
	    delete_def_function_contents(dfunc, FALSE);
	ga_clear_strings(&dfunc->df_var_names);
    }
    else
    {
	if (add_def_function(ufunc) == FAIL)
	    return FAIL;
	new_def_function = TRUE;
    }

    ufunc->uf_def_status = UF_COMPILING;

    CLEAR_FIELD(cctx);

    cctx.ctx_compile_type = compile_type;
    cctx.ctx_ufunc = ufunc;
    cctx.ctx_lnum = -1;
    cctx.ctx_outer = outer_cctx;
    ga_init2(&cctx.ctx_locals, sizeof(lvar_T), 10);
    ga_init2(&cctx.ctx_type_stack, sizeof(type_T *), 50);
    ga_init2(&cctx.ctx_imports, sizeof(imported_T), 10);
    cctx.ctx_type_list = &ufunc->uf_type_list;
    ga_init2(&cctx.ctx_instr, sizeof(isn_T), 50);
    instr = &cctx.ctx_instr;

    // Set the context to the function, it may be compiled when called from
    // another script.  Set the script version to the most modern one.
    // The line number will be set in next_line_from_context().
    current_sctx = ufunc->uf_script_ctx;
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;

    // Don't use the flag from ":legacy" here.
    cmdmod.cmod_flags &= ~CMOD_LEGACY;

    // Make sure error messages are OK.
    do_estack_push = !estack_top_is_ufunc(ufunc, 1);
    if (do_estack_push)
	estack_push_ufunc(ufunc, 1);
    estack_compiling = TRUE;

    if (ufunc->uf_def_args.ga_len > 0)
    {
	int	count = ufunc->uf_def_args.ga_len;
	int	first_def_arg = ufunc->uf_args.ga_len - count;
	int	i;
	char_u	*arg;
	int	off = STACK_FRAME_SIZE + (ufunc->uf_va_name != NULL ? 1 : 0);
	int	did_set_arg_type = FALSE;

	// Produce instructions for the default values of optional arguments.
	SOURCING_LNUM = 0;  // line number unknown
	for (i = 0; i < count; ++i)
	{
	    garray_T	*stack = &cctx.ctx_type_stack;
	    type_T	*val_type;
	    int		arg_idx = first_def_arg + i;
	    where_T	where;
	    int		r;
	    int		jump_instr_idx = instr->ga_len;
	    isn_T	*isn;

	    // Use a JUMP_IF_ARG_SET instruction to skip if the value was given.
	    if (generate_JUMP_IF_ARG_SET(&cctx, i - count - off) == FAIL)
		goto erret;

	    // Make sure later arguments are not found.
	    ufunc->uf_args_visible = arg_idx;

	    arg = ((char_u **)(ufunc->uf_def_args.ga_data))[i];
	    r = compile_expr0(&arg, &cctx);

	    if (r == FAIL)
		goto erret;

	    // If no type specified use the type of the default value.
	    // Otherwise check that the default value type matches the
	    // specified type.
	    val_type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    where.wt_index = arg_idx + 1;
	    where.wt_variable = FALSE;
	    if (ufunc->uf_arg_types[arg_idx] == &t_unknown)
	    {
		did_set_arg_type = TRUE;
		ufunc->uf_arg_types[arg_idx] = val_type;
	    }
	    else if (check_type(ufunc->uf_arg_types[arg_idx], val_type,
							  TRUE, where) == FAIL)
		goto erret;

	    if (generate_STORE(&cctx, ISN_STORE, i - count - off, NULL) == FAIL)
		goto erret;

	    // set instruction index in JUMP_IF_ARG_SET to here
	    isn = ((isn_T *)instr->ga_data) + jump_instr_idx;
	    isn->isn_arg.jumparg.jump_where = instr->ga_len;
	}

	if (did_set_arg_type)
	    set_function_type(ufunc);
    }
    ufunc->uf_args_visible = ufunc->uf_args.ga_len;

    /*
     * Loop over all the lines of the function and generate instructions.
     */
    for (;;)
    {
	exarg_T	    ea;
	int	    starts_with_colon = FALSE;
	char_u	    *cmd;
	cmdmod_T    local_cmdmod;

	// Bail out on the first error to avoid a flood of errors and report
	// the right line number when inside try/catch.
	if (did_emsg_before != did_emsg)
	    goto erret;

	if (line != NULL && *line == '|')
	    // the line continues after a '|'
	    ++line;
	else if (line != NULL && *skipwhite(line) != NUL
		&& !(*line == '#' && (line == cctx.ctx_line_start
						    || VIM_ISWHITE(line[-1]))))
	{
	    semsg(_(e_trailing_arg), line);
	    goto erret;
	}
	else if (line != NULL && vim9_bad_comment(skipwhite(line)))
	    goto erret;
	else
	{
	    line = next_line_from_context(&cctx, FALSE);
	    if (cctx.ctx_lnum >= ufunc->uf_lines.ga_len)
	    {
		// beyond the last line
#ifdef FEAT_PROFILE
		if (cctx.ctx_skip != SKIP_YES)
		    may_generate_prof_end(&cctx, prof_lnum);
#endif
		break;
	    }
	    // Make a copy, splitting off nextcmd and removing trailing spaces
	    // may change it.
	    if (line != NULL)
	    {
		line = vim_strsave(line);
		vim_free(line_to_free);
		line_to_free = line;
	    }
	}

	CLEAR_FIELD(ea);
	ea.cmdlinep = &line;
	ea.cmd = skipwhite(line);

	if (*ea.cmd == '#')
	{
	    // "#" starts a comment
	    line = (char_u *)"";
	    continue;
	}

#ifdef FEAT_PROFILE
	if (cctx.ctx_compile_type == CT_PROFILE && cctx.ctx_lnum != prof_lnum
						  && cctx.ctx_skip != SKIP_YES)
	{
	    may_generate_prof_end(&cctx, prof_lnum);

	    prof_lnum = cctx.ctx_lnum;
	    generate_instr(&cctx, ISN_PROF_START);
	}
#endif
	if (cctx.ctx_compile_type == CT_DEBUG && cctx.ctx_lnum != debug_lnum
						  && cctx.ctx_skip != SKIP_YES)
	{
	    debug_lnum = cctx.ctx_lnum;
	    generate_instr_debug(&cctx);
	}
	cctx.ctx_prev_lnum = cctx.ctx_lnum + 1;

	// Some things can be recognized by the first character.
	switch (*ea.cmd)
	{
	    case '}':
		{
		    // "}" ends a block scope
		    scopetype_T stype = cctx.ctx_scope == NULL
					  ? NO_SCOPE : cctx.ctx_scope->se_type;

		    if (stype == BLOCK_SCOPE)
		    {
			compile_endblock(&cctx);
			line = ea.cmd;
		    }
		    else
		    {
			emsg(_(e_using_rcurly_outside_if_block_scope));
			goto erret;
		    }
		    if (line != NULL)
			line = skipwhite(ea.cmd + 1);
		    continue;
		}

	    case '{':
		// "{" starts a block scope
		// "{'a': 1}->func() is something else
		if (ends_excmd(*skipwhite(ea.cmd + 1)))
		{
		    line = compile_block(ea.cmd, &cctx);
		    continue;
		}
		break;
	}

	/*
	 * COMMAND MODIFIERS
	 */
	cctx.ctx_has_cmdmod = FALSE;
	if (parse_command_modifiers(&ea, &errormsg, &local_cmdmod, FALSE)
								       == FAIL)
	{
	    if (errormsg != NULL)
		goto erret;
	    // empty line or comment
	    line = (char_u *)"";
	    continue;
	}
	generate_cmdmods(&cctx, &local_cmdmod);
	undo_cmdmod(&local_cmdmod);

	// Check if there was a colon after the last command modifier or before
	// the current position.
	for (p = ea.cmd; p >= line; --p)
	{
	    if (*p == ':')
		starts_with_colon = TRUE;
	    if (p < ea.cmd && !VIM_ISWHITE(*p))
		break;
	}

	// Skip ":call" to get to the function name, unless using :legacy
	p = ea.cmd;
	if (!(local_cmdmod.cmod_flags & CMOD_LEGACY))
	{
	    if (checkforcmd(&ea.cmd, "call", 3))
	    {
		if (*ea.cmd == '(')
		    // not for "call()"
		    ea.cmd = p;
		else
		    ea.cmd = skipwhite(ea.cmd);
	    }

	    if (!starts_with_colon)
	    {
		int	    assign;

		// Check for assignment after command modifiers.
		assign = may_compile_assignment(&ea, &line, &cctx);
		if (assign == OK)
		    goto nextline;
		if (assign == FAIL)
		    goto erret;
	    }
	}

	/*
	 * COMMAND after range
	 * 'text'->func() should not be confused with 'a mark
	 * "++nr" and "--nr" are eval commands
	 */
	cmd = ea.cmd;
	if (!(local_cmdmod.cmod_flags & CMOD_LEGACY)
		&& (starts_with_colon || !(*cmd == '\''
		       || (cmd[0] == cmd[1] && (*cmd == '+' || *cmd == '-')))))
	{
	    ea.cmd = skip_range(ea.cmd, TRUE, NULL);
	    if (ea.cmd > cmd)
	    {
		if (!starts_with_colon)
		{
		    semsg(_(e_colon_required_before_range_str), cmd);
		    goto erret;
		}
		ea.addr_count = 1;
		if (ends_excmd2(line, ea.cmd))
		{
		    // A range without a command: jump to the line.
		    // TODO: compile to a more efficient command, possibly
		    // calling parse_cmd_address().
		    ea.cmdidx = CMD_SIZE;
		    line = compile_exec(line, &ea, &cctx);
		    goto nextline;
		}
	    }
	}
	p = find_ex_command(&ea, NULL,
		starts_with_colon || (local_cmdmod.cmod_flags & CMOD_LEGACY)
						  ? NULL : item_exists, &cctx);

	if (p == NULL)
	{
	    if (cctx.ctx_skip != SKIP_YES)
		emsg(_(e_ambiguous_use_of_user_defined_command));
	    goto erret;
	}

	// When using ":legacy cmd" always use compile_exec().
	if (local_cmdmod.cmod_flags & CMOD_LEGACY)
	{
	    char_u *start = ea.cmd;

	    switch (ea.cmdidx)
	    {
		case CMD_if:
		case CMD_elseif:
		case CMD_else:
		case CMD_endif:
		case CMD_for:
		case CMD_endfor:
		case CMD_continue:
		case CMD_break:
		case CMD_while:
		case CMD_endwhile:
		case CMD_try:
		case CMD_catch:
		case CMD_finally:
		case CMD_endtry:
			semsg(_(e_cannot_use_legacy_with_command_str), ea.cmd);
			goto erret;
		default: break;
	    }

	    // ":legacy return expr" needs to be handled differently.
	    if (checkforcmd(&start, "return", 4))
		ea.cmdidx = CMD_return;
	    else
		ea.cmdidx = CMD_legacy;
	}

	if (p == ea.cmd && ea.cmdidx != CMD_SIZE)
	{
	    if (cctx.ctx_skip == SKIP_YES)
	    {
		line += STRLEN(line);
		goto nextline;
	    }

	    // Expression or function call.
	    if (ea.cmdidx != CMD_eval)
	    {
		// CMD_var cannot happen, compile_assignment() above would be
		// used.  Most likely an assignment to a non-existing variable.
		semsg(_(e_command_not_recognized_str), ea.cmd);
		goto erret;
	    }
	}

	if (cctx.ctx_had_return
		&& ea.cmdidx != CMD_elseif
		&& ea.cmdidx != CMD_else
		&& ea.cmdidx != CMD_endif
		&& ea.cmdidx != CMD_endfor
		&& ea.cmdidx != CMD_endwhile
		&& ea.cmdidx != CMD_catch
		&& ea.cmdidx != CMD_finally
		&& ea.cmdidx != CMD_endtry)
	{
	    emsg(_(e_unreachable_code_after_return));
	    goto erret;
	}

	p = skipwhite(p);
	if (ea.cmdidx != CMD_SIZE
			    && ea.cmdidx != CMD_write && ea.cmdidx != CMD_read)
	{
	    if (ea.cmdidx >= 0)
		ea.argt = excmd_get_argt(ea.cmdidx);
	    if ((ea.argt & EX_BANG) && *p == '!')
	    {
		ea.forceit = TRUE;
		p = skipwhite(p + 1);
	    }
	}

	switch (ea.cmdidx)
	{
	    case CMD_def:
		    ea.arg = p;
		    line = compile_nested_function(&ea, &cctx);
		    break;

	    case CMD_function:
		    // TODO: should we allow this, e.g. to declare a global
		    // function?
		    emsg(_(e_cannot_use_function_inside_def));
		    goto erret;

	    case CMD_return:
		    line = compile_return(p, check_return_type,
				 local_cmdmod.cmod_flags & CMOD_LEGACY, &cctx);
		    cctx.ctx_had_return = TRUE;
		    break;

	    case CMD_let:
		    emsg(_(e_cannot_use_let_in_vim9_script));
		    break;
	    case CMD_var:
	    case CMD_final:
	    case CMD_const:
	    case CMD_increment:
	    case CMD_decrement:
		    line = compile_assignment(p, &ea, ea.cmdidx, &cctx);
		    if (line == p)
			line = NULL;
		    break;

	    case CMD_unlet:
	    case CMD_unlockvar:
	    case CMD_lockvar:
		    line = compile_unletlock(p, &ea, &cctx);
		    break;

	    case CMD_import:
		    emsg(_(e_import_can_only_be_used_in_script));
		    line = NULL;
		    break;

	    case CMD_if:
		    line = compile_if(p, &cctx);
		    break;
	    case CMD_elseif:
		    line = compile_elseif(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_else:
		    line = compile_else(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_endif:
		    line = compile_endif(p, &cctx);
		    break;

	    case CMD_while:
		    line = compile_while(p, &cctx);
		    break;
	    case CMD_endwhile:
		    line = compile_endwhile(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;

	    case CMD_for:
		    line = compile_for(p, &cctx);
		    break;
	    case CMD_endfor:
		    line = compile_endfor(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_continue:
		    line = compile_continue(p, &cctx);
		    break;
	    case CMD_break:
		    line = compile_break(p, &cctx);
		    break;

	    case CMD_try:
		    line = compile_try(p, &cctx);
		    break;
	    case CMD_catch:
		    line = compile_catch(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_finally:
		    line = compile_finally(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_endtry:
		    line = compile_endtry(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_throw:
		    line = compile_throw(p, &cctx);
		    break;

	    case CMD_eval:
		    line = compile_eval(p, &cctx);
		    break;

	    case CMD_echo:
	    case CMD_echon:
	    case CMD_execute:
	    case CMD_echomsg:
	    case CMD_echoerr:
		    line = compile_mult_expr(p, ea.cmdidx, &cctx);
		    break;

	    case CMD_put:
		    ea.cmd = cmd;
		    line = compile_put(p, &ea, &cctx);
		    break;

	    case CMD_substitute:
		    if (cctx.ctx_skip == SKIP_YES)
			line = (char_u *)"";
		    else
		    {
			ea.arg = p;
			line = compile_substitute(line, &ea, &cctx);
		    }
		    break;

	    case CMD_redir:
		    ea.arg = p;
		    line = compile_redir(line, &ea, &cctx);
		    break;

	    case CMD_cexpr:
	    case CMD_lexpr:
	    case CMD_caddexpr:
	    case CMD_laddexpr:
	    case CMD_cgetexpr:
	    case CMD_lgetexpr:
#ifdef FEAT_QUICKFIX
		    ea.arg = p;
		    line = compile_cexpr(line, &ea, &cctx);
#else
		    ex_ni(&ea);
		    line = NULL;
#endif
		    break;

	    // TODO: any other commands with an expression argument?

	    case CMD_append:
	    case CMD_change:
	    case CMD_insert:
	    case CMD_k:
	    case CMD_t:
	    case CMD_xit:
		    not_in_vim9(&ea);
		    goto erret;

	    case CMD_SIZE:
		    if (cctx.ctx_skip != SKIP_YES)
		    {
			semsg(_(e_invalid_command_str), ea.cmd);
			goto erret;
		    }
		    // We don't check for a next command here.
		    line = (char_u *)"";
		    break;

	    case CMD_lua:
	    case CMD_mzscheme:
	    case CMD_perl:
	    case CMD_py3:
	    case CMD_python3:
	    case CMD_python:
	    case CMD_pythonx:
	    case CMD_ruby:
	    case CMD_tcl:
		    ea.arg = p;
		    if (vim_strchr(line, '\n') == NULL)
			line = compile_exec(line, &ea, &cctx);
		    else
			// heredoc lines have been concatenated with NL
			// characters in get_function_body()
			line = compile_script(line, &cctx);
		    break;

	    default:
		    // Not recognized, execute with do_cmdline_cmd().
		    ea.arg = p;
		    line = compile_exec(line, &ea, &cctx);
		    break;
	}
nextline:
	if (line == NULL)
	    goto erret;
	line = skipwhite(line);

	// Undo any command modifiers.
	generate_undo_cmdmods(&cctx);

	if (cctx.ctx_type_stack.ga_len < 0)
	{
	    iemsg("Type stack underflow");
	    goto erret;
	}
    }

    if (cctx.ctx_scope != NULL)
    {
	if (cctx.ctx_scope->se_type == IF_SCOPE)
	    emsg(_(e_endif));
	else if (cctx.ctx_scope->se_type == WHILE_SCOPE)
	    emsg(_(e_endwhile));
	else if (cctx.ctx_scope->se_type == FOR_SCOPE)
	    emsg(_(e_endfor));
	else
	    emsg(_(e_missing_rcurly));
	goto erret;
    }

    if (!cctx.ctx_had_return)
    {
	if (ufunc->uf_ret_type->tt_type == VAR_UNKNOWN)
	    ufunc->uf_ret_type = &t_void;
	else if (ufunc->uf_ret_type->tt_type != VAR_VOID)
	{
	    emsg(_(e_missing_return_statement));
	    goto erret;
	}

	// Return void if there is no return at the end.
	generate_instr(&cctx, ISN_RETURN_VOID);
    }

    // When compiled with ":silent!" and there was an error don't consider the
    // function compiled.
    if (emsg_silent == 0 || did_emsg_silent == did_emsg_silent_before)
    {
	dfunc_T	*dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;
	dfunc->df_deleted = FALSE;
	dfunc->df_script_seq = current_sctx.sc_seq;
#ifdef FEAT_PROFILE
	if (cctx.ctx_compile_type == CT_PROFILE)
	{
	    dfunc->df_instr_prof = instr->ga_data;
	    dfunc->df_instr_prof_count = instr->ga_len;
	}
	else
#endif
	if (cctx.ctx_compile_type == CT_DEBUG)
	{
	    dfunc->df_instr_debug = instr->ga_data;
	    dfunc->df_instr_debug_count = instr->ga_len;
	}
	else
	{
	    dfunc->df_instr = instr->ga_data;
	    dfunc->df_instr_count = instr->ga_len;
	}
	dfunc->df_varcount = dfunc->df_var_names.ga_len;
	dfunc->df_has_closure = cctx.ctx_has_closure;
	if (cctx.ctx_outer_used)
	    ufunc->uf_flags |= FC_CLOSURE;
	ufunc->uf_def_status = UF_COMPILED;
    }

    ret = OK;

erret:
    if (ufunc->uf_def_status == UF_COMPILING)
    {
	dfunc_T	*dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;

	// Compiling aborted, free the generated instructions.
	clear_instr_ga(instr);
	VIM_CLEAR(dfunc->df_name);
	ga_clear_strings(&dfunc->df_var_names);

	// If using the last entry in the table and it was added above, we
	// might as well remove it.
	if (!dfunc->df_deleted && new_def_function
			    && ufunc->uf_dfunc_idx == def_functions.ga_len - 1)
	{
	    --def_functions.ga_len;
	    ufunc->uf_dfunc_idx = 0;
	}
	ufunc->uf_def_status = UF_COMPILE_ERROR;

	while (cctx.ctx_scope != NULL)
	    drop_scope(&cctx);

	if (errormsg != NULL)
	    emsg(errormsg);
	else if (did_emsg == did_emsg_before)
	    emsg(_(e_compiling_def_function_failed));
    }

    if (cctx.ctx_redir_lhs.lhs_name != NULL)
    {
	if (ret == OK)
	{
	    emsg(_(e_missing_redir_end));
	    ret = FAIL;
	}
	vim_free(cctx.ctx_redir_lhs.lhs_name);
	vim_free(cctx.ctx_redir_lhs.lhs_whole);
    }

    current_sctx = save_current_sctx;
    estack_compiling = save_estack_compiling;
    cmdmod.cmod_flags =	save_cmod_flags;
    if (do_estack_push)
	estack_pop();

    vim_free(line_to_free);
    free_imported(&cctx);
    free_locals(&cctx);
    ga_clear(&cctx.ctx_type_stack);
    return ret;
}

    void
set_function_type(ufunc_T *ufunc)
{
    int varargs = ufunc->uf_va_name != NULL;
    int argcount = ufunc->uf_args.ga_len;

    // Create a type for the function, with the return type and any
    // argument types.
    // A vararg is included in uf_args.ga_len but not in uf_arg_types.
    // The type is included in "tt_args".
    if (argcount > 0 || varargs)
    {
	if (ufunc->uf_type_list.ga_itemsize == 0)
	    ga_init2(&ufunc->uf_type_list, sizeof(type_T *), 10);
	ufunc->uf_func_type = alloc_func_type(ufunc->uf_ret_type,
					   argcount, &ufunc->uf_type_list);
	// Add argument types to the function type.
	if (func_type_add_arg_types(ufunc->uf_func_type,
				    argcount + varargs,
				    &ufunc->uf_type_list) == FAIL)
	    return;
	ufunc->uf_func_type->tt_argcount = argcount + varargs;
	ufunc->uf_func_type->tt_min_argcount =
				      argcount - ufunc->uf_def_args.ga_len;
	if (ufunc->uf_arg_types == NULL)
	{
	    int i;

	    // lambda does not have argument types.
	    for (i = 0; i < argcount; ++i)
		ufunc->uf_func_type->tt_args[i] = &t_any;
	}
	else
	    mch_memmove(ufunc->uf_func_type->tt_args,
			 ufunc->uf_arg_types, sizeof(type_T *) * argcount);
	if (varargs)
	{
	    ufunc->uf_func_type->tt_args[argcount] =
		   ufunc->uf_va_type == NULL ? &t_list_any : ufunc->uf_va_type;
	    ufunc->uf_func_type->tt_flags = TTFLAG_VARARGS;
	}
    }
    else
	// No arguments, can use a predefined type.
	ufunc->uf_func_type = get_func_type(ufunc->uf_ret_type,
					   argcount, &ufunc->uf_type_list);
}


/*
 * Delete an instruction, free what it contains.
 */
    void
delete_instr(isn_T *isn)
{
    switch (isn->isn_type)
    {
	case ISN_DEF:
	case ISN_EXEC:
	case ISN_EXEC_SPLIT:
	case ISN_LEGACY_EVAL:
	case ISN_LOADAUTO:
	case ISN_LOADB:
	case ISN_LOADENV:
	case ISN_LOADG:
	case ISN_LOADOPT:
	case ISN_LOADT:
	case ISN_LOADW:
	case ISN_PUSHEXC:
	case ISN_PUSHFUNC:
	case ISN_PUSHS:
	case ISN_RANGE:
	case ISN_STOREAUTO:
	case ISN_STOREB:
	case ISN_STOREENV:
	case ISN_STOREG:
	case ISN_STORET:
	case ISN_STOREW:
	case ISN_STRINGMEMBER:
	    vim_free(isn->isn_arg.string);
	    break;

	case ISN_SUBSTITUTE:
	    {
		int	idx;
		isn_T	*list = isn->isn_arg.subs.subs_instr;

		vim_free(isn->isn_arg.subs.subs_cmd);
		for (idx = 0; list[idx].isn_type != ISN_FINISH; ++idx)
		    delete_instr(list + idx);
		vim_free(list);
	    }
	    break;

	case ISN_INSTR:
	    {
		int	idx;
		isn_T	*list = isn->isn_arg.instr;

		for (idx = 0; list[idx].isn_type != ISN_FINISH; ++idx)
		    delete_instr(list + idx);
		vim_free(list);
	    }
	    break;

	case ISN_LOADS:
	case ISN_STORES:
	    vim_free(isn->isn_arg.loadstore.ls_name);
	    break;

	case ISN_UNLET:
	case ISN_UNLETENV:
	    vim_free(isn->isn_arg.unlet.ul_name);
	    break;

	case ISN_STOREOPT:
	    vim_free(isn->isn_arg.storeopt.so_name);
	    break;

	case ISN_PUSHBLOB:   // push blob isn_arg.blob
	    blob_unref(isn->isn_arg.blob);
	    break;

	case ISN_PUSHJOB:
#ifdef FEAT_JOB_CHANNEL
	    job_unref(isn->isn_arg.job);
#endif
	    break;

	case ISN_PUSHCHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    channel_unref(isn->isn_arg.channel);
#endif
	    break;

	case ISN_UCALL:
	    vim_free(isn->isn_arg.ufunc.cuf_name);
	    break;

	case ISN_FUNCREF:
	    {
		dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
					       + isn->isn_arg.funcref.fr_func;
		ufunc_T *ufunc = dfunc->df_ufunc;

		if (ufunc != NULL && func_name_refcount(ufunc->uf_name))
		    func_ptr_unref(ufunc);
	    }
	    break;

	case ISN_DCALL:
	    {
		dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
					       + isn->isn_arg.dfunc.cdf_idx;

		if (dfunc->df_ufunc != NULL
			       && func_name_refcount(dfunc->df_ufunc->uf_name))
		    func_ptr_unref(dfunc->df_ufunc);
	    }
	    break;

	case ISN_NEWFUNC:
	    {
		char_u  *lambda = isn->isn_arg.newfunc.nf_lambda;
		ufunc_T *ufunc = find_func_even_dead(lambda, TRUE, NULL);

		if (ufunc != NULL)
		{
		    unlink_def_function(ufunc);
		    func_ptr_unref(ufunc);
		}

		vim_free(lambda);
		vim_free(isn->isn_arg.newfunc.nf_global);
	    }
	    break;

	case ISN_CHECKTYPE:
	case ISN_SETTYPE:
	    free_type(isn->isn_arg.type.ct_type);
	    break;

	case ISN_CMDMOD:
	    vim_regfree(isn->isn_arg.cmdmod.cf_cmdmod
					       ->cmod_filter_regmatch.regprog);
	    vim_free(isn->isn_arg.cmdmod.cf_cmdmod);
	    break;

	case ISN_LOADSCRIPT:
	case ISN_STORESCRIPT:
	    vim_free(isn->isn_arg.script.scriptref);
	    break;

	case ISN_TRY:
	    vim_free(isn->isn_arg.try.try_ref);
	    break;

	case ISN_CEXPR_CORE:
	    vim_free(isn->isn_arg.cexpr.cexpr_ref->cer_cmdline);
	    vim_free(isn->isn_arg.cexpr.cexpr_ref);
	    break;

	case ISN_2BOOL:
	case ISN_2STRING:
	case ISN_2STRING_ANY:
	case ISN_ADDBLOB:
	case ISN_ADDLIST:
	case ISN_ANYINDEX:
	case ISN_ANYSLICE:
	case ISN_BCALL:
	case ISN_BLOBAPPEND:
	case ISN_BLOBINDEX:
	case ISN_BLOBSLICE:
	case ISN_CATCH:
	case ISN_CEXPR_AUCMD:
	case ISN_CHECKLEN:
	case ISN_CHECKNR:
	case ISN_CMDMOD_REV:
	case ISN_COMPAREANY:
	case ISN_COMPAREBLOB:
	case ISN_COMPAREBOOL:
	case ISN_COMPAREDICT:
	case ISN_COMPAREFLOAT:
	case ISN_COMPAREFUNC:
	case ISN_COMPARELIST:
	case ISN_COMPARENR:
	case ISN_COMPARESPECIAL:
	case ISN_COMPARESTRING:
	case ISN_CONCAT:
	case ISN_COND2BOOL:
	case ISN_DEBUG:
	case ISN_DROP:
	case ISN_ECHO:
	case ISN_ECHOERR:
	case ISN_ECHOMSG:
	case ISN_ENDTRY:
	case ISN_EXECCONCAT:
	case ISN_EXECUTE:
	case ISN_FINALLY:
	case ISN_FINISH:
	case ISN_FOR:
	case ISN_GETITEM:
	case ISN_JUMP:
	case ISN_JUMP_IF_ARG_SET:
	case ISN_LISTAPPEND:
	case ISN_LISTINDEX:
	case ISN_LISTSLICE:
	case ISN_LOAD:
	case ISN_LOADBDICT:
	case ISN_LOADGDICT:
	case ISN_LOADOUTER:
	case ISN_LOADREG:
	case ISN_LOADTDICT:
	case ISN_LOADV:
	case ISN_LOADWDICT:
	case ISN_LOCKCONST:
	case ISN_MEMBER:
	case ISN_NEGATENR:
	case ISN_NEWDICT:
	case ISN_NEWLIST:
	case ISN_OPANY:
	case ISN_OPFLOAT:
	case ISN_OPNR:
	case ISN_PCALL:
	case ISN_PCALL_END:
	case ISN_PROF_END:
	case ISN_PROF_START:
	case ISN_PUSHBOOL:
	case ISN_PUSHF:
	case ISN_PUSHNR:
	case ISN_PUSHSPEC:
	case ISN_PUT:
	case ISN_REDIREND:
	case ISN_REDIRSTART:
	case ISN_RETURN:
	case ISN_RETURN_VOID:
	case ISN_SHUFFLE:
	case ISN_SLICE:
	case ISN_STORE:
	case ISN_STOREINDEX:
	case ISN_STORENR:
	case ISN_STOREOUTER:
	case ISN_STORERANGE:
	case ISN_STOREREG:
	case ISN_STOREV:
	case ISN_STRINDEX:
	case ISN_STRSLICE:
	case ISN_THROW:
	case ISN_TRYCONT:
	case ISN_UNLETINDEX:
	case ISN_UNLETRANGE:
	case ISN_UNPACK:
	    // nothing allocated
	    break;
    }
}

/*
 * Free all instructions for "dfunc" except df_name.
 */
    static void
delete_def_function_contents(dfunc_T *dfunc, int mark_deleted)
{
    int idx;

    ga_clear(&dfunc->df_def_args_isn);
    ga_clear_strings(&dfunc->df_var_names);

    if (dfunc->df_instr != NULL)
    {
	for (idx = 0; idx < dfunc->df_instr_count; ++idx)
	    delete_instr(dfunc->df_instr + idx);
	VIM_CLEAR(dfunc->df_instr);
	dfunc->df_instr = NULL;
    }
    if (dfunc->df_instr_debug != NULL)
    {
	for (idx = 0; idx < dfunc->df_instr_debug_count; ++idx)
	    delete_instr(dfunc->df_instr_debug + idx);
	VIM_CLEAR(dfunc->df_instr_debug);
	dfunc->df_instr_debug = NULL;
    }
#ifdef FEAT_PROFILE
    if (dfunc->df_instr_prof != NULL)
    {
	for (idx = 0; idx < dfunc->df_instr_prof_count; ++idx)
	    delete_instr(dfunc->df_instr_prof + idx);
	VIM_CLEAR(dfunc->df_instr_prof);
	dfunc->df_instr_prof = NULL;
    }
#endif

    if (mark_deleted)
	dfunc->df_deleted = TRUE;
    if (dfunc->df_ufunc != NULL)
	dfunc->df_ufunc->uf_def_status = UF_NOT_COMPILED;
}

/*
 * When a user function is deleted, clear the contents of any associated def
 * function, unless another user function still uses it.
 * The position in def_functions can be re-used.
 */
    void
unlink_def_function(ufunc_T *ufunc)
{
    if (ufunc->uf_dfunc_idx > 0)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;

	if (--dfunc->df_refcount <= 0)
	    delete_def_function_contents(dfunc, TRUE);
	ufunc->uf_def_status = UF_NOT_COMPILED;
	ufunc->uf_dfunc_idx = 0;
	if (dfunc->df_ufunc == ufunc)
	    dfunc->df_ufunc = NULL;
    }
}

/*
 * Used when a user function refers to an existing dfunc.
 */
    void
link_def_function(ufunc_T *ufunc)
{
    if (ufunc->uf_dfunc_idx > 0)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;

	++dfunc->df_refcount;
    }
}

#if defined(EXITFREE) || defined(PROTO)
/*
 * Free all functions defined with ":def".
 */
    void
free_def_functions(void)
{
    int idx;

    for (idx = 0; idx < def_functions.ga_len; ++idx)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data) + idx;

	delete_def_function_contents(dfunc, TRUE);
	vim_free(dfunc->df_name);
    }

    ga_clear(&def_functions);
}
#endif


#endif // FEAT_EVAL
