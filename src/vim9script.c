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
	si->sn_save_cpo = vim_strsave(p_cpo);
	set_option_value((char_u *)"cpo", 0L, (char_u *)CPO_VIM, 0);
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
		semsg(_(e_missing_var_str), eap->cmd);
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
	case CMD_var:
	case CMD_final:
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
free_imports_and_script_vars(int sid)
{
    scriptitem_T    *si = SCRIPT_ITEM(sid);
    int		    idx;

    for (idx = 0; idx < si->sn_imports.ga_len; ++idx)
    {
	imported_T *imp = ((imported_T *)si->sn_imports.ga_data) + idx;

	vim_free(imp->imp_name);
    }
    ga_clear(&si->sn_imports);

    free_all_script_vars(si);

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
	type_T	    **type,
	cctx_T	    *cctx)
{
    int		idx = -1;
    svar_T	*sv;
    scriptitem_T *script = SCRIPT_ITEM(sid);

    // find name in "script"
    // TODO: also find script-local user function
    idx = get_script_item_idx(sid, name, FALSE, cctx);
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
 * "cctx" is NULL at the script level.
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

	    idx = find_exported(sid, name, &ufunc, &type, cctx);

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
    typval_T	    init_tv;

    if (eap->cmdidx == CMD_final || eap->cmdidx == CMD_const)
    {
	if (eap->cmdidx == CMD_final)
	    emsg(_(e_final_requires_a_value));
	else
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
    type = parse_type(&p, &si->sn_type_list, TRUE);
    if (type == NULL)
    {
	vim_free(name);
	return p;
    }

    // Create the variable with 0/NULL value.
    CLEAR_FIELD(init_tv);
    if (type->tt_type == VAR_ANY)
	// A variable of type "any" is not possible, just use zero instead
	init_tv.v_type = VAR_NUMBER;
    else
	init_tv.v_type = type->tt_type;
    set_var_const(name, type, &init_tv, FALSE, 0);

    vim_free(name);
    return p;
}

/*
 * Vim9 part of adding a script variable: add it to sn_all_vars (lookup by name
 * with a hashtable) and sn_var_vals (lookup by index).
 * When "type" is NULL use "tv" for the type.
 */
    void
add_vim9_script_var(dictitem_T *di, typval_T *tv, type_T *type)
{
    scriptitem_T *si = SCRIPT_ITEM(current_sctx.sc_sid);

    // Store a pointer to the typval_T, so that it can be found by
    // index instead of using a hastab lookup.
    if (ga_grow(&si->sn_var_vals, 1) == OK)
    {
	svar_T *sv = ((svar_T *)si->sn_var_vals.ga_data)
						      + si->sn_var_vals.ga_len;
	hashitem_T *hi;
	sallvar_T *newsav = (sallvar_T *)alloc_clear(
				       sizeof(sallvar_T) + STRLEN(di->di_key));

	if (newsav == NULL)
	    return;

	sv->sv_tv = &di->di_tv;
	if (type == NULL)
	    sv->sv_type = typval2type(tv, &si->sn_type_list);
	else
	    sv->sv_type = type;
	sv->sv_const = (di->di_flags & DI_FLAGS_LOCK) ? ASSIGN_CONST : 0;
	sv->sv_export = is_export;
	newsav->sav_var_vals_idx = si->sn_var_vals.ga_len;
	++si->sn_var_vals.ga_len;

	STRCPY(&newsav->sav_key, di->di_key);
	sv->sv_name = newsav->sav_key;
	newsav->sav_di = di;
	newsav->sav_block_id = si->sn_current_block_id;

	hi = hash_find(&si->sn_all_vars.dv_hashtab, newsav->sav_key);
	if (!HASHITEM_EMPTY(hi))
	{
	    sallvar_T *sav = HI2SAV(hi);

	    // variable with this name exists in another block
	    while (sav->sav_next != NULL)
		sav = sav->sav_next;
	    sav->sav_next = newsav;
	}
	else
	    // new variable name
	    hash_add(&si->sn_all_vars.dv_hashtab, newsav->sav_key);

	// let ex_export() know the export worked.
	is_export = FALSE;
    }
}

/*
 * Hide a script variable when leaving a block.
 * "idx" is de index in sn_var_vals.
 * When "func_defined" is non-zero then a function was defined in this block,
 * the variable may be accessed by it.  Otherwise the variable can be cleared.
 */
    void
hide_script_var(scriptitem_T *si, int idx, int func_defined)
{
    svar_T	*sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;
    hashtab_T	*script_ht = get_script_local_ht();
    hashtab_T	*all_ht = &si->sn_all_vars.dv_hashtab;
    hashitem_T	*script_hi;
    hashitem_T	*all_hi;

    // Remove a variable declared inside the block, if it still exists.
    // If it was added in a nested block it will already have been removed.
    // The typval is moved into the sallvar_T.
    script_hi = hash_find(script_ht, sv->sv_name);
    all_hi = hash_find(all_ht, sv->sv_name);
    if (!HASHITEM_EMPTY(script_hi) && !HASHITEM_EMPTY(all_hi))
    {
	dictitem_T	*di = HI2DI(script_hi);
	sallvar_T	*sav = HI2SAV(all_hi);
	sallvar_T	*sav_prev = NULL;

	// There can be multiple entries with the same name in different
	// blocks, find the right one.
	while (sav != NULL && sav->sav_var_vals_idx != idx)
	{
	    sav_prev = sav;
	    sav = sav->sav_next;
	}
	if (sav != NULL)
	{
	    if (func_defined)
	    {
		// move the typval from the dictitem to the sallvar
		sav->sav_tv = di->di_tv;
		di->di_tv.v_type = VAR_UNKNOWN;
		sav->sav_flags = di->di_flags;
		sav->sav_di = NULL;
		sv->sv_tv = &sav->sav_tv;
	    }
	    else
	    {
		if (sav_prev == NULL)
		    hash_remove(all_ht, all_hi);
		else
		    sav_prev->sav_next = sav->sav_next;
		sv->sv_name = NULL;
		vim_free(sav);
	    }
	    delete_var(script_ht, script_hi);
	}
    }
}

/*
 * Free the script variables from "sn_all_vars".
 */
    void
free_all_script_vars(scriptitem_T *si)
{
    int		todo;
    hashtab_T	*ht = &si->sn_all_vars.dv_hashtab;
    hashitem_T	*hi;
    sallvar_T	*sav;
    sallvar_T	*sav_next;

    hash_lock(ht);
    todo = (int)ht->ht_used;
    for (hi = ht->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;

	    // Free the variable.  Don't remove it from the hashtab, ht_array
	    // might change then.  hash_clear() takes care of it later.
	    sav = HI2SAV(hi);
	    while (sav != NULL)
	    {
		sav_next = sav->sav_next;
		if (sav->sav_di == NULL)
		    clear_tv(&sav->sav_tv);
		vim_free(sav);
		sav = sav_next;
	    }
	}
    }
    hash_clear(ht);
    hash_init(ht);

    ga_clear(&si->sn_var_vals);
}

