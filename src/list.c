/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * list.c: List support and container (List, Dict, Blob) functions.
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

static char *e_listblobarg = N_("E899: Argument of %s must be a List or Blob");

// List heads for garbage collection.
static list_T		*first_list = NULL;	// list of all lists

/*
 * Add a watcher to a list.
 */
    void
list_add_watch(list_T *l, listwatch_T *lw)
{
    lw->lw_next = l->lv_watch;
    l->lv_watch = lw;
}

/*
 * Remove a watcher from a list.
 * No warning when it isn't found...
 */
    void
list_rem_watch(list_T *l, listwatch_T *lwrem)
{
    listwatch_T	*lw, **lwp;

    lwp = &l->lv_watch;
    for (lw = l->lv_watch; lw != NULL; lw = lw->lw_next)
    {
	if (lw == lwrem)
	{
	    *lwp = lw->lw_next;
	    break;
	}
	lwp = &lw->lw_next;
    }
}

/*
 * Just before removing an item from a list: advance watchers to the next
 * item.
 */
    static void
list_fix_watch(list_T *l, listitem_T *item)
{
    listwatch_T	*lw;

    for (lw = l->lv_watch; lw != NULL; lw = lw->lw_next)
	if (lw->lw_item == item)
	    lw->lw_item = item->li_next;
}

    static void
list_init(list_T *l)
{
    // Prepend the list to the list of lists for garbage collection.
    if (first_list != NULL)
	first_list->lv_used_prev = l;
    l->lv_used_prev = NULL;
    l->lv_used_next = first_list;
    first_list = l;
}

/*
 * Allocate an empty header for a list.
 * Caller should take care of the reference count.
 */
    list_T *
list_alloc(void)
{
    list_T  *l;

    l = ALLOC_CLEAR_ONE(list_T);
    if (l != NULL)
	list_init(l);
    return l;
}

/*
 * list_alloc() with an ID for alloc_fail().
 */
    list_T *
list_alloc_id(alloc_id_T id UNUSED)
{
#ifdef FEAT_EVAL
    if (alloc_fail_id == id && alloc_does_fail(sizeof(list_T)))
	return NULL;
#endif
    return (list_alloc());
}

/*
 * Allocate space for a list, plus "count" items.
 * Next list_set_item() must be called for each item.
 */
    list_T *
list_alloc_with_items(int count)
{
    list_T	*l;

    l = (list_T *)alloc_clear(sizeof(list_T) + count * sizeof(listitem_T));
    if (l != NULL)
    {
	list_init(l);

	if (count > 0)
	{
	    listitem_T	*li = (listitem_T *)(l + 1);
	    int		i;

	    l->lv_len = count;
	    l->lv_with_items = count;
	    l->lv_first = li;
	    l->lv_last = li + count - 1;
	    for (i = 0; i < count; ++i)
	    {
		if (i == 0)
		    li->li_prev = NULL;
		else
		    li->li_prev = li - 1;
		if (i == count - 1)
		    li->li_next = NULL;
		else
		    li->li_next = li + 1;
		++li;
	    }
	}
    }
    return l;
}

/*
 * Set item "idx" for a list previously allocated with list_alloc_with_items().
 * The contents of "tv" is moved into the list item.
 * Each item must be set exactly once.
 */
    void
list_set_item(list_T *l, int idx, typval_T *tv)
{
    listitem_T	*li = (listitem_T *)(l + 1) + idx;

    li->li_tv = *tv;
}

/*
 * Allocate an empty list for a return value, with reference count set.
 * Returns OK or FAIL.
 */
    int
rettv_list_alloc(typval_T *rettv)
{
    list_T	*l = list_alloc();

    if (l == NULL)
	return FAIL;

    rettv->v_lock = 0;
    rettv_list_set(rettv, l);
    return OK;
}

/*
 * Same as rettv_list_alloc() but uses an allocation id for testing.
 */
    int
rettv_list_alloc_id(typval_T *rettv, alloc_id_T id UNUSED)
{
#ifdef FEAT_EVAL
    if (alloc_fail_id == id && alloc_does_fail(sizeof(list_T)))
	return FAIL;
#endif
    return rettv_list_alloc(rettv);
}


/*
 * Set a list as the return value.  Increments the reference count.
 */
    void
rettv_list_set(typval_T *rettv, list_T *l)
{
    rettv->v_type = VAR_LIST;
    rettv->vval.v_list = l;
    if (l != NULL)
	++l->lv_refcount;
}

/*
 * Unreference a list: decrement the reference count and free it when it
 * becomes zero.
 */
    void
list_unref(list_T *l)
{
    if (l != NULL && --l->lv_refcount <= 0)
	list_free(l);
}

/*
 * Free a list, including all non-container items it points to.
 * Ignores the reference count.
 */
    static void
list_free_contents(list_T *l)
{
    listitem_T *item;

    if (l->lv_first != &range_list_item)
	for (item = l->lv_first; item != NULL; item = l->lv_first)
	{
	    // Remove the item before deleting it.
	    l->lv_first = item->li_next;
	    clear_tv(&item->li_tv);
	    list_free_item(l, item);
	}
}

/*
 * Go through the list of lists and free items without the copyID.
 * But don't free a list that has a watcher (used in a for loop), these
 * are not referenced anywhere.
 */
    int
list_free_nonref(int copyID)
{
    list_T	*ll;
    int		did_free = FALSE;

    for (ll = first_list; ll != NULL; ll = ll->lv_used_next)
	if ((ll->lv_copyID & COPYID_MASK) != (copyID & COPYID_MASK)
						      && ll->lv_watch == NULL)
	{
	    // Free the List and ordinary items it contains, but don't recurse
	    // into Lists and Dictionaries, they will be in the list of dicts
	    // or list of lists.
	    list_free_contents(ll);
	    did_free = TRUE;
	}
    return did_free;
}

    static void
list_free_list(list_T  *l)
{
    // Remove the list from the list of lists for garbage collection.
    if (l->lv_used_prev == NULL)
	first_list = l->lv_used_next;
    else
	l->lv_used_prev->lv_used_next = l->lv_used_next;
    if (l->lv_used_next != NULL)
	l->lv_used_next->lv_used_prev = l->lv_used_prev;

    vim_free(l);
}

    void
list_free_items(int copyID)
{
    list_T	*ll, *ll_next;

    for (ll = first_list; ll != NULL; ll = ll_next)
    {
	ll_next = ll->lv_used_next;
	if ((ll->lv_copyID & COPYID_MASK) != (copyID & COPYID_MASK)
						      && ll->lv_watch == NULL)
	{
	    // Free the List and ordinary items it contains, but don't recurse
	    // into Lists and Dictionaries, they will be in the list of dicts
	    // or list of lists.
	    list_free_list(ll);
	}
    }
}

    void
list_free(list_T *l)
{
    if (!in_free_unref_items)
    {
	list_free_contents(l);
	list_free_list(l);
    }
}

/*
 * Allocate a list item.
 * It is not initialized, don't forget to set v_lock.
 */
    listitem_T *
listitem_alloc(void)
{
    return ALLOC_ONE(listitem_T);
}

