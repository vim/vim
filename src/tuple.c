/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * tuple.c: Tuple support functions.
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

// Tuple heads for garbage collection.
static tuple_T		*first_tuple = NULL;	// list of all tuples

    static void
tuple_init(tuple_T *tuple)
{
    // Prepend the tuple to the list of tuples for garbage collection.
    if (first_tuple != NULL)
	first_tuple->tv_used_prev = tuple;
    tuple->tv_used_prev = NULL;
    tuple->tv_used_next = first_tuple;
    first_tuple = tuple;

    ga_init2(&tuple->tv_items, sizeof(typval_T), 20);
}

/*
 * Allocate an empty header for a tuple.
 * Caller should take care of the reference count.
 */
    tuple_T *
tuple_alloc(void)
{
    tuple_T  *tuple;

    tuple = ALLOC_CLEAR_ONE(tuple_T);
    if (tuple != NULL)
	tuple_init(tuple);
    return tuple;
}

/*
 * Allocate space for a tuple with "count" items.
 * This uses one allocation for efficiency.
 * The reference count is not set.
 * Next tuple_set_item() must be called for each item.
 */
    tuple_T *
tuple_alloc_with_items(int count)
{
    tuple_T	*tuple;

    tuple = tuple_alloc();
    if (tuple == NULL)
	return NULL;

    if (count <= 0)
	return tuple;

    if (ga_grow(&tuple->tv_items, count) == FAIL)
    {
	tuple_free(tuple);
	return NULL;
    }

    return tuple;
}

/*
 * Set item "idx" for a tuple previously allocated with
 * tuple_alloc_with_items().
 * The contents of "tv" is copied into the tuple item.
 * Each item must be set exactly once.
 */
    void
tuple_set_item(tuple_T *tuple, int idx, typval_T *tv)
{
    *TUPLE_ITEM(tuple, idx) = *tv;
    tuple->tv_items.ga_len++;
}

/*
 * Allocate an empty tuple for a return value, with reference count set.
 * Returns OK or FAIL.
 */
    int
rettv_tuple_alloc(typval_T *rettv)
{
    tuple_T	*tuple = tuple_alloc();

    if (tuple == NULL)
	return FAIL;

    rettv->v_lock = 0;
    rettv_tuple_set(rettv, tuple);
    return OK;
}

/*
 * Set a tuple as the return value.  Increments the reference count.
 */
    void
rettv_tuple_set(typval_T *rettv, tuple_T *tuple)
{
    rettv->v_type = VAR_TUPLE;
    rettv->vval.v_tuple = tuple;
    if (tuple != NULL)
	++tuple->tv_refcount;
}

/*
 * Set a new tuple with "count" items as the return value.
 * Returns OK on success and FAIL on allocation failure.
 */
    int
rettv_tuple_set_with_items(typval_T *rettv, int count)
{
    tuple_T *new_tuple;

    new_tuple = tuple_alloc_with_items(count);
    if (new_tuple == NULL)
	return FAIL;

    rettv_tuple_set(rettv, new_tuple);

    return OK;
}

/*
 * Unreference a tuple: decrement the reference count and free it when it
 * becomes zero.
 */
    void
tuple_unref(tuple_T *tuple)
{
    if (tuple != NULL && --tuple->tv_refcount <= 0)
	tuple_free(tuple);
}

/*
 * Free a tuple, including all non-container items it points to.
 * Ignores the reference count.
 */
    static void
tuple_free_contents(tuple_T *tuple)
{
    for (int i = 0; i < TUPLE_LEN(tuple); i++)
	clear_tv(TUPLE_ITEM(tuple, i));

    ga_clear(&tuple->tv_items);
}

/*
 * Go through the list of tuples and free items without the copyID.
 * But don't free a tuple that has a watcher (used in a for loop), these
 * are not referenced anywhere.
 */
    int
tuple_free_nonref(int copyID)
{
    tuple_T	*tt;
    int		did_free = FALSE;

    for (tt = first_tuple; tt != NULL; tt = tt->tv_used_next)
	if ((tt->tv_copyID & COPYID_MASK) != (copyID & COPYID_MASK))
	{
	    // Free the Tuple and ordinary items it contains, but don't recurse
	    // into Lists and Dictionaries, they will be in the list of dicts
	    // or list of lists.
	    tuple_free_contents(tt);
	    did_free = TRUE;
	}
    return did_free;
}

    static void
