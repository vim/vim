/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9type.c: handling of types
 */

#define USING_FLOAT_STUFF
#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

#ifdef VMS
# include <float.h>
#endif

/*
 * Allocate memory for a type_T and add the pointer to type_gap, so that it can
 * be easily freed later.
 */
    type_T *
get_type_ptr(garray_T *type_gap)
{
    type_T *type;

    if (ga_grow(type_gap, 1) == FAIL)
	return NULL;
    type = ALLOC_CLEAR_ONE(type_T);
    if (type != NULL)
    {
	((type_T **)type_gap->ga_data)[type_gap->ga_len] = type;
	++type_gap->ga_len;
    }
    return type;
}

    void
clear_type_list(garray_T *gap)
{
    while (gap->ga_len > 0)
	vim_free(((type_T **)gap->ga_data)[--gap->ga_len]);
    ga_clear(gap);
}

/*
 * Take a type that is using entries in a growarray and turn it into a type
 * with allocated entries.
 */
    type_T *
alloc_type(type_T *type)
{
    type_T *ret;

    if (type == NULL)
	return NULL;

    // A fixed type never contains allocated types, return as-is.
    if (type->tt_flags & TTFLAG_STATIC)
	return type;

    ret = ALLOC_ONE(type_T);
    *ret = *type;

    if (ret->tt_member != NULL)
	ret->tt_member = alloc_type(ret->tt_member);
    if (type->tt_args != NULL)
    {
	int i;

	ret->tt_args = ALLOC_MULT(type_T *, type->tt_argcount);
	if (ret->tt_args != NULL)
	    for (i = 0; i < type->tt_argcount; ++i)
		ret->tt_args[i] = alloc_type(type->tt_args[i]);
    }

    return ret;
}

/*
 * Free a type that was created with alloc_type().
 */
    void
free_type(type_T *type)
{
    int i;

    if (type == NULL || (type->tt_flags & TTFLAG_STATIC))
	return;
    if (type->tt_args != NULL)
    {
	for (i = 0; i < type->tt_argcount; ++i)
	    free_type(type->tt_args[i]);
	vim_free(type->tt_args);
    }
    free_type(type->tt_member);
    vim_free(type);
}

    type_T *
get_list_type(type_T *member_type, garray_T *type_gap)
{
    type_T *type;

    // recognize commonly used types
    if (member_type == NULL || member_type->tt_type == VAR_ANY)
	return &t_list_any;
    if (member_type->tt_type == VAR_VOID
	    || member_type->tt_type == VAR_UNKNOWN)
	return &t_list_empty;
    if (member_type->tt_type == VAR_BOOL)
	return &t_list_bool;
    if (member_type->tt_type == VAR_NUMBER)
	return &t_list_number;
    if (member_type->tt_type == VAR_STRING)
	return &t_list_string;

    // Not a common type, create a new entry.
    type = get_type_ptr(type_gap);
    if (type == NULL)
	return &t_any;
    type->tt_type = VAR_LIST;
    type->tt_member = member_type;
    type->tt_argcount = 0;
    type->tt_args = NULL;
    return type;
}

    type_T *
get_dict_type(type_T *member_type, garray_T *type_gap)
{
    type_T *type;

    // recognize commonly used types
    if (member_type == NULL || member_type->tt_type == VAR_ANY)
	return &t_dict_any;
    if (member_type->tt_type == VAR_VOID
	    || member_type->tt_type == VAR_UNKNOWN)
	return &t_dict_empty;
    if (member_type->tt_type == VAR_BOOL)
	return &t_dict_bool;
    if (member_type->tt_type == VAR_NUMBER)
	return &t_dict_number;
    if (member_type->tt_type == VAR_STRING)
	return &t_dict_string;

    // Not a common type, create a new entry.
    type = get_type_ptr(type_gap);
    if (type == NULL)
	return &t_any;
    type->tt_type = VAR_DICT;
    type->tt_member = member_type;
    type->tt_argcount = 0;
    type->tt_args = NULL;
    return type;
}

