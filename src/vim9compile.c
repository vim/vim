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
    int		lv_from_outer;	// when TRUE using ctx_outer scope
    int		lv_const;	// when TRUE cannot be assigned to
    int		lv_arg;		// when TRUE this is an argument
} lvar_T;

/*
 * Context for compiling lines of Vim script.
 * Stores info about the local variables and condition stack.
 */
struct cctx_S {
    ufunc_T	*ctx_ufunc;	    // current function
    int		ctx_lnum;	    // line number in current function
    char_u	*ctx_line_start;    // start of current line or NULL
    garray_T	ctx_instr;	    // generated instructions

    garray_T	ctx_locals;	    // currently visible local variables
    int		ctx_locals_count;   // total number of local variables

    int		ctx_closure_count;  // number of closures created in the
				    // function

    garray_T	ctx_imports;	    // imported items

    skip_T	ctx_skip;
    scope_T	*ctx_scope;	    // current scope, NULL at toplevel
    int		ctx_had_return;	    // last seen statement was "return"

    cctx_T	*ctx_outer;	    // outer scope for lambda or nested
				    // function
    int		ctx_outer_used;	    // var in ctx_outer was used

    garray_T	ctx_type_stack;	    // type of each item on the stack
    garray_T	*ctx_type_list;	    // list of pointers to allocated types
};

static void delete_def_function_contents(dfunc_T *dfunc);

/*
 * Lookup variable "name" in the local scope and return it.
 * Return NULL if not found.
 */
    static lvar_T *
lookup_local(char_u *name, size_t len, cctx_T *cctx)
{
    int	    idx;
    lvar_T  *lvar;

    if (len == 0)
	return NULL;

    // Find local in current function scope.
    for (idx = 0; idx < cctx->ctx_locals.ga_len; ++idx)
    {
	lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;
	if (STRNCMP(name, lvar->lv_name, len) == 0
					       && STRLEN(lvar->lv_name) == len)
	{
	    lvar->lv_from_outer = FALSE;
	    return lvar;
	}
    }

    // Find local in outer function scope.
    if (cctx->ctx_outer != NULL)
    {
	lvar = lookup_local(name, len, cctx->ctx_outer);
	if (lvar != NULL)
	{
	    // TODO: are there situations we should not mark the outer scope as
	    // used?
	    cctx->ctx_outer_used = TRUE;
	    lvar->lv_from_outer = TRUE;
	    return lvar;
	}
    }

    return NULL;
}

/*
 * Lookup an argument in the current function and an enclosing function.
 * Returns the argument index in "idxp"
 * Returns the argument type in "type"
 * Sets "gen_load_outer" to TRUE if found in outer scope.
 * Returns OK when found, FAIL otherwise.
 */
    static int
lookup_arg(
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
    for (idx = 0; idx < cctx->ctx_ufunc->uf_args.ga_len; ++idx)
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
	if (lookup_arg(name, len, idxp, type, gen_load_outer, cctx->ctx_outer)
									 == OK)
	{
	    *gen_load_outer = TRUE;
	    return OK;
	}
    }

    return FAIL;
}

/*
 * Returnd TRUE if the script context is Vim9 script.
 */
    static int
script_is_vim9()
{
    return SCRIPT_ITEM(current_sctx.sc_sid)->sn_version == SCRIPT_VERSION_VIM9;
}

/*
 * Lookup a variable in the current script.
 * If "vim9script" is TRUE the script must be Vim9 script.  Used for "var"
 * without "s:".
 * Returns OK or FAIL.
 */
    static int
lookup_script(char_u *name, size_t len, int vim9script)
{
    int		    cc;
    hashtab_T	    *ht = &SCRIPT_VARS(current_sctx.sc_sid);
    dictitem_T	    *di;

    if (vim9script && !script_is_vim9())
	return FAIL;
    cc = name[len];
    name[len] = NUL;
    di = find_var_in_ht(ht, 0, name, TRUE);
    name[len] = cc;
    return di == NULL ? FAIL: OK;
}

/*
 * Check if "p[len]" is already defined, either in script "import_sid" or in
 * compilation context "cctx".
 * Does not check the global namespace.
 * Return FAIL and give an error if it defined.
 */
    int
check_defined(char_u *p, size_t len, cctx_T *cctx)
{
    int		c = p[len];
    ufunc_T	*ufunc = NULL;

    p[len] = NUL;
    if (lookup_script(p, len, FALSE) == OK
	    || (cctx != NULL
		&& (lookup_local(p, len, cctx) != NULL
		    || lookup_arg(p, len, NULL, NULL, NULL, cctx) == OK))
	    || find_imported(p, len, cctx) != NULL
	    || (ufunc = find_func_even_dead(p, FALSE, cctx)) != NULL)
    {
	// A local or script-local function can shadow a global function.
	if (ufunc == NULL || !func_is_global(ufunc)
		|| (p[0] == 'g' && p[1] == ':'))
	{
	    p[len] = c;
	    semsg(_(e_name_already_defined_str), p);
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
 * If type at "offset" isn't already VAR_STRING then generate ISN_2STRING.
 * But only for simple types.
 */
    static int
may_generate_2STRING(int offset, cctx_T *cctx)
{
    isn_T	*isn;
    isntype_T	isntype = ISN_2STRING;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	**type = ((type_T **)stack->ga_data) + stack->ga_len + offset;

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

	// conversion not possible
	case VAR_VOID:
	case VAR_BLOB:
	case VAR_FUNC:
	case VAR_PARTIAL:
	case VAR_LIST:
	case VAR_DICT:
	case VAR_JOB:
	case VAR_CHANNEL:
			 to_string_error((*type)->tt_type);
			 return FAIL;
    }

    *type = &t_string;
    if ((isn = generate_instr(cctx, isntype)) == NULL)
	return FAIL;
    isn->isn_arg.number = offset;

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
    isn_T *isn = generate_instr_drop(cctx,
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
get_compare_isn(exptype_T exptype, vartype_T type1, vartype_T type2)
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
	      && (type2 == VAR_NUMBER || type2 ==VAR_FLOAT)))
	isntype = ISN_COMPAREANY;

    if ((exptype == EXPR_IS || exptype == EXPR_ISNOT)
	    && (isntype == ISN_COMPAREBOOL
	    || isntype == ISN_COMPARESPECIAL
	    || isntype == ISN_COMPARENR
	    || isntype == ISN_COMPAREFLOAT))
    {
	semsg(_(e_cannot_use_str_with_str),
		exptype == EXPR_IS ? "is" : "isnot" , vartype_name(type1));
	return ISN_DROP;
    }
    if (isntype == ISN_DROP
	    || ((exptype != EXPR_EQUAL && exptype != EXPR_NEQUAL
		    && (type1 == VAR_BOOL || type1 == VAR_SPECIAL
		       || type2 == VAR_BOOL || type2 == VAR_SPECIAL)))
	    || ((exptype != EXPR_EQUAL && exptype != EXPR_NEQUAL
				 && exptype != EXPR_IS && exptype != EXPR_ISNOT
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
check_compare_types(exptype_T type, typval_T *tv1, typval_T *tv2)
{
    if (get_compare_isn(type, tv1->v_type, tv2->v_type) == ISN_DROP)
	return FAIL;
    return OK;
}

/*
 * Generate an ISN_COMPARE* instruction with a boolean result.
 */
    static int
generate_COMPARE(cctx_T *cctx, exptype_T exptype, int ic)
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
    isntype = get_compare_isn(exptype, type1, type2);
    if (isntype == ISN_DROP)
	return FAIL;

    if ((isn = generate_instr(cctx, isntype)) == NULL)
	return FAIL;
    isn->isn_arg.op.op_type = exptype;
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
 */
    static int
generate_2BOOL(cctx_T *cctx, int invert)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_2BOOL)) == NULL)
	return FAIL;
    isn->isn_arg.number = invert;

    // type becomes bool
    ((type_T **)stack->ga_data)[stack->ga_len - 1] = &t_bool;

    return OK;
}

    static int
generate_TYPECHECK(
	cctx_T	    *cctx,
	type_T	    *expected,
	int	    offset)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    RETURN_OK_IF_SKIP(cctx);
    if ((isn = generate_instr(cctx, ISN_CHECKTYPE)) == NULL)
	return FAIL;
    isn->isn_arg.type.ct_type = alloc_type(expected);
    isn->isn_arg.type.ct_off = offset;

    // type becomes expected
    ((type_T **)stack->ga_data)[stack->ga_len + offset] = expected;

    return OK;
}

/*
 * Check that
 * - "actual" matches "expected" type or
 * - "actual" is a type that can be "expected" type: add a runtime check; or
 * - return FAIL.
 */
    static int
need_type(
	type_T	*actual,
	type_T	*expected,
	int	offset,
	cctx_T	*cctx,
	int	silent)
{
    if (expected == &t_bool && actual != &t_bool
					&& (actual->tt_flags & TTFLAG_BOOL_OK))
    {
	// Using "0", "1" or the result of an expression with "&&" or "||" as a
	// boolean is OK but requires a conversion.
	generate_2BOOL(cctx, FALSE);
	return OK;
    }

    if (check_type(expected, actual, FALSE, 0) == OK)
	return OK;

    // If the actual type can be the expected type add a runtime check.
    // TODO: if it's a constant a runtime check makes no sense.
    if (actual->tt_type == VAR_ANY
	    || actual->tt_type == VAR_UNKNOWN
	    || (actual->tt_type == VAR_FUNC
		&& (expected->tt_type == VAR_FUNC
					   || expected->tt_type == VAR_PARTIAL)
		&& (actual->tt_member == &t_any || actual->tt_argcount < 0))
	    || (actual->tt_type == VAR_LIST
		&& expected->tt_type == VAR_LIST
		&& actual->tt_member == &t_any)
	    || (actual->tt_type == VAR_DICT
		&& expected->tt_type == VAR_DICT
		&& actual->tt_member == &t_any))
    {
	generate_TYPECHECK(cctx, expected, offset);
	return OK;
    }

    if (!silent)
	type_mismatch(expected, actual);
    return FAIL;
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
    {
	type_T	*type = get_type_ptr(cctx->ctx_type_list);

	// A 0 or 1 number can also be used as a bool.
	if (type != NULL)
	{
	    type->tt_type = VAR_NUMBER;
	    type->tt_flags = TTFLAG_BOOL_OK;
	    ((type_T **)stack->ga_data)[stack->ga_len - 1] = type;
	}
    }
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

    RETURN_OK_IF_SKIP(cctx);
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
 */
    static int
generate_GETITEM(cctx_T *cctx, int index)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
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
    isn->isn_arg.number = index;

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

    RETURN_OK_IF_SKIP(cctx);
    if (isn_type == ISN_LOADSCRIPT)
	isn = generate_instr_type(cctx, isn_type, type);
    else
	isn = generate_instr_drop(cctx, isn_type, 1);
    if (isn == NULL)
	return FAIL;
    isn->isn_arg.script.script_sid = sid;
    isn->isn_arg.script.script_idx = idx;
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
    isn->isn_arg.funcref.fr_var_idx = cctx->ctx_closure_count++;

    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] =
	       ufunc->uf_func_type == NULL ? &t_func_any : ufunc->uf_func_type;
    ++stack->ga_len;

    return OK;
}

/*
 * Generate an ISN_NEWFUNC instruction.
 */
    static int