/*
 * Free a list item, unless it was allocated together with the list itself.
 * Does not clear the value.  Does not notify watchers.
 */
    void
list_free_item(list_T *l, listitem_T *item)
{
    if (l->lv_with_items == 0 || item < (listitem_T *)l
			   || item >= (listitem_T *)(l + 1) + l->lv_with_items)
	vim_free(item);
}

/*
 * Free a list item, unless it was allocated together with the list itself.
 * Also clears the value.  Does not notify watchers.
 */
    void
listitem_free(list_T *l, listitem_T *item)
{
    clear_tv(&item->li_tv);
    list_free_item(l, item);
}

/*
 * Remove a list item from a List and free it.  Also clears the value.
 */
    void
listitem_remove(list_T *l, listitem_T *item)
{
    vimlist_remove(l, item, item);
    listitem_free(l, item);
}

/*
 * Get the number of items in a list.
 */
    long
list_len(list_T *l)
{
    if (l == NULL)
	return 0L;
    return l->lv_len;
}

/*
 * Return TRUE when two lists have exactly the same values.
 */
    int
list_equal(
    list_T	*l1,
    list_T	*l2,
    int		ic,	// ignore case for strings
    int		recursive)  // TRUE when used recursively
{
    listitem_T	*item1, *item2;

    if (l1 == NULL || l2 == NULL)
	return FALSE;
    if (l1 == l2)
	return TRUE;
    if (list_len(l1) != list_len(l2))
	return FALSE;

    range_list_materialize(l1);
    range_list_materialize(l2);

    for (item1 = l1->lv_first, item2 = l2->lv_first;
	    item1 != NULL && item2 != NULL;
			       item1 = item1->li_next, item2 = item2->li_next)
	if (!tv_equal(&item1->li_tv, &item2->li_tv, ic, recursive))
	    return FALSE;
    return item1 == NULL && item2 == NULL;
}

/*
 * Locate item with index "n" in list "l" and return it.
 * A negative index is counted from the end; -1 is the last item.
 * Returns NULL when "n" is out of range.
 */
    listitem_T *
list_find(list_T *l, long n)
{
    listitem_T	*item;
    long	idx;

    if (l == NULL)
	return NULL;

    // Negative index is relative to the end.
    if (n < 0)
	n = l->lv_len + n;

    // Check for index out of range.
    if (n < 0 || n >= l->lv_len)
	return NULL;

    range_list_materialize(l);

    // When there is a cached index may start search from there.
    if (l->lv_idx_item != NULL)
    {
	if (n < l->lv_idx / 2)
	{
	    // closest to the start of the list
	    item = l->lv_first;
	    idx = 0;
	}
	else if (n > (l->lv_idx + l->lv_len) / 2)
	{
	    // closest to the end of the list
	    item = l->lv_last;
	    idx = l->lv_len - 1;
	}
	else
	{
	    // closest to the cached index
	    item = l->lv_idx_item;
	    idx = l->lv_idx;
	}
    }
    else
    {
	if (n < l->lv_len / 2)
	{
	    // closest to the start of the list
	    item = l->lv_first;
	    idx = 0;
	}
	else
	{
	    // closest to the end of the list
	    item = l->lv_last;
	    idx = l->lv_len - 1;
	}
    }

    while (n > idx)
    {
	// search forward
	item = item->li_next;
	++idx;
    }
    while (n < idx)
    {
	// search backward
	item = item->li_prev;
	--idx;
    }

    // cache the used index
    l->lv_idx = idx;
    l->lv_idx_item = item;

    return item;
}

/*
 * Get list item "l[idx]" as a number.
 */
    long
list_find_nr(
    list_T	*l,
    long	idx,
    int		*errorp)	// set to TRUE when something wrong
{
    listitem_T	*li;

    if (l != NULL && l->lv_first == &range_list_item)
    {
	long	    n = idx;

	// not materialized range() list: compute the value.
	// Negative index is relative to the end.
	if (n < 0)
	    n = l->lv_len + n;

	// Check for index out of range.
	if (n < 0 || n >= l->lv_len)
	{
	    if (errorp != NULL)
		*errorp = TRUE;
	    return -1L;
	}

	return l->lv_start + n * l->lv_stride;
    }

    li = list_find(l, idx);
    if (li == NULL)
    {
	if (errorp != NULL)
	    *errorp = TRUE;
	return -1L;
    }
    return (long)tv_get_number_chk(&li->li_tv, errorp);
}

/*
 * Get list item "l[idx - 1]" as a string.  Returns NULL for failure.
 */
    char_u *
list_find_str(list_T *l, long idx)
{
    listitem_T	*li;

    li = list_find(l, idx - 1);
    if (li == NULL)
    {
	semsg(_(e_listidx), idx);
	return NULL;
    }
    return tv_get_string(&li->li_tv);
}

/*
 * Locate "item" list "l" and return its index.
 * Returns -1 when "item" is not in the list.
 */
    long
list_idx_of_item(list_T *l, listitem_T *item)
{
    long	idx = 0;
    listitem_T	*li;

    if (l == NULL)
	return -1;
    range_list_materialize(l);
    idx = 0;
    for (li = l->lv_first; li != NULL && li != item; li = li->li_next)
	++idx;
    if (li == NULL)
	return -1;
    return idx;
}

/*
 * Append item "item" to the end of list "l".
 */
    void
list_append(list_T *l, listitem_T *item)
{
    range_list_materialize(l);
    if (l->lv_last == NULL)
    {
	// empty list
	l->lv_first = item;
	l->lv_last = item;
	item->li_prev = NULL;
    }
    else
    {
	l->lv_last->li_next = item;
	item->li_prev = l->lv_last;
	l->lv_last = item;
    }
    ++l->lv_len;
    item->li_next = NULL;
}

/*
 * Append typval_T "tv" to the end of list "l".  "tv" is copied.
 * Return FAIL when out of memory.
 */
    int
list_append_tv(list_T *l, typval_T *tv)
{
    listitem_T	*li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    copy_tv(tv, &li->li_tv);
    list_append(l, li);
    return OK;
}

/*
 * As list_append_tv() but move the value instead of copying it.
 * Return FAIL when out of memory.
 */
    int
list_append_tv_move(list_T *l, typval_T *tv)
{
    listitem_T	*li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    li->li_tv = *tv;
    list_append(l, li);
    return OK;
}

/*
 * Add a dictionary to a list.  Used by getqflist().
 * Return FAIL when out of memory.
 */
    int
list_append_dict(list_T *list, dict_T *dict)
{
    listitem_T	*li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    li->li_tv.v_type = VAR_DICT;
    li->li_tv.v_lock = 0;
    li->li_tv.vval.v_dict = dict;
    list_append(list, li);
    ++dict->dv_refcount;
    return OK;
}

/*
 * Append list2 to list1.
 * Return FAIL when out of memory.
 */
    int
list_append_list(list_T *list1, list_T *list2)
{
    listitem_T	*li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    li->li_tv.v_type = VAR_LIST;
    li->li_tv.v_lock = 0;
    li->li_tv.vval.v_list = list2;
    list_append(list1, li);
    ++list2->lv_refcount;
    return OK;
}

