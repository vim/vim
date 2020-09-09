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

    int
in_vim9script(void)
{
    // Do not go up the stack, a ":function" inside vim9script uses legacy
    // syntax.  "sc_version" is also set when compiling a ":def" function in
    // legacy script.
    return current_sctx.sc_version == SCRIPT_VERSION_VIM9;
}

/*
 * ":vim9script".
 */
    void
ex_vim9script(exarg_T *eap)
{
    scriptitem_T    *si;

    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_(e_vim9script_can_only_be_used_in_script));
	return;
    }
    si = SCRIPT_ITEM(current_sctx.sc_sid);
    if (si->sn_had_command)
    {
	emsg(_(e_vim9script_must_be_first_command_in_script));
	return;
    }
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;
    si->sn_version = SCRIPT_VERSION_VIM9;
    si->sn_had_command = TRUE;

    if (STRCMP(p_cpo, CPO_VIM) != 0)
    {
	si->sn_save_cpo = p_cpo;
	p_cpo = vim_strsave((char_u *)CPO_VIM);
    }
}

/*
 * When in Vim9 script give an error and return FAIL.
 */
    int
not_in_vim9(exarg_T *eap)
{
    if (in_vim9script())
	switch (eap->cmdidx)
	{
	    case CMD_append:
	    case CMD_change:
	    case CMD_insert:
	    case CMD_t:
	    case CMD_xit:
		semsg(_(e_missing_let_str), eap->cmd);
		return FAIL;
	    default: break;
	}
    return OK;
}

/*
 * ":export let Name: type"
 * ":export const Name: type"
 * ":export def Name(..."
 * ":export class Name ..."
 */
    void
ex_export(exarg_T *eap)
{
    if (!in_vim9script())
    {
	emsg(_(e_export_can_only_be_used_in_vim9script));
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
		emsg(_(e_export_with_invalid_argument));
		is_export = FALSE;
	    }
	    break;
	default:
	    emsg(_(e_invalid_command_after_export));
	    break;
    }
}

/*
 * Add a new imported item entry to the current script.
 */
    static imported_T *
new_imported(garray_T *gap)
{
    if (ga_grow(gap, 1) == OK)
	return ((imported_T *)gap->ga_data + gap->ga_len++);
    return NULL;
}

/*
 * Free all imported items in script "sid".
 */
    void
free_imports(int sid)
{
    scriptitem_T    *si = SCRIPT_ITEM(sid);
    int		    idx;

    for (idx = 0; idx < si->sn_imports.ga_len; ++idx)
    {
	imported_T *imp = ((imported_T *)si->sn_imports.ga_data) + idx;

	vim_free(imp->imp_name);
    }
    ga_clear(&si->sn_imports);
    ga_clear(&si->sn_var_vals);
    clear_type_list(&si->sn_type_list);
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
    char_u	*cmd_end;
    evalarg_T	evalarg;

    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_(e_import_can_only_be_used_in_script));
	return;
    }
    fill_evalarg_from_eap(&evalarg, eap, eap->skip);

    cmd_end = handle_import(eap->arg, NULL, current_sctx.sc_sid,
							       &evalarg, NULL);
    if (cmd_end != NULL)
	eap->nextcmd = check_nextcmd(cmd_end);
    clear_evalarg(&evalarg, eap);
}

/*
 * Find an exported item in "sid" matching the name at "*argp".
 * When it is a variable return the index.
 * When it is a user function return "*ufunc".
 * When not found returns -1 and "*ufunc" is NULL.
 */
    int
