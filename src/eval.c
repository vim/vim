/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * eval.c: Expression evaluation.
 */
#if defined(MSDOS) || defined(MSWIN)
# include <io.h>	/* for mch_open(), must be before vim.h */
#endif

#include "vim.h"

#ifdef AMIGA
# include <time.h>	/* for strftime() */
#endif

#ifdef MACOS
# include <time.h>	/* for time_t */
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if defined(FEAT_EVAL) || defined(PROTO)

#if SIZEOF_INT <= 3		/* use long if int is smaller than 32 bits */
typedef long	varnumber_T;
#else
typedef int	varnumber_T;
#endif

/*
 * Structure to hold an internal variable.
 */
typedef struct
{
    char_u	*var_name;	/* name of variable */
    char	var_type;	/* VAR_NUMBER or VAR_STRING */
    union
    {
	varnumber_T	var_number;   /* number value */
	char_u		*var_string;  /* string value (Careful: can be NULL!) */
    } var_val;
} var;

#define VAR_UNKNOWN 0
#define VAR_NUMBER  1
#define VAR_STRING  2

typedef var *	VAR;

/*
 * All user-defined global variables are stored in "variables".
 */
garray_T	variables = {0, 0, sizeof(var), 4, NULL};

/*
 * Array to hold an array with variables local to each sourced script.
 */
static garray_T	    ga_scripts = {0, 0, sizeof(garray_T), 4, NULL};
#define SCRIPT_VARS(id) (((garray_T *)ga_scripts.ga_data)[(id) - 1])


#define VAR_ENTRY(idx)	(((VAR)(variables.ga_data))[idx])
#define VAR_GAP_ENTRY(idx, gap)	(((VAR)(gap->ga_data))[idx])
#define BVAR_ENTRY(idx)	(((VAR)(curbuf->b_vars.ga_data))[idx])
#define WVAR_ENTRY(idx)	(((VAR)(curwin->w_vars.ga_data))[idx])

static int echo_attr = 0;   /* attributes used for ":echo" */

/*
 * Structure to hold info for a user function.
 */
typedef struct ufunc ufunc_T;

struct ufunc
{
    ufunc_T	*next;		/* next function in list */
    char_u	*name;		/* name of function; can start with <SNR>123_
				   (<SNR> is K_SPECIAL KS_EXTRA KE_SNR) */
    int		varargs;	/* variable nr of arguments */
    int		flags;
    int		calls;		/* nr of active calls */
    garray_T	args;		/* arguments */
    garray_T	lines;		/* function lines */
    scid_T	script_ID;	/* ID of script where function was defined,
				   used for s: variables */
};

/* function flags */
#define FC_ABORT    1		/* abort function on error */
#define FC_RANGE    2		/* function accepts range */

/*
 * All user-defined functions are found in the forward-linked function list.
 * The first function is pointed at by firstfunc.
 */
ufunc_T		*firstfunc = NULL;

#define FUNCARG(fp, j)	((char_u **)(fp->args.ga_data))[j]
#define FUNCLINE(fp, j)	((char_u **)(fp->lines.ga_data))[j]

/* structure to hold info for a function that is currently being executed. */
struct funccall
{
    ufunc_T	*func;		/* function being called */
    int		linenr;		/* next line to be executed */
    int		returned;	/* ":return" used */
    int		argcount;	/* nr of arguments */
    VAR		argvars;	/* arguments */
    var		a0_var;		/* "a:0" variable */
    var		firstline;	/* "a:firstline" variable */
    var		lastline;	/* "a:lastline" variable */
    garray_T	l_vars;		/* local function variables */
    VAR		retvar;		/* return value variable */
    linenr_T	breakpoint;	/* next line with breakpoint or zero */
    int		dbg_tick;	/* debug_tick when breakpoint was set */
    int		level;		/* top nesting level of executed function */
};

/*
 * Return the name of the executed function.
 */
    char_u *
func_name(cookie)
    void *cookie;
{
    return ((struct funccall *)cookie)->func->name;
}

/*
 * Return the address holding the next breakpoint line for a funccall cookie.
 */
    linenr_T *
func_breakpoint(cookie)
    void *cookie;
{
    return &((struct funccall *)cookie)->breakpoint;
}

/*
 * Return the address holding the debug tick for a funccall cookie.
 */
    int *
func_dbg_tick(cookie)
    void *cookie;
{
    return &((struct funccall *)cookie)->dbg_tick;
}

/*
 * Return the nesting level for a funccall cookie.
 */
    int
func_level(cookie)
    void *cookie;
{
    return ((struct funccall *)cookie)->level;
}

/* pointer to funccal for currently active function */
struct funccall *current_funccal = NULL;

/*
 * Return TRUE when a function was ended by a ":return" command.
 */
    int
current_func_returned()
{
    return current_funccal->returned;
}


/*
 * Array to hold the value of v: variables.
 */
#include "version.h"

/* values for flags: */
#define VV_COMPAT   1	    /* compatible, also used without "v:" */
#define VV_RO	    2	    /* read-only */

struct vimvar
{
    char	*name;		/* name of variable, without v: */
    int		len;		/* length of name */
    char_u	*val;		/* current value (can also be a number!) */
    char	type;		/* VAR_NUMBER or VAR_STRING */
    char	flags;		/* VV_COMPAT and VV_RO */
} vimvars[VV_LEN] =
{   /* The order here must match the VV_ defines in vim.h! */
    {"count", sizeof("count") - 1, NULL, VAR_NUMBER, VV_COMPAT+VV_RO},
    {"count1", sizeof("count1") - 1, NULL, VAR_NUMBER, VV_RO},
    {"prevcount", sizeof("prevcount") - 1, NULL, VAR_NUMBER, VV_RO},
    {"errmsg", sizeof("errmsg") - 1, NULL, VAR_STRING, VV_COMPAT},
    {"warningmsg", sizeof("warningmsg") - 1, NULL, VAR_STRING, 0},
    {"statusmsg", sizeof("statusmsg") - 1, NULL, VAR_STRING, 0},
    {"shell_error", sizeof("shell_error") - 1, NULL, VAR_NUMBER,
							     VV_COMPAT+VV_RO},
    {"this_session", sizeof("this_session") - 1, NULL, VAR_STRING, VV_COMPAT},
    {"version", sizeof("version") - 1, (char_u *)VIM_VERSION_100,
						 VAR_NUMBER, VV_COMPAT+VV_RO},
    {"lnum", sizeof("lnum") - 1, NULL, VAR_NUMBER, VV_RO},
    {"termresponse", sizeof("termresponse") - 1, NULL, VAR_STRING, VV_RO},
    {"fname", sizeof("fname") - 1, NULL, VAR_STRING, VV_RO},
    {"lang", sizeof("lang") - 1, NULL, VAR_STRING, VV_RO},
    {"lc_time", sizeof("lc_time") - 1, NULL, VAR_STRING, VV_RO},
    {"ctype", sizeof("ctype") - 1, NULL, VAR_STRING, VV_RO},
    {"charconvert_from", sizeof("charconvert_from") - 1, NULL, VAR_STRING, VV_RO},
    {"charconvert_to", sizeof("charconvert_to") - 1, NULL, VAR_STRING, VV_RO},
    {"fname_in", sizeof("fname_in") - 1, NULL, VAR_STRING, VV_RO},
    {"fname_out", sizeof("fname_out") - 1, NULL, VAR_STRING, VV_RO},
    {"fname_new", sizeof("fname_new") - 1, NULL, VAR_STRING, VV_RO},
    {"fname_diff", sizeof("fname_diff") - 1, NULL, VAR_STRING, VV_RO},
    {"cmdarg", sizeof("cmdarg") - 1, NULL, VAR_STRING, VV_RO},
    {"foldstart", sizeof("foldstart") - 1, NULL, VAR_NUMBER, VV_RO},
    {"foldend", sizeof("foldend") - 1, NULL, VAR_NUMBER, VV_RO},
    {"folddashes", sizeof("folddashes") - 1, NULL, VAR_STRING, VV_RO},
    {"foldlevel", sizeof("foldlevel") - 1, NULL, VAR_NUMBER, VV_RO},
    {"progname", sizeof("progname") - 1, NULL, VAR_STRING, VV_RO},
    {"servername", sizeof("servername") - 1, NULL, VAR_STRING, VV_RO},
    {"dying", sizeof("dying") - 1, NULL, VAR_NUMBER, VV_RO},
    {"exception", sizeof("exception") - 1, NULL, VAR_STRING, VV_RO},
    {"throwpoint", sizeof("throwpoint") - 1, NULL, VAR_STRING, VV_RO},
    {"register", sizeof("register") - 1, NULL, VAR_STRING, VV_RO},
    {"cmdbang", sizeof("cmdbang") - 1, NULL, VAR_NUMBER, VV_RO},
};

static int eval0 __ARGS((char_u *arg,  VAR retvar, char_u **nextcmd, int evaluate));
static int eval1 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval2 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval3 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval4 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval5 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval6 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval7 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_option_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_string_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_lit_string_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_env_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int find_internal_func __ARGS((char_u *name));
static int get_func_var __ARGS((char_u *name, int len, VAR retvar, char_u **arg, linenr_T firstline, linenr_T lastline, int *doesrange, int evaluate));
static int call_func __ARGS((char_u *name, int len, VAR retvar, int argcount, VAR argvars, linenr_T firstline, linenr_T lastline, int *doesrange, int evaluate));
static void f_append __ARGS((VAR argvars, VAR retvar));
static void f_argc __ARGS((VAR argvars, VAR retvar));
static void f_argidx __ARGS((VAR argvars, VAR retvar));
static void f_argv __ARGS((VAR argvars, VAR retvar));
static void f_browse __ARGS((VAR argvars, VAR retvar));
static buf_T *find_buffer __ARGS((VAR avar));
static void f_bufexists __ARGS((VAR argvars, VAR retvar));
static void f_buflisted __ARGS((VAR argvars, VAR retvar));
static void f_bufloaded __ARGS((VAR argvars, VAR retvar));
static buf_T *get_buf_var __ARGS((VAR avar));
static void f_bufname __ARGS((VAR argvars, VAR retvar));
static void f_bufnr __ARGS((VAR argvars, VAR retvar));
static void f_bufwinnr __ARGS((VAR argvars, VAR retvar));
static void f_byte2line __ARGS((VAR argvars, VAR retvar));
static void f_char2nr __ARGS((VAR argvars, VAR retvar));
static void f_cindent __ARGS((VAR argvars, VAR retvar));
static void f_col __ARGS((VAR argvars, VAR retvar));
static void f_confirm __ARGS((VAR argvars, VAR retvar));
static void f_cscope_connection __ARGS((VAR argvars, VAR retvar));
static void f_cursor __ARGS((VAR argsvars, VAR retvar));
static void f_delete __ARGS((VAR argvars, VAR retvar));
static void f_did_filetype __ARGS((VAR argvars, VAR retvar));
static void f_escape __ARGS((VAR argvars, VAR retvar));
static void f_eventhandler __ARGS((VAR argvars, VAR retvar));
static void f_executable __ARGS((VAR argvars, VAR retvar));
static void f_exists __ARGS((VAR argvars, VAR retvar));
static void f_expand __ARGS((VAR argvars, VAR retvar));
static void f_filereadable __ARGS((VAR argvars, VAR retvar));
static void f_filewritable __ARGS((VAR argvars, VAR retvar));
static void f_fnamemodify __ARGS((VAR argvars, VAR retvar));
static void f_foldclosed __ARGS((VAR argvars, VAR retvar));
static void f_foldclosedend __ARGS((VAR argvars, VAR retvar));
static void foldclosed_both __ARGS((VAR argvars, VAR retvar, int end));
static void f_foldlevel __ARGS((VAR argvars, VAR retvar));
static void f_foldtext __ARGS((VAR argvars, VAR retvar));
static void f_foreground __ARGS((VAR argvars, VAR retvar));
static void f_getbufvar __ARGS((VAR argvars, VAR retvar));
static void f_getchar __ARGS((VAR argvars, VAR retvar));
static void f_getcharmod __ARGS((VAR argvars, VAR retvar));
static void f_getcmdline __ARGS((VAR argvars, VAR retvar));
static void f_getcmdpos __ARGS((VAR argvars, VAR retvar));
static void f_getcwd __ARGS((VAR argvars, VAR retvar));
static void f_getfsize __ARGS((VAR argvars, VAR retvar));
static void f_getftime __ARGS((VAR argvars, VAR retvar));
static void f_getline __ARGS((VAR argvars, VAR retvar));
static void f_getreg __ARGS((VAR argvars, VAR retvar));
static void f_getregtype __ARGS((VAR argvars, VAR retvar));
static void f_getwinposx __ARGS((VAR argvars, VAR retvar));
static void f_getwinposy __ARGS((VAR argvars, VAR retvar));
static void f_getwinvar __ARGS((VAR argvars, VAR retvar));
static void f_glob __ARGS((VAR argvars, VAR retvar));
static void f_globpath __ARGS((VAR argvars, VAR retvar));
static void f_has __ARGS((VAR argvars, VAR retvar));
static void f_hasmapto __ARGS((VAR argvars, VAR retvar));
static void f_histadd __ARGS((VAR argvars, VAR retvar));
static void f_histdel __ARGS((VAR argvars, VAR retvar));
static void f_histget __ARGS((VAR argvars, VAR retvar));
static void f_histnr __ARGS((VAR argvars, VAR retvar));
static void f_hlexists __ARGS((VAR argvars, VAR retvar));
static void f_hlID __ARGS((VAR argvars, VAR retvar));
static void f_hostname __ARGS((VAR argvars, VAR retvar));
static void f_iconv __ARGS((VAR argvars, VAR retvar));
static void f_indent __ARGS((VAR argvars, VAR retvar));
static void f_isdirectory __ARGS((VAR argvars, VAR retvar));
static void f_input __ARGS((VAR argvars, VAR retvar));
static void f_inputdialog __ARGS((VAR argvars, VAR retvar));
static void f_inputrestore __ARGS((VAR argvars, VAR retvar));
static void f_inputsave __ARGS((VAR argvars, VAR retvar));
static void f_inputsecret __ARGS((VAR argvars, VAR retvar));
static void f_last_buffer_nr __ARGS((VAR argvars, VAR retvar));
static void f_libcall __ARGS((VAR argvars, VAR retvar));
static void f_libcallnr __ARGS((VAR argvars, VAR retvar));
static void libcall_common __ARGS((VAR argvars, VAR retvar, int type));
static void f_line __ARGS((VAR argvars, VAR retvar));
static void f_line2byte __ARGS((VAR argvars, VAR retvar));
static void f_lispindent __ARGS((VAR argvars, VAR retvar));
static void f_localtime __ARGS((VAR argvars, VAR retvar));
static void f_maparg __ARGS((VAR argvars, VAR retvar));
static void f_mapcheck __ARGS((VAR argvars, VAR retvar));
static void get_maparg __ARGS((VAR argvars, VAR retvar, int exact));
static void f_match __ARGS((VAR argvars, VAR retvar));
static void f_matchend __ARGS((VAR argvars, VAR retvar));
static void f_matchstr __ARGS((VAR argvars, VAR retvar));
static void f_mode __ARGS((VAR argvars, VAR retvar));
static void f_nextnonblank __ARGS((VAR argvars, VAR retvar));
static void f_nr2char __ARGS((VAR argvars, VAR retvar));
static void f_prevnonblank __ARGS((VAR argvars, VAR retvar));
static void f_setbufvar __ARGS((VAR argvars, VAR retvar));
static void f_setcmdpos __ARGS((VAR argvars, VAR retvar));
static void f_setwinvar __ARGS((VAR argvars, VAR retvar));
static void f_rename __ARGS((VAR argvars, VAR retvar));
static void f_resolve __ARGS((VAR argvars, VAR retvar));
static void f_search __ARGS((VAR argvars, VAR retvar));
static void f_searchpair __ARGS((VAR argvars, VAR retvar));
static int get_search_arg __ARGS((VAR varp, int *flagsp));
static void f_remote_expr __ARGS((VAR argvars, VAR retvar));
static void f_remote_foreground __ARGS((VAR argvars, VAR retvar));
static void f_remote_peek __ARGS((VAR argvars, VAR retvar));
static void f_remote_read __ARGS((VAR argvars, VAR retvar));
static void f_remote_send __ARGS((VAR argvars, VAR retvar));
static void f_server2client __ARGS((VAR argvars, VAR retvar));
static void f_serverlist __ARGS((VAR argvars, VAR retvar));
static void f_setline __ARGS((VAR argvars, VAR retvar));
static void f_setreg __ARGS((VAR argvars, VAR retvar));
static void f_simplify __ARGS((VAR argvars, VAR retvar));
static void find_some_match __ARGS((VAR argvars, VAR retvar, int start));
static void f_strftime __ARGS((VAR argvars, VAR retvar));
static void f_stridx __ARGS((VAR argvars, VAR retvar));
static void f_strlen __ARGS((VAR argvars, VAR retvar));
static void f_strpart __ARGS((VAR argvars, VAR retvar));
static void f_strridx __ARGS((VAR argvars, VAR retvar));
static void f_strtrans __ARGS((VAR argvars, VAR retvar));
static void f_synID __ARGS((VAR argvars, VAR retvar));
static void f_synIDattr __ARGS((VAR argvars, VAR retvar));
static void f_synIDtrans __ARGS((VAR argvars, VAR retvar));
static void f_system __ARGS((VAR argvars, VAR retvar));
static void f_submatch __ARGS((VAR argvars, VAR retvar));
static void f_substitute __ARGS((VAR argvars, VAR retvar));
static void f_tempname __ARGS((VAR argvars, VAR retvar));
static void f_tolower __ARGS((VAR argvars, VAR retvar));
static void f_toupper __ARGS((VAR argvars, VAR retvar));
static void f_type __ARGS((VAR argvars, VAR retvar));
static void f_virtcol __ARGS((VAR argvars, VAR retvar));
static void f_visualmode __ARGS((VAR argvars, VAR retvar));
static void f_winbufnr __ARGS((VAR argvars, VAR retvar));
static void f_wincol __ARGS((VAR argvars, VAR retvar));
static void f_winheight __ARGS((VAR argvars, VAR retvar));
static void f_winline __ARGS((VAR argvars, VAR retvar));
static void f_winnr __ARGS((VAR argvars, VAR retvar));
static void f_winrestcmd __ARGS((VAR argvars, VAR retvar));
static void f_winwidth __ARGS((VAR argvars, VAR retvar));
static win_T *find_win_by_nr __ARGS((VAR vp));
static pos_T *var2fpos __ARGS((VAR varp, int lnum));
static int get_env_len __ARGS((char_u **arg));
static int get_id_len __ARGS((char_u **arg));
static int get_func_len __ARGS((char_u **arg, char_u **alias, int evaluate));
static char_u *find_name_end __ARGS((char_u *arg, char_u **expr_start, char_u **expr_end));
static int eval_isnamec __ARGS((int c));
static int find_vim_var __ARGS((char_u *name, int len));
static int get_var_var __ARGS((char_u *name, int len, VAR retvar));
static VAR alloc_var __ARGS((void));
static VAR alloc_string_var __ARGS((char_u *string));
static void free_var __ARGS((VAR varp));
static void clear_var __ARGS((VAR varp));
static long get_var_number __ARGS((VAR varp));
static linenr_T get_var_lnum __ARGS((VAR argvars));
static char_u *get_var_string __ARGS((VAR varp));
static char_u *get_var_string_buf __ARGS((VAR varp, char_u *buf));
static VAR find_var __ARGS((char_u *name, int writing));
static VAR find_var_in_ga __ARGS((garray_T *gap, char_u *varname));
static garray_T *find_var_ga __ARGS((char_u *name, char_u **varname));
static void var_free_one __ARGS((VAR v));
static void list_one_var __ARGS((VAR v, char_u *prefix));
static void list_vim_var __ARGS((int i));
static void list_one_var_a __ARGS((char_u *prefix, char_u *name, int type, char_u *string));
static void set_var __ARGS((char_u *name, VAR varp));
static void copy_var __ARGS((VAR from, VAR to));
static char_u *find_option_end __ARGS((char_u **arg, int *opt_flags));
static char_u *trans_function_name __ARGS((char_u **pp, int skip, int internal));
static int eval_fname_script __ARGS((char_u *p));
static int eval_fname_sid __ARGS((char_u *p));
static void list_func_head __ARGS((ufunc_T *fp, int indent));
static void cat_func_name __ARGS((char_u *buf, ufunc_T *fp));
static ufunc_T *find_func __ARGS((char_u *name));
static void call_user_func __ARGS((ufunc_T *fp, int argcount, VAR argvars, VAR retvar, linenr_T firstline, linenr_T lastline));

/* Magic braces are always enabled, otherwise Vim scripts would not be
 * portable. */
#define FEAT_MAGIC_BRACES

#ifdef FEAT_MAGIC_BRACES
static char_u * make_expanded_name __ARGS((char_u *in_start,  char_u *expr_start,  char_u *expr_end,  char_u *in_end));
#endif

/*
 * Set an internal variable to a string value. Creates the variable if it does
 * not already exist.
 */
    void
set_internal_string_var(name, value)
    char_u	*name;
    char_u	*value;
{
    char_u  *val;
    VAR	    varp;

    val = vim_strsave(value);
    if (val != NULL)
    {
	varp = alloc_string_var(val);
	if (varp != NULL)
	{
	    set_var(name, varp);
	    free_var(varp);
	}
    }
}

# if defined(FEAT_MBYTE) || defined(PROTO)
    int
eval_charconvert(enc_from, enc_to, fname_from, fname_to)
    char_u	*enc_from;
    char_u	*enc_to;
    char_u	*fname_from;
    char_u	*fname_to;
{
    int		err = FALSE;

    set_vim_var_string(VV_CC_FROM, enc_from, -1);
    set_vim_var_string(VV_CC_TO, enc_to, -1);
    set_vim_var_string(VV_FNAME_IN, fname_from, -1);
    set_vim_var_string(VV_FNAME_OUT, fname_to, -1);
    if (eval_to_bool(p_ccv, &err, NULL, FALSE))
	err = TRUE;
    set_vim_var_string(VV_CC_FROM, NULL, -1);
    set_vim_var_string(VV_CC_TO, NULL, -1);
    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_FNAME_OUT, NULL, -1);

    if (err)
	return FAIL;
    return OK;
}
# endif

# if defined(FEAT_POSTSCRIPT) || defined(PROTO)
    int
eval_printexpr(fname, args)
    char_u	*fname;
    char_u	*args;
{
    int		err = FALSE;

    set_vim_var_string(VV_FNAME_IN, fname, -1);
    set_vim_var_string(VV_CMDARG, args, -1);
    if (eval_to_bool(p_pexpr, &err, NULL, FALSE))
	err = TRUE;
    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_CMDARG, NULL, -1);

    if (err)
    {
	mch_remove(fname);
	return FAIL;
    }
    return OK;
}
# endif

# if defined(FEAT_DIFF) || defined(PROTO)
    void
eval_diff(origfile, newfile, outfile)
    char_u	*origfile;
    char_u	*newfile;
    char_u	*outfile;
{
    int		err = FALSE;

    set_vim_var_string(VV_FNAME_IN, origfile, -1);
    set_vim_var_string(VV_FNAME_NEW, newfile, -1);
    set_vim_var_string(VV_FNAME_OUT, outfile, -1);
    (void)eval_to_bool(p_dex, &err, NULL, FALSE);
    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_FNAME_NEW, NULL, -1);
    set_vim_var_string(VV_FNAME_OUT, NULL, -1);
}

    void
eval_patch(origfile, difffile, outfile)
    char_u	*origfile;
    char_u	*difffile;
    char_u	*outfile;
{
    int		err;

    set_vim_var_string(VV_FNAME_IN, origfile, -1);
    set_vim_var_string(VV_FNAME_DIFF, difffile, -1);
    set_vim_var_string(VV_FNAME_OUT, outfile, -1);
    (void)eval_to_bool(p_pex, &err, NULL, FALSE);
    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_FNAME_DIFF, NULL, -1);
    set_vim_var_string(VV_FNAME_OUT, NULL, -1);
}
# endif

/*
 * Top level evaluation function, returning a boolean.
 * Sets "error" to TRUE if there was an error.
 * Return TRUE or FALSE.
 */
    int
eval_to_bool(arg, error, nextcmd, skip)
    char_u	*arg;
    int		*error;
    char_u	**nextcmd;
    int		skip;	    /* only parse, don't execute */
{
    var		retvar;
    int		retval = FALSE;

    if (skip)
	++emsg_skip;
    if (eval0(arg, &retvar, nextcmd, !skip) == FAIL)
    {
	*error = TRUE;
    }
    else
    {
	*error = FALSE;
	if (!skip)
	{
	    retval = (get_var_number(&retvar) != 0);
	    clear_var(&retvar);
	}
    }
    if (skip)
	--emsg_skip;

    return retval;
}

/*
 * Top level evaluation function, returning a string.  If "skip" is TRUE,
 * only parsing to "nextcmd" is done, without reporting errors.  Return
 * pointer to allocated memory, or NULL for failure or when "skip" is TRUE.
 */
    char_u *
eval_to_string_skip(arg, nextcmd, skip)
    char_u	*arg;
    char_u	**nextcmd;
    int		skip;	    /* only parse, don't execute */
{
    var		retvar;
    char_u	*retval;

    if (skip)
	++emsg_skip;
    if (eval0(arg, &retvar, nextcmd, !skip) == FAIL || skip)
	retval = NULL;
    else
    {
	retval = vim_strsave(get_var_string(&retvar));
	clear_var(&retvar);
    }
    if (skip)
	--emsg_skip;

    return retval;
}

/*
 * Top level evaluation function, returning a string.
 * Return pointer to allocated memory, or NULL for failure.
 */
    char_u *
eval_to_string(arg, nextcmd)
    char_u	*arg;
    char_u	**nextcmd;
{
    var		retvar;
    char_u	*retval;

    if (eval0(arg, &retvar, nextcmd, TRUE) == FAIL)
	retval = NULL;
    else
    {
	retval = vim_strsave(get_var_string(&retvar));
	clear_var(&retvar);
    }

    return retval;
}

/*
 * Call eval_to_string() with "sandbox" set and not using local variables.
 */
    char_u *
eval_to_string_safe(arg, nextcmd)
    char_u	*arg;
    char_u	**nextcmd;
{
    char_u	*retval;
    void	*save_funccalp;

    save_funccalp = save_funccal();
    ++sandbox;
    retval = eval_to_string(arg, nextcmd);
    --sandbox;
    restore_funccal(save_funccalp);
    return retval;
}

#if 0 /* not used */
/*
 * Top level evaluation function, returning a string.
 * Advances "arg" to the first non-blank after the evaluated expression.
 * Return pointer to allocated memory, or NULL for failure.
 * Doesn't give error messages.
 */
    char_u *
eval_arg_to_string(arg)
    char_u	**arg;
{
    var		retvar;
    char_u	*retval;
    int		ret;

    ++emsg_off;

    ret = eval1(arg, &retvar, TRUE);
    if (ret == FAIL)
	retval = NULL;
    else
    {
	retval = vim_strsave(get_var_string(&retvar));
	clear_var(&retvar);
    }

    --emsg_off;

    return retval;
}
#endif

/*
 * Top level evaluation function, returning a number.
 * Evaluates "expr" silently.
 * Returns -1 for an error.
 */
    int
eval_to_number(expr)
    char_u	*expr;
{
    var		retvar;
    int		retval;
    char_u	*p = expr;

    ++emsg_off;

    if (eval1(&p, &retvar, TRUE) == FAIL)
	retval = -1;
    else
    {
	retval = get_var_number(&retvar);
	clear_var(&retvar);
    }
    --emsg_off;

    return retval;
}

#if (defined(FEAT_USR_CMDS) && defined(FEAT_CMDL_COMPL)) || defined(PROTO)
/*
 * Call some vimL function and return the result as a string
 * Uses argv[argc] for the function arguments.
 */
    char_u *
call_vim_function(func, argc, argv, safe)
    char_u      *func;
    int		argc;
    char_u      **argv;
    int		safe;		/* use the sandbox */
{
    char_u	*retval = NULL;
    var		retvar;
    VAR		argvars;
    long	n;
    int		len;
    int		i;
    int		doesrange;
    void	*save_funccalp = NULL;

    argvars = (VAR)alloc((unsigned)(argc * sizeof(var)));
    if (argvars == NULL)
	return NULL;

    for (i = 0; i < argc; i++)
    {
	/* Recognize a number argument, the others must be strings. */
	vim_str2nr(argv[i], NULL, &len, TRUE, TRUE, &n, NULL);
	if (len != 0 && len == (int)STRLEN(argv[i]))
	{
	    argvars[i].var_type = VAR_NUMBER;
	    argvars[i].var_val.var_number = n;
	}
	else
	{
	    argvars[i].var_type = VAR_STRING;
	    argvars[i].var_val.var_string = argv[i];
	}
    }

    if (safe)
    {
	save_funccalp = save_funccal();
	++sandbox;
    }

    retvar.var_type = VAR_UNKNOWN;	/* clear_var() uses this */
    if (call_func(func, (int)STRLEN(func), &retvar, argc, argvars,
		    curwin->w_cursor.lnum, curwin->w_cursor.lnum,
		    &doesrange, TRUE) == OK)
	retval = vim_strsave(get_var_string(&retvar));

    clear_var(&retvar);
    vim_free(argvars);

    if (safe)
    {
	--sandbox;
	restore_funccal(save_funccalp);
    }
    return retval;
}
#endif

/*
 * Save the current function call pointer, and set it to NULL.
 * Used when executing autocommands and for ":source".
 */
    void *
save_funccal()
{
    struct funccall *fc;

    fc = current_funccal;
    current_funccal = NULL;
    return (void *)fc;
}

    void
restore_funccal(fc)
    void *fc;
{
    current_funccal = (struct funccall *)fc;
}

#ifdef FEAT_FOLDING
/*
 * Evaluate 'foldexpr'.  Returns the foldlevel, and any character preceding
 * it in "*cp".  Doesn't give error messages.
 */
    int
eval_foldexpr(arg, cp)
    char_u	*arg;
    int		*cp;
{
    var		retvar;
    int		retval;
    char_u	*s;

    ++emsg_off;
    ++sandbox;
    *cp = NUL;
    if (eval0(arg, &retvar, NULL, TRUE) == FAIL)
	retval = 0;
    else
    {
	/* If the result is a number, just return the number. */
	if (retvar.var_type == VAR_NUMBER)
	    retval = retvar.var_val.var_number;
	else if (retvar.var_type == VAR_UNKNOWN
		|| retvar.var_val.var_string == NULL)
	    retval = 0;
	else
	{
	    /* If the result is a string, check if there is a non-digit before
	     * the number. */
	    s = retvar.var_val.var_string;
	    if (!VIM_ISDIGIT(*s) && *s != '-')
		*cp = *s++;
	    retval = atol((char *)s);
	}
	clear_var(&retvar);
    }
    --emsg_off;
    --sandbox;

    return retval;
}
#endif

#ifdef FEAT_MAGIC_BRACES
/*
 * Expands out the 'magic' {}'s in a variable/function name.
 * Note that this can call itself recursively, to deal with
 * constructs like foo{bar}{baz}{bam}
 * The four pointer arguments point to "foo{expre}ss{ion}bar"
 *			"in_start"      ^
 *			"expr_start"	   ^
 *			"expr_end"		 ^
 *			"in_end"			    ^
 *
 * Returns a new allocated string, which the caller must free.
 * Returns NULL for failure.
 */
    static char_u *
make_expanded_name(in_start, expr_start, expr_end, in_end)
    char_u	*in_start;
    char_u	*expr_start;
    char_u	*expr_end;
    char_u	*in_end;
{
    char_u	c1;
    char_u	*retval = NULL;
    char_u	*temp_result;
    char_u	*nextcmd = NULL;

    if (expr_end == NULL || in_end == NULL)
	return NULL;
    *expr_start	= NUL;
    *expr_end = NUL;
    c1 = *in_end;
    *in_end = NUL;

    temp_result = eval_to_string(expr_start + 1, &nextcmd);
    if (temp_result != NULL && nextcmd == NULL)
    {
	retval = alloc((unsigned)(STRLEN(temp_result) + (expr_start - in_start)
						   + (in_end - expr_end) + 1));

	if (retval != NULL)
	{
	    STRCPY(retval, in_start);
	    STRCAT(retval, temp_result);
	    STRCAT(retval, expr_end + 1);
	}
    }
    vim_free(temp_result);

    *in_end = c1;		/* put char back for error messages */
    *expr_start = '{';
    *expr_end = '}';

    if (retval != NULL)
    {
	temp_result = find_name_end(retval, &expr_start, &expr_end);
	if (expr_start != NULL)
	{
	    /* Further expansion! */
	    temp_result = make_expanded_name(retval, expr_start,
						       expr_end, temp_result);
	    vim_free(retval);
	    retval = temp_result;
	}
    }

    return retval;

}
#endif /* FEAT_MAGIC_BRACES */

/*
 * ":let var = expr"	assignment command.
 * ":let var"		list one variable value
 * ":let"		list all variable values
 */
    void
