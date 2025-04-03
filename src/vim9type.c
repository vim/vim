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

// When not generating protos this is included in proto.h
#ifdef PROTO
# include "vim9.h"
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
    if (type == NULL)
	return NULL;

    ((type_T **)type_gap->ga_data)[type_gap->ga_len] = type;
    ++type_gap->ga_len;
    return type;
}

/*
 * Make a shallow copy of "type".
 * When allocation fails returns "type".
 */
    type_T *
copy_type(type_T *type, garray_T *type_gap)
{
    type_T *copy = get_type_ptr(type_gap);

    if (copy == NULL)
	return type;
    *copy = *type;
    copy->tt_flags &= ~TTFLAG_STATIC;

    if (type->tt_args != NULL
	   && func_type_add_arg_types(copy, type->tt_argcount, type_gap) == OK)
	for (int i = 0; i < type->tt_argcount; ++i)
	    copy->tt_args[i] = type->tt_args[i];

    return copy;
}

/*
 * Inner part of copy_type_deep().
 * When allocation fails returns "type".
 */
    static type_T *
copy_type_deep_rec(type_T *type, garray_T *type_gap, garray_T *seen_types)
{
    for (int i = 0; i < seen_types->ga_len; ++i)
	if (((type_T **)seen_types->ga_data)[i * 2] == type)
	    // seen this type before, return the copy we made
	    return ((type_T **)seen_types->ga_data)[i * 2 + 1];

    type_T *copy = copy_type(type, type_gap);
    if (ga_grow(seen_types, 1) == FAIL)
	return copy;
    ((type_T **)seen_types->ga_data)[seen_types->ga_len * 2] = type;
    ((type_T **)seen_types->ga_data)[seen_types->ga_len * 2 + 1] = copy;
    ++seen_types->ga_len;

    if (copy->tt_member != NULL)
	copy->tt_member = copy_type_deep_rec(copy->tt_member,
							 type_gap, seen_types);
    if (type->tt_args != NULL)
	for (int i = 0; i < type->tt_argcount; ++i)
	    copy->tt_args[i] = copy_type_deep_rec(copy->tt_args[i],
							 type_gap, seen_types);

    return copy;
}

/*
 * Make a deep copy of "type".
 * When allocation fails returns "type".
 */
    static type_T *
copy_type_deep(type_T *type, garray_T *type_gap)
{
    garray_T seen_types;
    // stores type pairs : a type we have seen and the copy used
    ga_init2(&seen_types, sizeof(type_T *) * 2, 20);

    type_T *res = copy_type_deep_rec(type, type_gap, &seen_types);

    ga_clear(&seen_types);
    return res;
}

    void
clear_type_list(garray_T *gap)
{
    while (gap->ga_len > 0)
	vim_free(((type_T **)gap->ga_data)[--gap->ga_len]);
    ga_clear(gap);
}

    void
clear_func_type_list(garray_T *gap, type_T **func_type)
{
    while (gap->ga_len > 0)
    {
	// func_type pointing to the uf_type_list, so reset pointer
	if (*func_type == ((type_T **)gap->ga_data)[--gap->ga_len])
	    *func_type = &t_func_any;
	vim_free(((type_T **)gap->ga_data)[gap->ga_len]);
    }
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

    if (type->tt_argcount > 0 && type->tt_args != NULL)
    {
	int i;

	ret->tt_args = ALLOC_MULT(type_T *, type->tt_argcount);
	if (ret->tt_args != NULL)
	    for (i = 0; i < type->tt_argcount; ++i)
		ret->tt_args[i] = alloc_type(type->tt_args[i]);
    }
    else
	ret->tt_args = NULL;

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

/*
 * Return TRUE if "type" is to be recursed into for setting the type.
 */
    static int
set_tv_type_recurse(type_T *type)
{
    return type->tt_member != NULL
		&& (type->tt_member->tt_type == VAR_DICT
				|| type->tt_member->tt_type == VAR_LIST)
		&& type->tt_member->tt_member != NULL
		&& type->tt_member->tt_member != &t_any
		&& type->tt_member->tt_member != &t_unknown;
}

/*
 * Set the type of Dict "d" to "type"
 */
    static void
set_tv_type_dict(dict_T *d, type_T *type)
{
    if (d->dv_type == type)
	return;

    free_type(d->dv_type);
    d->dv_type = alloc_type(type);

    // Need to recursively set the type of dict items?
    if (!set_tv_type_recurse(type))
	return;

    int		todo = (int)d->dv_hashtab.ht_used;
    hashitem_T	*hi;
    dictitem_T	*di;

    FOR_ALL_HASHTAB_ITEMS(&d->dv_hashtab, hi, todo)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    di = HI2DI(hi);
	    set_tv_type(&di->di_tv, type->tt_member);
	}
    }
}

/*
 * Set the type of List "l" to "type"
 */
    static void
set_tv_type_list(list_T *l, type_T *type)
{
    if (l->lv_type == type)
	return;

    free_type(l->lv_type);
    l->lv_type = alloc_type(type);

    // Need to recursively set the type of list items?
    if (l->lv_first == &range_list_item || !set_tv_type_recurse(type))
	return;

    listitem_T	*li;

    FOR_ALL_LIST_ITEMS(l, li)
	set_tv_type(&li->li_tv, type->tt_member);
}

/*
 * Set the type of Tuple "tuple" to "type"
 */
    static void
set_tv_type_tuple(tuple_T *tuple, type_T *type)
{
    if (tuple->tv_type == type)
	return;

    free_type(tuple->tv_type);
    tuple->tv_type = alloc_type(type);

    if (type->tt_argcount <= 0)
	return;

    // recursively set the type of list items
    type_T	*item_type;
    for (int i = 0; i < tuple_len(tuple); i++)
    {
	if ((type->tt_flags & TTFLAG_VARARGS) && (i >= type->tt_argcount - 1))
	    // For a variadic tuple, the last type is a List.  So use the
	    // List member type.
	    item_type = type->tt_args[type->tt_argcount - 1]->tt_member;
	else
	    item_type = type->tt_args[i];

	set_tv_type(TUPLE_ITEM(tuple, i), item_type);
    }
}

/*
 * Set the type of "tv" to "type" if it is a list or tuple or dict.
 */
    void
set_tv_type(typval_T *tv, type_T *type)
{
    if (type->tt_type == VAR_ANY)
	// If the variable type is "any", then keep the value type.
	// e.g.  var x: any = [1, 2] or var y: any = {v: 1}
	return;

    if (tv->v_type == VAR_DICT && tv->vval.v_dict != NULL)
	set_tv_type_dict(tv->vval.v_dict, type);
    else if (tv->v_type == VAR_LIST && tv->vval.v_list != NULL)
	set_tv_type_list(tv->vval.v_list, type);
    else if (tv->v_type == VAR_TUPLE && tv->vval.v_tuple != NULL)
	set_tv_type_tuple(tv->vval.v_tuple, type);
}

/*
 * For a tuple type, reserve space for "typecount" types (including the
 * repeated type).
 */
    static int