find_exported(
	int	    sid,
	char_u	    *name,
	ufunc_T	    **ufunc,
	type_T	    **type)
{
    int		idx = -1;
    svar_T	*sv;
    scriptitem_T *script = SCRIPT_ITEM(sid);

    // find name in "script"
    // TODO: also find script-local user function
    idx = get_script_item_idx(sid, name, FALSE);
    if (idx >= 0)
    {
	sv = ((svar_T *)script->sn_var_vals.ga_data) + idx;
	if (!sv->sv_export)
	{
	    semsg(_(e_item_not_exported_in_script_str), name);
	    return -1;
	}
	*type = sv->sv_type;
	*ufunc = NULL;
    }
    else
    {
	char_u	buffer[200];
	char_u	*funcname;

	// it could be a user function.
	if (STRLEN(name) < sizeof(buffer) - 15)
	    funcname = buffer;
	else
	{
	    funcname = alloc(STRLEN(name) + 15);
	    if (funcname == NULL)
		return -1;
	}
	funcname[0] = K_SPECIAL;
	funcname[1] = KS_EXTRA;
	funcname[2] = (int)KE_SNR;
	sprintf((char *)funcname + 3, "%ld_%s", (long)sid, name);
	*ufunc = find_func(funcname, FALSE, NULL);
	if (funcname != buffer)
	    vim_free(funcname);

	if (*ufunc == NULL)
	{
	    semsg(_(e_item_not_found_in_script_str), name);
	    return -1;
	}
    }

    return idx;
}

/*
 * Handle an ":import" command and add the resulting imported_T to "gap", when
 * not NULL, or script "import_sid" sn_imports.
 * Returns a pointer to after the command or NULL in case of failure
 */
    char_u *
