/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * optionstr.c: Functions related to string options
 */

#include "vim.h"

static char_u shm_buf[SHM_LEN];
static int set_shm_recursive = 0;

static char *(p_ambw_values[]) = {"single", "double", NULL};
static char *(p_bg_values[]) = {"light", "dark", NULL};
static char *(p_bkc_values[]) = {"yes", "auto", "no", "breaksymlink", "breakhardlink", NULL};
static char *(p_bo_values[]) = {"all", "backspace", "cursor", "complete",
				 "copy", "ctrlg", "error", "esc", "ex",
				 "hangul", "insertmode", "lang", "mess",
				 "showmatch", "operator", "register", "shell",
				 "spell", "term", "wildmode", NULL};
static char *(p_nf_values[]) = {"bin", "octal", "hex", "alpha", "unsigned", NULL};
static char *(p_ff_values[]) = {FF_UNIX, FF_DOS, FF_MAC, NULL};
#ifdef FEAT_CRYPT
static char *(p_cm_values[]) = {"zip", "blowfish", "blowfish2",
 # ifdef FEAT_SODIUM
    "xchacha20",
 # endif
    NULL};
#endif
static char *(p_cmp_values[]) = {"internal", "keepascii", NULL};
static char *(p_dy_values[]) = {"lastline", "truncate", "uhex", NULL};
#ifdef FEAT_FOLDING
static char *(p_fdo_values[]) = {"all", "block", "hor", "mark", "percent",
				 "quickfix", "search", "tag", "insert",
				 "undo", "jump", NULL};
#endif
#ifdef FEAT_SESSION
// Also used for 'viewoptions'!  Keep in sync with SSOP_ flags.
static char *(p_ssop_values[]) = {"buffers", "winpos", "resize", "winsize",
    "localoptions", "options", "help", "blank", "globals", "slash", "unix",
    "sesdir", "curdir", "folds", "cursor", "tabpages", "terminal", "skiprtp",
    NULL};
#endif
// Keep in sync with SWB_ flags in option.h
static char *(p_swb_values[]) = {"useopen", "usetab", "split", "newtab", "vsplit", "uselast", NULL};
static char *(p_spk_values[]) = {"cursor", "screen", "topline", NULL};
static char *(p_tc_values[]) = {"followic", "ignore", "match", "followscs", "smart", NULL};
#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_MSWIN)
static char *(p_toolbar_values[]) = {"text", "icons", "tooltips", "horiz", NULL};
#endif
#if defined(FEAT_TOOLBAR) && defined(FEAT_GUI_GTK)
static char *(p_tbis_values[]) = {"tiny", "small", "medium", "large", "huge", "giant", NULL};
#endif
#if defined(UNIX) || defined(VMS)
static char *(p_ttym_values[]) = {"xterm", "xterm2", "dec", "netterm", "jsbterm", "pterm", "urxvt", "sgr", NULL};
#endif
static char *(p_ve_values[]) = {"block", "insert", "all", "onemore", "none", "NONE", NULL};
static char *(p_wop_values[]) = {"fuzzy", "tagfile", "pum", NULL};
#ifdef FEAT_WAK
static char *(p_wak_values[]) = {"yes", "menu", "no", NULL};
#endif
static char *(p_mousem_values[]) = {"extend", "popup", "popup_setpos", "mac", NULL};
static char *(p_sel_values[]) = {"inclusive", "exclusive", "old", NULL};
static char *(p_slm_values[]) = {"mouse", "key", "cmd", NULL};
static char *(p_km_values[]) = {"startsel", "stopsel", NULL};
#ifdef FEAT_BROWSE
static char *(p_bsdir_values[]) = {"current", "last", "buffer", NULL};
#endif
static char *(p_scbopt_values[]) = {"ver", "hor", "jump", NULL};
static char *(p_debug_values[]) = {"msg", "throw", "beep", NULL};
static char *(p_ead_values[]) = {"both", "ver", "hor", NULL};
static char *(p_buftype_values[]) = {"nofile", "nowrite", "quickfix", "help", "terminal", "acwrite", "prompt", "popup", NULL};
static char *(p_bufhidden_values[]) = {"hide", "unload", "delete", "wipe", NULL};
static char *(p_bs_values[]) = {"indent", "eol", "start", "nostop", NULL};
#ifdef FEAT_FOLDING
static char *(p_fdm_values[]) = {"manual", "expr", "marker", "indent", "syntax",
# ifdef FEAT_DIFF
				"diff",
# endif
				NULL};
static char *(p_fcl_values[]) = {"all", NULL};
#endif
static char *(p_cot_values[]) = {"menu", "menuone", "longest", "preview", "popup", "popuphidden", "noinsert", "noselect", NULL};
#ifdef BACKSLASH_IN_FILENAME
static char *(p_csl_values[]) = {"slash", "backslash", NULL};
#endif
#ifdef FEAT_SIGNS
static char *(p_scl_values[]) = {"yes", "no", "auto", "number", NULL};
#endif
#if defined(MSWIN) && defined(FEAT_TERMINAL)
static char *(p_twt_values[]) = {"winpty", "conpty", "", NULL};
#endif
static char *(p_sloc_values[]) = {"last", "statusline", "tabline", NULL};

static int check_opt_strings(char_u *val, char **values, int list);
static int opt_strings_flags(char_u *val, char **values, unsigned *flagp, int list);

/*
 * After setting various option values: recompute variables that depend on
 * option values.
 */
    void
didset_string_options(void)
{
    (void)opt_strings_flags(p_cmp, p_cmp_values, &cmp_flags, TRUE);
    (void)opt_strings_flags(p_bkc, p_bkc_values, &bkc_flags, TRUE);
    (void)opt_strings_flags(p_bo, p_bo_values, &bo_flags, TRUE);
#ifdef FEAT_SESSION
    (void)opt_strings_flags(p_ssop, p_ssop_values, &ssop_flags, TRUE);
    (void)opt_strings_flags(p_vop, p_ssop_values, &vop_flags, TRUE);
#endif
#ifdef FEAT_FOLDING
    (void)opt_strings_flags(p_fdo, p_fdo_values, &fdo_flags, TRUE);
#endif
    (void)opt_strings_flags(p_dy, p_dy_values, &dy_flags, TRUE);
    (void)opt_strings_flags(p_tc, p_tc_values, &tc_flags, FALSE);
    (void)opt_strings_flags(p_ve, p_ve_values, &ve_flags, TRUE);
#if defined(UNIX) || defined(VMS)
    (void)opt_strings_flags(p_ttym, p_ttym_values, &ttym_flags, FALSE);
#endif
#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_MSWIN)
    (void)opt_strings_flags(p_toolbar, p_toolbar_values, &toolbar_flags, TRUE);
#endif
#if defined(FEAT_TOOLBAR) && defined(FEAT_GUI_GTK)
    (void)opt_strings_flags(p_tbis, p_tbis_values, &tbis_flags, FALSE);
#endif
    (void)opt_strings_flags(p_swb, p_swb_values, &swb_flags, TRUE);
}

#if defined(FEAT_EVAL)
/*
 * Trigger the OptionSet autocommand.
 * "opt_idx"	is the index of the option being set.
 * "opt_flags"	can be OPT_LOCAL etc.
 * "oldval"	the old value
 *  "oldval_l"  the old local value (only non-NULL if global and local value
 *		are set)
 * "oldval_g"   the old global value (only non-NULL if global and local value
 *		are set)
 * "newval"	the new value
 */
    void
trigger_optionset_string(
	int	opt_idx,
	int	opt_flags,
	char_u  *oldval,
	char_u  *oldval_l,
	char_u  *oldval_g,
	char_u  *newval)
{
    // Don't do this recursively.
    if (oldval == NULL || newval == NULL
				    || *get_vim_var_str(VV_OPTION_TYPE) != NUL)
	return;

    char_u buf_type[7];

    sprintf((char *)buf_type, "%s",
	    (opt_flags & OPT_LOCAL) ? "local" : "global");
    set_vim_var_string(VV_OPTION_OLD, oldval, -1);
    set_vim_var_string(VV_OPTION_NEW, newval, -1);
    set_vim_var_string(VV_OPTION_TYPE, buf_type, -1);
    if (opt_flags & OPT_LOCAL)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"setlocal", -1);
	set_vim_var_string(VV_OPTION_OLDLOCAL, oldval, -1);
    }
    if (opt_flags & OPT_GLOBAL)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"setglobal", -1);
	set_vim_var_string(VV_OPTION_OLDGLOBAL, oldval, -1);
    }
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"set", -1);
	set_vim_var_string(VV_OPTION_OLDLOCAL, oldval_l, -1);
	set_vim_var_string(VV_OPTION_OLDGLOBAL, oldval_g, -1);
    }
    if (opt_flags & OPT_MODELINE)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"modeline", -1);
	set_vim_var_string(VV_OPTION_OLDLOCAL, oldval, -1);
    }
    apply_autocmds(EVENT_OPTIONSET,
	    get_option_fullname(opt_idx), NULL, FALSE,
	    NULL);
    reset_v_option_vars();
}
#endif

    static char *
illegal_char(char *errbuf, int c)
{
    if (errbuf == NULL)
	return "";
    sprintf((char *)errbuf, _(e_illegal_character_str), (char *)transchar(c));
    return errbuf;
}

/*
 * Check string options in a buffer for NULL value.
 */
    void
check_buf_options(buf_T *buf)
{
    check_string_option(&buf->b_p_bh);
    check_string_option(&buf->b_p_bt);
    check_string_option(&buf->b_p_fenc);
    check_string_option(&buf->b_p_ff);
#ifdef FEAT_FIND_ID
    check_string_option(&buf->b_p_def);
    check_string_option(&buf->b_p_inc);
# ifdef FEAT_EVAL
    check_string_option(&buf->b_p_inex);
# endif
#endif
#if defined(FEAT_EVAL)
    check_string_option(&buf->b_p_inde);
    check_string_option(&buf->b_p_indk);
#endif
#if defined(FEAT_BEVAL) && defined(FEAT_EVAL)
    check_string_option(&buf->b_p_bexpr);
#endif
#if defined(FEAT_CRYPT)
    check_string_option(&buf->b_p_cm);
#endif
    check_string_option(&buf->b_p_fp);
#if defined(FEAT_EVAL)
    check_string_option(&buf->b_p_fex);
#endif
#ifdef FEAT_CRYPT
    check_string_option(&buf->b_p_key);
#endif
    check_string_option(&buf->b_p_kp);
    check_string_option(&buf->b_p_mps);
    check_string_option(&buf->b_p_fo);
    check_string_option(&buf->b_p_flp);
    check_string_option(&buf->b_p_isk);
    check_string_option(&buf->b_p_com);
#ifdef FEAT_FOLDING
    check_string_option(&buf->b_p_cms);
#endif
    check_string_option(&buf->b_p_nf);
    check_string_option(&buf->b_p_qe);
#ifdef FEAT_SYN_HL
    check_string_option(&buf->b_p_syn);
    check_string_option(&buf->b_s.b_syn_isk);
#endif
#ifdef FEAT_SPELL
    check_string_option(&buf->b_s.b_p_spc);
    check_string_option(&buf->b_s.b_p_spf);
    check_string_option(&buf->b_s.b_p_spl);
    check_string_option(&buf->b_s.b_p_spo);
#endif
    check_string_option(&buf->b_p_sua);
    check_string_option(&buf->b_p_cink);
    check_string_option(&buf->b_p_cino);
    check_string_option(&buf->b_p_cinsd);
    parse_cino(buf);
    check_string_option(&buf->b_p_lop);
    check_string_option(&buf->b_p_ft);
    check_string_option(&buf->b_p_cinw);
    check_string_option(&buf->b_p_cpt);
#ifdef FEAT_COMPL_FUNC
    check_string_option(&buf->b_p_cfu);
    check_string_option(&buf->b_p_ofu);
    check_string_option(&buf->b_p_tsrfu);
#endif
#ifdef FEAT_EVAL
    check_string_option(&buf->b_p_tfu);
#endif
#ifdef FEAT_KEYMAP
    check_string_option(&buf->b_p_keymap);
#endif
#ifdef FEAT_QUICKFIX
    check_string_option(&buf->b_p_gp);
    check_string_option(&buf->b_p_mp);
    check_string_option(&buf->b_p_efm);
#endif
    check_string_option(&buf->b_p_ep);
    check_string_option(&buf->b_p_path);
    check_string_option(&buf->b_p_tags);
    check_string_option(&buf->b_p_tc);
    check_string_option(&buf->b_p_dict);
    check_string_option(&buf->b_p_tsr);
    check_string_option(&buf->b_p_lw);
    check_string_option(&buf->b_p_bkc);
    check_string_option(&buf->b_p_menc);
#ifdef FEAT_VARTABS
    check_string_option(&buf->b_p_vsts);
    check_string_option(&buf->b_p_vts);
#endif
}

