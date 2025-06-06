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

#if defined(FEAT_EVAL) || defined(PROTO)


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

/*
 * Returns a pointer to the '<' character in "name" that starts the generic
 * type argument list, skipping an initial <SNR> or <lambda> prefix if present.
 * Returns NULL if no '<' is found before a '(' or the end of the string.
 *
 * Example:
 *   "<SNR>123_Fn<number>"    -> returns pointer to '<'
 *   "<lambda>123_Fn<number>" -> returns pointer to '<'
 *   "Func<number>"           -> returns pointer to '<'
 *   "Func()"                 -> returns NULL
 */
    char_u *
generic_func_find_open_angle_bracket(char_u *name)
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
 * Returns TRUE if 'c' is a valid generic type or space character.
 * Only type names, comma separators and angle brackets are supported.
 */
    static int
is_valid_generic_char(char_u c)
{
    return (ASCII_ISALNUM(c) || VIM_ISWHITE(c)
				|| c == ',' || c == '<' || c == '>');
}

/*
 * Finds the matching '>' character for a generic function type parameter or
 * argument list, starting from the character after the opening '<'.
 *
 * Handles nested angle brackets by tracking the nesting level.
 * Only allows alphanumeric characters, whitespace, commas, and angle brackets
 * within the type argument list. Stops parsing if an open parenthesis '(' is
 * encountered.
 *
 * Arguments:
 *   s - pointer to the character after the opening '<'
 *
 * Returns:
 *   Pointer to the matching '>' character if found,
 *   or NULL if not found, invalid syntax, or on error.
 */
    char_u *
generic_func_find_close_angle_bracket(char_u *s)
{
    char_u	*p = s;
    int		nesting_level = 0;

    while (*p != NUL)	// skip to the outermose angle bracket
    {
	if (!is_valid_generic_char(*p))
	    return NULL;

	if (*p == '<')	// nested '<'
	    nesting_level++;
	else if (*p == '>')
	{
	    if (nesting_level == 0)
		break;
	    nesting_level--;
	}
	++p;
    }

    return *p == '>' ? p : NULL;
}

/*
 * Parses the concrete type arguments provided in a generic function call,
 * between the opening '<' and closing '>' characters.
 *
 * Arguments:
 *   func_name - the name of the function being called (used for error messages)
 *   len       - length of the function name
 *   start     - pointer to the opening '<' character in the call
 *   types_gap - growarray for allocating new type objects
 *   gtfn_gap  - growarray to store parsed type argument names and their types
 *
 * Returns:
 *   Pointer to the character after the closing '>' on success,
 *   or NULL on failure (with an error message reported).
 *
 * This function enforces correct syntax for generic type argument lists,
 * including whitespace rules, comma separation, and non-empty argument lists.
 */
    char_u *
parse_generic_func_type_args(
    char_u	*func_name,
    size_t	len,
    char_u	*start,
    garray_T	*types_gap,
    garray_T	*gtfn_gap)
{
    gf_type_name_T	*gftn_item;
    type_T		*type_arg;
    char_u		*p = start;

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

	if (*p == NUL || *p == '>')
	{
	    semsg(_(e_missing_type_after_str), start);
	    return NULL;
	}

	char_u	    *type_name = p;

	// parse the type
	type_arg = parse_type(&p, types_gap, NULL, NULL, TRUE);
	if (type_arg == NULL)
	    return NULL;

	// create space for the name and the new type
	if (ga_grow(gtfn_gap, 1) == FAIL)
	    return NULL;
	gftn_item = (gf_type_name_T *)gtfn_gap->ga_data + gtfn_gap->ga_len;
	gtfn_gap->ga_len++;

	// copy the type name (temporarily NUL-terminate for copying)
	char_u	cc = *p;
	*p = NUL;
	vim_strncpy(gftn_item->gftn_name, type_name,
						sizeof(gftn_item->gftn_name));
	*p = cc;

	// add the new type
	gftn_item->gftn_type = type_arg;

	p = skipwhite(p);

	// after a type, expect ',' or '>'
	if (*p != ',' && *p != '>')
	{
	    semsg(_(e_missing_comma_in_generic_function), start);
	    return NULL;
	}

	// if there's a comma, require whitespace after it and skip it
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

    // ensure the list of types ends in a closing '>'
    if (*p != '>')
	return NULL;

    // no whitespace allowed before '>'
    if (VIM_ISWHITE(*(p - 1)))
    {
	semsg(_(e_no_white_space_allowed_before_str_str), ">", p);
	return NULL;
    }

    // at least one type argument is required
    if (gtfn_gap->ga_len == 0)
    {
	char_u	cc = func_name[len];
	func_name[len] = NUL;
	semsg(_(e_empty_type_list_for_generic_function_str), func_name);
	func_name[len] = cc;
	return NULL;
    }
    ++p;	// skip the '>'

    return p;
}

