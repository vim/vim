/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Code to handle user-settable options. This is all pretty much table-
 * driven. Checklist for adding a new option:
 * - Put it in the options array below (copy an existing entry).
 * - For a global option: Add a variable for it in option.h.
 * - For a buffer or window local option:
 *   - Add a PV_XX entry to the enum below.
 *   - Add a variable to the window or buffer struct in structs.h.
 *   - For a window option, add some code to copy_winopt().
 *   - For a buffer option, add some code to buf_copy_options().
 *   - For a buffer string option, add code to check_buf_options().
 * - If it's a numeric option, add any necessary bounds checks to do_set().
 * - If it's a list of flags, add some code in do_set(), search for WW_ALL.
 * - When adding an option with expansion (P_EXPAND), but with a different
 *   default for Vi and Vim (no P_VI_DEF), add some code at VIMEXP.
 * - Add documentation!  One line in doc/quickref.txt, full description in
 *   options.txt, and any other related places.
 * - Add an entry in runtime/optwin.vim.
 * When making changes:
 * - Adjust the help for the option in doc/option.txt.
 * - When an entry has the P_VIM flag, or is lacking the P_VI_DEF flag, add a
 *   comment at the help for the 'compatible' option.
 */

#define IN_OPTION_C
#include "vim.h"
#include "optiondefs.h"

static void set_options_default(int opt_flags);
static void set_string_default_esc(char *name, char_u *val, int escape);
static char_u *find_dup_item(char_u *origval, char_u *newval, long_u flags);
static char_u *option_expand(int opt_idx, char_u *val);
static void didset_options(void);
static void didset_options2(void);
#if defined(FEAT_EVAL) || defined(PROTO)
static long_u *insecure_flag(int opt_idx, int opt_flags);
#else
# define insecure_flag(opt_idx, opt_flags) (&options[opt_idx].flags)
#endif
static char *set_bool_option(int opt_idx, char_u *varp, int value, int opt_flags);
static char *set_num_option(int opt_idx, char_u *varp, long value, char *errbuf, size_t errbuflen, int opt_flags);
static int find_key_option(char_u *arg_arg, int has_lt);
static void showoptions(int all, int opt_flags);
static int optval_default(struct vimoption *, char_u *varp, int compatible);
static void showoneopt(struct vimoption *, int opt_flags);
static int put_setstring(FILE *fd, char *cmd, char *name, char_u **valuep, long_u flags);
static int put_setnum(FILE *fd, char *cmd, char *name, long *valuep);
static int put_setbool(FILE *fd, char *cmd, char *name, int value);
static int istermoption(struct vimoption *p);
static char_u *get_varp_scope(struct vimoption *p, int opt_flags);
static char_u *get_varp(struct vimoption *);
static void check_win_options(win_T *win);
static void option_value2string(struct vimoption *, int opt_flags);
static void check_winopt(winopt_T *wop);
static int wc_use_keyname(char_u *varp, long *wcp);
static void paste_option_changed(void);
static void compatible_set(void);

/*
 * Initialize the options, first part.
 *
 * Called only once from main(), just after creating the first buffer.
 * If "clean_arg" is TRUE Vim was started with --clean.
 */
    void
set_init_1(int clean_arg)
{
    char_u	*p;
    int		opt_idx;
    long_u	n;

#ifdef FEAT_LANGMAP
    langmap_init();
#endif

    // Be Vi compatible by default
    p_cp = TRUE;

    // Use POSIX compatibility when $VIM_POSIX is set.
    if (mch_getenv((char_u *)"VIM_POSIX") != NULL)
    {
	set_string_default("cpo", (char_u *)CPO_ALL);
	set_string_default("shm", (char_u *)SHM_POSIX);
    }

    /*
     * Find default value for 'shell' option.
     * Don't use it if it is empty.
     */
    if (((p = mch_getenv((char_u *)"SHELL")) != NULL && *p != NUL)
#if defined(MSWIN)
	    || ((p = mch_getenv((char_u *)"COMSPEC")) != NULL && *p != NUL)
	    || ((p = (char_u *)default_shell()) != NULL && *p != NUL)
#endif
	    )
#if defined(MSWIN)
    {
	// For MS-Windows put the path in quotes instead of escaping spaces.
	char_u	    *cmd;
	size_t	    len;

	if (vim_strchr(p, ' ') != NULL)
	{
	    len = STRLEN(p) + 3;  // two quotes and a trailing NUL
	    cmd = alloc(len);
	    if (cmd != NULL)
	    {
		vim_snprintf((char *)cmd, len, "\"%s\"", p);
		set_string_default("sh", cmd);
		vim_free(cmd);
	    }
	}
	else
	    set_string_default("sh", p);
    }
#else
	set_string_default_esc("sh", p, TRUE);
#endif

#ifdef FEAT_WILDIGN
    /*
     * Set the default for 'backupskip' to include environment variables for
     * temp files.
     */
    {
# ifdef UNIX
	static char	*(names[4]) = {"", "TMPDIR", "TEMP", "TMP"};
# else
	static char	*(names[3]) = {"TMPDIR", "TEMP", "TMP"};
# endif
	int		len;
	garray_T	ga;
	int		mustfree;
	char_u		*item;

	opt_idx = findoption((char_u *)"backupskip");

	ga_init2(&ga, 1, 100);
	for (n = 0; n < (long)ARRAY_LENGTH(names); ++n)
	{
	    mustfree = FALSE;
# ifdef UNIX
	    if (*names[n] == NUL)
#  ifdef MACOS_X
		p = (char_u *)"/private/tmp";
#  else
		p = (char_u *)"/tmp";
#  endif
	    else
# endif
		p = vim_getenv((char_u *)names[n], &mustfree);
	    if (p != NULL && *p != NUL)
	    {
		// First time count the NUL, otherwise count the ','.
		len = (int)STRLEN(p) + 3;
		item = alloc(len);
		STRCPY(item, p);
		add_pathsep(item);
		STRCAT(item, "*");
		if (find_dup_item(ga.ga_data, item, options[opt_idx].flags)
									== NULL
			&& ga_grow(&ga, len) == OK)
		{
		    if (ga.ga_len > 0)
			STRCAT(ga.ga_data, ",");
		    STRCAT(ga.ga_data, item);
		    ga.ga_len += len;
		}
		vim_free(item);
	    }
	    if (mustfree)
		vim_free(p);
	}
	if (ga.ga_data != NULL)
	{
	    set_string_default("bsk", ga.ga_data);
	    vim_free(ga.ga_data);
	}
    }
#endif

    /*
     * 'maxmemtot' and 'maxmem' may have to be adjusted for available memory
     */
    opt_idx = findoption((char_u *)"maxmemtot");
    if (opt_idx >= 0)
    {
#if !defined(HAVE_AVAIL_MEM) && !defined(HAVE_TOTAL_MEM)
	if (options[opt_idx].def_val[VI_DEFAULT] == (char_u *)0L)
#endif
	{
#ifdef HAVE_AVAIL_MEM
	    // Use amount of memory available at this moment.
	    n = (mch_avail_mem(FALSE) >> 1);
#else
# ifdef HAVE_TOTAL_MEM
	    // Use amount of memory available to Vim.
	    n = (mch_total_mem(FALSE) >> 1);
# else
	    n = (0x7fffffff >> 11);
# endif
#endif
	    options[opt_idx].def_val[VI_DEFAULT] = (char_u *)n;
	    opt_idx = findoption((char_u *)"maxmem");
	    if (opt_idx >= 0)
	    {
#if !defined(HAVE_AVAIL_MEM) && !defined(HAVE_TOTAL_MEM)
		if ((long)(long_i)options[opt_idx].def_val[VI_DEFAULT] > (long)n
		  || (long)(long_i)options[opt_idx].def_val[VI_DEFAULT] == 0L)
#endif
		    options[opt_idx].def_val[VI_DEFAULT] = (char_u *)n;
	    }
	}
    }

#ifdef FEAT_SEARCHPATH
    {
	char_u	*cdpath;
	char_u	*buf;
	int	i;
	int	j;
	int	mustfree = FALSE;

	// Initialize the 'cdpath' option's default value.
	cdpath = vim_getenv((char_u *)"CDPATH", &mustfree);
	if (cdpath != NULL)
	{
	    buf = alloc((STRLEN(cdpath) << 1) + 2);
	    if (buf != NULL)
	    {
		buf[0] = ',';	    // start with ",", current dir first
		j = 1;
		for (i = 0; cdpath[i] != NUL; ++i)
		{
		    if (vim_ispathlistsep(cdpath[i]))
			buf[j++] = ',';
		    else
		    {
			if (cdpath[i] == ' ' || cdpath[i] == ',')
			    buf[j++] = '\\';
			buf[j++] = cdpath[i];
		    }
		}
		buf[j] = NUL;
		opt_idx = findoption((char_u *)"cdpath");
		if (opt_idx >= 0)
		{
		    options[opt_idx].def_val[VI_DEFAULT] = buf;
		    options[opt_idx].flags |= P_DEF_ALLOCED;
		}
		else
		    vim_free(buf); // cannot happen
	    }
	    if (mustfree)
		vim_free(cdpath);
	}
    }
#endif

#if defined(FEAT_POSTSCRIPT) && (defined(MSWIN) || defined(VMS) || defined(EBCDIC) || defined(MAC) || defined(hpux))
    // Set print encoding on platforms that don't default to latin1
    set_string_default("penc",
# if defined(MSWIN)
		       (char_u *)"cp1252"
# else
#  ifdef VMS
		       (char_u *)"dec-mcs"
#  else
#   ifdef EBCDIC
		       (char_u *)"ebcdic-uk"
#   else
#    ifdef MAC
		       (char_u *)"mac-roman"
#    else // HPUX
		       (char_u *)"hp-roman8"
#    endif
#   endif
#  endif
# endif
		       );
#endif

#ifdef FEAT_POSTSCRIPT
    // 'printexpr' must be allocated to be able to evaluate it.
    set_string_default("pexpr",
# if defined(MSWIN)
	    (char_u *)"system('copy' . ' ' . v:fname_in . (&printdevice == '' ? ' LPT1:' : (' \"' . &printdevice . '\"'))) . delete(v:fname_in)"
# else
#  ifdef VMS
	    (char_u *)"system('print/delete' . (&printdevice == '' ? '' : ' /queue=' . &printdevice) . ' ' . v:fname_in)"

#  else
	    (char_u *)"system('lpr' . (&printdevice == '' ? '' : ' -P' . &printdevice) . ' ' . v:fname_in) . delete(v:fname_in) + v:shell_error"
#  endif
# endif
	    );
#endif

    /*
     * Set all the options (except the terminal options) to their default
     * value.  Also set the global value for local options.
     */
    set_options_default(0);

#ifdef CLEAN_RUNTIMEPATH
    if (clean_arg)
    {
	opt_idx = findoption((char_u *)"runtimepath");
	if (opt_idx >= 0)
	{
	    options[opt_idx].def_val[VI_DEFAULT] = (char_u *)CLEAN_RUNTIMEPATH;
	    p_rtp = (char_u *)CLEAN_RUNTIMEPATH;
	}
	opt_idx = findoption((char_u *)"packpath");
	if (opt_idx >= 0)
	{
	    options[opt_idx].def_val[VI_DEFAULT] = (char_u *)CLEAN_RUNTIMEPATH;
	    p_pp = (char_u *)CLEAN_RUNTIMEPATH;
	}
    }
#endif

#ifdef FEAT_GUI
    if (found_reverse_arg)
	set_option_value((char_u *)"bg", 0L, (char_u *)"dark", 0);
#endif

    curbuf->b_p_initialized = TRUE;
    curbuf->b_p_ar = -1;	// no local 'autoread' value
    curbuf->b_p_ul = NO_LOCAL_UNDOLEVEL;
    check_buf_options(curbuf);
    check_win_options(curwin);
    check_options();

    // Must be before option_expand(), because that one needs vim_isIDc()
    didset_options();

#ifdef FEAT_SPELL
    // Use the current chartab for the generic chartab. This is not in
    // didset_options() because it only depends on 'encoding'.
    init_spell_chartab();
#endif

    /*
     * Expand environment variables and things like "~" for the defaults.
     * If option_expand() returns non-NULL the variable is expanded.  This can
     * only happen for non-indirect options.
     * Also set the default to the expanded value, so ":set" does not list
     * them.
     * Don't set the P_ALLOCED flag, because we don't want to free the
     * default.
     */
    for (opt_idx = 0; !istermoption_idx(opt_idx); opt_idx++)
    {
	if ((options[opt_idx].flags & P_GETTEXT)
					      && options[opt_idx].var != NULL)
	    p = (char_u *)_(*(char **)options[opt_idx].var);
	else
	    p = option_expand(opt_idx, NULL);
	if (p != NULL && (p = vim_strsave(p)) != NULL)
	{
	    *(char_u **)options[opt_idx].var = p;
	    // VIMEXP
	    // Defaults for all expanded options are currently the same for Vi
	    // and Vim.  When this changes, add some code here!  Also need to
	    // split P_DEF_ALLOCED in two.
	    if (options[opt_idx].flags & P_DEF_ALLOCED)
		vim_free(options[opt_idx].def_val[VI_DEFAULT]);
	    options[opt_idx].def_val[VI_DEFAULT] = p;
	    options[opt_idx].flags |= P_DEF_ALLOCED;
	}
    }

    save_file_ff(curbuf);	// Buffer is unchanged

#if defined(FEAT_ARABIC)
    // Detect use of mlterm.
    // Mlterm is a terminal emulator akin to xterm that has some special
    // abilities (bidi namely).
    // NOTE: mlterm's author is being asked to 'set' a variable
    //       instead of an environment variable due to inheritance.
    if (mch_getenv((char_u *)"MLTERM") != NULL)
	set_option_value((char_u *)"tbidi", 1L, NULL, 0);
#endif

    didset_options2();

# if defined(MSWIN) && defined(FEAT_GETTEXT)
    /*
     * If $LANG isn't set, try to get a good value for it.  This makes the
     * right language be used automatically.  Don't do this for English.
     */
    if (mch_getenv((char_u *)"LANG") == NULL)
    {
	char	buf[20];

	// Could use LOCALE_SISO639LANGNAME, but it's not in Win95.
	// LOCALE_SABBREVLANGNAME gives us three letters, like "enu", we use
	// only the first two.
	n = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME,
							     (LPTSTR)buf, 20);
	if (n >= 2 && STRNICMP(buf, "en", 2) != 0)
	{
	    // There are a few exceptions (probably more)
	    if (STRNICMP(buf, "cht", 3) == 0 || STRNICMP(buf, "zht", 3) == 0)
		STRCPY(buf, "zh_TW");
	    else if (STRNICMP(buf, "chs", 3) == 0
					      || STRNICMP(buf, "zhc", 3) == 0)
		STRCPY(buf, "zh_CN");
	    else if (STRNICMP(buf, "jp", 2) == 0)
		STRCPY(buf, "ja");
	    else
		buf[2] = NUL;		// truncate to two-letter code
	    vim_setenv((char_u *)"LANG", (char_u *)buf);
	}
    }
# else
#  ifdef MACOS_CONVERT
    // Moved to os_mac_conv.c to avoid dependency problems.
    mac_lang_init();
#  endif
# endif

# ifdef MSWIN
    // MS-Windows has builtin support for conversion to and from Unicode, using
    // "utf-8" for 'encoding' should work best for most users.
    p = vim_strsave((char_u *)ENC_DFLT);
# else
    // enc_locale() will try to find the encoding of the current locale.
    // This works best for properly configured systems, old and new.
    p = enc_locale();
# endif
    if (p != NULL)
    {
	char_u *save_enc;

	// Try setting 'encoding' and check if the value is valid.
	// If not, go back to the default encoding.
	save_enc = p_enc;
	p_enc = p;
	if (STRCMP(p_enc, "gb18030") == 0)
	{
	    // We don't support "gb18030", but "cp936" is a good substitute
	    // for practical purposes, thus use that.  It's not an alias to
	    // still support conversion between gb18030 and utf-8.
	    p_enc = vim_strsave((char_u *)"cp936");
	    vim_free(p);
	}
	if (mb_init() == NULL)
	{
	    opt_idx = findoption((char_u *)"encoding");
	    if (opt_idx >= 0)
	    {
		options[opt_idx].def_val[VI_DEFAULT] = p_enc;
		options[opt_idx].flags |= P_DEF_ALLOCED;
	    }

#if defined(MSWIN) || defined(MACOS_X) || defined(VMS)
	    if (STRCMP(p_enc, "latin1") == 0 || enc_utf8)
	    {
		// Adjust the default for 'isprint' and 'iskeyword' to match
		// latin1.  Also set the defaults for when 'nocompatible' is
		// set.
		set_string_option_direct((char_u *)"isp", -1,
					      ISP_LATIN1, OPT_FREE, SID_NONE);
		set_string_option_direct((char_u *)"isk", -1,
					      ISK_LATIN1, OPT_FREE, SID_NONE);
		opt_idx = findoption((char_u *)"isp");
		if (opt_idx >= 0)
		    options[opt_idx].def_val[VIM_DEFAULT] = ISP_LATIN1;
		opt_idx = findoption((char_u *)"isk");
		if (opt_idx >= 0)
		    options[opt_idx].def_val[VIM_DEFAULT] = ISK_LATIN1;
		(void)init_chartab();
	    }
#endif

#if defined(MSWIN) && (!defined(FEAT_GUI) || defined(VIMDLL))
	    // Win32 console: When GetACP() returns a different value from
	    // GetConsoleCP() set 'termencoding'.
	    if (
# ifdef VIMDLL
	       (!gui.in_use && !gui.starting) &&
# endif
	        GetACP() != GetConsoleCP())
	    {
		char	buf[50];

		// Win32 console: In ConPTY, GetConsoleCP() returns zero.
		// Use an alternative value.
		if (GetConsoleCP() == 0)
		    sprintf(buf, "cp%ld", (long)GetACP());
		else
		    sprintf(buf, "cp%ld", (long)GetConsoleCP());
		p_tenc = vim_strsave((char_u *)buf);
		if (p_tenc != NULL)
		{
		    opt_idx = findoption((char_u *)"termencoding");
		    if (opt_idx >= 0)
		    {
			options[opt_idx].def_val[VI_DEFAULT] = p_tenc;
			options[opt_idx].flags |= P_DEF_ALLOCED;
		    }
		    convert_setup(&input_conv, p_tenc, p_enc);
		    convert_setup(&output_conv, p_enc, p_tenc);
		}
		else
		    p_tenc = empty_option;
	    }
#endif
#if defined(MSWIN)
	    // $HOME may have characters in active code page.
	    init_homedir();
#endif
	}
	else
	{
	    vim_free(p_enc);
	    p_enc = save_enc;
	}
    }

#ifdef FEAT_MULTI_LANG
    // Set the default for 'helplang'.
    set_helplang_default(get_mess_lang());
#endif
}

static char_u *fencs_utf8_default = (char_u *)"ucs-bom,utf-8,default,latin1";

/*
 * Set the "fileencodings" option to the default value for when 'encoding' is
 * utf-8.
 */
    void
set_fencs_unicode()
{
    set_string_option_direct((char_u *)"fencs", -1, fencs_utf8_default,
								  OPT_FREE, 0);
}

/*
 * Set an option to its default value.
 * This does not take care of side effects!
 */
    static void
