/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9class.c: Vim9 script class support
 */

#define USING_FLOAT_STUFF
#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

// When not generating protos this is included in proto.h
#ifdef PROTO
# include "vim9.h"
#endif

/*
 * Handle ":class" and ":abstract class" up to ":endclass".
 */
    void
ex_class(exarg_T *eap)
{
    if (!current_script_is_vim9()
		|| (cmdmod.cmod_flags & CMOD_LEGACY)
		|| !getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	emsg(_(e_class_can_only_be_defined_in_vim9_script));
	return;
    }

    char_u *arg = eap->arg;
    int is_abstract = eap->cmdidx == CMD_abstract;
    if (is_abstract)
    {
	if (STRNCMP(arg, "class", 5) != 0 || !VIM_ISWHITE(arg[5]))
	{
	    semsg(_(e_invalid_argument_str), arg);
	    return;
	}
	arg = skipwhite(arg + 5);
    }

    if (!ASCII_ISUPPER(*arg))
    {
	semsg(_(e_class_name_must_start_with_uppercase_letter_str), arg);
	return;
    }
    char_u *name_end = find_name_end(arg, NULL, NULL, FNE_CHECK_START);
    if (!IS_WHITE_OR_NUL(*name_end))
    {
	semsg(_(e_white_space_required_after_class_name_str), arg);
	return;
    }

    // TODO:
    //    generics: <Tkey, Tentry>
    //    extends SomeClass
    //    implements SomeInterface
    //    specifies SomeInterface
    //    check nothing follows

    // TODO: handle "is_export" if it is set

    garray_T	type_list;	    // list of pointers to allocated types
    ga_init2(&type_list, sizeof(type_T *), 10);

    // Growarray with object members declared in the class.
    garray_T objmembers;
    ga_init2(&objmembers, sizeof(objmember_T), 10);

    // Growarray with object methods declared in the class.
    garray_T objmethods;
    ga_init2(&objmethods, sizeof(ufunc_T), 10);

    /*
     * Go over the body of the class until "endclass" is found.
     */
    char_u *theline = NULL;
    int success = FALSE;
    for (;;)
    {
	vim_free(theline);
	theline = eap->getline(':', eap->cookie, 0, GETLINE_CONCAT_ALL);
	if (theline == NULL)
	    break;
	char_u *line = skipwhite(theline);

	// TODO:
	// class members (public, read access, private):
	//	  static varname
	//	  public static varname
	//	  static _varname
	//
	// constructors:
	//	  def new()
	//	  enddef
	//	  def newOther()
	//	  enddef
	//
	// methods (object, class, generics):
	//	  def someMethod()
	//	  enddef
	//	  static def someMethod()
	//	  enddef
	//	  def <Tval> someMethod()
	//	  enddef
	//	  static def <Tval> someMethod()
	//	  enddef

	char_u *p = line;
	if (checkforcmd(&p, "endclass", 4))
	{
	    if (STRNCMP(line, "endclass", 8) != 0)
		semsg(_(e_command_cannot_be_shortened_str), line);
	    else if (*p == '|' || !ends_excmd2(line, p))
		semsg(_(e_trailing_characters_str), p);
	    else
		success = TRUE;
	    break;
	}

	// "this.varname"
	// "this._varname"
	// TODO:
	//	"public this.varname"
	if (STRNCMP(line, "this", 4) == 0)
	{
	    if (line[4] != '.' || !eval_isnamec1(line[5]))
	    {
		semsg(_(e_invalid_object_member_declaration_str), line);
		break;
	    }
	    char_u *varname = line + 5;
	    char_u *varname_end = to_name_end(varname, FALSE);

	    char_u *colon = skipwhite(varname_end);
	    // TODO: accept initialization and figure out type from it
	    if (*colon != ':')
	    {
		emsg(_(e_type_or_initialization_required));
		break;
	    }
	    if (VIM_ISWHITE(*varname_end))
	    {
		semsg(_(e_no_white_space_allowed_before_colon_str), varname);
		break;
	    }
	    if (!VIM_ISWHITE(colon[1]))
	    {
		semsg(_(e_white_space_required_after_str_str), ":", varname);
		break;
	    }

	    char_u *type_arg = skipwhite(colon + 1);
	    type_T *type = parse_type(&type_arg, &type_list, TRUE);
	    if (type == NULL)
		break;

	    if (ga_grow(&objmembers, 1) == FAIL)
		break;
	    objmember_T *m = ((objmember_T *)objmembers.ga_data)
							  + objmembers.ga_len;
	    m->om_name = vim_strnsave(varname, varname_end - varname);
	    m->om_type = type;
	    ++objmembers.ga_len;
	}

	else
	{
	    semsg(_(e_not_valid_command_in_class_str), line);
	    break;
	}
    }
    vim_free(theline);

    if (success)
    {
	class_T *cl = ALLOC_CLEAR_ONE(class_T);
	if (cl == NULL)
	    goto cleanup;
	cl->class_refcount = 1;
	cl->class_name = vim_strnsave(arg, name_end - arg);

	// Members are used by the new() function, add them here.
	cl->class_obj_member_count = objmembers.ga_len;
	cl->class_obj_members = objmembers.ga_len == 0 ? NULL
				  : ALLOC_MULT(objmember_T, objmembers.ga_len);
	if (cl->class_name == NULL
		|| (objmembers.ga_len > 0 && cl->class_obj_members == NULL))
	{
	    vim_free(cl->class_name);
	    vim_free(cl->class_obj_members);
	    vim_free(cl);
	    goto cleanup;
	}
	mch_memmove(cl->class_obj_members, objmembers.ga_data,
				      sizeof(objmember_T) * objmembers.ga_len);
	vim_free(objmembers.ga_data);

	int have_new = FALSE;
	for (int i = 0; i < objmethods.ga_len; ++i)
	    if (STRCMP((((ufunc_T *)objmethods.ga_data) + i)->uf_name,
								   "new") == 0)
	    {
		have_new = TRUE;
		break;
	    }
	if (!have_new)
	{
	    // No new() method was defined, add the default constructor.
	    garray_T fga;
	    ga_init2(&fga, 1, 1000);
	    ga_concat(&fga, (char_u *)"new(");
	    for (int i = 0; i < cl->class_obj_member_count; ++i)
	    {
		if (i > 0)
		    ga_concat(&fga, (char_u *)", ");
		ga_concat(&fga, (char_u *)"this.");
		objmember_T *m = cl->class_obj_members + i;
		ga_concat(&fga, (char_u *)m->om_name);
	    }
	    ga_concat(&fga, (char_u *)")\nenddef\n");
	    ga_append(&fga, NUL);

	    exarg_T fea;
	    CLEAR_FIELD(fea);
	    fea.cmdidx = CMD_def;
	    fea.cmd = fea.arg = fga.ga_data;

	    garray_T lines_to_free;
	    ga_init2(&lines_to_free, sizeof(char_u *), 50);

	    ufunc_T *nf = define_function(&fea, NULL, &lines_to_free, cl);

	    ga_clear_strings(&lines_to_free);
	    vim_free(fga.ga_data);

	    if (nf != NULL && ga_grow(&objmethods, 1) == OK)
	    {
		((ufunc_T **)objmethods.ga_data)[objmethods.ga_len] = nf;
		++objmethods.ga_len;

		nf->uf_flags |= FC_NEW;
		nf->uf_class = cl;
		nf->uf_ret_type = get_type_ptr(&type_list);
		if (nf->uf_ret_type != NULL)
		{
		    nf->uf_ret_type->tt_type = VAR_OBJECT;
		    nf->uf_ret_type->tt_member = (type_T *)cl;
		    nf->uf_ret_type->tt_argcount = 0;
		    nf->uf_ret_type->tt_args = NULL;
		}
		cl->class_new_func = nf;
	    }
	}

	cl->class_obj_method_count = objmethods.ga_len;
	cl->class_obj_methods = ALLOC_MULT(ufunc_T *, objmethods.ga_len);
	if (cl->class_obj_methods == NULL)
	{
	    vim_free(cl->class_name);
	    vim_free(cl->class_obj_members);
	    vim_free(cl->class_obj_methods);
	    vim_free(cl);
	    goto cleanup;
	}
	mch_memmove(cl->class_obj_methods, objmethods.ga_data,
					sizeof(ufunc_T *) * objmethods.ga_len);
	vim_free(objmethods.ga_data);

	cl->class_type.tt_type = VAR_CLASS;
	cl->class_type.tt_member = (type_T *)cl;
	cl->class_type_list = type_list;

	// TODO:
	// - Add the methods to the class
	//	- array with ufunc_T pointers
	// - Fill hashtab with object members and methods
	// - Generate the default new() method, if needed.
	// Later:
	// - class members
	// - class methods

	// Add the class to the script-local variables.
	typval_T tv;
	tv.v_type = VAR_CLASS;
	tv.vval.v_class = cl;
	set_var_const(cl->class_name, current_sctx.sc_sid,
					     NULL, &tv, FALSE, ASSIGN_DECL, 0);
	return;
    }

cleanup:
    for (int i = 0; i < objmembers.ga_len; ++i)
    {
	objmember_T *m = ((objmember_T *)objmembers.ga_data) + i;
	vim_free(m->om_name);
    }
    ga_clear(&objmembers);

    ga_clear(&objmethods);
    clear_type_list(&type_list);
}

