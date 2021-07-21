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

#if defined(FEAT_EVAL)
# include "vim9.h"
#endif

/*
 * Return TRUE when currently using Vim9 script syntax.
 * Does not go up the stack, a ":function" inside vim9script uses legacy
 * syntax.
 */
    int
in_vim9script(void)
{
    // "sc_version" is also set when compiling a ":def" function in legacy
    // script.
    return (current_sctx.sc_version == SCRIPT_VERSION_VIM9
					 || (cmdmod.cmod_flags & CMOD_VIM9CMD))
		&& !(cmdmod.cmod_flags & CMOD_LEGACY);
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return TRUE if the current script is Vim9 script.
 * This also returns TRUE in a legacy function in a Vim9 script.
 */
    int
current_script_is_vim9(void)
{
    return SCRIPT_ID_VALID(current_sctx.sc_sid)
	    && SCRIPT_ITEM(current_sctx.sc_sid)->sn_version
						       == SCRIPT_VERSION_VIM9;
}
#endif

/*
 * ":vim9script".
 */
    void
ex_vim9script(exarg_T *eap UNUSED)
{
#ifdef FEAT_EVAL
    int		    sid = current_sctx.sc_sid;
    scriptitem_T    *si;

    if (!getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_(e_vim9script_can_only_be_used_in_script));
	return;
    }

    si = SCRIPT_ITEM(sid);
    if (si->sn_state == SN_STATE_HAD_COMMAND)
    {
	emsg(_(e_vim9script_must_be_first_command_in_script));
	return;
    }
    if (!IS_WHITE_OR_NUL(*eap->arg) && STRCMP(eap->arg, "noclear") != 0)
    {
	semsg(_(e_invarg2), eap->arg);
	return;
    }
    if (si->sn_state == SN_STATE_RELOAD && IS_WHITE_OR_NUL(*eap->arg))
    {
	hashtab_T	*ht = &SCRIPT_VARS(sid);

	// Reloading a script without the "noclear" argument: clear
	// script-local variables and functions.
	hashtab_free_contents(ht);
	hash_init(ht);
	delete_script_functions(sid);

	// old imports and script variables are no longer valid
	free_imports_and_script_vars(sid);
    }
    si->sn_state = SN_STATE_HAD_COMMAND;

    current_sctx.sc_version = SCRIPT_VERSION_VIM9;
    si->sn_version = SCRIPT_VERSION_VIM9;

    if (STRCMP(p_cpo, CPO_VIM) != 0)
    {
	si->sn_save_cpo = vim_strsave(p_cpo);
	set_option_value((char_u *)"cpo", 0L, (char_u *)CPO_VIM, OPT_NO_REDRAW);
    }
#else
    // No check for this being the first command, it doesn't matter.
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;
#endif
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
	    case CMD_k:
		if (eap->addr_count > 0)
		{
		    emsg(_(e_norange));
		    return FAIL;
		}
		// FALLTHROUGH
	    case CMD_append:
	    case CMD_change:
	    case CMD_insert:
	    case CMD_open:
	    case CMD_t:
	    case CMD_xit:
		semsg(_(e_command_not_supported_in_vim9_script_missing_var_str), eap->cmd);
		return FAIL;
	    default: break;
	}
    return OK;
}

/*
 * Give an error message if "p" points at "#{" and return TRUE.
 * This avoids that using a legacy style #{} dictionary leads to difficult to
 * understand errors.
 */
    int