/*
 * Parses the type parameters specified when defining a new generic function,
 * between the opening '<' and closing '>' characters.
 *
 * Arguments:
 *   func_name - the name of the function being defined (for error
 *               messages)
 *   p         - pointer to the opening '<' character in the
 *               definition
 *   gtl_gap   - growarray to store the created type_T objects for
 *               each parameter
 *   gt_gap    - growarray to store the mapping of parameter names to
 *               types
 *
 * Returns:
 *   Pointer to the character after the closing '>' on success,
 *   or NULL on failure (with an error message reported).
 *
 * This function enforces correct syntax for generic type parameter lists:
 * - No whitespace before or after the opening '<'
 * - Each parameter must be a single uppercase letter
 * - Parameters must be separated by a comma and whitespace
 * - No whitespace after a parameter name
 * - The list must not be empty
 */
    char_u *
parse_generic_func_type_params(
    char_u	*func_name,
    char_u	*p,
    garray_T	*gtl_gap,
    garray_T	*gt_gap)
{
    // No white space allowed before the "<"
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
		semsg(_(e_generic_type_name_must_be_uppercase_letter_str), p);
	    else
		semsg(_(e_missing_type_after_str), p - 1);
	    return NULL;
	}

	if (ASCII_ISALNUM(*(p + 1)))
	{
	    semsg(_(e_generic_typename_not_a_single_letter), p);
	    return NULL;
	}

	if (ga_grow(gtl_gap, 1) == FAIL)
	    return NULL;
	type_T *gt =
	    &((type_T *)gtl_gap->ga_data)[gtl_gap->ga_len];
	gtl_gap->ga_len++;

	CLEAR_POINTER(gt);
	gt->tt_type = VAR_ANY;
	gt->tt_flags = TTFLAG_GENERIC;

	if (ga_grow(gt_gap, 1) == FAIL)
	    return NULL;
	generic_T *generic =
	    &((generic_T *)gt_gap->ga_data)[gt_gap->ga_len];
	gt_gap->ga_len++;

	generic->gt_name = *p;
	generic->gt_type = gt;

	if (VIM_ISWHITE(*(p + 1)))
	{
	    char	name[2];

	    name[0] = *p;
	    name[1] = NUL;
	    semsg(_(e_no_white_space_allowed_after_str_str), name, p);
	    return NULL;
	}

	p++;		// skip the generic type name

	if (*p != ',' && *p != '>')
	{
	    semsg(_(e_missing_comma_in_generic_function), start);
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

    if (gtl_gap->ga_len == 0)
    {
	emsg_funcname(e_empty_type_list_for_generic_function_str, func_name);
	return NULL;
    }
    p++;

    return p;
}

/*
 * Returns the index of the generic type "t" (of type VAR_ANY) in the generic
 * function "fp".
 *
 * Arguments:
 *   fp - pointer to the generic function (ufunc_T)
 *   t  - pointer to the type_T to search for in the function's generic type
 *        list
 *
 * Returns:
 *   The zero-based index of "t" in fp->uf_generic_type_list if found,
 *   or -1 if not found.
 */
    static int
get_generic_type_index(ufunc_T *fp, type_T *t)
{
    for (int i = 0; i < fp->uf_generic_argcount; i++)
    {
	if (&fp->uf_generic_type_list[i] == t)
	    return i;
    }
    return -1;
}

/*
 * Evaluates the type arguments for a generic function call and looks up the
 * corresponding concrete function.
 *
 * Arguments:
 *   ufunc     - the original (possibly generic) function to evaluate
 *   name      - the function name (used for error messages and lookup)
 *   namelen   - length of the function name
 *   afterchar - the character immediately following the function name
 *               (should be '<' if type arguments are present)
 *
 * Returns:
 *   The concrete function corresponding to the given type arguments,
 *   or NULL on error (with an error message reported).
 *
 * Behavior:
 *   - If 'ufunc' is a generic function and 'afterchar' is '<', attempts to
 *     find or instantiate the concrete function with the specified type
 *     arguments.
 *   - If 'ufunc' is generic but 'afterchar' is not '<', reports a missing
 *     type argument error.
 *   - If 'ufunc' is not generic but 'afterchar' is '<', reports an error that
 *     the function is not generic.
 *   - Otherwise, returns the original function.
 */
    ufunc_T *