/*
 * Make a copy of "str" and append it as an item to list "l".
 * When "len" >= 0 use "str[len]".
 * Returns FAIL when out of memory.
 */
    int
list_append_string(list_T *l, char_u *str, int len)
{
    listitem_T *li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    list_append(l, li);
    li->li_tv.v_type = VAR_STRING;
    li->li_tv.v_lock = 0;
    if (str == NULL)
	li->li_tv.vval.v_string = NULL;
    else if ((li->li_tv.vval.v_string = (len >= 0 ? vim_strnsave(str, len)
						 : vim_strsave(str))) == NULL)
	return FAIL;
    return OK;
}

/*
 * Append "n" to list "l".
 * Returns FAIL when out of memory.
 */
    int
list_append_number(list_T *l, varnumber_T n)
{
    listitem_T	*li;

    li = listitem_alloc();
    if (li == NULL)
	return FAIL;
    li->li_tv.v_type = VAR_NUMBER;
    li->li_tv.v_lock = 0;
    li->li_tv.vval.v_number = n;
    list_append(l, li);
    return OK;
}

/*
 * Insert typval_T "tv" in list "l" before "item".
 * If "item" is NULL append at the end.
 * Return FAIL when out of memory.
 */
    int
list_insert_tv(list_T *l, typval_T *tv, listitem_T *item)
{
    listitem_T	*ni = listitem_alloc();

    if (ni == NULL)
	return FAIL;
    copy_tv(tv, &ni->li_tv);
    list_insert(l, ni, item);
    return OK;
}

    void
list_insert(list_T *l, listitem_T *ni, listitem_T *item)
{
    range_list_materialize(l);
    if (item == NULL)
	// Append new item at end of list.
	list_append(l, ni);
    else
    {
	// Insert new item before existing item.
	ni->li_prev = item->li_prev;
	ni->li_next = item;
	if (item->li_prev == NULL)
	{
	    l->lv_first = ni;
	    ++l->lv_idx;
	}
	else
	{
	    item->li_prev->li_next = ni;
	    l->lv_idx_item = NULL;
	}
	item->li_prev = ni;
	++l->lv_len;
    }
}

/*
 * Extend "l1" with "l2".
 * If "bef" is NULL append at the end, otherwise insert before this item.
 * Returns FAIL when out of memory.
 */
    int
list_extend(list_T *l1, list_T *l2, listitem_T *bef)
{
    listitem_T	*item;
    int		todo = l2->lv_len;

    range_list_materialize(l1);
    range_list_materialize(l2);

    // We also quit the loop when we have inserted the original item count of
    // the list, avoid a hang when we extend a list with itself.
    for (item = l2->lv_first; item != NULL && --todo >= 0; item = item->li_next)
	if (list_insert_tv(l1, &item->li_tv, bef) == FAIL)
	    return FAIL;
    return OK;
}

/*
 * Concatenate lists "l1" and "l2" into a new list, stored in "tv".
 * Return FAIL when out of memory.
 */
    int
list_concat(list_T *l1, list_T *l2, typval_T *tv)
{
    list_T	*l;

    if (l1 == NULL || l2 == NULL)
	return FAIL;

    // make a copy of the first list.
    l = list_copy(l1, FALSE, 0);
    if (l == NULL)
	return FAIL;
    tv->v_type = VAR_LIST;
    tv->vval.v_list = l;

    // append all items from the second list
    return list_extend(l, l2, NULL);
}

/*
 * Make a copy of list "orig".  Shallow if "deep" is FALSE.
 * The refcount of the new list is set to 1.
 * See item_copy() for "copyID".
 * Returns NULL when out of memory.
 */
    list_T *
list_copy(list_T *orig, int deep, int copyID)
{
    list_T	*copy;
    listitem_T	*item;
    listitem_T	*ni;

    if (orig == NULL)
	return NULL;

    copy = list_alloc();
    if (copy != NULL)
    {
	if (copyID != 0)
	{
	    // Do this before adding the items, because one of the items may
	    // refer back to this list.
	    orig->lv_copyID = copyID;
	    orig->lv_copylist = copy;
	}
	range_list_materialize(orig);
	for (item = orig->lv_first; item != NULL && !got_int;
							 item = item->li_next)
	{
	    ni = listitem_alloc();
	    if (ni == NULL)
		break;
	    if (deep)
	    {
		if (item_copy(&item->li_tv, &ni->li_tv, deep, copyID) == FAIL)
		{
		    vim_free(ni);
		    break;
		}
	    }
	    else
		copy_tv(&item->li_tv, &ni->li_tv);
	    list_append(copy, ni);
	}
	++copy->lv_refcount;
	if (item != NULL)
	{
	    list_unref(copy);
	    copy = NULL;
	}
    }

    return copy;
}

/*
 * Remove items "item" to "item2" from list "l".
 * Does not free the listitem or the value!
 * This used to be called list_remove, but that conflicts with a Sun header
 * file.
 */
    void
vimlist_remove(list_T *l, listitem_T *item, listitem_T *item2)
{
    listitem_T	*ip;

    range_list_materialize(l);

    // notify watchers
    for (ip = item; ip != NULL; ip = ip->li_next)
    {
	--l->lv_len;
	list_fix_watch(l, ip);
	if (ip == item2)
	    break;
    }

    if (item2->li_next == NULL)
	l->lv_last = item->li_prev;
    else
	item2->li_next->li_prev = item->li_prev;
    if (item->li_prev == NULL)
	l->lv_first = item2->li_next;
    else
	item->li_prev->li_next = item2->li_next;
    l->lv_idx_item = NULL;
}

/*
 * Return an allocated string with the string representation of a list.
 * May return NULL.
 */
    char_u *
list2string(typval_T *tv, int copyID, int restore_copyID)
{
    garray_T	ga;

    if (tv->vval.v_list == NULL)
	return NULL;
    ga_init2(&ga, (int)sizeof(char), 80);
    ga_append(&ga, '[');
    range_list_materialize(tv->vval.v_list);
    if (list_join(&ga, tv->vval.v_list, (char_u *)", ",
				       FALSE, restore_copyID, copyID) == FAIL)
    {
	vim_free(ga.ga_data);
	return NULL;
    }
    ga_append(&ga, ']');
    ga_append(&ga, NUL);
    return (char_u *)ga.ga_data;
}

typedef struct join_S {
    char_u	*s;
    char_u	*tofree;
} join_T;

    static int
