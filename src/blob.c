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

    int
blob_copy(typval_T *from, typval_T *to)
{
    int	    ret = OK;

    to->v_type = VAR_BLOB;
    if (from->vval.v_blob == NULL)
	to->vval.v_blob = NULL;
    else if (rettv_blob_alloc(to) == FAIL)
	ret = FAIL;
    else
    {
	int  len = from->vval.v_blob->bv_ga.ga_len;

	if (len > 0)
	{
	    to->vval.v_blob->bv_ga.ga_data =
			    vim_memsave(from->vval.v_blob->bv_ga.ga_data, len);
	    if (to->vval.v_blob->bv_ga.ga_data == NULL)
		len = 0;
	}
	to->vval.v_blob->bv_ga.ga_len = len;
    }
    return ret;
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
    int
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
    int	    i;
    int	    len1 = blob_len(b1);
    int	    len2 = blob_len(b2);

    // empty and NULL are considered the same
    if (len1 == 0 && len2 == 0)
	return TRUE;
    if (b1 == b2)
	return TRUE;
    if (len1 != len2)
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
	emsg(_(e_write));
	return FAIL;
    }
    return OK;
}

/*
 * Convert a blob to a readable form: "0z00112233.44556677.8899"
 */
    char_u *
blob2string(blob_T *blob, char_u **tofree, char_u *numbuf)
{
    int		i;
    garray_T    ga;

    if (blob == NULL)
    {
	*tofree = NULL;
	return (char_u *)"0z";
    }

    // Store bytes in the growarray.
    ga_init2(&ga, 1, 4000);
    ga_concat(&ga, (char_u *)"0z");
    for (i = 0; i < blob_len(blob); i++)
    {
	if (i > 0 && (i & 3) == 0)
	    ga_concat(&ga, (char_u *)".");
	vim_snprintf((char *)numbuf, NUMBUFLEN, "%02X", (int)blob_get(blob, i));
	ga_concat(&ga, numbuf);
    }
    *tofree = ga.ga_data;
    return *tofree;
}

/*
 * Convert a string variable, in the format of blob2string(), to a blob.
 * Return NULL when conversion failed.
 */
    blob_T *
string2blob(char_u *str)
{
    blob_T  *blob = blob_alloc();
    char_u  *s = str;

    if (blob == NULL)
	return NULL;
    if (s[0] != '0' || (s[1] != 'z' && s[1] != 'Z'))
	goto failed;
    s += 2;
    while (vim_isxdigit(*s))
    {
	if (!vim_isxdigit(s[1]))
	    goto failed;
	ga_append(&blob->bv_ga, (hex2nr(s[0]) << 4) + hex2nr(s[1]));
	s += 2;
	if (*s == '.' && vim_isxdigit(s[1]))
	    ++s;
    }
    if (*skipwhite(s) != NUL)
	goto failed;  // text after final digit

    ++blob->bv_refcount;
    return blob;

failed:
    blob_free(blob);
    return NULL;
}

#endif /* defined(FEAT_EVAL) */
