/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * crypt.c: Generic encryption support.
 */
#include "vim.h"

#if defined(FEAT_CRYPT) || defined(PROTO)
/*
 * Optional encryption support.
 * Mohsin Ahmed, mosh@sasi.com, 1998-09-24
 * Based on zip/crypt sources.
 * Refactored by David Leadbeater, 2014.
 *
 * NOTE FOR USA: Since 2000 exporting this code from the USA is allowed to
 * most countries.  There are a few exceptions, but that still should not be a
 * problem since this code was originally created in Europe and India.
 *
 * Blowfish addition originally made by Mohsin Ahmed,
 * http://www.cs.albany.edu/~mosh 2010-03-14
 * Based on blowfish by Bruce Schneier (http://www.schneier.com/blowfish.html)
 * and sha256 by Christophe Devine.
 */

typedef struct {
    char    *name;	/* encryption name as used in 'cryptmethod' */
    char    *magic;	/* magic bytes stored in file header */
    int	    salt_len;	/* length of salt, or 0 when not using salt */
    int	    seed_len;	/* length of seed, or 0 when not using salt */
#ifdef CRYPT_NOT_INPLACE
    int	    works_inplace; /* encryption/decryption can be done in-place */
#endif
    int	    whole_undofile; /* whole undo file is encrypted */

    /* Optional function pointer for a self-test. */
    int (* self_test_fn)();

    // Function pointer for initializing encryption/decryption.
    int (* init_fn)(cryptstate_T *state, char_u *key,
		      char_u *salt, int salt_len, char_u *seed, int seed_len);

    /* Function pointers for encoding/decoding from one buffer into another.
     * Optional, however, these or the _buffer ones should be configured. */
    void (*encode_fn)(cryptstate_T *state, char_u *from, size_t len,
								  char_u *to);
    void (*decode_fn)(cryptstate_T *state, char_u *from, size_t len,
								  char_u *to);

    /* Function pointers for encoding and decoding, can buffer data if needed.
     * Optional (however, these or the above should be configured). */
    long (*encode_buffer_fn)(cryptstate_T *state, char_u *from, size_t len,
							     char_u **newptr);
    long (*decode_buffer_fn)(cryptstate_T *state, char_u *from, size_t len,
							     char_u **newptr);

    /* Function pointers for in-place encoding and decoding, used for
     * crypt_*_inplace(). "from" and "to" arguments will be equal.
     * These may be the same as decode_fn and encode_fn above, however an
     * algorithm may implement them in a way that is not interchangeable with
     * the crypt_(en|de)code() interface (for example because it wishes to add
     * padding to files).
     * This method is used for swap and undo files which have a rigid format.
     */
    void (*encode_inplace_fn)(cryptstate_T *state, char_u *p1, size_t len,
								  char_u *p2);
    void (*decode_inplace_fn)(cryptstate_T *state, char_u *p1, size_t len,
								  char_u *p2);
} cryptmethod_T;

/* index is method_nr of cryptstate_T, CRYPT_M_* */
static cryptmethod_T cryptmethods[CRYPT_M_COUNT] = {
    /* PK_Zip; very weak */
    {
	"zip",
	"VimCrypt~01!",
	0,
	0,
#ifdef CRYPT_NOT_INPLACE
	TRUE,
#endif
	FALSE,
	NULL,
	crypt_zip_init,
	crypt_zip_encode, crypt_zip_decode,
	NULL, NULL,
	crypt_zip_encode, crypt_zip_decode,
    },

    /* Blowfish/CFB + SHA-256 custom key derivation; implementation issues. */
    {
	"blowfish",
	"VimCrypt~02!",
	8,
	8,
#ifdef CRYPT_NOT_INPLACE
	TRUE,
#endif
	FALSE,
	blowfish_self_test,
	crypt_blowfish_init,
	crypt_blowfish_encode, crypt_blowfish_decode,
	NULL, NULL,
	crypt_blowfish_encode, crypt_blowfish_decode,
    },

    /* Blowfish/CFB + SHA-256 custom key derivation; fixed. */
    {
	"blowfish2",
	"VimCrypt~03!",
	8,
	8,
#ifdef CRYPT_NOT_INPLACE
	TRUE,
#endif
	TRUE,
	blowfish_self_test,
	crypt_blowfish_init,
	crypt_blowfish_encode, crypt_blowfish_decode,
	NULL, NULL,
	crypt_blowfish_encode, crypt_blowfish_decode,
    },

    /* NOTE: when adding a new method, use some random bytes for the magic key,
     * to avoid that a text file is recognized as encrypted. */
};