vim9_bad_comment(char_u *p)
{
    if (p[0] == '#' && p[1] == '{' && p[2] != '{')
    {
	emsg(_(e_cannot_use_hash_curly_to_start_comment));
	return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if "p" points at a "#" not followed by one '{'.
 * Does not check for white space.
 */
    int
vim9_comment_start(char_u *p)
{
    return p[0] == '#' && (p[1] != '{' || p[2] == '{');
}

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * "++nr" and "--nr" commands.
 */
    void
ex_incdec(exarg_T *eap)
{
    char_u	*cmd = eap->cmd;
    char_u	*nextcmd = eap->nextcmd;
    size_t	len = STRLEN(eap->cmd) + 8;

    if (VIM_ISWHITE(cmd[2]))
    {
	semsg(_(e_no_white_space_allowed_after_str_str),
			 eap->cmdidx == CMD_increment ? "++" : "--", eap->cmd);
	return;
    }

    // This works like "nr += 1" or "nr -= 1".
    // Add a '|' to avoid looking in the next line.
    eap->cmd = alloc(len);
    if (eap->cmd == NULL)
	return;
    vim_snprintf((char *)eap->cmd, len, "%s %c= 1 |", cmd + 2,
				     eap->cmdidx == CMD_increment ? '+' : '-');
    eap->arg = eap->cmd;
    eap->cmdidx = CMD_var;
    eap->nextcmd = NULL;
    ex_let(eap);
    vim_free(eap->cmd);
    eap->cmd = cmd;
    eap->nextcmd = nextcmd;
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
    (void)find_ex_command(eap, NULL, lookup_scriptitem, NULL);
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
 * Mark all imports as possible to redefine.  Used when a script is loaded
 * again but not cleared.
 */
    void
mark_imports_for_reload(int sid)
{
    scriptitem_T    *si = SCRIPT_ITEM(sid);
    int		    idx;

    for (idx = 0; idx < si->sn_imports.ga_len; ++idx)
    {
	imported_T *imp = ((imported_T *)si->sn_imports.ga_data) + idx;

	imp->imp_flags |= IMP_FLAGS_RELOAD;
    }
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
	cctx_T	    *cctx,
	int	    verbose)
{
    int		idx = -1;
    svar_T	*sv;
    scriptitem_T *script = SCRIPT_ITEM(sid);

    // Find name in "script".
    idx = get_script_item_idx(sid, name, 0, cctx);
    if (idx >= 0)
    {
	sv = ((svar_T *)script->sn_var_vals.ga_data) + idx;
	if (!sv->sv_export)
	{
	    if (verbose)
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
	    if (verbose)
		semsg(_(e_item_not_found_in_script_str), name);
	    return -1;
	}
	else if (((*ufunc)->uf_flags & FC_EXPORT) == 0)
	{
	    if (verbose)
		semsg(_(e_item_not_exported_in_script_str), name);
	    *ufunc = NULL;
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
    int		ret = FAIL;
    typval_T	tv;
    int		sid = -1;
    int		res;
    int		mult = FALSE;
    garray_T	names;
    garray_T	as_names;

    tv.v_type = VAR_UNKNOWN;
    ga_init2(&names, sizeof(char_u *), 10);
    ga_init2(&as_names, sizeof(char_u *), 10);
    if (*arg == '{')
    {
	// "import {item, item} from ..."
	mult = TRUE;
	arg = skipwhite_and_linebreak(arg + 1, evalarg);
    }

    for (;;)
    {
	char_u	    *p = arg;
	int	    had_comma = FALSE;
	char_u	    *as_name = NULL;

	// accept "*" or "Name"
	if (!mult && arg[0] == '*' && IS_WHITE_OR_NUL(arg[1]))
	    ++arg;
	else
	    while (eval_isnamec(*arg))
		++arg;
	if (p == arg)
	    break;
	if (ga_grow(&names, 1) == FAIL || ga_grow(&as_names, 1) == FAIL)
	    goto erret;
	((char_u **)names.ga_data)[names.ga_len] = vim_strnsave(p, arg - p);
	++names.ga_len;

	arg = skipwhite_and_linebreak(arg, evalarg);
	if (STRNCMP("as", arg, 2) == 0 && IS_WHITE_OR_NUL(arg[2]))
	{
	    // skip over "as Name "; no line break allowed after "as"
	    arg = skipwhite(arg + 2);
	    p = arg;
	    if (eval_isnamec1(*arg))
		while (eval_isnamec(*arg))
		    ++arg;
	    if (check_defined(p, arg - p, cctx, FALSE) == FAIL)
		goto erret;
	    as_name = vim_strnsave(p, arg - p);
	    arg = skipwhite_and_linebreak(arg, evalarg);
	}
	else if (*arg_start == '*')
	{
	    emsg(_(e_missing_as_after_star));
	    goto erret;
	}
	// without "as Name" the as_names entry is NULL
	((char_u **)as_names.ga_data)[as_names.ga_len] = as_name;
	++as_names.ga_len;

	if (!mult)
	    break;
	if (*arg == ',')
	{
	    had_comma = TRUE;
	    ++arg;
	}
	arg = skipwhite_and_linebreak(arg, evalarg);
	if (*arg == '}')
	{
	    ++arg;
	    break;
	}
	if (!had_comma)
	{
	    emsg(_(e_missing_comma_in_import));
	    goto erret;
	}
    }
    arg = skipwhite_and_linebreak(arg, evalarg);

    if (names.ga_len == 0)
    {
	emsg(_(e_syntax_error_in_import));
	goto erret;
    }

    if (STRNCMP("from", arg, 4) != 0 || !IS_WHITE_OR_NUL(arg[4]))
    {
	emsg(_(e_missing_from));
	goto erret;
    }

    // The name of the file can be an expression, which must evaluate to a
    // string.
    arg = skipwhite_and_linebreak(arg + 4, evalarg);
    ret = eval0(arg, &tv, NULL, evalarg);
    if (ret == FAIL)
	goto erret;
    if (tv.v_type != VAR_STRING
		       || tv.vval.v_string == NULL || *tv.vval.v_string == NUL)
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
	    goto erret;
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
	    goto erret;
	}
	vim_snprintf((char *)from_name, len, "import/%s", tv.vval.v_string);
	res = source_in_path(p_rtp, from_name, DIP_NOAFTER, &sid);
	vim_free(from_name);
    }

    if (res == FAIL || sid <= 0)
    {
	semsg(_(e_could_not_import_str), tv.vval.v_string);
	goto erret;
    }

    if (*arg_start == '*')
    {
	imported_T  *imported;
	char_u	    *as_name = ((char_u **)as_names.ga_data)[0];

	// "import * as That"
	imported = find_imported(as_name, STRLEN(as_name), cctx);
	if (imported != NULL && imported->imp_sid == sid)
	{
	    if (imported->imp_flags & IMP_FLAGS_RELOAD)
		// import already defined on a previous script load
		imported->imp_flags &= ~IMP_FLAGS_RELOAD;
	    else
	    {
		semsg(_(e_name_already_defined_str), as_name);
		goto erret;
	    }
	}

	imported = new_imported(gap != NULL ? gap
					: &SCRIPT_ITEM(import_sid)->sn_imports);
	if (imported == NULL)
	    goto erret;
	imported->imp_name = as_name;
	((char_u **)as_names.ga_data)[0] = NULL;
	imported->imp_sid = sid;
	imported->imp_flags = IMP_FLAGS_STAR;
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
	    char_u	*as_name = ((char_u **)as_names.ga_data)[i];
	    size_t	len = STRLEN(name);
	    int		idx;
	    imported_T	*imported;
	    ufunc_T	*ufunc = NULL;
	    type_T	*type;

	    idx = find_exported(sid, name, &ufunc, &type, cctx, TRUE);

	    if (idx < 0 && ufunc == NULL)
		goto erret;

	    // If already imported with the same properties and the
	    // IMP_FLAGS_RELOAD set then we keep that entry.  Otherwise create
	    // a new one (and give an error for an existing import).
	    imported = find_imported(name, len, cctx);
	    if (imported != NULL
		    && (imported->imp_flags & IMP_FLAGS_RELOAD)
		    && imported->imp_sid == sid
		    && (idx >= 0
			? (equal_type(imported->imp_type, type)
			    && imported->imp_var_vals_idx == idx)
			: (equal_type(imported->imp_type, ufunc->uf_func_type)
			    && STRCMP(imported->imp_funcname,
							ufunc->uf_name) == 0)))
	    {
		imported->imp_flags &= ~IMP_FLAGS_RELOAD;
	    }
	    else
	    {
		if (as_name == NULL
			      && check_defined(name, len, cctx, FALSE) == FAIL)
		    goto erret;

		imported = new_imported(gap != NULL ? gap
				       : &SCRIPT_ITEM(import_sid)->sn_imports);
		if (imported == NULL)
		    goto erret;

		if (as_name == NULL)
		{
		    imported->imp_name = name;
		    ((char_u **)names.ga_data)[i] = NULL;
		}
		else
		{
		    // "import This as That ..."
		    imported->imp_name = as_name;
		    ((char_u **)as_names.ga_data)[i] = NULL;
		}
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
    }
erret:
    clear_tv(&tv);
    ga_clear_strings(&names);
    ga_clear_strings(&as_names);
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
	semsg(_(e_white_space_required_after_str_str), ":", p);
	return arg + STRLEN(arg);
    }
    name = vim_strnsave(arg, p - arg);

    // parse type, check for reserved name
    p = skipwhite(p + 1);
    type = parse_type(&p, &si->sn_type_list, TRUE);
    if (type == NULL || check_reserved_name(name) == FAIL)
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
    set_var_const(name, type, &init_tv, FALSE, 0, 0);

    vim_free(name);
    return p;
}

/*
 * Vim9 part of adding a script variable: add it to sn_all_vars (lookup by name
 * with a hashtable) and sn_var_vals (lookup by index).
 * When "create" is TRUE this is a new variable, otherwise find and update an
 * existing variable.
 * "flags" can have ASSIGN_FINAL or ASSIGN_CONST.
 * When "*type" is NULL use "tv" for the type and update "*type".  If
 * "do_member" is TRUE also use the member type, otherwise use "any".
 */
    void
update_vim9_script_var(
	int	    create,
	dictitem_T  *di,
	int	    flags,
	typval_T    *tv,
	type_T	    **type,
	int	    do_member)
{
    scriptitem_T    *si = SCRIPT_ITEM(current_sctx.sc_sid);
    hashitem_T	    *hi;
    svar_T	    *sv;

    if (create)
    {
	sallvar_T	    *newsav;

	// Store a pointer to the typval_T, so that it can be found by index
	// instead of using a hastab lookup.
	if (ga_grow(&si->sn_var_vals, 1) == FAIL)
	    return;

	sv = ((svar_T *)si->sn_var_vals.ga_data) + si->sn_var_vals.ga_len;
	newsav = (sallvar_T *)alloc_clear(
				       sizeof(sallvar_T) + STRLEN(di->di_key));
	if (newsav == NULL)
	    return;

	sv->sv_tv = &di->di_tv;
	sv->sv_const = (flags & ASSIGN_FINAL) ? ASSIGN_FINAL
				   : (flags & ASSIGN_CONST) ? ASSIGN_CONST : 0;
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
    }
    else
    {
	sv = find_typval_in_script(&di->di_tv);
    }
    if (sv != NULL)
    {
	if (*type == NULL)
	    *type = typval2type(tv, get_copyID(), &si->sn_type_list,
								    do_member);
	sv->sv_type = *type;
    }

    // let ex_export() know the export worked.
    is_export = FALSE;
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

    // existing commands using script variable indexes are no longer valid
    si->sn_script_seq = current_sctx.sc_seq;
}

/*
 * Find the script-local variable that links to "dest".
 * Returns NULL if not found and give an internal error.
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
    iemsg("find_typval_in_script(): not found");
    return NULL;
}

/*
 * Check if the type of script variable "dest" allows assigning "value".
 * If needed convert "value" to a bool.
 */
    int
check_script_var_type(
	typval_T    *dest,
	typval_T    *value,
	char_u	    *name,
	where_T	    where)
{
    svar_T  *sv = find_typval_in_script(dest);
    int	    ret;

    if (sv != NULL)
    {
	if (sv->sv_const != 0)
	{
	    semsg(_(e_cannot_change_readonly_variable_str), name);
	    return FAIL;
	}
	ret = check_typval_type(sv->sv_type, value, where);
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

// words that cannot be used as a variable
static char *reserved[] = {
    "true",
    "false",
    "null",
    "this",
    NULL
};

    int
check_reserved_name(char_u *name)
{
    int idx;

    for (idx = 0; reserved[idx] != NULL; ++idx)
	if (STRCMP(reserved[idx], name) == 0)
	{
	    semsg(_(e_cannot_use_reserved_name), name);
	    return FAIL;
	}
    return OK;
}

#endif // FEAT_EVAL