/*
 * Allocate a new type for a function.
 */
    type_T *
alloc_func_type(type_T *ret_type, int argcount, garray_T *type_gap)
{
    type_T *type = get_type_ptr(type_gap);

    if (type == NULL)
	return &t_any;
    type->tt_type = VAR_FUNC;
    type->tt_member = ret_type;
    type->tt_argcount = argcount;
    type->tt_args = NULL;
    return type;
}

/*
 * Get a function type, based on the return type "ret_type".
 * If "argcount" is -1 or 0 a predefined type can be used.
 * If "argcount" > 0 always create a new type, so that arguments can be added.
 */
    type_T *
get_func_type(type_T *ret_type, int argcount, garray_T *type_gap)
{
    // recognize commonly used types
    if (argcount <= 0)
    {
	if (ret_type == &t_unknown)
	{
	    // (argcount == 0) is not possible
	    return &t_func_unknown;
	}
	if (ret_type == &t_void)
	{
	    if (argcount == 0)
		return &t_func_0_void;
	    else
		return &t_func_void;
	}
	if (ret_type == &t_any)
	{
	    if (argcount == 0)
		return &t_func_0_any;
	    else
		return &t_func_any;
	}
	if (ret_type == &t_number)
	{
	    if (argcount == 0)
		return &t_func_0_number;
	    else
		return &t_func_number;
	}
	if (ret_type == &t_string)
	{
	    if (argcount == 0)
		return &t_func_0_string;
	    else
		return &t_func_string;
	}
    }

    return alloc_func_type(ret_type, argcount, type_gap);
}

/*
 * For a function type, reserve space for "argcount" argument types (including
 * vararg).
 */
    int
func_type_add_arg_types(
	type_T	    *functype,
	int	    argcount,
	garray_T    *type_gap)
{
    // To make it easy to free the space needed for the argument types, add the
    // pointer to type_gap.
    if (ga_grow(type_gap, 1) == FAIL)
	return FAIL;
    functype->tt_args = ALLOC_CLEAR_MULT(type_T *, argcount);
    if (functype->tt_args == NULL)
	return FAIL;
    ((type_T **)type_gap->ga_data)[type_gap->ga_len] =
						     (void *)functype->tt_args;
    ++type_gap->ga_len;
    return OK;
}

/*
 * Get a type_T for a typval_T.
 * "type_gap" is used to temporarily create types in.
 */
    static type_T *
