/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9generics.c: Vim9 script generics support
 */

#include "vim.h"

#if defined(FEAT_EVAL)


/*
 * A hash table is used to lookup a generic function with specific types.
 * The specific type names are used as the key.
 */
typedef struct gfitem_S gfitem_T;
struct gfitem_S
{
    ufunc_T	*gfi_ufunc;
    char_u	gfi_name[1];	// actually longer
};
#define GFITEM_KEY_OFF	offsetof(gfitem_T, gfi_name)
#define HI2GFITEM(hi)	((gfitem_T *)((hi)->hi_key - GFITEM_KEY_OFF))

static type_T *find_generic_type_in_cctx(char_u *gt_name, size_t len, cctx_T *cctx);

/*
 * Returns a pointer to the first '<' character in "name" that starts the
 * generic type argument list, skipping an initial <SNR> or <lambda> prefix if
 * present.  The prefix is only skipped if "name" starts with '<'.
 *
 * Returns NULL if no '<' is found before a '(' or the end of the string.
 * The returned pointer refers to the original string.
 *
 * Examples:
 *   "<SNR>123_Fn<number>"    -> returns pointer to '<'
 *   "<lambda>123_Fn<number>" -> returns pointer to '<'
 *   "Func<number>"           -> returns pointer to '<'
 *   "Func()"                 -> returns NULL
 */
    char_u *
generic_func_find_open_bracket(char_u *name)
{
    char_u	*p = name;

    if (name[0] == '<')
    {
	// Skip the <SNR> or <lambda> at the start of the name
	if (STRNCMP(name + 1, "SNR>", 4) == 0)
	    p += 5;
	else if (STRNCMP(name + 1, "lambda>", 7) == 0)
	    p += 8;
    }

    while (*p && *p != '(' && *p != '<')
	p++;

    if (*p == '<')
	return p;

    return NULL;
}

/*
 * Finds the matching '>' character for a generic function or class type
 * parameter or argument list, starting from the opening '<'.
 *
 * Enforces correct syntax for a flat, comma-separated list of types:
 * - No whitespace before or after type names or commas
 * - Each type must be non-empty and separated by a comma and whitespace
 * - At least one type must be present
 *
 * Arguments:
 *   start - pointer to the opening '<'
 *
 * Returns:
 *   Pointer to the matching '>' character if found and syntax is valid,
 *   or NULL if not found, invalid syntax, or on error.
 */
    static char_u *
generic_find_close_bracket(char_u *start)
{
    char_u	*p = start + 1;
    int		type_count = 0;

    while (*p && *p != '>')
    {
	char_u	*typename = p;

	if (VIM_ISWHITE(*p))
	{
	    char tmpstr[2];
	    tmpstr[0] = *(p - 1); tmpstr[1] = NUL;
	    semsg(_(e_no_white_space_allowed_after_str_str), tmpstr, start);
	    return NULL;
	}

	if (ASCII_ISALNUM(*p))
	    p = skip_type(p, FALSE);
	if (p == typename)
	{
	    char_u cc = *p;
	    *p = NUL;
	    semsg(_(e_missing_type_after_str), start);
	    *p = cc;
	    return NULL;
	}
	type_count++;

	if (*p == '>' || *p == NUL)
	    break;

	if (VIM_ISWHITE(*p))
	{
	    char_u cc = *p;
	    *p = NUL;
	    semsg(_(e_no_white_space_allowed_after_str_str), typename, start);
	    *p = cc;
	    return NULL;
	}

	if (*p != ',')
	{
	    semsg(_(e_missing_comma_in_generic_str), start);
	    return NULL;
	}
	p++;

	if (*p == NUL)
	    break;

	if (!VIM_ISWHITE(*p))
	{
	    semsg(_(e_white_space_required_after_str_str), ",", start);
	    return NULL;
	}
	p = skipwhite(p);
    }

    if (*p != '>')
    {
	semsg(_(e_missing_closing_angle_bracket_in_generic_str), start);
	return NULL;
    }

    char_u *after = skipwhite(p + 1);
    if (after > p + 1 && (*after == '(' || *after == '.'))
    {
	// white space not allowed between '>' and '(' or '.'
	semsg(_(e_no_white_space_allowed_after_str_str), ">", start);
	return NULL;
    }

    if (type_count == 0)
    {
	semsg(_(e_empty_type_list_for_generic_str), start);
	return NULL;
    }

    return p;
}

/*
 * Advances the argument pointer past the type argument list of a generic
 * function or class.
 *
 * On entry, "*argp" must point to the opening '<' of a generic type argument
 * list.  This function finds the matching closing '>' (validating the syntax
 * via generic_find_close_bracket), and if successful, advances "*argp" to
 * the character immediately after the closing '>'.
 *
 * Returns OK on success, or FAIL if the type argument list is invalid or no
 * matching '>' is found. On failure, "*argp" is not modified.
 */
    int
skip_generic_type_args(char_u **argp)
{
    char_u *p = generic_find_close_bracket(*argp);
    if (p == NULL)
	return FAIL;

    *argp = p + 1;	// skip '>'

    return OK;
}

/*
 * Appends the generic function type arguments, starting at "*argp", to the
 * function name "funcname" (of length "namelen") and returns a newly allocated
 * string containing the result.
 *
 * On entry, "*argp" must point to the opening '<' of the generic type argument
 * list.  If the type argument list is valid, the substring from "*argp" up to
 * and including the matching '>' is appended to "funcname". On success,
 * "*argp" is updated to point to the character after the closing '>'.
 *
 * Returns:
 *   A newly allocated string with the combined function name and type
 *   arguments, or NULL if there is a syntax error in the generic type
 *   arguments.
 *
 * The caller is responsible for freeing the returned string.
 */
    char_u *
append_generic_func_type_args(
    char_u	*funcname,
    size_t	namelen,
    char_u	**argp)
{
    char_u	*p = generic_find_close_bracket(*argp);
    size_t	argslen;
    char_u	*name;

    if (p == NULL)
	return NULL;

    // The type arguments can be of any length, do not use IObuff.
    argslen = (size_t)(p - *argp) + 1;
    name = alloc(namelen + argslen + 1);
    if (name == NULL)
	return NULL;
    mch_memmove(name, funcname, namelen);
    mch_memmove(name + namelen, *argp, argslen);
    name[namelen + argslen] = NUL;

    *argp = p + 1;

    return name;
}

