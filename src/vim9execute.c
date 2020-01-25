/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9execute.c: execute Vim9 script instructions
 */

#define USING_FLOAT_STUFF
#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

#ifdef VMS
# include <float.h>
#endif

#include "vim9.h"

// Structure put on ec_trystack when ISN_TRY is encountered.
typedef struct {
    int	    tcd_frame;		// ec_frame when ISN_TRY was encountered
    int	    tcd_catch_idx;	// instruction of the first catch
    int	    tcd_finally_idx;	// instruction of the finally block
    int	    tcd_caught;		// catch block entered
    int	    tcd_return;		// when TRUE return from end of :finally
} trycmd_T;


// A stack is used to store:
// - arguments passed to a :def function
// - info about the calling function, to use when returning
// - local variables
// - temporary values
//
// In detail (FP == Frame Pointer):
//	  arg1		first argument from caller (if present)
//	  arg2		second argument from caller (if present)
//	  extra_arg1	any missing optional argument default value
// FP ->  cur_func	calling function
//        current	previous instruction pointer
//        frame_ptr	previous Frame Pointer
//        var1		space for local variable
//        var2		space for local variable
//        ....		fixed space for max. number of local variables
//        temp		temporary values
//        ....		flexible space for temporary values (can grow big)

/*
 * Execution context.
 */
typedef struct {
    garray_T	ec_stack;	// stack of typval_T values
    int		ec_frame;	// index in ec_stack: context of ec_dfunc_idx

    garray_T	ec_trystack;	// stack of trycmd_T values
    int		ec_in_catch;	// when TRUE in catch or finally block

    int		ec_dfunc_idx;	// current function index
    isn_T	*ec_instr;	// array with instructions
    int		ec_iidx;	// index in ec_instr: instruction to execute
} ectx_T;

// Get pointer to item relative to the bottom of the stack, -1 is the last one.
#define STACK_TV_BOT(idx) (((typval_T *)ectx->ec_stack.ga_data) + ectx->ec_stack.ga_len + idx)

/*
 * Return the number of arguments, including any vararg.
 */
    static int
ufunc_argcount(ufunc_T *ufunc)
{
    return ufunc->uf_args.ga_len + (ufunc->uf_va_name != NULL ? 1 : 0);
}

/*
 * Call compiled function "cdf_idx" from compiled code.
 *
 * Stack has:
 * - current arguments (already there)
 * - omitted optional argument (default values) added here
 * - stack frame:
 *	- pointer to calling function
 *	- Index of next instruction in calling function
 *	- previous frame pointer
 * - reserved space for local variables
 */
    static int
call_dfunc(int cdf_idx, int argcount, ectx_T *ectx)
{
    dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data) + cdf_idx;
    ufunc_T *ufunc = dfunc->df_ufunc;
    int	    optcount = ufunc_argcount(ufunc) - argcount;
    int	    idx;

    if (dfunc->df_deleted)
    {
	emsg_funcname(e_func_deleted, ufunc->uf_name);
	return FAIL;
    }

    if (ga_grow(&ectx->ec_stack, optcount + 3 + dfunc->df_varcount) == FAIL)
	return FAIL;

// TODO: Put omitted argument default values on the stack.
    if (optcount > 0)
    {
	emsg("optional arguments not implemented yet");
	return FAIL;
    }
    if (optcount < 0)
    {
	emsg("argument count wrong?");
	return FAIL;
    }
//    for (idx = argcount - dfunc->df_minarg;
//				     idx < dfunc->df_maxarg; ++idx)
//    {
//	copy_tv(&dfunc->df_defarg[idx], STACK_TV_BOT(0));
//	++ectx->ec_stack.ga_len;
//    }

    // Store current execution state in stack frame for ISN_RETURN.
    // TODO: If the actual number of arguments doesn't match what the called
    // function expects things go bad.
    STACK_TV_BOT(0)->vval.v_number = ectx->ec_dfunc_idx;
    STACK_TV_BOT(1)->vval.v_number = ectx->ec_iidx;
    STACK_TV_BOT(2)->vval.v_number = ectx->ec_frame;
    ectx->ec_frame = ectx->ec_stack.ga_len;

    // Initialize local variables
    for (idx = 0; idx < dfunc->df_varcount; ++idx)
	STACK_TV_BOT(STACK_FRAME_SIZE + idx)->v_type = VAR_UNKNOWN;
    ectx->ec_stack.ga_len += STACK_FRAME_SIZE + dfunc->df_varcount;

    // Set execution state to the start of the called function.
    ectx->ec_dfunc_idx = cdf_idx;
    ectx->ec_instr = dfunc->df_instr;
    estack_push_ufunc(ETYPE_UFUNC, dfunc->df_ufunc, 1);
    ectx->ec_iidx = 0;

    return OK;
}

// Get pointer to item in the stack.
#define STACK_TV(idx) (((typval_T *)ectx->ec_stack.ga_data) + idx)

/*
 * Return from the current function.
 */
    static void
func_return(ectx_T *ectx)
{
    int		ret_idx = ectx->ec_stack.ga_len - 1;
    int		idx;
    dfunc_T	*dfunc;

    // execution context goes one level up
    estack_pop();

    // Clear the local variables and temporary values, but not
    // the return value.
    for (idx = ectx->ec_frame + STACK_FRAME_SIZE;
					 idx < ectx->ec_stack.ga_len - 1; ++idx)
	clear_tv(STACK_TV(idx));
    dfunc = ((dfunc_T *)def_functions.ga_data) + ectx->ec_dfunc_idx;
    ectx->ec_stack.ga_len = ectx->ec_frame
					 - ufunc_argcount(dfunc->df_ufunc) + 1;
    ectx->ec_dfunc_idx = STACK_TV(ectx->ec_frame)->vval.v_number;
    ectx->ec_iidx = STACK_TV(ectx->ec_frame + 1)->vval.v_number;
    ectx->ec_frame = STACK_TV(ectx->ec_frame + 2)->vval.v_number;
    *STACK_TV_BOT(-1) = *STACK_TV(ret_idx);
    dfunc = ((dfunc_T *)def_functions.ga_data) + ectx->ec_dfunc_idx;
    ectx->ec_instr = dfunc->df_instr;
}

#undef STACK_TV

/*
 * Prepare arguments and rettv for calling a builtin or user function.
 */
    static int
call_prepare(int argcount, typval_T *argvars, ectx_T *ectx)
{
    int		idx;
    typval_T	*tv;

    // Move arguments from bottom of the stack to argvars[] and add terminator.
    for (idx = 0; idx < argcount; ++idx)
	argvars[idx] = *STACK_TV_BOT(idx - argcount);
    argvars[argcount].v_type = VAR_UNKNOWN;

    // Result replaces the arguments on the stack.
    if (argcount > 0)
	ectx->ec_stack.ga_len -= argcount - 1;
    else if (ga_grow(&ectx->ec_stack, 1) == FAIL)
	return FAIL;
    else
	++ectx->ec_stack.ga_len;

    // Default return value is zero.
    tv = STACK_TV_BOT(-1);
    tv->v_type = VAR_NUMBER;
    tv->vval.v_number = 0;

    return OK;
}

/*
 * Call a builtin function by index.
 */
    static int
call_bfunc(int func_idx, int argcount, ectx_T *ectx)
{
    typval_T	argvars[MAX_FUNC_ARGS];
    int		idx;

    if (call_prepare(argcount, argvars, ectx) == FAIL)
	return FAIL;

    // Call the builtin function.
    call_internal_func_by_idx(func_idx, argvars, STACK_TV_BOT(-1));

    // Clear the arguments.
    for (idx = 0; idx < argcount; ++idx)
	clear_tv(&argvars[idx]);
    return OK;
}

/*
 * Execute a user defined function.
 */
    static int