typval2type_int(typval_T *tv, garray_T *type_gap)
{
    type_T  *type;
    type_T  *member_type = &t_any;
    int	    argcount = 0;

    if (tv->v_type == VAR_NUMBER)
	return &t_number;
    if (tv->v_type == VAR_BOOL)
	return &t_bool;
    if (tv->v_type == VAR_STRING)
	return &t_string;

    if (tv->v_type == VAR_LIST)
    {
	list_T	    *l = tv->vval.v_list;
	listitem_T  *li;

	if (l == NULL || l->lv_first == NULL)
	    return &t_list_empty;
	if (l->lv_first == &range_list_item)
	    return &t_list_number;

	// Use the common type of all members.
	member_type = typval2type(&l->lv_first->li_tv, type_gap);
	for (li = l->lv_first->li_next; li != NULL; li = li->li_next)
	    common_type(typval2type(&li->li_tv, type_gap),
					  member_type, &member_type, type_gap);
	return get_list_type(member_type, type_gap);
    }

    if (tv->v_type == VAR_DICT)
    {
	dict_iterator_T iter;
	typval_T	*value;

	if (tv->vval.v_dict == NULL
				   || tv->vval.v_dict->dv_hashtab.ht_used == 0)
	    return &t_dict_empty;

	// Use the common type of all values.
	dict_iterate_start(tv, &iter);
	dict_iterate_next(&iter, &value);
	member_type = typval2type(value, type_gap);
	while (dict_iterate_next(&iter, &value) != NULL)
	    common_type(typval2type(value, type_gap),
					  member_type, &member_type, type_gap);
	return get_dict_type(member_type, type_gap);
    }

    if (tv->v_type == VAR_FUNC || tv->v_type == VAR_PARTIAL)
    {
	char_u	*name = NULL;
	ufunc_T *ufunc = NULL;

	if (tv->v_type == VAR_PARTIAL)
	{
	    if (tv->vval.v_partial->pt_func != NULL)
		ufunc = tv->vval.v_partial->pt_func;
	    else
		name = tv->vval.v_partial->pt_name;
	}
	else
	    name = tv->vval.v_string;
	if (name != NULL)
	{
	    int idx = find_internal_func(name);

	    if (idx >= 0)
	    {
		// TODO: get actual arg count and types
		argcount = -1;
		member_type = internal_func_ret_type(idx, 0, NULL);
	    }
	    else
		ufunc = find_func(name, FALSE, NULL);
	}
	if (ufunc != NULL)
	{
	    // May need to get the argument types from default values by
	    // compiling the function.
	    if (ufunc->uf_def_status == UF_TO_BE_COMPILED
			    && compile_def_function(ufunc, TRUE, NULL) == FAIL)
		return NULL;
	    if (ufunc->uf_func_type != NULL)
		return ufunc->uf_func_type;
	}
    }

    type = get_type_ptr(type_gap);
    if (type == NULL)
	return NULL;
    type->tt_type = tv->v_type;
    type->tt_argcount = argcount;
    type->tt_member = member_type;

    return type;
}

/*
 * Return TRUE if "tv" is not a bool but should be converted to bool.
 */
    int
need_convert_to_bool(type_T *type, typval_T *tv)
{
    return type != NULL && type == &t_bool && tv->v_type != VAR_BOOL
	    && (tv->v_type == VAR_NUMBER
		       && (tv->vval.v_number == 0 || tv->vval.v_number == 1));
}

/*
 * Get a type_T for a typval_T.
 * "type_list" is used to temporarily create types in.
 */
    type_T *
typval2type(typval_T *tv, garray_T *type_gap)
{
    type_T *type = typval2type_int(tv, type_gap);

    if (type != NULL && type != &t_bool
	    && (tv->v_type == VAR_NUMBER
		    && (tv->vval.v_number == 0 || tv->vval.v_number == 1)))
    {
	type_T *newtype = get_type_ptr(type_gap);

	// Number 0 and 1 and expression with "&&" or "||" can also be used
	// for bool.
	if (newtype != NULL)
	{
	    *newtype = *type;
	    newtype->tt_flags = TTFLAG_BOOL_OK;
	    type = newtype;
	}
    }
    return type;
}

/*
 * Get a type_T for a typval_T, used for v: variables.
 * "type_list" is used to temporarily create types in.
 */
    type_T *
typval2type_vimvar(typval_T *tv, garray_T *type_gap)
{
    if (tv->v_type == VAR_LIST)  // e.g. for v:oldfiles
	return &t_list_string;
    if (tv->v_type == VAR_DICT)  // e.g. for v:completed_item
	return &t_dict_any;
    return typval2type(tv, type_gap);
}


/*
 * Return FAIL if "expected" and "actual" don't match.
 * When "argidx" > 0 it is included in the error message.
 */
    int
check_typval_type(type_T *expected, typval_T *actual_tv, int argidx)
{
    garray_T	type_list;
    type_T	*actual_type;
    int		res = FAIL;

    ga_init2(&type_list, sizeof(type_T *), 10);
    actual_type = typval2type(actual_tv, &type_list);
    if (actual_type != NULL)
	res = check_type(expected, actual_type, TRUE, argidx);
    clear_type_list(&type_list);
    return res;
}

    void
type_mismatch(type_T *expected, type_T *actual)
{
    arg_type_mismatch(expected, actual, 0);
}

    void