/*
 * Free the string allocated for an option.
 * Checks for the string being empty_option. This may happen if we're out of
 * memory, vim_strsave() returned NULL, which was replaced by empty_option by
 * check_options().
 * Does NOT check for P_ALLOCED flag!
 */
    void
free_string_option(char_u *p)
{
    if (p != empty_option)
	vim_free(p);
}

    void
clear_string_option(char_u **pp)
{
    if (*pp != empty_option)
	vim_free(*pp);
    *pp = empty_option;
}

    void
check_string_option(char_u **pp)
{
    if (*pp == NULL)
	*pp = empty_option;
}

/*
 * Set global value for string option when it's a local option.
 */
    static void
set_string_option_global(
    int		opt_idx,	// option index
    char_u	**varp)		// pointer to option variable
{
    char_u	**p, *s;

    // the global value is always allocated
    if (is_window_local_option(opt_idx))
	p = (char_u **)GLOBAL_WO(varp);
    else
	p = (char_u **)get_option_var(opt_idx);
    if (!is_global_option(opt_idx)
	    && p != varp
	    && (s = vim_strsave(*varp)) != NULL)
    {
	free_string_option(*p);
	*p = s;
    }
}

/*
 * Set a string option to a new value (without checking the effect).
 * The string is copied into allocated memory.
 * if ("opt_idx" == -1) "name" is used, otherwise "opt_idx" is used.
 * When "set_sid" is zero set the scriptID to current_sctx.sc_sid.  When
 * "set_sid" is SID_NONE don't set the scriptID.  Otherwise set the scriptID to
 * "set_sid".
 */
    void
