/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * evalvars.c: functions for dealing with variables
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

static char *e_letunexp	= N_("E18: Unexpected characters in :let");

static void ex_let_const(exarg_T *eap, int is_const);
static char_u *skip_var_one(char_u *arg);
static void list_glob_vars(int *first);
static void list_buf_vars(int *first);
static void list_win_vars(int *first);
static void list_tab_vars(int *first);
static char_u *list_arg_vars(exarg_T *eap, char_u *arg, int *first);
static char_u *ex_let_one(char_u *arg, typval_T *tv, int copy, int is_const, char_u *endchars, char_u *op);
static void ex_unletlock(exarg_T *eap, char_u *argstart, int deep);
static int do_unlet_var(lval_T *lp, char_u *name_end, int forceit);
static int do_lock_var(lval_T *lp, char_u *name_end, int deep, int lock);
static void item_lock(typval_T *tv, int deep, int lock);
static void list_one_var(dictitem_T *v, char *prefix, int *first);
static void list_one_var_a(char *prefix, char_u *name, int type, char_u *string, int *first);

/*
 * Get a list of lines from a HERE document. The here document is a list of
 * lines surrounded by a marker.
 *	cmd << {marker}
 *	  {line1}
 *	  {line2}
 *	  ....
 *	{marker}
 *
 * The {marker} is a string. If the optional 'trim' word is supplied before the
 * marker, then the leading indentation before the lines (matching the
 * indentation in the 'cmd' line) is stripped.
 * Returns a List with {lines} or NULL.
 */
    static list_T *
heredoc_get(exarg_T *eap, char_u *cmd)
{
    char_u	*theline;
    char_u	*marker;
    list_T	*l;
    char_u	*p;
    int		marker_indent_len = 0;
    int		text_indent_len = 0;
    char_u	*text_indent = NULL;

    if (eap->getline == NULL)
    {
	emsg(_("E991: cannot use =<< here"));
	return NULL;
    }

    // Check for the optional 'trim' word before the marker
    cmd = skipwhite(cmd);
    if (STRNCMP(cmd, "trim", 4) == 0 && (cmd[4] == NUL || VIM_ISWHITE(cmd[4])))
    {
	cmd = skipwhite(cmd + 4);

	// Trim the indentation from all the lines in the here document.
	// The amount of indentation trimmed is the same as the indentation of
	// the first line after the :let command line.  To find the end marker
	// the indent of the :let command line is trimmed.
	p = *eap->cmdlinep;
	while (VIM_ISWHITE(*p))
	{
	    p++;
	    marker_indent_len++;
	}
	text_indent_len = -1;
    }

    // The marker is the next word.
    if (*cmd != NUL && *cmd != '"')
    {
	marker = skipwhite(cmd);
	p = skiptowhite(marker);
	if (*skipwhite(p) != NUL && *skipwhite(p) != '"')
	{
	    emsg(_(e_trailing));
	    return NULL;
	}
	*p = NUL;
	if (vim_islower(*marker))
	{
	    emsg(_("E221: Marker cannot start with lower case letter"));
	    return NULL;
	}
    }
    else
    {
	emsg(_("E172: Missing marker"));
	return NULL;
    }

    l = list_alloc();
    if (l == NULL)
	return NULL;

    for (;;)
    {
	int	mi = 0;
	int	ti = 0;

	theline = eap->getline(NUL, eap->cookie, 0, FALSE);
	if (theline == NULL)
	{
	    semsg(_("E990: Missing end marker '%s'"), marker);
	    break;
	}

	// with "trim": skip the indent matching the :let line to find the
	// marker
	if (marker_indent_len > 0
		&& STRNCMP(theline, *eap->cmdlinep, marker_indent_len) == 0)
	    mi = marker_indent_len;
	if (STRCMP(marker, theline + mi) == 0)
	{
	    vim_free(theline);
	    break;
	}

	if (text_indent_len == -1 && *theline != NUL)
	{
	    // set the text indent from the first line.
	    p = theline;
	    text_indent_len = 0;
	    while (VIM_ISWHITE(*p))
	    {
		p++;
		text_indent_len++;
	    }
	    text_indent = vim_strnsave(theline, text_indent_len);
	}
	// with "trim": skip the indent matching the first line
	if (text_indent != NULL)
	    for (ti = 0; ti < text_indent_len; ++ti)
		if (theline[ti] != text_indent[ti])
		    break;

	if (list_append_string(l, theline + ti, -1) == FAIL)
	    break;
	vim_free(theline);
    }
    vim_free(text_indent);

    return l;
}

/*
 * ":let"			list all variable values
 * ":let var1 var2"		list variable values
 * ":let var = expr"		assignment command.
 * ":let var += expr"		assignment command.
 * ":let var -= expr"		assignment command.
 * ":let var *= expr"		assignment command.
 * ":let var /= expr"		assignment command.
 * ":let var %= expr"		assignment command.
 * ":let var .= expr"		assignment command.
 * ":let var ..= expr"		assignment command.
 * ":let [var1, var2] = expr"	unpack list.
 */
    void
ex_let(exarg_T *eap)
{
    ex_let_const(eap, FALSE);
}

/*
 * ":const"			list all variable values
 * ":const var1 var2"		list variable values
 * ":const var = expr"		assignment command.
 * ":const [var1, var2] = expr"	unpack list.
 */
    void
ex_const(exarg_T *eap)
{
    ex_let_const(eap, TRUE);
}

    static void