call_ufunc(ufunc_T *ufunc, int argcount, ectx_T *ectx)
{
    typval_T	argvars[MAX_FUNC_ARGS];
    funcexe_T   funcexe;
    int		error;
    int		idx;

    if (ufunc->uf_dfunc_idx >= 0)
	// The function has been compiled, can call it quickly.
	return call_dfunc(ufunc->uf_dfunc_idx, argcount, ectx);

    if (call_prepare(argcount, argvars, ectx) == FAIL)
	return FAIL;
    vim_memset(&funcexe, 0, sizeof(funcexe));
    funcexe.evaluate = TRUE;

    // Call the user function.  Result goes in last position on the stack.
    // TODO: add selfdict if there is one
    error = call_user_func_check(ufunc, argcount, argvars,
					     STACK_TV_BOT(-1), &funcexe, NULL);

    // Clear the arguments.
    for (idx = 0; idx < argcount; ++idx)
	clear_tv(&argvars[idx]);

    if (error != FCERR_NONE)
    {
	user_func_error(error, ufunc->uf_name);
	return FAIL;
    }
    return OK;
}

/*
 * Execute a function by "name".
 * This can be a builtin function or a user function.
 * Returns FAIL if not found without an error message.
 */
    static int
call_by_name(char_u *name, int argcount, ectx_T *ectx)
{
    ufunc_T *ufunc;

    if (builtin_function(name, -1))
    {
	int func_idx = find_internal_func(name);

	if (func_idx < 0)
	    return FAIL;
	if (check_internal_func(func_idx, argcount) == FAIL)
	    return FAIL;
	return call_bfunc(func_idx, argcount, ectx);
    }

    ufunc = find_func(name, NULL);
    if (ufunc != NULL)
	return call_ufunc(ufunc, argcount, ectx);

    return FAIL;
}

    static int
call_partial(typval_T *tv, int argcount, ectx_T *ectx)
{
    char_u	*name;
    int		called_emsg_before = called_emsg;

    if (tv->v_type == VAR_PARTIAL)
    {
	partial_T *pt = tv->vval.v_partial;

	if (pt->pt_func != NULL)
	    return call_ufunc(pt->pt_func, argcount, ectx);
	name = pt->pt_name;
    }
    else
	name = tv->vval.v_string;
    if (call_by_name(name, argcount, ectx) == FAIL)
    {
	if (called_emsg == called_emsg_before)
	    semsg(_(e_unknownfunc), name);
	return FAIL;
    }
    return OK;
}

/*
 * Execute a function by "name".
 * This can be a builtin function, user function or a funcref.
 */
    static int
call_eval_func(char_u *name, int argcount, ectx_T *ectx)
{
    int		called_emsg_before = called_emsg;

    if (call_by_name(name, argcount, ectx) == FAIL
					  && called_emsg == called_emsg_before)
    {
	// "name" may be a variable that is a funcref or partial
	//    if find variable
	//      call_partial()
	//    else
	//      semsg(_(e_unknownfunc), name);
	emsg("call_eval_func(partial) not implemented yet");
	return FAIL;
    }
    return OK;
}

/*
 * Call a "def" function from old Vim script.
 * Return OK or FAIL.
 */
    int