tuple_free_list(tuple_T  *tuple)
{
    // Remove the tuple from the list of tuples for garbage collection.
    if (tuple->tv_used_prev == NULL)
	first_tuple = tuple->tv_used_next;
    else
	tuple->tv_used_prev->tv_used_next = tuple->tv_used_next;
    if (tuple->tv_used_next != NULL)
	tuple->tv_used_next->tv_used_prev = tuple->tv_used_prev;

    free_type(tuple->tv_type);
    vim_free(tuple);
}

    void
tuple_free_items(int copyID)
{
    tuple_T	*tt, *tt_next;

    for (tt = first_tuple; tt != NULL; tt = tt_next)
    {
	tt_next = tt->tv_used_next;
	if ((tt->tv_copyID & COPYID_MASK) != (copyID & COPYID_MASK))
	{
	    // Free the tuple and ordinary items it contains, but don't recurse
	    // into Lists and Dictionaries, they will be in the list of dicts
	    // or list of lists.
	    tuple_free_list(tt);
	}
    }
}

    void
tuple_free(tuple_T *tuple)
{
    if (in_free_unref_items)
	return;

    tuple_free_contents(tuple);
    tuple_free_list(tuple);
}

/*
 * Get the number of items in a tuple.
 */
    long
tuple_len(tuple_T *tuple)
{
    if (tuple == NULL)
	return 0L;
    return tuple->tv_items.ga_len;
}

/*
 * Return TRUE when two tuples have exactly the same values.
 */
    int
tuple_equal(
    tuple_T	*t1,
    tuple_T	*t2,
    int		ic)	// ignore case for strings
{
    if (t1 == t2)
	return TRUE;

    int t1_len = tuple_len(t1);
    int t2_len = tuple_len(t2);

    if (t1_len != t2_len)
	return FALSE;

    if (t1_len == 0)
	// empty and NULL tuples are considered equal
	return TRUE;

    // If the tuples "t1" or "t2" is NULL, then it is handled by the length
    // checks above.

    for (int i = 0, j = 0; i < t1_len && j < t2_len; i++, j++)
	if (!tv_equal(TUPLE_ITEM(t1, i), TUPLE_ITEM(t2, j), ic))
	    return FALSE;

    return TRUE;
}

/*
 * Locate item with index "n" in tuple "tuple" and return it.
 * A negative index is counted from the end; -1 is the last item.
 * Returns NULL when "n" is out of range.
 */
    typval_T *
tuple_find(tuple_T *tuple, long n)
{
    if (tuple == NULL)
	return NULL;

    // Negative index is relative to the end.
    if (n < 0)
	n = TUPLE_LEN(tuple) + n;

    // Check for index out of range.
    if (n < 0 || n >= TUPLE_LEN(tuple))
	return NULL;

    return TUPLE_ITEM(tuple, n);
}

    int
tuple_append_tv(tuple_T *tuple, typval_T *tv)
{
    if (ga_grow(&tuple->tv_items, 1) == FAIL)
	return FAIL;

    tuple_set_item(tuple, TUPLE_LEN(tuple), tv);

    return OK;
}

/*
 * Concatenate tuples "t1" and "t2" into a new tuple, stored in "tv".
 * Return FAIL when out of memory.
 */
    int
tuple_concat(tuple_T *t1, tuple_T *t2, typval_T *tv)
{
    tuple_T	*tuple;

    // make a copy of the first tuple.
    if (t1 == NULL)
	tuple = tuple_alloc();
    else
	tuple = tuple_copy(t1, FALSE, TRUE, 0);
    if (tuple == NULL)
	return FAIL;

    tv->v_type = VAR_TUPLE;
    tv->v_lock = 0;
    tv->vval.v_tuple = tuple;
    if (t1 == NULL)
	++tuple->tv_refcount;

    // append all the items from the second tuple
    for (int i = 0; i < tuple_len(t2); i++)
    {
	typval_T    new_tv;

	copy_tv(TUPLE_ITEM(t2, i), &new_tv);

	if (tuple_append_tv(tuple, &new_tv) == FAIL)
	{
	    tuple_free(tuple);
	    return FAIL;
	}
    }

    return OK;
}

/*
 * Return a slice of tuple starting at index n1 and ending at index n2,
 * inclusive (tuple[n1 : n2])
 */
    tuple_T *