eval_generic_func(
    ufunc_T	*ufunc,
    char_u	*name,
    int		namelen,
    char_u	afterchar)
{
    if (IS_GENERIC_FUNC(ufunc))
    {
	if (afterchar == '<')
	    ufunc = find_generic_func(ufunc, name, namelen);
	else
	{
	    emsg_funcname(e_generic_func_missing_type_args_str, name);
	    return NULL;
	}
    }
    else if (afterchar == '<')
    {
	emsg_funcname(e_not_a_generic_function_str, name);
	return NULL;
    }

    return ufunc;
}

/*
 * Checks if the string at "*arg" represents a generic function call with type
 * arguments.
 *
 * For a generic function call, "*arg" should point to a '<', followed by one
 * or more type arguments separated by ',', then a closing '>', and finally an
 * opening '('.
 *
 * If the pattern matches, advances "*arg" to point to the '(' and returns
 * TRUE.  If not, leaves "*arg" unchanged and returns FALSE.
 *
 * Example:
 *   "<number, string>("
 */
    int
generic_func_call(char_u **arg)
{
    char_u	*p = *arg;

    if (*p != '<')
	return FALSE;

    p++;	// skip opening "<"
    p = generic_func_find_close_angle_bracket(p);
    if (p == NULL)
	return FALSE;

    p++;	// skip closing ">"

    if (*p != '(')
	return FALSE;

    *arg = p;
    return TRUE;
}

/*
 * Replaces all occurrences of the generic type "generic_type" in the function
 * definition with the corresponding concrete type from "new_ufunc", based on
 * the mapping from the original generic function "ufunc".
 *
 * This is used when instantiating a new function "new_ufunc" from a generic
 * function "ufunc" with specific type arguments. The function recursively
 * updates the argument types, variable argument types, and return types,
 * including nested types (such as lists, dictionaries, and tuples).
 *
 * Arguments:
 *   ufunc         - the original generic function
 *   new_ufunc     - the new function being created with concrete types
 *   generic_type  - the generic type to be replaced
 *   specific_type - pointer to the location where the concrete type should be
 *                   set
 *   func_type     - pointer to the function type to update (may be NULL)
 */
    static void
update_generic_type(
    ufunc_T	*ufunc,
    ufunc_T	*new_ufunc,
    type_T	*generic_type,
    type_T	**specific_type,
    type_T	**func_type)
{
    int	idx;

    switch (generic_type->tt_type)
    {
	case VAR_ANY:
	    idx = get_generic_type_index(ufunc, generic_type);
	    if (idx != -1)
	    {
		*specific_type = new_ufunc->uf_generic_args[idx].gt_type;
		if (func_type != NULL)
		    *func_type = new_ufunc->uf_generic_args[idx].gt_type;
	    }
	    break;
	case VAR_LIST:
	case VAR_DICT:
	    update_generic_type(ufunc, new_ufunc, generic_type->tt_member,
		    &(*specific_type)->tt_member,
		    func_type != NULL ? &(*func_type)->tt_member : NULL);
	    break;
	case VAR_TUPLE:
	    for (int i = 0; i < generic_type->tt_argcount; i++)
		update_generic_type(ufunc, new_ufunc,
			generic_type->tt_args[i],
			&(*specific_type)->tt_args[i],
			func_type != NULL ? &(*func_type)->tt_args[i] : NULL);
	    break;
	default:
	    break;
    }
}

/*
 * Adds a new concrete instance of a generic function for a specific set of
 * type arguments.
 *
 * Arguments:
 *   fp       - the original generic function to instantiate
 *   key      - a string key representing the specific type arguments (used
 *              for lookup)
 *   gftn_gap - growarray containing the parsed type arguments and their names
 *
 * Returns:
 *   Pointer to the new ufunc_T representing the instantiated function,
 *   or NULL if the function already exists or on allocation failure.
 *
 * This function:
 *   - Checks if a function with the given type arguments already exists.
 *   - Allocates and initializes a new function instance with the specific
 *     types.
 *   - Copies and updates all relevant type information (arguments, return
 *     type, etc.).
 *   - Registers the new function in the generic function's lookup table.
 */
    static ufunc_T *