list_join_inner(
    garray_T	*gap,		// to store the result in
    list_T	*l,
    char_u	*sep,
    int		echo_style,
    int		restore_copyID,
    int		copyID,
    garray_T	*join_gap)	// to keep each list item string
{
    int		i;
    join_T	*p;
    int		len;
    int		sumlen = 0;
    int		first = TRUE;
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];
    listitem_T	*item;
    char_u	*s;

    // Stringify each item in the list.
    range_list_materialize(l);
    for (item = l->lv_first; item != NULL && !got_int; item = item->li_next)
    {
	s = echo_string_core(&item->li_tv, &tofree, numbuf, copyID,
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
    // multiple copy operations.  Add 2 for a tailing ']' and NUL.
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

    return OK;
}

/*
 * Join list "l" into a string in "*gap", using separator "sep".
 * When "echo_style" is TRUE use String as echoed, otherwise as inside a List.
 * Return FAIL or OK.
 */
    int
list_join(
    garray_T	*gap,
    list_T	*l,
    char_u	*sep,
    int		echo_style,
    int		restore_copyID,
    int		copyID)
{
    garray_T	join_ga;
    int		retval;
    join_T	*p;
    int		i;

    if (l->lv_len < 1)
	return OK; // nothing to do
    ga_init2(&join_ga, (int)sizeof(join_T), l->lv_len);
    retval = list_join_inner(gap, l, sep, echo_style, restore_copyID,
							    copyID, &join_ga);

    // Dispose each item in join_ga.
    if (join_ga.ga_data != NULL)
    {
	p = (join_T *)join_ga.ga_data;
	for (i = 0; i < join_ga.ga_len; ++i)
	{
	    vim_free(p->tofree);
	    ++p;
	}
	ga_clear(&join_ga);
    }

    return retval;
}

/*
 * "join()" function
 */
    void
f_join(typval_T *argvars, typval_T *rettv)
{
    garray_T	ga;
    char_u	*sep;

    if (argvars[0].v_type != VAR_LIST)
    {
	emsg(_(e_listreq));
	return;
    }
    if (argvars[0].vval.v_list == NULL)
	return;
    if (argvars[1].v_type == VAR_UNKNOWN)
	sep = (char_u *)" ";
    else
	sep = tv_get_string_chk(&argvars[1]);

    rettv->v_type = VAR_STRING;

    if (sep != NULL)
    {
	ga_init2(&ga, (int)sizeof(char), 80);
	list_join(&ga, argvars[0].vval.v_list, sep, TRUE, FALSE, 0);
	ga_append(&ga, NUL);
	rettv->vval.v_string = (char_u *)ga.ga_data;
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * Allocate a variable for a List and fill it from "*arg".
 * Return OK or FAIL.
 */
    int
get_list_tv(char_u **arg, typval_T *rettv, int evaluate, int do_error)
{
    list_T	*l = NULL;
    typval_T	tv;
    listitem_T	*item;

    if (evaluate)
    {
	l = list_alloc();
	if (l == NULL)
	    return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    while (**arg != ']' && **arg != NUL)
    {
	if (eval1(arg, &tv, evaluate) == FAIL)	// recursive!
	    goto failret;
	if (evaluate)
	{
	    item = listitem_alloc();
	    if (item != NULL)
	    {
		item->li_tv = tv;
		item->li_tv.v_lock = 0;
		list_append(l, item);
	    }
	    else
		clear_tv(&tv);
	}

	if (**arg == ']')
	    break;
	if (**arg != ',')
	{
	    if (do_error)
		semsg(_("E696: Missing comma in List: %s"), *arg);
	    goto failret;
	}
	*arg = skipwhite(*arg + 1);
    }

    if (**arg != ']')
    {
	if (do_error)
	    semsg(_("E697: Missing end of List ']': %s"), *arg);
failret:
	if (evaluate)
	    list_free(l);
	return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    if (evaluate)
	rettv_list_set(rettv, l);

    return OK;
}

/*
 * Write "list" of strings to file "fd".
 */
    int
write_list(FILE *fd, list_T *list, int binary)
{
    listitem_T	*li;
    int		c;
    int		ret = OK;
    char_u	*s;

    range_list_materialize(list);
    for (li = list->lv_first; li != NULL; li = li->li_next)
    {
	for (s = tv_get_string(&li->li_tv); *s != NUL; ++s)
	{
	    if (*s == '\n')
		c = putc(NUL, fd);
	    else
		c = putc(*s, fd);
	    if (c == EOF)
	    {
		ret = FAIL;
		break;
	    }
	}
	if (!binary || li->li_next != NULL)
	    if (putc('\n', fd) == EOF)
	    {
		ret = FAIL;
		break;
	    }
	if (ret == FAIL)
	{
	    emsg(_(e_write));
	    break;
	}
    }
    return ret;
}

/*
 * Initialize a static list with 10 items.
 */
    void
init_static_list(staticList10_T *sl)
{
    list_T  *l = &sl->sl_list;
    int	    i;

    memset(sl, 0, sizeof(staticList10_T));
    l->lv_first = &sl->sl_items[0];
    l->lv_last = &sl->sl_items[9];
    l->lv_refcount = DO_NOT_FREE_CNT;
    l->lv_lock = VAR_FIXED;
    sl->sl_list.lv_len = 10;

    for (i = 0; i < 10; ++i)
    {
	listitem_T *li = &sl->sl_items[i];

	if (i == 0)
	    li->li_prev = NULL;
	else
	    li->li_prev = li - 1;
	if (i == 9)
	    li->li_next = NULL;
	else
	    li->li_next = li + 1;
    }
}

/*
 * "list2str()" function
 */
    void
f_list2str(typval_T *argvars, typval_T *rettv)
{
    list_T	*l;
    listitem_T	*li;
    garray_T	ga;
    int		utf8 = FALSE;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    if (argvars[0].v_type != VAR_LIST)
    {
	emsg(_(e_invarg));
	return;
    }

    l = argvars[0].vval.v_list;
    if (l == NULL)
	return;  // empty list results in empty string

    if (argvars[1].v_type != VAR_UNKNOWN)
	utf8 = (int)tv_get_number_chk(&argvars[1], NULL);

    range_list_materialize(l);
    ga_init2(&ga, 1, 80);
    if (has_mbyte || utf8)
    {
	char_u	buf[MB_MAXBYTES + 1];
	int	(*char2bytes)(int, char_u *);

	if (utf8 || enc_utf8)
	    char2bytes = utf_char2bytes;
	else
	    char2bytes = mb_char2bytes;

	for (li = l->lv_first; li != NULL; li = li->li_next)
	{
	    buf[(*char2bytes)(tv_get_number(&li->li_tv), buf)] = NUL;
	    ga_concat(&ga, buf);
	}
	ga_append(&ga, NUL);
    }
    else if (ga_grow(&ga, list_len(l) + 1) == OK)
    {
	for (li = l->lv_first; li != NULL; li = li->li_next)
	    ga_append(&ga, tv_get_number(&li->li_tv));
	ga_append(&ga, NUL);
    }

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = ga.ga_data;
}

    void
list_remove(typval_T *argvars, typval_T *rettv, char_u *arg_errmsg)
{
    list_T	*l;
    listitem_T	*item, *item2;
    listitem_T	*li;
    int		error = FALSE;
    int		idx;

    if ((l = argvars[0].vval.v_list) == NULL
			      || var_check_lock(l->lv_lock, arg_errmsg, TRUE))
	return;

    idx = (long)tv_get_number_chk(&argvars[1], &error);
    if (error)
	;		// type error: do nothing, errmsg already given
    else if ((item = list_find(l, idx)) == NULL)
	semsg(_(e_listidx), idx);
    else
    {
	if (argvars[2].v_type == VAR_UNKNOWN)
	{
	    // Remove one item, return its value.
	    vimlist_remove(l, item, item);
	    *rettv = item->li_tv;
	    list_free_item(l, item);
	}
	else
	{
	    // Remove range of items, return list with values.
	    int end = (long)tv_get_number_chk(&argvars[2], &error);

	    if (error)
		;		// type error: do nothing
	    else if ((item2 = list_find(l, end)) == NULL)
		semsg(_(e_listidx), end);
	    else
	    {
		int	    cnt = 0;

		for (li = item; li != NULL; li = li->li_next)
		{
		    ++cnt;
		    if (li == item2)
			break;
		}
		if (li == NULL)  // didn't find "item2" after "item"
		    emsg(_(e_invrange));
		else
		{
		    vimlist_remove(l, item, item2);
		    if (rettv_list_alloc(rettv) == OK)
		    {
			l = rettv->vval.v_list;
			l->lv_first = item;
			l->lv_last = item2;
			item->li_prev = NULL;
			item2->li_next = NULL;
			l->lv_len = cnt;
		    }
		}
	    }
	}
    }
}

static int item_compare(const void *s1, const void *s2);
static int item_compare2(const void *s1, const void *s2);

// struct used in the array that's given to qsort()
typedef struct
{
    listitem_T	*item;
    int		idx;
} sortItem_T;

// struct storing information about current sort
typedef struct
{
    int		item_compare_ic;
    int		item_compare_numeric;
    int		item_compare_numbers;
#ifdef FEAT_FLOAT
    int		item_compare_float;
#endif
    char_u	*item_compare_func;
    partial_T	*item_compare_partial;
    dict_T	*item_compare_selfdict;
    int		item_compare_func_err;
    int		item_compare_keep_zero;
} sortinfo_T;
static sortinfo_T	*sortinfo = NULL;
#define ITEM_COMPARE_FAIL 999

/*
 * Compare functions for f_sort() and f_uniq() below.
 */
    static int
item_compare(const void *s1, const void *s2)
{
    sortItem_T  *si1, *si2;
    typval_T	*tv1, *tv2;
    char_u	*p1, *p2;
    char_u	*tofree1 = NULL, *tofree2 = NULL;
    int		res;
    char_u	numbuf1[NUMBUFLEN];
    char_u	numbuf2[NUMBUFLEN];

    si1 = (sortItem_T *)s1;
    si2 = (sortItem_T *)s2;
    tv1 = &si1->item->li_tv;
    tv2 = &si2->item->li_tv;

    if (sortinfo->item_compare_numbers)
    {
	varnumber_T	v1 = tv_get_number(tv1);
	varnumber_T	v2 = tv_get_number(tv2);

	return v1 == v2 ? 0 : v1 > v2 ? 1 : -1;
    }

#ifdef FEAT_FLOAT
    if (sortinfo->item_compare_float)
    {
	float_T	v1 = tv_get_float(tv1);
	float_T	v2 = tv_get_float(tv2);

	return v1 == v2 ? 0 : v1 > v2 ? 1 : -1;
    }
#endif

    // tv2string() puts quotes around a string and allocates memory.  Don't do
    // that for string variables. Use a single quote when comparing with a
    // non-string to do what the docs promise.
    if (tv1->v_type == VAR_STRING)
    {
	if (tv2->v_type != VAR_STRING || sortinfo->item_compare_numeric)
	    p1 = (char_u *)"'";
	else
	    p1 = tv1->vval.v_string;
    }
    else
	p1 = tv2string(tv1, &tofree1, numbuf1, 0);
    if (tv2->v_type == VAR_STRING)
    {
	if (tv1->v_type != VAR_STRING || sortinfo->item_compare_numeric)
	    p2 = (char_u *)"'";
	else
	    p2 = tv2->vval.v_string;
    }
    else
	p2 = tv2string(tv2, &tofree2, numbuf2, 0);
    if (p1 == NULL)
	p1 = (char_u *)"";
    if (p2 == NULL)
	p2 = (char_u *)"";
    if (!sortinfo->item_compare_numeric)
    {
	if (sortinfo->item_compare_ic)
	    res = STRICMP(p1, p2);
	else
	    res = STRCMP(p1, p2);
    }
    else
    {
	double n1, n2;
	n1 = strtod((char *)p1, (char **)&p1);
	n2 = strtod((char *)p2, (char **)&p2);
	res = n1 == n2 ? 0 : n1 > n2 ? 1 : -1;
    }

    // When the result would be zero, compare the item indexes.  Makes the
    // sort stable.
    if (res == 0 && !sortinfo->item_compare_keep_zero)
	res = si1->idx > si2->idx ? 1 : -1;

    vim_free(tofree1);
    vim_free(tofree2);
    return res;
}

    static int
item_compare2(const void *s1, const void *s2)
{
    sortItem_T  *si1, *si2;
    int		res;
    typval_T	rettv;
    typval_T	argv[3];
    char_u	*func_name;
    partial_T	*partial = sortinfo->item_compare_partial;
    funcexe_T	funcexe;

    // shortcut after failure in previous call; compare all items equal
    if (sortinfo->item_compare_func_err)
	return 0;

    si1 = (sortItem_T *)s1;
    si2 = (sortItem_T *)s2;

    if (partial == NULL)
	func_name = sortinfo->item_compare_func;
    else
	func_name = partial_name(partial);

    // Copy the values.  This is needed to be able to set v_lock to VAR_FIXED
    // in the copy without changing the original list items.
    copy_tv(&si1->item->li_tv, &argv[0]);
    copy_tv(&si2->item->li_tv, &argv[1]);

    rettv.v_type = VAR_UNKNOWN;		// clear_tv() uses this
    vim_memset(&funcexe, 0, sizeof(funcexe));
    funcexe.evaluate = TRUE;
    funcexe.partial = partial;
    funcexe.selfdict = sortinfo->item_compare_selfdict;
    res = call_func(func_name, -1, &rettv, 2, argv, &funcexe);
    clear_tv(&argv[0]);
    clear_tv(&argv[1]);

    if (res == FAIL)
	res = ITEM_COMPARE_FAIL;
    else
	res = (int)tv_get_number_chk(&rettv, &sortinfo->item_compare_func_err);
    if (sortinfo->item_compare_func_err)
	res = ITEM_COMPARE_FAIL;  // return value has wrong type
    clear_tv(&rettv);

    // When the result would be zero, compare the pointers themselves.  Makes
    // the sort stable.
    if (res == 0 && !sortinfo->item_compare_keep_zero)
	res = si1->idx > si2->idx ? 1 : -1;

    return res;
}

/*
 * "sort()" or "uniq()" function
 */
    static void
do_sort_uniq(typval_T *argvars, typval_T *rettv, int sort)
{
    list_T	*l;
    listitem_T	*li;
    sortItem_T	*ptrs;
    sortinfo_T	*old_sortinfo;
    sortinfo_T	info;
    long	len;
    long	i;

    // Pointer to current info struct used in compare function. Save and
    // restore the current one for nested calls.
    old_sortinfo = sortinfo;
    sortinfo = &info;

    if (argvars[0].v_type != VAR_LIST)
	semsg(_(e_listarg), sort ? "sort()" : "uniq()");
    else
    {
	l = argvars[0].vval.v_list;
	if (l == NULL || var_check_lock(l->lv_lock,
	     (char_u *)(sort ? N_("sort() argument") : N_("uniq() argument")),
									TRUE))
	    goto theend;
	rettv_list_set(rettv, l);
	range_list_materialize(l);

	len = list_len(l);
	if (len <= 1)
	    goto theend;	// short list sorts pretty quickly

	info.item_compare_ic = FALSE;
	info.item_compare_numeric = FALSE;
	info.item_compare_numbers = FALSE;
#ifdef FEAT_FLOAT
	info.item_compare_float = FALSE;
#endif
	info.item_compare_func = NULL;
	info.item_compare_partial = NULL;
	info.item_compare_selfdict = NULL;
	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    // optional second argument: {func}
	    if (argvars[1].v_type == VAR_FUNC)
		info.item_compare_func = argvars[1].vval.v_string;
	    else if (argvars[1].v_type == VAR_PARTIAL)
		info.item_compare_partial = argvars[1].vval.v_partial;
	    else
	    {
		int	    error = FALSE;

		i = (long)tv_get_number_chk(&argvars[1], &error);
		if (error)
		    goto theend;	// type error; errmsg already given
		if (i == 1)
		    info.item_compare_ic = TRUE;
		else if (argvars[1].v_type != VAR_NUMBER)
		    info.item_compare_func = tv_get_string(&argvars[1]);
		else if (i != 0)
		{
		    emsg(_(e_invarg));
		    goto theend;
		}
		if (info.item_compare_func != NULL)
		{
		    if (*info.item_compare_func == NUL)
		    {
			// empty string means default sort
			info.item_compare_func = NULL;
		    }
		    else if (STRCMP(info.item_compare_func, "n") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_numeric = TRUE;
		    }
		    else if (STRCMP(info.item_compare_func, "N") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_numbers = TRUE;
		    }
#ifdef FEAT_FLOAT
		    else if (STRCMP(info.item_compare_func, "f") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_float = TRUE;
		    }
#endif
		    else if (STRCMP(info.item_compare_func, "i") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_ic = TRUE;
		    }
		}
	    }

	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		// optional third argument: {dict}
		if (argvars[2].v_type != VAR_DICT)
		{
		    emsg(_(e_dictreq));
		    goto theend;
		}
		info.item_compare_selfdict = argvars[2].vval.v_dict;
	    }
	}

	// Make an array with each entry pointing to an item in the List.
	ptrs = ALLOC_MULT(sortItem_T, len);
	if (ptrs == NULL)
	    goto theend;

	i = 0;
	if (sort)
	{
	    // sort(): ptrs will be the list to sort
	    for (li = l->lv_first; li != NULL; li = li->li_next)
	    {
		ptrs[i].item = li;
		ptrs[i].idx = i;
		++i;
	    }

	    info.item_compare_func_err = FALSE;
	    info.item_compare_keep_zero = FALSE;
	    // test the compare function
	    if ((info.item_compare_func != NULL
					 || info.item_compare_partial != NULL)
		    && item_compare2((void *)&ptrs[0], (void *)&ptrs[1])
							 == ITEM_COMPARE_FAIL)
		emsg(_("E702: Sort compare function failed"));
	    else
	    {
		// Sort the array with item pointers.
		qsort((void *)ptrs, (size_t)len, sizeof(sortItem_T),
		    info.item_compare_func == NULL
					  && info.item_compare_partial == NULL
					       ? item_compare : item_compare2);

		if (!info.item_compare_func_err)
		{
		    // Clear the List and append the items in sorted order.
		    l->lv_first = l->lv_last = l->lv_idx_item = NULL;
		    l->lv_len = 0;
		    for (i = 0; i < len; ++i)
			list_append(l, ptrs[i].item);
		}
	    }
	}
	else
	{
	    int	(*item_compare_func_ptr)(const void *, const void *);

	    // f_uniq(): ptrs will be a stack of items to remove
	    info.item_compare_func_err = FALSE;
	    info.item_compare_keep_zero = TRUE;
	    item_compare_func_ptr = info.item_compare_func != NULL
					  || info.item_compare_partial != NULL
					       ? item_compare2 : item_compare;

	    for (li = l->lv_first; li != NULL && li->li_next != NULL;
							     li = li->li_next)
	    {
		if (item_compare_func_ptr((void *)&li, (void *)&li->li_next)
									 == 0)
		    ptrs[i++].item = li;
		if (info.item_compare_func_err)
		{
		    emsg(_("E882: Uniq compare function failed"));
		    break;
		}
	    }

	    if (!info.item_compare_func_err)
	    {
		while (--i >= 0)
		{
		    li = ptrs[i].item->li_next;
		    ptrs[i].item->li_next = li->li_next;
		    if (li->li_next != NULL)
			li->li_next->li_prev = ptrs[i].item;
		    else
			l->lv_last = ptrs[i].item;
		    list_fix_watch(l, li);
		    listitem_free(l, li);
		    l->lv_len--;
		}
	    }
	}

	vim_free(ptrs);
    }
theend:
    sortinfo = old_sortinfo;
}

/*
 * "sort({list})" function
 */
    void
f_sort(typval_T *argvars, typval_T *rettv)
{
    do_sort_uniq(argvars, rettv, TRUE);
}

/*
 * "uniq({list})" function
 */
    void
f_uniq(typval_T *argvars, typval_T *rettv)
{
    do_sort_uniq(argvars, rettv, FALSE);
}

/*
 * Handle one item for map() and filter().
 */
    static int
filter_map_one(typval_T *tv, typval_T *expr, int map, int *remp)
{
    typval_T	rettv;
    typval_T	argv[3];
    int		retval = FAIL;

    copy_tv(tv, get_vim_var_tv(VV_VAL));
    argv[0] = *get_vim_var_tv(VV_KEY);
    argv[1] = *get_vim_var_tv(VV_VAL);
    if (eval_expr_typval(expr, argv, 2, &rettv) == FAIL)
	goto theend;
    if (map)
    {
	// map(): replace the list item value
	clear_tv(tv);
	rettv.v_lock = 0;
	*tv = rettv;
    }
    else
    {
	int	    error = FALSE;

	// filter(): when expr is zero remove the item
	*remp = (tv_get_number_chk(&rettv, &error) == 0);
	clear_tv(&rettv);
	// On type error, nothing has been removed; return FAIL to stop the
	// loop.  The error message was given by tv_get_number_chk().
	if (error)
	    goto theend;
    }
    retval = OK;
theend:
    clear_tv(get_vim_var_tv(VV_VAL));
    return retval;
}

/*
 * Implementation of map() and filter().
 */
    static void
filter_map(typval_T *argvars, typval_T *rettv, int map)
{
    typval_T	*expr;
    listitem_T	*li, *nli;
    list_T	*l = NULL;
    dictitem_T	*di;
    hashtab_T	*ht;
    hashitem_T	*hi;
    dict_T	*d = NULL;
    blob_T	*b = NULL;
    int		rem;
    int		todo;
    char_u	*ermsg = (char_u *)(map ? "map()" : "filter()");
    char_u	*arg_errmsg = (char_u *)(map ? N_("map() argument")
				   : N_("filter() argument"));
    int		save_did_emsg;
    int		idx = 0;

    if (argvars[0].v_type == VAR_BLOB)
    {
	if ((b = argvars[0].vval.v_blob) == NULL)
	    return;
    }
    else if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) == NULL
	      || (!map && var_check_lock(l->lv_lock, arg_errmsg, TRUE)))
	    return;
    }
    else if (argvars[0].v_type == VAR_DICT)
    {
	if ((d = argvars[0].vval.v_dict) == NULL
	      || (!map && var_check_lock(d->dv_lock, arg_errmsg, TRUE)))
	    return;
    }
    else
    {
	semsg(_(e_listdictarg), ermsg);
	return;
    }

    expr = &argvars[1];
    // On type errors, the preceding call has already displayed an error
    // message.  Avoid a misleading error message for an empty string that
    // was not passed as argument.
    if (expr->v_type != VAR_UNKNOWN)
    {
	typval_T	save_val;
	typval_T	save_key;

	prepare_vimvar(VV_VAL, &save_val);
	prepare_vimvar(VV_KEY, &save_key);

	// We reset "did_emsg" to be able to detect whether an error
	// occurred during evaluation of the expression.
	save_did_emsg = did_emsg;
	did_emsg = FALSE;

	if (argvars[0].v_type == VAR_DICT)
	{
	    ht = &d->dv_hashtab;
	    hash_lock(ht);
	    todo = (int)ht->ht_used;
	    for (hi = ht->ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    int r;

		    --todo;
		    di = HI2DI(hi);
		    if (map && (var_check_lock(di->di_tv.v_lock,
							   arg_errmsg, TRUE)
				|| var_check_ro(di->di_flags,
							   arg_errmsg, TRUE)))
			break;
		    set_vim_var_string(VV_KEY, di->di_key, -1);
		    r = filter_map_one(&di->di_tv, expr, map, &rem);
		    clear_tv(get_vim_var_tv(VV_KEY));
		    if (r == FAIL || did_emsg)
			break;
		    if (!map && rem)
		    {
			if (var_check_fixed(di->di_flags, arg_errmsg, TRUE)
			    || var_check_ro(di->di_flags, arg_errmsg, TRUE))
			    break;
			dictitem_remove(d, di);
		    }
		}
	    }
	    hash_unlock(ht);
	}
	else if (argvars[0].v_type == VAR_BLOB)
	{
	    int		i;
	    typval_T	tv;
	    varnumber_T	val;

	    // set_vim_var_nr() doesn't set the type
	    set_vim_var_type(VV_KEY, VAR_NUMBER);

	    for (i = 0; i < b->bv_ga.ga_len; i++)
	    {
		tv.v_type = VAR_NUMBER;
		val = blob_get(b, i);
		tv.vval.v_number = val;
		set_vim_var_nr(VV_KEY, idx);
		if (filter_map_one(&tv, expr, map, &rem) == FAIL || did_emsg)
		    break;
		if (tv.v_type != VAR_NUMBER)
		{
		    emsg(_(e_invalblob));
		    break;
		}
		if (map)
		{
		    if (tv.vval.v_number != val)
			blob_set(b, i, tv.vval.v_number);
		}
		else if (rem)
		{
		    char_u *p = (char_u *)argvars[0].vval.v_blob->bv_ga.ga_data;

		    mch_memmove(p + i, p + i + 1,
					      (size_t)b->bv_ga.ga_len - i - 1);
		    --b->bv_ga.ga_len;
		    --i;
		}
		++idx;
	    }
	}
	else // argvars[0].v_type == VAR_LIST
	{
	    // set_vim_var_nr() doesn't set the type
	    set_vim_var_type(VV_KEY, VAR_NUMBER);

	    range_list_materialize(l);
	    for (li = l->lv_first; li != NULL; li = nli)
	    {
		if (map && var_check_lock(li->li_tv.v_lock, arg_errmsg, TRUE))
		    break;
		nli = li->li_next;
		set_vim_var_nr(VV_KEY, idx);
		if (filter_map_one(&li->li_tv, expr, map, &rem) == FAIL
								  || did_emsg)
		    break;
		if (!map && rem)
		    listitem_remove(l, li);
		++idx;
	    }
	}

	restore_vimvar(VV_KEY, &save_key);
	restore_vimvar(VV_VAL, &save_val);

	did_emsg |= save_did_emsg;
    }

    copy_tv(&argvars[0], rettv);
}

