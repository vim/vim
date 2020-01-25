/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * testing.c: Support for tests.
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Prepare "gap" for an assert error and add the sourcing position.
 */
    static void
prepare_assert_error(garray_T *gap)
{
    char    buf[NUMBUFLEN];
    char_u  *sname = estack_sfile();

    ga_init2(gap, 1, 100);
    if (sname != NULL)
    {
	ga_concat(gap, sname);
	if (SOURCING_LNUM > 0)
	    ga_concat(gap, (char_u *)" ");
    }
    if (SOURCING_LNUM > 0)
    {
	sprintf(buf, "line %ld", (long)SOURCING_LNUM);
	ga_concat(gap, (char_u *)buf);
    }
    if (sname != NULL || SOURCING_LNUM > 0)
	ga_concat(gap, (char_u *)": ");
    vim_free(sname);
}

/*
 * Append "p[clen]" to "gap", escaping unprintable characters.
 * Changes NL to \n, CR to \r, etc.
 */
    static void
ga_concat_esc(garray_T *gap, char_u *p, int clen)
{
    char_u  buf[NUMBUFLEN];

    if (clen > 1)
    {
	mch_memmove(buf, p, clen);
	buf[clen] = NUL;
	ga_concat(gap, buf);
    }
    else switch (*p)
    {
	case BS: ga_concat(gap, (char_u *)"\\b"); break;
	case ESC: ga_concat(gap, (char_u *)"\\e"); break;
	case FF: ga_concat(gap, (char_u *)"\\f"); break;
	case NL: ga_concat(gap, (char_u *)"\\n"); break;
	case TAB: ga_concat(gap, (char_u *)"\\t"); break;
	case CAR: ga_concat(gap, (char_u *)"\\r"); break;
	case '\\': ga_concat(gap, (char_u *)"\\\\"); break;
	default:
		   if (*p < ' ')
		   {
		       vim_snprintf((char *)buf, NUMBUFLEN, "\\x%02x", *p);
		       ga_concat(gap, buf);
		   }
		   else
		       ga_append(gap, *p);
		   break;
    }
}

/*
 * Append "str" to "gap", escaping unprintable characters.
 * Changes NL to \n, CR to \r, etc.
 */
    static void
ga_concat_shorten_esc(garray_T *gap, char_u *str)
{
    char_u  *p;
    char_u  *s;
    int	    c;
    int	    clen;
    char_u  buf[NUMBUFLEN];
    int	    same_len;

    if (str == NULL)
    {
	ga_concat(gap, (char_u *)"NULL");
	return;
    }

    for (p = str; *p != NUL; ++p)
    {
	same_len = 1;
	s = p;
	c = mb_ptr2char_adv(&s);
	clen = s - p;
	while (*s != NUL && c == mb_ptr2char(s))
	{
	    ++same_len;
	    s += clen;
	}
	if (same_len > 20)
	{
	    ga_concat(gap, (char_u *)"\\[");
	    ga_concat_esc(gap, p, clen);
	    ga_concat(gap, (char_u *)" occurs ");
	    vim_snprintf((char *)buf, NUMBUFLEN, "%d", same_len);
	    ga_concat(gap, buf);
	    ga_concat(gap, (char_u *)" times]");
	    p = s - 1;
	}
	else
	    ga_concat_esc(gap, p, clen);
    }
}

/*
 * Fill "gap" with information about an assert error.
 */
    static void
fill_assert_error(
    garray_T	*gap,
    typval_T	*opt_msg_tv,
    char_u      *exp_str,
    typval_T	*exp_tv,
    typval_T	*got_tv,
    assert_type_T atype)
{
    char_u	numbuf[NUMBUFLEN];
    char_u	*tofree;

    if (opt_msg_tv->v_type != VAR_UNKNOWN)
    {
	ga_concat(gap, echo_string(opt_msg_tv, &tofree, numbuf, 0));
	vim_free(tofree);
	ga_concat(gap, (char_u *)": ");
    }

    if (atype == ASSERT_MATCH || atype == ASSERT_NOTMATCH)
	ga_concat(gap, (char_u *)"Pattern ");
    else if (atype == ASSERT_NOTEQUAL)
	ga_concat(gap, (char_u *)"Expected not equal to ");
    else
	ga_concat(gap, (char_u *)"Expected ");
    if (exp_str == NULL)
    {
	ga_concat_shorten_esc(gap, tv2string(exp_tv, &tofree, numbuf, 0));
	vim_free(tofree);
    }
    else
	ga_concat_shorten_esc(gap, exp_str);
    if (atype != ASSERT_NOTEQUAL)
    {
	if (atype == ASSERT_MATCH)
	    ga_concat(gap, (char_u *)" does not match ");
	else if (atype == ASSERT_NOTMATCH)
	    ga_concat(gap, (char_u *)" does match ");
	else
	    ga_concat(gap, (char_u *)" but got ");
	ga_concat_shorten_esc(gap, tv2string(got_tv, &tofree, numbuf, 0));
	vim_free(tofree);
    }
}

    static int
