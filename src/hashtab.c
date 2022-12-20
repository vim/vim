/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * hashtab.c: Handling of a hashtable with Vim-specific properties.
 *
 * Each item in a hashtable has a NUL terminated string key.  A key can appear
 * only once in the table.
 *
 * A hash number is computed from the key for quick lookup.  When the hashes
 * of two different keys point to the same entry an algorithm is used to
 * iterate over other entries in the table until the right one is found.
 * To make the iteration work removed keys are different from entries where a
 * key was never present.
 *
 * The mechanism has been partly based on how Python Dictionaries are
 * implemented.  The algorithm is from Knuth Vol. 3, Sec. 6.4.
 *
 * The hashtable grows to accommodate more entries when needed.  At least 1/3
 * of the entries is empty to keep the lookup efficient (at the cost of extra
 * memory).
 */

#include "vim.h"

#if 0
# define HT_DEBUG	// extra checks for table consistency  and statistics

static long_u hash_count_lookup = 0;	// count number of hashtab lookups
static long_u hash_count_perturb = 0;	// count number of "misses"
#endif

// Magic value for algorithm that walks through the array.
#define PERTURB_SHIFT 5

static hashitem_T *hash_do_lookup(hashitem_T *tbl, char_u *key, hash_T hash,
                                  long_u mask);
static hashitem_T *hash_do_rehash(hashitem_T *oldtbl, long_u oldused,
                                  hashitem_T *newtbl, long_u newmask);
static hashitem_T *hash_rehash_small(hashtab_T *ht);
static hashitem_T *hash_rehash_big(hashtab_T *ht, long_u newsize);
static long_u hash_calc_sz(long_u cap);
static int hash_may_resize(hashtab_T *ht, long_u minitems);

#if 0 // currently not used
/*
 * Create an empty hash table.
 * Returns NULL when out of memory.
 */
    hashtab_T *
hash_create(void)
{
    hashtab_T *ht;

    ht = ALLOC_ONE(hashtab_T);
    if (ht != NULL)
	hash_init(ht);
    return ht;
}
#endif

/*
 * Initialize an empty hash table.
 */
    void
hash_init(hashtab_T *ht)
{
    // This zeroes all "ht_" entries and all the "hi_key" in "ht_smallarray".
    CLEAR_POINTER(ht);
    ht->ht_array = ht->ht_smallarray;
    ht->ht_mask = HT_INIT_SIZE - 1;
}

/*
 * If "ht->ht_flags" has HTFLAGS_FROZEN then give an error message using
 * "command" and return TRUE.
 */
    int
check_hashtab_frozen(hashtab_T *ht, char *command)
{
    if ((ht->ht_flags & HTFLAGS_FROZEN) == 0)
	return FALSE;

    semsg(_(e_not_allowed_to_add_or_remove_entries_str), command);
    return TRUE;
}

/*
 * Free the array of a hash table.  Does not free the items it contains!
 * If "ht" is not freed then you should call hash_init() next!
 */
    void
hash_clear(hashtab_T *ht)
{
    if (ht->ht_array != ht->ht_smallarray)
	vim_free(ht->ht_array);
}

#if defined(FEAT_SPELL) || defined(FEAT_TERMINAL) || defined(PROTO)
/*
 * Free the array of a hash table and all the keys it contains.  The keys must
 * have been allocated.  "off" is the offset from the start of the allocate
 * memory to the location of the key (it's always positive).
 */
    void
hash_clear_all(hashtab_T *ht, int off)
{
    long_u	todo = ht->ht_used;
    for (hashitem_T *hi = ht->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    vim_free(hi->hi_key - off);
	    --todo;
	}
    }
    hash_clear(ht);
}
#endif

/*
 * Find "key" in hashtable "ht".  "key" must not be NULL.
 * Always returns a pointer to a hashitem.  If the item was not found then
 * HASHITEM_EMPTY() is TRUE.  The pointer is then the place where the key
 * would be added.
 * WARNING: The returned pointer becomes invalid when the hashtable is changed
 * (adding, setting or removing an item)!
 */
    hashitem_T *
hash_find(hashtab_T *ht, char_u *key)
{
    return hash_lookup(ht, key, hash_hash(key));
}


/*
 * Hash lookup function, for internal use
 */
    static hashitem_T *