/*
 * Find the script-local variable that links to "dest".
 * Returns NULL if not found.
 */
    svar_T *
find_typval_in_script(typval_T *dest)
{
    scriptitem_T    *si = SCRIPT_ITEM(current_sctx.sc_sid);
    int		    idx;

    if (si->sn_version != SCRIPT_VERSION_VIM9)
	// legacy script doesn't store variable types
	return NULL;

    // Find the svar_T in sn_var_vals.
    for (idx = 0; idx < si->sn_var_vals.ga_len; ++idx)
    {
	svar_T    *sv = ((svar_T *)si->sn_var_vals.ga_data) + idx;

	// If "sv_name" is NULL the variable was hidden when leaving a block,
	// don't check "sv_tv" then, it might be used for another variable now.
	if (sv->sv_name != NULL && sv->sv_tv == dest)
	    return sv;
    }
    iemsg("check_script_var_type(): not found");
    return NULL;
}

/*
 * Check if the type of script variable "dest" allows assigning "value".
 * If needed convert "value" to a bool.
 */
    int
check_script_var_type(typval_T *dest, typval_T *value, char_u *name)
{
    svar_T  *sv = find_typval_in_script(dest);
    int	    ret;

    if (sv != NULL)
    {
	if (sv->sv_const)
	{
	    semsg(_(e_readonlyvar), name);
	    return FAIL;
	}
	ret = check_typval_type(sv->sv_type, value, 0);
	if (ret == OK && need_convert_to_bool(sv->sv_type, value))
	{
	    int	val = tv2bool(value);

	    clear_tv(value);
	    value->v_type = VAR_BOOL;
	    value->v_lock = 0;
	    value->vval.v_number = val ? VVAL_TRUE : VVAL_FALSE;
	}
	return ret;
    }

    return OK; // not really
}

#endif // FEAT_EVAL