/*
 * Returns a newly allocated string containing the function name from "fp" with
 * the generic type arguments from "*argp" appended.
 *
 * On entry, "*argp" must point to the opening '<' of the generic type argument
 * list.  On success, "*argp" is advanced to the character after the closing
 * '>'.
 *
 * Returns:
 *   A newly allocated string with the combined function name and type
 *   arguments, or NULL if "fp" is not a generic function, if there is a
 *   parsing error, or on memory allocation failure.
 *
 * The caller is responsible for freeing the returned string.
 */
    char_u *
get_generic_func_name(ufunc_T *fp, char_u **argp)
{
    if (!IS_GENERIC_FUNC(fp))
    {
	emsg_funcname(e_not_a_generic_function_str, fp->uf_name);
	return NULL;
    }

    return append_generic_func_type_args(fp->uf_name, fp->uf_namelen, argp);
}

/*
 * Parses the concrete type arguments provided for a generic function or
 * class, starting at the opening '<' character and ending at the matching '>'.
 *
 * On entry, "start" must point to the opening '<' character.
 * On success, returns a pointer to the character after the closing '>'.
 * On failure, returns NULL and reports an error message.
 *
 * Arguments:
 *   name      - the name of the function or class (used for error messages)
 *   namelen   - length of the name
 *   start     - pointer to the opening '<' character in the call
 *   gatab     - args table to allocate new type objects and to store parsed
 *               type argument names and their types.
 *   cctx      - compile context for type resolution (may be NULL)
 *
 * This function enforces correct syntax for generic type argument lists,
 * including whitespace rules, comma separation, and non-empty argument lists.
 */
    char_u *
parse_generic_type_args(
    char_u		*name,
    size_t		namelen,
    char_u		*start,
    generic_args_tab_T	*gatab,
    cctx_T		*cctx)
{
    generic_T	*generic_arg;
    type_T	*type_arg;
    char_u	*p = start;

    // White spaces not allowed after '<'
    if (VIM_ISWHITE(*(p + 1)))
    {
	semsg(_(e_no_white_space_allowed_after_str_str), "<", p);
	return NULL;
    }

    ++p;	// skip the '<'

    // parse each type argument until '>' or end of string
    while (*p && *p != '>')
    {
	p = skipwhite(p);

	if (!ASCII_ISALNUM(*p))
	{
	    semsg(_(e_missing_type_after_str), start);
	    return NULL;
	}

	// parse the type
	type_arg = parse_type(&p, &gatab->gat_arg_types, cctx, TRUE);
	if (type_arg == NULL || !valid_declaration_type(type_arg))
	    return NULL;

	char	*ret_free = NULL;
	char	*ret_name = type_name(type_arg, &ret_free);

	// create space for the name and the new type
	if (ga_grow(&gatab->gat_args, 1) == FAIL)
	{
	    vim_free(ret_free);
	    return NULL;
	}
	generic_arg = (generic_T *)gatab->gat_args.ga_data +
						gatab->gat_args.ga_len;

	// copy the type name and add the new type
	generic_arg->gt_name = vim_strsave((char_u *)ret_name);
	vim_free(ret_free);
	if (generic_arg->gt_name == NULL)
	    return NULL;
	generic_arg->gt_type = type_arg;
	gatab->gat_args.ga_len++;

	p = skipwhite(p);

	if (*p == NUL || *p == '>')
	    break;

	// after a type, expect ',' or '>'
	if (*p != ',')
	{
	    semsg(_(e_missing_comma_in_generic_str), start);
	    return NULL;
	}

	if (*(p + 1) == NUL)
	    break;

	// Require whitespace after a comma and skip it
	if (!VIM_ISWHITE(*(p + 1)))
	{
	    semsg(_(e_white_space_required_after_str_str), ",", p);
	    return NULL;
	}
	p++;
    }

    // ensure the list of types ends in a closing '>'
    if (*p != '>')
    {
	semsg(_(e_missing_closing_angle_bracket_in_generic_str), name);
	return NULL;
    }

    // no whitespace allowed before '>'
    if (VIM_ISWHITE(*(p - 1)))
    {
	semsg(_(e_no_white_space_allowed_before_str_str), ">", p);
	return NULL;
    }

    // at least one type argument is required
    if (generic_args_table_size(gatab) == 0)
    {
	char_u	cc = name[namelen];
	name[namelen] = NUL;
	semsg(_(e_empty_type_list_for_generic_str), name);
	name[namelen] = cc;
	return NULL;
    }
    ++p;	// skip the '>'

    return p;
}

/*
 * Checks if a generic type name already exists in the current context.
 *
 * This function verifies that the given generic type name "gt_name" does not
 * conflict with an imported variable, an existing generic type in "gatab", a
 * generic type in the current or outer compile context "cctx" or a generic
 * type of the class being defined. If a conflict is found, an appropriate
 * error message is reported.
 *
 * Arguments:
 *   gt_name  - the generic type name to check
 *   name_len - the length of "gt_name"
 *   gatab    - args table to allocate new type objects and to store parsed
 *              type argument names and their types.
 *   cctx     - current compile context, used to check for outer generic
 *              types (may be NULL)
 *
 * Returns:
 *   TRUE if the name already exists or conflicts, FALSE otherwise.
 */
    static int