ex_let(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    char_u	*expr;
    char_u	*name;
    VAR		varp;
    var		retvar;
    char_u	*p;
    int		c1 = 0, c2;
    int		i;
    char_u	*expr_start;
    char_u	*expr_end;
    char_u	*name_end;

    name_end = find_name_end(arg, &expr_start, &expr_end);
    expr = vim_strchr(name_end, '=');
    if (expr == NULL)
    {
	if (ends_excmd(*arg))
	{
	    if (!eap->skip)
	    {
		/*
		 * List all variables.
		 */
		for (i = 0; i < variables.ga_len && !got_int; ++i)
		    if (VAR_ENTRY(i).var_name != NULL)
			list_one_var(&VAR_ENTRY(i), (char_u *)"");
		for (i = 0; i < curbuf->b_vars.ga_len && !got_int; ++i)
		    if (BVAR_ENTRY(i).var_name != NULL)
			list_one_var(&BVAR_ENTRY(i), (char_u *)"b:");
		for (i = 0; i < curwin->w_vars.ga_len && !got_int; ++i)
		    if (WVAR_ENTRY(i).var_name != NULL)
			list_one_var(&WVAR_ENTRY(i), (char_u *)"w:");
		for (i = 0; i < VV_LEN && !got_int; ++i)
		    if (vimvars[i].type == VAR_NUMBER || vimvars[i].val != NULL)
			list_vim_var(i);
	    }
	}
	else
	{
	    int		error = FALSE;

	    /*
	     * List variables.
	     */
	    while (!ends_excmd(*arg) && !got_int)
	    {
		char_u	*temp_string = NULL;
		int	arg_len;

		/* Find the end of the name. */
		name_end = find_name_end(arg, &expr_start, &expr_end);

		if (!vim_iswhite(*name_end) && !ends_excmd(*name_end))
		{
		    emsg_severe = TRUE;
		    EMSG(_(e_trailing));
		    break;
		}
		if (!error && !eap->skip)
		{
#ifdef FEAT_MAGIC_BRACES
		    if (expr_start != NULL)
		    {
			temp_string = make_expanded_name(arg, expr_start,
							 expr_end, name_end);
			if (temp_string == NULL)
			{
			    /*
			     * Report an invalid expression in braces, unless
			     * the expression evaluation has been cancelled due
			     * to an aborting error, an interrupt, or an
			     * exception.
			     */
			    if (!aborting())
			    {
				emsg_severe = TRUE;
				EMSG2(_(e_invarg2), arg);
				break;
			    }
			    error = TRUE;
			    arg = skipwhite(name_end);
			    continue;
			}
			arg = temp_string;
			arg_len = STRLEN(temp_string);
		    }
		    else
#endif
		    {
			c1 = *name_end;
			*name_end = NUL;
			arg_len = (int)(name_end - arg);
		    }
		    i = find_vim_var(arg, arg_len);
		    if (i >= 0)
			list_vim_var(i);
		    else if (STRCMP("b:changedtick", arg) == 0)
		    {
			char_u	numbuf[NUMBUFLEN];

			sprintf((char *)numbuf, "%ld",
						 (long)curbuf->b_changedtick);
			list_one_var_a((char_u *)"b:", (char_u *)"changedtick",
							  VAR_NUMBER, numbuf);
		    }
		    else
		    {
			varp = find_var(arg, FALSE);
			if (varp == NULL)
			{
			    /* Skip further arguments but do continue to
			     * search for a trailing command. */
			    EMSG2(_("E106: Unknown variable: \"%s\""), arg);
			    error = TRUE;
			}
			else
			{
			    name = vim_strchr(arg, ':');
			    if (name != NULL)
			    {
				/* "a:" vars have no name stored, use whole
				 * arg */
				if (arg[0] == 'a' && arg[1] == ':')
				    c2 = NUL;
				else
				{
				    c2 = *++name;
				    *name = NUL;
				}
				list_one_var(varp, arg);
				if (c2 != NUL)
				    *name = c2;
			    }
			    else
				list_one_var(varp, (char_u *)"");
			}
		    }
#ifdef FEAT_MAGIC_BRACES
		    if (expr_start != NULL)
			vim_free(temp_string);
		    else
#endif
			*name_end = c1;
		}
		arg = skipwhite(name_end);
	    }
	}
	eap->nextcmd = check_nextcmd(arg);
    }
    else
    {
	if (eap->skip)
	    ++emsg_skip;
	i = eval0(expr + 1, &retvar, &eap->nextcmd, !eap->skip);
	if (eap->skip)
	{
	    if (i != FAIL)
		clear_var(&retvar);
	    --emsg_skip;
	}
	else if (i != FAIL)
	{
	    /*
	     * ":let $VAR = expr": Set environment variable.
	     */
	    if (*arg == '$')
	    {
		int	len;
		int	cc;

		/* Find the end of the name. */
		++arg;
		name = arg;
		len = get_env_len(&arg);
		if (len == 0)
		    EMSG2(_(e_invarg2), name - 1);
		else
		{
		    if (*skipwhite(arg) != '=')
			EMSG(_(e_letunexp));
		    else
		    {
			cc = name[len];
			name[len] = NUL;
			p = get_var_string(&retvar);
			vim_setenv(name, p);
			if (STRICMP(name, "HOME") == 0)
			    init_homedir();
			else if (didset_vim && STRICMP(name, "VIM") == 0)
			    didset_vim = FALSE;
			else if (didset_vimruntime
					  && STRICMP(name, "VIMRUNTIME") == 0)
			    didset_vimruntime = FALSE;
			name[len] = cc;
		    }
		}
	    }

	    /*
	     * ":let &option = expr": Set option value.
	     * ":let &l:option = expr": Set local option value.
	     * ":let &g:option = expr": Set global option value.
	     */
	    else if (*arg == '&')
	    {
		int opt_flags;

		/*
		 * Find the end of the name;
		 */
		p = find_option_end(&arg, &opt_flags);
		if (p == NULL || *skipwhite(p) != '=')
		    EMSG(_(e_letunexp));
		else
		{
		    c1 = *p;
		    *p = NUL;
		    set_option_value(arg, get_var_number(&retvar),
					  get_var_string(&retvar), opt_flags);
		    *p = c1;		    /* put back for error messages */
		}
	    }

	    /*
	     * ":let @r = expr": Set register contents.
	     */
	    else if (*arg == '@')
	    {
		++arg;
		if (*skipwhite(arg + 1) != '=')
		    EMSG(_(e_letunexp));
		else
		    write_reg_contents(*arg == '@' ? '"' : *arg,
					  get_var_string(&retvar), -1, FALSE);
	    }

	    /*
	     * ":let var = expr": Set internal variable.
	     */
	    else if (eval_isnamec(*arg) && !VIM_ISDIGIT(*arg))
	    {
		/* Find the end of the name. */
		p = find_name_end(arg, &expr_start, &expr_end);

		if (*skipwhite(p) != '=')
		    EMSG(_(e_letunexp));
		else if (p - arg == 13
				    && STRNCMP(arg, "b:changedtick", 13) == 0)
		    EMSG2(_(e_readonlyvar), arg);
#ifdef FEAT_MAGIC_BRACES
		else if (expr_start != NULL)
		{
		    char_u  *temp_string;

		    temp_string = make_expanded_name(arg, expr_start,
								 expr_end, p);
		    if (temp_string == NULL)
		    {
			/*
			 * Report an invalid expression in braces, unless the
			 * expression evaluation has been cancelled due to an
			 * aborting error, an interrupt, or an exception.
			 */
			if (!aborting())
			    EMSG2(_(e_invarg2), arg);
		    }
		    else
		    {
			set_var(temp_string, &retvar);
			vim_free(temp_string);
		    }
		}
#endif
		else
		{
		    c1 = *p;
		    *p = NUL;
		    set_var(arg, &retvar);
		    *p = c1;		/* put char back for error messages */
		}
	    }

	    else
	    {
		EMSG2(_(e_invarg2), arg);
	    }

	    clear_var(&retvar);
	}
    }
}

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)

    void
set_context_for_expression(xp, arg, cmdidx)
    expand_T	*xp;
    char_u	*arg;
    cmdidx_T	cmdidx;
{
    int		got_eq = FALSE;
    int		c;

    xp->xp_context = cmdidx == CMD_let ? EXPAND_USER_VARS
				       : cmdidx == CMD_call ? EXPAND_FUNCTIONS
				       : EXPAND_EXPRESSION;
    while ((xp->xp_pattern = vim_strpbrk(arg,
				  (char_u *)"\"'+-*/%.=!?~|&$([<>,#")) != NULL)
    {
	c = *xp->xp_pattern;
	if (c == '&')
	{
	    c = xp->xp_pattern[1];
	    if (c == '&')
	    {
		++xp->xp_pattern;
		xp->xp_context = cmdidx != CMD_let || got_eq
					 ? EXPAND_EXPRESSION : EXPAND_NOTHING;
	    }
	    else if (c != ' ')
		xp->xp_context = EXPAND_SETTINGS;
	}
	else if (c == '$')
	{
	    /* environment variable */
	    xp->xp_context = EXPAND_ENV_VARS;
	}
	else if (c == '=')
	{
	    got_eq = TRUE;
	    xp->xp_context = EXPAND_EXPRESSION;
	}
	else if (c == '<'
		&& xp->xp_context == EXPAND_FUNCTIONS
		&& vim_strchr(xp->xp_pattern, '(') == NULL)
	{
	    /* Function name can start with "<SNR>" */
	    break;
	}
	else if (cmdidx != CMD_let || got_eq)
	{
	    if (c == '"')	    /* string */
	    {
		while ((c = *++xp->xp_pattern) != NUL && c != '"')
		    if (c == '\\' && xp->xp_pattern[1] != NUL)
			++xp->xp_pattern;
		xp->xp_context = EXPAND_NOTHING;
	    }
	    else if (c == '\'')	    /* literal string */
	    {
		while ((c = *++xp->xp_pattern) != NUL && c != '\'')
		    /* skip */ ;
		xp->xp_context = EXPAND_NOTHING;
	    }
	    else if (c == '|')
	    {
		if (xp->xp_pattern[1] == '|')
		{
		    ++xp->xp_pattern;
		    xp->xp_context = EXPAND_EXPRESSION;
		}
		else
		    xp->xp_context = EXPAND_COMMANDS;
	    }
	    else
		xp->xp_context = EXPAND_EXPRESSION;
	}
	else
	    xp->xp_context = EXPAND_NOTHING;
	arg = xp->xp_pattern;
	if (*arg != NUL)
	    while ((c = *++arg) != NUL && (c == ' ' || c == '\t'))
		/* skip */ ;
    }
    xp->xp_pattern = arg;
}

#endif /* FEAT_CMDL_COMPL */

/*
 * ":1,25call func(arg1, arg2)"	function call.
 */
    void
ex_call(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    char_u	*startarg;
    char_u	*alias;
    char_u	*name;
    var		retvar;
    int		len;
    linenr_T	lnum;
    int		doesrange;
    int		failed = FALSE;

    name = arg;
    len = get_func_len(&arg, &alias, !eap->skip);
    if (len == 0)
	goto end;
    if (alias != NULL)
	name = alias;

    startarg = arg;
    retvar.var_type = VAR_UNKNOWN;	/* clear_var() uses this */

    if (*startarg != '(')
    {
	EMSG2(_("E107: Missing braces: %s"), name);
	goto end;
    }

    /*
     * When skipping, evaluate the function once, to find the end of the
     * arguments.
     * When the function takes a range, this is discovered after the first
     * call, and the loop is broken.
     */
    if (eap->skip)
    {
	++emsg_skip;
	lnum = eap->line2;	/* do it once, also with an invalid range */
    }
    else
	lnum = eap->line1;
    for ( ; lnum <= eap->line2; ++lnum)
    {
	if (!eap->skip && eap->addr_count > 0)
	{
	    curwin->w_cursor.lnum = lnum;
	    curwin->w_cursor.col = 0;
	}
	arg = startarg;
	if (get_func_var(name, len, &retvar, &arg,
		      eap->line1, eap->line2, &doesrange, !eap->skip) == FAIL)
	{
	    failed = TRUE;
	    break;
	}
	clear_var(&retvar);
	if (doesrange || eap->skip)
	    break;
	/* Stop when immediately aborting on error, or when an interrupt
	 * occurred or an exception was thrown but not caught.  get_func_var()
	 * returned OK, so that the check for trailing characters below is
	 * executed. */
	if (aborting())
	    break;
    }
    if (eap->skip)
	--emsg_skip;

    if (!failed)
    {
	/* Check for trailing illegal characters and a following command. */
	if (!ends_excmd(*arg))
	{
	    emsg_severe = TRUE;
	    EMSG(_(e_trailing));
	}
	else
	    eap->nextcmd = check_nextcmd(arg);
    }

end:
    if (alias != NULL)
	vim_free(alias);
}

/*
 * ":unlet[!] var1 ... " command.
 */
    void
ex_unlet(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    char_u	*name_end;
    char_u	cc;
    char_u	*expr_start;
    char_u	*expr_end;
    int		error = FALSE;

    do
    {
	/* Find the end of the name. */
	name_end = find_name_end(arg, &expr_start, &expr_end);

	if (!vim_iswhite(*name_end) && !ends_excmd(*name_end))
	{
	    emsg_severe = TRUE;
	    EMSG(_(e_trailing));
	    break;
	}

	if (!error && !eap->skip)
	{
#ifdef FEAT_MAGIC_BRACES
	    if (expr_start != NULL)
	    {
		char_u  *temp_string;

		temp_string = make_expanded_name(arg, expr_start,
							 expr_end, name_end);
		if (temp_string == NULL)
		{
		    /*
		     * Report an invalid expression in braces, unless the
		     * expression evaluation has been cancelled due to an
		     * aborting error, an interrupt, or an exception.
		     */
		    if (!aborting())
		    {
			emsg_severe = TRUE;
			EMSG2(_(e_invarg2), arg);
			break;
		    }
		    error = TRUE;
		}
		else
		{
		    if (do_unlet(temp_string) == FAIL && !eap->forceit)
		    {
			EMSG2(_("E108: No such variable: \"%s\""), temp_string);
			error = TRUE;
		    }
		    vim_free(temp_string);
		}
	    }
	    else
#endif
	    {
		cc = *name_end;
		*name_end = NUL;

		if (do_unlet(arg) == FAIL && !eap->forceit)
		{
		    EMSG2(_("E108: No such variable: \"%s\""), arg);
		    error = TRUE;
		}

		*name_end = cc;
	    }
	}
	arg = skipwhite(name_end);
    } while (!ends_excmd(*arg));

    eap->nextcmd = check_nextcmd(arg);
}

/*
 * "unlet" a variable.  Return OK if it existed, FAIL if not.
 */
    int
do_unlet(name)
    char_u	*name;
{
    VAR		v;

    v = find_var(name, TRUE);
    if (v != NULL)
    {
	var_free_one(v);
	return OK;
    }
    return FAIL;
}

#if (defined(FEAT_MENU) && defined(FEAT_MULTI_LANG)) || defined(PROTO)
/*
 * Delete all "menutrans_" variables.
 */
    void
del_menutrans_vars()
{
    int		i;

    for (i = 0; i < variables.ga_len; ++i)
	if (VAR_ENTRY(i).var_name != NULL
		&& STRNCMP(VAR_ENTRY(i).var_name, "menutrans_", 10) == 0)
	    var_free_one(&VAR_ENTRY(i));
}
#endif

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)

/*
 * Local string buffer for the next two functions to store a variable name
 * with its prefix. Allocated in cat_prefix_varname(), freed later in
 * get_user_var_name().
 */

static char_u *cat_prefix_varname __ARGS((int prefix, char_u *name));

static char_u	*varnamebuf = NULL;
static int	varnamebuflen = 0;

/*
 * Function to concatenate a prefix and a variable name.
 */
    static char_u *
cat_prefix_varname(prefix, name)
    int		prefix;
    char_u	*name;
{
    int		len;

    len = (int)STRLEN(name) + 3;
    if (len > varnamebuflen)
    {
	vim_free(varnamebuf);
	len += 10;			/* some additional space */
	varnamebuf = alloc(len);
	if (varnamebuf == NULL)
	{
	    varnamebuflen = 0;
	    return NULL;
	}
	varnamebuflen = len;
    }
    *varnamebuf = prefix;
    varnamebuf[1] = ':';
    STRCPY(varnamebuf + 2, name);
    return varnamebuf;
}

/*
 * Function given to ExpandGeneric() to obtain the list of user defined
 * (global/buffer/window/built-in) variable names.
 */
/*ARGSUSED*/
    char_u *
get_user_var_name(xp, idx)
    expand_T	*xp;
    int		idx;
{
    static int	gidx;
    static int	bidx;
    static int	widx;
    static int	vidx;
    char_u	*name;

    if (idx == 0)
	gidx = bidx = widx = vidx = 0;
    if (gidx < variables.ga_len)			/* Global variables */
    {
	while ((name = VAR_ENTRY(gidx++).var_name) == NULL
		&& gidx < variables.ga_len)
	    /* skip */;
	if (name != NULL)
	{
	    if (STRNCMP("g:", xp->xp_pattern, 2) == 0)
		return cat_prefix_varname('g', name);
	    else
		return name;
	}
    }
    if (bidx < curbuf->b_vars.ga_len)		/* Current buffer variables */
    {
	while ((name = BVAR_ENTRY(bidx++).var_name) == NULL
		&& bidx < curbuf->b_vars.ga_len)
	    /* skip */;
	if (name != NULL)
	    return cat_prefix_varname('b', name);
    }
    if (bidx == curbuf->b_vars.ga_len)
    {
	++bidx;
	return (char_u *)"b:changedtick";
    }
    if (widx < curwin->w_vars.ga_len)		/* Current window variables */
    {
	while ((name = WVAR_ENTRY(widx++).var_name) == NULL
		&& widx < curwin->w_vars.ga_len)
	    /* skip */;
	if (name != NULL)
	    return cat_prefix_varname('w', name);
    }
    if (vidx < VV_LEN)				      /* Built-in variables */
	return cat_prefix_varname('v', (char_u *)vimvars[vidx++].name);

    vim_free(varnamebuf);
    varnamebuf = NULL;
    varnamebuflen = 0;
    return NULL;
}

#endif /* FEAT_CMDL_COMPL */

/*
 * types for expressions.
 */
typedef enum
{
    TYPE_UNKNOWN = 0
    , TYPE_EQUAL	/* == */
    , TYPE_NEQUAL	/* != */
    , TYPE_GREATER	/* >  */
    , TYPE_GEQUAL	/* >= */
    , TYPE_SMALLER	/* <  */
    , TYPE_SEQUAL	/* <= */
    , TYPE_MATCH	/* =~ */
    , TYPE_NOMATCH	/* !~ */
} exptype_T;

/*
 * The "evaluate" argument: When FALSE, the argument is only parsed but not
 * executed.  The function may return OK, but the retvar will be of type
 * VAR_UNKNOWN.  The function still returns FAIL for a syntax error.
 */

/*
 * Handle zero level expression.
 * This calls eval1() and handles error message and nextcmd.
 * Return OK or FAIL.
 */
    static int
eval0(arg, retvar, nextcmd, evaluate)
    char_u	*arg;
    VAR		retvar;
    char_u	**nextcmd;
    int		evaluate;
{
    int		ret;
    char_u	*p;

    p = skipwhite(arg);
    ret = eval1(&p, retvar, evaluate);
    if (ret == FAIL || !ends_excmd(*p))
    {
	if (ret != FAIL)
	    clear_var(retvar);
	/*
	 * Report the invalid expression unless the expression evaluation has
	 * been cancelled due to an aborting error, an interrupt, or an
	 * exception.
	 */
	if (!aborting())
	    EMSG2(_(e_invexpr2), arg);
	ret = FAIL;
    }
    if (nextcmd != NULL)
	*nextcmd = check_nextcmd(p);

    return ret;
}

/*
 * Handle top level expression:
 *	expr1 ? expr0 : expr0
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval1(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    int		result;
    var		var2;

    /*
     * Get the first variable.
     */
    if (eval2(arg, retvar, evaluate) == FAIL)
	return FAIL;

    if ((*arg)[0] == '?')
    {
	result = FALSE;
	if (evaluate)
	{
	    if (get_var_number(retvar) != 0)
		result = TRUE;
	    clear_var(retvar);
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval1(arg, retvar, evaluate && result) == FAIL) /* recursive! */
	    return FAIL;

	/*
	 * Check for the ":".
	 */
	if ((*arg)[0] != ':')
	{
	    EMSG(_("E109: Missing ':' after '?'"));
	    if (evaluate && result)
		clear_var(retvar);
	    return FAIL;
	}

	/*
	 * Get the third variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval1(arg, &var2, evaluate && !result) == FAIL) /* recursive! */
	{
	    if (evaluate && result)
		clear_var(retvar);
	    return FAIL;
	}
	if (evaluate && !result)
	    *retvar = var2;
    }

    return OK;
}

/*
 * Handle first level expression:
 *	expr2 || expr2 || expr2	    logical OR
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval2(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    long	result;
    int		first;

    /*
     * Get the first variable.
     */
    if (eval3(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat until there is no following "||".
     */
    first = TRUE;
    result = FALSE;
    while ((*arg)[0] == '|' && (*arg)[1] == '|')
    {
	if (evaluate && first)
	{
	    if (get_var_number(retvar) != 0)
		result = TRUE;
	    clear_var(retvar);
	    first = FALSE;
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 2);
	if (eval3(arg, &var2, evaluate && !result) == FAIL)
	    return FAIL;

	/*
	 * Compute the result.
	 */
	if (evaluate && !result)
	{
	    if (get_var_number(&var2) != 0)
		result = TRUE;
	    clear_var(&var2);
	}
	if (evaluate)
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = result;
	}
    }

    return OK;
}

/*
 * Handle second level expression:
 *	expr3 && expr3 && expr3	    logical AND
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval3(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    long	result;
    int		first;

    /*
     * Get the first variable.
     */
    if (eval4(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat until there is no following "&&".
     */
    first = TRUE;
    result = TRUE;
    while ((*arg)[0] == '&' && (*arg)[1] == '&')
    {
	if (evaluate && first)
	{
	    if (get_var_number(retvar) == 0)
		result = FALSE;
	    clear_var(retvar);
	    first = FALSE;
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 2);
	if (eval4(arg, &var2, evaluate && result) == FAIL)
	    return FAIL;

	/*
	 * Compute the result.
	 */
	if (evaluate && result)
	{
	    if (get_var_number(&var2) == 0)
		result = FALSE;
	    clear_var(&var2);
	}
	if (evaluate)
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = result;
	}
    }

    return OK;
}

/*
 * Handle third level expression:
 *	var1 == var2
 *	var1 =~ var2
 *	var1 != var2
 *	var1 !~ var2
 *	var1 > var2
 *	var1 >= var2
 *	var1 < var2
 *	var1 <= var2
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval4(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    char_u	*p;
    int		i;
    exptype_T	type = TYPE_UNKNOWN;
    int		len = 2;
    long	n1, n2;
    char_u	*s1, *s2;
    char_u	buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    regmatch_T	regmatch;
    int		ic;
    char_u	*save_cpo;

    /*
     * Get the first variable.
     */
    if (eval5(arg, retvar, evaluate) == FAIL)
	return FAIL;

    p = *arg;
    switch (p[0])
    {
	case '=':   if (p[1] == '=')
			type = TYPE_EQUAL;
		    else if (p[1] == '~')
			type = TYPE_MATCH;
		    break;
	case '!':   if (p[1] == '=')
			type = TYPE_NEQUAL;
		    else if (p[1] == '~')
			type = TYPE_NOMATCH;
		    break;
	case '>':   if (p[1] != '=')
		    {
			type = TYPE_GREATER;
			len = 1;
		    }
		    else
			type = TYPE_GEQUAL;
		    break;
	case '<':   if (p[1] != '=')
		    {
			type = TYPE_SMALLER;
			len = 1;
		    }
		    else
			type = TYPE_SEQUAL;
		    break;
    }

    /*
     * If there is a comparitive operator, use it.
     */
    if (type != TYPE_UNKNOWN)
    {
	/* extra question mark appended: ignore case */
	if (p[len] == '?')
	{
	    ic = TRUE;
	    ++len;
	}
	/* extra '#' appended: match case */
	else if (p[len] == '#')
	{
	    ic = FALSE;
	    ++len;
	}
	/* nothing appened: use 'ignorecase' */
	else
	    ic = p_ic;

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(p + len);
	if (eval5(arg, &var2, evaluate) == FAIL)
	{
	    clear_var(retvar);
	    return FAIL;
	}

	if (evaluate)
	{
	    /*
	     * If one of the two variables is a number, compare as a number.
	     * When using "=~" or "!~", always compare as string.
	     */
	    if ((retvar->var_type == VAR_NUMBER || var2.var_type == VAR_NUMBER)
		    && type != TYPE_MATCH && type != TYPE_NOMATCH)
	    {
		n1 = get_var_number(retvar);
		n2 = get_var_number(&var2);
		switch (type)
		{
		    case TYPE_EQUAL:    n1 = (n1 == n2); break;
		    case TYPE_NEQUAL:   n1 = (n1 != n2); break;
		    case TYPE_GREATER:  n1 = (n1 > n2); break;
		    case TYPE_GEQUAL:   n1 = (n1 >= n2); break;
		    case TYPE_SMALLER:  n1 = (n1 < n2); break;
		    case TYPE_SEQUAL:   n1 = (n1 <= n2); break;
		    case TYPE_UNKNOWN:
		    case TYPE_MATCH:
		    case TYPE_NOMATCH:  break;  /* avoid gcc warning */
		}
	    }
	    else
	    {
		s1 = get_var_string_buf(retvar, buf1);
		s2 = get_var_string_buf(&var2, buf2);
		if (type != TYPE_MATCH && type != TYPE_NOMATCH)
		    i = ic ? MB_STRICMP(s1, s2) : STRCMP(s1, s2);
		else
		    i = 0;
		n1 = FALSE;
		switch (type)
		{
		    case TYPE_EQUAL:    n1 = (i == 0); break;
		    case TYPE_NEQUAL:   n1 = (i != 0); break;
		    case TYPE_GREATER:  n1 = (i > 0); break;
		    case TYPE_GEQUAL:   n1 = (i >= 0); break;
		    case TYPE_SMALLER:  n1 = (i < 0); break;
		    case TYPE_SEQUAL:   n1 = (i <= 0); break;

		    case TYPE_MATCH:
		    case TYPE_NOMATCH:
			    /* avoid 'l' flag in 'cpoptions' */
			    save_cpo = p_cpo;
			    p_cpo = (char_u *)"";
			    regmatch.regprog = vim_regcomp(s2,
							RE_MAGIC + RE_STRING);
			    regmatch.rm_ic = ic;
			    if (regmatch.regprog != NULL)
			    {
				n1 = vim_regexec_nl(&regmatch, s1, (colnr_T)0);
				vim_free(regmatch.regprog);
				if (type == TYPE_NOMATCH)
				    n1 = !n1;
			    }
			    p_cpo = save_cpo;
			    break;

		    case TYPE_UNKNOWN:  break;  /* avoid gcc warning */
		}
	    }
	    clear_var(retvar);
	    clear_var(&var2);
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = n1;
	}
    }

    return OK;
}

/*
 * Handle fourth level expression:
 *	+	number addition
 *	-	number subtraction
 *	.	string concatenation
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval5(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    int		op;
    long	n1, n2;
    char_u	*s1, *s2;
    char_u	buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    char_u	*p;

    /*
     * Get the first variable.
     */
    if (eval6(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no '+', '-' or '.' is following.
     */
    for (;;)
    {
	op = **arg;
	if (op != '+' && op != '-' && op != '.')
	    break;

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval6(arg, &var2, evaluate) == FAIL)
	{
	    clear_var(retvar);
	    return FAIL;
	}

	if (evaluate)
	{
	    /*
	     * Compute the result.
	     */
	    if (op == '.')
	    {
		s1 = get_var_string_buf(retvar, buf1);
		s2 = get_var_string_buf(&var2, buf2);
		op = (int)STRLEN(s1);
		p = alloc((unsigned)(op + STRLEN(s2) + 1));
		if (p != NULL)
		{
		    STRCPY(p, s1);
		    STRCPY(p + op, s2);
		}
		clear_var(retvar);
		retvar->var_type = VAR_STRING;
		retvar->var_val.var_string = p;
	    }
	    else
	    {
		n1 = get_var_number(retvar);
		n2 = get_var_number(&var2);
		clear_var(retvar);
		if (op == '+')
		    n1 = n1 + n2;
		else
		    n1 = n1 - n2;
		retvar->var_type = VAR_NUMBER;
		retvar->var_val.var_number = n1;
	    }
	    clear_var(&var2);
	}
    }
    return OK;
}

/*
 * Handle fifth level expression:
 *	*	number multiplication
 *	/	number division
 *	%	number modulo
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval6(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    int		op;
    long	n1, n2;

    /*
     * Get the first variable.
     */
    if (eval7(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no '*', '/' or '%' is following.
     */
    for (;;)
    {
	op = **arg;
	if (op != '*' && op != '/' && op != '%')
	    break;

	if (evaluate)
	{
	    n1 = get_var_number(retvar);
	    clear_var(retvar);
	}
	else
	    n1 = 0;

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval7(arg, &var2, evaluate) == FAIL)
	    return FAIL;

	if (evaluate)
	{
	    n2 = get_var_number(&var2);
	    clear_var(&var2);

	    /*
	     * Compute the result.
	     */
	    if (op == '*')
		n1 = n1 * n2;
	    else if (op == '/')
	    {
		if (n2 == 0)	/* give an error message? */
		    n1 = 0x7fffffffL;
		else
		    n1 = n1 / n2;
	    }
	    else
	    {
		if (n2 == 0)	/* give an error message? */
		    n1 = 0;
		else
		    n1 = n1 % n2;
	    }
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = n1;
	}
    }

    return OK;
}

/*
 * Handle sixth level expression:
 *  number		number constant
 *  "string"		string contstant
 *  'string'		literal string contstant
 *  &option-name	option value
 *  @r			register contents
 *  identifier		variable value
 *  function()		function call
 *  $VAR		environment variable
 *  (expression)	nested expression
 *
 *  Also handle:
 *  ! in front		logical NOT
 *  - in front		unary minus
 *  + in front		unary plus (ignored)
 *  trailing []		subscript in String
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval7(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    long	n;
    int		len;
    char_u	*s;
    int		val;
    char_u	*start_leader, *end_leader;
    int		ret = OK;
    char_u	*alias;

    /*
     * Initialise variable so that clear_var() can't mistake this for a string
     * and free a string that isn't there.
     */
    retvar->var_type = VAR_UNKNOWN;

    /*
     * Skip '!' and '-' characters.  They are handled later.
     */
    start_leader = *arg;
    while (**arg == '!' || **arg == '-' || **arg == '+')
	*arg = skipwhite(*arg + 1);
    end_leader = *arg;

    switch (**arg)
    {
    /*
     * Number constant.
     */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
		vim_str2nr(*arg, NULL, &len, TRUE, TRUE, &n, NULL);
		*arg += len;
		if (evaluate)
		{
		    retvar->var_type = VAR_NUMBER;
		    retvar->var_val.var_number = n;
		}
		break;

    /*
     * String constant: "string".
     */
    case '"':	ret = get_string_var(arg, retvar, evaluate);
		break;

    /*
     * Literal string constant: 'string'.
     */
    case '\'':	ret = get_lit_string_var(arg, retvar, evaluate);
		break;

    /*
     * Option value: &name
     */
    case '&':	ret = get_option_var(arg, retvar, evaluate);
		break;

    /*
     * Environment variable: $VAR.
     */
    case '$':	ret = get_env_var(arg, retvar, evaluate);
		break;

    /*
     * Register contents: @r.
     */
    case '@':	++*arg;
		if (evaluate)
		{
		    retvar->var_type = VAR_STRING;
		    retvar->var_val.var_string = get_reg_contents(**arg, FALSE);
		}
		if (**arg != NUL)
		    ++*arg;
		break;

    /*
     * nested expression: (expression).
     */
    case '(':	*arg = skipwhite(*arg + 1);
		ret = eval1(arg, retvar, evaluate);	/* recursive! */
		if (**arg == ')')
		    ++*arg;
		else if (ret == OK)
		{
		    EMSG(_("E110: Missing ')'"));
		    clear_var(retvar);
		    ret = FAIL;
		}
		break;

    /*
     * Must be a variable or function name then.
     */
    default:	s = *arg;
		len = get_func_len(arg, &alias, evaluate);
		if (alias != NULL)
		    s = alias;

		if (len == 0)
		    ret = FAIL;
		else
		{
		    if (**arg == '(')		/* recursive! */
		    {
			ret = get_func_var(s, len, retvar, arg,
				  curwin->w_cursor.lnum, curwin->w_cursor.lnum,
				  &len, evaluate);
			/* Stop the expression evaluation when immediately
			 * aborting on error, or when an interrupt occurred or
			 * an exception was thrown but not caught. */
			if (aborting())
			{
			    if (ret == OK)
				clear_var(retvar);
			    ret = FAIL;
			}
		    }
		    else if (evaluate)
			ret = get_var_var(s, len, retvar);
		}

		if (alias != NULL)
		    vim_free(alias);

		break;
    }
    *arg = skipwhite(*arg);

    /*
     * Handle expr[expr] subscript.
     */
    if (**arg == '[' && ret == OK)
    {
	/*
	 * Get the variable from inside the [].
	 */
	*arg = skipwhite(*arg + 1);
	if (eval1(arg, &var2, evaluate) == FAIL)	/* recursive! */
	{
	    clear_var(retvar);
	    return FAIL;
	}

	/* Check for the ']'. */
	if (**arg != ']')
	{
	    EMSG(_("E111: Missing ']'"));
	    clear_var(retvar);
	    clear_var(&var2);
	    return FAIL;
	}

	if (evaluate)
	{
	    n = get_var_number(&var2);
	    clear_var(&var2);

	    /*
	     * The resulting variable is a string of a single character.
	     * If the index is too big or negative, the result is empty.
	     */
	    s = get_var_string(retvar);
	    if (n >= (long)STRLEN(s) || n < 0)
		s = NULL;
	    else
		s = vim_strnsave(s + n, 1);
	    clear_var(retvar);
	    retvar->var_type = VAR_STRING;
	    retvar->var_val.var_string = s;
	}
	*arg = skipwhite(*arg + 1);	/* skip the ']' */
    }

    /*
     * Apply logical NOT and unary '-', from right to left, ignore '+'.
     */
    if (ret == OK && evaluate && end_leader > start_leader)
    {
	val = get_var_number(retvar);
	while (end_leader > start_leader)
	{
	    --end_leader;
	    if (*end_leader == '!')
		val = !val;
	    else if (*end_leader == '-')
		val = -val;
	}
	clear_var(retvar);
	retvar->var_type = VAR_NUMBER;
	retvar->var_val.var_number = val;
    }

    return ret;
}