ex_let_const(exarg_T *eap, int is_const)
{
    char_u	*arg = eap->arg;
    char_u	*expr = NULL;
    typval_T	rettv;
    int		i;
    int		var_count = 0;
    int		semicolon = 0;
    char_u	op[2];
    char_u	*argend;
    int		first = TRUE;
    int		concat;

    argend = skip_var_list(arg, &var_count, &semicolon);
    if (argend == NULL)
	return;
    if (argend > arg && argend[-1] == '.')  // for var.='str'
	--argend;
    expr = skipwhite(argend);
    concat = expr[0] == '.'
	&& ((expr[1] == '=' && current_sctx.sc_version < 2)
		|| (expr[1] == '.' && expr[2] == '='));
    if (*expr != '=' && !((vim_strchr((char_u *)"+-*/%", *expr) != NULL
						 && expr[1] == '=') || concat))
    {
	// ":let" without "=": list variables
	if (*arg == '[')
	    emsg(_(e_invarg));
	else if (expr[0] == '.')
	    emsg(_("E985: .= is not supported with script version 2"));
	else if (!ends_excmd(*arg))
	    // ":let var1 var2"
	    arg = list_arg_vars(eap, arg, &first);
	else if (!eap->skip)
	{
	    // ":let"
	    list_glob_vars(&first);
	    list_buf_vars(&first);
	    list_win_vars(&first);
	    list_tab_vars(&first);
	    list_script_vars(&first);
	    list_func_vars(&first);
	    list_vim_vars(&first);
	}
	eap->nextcmd = check_nextcmd(arg);
    }
    else if (expr[0] == '=' && expr[1] == '<' && expr[2] == '<')
    {
	list_T	*l;

	// HERE document
	l = heredoc_get(eap, expr + 3);
	if (l != NULL)
	{
	    rettv_list_set(&rettv, l);
	    op[0] = '=';
	    op[1] = NUL;
	    (void)ex_let_vars(eap->arg, &rettv, FALSE, semicolon, var_count,
								is_const, op);
	    clear_tv(&rettv);
	}
    }
    else
    {
	op[0] = '=';
	op[1] = NUL;
	if (*expr != '=')
	{
	    if (vim_strchr((char_u *)"+-*/%.", *expr) != NULL)
	    {
		op[0] = *expr;   // +=, -=, *=, /=, %= or .=
		if (expr[0] == '.' && expr[1] == '.') // ..=
		    ++expr;
	    }
	    expr = skipwhite(expr + 2);
	}
	else
	    expr = skipwhite(expr + 1);

	if (eap->skip)
	    ++emsg_skip;
	i = eval0(expr, &rettv, &eap->nextcmd, !eap->skip);
	if (eap->skip)
	{
	    if (i != FAIL)
		clear_tv(&rettv);
	    --emsg_skip;
	}
	else if (i != FAIL)
	{
	    (void)ex_let_vars(eap->arg, &rettv, FALSE, semicolon, var_count,
								 is_const, op);
	    clear_tv(&rettv);
	}
    }
}

/*
 * Assign the typevalue "tv" to the variable or variables at "arg_start".
 * Handles both "var" with any type and "[var, var; var]" with a list type.
 * When "op" is not NULL it points to a string with characters that
 * must appear after the variable(s).  Use "+", "-" or "." for add, subtract
 * or concatenate.
 * Returns OK or FAIL;
 */
    int
ex_let_vars(
    char_u	*arg_start,
    typval_T	*tv,
    int		copy,		// copy values from "tv", don't move
    int		semicolon,	// from skip_var_list()
    int		var_count,	// from skip_var_list()
    int		is_const,	// lock variables for const
    char_u	*op)
{
    char_u	*arg = arg_start;
    list_T	*l;
    int		i;
    listitem_T	*item;
    typval_T	ltv;

    if (*arg != '[')
    {
	// ":let var = expr" or ":for var in list"
	if (ex_let_one(arg, tv, copy, is_const, op, op) == NULL)
	    return FAIL;
	return OK;
    }

    // ":let [v1, v2] = list" or ":for [v1, v2] in listlist"
    if (tv->v_type != VAR_LIST || (l = tv->vval.v_list) == NULL)
    {
	emsg(_(e_listreq));
	return FAIL;
    }

    i = list_len(l);
    if (semicolon == 0 && var_count < i)
    {
	emsg(_("E687: Less targets than List items"));
	return FAIL;
    }
    if (var_count - semicolon > i)
    {
	emsg(_("E688: More targets than List items"));
	return FAIL;
    }

    item = l->lv_first;
    while (*arg != ']')
    {
	arg = skipwhite(arg + 1);
	arg = ex_let_one(arg, &item->li_tv, TRUE, is_const,
							  (char_u *)",;]", op);
	item = item->li_next;
	if (arg == NULL)
	    return FAIL;

	arg = skipwhite(arg);
	if (*arg == ';')
	{
	    // Put the rest of the list (may be empty) in the var after ';'.
	    // Create a new list for this.
	    l = list_alloc();
	    if (l == NULL)
		return FAIL;
	    while (item != NULL)
	    {
		list_append_tv(l, &item->li_tv);
		item = item->li_next;
	    }

	    ltv.v_type = VAR_LIST;
	    ltv.v_lock = 0;
	    ltv.vval.v_list = l;
	    l->lv_refcount = 1;

	    arg = ex_let_one(skipwhite(arg + 1), &ltv, FALSE, is_const,
							    (char_u *)"]", op);
	    clear_tv(&ltv);
	    if (arg == NULL)
		return FAIL;
	    break;
	}
	else if (*arg != ',' && *arg != ']')
	{
	    internal_error("ex_let_vars()");
	    return FAIL;
	}
    }

    return OK;
}

/*
 * Skip over assignable variable "var" or list of variables "[var, var]".
 * Used for ":let varvar = expr" and ":for varvar in expr".
 * For "[var, var]" increment "*var_count" for each variable.
 * for "[var, var; var]" set "semicolon".
 * Return NULL for an error.
 */
    char_u *
skip_var_list(
    char_u	*arg,
    int		*var_count,
    int		*semicolon)
{
    char_u	*p, *s;

    if (*arg == '[')
    {
	// "[var, var]": find the matching ']'.
	p = arg;
	for (;;)
	{
	    p = skipwhite(p + 1);	// skip whites after '[', ';' or ','
	    s = skip_var_one(p);
	    if (s == p)
	    {
		semsg(_(e_invarg2), p);
		return NULL;
	    }
	    ++*var_count;

	    p = skipwhite(s);
	    if (*p == ']')
		break;
	    else if (*p == ';')
	    {
		if (*semicolon == 1)
		{
		    emsg(_("Double ; in list of variables"));
		    return NULL;
		}
		*semicolon = 1;
	    }
	    else if (*p != ',')
	    {
		semsg(_(e_invarg2), p);
		return NULL;
	    }
	}
	return p + 1;
    }
    else
	return skip_var_one(arg);
}

/*
 * Skip one (assignable) variable name, including @r, $VAR, &option, d.key,
 * l[idx].
 */
    static char_u *
skip_var_one(char_u *arg)
{
    if (*arg == '@' && arg[1] != NUL)
	return arg + 2;
    return find_name_end(*arg == '$' || *arg == '&' ? arg + 1 : arg,
				   NULL, NULL, FNE_INCL_BR | FNE_CHECK_START);
}

/*
 * List variables for hashtab "ht" with prefix "prefix".
 * If "empty" is TRUE also list NULL strings as empty strings.
 */
    void