generic_name_exists(
    char_u		*gt_name,
    size_t		name_len,
    generic_args_tab_T	*gatab,
    cctx_T		*cctx)
{
    typval_T	tv;

    tv.v_type = VAR_UNKNOWN;

    if (eval_variable_import(gt_name, &tv, EVAL_VAR_NO_GENERIC) == OK)
    {
	semsg(_(e_redefining_script_item_str), gt_name);
	clear_tv(&tv);
	return TRUE;
    }

    for (int i = 0; i < gatab->gat_args.ga_len; i++)
    {
	generic_T *generic = &((generic_T *)gatab->gat_args.ga_data)[i];

	if (STRNCMP(gt_name, generic->gt_name, name_len) == 0
				&& generic->gt_name[name_len] == NUL)
	{
	    semsg(_(e_duplicate_type_var_name_str), gt_name);
	    return TRUE;
	}
    }

    class_T *cl = get_type_resolve_ctx_class();
    if ((cctx != NULL
		&& find_generic_type_in_cctx(gt_name, name_len, cctx) != NULL)
	    || (cl != NULL
		&& find_generic_type_in_class(gt_name, name_len, cl) != NULL))
    {
	semsg(_(e_duplicate_type_var_name_str), gt_name);
	return TRUE;
    }

    return FALSE;
}

/*
 * Parses the type parameters specified when defining a new generic function,
 * class or interface, starting at the opening '<' character and ending at the
 * matching '>'.
 *
 * On entry, "p" must point to the opening '<' character.
 * On success, returns a pointer to the character after the closing '>'.
 * On failure, returns NULL and reports an error message.
 *
 * Arguments:
 *   name      - the name of the function or class being defined
 *   p         - pointer to the opening '<' character in the definition
 *   gatab     - args table to allocate new type objects and to store parsed
 *               type argument names and their types.
 *   cctx      - current compile context, used to check for duplicate names in
 *		 outer scopes (may be NULL)
 *
 * This function enforces correct syntax for generic type parameter lists:
 * - No whitespace before or after the opening '<'
 * - Parameters must be separated by a comma and whitespace
 * - No whitespace after a parameter name
 * - The list must not be empty
 */
    char_u *
parse_generic_type_params(
    char_u		*name,
    char_u		*p,
    generic_args_tab_T	*gatab,
    cctx_T		*cctx)
{
    // No white space allowed before the '<'
    if (VIM_ISWHITE(*(p - 1)))
    {
	semsg(_(e_no_white_space_allowed_before_str_str), "<", p);
	return NULL;
    }

    if (VIM_ISWHITE(*(p + 1)))
    {
	semsg(_(e_no_white_space_allowed_after_str_str), "<", p);
	return NULL;
    }

    char_u	    *start = ++p;

    while (*p && *p != '>')
    {
	p = skipwhite(p);

	if (*p == NUL || *p == '>')
	{
	    semsg(_(e_missing_type_after_str), p - 1);
	    return NULL;
	}

	if (!ASCII_ISUPPER(*p))
	{
	    if (ASCII_ISLOWER(*p))
		semsg(_(e_type_var_name_must_start_with_uppercase_letter_str), p);
	    else
		semsg(_(e_missing_type_after_str), p - 1);
	    return NULL;
	}

	char_u	*name_start = p;
	char_u	*name_end = NULL;
	char_u	cc;
	size_t	name_len = 0;

	p++;
	while (ASCII_ISALNUM(*p) || *p == '_')
	    p++;
	name_end = p;

	name_len = name_end - name_start;
	cc = *name_end;
	*name_end = NUL;

	// The type variable cannot have the name of the function or class
	// being defined, without a "<SNR>123_" prefix.
	char_u	*def_name = name;
	if (def_name[0] == K_SPECIAL && vim_strchr(def_name, '_') != NULL)
	    def_name = vim_strchr(def_name, '_') + 1;
	if (STRCMP(def_name, name_start) == 0)
	{
	    semsg(_(e_redefining_script_item_str), name_start);
	    *name_end = cc;
	    return NULL;
	}

	int name_exists = generic_name_exists(name_start, name_len, gatab,
									cctx);
	*name_end = cc;
	if (name_exists)
	    return NULL;

	if (ga_grow(&gatab->gat_args, 1) == FAIL)
	    return NULL;
	generic_T *generic =
	    &((generic_T *)gatab->gat_args.ga_data)[gatab->gat_args.ga_len];
	gatab->gat_args.ga_len++;

	generic->gt_name = alloc(name_len + 1);
	if (generic->gt_name == NULL)
	    return NULL;
	vim_strncpy(generic->gt_name, name_start, name_len);
	generic->gt_type = NULL;

	if (VIM_ISWHITE(*p))
	{
	    semsg(_(e_no_white_space_allowed_after_str_str), generic->gt_name,
		    name_start);
	    return NULL;
	}

	if (*p != ',' && *p != '>')
	{
	    semsg(_(e_missing_comma_in_generic_str), start);
	    return NULL;
	}
	if (*p == ',')
	{
	    if (!VIM_ISWHITE(*(p + 1)))
	    {
		semsg(_(e_white_space_required_after_str_str), ",", p);
		return NULL;
	    }
	    p++;
	}
    }
    if (*p != '>')
	return NULL;
    p++;

    int gat_sz = generic_args_table_size(gatab);

    if (gat_sz == 0)
    {
	emsg_funcname(e_empty_type_list_for_generic_str, name);
	return NULL;
    }

    // set the generic params to VAR_ANY type
    if (ga_grow(&gatab->gat_param_types, gat_sz) == FAIL)
	return NULL;

    gatab->gat_param_types.ga_len = gat_sz;
    for (int i = 0; i < generic_args_table_size(gatab); i++)
    {
	type_T *gt = &((type_T *)gatab->gat_param_types.ga_data)[i];

	CLEAR_POINTER(gt);
	gt->tt_type = VAR_ANY;
	gt->tt_flags = TTFLAG_GENERIC;

	generic_T *generic = &((generic_T *)gatab->gat_args.ga_data)[i];
	generic->gt_type = gt;
    }

    return p;
}

/*
 * Initialize a new generic function "fp" using the list of generic types and
 * generic arguments in "gatab".
 *
 * This function:
 *   - Marks the function as generic.
 *   - Sets the generic argument count and stores the type and argument lists.
 *   - Transfers ownership of the arrays from the growarrays to the function.
 *   - Initializes the generic function's lookup table.
 */
    void