/*
 * Find member "name" in class "cl" and return its type.
 * When not found t_any is returned.
 */
    type_T *
class_member_type(
	class_T *cl,
	char_u	*name,
	char_u	*name_end,
	int	*member_idx)
{
    *member_idx = -1;  // not found (yet)
    size_t len = name_end - name;

    for (int i = 0; i < cl->class_obj_member_count; ++i)
    {
	objmember_T *m = cl->class_obj_members + i;
	if (STRNCMP(m->om_name, name, len) == 0 && m->om_name[len] == NUL)
	{
	    *member_idx = i;
	    return m->om_type;
	}
    }
    return &t_any;
}

/*
 * Handle ":interface" up to ":endinterface".
 */
    void
ex_interface(exarg_T *eap UNUSED)
{
    // TODO
}

/*
 * Handle ":enum" up to ":endenum".
 */
    void
ex_enum(exarg_T *eap UNUSED)
{
    // TODO
}

/*
 * Handle ":type".
 */
    void
ex_type(exarg_T *eap UNUSED)
{
    // TODO
}

/*
 * Evaluate what comes after a class:
 * - class member: SomeClass.varname
 * - class method: SomeClass.SomeMethod()
 * - class constructor: SomeClass.new()
 * - object member: someObject.varname
 * - object method: someObject.SomeMethod()
 *
 * "*arg" points to the '.'.
 * "*arg" is advanced to after the member name or method call.
 *
 * Returns FAIL or OK.
 */
    int