list_hashtable_vars(
    hashtab_T	*ht,
    char	*prefix,
    int		empty,
    int		*first)
{
    hashitem_T	*hi;
    dictitem_T	*di;
    int		todo;
    char_u	buf[IOSIZE];

    todo = (int)ht->ht_used;
    for (hi = ht->ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    di = HI2DI(hi);

	    // apply :filter /pat/ to variable name
	    vim_strncpy((char_u *)buf, (char_u *)prefix, IOSIZE - 1);
	    vim_strcat((char_u *)buf, di->di_key, IOSIZE);
	    if (message_filtered(buf))
		continue;

	    if (empty || di->di_tv.v_type != VAR_STRING
					   || di->di_tv.vval.v_string != NULL)
		list_one_var(di, prefix, first);
	}
    }
}

/*
 * List global variables.
 */
    static void
list_glob_vars(int *first)
{
    list_hashtable_vars(&globvarht, "", TRUE, first);
}

/*
 * List buffer variables.
 */
    static void
list_buf_vars(int *first)
{
    list_hashtable_vars(&curbuf->b_vars->dv_hashtab, "b:", TRUE, first);
}

/*
 * List window variables.
 */
    static void
list_win_vars(int *first)
{
    list_hashtable_vars(&curwin->w_vars->dv_hashtab, "w:", TRUE, first);
}

/*
 * List tab page variables.
 */
    static void
list_tab_vars(int *first)
{
    list_hashtable_vars(&curtab->tp_vars->dv_hashtab, "t:", TRUE, first);
}

/*
 * List variables in "arg".
 */
    static char_u *
list_arg_vars(exarg_T *eap, char_u *arg, int *first)
{
    int		error = FALSE;
    int		len;
    char_u	*name;
    char_u	*name_start;
    char_u	*arg_subsc;
    char_u	*tofree;
    typval_T    tv;

    while (!ends_excmd(*arg) && !got_int)
    {
	if (error || eap->skip)
	{
	    arg = find_name_end(arg, NULL, NULL, FNE_INCL_BR | FNE_CHECK_START);
	    if (!VIM_ISWHITE(*arg) && !ends_excmd(*arg))
	    {
		emsg_severe = TRUE;
		emsg(_(e_trailing));
		break;
	    }
	}
	else
	{
	    // get_name_len() takes care of expanding curly braces
	    name_start = name = arg;
	    len = get_name_len(&arg, &tofree, TRUE, TRUE);
	    if (len <= 0)
	    {
		// This is mainly to keep test 49 working: when expanding
		// curly braces fails overrule the exception error message.
		if (len < 0 && !aborting())
		{
		    emsg_severe = TRUE;
		    semsg(_(e_invarg2), arg);
		    break;
		}
		error = TRUE;
	    }
	    else
	    {
		if (tofree != NULL)
		    name = tofree;
		if (get_var_tv(name, len, &tv, NULL, TRUE, FALSE) == FAIL)
		    error = TRUE;
		else
		{
		    // handle d.key, l[idx], f(expr)
		    arg_subsc = arg;
		    if (handle_subscript(&arg, &tv, TRUE, TRUE,
							  name, &name) == FAIL)
			error = TRUE;
		    else
		    {
			if (arg == arg_subsc && len == 2 && name[1] == ':')
			{
			    switch (*name)
			    {
				case 'g': list_glob_vars(first); break;
				case 'b': list_buf_vars(first); break;
				case 'w': list_win_vars(first); break;
				case 't': list_tab_vars(first); break;
				case 'v': list_vim_vars(first); break;
				case 's': list_script_vars(first); break;
				case 'l': list_func_vars(first); break;
				default:
					  semsg(_("E738: Can't list variables for %s"), name);
			    }
			}
			else
			{
			    char_u	numbuf[NUMBUFLEN];
			    char_u	*tf;
			    int		c;
			    char_u	*s;

			    s = echo_string(&tv, &tf, numbuf, 0);
			    c = *arg;
			    *arg = NUL;
			    list_one_var_a("",
				    arg == arg_subsc ? name : name_start,
				    tv.v_type,
				    s == NULL ? (char_u *)"" : s,
				    first);
			    *arg = c;
			    vim_free(tf);
			}
			clear_tv(&tv);
		    }
		}
	    }

	    vim_free(tofree);
	}

	arg = skipwhite(arg);
    }

    return arg;
}

/*
 * Set one item of ":let var = expr" or ":let [v1, v2] = list" to its value.
 * Returns a pointer to the char just after the var name.
 * Returns NULL if there is an error.
 */
    static char_u *