tuple_slice(tuple_T *tuple, long n1, long n2)
{
    tuple_T	*new_tuple;

    new_tuple = tuple_alloc_with_items(n2 - n1 + 1);
    if (new_tuple == NULL)
	return NULL;

    for (int i = n1; i <= n2; i++)
    {
	typval_T    new_tv;

	copy_tv(TUPLE_ITEM(tuple, i), &new_tv);

	if (tuple_append_tv(new_tuple, &new_tv) == FAIL)
	{
	    tuple_free(new_tuple);
	    return NULL;
	}
    }

    return new_tuple;
}

    int
tuple_slice_or_index(
    tuple_T	*tuple,
    int		range,
    varnumber_T	n1_arg,
    varnumber_T	n2_arg,
    int		exclusive,
    typval_T	*rettv,
    int		verbose)
{
    long	len = tuple_len(tuple);
    varnumber_T	n1 = n1_arg;
    varnumber_T	n2 = n2_arg;
    typval_T	var1;

    if (n1 < 0)
	n1 = len + n1;
    if (n1 < 0 || n1 >= len)
    {
	// For a range we allow invalid values and for legacy script return an
	// empty tuple, for Vim9 script start at the first item.
	// A tuple index out of range is an error.
	if (!range)
	{
	    if (verbose)
		semsg(_(e_tuple_index_out_of_range_nr), (long)n1_arg);
	    return FAIL;
	}
	if (in_vim9script())
	    n1 = n1 < 0 ? 0 : len;
	else
	    n1 = len;
    }
    if (range)
    {
	tuple_T	*new_tuple;

	if (n2 < 0)
	    n2 = len + n2;
	else if (n2 >= len)
	    n2 = len - (exclusive ? 0 : 1);
	if (exclusive)
	    --n2;
	if (n2 < 0 || n2 + 1 < n1)
	    n2 = -1;
	new_tuple = tuple_slice(tuple, n1, n2);
	if (new_tuple == NULL)
	    return FAIL;
	clear_tv(rettv);
	rettv_tuple_set(rettv, new_tuple);
    }
    else
    {
	// copy the item to "var1" to avoid that freeing the tuple makes it
	// invalid.
	copy_tv(tuple_find(tuple, n1), &var1);
	clear_tv(rettv);
	*rettv = var1;
    }
    return OK;
}

/*
 * Make a copy of tuple "orig".  Shallow if "deep" is FALSE.
 * The refcount of the new tuple is set to 1.
 * See item_copy() for "top" and "copyID".
 * Returns NULL when out of memory.
 */
    tuple_T *
tuple_copy(tuple_T *orig, int deep, int top, int copyID)
{
    tuple_T	*copy;
    int		idx;

    if (orig == NULL)
	return NULL;

    copy = tuple_alloc_with_items(TUPLE_LEN(orig));
    if (copy == NULL)
	return NULL;

    if (orig->tv_type == NULL || top || deep)
	copy->tv_type = NULL;
    else
	copy->tv_type = alloc_type(orig->tv_type);
    if (copyID != 0)
    {
	// Do this before adding the items, because one of the items may
	// refer back to this tuple.
	orig->tv_copyID = copyID;
	orig->tv_copytuple = copy;
    }

    for (idx = 0; idx < TUPLE_LEN(orig) && !got_int; idx++)
    {
	copy->tv_items.ga_len++;
	if (deep)
	{
	    if (item_copy(TUPLE_ITEM(orig, idx), TUPLE_ITEM(copy, idx),
						deep, FALSE, copyID) == FAIL)
		break;
	}
	else
	    copy_tv(TUPLE_ITEM(orig, idx), TUPLE_ITEM(copy, idx));
    }

    ++copy->tv_refcount;
    if (idx != TUPLE_LEN(orig))
    {
	tuple_unref(copy);
	copy = NULL;
    }

    return copy;
}

/*
 * Allocate a variable for a tuple and fill it from "*arg".
 * "*arg" points to the "," after the first element.
 * "rettv" contains the first element.
 * Returns OK or FAIL.
 */
    int