/*
 * Get an option value.
 * "arg" points to the '&' or '+' before the option name.
 * "arg" is advanced to character after the option name.
 * Return OK or FAIL.
 */
    static int
get_option_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;		/* when NULL, only check if option exists */
    int		evaluate;
{
    char_u	*option_end;
    long	numval;
    char_u	*stringval;
    int		opt_type;
    int		c;
    int		working = (**arg == '+');    /* has("+option") */
    int		ret = OK;
    int		opt_flags;

    /*
     * Isolate the option name and find its value.
     */
    option_end = find_option_end(arg, &opt_flags);
    if (option_end == NULL)
    {
	if (retvar != NULL)
	    EMSG2(_("E112: Option name missing: %s"), *arg);
	return FAIL;
    }

    if (!evaluate)
    {
	*arg = option_end;
	return OK;
    }

    c = *option_end;
    *option_end = NUL;
    opt_type = get_option_value(*arg, &numval,
			       retvar == NULL ? NULL : &stringval, opt_flags);

    if (opt_type == -3)			/* invalid name */
    {
	if (retvar != NULL)
	    EMSG2(_("E113: Unknown option: %s"), *arg);
	ret = FAIL;
    }
    else if (retvar != NULL)
    {
	if (opt_type == -2)		/* hidden string option */
	{
	    retvar->var_type = VAR_STRING;
	    retvar->var_val.var_string = NULL;
	}
	else if (opt_type == -1)	/* hidden number option */
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = 0;
	}
	else if (opt_type == 1)		/* number option */
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = numval;
	}
	else				/* string option */
	{
	    retvar->var_type = VAR_STRING;
	    retvar->var_val.var_string = stringval;
	}
    }
    else if (working && (opt_type == -2 || opt_type == -1))
	ret = FAIL;

    *option_end = c;		    /* put back for error messages */
    *arg = option_end;

    return ret;
}

/*
 * Allocate a variable for a string constant.
 * Return OK or FAIL.
 */
    static int
get_string_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    char_u	*p;
    char_u	*name;
    int		i;
    int		extra = 0;

    /*
     * Find the end of the string, skipping backslashed characters.
     */
    for (p = *arg + 1; *p && *p != '"'; ++p)
    {
	if (*p == '\\' && p[1] != NUL)
	{
	    ++p;
	    /* A "\<x>" form occupies at least 4 characters, and produces up
	     * to 6 characters: reserve space for 2 extra */
	    if (*p == '<')
		extra += 2;
	}
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    p += (*mb_ptr2len_check)(p) - 1;
#endif
    }

    if (*p != '"')
    {
	EMSG2(_("E114: Missing quote: %s"), *arg);
	return FAIL;
    }

    /* If only parsing, set *arg and return here */
    if (!evaluate)
    {
	*arg = p + 1;
	return OK;
    }

    /*
     * Copy the string into allocated memory, handling backslashed
     * characters.
     */
    name = alloc((unsigned)(p - *arg + extra));
    if (name == NULL)
	return FAIL;

    i = 0;
    for (p = *arg + 1; *p && *p != '"'; ++p)
    {
	if (*p == '\\')
	{
	    switch (*++p)
	    {
		case 'b': name[i++] = BS; break;
		case 'e': name[i++] = ESC; break;
		case 'f': name[i++] = FF; break;
		case 'n': name[i++] = NL; break;
		case 'r': name[i++] = CAR; break;
		case 't': name[i++] = TAB; break;

		case 'X': /* hex: "\x1", "\x12" */
		case 'x':
		case 'u': /* Unicode: "\u0023" */
		case 'U':
			  if (vim_isxdigit(p[1]))
			  {
			      int	n, nr;
			      int	c = toupper(*p);

			      if (c == 'X')
				  n = 2;
			      else
				  n = 4;
			      nr = 0;
			      while (--n >= 0 && vim_isxdigit(p[1]))
			      {
				  ++p;
				  nr = (nr << 4) + hex2nr(*p);
			      }
#ifdef FEAT_MBYTE
			      /* For "\u" store the number according to
			       * 'encoding'. */
			      if (c != 'X')
				  i += (*mb_char2bytes)(nr, name + i);
			      else
#endif
				  name[i++] = nr;
			  }
			  else
			      name[i++] = *p;
			  break;

			  /* octal: "\1", "\12", "\123" */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7': name[i] = *p - '0';
			  if (p[1] >= '0' && p[1] <= '7')
			  {
			      ++p;
			      name[i] = (name[i] << 3) + *p - '0';
			      if (p[1] >= '0' && p[1] <= '7')
			      {
				  ++p;
				  name[i] = (name[i] << 3) + *p - '0';
			      }
			  }
			  ++i;
			  break;

			    /* Special key, e.g.: "\<C-W>" */
		case '<': extra = trans_special(&p, name + i, TRUE);
			  if (extra != 0)
			  {
			      i += extra;
			      --p;
			      break;
			  }
			  /* FALLTHROUGH */

		default:  name[i++] = *p;
			  break;
	    }
	}
	else
	    name[i++] = *p;

#ifdef FEAT_MBYTE
	/* For a multi-byte character copy the bytes after the first one. */
	if (has_mbyte)
	{
	    int	l = (*mb_ptr2len_check)(p);

	    while (--l > 0)
		name[i++] = *++p;
	}
#endif
    }
    name[i] = NUL;
    *arg = p + 1;

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = name;

    return OK;
}

/*
 * Allocate a variable for an backtick-string constant.
 * Return OK or FAIL.
 */
    static int
get_lit_string_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    char_u	*p;
    char_u	*name;

    /*
     * Find the end of the string.
     */
    p = vim_strchr(*arg + 1, '\'');
    if (p == NULL)
    {
	EMSG2(_("E115: Missing quote: %s"), *arg);
	return FAIL;
    }

    if (evaluate)
    {
	/*
	 * Copy the string into allocated memory.
	 */
	name = vim_strnsave(*arg + 1, (int)(p - (*arg + 1)));
	if (name == NULL)
	    return FAIL;

	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = name;
    }

    *arg = p + 1;

    return OK;
}

/*
 * Get the value of an environment variable.
 * "arg" is pointing to the '$'.  It is advanced to after the name.
 * If the environment variable was not set, silently assume it is empty.
 * Always return OK.
 */
    static int
get_env_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    char_u	*string = NULL;
    int		len;
    int		cc;
    char_u	*name;

    ++*arg;
    name = *arg;
    len = get_env_len(arg);
    if (evaluate)
    {
	if (len != 0)
	{
	    cc = name[len];
	    name[len] = NUL;
	    /* first try mch_getenv(), fast for normal environment vars */
	    string = mch_getenv(name);
	    if (string != NULL && *string != NUL)
		string = vim_strsave(string);
	    else
	    {
		/* next try expanding things like $VIM and ${HOME} */
		string = expand_env_save(name - 1);
		if (string != NULL && *string == '$')
		{
		    vim_free(string);
		    string = NULL;
		}
	    }
	    name[len] = cc;
	}
	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = string;
    }

    return OK;
}

/*
 * Array with names and number of arguments of all internal functions
 * MUST BE KEPT SORTED IN strcmp() ORDER FOR BINARY SEARCH!
 */
static struct fst
{
    char	*f_name;	/* function name */
    char	f_min_argc;	/* minimal number of arguments */
    char	f_max_argc;	/* maximal number of arguments */
    void	(*f_func) __ARGS((VAR args, VAR rvar)); /* impl. function */
} functions[] =
{
    {"append",		2, 2, f_append},
    {"argc",		0, 0, f_argc},
    {"argidx",		0, 0, f_argidx},
    {"argv",		1, 1, f_argv},
    {"browse",		4, 4, f_browse},
    {"bufexists",	1, 1, f_bufexists},
    {"buffer_exists",	1, 1, f_bufexists},	/* obsolete */
    {"buffer_name",	1, 1, f_bufname},	/* obsolete */
    {"buffer_number",	1, 1, f_bufnr},		/* obsolete */
    {"buflisted",	1, 1, f_buflisted},
    {"bufloaded",	1, 1, f_bufloaded},
    {"bufname",		1, 1, f_bufname},
    {"bufnr",		1, 1, f_bufnr},
    {"bufwinnr",	1, 1, f_bufwinnr},
    {"byte2line",	1, 1, f_byte2line},
    {"char2nr",		1, 1, f_char2nr},
    {"cindent",		1, 1, f_cindent},
    {"col",		1, 1, f_col},
    {"confirm",		1, 4, f_confirm},
    {"cscope_connection",0,3, f_cscope_connection},
    {"cursor",		2, 2, f_cursor},
    {"delete",		1, 1, f_delete},
    {"did_filetype",	0, 0, f_did_filetype},
    {"escape",		2, 2, f_escape},
    {"eventhandler",	0, 0, f_eventhandler},
    {"executable",	1, 1, f_executable},
    {"exists",		1, 1, f_exists},
    {"expand",		1, 2, f_expand},
    {"file_readable",	1, 1, f_filereadable},	/* obsolete */
    {"filereadable",	1, 1, f_filereadable},
    {"filewritable",	1, 1, f_filewritable},
    {"fnamemodify",	2, 2, f_fnamemodify},
    {"foldclosed",	1, 1, f_foldclosed},
    {"foldclosedend",	1, 1, f_foldclosedend},
    {"foldlevel",	1, 1, f_foldlevel},
    {"foldtext",	0, 0, f_foldtext},
    {"foreground",	0, 0, f_foreground},
    {"getbufvar",	2, 2, f_getbufvar},
    {"getchar",		0, 1, f_getchar},
    {"getcharmod",	0, 0, f_getcharmod},
    {"getcmdline",	0, 0, f_getcmdline},
    {"getcmdpos",	0, 0, f_getcmdpos},
    {"getcwd",		0, 0, f_getcwd},
    {"getfsize",	1, 1, f_getfsize},
    {"getftime",	1, 1, f_getftime},
    {"getline",		1, 1, f_getline},
    {"getreg",		0, 1, f_getreg},
    {"getregtype",	0, 1, f_getregtype},
    {"getwinposx",	0, 0, f_getwinposx},
    {"getwinposy",	0, 0, f_getwinposy},
    {"getwinvar",	2, 2, f_getwinvar},
    {"glob",		1, 1, f_glob},
    {"globpath",	2, 2, f_globpath},
    {"has",		1, 1, f_has},
    {"hasmapto",	1, 2, f_hasmapto},
    {"highlightID",	1, 1, f_hlID},		/* obsolete */
    {"highlight_exists",1, 1, f_hlexists},	/* obsolete */
    {"histadd",		2, 2, f_histadd},
    {"histdel",		1, 2, f_histdel},
    {"histget",		1, 2, f_histget},
    {"histnr",		1, 1, f_histnr},
    {"hlID",		1, 1, f_hlID},
    {"hlexists",	1, 1, f_hlexists},
    {"hostname",	0, 0, f_hostname},
    {"iconv",		3, 3, f_iconv},
    {"indent",		1, 1, f_indent},
    {"input",		1, 2, f_input},
    {"inputdialog",	1, 3, f_inputdialog},
    {"inputrestore",	0, 0, f_inputrestore},
    {"inputsave",	0, 0, f_inputsave},
    {"inputsecret",	1, 2, f_inputsecret},
    {"isdirectory",	1, 1, f_isdirectory},
    {"last_buffer_nr",	0, 0, f_last_buffer_nr},/* obsolete */
    {"libcall",		3, 3, f_libcall},
    {"libcallnr",	3, 3, f_libcallnr},
    {"line",		1, 1, f_line},
    {"line2byte",	1, 1, f_line2byte},
    {"lispindent",	1, 1, f_lispindent},
    {"localtime",	0, 0, f_localtime},
    {"maparg",		1, 2, f_maparg},
    {"mapcheck",	1, 2, f_mapcheck},
    {"match",		2, 3, f_match},
    {"matchend",	2, 3, f_matchend},
    {"matchstr",	2, 3, f_matchstr},
    {"mode",		0, 0, f_mode},
    {"nextnonblank",	1, 1, f_nextnonblank},
    {"nr2char",		1, 1, f_nr2char},
    {"prevnonblank",	1, 1, f_prevnonblank},
    {"remote_expr",	2, 3, f_remote_expr},
    {"remote_foreground", 1, 1, f_remote_foreground},
    {"remote_peek",	1, 2, f_remote_peek},
    {"remote_read",	1, 1, f_remote_read},
    {"remote_send",	2, 3, f_remote_send},
    {"rename",		2, 2, f_rename},
    {"resolve",		1, 1, f_resolve},
    {"search",		1, 2, f_search},
    {"searchpair",	3, 5, f_searchpair},
    {"server2client",	2, 2, f_server2client},
    {"serverlist",	0, 0, f_serverlist},
    {"setbufvar",	3, 3, f_setbufvar},
    {"setcmdpos",	1, 1, f_setcmdpos},
    {"setline",		2, 2, f_setline},
    {"setreg",		2, 3, f_setreg},
    {"setwinvar",	3, 3, f_setwinvar},
    {"simplify",	1, 1, f_simplify},
#ifdef HAVE_STRFTIME
    {"strftime",	1, 2, f_strftime},
#endif
    {"stridx",		2, 2, f_stridx},
    {"strlen",		1, 1, f_strlen},
    {"strpart",		2, 3, f_strpart},
    {"strridx",		2, 2, f_strridx},
    {"strtrans",	1, 1, f_strtrans},
    {"submatch",	1, 1, f_submatch},
    {"substitute",	4, 4, f_substitute},
    {"synID",		3, 3, f_synID},
    {"synIDattr",	2, 3, f_synIDattr},
    {"synIDtrans",	1, 1, f_synIDtrans},
    {"system",		1, 1, f_system},
    {"tempname",	0, 0, f_tempname},
    {"tolower",		1, 1, f_tolower},
    {"toupper",		1, 1, f_toupper},
    {"type",		1, 1, f_type},
    {"virtcol",		1, 1, f_virtcol},
    {"visualmode",	0, 1, f_visualmode},
    {"winbufnr",	1, 1, f_winbufnr},
    {"wincol",		0, 0, f_wincol},
    {"winheight",	1, 1, f_winheight},
    {"winline",		0, 0, f_winline},
    {"winnr",		0, 0, f_winnr},
    {"winrestcmd",	0, 0, f_winrestcmd},
    {"winwidth",	1, 1, f_winwidth},
};

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)

/*
 * Function given to ExpandGeneric() to obtain the list of internal
 * or user defined function names.
 */
    char_u *
get_function_name(xp, idx)
    expand_T	*xp;
    int		idx;
{
    static int	intidx = -1;
    char_u	*name;

    if (idx == 0)
	intidx = -1;
    if (intidx < 0)
    {
	name = get_user_func_name(xp, idx);
	if (name != NULL)
	    return name;
    }
    if (++intidx < (int)(sizeof(functions) / sizeof(struct fst)))
    {
	STRCPY(IObuff, functions[intidx].f_name);
	STRCAT(IObuff, "(");
	if (functions[intidx].f_max_argc == 0)
	    STRCAT(IObuff, ")");
	return IObuff;
    }

    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the list of internal or
 * user defined variable or function names.
 */
/*ARGSUSED*/
    char_u *
get_expr_name(xp, idx)
    expand_T	*xp;
    int		idx;
{
    static int	intidx = -1;
    char_u	*name;

    if (idx == 0)
	intidx = -1;
    if (intidx < 0)
    {
	name = get_function_name(xp, idx);
	if (name != NULL)
	    return name;
    }
    return get_user_var_name(xp, ++intidx);
}

#endif /* FEAT_CMDL_COMPL */

/*
 * Find internal function in table above.
 * Return index, or -1 if not found
 */
    static int
find_internal_func(name)
    char_u	*name;		/* name of the function */
{
    int		first = 0;
    int		last = (int)(sizeof(functions) / sizeof(struct fst)) - 1;
    int		cmp;
    int		x;

    /*
     * Find the function name in the table. Binary search.
     */
    while (first <= last)
    {
	x = first + ((unsigned)(last - first) >> 1);
	cmp = STRCMP(name, functions[x].f_name);
	if (cmp < 0)
	    last = x - 1;
	else if (cmp > 0)
	    first = x + 1;
	else
	    return x;
    }
    return -1;
}

/*
 * Allocate a variable for the result of a function.
 * Return OK or FAIL.
 */
    static int
get_func_var(name, len, retvar, arg, firstline, lastline, doesrange, evaluate)
    char_u	*name;		/* name of the function */
    int		len;		/* length of "name" */
    VAR		retvar;
    char_u	**arg;		/* argument, pointing to the '(' */
    linenr_T	firstline;	/* first line of range */
    linenr_T	lastline;	/* last line of range */
    int		*doesrange;	/* return: function handled range */
    int		evaluate;
{
    char_u	*argp;
    int		ret = OK;
#define MAX_FUNC_ARGS	20
    var		argvars[MAX_FUNC_ARGS];	/* vars for arguments */
    int		argcount = 0;		/* number of arguments found */

    /*
     * Get the arguments.
     */
    argp = *arg;
    while (argcount < MAX_FUNC_ARGS)
    {
	argp = skipwhite(argp + 1);	    /* skip the '(' or ',' */
	if (*argp == ')' || *argp == ',' || *argp == NUL)
	    break;
	argvars[argcount].var_name = NULL;  /* the name is not stored */
	if (eval1(&argp, &argvars[argcount], evaluate) == FAIL)
	{
	    ret = FAIL;
	    break;
	}
	++argcount;
	if (*argp != ',')
	    break;
    }
    if (*argp == ')')
	++argp;
    else
	ret = FAIL;

    if (ret == OK)
	ret = call_func(name, len, retvar, argcount, argvars,
				    firstline, lastline, doesrange, evaluate);
    else if (!aborting())
	EMSG2(_("E116: Invalid arguments for function %s"), name);

    while (--argcount >= 0)
	clear_var(&argvars[argcount]);

    *arg = skipwhite(argp);
    return ret;
}


/*
 * Call a function with its resolved parameters
 * Return OK or FAIL.
 */
    static int
call_func(name, len, retvar, argcount, argvars, firstline, lastline,
							  doesrange, evaluate)
    char_u	*name;		/* name of the function */
    int		len;		/* length of "name" */
    VAR		retvar;		/* return value goes here */
    int		argcount;	/* number of "argvars" */
    VAR		argvars;	/* vars for arguments */
    linenr_T	firstline;	/* first line of range */
    linenr_T	lastline;	/* last line of range */
    int		*doesrange;	/* return: function handled range */
    int		evaluate;
{
    int		ret = FAIL;
    static char *errors[] =
		{N_("E117: Unknown function: %s"),
		 N_("E118: Too many arguments for function: %s"),
		 N_("E119: Not enough arguments for function: %s"),
		 N_("E120: Using <SID> not in a script context: %s"),
		};
#define ERROR_UNKNOWN	0
#define ERROR_TOOMANY	1
#define ERROR_TOOFEW	2
#define ERROR_SCRIPT	3
#define ERROR_NONE	4
#define ERROR_OTHER	5
    int		error = ERROR_NONE;
    int		i;
    int		llen;
    ufunc_T	*fp;
    int		cc;
#define FLEN_FIXED 40
    char_u	fname_buf[FLEN_FIXED + 1];
    char_u	*fname;

    /*
     * In a script change <SID>name() and s:name() to K_SNR 123_name().
     * Change <SNR>123_name() to K_SNR 123_name().
     * Use fname_buf[] when it fits, otherwise allocate memory (slow).
     */
    cc = name[len];
    name[len] = NUL;
    llen = eval_fname_script(name);
    if (llen > 0)
    {
	fname_buf[0] = K_SPECIAL;
	fname_buf[1] = KS_EXTRA;
	fname_buf[2] = (int)KE_SNR;
	i = 3;
	if (eval_fname_sid(name))	/* "<SID>" or "s:" */
	{
	    if (current_SID <= 0)
		error = ERROR_SCRIPT;
	    else
	    {
		sprintf((char *)fname_buf + 3, "%ld_", (long)current_SID);
		i = (int)STRLEN(fname_buf);
	    }
	}
	if (i + STRLEN(name + llen) < FLEN_FIXED)
	{
	    STRCPY(fname_buf + i, name + llen);
	    fname = fname_buf;
	}
	else
	{
	    fname = alloc((unsigned)(i + STRLEN(name + llen) + 1));
	    if (fname == NULL)
		error = ERROR_OTHER;
	    else
	    {
		mch_memmove(fname, fname_buf, (size_t)i);
		STRCPY(fname + i, name + llen);
	    }
	}
    }
    else
	fname = name;

    *doesrange = FALSE;


    /* execute the function if no errors detected and executing */
    if (evaluate && error == ERROR_NONE)
    {
	retvar->var_type = VAR_NUMBER;	/* default is number retvar */
	error = ERROR_UNKNOWN;

	if (!ASCII_ISLOWER(fname[0]))
	{
	    /*
	     * User defined function.
	     */
	    fp = find_func(fname);
#ifdef FEAT_AUTOCMD
	    if (fp == NULL && apply_autocmds(EVENT_FUNCUNDEFINED,
						    fname, fname, TRUE, NULL)
#ifdef FEAT_EVAL
		    && !aborting()
#endif
	       )
	    {
		/* executed an autocommand, search for function again */
		fp = find_func(fname);
	    }
#endif
	    if (fp != NULL)
	    {
		if (fp->flags & FC_RANGE)
		    *doesrange = TRUE;
		if (argcount < fp->args.ga_len)
		    error = ERROR_TOOFEW;
		else if (!fp->varargs && argcount > fp->args.ga_len)
		    error = ERROR_TOOMANY;
		else
		{
		    /*
		     * Call the user function.
		     * Save and restore search patterns, script variables and
		     * redo buffer.
		     */
		    save_search_patterns();
		    saveRedobuff();
		    ++fp->calls;
		    call_user_func(fp, argcount, argvars, retvar,
							 firstline, lastline);
		    --fp->calls;
		    restoreRedobuff();
		    restore_search_patterns();
		    error = ERROR_NONE;
		}
	    }
	}
	else
	{
	    /*
	     * Find the function name in the table, call its implementation.
	     */
	    i = find_internal_func(fname);
	    if (i >= 0)
	    {
		if (argcount < functions[i].f_min_argc)
		    error = ERROR_TOOFEW;
		else if (argcount > functions[i].f_max_argc)
		    error = ERROR_TOOMANY;
		else
		{
		    argvars[argcount].var_type = VAR_UNKNOWN;
		    functions[i].f_func(argvars, retvar);
		    error = ERROR_NONE;
		}
	    }
	}
	/*
	 * The function call (or "FuncUndefined" autocommand sequence) might
	 * have been aborted by an error, an interrupt, or an explicitly thrown
	 * exception that has not been caught so far.  This situation can be
	 * tested for by calling aborting().  For an error in an internal
	 * function or for the "E132" error in call_user_func(), however, the
	 * throw point at which the "force_abort" flag (temporarily reset by
	 * emsg()) is normally updated has not been reached yet. We need to
	 * update that flag first to make aborting() reliable.
	 */
	update_force_abort();
    }
    if (error == ERROR_NONE)
	ret = OK;

    /*
     * Report an error unless the argument evaluation or function call has been
     * cancelled due to an aborting error, an interrupt, or an exception.
     */
    if (error < ERROR_NONE && !aborting())
	EMSG2((char_u *)_(errors[error]), name);

    name[len] = cc;
    if (fname != name && fname != fname_buf)
	vim_free(fname);

    return ret;
}

/*********************************************
 * Implementation of the built-in functions
 */

/*
 * "append(lnum, string)" function
 */
    static void
f_append(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    long	lnum;

    lnum = get_var_lnum(argvars);
    retvar->var_val.var_number = 1; /* Default: Failed */

    if (lnum >= 0
	    && lnum <= curbuf->b_ml.ml_line_count
	    && u_save(lnum, lnum + 1) == OK)
    {
	ml_append(lnum, get_var_string(&argvars[1]), (colnr_T)0, FALSE);
	if (curwin->w_cursor.lnum > lnum)
	    ++curwin->w_cursor.lnum;
	appended_lines_mark(lnum, 1L);
	retvar->var_val.var_number = 0;
    }
}

/*
 * "argc()" function
 */
/* ARGSUSED */
    static void
f_argc(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = ARGCOUNT;
}

/*
 * "argidx()" function
 */
/* ARGSUSED */
    static void
f_argidx(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = curwin->w_arg_idx;
}

/*
 * "argv(nr)" function
 */
    static void
f_argv(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		idx;

    idx = get_var_number(&argvars[0]);
    if (idx >= 0 && idx < ARGCOUNT)
	retvar->var_val.var_string = vim_strsave(alist_name(&ARGLIST[idx]));
    else
	retvar->var_val.var_string = NULL;
    retvar->var_type = VAR_STRING;
}

/*
 * "browse(save, title, initdir, default)" function
 */
/* ARGSUSED */
    static void
f_browse(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_BROWSE
    int		save;
    char_u	*title;
    char_u	*initdir;
    char_u	*defname;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];

    save = get_var_number(&argvars[0]);
    title = get_var_string(&argvars[1]);
    initdir = get_var_string_buf(&argvars[2], buf);
    defname = get_var_string_buf(&argvars[3], buf2);

    retvar->var_val.var_string =
		 do_browse(save, title, defname, NULL, initdir, NULL, curbuf);
#else
    retvar->var_val.var_string = NULL;
#endif
    retvar->var_type = VAR_STRING;
}

/*
 * Find a buffer by number or exact name.
 */
    static buf_T *
find_buffer(avar)
    VAR		avar;
{
    buf_T	*buf = NULL;
    char_u	*name;

    if (avar->var_type == VAR_NUMBER)
	buf = buflist_findnr((int)avar->var_val.var_number);
    else if (avar->var_val.var_string != NULL)
    {
	/* First make the name into a full path name */
	name = FullName_save(avar->var_val.var_string,
#ifdef UNIX
		TRUE	    /* force expansion, get rid of symbolic links */
#else
		FALSE
#endif
		);
	if (name != NULL)
	{
	    buf = buflist_findname(name);
	    vim_free(name);
	}
    }
    return buf;
}

/*
 * "bufexists(expr)" function
 */
    static void
f_bufexists(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = (find_buffer(&argvars[0]) != NULL);
}

/*
 * "buflisted(expr)" function
 */
    static void
f_buflisted(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    buf_T	*buf;

    buf = find_buffer(&argvars[0]);
    retvar->var_val.var_number = (buf != NULL && buf->b_p_bl);
}

/*
 * "bufloaded(expr)" function
 */
    static void
f_bufloaded(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    buf_T	*buf;

    buf = find_buffer(&argvars[0]);
    retvar->var_val.var_number = (buf != NULL && buf->b_ml.ml_mfp != NULL);
}

/*
 * Get buffer by number or pattern.
 */
    static buf_T *
get_buf_var(avar)
    VAR		avar;
{
    char_u	*name = avar->var_val.var_string;
    int		save_magic;
    char_u	*save_cpo;
    buf_T	*buf;

    if (avar->var_type == VAR_NUMBER)
	return buflist_findnr((int)avar->var_val.var_number);
    if (name == NULL || *name == NUL)
	return curbuf;
    if (name[0] == '$' && name[1] == NUL)
	return lastbuf;

    /* Ignore 'magic' and 'cpoptions' here to make scripts portable */
    save_magic = p_magic;
    p_magic = TRUE;
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    buf = buflist_findnr(buflist_findpat(name, name + STRLEN(name),
								TRUE, FALSE));

    p_magic = save_magic;
    p_cpo = save_cpo;

    /* If not found, try expanding the name, like done for bufexists(). */
    if (buf == NULL)
	buf = find_buffer(avar);

    return buf;
}

/*
 * "bufname(expr)" function
 */
    static void
f_bufname(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    buf_T	*buf;

    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
    retvar->var_type = VAR_STRING;
    if (buf != NULL && buf->b_fname != NULL)
	retvar->var_val.var_string = vim_strsave(buf->b_fname);
    else
	retvar->var_val.var_string = NULL;
    --emsg_off;
}

/*
 * "bufnr(expr)" function
 */
    static void
f_bufnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    buf_T	*buf;

    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
    if (buf != NULL)
	retvar->var_val.var_number = buf->b_fnum;
    else
	retvar->var_val.var_number = -1;
    --emsg_off;
}

/*
 * "bufwinnr(nr)" function
 */
    static void
f_bufwinnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_WINDOWS
    win_T	*wp;
    int		winnr = 0;
#endif
    buf_T	*buf;

    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
#ifdef FEAT_WINDOWS
    for (wp = firstwin; wp; wp = wp->w_next)
    {
	++winnr;
	if (wp->w_buffer == buf)
	    break;
    }
    retvar->var_val.var_number = (wp != NULL ? winnr : -1);
#else
    retvar->var_val.var_number = (curwin->w_buffer == buf ? 1 : -1);
#endif
    --emsg_off;
}

/*
 * "byte2line(byte)" function
 */
/*ARGSUSED*/
    static void
f_byte2line(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifndef FEAT_BYTEOFF
    retvar->var_val.var_number = -1;
#else
    long	boff = 0;

    boff = get_var_number(&argvars[0]) - 1;
    if (boff < 0)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = ml_find_line_or_offset(curbuf,
							  (linenr_T)0, &boff);
#endif
}

/*
 * "char2nr(string)" function
 */
    static void
f_char2nr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_MBYTE
    if (has_mbyte)
	retvar->var_val.var_number =
				  (*mb_ptr2char)(get_var_string(&argvars[0]));
    else
#endif
    retvar->var_val.var_number = get_var_string(&argvars[0])[0];
}

/*
 * "cindent(lnum)" function
 */
    static void
f_cindent(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_CINDENT
    pos_T	pos;
    linenr_T	lnum;

    pos = curwin->w_cursor;
    lnum = get_var_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
	curwin->w_cursor.lnum = lnum;
	retvar->var_val.var_number = get_c_indent();
	curwin->w_cursor = pos;
    }
    else
#endif
	retvar->var_val.var_number = -1;
}

/*
 * "col(string)" function
 */
    static void
f_col(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    colnr_T	col = 0;
    pos_T	*fp;

    fp = var2fpos(&argvars[0], FALSE);
    if (fp != NULL)
    {
	if (fp->col == MAXCOL)
	{
	    /* '> can be MAXCOL, get the length of the line then */
	    if (fp->lnum <= curbuf->b_ml.ml_line_count)
		col = STRLEN(ml_get(fp->lnum)) + 1;
	    else
		col = MAXCOL;
	}
	else
	{
	    col = fp->col + 1;
#ifdef FEAT_VIRTUALEDIT
	    /* col(".") when the cursor is on the NUL at the end of the line
	     * because of "coladd" can be seen as an extra column. */
	    if (virtual_active() && fp == &curwin->w_cursor)
	    {
		char_u	*p = ml_get_cursor();

		if (curwin->w_cursor.coladd >= (colnr_T)chartabsize(p,
				 curwin->w_virtcol - curwin->w_cursor.coladd))
		{
# ifdef FEAT_MBYTE
		    int		l;

		    if (*p != NUL && p[(l = (*mb_ptr2len_check)(p))] == NUL)
			col += l;
# else
		    if (*p != NUL && p[1] == NUL)
			++col;
# endif
		}
	    }
#endif
	}
    }
    retvar->var_val.var_number = col;
}

/*
 * "confirm(message, buttons[, default [, type]])" function
 */
/*ARGSUSED*/
    static void
f_confirm(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
    char_u	*message;
    char_u	*buttons = NULL;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    int		def = 1;
    int		type = VIM_GENERIC;
    int		c;

    message = get_var_string(&argvars[0]);
    if (argvars[1].var_type != VAR_UNKNOWN)
    {
	buttons = get_var_string_buf(&argvars[1], buf);
	if (argvars[2].var_type != VAR_UNKNOWN)
	{
	    def = get_var_number(&argvars[2]);
	    if (argvars[3].var_type != VAR_UNKNOWN)
	    {
		/* avoid that TOUPPER_ASC calls get_var_string_buf() twice */
		c = *get_var_string_buf(&argvars[3], buf2);
		switch (TOUPPER_ASC(c))
		{
		    case 'E': type = VIM_ERROR; break;
		    case 'Q': type = VIM_QUESTION; break;
		    case 'I': type = VIM_INFO; break;
		    case 'W': type = VIM_WARNING; break;
		    case 'G': type = VIM_GENERIC; break;
		}
	    }
	}
    }

    if (buttons == NULL || *buttons == NUL)
	buttons = (char_u *)_("&Ok");

    retvar->var_val.var_number = do_dialog(type, NULL, message, buttons,
								   def, NULL);
#else
    retvar->var_val.var_number = 0;
#endif
}


/*
 * "cscope_connection([{num} , {dbpath} [, {prepend}]])" function
 *
 * Checks the existence of a cscope connection.
 */
/*ARGSUSED*/
    static void