ex_let_one(
    char_u	*arg,		// points to variable name
    typval_T	*tv,		// value to assign to variable
    int		copy,		// copy value from "tv"
    int		is_const,	// lock variable for const
    char_u	*endchars,	// valid chars after variable name  or NULL
    char_u	*op)		// "+", "-", "."  or NULL
{
    int		c1;
    char_u	*name;
    char_u	*p;
    char_u	*arg_end = NULL;
    int		len;
    int		opt_flags;
    char_u	*tofree = NULL;

    // ":let $VAR = expr": Set environment variable.
    if (*arg == '$')
    {
	if (is_const)
	{
	    emsg(_("E996: Cannot lock an environment variable"));
	    return NULL;
	}
	// Find the end of the name.
	++arg;
	name = arg;
	len = get_env_len(&arg);
	if (len == 0)
	    semsg(_(e_invarg2), name - 1);
	else
	{
	    if (op != NULL && vim_strchr((char_u *)"+-*/%", *op) != NULL)
		semsg(_(e_letwrong), op);
	    else if (endchars != NULL
			     && vim_strchr(endchars, *skipwhite(arg)) == NULL)
		emsg(_(e_letunexp));
	    else if (!check_secure())
	    {
		c1 = name[len];
		name[len] = NUL;
		p = tv_get_string_chk(tv);
		if (p != NULL && op != NULL && *op == '.')
		{
		    int	    mustfree = FALSE;
		    char_u  *s = vim_getenv(name, &mustfree);

		    if (s != NULL)
		    {
			p = tofree = concat_str(s, p);
			if (mustfree)
			    vim_free(s);
		    }
		}
		if (p != NULL)
		{
		    vim_setenv(name, p);
		    if (STRICMP(name, "HOME") == 0)
			init_homedir();
		    else if (didset_vim && STRICMP(name, "VIM") == 0)
			didset_vim = FALSE;
		    else if (didset_vimruntime
					&& STRICMP(name, "VIMRUNTIME") == 0)
			didset_vimruntime = FALSE;
		    arg_end = arg;
		}
		name[len] = c1;
		vim_free(tofree);
	    }
	}
    }

    // ":let &option = expr": Set option value.
    // ":let &l:option = expr": Set local option value.
    // ":let &g:option = expr": Set global option value.
    else if (*arg == '&')
    {
	if (is_const)
	{
	    emsg(_("E996: Cannot lock an option"));
	    return NULL;
	}
	// Find the end of the name.
	p = find_option_end(&arg, &opt_flags);
	if (p == NULL || (endchars != NULL
			      && vim_strchr(endchars, *skipwhite(p)) == NULL))
	    emsg(_(e_letunexp));
	else
	{
	    long	n;
	    int		opt_type;
	    long	numval;
	    char_u	*stringval = NULL;
	    char_u	*s;

	    c1 = *p;
	    *p = NUL;

	    n = (long)tv_get_number(tv);
	    s = tv_get_string_chk(tv);	    // != NULL if number or string
	    if (s != NULL && op != NULL && *op != '=')
	    {
		opt_type = get_option_value(arg, &numval,
						       &stringval, opt_flags);
		if ((opt_type == 1 && *op == '.')
			|| (opt_type == 0 && *op != '.'))
		{
		    semsg(_(e_letwrong), op);
		    s = NULL;  // don't set the value
		}
		else
		{
		    if (opt_type == 1)  // number
		    {
			switch (*op)
			{
			    case '+': n = numval + n; break;
			    case '-': n = numval - n; break;
			    case '*': n = numval * n; break;
			    case '/': n = (long)num_divide(numval, n); break;
			    case '%': n = (long)num_modulus(numval, n); break;
			}
		    }
		    else if (opt_type == 0 && stringval != NULL) // string
		    {
			s = concat_str(stringval, s);
			vim_free(stringval);
			stringval = s;
		    }
		}
	    }
	    if (s != NULL)
	    {
		set_option_value(arg, n, s, opt_flags);
		arg_end = p;
	    }
	    *p = c1;
	    vim_free(stringval);
	}
    }

    // ":let @r = expr": Set register contents.
    else if (*arg == '@')
    {
	if (is_const)
	{
	    emsg(_("E996: Cannot lock a register"));
	    return NULL;
	}
	++arg;
	if (op != NULL && vim_strchr((char_u *)"+-*/%", *op) != NULL)
	    semsg(_(e_letwrong), op);
	else if (endchars != NULL
			 && vim_strchr(endchars, *skipwhite(arg + 1)) == NULL)
	    emsg(_(e_letunexp));
	else
	{
	    char_u	*ptofree = NULL;
	    char_u	*s;

	    p = tv_get_string_chk(tv);
	    if (p != NULL && op != NULL && *op == '.')
	    {
		s = get_reg_contents(*arg == '@' ? '"' : *arg, GREG_EXPR_SRC);
		if (s != NULL)
		{
		    p = ptofree = concat_str(s, p);
		    vim_free(s);
		}
	    }
	    if (p != NULL)
	    {
		write_reg_contents(*arg == '@' ? '"' : *arg, p, -1, FALSE);
		arg_end = arg + 1;
	    }
	    vim_free(ptofree);
	}
    }

    // ":let var = expr": Set internal variable.
    // ":let {expr} = expr": Idem, name made with curly braces
    else if (eval_isnamec1(*arg) || *arg == '{')
    {
	lval_T	lv;

	p = get_lval(arg, tv, &lv, FALSE, FALSE, 0, FNE_CHECK_START);
	if (p != NULL && lv.ll_name != NULL)
	{
	    if (endchars != NULL && vim_strchr(endchars, *skipwhite(p)) == NULL)
		emsg(_(e_letunexp));
	    else
	    {
		set_var_lval(&lv, p, tv, copy, is_const, op);
		arg_end = p;
	    }
	}
	clear_lval(&lv);
    }

    else
	semsg(_(e_invarg2), arg);

    return arg_end;
}

/*
 * ":unlet[!] var1 ... " command.
 */
    void
ex_unlet(exarg_T *eap)
{
    ex_unletlock(eap, eap->arg, 0);
}

/*
 * ":lockvar" and ":unlockvar" commands
 */
    void
ex_lockvar(exarg_T *eap)
{
    char_u	*arg = eap->arg;
    int		deep = 2;

    if (eap->forceit)
	deep = -1;
    else if (vim_isdigit(*arg))
    {
	deep = getdigits(&arg);
	arg = skipwhite(arg);
    }

    ex_unletlock(eap, arg, deep);
}

/*
 * ":unlet", ":lockvar" and ":unlockvar" are quite similar.
 */
    static void
ex_unletlock(
    exarg_T	*eap,
    char_u	*argstart,
    int		deep)
{
    char_u	*arg = argstart;
    char_u	*name_end;
    int		error = FALSE;
    lval_T	lv;

    do
    {
	if (*arg == '$')
	{
	    char_u    *name = ++arg;

	    if (get_env_len(&arg) == 0)
	    {
		semsg(_(e_invarg2), name - 1);
		return;
	    }
	    vim_unsetenv(name);
	    arg = skipwhite(arg);
	    continue;
	}

	// Parse the name and find the end.
	name_end = get_lval(arg, NULL, &lv, TRUE, eap->skip || error, 0,
							     FNE_CHECK_START);
	if (lv.ll_name == NULL)
	    error = TRUE;	    // error but continue parsing
	if (name_end == NULL || (!VIM_ISWHITE(*name_end)
						   && !ends_excmd(*name_end)))
	{
	    if (name_end != NULL)
	    {
		emsg_severe = TRUE;
		emsg(_(e_trailing));
	    }
	    if (!(eap->skip || error))
		clear_lval(&lv);
	    break;
	}

	if (!error && !eap->skip)
	{
	    if (eap->cmdidx == CMD_unlet)
	    {
		if (do_unlet_var(&lv, name_end, eap->forceit) == FAIL)
		    error = TRUE;
	    }
	    else
	    {
		if (do_lock_var(&lv, name_end, deep,
					  eap->cmdidx == CMD_lockvar) == FAIL)
		    error = TRUE;
	    }
	}

	if (!eap->skip)
	    clear_lval(&lv);

	arg = skipwhite(name_end);
    } while (!ends_excmd(*arg));

    eap->nextcmd = check_nextcmd(arg);
}

    static int