class_object_index(
    char_u	**arg,
    typval_T	*rettv,
    evalarg_T	*evalarg,
    int		verbose UNUSED)	// give error messages
{
    // int		evaluate = evalarg != NULL
    //				      && (evalarg->eval_flags & EVAL_EVALUATE);

    if (VIM_ISWHITE((*arg)[1]))
    {
	semsg(_(e_no_white_space_allowed_after_str_str), ".", *arg);
	return FAIL;
    }

    ++*arg;
    char_u *name = *arg;
    char_u *name_end = find_name_end(name, NULL, NULL, FNE_CHECK_START);
    if (name_end == name)
	return FAIL;
    size_t len = name_end - name;

    class_T *cl = rettv->v_type == VAR_CLASS ? rettv->vval.v_class
					     : rettv->vval.v_object->obj_class;
    if (*name_end == '(')
    {
	for (int i = 0; i < cl->class_obj_method_count; ++i)
	{
	    ufunc_T *fp = cl->class_obj_methods[i];
	    if (STRNCMP(name, fp->uf_name, len) == 0 && fp->uf_name[len] == NUL)
	    {
		typval_T    argvars[MAX_FUNC_ARGS + 1];
		int	    argcount = 0;

		char_u *argp = name_end;
		int ret = get_func_arguments(&argp, evalarg, 0,
							   argvars, &argcount);
		if (ret == FAIL)
		    return FAIL;

		funcexe_T   funcexe;
		CLEAR_FIELD(funcexe);
		funcexe.fe_evaluate = TRUE;

		// Clear the class or object after calling the function, in
		// case the refcount is one.
		typval_T tv_tofree = *rettv;
		rettv->v_type = VAR_UNKNOWN;

		// Call the user function.  Result goes into rettv;
		// TODO: pass the object
		int error = call_user_func_check(fp, argcount, argvars,
							rettv, &funcexe, NULL);

		// Clear the previous rettv and the arguments.
		clear_tv(&tv_tofree);
		for (int idx = 0; idx < argcount; ++idx)
		    clear_tv(&argvars[idx]);

		if (error != FCERR_NONE)
		{
		    user_func_error(error, printable_func_name(fp),
							 funcexe.fe_found_var);
		    return FAIL;
		}
		*arg = argp;
		return OK;
	    }
	}

	semsg(_(e_method_not_found_on_class_str_str), cl->class_name, name);
    }

    else if (rettv->v_type == VAR_OBJECT)
    {
	for (int i = 0; i < cl->class_obj_member_count; ++i)
	{
	    objmember_T *m = &cl->class_obj_members[i];
	    if (STRNCMP(name, m->om_name, len) == 0 && m->om_name[len] == NUL)
	    {
		// The object only contains a pointer to the class, the member
		// values array follows right after that.
		object_T *obj = rettv->vval.v_object;
		typval_T *tv = (typval_T *)(obj + 1) + i;
		copy_tv(tv, rettv);
		object_unref(obj);

		*arg = name_end;
		return OK;
	    }
	}

	semsg(_(e_member_not_found_on_object_str_str), cl->class_name, name);
    }

    // TODO: class member

    return FAIL;
}