tuple_type_add_types(
    type_T	*tupletype,
    int		typecount,
    garray_T	*type_gap)
{
    // To make it easy to free the space needed for the types, add the
    // pointer to type_gap.
    if (ga_grow(type_gap, 1) == FAIL)
	return FAIL;
    tupletype->tt_args = ALLOC_CLEAR_MULT(type_T *, typecount);
    if (tupletype->tt_args == NULL)
	return FAIL;
    ((type_T **)type_gap->ga_data)[type_gap->ga_len] =
						(void *)tupletype->tt_args;
    ++type_gap->ga_len;
    return OK;
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

/*
 * Create and return a tuple type from the tuple item types in
 * "tuple_types_ga".
 */
    type_T *
get_tuple_type(
    garray_T	*tuple_types_gap,
    garray_T	*type_gap)
{
    type_T	*type;
    type_T	**tuple_types = tuple_types_gap->ga_data;
    int		typecount = tuple_types_gap->ga_len;

    // recognize commonly used types
    if (typecount == 0)
	return &t_tuple_any;

    // Not a common type, create a new entry.
    type = get_type_ptr(type_gap);
    if (type == NULL)
	return &t_any;
    type->tt_type = VAR_TUPLE;
    type->tt_member = NULL;
    if (typecount > 0)
    {
	if (tuple_type_add_types(type, typecount, type_gap) == FAIL)
	    return NULL;
	mch_memmove(type->tt_args, tuple_types, sizeof(type_T *) * typecount);
    }
    type->tt_argcount = typecount;
    type->tt_flags = 0;

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
    type->tt_member = ret_type == NULL ? &t_unknown : ret_type;
    type->tt_argcount = argcount;
    type->tt_args = NULL;
    return type;
}

/*
 * Allocate a new type for a tuple.
 */
    static type_T *
alloc_tuple_type(int typecount, garray_T *type_gap)
{
    type_T *type = get_type_ptr(type_gap);

    if (type == NULL)
	return &t_any;
    type->tt_type = VAR_TUPLE;
    type->tt_member = NULL;
    type->tt_argcount = typecount;
    type->tt_args = NULL;
    return type;
}

/*
 * Get a function type, based on the return type "ret_type".
 * "argcount" must be -1 or 0, a predefined type can be used.
 */
    type_T *
get_func_type(type_T *ret_type, int argcount, garray_T *type_gap)
{
    // recognize commonly used types
    if (ret_type == &t_unknown || ret_type == NULL)
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
 * Return TRUE if "type" is NULL, any or unknown.
 * This also works for const (comparing with &t_any and &t_unknown doesn't).
 */
    int
type_any_or_unknown(type_T *type)
{
    return type == NULL || type->tt_type == VAR_ANY
					|| type->tt_type == VAR_UNKNOWN;
}

/*
 * Get a type_T for a "special" typval in "tv".
 */
    static type_T *
special_typval2type(typval_T *tv)
{
    switch (tv->vval.v_number)
    {
	case VVAL_NULL:
	    return &t_null;

	case VVAL_NONE:
	    return &t_none;

	case VVAL_TRUE:
	case VVAL_FALSE:
	    return &t_bool;

	default:
	    return &t_unknown;
    }
}

/*
 * Get a type_T for a List typval in "tv".
 * When "flags" has TVTT_DO_MEMBER also get the member type, otherwise use
 * "any".
 * When "flags" has TVTT_MORE_SPECIFIC get the more specific member type if it
 * is "any".
 */
    static type_T *
list_typval2type(typval_T *tv, int copyID, garray_T *type_gap, int flags)
{
    list_T	*l = tv->vval.v_list;
    listitem_T	*li;
    type_T	*member_type = NULL;

    // An empty list has type list<unknown>, unless the type was specified
    // and is not list<any>.  This matters when assigning to a variable
    // with a specific list type.
    if (l == NULL || (l->lv_first == NULL
		&& (l->lv_type == NULL || l->lv_type->tt_member == &t_any)))
	return &t_list_empty;

    if ((flags & TVTT_DO_MEMBER) == 0)
	return &t_list_any;

    // If the type is list<any> go through the members, it may end up a
    // more specific type.
    if (l->lv_type != NULL && (l->lv_first == NULL
		|| (flags & TVTT_MORE_SPECIFIC) == 0
		|| l->lv_type->tt_member != &t_any))
	// make a copy, lv_type may be freed if the list is freed
	return copy_type_deep(l->lv_type, type_gap);

    if (l->lv_first == &range_list_item)
	return &t_list_number;

    if (l->lv_copyID == copyID)
	// avoid recursion
	return &t_list_any;

    l->lv_copyID = copyID;

    // Use the common type of all members.
    member_type = typval2type(&l->lv_first->li_tv, copyID, type_gap,
							TVTT_DO_MEMBER);
    for (li = l->lv_first->li_next; li != NULL; li = li->li_next)
	common_type(typval2type(&li->li_tv, copyID, type_gap, TVTT_DO_MEMBER),
					member_type, &member_type, type_gap);

    return get_list_type(member_type, type_gap);
}

/*
 * Get a type_T for a Tuple typval in "tv".
 * When "flags" has TVTT_DO_MEMBER also get the member type, otherwise use
 * "any".
 * When "flags" has TVTT_MORE_SPECIFIC get the more specific member type if it
 * is "any".
 */
    static type_T *
tuple_typval2type(typval_T *tv, int copyID, garray_T *type_gap, int flags)
{
    tuple_T	*tuple = tv->vval.v_tuple;
    int		len = tuple_len(tuple);
    type_T	*type = NULL;

    // An empty tuple has type tuple<unknown>, unless the type was specified
    // and is not tuple<any>.  This matters when assigning to a variable
    // with a specific tuple type.
    if (tuple == NULL || (len == 0 && (tuple->tv_type == NULL
				|| tuple->tv_type->tt_argcount == 0)))
	return &t_tuple_empty;

    if ((flags & TVTT_DO_MEMBER) == 0)
	return &t_tuple_any;

    // If the type is tuple<any> go through the members, it may end up a
    // more specific type.
    if (tuple->tv_type != NULL && (len == 0
					|| (flags & TVTT_MORE_SPECIFIC) == 0))
	// make a copy, tv_type may be freed if the tuple is freed
	return copy_type_deep(tuple->tv_type, type_gap);

    if (tuple->tv_copyID == copyID)
	// avoid recursion
	return &t_tuple_any;

    tuple->tv_copyID = copyID;

    garray_T	tuple_types_ga;
    ga_init2(&tuple_types_ga, sizeof(type_T *), 10);
    for (int i = 0; i < len; i++)
    {
	type = typval2type(TUPLE_ITEM(tuple, i), copyID, type_gap,
							TVTT_DO_MEMBER);
	if (ga_grow(&tuple_types_ga, 1) == FAIL)
	{
	    ga_clear(&tuple_types_ga);
	    return NULL;
	}
	((type_T **)tuple_types_ga.ga_data)[tuple_types_ga.ga_len] = type;
	tuple_types_ga.ga_len++;
    }

    type_T *tuple_type = get_tuple_type(&tuple_types_ga, type_gap);
    ga_clear(&tuple_types_ga);

    return tuple_type;
}

/*
 * Get a type_T for a Dict typval in "tv".
 * When "flags" has TVTT_DO_MEMBER also get the member type, otherwise use
 * "any".
 * When "flags" has TVTT_MORE_SPECIFIC get the more specific member type if it
 * is "any".
 */
    static type_T *
dict_typval2type(typval_T *tv, int copyID, garray_T *type_gap, int flags)
{
    dict_iterator_T	iter;
    typval_T		*value;
    dict_T		*d = tv->vval.v_dict;
    type_T		*member_type = NULL;

    if (d == NULL || (d->dv_hashtab.ht_used == 0 && d->dv_type == NULL))
	return &t_dict_empty;

    if ((flags & TVTT_DO_MEMBER) == 0)
	return &t_dict_any;

    // If the type is dict<any> go through the members, it may end up a
    // more specific type.
    if (d->dv_type != NULL && (d->dv_hashtab.ht_used == 0
		|| (flags & TVTT_MORE_SPECIFIC) == 0
		|| d->dv_type->tt_member != &t_any))
	return d->dv_type;

    if (d->dv_copyID == copyID)
	// avoid recursion
	return &t_dict_any;

    d->dv_copyID = copyID;

    // Use the common type of all values.
    dict_iterate_start(tv, &iter);
    dict_iterate_next(&iter, &value);
    member_type = typval2type(value, copyID, type_gap, TVTT_DO_MEMBER);

    while (dict_iterate_next(&iter, &value) != NULL)
	common_type(typval2type(value, copyID, type_gap, TVTT_DO_MEMBER),
					member_type, &member_type, type_gap);

    return get_dict_type(member_type, type_gap);
}

/*
 * Get a type_T for a "partial" typval in "tv".
 */
    static type_T *
partial_typval2type(typval_T *tv, ufunc_T *ufunc, garray_T *type_gap)
{
    partial_T *pt = tv->vval.v_partial;
    type_T  *type;

    type = get_type_ptr(type_gap);
    if (type == NULL)
	return NULL;

    *type = *ufunc->uf_func_type;
    if (type->tt_argcount >= 0 && pt->pt_argc > 0)
    {
	type->tt_argcount -= pt->pt_argc;
	type->tt_min_argcount -= pt->pt_argc;
	if (type->tt_argcount > 0 && func_type_add_arg_types(type,
		    type->tt_argcount, type_gap) == OK)
	    for (int i = 0; i < type->tt_argcount; ++i)
		type->tt_args[i] =
		    ufunc->uf_func_type->tt_args[i + pt->pt_argc];
    }
    if (pt->pt_func != NULL)
	type->tt_member = pt->pt_func->uf_ret_type;

    return type;
}

/*
 * Get a type_T for a "class" or an "object" typval in "tv".
 */
    static type_T *
oc_typval2type(typval_T *tv)
{
    if (tv->v_type == VAR_CLASS)
    {
	if (tv->vval.v_class == NULL)
	    return &t_class;

	return &tv->vval.v_class->class_type;
    }

    if (tv->vval.v_object != NULL)
	return &tv->vval.v_object->obj_class->class_object_type;

    return &t_object_any;
}

/*
 * Get a type_T for a "function" or a "partial"
 */
    static type_T *
fp_typval2type(typval_T *tv, garray_T *type_gap)
{
    char_u	*name = NULL;
    ufunc_T	*ufunc = NULL;
    type_T	*type;
    type_T	*member_type = NULL;
    int		argcount = 0;
    int		min_argcount = 0;

    if (tv->v_type == VAR_PARTIAL && tv->vval.v_partial != NULL)
    {
	if (tv->vval.v_partial->pt_func != NULL)
	    ufunc = tv->vval.v_partial->pt_func;
	else
	    name = tv->vval.v_partial->pt_name;
    }
    else
	name = tv->vval.v_string;

    if (name == NULL && ufunc == NULL)
	return &t_func_unknown;

    if (name != NULL)
    {
	int idx = find_internal_func(name);

	if (idx >= 0)
	{
	    type_T *decl_type;  // unused

	    internal_func_get_argcount(idx, &argcount, &min_argcount);
	    member_type = internal_func_ret_type(idx, 0, NULL, &decl_type,
		    type_gap);
	}
	else
	    ufunc = find_func(name, FALSE);
    }
    if (ufunc != NULL)
    {
	// May need to get the argument types from default values by
	// compiling the function.
	if (ufunc->uf_def_status == UF_TO_BE_COMPILED
		&& compile_def_function(ufunc, TRUE, CT_NONE, NULL)
		== FAIL)
	    return NULL;
	if (ufunc->uf_func_type == NULL)
	    set_function_type(ufunc);
	if (ufunc->uf_func_type != NULL)
	{
	    if (tv->v_type == VAR_PARTIAL && tv->vval.v_partial != NULL)
		return partial_typval2type(tv, ufunc, type_gap);
	    return ufunc->uf_func_type;
	}
    }

    type = get_type_ptr(type_gap);
    if (type == NULL)
	return NULL;
    type->tt_type = tv->v_type;
    type->tt_argcount = argcount;
    type->tt_min_argcount = min_argcount;
    if (tv->v_type == VAR_PARTIAL && tv->vval.v_partial != NULL
					&& tv->vval.v_partial->pt_argc > 0)
    {
	type->tt_argcount -= tv->vval.v_partial->pt_argc;
	type->tt_min_argcount -= tv->vval.v_partial->pt_argc;
    }
    type->tt_member = member_type;

    return type;
}

/*
 * Get a type_T for a typval_T.
 * "type_gap" is used to temporarily create types in.
 * When "flags" has TVTT_DO_MEMBER also get the member type, otherwise use
 * "any".
 * When "flags" has TVTT_MORE_SPECIFIC get the more specific member type if it
 * is "any".
 */
    static type_T *
typval2type_int(typval_T *tv, int copyID, garray_T *type_gap, int flags)
{
    switch (tv->v_type)
    {
	case VAR_UNKNOWN:
	    return &t_unknown;

	case VAR_ANY:
	    return &t_any;

	case VAR_VOID:
	    return &t_void;

	case VAR_BOOL:
	    return &t_bool;

	case VAR_SPECIAL:
	    return special_typval2type(tv);

	case VAR_NUMBER:
	    return &t_number;

	case VAR_FLOAT:
	    return &t_float;

	case VAR_STRING:
	    return &t_string;

	case VAR_BLOB:
	    if (tv->vval.v_blob == NULL)
		return &t_blob_null;
	    return &t_blob;

	case VAR_LIST:
	    return list_typval2type(tv, copyID, type_gap, flags);

	case VAR_TUPLE:
	    return tuple_typval2type(tv, copyID, type_gap, flags);

	case VAR_DICT:
	    return dict_typval2type(tv, copyID, type_gap, flags);

	case VAR_JOB:
	    return &t_job;

	case VAR_CHANNEL:
	    return &t_channel;

	case VAR_CLASS:
	case VAR_OBJECT:
	    return oc_typval2type(tv);

	case VAR_TYPEALIAS:
	    return &t_typealias;

	case VAR_FUNC:
	case VAR_PARTIAL:
	    return fp_typval2type(tv, type_gap);

	case VAR_INSTR:
	default:
	    break;
    }

    return NULL;
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
 * When "flags" has TVTT_DO_MEMBER also get the member type, otherwise use
 * "any".
 * When "flags" has TVTT_MORE_SPECIFIC get the most specific member type.
 */
    type_T *
typval2type(typval_T *tv, int copyID, garray_T *type_gap, int flags)
{
    type_T *type = typval2type_int(tv, copyID, type_gap, flags);

    if (type == NULL)
	return NULL;

    if (type != &t_bool && (tv->v_type == VAR_NUMBER
		&& (tv->vval.v_number == 0 || tv->vval.v_number == 1)))
	// Number 0 and 1 and expression with "&&" or "||" can also be used
	// for bool.
	type = &t_number_bool;
    else if (type != &t_float && tv->v_type == VAR_NUMBER)
	// A number can also be used for float.
	type = &t_number_float;
    return type;
}

/*
 * Return TRUE if "type" can be used for a variable declaration.
 * Give an error and return FALSE if not.
 */
    int
valid_declaration_type(type_T *type)
{
    if (type->tt_type == VAR_SPECIAL  // null, none
	    || type->tt_type == VAR_VOID)
    {
	char *tofree = NULL;
	char *name = type_name(type, &tofree);
	semsg(_(e_invalid_type_for_object_variable_str), name);
	vim_free(tofree);
	return FALSE;
    }
    return TRUE;
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
    if (tv->v_type == VAR_DICT)  // e.g. for v:event
	return &t_dict_any;
    return typval2type(tv, get_copyID(), type_gap, TVTT_DO_MEMBER);
}

    int
check_typval_arg_type(
	type_T	    *expected,
	typval_T    *actual_tv,
	char	    *func_name,
	int	    arg_idx)
{
    where_T	where = WHERE_INIT;

    if (arg_idx > 0)
    {
	where.wt_index = arg_idx;
	where.wt_kind = WT_ARGUMENT;
    }
    where.wt_func_name = func_name;
    return check_typval_type(expected, actual_tv, where);
}

/*
 * Return FAIL if "expected" and "actual" don't match.
 * When "argidx" > 0 it is included in the error message.
 */
    int
check_typval_type(type_T *expected, typval_T *actual_tv, where_T where)
{
    garray_T	type_list;
    type_T	*actual_type;
    int		res = FAIL;

    if (expected == NULL)
	return OK;  // didn't expect anything.

    ga_init2(&type_list, sizeof(type_T *), 10);

    // A null_function and null_partial are special cases, they can be used to
    // clear a variable.
    if ((actual_tv->v_type == VAR_FUNC && actual_tv->vval.v_string == NULL)
	    || (actual_tv->v_type == VAR_PARTIAL
					 && actual_tv->vval.v_partial == NULL))
	actual_type = &t_func_unknown;
    else
	// When the actual type is list<any> or dict<any> go through the values
	// to possibly get a more specific type.
	actual_type = typval2type(actual_tv, get_copyID(), &type_list,
					  TVTT_DO_MEMBER | TVTT_MORE_SPECIFIC);
    if (actual_type != NULL)
    {
	res = check_type_maybe(expected, actual_type, TRUE, where);
	if (res == MAYBE && !(actual_type->tt_type == VAR_FUNC
				      && (actual_type->tt_member == &t_unknown
					  || actual_type->tt_member == NULL)))
	{
	    // If a type check is needed that means assigning "any" or
	    // "unknown" to a more specific type, which fails here.
	    // Except when it looks like a lambda, since they have an
	    // incomplete type.  A legacy lambda function has a NULL return
	    // type.
	    type_mismatch_where(expected, actual_type, where);
	    res = FAIL;
	}
    }
    clear_type_list(&type_list);
    return res;
}

    void
arg_type_mismatch(type_T *expected, type_T *actual, int arg_idx)
{
    where_T	where = WHERE_INIT;

    if (arg_idx > 0)
    {
	where.wt_index = arg_idx;
	where.wt_kind = WT_ARGUMENT;
    }
    type_mismatch_where(expected, actual, where);
}

    void
type_mismatch_where(type_T *expected, type_T *actual, where_T where)
{
    char *tofree1, *tofree2;
    char *typename1 = type_name(expected, &tofree1);
    char *typename2 = type_name(actual, &tofree2);

    switch (where.wt_kind)
    {
	case WT_MEMBER:
	    semsg(_(e_variable_str_type_mismatch_expected_str_but_got_str),
		    where.wt_func_name, typename1, typename2);
	    break;
	case WT_METHOD:
	case WT_METHOD_ARG:
	case WT_METHOD_RETURN:
	    semsg(_(e_method_str_type_mismatch_expected_str_but_got_str),
		    where.wt_func_name, typename1, typename2);
	    break;
	case WT_VARIABLE:
	    if (where.wt_func_name == NULL)
		semsg(_(e_variable_nr_type_mismatch_expected_str_but_got_str),
			where.wt_index, typename1, typename2);
	    else
		semsg(_(e_variable_nr_type_mismatch_expected_str_but_got_str_in_str),
			where.wt_index, typename1, typename2, where.wt_func_name);
	    break;
	case WT_ARGUMENT:
	    if (where.wt_func_name == NULL)
		semsg(_(e_argument_nr_type_mismatch_expected_str_but_got_str),
			where.wt_index, typename1, typename2);
	    else
		semsg(_(e_argument_nr_type_mismatch_expected_str_but_got_str_in_str),
			where.wt_index, typename1, typename2, where.wt_func_name);
	    break;
	case WT_CAST:
	case WT_UNKNOWN:
	    if (where.wt_func_name == NULL)
		semsg(_(e_type_mismatch_expected_str_but_got_str),
			typename1, typename2);
	    else
		semsg(_(e_type_mismatch_expected_str_but_got_str_in_str),
			typename1, typename2, where.wt_func_name);
	    break;
    }

    vim_free(tofree1);
    vim_free(tofree2);
}

/*
 * Check if the expected and actual types match for a tuple
 */
    static int
check_tuple_type_maybe(
    type_T	*expected,
    type_T	*actual,
    where_T	where)
{
    if (expected->tt_argcount == -1 || actual->tt_argcount == -1
		|| expected->tt_args == NULL || actual->tt_args == NULL)
	return OK;

    // For a non-variadic tuple, the number of items must match
    if (!(expected->tt_flags & TTFLAG_VARARGS)
	    && expected->tt_argcount != actual->tt_argcount)
	return FAIL;

    // compare the type of each tuple item
    for (int i = 0; i < actual->tt_argcount; ++i)
    {
	type_T	*exp_type;
	type_T	*actual_type;

	if (expected->tt_flags & TTFLAG_VARARGS)
	{
	    if (i < expected->tt_argcount - 1)
		exp_type = expected->tt_args[i];
	    else
		// For a variadic tuple, the last type is a List.  So use the
		// List member type.
		exp_type = expected->tt_args[expected->tt_argcount - 1]->tt_member;
	}
	else
	    exp_type = expected->tt_args[i];

	if (actual->tt_flags & TTFLAG_VARARGS)
	{
	    if (i < actual->tt_argcount - 1)
		actual_type = actual->tt_args[i];
	    else
		// For a variadic tuple, the last type is a List.  So use the
		// List member type.
		actual_type = actual->tt_args[actual->tt_argcount - 1]->tt_member;
	}
	else
	    actual_type = actual->tt_args[i];

	// Allow for using "any" type for a tuple item
	if (actual->tt_args[i] != &t_any && check_type(exp_type, actual_type,
						FALSE, where) == FAIL)
	    return FAIL;
    }

    return OK;
}

/*
 * Check if the expected and actual types match.
 * Does not allow for assigning "any" to a specific type.
 * When "argidx" > 0 it is included in the error message.
 * Return OK if types match.
 * Return FAIL if types do not match.
 */
    int
check_type(
	type_T	*expected,
	type_T	*actual,
	int	give_msg,
	where_T where)
{
    int ret = check_type_maybe(expected, actual, give_msg, where);

    return ret == MAYBE ? OK : ret;
}

/*
 * As check_type() but return MAYBE when a runtime type check should be used
 * when compiling.
 */
    int
check_type_maybe(
	type_T	*expected,
	type_T	*actual,
	int	give_msg,
	where_T where)
{
    int ret = OK;

    // When expected is "unknown" we accept any actual type.
    // When expected is "any" we accept any actual type except "void".
    if (expected->tt_type != VAR_UNKNOWN
	    && !(expected->tt_type == VAR_ANY && actual->tt_type != VAR_VOID))

    {
	// tt_type should match, except that a "partial" can be assigned to a
	// variable with type "func".
	// And "unknown" (using global variable) and "any" need a runtime type
	// check.
	if (!(expected->tt_type == actual->tt_type
		    || actual->tt_type == VAR_UNKNOWN
		    || actual->tt_type == VAR_ANY
		    || (expected->tt_type == VAR_FUNC
					   && actual->tt_type == VAR_PARTIAL)))
	{
	    if (expected->tt_type == VAR_BOOL
					&& (actual->tt_flags & TTFLAG_BOOL_OK))
		// Using number 0 or 1 for bool is OK.
		return OK;
	    if (expected->tt_type == VAR_FLOAT
		    && actual->tt_type == VAR_NUMBER
		    && ((expected->tt_flags & TTFLAG_NUMBER_OK)
			     || (actual->tt_flags & TTFLAG_FLOAT_OK)))
		// Using a number where a float is expected is OK here.
		return OK;
	    if (give_msg)
		type_mismatch_where(expected, actual, where);
	    return FAIL;
	}
	if (expected->tt_type == VAR_DICT || expected->tt_type == VAR_LIST)
	{
	    // "unknown" is used for an empty list or dict
	    if (actual->tt_member != NULL && actual->tt_member != &t_unknown)
		ret = check_type_maybe(expected->tt_member, actual->tt_member,
								 FALSE, where);
	}
	else if (expected->tt_type == VAR_TUPLE && actual != &t_any)
	    ret =  check_tuple_type_maybe(expected, actual, where);
	else if (expected->tt_type == VAR_FUNC && actual != &t_any)
	{
	    // If the return type is unknown it can be anything, including
	    // nothing, thus there is no point in checking.
	    if (expected->tt_member != &t_unknown)
	    {
		if (actual->tt_member != NULL
					    && actual->tt_member != &t_unknown)
		{
		    where_T  func_where = where;

		    func_where.wt_kind = WT_METHOD_RETURN;
		    ret = check_type_maybe(expected->tt_member,
					    actual->tt_member, FALSE,
					    func_where);
		}
		else
		    ret = MAYBE;
	    }
	    if (ret != FAIL
		    && ((expected->tt_flags & TTFLAG_VARARGS)
			!= (actual->tt_flags & TTFLAG_VARARGS))
		    && expected->tt_argcount != -1)
		ret = FAIL;
	    if (ret != FAIL && expected->tt_argcount != -1
		    && actual->tt_min_argcount != -1
		    && (actual->tt_argcount == -1
			|| (actual->tt_argcount < expected->tt_min_argcount
			    || actual->tt_argcount > expected->tt_argcount)))
		ret = FAIL;
	    if (ret != FAIL && expected->tt_args != NULL
						    && actual->tt_args != NULL)
	    {
		int i;

		for (i = 0; i < expected->tt_argcount
					       && i < actual->tt_argcount; ++i)
		{
		    where_T  func_where = where;
		    func_where.wt_kind = WT_METHOD_ARG;

		    // Allow for using "any" argument type, lambda's have them.
		    if (actual->tt_args[i] != &t_any && check_type(
			    expected->tt_args[i], actual->tt_args[i], FALSE,
							func_where) == FAIL)
		    {
			ret = FAIL;
			break;
		    }
		}
	    }
	    if (ret == OK && expected->tt_argcount >= 0
						  && actual->tt_argcount == -1)
		// check the argument count at runtime
		ret = MAYBE;
	}
	else if (expected->tt_type == VAR_OBJECT)
	{
	    if (actual->tt_type == VAR_ANY)
		return MAYBE;	// use runtime type check
	    if (actual->tt_type != VAR_OBJECT)
		return FAIL;	// don't use tt_class
	    if (actual->tt_class == NULL)    // null object
		return OK;
	    // t_object_any matches any object except for an enum item
	    if (expected == &t_object_any && !IS_ENUM(actual->tt_class))
		return OK;

	    // For object method arguments, do a invariant type check in
	    // an extended class.  For all others, do a covariance type check.
	    if (where.wt_kind == WT_METHOD_ARG)
	    {
		if (actual->tt_class != expected->tt_class)
		    ret = FAIL;
	    }
	    else if (!class_instance_of(actual->tt_class, expected->tt_class))
	    {
		// Check if this is an up-cast, if so we'll have to check the type at
		// runtime.
		if (where.wt_kind == WT_CAST &&
			class_instance_of(expected->tt_class, actual->tt_class))
		    ret = MAYBE;
		else
		    ret = FAIL;
	    }
	}

	if (ret == FAIL && give_msg)
	    type_mismatch_where(expected, actual, where);
    }

    if (ret == OK && expected->tt_type != VAR_UNKNOWN
	    && expected->tt_type != VAR_ANY
	    && (actual->tt_type == VAR_UNKNOWN || actual->tt_type == VAR_ANY))
	// check the type at runtime
	ret = MAYBE;

    return ret;
}

/*
 * Check that the arguments of "type" match "argvars[argcount]".
 * "base_tv" is from "expr->Func()".
 * Return OK/FAIL.
 */
    int
check_argument_types(
	type_T	    *type,
	typval_T    *argvars,
	int	    argcount,
	typval_T    *base_tv,
	char_u	    *name)
{
    int	    varargs = (type->tt_flags & TTFLAG_VARARGS) ? 1 : 0;
    int	    i;
    int	    totcount = argcount + (base_tv == NULL ? 0 : 1);

    if (type->tt_type != VAR_FUNC && type->tt_type != VAR_PARTIAL)
	return OK;  // just in case
    if (totcount < type->tt_min_argcount - varargs)
    {
	emsg_funcname(e_not_enough_arguments_for_function_str, name);
	return FAIL;
    }
    if (!varargs && type->tt_argcount >= 0 && totcount > type->tt_argcount)
    {
	emsg_funcname(e_too_many_arguments_for_function_str, name);
	return FAIL;
    }
    if (type->tt_args == NULL)
	return OK;  // cannot check


    for (i = 0; i < totcount; ++i)
    {
	type_T	    *expected;
	typval_T    *tv;

	if (base_tv != NULL)
	{
	    if (i == 0)
		tv = base_tv;
	    else
		tv = &argvars[i - 1];
	}
	else
	    tv = &argvars[i];
	if (varargs && i >= type->tt_argcount - 1)
	{
	    expected = type->tt_args[type->tt_argcount - 1];
	    if (expected != NULL && expected->tt_type == VAR_LIST)
		expected = expected->tt_member;
	    if (expected == NULL)
		expected = &t_any;
	}
	else
	    expected = type->tt_args[i];

	// check the type, unless the value is v:none
	if ((tv->v_type != VAR_SPECIAL || tv->vval.v_number != VVAL_NONE)
		   && check_typval_arg_type(expected, tv, NULL, i + 1) == FAIL)
	    return FAIL;
    }
    return OK;
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

    // Also skip over "." for imported classes: "import.ClassName".
    while (ASCII_ISALNUM(*p) || *p == '_' || *p == '.')
	++p;

    // Skip over "<type>"; this is permissive about white space.
    if (*skipwhite(p) == '<')
    {
	if (STRNCMP("tuple", start, 5) == 0)
	{
	    // handle tuple<{type1}, {type2}, ....<type>>
	    p = skipwhite(p + 1);
	    while (*p != '>' && *p != NUL)
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
	    if (*p == '>')
		p++;
	}
	else
	{
	    p = skipwhite(p);
	    p = skip_type(skipwhite(p + 1), FALSE);
	    p = skipwhite(p);
	    if (*p == '>')
		++p;
	}
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
 * "info" is extra information for an error message.
 * Returns NULL in case of failure.
 */
    static type_T *
parse_type_member(
	char_u	    **arg,
	type_T	    *type,
	garray_T    *type_gap,
	int	    give_error,
	char	    *info)
{
    char_u  *arg_start = *arg;
    type_T  *member_type;
    int	    prev_called_emsg = called_emsg;

    if (**arg != '<')
    {
	if (give_error)
	{
	    if (*skipwhite(*arg) == '<')
		semsg(_(e_no_white_space_allowed_before_str_str), "<", *arg);
	    else
		semsg(_(e_missing_type_after_str), info);
	}
	return NULL;
    }
    *arg = skipwhite(*arg + 1);

    member_type = parse_type(arg, type_gap, give_error);
    if (member_type == NULL)
	return NULL;

    *arg = skipwhite(*arg);
    if (**arg != '>' && called_emsg == prev_called_emsg)
    {
	if (give_error)
	    semsg(_(e_missing_gt_after_type_str), arg_start);
	return NULL;
    }
    ++*arg;

    if (type->tt_type == VAR_LIST)
	return get_list_type(member_type, type_gap);
    return get_dict_type(member_type, type_gap);
}

/*
 * Parse a "func" type at "*arg" and advance over it.
 * When "give_error" is TRUE give error messages, otherwise be quiet.
 * Return NULL for failure.
 */
    static type_T *
parse_type_func(char_u **arg, size_t len, garray_T *type_gap, int give_error)
{
    char_u  *p;
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
		if (give_error)
		    emsg(_(e_mandatory_argument_after_optional_argument));
		return NULL;
	    }

	    type = parse_type(&p, type_gap, give_error);
	    if (type == NULL)
		return NULL;
	    if ((flags & TTFLAG_VARARGS) != 0 && type->tt_type != VAR_LIST)
	    {
		char *tofree;
		semsg(_(e_variable_arguments_type_must_be_list_str),
			type_name(type, &tofree));
		vim_free(tofree);
		return NULL;
	    }
	    arg_type[argcount++] = type;

	    // Nothing comes after "...{type}".
	    if (flags & TTFLAG_VARARGS)
		break;

	    if (*p != ',' && *skipwhite(p) == ',')
	    {
		if (give_error)
		    semsg(_(e_no_white_space_allowed_before_str_str),
			    ",", p);
		return NULL;
	    }
	    if (*p == ',')
	    {
		++p;
		if (!VIM_ISWHITE(*p))
		{
		    if (give_error)
			semsg(_(e_white_space_required_after_str_str),
				",", p - 1);
		    return NULL;
		}
	    }
	    p = skipwhite(p);
	    if (argcount == MAX_FUNC_ARGS)
	    {
		if (give_error)
		    emsg(_(e_too_many_argument_types));
		return NULL;
	    }
	}

	p = skipwhite(p);
	if (*p != ')')
	{
	    if (give_error)
		emsg(_(e_missing_closing_paren));
	    return NULL;
	}
	*arg = p + 1;
    }
    if (**arg == ':')
    {
	// parse return type
	++*arg;
	if (!VIM_ISWHITE(**arg) && give_error)
	    semsg(_(e_white_space_required_after_str_str), ":", *arg - 1);
	*arg = skipwhite(*arg);
	ret_type = parse_type(arg, type_gap, give_error);
	if (ret_type == NULL)
	    return NULL;
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
	    if (func_type_add_arg_types(type, argcount, type_gap) == FAIL)
		return NULL;
	    mch_memmove(type->tt_args, arg_type, sizeof(type_T *) * argcount);
	}
    }

    return type;
}

/*
 * Parse a "tuple" type at "*arg" and advance over it.
 * When "give_error" is TRUE give error messages, otherwise be quiet.
 * Return NULL for failure.
 */
    static type_T *
parse_type_tuple(char_u **arg, garray_T *type_gap, int give_error)
{
    char_u	*p;
    type_T	*type;
    type_T	*ret_type = NULL;
    int		typecount = -1;
    int		flags = 0;
    garray_T	tuple_types_ga;

    ga_init2(&tuple_types_ga, sizeof(type_T *), 10);

    // tuple<{type}, {type}>
    // tuple<{type}, ...{type}>
    if (**arg != '<')
    {
	if (give_error)
	{
	    if (*skipwhite(*arg) == '<')
		semsg(_(e_no_white_space_allowed_before_str_str), "<", *arg);
	    else
		semsg(_(e_missing_type_after_str), "tuple");
	}

	// only "tuple" is specified
	return NULL;
    }

    p = ++*arg;
    typecount = 0;
    while (*p != NUL && *p != '>')
    {
	if (STRNCMP(p, "...", 3) == 0)
	{
	    flags |= TTFLAG_VARARGS;
	    p += 3;
	}

	type = parse_type(&p, type_gap, give_error);
	if (type == NULL)
	    goto on_err;

	if ((flags & TTFLAG_VARARGS) != 0 && type->tt_type != VAR_LIST)
	{
	    char *tofree;
	    semsg(_(e_variadic_tuple_must_end_with_list_type_str),
						type_name(type, &tofree));
	    vim_free(tofree);
	    goto on_err;
	}

	// Add the item type
	if (ga_grow(&tuple_types_ga, 1) == FAIL)
	    goto on_err;
	((type_T **)tuple_types_ga.ga_data)[tuple_types_ga.ga_len] = type;
	tuple_types_ga.ga_len++;
	typecount++;

	// Nothing comes after "...{type}".
	if (flags & TTFLAG_VARARGS)
	    break;

	if (*p != ',' && *skipwhite(p) == ',')
	{
	    if (give_error)
		semsg(_(e_no_white_space_allowed_before_str_str), ",", p);
	    goto on_err;
	}
	if (*p == ',')
	{
	    ++p;
	    if (!VIM_ISWHITE(*p))
	    {
		if (give_error)
		    semsg(_(e_white_space_required_after_str_str),
			    ",", p - 1);
		goto on_err;
	    }
	}
	p = skipwhite(p);
    }

    p = skipwhite(p);
    if (*p != '>' || typecount <= 0)
    {
	if (give_error)
	    semsg(_(e_missing_type_after_str), p);
	goto on_err;
    }
    *arg = p + 1;

    ret_type = alloc_tuple_type(typecount, type_gap);
    ret_type->tt_flags = flags;
    ret_type->tt_argcount = typecount;
    if (tuple_type_add_types(ret_type, typecount, type_gap) == FAIL)
	return NULL;
    mch_memmove(ret_type->tt_args, tuple_types_ga.ga_data,
						sizeof(type_T *) * typecount);

on_err:
    ga_clear(&tuple_types_ga);

    return ret_type;
}

/*
 * Parse a "object" type at "*arg" and advance over it.
 * When "give_error" is TRUE give error messages, otherwise be quiet.
 * Return NULL for failure.
 */
    static type_T *
parse_type_object(char_u **arg, garray_T *type_gap, int give_error)
{
    char_u	*arg_start = *arg;
    type_T	*object_type;
    int		prev_called_emsg = called_emsg;

    // object<X> or object<any>
    if (**arg != '<')
    {
	if (give_error)
	{
	    if (*skipwhite(*arg) == '<')
		semsg(_(e_no_white_space_allowed_before_str_str), "<", *arg);
	    else
		semsg(_(e_missing_type_after_str), "object");
	}

	// only "object" is specified
	return NULL;
    }

    // skip spaces following "object<"
    *arg = skipwhite(*arg + 1);

    object_type = parse_type(arg, type_gap, give_error);
    if (object_type == NULL)
	return NULL;

    *arg = skipwhite(*arg);
    if (**arg != '>' && called_emsg == prev_called_emsg)
    {
	if (give_error)
	    semsg(_(e_missing_gt_after_type_str), arg_start);
	return NULL;
    }
    ++*arg;

    if (object_type->tt_type == VAR_ANY)
	return &t_object_any;

    if (object_type->tt_type != VAR_OBJECT)
    {
	// specified type is not a class
	if (give_error)
	    semsg(_(e_class_name_not_found_str), arg_start);
	return NULL;
    }

    return object_type;
}

/*
 * Parse a user defined type at "*arg" and advance over it.
 * It can be a class or an interface or a typealias name, possibly imported.
 * Return NULL if a type is not found.
 */
    static type_T *
parse_type_user_defined(
    char_u	**arg,
    size_t	len,
    garray_T	*type_gap,
    int		give_error)
{
    int		did_emsg_before = did_emsg;
    typval_T	tv;

    tv.v_type = VAR_UNKNOWN;
    if (eval_variable_import(*arg, &tv) == OK)
    {
	if (tv.v_type == VAR_CLASS && tv.vval.v_class != NULL)
	{
	    type_T *type = get_type_ptr(type_gap);
	    if (type != NULL)
	    {
		// Although the name is that of a class or interface, the type
		// uses will be an object.
		type->tt_type = VAR_OBJECT;
		type->tt_class = tv.vval.v_class;
		clear_tv(&tv);

		*arg += len;
		// Skip over ".ClassName".
		while (ASCII_ISALNUM(**arg) || **arg == '_' || **arg == '.')
		    ++*arg;

		return type;
	    }
	}
	else if (tv.v_type == VAR_TYPEALIAS)
	{
	    // user defined type
	    type_T *type = copy_type(tv.vval.v_typealias->ta_type, type_gap);
	    *arg += len;
	    clear_tv(&tv);
	    // Skip over ".TypeName".
	    while (ASCII_ISALNUM(**arg) || **arg == '_' || **arg == '.')
		++*arg;
	    return type;
	}

	clear_tv(&tv);
    }

    if (give_error && (did_emsg == did_emsg_before))
    {
	char_u	*p = skip_type(*arg, FALSE);
	char	cc = *p;

	*p = NUL;
	semsg(_(e_type_not_recognized_str), *arg);
	*p = cc;
    }

    return NULL;
}

/*
 * Parse a type at "arg" and advance over it.
 * When "give_error" is TRUE give error messages, otherwise be quiet.
 * Return NULL for failure.
 */
    type_T *
parse_type(char_u **arg, garray_T *type_gap, int give_error)
{
    char_u  *p = *arg;
    size_t  len;

    // Skip over the first word.
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
		return parse_type_member(arg, &t_dict_any,
						 type_gap, give_error, "dict");
	    }
	    break;
	case 'f':
	    if (len == 5 && STRNCMP(*arg, "float", len) == 0)
	    {
		*arg += len;
		return &t_float;
	    }
	    if (len == 4 && STRNCMP(*arg, "func", len) == 0)
		return parse_type_func(arg, len, type_gap, give_error);
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
		return parse_type_member(arg, &t_list_any,
						 type_gap, give_error, "list");
	    }
	    break;
	case 'n':
	    if (len == 6 && STRNCMP(*arg, "number", len) == 0)
	    {
		*arg += len;
		return &t_number;
	    }
	    break;
	case 'o':
	    if (len == 6 && STRNCMP(*arg, "object", len) == 0)
	    {
		*arg += len;
		return parse_type_object(arg, type_gap, give_error);
	    }
	    break;
	case 's':
	    if (len == 6 && STRNCMP(*arg, "string", len) == 0)
	    {
		*arg += len;
		return &t_string;
	    }
	    break;
	case 't':
	    if (len == 5 && STRNCMP(*arg, "tuple", len) == 0)
	    {
		*arg += len;
		return parse_type_tuple(arg, type_gap, give_error);
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

    // User defined type
    return parse_type_user_defined(arg, len, type_gap, give_error);
}

/*
 * Check if "type1" and "type2" are exactly the same.
 * "flags" can have ETYPE_ARG_UNKNOWN, which means that an unknown argument
 * type in "type1" is accepted.
 */
    int
equal_type(type_T *type1, type_T *type2, int flags)
{
    int i;

    if (type1 == NULL || type2 == NULL)
	return FALSE;
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
	case VAR_INSTR:
	case VAR_TYPEALIAS:
	    break;  // not composite is always OK
	case VAR_OBJECT:
	case VAR_CLASS:
	    // Objects are considered equal if they are from the same class
	    return type1->tt_class == type2->tt_class;
	case VAR_LIST:
	case VAR_DICT:
	    return equal_type(type1->tt_member, type2->tt_member, flags);
	case VAR_TUPLE:
	    if (type1->tt_argcount != type2->tt_argcount)
		return FALSE;
	    if (type1->tt_argcount < 0
			   || type1->tt_args == NULL || type2->tt_args == NULL)
		return TRUE;
	    for (i = 0; i < type1->tt_argcount; ++i)
		if ((flags & ETYPE_ARG_UNKNOWN) == 0
			&& !equal_type(type1->tt_args[i], type2->tt_args[i],
									flags))
		    return FALSE;
	    return TRUE;
	case VAR_FUNC:
	case VAR_PARTIAL:
	    if (!equal_type(type1->tt_member, type2->tt_member, flags)
		    || type1->tt_argcount != type2->tt_argcount)
		return FALSE;
	    if (type1->tt_argcount < 0
			   || type1->tt_args == NULL || type2->tt_args == NULL)
		return TRUE;
	    for (i = 0; i < type1->tt_argcount; ++i)
		if ((flags & ETYPE_ARG_UNKNOWN) == 0
			&& !equal_type(type1->tt_args[i], type2->tt_args[i],
									flags))
		    return FALSE;
	    return TRUE;
    }
    return TRUE;
}