f_cscope_connection(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_CSCOPE
    int		num = 0;
    char_u	*dbpath = NULL;
    char_u	*prepend = NULL;
    char_u	buf[NUMBUFLEN];

    if (argvars[0].var_type != VAR_UNKNOWN
	    && argvars[1].var_type != VAR_UNKNOWN)
    {
	num = (int)get_var_number(&argvars[0]);
	dbpath = get_var_string(&argvars[1]);
	if (argvars[2].var_type != VAR_UNKNOWN)
	    prepend = get_var_string_buf(&argvars[2], buf);
    }

    retvar->var_val.var_number = cs_connection(num, dbpath, prepend);
#else
    retvar->var_val.var_number = 0;
#endif
}

/*
 * "cursor(lnum, col)" function
 *
 * Moves the cursor to the specified line and column
 */
/*ARGSUSED*/
    static void
f_cursor(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    long	line, col;

    line = get_var_lnum(argvars);
    if (line > 0)
	curwin->w_cursor.lnum = line;
    col = get_var_number(&argvars[1]);
    if (col > 0)
	curwin->w_cursor.col = col - 1;
#ifdef FEAT_VIRTUALEDIT
    curwin->w_cursor.coladd = 0;
#endif

    /* Make sure the cursor is in a valid position. */
    check_cursor();
#ifdef FEAT_MBYTE
    /* Correct cursor for multi-byte character. */
    if (has_mbyte)
	mb_adjust_cursor();
#endif
}

/*
 * "libcall()" function
 */
    static void
f_libcall(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    libcall_common(argvars, retvar, VAR_STRING);
}

/*
 * "libcallnr()" function
 */
    static void
f_libcallnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    libcall_common(argvars, retvar, VAR_NUMBER);
}

    static void
libcall_common(argvars, retvar, type)
    VAR		argvars;
    VAR		retvar;
    int		type;
{
#ifdef FEAT_LIBCALL
    char_u		*string_in;
    char_u		**string_result;
    int			nr_result;
#endif

    retvar->var_type = type;
    if (type == VAR_NUMBER)
	retvar->var_val.var_number = 0;
    else
	retvar->var_val.var_string = NULL;

    if (check_restricted() || check_secure())
	return;

#ifdef FEAT_LIBCALL
    /* The first two args must be strings, otherwise its meaningless */
    if (argvars[0].var_type == VAR_STRING && argvars[1].var_type == VAR_STRING)
    {
	if (argvars[2].var_type == VAR_NUMBER)
	    string_in = NULL;
	else
	    string_in = argvars[2].var_val.var_string;
	if (type == VAR_NUMBER)
	    string_result = NULL;
	else
	    string_result = &retvar->var_val.var_string;
	if (mch_libcall(argvars[0].var_val.var_string,
			     argvars[1].var_val.var_string,
			     string_in,
			     argvars[2].var_val.var_number,
			     string_result,
			     &nr_result) == OK
		&& type == VAR_NUMBER)
	    retvar->var_val.var_number = nr_result;
    }
#endif
}

/*
 * "delete()" function
 */
    static void
f_delete(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    if (check_restricted() || check_secure())
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = mch_remove(get_var_string(&argvars[0]));
}

/*
 * "did_filetype()" function
 */
/*ARGSUSED*/
    static void
f_did_filetype(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_AUTOCMD
    retvar->var_val.var_number = did_filetype;
#else
    retvar->var_val.var_number = 0;
#endif
}

/*
 * "escape({string}, {chars})" function
 */
    static void
f_escape(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[NUMBUFLEN];

    retvar->var_val.var_string =
	vim_strsave_escaped(get_var_string(&argvars[0]),
		get_var_string_buf(&argvars[1], buf));
    retvar->var_type = VAR_STRING;
}

/*
 * "eventhandler()" function
 */
/*ARGSUSED*/
    static void
f_eventhandler(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = vgetc_busy;
}

/*
 * "executable()" function
 */
    static void
f_executable(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = mch_can_exe(get_var_string(&argvars[0]));
}

/*
 * "exists()" function
 */
    static void
f_exists(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;
    char_u	*name;
    int		n = FALSE;
    int		len = 0;

    p = get_var_string(&argvars[0]);
    if (*p == '$')			/* environment variable */
    {
	/* first try "normal" environment variables (fast) */
	if (mch_getenv(p + 1) != NULL)
	    n = TRUE;
	else
	{
	    /* try expanding things like $VIM and ${HOME} */
	    p = expand_env_save(p);
	    if (p != NULL && *p != '$')
		n = TRUE;
	    vim_free(p);
	}
    }
    else if (*p == '&' || *p == '+')			/* option */
	n = (get_option_var(&p, NULL, TRUE) == OK);
    else if (*p == '*')			/* internal or user defined function */
    {
	++p;
	p = trans_function_name(&p, FALSE, TRUE);
	if (p != NULL)
	{
	    if (ASCII_ISUPPER(*p) || p[0] == K_SPECIAL)
		n = (find_func(p) != NULL);
	    else if (ASCII_ISLOWER(*p))
		n = (find_internal_func(p) >= 0);
	    vim_free(p);
	}
    }
    else if (*p == ':')
    {
	n = cmd_exists(p + 1);
    }
    else if (*p == '#')
    {
#ifdef FEAT_AUTOCMD
	name = p + 1;
	p = vim_strchr(name, '#');
	if (p != NULL)
	    n = au_exists(name, p, p + 1);
	else
	    n = au_exists(name, name + STRLEN(name), NULL);
#endif
    }
    else				/* internal variable */
    {
#ifdef FEAT_MAGIC_BRACES
	char_u	*expr_start;
	char_u	*expr_end;
	char_u  *temp_string = NULL;
	char_u	*s;
#endif
	name = p;

#ifdef FEAT_MAGIC_BRACES
	/* Find the end of the name. */
	s = find_name_end(name, &expr_start, &expr_end);
	if (expr_start != NULL)
	{
	    temp_string = make_expanded_name(name, expr_start, expr_end, s);
	    if (temp_string != NULL)
	    {
		len = STRLEN(temp_string);
		name = temp_string;
	    }
	}
#endif
	if (len == 0)
	    len = get_id_len(&p);
	if (len != 0)
	    n = (get_var_var(name, len, NULL) == OK);

#ifdef FEAT_MAGIC_BRACES
	vim_free(temp_string);
#endif
    }

    retvar->var_val.var_number = n;
}

/*
 * "expand()" function
 */
    static void
f_expand(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*s;
    int		len;
    char_u	*errormsg;
    int		flags = WILD_SILENT|WILD_USE_NL|WILD_LIST_NOTFOUND;
    expand_T	xpc;

    retvar->var_type = VAR_STRING;
    s = get_var_string(&argvars[0]);
    if (*s == '%' || *s == '#' || *s == '<')
    {
	++emsg_off;
	retvar->var_val.var_string = eval_vars(s, &len, NULL, &errormsg, s);
	--emsg_off;
    }
    else
    {
	/* When the optional second argument is non-zero, don't remove matches
	 * for 'suffixes' and 'wildignore' */
	if (argvars[1].var_type != VAR_UNKNOWN && get_var_number(&argvars[1]))
	    flags |= WILD_KEEP_ALL;
	ExpandInit(&xpc);
	xpc.xp_context = EXPAND_FILES;
	retvar->var_val.var_string = ExpandOne(&xpc, s, NULL, flags, WILD_ALL);
	ExpandCleanup(&xpc);
    }
}

/*
 * "filereadable()" function
 */
    static void
f_filereadable(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    FILE	*fd;
    char_u	*p;
    int		n;

    p = get_var_string(&argvars[0]);
    if (*p && !mch_isdir(p) && (fd = mch_fopen((char *)p, "r")) != NULL)
    {
	n = TRUE;
	fclose(fd);
    }
    else
	n = FALSE;

    retvar->var_val.var_number = n;
}

/*
 * return 0 for not writable, 1 for writable file, 2 for a dir which we have
 * rights to write into.
 */
    static void
f_filewritable(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;
    int		retval = 0;
#if defined(UNIX) || defined(VMS)
    int		perm = 0;
#endif

    p = get_var_string(&argvars[0]);
#if defined(UNIX) || defined(VMS)
    perm = mch_getperm(p);
#endif
#ifndef MACOS_CLASSIC /* TODO: get either mch_writable or mch_access */
    if (
# ifdef WIN3264
	    mch_writable(p) &&
# else
# if defined(UNIX) || defined(VMS)
	    (perm & 0222) &&
#  endif
# endif
	    mch_access((char *)p, W_OK) == 0
       )
#endif
    {
	++retval;
	if (mch_isdir(p))
	    ++retval;
    }
    retvar->var_val.var_number = retval;
}

/*
 * "fnamemodify({fname}, {mods})" function
 */
    static void
f_fnamemodify(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*fname;
    char_u	*mods;
    int		usedlen = 0;
    int		len;
    char_u	*fbuf = NULL;
    char_u	buf[NUMBUFLEN];

    fname = get_var_string(&argvars[0]);
    mods = get_var_string_buf(&argvars[1], buf);
    len = (int)STRLEN(fname);

    (void)modify_fname(mods, &usedlen, &fname, &fbuf, &len);

    retvar->var_type = VAR_STRING;
    if (fname == NULL)
	retvar->var_val.var_string = NULL;
    else
	retvar->var_val.var_string = vim_strnsave(fname, len);
    vim_free(fbuf);
}

/*
 * "foldclosed()" function
 */
    static void
f_foldclosed(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    foldclosed_both(argvars, retvar, FALSE);
}

/*
 * "foldclosedend()" function
 */
    static void
f_foldclosedend(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    foldclosed_both(argvars, retvar, TRUE);
}

/*
 * "foldclosed()" function
 */
    static void
foldclosed_both(argvars, retvar, end)
    VAR		argvars;
    VAR		retvar;
    int		end;
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;
    linenr_T	first, last;

    lnum = get_var_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
	if (hasFoldingWin(curwin, lnum, &first, &last, FALSE, NULL))
	{
	    if (end)
		retvar->var_val.var_number = (varnumber_T)last;
	    else
		retvar->var_val.var_number = (varnumber_T)first;
	    return;
	}
    }
#endif
    retvar->var_val.var_number = -1;
}

/*
 * "foldlevel()" function
 */
    static void
f_foldlevel(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;

    lnum = get_var_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	retvar->var_val.var_number = foldLevel(lnum);
    else
#endif
	retvar->var_val.var_number = 0;
}

/*
 * "foldtext()" function
 */
/*ARGSUSED*/
    static void
f_foldtext(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;
    char_u	*s;
    char_u	*r;
    int		len;
    char	*txt;
#endif

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;
#ifdef FEAT_FOLDING
    if ((linenr_T)vimvars[VV_FOLDSTART].val > 0
	    && (linenr_T)vimvars[VV_FOLDEND].val <= curbuf->b_ml.ml_line_count
	    && vimvars[VV_FOLDDASHES].val != NULL)
    {
	/* Find first non-empty line in the fold. */
	lnum = (linenr_T)vimvars[VV_FOLDSTART].val;
	while (lnum < (linenr_T)vimvars[VV_FOLDEND].val)
	{
	    if (!linewhite(lnum))
		break;
	    ++lnum;
	}

	/* Find interesting text in this line. */
	s = skipwhite(ml_get(lnum));
	/* skip C comment-start */
	if (s[0] == '/' && (s[1] == '*' || s[1] == '/'))
	    s = skipwhite(s + 2);
	txt = _("+-%s%3ld lines: ");
	r = alloc((unsigned)(STRLEN(txt)
		    + STRLEN(vimvars[VV_FOLDDASHES].val)    /* for %s */
		    + 20				    /* for %3ld */
		    + STRLEN(s)));			    /* concatenated */
	if (r != NULL)
	{
	    sprintf((char *)r, txt, vimvars[VV_FOLDDASHES].val,
		    (long)((linenr_T)vimvars[VV_FOLDEND].val
				   - (linenr_T)vimvars[VV_FOLDSTART].val + 1));
	    len = (int)STRLEN(r);
	    STRCAT(r, s);
	    /* remove 'foldmarker' and 'commentstring' */
	    foldtext_cleanup(r + len);
	    retvar->var_val.var_string = r;
	}
    }
#endif
}

/*
 * "foreground()" function
 */
/*ARGSUSED*/
    static void
f_foreground(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = 0;
#ifdef FEAT_GUI
    if (gui.in_use)
	gui_mch_set_foreground();
#else
# ifdef WIN32
    win32_set_foreground();
# endif
#endif
}

/*
 * "getchar()" function
 */
    static void
f_getchar(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    varnumber_T		n;

    ++no_mapping;
    ++allow_keys;
    if (argvars[0].var_type == VAR_UNKNOWN)
	/* getchar(): blocking wait. */
	n = safe_vgetc();
    else if (get_var_number(&argvars[0]) == 1)
	/* getchar(1): only check if char avail */
	n = vpeekc();
    else if (vpeekc() == NUL)
	/* getchar(0) and no char avail: return zero */
	n = 0;
    else
	/* getchar(0) and char avail: return char */
	n = safe_vgetc();
    --no_mapping;
    --allow_keys;

    retvar->var_val.var_number = n;
    if (IS_SPECIAL(n) || mod_mask != 0)
    {
	char_u		temp[10];   /* modifier: 3, mbyte-char: 6, NUL: 1 */
	int		i = 0;

	/* Turn a special key into three bytes, plus modifier. */
	if (mod_mask != 0)
	{
	    temp[i++] = K_SPECIAL;
	    temp[i++] = KS_MODIFIER;
	    temp[i++] = mod_mask;
	}
	if (IS_SPECIAL(n))
	{
	    temp[i++] = K_SPECIAL;
	    temp[i++] = K_SECOND(n);
	    temp[i++] = K_THIRD(n);
	}
#ifdef FEAT_MBYTE
	else if (has_mbyte)
	    i += (*mb_char2bytes)(n, temp + i);
#endif
	else
	    temp[i++] = n;
	temp[i++] = NUL;
	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = vim_strsave(temp);
    }
}

/*
 * "getcharmod()" function
 */
/*ARGSUSED*/
    static void
f_getcharmod(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = mod_mask;
}

/*
 * "getcmdline()" function
 */
/*ARGSUSED*/
    static void
f_getcmdline(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = get_cmdline_str();
}

/*
 * "getcmdpos()" function
 */
/*ARGSUSED*/
    static void
f_getcmdpos(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = get_cmdline_pos() + 1;
}

/*
 * "getbufvar()" function
 */
    static void
f_getbufvar(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    buf_T	*buf;
    buf_T	*save_curbuf;
    char_u	*varname;
    VAR		v;

    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
    varname = get_var_string(&argvars[1]);

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;

    if (buf != NULL && varname != NULL)
    {
	if (*varname == '&')	/* buffer-local-option */
	{
	    /* set curbuf to be our buf, temporarily */
	    save_curbuf = curbuf;
	    curbuf = buf;

	    get_option_var(&varname, retvar, TRUE);

	    /* restore previous notion of curbuf */
	    curbuf = save_curbuf;
	}
	else
	{
	    /* look up the variable */
	    v = find_var_in_ga(&buf->b_vars, varname);
	    if (v != NULL)
		copy_var(v, retvar);
	}
    }

    --emsg_off;
}

/*
 * "getcwd()" function
 */
/*ARGSUSED*/
    static void
f_getcwd(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	cwd[MAXPATHL];

    retvar->var_type = VAR_STRING;
    if (mch_dirname(cwd, MAXPATHL) == FAIL)
	retvar->var_val.var_string = NULL;
    else
    {
	retvar->var_val.var_string = vim_strsave(cwd);
#ifdef BACKSLASH_IN_FILENAME
	slash_adjust(retvar->var_val.var_string);
#endif
    }
}

/*
 * "getfsize({fname})" function
 */
    static void
f_getfsize(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*fname;
    struct stat	st;

    fname = get_var_string(&argvars[0]);

    retvar->var_type = VAR_NUMBER;

    if (mch_stat((char *)fname, &st) >= 0)
    {
	if (mch_isdir(fname))
	    retvar->var_val.var_number = 0;
	else
	    retvar->var_val.var_number = (varnumber_T)st.st_size;
    }
    else
	  retvar->var_val.var_number = -1;
}

/*
 * "getftime({fname})" function
 */
    static void
f_getftime(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*fname;
    struct stat	st;

    fname = get_var_string(&argvars[0]);

    if (mch_stat((char *)fname, &st) >= 0)
	retvar->var_val.var_number = (varnumber_T)st.st_mtime;
    else
	retvar->var_val.var_number = -1;
}

/*
 * "getreg()" function
 */
    static void
f_getreg(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*strregname;
    int		regname;

    if (argvars[0].var_type != VAR_UNKNOWN)
	strregname = get_var_string(&argvars[0]);
    else
	strregname = vimvars[VV_REG].val;
    regname = (strregname == NULL ? '"' : *strregname);
    if (regname == 0)
	regname = '"';

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = get_reg_contents(regname, TRUE);
}

/*
 * "getregtype()" function
 */
    static void
f_getregtype(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*strregname;
    int		regname;
    char_u	buf[NUMBUFLEN + 2];
    long	reglen = 0;

    if (argvars[0].var_type != VAR_UNKNOWN)
	strregname = get_var_string(&argvars[0]);
    else
	/* Default to v:register */
	strregname = vimvars[VV_REG].val;

    regname = (strregname == NULL ? '"' : *strregname);
    if (regname == 0)
	regname = '"';

    buf[0] = NUL;
    buf[1] = NUL;
    switch (get_reg_type(regname, &reglen))
    {
	case MLINE: buf[0] = 'V'; break;
	case MCHAR: buf[0] = 'v'; break;
#ifdef FEAT_VISUAL
	case MBLOCK:
		buf[0] = Ctrl_V;
		sprintf((char *)buf + 1, "%ld", reglen + 1);
		break;
#endif
    }
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strsave(buf);
}

/*
 * "getline(lnum)" function
 */
    static void
f_getline(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_T	lnum;
    char_u	*p;

    lnum = get_var_lnum(argvars);

    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	p = ml_get(lnum);
    else
	p = (char_u *)"";

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strsave(p);
}

/*
 * "getwinposx()" function
 */
/*ARGSUSED*/
    static void
f_getwinposx(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = -1;
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    retvar->var_val.var_number = x;
    }
#endif
}

/*
 * "getwinposy()" function
 */
/*ARGSUSED*/
    static void
f_getwinposy(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = -1;
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    retvar->var_val.var_number = y;
    }
#endif
}

/*
 * "getwinvar()" function
 */
    static void
f_getwinvar(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    win_T	*win, *oldcurwin;
    char_u	*varname;
    VAR		v;

    ++emsg_off;
    win = find_win_by_nr(&argvars[0]);
    varname = get_var_string(&argvars[1]);

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;

    if (win != NULL && varname != NULL)
    {
	if (*varname == '&')	/* window-local-option */
	{
	    /* set curwin to be our win, temporarily */
	    oldcurwin = curwin;
	    curwin = win;

	    get_option_var(&varname, retvar , 1);

	    /* restore previous notion of curwin */
	    curwin = oldcurwin;
	}
	else
	{
	    /* look up the variable */
	    v = find_var_in_ga(&win->w_vars, varname);
	    if (v != NULL)
		copy_var(v, retvar);
	}
    }

    --emsg_off;
}

/*
 * "glob()" function
 */
    static void
f_glob(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    expand_T	xpc;

    ExpandInit(&xpc);
    xpc.xp_context = EXPAND_FILES;
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = ExpandOne(&xpc, get_var_string(&argvars[0]),
				     NULL, WILD_USE_NL|WILD_SILENT, WILD_ALL);
    ExpandCleanup(&xpc);
}

/*
 * "globpath()" function
 */
    static void
f_globpath(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf1[NUMBUFLEN];

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = globpath(get_var_string(&argvars[0]),
				       get_var_string_buf(&argvars[1], buf1));
}

/*
 * "has()" function
 */
    static void
f_has(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		i;
    char_u	*name;
    int		n = FALSE;
    static char	*(has_list[]) =
    {
#ifdef AMIGA
	"amiga",
# ifdef FEAT_ARP
	"arp",
# endif
#endif
#ifdef __BEOS__
	"beos",
#endif
#ifdef MSDOS
# ifdef DJGPP
	"dos32",
# else
	"dos16",
# endif
#endif
#ifdef MACOS /* TODO: Should we add MACOS_CLASSIC, MACOS_X? (Dany) */
	"mac",
#endif
#if defined(MACOS_X_UNIX)
	"macunix",
#endif
#ifdef OS2
	"os2",
#endif
#ifdef __QNX__
	"qnx",
#endif
#ifdef RISCOS
	"riscos",
#endif
#ifdef UNIX
	"unix",
#endif
#ifdef VMS
	"vms",
#endif
#ifdef WIN16
	"win16",
#endif
#ifdef WIN32
	"win32",
#endif
#if defined(UNIX) && (defined(__CYGWIN32__) || defined(__CYGWIN__))
	"win32unix",
#endif
#ifdef WIN64
	"win64",
#endif
#ifdef EBCDIC
	"ebcdic",
#endif
#ifndef CASE_INSENSITIVE_FILENAME
	"fname_case",
#endif
#ifdef FEAT_ARABIC
	"arabic",
#endif
#ifdef FEAT_AUTOCMD
	"autocmd",
#endif
#ifdef FEAT_BEVAL
	"balloon_eval",
#endif
#if defined(SOME_BUILTIN_TCAPS) || defined(ALL_BUILTIN_TCAPS)
	"builtin_terms",
# ifdef ALL_BUILTIN_TCAPS
	"all_builtin_terms",
# endif
#endif
#ifdef FEAT_BYTEOFF
	"byte_offset",
#endif
#ifdef FEAT_CINDENT
	"cindent",
#endif
#ifdef FEAT_CLIENTSERVER
	"clientserver",
#endif
#ifdef FEAT_CLIPBOARD
	"clipboard",
#endif
#ifdef FEAT_CMDL_COMPL
	"cmdline_compl",
#endif
#ifdef FEAT_CMDHIST
	"cmdline_hist",
#endif
#ifdef FEAT_COMMENTS
	"comments",
#endif
#ifdef FEAT_CRYPT
	"cryptv",
#endif
#ifdef FEAT_CSCOPE
	"cscope",
#endif
#ifdef DEBUG
	"debug",
#endif
#ifdef FEAT_CON_DIALOG
	"dialog_con",
#endif
#ifdef FEAT_GUI_DIALOG
	"dialog_gui",
#endif
#ifdef FEAT_DIFF
	"diff",
#endif
#ifdef FEAT_DIGRAPHS
	"digraphs",
#endif
#ifdef FEAT_DND
	"dnd",
#endif
#ifdef FEAT_EMACS_TAGS
	"emacs_tags",
#endif
	"eval",	    /* always present, of course! */
#ifdef FEAT_EX_EXTRA
	"ex_extra",
#endif
#ifdef FEAT_SEARCH_EXTRA
	"extra_search",
#endif
#ifdef FEAT_FKMAP
	"farsi",
#endif
#ifdef FEAT_SEARCHPATH
	"file_in_path",
#endif
#ifdef FEAT_FIND_ID
	"find_in_path",
#endif
#ifdef FEAT_FOLDING
	"folding",
#endif
#ifdef FEAT_FOOTER
	"footer",
#endif
#if !defined(USE_SYSTEM) && defined(UNIX)
	"fork",
#endif
#ifdef FEAT_GETTEXT
	"gettext",
#endif
#ifdef FEAT_GUI
	"gui",
#endif
#ifdef FEAT_GUI_ATHENA
# ifdef FEAT_GUI_NEXTAW
	"gui_neXtaw",
# else
	"gui_athena",
# endif
#endif
#ifdef FEAT_GUI_BEOS
	"gui_beos",
#endif
#ifdef FEAT_GUI_GTK
	"gui_gtk",
# ifdef HAVE_GTK2
	"gui_gtk2",
# endif
#endif
#ifdef FEAT_GUI_MAC
	"gui_mac",
#endif
#ifdef FEAT_GUI_MOTIF
	"gui_motif",
#endif
#ifdef FEAT_GUI_PHOTON
	"gui_photon",
#endif
#ifdef FEAT_GUI_W16
	"gui_win16",
#endif
#ifdef FEAT_GUI_W32
	"gui_win32",
#endif
#ifdef FEAT_HANGULIN
	"hangul_input",
#endif
#if defined(HAVE_ICONV_H) && defined(USE_ICONV)
	"iconv",
#endif
#ifdef FEAT_INS_EXPAND
	"insert_expand",
#endif
#ifdef FEAT_JUMPLIST
	"jumplist",
#endif
#ifdef FEAT_KEYMAP
	"keymap",
#endif
#ifdef FEAT_LANGMAP
	"langmap",
#endif
#ifdef FEAT_LIBCALL
	"libcall",
#endif
#ifdef FEAT_LINEBREAK
	"linebreak",
#endif
#ifdef FEAT_LISP
	"lispindent",
#endif
#ifdef FEAT_LISTCMDS
	"listcmds",
#endif
#ifdef FEAT_LOCALMAP
	"localmap",
#endif
#ifdef FEAT_MENU
	"menu",
#endif
#ifdef FEAT_SESSION
	"mksession",
#endif
#ifdef FEAT_MODIFY_FNAME
	"modify_fname",
#endif
#ifdef FEAT_MOUSE
	"mouse",
#endif
#ifdef FEAT_MOUSESHAPE
	"mouseshape",
#endif
#if defined(UNIX) || defined(VMS)
# ifdef FEAT_MOUSE_DEC
	"mouse_dec",
# endif
# ifdef FEAT_MOUSE_GPM
	"mouse_gpm",
# endif
# ifdef FEAT_MOUSE_JSB
	"mouse_jsbterm",
# endif
# ifdef FEAT_MOUSE_NET
	"mouse_netterm",
# endif
# ifdef FEAT_MOUSE_PTERM
	"mouse_pterm",
# endif
# ifdef FEAT_MOUSE_XTERM
	"mouse_xterm",
# endif
#endif
#ifdef FEAT_MBYTE
	"multi_byte",
#endif
#ifdef FEAT_MBYTE_IME
	"multi_byte_ime",
#endif
#ifdef FEAT_MULTI_LANG
	"multi_lang",
#endif
#ifdef FEAT_OLE
	"ole",
#endif
#ifdef FEAT_OSFILETYPE
	"osfiletype",
#endif
#ifdef FEAT_PATH_EXTRA
	"path_extra",
#endif
#ifdef FEAT_PERL
#ifndef DYNAMIC_PERL
	"perl",
#endif
#endif
#ifdef FEAT_PYTHON
#ifndef DYNAMIC_PYTHON
	"python",
#endif
#endif
#ifdef FEAT_POSTSCRIPT
	"postscript",
#endif
#ifdef FEAT_PRINTER
	"printer",
#endif
#ifdef FEAT_QUICKFIX
	"quickfix",
#endif
#ifdef FEAT_RIGHTLEFT
	"rightleft",
#endif
#if defined(FEAT_RUBY) && !defined(DYNAMIC_RUBY)
	"ruby",
#endif
#ifdef FEAT_SCROLLBIND
	"scrollbind",
#endif
#ifdef FEAT_CMDL_INFO
	"showcmd",
	"cmdline_info",
#endif
#ifdef FEAT_SIGNS
	"signs",
#endif
#ifdef FEAT_SMARTINDENT
	"smartindent",
#endif
#ifdef FEAT_SNIFF
	"sniff",
#endif
#ifdef FEAT_STL_OPT
	"statusline",
#endif
#ifdef FEAT_SUN_WORKSHOP
	"sun_workshop",
#endif
#ifdef FEAT_NETBEANS_INTG
	"netbeans_intg",
#endif
#ifdef FEAT_SYN_HL
	"syntax",
#endif
#if defined(USE_SYSTEM) || !defined(UNIX)
	"system",
#endif
#ifdef FEAT_TAG_BINS
	"tag_binary",
#endif
#ifdef FEAT_TAG_OLDSTATIC
	"tag_old_static",
#endif
#ifdef FEAT_TAG_ANYWHITE
	"tag_any_white",
#endif
#ifdef FEAT_TCL
# ifndef DYNAMIC_TCL
	"tcl",
# endif
#endif
#ifdef TERMINFO
	"terminfo",
#endif
#ifdef FEAT_TERMRESPONSE
	"termresponse",
#endif
#ifdef FEAT_TEXTOBJ
	"textobjects",
#endif
#ifdef HAVE_TGETENT
	"tgetent",
#endif
#ifdef FEAT_TITLE
	"title",
#endif
#ifdef FEAT_TOOLBAR
	"toolbar",
#endif
#ifdef FEAT_USR_CMDS
	"user-commands",    /* was accidentally included in 5.4 */
	"user_commands",
#endif
#ifdef FEAT_VIMINFO
	"viminfo",
#endif
#ifdef FEAT_VERTSPLIT
	"vertsplit",
#endif
#ifdef FEAT_VIRTUALEDIT
	"virtualedit",
#endif
#ifdef FEAT_VISUAL
	"visual",
#endif
#ifdef FEAT_VISUALEXTRA
	"visualextra",
#endif
#ifdef FEAT_VREPLACE
	"vreplace",
#endif
#ifdef FEAT_WILDIGN
	"wildignore",
#endif
#ifdef FEAT_WILDMENU
	"wildmenu",
#endif
#ifdef FEAT_WINDOWS
	"windows",
#endif
#ifdef FEAT_WAK
	"winaltkeys",
#endif
#ifdef FEAT_WRITEBACKUP
	"writebackup",
#endif
#ifdef FEAT_XIM
	"xim",
#endif
#ifdef FEAT_XFONTSET
	"xfontset",
#endif
#ifdef USE_XSMP
	"xsmp",
#endif
#ifdef USE_XSMP_INTERACT
	"xsmp_interact",
#endif
#ifdef FEAT_XCLIPBOARD
	"xterm_clipboard",
#endif
#ifdef FEAT_XTERM_SAVE
	"xterm_save",
#endif
#if defined(UNIX) && defined(FEAT_X11)
	"X11",
#endif
	NULL
    };

    name = get_var_string(&argvars[0]);
    for (i = 0; has_list[i] != NULL; ++i)
	if (STRICMP(name, has_list[i]) == 0)
	{
	    n = TRUE;
	    break;
	}

    if (n == FALSE)
    {
	if (STRNICMP(name, "patch", 5) == 0)
	    n = has_patch(atoi((char *)name + 5));
	else if (STRICMP(name, "vim_starting") == 0)
	    n = (starting != 0);
#ifdef DYNAMIC_TCL
	else if (STRICMP(name, "tcl") == 0)
	    n = tcl_enabled(FALSE);
#endif
#if defined(USE_ICONV) && defined(DYNAMIC_ICONV)
	else if (STRICMP(name, "iconv") == 0)
	    n = iconv_enabled(FALSE);
#endif
#ifdef DYNAMIC_RUBY
	else if (STRICMP(name, "ruby") == 0)
	    n = ruby_enabled(FALSE);
#endif
#ifdef DYNAMIC_PYTHON
	else if (STRICMP(name, "python") == 0)
	    n = python_enabled(FALSE);
#endif
#ifdef DYNAMIC_PERL
	else if (STRICMP(name, "perl") == 0)
	    n = perl_enabled(FALSE);
#endif
#ifdef FEAT_GUI
	else if (STRICMP(name, "gui_running") == 0)
	    n = (gui.in_use || gui.starting);
# ifdef FEAT_GUI_W32
	else if (STRICMP(name, "gui_win32s") == 0)
	    n = gui_is_win32s();
# endif
# ifdef FEAT_BROWSE
	else if (STRICMP(name, "browse") == 0)
	    n = gui.in_use;	/* gui_mch_browse() works when GUI is running */
# endif
#endif
#ifdef FEAT_SYN_HL
	else if (STRICMP(name, "syntax_items") == 0)
	    n = syntax_present(curbuf);
#endif
#if defined(WIN3264)
	else if (STRICMP(name, "win95") == 0)
	    n = mch_windows95();
#endif
    }

    retvar->var_val.var_number = n;
}

/*
 * "hasmapto()" function
 */
    static void
f_hasmapto(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*name;
    char_u	*mode;
    char_u	buf[NUMBUFLEN];

    name = get_var_string(&argvars[0]);
    if (argvars[1].var_type == VAR_UNKNOWN)
	mode = (char_u *)"nvo";
    else
	mode = get_var_string_buf(&argvars[1], buf);

    if (map_to_exists(name, mode))
	retvar->var_val.var_number = TRUE;
    else
	retvar->var_val.var_number = FALSE;
}

/*
 * "histadd()" function
 */
/*ARGSUSED*/
    static void
f_histadd(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_CMDHIST
    int		histype;
    char_u	*str;
    char_u	buf[NUMBUFLEN];
#endif

    retvar->var_val.var_number = FALSE;
    if (check_restricted() || check_secure())
	return;
#ifdef FEAT_CMDHIST
    histype = get_histtype(get_var_string(&argvars[0]));
    if (histype >= 0)
    {
	str = get_var_string_buf(&argvars[1], buf);
	if (*str != NUL)
	{
	    add_to_history(histype, str, FALSE, NUL);
	    retvar->var_val.var_number = TRUE;
	    return;
	}
    }
#endif
}

/*
 * "histdel()" function
 */
/*ARGSUSED*/
    static void
f_histdel(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_CMDHIST
    int		n;
    char_u	buf[NUMBUFLEN];

    if (argvars[1].var_type == VAR_UNKNOWN)
	/* only one argument: clear entire history */
	n = clr_history(get_histtype(get_var_string(&argvars[0])));
    else if (argvars[1].var_type == VAR_NUMBER)
	/* index given: remove that entry */
	n = del_history_idx(get_histtype(get_var_string(&argvars[0])),
					    (int)get_var_number(&argvars[1]));
    else
	/* string given: remove all matching entries */
	n = del_history_entry(get_histtype(get_var_string(&argvars[0])),
					get_var_string_buf(&argvars[1], buf));
    retvar->var_val.var_number = n;
#else
    retvar->var_val.var_number = 0;
#endif
}