set_option_default(
    int		opt_idx,
    int		opt_flags,	// OPT_FREE, OPT_LOCAL and/or OPT_GLOBAL
    int		compatible)	// use Vi default value
{
    char_u	*varp;		// pointer to variable for current option
    int		dvi;		// index in def_val[]
    long_u	flags;
    long_u	*flagsp;
    int		both = (opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0;

    varp = get_varp_scope(&(options[opt_idx]), both ? OPT_LOCAL : opt_flags);
    flags = options[opt_idx].flags;
    if (varp != NULL)	    // skip hidden option, nothing to do for it
    {
	dvi = ((flags & P_VI_DEF) || compatible) ? VI_DEFAULT : VIM_DEFAULT;
	if (flags & P_STRING)
	{
	    // 'fencs' default value depends on 'encoding'
	    if (options[opt_idx].var == (char_u *)&p_fencs && enc_utf8)
		set_fencs_unicode();
	    // Use set_string_option_direct() for local options to handle
	    // freeing and allocating the value.
	    else if (options[opt_idx].indir != PV_NONE)
		set_string_option_direct(NULL, opt_idx,
				 options[opt_idx].def_val[dvi], opt_flags, 0);
	    else
	    {
		if ((opt_flags & OPT_FREE) && (flags & P_ALLOCED))
		    free_string_option(*(char_u **)(varp));
		*(char_u **)varp = options[opt_idx].def_val[dvi];
		options[opt_idx].flags &= ~P_ALLOCED;
	    }
	}
	else if (flags & P_NUM)
	{
	    if (options[opt_idx].indir == PV_SCROLL)
		win_comp_scroll(curwin);
	    else
	    {
		long def_val = (long)(long_i)options[opt_idx].def_val[dvi];

		if ((long *)varp == &curwin->w_p_so
			|| (long *)varp == &curwin->w_p_siso)
		    // 'scrolloff' and 'sidescrolloff' local values have a
		    // different default value than the global default.
		    *(long *)varp = -1;
		else
		    *(long *)varp = def_val;
		// May also set global value for local option.
		if (both)
		    *(long *)get_varp_scope(&(options[opt_idx]), OPT_GLOBAL) =
								def_val;
	    }
	}
	else	// P_BOOL
	{
	    // the cast to long is required for Manx C, long_i is needed for
	    // MSVC
	    *(int *)varp = (int)(long)(long_i)options[opt_idx].def_val[dvi];
#ifdef UNIX
	    // 'modeline' defaults to off for root
	    if (options[opt_idx].indir == PV_ML && getuid() == ROOT_UID)
		*(int *)varp = FALSE;
#endif
	    // May also set global value for local option.
	    if (both)
		*(int *)get_varp_scope(&(options[opt_idx]), OPT_GLOBAL) =
								*(int *)varp;
	}

	// The default value is not insecure.
	flagsp = insecure_flag(opt_idx, opt_flags);
	*flagsp = *flagsp & ~P_INSECURE;
    }

#ifdef FEAT_EVAL
    set_option_sctx_idx(opt_idx, opt_flags, current_sctx);
#endif
}

/*
 * Set all options (except terminal options) to their default value.
 * When "opt_flags" is non-zero skip 'encoding'.
 */
    static void
set_options_default(
    int		opt_flags)	// OPT_FREE, OPT_LOCAL and/or OPT_GLOBAL
{
    int		i;
    win_T	*wp;
    tabpage_T	*tp;

    for (i = 0; !istermoption_idx(i); i++)
	if (!(options[i].flags & P_NODEFAULT)
		&& (opt_flags == 0
		    || (options[i].var != (char_u *)&p_enc
# if defined(FEAT_CRYPT)
			&& options[i].var != (char_u *)&p_cm
			&& options[i].var != (char_u *)&p_key
# endif
			)))
	    set_option_default(i, opt_flags, p_cp);

    // The 'scroll' option must be computed for all windows.
    FOR_ALL_TAB_WINDOWS(tp, wp)
	win_comp_scroll(wp);
#ifdef FEAT_CINDENT
    parse_cino(curbuf);
#endif
}

/*
 * Set the Vi-default value of a string option.
 * Used for 'sh', 'backupskip' and 'term'.
 * When "escape" is TRUE escape spaces with a backslash.
 */
    static void
set_string_default_esc(char *name, char_u *val, int escape)
{
    char_u	*p;
    int		opt_idx;

    if (escape && vim_strchr(val, ' ') != NULL)
	p = vim_strsave_escaped(val, (char_u *)" ");
    else
	p = vim_strsave(val);
    if (p != NULL)		// we don't want a NULL
    {
	opt_idx = findoption((char_u *)name);
	if (opt_idx >= 0)
	{
	    if (options[opt_idx].flags & P_DEF_ALLOCED)
		vim_free(options[opt_idx].def_val[VI_DEFAULT]);
	    options[opt_idx].def_val[VI_DEFAULT] = p;
	    options[opt_idx].flags |= P_DEF_ALLOCED;
	}
    }
}

    void
set_string_default(char *name, char_u *val)
{
    set_string_default_esc(name, val, FALSE);
}

/*
 * For an option value that contains comma separated items, find "newval" in
 * "origval".  Return NULL if not found.
 */
    static char_u *
find_dup_item(char_u *origval, char_u *newval, long_u flags)
{
    int	    bs = 0;
    size_t  newlen;
    char_u  *s;

    if (origval == NULL)
	return NULL;

    newlen = STRLEN(newval);
    for (s = origval; *s != NUL; ++s)
    {
	if ((!(flags & P_COMMA)
		    || s == origval
		    || (s[-1] == ',' && !(bs & 1)))
		&& STRNCMP(s, newval, newlen) == 0
		&& (!(flags & P_COMMA)
		    || s[newlen] == ','
		    || s[newlen] == NUL))
	    return s;
	// Count backslashes.  Only a comma with an even number of backslashes
	// or a single backslash preceded by a comma before it is recognized as
	// a separator.
	if ((s > origval + 1
		    && s[-1] == '\\'
		    && s[-2] != ',')
		|| (s == origval + 1
		    && s[-1] == '\\'))
	    ++bs;
	else
	    bs = 0;
    }
    return NULL;
}

/*
 * Set the Vi-default value of a number option.
 * Used for 'lines' and 'columns'.
 */
    void
set_number_default(char *name, long val)
{
    int		opt_idx;

    opt_idx = findoption((char_u *)name);
    if (opt_idx >= 0)
	options[opt_idx].def_val[VI_DEFAULT] = (char_u *)(long_i)val;
}

/*
 * Set all window-local and buffer-local options to the Vim default.
 * local-global options will use the global value.
 * When "do_buffer" is FALSE don't set buffer-local options.
 */
    void
set_local_options_default(win_T *wp, int do_buffer)
{
    win_T	*save_curwin = curwin;
    int		i;

    curwin = wp;
    curbuf = curwin->w_buffer;
    block_autocmds();

    for (i = 0; !istermoption_idx(i); i++)
    {
	struct vimoption    *p = &(options[i]);
	char_u		    *varp = get_varp_scope(p, OPT_LOCAL);

	if (p->indir != PV_NONE
		&& (do_buffer || (p->indir & PV_BUF) == 0)
		&& !(options[i].flags & P_NODEFAULT)
		&& !optval_default(p, varp, FALSE))
	    set_option_default(i, OPT_FREE|OPT_LOCAL, FALSE);
    }

    unblock_autocmds();
    curwin = save_curwin;
    curbuf = curwin->w_buffer;
}

#if defined(EXITFREE) || defined(PROTO)
/*
 * Free all options.
 */
    void
free_all_options(void)
{
    int		i;

    for (i = 0; !istermoption_idx(i); i++)
    {
	if (options[i].indir == PV_NONE)
	{
	    // global option: free value and default value.
	    if ((options[i].flags & P_ALLOCED) && options[i].var != NULL)
		free_string_option(*(char_u **)options[i].var);
	    if (options[i].flags & P_DEF_ALLOCED)
		free_string_option(options[i].def_val[VI_DEFAULT]);
	}
	else if (options[i].var != VAR_WIN
		&& (options[i].flags & P_STRING))
	    // buffer-local option: free global value
	    clear_string_option((char_u **)options[i].var);
    }
}
#endif


/*
 * Initialize the options, part two: After getting Rows and Columns and
 * setting 'term'.
 */
    void
set_init_2(void)
{
    int		idx;

    /*
     * 'scroll' defaults to half the window height. The stored default is zero,
     * which results in the actual value computed from the window height.
     */
    idx = findoption((char_u *)"scroll");
    if (idx >= 0 && !(options[idx].flags & P_WAS_SET))
	set_option_default(idx, OPT_LOCAL, p_cp);
    comp_col();

    /*
     * 'window' is only for backwards compatibility with Vi.
     * Default is Rows - 1.
     */
    if (!option_was_set((char_u *)"window"))
	p_window = Rows - 1;
    set_number_default("window", Rows - 1);

    // For DOS console the default is always black.
#if !((defined(MSWIN)) && !defined(FEAT_GUI))
    /*
     * If 'background' wasn't set by the user, try guessing the value,
     * depending on the terminal name.  Only need to check for terminals
     * with a dark background, that can handle color.
     */
    idx = findoption((char_u *)"bg");
    if (idx >= 0 && !(options[idx].flags & P_WAS_SET)
						 && *term_bg_default() == 'd')
    {
	set_string_option_direct(NULL, idx, (char_u *)"dark", OPT_FREE, 0);
	// don't mark it as set, when starting the GUI it may be
	// changed again
	options[idx].flags &= ~P_WAS_SET;
    }
#endif

#ifdef CURSOR_SHAPE
    parse_shape_opt(SHAPE_CURSOR); // set cursor shapes from 'guicursor'
#endif
#ifdef FEAT_MOUSESHAPE
    parse_shape_opt(SHAPE_MOUSE);  // set mouse shapes from 'mouseshape'
#endif
#ifdef FEAT_PRINTER
    (void)parse_printoptions();	    // parse 'printoptions' default value
#endif
}

/*
 * Initialize the options, part three: After reading the .vimrc
 */
    void
set_init_3(void)
{
#if defined(UNIX) || defined(MSWIN)
/*
 * Set 'shellpipe' and 'shellredir', depending on the 'shell' option.
 * This is done after other initializations, where 'shell' might have been
 * set, but only if they have not been set before.
 */
    char_u  *p;
    int	    idx_srr;
    int	    do_srr;
# ifdef FEAT_QUICKFIX
    int	    idx_sp;
    int	    do_sp;
# endif

    idx_srr = findoption((char_u *)"srr");
    if (idx_srr < 0)
	do_srr = FALSE;
    else
	do_srr = !(options[idx_srr].flags & P_WAS_SET);
# ifdef FEAT_QUICKFIX
    idx_sp = findoption((char_u *)"sp");
    if (idx_sp < 0)
	do_sp = FALSE;
    else
	do_sp = !(options[idx_sp].flags & P_WAS_SET);
# endif
    p = get_isolated_shell_name();
    if (p != NULL)
    {
	/*
	 * Default for p_sp is "| tee", for p_srr is ">".
	 * For known shells it is changed here to include stderr.
	 */
	if (	   fnamecmp(p, "csh") == 0
		|| fnamecmp(p, "tcsh") == 0
# if defined(MSWIN)	// also check with .exe extension
		|| fnamecmp(p, "csh.exe") == 0
		|| fnamecmp(p, "tcsh.exe") == 0
# endif
	   )
	{
# if defined(FEAT_QUICKFIX)
	    if (do_sp)
	    {
#  ifdef MSWIN
		p_sp = (char_u *)">&";
#  else
		p_sp = (char_u *)"|& tee";
#  endif
		options[idx_sp].def_val[VI_DEFAULT] = p_sp;
	    }
# endif
	    if (do_srr)
	    {
		p_srr = (char_u *)">&";
		options[idx_srr].def_val[VI_DEFAULT] = p_srr;
	    }
	}
# ifdef MSWIN
	// Windows PowerShell output is UTF-16 with BOM so re-encode to the
	// current codepage.
	else if (   fnamecmp(p, "powershell") == 0
		    || fnamecmp(p, "powershell.exe") == 0
		)
	{
# if defined(FEAT_QUICKFIX)
		if (do_sp)
		{
		    p_sp = (char_u *)"2>&1 | Out-File -Encoding default";
		    options[idx_sp].def_val[VI_DEFAULT] = p_sp;
		}
# endif
		if (do_srr)
		{
		    p_srr = (char_u *)"2>&1 | Out-File -Encoding default";
		    options[idx_srr].def_val[VI_DEFAULT] = p_srr;
		}
	}
#endif
	else
	    // Always use POSIX shell style redirection if we reach this
	    if (       fnamecmp(p, "sh") == 0
		    || fnamecmp(p, "ksh") == 0
		    || fnamecmp(p, "mksh") == 0
		    || fnamecmp(p, "pdksh") == 0
		    || fnamecmp(p, "zsh") == 0
		    || fnamecmp(p, "zsh-beta") == 0
		    || fnamecmp(p, "bash") == 0
		    || fnamecmp(p, "fish") == 0
		    || fnamecmp(p, "ash") == 0
		    || fnamecmp(p, "dash") == 0
		    || fnamecmp(p, "pwsh") == 0
# ifdef MSWIN
		    || fnamecmp(p, "cmd") == 0
		    || fnamecmp(p, "sh.exe") == 0
		    || fnamecmp(p, "ksh.exe") == 0
		    || fnamecmp(p, "mksh.exe") == 0
		    || fnamecmp(p, "pdksh.exe") == 0
		    || fnamecmp(p, "zsh.exe") == 0
		    || fnamecmp(p, "zsh-beta.exe") == 0
		    || fnamecmp(p, "bash.exe") == 0
		    || fnamecmp(p, "cmd.exe") == 0
		    || fnamecmp(p, "dash.exe") == 0
		    || fnamecmp(p, "pwsh.exe") == 0
# endif
		    )
	    {
# if defined(FEAT_QUICKFIX)
		if (do_sp)
		{
#  ifdef MSWIN
		    p_sp = (char_u *)">%s 2>&1";
#  else
		    if (fnamecmp(p, "pwsh") == 0)
			p_sp = (char_u *)">%s 2>&1";
		    else
			p_sp = (char_u *)"2>&1| tee";
#  endif
		    options[idx_sp].def_val[VI_DEFAULT] = p_sp;
		}
# endif
		if (do_srr)
		{
		    p_srr = (char_u *)">%s 2>&1";
		    options[idx_srr].def_val[VI_DEFAULT] = p_srr;
		}
	    }
	vim_free(p);
    }
#endif

#if defined(MSWIN)
    /*
     * Set 'shellcmdflag', 'shellxquote', and 'shellquote' depending on the
     * 'shell' option.
     * This is done after other initializations, where 'shell' might have been
     * set, but only if they have not been set before.
     * Default values depend on shell (cmd.exe is default shell):
     *
     *			    p_shcf	p_sxq
     * cmd.exe          -   "/c"	"("
     * powershell.exe   -   "-Command"	"\""
     * pwsh.exe		-   "-c"	"\""
     * "sh" like shells -   "-c"	"\""
     *
     * For Win32 p_sxq is set instead of p_shq to include shell redirection.
     */
    if (strstr((char *)gettail(p_sh), "powershell") != NULL)
    {
	int	idx_opt;

	idx_opt = findoption((char_u *)"shcf");
	if (idx_opt >= 0 && !(options[idx_opt].flags & P_WAS_SET))
	{
	    p_shcf = (char_u*)"-Command";
	    options[idx_opt].def_val[VI_DEFAULT] = p_shcf;
	}

	idx_opt = findoption((char_u *)"sxq");
	if (idx_opt >= 0 && !(options[idx_opt].flags & P_WAS_SET))
	{
	    p_sxq = (char_u*)"\"";
	    options[idx_opt].def_val[VI_DEFAULT] = p_sxq;
	}
    }
    else if (strstr((char *)gettail(p_sh), "sh") != NULL)
    {
	int	idx3;

	idx3 = findoption((char_u *)"shcf");
	if (idx3 >= 0 && !(options[idx3].flags & P_WAS_SET))
	{
	    p_shcf = (char_u *)"-c";
	    options[idx3].def_val[VI_DEFAULT] = p_shcf;
	}

	// Somehow Win32 requires the quotes around the redirection too
	idx3 = findoption((char_u *)"sxq");
	if (idx3 >= 0 && !(options[idx3].flags & P_WAS_SET))
	{
	    p_sxq = (char_u *)"\"";
	    options[idx3].def_val[VI_DEFAULT] = p_sxq;
	}
    }
    else if (strstr((char *)gettail(p_sh), "cmd.exe") != NULL)
    {
	int	idx3;

	/*
	 * cmd.exe on Windows will strip the first and last double quote given
	 * on the command line, e.g. most of the time things like:
	 *   cmd /c "my path/to/echo" "my args to echo"
	 * become:
	 *   my path/to/echo" "my args to echo
	 * when executed.
	 *
	 * To avoid this, set shellxquote to surround the command in
	 * parenthesis.  This appears to make most commands work, without
	 * breaking commands that worked previously, such as
	 * '"path with spaces/cmd" "a&b"'.
	 */
	idx3 = findoption((char_u *)"sxq");
	if (idx3 >= 0 && !(options[idx3].flags & P_WAS_SET))
	{
	    p_sxq = (char_u *)"(";
	    options[idx3].def_val[VI_DEFAULT] = p_sxq;
	}

	idx3 = findoption((char_u *)"shcf");
	if (idx3 >= 0 && !(options[idx3].flags & P_WAS_SET))
	{
	    p_shcf = (char_u *)"/c";
	    options[idx3].def_val[VI_DEFAULT] = p_shcf;
	}
    }
#endif

    if (BUFEMPTY())
    {
	int idx_ffs = findoption((char_u *)"ffs");

	// Apply the first entry of 'fileformats' to the initial buffer.
	if (idx_ffs >= 0 && (options[idx_ffs].flags & P_WAS_SET))
	    set_fileformat(default_fileformat(), OPT_LOCAL);
    }

#ifdef FEAT_TITLE
    set_title_defaults();
#endif
}

#if defined(FEAT_MULTI_LANG) || defined(PROTO)
/*
 * When 'helplang' is still at its default value, set it to "lang".
 * Only the first two characters of "lang" are used.
 */
    void
set_helplang_default(char_u *lang)
{
    int		idx;

    if (lang == NULL || STRLEN(lang) < 2)	// safety check
	return;
    idx = findoption((char_u *)"hlg");
    if (idx >= 0 && !(options[idx].flags & P_WAS_SET))
    {
	if (options[idx].flags & P_ALLOCED)
	    free_string_option(p_hlg);
	p_hlg = vim_strsave(lang);
	if (p_hlg == NULL)
	    p_hlg = empty_option;
	else
	{
	    // zh_CN becomes "cn", zh_TW becomes "tw"
	    if (STRNICMP(p_hlg, "zh_", 3) == 0 && STRLEN(p_hlg) >= 5)
	    {
		p_hlg[0] = TOLOWER_ASC(p_hlg[3]);
		p_hlg[1] = TOLOWER_ASC(p_hlg[4]);
	    }
	    // any C like setting, such as C.UTF-8, becomes "en"
	    else if (STRLEN(p_hlg) >= 1 && *p_hlg == 'C')
	    {
		p_hlg[0] = 'e';
		p_hlg[1] = 'n';
	    }
	    p_hlg[2] = NUL;
	}
	options[idx].flags |= P_ALLOCED;
    }
}
#endif

#ifdef FEAT_TITLE
/*
 * 'title' and 'icon' only default to true if they have not been set or reset
 * in .vimrc and we can read the old value.
 * When 'title' and 'icon' have been reset in .vimrc, we won't even check if
 * they can be reset.  This reduces startup time when using X on a remote
 * machine.
 */
    void
set_title_defaults(void)
{
    int	    idx1;
    long    val;

    /*
     * If GUI is (going to be) used, we can always set the window title and
     * icon name.  Saves a bit of time, because the X11 display server does
     * not need to be contacted.
     */
    idx1 = findoption((char_u *)"title");
    if (idx1 >= 0 && !(options[idx1].flags & P_WAS_SET))
    {
#ifdef FEAT_GUI
	if (gui.starting || gui.in_use)
	    val = TRUE;
	else
#endif
	    val = mch_can_restore_title();
	options[idx1].def_val[VI_DEFAULT] = (char_u *)(long_i)val;
	p_title = val;
    }
    idx1 = findoption((char_u *)"icon");
    if (idx1 >= 0 && !(options[idx1].flags & P_WAS_SET))
    {
#ifdef FEAT_GUI
	if (gui.starting || gui.in_use)
	    val = TRUE;
	else
#endif
	    val = mch_can_restore_icon();
	options[idx1].def_val[VI_DEFAULT] = (char_u *)(long_i)val;
	p_icon = val;
    }
}
#endif

    void
ex_set(exarg_T *eap)
{
    int		flags = 0;

    if (eap->cmdidx == CMD_setlocal)
	flags = OPT_LOCAL;
    else if (eap->cmdidx == CMD_setglobal)
	flags = OPT_GLOBAL;
#if defined(FEAT_EVAL) && defined(FEAT_BROWSE)
    if ((cmdmod.cmod_flags & CMOD_BROWSE) && flags == 0)
	ex_options(eap);
    else
#endif
    {
	if (eap->forceit)
	    flags |= OPT_ONECOLUMN;
	(void)do_set(eap->arg, flags);
    }
}

/*
 * Parse 'arg' for option settings.
 *
 * 'arg' may be IObuff, but only when no errors can be present and option
 * does not need to be expanded with option_expand().
 * "opt_flags":
 * 0 for ":set"
 * OPT_GLOBAL   for ":setglobal"
 * OPT_LOCAL    for ":setlocal" and a modeline
 * OPT_MODELINE for a modeline
 * OPT_WINONLY  to only set window-local options
 * OPT_NOWIN	to skip setting window-local options
 *
 * returns FAIL if an error is detected, OK otherwise
 */
    int
do_set(
    char_u	*arg_start,	// option string (may be written to!)
    int		opt_flags)
{
    char_u	*arg = arg_start;
    int		opt_idx;
    char	*errmsg;
    char	errbuf[80];
    char_u	*startarg;
    int		prefix;	// 1: nothing, 0: "no", 2: "inv" in front of name
    int		nextchar;	    // next non-white char after option name
    int		afterchar;	    // character just after option name
    int		len;
    int		i;
    varnumber_T	value;
    int		key;
    long_u	flags;		    // flags for current option
    char_u	*varp = NULL;	    // pointer to variable for current option
    int		did_show = FALSE;   // already showed one value
    int		adding;		    // "opt+=arg"
    int		prepending;	    // "opt^=arg"
    int		removing;	    // "opt-=arg"
    int		cp_val = 0;
    char_u	key_name[2];

    if (*arg == NUL)
    {
	showoptions(0, opt_flags);
	did_show = TRUE;
	goto theend;
    }

    while (*arg != NUL)		// loop to process all options
    {
	errmsg = NULL;
	startarg = arg;		// remember for error message

	if (STRNCMP(arg, "all", 3) == 0 && !isalpha(arg[3])
						&& !(opt_flags & OPT_MODELINE))
	{
	    /*
	     * ":set all"  show all options.
	     * ":set all&" set all options to their default value.
	     */
	    arg += 3;
	    if (*arg == '&')
	    {
		++arg;
		// Only for :set command set global value of local options.
		set_options_default(OPT_FREE | opt_flags);
		didset_options();
		didset_options2();
		redraw_all_later(CLEAR);
	    }
	    else
	    {
		showoptions(1, opt_flags);
		did_show = TRUE;
	    }
	}
	else if (STRNCMP(arg, "termcap", 7) == 0 && !(opt_flags & OPT_MODELINE))
	{
	    showoptions(2, opt_flags);
	    show_termcodes();
	    did_show = TRUE;
	    arg += 7;
	}
	else
	{
	    prefix = 1;
	    if (STRNCMP(arg, "no", 2) == 0 && STRNCMP(arg, "novice", 6) != 0)
	    {
		prefix = 0;
		arg += 2;
	    }
	    else if (STRNCMP(arg, "inv", 3) == 0)
	    {
		prefix = 2;
		arg += 3;
	    }

	    // find end of name
	    key = 0;
	    if (*arg == '<')
	    {
		opt_idx = -1;
		// look out for <t_>;>
		if (arg[1] == 't' && arg[2] == '_' && arg[3] && arg[4])
		    len = 5;
		else
		{
		    len = 1;
		    while (arg[len] != NUL && arg[len] != '>')
			++len;
		}
		if (arg[len] != '>')
		{
		    errmsg = e_invarg;
		    goto skip;
		}
		arg[len] = NUL;			    // put NUL after name
		if (arg[1] == 't' && arg[2] == '_') // could be term code
		    opt_idx = findoption(arg + 1);
		arg[len++] = '>';		    // restore '>'
		if (opt_idx == -1)
		    key = find_key_option(arg + 1, TRUE);
	    }
	    else
	    {
		len = 0;
		/*
		 * The two characters after "t_" may not be alphanumeric.
		 */
		if (arg[0] == 't' && arg[1] == '_' && arg[2] && arg[3])
		    len = 4;
		else
		    while (ASCII_ISALNUM(arg[len]) || arg[len] == '_')
			++len;
		nextchar = arg[len];
		arg[len] = NUL;			    // put NUL after name
		opt_idx = findoption(arg);
		arg[len] = nextchar;		    // restore nextchar
		if (opt_idx == -1)
		    key = find_key_option(arg, FALSE);
	    }

	    // remember character after option name
	    afterchar = arg[len];

	    if (in_vim9script())
	    {
		char_u *p = skipwhite(arg + len);

		// disallow white space before =val, +=val, -=val, ^=val
		if (p > arg + len && (p[0] == '='
			|| (vim_strchr((char_u *)"+-^", p[0]) != NULL
							      && p[1] == '=')))
		{
		    errmsg = e_no_white_space_allowed_between_option_and;
		    arg = p;
		    startarg = p;
		    goto skip;
		}
	    }
	    else
		// skip white space, allow ":set ai  ?", ":set hlsearch  !"
		while (VIM_ISWHITE(arg[len]))
		    ++len;

	    adding = FALSE;
	    prepending = FALSE;
	    removing = FALSE;
	    if (arg[len] != NUL && arg[len + 1] == '=')
	    {
		if (arg[len] == '+')
		{
		    adding = TRUE;		// "+="
		    ++len;
		}
		else if (arg[len] == '^')
		{
		    prepending = TRUE;		// "^="
		    ++len;
		}
		else if (arg[len] == '-')
		{
		    removing = TRUE;		// "-="
		    ++len;
		}
	    }
	    nextchar = arg[len];

	    if (opt_idx == -1 && key == 0)	// found a mismatch: skip
	    {
		if (in_vim9script() && arg > arg_start
				  && vim_strchr((char_u *)"!&<", *arg) != NULL)
		    errmsg = e_no_white_space_allowed_between_option_and;
		else
		    errmsg = N_("E518: Unknown option");
		goto skip;
	    }

	    if (opt_idx >= 0)
	    {
		if (options[opt_idx].var == NULL)   // hidden option: skip
		{
		    // Only give an error message when requesting the value of
		    // a hidden option, ignore setting it.
		    if (vim_strchr((char_u *)"=:!&<", nextchar) == NULL
			    && (!(options[opt_idx].flags & P_BOOL)
				|| nextchar == '?'))
			errmsg = N_("E519: Option not supported");
		    goto skip;
		}

		flags = options[opt_idx].flags;
		varp = get_varp_scope(&(options[opt_idx]), opt_flags);
	    }
	    else
	    {
		flags = P_STRING;
		if (key < 0)
		{
		    key_name[0] = KEY2TERMCAP0(key);
		    key_name[1] = KEY2TERMCAP1(key);
		}
		else
		{
		    key_name[0] = KS_KEY;
		    key_name[1] = (key & 0xff);
		}
	    }

	    // Skip all options that are not window-local (used when showing
	    // an already loaded buffer in a window).
	    if ((opt_flags & OPT_WINONLY)
			  && (opt_idx < 0 || options[opt_idx].var != VAR_WIN))
		goto skip;

	    // Skip all options that are window-local (used for :vimgrep).
	    if ((opt_flags & OPT_NOWIN) && opt_idx >= 0
					   && options[opt_idx].var == VAR_WIN)
		goto skip;

	    // Disallow changing some options from modelines.
	    if (opt_flags & OPT_MODELINE)
	    {
		if (flags & (P_SECURE | P_NO_ML))
		{
		    errmsg = N_("E520: Not allowed in a modeline");
		    goto skip;
		}
		if ((flags & P_MLE) && !p_mle)
		{
		    errmsg = N_("E992: Not allowed in a modeline when 'modelineexpr' is off");
		    goto skip;
		}
#ifdef FEAT_DIFF
		// In diff mode some options are overruled.  This avoids that
		// 'foldmethod' becomes "marker" instead of "diff" and that
		// "wrap" gets set.
		if (curwin->w_p_diff
			&& opt_idx >= 0  // shut up coverity warning
			&& (
#ifdef FEAT_FOLDING
			    options[opt_idx].indir == PV_FDM ||
#endif
			    options[opt_idx].indir == PV_WRAP))
		    goto skip;
#endif
	    }

#ifdef HAVE_SANDBOX
	    // Disallow changing some options in the sandbox
	    if (sandbox != 0 && (flags & P_SECURE))
	    {
		errmsg = e_not_allowed_in_sandbox;
		goto skip;
	    }
#endif

	    if (vim_strchr((char_u *)"?=:!&<", nextchar) != NULL)
	    {
		arg += len;
		cp_val = p_cp;
		if (nextchar == '&' && arg[1] == 'v' && arg[2] == 'i')
		{
		    if (arg[3] == 'm')	// "opt&vim": set to Vim default
		    {
			cp_val = FALSE;
			arg += 3;
		    }
		    else		// "opt&vi": set to Vi default
		    {
			cp_val = TRUE;
			arg += 2;
		    }
		}
		if (vim_strchr((char_u *)"?!&<", nextchar) != NULL
			&& arg[1] != NUL && !VIM_ISWHITE(arg[1]))
		{
		    errmsg = e_trailing;
		    goto skip;
		}
	    }

	    /*
	     * allow '=' and ':' for historical reasons (MSDOS command.com
	     * allows only one '=' character per "set" command line. grrr. (jw)
	     */
	    if (nextchar == '?'
		    || (prefix == 1
			&& vim_strchr((char_u *)"=:&<", nextchar) == NULL
			&& !(flags & P_BOOL)))
	    {
		/*
		 * print value
		 */
		if (did_show)
		    msg_putchar('\n');	    // cursor below last one
		else
		{
		    gotocmdline(TRUE);	    // cursor at status line
		    did_show = TRUE;	    // remember that we did a line
		}
		if (opt_idx >= 0)
		{
		    showoneopt(&options[opt_idx], opt_flags);
#ifdef FEAT_EVAL
		    if (p_verbose > 0)
		    {
			// Mention where the option was last set.
			if (varp == options[opt_idx].var)
			    last_set_msg(options[opt_idx].script_ctx);
			else if ((int)options[opt_idx].indir & PV_WIN)
			    last_set_msg(curwin->w_p_script_ctx[
				      (int)options[opt_idx].indir & PV_MASK]);
			else if ((int)options[opt_idx].indir & PV_BUF)
			    last_set_msg(curbuf->b_p_script_ctx[
				      (int)options[opt_idx].indir & PV_MASK]);
		    }
#endif
		}
		else
		{
		    char_u	    *p;

		    p = find_termcode(key_name);
		    if (p == NULL)
		    {
			errmsg = N_("E846: Key code not set");
			goto skip;
		    }
		    else
			(void)show_one_termcode(key_name, p, TRUE);
		}
		if (nextchar != '?'
			&& nextchar != NUL && !VIM_ISWHITE(afterchar))
		    errmsg = e_trailing;
	    }
	    else
	    {
		int value_is_replaced = !prepending && !adding && !removing;
		int value_checked = FALSE;

		if (flags & P_BOOL)		    // boolean
		{
		    if (nextchar == '=' || nextchar == ':')
		    {
			errmsg = e_invarg;
			goto skip;
		    }

		    /*
		     * ":set opt!": invert
		     * ":set opt&": reset to default value
		     * ":set opt<": reset to global value
		     */
		    if (nextchar == '!')
			value = *(int *)(varp) ^ 1;
		    else if (nextchar == '&')
			value = (int)(long)(long_i)options[opt_idx].def_val[
						((flags & P_VI_DEF) || cp_val)
						 ?  VI_DEFAULT : VIM_DEFAULT];
		    else if (nextchar == '<')
		    {
			// For 'autoread' -1 means to use global value.
			if ((int *)varp == &curbuf->b_p_ar
						    && opt_flags == OPT_LOCAL)
			    value = -1;
			else
			    value = *(int *)get_varp_scope(&(options[opt_idx]),
								  OPT_GLOBAL);
		    }
		    else
		    {
			/*
			 * ":set invopt": invert
			 * ":set opt" or ":set noopt": set or reset
			 */
			if (nextchar != NUL && !VIM_ISWHITE(afterchar))
			{
			    errmsg = e_trailing;
			    goto skip;
			}
			if (prefix == 2)	// inv
			    value = *(int *)(varp) ^ 1;
			else
			    value = prefix;
		    }

		    errmsg = set_bool_option(opt_idx, varp, (int)value,
								   opt_flags);
		}
		else				    // numeric or string
		{
		    if (vim_strchr((char_u *)"=:&<", nextchar) == NULL
							       || prefix != 1)
		    {
			errmsg = e_invarg;
			goto skip;
		    }

		    if (flags & P_NUM)		    // numeric
		    {
			/*
			 * Different ways to set a number option:
			 * &	    set to default value
			 * <	    set to global value
			 * <xx>	    accept special key codes for 'wildchar'
			 * c	    accept any non-digit for 'wildchar'
			 * [-]0-9   set number
			 * other    error
			 */
			++arg;
			if (nextchar == '&')
			    value = (long)(long_i)options[opt_idx].def_val[
						((flags & P_VI_DEF) || cp_val)
						 ?  VI_DEFAULT : VIM_DEFAULT];
			else if (nextchar == '<')
			{
			    // For 'undolevels' NO_LOCAL_UNDOLEVEL means to
			    // use the global value.
			    if ((long *)varp == &curbuf->b_p_ul
						    && opt_flags == OPT_LOCAL)
				value = NO_LOCAL_UNDOLEVEL;
			    else
				value = *(long *)get_varp_scope(
					     &(options[opt_idx]), OPT_GLOBAL);
			}
			else if (((long *)varp == &p_wc
				    || (long *)varp == &p_wcm)
				&& (*arg == '<'
				    || *arg == '^'
				    || (*arg != NUL
					&& (!arg[1] || VIM_ISWHITE(arg[1]))
					&& !VIM_ISDIGIT(*arg))))
			{
			    value = string_to_key(arg, FALSE);
			    if (value == 0 && (long *)varp != &p_wcm)
			    {
				errmsg = e_invarg;
				goto skip;
			    }
			}
			else if (*arg == '-' || VIM_ISDIGIT(*arg))
			{
			    // Allow negative (for 'undolevels'), octal and
			    // hex numbers.
			    vim_str2nr(arg, NULL, &i, STR2NR_ALL,
						     &value, NULL, 0, TRUE);
			    if (i == 0 || (arg[i] != NUL
						      && !VIM_ISWHITE(arg[i])))
			    {
				errmsg = N_("E521: Number required after =");
				goto skip;
			    }
			}
			else
			{
			    errmsg = N_("E521: Number required after =");
			    goto skip;
			}

			if (adding)
			    value = *(long *)varp + value;
			if (prepending)
			    value = *(long *)varp * value;
			if (removing)
			    value = *(long *)varp - value;
			errmsg = set_num_option(opt_idx, varp, value,
					   errbuf, sizeof(errbuf), opt_flags);
		    }
		    else if (opt_idx >= 0)		    // string
		    {
			char_u	  *save_arg = NULL;
			char_u	  *s = NULL;
			char_u	  *oldval = NULL; // previous value if *varp
			char_u	  *newval;
			char_u	  *origval = NULL;
			char_u	  *origval_l = NULL;
			char_u	  *origval_g = NULL;
#if defined(FEAT_EVAL)
			char_u	  *saved_origval = NULL;
			char_u	  *saved_origval_l = NULL;
			char_u	  *saved_origval_g = NULL;
			char_u	  *saved_newval = NULL;
#endif
			unsigned  newlen;
			int	  comma;
			int	  new_value_alloced;	// new string option
							// was allocated

			// When using ":set opt=val" for a global option
			// with a local value the local value will be
			// reset, use the global value here.
			if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0
				&& ((int)options[opt_idx].indir & PV_BOTH))
			    varp = options[opt_idx].var;

			// The old value is kept until we are sure that the
			// new value is valid.
			oldval = *(char_u **)varp;

			if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
			{
			    origval_l = *(char_u **)get_varp_scope(
					       &(options[opt_idx]), OPT_LOCAL);
			    origval_g = *(char_u **)get_varp_scope(
					      &(options[opt_idx]), OPT_GLOBAL);

			    // A global-local string option might have an empty
			    // option as value to indicate that the global
			    // value should be used.
			    if (((int)options[opt_idx].indir & PV_BOTH)
						  && origval_l == empty_option)
				origval_l = origval_g;
			}

			// When setting the local value of a global
			// option, the old value may be the global value.
			if (((int)options[opt_idx].indir & PV_BOTH)
					       && (opt_flags & OPT_LOCAL))
			    origval = *(char_u **)get_varp(
						       &options[opt_idx]);
			else
			    origval = oldval;

			if (nextchar == '&')	// set to default val
			{
			    newval = options[opt_idx].def_val[
						((flags & P_VI_DEF) || cp_val)
						 ?  VI_DEFAULT : VIM_DEFAULT];
			    if ((char_u **)varp == &p_bg)
			    {
				// guess the value of 'background'
#ifdef FEAT_GUI
				if (gui.in_use)
				    newval = gui_bg_default();
				else
#endif
				    newval = term_bg_default();
			    }
			    else if ((char_u **)varp == &p_fencs && enc_utf8)
				newval = fencs_utf8_default;

			    // expand environment variables and ~ (since the
			    // default value was already expanded, only
			    // required when an environment variable was set
			    // later
			    if (newval == NULL)
				newval = empty_option;
			    else
			    {
				s = option_expand(opt_idx, newval);
				if (s == NULL)
				    s = newval;
				newval = vim_strsave(s);
			    }
			    new_value_alloced = TRUE;
			}
			else if (nextchar == '<')	// set to global val
			{
			    newval = vim_strsave(*(char_u **)get_varp_scope(
					     &(options[opt_idx]), OPT_GLOBAL));
			    new_value_alloced = TRUE;
			}
			else
			{
			    ++arg;	// jump to after the '=' or ':'

			    /*
			     * Set 'keywordprg' to ":help" if an empty
			     * value was passed to :set by the user.
			     * Misuse errbuf[] for the resulting string.
			     */
			    if (varp == (char_u *)&p_kp
					      && (*arg == NUL || *arg == ' '))
			    {
				STRCPY(errbuf, ":help");
				save_arg = arg;
				arg = (char_u *)errbuf;
			    }
			    /*
			     * Convert 'backspace' number to string, for
			     * adding, prepending and removing string.
			     */
			    else if (varp == (char_u *)&p_bs
					 && VIM_ISDIGIT(**(char_u **)varp))
			    {
				i = getdigits((char_u **)varp);
				switch (i)
				{
				    case 0:
					*(char_u **)varp = empty_option;
					break;
				    case 1:
					*(char_u **)varp = vim_strsave(
						      (char_u *)"indent,eol");
					break;
				    case 2:
					*(char_u **)varp = vim_strsave(
						(char_u *)"indent,eol,start");
					break;
				    case 3:
					*(char_u **)varp = vim_strsave(
						(char_u *)"indent,eol,nostop");
					break;
				}
				vim_free(oldval);
				if (origval == oldval)
				    origval = *(char_u **)varp;
				if (origval_l == oldval)
				    origval_l = *(char_u **)varp;
				if (origval_g == oldval)
				    origval_g = *(char_u **)varp;
				oldval = *(char_u **)varp;
			    }
			    /*
			     * Convert 'whichwrap' number to string, for
			     * backwards compatibility with Vim 3.0.
			     * Misuse errbuf[] for the resulting string.
			     */
			    else if (varp == (char_u *)&p_ww
							 && VIM_ISDIGIT(*arg))
			    {
				*errbuf = NUL;
				i = getdigits(&arg);
				if (i & 1)
				    STRCAT(errbuf, "b,");
				if (i & 2)
				    STRCAT(errbuf, "s,");
				if (i & 4)
				    STRCAT(errbuf, "h,l,");
				if (i & 8)
				    STRCAT(errbuf, "<,>,");
				if (i & 16)
				    STRCAT(errbuf, "[,],");
				if (*errbuf != NUL)	// remove trailing ,
				    errbuf[STRLEN(errbuf) - 1] = NUL;
				save_arg = arg;
				arg = (char_u *)errbuf;
			    }
			    /*
			     * Remove '>' before 'dir' and 'bdir', for
			     * backwards compatibility with version 3.0
			     */
			    else if (  *arg == '>'
				    && (varp == (char_u *)&p_dir
					    || varp == (char_u *)&p_bdir))
			    {
				++arg;
			    }

			    /*
			     * Copy the new string into allocated memory.
			     * Can't use set_string_option_direct(), because
			     * we need to remove the backslashes.
			     */
			    // get a bit too much
			    newlen = (unsigned)STRLEN(arg) + 1;
			    if (adding || prepending || removing)
				newlen += (unsigned)STRLEN(origval) + 1;
			    newval = alloc(newlen);
			    if (newval == NULL)  // out of mem, don't change
				break;
			    s = newval;

			    /*
			     * Copy the string, skip over escaped chars.
			     * For MS-DOS and WIN32 backslashes before normal
			     * file name characters are not removed, and keep
			     * backslash at start, for "\\machine\path", but
			     * do remove it for "\\\\machine\\path".
			     * The reverse is found in ExpandOldSetting().
			     */
			    while (*arg && !VIM_ISWHITE(*arg))
			    {
				if (*arg == '\\' && arg[1] != NUL
#ifdef BACKSLASH_IN_FILENAME
					&& !((flags & P_EXPAND)
						&& vim_isfilec(arg[1])
						&& !VIM_ISWHITE(arg[1])
						&& (arg[1] != '\\'
						    || (s == newval
							&& arg[2] != '\\')))
#endif
								    )
				    ++arg;	// remove backslash
				if (has_mbyte
					&& (i = (*mb_ptr2len)(arg)) > 1)
				{
				    // copy multibyte char
				    mch_memmove(s, arg, (size_t)i);
				    arg += i;
				    s += i;
				}
				else
				    *s++ = *arg++;
			    }
			    *s = NUL;

			    /*
			     * Expand environment variables and ~.
			     * Don't do it when adding without inserting a
			     * comma.
			     */
			    if (!(adding || prepending || removing)
							 || (flags & P_COMMA))
			    {
				s = option_expand(opt_idx, newval);
				if (s != NULL)
				{
				    vim_free(newval);
				    newlen = (unsigned)STRLEN(s) + 1;
				    if (adding || prepending || removing)
					newlen += (unsigned)STRLEN(origval) + 1;
				    newval = alloc(newlen);
				    if (newval == NULL)
					break;
				    STRCPY(newval, s);
				}
			    }

			    // locate newval[] in origval[] when removing it
			    // and when adding to avoid duplicates
			    i = 0;	// init for GCC
			    if (removing || (flags & P_NODUP))
			    {
				i = (int)STRLEN(newval);
				s = find_dup_item(origval, newval, flags);

				// do not add if already there
				if ((adding || prepending) && s != NULL)
				{
				    prepending = FALSE;
				    adding = FALSE;
				    STRCPY(newval, origval);
				}

				// if no duplicate, move pointer to end of
				// original value
				if (s == NULL)
				    s = origval + (int)STRLEN(origval);
			    }

			    // concatenate the two strings; add a ',' if
			    // needed
			    if (adding || prepending)
			    {
				comma = ((flags & P_COMMA) && *origval != NUL
							   && *newval != NUL);
				if (adding)
				{
				    i = (int)STRLEN(origval);
				    // strip a trailing comma, would get 2
				    if (comma && i > 1
					  && (flags & P_ONECOMMA) == P_ONECOMMA
					  && origval[i - 1] == ','
					  && origval[i - 2] != '\\')
					i--;
				    mch_memmove(newval + i + comma, newval,
							  STRLEN(newval) + 1);
				    mch_memmove(newval, origval, (size_t)i);
				}
				else
				{
				    i = (int)STRLEN(newval);
				    STRMOVE(newval + i + comma, origval);
				}
				if (comma)
				    newval[i] = ',';
			    }

			    // Remove newval[] from origval[]. (Note: "i" has
			    // been set above and is used here).
			    if (removing)
			    {
				STRCPY(newval, origval);
				if (*s)
				{
				    // may need to remove a comma
				    if (flags & P_COMMA)
				    {
					if (s == origval)
					{
					    // include comma after string
					    if (s[i] == ',')
						++i;
					}
					else
					{
					    // include comma before string
					    --s;
					    ++i;
					}
				    }
				    STRMOVE(newval + (s - origval), s + i);
				}
			    }

			    if (flags & P_FLAGLIST)
			    {
				// Remove flags that appear twice.
				for (s = newval; *s;)
				{
				    // if options have P_FLAGLIST and
				    // P_ONECOMMA such as 'whichwrap'
				    if (flags & P_ONECOMMA)
				    {
					if (*s != ',' && *(s + 1) == ','
					      && vim_strchr(s + 2, *s) != NULL)
					{
					    // Remove the duplicated value and
					    // the next comma.
					    STRMOVE(s, s + 2);
					    continue;
					}
				    }
				    else
				    {
					if ((!(flags & P_COMMA) || *s != ',')
					      && vim_strchr(s + 1, *s) != NULL)
					{
					    STRMOVE(s, s + 1);
					    continue;
					}
				    }
				    ++s;
				}
			    }

			    if (save_arg != NULL)   // number for 'whichwrap'
				arg = save_arg;
			    new_value_alloced = TRUE;
			}

			/*
			 * Set the new value.
			 */
			*(char_u **)(varp) = newval;

#if defined(FEAT_EVAL)
			if (!starting
# ifdef FEAT_CRYPT
				&& options[opt_idx].indir != PV_KEY
# endif
					  && origval != NULL && newval != NULL)
			{
			    // origval may be freed by
			    // did_set_string_option(), make a copy.
			    saved_origval = vim_strsave(origval);
			    // newval (and varp) may become invalid if the
			    // buffer is closed by autocommands.
			    saved_newval = vim_strsave(newval);
			    if (origval_l != NULL)
				saved_origval_l = vim_strsave(origval_l);
			    if (origval_g != NULL)
				saved_origval_g = vim_strsave(origval_g);
			}
#endif

			{
			    long_u *p = insecure_flag(opt_idx, opt_flags);
			    int	    secure_saved = secure;

			    // When an option is set in the sandbox, from a
			    // modeline or in secure mode, then deal with side
			    // effects in secure mode.  Also when the value was
			    // set with the P_INSECURE flag and is not
			    // completely replaced.
			    if ((opt_flags & OPT_MODELINE)
#ifdef HAVE_SANDBOX
				  || sandbox != 0
#endif
				  || (!value_is_replaced && (*p & P_INSECURE)))
				secure = 1;

			    // Handle side effects, and set the global value
			    // for ":set" on local options. Note: when setting
			    // 'syntax' or 'filetype' autocommands may be
			    // triggered that can cause havoc.
			    errmsg = did_set_string_option(
				    opt_idx, (char_u **)varp,
				    new_value_alloced, oldval, errbuf,
				    opt_flags, &value_checked);

			    secure = secure_saved;
			}

#if defined(FEAT_EVAL)
			if (errmsg == NULL)
			    trigger_optionsset_string(
				    opt_idx, opt_flags, saved_origval,
				    saved_origval_l, saved_origval_g,
				    saved_newval);
			vim_free(saved_origval);
			vim_free(saved_origval_l);
			vim_free(saved_origval_g);
			vim_free(saved_newval);
#endif
			// If error detected, print the error message.
			if (errmsg != NULL)
			    goto skip;
		    }
		    else	    // key code option
		    {
			char_u	    *p;

			if (nextchar == '&')
			{
			    if (add_termcap_entry(key_name, TRUE) == FAIL)
				errmsg = N_("E522: Not found in termcap");
			}
			else
			{
			    ++arg; // jump to after the '=' or ':'
			    for (p = arg; *p && !VIM_ISWHITE(*p); ++p)
				if (*p == '\\' && p[1] != NUL)
				    ++p;
			    nextchar = *p;
			    *p = NUL;
			    add_termcode(key_name, arg, FALSE);
			    *p = nextchar;
			}
			if (full_screen)
			    ttest(FALSE);
			redraw_all_later(CLEAR);
		    }
		}

		if (opt_idx >= 0)
		    did_set_option(
			 opt_idx, opt_flags, value_is_replaced, value_checked);
	    }

skip:
	    /*
	     * Advance to next argument.
	     * - skip until a blank found, taking care of backslashes
	     * - skip blanks
	     * - skip one "=val" argument (for hidden options ":set gfn =xx")
	     */
	    for (i = 0; i < 2 ; ++i)
	    {
		while (*arg != NUL && !VIM_ISWHITE(*arg))
		    if (*arg++ == '\\' && *arg != NUL)
			++arg;
		arg = skipwhite(arg);
		if (*arg != '=')
		    break;
	    }
	}

	if (errmsg != NULL)
	{
	    vim_strncpy(IObuff, (char_u *)_(errmsg), IOSIZE - 1);
	    i = (int)STRLEN(IObuff) + 2;
	    if (i + (arg - startarg) < IOSIZE)
	    {
		// append the argument with the error
		STRCAT(IObuff, ": ");
		mch_memmove(IObuff + i, startarg, (arg - startarg));
		IObuff[i + (arg - startarg)] = NUL;
	    }
	    // make sure all characters are printable
	    trans_characters(IObuff, IOSIZE);

	    ++no_wait_return;		// wait_return done later
	    emsg((char *)IObuff);	// show error highlighted
	    --no_wait_return;

	    return FAIL;
	}

	arg = skipwhite(arg);
    }

theend:
    if (silent_mode && did_show)
    {
	// After displaying option values in silent mode.
	silent_mode = FALSE;
	info_message = TRUE;	// use mch_msg(), not mch_errmsg()
	msg_putchar('\n');
	cursor_on();		// msg_start() switches it off
	out_flush();
	silent_mode = TRUE;
	info_message = FALSE;	// use mch_msg(), not mch_errmsg()
    }

    return OK;
}