/*
 * Make a copy of an object.
 */
    void
copy_object(typval_T *from, typval_T *to)
{
    *to = *from;
    if (to->vval.v_object != NULL)
	++to->vval.v_object->obj_refcount;
}

/*
 * Free an object.
 */
    static void
object_clear(object_T *obj)
{
    class_T *cl = obj->obj_class;

    // the member values are just after the object structure
    typval_T *tv = (typval_T *)(obj + 1);
    for (int i = 0; i < cl->class_obj_member_count; ++i)
	clear_tv(tv + i);

    // Remove from the list headed by "first_object".
    object_cleared(obj);

    vim_free(obj);
    class_unref(cl);
}

/*
 * Unreference an object.
 */
    void
object_unref(object_T *obj)
{
    if (obj != NULL && --obj->obj_refcount <= 0)
	object_clear(obj);
}

/*
 * Make a copy of a class.
 */
    void
copy_class(typval_T *from, typval_T *to)
{
    *to = *from;
    if (to->vval.v_class != NULL)
	++to->vval.v_class->class_refcount;
}

/*
 * Unreference a class.  Free it when the reference count goes down to zero.
 */
    void
class_unref(class_T *cl)
{
    if (cl != NULL && --cl->class_refcount <= 0)
    {
	vim_free(cl->class_name);

	for (int i = 0; i < cl->class_obj_member_count; ++i)
	{
	    objmember_T *m = &cl->class_obj_members[i];
	    vim_free(m->om_name);
	}
	vim_free(cl->class_obj_members);

	vim_free(cl->class_obj_methods);

	if (cl->class_new_func != NULL)
	    func_ptr_unref(cl->class_new_func);

	clear_type_list(&cl->class_type_list);

	vim_free(cl);
    }
}

static object_T *first_object = NULL;

/*
 * Call this function when an object has been created.  It will be added to the
 * list headed by "first_object".
 */
    void
object_created(object_T *obj)
{
    if (first_object != NULL)
    {
	obj->obj_next_used = first_object;
	first_object->obj_prev_used = obj;
    }
    first_object = obj;
}

/*
 * Call this function when an object has been cleared and is about to be freed.
 * It is removed from the list headed by "first_object".
 */
    void
object_cleared(object_T *obj)
{
    if (obj->obj_next_used != NULL)
	obj->obj_next_used->obj_prev_used = obj->obj_prev_used;
    if (obj->obj_prev_used != NULL)
	obj->obj_prev_used->obj_next_used = obj->obj_next_used;
    else if (first_object == obj)
	first_object = obj->obj_next_used;
}

/*
 * Go through the list of all objects and free items without "copyID".
 */
    int
object_free_nonref(int copyID)
{
    int		did_free = FALSE;
    object_T	*next_obj;

    for (object_T *obj = first_object; obj != NULL; obj = next_obj)
    {
	next_obj = obj->obj_next_used;
	if ((obj->obj_copyID & COPYID_MASK) != (copyID & COPYID_MASK))
	{
	    // Free the object and items it contains.
	    object_clear(obj);
	    did_free = TRUE;
	}
    }

    return did_free;
}


#endif // FEAT_EVAL