assert_equal_common(typval_T *argvars, assert_type_T atype)
{
    garray_T	ga;

    if (tv_equal(&argvars[0], &argvars[1], FALSE, FALSE)
						   != (atype == ASSERT_EQUAL))
    {
	prepare_assert_error(&ga);
	fill_assert_error(&ga, &argvars[2], NULL, &argvars[0], &argvars[1],
								       atype);
	assert_error(&ga);
	ga_clear(&ga);
	return 1;
    }
    return 0;
}

    static int
assert_match_common(typval_T *argvars, assert_type_T atype)
{
    garray_T	ga;
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*pat = tv_get_string_buf_chk(&argvars[0], buf1);
    char_u	*text = tv_get_string_buf_chk(&argvars[1], buf2);

    if (pat == NULL || text == NULL)
	emsg(_(e_invarg));
    else if (pattern_match(pat, text, FALSE) != (atype == ASSERT_MATCH))
    {
	prepare_assert_error(&ga);
	fill_assert_error(&ga, &argvars[2], NULL, &argvars[0], &argvars[1],
									atype);
	assert_error(&ga);
	ga_clear(&ga);
	return 1;
    }
    return 0;
}

/*
 * Common for assert_true() and assert_false().
 * Return non-zero for failure.
 */
    static int
assert_bool(typval_T *argvars, int isTrue)
{
    int		error = FALSE;
    garray_T	ga;

    if (argvars[0].v_type == VAR_BOOL
	    && argvars[0].vval.v_number == (isTrue ? VVAL_TRUE : VVAL_FALSE))
	return 0;
    if (argvars[0].v_type != VAR_NUMBER
	    || (tv_get_number_chk(&argvars[0], &error) == 0) == isTrue
	    || error)
    {
	prepare_assert_error(&ga);
	fill_assert_error(&ga, &argvars[1],
		(char_u *)(isTrue ? "True" : "False"),
		NULL, &argvars[0], ASSERT_OTHER);
	assert_error(&ga);
	ga_clear(&ga);
	return 1;
    }
    return 0;
}

    static void
assert_append_cmd_or_arg(garray_T *gap, typval_T *argvars, char_u *cmd)
{
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];

    if (argvars[1].v_type != VAR_UNKNOWN && argvars[2].v_type != VAR_UNKNOWN)
    {
	ga_concat(gap, echo_string(&argvars[2], &tofree, numbuf, 0));
	vim_free(tofree);
    }
    else
	ga_concat(gap, cmd);
}

    static int
assert_beeps(typval_T *argvars)
{
    char_u	*cmd = tv_get_string_chk(&argvars[0]);
    garray_T	ga;
    int		ret = 0;

    called_vim_beep = FALSE;
    suppress_errthrow = TRUE;
    emsg_silent = FALSE;
    do_cmdline_cmd(cmd);
    if (!called_vim_beep)
    {
	prepare_assert_error(&ga);
	ga_concat(&ga, (char_u *)"command did not beep: ");
	ga_concat(&ga, cmd);
	assert_error(&ga);
	ga_clear(&ga);
	ret = 1;
    }

    suppress_errthrow = FALSE;
    emsg_on_display = FALSE;
    return ret;
}

/*
 * "assert_beeps(cmd [, error])" function
 */
    void
f_assert_beeps(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_beeps(argvars);
}

/*
 * "assert_equal(expected, actual[, msg])" function
 */
    void
f_assert_equal(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_equal_common(argvars, ASSERT_EQUAL);
}

    static int