hash_do_lookup(hashitem_T* tbl, char_u *key, hash_T hash, long_u mask)
{
    /*
     * Quickly handle the most common situations:
     * - return if there is no item at all
     * - skip over a removed item
     * - return if the item matches
     */
    long_u idx = (hash & mask);
    hashitem_T *hi = &tbl[idx], *freeitem;

#ifdef HT_DEBUG
    ++hash_count_lookup;
#endif
    if (NULL == hi->hi_key)
	return hi;
    else if (HI_KEY_REMOVED == hi->hi_key)
	freeitem = hi;
    else if (hi->hi_hash == hash && 0 == STRCMP(hi->hi_key, key))
	return hi;
    else
	freeitem = NULL;

    /*
     * Need to search through the table to find the key.  The algorithm
     * to step through the table starts with large steps, gradually becoming
     * smaller down to (1/4 table size + 1).  This means it goes through all
     * table entries in the end.
     * When we run into a NULL key it's clear that the key isn't there.
     * Return the first available slot found (can be a slot of a removed
     * item).
     */
    for (hash_T perturb = hash; ; perturb >>= PERTURB_SHIFT)
    {
#ifdef HT_DEBUG
	++hash_count_perturb;	    // count a "miss" for hashtab lookup
#endif
	idx = ((idx << 2U) + idx + perturb + 1U);
	hi = &tbl[idx & mask];
	if (NULL == hi->hi_key) {
	    return NULL == freeitem ? hi : freeitem;
        } else if (HI_KEY_REMOVED == hi->hi_key) {
            if (NULL == freeitem)
                freeitem = hi;
        } else if (hi->hi_hash == hash
		&& 0 == STRCMP(hi->hi_key, key)) {
	    return hi;
        }
    }
}

/*
 * Like hash_find(), but caller computes "hash".
 */
    hashitem_T *
hash_lookup(hashtab_T *ht, char_u *key, hash_T hash)
{
    return hash_do_lookup(ht->ht_array, key, hash, ht->ht_mask);
}

#if defined(FEAT_EVAL) || defined(FEAT_SYN_HL) || defined(PROTO)
/*
 * Print the efficiency of hashtable lookups.
 * Useful when trying different hash algorithms.
 * Called when exiting.
 */
    void
hash_debug_results(void)
{
# ifdef HT_DEBUG
    fprintf(stderr, "\r\n\r\n\r\n\r\n");
    fprintf(stderr, "Number of hashtable lookups: %lu\r\n", hash_count_lookup);
    fprintf(stderr, "Number of perturb loops: %lu\r\n", hash_count_perturb);
    fprintf(stderr, "Percentage of perturb loops: %lu%%\r\n",
				hash_count_perturb * 100lu / hash_count_lookup);
# endif
}
#endif

/*
 * Add item with key "key" to hashtable "ht".
 * "command" is used for the error message when the hashtab if frozen.
 * Returns FAIL when out of memory or the key is already present.
 */
    int
hash_add(hashtab_T *ht, char_u *key, char *command)
{
    if (!check_hashtab_frozen(ht, command)) {
        hash_T hash = hash_hash(key);
        hashitem_T *hi = hash_lookup(ht, key, hash);
        if (HASHITEM_EMPTY(hi)) {
            return hash_add_item(ht, hi, key, hash);
        }
        internal_error("hash_add()");
    }
    return FAIL;
}

/*
 * Add item "hi" with "key" to hashtable "ht".  "key" must not be NULL and
 * "hi" must have been obtained with hash_lookup() and point to an empty item.
 * "hi" is invalid after this!
 * Returns OK or FAIL (out of memory).
 */
    int
hash_add_item(
    hashtab_T	*ht,
    hashitem_T	*hi,
    char_u	*key,
    hash_T	hash)
{
    ++ht->ht_used;
    ++ht->ht_changed;
    ht->ht_filled += (NULL == hi->hi_key);
    hi->hi_key = key;
    hi->hi_hash = hash;

    // When the space gets low may resize the array.
    return hash_may_resize(ht, 0);
}

#if 0  // not used
/*
 * Overwrite hashtable item "hi" with "key".  "hi" must point to the item that
 * is to be overwritten.  Thus the number of items in the hashtable doesn't
 * change.
 * Although the key must be identical, the pointer may be different, thus it's
 * set anyway (the key is part of an item with that key).
 * The caller must take care of freeing the old item.
 * "hi" is invalid after this!
 */
    void
hash_set(hashitem_T *hi, char_u *key)
{
    hi->hi_key = key;
}
#endif

/*
 * Remove item "hi" from  hashtable "ht".  "hi" must have been obtained with
 * hash_lookup().
 * "command" is used for the error message when the hashtab if frozen.
 * The caller must take care of freeing the item itself.
 */
    int
hash_remove(hashtab_T *ht, hashitem_T *hi, char *command)
{
    if (check_hashtab_frozen(ht, command))
	return FAIL;
    --ht->ht_used;
    ++ht->ht_changed;
    hi->hi_key = HI_KEY_REMOVED;
    hash_may_resize(ht, 0);
    return OK;
}

/*
 * Lock a hashtable: prevent that ht_array changes.
 * Don't use this when items are to be added!
 * Must call hash_unlock() later.
 */
    void
hash_lock(hashtab_T *ht)
{
    ht->ht_flags |= HTFLAGS_LOCKED;
}

#if defined(FEAT_PROP_POPUP) || defined(PROTO)
/*
 * Lock a hashtable at the specified number of entries.
 * Caller must make sure no more than "size" entries will be added.
 * Must call hash_unlock() later.
 */
    void
hash_lock_size(hashtab_T *ht, int size)
{
    (void)hash_may_resize(ht, size);
    hash_lock(ht);
}
#endif

/*
 * Unlock a hashtable: allow ht_array changes again.
 * Table will be resized (shrink) when necessary.
 * This must balance a call to hash_lock().
 */
    void
