/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * opaque.c: Opaque data type functions
 */

#include "vim.h"

#if defined(FEAT_EVAL)

/*
 * Allocate new opaque data type, Returns NULL on failure. Caller must manage
 * reference count, it is set to zero. "data" may be NULL.
 */
    opaque_T *
opaque_new(char_u *type, void *data, size_t data_sz)
{
    opaque_T	*op = alloc_clear(offsetof(opaque_T, op_data) + data_sz);

    if (op == NULL)
	return NULL;

    op->op_type = vim_strsave(type);

    if (op->op_type == NULL)
    {
	vim_free(op);
	return NULL;
    }

    if (data != NULL)
	memcpy(op->op_data, data, data_sz);

    return op;
}

    static void
opaque_free(opaque_T *op)
{
    if (op->op_free_func != NULL)
	op->op_free_func(op);
    vim_free(op->op_type);
    vim_free(op);
}

    void
opaque_unref(opaque_T *op)
{
    if (op != NULL && --op->op_refcount <= 0)
	opaque_free(op);
}

#endif // FEAT_EVAL