/*
 * "filter()" function
 */
    void
f_filter(typval_T *argvars, typval_T *rettv)
{
    filter_map(argvars, rettv, FALSE);
}

/*
 * "map()" function
 */
    void
f_map(typval_T *argvars, typval_T *rettv)
{
    filter_map(argvars, rettv, TRUE);
}

/*
 * "add(list, item)" function
 */
    void
f_add(typval_T *argvars, typval_T *rettv)
{
    list_T	*l;
    blob_T	*b;

    rettv->vval.v_number = 1; // Default: Failed
    if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) != NULL
		&& !var_check_lock(l->lv_lock,
					 (char_u *)N_("add() argument"), TRUE)
		&& list_append_tv(l, &argvars[1]) == OK)
	    copy_tv(&argvars[0], rettv);
    }
    else if (argvars[0].v_type == VAR_BLOB)
    {
	if ((b = argvars[0].vval.v_blob) != NULL
		&& !var_check_lock(b->bv_lock,
					 (char_u *)N_("add() argument"), TRUE))
	{
	    int		error = FALSE;
	    varnumber_T n = tv_get_number_chk(&argvars[1], &error);

	    if (!error)
	    {
		ga_append(&b->bv_ga, (int)n);
		copy_tv(&argvars[0], rettv);
	    }
	}
    }
    else
	emsg(_(e_listblobreq));
}