/*
 * "histget()" function
 */
/*ARGSUSED*/
    static void
f_histget(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_CMDHIST
    int		type;
    int		idx;

    type = get_histtype(get_var_string(&argvars[0]));
    if (argvars[1].var_type == VAR_UNKNOWN)
	idx = get_history_idx(type);
    else
	idx = (int)get_var_number(&argvars[1]);
    retvar->var_val.var_string = vim_strsave(get_history_entry(type, idx));
#else
    retvar->var_val.var_string = NULL;
#endif
    retvar->var_type = VAR_STRING;
}

/*
 * "histnr()" function
 */
/*ARGSUSED*/
    static void
f_histnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		i;

#ifdef FEAT_CMDHIST
    i = get_histtype(get_var_string(&argvars[0]));
    if (i >= HIST_CMD && i < HIST_COUNT)
	i = get_history_idx(i);
    else
#endif
	i = -1;
    retvar->var_val.var_number = i;
}

/*
 * "highlight_exists()" function
 */
    static void
f_hlexists(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = highlight_exists(get_var_string(&argvars[0]));
}

/*
 * "highlightID(name)" function
 */
    static void
f_hlID(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = syn_name2id(get_var_string(&argvars[0]));
}

/*
 * "hostname()" function
 */
/*ARGSUSED*/
    static void
f_hostname(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u hostname[256];

    mch_get_host_name(hostname, 256);
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strsave(hostname);
}

/*
 * iconv() function
 */
/*ARGSUSED*/
    static void
f_iconv(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_MBYTE
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*from, *to, *str;
    vimconv_T	vimconv;
#endif

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;

#ifdef FEAT_MBYTE
    str = get_var_string(&argvars[0]);
    from = enc_canonize(enc_skip(get_var_string_buf(&argvars[1], buf1)));
    to = enc_canonize(enc_skip(get_var_string_buf(&argvars[2], buf2)));
    vimconv.vc_type = CONV_NONE;
    convert_setup(&vimconv, from, to);

    /* If the encodings are equal, no conversion needed. */
    if (vimconv.vc_type == CONV_NONE)
	retvar->var_val.var_string = vim_strsave(str);
    else
	retvar->var_val.var_string = string_convert(&vimconv, str, NULL);

    convert_setup(&vimconv, NULL, NULL);
    vim_free(from);
    vim_free(to);
#endif
}

/*
 * "indent()" function
 */
    static void
f_indent(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_T	lnum;

    lnum = get_var_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	retvar->var_val.var_number = get_indent_lnum(lnum);
    else
	retvar->var_val.var_number = -1;
}

static int inputsecret_flag = 0;

/*
 * "input()" function
 *     Also handles inputsecret() when inputsecret is set.
 */
    static void
f_input(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*prompt = get_var_string(&argvars[0]);
    char_u	*p = NULL;
    int		c;
    char_u	buf[NUMBUFLEN];
    int		cmd_silent_save = cmd_silent;

    retvar->var_type = VAR_STRING;

#ifdef NO_CONSOLE_INPUT
    /* While starting up, there is no place to enter text. */
    if (no_console_input())
    {
	retvar->var_val.var_string = NULL;
	return;
    }
#endif

    cmd_silent = FALSE;		/* Want to see the prompt. */
    if (prompt != NULL)
    {
	/* Only the part of the message after the last NL is considered as
	 * prompt for the command line */
	p = vim_strrchr(prompt, '\n');
	if (p == NULL)
	    p = prompt;
	else
	{
	    ++p;
	    c = *p;
	    *p = NUL;
	    msg_start();
	    msg_clr_eos();
	    msg_puts_attr(prompt, echo_attr);
	    msg_didout = FALSE;
	    msg_starthere();
	    *p = c;
	}
	cmdline_row = msg_row;
    }

    if (argvars[1].var_type != VAR_UNKNOWN)
	stuffReadbuffSpec(get_var_string_buf(&argvars[1], buf));

    retvar->var_val.var_string =
		getcmdline_prompt(inputsecret_flag ? NUL : '@', p, echo_attr);

    /* since the user typed this, no need to wait for return */
    need_wait_return = FALSE;
    msg_didout = FALSE;
    cmd_silent = cmd_silent_save;
}

/*
 * "inputdialog()" function
 */
    static void
f_inputdialog(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#if defined(FEAT_GUI_TEXTDIALOG)
    /* Use a GUI dialog if the GUI is running and 'c' is not in 'guioptions' */
    if (gui.in_use && vim_strchr(p_go, GO_CONDIALOG) == NULL)
    {
	char_u	*message;
	char_u	buf[NUMBUFLEN];

	message = get_var_string(&argvars[0]);
	if (argvars[1].var_type != VAR_UNKNOWN)
	{
	    STRNCPY(IObuff, get_var_string_buf(&argvars[1], buf), IOSIZE);
	    IObuff[IOSIZE - 1] = NUL;
	}
	else
	    IObuff[0] = NUL;
	if (do_dialog(VIM_QUESTION, NULL, message, (char_u *)_("&OK\n&Cancel"),
							      1, IObuff) == 1)
	    retvar->var_val.var_string = vim_strsave(IObuff);
	else
	{
	    if (argvars[1].var_type != VAR_UNKNOWN
					&& argvars[2].var_type != VAR_UNKNOWN)
		retvar->var_val.var_string = vim_strsave(
					get_var_string_buf(&argvars[2], buf));
	    else
		retvar->var_val.var_string = NULL;
	}
	retvar->var_type = VAR_STRING;
    }
    else
#endif
	f_input(argvars, retvar);
}

static garray_T	    ga_userinput = {0, 0, sizeof(tasave_T), 4, NULL};

/*
 * "inputrestore()" function
 */
/*ARGSUSED*/
    static void
f_inputrestore(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    if (ga_userinput.ga_len > 0)
    {
	--ga_userinput.ga_len;
	++ga_userinput.ga_room;
	restore_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	retvar->var_val.var_number = 0; /* OK */
    }
    else if (p_verbose > 1)
    {
	msg((char_u *)_("called inputrestore() more often than inputsave()"));
	retvar->var_val.var_number = 1; /* Failed */
    }
}

/*
 * "inputsave()" function
 */
/*ARGSUSED*/
    static void
f_inputsave(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    /* Add an entry to the stack of typehead storage. */
    if (ga_grow(&ga_userinput, 1) == OK)
    {
	save_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	++ga_userinput.ga_len;
	--ga_userinput.ga_room;
	retvar->var_val.var_number = 0; /* OK */
    }
    else
	retvar->var_val.var_number = 1; /* Failed */
}

/*
 * "inputsecret()" function
 */
    static void
f_inputsecret(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    ++cmdline_star;
    ++inputsecret_flag;
    f_input(argvars, retvar);
    --cmdline_star;
    --inputsecret_flag;
}

/*
 * "isdirectory()" function
 */
    static void
f_isdirectory(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = mch_isdir(get_var_string(&argvars[0]));
}

/*
 * "last_buffer_nr()" function.
 */
/*ARGSUSED*/
    static void
f_last_buffer_nr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		n = 0;
    buf_T	*buf;

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (n < buf->b_fnum)
	    n = buf->b_fnum;

    retvar->var_val.var_number = n;
}

/*
 * "line(string)" function
 */
    static void
f_line(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_T	lnum = 0;
    pos_T	*fp;

    fp = var2fpos(&argvars[0], TRUE);
    if (fp != NULL)
	lnum = fp->lnum;
    retvar->var_val.var_number = lnum;
}

/*
 * "line2byte(lnum)" function
 */
/*ARGSUSED*/
    static void
f_line2byte(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifndef FEAT_BYTEOFF
    retvar->var_val.var_number = -1;
#else
    linenr_T	lnum;

    lnum = get_var_lnum(argvars);
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count + 1)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = ml_find_line_or_offset(curbuf, lnum, NULL);
    if (retvar->var_val.var_number >= 0)
	++retvar->var_val.var_number;
#endif
}

/*
 * "lispindent(lnum)" function
 */
    static void
f_lispindent(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_LISP
    pos_T	pos;
    linenr_T	lnum;

    pos = curwin->w_cursor;
    lnum = get_var_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
	curwin->w_cursor.lnum = lnum;
	retvar->var_val.var_number = get_lisp_indent();
	curwin->w_cursor = pos;
    }
    else
#endif
	retvar->var_val.var_number = -1;
}

/*
 * "localtime()" function
 */
/*ARGSUSED*/
    static void
f_localtime(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = (varnumber_T)time(NULL);
}

/*
 * "maparg()" function
 */
    static void
f_maparg(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    get_maparg(argvars, retvar, TRUE);
}

/*
 * "mapcheck()" function
 */
    static void
f_mapcheck(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    get_maparg(argvars, retvar, FALSE);
}

    static void
get_maparg(argvars, retvar, exact)
    VAR		argvars;
    VAR		retvar;
    int		exact;
{
    char_u	*keys;
    char_u	*which;
    char_u	buf[NUMBUFLEN];
    char_u	*keys_buf = NULL;
    char_u	*rhs;
    int		mode;
    garray_T	ga;

    /* return empty string for failure */
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;

    keys = get_var_string(&argvars[0]);
    if (*keys == NUL)
	return;

    if (argvars[1].var_type != VAR_UNKNOWN)
	which = get_var_string_buf(&argvars[1], buf);
    else
	which = (char_u *)"";
    mode = get_map_mode(&which, 0);

    keys = replace_termcodes(keys, &keys_buf, TRUE, TRUE);
    rhs = check_map(keys, mode, exact);
    vim_free(keys_buf);
    if (rhs != NULL)
    {
	ga_init(&ga);
	ga.ga_itemsize = 1;
	ga.ga_growsize = 40;

	while (*rhs != NUL)
	    ga_concat(&ga, str2special(&rhs, FALSE));

	ga_append(&ga, NUL);
	retvar->var_val.var_string = (char_u *)ga.ga_data;
    }
}

/*
 * "match()" function
 */
    static void
f_match(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    find_some_match(argvars, retvar, 1);
}

/*
 * "matchend()" function
 */
    static void
f_matchend(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    find_some_match(argvars, retvar, 0);
}

/*
 * "matchstr()" function
 */
    static void
f_matchstr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    find_some_match(argvars, retvar, 2);
}

    static void
find_some_match(argvars, retvar, type)
    VAR		argvars;
    VAR		retvar;
    int		type;
{
    char_u	*str;
    char_u	*pat;
    regmatch_T	regmatch;
    char_u	patbuf[NUMBUFLEN];
    char_u	*save_cpo;
    long	start = 0;

    /* Make 'cpoptions' empty, the 'l' flag should not be used here. */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    str = get_var_string(&argvars[0]);
    pat = get_var_string_buf(&argvars[1], patbuf);

    if (type == 2)
    {
	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = NULL;
    }
    else
	retvar->var_val.var_number = -1;

    if (argvars[2].var_type != VAR_UNKNOWN)
    {
	start = get_var_number(&argvars[2]);
	if (start < 0)
	    start = 0;
	if (start > (long)STRLEN(str))
	    goto theend;
	str += start;
    }

    regmatch.regprog = vim_regcomp(pat, RE_MAGIC + RE_STRING);
    if (regmatch.regprog != NULL)
    {
	regmatch.rm_ic = p_ic;
	if (vim_regexec_nl(&regmatch, str, (colnr_T)0))
	{
	    if (type == 2)
		retvar->var_val.var_string = vim_strnsave(regmatch.startp[0],
				(int)(regmatch.endp[0] - regmatch.startp[0]));
	    else
	    {
		if (type != 0)
		    retvar->var_val.var_number =
				      (varnumber_T)(regmatch.startp[0] - str);
		else
		    retvar->var_val.var_number =
					(varnumber_T)(regmatch.endp[0] - str);
		retvar->var_val.var_number += start;
	    }
	}
	vim_free(regmatch.regprog);
    }

theend:
    p_cpo = save_cpo;
}

/*
 * "mode()" function
 */
/*ARGSUSED*/
    static void
f_mode(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[2];

#ifdef FEAT_VISUAL
    if (VIsual_active)
    {
	if (VIsual_select)
	    buf[0] = VIsual_mode + 's' - 'v';
	else
	    buf[0] = VIsual_mode;
    }
    else
#endif
	if (State == HITRETURN || State == ASKMORE || State == SETWSIZE)
	buf[0] = 'r';
    else if (State & INSERT)
    {
	if (State & REPLACE_FLAG)
	    buf[0] = 'R';
	else
	    buf[0] = 'i';
    }
    else if (State & CMDLINE)
	buf[0] = 'c';
    else
	buf[0] = 'n';

    buf[1] = NUL;
    retvar->var_val.var_string = vim_strsave(buf);
    retvar->var_type = VAR_STRING;
}

/*
 * "nr2char()" function
 */
    static void
f_nr2char(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[NUMBUFLEN];

#ifdef FEAT_MBYTE
    if (has_mbyte)
	buf[(*mb_char2bytes)((int)get_var_number(&argvars[0]), buf)] = NUL;
    else
#endif
    {
	buf[0] = (char_u)get_var_number(&argvars[0]);
	buf[1] = NUL;
    }
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strsave(buf);
}

/*
 * "rename({from}, {to})" function
 */
    static void
f_rename(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = vim_rename(get_var_string(&argvars[0]),
					get_var_string_buf(&argvars[1], buf));
}

/*
 * "resolve()" function
 */
    static void
f_resolve(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;

    p = get_var_string(&argvars[0]);
#ifdef FEAT_SHORTCUT
    {
	char_u	*v = NULL;

	v = mch_resolve_shortcut(p);
	if (v != NULL)
	    retvar->var_val.var_string = v;
	else
	    retvar->var_val.var_string = vim_strsave(p);
    }
#else
# ifdef HAVE_READLINK
    {
	char_u	buf[MAXPATHL + 1];
	char_u	*cpy;
	int	len;
	char_u	*remain = NULL;
	char_u	*q;
	int	is_relative_to_current = FALSE;
	int	has_trailing_pathsep = FALSE;
	int	limit = 100;

	p = vim_strsave(p);

	if (p[0] == '.' && (vim_ispathsep(p[1])
				   || (p[1] == '.' && (vim_ispathsep(p[2])))))
	    is_relative_to_current = TRUE;

	len = STRLEN(p);
	if (len > 0 && vim_ispathsep(p[len-1]))
	    has_trailing_pathsep = TRUE;

	q = getnextcomp(p);
	if (*q != NUL)
	{
	    /* Separate the first path component in "p", and keep the
	     * remainder (beginning with the path separator). */
	    remain = vim_strsave(q - 1);
	    q[-1] = NUL;
	}

	for (;;)
	{
	    for (;;)
	    {
		len = readlink((char *)p, (char *)buf, MAXPATHL);
		if (len <= 0)
		    break;
		buf[len] = NUL;

		if (limit-- == 0)
		{
		    vim_free(p);
		    vim_free(remain);
		    EMSG(_("E655: Too many symbolic links (cycle?)"));
		    retvar->var_val.var_string = NULL;
		    goto fail;
		}

		/* Ensure that the result will have a trailing path separator
		 * if the argument has one. */
		if (remain == NULL && has_trailing_pathsep)
		    add_pathsep(buf);

		/* Separate the first path component in the link value and
		 * concatenate the remainders. */
		q = getnextcomp(vim_ispathsep(*buf) ? buf + 1 : buf);
		if (*q != NUL)
		{
		    if (remain == NULL)
			remain = vim_strsave(q - 1);
		    else
		    {
			cpy = vim_strnsave(q-1, STRLEN(q-1)+STRLEN(remain));
			if (cpy != NULL)
			{
			    STRCAT(cpy, remain);
			    vim_free(remain);
			    remain = cpy;
			}
		    }
		    q[-1] = NUL;
		}

		q = gettail(p);
		if (q > p && *q == NUL)
		{
		    /* Ignore trailing path separator. */
		    q[-1] = NUL;
		    q = gettail(p);
		}
		if (q > p && !mch_isFullName(buf))
		{
		    /* symlink is relative to directory of argument */
		    cpy = alloc((unsigned)(STRLEN(p) + STRLEN(buf) + 1));
		    if (cpy != NULL)
		    {
			STRCPY(cpy, p);
			STRCPY(gettail(cpy), buf);
			vim_free(p);
			p = cpy;
		    }
		}
		else
		{
		    vim_free(p);
		    p = vim_strsave(buf);
		}
	    }

	    if (remain == NULL)
		break;

	    /* Append the first path component of "remain" to "p". */
	    q = getnextcomp(remain + 1);
	    len = q - remain - (*q != NUL);
	    cpy = vim_strnsave(p, STRLEN(p) + len);
	    if (cpy != NULL)
	    {
		STRNCAT(cpy, remain, len);
		vim_free(p);
		p = cpy;
	    }
	    /* Shorten "remain". */
	    if (*q != NUL)
		STRCPY(remain, q - 1);
	    else
	    {
		vim_free(remain);
		remain = NULL;
	    }
	}

	/* If the result is a relative path name, make it explicitly relative to
	 * the current directory if and only if the argument had this form. */
	if (!vim_ispathsep(*p))
	{
	    if (is_relative_to_current
		    && *p != NUL
		    && !(p[0] == '.'
			&& (p[1] == NUL
			    || vim_ispathsep(p[1])
			    || (p[1] == '.'
				&& (p[2] == NUL
				    || vim_ispathsep(p[2]))))))
	    {
		/* Prepend "./". */
		cpy = vim_strnsave((char_u *)"./", 2 + STRLEN(p));
		if (cpy != NULL)
		{
		    STRCAT(cpy, p);
		    vim_free(p);
		    p = cpy;
		}
	    }
	    else if (!is_relative_to_current)
	    {
		/* Strip leading "./". */
		q = p;
		while (q[0] == '.' && vim_ispathsep(q[1]))
		    q += 2;
		if (q > p)
		    mch_memmove(p, p + 2, STRLEN(p + 2) + (size_t)1);
	    }
	}

	/* Ensure that the result will have no trailing path separator
	 * if the argument had none.  But keep "/" or "//". */
	if (!has_trailing_pathsep)
	{
	    q = p + STRLEN(p);
	    while ((q > p + 2 || (q == p + 2 && !vim_ispathsep(*p)))
		   && vim_ispathsep(q[-1]))
		--q;
	    *q = NUL;
	}

	retvar->var_val.var_string = p;
    }
# else
    retvar->var_val.var_string = vim_strsave(p);
# endif
#endif

    simplify_filename(retvar->var_val.var_string);

#ifdef HAVE_READLINK
fail:
#endif
    retvar->var_type = VAR_STRING;
}

/*
 * "simplify()" function
 */
    static void
f_simplify(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;

    p = get_var_string(&argvars[0]);
    retvar->var_val.var_string = vim_strsave(p);
    simplify_filename(retvar->var_val.var_string);	/* simplify in place */
    retvar->var_type = VAR_STRING;
}

/*
 * "search()" function
 */
    static void
f_search(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*pat;
    pos_T	pos;
    int		save_p_ws = p_ws;
    int		dir;

    pat = get_var_string(&argvars[0]);
    dir = get_search_arg(&argvars[1], NULL);	/* may set p_ws */

    pos = curwin->w_cursor;
    if (searchit(curwin, curbuf, &pos, dir, pat, 1L,
					      SEARCH_KEEP, RE_SEARCH) != FAIL)
    {
	retvar->var_val.var_number = pos.lnum;
	curwin->w_cursor = pos;
	/* "/$" will put the cursor after the end of the line, may need to
	 * correct that here */
	check_cursor();
    }
    else
	retvar->var_val.var_number = 0;
    p_ws = save_p_ws;
}

#define SP_NOMOVE	1	/* don't move cursor */
#define SP_REPEAT	2	/* repeat to find outer pair */
#define SP_RETCOUNT	4	/* return matchcount */

/*
 * "searchpair()" function
 */
    static void
f_searchpair(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*spat, *mpat, *epat;
    char_u	*skip;
    char_u	*pat, *pat2, *pat3;
    pos_T	pos;
    pos_T	firstpos;
    pos_T	save_cursor;
    pos_T	save_pos;
    int		save_p_ws = p_ws;
    char_u	*save_cpo;
    int		dir;
    int		flags = 0;
    char_u	nbuf1[NUMBUFLEN];
    char_u	nbuf2[NUMBUFLEN];
    char_u	nbuf3[NUMBUFLEN];
    int		n;
    int		r;
    int		nest = 1;
    int		err;

    retvar->var_val.var_number = 0;	/* default: FAIL */

    /* Make 'cpoptions' empty, the 'l' flag should not be used here. */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    /* Get the three pattern arguments: start, middle, end. */
    spat = get_var_string(&argvars[0]);
    mpat = get_var_string_buf(&argvars[1], nbuf1);
    epat = get_var_string_buf(&argvars[2], nbuf2);

    /* Make two search patterns: start/end (pat2, for in nested pairs) and
     * start/middle/end (pat3, for the top pair). */
    pat2 = alloc((unsigned)(STRLEN(spat) + STRLEN(epat) + 15));
    pat3 = alloc((unsigned)(STRLEN(spat) + STRLEN(mpat) + STRLEN(epat) + 23));
    if (pat2 == NULL || pat3 == NULL)
	goto theend;
    sprintf((char *)pat2, "\\(%s\\m\\)\\|\\(%s\\m\\)", spat, epat);
    if (*mpat == NUL)
	STRCPY(pat3, pat2);
    else
	sprintf((char *)pat3, "\\(%s\\m\\)\\|\\(%s\\m\\)\\|\\(%s\\m\\)",
							    spat, epat, mpat);

    /* Handle the optional fourth argument: flags */
    dir = get_search_arg(&argvars[3], &flags); /* may set p_ws */

    /* Optional fifth argument: skip expresion */
    if (argvars[3].var_type == VAR_UNKNOWN
	    || argvars[4].var_type == VAR_UNKNOWN)
	skip = (char_u *)"";
    else
	skip = get_var_string_buf(&argvars[4], nbuf3);

    save_cursor = curwin->w_cursor;
    pos = curwin->w_cursor;
    firstpos.lnum = 0;
    pat = pat3;
    for (;;)
    {
	n = searchit(curwin, curbuf, &pos, dir, pat, 1L,
						      SEARCH_KEEP, RE_SEARCH);
	if (n == FAIL || (firstpos.lnum != 0 && equalpos(pos, firstpos)))
	    /* didn't find it or found the first match again: FAIL */
	    break;

	if (firstpos.lnum == 0)
	    firstpos = pos;

	/* If the skip pattern matches, ignore this match. */
	if (*skip != NUL)
	{
	    save_pos = curwin->w_cursor;
	    curwin->w_cursor = pos;
	    r = eval_to_bool(skip, &err, NULL, FALSE);
	    curwin->w_cursor = save_pos;
	    if (err)
	    {
		/* Evaluating {skip} caused an error, break here. */
		curwin->w_cursor = save_cursor;
		retvar->var_val.var_number = -1;
		break;
	    }
	    if (r)
		continue;
	}

	if ((dir == BACKWARD && n == 3) || (dir == FORWARD && n == 2))
	{
	    /* Found end when searching backwards or start when searching
	     * forward: nested pair. */
	    ++nest;
	    pat = pat2;		/* nested, don't search for middle */
	}
	else
	{
	    /* Found end when searching forward or start when searching
	     * backward: end of (nested) pair; or found middle in outer pair. */
	    if (--nest == 1)
		pat = pat3;	/* outer level, search for middle */
	}

	if (nest == 0)
	{
	    /* Found the match: return matchcount or line number. */
	    if (flags & SP_RETCOUNT)
		++retvar->var_val.var_number;
	    else
		retvar->var_val.var_number = pos.lnum;
	    curwin->w_cursor = pos;
	    if (!(flags & SP_REPEAT))
		break;
	    nest = 1;	    /* search for next unmatched */
	}
    }

    /* If 'n' flag is used or search failed: restore cursor position. */
    if ((flags & SP_NOMOVE) || retvar->var_val.var_number == 0)
	curwin->w_cursor = save_cursor;

theend:
    vim_free(pat2);
    vim_free(pat3);
    p_ws = save_p_ws;
    p_cpo = save_cpo;
}

    static int
get_search_arg(varp, flagsp)
    VAR		varp;
    int		*flagsp;
{
    int		dir = FORWARD;
    char_u	*flags;
    char_u	nbuf[NUMBUFLEN];

    if (varp->var_type != VAR_UNKNOWN)
    {
	flags = get_var_string_buf(varp, nbuf);
	if (vim_strchr(flags, 'b') != NULL)
	    dir = BACKWARD;
	if (vim_strchr(flags, 'w') != NULL)
	    p_ws = TRUE;
	if (vim_strchr(flags, 'W') != NULL)
	    p_ws = FALSE;
	if (flagsp != NULL)
	{
	    if (vim_strchr(flags, 'n') != NULL)
		*flagsp |= SP_NOMOVE;
	    if (vim_strchr(flags, 'r') != NULL)
		*flagsp |= SP_REPEAT;
	    if (vim_strchr(flags, 'm') != NULL)
		*flagsp |= SP_RETCOUNT;
	}
    }
    return dir;
}

/*
 * "setbufvar()" function
 */
/*ARGSUSED*/
    static void
f_setbufvar(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    buf_T	*buf;
#ifdef FEAT_AUTOCMD
    aco_save_T	aco;
#else
    buf_T	*save_curbuf;
#endif
    char_u	*varname, *bufvarname;
    VAR		varp;
    char_u	nbuf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;
    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
    varname = get_var_string(&argvars[1]);
    varp = &argvars[2];

    if (buf != NULL && varname != NULL && varp != NULL)
    {
	/* set curbuf to be our buf, temporarily */
#ifdef FEAT_AUTOCMD
	aucmd_prepbuf(&aco, buf);
#else
	save_curbuf = curbuf;
	curbuf = buf;
#endif

	if (*varname == '&')
	{
	    ++varname;
	    set_option_value(varname, get_var_number(varp),
				   get_var_string_buf(varp, nbuf), OPT_LOCAL);
	}
	else
	{
	    bufvarname = alloc((unsigned)STRLEN(varname) + 3);
	    if (bufvarname != NULL)
	    {
		STRCPY(bufvarname, "b:");
		STRCPY(bufvarname + 2, varname);
		set_var(bufvarname, varp);
		vim_free(bufvarname);
	    }
	}

	/* reset notion of buffer */
#ifdef FEAT_AUTOCMD
	aucmd_restbuf(&aco);
#else
	curbuf = save_curbuf;
#endif
    }
    --emsg_off;
}

/*
 * "setcmdpos()" function
 */
    static void
f_setcmdpos(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = set_cmdline_pos(
					(int)get_var_number(&argvars[0]) - 1);
}

/*
 * "setline()" function
 */
    static void
f_setline(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_T	lnum;
    char_u	*line;

    lnum = get_var_lnum(argvars);
    line = get_var_string(&argvars[1]);
    retvar->var_val.var_number = 1;		/* FAIL is default */

    if (lnum >= 1
	    && lnum <= curbuf->b_ml.ml_line_count
	    && u_savesub(lnum) == OK
	    && ml_replace(lnum, line, TRUE) == OK)
    {
	changed_bytes(lnum, 0);
	check_cursor_col();
	retvar->var_val.var_number = 0;
    }
}

/*
 * "setreg()" function
 */
    static void
f_setreg(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		regname;
    char_u	*strregname;
    char_u	*stropt;
    int		append;
    char_u	yank_type;
    long	block_len;

    block_len = -1;
    yank_type = MAUTO;
    append = FALSE;

    strregname = get_var_string(argvars);
    retvar->var_val.var_number = 1;		/* FAIL is default */

    regname = (strregname == NULL ? '"' : *strregname);
    if (regname == 0 || regname == '@')
	regname = '"';
    else if (regname == '=')
	return;

    if (argvars[2].var_type != VAR_UNKNOWN)
    {
	for (stropt = get_var_string(&argvars[2]); *stropt != NUL; ++stropt)
	    switch (*stropt)
	    {
		case 'a': case 'A':	/* append */
		    append = TRUE;
		    break;
		case 'v': case 'c':	/* character-wise selection */
		    yank_type = MCHAR;
		    break;
		case 'V': case 'l':	/* line-wise selection */
		    yank_type = MLINE;
		    break;
#ifdef FEAT_VISUAL
		case 'b': case Ctrl_V:	/* block-wise selection */
		    yank_type = MBLOCK;
		    if (VIM_ISDIGIT(stropt[1]))
		    {
			++stropt;
			block_len = getdigits(&stropt) - 1;
			--stropt;
		    }
		    break;
#endif
	    }
    }

    write_reg_contents_ex(regname, get_var_string(&argvars[1]), -1,
						append, yank_type, block_len);
    retvar->var_val.var_number = 0;
}


/*
 * "setwinvar(expr)" function
 */
/*ARGSUSED*/
    static void
f_setwinvar(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    win_T	*win;
#ifdef FEAT_WINDOWS
    win_T	*save_curwin;
#endif
    char_u	*varname, *winvarname;
    VAR		varp;
    char_u	nbuf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;
    ++emsg_off;
    win = find_win_by_nr(&argvars[0]);
    varname = get_var_string(&argvars[1]);
    varp = &argvars[2];

    if (win != NULL && varname != NULL && varp != NULL)
    {
#ifdef FEAT_WINDOWS
	/* set curwin to be our win, temporarily */
	save_curwin = curwin;
	curwin = win;
	curbuf = curwin->w_buffer;
#endif

	if (*varname == '&')
	{
	    ++varname;
	    set_option_value(varname, get_var_number(varp),
				   get_var_string_buf(varp, nbuf), OPT_LOCAL);
	}
	else
	{
	    winvarname = alloc((unsigned)STRLEN(varname) + 3);
	    if (winvarname != NULL)
	    {
		STRCPY(winvarname, "w:");
		STRCPY(winvarname + 2, varname);
		set_var(winvarname, varp);
		vim_free(winvarname);
	    }
	}

#ifdef FEAT_WINDOWS
	/* Restore current window, if it's still valid (autocomands can make
	 * it invalid). */
	if (win_valid(save_curwin))
	{
	    curwin = save_curwin;
	    curbuf = curwin->w_buffer;
	}
#endif
    }
    --emsg_off;
}

/*
 * "nextnonblank()" function
 */
    static void
f_nextnonblank(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_T	lnum;

    for (lnum = get_var_lnum(argvars); ; ++lnum)
    {
	if (lnum > curbuf->b_ml.ml_line_count)
	{
	    lnum = 0;
	    break;
	}
	if (*skipwhite(ml_get(lnum)) != NUL)
	    break;
    }
    retvar->var_val.var_number = lnum;
}

/*
 * "prevnonblank()" function
 */
    static void
f_prevnonblank(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_T	lnum;

    lnum = get_var_lnum(argvars);
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count)
	lnum = 0;
    else
	while (lnum >= 1 && *skipwhite(ml_get(lnum)) == NUL)
	    --lnum;
    retvar->var_val.var_number = lnum;
}

#if defined(FEAT_CLIENTSERVER) && defined(FEAT_X11)
static void make_connection __ARGS((void));
static int check_connection __ARGS((void));

    static void
make_connection()
{
    if (X_DISPLAY == NULL
# ifdef FEAT_GUI
	    && !gui.in_use
# endif
	    )
    {
	x_force_connect = TRUE;
	setup_term_clip();
	x_force_connect = FALSE;
    }
}

    static int
check_connection()
{
    make_connection();
    if (X_DISPLAY == NULL)
    {
	EMSG(_("E240: No connection to Vim server"));
	return FAIL;
    }
    return OK;
}
#endif

/*ARGSUSED*/
    static void
f_serverlist(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*r = NULL;

#ifdef FEAT_CLIENTSERVER
# ifdef WIN32
    r = serverGetVimNames();
# else
    make_connection();
    if (X_DISPLAY != NULL)
	r = serverGetVimNames(X_DISPLAY);
# endif
#endif
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = r;
}

/*ARGSUSED*/
    static void
f_remote_peek(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_CLIENTSERVER
    var		v;
    char_u	*s = NULL;
# ifdef WIN32
    int		n = 0;
# endif

    if (check_restricted() || check_secure())
    {
	retvar->var_val.var_number = -1;
	return;
    }
# ifdef WIN32
    sscanf(get_var_string(&argvars[0]), "%x", &n);
    if (n == 0)
	retvar->var_val.var_number = -1;
    else
    {
	s = serverGetReply((HWND)n, FALSE, FALSE, FALSE);
	retvar->var_val.var_number = (s != NULL);
    }
# else
    retvar->var_val.var_number = 0;
    if (check_connection() == FAIL)
	return;

    retvar->var_val.var_number = serverPeekReply(X_DISPLAY,
			     serverStrToWin(get_var_string(&argvars[0])), &s);
# endif

    if (argvars[1].var_type != VAR_UNKNOWN && retvar->var_val.var_number > 0)
    {
	v.var_type = VAR_STRING;
	v.var_val.var_string = vim_strsave(s);
	set_var(get_var_string(&argvars[1]), &v);
    }
#else
    retvar->var_val.var_number = -1;
#endif
}

/*ARGSUSED*/
    static void
f_remote_read(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*r = NULL;

#ifdef FEAT_CLIENTSERVER
    if (!check_restricted() && !check_secure())
    {
# ifdef WIN32
	/* The server's HWND is encoded in the 'id' parameter */
	int		n = 0;

	sscanf(get_var_string(&argvars[0]), "%x", &n);
	if (n != 0)
	    r = serverGetReply((HWND)n, FALSE, TRUE, TRUE);
	if (r == NULL)
# else
	if (check_connection() == FAIL || serverReadReply(X_DISPLAY,
		  serverStrToWin(get_var_string(&argvars[0])), &r, FALSE) < 0)
# endif
	    EMSG(_("E277: Unable to read a server reply"));
    }
#endif
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = r;
}

