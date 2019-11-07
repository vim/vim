/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Lua interface by Prabir Shrestha
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#include "duktape/duktape.h"

/* ex commands */
    void
ex_ecmascript(exarg_T *eap)
{
    char *script;
    duk_context *ctx = duk_create_heap_default();
    if (!ctx) return;

    script = (char *) script_get(eap, eap->arg);

    // TODO

    if (script != NULL) vim_free(script);
}