generic_func_init(ufunc_T *fp, generic_args_tab_T *gatab)
{
    fp->uf_flags |= FC_GENERIC;
    fp->uf_generic_argcount = gatab->gat_args.ga_len;
    fp->uf_generic_args = (generic_T *)gatab->gat_args.ga_data;
    ga_init(&gatab->gat_args);	// remove the reference to the args
    fp->uf_generic_param_types = (type_T *)gatab->gat_param_types.ga_data;
    ga_init(&gatab->gat_param_types);	// remove the reference
    ga_init(&fp->uf_generic_arg_types);
    hash_init(&fp->uf_generic_functab);
}

/*
 * Initialize the generic args table for a class or a function
 */
    void
generic_args_table_init(generic_args_tab_T *gatab)
{
    ga_init2(&gatab->gat_args, sizeof(generic_T), 10);
    ga_init2(&gatab->gat_param_types, sizeof(type_T), 10);
    ga_init2(&gatab->gat_arg_types, sizeof(type_T), 10);
}

/*
 * Return the number of entries in the generic args table
 */
    int
generic_args_table_size(generic_args_tab_T *gatab)
{
    return gatab->gat_args.ga_len;
}

/*
 * Free all the generic args table items
 */
    void
generic_args_table_clear(generic_args_tab_T *gatab)
{
    // "gat_param_types" is an array of types, not a list of pointers
    ga_clear(&gatab->gat_param_types);
    clear_type_list(&gatab->gat_arg_types);
    for (int i = 0; i < gatab->gat_args.ga_len; i++)
    {
	generic_T *generic = &((generic_T *)gatab->gat_args.ga_data)[i];
	VIM_CLEAR(generic->gt_name);
    }
    ga_clear(&gatab->gat_args);
}

/*
 * When a cloning a function "fp" to "new_fp", copy the generic function
 * related information.  Returns FAIL when out of memory, "new_fp" is not a
 * generic function then.
 */
    int
copy_generic_function(ufunc_T *fp, ufunc_T *new_fp)
{
    int		i;
    int		sz;

    if (!IS_GENERIC_FUNC(fp))
	return OK;

    // "new_fp" is a copy of "fp", don't keep the generic state of "fp".  If
    // memory allocation fails "new_fp" is not a generic function.
    new_fp->uf_flags &= ~FC_GENERIC;
    new_fp->uf_generic_args = NULL;
    ga_init(&new_fp->uf_generic_arg_types);
    hash_init(&new_fp->uf_generic_functab);

    sz = fp->uf_generic_argcount * sizeof(type_T);
    new_fp->uf_generic_param_types = alloc_clear_id(sz,
						    aid_generic_func_copy);
    if (new_fp->uf_generic_param_types == NULL)
	return FAIL;

    memcpy(new_fp->uf_generic_param_types, fp->uf_generic_param_types, sz);

    sz = fp->uf_generic_argcount * sizeof(generic_T);
    new_fp->uf_generic_args = alloc_clear(sz);
    if (new_fp->uf_generic_args == NULL)
    {
	VIM_CLEAR(new_fp->uf_generic_param_types);
	return FAIL;
    }
    memcpy(new_fp->uf_generic_args, fp->uf_generic_args, sz);

    for (i = 0; i < fp->uf_generic_argcount; i++)
    {
	new_fp->uf_generic_args[i].gt_name =
	    vim_strsave(fp->uf_generic_args[i].gt_name);
	if (new_fp->uf_generic_args[i].gt_name == NULL)
	{
	    while (--i >= 0)
		vim_free(new_fp->uf_generic_args[i].gt_name);
	    VIM_CLEAR(new_fp->uf_generic_args);
	    VIM_CLEAR(new_fp->uf_generic_param_types);
	    return FAIL;
	}
    }

    for (i = 0; i < fp->uf_generic_argcount; i++)
	new_fp->uf_generic_args[i].gt_type =
	    &new_fp->uf_generic_param_types[i];

    new_fp->uf_flags |= FC_GENERIC;
    return OK;
}

/*
 * Returns the index of the generic type "t" in the array "param_types" with
 * "count" items, or -1 if not found.
 */
    static int
generic_type_index(type_T *param_types, int count, type_T *t)
{
    for (int i = 0; i < count; i++)
	if (&param_types[i] == t)
	    return i;
    return -1;
}

/*
 * Returns the index of the generic type pointer "t" in the generic type list
 * of the function "fp".
 *
 * Arguments:
 *   fp - pointer to the generic function (ufunc_T)
 *   t  - pointer to the type_T to search for in the function's generic type
 *        list
 *
 * Returns:
 *   The zero-based index of "t" in fp->uf_generic_param_types if found,
 *   or -1 if not found.
 */
    static int
get_generic_type_index(ufunc_T *fp, type_T *t)
{
    return generic_type_index(fp->uf_generic_param_types,
						fp->uf_generic_argcount, t);
}

/*
 * Evaluates the type arguments for a generic function call and looks up the
 * corresponding concrete function.
 *
 * Arguments:
 *   ufunc - the original (possibly generic) function to evaluate
 *   name  - the function name (used for error messages and lookup)
 *   argp   - pointer to a pointer to the argument string; on entry, "*argp"
 *            should point to the character after the function name (possibly
 *            '<')
 *
 * Returns:
 *   The concrete function corresponding to the given type arguments,
 *   or NULL on error (with an error message reported).
 *
 * Behavior:
 *   - If "ufunc" is a generic function and "*argp" points to '<', attempts to
 *     find or instantiate the concrete function with the specified type
 *     arguments.  On success, advances "*argp" past the type argument list.
 *   - If "ufunc" is generic but "*argp" does not point to '<', reports a
 *     missing type argument error.
 *   - If "ufunc" is not generic but "*argp" points to '<', reports an error
 *     that the function is not generic.
 *   - Otherwise, returns the original function.
 */
    ufunc_T *
eval_generic_func(
    ufunc_T	*ufunc,
    char_u	*name,
    char_u	**argp)
{
    if (IS_GENERIC_FUNC(ufunc))
    {
	if (**argp == '<')
	    ufunc = find_generic_func(ufunc, name, argp);
	else
	{
	    emsg_funcname(e_generic_func_missing_type_args_str, name);
	    return NULL;
	}
    }
    else if (**argp == '<')
    {
	emsg_funcname(e_not_a_generic_function_str, name);
	return NULL;
    }

    return ufunc;
}