/*
 * Call this when an option has been given a new value through a user command.
 * Sets the P_WAS_SET flag and takes care of the P_INSECURE flag.
 */
    void
did_set_option(
    int	    opt_idx,
    int	    opt_flags,	    // possibly with OPT_MODELINE
    int	    new_value,	    // value was replaced completely
    int	    value_checked)  // value was checked to be safe, no need to set the
			    // P_INSECURE flag.
{
    long_u	*p;

    options[opt_idx].flags |= P_WAS_SET;

    // When an option is set in the sandbox, from a modeline or in secure mode
    // set the P_INSECURE flag.  Otherwise, if a new value is stored reset the
    // flag.
    p = insecure_flag(opt_idx, opt_flags);
    if (!value_checked && (secure
#ifdef HAVE_SANDBOX
	    || sandbox != 0
#endif
	    || (opt_flags & OPT_MODELINE)))
	*p = *p | P_INSECURE;
    else if (new_value)
	*p = *p & ~P_INSECURE;
}

/*
 * Convert a key name or string into a key value.
 * Used for 'wildchar' and 'cedit' options.
 * When "multi_byte" is TRUE allow for multi-byte characters.
 */
    int
string_to_key(char_u *arg, int multi_byte)
{
    if (*arg == '<')
	return find_key_option(arg + 1, TRUE);
    if (*arg == '^')
	return Ctrl_chr(arg[1]);
    if (multi_byte)
	return PTR2CHAR(arg);
    return *arg;
}

#ifdef FEAT_TITLE
/*
 * When changing 'title', 'titlestring', 'icon' or 'iconstring', call
 * maketitle() to create and display it.
 * When switching the title or icon off, call mch_restore_title() to get
 * the old value back.
 */
    void
did_set_title(void)
{
    if (starting != NO_SCREEN
#ifdef FEAT_GUI
	    && !gui.starting
#endif
				)
	maketitle();
}
#endif

/*
 * set_options_bin -  called when 'bin' changes value.
 */
    void
set_options_bin(
    int		oldval,
    int		newval,
    int		opt_flags)	// OPT_LOCAL and/or OPT_GLOBAL
{
    /*
     * The option values that are changed when 'bin' changes are
     * copied when 'bin is set and restored when 'bin' is reset.
     */
    if (newval)
    {
	if (!oldval)		// switched on
	{
	    if (!(opt_flags & OPT_GLOBAL))
	    {
		curbuf->b_p_tw_nobin = curbuf->b_p_tw;
		curbuf->b_p_wm_nobin = curbuf->b_p_wm;
		curbuf->b_p_ml_nobin = curbuf->b_p_ml;
		curbuf->b_p_et_nobin = curbuf->b_p_et;
	    }
	    if (!(opt_flags & OPT_LOCAL))
	    {
		p_tw_nobin = p_tw;
		p_wm_nobin = p_wm;
		p_ml_nobin = p_ml;
		p_et_nobin = p_et;
	    }
	}

	if (!(opt_flags & OPT_GLOBAL))
	{
	    curbuf->b_p_tw = 0;	// no automatic line wrap
	    curbuf->b_p_wm = 0;	// no automatic line wrap
	    curbuf->b_p_ml = 0;	// no modelines
	    curbuf->b_p_et = 0;	// no expandtab
	}
	if (!(opt_flags & OPT_LOCAL))
	{
	    p_tw = 0;
	    p_wm = 0;
	    p_ml = FALSE;
	    p_et = FALSE;
	    p_bin = TRUE;	// needed when called for the "-b" argument
	}
    }
    else if (oldval)		// switched off
    {
	if (!(opt_flags & OPT_GLOBAL))
	{
	    curbuf->b_p_tw = curbuf->b_p_tw_nobin;
	    curbuf->b_p_wm = curbuf->b_p_wm_nobin;
	    curbuf->b_p_ml = curbuf->b_p_ml_nobin;
	    curbuf->b_p_et = curbuf->b_p_et_nobin;
	}
	if (!(opt_flags & OPT_LOCAL))
	{
	    p_tw = p_tw_nobin;
	    p_wm = p_wm_nobin;
	    p_ml = p_ml_nobin;
	    p_et = p_et_nobin;
	}
    }
}

/*
 * Expand environment variables for some string options.
 * These string options cannot be indirect!
 * If "val" is NULL expand the current value of the option.
 * Return pointer to NameBuff, or NULL when not expanded.
 */
    static char_u *
option_expand(int opt_idx, char_u *val)
{
    // if option doesn't need expansion nothing to do
    if (!(options[opt_idx].flags & P_EXPAND) || options[opt_idx].var == NULL)
	return NULL;

    // If val is longer than MAXPATHL no meaningful expansion can be done,
    // expand_env() would truncate the string.
    if (val != NULL && STRLEN(val) > MAXPATHL)
	return NULL;

    if (val == NULL)
	val = *(char_u **)options[opt_idx].var;

    /*
     * Expanding this with NameBuff, expand_env() must not be passed IObuff.
     * Escape spaces when expanding 'tags', they are used to separate file
     * names.
     * For 'spellsuggest' expand after "file:".
     */
    expand_env_esc(val, NameBuff, MAXPATHL,
	    (char_u **)options[opt_idx].var == &p_tags, FALSE,
#ifdef FEAT_SPELL
	    (char_u **)options[opt_idx].var == &p_sps ? (char_u *)"file:" :
#endif
				  NULL);
    if (STRCMP(NameBuff, val) == 0)   // they are the same
	return NULL;

    return NameBuff;
}

/*
 * After setting various option values: recompute variables that depend on
 * option values.
 */
    static void
didset_options(void)
{
    // initialize the table for 'iskeyword' et.al.
    (void)init_chartab();

    didset_string_options();

#ifdef FEAT_SPELL
    (void)spell_check_msm();
    (void)spell_check_sps();
    (void)compile_cap_prog(curwin->w_s);
    (void)did_set_spell_option(TRUE);
#endif
#ifdef FEAT_CMDWIN
    // set cedit_key
    (void)check_cedit();
#endif
#ifdef FEAT_LINEBREAK
    // initialize the table for 'breakat'.
    fill_breakat_flags();
#endif
    after_copy_winopt(curwin);
}

/*
 * More side effects of setting options.
 */
    static void
didset_options2(void)
{
    // Initialize the highlight_attr[] table.
    (void)highlight_changed();

    // Parse default for 'wildmode'
    check_opt_wim();

    // Parse default for 'listchars'.
    (void)set_chars_option(curwin, &curwin->w_p_lcs);

    // Parse default for 'fillchars'.
    (void)set_chars_option(curwin, &p_fcs);

#ifdef FEAT_CLIPBOARD
    // Parse default for 'clipboard'
    (void)check_clipboard_option();
#endif
#ifdef FEAT_VARTABS
    vim_free(curbuf->b_p_vsts_array);
    (void)tabstop_set(curbuf->b_p_vsts, &curbuf->b_p_vsts_array);
    vim_free(curbuf->b_p_vts_array);
    (void)tabstop_set(curbuf->b_p_vts,  &curbuf->b_p_vts_array);
#endif
}

/*
 * Check for string options that are NULL (normally only termcap options).
 */
    void
check_options(void)
{
    int		opt_idx;

    for (opt_idx = 0; options[opt_idx].fullname != NULL; opt_idx++)
	if ((options[opt_idx].flags & P_STRING) && options[opt_idx].var != NULL)
	    check_string_option((char_u **)get_varp(&(options[opt_idx])));
}

/*
 * Return the option index found by a pointer into term_strings[].
 * Return -1 if not found.
 */
    int
get_term_opt_idx(char_u **p)
{
    int opt_idx;

    for (opt_idx = 1; options[opt_idx].fullname != NULL; opt_idx++)
	if (options[opt_idx].var == (char_u *)p)
	    return opt_idx;
    return -1; // cannot happen: didn't find it!
}

/*
 * Mark a terminal option as allocated, found by a pointer into term_strings[].
 * Return the option index or -1 if not found.
 */
    int
set_term_option_alloced(char_u **p)
{
    int		opt_idx = get_term_opt_idx(p);

    if (opt_idx >= 0)
	options[opt_idx].flags |= P_ALLOCED;
    return opt_idx;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return TRUE when option "opt" was set from a modeline or in secure mode.
 * Return FALSE when it wasn't.
 * Return -1 for an unknown option.
 */
    int
was_set_insecurely(char_u *opt, int opt_flags)
{
    int	    idx = findoption(opt);
    long_u  *flagp;

    if (idx >= 0)
    {
	flagp = insecure_flag(idx, opt_flags);
	return (*flagp & P_INSECURE) != 0;
    }
    internal_error("was_set_insecurely()");
    return -1;
}

/*
 * Get a pointer to the flags used for the P_INSECURE flag of option
 * "opt_idx".  For some local options a local flags field is used.
 * NOTE: Caller must make sure that "curwin" is set to the window from which
 * the option is used.
 */
    static long_u *
insecure_flag(int opt_idx, int opt_flags)
{
    if (opt_flags & OPT_LOCAL)
	switch ((int)options[opt_idx].indir)
	{
#ifdef FEAT_STL_OPT
	    case PV_STL:	return &curwin->w_p_stl_flags;
#endif
#ifdef FEAT_EVAL
# ifdef FEAT_FOLDING
	    case PV_FDE:	return &curwin->w_p_fde_flags;
	    case PV_FDT:	return &curwin->w_p_fdt_flags;
# endif
# ifdef FEAT_BEVAL
	    case PV_BEXPR:	return &curbuf->b_p_bexpr_flags;
# endif
# if defined(FEAT_CINDENT)
	    case PV_INDE:	return &curbuf->b_p_inde_flags;
# endif
	    case PV_FEX:	return &curbuf->b_p_fex_flags;
# ifdef FEAT_FIND_ID
	    case PV_INEX:	return &curbuf->b_p_inex_flags;
# endif
#endif
	}

    // Nothing special, return global flags field.
    return &options[opt_idx].flags;
}
#endif

#if defined(FEAT_TITLE) || defined(PROTO)
/*
 * Redraw the window title and/or tab page text later.
 */
void redraw_titles(void)
{
    need_maketitle = TRUE;
    redraw_tabline = TRUE;
}
#endif

/*
 * Return TRUE if "val" is a valid name: only consists of alphanumeric ASCII
 * characters or characters in "allowed".
 */
    int
valid_name(char_u *val, char *allowed)
{
    char_u *s;

    for (s = val; *s != NUL; ++s)
	if (!ASCII_ISALNUM(*s) && vim_strchr((char_u *)allowed, *s) == NULL)
	    return FALSE;
    return TRUE;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Set the script_ctx for an option, taking care of setting the buffer- or
 * window-local value.
 */
    void
set_option_sctx_idx(int opt_idx, int opt_flags, sctx_T script_ctx)
{
    int		both = (opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0;
    int		indir = (int)options[opt_idx].indir;
    sctx_T	new_script_ctx = script_ctx;

    // Modeline already has the line number set.
    if (!(opt_flags & OPT_MODELINE))
	new_script_ctx.sc_lnum += SOURCING_LNUM;

    // Remember where the option was set.  For local options need to do that
    // in the buffer or window structure.
    if (both || (opt_flags & OPT_GLOBAL) || (indir & (PV_BUF|PV_WIN)) == 0)
	options[opt_idx].script_ctx = new_script_ctx;
    if (both || (opt_flags & OPT_LOCAL))
    {
	if (indir & PV_BUF)
	    curbuf->b_p_script_ctx[indir & PV_MASK] = new_script_ctx;
	else if (indir & PV_WIN)
	    curwin->w_p_script_ctx[indir & PV_MASK] = new_script_ctx;
    }
}

/*
 * Set the script_ctx for a termcap option.
 * "name" must be the two character code, e.g. "RV".
 * When "name" is NULL use "opt_idx".
 */
    void
set_term_option_sctx_idx(char *name, int opt_idx)
{
    char_u  buf[5];
    int	    idx;

    if (name == NULL)
	idx = opt_idx;
    else
    {
	buf[0] = 't';
	buf[1] = '_';
	buf[2] = name[0];
	buf[3] = name[1];
	buf[4] = 0;
	idx = findoption(buf);
    }
    if (idx >= 0)
	set_option_sctx_idx(idx, OPT_GLOBAL, current_sctx);
}
#endif

#if defined(FEAT_EVAL)
/*
 * Apply the OptionSet autocommand.
 */
    static void
apply_optionset_autocmd(
	int	opt_idx,
	long	opt_flags,
	long	oldval,
	long	oldval_g,
	long	newval,
	char	*errmsg)
{
    char_u buf_old[12], buf_old_global[12], buf_new[12], buf_type[12];

    // Don't do this while starting up, failure or recursively.
    if (starting || errmsg != NULL || *get_vim_var_str(VV_OPTION_TYPE) != NUL)
	return;

    vim_snprintf((char *)buf_old, sizeof(buf_old), "%ld", oldval);
    vim_snprintf((char *)buf_old_global, sizeof(buf_old_global), "%ld",
							oldval_g);
    vim_snprintf((char *)buf_new, sizeof(buf_new), "%ld", newval);
    vim_snprintf((char *)buf_type, sizeof(buf_type), "%s",
				(opt_flags & OPT_LOCAL) ? "local" : "global");
    set_vim_var_string(VV_OPTION_NEW, buf_new, -1);
    set_vim_var_string(VV_OPTION_OLD, buf_old, -1);
    set_vim_var_string(VV_OPTION_TYPE, buf_type, -1);
    if (opt_flags & OPT_LOCAL)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"setlocal", -1);
	set_vim_var_string(VV_OPTION_OLDLOCAL, buf_old, -1);
    }
    if (opt_flags & OPT_GLOBAL)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"setglobal", -1);
	set_vim_var_string(VV_OPTION_OLDGLOBAL, buf_old, -1);
    }
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"set", -1);
	set_vim_var_string(VV_OPTION_OLDLOCAL, buf_old, -1);
	set_vim_var_string(VV_OPTION_OLDGLOBAL, buf_old_global, -1);
    }
    if (opt_flags & OPT_MODELINE)
    {
	set_vim_var_string(VV_OPTION_COMMAND, (char_u *)"modeline", -1);
	set_vim_var_string(VV_OPTION_OLDLOCAL, buf_old, -1);
    }
    apply_autocmds(EVENT_OPTIONSET, (char_u *)options[opt_idx].fullname,
	    NULL, FALSE, NULL);
    reset_v_option_vars();
}
#endif

/*
 * Set the value of a boolean option, and take care of side effects.
 * Returns NULL for success, or an error message for an error.
 */
    static char *