handle_import(
	char_u	    *arg_start,
	garray_T    *gap,
	int	    import_sid,
	evalarg_T   *evalarg,
	void	    *cctx)
{
    char_u	*arg = arg_start;
    char_u	*cmd_end = NULL;
    char_u	*as_name = NULL;
    int		ret = FAIL;
    typval_T	tv;
    int		sid = -1;
    int		res;
    garray_T	names;

    ga_init2(&names, sizeof(char_u *), 10);
    if (*arg == '{')
    {
	// "import {item, item} from ..."
	arg = skipwhite_and_linebreak(arg + 1, evalarg);
	for (;;)
	{
	    char_u  *p = arg;
	    int	    had_comma = FALSE;

	    while (eval_isnamec(*arg))
		++arg;
	    if (p == arg)
		break;
	    if (ga_grow(&names, 1) == FAIL)
		goto erret;
	    ((char_u **)names.ga_data)[names.ga_len] =
						      vim_strnsave(p, arg - p);
	    ++names.ga_len;
	    if (*arg == ',')
	    {
		had_comma = TRUE;
		++arg;
	    }
	    arg = skipwhite_and_linebreak(arg, evalarg);
	    if (*arg == '}')
	    {
		arg = skipwhite_and_linebreak(arg + 1, evalarg);
		break;
	    }
	    if (!had_comma)
	    {
		emsg(_(e_missing_comma_in_import));
		goto erret;
	    }
	}
	if (names.ga_len == 0)
	{
	    emsg(_(e_syntax_error_in_import));
	    goto erret;
	}
    }
    else
    {
	// "import Name from ..."
	// "import * as Name from ..."
	// "import item [as Name] from ..."
	arg = skipwhite_and_linebreak(arg, evalarg);
	if (arg[0] == '*' && IS_WHITE_OR_NUL(arg[1]))
	    arg = skipwhite_and_linebreak(arg + 1, evalarg);
	else if (eval_isnamec1(*arg))
	{
	    char_u  *p = arg;

	    while (eval_isnamec(*arg))
		++arg;
	    if (ga_grow(&names, 1) == FAIL)
		goto erret;
	    ((char_u **)names.ga_data)[names.ga_len] =
						      vim_strnsave(p, arg - p);
	    ++names.ga_len;
	    arg = skipwhite_and_linebreak(arg, evalarg);
	}
	else
	{
	    emsg(_(e_syntax_error_in_import));
	    goto erret;
	}

	if (STRNCMP("as", arg, 2) == 0 && IS_WHITE_OR_NUL(arg[2]))
	{
	    char_u *p;

	    // skip over "as Name "; no line break allowed after "as"
	    arg = skipwhite(arg + 2);
	    p = arg;
	    if (eval_isnamec1(*arg))
		while (eval_isnamec(*arg))
		    ++arg;
	    if (check_defined(p, arg - p, cctx) == FAIL)
		goto erret;
	    as_name = vim_strnsave(p, arg - p);
	    arg = skipwhite_and_linebreak(arg, evalarg);
	}
	else if (*arg_start == '*')
	{
	    emsg(_(e_missing_as_after_star));
	    goto erret;
	}
    }

    if (STRNCMP("from", arg, 4) != 0 || !IS_WHITE_OR_NUL(arg[4]))
    {
	emsg(_(e_missing_from));
	goto erret;
    }

    arg = skipwhite_and_linebreak(arg + 4, evalarg);
    tv.v_type = VAR_UNKNOWN;
    // TODO: should we accept any expression?
    if (*arg == '\'')
	ret = eval_lit_string(&arg, &tv, TRUE);
    else if (*arg == '"')
	ret = eval_string(&arg, &tv, TRUE);
    if (ret == FAIL || tv.vval.v_string == NULL || *tv.vval.v_string == NUL)
    {
	emsg(_(e_invalid_string_after_from));
	goto erret;
    }
    cmd_end = arg;

    /*
     * find script file
     */
    if (*tv.vval.v_string == '.')
    {
	size_t		len;
	scriptitem_T	*si = SCRIPT_ITEM(current_sctx.sc_sid);
	char_u		*tail = gettail(si->sn_name);
	char_u		*from_name;

	// Relative to current script: "./name.vim", "../../name.vim".
	len = STRLEN(si->sn_name) - STRLEN(tail) + STRLEN(tv.vval.v_string) + 2;
	from_name = alloc((int)len);
	if (from_name == NULL)
	{
	    clear_tv(&tv);
	    goto erret;
	}
	vim_strncpy(from_name, si->sn_name, tail - si->sn_name);
	add_pathsep(from_name);
	STRCAT(from_name, tv.vval.v_string);
	simplify_filename(from_name);

	res = do_source(from_name, FALSE, DOSO_NONE, &sid);
	vim_free(from_name);
    }
    else if (mch_isFullName(tv.vval.v_string))
    {
	// Absolute path: "/tmp/name.vim"
	res = do_source(tv.vval.v_string, FALSE, DOSO_NONE, &sid);
    }
    else
    {
	size_t	    len = 7 + STRLEN(tv.vval.v_string) + 1;
	char_u	    *from_name;

	// Find file in "import" subdirs in 'runtimepath'.
	from_name = alloc((int)len);
	if (from_name == NULL)
	{
	    clear_tv(&tv);
	    goto erret;
	}
	vim_snprintf((char *)from_name, len, "import/%s", tv.vval.v_string);
	res = source_in_path(p_rtp, from_name, DIP_NOAFTER, &sid);
	vim_free(from_name);
    }

    if (res == FAIL || sid <= 0)
    {
	semsg(_(e_could_not_import_str), tv.vval.v_string);
	clear_tv(&tv);
	goto erret;
    }
    clear_tv(&tv);

    if (*arg_start == '*')
    {
	imported_T *imported = new_imported(gap != NULL ? gap
					: &SCRIPT_ITEM(import_sid)->sn_imports);

	if (imported == NULL)
	    goto erret;
	imported->imp_name = as_name;
	as_name = NULL;
	imported->imp_sid = sid;
	imported->imp_all = TRUE;
    }
    else
    {
	int i;

	arg = arg_start;
	if (*arg == '{')
	    arg = skipwhite(arg + 1);
	for (i = 0; i < names.ga_len; ++i)
	{
	    char_u	*name = ((char_u **)names.ga_data)[i];
	    int		idx;
	    imported_T	*imported;
	    ufunc_T	*ufunc = NULL;
	    type_T	*type;

	    idx = find_exported(sid, name, &ufunc, &type);

	    if (idx < 0 && ufunc == NULL)
		goto erret;

	    if (check_defined(name, STRLEN(name), cctx) == FAIL)
		goto erret;

	    imported = new_imported(gap != NULL ? gap
				       : &SCRIPT_ITEM(import_sid)->sn_imports);
	    if (imported == NULL)
		goto erret;

	    // TODO: check for "as" following
	    // imported->imp_name = vim_strsave(as_name);
	    imported->imp_name = name;
	    ((char_u **)names.ga_data)[i] = NULL;
	    imported->imp_sid = sid;
	    if (idx >= 0)
	    {
		imported->imp_type = type;
		imported->imp_var_vals_idx = idx;
	    }
	    else
	    {
		imported->imp_type = ufunc->uf_func_type;
		imported->imp_funcname = ufunc->uf_name;
	    }
	}
    }
erret:
    ga_clear_strings(&names);
    vim_free(as_name);
    return cmd_end;
}