/*
 * Checks if the string at "*argp" represents a generic function call with type
 * arguments, i.e., if it starts with a '<', contains a valid type argument
 * list, a closing '>', and is immediately followed by '('.
 *
 * On entry, "*argp" should point to the '<' character.
 * If the pattern matches, advances "*argp" to point to the '(' and returns
 * TRUE.  If not, leaves "*argp" unchanged and returns FALSE.
 *
 * Example:
 *   "<number, string>("
 */
    int
generic_func_call(char_u **argp)
{
    char_u	*p = *argp;

    if (*p != '<')
	return FALSE;

    if (skip_generic_type_args(&p) == FAIL)
	return FALSE;

    if (*p != '(')
	return FALSE;

    *argp = p;
    return TRUE;
}

/*
 * Recursively replaces all occurrences of a generic type in "generic_type"
 * with the corresponding concrete type.  The types are updated in place in
 * "specific_type" and "func_type" (may be NULL), which are copies of
 * "generic_type".
 *
 * For a concrete class "new_cl" created from the generic class "cl" a type
 * variable of "cl" is replaced with the type of "new_cl".  For a generic
 * function "new_fp" created from "fp" a type variable of "fp" is replaced with
 * the type of "new_fp".  "cl" and "new_cl" or "fp" and "new_fp" can be NULL.
 */
    void
update_generic_type(
    class_T	*cl,
    class_T	*new_cl,
    ufunc_T	*fp,
    ufunc_T	*new_fp,
    type_T	*generic_type,
    type_T	**specific_type,
    type_T	**func_type)
{
    type_T	*t = NULL;
    int		idx;

    // When out of memory "generic_type" was not copied, it must not be
    // changed.
    if (*specific_type == generic_type && generic_type->tt_type != VAR_ANY)
	return;

    switch (generic_type->tt_type)
    {
	case VAR_ANY:
	    if (cl != NULL && (idx = generic_type_index(
			    cl->class_generic_param_types,
			    cl->class_generic_argcount, generic_type)) >= 0)
		t = new_cl->class_generic_args[idx].gt_type;
	    else if (fp != NULL && new_fp->uf_generic_args != NULL
		    && (idx = get_generic_type_index(fp, generic_type)) >= 0)
		t = new_fp->uf_generic_args[idx].gt_type;
	    else if (IS_GENERIC_TYPE(generic_type))
		// A type variable of another class or function (e.g. of the
		// class of an inherited method): use it instead of the copy,
		// type variables are found by their address.
		t = generic_type;
	    if (t != NULL)
	    {
		*specific_type = t;
		if (func_type != NULL)
		    *func_type = t;
	    }
	    break;
	case VAR_LIST:
	case VAR_DICT:
	    update_generic_type(cl, new_cl, fp, new_fp,
		    generic_type->tt_member,
		    &(*specific_type)->tt_member,
		    func_type != NULL ? &(*func_type)->tt_member : NULL);
	    break;
	case VAR_FUNC:
	    update_generic_type(cl, new_cl, fp, new_fp,
		    generic_type->tt_member,
		    &(*specific_type)->tt_member,
		    func_type != NULL ? &(*func_type)->tt_member : NULL);
	    // FALLTHROUGH
	case VAR_TUPLE:
	case VAR_OBJECT:	// type arguments of a generic class
	    for (int i = 0; i < generic_type->tt_argcount; i++)
		update_generic_type(cl, new_cl, fp, new_fp,
			generic_type->tt_args[i],
			&(*specific_type)->tt_args[i],
			func_type != NULL ? &(*func_type)->tt_args[i] : NULL);
	    break;
	default:
	    break;
    }
}

// Last unique number given to a class, see generic_key_add_class_ids().
static int last_class_id = 0;

/*
 * Append a unique number for each class and type variable used in "type" to
 * "gap".  A type variable is identified by its address, a key using it is
 * only valid while the type variable exists (see
 * generic_class_remove_parents()).
 */
    static void
generic_key_add_class_ids(type_T *type, garray_T *gap)
{
    char_u	buf[NUMBUFLEN + 2];

    if (type == NULL)
	return;

    if ((type->tt_type == VAR_OBJECT || type->tt_type == VAR_CLASS)
	    && type->tt_class != NULL)
    {
	class_T *cl = type->tt_class;

	if (cl->class_id == 0)
	    cl->class_id = ++last_class_id;
	vim_snprintf((char *)buf, sizeof(buf), " #%d", cl->class_id);
	ga_concat(gap, buf);
    }
    else if (IS_GENERIC_TYPE(type))
    {
	// Different type variables have the same name, use the address.
	vim_snprintf((char *)buf, sizeof(buf), " @%p", (void *)type);
	ga_concat(gap, buf);
    }

    generic_key_add_class_ids(type->tt_member, gap);
    if (type->tt_args != NULL)
	for (int i = 0; i < type->tt_argcount; i++)
	    generic_key_add_class_ids(type->tt_args[i], gap);
}

/*
 * Build the key to look up the concrete function or class for the type
 * arguments in "gatab" in "gap".  The key starts with the type names
 * separated by ", " and "*typeslen" is set to the length of this part.
 * Different classes can have the same name (e.g. an imported class), so a
 * unique number for each class used in the types is appended.
 */
    void
generic_args_key(generic_args_tab_T *gatab, garray_T *gap, size_t *typeslen)
{
    int		i;

    for (i = 0; i < gatab->gat_args.ga_len; i++)
    {
	generic_T  *generic_arg = (generic_T *)gatab->gat_args.ga_data + i;

	if (i > 0)
	    ga_concat(gap, (char_u *)", ");
	ga_concat(gap, generic_arg->gt_name);
    }
    *typeslen = gap->ga_len;

    for (i = 0; i < gatab->gat_args.ga_len; i++)
	generic_key_add_class_ids(
		((generic_T *)gatab->gat_args.ga_data + i)->gt_type, gap);
    ga_append(gap, NUL);
}

