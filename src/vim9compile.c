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
    union {
	ifscope_T	se_if;
	whilescope_T	se_while;
	forscope_T	se_for;
	tryscope_T	se_try;
    };
};

/*
 * Entry for "ctx_locals".  Used for arguments and local variables.
 */
typedef struct {
    char_u	*lv_name;
    type_T	*lv_type;
    int		lv_const;   // when TRUE cannot be assigned to
    int		lv_arg;	    // when TRUE this is an argument
} lvar_T;

/*
 * Context for compiling lines of Vim script.
 * Stores info about the local variables and condition stack.
 */
struct cctx_S {
    ufunc_T	*ctx_ufunc;	    // current function
    int		ctx_lnum;	    // line number in current function
    garray_T	ctx_instr;	    // generated instructions

    garray_T	ctx_locals;	    // currently visible local variables
    int		ctx_max_local;	    // maximum number of locals at one time

    garray_T	ctx_imports;	    // imported items

    scope_T	*ctx_scope;	    // current scope, NULL at toplevel

    garray_T	ctx_type_stack;	    // type of each item on the stack
    garray_T	*ctx_type_list;	    // space for adding types
};

static char e_var_notfound[] = N_("E1001: variable not found: %s");
static char e_syntax_at[] = N_("E1002: Syntax error at %s");

static int compile_expr1(char_u **arg,  cctx_T *cctx);
static int compile_expr2(char_u **arg,  cctx_T *cctx);
static int compile_expr3(char_u **arg,  cctx_T *cctx);

/*
 * Lookup variable "name" in the local scope and return the index.
 */
    static int
lookup_local(char_u *name, size_t len, cctx_T *cctx)
{
    int	    idx;

    if (len <= 0)
	return -1;
    for (idx = 0; idx < cctx->ctx_locals.ga_len; ++idx)
    {
	lvar_T *lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;

	if (STRNCMP(name, lvar->lv_name, len) == 0
					       && STRLEN(lvar->lv_name) == len)
	    return idx;
    }
    return -1;
}

/*
 * Lookup an argument in the current function.
 * Returns the argument index or -1 if not found.
 */
    static int
lookup_arg(char_u *name, size_t len, cctx_T *cctx)
{
    int	    idx;

    if (len <= 0)
	return -1;
    for (idx = 0; idx < cctx->ctx_ufunc->uf_args.ga_len; ++idx)
    {
	char_u *arg = FUNCARG(cctx->ctx_ufunc, idx);

	if (STRNCMP(name, arg, len) == 0 && STRLEN(arg) == len)
	    return idx;
    }
    return -1;
}

/*
 * Lookup a vararg argument in the current function.
 * Returns TRUE if there is a match.
 */
    static int
lookup_vararg(char_u *name, size_t len, cctx_T *cctx)
{
    char_u  *va_name = cctx->ctx_ufunc->uf_va_name;

    return len > 0 && va_name != NULL
		 && STRNCMP(name, va_name, len) == 0 && STRLEN(va_name) == len;
}

/*
 * Lookup a variable in the current script.
 * Returns OK or FAIL.
 */
    static int
lookup_script(char_u *name, size_t len)
{
    int		    cc;
    hashtab_T	    *ht = &SCRIPT_VARS(current_sctx.sc_sid);
    dictitem_T	    *di;

    cc = name[len];
    name[len] = NUL;
    di = find_var_in_ht(ht, 0, name, TRUE);
    name[len] = cc;
    return di == NULL ? FAIL: OK;
}

    static type_T *
get_list_type(type_T *member_type, garray_T *type_list)
{
    type_T *type;

    // recognize commonly used types
    if (member_type->tt_type == VAR_UNKNOWN)
	return &t_list_any;
    if (member_type->tt_type == VAR_NUMBER)
	return &t_list_number;
    if (member_type->tt_type == VAR_STRING)
	return &t_list_string;

    // Not a common type, create a new entry.
    if (ga_grow(type_list, 1) == FAIL)
	return FAIL;
    type = ((type_T *)type_list->ga_data) + type_list->ga_len;
    ++type_list->ga_len;
    type->tt_type = VAR_LIST;
    type->tt_member = member_type;
    return type;
}

    static type_T *
get_dict_type(type_T *member_type, garray_T *type_list)
{
    type_T *type;

    // recognize commonly used types
    if (member_type->tt_type == VAR_UNKNOWN)
	return &t_dict_any;
    if (member_type->tt_type == VAR_NUMBER)
	return &t_dict_number;
    if (member_type->tt_type == VAR_STRING)
	return &t_dict_string;

    // Not a common type, create a new entry.
    if (ga_grow(type_list, 1) == FAIL)
	return FAIL;
    type = ((type_T *)type_list->ga_data) + type_list->ga_len;
    ++type_list->ga_len;
    type->tt_type = VAR_DICT;
    type->tt_member = member_type;
    return type;
}

/////////////////////////////////////////////////////////////////////
// Following generate_ functions expect the caller to call ga_grow().

/*
 * Generate an instruction without arguments.
 * Returns a pointer to the new instruction, NULL if failed.
 */
    static isn_T *
generate_instr(cctx_T *cctx, isntype_T isn_type)
{
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*isn;

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
    ((type_T **)stack->ga_data)[stack->ga_len] = type;
    ++stack->ga_len;

    return isn;
}

/*
 * If type at "offset" isn't already VAR_STRING then generate ISN_2STRING.
 */
    static int
may_generate_2STRING(int offset, cctx_T *cctx)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	**type = ((type_T **)stack->ga_data) + stack->ga_len + offset;

    if ((*type)->tt_type == VAR_STRING)
	return OK;
    *type = &t_string;

    if ((isn = generate_instr(cctx, ISN_2STRING)) == NULL)
	return FAIL;
    isn->isn_arg.number = offset;

    return OK;
}

    static int
check_number_or_float(vartype_T type1, vartype_T type2, char_u *op)
{
    if (!((type1 == VAR_NUMBER || type1 == VAR_FLOAT || type1 == VAR_UNKNOWN)
	    && (type2 == VAR_NUMBER || type2 == VAR_FLOAT
						     || type2 == VAR_UNKNOWN)))
    {
	if (*op == '+')
	    semsg(_("E1035: wrong argument type for +"));
	else
	    semsg(_("E1036: %c requires number or float arguments"), *op);
	return FAIL;
    }
    return OK;
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

    // Get the known type of the two items on the stack.  If they are matching
    // use a type-specific instruction. Otherwise fall back to runtime type
    // checking.
    type1 = ((type_T **)stack->ga_data)[stack->ga_len - 2];
    type2 = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    vartype = VAR_UNKNOWN;
    if (type1->tt_type == type2->tt_type
	    && (type1->tt_type == VAR_NUMBER
		|| type1->tt_type == VAR_LIST
#ifdef FEAT_FLOAT
		|| type1->tt_type == VAR_FLOAT
#endif
		|| type1->tt_type == VAR_BLOB))
	vartype = type1->tt_type;

    switch (*op)
    {
	case '+': if (vartype != VAR_LIST && vartype != VAR_BLOB
			  && check_number_or_float(
				   type1->tt_type, type2->tt_type, op) == FAIL)
		      return FAIL;
		  isn = generate_instr_drop(cctx,
			    vartype == VAR_NUMBER ? ISN_OPNR
			  : vartype == VAR_LIST ? ISN_ADDLIST
			  : vartype == VAR_BLOB ? ISN_ADDBLOB
#ifdef FEAT_FLOAT
			  : vartype == VAR_FLOAT ? ISN_OPFLOAT
#endif
			  : ISN_OPANY, 1);
		  if (isn != NULL)
		      isn->isn_arg.op.op_type = EXPR_ADD;
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

	case '%': if ((type1->tt_type != VAR_UNKNOWN
					       && type1->tt_type != VAR_NUMBER)
			  || (type2->tt_type != VAR_UNKNOWN
					      && type2->tt_type != VAR_NUMBER))
		  {
		      emsg(_("E1035: % requires number arguments"));
		      return FAIL;
		  }
		  isn = generate_instr_drop(cctx,
			      vartype == VAR_NUMBER ? ISN_OPNR : ISN_OPANY, 1);
		  if (isn != NULL)
		      isn->isn_arg.op.op_type = EXPR_REM;
		  break;
    }

    // correct type of result
    if (vartype == VAR_UNKNOWN)
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
 * Generate an ISN_COMPARE* instruction with a boolean result.
 */
    static int
generate_COMPARE(cctx_T *cctx, exptype_T exptype, int ic)
{
    isntype_T	isntype = ISN_DROP;
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    vartype_T	type1;
    vartype_T	type2;

    // Get the known type of the two items on the stack.  If they are matching
    // use a type-specific instruction. Otherwise fall back to runtime type
    // checking.
    type1 = ((type_T **)stack->ga_data)[stack->ga_len - 2]->tt_type;
    type2 = ((type_T **)stack->ga_data)[stack->ga_len - 1]->tt_type;
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
	    case VAR_PARTIAL: isntype = ISN_COMPAREPARTIAL; break;
	    default: isntype = ISN_COMPAREANY; break;
	}
    }
    else if (type1 == VAR_UNKNOWN || type2 == VAR_UNKNOWN
	    || ((type1 == VAR_NUMBER || type1 == VAR_FLOAT)
	      && (type2 == VAR_NUMBER || type2 ==VAR_FLOAT)))
	isntype = ISN_COMPAREANY;

    if ((exptype == EXPR_IS || exptype == EXPR_ISNOT)
	    && (isntype == ISN_COMPAREBOOL
	    || isntype == ISN_COMPARESPECIAL
	    || isntype == ISN_COMPARENR
	    || isntype == ISN_COMPAREFLOAT))
    {
	semsg(_("E1037: Cannot use \"%s\" with %s"),
		exptype == EXPR_IS ? "is" : "isnot" , vartype_name(type1));
	return FAIL;
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
	semsg(_("E1037: Cannot compare %s with %s"),
		vartype_name(type1), vartype_name(type2));
	return FAIL;
    }

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

    if ((isn = generate_instr(cctx, ISN_2BOOL)) == NULL)
	return FAIL;
    isn->isn_arg.number = invert;

    // type becomes bool
    ((type_T **)stack->ga_data)[stack->ga_len - 1] = &t_bool;

    return OK;
}

    static int
generate_TYPECHECK(cctx_T *cctx, type_T *vartype, int offset)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    if ((isn = generate_instr(cctx, ISN_CHECKTYPE)) == NULL)
	return FAIL;
    isn->isn_arg.type.ct_type = vartype->tt_type;  // TODO: whole type
    isn->isn_arg.type.ct_off = offset;

    // type becomes vartype
    ((type_T **)stack->ga_data)[stack->ga_len - 1] = vartype;

    return OK;
}

/*
 * Generate an ISN_PUSHNR instruction.
 */
    static int
generate_PUSHNR(cctx_T *cctx, varnumber_T number)
{
    isn_T	*isn;

    if ((isn = generate_instr_type(cctx, ISN_PUSHNR, &t_number)) == NULL)
	return FAIL;
    isn->isn_arg.number = number;

    return OK;
}