/*
 * Find the common type of "type1" (VAR_FUNC) and "type2" (VAR_FUNC) and put it
 * in "dest".  "type2" and "dest" may be the same.
 */
    static void
common_type_var_func(
    type_T	*type1,
    type_T	*type2,
    type_T	**dest,
    garray_T	*type_gap)
{
    type_T *common;

    // When one of the types is t_func_unknown return the other one.
    // Useful if a list or dict item is null_func.
    if (type1 == &t_func_unknown)
    {
	*dest = type2;
	return;
    }
    if (type2 == &t_func_unknown)
    {
	*dest = type1;
	return;
    }

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
	// Use -1 for "tt_argcount" to indicate an unknown number of
	// arguments.
	*dest = alloc_func_type(common, -1, type_gap);

    // Use the minimum of min_argcount.
    (*dest)->tt_min_argcount =
	type1->tt_min_argcount < type2->tt_min_argcount
	? type1->tt_min_argcount : type2->tt_min_argcount;
}

/*
 * Find the common type of "type1" and "type2" and put it in "dest".
 * "type2" and "dest" may be the same.
 */
    void
common_type(type_T *type1, type_T *type2, type_T **dest, garray_T *type_gap)
{
    if (equal_type(type1, type2, 0))
    {
	*dest = type1;
	return;
    }

    // If either is VAR_UNKNOWN use the other type.  An empty list/dict has no
    // specific type.
    if (type1 == NULL || type1->tt_type == VAR_UNKNOWN)
    {
	*dest = type2;
	return;
    }
    if (type2 == NULL || type2->tt_type == VAR_UNKNOWN)
    {
	*dest = type1;
	return;
    }

    if (type1->tt_type == type2->tt_type)
    {
	if (type1->tt_type == VAR_LIST
		|| type1->tt_type == VAR_DICT)
	{
	    type_T *common;

	    common_type(type1->tt_member, type2->tt_member, &common, type_gap);
	    if (type1->tt_type == VAR_LIST)
		*dest = get_list_type(common, type_gap);
	    else
		*dest = get_dict_type(common, type_gap);
	    return;
	}
	else if (type1->tt_type == VAR_FUNC)
	{
	    common_type_var_func(type1, type2, dest, type_gap);
	    return;
	}
	else if (type1->tt_type == VAR_OBJECT)
	{
	    *dest = &t_object_any;
	    return;
	}
    }

    *dest = &t_any;
}

