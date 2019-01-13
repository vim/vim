/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * blob.c: Blob support by Yasuhiro Matsumoto
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
    blob_T *blob = (blob_T *)alloc_clear(sizeof(blob_T));

    if (blob != NULL)
	ga_init2(&blob->bv_ga, 1, 100);
    return blob;
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

    rettv_blob_set(rettv, b);
    return OK;
}

/*
 * Set a blob as the return value.
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
    ga_clear(&b->bv_ga);
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
 * Get the length of data.
 */
    long
blob_len(blob_T *b)
{
    if (b == NULL)
	return 0L;
    return b->bv_ga.ga_len;
}

/*
 * Get byte "idx" in blob "b".
 * Caller must check that "idx" is valid.
 */
    char_u
blob_get(blob_T *b, int idx)
{
    return ((char_u*)b->bv_ga.ga_data)[idx];
}

/*
 * Store one byte "c" in blob "b" at "idx".
 * Caller must make sure that "idx" is valid.
 */
    void
blob_set(blob_T *b, int idx, char_u c)
{
    ((char_u*)b->bv_ga.ga_data)[idx] = c;
}

/*
 * Return TRUE when two blobs have exactly the same values.
 */
    int
blob_equal(
    blob_T	*b1,
    blob_T	*b2)
{
    int i;

    if (b1 == NULL || b2 == NULL)
	return FALSE;
    if (b1 == b2)
	return TRUE;
    if (blob_len(b1) != blob_len(b2))
	return FALSE;

    for (i = 0; i < b1->bv_ga.ga_len; i++)
	if (blob_get(b1, i) != blob_get(b2, i)) return FALSE;
    return TRUE;
}

/*
 * Read "blob" from file "fd".
 * Return OK or FAIL.
 */
    int
read_blob(FILE *fd, blob_T *blob)
{
    struct stat	st;

    if (fstat(fileno(fd), &st) < 0)
	return FAIL;
    if (ga_grow(&blob->bv_ga, st.st_size) == FAIL)
	return FAIL;
    blob->bv_ga.ga_len = st.st_size;
    if (fread(blob->bv_ga.ga_data, 1, blob->bv_ga.ga_len, fd)
						  < (size_t)blob->bv_ga.ga_len)
	return FAIL;
    return OK;
}

/*
 * Write "blob" to file "fd".
 * Return OK or FAIL.
 */
    int
write_blob(FILE *fd, blob_T *blob)
{
    if (fwrite(blob->bv_ga.ga_data, 1, blob->bv_ga.ga_len, fd)
						  < (size_t)blob->bv_ga.ga_len)
    {
	EMSG(_(e_write));
	return FAIL;
    }
    return OK;
}

#endif /* defined(FEAT_EVAL) */