/*
 * Look up the type arguments in "gatab" in the table "ht" of a generic
 * function or class.  The key is built in "gkey_gap" and "*typeslen" is set,
 * see generic_args_key().  Returns the hash item or NULL if not found.
 */
    hashitem_T *
generic_args_lookup(
    hashtab_T		*ht,
    generic_args_tab_T	*gatab,
    garray_T		*gkey_gap,
    size_t		*typeslen)
{
    hashitem_T	*hi;

    generic_args_key(gatab, gkey_gap, typeslen);
    hi = hash_find(ht, (char_u *)gkey_gap->ga_data);
    return HASHITEM_EMPTY(hi) ? NULL : hi;
}

/*
 * Make a copy of all the argument types, the return type, the vararg type and
 * the function type of function "fp" for the function "new_fp" created from
 * it and replace the generic types in them, see update_generic_type().
 * Returns FAIL when out of memory, then a type of "new_fp" may be the type of
 * "fp" and "new_fp" must not be used.
 */
    int
update_func_generic_types(
    class_T	*cl,
    class_T	*new_cl,
    ufunc_T	*fp,
    ufunc_T	*new_fp)
{
    int		i;

    // "uf_arg_types" is NULL when out of memory while defining "fp".  When
    // out of memory copy_type_deep() returns the type itself.
    if (fp->uf_arg_types != NULL)
	for (i = 0; i < fp->uf_args.ga_len; i++)
	{
	    new_fp->uf_arg_types[i] = copy_type_deep(fp->uf_arg_types[i],
						&new_fp->uf_type_list);
	    if (new_fp->uf_arg_types[i] == fp->uf_arg_types[i])
		return FAIL;
	}
    if (fp->uf_ret_type != NULL)
    {
	new_fp->uf_ret_type = copy_type_deep(fp->uf_ret_type,
						&new_fp->uf_type_list);
	if (new_fp->uf_ret_type == fp->uf_ret_type)
	    return FAIL;
    }
    if (fp->uf_va_type != NULL)
    {
	new_fp->uf_va_type = copy_type_deep(fp->uf_va_type,
						&new_fp->uf_type_list);
	if (new_fp->uf_va_type == fp->uf_va_type)
	    return FAIL;
    }
    if (fp->uf_func_type != NULL)
    {
	new_fp->uf_func_type = copy_type_deep(fp->uf_func_type,
						&new_fp->uf_type_list);
	if (new_fp->uf_func_type == fp->uf_func_type)
	    return FAIL;
    }

    // The function type has no argument types when out of memory.
    type_T	*ft = new_fp->uf_func_type;
    if (fp->uf_arg_types != NULL)
	for (i = 0; i < fp->uf_args.ga_len; i++)
	    update_generic_type(cl, new_cl, fp, new_fp, fp->uf_arg_types[i],
				&new_fp->uf_arg_types[i],
				ft != NULL && ft->tt_args != NULL
					&& ft->tt_argcount > i
						? &ft->tt_args[i] : NULL);
    if (fp->uf_va_type != NULL)
    {
	// The varargs type is the last argument of the function type.
	int	va_idx = fp->uf_args.ga_len;

	update_generic_type(cl, new_cl, fp, new_fp, fp->uf_va_type,
			    &new_fp->uf_va_type,
			    ft != NULL && ft->tt_args != NULL
					&& ft->tt_argcount > va_idx
						? &ft->tt_args[va_idx] : NULL);
    }
    if (fp->uf_ret_type != NULL)
	update_generic_type(cl, new_cl, fp, new_fp, fp->uf_ret_type,
			    &new_fp->uf_ret_type,
			    ft != NULL ? &ft->tt_member : NULL);
    return OK;
}

/*
 * Adds a new concrete instance of a generic function for a specific set of
 * type arguments.
 *
 * Arguments:
 *   fp       - the original generic function to instantiate
 *   key      - a string key representing the specific type arguments (used for
 *		lookup), see generic_args_key()
 *   typeslen - length of the type names at the start of "key"
 *   gatab    - generic args table containing the parsed type
 *              arguments and their names
 *
 * Returns:
 *   Pointer to the new ufunc_T representing the instantiated function,
 *   or NULL if the function already exists or on allocation failure.
 *
 * This function:
 *   - Checks if a function with the given type arguments already exists.
 *   - Allocates and initializes a new function instance with the specific
 *     types.
 *   - Updates the function's name and expanded name to include the type
 *     arguments.
 *   - Copies and updates all relevant type information (argument types, return
 *     type, vararg type, function type), replacing generic types with the
 *     actual types.
 *   - Sets the new function's status to UF_TO_BE_COMPILED.
 *   - Registers the new function in the generic function's lookup table.
 */
    static ufunc_T *