set_string_option_direct(
    char_u	*name,
    int		opt_idx,
    char_u	*val,
    int		opt_flags,	// OPT_FREE, OPT_LOCAL and/or OPT_GLOBAL
    int		set_sid UNUSED)
{
    char_u	*s;
    char_u	**varp;
    int		both = (opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0;
    int		idx = opt_idx;

    if (idx == -1)		// use name
    {
	idx = findoption(name);
	if (idx < 0)	// not found (should not happen)
	{
	    semsg(_(e_internal_error_str), "set_string_option_direct()");
	    siemsg(_("For option %s"), name);
	    return;
	}
    }

    if (is_hidden_option(idx))		// can't set hidden option
	return;

    s = vim_strsave(val);
    if (s == NULL)
	return;

    varp = (char_u **)get_option_varp_scope(idx,
	    both ? OPT_LOCAL : opt_flags);
    if ((opt_flags & OPT_FREE) && (get_option_flags(idx) & P_ALLOCED))
	free_string_option(*varp);
    *varp = s;

    // For buffer/window local option may also set the global value.
    if (both)
	set_string_option_global(idx, varp);

    set_option_flag(idx, P_ALLOCED);

    // When setting both values of a global option with a local value,
    // make the local value empty, so that the global value is used.
    if (is_global_local_option(idx) && both)
    {
	free_string_option(*varp);
	*varp = empty_option;
    }
# ifdef FEAT_EVAL
    if (set_sid != SID_NONE)
    {
	sctx_T script_ctx;

	if (set_sid == 0)
	    script_ctx = current_sctx;
	else
	{
	    script_ctx.sc_sid = set_sid;
	    script_ctx.sc_seq = 0;
	    script_ctx.sc_lnum = 0;
	    script_ctx.sc_version = 1;
	}
	set_option_sctx_idx(idx, opt_flags, script_ctx);
    }
# endif
}

/*
 * Like set_string_option_direct(), but for a window-local option in "wp".
 * Blocks autocommands to avoid the old curwin becoming invalid.
 */
    void
set_string_option_direct_in_win(
	win_T		*wp,
	char_u		*name,
	int		opt_idx,
	char_u		*val,
	int		opt_flags,
	int		set_sid)
{
    win_T	*save_curwin = curwin;

    block_autocmds();
    curwin = wp;
    curbuf = curwin->w_buffer;
    set_string_option_direct(name, opt_idx, val, opt_flags, set_sid);
    curwin = save_curwin;
    curbuf = curwin->w_buffer;
    unblock_autocmds();
}

#if defined(FEAT_PROP_POPUP) || defined(PROTO)
/*
 * Like set_string_option_direct(), but for a buffer-local option in "buf".
 * Blocks autocommands to avoid the old curbuf becoming invalid.
 */
    void
set_string_option_direct_in_buf(
	buf_T		*buf,
	char_u		*name,
	int		opt_idx,
	char_u		*val,
	int		opt_flags,
	int		set_sid)
{
    buf_T	*save_curbuf = curbuf;

    block_autocmds();
    curbuf = buf;
    curwin->w_buffer = curbuf;
    set_string_option_direct(name, opt_idx, val, opt_flags, set_sid);
    curbuf = save_curbuf;
    curwin->w_buffer = curbuf;
    unblock_autocmds();
}
#endif

/*
 * Set a string option to a new value, and handle the effects.
 *
 * Returns NULL on success or an untranslated error message on error.
 */
    char *
set_string_option(
    int		opt_idx,
    char_u	*value,
    int		opt_flags)	// OPT_LOCAL and/or OPT_GLOBAL
{
    char_u	*s;
    char_u	**varp;
    char_u	*oldval;
#if defined(FEAT_EVAL)
    char_u	*oldval_l = NULL;
    char_u	*oldval_g = NULL;
    char_u	*saved_oldval = NULL;
    char_u	*saved_oldval_l = NULL;
    char_u	*saved_oldval_g = NULL;
    char_u	*saved_newval = NULL;
#endif
    char	*errmsg = NULL;
    int		value_checked = FALSE;

    if (is_hidden_option(opt_idx))	// don't set hidden option
	return NULL;

    s = vim_strsave(value == NULL ? (char_u *)"" : value);
    if (s == NULL)
	return NULL;

    varp = (char_u **)get_option_varp_scope(opt_idx,
	    (opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0
	    ? (is_global_local_option(opt_idx)
		? OPT_GLOBAL : OPT_LOCAL)
	    : opt_flags);
    oldval = *varp;
#if defined(FEAT_EVAL)
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
    {
	oldval_l = *(char_u **)get_option_varp_scope(opt_idx, OPT_LOCAL);
	oldval_g = *(char_u **)get_option_varp_scope(opt_idx, OPT_GLOBAL);
    }
#endif
    *varp = s;

#if defined(FEAT_EVAL)
    if (!starting
# ifdef FEAT_CRYPT
	    && !is_crypt_key_option(opt_idx)
# endif
       )
    {
	if (oldval_l != NULL)
	    saved_oldval_l = vim_strsave(oldval_l);
	if (oldval_g != NULL)
	    saved_oldval_g = vim_strsave(oldval_g);
	saved_oldval = vim_strsave(oldval);
	saved_newval = vim_strsave(s);
    }
#endif
    if ((errmsg = did_set_string_option(opt_idx, varp, oldval, NULL,
		    opt_flags, &value_checked)) == NULL)
	did_set_option(opt_idx, opt_flags, TRUE, value_checked);

#if defined(FEAT_EVAL)
    // call autocommand after handling side effects
    if (errmsg == NULL)
	trigger_optionset_string(opt_idx, opt_flags,
		saved_oldval, saved_oldval_l,
		saved_oldval_g, saved_newval);
    vim_free(saved_oldval);
    vim_free(saved_oldval_l);
    vim_free(saved_oldval_g);
    vim_free(saved_newval);
#endif
    return errmsg;
}

/*
 * Return TRUE if "val" is a valid 'filetype' name.
 * Also used for 'syntax' and 'keymap'.
 */
    static int
valid_filetype(char_u *val)
{
    return valid_name(val, ".-_");
}

#ifdef FEAT_STL_OPT
/*
 * Check validity of options with the 'statusline' format.
 * Return an untranslated error message or NULL.
 */
    static char *
check_stl_option(char_u *s)
{
    int		groupdepth = 0;
    static char errbuf[80];

    while (*s)
    {
	// Check for valid keys after % sequences
	while (*s && *s != '%')
	    s++;
	if (!*s)
	    break;
	s++;
	if (*s == '%' || *s == STL_TRUNCMARK || *s == STL_MIDDLEMARK)
	{
	    s++;
	    continue;
	}
	if (*s == ')')
	{
	    s++;
	    if (--groupdepth < 0)
		break;
	    continue;
	}
	if (*s == '-')
	    s++;
	while (VIM_ISDIGIT(*s))
	    s++;
	if (*s == STL_USER_HL)
	    continue;
	if (*s == '.')
	{
	    s++;
	    while (*s && VIM_ISDIGIT(*s))
		s++;
	}
	if (*s == '(')
	{
	    groupdepth++;
	    continue;
	}
	if (vim_strchr(STL_ALL, *s) == NULL)
	{
	    return illegal_char(errbuf, *s);
	}
	if (*s == '{')
	{
	    int reevaluate = (*++s == '%');

	    if (reevaluate && *++s == '}')
		// "}" is not allowed immediately after "%{%"
		return illegal_char(errbuf, '}');
	    while ((*s != '}' || (reevaluate && s[-1] != '%')) && *s)
		s++;
	    if (*s != '}')
		return e_unclosed_expression_sequence;
	}
    }
    if (groupdepth != 0)
	return e_unbalanced_groups;
    return NULL;
}
#endif

/*
 * Check for a "normal" directory or file name in some options.  Disallow a
 * path separator (slash and/or backslash), wildcards and characters that are
 * often illegal in a file name. Be more permissive if "secure" is off.
 */
    static int
check_illegal_path_names(int opt_idx, char_u **varp)
{
    return (((get_option_flags(opt_idx) & P_NFNAME)
		&& vim_strpbrk(*varp, (char_u *)(secure
			? "/\\*?[|;&<>\r\n" : "/\\*?[<>\r\n")) != NULL)
	    || ((get_option_flags(opt_idx) & P_NDNAME)
		&& vim_strpbrk(*varp, (char_u *)"*?[|;&<>\r\n") != NULL));
}

/*
 * The 'term' option is changed.
 */
    static char *
did_set_term(int *opt_idx, long_u *free_oldval)
{
    char *errmsg = NULL;

    if (T_NAME[0] == NUL)
	errmsg = e_cannot_set_term_to_empty_string;
#ifdef FEAT_GUI
    else if (gui.in_use)
	errmsg = e_cannot_change_term_in_GUI;
    else if (term_is_gui(T_NAME))
	errmsg = e_use_gui_to_start_GUI;
#endif
    else if (set_termname(T_NAME) == FAIL)
	errmsg = e_not_found_in_termcap;
    else
    {
	// Screen colors may have changed.
	redraw_later_clear();

	// Both 'term' and 'ttytype' point to T_NAME, only set the
	// P_ALLOCED flag on 'term'.
	*opt_idx = findoption((char_u *)"term");
	*free_oldval = (get_option_flags(*opt_idx) & P_ALLOCED);
    }

    return errmsg;
}

/*
 * The 'backupcopy' option is changed.
 */
    static char *
did_set_backupcopy(
    char_u	*oldval,
    int		opt_flags)
{
    char_u		*bkc = p_bkc;
    unsigned int	*flags = &bkc_flags;
    char		*errmsg = NULL;

    if (opt_flags & OPT_LOCAL)
    {
	bkc = curbuf->b_p_bkc;
	flags = &curbuf->b_bkc_flags;
    }

    if ((opt_flags & OPT_LOCAL) && *bkc == NUL)
	// make the local value empty: use the global value
	*flags = 0;
    else
    {
	if (opt_strings_flags(bkc, p_bkc_values, flags, TRUE) != OK)
	    errmsg = e_invalid_argument;
	if ((((int)*flags & BKC_AUTO) != 0)
		+ (((int)*flags & BKC_YES) != 0)
		+ (((int)*flags & BKC_NO) != 0) != 1)
	{
	    // Must have exactly one of "auto", "yes"  and "no".
	    (void)opt_strings_flags(oldval, p_bkc_values, flags, TRUE);
	    errmsg = e_invalid_argument;
	}
    }

    return errmsg;
}

/*
 * The 'backupext' or the 'patchmode' option is changed.
 */
    static char *
did_set_backupext_or_patchmode(void)
{
    if (STRCMP(*p_bex == '.' ? p_bex + 1 : p_bex,
		*p_pm == '.' ? p_pm + 1 : p_pm) == 0)
	return e_backupext_and_patchmode_are_equal;

    return NULL;
}

#ifdef FEAT_LINEBREAK
/*
 * The 'breakindentopt' option is changed.
 */
    static char *
did_set_breakindentopt(void)
{
    char *errmsg = NULL;

    if (briopt_check(curwin) == FAIL)
	errmsg = e_invalid_argument;
    // list setting requires a redraw
    if (curwin->w_briopt_list)
	redraw_all_later(UPD_NOT_VALID);

    return errmsg;
}
#endif

/*
 * The 'isident' or the 'iskeyword' or the 'isprint' or the 'isfname' option is
 * changed.
 */
    static char *
did_set_isopt(int *did_chartab)
{
    // 'isident', 'iskeyword', 'isprint or 'isfname' option: refill g_chartab[]
    // If the new option is invalid, use old value.
    // 'lisp' option: refill g_chartab[] for '-' char.
    if (init_chartab() == FAIL)
    {
	*did_chartab = TRUE;		// need to restore it below
	return e_invalid_argument;	// error in value
    }

    return NULL;
}

/*
 * The 'helpfile' option is changed.
 */
    static void
did_set_helpfile(void)
{
    // May compute new values for $VIM and $VIMRUNTIME
    if (didset_vim)
	vim_unsetenv_ext((char_u *)"VIM");
    if (didset_vimruntime)
	vim_unsetenv_ext((char_u *)"VIMRUNTIME");
}

#ifdef FEAT_SYN_HL
/*
 * The 'cursorlineopt' option is changed.
 */
    static char *
did_set_cursorlineopt(char_u **varp)
{
    if (**varp == NUL || fill_culopt_flags(*varp, curwin) != OK)
	return e_invalid_argument;

    return NULL;
}
#endif

#ifdef FEAT_MULTI_LANG
/*
 * The 'helplang' option is changed.
 */
    static char *
did_set_helplang(void)
{
    char *errmsg = NULL;

    // Check for "", "ab", "ab,cd", etc.
    for (char_u *s = p_hlg; *s != NUL; s += 3)
    {
	if (s[1] == NUL || ((s[2] != ',' || s[3] == NUL) && s[2] != NUL))
	{
	    errmsg = e_invalid_argument;
	    break;
	}
	if (s[2] == NUL)
	    break;
    }

    return errmsg;
}
#endif

/*
 * The 'highlight' option is changed.
 */
    static char *
did_set_highlight(void)
{
    if (highlight_changed() == FAIL)
	return e_invalid_argument;	// invalid flags

    return NULL;
}

/*
 * An option that accepts a list of flags is changed.
 * e.g. 'viewoptions', 'switchbuf', 'casemap', etc.
 */
    static char *
did_set_opt_flags(char_u *val, char **values, unsigned *flagp, int list)
{
    if (opt_strings_flags(val, values, flagp, list) == FAIL)
	return e_invalid_argument;

    return NULL;
}

/*
 * An option that accepts a list of string values is changed.
 * e.g. 'nrformats', 'scrollopt', 'wildoptions', etc.
 */
    static char *
did_set_opt_strings(char_u *val, char **values, int list)
{
    return did_set_opt_flags(val, values, NULL, list);
}

#ifdef FEAT_SESSION
/*
 * The 'sessionoptions' option is changed.
 */
    static char *
did_set_sessionoptions(char_u *oldval)
{
    if (opt_strings_flags(p_ssop, p_ssop_values, &ssop_flags, TRUE) != OK)
	return e_invalid_argument;
    if ((ssop_flags & SSOP_CURDIR) && (ssop_flags & SSOP_SESDIR))
    {
	// Don't allow both "sesdir" and "curdir".
	(void)opt_strings_flags(oldval, p_ssop_values, &ssop_flags, TRUE);
	return e_invalid_argument;
    }

    return NULL;
}
#endif

/*
 * The 'ambiwidth' option is changed.
 */
    static char *
did_set_ambiwidth(void)
{
    if (check_opt_strings(p_ambw, p_ambw_values, FALSE) != OK)
	return e_invalid_argument;

    return check_chars_options();
}

/*
 * The 'background' option is changed.
 */
    static char *
did_set_background(void)
{
    if (check_opt_strings(p_bg, p_bg_values, FALSE) == FAIL)
	return e_invalid_argument;

#ifdef FEAT_EVAL
    int dark = (*p_bg == 'd');
#endif

    init_highlight(FALSE, FALSE);

#ifdef FEAT_EVAL
    if (dark != (*p_bg == 'd')
	    && get_var_value((char_u *)"g:colors_name") != NULL)
    {
	// The color scheme must have set 'background' back to another
	// value, that's not what we want here.  Disable the color
	// scheme and set the colors again.
	do_unlet((char_u *)"g:colors_name", TRUE);
	free_string_option(p_bg);
	p_bg = vim_strsave((char_u *)(dark ? "dark" : "light"));
	check_string_option(&p_bg);
	init_highlight(FALSE, FALSE);
    }
#endif
#ifdef FEAT_TERMINAL
    term_update_colors_all();
#endif

    return NULL;
}

/*
 * The 'wildmode' option is changed.
 */
    static char *
did_set_wildmode(void)
{
    if (check_opt_wim() == FAIL)
	return e_invalid_argument;
    return NULL;
}

#ifdef FEAT_WAK
/*
 * The 'winaltkeys' option is changed.
 */
    static char *
did_set_winaltkeys(void)
{
    char *errmsg = NULL;

    if (*p_wak == NUL
	    || check_opt_strings(p_wak, p_wak_values, FALSE) != OK)
	errmsg = e_invalid_argument;
# ifdef FEAT_MENU
#  if defined(FEAT_GUI_MOTIF)
    else if (gui.in_use)
	gui_motif_set_mnemonics(p_wak[0] == 'y' || p_wak[0] == 'm');
#  elif defined(FEAT_GUI_GTK)
    else if (gui.in_use)
	gui_gtk_set_mnemonics(p_wak[0] == 'y' || p_wak[0] == 'm');
#  endif
# endif
    return errmsg;
}
#endif

/*
 * The 'eventignore' option is changed.
 */
    static char *
did_set_eventignore(void)
{
    if (check_ei() == FAIL)
	return e_invalid_argument;
    return NULL;
}

/*
 * One of the 'encoding', 'fileencoding', 'termencoding' or 'makeencoding'
 * options is changed.
 */
    static char *
did_set_encoding(char_u **varp, char_u **gvarp, int opt_flags)
{
    char	*errmsg = NULL;
    char_u	*p;

    if (gvarp == &p_fenc)
    {
	if (!curbuf->b_p_ma && opt_flags != OPT_GLOBAL)
	    errmsg = e_cannot_make_changes_modifiable_is_off;
	else if (vim_strchr(*varp, ',') != NULL)
	    // No comma allowed in 'fileencoding'; catches confusing it
	    // with 'fileencodings'.
	    errmsg = e_invalid_argument;
	else
	{
	    // May show a "+" in the title now.
	    redraw_titles();
	    // Add 'fileencoding' to the swap file.
	    ml_setflags(curbuf);
	}
    }
    if (errmsg == NULL)
    {
	// canonize the value, so that STRCMP() can be used on it
	p = enc_canonize(*varp);
	if (p != NULL)
	{
	    vim_free(*varp);
	    *varp = p;
	}
	if (varp == &p_enc)
	{
	    errmsg = mb_init();
	    redraw_titles();
	}
    }

#if defined(FEAT_GUI_GTK)
    if (errmsg == NULL && varp == &p_tenc && gui.in_use)
    {
	// GTK uses only a single encoding, and that is UTF-8.
	if (STRCMP(p_tenc, "utf-8") != 0)
	    errmsg = e_cannot_be_changed_in_gtk_GUI;
    }
#endif

    if (errmsg == NULL)
    {
#ifdef FEAT_KEYMAP
	// When 'keymap' is used and 'encoding' changes, reload the keymap
	// (with another encoding).
	if (varp == &p_enc && *curbuf->b_p_keymap != NUL)
	    (void)keymap_init();
#endif

	// When 'termencoding' is not empty and 'encoding' changes or when
	// 'termencoding' changes, need to setup for keyboard input and
	// display output conversion.
	if (((varp == &p_enc && *p_tenc != NUL) || varp == &p_tenc))
	{
	    if (convert_setup(&input_conv, p_tenc, p_enc) == FAIL
		    || convert_setup(&output_conv, p_enc, p_tenc) == FAIL)
	    {
		semsg(_(e_cannot_convert_between_str_and_str),
			p_tenc, p_enc);
		errmsg = e_invalid_argument;
	    }
	}

#if defined(MSWIN)
	// $HOME may have characters in active code page.
	if (varp == &p_enc)
	    init_homedir();
#endif
    }

    return errmsg;
}

#if defined(FEAT_POSTSCRIPT)
/*
 * The 'printencoding' option is changed.
 */
    static void
did_set_printencoding(void)
{
    char_u	*s, *p;

    // Canonize 'printencoding' if VIM standard one
    p = enc_canonize(p_penc);
    if (p != NULL)
    {
	vim_free(p_penc);
	p_penc = p;
    }
    else
    {
	// Ensure lower case and '-' for '_'
	for (s = p_penc; *s != NUL; s++)
	{
	    if (*s == '_')
		*s = '-';
	    else
		*s = TOLOWER_ASC(*s);
	}
    }
}
#endif

#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
/*
 * The 'imactivatekey' option is changed.
 */
    static char *
did_set_imactivatekey(void)
{
    if (!im_xim_isvalid_imactivate())
	return e_invalid_argument;
    return NULL;
}
#endif

#ifdef FEAT_KEYMAP
/*
 * The 'keymap' option is changed.
 */
    static char *
did_set_keymap(char_u **varp, int opt_flags, int *value_checked)
{
    char *errmsg = NULL;

    if (!valid_filetype(*varp))
	errmsg = e_invalid_argument;
    else
    {
	int	    secure_save = secure;

	// Reset the secure flag, since the value of 'keymap' has
	// been checked to be safe.
	secure = 0;

	// load or unload key mapping tables
	errmsg = keymap_init();

	secure = secure_save;

	// Since we check the value, there is no need to set P_INSECURE,
	// even when the value comes from a modeline.
	*value_checked = TRUE;
    }

    if (errmsg == NULL)
    {
	if (*curbuf->b_p_keymap != NUL)
	{
	    // Installed a new keymap, switch on using it.
	    curbuf->b_p_iminsert = B_IMODE_LMAP;
	    if (curbuf->b_p_imsearch != B_IMODE_USE_INSERT)
		curbuf->b_p_imsearch = B_IMODE_LMAP;
	}
	else
	{
	    // Cleared the keymap, may reset 'iminsert' and 'imsearch'.
	    if (curbuf->b_p_iminsert == B_IMODE_LMAP)
		curbuf->b_p_iminsert = B_IMODE_NONE;
	    if (curbuf->b_p_imsearch == B_IMODE_LMAP)
		curbuf->b_p_imsearch = B_IMODE_USE_INSERT;
	}
	if ((opt_flags & OPT_LOCAL) == 0)
	{
	    set_iminsert_global();
	    set_imsearch_global();
	}
	status_redraw_curbuf();
    }

    return errmsg;
}
#endif

/*
 * The 'fileformat' option is changed.
 */
    static char *
did_set_fileformat(char_u **varp, char_u *oldval, int opt_flags)
{
    if (!curbuf->b_p_ma && !(opt_flags & OPT_GLOBAL))
	return e_cannot_make_changes_modifiable_is_off;
    else if (check_opt_strings(*varp, p_ff_values, FALSE) != OK)
	return e_invalid_argument;

    // may also change 'textmode'
    if (get_fileformat(curbuf) == EOL_DOS)
	curbuf->b_p_tx = TRUE;
    else
	curbuf->b_p_tx = FALSE;
    redraw_titles();
    // update flag in swap file
    ml_setflags(curbuf);
    // Redraw needed when switching to/from "mac": a CR in the text
    // will be displayed differently.
    if (get_fileformat(curbuf) == EOL_MAC || *oldval == 'm')
	redraw_curbuf_later(UPD_NOT_VALID);

    return NULL;
}

/*
 * The 'fileformats' option is changed.
 */
    static char *
did_set_fileformats(void)
{
    if (check_opt_strings(p_ffs, p_ff_values, TRUE) != OK)
	return e_invalid_argument;

    // also change 'textauto'
    if (*p_ffs == NUL)
	p_ta = FALSE;
    else
	p_ta = TRUE;

    return NULL;
}

#if defined(FEAT_CRYPT)
/*
 * The 'cryptkey' option is changed.
 */
    static void
did_set_cryptkey(char_u *oldval)
{
    // Make sure the ":set" command doesn't show the new value in the
    // history.
    remove_key_from_history();

    if (STRCMP(curbuf->b_p_key, oldval) != 0)
    {
	// Need to update the swapfile.
	ml_set_crypt_key(curbuf, oldval,
		*curbuf->b_p_cm == NUL ? p_cm : curbuf->b_p_cm);
	changed_internal();
    }
}

/*
 * The 'cryptmethod' option is changed.
 */
    static char *
did_set_cryptmethod(char_u *oldval, int opt_flags)
{
    char_u  *p;
    char_u  *s;

    if (opt_flags & OPT_LOCAL)
	p = curbuf->b_p_cm;
    else
	p = p_cm;
    if (check_opt_strings(p, p_cm_values, TRUE) != OK)
	return e_invalid_argument;
    else if (crypt_self_test() == FAIL)
	return e_invalid_argument;

    // When setting the global value to empty, make it "zip".
    if (*p_cm == NUL)
    {
	free_string_option(p_cm);
	p_cm = vim_strsave((char_u *)"zip");
    }
    // When using ":set cm=name" the local value is going to be empty.
    // Do that here, otherwise the crypt functions will still use the
    // local value.
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
    {
	free_string_option(curbuf->b_p_cm);
	curbuf->b_p_cm = empty_option;
    }

    // Need to update the swapfile when the effective method changed.
    // Set "s" to the effective old value, "p" to the effective new
    // method and compare.
    if ((opt_flags & OPT_LOCAL) && *oldval == NUL)
	s = p_cm;  // was previously using the global value
    else
	s = oldval;
    if (*curbuf->b_p_cm == NUL)
	p = p_cm;  // is now using the global value
    else
	p = curbuf->b_p_cm;
    if (STRCMP(s, p) != 0)
	ml_set_crypt_key(curbuf, curbuf->b_p_key, s);

    // If the global value changes need to update the swapfile for all
    // buffers using that value.
    if ((opt_flags & OPT_GLOBAL) && STRCMP(p_cm, oldval) != 0)
    {
	buf_T	*buf;

	FOR_ALL_BUFFERS(buf)
	    if (buf != curbuf && *buf->b_p_cm == NUL)
		ml_set_crypt_key(buf, buf->b_p_key, oldval);
    }
    return NULL;
}
#endif

/*
 * The 'matchpairs' option is changed.
 */
    static char *
did_set_matchpairs(char_u **varp)
{
    char_u	*p;

    if (has_mbyte)
    {
	for (p = *varp; *p != NUL; ++p)
	{
	    int x2 = -1;
	    int x3 = -1;

	    p += mb_ptr2len(p);
	    if (*p != NUL)
		x2 = *p++;
	    if (*p != NUL)
	    {
		x3 = mb_ptr2char(p);
		p += mb_ptr2len(p);
	    }
	    if (x2 != ':' || x3 == -1 || (*p != NUL && *p != ','))
		return e_invalid_argument;
	    if (*p == NUL)
		break;
	}
    }
    else
    {
	// Check for "x:y,x:y"
	for (p = *varp; *p != NUL; p += 4)
	{
	    if (p[1] != ':' || p[2] == NUL || (p[3] != NUL && p[3] != ','))
		return e_invalid_argument;
	    if (p[3] == NUL)
		break;
	}
    }

    return NULL;
}

/*
 * The 'comments' option is changed.
 */
    static char *
did_set_comments(char_u **varp, char *errbuf)
{
    char_u	*s;
    char	*errmsg = NULL;

    for (s = *varp; *s; )
    {
	while (*s && *s != ':')
	{
	    if (vim_strchr((char_u *)COM_ALL, *s) == NULL
		    && !VIM_ISDIGIT(*s) && *s != '-')
	    {
		errmsg = illegal_char(errbuf, *s);
		break;
	    }
	    ++s;
	}
	if (*s++ == NUL)
	    errmsg = e_missing_colon;
	else if (*s == ',' || *s == NUL)
	    errmsg = e_zero_length_string;
	if (errmsg != NULL)
	    break;
	while (*s && *s != ',')
	{
	    if (*s == '\\' && s[1] != NUL)
		++s;
	    ++s;
	}
	s = skip_to_option_part(s);
    }

    return errmsg;
}

/*
 * The global 'listchars' or 'fillchars' option is changed.
 */
    static char *
did_set_global_listfillchars(char_u **varp, int opt_flags)
{
    char	*errmsg = NULL;
    char_u	**local_ptr = varp == &p_lcs
	? &curwin->w_p_lcs : &curwin->w_p_fcs;

    // only apply the global value to "curwin" when it does not have a
    // local value
    errmsg = set_chars_option(curwin, varp,
	    **local_ptr == NUL || !(opt_flags & OPT_GLOBAL));
    if (errmsg != NULL)
	return errmsg;

    tabpage_T	*tp;
    win_T	*wp;

    // If the current window is set to use the global
    // 'listchars'/'fillchars' value, clear the window-local value.
    if (!(opt_flags & OPT_GLOBAL))
	clear_string_option(local_ptr);
    FOR_ALL_TAB_WINDOWS(tp, wp)
    {
	// If the current window has a local value need to apply it
	// again, it was changed when setting the global value.
	// If no error was returned above, we don't expect an error
	// here, so ignore the return value.
	local_ptr = varp == &p_lcs ? &wp->w_p_lcs : &wp->w_p_fcs;
	if (**local_ptr == NUL)
	    (void)set_chars_option(wp, local_ptr, TRUE);
    }

    redraw_all_later(UPD_NOT_VALID);

    return NULL;
}

/*
 * The 'verbosefile' option is changed.
 */
    static char *
did_set_verbosefile(void)
{
    verbose_stop();
    if (*p_vfile != NUL && verbose_open() == FAIL)
	return e_invalid_argument;

    return NULL;
}

#ifdef FEAT_VIMINFO
/*
 * The 'viminfo' option is changed.
 */
    static char *
did_set_viminfo(char *errbuf)
{
    char_u	*s;
    char	*errmsg = NULL;

    for (s = p_viminfo; *s;)
    {
	// Check it's a valid character
	if (vim_strchr((char_u *)"!\"%'/:<@cfhnrs", *s) == NULL)
	{
	    errmsg = illegal_char(errbuf, *s);
	    break;
	}
	if (*s == 'n')	// name is always last one
	    break;
	else if (*s == 'r') // skip until next ','
	{
	    while (*++s && *s != ',')
		;
	}
	else if (*s == '%')
	{
	    // optional number
	    while (vim_isdigit(*++s))
		;
	}
	else if (*s == '!' || *s == 'h' || *s == 'c')
	    ++s;		// no extra chars
	else		// must have a number
	{
	    while (vim_isdigit(*++s))
		;

	    if (!VIM_ISDIGIT(*(s - 1)))
	    {
		if (errbuf != NULL)
		{
		    sprintf(errbuf,
			    _(e_missing_number_after_angle_str_angle),
			    transchar_byte(*(s - 1)));
		    errmsg = errbuf;
		}
		else
		    errmsg = "";
		break;
	    }
	}
	if (*s == ',')
	    ++s;
	else if (*s)
	{
	    if (errbuf != NULL)
		errmsg = e_missing_comma;
	    else
		errmsg = "";
	    break;
	}
    }
    if (*p_viminfo && errmsg == NULL && get_viminfo_parameter('\'') < 0)
	errmsg = e_must_specify_a_value;

    return errmsg;
}
#endif

/*
 * Some terminal option (t_xxx) is changed
 */
    static void
did_set_term_option(char_u **varp, int *did_swaptcap UNUSED)
{
    // ":set t_Co=0" and ":set t_Co=1" do ":set t_Co="
    if (varp == &T_CCO)
    {
	int colors = atoi((char *)T_CCO);

	// Only reinitialize colors if t_Co value has really changed to
	// avoid expensive reload of colorscheme if t_Co is set to the
	// same value multiple times.
	if (colors != t_colors)
	{
	    t_colors = colors;
	    if (t_colors <= 1)
	    {
		vim_free(T_CCO);
		T_CCO = empty_option;
	    }
#if defined(FEAT_VTP) && defined(FEAT_TERMGUICOLORS)
	    if (is_term_win32())
	    {
		swap_tcap();
		*did_swaptcap = TRUE;
	    }
#endif
	    // We now have a different color setup, initialize it again.
	    init_highlight(TRUE, FALSE);
	}
    }
    ttest(FALSE);
    if (varp == &T_ME)
    {
	out_str(T_ME);
	redraw_later(UPD_CLEAR);
#if defined(MSWIN) && (!defined(FEAT_GUI_MSWIN) || defined(VIMDLL))
	// Since t_me has been set, this probably means that the user
	// wants to use this as default colors.  Need to reset default
	// background/foreground colors.
# ifdef VIMDLL
	if (!gui.in_use && !gui.starting)
# endif
	    mch_set_normal_colors();
#endif
    }
    if (varp == &T_BE && termcap_active)
    {
	MAY_WANT_TO_LOG_THIS;

	if (*T_BE == NUL)
	    // When clearing t_BE we assume the user no longer wants
	    // bracketed paste, thus disable it by writing t_BD.
	    out_str(T_BD);
	else
	    out_str(T_BE);
    }
}

#ifdef FEAT_LINEBREAK
/*
 * The 'showbreak' option is changed.
 */
    static char *
did_set_showbreak(char_u **varp)
{
    char_u	*s;

    for (s = *varp; *s; )
    {
	if (ptr2cells(s) != 1)
	    return e_showbreak_contains_unprintable_or_wide_character;
	MB_PTR_ADV(s);
    }

    return NULL;
}
#endif

#ifdef FEAT_GUI
/*
 * The 'guifont' option is changed.
 */
    static char *
did_set_guifont(char_u *oldval UNUSED)
{
    char_u	*p;
    char	*errmsg = NULL;

    if (gui.in_use)
    {
	p = p_guifont;
# if defined(FEAT_GUI_GTK)
	// Put up a font dialog and let the user select a new value.
	// If this is cancelled go back to the old value but don't
	// give an error message.
	if (STRCMP(p, "*") == 0)
	{
	    p = gui_mch_font_dialog(oldval);
	    free_string_option(p_guifont);
	    p_guifont = (p != NULL) ? p : vim_strsave(oldval);
	}
# endif
	if (p != NULL && gui_init_font(p_guifont, FALSE) != OK)
	{
# if defined(FEAT_GUI_MSWIN) || defined(FEAT_GUI_PHOTON)
	    if (STRCMP(p_guifont, "*") == 0)
	    {
		// Dialog was cancelled: Keep the old value without giving
		// an error message.
		free_string_option(p_guifont);
		p_guifont = vim_strsave(oldval);
	    }
	    else
# endif
		errmsg = e_invalid_fonts;
	}
    }

    return errmsg;
}

# ifdef FEAT_XFONTSET
/*
 * The 'guifontset' option is changed.
 */
    static char *
did_set_guifontset()
{
    char *errmsg = NULL;

    if (STRCMP(p_guifontset, "*") == 0)
	errmsg = e_cant_select_fontset;
    else if (gui.in_use && gui_init_font(p_guifontset, TRUE) != OK)
	errmsg = e_invalid_fontset;

    return errmsg;
}
# endif

/*
 * The 'guifontwide' option is changed.
 */
    static char *
did_set_guifontwide(void)
{
    char *errmsg = NULL;

    if (STRCMP(p_guifontwide, "*") == 0)
	errmsg = e_cant_select_wide_font;
    else if (gui_get_wide_font() == FAIL)
	errmsg = e_invalid_wide_font;

    return errmsg;
}
#endif

#if defined(FEAT_GUI_GTK)
    static void
did_set_guiligatures(void)
{
    gui_set_ligatures();
}
#endif

#ifdef FEAT_MOUSESHAPE
    static char *
did_set_mouseshape(void)
{
    char *errmsg = NULL;

    errmsg = parse_shape_opt(SHAPE_MOUSE);
    update_mouseshape(-1);

    return errmsg;
}
#endif

/*
 * The 'titlestring' or the 'iconstring' option is changed.
 */
    static void
did_set_titleiconstring(char_u **varp UNUSED)
{
#ifdef FEAT_STL_OPT
    int	flagval = (varp == &p_titlestring) ? STL_IN_TITLE : STL_IN_ICON;

    // NULL => statusline syntax
    if (vim_strchr(*varp, '%') && check_stl_option(*varp) == NULL)
	stl_syntax |= flagval;
    else
	stl_syntax &= ~flagval;
#endif
    did_set_title();
}

#ifdef FEAT_GUI
/*
 * The 'guioptions' option is changed.
 */
    static void
did_set_guioptions(char_u *oldval)
{
    gui_init_which_components(oldval);
}
#endif

#if defined(FEAT_GUI_TABLINE)
    static void
did_set_guitablabel()
{
    redraw_tabline = TRUE;
}
#endif

#if defined(UNIX) || defined(VMS)
/*
 * The 'ttymouse' option is changed.
 */
    static char *
did_set_ttymouse(void)
{
    char *errmsg = NULL;

    // Switch the mouse off before changing the escape sequences used for
    // that.
    mch_setmouse(FALSE);
    if (opt_strings_flags(p_ttym, p_ttym_values, &ttym_flags, FALSE) != OK)
	errmsg = e_invalid_argument;
    else
	check_mouse_termcode();
    if (termcap_active)
	setmouse();		// may switch it on again

    return errmsg;
}
#endif

/*
 * The 'selection' option is changed.
 */
    static char *
did_set_selection(void)
{
    if (*p_sel == NUL
	    || check_opt_strings(p_sel, p_sel_values, FALSE) != OK)
	return e_invalid_argument;

    return NULL;
}

#ifdef FEAT_BROWSE
/*
 * The 'browsedir' option is changed.
 */
    static char *
did_set_browsedir(void)
{
    if (check_opt_strings(p_bsdir, p_bsdir_values, FALSE) != OK
	    && !mch_isdir(p_bsdir))
	return e_invalid_argument;

    return NULL;
}
#endif

/*
 * The 'keymodel' option is changed.
 */
    static char *
did_set_keymodel(void)
{
    if (check_opt_strings(p_km, p_km_values, TRUE) != OK)
	return e_invalid_argument;

    km_stopsel = (vim_strchr(p_km, 'o') != NULL);
    km_startsel = (vim_strchr(p_km, 'a') != NULL);
    return NULL;
}

/*
 * The 'keyprotocol' option is changed.
 */
    static char *
did_set_keyprotocol(void)
{
    if (match_keyprotocol(NULL) == KEYPROTOCOL_FAIL)
	return e_invalid_argument;

    return NULL;
}

/*
 * The 'mousemodel' option is changed.
 */
    static char *
did_set_mousemodel(void)
{
    if (check_opt_strings(p_mousem, p_mousem_values, FALSE) != OK)
	return e_invalid_argument;
#if defined(FEAT_GUI_MOTIF) && defined(FEAT_MENU) && (XmVersion <= 1002)
    else if (*p_mousem != *oldval)
	// Changed from "extend" to "popup" or "popup_setpos" or vv: need
	// to create or delete the popup menus.
	gui_motif_update_mousemodel(root_menu);
#endif

    return NULL;
}

/*
 * The 'display' option is changed.
 */
    static char *
did_set_display(void)
{
    if (opt_strings_flags(p_dy, p_dy_values, &dy_flags, TRUE) != OK)
	return e_invalid_argument;

    (void)init_chartab();
    return NULL;
}

#ifdef FEAT_SPELL
/*
 * The 'spellfile' option is changed.
 */
    static char *
did_set_spellfile(char_u **varp)
{
    if (!valid_spellfile(*varp))
	return e_invalid_argument;

    // If there is a window for this buffer in which 'spell' is set load the
    // wordlists.
    return did_set_spell_option(TRUE);
}

/*
 * The 'spell' option is changed.
 */
    static char *
did_set_spell(char_u **varp)
{
    if (!valid_spelllang(*varp))
	return e_invalid_argument;

    // If there is a window for this buffer in which 'spell' is set load the
    // wordlists.
    return did_set_spell_option(FALSE);
}

/*
 * The 'spellcapcheck' option is changed.
 */
    static char *
did_set_spellcapcheck(void)
{
    // compile the regexp program.
    return compile_cap_prog(curwin->w_s);
}

/*
 * The 'spelloptions' option is changed.
 */
    static char *
did_set_spelloptions(char_u **varp)
{
    if (**varp != NUL && STRCMP("camel", *varp) != 0)
	return e_invalid_argument;

    return NULL;
}

/*
 * The 'spellsuggest' option is changed.
 */
    static char *
did_set_spellsuggest(void)
{
    if (spell_check_sps() != OK)
	return e_invalid_argument;

    return NULL;
}

/*
 * The 'mkspellmem' option is changed.
 */
    static char *
did_set_mkspellmem(void)
{
    if (spell_check_msm() != OK)
	return e_invalid_argument;

    return NULL;
}
#endif

/*
 * The 'buftype' option is changed.
 */
    static char *
did_set_buftype(void)
{
    if (check_opt_strings(curbuf->b_p_bt, p_buftype_values, FALSE) != OK)
	return e_invalid_argument;

    if (curwin->w_status_height)
    {
	curwin->w_redr_status = TRUE;
	redraw_later(UPD_VALID);
    }
    curbuf->b_help = (curbuf->b_p_bt[0] == 'h');
    redraw_titles();

    return NULL;
}

#ifdef FEAT_STL_OPT
/*
 * The 'statusline' or the 'tabline' or the 'rulerformat' option is changed.
 */
    static char *
did_set_statusline(char_u **varp)
{
    char_u	*s;
    char	*errmsg = NULL;
    int		wid;

    if (varp == &p_ruf)	// reset ru_wid first
	ru_wid = 0;
    s = *varp;
    if (varp == &p_ruf && *s == '%')
    {
	// set ru_wid if 'ruf' starts with "%99("
	if (*++s == '-')	// ignore a '-'
	    s++;
	wid = getdigits(&s);
	if (wid && *s == '(' && (errmsg = check_stl_option(p_ruf)) == NULL)
	    ru_wid = wid;
	else
	    errmsg = check_stl_option(p_ruf);
    }
    // check 'statusline' or 'tabline' only if it doesn't start with "%!"
    else if (varp == &p_ruf || s[0] != '%' || s[1] != '!')
	errmsg = check_stl_option(s);
    if (varp == &p_ruf && errmsg == NULL)
	comp_col();

    return errmsg;
}
#endif

/*
 * The 'complete' option is changed.
 */
    static char *
did_set_complete(char_u **varp, char *errbuf)
{
    char_u	*s;

    // check if it is a valid value for 'complete' -- Acevedo
    for (s = *varp; *s;)
    {
	while (*s == ',' || *s == ' ')
	    s++;
	if (!*s)
	    break;
	if (vim_strchr((char_u *)".wbuksid]tU", *s) == NULL)
	    return illegal_char(errbuf, *s);
	if (*++s != NUL && *s != ',' && *s != ' ')
	{
	    if (s[-1] == 'k' || s[-1] == 's')
	    {
		// skip optional filename after 'k' and 's'
		while (*s && *s != ',' && *s != ' ')
		{
		    if (*s == '\\' && s[1] != NUL)
			++s;
		    ++s;
		}
	    }
	    else
	    {
		if (errbuf != NULL)
		{
		    sprintf((char *)errbuf,
			    _(e_illegal_character_after_chr), *--s);
		    return errbuf;
		}
		return "";
	    }
	}
    }

    return NULL;
}

/*
 * The 'completeopt' option is changed.
 */
    static char *
did_set_completeopt(void)
{
    if (check_opt_strings(p_cot, p_cot_values, TRUE) != OK)
	return e_invalid_argument;

    completeopt_was_set();
    return NULL;
}

#ifdef BACKSLASH_IN_FILENAME
/*
 * The 'completeslash' option is changed.
 */
    static char *
did_set_completeslash(void)
{
    if (check_opt_strings(p_csl, p_csl_values, FALSE) != OK
	    || check_opt_strings(curbuf->b_p_csl, p_csl_values, FALSE) != OK)
	return e_invalid_argument;

    return NULL;
}
#endif

#ifdef FEAT_SIGNS
/*
 * The 'signcolumn' option is changed.
 */
    static char *
did_set_signcolumn(char_u **varp, char_u *oldval)
{
    if (check_opt_strings(*varp, p_scl_values, FALSE) != OK)
	return e_invalid_argument;
    // When changing the 'signcolumn' to or from 'number', recompute the
    // width of the number column if 'number' or 'relativenumber' is set.
    if (((*oldval == 'n' && *(oldval + 1) == 'u')
		|| (*curwin->w_p_scl == 'n' && *(curwin->w_p_scl + 1) =='u'))
	    && (curwin->w_p_nu || curwin->w_p_rnu))
	curwin->w_nrwidth_line_count = 0;

    return NULL;
}
#endif

#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_MSWIN)
/*
 * The 'toolbar' option is changed.
 */
    static char *
did_set_toolbar(void)
{
    if (opt_strings_flags(p_toolbar, p_toolbar_values,
		&toolbar_flags, TRUE) != OK)
	return e_invalid_argument;

    out_flush();
    gui_mch_show_toolbar((toolbar_flags &
		(TOOLBAR_TEXT | TOOLBAR_ICONS)) != 0);
    return NULL;
}
#endif

#if defined(FEAT_TOOLBAR) && defined(FEAT_GUI_GTK)
/*
 * The 'toolbariconsize' option is changed.  GTK+ 2 only.
 */
    static char *
did_set_toolbariconsize(void)
{
    if (opt_strings_flags(p_tbis, p_tbis_values, &tbis_flags, FALSE) != OK)
	return e_invalid_argument;

    out_flush();
    gui_mch_show_toolbar((toolbar_flags &
		(TOOLBAR_TEXT | TOOLBAR_ICONS)) != 0);
    return NULL;
}
#endif

/*
 * The 'pastetoggle' option is changed.
 */
    static void
did_set_pastetoggle(void)
{
    char_u	*p;

    // translate key codes like in a mapping
    if (*p_pt)
    {
	(void)replace_termcodes(p_pt, &p,
		REPTERM_FROM_PART | REPTERM_DO_LT, NULL);
	if (p != NULL)
	{
	    free_string_option(p_pt);
	    p_pt = p;
	}
    }
}

/*
 * The 'backspace' option is changed.
 */
    static char *
did_set_backspace(void)
{
    if (VIM_ISDIGIT(*p_bs))
    {
	if (*p_bs > '3' || p_bs[1] != NUL)
	    return e_invalid_argument;
    }
    else if (check_opt_strings(p_bs, p_bs_values, TRUE) != OK)
	return e_invalid_argument;

    return NULL;
}

/*
 * The 'tagcase' option is changed.
 */
    static char *
did_set_tagcase(int opt_flags)
{
    unsigned int	*flags;
    char_u		*p;

    if (opt_flags & OPT_LOCAL)
    {
	p = curbuf->b_p_tc;
	flags = &curbuf->b_tc_flags;
    }
    else
    {
	p = p_tc;
	flags = &tc_flags;
    }

    if ((opt_flags & OPT_LOCAL) && *p == NUL)
	// make the local value empty: use the global value
	*flags = 0;
    else if (*p == NUL
	    || opt_strings_flags(p, p_tc_values, flags, FALSE) != OK)
	return e_invalid_argument;

    return NULL;
}

#ifdef FEAT_DIFF
/*
 * The 'diffopt' option is changed.
 */
    static char *
did_set_diffopt(void)
{
    if (diffopt_changed() == FAIL)
	return e_invalid_argument;

    return NULL;
}
#endif

#ifdef FEAT_FOLDING
/*
 * The 'foldmethod' option is changed.
 */
    static char *
did_set_foldmethod(char_u **varp)
{
    if (check_opt_strings(*varp, p_fdm_values, FALSE) != OK
	    || *curwin->w_p_fdm == NUL)
	return e_invalid_argument;

    foldUpdateAll(curwin);
    if (foldmethodIsDiff(curwin))
	newFoldLevel();
    return NULL;
}

/*
 * The 'foldmarker' option is changed.
 */
    static char *
did_set_foldmarker(char_u **varp)
{
    char_u	*p;

    p = vim_strchr(*varp, ',');
    if (p == NULL)
	return e_comma_required;
    else if (p == *varp || p[1] == NUL)
	return e_invalid_argument;
    else if (foldmethodIsMarker(curwin))
	foldUpdateAll(curwin);

    return NULL;
}

/*
 * The 'commentstring' option is changed.
 */
    static char *
did_set_commentstring(char_u **varp)
{
    if (**varp != NUL && strstr((char *)*varp, "%s") == NULL)
	return e_commentstring_must_be_empty_or_contain_str;

    return NULL;
}

/*
 * The 'foldignore' option is changed.
 */
    static void
did_set_foldignore(void)
{
    if (foldmethodIsIndent(curwin))
	foldUpdateAll(curwin);
}
#endif

/*
 * The 'virtualedit' option is changed.
 */
    static char *
did_set_virtualedit(char_u *oldval, int opt_flags)
{
    char_u		*ve = p_ve;
    unsigned int	*flags = &ve_flags;

    if (opt_flags & OPT_LOCAL)
    {
	ve = curwin->w_p_ve;
	flags = &curwin->w_ve_flags;
    }

    if ((opt_flags & OPT_LOCAL) && *ve == NUL)
	// make the local value empty: use the global value
	*flags = 0;
    else
    {
	if (opt_strings_flags(ve, p_ve_values, flags, TRUE) != OK)
	    return e_invalid_argument;
	else if (STRCMP(ve, oldval) != 0)
	{
	    // Recompute cursor position in case the new 've' setting
	    // changes something.
	    validate_virtcol();
	    coladvance(curwin->w_virtcol);
	}
    }

    return NULL;
}

#if defined(FEAT_CSCOPE) && defined(FEAT_QUICKFIX)
/*
 * The 'cscopequickfix' option is changed.
 */
    static char *
did_set_cscopequickfix(void)
{
    char_u	*p;

    if (p_csqf == NULL)
	return NULL;

    p = p_csqf;
    while (*p != NUL)
    {
	if (vim_strchr((char_u *)CSQF_CMDS, *p) == NULL
		|| p[1] == NUL
		|| vim_strchr((char_u *)CSQF_FLAGS, p[1]) == NULL
		|| (p[2] != NUL && p[2] != ','))
	    return e_invalid_argument;
	else if (p[2] == NUL)
	    break;
	else
	    p += 3;
    }

    return NULL;
}
#endif

/*
 * The 'cinoptions' option is changed.
 */
    static void
did_set_cinoptions(void)
{
    // TODO: recognize errors
    parse_cino(curbuf);
}

/*
 * The 'lispoptions' option is changed.
 */
    static char *
did_set_lispoptions(char_u **varp)
{
    if (**varp != NUL && STRCMP(*varp, "expr:0") != 0
	    && STRCMP(*varp, "expr:1") != 0)
	return e_invalid_argument;

    return NULL;
}

#if defined(FEAT_RENDER_OPTIONS)
/*
 * The 'renderoptions' option is changed.
 */
    static char *
did_set_renderoptions(void)
{
    if (!gui_mch_set_rendering_options(p_rop))
	return e_invalid_argument;

    return NULL;
}
#endif

/*
 * The 'filetype' or the 'syntax' option is changed.
 */
    static char *
did_set_filetype_or_syntax(
    char_u	**varp,
    char_u	*oldval,
    int		*value_checked,
    int		*value_changed)
{
    if (!valid_filetype(*varp))
	return e_invalid_argument;

    *value_changed = STRCMP(oldval, *varp) != 0;

    // Since we check the value, there is no need to set P_INSECURE,
    // even when the value comes from a modeline.
    *value_checked = TRUE;

    return NULL;
}

#ifdef FEAT_TERMINAL
/*
 * The 'termwinkey' option is changed.
 */
    static char *
did_set_termwinkey(void)
{
    if (*curwin->w_p_twk != NUL
	    && string_to_key(curwin->w_p_twk, TRUE) == 0)
	return e_invalid_argument;

    return NULL;
}

/*
 * The 'termwinsize' option is changed.
 */
    static char *
did_set_termwinsize(void)
{
    char_u	*p;

    if (*curwin->w_p_tws == NUL)
	return NULL;

    p = skipdigits(curwin->w_p_tws);
    if (p == curwin->w_p_tws
	    || (*p != 'x' && *p != '*')
	    || *skipdigits(p + 1) != NUL)
	return e_invalid_argument;

    return NULL;
}
#endif

#ifdef FEAT_VARTABS
/*
 * The 'varsofttabstop' option is changed.
 */
    static char *
did_set_varsofttabstop(char_u **varp)
{
    char_u *cp;

    if (!(*varp)[0] || ((*varp)[0] == '0' && !(*varp)[1]))
    {
	if (curbuf->b_p_vsts_array)
	{
	    vim_free(curbuf->b_p_vsts_array);
	    curbuf->b_p_vsts_array = 0;
	}
    }
    else
    {
	for (cp = *varp; *cp; ++cp)
	{
	    if (vim_isdigit(*cp))
		continue;
	    if (*cp == ',' && cp > *varp && *(cp-1) != ',')
		continue;
	    return e_invalid_argument;
	}

	int *oldarray = curbuf->b_p_vsts_array;
	if (tabstop_set(*varp, &(curbuf->b_p_vsts_array)) == OK)
	{
	    if (oldarray)
		vim_free(oldarray);
	}
	else
	    return e_invalid_argument;
    }

    return NULL;
}

/*
 * The 'vartabstop' option is changed.
 */
    static char *
did_set_vartabstop(char_u **varp)
{
    char_u *cp;

    if (!(*varp)[0] || ((*varp)[0] == '0' && !(*varp)[1]))
    {
	if (curbuf->b_p_vts_array)
	{
	    vim_free(curbuf->b_p_vts_array);
	    curbuf->b_p_vts_array = NULL;
	}
    }
    else
    {
	for (cp = *varp; *cp; ++cp)
	{
	    if (vim_isdigit(*cp))
		continue;
	    if (*cp == ',' && cp > *varp && *(cp-1) != ',')
		continue;
	    return e_invalid_argument;
	}

	int *oldarray = curbuf->b_p_vts_array;

	if (tabstop_set(*varp, &(curbuf->b_p_vts_array)) == OK)
	{
	    vim_free(oldarray);
# ifdef FEAT_FOLDING
	    if (foldmethodIsIndent(curwin))
		foldUpdateAll(curwin);
# endif
	}
	else
	    return e_invalid_argument;
    }

    return NULL;
}
#endif

#ifdef FEAT_PROP_POPUP
/*
 * The 'previewpopup' option is changed.
 */
    static char *
did_set_previewpopup(void)
{
    if (parse_previewpopup(NULL) == FAIL)
	return e_invalid_argument;

    return NULL;
}

# ifdef FEAT_QUICKFIX
/*
 * The 'completepopup' option is changed.
 */
    static char *
did_set_completepopup(void)
{
    if (parse_completepopup(NULL) == FAIL)
	return e_invalid_argument;

    popup_close_info();
    return NULL;
}
# endif
#endif

#ifdef FEAT_EVAL
/*
 * One of the '*expr' options is changed: 'balloonexpr', 'diffexpr',
 * 'foldexpr', 'foldtext', 'formatexpr', 'includeexpr', 'indentexpr',
 * 'patchexpr', 'printexpr' and 'charconvert'.
 *
 */
    static void
did_set_optexpr(char_u **varp)
{
    // If the option value starts with <SID> or s:, then replace that with
    // the script identifier.
    char_u *name = get_scriptlocal_funcname(*varp);
    if (name != NULL)
    {
	free_string_option(*varp);
	*varp = name;
    }

# ifdef FEAT_FOLDING
    if (varp == &curwin->w_p_fde && foldmethodIsExpr(curwin))
	foldUpdateAll(curwin);
# endif
}
#endif

/*
 * An option which is a list of flags is set.  Valid values are in 'flags'.
 */
    static char *
did_set_option_listflag(char_u **varp, char_u *flags, char *errbuf)
{
    char_u	*s;

    for (s = *varp; *s; ++s)
	if (vim_strchr(flags, *s) == NULL)
	    return illegal_char(errbuf, *s);

    return NULL;
}

#ifdef FEAT_SYN_HL
/*
 * When the 'syntax' option is set, load the syntax of that name.
 */
    static void
do_syntax_autocmd(int value_changed)
{
    static int syn_recursive = 0;

    ++syn_recursive;
    // Only pass TRUE for "force" when the value changed or not used
    // recursively, to avoid endless recurrence.
    apply_autocmds(EVENT_SYNTAX, curbuf->b_p_syn, curbuf->b_fname,
	    value_changed || syn_recursive == 1, curbuf);
    curbuf->b_flags |= BF_SYN_SET;
    --syn_recursive;
}
#endif

/*
 * When the 'filetype' option is set, trigger the FileType autocommand.
 */
    static void
do_filetype_autocmd(char_u **varp, int opt_flags, int value_changed)
{
    // Skip this when called from a modeline and the filetype was already set
    // to this value.
    if ((opt_flags & OPT_MODELINE) && !value_changed)
	return;

    static int  ft_recursive = 0;
    int	    secure_save = secure;

    // Reset the secure flag, since the value of 'filetype' has
    // been checked to be safe.
    secure = 0;

    ++ft_recursive;
    did_filetype = TRUE;
    // Only pass TRUE for "force" when the value changed or not
    // used recursively, to avoid endless recurrence.
    apply_autocmds(EVENT_FILETYPE, curbuf->b_p_ft, curbuf->b_fname,
	    value_changed || ft_recursive == 1, curbuf);
    --ft_recursive;
    // Just in case the old "curbuf" is now invalid.
    if (varp != &(curbuf->b_p_ft))
	varp = NULL;

    secure = secure_save;
}

#ifdef FEAT_SPELL
/*
 * When the 'spelllang' option is set, source the spell/LANG.vim file in
 * 'runtimepath'.
 */
    static void
do_spelllang_source(void)
{
    char_u	fname[200];
    char_u	*p;
    char_u	*q = curwin->w_s->b_p_spl;

    // Skip the first name if it is "cjk".
    if (STRNCMP(q, "cjk,", 4) == 0)
	q += 4;

    // They could set 'spellcapcheck' depending on the language.  Use the first
    // name in 'spelllang' up to '_region' or '.encoding'.
    for (p = q; *p != NUL; ++p)
	if (!ASCII_ISALNUM(*p) && *p != '-')
	    break;
    if (p > q)
    {
	vim_snprintf((char *)fname, 200, "spell/%.*s.vim",
		(int)(p - q), q);
	source_runtime(fname, DIP_ALL);
    }
}
#endif

/*
 * Handle string options that need some action to perform when changed.
 * The new value must be allocated.
 * Returns NULL for success, or an untranslated error message for an error.
 */
    char *
did_set_string_option(
    int		opt_idx,		// index in options[] table
    char_u	**varp,			// pointer to the option variable
    char_u	*oldval,		// previous value of the option
    char	*errbuf,		// buffer for errors, or NULL
    int		opt_flags,		// OPT_LOCAL and/or OPT_GLOBAL
    int		*value_checked)		// value was checked to be safe, no
					// need to set P_INSECURE
{
    char	*errmsg = NULL;
    int		did_chartab = FALSE;
    char_u	**gvarp;
    long_u	free_oldval = (get_option_flags(opt_idx) & P_ALLOCED);
#ifdef FEAT_GUI
    // set when changing an option that only requires a redraw in the GUI
    int		redraw_gui_only = FALSE;
#endif
    int		value_changed = FALSE;
    int		did_swaptcap = FALSE;

    // Get the global option to compare with, otherwise we would have to check
    // two values for all local options.
    gvarp = (char_u **)get_option_varp_scope(opt_idx, OPT_GLOBAL);

    // Disallow changing some options from secure mode
    if ((secure
#ifdef HAVE_SANDBOX
		|| sandbox != 0
#endif
		) && (get_option_flags(opt_idx) & P_SECURE))
	errmsg = e_not_allowed_here;
    // Check for a "normal" directory or file name in some options.
    else if (check_illegal_path_names(opt_idx, varp))
	errmsg = e_invalid_argument;
    else if (varp == &T_NAME)			// 'term'
	errmsg = did_set_term(&opt_idx, &free_oldval);
    else if (gvarp == &p_bkc)			// 'backupcopy'
	errmsg = did_set_backupcopy(oldval, opt_flags);
    else if (  varp == &p_bex			// 'backupext'
	    || varp == &p_pm)			// 'patchmode'
	errmsg = did_set_backupext_or_patchmode();
#ifdef FEAT_LINEBREAK
    else if (varp == &curwin->w_p_briopt)	// 'breakindentopt'
	errmsg = did_set_breakindentopt();
#endif

    else if (  varp == &p_isi			// 'isident'
	    || varp == &(curbuf->b_p_isk)	// 'iskeyword'
	    || varp == &p_isp			// 'isprint'
	    || varp == &p_isf)			// 'isfname'
	errmsg = did_set_isopt(&did_chartab);
    else if (varp == &p_hf)			// 'helpfile'
	did_set_helpfile();
#ifdef FEAT_SYN_HL
    else if (  varp == &curwin->w_p_culopt	// 'cursorlineopt'
	    || gvarp == &curwin->w_allbuf_opt.wo_culopt)
	errmsg = did_set_cursorlineopt(varp);
    else if (varp == &curwin->w_p_cc)		// 'colorcolumn'
	errmsg = check_colorcolumn(curwin);
#endif
#ifdef FEAT_MULTI_LANG
    else if (varp == &p_hlg)			// 'helplang'
	errmsg = did_set_helplang();
#endif
    else if (varp == &p_hl)			// 'highlight'
	errmsg = did_set_highlight();
    else if (gvarp == &p_nf)			// 'nrformats'
	errmsg = did_set_opt_strings(*varp, p_nf_values, TRUE);
#ifdef FEAT_SESSION
    else if (varp == &p_ssop)			// 'sessionoptions'
	errmsg = did_set_sessionoptions(oldval);
    else if (varp == &p_vop)			// 'viewoptions'
	errmsg = did_set_opt_flags(p_vop, p_ssop_values, &vop_flags, TRUE);
#endif
    else if (varp == &p_sbo)			// 'scrollopt'
	errmsg = did_set_opt_strings(p_sbo, p_scbopt_values, TRUE);
    else if (  varp == &p_ambw			// 'ambiwidth'
	    || varp == &p_emoji)		// 'emoji'
	errmsg = did_set_ambiwidth();
    else if (varp == &p_bg)			// 'background'
	errmsg = did_set_background();
    else if (varp == &p_wim)			// 'wildmode'
	errmsg = did_set_wildmode();
    else if (varp == &p_wop)			// 'wildoptions'
	errmsg = did_set_opt_strings(p_wop, p_wop_values, TRUE);
#ifdef FEAT_WAK
    else if (varp == &p_wak)			// 'winaltkeys'
	errmsg = did_set_winaltkeys();
#endif
    else if (varp == &p_ei)			// 'eventignore'
	errmsg = did_set_eventignore();

    else if (  varp == &p_enc			// 'encoding'
	    || gvarp == &p_fenc			// 'fileencoding'
	    || varp == &p_tenc			// 'termencoding'
	    || gvarp == &p_menc)		// 'makeencoding'
	errmsg = did_set_encoding(varp, gvarp, opt_flags);
#if defined(FEAT_POSTSCRIPT)
    else if (varp == &p_penc)			// 'printencoding'
	did_set_printencoding();
#endif
#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
    else if (varp == &p_imak)			// 'imactivatekey'
	errmsg = did_set_imactivatekey();
#endif
#ifdef FEAT_KEYMAP
    else if (varp == &curbuf->b_p_keymap)	// 'keymap'
	errmsg = did_set_keymap(varp, opt_flags, value_checked);
#endif
    else if (gvarp == &p_ff)			// 'fileformat'
	errmsg = did_set_fileformat(varp, oldval, opt_flags);
    else if (varp == &p_ffs)			// 'fileformats'
	errmsg = did_set_fileformats();
#if defined(FEAT_CRYPT)
    else if (gvarp == &p_key)			// 'cryptkey'
	did_set_cryptkey(oldval);
    else if (gvarp == &p_cm)			// 'cryptmethod'
	errmsg = did_set_cryptmethod(oldval, opt_flags);
#endif
    else if (gvarp == &p_mps)			// 'matchpairs'
	errmsg = did_set_matchpairs(varp);
    else if (gvarp == &p_com)			// 'comments'
	errmsg = did_set_comments(varp, errbuf);
    else if (  varp == &p_lcs			// global 'listchars'
	    || varp == &p_fcs)			// global 'fillchars'
	errmsg = did_set_global_listfillchars(varp, opt_flags);
    else if (varp == &curwin->w_p_lcs)		// local 'listchars'
	errmsg = set_chars_option(curwin, varp, TRUE);
    else if (varp == &curwin->w_p_fcs)		// local 'fillchars'
	errmsg = set_chars_option(curwin, varp, TRUE);
    else if (varp == &p_cedit)			// 'cedit'
	errmsg = check_cedit();
    else if (varp == &p_vfile)			// 'verbosefile'
	errmsg = did_set_verbosefile();
#ifdef FEAT_VIMINFO
    else if (varp == &p_viminfo)		// 'viminfo'
	errmsg = did_set_viminfo(errbuf);
#endif // FEAT_VIMINFO
    // terminal options
    else if (istermoption_idx(opt_idx) && full_screen)
	did_set_term_option(varp, &did_swaptcap);
#ifdef FEAT_LINEBREAK
    else if (gvarp == &p_sbr)			// 'showbreak'
	errmsg = did_set_showbreak(varp);
#endif
#ifdef FEAT_GUI
    else if (varp == &p_guifont)		// 'guifont'
    {
	errmsg = did_set_guifont(oldval);
	redraw_gui_only = TRUE;
    }
# ifdef FEAT_XFONTSET
    else if (varp == &p_guifontset)		// 'guifontset'
    {
	errmsg = did_set_guifontset();
	redraw_gui_only = TRUE;
    }
# endif
    else if (varp == &p_guifontwide)		// 'guifontwide'
    {
	errmsg = did_set_guifontwide();
	redraw_gui_only = TRUE;
    }
#endif
#if defined(FEAT_GUI_GTK)
    else if (varp == &p_guiligatures)		// 'guiligatures'
    {
	did_set_guiligatures();
	redraw_gui_only = TRUE;
    }
#endif
#ifdef CURSOR_SHAPE
    else if (varp == &p_guicursor)		// 'guicursor'
	errmsg = parse_shape_opt(SHAPE_CURSOR);
#endif
#ifdef FEAT_MOUSESHAPE
    else if (varp == &p_mouseshape)		// 'mouseshape'
	errmsg = did_set_mouseshape();
#endif
#ifdef FEAT_PRINTER
    else if (varp == &p_popt)			// 'printoptions'
	errmsg = parse_printoptions();
# if defined(FEAT_POSTSCRIPT)
    else if (varp == &p_pmfn)			// 'printmbfont'
	errmsg = parse_printmbfont();
# endif
#endif
#ifdef FEAT_LANGMAP
    else if (varp == &p_langmap)		// 'langmap'
	langmap_set();
#endif
#ifdef FEAT_LINEBREAK
    else if (varp == &p_breakat)		// 'breakat'
	fill_breakat_flags();
#endif
    else if (  varp == &p_titlestring		// 'titlestring'
	    || varp == &p_iconstring)		// 'iconstring'
	did_set_titleiconstring(varp);
#ifdef FEAT_GUI
    else if (varp == &p_go)			// 'guioptions'
    {
	did_set_guioptions(oldval);
	redraw_gui_only = TRUE;
    }
#endif
#if defined(FEAT_GUI_TABLINE)
    else if (varp == &p_gtl)			// 'guitablabel'
    {
	did_set_guitablabel();
	redraw_gui_only = TRUE;
    }
    else if (varp == &p_gtt)			// 'guitabtooltip'
	redraw_gui_only = TRUE;
#endif
#if defined(UNIX) || defined(VMS)
    else if (varp == &p_ttym)			// 'ttymouse'
	errmsg = did_set_ttymouse();
#endif
    else if (varp == &p_sel)			// 'selection'
	errmsg = did_set_selection();
    else if (varp == &p_slm)			// 'selectmode'
	errmsg = did_set_opt_strings(p_slm, p_slm_values, TRUE);
#ifdef FEAT_BROWSE
    else if (varp == &p_bsdir)			// 'browsedir'
	errmsg = did_set_browsedir();
#endif
    else if (varp == &p_km)			// 'keymodel'
	errmsg = did_set_keymodel();
    else if (varp == &p_kpc)			// 'keyprotocol'
	errmsg = did_set_keyprotocol();
    else if (varp == &p_mousem)			// 'mousemodel'
	errmsg = did_set_mousemodel();
    else if (varp == &p_swb)			// 'switchbuf'
	errmsg = did_set_opt_flags(p_swb, p_swb_values, &swb_flags, TRUE);
    else if (varp == &p_spk)			// 'splitkeep'
	errmsg = did_set_opt_strings(p_spk, p_spk_values, FALSE);
    else if (varp == &p_debug)			// 'debug'
	errmsg = did_set_opt_strings(p_debug, p_debug_values, TRUE);
    else if (varp == &p_dy)			// 'display'
	errmsg = did_set_display();
    else if (varp == &p_ead)			// 'eadirection'
	errmsg = did_set_opt_strings(p_ead, p_ead_values, FALSE);
#ifdef FEAT_CLIPBOARD
    else if (varp == &p_cb)			// 'clipboard'
	errmsg = check_clipboard_option();
#endif
#ifdef FEAT_SPELL
    else if (varp == &(curwin->w_s->b_p_spf))	// 'spellfile'
	errmsg = did_set_spellfile(varp);
    else if (varp == &(curwin->w_s->b_p_spl))	// 'spell'
	errmsg = did_set_spell(varp);
    else if (varp == &(curwin->w_s->b_p_spc))	// 'spellcapcheck'
	errmsg = did_set_spellcapcheck();
    else if (varp == &(curwin->w_s->b_p_spo))	// 'spelloptions'
	errmsg = did_set_spelloptions(varp);
    else if (varp == &p_sps)			// 'spellsuggest'
	errmsg = did_set_spellsuggest();
    else if (varp == &p_msm)			// 'mkspellmem'
	errmsg = did_set_mkspellmem();
#endif
    else if (gvarp == &p_bh)			// 'bufhidden'
	errmsg = did_set_opt_strings(curbuf->b_p_bh, p_bufhidden_values,
								FALSE);
    else if (gvarp == &p_bt)			// 'buftype'
	errmsg = did_set_buftype();
#ifdef FEAT_STL_OPT
    else if (  gvarp == &p_stl			// 'statusline'
	    || varp == &p_tal			// 'tabline'
	    || varp == &p_ruf)			// 'rulerformat'
	errmsg = did_set_statusline(varp);
#endif
    else if (gvarp == &p_cpt)			// 'complete'
	errmsg = did_set_complete(varp, errbuf);
    else if (varp == &p_cot)			// 'completeopt'
	errmsg = did_set_completeopt();
#ifdef BACKSLASH_IN_FILENAME
    else if (gvarp == &p_csl)			// 'completeslash'
	errmsg = did_set_completeslash();
#endif
#ifdef FEAT_SIGNS
    else if (varp == &curwin->w_p_scl)		// 'signcolumn'
	errmsg = did_set_signcolumn(varp, oldval);
#endif
    else if (varp == &p_sloc)			// 'showcmdloc'
	errmsg = did_set_opt_strings(p_sloc, p_sloc_values, FALSE);
#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_MSWIN)
    else if (varp == &p_toolbar)		// 'toolbar'
	errmsg = did_set_toolbar();
#endif
#if defined(FEAT_TOOLBAR) && defined(FEAT_GUI_GTK)
    else if (varp == &p_tbis)			// 'toolbariconsize'
	errmsg = did_set_toolbariconsize();
#endif
    else if (varp == &p_pt)			// 'pastetoggle'
	did_set_pastetoggle();
    else if (varp == &p_bs)			// 'backspace'
	errmsg = did_set_backspace();
    else if (varp == &p_bo)			// 'belloff'
	errmsg = did_set_opt_flags(p_bo, p_bo_values, &bo_flags, TRUE);
    else if (gvarp == &p_tc)			// 'tagcase'
	errmsg = did_set_tagcase(opt_flags);
    else if (varp == &p_cmp)			// 'casemap'
	errmsg = did_set_opt_flags(p_cmp, p_cmp_values, &cmp_flags, TRUE);
#ifdef FEAT_DIFF
    else if (varp == &p_dip)			// 'diffopt'
	errmsg = did_set_diffopt();
#endif
#ifdef FEAT_FOLDING
    else if (gvarp == &curwin->w_allbuf_opt.wo_fdm)	// 'foldmethod'
	errmsg = did_set_foldmethod(varp);
    else if (gvarp == &curwin->w_allbuf_opt.wo_fmr)	// 'foldmarker'
	errmsg = did_set_foldmarker(varp);
    else if (gvarp == &p_cms)			// 'commentstring'
	errmsg = did_set_commentstring(varp);
    else if (varp == &p_fdo)			// 'foldopen'
	errmsg = did_set_opt_flags(p_fdo, p_fdo_values, &fdo_flags, TRUE);
    else if (varp == &p_fcl)			// 'foldclose'
	errmsg = did_set_opt_strings(p_fcl, p_fcl_values, TRUE);
    else if (gvarp == &curwin->w_allbuf_opt.wo_fdi)	// 'foldignore'
	did_set_foldignore();
#endif
    else if (gvarp == &p_ve)			// 'virtualedit'
	errmsg = did_set_virtualedit(oldval, opt_flags);
#if defined(FEAT_CSCOPE) && defined(FEAT_QUICKFIX)
    else if (varp == &p_csqf)			// 'cscopequickfix'
	errmsg = did_set_cscopequickfix();
#endif
    else if (gvarp == &p_cino)			// 'cinoptions'
	did_set_cinoptions();
    else if (gvarp == &p_lop)			// 'lispoptions'
	errmsg = did_set_lispoptions(varp);
#if defined(FEAT_RENDER_OPTIONS)
    else if (varp == &p_rop)			// 'renderoptions'
	errmsg = did_set_renderoptions();
#endif
    else if (gvarp == &p_ft)			// 'filetype'
	errmsg = did_set_filetype_or_syntax(varp, oldval, value_checked,
							&value_changed);
#ifdef FEAT_SYN_HL
    else if (gvarp == &p_syn)			// 'syntax'
	errmsg = did_set_filetype_or_syntax(varp, oldval, value_checked,
							&value_changed);
#endif
#ifdef FEAT_TERMINAL
    else if (varp == &curwin->w_p_twk)		// 'termwinkey'
	errmsg = did_set_termwinkey();
    else if (varp == &curwin->w_p_tws)		// 'termwinsize'
	errmsg = did_set_termwinsize();
    else if (varp == &curwin->w_p_wcr)		// 'wincolor'
	term_update_wincolor(curwin);
# if defined(MSWIN)
    else if (varp == &p_twt)			// 'termwintype'
	errmsg = did_set_opt_strings(p_twt, p_twt_values, FALSE);
# endif
#endif
#ifdef FEAT_VARTABS
    else if (varp == &(curbuf->b_p_vsts))	// 'varsofttabstop'
	errmsg = did_set_varsofttabstop(varp);
    else if (varp == &(curbuf->b_p_vts))	// 'vartabstop'
	errmsg = did_set_vartabstop(varp);
#endif
#ifdef FEAT_PROP_POPUP
    else if (varp == &p_pvp)			// 'previewpopup'
	errmsg = did_set_previewpopup();
# ifdef FEAT_QUICKFIX
    else if (varp == &p_cpp)			// 'completepopup'
	errmsg = did_set_completepopup();
# endif
#endif
#ifdef FEAT_EVAL
    else if (
# ifdef FEAT_BEVAL
	    varp == &p_bexpr ||			// 'balloonexpr'
# endif
# ifdef FEAT_DIFF
	    varp == &p_dex ||			// 'diffexpr'
# endif
# ifdef FEAT_FOLDING
	    gvarp == &curwin->w_allbuf_opt.wo_fde ||	// 'foldexpr'
	    gvarp == &curwin->w_allbuf_opt.wo_fdt ||	// 'foldtext'
# endif
	    gvarp == &p_fex ||			// 'formatexpr'
# ifdef FEAT_FIND_ID
	    gvarp == &p_inex ||			// 'includeexpr'
# endif
	    gvarp == &p_inde ||			// 'indentexpr'
# ifdef FEAT_DIFF
	    varp == &p_pex ||			// 'patchexpr'
# endif
# ifdef FEAT_POSTSCRIPT
	    varp == &p_pexpr ||			// 'printexpr'
# endif
	    varp == &p_ccv)			// 'charconvert'
	did_set_optexpr(varp);
#endif
#ifdef FEAT_COMPL_FUNC
    else if (gvarp == &p_cfu)			// 'completefunc'
	errmsg = set_completefunc_option();
    else if (gvarp == &p_ofu)			// 'omnifunc'
	errmsg = set_omnifunc_option();
    else if (gvarp == &p_tsrfu)			// 'thesaurusfunc'
	errmsg = set_thesaurusfunc_option();
#endif
#if defined(FEAT_EVAL) && \
     (defined(FEAT_XIM) || defined(IME_WITHOUT_XIM) || defined(VIMDLL))
    else if (gvarp == &p_imaf)			// 'imactivatefunc'
	errmsg = set_imactivatefunc_option();
    else if (gvarp == &p_imsf)			// 'imstatusfunc'
	errmsg = set_imstatusfunc_option();
#endif
    else if (varp == &p_opfunc)			// 'operatorfunc'
	errmsg = set_operatorfunc_option();
#ifdef FEAT_QUICKFIX
    else if (varp == &p_qftf)			// 'quickfixtextfunc'
	errmsg = qf_process_qftf_option();
#endif
#ifdef FEAT_EVAL
    else if (gvarp == &p_tfu)			// 'tagfunc'
	errmsg = set_tagfunc_option();
#endif
    else if (varp == &p_ww)			// 'whichwrap'
	errmsg = did_set_option_listflag(varp, (char_u *)WW_ALL, errbuf);
    else if (varp == &p_shm)			// 'shortmess'
	errmsg = did_set_option_listflag(varp, (char_u *)SHM_ALL, errbuf);
    else if (varp == &(p_cpo))			// 'cpoptions'
	errmsg = did_set_option_listflag(varp, (char_u *)CPO_ALL, errbuf);
    else if (varp == &(curbuf->b_p_fo))		// 'formatoptions'
	errmsg = did_set_option_listflag(varp, (char_u *)FO_ALL, errbuf);
#ifdef FEAT_CONCEAL
    else if (varp == &curwin->w_p_cocu)		// 'concealcursor'
	errmsg = did_set_option_listflag(varp, (char_u *)COCU_ALL, errbuf);
#endif
    else if (varp == &p_mouse)			// 'mouse'
	errmsg = did_set_option_listflag(varp, (char_u *)MOUSE_ALL, errbuf);
#if defined(FEAT_GUI)
    else if (varp == &p_go)			// 'guioptions'
	errmsg = did_set_option_listflag(varp, (char_u *)GO_ALL, errbuf);
#endif

    // If an error is detected, restore the previous value.
    if (errmsg != NULL)
    {
	free_string_option(*varp);
	*varp = oldval;
	// When resetting some values, need to act on it.
	if (did_chartab)
	    (void)init_chartab();
	if (varp == &p_hl)
	    (void)highlight_changed();
    }
    else
    {
#ifdef FEAT_EVAL
	// Remember where the option was set.
	set_option_sctx_idx(opt_idx, opt_flags, current_sctx);
#endif
	// Free string options that are in allocated memory.
	// Use "free_oldval", because recursiveness may change the flags under
	// our fingers (esp. init_highlight()).
	if (free_oldval)
	    free_string_option(oldval);
	set_option_flag(opt_idx, P_ALLOCED);

	if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0
		&& is_global_local_option(opt_idx))
	{
	    // global option with local value set to use global value; free
	    // the local value and make it empty
	    char_u *p = get_option_varp_scope(opt_idx, OPT_LOCAL);
	    free_string_option(*(char_u **)p);
	    *(char_u **)p = empty_option;
	}

	// May set global value for local option.
	else if (!(opt_flags & OPT_LOCAL) && opt_flags != OPT_GLOBAL)
	    set_string_option_global(opt_idx, varp);

	// Trigger the autocommand only after setting the flags.
#ifdef FEAT_SYN_HL
	if (varp == &(curbuf->b_p_syn))
	    do_syntax_autocmd(value_changed);
#endif
	else if (varp == &(curbuf->b_p_ft))
	    do_filetype_autocmd(varp, opt_flags, value_changed);
#ifdef FEAT_SPELL
	if (varp == &(curwin->w_s->b_p_spl))
	    do_spelllang_source();
#endif
    }

    if (varp == &p_mouse)
    {
	if (*p_mouse == NUL)
	    mch_setmouse(FALSE);    // switch mouse off
	else
	    setmouse();		    // in case 'mouse' changed
    }

#if defined(FEAT_LUA) || defined(PROTO)
    if (varp == &p_rtp)
	update_package_paths_in_lua();
#endif

#if defined(FEAT_LINEBREAK)
    // Changing Formatlistpattern when briopt includes the list setting:
    // redraw
    if ((varp == &p_flp || varp == &(curbuf->b_p_flp))
	    && curwin->w_briopt_list)
	redraw_all_later(UPD_NOT_VALID);
#endif

    if (curwin->w_curswant != MAXCOL
		   && (get_option_flags(opt_idx) & (P_CURSWANT | P_RALL)) != 0)
	curwin->w_set_curswant = TRUE;

    if ((opt_flags & OPT_NO_REDRAW) == 0)
    {
#ifdef FEAT_GUI
	// check redraw when it's not a GUI option or the GUI is active.
	if (!redraw_gui_only || gui.in_use)
#endif
	    check_redraw(get_option_flags(opt_idx));
    }

#if defined(FEAT_VTP) && defined(FEAT_TERMGUICOLORS)
    if (did_swaptcap)
    {
	set_termname((char_u *)"win32");
	init_highlight(TRUE, FALSE);
    }
#endif

    return errmsg;
}

/*
 * Check an option that can be a range of string values.
 *
 * Return OK for correct value, FAIL otherwise.
 * Empty is always OK.
 */
    static int
check_opt_strings(
    char_u	*val,
    char	**values,
    int		list)	    // when TRUE: accept a list of values
{
    return opt_strings_flags(val, values, NULL, list);
}

/*
 * Handle an option that can be a range of string values.
 * Set a flag in "*flagp" for each string present.
 *
 * Return OK for correct value, FAIL otherwise.
 * Empty is always OK.
 */
    static int
opt_strings_flags(
    char_u	*val,		// new value
    char	**values,	// array of valid string values
    unsigned	*flagp,
    int		list)		// when TRUE: accept a list of values
{
    int		i;
    int		len;
    unsigned	new_flags = 0;

    while (*val)
    {
	for (i = 0; ; ++i)
	{
	    if (values[i] == NULL)	// val not found in values[]
		return FAIL;

	    len = (int)STRLEN(values[i]);
	    if (STRNCMP(values[i], val, len) == 0
		    && ((list && val[len] == ',') || val[len] == NUL))
	    {
		val += len + (val[len] == ',');
		new_flags |= (1 << i);
		break;		// check next item in val list
	    }
	}
    }
    if (flagp != NULL)
	*flagp = new_flags;

    return OK;
}

/*
 * return OK if "p" is a valid fileformat name, FAIL otherwise.
 */
    int
check_ff_value(char_u *p)
{
    return check_opt_strings(p, p_ff_values, FALSE);
}

/*
 * Save the acutal shortmess Flags and clear them
 * temporarily to avoid that file messages
 * overwrites any output from the following commands.
 *
 * Caller must make sure to first call save_clear_shm_value() and then
 * restore_shm_value() exactly the same number of times.
 */
    void
save_clear_shm_value()
{
    if (STRLEN(p_shm) >= SHM_LEN)
    {
	iemsg(e_internal_error_shortmess_too_long);
	return;
    }

    if (++set_shm_recursive == 1)
    {
	STRCPY(shm_buf, p_shm);
	set_option_value_give_err((char_u *)"shm", 0L, (char_u *)"", 0);
    }
}

/*
 * Restore the shortmess Flags set from the save_clear_shm_value() function.
 */
    void
restore_shm_value()
{
    if (--set_shm_recursive == 0)
    {
	set_option_value_give_err((char_u *)"shm", 0L, shm_buf, 0);
	vim_memset(shm_buf, 0, SHM_LEN);
    }
}