/*
 * "count()" function
 */
    void
f_count(typval_T *argvars, typval_T *rettv)
{
    long	n = 0;
    int		ic = FALSE;
    int		error = FALSE;

    if (argvars[2].v_type != VAR_UNKNOWN)
	ic = (int)tv_get_number_chk(&argvars[2], &error);

    if (argvars[0].v_type == VAR_STRING)
    {
	char_u *expr = tv_get_string_chk(&argvars[1]);
	char_u *p = argvars[0].vval.v_string;
	char_u *next;

	if (!error && expr != NULL && *expr != NUL && p != NULL)
	{
	    if (ic)
	    {
		size_t len = STRLEN(expr);

		while (*p != NUL)
		{
		    if (MB_STRNICMP(p, expr, len) == 0)
		    {
			++n;
			p += len;
		    }
		    else
			MB_PTR_ADV(p);
		}
	    }
	    else
		while ((next = (char_u *)strstr((char *)p, (char *)expr))
								       != NULL)
		{
		    ++n;
		    p = next + STRLEN(expr);
		}
	}

    }
    else if (argvars[0].v_type == VAR_LIST)
    {
	listitem_T	*li;
	list_T		*l;
	long		idx;

	if ((l = argvars[0].vval.v_list) != NULL)
	{
	    li = l->lv_first;
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		if (argvars[3].v_type != VAR_UNKNOWN)
		{
		    idx = (long)tv_get_number_chk(&argvars[3], &error);
		    if (!error)
		    {
			li = list_find(l, idx);
			if (li == NULL)
			    semsg(_(e_listidx), idx);
		    }
		}
		if (error)
		    li = NULL;
	    }

	    for ( ; li != NULL; li = li->li_next)
		if (tv_equal(&li->li_tv, &argvars[1], ic, FALSE))
		    ++n;
	}
    }
    else if (argvars[0].v_type == VAR_DICT)
    {
	int		todo;
	dict_T		*d;
	hashitem_T	*hi;

	if ((d = argvars[0].vval.v_dict) != NULL)
	{
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		if (argvars[3].v_type != VAR_UNKNOWN)
		    emsg(_(e_invarg));
	    }

	    todo = error ? 0 : (int)d->dv_hashtab.ht_used;
	    for (hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;
		    if (tv_equal(&HI2DI(hi)->di_tv, &argvars[1], ic, FALSE))
			++n;
		}
	    }
	}
    }
    else
	semsg(_(e_listdictarg), "count()");
    rettv->vval.v_number = n;
}