eval_tuple(char_u **arg, typval_T *rettv, evalarg_T *evalarg, int do_error)
{
    int		evaluate = evalarg == NULL ? FALSE
					 : evalarg->eval_flags & EVAL_EVALUATE;
    tuple_T	*tuple = NULL;
    typval_T	tv;
    int		vim9script = in_vim9script();
    int		had_comma;

    if (check_typval_is_value(rettv) == FAIL)
    {
	// the first item is not a valid value type
	clear_tv(rettv);
	return FAIL;
    }

    if (evaluate)
    {
	tuple = tuple_alloc();
	if (tuple == NULL)
	    return FAIL;

	if (rettv->v_type != VAR_UNKNOWN)
	{
	    // Add the first item to the tuple from "rettv"
	    if (tuple_append_tv(tuple, rettv) == FAIL)
		return FAIL;
	}
    }

    if (**arg == ')')
	// empty tuple
	goto done;

    if (vim9script && !IS_WHITE_NL_OR_NUL((*arg)[1]) && (*arg)[1] != ')')
    {
	semsg(_(e_white_space_required_after_str_str), ",", *arg);
	goto failret;
    }

    *arg = skipwhite_and_linebreak(*arg + 1, evalarg);
    while (**arg != ')' && **arg != NUL)
    {
	if (eval1(arg, &tv, evalarg) == FAIL)	// recursive!
	    goto failret;
	if (check_typval_is_value(&tv) == FAIL)
	{
	    if (evaluate)
		clear_tv(&tv);
	    goto failret;
	}

	if (evaluate)
	{
	    if (tuple_append_tv(tuple, &tv) == FAIL)
	    {
		clear_tv(&tv);
		goto failret;
	    }
	}

	if (!vim9script)
	    *arg = skipwhite(*arg);

	// the comma must come after the value
	had_comma = **arg == ',';
	if (had_comma)
	{
	    if (vim9script && !IS_WHITE_NL_OR_NUL((*arg)[1]) && (*arg)[1] != ')')
	    {
		semsg(_(e_white_space_required_after_str_str), ",", *arg);
		goto failret;
	    }
	    *arg = skipwhite(*arg + 1);
	}

	// The ")" can be on the next line.  But a double quoted string may
	// follow, not a comment.
	*arg = skipwhite_and_linebreak(*arg, evalarg);
	if (**arg == ')')
	    break;

	if (!had_comma)
	{
	    if (do_error)
	    {
		if (**arg == ',')
		    semsg(_(e_no_white_space_allowed_before_str_str),
								    ",", *arg);
		else
		    semsg(_(e_missing_comma_in_tuple_str), *arg);
	    }
	    goto failret;
	}
    }

    if (**arg != ')')
    {
	if (do_error)
	    semsg(_(e_missing_end_of_tuple_rsp_str), *arg);
failret:
	if (evaluate)
	    tuple_free(tuple);
	return FAIL;
    }

done:
    *arg += 1;
    if (evaluate)
	rettv_tuple_set(rettv, tuple);

    return OK;
}

/*
 * Lock or unlock a tuple.  "deep" is number of levels to go.
 * When "check_refcount" is TRUE do not lock a tuple with a reference
 * count larger than 1.
 */
    void
tuple_lock(tuple_T *tuple, int deep, int lock, int check_refcount)
{
    if (tuple == NULL || (check_refcount && tuple->tv_refcount > 1))
	return;

    if (lock)
	tuple->tv_lock |= VAR_LOCKED;
    else
	tuple->tv_lock &= ~VAR_LOCKED;

    if (deep < 0 || deep > 1)
    {
	// recursive: lock/unlock the items the Tuple contains
	for (int i = 0; i < TUPLE_LEN(tuple); i++)
	    item_lock(TUPLE_ITEM(tuple, i), deep - 1, lock, check_refcount);
    }
}

typedef struct join_S {
    char_u	*s;
    char_u	*tofree;
} join_T;

    static int