/*ARGSUSED*/
    static void
f_server2client(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_CLIENTSERVER
    char_u	buf[NUMBUFLEN];
    char_u	*server = get_var_string(&argvars[0]);
    char_u	*reply = get_var_string_buf(&argvars[1], buf);

    retvar->var_val.var_number = -1;
    if (check_restricted() || check_secure())
	return;
# ifdef FEAT_X11
    if (check_connection() == FAIL)
	return;
# endif

    if (serverSendReply(server, reply) < 0)
    {
	EMSG(_("E258: Unable to send to client"));
	return;
    }
    retvar->var_val.var_number = 0;
#else
    retvar->var_val.var_number = -1;
#endif
}

#ifdef FEAT_CLIENTSERVER
static void remote_common __ARGS((VAR argvars, VAR retvar, int expr));

    static void
remote_common(argvars, retvar, expr)
    VAR		argvars;
    VAR		retvar;
    int		expr;
{
    char_u	*server_name;
    char_u	*keys;
    char_u	*r = NULL;
    char_u	buf[NUMBUFLEN];
# ifdef WIN32
    HWND	w;
# else
    Window	w;
# endif

    if (check_restricted() || check_secure())
	return;

# ifdef FEAT_X11
    if (check_connection() == FAIL)
	return;
# endif

    server_name = get_var_string(&argvars[0]);
    keys = get_var_string_buf(&argvars[1], buf);
# ifdef WIN32
    if (serverSendToVim(server_name, keys, &r, &w, expr, TRUE) < 0)
# else
    if (serverSendToVim(X_DISPLAY, server_name, keys, &r, &w, expr, 0, TRUE)
									  < 0)
# endif
    {
	if (r != NULL)
	    EMSG(r);		/* sending worked but evaluation failed */
	else
	    EMSG2(_("E241: Unable to send to %s"), server_name);
	return;
    }

    retvar->var_val.var_string = r;

    if (argvars[2].var_type != VAR_UNKNOWN)
    {
	var	v;
	char_u	str[30];

	sprintf((char *)str, "0x%x", (unsigned int)w);
	v.var_type = VAR_STRING;
	v.var_val.var_string = vim_strsave(str);
	set_var(get_var_string(&argvars[2]), &v);
    }
}
#endif

/*
 * "remote_expr()" function
 */
/*ARGSUSED*/
    static void
f_remote_expr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;
#ifdef FEAT_CLIENTSERVER
    remote_common(argvars, retvar, TRUE);
#endif
}

/*
 * "remote_send()" function
 */
/*ARGSUSED*/
    static void
f_remote_send(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;
#ifdef FEAT_CLIENTSERVER
    remote_common(argvars, retvar, FALSE);
#endif
}

/*
 * "remote_foreground()" function
 */
/*ARGSUSED*/
    static void
f_remote_foreground(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = 0;
#ifdef FEAT_CLIENTSERVER
# ifdef WIN32
    /* On Win32 it's done in this application. */
    serverForeground(get_var_string(&argvars[0]));
# else
    /* Send a foreground() expression to the server. */
    argvars[1].var_type = VAR_STRING;
    argvars[1].var_val.var_string = vim_strsave((char_u *)"foreground()");
    argvars[2].var_type = VAR_UNKNOWN;
    remote_common(argvars, retvar, TRUE);
    vim_free(argvars[1].var_val.var_string);
# endif
#endif
}

#ifdef HAVE_STRFTIME
/*
 * "strftime({format}[, {time}])" function
 */
    static void
f_strftime(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	result_buf[256];
    struct tm	*curtime;
    time_t	seconds;
    char_u	*p;

    retvar->var_type = VAR_STRING;

    p = get_var_string(&argvars[0]);
    if (argvars[1].var_type == VAR_UNKNOWN)
	seconds = time(NULL);
    else
	seconds = (time_t)get_var_number(&argvars[1]);
    curtime = localtime(&seconds);
    /* MSVC returns NULL for an invalid value of seconds. */
    if (curtime == NULL)
	retvar->var_val.var_string = vim_strsave((char_u *)_("(Invalid)"));
    else
    {
# ifdef FEAT_MBYTE
	vimconv_T   conv;
	char_u	    *enc;

	conv.vc_type = CONV_NONE;
	enc = enc_locale();
	convert_setup(&conv, p_enc, enc);
	if (conv.vc_type != CONV_NONE)
	    p = string_convert(&conv, p, NULL);
# endif
	if (p != NULL)
	    (void)strftime((char *)result_buf, sizeof(result_buf),
							  (char *)p, curtime);
	else
	    result_buf[0] = NUL;

# ifdef FEAT_MBYTE
	if (conv.vc_type != CONV_NONE)
	    vim_free(p);
	convert_setup(&conv, enc, p_enc);
	if (conv.vc_type != CONV_NONE)
	    retvar->var_val.var_string =
				      string_convert(&conv, result_buf, NULL);
	else
# endif
	    retvar->var_val.var_string = vim_strsave(result_buf);

# ifdef FEAT_MBYTE
	/* Release conversion descriptors */
	convert_setup(&conv, NULL, NULL);
	vim_free(enc);
# endif
    }
}
#endif

/*
 * "stridx()" function
 */
    static void
f_stridx(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*pos;

    needle = get_var_string(&argvars[1]);
    haystack = get_var_string_buf(&argvars[0], buf);
    pos	= (char_u *)strstr((char *)haystack, (char *)needle);

    if (pos == NULL)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = (varnumber_T) (pos - haystack);
}

/*
 * "strridx()" function
 */
    static void
f_strridx(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*rest;
    char_u	*lastmatch = NULL;

    needle = get_var_string(&argvars[1]);
    haystack = get_var_string_buf(&argvars[0], buf);
    rest = haystack;
    while (*haystack != '\0')
    {
	rest = (char_u *)strstr((char *)rest, (char *)needle);
	if (rest == NULL)
	    break;
	lastmatch = rest++;
    }

    if (lastmatch == NULL)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = (varnumber_T) (lastmatch - haystack);
}

/*
 * "strlen()" function
 */
    static void
f_strlen(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = (varnumber_T) (STRLEN(get_var_string(&argvars[0])));
}

/*
 * "strpart()" function
 */
    static void
f_strpart(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;
    int		n;
    int		len;
    int		slen;

    p = get_var_string(&argvars[0]);
    slen = (int)STRLEN(p);

    n = get_var_number(&argvars[1]);
    if (argvars[2].var_type != VAR_UNKNOWN)
	len = get_var_number(&argvars[2]);
    else
	len = slen - n;	    /* default len: all bytes that are available. */

    /*
     * Only return the overlap between the specified part and the actual
     * string.
     */
    if (n < 0)
    {
	len += n;
	n = 0;
    }
    else if (n > slen)
	n = slen;
    if (len < 0)
	len = 0;
    else if (n + len > slen)
	len = slen - n;

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strnsave(p + n, len);
}

/*
 * "strtrans()" function
 */
    static void
f_strtrans(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = transstr(get_var_string(&argvars[0]));
}

/*
 * "synID(line, col, trans)" function
 */
/*ARGSUSED*/
    static void
f_synID(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		id = 0;
#ifdef FEAT_SYN_HL
    long	line;
    long	col;
    int		trans;

    line = get_var_lnum(argvars);
    col = get_var_number(&argvars[1]) - 1;
    trans = get_var_number(&argvars[2]);

    if (line >= 1 && line <= curbuf->b_ml.ml_line_count
	    && col >= 0 && col < (long)STRLEN(ml_get(line)))
	id = syn_get_id(line, col, trans);
#endif

    retvar->var_val.var_number = id;
}

/*
 * "synIDattr(id, what [, mode])" function
 */
/*ARGSUSED*/
    static void
f_synIDattr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p = NULL;
#ifdef FEAT_SYN_HL
    int		id;
    char_u	*what;
    char_u	*mode;
    char_u	modebuf[NUMBUFLEN];
    int		modec;

    id = get_var_number(&argvars[0]);
    what = get_var_string(&argvars[1]);
    if (argvars[2].var_type != VAR_UNKNOWN)
    {
	mode = get_var_string_buf(&argvars[2], modebuf);
	modec = TOLOWER_ASC(mode[0]);
	if (modec != 't' && modec != 'c'
#ifdef FEAT_GUI
		&& modec != 'g'
#endif
		)
	    modec = 0;	/* replace invalid with current */
    }
    else
    {
#ifdef FEAT_GUI
	if (gui.in_use)
	    modec = 'g';
	else
#endif
	    if (t_colors > 1)
	    modec = 'c';
	else
	    modec = 't';
    }


    switch (TOLOWER_ASC(what[0]))
    {
	case 'b':
		if (TOLOWER_ASC(what[1]) == 'g')	/* bg[#] */
		    p = highlight_color(id, what, modec);
		else					/* bold */
		    p = highlight_has_attr(id, HL_BOLD, modec);
		break;

	case 'f':					/* fg[#] */
		p = highlight_color(id, what, modec);
		break;

	case 'i':
		if (TOLOWER_ASC(what[1]) == 'n')	/* inverse */
		    p = highlight_has_attr(id, HL_INVERSE, modec);
		else					/* italic */
		    p = highlight_has_attr(id, HL_ITALIC, modec);
		break;

	case 'n':					/* name */
		p = get_highlight_name(NULL, id - 1);
		break;

	case 'r':					/* reverse */
		p = highlight_has_attr(id, HL_INVERSE, modec);
		break;

	case 's':					/* standout */
		p = highlight_has_attr(id, HL_STANDOUT, modec);
		break;

	case 'u':					/* underline */
		p = highlight_has_attr(id, HL_UNDERLINE, modec);
		break;
    }

    if (p != NULL)
	p = vim_strsave(p);
#endif
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = p;
}

/*
 * "synIDtrans(id)" function
 */
/*ARGSUSED*/
    static void
f_synIDtrans(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		id;

#ifdef FEAT_SYN_HL
    id = get_var_number(&argvars[0]);

    if (id > 0)
	id = syn_get_final_id(id);
    else
#endif
	id = 0;

    retvar->var_val.var_number = id;
}

/*
 * "system()" function
 */
    static void
f_system(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;

    p = get_cmd_output(get_var_string(&argvars[0]), SHELL_SILENT);
#ifdef USE_CR
    /* translate <CR> into <NL> */
    if (p != NULL)
    {
	char_u	*s;

	for (s = p; *s; ++s)
	{
	    if (*s == CAR)
		*s = NL;
	}
    }
#else
# ifdef USE_CRNL
    /* translate <CR><NL> into <NL> */
    if (p != NULL)
    {
	char_u	*s, *d;

	d = p;
	for (s = p; *s; ++s)
	{
	    if (s[0] == CAR && s[1] == NL)
		++s;
	    *d++ = *s;
	}
	*d = NUL;
    }
# endif
#endif
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = p;
}

/*
 * "submatch()" function
 */
    static void
f_submatch(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = reg_submatch((int)get_var_number(&argvars[0]));
}

/*
 * "substitute()" function
 */
    static void
f_substitute(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	patbuf[NUMBUFLEN];
    char_u	subbuf[NUMBUFLEN];
    char_u	flagsbuf[NUMBUFLEN];

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = do_string_sub(
	    get_var_string(&argvars[0]),
	    get_var_string_buf(&argvars[1], patbuf),
	    get_var_string_buf(&argvars[2], subbuf),
	    get_var_string_buf(&argvars[3], flagsbuf));
}

/*
 * "tempname()" function
 */
/*ARGSUSED*/
    static void
f_tempname(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    static int	x = 'A';

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_tempname(x);

    /* Advance 'x' to use A-Z and 0-9, so that there are at least 34 different
     * names.  Skip 'I' and 'O', they are used for shell redirection. */
    do
    {
	if (x == 'Z')
	    x = '0';
	else if (x == '9')
	    x = 'A';
	else
	{
#ifdef EBCDIC
	    if (x == 'I')
		x = 'J';
	    else if (x == 'R')
		x = 'S';
	    else
#endif
		++x;
	}
    } while (x == 'I' || x == 'O');
}

/*
 * "tolower(string)" function
 */
    static void
f_tolower(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;

    p = vim_strsave(get_var_string(&argvars[0]));
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = p;

    if (p != NULL)
	while (*p != NUL)
	{
#ifdef FEAT_MBYTE
	    int		l;

	    if (enc_utf8)
	    {
		int c, lc;

		c = utf_ptr2char(p);
		lc = utf_tolower(c);
		l = utf_ptr2len_check(p);
		/* TODO: reallocate string when byte count changes. */
		if (utf_char2len(lc) == l)
		    utf_char2bytes(lc, p);
		p += l;
	    }
	    else if (has_mbyte && (l = (*mb_ptr2len_check)(p)) > 1)
		p += l;		/* skip multi-byte character */
	    else
#endif
	    {
		*p = TOLOWER_LOC(*p); /* note that tolower() can be a macro */
		++p;
	    }
	}
}

/*
 * "toupper(string)" function
 */
    static void
f_toupper(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;

    p = vim_strsave(get_var_string(&argvars[0]));
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = p;

    if (p != NULL)
	while (*p != NUL)
	{
#ifdef FEAT_MBYTE
	    int		l;

	    if (enc_utf8)
	    {
		int c, uc;

		c = utf_ptr2char(p);
		uc = utf_toupper(c);
		l = utf_ptr2len_check(p);
		/* TODO: reallocate string when byte count changes. */
		if (utf_char2len(uc) == l)
		    utf_char2bytes(uc, p);
		p += l;
	    }
	    else if (has_mbyte && (l = (*mb_ptr2len_check)(p)) > 1)
		p += l;		/* skip multi-byte character */
	    else
#endif
	    {
		*p = TOUPPER_LOC(*p); /* note that toupper() can be a macro */
		p++;
	    }
	}
}

/*
 * "type(expr)" function
 */
    static void
f_type(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    if (argvars[0].var_type == VAR_NUMBER)
	retvar->var_val.var_number = 0;
    else
	retvar->var_val.var_number = 1;
}

/*
 * "virtcol(string)" function
 */
    static void
f_virtcol(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    colnr_T	vcol = 0;
    pos_T	*fp;

    fp = var2fpos(&argvars[0], FALSE);
    if (fp != NULL && fp->lnum <= curbuf->b_ml.ml_line_count)
    {
	getvvcol(curwin, fp, NULL, NULL, &vcol);
	++vcol;
    }

    retvar->var_val.var_number = vcol;
}

/*
 * "visualmode()" function
 */
/*ARGSUSED*/
    static void
f_visualmode(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_VISUAL
    char_u	str[2];

    retvar->var_type = VAR_STRING;
    str[0] = curbuf->b_visual_mode_eval;
    str[1] = NUL;
    retvar->var_val.var_string = vim_strsave(str);

    /* A non-zero number or non-empty string argument: reset mode. */
    if ((argvars[0].var_type == VAR_NUMBER
		&& argvars[0].var_val.var_number != 0)
	    || (argvars[0].var_type == VAR_STRING
		&& *get_var_string(&argvars[0]) != NUL))
	curbuf->b_visual_mode_eval = NUL;
#else
    retvar->var_val.var_number = 0; /* return anything, it won't work anyway */
#endif
}

/*
 * "winbufnr(nr)" function
 */
    static void
f_winbufnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    win_T	*wp;

    wp = find_win_by_nr(&argvars[0]);
    if (wp == NULL)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = wp->w_buffer->b_fnum;
}

/*
 * "wincol()" function
 */
/*ARGSUSED*/
    static void
f_wincol(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    validate_cursor();
    retvar->var_val.var_number = curwin->w_wcol + 1;
}

/*
 * "winheight(nr)" function
 */
    static void
f_winheight(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    win_T	*wp;

    wp = find_win_by_nr(&argvars[0]);
    if (wp == NULL)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = wp->w_height;
}

/*
 * "winline()" function
 */
/*ARGSUSED*/
    static void
f_winline(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    validate_cursor();
    retvar->var_val.var_number = curwin->w_wrow + 1;
}

/*
 * "winnr()" function
 */
/* ARGSUSED */
    static void
f_winnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		nr = 1;
#ifdef FEAT_WINDOWS
    win_T	*wp;

    for (wp = firstwin; wp != curwin; wp = wp->w_next)
	++nr;
#endif
    retvar->var_val.var_number = nr;
}

/*
 * "winrestcmd()" function
 */
/* ARGSUSED */
    static void
f_winrestcmd(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef FEAT_WINDOWS
    win_T	*wp;
    int		winnr = 1;
    garray_T	ga;
    char_u	buf[50];

    ga_init2(&ga, (int)sizeof(char), 70);
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
    {
	sprintf((char *)buf, "%dresize %d|", winnr, wp->w_height);
	ga_concat(&ga, buf);
# ifdef FEAT_VERTSPLIT
	sprintf((char *)buf, "vert %dresize %d|", winnr, wp->w_width);
	ga_concat(&ga, buf);
# endif
	++winnr;
    }

    retvar->var_val.var_string = ga.ga_data;
#else
    retvar->var_val.var_string = NULL;
#endif
    retvar->var_type = VAR_STRING;
}

/*
 * "winwidth(nr)" function
 */
    static void
f_winwidth(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    win_T	*wp;

    wp = find_win_by_nr(&argvars[0]);
    if (wp == NULL)
	retvar->var_val.var_number = -1;
    else
#ifdef FEAT_VERTSPLIT
	retvar->var_val.var_number = wp->w_width;
#else
	retvar->var_val.var_number = Columns;
#endif
}

    static win_T *
find_win_by_nr(vp)
    VAR		vp;
{
#ifdef FEAT_WINDOWS
    win_T	*wp;
#endif
    int		nr;

    nr = get_var_number(vp);

#ifdef FEAT_WINDOWS
    if (nr == 0)
	return curwin;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (--nr <= 0)
	    break;
    return wp;
#else
    if (nr == 0 || nr == 1)
	return curwin;
    return NULL;
#endif
}

/*
 * Translate a String variable into a position.
 */
    static pos_T *
var2fpos(varp, lnum)
    VAR		varp;
    int		lnum;		/* TRUE when $ is last line */
{
    char_u	*name;
    static pos_T	pos;
    pos_T	*pp;

    name = get_var_string(varp);
    if (name[0] == '.')		/* cursor */
	return &curwin->w_cursor;
    if (name[0] == '\'')	/* mark */
    {
	pp = getmark(name[1], FALSE);
	if (pp == NULL || pp == (pos_T *)-1 || pp->lnum <= 0)
	    return NULL;
	return pp;
    }
    if (name[0] == '$')		/* last column or line */
    {
	if (lnum)
	{
	    pos.lnum = curbuf->b_ml.ml_line_count;
	    pos.col = 0;
	}
	else
	{
	    pos.lnum = curwin->w_cursor.lnum;
	    pos.col = (colnr_T)STRLEN(ml_get_curline());
	}
	return &pos;
    }
    return NULL;
}

/*
 * Get the length of an environment variable name.
 * Advance "arg" to the first character after the name.
 * Return 0 for error.
 */
    static int
get_env_len(arg)
    char_u	**arg;
{
    char_u	*p;
    int		len;

    for (p = *arg; vim_isIDc(*p); ++p)
	;
    if (p == *arg)	    /* no name found */
	return 0;

    len = (int)(p - *arg);
    *arg = p;
    return len;
}

/*
 * Get the length of the name of a function or internal variable.
 * "arg" is advanced to the first non-white character after the name.
 * Return 0 if something is wrong.
 */
    static int
get_id_len(arg)
    char_u	**arg;
{
    char_u	*p;
    int		len;

    /* Find the end of the name. */
    for (p = *arg; eval_isnamec(*p); ++p)
	;
    if (p == *arg)	    /* no name found */
	return 0;

    len = (int)(p - *arg);
    *arg = skipwhite(p);

    return len;
}

/*
 * Get the length of the name of a function.
 * "arg" is advanced to the first non-white character after the name.
 * Return 0 if something is wrong.
 * If the name contains 'magic' {}'s, expand them and return the
 * expanded name in an allocated string via 'alias' - caller must free.
 */
    static int
get_func_len(arg, alias, evaluate)
    char_u	**arg;
    char_u	**alias;
    int		evaluate;
{
    int		len;
#ifdef FEAT_MAGIC_BRACES
    char_u	*p;
    char_u	*expr_start;
    char_u	*expr_end;
#endif

    *alias = NULL;  /* default to no alias */

    if ((*arg)[0] == K_SPECIAL && (*arg)[1] == KS_EXTRA
						  && (*arg)[2] == (int)KE_SNR)
    {
	/* hard coded <SNR>, already translated */
	*arg += 3;
	return get_id_len(arg) + 3;
    }
    len = eval_fname_script(*arg);
    if (len > 0)
    {
	/* literal "<SID>", "s:" or "<SNR>" */
	*arg += len;
    }

#ifdef FEAT_MAGIC_BRACES
    /*
     * Find the end of the name;
     */
    p = find_name_end(*arg, &expr_start, &expr_end);
    /* check for {} construction */
    if (expr_start != NULL)
    {
	char_u	*temp_string;

	if (!evaluate)
	{
	    len += (int)(p - *arg);
	    *arg = skipwhite(p);
	    return len;
	}

	/*
	 * Include any <SID> etc in the expanded string:
	 * Thus the -len here.
	 */
	temp_string = make_expanded_name(*arg - len, expr_start, expr_end, p);
	if (temp_string == NULL)
	    return 0;
	*alias = temp_string;
	*arg = skipwhite(p);
	return (int)STRLEN(temp_string);
    }
#endif

    len += get_id_len(arg);
    if (len == 0)
	EMSG2(_(e_invexpr2), *arg);

    return len;
}

    static char_u *
find_name_end(arg, expr_start, expr_end)
    char_u	*arg;
    char_u	**expr_start;
    char_u	**expr_end;
{
    int		nesting = 0;
    char_u	*p;

    *expr_start = NULL;
    *expr_end = NULL;

    for (p = arg; (*p != NUL && (eval_isnamec(*p) || nesting != 0)); ++p)
    {
#ifdef FEAT_MAGIC_BRACES
	if (*p == '{')
	{
	    nesting++;
	    if (*expr_start == NULL)
		*expr_start = p;
	}
	else if (*p == '}')
	{
	    nesting--;
	    if (nesting == 0 && *expr_end == NULL)
		*expr_end = p;
	}
#endif
    }

    return p;
}

/*
 * Return TRUE if character "c" can be used in a variable or function name.
 */
    static int
eval_isnamec(c)
    int	    c;
{
    return (ASCII_ISALNUM(c) || c == '_' || c == ':'
#ifdef FEAT_MAGIC_BRACES
	    || c == '{' || c == '}'
#endif
	    );
}

/*
 * Find a v: variable.
 * Return it's index, or -1 if not found.
 */
    static int
find_vim_var(name, len)
    char_u	*name;
    int		len;		/* length of "name" */
{
    char_u	*vname;
    int		vlen;
    int		i;

    /*
     * Ignore "v:" for old built-in variables, require it for new ones.
     */
    if (name[0] == 'v' && name[1] == ':')
    {
	vname = name + 2;
	vlen = len - 2;
    }
    else
    {
	vname = name;
	vlen = len;
    }
    for (i = 0; i < VV_LEN; ++i)
	if (vlen == vimvars[i].len && STRCMP(vname, vimvars[i].name) == 0
			 && ((vimvars[i].flags & VV_COMPAT) || vname != name))
	    return i;
    return -1;
}

/*
 * Set number v: variable to "val".
 */
    void
set_vim_var_nr(idx, val)
    int		idx;
    long	val;
{
    vimvars[idx].val = (char_u *)val;
}

/*
 * Get number v: variable value;
 */
    long
get_vim_var_nr(idx)
    int		idx;
{
    return (long)vimvars[idx].val;
}

/*
 * Set v:count, v:count1 and v:prevcount.
 */
    void
set_vcount(count, count1)
    long	count;
    long	count1;
{
    vimvars[VV_PREVCOUNT].val = vimvars[VV_COUNT].val;
    vimvars[VV_COUNT].val = (char_u *)count;
    vimvars[VV_COUNT1].val = (char_u *)count1;
}

/*
 * Set string v: variable to a copy of "val".
 */
    void
set_vim_var_string(idx, val, len)
    int		idx;
    char_u	*val;
    int		len;	    /* length of "val" to use or -1 (whole string) */
{
    vim_free(vimvars[idx].val);
    if (val == NULL)
	vimvars[idx].val = NULL;
    else if (len == -1)
	vimvars[idx].val = vim_strsave(val);
    else
	vimvars[idx].val = vim_strnsave(val, len);
}

/*
 * Set v:register if needed.
 */
    void
set_reg_var(c)
    int		c;
{
    char_u	regname;

    if (c == 0 || c == ' ')
	regname = '"';
    else
	regname = c;
    /* Avoid free/alloc when the value is already right. */
    if (vimvars[VV_REG].val == NULL || vimvars[VV_REG].val[0] != c)
	set_vim_var_string(VV_REG, &regname, 1);
}

/*
 * Get or set v:exception.  If "oldval" == NULL, return the current value.
 * Otherwise, restore the value to "oldval" and return NULL.
 * Must always be called in pairs to save and restore v:exception!  Does not
 * take care of memory allocations.
 */
    char_u *
v_exception(oldval)
    char_u	*oldval;
{
    if (oldval == NULL)
	return vimvars[VV_EXCEPTION].val;

    vimvars[VV_EXCEPTION].val = oldval;
    return NULL;
}

/*
 * Get or set v:throwpoint.  If "oldval" == NULL, return the current value.
 * Otherwise, restore the value to "oldval" and return NULL.
 * Must always be called in pairs to save and restore v:throwpoint!  Does not
 * take care of memory allocations.
 */
    char_u *
v_throwpoint(oldval)
    char_u	*oldval;
{
    if (oldval == NULL)
	return vimvars[VV_THROWPOINT].val;

    vimvars[VV_THROWPOINT].val = oldval;
    return NULL;
}

#if defined(FEAT_AUTOCMD) || defined(PROTO)
/*
 * Set v:cmdarg.
 * If "eap" != NULL, use "eap" to generate the value and return the old value.
 * If "oldarg" != NULL, restore the value to "oldarg" and return NULL.
 * Must always be called in pairs!
 */
    char_u *
set_cmdarg(eap, oldarg)
    exarg_T	*eap;
    char_u	*oldarg;
{
    char_u	*oldval;
    char_u	*newval;
    unsigned	len;

    oldval = vimvars[VV_CMDARG].val;
    if (eap != NULL)
    {
	if (eap->force_bin == FORCE_BIN)
	    len = 6;
	else if (eap->force_bin == FORCE_NOBIN)
	    len = 8;
	else
	    len = 0;
	if (eap->force_ff != 0)
	    len += (unsigned)STRLEN(eap->cmd + eap->force_ff) + 6;
# ifdef FEAT_MBYTE
	if (eap->force_enc != 0)
	    len += (unsigned)STRLEN(eap->cmd + eap->force_enc) + 7;
# endif

	newval = alloc(len + 1);
	if (newval == NULL)
	    return NULL;

	if (eap->force_bin == FORCE_BIN)
	    sprintf((char *)newval, " ++bin");
	else if (eap->force_bin == FORCE_NOBIN)
	    sprintf((char *)newval, " ++nobin");
	else
	    *newval = NUL;
	if (eap->force_ff != 0)
	    sprintf((char *)newval + STRLEN(newval), " ++ff=%s",
						    eap->cmd + eap->force_ff);
# ifdef FEAT_MBYTE
	if (eap->force_enc != 0)
	    sprintf((char *)newval + STRLEN(newval), " ++enc=%s",
						   eap->cmd + eap->force_enc);
# endif
	vimvars[VV_CMDARG].val = newval;
	return oldval;
    }

    vim_free(oldval);
    vimvars[VV_CMDARG].val = oldarg;
    return NULL;
}
#endif

/*
 * Get the value of internal variable "name".
 * Return OK or FAIL.
 */
    static int
get_var_var(name, len, retvar)
    char_u	*name;
    int		len;		/* length of "name" */
    VAR		retvar;		/* NULL when only checking existence */
{
    int		ret = OK;
    int		type = VAR_UNKNOWN;
    long	number = 1;
    char_u	*string = NULL;
    VAR		v;
    int		cc;
    int		i;

    /* truncate the name, so that we can use strcmp() */
    cc = name[len];
    name[len] = NUL;

    /*
     * Check for "b:changedtick".
     */
    if (STRCMP(name, "b:changedtick") == 0)
    {
	type = VAR_NUMBER;
	number = curbuf->b_changedtick;
    }

    /*
     * Check for built-in v: variables.
     */
    else if ((i = find_vim_var(name, len)) >= 0)
    {
	type = vimvars[i].type;
	number = (long)vimvars[i].val;
	string = vimvars[i].val;
    }

    /*
     * Check for user-defined variables.
     */
    else
    {
	v = find_var(name, FALSE);
	if (v != NULL)
	{
	    type = v->var_type;
	    number = v->var_val.var_number;
	    string = v->var_val.var_string;
	}
    }

    if (type == VAR_UNKNOWN)
    {
	if (retvar != NULL)
	    EMSG2(_("E121: Undefined variable: %s"), name);
	ret = FAIL;
    }
    else if (retvar != NULL)
    {
	retvar->var_type = type;
	if (type == VAR_NUMBER)
	    retvar->var_val.var_number = number;
	else if (type == VAR_STRING)
	{
	    if (string != NULL)
		string = vim_strsave(string);
	    retvar->var_val.var_string = string;
	}
    }

    name[len] = cc;

    return ret;
}

/*
 * Allocate memory for a variable, and make it emtpy (0 or NULL value).
 */
    static VAR
alloc_var()
{
    return (VAR)alloc_clear((unsigned)sizeof(var));
}

/*
 * Allocate memory for a variable, and assign a string to it.
 * The string "s" must have been allocated, it is consumed.
 * Return NULL for out of memory, the variable otherwise.
 */
    static VAR
alloc_string_var(s)
    char_u	*s;
{
    VAR	    retvar;

    retvar = alloc_var();
    if (retvar != NULL)
    {
	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = s;
    }
    else
	vim_free(s);
    return retvar;
}

/*
 * Free the memory for a variable.
 */
    static void
free_var(varp)
    VAR	    varp;
{
    if (varp != NULL)
    {
	if (varp->var_type == VAR_STRING)
	    vim_free(varp->var_val.var_string);
	vim_free(varp->var_name);
	vim_free(varp);
    }
}

/*
 * Free the memory for a variable value and set the value to NULL or 0.
 */
    static void
clear_var(varp)
    VAR	    varp;
{
    if (varp != NULL)
    {
	if (varp->var_type == VAR_STRING)
	{
	    vim_free(varp->var_val.var_string);
	    varp->var_val.var_string = NULL;
	}
	else
	    varp->var_val.var_number = 0;
    }
}

/*
 * Get the number value of a variable.
 * If it is a String variable, uses vim_str2nr().
 */
    static long
get_var_number(varp)
    VAR		varp;
{
    long	n;

    if (varp->var_type == VAR_NUMBER)
	return (long)(varp->var_val.var_number);
    else if (varp->var_type == VAR_UNKNOWN || varp->var_val.var_string == NULL)
	return 0L;
    else
    {
	vim_str2nr(varp->var_val.var_string, NULL, NULL, TRUE, TRUE, &n, NULL);
	return n;
    }
}

/*
 * Get the lnum from the first argument.  Also accepts ".", "$", etc.
 */
    static linenr_T
get_var_lnum(argvars)
    VAR		argvars;
{
    var		retvar;
    linenr_T	lnum;

    lnum = get_var_number(&argvars[0]);
    if (lnum == 0)  /* no valid number, try using line() */
    {
	retvar.var_type = VAR_NUMBER;
	f_line(argvars, &retvar);
	lnum = retvar.var_val.var_number;
	clear_var(&retvar);
    }
    return lnum;
}

/*
 * Get the string value of a variable.
 * If it is a Number variable, the number is converted into a string.
 * get_var_string() uses a single, static buffer.  YOU CAN ONLY USE IT ONCE!
 * get_var_string_buf() uses a given buffer.
 * If the String variable has never been set, return an empty string.
 * Never returns NULL;
 */
    static char_u *
get_var_string(varp)
    VAR	    varp;
{
    static char_u   mybuf[NUMBUFLEN];

    return get_var_string_buf(varp, mybuf);
}

    static char_u *
get_var_string_buf(varp, buf)
    VAR	    varp;
    char_u  *buf;
{
    if (varp->var_type == VAR_NUMBER)
    {
	sprintf((char *)buf, "%ld", (long)varp->var_val.var_number);
	return buf;
    }
    else if (varp->var_val.var_string == NULL)
	return (char_u *)"";
    else
	return varp->var_val.var_string;
}

/*
 * Find variable "name" in the list of variables.
 * Return a pointer to it if found, NULL if not found.
 */
    static VAR