generic_func_add(
    ufunc_T		*fp,
    char_u		*key,
    size_t		typeslen,
    generic_args_tab_T	*gatab)
{
    hashtab_T	*ht = &fp->uf_generic_functab;
    long_u	hash;
    hashitem_T	*hi;
    int		i;

    hash = hash_hash(key);
    hi = hash_lookup(ht, key, hash);
    if (!HASHITEM_EMPTY(hi))
	return NULL;

    size_t	keylen = STRLEN(key);
    gfitem_T    *gfitem = alloc(sizeof(gfitem_T) + keylen);
    if (gfitem == NULL)
	return NULL;

    STRCPY(gfitem->gfi_name, key);

    ufunc_T *new_fp = copy_function(fp, (int)(typeslen + 2));
    if (new_fp == NULL)
    {
	vim_free(gfitem);
	return NULL;
    }

    new_fp->uf_generic_arg_types = gatab->gat_arg_types;
    // now that the type arguments is copied, remove the reference to the type
    // arguments
    ga_init(&gatab->gat_arg_types);

    if (fp->uf_class != NULL)
	new_fp->uf_class = fp->uf_class;

    // Create a new name for the function: name<type1, type2...>
    new_fp->uf_name[new_fp->uf_namelen] =  '<';
    mch_memmove(new_fp->uf_name + new_fp->uf_namelen + 1, key, typeslen);
    new_fp->uf_name[new_fp->uf_namelen + typeslen + 1] =  '>';
    new_fp->uf_name[new_fp->uf_namelen + typeslen + 2] =  NUL;
    new_fp->uf_namelen += typeslen + 2;

    if (new_fp->uf_name_exp != NULL)
    {
	size_t	explen = STRLEN(new_fp->uf_name_exp);
	char_u	*new_name_exp = alloc(explen + typeslen + 3);
	if (new_name_exp != NULL)
	{
	    STRCPY(new_name_exp, new_fp->uf_name_exp);
	    new_name_exp[explen] = '<';
	    mch_memmove(new_name_exp + explen + 1, key, typeslen);
	    STRCPY(new_name_exp + explen + typeslen + 1, ">");
	    vim_free(new_fp->uf_name_exp);
	    new_fp->uf_name_exp = new_name_exp;
	}
    }

    gfitem->gfi_ufunc = new_fp;
    gfitem->gfi_ufunc->uf_def_status = UF_TO_BE_COMPILED;

    // Replace the t_any generic types with the actual types
    for (i = 0; i < fp->uf_generic_argcount; i++)
    {
	generic_T  *generic_arg;
	generic_arg = (generic_T *)gatab->gat_args.ga_data + i;
	generic_T *gt = &new_fp->uf_generic_args[i];
	gt->gt_type = generic_arg->gt_type;
    }

    // Copy the types and replace the generic types in them.  Use the
    // concrete class for an object type using a generic class.  When
    // creating the class fails the function is not used, it is created again
    // the next time and gives the error again.
    if (update_func_generic_types(NULL, NULL, fp, new_fp) == FAIL
				|| ufunc_resolve_generic_types(new_fp) == FAIL)
    {
	func_ptr_unref(new_fp);
	vim_free(gfitem);
	return NULL;
    }

    hash_add_item(ht, hi, gfitem->gfi_name, hash);

    return new_fp;
}

/*
 * Looks up a concrete instance of a generic function "fp" using the type
 * arguments specified in "gatab".
 *
 * The lookup key is built in the provided growarray "gkey_gap" by
 * generic_args_key(), which also sets "*typeslen".  The contents of
 * "gkey_gap" will be overwritten.
 *
 * Arguments:
 *   fp        - the generic function to search in
 *   gatab     - generic args table containing the parsed type
 *               arguments and their names
 *   gkey_gap  - growarray used to build and store the lookup key string
 *   typeslen  - set to the length of the type names in the key
 *
 * Returns:
 *   Pointer to the ufunc_T representing the concrete function if found, or
 *   NULL if no matching function exists.
 */
    static ufunc_T *
generic_lookup_func(
    ufunc_T		*fp,
    generic_args_tab_T	*gatab,
    garray_T		*gkey_gap,
    size_t		*typeslen)
{
    hashitem_T	*hi = generic_args_lookup(&fp->uf_generic_functab, gatab,
							gkey_gap, typeslen);

    return hi == NULL ? NULL : HI2GFITEM(hi)->gfi_ufunc;
}

/*
 * Returns a concrete instance of the generic function "fp" using the type
 * arguments specified in "gatab". If such an instance does not exist,
 * it is created and registered.
 *
 * Arguments:
 *   fp        - the generic function to instantiate
 *   gatab     - generic args table containing the parsed type
 *               arguments and their names
 *
 * Returns:
 *   Pointer to the ufunc_T representing the concrete function instance,
 *   or NULL if the type arguments are invalid or on allocation failure.
 *
 * Behavior:
 *   - If "fp" is not a generic function and no type arguments are given,
 *     returns "fp" as-is.
 *   - If "fp" is not generic but type arguments are given, reports an error
 *     and returns NULL.
 *   - Validates the number of type arguments, reporting errors for missing,
 *     too few, or too many.
 *   - Looks up an existing function instance with the given types.
 *   - If not found, creates and registers a new function instance.
 */
    ufunc_T *
generic_func_get(ufunc_T *fp, generic_args_tab_T *gatab)
{
    char	*emsg = NULL;

    if (!IS_GENERIC_FUNC(fp))
    {
	if (gatab && generic_args_table_size(gatab) > 0)
	{
	    emsg_funcname(e_not_a_generic_function_str, fp->uf_name);
	    return NULL;
	}
	return fp;
    }

    if (gatab == NULL || gatab->gat_args.ga_len == 0)
	emsg = e_generic_func_missing_type_args_str;
    else if (gatab->gat_args.ga_len < fp->uf_generic_argcount)
	emsg = e_not_enough_types_for_generic_function_str;
    else if (gatab->gat_args.ga_len > fp->uf_generic_argcount)
	emsg = e_too_many_types_for_generic_function_str;

    if (emsg != NULL)
    {
	emsg_funcname(emsg, printable_func_name(fp));
	return NULL;
    }

    // generic function call
    garray_T gkey_ga;

    ga_init2(&gkey_ga, 1, 80);

    // Look up the function with specific types
    size_t	typeslen;
    ufunc_T	*generic_fp = generic_lookup_func(fp, gatab, &gkey_ga,
								    &typeslen);
    if (generic_fp == NULL)
	// generic function with these type arguments doesn't exist.
	// Create a new one.
	generic_fp = generic_func_add(fp, (char_u *)gkey_ga.ga_data, typeslen,
									gatab);
    ga_clear(&gkey_ga);

    return generic_fp;
}

/*
 * Looks up or creates a concrete instance of a generic function "ufunc" using
 * the type arguments specified after the function name in "name".
 *
 * On entry, "name" points to the function name, and "*argp" points to the
 * opening '<' of the type argument list (i.e., name + namelen).
 *
 * Arguments:
 *   ufunc - the generic function to instantiate or look up
 *   name  - the function name, followed by the type argument list
 *   argp  - pointer to a pointer to the type argument list (should point to
 *           '<'); on success, advanced to the character after the closing '>'
 *
 * Returns:
 *   Pointer to the ufunc_T representing the concrete function instance if
 *   successful, or NULL if parsing fails or the instance cannot be created.
 *
 * This function:
 *   - Parses the type arguments from the string after the function name.
 *   - Looks up an existing function instance with those type arguments.
 *   - If not found, creates and registers a new function instance.
 *   - Advances "*argp" to after the type argument list on success.
 */
    ufunc_T *