do_unlet_var(
    lval_T	*lp,
    char_u	*name_end,
    int		forceit)
{
    int		ret = OK;
    int		cc;

    if (lp->ll_tv == NULL)
    {
	cc = *name_end;
	*name_end = NUL;

	// Normal name or expanded name.
	if (do_unlet(lp->ll_name, forceit) == FAIL)
	    ret = FAIL;
	*name_end = cc;
    }
    else if ((lp->ll_list != NULL
		 && var_check_lock(lp->ll_list->lv_lock, lp->ll_name, FALSE))
	    || (lp->ll_dict != NULL
		 && var_check_lock(lp->ll_dict->dv_lock, lp->ll_name, FALSE)))
	return FAIL;
    else if (lp->ll_range)
    {
	listitem_T    *li;
	listitem_T    *ll_li = lp->ll_li;
	int	      ll_n1 = lp->ll_n1;

	while (ll_li != NULL && (lp->ll_empty2 || lp->ll_n2 >= ll_n1))
	{
	    li = ll_li->li_next;
	    if (var_check_lock(ll_li->li_tv.v_lock, lp->ll_name, FALSE))
		return FAIL;
	    ll_li = li;
	    ++ll_n1;
	}

	// Delete a range of List items.
	while (lp->ll_li != NULL && (lp->ll_empty2 || lp->ll_n2 >= lp->ll_n1))
	{
	    li = lp->ll_li->li_next;
	    listitem_remove(lp->ll_list, lp->ll_li);
	    lp->ll_li = li;
	    ++lp->ll_n1;
	}
    }
    else
    {
	if (lp->ll_list != NULL)
	    // unlet a List item.
	    listitem_remove(lp->ll_list, lp->ll_li);
	else
	    // unlet a Dictionary item.
	    dictitem_remove(lp->ll_dict, lp->ll_di);
    }

    return ret;
}

/*
 * "unlet" a variable.  Return OK if it existed, FAIL if not.
 * When "forceit" is TRUE don't complain if the variable doesn't exist.
 */
    int
do_unlet(char_u *name, int forceit)
{
    hashtab_T	*ht;
    hashitem_T	*hi;
    char_u	*varname;
    dict_T	*d;
    dictitem_T	*di;

    ht = find_var_ht(name, &varname);
    if (ht != NULL && *varname != NUL)
    {
	d = get_current_funccal_dict(ht);
	if (d == NULL)
	{
	    if (ht == &globvarht)
		d = &globvardict;
	    else if (is_compatht(ht))
		d = &vimvardict;
	    else
	    {
		di = find_var_in_ht(ht, *name, (char_u *)"", FALSE);
		d = di == NULL ? NULL : di->di_tv.vval.v_dict;
	    }
	    if (d == NULL)
	    {
		internal_error("do_unlet()");
		return FAIL;
	    }
	}
	hi = hash_find(ht, varname);
	if (HASHITEM_EMPTY(hi))
	    hi = find_hi_in_scoped_ht(name, &ht);
	if (hi != NULL && !HASHITEM_EMPTY(hi))
	{
	    di = HI2DI(hi);
	    if (var_check_fixed(di->di_flags, name, FALSE)
		    || var_check_ro(di->di_flags, name, FALSE)
		    || var_check_lock(d->dv_lock, name, FALSE))
		return FAIL;

	    delete_var(ht, hi);
	    return OK;
	}
    }
    if (forceit)
	return OK;
    semsg(_("E108: No such variable: \"%s\""), name);
    return FAIL;
}

/*
 * Lock or unlock variable indicated by "lp".
 * "deep" is the levels to go (-1 for unlimited);
 * "lock" is TRUE for ":lockvar", FALSE for ":unlockvar".
 */
    static int
do_lock_var(
    lval_T	*lp,
    char_u	*name_end,
    int		deep,
    int		lock)
{
    int		ret = OK;
    int		cc;
    dictitem_T	*di;

    if (deep == 0)	// nothing to do
	return OK;

    if (lp->ll_tv == NULL)
    {
	cc = *name_end;
	*name_end = NUL;

	// Normal name or expanded name.
	di = find_var(lp->ll_name, NULL, TRUE);
	if (di == NULL)
	    ret = FAIL;
	else if ((di->di_flags & DI_FLAGS_FIX)
			&& di->di_tv.v_type != VAR_DICT
			&& di->di_tv.v_type != VAR_LIST)
	    // For historic reasons this error is not given for a list or dict.
	    // E.g., the b: dict could be locked/unlocked.
	    semsg(_("E940: Cannot lock or unlock variable %s"), lp->ll_name);
	else
	{
	    if (lock)
		di->di_flags |= DI_FLAGS_LOCK;
	    else
		di->di_flags &= ~DI_FLAGS_LOCK;
	    item_lock(&di->di_tv, deep, lock);
	}
	*name_end = cc;
    }
    else if (lp->ll_range)
    {
	listitem_T    *li = lp->ll_li;

	// (un)lock a range of List items.
	while (li != NULL && (lp->ll_empty2 || lp->ll_n2 >= lp->ll_n1))
	{
	    item_lock(&li->li_tv, deep, lock);
	    li = li->li_next;
	    ++lp->ll_n1;
	}
    }
    else if (lp->ll_list != NULL)
	// (un)lock a List item.
	item_lock(&lp->ll_li->li_tv, deep, lock);
    else
	// (un)lock a Dictionary item.
	item_lock(&lp->ll_di->di_tv, deep, lock);

    return ret;
}

/*
 * Lock or unlock an item.  "deep" is nr of levels to go.
 */
    static void