call_def_function(
    ufunc_T	*ufunc,
    int		argc,		// nr of arguments
    typval_T	*argv,		// arguments
    typval_T	*rettv)		// return value
{
    ectx_T	ectx;		// execution context
    int		initial_frame_ptr;
    typval_T	*tv;
    int		idx;
    int		ret = FAIL;
    dfunc_T	*dfunc;

// Get pointer to item in the stack.
#define STACK_TV(idx) (((typval_T *)ectx.ec_stack.ga_data) + idx)

// Get pointer to item at the bottom of the stack, -1 is the bottom.
#undef STACK_TV_BOT
#define STACK_TV_BOT(idx) (((typval_T *)ectx.ec_stack.ga_data) + ectx.ec_stack.ga_len + idx)

// Get pointer to local variable on the stack.
#define STACK_TV_VAR(idx) (((typval_T *)ectx.ec_stack.ga_data) + ectx.ec_frame + STACK_FRAME_SIZE + idx)

    vim_memset(&ectx, 0, sizeof(ectx));
    ga_init2(&ectx.ec_stack, sizeof(typval_T), 500);
    if (ga_grow(&ectx.ec_stack, 20) == FAIL)
	goto failed;
    ectx.ec_dfunc_idx = ufunc->uf_dfunc_idx;

    ga_init2(&ectx.ec_trystack, sizeof(trycmd_T), 10);

    // Put arguments on the stack.
    for (idx = 0; idx < argc; ++idx)
    {
	copy_tv(&argv[idx], STACK_TV_BOT(0));
	++ectx.ec_stack.ga_len;
    }

    // Frame pointer points to just after arguments.
    ectx.ec_frame = ectx.ec_stack.ga_len;
    initial_frame_ptr = ectx.ec_frame;

    // dummy frame entries
    for (idx = 0; idx < STACK_FRAME_SIZE; ++idx)
    {
	STACK_TV(ectx.ec_stack.ga_len)->v_type = VAR_UNKNOWN;
	++ectx.ec_stack.ga_len;
    }

    // Reserve space for local variables.
    dfunc = ((dfunc_T *)def_functions.ga_data) + ufunc->uf_dfunc_idx;
    for (idx = 0; idx < dfunc->df_varcount; ++idx)
	STACK_TV_VAR(idx)->v_type = VAR_UNKNOWN;
    ectx.ec_stack.ga_len += dfunc->df_varcount;

    ectx.ec_instr = dfunc->df_instr;
    ectx.ec_iidx = 0;
    for (;;)
    {
	isn_T	    *iptr;
	trycmd_T    *trycmd = NULL;

	if (did_throw && !ectx.ec_in_catch)
	{
	    garray_T	*trystack = &ectx.ec_trystack;

	    // An exception jumps to the first catch, finally, or returns from
	    // the current function.
	    if (trystack->ga_len > 0)
		trycmd = ((trycmd_T *)trystack->ga_data) + trystack->ga_len - 1;
	    if (trycmd != NULL && trycmd->tcd_frame == ectx.ec_frame)
	    {
		// jump to ":catch" or ":finally"
		ectx.ec_in_catch = TRUE;
		ectx.ec_iidx = trycmd->tcd_catch_idx;
	    }
	    else
	    {
		// not inside try or need to return from current functions.
		if (ectx.ec_frame == initial_frame_ptr)
		{
		    // At the toplevel we are done.  Push a dummy return value.
		    if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    tv = STACK_TV_BOT(0);
		    tv->v_type = VAR_NUMBER;
		    tv->vval.v_number = 0;
		    ++ectx.ec_stack.ga_len;
		    goto done;
		}

		func_return(&ectx);
	    }
	    continue;
	}

	iptr = &ectx.ec_instr[ectx.ec_iidx++];
	switch (iptr->isn_type)
	{
	    // execute Ex command line
	    case ISN_EXEC:
		do_cmdline_cmd(iptr->isn_arg.string);
		break;

	    // execute :echo {string} ...
	    case ISN_ECHO:
		{
		    int count = iptr->isn_arg.echo.echo_count;
		    int	atstart = TRUE;
		    int needclr = TRUE;

		    for (idx = 0; idx < count; ++idx)
		    {
			tv = STACK_TV_BOT(idx - count);
			echo_one(tv, iptr->isn_arg.echo.echo_with_white,
							   &atstart, &needclr);
			clear_tv(tv);
		    }
		    ectx.ec_stack.ga_len -= count;
		}
		break;

	    // load local variable or argument
	    case ISN_LOAD:
		if (ga_grow(&ectx.ec_stack, 1) == FAIL)
		    goto failed;
		copy_tv(STACK_TV_VAR(iptr->isn_arg.number), STACK_TV_BOT(0));
		++ectx.ec_stack.ga_len;
		break;

	    // load v: variable
	    case ISN_LOADV:
		if (ga_grow(&ectx.ec_stack, 1) == FAIL)
		    goto failed;
		copy_tv(get_vim_var_tv(iptr->isn_arg.number), STACK_TV_BOT(0));
		++ectx.ec_stack.ga_len;
		break;

	    // load s: variable in vim9script
	    case ISN_LOADSCRIPT:
		{
		    scriptitem_T *si =
				 &SCRIPT_ITEM(iptr->isn_arg.script.script_sid);
		    svar_T	 *sv;

		    sv = ((svar_T *)si->sn_var_vals.ga_data)
					     + iptr->isn_arg.script.script_idx;
		    if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    copy_tv(sv->sv_tv, STACK_TV_BOT(0));
		    ++ectx.ec_stack.ga_len;
		}
		break;

	    // load s: variable in old script
	    case ISN_LOADS:
		{
		    hashtab_T	*ht = &SCRIPT_VARS(iptr->isn_arg.loads.ls_sid);
		    char_u	*name = iptr->isn_arg.loads.ls_name;
		    dictitem_T	*di = find_var_in_ht(ht, 0, name, TRUE);
		    if (di == NULL)
		    {
			semsg(_("E121: Undefined variable: s:%s"), name);
			goto failed;
		    }
		    else
		    {
			if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			    goto failed;
			copy_tv(&di->di_tv, STACK_TV_BOT(0));
			++ectx.ec_stack.ga_len;
		    }
		}
		break;

	    // load g: variable
	    case ISN_LOADG:
		{
		    dictitem_T *di;

		    di = find_var_in_ht(get_globvar_ht(), 0,
						   iptr->isn_arg.string, TRUE);
		    if (di == NULL)
		    {
			semsg(_("E121: Undefined variable: g:%s"),
							 iptr->isn_arg.string);
			goto failed;
		    }
		    else
		    {
			if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			    goto failed;
			copy_tv(&di->di_tv, STACK_TV_BOT(0));
			++ectx.ec_stack.ga_len;
		    }
		}
		break;

	    // load &option
	    case ISN_LOADOPT:
		{
		    typval_T	optval;
		    char_u	*name = iptr->isn_arg.string;

		    if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    get_option_tv(&name, &optval, TRUE);
		    *STACK_TV_BOT(0) = optval;
		    ++ectx.ec_stack.ga_len;
		}
		break;

	    // load $ENV
	    case ISN_LOADENV:
		{
		    typval_T	optval;
		    char_u	*name = iptr->isn_arg.string;

		    if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    get_env_tv(&name, &optval, TRUE);
		    *STACK_TV_BOT(0) = optval;
		    ++ectx.ec_stack.ga_len;
		}
		break;

	    // load @register
	    case ISN_LOADREG:
		if (ga_grow(&ectx.ec_stack, 1) == FAIL)
		    goto failed;
		tv = STACK_TV_BOT(0);
		tv->v_type = VAR_STRING;
		tv->vval.v_string = get_reg_contents(
					  iptr->isn_arg.number, GREG_EXPR_SRC);
		++ectx.ec_stack.ga_len;
		break;

	    // store local variable
	    case ISN_STORE:
		--ectx.ec_stack.ga_len;
		tv = STACK_TV_VAR(iptr->isn_arg.number);
		clear_tv(tv);
		*tv = *STACK_TV_BOT(0);
		break;

	    // store script-local variable
	    case ISN_STORESCRIPT:
		{
		    scriptitem_T *si = &SCRIPT_ITEM(
					      iptr->isn_arg.script.script_sid);
		    svar_T	 *sv = ((svar_T *)si->sn_var_vals.ga_data)
					     + iptr->isn_arg.script.script_idx;

		    --ectx.ec_stack.ga_len;
		    clear_tv(sv->sv_tv);
		    *sv->sv_tv = *STACK_TV_BOT(0);
		}
		break;

	    // store option
	    case ISN_STOREOPT:
		{
		    long	n = 0;
		    char_u	*s = NULL;
		    char	*msg;

		    --ectx.ec_stack.ga_len;
		    tv = STACK_TV_BOT(0);
		    if (tv->v_type == VAR_STRING)
			s = tv->vval.v_string;
		    else if (tv->v_type == VAR_NUMBER)
			n = tv->vval.v_number;
		    else
		    {
			emsg(_("E1051: Expected string or number"));
			goto failed;
		    }
		    msg = set_option_value(iptr->isn_arg.storeopt.so_name,
					n, s, iptr->isn_arg.storeopt.so_flags);
		    if (msg != NULL)
		    {
			emsg(_(msg));
			goto failed;
		    }
		    clear_tv(tv);
		}
		break;

	    // store g: variable
	    case ISN_STOREG:
		{
		    dictitem_T *di;

		    --ectx.ec_stack.ga_len;
		    di = find_var_in_ht(get_globvar_ht(), 0,
						   iptr->isn_arg.string, TRUE);
		    if (di == NULL)
		    {
			funccal_entry_T entry;

			save_funccal(&entry);
			set_var_const(iptr->isn_arg.string, NULL,
						    STACK_TV_BOT(0), FALSE, 0);
			restore_funccal();
		    }
		    else
		    {
			clear_tv(&di->di_tv);
			di->di_tv = *STACK_TV_BOT(0);
		    }
		}
		break;

	    // store number in local variable
	    case ISN_STORENR:
		tv = STACK_TV_VAR(iptr->isn_arg.storenr.str_idx);
		clear_tv(tv);
		tv->v_type = VAR_NUMBER;
		tv->vval.v_number = iptr->isn_arg.storenr.str_val;
		break;

	    // push constant
	    case ISN_PUSHNR:
	    case ISN_PUSHBOOL:
	    case ISN_PUSHSPEC:
	    case ISN_PUSHF:
	    case ISN_PUSHS:
	    case ISN_PUSHBLOB:
		if (ga_grow(&ectx.ec_stack, 1) == FAIL)
		    goto failed;
		tv = STACK_TV_BOT(0);
		++ectx.ec_stack.ga_len;
		switch (iptr->isn_type)
		{
		    case ISN_PUSHNR:
			tv->v_type = VAR_NUMBER;
			tv->vval.v_number = iptr->isn_arg.number;
			break;
		    case ISN_PUSHBOOL:
			tv->v_type = VAR_BOOL;
			tv->vval.v_number = iptr->isn_arg.number;
			break;
		    case ISN_PUSHSPEC:
			tv->v_type = VAR_SPECIAL;
			tv->vval.v_number = iptr->isn_arg.number;
			break;
#ifdef FEAT_FLOAT
		    case ISN_PUSHF:
			tv->v_type = VAR_FLOAT;
			tv->vval.v_float = iptr->isn_arg.fnumber;
			break;
#endif
		    case ISN_PUSHBLOB:
			blob_copy(iptr->isn_arg.blob, tv);
			break;
		    default:
			tv->v_type = VAR_STRING;
			tv->vval.v_string = vim_strsave(iptr->isn_arg.string);
		}
		break;

	    // create a list from items on the stack; uses a single allocation
	    // for the list header and the items
	    case ISN_NEWLIST:
		{
		    int	    count = iptr->isn_arg.number;
		    list_T  *list = list_alloc_with_items(count);

		    if (list == NULL)
			goto failed;
		    for (idx = 0; idx < count; ++idx)
			list_set_item(list, idx, STACK_TV_BOT(idx - count));

		    if (count > 0)
			ectx.ec_stack.ga_len -= count - 1;
		    else if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    else
			++ectx.ec_stack.ga_len;
		    tv = STACK_TV_BOT(-1);
		    tv->v_type = VAR_LIST;
		    tv->vval.v_list = list;
		    ++list->lv_refcount;
		}
		break;

	    // create a dict from items on the stack
	    case ISN_NEWDICT:
		{
		    int	    count = iptr->isn_arg.number;
		    dict_T  *dict = dict_alloc();
		    dictitem_T *item;

		    if (dict == NULL)
			goto failed;
		    for (idx = 0; idx < count; ++idx)
		    {
			// check key type is VAR_STRING
			tv = STACK_TV_BOT(2 * (idx - count));
			item = dictitem_alloc(tv->vval.v_string);
			clear_tv(tv);
			if (item == NULL)
			    goto failed;
			item->di_tv = *STACK_TV_BOT(2 * (idx - count) + 1);
			item->di_tv.v_lock = 0;
			if (dict_add(dict, item) == FAIL)
			    goto failed;
		    }

		    if (count > 0)
			ectx.ec_stack.ga_len -= 2 * count - 1;
		    else if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    else
			++ectx.ec_stack.ga_len;
		    tv = STACK_TV_BOT(-1);
		    tv->v_type = VAR_DICT;
		    tv->vval.v_dict = dict;
		    ++dict->dv_refcount;
		}
		break;

	    // call a :def function
	    case ISN_DCALL:
		if (call_dfunc(iptr->isn_arg.dfunc.cdf_idx,
			      iptr->isn_arg.dfunc.cdf_argcount,
			      &ectx) == FAIL)
		    goto failed;
		break;

	    // call a builtin function
	    case ISN_BCALL:
		SOURCING_LNUM = iptr->isn_lnum;
		if (call_bfunc(iptr->isn_arg.bfunc.cbf_idx,
			      iptr->isn_arg.bfunc.cbf_argcount,
			      &ectx) == FAIL)
		    goto failed;
		break;

	    // call a funcref or partial
	    case ISN_PCALL:
		{
		    cpfunc_T	*pfunc = &iptr->isn_arg.pfunc;
		    int		r;
		    typval_T	partial;

		    SOURCING_LNUM = iptr->isn_lnum;
		    if (pfunc->cpf_top)
		    {
			// funcref is above the arguments
			tv = STACK_TV_BOT(-pfunc->cpf_argcount - 1);
		    }
		    else
		    {
			// Get the funcref from the stack.
			--ectx.ec_stack.ga_len;
			partial = *STACK_TV_BOT(0);
			tv = &partial;
		    }
		    r = call_partial(tv, pfunc->cpf_argcount, &ectx);
		    if (tv == &partial)
			clear_tv(&partial);
		    if (r == FAIL)
			goto failed;

		    if (pfunc->cpf_top)
		    {
			// Get the funcref from the stack, overwrite with the
			// return value.
			clear_tv(tv);
			--ectx.ec_stack.ga_len;
			*STACK_TV_BOT(-1) = *STACK_TV_BOT(0);
		    }
		}
		break;

	    // call a user defined function or funcref/partial
	    case ISN_UCALL:
		{
		    cufunc_T	*cufunc = &iptr->isn_arg.ufunc;

		    SOURCING_LNUM = iptr->isn_lnum;
		    if (call_eval_func(cufunc->cuf_name,
					  cufunc->cuf_argcount, &ectx) == FAIL)
			goto failed;
		}
		break;

	    // return from a :def function call
	    case ISN_RETURN:
		{
		    if (trycmd != NULL && trycmd->tcd_frame == ectx.ec_frame
			    && trycmd->tcd_finally_idx != 0)
		    {
			// jump to ":finally"
			ectx.ec_iidx = trycmd->tcd_finally_idx;
			trycmd->tcd_return = TRUE;
		    }
		    else
		    {
			// Restore previous function. If the frame pointer
			// is zero then there is none and we are done.
			if (ectx.ec_frame == initial_frame_ptr)
			    goto done;

			func_return(&ectx);
		    }
		}
		break;

	    // push a function reference to a compiled function
	    case ISN_FUNCREF:
		{
		    partial_T   *pt = NULL;

		    pt = ALLOC_CLEAR_ONE(partial_T);
		    if (pt == NULL)
			goto failed;
		    dfunc = ((dfunc_T *)def_functions.ga_data)
							+ iptr->isn_arg.number;
		    pt->pt_func = dfunc->df_ufunc;
		    pt->pt_refcount = 1;
		    ++dfunc->df_ufunc->uf_refcount;

		    if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    tv = STACK_TV_BOT(0);
		    ++ectx.ec_stack.ga_len;
		    tv->vval.v_partial = pt;
		    tv->v_type = VAR_PARTIAL;
		}
		break;

	    // jump if a condition is met
	    case ISN_JUMP:
		{
		    jumpwhen_T	when = iptr->isn_arg.jump.jump_when;
		    int		jump = TRUE;

		    if (when != JUMP_ALWAYS)
		    {
			tv = STACK_TV_BOT(-1);
			jump = tv2bool(tv);
			if (when == JUMP_IF_FALSE
					     || when == JUMP_AND_KEEP_IF_FALSE)
			    jump = !jump;
			if (when == JUMP_IF_FALSE || when == JUMP_IF_TRUE
								      || !jump)
			{
			    // drop the value from the stack
			    clear_tv(tv);
			    --ectx.ec_stack.ga_len;
			}
		    }
		    if (jump)
			ectx.ec_iidx = iptr->isn_arg.jump.jump_where;
		}
		break;

	    // top of a for loop
	    case ISN_FOR:
		{
		    list_T	*list = STACK_TV_BOT(-1)->vval.v_list;
		    typval_T	*idxtv =
				   STACK_TV_VAR(iptr->isn_arg.forloop.for_idx);

		    // push the next item from the list
		    if (ga_grow(&ectx.ec_stack, 1) == FAIL)
			goto failed;
		    if (++idxtv->vval.v_number >= list->lv_len)
			// past the end of the list, jump to "endfor"
			ectx.ec_iidx = iptr->isn_arg.forloop.for_end;
		    else if (list->lv_first == &range_list_item)
		    {
			// non-materialized range() list
			tv = STACK_TV_BOT(0);
			tv->v_type = VAR_NUMBER;
			tv->vval.v_number = list_find_nr(
					     list, idxtv->vval.v_number, NULL);
			++ectx.ec_stack.ga_len;
		    }
		    else
		    {
			listitem_T *li = list_find(list, idxtv->vval.v_number);

			if (li == NULL)
			    goto failed;
			copy_tv(&li->li_tv, STACK_TV_BOT(0));
			++ectx.ec_stack.ga_len;
		    }
		}
		break;

	    // start of ":try" block
	    case ISN_TRY:
		{
		    if (ga_grow(&ectx.ec_trystack, 1) == FAIL)
			goto failed;
		    trycmd = ((trycmd_T *)ectx.ec_trystack.ga_data)
						     + ectx.ec_trystack.ga_len;
		    ++ectx.ec_trystack.ga_len;
		    ++trylevel;
		    trycmd->tcd_frame = ectx.ec_frame;
		    trycmd->tcd_catch_idx = iptr->isn_arg.try.try_catch;
		    trycmd->tcd_finally_idx = iptr->isn_arg.try.try_finally;
		}
		break;

	    case ISN_PUSHEXC:
		if (current_exception == NULL)
		{
		    iemsg("Evaluating catch while current_exception is NULL");
		    goto failed;
		}
		if (ga_grow(&ectx.ec_stack, 1) == FAIL)
		    goto failed;
		tv = STACK_TV_BOT(0);
		++ectx.ec_stack.ga_len;
		tv->v_type = VAR_STRING;
		tv->vval.v_string = vim_strsave(
					   (char_u *)current_exception->value);
		break;

	    case ISN_CATCH:
		{
		    garray_T	*trystack = &ectx.ec_trystack;

		    if (trystack->ga_len > 0)
		    {
			trycmd = ((trycmd_T *)trystack->ga_data)
							+ trystack->ga_len - 1;
			trycmd->tcd_caught = TRUE;
		    }
		    did_emsg = got_int = did_throw = FALSE;
		    catch_exception(current_exception);
		}
		break;

	    // end of ":try" block
	    case ISN_ENDTRY:
		{
		    garray_T	*trystack = &ectx.ec_trystack;

		    if (trystack->ga_len > 0)
		    {
			--trystack->ga_len;
			--trylevel;
			trycmd = ((trycmd_T *)trystack->ga_data)
							    + trystack->ga_len;
			if (trycmd->tcd_caught)
			{
			    // discard the exception
			    if (caught_stack == current_exception)
				caught_stack = caught_stack->caught;
			    discard_current_exception();
			}

			if (trycmd->tcd_return)
			{
			    // Restore previous function. If the frame pointer
			    // is zero then there is none and we are done.
			    if (ectx.ec_frame == initial_frame_ptr)
				goto done;

			    func_return(&ectx);
			}
		    }
		}
		break;

	    case ISN_THROW:
		--ectx.ec_stack.ga_len;
		tv = STACK_TV_BOT(0);
		if (throw_exception(tv->vval.v_string, ET_USER, NULL) == FAIL)
		{
		    vim_free(tv->vval.v_string);
		    goto failed;
		}
		did_throw = TRUE;
		break;

	    // compare with special values
	    case ISN_COMPAREBOOL:
	    case ISN_COMPARESPECIAL:
		{
		    typval_T	*tv1 = STACK_TV_BOT(-2);
		    typval_T	*tv2 = STACK_TV_BOT(-1);
		    varnumber_T arg1 = tv1->vval.v_number;
		    varnumber_T arg2 = tv2->vval.v_number;
		    int		res;

		    switch (iptr->isn_arg.op.op_type)
		    {
			case EXPR_EQUAL: res = arg1 == arg2; break;
			case EXPR_NEQUAL: res = arg1 != arg2; break;
			default: res = 0; break;
		    }

		    --ectx.ec_stack.ga_len;
		    tv1->v_type = VAR_BOOL;
		    tv1->vval.v_number = res ? VVAL_TRUE : VVAL_FALSE;
		}
		break;

	    // Operation with two number arguments
	    case ISN_OPNR:
	    case ISN_COMPARENR:
		{
		    typval_T	*tv1 = STACK_TV_BOT(-2);
		    typval_T	*tv2 = STACK_TV_BOT(-1);
		    varnumber_T arg1 = tv1->vval.v_number;
		    varnumber_T arg2 = tv2->vval.v_number;
		    varnumber_T res;

		    switch (iptr->isn_arg.op.op_type)
		    {
			case EXPR_MULT: res = arg1 * arg2; break;
			case EXPR_DIV: res = arg1 / arg2; break;
			case EXPR_REM: res = arg1 % arg2; break;
			case EXPR_SUB: res = arg1 - arg2; break;
			case EXPR_ADD: res = arg1 + arg2; break;

			case EXPR_EQUAL: res = arg1 == arg2; break;
			case EXPR_NEQUAL: res = arg1 != arg2; break;
			case EXPR_GREATER: res = arg1 > arg2; break;
			case EXPR_GEQUAL: res = arg1 >= arg2; break;
			case EXPR_SMALLER: res = arg1 < arg2; break;
			case EXPR_SEQUAL: res = arg1 <= arg2; break;
			default: res = 0; break;
		    }

		    --ectx.ec_stack.ga_len;
		    if (iptr->isn_type == ISN_COMPARENR)
		    {
			tv1->v_type = VAR_BOOL;
			tv1->vval.v_number = res ? VVAL_TRUE : VVAL_FALSE;
		    }
		    else
			tv1->vval.v_number = res;
		}
		break;

	    // Computation with two float arguments
	    case ISN_OPFLOAT:
	    case ISN_COMPAREFLOAT:
		{
		    typval_T	*tv1 = STACK_TV_BOT(-2);
		    typval_T	*tv2 = STACK_TV_BOT(-1);
		    float_T	arg1 = tv1->vval.v_float;
		    float_T	arg2 = tv2->vval.v_float;
		    float_T	res = 0;
		    int		cmp = FALSE;

		    switch (iptr->isn_arg.op.op_type)
		    {
			case EXPR_MULT: res = arg1 * arg2; break;
			case EXPR_DIV: res = arg1 / arg2; break;
			case EXPR_SUB: res = arg1 - arg2; break;
			case EXPR_ADD: res = arg1 + arg2; break;

			case EXPR_EQUAL: cmp = arg1 == arg2; break;
			case EXPR_NEQUAL: cmp = arg1 != arg2; break;
			case EXPR_GREATER: cmp = arg1 > arg2; break;
			case EXPR_GEQUAL: cmp = arg1 >= arg2; break;
			case EXPR_SMALLER: cmp = arg1 < arg2; break;
			case EXPR_SEQUAL: cmp = arg1 <= arg2; break;
			default: cmp = 0; break;
		    }
		    --ectx.ec_stack.ga_len;
		    if (iptr->isn_type == ISN_COMPAREFLOAT)
		    {
			tv1->v_type = VAR_BOOL;
			tv1->vval.v_number = cmp ? VVAL_TRUE : VVAL_FALSE;
		    }
		    else
			tv1->vval.v_float = res;
		}
		break;

	    case ISN_COMPARELIST:
		{
		    typval_T	*tv1 = STACK_TV_BOT(-2);
		    typval_T	*tv2 = STACK_TV_BOT(-1);
		    list_T	*arg1 = tv1->vval.v_list;
		    list_T	*arg2 = tv2->vval.v_list;
		    int		cmp = FALSE;
		    int		ic = iptr->isn_arg.op.op_ic;

		    switch (iptr->isn_arg.op.op_type)
		    {
			case EXPR_EQUAL: cmp =
				      list_equal(arg1, arg2, ic, FALSE); break;
			case EXPR_NEQUAL: cmp =
				     !list_equal(arg1, arg2, ic, FALSE); break;
			case EXPR_IS: cmp = arg1 == arg2; break;
			case EXPR_ISNOT: cmp = arg1 != arg2; break;
			default: cmp = 0; break;
		    }
		    --ectx.ec_stack.ga_len;
		    clear_tv(tv1);
		    clear_tv(tv2);
		    tv1->v_type = VAR_BOOL;
		    tv1->vval.v_number = cmp ? VVAL_TRUE : VVAL_FALSE;
		}
		break;

	    case ISN_COMPAREBLOB:
		{
		    typval_T	*tv1 = STACK_TV_BOT(-2);
		    typval_T	*tv2 = STACK_TV_BOT(-1);
		    blob_T	*arg1 = tv1->vval.v_blob;
		    blob_T	*arg2 = tv2->vval.v_blob;
		    int		cmp = FALSE;

		    switch (iptr->isn_arg.op.op_type)
		    {
			case EXPR_EQUAL: cmp = blob_equal(arg1, arg2); break;
			case EXPR_NEQUAL: cmp = !blob_equal(arg1, arg2); break;
			case EXPR_IS: cmp = arg1 == arg2; break;
			case EXPR_ISNOT: cmp = arg1 != arg2; break;
			default: cmp = 0; break;
		    }
		    --ectx.ec_stack.ga_len;
		    clear_tv(tv1);
		    clear_tv(tv2);
		    tv1->v_type = VAR_BOOL;
		    tv1->vval.v_number = cmp ? VVAL_TRUE : VVAL_FALSE;
		}
		break;

		// TODO: handle separately
	    case ISN_COMPARESTRING:
	    case ISN_COMPAREDICT:
	    case ISN_COMPAREFUNC:
	    case ISN_COMPAREPARTIAL:
	    case ISN_COMPAREANY:
		{
		    typval_T	*tv1 = STACK_TV_BOT(-2);
		    typval_T	*tv2 = STACK_TV_BOT(-1);
		    exptype_T	exptype = iptr->isn_arg.op.op_type;
		    int		ic = iptr->isn_arg.op.op_ic;

		    typval_compare(tv1, tv2, exptype, ic);
		    clear_tv(tv2);
		    tv1->v_type = VAR_BOOL;
		    tv1->vval.v_number = tv1->vval.v_number
						      ? VVAL_TRUE : VVAL_FALSE;
		    --ectx.ec_stack.ga_len;
		}
		break;

	    case ISN_ADDLIST:
	    case ISN_ADDBLOB:
		{
		    typval_T *tv1 = STACK_TV_BOT(-2);
		    typval_T *tv2 = STACK_TV_BOT(-1);

		    if (iptr->isn_type == ISN_ADDLIST)
			eval_addlist(tv1, tv2);
		    else
			eval_addblob(tv1, tv2);
		    clear_tv(tv2);
		    --ectx.ec_stack.ga_len;
		}
		break;

	    // Computation with two arguments of unknown type
	    case ISN_OPANY:
		{
		    typval_T	*tv1 = STACK_TV_BOT(-2);
		    typval_T	*tv2 = STACK_TV_BOT(-1);
		    varnumber_T	n1, n2;
#ifdef FEAT_FLOAT
		    float_T	f1 = 0, f2 = 0;
#endif
		    int		error = FALSE;

		    if (iptr->isn_arg.op.op_type == EXPR_ADD)
		    {
			if (tv1->v_type == VAR_LIST && tv2->v_type == VAR_LIST)
			{
			    eval_addlist(tv1, tv2);
			    clear_tv(tv2);
			    --ectx.ec_stack.ga_len;
			    break;
			}
			else if (tv1->v_type == VAR_BLOB
						    && tv2->v_type == VAR_BLOB)
			{
			    eval_addblob(tv1, tv2);
			    clear_tv(tv2);
			    --ectx.ec_stack.ga_len;
			    break;
			}
		    }
#ifdef FEAT_FLOAT
		    if (tv1->v_type == VAR_FLOAT)
		    {
			f1 = tv1->vval.v_float;
			n1 = 0;
		    }
		    else
#endif
		    {
			n1 = tv_get_number_chk(tv1, &error);
			if (error)
			    goto failed;
#ifdef FEAT_FLOAT
			if (tv2->v_type == VAR_FLOAT)
			    f1 = n1;
#endif
		    }
#ifdef FEAT_FLOAT
		    if (tv2->v_type == VAR_FLOAT)
		    {
			f2 = tv2->vval.v_float;
			n2 = 0;
		    }
		    else
#endif
		    {
			n2 = tv_get_number_chk(tv2, &error);
			if (error)
			    goto failed;
#ifdef FEAT_FLOAT
			if (tv1->v_type == VAR_FLOAT)
			    f2 = n2;
#endif
		    }
#ifdef FEAT_FLOAT
		    // if there is a float on either side the result is a float
		    if (tv1->v_type == VAR_FLOAT || tv2->v_type == VAR_FLOAT)
		    {
			switch (iptr->isn_arg.op.op_type)
			{
			    case EXPR_MULT: f1 = f1 * f2; break;
			    case EXPR_DIV:  f1 = f1 / f2; break;
			    case EXPR_SUB:  f1 = f1 - f2; break;
			    case EXPR_ADD:  f1 = f1 + f2; break;
			    default: emsg(_(e_modulus)); goto failed;
			}
			clear_tv(tv1);
			clear_tv(tv2);
			tv1->v_type = VAR_FLOAT;
			tv1->vval.v_float = f1;
			--ectx.ec_stack.ga_len;
		    }
		    else
#endif
		    {
			switch (iptr->isn_arg.op.op_type)
			{
			    case EXPR_MULT: n1 = n1 * n2; break;
			    case EXPR_DIV:  n1 = num_divide(n1, n2); break;
			    case EXPR_SUB:  n1 = n1 - n2; break;
			    case EXPR_ADD:  n1 = n1 + n2; break;
			    default:	    n1 = num_modulus(n1, n2); break;
			}
			clear_tv(tv1);
			clear_tv(tv2);
			tv1->v_type = VAR_NUMBER;
			tv1->vval.v_number = n1;
			--ectx.ec_stack.ga_len;
		    }
		}
		break;

	    case ISN_CONCAT:
		{
		    char_u *str1 = STACK_TV_BOT(-2)->vval.v_string;
		    char_u *str2 = STACK_TV_BOT(-1)->vval.v_string;
		    char_u *res;

		    res = concat_str(str1, str2);
		    clear_tv(STACK_TV_BOT(-2));
		    clear_tv(STACK_TV_BOT(-1));
		    --ectx.ec_stack.ga_len;
		    STACK_TV_BOT(-1)->vval.v_string = res;
		}
		break;

	    case ISN_INDEX:
		{
		    list_T	*list;
		    varnumber_T	n;
		    listitem_T	*li;

		    // list index: list is at stack-2, index at stack-1
		    tv = STACK_TV_BOT(-2);
		    if (tv->v_type != VAR_LIST)
		    {
			emsg(_(e_listreq));
			goto failed;
		    }
		    list = tv->vval.v_list;

		    tv = STACK_TV_BOT(-1);
		    if (tv->v_type != VAR_NUMBER)
		    {
			emsg(_(e_number_exp));
			goto failed;
		    }
		    n = tv->vval.v_number;
		    clear_tv(tv);
		    if ((li = list_find(list, n)) == NULL)
		    {
			semsg(_(e_listidx), n);
			goto failed;
		    }
		    --ectx.ec_stack.ga_len;
		    clear_tv(STACK_TV_BOT(-1));
		    copy_tv(&li->li_tv, STACK_TV_BOT(-1));
		}
		break;

	    // dict member with string key
	    case ISN_MEMBER:
		{
		    dict_T	*dict;
		    dictitem_T	*di;

		    tv = STACK_TV_BOT(-1);
		    if (tv->v_type != VAR_DICT || tv->vval.v_dict == NULL)
		    {
			emsg(_(e_dictreq));
			goto failed;
		    }
		    dict = tv->vval.v_dict;

		    if ((di = dict_find(dict, iptr->isn_arg.string, -1))
								       == NULL)
		    {
			semsg(_(e_dictkey), iptr->isn_arg.string);
			goto failed;
		    }
		    clear_tv(tv);
		    copy_tv(&di->di_tv, tv);
		}
		break;

	    case ISN_NEGATENR:
		tv = STACK_TV_BOT(-1);
		tv->vval.v_number = -tv->vval.v_number;
		break;

	    case ISN_CHECKNR:
		{
		    int		error = FALSE;

		    tv = STACK_TV_BOT(-1);
		    if (check_not_string(tv) == FAIL)
		    {
			--ectx.ec_stack.ga_len;
			goto failed;
		    }
		    (void)tv_get_number_chk(tv, &error);
		    if (error)
			goto failed;
		}
		break;

	    case ISN_CHECKTYPE:
		{
		    checktype_T *ct = &iptr->isn_arg.type;

		    tv = STACK_TV_BOT(ct->ct_off);
		    if (tv->v_type != ct->ct_type)
		    {
			semsg(_("E1029: Expected %s but got %s"),
				    vartype_name(ct->ct_type),
				    vartype_name(tv->v_type));
			goto failed;
		    }
		}
		break;

	    case ISN_2BOOL:
		{
		    int n;

		    tv = STACK_TV_BOT(-1);
		    n = tv2bool(tv);
		    if (iptr->isn_arg.number)  // invert
			n = !n;
		    clear_tv(tv);
		    tv->v_type = VAR_BOOL;
		    tv->vval.v_number = n ? VVAL_TRUE : VVAL_FALSE;
		}
		break;

	    case ISN_2STRING:
		{
		    char_u *str;

		    tv = STACK_TV_BOT(iptr->isn_arg.number);
		    if (tv->v_type != VAR_STRING)
		    {
			str = typval_tostring(tv);
			clear_tv(tv);
			tv->v_type = VAR_STRING;
			tv->vval.v_string = str;
		    }
		}
		break;

	    case ISN_DROP:
		--ectx.ec_stack.ga_len;
		clear_tv(STACK_TV_BOT(0));
		break;
	}
    }

done:
    // function finished, get result from the stack.
    tv = STACK_TV_BOT(-1);
    *rettv = *tv;
    tv->v_type = VAR_UNKNOWN;
    ret = OK;

failed:
    for (idx = 0; idx < ectx.ec_stack.ga_len; ++idx)
	clear_tv(STACK_TV(idx));
    vim_free(ectx.ec_stack.ga_data);
    return ret;
}

