/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * blob.c: Blob support
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Allocate an empty blob.
 * Caller should take care of the reference count.
 */
    blob_T *
blob_alloc(void)
{
    return (blob_T *)alloc_clear(sizeof(blob_T));
}

/*
 * Allocate an empty blob for a return value, with reference count set.
 * Returns OK or FAIL.
 */
    int
rettv_blob_alloc(typval_T *rettv)
{
    blob_T	*b = blob_alloc();

    if (b == NULL)
	return FAIL;

    rettv->v_lock = 0;
    rettv_blob_set(rettv, b);
    return OK;
}

/*
 * Set a blob as the return value
 */
    void
rettv_blob_set(typval_T *rettv, blob_T *b)
{
    rettv->v_type = VAR_BLOB;
    rettv->vval.v_blob = b;
    if (b != NULL)
	++b->bv_refcount;
}

    void
blob_free(blob_T *b)
{
    vim_free(b->bv_buf);
    vim_free(b);
}

/*
 * Unreference a blob: decrement the reference count and free it when it
 * becomes zero.
 */
    void
blob_unref(blob_T *b)
{
    if (b != NULL && --b->bv_refcount <= 0)
	blob_free(b);
}

/*
 * Get the length of buffer.
 */
    long
blob_len(blob_T *b)
{
    if (b == NULL)
	return 0L;
    return b->bv_len;
}

/*
 * Return TRUE when two blobs have exactly the same values.
 */
    int
blob_equal(
    blob_T	*b1,
    blob_T	*b2)
{
  	size_t i;
    if (b1 == NULL || b2 == NULL)
	return FALSE;
    if (b1 == b2)
	return TRUE;
    if (blob_len(b1) != blob_len(b2))
	return FALSE;

    for (i = 0; i < b1->bv_len; i++)
	  	if (b1->bv_buf[i] != b2->bv_buf[i]) return FALSE;
}

/*
 * Make a copy of blob "orig".  Shallow if "deep" is FALSE.
 * The refcount of the new blob is set to 1.
 * See item_copy() for "copyID".
 * Returns NULL when out of memory.
 */
    blob_T *
blob_copy(blob_T *orig, int deep, int copyID)
{
  	size_t i;
    blob_T	*copy;

    if (orig == NULL)
	return NULL;

    copy = blob_alloc();
    if (copy != NULL)
    {
	copy->bv_len = orig->bv_len;
	copy->bv_buf = alloc(orig->bv_len);
	for (i = 0; i < orig->bv_len; i++)
	    copy->bv_buf[i] = orig->bv_buf[i];
	++copy->bv_refcount;
    }

    return copy;
}

/*
 * Allocate a variable for a Blob and fill it from "*arg".
 * Return OK or FAIL.
 */
    int
get_blob_tv(char_u **arg, typval_T *rettv, int evaluate)
{
    blob_T	*l = NULL;
    typval_T	tv;

    if (evaluate)
    {
	l = blob_alloc();
	if (l == NULL)
	    return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    while (**arg != ']' && **arg != NUL)
    {
#if 0
	if (eval7(arg, &tv, evaluate) == FAIL)	/* recursive! */
	    goto failret;
	if (evaluate)
	{
	    item = listitem_alloc();
	    if (item != NULL)
	    {
		item->li_tv = tv;
		item->li_tv.v_lock = 0;
		blob_append(l, item);
	    }
	    else
		clear_tv(&tv);
	}
#endif
	++*arg;

	if (**arg == '"')
	    break;
	*arg = skipwhite(*arg + 1);
    }

    if (**arg != '"')
    {
	EMSG2(_("E697: Missing end of Blob '\"': %s"), *arg);
failret:
	if (evaluate)
	    blob_free(l);
	return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    if (evaluate)
	rettv_blob_set(rettv, l);

    return OK;
}

/*
 * Read "blob" from file "fd".
 */
    int
read_blob(FILE *fd, blob_T *blob)
{
    int		n;
    int		ret = OK;
    struct stat	st;

    if (fstat(fileno(fd), &st) < 0)
	return FAIL;
    blob->bv_cap = blob->bv_len = st.st_size;
    blob->bv_buf = alloc_clear(blob->bv_len);
    if (fread(blob->bv_buf, 1, blob->bv_len, fd) < 0)
    {
	return FAIL;
    }
    return OK;
}

/*
 * Write "blob" to file "fd".
 */
    int
write_blob(FILE *fd, blob_T *blob)
{
    int		n;
    int		ret = OK;

    if (fwrite(blob->bv_buf, 1, blob->bv_len, fd) < 0)
    {
	EMSG(_(e_write));
	return FAIL;
    }
    return OK;
}

#endif /* defined(FEAT_EVAL) */