find_var(name, writing)
    char_u	*name;
    int		writing;
{
    int		i;
    char_u	*varname;
    garray_T	*gap;

    /* Check for function arguments "a:" */
    if (name[0] == 'a' && name[1] == ':')
    {
	if (writing)
	{
	    EMSG2(_(e_readonlyvar), name);
	    return NULL;
	}
	name += 2;
	if (current_funccal == NULL)
	    return NULL;
	if (VIM_ISDIGIT(*name))
	{
	    i = atol((char *)name);
	    if (i == 0)					/* a:0 */
		return &current_funccal->a0_var;
	    i += current_funccal->func->args.ga_len;
	    if (i > current_funccal->argcount)		/* a:999 */
		return NULL;
	    return &(current_funccal->argvars[i - 1]);	/* a:1, a:2, etc. */
	}
	if (STRCMP(name, "firstline") == 0)
	    return &(current_funccal->firstline);
	if (STRCMP(name, "lastline") == 0)
	    return &(current_funccal->lastline);
	for (i = 0; i < current_funccal->func->args.ga_len; ++i)
	    if (STRCMP(name, ((char_u **)
			      (current_funccal->func->args.ga_data))[i]) == 0)
	    return &(current_funccal->argvars[i]);	/* a:name */
	return NULL;
    }

    gap = find_var_ga(name, &varname);
    if (gap == NULL)
	return NULL;
    return find_var_in_ga(gap, varname);
}

    static VAR
find_var_in_ga(gap, varname)
    garray_T	*gap;
    char_u	*varname;
{
    int	i;

    for (i = gap->ga_len; --i >= 0; )
	if (VAR_GAP_ENTRY(i, gap).var_name != NULL
		&& STRCMP(VAR_GAP_ENTRY(i, gap).var_name, varname) == 0)
	    break;
    if (i < 0)
	return NULL;
    return &VAR_GAP_ENTRY(i, gap);
}

/*
 * Find the growarray and start of name without ':' for a variable name.
 */
    static garray_T *
find_var_ga(name, varname)
    char_u  *name;
    char_u  **varname;
{
    if (name[1] != ':')
    {
	/* If not "x:name" there must not be any ":" in the name. */
	if (vim_strchr(name, ':') != NULL)
	    return NULL;
	*varname = name;
	if (current_funccal == NULL)
	    return &variables;			/* global variable */
	return &current_funccal->l_vars;	/* local function variable */
    }
    *varname = name + 2;
    if (*name == 'b')				/* buffer variable */
	return &curbuf->b_vars;
    if (*name == 'w')				/* window variable */
	return &curwin->w_vars;
    if (*name == 'g')				/* global variable */
	return &variables;
    if (*name == 'l' && current_funccal != NULL)/* local function variable */
	return &current_funccal->l_vars;
    if (*name == 's'				/* script variable */
	    && current_SID > 0 && current_SID <= ga_scripts.ga_len)
	return &SCRIPT_VARS(current_SID);
    return NULL;
}

/*
 * Get the string value of a (global/local) variable.
 * Returns NULL when it doesn't exist.
 */
    char_u *
get_var_value(name)
    char_u	*name;
{
    VAR		v;

    v = find_var(name, FALSE);
    if (v == NULL)
	return NULL;
    return get_var_string(v);
}

/*
 * Allocate a new growarry for a sourced script.  It will be used while
 * sourcing this script and when executing functions defined in the script.
 */
    void
new_script_vars(id)
    scid_T id;
{
    if (ga_grow(&ga_scripts, (int)(id - ga_scripts.ga_len)) == OK)
    {
	while (ga_scripts.ga_len < id)
	{
	    var_init(&SCRIPT_VARS(ga_scripts.ga_len + 1));
	    ++ga_scripts.ga_len;
	    --ga_scripts.ga_room;
	}
    }
}

/*
 * Initialize internal variables for use.
 */
    void
var_init(gap)
    garray_T *gap;
{
    ga_init2(gap, (int)sizeof(var), 4);
}

/*
 * Clean up a list of internal variables.
 */
    void
var_clear(gap)
    garray_T *gap;
{
    int	    i;

    for (i = gap->ga_len; --i >= 0; )
	var_free_one(&VAR_GAP_ENTRY(i, gap));
    ga_clear(gap);
}

    static void
var_free_one(v)
    VAR	    v;
{
    vim_free(v->var_name);
    v->var_name = NULL;
    if (v->var_type == VAR_STRING)
	vim_free(v->var_val.var_string);
    v->var_val.var_string = NULL;
}

/*
 * List the value of one internal variable.
 */
    static void
list_one_var(v, prefix)
    VAR		v;
    char_u	*prefix;
{
    list_one_var_a(prefix, v->var_name, v->var_type, get_var_string(v));
}

/*
 * List the value of one "v:" variable.
 */
    static void
list_vim_var(i)
    int		i;	/* index in vimvars[] */
{
    char_u	*p;
    char_u	numbuf[NUMBUFLEN];

    if (vimvars[i].type == VAR_NUMBER)
    {
	p = numbuf;
	sprintf((char *)p, "%ld", (long)vimvars[i].val);
    }
    else if (vimvars[i].val == NULL)
	p = (char_u *)"";
    else
	p = vimvars[i].val;
    list_one_var_a((char_u *)"v:", (char_u *)vimvars[i].name,
							  vimvars[i].type, p);
}

    static void
list_one_var_a(prefix, name, type, string)
    char_u	*prefix;
    char_u	*name;
    int		type;
    char_u	*string;
{
    msg_attr(prefix, 0);    /* don't use msg(), it overwrites "v:statusmsg" */
    if (name != NULL)	/* "a:" vars don't have a name stored */
	msg_puts(name);
    msg_putchar(' ');
    msg_advance(22);
    if (type == VAR_NUMBER)
	msg_putchar('#');
    else
	msg_putchar(' ');
    msg_outtrans(string);
}

/*
 * Set variable "name" to value in "varp".
 * If the variable already exists, the value is updated.
 * Otherwise the variable is created.
 */
    static void
set_var(name, varp)
    char_u	*name;
    VAR		varp;
{
    int		i;
    VAR		v;
    char_u	*varname;
    garray_T	*gap;

    /*
     * Handle setting internal v: variables.
     */
    i = find_vim_var(name, (int)STRLEN(name));
    if (i >= 0)
    {
	if (vimvars[i].flags & VV_RO)
	    EMSG2(_(e_readonlyvar), name);
	else
	{
	    if (vimvars[i].type == VAR_STRING)
	    {
		vim_free(vimvars[i].val);
		vimvars[i].val = vim_strsave(get_var_string(varp));
	    }
	    else
		vimvars[i].val = (char_u *)(long)varp->var_val.var_number;
	}
	return;
    }

    v = find_var(name, TRUE);
    if (v != NULL)	    /* existing variable, only need to free string */
    {
	if (v->var_type == VAR_STRING)
	    vim_free(v->var_val.var_string);
    }
    else		    /* add a new variable */
    {
	gap = find_var_ga(name, &varname);
	if (gap == NULL)    /* illegal name */
	{
	    EMSG2(_("E461: Illegal variable name: %s"), name);
	    return;
	}

	/* Try to use an empty entry */
	for (i = gap->ga_len; --i >= 0; )
	    if (VAR_GAP_ENTRY(i, gap).var_name == NULL)
		break;
	if (i < 0)	    /* need to allocate more room */
	{
	    if (ga_grow(gap, 1) == FAIL)
		return;
	    i = gap->ga_len;
	}
	v = &VAR_GAP_ENTRY(i, gap);
	if ((v->var_name = vim_strsave(varname)) == NULL)
	    return;
	if (i == gap->ga_len)
	{
	    ++gap->ga_len;
	    --gap->ga_room;
	}
    }
    copy_var(varp, v);
}

    static void
copy_var(from, to)
    VAR	from;
    VAR	to;
{
    to->var_type = from->var_type;
    if (from->var_type == VAR_STRING)
	to->var_val.var_string = vim_strsave(get_var_string(from));
    else
	to->var_val.var_number = from->var_val.var_number;
}

/*
 * ":echo expr1 ..."	print each argument separated with a space, add a
 *			newline at the end.
 * ":echon expr1 ..."	print each argument plain.
 */
    void
ex_echo(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    var		retvar;
    char_u	*p;
    int		needclr = TRUE;
    int		atstart = TRUE;

    if (eap->skip)
	++emsg_skip;
    while (*arg != NUL && *arg != '|' && *arg != '\n' && !got_int)
    {
	p = arg;
	if (eval1(&arg, &retvar, !eap->skip) == FAIL)
	{
	    /*
	     * Report the invalid expression unless the expression evaluation
	     * has been cancelled due to an aborting error, an interrupt, or an
	     * exception.
	     */
	    if (!aborting())
		EMSG2(_(e_invexpr2), p);
	    break;
	}
	if (!eap->skip)
	{
	    if (atstart)
	    {
		atstart = FALSE;
		/* Call msg_start() after eval1(), evaluating the expression
		 * may cause a message to appear. */
		if (eap->cmdidx == CMD_echo)
		    msg_start();
	    }
	    else if (eap->cmdidx == CMD_echo)
		msg_puts_attr((char_u *)" ", echo_attr);
	    for (p = get_var_string(&retvar); *p != NUL && !got_int; ++p)
		if (*p == '\n' || *p == '\r' || *p == TAB)
		{
		    if (*p != TAB && needclr)
		    {
			/* remove any text still there from the command */
			msg_clr_eos();
			needclr = FALSE;
		    }
		    msg_putchar_attr(*p, echo_attr);
		}
		else
		{
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			int i = (*mb_ptr2len_check)(p);

			(void)msg_outtrans_len_attr(p, i, echo_attr);
			p += i - 1;
		    }
		    else
#endif
			(void)msg_outtrans_len_attr(p, 1, echo_attr);
		}
	}
	clear_var(&retvar);
	arg = skipwhite(arg);
    }
    eap->nextcmd = check_nextcmd(arg);

    if (eap->skip)
	--emsg_skip;
    else
    {
	/* remove text that may still be there from the command */
	if (needclr)
	    msg_clr_eos();
	if (eap->cmdidx == CMD_echo)
	    msg_end();
    }
}

/*
 * ":echohl {name}".
 */
    void
ex_echohl(eap)
    exarg_T	*eap;
{
    int		id;

    id = syn_name2id(eap->arg);
    if (id == 0)
	echo_attr = 0;
    else
	echo_attr = syn_id2attr(id);
}

/*
 * ":execute expr1 ..."	execute the result of an expression.
 * ":echomsg expr1 ..."	Print a message
 * ":echoerr expr1 ..."	Print an error
 * Each gets spaces around each argument and a newline at the end for
 * echo commands
 */
    void
ex_execute(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    var		retvar;
    int		ret = OK;
    char_u	*p;
    garray_T	ga;
    int		len;
    int		save_did_emsg;

    ga_init2(&ga, 1, 80);

    if (eap->skip)
	++emsg_skip;
    while (*arg != NUL && *arg != '|' && *arg != '\n')
    {
	p = arg;
	if (eval1(&arg, &retvar, !eap->skip) == FAIL)
	{
	    /*
	     * Report the invalid expression unless the expression evaluation
	     * has been cancelled due to an aborting error, an interrupt, or an
	     * exception.
	     */
	    if (!aborting())
		EMSG2(_(e_invexpr2), p);
	    ret = FAIL;
	    break;
	}

	if (!eap->skip)
	{
	    p = get_var_string(&retvar);
	    len = (int)STRLEN(p);
	    if (ga_grow(&ga, len + 2) == FAIL)
	    {
		clear_var(&retvar);
		ret = FAIL;
		break;
	    }
	    if (ga.ga_len)
	    {
		((char_u *)(ga.ga_data))[ga.ga_len++] = ' ';
		--ga.ga_room;
	    }
	    STRCPY((char_u *)(ga.ga_data) + ga.ga_len, p);
	    ga.ga_room -= len;
	    ga.ga_len += len;
	}

	clear_var(&retvar);
	arg = skipwhite(arg);
    }

    if (ret != FAIL && ga.ga_data != NULL)
    {
	if (eap->cmdidx == CMD_echomsg)
	    MSG_ATTR(ga.ga_data, echo_attr);
	else if (eap->cmdidx == CMD_echoerr)
	{
	    /* We don't want to abort following commands, restore did_emsg. */
	    save_did_emsg = did_emsg;
	    EMSG((char_u *)ga.ga_data);
	    if (!force_abort)
		did_emsg = save_did_emsg;
	}
	else if (eap->cmdidx == CMD_execute)
	    do_cmdline((char_u *)ga.ga_data,
		       eap->getline, eap->cookie, DOCMD_NOWAIT|DOCMD_VERBOSE);
    }

    ga_clear(&ga);

    if (eap->skip)
	--emsg_skip;

    eap->nextcmd = check_nextcmd(arg);
}

/*
 * Skip over the name of an option: "&option", "&g:option" or "&l:option".
 * "arg" points to the "&" or '+' when called, to "option" when returning.
 * Returns NULL when no option name found.  Otherwise pointer to the char
 * after the option name.
 */
    static char_u *
find_option_end(arg, opt_flags)
    char_u	**arg;
    int		*opt_flags;
{
    char_u	*p = *arg;

    ++p;
    if (*p == 'g' && p[1] == ':')
    {
	*opt_flags = OPT_GLOBAL;
	p += 2;
    }
    else if (*p == 'l' && p[1] == ':')
    {
	*opt_flags = OPT_LOCAL;
	p += 2;
    }
    else
	*opt_flags = 0;

    if (!ASCII_ISALPHA(*p))
	return NULL;
    *arg = p;

    if (p[0] == 't' && p[1] == '_' && p[2] != NUL && p[3] != NUL)
	p += 4;	    /* termcap option */
    else
	while (ASCII_ISALPHA(*p))
	    ++p;
    return p;
}

/*
 * ":function"
 */
    void
ex_function(eap)
    exarg_T	*eap;
{
    char_u	*theline;
    int		j;
    int		c;
#ifdef FEAT_MAGIC_BRACES
    int		saved_did_emsg;
#endif
    char_u	*name = NULL;
    char_u	*p;
    char_u	*arg;
    garray_T	newargs;
    garray_T	newlines;
    int		varargs = FALSE;
    int		mustend = FALSE;
    int		flags = 0;
    ufunc_T	*fp;
    int		indent;
    int		nesting;
    char_u	*skip_until = NULL;
    static char_u e_funcexts[] = N_("E122: Function %s already exists, add ! to replace it");

    /*
     * ":function" without argument: list functions.
     */
    if (ends_excmd(*eap->arg))
    {
	if (!eap->skip)
	    for (fp = firstfunc; fp != NULL && !got_int; fp = fp->next)
		list_func_head(fp, FALSE);
	eap->nextcmd = check_nextcmd(eap->arg);
	return;
    }

    p = eap->arg;
    name = trans_function_name(&p, eap->skip, FALSE);
    if (name == NULL && !eap->skip)
    {
	/*
	 * Return on an invalid expression in braces, unless the expression
	 * evaluation has been cancelled due to an aborting error, an
	 * interrupt, or an exception.
	 */
	if (!aborting())
	    return;
	else
	    eap->skip = TRUE;
    }
#ifdef FEAT_MAGIC_BRACES
    /* An error in a function call during evaluation of an expression in magic
     * braces should not cause the function not to be defined. */
    saved_did_emsg = did_emsg;
    did_emsg = FALSE;
#endif

    /*
     * ":function func" with only function name: list function.
     */
    if (vim_strchr(p, '(') == NULL)
    {
	if (!ends_excmd(*skipwhite(p)))
	{
	    EMSG(_(e_trailing));
	    goto erret_name;
	}
	eap->nextcmd = check_nextcmd(p);
	if (eap->nextcmd != NULL)
	    *p = NUL;
	if (!eap->skip && !got_int)
	{
	    fp = find_func(name);
	    if (fp != NULL)
	    {
		list_func_head(fp, TRUE);
		for (j = 0; j < fp->lines.ga_len && !got_int; ++j)
		{
		    msg_putchar('\n');
		    msg_outnum((long)(j + 1));
		    if (j < 9)
			msg_putchar(' ');
		    if (j < 99)
			msg_putchar(' ');
		    msg_prt_line(FUNCLINE(fp, j));
		    out_flush();	/* show a line at a time */
		    ui_breakcheck();
		}
		if (!got_int)
		{
		    msg_putchar('\n');
		    msg_puts((char_u *)"   endfunction");
		}
	    }
	    else
		EMSG2(_("E123: Undefined function: %s"), eap->arg);
	}
	goto erret_name;
    }

    /*
     * ":function name(arg1, arg2)" Define function.
     */
    p = skipwhite(p);
    if (*p != '(')
    {
	if (!eap->skip)
	{
	    EMSG2(_("E124: Missing '(': %s"), eap->arg);
	    goto erret_name;
	}
	/* attempt to continue by skipping some text */
	if (vim_strchr(p, '(') != NULL)
	    p = vim_strchr(p, '(');
    }
    p = skipwhite(p + 1);

    ga_init2(&newargs, (int)sizeof(char_u *), 3);
    ga_init2(&newlines, (int)sizeof(char_u *), 3);

    /*
     * Isolate the arguments: "arg1, arg2, ...)"
     */
    while (*p != ')')
    {
	if (p[0] == '.' && p[1] == '.' && p[2] == '.')
	{
	    varargs = TRUE;
	    p += 3;
	    mustend = TRUE;
	}
	else
	{
	    arg = p;
	    while (ASCII_ISALNUM(*p) || *p == '_')
		++p;
	    if (arg == p || isdigit(*arg)
		    || (p - arg == 9 && STRNCMP(arg, "firstline", 9) == 0)
		    || (p - arg == 8 && STRNCMP(arg, "lastline", 8) == 0))
	    {
		if (!eap->skip)
		    EMSG2(_("E125: Illegal argument: %s"), arg);
		break;
	    }
	    if (ga_grow(&newargs, 1) == FAIL)
		goto erret;
	    c = *p;
	    *p = NUL;
	    arg = vim_strsave(arg);
	    if (arg == NULL)
		goto erret;
	    ((char_u **)(newargs.ga_data))[newargs.ga_len] = arg;
	    *p = c;
	    newargs.ga_len++;
	    newargs.ga_room--;
	    if (*p == ',')
		++p;
	    else
		mustend = TRUE;
	}
	p = skipwhite(p);
	if (mustend && *p != ')')
	{
	    if (!eap->skip)
		EMSG2(_(e_invarg2), eap->arg);
	    break;
	}
    }
    ++p;	/* skip the ')' */

    /* find extra arguments "range" and "abort" */
    for (;;)
    {
	p = skipwhite(p);
	if (STRNCMP(p, "range", 5) == 0)
	{
	    flags |= FC_RANGE;
	    p += 5;
	}
	else if (STRNCMP(p, "abort", 5) == 0)
	{
	    flags |= FC_ABORT;
	    p += 5;
	}
	else
	    break;
    }

    if (*p != NUL && *p != '"' && *p != '\n' && !eap->skip && !did_emsg)
	EMSG(_(e_trailing));

    /*
     * Read the body of the function, until ":endfunction" is found.
     */
    if (KeyTyped)
    {
	/* Check if the function already exists, don't let the user type the
	 * whole function before telling him it doesn't work!  For a script we
	 * need to skip the body to be able to find what follows. */
	if (!eap->skip && !eap->forceit && find_func(name) != NULL)
	    EMSG2(_(e_funcexts), name);

	msg_putchar('\n');	    /* don't overwrite the function name */
	cmdline_row = msg_row;
    }

    indent = 2;
    nesting = 0;
    for (;;)
    {
	msg_scroll = TRUE;
	need_wait_return = FALSE;
	if (eap->getline == NULL)
	    theline = getcmdline(':', 0L, indent);
	else
	    theline = eap->getline(':', eap->cookie, indent);
	if (KeyTyped)
	    lines_left = Rows - 1;
	if (theline == NULL)
	{
	    EMSG(_("E126: Missing :endfunction"));
	    goto erret;
	}

	if (skip_until != NULL)
	{
	    /* between ":append" and "." and between ":python <<EOF" and "EOF"
	     * don't check for ":endfunc". */
	    if (STRCMP(theline, skip_until) == 0)
	    {
		vim_free(skip_until);
		skip_until = NULL;
	    }
	}
	else
	{
	    /* skip ':' and blanks*/
	    for (p = theline; vim_iswhite(*p) || *p == ':'; ++p)
		;

	    /* Check for "endfunction" (should be more strict...). */
	    if (STRNCMP(p, "endf", 4) == 0 && nesting-- == 0)
	    {
		vim_free(theline);
		break;
	    }

	    /* Increase indent inside "if", "while", and "try", decrease
	     * at "end". */
	    if (indent > 2 && STRNCMP(p, "end", 3) == 0)
		indent -= 2;
	    else if (STRNCMP(p, "if", 2) == 0 || STRNCMP(p, "wh", 2) == 0
		    || STRNCMP(p, "try", 3) == 0)
		indent += 2;

	    /* Check for defining a function inside this function. */
	    if (STRNCMP(p, "fu", 2) == 0)
	    {
		p = skipwhite(skiptowhite(p));
		p += eval_fname_script(p);
		if (ASCII_ISALPHA(*p))
		{
		    vim_free(trans_function_name(&p, TRUE, FALSE));
		    if (*skipwhite(p) == '(')
		    {
			++nesting;
			indent += 2;
		    }
		}
	    }

	    /* Check for ":append" or ":insert". */
	    p = skip_range(p, NULL);
	    if ((p[0] == 'a' && (!ASCII_ISALPHA(p[1]) || p[1] == 'p'))
		    || (p[0] == 'i'
			&& (!ASCII_ISALPHA(p[1]) || (p[1] == 'n'
				&& (!ASCII_ISALPHA(p[2]) || (p[2] == 's'))))))
		skip_until = vim_strsave((char_u *)".");

	    /* Check for ":python <<EOF", ":tcl <<EOF", etc. */
	    arg = skipwhite(skiptowhite(p));
	    if (arg[0] == '<' && arg[1] =='<'
		    && ((p[0] == 'p' && p[1] == 'y'
				    && (!ASCII_ISALPHA(p[2]) || p[2] == 't'))
			|| (p[0] == 'p' && p[1] == 'e'
				    && (!ASCII_ISALPHA(p[2]) || p[2] == 'r'))
			|| (p[0] == 't' && p[1] == 'c'
				    && (!ASCII_ISALPHA(p[2]) || p[2] == 'l'))
			|| (p[0] == 'r' && p[1] == 'u' && p[2] == 'b'
				    && (!ASCII_ISALPHA(p[3]) || p[3] == 'y'))
			))
	    {
		/* ":python <<" continues until a dot, like ":append" */
		p = skipwhite(arg + 2);
		if (*p == NUL)
		    skip_until = vim_strsave((char_u *)".");
		else
		    skip_until = vim_strsave(p);
	    }
	}

	/* Add the line to the function. */
	if (ga_grow(&newlines, 1) == FAIL)
	    goto erret;
	((char_u **)(newlines.ga_data))[newlines.ga_len] = theline;
	newlines.ga_len++;
	newlines.ga_room--;
    }

    /* Don't define the function when skipping commands or when an error was
     * detected. */
    if (eap->skip || did_emsg)
	goto erret;

    /*
     * If there are no errors, add the function
     */
    fp = find_func(name);
    if (fp != NULL)
    {
	if (!eap->forceit)
	{
	    EMSG2(_(e_funcexts), name);
	    goto erret;
	}
	if (fp->calls)
	{
	    EMSG2(_("E127: Cannot redefine function %s: It is in use"), name);
	    goto erret;
	}
	/* redefine existing function */
	ga_clear_strings(&(fp->args));
	ga_clear_strings(&(fp->lines));
	vim_free(name);
    }
    else
    {
	fp = (ufunc_T *)alloc((unsigned)sizeof(ufunc_T));
	if (fp == NULL)
	    goto erret;
	/* insert the new function in the function list */
	fp->next = firstfunc;
	firstfunc = fp;
	fp->name = name;
    }
    fp->args = newargs;
    fp->lines = newlines;
    fp->varargs = varargs;
    fp->flags = flags;
    fp->calls = 0;
    fp->script_ID = current_SID;
#ifdef FEAT_MAGIC_BRACES
    did_emsg |= saved_did_emsg;
#endif
    vim_free(skip_until);
    return;

erret:
    vim_free(skip_until);
    ga_clear_strings(&newargs);
    ga_clear_strings(&newlines);
erret_name:
    vim_free(name);
#ifdef FEAT_MAGIC_BRACES
    did_emsg |= saved_did_emsg;
#endif
}

/*
 * Get a function name, translating "<SID>" and "<SNR>".
 * Returns the function name in allocated memory, or NULL for failure.
 * Advances "pp" to just after the function name (if no error).
 */
    static char_u *
trans_function_name(pp, skip, internal)
    char_u	**pp;
    int		skip;		/* only find the end, don't evaluate */
    int		internal;	/* TRUE if internal function name OK */
{
    char_u	*name;
    char_u	*start;
    char_u	*end;
    int		lead;
    char_u	sid_buf[20];
    char_u	*temp_string = NULL;
    char_u	*expr_start, *expr_end;
    int		len;

    /* A name starting with "<SID>" or "<SNR>" is local to a script. */
    start = *pp;
    lead = eval_fname_script(start);
    if (lead > 0)
	start += lead;
    end = find_name_end(start, &expr_start, &expr_end);
    if (end == start)
    {
	if (!skip)
	    EMSG(_("E129: Function name required"));
	return NULL;
    }
#ifdef FEAT_MAGIC_BRACES
    if (expr_start != NULL && !skip)
    {
	/* expand magic curlies */
	temp_string = make_expanded_name(start, expr_start, expr_end, end);
	if (temp_string == NULL)
	{
	    /*
	     * Report an invalid expression in braces, unless the expression
	     * evaluation has been cancelled due to an aborting error, an
	     * interrupt, or an exception.
	     */
	    if (!aborting())
		EMSG2(_(e_invarg2), start);
	    else
		*pp = end;
	    return NULL;
	}
	start = temp_string;
	len = (int)STRLEN(temp_string);
    }
    else
#endif
	len = (int)(end - start);

    /*
     * Copy the function name to allocated memory.
     * Accept <SID>name() inside a script, translate into <SNR>123_name().
     * Accept <SNR>123_name() outside a script.
     */
    if (skip)
	lead = 0;	/* do nothing */
    else if (lead > 0)
    {
	lead = 3;
	if (eval_fname_sid(*pp))	/* If it's "<SID>" */
	{
	    if (current_SID <= 0)
	    {
		EMSG(_(e_usingsid));
		return NULL;
	    }
	    sprintf((char *)sid_buf, "%ld_", (long)current_SID);
	    lead += (int)STRLEN(sid_buf);
	}
    }
    else if (!internal && !ASCII_ISUPPER(*start))
    {
	EMSG2(_("E128: Function name must start with a capital: %s"), start);
	return NULL;
    }
    name = alloc((unsigned)(len + lead + 1));
    if (name != NULL)
    {
	if (lead > 0)
	{
	    name[0] = K_SPECIAL;
	    name[1] = KS_EXTRA;
	    name[2] = (int)KE_SNR;
	    if (eval_fname_sid(*pp))	/* If it's "<SID>" */
		STRCPY(name + 3, sid_buf);
	}
	mch_memmove(name + lead, start, (size_t)len);
	name[len + lead] = NUL;
    }
    *pp = end;

    vim_free(temp_string);
    return name;
}

/*
 * Return 5 if "p" starts with "<SID>" or "<SNR>" (ignoring case).
 * Return 2 if "p" starts with "s:".
 * Return 0 otherwise.
 */
    static int
eval_fname_script(p)
    char_u	*p;
{
    if (p[0] == '<' && (STRNICMP(p + 1, "SID>", 4) == 0
					  || STRNICMP(p + 1, "SNR>", 4) == 0))
	return 5;
    if (p[0] == 's' && p[1] == ':')
	return 2;
    return 0;
}

/*
 * Return TRUE if "p" starts with "<SID>" or "s:".
 * Only works if eval_fname_script() returned non-zero for "p"!
 */
    static int
eval_fname_sid(p)
    char_u	*p;
{
    return (*p == 's' || TOUPPER_ASC(p[2]) == 'I');
}

/*
 * List the head of the function: "name(arg1, arg2)".
 */
    static void
list_func_head(fp, indent)
    ufunc_T	*fp;
    int		indent;
{
    int		j;

    msg_start();
    if (indent)
	MSG_PUTS("   ");
    MSG_PUTS("function ");
    if (fp->name[0] == K_SPECIAL)
    {
	MSG_PUTS_ATTR("<SNR>", hl_attr(HLF_8));
	msg_puts(fp->name + 3);
    }
    else
	msg_puts(fp->name);
    msg_putchar('(');
    for (j = 0; j < fp->args.ga_len; ++j)
    {
	if (j)
	    MSG_PUTS(", ");
	msg_puts(FUNCARG(fp, j));
    }
    if (fp->varargs)
    {
	if (j)
	    MSG_PUTS(", ");
	MSG_PUTS("...");
    }
    msg_putchar(')');
}

/*
 * Find a function by name, return pointer to it in ufuncs.
 * Return NULL for unknown function.
 */
    static ufunc_T *
find_func(name)
    char_u	*name;
{
    ufunc_T	*fp;

    for (fp = firstfunc; fp != NULL; fp = fp->next)
	if (STRCMP(name, fp->name) == 0)
	    break;
    return fp;
}

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)

/*
 * Function given to ExpandGeneric() to obtain the list of user defined
 * function names.
 */
    char_u *
get_user_func_name(xp, idx)
    expand_T	*xp;
    int		idx;
{
    static ufunc_T *fp = NULL;

    if (idx == 0)
	fp = firstfunc;
    if (fp != NULL)
    {
	if (STRLEN(fp->name) + 4 >= IOSIZE)
	    return fp->name;	/* prevents overflow */

	cat_func_name(IObuff, fp);
	if (xp->xp_context != EXPAND_USER_FUNC)
	{
	    STRCAT(IObuff, "(");
	    if (!fp->varargs && fp->args.ga_len == 0)
		STRCAT(IObuff, ")");
	}

	fp = fp->next;
	return IObuff;
    }
    return NULL;
}

#endif /* FEAT_CMDL_COMPL */

/*
 * Copy the function name of "fp" to buffer "buf".
 * "buf" must be able to hold the function name plus three bytes.
 * Takes care of script-local function names.
 */
    static void
cat_func_name(buf, fp)
    char_u	*buf;
    ufunc_T	*fp;
{
    if (fp->name[0] == K_SPECIAL)
    {
	STRCPY(buf, "<SNR>");
	STRCAT(buf, fp->name + 3);
    }
    else
	STRCPY(buf, fp->name);
}

/*
 * ":delfunction {name}"
 */
    void
ex_delfunction(eap)
    exarg_T	*eap;
{
    ufunc_T	*fp = NULL, *pfp;
    char_u	*p;
    char_u	*name;

    p = eap->arg;
    name = trans_function_name(&p, eap->skip, FALSE);
    if (name == NULL)
	return;
    if (!ends_excmd(*skipwhite(p)))
    {
	vim_free(name);
	EMSG(_(e_trailing));
	return;
    }
    eap->nextcmd = check_nextcmd(p);
    if (eap->nextcmd != NULL)
	*p = NUL;

    if (!eap->skip)
	fp = find_func(name);
    vim_free(name);

    if (!eap->skip)
    {
	if (fp == NULL)
	{
	    EMSG2(_("E130: Undefined function: %s"), eap->arg);
	    return;
	}
	if (fp->calls)
	{
	    EMSG2(_("E131: Cannot delete function %s: It is in use"), eap->arg);
	    return;
	}

	/* clear this function */
	vim_free(fp->name);
	ga_clear_strings(&(fp->args));
	ga_clear_strings(&(fp->lines));

	/* remove the function from the function list */
	if (firstfunc == fp)
	    firstfunc = fp->next;
	else
	{
	    for (pfp = firstfunc; pfp != NULL; pfp = pfp->next)
		if (pfp->next == fp)
		{
		    pfp->next = fp->next;
		    break;
		}
	}
	vim_free(fp);
    }
}

/*
 * Call a user function.
 */
    static void