assert_equalfile(typval_T *argvars)
{
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*fname1 = tv_get_string_buf_chk(&argvars[0], buf1);
    char_u	*fname2 = tv_get_string_buf_chk(&argvars[1], buf2);
    garray_T	ga;
    FILE	*fd1;
    FILE	*fd2;

    if (fname1 == NULL || fname2 == NULL)
	return 0;

    IObuff[0] = NUL;
    fd1 = mch_fopen((char *)fname1, READBIN);
    if (fd1 == NULL)
    {
	vim_snprintf((char *)IObuff, IOSIZE, (char *)e_notread, fname1);
    }
    else
    {
	fd2 = mch_fopen((char *)fname2, READBIN);
	if (fd2 == NULL)
	{
	    fclose(fd1);
	    vim_snprintf((char *)IObuff, IOSIZE, (char *)e_notread, fname2);
	}
	else
	{
	    int c1, c2;
	    long count = 0;

	    for (;;)
	    {
		c1 = fgetc(fd1);
		c2 = fgetc(fd2);
		if (c1 == EOF)
		{
		    if (c2 != EOF)
			STRCPY(IObuff, "first file is shorter");
		    break;
		}
		else if (c2 == EOF)
		{
		    STRCPY(IObuff, "second file is shorter");
		    break;
		}
		else if (c1 != c2)
		{
		    vim_snprintf((char *)IObuff, IOSIZE,
					      "difference at byte %ld", count);
		    break;
		}
		++count;
	    }
	    fclose(fd1);
	    fclose(fd2);
	}
    }
    if (IObuff[0] != NUL)
    {
	prepare_assert_error(&ga);
	ga_concat(&ga, IObuff);
	assert_error(&ga);
	ga_clear(&ga);
	return 1;
    }
    return 0;
}

/*
 * "assert_equalfile(fname-one, fname-two)" function
 */
    void
f_assert_equalfile(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_equalfile(argvars);
}

/*
 * "assert_notequal(expected, actual[, msg])" function
 */
    void
f_assert_notequal(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_equal_common(argvars, ASSERT_NOTEQUAL);
}

/*
 * "assert_exception(string[, msg])" function
 */
    void
f_assert_exception(typval_T *argvars, typval_T *rettv)
{
    garray_T	ga;
    char_u	*error = tv_get_string_chk(&argvars[0]);

    if (*get_vim_var_str(VV_EXCEPTION) == NUL)
    {
	prepare_assert_error(&ga);
	ga_concat(&ga, (char_u *)"v:exception is not set");
	assert_error(&ga);
	ga_clear(&ga);
	rettv->vval.v_number = 1;
    }
    else if (error != NULL
	&& strstr((char *)get_vim_var_str(VV_EXCEPTION), (char *)error) == NULL)
    {
	prepare_assert_error(&ga);
	fill_assert_error(&ga, &argvars[1], NULL, &argvars[0],
				  get_vim_var_tv(VV_EXCEPTION), ASSERT_OTHER);
	assert_error(&ga);
	ga_clear(&ga);
	rettv->vval.v_number = 1;
    }
}

/*
 * "assert_fails(cmd [, error[, msg]])" function
 */
    void
f_assert_fails(typval_T *argvars, typval_T *rettv)
{
    char_u	*cmd = tv_get_string_chk(&argvars[0]);
    garray_T	ga;
    int		save_trylevel = trylevel;
    int		called_emsg_before = called_emsg;

    // trylevel must be zero for a ":throw" command to be considered failed
    trylevel = 0;
    suppress_errthrow = TRUE;
    emsg_silent = TRUE;

    do_cmdline_cmd(cmd);
    if (called_emsg == called_emsg_before)
    {
	prepare_assert_error(&ga);
	ga_concat(&ga, (char_u *)"command did not fail: ");
	assert_append_cmd_or_arg(&ga, argvars, cmd);
	assert_error(&ga);
	ga_clear(&ga);
	rettv->vval.v_number = 1;
    }
    else if (argvars[1].v_type != VAR_UNKNOWN)
    {
	char_u	buf[NUMBUFLEN];
	char	*error = (char *)tv_get_string_buf_chk(&argvars[1], buf);

	if (error == NULL
		  || strstr((char *)get_vim_var_str(VV_ERRMSG), error) == NULL)
	{
	    prepare_assert_error(&ga);
	    fill_assert_error(&ga, &argvars[2], NULL, &argvars[1],
				      get_vim_var_tv(VV_ERRMSG), ASSERT_OTHER);
	    ga_concat(&ga, (char_u *)": ");
	    assert_append_cmd_or_arg(&ga, argvars, cmd);
	    assert_error(&ga);
	    ga_clear(&ga);
	    rettv->vval.v_number = 1;
	}
    }

    trylevel = save_trylevel;
    suppress_errthrow = FALSE;
    emsg_silent = FALSE;
    emsg_on_display = FALSE;
    set_vim_var_string(VV_ERRMSG, NULL, 0);
}

