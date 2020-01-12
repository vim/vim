/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9script.c: :namespace, :import, :export and friends
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

#include "vim9.h"

/*
 * ":namespace".
 */
    void
ex_namespace(exarg_T *eap)
{
    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_("E1038: namespace can only be used in a script"));
	return;
    }
    if (current_sctx.sc_had_command)
    {
	emsg(_("E1039: namespace must be the first command in a script"));
	return;
    }
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;
}

/*
 * ":export".
 */
    void
ex_export(exarg_T *eap UNUSED)
{
    // TODO
    emsg(":export not implemented yet");
}

/*
 * ":import".
 */
    void
ex_import(exarg_T *eap UNUSED)
{
    // TODO
    emsg(":import not implemented yet");
}

#endif // FEAT_EVAL