/*
 * Declare a script-local variable without init: "let var: type".
 * "const" is an error since the value is missing.
 * Returns a pointer to after the type.
 */
    char_u *
vim9_declare_scriptvar(exarg_T *eap, char_u *arg)
{
    char_u	    *p;
    char_u	    *name;
    scriptitem_T    *si = SCRIPT_ITEM(current_sctx.sc_sid);
    type_T	    *type;
    int		    called_emsg_before = called_emsg;
    typval_T	    init_tv;

    if (eap->cmdidx == CMD_const)
    {
	emsg(_(e_const_requires_a_value));
	return arg + STRLEN(arg);
    }

    // Check for valid starting character.
    if (!eval_isnamec1(*arg))
    {
	semsg(_(e_invarg2), arg);
	return arg + STRLEN(arg);
    }

    for (p = arg + 1; *p != NUL && eval_isnamec(*p); MB_PTR_ADV(p))
	if (*p == ':' && (VIM_ISWHITE(p[1]) || p != arg + 1))
	    break;

    if (*p != ':')
    {
	emsg(_(e_type_or_initialization_required));
	return arg + STRLEN(arg);
    }
    if (!VIM_ISWHITE(p[1]))
    {
	semsg(_(e_white_space_required_after_str), ":");
	return arg + STRLEN(arg);
    }
    name = vim_strnsave(arg, p - arg);

    // parse type
    p = skipwhite(p + 1);
    type = parse_type(&p, &si->sn_type_list);
    if (called_emsg != called_emsg_before)
    {
	vim_free(name);
	return p;
    }

    // Create the variable with 0/NULL value.
    CLEAR_FIELD(init_tv);
    init_tv.v_type = type->tt_type;
    set_var_const(name, type, &init_tv, FALSE, 0);

    vim_free(name);
    return p;
}

/*
 * Check if the type of script variable "dest" allows assigning "value".
 */
    int
check_script_var_type(typval_T *dest, typval_T *value, char_u *name)
{
    scriptitem_T    *si = SCRIPT_ITEM(current_sctx.sc_sid);
    int		    idx;

    if (si->sn_version != SCRIPT_VERSION_VIM9)
	// legacy script doesn't store variable types
	return OK;

    // Find the svar_T in sn_var_vals.
    for (idx = 0; idx < si->sn_var_vals.ga_len; ++idx)
    {
	svar_T    *sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;

	if (sv->sv_tv == dest)
	{
	    if (sv->sv_const)
	    {
		semsg(_(e_readonlyvar), name);
		return FAIL;
	    }
	    return check_typval_type(sv->sv_type, value, 0);
	}
    }
    iemsg("check_script_var_type(): not found");
    return OK; // not really
}

#endif // FEAT_EVAL
