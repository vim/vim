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
 * Parse a member declaration, both object and class member.
 * Returns OK or FAIL.  When OK then "varname_end" is set to just after the
 * variable name and "type_ret" is set to the decleared or detected type.
 * "init_expr" is set to the initialisation expression (allocated), if there is
 * one.  For an interface "init_expr" is NULL.
 */
    static int
parse_member(
	exarg_T	*eap,
	char_u	*line,
	char_u	*varname,
	int	has_public,	    // TRUE if "public" seen before "varname"
	char_u	**varname_end,
	garray_T *type_list,
	type_T	**type_ret,
	char_u	**init_expr)
{
    *varname_end = to_name_end(varname, FALSE);
    if (*varname == '_' && has_public)
    {
	semsg(_(e_public_member_name_cannot_start_with_underscore_str), line);
	return FAIL;
    }

    char_u *colon = skipwhite(*varname_end);
    char_u *type_arg = colon;
    type_T *type = NULL;
    if (*colon == ':')
    {
	if (VIM_ISWHITE(**varname_end))
	{
	    semsg(_(e_no_white_space_allowed_before_colon_str), varname);
	    return FAIL;
	}
	if (!VIM_ISWHITE(colon[1]))
	{
	    semsg(_(e_white_space_required_after_str_str), ":", varname);
	    return FAIL;
	}
	type_arg = skipwhite(colon + 1);
	type = parse_type(&type_arg, type_list, TRUE);
	if (type == NULL)
	    return FAIL;
    }

    char_u *expr_start = skipwhite(type_arg);
    char_u *expr_end = expr_start;
    if (type == NULL && *expr_start != '=')
    {
	emsg(_(e_type_or_initialization_required));
	return FAIL;
    }

    if (*expr_start == '=')
    {
	if (!VIM_ISWHITE(expr_start[-1]) || !VIM_ISWHITE(expr_start[1]))
	{
	    semsg(_(e_white_space_required_before_and_after_str_at_str),
							"=", type_arg);
	    return FAIL;
	}
	expr_start = skipwhite(expr_start + 1);

	expr_end = expr_start;
	evalarg_T evalarg;
	fill_evalarg_from_eap(&evalarg, eap, FALSE);
	skip_expr(&expr_end, NULL);

	if (type == NULL)
	{
	    // No type specified, use the type of the initializer.
	    typval_T tv;
	    tv.v_type = VAR_UNKNOWN;
	    char_u *expr = expr_start;
	    int res = eval0(expr, &tv, eap, &evalarg);

	    if (res == OK)
	    {
		type = typval2type(&tv, get_copyID(), type_list,
						       TVTT_DO_MEMBER);
		clear_tv(&tv);
	    }
	    if (type == NULL)
	    {
		semsg(_(e_cannot_get_object_member_type_from_initializer_str),
			expr_start);
		clear_evalarg(&evalarg, NULL);
		return FAIL;
	    }
	}
	clear_evalarg(&evalarg, NULL);
    }
    if (!valid_declaration_type(type))
	return FAIL;

    *type_ret = type;
    if (expr_end > expr_start)
    {
	if (init_expr == NULL)
	{
	    emsg(_(e_cannot_initialize_member_in_interface));
	    return FAIL;
	}
	*init_expr = vim_strnsave(expr_start, expr_end - expr_start);
    }
    return OK;
}

/*
 * Add a member to an object or a class.
 * Returns OK when successful, "init_expr" will be consumed then.
 * Returns FAIL otherwise, caller might need to free "init_expr".
 */
    static int
add_member(
	garray_T    *gap,
	char_u	    *varname,
	char_u	    *varname_end,
	int	    has_public,
	type_T	    *type,
	char_u	    *init_expr)
{
    if (ga_grow(gap, 1) == FAIL)
	return FAIL;
    ocmember_T *m = ((ocmember_T *)gap->ga_data) + gap->ga_len;
    m->ocm_name = vim_strnsave(varname, varname_end - varname);
    m->ocm_access = has_public ? ACCESS_ALL
			      : *varname == '_' ? ACCESS_PRIVATE : ACCESS_READ;
    m->ocm_type = type;
    if (init_expr != NULL)
	m->ocm_init = init_expr;
    ++gap->ga_len;
    return OK;
}