/*
 * "assert_false(actual[, msg])" function
 */
    void
f_assert_false(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_bool(argvars, FALSE);
}

    static int
assert_inrange(typval_T *argvars)
{
    garray_T	ga;
    int		error = FALSE;
    char_u	*tofree;
    char	msg[200];
    char_u	numbuf[NUMBUFLEN];

#ifdef FEAT_FLOAT
    if (argvars[0].v_type == VAR_FLOAT
	    || argvars[1].v_type == VAR_FLOAT
	    || argvars[2].v_type == VAR_FLOAT)
    {
	float_T flower = tv_get_float(&argvars[0]);
	float_T fupper = tv_get_float(&argvars[1]);
	float_T factual = tv_get_float(&argvars[2]);

	if (factual < flower || factual > fupper)
	{
	    prepare_assert_error(&ga);
	    if (argvars[3].v_type != VAR_UNKNOWN)
	    {
		ga_concat(&ga, tv2string(&argvars[3], &tofree, numbuf, 0));
		vim_free(tofree);
	    }
	    else
	    {
		vim_snprintf(msg, 200, "Expected range %g - %g, but got %g",
						      flower, fupper, factual);
		ga_concat(&ga, (char_u *)msg);
	    }
	    assert_error(&ga);
	    ga_clear(&ga);
	    return 1;
	}
    }
    else
#endif
    {
	varnumber_T	lower = tv_get_number_chk(&argvars[0], &error);
	varnumber_T	upper = tv_get_number_chk(&argvars[1], &error);
	varnumber_T	actual = tv_get_number_chk(&argvars[2], &error);

	if (error)
	    return 0;
	if (actual < lower || actual > upper)
	{
	    prepare_assert_error(&ga);
	    if (argvars[3].v_type != VAR_UNKNOWN)
	    {
		ga_concat(&ga, tv2string(&argvars[3], &tofree, numbuf, 0));
		vim_free(tofree);
	    }
	    else
	    {
		vim_snprintf(msg, 200, "Expected range %ld - %ld, but got %ld",
				       (long)lower, (long)upper, (long)actual);
		ga_concat(&ga, (char_u *)msg);
	    }
	    assert_error(&ga);
	    ga_clear(&ga);
	    return 1;
	}
    }
    return 0;
}

/*
 * "assert_inrange(lower, upper[, msg])" function
 */
    void
f_assert_inrange(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_inrange(argvars);
}

/*
 * "assert_match(pattern, actual[, msg])" function
 */
    void
f_assert_match(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_match_common(argvars, ASSERT_MATCH);
}

/*
 * "assert_notmatch(pattern, actual[, msg])" function
 */
    void
f_assert_notmatch(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_match_common(argvars, ASSERT_NOTMATCH);
}

/*
 * "assert_report(msg)" function
 */
    void
f_assert_report(typval_T *argvars, typval_T *rettv)
{
    garray_T	ga;

    prepare_assert_error(&ga);
    ga_concat(&ga, tv_get_string(&argvars[0]));
    assert_error(&ga);
    ga_clear(&ga);
    rettv->vval.v_number = 1;
}

/*
 * "assert_true(actual[, msg])" function
 */
    void
f_assert_true(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_bool(argvars, TRUE);
}

/*
 * "test_alloc_fail(id, countdown, repeat)" function
 */
    void
f_test_alloc_fail(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (argvars[0].v_type != VAR_NUMBER
	    || argvars[0].vval.v_number <= 0
	    || argvars[1].v_type != VAR_NUMBER
	    || argvars[1].vval.v_number < 0
	    || argvars[2].v_type != VAR_NUMBER)
	emsg(_(e_invarg));
    else
    {
	alloc_fail_id = argvars[0].vval.v_number;
	if (alloc_fail_id >= aid_last)
	    emsg(_(e_invarg));
	alloc_fail_countdown = argvars[1].vval.v_number;
	alloc_fail_repeat = argvars[2].vval.v_number;
	did_outofmem_msg = FALSE;
    }
}