/*
 * Generate an ISN_PUSHBOOL instruction.
 */
    static int
generate_PUSHBOOL(cctx_T *cctx, varnumber_T number)
{
    isn_T	*isn;

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

    if ((isn = generate_instr_type(cctx, ISN_PUSHS, &t_string)) == NULL)
	return FAIL;
    isn->isn_arg.string = str;

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

    if ((isn = generate_instr_type(cctx, ISN_PUSHBLOB, &t_blob)) == NULL)
	return FAIL;
    isn->isn_arg.blob = blob;

    return OK;
}

/*
 * Generate an ISN_STORE instruction.
 */
    static int
generate_STORE(cctx_T *cctx, isntype_T isn_type, int idx, char_u *name)
{
    isn_T	*isn;

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

    if ((isn = generate_instr(cctx, ISN_STORENR)) == NULL)
	return FAIL;
    isn->isn_arg.storenr.str_idx = idx;
    isn->isn_arg.storenr.str_val = value;

    return OK;
}

/*
 * Generate an ISN_STOREOPT instruction
 */
    static int
generate_STOREOPT(cctx_T *cctx, char_u *name, int opt_flags)
{
    isn_T	*isn;

    if ((isn = generate_instr(cctx, ISN_STOREOPT)) == NULL)
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

    if ((isn = generate_instr_type(cctx, isn_type, type)) == NULL)
	return FAIL;
    if (name != NULL)
	isn->isn_arg.string = vim_strsave(name);
    else
	isn->isn_arg.number = idx;

    return OK;
}

/*
 * Generate an ISN_LOADS instruction.
 */
    static int
generate_LOADS(
	cctx_T	    *cctx,
	char_u	    *name,
	int	    sid)
{
    isn_T	*isn;

    if ((isn = generate_instr_type(cctx, ISN_LOADS, &t_any)) == NULL)
	return FAIL;
    isn->isn_arg.loads.ls_name = vim_strsave(name);
    isn->isn_arg.loads.ls_sid = sid;

    return OK;
}

/*
 * Generate an ISN_LOADSCRIPT or ISN_STORESCRIPT instruction.
 */
    static int
generate_SCRIPT(
	cctx_T	    *cctx,
	isntype_T   isn_type,
	int	    sid,
	int	    idx,
	type_T	    *type)
{
    isn_T	*isn;

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
    garray_T	*type_list = cctx->ctx_type_list;
    type_T	*type;
    type_T	*member;

    if ((isn = generate_instr(cctx, ISN_NEWLIST)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;

    // drop the value types
    stack->ga_len -= count;

    // use the first value type for the list member type
    if (count > 0)
	member = ((type_T **)stack->ga_data)[stack->ga_len];
    else
	member = &t_any;
    type = get_list_type(member, type_list);

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
    garray_T	*type_list = cctx->ctx_type_list;
    type_T	*type;
    type_T	*member;

    if ((isn = generate_instr(cctx, ISN_NEWDICT)) == NULL)
	return FAIL;
    isn->isn_arg.number = count;

    // drop the key and value types
    stack->ga_len -= 2 * count;

    // use the first value type for the list member type
    if (count > 0)
	member = ((type_T **)stack->ga_data)[stack->ga_len + 1];
    else
	member = &t_any;
    type = get_dict_type(member, type_list);

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
generate_FUNCREF(cctx_T *cctx, int dfunc_idx)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    if ((isn = generate_instr(cctx, ISN_FUNCREF)) == NULL)
	return FAIL;
    isn->isn_arg.number = dfunc_idx;

    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] = &t_partial_any;
    // TODO: argument and return types
    ++stack->ga_len;

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
 * Return FAIL if the number of arguments is wrong.
 */
    static int
generate_BCALL(cctx_T *cctx, int func_idx, int argcount)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    if (check_internal_func(func_idx, argcount) == FAIL)
	return FAIL;

    if ((isn = generate_instr(cctx, ISN_BCALL)) == NULL)
	return FAIL;
    isn->isn_arg.bfunc.cbf_idx = func_idx;
    isn->isn_arg.bfunc.cbf_argcount = argcount;

    stack->ga_len -= argcount; // drop the arguments
    if (ga_grow(stack, 1) == FAIL)
	return FAIL;
    ((type_T **)stack->ga_data)[stack->ga_len] =
				    internal_func_ret_type(func_idx, argcount);
    ++stack->ga_len;	    // add return value

    return OK;
}

/*
 * Generate an ISN_DCALL or ISN_UCALL instruction.
 * Return FAIL if the number of arguments is wrong.
 */
    static int
generate_CALL(cctx_T *cctx, ufunc_T *ufunc, int argcount)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    int		regular_args = ufunc->uf_args.ga_len;

    if (argcount > regular_args && !has_varargs(ufunc))
    {
	semsg(_(e_toomanyarg), ufunc->uf_name);
	return FAIL;
    }
    if (argcount < regular_args - ufunc->uf_def_args.ga_len)
    {
	semsg(_(e_toofewarg), ufunc->uf_name);
	return FAIL;
    }

    // Turn varargs into a list.
    if (ufunc->uf_va_name != NULL)
    {
	int count = argcount - regular_args;

	// TODO: add default values for optional arguments?
	generate_NEWLIST(cctx, count < 0 ? 0 : count);
	argcount = regular_args + 1;
    }

    if ((isn = generate_instr(cctx,
		    ufunc->uf_dfunc_idx >= 0 ? ISN_DCALL : ISN_UCALL)) == NULL)
	return FAIL;
    if (ufunc->uf_dfunc_idx >= 0)
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

    if ((isn = generate_instr(cctx, ISN_UCALL)) == NULL)
	return FAIL;
    isn->isn_arg.ufunc.cuf_name = vim_strsave(name);
    isn->isn_arg.ufunc.cuf_argcount = argcount;

    stack->ga_len -= argcount; // drop the arguments

    // drop the funcref/partial, get back the return value
    ((type_T **)stack->ga_data)[stack->ga_len - 1] = &t_any;

    return OK;
}

/*
 * Generate an ISN_PCALL instruction.
 */
    static int
generate_PCALL(cctx_T *cctx, int argcount, int at_top)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;

    if ((isn = generate_instr(cctx, ISN_PCALL)) == NULL)
	return FAIL;
    isn->isn_arg.pfunc.cpf_top = at_top;
    isn->isn_arg.pfunc.cpf_argcount = argcount;

    stack->ga_len -= argcount; // drop the arguments

    // drop the funcref/partial, get back the return value
    ((type_T **)stack->ga_data)[stack->ga_len - 1] = &t_any;

    return OK;
}

/*
 * Generate an ISN_MEMBER instruction.
 */
    static int
generate_MEMBER(cctx_T *cctx, char_u *name, size_t len)
{
    isn_T	*isn;
    garray_T	*stack = &cctx->ctx_type_stack;
    type_T	*type;

    if ((isn = generate_instr(cctx, ISN_MEMBER)) == NULL)
	return FAIL;
    isn->isn_arg.string = vim_strnsave(name, (int)len);

    // change dict type to dict member type
    type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
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

    if ((isn = generate_instr_drop(cctx, ISN_ECHO, count)) == NULL)
	return FAIL;
    isn->isn_arg.echo.echo_with_white = with_white;
    isn->isn_arg.echo.echo_count = count;

    return OK;
}

    static int
generate_EXEC(cctx_T *cctx, char_u *line)
{
    isn_T	*isn;

    if ((isn = generate_instr(cctx, ISN_EXEC)) == NULL)
	return FAIL;
    isn->isn_arg.string = vim_strsave(line);
    return OK;
}

static char e_white_both[] =
			N_("E1004: white space required before and after '%s'");

/*
 * Reserve space for a local variable.
 * Return the index or -1 if it failed.
 */
    static int
reserve_local(cctx_T *cctx, char_u *name, size_t len, int isConst, type_T *type)
{
    int	    idx;
    lvar_T  *lvar;

    if (lookup_arg(name, len, cctx) >= 0 || lookup_vararg(name, len, cctx))
    {
	emsg_namelen(_("E1006: %s is used as an argument"), name, (int)len);
	return -1;
    }

    if (ga_grow(&cctx->ctx_locals, 1) == FAIL)
	return -1;
    idx = cctx->ctx_locals.ga_len;
    if (cctx->ctx_max_local < idx + 1)
	cctx->ctx_max_local = idx + 1;
    ++cctx->ctx_locals.ga_len;

    lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;
    lvar->lv_name = vim_strnsave(name, (int)(len == 0 ? STRLEN(name) : len));
    lvar->lv_const = isConst;
    lvar->lv_type = type;

    return idx;
}

/*
 * Skip over a type definition and return a pointer to just after it.
 */
    char_u *
skip_type(char_u *start)
{
    char_u *p = start;

    while (ASCII_ISALNUM(*p) || *p == '_')
	++p;

    // Skip over "<type>"; this is permissive about white space.
    if (*skipwhite(p) == '<')
    {
	p = skipwhite(p);
	p = skip_type(skipwhite(p + 1));
	p = skipwhite(p);
	if (*p == '>')
	    ++p;
    }
    return p;
}

/*
 * Parse the member type: "<type>" and return "type" with the member set.
 * Use "type_list" if a new type needs to be added.
 * Returns NULL in case of failure.
 */
    static type_T *
parse_type_member(char_u **arg, type_T *type, garray_T *type_list)
{
    type_T  *member_type;

    if (**arg != '<')
    {
	if (*skipwhite(*arg) == '<')
	    emsg(_("E1007: No white space allowed before <"));
	else
	    emsg(_("E1008: Missing <type>"));
	return NULL;
    }
    *arg = skipwhite(*arg + 1);

    member_type = parse_type(arg, type_list);
    if (member_type == NULL)
	return NULL;

    *arg = skipwhite(*arg);
    if (**arg != '>')
    {
	emsg(_("E1009: Missing > after type"));
	return NULL;
    }
    ++*arg;

    if (type->tt_type == VAR_LIST)
	return get_list_type(member_type, type_list);
    return get_dict_type(member_type, type_list);
}

/*
 * Parse a type at "arg" and advance over it.
 * Return NULL for failure.
 */
    type_T *