/*
 * Return the item type of a List, Dict or a Tuple
 */
    type_T *
get_item_type(type_T *type)
{
    if (type->tt_type == VAR_TUPLE)
    {
	if (type->tt_argcount != 1)
	    return &t_any;

	if (type->tt_flags & TTFLAG_VARARGS)
	    return type->tt_args[0]->tt_member;
	else
	    return type->tt_args[0];
    }

    return type->tt_member;
}

/*
 * Push an entry onto the type stack.  "type" used both for the current type
 * and the declared type.
 * Returns FAIL when out of memory.
 */
    int
push_type_stack(cctx_T *cctx, type_T *type)
{
    return push_type_stack2(cctx, type, type);
}

/*
 * Push an entry onto the type stack.  "type" is the current type, "decl_type"
 * is the declared type.
 * Returns FAIL when out of memory.
 */
    int
push_type_stack2(cctx_T *cctx, type_T *type, type_T *decl_type)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type2_T	*typep;

    if (GA_GROW_FAILS(stack, 1))
	return FAIL;
    typep = ((type2_T *)stack->ga_data) + stack->ga_len;
    typep->type_curr = type;
    typep->type_decl = decl_type;
    ++stack->ga_len;
    return OK;
}

/*
 * Set the type of the top of the stack to "type".
 */
    void