#define DISASSEMBLE 1

/*
 * ":dissassemble".
 */
    void
ex_disassemble(exarg_T *eap)
{
#ifdef DISASSEMBLE
    ufunc_T	*ufunc = find_func(eap->arg, NULL);
    dfunc_T	*dfunc;
    isn_T	*instr;
    int		current;
    int		line_idx = 0;
    int		prev_current = 0;

    if (ufunc == NULL)
    {
	semsg("Cannot find function %s", eap->arg);
	return;
    }
    if (ufunc->uf_dfunc_idx < 0)
    {
	semsg("Function %s is not compiled", eap->arg);
	return;
    }
    if (ufunc->uf_name_exp != NULL)
	msg((char *)ufunc->uf_name_exp);
    else
	msg((char *)ufunc->uf_name);

    dfunc = ((dfunc_T *)def_functions.ga_data) + ufunc->uf_dfunc_idx;
    instr = dfunc->df_instr;
    for (current = 0; current < dfunc->df_instr_count; ++current)
    {
	isn_T	    *iptr = &instr[current];

	while (line_idx < iptr->isn_lnum && line_idx < ufunc->uf_lines.ga_len)
	{
	    if (current > prev_current)
	    {
		msg_puts("\n\n");
		prev_current = current;
	    }
	    msg(((char **)ufunc->uf_lines.ga_data)[line_idx++]);
	}

	switch (iptr->isn_type)
	{
	    case ISN_EXEC:
		smsg("%4d EXEC %s", current, iptr->isn_arg.string);
		break;
	    case ISN_ECHO:
		{
		    echo_T *echo = &iptr->isn_arg.echo;

		    smsg("%4d %s %d", current,
			    echo->echo_with_white ? "ECHO" : "ECHON",
			    echo->echo_count);
		}
		break;
	    case ISN_LOAD:
		if (iptr->isn_arg.number < 0)
		    smsg("%4d LOAD arg[%lld]", current,
				      iptr->isn_arg.number + STACK_FRAME_SIZE);
		else
		    smsg("%4d LOAD $%lld", current, iptr->isn_arg.number);
		break;
	    case ISN_LOADV:
		smsg("%4d LOADV v:%s", current,
				       get_vim_var_name(iptr->isn_arg.number));
		break;
	    case ISN_LOADSCRIPT:
		{
		    scriptitem_T *si =
				 &SCRIPT_ITEM(iptr->isn_arg.script.script_sid);
		    svar_T *sv = ((svar_T *)si->sn_var_vals.ga_data)
					     + iptr->isn_arg.script.script_idx;

		    smsg("%4d LOADSCRIPT %s from %s", current,
						     sv->sv_name, si->sn_name);
		}
		break;
	    case ISN_LOADS:
		{
		    scriptitem_T *si = &SCRIPT_ITEM(iptr->isn_arg.loads.ls_sid);

		    smsg("%4d LOADS s:%s from %s", current,
					    iptr->isn_arg.string, si->sn_name);
		}
		break;
	    case ISN_LOADG:
		smsg("%4d LOADG g:%s", current, iptr->isn_arg.string);
		break;
	    case ISN_LOADOPT:
		smsg("%4d LOADOPT %s", current, iptr->isn_arg.string);
		break;
	    case ISN_LOADENV:
		smsg("%4d LOADENV %s", current, iptr->isn_arg.string);
		break;
	    case ISN_LOADREG:
		smsg("%4d LOADREG @%c", current, iptr->isn_arg.number);
		break;

	    case ISN_STORE:
		smsg("%4d STORE $%lld", current, iptr->isn_arg.number);
		break;
	    case ISN_STOREG:
		smsg("%4d STOREG g:%s", current, iptr->isn_arg.string);
		break;
	    case ISN_STORESCRIPT:
		{
		    scriptitem_T *si =
				 &SCRIPT_ITEM(iptr->isn_arg.script.script_sid);
		    svar_T *sv = ((svar_T *)si->sn_var_vals.ga_data)
					     + iptr->isn_arg.script.script_idx;

		    smsg("%4d STORESCRIPT %s in %s", current,
						     sv->sv_name, si->sn_name);
		}
		break;
	    case ISN_STOREOPT:
		smsg("%4d STOREOPT &%s", current,
					       iptr->isn_arg.storeopt.so_name);
		break;

	    case ISN_STORENR:
		smsg("%4d STORE %lld in $%d", current,
				iptr->isn_arg.storenr.str_val,
				iptr->isn_arg.storenr.str_idx);
		break;

	    // constants
	    case ISN_PUSHNR:
		smsg("%4d PUSHNR %lld", current, iptr->isn_arg.number);
		break;
	    case ISN_PUSHBOOL:
	    case ISN_PUSHSPEC:
		smsg("%4d PUSH %s", current,
				   get_var_special_name(iptr->isn_arg.number));
		break;
	    case ISN_PUSHF:
		smsg("%4d PUSHF %g", current, iptr->isn_arg.fnumber);
		break;
	    case ISN_PUSHS:
		smsg("%4d PUSHS \"%s\"", current, iptr->isn_arg.string);
		break;
	    case ISN_PUSHBLOB:
		{
		    char_u	*r;
		    char_u	numbuf[NUMBUFLEN];
		    char_u	*tofree;

		    r = blob2string(iptr->isn_arg.blob, &tofree, numbuf);
		    smsg("%4d PUSHBLOB \"%s\"", current, r);
		    vim_free(tofree);
		}
		break;
	    case ISN_PUSHEXC:
		smsg("%4d PUSH v:exception", current);
		break;
	    case ISN_NEWLIST:
		smsg("%4d NEWLIST size %lld", current, iptr->isn_arg.number);
		break;
	    case ISN_NEWDICT:
		smsg("%4d NEWDICT size %lld", current, iptr->isn_arg.number);
		break;

	    // function call
	    case ISN_BCALL:
		{
		    cbfunc_T	*cbfunc = &iptr->isn_arg.bfunc;

		    smsg("%4d BCALL %s(argc %d)", current,
			    internal_func_name(cbfunc->cbf_idx),
			    cbfunc->cbf_argcount);
		}
		break;
	    case ISN_DCALL:
		{
		    cdfunc_T	*cdfunc = &iptr->isn_arg.dfunc;
		    dfunc_T	*df = ((dfunc_T *)def_functions.ga_data)
							     + cdfunc->cdf_idx;

		    smsg("%4d DCALL %s(argc %d)", current,
			    df->df_ufunc->uf_name_exp != NULL
				? df->df_ufunc->uf_name_exp
				: df->df_ufunc->uf_name, cdfunc->cdf_argcount);
		}
		break;
	    case ISN_UCALL:
		{
		    cufunc_T	*cufunc = &iptr->isn_arg.ufunc;

		    smsg("%4d UCALL %s(argc %d)", current,
				       cufunc->cuf_name, cufunc->cuf_argcount);
		}
		break;
	    case ISN_PCALL:
		{
		    cpfunc_T	*cpfunc = &iptr->isn_arg.pfunc;

		    smsg("%4d PCALL%s (argc %d)", current,
			   cpfunc->cpf_top ? " top" : "", cpfunc->cpf_argcount);
		}
		break;
	    case ISN_RETURN:
		smsg("%4d RETURN", current);
		break;
	    case ISN_FUNCREF:
		{
		    dfunc_T	*df = ((dfunc_T *)def_functions.ga_data)
							+ iptr->isn_arg.number;

		    smsg("%4d FUNCREF %s", current, df->df_ufunc->uf_name);
		}
		break;

	    case ISN_JUMP:
		{
		    char *when = "?";

		    switch (iptr->isn_arg.jump.jump_when)
		    {
			case JUMP_ALWAYS:
			    when = "JUMP";
			    break;
			case JUMP_IF_TRUE:
			    when = "JUMP_IF_TRUE";
			    break;
			case JUMP_AND_KEEP_IF_TRUE:
			    when = "JUMP_AND_KEEP_IF_TRUE";
			    break;
			case JUMP_IF_FALSE:
			    when = "JUMP_IF_FALSE";
			    break;
			case JUMP_AND_KEEP_IF_FALSE:
			    when = "JUMP_AND_KEEP_IF_FALSE";
			    break;
		    }
		    smsg("%4d %s -> %lld", current, when,
						iptr->isn_arg.jump.jump_where);
		}
		break;

	    case ISN_FOR:
		{
		    forloop_T *forloop = &iptr->isn_arg.forloop;

		    smsg("%4d FOR $%d -> %d", current,
					   forloop->for_idx, forloop->for_end);
		}
		break;

	    case ISN_TRY:
		{
		    try_T *try = &iptr->isn_arg.try;

		    smsg("%4d TRY catch -> %d, finally -> %d", current,
					     try->try_catch, try->try_finally);
		}
		break;
	    case ISN_CATCH:
		// TODO
		smsg("%4d CATCH", current);
		break;
	    case ISN_ENDTRY:
		smsg("%4d ENDTRY", current);
		break;
	    case ISN_THROW:
		smsg("%4d THROW", current);
		break;

	    // expression operations on number
	    case ISN_OPNR:
	    case ISN_OPFLOAT:
	    case ISN_OPANY:
		{
		    char *what;
		    char *ins;

		    switch (iptr->isn_arg.op.op_type)
		    {
			case EXPR_MULT: what = "*"; break;
			case EXPR_DIV: what = "/"; break;
			case EXPR_REM: what = "%"; break;
			case EXPR_SUB: what = "-"; break;
			case EXPR_ADD: what = "+"; break;
			default:       what = "???"; break;
		    }
		    switch (iptr->isn_type)
		    {
			case ISN_OPNR: ins = "OPNR"; break;
			case ISN_OPFLOAT: ins = "OPFLOAT"; break;
			case ISN_OPANY: ins = "OPANY"; break;
			default: ins = "???"; break;
		    }
		    smsg("%4d %s %s", current, ins, what);
		}
		break;

	    case ISN_COMPAREBOOL:
	    case ISN_COMPARESPECIAL:
	    case ISN_COMPARENR:
	    case ISN_COMPAREFLOAT:
	    case ISN_COMPARESTRING:
	    case ISN_COMPAREBLOB:
	    case ISN_COMPARELIST:
	    case ISN_COMPAREDICT:
	    case ISN_COMPAREFUNC:
	    case ISN_COMPAREPARTIAL:
	    case ISN_COMPAREANY:
		   {
		       char *p;
		       char buf[10];
		       char *type;

		       switch (iptr->isn_arg.op.op_type)
		       {
			   case EXPR_EQUAL:	 p = "=="; break;
			   case EXPR_NEQUAL:    p = "!="; break;
			   case EXPR_GREATER:   p = ">"; break;
			   case EXPR_GEQUAL:    p = ">="; break;
			   case EXPR_SMALLER:   p = "<"; break;
			   case EXPR_SEQUAL:    p = "<="; break;
			   case EXPR_MATCH:	 p = "=~"; break;
			   case EXPR_IS:	 p = "is"; break;
			   case EXPR_ISNOT:	 p = "isnot"; break;
			   case EXPR_NOMATCH:	 p = "!~"; break;
			   default:  p = "???"; break;
		       }
		       STRCPY(buf, p);
		       if (iptr->isn_arg.op.op_ic == TRUE)
			   strcat(buf, "?");
		       switch(iptr->isn_type)
		       {
			   case ISN_COMPAREBOOL: type = "COMPAREBOOL"; break;
			   case ISN_COMPARESPECIAL:
						 type = "COMPARESPECIAL"; break;
			   case ISN_COMPARENR: type = "COMPARENR"; break;
			   case ISN_COMPAREFLOAT: type = "COMPAREFLOAT"; break;
			   case ISN_COMPARESTRING:
						  type = "COMPARESTRING"; break;
			   case ISN_COMPAREBLOB: type = "COMPAREBLOB"; break;
			   case ISN_COMPARELIST: type = "COMPARELIST"; break;
			   case ISN_COMPAREDICT: type = "COMPAREDICT"; break;
			   case ISN_COMPAREFUNC: type = "COMPAREFUNC"; break;
			   case ISN_COMPAREPARTIAL:
						 type = "COMPAREPARTIAL"; break;
			   case ISN_COMPAREANY: type = "COMPAREANY"; break;
			   default: type = "???"; break;
		       }

		       smsg("%4d %s %s", current, type, buf);
		   }
		   break;

	    case ISN_ADDLIST: smsg("%4d ADDLIST", current); break;
	    case ISN_ADDBLOB: smsg("%4d ADDBLOB", current); break;

	    // expression operations
	    case ISN_CONCAT: smsg("%4d CONCAT", current); break;
	    case ISN_INDEX: smsg("%4d INDEX", current); break;
	    case ISN_MEMBER: smsg("%4d MEMBER %s", current,
						  iptr->isn_arg.string); break;
	    case ISN_NEGATENR: smsg("%4d NEGATENR", current); break;

	    case ISN_CHECKNR: smsg("%4d CHECKNR", current); break;
	    case ISN_CHECKTYPE: smsg("%4d CHECKTYPE %s stack[%d]", current,
				      vartype_name(iptr->isn_arg.type.ct_type),
				      iptr->isn_arg.type.ct_off);
				break;
	    case ISN_2BOOL: if (iptr->isn_arg.number)
				smsg("%4d INVERT (!val)", current);
			    else
				smsg("%4d 2BOOL (!!val)", current);
			    break;
	    case ISN_2STRING: smsg("%4d 2STRING stack[%d]", current,
							 iptr->isn_arg.number);
				break;

	    case ISN_DROP: smsg("%4d DROP", current); break;
	}
    }
#endif
}