#define CRYPT_MAGIC_LEN	12	/* cannot change */
static char	crypt_magic_head[] = "VimCrypt~";

/*
 * Return int value for crypt method name.
 * 0 for "zip", the old method.  Also for any non-valid value.
 * 1 for "blowfish".
 * 2 for "blowfish2".
 */
    int
crypt_method_nr_from_name(char_u *name)
{
    int i;

    for (i = 0; i < CRYPT_M_COUNT; ++i)
	if (STRCMP(name, cryptmethods[i].name) == 0)
	    return i;
    return 0;
}

/*
 * Get the crypt method used for a file from "ptr[len]", the magic text at the
 * start of the file.
 * Returns -1 when no encryption used.
 */
    int
crypt_method_nr_from_magic(char *ptr, int len)
{
    int i;

    if (len < CRYPT_MAGIC_LEN)
	return -1;

    for (i = 0; i < CRYPT_M_COUNT; i++)
	if (memcmp(ptr, cryptmethods[i].magic, CRYPT_MAGIC_LEN) == 0)
	    return i;

    i = (int)STRLEN(crypt_magic_head);
    if (len >= i && memcmp(ptr, crypt_magic_head, i) == 0)
	emsg(_("E821: File is encrypted with unknown method"));

    return -1;
}

#ifdef CRYPT_NOT_INPLACE
/*
 * Return TRUE if the crypt method for "method_nr" can be done in-place.
 */
    int
crypt_works_inplace(cryptstate_T *state)
{
    return cryptmethods[state->method_nr].works_inplace;
}
#endif

/*
 * Get the crypt method for buffer "buf" as a number.
 */
    int
crypt_get_method_nr(buf_T *buf)
{
    return crypt_method_nr_from_name(*buf->b_p_cm == NUL ? p_cm : buf->b_p_cm);
}

/*
 * Return TRUE when the buffer uses an encryption method that encrypts the
 * whole undo file, not only the text.
 */
    int
crypt_whole_undofile(int method_nr)
{
    return cryptmethods[method_nr].whole_undofile;
}

/*
 * Get crypt method specifc length of the file header in bytes.
 */
    int
crypt_get_header_len(int method_nr)
{
    return CRYPT_MAGIC_LEN
	+ cryptmethods[method_nr].salt_len
	+ cryptmethods[method_nr].seed_len;
}

/*
 * Set the crypt method for buffer "buf" to "method_nr" using the int value as
 * returned by crypt_method_nr_from_name().
 */
    void
crypt_set_cm_option(buf_T *buf, int method_nr)
{
    free_string_option(buf->b_p_cm);
    buf->b_p_cm = vim_strsave((char_u *)cryptmethods[method_nr].name);
}

/*
 * If the crypt method for the current buffer has a self-test, run it and
 * return OK/FAIL.
 */
    int
crypt_self_test(void)
{
    int method_nr = crypt_get_method_nr(curbuf);

    if (cryptmethods[method_nr].self_test_fn == NULL)
	return OK;
    return cryptmethods[method_nr].self_test_fn();
}

/*
 * Allocate a crypt state and initialize it.
 * Return NULL for failure.
 */
    cryptstate_T *
crypt_create(
    int		method_nr,
    char_u	*key,
    char_u	*salt,
    int		salt_len,
    char_u	*seed,
    int		seed_len)
{
    cryptstate_T *state = (cryptstate_T *)alloc((int)sizeof(cryptstate_T));

    if (state == NULL)
	return state;

    state->method_nr = method_nr;
    if (cryptmethods[method_nr].init_fn(
			   state, key, salt, salt_len, seed, seed_len) == FAIL)
    {
        vim_free(state);
        return NULL;
    }
    return state;
}

/*
 * Allocate a crypt state from a file header and initialize it.
 * Assumes that header contains at least the number of bytes that
 * crypt_get_header_len() returns for "method_nr".
 */
    cryptstate_T *
crypt_create_from_header(
    int		method_nr,
    char_u	*key,
    char_u	*header)
{
    char_u	*salt = NULL;
    char_u	*seed = NULL;
    int		salt_len = cryptmethods[method_nr].salt_len;
    int		seed_len = cryptmethods[method_nr].seed_len;

    if (salt_len > 0)
	salt = header + CRYPT_MAGIC_LEN;
    if (seed_len > 0)
	seed = header + CRYPT_MAGIC_LEN + salt_len;

    return crypt_create(method_nr, key, salt, salt_len, seed, seed_len);
}