set_type_on_stack(cctx_T *cctx, type_T *type, int offset)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type2_T	*typep = ((type2_T *)stack->ga_data)
						  + stack->ga_len - 1 - offset;

    typep->type_curr = type;
    typep->type_decl = &t_any;
}

/*
 * Get the current type from the type stack.  If "offset" is zero the one at
 * the top,
 * if "offset" is one the type above that, etc.
 * Returns &t_unknown if there is no such stack entry.
 */
    type_T *
get_type_on_stack(cctx_T *cctx, int offset)
{
    garray_T	*stack = &cctx->ctx_type_stack;

    if (offset + 1 > stack->ga_len)
	return &t_unknown;
    return (((type2_T *)stack->ga_data) + stack->ga_len - offset - 1)
								   ->type_curr;
}

/*
 * Get the declared type from the type stack.  If "offset" is zero the one at
 * the top,
 * if "offset" is one the type above that, etc.
 * Returns &t_unknown if there is no such stack entry.
 */
    type_T *
get_decl_type_on_stack(cctx_T *cctx, int offset)
{
    garray_T	*stack = &cctx->ctx_type_stack;

    if (offset + 1 > stack->ga_len)
	return &t_unknown;
    return (((type2_T *)stack->ga_data) + stack->ga_len - offset - 1)
								   ->type_decl;
}

