/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * opaque.c: Opaque data type functions.
 *
 * How to add a new opaque type:
 *
 * 1. Define a global opaque_type_T variable and make it extern in globals.h
 * 2. Update lookup_opaque_type() below as needed using the global type.
 */

#include "vim.h"

#if defined(FEAT_EVAL)

/*
 * Lookup the given opaque type string and return the struct if it exists, else
 * NULL/
 */
    opaque_type_T *
lookup_opaque_type(char_u *name, size_t namelen)
{
    opaque_type_T *type;

    if (STRNCMP(name, "TS", 2) == 0
	    && (type = tsvim_lookup_opaque_type(name, namelen)) != NULL)
	return type;
    if (STRCMP(name, "TestOpaque") == 0)
	return &test_opaque_type;
    return NULL;
}

/*
 * Allocate new opaque data type, Returns NULL on failure. Caller must manage
 * reference count, it is set to zero. "data" may be NULL.
 */
    opaque_T *
opaque_new(opaque_type_T *type, void *data, size_t data_sz)
{
    opaque_T	*op = alloc_clear(offsetof(opaque_T, op_data) + data_sz);

    if (op == NULL)
	return NULL;

    op->op_type = type;

    if (data != NULL)
	memcpy(op->op_data, data, data_sz);

    return op;
}

    static void
opaque_free(opaque_T *op)
{
    if (op->op_type->ot_free_func != NULL)
	op->op_type->ot_free_func(op);

    vim_free(op);
}

    void
opaque_unref(opaque_T *op)
{
    if (op != NULL && --op->op_refcount <= 0)
	opaque_free(op);
}

    bool
opaque_equal_ptr(opaque_T *a, opaque_T *b)
{
    return OP2DATA(a, void *) == OP2DATA(b, void *);
}

/*
 * Compare two opaque_property_T structs by case sensitive value.
 */
    static int
cmp_opaqueproperty_value(const void *a, const void *b)
{
    opaque_property_T *opp1 = (opaque_property_T *)a;
    opaque_property_T *opp2 = (opaque_property_T *)b;

    return STRNCMP(opp1->opp_name, opp2->opp_name, MAX(opp1->opp_name_len,
		opp2->opp_name_len));
}

/*
 * Lookup a property in an opaque type and return the pointer to it. "idx" is
 * set to the index of it in the properties array. If not exists, then return
 * NULL.
 */
    opaque_property_T *
lookup_opaque_property(opaque_type_T *ot, char_u *name, size_t namelen, int *idx)
{
    // Since properties array is sorted, we can use binary search
    opaque_property_T target;
    opaque_property_T *prop;

    target.opp_name = name;
    target.opp_type = NULL;
    target.opp_name_len = 0; // Not used
    
    prop = bsearch(&target, ot->ot_properties, ot->ot_property_count,
	    sizeof(ot->ot_properties[0]), cmp_opaqueproperty_value);

    if (prop == NULL)
	return NULL;

    *idx = prop->opp_idx;

    return prop;
}

/*
 * Returns OK if property called "name" is valid for the given opaque object. If
 * it is valid, the propety typal_T is set in "rettv". Returns FAIL on failure.
 */
    static int
get_opaque_property_tv(opaque_T *op, char_u *name, size_t namelen, typval_T *rettv)
{
    opaque_property_T	*prop;
    int			prop_idx;
    int			ret;
    typval_T		tv;

    prop = lookup_opaque_property(op->op_type, name, namelen, &prop_idx);

    if (prop == NULL)
	return FAIL;

    // Call getter function
    ret = op->op_type->ot_property_func(op, prop, &tv);

    if (ret == FAIL)
	return FAIL;

    *rettv = tv;
    opaque_unref(op);

    return OK;
}


/*
 * Evalute the property after an opaque: Opaque.property
 * Only supports reading from a property.
 *
 * "*arg" points to the '.'
 * "*arg" is advanced to after the property name.
 *
 * Returns FAIL on failure
 */
   int
opaque_property_index(char_u **arg, typval_T *rettv)
{
    opaque_T *op;
    char_u  *name;
    char_u  *name_end;
    size_t  len;

    if (VIM_ISWHITE((*arg)[1]))
    {
	semsg(_(e_no_white_space_allowed_after_str_str), ".", *arg);
	return FAIL;
    }

    ++*arg;
    name = *arg;
    name_end = find_name_end(name, NULL, NULL, FNE_CHECK_START);

    op = rettv->vval.v_opaque;

    if (op == NULL)
    {
	emsg(_(e_incomplete_type));
	return FAIL;
    }

    if (name_end == name)
	return FAIL;

    len = name_end - name;

    if (get_opaque_property_tv(op, name, len, rettv) == OK)
    {
	*arg = name_end;
	return OK;
    }

    // Property doesn't exist
    semsg(_(e_opaque_str_property_str_no_exist), name, op->op_type->ot_type);

    return FAIL;
}

#endif // FEAT_EVAL
