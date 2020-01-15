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

static char e_needs_vim9[] = N_("E1042: import/export can only be used in vim9script");

    int
in_vim9script(void)
{
    // TODO: go up the stack
    return current_sctx.sc_version == SCRIPT_VERSION_VIM9;
}

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
 * ":export let Name: type"
 * ":export const Name: type"
 * ":export def Name(..."
 * ":export class Name ..."
 *
 * ":export {Name, ...}"
 */
    void
ex_export(exarg_T *eap UNUSED)
{
    if (current_sctx.sc_version != SCRIPT_VERSION_VIM9)
    {
	emsg(_(e_needs_vim9));
	return;
    }

    eap->cmd = eap->arg;
    (void)find_ex_command(eap, NULL, lookup_scriptvar, NULL);
    switch (eap->cmdidx)
    {
	case CMD_let:
	case CMD_const:
	case CMD_def:
	// case CMD_class:
	    is_export = TRUE;
	    do_cmdline(eap->cmd, eap->getline, eap->cookie,
						DOCMD_VERBOSE + DOCMD_NOWAIT);

	    // The command will reset "is_export" when exporting an item.
	    if (is_export)
	    {
		emsg(_("E1044: export with invalid argument"));
		is_export = FALSE;
	    }
	    break;
	default:
	    emsg(_("E1043: Invalid command after :export"));
	    break;
    }
}

/*
 * ":import Item from 'filename'"
 * ":import {Item} from 'filename'".
 * ":import {Item as Alias} from 'filename'"
 * ":import {Item, Item} from 'filename'"
 * ":import {Item, Item as Alias} from 'filename'"
 *
 * ":import * as Name from 'filename'"
 */
    void
ex_import(exarg_T *eap UNUSED)
{
    if (current_sctx.sc_version != SCRIPT_VERSION_VIM9)
    {
	emsg(_(e_needs_vim9));
	return;
    }

    // TODO
    emsg(":import not implemented yet");
}

#endif // FEAT_EVAL