call_user_func(fp, argcount, argvars, retvar, firstline, lastline)
    ufunc_T	*fp;		/* pointer to function */
    int		argcount;	/* nr of args */
    VAR		argvars;	/* arguments */
    VAR		retvar;		/* return value */
    linenr_T	firstline;	/* first line of range */
    linenr_T	lastline;	/* last line of range */
{
    char_u		*save_sourcing_name;
    linenr_T		save_sourcing_lnum;
    scid_T		save_current_SID;
    struct funccall	fc;
    struct funccall	*save_fcp = current_funccal;
    int			save_did_emsg;
    static int		depth = 0;

    /* If depth of calling is getting too high, don't execute the function */
    if (depth >= p_mfd)
    {
	EMSG(_("E132: Function call depth is higher than 'maxfuncdepth'"));
	retvar->var_type = VAR_NUMBER;
	retvar->var_val.var_number = -1;
	return;
    }
    ++depth;

    line_breakcheck();		/* check for CTRL-C hit */

    /* set local variables */
    var_init(&fc.l_vars);
    fc.func = fp;
    fc.argcount = argcount;
    fc.argvars = argvars;
    fc.retvar = retvar;
    retvar->var_val.var_number = 0;
    fc.linenr = 0;
    fc.returned = FALSE;
    fc.level = ex_nesting_level;
    fc.a0_var.var_type = VAR_NUMBER;
    fc.a0_var.var_val.var_number = argcount - fp->args.ga_len;
    fc.a0_var.var_name = NULL;
    current_funccal = &fc;
    fc.firstline.var_type = VAR_NUMBER;
    fc.firstline.var_val.var_number = firstline;
    fc.firstline.var_name = NULL;
    fc.lastline.var_type = VAR_NUMBER;
    fc.lastline.var_val.var_number = lastline;
    fc.lastline.var_name = NULL;
    /* Check if this function has a breakpoint. */
    fc.breakpoint = dbg_find_breakpoint(FALSE, fp->name, (linenr_T)0);
    fc.dbg_tick = debug_tick;

    /* Don't redraw while executing the function. */
    ++RedrawingDisabled;
    save_sourcing_name = sourcing_name;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 1;
    sourcing_name = alloc((unsigned)((save_sourcing_name == NULL ? 0
		: STRLEN(save_sourcing_name)) + STRLEN(fp->name) + 13));
    if (sourcing_name != NULL)
    {
	if (save_sourcing_name != NULL
			  && STRNCMP(save_sourcing_name, "function ", 9) == 0)
	    sprintf((char *)sourcing_name, "%s..", save_sourcing_name);
	else
	    STRCPY(sourcing_name, "function ");
	cat_func_name(sourcing_name + STRLEN(sourcing_name), fp);

	if (p_verbose >= 12)
	{
	    ++no_wait_return;
	    msg_scroll = TRUE;	    /* always scroll up, don't overwrite */
	    msg_str((char_u *)_("calling %s"), sourcing_name);
	    if (p_verbose >= 14)
	    {
		int	i;
		char_u	buf[MSG_BUF_LEN];

		msg_puts((char_u *)"(");
		for (i = 0; i < argcount; ++i)
		{
		    if (i > 0)
			msg_puts((char_u *)", ");
		    if (argvars[i].var_type == VAR_NUMBER)
			msg_outnum((long)argvars[i].var_val.var_number);
		    else
		    {
			trunc_string(get_var_string(&argvars[i]),
							    buf, MSG_BUF_LEN);
			msg_puts((char_u *)"\"");
			msg_puts(buf);
			msg_puts((char_u *)"\"");
		    }
		}
		msg_puts((char_u *)")");
	    }
	    msg_puts((char_u *)"\n");   /* don't overwrite this either */
	    cmdline_row = msg_row;
	    --no_wait_return;
	}
    }
    save_current_SID = current_SID;
    current_SID = fp->script_ID;
    save_did_emsg = did_emsg;
    did_emsg = FALSE;

    /* call do_cmdline() to execute the lines */
    do_cmdline(NULL, get_func_line, (void *)&fc,
				     DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT);

    --RedrawingDisabled;

    /* when the function was aborted because of an error, return -1 */
    if ((did_emsg && (fp->flags & FC_ABORT)) || retvar->var_type == VAR_UNKNOWN)
    {
	clear_var(retvar);
	retvar->var_type = VAR_NUMBER;
	retvar->var_val.var_number = -1;
    }

    /* when being verbose, mention the return value */
    if (p_verbose >= 12)
    {
	char_u	*sn, *val;

	++no_wait_return;
	msg_scroll = TRUE;	    /* always scroll up, don't overwrite */

	/* Make sure the output fits in IObuff. */
	sn = sourcing_name;
	if (STRLEN(sourcing_name) > IOSIZE / 2 - 50)
	    sn = sourcing_name + STRLEN(sourcing_name) - (IOSIZE / 2 - 50);

	if (aborting())
	    smsg((char_u *)_("%s aborted"), sn);
	else if (fc.retvar->var_type == VAR_NUMBER)
	    smsg((char_u *)_("%s returning #%ld"), sn,
				       (long)fc.retvar->var_val.var_number);
	else if (fc.retvar->var_type == VAR_STRING)
	{
	    val = get_var_string(fc.retvar);
	    if (STRLEN(val) > IOSIZE / 2 - 50)
		val = val + STRLEN(val) - (IOSIZE / 2 - 50);
	    smsg((char_u *)_("%s returning \"%s\""), sn, val);
	}
	msg_puts((char_u *)"\n");   /* don't overwrite this either */
	cmdline_row = msg_row;
	--no_wait_return;
    }

    vim_free(sourcing_name);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    current_SID = save_current_SID;

    if (p_verbose >= 12 && sourcing_name != NULL)
    {
	++no_wait_return;
	msg_scroll = TRUE;	    /* always scroll up, don't overwrite */
	msg_str((char_u *)_("continuing in %s"), sourcing_name);
	msg_puts((char_u *)"\n");   /* don't overwrite this either */
	cmdline_row = msg_row;
	--no_wait_return;
    }

    did_emsg |= save_did_emsg;
    current_funccal = save_fcp;

    var_clear(&fc.l_vars);		/* free all local variables */
    --depth;
}

/*
 * ":return [expr]"
 */
    void
ex_return(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    var		retvar;
    int		returning = FALSE;

    if (current_funccal == NULL)
    {
	EMSG(_("E133: :return not inside a function"));
	return;
    }

    if (eap->skip)
	++emsg_skip;

    eap->nextcmd = NULL;
    if ((*arg != NUL && *arg != '|' && *arg != '\n')
	    && eval0(arg, &retvar, &eap->nextcmd, !eap->skip) != FAIL)
    {
	if (!eap->skip)
	    returning = do_return(eap, FALSE, TRUE, &retvar);
	else
	    clear_var(&retvar);
    }
    /* It's safer to return also on error. */
    else if (!eap->skip)
    {
	/*
	 * Return unless the expression evaluation has been cancelled due to an
	 * aborting error, an interrupt, or an exception.
	 */
	if (!aborting())
	    returning = do_return(eap, FALSE, TRUE, NULL);
    }

    /* When skipping or the return gets pending, advance to the next command
     * in this line (!returning).  Otherwise, ignore the rest of the line.
     * Following lines will be ignored by get_func_line(). */
    if (returning)
	eap->nextcmd = NULL;
    else if (eap->nextcmd == NULL)	    /* no argument */
	eap->nextcmd = check_nextcmd(arg);

    if (eap->skip)
	--emsg_skip;
}

/*
 * Return from a function.  Possibly makes the return pending.  Also called
 * for a pending return at the ":endtry" or after returning from an extra
 * do_cmdline().  "reanimate" is used in the latter case.  "is_cmd" is set
 * when called due to a ":return" command.  "value" may point to a variable
 * with the return value.  Returns TRUE when the return can be carried out,
 * FALSE when the return gets pending.
 */
    int
do_return(eap, reanimate, is_cmd, value)
    exarg_T	*eap;
    int		reanimate;
    int		is_cmd;
    void	*value;
{
    int		idx;
    struct condstack *cstack = eap->cstack;

    if (reanimate)
	/* Undo the return. */
	current_funccal->returned = FALSE;

    /*
     * Cleanup (and inactivate) conditionals, but stop when a try conditional
     * not in its finally clause (which then is to be executed next) is found.
     * In this case, make the ":return" pending for execution at the ":endtry".
     * Otherwise, return normally.
     */
    idx = cleanup_conditionals(eap->cstack, 0, TRUE);
    if (idx >= 0)
    {
	cstack->cs_pending[idx] = CSTP_RETURN;

	if (!is_cmd && !reanimate)
	    /* A pending return again gets pending.  "value" points to an
	     * allocated variable with the value of the original ":return"'s
	     * argument if present or is NULL else. */
	    cstack->cs_retvar[idx] = value;
	else
	{
	    /* When undoing a return in order to make it pending, get the stored
	     * return value. */
	    if (reanimate)
		value = current_funccal->retvar;

	    if (value != NULL)
	    {
		/* Store the value of the pending return. */
		if ((cstack->cs_retvar[idx] = alloc_var()) != NULL)
		    *(VAR)cstack->cs_retvar[idx] = *(VAR)value;
		else
		    EMSG(_(e_outofmem));
	    }
	    else
		cstack->cs_retvar[idx] = NULL;

	    if (reanimate)
	    {
		/* The pending return value could be overwritten by a ":return"
		 * without argument in a finally clause; reset the default
		 * return value. */
		current_funccal->retvar->var_type = VAR_NUMBER;
		current_funccal->retvar->var_val.var_number = 0;
	    }
	}
	report_make_pending(CSTP_RETURN, value);
    }
    else
    {
	current_funccal->returned = TRUE;

	/* If the return is carried out now, store the return value.  For
	 * a return immediately after reanimation, the value is already
	 * there. */
	if (!reanimate && value != NULL)
	{
	    clear_var(current_funccal->retvar);
	    *current_funccal->retvar = *(VAR)value;
	    if (!is_cmd)
		vim_free(value);
	}
    }

    return idx < 0;
}

/*
 * Free the variable with a pending return value.
 */
    void
discard_pending_return(retvar)
    void	*retvar;
{
    /* The variable was copied from one with an undefined var_name.  So we can't
     * use free_var() to clear and free it. */
    clear_var((VAR)retvar);
    vim_free(retvar);
}

/*
 * Generate a return command for producing the value of "retvar".  The result
 * is an allocated string.  Used by report_pending() for verbose messages.
 */
    char_u *
get_return_cmd(retvar)
    void	*retvar;
{
    char_u	*s = IObuff;

    if (retvar == NULL || ((VAR)retvar)->var_type == VAR_UNKNOWN)
	s = (char_u *)":return";
    else if (((VAR)retvar)->var_type == VAR_STRING)
	sprintf((char *)IObuff, ":return \"%s\"",
		((VAR)retvar)->var_val.var_string);
    else
	sprintf((char *)IObuff, ":return %ld",
		(long)(((VAR)retvar)->var_val.var_number));
    return vim_strsave(s);
}

/*
 * Get next function line.
 * Called by do_cmdline() to get the next line.
 * Returns allocated string, or NULL for end of function.
 */
/* ARGSUSED */
    char_u *
get_func_line(c, cookie, indent)
    int	    c;		    /* not used */
    void    *cookie;
    int	    indent;	    /* not used */
{
    struct funccall	*fcp = (struct funccall *)cookie;
    char_u		*retval;
    garray_T		*gap;  /* growarray with function lines */

    /* If breakpoints have been added/deleted need to check for it. */
    if (fcp->dbg_tick != debug_tick)
    {
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fcp->func->name,
							       sourcing_lnum);
	fcp->dbg_tick = debug_tick;
    }

    gap = &fcp->func->lines;
    if ((fcp->func->flags & FC_ABORT) && did_emsg && !aborted_in_try())
	retval = NULL;
    else if (fcp->returned || fcp->linenr >= gap->ga_len)
	retval = NULL;
    else
    {
	retval = vim_strsave(((char_u **)(gap->ga_data))[fcp->linenr++]);
	sourcing_lnum = fcp->linenr;
    }

    /* Did we encounter a breakpoint? */
    if (fcp->breakpoint != 0 && fcp->breakpoint <= sourcing_lnum)
    {
	dbg_breakpoint(fcp->func->name, sourcing_lnum);
	/* Find next breakpoint. */
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fcp->func->name,
							       sourcing_lnum);
	fcp->dbg_tick = debug_tick;
    }

    return retval;
}

/*
 * Return TRUE if the currently active function should be ended, because a
 * return was encountered or an error occured.  Used inside a ":while".
 */
    int
func_has_ended(cookie)
    void    *cookie;
{
    struct funccall  *fcp = (struct funccall *)cookie;

    /* Ignore the "abort" flag if the abortion behavior has been changed due to
     * an error inside a try conditional. */
    return (((fcp->func->flags & FC_ABORT) && did_emsg && !aborted_in_try())
	    || fcp->returned);
}

/*
 * return TRUE if cookie indicates a function which "abort"s on errors.
 */
    int
func_has_abort(cookie)
    void    *cookie;
{
    return ((struct funccall *)cookie)->func->flags & FC_ABORT;
}

#if defined(FEAT_VIMINFO) || defined(FEAT_SESSION)
typedef enum
{
    VAR_FLAVOUR_DEFAULT,
    VAR_FLAVOUR_SESSION,
    VAR_FLAVOUR_VIMINFO
} var_flavour_T;

static var_flavour_T var_flavour __ARGS((char_u *varname));

    static var_flavour_T
var_flavour(varname)
    char_u *varname;
{
    char_u *p = varname;

    if (ASCII_ISUPPER(*p))
    {
	while (*(++p))
	    if (ASCII_ISLOWER(*p))
		return VAR_FLAVOUR_SESSION;
	return VAR_FLAVOUR_VIMINFO;
    }
    else
	return VAR_FLAVOUR_DEFAULT;
}
#endif

#if defined(FEAT_VIMINFO) || defined(PROTO)
/*
 * Restore global vars that start with a capital from the viminfo file
 */
    int
read_viminfo_varlist(virp, writing)
    vir_T	*virp;
    int		writing;
{
    char_u	*tab;
    int		is_string = FALSE;
    VAR		varp = NULL;
    char_u	*val;

    if (!writing && (find_viminfo_parameter('!') != NULL))
    {
	tab = vim_strchr(virp->vir_line + 1, '\t');
	if (tab != NULL)
	{
	    *tab++ = '\0';	/* isolate the variable name */
	    if (*tab == 'S')	/* string var */
		is_string = TRUE;

	    tab = vim_strchr(tab, '\t');
	    if (tab != NULL)
	    {
		/* create a nameless variable to hold the value */
		if (is_string)
		{
		    val = viminfo_readstring(virp,
				       (int)(tab - virp->vir_line + 1), TRUE);
		    if (val != NULL)
			varp = alloc_string_var(val);
		}
		else
		{
		    varp = alloc_var();
		    if (varp != NULL)
		    {
			varp->var_type = VAR_NUMBER;
			varp->var_val.var_number = atol((char *)tab + 1);
		    }
		}
		/* assign the value to the variable */
		if (varp != NULL)
		{
		    set_var(virp->vir_line + 1, varp);
		    free_var(varp);
		}
	    }
	}
    }

    return viminfo_readline(virp);
}

/*
 * Write global vars that start with a capital to the viminfo file
 */
    void
write_viminfo_varlist(fp)
    FILE    *fp;
{
    garray_T	*gap = &variables;		/* global variable */
    VAR		this_var;
    int		i;

    if (find_viminfo_parameter('!') == NULL)
	return;

    fprintf(fp, _("\n# global variables:\n"));
    for (i = gap->ga_len; --i >= 0; )
    {
	this_var = &VAR_GAP_ENTRY(i, gap);
	if (this_var->var_name != NULL
		&& var_flavour(this_var->var_name) == VAR_FLAVOUR_VIMINFO)
	{
	    fprintf(fp, "!%s\t%s\t", this_var->var_name,
			  (this_var->var_type == VAR_STRING) ? "STR" : "NUM");
	    viminfo_writestring(fp, get_var_string(this_var));
	}
    }
}
#endif

#if defined(FEAT_SESSION) || defined(PROTO)
    int
store_session_globals(fd)
    FILE	*fd;
{
    garray_T	*gap = &variables;		/* global variable */
    VAR		this_var;
    int		i;
    char_u	*p, *t;

    for (i = gap->ga_len; --i >= 0; )
    {
	this_var = &VAR_GAP_ENTRY(i, gap);
	if (this_var->var_name != NULL)
	{
	    if (var_flavour(this_var->var_name) == VAR_FLAVOUR_SESSION)
	    {
		/* Escapse special characters with a backslash.  Turn a LF and
		 * CR into \n and \r. */
		p = vim_strsave_escaped(get_var_string(this_var),
							(char_u *)"\\\"\n\r");
		if (p == NULL)	    /* out of memory */
		    continue;
		for (t = p; *t != NUL; ++t)
		    if (*t == '\n')
			*t = 'n';
		    else if (*t == '\r')
			*t = 'r';
		if ((fprintf(fd, "let %s = %c%s%c",
			    this_var->var_name,
			    (this_var->var_type == VAR_STRING) ? '"' : ' ',
			    p,
			    (this_var->var_type == VAR_STRING) ? '"' : ' ') < 0)
			|| put_eol(fd) == FAIL)
		{
		    vim_free(p);
		    return FAIL;
		}
		vim_free(p);
	    }

	}
    }
    return OK;
}
#endif

#endif /* FEAT_EVAL */

#if defined(FEAT_MODIFY_FNAME) || defined(FEAT_EVAL) || defined(PROTO)


#ifdef WIN3264
/*
 * Functions for ":8" filename modifier: get 8.3 version of a filename.
 */
static int get_short_pathname __ARGS((char_u **fnamep, char_u **bufp, int *fnamelen));
static int shortpath_for_invalid_fname __ARGS((char_u **fname, char_u **bufp, int *fnamelen));
static int shortpath_for_partial __ARGS((char_u **fnamep, char_u **bufp, int *fnamelen));

/*
 * Get the short pathname of a file.
 * Returns 1 on success. *fnamelen is 0 for nonexistant path.
 */
    static int
get_short_pathname(fnamep, bufp, fnamelen)
    char_u	**fnamep;
    char_u	**bufp;
    int		*fnamelen;
{
    int		l,len;
    char_u	*newbuf;

    len = *fnamelen;

    l = GetShortPathName(*fnamep, *fnamep, len);
    if (l > len - 1)
    {
	/* If that doesn't work (not enough space), then save the string
	 * and try again with a new buffer big enough
	 */
	newbuf = vim_strnsave(*fnamep, l);
	if (newbuf == NULL)
	    return 0;

	vim_free(*bufp);
	*fnamep = *bufp = newbuf;

	l = GetShortPathName(*fnamep,*fnamep,l+1);

	/* Really should always succeed, as the buffer is big enough */
    }

    *fnamelen = l;
    return 1;
}

/*
 * Create a short path name.  Returns the length of the buffer it needs.
 * Doesn't copy over the end of the buffer passed in.
 */
    static int
shortpath_for_invalid_fname(fname, bufp, fnamelen)
    char_u	**fname;
    char_u	**bufp;
    int		*fnamelen;
{
    char_u	*s, *p, *pbuf2, *pbuf3;
    char_u	ch;
    int		l,len,len2,plen,slen;

    /* Make a copy */
    len2 = *fnamelen;
    pbuf2 = vim_strnsave(*fname, len2);
    pbuf3 = NULL;

    s = pbuf2 + len2 - 1; /* Find the end */
    slen = 1;
    plen = len2;

    l = 0;
    if (vim_ispathsep(*s))
    {
	--s;
	++slen;
	--plen;
    }

    do
    {
	/* Go back one path-seperator */
	while (s > pbuf2 && !vim_ispathsep(*s))
	{
	    --s;
	    ++slen;
	    --plen;
	}
	if (s <= pbuf2)
	    break;

	/* Remeber the character that is about to be blatted */
	ch = *s;
	*s = 0; /* get_short_pathname requires a null-terminated string */

	/* Try it in situ */
	p = pbuf2;
	if (!get_short_pathname(&p, &pbuf3, &plen))
	{
	    vim_free(pbuf2);
	    return -1;
	}
	*s = ch;    /* Preserve the string */
    } while (plen == 0);

    if (plen > 0)
    {
	/* Remeber the length of the new string.  */
	*fnamelen = len = plen + slen;
	vim_free(*bufp);
	if (len > len2)
	{
	    /* If there's not enough space in the currently allocated string,
	     * then copy it to a buffer big enough.
	     */
	    *fname= *bufp = vim_strnsave(p, len);
	    if (*fname == NULL)
		return -1;
	}
	else
	{
	    /* Transfer pbuf2 to being the main buffer  (it's big enough) */
	    *fname = *bufp = pbuf2;
	    if (p != pbuf2)
		strncpy(*fname, p, plen);
	    pbuf2 = NULL;
	}
	/* Concat the next bit */
	strncpy(*fname + plen, s, slen);
	(*fname)[len] = '\0';
    }
    vim_free(pbuf3);
    vim_free(pbuf2);
    return 0;
}

/*
 * Get a pathname for a partial path.
 */
    static int
shortpath_for_partial(fnamep, bufp, fnamelen)
    char_u	**fnamep;
    char_u	**bufp;
    int		*fnamelen;
{
    int		sepcount, len, tflen;
    char_u	*p;
    char_u	*pbuf, *tfname;
    int		hasTilde;

    /* Count up the path seperators from the RHS.. so we know which part
     * of the path to return.
     */
    sepcount = 0;
    for (p = *fnamep + *fnamelen - 1; p >= *fnamep; --p)
	if (vim_ispathsep(*p))
	    ++sepcount;

    /* Need full path first (use expand_env() to remove a "~/") */
    hasTilde = (**fnamep == '~');
    if (hasTilde)
	pbuf = tfname = expand_env_save(*fnamep);
    else
	pbuf = tfname = FullName_save(*fnamep, FALSE);

    len = tflen = STRLEN(tfname);

    if (!get_short_pathname(&tfname, &pbuf, &len))
	return -1;

    if (len == 0)
    {
	/* Don't have a valid filename, so shorten the rest of the
	 * path if we can. This CAN give us invalid 8.3 filenames, but
	 * there's not a lot of point in guessing what it might be.
	 */
	len = tflen;
	if (shortpath_for_invalid_fname(&tfname, &pbuf, &len) == -1)
	    return -1;
    }

    /* Count the paths backward to find the beginning of the desired string. */
    for (p = tfname + len - 1; p >= tfname; --p)
	if (vim_ispathsep(*p))
	{
	    if (sepcount == 0 || (hasTilde && sepcount == 1))
		break;
	    else
		sepcount --;
	}
    if (hasTilde)
    {
	--p;
	if (p >= tfname)
	    *p = '~';
	else
	    return -1;
    }
    else
	++p;

    /* Copy in the string - p indexes into tfname - allocated at pbuf */
    vim_free(*bufp);
    *fnamelen = (int)STRLEN(p);
    *bufp = pbuf;
    *fnamep = p;

    return 0;
}
#endif /* WIN3264 */

/*
 * Adjust a filename, according to a string of modifiers.
 * *fnamep must be NUL terminated when called.  When returning, the length is
 * determined by *fnamelen.
 * Returns valid flags.
 * When there is an error, *fnamep is set to NULL.
 */
    int
modify_fname(src, usedlen, fnamep, bufp, fnamelen)
    char_u	*src;		/* string with modifiers */
    int		*usedlen;	/* characters after src that are used */
    char_u	**fnamep;	/* file name so far */
    char_u	**bufp;		/* buffer for allocated file name or NULL */
    int		*fnamelen;	/* length of fnamep */
{
    int		valid = 0;
    char_u	*tail;
    char_u	*s, *p, *pbuf;
    char_u	dirname[MAXPATHL];
    int		c;
    int		has_fullname = 0;
#ifdef WIN3264
    int		has_shortname = 0;
#endif

repeat:
    /* ":p" - full path/file_name */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == 'p')
    {
	has_fullname = 1;

	valid |= VALID_PATH;
	*usedlen += 2;

	/* Expand "~/path" for all systems and "~user/path" for Unix and VMS */
	if ((*fnamep)[0] == '~'
#if !defined(UNIX) && !(defined(VMS) && defined(USER_HOME))
		&& ((*fnamep)[1] == '/'
# ifdef BACKSLASH_IN_FILENAME
		    || (*fnamep)[1] == '\\'
# endif
		    || (*fnamep)[1] == NUL)

#endif
	   )
	{
	    *fnamep = expand_env_save(*fnamep);
	    vim_free(*bufp);	/* free any allocated file name */
	    *bufp = *fnamep;
	    if (*fnamep == NULL)
		return -1;
	}

	/* When "/." or "/.." is used: force expansion to get rid of it. */
	for (p = *fnamep; *p != NUL; ++p)
	{
	    if (vim_ispathsep(*p)
		    && p[1] == '.'
		    && (p[2] == NUL
			|| vim_ispathsep(p[2])
			|| (p[2] == '.'
			    && (p[3] == NUL || vim_ispathsep(p[3])))))
		break;
	}

	/* FullName_save() is slow, don't use it when not needed. */
	if (*p != NUL || !vim_isAbsName(*fnamep))
	{
	    *fnamep = FullName_save(*fnamep, *p != NUL);
	    vim_free(*bufp);	/* free any allocated file name */
	    *bufp = *fnamep;
	    if (*fnamep == NULL)
		return -1;
	}

	/* Append a path separator to a directory. */
	if (mch_isdir(*fnamep))
	{
	    /* Make room for one or two extra characters. */
	    *fnamep = vim_strnsave(*fnamep, (int)STRLEN(*fnamep) + 2);
	    vim_free(*bufp);	/* free any allocated file name */
	    *bufp = *fnamep;
	    if (*fnamep == NULL)
		return -1;
	    add_pathsep(*fnamep);
	}
    }

    /* ":." - path relative to the current directory */
    /* ":~" - path relative to the home directory */
    /* ":8" - shortname path - postponed till after */
    while (src[*usedlen] == ':'
		  && ((c = src[*usedlen + 1]) == '.' || c == '~' || c == '8'))
    {
	*usedlen += 2;
	if (c == '8')
	{
#ifdef WIN3264
	    has_shortname = 1; /* Postpone this. */
#endif
	    continue;
	}
	pbuf = NULL;
	/* Need full path first (use expand_env() to remove a "~/") */
	if (!has_fullname)
	{
	    if (c == '.' && **fnamep == '~')
		p = pbuf = expand_env_save(*fnamep);
	    else
		p = pbuf = FullName_save(*fnamep, FALSE);
	}
	else
	    p = *fnamep;

	has_fullname = 0;

	if (p != NULL)
	{
	    if (c == '.')
	    {
		mch_dirname(dirname, MAXPATHL);
		s = shorten_fname(p, dirname);
		if (s != NULL)
		{
		    *fnamep = s;
		    if (pbuf != NULL)
		    {
			vim_free(*bufp);   /* free any allocated file name */
			*bufp = pbuf;
			pbuf = NULL;
		    }
		}
	    }
	    else
	    {
		home_replace(NULL, p, dirname, MAXPATHL, TRUE);
		/* Only replace it when it starts with '~' */
		if (*dirname == '~')
		{
		    s = vim_strsave(dirname);
		    if (s != NULL)
		    {
			*fnamep = s;
			vim_free(*bufp);
			*bufp = s;
		    }
		}
	    }
	    vim_free(pbuf);
	}
    }

    tail = gettail(*fnamep);
    *fnamelen = (int)STRLEN(*fnamep);

    /* ":h" - head, remove "/file_name", can be repeated  */
    /* Don't remove the first "/" or "c:\" */
    while (src[*usedlen] == ':' && src[*usedlen + 1] == 'h')
    {
	valid |= VALID_HEAD;
	*usedlen += 2;
	s = get_past_head(*fnamep);
	while (tail > s && vim_ispathsep(tail[-1]))
	    --tail;
	*fnamelen = (int)(tail - *fnamep);
#ifdef VMS
	if (*fnamelen > 0)
	    *fnamelen += 1; /* the path separator is part of the path */
#endif
	while (tail > s && !vim_ispathsep(tail[-1]))
	    --tail;
    }

    /* ":8" - shortname  */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == '8')
    {
	*usedlen += 2;
#ifdef WIN3264
	has_shortname = 1;
#endif
    }

#ifdef WIN3264
    /* Check shortname after we have done 'heads' and before we do 'tails'
     */
    if (has_shortname)
    {
	pbuf = NULL;
	/* Copy the string if it is shortened by :h */
	if (*fnamelen < (int)STRLEN(*fnamep))
	{
	    p = vim_strnsave(*fnamep, *fnamelen);
	    if (p == 0)
		return -1;
	    vim_free(*bufp);
	    *bufp = *fnamep = p;
	}

	/* Split into two implementations - makes it easier.  First is where
	 * there isn't a full name already, second is where there is.
	 */
	if (!has_fullname && !vim_isAbsName(*fnamep))
	{
	    if (shortpath_for_partial(fnamep, bufp, fnamelen) == -1)
		return -1;
	}
	else
	{
	    int		l;

	    /* Simple case, already have the full-name
	     * Nearly always shorter, so try first time. */
	    l = *fnamelen;
	    if (!get_short_pathname(fnamep, bufp, &l))
		return -1;

	    if (l == 0)
	    {
		/* Couldn't find the filename.. search the paths.
		 */
		l = *fnamelen;
		if (shortpath_for_invalid_fname(fnamep, bufp, &l ) == -1)
		    return -1;
	    }
	    *fnamelen = l;
	}
    }
#endif /* WIN3264 */

    /* ":t" - tail, just the basename */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == 't')
    {
	*usedlen += 2;
	*fnamelen -= (int)(tail - *fnamep);
	*fnamep = tail;
    }

    /* ":e" - extension, can be repeated */
    /* ":r" - root, without extension, can be repeated */
    while (src[*usedlen] == ':'
	    && (src[*usedlen + 1] == 'e' || src[*usedlen + 1] == 'r'))
    {
	/* find a '.' in the tail:
	 * - for second :e: before the current fname
	 * - otherwise: The last '.'
	 */
	if (src[*usedlen + 1] == 'e' && *fnamep > tail)
	    s = *fnamep - 2;
	else
	    s = *fnamep + *fnamelen - 1;
	for ( ; s > tail; --s)
	    if (s[0] == '.')
		break;
	if (src[*usedlen + 1] == 'e')		/* :e */
	{
	    if (s > tail)
	    {
		*fnamelen += (int)(*fnamep - (s + 1));
		*fnamep = s + 1;
#ifdef VMS
		/* cut version from the extension */
		s = *fnamep + *fnamelen - 1;
		for ( ; s > *fnamep; --s)
		    if (s[0] == ';')
			break;
		if (s > *fnamep)
		    *fnamelen = s - *fnamep;
#endif
	    }
	    else if (*fnamep <= tail)
		*fnamelen = 0;
	}
	else				/* :r */
	{
	    if (s > tail)	/* remove one extension */
		*fnamelen = (int)(s - *fnamep);
	}
	*usedlen += 2;
    }

    /* ":s?pat?foo?" - substitute */
    /* ":gs?pat?foo?" - global substitute */
    if (src[*usedlen] == ':'
	    && (src[*usedlen + 1] == 's'
		|| (src[*usedlen + 1] == 'g' && src[*usedlen + 2] == 's')))
    {
	char_u	    *str;
	char_u	    *pat;
	char_u	    *sub;
	int	    sep;
	char_u	    *flags;
	int	    didit = FALSE;

	flags = (char_u *)"";
	s = src + *usedlen + 2;
	if (src[*usedlen + 1] == 'g')
	{
	    flags = (char_u *)"g";
	    ++s;
	}

	sep = *s++;
	if (sep)
	{
	    /* find end of pattern */
	    p = vim_strchr(s, sep);
	    if (p != NULL)
	    {
		pat = vim_strnsave(s, (int)(p - s));
		if (pat != NULL)
		{
		    s = p + 1;
		    /* find end of substitution */
		    p = vim_strchr(s, sep);
		    if (p != NULL)
		    {
			sub = vim_strnsave(s, (int)(p - s));
			str = vim_strnsave(*fnamep, *fnamelen);
			if (sub != NULL && str != NULL)
			{
			    *usedlen = (int)(p + 1 - src);
			    s = do_string_sub(str, pat, sub, flags);
			    if (s != NULL)
			    {
				*fnamep = s;
				*fnamelen = (int)STRLEN(s);
				vim_free(*bufp);
				*bufp = s;
				didit = TRUE;
			    }
			}
			vim_free(sub);
			vim_free(str);
		    }
		    vim_free(pat);
		}
	    }
	    /* after using ":s", repeat all the modifiers */
	    if (didit)
		goto repeat;
	}
    }

    return valid;
}

/*
 * Perform a substitution on "str" with pattern "pat" and substitute "sub".
 * "flags" can be "g" to do a global substitute.
 * Returns an allocated string, NULL for error.
 */
    char_u *
do_string_sub(str, pat, sub, flags)
    char_u	*str;
    char_u	*pat;
    char_u	*sub;
    char_u	*flags;
{
    int		sublen;
    regmatch_T	regmatch;
    int		i;
    int		do_all;
    char_u	*tail;
    garray_T	ga;
    char_u	*ret;
    char_u	*save_cpo;

    /* Make 'cpoptions' empty, so that the 'l' flag doesn't work here */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    ga_init2(&ga, 1, 200);

    do_all = (flags[0] == 'g');

    regmatch.rm_ic = p_ic;
    regmatch.regprog = vim_regcomp(pat, RE_MAGIC + RE_STRING);
    if (regmatch.regprog != NULL)
    {
	tail = str;
	while (vim_regexec_nl(&regmatch, str, (colnr_T)(tail - str)))
	{
	    /*
	     * Get some space for a temporary buffer to do the substitution
	     * into.  It will contain:
	     * - The text up to where the match is.
	     * - The substituted text.
	     * - The text after the match.
	     */
	    sublen = vim_regsub(&regmatch, sub, tail, FALSE, TRUE, FALSE);
	    if (ga_grow(&ga, (int)(STRLEN(tail) + sublen -
			    (regmatch.endp[0] - regmatch.startp[0]))) == FAIL)
	    {
		ga_clear(&ga);
		break;
	    }

	    /* copy the text up to where the match is */
	    i = (int)(regmatch.startp[0] - tail);
	    mch_memmove((char_u *)ga.ga_data + ga.ga_len, tail, (size_t)i);
	    /* add the substituted text */
	    (void)vim_regsub(&regmatch, sub, (char_u *)ga.ga_data
					  + ga.ga_len + i, TRUE, TRUE, FALSE);
	    ga.ga_len += i + sublen - 1;
	    ga.ga_room -= i + sublen - 1;
	    /* avoid getting stuck on a match with an empty string */
	    if (tail == regmatch.endp[0])
	    {
		if (*tail == NUL)
		    break;
		*((char_u *)ga.ga_data + ga.ga_len) = *tail++;
		++ga.ga_len;
		--ga.ga_room;
	    }
	    else
	    {
		tail = regmatch.endp[0];
		if (*tail == NUL)
		    break;
	    }
	    if (!do_all)
		break;
	}

	if (ga.ga_data != NULL)
	    STRCPY((char *)ga.ga_data + ga.ga_len, tail);

	vim_free(regmatch.regprog);
    }

    ret = vim_strsave(ga.ga_data == NULL ? str : (char_u *)ga.ga_data);
    ga_clear(&ga);
    p_cpo = save_cpo;

    return ret;
}

#endif /* defined(FEAT_MODIFY_FNAME) || defined(FEAT_EVAL) */