/*
 * Get the member type of a dict or list from the items on the stack of "cctx".
 * The declared type is stored in "decl_type".
 * For a list "skip" is 1, for a dict "skip" is 2, keys are skipped.
 * Returns &t_void for an empty list or dict.
 * Otherwise finds the common type of all items.
 */
    type_T *
get_member_type_from_stack(
	int	    count,
	int	    skip,
	cctx_T	    *cctx)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type2_T	*typep;
    garray_T    *type_gap = cctx->ctx_type_list;
    int		i;
    type_T	*result;
    type_T	*type;

    // Use "unknown" for an empty list or dict.
    if (count == 0)
	return &t_unknown;
    // Find the common type from following items.
    typep = ((type2_T *)stack->ga_data) + stack->ga_len;
    result = &t_unknown;
    for (i = 0; i < count; ++i)
    {
	type = (typep -((count - i) * skip) + skip - 1)->type_curr;
	if (check_type_is_value(type) == FAIL)
	    return NULL;
	if (result != &t_any)
	    common_type(type, result, &result, type_gap);
    }

    return result;
}

/*
 * Get the types of items in a tuple on the stack of "cctx".
 * Returns the number of types.  Returns -1 on failure.
 */
    int
get_tuple_type_from_stack(
    int		count,
    garray_T	*tuple_types_gap,
    cctx_T	*cctx)
{
    garray_T	*stack = &cctx->ctx_type_stack;
    type2_T	*typep;
    type_T	*type = NULL;

    // Use "unknown" for an empty tuple
    if (count == 0)
	return 0;

    // Find the common type from following items.
    typep = ((type2_T *)stack->ga_data) + stack->ga_len;
    for (int i = 0; i < count; i++)
    {
	type = (typep - (count - i))->type_curr;
	if (check_type_is_value(type) == FAIL)
	    return -1;
	if (ga_grow(tuple_types_gap, 1) == FAIL)
	    return -1;
	((type_T **)tuple_types_gap->ga_data)[tuple_types_gap->ga_len] = type;
	tuple_types_gap->ga_len++;
    }

    return tuple_types_gap->ga_len;
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
	case VAR_TUPLE: return "tuple";
	case VAR_DICT: return "dict";
	case VAR_INSTR: return "instr";
	case VAR_CLASS: return "class";
	case VAR_OBJECT: return "object";
	case VAR_TYPEALIAS: return "typealias";

	case VAR_FUNC:
	case VAR_PARTIAL: return "func";
    }
    return "unknown";
}

