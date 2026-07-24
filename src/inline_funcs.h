/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * inline_funcs.h: shared inline function definitions.
 *
 * Included from vim.h after proto.h, so they may use its prototypes.
 */

/*
 * Copy the value of typval "from" to typval "to".  See copy_tv_inner().
 */
    static inline int
copy_tv(typval_T *from, typval_T *to)
{
    switch (from->v_type)
    {
	case VAR_NUMBER:
	case VAR_BOOL:
	case VAR_SPECIAL:
	case VAR_FLOAT:
	    to->v_type = from->v_type;
	    to->v_lock = 0;
	    to->vval = from->vval;	// union copy covers number and float
	    return OK;
	default:
	    return copy_tv_inner(from, to);
    }
}

/*
 * Free the memory for the value of typval "varp" and set it to NULL or 0.
 * See clear_tv_inner().
 */
    static inline void
clear_tv(typval_T *varp)
{
    if (varp == NULL)
	return;
    switch (varp->v_type)
    {
	case VAR_NUMBER:
	case VAR_BOOL:
	case VAR_SPECIAL:
	    varp->vval.v_number = 0;
	    varp->v_lock = 0;
	    break;
	case VAR_FLOAT:
	    varp->vval.v_float = 0.0;
	    varp->v_lock = 0;
	    break;
	default:
	    clear_tv_inner(varp);
    }
}