hash_unlock(hashtab_T *ht)
{
    ht->ht_flags &= ~HTFLAGS_LOCKED;
    (void)hash_may_resize(ht, 0);
}

/*
 * Rehash from oldtbl into newtbl, returns newtbl.
 */
    static hashitem_T*
hash_do_rehash(hashitem_T *oldtbl, long_u oldused,
               hashitem_T *newtbl, long_u newmask)
{
#ifdef HT_DEBUG
    // when rehashing, we should freeze the counters for lookups and perturbs
    const long_u lookups = hash_count_lookup;
    const long_u perturbs = hash_count_perturb;
#endif
    for (; oldused > 0; ++oldtbl) {
        if (!HASHITEM_EMPTY(oldtbl)) {
            hashitem_T *hi = hash_do_lookup(newtbl, oldtbl->hi_key,
                                            oldtbl->hi_hash, newmask);
            *hi = *oldtbl;
            --oldused;
        }
    }
#ifdef HT_DEBUG
    // restore the "frozen" counters
    hash_count_lookup = lookups;
    hash_count_perturb = perturbs;
#endif
    return newtbl;
}

/*
 * Rehash into small array (allocates scratch area on the stack), returns
 * ht->ht_smallarray.
 */
    static hashitem_T*
hash_rehash_small(hashtab_T *ht)
{
    hashitem_T scratch[HT_INIT_SIZE];
    CLEAR_FIELD(scratch);
    return (hashitem_T *) mch_memmove(
        ht->ht_smallarray,
        hash_do_rehash(ht->ht_array, ht->ht_used, scratch, HT_INIT_SIZE - 1),
        sizeof(scratch));
}

/*
 * Allocate and rehash into a big array, returns new array, or NULL if out of
 * memory.
 */
    static hashitem_T*
hash_rehash_big(hashtab_T *ht, long_u newsize)
{
    hashitem_T* newarray = ALLOC_CLEAR_MULT(hashitem_T, newsize);
    if (NULL == newarray) return NULL;
    return hash_do_rehash(ht->ht_array, ht->ht_used, newarray, newsize - 1);
}


/*
 * Calculate the right size of a hash table for a given capacity. Returns 0 in
 * case of overflow.
 */
    static long_u
hash_calc_sz(long_u cap)
{
    long_u retVal = HT_INIT_SIZE;
    while (3 * cap >= (retVal << 1) && retVal)
        retVal <<= 1;
    return retVal;
}

/*
 * Shrink a hashtable when there is too much empty space.
 * Grow a hashtable when there is not enough empty space.
 * Returns OK or FAIL (out of memory).
 */
    static int
hash_may_resize(
    hashtab_T	*ht,
    long_u	minitems)		// minimal number of items
{
    // Don't resize a locked table.
    if (ht->ht_flags & HTFLAGS_LOCKED)
	return OK;

#ifdef HT_DEBUG
    if (ht->ht_used > ht->ht_filled)
	emsg("hash_may_resize(): more used than filled");
    if (ht->ht_filled >= ht->ht_mask + 1)
	emsg("hash_may_resize(): table completely filled");
#endif
    const long_u cap = (minitems < ht->ht_used) ? ht->ht_used : minitems;
    const long_u oldsz = 1u + ht->ht_mask;
    // assume all goes to plan (we set the error again in case things
    // exceptionally go wrong)
    ht->ht_flags &= ~HTFLAGS_ERROR;
    // exit quickly if the array's size is still okay
    if (3u * cap <= 2u * oldsz && (HT_INIT_SIZE == oldsz || 5u * cap >= oldsz) &&
        5u * (ht->ht_filled - ht->ht_used) <= oldsz) {
        return OK;
    } else {
        // calculate new size, and rehash
        const long_u newsz = hash_calc_sz(cap);
        if (newsz > 0) {
            hashitem_T *newarray = (HT_INIT_SIZE == newsz)
                                       ? hash_rehash_small(ht)
                                       : hash_rehash_big(ht, newsz);
            if (NULL != newarray) {
                if (ht->ht_array != ht->ht_smallarray)
                    vim_free(ht->ht_array);
                ht->ht_array = newarray;
                ht->ht_mask = newsz - 1;
                ht->ht_filled = ht->ht_used;
                ++ht->ht_changed;
                return OK;
            }
        }
    }
    ht->ht_flags |= HTFLAGS_ERROR;
    return FAIL;
}

/*
 * Get the hash number for a key.
 * If you think you know a better hash function: Compile with HT_DEBUG set and
 * run a script that uses hashtables a lot.  Vim will then print statistics
 * when exiting.  Try that with the current hash algorithm and yours.  The
 * lower the percentage the better.
 */
    hash_T
hash_hash(char_u *key)
{
    hash_T	hash = *key;
    if (hash) {
        // A simplistic algorithm that appears to do very well.
        // Suggested by George Reilly.
        for (++key; NUL != *key; ++key)
            hash = hash * 101 + *key;
    }

    return hash;
}