arg_type_mismatch(type_T *expected, type_T *actual, int argidx)
{
    char *tofree1, *tofree2;
    char *typename1 = type_name(expected, &tofree1);
    char *typename2 = type_name(actual, &tofree2);

    if (argidx > 0)
	semsg(_(e_argument_nr_type_mismatch_expected_str_but_got_str),
						 argidx, typename1, typename2);
    else
	semsg(_(e_type_mismatch_expected_str_but_got_str),
							 typename1, typename2);
    vim_free(tofree1);
    vim_free(tofree2);
}

/*
 * Check if the expected and actual types match.
 * Does not allow for assigning "any" to a specific type.
 * When "argidx" > 0 it is included in the error message.
 */
    int
check_type(type_T *expected, type_T *actual, int give_msg, int argidx)
{
    int ret = OK;

    // When expected is "unknown" we accept any actual type.
    // When expected is "any" we accept any actual type except "void".
    if (expected->tt_type != VAR_UNKNOWN
	    && !(expected->tt_type == VAR_ANY && actual->tt_type != VAR_VOID))

    {
	// tt_type should match, except that a "partial" can be assigned to a
	// variable with type "func".
	if (!(expected->tt_type == actual->tt_type
		    || (expected->tt_type == VAR_FUNC
					   && actual->tt_type == VAR_PARTIAL)))
	{
	    if (expected->tt_type == VAR_BOOL
					&& (actual->tt_flags & TTFLAG_BOOL_OK))
		// Using number 0 or 1 for bool is OK.
		return OK;
	    if (give_msg)
		arg_type_mismatch(expected, actual, argidx);
	    return FAIL;
	}
	if (expected->tt_type == VAR_DICT || expected->tt_type == VAR_LIST)
	{
	    // "unknown" is used for an empty list or dict
	    if (actual->tt_member != &t_unknown)
		ret = check_type(expected->tt_member, actual->tt_member,
								     FALSE, 0);
	}
	else if (expected->tt_type == VAR_FUNC)
	{
	    if (expected->tt_member != &t_unknown)
		ret = check_type(expected->tt_member, actual->tt_member,
								     FALSE, 0);
	    if (ret == OK && expected->tt_argcount != -1
		    && actual->tt_argcount != -1
		    && (actual->tt_argcount < expected->tt_min_argcount
			|| actual->tt_argcount > expected->tt_argcount))
		    ret = FAIL;
	    if (expected->tt_args != NULL && actual->tt_args != NULL)
	    {
		int i;

		for (i = 0; i < expected->tt_argcount; ++i)
		    // Allow for using "any" argument type, lambda's have them.
		    if (actual->tt_args[i] != &t_any && check_type(
			    expected->tt_args[i], actual->tt_args[i], FALSE, 0)
								       == FAIL)
		    {
			ret = FAIL;
			break;
		    }
	    }
	}
	if (ret == FAIL && give_msg)
	    arg_type_mismatch(expected, actual, argidx);
    }
    return ret;
}

/*
 * Like check_type() but also allow for a runtime type check. E.g. "any" can be
 * used for "number".
 */
    int
check_arg_type(type_T *expected, type_T *actual, int argidx)
{
    if (check_type(expected, actual, FALSE, 0) == OK
					    || use_typecheck(actual, expected))
	return OK;
    // TODO: should generate a TYPECHECK instruction.
    return check_type(expected, actual, TRUE, argidx);
}

/*
 * Skip over a type definition and return a pointer to just after it.
 * When "optional" is TRUE then a leading "?" is accepted.
 */
    char_u *