set_bool_option(
    int		opt_idx,		// index in options[] table
    char_u	*varp,			// pointer to the option variable
    int		value,			// new value
    int		opt_flags)		// OPT_LOCAL and/or OPT_GLOBAL
{
    int		old_value = *(int *)varp;
#if defined(FEAT_EVAL)
    int		old_global_value = 0;
#endif

    // Disallow changing some options from secure mode
    if ((secure
#ifdef HAVE_SANDBOX
		|| sandbox != 0
#endif
		) && (options[opt_idx].flags & P_SECURE))
	return e_secure;

#if defined(FEAT_EVAL)
    // Save the global value before changing anything. This is needed as for
    // a global-only option setting the "local value" in fact sets the global
    // value (since there is only one value).
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
	old_global_value = *(int *)get_varp_scope(&(options[opt_idx]),
								   OPT_GLOBAL);
#endif

    *(int *)varp = value;	    // set the new value
#ifdef FEAT_EVAL
    // Remember where the option was set.
    set_option_sctx_idx(opt_idx, opt_flags, current_sctx);
#endif

#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

    // May set global value for local option.
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
	*(int *)get_varp_scope(&(options[opt_idx]), OPT_GLOBAL) = value;

    /*
     * Handle side effects of changing a bool option.
     */

    // 'compatible'
    if ((int *)varp == &p_cp)
	compatible_set();

#ifdef FEAT_LANGMAP
    if ((int *)varp == &p_lrm)
	// 'langremap' -> !'langnoremap'
	p_lnr = !p_lrm;
    else if ((int *)varp == &p_lnr)
	// 'langnoremap' -> !'langremap'
	p_lrm = !p_lnr;
#endif

#ifdef FEAT_SYN_HL
    else if ((int *)varp == &curwin->w_p_cul && !value && old_value)
	reset_cursorline();
#endif

#ifdef FEAT_PERSISTENT_UNDO
    // 'undofile'
    else if ((int *)varp == &curbuf->b_p_udf || (int *)varp == &p_udf)
    {
	// Only take action when the option was set. When reset we do not
	// delete the undo file, the option may be set again without making
	// any changes in between.
	if (curbuf->b_p_udf || p_udf)
	{
	    char_u	hash[UNDO_HASH_SIZE];
	    buf_T	*save_curbuf = curbuf;

	    FOR_ALL_BUFFERS(curbuf)
	    {
		// When 'undofile' is set globally: for every buffer, otherwise
		// only for the current buffer: Try to read in the undofile,
		// if one exists, the buffer wasn't changed and the buffer was
		// loaded
		if ((curbuf == save_curbuf
				|| (opt_flags & OPT_GLOBAL) || opt_flags == 0)
			&& !curbufIsChanged() && curbuf->b_ml.ml_mfp != NULL)
		{
#ifdef FEAT_CRYPT
		    if (crypt_get_method_nr(curbuf) == CRYPT_M_SOD)
			continue;
#endif
		    u_compute_hash(hash);
		    u_read_undo(NULL, hash, curbuf->b_fname);
		}
	    }
	    curbuf = save_curbuf;
	}
    }
#endif

    else if ((int *)varp == &curbuf->b_p_ro)
    {
	// when 'readonly' is reset globally, also reset readonlymode
	if (!curbuf->b_p_ro && (opt_flags & OPT_LOCAL) == 0)
	    readonlymode = FALSE;

	// when 'readonly' is set may give W10 again
	if (curbuf->b_p_ro)
	    curbuf->b_did_warn = FALSE;

#ifdef FEAT_TITLE
	redraw_titles();
#endif
    }

#ifdef FEAT_GUI
    else if ((int *)varp == &p_mh)
    {
	if (!p_mh)
	    gui_mch_mousehide(FALSE);
    }
#endif

    // when 'modifiable' is changed, redraw the window title
    else if ((int *)varp == &curbuf->b_p_ma)
    {
# ifdef FEAT_TERMINAL
	// Cannot set 'modifiable' when in Terminal mode.
	if (curbuf->b_p_ma && (term_in_normal_mode() || (bt_terminal(curbuf)
		      && curbuf->b_term != NULL && !term_is_finished(curbuf))))
	{
	    curbuf->b_p_ma = FALSE;
	    return N_("E946: Cannot make a terminal with running job modifiable");
	}
# endif
# ifdef FEAT_TITLE
	redraw_titles();
# endif
    }
#ifdef FEAT_TITLE
    // when 'endofline' is changed, redraw the window title
    else if ((int *)varp == &curbuf->b_p_eol)
    {
	redraw_titles();
    }
    // when 'fixeol' is changed, redraw the window title
    else if ((int *)varp == &curbuf->b_p_fixeol)
    {
	redraw_titles();
    }
    // when 'bomb' is changed, redraw the window title and tab page text
    else if ((int *)varp == &curbuf->b_p_bomb)
    {
	redraw_titles();
    }
#endif

    // when 'bin' is set also set some other options
    else if ((int *)varp == &curbuf->b_p_bin)
    {
	set_options_bin(old_value, curbuf->b_p_bin, opt_flags);
#ifdef FEAT_TITLE
	redraw_titles();
#endif
    }

    // when 'buflisted' changes, trigger autocommands
    else if ((int *)varp == &curbuf->b_p_bl && old_value != curbuf->b_p_bl)
    {
	apply_autocmds(curbuf->b_p_bl ? EVENT_BUFADD : EVENT_BUFDELETE,
						    NULL, NULL, TRUE, curbuf);
    }

    // when 'swf' is set, create swapfile, when reset remove swapfile
    else if ((int *)varp == &curbuf->b_p_swf)
    {
	if (curbuf->b_p_swf && p_uc)
	    ml_open_file(curbuf);		// create the swap file
	else
	    // no need to reset curbuf->b_may_swap, ml_open_file() will check
	    // buf->b_p_swf
	    mf_close_file(curbuf, TRUE);	// remove the swap file
    }

    // when 'terse' is set change 'shortmess'
    else if ((int *)varp == &p_terse)
    {
	char_u	*p;

	p = vim_strchr(p_shm, SHM_SEARCH);

	// insert 's' in p_shm
	if (p_terse && p == NULL)
	{
	    STRCPY(IObuff, p_shm);
	    STRCAT(IObuff, "s");
	    set_string_option_direct((char_u *)"shm", -1, IObuff, OPT_FREE, 0);
	}
	// remove 's' from p_shm
	else if (!p_terse && p != NULL)
	    STRMOVE(p, p + 1);
    }

    // when 'paste' is set or reset also change other options
    else if ((int *)varp == &p_paste)
    {
	paste_option_changed();
    }

    // when 'insertmode' is set from an autocommand need to do work here
    else if ((int *)varp == &p_im)
    {
	if (p_im)
	{
	    if ((State & INSERT) == 0)
		need_start_insertmode = TRUE;
	    stop_insert_mode = FALSE;
	}
	// only reset if it was set previously
	else if (old_value)
	{
	    need_start_insertmode = FALSE;
	    stop_insert_mode = TRUE;
	    if (restart_edit != 0 && mode_displayed)
		clear_cmdline = TRUE;	// remove "(insert)"
	    restart_edit = 0;
	}
    }

    // when 'ignorecase' is set or reset and 'hlsearch' is set, redraw
    else if ((int *)varp == &p_ic && p_hls)
    {
	redraw_all_later(SOME_VALID);
    }

#ifdef FEAT_SEARCH_EXTRA
    // when 'hlsearch' is set or reset: reset no_hlsearch
    else if ((int *)varp == &p_hls)
    {
	set_no_hlsearch(FALSE);
    }
#endif

    // when 'scrollbind' is set: snapshot the current position to avoid a jump
    // at the end of normal_cmd()
    else if ((int *)varp == &curwin->w_p_scb)
    {
	if (curwin->w_p_scb)
	{
	    do_check_scrollbind(FALSE);
	    curwin->w_scbind_pos = curwin->w_topline;
	}
    }

#if defined(FEAT_QUICKFIX)
    // There can be only one window with 'previewwindow' set.
    else if ((int *)varp == &curwin->w_p_pvw)
    {
	if (curwin->w_p_pvw)
	{
	    win_T	*win;

	    FOR_ALL_WINDOWS(win)
		if (win->w_p_pvw && win != curwin)
		{
		    curwin->w_p_pvw = FALSE;
		    return N_("E590: A preview window already exists");
		}
	}
    }
#endif

    // when 'textmode' is set or reset also change 'fileformat'
    else if ((int *)varp == &curbuf->b_p_tx)
    {
	set_fileformat(curbuf->b_p_tx ? EOL_DOS : EOL_UNIX, opt_flags);
    }

    // when 'textauto' is set or reset also change 'fileformats'
    else if ((int *)varp == &p_ta)
    {
	set_string_option_direct((char_u *)"ffs", -1,
				 p_ta ? (char_u *)DFLT_FFS_VIM : (char_u *)"",
						     OPT_FREE | opt_flags, 0);
    }

    /*
     * When 'lisp' option changes include/exclude '-' in
     * keyword characters.
     */
#ifdef FEAT_LISP
    else if (varp == (char_u *)&(curbuf->b_p_lisp))
    {
	(void)buf_init_chartab(curbuf, FALSE);	    // ignore errors
    }
#endif

#ifdef FEAT_TITLE
    // when 'title' changed, may need to change the title; same for 'icon'
    else if ((int *)varp == &p_title || (int *)varp == &p_icon)
    {
	did_set_title();
    }
#endif

    else if ((int *)varp == &curbuf->b_changed)
    {
	if (!value)
	    save_file_ff(curbuf);	// Buffer is unchanged
#ifdef FEAT_TITLE
	redraw_titles();
#endif
	modified_was_set = value;
    }

#ifdef BACKSLASH_IN_FILENAME
    else if ((int *)varp == &p_ssl)
    {
	if (p_ssl)
	{
	    psepc = '/';
	    psepcN = '\\';
	    pseps[0] = '/';
	}
	else
	{
	    psepc = '\\';
	    psepcN = '/';
	    pseps[0] = '\\';
	}

	// need to adjust the file name arguments and buffer names.
	buflist_slash_adjust();
	alist_slash_adjust();
# ifdef FEAT_EVAL
	scriptnames_slash_adjust();
# endif
    }
#endif

    // If 'wrap' is set, set w_leftcol to zero.
    else if ((int *)varp == &curwin->w_p_wrap)
    {
	if (curwin->w_p_wrap)
	    curwin->w_leftcol = 0;
    }

    else if ((int *)varp == &p_ea)
    {
	if (p_ea && !old_value)
	    win_equal(curwin, FALSE, 0);
    }

    else if ((int *)varp == &p_wiv)
    {
	/*
	 * When 'weirdinvert' changed, set/reset 't_xs'.
	 * Then set 'weirdinvert' according to value of 't_xs'.
	 */
	if (p_wiv && !old_value)
	    T_XS = (char_u *)"y";
	else if (!p_wiv && old_value)
	    T_XS = empty_option;
	p_wiv = (*T_XS != NUL);
    }

#ifdef FEAT_BEVAL_GUI
    else if ((int *)varp == &p_beval)
    {
	if (!balloonEvalForTerm)
	{
	    if (p_beval && !old_value)
		gui_mch_enable_beval_area(balloonEval);
	    else if (!p_beval && old_value)
		gui_mch_disable_beval_area(balloonEval);
	}
    }
#endif
#ifdef FEAT_BEVAL_TERM
    else if ((int *)varp == &p_bevalterm)
    {
	mch_bevalterm_changed();
    }
#endif

#ifdef FEAT_AUTOCHDIR
    else if ((int *)varp == &p_acd)
    {
	// Change directories when the 'acd' option is set now.
	DO_AUTOCHDIR;
    }
#endif

#ifdef FEAT_DIFF
    // 'diff'
    else if ((int *)varp == &curwin->w_p_diff)
    {
	// May add or remove the buffer from the list of diff buffers.
	diff_buf_adjust(curwin);
# ifdef FEAT_FOLDING
	if (foldmethodIsDiff(curwin))
	    foldUpdateAll(curwin);
# endif
    }
#endif

#ifdef HAVE_INPUT_METHOD
    // 'imdisable'
    else if ((int *)varp == &p_imdisable)
    {
	// Only de-activate it here, it will be enabled when changing mode.
	if (p_imdisable)
	    im_set_active(FALSE);
	else if (State & INSERT)
	    // When the option is set from an autocommand, it may need to take
	    // effect right away.
	    im_set_active(curbuf->b_p_iminsert == B_IMODE_IM);
    }
#endif

#ifdef FEAT_SPELL
    // 'spell'
    else if ((int *)varp == &curwin->w_p_spell)
    {
	if (curwin->w_p_spell)
	{
	    char	*errmsg = did_set_spelllang(curwin);

	    if (errmsg != NULL)
		emsg(_(errmsg));
	}
    }
#endif

#ifdef FEAT_ARABIC
    if ((int *)varp == &curwin->w_p_arab)
    {
	if (curwin->w_p_arab)
	{
	    /*
	     * 'arabic' is set, handle various sub-settings.
	     */
	    if (!p_tbidi)
	    {
		// set rightleft mode
		if (!curwin->w_p_rl)
		{
		    curwin->w_p_rl = TRUE;
		    changed_window_setting();
		}

		// Enable Arabic shaping (major part of what Arabic requires)
		if (!p_arshape)
		{
		    p_arshape = TRUE;
		    redraw_later_clear();
		}
	    }

	    // Arabic requires a utf-8 encoding, inform the user if its not
	    // set.
	    if (STRCMP(p_enc, "utf-8") != 0)
	    {
		static char *w_arabic = N_("W17: Arabic requires UTF-8, do ':set encoding=utf-8'");

		msg_source(HL_ATTR(HLF_W));
		msg_attr(_(w_arabic), HL_ATTR(HLF_W));
#ifdef FEAT_EVAL
		set_vim_var_string(VV_WARNINGMSG, (char_u *)_(w_arabic), -1);
#endif
	    }

	    // set 'delcombine'
	    p_deco = TRUE;

# ifdef FEAT_KEYMAP
	    // Force-set the necessary keymap for arabic
	    set_option_value((char_u *)"keymap", 0L, (char_u *)"arabic",
								   OPT_LOCAL);
# endif
	}
	else
	{
	    /*
	     * 'arabic' is reset, handle various sub-settings.
	     */
	    if (!p_tbidi)
	    {
		// reset rightleft mode
		if (curwin->w_p_rl)
		{
		    curwin->w_p_rl = FALSE;
		    changed_window_setting();
		}

		// 'arabicshape' isn't reset, it is a global option and
		// another window may still need it "on".
	    }

	    // 'delcombine' isn't reset, it is a global option and another
	    // window may still want it "on".

# ifdef FEAT_KEYMAP
	    // Revert to the default keymap
	    curbuf->b_p_iminsert = B_IMODE_NONE;
	    curbuf->b_p_imsearch = B_IMODE_USE_INSERT;
# endif
	}
    }

#endif

#if defined(FEAT_SIGNS) && defined(FEAT_GUI)
    else if (((int *)varp == &curwin->w_p_nu
		|| (int *)varp == &curwin->w_p_rnu)
	    && gui.in_use
	    && (*curwin->w_p_scl == 'n' && *(curwin->w_p_scl + 1) == 'u')
	    && curbuf->b_signlist != NULL)
    {
	// If the 'number' or 'relativenumber' options are modified and
	// 'signcolumn' is set to 'number', then clear the screen for a full
	// refresh. Otherwise the sign icons are not displayed properly in the
	// number column.  If the 'number' option is set and only the
	// 'relativenumber' option is toggled, then don't refresh the screen
	// (optimization).
	if (!(curwin->w_p_nu && ((int *)varp == &curwin->w_p_rnu)))
	    redraw_all_later(CLEAR);
    }
#endif

#ifdef FEAT_TERMGUICOLORS
    // 'termguicolors'
    else if ((int *)varp == &p_tgc)
    {
# ifdef FEAT_VTP
	// Do not turn on 'tgc' when 24-bit colors are not supported.
	if (
#  ifdef VIMDLL
	    !gui.in_use && !gui.starting &&
#  endif
	    !has_vtp_working())
	{
	    p_tgc = 0;
	    return N_("E954: 24-bit colors are not supported on this environment");
	}
	if (is_term_win32())
	    swap_tcap();
# endif
# ifdef FEAT_GUI
	if (!gui.in_use && !gui.starting)
# endif
	    highlight_gui_started();
# ifdef FEAT_VTP
	// reset t_Co
	if (is_term_win32())
	{
	    control_console_color_rgb();
	    set_termname(T_NAME);
	    init_highlight(TRUE, FALSE);
	}
# endif
    }
#endif

    /*
     * End of handling side effects for bool options.
     */

    // after handling side effects, call autocommand

    options[opt_idx].flags |= P_WAS_SET;

#if defined(FEAT_EVAL)
    apply_optionset_autocmd(opt_idx, opt_flags,
				(long)(old_value ? TRUE : FALSE),
				(long)(old_global_value ? TRUE : FALSE),
				(long)(value ? TRUE : FALSE), NULL);
#endif

    comp_col();			    // in case 'ruler' or 'showcmd' changed
    if (curwin->w_curswant != MAXCOL
		     && (options[opt_idx].flags & (P_CURSWANT | P_RALL)) != 0)
	curwin->w_set_curswant = TRUE;

    if ((opt_flags & OPT_NO_REDRAW) == 0)
	check_redraw(options[opt_idx].flags);

    return NULL;
}

/*
 * Set the value of a number option, and take care of side effects.
 * Returns NULL for success, or an error message for an error.
 */
    static char *
set_num_option(
    int		opt_idx,		// index in options[] table
    char_u	*varp,			// pointer to the option variable
    long	value,			// new value
    char	*errbuf,		// buffer for error messages
    size_t	errbuflen,		// length of "errbuf"
    int		opt_flags)		// OPT_LOCAL, OPT_GLOBAL,
					// OPT_MODELINE, etc.
{
    char	*errmsg = NULL;
    long	old_value = *(long *)varp;
#if defined(FEAT_EVAL)
    long	old_global_value = 0;	// only used when setting a local and
					// global option
#endif
    long	old_Rows = Rows;	// remember old Rows
    long	old_Columns = Columns;	// remember old Columns
    long	*pp = (long *)varp;

    // Disallow changing some options from secure mode.
    if ((secure
#ifdef HAVE_SANDBOX
		|| sandbox != 0
#endif
		) && (options[opt_idx].flags & P_SECURE))
	return e_secure;

#if defined(FEAT_EVAL)
    // Save the global value before changing anything. This is needed as for
    // a global-only option setting the "local value" in fact sets the global
    // value (since there is only one value).
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
	old_global_value = *(long *)get_varp_scope(&(options[opt_idx]),
								   OPT_GLOBAL);
#endif

    *pp = value;
#ifdef FEAT_EVAL
    // Remember where the option was set.
    set_option_sctx_idx(opt_idx, opt_flags, current_sctx);
#endif
#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

    if (curbuf->b_p_sw < 0)
    {
	errmsg = e_positive;
#ifdef FEAT_VARTABS
	// Use the first 'vartabstop' value, or 'tabstop' if vts isn't in use.
	curbuf->b_p_sw = tabstop_count(curbuf->b_p_vts_array) > 0
	               ? tabstop_first(curbuf->b_p_vts_array)
		       : curbuf->b_p_ts;
#else
	curbuf->b_p_sw = curbuf->b_p_ts;
#endif
    }

    /*
     * Number options that need some action when changed
     */
    if (pp == &p_wh || pp == &p_hh)
    {
	// 'winheight' and 'helpheight'
	if (p_wh < 1)
	{
	    errmsg = e_positive;
	    p_wh = 1;
	}
	if (p_wmh > p_wh)
	{
	    errmsg = e_winheight;
	    p_wh = p_wmh;
	}
	if (p_hh < 0)
	{
	    errmsg = e_positive;
	    p_hh = 0;
	}

	// Change window height NOW
	if (!ONE_WINDOW)
	{
	    if (pp == &p_wh && curwin->w_height < p_wh)
		win_setheight((int)p_wh);
	    if (pp == &p_hh && curbuf->b_help && curwin->w_height < p_hh)
		win_setheight((int)p_hh);
	}
    }
    else if (pp == &p_wmh)
    {
	// 'winminheight'
	if (p_wmh < 0)
	{
	    errmsg = e_positive;
	    p_wmh = 0;
	}
	if (p_wmh > p_wh)
	{
	    errmsg = e_winheight;
	    p_wmh = p_wh;
	}
	win_setminheight();
    }
    else if (pp == &p_wiw)
    {
	// 'winwidth'
	if (p_wiw < 1)
	{
	    errmsg = e_positive;
	    p_wiw = 1;
	}
	if (p_wmw > p_wiw)
	{
	    errmsg = e_winwidth;
	    p_wiw = p_wmw;
	}

	// Change window width NOW
	if (!ONE_WINDOW && curwin->w_width < p_wiw)
	    win_setwidth((int)p_wiw);
    }
    else if (pp == &p_wmw)
    {
	// 'winminwidth'
	if (p_wmw < 0)
	{
	    errmsg = e_positive;
	    p_wmw = 0;
	}
	if (p_wmw > p_wiw)
	{
	    errmsg = e_winwidth;
	    p_wmw = p_wiw;
	}
	win_setminwidth();
    }

    // (re)set last window status line
    else if (pp == &p_ls)
    {
	last_status(FALSE);
    }

    // (re)set tab page line
    else if (pp == &p_stal)
    {
	shell_new_rows();	// recompute window positions and heights
    }

#ifdef FEAT_GUI
    else if (pp == &p_linespace)
    {
	// Recompute gui.char_height and resize the Vim window to keep the
	// same number of lines.
	if (gui.in_use && gui_mch_adjust_charheight() == OK)
	    gui_set_shellsize(FALSE, FALSE, RESIZE_VERT);
    }
#endif

#ifdef FEAT_FOLDING
    // 'foldlevel'
    else if (pp == &curwin->w_p_fdl)
    {
	if (curwin->w_p_fdl < 0)
	    curwin->w_p_fdl = 0;
	newFoldLevel();
    }

    // 'foldminlines'
    else if (pp == &curwin->w_p_fml)
    {
	foldUpdateAll(curwin);
    }

    // 'foldnestmax'
    else if (pp == &curwin->w_p_fdn)
    {
	if (foldmethodIsSyntax(curwin) || foldmethodIsIndent(curwin))
	    foldUpdateAll(curwin);
    }

    // 'foldcolumn'
    else if (pp == &curwin->w_p_fdc)
    {
	if (curwin->w_p_fdc < 0)
	{
	    errmsg = e_positive;
	    curwin->w_p_fdc = 0;
	}
	else if (curwin->w_p_fdc > 12)
	{
	    errmsg = e_invarg;
	    curwin->w_p_fdc = 12;
	}
    }
#endif // FEAT_FOLDING

#if defined(FEAT_FOLDING) || defined(FEAT_CINDENT)
    // 'shiftwidth' or 'tabstop'
    else if (pp == &curbuf->b_p_sw || pp == &curbuf->b_p_ts)
    {
# ifdef FEAT_FOLDING
	if (foldmethodIsIndent(curwin))
	    foldUpdateAll(curwin);
# endif
# ifdef FEAT_CINDENT
	// When 'shiftwidth' changes, or it's zero and 'tabstop' changes:
	// parse 'cinoptions'.
	if (pp == &curbuf->b_p_sw || curbuf->b_p_sw == 0)
	    parse_cino(curbuf);
# endif
    }
#endif

    // 'maxcombine'
    else if (pp == &p_mco)
    {
	if (p_mco > MAX_MCO)
	    p_mco = MAX_MCO;
	else if (p_mco < 0)
	    p_mco = 0;
	screenclear();	    // will re-allocate the screen
    }

    else if (pp == &curbuf->b_p_iminsert)
    {
	if (curbuf->b_p_iminsert < 0 || curbuf->b_p_iminsert > B_IMODE_LAST)
	{
	    errmsg = e_invarg;
	    curbuf->b_p_iminsert = B_IMODE_NONE;
	}
	p_iminsert = curbuf->b_p_iminsert;
	if (termcap_active)	// don't do this in the alternate screen
	    showmode();
#if defined(FEAT_KEYMAP)
	// Show/unshow value of 'keymap' in status lines.
	status_redraw_curbuf();
#endif
    }

#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
    // 'imstyle'
    else if (pp == &p_imst)
    {
	if (p_imst != IM_ON_THE_SPOT && p_imst != IM_OVER_THE_SPOT)
	    errmsg = e_invarg;
    }
#endif

    else if (pp == &p_window)
    {
	if (p_window < 1)
	    p_window = 1;
	else if (p_window >= Rows)
	    p_window = Rows - 1;
    }

    else if (pp == &curbuf->b_p_imsearch)
    {
	if (curbuf->b_p_imsearch < -1 || curbuf->b_p_imsearch > B_IMODE_LAST)
	{
	    errmsg = e_invarg;
	    curbuf->b_p_imsearch = B_IMODE_NONE;
	}
	p_imsearch = curbuf->b_p_imsearch;
    }

#ifdef FEAT_TITLE
    // if 'titlelen' has changed, redraw the title
    else if (pp == &p_titlelen)
    {
	if (p_titlelen < 0)
	{
	    errmsg = e_positive;
	    p_titlelen = 85;
	}
	if (starting != NO_SCREEN && old_value != p_titlelen)
	    need_maketitle = TRUE;
    }
#endif

    // if p_ch changed value, change the command line height
    else if (pp == &p_ch)
    {
	if (p_ch < 1)
	{
	    errmsg = e_positive;
	    p_ch = 1;
	}
	if (p_ch > Rows - min_rows() + 1)
	    p_ch = Rows - min_rows() + 1;

	// Only compute the new window layout when startup has been
	// completed. Otherwise the frame sizes may be wrong.
	if (p_ch != old_value && full_screen
#ifdef FEAT_GUI
		&& !gui.starting
#endif
	   )
	    command_height();
    }

    // when 'updatecount' changes from zero to non-zero, open swap files
    else if (pp == &p_uc)
    {
	if (p_uc < 0)
	{
	    errmsg = e_positive;
	    p_uc = 100;
	}
	if (p_uc && !old_value)
	    ml_open_files();
    }
#ifdef FEAT_CONCEAL
    else if (pp == &curwin->w_p_cole)
    {
	if (curwin->w_p_cole < 0)
	{
	    errmsg = e_positive;
	    curwin->w_p_cole = 0;
	}
	else if (curwin->w_p_cole > 3)
	{
	    errmsg = e_invarg;
	    curwin->w_p_cole = 3;
	}
    }
#endif
#ifdef MZSCHEME_GUI_THREADS
    else if (pp == &p_mzq)
	mzvim_reset_timer();
#endif

#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
    // 'pyxversion'
    else if (pp == &p_pyx)
    {
	if (p_pyx != 0 && p_pyx != 2 && p_pyx != 3)
	    errmsg = e_invarg;
    }
#endif

    // sync undo before 'undolevels' changes
    else if (pp == &p_ul)
    {
	// use the old value, otherwise u_sync() may not work properly
	p_ul = old_value;
	u_sync(TRUE);
	p_ul = value;
    }
    else if (pp == &curbuf->b_p_ul)
    {
	// use the old value, otherwise u_sync() may not work properly
	curbuf->b_p_ul = old_value;
	u_sync(TRUE);
	curbuf->b_p_ul = value;
    }

#ifdef FEAT_LINEBREAK
    // 'numberwidth' must be positive
    else if (pp == &curwin->w_p_nuw)
    {
	if (curwin->w_p_nuw < 1)
	{
	    errmsg = e_positive;
	    curwin->w_p_nuw = 1;
	}
	if (curwin->w_p_nuw > 20)
	{
	    errmsg = e_invarg;
	    curwin->w_p_nuw = 20;
	}
	curwin->w_nrwidth_line_count = 0; // trigger a redraw
    }
#endif

    else if (pp == &curbuf->b_p_tw)
    {
	if (curbuf->b_p_tw < 0)
	{
	    errmsg = e_positive;
	    curbuf->b_p_tw = 0;
	}
#ifdef FEAT_SYN_HL
	{
	    win_T	*wp;
	    tabpage_T	*tp;

	    FOR_ALL_TAB_WINDOWS(tp, wp)
		check_colorcolumn(wp);
	}
#endif
    }

    /*
     * Check the bounds for numeric options here
     */
    if (Rows < min_rows() && full_screen)
    {
	if (errbuf != NULL)
	{
	    vim_snprintf((char *)errbuf, errbuflen,
			       _("E593: Need at least %d lines"), min_rows());
	    errmsg = errbuf;
	}
	Rows = min_rows();
    }
    if (Columns < MIN_COLUMNS && full_screen)
    {
	if (errbuf != NULL)
	{
	    vim_snprintf((char *)errbuf, errbuflen,
			    _("E594: Need at least %d columns"), MIN_COLUMNS);
	    errmsg = errbuf;
	}
	Columns = MIN_COLUMNS;
    }
    limit_screen_size();

    /*
     * If the screen (shell) height has been changed, assume it is the
     * physical screenheight.
     */
    if (old_Rows != Rows || old_Columns != Columns)
    {
	// Changing the screen size is not allowed while updating the screen.
	if (updating_screen)
	    *pp = old_value;
	else if (full_screen
#ifdef FEAT_GUI
		&& !gui.starting
#endif
	    )
	    set_shellsize((int)Columns, (int)Rows, TRUE);
	else
	{
	    // Postpone the resizing; check the size and cmdline position for
	    // messages.
	    check_shellsize();
	    if (cmdline_row > Rows - p_ch && Rows > p_ch)
		cmdline_row = Rows - p_ch;
	}
	if (p_window >= Rows || !option_was_set((char_u *)"window"))
	    p_window = Rows - 1;
    }

    if (curbuf->b_p_ts <= 0)
    {
	errmsg = e_positive;
	curbuf->b_p_ts = 8;
    }
    if (p_tm < 0)
    {
	errmsg = e_positive;
	p_tm = 0;
    }
    if ((curwin->w_p_scr <= 0
		|| (curwin->w_p_scr > curwin->w_height
		    && curwin->w_height > 0))
	    && full_screen)
    {
	if (pp == &(curwin->w_p_scr))
	{
	    if (curwin->w_p_scr != 0)
		errmsg = e_invalid_scroll_size;
	    win_comp_scroll(curwin);
	}
	// If 'scroll' became invalid because of a side effect silently adjust
	// it.
	else if (curwin->w_p_scr <= 0)
	    curwin->w_p_scr = 1;
	else // curwin->w_p_scr > curwin->w_height
	    curwin->w_p_scr = curwin->w_height;
    }
    if (p_hi < 0)
    {
	errmsg = e_positive;
	p_hi = 0;
    }
    else if (p_hi > 10000)
    {
	errmsg = e_invarg;
	p_hi = 10000;
    }
    if (p_re < 0 || p_re > 2)
    {
	errmsg = e_invarg;
	p_re = 0;
    }
    if (p_report < 0)
    {
	errmsg = e_positive;
	p_report = 1;
    }
    if ((p_sj < -100 || p_sj >= Rows) && full_screen)
    {
	if (Rows != old_Rows)	// Rows changed, just adjust p_sj
	    p_sj = Rows / 2;
	else
	{
	    errmsg = e_invalid_scroll_size;
	    p_sj = 1;
	}
    }
    if (p_so < 0 && full_screen)
    {
	errmsg = e_positive;
	p_so = 0;
    }
    if (p_siso < 0 && full_screen)
    {
	errmsg = e_positive;
	p_siso = 0;
    }
#ifdef FEAT_CMDWIN
    if (p_cwh < 1)
    {
	errmsg = e_positive;
	p_cwh = 1;
    }
#endif
    if (p_ut < 0)
    {
	errmsg = e_positive;
	p_ut = 2000;
    }
    if (p_ss < 0)
    {
	errmsg = e_positive;
	p_ss = 0;
    }

    // May set global value for local option.
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
	*(long *)get_varp_scope(&(options[opt_idx]), OPT_GLOBAL) = *pp;

    options[opt_idx].flags |= P_WAS_SET;

#if defined(FEAT_EVAL)
    apply_optionset_autocmd(opt_idx, opt_flags, old_value, old_global_value,
								value, errmsg);
#endif

    comp_col();			    // in case 'columns' or 'ls' changed
    if (curwin->w_curswant != MAXCOL
		     && (options[opt_idx].flags & (P_CURSWANT | P_RALL)) != 0)
	curwin->w_set_curswant = TRUE;
    if ((opt_flags & OPT_NO_REDRAW) == 0)
	check_redraw(options[opt_idx].flags);

    return errmsg;
}