/*
 * Return the type name of a List (list<type>) or Dict (dict<type>).
 * The result may be in allocated memory, in which case "tofree" is set.
 */
    static char *
type_name_list_or_dict(char *name, type_T *type, char **tofree)
{
    char *member_free;
    char *member_name;

    if (type->tt_member->tt_type == VAR_UNKNOWN)
	member_name = type_name(&t_any, &member_free);
    else
	member_name = type_name(type->tt_member, &member_free);

    size_t len = STRLEN(name) + STRLEN(member_name) + 3;
    *tofree = alloc(len);
    if (*tofree == NULL)
	return name;

    vim_snprintf(*tofree, len, "%s<%s>", name, member_name);
    vim_free(member_free);
    return *tofree;
}

/*
 * Return the type name of a tuple.
 * The result may be in allocated memory, in which case "tofree" is set.
 */
    static char *
type_name_tuple(type_T *type, char **tofree)
{
    garray_T    ga;
    int		i;
    int		varargs = (type->tt_flags & TTFLAG_VARARGS) ? 1 : 0;
    char	*arg_free = NULL;

    ga_init2(&ga, 1, 100);
    if (ga_grow(&ga, 20) == FAIL)
	goto failed;
    STRCPY(ga.ga_data, "tuple<");
    ga.ga_len += 6;

    if (type->tt_argcount <= 0)
	// empty tuple
	ga_concat(&ga, (char_u *)"any");
    else
    {
	if (type->tt_args == NULL)
	    ga_concat(&ga, (char_u *)"[unknown]");
	else
	{
	    for (i = 0; i < type->tt_argcount; ++i)
	    {
		char	*arg_type;
		int	len;

		arg_type = type_name(type->tt_args[i], &arg_free);
		if (i > 0)
		{
		    STRCPY((char *)ga.ga_data + ga.ga_len, ", ");
		    ga.ga_len += 2;
		}
		len = (int)STRLEN(arg_type);
		if (ga_grow(&ga, len + 8) == FAIL)
		    goto failed;
		if (varargs && i == type->tt_argcount - 1)
		    ga_concat(&ga, (char_u *)"...");
		ga_concat(&ga, (char_u *)arg_type);
		VIM_CLEAR(arg_free);
	    }
	}
    }

    STRCPY((char *)ga.ga_data + ga.ga_len, ">");
    *tofree = ga.ga_data;
    return ga.ga_data;

failed:
    vim_free(arg_free);
    ga_clear(&ga);
    return "[unknown]";
}