/*
 * Return TRUE when "tv" is not falsey: non-zero, non-empty string, non-empty
 * list, etc.  Mostly like what JavaScript does, except that empty list and
 * empty dictionary are FALSE.
 */
    int
tv2bool(typval_T *tv)
{
    switch (tv->v_type)
    {
	case VAR_NUMBER:
	    return tv->vval.v_number != 0;
	case VAR_FLOAT:
#ifdef FEAT_FLOAT
	    return tv->vval.v_float != 0.0;
#else
	    break;
#endif
	case VAR_PARTIAL:
	    return tv->vval.v_partial != NULL;
	case VAR_FUNC:
	case VAR_STRING:
	    return tv->vval.v_string != NULL && *tv->vval.v_string != NUL;
	case VAR_LIST:
	    return tv->vval.v_list != NULL && tv->vval.v_list->lv_len > 0;
	case VAR_DICT:
	    return tv->vval.v_dict != NULL
				    && tv->vval.v_dict->dv_hashtab.ht_used > 0;
	case VAR_BOOL:
	case VAR_SPECIAL:
	    return tv->vval.v_number == VVAL_TRUE ? TRUE : FALSE;
	case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
	    return tv->vval.v_job != NULL;
#else
	    break;
#endif
	case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    return tv->vval.v_channel != NULL;
#else
	    break;
#endif
	case VAR_BLOB:
	    return tv->vval.v_blob != NULL && tv->vval.v_blob->bv_ga.ga_len > 0;
	case VAR_UNKNOWN:
	case VAR_VOID:
	    break;
    }
    return FALSE;
}

/*
 * If "tv" is a string give an error and return FAIL.
 */
    int
check_not_string(typval_T *tv)
{
    if (tv->v_type == VAR_STRING)
    {
	emsg(_("E1030: Using a String as a Number"));
	clear_tv(tv);
	return FAIL;
    }
    return OK;
}


#endif // FEAT_EVAL