generic_func_add(ufunc_T *fp, char_u *key, garray_T *gftn_gap)
{
    hashtab_T	*ht = &fp->uf_generic_functab;
    long_u	hash;
    hashitem_T	*hi;

    hash = hash_hash(key);
    hi = hash_lookup(ht, key, hash);
    if (!HASHITEM_EMPTY(hi))
	return NULL;

    size_t	keylen = STRLEN(key);
    gfitem_T    *gfitem = alloc(sizeof(gfitem_T) + keylen);
    if (gfitem == NULL)
	return NULL;

    STRCPY(gfitem->gfi_name, key);

    ufunc_T *new_fp = copy_function(fp, (int)(keylen + 2));
    if (new_fp == NULL)
    {
	vim_free(gfitem);
	return NULL;
    }

    int sz = fp->uf_generic_argcount * sizeof(type_T);
    new_fp->uf_generic_type_list = alloc_clear(sz);
    if (new_fp->uf_generic_type_list == NULL)
	return NULL;
    memcpy(new_fp->uf_generic_type_list, fp->uf_generic_type_list, sz);

    sz = fp->uf_generic_argcount * sizeof(generic_T);
    new_fp->uf_generic_args = alloc_clear(sz);
    if (new_fp->uf_generic_args == NULL)
    {
	vim_free(new_fp->uf_generic_type_list);
	return NULL;
    }
    memcpy(new_fp->uf_generic_args, fp->uf_generic_args, sz);

    for (int i = 0; i < fp->uf_generic_argcount; i++)
	new_fp->uf_generic_args[i].gt_type =
				    &new_fp->uf_generic_type_list[i];
    hash_init(&new_fp->uf_generic_functab);

    if (fp->uf_class != NULL)
	new_fp->uf_class = fp->uf_class;

    // Create a new name for the function: name<type1, type2...>
    new_fp->uf_name[new_fp->uf_namelen] =  '<';
    STRCPY(new_fp->uf_name + new_fp->uf_namelen + 1, key);
    new_fp->uf_name[new_fp->uf_namelen + keylen + 1] =  '>';
    new_fp->uf_namelen += keylen + 2;

    if (new_fp->uf_name_exp != NULL)
    {
	char_u	*new_name_exp = alloc(STRLEN(new_fp->uf_name_exp) + keylen + 3);
	if (new_name_exp != NULL)
	{
	    STRCPY(new_name_exp, new_fp->uf_name_exp);
	    STRCAT(new_name_exp, "<");
	    STRCAT(new_name_exp, key);
	    STRCAT(new_name_exp, ">");
	    vim_free(new_fp->uf_name_exp);
	    new_fp->uf_name_exp = new_name_exp;
	}
    }

    gfitem->gfi_ufunc = new_fp;
    gfitem->gfi_ufunc->uf_def_status = UF_TO_BE_COMPILED;

    // create a copy of
    // - all the argument types
    // - return type
    // - vararg type
    // - function type
    // if any generic type is used, it will be replaced below).
    for (int i = 0; i < fp->uf_args.ga_len; i++)
	new_fp->uf_arg_types[i] = copy_type_deep(fp->uf_arg_types[i],
						&new_fp->uf_type_list);

    if (fp->uf_ret_type != NULL)
	new_fp->uf_ret_type = copy_type_deep(fp->uf_ret_type,
						&new_fp->uf_type_list);

    if (fp->uf_va_type != NULL)
	new_fp->uf_va_type = copy_type_deep(fp->uf_va_type,
						&new_fp->uf_type_list);

    if (fp->uf_func_type != NULL)
	new_fp->uf_func_type = copy_type_deep(fp->uf_func_type,
						&new_fp->uf_type_list);

    // Replace the t_any generic types with the actual types
    for (int i = 0; i < fp->uf_generic_argcount; i++)
    {
	gf_type_name_T  *gftn_item;
	gftn_item = (gf_type_name_T *)gftn_gap->ga_data + i;
	generic_T *gt = &new_fp->uf_generic_args[i];
	gt->gt_type = gftn_item->gftn_type;
    }

    // Update any generic types in the function arguments
    for (int i = 0; i < fp->uf_args.ga_len; i++)
	update_generic_type(fp, new_fp, fp->uf_arg_types[i],
			    &new_fp->uf_arg_types[i],
			    &new_fp->uf_func_type->tt_args[i]);

    // Update the vararg type if it uses generic types
    if (fp->uf_va_type != NULL)
	update_generic_type(fp, new_fp, fp->uf_va_type, &new_fp->uf_va_type,
			    NULL);

    // Update the return type if it is a generic type
    if (fp->uf_ret_type != NULL)
	update_generic_type(fp, new_fp, fp->uf_ret_type, &new_fp->uf_ret_type,
			    &new_fp->uf_func_type->tt_member);

    hash_add_item(ht, hi, gfitem->gfi_name, hash);

    return new_fp;
}