item_lock(typval_T *tv, int deep, int lock)
{
    static int	recurse = 0;
    list_T	*l;
    listitem_T	*li;
    dict_T	*d;
    blob_T	*b;
    hashitem_T	*hi;
    int		todo;

    if (recurse >= DICT_MAXNEST)
    {
	emsg(_("E743: variable nested too deep for (un)lock"));
	return;
    }
    if (deep == 0)
	return;
    ++recurse;

    // lock/unlock the item itself
    if (lock)
	tv->v_lock |= VAR_LOCKED;
    else
	tv->v_lock &= ~VAR_LOCKED;

    switch (tv->v_type)
    {
	case VAR_UNKNOWN:
	case VAR_NUMBER:
	case VAR_STRING:
	case VAR_FUNC:
	case VAR_PARTIAL:
	case VAR_FLOAT:
	case VAR_SPECIAL:
	case VAR_JOB:
	case VAR_CHANNEL:
	    break;

	case VAR_BLOB:
	    if ((b = tv->vval.v_blob) != NULL)
	    {
		if (lock)
		    b->bv_lock |= VAR_LOCKED;
		else
		    b->bv_lock &= ~VAR_LOCKED;
	    }
	    break;
	case VAR_LIST:
	    if ((l = tv->vval.v_list) != NULL)
	    {
		if (lock)
		    l->lv_lock |= VAR_LOCKED;
		else
		    l->lv_lock &= ~VAR_LOCKED;
		if (deep < 0 || deep > 1)
		    // recursive: lock/unlock the items the List contains
		    for (li = l->lv_first; li != NULL; li = li->li_next)
			item_lock(&li->li_tv, deep - 1, lock);
	    }
	    break;
	case VAR_DICT:
	    if ((d = tv->vval.v_dict) != NULL)
	    {
		if (lock)
		    d->dv_lock |= VAR_LOCKED;
		else
		    d->dv_lock &= ~VAR_LOCKED;
		if (deep < 0 || deep > 1)
		{
		    // recursive: lock/unlock the items the List contains
		    todo = (int)d->dv_hashtab.ht_used;
		    for (hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
		    {
			if (!HASHITEM_EMPTY(hi))
			{
			    --todo;
			    item_lock(&HI2DI(hi)->di_tv, deep - 1, lock);
			}
		    }
		}
	    }
    }
    --recurse;
}

/*
 * Get the value of internal variable "name".
 * Return OK or FAIL.  If OK is returned "rettv" must be cleared.
 */
    int
get_var_tv(
    char_u	*name,
    int		len,		// length of "name"
    typval_T	*rettv,		// NULL when only checking existence
    dictitem_T	**dip,		// non-NULL when typval's dict item is needed
    int		verbose,	// may give error message
    int		no_autoload)	// do not use script autoloading
{
    int		ret = OK;
    typval_T	*tv = NULL;
    dictitem_T	*v;
    int		cc;

    // truncate the name, so that we can use strcmp()
    cc = name[len];
    name[len] = NUL;

    // Check for user-defined variables.
    v = find_var(name, NULL, no_autoload);
    if (v != NULL)
    {
	tv = &v->di_tv;
	if (dip != NULL)
	    *dip = v;
    }

    if (tv == NULL)
    {
	if (rettv != NULL && verbose)
	    semsg(_(e_undefvar), name);
	ret = FAIL;
    }
    else if (rettv != NULL)
	copy_tv(tv, rettv);

    name[len] = cc;

    return ret;
}

/*
 * Get the string value of a (global/local) variable.
 * Note: see tv_get_string() for how long the pointer remains valid.
 * Returns NULL when it doesn't exist.
 */
    char_u *
get_var_value(char_u *name)
{
    dictitem_T	*v;

    v = find_var(name, NULL, FALSE);
    if (v == NULL)
	return NULL;
    return tv_get_string(&v->di_tv);
}

/*
 * Clean up a list of internal variables.
 * Frees all allocated variables and the value they contain.
 * Clears hashtab "ht", does not free it.
 */
    void
vars_clear(hashtab_T *ht)
{
    vars_clear_ext(ht, TRUE);
}

/*
 * Like vars_clear(), but only free the value if "free_val" is TRUE.
 */
    void
vars_clear_ext(hashtab_T *ht, int free_val)
{
    int		todo;
    hashitem_T	*hi;
    dictitem_T	*v;

    hash_lock(ht);
    todo = (int)ht->ht_used;
    for (hi = ht->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;

	    // Free the variable.  Don't remove it from the hashtab,
	    // ht_array might change then.  hash_clear() takes care of it
	    // later.
	    v = HI2DI(hi);
	    if (free_val)
		clear_tv(&v->di_tv);
	    if (v->di_flags & DI_FLAGS_ALLOC)
		vim_free(v);
	}
    }
    hash_clear(ht);
    ht->ht_used = 0;
}

/*
 * Delete a variable from hashtab "ht" at item "hi".
 * Clear the variable value and free the dictitem.
 */
    void
delete_var(hashtab_T *ht, hashitem_T *hi)
{
    dictitem_T	*di = HI2DI(hi);

    hash_remove(ht, hi);
    clear_tv(&di->di_tv);
    vim_free(di);
}

/*
 * List the value of one internal variable.
 */
    static void
list_one_var(dictitem_T *v, char *prefix, int *first)
{
    char_u	*tofree;
    char_u	*s;
    char_u	numbuf[NUMBUFLEN];

    s = echo_string(&v->di_tv, &tofree, numbuf, get_copyID());
    list_one_var_a(prefix, v->di_key, v->di_tv.v_type,
					 s == NULL ? (char_u *)"" : s, first);
    vim_free(tofree);
}

    static void
list_one_var_a(
    char	*prefix,
    char_u	*name,
    int		type,
    char_u	*string,
    int		*first)  // when TRUE clear rest of screen and set to FALSE
{
    // don't use msg() or msg_attr() to avoid overwriting "v:statusmsg"
    msg_start();
    msg_puts(prefix);
    if (name != NULL)	// "a:" vars don't have a name stored
	msg_puts((char *)name);
    msg_putchar(' ');
    msg_advance(22);
    if (type == VAR_NUMBER)
	msg_putchar('#');
    else if (type == VAR_FUNC || type == VAR_PARTIAL)
	msg_putchar('*');
    else if (type == VAR_LIST)
    {
	msg_putchar('[');
	if (*string == '[')
	    ++string;
    }
    else if (type == VAR_DICT)
    {
	msg_putchar('{');
	if (*string == '{')
	    ++string;
    }
    else
	msg_putchar(' ');

    msg_outtrans(string);

    if (type == VAR_FUNC || type == VAR_PARTIAL)
	msg_puts("()");
    if (*first)
    {
	msg_clr_eos();
	*first = FALSE;
    }
}

/*
 * Set variable "name" to value in "tv".
 * If the variable already exists, the value is updated.
 * Otherwise the variable is created.
 */
    void
set_var(
    char_u	*name,
    typval_T	*tv,
    int		copy)	    // make copy of value in "tv"
{
    set_var_const(name, tv, copy, FALSE);
}

/*
 * Set variable "name" to value in "tv".
 * If the variable already exists and "is_const" is FALSE the value is updated.
 * Otherwise the variable is created.
 */
    void
