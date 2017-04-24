/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * list.c: List support
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

/* List heads for garbage collection. */
static list_T		*first_list = NULL;	/* list of all lists */

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
    void
list_fix_watch(list_T *l, listitem_T *item)
{
    listwatch_T	*lw;

    for (lw = l->lv_watch; lw != NULL; lw = lw->lw_next)
	if (lw->lw_item == item)
	    lw->lw_item = item->li_next;
}

/*
 * Allocate an empty header for a list.
 * Caller should take care of the reference count.
 */
    list_T *
list_alloc(void)
{
    list_T  *l;

    l = (list_T *)alloc_clear(sizeof(list_T));
    if (l != NULL)
    {
	/* Prepend the list to the list of lists for garbage collection. */
	if (first_list != NULL)
	    first_list->lv_used_prev = l;
	l->lv_used_prev = NULL;
	l->lv_used_next = first_list;
	first_list = l;
    }
    return l;
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

    rettv->vval.v_list = l;
    rettv->v_type = VAR_LIST;
    rettv->v_lock = 0;
    ++l->lv_refcount;
    return OK;
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

    for (item = l->lv_first; item != NULL; item = l->lv_first)
    {
	/* Remove the item before deleting it. */
	l->lv_first = item->li_next;
	clear_tv(&item->li_tv);
	vim_free(item);
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
	    /* Free the List and ordinary items it contains, but don't recurse
	     * into Lists and Dictionaries, they will be in the list of dicts
	     * or list of lists. */
	    list_free_contents(ll);
	    did_free = TRUE;
	}
    return did_free;
}

    static void
list_free_list(list_T  *l)
{
    /* Remove the list from the list of lists for garbage collection. */
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
	    /* Free the List and ordinary items it contains, but don't recurse
	     * into Lists and Dictionaries, they will be in the list of dicts
	     * or list of lists. */
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
    return (listitem_T *)alloc(sizeof(listitem_T));
}

/*
 * Free a list item.  Also clears the value.  Does not notify watchers.
 */
    void
listitem_free(listitem_T *item)
{
    clear_tv(&item->li_tv);
    vim_free(item);
}

/*
 * Remove a list item from a List and free it.  Also clears the value.
 */
    void
listitem_remove(list_T *l, listitem_T *item)
{
    vimlist_remove(l, item, item);
    listitem_free(item);
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
    int		ic,	/* ignore case for strings */
    int		recursive)  /* TRUE when used recursively */
{
    listitem_T	*item1, *item2;

    if (l1 == NULL || l2 == NULL)
	return FALSE;
    if (l1 == l2)
	return TRUE;
    if (list_len(l1) != list_len(l2))
	return FALSE;

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

    /* Negative index is relative to the end. */
    if (n < 0)
	n = l->lv_len + n;

    /* Check for index out of range. */
    if (n < 0 || n >= l->lv_len)
	return NULL;

    /* When there is a cached index may start search from there. */
    if (l->lv_idx_item != NULL)
    {
	if (n < l->lv_idx / 2)
	{
	    /* closest to the start of the list */
	    item = l->lv_first;
	    idx = 0;
	}
	else if (n > (l->lv_idx + l->lv_len) / 2)
	{
	    /* closest to the end of the list */
	    item = l->lv_last;
	    idx = l->lv_len - 1;
	}
	else
	{
	    /* closest to the cached index */
	    item = l->lv_idx_item;
	    idx = l->lv_idx;
	}
    }
    else
    {
	if (n < l->lv_len / 2)
	{
	    /* closest to the start of the list */
	    item = l->lv_first;
	    idx = 0;
	}
	else
	{
	    /* closest to the end of the list */
	    item = l->lv_last;
	    idx = l->lv_len - 1;
	}
    }

    while (n > idx)
    {
	/* search forward */
	item = item->li_next;
	++idx;
    }
    while (n < idx)
    {
	/* search backward */
	item = item->li_prev;
	--idx;
    }

    /* cache the used index */
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
    int		*errorp)	/* set to TRUE when something wrong */
{
    listitem_T	*li;

    li = list_find(l, idx);
    if (li == NULL)
    {
	if (errorp != NULL)
	    *errorp = TRUE;
	return -1L;
    }
    return (long)get_tv_number_chk(&li->li_tv, errorp);
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
	EMSGN(_(e_listidx), idx);
	return NULL;
    }
    return get_tv_string(&li->li_tv);
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
    if (l->lv_last == NULL)
    {
	/* empty list */
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
 * Append typval_T "tv" to the end of list "l".
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
    if (item == NULL)
	/* Append new item at end of list. */
	list_append(l, ni);
    else
    {
	/* Insert new item before existing item. */
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

    /* We also quit the loop when we have inserted the original item count of
     * the list, avoid a hang when we extend a list with itself. */
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

    /* make a copy of the first list. */
    l = list_copy(l1, FALSE, 0);
    if (l == NULL)
	return FAIL;
    tv->v_type = VAR_LIST;
    tv->vval.v_list = l;

    /* append all items from the second list */
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
	    /* Do this before adding the items, because one of the items may
	     * refer back to this list. */
	    orig->lv_copyID = copyID;
	    orig->lv_copylist = copy;
	}
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

    /* notify watchers */
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
    garray_T	*gap,		/* to store the result in */
    list_T	*l,
    char_u	*sep,
    int		echo_style,
    int		restore_copyID,
    int		copyID,
    garray_T	*join_gap)	/* to keep each list item string */
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

    /* Stringify each item in the list. */
    for (item = l->lv_first; item != NULL && !got_int; item = item->li_next)
    {
	s = echo_string_core(&item->li_tv, &tofree, numbuf, copyID,
					   echo_style, restore_copyID, FALSE);
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
	if (did_echo_string_emsg)  /* recursion error, bail out */
	    break;
    }

    /* Allocate result buffer with its total size, avoid re-allocation and
     * multiple copy operations.  Add 2 for a tailing ']' and NUL. */
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
	return OK; /* nothing to do */
    ga_init2(&join_ga, (int)sizeof(join_T), l->lv_len);
    retval = list_join_inner(gap, l, sep, echo_style, restore_copyID,
							    copyID, &join_ga);

    /* Dispose each item in join_ga. */
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
 * Allocate a variable for a List and fill it from "*arg".
 * Return OK or FAIL.
 */
    int
get_list_tv(char_u **arg, typval_T *rettv, int evaluate)
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
	if (eval1(arg, &tv, evaluate) == FAIL)	/* recursive! */
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
	    EMSG2(_("E696: Missing comma in List: %s"), *arg);
	    goto failret;
	}
	*arg = skipwhite(*arg + 1);
    }

    if (**arg != ']')
    {
	EMSG2(_("E697: Missing end of List ']': %s"), *arg);
failret:
	if (evaluate)
	    list_free(l);
	return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    if (evaluate)
    {
	rettv->v_type = VAR_LIST;
	rettv->vval.v_list = l;
	++l->lv_refcount;
    }

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

    for (li = list->lv_first; li != NULL; li = li->li_next)
    {
	for (s = get_tv_string(&li->li_tv); *s != NUL; ++s)
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
	    EMSG(_(e_write));
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

#endif /* defined(FEAT_EVAL) */