/*
 * Looks up a concrete instance of a generic function "fp" using the type
 * arguments specified in "gftn_gap". The lookup key is constructed from the
 * type argument names and stored in "gfkey_gap".
 *
 * Arguments:
 *   fp        - the generic function to search in
 *   gftn_gap  - growarray containing the parsed type arguments and their names
 *   gfkey_gap - growarray used to build and store the lookup key string
 *
 * Returns:
 *   Pointer to the ufunc_T representing the concrete function if found,
 *   or NULL if no matching function exists.
 */
    static ufunc_T *
generic_lookup_func(ufunc_T *fp, garray_T *gftn_gap, garray_T *gfkey_gap)
{
    hashtab_T	*ht = &fp->uf_generic_functab;
    hashitem_T	*hi;

    for (int i = 0; i < gftn_gap->ga_len; i++)
    {
	gf_type_name_T  *gftn_item;

	gftn_item = (gf_type_name_T *)gftn_gap->ga_data + i;
	ga_concat(gfkey_gap, gftn_item->gftn_name);

	if (i != gftn_gap->ga_len - 1)
	{
	    ga_append(gfkey_gap, ',');
	    ga_append(gfkey_gap, ' ');
	}
    }
    ga_append(gfkey_gap, NUL);

    char_u	*key = ((char_u *)gfkey_gap->ga_data);

    hi = hash_find(ht, key);

    if (HASHITEM_EMPTY(hi))
	return NULL;

    gfitem_T	*gfitem = HI2GFITEM(hi);
    return gfitem->gfi_ufunc;
}

/*
 * Returns a concrete instance of the generic function "fp" using the type
 * arguments specified in "gftn_gap". If such an instance does not exist,
 * it is created.
 *
 * Arguments:
 *   fp        - the generic function to instantiate
 *   gftn_gap  - growarray containing the parsed type arguments and their names
 *
 * Returns:
 *   Pointer to the ufunc_T representing the concrete function instance,
 *   or NULL if the type arguments are invalid or on allocation failure.
 *
 * This function:
 *   - Validates the number of type arguments.
 *   - Looks up an existing function instance with the given types.
 *   - If not found, creates and registers a new function instance.
 */
    ufunc_T *
generic_func_get(ufunc_T *fp, garray_T *gftn_gap)
{
    char	*emsg = NULL;

    if (gftn_gap == NULL || gftn_gap->ga_len == 0)
	emsg = e_generic_func_missing_type_args_str;
    else if (gftn_gap->ga_len < fp->uf_generic_argcount)
	emsg = e_not_enough_types_for_generic_function_str;
    else if (gftn_gap->ga_len > fp->uf_generic_argcount)
	emsg = e_too_many_types_for_generic_function_str;

    if (emsg != NULL)
    {
	emsg_funcname(emsg, printable_func_name(fp));
	return NULL;
    }

    // generic function call
    garray_T gfkey_ga;

    ga_init2(&gfkey_ga, 1, 80);

    // Look up the function with specific types
    ufunc_T	*generic_fp = generic_lookup_func(fp, gftn_gap, &gfkey_ga);
    if (generic_fp == NULL)
	// generic function with these type arguments doesn't exist.
	// Create a new one.
	generic_fp = generic_func_add(fp, (char_u *)gfkey_ga.ga_data,
								gftn_gap);
    ga_clear(&gfkey_ga);

    return generic_fp;
}

