/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * ECMAScript interface by Bob Pepin and Prabir Shrestha
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#include "duktape/duktape.h"

static void vduk_pushtypval(duk_context *ctx, typval_T *tv) {
    if (tv == NULL)
    {
        duk_push_null(ctx);
        return;
    }
    switch (tv->v_type)
    {
        case VAR_STRING:
	    duk_push_string(ctx, (const char*)tv->vval.v_string);
            break;
        case VAR_NUMBER:
	    duk_push_int(ctx, tv->vval.v_number);
            break;
        default:
            duk_push_null(ctx);
    }
}

typval_T vduk_get_typval(duk_context *ctx, duk_idx_t idx) {
    typval_T tv;
    tv.v_lock = VAR_FIXED;
    if(duk_is_string(ctx, idx)) {
	tv.v_type = VAR_STRING;
	tv.vval.v_string = (char_u*)duk_get_string(ctx, idx);
    } else if(duk_is_number(ctx, idx)) {
	tv.v_type = VAR_NUMBER;
	tv.vval.v_number = duk_get_int(ctx, idx);
    } else {
	tv.v_type = VAR_UNKNOWN;
    }
    return tv;
}

duk_ret_t vduk_vimcall_func(duk_context *ctx) {
    char_u *name = (char_u*)duk_to_string(ctx, -2);
    int argcount = duk_get_length(ctx, -1);
    typval_T *argvars = (typval_T*)alloc(argcount * sizeof(typval_T));
    for(duk_uarridx_t i=0; i < argcount; i++) {
	duk_get_prop_index(ctx, -1, i);
	argvars[i] = vduk_get_typval(ctx, -1);
	duk_pop(ctx);
    }
    typval_T rettv;
    rettv.v_type = VAR_NUMBER;
    call_internal_func(name, argcount, argvars, &rettv);
    duk_pop_2(ctx);
    duk_ret_t retcount = 0;
    switch(rettv.v_type) {
    case VAR_STRING:
	duk_push_string(ctx, (const char*)rettv.vval.v_string);
	retcount = 1;
	break;
    case VAR_NUMBER:
	duk_push_int(ctx, rettv.vval.v_number);
	retcount = 1;
	break;
    default:
	break;
    }
    clear_tv(&rettv);
    return retcount;
}

duk_ret_t vduk_vimcmd_func(duk_context *ctx) {
    int r = do_cmdline_cmd((char_u*)duk_to_string(ctx, -1));
    duk_pop(ctx);
    duk_push_boolean(ctx, r == OK);
    return 1;
}

duk_ret_t vduk_vimeval_func(duk_context *ctx) {
    typval_T *tv = eval_expr((char_u*)duk_to_string(ctx, -1), NULL);
    duk_pop(ctx);
    vduk_pushtypval(ctx, tv);
    free_tv(tv);
    return 1;
}

duk_context *vduk_get_context() {
    static duk_context *ctx = NULL;
    if(ctx) return ctx;
    ctx = duk_create_heap_default();

    duk_push_global_object(ctx);

    duk_push_c_lightfunc(ctx, vduk_vimcall_func, 2, 2, 0);
    duk_put_prop_string(ctx, -2, "__vimcall");

    duk_push_c_lightfunc(ctx, vduk_vimcmd_func, 1, 1, 0);
    duk_put_prop_string(ctx, -2, "__vimcmd");

    duk_push_c_lightfunc(ctx, vduk_vimeval_func, 1, 1, 0);
    duk_put_prop_string(ctx, -2, "__vimeval");

    duk_pop(ctx);

    return ctx;
}

/* ex commands */
    void
ex_ecmascript(exarg_T *eap)
{
    duk_context *ctx = vduk_get_context();
    if (!ctx) {
	semsg("ecmascript: Failed to get ecmascript context");
	return;
    }

    char_u *script;
    script = script_get(eap, eap->arg);

    char *evalstr = script == NULL ? (char *) eap->arg : (char *) script;
    duk_eval_string(ctx, evalstr);
    if (duk_peval_string(ctx, evalstr) != 0) {
	semsg("ecmascript: %s", duk_safe_to_string(ctx, -1));
    }
    duk_pop(ctx);
    if (script != NULL) vim_free(script);
}