tuple_join_inner(
    garray_T	*gap,		// to store the result in
    tuple_T	*tuple,
    char_u	*sep,
    int		echo_style,
    int		restore_copyID,
    int		copyID,
    garray_T	*join_gap)	// to keep each tuple item string
{
    int		i;
    join_T	*p;
    int		len;
    int		sumlen = 0;
    int		first = TRUE;
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];
    char_u	*s;
    typval_T	*tv;

    // Stringify each item in the tuple.
    for (i = 0; i < TUPLE_LEN(tuple) && !got_int; i++)
    {
	tv = TUPLE_ITEM(tuple, i);
	s = echo_string_core(tv, &tofree, numbuf, copyID,
				      echo_style, restore_copyID, !echo_style);
	if (s == NULL)
	    return FAIL;

	len = (int)STRLEN(s);
	sumlen += len;

	(void)ga_grow(join_gap, 1);
	p = ((join_T *)join_gap->ga_data) + (join_gap->ga_len++);
	if (tofree != NULL || s != numbuf)
	{
	    p->s = s;
	    p->tofree = tofree;
	}
	else
	{
	    p->s = vim_strnsave(s, len);
	    p->tofree = p->s;
	}

	line_breakcheck();
	if (did_echo_string_emsg)  // recursion error, bail out
	    break;
    }

    // Allocate result buffer with its total size, avoid re-allocation and
    // multiple copy operations.  Add 2 for a tailing ')' and NUL.
    if (join_gap->ga_len >= 2)
	sumlen += (int)STRLEN(sep) * (join_gap->ga_len - 1);
    if (ga_grow(gap, sumlen + 2) == FAIL)
	return FAIL;

    for (i = 0; i < join_gap->ga_len && !got_int; ++i)
    {
	if (first)
	    first = FALSE;
	else
	    ga_concat(gap, sep);
	p = ((join_T *)join_gap->ga_data) + i;

	if (p->s != NULL)
	    ga_concat(gap, p->s);
	line_breakcheck();
    }

    // If there is only one item in the tuple, then add the separator after
    // that.
    if (join_gap->ga_len == 1)
	ga_concat(gap, sep);

    return OK;
}

/*
 * Join tuple "tuple" into a string in "*gap", using separator "sep".
 * When "echo_style" is TRUE use String as echoed, otherwise as inside a Tuple.
 * Return FAIL or OK.
 */
    int
tuple_join(
    garray_T	*gap,
    tuple_T	*tuple,
    char_u	*sep,
    int		echo_style,
    int		restore_copyID,
    int		copyID)
{
    garray_T	join_ga;
    int		retval;
    join_T	*p;
    int		i;

    if (TUPLE_LEN(tuple) < 1)
	return OK; // nothing to do
    ga_init2(&join_ga, sizeof(join_T), TUPLE_LEN(tuple));
    retval = tuple_join_inner(gap, tuple, sep, echo_style, restore_copyID,
							    copyID, &join_ga);

    if (join_ga.ga_data == NULL)
	return retval;

    // Dispose each item in join_ga.
    p = (join_T *)join_ga.ga_data;
    for (i = 0; i < join_ga.ga_len; ++i)
    {
	vim_free(p->tofree);
	++p;
    }
    ga_clear(&join_ga);

    return retval;
}

/*
 * Return an allocated string with the string representation of a tuple.
 * May return NULL.
 */
    char_u *
tuple2string(typval_T *tv, int copyID, int restore_copyID)
{
    garray_T	ga;

    if (tv->vval.v_tuple == NULL)
	return NULL;
    ga_init2(&ga, sizeof(char), 80);
    ga_append(&ga, '(');
    if (tuple_join(&ga, tv->vval.v_tuple, (char_u *)", ",
				       FALSE, restore_copyID, copyID) == FAIL)
    {
	vim_free(ga.ga_data);
	return NULL;
    }
    ga_append(&ga, ')');
    ga_append(&ga, NUL);
    return (char_u *)ga.ga_data;
}

/*
 * Implementation of foreach() for a Tuple.  Apply "expr" to
 * every item in Tuple "tuple" and return the result in "rettv".
 */
    void
tuple_foreach(
    tuple_T	*tuple,
    filtermap_T	filtermap,
    typval_T	*expr)
{
    int		len = tuple_len(tuple);
    int		rem;
    typval_T	newtv;
    funccall_T	*fc;

    // set_vim_var_nr() doesn't set the type
    set_vim_var_type(VV_KEY, VAR_NUMBER);

    // Create one funccall_T for all eval_expr_typval() calls.
    fc = eval_expr_get_funccal(expr, &newtv);

    for (int idx = 0; idx < len; idx++)
    {
	set_vim_var_nr(VV_KEY, idx);
	if (filter_map_one(TUPLE_ITEM(tuple, idx), expr, filtermap, fc,
						     &newtv, &rem) == FAIL)
	    break;
    }

    if (fc != NULL)
	remove_funccal();
}

/*
 * Count the number of times item "needle" occurs in Tuple "l" starting at index
 * "idx". Case is ignored if "ic" is TRUE.
 */
    long