skip_type(char_u *start, int optional)
{
    char_u *p = start;

    if (optional && *p == '?')
	++p;
    while (ASCII_ISALNUM(*p) || *p == '_')
	++p;

    // Skip over "<type>"; this is permissive about white space.
    if (*skipwhite(p) == '<')
    {
	p = skipwhite(p);
	p = skip_type(skipwhite(p + 1), FALSE);
	p = skipwhite(p);
	if (*p == '>')
	    ++p;
    }
    else if ((*p == '(' || (*p == ':' && VIM_ISWHITE(p[1])))
					     && STRNCMP("func", start, 4) == 0)
    {
	if (*p == '(')
	{
	    // handle func(args): type
	    ++p;
	    while (*p != ')' && *p != NUL)
	    {
		char_u *sp = p;

		if (STRNCMP(p, "...", 3) == 0)
		    p += 3;
		p = skip_type(p, TRUE);
		if (p == sp)
		    return p;  // syntax error
		if (*p == ',')
		    p = skipwhite(p + 1);
	    }
	    if (*p == ')')
	    {
		if (p[1] == ':')
		    p = skip_type(skipwhite(p + 2), FALSE);
		else
		    ++p;
	    }
	}
	else
	{
	    // handle func: return_type
	    p = skip_type(skipwhite(p + 1), FALSE);
	}
    }

    return p;
}

/*
 * Parse the member type: "<type>" and return "type" with the member set.
 * Use "type_gap" if a new type needs to be added.
 * Returns NULL in case of failure.
 */
    static type_T *
parse_type_member(char_u **arg, type_T *type, garray_T *type_gap)
{
    type_T  *member_type;
    int	    prev_called_emsg = called_emsg;

    if (**arg != '<')
    {
	if (*skipwhite(*arg) == '<')
	    semsg(_(e_no_white_space_allowed_before_str), "<");
	else
	    emsg(_(e_missing_type));
	return type;
    }
    *arg = skipwhite(*arg + 1);

    member_type = parse_type(arg, type_gap);

    *arg = skipwhite(*arg);
    if (**arg != '>' && called_emsg == prev_called_emsg)
    {
	emsg(_(e_missing_gt_after_type));
	return type;
    }
    ++*arg;

    if (type->tt_type == VAR_LIST)
	return get_list_type(member_type, type_gap);
    return get_dict_type(member_type, type_gap);
}

/*
 * Parse a type at "arg" and advance over it.
 * Return &t_any for failure.
 */
    type_T *