/*
 * Move the class or object members found while parsing a class into the class.
 * "gap" contains the found members.
 * "members" will be set to the newly allocated array of members and
 * "member_count" set to the number of members.
 * Returns OK or FAIL.
 */
    static int
add_members_to_class(
    garray_T	*gap,
    ocmember_T	**members,
    int		*member_count)
{
    *member_count = gap->ga_len;
    *members = gap->ga_len == 0 ? NULL : ALLOC_MULT(ocmember_T, gap->ga_len);
    if (gap->ga_len > 0 && *members == NULL)
	return FAIL;
    if (gap->ga_len > 0)
	mch_memmove(*members, gap->ga_data, sizeof(ocmember_T) * gap->ga_len);
    VIM_CLEAR(gap->ga_data);
    return OK;
}

/*
 * Handle ":class" and ":abstract class" up to ":endclass".
 * Handle ":interface" up to ":endinterface".
 */
    void
ex_class(exarg_T *eap)
{
    int is_class = eap->cmdidx == CMD_class;  // FALSE for :interface

    if (!current_script_is_vim9()
		|| (cmdmod.cmod_flags & CMOD_LEGACY)
		|| !getline_equal(eap->getline, eap->cookie, getsourceline))
    {
	if (is_class)
	    emsg(_(e_class_can_only_be_defined_in_vim9_script));
	else
	    emsg(_(e_interface_can_only_be_defined_in_vim9_script));
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
	if (is_class)
	    semsg(_(e_class_name_must_start_with_uppercase_letter_str), arg);
	else
	    semsg(_(e_interface_name_must_start_with_uppercase_letter_str),
									  arg);
	return;
    }
    char_u *name_end = find_name_end(arg, NULL, NULL, FNE_CHECK_START);
    if (!IS_WHITE_OR_NUL(*name_end))
    {
	semsg(_(e_white_space_required_after_name_str), arg);
	return;
    }

    // TODO:
    //    generics: <Tkey, Tentry>
    //    extends SomeClass
    //    implements SomeInterface
    //    specifies SomeInterface
    //    check that nothing follows
    //	  handle "is_export" if it is set

    garray_T	type_list;	    // list of pointers to allocated types
    ga_init2(&type_list, sizeof(type_T *), 10);

    // Growarray with class members declared in the class.
    garray_T classmembers;
    ga_init2(&classmembers, sizeof(ocmember_T), 10);

    // Growarray with functions declared in the class.
    garray_T classfunctions;
    ga_init2(&classfunctions, sizeof(ufunc_T *), 10);

    // Growarray with object members declared in the class.
    garray_T objmembers;
    ga_init2(&objmembers, sizeof(ocmember_T), 10);

    // Growarray with object methods declared in the class.
    garray_T objmethods;
    ga_init2(&objmethods, sizeof(ufunc_T *), 10);

    /*
     * Go over the body of the class/interface until "endclass" or
     * "endinterface" is found.
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

	// Skip empty and comment lines.
	if (*line == NUL)
	    continue;
	if (*line == '#')
	{
	    if (vim9_bad_comment(line))
		break;
	    continue;
	}

	char_u *p = line;
	char *end_name = is_class ? "endclass" : "endinterface";
	if (checkforcmd(&p, end_name, is_class ? 4 : 5))
	{
	    if (STRNCMP(line, end_name, is_class ? 8 : 12) != 0)
		semsg(_(e_command_cannot_be_shortened_str), line);
	    else if (*p == '|' || !ends_excmd2(line, p))
		semsg(_(e_trailing_characters_str), p);
	    else
		success = TRUE;
	    break;
	}
	char *wrong_name = is_class ? "endinterface" : "endclass";
	if (checkforcmd(&p, wrong_name, is_class ? 5 : 4))
	{
	    semsg(_(e_invalid_command_str), line);
	    break;
	}

	int has_public = FALSE;
	if (checkforcmd(&p, "public", 3))
	{
	    if (STRNCMP(line, "public", 6) != 0)
	    {
		semsg(_(e_command_cannot_be_shortened_str), line);
		break;
	    }
	    has_public = TRUE;
	    p = skipwhite(line + 6);

	    if (STRNCMP(p, "this", 4) != 0 && STRNCMP(p, "static", 6) != 0)
	    {
		emsg(_(e_public_must_be_followed_by_this_or_static));
		break;
	    }
	}

	int has_static = FALSE;
	char_u *ps = p;
	if (checkforcmd(&p, "static", 4))
	{
	    if (STRNCMP(ps, "static", 6) != 0)
	    {
		semsg(_(e_command_cannot_be_shortened_str), ps);
		break;
	    }
	    has_static = TRUE;
	    p = skipwhite(ps + 6);
	}

	// object members (public, read access, private):
	//	"this._varname"
	//	"this.varname"
	//	"public this.varname"
	if (STRNCMP(p, "this", 4) == 0)
	{
	    if (p[4] != '.' || !eval_isnamec1(p[5]))
	    {
		semsg(_(e_invalid_object_member_declaration_str), p);
		break;
	    }
	    char_u *varname = p + 5;
	    char_u *varname_end = NULL;
	    type_T *type = NULL;
	    char_u *init_expr = NULL;
	    if (parse_member(eap, line, varname, has_public,
			  &varname_end, &type_list, &type,
			  is_class ? &init_expr: NULL) == FAIL)
		break;
	    if (add_member(&objmembers, varname, varname_end,
					  has_public, type, init_expr) == FAIL)
	    {
		vim_free(init_expr);
		break;
	    }
	}

	// constructors:
	//	  def new()
	//	  enddef
	//	  def newOther()
	//	  enddef
	// object methods and class functions:
	//	  def SomeMethod()
	//	  enddef
	//	  static def ClassFunction()
	//	  enddef
	// TODO:
	//	  def <Tval> someMethod()
	//	  enddef
	else if (checkforcmd(&p, "def", 3))
	{
	    exarg_T	ea;
	    garray_T	lines_to_free;

	    // TODO: error for "public static def Func()"?

	    CLEAR_FIELD(ea);
	    ea.cmd = line;
	    ea.arg = p;
	    ea.cmdidx = CMD_def;
	    ea.getline = eap->getline;
	    ea.cookie = eap->cookie;

	    ga_init2(&lines_to_free, sizeof(char_u *), 50);
	    ufunc_T *uf = define_function(&ea, NULL, &lines_to_free,
					   is_class ? CF_CLASS : CF_INTERFACE);
	    ga_clear_strings(&lines_to_free);

	    if (uf != NULL)
	    {
		int is_new = STRNCMP(uf->uf_name, "new", 3) == 0;
		garray_T *fgap = has_static || is_new
					       ? &classfunctions : &objmethods;
		if (ga_grow(fgap, 1) == OK)
		{
		    if (is_new)
			uf->uf_flags |= FC_NEW;

		    ((ufunc_T **)fgap->ga_data)[fgap->ga_len] = uf;
		    ++fgap->ga_len;
		}
	    }
	}

	// class members
	else if (has_static)
	{
	    // class members (public, read access, private):
	    //	"static _varname"
	    //	"static varname"
	    //	"public static varname"
	    char_u *varname = p;
	    char_u *varname_end = NULL;
	    type_T *type = NULL;
	    char_u *init_expr = NULL;
	    if (parse_member(eap, line, varname, has_public,
		      &varname_end, &type_list, &type,
		      is_class ? &init_expr : NULL) == FAIL)
		break;
	    if (add_member(&classmembers, varname, varname_end,
				      has_public, type, init_expr) == FAIL)
	    {
		vim_free(init_expr);
		break;
	    }
	}

	else
	{
	    if (is_class)
		semsg(_(e_not_valid_command_in_class_str), line);
	    else
		semsg(_(e_not_valid_command_in_interface_str), line);
	    break;
	}
    }
    vim_free(theline);

    class_T *cl = NULL;
    if (success)
    {
	// "endclass" encountered without failures: Create the class.

	cl = ALLOC_CLEAR_ONE(class_T);
	if (cl == NULL)
	    goto cleanup;
	if (!is_class)
	    cl->class_flags = CLASS_INTERFACE;

	cl->class_refcount = 1;
	cl->class_name = vim_strnsave(arg, name_end - arg);
	if (cl->class_name == NULL)
	    goto cleanup;

	// Add class and object members to "cl".
	if (add_members_to_class(&classmembers,
				    &cl->class_class_members,
				    &cl->class_class_member_count) == FAIL
		|| add_members_to_class(&objmembers,
				    &cl->class_obj_members,
				    &cl->class_obj_member_count) == FAIL)
	    goto cleanup;

	if (is_class && cl->class_class_member_count > 0)
	{
	    // Allocate a typval for each class member and initialize it.
	    cl->class_members_tv = ALLOC_CLEAR_MULT(typval_T,
						 cl->class_class_member_count);
	    if (cl->class_members_tv != NULL)
		for (int i = 0; i < cl->class_class_member_count; ++i)
		{
		    ocmember_T *m = &cl->class_class_members[i];
		    typval_T *tv = &cl->class_members_tv[i];
		    if (m->ocm_init != NULL)
		    {
			typval_T *etv = eval_expr(m->ocm_init, eap);
			if (etv != NULL)
			{
			    *tv = *etv;
			    vim_free(etv);
			}
		    }
		    else
		    {
			// TODO: proper default value
			tv->v_type = m->ocm_type->tt_type;
			tv->vval.v_string = NULL;
		    }
		}
	}

	int have_new = FALSE;
	for (int i = 0; i < classfunctions.ga_len; ++i)
	    if (STRCMP(((ufunc_T **)classfunctions.ga_data)[i]->uf_name,
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
		ocmember_T *m = cl->class_obj_members + i;
		ga_concat(&fga, (char_u *)m->ocm_name);
		ga_concat(&fga, (char_u *)" = v:none");
	    }
	    ga_concat(&fga, (char_u *)")\nenddef\n");
	    ga_append(&fga, NUL);

	    exarg_T fea;
	    CLEAR_FIELD(fea);
	    fea.cmdidx = CMD_def;
	    fea.cmd = fea.arg = fga.ga_data;

	    garray_T lines_to_free;
	    ga_init2(&lines_to_free, sizeof(char_u *), 50);

	    ufunc_T *nf = define_function(&fea, NULL, &lines_to_free,
					   is_class ? CF_CLASS : CF_INTERFACE);

	    ga_clear_strings(&lines_to_free);
	    vim_free(fga.ga_data);

	    if (nf != NULL && ga_grow(&classfunctions, 1) == OK)
	    {
		((ufunc_T **)classfunctions.ga_data)[classfunctions.ga_len] = nf;
		++classfunctions.ga_len;

		nf->uf_flags |= FC_NEW;
		nf->uf_ret_type = get_type_ptr(&type_list);
		if (nf->uf_ret_type != NULL)
		{
		    nf->uf_ret_type->tt_type = VAR_OBJECT;
		    nf->uf_ret_type->tt_member = (type_T *)cl;
		    nf->uf_ret_type->tt_argcount = 0;
		    nf->uf_ret_type->tt_args = NULL;
		}
	    }
	}

	// loop 1: class functions, loop 2: object methods
	for (int loop = 1; loop <= 2; ++loop)
	{
	    garray_T *gap = loop == 1 ? &classfunctions : &objmethods;
	    int	     *fcount = loop == 1 ? &cl->class_class_function_count
					 : &cl->class_obj_method_count;
	    ufunc_T ***fup = loop == 1 ? &cl->class_class_functions
				       : &cl->class_obj_methods;

	    *fcount = gap->ga_len;
	    if (gap->ga_len == 0)
	    {
		*fup = NULL;
		continue;
	    }
	    *fup = ALLOC_MULT(ufunc_T *, gap->ga_len);
	    if (*fup == NULL)
		goto cleanup;
	    mch_memmove(*fup, gap->ga_data, sizeof(ufunc_T *) * gap->ga_len);
	    vim_free(gap->ga_data);

	    // Set the class pointer on all the object methods.
	    for (int i = 0; i < gap->ga_len; ++i)
	    {
		ufunc_T *fp = (*fup)[i];
		fp->uf_class = cl;
		if (loop == 2)
		    fp->uf_flags |= FC_OBJECT;
	    }
	}

	cl->class_type.tt_type = VAR_CLASS;
	cl->class_type.tt_member = (type_T *)cl;
	cl->class_object_type.tt_type = VAR_OBJECT;
	cl->class_object_type.tt_member = (type_T *)cl;
	cl->class_type_list = type_list;

	// TODO:
	// - Fill hashtab with object members and methods ?

	// Add the class to the script-local variables.
	typval_T tv;
	tv.v_type = VAR_CLASS;
	tv.vval.v_class = cl;
	set_var_const(cl->class_name, current_sctx.sc_sid,
					     NULL, &tv, FALSE, ASSIGN_DECL, 0);
	return;
    }

cleanup:
    if (cl != NULL)
    {
	vim_free(cl->class_name);
	vim_free(cl->class_class_functions);
	vim_free(cl->class_obj_members);
	vim_free(cl->class_obj_methods);
	vim_free(cl);
    }

    for (int round = 1; round <= 2; ++round)
    {
	garray_T *gap = round == 1 ? &classmembers : &objmembers;
	if (gap->ga_len == 0 || gap->ga_data == NULL)
	    continue;

	for (int i = 0; i < gap->ga_len; ++i)
	{
	    ocmember_T *m = ((ocmember_T *)gap->ga_data) + i;
	    vim_free(m->ocm_name);
	    vim_free(m->ocm_init);
	}
	ga_clear(gap);
    }

    for (int i = 0; i < objmethods.ga_len; ++i)
    {
	ufunc_T *uf = ((ufunc_T **)objmethods.ga_data)[i];
	func_clear_free(uf, FALSE);
    }
    ga_clear(&objmethods);

    for (int i = 0; i < classfunctions.ga_len; ++i)
    {
	ufunc_T *uf = ((ufunc_T **)classfunctions.ga_data)[i];
	func_clear_free(uf, FALSE);
    }
    ga_clear(&classfunctions);

    clear_type_list(&type_list);
}

/*
 * Find member "name" in class "cl", set "member_idx" to the member index and
 * return its type.
 * When not found "member_idx" is set to -1 and t_any is returned.
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
	ocmember_T *m = cl->class_obj_members + i;
	if (STRNCMP(m->ocm_name, name, len) == 0 && m->ocm_name[len] == NUL)
	{
	    *member_idx = i;
	    return m->ocm_type;
	}
    }

    semsg(_(e_unknown_variable_str), name);
    return &t_any;
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
 * - class function: SomeClass.SomeMethod()
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
	int on_class = rettv->v_type == VAR_CLASS;
	int count = on_class ? cl->class_class_function_count
			     : cl->class_obj_method_count;
	for (int i = 0; i < count; ++i)
	{
	    ufunc_T *fp = on_class ? cl->class_class_functions[i]
				   : cl->class_obj_methods[i];
	    // Use a separate pointer to avoid that ASAN complains about
	    // uf_name[] only being 4 characters.
	    char_u *ufname = (char_u *)fp->uf_name;
	    if (STRNCMP(name, ufname, len) == 0 && ufname[len] == NUL)
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
		if (rettv->v_type == VAR_OBJECT)
		{
		    funcexe.fe_object = rettv->vval.v_object;
		    ++funcexe.fe_object->obj_refcount;
		}

		// Clear the class or object after calling the function, in
		// case the refcount is one.
		typval_T tv_tofree = *rettv;
		rettv->v_type = VAR_UNKNOWN;

		// Call the user function.  Result goes into rettv;
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
	    ocmember_T *m = &cl->class_obj_members[i];
	    if (STRNCMP(name, m->ocm_name, len) == 0 && m->ocm_name[len] == NUL)
	    {
		if (*name == '_')
		{
		    semsg(_(e_cannot_access_private_member_str), m->ocm_name);
		    return FAIL;
		}

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

    else if (rettv->v_type == VAR_CLASS)
    {
	// class member
	for (int i = 0; i < cl->class_class_member_count; ++i)
	{
	    ocmember_T *m = &cl->class_class_members[i];
	    if (STRNCMP(name, m->ocm_name, len) == 0 && m->ocm_name[len] == NUL)
	    {
		if (*name == '_')
		{
		    semsg(_(e_cannot_access_private_member_str), m->ocm_name);
		    return FAIL;
		}

		typval_T *tv = &cl->class_members_tv[i];
		copy_tv(tv, rettv);
		class_unref(cl);

		*arg = name_end;
		return OK;
	    }
	}

	semsg(_(e_member_not_found_on_class_str_str), cl->class_name, name);
    }

    return FAIL;
}

/*
 * If "arg" points to a class or object method, return it.
 * Otherwise return NULL.
 */
    ufunc_T *
find_class_func(char_u **arg)
{
    char_u *name = *arg;
    char_u *name_end = find_name_end(name, NULL, NULL, FNE_CHECK_START);
    if (name_end == name || *name_end != '.')
	return NULL;

    size_t len = name_end - name;
    typval_T tv;
    tv.v_type = VAR_UNKNOWN;
    if (eval_variable(name, (int)len,
				    0, &tv, NULL, EVAL_VAR_NOAUTOLOAD) == FAIL)
	return NULL;
    if (tv.v_type != VAR_CLASS && tv.v_type != VAR_OBJECT)
	goto fail_after_eval;

    class_T *cl = tv.v_type == VAR_CLASS ? tv.vval.v_class
						 : tv.vval.v_object->obj_class;
    if (cl == NULL)
	goto fail_after_eval;
    char_u *fname = name_end + 1;
    char_u *fname_end = find_name_end(fname, NULL, NULL, FNE_CHECK_START);
    if (fname_end == fname)
	goto fail_after_eval;
    len = fname_end - fname;

    int count = tv.v_type == VAR_CLASS ? cl->class_class_function_count
				       : cl->class_obj_method_count;
    ufunc_T **funcs = tv.v_type == VAR_CLASS ? cl->class_class_functions
					     : cl->class_obj_methods;
    for (int i = 0; i < count; ++i)
    {
	ufunc_T *fp = funcs[i];
	// Use a separate pointer to avoid that ASAN complains about
	// uf_name[] only being 4 characters.
	char_u *ufname = (char_u *)fp->uf_name;
	if (STRNCMP(fname, ufname, len) == 0 && ufname[len] == NUL)
	{
	    clear_tv(&tv);
	    return fp;
	}
    }

fail_after_eval:
    clear_tv(&tv);
    return NULL;
}

/*
 * If "name[len]" is a class member in cctx->ctx_ufunc->uf_class return the
 * index in class.class_class_members[].
 * If "cl_ret" is not NULL set it to the class.
 * Otherwise return -1;
 */
    int
class_member_index(char_u *name, size_t len, class_T **cl_ret, cctx_T *cctx)
{
    if (cctx == NULL || cctx->ctx_ufunc == NULL
					  || cctx->ctx_ufunc->uf_class == NULL)
	return -1;
    class_T *cl = cctx->ctx_ufunc->uf_class;

    for (int i = 0; i < cl->class_class_member_count; ++i)
    {
	ocmember_T *m = &cl->class_class_members[i];
	if (STRNCMP(name, m->ocm_name, len) == 0 && m->ocm_name[len] == NUL)
	{
	    if (cl_ret != NULL)
		*cl_ret = cl;
	    return i;
	}
    }
    return -1;
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
    if (cl != NULL && --cl->class_refcount <= 0 && cl->class_name != NULL)
    {
	// Freeing what the class contains may recursively come back here.
	// Clear "class_name" first, if it is NULL the class does not need to
	// be freed.
	VIM_CLEAR(cl->class_name);

	for (int i = 0; i < cl->class_class_member_count; ++i)
	{
	    ocmember_T *m = &cl->class_class_members[i];
	    vim_free(m->ocm_name);
	    vim_free(m->ocm_init);
	    if (cl->class_members_tv != NULL)
		clear_tv(&cl->class_members_tv[i]);
	}
	vim_free(cl->class_class_members);
	vim_free(cl->class_members_tv);

	for (int i = 0; i < cl->class_obj_member_count; ++i)
	{
	    ocmember_T *m = &cl->class_obj_members[i];
	    vim_free(m->ocm_name);
	    vim_free(m->ocm_init);
	}
	vim_free(cl->class_obj_members);

	for (int i = 0; i < cl->class_class_function_count; ++i)
	{
	    ufunc_T *uf = cl->class_class_functions[i];
	    func_clear_free(uf, FALSE);
	}
	vim_free(cl->class_class_functions);

	for (int i = 0; i < cl->class_obj_method_count; ++i)
	{
	    ufunc_T *uf = cl->class_obj_methods[i];
	    func_clear_free(uf, FALSE);
	}
	vim_free(cl->class_obj_methods);

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