/*
 * Called after an option changed: check if something needs to be redrawn.
 */
    void
check_redraw(long_u flags)
{
    // Careful: P_RCLR and P_RALL are a combination of other P_ flags
    int		doclear = (flags & P_RCLR) == P_RCLR;
    int		all = ((flags & P_RALL) == P_RALL || doclear);

    if ((flags & P_RSTAT) || all)	// mark all status lines dirty
	status_redraw_all();

    if ((flags & P_RBUF) || (flags & P_RWIN) || all)
	changed_window_setting();
    if (flags & P_RBUF)
	redraw_curbuf_later(NOT_VALID);
    if (flags & P_RWINONLY)
	redraw_later(NOT_VALID);
    if (doclear)
	redraw_all_later(CLEAR);
    else if (all)
	redraw_all_later(NOT_VALID);
}

/*
 * Find index for option 'arg'.
 * Return -1 if not found.
 */
    int
findoption(char_u *arg)
{
    int		    opt_idx;
    char	    *s, *p;
    static short    quick_tab[27] = {0, 0};	// quick access table
    int		    is_term_opt;

    /*
     * For first call: Initialize the quick-access table.
     * It contains the index for the first option that starts with a certain
     * letter.  There are 26 letters, plus the first "t_" option.
     */
    if (quick_tab[1] == 0)
    {
	p = options[0].fullname;
	for (opt_idx = 1; (s = options[opt_idx].fullname) != NULL; opt_idx++)
	{
	    if (s[0] != p[0])
	    {
		if (s[0] == 't' && s[1] == '_')
		    quick_tab[26] = opt_idx;
		else
		    quick_tab[CharOrdLow(s[0])] = opt_idx;
	    }
	    p = s;
	}
    }

    /*
     * Check for name starting with an illegal character.
     */
#ifdef EBCDIC
    if (!islower(arg[0]))
#else
    if (arg[0] < 'a' || arg[0] > 'z')
#endif
	return -1;

    is_term_opt = (arg[0] == 't' && arg[1] == '_');
    if (is_term_opt)
	opt_idx = quick_tab[26];
    else
	opt_idx = quick_tab[CharOrdLow(arg[0])];
    for ( ; (s = options[opt_idx].fullname) != NULL; opt_idx++)
    {
	if (STRCMP(arg, s) == 0)		    // match full name
	    break;
    }
    if (s == NULL && !is_term_opt)
    {
	opt_idx = quick_tab[CharOrdLow(arg[0])];
	for ( ; options[opt_idx].fullname != NULL; opt_idx++)
	{
	    s = options[opt_idx].shortname;
	    if (s != NULL && STRCMP(arg, s) == 0)   // match short name
		break;
	    s = NULL;
	}
    }
    if (s == NULL)
	opt_idx = -1;
    return opt_idx;
}

#if defined(FEAT_EVAL) || defined(FEAT_TCL) || defined(FEAT_MZSCHEME)
/*
 * Get the value for an option.
 *
 * Returns:
 * Number option: gov_number, *numval gets value.
 * Toggle option: gov_bool,   *numval gets value.
 * String option: gov_string, *stringval gets allocated string.
 * Hidden Number option: gov_hidden_number.
 * Hidden Toggle option: gov_hidden_bool.
 * Hidden String option: gov_hidden_string.
 * Unknown option: gov_unknown.
 */
    getoption_T
get_option_value(
    char_u	*name,
    long	*numval,
    char_u	**stringval,	    // NULL when only checking existence
    int		opt_flags)
{
    int		opt_idx;
    char_u	*varp;

    opt_idx = findoption(name);
    if (opt_idx < 0)		    // option not in the table
    {
	int key;

	if (STRLEN(name) == 4 && name[0] == 't' && name[1] == '_'
				  && (key = find_key_option(name, FALSE)) != 0)
	{
	    char_u key_name[2];
	    char_u *p;

	    // check for a terminal option
	    if (key < 0)
	    {
		key_name[0] = KEY2TERMCAP0(key);
		key_name[1] = KEY2TERMCAP1(key);
	    }
	    else
	    {
		key_name[0] = KS_KEY;
		key_name[1] = (key & 0xff);
	    }
	    p = find_termcode(key_name);
	    if (p != NULL)
	    {
		if (stringval != NULL)
		    *stringval = vim_strsave(p);
		return gov_string;
	    }
	}
	return gov_unknown;
    }

    varp = get_varp_scope(&(options[opt_idx]), opt_flags);

    if (options[opt_idx].flags & P_STRING)
    {
	if (varp == NULL)		    // hidden option
	    return gov_hidden_string;
	if (stringval != NULL)
	{
#ifdef FEAT_CRYPT
	    // never return the value of the crypt key
	    if ((char_u **)varp == &curbuf->b_p_key
						&& **(char_u **)(varp) != NUL)
		*stringval = vim_strsave((char_u *)"*****");
	    else
#endif
		*stringval = vim_strsave(*(char_u **)(varp));
	}
	return gov_string;
    }

    if (varp == NULL)		    // hidden option
	return (options[opt_idx].flags & P_NUM)
					 ? gov_hidden_number : gov_hidden_bool;
    if (options[opt_idx].flags & P_NUM)
	*numval = *(long *)varp;
    else
    {
	// Special case: 'modified' is b_changed, but we also want to consider
	// it set when 'ff' or 'fenc' changed.
	if ((int *)varp == &curbuf->b_changed)
	    *numval = curbufIsChanged();
	else
	    *numval = (long) *(int *)varp;
    }
    return (options[opt_idx].flags & P_NUM) ? gov_number : gov_bool;
}
#endif

#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3) || defined(PROTO)
/*
 * Returns the option attributes and its value. Unlike the above function it
 * will return either global value or local value of the option depending on
 * what was requested, but it will never return global value if it was
 * requested to return local one and vice versa. Neither it will return
 * buffer-local value if it was requested to return window-local one.
 *
 * Pretends that option is absent if it is not present in the requested scope
 * (i.e. has no global, window-local or buffer-local value depending on
 * opt_type). Uses
 *
 * Returned flags:
 *       0 hidden or unknown option, also option that does not have requested
 *	   type (see SREQ_* in vim.h)
 *  see SOPT_* in vim.h for other flags
 *
 * Possible opt_type values: see SREQ_* in vim.h
 */
    int
get_option_value_strict(
    char_u	*name,
    long	*numval,
    char_u	**stringval,	    // NULL when only obtaining attributes
    int		opt_type,
    void	*from)
{
    int		opt_idx;
    char_u	*varp = NULL;
    struct vimoption *p;
    int		r = 0;

    opt_idx = findoption(name);
    if (opt_idx < 0)
	return 0;

    p = &(options[opt_idx]);

    // Hidden option
    if (p->var == NULL)
	return 0;

    if (p->flags & P_BOOL)
	r |= SOPT_BOOL;
    else if (p->flags & P_NUM)
	r |= SOPT_NUM;
    else if (p->flags & P_STRING)
	r |= SOPT_STRING;

    if (p->indir == PV_NONE)
    {
	if (opt_type == SREQ_GLOBAL)
	    r |= SOPT_GLOBAL;
	else
	    return 0; // Did not request global-only option
    }
    else
    {
	if (p->indir & PV_BOTH)
	    r |= SOPT_GLOBAL;
	else if (opt_type == SREQ_GLOBAL)
	    return 0; // Requested global option

	if (p->indir & PV_WIN)
	{
	    if (opt_type == SREQ_BUF)
		return 0; // Did not request window-local option
	    else
		r |= SOPT_WIN;
	}
	else if (p->indir & PV_BUF)
	{
	    if (opt_type == SREQ_WIN)
		return 0; // Did not request buffer-local option
	    else
		r |= SOPT_BUF;
	}
    }

    if (stringval == NULL)
	return r;

    if (opt_type == SREQ_GLOBAL)
	varp = p->var;
    else
    {
	if (opt_type == SREQ_BUF)
	{
	    // Special case: 'modified' is b_changed, but we also want to
	    // consider it set when 'ff' or 'fenc' changed.
	    if (p->indir == PV_MOD)
	    {
		*numval = bufIsChanged((buf_T *)from);
		varp = NULL;
	    }
#ifdef FEAT_CRYPT
	    else if (p->indir == PV_KEY)
	    {
		// never return the value of the crypt key
		*stringval = NULL;
		varp = NULL;
	    }
#endif
	    else
	    {
		buf_T *save_curbuf = curbuf;

		// only getting a pointer, no need to use aucmd_prepbuf()
		curbuf = (buf_T *)from;
		curwin->w_buffer = curbuf;
		varp = get_varp(p);
		curbuf = save_curbuf;
		curwin->w_buffer = curbuf;
	    }
	}
	else if (opt_type == SREQ_WIN)
	{
	    win_T	*save_curwin = curwin;

	    curwin = (win_T *)from;
	    curbuf = curwin->w_buffer;
	    varp = get_varp(p);
	    curwin = save_curwin;
	    curbuf = curwin->w_buffer;
	}
	if (varp == p->var)
	    return (r | SOPT_UNSET);
    }

    if (varp != NULL)
    {
	if (p->flags & P_STRING)
	    *stringval = vim_strsave(*(char_u **)(varp));
	else if (p->flags & P_NUM)
	    *numval = *(long *) varp;
	else
	    *numval = *(int *)varp;
    }

    return r;
}

/*
 * Iterate over options. First argument is a pointer to a pointer to a
 * structure inside options[] array, second is option type like in the above
 * function.
 *
 * If first argument points to NULL it is assumed that iteration just started
 * and caller needs the very first value.
 * If first argument points to the end marker function returns NULL and sets
 * first argument to NULL.
 *
 * Returns full option name for current option on each call.
 */
    char_u *
option_iter_next(void **option, int opt_type)
{
    struct vimoption	*ret = NULL;
    do
    {
	if (*option == NULL)
	    *option = (void *) options;
	else if (((struct vimoption *) (*option))->fullname == NULL)
	{
	    *option = NULL;
	    return NULL;
	}
	else
	    *option = (void *) (((struct vimoption *) (*option)) + 1);

	ret = ((struct vimoption *) (*option));

	// Hidden option
	if (ret->var == NULL)
	{
	    ret = NULL;
	    continue;
	}

	switch (opt_type)
	{
	    case SREQ_GLOBAL:
		if (!(ret->indir == PV_NONE || ret->indir & PV_BOTH))
		    ret = NULL;
		break;
	    case SREQ_BUF:
		if (!(ret->indir & PV_BUF))
		    ret = NULL;
		break;
	    case SREQ_WIN:
		if (!(ret->indir & PV_WIN))
		    ret = NULL;
		break;
	    default:
		internal_error("option_iter_next()");
		return NULL;
	}
    }
    while (ret == NULL);

    return (char_u *)ret->fullname;
}
#endif

/*
 * Return the flags for the option at 'opt_idx'.
 */
    long_u
get_option_flags(int opt_idx)
{
    return options[opt_idx].flags;
}

/*
 * Set a flag for the option at 'opt_idx'.
 */
    void
set_option_flag(int opt_idx, long_u flag)
{
    options[opt_idx].flags |= flag;
}

/*
 * Clear a flag for the option at 'opt_idx'.
 */
    void
clear_option_flag(int opt_idx, long_u flag)
{
    options[opt_idx].flags &= ~flag;
}

/*
 * Returns TRUE if the option at 'opt_idx' is a global option
 */
    int
is_global_option(int opt_idx)
{
    return options[opt_idx].indir == PV_NONE;
}

/*
 * Returns TRUE if the option at 'opt_idx' is a global option which also has a
 * local value.
 */
    int
is_global_local_option(int opt_idx)
{
    return options[opt_idx].indir & PV_BOTH;
}

/*
 * Returns TRUE if the option at 'opt_idx' is a window-local option
 */
    int
is_window_local_option(int opt_idx)
{
    return options[opt_idx].var == VAR_WIN;
}

/*
 * Returns TRUE if the option at 'opt_idx' is a hidden option
 */
    int
is_hidden_option(int opt_idx)
{
    return options[opt_idx].var == NULL;
}

#if defined(FEAT_CRYPT) || defined(PROTO)
/*
 * Returns TRUE if the option at 'opt_idx' is a crypt key option
 */
    int
is_crypt_key_option(int opt_idx)
{
    return options[opt_idx].indir == PV_KEY;
}
#endif

/*
 * Set the value of option "name".
 * Use "string" for string options, use "number" for other options.
 *
 * Returns NULL on success or error message on error.
 */
    char *
set_option_value(
    char_u	*name,
    long	number,
    char_u	*string,
    int		opt_flags)	// OPT_LOCAL or 0 (both)
{
    int		opt_idx;
    char_u	*varp;
    long_u	flags;

    opt_idx = findoption(name);
    if (opt_idx < 0)
    {
	int key;

	if (STRLEN(name) == 4 && name[0] == 't' && name[1] == '_'
		&& (key = find_key_option(name, FALSE)) != 0)
	{
	    char_u key_name[2];

	    if (key < 0)
	    {
		key_name[0] = KEY2TERMCAP0(key);
		key_name[1] = KEY2TERMCAP1(key);
	    }
	    else
	    {
		key_name[0] = KS_KEY;
		key_name[1] = (key & 0xff);
	    }
	    add_termcode(key_name, string, FALSE);
	    if (full_screen)
		ttest(FALSE);
	    redraw_all_later(CLEAR);
	    return NULL;
	}

	semsg(_("E355: Unknown option: %s"), name);
    }
    else
    {
	flags = options[opt_idx].flags;
#ifdef HAVE_SANDBOX
	// Disallow changing some options in the sandbox
	if (sandbox > 0 && (flags & P_SECURE))
	{
	    emsg(_(e_not_allowed_in_sandbox));
	    return NULL;
	}
#endif
	if (flags & P_STRING)
	    return set_string_option(opt_idx, string, opt_flags);
	else
	{
	    varp = get_varp_scope(&(options[opt_idx]), opt_flags);
	    if (varp != NULL)	// hidden option is not changed
	    {
		if (number == 0 && string != NULL)
		{
		    int idx;

		    // Either we are given a string or we are setting option
		    // to zero.
		    for (idx = 0; string[idx] == '0'; ++idx)
			;
		    if (string[idx] != NUL || idx == 0)
		    {
			// There's another character after zeros or the string
			// is empty.  In both cases, we are trying to set a
			// num option using a string.
			semsg(_("E521: Number required: &%s = '%s'"),
								name, string);
			return NULL;     // do nothing as we hit an error

		    }
		}
		if (flags & P_NUM)
		    return set_num_option(opt_idx, varp, number,
							  NULL, 0, opt_flags);
		else
		    return set_bool_option(opt_idx, varp, (int)number,
								   opt_flags);
	    }
	}
    }
    return NULL;
}

/*
 * Get the terminal code for a terminal option.
 * Returns NULL when not found.
 */
    char_u *
get_term_code(char_u *tname)
{
    int	    opt_idx;
    char_u  *varp;

    if (tname[0] != 't' || tname[1] != '_' ||
	    tname[2] == NUL || tname[3] == NUL)
	return NULL;
    if ((opt_idx = findoption(tname)) >= 0)
    {
	varp = get_varp(&(options[opt_idx]));
	if (varp != NULL)
	    varp = *(char_u **)(varp);
	return varp;
    }
    return find_termcode(tname + 2);
}

    char_u *
get_highlight_default(void)
{
    int i;

    i = findoption((char_u *)"hl");
    if (i >= 0)
	return options[i].def_val[VI_DEFAULT];
    return (char_u *)NULL;
}

    char_u *
get_encoding_default(void)
{
    int i;

    i = findoption((char_u *)"enc");
    if (i >= 0)
	return options[i].def_val[VI_DEFAULT];
    return (char_u *)NULL;
}

/*
 * Translate a string like "t_xx", "<t_xx>" or "<S-Tab>" to a key number.
 * When "has_lt" is true there is a '<' before "*arg_arg".
 * Returns 0 when the key is not recognized.
 */
    static int
find_key_option(char_u *arg_arg, int has_lt)
{
    int		key = 0;
    int		modifiers;
    char_u	*arg = arg_arg;

    /*
     * Don't use get_special_key_code() for t_xx, we don't want it to call
     * add_termcap_entry().
     */
    if (arg[0] == 't' && arg[1] == '_' && arg[2] && arg[3])
	key = TERMCAP2KEY(arg[2], arg[3]);
    else if (has_lt)
    {
	--arg;			    // put arg at the '<'
	modifiers = 0;
	key = find_special_key(&arg, &modifiers,
			    FSK_KEYCODE | FSK_KEEP_X_KEY | FSK_SIMPLIFY, NULL);
	if (modifiers)		    // can't handle modifiers here
	    key = 0;
    }
    return key;
}

/*
 * if 'all' == 0: show changed options
 * if 'all' == 1: show all normal options
 * if 'all' == 2: show all terminal options
 */
    static void
showoptions(
    int		all,
    int		opt_flags)	// OPT_LOCAL and/or OPT_GLOBAL
{
    struct vimoption	*p;
    int			col;
    int			isterm;
    char_u		*varp;
    struct vimoption	**items;
    int			item_count;
    int			run;
    int			row, rows;
    int			cols;
    int			i;
    int			len;

#define INC 20
#define GAP 3

    items = ALLOC_MULT(struct vimoption *, OPTION_COUNT);
    if (items == NULL)
	return;

    // Highlight title
    if (all == 2)
	msg_puts_title(_("\n--- Terminal codes ---"));
    else if (opt_flags & OPT_GLOBAL)
	msg_puts_title(_("\n--- Global option values ---"));
    else if (opt_flags & OPT_LOCAL)
	msg_puts_title(_("\n--- Local option values ---"));
    else
	msg_puts_title(_("\n--- Options ---"));

    /*
     * Do the loop two times:
     * 1. display the short items
     * 2. display the long items (only strings and numbers)
     * When "opt_flags" has OPT_ONECOLUMN do everything in run 2.
     */
    for (run = 1; run <= 2 && !got_int; ++run)
    {
	/*
	 * collect the items in items[]
	 */
	item_count = 0;
	for (p = &options[0]; p->fullname != NULL; p++)
	{
	    // apply :filter /pat/
	    if (message_filtered((char_u *)p->fullname))
		continue;

	    varp = NULL;
	    isterm = istermoption(p);
	    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) != 0)
	    {
		if (p->indir != PV_NONE && !isterm)
		    varp = get_varp_scope(p, opt_flags);
	    }
	    else
		varp = get_varp(p);
	    if (varp != NULL
		    && ((all == 2 && isterm)
			|| (all == 1 && !isterm)
			|| (all == 0 && !optval_default(p, varp, p_cp))))
	    {
		if (opt_flags & OPT_ONECOLUMN)
		    len = Columns;
		else if (p->flags & P_BOOL)
		    len = 1;		// a toggle option fits always
		else
		{
		    option_value2string(p, opt_flags);
		    len = (int)STRLEN(p->fullname) + vim_strsize(NameBuff) + 1;
		}
		if ((len <= INC - GAP && run == 1) ||
						(len > INC - GAP && run == 2))
		    items[item_count++] = p;
	    }
	}

	/*
	 * display the items
	 */
	if (run == 1)
	{
	    cols = (Columns + GAP - 3) / INC;
	    if (cols == 0)
		cols = 1;
	    rows = (item_count + cols - 1) / cols;
	}
	else	// run == 2
	    rows = item_count;
	for (row = 0; row < rows && !got_int; ++row)
	{
	    msg_putchar('\n');			// go to next line
	    if (got_int)			// 'q' typed in more
		break;
	    col = 0;
	    for (i = row; i < item_count; i += rows)
	    {
		msg_col = col;			// make columns
		showoneopt(items[i], opt_flags);
		col += INC;
	    }
	    out_flush();
	    ui_breakcheck();
	}
    }
    vim_free(items);
}

/*
 * Return TRUE if option "p" has its default value.
 */
    static int
optval_default(struct vimoption *p, char_u *varp, int compatible)
{
    int		dvi;

    if (varp == NULL)
	return TRUE;	    // hidden option is always at default
    dvi = ((p->flags & P_VI_DEF) || compatible) ? VI_DEFAULT : VIM_DEFAULT;
    if (p->flags & P_NUM)
	return (*(long *)varp == (long)(long_i)p->def_val[dvi]);
    if (p->flags & P_BOOL)
			// the cast to long is required for Manx C, long_i is
			// needed for MSVC
	return (*(int *)varp == (int)(long)(long_i)p->def_val[dvi]);
    // P_STRING
    return (STRCMP(*(char_u **)varp, p->def_val[dvi]) == 0);
}

/*
 * showoneopt: show the value of one option
 * must not be called with a hidden option!
 */
    static void
showoneopt(
    struct vimoption	*p,
    int			opt_flags)	// OPT_LOCAL or OPT_GLOBAL
{
    char_u	*varp;
    int		save_silent = silent_mode;

    silent_mode = FALSE;
    info_message = TRUE;	// use mch_msg(), not mch_errmsg()

    varp = get_varp_scope(p, opt_flags);

    // for 'modified' we also need to check if 'ff' or 'fenc' changed.
    if ((p->flags & P_BOOL) && ((int *)varp == &curbuf->b_changed
					? !curbufIsChanged() : !*(int *)varp))
	msg_puts("no");
    else if ((p->flags & P_BOOL) && *(int *)varp < 0)
	msg_puts("--");
    else
	msg_puts("  ");
    msg_puts(p->fullname);
    if (!(p->flags & P_BOOL))
    {
	msg_putchar('=');
	// put value string in NameBuff
	option_value2string(p, opt_flags);
	msg_outtrans(NameBuff);
    }

    silent_mode = save_silent;
    info_message = FALSE;
}

/*
 * Write modified options as ":set" commands to a file.
 *
 * There are three values for "opt_flags":
 * OPT_GLOBAL:		   Write global option values and fresh values of
 *			   buffer-local options (used for start of a session
 *			   file).
 * OPT_GLOBAL + OPT_LOCAL: Idem, add fresh values of window-local options for
 *			   curwin (used for a vimrc file).
 * OPT_LOCAL:		   Write buffer-local option values for curbuf, fresh
 *			   and local values for window-local options of
 *			   curwin.  Local values are also written when at the
 *			   default value, because a modeline or autocommand
 *			   may have set them when doing ":edit file" and the
 *			   user has set them back at the default or fresh
 *			   value.
 *			   When "local_only" is TRUE, don't write fresh
 *			   values, only local values (for ":mkview").
 * (fresh value = value used for a new buffer or window for a local option).
 *
 * Return FAIL on error, OK otherwise.
 */
    int