parse_type(char_u **arg, garray_T *type_gap)
{
    char_u  *p = *arg;
    size_t  len;

    // skip over the first word
    while (ASCII_ISALNUM(*p) || *p == '_')
	++p;
    len = p - *arg;

    switch (**arg)
    {
	case 'a':
	    if (len == 3 && STRNCMP(*arg, "any", len) == 0)
	    {
		*arg += len;
		return &t_any;
	    }
	    break;
	case 'b':
	    if (len == 4 && STRNCMP(*arg, "bool", len) == 0)
	    {
		*arg += len;
		return &t_bool;
	    }
	    if (len == 4 && STRNCMP(*arg, "blob", len) == 0)
	    {
		*arg += len;
		return &t_blob;
	    }
	    break;
	case 'c':
	    if (len == 7 && STRNCMP(*arg, "channel", len) == 0)
	    {
		*arg += len;
		return &t_channel;
	    }
	    break;
	case 'd':
	    if (len == 4 && STRNCMP(*arg, "dict", len) == 0)
	    {
		*arg += len;
		return parse_type_member(arg, &t_dict_any, type_gap);
	    }
	    break;
	case 'f':
	    if (len == 5 && STRNCMP(*arg, "float", len) == 0)
	    {
#ifdef FEAT_FLOAT
		*arg += len;
		return &t_float;
#else
		emsg(_(e_this_vim_is_not_compiled_with_float_support));
		return &t_any;
#endif
	    }
	    if (len == 4 && STRNCMP(*arg, "func", len) == 0)
	    {
		type_T  *type;
		type_T  *ret_type = &t_unknown;
		int	argcount = -1;
		int	flags = 0;
		int	first_optional = -1;
		type_T	*arg_type[MAX_FUNC_ARGS + 1];

		// func({type}, ...{type}): {type}
		*arg += len;
		if (**arg == '(')
		{
		    // "func" may or may not return a value, "func()" does
		    // not return a value.
		    ret_type = &t_void;

		    p = ++*arg;
		    argcount = 0;
		    while (*p != NUL && *p != ')')
		    {
			if (*p == '?')
			{
			    if (first_optional == -1)
				first_optional = argcount;
			    ++p;
			}
			else if (STRNCMP(p, "...", 3) == 0)
			{
			    flags |= TTFLAG_VARARGS;
			    p += 3;
			}
			else if (first_optional != -1)
			{
			    emsg(_(e_mandatory_argument_after_optional_argument));
			    return &t_any;
			}

			arg_type[argcount++] = parse_type(&p, type_gap);

			// Nothing comes after "...{type}".
			if (flags & TTFLAG_VARARGS)
			    break;

			if (*p != ',' && *skipwhite(p) == ',')
			{
			    semsg(_(e_no_white_space_allowed_before_str), ",");
			    return &t_any;
			}
			if (*p == ',')
			{
			    ++p;
			    if (!VIM_ISWHITE(*p))
			    {
				semsg(_(e_white_space_required_after_str), ",");
				return &t_any;
			    }
			}
			p = skipwhite(p);
			if (argcount == MAX_FUNC_ARGS)
			{
			    emsg(_(e_too_many_argument_types));
			    return &t_any;
			}
		    }

		    p = skipwhite(p);
		    if (*p != ')')
		    {
			emsg(_(e_missing_close));
			return &t_any;
		    }
		    *arg = p + 1;
		}
		if (**arg == ':')
		{
		    // parse return type
		    ++*arg;
		    if (!VIM_ISWHITE(**arg))
			semsg(_(e_white_space_required_after_str), ":");
		    *arg = skipwhite(*arg);
		    ret_type = parse_type(arg, type_gap);
		}
		if (flags == 0 && first_optional == -1 && argcount <= 0)
		    type = get_func_type(ret_type, argcount, type_gap);
		else
		{
		    type = alloc_func_type(ret_type, argcount, type_gap);
		    type->tt_flags = flags;
		    if (argcount > 0)
		    {
			type->tt_argcount = argcount;
			type->tt_min_argcount = first_optional == -1
						   ? argcount : first_optional;
			if (func_type_add_arg_types(type, argcount,
							     type_gap) == FAIL)
			    return &t_any;
			mch_memmove(type->tt_args, arg_type,
						  sizeof(type_T *) * argcount);
		    }
		}
		return type;
	    }
	    break;
	case 'j':
	    if (len == 3 && STRNCMP(*arg, "job", len) == 0)
	    {
		*arg += len;
		return &t_job;
	    }
	    break;
	case 'l':
	    if (len == 4 && STRNCMP(*arg, "list", len) == 0)
	    {
		*arg += len;
		return parse_type_member(arg, &t_list_any, type_gap);
	    }
	    break;
	case 'n':
	    if (len == 6 && STRNCMP(*arg, "number", len) == 0)
	    {
		*arg += len;
		return &t_number;
	    }
	    break;
	case 's':
	    if (len == 6 && STRNCMP(*arg, "string", len) == 0)
	    {
		*arg += len;
		return &t_string;
	    }
	    break;
	case 'v':
	    if (len == 4 && STRNCMP(*arg, "void", len) == 0)
	    {
		*arg += len;
		return &t_void;
	    }
	    break;
    }

    semsg(_(e_type_not_recognized_str), *arg);
    return &t_any;
}

/*
 * Check if "type1" and "type2" are exactly the same.
 */
    static int