parse_type(char_u **arg, garray_T *type_list)
{
    char_u  *p = *arg;
    size_t  len;

    // skip over the first word
    while (ASCII_ISALNUM(*p) || *p == '_')
	++p;
    len = p - *arg;

    switch (**arg)
    {
	case 'a':
	    if (len == 3 && STRNCMP(*arg, "any", len) == 0)
	    {
		*arg += len;
		return &t_any;
	    }
	    break;
	case 'b':
	    if (len == 4 && STRNCMP(*arg, "bool", len) == 0)
	    {
		*arg += len;
		return &t_bool;
	    }
	    if (len == 4 && STRNCMP(*arg, "blob", len) == 0)
	    {
		*arg += len;
		return &t_blob;
	    }
	    break;
	case 'c':
	    if (len == 7 && STRNCMP(*arg, "channel", len) == 0)
	    {
		*arg += len;
		return &t_channel;
	    }
	    break;
	case 'd':
	    if (len == 4 && STRNCMP(*arg, "dict", len) == 0)
	    {
		*arg += len;
		return parse_type_member(arg, &t_dict_any, type_list);
	    }
	    break;
	case 'f':
	    if (len == 5 && STRNCMP(*arg, "float", len) == 0)
	    {
		*arg += len;
		return &t_float;
	    }
	    if (len == 4 && STRNCMP(*arg, "func", len) == 0)
	    {
		*arg += len;
		// TODO: arguments and return type
		return &t_func_any;
	    }
	    break;
	case 'j':
	    if (len == 3 && STRNCMP(*arg, "job", len) == 0)
	    {
		*arg += len;
		return &t_job;
	    }
	    break;
	case 'l':
	    if (len == 4 && STRNCMP(*arg, "list", len) == 0)
	    {
		*arg += len;
		return parse_type_member(arg, &t_list_any, type_list);
	    }
	    break;
	case 'n':
	    if (len == 6 && STRNCMP(*arg, "number", len) == 0)
	    {
		*arg += len;
		return &t_number;
	    }
	    break;
	case 'p':
	    if (len == 4 && STRNCMP(*arg, "partial", len) == 0)
	    {
		*arg += len;
		// TODO: arguments and return type
		return &t_partial_any;
	    }
	    break;
	case 's':
	    if (len == 6 && STRNCMP(*arg, "string", len) == 0)
	    {
		*arg += len;
		return &t_string;
	    }
	    break;
	case 'v':
	    if (len == 4 && STRNCMP(*arg, "void", len) == 0)
	    {
		*arg += len;
		return &t_void;
	    }
	    break;
    }

    semsg(_("E1010: Type not recognized: %s"), *arg);
    return &t_any;
}

/*
 * Check if "type1" and "type2" are exactly the same.
 */
    static int
equal_type(type_T *type1, type_T *type2)
{
    if (type1->tt_type != type2->tt_type)
	return FALSE;
    switch (type1->tt_type)
    {
	case VAR_VOID:
	case VAR_UNKNOWN:
	case VAR_SPECIAL:
	case VAR_BOOL:
	case VAR_NUMBER:
	case VAR_FLOAT:
	case VAR_STRING:
	case VAR_BLOB:
	case VAR_JOB:
	case VAR_CHANNEL:
	    return TRUE;  // not composite is always OK
	case VAR_LIST:
	case VAR_DICT:
	    return equal_type(type1->tt_member, type2->tt_member);
	case VAR_FUNC:
	case VAR_PARTIAL:
	    // TODO; check argument types.
	    return equal_type(type1->tt_member, type2->tt_member)
		&& type1->tt_argcount == type2->tt_argcount;
    }
    return TRUE;
}

/*
 * Find the common type of "type1" and "type2" and put it in "dest".
 * "type2" and "dest" may be the same.
 */
    static void
common_type(type_T *type1, type_T *type2, type_T *dest)
{
    if (equal_type(type1, type2))
    {
	if (dest != type2)
	    *dest = *type2;
	return;
    }

    if (type1->tt_type == type2->tt_type)
    {
	dest->tt_type = type1->tt_type;
	if (type1->tt_type == VAR_LIST || type2->tt_type == VAR_DICT)
	{
	    common_type(type1->tt_member, type2->tt_member, dest->tt_member);
	    return;
	}
	// TODO: VAR_FUNC and VAR_PARTIAL
    }

    dest->tt_type = VAR_UNKNOWN;  // "any"
}

    char *
vartype_name(vartype_T type)
{
    switch (type)
    {
	case VAR_VOID: return "void";
	case VAR_UNKNOWN: return "any";
	case VAR_SPECIAL: return "special";
	case VAR_BOOL: return "bool";
	case VAR_NUMBER: return "number";
	case VAR_FLOAT: return "float";
	case VAR_STRING: return "string";
	case VAR_BLOB: return "blob";
	case VAR_JOB: return "job";
	case VAR_CHANNEL: return "channel";
	case VAR_LIST: return "list";
	case VAR_DICT: return "dict";
	case VAR_FUNC: return "function";
	case VAR_PARTIAL: return "partial";
    }
    return "???";
}

/*
 * Return the name of a type.
 * The result may be in allocated memory, in which case "tofree" is set.
 */
    char *