tuple_count(tuple_T *tuple, typval_T *needle, long idx, int ic)
{
    long	n = 0;

    if (tuple == NULL)
	return 0;

    int	len = TUPLE_LEN(tuple);
    if (len == 0)
	return 0;

    if (idx < 0 || idx >= len)
    {
	semsg(_(e_tuple_index_out_of_range_nr), idx);
	return 0;
    }

    for (int i = idx; i < len; i++)
    {
	if (tv_equal(TUPLE_ITEM(tuple, i), needle, ic))
	    ++n;
    }

    return n;
}

/*
 * "items(tuple)" function
 * Caller must have already checked that argvars[0] is a tuple.
 */
    void
tuple2items(typval_T *argvars, typval_T *rettv)
{
    tuple_T	*tuple = argvars[0].vval.v_tuple;
    varnumber_T	idx;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    if (tuple == NULL)
	return;  // null tuple behaves like an empty list

    for (idx = 0; idx < TUPLE_LEN(tuple); idx++)
    {
	list_T	*l = list_alloc();

	if (l == NULL)
	    break;

	if (list_append_list(rettv->vval.v_list, l) == FAIL)
	{
	    vim_free(l);
	    break;
	}
	if (list_append_number(l, idx) == FAIL
		|| list_append_tv(l, TUPLE_ITEM(tuple, idx)) == FAIL)
	    break;
    }
}

/*
 * Search for item "tv" in tuple "tuple" starting from index "start_idx".
 * If "ic" is set to TRUE, then case is ignored.
 *
 * Returns the index where "tv" is present or -1 if it is not found.
 */
    int
index_tuple(tuple_T *tuple, typval_T *tv, int start_idx, int ic)
{
    if (start_idx < 0)
    {
	start_idx = TUPLE_LEN(tuple) + start_idx;
	if (start_idx < 0)
	    start_idx = 0;
    }

    for (int idx = start_idx; idx < TUPLE_LEN(tuple); idx++)
    {
	if (tv_equal(TUPLE_ITEM(tuple, idx), tv, ic))
	    return idx;
    }

    return -1;		// "tv" not found
}

/*
 * Evaluate 'expr' for each item in the Tuple 'tuple' starting with the item at
 * 'startidx' and return the index of the item where 'expr' is TRUE.  Returns
 * -1 if 'expr' doesn't evaluate to TRUE for any of the items.
 */
    int
indexof_tuple(tuple_T *tuple, long startidx, typval_T *expr)
{
    long	idx = 0;
    int		len;
    int		found;

    if (tuple == NULL)
	return -1;

    len = TUPLE_LEN(tuple);

    if (startidx < 0)
    {
	// negative index: index from the end
	startidx = len + startidx;
	if (startidx < 0)
	    startidx = 0;
    }

    set_vim_var_type(VV_KEY, VAR_NUMBER);

    int		called_emsg_start = called_emsg;

    for (idx = startidx; idx < len; idx++)
    {
	set_vim_var_nr(VV_KEY, idx);
	copy_tv(TUPLE_ITEM(tuple, idx), get_vim_var_tv(VV_VAL));

	found = indexof_eval_expr(expr);
	clear_tv(get_vim_var_tv(VV_VAL));

	if (found)
	    return idx;

	if (called_emsg != called_emsg_start)
	    return -1;
    }

    return -1;
}

/*
 * Return the max or min of the items in tuple "tuple".
 * If a tuple item is not a number, then "error" is set to TRUE.
 */
    varnumber_T
tuple_max_min(tuple_T *tuple, int domax, int *error)
{
    varnumber_T	n = 0;
    varnumber_T	v;

    if (tuple == NULL || TUPLE_LEN(tuple) == 0)
	return 0;

    n = tv_get_number_chk(TUPLE_ITEM(tuple, 0), error);
    if (*error)
	return n; // type error; errmsg already given

    for (int idx = 1; idx < TUPLE_LEN(tuple); idx++)
    {
	v = tv_get_number_chk(TUPLE_ITEM(tuple, idx), error);
	if (*error)
	    return n; // type error; errmsg already given
	if (domax ? v > n : v < n)
	    n = v;
    }

    return n;
}

/*
 * Repeat the tuple "tuple" "n" times and set "rettv" to the new tuple.
 */
    void