/*
 * "extend(list, list [, idx])" function
 * "extend(dict, dict [, action])" function
 */
    void
f_extend(typval_T *argvars, typval_T *rettv)
{
    char_u      *arg_errmsg = (char_u *)N_("extend() argument");

    if (argvars[0].v_type == VAR_LIST && argvars[1].v_type == VAR_LIST)
    {
	list_T		*l1, *l2;
	listitem_T	*item;
	long		before;
	int		error = FALSE;

	l1 = argvars[0].vval.v_list;
	l2 = argvars[1].vval.v_list;
	if (l1 != NULL && !var_check_lock(l1->lv_lock, arg_errmsg, TRUE)
		&& l2 != NULL)
	{
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		before = (long)tv_get_number_chk(&argvars[2], &error);
		if (error)
		    return;		// type error; errmsg already given

		if (before == l1->lv_len)
		    item = NULL;
		else
		{
		    item = list_find(l1, before);
		    if (item == NULL)
		    {
			semsg(_(e_listidx), before);
			return;
		    }
		}
	    }
	    else
		item = NULL;
	    list_extend(l1, l2, item);

	    copy_tv(&argvars[0], rettv);
	}
    }
    else if (argvars[0].v_type == VAR_DICT && argvars[1].v_type == VAR_DICT)
    {
	dict_T	*d1, *d2;
	char_u	*action;
	int	i;

	d1 = argvars[0].vval.v_dict;
	d2 = argvars[1].vval.v_dict;
	if (d1 != NULL && !var_check_lock(d1->dv_lock, arg_errmsg, TRUE)
		&& d2 != NULL)
	{
	    // Check the third argument.
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		static char *(av[]) = {"keep", "force", "error"};

		action = tv_get_string_chk(&argvars[2]);
		if (action == NULL)
		    return;		// type error; errmsg already given
		for (i = 0; i < 3; ++i)
		    if (STRCMP(action, av[i]) == 0)
			break;
		if (i == 3)
		{
		    semsg(_(e_invarg2), action);
		    return;
		}
	    }
	    else
		action = (char_u *)"force";

	    dict_extend(d1, d2, action);

	    copy_tv(&argvars[0], rettv);
	}
    }
    else
	semsg(_(e_listdictarg), "extend()");
}