/*
 * Return the type name of a Class (class<name>) or Object (object<name>).
 * The result may be in allocated memory, in which case "tofree" is set.
 */
    static char *
type_name_class_or_obj(char *name, type_T *type, char **tofree)
{
    char_u *class_name;

    if (type->tt_class != NULL)
    {
	class_name = type->tt_class->class_name;
	if (IS_ENUM(type->tt_class))
	    name = "enum";
    }
    else
	class_name = (char_u *)"any";

    size_t len = STRLEN(name) + STRLEN(class_name) + 3;
    *tofree = alloc(len);
    if (*tofree == NULL)
	return name;

    vim_snprintf(*tofree, len, "%s<%s>", name, class_name);
    return *tofree;
}

/*
 * Return the type name of a function.
 * The result may be in allocated memory, in which case "tofree" is set.
 */
    static char *
type_name_func(type_T *type, char **tofree)
{
    garray_T    ga;
    int		i;
    int		varargs = (type->tt_flags & TTFLAG_VARARGS) ? 1 : 0;
    char	*arg_free = NULL;

    ga_init2(&ga, 1, 100);
    if (ga_grow(&ga, 20) == FAIL)
	goto failed;
    STRCPY(ga.ga_data, "func(");
    ga.ga_len += 5;

    for (i = 0; i < type->tt_argcount; ++i)
    {
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
	    goto failed;
	if (varargs && i == type->tt_argcount - 1)
	    ga_concat(&ga, (char_u *)"...");
	else if (i >= type->tt_min_argcount)
	    *((char *)ga.ga_data + ga.ga_len++) = '?';
	ga_concat(&ga, (char_u *)arg_type);
	VIM_CLEAR(arg_free);
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
	    goto failed;
	STRCPY((char *)ga.ga_data + ga.ga_len, "): ");
	STRCPY((char *)ga.ga_data + ga.ga_len + 3, ret_name);
	vim_free(ret_free);
    }
    *tofree = ga.ga_data;
    return ga.ga_data;

failed:
    vim_free(arg_free);
    ga_clear(&ga);
    return "[unknown]";
}

/*
 * Return the name of a type.
 * The result may be in allocated memory, in which case "tofree" is set.
 */
    char *
type_name(type_T *type, char **tofree)
{
    char *name;

    *tofree = NULL;
    if (type == NULL)
	return "[unknown]";
    name = vartype_name(type->tt_type);

    switch (type->tt_type)
    {
	case VAR_LIST:
	case VAR_DICT:
	    return type_name_list_or_dict(name, type, tofree);

	case VAR_TUPLE:
	    return type_name_tuple(type, tofree);

	case VAR_CLASS:
	case VAR_OBJECT:
	    return type_name_class_or_obj(name, type, tofree);

	case VAR_FUNC:
	    return type_name_func(type, tofree);

	default:
	    break;
    }

    return name;
}

/*
 * "typename(expr)" function
 */
    void
f_typename(typval_T *argvars, typval_T *rettv)
{
    garray_T	type_list;
    type_T	*type;
    char	*tofree;
    char	*name;

    rettv->v_type = VAR_STRING;
    ga_init2(&type_list, sizeof(type_T *), 10);
    if (argvars[0].v_type == VAR_TYPEALIAS)
    {
	type = copy_type(argvars[0].vval.v_typealias->ta_type, &type_list);
	// A type alias for a class has the type set to VAR_OBJECT.  Change it
	// to VAR_CLASS, so that the name is "typealias<class<xxx>>"
	if (type->tt_type == VAR_OBJECT)
	    type->tt_type = VAR_CLASS;
    }
    else
	type = typval2type(argvars, get_copyID(), &type_list, TVTT_DO_MEMBER);
    name = type_name(type, &tofree);
    if (argvars[0].v_type == VAR_TYPEALIAS)
    {
	vim_snprintf((char *)IObuff, IOSIZE, "typealias<%s>", name);
	rettv->vval.v_string = vim_strsave((char_u *)IObuff);
	if (tofree != NULL)
	    vim_free(tofree);
    }
    else
    {
	if (tofree != NULL)
	    rettv->vval.v_string = (char_u *)tofree;
	else
	    rettv->vval.v_string = vim_strsave((char_u *)name);
    }
    clear_type_list(&type_list);
}

/*
 * Check if the typval_T is a value type; report an error if it is not.
 * Note: a type, user defined or typealias, is not a value type.
 *
 * Return OK if it's a value type, else FAIL
 */
    int
check_typval_is_value(typval_T *tv)
{
    if (tv == NULL)
	return OK;

    switch (tv->v_type)
    {
	case VAR_CLASS:
	    {
		class_T *cl = tv->vval.v_class;
		char_u *class_name = (cl == NULL) ? (char_u *)""
							: cl->class_name;
		if (cl != NULL && IS_ENUM(cl))
		    semsg(_(e_using_enum_as_value_str), class_name);
		else
		    semsg(_(e_using_class_as_value_str), class_name);
	    }
	    return FAIL;

	case VAR_TYPEALIAS:
	    semsg(_(e_using_typealias_as_value_str),
		    tv->vval.v_typealias->ta_name);
	    return FAIL;

	default:
	    break;
    }
    return OK;
}

/*
 * Same as above, except check type_T.
 */
    int
check_type_is_value(type_T *type)
{
    if (type == NULL)
	return OK;
    switch (type->tt_type)
    {
	case VAR_CLASS:
	    if (type->tt_class != NULL && IS_ENUM(type->tt_class))
		semsg(_(e_using_enum_as_value_str),
			type->tt_class->class_name);
	    else
		semsg(_(e_using_class_as_value_str),
			type->tt_class == NULL ? (char_u *)""
			: type->tt_class->class_name);
	    return FAIL;

	case VAR_TYPEALIAS:
	    // TODO: Not sure what could be done here to get a name.
	    //       Maybe an optional argument?
	    emsg(_(e_using_typealias_as_var_val));
	    return FAIL;

	default:
	    break;
    }
    return OK;
}

#endif // FEAT_EVAL