tuple_repeat(tuple_T *tuple, int n, typval_T *rettv)
{
    rettv->v_type = VAR_TUPLE;
    rettv->vval.v_tuple = NULL;

    if (tuple == NULL || TUPLE_LEN(tuple) == 0 || n <= 0)
	return;

    if (rettv_tuple_set_with_items(rettv, TUPLE_LEN(tuple) * n) == FAIL)
	return;

    tuple_T	*new_tuple = rettv->vval.v_tuple;
    for (int count = 0; count < n; count++)
    {
	for (int idx = 0; idx < TUPLE_LEN(tuple); idx++)
	{
	    copy_tv(TUPLE_ITEM(tuple, idx),
		    TUPLE_ITEM(new_tuple, TUPLE_LEN(new_tuple)));
	    new_tuple->tv_items.ga_len++;
	}
    }
}

/*
 * Reverse "tuple" and return the new tuple in "rettv"
 */
    void
tuple_reverse(tuple_T *tuple, typval_T *rettv)
{
    rettv->v_type = VAR_TUPLE;
    rettv->vval.v_tuple = NULL;

    int	len = tuple_len(tuple);

    if (len == 0)
	return;

    if (rettv_tuple_set_with_items(rettv, len) == FAIL)
	return;

    tuple_T	*new_tuple = rettv->vval.v_tuple;
    for (int i = 0; i < len; i++)
	copy_tv(TUPLE_ITEM(tuple, i), TUPLE_ITEM(new_tuple, len - i - 1));
    new_tuple->tv_items.ga_len = tuple->tv_items.ga_len;
}

/*
 * Tuple reduce() function
 */
    void
tuple_reduce(typval_T *argvars, typval_T *expr, typval_T *rettv)
{
    tuple_T	*tuple = argvars[0].vval.v_tuple;
    int		called_emsg_start = called_emsg;
    typval_T	initial;
    int		idx = 0;
    funccall_T	*fc;
    typval_T	argv[3];
    int		r;

    if (argvars[2].v_type == VAR_UNKNOWN)
    {
	if (tuple == NULL || TUPLE_LEN(tuple) == 0)
	{
	    semsg(_(e_reduce_of_an_empty_str_with_no_initial_value), "Tuple");
	    return;
	}
	initial = *TUPLE_ITEM(tuple, 0);
	idx = 1;
    }
    else
    {
	initial = argvars[2];
	idx = 0;
    }

    copy_tv(&initial, rettv);

    if (tuple == NULL)
	return;

    // Create one funccall_T for all eval_expr_typval() calls.
    fc = eval_expr_get_funccal(expr, rettv);

    for ( ; idx < TUPLE_LEN(tuple); idx++)
    {
	argv[0] = *rettv;
	rettv->v_type = VAR_UNKNOWN;
	argv[1] = *TUPLE_ITEM(tuple, idx);

	r = eval_expr_typval(expr, TRUE, argv, 2, fc, rettv);

	clear_tv(&argv[0]);

	if (r == FAIL || called_emsg != called_emsg_start)
	    break;
    }

    if (fc != NULL)
	remove_funccal();
}

/*
 * Returns TRUE if two tuples with types "type1" and "type2" are addable.
 * Otherwise returns FALSE.
 */
    int
check_tuples_addable(type_T *type1, type_T *type2)
{
    int	addable = TRUE;

    // If the first operand is a variadic tuple and the second argument is
    // non-variadic, then concatenation is not possible.
    if ((type1->tt_flags & TTFLAG_VARARGS)
	    && !(type2->tt_flags & TTFLAG_VARARGS)
	    && (type2->tt_argcount > 0))
	addable = FALSE;

    if ((type1->tt_flags & TTFLAG_VARARGS)
	    && (type2->tt_flags & TTFLAG_VARARGS))
    {
	// two variadic tuples
	if (type1->tt_argcount > 1 || type2->tt_argcount > 1)
	    // one of the variadic tuple has fixed number of items
	    addable = FALSE;
	else if ((type1->tt_argcount == 1 && type2->tt_argcount == 1)
		&& !equal_type(type1->tt_args[0], type2->tt_args[0], 0))
	    // the tuples have different item types
	    addable = FALSE;
    }

    if (!addable)
    {
	emsg(_(e_cannot_use_variadic_tuple_in_concatenation));
	return FAIL;
    }

    return OK;
}

#endif // defined(FEAT_EVAL)