/*
 * "test_autochdir()"
 */
    void
f_test_autochdir(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#if defined(FEAT_AUTOCHDIR)
    test_autochdir = TRUE;
#endif
}

/*
 * "test_feedinput()"
 */
    void
f_test_feedinput(typval_T *argvars, typval_T *rettv UNUSED)
{
#ifdef USE_INPUT_BUF
    char_u	*val = tv_get_string_chk(&argvars[0]);

    if (val != NULL)
    {
	trash_input_buf();
	add_to_input_buf_csi(val, (int)STRLEN(val));
    }
#endif
}

/*
 * "test_getvalue({name})" function
 */
    void
f_test_getvalue(typval_T *argvars, typval_T *rettv)
{
    if (argvars[0].v_type != VAR_STRING)
	emsg(_(e_invarg));
    else
    {
	char_u *name = tv_get_string(&argvars[0]);

	if (STRCMP(name, (char_u *)"need_fileinfo") == 0)
	    rettv->vval.v_number = need_fileinfo;
	else
	    semsg(_(e_invarg2), name);
    }
}

/*
 * "test_option_not_set({name})" function
 */
    void
f_test_option_not_set(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u *name = (char_u *)"";

    if (argvars[0].v_type != VAR_STRING)
	emsg(_(e_invarg));
    else
    {
	name = tv_get_string(&argvars[0]);
	if (reset_option_was_set(name) == FAIL)
	    semsg(_(e_invarg2), name);
    }
}

/*
 * "test_override({name}, {val})" function
 */
    void
f_test_override(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u *name = (char_u *)"";
    int     val;
    static int save_starting = -1;

    if (argvars[0].v_type != VAR_STRING
	    || (argvars[1].v_type) != VAR_NUMBER)
	emsg(_(e_invarg));
    else
    {
	name = tv_get_string(&argvars[0]);
	val = (int)tv_get_number(&argvars[1]);

	if (STRCMP(name, (char_u *)"redraw") == 0)
	    disable_redraw_for_testing = val;
	else if (STRCMP(name, (char_u *)"redraw_flag") == 0)
	    ignore_redraw_flag_for_testing = val;
	else if (STRCMP(name, (char_u *)"char_avail") == 0)
	    disable_char_avail_for_testing = val;
	else if (STRCMP(name, (char_u *)"starting") == 0)
	{
	    if (val)
	    {
		if (save_starting < 0)
		    save_starting = starting;
		starting = 0;
	    }
	    else
	    {
		starting = save_starting;
		save_starting = -1;
	    }
	}
	else if (STRCMP(name, (char_u *)"nfa_fail") == 0)
	    nfa_fail_for_testing = val;
	else if (STRCMP(name, (char_u *)"no_query_mouse") == 0)
	    no_query_mouse_for_testing = val;
	else if (STRCMP(name, (char_u *)"no_wait_return") == 0)
	    no_wait_return = val;
	else if (STRCMP(name, (char_u *)"ALL") == 0)
	{
	    disable_char_avail_for_testing = FALSE;
	    disable_redraw_for_testing = FALSE;
	    ignore_redraw_flag_for_testing = FALSE;
	    nfa_fail_for_testing = FALSE;
	    no_query_mouse_for_testing = FALSE;
	    if (save_starting >= 0)
	    {
		starting = save_starting;
		save_starting = -1;
	    }
	}
	else
	    semsg(_(e_invarg2), name);
    }
}

/*
 * "test_refcount({expr})" function
 */
    void