set_var_const(
    char_u	*name,
    typval_T	*tv,
    int		copy,	    // make copy of value in "tv"
    int		is_const)   // disallow to modify existing variable
{
    dictitem_T	*v;
    char_u	*varname;
    hashtab_T	*ht;

    ht = find_var_ht(name, &varname);
    if (ht == NULL || *varname == NUL)
    {
	semsg(_(e_illvar), name);
	return;
    }
    v = find_var_in_ht(ht, 0, varname, TRUE);

    // Search in parent scope which is possible to reference from lambda
    if (v == NULL)
	v = find_var_in_scoped_ht(name, TRUE);

    if ((tv->v_type == VAR_FUNC || tv->v_type == VAR_PARTIAL)
				      && var_check_func_name(name, v == NULL))
	return;

    if (v != NULL)
    {
	if (is_const)
	{
	    emsg(_(e_cannot_mod));
	    return;
	}

	// existing variable, need to clear the value
	if (var_check_ro(v->di_flags, name, FALSE)
			      || var_check_lock(v->di_tv.v_lock, name, FALSE))
	    return;

	// Handle setting internal v: variables separately where needed to
	// prevent changing the type.
	if (is_vimvarht(ht))
	{
	    if (v->di_tv.v_type == VAR_STRING)
	    {
		VIM_CLEAR(v->di_tv.vval.v_string);
		if (copy || tv->v_type != VAR_STRING)
		{
		    char_u *val = tv_get_string(tv);

		    // Careful: when assigning to v:errmsg and tv_get_string()
		    // causes an error message the variable will alrady be set.
		    if (v->di_tv.vval.v_string == NULL)
			v->di_tv.vval.v_string = vim_strsave(val);
		}
		else
		{
		    // Take over the string to avoid an extra alloc/free.
		    v->di_tv.vval.v_string = tv->vval.v_string;
		    tv->vval.v_string = NULL;
		}
		return;
	    }
	    else if (v->di_tv.v_type == VAR_NUMBER)
	    {
		v->di_tv.vval.v_number = tv_get_number(tv);
		if (STRCMP(varname, "searchforward") == 0)
		    set_search_direction(v->di_tv.vval.v_number ? '/' : '?');
#ifdef FEAT_SEARCH_EXTRA
		else if (STRCMP(varname, "hlsearch") == 0)
		{
		    no_hlsearch = !v->di_tv.vval.v_number;
		    redraw_all_later(SOME_VALID);
		}
#endif
		return;
	    }
	    else if (v->di_tv.v_type != tv->v_type)
	    {
		semsg(_("E963: setting %s to value with wrong type"), name);
		return;
	    }
	}

	clear_tv(&v->di_tv);
    }
    else		    // add a new variable
    {
	// Can't add "v:" or "a:" variable.
	if (is_vimvarht(ht) || ht == get_funccal_args_ht())
	{
	    semsg(_(e_illvar), name);
	    return;
	}

	// Make sure the variable name is valid.
	if (!valid_varname(varname))
	    return;

	v = alloc(sizeof(dictitem_T) + STRLEN(varname));
	if (v == NULL)
	    return;
	STRCPY(v->di_key, varname);
	if (hash_add(ht, DI2HIKEY(v)) == FAIL)
	{
	    vim_free(v);
	    return;
	}
	v->di_flags = DI_FLAGS_ALLOC;
	if (is_const)
	    v->di_flags |= DI_FLAGS_LOCK;
    }

    if (copy || tv->v_type == VAR_NUMBER || tv->v_type == VAR_FLOAT)
	copy_tv(tv, &v->di_tv);
    else
    {
	v->di_tv = *tv;
	v->di_tv.v_lock = 0;
	init_tv(tv);
    }

    if (is_const)
	v->di_tv.v_lock |= VAR_LOCKED;
}

/*
 * Return TRUE if di_flags "flags" indicates variable "name" is read-only.
 * Also give an error message.
 */
    int
