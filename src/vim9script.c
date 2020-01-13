/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9script.c: :vim9script, :import, :export and friends
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

#include "vim9.h"

/*
 * ":vim9script".
 */
    void
ex_vim9script(exarg_T *eap)
{
    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_("E1038: vim9script can only be used in a script"));
	return;
    }
    if (SCRIPT_ITEM(current_sctx.sc_sid).sn_had_command)
    {
	emsg(_("E1039: vim9script must be the first command in a script"));
	return;
    }
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;
    SCRIPT_ITEM(current_sctx.sc_sid).sn_had_command = TRUE;
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