equal_type(type_T *type1, type_T *type2)
{
    int i;

    if (type1->tt_type != type2->tt_type)
	return FALSE;
    switch (type1->tt_type)
    {
	case VAR_UNKNOWN:
	case VAR_ANY:
	case VAR_VOID:
	case VAR_SPECIAL:
	case VAR_BOOL:
	case VAR_NUMBER:
	case VAR_FLOAT:
	case VAR_STRING:
	case VAR_BLOB:
	case VAR_JOB:
	case VAR_CHANNEL:
	    break;  // not composite is always OK
	case VAR_LIST:
	case VAR_DICT:
	    return equal_type(type1->tt_member, type2->tt_member);
	case VAR_FUNC:
	case VAR_PARTIAL:
	    if (!equal_type(type1->tt_member, type2->tt_member)
		    || type1->tt_argcount != type2->tt_argcount)
		return FALSE;
	    if (type1->tt_argcount < 0
			   || type1->tt_args == NULL || type2->tt_args == NULL)
		return TRUE;
	    for (i = 0; i < type1->tt_argcount; ++i)
		if (!equal_type(type1->tt_args[i], type2->tt_args[i]))
		    return FALSE;
	    return TRUE;
    }
    return TRUE;
}

/*
 * Find the common type of "type1" and "type2" and put it in "dest".
 * "type2" and "dest" may be the same.
 */
    void
common_type(type_T *type1, type_T *type2, type_T **dest, garray_T *type_gap)
{
    if (equal_type(type1, type2))
    {
	*dest = type1;
	return;
    }

    // If either is VAR_UNKNOWN use the other type.  An empty list/dict has no
    // specific type.
    if (type1->tt_type == VAR_UNKNOWN)
    {
	*dest = type2;
	return;
    }
    if (type2->tt_type == VAR_UNKNOWN)
    {
	*dest = type1;
	return;
    }

    if (type1->tt_type == type2->tt_type)
    {
	if (type1->tt_type == VAR_LIST || type2->tt_type == VAR_DICT)
	{
	    type_T *common;

	    common_type(type1->tt_member, type2->tt_member, &common, type_gap);
	    if (type1->tt_type == VAR_LIST)
		*dest = get_list_type(common, type_gap);
	    else
		*dest = get_dict_type(common, type_gap);
	    return;
	}
	if (type1->tt_type == VAR_FUNC)
	{
	    type_T *common;

	    common_type(type1->tt_member, type2->tt_member, &common, type_gap);
	    if (type1->tt_argcount == type2->tt_argcount
						    && type1->tt_argcount >= 0)
	    {
		int argcount = type1->tt_argcount;
		int i;

		*dest = alloc_func_type(common, argcount, type_gap);
		if (type1->tt_args != NULL && type2->tt_args != NULL)
		{
		    if (func_type_add_arg_types(*dest, argcount,
							     type_gap) == OK)
			for (i = 0; i < argcount; ++i)
			    common_type(type1->tt_args[i], type2->tt_args[i],
					       &(*dest)->tt_args[i], type_gap);
		}
	    }
	    else
		*dest = alloc_func_type(common, -1, type_gap);
	    // Use the minimum of min_argcount.
	    (*dest)->tt_min_argcount =
			type1->tt_min_argcount < type2->tt_min_argcount
			     ? type1->tt_min_argcount : type2->tt_min_argcount;
	    return;
	}
    }

    *dest = &t_any;
}

/*
 * Get the member type of a dict or list from the items on the stack.
 * "stack_top" points just after the last type on the type stack.
 * For a list "skip" is 1, for a dict "skip" is 2, keys are skipped.
 * Returns &t_void for an empty list or dict.
 * Otherwise finds the common type of all items.
 */
    type_T *
get_member_type_from_stack(
	type_T	    **stack_top,
	int	    count,
	int	    skip,
	garray_T    *type_gap)
{
    int	    i;
    type_T  *result;
    type_T  *type;

    // Use "any" for an empty list or dict.
    if (count == 0)
	return &t_unknown;

    // Use the first value type for the list member type, then find the common
    // type from following items.
    result = *(stack_top -(count * skip) + skip - 1);
    for (i = 1; i < count; ++i)
    {
	if (result == &t_any)
	    break;  // won't get more common
	type = *(stack_top -((count - i) * skip) + skip - 1);
	common_type(type, result, &result, type_gap);
    }

    return result;
}

    char *