var_check_ro(int flags, char_u *name, int use_gettext)
{
    if (flags & DI_FLAGS_RO)
    {
	semsg(_(e_readonlyvar), use_gettext ? (char_u *)_(name) : name);
	return TRUE;
    }
    if ((flags & DI_FLAGS_RO_SBX) && sandbox)
    {
	semsg(_(e_readonlysbx), use_gettext ? (char_u *)_(name) : name);
	return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if di_flags "flags" indicates variable "name" is fixed.
 * Also give an error message.
 */
    int
var_check_fixed(int flags, char_u *name, int use_gettext)
{
    if (flags & DI_FLAGS_FIX)
    {
	semsg(_("E795: Cannot delete variable %s"),
				      use_gettext ? (char_u *)_(name) : name);
	return TRUE;
    }
    return FALSE;
}

/*
 * Check if a funcref is assigned to a valid variable name.
 * Return TRUE and give an error if not.
 */
    int
var_check_func_name(
    char_u *name,    // points to start of variable name
    int    new_var)  // TRUE when creating the variable
{
    // Allow for w: b: s: and t:.
    if (!(vim_strchr((char_u *)"wbst", name[0]) != NULL && name[1] == ':')
	    && !ASCII_ISUPPER((name[0] != NUL && name[1] == ':')
						     ? name[2] : name[0]))
    {
	semsg(_("E704: Funcref variable name must start with a capital: %s"),
									name);
	return TRUE;
    }
    // Don't allow hiding a function.  When "v" is not NULL we might be
    // assigning another function to the same var, the type is checked
    // below.
    if (new_var && function_exists(name, FALSE))
    {
	semsg(_("E705: Variable name conflicts with existing function: %s"),
								    name);
	return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if "flags" indicates variable "name" is locked (immutable).
 * Also give an error message, using "name" or _("name") when use_gettext is
 * TRUE.
 */
    int
var_check_lock(int lock, char_u *name, int use_gettext)
{
    if (lock & VAR_LOCKED)
    {
	semsg(_("E741: Value is locked: %s"),
				name == NULL ? (char_u *)_("Unknown")
					     : use_gettext ? (char_u *)_(name)
					     : name);
	return TRUE;
    }
    if (lock & VAR_FIXED)
    {
	semsg(_("E742: Cannot change value of %s"),
				name == NULL ? (char_u *)_("Unknown")
					     : use_gettext ? (char_u *)_(name)
					     : name);
	return TRUE;
    }
    return FALSE;
}

/*
 * Check if a variable name is valid.
 * Return FALSE and give an error if not.
 */
    int
valid_varname(char_u *varname)
{
    char_u *p;

    for (p = varname; *p != NUL; ++p)
	if (!eval_isnamec1(*p) && (p == varname || !VIM_ISDIGIT(*p))
						   && *p != AUTOLOAD_CHAR)
	{
	    semsg(_(e_illvar), varname);
	    return FALSE;
	}
    return TRUE;
}

/*
 * getwinvar() and gettabwinvar()
 */
    static void
getwinvar(
    typval_T	*argvars,
    typval_T	*rettv,
    int		off)	    // 1 for gettabwinvar()
{
    win_T	*win;
    char_u	*varname;
    dictitem_T	*v;
    tabpage_T	*tp = NULL;
    int		done = FALSE;
    win_T	*oldcurwin;
    tabpage_T	*oldtabpage;
    int		need_switch_win;

    if (off == 1)
	tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    else
	tp = curtab;
    win = find_win_by_nr(&argvars[off], tp);
    varname = tv_get_string_chk(&argvars[off + 1]);
    ++emsg_off;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (win != NULL && varname != NULL)
    {
	// Set curwin to be our win, temporarily.  Also set the tabpage,
	// otherwise the window is not valid. Only do this when needed,
	// autocommands get blocked.
	need_switch_win = !(tp == curtab && win == curwin);
	if (!need_switch_win
		  || switch_win(&oldcurwin, &oldtabpage, win, tp, TRUE) == OK)
	{
	    if (*varname == '&')
	    {
		if (varname[1] == NUL)
		{
		    // get all window-local options in a dict
		    dict_T	*opts = get_winbuf_options(FALSE);

		    if (opts != NULL)
		    {
			rettv_dict_set(rettv, opts);
			done = TRUE;
		    }
		}
		else if (get_option_tv(&varname, rettv, 1) == OK)
		    // window-local-option
		    done = TRUE;
	    }
	    else
	    {
		// Look up the variable.
		// Let getwinvar({nr}, "") return the "w:" dictionary.
		v = find_var_in_ht(&win->w_vars->dv_hashtab, 'w',
							      varname, FALSE);
		if (v != NULL)
		{
		    copy_tv(&v->di_tv, rettv);
		    done = TRUE;
		}
	    }
	}

	if (need_switch_win)
	    // restore previous notion of curwin
	    restore_win(oldcurwin, oldtabpage, TRUE);
    }

    if (!done && argvars[off + 2].v_type != VAR_UNKNOWN)
	// use the default return value
	copy_tv(&argvars[off + 2], rettv);

    --emsg_off;
}

/*
 * "setwinvar()" and "settabwinvar()" functions
 */
    static void
setwinvar(typval_T *argvars, typval_T *rettv UNUSED, int off)
{
    win_T	*win;
    win_T	*save_curwin;
    tabpage_T	*save_curtab;
    int		need_switch_win;
    char_u	*varname, *winvarname;
    typval_T	*varp;
    char_u	nbuf[NUMBUFLEN];
    tabpage_T	*tp = NULL;

    if (check_secure())
	return;

    if (off == 1)
	tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    else
	tp = curtab;
    win = find_win_by_nr(&argvars[off], tp);
    varname = tv_get_string_chk(&argvars[off + 1]);
    varp = &argvars[off + 2];

    if (win != NULL && varname != NULL && varp != NULL)
    {
	need_switch_win = !(tp == curtab && win == curwin);
	if (!need_switch_win
	       || switch_win(&save_curwin, &save_curtab, win, tp, TRUE) == OK)
	{
	    if (*varname == '&')
	    {
		long	numval;
		char_u	*strval;
		int		error = FALSE;

		++varname;
		numval = (long)tv_get_number_chk(varp, &error);
		strval = tv_get_string_buf_chk(varp, nbuf);
		if (!error && strval != NULL)
		    set_option_value(varname, numval, strval, OPT_LOCAL);
	    }
	    else
	    {
		winvarname = alloc(STRLEN(varname) + 3);
		if (winvarname != NULL)
		{
		    STRCPY(winvarname, "w:");
		    STRCPY(winvarname + 2, varname);
		    set_var(winvarname, varp, TRUE);
		    vim_free(winvarname);
		}
	    }
	}
	if (need_switch_win)
	    restore_win(save_curwin, save_curtab, TRUE);
    }
}

    int
var_exists(char_u *var)
{
    char_u	*name;
    char_u	*tofree;
    typval_T    tv;
    int		len = 0;
    int		n = FALSE;

    // get_name_len() takes care of expanding curly braces
    name = var;
    len = get_name_len(&var, &tofree, TRUE, FALSE);
    if (len > 0)
    {
	if (tofree != NULL)
	    name = tofree;
	n = (get_var_tv(name, len, &tv, NULL, FALSE, TRUE) == OK);
	if (n)
	{
	    // handle d.key, l[idx], f(expr)
	    n = (handle_subscript(&var, &tv, TRUE, FALSE, name, &name) == OK);
	    if (n)
		clear_tv(&tv);
	}
    }
    if (*var != NUL)
	n = FALSE;

    vim_free(tofree);
    return n;
}

/*
 * "gettabvar()" function
 */
    void
f_gettabvar(typval_T *argvars, typval_T *rettv)
{
    win_T	*oldcurwin;
    tabpage_T	*tp, *oldtabpage;
    dictitem_T	*v;
    char_u	*varname;
    int		done = FALSE;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    varname = tv_get_string_chk(&argvars[1]);
    tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    if (tp != NULL && varname != NULL)
    {
	// Set tp to be our tabpage, temporarily.  Also set the window to the
	// first window in the tabpage, otherwise the window is not valid.
	if (switch_win(&oldcurwin, &oldtabpage,
		tp == curtab || tp->tp_firstwin == NULL ? firstwin
					    : tp->tp_firstwin, tp, TRUE) == OK)
	{
	    // look up the variable
	    // Let gettabvar({nr}, "") return the "t:" dictionary.
	    v = find_var_in_ht(&tp->tp_vars->dv_hashtab, 't', varname, FALSE);
	    if (v != NULL)
	    {
		copy_tv(&v->di_tv, rettv);
		done = TRUE;
	    }
	}

	// restore previous notion of curwin
	restore_win(oldcurwin, oldtabpage, TRUE);
    }

    if (!done && argvars[2].v_type != VAR_UNKNOWN)
	copy_tv(&argvars[2], rettv);
}

/*
 * "gettabwinvar()" function
 */
    void
f_gettabwinvar(typval_T *argvars, typval_T *rettv)
{
    getwinvar(argvars, rettv, 1);
}

/*
 * "getwinvar()" function
 */
    void
f_getwinvar(typval_T *argvars, typval_T *rettv)
{
    getwinvar(argvars, rettv, 0);
}

/*
 * "settabvar()" function
 */
    void
f_settabvar(typval_T *argvars, typval_T *rettv)
{
    tabpage_T	*save_curtab;
    tabpage_T	*tp;
    char_u	*varname, *tabvarname;
    typval_T	*varp;

    rettv->vval.v_number = 0;

    if (check_secure())
	return;

    tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    varname = tv_get_string_chk(&argvars[1]);
    varp = &argvars[2];

    if (varname != NULL && varp != NULL && tp != NULL)
    {
	save_curtab = curtab;
	goto_tabpage_tp(tp, FALSE, FALSE);

	tabvarname = alloc(STRLEN(varname) + 3);
	if (tabvarname != NULL)
	{
	    STRCPY(tabvarname, "t:");
	    STRCPY(tabvarname + 2, varname);
	    set_var(tabvarname, varp, TRUE);
	    vim_free(tabvarname);
	}

	// Restore current tabpage
	if (valid_tabpage(save_curtab))
	    goto_tabpage_tp(save_curtab, FALSE, FALSE);
    }
}

/*
 * "settabwinvar()" function
 */
    void
f_settabwinvar(typval_T *argvars, typval_T *rettv)
{
    setwinvar(argvars, rettv, 1);
}

/*
 * "setwinvar()" function
 */
    void
f_setwinvar(typval_T *argvars, typval_T *rettv)
{
    setwinvar(argvars, rettv, 0);
}

#endif // FEAT_EVAL