type_name(type_T *type, char **tofree)
{
    char *name = vartype_name(type->tt_type);

    *tofree = NULL;
    if (type->tt_type == VAR_LIST || type->tt_type == VAR_DICT)
    {
	char *member_free;
	char *member_name = type_name(type->tt_member, &member_free);
	size_t len;

	len = STRLEN(name) + STRLEN(member_name) + 3;
	*tofree = alloc(len);
	if (*tofree != NULL)
	{
	    vim_snprintf(*tofree, len, "%s<%s>", name, member_name);
	    vim_free(member_free);
	    return *tofree;
	}
    }
    // TODO: function and partial argument types

    return name;
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
    scriptitem_T    *si = &SCRIPT_ITEM(sid);
    int		    idx;

    // First look the name up in the hashtable.
    if (sid <= 0 || sid > script_items.ga_len)
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
 * Find "name" in imported items of the current script/
 */
    imported_T *
find_imported(char_u *name, cctx_T *cctx)
{
    scriptitem_T    *si = &SCRIPT_ITEM(current_sctx.sc_sid);
    int		    idx;

    if (cctx != NULL)
	for (idx = 0; idx < cctx->ctx_imports.ga_len; ++idx)
	{
	    imported_T *import = ((imported_T *)cctx->ctx_imports.ga_data)
									 + idx;

	    if (STRCMP(name, import->imp_name) == 0)
		return import;
	}

    for (idx = 0; idx < si->sn_imports.ga_len; ++idx)
    {
	imported_T *import = ((imported_T *)si->sn_imports.ga_data) + idx;

	if (STRCMP(name, import->imp_name) == 0)
	    return import;
    }
    return NULL;
}

/*
 * Generate an instruction to load script-local variable "name".
 */
    static int
compile_load_scriptvar(cctx_T *cctx, char_u *name)
{
    scriptitem_T    *si = &SCRIPT_ITEM(current_sctx.sc_sid);
    int		    idx = get_script_item_idx(current_sctx.sc_sid, name, FALSE);
    imported_T	    *import;

    if (idx == -1)
    {
	// variable exists but is not in sn_var_vals: old style script.
	return generate_LOADS(cctx, name, current_sctx.sc_sid);
    }
    if (idx >= 0)
    {
	svar_T		*sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;

	generate_SCRIPT(cctx, ISN_LOADSCRIPT,
					current_sctx.sc_sid, idx, sv->sv_type);
	return OK;
    }

    import = find_imported(name, cctx);
    if (import != NULL)
    {
	// TODO: check this is a variable, not a function
	generate_SCRIPT(cctx, ISN_LOADSCRIPT,
		import->imp_sid,
		import->imp_var_vals_idx,
		import->imp_type);
	return OK;
    }

    semsg(_("E1050: Item not found: %s"), name);
    return FAIL;
}

/*
 * Compile a variable name into a load instruction.
 * "end" points to just after the name.
 * When "error" is FALSE do not give an error when not found.
 */
    static int
compile_load(char_u **arg, char_u *end, cctx_T *cctx, int error)
{
    type_T	*type;
    char_u	*name;
    int		res = FAIL;

    if (*(*arg + 1) == ':')
    {
	// load namespaced variable
	name = vim_strnsave(*arg + 2, end - (*arg + 2));
	if (name == NULL)
	    return FAIL;

	if (**arg == 'v')
	{
	    // load v:var
	    int vidx = find_vim_var(name);

	    if (vidx < 0)
	    {
		if (error)
		    semsg(_(e_var_notfound), name);
		goto theend;
	    }

	    // TODO: get actual type
	    res = generate_LOAD(cctx, ISN_LOADV, vidx, NULL, &t_any);
	}
	else if (**arg == 'g')
	{
	    // Global variables can be defined later, thus we don't check if it
	    // exists, give error at runtime.
	    res = generate_LOAD(cctx, ISN_LOADG, 0, name, &t_any);
	}
	else if (**arg == 's')
	{
	    res = compile_load_scriptvar(cctx, name);
	}
	else
	{
	    semsg("Namespace not supported yet: %s", **arg);
	    goto theend;
	}
    }
    else
    {
	size_t	    len = end - *arg;
	int	    idx;
	int	    gen_load = FALSE;

	name = vim_strnsave(*arg, end - *arg);
	if (name == NULL)
	    return FAIL;

	idx = lookup_arg(*arg, len, cctx);
	if (idx >= 0)
	{
	    if (cctx->ctx_ufunc->uf_arg_types != NULL)
		type = cctx->ctx_ufunc->uf_arg_types[idx];
	    else
		type = &t_any;

	    // Arguments are located above the frame pointer.
	    idx -= cctx->ctx_ufunc->uf_args.ga_len + STACK_FRAME_SIZE;
	    if (cctx->ctx_ufunc->uf_va_name != NULL)
		--idx;
	    gen_load = TRUE;
	}
	else if (lookup_vararg(*arg, len, cctx))
	{
	    // varargs is always the last argument
	    idx = -STACK_FRAME_SIZE - 1;
	    type = cctx->ctx_ufunc->uf_va_type;
	    gen_load = TRUE;
	}
	else
	{
	    idx = lookup_local(*arg, len, cctx);
	    if (idx >= 0)
	    {
		type = (((lvar_T *)cctx->ctx_locals.ga_data) + idx)->lv_type;
		gen_load = TRUE;
	    }
	    else
	    {
		if ((len == 4 && STRNCMP("true", *arg, 4) == 0)
			|| (len == 5 && STRNCMP("false", *arg, 5) == 0))
		    res = generate_PUSHBOOL(cctx, **arg == 't'
						     ? VVAL_TRUE : VVAL_FALSE);
		else
		   res = compile_load_scriptvar(cctx, name);
	    }
	}
	if (gen_load)
	    res = generate_LOAD(cctx, ISN_LOAD, idx, NULL, type);
    }

    *arg = end;

theend:
    if (res == FAIL && error)
	semsg(_(e_var_notfound), name);
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
    char_u *p = *arg;

    while (*p != NUL && *p != ')')
    {
	if (compile_expr1(&p, cctx) == FAIL)
	    return FAIL;
	++*argcount;
	if (*p == ',')
	    p = skipwhite(p + 1);
    }
    if (*p != ')')
    {
	emsg(_(e_missing_close));
	return FAIL;
    }
    *arg = p + 1;
    return OK;
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
compile_call(char_u **arg, size_t varlen, cctx_T *cctx, int argcount_init)
{
    char_u	*name = *arg;
    char_u	*p = *arg + varlen + 1;
    int		argcount = argcount_init;
    char_u	namebuf[100];
    ufunc_T	*ufunc;

    if (varlen >= sizeof(namebuf))
    {
	semsg(_("E1011: name too long: %s"), name);
	return FAIL;
    }
    vim_strncpy(namebuf, name, varlen);

    *arg = skipwhite(*arg + varlen + 1);
    if (compile_arguments(arg, cctx, &argcount) == FAIL)
	return FAIL;

    if (ASCII_ISLOWER(*name))
    {
	int	    idx;

	// builtin function
	idx = find_internal_func(namebuf);
	if (idx >= 0)
	    return generate_BCALL(cctx, idx, argcount);
	semsg(_(e_unknownfunc), namebuf);
    }

    // User defined function or variable must start with upper case.
    if (!ASCII_ISUPPER(*name))
    {
	semsg(_("E1012: Invalid function name: %s"), namebuf);
	return FAIL;
    }

    // If we can find the function by name generate the right call.
    ufunc = find_func(namebuf, cctx);
    if (ufunc != NULL)
	return generate_CALL(cctx, ufunc, argcount);

    // If the name is a variable, load it and use PCALL.
    p = namebuf;
    if (compile_load(&p, namebuf + varlen, cctx, FALSE) == OK)
	return generate_PCALL(cctx, argcount, FALSE);

    // The function may be defined only later.  Need to figure out at runtime.
    return generate_UCALL(cctx, namebuf, argcount);
}

// like NAMESPACE_CHAR but with 'a' and 'l'.
#define VIM9_NAMESPACE_CHAR	(char_u *)"bgstvw"

/*
 * Find the end of a variable or function name.  Unlike find_name_end() this
 * does not recognize magic braces.
 * Return a pointer to just after the name.  Equal to "arg" if there is no
 * valid name.
 */
    char_u *
to_name_end(char_u *arg)
{
    char_u	*p;

    // Quick check for valid starting character.
    if (!eval_isnamec1(*arg))
	return arg;

    for (p = arg + 1; *p != NUL && eval_isnamec(*p); MB_PTR_ADV(p))
	// Include a namespace such as "s:var" and "v:var".  But "n:" is not
	// and can be used in slice "[n:]".
	if (*p == ':' && (p != arg + 1
			     || vim_strchr(VIM9_NAMESPACE_CHAR, *arg) == NULL))
	    break;
    return p;
}

/*
 * Like to_name_end() but also skip over a list or dict constant.
 */
    char_u *
to_name_const_end(char_u *arg)
{
    char_u	*p = to_name_end(arg);
    typval_T	rettv;

    if (p == arg && *arg == '[')
    {

	// Can be "[1, 2, 3]->Func()".
	if (get_list_tv(&p, &rettv, FALSE, FALSE) == FAIL)
	    p = arg;
    }
    else if (p == arg && *arg == '#' && arg[1] == '{')
    {
	++p;
	if (eval_dict(&p, &rettv, FALSE, TRUE) == FAIL)
	    p = arg;
    }
    else if (p == arg && *arg == '{')
    {
	int	    ret = get_lambda_tv(&p, &rettv, FALSE);

	if (ret == NOTDONE)
	    ret = eval_dict(&p, &rettv, FALSE, FALSE);
	if (ret != OK)
	    p = arg;
    }

    return p;
}

    static void
type_mismatch(type_T *expected, type_T *actual)
{
    char *tofree1, *tofree2;

    semsg(_("E1013: type mismatch, expected %s but got %s"),
		   type_name(expected, &tofree1), type_name(actual, &tofree2));
    vim_free(tofree1);
    vim_free(tofree2);
}

/*
 * Check if the expected and actual types match.
 */
    static int
check_type(type_T *expected, type_T *actual, int give_msg)
{
    if (expected->tt_type != VAR_UNKNOWN)
    {
	if (expected->tt_type != actual->tt_type)
	{
	    if (give_msg)
		type_mismatch(expected, actual);
	    return FAIL;
	}
	if (expected->tt_type == VAR_DICT || expected->tt_type == VAR_LIST)
	{
	    int ret = check_type(expected->tt_member, actual->tt_member,
									FALSE);
	    if (ret == FAIL && give_msg)
		type_mismatch(expected, actual);
	    return ret;
	}
    }
    return OK;
}

/*
 * Check that
 * - "actual" is "expected" type or
 * - "actual" is a type that can be "expected" type: add a runtime check; or
 * - return FAIL.
 */
    static int
need_type(type_T *actual, type_T *expected, int offset, cctx_T *cctx)
{
    if (equal_type(actual, expected) || expected->tt_type == VAR_UNKNOWN)
	return OK;
    if (actual->tt_type != VAR_UNKNOWN)
    {
	type_mismatch(expected, actual);
	return FAIL;
    }
    generate_TYPECHECK(cctx, expected, offset);
    return OK;
}

/*
 * parse a list: [expr, expr]
 * "*arg" points to the '['.
 */
    static int
compile_list(char_u **arg, cctx_T *cctx)
{
    char_u	*p = skipwhite(*arg + 1);
    int		count = 0;

    while (*p != ']')
    {
	if (*p == NUL)
	    return FAIL;
	if (compile_expr1(&p, cctx) == FAIL)
	    break;
	++count;
	if (*p == ',')
	    ++p;
	p = skipwhite(p);
    }
    *arg = p + 1;

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
    garray_T	*instr = &cctx->ctx_instr;
    typval_T	rettv;
    ufunc_T	*ufunc;

    // Get the funcref in "rettv".
    if (get_lambda_tv(arg, &rettv, TRUE) == FAIL)
	return FAIL;
    ufunc = rettv.vval.v_partial->pt_func;

    // The function will have one line: "return {expr}".
    // Compile it into instructions.
    compile_def_function(ufunc, TRUE);

    if (ufunc->uf_dfunc_idx >= 0)
    {
	if (ga_grow(instr, 1) == FAIL)
	    return FAIL;
	generate_FUNCREF(cctx, ufunc->uf_dfunc_idx);
	return OK;
    }
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
    if (get_lambda_tv(arg, &rettv, TRUE) == FAIL)
	return FAIL;

    if (**arg != '(')
    {
	if (*skipwhite(*arg) == '(')
	    semsg(_(e_nowhitespace));
	else
	    semsg(_(e_missing_paren), "lambda");
	clear_tv(&rettv);
	return FAIL;
    }

    // The function will have one line: "return {expr}".
    // Compile it into instructions.
    ufunc = rettv.vval.v_partial->pt_func;
    ++ufunc->uf_refcount;
    compile_def_function(ufunc, TRUE);

    // compile the arguments
    *arg = skipwhite(*arg + 1);
    if (compile_arguments(arg, cctx, &argcount) == OK)
	// call the compiled function
	ret = generate_CALL(cctx, ufunc, argcount);

    clear_tv(&rettv);
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
    int		count = 0;
    dict_T	*d = dict_alloc();
    dictitem_T	*item;

    if (d == NULL)
	return FAIL;
    *arg = skipwhite(*arg + 1);
    while (**arg != '}' && **arg != NUL)
    {
	char_u *key = NULL;

	if (literal)
	{
	    char_u *p = to_name_end(*arg);

	    if (p == *arg)
	    {
		semsg(_("E1014: Invalid key: %s"), *arg);
		return FAIL;
	    }
	    key = vim_strnsave(*arg, p - *arg);
	    if (generate_PUSHS(cctx, key) == FAIL)
		return FAIL;
	    *arg = p;
	}
	else
	{
	    isn_T		*isn;

	    if (compile_expr1(arg, cctx) == FAIL)
		return FAIL;
	    // TODO: check type is string
	    isn = ((isn_T *)instr->ga_data) + instr->ga_len - 1;
	    if (isn->isn_type == ISN_PUSHS)
		key = isn->isn_arg.string;
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

	*arg = skipwhite(*arg);
	if (**arg != ':')
	{
	    semsg(_(e_missing_dict_colon), *arg);
	    return FAIL;
	}

	*arg = skipwhite(*arg + 1);
	if (compile_expr1(arg, cctx) == FAIL)
	    return FAIL;
	++count;

	if (**arg == '}')
	    break;
	if (**arg != ',')
	{
	    semsg(_(e_missing_dict_comma), *arg);
	    goto failret;
	}
	*arg = skipwhite(*arg + 1);
    }

    if (**arg != '}')
    {
	semsg(_(e_missing_dict_end), *arg);
	goto failret;
    }
    *arg = *arg + 1;

    dict_unref(d);
    return generate_NEWDICT(cctx, count);

failret:
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
    ret = get_option_tv(arg, &rettv, TRUE);
    if (ret == OK)
    {
	// include the '&' in the name, get_option_tv() expects it.
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

    start = *arg;
    ++*arg;
    len = get_env_len(arg);
    if (len == 0)
    {
	semsg(_(e_syntax_at), start - 1);
	return FAIL;
    }

    // include the '$' in the name, get_env_tv() expects it.
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
	semsg(_(e_syntax_at), *arg - 1);
	return FAIL;
    }
    if (!valid_yank_reg(**arg, TRUE))
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
 */
    static int
apply_leader(typval_T *rettv, char_u *start, char_u *end)
{
    char_u *p = end;

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
	else
	{
	    int v = tv2bool(rettv);

	    // '!' is permissive in the type.
	    clear_tv(rettv);
	    rettv->v_type = VAR_BOOL;
	    rettv->vval.v_number = v ? VVAL_FALSE : VVAL_TRUE;
	}
    }
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

/*
 * Compile code to apply '-', '+' and '!'.
 */
    static int
compile_leader(cctx_T *cctx, char_u *start, char_u *end)
{
    char_u	*p = end;

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
    return OK;
}

/*
 * Compile whatever comes after "name" or "name()".
 */
    static int
compile_subscript(
	char_u **arg,
	cctx_T *cctx,
	char_u **start_leader,
	char_u *end_leader)
{
    for (;;)
    {
	if (**arg == '(')
	{
	    int	    argcount = 0;

	    // funcref(arg)
	    *arg = skipwhite(*arg + 1);
	    if (compile_arguments(arg, cctx, &argcount) == FAIL)
		return FAIL;
	    if (generate_PCALL(cctx, argcount, TRUE) == FAIL)
		return FAIL;
	}
	else if (**arg == '-' && (*arg)[1] == '>')
	{
	    char_u *p;

	    // something->method()
	    // Apply the '!', '-' and '+' first:
	    //   -1.0->func() works like (-1.0)->func()
	    if (compile_leader(cctx, *start_leader, end_leader) == FAIL)
		return FAIL;
	    *start_leader = end_leader;   // don't apply again later

	    *arg = skipwhite(*arg + 2);
	    if (**arg == '{')
	    {
		// lambda call:  list->{lambda}
		if (compile_lambda_call(arg, cctx) == FAIL)
		    return FAIL;
	    }
	    else
	    {
		// method call:  list->method()
		for (p = *arg; eval_isnamec1(*p); ++p)
		    ;
		if (*p != '(')
		{
		    semsg(_(e_missing_paren), arg);
		    return FAIL;
		}
		// TODO: base value may not be the first argument
		if (compile_call(arg, p - *arg, cctx, 1) == FAIL)
		    return FAIL;
	    }
	}
	else if (**arg == '[')
	{
	    // list index: list[123]
	    // TODO: more arguments
	    // TODO: dict member  dict['name']
	    *arg = skipwhite(*arg + 1);
	    if (compile_expr1(arg, cctx) == FAIL)
		return FAIL;

	    if (**arg != ']')
	    {
		emsg(_(e_missbrac));
		return FAIL;
	    }
	    *arg = skipwhite(*arg + 1);

	    if (generate_instr_drop(cctx, ISN_INDEX, 1) == FAIL)
		return FAIL;
	}
	else if (**arg == '.' && (*arg)[1] != '.')
	{
	    char_u *p;

	    ++*arg;
	    p = *arg;
	    // dictionary member: dict.name
	    if (eval_isnamec1(*p))
		while (eval_isnamec(*p))
		    MB_PTR_ADV(p);
	    if (p == *arg)
	    {
		semsg(_(e_syntax_at), *arg);
		return FAIL;
	    }
	    // TODO: check type is dict
	    if (generate_MEMBER(cctx, *arg, p - *arg) == FAIL)
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
 * Compile an expression at "*p" and add instructions to "instr".
 * "p" is advanced until after the expression, skipping white space.
 *
 * This is the equivalent of eval1(), eval2(), etc.
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
compile_expr7(char_u **arg, cctx_T *cctx)
{
    typval_T	rettv;
    char_u	*start_leader, *end_leader;
    int		ret = OK;

    /*
     * Skip '!', '-' and '+' characters.  They are handled later.
     */
    start_leader = *arg;
    while (**arg == '!' || **arg == '-' || **arg == '+')
	*arg = skipwhite(*arg + 1);
    end_leader = *arg;

    rettv.v_type = VAR_UNKNOWN;
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
	case '.':   if (get_number_tv(arg, &rettv, TRUE, FALSE) == FAIL)
			return FAIL;
		    break;

	/*
	 * String constant: "string".
	 */
	case '"':   if (get_string_tv(arg, &rettv, TRUE) == FAIL)
			return FAIL;
		    break;

	/*
	 * Literal string constant: 'str''ing'.
	 */
	case '\'':  if (get_lit_string_tv(arg, &rettv, TRUE) == FAIL)
			return FAIL;
		    break;

	/*
	 * Constant Vim variable.
	 */
	case 'v':   get_vim_constant(arg, &rettv);
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
						       NULL, NULL, NULL, TRUE);
			if (ret != FAIL && *start == '>')
			    ret = compile_lambda(arg, cctx);
			else
			    ret = compile_dict(arg, cctx, FALSE);
		    }
		    break;

	/*
	 * Option value: &name
	 */
	case '&':	ret = compile_get_option(arg, cctx);
			break;

	/*
	 * Environment variable: $VAR.
	 */
	case '$':	ret = compile_get_env(arg, cctx);
			break;

	/*
	 * Register contents: @r.
	 */
	case '@':	ret = compile_get_register(arg, cctx);
			break;
	/*
	 * nested expression: (expression).
	 */
	case '(':   *arg = skipwhite(*arg + 1);
		    ret = compile_expr1(arg, cctx);	// recursive!
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

    if (rettv.v_type != VAR_UNKNOWN)
    {
	// apply the '!', '-' and '+' before the constant
	if (apply_leader(&rettv, start_leader, end_leader) == FAIL)
	{
	    clear_tv(&rettv);
	    return FAIL;
	}
	start_leader = end_leader;   // don't apply again below

	// push constant
	switch (rettv.v_type)
	{
	    case VAR_BOOL:
		generate_PUSHBOOL(cctx, rettv.vval.v_number);
		break;
	    case VAR_SPECIAL:
		generate_PUSHSPEC(cctx, rettv.vval.v_number);
		break;
	    case VAR_NUMBER:
		generate_PUSHNR(cctx, rettv.vval.v_number);
		break;
#ifdef FEAT_FLOAT
	    case VAR_FLOAT:
		generate_PUSHF(cctx, rettv.vval.v_float);
		break;
#endif
	    case VAR_BLOB:
		generate_PUSHBLOB(cctx, rettv.vval.v_blob);
		rettv.vval.v_blob = NULL;
		break;
	    case VAR_STRING:
		generate_PUSHS(cctx, rettv.vval.v_string);
		rettv.vval.v_string = NULL;
		break;
	    default:
		iemsg("constant type missing");
		return FAIL;
	}
    }
    else if (ret == NOTDONE)
    {
	char_u	    *p;
	int	    r;

	if (!eval_isnamec1(**arg))
	{
	    semsg(_("E1015: Name expected: %s"), *arg);
	    return FAIL;
	}

	// "name" or "name()"
	p = to_name_end(*arg);
	if (*p == '(')
	    r = compile_call(arg, p - *arg, cctx, 0);
	else
	    r = compile_load(arg, p, cctx, TRUE);
	if (r == FAIL)
	    return FAIL;
    }

    if (compile_subscript(arg, cctx, &start_leader, end_leader) == FAIL)
	return FAIL;

    // Now deal with prefixed '-', '+' and '!', if not done already.
    return compile_leader(cctx, start_leader, end_leader);
}

/*
 *	*	number multiplication
 *	/	number division
 *	%	number modulo
 */
    static int
compile_expr6(char_u **arg, cctx_T *cctx)
{
    char_u	*op;

    // get the first variable
    if (compile_expr7(arg, cctx) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no "*", "/" or "%" is following.
     */
    for (;;)
    {
	op = skipwhite(*arg);
	if (*op != '*' && *op != '/' && *op != '%')
	    break;
	if (!VIM_ISWHITE(**arg) || !VIM_ISWHITE(op[1]))
	{
	    char_u buf[3];

	    vim_strncpy(buf, op, 1);
	    semsg(_(e_white_both), buf);
	}
	*arg = skipwhite(op + 1);

	// get the second variable
	if (compile_expr7(arg, cctx) == FAIL)
	    return FAIL;

	generate_two_op(cctx, op);
    }

    return OK;
}

/*
 *      +	number addition
 *      -	number subtraction
 *      ..	string concatenation
 */
    static int
compile_expr5(char_u **arg, cctx_T *cctx)
{
    char_u	*op;
    int		oplen;

    // get the first variable
    if (compile_expr6(arg, cctx) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no "+", "-" or ".." is following.
     */
    for (;;)
    {
	op = skipwhite(*arg);
	if (*op != '+' && *op != '-' && !(*op == '.' && (*(*arg + 1) == '.')))
	    break;
	oplen = (*op == '.' ? 2 : 1);

	if (!VIM_ISWHITE(**arg) || !VIM_ISWHITE(op[oplen]))
	{
	    char_u buf[3];

	    vim_strncpy(buf, op, oplen);
	    semsg(_(e_white_both), buf);
	}

	*arg = skipwhite(op + oplen);

	// get the second variable
	if (compile_expr6(arg, cctx) == FAIL)
	    return FAIL;

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
compile_expr4(char_u **arg, cctx_T *cctx)
{
    exptype_T	type = EXPR_UNKNOWN;
    char_u	*p;
    int		len = 2;
    int		i;
    int		type_is = FALSE;

    // get the first variable
    if (compile_expr5(arg, cctx) == FAIL)
	return FAIL;

    p = skipwhite(*arg);
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
			len = 1;
		    }
		    else
			type = EXPR_GEQUAL;
		    break;
	case '<':   if (p[1] != '=')
		    {
			type = EXPR_SMALLER;
			len = 1;
		    }
		    else
			type = EXPR_SEQUAL;
		    break;
	case 'i':   if (p[1] == 's')
		    {
			// "is" and "isnot"; but not a prefix of a name
			if (p[2] == 'n' && p[3] == 'o' && p[4] == 't')
			    len = 5;
			i = p[len];
			if (!isalnum(i) && i != '_')
			{
			    type = len == 2 ? EXPR_IS : EXPR_ISNOT;
			    type_is = TRUE;
			}
		    }
		    break;
    }

    /*
     * If there is a comparative operator, use it.
     */
    if (type != EXPR_UNKNOWN)
    {
	int ic = FALSE;  // Default: do not ignore case

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

	if (!VIM_ISWHITE(**arg) || !VIM_ISWHITE(p[len]))
	{
	    char_u buf[7];

	    vim_strncpy(buf, p, len);
	    semsg(_(e_white_both), buf);
	}

	// get the second variable
	*arg = skipwhite(p + len);
	if (compile_expr5(arg, cctx) == FAIL)
	    return FAIL;

	generate_COMPARE(cctx, type, ic);
    }

    return OK;
}

/*
 * Compile || or &&.
 */
    static int
compile_and_or(char_u **arg, cctx_T *cctx, char *op)
{
    char_u	*p = skipwhite(*arg);
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
	    if (!VIM_ISWHITE(**arg) || !VIM_ISWHITE(p[2]))
		semsg(_(e_white_both), op);

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
	    if ((opchar == '|' ? compile_expr3(arg, cctx)
					   : compile_expr4(arg, cctx)) == FAIL)
	    {
		ga_clear(&end_ga);
		return FAIL;
	    }
	    p = skipwhite(*arg);
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
 *	JUMP_AND_KEEP_IF_FALSE end
 *	EVAL expr4b		Push result of "expr4b"
 *	JUMP_AND_KEEP_IF_FALSE end
 *	EVAL expr4c		Push result of "expr4c"
 * end:
 */
    static int
compile_expr3(char_u **arg, cctx_T *cctx)
{
    // get the first variable
    if (compile_expr4(arg, cctx) == FAIL)
	return FAIL;

    // || and && work almost the same
    return compile_and_or(arg, cctx, "&&");
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
compile_expr2(char_u **arg, cctx_T *cctx)
{
    // eval the first expression
    if (compile_expr3(arg, cctx) == FAIL)
	return FAIL;

    // || and && work almost the same
    return compile_and_or(arg, cctx, "||");
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
compile_expr1(char_u **arg,  cctx_T *cctx)
{
    char_u	*p;

    // evaluate the first expression
    if (compile_expr2(arg, cctx) == FAIL)
	return FAIL;

    p = skipwhite(*arg);
    if (*p == '?')
    {
	garray_T	*instr = &cctx->ctx_instr;
	garray_T	*stack = &cctx->ctx_type_stack;
	int		alt_idx = instr->ga_len;
	int		end_idx;
	isn_T		*isn;
	type_T		*type1;
	type_T		*type2;

	if (!VIM_ISWHITE(**arg) || !VIM_ISWHITE(p[1]))
	    semsg(_(e_white_both), "?");

	generate_JUMP(cctx, JUMP_IF_FALSE, 0);

	// evaluate the second expression; any type is accepted
	*arg = skipwhite(p + 1);
	compile_expr1(arg, cctx);

	// remember the type and drop it
	--stack->ga_len;
	type1 = ((type_T **)stack->ga_data)[stack->ga_len];

	end_idx = instr->ga_len;
	generate_JUMP(cctx, JUMP_ALWAYS, 0);

	// jump here from JUMP_IF_FALSE
	isn = ((isn_T *)instr->ga_data) + alt_idx;
	isn->isn_arg.jump.jump_where = instr->ga_len;

	// Check for the ":".
	p = skipwhite(*arg);
	if (*p != ':')
	{
	    emsg(_(e_missing_colon));
	    return FAIL;
	}
	if (!VIM_ISWHITE(**arg) || !VIM_ISWHITE(p[1]))
	    semsg(_(e_white_both), ":");

	// evaluate the third expression
	*arg = skipwhite(p + 1);
	compile_expr1(arg, cctx);

	// If the types differ, the result has a more generic type.
	type2 = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	common_type(type1, type2, type2);

	// jump here from JUMP_ALWAYS
	isn = ((isn_T *)instr->ga_data) + end_idx;
	isn->isn_arg.jump.jump_where = instr->ga_len;
    }
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
	if (compile_expr1(&p, cctx) == FAIL)
	    return NULL;

	stack_type = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	if (set_return_type)
	    cctx->ctx_ufunc->uf_ret_type = stack_type;
	else if (need_type(stack_type, cctx->ctx_ufunc->uf_ret_type, -1, cctx)
								       == FAIL)
	    return NULL;
    }
    else
    {
	if (set_return_type)
	    cctx->ctx_ufunc->uf_ret_type = &t_void;
	else if (cctx->ctx_ufunc->uf_ret_type->tt_type != VAR_VOID)
	{
	    emsg(_("E1003: Missing return value"));
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

/*
 * Get a line for "=<<".
 * Return a pointer to the line in allocated memory.
 * Return NULL for end-of-file or some error.
 */
    static char_u *
heredoc_getline(
	int c UNUSED,
	void *cookie,
	int indent UNUSED,
	int do_concat UNUSED)
{
    cctx_T  *cctx = (cctx_T *)cookie;

    if (cctx->ctx_lnum == cctx->ctx_ufunc->uf_lines.ga_len)
	NULL;
    ++cctx->ctx_lnum;
    return vim_strsave(((char_u **)cctx->ctx_ufunc->uf_lines.ga_data)
							     [cctx->ctx_lnum]);
}

/*
 * compile "let var [= expr]", "const var = expr" and "var = expr"
 * "arg" points to "var".
 */
    static char_u *
compile_assignment(char_u *arg, exarg_T *eap, cmdidx_T cmdidx, cctx_T *cctx)
{
    char_u	*p;
    char_u	*ret = NULL;
    int		var_count = 0;
    int		semicolon = 0;
    size_t	varlen;
    garray_T	*instr = &cctx->ctx_instr;
    int		idx = -1;
    char_u	*op;
    int		option = FALSE;
    int		opt_type;
    int		opt_flags = 0;
    int		global = FALSE;
    int		script = FALSE;
    int		oplen = 0;
    int		heredoc = FALSE;
    type_T	*type;
    lvar_T	*lvar;
    char_u	*name;
    char_u	*sp;
    int		has_type = FALSE;
    int		is_decl = cmdidx == CMD_let || cmdidx == CMD_const;
    int		instr_count = -1;

    p = skip_var_list(arg, FALSE, &var_count, &semicolon);
    if (p == NULL)
	return NULL;
    if (var_count > 0)
    {
	// TODO: let [var, var] = list
	emsg("Cannot handle a list yet");
	return NULL;
    }

    varlen = p - arg;
    name = vim_strnsave(arg, (int)varlen);
    if (name == NULL)
	return NULL;

    if (*arg == '&')
    {
	int	    cc;
	long	    numval;
	char_u	    *stringval = NULL;

	option = TRUE;
	if (cmdidx == CMD_const)
	{
	    emsg(_(e_const_option));
	    return NULL;
	}
	if (is_decl)
	{
	    semsg(_("E1052: Cannot declare an option: %s"), arg);
	    goto theend;
	}
	p = arg;
	p = find_option_end(&p, &opt_flags);
	if (p == NULL)
	{
	    emsg(_(e_letunexp));
	    return NULL;
	}
	cc = *p;
	*p = NUL;
	opt_type = get_option_value(arg + 1, &numval, &stringval, opt_flags);
	*p = cc;
	if (opt_type == -3)
	{
	    semsg(_(e_unknown_option), *arg);
	    return NULL;
	}
	if (opt_type == -2 || opt_type == 0)
	    type = &t_string;
	else
	    type = &t_number;	// both number and boolean option
    }
    else if (STRNCMP(arg, "g:", 2) == 0)
    {
	global = TRUE;
	if (is_decl)
	{
	    semsg(_("E1016: Cannot declare a global variable: %s"), name);
	    goto theend;
	}
    }
    else
    {
	for (idx = 0; reserved[idx] != NULL; ++idx)
	    if (STRCMP(reserved[idx], name) == 0)
	    {
		semsg(_("E1034: Cannot use reserved name %s"), name);
		goto theend;
	    }

	idx = lookup_local(arg, varlen, cctx);
	if (idx >= 0)
	{
	    if (is_decl)
	    {
		semsg(_("E1017: Variable already declared: %s"), name);
		goto theend;
	    }
	    else
	    {
		lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;
		if (lvar->lv_const)
		{
		    semsg(_("E1018: Cannot assign to a constant: %s"), name);
		    goto theend;
		}
	    }
	}
	else if (lookup_script(arg, varlen) == OK)
	{
	    script = TRUE;
	    if (is_decl)
	    {
		semsg(_("E1054: Variable already declared in the script: %s"),
									 name);
		goto theend;
	    }
	}
    }

    if (!option)
    {
	if (is_decl && *p == ':')
	{
	    // parse optional type: "let var: type = expr"
	    p = skipwhite(p + 1);
	    type = parse_type(&p, cctx->ctx_type_list);
	    if (type == NULL)
		goto theend;
	    has_type = TRUE;
	}
	else if (idx < 0)
	{
	    // global and new local default to "any" type
	    type = &t_any;
	}
	else
	{
	    lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;
	    type = lvar->lv_type;
	}
    }

    sp = p;
    p = skipwhite(p);
    op = p;
    oplen = assignment_len(p, &heredoc);
    if (oplen > 0 && (!VIM_ISWHITE(*sp) || !VIM_ISWHITE(op[oplen])))
    {
	char_u  buf[4];

	vim_strncpy(buf, op, oplen);
	semsg(_(e_white_both), buf);
    }

    if (oplen == 3 && !heredoc && !global && type->tt_type != VAR_STRING
					       && type->tt_type != VAR_UNKNOWN)
    {
	emsg("E1019: Can only concatenate to string");
	goto theend;
    }

    // +=, /=, etc. require an existing variable
    if (idx < 0 && !global && !option)
    {
	if (oplen > 1 && !heredoc)
	{
	    semsg(_("E1020: cannot use an operator on a new variable: %s"),
									 name);
	    goto theend;
	}

	// new local variable
	idx = reserve_local(cctx, arg, varlen, cmdidx == CMD_const, type);
	if (idx < 0)
	    goto theend;
    }

    if (heredoc)
    {
	list_T	   *l;
	listitem_T *li;

	// [let] varname =<< [trim] {end}
	eap->getline = heredoc_getline;
	eap->cookie = cctx;
	l = heredoc_get(eap, op + 3);

	// Push each line and the create the list.
	for (li = l->lv_first; li != NULL; li = li->li_next)
	{
	    generate_PUSHS(cctx, li->li_tv.vval.v_string);
	    li->li_tv.vval.v_string = NULL;
	}
	generate_NEWLIST(cctx, l->lv_len);
	type = &t_list_string;
	list_free(l);
	p += STRLEN(p);
    }
    else if (oplen > 0)
    {
	// for "+=", "*=", "..=" etc. first load the current value
	if (*op != '=')
	{
	    if (option)
		generate_LOAD(cctx, ISN_LOADOPT, 0, name + 1, type);
	    else if (global)
		generate_LOAD(cctx, ISN_LOADG, 0, name + 2, type);
	    else
		generate_LOAD(cctx, ISN_LOAD, idx, NULL, type);
	}

	// compile the expression
	instr_count = instr->ga_len;
	p = skipwhite(p + oplen);
	if (compile_expr1(&p, cctx) == FAIL)
	    goto theend;

	if (idx >= 0 && (is_decl || !has_type))
	{
	    garray_T	*stack = &cctx->ctx_type_stack;
	    type_T	*stacktype =
				((type_T **)stack->ga_data)[stack->ga_len - 1];

	    lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + idx;
	    if (!has_type)
	    {
		if (stacktype->tt_type == VAR_VOID)
		{
		    emsg(_("E1031: Cannot use void value"));
		    goto theend;
		}
		else
		    lvar->lv_type = stacktype;
	    }
	    else
		if (check_type(lvar->lv_type, stacktype, TRUE) == FAIL)
		    goto theend;
	}
    }
    else if (cmdidx == CMD_const)
    {
	emsg(_("E1021: const requires a value"));
	goto theend;
    }
    else if (!has_type || option)
    {
	emsg(_("E1022: type or initialization required"));
	goto theend;
    }
    else
    {
	// variables are always initialized
	// TODO: support more types
	if (ga_grow(instr, 1) == FAIL)
	    goto theend;
	if (type->tt_type == VAR_STRING)
	    generate_PUSHS(cctx, vim_strsave((char_u *)""));
	else
	    generate_PUSHNR(cctx, 0);
    }

    if (oplen > 0 && *op != '=')
    {
	type_T	    *expected = &t_number;
	garray_T    *stack = &cctx->ctx_type_stack;
	type_T	    *stacktype;

	// TODO: if type is known use float or any operation

	if (*op == '.')
	    expected = &t_string;
	stacktype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
	if (need_type(stacktype, expected, -1, cctx) == FAIL)
	    goto theend;

	if (*op == '.')
	    generate_instr_drop(cctx, ISN_CONCAT, 1);
	else
	{
	    isn_T *isn = generate_instr_drop(cctx, ISN_OPNR, 1);

	    if (isn == NULL)
		goto theend;
	    switch (*op)
	    {
		case '+': isn->isn_arg.op.op_type = EXPR_ADD; break;
		case '-': isn->isn_arg.op.op_type = EXPR_SUB; break;
		case '*': isn->isn_arg.op.op_type = EXPR_MULT; break;
		case '/': isn->isn_arg.op.op_type = EXPR_DIV; break;
		case '%': isn->isn_arg.op.op_type = EXPR_REM; break;
	    }
	}
    }

    if (option)
	generate_STOREOPT(cctx, name + 1, opt_flags);
    else if (global)
	generate_STORE(cctx, ISN_STOREG, 0, name + 2);
    else if (script)
    {
	idx = get_script_item_idx(current_sctx.sc_sid, name, TRUE);
	// TODO: specific type
	generate_SCRIPT(cctx, ISN_STORESCRIPT,
					     current_sctx.sc_sid, idx, &t_any);
    }
    else
    {
	isn_T *isn = ((isn_T *)instr->ga_data) + instr->ga_len - 1;

	// optimization: turn "var = 123" from ISN_PUSHNR + ISN_STORE into
	// ISN_STORENR
	if (instr->ga_len == instr_count + 1 && isn->isn_type == ISN_PUSHNR)
	{
	    varnumber_T val = isn->isn_arg.number;
	    garray_T	*stack = &cctx->ctx_type_stack;

	    isn->isn_type = ISN_STORENR;
	    isn->isn_arg.storenr.str_idx = idx;
	    isn->isn_arg.storenr.str_val = val;
	    if (stack->ga_len > 0)
		--stack->ga_len;
	}
	else
	    generate_STORE(cctx, ISN_STORE, idx, NULL);
    }
    ret = p;

theend:
    vim_free(name);
    return ret;
}

/*
 * Compile an :import command.
 */
    static char_u *
compile_import(char_u *arg, cctx_T *cctx)
{
    return handle_import(arg, &cctx->ctx_imports, 0);
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
    scope_T	*scope;

    // compile "expr"
    if (compile_expr1(&p, cctx) == FAIL)
	return NULL;

    scope = new_scope(cctx, IF_SCOPE);
    if (scope == NULL)
	return NULL;

    // "where" is set when ":elseif", "else" or ":endif" is found
    scope->se_if.is_if_label = instr->ga_len;
    generate_JUMP(cctx, JUMP_IF_FALSE, 0);

    return p;
}

    static char_u *
compile_elseif(char_u *arg, cctx_T *cctx)
{
    char_u	*p = arg;
    garray_T	*instr = &cctx->ctx_instr;
    isn_T	*isn;
    scope_T	*scope = cctx->ctx_scope;

    if (scope == NULL || scope->se_type != IF_SCOPE)
    {
	emsg(_(e_elseif_without_if));
	return NULL;
    }
    cctx->ctx_locals.ga_len = scope->se_local_count;

    // jump from previous block to the end
    if (compile_jump_to_end(&scope->se_if.is_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	return NULL;

    // previous "if" or "elseif" jumps here
    isn = ((isn_T *)instr->ga_data) + scope->se_if.is_if_label;
    isn->isn_arg.jump.jump_where = instr->ga_len;

    // compile "expr"
    if (compile_expr1(&p, cctx) == FAIL)
	return NULL;

    // "where" is set when ":elseif", "else" or ":endif" is found
    scope->se_if.is_if_label = instr->ga_len;
    generate_JUMP(cctx, JUMP_IF_FALSE, 0);

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
    cctx->ctx_locals.ga_len = scope->se_local_count;

    // jump from previous block to the end
    if (compile_jump_to_end(&scope->se_if.is_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	return NULL;

    // previous "if" or "elseif" jumps here
    isn = ((isn_T *)instr->ga_data) + scope->se_if.is_if_label;
    isn->isn_arg.jump.jump_where = instr->ga_len;

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
    ifscope = &scope->se_if;
    cctx->ctx_scope = scope->se_outer;
    cctx->ctx_locals.ga_len = scope->se_local_count;

    // previous "if" or "elseif" jumps here
    isn = ((isn_T *)instr->ga_data) + scope->se_if.is_if_label;
    isn->isn_arg.jump.jump_where = instr->ga_len;

    // Fill in the "end" label in jumps at the end of the blocks.
    compile_fill_jump_to_end(&ifscope->is_end_label, cctx);

    vim_free(scope);
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
    int		loop_idx;	// index of loop iteration variable
    int		var_idx;	// index of "var"
    type_T	*vartype;

    // TODO: list of variables: "for [key, value] in dict"
    // parse "var"
    for (p = arg; eval_isnamec1(*p); ++p)
	;
    varlen = p - arg;
    var_idx = lookup_local(arg, varlen, cctx);
    if (var_idx >= 0)
    {
	semsg(_("E1023: variable already defined: %s"), arg);
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
    loop_idx = reserve_local(cctx, (char_u *)"", 0, FALSE, &t_number);
    if (loop_idx < 0)
	return NULL;

    // Reserve a variable to store "var"
    var_idx = reserve_local(cctx, arg, varlen, FALSE, &t_any);
    if (var_idx < 0)
	return NULL;

    generate_STORENR(cctx, loop_idx, -1);

    // compile "expr", it remains on the stack until "endfor"
    arg = p;
    if (compile_expr1(&arg, cctx) == FAIL)
	return NULL;

    // now we know the type of "var"
    vartype = ((type_T **)stack->ga_data)[stack->ga_len - 1];
    if (vartype->tt_type != VAR_LIST)
    {
	emsg(_("E1024: need a List to iterate over"));
	return NULL;
    }
    if (vartype->tt_member->tt_type != VAR_UNKNOWN)
    {
	lvar_T *lvar = ((lvar_T *)cctx->ctx_locals.ga_data) + var_idx;

	lvar->lv_type = vartype->tt_member;
    }

    // "for_end" is set when ":endfor" is found
    scope->se_for.fs_top_label = instr->ga_len;

    generate_FOR(cctx, loop_idx);
    generate_STORE(cctx, ISN_STORE, var_idx, NULL);

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
    forscope = &scope->se_for;
    cctx->ctx_scope = scope->se_outer;
    cctx->ctx_locals.ga_len = scope->se_local_count;

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

    scope->se_while.ws_top_label = instr->ga_len;

    // compile "expr"
    if (compile_expr1(&p, cctx) == FAIL)
	return NULL;

    // "while_end" is set when ":endwhile" is found
    if (compile_jump_to_end(&scope->se_while.ws_end_label,
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
    cctx->ctx_locals.ga_len = scope->se_local_count;

    // At end of ":for" scope jump back to the FOR instruction.
    generate_JUMP(cctx, JUMP_ALWAYS, scope->se_while.ws_top_label);

    // Fill in the "end" label in the WHILE statement so it can jump here.
    // And in any jumps for ":break"
    compile_fill_jump_to_end(&scope->se_while.ws_end_label, cctx);

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
	    scope->se_type == FOR_SCOPE ? scope->se_for.fs_top_label
					       : scope->se_while.ws_top_label);
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
	el = &scope->se_for.fs_end_label;
    else
	el = &scope->se_while.ws_end_label;
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
    cctx->ctx_locals.ga_len = scope->se_local_count;
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
    try_scope->se_try.ts_try_label = instr->ga_len;
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

    if (scope->se_try.ts_caught_all)
    {
	emsg(_("E1033: catch unreachable after catch-all"));
	return NULL;
    }

    // Jump from end of previous block to :finally or :endtry
    if (compile_jump_to_end(&scope->se_try.ts_end_label,
						    JUMP_ALWAYS, cctx) == FAIL)
	return NULL;

    // End :try or :catch scope: set value in ISN_TRY instruction
    isn = ((isn_T *)instr->ga_data) + scope->se_try.ts_try_label;
    if (isn->isn_arg.try.try_catch == 0)
	isn->isn_arg.try.try_catch = instr->ga_len;
    if (scope->se_try.ts_catch_label != 0)
    {
	// Previous catch without match jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_try.ts_catch_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
    }

    p = skipwhite(arg);
    if (ends_excmd(*p))
    {
	scope->se_try.ts_caught_all = TRUE;
	scope->se_try.ts_catch_label = 0;
    }
    else
    {
	// Push v:exception, push {expr} and MATCH
	generate_instr_type(cctx, ISN_PUSHEXC, &t_string);

	if (compile_expr1(&p, cctx) == FAIL)
	    return NULL;

	// TODO: check for strings?
	if (generate_COMPARE(cctx, EXPR_MATCH, FALSE) == FAIL)
	    return NULL;

	scope->se_try.ts_catch_label = instr->ga_len;
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
    isn = ((isn_T *)instr->ga_data) + scope->se_try.ts_try_label;
    if (isn->isn_arg.try.try_finally != 0)
    {
	emsg(_(e_finally_dup));
	return NULL;
    }

    // Fill in the "end" label in jumps at the end of the blocks.
    compile_fill_jump_to_end(&scope->se_try.ts_end_label, cctx);

    if (scope->se_try.ts_catch_label != 0)
    {
	// Previous catch without match jumps here
	isn = ((isn_T *)instr->ga_data) + scope->se_try.ts_catch_label;
	isn->isn_arg.jump.jump_where = instr->ga_len;
    }

    isn->isn_arg.try.try_finally = instr->ga_len;
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
	if (scope->se_type == FOR_SCOPE)
	    emsg(_(e_endfor));
	else
	    emsg(_(e_endif));
	return NULL;
    }

    isn = ((isn_T *)instr->ga_data) + scope->se_try.ts_try_label;
    if (isn->isn_arg.try.try_catch == 0 && isn->isn_arg.try.try_finally == 0)
    {
	emsg(_("E1032: missing :catch or :finally"));
	return NULL;
    }

    // Fill in the "end" label in jumps at the end of the blocks, if not done
    // by ":finally".
    compile_fill_jump_to_end(&scope->se_try.ts_end_label, cctx);

    // End :catch or :finally scope: set value in ISN_TRY instruction
    if (isn->isn_arg.try.try_finally == 0)
	isn->isn_arg.try.try_finally = instr->ga_len;
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

    if (ends_excmd(*p))
    {
	emsg(_(e_argreq));
	return NULL;
    }
    if (compile_expr1(&p, cctx) == FAIL)
	return NULL;
    if (may_generate_2STRING(-1, cctx) == FAIL)
	return NULL;
    if (generate_instr_drop(cctx, ISN_THROW, 1) == NULL)
	return NULL;

    return p;
}

/*
 * compile "echo expr"
 */
    static char_u *
compile_echo(char_u *arg, int with_white, cctx_T *cctx)
{
    char_u	*p = arg;
    int		count = 0;

    // for ()
    {
	if (compile_expr1(&p, cctx) == FAIL)
	    return NULL;
	++count;
    }

    generate_ECHO(cctx, with_white, count);

    return p;
}

/*
 * After ex_function() has collected all the function lines: parse and compile
 * the lines into instructions.
 * Adds the function to "def_functions".
 * When "set_return_type" is set then set ufunc->uf_ret_type to the type of the
 * return statement (used for lambda).
 */
    void
compile_def_function(ufunc_T *ufunc, int set_return_type)
{
    dfunc_T	*dfunc;
    char_u	*line = NULL;
    char_u	*p;
    exarg_T	ea;
    char	*errormsg = NULL;	// error message
    int		had_return = FALSE;
    cctx_T	cctx;
    garray_T	*instr;
    int		called_emsg_before = called_emsg;
    int		ret = FAIL;
    sctx_T	save_current_sctx = current_sctx;

    if (ufunc->uf_dfunc_idx >= 0)
    {
	// redefining a function that was compiled before
	dfunc = ((dfunc_T *)def_functions.ga_data) + ufunc->uf_dfunc_idx;
	dfunc->df_deleted = FALSE;
    }
    else
    {
	// Add the function to "def_functions".
	if (ga_grow(&def_functions, 1) == FAIL)
	    return;
	dfunc = ((dfunc_T *)def_functions.ga_data) + def_functions.ga_len;
	vim_memset(dfunc, 0, sizeof(dfunc_T));
	dfunc->df_idx = def_functions.ga_len;
	ufunc->uf_dfunc_idx = dfunc->df_idx;
	dfunc->df_ufunc = ufunc;
	++def_functions.ga_len;
    }

    vim_memset(&cctx, 0, sizeof(cctx));
    cctx.ctx_ufunc = ufunc;
    cctx.ctx_lnum = -1;
    ga_init2(&cctx.ctx_locals, sizeof(lvar_T), 10);
    ga_init2(&cctx.ctx_type_stack, sizeof(type_T *), 50);
    ga_init2(&cctx.ctx_imports, sizeof(imported_T), 10);
    cctx.ctx_type_list = &ufunc->uf_type_list;
    ga_init2(&cctx.ctx_instr, sizeof(isn_T), 50);
    instr = &cctx.ctx_instr;

    // Most modern script version.
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;

    for (;;)
    {
	if (line != NULL && *line == '|')
	    // the line continues after a '|'
	    ++line;
	else if (line != NULL && *line != NUL)
	{
	    semsg(_("E488: Trailing characters: %s"), line);
	    goto erret;
	}
	else
	{
	    do
	    {
		++cctx.ctx_lnum;
		if (cctx.ctx_lnum == ufunc->uf_lines.ga_len)
		    break;
		line = ((char_u **)ufunc->uf_lines.ga_data)[cctx.ctx_lnum];
	    } while (line == NULL);
	    if (cctx.ctx_lnum == ufunc->uf_lines.ga_len)
		break;
	    SOURCING_LNUM = ufunc->uf_script_ctx.sc_lnum + cctx.ctx_lnum + 1;
	}

	had_return = FALSE;
	vim_memset(&ea, 0, sizeof(ea));
	ea.cmdlinep = &line;
	ea.cmd = skipwhite(line);

	// "}" ends a block scope
	if (*ea.cmd == '}')
	{
	    scopetype_T stype = cctx.ctx_scope == NULL
					 ? NO_SCOPE : cctx.ctx_scope->se_type;

	    if (stype == BLOCK_SCOPE)
	    {
		compile_endblock(&cctx);
		line = ea.cmd;
	    }
	    else
	    {
		emsg("E1025: using } outside of a block scope");
		goto erret;
	    }
	    if (line != NULL)
		line = skipwhite(ea.cmd + 1);
	    continue;
	}

	// "{" starts a block scope
	if (*ea.cmd == '{')
	{
	    line = compile_block(ea.cmd, &cctx);
	    continue;
	}

	/*
	 * COMMAND MODIFIERS
	 */
	if (parse_command_modifiers(&ea, &errormsg, FALSE) == FAIL)
	{
	    if (errormsg != NULL)
		goto erret;
	    // empty line or comment
	    line = (char_u *)"";
	    continue;
	}

	// Skip ":call" to get to the function name.
	if (checkforcmd(&ea.cmd, "call", 3))
	    ea.cmd = skipwhite(ea.cmd);

	// Assuming the command starts with a variable or function name, find
	// what follows.  Also "&opt = value".
	p = (*ea.cmd == '&') ? ea.cmd + 1 : ea.cmd;
	p = to_name_end(p);
	if (p > ea.cmd && *p != NUL)
	{
	    int oplen;
	    int heredoc;

	    // "funcname(" is always a function call.
	    // "varname[]" is an expression.
	    // "g:varname" is an expression.
	    // "varname->expr" is an expression.
	    if (*p == '('
		    || *p == '['
		    || ((p - ea.cmd) > 2 && ea.cmd[1] == ':')
		    || (*p == '-' && p[1] == '>'))
	    {
		// TODO
	    }

	    oplen = assignment_len(skipwhite(p), &heredoc);
	    if (oplen > 0)
	    {
		// Recognize an assignment if we recognize the variable name:
		// "g:var = expr"
		// "var = expr"  where "var" is a local var name.
		// "&opt = expr"
		if (*ea.cmd == '&'
			|| ((p - ea.cmd) > 2 && ea.cmd[1] == ':')
			|| lookup_local(ea.cmd, p - ea.cmd, &cctx) >= 0
			|| lookup_script(ea.cmd, p - ea.cmd) == OK)
		{
		    line = compile_assignment(ea.cmd, &ea, CMD_SIZE, &cctx);
		    if (line == NULL)
			goto erret;
		    continue;
		}
	    }
	}

	/*
	 * COMMAND after range
	 */
	ea.cmd = skip_range(ea.cmd, NULL);
	p = find_ex_command(&ea, NULL, lookup_local, &cctx);

	if (p == ea.cmd && ea.cmdidx != CMD_SIZE)
	{
	    // Expression or function call.
	    if (ea.cmdidx == CMD_eval)
	    {
		p = ea.cmd;
		if (compile_expr1(&p, &cctx) == FAIL)
		    goto erret;

		// drop the return value
		generate_instr_drop(&cctx, ISN_DROP, 1);
		line = p;
		continue;
	    }
	    if (ea.cmdidx == CMD_let)
	    {
		line = compile_assignment(ea.cmd, &ea, CMD_SIZE, &cctx);
		if (line == NULL)
		    goto erret;
		continue;
	    }
	    iemsg("Command from find_ex_command() not handled");
	    goto erret;
	}

	p = skipwhite(p);

	switch (ea.cmdidx)
	{
	    case CMD_def:
	    case CMD_function:
		    // TODO: Nested function
		    emsg("Nested function not implemented yet");
		    goto erret;

	    case CMD_return:
		    line = compile_return(p, set_return_type, &cctx);
		    had_return = TRUE;
		    break;

	    case CMD_let:
	    case CMD_const:
		    line = compile_assignment(p, &ea, ea.cmdidx, &cctx);
		    break;

	    case CMD_import:
		    line = compile_import(p, &cctx);
		    break;

	    case CMD_if:
		    line = compile_if(p, &cctx);
		    break;
	    case CMD_elseif:
		    line = compile_elseif(p, &cctx);
		    break;
	    case CMD_else:
		    line = compile_else(p, &cctx);
		    break;
	    case CMD_endif:
		    line = compile_endif(p, &cctx);
		    break;

	    case CMD_while:
		    line = compile_while(p, &cctx);
		    break;
	    case CMD_endwhile:
		    line = compile_endwhile(p, &cctx);
		    break;

	    case CMD_for:
		    line = compile_for(p, &cctx);
		    break;
	    case CMD_endfor:
		    line = compile_endfor(p, &cctx);
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
		    break;
	    case CMD_finally:
		    line = compile_finally(p, &cctx);
		    break;
	    case CMD_endtry:
		    line = compile_endtry(p, &cctx);
		    break;
	    case CMD_throw:
		    line = compile_throw(p, &cctx);
		    break;

	    case CMD_echo:
		    line = compile_echo(p, TRUE, &cctx);
		    break;
	    case CMD_echon:
		    line = compile_echo(p, FALSE, &cctx);
		    break;

	    default:
		    // Not recognized, execute with do_cmdline_cmd().
		    generate_EXEC(&cctx, line);
		    line = (char_u *)"";
		    break;
	}
	if (line == NULL)
	    goto erret;

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
	    emsg(_("E1026: Missing }"));
	goto erret;
    }

    if (!had_return)
    {
	if (ufunc->uf_ret_type->tt_type != VAR_VOID)
	{
	    emsg(_("E1027: Missing return statement"));
	    goto erret;
	}

	// Return zero if there is no return at the end.
	generate_PUSHNR(&cctx, 0);
	generate_instr(&cctx, ISN_RETURN);
    }

    dfunc->df_instr = instr->ga_data;
    dfunc->df_instr_count = instr->ga_len;
    dfunc->df_varcount = cctx.ctx_max_local;

    ret = OK;

erret:
    if (ret == FAIL)
    {
	ga_clear(instr);
	ufunc->uf_dfunc_idx = -1;
	--def_functions.ga_len;
	if (errormsg != NULL)
	    emsg(errormsg);
	else if (called_emsg == called_emsg_before)
	    emsg("E1028: compile_def_function failed");

	// don't execute this function body
	ufunc->uf_lines.ga_len = 0;
    }

    current_sctx = save_current_sctx;
    ga_clear(&cctx.ctx_type_stack);
    ga_clear(&cctx.ctx_locals);
}

/*
 * Delete an instruction, free what it contains.
 */
    static void
delete_instr(isn_T *isn)
{
    switch (isn->isn_type)
    {
	case ISN_EXEC:
	case ISN_LOADENV:
	case ISN_LOADG:
	case ISN_LOADOPT:
	case ISN_MEMBER:
	case ISN_PUSHEXC:
	case ISN_PUSHS:
	case ISN_STOREG:
	    vim_free(isn->isn_arg.string);
	    break;

	case ISN_LOADS:
	    vim_free(isn->isn_arg.loads.ls_name);
	    break;

	case ISN_STOREOPT:
	    vim_free(isn->isn_arg.storeopt.so_name);
	    break;

	case ISN_PUSHBLOB:   // push blob isn_arg.blob
	    blob_unref(isn->isn_arg.blob);
	    break;

	case ISN_UCALL:
	    vim_free(isn->isn_arg.ufunc.cuf_name);
	    break;

	case ISN_2BOOL:
	case ISN_2STRING:
	case ISN_ADDBLOB:
	case ISN_ADDLIST:
	case ISN_BCALL:
	case ISN_CATCH:
	case ISN_CHECKNR:
	case ISN_CHECKTYPE:
	case ISN_COMPAREANY:
	case ISN_COMPAREBLOB:
	case ISN_COMPAREBOOL:
	case ISN_COMPAREDICT:
	case ISN_COMPAREFLOAT:
	case ISN_COMPAREFUNC:
	case ISN_COMPARELIST:
	case ISN_COMPARENR:
	case ISN_COMPAREPARTIAL:
	case ISN_COMPARESPECIAL:
	case ISN_COMPARESTRING:
	case ISN_CONCAT:
	case ISN_DCALL:
	case ISN_DROP:
	case ISN_ECHO:
	case ISN_ENDTRY:
	case ISN_FOR:
	case ISN_FUNCREF:
	case ISN_INDEX:
	case ISN_JUMP:
	case ISN_LOAD:
	case ISN_LOADSCRIPT:
	case ISN_LOADREG:
	case ISN_LOADV:
	case ISN_NEGATENR:
	case ISN_NEWDICT:
	case ISN_NEWLIST:
	case ISN_OPNR:
	case ISN_OPFLOAT:
	case ISN_OPANY:
	case ISN_PCALL:
	case ISN_PUSHF:
	case ISN_PUSHNR:
	case ISN_PUSHBOOL:
	case ISN_PUSHSPEC:
	case ISN_RETURN:
	case ISN_STORE:
	case ISN_STORENR:
	case ISN_STORESCRIPT:
	case ISN_THROW:
	case ISN_TRY:
	    // nothing allocated
	    break;
    }
}

/*
 * When a user function is deleted, delete any associated def function.
 */
    void
delete_def_function(ufunc_T *ufunc)
{
    int idx;

    if (ufunc->uf_dfunc_idx >= 0)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;
	ga_clear(&dfunc->df_def_args_isn);

	for (idx = 0; idx < dfunc->df_instr_count; ++idx)
	    delete_instr(dfunc->df_instr + idx);
	VIM_CLEAR(dfunc->df_instr);

	dfunc->df_deleted = TRUE;
    }
}

#if defined(EXITFREE) || defined(PROTO)
    void
free_def_functions(void)
{
    vim_free(def_functions.ga_data);
}
#endif


#endif // FEAT_EVAL