/*
 * "insert()" function
 */
    void
f_insert(typval_T *argvars, typval_T *rettv)
{
    long	before = 0;
    listitem_T	*item;
    list_T	*l;
    int		error = FALSE;

    if (argvars[0].v_type == VAR_BLOB)
    {
	int	    val, len;
	char_u	    *p;

	len = blob_len(argvars[0].vval.v_blob);
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    before = (long)tv_get_number_chk(&argvars[2], &error);
	    if (error)
		return;		// type error; errmsg already given
	    if (before < 0 || before > len)
	    {
		semsg(_(e_invarg2), tv_get_string(&argvars[2]));
		return;
	    }
	}
	val = tv_get_number_chk(&argvars[1], &error);
	if (error)
	    return;
	if (val < 0 || val > 255)
	{
	    semsg(_(e_invarg2), tv_get_string(&argvars[1]));
	    return;
	}

	if (ga_grow(&argvars[0].vval.v_blob->bv_ga, 1) == FAIL)
	    return;
	p = (char_u *)argvars[0].vval.v_blob->bv_ga.ga_data;
	mch_memmove(p + before + 1, p + before, (size_t)len - before);
	*(p + before) = val;
	++argvars[0].vval.v_blob->bv_ga.ga_len;

	copy_tv(&argvars[0], rettv);
    }
    else if (argvars[0].v_type != VAR_LIST)
	semsg(_(e_listblobarg), "insert()");
    else if ((l = argvars[0].vval.v_list) != NULL
	    && !var_check_lock(l->lv_lock,
				     (char_u *)N_("insert() argument"), TRUE))
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    before = (long)tv_get_number_chk(&argvars[2], &error);
	if (error)
	    return;		// type error; errmsg already given

	if (before == l->lv_len)
	    item = NULL;
	else
	{
	    item = list_find(l, before);
	    if (item == NULL)
	    {
		semsg(_(e_listidx), before);
		l = NULL;
	    }
	}
	if (l != NULL)
	{
	    list_insert_tv(l, &argvars[1], item);
	    copy_tv(&argvars[0], rettv);
	}
    }
}

/*
 * "remove()" function
 */
    void
f_remove(typval_T *argvars, typval_T *rettv)
{
    char_u	*arg_errmsg = (char_u *)N_("remove() argument");

    if (argvars[0].v_type == VAR_DICT)
	dict_remove(argvars, rettv, arg_errmsg);
    else if (argvars[0].v_type == VAR_BLOB)
	blob_remove(argvars, rettv);
    else if (argvars[0].v_type == VAR_LIST)
	list_remove(argvars, rettv, arg_errmsg);
    else
	semsg(_(e_listdictblobarg), "remove()");
}

/*
 * "reverse({list})" function
 */
    void
f_reverse(typval_T *argvars, typval_T *rettv)
{
    list_T	*l;
    listitem_T	*li, *ni;

    if (argvars[0].v_type == VAR_BLOB)
    {
	blob_T	*b = argvars[0].vval.v_blob;
	int	i, len = blob_len(b);

	for (i = 0; i < len / 2; i++)
	{
	    int tmp = blob_get(b, i);

	    blob_set(b, i, blob_get(b, len - i - 1));
	    blob_set(b, len - i - 1, tmp);
	}
	rettv_blob_set(rettv, b);
	return;
    }

    if (argvars[0].v_type != VAR_LIST)
	semsg(_(e_listblobarg), "reverse()");
    else if ((l = argvars[0].vval.v_list) != NULL
	    && !var_check_lock(l->lv_lock,
				    (char_u *)N_("reverse() argument"), TRUE))
    {
	li = l->lv_last;
	l->lv_first = l->lv_last = NULL;
	l->lv_len = 0;
	while (li != NULL)
	{
	    ni = li->li_prev;
	    list_append(l, li);
	    li = ni;
	}
	rettv_list_set(rettv, l);
	l->lv_idx = l->lv_len - l->lv_idx - 1;
    }
}

#endif // defined(FEAT_EVAL)