find_generic_func(ufunc_T *ufunc, char_u *name, char_u **argp)
{
    generic_args_tab_T	gatab;
    char_u	*p;
    ufunc_T	*new_ufunc = NULL;

    generic_args_table_init(&gatab);

    // Get the list of types following the name
    p = parse_generic_type_args(name, *argp - name, *argp, &gatab, NULL);
    if (p != NULL)
    {
	new_ufunc = generic_func_get(ufunc, &gatab);
	*argp = p;
    }

    generic_args_table_clear(&gatab);

    return new_ufunc;
}

/*
 * Searches for a generic type with the given name "gt_name" in the generic
 * function "ufunc".
 *
 * Arguments:
 *   gt_name - the name of the generic type to search for
 *   ufunc   - the generic function in which to search for the type
 *
 * Returns:
 *   Pointer to the type_T representing the found generic type,
 *   or NULL if the type is not found or if "ufunc" is not a generic function.
 */
    static type_T *
find_generic_type_in_ufunc(char_u *gt_name, size_t name_len, ufunc_T *ufunc)
{
    if (!IS_GENERIC_FUNC(ufunc))
	return NULL;

    for (int i = 0; i < ufunc->uf_generic_argcount; i++)
    {
	generic_T *generic;

	generic = ((generic_T *)ufunc->uf_generic_args) + i;
	if (STRNCMP(generic->gt_name, gt_name, name_len) == 0
				&& generic->gt_name[name_len] == NUL)
	{
	    type_T *type = generic->gt_type;
	    return type;
	}
    }

    return NULL;
}

/*
 * Searches for a generic type with the given name "gt_name" in the current
 * function context "cctx" and its outer (enclosing) contexts, if necessary.
 *
 * Arguments:
 *   gt_name - the name of the generic type to search for
 *   cctx    - the current compile context, which may be nested
 *
 * Returns:
 *   Pointer to the type_T representing the found generic type,
 *   or NULL if the type is not found in the current or any outer context.
 */
    static type_T *
find_generic_type_in_cctx(char_u *gt_name, size_t name_len, cctx_T *cctx)
{
    type_T	*type;

    type = find_generic_type_in_ufunc(gt_name, name_len, cctx->ctx_ufunc);
    if (type != NULL)
	return type;

    // An inherited method uses the type variables of the class that defined
    // it, not those of "uf_class".
    class_T *cl = cctx->ctx_generic_class;
    if (cl == NULL)
	cl = cctx->ctx_ufunc->uf_defclass != NULL
		    ? cctx->ctx_ufunc->uf_defclass : cctx->ctx_ufunc->uf_class;
    if (cl != NULL)
    {
	type = find_generic_type_in_class(gt_name, name_len, cl);
	if (type != NULL)
	    return type;
    }

    if (cctx->ctx_outer != NULL)
	return find_generic_type_in_cctx(gt_name, name_len, cctx->ctx_outer);

    return NULL;
}

/*
 * Looks up the type variable "gt_name" of "name_len" bytes, in this order:
 * 1. The function of the type resolution context (the function being
 *    defined), see set_type_resolve_ctx().
 * 2. When compiling ("cctx" is not NULL): the compiled function, its class
 *    and the outer compile contexts, see find_generic_type_in_cctx().
 * 3. Otherwise: the class of the type resolution context (the class being
 *    defined or created).  This class is not used when compiling, it may be
 *    for another definition.
 *
 * Returns:
 *   Pointer to the type_T representing the found generic type, or NULL if the
 *   type is not found.
 */
    type_T *
find_generic_type(
    char_u	*gt_name,
    size_t	name_len,
    cctx_T	*cctx)
{
    ufunc_T	*ufunc = get_type_resolve_ctx_ufunc();
    class_T	*cl = get_type_resolve_ctx_class();

    if (ufunc != NULL)
    {
	type_T *type = find_generic_type_in_ufunc(gt_name, name_len, ufunc);
	if (type != NULL)
	    return type;
    }

    if (cctx != NULL)
	return find_generic_type_in_cctx(gt_name, name_len, cctx);

    if (cl != NULL)
	return find_generic_type_in_class(gt_name, name_len, cl);

    return NULL;
}

/*
 * Unreferences all concrete function instances stored in the generic
 * function table of "fp", an instance is freed when it is not referenced
 * elsewhere.  Frees each associated gfitem_T structure and clears the hash
 * table.
 *
 * Arguments:
 *   fp - the generic function whose function table should be freed
 */
    static void
free_generic_functab(ufunc_T *fp)
{
    hashtab_T	*ht = &fp->uf_generic_functab;
    long	todo;
    hashitem_T	*hi;

    todo = (long)ht->ht_used;
    FOR_ALL_HASHTAB_ITEMS(ht, hi, todo)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    gfitem_T    *gfitem = HI2GFITEM(hi);

	    // The function may still be referenced, e.g. by a funcref.  Then
	    // it is freed when the last reference goes away.
	    func_ptr_unref(gfitem->gfi_ufunc);
	    vim_free(gfitem);
	    --todo;
	}
    }
    hash_clear(ht);
}

/*
 * Frees all memory and state associated with a generic function "fp".
 * This includes the generic type list, generic argument list, and all
 * concrete function instances in the generic function table.
 *
 * Arguments:
 *   fp - the generic function to clear
 */
    void
generic_func_clear_items(ufunc_T *fp)
{
    VIM_CLEAR(fp->uf_generic_param_types);
    clear_type_list(&fp->uf_generic_arg_types);
    for (int i = 0; i < fp->uf_generic_argcount; i++)
	VIM_CLEAR(fp->uf_generic_args[i].gt_name);
    VIM_CLEAR(fp->uf_generic_args);
    free_generic_functab(fp);
    fp->uf_flags &= ~FC_GENERIC;
}

#endif // FEAT_EVAL