f_test_refcount(typval_T *argvars, typval_T *rettv)
{
    int retval = -1;

    switch (argvars[0].v_type)
    {
	case VAR_UNKNOWN:
	case VAR_VOID:
	case VAR_NUMBER:
	case VAR_BOOL:
	case VAR_FLOAT:
	case VAR_SPECIAL:
	case VAR_STRING:
	    break;
	case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
	    if (argvars[0].vval.v_job != NULL)
		retval = argvars[0].vval.v_job->jv_refcount - 1;
#endif
	    break;
	case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    if (argvars[0].vval.v_channel != NULL)
		retval = argvars[0].vval.v_channel->ch_refcount - 1;
#endif
	    break;
	case VAR_FUNC:
	    if (argvars[0].vval.v_string != NULL)
	    {
		ufunc_T *fp;

		fp = find_func(argvars[0].vval.v_string, NULL);
		if (fp != NULL)
		    retval = fp->uf_refcount;
	    }
	    break;
	case VAR_PARTIAL:
	    if (argvars[0].vval.v_partial != NULL)
		retval = argvars[0].vval.v_partial->pt_refcount - 1;
	    break;
	case VAR_BLOB:
	    if (argvars[0].vval.v_blob != NULL)
		retval = argvars[0].vval.v_blob->bv_refcount - 1;
	    break;
	case VAR_LIST:
	    if (argvars[0].vval.v_list != NULL)
		retval = argvars[0].vval.v_list->lv_refcount - 1;
	    break;
	case VAR_DICT:
	    if (argvars[0].vval.v_dict != NULL)
		retval = argvars[0].vval.v_dict->dv_refcount - 1;
	    break;
    }

    rettv->v_type = VAR_NUMBER;
    rettv->vval.v_number = retval;

}

/*
 * "test_garbagecollect_now()" function
 */
    void
f_test_garbagecollect_now(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    // This is dangerous, any Lists and Dicts used internally may be freed
    // while still in use.
    garbage_collect(TRUE);
}

/*
 * "test_garbagecollect_soon()" function
 */
    void
f_test_garbagecollect_soon(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    may_garbage_collect = TRUE;
}

/*
 * "test_ignore_error()" function
 */
    void
f_test_ignore_error(typval_T *argvars, typval_T *rettv UNUSED)
{
     ignore_error_for_testing(tv_get_string(&argvars[0]));
}

    void
f_test_null_blob(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_BLOB;
    rettv->vval.v_blob = NULL;
}

#ifdef FEAT_JOB_CHANNEL
    void
f_test_null_channel(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_CHANNEL;
    rettv->vval.v_channel = NULL;
}
#endif

    void
f_test_null_dict(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv_dict_set(rettv, NULL);
}

#ifdef FEAT_JOB_CHANNEL
    void
f_test_null_job(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_JOB;
    rettv->vval.v_job = NULL;
}
#endif

    void
f_test_null_list(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv_list_set(rettv, NULL);
}

    void
f_test_null_partial(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_PARTIAL;
    rettv->vval.v_partial = NULL;
}

    void
f_test_null_string(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
}

#ifdef FEAT_GUI
    void
f_test_scrollbar(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u	*which;
    long	value;
    int		dragging;
    scrollbar_T *sb = NULL;

    if (argvars[0].v_type != VAR_STRING
	    || (argvars[1].v_type) != VAR_NUMBER
	    || (argvars[2].v_type) != VAR_NUMBER)
    {
	emsg(_(e_invarg));
	return;
    }
    which = tv_get_string(&argvars[0]);
    value = tv_get_number(&argvars[1]);
    dragging = tv_get_number(&argvars[2]);

    if (STRCMP(which, "left") == 0)
	sb = &curwin->w_scrollbars[SBAR_LEFT];
    else if (STRCMP(which, "right") == 0)
	sb = &curwin->w_scrollbars[SBAR_RIGHT];
    else if (STRCMP(which, "hor") == 0)
	sb = &gui.bottom_sbar;
    if (sb == NULL)
    {
	semsg(_(e_invarg2), which);
	return;
    }
    gui_drag_scrollbar(sb, value, dragging);
# ifndef USE_ON_FLY_SCROLL
    // need to loop through normal_cmd() to handle the scroll events
    exec_normal(FALSE, TRUE, FALSE);
# endif
}
#endif

    void
f_test_setmouse(typval_T *argvars, typval_T *rettv UNUSED)
{
    mouse_row = (time_t)tv_get_number(&argvars[0]) - 1;
    mouse_col = (time_t)tv_get_number(&argvars[1]) - 1;
}

    void
f_test_settime(typval_T *argvars, typval_T *rettv UNUSED)
{
    time_for_testing = (time_t)tv_get_number(&argvars[0]);
}


#endif // defined(FEAT_EVAL)