makeset(FILE *fd, int opt_flags, int local_only)
{
    struct vimoption	*p;
    char_u		*varp;			// currently used value
    char_u		*varp_fresh;		// local value
    char_u		*varp_local = NULL;	// fresh value
    char		*cmd;
    int			round;
    int			pri;

    /*
     * The options that don't have a default (terminal name, columns, lines)
     * are never written.  Terminal options are also not written.
     * Do the loop over "options[]" twice: once for options with the
     * P_PRI_MKRC flag and once without.
     */
    for (pri = 1; pri >= 0; --pri)
    {
      for (p = &options[0]; !istermoption(p); p++)
	if (!(p->flags & P_NO_MKRC)
		&& !istermoption(p)
		&& ((pri == 1) == ((p->flags & P_PRI_MKRC) != 0)))
	{
	    // skip global option when only doing locals
	    if (p->indir == PV_NONE && !(opt_flags & OPT_GLOBAL))
		continue;

	    // Do not store options like 'bufhidden' and 'syntax' in a vimrc
	    // file, they are always buffer-specific.
	    if ((opt_flags & OPT_GLOBAL) && (p->flags & P_NOGLOB))
		continue;

	    // Global values are only written when not at the default value.
	    varp = get_varp_scope(p, opt_flags);
	    if ((opt_flags & OPT_GLOBAL) && optval_default(p, varp, p_cp))
		continue;

	    if ((opt_flags & OPT_SKIPRTP) && (p->var == (char_u *)&p_rtp
						 || p->var == (char_u *)&p_pp))
		continue;

	    round = 2;
	    if (p->indir != PV_NONE)
	    {
		if (p->var == VAR_WIN)
		{
		    // skip window-local option when only doing globals
		    if (!(opt_flags & OPT_LOCAL))
			continue;
		    // When fresh value of window-local option is not at the
		    // default, need to write it too.
		    if (!(opt_flags & OPT_GLOBAL) && !local_only)
		    {
			varp_fresh = get_varp_scope(p, OPT_GLOBAL);
			if (!optval_default(p, varp_fresh, p_cp))
			{
			    round = 1;
			    varp_local = varp;
			    varp = varp_fresh;
			}
		    }
		}
	    }

	    // Round 1: fresh value for window-local options.
	    // Round 2: other values
	    for ( ; round <= 2; varp = varp_local, ++round)
	    {
		if (round == 1 || (opt_flags & OPT_GLOBAL))
		    cmd = "set";
		else
		    cmd = "setlocal";

		if (p->flags & P_BOOL)
		{
		    if (put_setbool(fd, cmd, p->fullname, *(int *)varp) == FAIL)
			return FAIL;
		}
		else if (p->flags & P_NUM)
		{
		    if (put_setnum(fd, cmd, p->fullname, (long *)varp) == FAIL)
			return FAIL;
		}
		else    // P_STRING
		{
		    int		do_endif = FALSE;

		    // Don't set 'syntax' and 'filetype' again if the value is
		    // already right, avoids reloading the syntax file.
		    if (
#if defined(FEAT_SYN_HL)
			    p->indir == PV_SYN ||
#endif
			    p->indir == PV_FT)
		    {
			if (fprintf(fd, "if &%s != '%s'", p->fullname,
						       *(char_u **)(varp)) < 0
				|| put_eol(fd) < 0)
			    return FAIL;
			do_endif = TRUE;
		    }
		    if (put_setstring(fd, cmd, p->fullname, (char_u **)varp,
							     p->flags) == FAIL)
			return FAIL;
		    if (do_endif)
		    {
			if (put_line(fd, "endif") == FAIL)
			    return FAIL;
		    }
		}
	    }
	}
    }
    return OK;
}

#if defined(FEAT_FOLDING) || defined(PROTO)
/*
 * Generate set commands for the local fold options only.  Used when
 * 'sessionoptions' or 'viewoptions' contains "folds" but not "options".
 */
    int
makefoldset(FILE *fd)
{
    if (put_setstring(fd, "setlocal", "fdm", &curwin->w_p_fdm, 0) == FAIL
# ifdef FEAT_EVAL
	    || put_setstring(fd, "setlocal", "fde", &curwin->w_p_fde, 0)
								       == FAIL
# endif
	    || put_setstring(fd, "setlocal", "fmr", &curwin->w_p_fmr, 0)
								       == FAIL
	    || put_setstring(fd, "setlocal", "fdi", &curwin->w_p_fdi, 0)
								       == FAIL
	    || put_setnum(fd, "setlocal", "fdl", &curwin->w_p_fdl) == FAIL
	    || put_setnum(fd, "setlocal", "fml", &curwin->w_p_fml) == FAIL
	    || put_setnum(fd, "setlocal", "fdn", &curwin->w_p_fdn) == FAIL
	    || put_setbool(fd, "setlocal", "fen", curwin->w_p_fen) == FAIL
	    )
	return FAIL;

    return OK;
}
#endif

    static int
put_setstring(
    FILE	*fd,
    char	*cmd,
    char	*name,
    char_u	**valuep,
    long_u	flags)
{
    char_u	*s;
    char_u	*buf = NULL;
    char_u	*part = NULL;
    char_u	*p;

    if (fprintf(fd, "%s %s=", cmd, name) < 0)
	return FAIL;
    if (*valuep != NULL)
    {
	// Output 'pastetoggle' as key names.  For other
	// options some characters have to be escaped with
	// CTRL-V or backslash
	if (valuep == &p_pt)
	{
	    s = *valuep;
	    while (*s != NUL)
		if (put_escstr(fd, str2special(&s, FALSE), 2) == FAIL)
		    return FAIL;
	}
	// expand the option value, replace $HOME by ~
	else if ((flags & P_EXPAND) != 0)
	{
	    int  size = (int)STRLEN(*valuep) + 1;

	    // replace home directory in the whole option value into "buf"
	    buf = alloc(size);
	    if (buf == NULL)
		goto fail;
	    home_replace(NULL, *valuep, buf, size, FALSE);

	    // If the option value is longer than MAXPATHL, we need to append
	    // each comma separated part of the option separately, so that it
	    // can be expanded when read back.
	    if (size >= MAXPATHL && (flags & P_COMMA) != 0
					   && vim_strchr(*valuep, ',') != NULL)
	    {
		part = alloc(size);
		if (part == NULL)
		    goto fail;

		// write line break to clear the option, e.g. ':set rtp='
		if (put_eol(fd) == FAIL)
		    goto fail;

		p = buf;
		while (*p != NUL)
		{
		    // for each comma separated option part, append value to
		    // the option, :set rtp+=value
		    if (fprintf(fd, "%s %s+=", cmd, name) < 0)
			goto fail;
		    (void)copy_option_part(&p, part, size,  ",");
		    if (put_escstr(fd, part, 2) == FAIL || put_eol(fd) == FAIL)
			goto fail;
		}
		vim_free(buf);
		vim_free(part);
		return OK;
	    }
	    if (put_escstr(fd, buf, 2) == FAIL)
	    {
		vim_free(buf);
		return FAIL;
	    }
	    vim_free(buf);
	}
	else if (put_escstr(fd, *valuep, 2) == FAIL)
	    return FAIL;
    }
    if (put_eol(fd) < 0)
	return FAIL;
    return OK;
fail:
    vim_free(buf);
    vim_free(part);
    return FAIL;
}

    static int
put_setnum(
    FILE	*fd,
    char	*cmd,
    char	*name,
    long	*valuep)
{
    long	wc;

    if (fprintf(fd, "%s %s=", cmd, name) < 0)
	return FAIL;
    if (wc_use_keyname((char_u *)valuep, &wc))
    {
	// print 'wildchar' and 'wildcharm' as a key name
	if (fputs((char *)get_special_key_name((int)wc, 0), fd) < 0)
	    return FAIL;
    }
    else if (fprintf(fd, "%ld", *valuep) < 0)
	return FAIL;
    if (put_eol(fd) < 0)
	return FAIL;
    return OK;
}

    static int
put_setbool(
    FILE	*fd,
    char	*cmd,
    char	*name,
    int		value)
{
    if (value < 0)	// global/local option using global value
	return OK;
    if (fprintf(fd, "%s %s%s", cmd, value ? "" : "no", name) < 0
	    || put_eol(fd) < 0)
	return FAIL;
    return OK;
}

/*
 * Clear all the terminal options.
 * If the option has been allocated, free the memory.
 * Terminal options are never hidden or indirect.
 */
    void
clear_termoptions(void)
{
    /*
     * Reset a few things before clearing the old options. This may cause
     * outputting a few things that the terminal doesn't understand, but the
     * screen will be cleared later, so this is OK.
     */
    mch_setmouse(FALSE);	    // switch mouse off
#ifdef FEAT_TITLE
    mch_restore_title(SAVE_RESTORE_BOTH);    // restore window titles
#endif
#if defined(FEAT_XCLIPBOARD) && defined(FEAT_GUI)
    // When starting the GUI close the display opened for the clipboard.
    // After restoring the title, because that will need the display.
    if (gui.starting)
	clear_xterm_clip();
#endif
    stoptermcap();			// stop termcap mode

    free_termoptions();
}

    void
free_termoptions(void)
{
    struct vimoption   *p;

    for (p = options; p->fullname != NULL; p++)
	if (istermoption(p))
	{
	    if (p->flags & P_ALLOCED)
		free_string_option(*(char_u **)(p->var));
	    if (p->flags & P_DEF_ALLOCED)
		free_string_option(p->def_val[VI_DEFAULT]);
	    *(char_u **)(p->var) = empty_option;
	    p->def_val[VI_DEFAULT] = empty_option;
	    p->flags &= ~(P_ALLOCED|P_DEF_ALLOCED);
#ifdef FEAT_EVAL
	    // remember where the option was cleared
	    set_option_sctx_idx((int)(p - options), OPT_GLOBAL, current_sctx);
#endif
	}
    clear_termcodes();
}

/*
 * Free the string for one term option, if it was allocated.
 * Set the string to empty_option and clear allocated flag.
 * "var" points to the option value.
 */
    void
free_one_termoption(char_u *var)
{
    struct vimoption   *p;

    for (p = &options[0]; p->fullname != NULL; p++)
	if (p->var == var)
	{
	    if (p->flags & P_ALLOCED)
		free_string_option(*(char_u **)(p->var));
	    *(char_u **)(p->var) = empty_option;
	    p->flags &= ~P_ALLOCED;
	    break;
	}
}

/*
 * Set the terminal option defaults to the current value.
 * Used after setting the terminal name.
 */
    void
set_term_defaults(void)
{
    struct vimoption   *p;

    for (p = &options[0]; p->fullname != NULL; p++)
    {
	if (istermoption(p) && p->def_val[VI_DEFAULT] != *(char_u **)(p->var))
	{
	    if (p->flags & P_DEF_ALLOCED)
	    {
		free_string_option(p->def_val[VI_DEFAULT]);
		p->flags &= ~P_DEF_ALLOCED;
	    }
	    p->def_val[VI_DEFAULT] = *(char_u **)(p->var);
	    if (p->flags & P_ALLOCED)
	    {
		p->flags |= P_DEF_ALLOCED;
		p->flags &= ~P_ALLOCED;	 // don't free the value now
	    }
	}
    }
}

/*
 * return TRUE if 'p' starts with 't_'
 */
    static int
istermoption(struct vimoption *p)
{
    return (p->fullname[0] == 't' && p->fullname[1] == '_');
}

/*
 * Returns TRUE if the option at 'opt_idx' starts with 't_'
 */
    int
istermoption_idx(int opt_idx)
{
    return istermoption(&options[opt_idx]);
}

#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3) || defined(PROTO)
/*
 * Unset local option value, similar to ":set opt<".
 */
    void
unset_global_local_option(char_u *name, void *from)
{
    struct vimoption *p;
    int		opt_idx;
    buf_T	*buf = (buf_T *)from;

    opt_idx = findoption(name);
    if (opt_idx < 0)
	return;
    p = &(options[opt_idx]);

    switch ((int)p->indir)
    {
	// global option with local value: use local value if it's been set
	case PV_EP:
	    clear_string_option(&buf->b_p_ep);
	    break;
	case PV_KP:
	    clear_string_option(&buf->b_p_kp);
	    break;
	case PV_PATH:
	    clear_string_option(&buf->b_p_path);
	    break;
	case PV_AR:
	    buf->b_p_ar = -1;
	    break;
	case PV_BKC:
	    clear_string_option(&buf->b_p_bkc);
	    buf->b_bkc_flags = 0;
	    break;
	case PV_TAGS:
	    clear_string_option(&buf->b_p_tags);
	    break;
	case PV_TC:
	    clear_string_option(&buf->b_p_tc);
	    buf->b_tc_flags = 0;
	    break;
        case PV_SISO:
            curwin->w_p_siso = -1;
            break;
        case PV_SO:
            curwin->w_p_so = -1;
            break;
#ifdef FEAT_FIND_ID
	case PV_DEF:
	    clear_string_option(&buf->b_p_def);
	    break;
	case PV_INC:
	    clear_string_option(&buf->b_p_inc);
	    break;
#endif
	case PV_DICT:
	    clear_string_option(&buf->b_p_dict);
	    break;
	case PV_TSR:
	    clear_string_option(&buf->b_p_tsr);
	    break;
	case PV_FP:
	    clear_string_option(&buf->b_p_fp);
	    break;
#ifdef FEAT_QUICKFIX
	case PV_EFM:
	    clear_string_option(&buf->b_p_efm);
	    break;
	case PV_GP:
	    clear_string_option(&buf->b_p_gp);
	    break;
	case PV_MP:
	    clear_string_option(&buf->b_p_mp);
	    break;
#endif
#if defined(FEAT_BEVAL) && defined(FEAT_EVAL)
	case PV_BEXPR:
	    clear_string_option(&buf->b_p_bexpr);
	    break;
#endif
#if defined(FEAT_CRYPT)
	case PV_CM:
	    clear_string_option(&buf->b_p_cm);
	    break;
#endif
#ifdef FEAT_LINEBREAK
	case PV_SBR:
	    clear_string_option(&((win_T *)from)->w_p_sbr);
	    break;
#endif
#ifdef FEAT_STL_OPT
	case PV_STL:
	    clear_string_option(&((win_T *)from)->w_p_stl);
	    break;
#endif
	case PV_UL:
	    buf->b_p_ul = NO_LOCAL_UNDOLEVEL;
	    break;
#ifdef FEAT_LISP
	case PV_LW:
	    clear_string_option(&buf->b_p_lw);
	    break;
#endif
	case PV_MENC:
	    clear_string_option(&buf->b_p_menc);
	    break;
	case PV_LCS:
	    clear_string_option(&((win_T *)from)->w_p_lcs);
	    set_chars_option((win_T *)from, &((win_T *)from)->w_p_lcs);
	    redraw_later(NOT_VALID);
	    break;
	case PV_VE:
	    clear_string_option(&((win_T *)from)->w_p_ve);
	    ((win_T *)from)->w_ve_flags = 0;
	    break;
    }
}
#endif

/*
 * Get pointer to option variable, depending on local or global scope.
 */
    static char_u *
get_varp_scope(struct vimoption *p, int opt_flags)
{
    if ((opt_flags & OPT_GLOBAL) && p->indir != PV_NONE)
    {
	if (p->var == VAR_WIN)
	    return (char_u *)GLOBAL_WO(get_varp(p));
	return p->var;
    }
    if ((opt_flags & OPT_LOCAL) && ((int)p->indir & PV_BOTH))
    {
	switch ((int)p->indir)
	{
	    case PV_FP:   return (char_u *)&(curbuf->b_p_fp);
#ifdef FEAT_QUICKFIX
	    case PV_EFM:  return (char_u *)&(curbuf->b_p_efm);
	    case PV_GP:   return (char_u *)&(curbuf->b_p_gp);
	    case PV_MP:   return (char_u *)&(curbuf->b_p_mp);
#endif
	    case PV_EP:   return (char_u *)&(curbuf->b_p_ep);
	    case PV_KP:   return (char_u *)&(curbuf->b_p_kp);
	    case PV_PATH: return (char_u *)&(curbuf->b_p_path);
	    case PV_AR:   return (char_u *)&(curbuf->b_p_ar);
	    case PV_TAGS: return (char_u *)&(curbuf->b_p_tags);
	    case PV_TC:   return (char_u *)&(curbuf->b_p_tc);
            case PV_SISO: return (char_u *)&(curwin->w_p_siso);
            case PV_SO:   return (char_u *)&(curwin->w_p_so);
#ifdef FEAT_FIND_ID
	    case PV_DEF:  return (char_u *)&(curbuf->b_p_def);
	    case PV_INC:  return (char_u *)&(curbuf->b_p_inc);
#endif
	    case PV_DICT: return (char_u *)&(curbuf->b_p_dict);
	    case PV_TSR:  return (char_u *)&(curbuf->b_p_tsr);
#if defined(FEAT_BEVAL) && defined(FEAT_EVAL)
	    case PV_BEXPR: return (char_u *)&(curbuf->b_p_bexpr);
#endif
#if defined(FEAT_CRYPT)
	    case PV_CM:	  return (char_u *)&(curbuf->b_p_cm);
#endif
#ifdef FEAT_LINEBREAK
	    case PV_SBR:  return (char_u *)&(curwin->w_p_sbr);
#endif
#ifdef FEAT_STL_OPT
	    case PV_STL:  return (char_u *)&(curwin->w_p_stl);
#endif
	    case PV_UL:   return (char_u *)&(curbuf->b_p_ul);
#ifdef FEAT_LISP
	    case PV_LW:   return (char_u *)&(curbuf->b_p_lw);
#endif
	    case PV_BKC:  return (char_u *)&(curbuf->b_p_bkc);
	    case PV_MENC: return (char_u *)&(curbuf->b_p_menc);
	    case PV_LCS:  return (char_u *)&(curwin->w_p_lcs);
	    case PV_VE:	  return (char_u *)&(curwin->w_p_ve);

	}
	return NULL; // "cannot happen"
    }
    return get_varp(p);
}

/*
 * Get pointer to option variable at 'opt_idx', depending on local or global
 * scope.
 */
    char_u *
get_option_varp_scope(int opt_idx, int opt_flags)
{
    return get_varp_scope(&(options[opt_idx]), opt_flags);
}

/*
 * Get pointer to option variable.
 */
    static char_u *
get_varp(struct vimoption *p)
{
    // hidden option, always return NULL
    if (p->var == NULL)
	return NULL;

    switch ((int)p->indir)
    {
	case PV_NONE:	return p->var;

	// global option with local value: use local value if it's been set
	case PV_EP:	return *curbuf->b_p_ep != NUL
				    ? (char_u *)&curbuf->b_p_ep : p->var;
	case PV_KP:	return *curbuf->b_p_kp != NUL
				    ? (char_u *)&curbuf->b_p_kp : p->var;
	case PV_PATH:	return *curbuf->b_p_path != NUL
				    ? (char_u *)&(curbuf->b_p_path) : p->var;
	case PV_AR:	return curbuf->b_p_ar >= 0
				    ? (char_u *)&(curbuf->b_p_ar) : p->var;
	case PV_TAGS:	return *curbuf->b_p_tags != NUL
				    ? (char_u *)&(curbuf->b_p_tags) : p->var;
	case PV_TC:	return *curbuf->b_p_tc != NUL
				    ? (char_u *)&(curbuf->b_p_tc) : p->var;
	case PV_BKC:	return *curbuf->b_p_bkc != NUL
				    ? (char_u *)&(curbuf->b_p_bkc) : p->var;
	case PV_SISO:	return curwin->w_p_siso >= 0
				    ? (char_u *)&(curwin->w_p_siso) : p->var;
	case PV_SO:	return curwin->w_p_so >= 0
				    ? (char_u *)&(curwin->w_p_so) : p->var;
#ifdef FEAT_FIND_ID
	case PV_DEF:	return *curbuf->b_p_def != NUL
				    ? (char_u *)&(curbuf->b_p_def) : p->var;
	case PV_INC:	return *curbuf->b_p_inc != NUL
				    ? (char_u *)&(curbuf->b_p_inc) : p->var;
#endif
	case PV_DICT:	return *curbuf->b_p_dict != NUL
				    ? (char_u *)&(curbuf->b_p_dict) : p->var;
	case PV_TSR:	return *curbuf->b_p_tsr != NUL
				    ? (char_u *)&(curbuf->b_p_tsr) : p->var;
	case PV_FP:	return *curbuf->b_p_fp != NUL
				    ? (char_u *)&(curbuf->b_p_fp) : p->var;
#ifdef FEAT_QUICKFIX
	case PV_EFM:	return *curbuf->b_p_efm != NUL
				    ? (char_u *)&(curbuf->b_p_efm) : p->var;
	case PV_GP:	return *curbuf->b_p_gp != NUL
				    ? (char_u *)&(curbuf->b_p_gp) : p->var;
	case PV_MP:	return *curbuf->b_p_mp != NUL
				    ? (char_u *)&(curbuf->b_p_mp) : p->var;
#endif
#if defined(FEAT_BEVAL) && defined(FEAT_EVAL)
	case PV_BEXPR:	return *curbuf->b_p_bexpr != NUL
				    ? (char_u *)&(curbuf->b_p_bexpr) : p->var;
#endif
#if defined(FEAT_CRYPT)
	case PV_CM:	return *curbuf->b_p_cm != NUL
				    ? (char_u *)&(curbuf->b_p_cm) : p->var;
#endif
#ifdef FEAT_LINEBREAK
	case PV_SBR:	return *curwin->w_p_sbr != NUL
				    ? (char_u *)&(curwin->w_p_sbr) : p->var;
#endif
#ifdef FEAT_STL_OPT
	case PV_STL:	return *curwin->w_p_stl != NUL
				    ? (char_u *)&(curwin->w_p_stl) : p->var;
#endif
	case PV_UL:	return curbuf->b_p_ul != NO_LOCAL_UNDOLEVEL
				    ? (char_u *)&(curbuf->b_p_ul) : p->var;
#ifdef FEAT_LISP
	case PV_LW:	return *curbuf->b_p_lw != NUL
				    ? (char_u *)&(curbuf->b_p_lw) : p->var;
#endif
	case PV_MENC:	return *curbuf->b_p_menc != NUL
				    ? (char_u *)&(curbuf->b_p_menc) : p->var;
#ifdef FEAT_ARABIC
	case PV_ARAB:	return (char_u *)&(curwin->w_p_arab);
#endif
	case PV_LIST:	return (char_u *)&(curwin->w_p_list);
	case PV_LCS:	return *curwin->w_p_lcs != NUL
				    ? (char_u *)&(curwin->w_p_lcs) : p->var;
	case PV_VE:	return *curwin->w_p_ve != NUL
				    ? (char_u *)&(curwin->w_p_ve) : p->var;
#ifdef FEAT_SPELL
	case PV_SPELL:	return (char_u *)&(curwin->w_p_spell);
#endif
#ifdef FEAT_SYN_HL
	case PV_CUC:	return (char_u *)&(curwin->w_p_cuc);
	case PV_CUL:	return (char_u *)&(curwin->w_p_cul);
	case PV_CULOPT:	return (char_u *)&(curwin->w_p_culopt);
	case PV_CC:	return (char_u *)&(curwin->w_p_cc);
#endif
#ifdef FEAT_DIFF
	case PV_DIFF:	return (char_u *)&(curwin->w_p_diff);
#endif
#ifdef FEAT_FOLDING
	case PV_FDC:	return (char_u *)&(curwin->w_p_fdc);
	case PV_FEN:	return (char_u *)&(curwin->w_p_fen);
	case PV_FDI:	return (char_u *)&(curwin->w_p_fdi);
	case PV_FDL:	return (char_u *)&(curwin->w_p_fdl);
	case PV_FDM:	return (char_u *)&(curwin->w_p_fdm);
	case PV_FML:	return (char_u *)&(curwin->w_p_fml);
	case PV_FDN:	return (char_u *)&(curwin->w_p_fdn);
# ifdef FEAT_EVAL
	case PV_FDE:	return (char_u *)&(curwin->w_p_fde);
	case PV_FDT:	return (char_u *)&(curwin->w_p_fdt);
# endif
	case PV_FMR:	return (char_u *)&(curwin->w_p_fmr);
#endif
	case PV_NU:	return (char_u *)&(curwin->w_p_nu);
	case PV_RNU:	return (char_u *)&(curwin->w_p_rnu);
#ifdef FEAT_LINEBREAK
	case PV_NUW:	return (char_u *)&(curwin->w_p_nuw);
#endif
	case PV_WFH:	return (char_u *)&(curwin->w_p_wfh);
	case PV_WFW:	return (char_u *)&(curwin->w_p_wfw);
#if defined(FEAT_QUICKFIX)
	case PV_PVW:	return (char_u *)&(curwin->w_p_pvw);
#endif
#ifdef FEAT_RIGHTLEFT
	case PV_RL:	return (char_u *)&(curwin->w_p_rl);
	case PV_RLC:	return (char_u *)&(curwin->w_p_rlc);
#endif
	case PV_SCROLL:	return (char_u *)&(curwin->w_p_scr);
	case PV_WRAP:	return (char_u *)&(curwin->w_p_wrap);
#ifdef FEAT_LINEBREAK
	case PV_LBR:	return (char_u *)&(curwin->w_p_lbr);
	case PV_BRI:	return (char_u *)&(curwin->w_p_bri);
	case PV_BRIOPT: return (char_u *)&(curwin->w_p_briopt);
#endif
	case PV_WCR:	return (char_u *)&(curwin->w_p_wcr);
	case PV_SCBIND: return (char_u *)&(curwin->w_p_scb);
	case PV_CRBIND: return (char_u *)&(curwin->w_p_crb);
#ifdef FEAT_CONCEAL
	case PV_COCU:   return (char_u *)&(curwin->w_p_cocu);
	case PV_COLE:   return (char_u *)&(curwin->w_p_cole);
#endif
#ifdef FEAT_TERMINAL
	case PV_TWK:    return (char_u *)&(curwin->w_p_twk);
	case PV_TWS:    return (char_u *)&(curwin->w_p_tws);
	case PV_TWSL:	return (char_u *)&(curbuf->b_p_twsl);
#endif

	case PV_AI:	return (char_u *)&(curbuf->b_p_ai);
	case PV_BIN:	return (char_u *)&(curbuf->b_p_bin);
	case PV_BOMB:	return (char_u *)&(curbuf->b_p_bomb);
	case PV_BH:	return (char_u *)&(curbuf->b_p_bh);
	case PV_BT:	return (char_u *)&(curbuf->b_p_bt);
	case PV_BL:	return (char_u *)&(curbuf->b_p_bl);
	case PV_CI:	return (char_u *)&(curbuf->b_p_ci);
#ifdef FEAT_CINDENT
	case PV_CIN:	return (char_u *)&(curbuf->b_p_cin);
	case PV_CINK:	return (char_u *)&(curbuf->b_p_cink);
	case PV_CINO:	return (char_u *)&(curbuf->b_p_cino);
#endif
#if defined(FEAT_SMARTINDENT) || defined(FEAT_CINDENT)
	case PV_CINW:	return (char_u *)&(curbuf->b_p_cinw);
#endif
	case PV_COM:	return (char_u *)&(curbuf->b_p_com);
#ifdef FEAT_FOLDING
	case PV_CMS:	return (char_u *)&(curbuf->b_p_cms);
#endif
	case PV_CPT:	return (char_u *)&(curbuf->b_p_cpt);
#ifdef BACKSLASH_IN_FILENAME
	case PV_CSL:	return (char_u *)&(curbuf->b_p_csl);
#endif
#ifdef FEAT_COMPL_FUNC
	case PV_CFU:	return (char_u *)&(curbuf->b_p_cfu);
	case PV_OFU:	return (char_u *)&(curbuf->b_p_ofu);
#endif
#ifdef FEAT_EVAL
	case PV_TFU:	return (char_u *)&(curbuf->b_p_tfu);
#endif
	case PV_EOL:	return (char_u *)&(curbuf->b_p_eol);
	case PV_FIXEOL:	return (char_u *)&(curbuf->b_p_fixeol);
	case PV_ET:	return (char_u *)&(curbuf->b_p_et);
	case PV_FENC:	return (char_u *)&(curbuf->b_p_fenc);
	case PV_FF:	return (char_u *)&(curbuf->b_p_ff);
	case PV_FT:	return (char_u *)&(curbuf->b_p_ft);
	case PV_FO:	return (char_u *)&(curbuf->b_p_fo);
	case PV_FLP:	return (char_u *)&(curbuf->b_p_flp);
	case PV_IMI:	return (char_u *)&(curbuf->b_p_iminsert);
	case PV_IMS:	return (char_u *)&(curbuf->b_p_imsearch);
	case PV_INF:	return (char_u *)&(curbuf->b_p_inf);
	case PV_ISK:	return (char_u *)&(curbuf->b_p_isk);
#ifdef FEAT_FIND_ID
# ifdef FEAT_EVAL
	case PV_INEX:	return (char_u *)&(curbuf->b_p_inex);
# endif
#endif
#if defined(FEAT_CINDENT) && defined(FEAT_EVAL)
	case PV_INDE:	return (char_u *)&(curbuf->b_p_inde);
	case PV_INDK:	return (char_u *)&(curbuf->b_p_indk);
#endif
#ifdef FEAT_EVAL
	case PV_FEX:	return (char_u *)&(curbuf->b_p_fex);
#endif
#ifdef FEAT_CRYPT
	case PV_KEY:	return (char_u *)&(curbuf->b_p_key);
#endif
#ifdef FEAT_LISP
	case PV_LISP:	return (char_u *)&(curbuf->b_p_lisp);
#endif
	case PV_ML:	return (char_u *)&(curbuf->b_p_ml);
	case PV_MPS:	return (char_u *)&(curbuf->b_p_mps);
	case PV_MA:	return (char_u *)&(curbuf->b_p_ma);
	case PV_MOD:	return (char_u *)&(curbuf->b_changed);
	case PV_NF:	return (char_u *)&(curbuf->b_p_nf);
	case PV_PI:	return (char_u *)&(curbuf->b_p_pi);
#ifdef FEAT_TEXTOBJ
	case PV_QE:	return (char_u *)&(curbuf->b_p_qe);
#endif
	case PV_RO:	return (char_u *)&(curbuf->b_p_ro);
#ifdef FEAT_SMARTINDENT
	case PV_SI:	return (char_u *)&(curbuf->b_p_si);
#endif
	case PV_SN:	return (char_u *)&(curbuf->b_p_sn);
	case PV_STS:	return (char_u *)&(curbuf->b_p_sts);
#ifdef FEAT_SEARCHPATH
	case PV_SUA:	return (char_u *)&(curbuf->b_p_sua);
#endif
	case PV_SWF:	return (char_u *)&(curbuf->b_p_swf);
#ifdef FEAT_SYN_HL
	case PV_SMC:	return (char_u *)&(curbuf->b_p_smc);
	case PV_SYN:	return (char_u *)&(curbuf->b_p_syn);
#endif
#ifdef FEAT_SPELL
	case PV_SPC:	return (char_u *)&(curwin->w_s->b_p_spc);
	case PV_SPF:	return (char_u *)&(curwin->w_s->b_p_spf);
	case PV_SPL:	return (char_u *)&(curwin->w_s->b_p_spl);
	case PV_SPO:	return (char_u *)&(curwin->w_s->b_p_spo);
#endif
	case PV_SW:	return (char_u *)&(curbuf->b_p_sw);
	case PV_TS:	return (char_u *)&(curbuf->b_p_ts);
	case PV_TW:	return (char_u *)&(curbuf->b_p_tw);
	case PV_TX:	return (char_u *)&(curbuf->b_p_tx);
#ifdef FEAT_PERSISTENT_UNDO
	case PV_UDF:	return (char_u *)&(curbuf->b_p_udf);
#endif
	case PV_WM:	return (char_u *)&(curbuf->b_p_wm);
#ifdef FEAT_KEYMAP
	case PV_KMAP:	return (char_u *)&(curbuf->b_p_keymap);
#endif
#ifdef FEAT_SIGNS
	case PV_SCL:	return (char_u *)&(curwin->w_p_scl);
#endif
#ifdef FEAT_VARTABS
	case PV_VSTS:	return (char_u *)&(curbuf->b_p_vsts);
	case PV_VTS:	return (char_u *)&(curbuf->b_p_vts);
#endif
	default:	iemsg(_("E356: get_varp ERROR"));
    }
    // always return a valid pointer to avoid a crash!
    return (char_u *)&(curbuf->b_p_wm);
}