/*
 * Read the crypt method specific header data from "fp".
 * Return an allocated cryptstate_T or NULL on error.
 */
    cryptstate_T *
crypt_create_from_file(FILE *fp, char_u *key)
{
    int		method_nr;
    int		header_len;
    char	magic_buffer[CRYPT_MAGIC_LEN];
    char_u	*buffer;
    cryptstate_T *state;

    if (fread(magic_buffer, CRYPT_MAGIC_LEN, 1, fp) != 1)
	return NULL;
    method_nr = crypt_method_nr_from_magic(magic_buffer, CRYPT_MAGIC_LEN);
    if (method_nr < 0)
	return NULL;

    header_len = crypt_get_header_len(method_nr);
    if ((buffer = alloc(header_len)) == NULL)
	return NULL;
    mch_memmove(buffer, magic_buffer, CRYPT_MAGIC_LEN);
    if (header_len > CRYPT_MAGIC_LEN
	    && fread(buffer + CRYPT_MAGIC_LEN,
				    header_len - CRYPT_MAGIC_LEN, 1, fp) != 1)
    {
	vim_free(buffer);
	return NULL;
    }

    state = crypt_create_from_header(method_nr, key, buffer);
    vim_free(buffer);
    return state;
}

/*
 * Allocate a cryptstate_T for writing and initialize it with "key".
 * Allocates and fills in the header and stores it in "header", setting
 * "header_len".  The header may include salt and seed, depending on
 * cryptmethod.  Caller must free header.
 * Returns the state or NULL on failure.
 */
    cryptstate_T *
crypt_create_for_writing(
    int	    method_nr,
    char_u  *key,
    char_u  **header,
    int	    *header_len)
{
    int	    len = crypt_get_header_len(method_nr);
    char_u  *salt = NULL;
    char_u  *seed = NULL;
    int	    salt_len = cryptmethods[method_nr].salt_len;
    int	    seed_len = cryptmethods[method_nr].seed_len;
    cryptstate_T *state;

    *header_len = len;
    *header = alloc(len);
    if (*header == NULL)
	return NULL;

    mch_memmove(*header, cryptmethods[method_nr].magic, CRYPT_MAGIC_LEN);
    if (salt_len > 0 || seed_len > 0)
    {
	if (salt_len > 0)
	    salt = *header + CRYPT_MAGIC_LEN;
	if (seed_len > 0)
	    seed = *header + CRYPT_MAGIC_LEN + salt_len;

	/* TODO: Should this be crypt method specific? (Probably not worth
	 * it).  sha2_seed is pretty bad for large amounts of entropy, so make
	 * that into something which is suitable for anything. */
	sha2_seed(salt, salt_len, seed, seed_len);
    }

    state = crypt_create(method_nr, key, salt, salt_len, seed, seed_len);
    if (state == NULL)
	VIM_CLEAR(*header);
    return state;
}

/*
 * Free the crypt state.
 */
    void
crypt_free_state(cryptstate_T *state)
{
    vim_free(state->method_state);
    vim_free(state);
}

#ifdef CRYPT_NOT_INPLACE
/*
 * Encode "from[len]" and store the result in a newly allocated buffer, which
 * is stored in "newptr".
 * Return number of bytes in "newptr", 0 for need more or -1 on error.
 */
    long
crypt_encode_alloc(
    cryptstate_T *state,
    char_u	*from,
    size_t	len,
    char_u	**newptr)
{
    cryptmethod_T *method = &cryptmethods[state->method_nr];

    if (method->encode_buffer_fn != NULL)
	/* Has buffer function, pass through. */
	return method->encode_buffer_fn(state, from, len, newptr);
    if (len == 0)
	/* Not buffering, just return EOF. */
	return (long)len;

    *newptr = alloc((long)len);
    if (*newptr == NULL)
	return -1;
    method->encode_fn(state, from, len, *newptr);
    return (long)len;
}

/*
 * Decrypt "ptr[len]" and store the result in a newly allocated buffer, which
 * is stored in "newptr".
 * Return number of bytes in "newptr", 0 for need more or -1 on error.
 */
    long