vartype_name(vartype_T type)
{
    switch (type)
    {
	case VAR_UNKNOWN: break;
	case VAR_ANY: return "any";
	case VAR_VOID: return "void";
	case VAR_SPECIAL: return "special";
	case VAR_BOOL: return "bool";
	case VAR_NUMBER: return "number";
	case VAR_FLOAT: return "float";
	case VAR_STRING: return "string";
	case VAR_BLOB: return "blob";
	case VAR_JOB: return "job";
	case VAR_CHANNEL: return "channel";
	case VAR_LIST: return "list";
	case VAR_DICT: return "dict";

	case VAR_FUNC:
	case VAR_PARTIAL: return "func";
    }
    return "unknown";
}

/*
 * Return the name of a type.
 * The result may be in allocated memory, in which case "tofree" is set.
 */
    char *
type_name(type_T *type, char **tofree)
{
    char *name = vartype_name(type->tt_type);

    *tofree = NULL;
    if (type->tt_type == VAR_LIST || type->tt_type == VAR_DICT)
    {
	char *member_free;
	char *member_name = type_name(type->tt_member, &member_free);
	size_t len;

	len = STRLEN(name) + STRLEN(member_name) + 3;
	*tofree = alloc(len);
	if (*tofree != NULL)
	{
	    vim_snprintf(*tofree, len, "%s<%s>", name, member_name);
	    vim_free(member_free);
	    return *tofree;
	}
    }
    if (type->tt_type == VAR_FUNC)
    {
	garray_T    ga;
	int	    i;
	int	    varargs = (type->tt_flags & TTFLAG_VARARGS) ? 1 : 0;

	ga_init2(&ga, 1, 100);
	if (ga_grow(&ga, 20) == FAIL)
	    return "[unknown]";
	STRCPY(ga.ga_data, "func(");
	ga.ga_len += 5;

	for (i = 0; i < type->tt_argcount; ++i)
	{
	    char *arg_free;
	    char *arg_type;
	    int  len;

	    if (type->tt_args == NULL)
		arg_type = "[unknown]";
	    else
		arg_type = type_name(type->tt_args[i], &arg_free);
	    if (i > 0)
	    {
		STRCPY((char *)ga.ga_data + ga.ga_len, ", ");
		ga.ga_len += 2;
	    }
	    len = (int)STRLEN(arg_type);
	    if (ga_grow(&ga, len + 8) == FAIL)
	    {
		vim_free(arg_free);
		ga_clear(&ga);
		return "[unknown]";
	    }
	    if (varargs && i == type->tt_argcount - 1)
		ga_concat(&ga, (char_u *)"...");
	    else if (i >= type->tt_min_argcount)
		*((char *)ga.ga_data + ga.ga_len++) = '?';
	    ga_concat(&ga, (char_u *)arg_type);
	    vim_free(arg_free);
	}
	if (type->tt_argcount < 0)
	    // any number of arguments
	    ga_concat(&ga, (char_u *)"...");

	if (type->tt_member == &t_void)
	    STRCPY((char *)ga.ga_data + ga.ga_len, ")");
	else
	{
	    char *ret_free;
	    char *ret_name = type_name(type->tt_member, &ret_free);
	    int  len;

	    len = (int)STRLEN(ret_name) + 4;
	    if (ga_grow(&ga, len) == FAIL)
	    {
		vim_free(ret_free);
		ga_clear(&ga);
		return "[unknown]";
	    }
	    STRCPY((char *)ga.ga_data + ga.ga_len, "): ");
	    STRCPY((char *)ga.ga_data + ga.ga_len + 3, ret_name);
	    vim_free(ret_free);
	}
	*tofree = ga.ga_data;
	return ga.ga_data;
    }

    return name;
}

#endif // FEAT_EVAL