/*
 * Return a pointer to the variable for option at 'opt_idx'
 */
    char_u *
get_option_var(int opt_idx)
{
    return options[opt_idx].var;
}

/*
 * Return the full name of the option at 'opt_idx'
 */
    char_u *
get_option_fullname(int opt_idx)
{
    return (char_u *)options[opt_idx].fullname;
}

/*
 * Get the value of 'equalprg', either the buffer-local one or the global one.
 */
    char_u *
get_equalprg(void)
{
    if (*curbuf->b_p_ep == NUL)
	return p_ep;
    return curbuf->b_p_ep;
}

/*
 * Copy options from one window to another.
 * Used when splitting a window.
 */
    void
win_copy_options(win_T *wp_from, win_T *wp_to)
{
    copy_winopt(&wp_from->w_onebuf_opt, &wp_to->w_onebuf_opt);
    copy_winopt(&wp_from->w_allbuf_opt, &wp_to->w_allbuf_opt);
    after_copy_winopt(wp_to);
}

/*
 * After copying window options: update variables depending on options.
 */
    void
after_copy_winopt(win_T *wp UNUSED)
{
#ifdef FEAT_LINEBREAK
    briopt_check(wp);
#endif
#ifdef FEAT_SYN_HL
    fill_culopt_flags(NULL, wp);
    check_colorcolumn(wp);
#endif
    set_chars_option(wp, &wp->w_p_lcs);
}

/*
 * Copy the options from one winopt_T to another.
 * Doesn't free the old option values in "to", use clear_winopt() for that.
 * The 'scroll' option is not copied, because it depends on the window height.
 * The 'previewwindow' option is reset, there can be only one preview window.
 */
    void
copy_winopt(winopt_T *from, winopt_T *to)
{
#ifdef FEAT_ARABIC
    to->wo_arab = from->wo_arab;
#endif
    to->wo_list = from->wo_list;
    to->wo_lcs = vim_strsave(from->wo_lcs);
    to->wo_nu = from->wo_nu;
    to->wo_rnu = from->wo_rnu;
    to->wo_ve = vim_strsave(from->wo_ve);
    to->wo_ve_flags = from->wo_ve_flags;
#ifdef FEAT_LINEBREAK
    to->wo_nuw = from->wo_nuw;
#endif
#ifdef FEAT_RIGHTLEFT
    to->wo_rl  = from->wo_rl;
    to->wo_rlc = vim_strsave(from->wo_rlc);
#endif
#ifdef FEAT_LINEBREAK
    to->wo_sbr = vim_strsave(from->wo_sbr);
#endif
#ifdef FEAT_STL_OPT
    to->wo_stl = vim_strsave(from->wo_stl);
#endif
    to->wo_wrap = from->wo_wrap;
#ifdef FEAT_DIFF
    to->wo_wrap_save = from->wo_wrap_save;
#endif
#ifdef FEAT_LINEBREAK
    to->wo_lbr = from->wo_lbr;
    to->wo_bri = from->wo_bri;
    to->wo_briopt = vim_strsave(from->wo_briopt);
#endif
    to->wo_wcr = vim_strsave(from->wo_wcr);
    to->wo_scb = from->wo_scb;
    to->wo_scb_save = from->wo_scb_save;
    to->wo_crb = from->wo_crb;
    to->wo_crb_save = from->wo_crb_save;
#ifdef FEAT_SPELL
    to->wo_spell = from->wo_spell;
#endif
#ifdef FEAT_SYN_HL
    to->wo_cuc = from->wo_cuc;
    to->wo_cul = from->wo_cul;
    to->wo_culopt = vim_strsave(from->wo_culopt);
    to->wo_cc = vim_strsave(from->wo_cc);
#endif
#ifdef FEAT_DIFF
    to->wo_diff = from->wo_diff;
    to->wo_diff_saved = from->wo_diff_saved;
#endif
#ifdef FEAT_CONCEAL
    to->wo_cocu = vim_strsave(from->wo_cocu);
    to->wo_cole = from->wo_cole;
#endif
#ifdef FEAT_TERMINAL
    to->wo_twk = vim_strsave(from->wo_twk);
    to->wo_tws = vim_strsave(from->wo_tws);
#endif
#ifdef FEAT_FOLDING
    to->wo_fdc = from->wo_fdc;
    to->wo_fdc_save = from->wo_fdc_save;
    to->wo_fen = from->wo_fen;
    to->wo_fen_save = from->wo_fen_save;
    to->wo_fdi = vim_strsave(from->wo_fdi);
    to->wo_fml = from->wo_fml;
    to->wo_fdl = from->wo_fdl;
    to->wo_fdl_save = from->wo_fdl_save;
    to->wo_fdm = vim_strsave(from->wo_fdm);
    to->wo_fdm_save = from->wo_diff_saved
			      ? vim_strsave(from->wo_fdm_save) : empty_option;
    to->wo_fdn = from->wo_fdn;
# ifdef FEAT_EVAL
    to->wo_fde = vim_strsave(from->wo_fde);
    to->wo_fdt = vim_strsave(from->wo_fdt);
# endif
    to->wo_fmr = vim_strsave(from->wo_fmr);
#endif
#ifdef FEAT_SIGNS
    to->wo_scl = vim_strsave(from->wo_scl);
#endif

#ifdef FEAT_EVAL
    // Copy the script context so that we know where the value was last set.
    mch_memmove(to->wo_script_ctx, from->wo_script_ctx,
						    sizeof(to->wo_script_ctx));
#endif
    check_winopt(to);		// don't want NULL pointers
}

/*
 * Check string options in a window for a NULL value.
 */
    static void
check_win_options(win_T *win)
{
    check_winopt(&win->w_onebuf_opt);
    check_winopt(&win->w_allbuf_opt);
}

/*
 * Check for NULL pointers in a winopt_T and replace them with empty_option.
 */
    static void
check_winopt(winopt_T *wop UNUSED)
{
#ifdef FEAT_FOLDING
    check_string_option(&wop->wo_fdi);
    check_string_option(&wop->wo_fdm);
    check_string_option(&wop->wo_fdm_save);
# ifdef FEAT_EVAL
    check_string_option(&wop->wo_fde);
    check_string_option(&wop->wo_fdt);
# endif
    check_string_option(&wop->wo_fmr);
#endif
#ifdef FEAT_SIGNS
    check_string_option(&wop->wo_scl);
#endif
#ifdef FEAT_RIGHTLEFT
    check_string_option(&wop->wo_rlc);
#endif
#ifdef FEAT_LINEBREAK
    check_string_option(&wop->wo_sbr);
#endif
#ifdef FEAT_STL_OPT
    check_string_option(&wop->wo_stl);
#endif
#ifdef FEAT_SYN_HL
    check_string_option(&wop->wo_culopt);
    check_string_option(&wop->wo_cc);
#endif
#ifdef FEAT_CONCEAL
    check_string_option(&wop->wo_cocu);
#endif
#ifdef FEAT_TERMINAL
    check_string_option(&wop->wo_twk);
    check_string_option(&wop->wo_tws);
#endif
#ifdef FEAT_LINEBREAK
    check_string_option(&wop->wo_briopt);
#endif
    check_string_option(&wop->wo_wcr);
    check_string_option(&wop->wo_lcs);
    check_string_option(&wop->wo_ve);
}

/*
 * Free the allocated memory inside a winopt_T.
 */
    void
clear_winopt(winopt_T *wop UNUSED)
{
#ifdef FEAT_FOLDING
    clear_string_option(&wop->wo_fdi);
    clear_string_option(&wop->wo_fdm);
    clear_string_option(&wop->wo_fdm_save);
# ifdef FEAT_EVAL
    clear_string_option(&wop->wo_fde);
    clear_string_option(&wop->wo_fdt);
# endif
    clear_string_option(&wop->wo_fmr);
#endif
#ifdef FEAT_SIGNS
    clear_string_option(&wop->wo_scl);
#endif
#ifdef FEAT_LINEBREAK
    clear_string_option(&wop->wo_briopt);
#endif
    clear_string_option(&wop->wo_wcr);
#ifdef FEAT_RIGHTLEFT
    clear_string_option(&wop->wo_rlc);
#endif
#ifdef FEAT_LINEBREAK
    clear_string_option(&wop->wo_sbr);
#endif
#ifdef FEAT_STL_OPT
    clear_string_option(&wop->wo_stl);
#endif
#ifdef FEAT_SYN_HL
    clear_string_option(&wop->wo_culopt);
    clear_string_option(&wop->wo_cc);
#endif
#ifdef FEAT_CONCEAL
    clear_string_option(&wop->wo_cocu);
#endif
#ifdef FEAT_TERMINAL
    clear_string_option(&wop->wo_twk);
    clear_string_option(&wop->wo_tws);
#endif
    clear_string_option(&wop->wo_lcs);
    clear_string_option(&wop->wo_ve);
}

#ifdef FEAT_EVAL
// Index into the options table for a buffer-local option enum.
static int buf_opt_idx[BV_COUNT];
# define COPY_OPT_SCTX(buf, bv) buf->b_p_script_ctx[bv] = options[buf_opt_idx[bv]].script_ctx

/*
 * Initialize buf_opt_idx[] if not done already.
 */
    static void
init_buf_opt_idx(void)
{
    static int did_init_buf_opt_idx = FALSE;
    int i;

    if (did_init_buf_opt_idx)
	return;
    did_init_buf_opt_idx = TRUE;
    for (i = 0; !istermoption_idx(i); i++)
	if (options[i].indir & PV_BUF)
	    buf_opt_idx[options[i].indir & PV_MASK] = i;
}
#else
# define COPY_OPT_SCTX(buf, bv)
#endif

/*
 * Copy global option values to local options for one buffer.
 * Used when creating a new buffer and sometimes when entering a buffer.
 * flags:
 * BCO_ENTER	We will enter the buffer "buf".
 * BCO_ALWAYS	Always copy the options, but only set b_p_initialized when
 *		appropriate.
 * BCO_NOHELP	Don't copy the values to a help buffer.
 */
    void
buf_copy_options(buf_T *buf, int flags)
{
    int		should_copy = TRUE;
    char_u	*save_p_isk = NULL;	    // init for GCC
    int		dont_do_help;
    int		did_isk = FALSE;

    /*
     * Skip this when the option defaults have not been set yet.  Happens when
     * main() allocates the first buffer.
     */
    if (p_cpo != NULL)
    {
	/*
	 * Always copy when entering and 'cpo' contains 'S'.
	 * Don't copy when already initialized.
	 * Don't copy when 'cpo' contains 's' and not entering.
	 * 'S'	BCO_ENTER  initialized	's'  should_copy
	 * yes	  yes	       X	 X	TRUE
	 * yes	  no	      yes	 X	FALSE
	 * no	   X	      yes	 X	FALSE
	 *  X	  no	      no	yes	FALSE
	 *  X	  no	      no	no	TRUE
	 * no	  yes	      no	 X	TRUE
	 */
	if ((vim_strchr(p_cpo, CPO_BUFOPTGLOB) == NULL || !(flags & BCO_ENTER))
		&& (buf->b_p_initialized
		    || (!(flags & BCO_ENTER)
			&& vim_strchr(p_cpo, CPO_BUFOPT) != NULL)))
	    should_copy = FALSE;

	if (should_copy || (flags & BCO_ALWAYS))
	{
#ifdef FEAT_EVAL
	    CLEAR_FIELD(buf->b_p_script_ctx);
	    init_buf_opt_idx();
#endif
	    // Don't copy the options specific to a help buffer when
	    // BCO_NOHELP is given or the options were initialized already
	    // (jumping back to a help file with CTRL-T or CTRL-O)
	    dont_do_help = ((flags & BCO_NOHELP) && buf->b_help)
						       || buf->b_p_initialized;
	    if (dont_do_help)		// don't free b_p_isk
	    {
		save_p_isk = buf->b_p_isk;
		buf->b_p_isk = NULL;
	    }
	    /*
	     * Always free the allocated strings.  If not already initialized,
	     * reset 'readonly' and copy 'fileformat'.
	     */
	    if (!buf->b_p_initialized)
	    {
		free_buf_options(buf, TRUE);
		buf->b_p_ro = FALSE;		// don't copy readonly
		buf->b_p_tx = p_tx;
		buf->b_p_fenc = vim_strsave(p_fenc);
		switch (*p_ffs)
		{
		    case 'm':
			buf->b_p_ff = vim_strsave((char_u *)FF_MAC); break;
		    case 'd':
			buf->b_p_ff = vim_strsave((char_u *)FF_DOS); break;
		    case 'u':
			buf->b_p_ff = vim_strsave((char_u *)FF_UNIX); break;
		    default:
			buf->b_p_ff = vim_strsave(p_ff);
		}
		if (buf->b_p_ff != NULL)
		    buf->b_start_ffc = *buf->b_p_ff;
		buf->b_p_bh = empty_option;
		buf->b_p_bt = empty_option;
	    }
	    else
		free_buf_options(buf, FALSE);

	    buf->b_p_ai = p_ai;
	    COPY_OPT_SCTX(buf, BV_AI);
	    buf->b_p_ai_nopaste = p_ai_nopaste;
	    buf->b_p_sw = p_sw;
	    COPY_OPT_SCTX(buf, BV_SW);
	    buf->b_p_tw = p_tw;
	    COPY_OPT_SCTX(buf, BV_TW);
	    buf->b_p_tw_nopaste = p_tw_nopaste;
	    buf->b_p_tw_nobin = p_tw_nobin;
	    buf->b_p_wm = p_wm;
	    COPY_OPT_SCTX(buf, BV_WM);
	    buf->b_p_wm_nopaste = p_wm_nopaste;
	    buf->b_p_wm_nobin = p_wm_nobin;
	    buf->b_p_bin = p_bin;
	    COPY_OPT_SCTX(buf, BV_BIN);
	    buf->b_p_bomb = p_bomb;
	    COPY_OPT_SCTX(buf, BV_BOMB);
	    buf->b_p_fixeol = p_fixeol;
	    COPY_OPT_SCTX(buf, BV_FIXEOL);
	    buf->b_p_et = p_et;
	    COPY_OPT_SCTX(buf, BV_ET);
	    buf->b_p_et_nobin = p_et_nobin;
	    buf->b_p_et_nopaste = p_et_nopaste;
	    buf->b_p_ml = p_ml;
	    COPY_OPT_SCTX(buf, BV_ML);
	    buf->b_p_ml_nobin = p_ml_nobin;
	    buf->b_p_inf = p_inf;
	    COPY_OPT_SCTX(buf, BV_INF);
	    if (cmdmod.cmod_flags & CMOD_NOSWAPFILE)
		buf->b_p_swf = FALSE;
	    else
	    {
		buf->b_p_swf = p_swf;
		COPY_OPT_SCTX(buf, BV_INF);
	    }
	    buf->b_p_cpt = vim_strsave(p_cpt);
	    COPY_OPT_SCTX(buf, BV_CPT);
#ifdef BACKSLASH_IN_FILENAME
	    buf->b_p_csl = vim_strsave(p_csl);
	    COPY_OPT_SCTX(buf, BV_CSL);
#endif
#ifdef FEAT_COMPL_FUNC
	    buf->b_p_cfu = vim_strsave(p_cfu);
	    COPY_OPT_SCTX(buf, BV_CFU);
	    buf->b_p_ofu = vim_strsave(p_ofu);
	    COPY_OPT_SCTX(buf, BV_OFU);
#endif
#ifdef FEAT_EVAL
	    buf->b_p_tfu = vim_strsave(p_tfu);
	    COPY_OPT_SCTX(buf, BV_TFU);
#endif
	    buf->b_p_sts = p_sts;
	    COPY_OPT_SCTX(buf, BV_STS);
	    buf->b_p_sts_nopaste = p_sts_nopaste;
#ifdef FEAT_VARTABS
	    buf->b_p_vsts = vim_strsave(p_vsts);
	    COPY_OPT_SCTX(buf, BV_VSTS);
	    if (p_vsts && p_vsts != empty_option)
		(void)tabstop_set(p_vsts, &buf->b_p_vsts_array);
	    else
		buf->b_p_vsts_array = 0;
	    buf->b_p_vsts_nopaste = p_vsts_nopaste
				 ? vim_strsave(p_vsts_nopaste) : NULL;
#endif
	    buf->b_p_sn = p_sn;
	    COPY_OPT_SCTX(buf, BV_SN);
	    buf->b_p_com = vim_strsave(p_com);
	    COPY_OPT_SCTX(buf, BV_COM);
#ifdef FEAT_FOLDING
	    buf->b_p_cms = vim_strsave(p_cms);
	    COPY_OPT_SCTX(buf, BV_CMS);
#endif
	    buf->b_p_fo = vim_strsave(p_fo);
	    COPY_OPT_SCTX(buf, BV_FO);
	    buf->b_p_flp = vim_strsave(p_flp);
	    COPY_OPT_SCTX(buf, BV_FLP);
	    // NOTE: Valgrind may report a bogus memory leak for 'nrformats'
	    // when it is set to 8 bytes in defaults.vim.
	    buf->b_p_nf = vim_strsave(p_nf);
	    COPY_OPT_SCTX(buf, BV_NF);
	    buf->b_p_mps = vim_strsave(p_mps);
	    COPY_OPT_SCTX(buf, BV_MPS);
#ifdef FEAT_SMARTINDENT
	    buf->b_p_si = p_si;
	    COPY_OPT_SCTX(buf, BV_SI);
#endif
	    buf->b_p_ci = p_ci;
	    COPY_OPT_SCTX(buf, BV_CI);
#ifdef FEAT_CINDENT
	    buf->b_p_cin = p_cin;
	    COPY_OPT_SCTX(buf, BV_CIN);
	    buf->b_p_cink = vim_strsave(p_cink);
	    COPY_OPT_SCTX(buf, BV_CINK);
	    buf->b_p_cino = vim_strsave(p_cino);
	    COPY_OPT_SCTX(buf, BV_CINO);
#endif
	    // Don't copy 'filetype', it must be detected
	    buf->b_p_ft = empty_option;
	    buf->b_p_pi = p_pi;
	    COPY_OPT_SCTX(buf, BV_PI);
#if defined(FEAT_SMARTINDENT) || defined(FEAT_CINDENT)
	    buf->b_p_cinw = vim_strsave(p_cinw);
	    COPY_OPT_SCTX(buf, BV_CINW);
#endif
#ifdef FEAT_LISP
	    buf->b_p_lisp = p_lisp;
	    COPY_OPT_SCTX(buf, BV_LISP);
#endif
#ifdef FEAT_SYN_HL
	    // Don't copy 'syntax', it must be set
	    buf->b_p_syn = empty_option;
	    buf->b_p_smc = p_smc;
	    COPY_OPT_SCTX(buf, BV_SMC);
	    buf->b_s.b_syn_isk = empty_option;
#endif
#ifdef FEAT_SPELL
	    buf->b_s.b_p_spc = vim_strsave(p_spc);
	    COPY_OPT_SCTX(buf, BV_SPC);
	    (void)compile_cap_prog(&buf->b_s);
	    buf->b_s.b_p_spf = vim_strsave(p_spf);
	    COPY_OPT_SCTX(buf, BV_SPF);
	    buf->b_s.b_p_spl = vim_strsave(p_spl);
	    COPY_OPT_SCTX(buf, BV_SPL);
	    buf->b_s.b_p_spo = vim_strsave(p_spo);
	    COPY_OPT_SCTX(buf, BV_SPO);
#endif
#if defined(FEAT_CINDENT) && defined(FEAT_EVAL)
	    buf->b_p_inde = vim_strsave(p_inde);
	    COPY_OPT_SCTX(buf, BV_INDE);
	    buf->b_p_indk = vim_strsave(p_indk);
	    COPY_OPT_SCTX(buf, BV_INDK);
#endif
	    buf->b_p_fp = empty_option;
#if defined(FEAT_EVAL)
	    buf->b_p_fex = vim_strsave(p_fex);
	    COPY_OPT_SCTX(buf, BV_FEX);
#endif
#ifdef FEAT_CRYPT
	    buf->b_p_key = vim_strsave(p_key);
	    COPY_OPT_SCTX(buf, BV_KEY);
#endif
#ifdef FEAT_SEARCHPATH
	    buf->b_p_sua = vim_strsave(p_sua);
	    COPY_OPT_SCTX(buf, BV_SUA);
#endif
#ifdef FEAT_KEYMAP
	    buf->b_p_keymap = vim_strsave(p_keymap);
	    COPY_OPT_SCTX(buf, BV_KMAP);
	    buf->b_kmap_state |= KEYMAP_INIT;
#endif
#ifdef FEAT_TERMINAL
	    buf->b_p_twsl = p_twsl;
	    COPY_OPT_SCTX(buf, BV_TWSL);
#endif
	    // This isn't really an option, but copying the langmap and IME
	    // state from the current buffer is better than resetting it.
	    buf->b_p_iminsert = p_iminsert;
	    COPY_OPT_SCTX(buf, BV_IMI);
	    buf->b_p_imsearch = p_imsearch;
	    COPY_OPT_SCTX(buf, BV_IMS);

	    // options that are normally global but also have a local value
	    // are not copied, start using the global value
	    buf->b_p_ar = -1;
	    buf->b_p_ul = NO_LOCAL_UNDOLEVEL;
	    buf->b_p_bkc = empty_option;
	    buf->b_bkc_flags = 0;
#ifdef FEAT_QUICKFIX
	    buf->b_p_gp = empty_option;
	    buf->b_p_mp = empty_option;
	    buf->b_p_efm = empty_option;
#endif
	    buf->b_p_ep = empty_option;
	    buf->b_p_kp = empty_option;
	    buf->b_p_path = empty_option;
	    buf->b_p_tags = empty_option;
	    buf->b_p_tc = empty_option;
	    buf->b_tc_flags = 0;
#ifdef FEAT_FIND_ID
	    buf->b_p_def = empty_option;
	    buf->b_p_inc = empty_option;
# ifdef FEAT_EVAL
	    buf->b_p_inex = vim_strsave(p_inex);
	    COPY_OPT_SCTX(buf, BV_INEX);
# endif
#endif
	    buf->b_p_dict = empty_option;
	    buf->b_p_tsr = empty_option;
#ifdef FEAT_TEXTOBJ
	    buf->b_p_qe = vim_strsave(p_qe);
	    COPY_OPT_SCTX(buf, BV_QE);
#endif
#if defined(FEAT_BEVAL) && defined(FEAT_EVAL)
	    buf->b_p_bexpr = empty_option;
#endif
#if defined(FEAT_CRYPT)
	    buf->b_p_cm = empty_option;
#endif
#ifdef FEAT_PERSISTENT_UNDO
	    buf->b_p_udf = p_udf;
	    COPY_OPT_SCTX(buf, BV_UDF);
#endif
#ifdef FEAT_LISP
	    buf->b_p_lw = empty_option;
#endif
	    buf->b_p_menc = empty_option;

	    /*
	     * Don't copy the options set by ex_help(), use the saved values,
	     * when going from a help buffer to a non-help buffer.
	     * Don't touch these at all when BCO_NOHELP is used and going from
	     * or to a help buffer.
	     */
	    if (dont_do_help)
	    {
		buf->b_p_isk = save_p_isk;
#ifdef FEAT_VARTABS
		if (p_vts && p_vts != empty_option && !buf->b_p_vts_array)
		    (void)tabstop_set(p_vts, &buf->b_p_vts_array);
		else
		    buf->b_p_vts_array = NULL;
#endif
	    }
	    else
	    {
		buf->b_p_isk = vim_strsave(p_isk);
		COPY_OPT_SCTX(buf, BV_ISK);
		did_isk = TRUE;
		buf->b_p_ts = p_ts;
#ifdef FEAT_VARTABS
		buf->b_p_vts = vim_strsave(p_vts);
		COPY_OPT_SCTX(buf, BV_VTS);
		if (p_vts && p_vts != empty_option && !buf->b_p_vts_array)
		    (void)tabstop_set(p_vts, &buf->b_p_vts_array);
		else
		    buf->b_p_vts_array = NULL;
#endif
		buf->b_help = FALSE;
		if (buf->b_p_bt[0] == 'h')
		    clear_string_option(&buf->b_p_bt);
		buf->b_p_ma = p_ma;
		COPY_OPT_SCTX(buf, BV_MA);
	    }
	}

	/*
	 * When the options should be copied (ignoring BCO_ALWAYS), set the
	 * flag that indicates that the options have been initialized.
	 */
	if (should_copy)
	    buf->b_p_initialized = TRUE;
    }

    check_buf_options(buf);	    // make sure we don't have NULLs
    if (did_isk)
	(void)buf_init_chartab(buf, FALSE);
}

/*
 * Reset the 'modifiable' option and its default value.
 */
    void
reset_modifiable(void)
{
    int		opt_idx;

    curbuf->b_p_ma = FALSE;
    p_ma = FALSE;
    opt_idx = findoption((char_u *)"ma");
    if (opt_idx >= 0)
	options[opt_idx].def_val[VI_DEFAULT] = FALSE;
}

/*
 * Set the global value for 'iminsert' to the local value.
 */
    void
set_iminsert_global(void)
{
    p_iminsert = curbuf->b_p_iminsert;
}

/*
 * Set the global value for 'imsearch' to the local value.
 */
    void
set_imsearch_global(void)
{
    p_imsearch = curbuf->b_p_imsearch;
}

static int expand_option_idx = -1;
static char_u expand_option_name[5] = {'t', '_', NUL, NUL, NUL};
static int expand_option_flags = 0;

    void