crypt_decode_alloc(
    cryptstate_T *state,
    char_u	*ptr,
    long	len,
    char_u      **newptr)
{
    cryptmethod_T *method = &cryptmethods[state->method_nr];

    if (method->decode_buffer_fn != NULL)
	/* Has buffer function, pass through. */
	return method->decode_buffer_fn(state, ptr, len, newptr);

    if (len == 0)
	/* Not buffering, just return EOF. */
	return len;

    *newptr = alloc(len);
    if (*newptr == NULL)
	return -1;
    method->decode_fn(state, ptr, len, *newptr);
    return len;
}
#endif

/*
 * Encrypting "from[len]" into "to[len]".
 */
    void
crypt_encode(
    cryptstate_T *state,
    char_u	*from,
    size_t	len,
    char_u	*to)
{
    cryptmethods[state->method_nr].encode_fn(state, from, len, to);
}

#if 0  // unused
/*
 * decrypting "from[len]" into "to[len]".
 */
    void
crypt_decode(
    cryptstate_T *state,
    char_u	*from,
    size_t	len,
    char_u	*to)
{
    cryptmethods[state->method_nr].decode_fn(state, from, len, to);
}
#endif

/*
 * Simple inplace encryption, modifies "buf[len]" in place.
 */
    void
crypt_encode_inplace(
    cryptstate_T *state,
    char_u	*buf,
    size_t	len)
{
    cryptmethods[state->method_nr].encode_inplace_fn(state, buf, len, buf);
}

/*
 * Simple inplace decryption, modifies "buf[len]" in place.
 */
    void
crypt_decode_inplace(
    cryptstate_T *state,
    char_u	*buf,
    size_t	len)
{
    cryptmethods[state->method_nr].decode_inplace_fn(state, buf, len, buf);
}

/*
 * Free an allocated crypt key.  Clear the text to make sure it doesn't stay
 * in memory anywhere.
 */
    void
crypt_free_key(char_u *key)
{
    char_u *p;

    if (key != NULL)
    {
	for (p = key; *p != NUL; ++p)
	    *p = 0;
	vim_free(key);
    }
}

/*
 * Check the crypt method and give a warning if it's outdated.
 */
    void
crypt_check_method(int method)
{
    if (method < CRYPT_M_BF2)
    {
	msg_scroll = TRUE;
	msg(_("Warning: Using a weak encryption method; see :help 'cm'"));
    }
}

    void
crypt_check_current_method(void)
{
    crypt_check_method(crypt_get_method_nr(curbuf));
}

/*
 * Ask the user for a crypt key.
 * When "store" is TRUE, the new key is stored in the 'key' option, and the
 * 'key' option value is returned: Don't free it.
 * When "store" is FALSE, the typed key is returned in allocated memory.
 * Returns NULL on failure.
 */
    char_u *
crypt_get_key(
    int		store,
    int		twice)	    /* Ask for the key twice. */
{
    char_u	*p1, *p2 = NULL;
    int		round;

    for (round = 0; ; ++round)
    {
	cmdline_star = TRUE;
	cmdline_row = msg_row;
	p1 = getcmdline_prompt(NUL, round == 0
		? (char_u *)_("Enter encryption key: ")
		: (char_u *)_("Enter same key again: "), 0, EXPAND_NOTHING,
		NULL);
	cmdline_star = FALSE;

	if (p1 == NULL)
	    break;

	if (round == twice)
	{
	    if (p2 != NULL && STRCMP(p1, p2) != 0)
	    {
		msg(_("Keys don't match!"));
		crypt_free_key(p1);
		crypt_free_key(p2);
		p2 = NULL;
		round = -1;		/* do it again */
		continue;
	    }

	    if (store)
	    {
		set_option_value((char_u *)"key", 0L, p1, OPT_LOCAL);
		crypt_free_key(p1);
		p1 = curbuf->b_p_key;
	    }
	    break;
	}
	p2 = p1;
    }

    /* since the user typed this, no need to wait for return */
    if (msg_didout)
	msg_putchar('\n');
    need_wait_return = FALSE;
    msg_didout = FALSE;

    crypt_free_key(p2);
    return p1;
}


/*
 * Append a message to IObuff for the encryption/decryption method being used.
 */
    void
crypt_append_msg(
    buf_T *buf)
{
    if (crypt_get_method_nr(buf) == 0)
	STRCAT(IObuff, _("[crypted]"));
    else
    {
	STRCAT(IObuff, "[");
	STRCAT(IObuff, *buf->b_p_cm == NUL ? p_cm : buf->b_p_cm);
	STRCAT(IObuff, "]");
    }
}

#endif /* FEAT_CRYPT */
