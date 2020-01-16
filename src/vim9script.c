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
 * Add a new imported item entry to the current script.
 */
    static imported_T *
new_imported(void)
{
    scriptitem_T *si = &SCRIPT_ITEM(current_sctx.sc_sid);

    // Store a pointer to the typval_T, so that it can be found by
    // index instead of using a hastab lookup.
    if (ga_grow(&si->sn_imports, 1) == OK)
    {
	imported_T *imp = ((imported_T *)si->sn_imports.ga_data
						      + si->sn_imports.ga_len);

	++si->sn_imports.ga_len;
	return imp;
    }
    return NULL;
}

/*
 * ":import Item from 'filename'"
 * ":import Item as Alias from 'filename'"
 * ":import {Item} from 'filename'".
 * ":import {Item as Alias} from 'filename'"
 * ":import {Item, Item} from 'filename'"
 * ":import {Item, Item as Alias} from 'filename'"
 *
 * ":import * as Name from 'filename'"
 */
    void
ex_import(exarg_T *eap)
{
    char_u	*arg = eap->arg;
    char_u	*as_ptr = NULL;
    char_u	*from_ptr;
    int		as_len = 0;
    int		ret = FAIL;
    typval_T	tv;
    int		sid = -1;

    if (current_sctx.sc_version != SCRIPT_VERSION_VIM9)
    {
	emsg(_(e_needs_vim9));
	return;
    }

    if (*arg == '{')
    {
	// skip over {item} list
	while (*arg != NUL && *arg != '}')
	    ++arg;
	if (*arg == '}')
	    arg = skipwhite(arg + 1);
    }
    else
    {
	if (*arg == '*')
	    arg = skipwhite(arg + 1);
	else
	{
	    while (eval_isnamec1(*arg))
		++arg;
	    arg = skipwhite(arg);
	}
	if (STRNCMP("as", arg, 2) == 0 && VIM_ISWHITE(arg[2]))
	{
	    // skip over "as Name "
	    arg = skipwhite(arg + 2);
	    as_ptr = arg;
	    while (eval_isnamec1(*arg))
		++arg;
	    as_len = (int)(arg - as_ptr);
	    arg = skipwhite(arg);
	}
	else if (*eap->arg == '*')
	{
	    emsg(_("E1045: Missing \"as\" after *"));
	    return;
	}
    }
    if (STRNCMP("from", arg, 4) != 0 || !VIM_ISWHITE(arg[4]))
    {
	emsg(_("E1045: Missing \"from\""));
	return;
    }
    from_ptr = arg;
    arg = skipwhite(arg + 4);
    tv.v_type = VAR_UNKNOWN;
    if (*arg == '\'')
	ret = get_lit_string_tv(&arg, &tv, TRUE);
    else if (*arg == '"')
	ret = get_string_tv(&arg, &tv, TRUE);
    if (ret == FAIL || tv.vval.v_string == NULL || *tv.vval.v_string == NUL)
    {
	emsg(_("E1045: Invalid string after \"from\""));
	return;
    }
    eap->nextcmd = check_nextcmd(arg);

    // find script tv.vval.v_string
    if (*tv.vval.v_string == '.')
    {
	size_t		len;
	scriptitem_T	*si = &SCRIPT_ITEM(current_sctx.sc_sid);
	char_u		*tail = gettail(si->sn_name);
	char_u		*from_name;
	int		res;

	// Relative to current script: "./name.vim", "../../name.vim".
	len = STRLEN(si->sn_name) - STRLEN(tail) + STRLEN(tv.vval.v_string) + 2;
	from_name = alloc((int)len);
	if (from_name == NULL)
	{
	    clear_tv(&tv);
	    return;
	}
	vim_strncpy(from_name, si->sn_name, tail - si->sn_name);
	add_pathsep(from_name);
	STRCAT(from_name, tv.vval.v_string);

	sid = -1;
	res = do_source(from_name, FALSE, DOSO_NONE, &sid);
	vim_free(from_name);
	if (res == FAIL || sid <= 0)
	    return;
    }
    else
    {
	emsg("Searching for import in 'runtimepath' not supported yet");
	return;
    }
    clear_tv(&tv);

    if (*eap->arg == '*')
    {
	imported_T *imported = new_imported();

	if (imported == NULL)
	    return;
	imported->imp_name = vim_strnsave(as_ptr, as_len);
	imported->imp_sid = sid;
	imported->imp_all = TRUE;
    }
    else
    {
	scriptitem_T *script = &SCRIPT_ITEM(sid);

	arg = eap->arg;
	if (*arg == '{')
	    arg = skipwhite(arg + 1);
	for (;;)
	{
	    char_u	*name = arg;
	    int		name_len;
	    int		cc;
	    int		idx;
	    svar_T	*sv;
	    imported_T *imported;

	    // isolate one name
	    while (eval_isnamec1(*arg))
		++arg;
	    name_len = (int)(arg - name);

	    // find name in "script"
	    // TODO: also find script-local user function
	    cc = *arg;
	    *arg = NUL;
	    idx = get_script_item_idx(sid, name);
	    if (idx < 0)
	    {
		semsg(_("E1048: Item not found in script: %s"), name);
		*arg = cc;
		return;
	    }
	    sv = ((svar_T *)script->sn_var_vals.ga_data) + idx;
	    if (!sv->sv_export)
	    {
		semsg(_("E1049: Item not exported in script: %s"), name);
		*arg = cc;
		return;
	    }

	    imported = new_imported();
	    if (imported == NULL)
		return;

	    *arg = cc;
	    arg = skipwhite(arg);

	    // TODO: check for "as" following
	    // imported->imp_name = vim_strnsave(as_ptr, as_len);
	    imported->imp_name = vim_strnsave(name, name_len);
	    imported->imp_sid = sid;
	    imported->imp_type = sv->sv_type;
	    imported->imp_var_vals_idx = idx;

	    arg = skipwhite(arg);
	    if (*eap->arg != '{' || *arg == '}')
		break;
	    if (*arg != ',')
	    {
		emsg(_("E1046: Missing comma in import"));
		return;
	    }
	    arg = skipwhite(arg + 1);
	}
	if (arg != from_ptr)
	{
	    emsg(_("E1047: syntax error in import"));
	    return;
	}
    }
}

#endif // FEAT_EVAL