set_context_in_set_cmd(
    expand_T	*xp,
    char_u	*arg,
    int		opt_flags)	// OPT_GLOBAL and/or OPT_LOCAL
{
    int		nextchar;
    long_u	flags = 0;	// init for GCC
    int		opt_idx = 0;	// init for GCC
    char_u	*p;
    char_u	*s;
    int		is_term_option = FALSE;
    int		key;

    expand_option_flags = opt_flags;

    xp->xp_context = EXPAND_SETTINGS;
    if (*arg == NUL)
    {
	xp->xp_pattern = arg;
	return;
    }
    p = arg + STRLEN(arg) - 1;
    if (*p == ' ' && *(p - 1) != '\\')
    {
	xp->xp_pattern = p + 1;
	return;
    }
    while (p > arg)
    {
	s = p;
	// count number of backslashes before ' ' or ','
	if (*p == ' ' || *p == ',')
	{
	    while (s > arg && *(s - 1) == '\\')
		--s;
	}
	// break at a space with an even number of backslashes
	if (*p == ' ' && ((p - s) & 1) == 0)
	{
	    ++p;
	    break;
	}
	--p;
    }
    if (STRNCMP(p, "no", 2) == 0 && STRNCMP(p, "novice", 6) != 0)
    {
	xp->xp_context = EXPAND_BOOL_SETTINGS;
	p += 2;
    }
    if (STRNCMP(p, "inv", 3) == 0)
    {
	xp->xp_context = EXPAND_BOOL_SETTINGS;
	p += 3;
    }
    xp->xp_pattern = arg = p;
    if (*arg == '<')
    {
	while (*p != '>')
	    if (*p++ == NUL)	    // expand terminal option name
		return;
	key = get_special_key_code(arg + 1);
	if (key == 0)		    // unknown name
	{
	    xp->xp_context = EXPAND_NOTHING;
	    return;
	}
	nextchar = *++p;
	is_term_option = TRUE;
	expand_option_name[2] = KEY2TERMCAP0(key);
	expand_option_name[3] = KEY2TERMCAP1(key);
    }
    else
    {
	if (p[0] == 't' && p[1] == '_')
	{
	    p += 2;
	    if (*p != NUL)
		++p;
	    if (*p == NUL)
		return;		// expand option name
	    nextchar = *++p;
	    is_term_option = TRUE;
	    expand_option_name[2] = p[-2];
	    expand_option_name[3] = p[-1];
	}
	else
	{
	    // Allow * wildcard
	    while (ASCII_ISALNUM(*p) || *p == '_' || *p == '*')
		p++;
	    if (*p == NUL)
		return;
	    nextchar = *p;
	    *p = NUL;
	    opt_idx = findoption(arg);
	    *p = nextchar;
	    if (opt_idx == -1 || options[opt_idx].var == NULL)
	    {
		xp->xp_context = EXPAND_NOTHING;
		return;
	    }
	    flags = options[opt_idx].flags;
	    if (flags & P_BOOL)
	    {
		xp->xp_context = EXPAND_NOTHING;
		return;
	    }
	}
    }
    // handle "-=" and "+="
    if ((nextchar == '-' || nextchar == '+' || nextchar == '^') && p[1] == '=')
    {
	++p;
	nextchar = '=';
    }
    if ((nextchar != '=' && nextchar != ':')
				    || xp->xp_context == EXPAND_BOOL_SETTINGS)
    {
	xp->xp_context = EXPAND_UNSUCCESSFUL;
	return;
    }
    if (xp->xp_context != EXPAND_BOOL_SETTINGS && p[1] == NUL)
    {
	xp->xp_context = EXPAND_OLD_SETTING;
	if (is_term_option)
	    expand_option_idx = -1;
	else
	    expand_option_idx = opt_idx;
	xp->xp_pattern = p + 1;
	return;
    }
    xp->xp_context = EXPAND_NOTHING;
    if (is_term_option || (flags & P_NUM))
	return;

    xp->xp_pattern = p + 1;

    if (flags & P_EXPAND)
    {
	p = options[opt_idx].var;
	if (p == (char_u *)&p_bdir
		|| p == (char_u *)&p_dir
		|| p == (char_u *)&p_path
		|| p == (char_u *)&p_pp
		|| p == (char_u *)&p_rtp
#ifdef FEAT_SEARCHPATH
		|| p == (char_u *)&p_cdpath
#endif
#ifdef FEAT_SESSION
		|| p == (char_u *)&p_vdir
#endif
		)
	{
	    xp->xp_context = EXPAND_DIRECTORIES;
	    if (p == (char_u *)&p_path
#ifdef FEAT_SEARCHPATH
		    || p == (char_u *)&p_cdpath
#endif
		   )
		xp->xp_backslash = XP_BS_THREE;
	    else
		xp->xp_backslash = XP_BS_ONE;
	}
	else if (p == (char_u *)&p_ft)
	{
	    xp->xp_context = EXPAND_FILETYPE;
	}
	else
	{
	    xp->xp_context = EXPAND_FILES;
	    // for 'tags' need three backslashes for a space
	    if (p == (char_u *)&p_tags)
		xp->xp_backslash = XP_BS_THREE;
	    else
		xp->xp_backslash = XP_BS_ONE;
	}
    }

    // For an option that is a list of file names, find the start of the
    // last file name.
    for (p = arg + STRLEN(arg) - 1; p > xp->xp_pattern; --p)
    {
	// count number of backslashes before ' ' or ','
	if (*p == ' ' || *p == ',')
	{
	    s = p;
	    while (s > xp->xp_pattern && *(s - 1) == '\\')
		--s;
	    if ((*p == ' ' && (xp->xp_backslash == XP_BS_THREE && (p - s) < 3))
		    || (*p == ',' && (flags & P_COMMA) && ((p - s) & 1) == 0))
	    {
		xp->xp_pattern = p + 1;
		break;
	    }
	}

#ifdef FEAT_SPELL
	// for 'spellsuggest' start at "file:"
	if (options[opt_idx].var == (char_u *)&p_sps
					       && STRNCMP(p, "file:", 5) == 0)
	{
	    xp->xp_pattern = p + 5;
	    break;
	}
#endif
    }

    return;
}

    int
ExpandSettings(
    expand_T	*xp,
    regmatch_T	*regmatch,
    int		*num_file,
    char_u	***file)
{
    int		num_normal = 0;	    // Nr of matching non-term-code settings
    int		num_term = 0;	    // Nr of matching terminal code settings
    int		opt_idx;
    int		match;
    int		count = 0;
    char_u	*str;
    int		loop;
    int		is_term_opt;
    char_u	name_buf[MAX_KEY_NAME_LEN];
    static char *(names[]) = {"all", "termcap"};
    int		ic = regmatch->rm_ic;	// remember the ignore-case flag

    // do this loop twice:
    // loop == 0: count the number of matching options
    // loop == 1: copy the matching options into allocated memory
    for (loop = 0; loop <= 1; ++loop)
    {
	regmatch->rm_ic = ic;
	if (xp->xp_context != EXPAND_BOOL_SETTINGS)
	{
	    for (match = 0; match < (int)ARRAY_LENGTH(names); ++match)
		if (vim_regexec(regmatch, (char_u *)names[match], (colnr_T)0))
		{
		    if (loop == 0)
			num_normal++;
		    else
			(*file)[count++] = vim_strsave((char_u *)names[match]);
		}
	}
	for (opt_idx = 0; (str = (char_u *)options[opt_idx].fullname) != NULL;
								    opt_idx++)
	{
	    if (options[opt_idx].var == NULL)
		continue;
	    if (xp->xp_context == EXPAND_BOOL_SETTINGS
	      && !(options[opt_idx].flags & P_BOOL))
		continue;
	    is_term_opt = istermoption_idx(opt_idx);
	    if (is_term_opt && num_normal > 0)
		continue;
	    match = FALSE;
	    if (vim_regexec(regmatch, str, (colnr_T)0)
		    || (options[opt_idx].shortname != NULL
			&& vim_regexec(regmatch,
			   (char_u *)options[opt_idx].shortname, (colnr_T)0)))
		match = TRUE;
	    else if (is_term_opt)
	    {
		name_buf[0] = '<';
		name_buf[1] = 't';
		name_buf[2] = '_';
		name_buf[3] = str[2];
		name_buf[4] = str[3];
		name_buf[5] = '>';
		name_buf[6] = NUL;
		if (vim_regexec(regmatch, name_buf, (colnr_T)0))
		{
		    match = TRUE;
		    str = name_buf;
		}
	    }
	    if (match)
	    {
		if (loop == 0)
		{
		    if (is_term_opt)
			num_term++;
		    else
			num_normal++;
		}
		else
		    (*file)[count++] = vim_strsave(str);
	    }
	}
	/*
	 * Check terminal key codes, these are not in the option table
	 */
	if (xp->xp_context != EXPAND_BOOL_SETTINGS  && num_normal == 0)
	{
	    for (opt_idx = 0; (str = get_termcode(opt_idx)) != NULL; opt_idx++)
	    {
		if (!isprint(str[0]) || !isprint(str[1]))
		    continue;

		name_buf[0] = 't';
		name_buf[1] = '_';
		name_buf[2] = str[0];
		name_buf[3] = str[1];
		name_buf[4] = NUL;

		match = FALSE;
		if (vim_regexec(regmatch, name_buf, (colnr_T)0))
		    match = TRUE;
		else
		{
		    name_buf[0] = '<';
		    name_buf[1] = 't';
		    name_buf[2] = '_';
		    name_buf[3] = str[0];
		    name_buf[4] = str[1];
		    name_buf[5] = '>';
		    name_buf[6] = NUL;

		    if (vim_regexec(regmatch, name_buf, (colnr_T)0))
			match = TRUE;
		}
		if (match)
		{
		    if (loop == 0)
			num_term++;
		    else
			(*file)[count++] = vim_strsave(name_buf);
		}
	    }

	    /*
	     * Check special key names.
	     */
	    regmatch->rm_ic = TRUE;		// ignore case here
	    for (opt_idx = 0; (str = get_key_name(opt_idx)) != NULL; opt_idx++)
	    {
		name_buf[0] = '<';
		STRCPY(name_buf + 1, str);
		STRCAT(name_buf, ">");

		if (vim_regexec(regmatch, name_buf, (colnr_T)0))
		{
		    if (loop == 0)
			num_term++;
		    else
			(*file)[count++] = vim_strsave(name_buf);
		}
	    }
	}
	if (loop == 0)
	{
	    if (num_normal > 0)
		*num_file = num_normal;
	    else if (num_term > 0)
		*num_file = num_term;
	    else
		return OK;
	    *file = ALLOC_MULT(char_u *, *num_file);
	    if (*file == NULL)
	    {
		*file = (char_u **)"";
		return FAIL;
	    }
	}
    }
    return OK;
}

    int
ExpandOldSetting(int *num_file, char_u ***file)
{
    char_u  *var = NULL;	// init for GCC
    char_u  *buf;

    *num_file = 0;
    *file = ALLOC_ONE(char_u *);
    if (*file == NULL)
	return FAIL;

    /*
     * For a terminal key code expand_option_idx is < 0.
     */
    if (expand_option_idx < 0)
    {
	var = find_termcode(expand_option_name + 2);
	if (var == NULL)
	    expand_option_idx = findoption(expand_option_name);
    }

    if (expand_option_idx >= 0)
    {
	// put string of option value in NameBuff
	option_value2string(&options[expand_option_idx], expand_option_flags);
	var = NameBuff;
    }
    else if (var == NULL)
	var = (char_u *)"";

    // A backslash is required before some characters.  This is the reverse of
    // what happens in do_set().
    buf = vim_strsave_escaped(var, escape_chars);

    if (buf == NULL)
    {
	VIM_CLEAR(*file);
	return FAIL;
    }

#ifdef BACKSLASH_IN_FILENAME
    // For MS-Windows et al. we don't double backslashes at the start and
    // before a file name character.
    for (var = buf; *var != NUL; MB_PTR_ADV(var))
	if (var[0] == '\\' && var[1] == '\\'
		&& expand_option_idx >= 0
		&& (options[expand_option_idx].flags & P_EXPAND)
		&& vim_isfilec(var[2])
		&& (var[2] != '\\' || (var == buf && var[4] != '\\')))
	    STRMOVE(var, var + 1);
#endif

    *file[0] = buf;
    *num_file = 1;
    return OK;
}

/*
 * Get the value for the numeric or string option *opp in a nice format into
 * NameBuff[].  Must not be called with a hidden option!
 */
    static void
option_value2string(
    struct vimoption	*opp,
    int			opt_flags)	// OPT_GLOBAL and/or OPT_LOCAL
{
    char_u	*varp;

    varp = get_varp_scope(opp, opt_flags);

    if (opp->flags & P_NUM)
    {
	long wc = 0;

	if (wc_use_keyname(varp, &wc))
	    STRCPY(NameBuff, get_special_key_name((int)wc, 0));
	else if (wc != 0)
	    STRCPY(NameBuff, transchar((int)wc));
	else
	    sprintf((char *)NameBuff, "%ld", *(long *)varp);
    }
    else    // P_STRING
    {
	varp = *(char_u **)(varp);
	if (varp == NULL)		    // just in case
	    NameBuff[0] = NUL;
#ifdef FEAT_CRYPT
	// don't show the actual value of 'key', only that it's set
	else if (opp->var == (char_u *)&p_key && *varp)
	    STRCPY(NameBuff, "*****");
#endif
	else if (opp->flags & P_EXPAND)
	    home_replace(NULL, varp, NameBuff, MAXPATHL, FALSE);
	// Translate 'pastetoggle' into special key names
	else if ((char_u **)opp->var == &p_pt)
	    str2specialbuf(p_pt, NameBuff, MAXPATHL);
	else
	    vim_strncpy(NameBuff, varp, MAXPATHL - 1);
    }
}

/*
 * Return TRUE if "varp" points to 'wildchar' or 'wildcharm' and it can be
 * printed as a keyname.
 * "*wcp" is set to the value of the option if it's 'wildchar' or 'wildcharm'.
 */
    static int
wc_use_keyname(char_u *varp, long *wcp)
{
    if (((long *)varp == &p_wc) || ((long *)varp == &p_wcm))
    {
	*wcp = *(long *)varp;
	if (IS_SPECIAL(*wcp) || find_special_key_in_table((int)*wcp) >= 0)
	    return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if "x" is present in 'shortmess' option, or
 * 'shortmess' contains 'a' and "x" is present in SHM_A.
 */
    int
shortmess(int x)
{
    return p_shm != NULL &&
	    (   vim_strchr(p_shm, x) != NULL
	    || (vim_strchr(p_shm, 'a') != NULL
		&& vim_strchr((char_u *)SHM_A, x) != NULL));
}

/*
 * paste_option_changed() - Called after p_paste was set or reset.
 */
    static void
paste_option_changed(void)
{
    static int	old_p_paste = FALSE;
    static int	save_sm = 0;
    static int	save_sta = 0;
#ifdef FEAT_CMDL_INFO
    static int	save_ru = 0;
#endif
#ifdef FEAT_RIGHTLEFT
    static int	save_ri = 0;
    static int	save_hkmap = 0;
#endif
    buf_T	*buf;

    if (p_paste)
    {
	/*
	 * Paste switched from off to on.
	 * Save the current values, so they can be restored later.
	 */
	if (!old_p_paste)
	{
	    // save options for each buffer
	    FOR_ALL_BUFFERS(buf)
	    {
		buf->b_p_tw_nopaste = buf->b_p_tw;
		buf->b_p_wm_nopaste = buf->b_p_wm;
		buf->b_p_sts_nopaste = buf->b_p_sts;
		buf->b_p_ai_nopaste = buf->b_p_ai;
		buf->b_p_et_nopaste = buf->b_p_et;
#ifdef FEAT_VARTABS
		if (buf->b_p_vsts_nopaste)
		    vim_free(buf->b_p_vsts_nopaste);
		buf->b_p_vsts_nopaste = buf->b_p_vsts && buf->b_p_vsts != empty_option
				     ? vim_strsave(buf->b_p_vsts) : NULL;
#endif
	    }

	    // save global options
	    save_sm = p_sm;
	    save_sta = p_sta;
#ifdef FEAT_CMDL_INFO
	    save_ru = p_ru;
#endif
#ifdef FEAT_RIGHTLEFT
	    save_ri = p_ri;
	    save_hkmap = p_hkmap;
#endif
	    // save global values for local buffer options
	    p_ai_nopaste = p_ai;
	    p_et_nopaste = p_et;
	    p_sts_nopaste = p_sts;
	    p_tw_nopaste = p_tw;
	    p_wm_nopaste = p_wm;
#ifdef FEAT_VARTABS
	    if (p_vsts_nopaste)
		vim_free(p_vsts_nopaste);
	    p_vsts_nopaste = p_vsts && p_vsts != empty_option ? vim_strsave(p_vsts) : NULL;
#endif
	}

	/*
	 * Always set the option values, also when 'paste' is set when it is
	 * already on.
	 */
	// set options for each buffer
	FOR_ALL_BUFFERS(buf)
	{
	    buf->b_p_tw = 0;	    // textwidth is 0
	    buf->b_p_wm = 0;	    // wrapmargin is 0
	    buf->b_p_sts = 0;	    // softtabstop is 0
	    buf->b_p_ai = 0;	    // no auto-indent
	    buf->b_p_et = 0;	    // no expandtab
#ifdef FEAT_VARTABS
	    if (buf->b_p_vsts)
		free_string_option(buf->b_p_vsts);
	    buf->b_p_vsts = empty_option;
	    if (buf->b_p_vsts_array)
		vim_free(buf->b_p_vsts_array);
	    buf->b_p_vsts_array = 0;
#endif
	}

	// set global options
	p_sm = 0;		    // no showmatch
	p_sta = 0;		    // no smarttab
#ifdef FEAT_CMDL_INFO
	if (p_ru)
	    status_redraw_all();    // redraw to remove the ruler
	p_ru = 0;		    // no ruler
#endif
#ifdef FEAT_RIGHTLEFT
	p_ri = 0;		    // no reverse insert
	p_hkmap = 0;		    // no Hebrew keyboard
#endif
	// set global values for local buffer options
	p_tw = 0;
	p_wm = 0;
	p_sts = 0;
	p_ai = 0;
#ifdef FEAT_VARTABS
	if (p_vsts)
	    free_string_option(p_vsts);
	p_vsts = empty_option;
#endif
    }

    /*
     * Paste switched from on to off: Restore saved values.
     */
    else if (old_p_paste)
    {
	// restore options for each buffer
	FOR_ALL_BUFFERS(buf)
	{
	    buf->b_p_tw = buf->b_p_tw_nopaste;
	    buf->b_p_wm = buf->b_p_wm_nopaste;
	    buf->b_p_sts = buf->b_p_sts_nopaste;
	    buf->b_p_ai = buf->b_p_ai_nopaste;
	    buf->b_p_et = buf->b_p_et_nopaste;
#ifdef FEAT_VARTABS
	    if (buf->b_p_vsts)
		free_string_option(buf->b_p_vsts);
	    buf->b_p_vsts = buf->b_p_vsts_nopaste
			 ? vim_strsave(buf->b_p_vsts_nopaste) : empty_option;
	    if (buf->b_p_vsts_array)
		vim_free(buf->b_p_vsts_array);
	    if (buf->b_p_vsts && buf->b_p_vsts != empty_option)
		(void)tabstop_set(buf->b_p_vsts, &buf->b_p_vsts_array);
	    else
		buf->b_p_vsts_array = 0;
#endif
	}

	// restore global options
	p_sm = save_sm;
	p_sta = save_sta;
#ifdef FEAT_CMDL_INFO
	if (p_ru != save_ru)
	    status_redraw_all();    // redraw to draw the ruler
	p_ru = save_ru;
#endif
#ifdef FEAT_RIGHTLEFT
	p_ri = save_ri;
	p_hkmap = save_hkmap;
#endif
	// set global values for local buffer options
	p_ai = p_ai_nopaste;
	p_et = p_et_nopaste;
	p_sts = p_sts_nopaste;
	p_tw = p_tw_nopaste;
	p_wm = p_wm_nopaste;
#ifdef FEAT_VARTABS
	if (p_vsts)
	    free_string_option(p_vsts);
	p_vsts = p_vsts_nopaste ? vim_strsave(p_vsts_nopaste) : empty_option;
#endif
    }

    old_p_paste = p_paste;
}

/*
 * vimrc_found() - Called when a ".vimrc" or "VIMINIT" has been found.
 *
 * Reset 'compatible' and set the values for options that didn't get set yet
 * to the Vim defaults.
 * Don't do this if the 'compatible' option has been set or reset before.
 * When "fname" is not NULL, use it to set $"envname" when it wasn't set yet.
 */
    void
vimrc_found(char_u *fname, char_u *envname)
{
    int		opt_idx;
    int		dofree = FALSE;
    char_u	*p;

    if (!option_was_set((char_u *)"cp"))
    {
	p_cp = FALSE;
	for (opt_idx = 0; !istermoption_idx(opt_idx); opt_idx++)
	    if (!(options[opt_idx].flags & (P_WAS_SET|P_VI_DEF)))
		set_option_default(opt_idx, OPT_FREE, FALSE);
	didset_options();
	didset_options2();
    }

    if (fname != NULL)
    {
	p = vim_getenv(envname, &dofree);
	if (p == NULL)
	{
	    // Set $MYVIMRC to the first vimrc file found.
	    p = FullName_save(fname, FALSE);
	    if (p != NULL)
	    {
		vim_setenv(envname, p);
		vim_free(p);
	    }
	}
	else if (dofree)
	    vim_free(p);
    }
}

/*
 * Set 'compatible' on or off.  Called for "-C" and "-N" command line arg.
 */
    void
change_compatible(int on)
{
    int	    opt_idx;

    if (p_cp != on)
    {
	p_cp = on;
	compatible_set();
    }
    opt_idx = findoption((char_u *)"cp");
    if (opt_idx >= 0)
	options[opt_idx].flags |= P_WAS_SET;
}

/*
 * Return TRUE when option "name" has been set.
 * Only works correctly for global options.
 */
    int
option_was_set(char_u *name)
{
    int idx;

    idx = findoption(name);
    if (idx < 0)	// unknown option
	return FALSE;
    if (options[idx].flags & P_WAS_SET)
	return TRUE;
    return FALSE;
}

/*
 * Reset the flag indicating option "name" was set.
 */
    int
reset_option_was_set(char_u *name)
{
    int idx = findoption(name);

    if (idx >= 0)
    {
	options[idx].flags &= ~P_WAS_SET;
	return OK;
    }
    return FAIL;
}

/*
 * compatible_set() - Called when 'compatible' has been set or unset.
 *
 * When 'compatible' set: Set all relevant options (those that have the P_VIM)
 * flag) to a Vi compatible value.
 * When 'compatible' is unset: Set all options that have a different default
 * for Vim (without the P_VI_DEF flag) to that default.
 */
    static void
compatible_set(void)
{
    int	    opt_idx;

    for (opt_idx = 0; !istermoption_idx(opt_idx); opt_idx++)
	if (	   ((options[opt_idx].flags & P_VIM) && p_cp)
		|| (!(options[opt_idx].flags & P_VI_DEF) && !p_cp))
	    set_option_default(opt_idx, OPT_FREE, p_cp);
    didset_options();
    didset_options2();
}

#if defined(FEAT_LINEBREAK) || defined(PROTO)

/*
 * fill_breakat_flags() -- called when 'breakat' changes value.
 */
    void
fill_breakat_flags(void)
{
    char_u	*p;
    int		i;

    for (i = 0; i < 256; i++)
	breakat_flags[i] = FALSE;

    if (p_breakat != NULL)
	for (p = p_breakat; *p; p++)
	    breakat_flags[*p] = TRUE;
}
#endif

/*
 * Check if backspacing over something is allowed.
 */
    int
can_bs(
    int		what)	    // BS_INDENT, BS_EOL, BS_START or BS_NOSTOP
{
#ifdef FEAT_JOB_CHANNEL
    if (what == BS_START && bt_prompt(curbuf))
	return FALSE;
#endif
    switch (*p_bs)
    {
	case '3':       return TRUE;
	case '2':	return (what != BS_NOSTOP);
	case '1':	return (what != BS_START);
	case '0':	return FALSE;
    }
    return vim_strchr(p_bs, what) != NULL;
}

/*
 * Return the effective 'scrolloff' value for the current window, using the
 * global value when appropriate.
 */
    long
get_scrolloff_value(void)
{
    return curwin->w_p_so < 0 ? p_so : curwin->w_p_so;
}

/*
 * Return the effective 'sidescrolloff' value for the current window, using the
 * global value when appropriate.
 */
    long
get_sidescrolloff_value(void)
{
    return curwin->w_p_siso < 0 ? p_siso : curwin->w_p_siso;
}

/*
 * Get the local or global value of 'backupcopy'.
 */
    unsigned int
get_bkc_value(buf_T *buf)
{
    return buf->b_bkc_flags ? buf->b_bkc_flags : bkc_flags;
}

/*
 * Get the local or global value of the 'virtualedit' flags.
 */
    unsigned int
get_ve_flags(void)
{
    return (curwin->w_ve_flags ? curwin->w_ve_flags : ve_flags)
	    & ~(VE_NONE | VE_NONEU);
}

#if defined(FEAT_LINEBREAK) || defined(PROTO)
/*
 * Get the local or global value of 'showbreak'.
 */
    char_u *
get_showbreak_value(win_T *win)
{
    if (win->w_p_sbr == NULL || *win->w_p_sbr == NUL)
	return p_sbr;
    if (STRCMP(win->w_p_sbr, "NONE") == 0)
	return empty_option;
    return win->w_p_sbr;
}
#endif

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Get window or buffer local options.
 */
    dict_T *
get_winbuf_options(int bufopt)
{
    dict_T	*d;
    int		opt_idx;

    d = dict_alloc();
    if (d == NULL)
	return NULL;

    for (opt_idx = 0; !istermoption_idx(opt_idx); opt_idx++)
    {
	struct vimoption *opt = &options[opt_idx];

	if ((bufopt && (opt->indir & PV_BUF))
					 || (!bufopt && (opt->indir & PV_WIN)))
	{
	    char_u *varp = get_varp(opt);

	    if (varp != NULL)
	    {
		if (opt->flags & P_STRING)
		    dict_add_string(d, opt->fullname, *(char_u **)varp);
		else if (opt->flags & P_NUM)
		    dict_add_number(d, opt->fullname, *(long *)varp);
		else
		    dict_add_number(d, opt->fullname, *(int *)varp);
	    }
	}
    }

    return d;
}
#endif

#if defined(FEAT_SYN_HL) || defined(PROTO)
/*
 * This is called when 'culopt' is changed
 */
    int
fill_culopt_flags(char_u *val, win_T *wp)
{
    char_u	*p;
    char_u	culopt_flags_new = 0;

    if (val == NULL)
	p = wp->w_p_culopt;
    else
	p = val;
    while (*p != NUL)
    {
	if (STRNCMP(p, "line", 4) == 0)
	{
	    p += 4;
	    culopt_flags_new |= CULOPT_LINE;
	}
	else if (STRNCMP(p, "both", 4) == 0)
	{
	    p += 4;
	    culopt_flags_new |= CULOPT_LINE | CULOPT_NBR;
	}
	else if (STRNCMP(p, "number", 6) == 0)
	{
	    p += 6;
	    culopt_flags_new |= CULOPT_NBR;
	}
	else if (STRNCMP(p, "screenline", 10) == 0)
	{
	    p += 10;
	    culopt_flags_new |= CULOPT_SCRLINE;
	}

	if (*p != ',' && *p != NUL)
	    return FAIL;
	if (*p == ',')
	    ++p;
    }

    // Can't have both "line" and "screenline".
    if ((culopt_flags_new & CULOPT_LINE) && (culopt_flags_new & CULOPT_SCRLINE))
	return FAIL;
    wp->w_p_culopt_flags = culopt_flags_new;

    return OK;
}
#endif

/*
 * Get the value of 'magic' adjusted for Vim9 script.
 */
    int
magic_isset(void)
{
    switch (magic_overruled)
    {
	case OPTION_MAGIC_ON:      return TRUE;
	case OPTION_MAGIC_OFF:     return FALSE;
	case OPTION_MAGIC_NOT_SET: break;
    }
#ifdef FEAT_EVAL
    if (in_vim9script())
	return TRUE;
#endif
    return p_magic;
}