/*
 * Looks up a concrete instance of a generic function "ufunc" using the type
 * arguments specified after the function name in "name" (starting at name +
 * namelen).
 *
 * Arguments:
 *   ufunc    - the generic function to instantiate or look up
 *   name     - the function name, followed by the type argument list
 *   namelen  - length of the function name (type arguments start at
 *              name + namelen)
 *
 * Returns:
 *   Pointer to the ufunc_T representing the concrete function instance if
 *   successful, or NULL if parsing fails or the instance cannot be created.
 *
 * This function:
 *   - Parses the type arguments from the string after the function name.
 *   - Looks up an existing function instance with those type arguments.
 *   - If not found, creates and registers a new function instance.
 */
    ufunc_T *
find_generic_func(ufunc_T *ufunc, char_u *name, int namelen)
{
    garray_T	gftn_table;
    garray_T	types_ga;
    char_u	*p;
    ufunc_T	*new_ufunc = NULL;

    ga_init2(&gftn_table, sizeof(gf_type_name_T), 10);
    ga_init2(&types_ga, sizeof(type_T *), 10);

    // Get the list of types following the name
    p = parse_generic_func_type_args(name, namelen, name + namelen,
						&types_ga, &gftn_table);
    if (p != NULL)
	new_ufunc = generic_func_get(ufunc, &gftn_table);

    ga_clear(&gftn_table);
    clear_type_list(&types_ga);

    return new_ufunc;
}

/*
 * Searches for a generic type with the given name "type_name" in the generic
 * function "ufunc".
 *
 * Arguments:
 *   type_name - the name of the generic type to search for (as a single
 *               character)
 *   ufunc     - the generic function in which to search for the type
 *
 * Returns:
 *   Pointer to the type_T representing the found generic type,
 *   or NULL if the type is not found or if "ufunc" is not a generic function.
 */
    static type_T *
find_generic_type_in_ufunc(char_u *type_name, ufunc_T *ufunc)
{
    if (!IS_GENERIC_FUNC(ufunc))
	return NULL;

    for (int i = 0; i < ufunc->uf_generic_argcount; i++)
    {
	generic_T *generic;

	generic = ((generic_T *)ufunc->uf_generic_args) + i;
	if (generic->gt_name == *type_name)
	{
	    type_T *type = generic->gt_type;
	    return type;
	}
    }

    return NULL;
}

/*
 * Searches for a generic type with the given name "type_name" in the current
 * function context "cctx" and its outer (enclosing) contexts, if necessary.
 *
 * Arguments:
 *   type_name - the name of the generic type to search for (as a single
 *               character)
 *   cctx      - the current compile context, which may be nested
 *
 * Returns:
 *   Pointer to the type_T representing the found generic type,
 *   or NULL if the type is not found in the current or any outer context.
 */
    static type_T *
find_generic_type_in_cctx(char_u *type_name, cctx_T *cctx)
{
    type_T	*type;

    type = find_generic_type_in_ufunc(type_name, cctx->ctx_ufunc);
    if (type != NULL)
	return type;

    if (cctx->ctx_outer != NULL)
	return find_generic_type_in_cctx(type_name, cctx->ctx_outer);

    return NULL;
}

/*
 * Looks up a generic type with the given name "type_name" in the generic
 * function "ufunc".  If not found, searches in the enclosing compile context
 * "cctx" (for nested functions).
 *
 * Arguments:
 *   type_name - the name of the generic type to search for (as a single
 *               character)
 *   ufunc     - the generic function to search in first (may be NULL)
 *   cctx      - the compile context to search in outer functions if not found
 *               in "ufunc" (may be NULL)
 *
 * Returns:
 *   Pointer to the type_T representing the found generic type, or NULL if the
 *   type is not found in the given function or any outer context.
 */
    type_T *
find_generic_type(char_u *type_name, ufunc_T *ufunc, cctx_T *cctx)
{
    if (ufunc != NULL)
    {
	type_T *type = find_generic_type_in_ufunc(type_name, ufunc);
	if (type != NULL)
	    return type;
    }

    if (cctx != NULL && ufunc != cctx->ctx_ufunc)
	return find_generic_type_in_cctx(type_name, cctx);

    return NULL;
}

/*
 * Frees all concrete function instances stored in the generic function table
 * of "fp". This includes freeing each instantiated function and its
 * associated gfitem_T structure, and clearing the hash table.
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

	    func_clear_free(gfitem->gfi_ufunc, FALSE);
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
    VIM_CLEAR(fp->uf_generic_type_list);
    VIM_CLEAR(fp->uf_generic_args);
    free_generic_functab(fp);
    fp->uf_flags &= ~FC_GENERIC;
}

#endif // FEAT_EVAL
