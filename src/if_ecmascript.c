/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * ECMAScript interface by Prabir Shrestha
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#include "duktape/duktape.h"

duk_context *vduk_get_context() {
    static duk_context *ctx = NULL;
    if(ctx) return ctx;
    ctx = duk_create_heap_default();

    duk_push_global_object(ctx);

    duk_idx_t obj_idx_vim;

    /* js: vim = {} */
    duk_push_string(ctx, "vim");
    obj_idx_vim = duk_push_object(ctx);
    duk_put_prop(ctx, 0);
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
    } else {
	smsg("ecmascript: %s", duk_safe_to_string(ctx, -1));
    }
    duk_pop(ctx);
    if (script != NULL) vim_free(script);
}