generate_NEWFUNC(cctx_T *cctx, char_u *lambda_name, char_u *func_name)
{
    isn_T	*isn;
    char_u	*name;

    RETURN_OK_IF_SKIP(cctx);
    name = vim_strsave(lambda_name);
    if (name == NULL)
	return FAIL;
    if ((isn = generate_instr(cctx, ISN_NEWFUNC)) == NULL)
	return FAIL;
    isn->isn_arg.newfunc.nf_lambda = name;
    isn->isn_arg.newfunc.nf_global = func_name;

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
    type_T	*argtypes[MAX_FUNC_ARGS];
    int		i;

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

    if ((isn = generate_instr(cctx, ISN_BCALL)) == NULL)
	return FAIL;
    isn->isn_arg.bfunc.cbf_idx = func_idx;
    isn->isn_arg.bfunc.cbf_argcount = argcount;

    for (i = 0; i < argcount; ++i)
	argtypes[i] = ((type_T **)stack->ga_data)[stack->ga_len - argcount + i];

    stack->ga_len -= argcount; // drop the arguments
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] =
			  internal_func_ret_type(func_idx, argcount, argtypes);
    ++stack->ga_len;	    // add return value

    return OK;
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

    if (ufunc->uf_def_status != UF_NOT_COMPILED)
    {
	int		i;

	for (i = 0; i < argcount; ++i)
	{
	    type_T *expected;
	    type_T *actual;

	    if (i < regular_args)
	    {
		if (ufunc->uf_arg_types == NULL)
		    continue;
		expected = ufunc->uf_arg_types[i];
	    }
	    else if (ufunc->uf_va_type == NULL || ufunc->uf_va_type == &t_any)
		// possibly a lambda or "...: any"
		expected = &t_any;
	    else
		expected = ufunc->uf_va_type->tt_member;
	    actual = ((type_T **)stack->ga_data)[stack->ga_len - argcount + i];
	    if (need_type(actual, expected, -argcount + i, cctx, TRUE) == FAIL)
	    {
		arg_type_mismatch(expected, actual, i + 1);
		return FAIL;
	    }
	}
	if (ufunc->uf_def_status == UF_TO_BE_COMPILED)
	    if (compile_def_function(ufunc, ufunc->uf_ret_type == NULL, NULL)
								       == FAIL)
		return FAIL;
    }

    if ((isn = generate_instr(cctx,
		    ufunc->uf_def_status != UF_NOT_COMPILED ? ISN_DCALL
							 : ISN_UCALL)) == NULL)
	return FAIL;
    if (ufunc->uf_def_status != UF_NOT_COMPILED)
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
		semsg(_(e_toofewarg), "[reference]");
		return FAIL;
	    }
	    if (!varargs && argcount > type->tt_argcount)
	    {
		semsg(_(e_toomanyarg), "[reference]");
		return FAIL;
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
	((type_T **)stack->ga_data)[stack->ga_len - 1] = type->tt_member;

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
generate_EXECCONCAT(cctx_T *cctx, int count)
{
    isn_T	*isn;

    if ((isn = generate_instr_drop(cctx, ISN_EXECCONCAT, count)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;
    return OK;
}

/*
 * Reserve space for a local variable.
 * Return the variable or NULL if it failed.
 */
    static lvar_T *
reserve_local(cctx_T *cctx, char_u *name, size_t len, int isConst, type_T *type)
{
    lvar_T  *lvar;

    if (lookup_arg(name, len, NULL, NULL, NULL, cctx) == OK)
    {
	emsg_namelen(_(e_str_is_used_as_argument), name, (int)len);
	return NULL;
    }

    if (ga_grow(&cctx->ctx_locals, 1) == FAIL)
	return NULL;
    lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + cctx->ctx_locals.ga_len++;

    // Every local variable uses the next entry on the stack.  We could re-use
    // the last ones when leaving a scope, but then variables used in a closure
    // might get overwritten.  To keep things simple do not re-use stack
    // entries.  This is less efficient, but memory is cheap these days.
    lvar->lv_idx = cctx->ctx_locals_count++;

    lvar->lv_name = vim_strnsave(name, len == 0 ? STRLEN(name) : len);
    lvar->lv_const = isConst;
    lvar->lv_type = type;

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
 * Find "name" in script-local items of script "sid".
 * Returns the index in "sn_var_vals" if found.
 * If found but not in "sn_var_vals" returns -1.
 * If not found returns -2.
 */
    int
get_script_item_idx(int sid, char_u *name, int check_writable)
{
    hashtab_T	    *ht;
    dictitem_T	    *di;
    scriptitem_T    *si = SCRIPT_ITEM(sid);
    int		    idx;

    // First look the name up in the hashtable.
    if (!SCRIPT_ID_VALID(sid))
	return -1;
    ht = &SCRIPT_VARS(sid);
    di = find_var_in_ht(ht, 0, name, TRUE);
    if (di == NULL)
	return -2;

    // Now find the svar_T index in sn_var_vals.
    for (idx = 0; idx < si->sn_var_vals.ga_len; ++idx)
    {
	svar_T    *sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;

	if (sv->sv_tv == &di->di_tv)
	{
	    if (check_writable && sv->sv_const)
		semsg(_(e_readonlyvar), name);
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
 * Return TRUE if "p" points at a "#" but not at "#{".
 */
    int
vim9_comment_start(char_u *p)
{
    return p[0] == '#' && p[1] != '{';
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
 * If "*arg" is at the end of the line, advance to the next line.
 * Also when "whitep" points to white space and "*arg" is on a "#".
 * Return FAIL if beyond the last line, "*arg" is unmodified then.
 */
    static int
may_get_next_line(char_u *whitep, char_u **arg, cctx_T *cctx)
{
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
} ppconst_T;

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
    idx = get_script_item_idx(current_sctx.sc_sid, name, FALSE);
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
	if (import->imp_all)
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

	    idx = find_exported(import->imp_sid, exp_name, &ufunc, &type);
	    *p = cc;
	    p = skipwhite(p);

	    // TODO: what if it is a function?
	    if (idx < 0)
		return FAIL;
	    *end = p;

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
    if (ufunc->uf_def_status == UF_TO_BE_COMPILED)
	if (compile_def_function(ufunc, TRUE, NULL) == FAIL)
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
	// load namespaced variable
	if (end <= *arg + 2)
	{
	    isntype_T  isn_type;

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

	    name = vim_strnsave(*arg + 2, end - (*arg + 2));
	    if (name == NULL)
		return FAIL;

	    switch (**arg)
	    {
		case 'v': res = generate_LOADV(cctx, name, error);
			  break;
		case 's': res = compile_load_scriptvar(cctx, name,
							    NULL, NULL, error);
			  break;
		case 'g': isn_type = ISN_LOADG; break;
		case 'w': isn_type = ISN_LOADW; break;
		case 't': isn_type = ISN_LOADT; break;
		case 'b': isn_type = ISN_LOADB; break;
		default:  semsg(_(e_namespace_not_supported_str), *arg);
			  goto theend;
	    }
	    if (isn_type != ISN_DROP)
	    {
		// Global, Buffer-local, Window-local and Tabpage-local
		// variables can be defined later, thus we don't check if it
		// exists, give error at runtime.
		res = generate_LOAD(cctx, isn_type, 0, name, &t_any);
	    }
	}
    }
    else
    {
	size_t	    len = end - *arg;
	int	    idx;
	int	    gen_load = FALSE;
	int	    gen_load_outer = FALSE;

	name = vim_strnsave(*arg, end - *arg);
	if (name == NULL)
	    return FAIL;

	if (lookup_arg(*arg, len, &idx, &type, &gen_load_outer, cctx) == OK)
	{
	    if (!gen_load_outer)
		gen_load = TRUE;
	}
	else
	{
	    lvar_T *lvar = lookup_local(*arg, len, cctx);

	    if (lvar != NULL)
	    {
		type = lvar->lv_type;
		idx = lvar->lv_idx;
		if (lvar->lv_from_outer)
		    gen_load_outer = TRUE;
		else
		    gen_load = TRUE;
	    }
	    else
	    {
		// "var" can be script-local even without using "s:" if it
		// already exists in a Vim9 script or when it's imported.
		if (lookup_script(*arg, len, TRUE) == OK
			|| find_imported(name, 0, cctx) != NULL)
		   res = compile_load_scriptvar(cctx, name, *arg, &end, FALSE);

		// When evaluating an expression and the name starts with an
		// uppercase letter or "x:" it can be a user defined function.
		// TODO: this is just guessing
		if (res == FAIL && is_expr
				   && (ASCII_ISUPPER(*name) || name[1] == ':'))
		    res = generate_funcref(cctx, name);
	    }
	}
	if (gen_load)
	    res = generate_LOAD(cctx, ISN_LOAD, idx, NULL, type);
	if (gen_load_outer)
	{
	    res = generate_LOAD(cctx, ISN_LOADOUTER, idx, NULL, type);
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
 * Compile the argument expressions.
 * "arg" points to just after the "(" and is advanced to after the ")"
 */
    static int
compile_arguments(char_u **arg, cctx_T *cctx, int *argcount)
{
    char_u  *p = *arg;
    char_u  *whitep = *arg;

    for (;;)
    {
	if (may_get_next_line(whitep, &p, cctx) == FAIL)
	    goto failret;
	if (*p == ')')
	{
	    *arg = p + 1;
	    return OK;
	}

	if (compile_expr0(&p, cctx) == FAIL)
	    return FAIL;
	++*argcount;

	if (*p != ',' && *skipwhite(p) == ',')
	{
	    semsg(_(e_no_white_space_allowed_before_str), ",");
	    p = skipwhite(p);
	}
	if (*p == ',')
	{
	    ++p;
	    if (*p != NUL && !VIM_ISWHITE(*p))
		semsg(_(e_white_space_required_after_str), ",");
	}
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
    ufunc_T	*ufunc;
    int		res = FAIL;
    int		is_autoload;

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
	if (*s == ')' && argvars[0].v_type == VAR_STRING)
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

    *arg = skipwhite(*arg + varlen + 1);
    if (compile_arguments(arg, cctx, &argcount) == FAIL)
	goto theend;

    is_autoload = vim_strchr(name, '#') != NULL;
    if (ASCII_ISLOWER(*name) && name[1] != ':' && !is_autoload)
    {
	int	    idx;

	// builtin function
	idx = find_internal_func(name);
	if (idx >= 0)
	    res = generate_BCALL(cctx, idx, argcount, argcount_init == 1);
	else
	    semsg(_(e_unknownfunc), namebuf);
	goto theend;
    }

    // If we can find the function by name generate the right call.
    // Skip global functions here, a local funcref takes precedence.
    ufunc = find_func(name, FALSE, cctx);
    if (ufunc != NULL && !func_is_global(ufunc))
    {
	res = generate_CALL(cctx, ufunc, argcount);
	goto theend;
    }

    // If the name is a variable, load it and use PCALL.
    // Not for g:Func(), we don't know if it is a variable or not.
    // Not for eome#Func(), it will be loaded later.
    p = namebuf;
    if (STRNCMP(namebuf, "g:", 2) != 0 && !is_autoload
	    && compile_load(&p, namebuf + varlen, cctx, FALSE, FALSE) == OK)
    {
	garray_T    *stack = &cctx->ctx_type_stack;
	type_T	    *type;

	type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
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
 * When "namespace" is TRUE recognize "b:", "s:", etc.
 * Return a pointer to just after the name.  Equal to "arg" if there is no
 * valid name.
 */
    static char_u *
to_name_end(char_u *arg, int namespace)
{
    char_u	*p;

    // Quick check for valid starting character.
    if (!eval_isnamec1(*arg))
	return arg;

    for (p = arg + 1; *p != NUL && eval_isnamec(*p); MB_PTR_ADV(p))
	// Include a namespace such as "s:var" and "v:var".  But "n:" is not
	// and can be used in slice "[n:]".
	if (*p == ':' && (p != arg + 1
			     || !namespace
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
    else if (p == arg && *arg == '#' && arg[1] == '{')
    {
	// Can be "#{a: 1}->Func()".
	++p;
	if (eval_dict(&p, &rettv, NULL, TRUE) == FAIL)
	    p = arg;
    }

    return p;
}

/*
 * parse a list: [expr, expr]
 * "*arg" points to the '['.
 */
    static int
compile_list(char_u **arg, cctx_T *cctx)
{
    char_u	*p = skipwhite(*arg + 1);
    char_u	*whitep = *arg + 1;
    int		count = 0;

    for (;;)
    {
	if (may_get_next_line(whitep, &p, cctx) == FAIL)
	{
	    semsg(_(e_list_end), *arg);
	    return FAIL;
	}
	if (*p == ',')
	{
	    semsg(_(e_no_white_space_allowed_before_str), ",");
	    return FAIL;
	}
	if (*p == ']')
	{
	    ++p;
	    break;
	}
	if (compile_expr0(&p, cctx) == FAIL)
	    break;
	++count;
	if (*p == ',')
	{
	    ++p;
	    if (*p != ']' && !IS_WHITE_OR_NUL(*p))
	    {
		semsg(_(e_white_space_required_after_str), ",");
		return FAIL;
	    }
	}
	whitep = p;
	p = skipwhite(p);
    }
    *arg = p;

    generate_NEWLIST(cctx, count);
    return OK;
}

/*
 * parse a lambda: {arg, arg -> expr}
 * "*arg" points to the '{'.
 */
    static int
compile_lambda(char_u **arg, cctx_T *cctx)
{
    typval_T	rettv;
    ufunc_T	*ufunc;
    evalarg_T	evalarg;

    CLEAR_FIELD(evalarg);
    evalarg.eval_flags = EVAL_EVALUATE;
    evalarg.eval_cctx = cctx;

    // Get the funcref in "rettv".
    if (get_lambda_tv(arg, &rettv, &evalarg) != OK)
	return FAIL;

    ufunc = rettv.vval.v_partial->pt_func;
    ++ufunc->uf_refcount;
    clear_tv(&rettv);
    ga_init2(&ufunc->uf_type_list, sizeof(type_T *), 10);

    // The function will have one line: "return {expr}".
    // Compile it into instructions.
    compile_def_function(ufunc, TRUE, cctx);

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
 * Compile a lamda call: expr->{lambda}(args)
 * "arg" points to the "{".
 */
    static int
compile_lambda_call(char_u **arg, cctx_T *cctx)
{
    ufunc_T	*ufunc;
    typval_T	rettv;
    int		argcount = 1;
    int		ret = FAIL;

    // Get the funcref in "rettv".
    if (get_lambda_tv(arg, &rettv, &EVALARG_EVALUATE) == FAIL)
	return FAIL;

    if (**arg != '(')
    {
	if (*skipwhite(*arg) == '(')
	    emsg(_(e_nowhitespace));
	else
	    semsg(_(e_missing_paren), "lambda");
	clear_tv(&rettv);
	return FAIL;
    }

    ufunc = rettv.vval.v_partial->pt_func;
    ++ufunc->uf_refcount;
    clear_tv(&rettv);
    ga_init2(&ufunc->uf_type_list, sizeof(type_T *), 10);

    // The function will have one line: "return {expr}".
    // Compile it into instructions.
    compile_def_function(ufunc, TRUE, cctx);

    // compile the arguments
    *arg = skipwhite(*arg + 1);
    if (compile_arguments(arg, cctx, &argcount) == OK)
	// call the compiled function
	ret = generate_CALL(cctx, ufunc, argcount);

    if (ret == FAIL)
	func_ptr_unref(ufunc);
    return ret;
}

/*
 * parse a dict: {'key': val} or #{key: val}
 * "*arg" points to the '{'.
 */
    static int
compile_dict(char_u **arg, cctx_T *cctx, int literal)
{
    garray_T	*instr = &cctx->ctx_instr;
    garray_T	*stack = &cctx->ctx_type_stack;
    int		count = 0;
    dict_T	*d = dict_alloc();
    dictitem_T	*item;
    char_u	*whitep = *arg;
    char_u	*p;

    if (d == NULL)
	return FAIL;
    *arg = skipwhite(*arg + 1);
    for (;;)
    {
	char_u *key = NULL;

	if (may_get_next_line(whitep, arg, cctx) == FAIL)
	{
	    *arg = NULL;
	    goto failret;
	}

	if (**arg == '}')
	    break;

	if (literal)
	{
	    char_u *end = to_name_end(*arg, !literal);

	    if (end == *arg)
	    {
		semsg(_(e_invalid_key_str), *arg);
		return FAIL;
	    }
	    key = vim_strnsave(*arg, end - *arg);
	    if (generate_PUSHS(cctx, key) == FAIL)
		return FAIL;
	    *arg = end;
	}
	else
	{
	    isn_T		*isn;

	    if (compile_expr0(arg, cctx) == FAIL)
		return FAIL;
	    isn = ((isn_T *)instr->ga_data) + instr->ga_len - 1;
	    if (isn->isn_type == ISN_PUSHS)
		key = isn->isn_arg.string;
	    else
	    {
		type_T *keytype = ((type_T **)stack->ga_data)
							   [stack->ga_len - 1];
		if (need_type(keytype, &t_string, -1, cctx, FALSE) == FAIL)
		    return FAIL;
	    }
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
		semsg(_(e_no_white_space_allowed_before_str), ":");
	    else
		semsg(_(e_missing_dict_colon), *arg);
	    return FAIL;
	}
	whitep = *arg + 1;
	if (!IS_WHITE_OR_NUL(*whitep))
	{
	    semsg(_(e_white_space_required_after_str), ":");
	    return FAIL;
	}

	*arg = skipwhite(*arg + 1);
	if (may_get_next_line(whitep, arg, cctx) == FAIL)
	{
	    *arg = NULL;
	    goto failret;
	}

	if (compile_expr0(arg, cctx) == FAIL)
	    return FAIL;
	++count;

	whitep = *arg;
	*arg = skipwhite(*arg);
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
	    semsg(_(e_no_white_space_allowed_before_str), ",");
	    return FAIL;
	}
	whitep = *arg + 1;
	*arg = skipwhite(*arg + 1);
    }

    *arg = *arg + 1;

    // Allow for following comment, after at least one space.
    p = skipwhite(*arg);
    if (VIM_ISWHITE(**arg) && vim9_comment_start(p))
	*arg += STRLEN(*arg);

    dict_unref(d);
    return generate_NEWDICT(cctx, count);

failret:
    if (*arg == NULL)
	semsg(_(e_missing_dict_end), _("[end of lines]"));
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
	char_u *name = vim_strnsave(start, *arg - start);
	type_T	*type = rettv.v_type == VAR_NUMBER ? &t_number : &t_string;

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
	else
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

    exptype_T
get_compare_type(char_u *p, int *len, int *type_is)
{
    exptype_T	type = EXPR_UNKNOWN;
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
	    int  invert = TRUE;

	    while (p > start && p[-1] == '!')
	    {
		--p;
		invert = !invert;
	    }
	    if (generate_2BOOL(cctx, invert) == FAIL)
		return FAIL;
	}
    }
    *end = p;
    return OK;
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
				 && (next[2] == '{' || ASCII_ISALPHA(next[2])))
		    || (next[0] == '.' && eval_isdictc(next[1]))))
	    {
		next = next_line_from_context(cctx, TRUE);
		if (next == NULL)
		    return FAIL;
		*arg = next;
		p = skipwhite(*arg);
	    }
	}

	// Do not skip over white space to find the "(", "exeucte 'x' ()" is
	// not a function call.
	if (**arg == '(')
	{
	    garray_T    *stack = &cctx->ctx_type_stack;
	    type_T	*type;
	    int		argcount = 0;

	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;

	    // funcref(arg)
	    type = ((type_T **)stack->ga_data)[stack->ga_len - 1];

	    *arg = skipwhite(p + 1);
	    if (compile_arguments(arg, cctx, &argcount) == FAIL)
		return FAIL;
	    if (generate_PCALL(cctx, argcount, name_start, type, TRUE) == FAIL)
		return FAIL;
	}
	else if (*p == '-' && p[1] == '>')
	{
	    char_u *pstart = p;

	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;

	    // something->method()
	    // Apply the '!', '-' and '+' first:
	    //   -1.0->func() works like (-1.0)->func()
	    if (compile_leader(cctx, TRUE, start_leader, end_leader) == FAIL)
		return FAIL;

	    p += 2;
	    *arg = skipwhite(p);
	    // No line break supported right after "->".
	    if (**arg == '{')
	    {
		// lambda call:  list->{lambda}
		if (compile_lambda_call(arg, cctx) == FAIL)
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
		// TODO: base value may not be the first argument
		if (compile_call(arg, p - *arg, cctx, ppconst, 1) == FAIL)
		    return FAIL;
	    }
	}
	else if (**arg == '[')
	{
	    garray_T	*stack = &cctx->ctx_type_stack;
	    type_T	**typep;
	    type_T	*valtype;
	    vartype_T	vtype;
	    int		is_slice = FALSE;

	    // list index: list[123]
	    // dict member: dict[key]
	    // string index: text[123]
	    // TODO: blob index
	    // TODO: more arguments
	    // TODO: recognize list or dict at runtime
	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;

	    ++p;
	    *arg = skipwhite(p);
	    if (may_get_next_line_error(p, arg, cctx) == FAIL)
		return FAIL;
	    if (**arg == ':')
		// missing first index is equal to zero
		generate_PUSHNR(cctx, 0);
	    else
	    {
		if (compile_expr0(arg, cctx) == FAIL)
		    return FAIL;
		if (may_get_next_line_error(p, arg, cctx) == FAIL)
		    return FAIL;
		*arg = skipwhite(*arg);
	    }
	    if (**arg == ':')
	    {
		*arg = skipwhite(*arg + 1);
		if (may_get_next_line_error(p, arg, cctx) == FAIL)
		    return FAIL;
		if (**arg == ']')
		    // missing second index is equal to end of string
		    generate_PUSHNR(cctx, -1);
		else
		{
		    if (compile_expr0(arg, cctx) == FAIL)
		    return FAIL;
		    if (may_get_next_line_error(p, arg, cctx) == FAIL)
			return FAIL;
		    *arg = skipwhite(*arg);
		}
		is_slice = TRUE;
	    }

	    if (**arg != ']')
	    {
		emsg(_(e_missbrac));
		return FAIL;
	    }
	    *arg = *arg + 1;

	    // We can index a list and a dict.  If we don't know the type
	    // we can use the index value type.
	    // TODO: If we don't know use an instruction to figure it out at
	    // runtime.
	    typep = ((type_T **)stack->ga_data) + stack->ga_len
							  - (is_slice ? 3 : 2);
	    vtype = (*typep)->tt_type;
	    valtype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    // If the index is a string, the variable must be a Dict.
	    if (*typep == &t_any && valtype == &t_string)
		vtype = VAR_DICT;
	    if (vtype == VAR_STRING || vtype == VAR_LIST || vtype == VAR_BLOB)
	    {
		if (need_type(valtype, &t_number, -1, cctx, FALSE) == FAIL)
		    return FAIL;
		if (is_slice)
		{
		    valtype = ((type_T **)stack->ga_data)[stack->ga_len - 2];
		    if (need_type(valtype, &t_number, -2, cctx, FALSE) == FAIL)
			return FAIL;
		}
	    }

	    if (vtype == VAR_DICT)
	    {
		if (is_slice)
		{
		    emsg(_(e_cannot_slice_dictionary));
		    return FAIL;
		}
		if ((*typep)->tt_type == VAR_DICT)
		    *typep = (*typep)->tt_member;
		else
		{
		    if (need_type(*typep, &t_dict_any, -2, cctx, FALSE) == FAIL)
			return FAIL;
		    *typep = &t_any;
		}
		if (may_generate_2STRING(-1, cctx) == FAIL)
		    return FAIL;
		if (generate_instr_drop(cctx, ISN_MEMBER, 1) == FAIL)
		    return FAIL;
	    }
	    else if (vtype == VAR_STRING)
	    {
		*typep = &t_string;
		if ((is_slice
			? generate_instr_drop(cctx, ISN_STRSLICE, 2)
			: generate_instr_drop(cctx, ISN_STRINDEX, 1)) == FAIL)
		    return FAIL;
	    }
	    else if (vtype == VAR_BLOB)
	    {
		emsg("Sorry, blob index and slice not implemented yet");
		return FAIL;
	    }
	    else if (vtype == VAR_LIST || *typep == &t_any)
	    {
		if (is_slice)
		{
		    if (generate_instr_drop(cctx,
			     vtype == VAR_LIST ?  ISN_LISTSLICE : ISN_ANYSLICE,
								    2) == FAIL)
			return FAIL;
		}
		else
		{
		    if ((*typep)->tt_type == VAR_LIST)
			*typep = (*typep)->tt_member;
		    if (generate_instr_drop(cctx,
			     vtype == VAR_LIST ?  ISN_LISTINDEX : ISN_ANYINDEX,
								    1) == FAIL)
			return FAIL;
		}
	    }
	    else
	    {
		emsg(_(e_string_list_dict_or_blob_required));
		return FAIL;
	    }
	}
	else if (*p == '.' && p[1] != '.')
	{
	    if (generate_ppconst(cctx, ppconst) == FAIL)
		return FAIL;

	    *arg = p + 1;
	    if (may_get_next_line(*arg, arg, cctx) == FAIL)
		return FAIL;
	    // dictionary member: dict.name
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
 * If the value is a constant "ppconst->pp_ret" will be set.
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
 *  {key: val, key: val}   Dictionary
 *  #{key: val, key: val}  Dictionary with literal keys
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

    /*
     * Skip '!', '-' and '+' characters.  They are handled later.
     */
    start_leader = *arg;
    while (**arg == '!' || **arg == '-' || **arg == '+')
	*arg = skipwhite(*arg + 1);
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
	 * List: [expr, expr]
	 */
	case '[':   ret = compile_list(arg, cctx);
		    break;

	/*
	 * Dictionary: #{key: val, key: val}
	 */
	case '#':   if ((*arg)[1] == '{')
		    {
			++*arg;
			ret = compile_dict(arg, cctx, TRUE);
		    }
		    else
			ret = NOTDONE;
		    break;

	/*
	 * Lambda: {arg, arg -> expr}
	 * Dictionary: {'key': val, 'key': val}
	 */
	case '{':   {
			char_u *start = skipwhite(*arg + 1);

			// Find out what comes after the arguments.
			ret = get_function_args(&start, '-', NULL,
					   NULL, NULL, NULL, TRUE, NULL, NULL);
			if (ret != FAIL && *start == '>')
			    ret = compile_lambda(arg, cctx);
			else
			    ret = compile_dict(arg, cctx, FALSE);
		    }
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
	 */
	case '(':   *arg = skipwhite(*arg + 1);

		    // recursive!
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
		    *arg = skipwhite(*arg);
		    if (**arg == ')')
			++*arg;
		    else if (ret == OK)
		    {
			emsg(_(e_missing_close));
			ret = FAIL;
		    }
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
	    semsg(_(e_name_expected), *arg);
	    return FAIL;
	}

	// "name" or "name()"
	p = to_name_end(*arg, TRUE);
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
    semsg(_(e_white_space_required_before_and_after_str), buf);
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
	int		called_emsg_before = called_emsg;

	++*arg;
	want_type = parse_type(arg, cctx->ctx_type_list);
	if (called_emsg != called_emsg_before)
	    return FAIL;

	if (**arg != '>')
	{
	    if (*skipwhite(*arg) == '>')
		semsg(_(e_no_white_space_allowed_before_str), ">");
	    else
		emsg(_(e_missing_gt));
	    return FAIL;
	}
	++*arg;
	if (may_get_next_line_error(*arg - 1, arg, cctx) == FAIL)
	    return FAIL;
    }

    if (compile_expr7(arg, cctx, ppconst) == FAIL)
	return FAIL;

    if (want_type != NULL)
    {
	garray_T    *stack = &cctx->ctx_type_stack;
	type_T	    *actual;

	generate_ppconst(cctx, ppconst);
	actual = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	if (check_type(want_type, actual, FALSE, 0) == FAIL)
	{
	    if (need_type(actual, want_type, -1, cctx, FALSE) == FAIL)
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
	*arg = skipwhite(op + 1);
	if (may_get_next_line(op + 1, arg, cctx) == FAIL)
	    return FAIL;

	// get the second expression
	if (compile_expr7t(arg, cctx, ppconst) == FAIL)
	    return FAIL;

	if (ppconst->pp_used == ppconst_used + 2
		&& ppconst->pp_tv[ppconst_used].v_type == VAR_NUMBER
		&& ppconst->pp_tv[ppconst_used + 1].v_type == VAR_NUMBER)
	{
	    typval_T *tv1 = &ppconst->pp_tv[ppconst_used];
	    typval_T *tv2 = &ppconst->pp_tv[ppconst_used + 1];
	    varnumber_T res = 0;

	    // both are numbers: compute the result
	    switch (*op)
	    {
		case '*': res = tv1->vval.v_number * tv2->vval.v_number;
			  break;
		case '/': res = tv1->vval.v_number / tv2->vval.v_number;
			  break;
		case '%': res = tv1->vval.v_number % tv2->vval.v_number;
			  break;
	    }
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
 *      +	number addition
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

	*arg = skipwhite(op + oplen);
	if (may_get_next_line(op + oplen, arg, cctx) == FAIL)
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
	    if (*op == '.')
	    {
		if (may_generate_2STRING(-2, cctx) == FAIL
			|| may_generate_2STRING(-1, cctx) == FAIL)
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
    exptype_T	type = EXPR_UNKNOWN;
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
	    semsg(_(e_invexpr2), *arg);
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
	*arg = skipwhite(p + len);
	if (may_get_next_line(p + len, arg, cctx) == FAIL)
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
	garray_T	*stack = &cctx->ctx_type_stack;
	type_T		**typep;

	/*
	 * Repeat until there is no following "||" or "&&"
	 */
	ga_init2(&end_ga, sizeof(int), 10);
	while (p[0] == opchar && p[1] == opchar)
	{
	    if (next != NULL)
	    {
		*arg = next_line_from_context(cctx, TRUE);
		p = skipwhite(*arg);
	    }

	    if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(p[2]))
	    {
		semsg(_(e_white_space_required_before_and_after_str), op);
		return FAIL;
	    }

	    // TODO: use ppconst if the value is a constant
	    generate_ppconst(cctx, ppconst);

	    if (ga_grow(&end_ga, 1) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }
	    *(((int *)end_ga.ga_data) + end_ga.ga_len) = instr->ga_len;
	    ++end_ga.ga_len;
	    generate_JUMP(cctx, opchar == '|'
			 ?  JUMP_AND_KEEP_IF_TRUE : JUMP_AND_KEEP_IF_FALSE, 0);

	    // eval the next expression
	    *arg = skipwhite(p + 2);
	    if (may_get_next_line(p + 2, arg, cctx) == FAIL)
		return FAIL;

	    if ((opchar == '|' ? compile_expr3(arg, cctx, ppconst)
				  : compile_expr4(arg, cctx, ppconst)) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }

	    p = may_peek_next_line(cctx, *arg, &next);
	}
	generate_ppconst(cctx, ppconst);

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

	// The resulting type can be used as a bool.
	typep = ((type_T **)stack->ga_data) + stack->ga_len - 1;
	if (*typep != &t_bool)
	{
	    type_T *type = get_type_ptr(cctx->ctx_type_list);

	    if (type != NULL)
	    {
		*type = **typep;
		type->tt_flags |= TTFLAG_BOOL_OK;
		*typep = type;
	    }
	}
    }

    return OK;
}

/*
 * expr4a && expr4a && expr4a	    logical AND
 *
 * Produces instructions:
 *	EVAL expr4a		Push result of "expr4a"
 *	JUMP_AND_KEEP_IF_FALSE end
 *	EVAL expr4b		Push result of "expr4b"
 *	JUMP_AND_KEEP_IF_FALSE end
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
 *	JUMP_AND_KEEP_IF_TRUE end
 *	EVAL expr3b		Push result of "expr3b"
 *	JUMP_AND_KEEP_IF_TRUE end
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
 *
 * Produces instructions:
 *	EVAL expr2		Push result of "expr"
 *      JUMP_IF_FALSE alt	jump if false
 *      EVAL expr1a
 *      JUMP_ALWAYS end
 * alt:	EVAL expr1b
 * end:
 */
    static int
compile_expr1(char_u **arg,  cctx_T *cctx, ppconst_T *ppconst)
{
    char_u	*p;
    int		ppconst_used = ppconst->pp_used;
    char_u	*next;

    // Ignore all kinds of errors when not producing code.
    if (cctx->ctx_skip == SKIP_YES)
    {
	skip_expr(arg);
	return OK;
    }

    // Evaluate the first expression.
    if (compile_expr2(arg, cctx, ppconst) == FAIL)
	return FAIL;

    p = may_peek_next_line(cctx, *arg, &next);
    if (*p == '?')
    {
	garray_T	*instr = &cctx->ctx_instr;
	garray_T	*stack = &cctx->ctx_type_stack;
	int		alt_idx = instr->ga_len;
	int		end_idx = 0;
	isn_T		*isn;
	type_T		*type1 = NULL;
	type_T		*type2;
	int		has_const_expr = FALSE;
	int		const_value = FALSE;
	int		save_skip = cctx->ctx_skip;

	if (next != NULL)
	{
	    *arg = next_line_from_context(cctx, TRUE);
	    p = skipwhite(*arg);
	}

	if (!IS_WHITE_OR_NUL(**arg) || !IS_WHITE_OR_NUL(p[1]))
	{
	    semsg(_(e_white_space_required_before_and_after_str), "?");
	    return FAIL;
	}

	if (ppconst->pp_used == ppconst_used + 1)
	{
	    // the condition is a constant, we know whether the ? or the :
	    // expression is to be evaluated.
	    has_const_expr = TRUE;
	    const_value = tv2bool(&ppconst->pp_tv[ppconst_used]);
	    clear_tv(&ppconst->pp_tv[ppconst_used]);
	    --ppconst->pp_used;
	    cctx->ctx_skip = save_skip == SKIP_YES || !const_value
							 ? SKIP_YES : SKIP_NOT;
	}
	else
	{
	    generate_ppconst(cctx, ppconst);
	    generate_JUMP(cctx, JUMP_IF_FALSE, 0);
	}

	// evaluate the second expression; any type is accepted
	*arg = skipwhite(p + 1);
	if (may_get_next_line(p + 1, arg, cctx) == FAIL)
	    return FAIL;
	if (compile_expr1(arg, cctx, ppconst) == FAIL)
	    return FAIL;

	if (!has_const_expr)
	{
	    generate_ppconst(cctx, ppconst);

	    // remember the type and drop it
	    --stack->ga_len;
	    type1 = ((type_T **)stack->ga_data)[stack->ga_len];

	    end_idx = instr->ga_len;
	    generate_JUMP(cctx, JUMP_ALWAYS, 0);

	    // jump here from JUMP_IF_FALSE
	    isn = ((isn_T *)instr->ga_data) + alt_idx;
	    isn->isn_arg.jump.jump_where = instr->ga_len;
	}

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
	    semsg(_(e_white_space_required_before_and_after_str), ":");
	    return FAIL;
	}

	// evaluate the third expression
	if (has_const_expr)
	    cctx->ctx_skip = save_skip == SKIP_YES || const_value
							 ? SKIP_YES : SKIP_NOT;
	*arg = skipwhite(p + 1);
	if (may_get_next_line(p + 1, arg, cctx) == FAIL)
	    return FAIL;
	if (compile_expr1(arg, cctx, ppconst) == FAIL)
	    return FAIL;

	if (!has_const_expr)
	{
	    generate_ppconst(cctx, ppconst);

	    // If the types differ, the result has a more generic type.
	    type2 = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    common_type(type1, type2, &type2, cctx->ctx_type_list);

	    // jump here from JUMP_ALWAYS
	    isn = ((isn_T *)instr->ga_data) + end_idx;
	    isn->isn_arg.jump.jump_where = instr->ga_len;
	}

	cctx->ctx_skip = save_skip;
    }
    return OK;
}

/*
 * Toplevel expression.
 */
    static int
compile_expr0(char_u **arg,  cctx_T *cctx)
{
    ppconst_T	ppconst;

    CLEAR_FIELD(ppconst);
    if (compile_expr1(arg, cctx, &ppconst) == FAIL)
    {
	clear_ppconst(&ppconst);
	return FAIL;
    }
    if (generate_ppconst(cctx, &ppconst) == FAIL)
	return FAIL;
    return OK;
}

/*
 * compile "return [expr]"
 */
    static char_u *
compile_return(char_u *arg, int set_return_type, cctx_T *cctx)
{
    char_u	*p = arg;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*stack_type;

    if (*p != NUL && *p != '|' && *p != '\n')
    {
	// compile return argument into instructions
	if (compile_expr0(&p, cctx) == FAIL)
	    return NULL;

	stack_type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	if (set_return_type)
	    cctx->ctx_ufunc->uf_ret_type = stack_type;
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
							  cctx, FALSE) == FAIL)
		return NULL;
	}
    }
    else
    {
	// "set_return_type" cannot be TRUE, only used for a lambda which
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

    if (generate_instr(cctx, ISN_RETURN) == NULL)
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
	if (cctx->ctx_lnum == cctx->ctx_ufunc->uf_lines.ga_len)
	    return NULL;
	++cctx->ctx_lnum;
	p = ((char_u **)cctx->ctx_ufunc->uf_lines.ga_data)[cctx->ctx_lnum];
	// Comment lines result in NULL pointers, skip them.
	if (p != NULL)
	    return vim_strsave(p);
    }
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
    lvar_T	*lvar;
    ufunc_T	*ufunc;
    int		r;

    if (eap->forceit)
    {
	emsg(_(e_cannot_use_bang_with_nested_def));
	return NULL;
    }

    // Only g:Func() can use a namespace.
    if (name_start[1] == ':' && !is_global)
    {
	semsg(_(e_namespace_not_supported_str), name_start);
	return NULL;
    }
    if (check_defined(name_start, name_end - name_start, cctx) == FAIL)
	return NULL;

    eap->arg = name_end;
    eap->getline = exarg_getline;
    eap->cookie = cctx;
    eap->skip = cctx->ctx_skip == SKIP_YES;
    eap->forceit = FALSE;
    lambda_name = get_lambda_name();
    ufunc = def_function(eap, lambda_name);

    if (ufunc == NULL)
	return eap->skip ? (char_u *)"" : NULL;
    if (ufunc->uf_def_status == UF_TO_BE_COMPILED
	    && compile_def_function(ufunc, TRUE, cctx) == FAIL)
	return NULL;

    if (is_global)
    {
	char_u *func_name = vim_strnsave(name_start + 2,
						    name_end - name_start - 2);

	if (func_name == NULL)
	    r = FAIL;
	else
	    r = generate_NEWFUNC(cctx, lambda_name, func_name);
    }
    else
    {
	// Define a local variable for the function reference.
	lvar = reserve_local(cctx, name_start, name_end - name_start,
						    TRUE, ufunc->uf_func_type);
	if (lvar == NULL)
	    return NULL;
	if (generate_FUNCREF(cctx, ufunc) == FAIL)
	    return NULL;
	r = generate_STORE(cctx, ISN_STORE, lvar->lv_idx, NULL);
    }

    // TODO: warning for trailing text?
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

// words that cannot be used as a variable
static char *reserved[] = {
    "true",
    "false",
    NULL
};

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
} assign_dest_T;

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
	    generate_LOAD(cctx, ISN_LOADG, 0, name + 2, type);
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
	    if (lvar->lv_from_outer)
		generate_LOAD(cctx, ISN_LOADOUTER, lvar->lv_idx,
							   NULL, type);
	    else
		generate_LOAD(cctx, ISN_LOAD, lvar->lv_idx, NULL, type);
	    break;
    }
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
 * Compile declaration and assignment:
 * "let var", "let var = expr", "const var = expr" and "var = expr"
 * "arg" points to "var".
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
    int		scriptvar_sid = 0;
    int		scriptvar_idx = -1;
    int		semicolon = 0;
    garray_T	*instr = &cctx->ctx_instr;
    garray_T    *stack = &cctx->ctx_type_stack;
    char_u	*op;
    int		oplen = 0;
    int		heredoc = FALSE;
    type_T	*type = &t_any;
    type_T	*member_type = &t_any;
    char_u	*name = NULL;
    char_u	*sp;
    int		is_decl = cmdidx == CMD_let || cmdidx == CMD_const;

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

    sp = p;
    p = skipwhite(p);
    op = p;
    oplen = assignment_len(p, &heredoc);

    if (var_count > 0 && oplen == 0)
	// can be something like "[1, 2]->func()"
	return arg;

    if (oplen > 0 && (!VIM_ISWHITE(*sp) || !VIM_ISWHITE(op[oplen])))
    {
	error_white_both(op, oplen);
	return NULL;
    }

    if (heredoc)
    {
	list_T	   *l;
	listitem_T *li;

	// [let] varname =<< [trim] {end}
	eap->getline = exarg_getline;
	eap->cookie = cctx;
	l = heredoc_get(eap, op + 3, FALSE);

	// Push each line and the create the list.
	FOR_ALL_LIST_ITEMS(l, li)
	{
	    generate_PUSHS(cctx, li->li_tv.vval.v_string);
	    li->li_tv.vval.v_string = NULL;
	}
	generate_NEWLIST(cctx, l->lv_len);
	type = &t_list_string;
	member_type = &t_list_string;
	list_free(l);
	p += STRLEN(p);
	end = p;
    }
    else if (var_count > 0)
    {
	// for "[var, var] = expr" evaluate the expression here, loop over the
	// list of variables below.

	p = skipwhite(op + oplen);
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
	    if (need_type(stacktype, &t_list_any, -1, cctx, FALSE) == FAIL)
		goto theend;
	    // TODO: check the length of a constant list here
	    generate_CHECKLEN(cctx, semicolon ? var_count - 1 : var_count,
								    semicolon);
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
	char_u		*var_end = skip_var_one(var_start, FALSE);
	size_t		varlen;
	int		new_local = FALSE;
	int		opt_type;
	int		opt_flags = 0;
	assign_dest_T	dest = dest_local;
	int		vimvaridx = -1;
	lvar_T		*lvar = NULL;
	lvar_T		arg_lvar;
	int		has_type = FALSE;
	int		has_index = FALSE;
	int		instr_count = -1;

	if (*var_start == '@')
	    p = var_start + 2;
	else
	{
	    // skip over the leading "&", "&l:", "&g:" and "$"
	    p = skip_option_env_lead(var_start);
	    p = to_name_end(p, TRUE);
	}

	// "a: type" is declaring variable "a" with a type, not "a:".
	if (is_decl && var_end == var_start + 2 && var_end[-1] == ':')
	    --var_end;
	if (is_decl && p == var_start + 2 && p[-1] == ':')
	    --p;

	varlen = p - var_start;
	vim_free(name);
	name = vim_strnsave(var_start, varlen);
	if (name == NULL)
	    return NULL;
	if (!heredoc)
	    type = &t_any;

	if (cctx->ctx_skip != SKIP_YES)
	{
	    int	    declare_error = FALSE;

	    if (*var_start == '&')
	    {
		int	    cc;
		long	    numval;

		dest = dest_option;
		if (cmdidx == CMD_const)
		{
		    emsg(_(e_const_option));
		    goto theend;
		}
		declare_error = is_decl;
		p = var_start;
		p = find_option_end(&p, &opt_flags);
		if (p == NULL)
		{
		    // cannot happen?
		    emsg(_(e_letunexp));
		    goto theend;
		}
		cc = *p;
		*p = NUL;
		opt_type = get_option_value(skip_option_env_lead(var_start),
						     &numval, NULL, opt_flags);
		*p = cc;
		if (opt_type == -3)
		{
		    semsg(_(e_unknown_option), var_start);
		    goto theend;
		}
		if (opt_type == -2 || opt_type == 0)
		    type = &t_string;
		else
		    type = &t_number;	// both number and boolean option
	    }
	    else if (*var_start == '$')
	    {
		dest = dest_env;
		type = &t_string;
		declare_error = is_decl;
	    }
	    else if (*var_start == '@')
	    {
		if (!valid_yank_reg(var_start[1], FALSE) || var_start[1] == '.')
		{
		    emsg_invreg(var_start[1]);
		    goto theend;
		}
		dest = dest_reg;
		type = &t_string;
		declare_error = is_decl;
	    }
	    else if (varlen > 1 && STRNCMP(var_start, "g:", 2) == 0)
	    {
		dest = dest_global;
		declare_error = is_decl;
	    }
	    else if (varlen > 1 && STRNCMP(var_start, "b:", 2) == 0)
	    {
		dest = dest_buffer;
		declare_error = is_decl;
	    }
	    else if (varlen > 1 && STRNCMP(var_start, "w:", 2) == 0)
	    {
		dest = dest_window;
		declare_error = is_decl;
	    }
	    else if (varlen > 1 && STRNCMP(var_start, "t:", 2) == 0)
	    {
		dest = dest_tab;
		declare_error = is_decl;
	    }
	    else if (varlen > 1 && STRNCMP(var_start, "v:", 2) == 0)
	    {
		typval_T	*vtv;
		int		di_flags;

		vimvaridx = find_vim_var(name + 2, &di_flags);
		if (vimvaridx < 0)
		{
		    semsg(_(e_variable_not_found_str), var_start);
		    goto theend;
		}
		// We use the current value of "sandbox" here, is that OK?
		if (var_check_ro(di_flags, name, FALSE))
		    goto theend;
		dest = dest_vimvar;
		vtv = get_vim_var_tv(vimvaridx);
		type = typval2type_vimvar(vtv, cctx->ctx_type_list);
		declare_error = is_decl;
	    }
	    else
	    {
		int	    idx;

		for (idx = 0; reserved[idx] != NULL; ++idx)
		    if (STRCMP(reserved[idx], name) == 0)
		    {
			semsg(_(e_cannot_use_reserved_name), name);
			goto theend;
		    }

		lvar = lookup_local(var_start, varlen, cctx);
		if (lvar == NULL)
		{
		    CLEAR_FIELD(arg_lvar);
		    if (lookup_arg(var_start, varlen,
				&arg_lvar.lv_idx, &arg_lvar.lv_type,
				&arg_lvar.lv_from_outer, cctx) == OK)
		    {
			if (is_decl)
			{
			    semsg(_(e_str_is_used_as_argument), name);
			    goto theend;
			}
			lvar = &arg_lvar;
		    }
		}
		if (lvar != NULL)
		{
		    if (is_decl)
		    {
			semsg(_(e_variable_already_declared), name);
			goto theend;
		    }
		}
		else
		{
		    int script_namespace = varlen > 1
					   && STRNCMP(var_start, "s:", 2) == 0;
		    int script_var = (script_namespace
			      ? lookup_script(var_start + 2, varlen - 2, FALSE)
			      : lookup_script(var_start, varlen, TRUE)) == OK;
		    imported_T  *import =
					find_imported(var_start, varlen, cctx);

		    if (script_namespace || script_var || import != NULL)
		    {
			char_u	*rawname = name + (name[1] == ':' ? 2 : 0);

			if (is_decl)
			{
			    if (script_namespace)
				semsg(_(e_cannot_declare_script_variable_in_function),
									 name);
			    else
				semsg(_(e_variable_already_declared_in_script),
									 name);
			    goto theend;
			}
			else if (cctx->ctx_ufunc->uf_script_ctx_version
							== SCRIPT_VERSION_VIM9
				&& script_namespace
					      && !script_var && import == NULL)
			{
			    semsg(_(e_unknown_variable_str), name);
			    goto theend;
			}

			dest = dest_script;

			// existing script-local variables should have a type
			scriptvar_sid = current_sctx.sc_sid;
			if (import != NULL)
			    scriptvar_sid = import->imp_sid;
			if (SCRIPT_ID_VALID(scriptvar_sid))
			{
			    scriptvar_idx = get_script_item_idx(scriptvar_sid,
								rawname, TRUE);
			    if (scriptvar_idx > 0)
			    {
				scriptitem_T *si = SCRIPT_ITEM(scriptvar_sid);
				svar_T	     *sv =
					    ((svar_T *)si->sn_var_vals.ga_data)
							       + scriptvar_idx;
				type = sv->sv_type;
			    }
			}
		    }
		    else if (name[1] == ':' && name[2] != NUL)
		    {
			semsg(_(e_cannot_use_namespaced_variable), name);
			goto theend;
		    }
		    else if (!is_decl)
		    {
			semsg(_(e_unknown_variable_str), name);
			goto theend;
		    }
		    else if (check_defined(var_start, varlen, cctx) == FAIL)
			goto theend;
		}
	    }

	    if (declare_error)
	    {
		vim9_declare_error(name);
		goto theend;
	    }
	}

	// handle "a:name" as a name, not index "name" on "a"
	if (varlen > 1 || var_start[varlen] != ':')
	    p = var_end;

	if (dest != dest_option)
	{
	    if (is_decl && *p == ':')
	    {
		// parse optional type: "let var: type = expr"
		if (!VIM_ISWHITE(p[1]))
		{
		    semsg(_(e_white_space_required_after_str), ":");
		    goto theend;
		}
		p = skipwhite(p + 1);
		type = parse_type(&p, cctx->ctx_type_list);
		has_type = TRUE;
	    }
	    else if (lvar != NULL)
		type = lvar->lv_type;
	}

	if (oplen == 3 && !heredoc && dest != dest_global
			&& type->tt_type != VAR_STRING
			&& type->tt_type != VAR_ANY)
	{
	    emsg(_(e_can_only_concatenate_to_string));
	    goto theend;
	}

	if (lvar == NULL && dest == dest_local && cctx->ctx_skip != SKIP_YES)
	{
	    if (oplen > 1 && !heredoc)
	    {
		// +=, /=, etc. require an existing variable
		semsg(_(e_cannot_use_operator_on_new_variable), name);
		goto theend;
	    }

	    // new local variable
	    if ((type->tt_type == VAR_FUNC || type->tt_type == VAR_PARTIAL)
					    && var_wrong_func_name(name, TRUE))
		goto theend;
	    lvar = reserve_local(cctx, var_start, varlen,
						    cmdidx == CMD_const, type);
	    if (lvar == NULL)
		goto theend;
	    new_local = TRUE;
	}

	member_type = type;
	if (var_end > var_start + varlen)
	{
	    // Something follows after the variable: "var[idx]".
	    if (is_decl)
	    {
		emsg(_(e_cannot_use_index_when_declaring_variable));
		goto theend;
	    }

	    if (var_start[varlen] == '[')
	    {
		has_index = TRUE;
		if (type->tt_member == NULL)
		    member_type = &t_any;
		else
		    member_type = type->tt_member;
	    }
	    else
	    {
		semsg("Not supported yet: %s", var_start);
		goto theend;
	    }
	}
	else if (lvar == &arg_lvar)
	{
	    semsg(_(e_cannot_assign_to_argument), name);
	    goto theend;
	}
	if (!is_decl && lvar != NULL && lvar->lv_const && !has_index)
	{
	    semsg(_(e_cannot_assign_to_constant), name);
	    goto theend;
	}

	if (!heredoc)
	{
	    if (cctx->ctx_skip == SKIP_YES)
	    {
		if (oplen > 0 && var_count == 0)
		{
		    // skip over the "=" and the expression
		    p = skipwhite(op + oplen);
		    compile_expr0(&p, cctx);
		}
	    }
	    else if (oplen > 0)
	    {
		type_T	*stacktype;

		// For "var = expr" evaluate the expression.
		if (var_count == 0)
		{
		    int	r;

		    // for "+=", "*=", "..=" etc. first load the current value
		    if (*op != '=')
		    {
			generate_loadvar(cctx, dest, name, lvar, type);

			if (has_index)
			{
			    // TODO: get member from list or dict
			    emsg("Index with operation not supported yet");
			    goto theend;
			}
		    }

		    // Compile the expression.  Temporarily hide the new local
		    // variable here, it is not available to this expression.
		    if (new_local)
			--cctx->ctx_locals.ga_len;
		    instr_count = instr->ga_len;
		    p = skipwhite(op + oplen);
		    r = compile_expr0(&p, cctx);
		    if (new_local)
			++cctx->ctx_locals.ga_len;
		    if (r == FAIL)
			goto theend;
		}
		else if (semicolon && var_idx == var_count - 1)
		{
		    // For "[var; var] = expr" get the rest of the list
		    if (generate_SLICE(cctx, var_count - 1) == FAIL)
			goto theend;
		}
		else
		{
		    // For "[var, var] = expr" get the "var_idx" item from the
		    // list.
		    if (generate_GETITEM(cctx, var_idx) == FAIL)
			return FAIL;
		}

		stacktype = stack->ga_len == 0 ? &t_void
			      : ((type_T **)stack->ga_data)[stack->ga_len - 1];
		if (lvar != NULL && (is_decl || !has_type))
		{
		    if ((stacktype->tt_type == VAR_FUNC
				|| stacktype->tt_type == VAR_PARTIAL)
			    && var_wrong_func_name(name, TRUE))
			goto theend;

		    if (new_local && !has_type)
		    {
			if (stacktype->tt_type == VAR_VOID)
			{
			    emsg(_(e_cannot_use_void_value));
			    goto theend;
			}
			else
			{
			    // An empty list or dict has a &t_void member,
			    // for a variable that implies &t_any.
			    if (stacktype == &t_list_empty)
				lvar->lv_type = &t_list_any;
			    else if (stacktype == &t_dict_empty)
				lvar->lv_type = &t_dict_any;
			    else
				lvar->lv_type = stacktype;
			}
		    }
		    else if (*op == '=')
		    {
			type_T *use_type = lvar->lv_type;

			// without operator check type here, otherwise below
			if (has_index)
			{
			    use_type = use_type->tt_member;
			    if (use_type == NULL)
				// could be indexing "any"
				use_type = &t_any;
			}
			if (need_type(stacktype, use_type, -1, cctx, FALSE)
								       == FAIL)
			    goto theend;
		    }
		}
		else if (*p != '=' && need_type(stacktype, member_type, -1,
							  cctx, FALSE) == FAIL)
		    goto theend;
	    }
	    else if (cmdidx == CMD_const)
	    {
		emsg(_(e_const_requires_a_value));
		goto theend;
	    }
	    else if (!has_type || dest == dest_option)
	    {
		emsg(_(e_type_or_initialization_required));
		goto theend;
	    }
	    else
	    {
		// variables are always initialized
		if (ga_grow(instr, 1) == FAIL)
		    goto theend;
		switch (member_type->tt_type)
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
			generate_PUSHBLOB(cctx, NULL);
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
		expected = member_type;
	    stacktype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    if (
#ifdef FEAT_FLOAT
		// If variable is float operation with number is OK.
		!(expected == &t_float && stacktype == &t_number) &&
#endif
		    need_type(stacktype, expected, -1, cctx, FALSE) == FAIL)
		goto theend;

	    if (*op == '.')
	    {
		if (generate_instr_drop(cctx, ISN_CONCAT, 1) == NULL)
		    goto theend;
	    }
	    else if (*op == '+')
	    {
		if (generate_add_instr(cctx,
			    operator_type(member_type, stacktype),
					       member_type, stacktype) == FAIL)
		    goto theend;
	    }
	    else if (generate_two_op(cctx, op) == FAIL)
		goto theend;
	}

	if (has_index)
	{
	    int r;

	    // Compile the "idx" in "var[idx]".
	    if (new_local)
		--cctx->ctx_locals.ga_len;
	    p = skipwhite(var_start + varlen + 1);
	    r = compile_expr0(&p, cctx);
	    if (new_local)
		++cctx->ctx_locals.ga_len;
	    if (r == FAIL)
		goto theend;
	    if (*skipwhite(p) != ']')
	    {
		// this should not happen
		emsg(_(e_missbrac));
		goto theend;
	    }
	    if (type == &t_any)
	    {
		type_T	    *idx_type = ((type_T **)stack->ga_data)[
							    stack->ga_len - 1];
		// Index on variable of unknown type: guess the type from the
		// index type: number is dict, otherwise dict.
		// TODO: should do the assignment at runtime
		if (idx_type->tt_type == VAR_NUMBER)
		    type = &t_list_any;
		else
		    type = &t_dict_any;
	    }
	    if (type->tt_type == VAR_DICT
		    && may_generate_2STRING(-1, cctx) == FAIL)
		goto theend;
	    if (type->tt_type == VAR_LIST
		    && ((type_T **)stack->ga_data)[stack->ga_len - 1]->tt_type
								 != VAR_NUMBER)
	    {
		emsg(_(e_number_exp));
		goto theend;
	    }

	    // Load the dict or list.  On the stack we then have:
	    // - value
	    // - index
	    // - variable
	    generate_loadvar(cctx, dest, name, lvar, type);

	    if (type->tt_type == VAR_LIST)
	    {
		if (generate_instr_drop(cctx, ISN_STORELIST, 3) == FAIL)
		    return FAIL;
	    }
	    else if (type->tt_type == VAR_DICT)
	    {
		if (generate_instr_drop(cctx, ISN_STOREDICT, 3) == FAIL)
		    return FAIL;
	    }
	    else
	    {
		emsg(_(e_listreq));
		goto theend;
	    }
	}
	else
	{
	    if (is_decl && eap->forceit && cmdidx == CMD_const
		    && (dest == dest_script || dest == dest_local))
		// ":const! var": lock the value, but not referenced variables
		generate_LOCKCONST(cctx);

	    switch (dest)
	    {
		case dest_option:
		    generate_STOREOPT(cctx, skip_option_env_lead(name),
								    opt_flags);
		    break;
		case dest_global:
		    // include g: with the name, easier to execute that way
		    generate_STORE(cctx, ISN_STOREG, 0, name);
		    break;
		case dest_buffer:
		    // include b: with the name, easier to execute that way
		    generate_STORE(cctx, ISN_STOREB, 0, name);
		    break;
		case dest_window:
		    // include w: with the name, easier to execute that way
		    generate_STORE(cctx, ISN_STOREW, 0, name);
		    break;
		case dest_tab:
		    // include t: with the name, easier to execute that way
		    generate_STORE(cctx, ISN_STORET, 0, name);
		    break;
		case dest_env:
		    generate_STORE(cctx, ISN_STOREENV, 0, name + 1);
		    break;
		case dest_reg:
		    generate_STORE(cctx, ISN_STOREREG, name[1], NULL);
		    break;
		case dest_vimvar:
		    generate_STORE(cctx, ISN_STOREV, vimvaridx, NULL);
		    break;
		case dest_script:
		    {
			if (scriptvar_idx < 0)
			{
			    char_u *name_s = name;

			    // Include s: in the name for store_var()
			    if (name[1] != ':')
			    {
				int len = (int)STRLEN(name) + 3;

				name_s = alloc(len);
				if (name_s == NULL)
				    name_s = name;
				else
				    vim_snprintf((char *)name_s, len,
								 "s:%s", name);
			    }
			    generate_OLDSCRIPT(cctx, ISN_STORES, name_s,
							  scriptvar_sid, type);
			    if (name_s != name)
				vim_free(name_s);
			}
			else
			    generate_VIM9SCRIPT(cctx, ISN_STORESCRIPT,
					   scriptvar_sid, scriptvar_idx, type);
		    }
		    break;
		case dest_local:
		    if (lvar != NULL)
		    {
			isn_T *isn = ((isn_T *)instr->ga_data)
							   + instr->ga_len - 1;

			// optimization: turn "var = 123" from ISN_PUSHNR +
			// ISN_STORE into ISN_STORENR
			if (!lvar->lv_from_outer
					&& instr->ga_len == instr_count + 1
					&& isn->isn_type == ISN_PUSHNR)
			{
			    varnumber_T val = isn->isn_arg.number;

			    isn->isn_type = ISN_STORENR;
			    isn->isn_arg.storenr.stnr_idx = lvar->lv_idx;
			    isn->isn_arg.storenr.stnr_val = val;
			    if (stack->ga_len > 0)
				--stack->ga_len;
			}
			else if (lvar->lv_from_outer)
			    generate_STORE(cctx, ISN_STOREOUTER, lvar->lv_idx,
									 NULL);
			else
			    generate_STORE(cctx, ISN_STORE, lvar->lv_idx, NULL);
		    }
		    break;
	    }
	}

	if (var_idx + 1 < var_count)
	    var_start = skipwhite(var_end + 1);
    }

    // for "[var, var] = expr" drop the "expr" value
    if (var_count > 0 && !semicolon)
    {
	    if (generate_instr_drop(cctx, ISN_DROP, 1) == NULL)
	    goto theend;
    }

    ret = skipwhite(end);

theend:
    vim_free(name);
    return ret;
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
    cctx_T *cctx = coookie;

    if (lvp->ll_tv == NULL)
    {
	char_u	*p = lvp->ll_name;
	int	cc = *name_end;
	int	ret = OK;

	// Normal name.  Only supports g:, w:, t: and b: namespaces.
	*name_end = NUL;
	if (*p == '$')
	    ret = generate_UNLET(cctx, ISN_UNLETENV, p + 1, eap->forceit);
	else if (check_vim9_unlet(p) == FAIL)
	    ret = FAIL;
	else
	    ret = generate_UNLET(cctx, ISN_UNLET, p, eap->forceit);

	*name_end = cc;
	return ret;
    }

    // TODO: unlet {list}[idx]
    // TODO: unlet {dict}[key]
    emsg("Sorry, :unlet not fully implemented yet");
    return FAIL;
}

/*
 * compile "unlet var", "lock var" and "unlock var"
 * "arg" points to "var".
 */
    static char_u *
compile_unletlock(char_u *arg, exarg_T *eap, cctx_T *cctx)
{
    char_u *p = arg;

    if (eap->cmdidx != CMD_unlet)
    {
	emsg("Sorry, :lock and unlock not implemented yet");
	return NULL;
    }

    if (*p == '!')
    {
	p = skipwhite(p + 1);
	eap->forceit = TRUE;
    }

    ex_unletlock(eap, p, 0, GLV_NO_AUTOLOAD, compile_unlet, cctx);
    return eap->nextcmd == NULL ? (char_u *)"" : eap->nextcmd;
}

/*
 * Compile an :import command.
 */
    static char_u *
compile_import(char_u *arg, cctx_T *cctx)
{
    return handle_import(arg, &cctx->ctx_imports, 0, NULL, cctx);
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
compile_fill_jump_to_end(endlabel_T **el, cctx_T *cctx)
{
    garray_T	*instr = &cctx->ctx_instr;

    while (*el != NULL)
    {
	endlabel_T  *cur = (*el);
	isn_T	    *isn;

	isn = ((isn_T *)instr->ga_data) + cur->el_end_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
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
    if (cctx->ctx_skip == SKIP_YES)
	clear_ppconst(&ppconst);
    else if (instr->ga_len == instr_count && ppconst.pp_used == 1)
    {
	// The expression results in a constant.
	cctx->ctx_skip = tv2bool(&ppconst.pp_tv[0]) ? SKIP_NOT : SKIP_YES;
	clear_ppconst(&ppconst);
    }
    else
    {
	// Not a constant, generate instructions for the expression.
	cctx->ctx_skip = SKIP_UNKNOWN;
	if (generate_ppconst(cctx, &ppconst) == FAIL)
	    return NULL;
    }

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

    if (cctx->ctx_skip == SKIP_UNKNOWN)
    {
	if (compile_jump_to_end(&scope->se_u.se_if.is_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	    return NULL;
	// previous "if" or "elseif" jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_if.is_if_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
    }

    // compile "expr"; if we know it evaluates to FALSE skip the block
    CLEAR_FIELD(ppconst);
    if (cctx->ctx_skip == SKIP_YES)
	cctx->ctx_skip = SKIP_UNKNOWN;
    if (compile_expr1(&p, cctx, &ppconst) == FAIL)
    {
	clear_ppconst(&ppconst);
	return NULL;
    }
    cctx->ctx_skip = save_skip;
    if (scope->se_skip_save == SKIP_YES)
	clear_ppconst(&ppconst);
    else if (instr->ga_len == instr_count && ppconst.pp_used == 1)
    {
	// The expression results in a constant.
	// TODO: how about nesting?
	cctx->ctx_skip = tv2bool(&ppconst.pp_tv[0]) ? SKIP_NOT : SKIP_YES;
	clear_ppconst(&ppconst);
	scope->se_u.se_if.is_if_label = -1;
    }
    else
    {
	// Not a constant, generate instructions for the expression.
	cctx->ctx_skip = SKIP_UNKNOWN;
	if (generate_ppconst(cctx, &ppconst) == FAIL)
	    return NULL;

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

    if (scope->se_skip_save != SKIP_YES)
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
    compile_fill_jump_to_end(&ifscope->is_end_label, cctx);
    cctx->ctx_skip = scope->se_skip_save;

    // If all the blocks end in :return and there is an :else then the
    // had_return flag is set.
    cctx->ctx_had_return = ifscope->is_had_return && ifscope->is_seen_else;

    drop_scope(cctx);
    return arg;
}

/*
 * compile "for var in expr"
 *
 * Produces instructions:
 *       PUSHNR -1
 *       STORE loop-idx		Set index to -1
 *       EVAL expr		Push result of "expr"
 * top:  FOR loop-idx, end	Increment index, use list on bottom of stack
 *				- if beyond end, jump to "end"
 *				- otherwise get item from list and push it
 *       STORE var		Store item in "var"
 *       ... body ...
 *       JUMP top		Jump back to repeat
 * end:	 DROP			Drop the result of "expr"
 *
 */
    static char_u *
compile_for(char_u *arg, cctx_T *cctx)
{
    char_u	*p;
    size_t	varlen;
    garray_T	*instr = &cctx->ctx_instr;
    garray_T	*stack = &cctx->ctx_type_stack;
    scope_T	*scope;
    lvar_T	*loop_lvar;	// loop iteration variable
    lvar_T	*var_lvar;	// variable for "var"
    type_T	*vartype;

    // TODO: list of variables: "for [key, value] in dict"
    // parse "var"
    for (p = arg; eval_isnamec1(*p); ++p)
	;
    varlen = p - arg;
    var_lvar = lookup_local(arg, varlen, cctx);
    if (var_lvar != NULL)
    {
	semsg(_(e_variable_already_declared), arg);
	return NULL;
    }

    // consume "in"
    p = skipwhite(p);
    if (STRNCMP(p, "in", 2) != 0 || !VIM_ISWHITE(p[2]))
    {
	emsg(_(e_missing_in));
	return NULL;
    }
    p = skipwhite(p + 2);


    scope = new_scope(cctx, FOR_SCOPE);
    if (scope == NULL)
	return NULL;

    // Reserve a variable to store the loop iteration counter.
    loop_lvar = reserve_local(cctx, (char_u *)"", 0, FALSE, &t_number);
    if (loop_lvar == NULL)
    {
	// out of memory
	drop_scope(cctx);
	return NULL;
    }

    // Reserve a variable to store "var"
    var_lvar = reserve_local(cctx, arg, varlen, FALSE, &t_any);
    if (var_lvar == NULL)
    {
	// out of memory or used as an argument
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

    // Now that we know the type of "var", check that it is a list, now or at
    // runtime.
    vartype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    if (need_type(vartype, &t_list_any, -1, cctx, FALSE) == FAIL)
    {
	drop_scope(cctx);
	return NULL;
    }
    if (vartype->tt_type == VAR_LIST && vartype->tt_member->tt_type != VAR_ANY)
	var_lvar->lv_type = vartype->tt_member;

    // "for_end" is set when ":endfor" is found
    scope->se_u.se_for.fs_top_label = instr->ga_len;

    generate_FOR(cctx, loop_lvar->lv_idx);
    generate_STORE(cctx, ISN_STORE, var_lvar->lv_idx, NULL);

    return arg;
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

    // Fill in the "end" label in the FOR statement so it can jump here
    isn = ((isn_T *)instr->ga_data) + forscope->fs_top_label;
    isn->isn_arg.forloop.for_end = instr->ga_len;

    // Fill in the "end" label any BREAK statements
    compile_fill_jump_to_end(&forscope->fs_end_label, cctx);

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
    garray_T	*instr = &cctx->ctx_instr;
    scope_T	*scope;

    scope = new_scope(cctx, WHILE_SCOPE);
    if (scope == NULL)
	return NULL;

    scope->se_u.se_while.ws_top_label = instr->ga_len;

    // compile "expr"
    if (compile_expr0(&p, cctx) == FAIL)
	return NULL;

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

    if (scope == NULL || scope->se_type != WHILE_SCOPE)
    {
	emsg(_(e_while));
	return NULL;
    }
    cctx->ctx_scope = scope->se_outer;
    unwind_locals(cctx, scope->se_local_count);

    // At end of ":for" scope jump back to the FOR instruction.
    generate_JUMP(cctx, JUMP_ALWAYS, scope->se_u.se_while.ws_top_label);

    // Fill in the "end" label in the WHILE statement so it can jump here.
    // And in any jumps for ":break"
    compile_fill_jump_to_end(&scope->se_u.se_while.ws_end_label, cctx);

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

    for (;;)
    {
	if (scope == NULL)
	{
	    emsg(_(e_continue));
	    return NULL;
	}
	if (scope->se_type == FOR_SCOPE || scope->se_type == WHILE_SCOPE)
	    break;
	scope = scope->se_outer;
    }

    // Jump back to the FOR or WHILE instruction.
    generate_JUMP(cctx, JUMP_ALWAYS,
	    scope->se_type == FOR_SCOPE ? scope->se_u.se_for.fs_top_label
					  : scope->se_u.se_while.ws_top_label);
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
 * catch1:  PUSH exeception
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

    // scope that holds the jumps that go to catch/finally/endtry
    try_scope = new_scope(cctx, TRY_SCOPE);
    if (try_scope == NULL)
	return NULL;

    // "catch" is set when the first ":catch" is found.
    // "finally" is set when ":finally" or ":endtry" is found
    try_scope->se_u.se_try.ts_try_label = instr->ga_len;
    if (generate_instr(cctx, ISN_TRY) == NULL)
	return NULL;

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

    // Jump from end of previous block to :finally or :endtry
    if (compile_jump_to_end(&scope->se_u.se_try.ts_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	return NULL;

    // End :try or :catch scope: set value in ISN_TRY instruction
    isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_try_label;
    if (isn->isn_arg.try.try_catch == 0)
	isn->isn_arg.try.try_catch = instr->ga_len;
    if (scope->se_u.se_try.ts_catch_label != 0)
    {
	// Previous catch without match jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_catch_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
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

	end = skip_regexp_ex(p + 1, *p, TRUE, &tofree, &dropped);
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

    if (generate_instr(cctx, ISN_CATCH) == NULL)
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
    if (isn->isn_arg.try.try_finally != 0)
    {
	emsg(_(e_finally_dup));
	return NULL;
    }

    // Fill in the "end" label in jumps at the end of the blocks.
    compile_fill_jump_to_end(&scope->se_u.se_try.ts_end_label, cctx);

    isn->isn_arg.try.try_finally = instr->ga_len;
    if (scope->se_u.se_try.ts_catch_label != 0)
    {
	// Previous catch without match jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_catch_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
	scope->se_u.se_try.ts_catch_label = 0;
    }

    // TODO: set index in ts_finally_label jumps

    return arg;
}

    static char_u *
compile_endtry(char_u *arg, cctx_T *cctx)
{
    scope_T	*scope = cctx->ctx_scope;
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*isn;

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

    isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_try_label;
    if (isn->isn_arg.try.try_catch == 0 && isn->isn_arg.try.try_finally == 0)
    {
	emsg(_(e_missing_catch_or_finally));
	return NULL;
    }

    // Fill in the "end" label in jumps at the end of the blocks, if not done
    // by ":finally".
    compile_fill_jump_to_end(&scope->se_u.se_try.ts_end_label, cctx);

    // End :catch or :finally scope: set value in ISN_TRY instruction
    if (isn->isn_arg.try.try_catch == 0)
	isn->isn_arg.try.try_catch = instr->ga_len;
    if (isn->isn_arg.try.try_finally == 0)
	isn->isn_arg.try.try_finally = instr->ga_len;

    if (scope->se_u.se_try.ts_catch_label != 0)
    {
	// Last catch without match jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_u.se_try.ts_catch_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
    }

    compile_endblock(cctx);

    if (generate_instr(cctx, ISN_ENDTRY) == NULL)
	return NULL;
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
    if (may_generate_2STRING(-1, cctx) == FAIL)
	return NULL;
    if (generate_instr_drop(cctx, ISN_THROW, 1) == NULL)
	return NULL;

    return p;
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
    char_u	*prev;
    int		count = 0;

    for (;;)
    {
	if (compile_expr0(&p, cctx) == FAIL)
	    return NULL;
	++count;
	prev = p;
	p = skipwhite(p);
	if (ends_excmd2(prev, p))
	    break;
    }

    if (cmdidx == CMD_echo || cmdidx == CMD_echon)
	generate_ECHO(cctx, cmdidx == CMD_echo, count);
    else if (cmdidx == CMD_execute)
	generate_MULT_EXPR(cctx, ISN_EXECUTE, count);
    else if (cmdidx == CMD_echomsg)
	generate_MULT_EXPR(cctx, ISN_ECHOMSG, count);
    else
	generate_MULT_EXPR(cctx, ISN_ECHOERR, count);
    return p;
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

    // TODO: if the range is something like "$" need to evaluate at runtime
    if (parse_cmd_address(eap, &errormsg, FALSE) == FAIL)
	return NULL;
    if (eap->addr_count == 0)
	lnum = -1;
    else
	lnum = eap->line2;
    if (above)
	--lnum;

    generate_PUT(cctx, eap->regname, lnum);
    return line;
}

/*
 * A command that is not compiled, execute with legacy code.
 */
    static char_u *
compile_exec(char_u *line, exarg_T *eap, cctx_T *cctx)
{
    char_u  *p;
    int	    has_expr = FALSE;
    char_u  *nextcmd = (char_u *)"";

    if (cctx->ctx_skip == SKIP_YES)
	goto theend;

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
    }

    if (eap->cmdidx == CMD_syntax && STRNCMP(eap->arg, "include ", 8) == 0)
    {
	// expand filename in "syntax include [@group] filename"
	has_expr = TRUE;
	eap->arg = skipwhite(eap->arg + 7);
	if (*eap->arg == '@')
	    eap->arg = skiptowhite(eap->arg);
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
	    may_generate_2STRING(-1, cctx);
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
    ++def_functions.ga_len;
    return OK;
}

/*
 * After ex_function() has collected all the function lines: parse and compile
 * the lines into instructions.
 * Adds the function to "def_functions".
 * When "set_return_type" is set then set ufunc->uf_ret_type to the type of the
 * return statement (used for lambda).
 * "outer_cctx" is set for a nested function.
 * This can be used recursively through compile_lambda(), which may reallocate
 * "def_functions".
 * Returns OK or FAIL.
 */
    int
compile_def_function(ufunc_T *ufunc, int set_return_type, cctx_T *outer_cctx)
{
    char_u	*line = NULL;
    char_u	*p;
    char	*errormsg = NULL;	// error message
    cctx_T	cctx;
    garray_T	*instr;
    int		called_emsg_before = called_emsg;
    int		ret = FAIL;
    sctx_T	save_current_sctx = current_sctx;
    int		do_estack_push;
    int		emsg_before = called_emsg;
    int		new_def_function = FALSE;

    // When using a function that was compiled before: Free old instructions.
    // Otherwise add a new entry in "def_functions".
    if (ufunc->uf_dfunc_idx > 0)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;
	delete_def_function_contents(dfunc);
    }
    else
    {
	if (add_def_function(ufunc) == FAIL)
	    return FAIL;
	new_def_function = TRUE;
    }

    ufunc->uf_def_status = UF_COMPILING;

    CLEAR_FIELD(cctx);
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

    // Make sure error messages are OK.
    do_estack_push = !estack_top_is_ufunc(ufunc, 1);
    if (do_estack_push)
	estack_push_ufunc(ufunc, 1);

    if (ufunc->uf_def_args.ga_len > 0)
    {
	int	count = ufunc->uf_def_args.ga_len;
	int	first_def_arg = ufunc->uf_args.ga_len - count;
	int	i;
	char_u	*arg;
	int	off = STACK_FRAME_SIZE + (ufunc->uf_va_name != NULL ? 1 : 0);
	int	did_set_arg_type = FALSE;

	// Produce instructions for the default values of optional arguments.
	// Store the instruction index in uf_def_arg_idx[] so that we know
	// where to start when the function is called, depending on the number
	// of arguments.
	ufunc->uf_def_arg_idx = ALLOC_CLEAR_MULT(int, count + 1);
	if (ufunc->uf_def_arg_idx == NULL)
	    goto erret;
	for (i = 0; i < count; ++i)
	{
	    garray_T	*stack = &cctx.ctx_type_stack;
	    type_T	*val_type;
	    int		arg_idx = first_def_arg + i;

	    ufunc->uf_def_arg_idx[i] = instr->ga_len;
	    arg = ((char_u **)(ufunc->uf_def_args.ga_data))[i];
	    if (compile_expr0(&arg, &cctx) == FAIL)
		goto erret;

	    // If no type specified use the type of the default value.
	    // Otherwise check that the default value type matches the
	    // specified type.
	    val_type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	    if (ufunc->uf_arg_types[arg_idx] == &t_unknown)
	    {
		did_set_arg_type = TRUE;
		ufunc->uf_arg_types[arg_idx] = val_type;
	    }
	    else if (check_type(ufunc->uf_arg_types[arg_idx], val_type,
						    TRUE, arg_idx + 1) == FAIL)
		goto erret;

	    if (generate_STORE(&cctx, ISN_STORE, i - count - off, NULL) == FAIL)
		goto erret;
	}
	ufunc->uf_def_arg_idx[count] = instr->ga_len;

	if (did_set_arg_type)
	    set_function_type(ufunc);
    }

    /*
     * Loop over all the lines of the function and generate instructions.
     */
    for (;;)
    {
	exarg_T	    ea;
	cmdmod_T    save_cmdmod;
	int	    starts_with_colon = FALSE;
	char_u	    *cmd;
	int	    save_msg_scroll = msg_scroll;

	// Bail out on the first error to avoid a flood of errors and report
	// the right line number when inside try/catch.
	if (emsg_before != called_emsg)
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
	else
	{
	    line = next_line_from_context(&cctx, FALSE);
	    if (cctx.ctx_lnum >= ufunc->uf_lines.ga_len)
		// beyond the last line
		break;
	}
	emsg_before = called_emsg;

	CLEAR_FIELD(ea);
	ea.cmdlinep = &line;
	ea.cmd = skipwhite(line);

	// Some things can be recognized by the first character.
	switch (*ea.cmd)
	{
	    case '#':
		// "#" starts a comment, but "#{" does not.
		if (ea.cmd[1] != '{')
		{
		    line = (char_u *)"";
		    continue;
		}
		break;

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

	    case ':':
		starts_with_colon = TRUE;
		break;
	}

	/*
	 * COMMAND MODIFIERS
	 */
	save_cmdmod = cmdmod;
	if (parse_command_modifiers(&ea, &errormsg, FALSE) == FAIL)
	{
	    if (errormsg != NULL)
		goto erret;
	    // empty line or comment
	    line = (char_u *)"";
	    continue;
	}
	// TODO: use modifiers in the command
	undo_cmdmod(&ea, save_msg_scroll);
	cmdmod = save_cmdmod;

	// Skip ":call" to get to the function name.
	p = ea.cmd;
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
	    char_u *pskip;

	    // Assuming the command starts with a variable or function name,
	    // find what follows.
	    // Skip over "var.member", "var[idx]" and the like.
	    // Also "&opt = val", "$ENV = val" and "@r = val".
	    pskip = (*ea.cmd == '&' || *ea.cmd == '$' || *ea.cmd == '@')
							 ? ea.cmd + 1 : ea.cmd;
	    p = to_name_end(pskip, TRUE);
	    if (p > ea.cmd && *p != NUL)
	    {
		char_u *var_end;
		int	oplen;
		int	heredoc;

		if (ea.cmd[0] == '@')
		    var_end = ea.cmd + 2;
		else
		    var_end = find_name_end(pskip, NULL, NULL,
						FNE_CHECK_START | FNE_INCL_BR);
		oplen = assignment_len(skipwhite(var_end), &heredoc);
		if (oplen > 0)
		{
		    size_t len = p - ea.cmd;

		    // Recognize an assignment if we recognize the variable
		    // name:
		    // "g:var = expr"
		    // "local = expr"  where "local" is a local var.
		    // "script = expr"  where "script" is a script-local var.
		    // "import = expr"  where "import" is an imported var
		    // "&opt = expr"
		    // "$ENV = expr"
		    // "@r = expr"
		    if (*ea.cmd == '&'
			    || *ea.cmd == '$'
			    || *ea.cmd == '@'
			    || ((len) > 2 && ea.cmd[1] == ':')
			    || lookup_local(ea.cmd, len, &cctx) != NULL
			    || lookup_arg(ea.cmd, len, NULL, NULL,
							     NULL, &cctx) == OK
			    || lookup_script(ea.cmd, len, FALSE) == OK
			    || find_imported(ea.cmd, len, &cctx) != NULL)
		    {
			line = compile_assignment(ea.cmd, &ea, CMD_SIZE, &cctx);
			if (line == NULL || line == ea.cmd)
			    goto erret;
			continue;
		    }
		}
	    }

	    if (*ea.cmd == '[')
	    {
		// [var, var] = expr
		line = compile_assignment(ea.cmd, &ea, CMD_SIZE, &cctx);
		if (line == NULL)
		    goto erret;
		if (line != ea.cmd)
		    continue;
	    }
	}

	/*
	 * COMMAND after range
	 * 'text'->func() should not be confused with 'a mark
	 */
	cmd = ea.cmd;
	if (*cmd != '\'' || starts_with_colon)
	{
	    ea.cmd = skip_range(ea.cmd, TRUE, NULL);
	    if (ea.cmd > cmd)
	    {
		if (!starts_with_colon)
		{
		    emsg(_(e_colon_required_before_a_range));
		    goto erret;
		}
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
	p = find_ex_command(&ea, NULL, starts_with_colon ? NULL
		   : (void *(*)(char_u *, size_t, cctx_T *))lookup_local,
									&cctx);

	if (p == ea.cmd && ea.cmdidx != CMD_SIZE)
	{
	    if (cctx.ctx_skip == SKIP_YES)
	    {
		line += STRLEN(line);
		continue;
	    }

	    // Expression or function call.
	    if (ea.cmdidx != CMD_eval)
	    {
		// CMD_let cannot happen, compile_assignment() above is used
		iemsg("Command from find_ex_command() not handled");
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
		    line = compile_return(p, set_return_type, &cctx);
		    cctx.ctx_had_return = TRUE;
		    break;

	    case CMD_let:
	    case CMD_const:
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
		    line = compile_import(p, &cctx);
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
		    if (compile_expr0(&p, &cctx) == FAIL)
			goto erret;

		    // drop the result
		    generate_instr_drop(&cctx, ISN_DROP, 1);

		    line = skipwhite(p);
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

	    // TODO: any other commands with an expression argument?

	    case CMD_append:
	    case CMD_change:
	    case CMD_insert:
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

	    default:
		    if (cctx.ctx_skip == SKIP_YES)
		    {
			// We don't check for a next command here.
			line = (char_u *)"";
		    }
		    else
		    {
			// Not recognized, execute with do_cmdline_cmd().
			ea.arg = p;
			line = compile_exec(line, &ea, &cctx);
		    }
		    break;
	}
nextline:
	if (line == NULL)
	    goto erret;
	line = skipwhite(line);

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
	if (ufunc->uf_ret_type->tt_type != VAR_VOID)
	{
	    emsg(_(e_missing_return_statement));
	    goto erret;
	}

	// Return zero if there is no return at the end.
	generate_PUSHNR(&cctx, 0);
	generate_instr(&cctx, ISN_RETURN);
    }

    {
	dfunc_T	*dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;
	dfunc->df_deleted = FALSE;
	dfunc->df_instr = instr->ga_data;
	dfunc->df_instr_count = instr->ga_len;
	dfunc->df_varcount = cctx.ctx_locals_count;
	dfunc->df_closure_count = cctx.ctx_closure_count;
	if (cctx.ctx_outer_used)
	    ufunc->uf_flags |= FC_CLOSURE;
	ufunc->uf_def_status = UF_COMPILED;
    }

    ret = OK;

erret:
    if (ret == FAIL)
    {
	int idx;
	dfunc_T	*dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;

	for (idx = 0; idx < instr->ga_len; ++idx)
	    delete_instr(((isn_T *)instr->ga_data) + idx);
	ga_clear(instr);

	// If using the last entry in the table and it was added above, we
	// might as well remove it.
	if (!dfunc->df_deleted && new_def_function
			    && ufunc->uf_dfunc_idx == def_functions.ga_len - 1)
	{
	    --def_functions.ga_len;
	    ufunc->uf_dfunc_idx = 0;
	}
	ufunc->uf_def_status = UF_NOT_COMPILED;

	while (cctx.ctx_scope != NULL)
	    drop_scope(&cctx);

	// Don't execute this function body.
	ga_clear_strings(&ufunc->uf_lines);

	if (errormsg != NULL)
	    emsg(errormsg);
	else if (called_emsg == called_emsg_before)
	    emsg(_(e_compile_def_function_failed));
    }

    current_sctx = save_current_sctx;
    if (do_estack_push)
	estack_pop();

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
		    ufunc->uf_va_type == NULL ? &t_any : ufunc->uf_va_type;
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
	case ISN_EXEC:
	case ISN_LOADENV:
	case ISN_LOADG:
	case ISN_LOADB:
	case ISN_LOADW:
	case ISN_LOADT:
	case ISN_LOADOPT:
	case ISN_STRINGMEMBER:
	case ISN_PUSHEXC:
	case ISN_PUSHS:
	case ISN_STOREENV:
	case ISN_STOREG:
	case ISN_STOREB:
	case ISN_STOREW:
	case ISN_STORET:
	case ISN_PUSHFUNC:
	    vim_free(isn->isn_arg.string);
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
		func_ptr_unref(dfunc->df_ufunc);
	    }
	    break;

	case ISN_NEWFUNC:
	    {
		char_u  *lambda = isn->isn_arg.newfunc.nf_lambda;
		ufunc_T *ufunc = find_func_even_dead(lambda, TRUE, NULL);

		if (ufunc != NULL)
		{
		    // Clear uf_dfunc_idx so that the function is deleted.
		    clear_def_function(ufunc);
		    ufunc->uf_dfunc_idx = 0;
		    func_ptr_unref(ufunc);
		}

		vim_free(lambda);
		vim_free(isn->isn_arg.newfunc.nf_global);
	    }
	    break;

	case ISN_CHECKTYPE:
	    free_type(isn->isn_arg.type.ct_type);
	    break;

	case ISN_2BOOL:
	case ISN_2STRING:
	case ISN_2STRING_ANY:
	case ISN_ADDBLOB:
	case ISN_ADDLIST:
	case ISN_ANYINDEX:
	case ISN_ANYSLICE:
	case ISN_BCALL:
	case ISN_CATCH:
	case ISN_CHECKLEN:
	case ISN_CHECKNR:
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
	case ISN_DCALL:
	case ISN_DROP:
	case ISN_ECHO:
	case ISN_ECHOERR:
	case ISN_ECHOMSG:
	case ISN_ENDTRY:
	case ISN_EXECCONCAT:
	case ISN_EXECUTE:
	case ISN_FOR:
	case ISN_GETITEM:
	case ISN_JUMP:
	case ISN_LISTINDEX:
	case ISN_LISTSLICE:
	case ISN_LOAD:
	case ISN_LOADBDICT:
	case ISN_LOADGDICT:
	case ISN_LOADOUTER:
	case ISN_LOADREG:
	case ISN_LOADSCRIPT:
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
	case ISN_PUSHBOOL:
	case ISN_PUSHF:
	case ISN_PUSHNR:
	case ISN_PUSHSPEC:
	case ISN_PUT:
	case ISN_RETURN:
	case ISN_SHUFFLE:
	case ISN_SLICE:
	case ISN_STORE:
	case ISN_STOREDICT:
	case ISN_STORELIST:
	case ISN_STORENR:
	case ISN_STOREOUTER:
	case ISN_STOREREG:
	case ISN_STORESCRIPT:
	case ISN_STOREV:
	case ISN_STRINDEX:
	case ISN_STRSLICE:
	case ISN_THROW:
	case ISN_TRY:
	    // nothing allocated
	    break;
    }
}

/*
 * Free all instructions for "dfunc".
 */
    static void
delete_def_function_contents(dfunc_T *dfunc)
{
    int idx;

    ga_clear(&dfunc->df_def_args_isn);

    if (dfunc->df_instr != NULL)
    {
	for (idx = 0; idx < dfunc->df_instr_count; ++idx)
	    delete_instr(dfunc->df_instr + idx);
	VIM_CLEAR(dfunc->df_instr);
    }

    dfunc->df_deleted = TRUE;
}

/*
 * When a user function is deleted, clear the contents of any associated def
 * function.  The position in def_functions can be re-used.
 */
    void
clear_def_function(ufunc_T *ufunc)
{
    if (ufunc->uf_dfunc_idx > 0)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;

	delete_def_function_contents(dfunc);
	ufunc->uf_def_status = UF_NOT_COMPILED;
    }
}

/*
 * Used when a user function is about to be deleted: remove the pointer to it.
 * The entry in def_functions is then unused.
 */
    void
unlink_def_function(ufunc_T *ufunc)
{
    dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data) + ufunc->uf_dfunc_idx;

    dfunc->df_ufunc = NULL;
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

	delete_def_function_contents(dfunc);
    }

    ga_clear(&def_functions);
}
#endif


#endif // FEAT_EVAL
