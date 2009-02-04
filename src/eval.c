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
#if defined(MSDOS) || defined(WIN16) || defined(WIN32) || defined(_WIN64)
# include "vimio.h"	/* for mch_open(), must be before vim.h */
#endif

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

#ifdef AMIGA
# include <time.h>	/* for strftime() */
#endif

#ifdef MACOS
# include <time.h>	/* for time_t */
#endif

#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
# include <math.h>
#endif

#define DICT_MAXNEST 100	/* maximum nesting of lists and dicts */

#define DO_NOT_FREE_CNT 99999	/* refcount for dict or list that should not
				   be freed. */

/*
 * In a hashtab item "hi_key" points to "di_key" in a dictitem.
 * This avoids adding a pointer to the hashtab item.
 * DI2HIKEY() converts a dictitem pointer to a hashitem key pointer.
 * HIKEY2DI() converts a hashitem key pointer to a dictitem pointer.
 * HI2DI() converts a hashitem pointer to a dictitem pointer.
 */
static dictitem_T dumdi;
#define DI2HIKEY(di) ((di)->di_key)
#define HIKEY2DI(p)  ((dictitem_T *)(p - (dumdi.di_key - (char_u *)&dumdi)))
#define HI2DI(hi)     HIKEY2DI((hi)->hi_key)

/*
 * Structure returned by get_lval() and used by set_var_lval().
 * For a plain name:
 *	"name"	    points to the variable name.
 *	"exp_name"  is NULL.
 *	"tv"	    is NULL
 * For a magic braces name:
 *	"name"	    points to the expanded variable name.
 *	"exp_name"  is non-NULL, to be freed later.
 *	"tv"	    is NULL
 * For an index in a list:
 *	"name"	    points to the (expanded) variable name.
 *	"exp_name"  NULL or non-NULL, to be freed later.
 *	"tv"	    points to the (first) list item value
 *	"li"	    points to the (first) list item
 *	"range", "n1", "n2" and "empty2" indicate what items are used.
 * For an existing Dict item:
 *	"name"	    points to the (expanded) variable name.
 *	"exp_name"  NULL or non-NULL, to be freed later.
 *	"tv"	    points to the dict item value
 *	"newkey"    is NULL
 * For a non-existing Dict item:
 *	"name"	    points to the (expanded) variable name.
 *	"exp_name"  NULL or non-NULL, to be freed later.
 *	"tv"	    points to the Dictionary typval_T
 *	"newkey"    is the key for the new item.
 */
typedef struct lval_S
{
    char_u	*ll_name;	/* start of variable name (can be NULL) */
    char_u	*ll_exp_name;	/* NULL or expanded name in allocated memory. */
    typval_T	*ll_tv;		/* Typeval of item being used.  If "newkey"
				   isn't NULL it's the Dict to which to add
				   the item. */
    listitem_T	*ll_li;		/* The list item or NULL. */
    list_T	*ll_list;	/* The list or NULL. */
    int		ll_range;	/* TRUE when a [i:j] range was used */
    long	ll_n1;		/* First index for list */
    long	ll_n2;		/* Second index for list range */
    int		ll_empty2;	/* Second index is empty: [i:] */
    dict_T	*ll_dict;	/* The Dictionary or NULL */
    dictitem_T	*ll_di;		/* The dictitem or NULL */
    char_u	*ll_newkey;	/* New key for Dict in alloc. mem or NULL. */
} lval_T;


static char *e_letunexp	= N_("E18: Unexpected characters in :let");
static char *e_listidx = N_("E684: list index out of range: %ld");
static char *e_undefvar = N_("E121: Undefined variable: %s");
static char *e_missbrac = N_("E111: Missing ']'");
static char *e_listarg = N_("E686: Argument of %s must be a List");
static char *e_listdictarg = N_("E712: Argument of %s must be a List or Dictionary");
static char *e_emptykey = N_("E713: Cannot use empty key for Dictionary");
static char *e_listreq = N_("E714: List required");
static char *e_dictreq = N_("E715: Dictionary required");
static char *e_toomanyarg = N_("E118: Too many arguments for function: %s");
static char *e_dictkey = N_("E716: Key not present in Dictionary: %s");
static char *e_funcexts = N_("E122: Function %s already exists, add ! to replace it");
static char *e_funcdict = N_("E717: Dictionary entry already exists");
static char *e_funcref = N_("E718: Funcref required");
static char *e_dictrange = N_("E719: Cannot use [:] with a Dictionary");
static char *e_letwrong = N_("E734: Wrong variable type for %s=");
static char *e_nofunc = N_("E130: Unknown function: %s");
static char *e_illvar = N_("E461: Illegal variable name: %s");

/*
 * All user-defined global variables are stored in dictionary "globvardict".
 * "globvars_var" is the variable that is used for "g:".
 */
static dict_T		globvardict;
static dictitem_T	globvars_var;
#define globvarht globvardict.dv_hashtab

/*
 * Old Vim variables such as "v:version" are also available without the "v:".
 * Also in functions.  We need a special hashtable for them.
 */
static hashtab_T	compat_hashtab;

/*
 * When recursively copying lists and dicts we need to remember which ones we
 * have done to avoid endless recursiveness.  This unique ID is used for that.
 */
static int current_copyID = 0;

/*
 * Array to hold the hashtab with variables local to each sourced script.
 * Each item holds a variable (nameless) that points to the dict_T.
 */
typedef struct
{
    dictitem_T	sv_var;
    dict_T	sv_dict;
} scriptvar_T;

static garray_T	    ga_scripts = {0, 0, sizeof(scriptvar_T), 4, NULL};
#define SCRIPT_SV(id) (((scriptvar_T *)ga_scripts.ga_data)[(id) - 1])
#define SCRIPT_VARS(id) (SCRIPT_SV(id).sv_dict.dv_hashtab)

static int echo_attr = 0;   /* attributes used for ":echo" */

/* Values for trans_function_name() argument: */
#define TFN_INT		1	/* internal function name OK */
#define TFN_QUIET	2	/* no error messages */

/*
 * Structure to hold info for a user function.
 */
typedef struct ufunc ufunc_T;

struct ufunc
{
    int		uf_varargs;	/* variable nr of arguments */
    int		uf_flags;
    int		uf_calls;	/* nr of active calls */
    garray_T	uf_args;	/* arguments */
    garray_T	uf_lines;	/* function lines */
#ifdef FEAT_PROFILE
    int		uf_profiling;	/* TRUE when func is being profiled */
    /* profiling the function as a whole */
    int		uf_tm_count;	/* nr of calls */
    proftime_T	uf_tm_total;	/* time spent in function + children */
    proftime_T	uf_tm_self;	/* time spent in function itself */
    proftime_T	uf_tm_children;	/* time spent in children this call */
    /* profiling the function per line */
    int		*uf_tml_count;	/* nr of times line was executed */
    proftime_T	*uf_tml_total;	/* time spent in a line + children */
    proftime_T	*uf_tml_self;	/* time spent in a line itself */
    proftime_T	uf_tml_start;	/* start time for current line */
    proftime_T	uf_tml_children; /* time spent in children for this line */
    proftime_T	uf_tml_wait;	/* start wait time for current line */
    int		uf_tml_idx;	/* index of line being timed; -1 if none */
    int		uf_tml_execed;	/* line being timed was executed */
#endif
    scid_T	uf_script_ID;	/* ID of script where function was defined,
				   used for s: variables */
    int		uf_refcount;	/* for numbered function: reference count */
    char_u	uf_name[1];	/* name of function (actually longer); can
				   start with <SNR>123_ (<SNR> is K_SPECIAL
				   KS_EXTRA KE_SNR) */
};

/* function flags */
#define FC_ABORT    1		/* abort function on error */
#define FC_RANGE    2		/* function accepts range */
#define FC_DICT	    4		/* Dict function, uses "self" */

/*
 * All user-defined functions are found in this hashtable.
 */
static hashtab_T	func_hashtab;

/* The names of packages that once were loaded are remembered. */
static garray_T		ga_loaded = {0, 0, sizeof(char_u *), 4, NULL};

/* list heads for garbage collection */
static dict_T		*first_dict = NULL;	/* list of all dicts */
static list_T		*first_list = NULL;	/* list of all lists */

/* From user function to hashitem and back. */
static ufunc_T dumuf;
#define UF2HIKEY(fp) ((fp)->uf_name)
#define HIKEY2UF(p)  ((ufunc_T *)(p - (dumuf.uf_name - (char_u *)&dumuf)))
#define HI2UF(hi)     HIKEY2UF((hi)->hi_key)

#define FUNCARG(fp, j)	((char_u **)(fp->uf_args.ga_data))[j]
#define FUNCLINE(fp, j)	((char_u **)(fp->uf_lines.ga_data))[j]

#define MAX_FUNC_ARGS	20	/* maximum number of function arguments */
#define VAR_SHORT_LEN	20	/* short variable name length */
#define FIXVAR_CNT	12	/* number of fixed variables */

/* structure to hold info for a function that is currently being executed. */
typedef struct funccall_S funccall_T;

struct funccall_S
{
    ufunc_T	*func;		/* function being called */
    int		linenr;		/* next line to be executed */
    int		returned;	/* ":return" used */
    struct			/* fixed variables for arguments */
    {
	dictitem_T	var;		/* variable (without room for name) */
	char_u	room[VAR_SHORT_LEN];	/* room for the name */
    } fixvar[FIXVAR_CNT];
    dict_T	l_vars;		/* l: local function variables */
    dictitem_T	l_vars_var;	/* variable for l: scope */
    dict_T	l_avars;	/* a: argument variables */
    dictitem_T	l_avars_var;	/* variable for a: scope */
    list_T	l_varlist;	/* list for a:000 */
    listitem_T	l_listitems[MAX_FUNC_ARGS];	/* listitems for a:000 */
    typval_T	*rettv;		/* return value */
    linenr_T	breakpoint;	/* next line with breakpoint or zero */
    int		dbg_tick;	/* debug_tick when breakpoint was set */
    int		level;		/* top nesting level of executed function */
#ifdef FEAT_PROFILE
    proftime_T	prof_child;	/* time spent in a child */
#endif
    funccall_T	*caller;	/* calling function or NULL */
};

/*
 * Info used by a ":for" loop.
 */
typedef struct
{
    int		fi_semicolon;	/* TRUE if ending in '; var]' */
    int		fi_varcount;	/* nr of variables in the list */
    listwatch_T	fi_lw;		/* keep an eye on the item used. */
    list_T	*fi_list;	/* list being used */
} forinfo_T;

/*
 * Struct used by trans_function_name()
 */
typedef struct
{
    dict_T	*fd_dict;	/* Dictionary used */
    char_u	*fd_newkey;	/* new key in "dict" in allocated memory */
    dictitem_T	*fd_di;		/* Dictionary item used */
} funcdict_T;


/*
 * Array to hold the value of v: variables.
 * The value is in a dictitem, so that it can also be used in the v: scope.
 * The reason to use this table anyway is for very quick access to the
 * variables with the VV_ defines.
 */
#include "version.h"

/* values for vv_flags: */
#define VV_COMPAT	1	/* compatible, also used without "v:" */
#define VV_RO		2	/* read-only */
#define VV_RO_SBX	4	/* read-only in the sandbox */

#define VV_NAME(s, t)	s, {{t}}, {0}

static struct vimvar
{
    char	*vv_name;	/* name of variable, without v: */
    dictitem_T	vv_di;		/* value and name for key */
    char	vv_filler[16];	/* space for LONGEST name below!!! */
    char	vv_flags;	/* VV_COMPAT, VV_RO, VV_RO_SBX */
} vimvars[VV_LEN] =
{
    /*
     * The order here must match the VV_ defines in vim.h!
     * Initializing a union does not work, leave tv.vval empty to get zero's.
     */
    {VV_NAME("count",		 VAR_NUMBER), VV_COMPAT+VV_RO},
    {VV_NAME("count1",		 VAR_NUMBER), VV_RO},
    {VV_NAME("prevcount",	 VAR_NUMBER), VV_RO},
    {VV_NAME("errmsg",		 VAR_STRING), VV_COMPAT},
    {VV_NAME("warningmsg",	 VAR_STRING), 0},
    {VV_NAME("statusmsg",	 VAR_STRING), 0},
    {VV_NAME("shell_error",	 VAR_NUMBER), VV_COMPAT+VV_RO},
    {VV_NAME("this_session",	 VAR_STRING), VV_COMPAT},
    {VV_NAME("version",		 VAR_NUMBER), VV_COMPAT+VV_RO},
    {VV_NAME("lnum",		 VAR_NUMBER), VV_RO_SBX},
    {VV_NAME("termresponse",	 VAR_STRING), VV_RO},
    {VV_NAME("fname",		 VAR_STRING), VV_RO},
    {VV_NAME("lang",		 VAR_STRING), VV_RO},
    {VV_NAME("lc_time",		 VAR_STRING), VV_RO},
    {VV_NAME("ctype",		 VAR_STRING), VV_RO},
    {VV_NAME("charconvert_from", VAR_STRING), VV_RO},
    {VV_NAME("charconvert_to",	 VAR_STRING), VV_RO},
    {VV_NAME("fname_in",	 VAR_STRING), VV_RO},
    {VV_NAME("fname_out",	 VAR_STRING), VV_RO},
    {VV_NAME("fname_new",	 VAR_STRING), VV_RO},
    {VV_NAME("fname_diff",	 VAR_STRING), VV_RO},
    {VV_NAME("cmdarg",		 VAR_STRING), VV_RO},
    {VV_NAME("foldstart",	 VAR_NUMBER), VV_RO_SBX},
    {VV_NAME("foldend",		 VAR_NUMBER), VV_RO_SBX},
    {VV_NAME("folddashes",	 VAR_STRING), VV_RO_SBX},
    {VV_NAME("foldlevel",	 VAR_NUMBER), VV_RO_SBX},
    {VV_NAME("progname",	 VAR_STRING), VV_RO},
    {VV_NAME("servername",	 VAR_STRING), VV_RO},
    {VV_NAME("dying",		 VAR_NUMBER), VV_RO},
    {VV_NAME("exception",	 VAR_STRING), VV_RO},
    {VV_NAME("throwpoint",	 VAR_STRING), VV_RO},
    {VV_NAME("register",	 VAR_STRING), VV_RO},
    {VV_NAME("cmdbang",		 VAR_NUMBER), VV_RO},
    {VV_NAME("insertmode",	 VAR_STRING), VV_RO},
    {VV_NAME("val",		 VAR_UNKNOWN), VV_RO},
    {VV_NAME("key",		 VAR_UNKNOWN), VV_RO},
    {VV_NAME("profiling",	 VAR_NUMBER), VV_RO},
    {VV_NAME("fcs_reason",	 VAR_STRING), VV_RO},
    {VV_NAME("fcs_choice",	 VAR_STRING), 0},
    {VV_NAME("beval_bufnr",	 VAR_NUMBER), VV_RO},
    {VV_NAME("beval_winnr",	 VAR_NUMBER), VV_RO},
    {VV_NAME("beval_lnum",	 VAR_NUMBER), VV_RO},
    {VV_NAME("beval_col",	 VAR_NUMBER), VV_RO},
    {VV_NAME("beval_text",	 VAR_STRING), VV_RO},
    {VV_NAME("scrollstart",	 VAR_STRING), 0},
    {VV_NAME("swapname",	 VAR_STRING), VV_RO},
    {VV_NAME("swapchoice",	 VAR_STRING), 0},
    {VV_NAME("swapcommand",	 VAR_STRING), VV_RO},
    {VV_NAME("char",		 VAR_STRING), VV_RO},
    {VV_NAME("mouse_win",	 VAR_NUMBER), 0},
    {VV_NAME("mouse_lnum",	 VAR_NUMBER), 0},
    {VV_NAME("mouse_col",	 VAR_NUMBER), 0},
    {VV_NAME("operator",	 VAR_STRING), VV_RO},
    {VV_NAME("searchforward",	 VAR_NUMBER), 0},
    {VV_NAME("oldfiles",	 VAR_LIST), 0},
};

/* shorthand */
#define vv_type		vv_di.di_tv.v_type
#define vv_nr		vv_di.di_tv.vval.v_number
#define vv_float	vv_di.di_tv.vval.v_float
#define vv_str		vv_di.di_tv.vval.v_string
#define vv_list		vv_di.di_tv.vval.v_list
#define vv_tv		vv_di.di_tv

/*
 * The v: variables are stored in dictionary "vimvardict".
 * "vimvars_var" is the variable that is used for the "l:" scope.
 */
static dict_T		vimvardict;
static dictitem_T	vimvars_var;
#define vimvarht  vimvardict.dv_hashtab

static void prepare_vimvar __ARGS((int idx, typval_T *save_tv));
static void restore_vimvar __ARGS((int idx, typval_T *save_tv));
#if defined(FEAT_USR_CMDS) && defined(FEAT_CMDL_COMPL)
static int call_vim_function __ARGS((char_u *func, int argc, char_u **argv, int safe, typval_T *rettv));
#endif
static int ex_let_vars __ARGS((char_u *arg, typval_T *tv, int copy, int semicolon, int var_count, char_u *nextchars));
static char_u *skip_var_list __ARGS((char_u *arg, int *var_count, int *semicolon));
static char_u *skip_var_one __ARGS((char_u *arg));
static void list_hashtable_vars __ARGS((hashtab_T *ht, char_u *prefix, int empty, int *first));
static void list_glob_vars __ARGS((int *first));
static void list_buf_vars __ARGS((int *first));
static void list_win_vars __ARGS((int *first));
#ifdef FEAT_WINDOWS
static void list_tab_vars __ARGS((int *first));
#endif
static void list_vim_vars __ARGS((int *first));
static void list_script_vars __ARGS((int *first));
static void list_func_vars __ARGS((int *first));
static char_u *list_arg_vars __ARGS((exarg_T *eap, char_u *arg, int *first));
static char_u *ex_let_one __ARGS((char_u *arg, typval_T *tv, int copy, char_u *endchars, char_u *op));
static int check_changedtick __ARGS((char_u *arg));
static char_u *get_lval __ARGS((char_u *name, typval_T *rettv, lval_T *lp, int unlet, int skip, int quiet, int fne_flags));
static void clear_lval __ARGS((lval_T *lp));
static void set_var_lval __ARGS((lval_T *lp, char_u *endp, typval_T *rettv, int copy, char_u *op));
static int tv_op __ARGS((typval_T *tv1, typval_T *tv2, char_u  *op));
static void list_add_watch __ARGS((list_T *l, listwatch_T *lw));
static void list_rem_watch __ARGS((list_T *l, listwatch_T *lwrem));
static void list_fix_watch __ARGS((list_T *l, listitem_T *item));
static void ex_unletlock __ARGS((exarg_T *eap, char_u *argstart, int deep));
static int do_unlet_var __ARGS((lval_T *lp, char_u *name_end, int forceit));
static int do_lock_var __ARGS((lval_T *lp, char_u *name_end, int deep, int lock));
static void item_lock __ARGS((typval_T *tv, int deep, int lock));
static int tv_islocked __ARGS((typval_T *tv));

static int eval0 __ARGS((char_u *arg,  typval_T *rettv, char_u **nextcmd, int evaluate));
static int eval1 __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int eval2 __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int eval3 __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int eval4 __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int eval5 __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int eval6 __ARGS((char_u **arg, typval_T *rettv, int evaluate, int want_string));
static int eval7 __ARGS((char_u **arg, typval_T *rettv, int evaluate, int want_string));

static int eval_index __ARGS((char_u **arg, typval_T *rettv, int evaluate, int verbose));
static int get_option_tv __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int get_string_tv __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int get_lit_string_tv __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int get_list_tv __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int rettv_list_alloc __ARGS((typval_T *rettv));
static listitem_T *listitem_alloc __ARGS((void));
static void listitem_free __ARGS((listitem_T *item));
static void listitem_remove __ARGS((list_T *l, listitem_T *item));
static long list_len __ARGS((list_T *l));
static int list_equal __ARGS((list_T *l1, list_T *l2, int ic));
static int dict_equal __ARGS((dict_T *d1, dict_T *d2, int ic));
static int tv_equal __ARGS((typval_T *tv1, typval_T *tv2, int ic));
static listitem_T *list_find __ARGS((list_T *l, long n));
static long list_find_nr __ARGS((list_T *l, long idx, int *errorp));
static long list_idx_of_item __ARGS((list_T *l, listitem_T *item));
static void list_append __ARGS((list_T *l, listitem_T *item));
static int list_append_tv __ARGS((list_T *l, typval_T *tv));
static int list_append_number __ARGS((list_T *l, varnumber_T n));
static int list_insert_tv __ARGS((list_T *l, typval_T *tv, listitem_T *item));
static int list_extend __ARGS((list_T	*l1, list_T *l2, listitem_T *bef));
static int list_concat __ARGS((list_T *l1, list_T *l2, typval_T *tv));
static list_T *list_copy __ARGS((list_T *orig, int deep, int copyID));
static void list_remove __ARGS((list_T *l, listitem_T *item, listitem_T *item2));
static char_u *list2string __ARGS((typval_T *tv, int copyID));
static int list_join __ARGS((garray_T *gap, list_T *l, char_u *sep, int echo, int copyID));
static void set_ref_in_ht __ARGS((hashtab_T *ht, int copyID));
static void set_ref_in_list __ARGS((list_T *l, int copyID));
static void set_ref_in_item __ARGS((typval_T *tv, int copyID));
static void dict_unref __ARGS((dict_T *d));
static void dict_free __ARGS((dict_T *d, int recurse));
static dictitem_T *dictitem_alloc __ARGS((char_u *key));
static dictitem_T *dictitem_copy __ARGS((dictitem_T *org));
static void dictitem_remove __ARGS((dict_T *dict, dictitem_T *item));
static void dictitem_free __ARGS((dictitem_T *item));
static dict_T *dict_copy __ARGS((dict_T *orig, int deep, int copyID));
static int dict_add __ARGS((dict_T *d, dictitem_T *item));
static long dict_len __ARGS((dict_T *d));
static dictitem_T *dict_find __ARGS((dict_T *d, char_u *key, int len));
static char_u *dict2string __ARGS((typval_T *tv, int copyID));
static int get_dict_tv __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static char_u *echo_string __ARGS((typval_T *tv, char_u **tofree, char_u *numbuf, int copyID));
static char_u *tv2string __ARGS((typval_T *tv, char_u **tofree, char_u *numbuf, int copyID));
static char_u *string_quote __ARGS((char_u *str, int function));
#ifdef FEAT_FLOAT
static int string2float __ARGS((char_u *text, float_T *value));
#endif
static int get_env_tv __ARGS((char_u **arg, typval_T *rettv, int evaluate));
static int find_internal_func __ARGS((char_u *name));
static char_u *deref_func_name __ARGS((char_u *name, int *lenp));
static int get_func_tv __ARGS((char_u *name, int len, typval_T *rettv, char_u **arg, linenr_T firstline, linenr_T lastline, int *doesrange, int evaluate, dict_T *selfdict));
static int call_func __ARGS((char_u *name, int len, typval_T *rettv, int argcount, typval_T *argvars, linenr_T firstline, linenr_T lastline, int *doesrange, int evaluate, dict_T *selfdict));
static void emsg_funcname __ARGS((char *ermsg, char_u *name));
static int non_zero_arg __ARGS((typval_T *argvars));

#ifdef FEAT_FLOAT
static void f_abs __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_add __ARGS((typval_T *argvars, typval_T *rettv));
static void f_append __ARGS((typval_T *argvars, typval_T *rettv));
static void f_argc __ARGS((typval_T *argvars, typval_T *rettv));
static void f_argidx __ARGS((typval_T *argvars, typval_T *rettv));
static void f_argv __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_atan __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_browse __ARGS((typval_T *argvars, typval_T *rettv));
static void f_browsedir __ARGS((typval_T *argvars, typval_T *rettv));
static void f_bufexists __ARGS((typval_T *argvars, typval_T *rettv));
static void f_buflisted __ARGS((typval_T *argvars, typval_T *rettv));
static void f_bufloaded __ARGS((typval_T *argvars, typval_T *rettv));
static void f_bufname __ARGS((typval_T *argvars, typval_T *rettv));
static void f_bufnr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_bufwinnr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_byte2line __ARGS((typval_T *argvars, typval_T *rettv));
static void f_byteidx __ARGS((typval_T *argvars, typval_T *rettv));
static void f_call __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_ceil __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_changenr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_char2nr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_cindent __ARGS((typval_T *argvars, typval_T *rettv));
static void f_clearmatches __ARGS((typval_T *argvars, typval_T *rettv));
static void f_col __ARGS((typval_T *argvars, typval_T *rettv));
#if defined(FEAT_INS_EXPAND)
static void f_complete __ARGS((typval_T *argvars, typval_T *rettv));
static void f_complete_add __ARGS((typval_T *argvars, typval_T *rettv));
static void f_complete_check __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_confirm __ARGS((typval_T *argvars, typval_T *rettv));
static void f_copy __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_cos __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_count __ARGS((typval_T *argvars, typval_T *rettv));
static void f_cscope_connection __ARGS((typval_T *argvars, typval_T *rettv));
static void f_cursor __ARGS((typval_T *argsvars, typval_T *rettv));
static void f_deepcopy __ARGS((typval_T *argvars, typval_T *rettv));
static void f_delete __ARGS((typval_T *argvars, typval_T *rettv));
static void f_did_filetype __ARGS((typval_T *argvars, typval_T *rettv));
static void f_diff_filler __ARGS((typval_T *argvars, typval_T *rettv));
static void f_diff_hlID __ARGS((typval_T *argvars, typval_T *rettv));
static void f_empty __ARGS((typval_T *argvars, typval_T *rettv));
static void f_escape __ARGS((typval_T *argvars, typval_T *rettv));
static void f_eval __ARGS((typval_T *argvars, typval_T *rettv));
static void f_eventhandler __ARGS((typval_T *argvars, typval_T *rettv));
static void f_executable __ARGS((typval_T *argvars, typval_T *rettv));
static void f_exists __ARGS((typval_T *argvars, typval_T *rettv));
static void f_expand __ARGS((typval_T *argvars, typval_T *rettv));
static void f_extend __ARGS((typval_T *argvars, typval_T *rettv));
static void f_feedkeys __ARGS((typval_T *argvars, typval_T *rettv));
static void f_filereadable __ARGS((typval_T *argvars, typval_T *rettv));
static void f_filewritable __ARGS((typval_T *argvars, typval_T *rettv));
static void f_filter __ARGS((typval_T *argvars, typval_T *rettv));
static void f_finddir __ARGS((typval_T *argvars, typval_T *rettv));
static void f_findfile __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_float2nr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_floor __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_fnameescape __ARGS((typval_T *argvars, typval_T *rettv));
static void f_fnamemodify __ARGS((typval_T *argvars, typval_T *rettv));
static void f_foldclosed __ARGS((typval_T *argvars, typval_T *rettv));
static void f_foldclosedend __ARGS((typval_T *argvars, typval_T *rettv));
static void f_foldlevel __ARGS((typval_T *argvars, typval_T *rettv));
static void f_foldtext __ARGS((typval_T *argvars, typval_T *rettv));
static void f_foldtextresult __ARGS((typval_T *argvars, typval_T *rettv));
static void f_foreground __ARGS((typval_T *argvars, typval_T *rettv));
static void f_function __ARGS((typval_T *argvars, typval_T *rettv));
static void f_garbagecollect __ARGS((typval_T *argvars, typval_T *rettv));
static void f_get __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getbufline __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getbufvar __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getchar __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getcharmod __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getcmdline __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getcmdpos __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getcmdtype __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getcwd __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getfontname __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getfperm __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getfsize __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getftime __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getftype __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getline __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getmatches __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getpid __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getpos __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getqflist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getreg __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getregtype __ARGS((typval_T *argvars, typval_T *rettv));
static void f_gettabwinvar __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getwinposx __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getwinposy __ARGS((typval_T *argvars, typval_T *rettv));
static void f_getwinvar __ARGS((typval_T *argvars, typval_T *rettv));
static void f_glob __ARGS((typval_T *argvars, typval_T *rettv));
static void f_globpath __ARGS((typval_T *argvars, typval_T *rettv));
static void f_has __ARGS((typval_T *argvars, typval_T *rettv));
static void f_has_key __ARGS((typval_T *argvars, typval_T *rettv));
static void f_haslocaldir __ARGS((typval_T *argvars, typval_T *rettv));
static void f_hasmapto __ARGS((typval_T *argvars, typval_T *rettv));
static void f_histadd __ARGS((typval_T *argvars, typval_T *rettv));
static void f_histdel __ARGS((typval_T *argvars, typval_T *rettv));
static void f_histget __ARGS((typval_T *argvars, typval_T *rettv));
static void f_histnr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_hlID __ARGS((typval_T *argvars, typval_T *rettv));
static void f_hlexists __ARGS((typval_T *argvars, typval_T *rettv));
static void f_hostname __ARGS((typval_T *argvars, typval_T *rettv));
static void f_iconv __ARGS((typval_T *argvars, typval_T *rettv));
static void f_indent __ARGS((typval_T *argvars, typval_T *rettv));
static void f_index __ARGS((typval_T *argvars, typval_T *rettv));
static void f_input __ARGS((typval_T *argvars, typval_T *rettv));
static void f_inputdialog __ARGS((typval_T *argvars, typval_T *rettv));
static void f_inputlist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_inputrestore __ARGS((typval_T *argvars, typval_T *rettv));
static void f_inputsave __ARGS((typval_T *argvars, typval_T *rettv));
static void f_inputsecret __ARGS((typval_T *argvars, typval_T *rettv));
static void f_insert __ARGS((typval_T *argvars, typval_T *rettv));
static void f_isdirectory __ARGS((typval_T *argvars, typval_T *rettv));
static void f_islocked __ARGS((typval_T *argvars, typval_T *rettv));
static void f_items __ARGS((typval_T *argvars, typval_T *rettv));
static void f_join __ARGS((typval_T *argvars, typval_T *rettv));
static void f_keys __ARGS((typval_T *argvars, typval_T *rettv));
static void f_last_buffer_nr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_len __ARGS((typval_T *argvars, typval_T *rettv));
static void f_libcall __ARGS((typval_T *argvars, typval_T *rettv));
static void f_libcallnr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_line __ARGS((typval_T *argvars, typval_T *rettv));
static void f_line2byte __ARGS((typval_T *argvars, typval_T *rettv));
static void f_lispindent __ARGS((typval_T *argvars, typval_T *rettv));
static void f_localtime __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_log10 __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_map __ARGS((typval_T *argvars, typval_T *rettv));
static void f_maparg __ARGS((typval_T *argvars, typval_T *rettv));
static void f_mapcheck __ARGS((typval_T *argvars, typval_T *rettv));
static void f_match __ARGS((typval_T *argvars, typval_T *rettv));
static void f_matchadd __ARGS((typval_T *argvars, typval_T *rettv));
static void f_matcharg __ARGS((typval_T *argvars, typval_T *rettv));
static void f_matchdelete __ARGS((typval_T *argvars, typval_T *rettv));
static void f_matchend __ARGS((typval_T *argvars, typval_T *rettv));
static void f_matchlist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_matchstr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_max __ARGS((typval_T *argvars, typval_T *rettv));
static void f_min __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef vim_mkdir
static void f_mkdir __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_mode __ARGS((typval_T *argvars, typval_T *rettv));
static void f_nextnonblank __ARGS((typval_T *argvars, typval_T *rettv));
static void f_nr2char __ARGS((typval_T *argvars, typval_T *rettv));
static void f_pathshorten __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_pow __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_prevnonblank __ARGS((typval_T *argvars, typval_T *rettv));
static void f_printf __ARGS((typval_T *argvars, typval_T *rettv));
static void f_pumvisible __ARGS((typval_T *argvars, typval_T *rettv));
static void f_range __ARGS((typval_T *argvars, typval_T *rettv));
static void f_readfile __ARGS((typval_T *argvars, typval_T *rettv));
static void f_reltime __ARGS((typval_T *argvars, typval_T *rettv));
static void f_reltimestr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_remote_expr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_remote_foreground __ARGS((typval_T *argvars, typval_T *rettv));
static void f_remote_peek __ARGS((typval_T *argvars, typval_T *rettv));
static void f_remote_read __ARGS((typval_T *argvars, typval_T *rettv));
static void f_remote_send __ARGS((typval_T *argvars, typval_T *rettv));
static void f_remove __ARGS((typval_T *argvars, typval_T *rettv));
static void f_rename __ARGS((typval_T *argvars, typval_T *rettv));
static void f_repeat __ARGS((typval_T *argvars, typval_T *rettv));
static void f_resolve __ARGS((typval_T *argvars, typval_T *rettv));
static void f_reverse __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_round __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_search __ARGS((typval_T *argvars, typval_T *rettv));
static void f_searchdecl __ARGS((typval_T *argvars, typval_T *rettv));
static void f_searchpair __ARGS((typval_T *argvars, typval_T *rettv));
static void f_searchpairpos __ARGS((typval_T *argvars, typval_T *rettv));
static void f_searchpos __ARGS((typval_T *argvars, typval_T *rettv));
static void f_server2client __ARGS((typval_T *argvars, typval_T *rettv));
static void f_serverlist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setbufvar __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setcmdpos __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setline __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setloclist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setmatches __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setpos __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setqflist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setreg __ARGS((typval_T *argvars, typval_T *rettv));
static void f_settabwinvar __ARGS((typval_T *argvars, typval_T *rettv));
static void f_setwinvar __ARGS((typval_T *argvars, typval_T *rettv));
static void f_shellescape __ARGS((typval_T *argvars, typval_T *rettv));
static void f_simplify __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_sin __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_sort __ARGS((typval_T *argvars, typval_T *rettv));
static void f_soundfold __ARGS((typval_T *argvars, typval_T *rettv));
static void f_spellbadword __ARGS((typval_T *argvars, typval_T *rettv));
static void f_spellsuggest __ARGS((typval_T *argvars, typval_T *rettv));
static void f_split __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_sqrt __ARGS((typval_T *argvars, typval_T *rettv));
static void f_str2float __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_str2nr __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef HAVE_STRFTIME
static void f_strftime __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_stridx __ARGS((typval_T *argvars, typval_T *rettv));
static void f_string __ARGS((typval_T *argvars, typval_T *rettv));
static void f_strlen __ARGS((typval_T *argvars, typval_T *rettv));
static void f_strpart __ARGS((typval_T *argvars, typval_T *rettv));
static void f_strridx __ARGS((typval_T *argvars, typval_T *rettv));
static void f_strtrans __ARGS((typval_T *argvars, typval_T *rettv));
static void f_submatch __ARGS((typval_T *argvars, typval_T *rettv));
static void f_substitute __ARGS((typval_T *argvars, typval_T *rettv));
static void f_synID __ARGS((typval_T *argvars, typval_T *rettv));
static void f_synIDattr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_synIDtrans __ARGS((typval_T *argvars, typval_T *rettv));
static void f_synstack __ARGS((typval_T *argvars, typval_T *rettv));
static void f_system __ARGS((typval_T *argvars, typval_T *rettv));
static void f_tabpagebuflist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_tabpagenr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_tabpagewinnr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_taglist __ARGS((typval_T *argvars, typval_T *rettv));
static void f_tagfiles __ARGS((typval_T *argvars, typval_T *rettv));
static void f_tempname __ARGS((typval_T *argvars, typval_T *rettv));
static void f_test __ARGS((typval_T *argvars, typval_T *rettv));
static void f_tolower __ARGS((typval_T *argvars, typval_T *rettv));
static void f_toupper __ARGS((typval_T *argvars, typval_T *rettv));
static void f_tr __ARGS((typval_T *argvars, typval_T *rettv));
#ifdef FEAT_FLOAT
static void f_trunc __ARGS((typval_T *argvars, typval_T *rettv));
#endif
static void f_type __ARGS((typval_T *argvars, typval_T *rettv));
static void f_values __ARGS((typval_T *argvars, typval_T *rettv));
static void f_virtcol __ARGS((typval_T *argvars, typval_T *rettv));
static void f_visualmode __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winbufnr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_wincol __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winheight __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winline __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winnr __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winrestcmd __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winrestview __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winsaveview __ARGS((typval_T *argvars, typval_T *rettv));
static void f_winwidth __ARGS((typval_T *argvars, typval_T *rettv));
static void f_writefile __ARGS((typval_T *argvars, typval_T *rettv));

static int list2fpos __ARGS((typval_T *arg, pos_T *posp, int *fnump));
static pos_T *var2fpos __ARGS((typval_T *varp, int dollar_lnum, int *fnum));
static int get_env_len __ARGS((char_u **arg));
static int get_id_len __ARGS((char_u **arg));
static int get_name_len __ARGS((char_u **arg, char_u **alias, int evaluate, int verbose));
static char_u *find_name_end __ARGS((char_u *arg, char_u **expr_start, char_u **expr_end, int flags));
#define FNE_INCL_BR	1	/* find_name_end(): include [] in name */
#define FNE_CHECK_START	2	/* find_name_end(): check name starts with
				   valid character */
static char_u * make_expanded_name __ARGS((char_u *in_start, char_u *expr_start, char_u *expr_end, char_u *in_end));
static int eval_isnamec __ARGS((int c));
static int eval_isnamec1 __ARGS((int c));
static int get_var_tv __ARGS((char_u *name, int len, typval_T *rettv, int verbose));
static int handle_subscript __ARGS((char_u **arg, typval_T *rettv, int evaluate, int verbose));
static typval_T *alloc_tv __ARGS((void));
static typval_T *alloc_string_tv __ARGS((char_u *string));
static void init_tv __ARGS((typval_T *varp));
static long get_tv_number __ARGS((typval_T *varp));
static linenr_T get_tv_lnum __ARGS((typval_T *argvars));
static linenr_T get_tv_lnum_buf __ARGS((typval_T *argvars, buf_T *buf));
static char_u *get_tv_string __ARGS((typval_T *varp));
static char_u *get_tv_string_buf __ARGS((typval_T *varp, char_u *buf));
static char_u *get_tv_string_buf_chk __ARGS((typval_T *varp, char_u *buf));
static dictitem_T *find_var __ARGS((char_u *name, hashtab_T **htp));
static dictitem_T *find_var_in_ht __ARGS((hashtab_T *ht, char_u *varname, int writing));
static hashtab_T *find_var_ht __ARGS((char_u *name, char_u **varname));
static void vars_clear_ext __ARGS((hashtab_T *ht, int free_val));
static void delete_var __ARGS((hashtab_T *ht, hashitem_T *hi));
static void list_one_var __ARGS((dictitem_T *v, char_u *prefix, int *first));
static void list_one_var_a __ARGS((char_u *prefix, char_u *name, int type, char_u *string, int *first));
static void set_var __ARGS((char_u *name, typval_T *varp, int copy));
static int var_check_ro __ARGS((int flags, char_u *name));
static int var_check_fixed __ARGS((int flags, char_u *name));
static int tv_check_lock __ARGS((int lock, char_u *name));
static void copy_tv __ARGS((typval_T *from, typval_T *to));
static int item_copy __ARGS((typval_T *from, typval_T *to, int deep, int copyID));
static char_u *find_option_end __ARGS((char_u **arg, int *opt_flags));
static char_u *trans_function_name __ARGS((char_u **pp, int skip, int flags, funcdict_T *fd));
static int eval_fname_script __ARGS((char_u *p));
static int eval_fname_sid __ARGS((char_u *p));
static void list_func_head __ARGS((ufunc_T *fp, int indent));
static ufunc_T *find_func __ARGS((char_u *name));
static int function_exists __ARGS((char_u *name));
static int builtin_function __ARGS((char_u *name));
#ifdef FEAT_PROFILE
static void func_do_profile __ARGS((ufunc_T *fp));
static void prof_sort_list __ARGS((FILE *fd, ufunc_T **sorttab, int st_len, char *title, int prefer_self));
static void prof_func_line __ARGS((FILE *fd, int count, proftime_T *total, proftime_T *self, int prefer_self));
static int
# ifdef __BORLANDC__
    _RTLENTRYF
# endif
	prof_total_cmp __ARGS((const void *s1, const void *s2));
static int
# ifdef __BORLANDC__
    _RTLENTRYF
# endif
	prof_self_cmp __ARGS((const void *s1, const void *s2));
#endif
static int script_autoload __ARGS((char_u *name, int reload));
static char_u *autoload_name __ARGS((char_u *name));
static void cat_func_name __ARGS((char_u *buf, ufunc_T *fp));
static void func_free __ARGS((ufunc_T *fp));
static void func_unref __ARGS((char_u *name));
static void func_ref __ARGS((char_u *name));
static void call_user_func __ARGS((ufunc_T *fp, int argcount, typval_T *argvars, typval_T *rettv, linenr_T firstline, linenr_T lastline, dict_T *selfdict));
static int can_free_funccal __ARGS((funccall_T *fc, int copyID)) ;
static void free_funccal __ARGS((funccall_T *fc, int free_val));
static void add_nr_var __ARGS((dict_T *dp, dictitem_T *v, char *name, varnumber_T nr));
static win_T *find_win_by_nr __ARGS((typval_T *vp, tabpage_T *tp));
static void getwinvar __ARGS((typval_T *argvars, typval_T *rettv, int off));
static int searchpair_cmn __ARGS((typval_T *argvars, pos_T *match_pos));
static int search_cmn __ARGS((typval_T *argvars, pos_T *match_pos, int *flagsp));
static void setwinvar __ARGS((typval_T *argvars, typval_T *rettv, int off));

/* Character used as separated in autoload function/variable names. */
#define AUTOLOAD_CHAR '#'

/*
 * Initialize the global and v: variables.
 */
    void
eval_init()
{
    int		    i;
    struct vimvar   *p;

    init_var_dict(&globvardict, &globvars_var);
    init_var_dict(&vimvardict, &vimvars_var);
    hash_init(&compat_hashtab);
    hash_init(&func_hashtab);

    for (i = 0; i < VV_LEN; ++i)
    {
	p = &vimvars[i];
	STRCPY(p->vv_di.di_key, p->vv_name);
	if (p->vv_flags & VV_RO)
	    p->vv_di.di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
	else if (p->vv_flags & VV_RO_SBX)
	    p->vv_di.di_flags = DI_FLAGS_RO_SBX | DI_FLAGS_FIX;
	else
	    p->vv_di.di_flags = DI_FLAGS_FIX;

	/* add to v: scope dict, unless the value is not always available */
	if (p->vv_type != VAR_UNKNOWN)
	    hash_add(&vimvarht, p->vv_di.di_key);
	if (p->vv_flags & VV_COMPAT)
	    /* add to compat scope dict */
	    hash_add(&compat_hashtab, p->vv_di.di_key);
    }
    set_vim_var_nr(VV_SEARCHFORWARD, 1L);
}

#if defined(EXITFREE) || defined(PROTO)
    void
eval_clear()
{
    int		    i;
    struct vimvar   *p;

    for (i = 0; i < VV_LEN; ++i)
    {
	p = &vimvars[i];
	if (p->vv_di.di_tv.v_type == VAR_STRING)
	{
	    vim_free(p->vv_str);
	    p->vv_str = NULL;
	}
	else if (p->vv_di.di_tv.v_type == VAR_LIST)
	{
	    list_unref(p->vv_list);
	    p->vv_list = NULL;
	}
    }
    hash_clear(&vimvarht);
    hash_init(&vimvarht);  /* garbage_collect() will access it */
    hash_clear(&compat_hashtab);

    /* script-local variables */
    for (i = 1; i <= ga_scripts.ga_len; ++i)
	vars_clear(&SCRIPT_VARS(i));
    ga_clear(&ga_scripts);
    free_scriptnames();

    /* global variables */
    vars_clear(&globvarht);

    /* autoloaded script names */
    ga_clear_strings(&ga_loaded);

    /* unreferenced lists and dicts */
    (void)garbage_collect();

    /* functions */
    free_all_functions();
    hash_clear(&func_hashtab);
}
#endif

/*
 * Return the name of the executed function.
 */
    char_u *
func_name(cookie)
    void *cookie;
{
    return ((funccall_T *)cookie)->func->uf_name;
}

/*
 * Return the address holding the next breakpoint line for a funccall cookie.
 */
    linenr_T *
func_breakpoint(cookie)
    void *cookie;
{
    return &((funccall_T *)cookie)->breakpoint;
}

/*
 * Return the address holding the debug tick for a funccall cookie.
 */
    int *
func_dbg_tick(cookie)
    void *cookie;
{
    return &((funccall_T *)cookie)->dbg_tick;
}

/*
 * Return the nesting level for a funccall cookie.
 */
    int
func_level(cookie)
    void *cookie;
{
    return ((funccall_T *)cookie)->level;
}

/* pointer to funccal for currently active function */
funccall_T *current_funccal = NULL;

/* pointer to list of previously used funccal, still around because some
 * item in it is still being used. */
funccall_T *previous_funccal = NULL;

/*
 * Return TRUE when a function was ended by a ":return" command.
 */
    int
current_func_returned()
{
    return current_funccal->returned;
}


/*
 * Set an internal variable to a string value. Creates the variable if it does
 * not already exist.
 */
    void
set_internal_string_var(name, value)
    char_u	*name;
    char_u	*value;
{
    char_u	*val;
    typval_T	*tvp;

    val = vim_strsave(value);
    if (val != NULL)
    {
	tvp = alloc_string_tv(val);
	if (tvp != NULL)
	{
	    set_var(name, tvp, FALSE);
	    free_tv(tvp);
	}
    }
}

static lval_T	*redir_lval = NULL;
static garray_T redir_ga;	/* only valid when redir_lval is not NULL */
static char_u	*redir_endp = NULL;
static char_u	*redir_varname = NULL;

/*
 * Start recording command output to a variable
 * Returns OK if successfully completed the setup.  FAIL otherwise.
 */
    int
var_redir_start(name, append)
    char_u	*name;
    int		append;		/* append to an existing variable */
{
    int		save_emsg;
    int		err;
    typval_T	tv;

    /* Make sure a valid variable name is specified */
    if (!eval_isnamec1(*name))
    {
	EMSG(_(e_invarg));
	return FAIL;
    }

    redir_varname = vim_strsave(name);
    if (redir_varname == NULL)
	return FAIL;

    redir_lval = (lval_T *)alloc_clear((unsigned)sizeof(lval_T));
    if (redir_lval == NULL)
    {
	var_redir_stop();
	return FAIL;
    }

    /* The output is stored in growarray "redir_ga" until redirection ends. */
    ga_init2(&redir_ga, (int)sizeof(char), 500);

    /* Parse the variable name (can be a dict or list entry). */
    redir_endp = get_lval(redir_varname, NULL, redir_lval, FALSE, FALSE, FALSE,
							     FNE_CHECK_START);
    if (redir_endp == NULL || redir_lval->ll_name == NULL || *redir_endp != NUL)
    {
	if (redir_endp != NULL && *redir_endp != NUL)
	    /* Trailing characters are present after the variable name */
	    EMSG(_(e_trailing));
	else
	    EMSG(_(e_invarg));
	var_redir_stop();
	return FAIL;
    }

    /* check if we can write to the variable: set it to or append an empty
     * string */
    save_emsg = did_emsg;
    did_emsg = FALSE;
    tv.v_type = VAR_STRING;
    tv.vval.v_string = (char_u *)"";
    if (append)
	set_var_lval(redir_lval, redir_endp, &tv, TRUE, (char_u *)".");
    else
	set_var_lval(redir_lval, redir_endp, &tv, TRUE, (char_u *)"=");
    err = did_emsg;
    did_emsg |= save_emsg;
    if (err)
    {
	var_redir_stop();
	return FAIL;
    }
    if (redir_lval->ll_newkey != NULL)
    {
	/* Dictionary item was created, don't do it again. */
	vim_free(redir_lval->ll_newkey);
	redir_lval->ll_newkey = NULL;
    }

    return OK;
}

/*
 * Append "value[value_len]" to the variable set by var_redir_start().
 * The actual appending is postponed until redirection ends, because the value
 * appended may in fact be the string we write to, changing it may cause freed
 * memory to be used:
 *   :redir => foo
 *   :let foo
 *   :redir END
 */
    void
var_redir_str(value, value_len)
    char_u	*value;
    int		value_len;
{
    int		len;

    if (redir_lval == NULL)
	return;

    if (value_len == -1)
	len = (int)STRLEN(value);	/* Append the entire string */
    else
	len = value_len;		/* Append only "value_len" characters */

    if (ga_grow(&redir_ga, len) == OK)
    {
	mch_memmove((char *)redir_ga.ga_data + redir_ga.ga_len, value, len);
	redir_ga.ga_len += len;
    }
    else
	var_redir_stop();
}

/*
 * Stop redirecting command output to a variable.
 */
    void
var_redir_stop()
{
    typval_T	tv;

    if (redir_lval != NULL)
    {
	/* Append the trailing NUL. */
	ga_append(&redir_ga, NUL);

	/* Assign the text to the variable. */
	tv.v_type = VAR_STRING;
	tv.vval.v_string = redir_ga.ga_data;
	set_var_lval(redir_lval, redir_endp, &tv, FALSE, (char_u *)".");
	vim_free(tv.vval.v_string);

	clear_lval(redir_lval);
	vim_free(redir_lval);
	redir_lval = NULL;
    }
    vim_free(redir_varname);
    redir_varname = NULL;
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
    typval_T	tv;
    int		retval = FALSE;

    if (skip)
	++emsg_skip;
    if (eval0(arg, &tv, nextcmd, !skip) == FAIL)
	*error = TRUE;
    else
    {
	*error = FALSE;
	if (!skip)
	{
	    retval = (get_tv_number_chk(&tv, error) != 0);
	    clear_tv(&tv);
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
    typval_T	tv;
    char_u	*retval;

    if (skip)
	++emsg_skip;
    if (eval0(arg, &tv, nextcmd, !skip) == FAIL || skip)
	retval = NULL;
    else
    {
	retval = vim_strsave(get_tv_string(&tv));
	clear_tv(&tv);
    }
    if (skip)
	--emsg_skip;

    return retval;
}

/*
 * Skip over an expression at "*pp".
 * Return FAIL for an error, OK otherwise.
 */
    int
skip_expr(pp)
    char_u	**pp;
{
    typval_T	rettv;

    *pp = skipwhite(*pp);
    return eval1(pp, &rettv, FALSE);
}

/*
 * Top level evaluation function, returning a string.
 * When "convert" is TRUE convert a List into a sequence of lines and convert
 * a Float to a String.
 * Return pointer to allocated memory, or NULL for failure.
 */
    char_u *
eval_to_string(arg, nextcmd, convert)
    char_u	*arg;
    char_u	**nextcmd;
    int		convert;
{
    typval_T	tv;
    char_u	*retval;
    garray_T	ga;
    char_u	numbuf[NUMBUFLEN];

    if (eval0(arg, &tv, nextcmd, TRUE) == FAIL)
	retval = NULL;
    else
    {
	if (convert && tv.v_type == VAR_LIST)
	{
	    ga_init2(&ga, (int)sizeof(char), 80);
	    if (tv.vval.v_list != NULL)
		list_join(&ga, tv.vval.v_list, (char_u *)"\n", TRUE, 0);
	    ga_append(&ga, NUL);
	    retval = (char_u *)ga.ga_data;
	}
#ifdef FEAT_FLOAT
	else if (convert && tv.v_type == VAR_FLOAT)
	{
	    vim_snprintf((char *)numbuf, NUMBUFLEN, "%g", tv.vval.v_float);
	    retval = vim_strsave(numbuf);
	}
#endif
	else
	    retval = vim_strsave(get_tv_string(&tv));
	clear_tv(&tv);
    }

    return retval;
}

/*
 * Call eval_to_string() without using current local variables and using
 * textlock.  When "use_sandbox" is TRUE use the sandbox.
 */
    char_u *
eval_to_string_safe(arg, nextcmd, use_sandbox)
    char_u	*arg;
    char_u	**nextcmd;
    int		use_sandbox;
{
    char_u	*retval;
    void	*save_funccalp;

    save_funccalp = save_funccal();
    if (use_sandbox)
	++sandbox;
    ++textlock;
    retval = eval_to_string(arg, nextcmd, FALSE);
    if (use_sandbox)
	--sandbox;
    --textlock;
    restore_funccal(save_funccalp);
    return retval;
}

/*
 * Top level evaluation function, returning a number.
 * Evaluates "expr" silently.
 * Returns -1 for an error.
 */
    int
eval_to_number(expr)
    char_u	*expr;
{
    typval_T	rettv;
    int		retval;
    char_u	*p = skipwhite(expr);

    ++emsg_off;

    if (eval1(&p, &rettv, TRUE) == FAIL)
	retval = -1;
    else
    {
	retval = get_tv_number_chk(&rettv, NULL);
	clear_tv(&rettv);
    }
    --emsg_off;

    return retval;
}

/*
 * Prepare v: variable "idx" to be used.
 * Save the current typeval in "save_tv".
 * When not used yet add the variable to the v: hashtable.
 */
    static void
prepare_vimvar(idx, save_tv)
    int		idx;
    typval_T	*save_tv;
{
    *save_tv = vimvars[idx].vv_tv;
    if (vimvars[idx].vv_type == VAR_UNKNOWN)
	hash_add(&vimvarht, vimvars[idx].vv_di.di_key);
}

/*
 * Restore v: variable "idx" to typeval "save_tv".
 * When no longer defined, remove the variable from the v: hashtable.
 */
    static void
restore_vimvar(idx, save_tv)
    int		idx;
    typval_T	*save_tv;
{
    hashitem_T	*hi;

    vimvars[idx].vv_tv = *save_tv;
    if (vimvars[idx].vv_type == VAR_UNKNOWN)
    {
	hi = hash_find(&vimvarht, vimvars[idx].vv_di.di_key);
	if (HASHITEM_EMPTY(hi))
	    EMSG2(_(e_intern2), "restore_vimvar()");
	else
	    hash_remove(&vimvarht, hi);
    }
}

#if defined(FEAT_SPELL) || defined(PROTO)
/*
 * Evaluate an expression to a list with suggestions.
 * For the "expr:" part of 'spellsuggest'.
 * Returns NULL when there is an error.
 */
    list_T *
eval_spell_expr(badword, expr)
    char_u	*badword;
    char_u	*expr;
{
    typval_T	save_val;
    typval_T	rettv;
    list_T	*list = NULL;
    char_u	*p = skipwhite(expr);

    /* Set "v:val" to the bad word. */
    prepare_vimvar(VV_VAL, &save_val);
    vimvars[VV_VAL].vv_type = VAR_STRING;
    vimvars[VV_VAL].vv_str = badword;
    if (p_verbose == 0)
	++emsg_off;

    if (eval1(&p, &rettv, TRUE) == OK)
    {
	if (rettv.v_type != VAR_LIST)
	    clear_tv(&rettv);
	else
	    list = rettv.vval.v_list;
    }

    if (p_verbose == 0)
	--emsg_off;
    restore_vimvar(VV_VAL, &save_val);

    return list;
}

/*
 * "list" is supposed to contain two items: a word and a number.  Return the
 * word in "pp" and the number as the return value.
 * Return -1 if anything isn't right.
 * Used to get the good word and score from the eval_spell_expr() result.
 */
    int
get_spellword(list, pp)
    list_T	*list;
    char_u	**pp;
{
    listitem_T	*li;

    li = list->lv_first;
    if (li == NULL)
	return -1;
    *pp = get_tv_string(&li->li_tv);

    li = li->li_next;
    if (li == NULL)
	return -1;
    return get_tv_number(&li->li_tv);
}
#endif

/*
 * Top level evaluation function.
 * Returns an allocated typval_T with the result.
 * Returns NULL when there is an error.
 */
    typval_T *
eval_expr(arg, nextcmd)
    char_u	*arg;
    char_u	**nextcmd;
{
    typval_T	*tv;

    tv = (typval_T *)alloc(sizeof(typval_T));
    if (tv != NULL && eval0(arg, tv, nextcmd, TRUE) == FAIL)
    {
	vim_free(tv);
	tv = NULL;
    }

    return tv;
}


#if (defined(FEAT_USR_CMDS) && defined(FEAT_CMDL_COMPL)) \
	|| defined(FEAT_COMPL_FUNC) || defined(PROTO)
/*
 * Call some vimL function and return the result in "*rettv".
 * Uses argv[argc] for the function arguments.  Only Number and String
 * arguments are currently supported.
 * Returns OK or FAIL.
 */
    static int
call_vim_function(func, argc, argv, safe, rettv)
    char_u      *func;
    int		argc;
    char_u      **argv;
    int		safe;		/* use the sandbox */
    typval_T	*rettv;
{
    typval_T	*argvars;
    long	n;
    int		len;
    int		i;
    int		doesrange;
    void	*save_funccalp = NULL;
    int		ret;

    argvars = (typval_T *)alloc((unsigned)((argc + 1) * sizeof(typval_T)));
    if (argvars == NULL)
	return FAIL;

    for (i = 0; i < argc; i++)
    {
	/* Pass a NULL or empty argument as an empty string */
	if (argv[i] == NULL || *argv[i] == NUL)
	{
	    argvars[i].v_type = VAR_STRING;
	    argvars[i].vval.v_string = (char_u *)"";
	    continue;
	}

	/* Recognize a number argument, the others must be strings. */
	vim_str2nr(argv[i], NULL, &len, TRUE, TRUE, &n, NULL);
	if (len != 0 && len == (int)STRLEN(argv[i]))
	{
	    argvars[i].v_type = VAR_NUMBER;
	    argvars[i].vval.v_number = n;
	}
	else
	{
	    argvars[i].v_type = VAR_STRING;
	    argvars[i].vval.v_string = argv[i];
	}
    }

    if (safe)
    {
	save_funccalp = save_funccal();
	++sandbox;
    }

    rettv->v_type = VAR_UNKNOWN;		/* clear_tv() uses this */
    ret = call_func(func, (int)STRLEN(func), rettv, argc, argvars,
		    curwin->w_cursor.lnum, curwin->w_cursor.lnum,
		    &doesrange, TRUE, NULL);
    if (safe)
    {
	--sandbox;
	restore_funccal(save_funccalp);
    }
    vim_free(argvars);

    if (ret == FAIL)
	clear_tv(rettv);

    return ret;
}

# if (defined(FEAT_USR_CMDS) && defined(FEAT_CMDL_COMPL)) || defined(PROTO)
/*
 * Call vimL function "func" and return the result as a string.
 * Returns NULL when calling the function fails.
 * Uses argv[argc] for the function arguments.
 */
    void *
call_func_retstr(func, argc, argv, safe)
    char_u      *func;
    int		argc;
    char_u      **argv;
    int		safe;		/* use the sandbox */
{
    typval_T	rettv;
    char_u	*retval;

    if (call_vim_function(func, argc, argv, safe, &rettv) == FAIL)
	return NULL;

    retval = vim_strsave(get_tv_string(&rettv));
    clear_tv(&rettv);
    return retval;
}
# endif

# if defined(FEAT_COMPL_FUNC) || defined(PROTO)
/*
 * Call vimL function "func" and return the result as a number.
 * Returns -1 when calling the function fails.
 * Uses argv[argc] for the function arguments.
 */
    long
call_func_retnr(func, argc, argv, safe)
    char_u      *func;
    int		argc;
    char_u      **argv;
    int		safe;		/* use the sandbox */
{
    typval_T	rettv;
    long	retval;

    if (call_vim_function(func, argc, argv, safe, &rettv) == FAIL)
	return -1;

    retval = get_tv_number_chk(&rettv, NULL);
    clear_tv(&rettv);
    return retval;
}
# endif

/*
 * Call vimL function "func" and return the result as a List.
 * Uses argv[argc] for the function arguments.
 * Returns NULL when there is something wrong.
 */
    void *
call_func_retlist(func, argc, argv, safe)
    char_u      *func;
    int		argc;
    char_u      **argv;
    int		safe;		/* use the sandbox */
{
    typval_T	rettv;

    if (call_vim_function(func, argc, argv, safe, &rettv) == FAIL)
	return NULL;

    if (rettv.v_type != VAR_LIST)
    {
	clear_tv(&rettv);
	return NULL;
    }

    return rettv.vval.v_list;
}
#endif


/*
 * Save the current function call pointer, and set it to NULL.
 * Used when executing autocommands and for ":source".
 */
    void *
save_funccal()
{
    funccall_T *fc = current_funccal;

    current_funccal = NULL;
    return (void *)fc;
}

    void
restore_funccal(vfc)
    void *vfc;
{
    funccall_T *fc = (funccall_T *)vfc;

    current_funccal = fc;
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Prepare profiling for entering a child or something else that is not
 * counted for the script/function itself.
 * Should always be called in pair with prof_child_exit().
 */
    void
prof_child_enter(tm)
    proftime_T *tm;	/* place to store waittime */
{
    funccall_T *fc = current_funccal;

    if (fc != NULL && fc->func->uf_profiling)
	profile_start(&fc->prof_child);
    script_prof_save(tm);
}

/*
 * Take care of time spent in a child.
 * Should always be called after prof_child_enter().
 */
    void
prof_child_exit(tm)
    proftime_T *tm;	/* where waittime was stored */
{
    funccall_T *fc = current_funccal;

    if (fc != NULL && fc->func->uf_profiling)
    {
	profile_end(&fc->prof_child);
	profile_sub_wait(tm, &fc->prof_child); /* don't count waiting time */
	profile_add(&fc->func->uf_tm_children, &fc->prof_child);
	profile_add(&fc->func->uf_tml_children, &fc->prof_child);
    }
    script_prof_restore(tm);
}
#endif


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
    typval_T	tv;
    int		retval;
    char_u	*s;
    int		use_sandbox = was_set_insecurely((char_u *)"foldexpr",
								   OPT_LOCAL);

    ++emsg_off;
    if (use_sandbox)
	++sandbox;
    ++textlock;
    *cp = NUL;
    if (eval0(arg, &tv, NULL, TRUE) == FAIL)
	retval = 0;
    else
    {
	/* If the result is a number, just return the number. */
	if (tv.v_type == VAR_NUMBER)
	    retval = tv.vval.v_number;
	else if (tv.v_type != VAR_STRING || tv.vval.v_string == NULL)
	    retval = 0;
	else
	{
	    /* If the result is a string, check if there is a non-digit before
	     * the number. */
	    s = tv.vval.v_string;
	    if (!VIM_ISDIGIT(*s) && *s != '-')
		*cp = *s++;
	    retval = atol((char *)s);
	}
	clear_tv(&tv);
    }
    --emsg_off;
    if (use_sandbox)
	--sandbox;
    --textlock;

    return retval;
}
#endif

/*
 * ":let"			list all variable values
 * ":let var1 var2"		list variable values
 * ":let var = expr"		assignment command.
 * ":let var += expr"		assignment command.
 * ":let var -= expr"		assignment command.
 * ":let var .= expr"		assignment command.
 * ":let [var1, var2] = expr"	unpack list.
 */
    void
ex_let(eap)
    exarg_T	*eap;
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

    argend = skip_var_list(arg, &var_count, &semicolon);
    if (argend == NULL)
	return;
    if (argend > arg && argend[-1] == '.')  /* for var.='str' */
	--argend;
    expr = vim_strchr(argend, '=');
    if (expr == NULL)
    {
	/*
	 * ":let" without "=": list variables
	 */
	if (*arg == '[')
	    EMSG(_(e_invarg));
	else if (!ends_excmd(*arg))
	    /* ":let var1 var2" */
	    arg = list_arg_vars(eap, arg, &first);
	else if (!eap->skip)
	{
	    /* ":let" */
	    list_glob_vars(&first);
	    list_buf_vars(&first);
	    list_win_vars(&first);
#ifdef FEAT_WINDOWS
	    list_tab_vars(&first);
#endif
	    list_script_vars(&first);
	    list_func_vars(&first);
	    list_vim_vars(&first);
	}
	eap->nextcmd = check_nextcmd(arg);
    }
    else
    {
	op[0] = '=';
	op[1] = NUL;
	if (expr > argend)
	{
	    if (vim_strchr((char_u *)"+-.", expr[-1]) != NULL)
		op[0] = expr[-1];   /* +=, -= or .= */
	}
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
									  op);
	    clear_tv(&rettv);
	}
    }
}

/*
 * Assign the typevalue "tv" to the variable or variables at "arg_start".
 * Handles both "var" with any type and "[var, var; var]" with a list type.
 * When "nextchars" is not NULL it points to a string with characters that
 * must appear after the variable(s).  Use "+", "-" or "." for add, subtract
 * or concatenate.
 * Returns OK or FAIL;
 */
    static int
ex_let_vars(arg_start, tv, copy, semicolon, var_count, nextchars)
    char_u	*arg_start;
    typval_T	*tv;
    int		copy;		/* copy values from "tv", don't move */
    int		semicolon;	/* from skip_var_list() */
    int		var_count;	/* from skip_var_list() */
    char_u	*nextchars;
{
    char_u	*arg = arg_start;
    list_T	*l;
    int		i;
    listitem_T	*item;
    typval_T	ltv;

    if (*arg != '[')
    {
	/*
	 * ":let var = expr" or ":for var in list"
	 */
	if (ex_let_one(arg, tv, copy, nextchars, nextchars) == NULL)
	    return FAIL;
	return OK;
    }

    /*
     * ":let [v1, v2] = list" or ":for [v1, v2] in listlist"
     */
    if (tv->v_type != VAR_LIST || (l = tv->vval.v_list) == NULL)
    {
	EMSG(_(e_listreq));
	return FAIL;
    }

    i = list_len(l);
    if (semicolon == 0 && var_count < i)
    {
	EMSG(_("E687: Less targets than List items"));
	return FAIL;
    }
    if (var_count - semicolon > i)
    {
	EMSG(_("E688: More targets than List items"));
	return FAIL;
    }

    item = l->lv_first;
    while (*arg != ']')
    {
	arg = skipwhite(arg + 1);
	arg = ex_let_one(arg, &item->li_tv, TRUE, (char_u *)",;]", nextchars);
	item = item->li_next;
	if (arg == NULL)
	    return FAIL;

	arg = skipwhite(arg);
	if (*arg == ';')
	{
	    /* Put the rest of the list (may be empty) in the var after ';'.
	     * Create a new list for this. */
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

	    arg = ex_let_one(skipwhite(arg + 1), &ltv, FALSE,
						    (char_u *)"]", nextchars);
	    clear_tv(&ltv);
	    if (arg == NULL)
		return FAIL;
	    break;
	}
	else if (*arg != ',' && *arg != ']')
	{
	    EMSG2(_(e_intern2), "ex_let_vars()");
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
    static char_u *
skip_var_list(arg, var_count, semicolon)
    char_u	*arg;
    int		*var_count;
    int		*semicolon;
{
    char_u	*p, *s;

    if (*arg == '[')
    {
	/* "[var, var]": find the matching ']'. */
	p = arg;
	for (;;)
	{
	    p = skipwhite(p + 1);	/* skip whites after '[', ';' or ',' */
	    s = skip_var_one(p);
	    if (s == p)
	    {
		EMSG2(_(e_invarg2), p);
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
		    EMSG(_("Double ; in list of variables"));
		    return NULL;
		}
		*semicolon = 1;
	    }
	    else if (*p != ',')
	    {
		EMSG2(_(e_invarg2), p);
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
skip_var_one(arg)
    char_u	*arg;
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
    static void
list_hashtable_vars(ht, prefix, empty, first)
    hashtab_T	*ht;
    char_u	*prefix;
    int		empty;
    int		*first;
{
    hashitem_T	*hi;
    dictitem_T	*di;
    int		todo;

    todo = (int)ht->ht_used;
    for (hi = ht->ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    di = HI2DI(hi);
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
list_glob_vars(first)
    int *first;
{
    list_hashtable_vars(&globvarht, (char_u *)"", TRUE, first);
}

/*
 * List buffer variables.
 */
    static void
list_buf_vars(first)
    int *first;
{
    char_u	numbuf[NUMBUFLEN];

    list_hashtable_vars(&curbuf->b_vars.dv_hashtab, (char_u *)"b:",
								 TRUE, first);

    sprintf((char *)numbuf, "%ld", (long)curbuf->b_changedtick);
    list_one_var_a((char_u *)"b:", (char_u *)"changedtick", VAR_NUMBER,
							       numbuf, first);
}

/*
 * List window variables.
 */
    static void
list_win_vars(first)
    int *first;
{
    list_hashtable_vars(&curwin->w_vars.dv_hashtab,
						 (char_u *)"w:", TRUE, first);
}

#ifdef FEAT_WINDOWS
/*
 * List tab page variables.
 */
    static void
list_tab_vars(first)
    int *first;
{
    list_hashtable_vars(&curtab->tp_vars.dv_hashtab,
						 (char_u *)"t:", TRUE, first);
}
#endif

/*
 * List Vim variables.
 */
    static void
list_vim_vars(first)
    int *first;
{
    list_hashtable_vars(&vimvarht, (char_u *)"v:", FALSE, first);
}

/*
 * List script-local variables, if there is a script.
 */
    static void
list_script_vars(first)
    int *first;
{
    if (current_SID > 0 && current_SID <= ga_scripts.ga_len)
	list_hashtable_vars(&SCRIPT_VARS(current_SID),
						(char_u *)"s:", FALSE, first);
}

/*
 * List function variables, if there is a function.
 */
    static void
list_func_vars(first)
    int *first;
{
    if (current_funccal != NULL)
	list_hashtable_vars(&current_funccal->l_vars.dv_hashtab,
						(char_u *)"l:", FALSE, first);
}

/*
 * List variables in "arg".
 */
    static char_u *
list_arg_vars(eap, arg, first)
    exarg_T	*eap;
    char_u	*arg;
    int		*first;
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
	    if (!vim_iswhite(*arg) && !ends_excmd(*arg))
	    {
		emsg_severe = TRUE;
		EMSG(_(e_trailing));
		break;
	    }
	}
	else
	{
	    /* get_name_len() takes care of expanding curly braces */
	    name_start = name = arg;
	    len = get_name_len(&arg, &tofree, TRUE, TRUE);
	    if (len <= 0)
	    {
		/* This is mainly to keep test 49 working: when expanding
		 * curly braces fails overrule the exception error message. */
		if (len < 0 && !aborting())
		{
		    emsg_severe = TRUE;
		    EMSG2(_(e_invarg2), arg);
		    break;
		}
		error = TRUE;
	    }
	    else
	    {
		if (tofree != NULL)
		    name = tofree;
		if (get_var_tv(name, len, &tv, TRUE) == FAIL)
		    error = TRUE;
		else
		{
		    /* handle d.key, l[idx], f(expr) */
		    arg_subsc = arg;
		    if (handle_subscript(&arg, &tv, TRUE, TRUE) == FAIL)
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
#ifdef FEAT_WINDOWS
				case 't': list_tab_vars(first); break;
#endif
				case 'v': list_vim_vars(first); break;
				case 's': list_script_vars(first); break;
				case 'l': list_func_vars(first); break;
				default:
					  EMSG2(_("E738: Can't list variables for %s"), name);
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
			    list_one_var_a((char_u *)"",
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
ex_let_one(arg, tv, copy, endchars, op)
    char_u	*arg;		/* points to variable name */
    typval_T	*tv;		/* value to assign to variable */
    int		copy;		/* copy value from "tv" */
    char_u	*endchars;	/* valid chars after variable name  or NULL */
    char_u	*op;		/* "+", "-", "."  or NULL*/
{
    int		c1;
    char_u	*name;
    char_u	*p;
    char_u	*arg_end = NULL;
    int		len;
    int		opt_flags;
    char_u	*tofree = NULL;

    /*
     * ":let $VAR = expr": Set environment variable.
     */
    if (*arg == '$')
    {
	/* Find the end of the name. */
	++arg;
	name = arg;
	len = get_env_len(&arg);
	if (len == 0)
	    EMSG2(_(e_invarg2), name - 1);
	else
	{
	    if (op != NULL && (*op == '+' || *op == '-'))
		EMSG2(_(e_letwrong), op);
	    else if (endchars != NULL
			     && vim_strchr(endchars, *skipwhite(arg)) == NULL)
		EMSG(_(e_letunexp));
	    else
	    {
		c1 = name[len];
		name[len] = NUL;
		p = get_tv_string_chk(tv);
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

    /*
     * ":let &option = expr": Set option value.
     * ":let &l:option = expr": Set local option value.
     * ":let &g:option = expr": Set global option value.
     */
    else if (*arg == '&')
    {
	/* Find the end of the name. */
	p = find_option_end(&arg, &opt_flags);
	if (p == NULL || (endchars != NULL
			      && vim_strchr(endchars, *skipwhite(p)) == NULL))
	    EMSG(_(e_letunexp));
	else
	{
	    long	n;
	    int		opt_type;
	    long	numval;
	    char_u	*stringval = NULL;
	    char_u	*s;

	    c1 = *p;
	    *p = NUL;

	    n = get_tv_number(tv);
	    s = get_tv_string_chk(tv);	    /* != NULL if number or string */
	    if (s != NULL && op != NULL && *op != '=')
	    {
		opt_type = get_option_value(arg, &numval,
						       &stringval, opt_flags);
		if ((opt_type == 1 && *op == '.')
			|| (opt_type == 0 && *op != '.'))
		    EMSG2(_(e_letwrong), op);
		else
		{
		    if (opt_type == 1)  /* number */
		    {
			if (*op == '+')
			    n = numval + n;
			else
			    n = numval - n;
		    }
		    else if (opt_type == 0 && stringval != NULL) /* string */
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

    /*
     * ":let @r = expr": Set register contents.
     */
    else if (*arg == '@')
    {
	++arg;
	if (op != NULL && (*op == '+' || *op == '-'))
	    EMSG2(_(e_letwrong), op);
	else if (endchars != NULL
			 && vim_strchr(endchars, *skipwhite(arg + 1)) == NULL)
	    EMSG(_(e_letunexp));
	else
	{
	    char_u	*ptofree = NULL;
	    char_u	*s;

	    p = get_tv_string_chk(tv);
	    if (p != NULL && op != NULL && *op == '.')
	    {
		s = get_reg_contents(*arg == '@' ? '"' : *arg, TRUE, TRUE);
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

    /*
     * ":let var = expr": Set internal variable.
     * ":let {expr} = expr": Idem, name made with curly braces
     */
    else if (eval_isnamec1(*arg) || *arg == '{')
    {
	lval_T	lv;

	p = get_lval(arg, tv, &lv, FALSE, FALSE, FALSE, FNE_CHECK_START);
	if (p != NULL && lv.ll_name != NULL)
	{
	    if (endchars != NULL && vim_strchr(endchars, *skipwhite(p)) == NULL)
		EMSG(_(e_letunexp));
	    else
	    {
		set_var_lval(&lv, p, tv, copy, op);
		arg_end = p;
	    }
	}
	clear_lval(&lv);
    }

    else
	EMSG2(_(e_invarg2), arg);

    return arg_end;
}

/*
 * If "arg" is equal to "b:changedtick" give an error and return TRUE.
 */
    static int
check_changedtick(arg)
    char_u	*arg;
{
    if (STRNCMP(arg, "b:changedtick", 13) == 0 && !eval_isnamec(arg[13]))
    {
	EMSG2(_(e_readonlyvar), arg);
	return TRUE;
    }
    return FALSE;
}

/*
 * Get an lval: variable, Dict item or List item that can be assigned a value
 * to: "name", "na{me}", "name[expr]", "name[expr:expr]", "name[expr][expr]",
 * "name.key", "name.key[expr]" etc.
 * Indexing only works if "name" is an existing List or Dictionary.
 * "name" points to the start of the name.
 * If "rettv" is not NULL it points to the value to be assigned.
 * "unlet" is TRUE for ":unlet": slightly different behavior when something is
 * wrong; must end in space or cmd separator.
 *
 * Returns a pointer to just after the name, including indexes.
 * When an evaluation error occurs "lp->ll_name" is NULL;
 * Returns NULL for a parsing error.  Still need to free items in "lp"!
 */
    static char_u *
get_lval(name, rettv, lp, unlet, skip, quiet, fne_flags)
    char_u	*name;
    typval_T	*rettv;
    lval_T	*lp;
    int		unlet;
    int		skip;
    int		quiet;	    /* don't give error messages */
    int		fne_flags;  /* flags for find_name_end() */
{
    char_u	*p;
    char_u	*expr_start, *expr_end;
    int		cc;
    dictitem_T	*v;
    typval_T	var1;
    typval_T	var2;
    int		empty1 = FALSE;
    listitem_T	*ni;
    char_u	*key = NULL;
    int		len;
    hashtab_T	*ht;

    /* Clear everything in "lp". */
    vim_memset(lp, 0, sizeof(lval_T));

    if (skip)
    {
	/* When skipping just find the end of the name. */
	lp->ll_name = name;
	return find_name_end(name, NULL, NULL, FNE_INCL_BR | fne_flags);
    }

    /* Find the end of the name. */
    p = find_name_end(name, &expr_start, &expr_end, fne_flags);
    if (expr_start != NULL)
    {
	/* Don't expand the name when we already know there is an error. */
	if (unlet && !vim_iswhite(*p) && !ends_excmd(*p)
						    && *p != '[' && *p != '.')
	{
	    EMSG(_(e_trailing));
	    return NULL;
	}

	lp->ll_exp_name = make_expanded_name(name, expr_start, expr_end, p);
	if (lp->ll_exp_name == NULL)
	{
	    /* Report an invalid expression in braces, unless the
	     * expression evaluation has been cancelled due to an
	     * aborting error, an interrupt, or an exception. */
	    if (!aborting() && !quiet)
	    {
		emsg_severe = TRUE;
		EMSG2(_(e_invarg2), name);
		return NULL;
	    }
	}
	lp->ll_name = lp->ll_exp_name;
    }
    else
	lp->ll_name = name;

    /* Without [idx] or .key we are done. */
    if ((*p != '[' && *p != '.') || lp->ll_name == NULL)
	return p;

    cc = *p;
    *p = NUL;
    v = find_var(lp->ll_name, &ht);
    if (v == NULL && !quiet)
	EMSG2(_(e_undefvar), lp->ll_name);
    *p = cc;
    if (v == NULL)
	return NULL;

    /*
     * Loop until no more [idx] or .key is following.
     */
    lp->ll_tv = &v->di_tv;
    while (*p == '[' || (*p == '.' && lp->ll_tv->v_type == VAR_DICT))
    {
	if (!(lp->ll_tv->v_type == VAR_LIST && lp->ll_tv->vval.v_list != NULL)
		&& !(lp->ll_tv->v_type == VAR_DICT
					   && lp->ll_tv->vval.v_dict != NULL))
	{
	    if (!quiet)
		EMSG(_("E689: Can only index a List or Dictionary"));
	    return NULL;
	}
	if (lp->ll_range)
	{
	    if (!quiet)
		EMSG(_("E708: [:] must come last"));
	    return NULL;
	}

	len = -1;
	if (*p == '.')
	{
	    key = p + 1;
	    for (len = 0; ASCII_ISALNUM(key[len]) || key[len] == '_'; ++len)
		;
	    if (len == 0)
	    {
		if (!quiet)
		    EMSG(_(e_emptykey));
		return NULL;
	    }
	    p = key + len;
	}
	else
	{
	    /* Get the index [expr] or the first index [expr: ]. */
	    p = skipwhite(p + 1);
	    if (*p == ':')
		empty1 = TRUE;
	    else
	    {
		empty1 = FALSE;
		if (eval1(&p, &var1, TRUE) == FAIL)	/* recursive! */
		    return NULL;
		if (get_tv_string_chk(&var1) == NULL)
		{
		    /* not a number or string */
		    clear_tv(&var1);
		    return NULL;
		}
	    }

	    /* Optionally get the second index [ :expr]. */
	    if (*p == ':')
	    {
		if (lp->ll_tv->v_type == VAR_DICT)
		{
		    if (!quiet)
			EMSG(_(e_dictrange));
		    if (!empty1)
			clear_tv(&var1);
		    return NULL;
		}
		if (rettv != NULL && (rettv->v_type != VAR_LIST
					       || rettv->vval.v_list == NULL))
		{
		    if (!quiet)
			EMSG(_("E709: [:] requires a List value"));
		    if (!empty1)
			clear_tv(&var1);
		    return NULL;
		}
		p = skipwhite(p + 1);
		if (*p == ']')
		    lp->ll_empty2 = TRUE;
		else
		{
		    lp->ll_empty2 = FALSE;
		    if (eval1(&p, &var2, TRUE) == FAIL)	/* recursive! */
		    {
			if (!empty1)
			    clear_tv(&var1);
			return NULL;
		    }
		    if (get_tv_string_chk(&var2) == NULL)
		    {
			/* not a number or string */
			if (!empty1)
			    clear_tv(&var1);
			clear_tv(&var2);
			return NULL;
		    }
		}
		lp->ll_range = TRUE;
	    }
	    else
		lp->ll_range = FALSE;

	    if (*p != ']')
	    {
		if (!quiet)
		    EMSG(_(e_missbrac));
		if (!empty1)
		    clear_tv(&var1);
		if (lp->ll_range && !lp->ll_empty2)
		    clear_tv(&var2);
		return NULL;
	    }

	    /* Skip to past ']'. */
	    ++p;
	}

	if (lp->ll_tv->v_type == VAR_DICT)
	{
	    if (len == -1)
	    {
		/* "[key]": get key from "var1" */
		key = get_tv_string(&var1);	/* is number or string */
		if (*key == NUL)
		{
		    if (!quiet)
			EMSG(_(e_emptykey));
		    clear_tv(&var1);
		    return NULL;
		}
	    }
	    lp->ll_list = NULL;
	    lp->ll_dict = lp->ll_tv->vval.v_dict;
	    lp->ll_di = dict_find(lp->ll_dict, key, len);
	    if (lp->ll_di == NULL)
	    {
		/* Key does not exist in dict: may need to add it. */
		if (*p == '[' || *p == '.' || unlet)
		{
		    if (!quiet)
			EMSG2(_(e_dictkey), key);
		    if (len == -1)
			clear_tv(&var1);
		    return NULL;
		}
		if (len == -1)
		    lp->ll_newkey = vim_strsave(key);
		else
		    lp->ll_newkey = vim_strnsave(key, len);
		if (len == -1)
		    clear_tv(&var1);
		if (lp->ll_newkey == NULL)
		    p = NULL;
		break;
	    }
	    if (len == -1)
		clear_tv(&var1);
	    lp->ll_tv = &lp->ll_di->di_tv;
	}
	else
	{
	    /*
	     * Get the number and item for the only or first index of the List.
	     */
	    if (empty1)
		lp->ll_n1 = 0;
	    else
	    {
		lp->ll_n1 = get_tv_number(&var1);   /* is number or string */
		clear_tv(&var1);
	    }
	    lp->ll_dict = NULL;
	    lp->ll_list = lp->ll_tv->vval.v_list;
	    lp->ll_li = list_find(lp->ll_list, lp->ll_n1);
	    if (lp->ll_li == NULL)
	    {
		if (lp->ll_n1 < 0)
		{
		    lp->ll_n1 = 0;
		    lp->ll_li = list_find(lp->ll_list, lp->ll_n1);
		}
	    }
	    if (lp->ll_li == NULL)
	    {
		if (lp->ll_range && !lp->ll_empty2)
		    clear_tv(&var2);
		return NULL;
	    }

	    /*
	     * May need to find the item or absolute index for the second
	     * index of a range.
	     * When no index given: "lp->ll_empty2" is TRUE.
	     * Otherwise "lp->ll_n2" is set to the second index.
	     */
	    if (lp->ll_range && !lp->ll_empty2)
	    {
		lp->ll_n2 = get_tv_number(&var2);   /* is number or string */
		clear_tv(&var2);
		if (lp->ll_n2 < 0)
		{
		    ni = list_find(lp->ll_list, lp->ll_n2);
		    if (ni == NULL)
			return NULL;
		    lp->ll_n2 = list_idx_of_item(lp->ll_list, ni);
		}

		/* Check that lp->ll_n2 isn't before lp->ll_n1. */
		if (lp->ll_n1 < 0)
		    lp->ll_n1 = list_idx_of_item(lp->ll_list, lp->ll_li);
		if (lp->ll_n2 < lp->ll_n1)
		    return NULL;
	    }

	    lp->ll_tv = &lp->ll_li->li_tv;
	}
    }

    return p;
}

/*
 * Clear lval "lp" that was filled by get_lval().
 */
    static void
clear_lval(lp)
    lval_T	*lp;
{
    vim_free(lp->ll_exp_name);
    vim_free(lp->ll_newkey);
}

/*
 * Set a variable that was parsed by get_lval() to "rettv".
 * "endp" points to just after the parsed name.
 * "op" is NULL, "+" for "+=", "-" for "-=", "." for ".=" or "=" for "=".
 */
    static void
set_var_lval(lp, endp, rettv, copy, op)
    lval_T	*lp;
    char_u	*endp;
    typval_T	*rettv;
    int		copy;
    char_u	*op;
{
    int		cc;
    listitem_T	*ri;
    dictitem_T	*di;

    if (lp->ll_tv == NULL)
    {
	if (!check_changedtick(lp->ll_name))
	{
	    cc = *endp;
	    *endp = NUL;
	    if (op != NULL && *op != '=')
	    {
		typval_T tv;

		/* handle +=, -= and .= */
		if (get_var_tv(lp->ll_name, (int)STRLEN(lp->ll_name),
							     &tv, TRUE) == OK)
		{
		    if (tv_op(&tv, rettv, op) == OK)
			set_var(lp->ll_name, &tv, FALSE);
		    clear_tv(&tv);
		}
	    }
	    else
		set_var(lp->ll_name, rettv, copy);
	    *endp = cc;
	}
    }
    else if (tv_check_lock(lp->ll_newkey == NULL
		? lp->ll_tv->v_lock
		: lp->ll_tv->vval.v_dict->dv_lock, lp->ll_name))
	;
    else if (lp->ll_range)
    {
	/*
	 * Assign the List values to the list items.
	 */
	for (ri = rettv->vval.v_list->lv_first; ri != NULL; )
	{
	    if (op != NULL && *op != '=')
		tv_op(&lp->ll_li->li_tv, &ri->li_tv, op);
	    else
	    {
		clear_tv(&lp->ll_li->li_tv);
		copy_tv(&ri->li_tv, &lp->ll_li->li_tv);
	    }
	    ri = ri->li_next;
	    if (ri == NULL || (!lp->ll_empty2 && lp->ll_n2 == lp->ll_n1))
		break;
	    if (lp->ll_li->li_next == NULL)
	    {
		/* Need to add an empty item. */
		if (list_append_number(lp->ll_list, 0) == FAIL)
		{
		    ri = NULL;
		    break;
		}
	    }
	    lp->ll_li = lp->ll_li->li_next;
	    ++lp->ll_n1;
	}
	if (ri != NULL)
	    EMSG(_("E710: List value has more items than target"));
	else if (lp->ll_empty2
		? (lp->ll_li != NULL && lp->ll_li->li_next != NULL)
		: lp->ll_n1 != lp->ll_n2)
	    EMSG(_("E711: List value has not enough items"));
    }
    else
    {
	/*
	 * Assign to a List or Dictionary item.
	 */
	if (lp->ll_newkey != NULL)
	{
	    if (op != NULL && *op != '=')
	    {
		EMSG2(_(e_letwrong), op);
		return;
	    }

	    /* Need to add an item to the Dictionary. */
	    di = dictitem_alloc(lp->ll_newkey);
	    if (di == NULL)
		return;
	    if (dict_add(lp->ll_tv->vval.v_dict, di) == FAIL)
	    {
		vim_free(di);
		return;
	    }
	    lp->ll_tv = &di->di_tv;
	}
	else if (op != NULL && *op != '=')
	{
	    tv_op(lp->ll_tv, rettv, op);
	    return;
	}
	else
	    clear_tv(lp->ll_tv);

	/*
	 * Assign the value to the variable or list item.
	 */
	if (copy)
	    copy_tv(rettv, lp->ll_tv);
	else
	{
	    *lp->ll_tv = *rettv;
	    lp->ll_tv->v_lock = 0;
	    init_tv(rettv);
	}
    }
}

/*
 * Handle "tv1 += tv2", "tv1 -= tv2" and "tv1 .= tv2"
 * Returns OK or FAIL.
 */
    static int
tv_op(tv1, tv2, op)
    typval_T *tv1;
    typval_T *tv2;
    char_u  *op;
{
    long	n;
    char_u	numbuf[NUMBUFLEN];
    char_u	*s;

    /* Can't do anything with a Funcref or a Dict on the right. */
    if (tv2->v_type != VAR_FUNC && tv2->v_type != VAR_DICT)
    {
	switch (tv1->v_type)
	{
	    case VAR_DICT:
	    case VAR_FUNC:
		break;

	    case VAR_LIST:
		if (*op != '+' || tv2->v_type != VAR_LIST)
		    break;
		/* List += List */
		if (tv1->vval.v_list != NULL && tv2->vval.v_list != NULL)
		    list_extend(tv1->vval.v_list, tv2->vval.v_list, NULL);
		return OK;

	    case VAR_NUMBER:
	    case VAR_STRING:
		if (tv2->v_type == VAR_LIST)
		    break;
		if (*op == '+' || *op == '-')
		{
		    /* nr += nr  or  nr -= nr*/
		    n = get_tv_number(tv1);
#ifdef FEAT_FLOAT
		    if (tv2->v_type == VAR_FLOAT)
		    {
			float_T f = n;

			if (*op == '+')
			    f += tv2->vval.v_float;
			else
			    f -= tv2->vval.v_float;
			clear_tv(tv1);
			tv1->v_type = VAR_FLOAT;
			tv1->vval.v_float = f;
		    }
		    else
#endif
		    {
			if (*op == '+')
			    n += get_tv_number(tv2);
			else
			    n -= get_tv_number(tv2);
			clear_tv(tv1);
			tv1->v_type = VAR_NUMBER;
			tv1->vval.v_number = n;
		    }
		}
		else
		{
		    if (tv2->v_type == VAR_FLOAT)
			break;

		    /* str .= str */
		    s = get_tv_string(tv1);
		    s = concat_str(s, get_tv_string_buf(tv2, numbuf));
		    clear_tv(tv1);
		    tv1->v_type = VAR_STRING;
		    tv1->vval.v_string = s;
		}
		return OK;

#ifdef FEAT_FLOAT
	    case VAR_FLOAT:
		{
		    float_T f;

		    if (*op == '.' || (tv2->v_type != VAR_FLOAT
				    && tv2->v_type != VAR_NUMBER
				    && tv2->v_type != VAR_STRING))
			break;
		    if (tv2->v_type == VAR_FLOAT)
			f = tv2->vval.v_float;
		    else
			f = get_tv_number(tv2);
		    if (*op == '+')
			tv1->vval.v_float += f;
		    else
			tv1->vval.v_float -= f;
		}
		return OK;
#endif
	}
    }

    EMSG2(_(e_letwrong), op);
    return FAIL;
}

/*
 * Add a watcher to a list.
 */
    static void
list_add_watch(l, lw)
    list_T	*l;
    listwatch_T	*lw;
{
    lw->lw_next = l->lv_watch;
    l->lv_watch = lw;
}

/*
 * Remove a watcher from a list.
 * No warning when it isn't found...
 */
    static void
list_rem_watch(l, lwrem)
    list_T	*l;
    listwatch_T	*lwrem;
{
    listwatch_T	*lw, **lwp;

    lwp = &l->lv_watch;
    for (lw = l->lv_watch; lw != NULL; lw = lw->lw_next)
    {
	if (lw == lwrem)
	{
	    *lwp = lw->lw_next;
	    break;
	}
	lwp = &lw->lw_next;
    }
}

/*
 * Just before removing an item from a list: advance watchers to the next
 * item.
 */
    static void
list_fix_watch(l, item)
    list_T	*l;
    listitem_T	*item;
{
    listwatch_T	*lw;

    for (lw = l->lv_watch; lw != NULL; lw = lw->lw_next)
	if (lw->lw_item == item)
	    lw->lw_item = item->li_next;
}

/*
 * Evaluate the expression used in a ":for var in expr" command.
 * "arg" points to "var".
 * Set "*errp" to TRUE for an error, FALSE otherwise;
 * Return a pointer that holds the info.  Null when there is an error.
 */
    void *
eval_for_line(arg, errp, nextcmdp, skip)
    char_u	*arg;
    int		*errp;
    char_u	**nextcmdp;
    int		skip;
{
    forinfo_T	*fi;
    char_u	*expr;
    typval_T	tv;
    list_T	*l;

    *errp = TRUE;	/* default: there is an error */

    fi = (forinfo_T *)alloc_clear(sizeof(forinfo_T));
    if (fi == NULL)
	return NULL;

    expr = skip_var_list(arg, &fi->fi_varcount, &fi->fi_semicolon);
    if (expr == NULL)
	return fi;

    expr = skipwhite(expr);
    if (expr[0] != 'i' || expr[1] != 'n' || !vim_iswhite(expr[2]))
    {
	EMSG(_("E690: Missing \"in\" after :for"));
	return fi;
    }

    if (skip)
	++emsg_skip;
    if (eval0(skipwhite(expr + 2), &tv, nextcmdp, !skip) == OK)
    {
	*errp = FALSE;
	if (!skip)
	{
	    l = tv.vval.v_list;
	    if (tv.v_type != VAR_LIST || l == NULL)
	    {
		EMSG(_(e_listreq));
		clear_tv(&tv);
	    }
	    else
	    {
		/* No need to increment the refcount, it's already set for the
		 * list being used in "tv". */
		fi->fi_list = l;
		list_add_watch(l, &fi->fi_lw);
		fi->fi_lw.lw_item = l->lv_first;
	    }
	}
    }
    if (skip)
	--emsg_skip;

    return fi;
}

/*
 * Use the first item in a ":for" list.  Advance to the next.
 * Assign the values to the variable (list).  "arg" points to the first one.
 * Return TRUE when a valid item was found, FALSE when at end of list or
 * something wrong.
 */
    int
next_for_item(fi_void, arg)
    void	*fi_void;
    char_u	*arg;
{
    forinfo_T    *fi = (forinfo_T *)fi_void;
    int		result;
    listitem_T	*item;

    item = fi->fi_lw.lw_item;
    if (item == NULL)
	result = FALSE;
    else
    {
	fi->fi_lw.lw_item = item->li_next;
	result = (ex_let_vars(arg, &item->li_tv, TRUE,
			      fi->fi_semicolon, fi->fi_varcount, NULL) == OK);
    }
    return result;
}

/*
 * Free the structure used to store info used by ":for".
 */
    void
free_for_info(fi_void)
    void *fi_void;
{
    forinfo_T    *fi = (forinfo_T *)fi_void;

    if (fi != NULL && fi->fi_list != NULL)
    {
	list_rem_watch(fi->fi_list, &fi->fi_lw);
	list_unref(fi->fi_list);
    }
    vim_free(fi);
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
    char_u	*p;

    if (cmdidx == CMD_let)
    {
	xp->xp_context = EXPAND_USER_VARS;
	if (vim_strpbrk(arg, (char_u *)"\"'+-*/%.=!?~|&$([<>,#") == NULL)
	{
	    /* ":let var1 var2 ...": find last space. */
	    for (p = arg + STRLEN(arg); p >= arg; )
	    {
		xp->xp_pattern = p;
		mb_ptr_back(arg, p);
		if (vim_iswhite(*p))
		    break;
	    }
	    return;
	}
    }
    else
	xp->xp_context = cmdidx == CMD_call ? EXPAND_FUNCTIONS
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
	    {
		xp->xp_context = EXPAND_SETTINGS;
		if ((c == 'l' || c == 'g') && xp->xp_pattern[2] == ':')
		    xp->xp_pattern += 2;

	    }
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
		/* Trick: '' is like stopping and starting a literal string. */
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
	    /* Doesn't look like something valid, expand as an expression
	     * anyway. */
	    xp->xp_context = EXPAND_EXPRESSION;
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
    char_u	*name;
    char_u	*tofree;
    int		len;
    typval_T	rettv;
    linenr_T	lnum;
    int		doesrange;
    int		failed = FALSE;
    funcdict_T	fudi;

    tofree = trans_function_name(&arg, eap->skip, TFN_INT, &fudi);
    if (fudi.fd_newkey != NULL)
    {
	/* Still need to give an error message for missing key. */
	EMSG2(_(e_dictkey), fudi.fd_newkey);
	vim_free(fudi.fd_newkey);
    }
    if (tofree == NULL)
	return;

    /* Increase refcount on dictionary, it could get deleted when evaluating
     * the arguments. */
    if (fudi.fd_dict != NULL)
	++fudi.fd_dict->dv_refcount;

    /* If it is the name of a variable of type VAR_FUNC use its contents. */
    len = (int)STRLEN(tofree);
    name = deref_func_name(tofree, &len);

    /* Skip white space to allow ":call func ()".  Not good, but required for
     * backward compatibility. */
    startarg = skipwhite(arg);
    rettv.v_type = VAR_UNKNOWN;	/* clear_tv() uses this */

    if (*startarg != '(')
    {
	EMSG2(_("E107: Missing parentheses: %s"), eap->arg);
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
	if (get_func_tv(name, (int)STRLEN(name), &rettv, &arg,
		    eap->line1, eap->line2, &doesrange,
					    !eap->skip, fudi.fd_dict) == FAIL)
	{
	    failed = TRUE;
	    break;
	}

	/* Handle a function returning a Funcref, Dictionary or List. */
	if (handle_subscript(&arg, &rettv, !eap->skip, TRUE) == FAIL)
	{
	    failed = TRUE;
	    break;
	}

	clear_tv(&rettv);
	if (doesrange || eap->skip)
	    break;

	/* Stop when immediately aborting on error, or when an interrupt
	 * occurred or an exception was thrown but not caught.
	 * get_func_tv() returned OK, so that the check for trailing
	 * characters below is executed. */
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
    dict_unref(fudi.fd_dict);
    vim_free(tofree);
}

/*
 * ":unlet[!] var1 ... " command.
 */
    void
ex_unlet(eap)
    exarg_T	*eap;
{
    ex_unletlock(eap, eap->arg, 0);
}

/*
 * ":lockvar" and ":unlockvar" commands
 */
    void
ex_lockvar(eap)
    exarg_T	*eap;
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
ex_unletlock(eap, argstart, deep)
    exarg_T	*eap;
    char_u	*argstart;
    int		deep;
{
    char_u	*arg = argstart;
    char_u	*name_end;
    int		error = FALSE;
    lval_T	lv;

    do
    {
	/* Parse the name and find the end. */
	name_end = get_lval(arg, NULL, &lv, TRUE, eap->skip || error, FALSE,
							     FNE_CHECK_START);
	if (lv.ll_name == NULL)
	    error = TRUE;	    /* error but continue parsing */
	if (name_end == NULL || (!vim_iswhite(*name_end)
						   && !ends_excmd(*name_end)))
	{
	    if (name_end != NULL)
	    {
		emsg_severe = TRUE;
		EMSG(_(e_trailing));
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
do_unlet_var(lp, name_end, forceit)
    lval_T	*lp;
    char_u	*name_end;
    int		forceit;
{
    int		ret = OK;
    int		cc;

    if (lp->ll_tv == NULL)
    {
	cc = *name_end;
	*name_end = NUL;

	/* Normal name or expanded name. */
	if (check_changedtick(lp->ll_name))
	    ret = FAIL;
	else if (do_unlet(lp->ll_name, forceit) == FAIL)
	    ret = FAIL;
	*name_end = cc;
    }
    else if (tv_check_lock(lp->ll_tv->v_lock, lp->ll_name))
	return FAIL;
    else if (lp->ll_range)
    {
	listitem_T    *li;

	/* Delete a range of List items. */
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
	    /* unlet a List item. */
	    listitem_remove(lp->ll_list, lp->ll_li);
	else
	    /* unlet a Dictionary item. */
	    dictitem_remove(lp->ll_dict, lp->ll_di);
    }

    return ret;
}

/*
 * "unlet" a variable.  Return OK if it existed, FAIL if not.
 * When "forceit" is TRUE don't complain if the variable doesn't exist.
 */
    int
do_unlet(name, forceit)
    char_u	*name;
    int		forceit;
{
    hashtab_T	*ht;
    hashitem_T	*hi;
    char_u	*varname;
    dictitem_T	*di;

    ht = find_var_ht(name, &varname);
    if (ht != NULL && *varname != NUL)
    {
	hi = hash_find(ht, varname);
	if (!HASHITEM_EMPTY(hi))
	{
	    di = HI2DI(hi);
	    if (var_check_fixed(di->di_flags, name)
		    || var_check_ro(di->di_flags, name))
		return FAIL;
	    delete_var(ht, hi);
	    return OK;
	}
    }
    if (forceit)
	return OK;
    EMSG2(_("E108: No such variable: \"%s\""), name);
    return FAIL;
}

/*
 * Lock or unlock variable indicated by "lp".
 * "deep" is the levels to go (-1 for unlimited);
 * "lock" is TRUE for ":lockvar", FALSE for ":unlockvar".
 */
    static int
do_lock_var(lp, name_end, deep, lock)
    lval_T	*lp;
    char_u	*name_end;
    int		deep;
    int		lock;
{
    int		ret = OK;
    int		cc;
    dictitem_T	*di;

    if (deep == 0)	/* nothing to do */
	return OK;

    if (lp->ll_tv == NULL)
    {
	cc = *name_end;
	*name_end = NUL;

	/* Normal name or expanded name. */
	if (check_changedtick(lp->ll_name))
	    ret = FAIL;
	else
	{
	    di = find_var(lp->ll_name, NULL);
	    if (di == NULL)
		ret = FAIL;
	    else
	    {
		if (lock)
		    di->di_flags |= DI_FLAGS_LOCK;
		else
		    di->di_flags &= ~DI_FLAGS_LOCK;
		item_lock(&di->di_tv, deep, lock);
	    }
	}
	*name_end = cc;
    }
    else if (lp->ll_range)
    {
	listitem_T    *li = lp->ll_li;

	/* (un)lock a range of List items. */
	while (li != NULL && (lp->ll_empty2 || lp->ll_n2 >= lp->ll_n1))
	{
	    item_lock(&li->li_tv, deep, lock);
	    li = li->li_next;
	    ++lp->ll_n1;
	}
    }
    else if (lp->ll_list != NULL)
	/* (un)lock a List item. */
	item_lock(&lp->ll_li->li_tv, deep, lock);
    else
	/* un(lock) a Dictionary item. */
	item_lock(&lp->ll_di->di_tv, deep, lock);

    return ret;
}

/*
 * Lock or unlock an item.  "deep" is nr of levels to go.
 */
    static void
item_lock(tv, deep, lock)
    typval_T	*tv;
    int		deep;
    int		lock;
{
    static int	recurse = 0;
    list_T	*l;
    listitem_T	*li;
    dict_T	*d;
    hashitem_T	*hi;
    int		todo;

    if (recurse >= DICT_MAXNEST)
    {
	EMSG(_("E743: variable nested too deep for (un)lock"));
	return;
    }
    if (deep == 0)
	return;
    ++recurse;

    /* lock/unlock the item itself */
    if (lock)
	tv->v_lock |= VAR_LOCKED;
    else
	tv->v_lock &= ~VAR_LOCKED;

    switch (tv->v_type)
    {
	case VAR_LIST:
	    if ((l = tv->vval.v_list) != NULL)
	    {
		if (lock)
		    l->lv_lock |= VAR_LOCKED;
		else
		    l->lv_lock &= ~VAR_LOCKED;
		if (deep < 0 || deep > 1)
		    /* recursive: lock/unlock the items the List contains */
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
		    /* recursive: lock/unlock the items the List contains */
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
 * Return TRUE if typeval "tv" is locked: Either that value is locked itself
 * or it refers to a List or Dictionary that is locked.
 */
    static int
tv_islocked(tv)
    typval_T	*tv;
{
    return (tv->v_lock & VAR_LOCKED)
	|| (tv->v_type == VAR_LIST
		&& tv->vval.v_list != NULL
		&& (tv->vval.v_list->lv_lock & VAR_LOCKED))
	|| (tv->v_type == VAR_DICT
		&& tv->vval.v_dict != NULL
		&& (tv->vval.v_dict->dv_lock & VAR_LOCKED));
}

#if (defined(FEAT_MENU) && defined(FEAT_MULTI_LANG)) || defined(PROTO)
/*
 * Delete all "menutrans_" variables.
 */
    void
del_menutrans_vars()
{
    hashitem_T	*hi;
    int		todo;

    hash_lock(&globvarht);
    todo = (int)globvarht.ht_used;
    for (hi = globvarht.ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    if (STRNCMP(HI2DI(hi)->di_key, "menutrans_", 10) == 0)
		delete_var(&globvarht, hi);
	}
    }
    hash_unlock(&globvarht);
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
    static long_u	gdone;
    static long_u	bdone;
    static long_u	wdone;
#ifdef FEAT_WINDOWS
    static long_u	tdone;
#endif
    static int		vidx;
    static hashitem_T	*hi;
    hashtab_T		*ht;

    if (idx == 0)
    {
	gdone = bdone = wdone = vidx = 0;
#ifdef FEAT_WINDOWS
	tdone = 0;
#endif
    }

    /* Global variables */
    if (gdone < globvarht.ht_used)
    {
	if (gdone++ == 0)
	    hi = globvarht.ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	if (STRNCMP("g:", xp->xp_pattern, 2) == 0)
	    return cat_prefix_varname('g', hi->hi_key);
	return hi->hi_key;
    }

    /* b: variables */
    ht = &curbuf->b_vars.dv_hashtab;
    if (bdone < ht->ht_used)
    {
	if (bdone++ == 0)
	    hi = ht->ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	return cat_prefix_varname('b', hi->hi_key);
    }
    if (bdone == ht->ht_used)
    {
	++bdone;
	return (char_u *)"b:changedtick";
    }

    /* w: variables */
    ht = &curwin->w_vars.dv_hashtab;
    if (wdone < ht->ht_used)
    {
	if (wdone++ == 0)
	    hi = ht->ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	return cat_prefix_varname('w', hi->hi_key);
    }

#ifdef FEAT_WINDOWS
    /* t: variables */
    ht = &curtab->tp_vars.dv_hashtab;
    if (tdone < ht->ht_used)
    {
	if (tdone++ == 0)
	    hi = ht->ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	return cat_prefix_varname('t', hi->hi_key);
    }
#endif

    /* v: variables */
    if (vidx < VV_LEN)
	return cat_prefix_varname('v', (char_u *)vimvars[vidx++].vv_name);

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
 * executed.  The function may return OK, but the rettv will be of type
 * VAR_UNKNOWN.  The function still returns FAIL for a syntax error.
 */

/*
 * Handle zero level expression.
 * This calls eval1() and handles error message and nextcmd.
 * Put the result in "rettv" when returning OK and "evaluate" is TRUE.
 * Note: "rettv.v_lock" is not set.
 * Return OK or FAIL.
 */
    static int
eval0(arg, rettv, nextcmd, evaluate)
    char_u	*arg;
    typval_T	*rettv;
    char_u	**nextcmd;
    int		evaluate;
{
    int		ret;
    char_u	*p;

    p = skipwhite(arg);
    ret = eval1(&p, rettv, evaluate);
    if (ret == FAIL || !ends_excmd(*p))
    {
	if (ret != FAIL)
	    clear_tv(rettv);
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
 *	expr2 ? expr1 : expr1
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Note: "rettv.v_lock" is not set.
 *
 * Return OK or FAIL.
 */
    static int
eval1(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    int		result;
    typval_T	var2;

    /*
     * Get the first variable.
     */
    if (eval2(arg, rettv, evaluate) == FAIL)
	return FAIL;

    if ((*arg)[0] == '?')
    {
	result = FALSE;
	if (evaluate)
	{
	    int		error = FALSE;

	    if (get_tv_number_chk(rettv, &error) != 0)
		result = TRUE;
	    clear_tv(rettv);
	    if (error)
		return FAIL;
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval1(arg, rettv, evaluate && result) == FAIL) /* recursive! */
	    return FAIL;

	/*
	 * Check for the ":".
	 */
	if ((*arg)[0] != ':')
	{
	    EMSG(_("E109: Missing ':' after '?'"));
	    if (evaluate && result)
		clear_tv(rettv);
	    return FAIL;
	}

	/*
	 * Get the third variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval1(arg, &var2, evaluate && !result) == FAIL) /* recursive! */
	{
	    if (evaluate && result)
		clear_tv(rettv);
	    return FAIL;
	}
	if (evaluate && !result)
	    *rettv = var2;
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
eval2(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    typval_T	var2;
    long	result;
    int		first;
    int		error = FALSE;

    /*
     * Get the first variable.
     */
    if (eval3(arg, rettv, evaluate) == FAIL)
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
	    if (get_tv_number_chk(rettv, &error) != 0)
		result = TRUE;
	    clear_tv(rettv);
	    if (error)
		return FAIL;
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
	    if (get_tv_number_chk(&var2, &error) != 0)
		result = TRUE;
	    clear_tv(&var2);
	    if (error)
		return FAIL;
	}
	if (evaluate)
	{
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = result;
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
eval3(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    typval_T	var2;
    long	result;
    int		first;
    int		error = FALSE;

    /*
     * Get the first variable.
     */
    if (eval4(arg, rettv, evaluate) == FAIL)
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
	    if (get_tv_number_chk(rettv, &error) == 0)
		result = FALSE;
	    clear_tv(rettv);
	    if (error)
		return FAIL;
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
	    if (get_tv_number_chk(&var2, &error) == 0)
		result = FALSE;
	    clear_tv(&var2);
	    if (error)
		return FAIL;
	}
	if (evaluate)
	{
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = result;
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
 *	var1 is var2
 *	var1 isnot var2
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval4(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    typval_T	var2;
    char_u	*p;
    int		i;
    exptype_T	type = TYPE_UNKNOWN;
    int		type_is = FALSE;    /* TRUE for "is" and "isnot" */
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
    if (eval5(arg, rettv, evaluate) == FAIL)
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
	case 'i':   if (p[1] == 's')
		    {
			if (p[2] == 'n' && p[3] == 'o' && p[4] == 't')
			    len = 5;
			if (!vim_isIDc(p[len]))
			{
			    type = len == 2 ? TYPE_EQUAL : TYPE_NEQUAL;
			    type_is = TRUE;
			}
		    }
		    break;
    }

    /*
     * If there is a comparative operator, use it.
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
	/* nothing appended: use 'ignorecase' */
	else
	    ic = p_ic;

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(p + len);
	if (eval5(arg, &var2, evaluate) == FAIL)
	{
	    clear_tv(rettv);
	    return FAIL;
	}

	if (evaluate)
	{
	    if (type_is && rettv->v_type != var2.v_type)
	    {
		/* For "is" a different type always means FALSE, for "notis"
		 * it means TRUE. */
		n1 = (type == TYPE_NEQUAL);
	    }
	    else if (rettv->v_type == VAR_LIST || var2.v_type == VAR_LIST)
	    {
		if (type_is)
		{
		    n1 = (rettv->v_type == var2.v_type
				   && rettv->vval.v_list == var2.vval.v_list);
		    if (type == TYPE_NEQUAL)
			n1 = !n1;
		}
		else if (rettv->v_type != var2.v_type
			|| (type != TYPE_EQUAL && type != TYPE_NEQUAL))
		{
		    if (rettv->v_type != var2.v_type)
			EMSG(_("E691: Can only compare List with List"));
		    else
			EMSG(_("E692: Invalid operation for Lists"));
		    clear_tv(rettv);
		    clear_tv(&var2);
		    return FAIL;
		}
		else
		{
		    /* Compare two Lists for being equal or unequal. */
		    n1 = list_equal(rettv->vval.v_list, var2.vval.v_list, ic);
		    if (type == TYPE_NEQUAL)
			n1 = !n1;
		}
	    }

	    else if (rettv->v_type == VAR_DICT || var2.v_type == VAR_DICT)
	    {
		if (type_is)
		{
		    n1 = (rettv->v_type == var2.v_type
				   && rettv->vval.v_dict == var2.vval.v_dict);
		    if (type == TYPE_NEQUAL)
			n1 = !n1;
		}
		else if (rettv->v_type != var2.v_type
			|| (type != TYPE_EQUAL && type != TYPE_NEQUAL))
		{
		    if (rettv->v_type != var2.v_type)
			EMSG(_("E735: Can only compare Dictionary with Dictionary"));
		    else
			EMSG(_("E736: Invalid operation for Dictionary"));
		    clear_tv(rettv);
		    clear_tv(&var2);
		    return FAIL;
		}
		else
		{
		    /* Compare two Dictionaries for being equal or unequal. */
		    n1 = dict_equal(rettv->vval.v_dict, var2.vval.v_dict, ic);
		    if (type == TYPE_NEQUAL)
			n1 = !n1;
		}
	    }

	    else if (rettv->v_type == VAR_FUNC || var2.v_type == VAR_FUNC)
	    {
		if (rettv->v_type != var2.v_type
			|| (type != TYPE_EQUAL && type != TYPE_NEQUAL))
		{
		    if (rettv->v_type != var2.v_type)
			EMSG(_("E693: Can only compare Funcref with Funcref"));
		    else
			EMSG(_("E694: Invalid operation for Funcrefs"));
		    clear_tv(rettv);
		    clear_tv(&var2);
		    return FAIL;
		}
		else
		{
		    /* Compare two Funcrefs for being equal or unequal. */
		    if (rettv->vval.v_string == NULL
						|| var2.vval.v_string == NULL)
			n1 = FALSE;
		    else
			n1 = STRCMP(rettv->vval.v_string,
						     var2.vval.v_string) == 0;
		    if (type == TYPE_NEQUAL)
			n1 = !n1;
		}
	    }

#ifdef FEAT_FLOAT
	    /*
	     * If one of the two variables is a float, compare as a float.
	     * When using "=~" or "!~", always compare as string.
	     */
	    else if ((rettv->v_type == VAR_FLOAT || var2.v_type == VAR_FLOAT)
		    && type != TYPE_MATCH && type != TYPE_NOMATCH)
	    {
		float_T f1, f2;

		if (rettv->v_type == VAR_FLOAT)
		    f1 = rettv->vval.v_float;
		else
		    f1 = get_tv_number(rettv);
		if (var2.v_type == VAR_FLOAT)
		    f2 = var2.vval.v_float;
		else
		    f2 = get_tv_number(&var2);
		n1 = FALSE;
		switch (type)
		{
		    case TYPE_EQUAL:    n1 = (f1 == f2); break;
		    case TYPE_NEQUAL:   n1 = (f1 != f2); break;
		    case TYPE_GREATER:  n1 = (f1 > f2); break;
		    case TYPE_GEQUAL:   n1 = (f1 >= f2); break;
		    case TYPE_SMALLER:  n1 = (f1 < f2); break;
		    case TYPE_SEQUAL:   n1 = (f1 <= f2); break;
		    case TYPE_UNKNOWN:
		    case TYPE_MATCH:
		    case TYPE_NOMATCH:  break;  /* avoid gcc warning */
		}
	    }
#endif

	    /*
	     * If one of the two variables is a number, compare as a number.
	     * When using "=~" or "!~", always compare as string.
	     */
	    else if ((rettv->v_type == VAR_NUMBER || var2.v_type == VAR_NUMBER)
		    && type != TYPE_MATCH && type != TYPE_NOMATCH)
	    {
		n1 = get_tv_number(rettv);
		n2 = get_tv_number(&var2);
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
		s1 = get_tv_string_buf(rettv, buf1);
		s2 = get_tv_string_buf(&var2, buf2);
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
	    clear_tv(rettv);
	    clear_tv(&var2);
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = n1;
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
eval5(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    typval_T	var2;
    typval_T	var3;
    int		op;
    long	n1, n2;
#ifdef FEAT_FLOAT
    float_T	f1 = 0, f2 = 0;
#endif
    char_u	*s1, *s2;
    char_u	buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    char_u	*p;

    /*
     * Get the first variable.
     */
    if (eval6(arg, rettv, evaluate, FALSE) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no '+', '-' or '.' is following.
     */
    for (;;)
    {
	op = **arg;
	if (op != '+' && op != '-' && op != '.')
	    break;

	if ((op != '+' || rettv->v_type != VAR_LIST)
#ifdef FEAT_FLOAT
		&& (op == '.' || rettv->v_type != VAR_FLOAT)
#endif
		)
	{
	    /* For "list + ...", an illegal use of the first operand as
	     * a number cannot be determined before evaluating the 2nd
	     * operand: if this is also a list, all is ok.
	     * For "something . ...", "something - ..." or "non-list + ...",
	     * we know that the first operand needs to be a string or number
	     * without evaluating the 2nd operand.  So check before to avoid
	     * side effects after an error. */
	    if (evaluate && get_tv_string_chk(rettv) == NULL)
	    {
		clear_tv(rettv);
		return FAIL;
	    }
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval6(arg, &var2, evaluate, op == '.') == FAIL)
	{
	    clear_tv(rettv);
	    return FAIL;
	}

	if (evaluate)
	{
	    /*
	     * Compute the result.
	     */
	    if (op == '.')
	    {
		s1 = get_tv_string_buf(rettv, buf1);	/* already checked */
		s2 = get_tv_string_buf_chk(&var2, buf2);
		if (s2 == NULL)		/* type error ? */
		{
		    clear_tv(rettv);
		    clear_tv(&var2);
		    return FAIL;
		}
		p = concat_str(s1, s2);
		clear_tv(rettv);
		rettv->v_type = VAR_STRING;
		rettv->vval.v_string = p;
	    }
	    else if (op == '+' && rettv->v_type == VAR_LIST
						   && var2.v_type == VAR_LIST)
	    {
		/* concatenate Lists */
		if (list_concat(rettv->vval.v_list, var2.vval.v_list,
							       &var3) == FAIL)
		{
		    clear_tv(rettv);
		    clear_tv(&var2);
		    return FAIL;
		}
		clear_tv(rettv);
		*rettv = var3;
	    }
	    else
	    {
		int	    error = FALSE;

#ifdef FEAT_FLOAT
		if (rettv->v_type == VAR_FLOAT)
		{
		    f1 = rettv->vval.v_float;
		    n1 = 0;
		}
		else
#endif
		{
		    n1 = get_tv_number_chk(rettv, &error);
		    if (error)
		    {
			/* This can only happen for "list + non-list".  For
			 * "non-list + ..." or "something - ...", we returned
			 * before evaluating the 2nd operand. */
			clear_tv(rettv);
			return FAIL;
		    }
#ifdef FEAT_FLOAT
		    if (var2.v_type == VAR_FLOAT)
			f1 = n1;
#endif
		}
#ifdef FEAT_FLOAT
		if (var2.v_type == VAR_FLOAT)
		{
		    f2 = var2.vval.v_float;
		    n2 = 0;
		}
		else
#endif
		{
		    n2 = get_tv_number_chk(&var2, &error);
		    if (error)
		    {
			clear_tv(rettv);
			clear_tv(&var2);
			return FAIL;
		    }
#ifdef FEAT_FLOAT
		    if (rettv->v_type == VAR_FLOAT)
			f2 = n2;
#endif
		}
		clear_tv(rettv);

#ifdef FEAT_FLOAT
		/* If there is a float on either side the result is a float. */
		if (rettv->v_type == VAR_FLOAT || var2.v_type == VAR_FLOAT)
		{
		    if (op == '+')
			f1 = f1 + f2;
		    else
			f1 = f1 - f2;
		    rettv->v_type = VAR_FLOAT;
		    rettv->vval.v_float = f1;
		}
		else
#endif
		{
		    if (op == '+')
			n1 = n1 + n2;
		    else
			n1 = n1 - n2;
		    rettv->v_type = VAR_NUMBER;
		    rettv->vval.v_number = n1;
		}
	    }
	    clear_tv(&var2);
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
eval6(arg, rettv, evaluate, want_string)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
    int		want_string;  /* after "." operator */
{
    typval_T	var2;
    int		op;
    long	n1, n2;
#ifdef FEAT_FLOAT
    int		use_float = FALSE;
    float_T	f1 = 0, f2;
#endif
    int		error = FALSE;

    /*
     * Get the first variable.
     */
    if (eval7(arg, rettv, evaluate, want_string) == FAIL)
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
#ifdef FEAT_FLOAT
	    if (rettv->v_type == VAR_FLOAT)
	    {
		f1 = rettv->vval.v_float;
		use_float = TRUE;
		n1 = 0;
	    }
	    else
#endif
		n1 = get_tv_number_chk(rettv, &error);
	    clear_tv(rettv);
	    if (error)
		return FAIL;
	}
	else
	    n1 = 0;

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval7(arg, &var2, evaluate, FALSE) == FAIL)
	    return FAIL;

	if (evaluate)
	{
#ifdef FEAT_FLOAT
	    if (var2.v_type == VAR_FLOAT)
	    {
		if (!use_float)
		{
		    f1 = n1;
		    use_float = TRUE;
		}
		f2 = var2.vval.v_float;
		n2 = 0;
	    }
	    else
#endif
	    {
		n2 = get_tv_number_chk(&var2, &error);
		clear_tv(&var2);
		if (error)
		    return FAIL;
#ifdef FEAT_FLOAT
		if (use_float)
		    f2 = n2;
#endif
	    }

	    /*
	     * Compute the result.
	     * When either side is a float the result is a float.
	     */
#ifdef FEAT_FLOAT
	    if (use_float)
	    {
		if (op == '*')
		    f1 = f1 * f2;
		else if (op == '/')
		{
		    /* We rely on the floating point library to handle divide
		     * by zero to result in "inf" and not a crash. */
		    f1 = f1 / f2;
		}
		else
		{
		    EMSG(_("E804: Cannot use '%' with Float"));
		    return FAIL;
		}
		rettv->v_type = VAR_FLOAT;
		rettv->vval.v_float = f1;
	    }
	    else
#endif
	    {
		if (op == '*')
		    n1 = n1 * n2;
		else if (op == '/')
		{
		    if (n2 == 0)	/* give an error message? */
		    {
			if (n1 == 0)
			    n1 = -0x7fffffffL - 1L;	/* similar to NaN */
			else if (n1 < 0)
			    n1 = -0x7fffffffL;
			else
			    n1 = 0x7fffffffL;
		    }
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
		rettv->v_type = VAR_NUMBER;
		rettv->vval.v_number = n1;
	    }
	}
    }

    return OK;
}

/*
 * Handle sixth level expression:
 *  number		number constant
 *  "string"		string constant
 *  'string'		literal string constant
 *  &option-name	option value
 *  @r			register contents
 *  identifier		variable value
 *  function()		function call
 *  $VAR		environment variable
 *  (expression)	nested expression
 *  [expr, expr]	List
 *  {key: val, key: val}  Dictionary
 *
 *  Also handle:
 *  ! in front		logical NOT
 *  - in front		unary minus
 *  + in front		unary plus (ignored)
 *  trailing []		subscript in String or List
 *  trailing .name	entry in Dictionary
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval7(arg, rettv, evaluate, want_string)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
    int		want_string;	/* after "." operator */
{
    long	n;
    int		len;
    char_u	*s;
    char_u	*start_leader, *end_leader;
    int		ret = OK;
    char_u	*alias;

    /*
     * Initialise variable so that clear_tv() can't mistake this for a
     * string and free a string that isn't there.
     */
    rettv->v_type = VAR_UNKNOWN;

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
	{
#ifdef FEAT_FLOAT
		char_u *p = skipdigits(*arg + 1);
		int    get_float = FALSE;

		/* We accept a float when the format matches
		 * "[0-9]\+\.[0-9]\+\([eE][+-]\?[0-9]\+\)\?".  This is very
		 * strict to avoid backwards compatibility problems.
		 * Don't look for a float after the "." operator, so that
		 * ":let vers = 1.2.3" doesn't fail. */
		if (!want_string && p[0] == '.' && vim_isdigit(p[1]))
		{
		    get_float = TRUE;
		    p = skipdigits(p + 2);
		    if (*p == 'e' || *p == 'E')
		    {
			++p;
			if (*p == '-' || *p == '+')
			    ++p;
			if (!vim_isdigit(*p))
			    get_float = FALSE;
			else
			    p = skipdigits(p + 1);
		    }
		    if (ASCII_ISALPHA(*p) || *p == '.')
			get_float = FALSE;
		}
		if (get_float)
		{
		    float_T	f;

		    *arg += string2float(*arg, &f);
		    if (evaluate)
		    {
			rettv->v_type = VAR_FLOAT;
			rettv->vval.v_float = f;
		    }
		}
		else
#endif
		{
		    vim_str2nr(*arg, NULL, &len, TRUE, TRUE, &n, NULL);
		    *arg += len;
		    if (evaluate)
		    {
			rettv->v_type = VAR_NUMBER;
			rettv->vval.v_number = n;
		    }
		}
		break;
	}

    /*
     * String constant: "string".
     */
    case '"':	ret = get_string_tv(arg, rettv, evaluate);
		break;

    /*
     * Literal string constant: 'str''ing'.
     */
    case '\'':	ret = get_lit_string_tv(arg, rettv, evaluate);
		break;

    /*
     * List: [expr, expr]
     */
    case '[':	ret = get_list_tv(arg, rettv, evaluate);
		break;

    /*
     * Dictionary: {key: val, key: val}
     */
    case '{':	ret = get_dict_tv(arg, rettv, evaluate);
		break;

    /*
     * Option value: &name
     */
    case '&':	ret = get_option_tv(arg, rettv, evaluate);
		break;

    /*
     * Environment variable: $VAR.
     */
    case '$':	ret = get_env_tv(arg, rettv, evaluate);
		break;

    /*
     * Register contents: @r.
     */
    case '@':	++*arg;
		if (evaluate)
		{
		    rettv->v_type = VAR_STRING;
		    rettv->vval.v_string = get_reg_contents(**arg, TRUE, TRUE);
		}
		if (**arg != NUL)
		    ++*arg;
		break;

    /*
     * nested expression: (expression).
     */
    case '(':	*arg = skipwhite(*arg + 1);
		ret = eval1(arg, rettv, evaluate);	/* recursive! */
		if (**arg == ')')
		    ++*arg;
		else if (ret == OK)
		{
		    EMSG(_("E110: Missing ')'"));
		    clear_tv(rettv);
		    ret = FAIL;
		}
		break;

    default:	ret = NOTDONE;
		break;
    }

    if (ret == NOTDONE)
    {
	/*
	 * Must be a variable or function name.
	 * Can also be a curly-braces kind of name: {expr}.
	 */
	s = *arg;
	len = get_name_len(arg, &alias, evaluate, TRUE);
	if (alias != NULL)
	    s = alias;

	if (len <= 0)
	    ret = FAIL;
	else
	{
	    if (**arg == '(')		/* recursive! */
	    {
		/* If "s" is the name of a variable of type VAR_FUNC
		 * use its contents. */
		s = deref_func_name(s, &len);

		/* Invoke the function. */
		ret = get_func_tv(s, len, rettv, arg,
			  curwin->w_cursor.lnum, curwin->w_cursor.lnum,
			  &len, evaluate, NULL);
		/* Stop the expression evaluation when immediately
		 * aborting on error, or when an interrupt occurred or
		 * an exception was thrown but not caught. */
		if (aborting())
		{
		    if (ret == OK)
			clear_tv(rettv);
		    ret = FAIL;
		}
	    }
	    else if (evaluate)
		ret = get_var_tv(s, len, rettv, TRUE);
	    else
		ret = OK;
	}

	if (alias != NULL)
	    vim_free(alias);
    }

    *arg = skipwhite(*arg);

    /* Handle following '[', '(' and '.' for expr[expr], expr.name,
     * expr(expr). */
    if (ret == OK)
	ret = handle_subscript(arg, rettv, evaluate, TRUE);

    /*
     * Apply logical NOT and unary '-', from right to left, ignore '+'.
     */
    if (ret == OK && evaluate && end_leader > start_leader)
    {
	int	    error = FALSE;
	int	    val = 0;
#ifdef FEAT_FLOAT
	float_T	    f = 0.0;

	if (rettv->v_type == VAR_FLOAT)
	    f = rettv->vval.v_float;
	else
#endif
	    val = get_tv_number_chk(rettv, &error);
	if (error)
	{
	    clear_tv(rettv);
	    ret = FAIL;
	}
	else
	{
	    while (end_leader > start_leader)
	    {
		--end_leader;
		if (*end_leader == '!')
		{
#ifdef FEAT_FLOAT
		    if (rettv->v_type == VAR_FLOAT)
			f = !f;
		    else
#endif
			val = !val;
		}
		else if (*end_leader == '-')
		{
#ifdef FEAT_FLOAT
		    if (rettv->v_type == VAR_FLOAT)
			f = -f;
		    else
#endif
			val = -val;
		}
	    }
#ifdef FEAT_FLOAT
	    if (rettv->v_type == VAR_FLOAT)
	    {
		clear_tv(rettv);
		rettv->vval.v_float = f;
	    }
	    else
#endif
	    {
		clear_tv(rettv);
		rettv->v_type = VAR_NUMBER;
		rettv->vval.v_number = val;
	    }
	}
    }

    return ret;
}

/*
 * Evaluate an "[expr]" or "[expr:expr]" index.  Also "dict.key".
 * "*arg" points to the '[' or '.'.
 * Returns FAIL or OK. "*arg" is advanced to after the ']'.
 */
    static int
eval_index(arg, rettv, evaluate, verbose)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
    int		verbose;	/* give error messages */
{
    int		empty1 = FALSE, empty2 = FALSE;
    typval_T	var1, var2;
    long	n1, n2 = 0;
    long	len = -1;
    int		range = FALSE;
    char_u	*s;
    char_u	*key = NULL;

    if (rettv->v_type == VAR_FUNC
#ifdef FEAT_FLOAT
	    || rettv->v_type == VAR_FLOAT
#endif
	    )
    {
	if (verbose)
	    EMSG(_("E695: Cannot index a Funcref"));
	return FAIL;
    }

    if (**arg == '.')
    {
	/*
	 * dict.name
	 */
	key = *arg + 1;
	for (len = 0; ASCII_ISALNUM(key[len]) || key[len] == '_'; ++len)
	    ;
	if (len == 0)
	    return FAIL;
	*arg = skipwhite(key + len);
    }
    else
    {
	/*
	 * something[idx]
	 *
	 * Get the (first) variable from inside the [].
	 */
	*arg = skipwhite(*arg + 1);
	if (**arg == ':')
	    empty1 = TRUE;
	else if (eval1(arg, &var1, evaluate) == FAIL)	/* recursive! */
	    return FAIL;
	else if (evaluate && get_tv_string_chk(&var1) == NULL)
	{
	    /* not a number or string */
	    clear_tv(&var1);
	    return FAIL;
	}

	/*
	 * Get the second variable from inside the [:].
	 */
	if (**arg == ':')
	{
	    range = TRUE;
	    *arg = skipwhite(*arg + 1);
	    if (**arg == ']')
		empty2 = TRUE;
	    else if (eval1(arg, &var2, evaluate) == FAIL)	/* recursive! */
	    {
		if (!empty1)
		    clear_tv(&var1);
		return FAIL;
	    }
	    else if (evaluate && get_tv_string_chk(&var2) == NULL)
	    {
		/* not a number or string */
		if (!empty1)
		    clear_tv(&var1);
		clear_tv(&var2);
		return FAIL;
	    }
	}

	/* Check for the ']'. */
	if (**arg != ']')
	{
	    if (verbose)
		EMSG(_(e_missbrac));
	    clear_tv(&var1);
	    if (range)
		clear_tv(&var2);
	    return FAIL;
	}
	*arg = skipwhite(*arg + 1);	/* skip the ']' */
    }

    if (evaluate)
    {
	n1 = 0;
	if (!empty1 && rettv->v_type != VAR_DICT)
	{
	    n1 = get_tv_number(&var1);
	    clear_tv(&var1);
	}
	if (range)
	{
	    if (empty2)
		n2 = -1;
	    else
	    {
		n2 = get_tv_number(&var2);
		clear_tv(&var2);
	    }
	}

	switch (rettv->v_type)
	{
	    case VAR_NUMBER:
	    case VAR_STRING:
		s = get_tv_string(rettv);
		len = (long)STRLEN(s);
		if (range)
		{
		    /* The resulting variable is a substring.  If the indexes
		     * are out of range the result is empty. */
		    if (n1 < 0)
		    {
			n1 = len + n1;
			if (n1 < 0)
			    n1 = 0;
		    }
		    if (n2 < 0)
			n2 = len + n2;
		    else if (n2 >= len)
			n2 = len;
		    if (n1 >= len || n2 < 0 || n1 > n2)
			s = NULL;
		    else
			s = vim_strnsave(s + n1, (int)(n2 - n1 + 1));
		}
		else
		{
		    /* The resulting variable is a string of a single
		     * character.  If the index is too big or negative the
		     * result is empty. */
		    if (n1 >= len || n1 < 0)
			s = NULL;
		    else
			s = vim_strnsave(s + n1, 1);
		}
		clear_tv(rettv);
		rettv->v_type = VAR_STRING;
		rettv->vval.v_string = s;
		break;

	    case VAR_LIST:
		len = list_len(rettv->vval.v_list);
		if (n1 < 0)
		    n1 = len + n1;
		if (!empty1 && (n1 < 0 || n1 >= len))
		{
		    /* For a range we allow invalid values and return an empty
		     * list.  A list index out of range is an error. */
		    if (!range)
		    {
			if (verbose)
			    EMSGN(_(e_listidx), n1);
			return FAIL;
		    }
		    n1 = len;
		}
		if (range)
		{
		    list_T	*l;
		    listitem_T	*item;

		    if (n2 < 0)
			n2 = len + n2;
		    else if (n2 >= len)
			n2 = len - 1;
		    if (!empty2 && (n2 < 0 || n2 + 1 < n1))
			n2 = -1;
		    l = list_alloc();
		    if (l == NULL)
			return FAIL;
		    for (item = list_find(rettv->vval.v_list, n1);
							       n1 <= n2; ++n1)
		    {
			if (list_append_tv(l, &item->li_tv) == FAIL)
			{
			    list_free(l, TRUE);
			    return FAIL;
			}
			item = item->li_next;
		    }
		    clear_tv(rettv);
		    rettv->v_type = VAR_LIST;
		    rettv->vval.v_list = l;
		    ++l->lv_refcount;
		}
		else
		{
		    copy_tv(&list_find(rettv->vval.v_list, n1)->li_tv, &var1);
		    clear_tv(rettv);
		    *rettv = var1;
		}
		break;

	    case VAR_DICT:
		if (range)
		{
		    if (verbose)
			EMSG(_(e_dictrange));
		    if (len == -1)
			clear_tv(&var1);
		    return FAIL;
		}
		{
		    dictitem_T	*item;

		    if (len == -1)
		    {
			key = get_tv_string(&var1);
			if (*key == NUL)
			{
			    if (verbose)
				EMSG(_(e_emptykey));
			    clear_tv(&var1);
			    return FAIL;
			}
		    }

		    item = dict_find(rettv->vval.v_dict, key, (int)len);

		    if (item == NULL && verbose)
			EMSG2(_(e_dictkey), key);
		    if (len == -1)
			clear_tv(&var1);
		    if (item == NULL)
			return FAIL;

		    copy_tv(&item->di_tv, &var1);
		    clear_tv(rettv);
		    *rettv = var1;
		}
		break;
	}
    }

    return OK;
}

/*
 * Get an option value.
 * "arg" points to the '&' or '+' before the option name.
 * "arg" is advanced to character after the option name.
 * Return OK or FAIL.
 */
    static int
get_option_tv(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;	/* when NULL, only check if option exists */
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
	if (rettv != NULL)
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
			       rettv == NULL ? NULL : &stringval, opt_flags);

    if (opt_type == -3)			/* invalid name */
    {
	if (rettv != NULL)
	    EMSG2(_("E113: Unknown option: %s"), *arg);
	ret = FAIL;
    }
    else if (rettv != NULL)
    {
	if (opt_type == -2)		/* hidden string option */
	{
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = NULL;
	}
	else if (opt_type == -1)	/* hidden number option */
	{
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = 0;
	}
	else if (opt_type == 1)		/* number option */
	{
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = numval;
	}
	else				/* string option */
	{
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = stringval;
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
get_string_tv(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    char_u	*p;
    char_u	*name;
    int		extra = 0;

    /*
     * Find the end of the string, skipping backslashed characters.
     */
    for (p = *arg + 1; *p != NUL && *p != '"'; mb_ptr_adv(p))
    {
	if (*p == '\\' && p[1] != NUL)
	{
	    ++p;
	    /* A "\<x>" form occupies at least 4 characters, and produces up
	     * to 6 characters: reserve space for 2 extra */
	    if (*p == '<')
		extra += 2;
	}
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
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = name;

    for (p = *arg + 1; *p != NUL && *p != '"'; )
    {
	if (*p == '\\')
	{
	    switch (*++p)
	    {
		case 'b': *name++ = BS; ++p; break;
		case 'e': *name++ = ESC; ++p; break;
		case 'f': *name++ = FF; ++p; break;
		case 'n': *name++ = NL; ++p; break;
		case 'r': *name++ = CAR; ++p; break;
		case 't': *name++ = TAB; ++p; break;

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
			      ++p;
#ifdef FEAT_MBYTE
			      /* For "\u" store the number according to
			       * 'encoding'. */
			      if (c != 'X')
				  name += (*mb_char2bytes)(nr, name);
			      else
#endif
				  *name++ = nr;
			  }
			  break;

			  /* octal: "\1", "\12", "\123" */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7': *name = *p++ - '0';
			  if (*p >= '0' && *p <= '7')
			  {
			      *name = (*name << 3) + *p++ - '0';
			      if (*p >= '0' && *p <= '7')
				  *name = (*name << 3) + *p++ - '0';
			  }
			  ++name;
			  break;

			    /* Special key, e.g.: "\<C-W>" */
		case '<': extra = trans_special(&p, name, TRUE);
			  if (extra != 0)
			  {
			      name += extra;
			      break;
			  }
			  /* FALLTHROUGH */

		default:  MB_COPY_CHAR(p, name);
			  break;
	    }
	}
	else
	    MB_COPY_CHAR(p, name);

    }
    *name = NUL;
    *arg = p + 1;

    return OK;
}

/*
 * Allocate a variable for a 'str''ing' constant.
 * Return OK or FAIL.
 */
    static int
get_lit_string_tv(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    char_u	*p;
    char_u	*str;
    int		reduce = 0;

    /*
     * Find the end of the string, skipping ''.
     */
    for (p = *arg + 1; *p != NUL; mb_ptr_adv(p))
    {
	if (*p == '\'')
	{
	    if (p[1] != '\'')
		break;
	    ++reduce;
	    ++p;
	}
    }

    if (*p != '\'')
    {
	EMSG2(_("E115: Missing quote: %s"), *arg);
	return FAIL;
    }

    /* If only parsing return after setting "*arg" */
    if (!evaluate)
    {
	*arg = p + 1;
	return OK;
    }

    /*
     * Copy the string into allocated memory, handling '' to ' reduction.
     */
    str = alloc((unsigned)((p - *arg) - reduce));
    if (str == NULL)
	return FAIL;
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = str;

    for (p = *arg + 1; *p != NUL; )
    {
	if (*p == '\'')
	{
	    if (p[1] != '\'')
		break;
	    ++p;
	}
	MB_COPY_CHAR(p, str);
    }
    *str = NUL;
    *arg = p + 1;

    return OK;
}

/*
 * Allocate a variable for a List and fill it from "*arg".
 * Return OK or FAIL.
 */
    static int
get_list_tv(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    list_T	*l = NULL;
    typval_T	tv;
    listitem_T	*item;

    if (evaluate)
    {
	l = list_alloc();
	if (l == NULL)
	    return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    while (**arg != ']' && **arg != NUL)
    {
	if (eval1(arg, &tv, evaluate) == FAIL)	/* recursive! */
	    goto failret;
	if (evaluate)
	{
	    item = listitem_alloc();
	    if (item != NULL)
	    {
		item->li_tv = tv;
		item->li_tv.v_lock = 0;
		list_append(l, item);
	    }
	    else
		clear_tv(&tv);
	}

	if (**arg == ']')
	    break;
	if (**arg != ',')
	{
	    EMSG2(_("E696: Missing comma in List: %s"), *arg);
	    goto failret;
	}
	*arg = skipwhite(*arg + 1);
    }

    if (**arg != ']')
    {
	EMSG2(_("E697: Missing end of List ']': %s"), *arg);
failret:
	if (evaluate)
	    list_free(l, TRUE);
	return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    if (evaluate)
    {
	rettv->v_type = VAR_LIST;
	rettv->vval.v_list = l;
	++l->lv_refcount;
    }

    return OK;
}

/*
 * Allocate an empty header for a list.
 * Caller should take care of the reference count.
 */
    list_T *
list_alloc()
{
    list_T  *l;

    l = (list_T *)alloc_clear(sizeof(list_T));
    if (l != NULL)
    {
	/* Prepend the list to the list of lists for garbage collection. */
	if (first_list != NULL)
	    first_list->lv_used_prev = l;
	l->lv_used_prev = NULL;
	l->lv_used_next = first_list;
	first_list = l;
    }
    return l;
}

/*
 * Allocate an empty list for a return value.
 * Returns OK or FAIL.
 */
    static int
rettv_list_alloc(rettv)
    typval_T	*rettv;
{
    list_T	*l = list_alloc();

    if (l == NULL)
	return FAIL;

    rettv->vval.v_list = l;
    rettv->v_type = VAR_LIST;
    ++l->lv_refcount;
    return OK;
}

/*
 * Unreference a list: decrement the reference count and free it when it
 * becomes zero.
 */
    void
list_unref(l)
    list_T *l;
{
    if (l != NULL && --l->lv_refcount <= 0)
	list_free(l, TRUE);
}

/*
 * Free a list, including all items it points to.
 * Ignores the reference count.
 */
    void
list_free(l, recurse)
    list_T  *l;
    int	    recurse;	/* Free Lists and Dictionaries recursively. */
{
    listitem_T *item;

    /* Remove the list from the list of lists for garbage collection. */
    if (l->lv_used_prev == NULL)
	first_list = l->lv_used_next;
    else
	l->lv_used_prev->lv_used_next = l->lv_used_next;
    if (l->lv_used_next != NULL)
	l->lv_used_next->lv_used_prev = l->lv_used_prev;

    for (item = l->lv_first; item != NULL; item = l->lv_first)
    {
	/* Remove the item before deleting it. */
	l->lv_first = item->li_next;
	if (recurse || (item->li_tv.v_type != VAR_LIST
					   && item->li_tv.v_type != VAR_DICT))
	    clear_tv(&item->li_tv);
	vim_free(item);
    }
    vim_free(l);
}

/*
 * Allocate a list item.
 */
    static listitem_T *
listitem_alloc()
{
    return (listitem_T *)alloc(sizeof(listitem_T));
}

/*
 * Free a list item.  Also clears the value.  Does not notify watchers.
 */
    static void
listitem_free(item)
    listitem_T *item;
{
    clear_tv(&item->li_tv);
    vim_free(item);
}

/*
 * Remove a list item from a List and free it.  Also clears the value.
 */
    static void
listitem_remove(l, item)
    list_T  *l;
    listitem_T *item;
{
    list_remove(l, item, item);
    listitem_free(item);
}

/*
 * Get the number of items in a list.
 */
    static long
list_len(l)
    list_T	*l;
{
    if (l == NULL)
	return 0L;
    return l->lv_len;
}

/*
 * Return TRUE when two lists have exactly the same values.
 */
    static int
list_equal(l1, l2, ic)
    list_T	*l1;
    list_T	*l2;
    int		ic;	/* ignore case for strings */
{
    listitem_T	*item1, *item2;

    if (l1 == NULL || l2 == NULL)
	return FALSE;
    if (l1 == l2)
	return TRUE;
    if (list_len(l1) != list_len(l2))
	return FALSE;

    for (item1 = l1->lv_first, item2 = l2->lv_first;
	    item1 != NULL && item2 != NULL;
			       item1 = item1->li_next, item2 = item2->li_next)
	if (!tv_equal(&item1->li_tv, &item2->li_tv, ic))
	    return FALSE;
    return item1 == NULL && item2 == NULL;
}

#if defined(FEAT_PYTHON) || defined(PROTO)
/*
 * Return the dictitem that an entry in a hashtable points to.
 */
    dictitem_T *
dict_lookup(hi)
    hashitem_T *hi;
{
    return HI2DI(hi);
}
#endif

/*
 * Return TRUE when two dictionaries have exactly the same key/values.
 */
    static int
dict_equal(d1, d2, ic)
    dict_T	*d1;
    dict_T	*d2;
    int		ic;	/* ignore case for strings */
{
    hashitem_T	*hi;
    dictitem_T	*item2;
    int		todo;

    if (d1 == NULL || d2 == NULL)
	return FALSE;
    if (d1 == d2)
	return TRUE;
    if (dict_len(d1) != dict_len(d2))
	return FALSE;

    todo = (int)d1->dv_hashtab.ht_used;
    for (hi = d1->dv_hashtab.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    item2 = dict_find(d2, hi->hi_key, -1);
	    if (item2 == NULL)
		return FALSE;
	    if (!tv_equal(&HI2DI(hi)->di_tv, &item2->di_tv, ic))
		return FALSE;
	    --todo;
	}
    }
    return TRUE;
}

/*
 * Return TRUE if "tv1" and "tv2" have the same value.
 * Compares the items just like "==" would compare them, but strings and
 * numbers are different.  Floats and numbers are also different.
 */
    static int
tv_equal(tv1, tv2, ic)
    typval_T *tv1;
    typval_T *tv2;
    int	    ic;	    /* ignore case */
{
    char_u	buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    char_u	*s1, *s2;
    static int  recursive = 0;	    /* cach recursive loops */
    int		r;

    if (tv1->v_type != tv2->v_type)
	return FALSE;
    /* Catch lists and dicts that have an endless loop by limiting
     * recursiveness to 1000.  We guess they are equal then. */
    if (recursive >= 1000)
	return TRUE;

    switch (tv1->v_type)
    {
	case VAR_LIST:
	    ++recursive;
	    r = list_equal(tv1->vval.v_list, tv2->vval.v_list, ic);
	    --recursive;
	    return r;

	case VAR_DICT:
	    ++recursive;
	    r = dict_equal(tv1->vval.v_dict, tv2->vval.v_dict, ic);
	    --recursive;
	    return r;

	case VAR_FUNC:
	    return (tv1->vval.v_string != NULL
		    && tv2->vval.v_string != NULL
		    && STRCMP(tv1->vval.v_string, tv2->vval.v_string) == 0);

	case VAR_NUMBER:
	    return tv1->vval.v_number == tv2->vval.v_number;

#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    return tv1->vval.v_float == tv2->vval.v_float;
#endif

	case VAR_STRING:
	    s1 = get_tv_string_buf(tv1, buf1);
	    s2 = get_tv_string_buf(tv2, buf2);
	    return ((ic ? MB_STRICMP(s1, s2) : STRCMP(s1, s2)) == 0);
    }

    EMSG2(_(e_intern2), "tv_equal()");
    return TRUE;
}

/*
 * Locate item with index "n" in list "l" and return it.
 * A negative index is counted from the end; -1 is the last item.
 * Returns NULL when "n" is out of range.
 */
    static listitem_T *
list_find(l, n)
    list_T	*l;
    long	n;
{
    listitem_T	*item;
    long	idx;

    if (l == NULL)
	return NULL;

    /* Negative index is relative to the end. */
    if (n < 0)
	n = l->lv_len + n;

    /* Check for index out of range. */
    if (n < 0 || n >= l->lv_len)
	return NULL;

    /* When there is a cached index may start search from there. */
    if (l->lv_idx_item != NULL)
    {
	if (n < l->lv_idx / 2)
	{
	    /* closest to the start of the list */
	    item = l->lv_first;
	    idx = 0;
	}
	else if (n > (l->lv_idx + l->lv_len) / 2)
	{
	    /* closest to the end of the list */
	    item = l->lv_last;
	    idx = l->lv_len - 1;
	}
	else
	{
	    /* closest to the cached index */
	    item = l->lv_idx_item;
	    idx = l->lv_idx;
	}
    }
    else
    {
	if (n < l->lv_len / 2)
	{
	    /* closest to the start of the list */
	    item = l->lv_first;
	    idx = 0;
	}
	else
	{
	    /* closest to the end of the list */
	    item = l->lv_last;
	    idx = l->lv_len - 1;
	}
    }

    while (n > idx)
    {
	/* search forward */
	item = item->li_next;
	++idx;
    }
    while (n < idx)
    {
	/* search backward */
	item = item->li_prev;
	--idx;
    }

    /* cache the used index */
    l->lv_idx = idx;
    l->lv_idx_item = item;

    return item;
}

/*
 * Get list item "l[idx]" as a number.
 */
    static long
list_find_nr(l, idx, errorp)
    list_T	*l;
    long	idx;
    int		*errorp;	/* set to TRUE when something wrong */
{
    listitem_T	*li;

    li = list_find(l, idx);
    if (li == NULL)
    {
	if (errorp != NULL)
	    *errorp = TRUE;
	return -1L;
    }
    return get_tv_number_chk(&li->li_tv, errorp);
}

/*
 * Get list item "l[idx - 1]" as a string.  Returns NULL for failure.
 */
    char_u *
list_find_str(l, idx)
    list_T	*l;
    long	idx;
{
    listitem_T	*li;

    li = list_find(l, idx - 1);
    if (li == NULL)
    {
	EMSGN(_(e_listidx), idx);
	return NULL;
    }
    return get_tv_string(&li->li_tv);
}

/*
 * Locate "item" list "l" and return its index.
 * Returns -1 when "item" is not in the list.
 */
    static long
list_idx_of_item(l, item)
    list_T	*l;
    listitem_T	*item;
{
    long	idx = 0;
    listitem_T	*li;

    if (l == NULL)
	return -1;
    idx = 0;
    for (li = l->lv_first; li != NULL && li != item; li = li->li_next)
	++idx;
    if (li == NULL)
	return -1;
    return idx;
}

/*
 * Append item "item" to the end of list "l".
 */
    static void
list_append(l, item)
    list_T	*l;
    listitem_T	*item;
{
    if (l->lv_last == NULL)
    {
	/* empty list */
	l->lv_first = item;
	l->lv_last = item;
	item->li_prev = NULL;
    }
    else
    {
	l->lv_last->li_next = item;
	item->li_prev = l->lv_last;
	l->lv_last = item;
    }
    ++l->lv_len;
    item->li_next = NULL;
}

/*
 * Append typval_T "tv" to the end of list "l".
 * Return FAIL when out of memory.
 */
    static int
list_append_tv(l, tv)
    list_T	*l;
    typval_T	*tv;
{
    listitem_T	*li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    copy_tv(tv, &li->li_tv);
    list_append(l, li);
    return OK;
}

/*
 * Add a dictionary to a list.  Used by getqflist().
 * Return FAIL when out of memory.
 */
    int
list_append_dict(list, dict)
    list_T	*list;
    dict_T	*dict;
{
    listitem_T	*li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    li->li_tv.v_type = VAR_DICT;
    li->li_tv.v_lock = 0;
    li->li_tv.vval.v_dict = dict;
    list_append(list, li);
    ++dict->dv_refcount;
    return OK;
}

/*
 * Make a copy of "str" and append it as an item to list "l".
 * When "len" >= 0 use "str[len]".
 * Returns FAIL when out of memory.
 */
    int
list_append_string(l, str, len)
    list_T	*l;
    char_u	*str;
    int		len;
{
    listitem_T *li = listitem_alloc();

    if (li == NULL)
	return FAIL;
    list_append(l, li);
    li->li_tv.v_type = VAR_STRING;
    li->li_tv.v_lock = 0;
    if (str == NULL)
	li->li_tv.vval.v_string = NULL;
    else if ((li->li_tv.vval.v_string = (len >= 0 ? vim_strnsave(str, len)
						 : vim_strsave(str))) == NULL)
	return FAIL;
    return OK;
}

/*
 * Append "n" to list "l".
 * Returns FAIL when out of memory.
 */
    static int
list_append_number(l, n)
    list_T	*l;
    varnumber_T	n;
{
    listitem_T	*li;

    li = listitem_alloc();
    if (li == NULL)
	return FAIL;
    li->li_tv.v_type = VAR_NUMBER;
    li->li_tv.v_lock = 0;
    li->li_tv.vval.v_number = n;
    list_append(l, li);
    return OK;
}

/*
 * Insert typval_T "tv" in list "l" before "item".
 * If "item" is NULL append at the end.
 * Return FAIL when out of memory.
 */
    static int
list_insert_tv(l, tv, item)
    list_T	*l;
    typval_T	*tv;
    listitem_T	*item;
{
    listitem_T	*ni = listitem_alloc();

    if (ni == NULL)
	return FAIL;
    copy_tv(tv, &ni->li_tv);
    if (item == NULL)
	/* Append new item at end of list. */
	list_append(l, ni);
    else
    {
	/* Insert new item before existing item. */
	ni->li_prev = item->li_prev;
	ni->li_next = item;
	if (item->li_prev == NULL)
	{
	    l->lv_first = ni;
	    ++l->lv_idx;
	}
	else
	{
	    item->li_prev->li_next = ni;
	    l->lv_idx_item = NULL;
	}
	item->li_prev = ni;
	++l->lv_len;
    }
    return OK;
}

/*
 * Extend "l1" with "l2".
 * If "bef" is NULL append at the end, otherwise insert before this item.
 * Returns FAIL when out of memory.
 */
    static int
list_extend(l1, l2, bef)
    list_T	*l1;
    list_T	*l2;
    listitem_T	*bef;
{
    listitem_T	*item;
    int		todo = l2->lv_len;

    /* We also quit the loop when we have inserted the original item count of
     * the list, avoid a hang when we extend a list with itself. */
    for (item = l2->lv_first; item != NULL && --todo >= 0; item = item->li_next)
	if (list_insert_tv(l1, &item->li_tv, bef) == FAIL)
	    return FAIL;
    return OK;
}

/*
 * Concatenate lists "l1" and "l2" into a new list, stored in "tv".
 * Return FAIL when out of memory.
 */
    static int
list_concat(l1, l2, tv)
    list_T	*l1;
    list_T	*l2;
    typval_T	*tv;
{
    list_T	*l;

    if (l1 == NULL || l2 == NULL)
	return FAIL;

    /* make a copy of the first list. */
    l = list_copy(l1, FALSE, 0);
    if (l == NULL)
	return FAIL;
    tv->v_type = VAR_LIST;
    tv->vval.v_list = l;

    /* append all items from the second list */
    return list_extend(l, l2, NULL);
}

/*
 * Make a copy of list "orig".  Shallow if "deep" is FALSE.
 * The refcount of the new list is set to 1.
 * See item_copy() for "copyID".
 * Returns NULL when out of memory.
 */
    static list_T *
list_copy(orig, deep, copyID)
    list_T	*orig;
    int		deep;
    int		copyID;
{
    list_T	*copy;
    listitem_T	*item;
    listitem_T	*ni;

    if (orig == NULL)
	return NULL;

    copy = list_alloc();
    if (copy != NULL)
    {
	if (copyID != 0)
	{
	    /* Do this before adding the items, because one of the items may
	     * refer back to this list. */
	    orig->lv_copyID = copyID;
	    orig->lv_copylist = copy;
	}
	for (item = orig->lv_first; item != NULL && !got_int;
							 item = item->li_next)
	{
	    ni = listitem_alloc();
	    if (ni == NULL)
		break;
	    if (deep)
	    {
		if (item_copy(&item->li_tv, &ni->li_tv, deep, copyID) == FAIL)
		{
		    vim_free(ni);
		    break;
		}
	    }
	    else
		copy_tv(&item->li_tv, &ni->li_tv);
	    list_append(copy, ni);
	}
	++copy->lv_refcount;
	if (item != NULL)
	{
	    list_unref(copy);
	    copy = NULL;
	}
    }

    return copy;
}

/*
 * Remove items "item" to "item2" from list "l".
 * Does not free the listitem or the value!
 */
    static void
list_remove(l, item, item2)
    list_T	*l;
    listitem_T	*item;
    listitem_T	*item2;
{
    listitem_T	*ip;

    /* notify watchers */
    for (ip = item; ip != NULL; ip = ip->li_next)
    {
	--l->lv_len;
	list_fix_watch(l, ip);
	if (ip == item2)
	    break;
    }

    if (item2->li_next == NULL)
	l->lv_last = item->li_prev;
    else
	item2->li_next->li_prev = item->li_prev;
    if (item->li_prev == NULL)
	l->lv_first = item2->li_next;
    else
	item->li_prev->li_next = item2->li_next;
    l->lv_idx_item = NULL;
}

/*
 * Return an allocated string with the string representation of a list.
 * May return NULL.
 */
    static char_u *
list2string(tv, copyID)
    typval_T	*tv;
    int		copyID;
{
    garray_T	ga;

    if (tv->vval.v_list == NULL)
	return NULL;
    ga_init2(&ga, (int)sizeof(char), 80);
    ga_append(&ga, '[');
    if (list_join(&ga, tv->vval.v_list, (char_u *)", ", FALSE, copyID) == FAIL)
    {
	vim_free(ga.ga_data);
	return NULL;
    }
    ga_append(&ga, ']');
    ga_append(&ga, NUL);
    return (char_u *)ga.ga_data;
}

/*
 * Join list "l" into a string in "*gap", using separator "sep".
 * When "echo" is TRUE use String as echoed, otherwise as inside a List.
 * Return FAIL or OK.
 */
    static int
list_join(gap, l, sep, echo, copyID)
    garray_T	*gap;
    list_T	*l;
    char_u	*sep;
    int		echo;
    int		copyID;
{
    int		first = TRUE;
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];
    listitem_T	*item;
    char_u	*s;

    for (item = l->lv_first; item != NULL && !got_int; item = item->li_next)
    {
	if (first)
	    first = FALSE;
	else
	    ga_concat(gap, sep);

	if (echo)
	    s = echo_string(&item->li_tv, &tofree, numbuf, copyID);
	else
	    s = tv2string(&item->li_tv, &tofree, numbuf, copyID);
	if (s != NULL)
	    ga_concat(gap, s);
	vim_free(tofree);
	if (s == NULL)
	    return FAIL;
    }
    return OK;
}

/*
 * Garbage collection for lists and dictionaries.
 *
 * We use reference counts to be able to free most items right away when they
 * are no longer used.  But for composite items it's possible that it becomes
 * unused while the reference count is > 0: When there is a recursive
 * reference.  Example:
 *	:let l = [1, 2, 3]
 *	:let d = {9: l}
 *	:let l[1] = d
 *
 * Since this is quite unusual we handle this with garbage collection: every
 * once in a while find out which lists and dicts are not referenced from any
 * variable.
 *
 * Here is a good reference text about garbage collection (refers to Python
 * but it applies to all reference-counting mechanisms):
 *	http://python.ca/nas/python/gc/
 */

/*
 * Do garbage collection for lists and dicts.
 * Return TRUE if some memory was freed.
 */
    int
garbage_collect()
{
    dict_T	*dd;
    list_T	*ll;
    int		copyID = ++current_copyID;
    buf_T	*buf;
    win_T	*wp;
    int		i;
    funccall_T	*fc, **pfc;
    int		did_free = FALSE;
#ifdef FEAT_WINDOWS
    tabpage_T	*tp;
#endif

    /* Only do this once. */
    want_garbage_collect = FALSE;
    may_garbage_collect = FALSE;
    garbage_collect_at_exit = FALSE;

    /*
     * 1. Go through all accessible variables and mark all lists and dicts
     *    with copyID.
     */
    /* script-local variables */
    for (i = 1; i <= ga_scripts.ga_len; ++i)
	set_ref_in_ht(&SCRIPT_VARS(i), copyID);

    /* buffer-local variables */
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	set_ref_in_ht(&buf->b_vars.dv_hashtab, copyID);

    /* window-local variables */
    FOR_ALL_TAB_WINDOWS(tp, wp)
	set_ref_in_ht(&wp->w_vars.dv_hashtab, copyID);

#ifdef FEAT_WINDOWS
    /* tabpage-local variables */
    for (tp = first_tabpage; tp != NULL; tp = tp->tp_next)
	set_ref_in_ht(&tp->tp_vars.dv_hashtab, copyID);
#endif

    /* global variables */
    set_ref_in_ht(&globvarht, copyID);

    /* function-local variables */
    for (fc = current_funccal; fc != NULL; fc = fc->caller)
    {
	set_ref_in_ht(&fc->l_vars.dv_hashtab, copyID);
	set_ref_in_ht(&fc->l_avars.dv_hashtab, copyID);
    }

    /* v: vars */
    set_ref_in_ht(&vimvarht, copyID);

    /*
     * 2. Go through the list of dicts and free items without the copyID.
     */
    for (dd = first_dict; dd != NULL; )
	if (dd->dv_copyID != copyID)
	{
	    /* Free the Dictionary and ordinary items it contains, but don't
	     * recurse into Lists and Dictionaries, they will be in the list
	     * of dicts or list of lists. */
	    dict_free(dd, FALSE);
	    did_free = TRUE;

	    /* restart, next dict may also have been freed */
	    dd = first_dict;
	}
	else
	    dd = dd->dv_used_next;

    /*
     * 3. Go through the list of lists and free items without the copyID.
     *    But don't free a list that has a watcher (used in a for loop), these
     *    are not referenced anywhere.
     */
    for (ll = first_list; ll != NULL; )
	if (ll->lv_copyID != copyID && ll->lv_watch == NULL)
	{
	    /* Free the List and ordinary items it contains, but don't recurse
	     * into Lists and Dictionaries, they will be in the list of dicts
	     * or list of lists. */
	    list_free(ll, FALSE);
	    did_free = TRUE;

	    /* restart, next list may also have been freed */
	    ll = first_list;
	}
	else
	    ll = ll->lv_used_next;

    /* check if any funccal can be freed now */
    for (pfc = &previous_funccal; *pfc != NULL; )
    {
	if (can_free_funccal(*pfc, copyID))
	{
	    fc = *pfc;
	    *pfc = fc->caller;
	    free_funccal(fc, TRUE);
	    did_free = TRUE;
	}
	else
	    pfc = &(*pfc)->caller;
    }

    return did_free;
}

/*
 * Mark all lists and dicts referenced through hashtab "ht" with "copyID".
 */
    static void
set_ref_in_ht(ht, copyID)
    hashtab_T	*ht;
    int		copyID;
{
    int		todo;
    hashitem_T	*hi;

    todo = (int)ht->ht_used;
    for (hi = ht->ht_array; todo > 0; ++hi)
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    set_ref_in_item(&HI2DI(hi)->di_tv, copyID);
	}
}

/*
 * Mark all lists and dicts referenced through list "l" with "copyID".
 */
    static void
set_ref_in_list(l, copyID)
    list_T	*l;
    int		copyID;
{
    listitem_T *li;

    for (li = l->lv_first; li != NULL; li = li->li_next)
	set_ref_in_item(&li->li_tv, copyID);
}

/*
 * Mark all lists and dicts referenced through typval "tv" with "copyID".
 */
    static void
set_ref_in_item(tv, copyID)
    typval_T	*tv;
    int		copyID;
{
    dict_T	*dd;
    list_T	*ll;

    switch (tv->v_type)
    {
	case VAR_DICT:
	    dd = tv->vval.v_dict;
	    if (dd != NULL && dd->dv_copyID != copyID)
	    {
		/* Didn't see this dict yet. */
		dd->dv_copyID = copyID;
		set_ref_in_ht(&dd->dv_hashtab, copyID);
	    }
	    break;

	case VAR_LIST:
	    ll = tv->vval.v_list;
	    if (ll != NULL && ll->lv_copyID != copyID)
	    {
		/* Didn't see this list yet. */
		ll->lv_copyID = copyID;
		set_ref_in_list(ll, copyID);
	    }
	    break;
    }
    return;
}

/*
 * Allocate an empty header for a dictionary.
 */
    dict_T *
dict_alloc()
{
    dict_T *d;

    d = (dict_T *)alloc(sizeof(dict_T));
    if (d != NULL)
    {
	/* Add the list to the list of dicts for garbage collection. */
	if (first_dict != NULL)
	    first_dict->dv_used_prev = d;
	d->dv_used_next = first_dict;
	d->dv_used_prev = NULL;
	first_dict = d;

	hash_init(&d->dv_hashtab);
	d->dv_lock = 0;
	d->dv_refcount = 0;
	d->dv_copyID = 0;
    }
    return d;
}

/*
 * Unreference a Dictionary: decrement the reference count and free it when it
 * becomes zero.
 */
    static void
dict_unref(d)
    dict_T *d;
{
    if (d != NULL && --d->dv_refcount <= 0)
	dict_free(d, TRUE);
}

/*
 * Free a Dictionary, including all items it contains.
 * Ignores the reference count.
 */
    static void
dict_free(d, recurse)
    dict_T  *d;
    int	    recurse;	/* Free Lists and Dictionaries recursively. */
{
    int		todo;
    hashitem_T	*hi;
    dictitem_T	*di;

    /* Remove the dict from the list of dicts for garbage collection. */
    if (d->dv_used_prev == NULL)
	first_dict = d->dv_used_next;
    else
	d->dv_used_prev->dv_used_next = d->dv_used_next;
    if (d->dv_used_next != NULL)
	d->dv_used_next->dv_used_prev = d->dv_used_prev;

    /* Lock the hashtab, we don't want it to resize while freeing items. */
    hash_lock(&d->dv_hashtab);
    todo = (int)d->dv_hashtab.ht_used;
    for (hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    /* Remove the item before deleting it, just in case there is
	     * something recursive causing trouble. */
	    di = HI2DI(hi);
	    hash_remove(&d->dv_hashtab, hi);
	    if (recurse || (di->di_tv.v_type != VAR_LIST
					     && di->di_tv.v_type != VAR_DICT))
		clear_tv(&di->di_tv);
	    vim_free(di);
	    --todo;
	}
    }
    hash_clear(&d->dv_hashtab);
    vim_free(d);
}

/*
 * Allocate a Dictionary item.
 * The "key" is copied to the new item.
 * Note that the value of the item "di_tv" still needs to be initialized!
 * Returns NULL when out of memory.
 */
    static dictitem_T *
dictitem_alloc(key)
    char_u	*key;
{
    dictitem_T *di;

    di = (dictitem_T *)alloc((unsigned)(sizeof(dictitem_T) + STRLEN(key)));
    if (di != NULL)
    {
	STRCPY(di->di_key, key);
	di->di_flags = 0;
    }
    return di;
}

/*
 * Make a copy of a Dictionary item.
 */
    static dictitem_T *
dictitem_copy(org)
    dictitem_T *org;
{
    dictitem_T *di;

    di = (dictitem_T *)alloc((unsigned)(sizeof(dictitem_T)
						      + STRLEN(org->di_key)));
    if (di != NULL)
    {
	STRCPY(di->di_key, org->di_key);
	di->di_flags = 0;
	copy_tv(&org->di_tv, &di->di_tv);
    }
    return di;
}

/*
 * Remove item "item" from Dictionary "dict" and free it.
 */
    static void
dictitem_remove(dict, item)
    dict_T	*dict;
    dictitem_T	*item;
{
    hashitem_T	*hi;

    hi = hash_find(&dict->dv_hashtab, item->di_key);
    if (HASHITEM_EMPTY(hi))
	EMSG2(_(e_intern2), "dictitem_remove()");
    else
	hash_remove(&dict->dv_hashtab, hi);
    dictitem_free(item);
}

/*
 * Free a dict item.  Also clears the value.
 */
    static void
dictitem_free(item)
    dictitem_T *item;
{
    clear_tv(&item->di_tv);
    vim_free(item);
}

/*
 * Make a copy of dict "d".  Shallow if "deep" is FALSE.
 * The refcount of the new dict is set to 1.
 * See item_copy() for "copyID".
 * Returns NULL when out of memory.
 */
    static dict_T *
dict_copy(orig, deep, copyID)
    dict_T	*orig;
    int		deep;
    int		copyID;
{
    dict_T	*copy;
    dictitem_T	*di;
    int		todo;
    hashitem_T	*hi;

    if (orig == NULL)
	return NULL;

    copy = dict_alloc();
    if (copy != NULL)
    {
	if (copyID != 0)
	{
	    orig->dv_copyID = copyID;
	    orig->dv_copydict = copy;
	}
	todo = (int)orig->dv_hashtab.ht_used;
	for (hi = orig->dv_hashtab.ht_array; todo > 0 && !got_int; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;

		di = dictitem_alloc(hi->hi_key);
		if (di == NULL)
		    break;
		if (deep)
		{
		    if (item_copy(&HI2DI(hi)->di_tv, &di->di_tv, deep,
							      copyID) == FAIL)
		    {
			vim_free(di);
			break;
		    }
		}
		else
		    copy_tv(&HI2DI(hi)->di_tv, &di->di_tv);
		if (dict_add(copy, di) == FAIL)
		{
		    dictitem_free(di);
		    break;
		}
	    }
	}

	++copy->dv_refcount;
	if (todo > 0)
	{
	    dict_unref(copy);
	    copy = NULL;
	}
    }

    return copy;
}

/*
 * Add item "item" to Dictionary "d".
 * Returns FAIL when out of memory and when key already existed.
 */
    static int
dict_add(d, item)
    dict_T	*d;
    dictitem_T	*item;
{
    return hash_add(&d->dv_hashtab, item->di_key);
}

/*
 * Add a number or string entry to dictionary "d".
 * When "str" is NULL use number "nr", otherwise use "str".
 * Returns FAIL when out of memory and when key already exists.
 */
    int
dict_add_nr_str(d, key, nr, str)
    dict_T	*d;
    char	*key;
    long	nr;
    char_u	*str;
{
    dictitem_T	*item;

    item = dictitem_alloc((char_u *)key);
    if (item == NULL)
	return FAIL;
    item->di_tv.v_lock = 0;
    if (str == NULL)
    {
	item->di_tv.v_type = VAR_NUMBER;
	item->di_tv.vval.v_number = nr;
    }
    else
    {
	item->di_tv.v_type = VAR_STRING;
	item->di_tv.vval.v_string = vim_strsave(str);
    }
    if (dict_add(d, item) == FAIL)
    {
	dictitem_free(item);
	return FAIL;
    }
    return OK;
}

/*
 * Get the number of items in a Dictionary.
 */
    static long
dict_len(d)
    dict_T	*d;
{
    if (d == NULL)
	return 0L;
    return (long)d->dv_hashtab.ht_used;
}

/*
 * Find item "key[len]" in Dictionary "d".
 * If "len" is negative use strlen(key).
 * Returns NULL when not found.
 */
    static dictitem_T *
dict_find(d, key, len)
    dict_T	*d;
    char_u	*key;
    int		len;
{
#define AKEYLEN 200
    char_u	buf[AKEYLEN];
    char_u	*akey;
    char_u	*tofree = NULL;
    hashitem_T	*hi;

    if (len < 0)
	akey = key;
    else if (len >= AKEYLEN)
    {
	tofree = akey = vim_strnsave(key, len);
	if (akey == NULL)
	    return NULL;
    }
    else
    {
	/* Avoid a malloc/free by using buf[]. */
	vim_strncpy(buf, key, len);
	akey = buf;
    }

    hi = hash_find(&d->dv_hashtab, akey);
    vim_free(tofree);
    if (HASHITEM_EMPTY(hi))
	return NULL;
    return HI2DI(hi);
}

/*
 * Get a string item from a dictionary.
 * When "save" is TRUE allocate memory for it.
 * Returns NULL if the entry doesn't exist or out of memory.
 */
    char_u *
get_dict_string(d, key, save)
    dict_T	*d;
    char_u	*key;
    int		save;
{
    dictitem_T	*di;
    char_u	*s;

    di = dict_find(d, key, -1);
    if (di == NULL)
	return NULL;
    s = get_tv_string(&di->di_tv);
    if (save && s != NULL)
	s = vim_strsave(s);
    return s;
}

/*
 * Get a number item from a dictionary.
 * Returns 0 if the entry doesn't exist or out of memory.
 */
    long
get_dict_number(d, key)
    dict_T	*d;
    char_u	*key;
{
    dictitem_T	*di;

    di = dict_find(d, key, -1);
    if (di == NULL)
	return 0;
    return get_tv_number(&di->di_tv);
}

/*
 * Return an allocated string with the string representation of a Dictionary.
 * May return NULL.
 */
    static char_u *
dict2string(tv, copyID)
    typval_T	*tv;
    int		copyID;
{
    garray_T	ga;
    int		first = TRUE;
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];
    hashitem_T	*hi;
    char_u	*s;
    dict_T	*d;
    int		todo;

    if ((d = tv->vval.v_dict) == NULL)
	return NULL;
    ga_init2(&ga, (int)sizeof(char), 80);
    ga_append(&ga, '{');

    todo = (int)d->dv_hashtab.ht_used;
    for (hi = d->dv_hashtab.ht_array; todo > 0 && !got_int; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;

	    if (first)
		first = FALSE;
	    else
		ga_concat(&ga, (char_u *)", ");

	    tofree = string_quote(hi->hi_key, FALSE);
	    if (tofree != NULL)
	    {
		ga_concat(&ga, tofree);
		vim_free(tofree);
	    }
	    ga_concat(&ga, (char_u *)": ");
	    s = tv2string(&HI2DI(hi)->di_tv, &tofree, numbuf, copyID);
	    if (s != NULL)
		ga_concat(&ga, s);
	    vim_free(tofree);
	    if (s == NULL)
		break;
	}
    }
    if (todo > 0)
    {
	vim_free(ga.ga_data);
	return NULL;
    }

    ga_append(&ga, '}');
    ga_append(&ga, NUL);
    return (char_u *)ga.ga_data;
}

/*
 * Allocate a variable for a Dictionary and fill it from "*arg".
 * Return OK or FAIL.  Returns NOTDONE for {expr}.
 */
    static int
get_dict_tv(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    dict_T	*d = NULL;
    typval_T	tvkey;
    typval_T	tv;
    char_u	*key = NULL;
    dictitem_T	*item;
    char_u	*start = skipwhite(*arg + 1);
    char_u	buf[NUMBUFLEN];

    /*
     * First check if it's not a curly-braces thing: {expr}.
     * Must do this without evaluating, otherwise a function may be called
     * twice.  Unfortunately this means we need to call eval1() twice for the
     * first item.
     * But {} is an empty Dictionary.
     */
    if (*start != '}')
    {
	if (eval1(&start, &tv, FALSE) == FAIL)	/* recursive! */
	    return FAIL;
	if (*start == '}')
	    return NOTDONE;
    }

    if (evaluate)
    {
	d = dict_alloc();
	if (d == NULL)
	    return FAIL;
    }
    tvkey.v_type = VAR_UNKNOWN;
    tv.v_type = VAR_UNKNOWN;

    *arg = skipwhite(*arg + 1);
    while (**arg != '}' && **arg != NUL)
    {
	if (eval1(arg, &tvkey, evaluate) == FAIL)	/* recursive! */
	    goto failret;
	if (**arg != ':')
	{
	    EMSG2(_("E720: Missing colon in Dictionary: %s"), *arg);
	    clear_tv(&tvkey);
	    goto failret;
	}
	if (evaluate)
	{
	    key = get_tv_string_buf_chk(&tvkey, buf);
	    if (key == NULL || *key == NUL)
	    {
		/* "key" is NULL when get_tv_string_buf_chk() gave an errmsg */
		if (key != NULL)
		    EMSG(_(e_emptykey));
		clear_tv(&tvkey);
		goto failret;
	    }
	}

	*arg = skipwhite(*arg + 1);
	if (eval1(arg, &tv, evaluate) == FAIL)	/* recursive! */
	{
	    if (evaluate)
		clear_tv(&tvkey);
	    goto failret;
	}
	if (evaluate)
	{
	    item = dict_find(d, key, -1);
	    if (item != NULL)
	    {
		EMSG2(_("E721: Duplicate key in Dictionary: \"%s\""), key);
		clear_tv(&tvkey);
		clear_tv(&tv);
		goto failret;
	    }
	    item = dictitem_alloc(key);
	    clear_tv(&tvkey);
	    if (item != NULL)
	    {
		item->di_tv = tv;
		item->di_tv.v_lock = 0;
		if (dict_add(d, item) == FAIL)
		    dictitem_free(item);
	    }
	}

	if (**arg == '}')
	    break;
	if (**arg != ',')
	{
	    EMSG2(_("E722: Missing comma in Dictionary: %s"), *arg);
	    goto failret;
	}
	*arg = skipwhite(*arg + 1);
    }

    if (**arg != '}')
    {
	EMSG2(_("E723: Missing end of Dictionary '}': %s"), *arg);
failret:
	if (evaluate)
	    dict_free(d, TRUE);
	return FAIL;
    }

    *arg = skipwhite(*arg + 1);
    if (evaluate)
    {
	rettv->v_type = VAR_DICT;
	rettv->vval.v_dict = d;
	++d->dv_refcount;
    }

    return OK;
}

/*
 * Return a string with the string representation of a variable.
 * If the memory is allocated "tofree" is set to it, otherwise NULL.
 * "numbuf" is used for a number.
 * Does not put quotes around strings, as ":echo" displays values.
 * When "copyID" is not NULL replace recursive lists and dicts with "...".
 * May return NULL.
 */
    static char_u *
echo_string(tv, tofree, numbuf, copyID)
    typval_T	*tv;
    char_u	**tofree;
    char_u	*numbuf;
    int		copyID;
{
    static int	recurse = 0;
    char_u	*r = NULL;

    if (recurse >= DICT_MAXNEST)
    {
	EMSG(_("E724: variable nested too deep for displaying"));
	*tofree = NULL;
	return NULL;
    }
    ++recurse;

    switch (tv->v_type)
    {
	case VAR_FUNC:
	    *tofree = NULL;
	    r = tv->vval.v_string;
	    break;

	case VAR_LIST:
	    if (tv->vval.v_list == NULL)
	    {
		*tofree = NULL;
		r = NULL;
	    }
	    else if (copyID != 0 && tv->vval.v_list->lv_copyID == copyID)
	    {
		*tofree = NULL;
		r = (char_u *)"[...]";
	    }
	    else
	    {
		tv->vval.v_list->lv_copyID = copyID;
		*tofree = list2string(tv, copyID);
		r = *tofree;
	    }
	    break;

	case VAR_DICT:
	    if (tv->vval.v_dict == NULL)
	    {
		*tofree = NULL;
		r = NULL;
	    }
	    else if (copyID != 0 && tv->vval.v_dict->dv_copyID == copyID)
	    {
		*tofree = NULL;
		r = (char_u *)"{...}";
	    }
	    else
	    {
		tv->vval.v_dict->dv_copyID = copyID;
		*tofree = dict2string(tv, copyID);
		r = *tofree;
	    }
	    break;

	case VAR_STRING:
	case VAR_NUMBER:
	    *tofree = NULL;
	    r = get_tv_string_buf(tv, numbuf);
	    break;

#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    *tofree = NULL;
	    vim_snprintf((char *)numbuf, NUMBUFLEN, "%g", tv->vval.v_float);
	    r = numbuf;
	    break;
#endif

	default:
	    EMSG2(_(e_intern2), "echo_string()");
	    *tofree = NULL;
    }

    --recurse;
    return r;
}

/*
 * Return a string with the string representation of a variable.
 * If the memory is allocated "tofree" is set to it, otherwise NULL.
 * "numbuf" is used for a number.
 * Puts quotes around strings, so that they can be parsed back by eval().
 * May return NULL.
 */
    static char_u *
tv2string(tv, tofree, numbuf, copyID)
    typval_T	*tv;
    char_u	**tofree;
    char_u	*numbuf;
    int		copyID;
{
    switch (tv->v_type)
    {
	case VAR_FUNC:
	    *tofree = string_quote(tv->vval.v_string, TRUE);
	    return *tofree;
	case VAR_STRING:
	    *tofree = string_quote(tv->vval.v_string, FALSE);
	    return *tofree;
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    *tofree = NULL;
	    vim_snprintf((char *)numbuf, NUMBUFLEN - 1, "%g", tv->vval.v_float);
	    return numbuf;
#endif
	case VAR_NUMBER:
	case VAR_LIST:
	case VAR_DICT:
	    break;
	default:
	    EMSG2(_(e_intern2), "tv2string()");
    }
    return echo_string(tv, tofree, numbuf, copyID);
}

/*
 * Return string "str" in ' quotes, doubling ' characters.
 * If "str" is NULL an empty string is assumed.
 * If "function" is TRUE make it function('string').
 */
    static char_u *
string_quote(str, function)
    char_u	*str;
    int		function;
{
    unsigned	len;
    char_u	*p, *r, *s;

    len = (function ? 13 : 3);
    if (str != NULL)
    {
	len += (unsigned)STRLEN(str);
	for (p = str; *p != NUL; mb_ptr_adv(p))
	    if (*p == '\'')
		++len;
    }
    s = r = alloc(len);
    if (r != NULL)
    {
	if (function)
	{
	    STRCPY(r, "function('");
	    r += 10;
	}
	else
	    *r++ = '\'';
	if (str != NULL)
	    for (p = str; *p != NUL; )
	    {
		if (*p == '\'')
		    *r++ = '\'';
		MB_COPY_CHAR(p, r);
	    }
	*r++ = '\'';
	if (function)
	    *r++ = ')';
	*r++ = NUL;
    }
    return s;
}

#ifdef FEAT_FLOAT
/*
 * Convert the string "text" to a floating point number.
 * This uses strtod().  setlocale(LC_NUMERIC, "C") has been used to make sure
 * this always uses a decimal point.
 * Returns the length of the text that was consumed.
 */
    static int
string2float(text, value)
    char_u	*text;
    float_T	*value;	    /* result stored here */
{
    char	*s = (char *)text;
    float_T	f;

    f = strtod(s, &s);
    *value = f;
    return (int)((char_u *)s - text);
}
#endif

/*
 * Get the value of an environment variable.
 * "arg" is pointing to the '$'.  It is advanced to after the name.
 * If the environment variable was not set, silently assume it is empty.
 * Always return OK.
 */
    static int
get_env_tv(arg, rettv, evaluate)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;
{
    char_u	*string = NULL;
    int		len;
    int		cc;
    char_u	*name;
    int		mustfree = FALSE;

    ++*arg;
    name = *arg;
    len = get_env_len(arg);
    if (evaluate)
    {
	if (len != 0)
	{
	    cc = name[len];
	    name[len] = NUL;
	    /* first try vim_getenv(), fast for normal environment vars */
	    string = vim_getenv(name, &mustfree);
	    if (string != NULL && *string != NUL)
	    {
		if (!mustfree)
		    string = vim_strsave(string);
	    }
	    else
	    {
		if (mustfree)
		    vim_free(string);

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
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = string;
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
    void	(*f_func) __ARGS((typval_T *args, typval_T *rvar));
				/* implementation of function */
} functions[] =
{
#ifdef FEAT_FLOAT
    {"abs",		1, 1, f_abs},
#endif
    {"add",		2, 2, f_add},
    {"append",		2, 2, f_append},
    {"argc",		0, 0, f_argc},
    {"argidx",		0, 0, f_argidx},
    {"argv",		0, 1, f_argv},
#ifdef FEAT_FLOAT
    {"atan",		1, 1, f_atan},
#endif
    {"browse",		4, 4, f_browse},
    {"browsedir",	2, 2, f_browsedir},
    {"bufexists",	1, 1, f_bufexists},
    {"buffer_exists",	1, 1, f_bufexists},	/* obsolete */
    {"buffer_name",	1, 1, f_bufname},	/* obsolete */
    {"buffer_number",	1, 1, f_bufnr},		/* obsolete */
    {"buflisted",	1, 1, f_buflisted},
    {"bufloaded",	1, 1, f_bufloaded},
    {"bufname",		1, 1, f_bufname},
    {"bufnr",		1, 2, f_bufnr},
    {"bufwinnr",	1, 1, f_bufwinnr},
    {"byte2line",	1, 1, f_byte2line},
    {"byteidx",		2, 2, f_byteidx},
    {"call",		2, 3, f_call},
#ifdef FEAT_FLOAT
    {"ceil",		1, 1, f_ceil},
#endif
    {"changenr",	0, 0, f_changenr},
    {"char2nr",		1, 1, f_char2nr},
    {"cindent",		1, 1, f_cindent},
    {"clearmatches",	0, 0, f_clearmatches},
    {"col",		1, 1, f_col},
#if defined(FEAT_INS_EXPAND)
    {"complete",	2, 2, f_complete},
    {"complete_add",	1, 1, f_complete_add},
    {"complete_check",	0, 0, f_complete_check},
#endif
    {"confirm",		1, 4, f_confirm},
    {"copy",		1, 1, f_copy},
#ifdef FEAT_FLOAT
    {"cos",		1, 1, f_cos},
#endif
    {"count",		2, 4, f_count},
    {"cscope_connection",0,3, f_cscope_connection},
    {"cursor",		1, 3, f_cursor},
    {"deepcopy",	1, 2, f_deepcopy},
    {"delete",		1, 1, f_delete},
    {"did_filetype",	0, 0, f_did_filetype},
    {"diff_filler",	1, 1, f_diff_filler},
    {"diff_hlID",	2, 2, f_diff_hlID},
    {"empty",		1, 1, f_empty},
    {"escape",		2, 2, f_escape},
    {"eval",		1, 1, f_eval},
    {"eventhandler",	0, 0, f_eventhandler},
    {"executable",	1, 1, f_executable},
    {"exists",		1, 1, f_exists},
    {"expand",		1, 2, f_expand},
    {"extend",		2, 3, f_extend},
    {"feedkeys",	1, 2, f_feedkeys},
    {"file_readable",	1, 1, f_filereadable},	/* obsolete */
    {"filereadable",	1, 1, f_filereadable},
    {"filewritable",	1, 1, f_filewritable},
    {"filter",		2, 2, f_filter},
    {"finddir",		1, 3, f_finddir},
    {"findfile",	1, 3, f_findfile},
#ifdef FEAT_FLOAT
    {"float2nr",	1, 1, f_float2nr},
    {"floor",		1, 1, f_floor},
#endif
    {"fnameescape",	1, 1, f_fnameescape},
    {"fnamemodify",	2, 2, f_fnamemodify},
    {"foldclosed",	1, 1, f_foldclosed},
    {"foldclosedend",	1, 1, f_foldclosedend},
    {"foldlevel",	1, 1, f_foldlevel},
    {"foldtext",	0, 0, f_foldtext},
    {"foldtextresult",	1, 1, f_foldtextresult},
    {"foreground",	0, 0, f_foreground},
    {"function",	1, 1, f_function},
    {"garbagecollect",	0, 1, f_garbagecollect},
    {"get",		2, 3, f_get},
    {"getbufline",	2, 3, f_getbufline},
    {"getbufvar",	2, 2, f_getbufvar},
    {"getchar",		0, 1, f_getchar},
    {"getcharmod",	0, 0, f_getcharmod},
    {"getcmdline",	0, 0, f_getcmdline},
    {"getcmdpos",	0, 0, f_getcmdpos},
    {"getcmdtype",	0, 0, f_getcmdtype},
    {"getcwd",		0, 0, f_getcwd},
    {"getfontname",	0, 1, f_getfontname},
    {"getfperm",	1, 1, f_getfperm},
    {"getfsize",	1, 1, f_getfsize},
    {"getftime",	1, 1, f_getftime},
    {"getftype",	1, 1, f_getftype},
    {"getline",		1, 2, f_getline},
    {"getloclist",	1, 1, f_getqflist},
    {"getmatches",	0, 0, f_getmatches},
    {"getpid",		0, 0, f_getpid},
    {"getpos",		1, 1, f_getpos},
    {"getqflist",	0, 0, f_getqflist},
    {"getreg",		0, 2, f_getreg},
    {"getregtype",	0, 1, f_getregtype},
    {"gettabwinvar",	3, 3, f_gettabwinvar},
    {"getwinposx",	0, 0, f_getwinposx},
    {"getwinposy",	0, 0, f_getwinposy},
    {"getwinvar",	2, 2, f_getwinvar},
    {"glob",		1, 2, f_glob},
    {"globpath",	2, 3, f_globpath},
    {"has",		1, 1, f_has},
    {"has_key",		2, 2, f_has_key},
    {"haslocaldir",	0, 0, f_haslocaldir},
    {"hasmapto",	1, 3, f_hasmapto},
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
    {"index",		2, 4, f_index},
    {"input",		1, 3, f_input},
    {"inputdialog",	1, 3, f_inputdialog},
    {"inputlist",	1, 1, f_inputlist},
    {"inputrestore",	0, 0, f_inputrestore},
    {"inputsave",	0, 0, f_inputsave},
    {"inputsecret",	1, 2, f_inputsecret},
    {"insert",		2, 3, f_insert},
    {"isdirectory",	1, 1, f_isdirectory},
    {"islocked",	1, 1, f_islocked},
    {"items",		1, 1, f_items},
    {"join",		1, 2, f_join},
    {"keys",		1, 1, f_keys},
    {"last_buffer_nr",	0, 0, f_last_buffer_nr},/* obsolete */
    {"len",		1, 1, f_len},
    {"libcall",		3, 3, f_libcall},
    {"libcallnr",	3, 3, f_libcallnr},
    {"line",		1, 1, f_line},
    {"line2byte",	1, 1, f_line2byte},
    {"lispindent",	1, 1, f_lispindent},
    {"localtime",	0, 0, f_localtime},
#ifdef FEAT_FLOAT
    {"log10",		1, 1, f_log10},
#endif
    {"map",		2, 2, f_map},
    {"maparg",		1, 3, f_maparg},
    {"mapcheck",	1, 3, f_mapcheck},
    {"match",		2, 4, f_match},
    {"matchadd",	2, 4, f_matchadd},
    {"matcharg",	1, 1, f_matcharg},
    {"matchdelete",	1, 1, f_matchdelete},
    {"matchend",	2, 4, f_matchend},
    {"matchlist",	2, 4, f_matchlist},
    {"matchstr",	2, 4, f_matchstr},
    {"max",		1, 1, f_max},
    {"min",		1, 1, f_min},
#ifdef vim_mkdir
    {"mkdir",		1, 3, f_mkdir},
#endif
    {"mode",		0, 1, f_mode},
    {"nextnonblank",	1, 1, f_nextnonblank},
    {"nr2char",		1, 1, f_nr2char},
    {"pathshorten",	1, 1, f_pathshorten},
#ifdef FEAT_FLOAT
    {"pow",		2, 2, f_pow},
#endif
    {"prevnonblank",	1, 1, f_prevnonblank},
    {"printf",		2, 19, f_printf},
    {"pumvisible",	0, 0, f_pumvisible},
    {"range",		1, 3, f_range},
    {"readfile",	1, 3, f_readfile},
    {"reltime",		0, 2, f_reltime},
    {"reltimestr",	1, 1, f_reltimestr},
    {"remote_expr",	2, 3, f_remote_expr},
    {"remote_foreground", 1, 1, f_remote_foreground},
    {"remote_peek",	1, 2, f_remote_peek},
    {"remote_read",	1, 1, f_remote_read},
    {"remote_send",	2, 3, f_remote_send},
    {"remove",		2, 3, f_remove},
    {"rename",		2, 2, f_rename},
    {"repeat",		2, 2, f_repeat},
    {"resolve",		1, 1, f_resolve},
    {"reverse",		1, 1, f_reverse},
#ifdef FEAT_FLOAT
    {"round",		1, 1, f_round},
#endif
    {"search",		1, 4, f_search},
    {"searchdecl",	1, 3, f_searchdecl},
    {"searchpair",	3, 7, f_searchpair},
    {"searchpairpos",	3, 7, f_searchpairpos},
    {"searchpos",	1, 4, f_searchpos},
    {"server2client",	2, 2, f_server2client},
    {"serverlist",	0, 0, f_serverlist},
    {"setbufvar",	3, 3, f_setbufvar},
    {"setcmdpos",	1, 1, f_setcmdpos},
    {"setline",		2, 2, f_setline},
    {"setloclist",	2, 3, f_setloclist},
    {"setmatches",	1, 1, f_setmatches},
    {"setpos",		2, 2, f_setpos},
    {"setqflist",	1, 2, f_setqflist},
    {"setreg",		2, 3, f_setreg},
    {"settabwinvar",	4, 4, f_settabwinvar},
    {"setwinvar",	3, 3, f_setwinvar},
    {"shellescape",	1, 2, f_shellescape},
    {"simplify",	1, 1, f_simplify},
#ifdef FEAT_FLOAT
    {"sin",		1, 1, f_sin},
#endif
    {"sort",		1, 2, f_sort},
    {"soundfold",	1, 1, f_soundfold},
    {"spellbadword",	0, 1, f_spellbadword},
    {"spellsuggest",	1, 3, f_spellsuggest},
    {"split",		1, 3, f_split},
#ifdef FEAT_FLOAT
    {"sqrt",		1, 1, f_sqrt},
    {"str2float",	1, 1, f_str2float},
#endif
    {"str2nr",		1, 2, f_str2nr},
#ifdef HAVE_STRFTIME
    {"strftime",	1, 2, f_strftime},
#endif
    {"stridx",		2, 3, f_stridx},
    {"string",		1, 1, f_string},
    {"strlen",		1, 1, f_strlen},
    {"strpart",		2, 3, f_strpart},
    {"strridx",		2, 3, f_strridx},
    {"strtrans",	1, 1, f_strtrans},
    {"submatch",	1, 1, f_submatch},
    {"substitute",	4, 4, f_substitute},
    {"synID",		3, 3, f_synID},
    {"synIDattr",	2, 3, f_synIDattr},
    {"synIDtrans",	1, 1, f_synIDtrans},
    {"synstack",	2, 2, f_synstack},
    {"system",		1, 2, f_system},
    {"tabpagebuflist",	0, 1, f_tabpagebuflist},
    {"tabpagenr",	0, 1, f_tabpagenr},
    {"tabpagewinnr",	1, 2, f_tabpagewinnr},
    {"tagfiles",	0, 0, f_tagfiles},
    {"taglist",		1, 1, f_taglist},
    {"tempname",	0, 0, f_tempname},
    {"test",		1, 1, f_test},
    {"tolower",		1, 1, f_tolower},
    {"toupper",		1, 1, f_toupper},
    {"tr",		3, 3, f_tr},
#ifdef FEAT_FLOAT
    {"trunc",		1, 1, f_trunc},
#endif
    {"type",		1, 1, f_type},
    {"values",		1, 1, f_values},
    {"virtcol",		1, 1, f_virtcol},
    {"visualmode",	0, 1, f_visualmode},
    {"winbufnr",	1, 1, f_winbufnr},
    {"wincol",		0, 0, f_wincol},
    {"winheight",	1, 1, f_winheight},
    {"winline",		0, 0, f_winline},
    {"winnr",		0, 1, f_winnr},
    {"winrestcmd",	0, 0, f_winrestcmd},
    {"winrestview",	1, 1, f_winrestview},
    {"winsaveview",	0, 0, f_winsaveview},
    {"winwidth",	1, 1, f_winwidth},
    {"writefile",	2, 3, f_writefile},
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
 * Check if "name" is a variable of type VAR_FUNC.  If so, return the function
 * name it contains, otherwise return "name".
 */
    static char_u *
deref_func_name(name, lenp)
    char_u	*name;
    int		*lenp;
{
    dictitem_T	*v;
    int		cc;

    cc = name[*lenp];
    name[*lenp] = NUL;
    v = find_var(name, NULL);
    name[*lenp] = cc;
    if (v != NULL && v->di_tv.v_type == VAR_FUNC)
    {
	if (v->di_tv.vval.v_string == NULL)
	{
	    *lenp = 0;
	    return (char_u *)"";	/* just in case */
	}
	*lenp = (int)STRLEN(v->di_tv.vval.v_string);
	return v->di_tv.vval.v_string;
    }

    return name;
}

/*
 * Allocate a variable for the result of a function.
 * Return OK or FAIL.
 */
    static int
get_func_tv(name, len, rettv, arg, firstline, lastline, doesrange,
							   evaluate, selfdict)
    char_u	*name;		/* name of the function */
    int		len;		/* length of "name" */
    typval_T	*rettv;
    char_u	**arg;		/* argument, pointing to the '(' */
    linenr_T	firstline;	/* first line of range */
    linenr_T	lastline;	/* last line of range */
    int		*doesrange;	/* return: function handled range */
    int		evaluate;
    dict_T	*selfdict;	/* Dictionary for "self" */
{
    char_u	*argp;
    int		ret = OK;
    typval_T	argvars[MAX_FUNC_ARGS + 1];	/* vars for arguments */
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
	ret = call_func(name, len, rettv, argcount, argvars,
			  firstline, lastline, doesrange, evaluate, selfdict);
    else if (!aborting())
    {
	if (argcount == MAX_FUNC_ARGS)
	    emsg_funcname(N_("E740: Too many arguments for function %s"), name);
	else
	    emsg_funcname(N_("E116: Invalid arguments for function %s"), name);
    }

    while (--argcount >= 0)
	clear_tv(&argvars[argcount]);

    *arg = skipwhite(argp);
    return ret;
}


/*
 * Call a function with its resolved parameters
 * Return OK when the function can't be called,  FAIL otherwise.
 * Also returns OK when an error was encountered while executing the function.
 */
    static int
call_func(name, len, rettv, argcount, argvars, firstline, lastline,
						doesrange, evaluate, selfdict)
    char_u	*name;		/* name of the function */
    int		len;		/* length of "name" */
    typval_T	*rettv;		/* return value goes here */
    int		argcount;	/* number of "argvars" */
    typval_T	*argvars;	/* vars for arguments, must have "argcount"
				   PLUS ONE elements! */
    linenr_T	firstline;	/* first line of range */
    linenr_T	lastline;	/* last line of range */
    int		*doesrange;	/* return: function handled range */
    int		evaluate;
    dict_T	*selfdict;	/* Dictionary for "self" */
{
    int		ret = FAIL;
#define ERROR_UNKNOWN	0
#define ERROR_TOOMANY	1
#define ERROR_TOOFEW	2
#define ERROR_SCRIPT	3
#define ERROR_DICT	4
#define ERROR_NONE	5
#define ERROR_OTHER	6
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
	rettv->v_type = VAR_NUMBER;	/* default is number rettv */
	error = ERROR_UNKNOWN;

	if (!builtin_function(fname))
	{
	    /*
	     * User defined function.
	     */
	    fp = find_func(fname);

#ifdef FEAT_AUTOCMD
	    /* Trigger FuncUndefined event, may load the function. */
	    if (fp == NULL
		    && apply_autocmds(EVENT_FUNCUNDEFINED,
						     fname, fname, TRUE, NULL)
		    && !aborting())
	    {
		/* executed an autocommand, search for the function again */
		fp = find_func(fname);
	    }
#endif
	    /* Try loading a package. */
	    if (fp == NULL && script_autoload(fname, TRUE) && !aborting())
	    {
		/* loaded a package, search for the function again */
		fp = find_func(fname);
	    }

	    if (fp != NULL)
	    {
		if (fp->uf_flags & FC_RANGE)
		    *doesrange = TRUE;
		if (argcount < fp->uf_args.ga_len)
		    error = ERROR_TOOFEW;
		else if (!fp->uf_varargs && argcount > fp->uf_args.ga_len)
		    error = ERROR_TOOMANY;
		else if ((fp->uf_flags & FC_DICT) && selfdict == NULL)
		    error = ERROR_DICT;
		else
		{
		    /*
		     * Call the user function.
		     * Save and restore search patterns, script variables and
		     * redo buffer.
		     */
		    save_search_patterns();
		    saveRedobuff();
		    ++fp->uf_calls;
		    call_user_func(fp, argcount, argvars, rettv,
					       firstline, lastline,
				  (fp->uf_flags & FC_DICT) ? selfdict : NULL);
		    if (--fp->uf_calls <= 0 && isdigit(*fp->uf_name)
						      && fp->uf_refcount <= 0)
			/* Function was unreferenced while being used, free it
			 * now. */
			func_free(fp);
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
		    argvars[argcount].v_type = VAR_UNKNOWN;
		    functions[i].f_func(argvars, rettv);
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
    if (!aborting())
    {
	switch (error)
	{
	    case ERROR_UNKNOWN:
		    emsg_funcname(N_("E117: Unknown function: %s"), name);
		    break;
	    case ERROR_TOOMANY:
		    emsg_funcname(e_toomanyarg, name);
		    break;
	    case ERROR_TOOFEW:
		    emsg_funcname(N_("E119: Not enough arguments for function: %s"),
									name);
		    break;
	    case ERROR_SCRIPT:
		    emsg_funcname(N_("E120: Using <SID> not in a script context: %s"),
									name);
		    break;
	    case ERROR_DICT:
		    emsg_funcname(N_("E725: Calling dict function without Dictionary: %s"),
									name);
		    break;
	}
    }

    name[len] = cc;
    if (fname != name && fname != fname_buf)
	vim_free(fname);

    return ret;
}

/*
 * Give an error message with a function name.  Handle <SNR> things.
 * "ermsg" is to be passed without translation, use N_() instead of _().
 */
    static void
emsg_funcname(ermsg, name)
    char	*ermsg;
    char_u	*name;
{
    char_u	*p;

    if (*name == K_SPECIAL)
	p = concat_str((char_u *)"<SNR>", name + 3);
    else
	p = name;
    EMSG2(_(ermsg), p);
    if (p != name)
	vim_free(p);
}

/*
 * Return TRUE for a non-zero Number and a non-empty String.
 */
    static int
non_zero_arg(argvars)
    typval_T	*argvars;
{
    return ((argvars[0].v_type == VAR_NUMBER
		&& argvars[0].vval.v_number != 0)
	    || (argvars[0].v_type == VAR_STRING
		&& argvars[0].vval.v_string != NULL
		&& *argvars[0].vval.v_string != NUL));
}

/*********************************************
 * Implementation of the built-in functions
 */

#ifdef FEAT_FLOAT
/*
 * "abs(expr)" function
 */
    static void
f_abs(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    if (argvars[0].v_type == VAR_FLOAT)
    {
	rettv->v_type = VAR_FLOAT;
	rettv->vval.v_float = fabs(argvars[0].vval.v_float);
    }
    else
    {
	varnumber_T	n;
	int		error = FALSE;

	n = get_tv_number_chk(&argvars[0], &error);
	if (error)
	    rettv->vval.v_number = -1;
	else if (n > 0)
	    rettv->vval.v_number = n;
	else
	    rettv->vval.v_number = -n;
    }
}
#endif

/*
 * "add(list, item)" function
 */
    static void
f_add(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    list_T	*l;

    rettv->vval.v_number = 1; /* Default: Failed */
    if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) != NULL
		&& !tv_check_lock(l->lv_lock, (char_u *)"add()")
		&& list_append_tv(l, &argvars[1]) == OK)
	    copy_tv(&argvars[0], rettv);
    }
    else
	EMSG(_(e_listreq));
}

/*
 * "append(lnum, string/list)" function
 */
    static void
f_append(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    long	lnum;
    char_u	*line;
    list_T	*l = NULL;
    listitem_T	*li = NULL;
    typval_T	*tv;
    long	added = 0;

    lnum = get_tv_lnum(argvars);
    if (lnum >= 0
	    && lnum <= curbuf->b_ml.ml_line_count
	    && u_save(lnum, lnum + 1) == OK)
    {
	if (argvars[1].v_type == VAR_LIST)
	{
	    l = argvars[1].vval.v_list;
	    if (l == NULL)
		return;
	    li = l->lv_first;
	}
	rettv->vval.v_number = 0;	/* Default: Success */
	for (;;)
	{
	    if (l == NULL)
		tv = &argvars[1];	/* append a string */
	    else if (li == NULL)
		break;			/* end of list */
	    else
		tv = &li->li_tv;	/* append item from list */
	    line = get_tv_string_chk(tv);
	    if (line == NULL)		/* type error */
	    {
		rettv->vval.v_number = 1;	/* Failed */
		break;
	    }
	    ml_append(lnum + added, line, (colnr_T)0, FALSE);
	    ++added;
	    if (l == NULL)
		break;
	    li = li->li_next;
	}

	appended_lines_mark(lnum, added);
	if (curwin->w_cursor.lnum > lnum)
	    curwin->w_cursor.lnum += added;
    }
    else
	rettv->vval.v_number = 1;	/* Failed */
}

/*
 * "argc()" function
 */
/* ARGSUSED */
    static void
f_argc(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = ARGCOUNT;
}

/*
 * "argidx()" function
 */
/* ARGSUSED */
    static void
f_argidx(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = curwin->w_arg_idx;
}

/*
 * "argv(nr)" function
 */
    static void
f_argv(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		idx;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	idx = get_tv_number_chk(&argvars[0], NULL);
	if (idx >= 0 && idx < ARGCOUNT)
	    rettv->vval.v_string = vim_strsave(alist_name(&ARGLIST[idx]));
	else
	    rettv->vval.v_string = NULL;
	rettv->v_type = VAR_STRING;
    }
    else if (rettv_list_alloc(rettv) == OK)
	for (idx = 0; idx < ARGCOUNT; ++idx)
	    list_append_string(rettv->vval.v_list,
					       alist_name(&ARGLIST[idx]), -1);
}

#ifdef FEAT_FLOAT
static int get_float_arg __ARGS((typval_T *argvars, float_T *f));

/*
 * Get the float value of "argvars[0]" into "f".
 * Returns FAIL when the argument is not a Number or Float.
 */
    static int
get_float_arg(argvars, f)
    typval_T	*argvars;
    float_T	*f;
{
    if (argvars[0].v_type == VAR_FLOAT)
    {
	*f = argvars[0].vval.v_float;
	return OK;
    }
    if (argvars[0].v_type == VAR_NUMBER)
    {
	*f = (float_T)argvars[0].vval.v_number;
	return OK;
    }
    EMSG(_("E808: Number or Float required"));
    return FAIL;
}

/*
 * "atan()" function
 */
    static void
f_atan(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = atan(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "browse(save, title, initdir, default)" function
 */
/* ARGSUSED */
    static void
f_browse(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_BROWSE
    int		save;
    char_u	*title;
    char_u	*initdir;
    char_u	*defname;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    int		error = FALSE;

    save = get_tv_number_chk(&argvars[0], &error);
    title = get_tv_string_chk(&argvars[1]);
    initdir = get_tv_string_buf_chk(&argvars[2], buf);
    defname = get_tv_string_buf_chk(&argvars[3], buf2);

    if (error || title == NULL || initdir == NULL || defname == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string =
		 do_browse(save ? BROWSE_SAVE : 0,
				 title, defname, NULL, initdir, NULL, curbuf);
#else
    rettv->vval.v_string = NULL;
#endif
    rettv->v_type = VAR_STRING;
}

/*
 * "browsedir(title, initdir)" function
 */
/* ARGSUSED */
    static void
f_browsedir(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_BROWSE
    char_u	*title;
    char_u	*initdir;
    char_u	buf[NUMBUFLEN];

    title = get_tv_string_chk(&argvars[0]);
    initdir = get_tv_string_buf_chk(&argvars[1], buf);

    if (title == NULL || initdir == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = do_browse(BROWSE_DIR,
				    title, NULL, NULL, initdir, NULL, curbuf);
#else
    rettv->vval.v_string = NULL;
#endif
    rettv->v_type = VAR_STRING;
}

static buf_T *find_buffer __ARGS((typval_T *avar));

/*
 * Find a buffer by number or exact name.
 */
    static buf_T *
find_buffer(avar)
    typval_T	*avar;
{
    buf_T	*buf = NULL;

    if (avar->v_type == VAR_NUMBER)
	buf = buflist_findnr((int)avar->vval.v_number);
    else if (avar->v_type == VAR_STRING && avar->vval.v_string != NULL)
    {
	buf = buflist_findname_exp(avar->vval.v_string);
	if (buf == NULL)
	{
	    /* No full path name match, try a match with a URL or a "nofile"
	     * buffer, these don't use the full path. */
	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
		if (buf->b_fname != NULL
			&& (path_with_url(buf->b_fname)
#ifdef FEAT_QUICKFIX
			    || bt_nofile(buf)
#endif
			   )
			&& STRCMP(buf->b_fname, avar->vval.v_string) == 0)
		    break;
	}
    }
    return buf;
}

/*
 * "bufexists(expr)" function
 */
    static void
f_bufexists(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = (find_buffer(&argvars[0]) != NULL);
}

/*
 * "buflisted(expr)" function
 */
    static void
f_buflisted(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    buf_T	*buf;

    buf = find_buffer(&argvars[0]);
    rettv->vval.v_number = (buf != NULL && buf->b_p_bl);
}

/*
 * "bufloaded(expr)" function
 */
    static void
f_bufloaded(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    buf_T	*buf;

    buf = find_buffer(&argvars[0]);
    rettv->vval.v_number = (buf != NULL && buf->b_ml.ml_mfp != NULL);
}

static buf_T *get_buf_tv __ARGS((typval_T *tv));

/*
 * Get buffer by number or pattern.
 */
    static buf_T *
get_buf_tv(tv)
    typval_T	*tv;
{
    char_u	*name = tv->vval.v_string;
    int		save_magic;
    char_u	*save_cpo;
    buf_T	*buf;

    if (tv->v_type == VAR_NUMBER)
	return buflist_findnr((int)tv->vval.v_number);
    if (tv->v_type != VAR_STRING)
	return NULL;
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
	buf = find_buffer(tv);

    return buf;
}

/*
 * "bufname(expr)" function
 */
    static void
f_bufname(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    buf_T	*buf;

    (void)get_tv_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = get_buf_tv(&argvars[0]);
    rettv->v_type = VAR_STRING;
    if (buf != NULL && buf->b_fname != NULL)
	rettv->vval.v_string = vim_strsave(buf->b_fname);
    else
	rettv->vval.v_string = NULL;
    --emsg_off;
}

/*
 * "bufnr(expr)" function
 */
    static void
f_bufnr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    buf_T	*buf;
    int		error = FALSE;
    char_u	*name;

    (void)get_tv_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = get_buf_tv(&argvars[0]);
    --emsg_off;

    /* If the buffer isn't found and the second argument is not zero create a
     * new buffer. */
    if (buf == NULL
	    && argvars[1].v_type != VAR_UNKNOWN
	    && get_tv_number_chk(&argvars[1], &error) != 0
	    && !error
	    && (name = get_tv_string_chk(&argvars[0])) != NULL
	    && !error)
	buf = buflist_new(name, NULL, (linenr_T)1, 0);

    if (buf != NULL)
	rettv->vval.v_number = buf->b_fnum;
    else
	rettv->vval.v_number = -1;
}

/*
 * "bufwinnr(nr)" function
 */
    static void
f_bufwinnr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_WINDOWS
    win_T	*wp;
    int		winnr = 0;
#endif
    buf_T	*buf;

    (void)get_tv_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = get_buf_tv(&argvars[0]);
#ifdef FEAT_WINDOWS
    for (wp = firstwin; wp; wp = wp->w_next)
    {
	++winnr;
	if (wp->w_buffer == buf)
	    break;
    }
    rettv->vval.v_number = (wp != NULL ? winnr : -1);
#else
    rettv->vval.v_number = (curwin->w_buffer == buf ? 1 : -1);
#endif
    --emsg_off;
}

/*
 * "byte2line(byte)" function
 */
/*ARGSUSED*/
    static void
f_byte2line(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifndef FEAT_BYTEOFF
    rettv->vval.v_number = -1;
#else
    long	boff = 0;

    boff = get_tv_number(&argvars[0]) - 1;  /* boff gets -1 on type error */
    if (boff < 0)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = ml_find_line_or_offset(curbuf,
							  (linenr_T)0, &boff);
#endif
}

/*
 * "byteidx()" function
 */
/*ARGSUSED*/
    static void
f_byteidx(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_MBYTE
    char_u	*t;
#endif
    char_u	*str;
    long	idx;

    str = get_tv_string_chk(&argvars[0]);
    idx = get_tv_number_chk(&argvars[1], NULL);
    rettv->vval.v_number = -1;
    if (str == NULL || idx < 0)
	return;

#ifdef FEAT_MBYTE
    t = str;
    for ( ; idx > 0; idx--)
    {
	if (*t == NUL)		/* EOL reached */
	    return;
	t += (*mb_ptr2len)(t);
    }
    rettv->vval.v_number = (varnumber_T)(t - str);
#else
    if ((size_t)idx <= STRLEN(str))
	rettv->vval.v_number = idx;
#endif
}

/*
 * "call(func, arglist)" function
 */
    static void
f_call(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*func;
    typval_T	argv[MAX_FUNC_ARGS + 1];
    int		argc = 0;
    listitem_T	*item;
    int		dummy;
    dict_T	*selfdict = NULL;

    rettv->vval.v_number = 0;
    if (argvars[1].v_type != VAR_LIST)
    {
	EMSG(_(e_listreq));
	return;
    }
    if (argvars[1].vval.v_list == NULL)
	return;

    if (argvars[0].v_type == VAR_FUNC)
	func = argvars[0].vval.v_string;
    else
	func = get_tv_string(&argvars[0]);
    if (*func == NUL)
	return;		/* type error or empty name */

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	if (argvars[2].v_type != VAR_DICT)
	{
	    EMSG(_(e_dictreq));
	    return;
	}
	selfdict = argvars[2].vval.v_dict;
    }

    for (item = argvars[1].vval.v_list->lv_first; item != NULL;
							 item = item->li_next)
    {
	if (argc == MAX_FUNC_ARGS)
	{
	    EMSG(_("E699: Too many arguments"));
	    break;
	}
	/* Make a copy of each argument.  This is needed to be able to set
	 * v_lock to VAR_FIXED in the copy without changing the original list.
	 */
	copy_tv(&item->li_tv, &argv[argc++]);
    }

    if (item == NULL)
	(void)call_func(func, (int)STRLEN(func), rettv, argc, argv,
				 curwin->w_cursor.lnum, curwin->w_cursor.lnum,
						      &dummy, TRUE, selfdict);

    /* Free the arguments. */
    while (argc > 0)
	clear_tv(&argv[--argc]);
}

#ifdef FEAT_FLOAT
/*
 * "ceil({float})" function
 */
    static void
f_ceil(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = ceil(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "changenr()" function
 */
/*ARGSUSED*/
    static void
f_changenr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = curbuf->b_u_seq_cur;
}

/*
 * "char2nr(string)" function
 */
    static void
f_char2nr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_MBYTE
    if (has_mbyte)
	rettv->vval.v_number = (*mb_ptr2char)(get_tv_string(&argvars[0]));
    else
#endif
    rettv->vval.v_number = get_tv_string(&argvars[0])[0];
}

/*
 * "cindent(lnum)" function
 */
    static void
f_cindent(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_CINDENT
    pos_T	pos;
    linenr_T	lnum;

    pos = curwin->w_cursor;
    lnum = get_tv_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
	curwin->w_cursor.lnum = lnum;
	rettv->vval.v_number = get_c_indent();
	curwin->w_cursor = pos;
    }
    else
#endif
	rettv->vval.v_number = -1;
}

/*
 * "clearmatches()" function
 */
/*ARGSUSED*/
    static void
f_clearmatches(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_SEARCH_EXTRA
    clear_matches(curwin);
#endif
}

/*
 * "col(string)" function
 */
    static void
f_col(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    colnr_T	col = 0;
    pos_T	*fp;
    int		fnum = curbuf->b_fnum;

    fp = var2fpos(&argvars[0], FALSE, &fnum);
    if (fp != NULL && fnum == curbuf->b_fnum)
    {
	if (fp->col == MAXCOL)
	{
	    /* '> can be MAXCOL, get the length of the line then */
	    if (fp->lnum <= curbuf->b_ml.ml_line_count)
		col = (colnr_T)STRLEN(ml_get(fp->lnum)) + 1;
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

		    if (*p != NUL && p[(l = (*mb_ptr2len)(p))] == NUL)
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
    rettv->vval.v_number = col;
}

#if defined(FEAT_INS_EXPAND)
/*
 * "complete()" function
 */
/*ARGSUSED*/
    static void
f_complete(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int	    startcol;

    if ((State & INSERT) == 0)
    {
	EMSG(_("E785: complete() can only be used in Insert mode"));
	return;
    }

    /* Check for undo allowed here, because if something was already inserted
     * the line was already saved for undo and this check isn't done. */
    if (!undo_allowed())
	return;

    if (argvars[1].v_type != VAR_LIST || argvars[1].vval.v_list == NULL)
    {
	EMSG(_(e_invarg));
	return;
    }

    startcol = get_tv_number_chk(&argvars[0], NULL);
    if (startcol <= 0)
	return;

    set_completion(startcol - 1, argvars[1].vval.v_list);
}

/*
 * "complete_add()" function
 */
/*ARGSUSED*/
    static void
f_complete_add(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = ins_compl_add_tv(&argvars[0], 0);
}

/*
 * "complete_check()" function
 */
/*ARGSUSED*/
    static void
f_complete_check(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		saved = RedrawingDisabled;

    RedrawingDisabled = 0;
    ins_compl_check_keys(0);
    rettv->vval.v_number = compl_interrupted;
    RedrawingDisabled = saved;
}
#endif

/*
 * "confirm(message, buttons[, default [, type]])" function
 */
/*ARGSUSED*/
    static void
f_confirm(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
    char_u	*message;
    char_u	*buttons = NULL;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    int		def = 1;
    int		type = VIM_GENERIC;
    char_u	*typestr;
    int		error = FALSE;

    message = get_tv_string_chk(&argvars[0]);
    if (message == NULL)
	error = TRUE;
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	buttons = get_tv_string_buf_chk(&argvars[1], buf);
	if (buttons == NULL)
	    error = TRUE;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    def = get_tv_number_chk(&argvars[2], &error);
	    if (argvars[3].v_type != VAR_UNKNOWN)
	    {
		typestr = get_tv_string_buf_chk(&argvars[3], buf2);
		if (typestr == NULL)
		    error = TRUE;
		else
		{
		    switch (TOUPPER_ASC(*typestr))
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
    }

    if (buttons == NULL || *buttons == NUL)
	buttons = (char_u *)_("&Ok");

    if (error)
	rettv->vval.v_number = 0;
    else
	rettv->vval.v_number = do_dialog(type, NULL, message, buttons,
								   def, NULL);
#else
    rettv->vval.v_number = 0;
#endif
}

/*
 * "copy()" function
 */
    static void
f_copy(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    item_copy(&argvars[0], rettv, FALSE, 0);
}

#ifdef FEAT_FLOAT
/*
 * "cos()" function
 */
    static void
f_cos(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = cos(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "count()" function
 */
    static void
f_count(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    long	n = 0;
    int		ic = FALSE;

    if (argvars[0].v_type == VAR_LIST)
    {
	listitem_T	*li;
	list_T		*l;
	long		idx;

	if ((l = argvars[0].vval.v_list) != NULL)
	{
	    li = l->lv_first;
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		int error = FALSE;

		ic = get_tv_number_chk(&argvars[2], &error);
		if (argvars[3].v_type != VAR_UNKNOWN)
		{
		    idx = get_tv_number_chk(&argvars[3], &error);
		    if (!error)
		    {
			li = list_find(l, idx);
			if (li == NULL)
			    EMSGN(_(e_listidx), idx);
		    }
		}
		if (error)
		    li = NULL;
	    }

	    for ( ; li != NULL; li = li->li_next)
		if (tv_equal(&li->li_tv, &argvars[1], ic))
		    ++n;
	}
    }
    else if (argvars[0].v_type == VAR_DICT)
    {
	int		todo;
	dict_T		*d;
	hashitem_T	*hi;

	if ((d = argvars[0].vval.v_dict) != NULL)
	{
	    int error = FALSE;

	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		ic = get_tv_number_chk(&argvars[2], &error);
		if (argvars[3].v_type != VAR_UNKNOWN)
		    EMSG(_(e_invarg));
	    }

	    todo = error ? 0 : (int)d->dv_hashtab.ht_used;
	    for (hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;
		    if (tv_equal(&HI2DI(hi)->di_tv, &argvars[1], ic))
			++n;
		}
	    }
	}
    }
    else
	EMSG2(_(e_listdictarg), "count()");
    rettv->vval.v_number = n;
}

/*
 * "cscope_connection([{num} , {dbpath} [, {prepend}]])" function
 *
 * Checks the existence of a cscope connection.
 */
/*ARGSUSED*/
    static void
f_cscope_connection(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_CSCOPE
    int		num = 0;
    char_u	*dbpath = NULL;
    char_u	*prepend = NULL;
    char_u	buf[NUMBUFLEN];

    if (argvars[0].v_type != VAR_UNKNOWN
	    && argvars[1].v_type != VAR_UNKNOWN)
    {
	num = (int)get_tv_number(&argvars[0]);
	dbpath = get_tv_string(&argvars[1]);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    prepend = get_tv_string_buf(&argvars[2], buf);
    }

    rettv->vval.v_number = cs_connection(num, dbpath, prepend);
#else
    rettv->vval.v_number = 0;
#endif
}

/*
 * "cursor(lnum, col)" function
 *
 * Moves the cursor to the specified line and column
 */
/*ARGSUSED*/
    static void
f_cursor(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    long	line, col;
#ifdef FEAT_VIRTUALEDIT
    long	coladd = 0;
#endif

    if (argvars[1].v_type == VAR_UNKNOWN)
    {
	pos_T	    pos;

	if (list2fpos(argvars, &pos, NULL) == FAIL)
	    return;
	line = pos.lnum;
	col = pos.col;
#ifdef FEAT_VIRTUALEDIT
	coladd = pos.coladd;
#endif
    }
    else
    {
	line = get_tv_lnum(argvars);
	col = get_tv_number_chk(&argvars[1], NULL);
#ifdef FEAT_VIRTUALEDIT
	if (argvars[2].v_type != VAR_UNKNOWN)
	    coladd = get_tv_number_chk(&argvars[2], NULL);
#endif
    }
    if (line < 0 || col < 0
#ifdef FEAT_VIRTUALEDIT
			    || coladd < 0
#endif
	    )
	return;		/* type error; errmsg already given */
    if (line > 0)
	curwin->w_cursor.lnum = line;
    if (col > 0)
	curwin->w_cursor.col = col - 1;
#ifdef FEAT_VIRTUALEDIT
    curwin->w_cursor.coladd = coladd;
#endif

    /* Make sure the cursor is in a valid position. */
    check_cursor();
#ifdef FEAT_MBYTE
    /* Correct cursor for multi-byte character. */
    if (has_mbyte)
	mb_adjust_cursor();
#endif

    curwin->w_set_curswant = TRUE;
}

/*
 * "deepcopy()" function
 */
    static void
f_deepcopy(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		noref = 0;

    if (argvars[1].v_type != VAR_UNKNOWN)
	noref = get_tv_number_chk(&argvars[1], NULL);
    if (noref < 0 || noref > 1)
	EMSG(_(e_invarg));
    else
	item_copy(&argvars[0], rettv, TRUE, noref == 0 ? ++current_copyID : 0);
}

/*
 * "delete()" function
 */
    static void
f_delete(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    if (check_restricted() || check_secure())
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = mch_remove(get_tv_string(&argvars[0]));
}

/*
 * "did_filetype()" function
 */
/*ARGSUSED*/
    static void
f_did_filetype(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_AUTOCMD
    rettv->vval.v_number = did_filetype;
#else
    rettv->vval.v_number = 0;
#endif
}

/*
 * "diff_filler()" function
 */
/*ARGSUSED*/
    static void
f_diff_filler(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_DIFF
    rettv->vval.v_number = diff_check_fill(curwin, get_tv_lnum(argvars));
#endif
}

/*
 * "diff_hlID()" function
 */
/*ARGSUSED*/
    static void
f_diff_hlID(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_DIFF
    linenr_T		lnum = get_tv_lnum(argvars);
    static linenr_T	prev_lnum = 0;
    static int		changedtick = 0;
    static int		fnum = 0;
    static int		change_start = 0;
    static int		change_end = 0;
    static hlf_T	hlID = (hlf_T)0;
    int			filler_lines;
    int			col;

    if (lnum < 0)	/* ignore type error in {lnum} arg */
	lnum = 0;
    if (lnum != prev_lnum
	    || changedtick != curbuf->b_changedtick
	    || fnum != curbuf->b_fnum)
    {
	/* New line, buffer, change: need to get the values. */
	filler_lines = diff_check(curwin, lnum);
	if (filler_lines < 0)
	{
	    if (filler_lines == -1)
	    {
		change_start = MAXCOL;
		change_end = -1;
		if (diff_find_change(curwin, lnum, &change_start, &change_end))
		    hlID = HLF_ADD;	/* added line */
		else
		    hlID = HLF_CHD;	/* changed line */
	    }
	    else
		hlID = HLF_ADD;	/* added line */
	}
	else
	    hlID = (hlf_T)0;
	prev_lnum = lnum;
	changedtick = curbuf->b_changedtick;
	fnum = curbuf->b_fnum;
    }

    if (hlID == HLF_CHD || hlID == HLF_TXD)
    {
	col = get_tv_number(&argvars[1]) - 1; /* ignore type error in {col} */
	if (col >= change_start && col <= change_end)
	    hlID = HLF_TXD;			/* changed text */
	else
	    hlID = HLF_CHD;			/* changed line */
    }
    rettv->vval.v_number = hlID == (hlf_T)0 ? 0 : (int)hlID;
#endif
}

/*
 * "empty({expr})" function
 */
    static void
f_empty(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		n;

    switch (argvars[0].v_type)
    {
	case VAR_STRING:
	case VAR_FUNC:
	    n = argvars[0].vval.v_string == NULL
					  || *argvars[0].vval.v_string == NUL;
	    break;
	case VAR_NUMBER:
	    n = argvars[0].vval.v_number == 0;
	    break;
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    n = argvars[0].vval.v_float == 0.0;
	    break;
#endif
	case VAR_LIST:
	    n = argvars[0].vval.v_list == NULL
				  || argvars[0].vval.v_list->lv_first == NULL;
	    break;
	case VAR_DICT:
	    n = argvars[0].vval.v_dict == NULL
			|| argvars[0].vval.v_dict->dv_hashtab.ht_used == 0;
	    break;
	default:
	    EMSG2(_(e_intern2), "f_empty()");
	    n = 0;
    }

    rettv->vval.v_number = n;
}

/*
 * "escape({string}, {chars})" function
 */
    static void
f_escape(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	buf[NUMBUFLEN];

    rettv->vval.v_string = vim_strsave_escaped(get_tv_string(&argvars[0]),
					 get_tv_string_buf(&argvars[1], buf));
    rettv->v_type = VAR_STRING;
}

/*
 * "eval()" function
 */
/*ARGSUSED*/
    static void
f_eval(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*s;

    s = get_tv_string_chk(&argvars[0]);
    if (s != NULL)
	s = skipwhite(s);

    if (s == NULL || eval1(&s, rettv, TRUE) == FAIL)
    {
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = 0;
    }
    else if (*s != NUL)
	EMSG(_(e_trailing));
}

/*
 * "eventhandler()" function
 */
/*ARGSUSED*/
    static void
f_eventhandler(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = vgetc_busy;
}

/*
 * "executable()" function
 */
    static void
f_executable(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = mch_can_exe(get_tv_string(&argvars[0]));
}

/*
 * "exists()" function
 */
    static void
f_exists(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p;
    char_u	*name;
    int		n = FALSE;
    int		len = 0;

    p = get_tv_string(&argvars[0]);
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
    {
	n = (get_option_tv(&p, NULL, TRUE) == OK);
	if (*skipwhite(p) != NUL)
	    n = FALSE;			/* trailing garbage */
    }
    else if (*p == '*')			/* internal or user defined function */
    {
	n = function_exists(p + 1);
    }
    else if (*p == ':')
    {
	n = cmd_exists(p + 1);
    }
    else if (*p == '#')
    {
#ifdef FEAT_AUTOCMD
	if (p[1] == '#')
	    n = autocmd_supported(p + 2);
	else
	    n = au_exists(p + 1);
#endif
    }
    else				/* internal variable */
    {
	char_u	    *tofree;
	typval_T    tv;

	/* get_name_len() takes care of expanding curly braces */
	name = p;
	len = get_name_len(&p, &tofree, TRUE, FALSE);
	if (len > 0)
	{
	    if (tofree != NULL)
		name = tofree;
	    n = (get_var_tv(name, len, &tv, FALSE) == OK);
	    if (n)
	    {
		/* handle d.key, l[idx], f(expr) */
		n = (handle_subscript(&p, &tv, TRUE, FALSE) == OK);
		if (n)
		    clear_tv(&tv);
	    }
	}
	if (*p != NUL)
	    n = FALSE;

	vim_free(tofree);
    }

    rettv->vval.v_number = n;
}

/*
 * "expand()" function
 */
    static void
f_expand(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*s;
    int		len;
    char_u	*errormsg;
    int		flags = WILD_SILENT|WILD_USE_NL|WILD_LIST_NOTFOUND;
    expand_T	xpc;
    int		error = FALSE;

    rettv->v_type = VAR_STRING;
    s = get_tv_string(&argvars[0]);
    if (*s == '%' || *s == '#' || *s == '<')
    {
	++emsg_off;
	rettv->vval.v_string = eval_vars(s, s, &len, NULL, &errormsg, NULL);
	--emsg_off;
    }
    else
    {
	/* When the optional second argument is non-zero, don't remove matches
	 * for 'wildignore' and don't put matches for 'suffixes' at the end. */
	if (argvars[1].v_type != VAR_UNKNOWN
				    && get_tv_number_chk(&argvars[1], &error))
	    flags |= WILD_KEEP_ALL;
	if (!error)
	{
	    ExpandInit(&xpc);
	    xpc.xp_context = EXPAND_FILES;
	    rettv->vval.v_string = ExpandOne(&xpc, s, NULL, flags, WILD_ALL);
	}
	else
	    rettv->vval.v_string = NULL;
    }
}

/*
 * "extend(list, list [, idx])" function
 * "extend(dict, dict [, action])" function
 */
    static void
f_extend(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = 0;
    if (argvars[0].v_type == VAR_LIST && argvars[1].v_type == VAR_LIST)
    {
	list_T		*l1, *l2;
	listitem_T	*item;
	long		before;
	int		error = FALSE;

	l1 = argvars[0].vval.v_list;
	l2 = argvars[1].vval.v_list;
	if (l1 != NULL && !tv_check_lock(l1->lv_lock, (char_u *)"extend()")
		&& l2 != NULL)
	{
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		before = get_tv_number_chk(&argvars[2], &error);
		if (error)
		    return;		/* type error; errmsg already given */

		if (before == l1->lv_len)
		    item = NULL;
		else
		{
		    item = list_find(l1, before);
		    if (item == NULL)
		    {
			EMSGN(_(e_listidx), before);
			return;
		    }
		}
	    }
	    else
		item = NULL;
	    list_extend(l1, l2, item);

	    copy_tv(&argvars[0], rettv);
	}
    }
    else if (argvars[0].v_type == VAR_DICT && argvars[1].v_type == VAR_DICT)
    {
	dict_T		*d1, *d2;
	dictitem_T	*di1;
	char_u		*action;
	int		i;
	hashitem_T	*hi2;
	int		todo;

	d1 = argvars[0].vval.v_dict;
	d2 = argvars[1].vval.v_dict;
	if (d1 != NULL && !tv_check_lock(d1->dv_lock, (char_u *)"extend()")
		&& d2 != NULL)
	{
	    /* Check the third argument. */
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		static char *(av[]) = {"keep", "force", "error"};

		action = get_tv_string_chk(&argvars[2]);
		if (action == NULL)
		    return;		/* type error; errmsg already given */
		for (i = 0; i < 3; ++i)
		    if (STRCMP(action, av[i]) == 0)
			break;
		if (i == 3)
		{
		    EMSG2(_(e_invarg2), action);
		    return;
		}
	    }
	    else
		action = (char_u *)"force";

	    /* Go over all entries in the second dict and add them to the
	     * first dict. */
	    todo = (int)d2->dv_hashtab.ht_used;
	    for (hi2 = d2->dv_hashtab.ht_array; todo > 0; ++hi2)
	    {
		if (!HASHITEM_EMPTY(hi2))
		{
		    --todo;
		    di1 = dict_find(d1, hi2->hi_key, -1);
		    if (di1 == NULL)
		    {
			di1 = dictitem_copy(HI2DI(hi2));
			if (di1 != NULL && dict_add(d1, di1) == FAIL)
			    dictitem_free(di1);
		    }
		    else if (*action == 'e')
		    {
			EMSG2(_("E737: Key already exists: %s"), hi2->hi_key);
			break;
		    }
		    else if (*action == 'f')
		    {
			clear_tv(&di1->di_tv);
			copy_tv(&HI2DI(hi2)->di_tv, &di1->di_tv);
		    }
		}
	    }

	    copy_tv(&argvars[0], rettv);
	}
    }
    else
	EMSG2(_(e_listdictarg), "extend()");
}

/*
 * "feedkeys()" function
 */
/*ARGSUSED*/
    static void
f_feedkeys(argvars, rettv)
    typval_T    *argvars;
    typval_T    *rettv;
{
    int		remap = TRUE;
    char_u	*keys, *flags;
    char_u	nbuf[NUMBUFLEN];
    int		typed = FALSE;
    char_u	*keys_esc;

    /* This is not allowed in the sandbox.  If the commands would still be
     * executed in the sandbox it would be OK, but it probably happens later,
     * when "sandbox" is no longer set. */
    if (check_secure())
	return;

    rettv->vval.v_number = 0;
    keys = get_tv_string(&argvars[0]);
    if (*keys != NUL)
    {
	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    flags = get_tv_string_buf(&argvars[1], nbuf);
	    for ( ; *flags != NUL; ++flags)
	    {
		switch (*flags)
		{
		    case 'n': remap = FALSE; break;
		    case 'm': remap = TRUE; break;
		    case 't': typed = TRUE; break;
		}
	    }
	}

	/* Need to escape K_SPECIAL and CSI before putting the string in the
	 * typeahead buffer. */
	keys_esc = vim_strsave_escape_csi(keys);
	if (keys_esc != NULL)
	{
	    ins_typebuf(keys_esc, (remap ? REMAP_YES : REMAP_NONE),
					       typebuf.tb_len, !typed, FALSE);
	    vim_free(keys_esc);
	    if (vgetc_busy)
		typebuf_was_filled = TRUE;
	}
    }
}

/*
 * "filereadable()" function
 */
    static void
f_filereadable(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		fd;
    char_u	*p;
    int		n;

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif
    p = get_tv_string(&argvars[0]);
    if (*p && !mch_isdir(p) && (fd = mch_open((char *)p,
					      O_RDONLY | O_NONBLOCK, 0)) >= 0)
    {
	n = TRUE;
	close(fd);
    }
    else
	n = FALSE;

    rettv->vval.v_number = n;
}

/*
 * Return 0 for not writable, 1 for writable file, 2 for a dir which we have
 * rights to write into.
 */
    static void
f_filewritable(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = filewritable(get_tv_string(&argvars[0]));
}

static void findfilendir __ARGS((typval_T *argvars, typval_T *rettv, int find_what));

    static void
findfilendir(argvars, rettv, find_what)
    typval_T	*argvars;
    typval_T	*rettv;
    int		find_what;
{
#ifdef FEAT_SEARCHPATH
    char_u	*fname;
    char_u	*fresult = NULL;
    char_u	*path = *curbuf->b_p_path == NUL ? p_path : curbuf->b_p_path;
    char_u	*p;
    char_u	pathbuf[NUMBUFLEN];
    int		count = 1;
    int		first = TRUE;
    int		error = FALSE;
#endif

    rettv->vval.v_string = NULL;
    rettv->v_type = VAR_STRING;

#ifdef FEAT_SEARCHPATH
    fname = get_tv_string(&argvars[0]);

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	p = get_tv_string_buf_chk(&argvars[1], pathbuf);
	if (p == NULL)
	    error = TRUE;
	else
	{
	    if (*p != NUL)
		path = p;

	    if (argvars[2].v_type != VAR_UNKNOWN)
		count = get_tv_number_chk(&argvars[2], &error);
	}
    }

    if (count < 0 && rettv_list_alloc(rettv) == FAIL)
	error = TRUE;

    if (*fname != NUL && !error)
    {
	do
	{
	    if (rettv->v_type == VAR_STRING)
		vim_free(fresult);
	    fresult = find_file_in_path_option(first ? fname : NULL,
					       first ? (int)STRLEN(fname) : 0,
					0, first, path,
					find_what,
					curbuf->b_ffname,
					find_what == FINDFILE_DIR
					    ? (char_u *)"" : curbuf->b_p_sua);
	    first = FALSE;

	    if (fresult != NULL && rettv->v_type == VAR_LIST)
		list_append_string(rettv->vval.v_list, fresult, -1);

	} while ((rettv->v_type == VAR_LIST || --count > 0) && fresult != NULL);
    }

    if (rettv->v_type == VAR_STRING)
	rettv->vval.v_string = fresult;
#endif
}

static void filter_map __ARGS((typval_T *argvars, typval_T *rettv, int map));
static int filter_map_one __ARGS((typval_T *tv, char_u *expr, int map, int *remp));

/*
 * Implementation of map() and filter().
 */
    static void
filter_map(argvars, rettv, map)
    typval_T	*argvars;
    typval_T	*rettv;
    int		map;
{
    char_u	buf[NUMBUFLEN];
    char_u	*expr;
    listitem_T	*li, *nli;
    list_T	*l = NULL;
    dictitem_T	*di;
    hashtab_T	*ht;
    hashitem_T	*hi;
    dict_T	*d = NULL;
    typval_T	save_val;
    typval_T	save_key;
    int		rem;
    int		todo;
    char_u	*ermsg = map ? (char_u *)"map()" : (char_u *)"filter()";
    int		save_did_emsg;

    rettv->vval.v_number = 0;
    if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) == NULL
		|| (map && tv_check_lock(l->lv_lock, ermsg)))
	    return;
    }
    else if (argvars[0].v_type == VAR_DICT)
    {
	if ((d = argvars[0].vval.v_dict) == NULL
		|| (map && tv_check_lock(d->dv_lock, ermsg)))
	    return;
    }
    else
    {
	EMSG2(_(e_listdictarg), ermsg);
	return;
    }

    expr = get_tv_string_buf_chk(&argvars[1], buf);
    /* On type errors, the preceding call has already displayed an error
     * message.  Avoid a misleading error message for an empty string that
     * was not passed as argument. */
    if (expr != NULL)
    {
	prepare_vimvar(VV_VAL, &save_val);
	expr = skipwhite(expr);

	/* We reset "did_emsg" to be able to detect whether an error
	 * occurred during evaluation of the expression. */
	save_did_emsg = did_emsg;
	did_emsg = FALSE;

	if (argvars[0].v_type == VAR_DICT)
	{
	    prepare_vimvar(VV_KEY, &save_key);
	    vimvars[VV_KEY].vv_type = VAR_STRING;

	    ht = &d->dv_hashtab;
	    hash_lock(ht);
	    todo = (int)ht->ht_used;
	    for (hi = ht->ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;
		    di = HI2DI(hi);
		    if (tv_check_lock(di->di_tv.v_lock, ermsg))
			break;
		    vimvars[VV_KEY].vv_str = vim_strsave(di->di_key);
		    if (filter_map_one(&di->di_tv, expr, map, &rem) == FAIL
								  || did_emsg)
			break;
		    if (!map && rem)
			dictitem_remove(d, di);
		    clear_tv(&vimvars[VV_KEY].vv_tv);
		}
	    }
	    hash_unlock(ht);

	    restore_vimvar(VV_KEY, &save_key);
	}
	else
	{
	    for (li = l->lv_first; li != NULL; li = nli)
	    {
		if (tv_check_lock(li->li_tv.v_lock, ermsg))
		    break;
		nli = li->li_next;
		if (filter_map_one(&li->li_tv, expr, map, &rem) == FAIL
								  || did_emsg)
		    break;
		if (!map && rem)
		    listitem_remove(l, li);
	    }
	}

	restore_vimvar(VV_VAL, &save_val);

	did_emsg |= save_did_emsg;
    }

    copy_tv(&argvars[0], rettv);
}

    static int
filter_map_one(tv, expr, map, remp)
    typval_T	*tv;
    char_u	*expr;
    int		map;
    int		*remp;
{
    typval_T	rettv;
    char_u	*s;
    int		retval = FAIL;

    copy_tv(tv, &vimvars[VV_VAL].vv_tv);
    s = expr;
    if (eval1(&s, &rettv, TRUE) == FAIL)
	goto theend;
    if (*s != NUL)  /* check for trailing chars after expr */
    {
	EMSG2(_(e_invexpr2), s);
	goto theend;
    }
    if (map)
    {
	/* map(): replace the list item value */
	clear_tv(tv);
	rettv.v_lock = 0;
	*tv = rettv;
    }
    else
    {
	int	    error = FALSE;

	/* filter(): when expr is zero remove the item */
	*remp = (get_tv_number_chk(&rettv, &error) == 0);
	clear_tv(&rettv);
	/* On type error, nothing has been removed; return FAIL to stop the
	 * loop.  The error message was given by get_tv_number_chk(). */
	if (error)
	    goto theend;
    }
    retval = OK;
theend:
    clear_tv(&vimvars[VV_VAL].vv_tv);
    return retval;
}

/*
 * "filter()" function
 */
    static void
f_filter(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    filter_map(argvars, rettv, FALSE);
}

/*
 * "finddir({fname}[, {path}[, {count}]])" function
 */
    static void
f_finddir(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    findfilendir(argvars, rettv, FINDFILE_DIR);
}

/*
 * "findfile({fname}[, {path}[, {count}]])" function
 */
    static void
f_findfile(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    findfilendir(argvars, rettv, FINDFILE_FILE);
}

#ifdef FEAT_FLOAT
/*
 * "float2nr({float})" function
 */
    static void
f_float2nr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    if (get_float_arg(argvars, &f) == OK)
    {
	if (f < -0x7fffffff)
	    rettv->vval.v_number = -0x7fffffff;
	else if (f > 0x7fffffff)
	    rettv->vval.v_number = 0x7fffffff;
	else
	    rettv->vval.v_number = (varnumber_T)f;
    }
    else
	rettv->vval.v_number = 0;
}

/*
 * "floor({float})" function
 */
    static void
f_floor(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = floor(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "fnameescape({string})" function
 */
    static void
f_fnameescape(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_string = vim_strsave_fnameescape(
					   get_tv_string(&argvars[0]), FALSE);
    rettv->v_type = VAR_STRING;
}

/*
 * "fnamemodify({fname}, {mods})" function
 */
    static void
f_fnamemodify(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*fname;
    char_u	*mods;
    int		usedlen = 0;
    int		len;
    char_u	*fbuf = NULL;
    char_u	buf[NUMBUFLEN];

    fname = get_tv_string_chk(&argvars[0]);
    mods = get_tv_string_buf_chk(&argvars[1], buf);
    if (fname == NULL || mods == NULL)
	fname = NULL;
    else
    {
	len = (int)STRLEN(fname);
	(void)modify_fname(mods, &usedlen, &fname, &fbuf, &len);
    }

    rettv->v_type = VAR_STRING;
    if (fname == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = vim_strnsave(fname, len);
    vim_free(fbuf);
}

static void foldclosed_both __ARGS((typval_T *argvars, typval_T *rettv, int end));

/*
 * "foldclosed()" function
 */
    static void
foldclosed_both(argvars, rettv, end)
    typval_T	*argvars;
    typval_T	*rettv;
    int		end;
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;
    linenr_T	first, last;

    lnum = get_tv_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
	if (hasFoldingWin(curwin, lnum, &first, &last, FALSE, NULL))
	{
	    if (end)
		rettv->vval.v_number = (varnumber_T)last;
	    else
		rettv->vval.v_number = (varnumber_T)first;
	    return;
	}
    }
#endif
    rettv->vval.v_number = -1;
}

/*
 * "foldclosed()" function
 */
    static void
f_foldclosed(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    foldclosed_both(argvars, rettv, FALSE);
}

/*
 * "foldclosedend()" function
 */
    static void
f_foldclosedend(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    foldclosed_both(argvars, rettv, TRUE);
}

/*
 * "foldlevel()" function
 */
    static void
f_foldlevel(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;

    lnum = get_tv_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	rettv->vval.v_number = foldLevel(lnum);
    else
#endif
	rettv->vval.v_number = 0;
}

/*
 * "foldtext()" function
 */
/*ARGSUSED*/
    static void
f_foldtext(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;
    char_u	*s;
    char_u	*r;
    int		len;
    char	*txt;
#endif

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_FOLDING
    if ((linenr_T)vimvars[VV_FOLDSTART].vv_nr > 0
	    && (linenr_T)vimvars[VV_FOLDEND].vv_nr
						 <= curbuf->b_ml.ml_line_count
	    && vimvars[VV_FOLDDASHES].vv_str != NULL)
    {
	/* Find first non-empty line in the fold. */
	lnum = (linenr_T)vimvars[VV_FOLDSTART].vv_nr;
	while (lnum < (linenr_T)vimvars[VV_FOLDEND].vv_nr)
	{
	    if (!linewhite(lnum))
		break;
	    ++lnum;
	}

	/* Find interesting text in this line. */
	s = skipwhite(ml_get(lnum));
	/* skip C comment-start */
	if (s[0] == '/' && (s[1] == '*' || s[1] == '/'))
	{
	    s = skipwhite(s + 2);
	    if (*skipwhite(s) == NUL
			    && lnum + 1 < (linenr_T)vimvars[VV_FOLDEND].vv_nr)
	    {
		s = skipwhite(ml_get(lnum + 1));
		if (*s == '*')
		    s = skipwhite(s + 1);
	    }
	}
	txt = _("+-%s%3ld lines: ");
	r = alloc((unsigned)(STRLEN(txt)
		    + STRLEN(vimvars[VV_FOLDDASHES].vv_str)    /* for %s */
		    + 20				    /* for %3ld */
		    + STRLEN(s)));			    /* concatenated */
	if (r != NULL)
	{
	    sprintf((char *)r, txt, vimvars[VV_FOLDDASHES].vv_str,
		    (long)((linenr_T)vimvars[VV_FOLDEND].vv_nr
				- (linenr_T)vimvars[VV_FOLDSTART].vv_nr + 1));
	    len = (int)STRLEN(r);
	    STRCAT(r, s);
	    /* remove 'foldmarker' and 'commentstring' */
	    foldtext_cleanup(r + len);
	    rettv->vval.v_string = r;
	}
    }
#endif
}

/*
 * "foldtextresult(lnum)" function
 */
/*ARGSUSED*/
    static void
f_foldtextresult(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;
    char_u	*text;
    char_u	buf[51];
    foldinfo_T  foldinfo;
    int		fold_count;
#endif

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_FOLDING
    lnum = get_tv_lnum(argvars);
    /* treat illegal types and illegal string values for {lnum} the same */
    if (lnum < 0)
	lnum = 0;
    fold_count = foldedCount(curwin, lnum, &foldinfo);
    if (fold_count > 0)
    {
	text = get_foldtext(curwin, lnum, lnum + fold_count - 1,
							      &foldinfo, buf);
	if (text == buf)
	    text = vim_strsave(text);
	rettv->vval.v_string = text;
    }
#endif
}

/*
 * "foreground()" function
 */
/*ARGSUSED*/
    static void
f_foreground(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = 0;
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
 * "function()" function
 */
/*ARGSUSED*/
    static void
f_function(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*s;

    rettv->vval.v_number = 0;
    s = get_tv_string(&argvars[0]);
    if (s == NULL || *s == NUL || VIM_ISDIGIT(*s))
	EMSG2(_(e_invarg2), s);
    /* Don't check an autoload name for existence here. */
    else if (vim_strchr(s, AUTOLOAD_CHAR) == NULL && !function_exists(s))
	EMSG2(_("E700: Unknown function: %s"), s);
    else
    {
	rettv->vval.v_string = vim_strsave(s);
	rettv->v_type = VAR_FUNC;
    }
}

/*
 * "garbagecollect()" function
 */
/*ARGSUSED*/
    static void
f_garbagecollect(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    /* This is postponed until we are back at the toplevel, because we may be
     * using Lists and Dicts internally.  E.g.: ":echo [garbagecollect()]". */
    want_garbage_collect = TRUE;

    if (argvars[0].v_type != VAR_UNKNOWN && get_tv_number(&argvars[0]) == 1)
	garbage_collect_at_exit = TRUE;
}

/*
 * "get()" function
 */
    static void
f_get(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    listitem_T	*li;
    list_T	*l;
    dictitem_T	*di;
    dict_T	*d;
    typval_T	*tv = NULL;

    if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) != NULL)
	{
	    int		error = FALSE;

	    li = list_find(l, get_tv_number_chk(&argvars[1], &error));
	    if (!error && li != NULL)
		tv = &li->li_tv;
	}
    }
    else if (argvars[0].v_type == VAR_DICT)
    {
	if ((d = argvars[0].vval.v_dict) != NULL)
	{
	    di = dict_find(d, get_tv_string(&argvars[1]), -1);
	    if (di != NULL)
		tv = &di->di_tv;
	}
    }
    else
	EMSG2(_(e_listdictarg), "get()");

    if (tv == NULL)
    {
	if (argvars[2].v_type == VAR_UNKNOWN)
	    rettv->vval.v_number = 0;
	else
	    copy_tv(&argvars[2], rettv);
    }
    else
	copy_tv(tv, rettv);
}

static void get_buffer_lines __ARGS((buf_T *buf, linenr_T start, linenr_T end, int retlist, typval_T *rettv));

/*
 * Get line or list of lines from buffer "buf" into "rettv".
 * Return a range (from start to end) of lines in rettv from the specified
 * buffer.
 * If 'retlist' is TRUE, then the lines are returned as a Vim List.
 */
    static void
get_buffer_lines(buf, start, end, retlist, rettv)
    buf_T	*buf;
    linenr_T	start;
    linenr_T	end;
    int		retlist;
    typval_T	*rettv;
{
    char_u	*p;

    if (retlist)
    {
	if (rettv_list_alloc(rettv) == FAIL)
	    return;
    }
    else
	rettv->vval.v_number = 0;

    if (buf == NULL || buf->b_ml.ml_mfp == NULL || start < 0)
	return;

    if (!retlist)
    {
	if (start >= 1 && start <= buf->b_ml.ml_line_count)
	    p = ml_get_buf(buf, start, FALSE);
	else
	    p = (char_u *)"";

	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = vim_strsave(p);
    }
    else
    {
	if (end < start)
	    return;

	if (start < 1)
	    start = 1;
	if (end > buf->b_ml.ml_line_count)
	    end = buf->b_ml.ml_line_count;
	while (start <= end)
	    if (list_append_string(rettv->vval.v_list,
				 ml_get_buf(buf, start++, FALSE), -1) == FAIL)
		break;
    }
}

/*
 * "getbufline()" function
 */
    static void
f_getbufline(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    linenr_T	lnum;
    linenr_T	end;
    buf_T	*buf;

    (void)get_tv_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = get_buf_tv(&argvars[0]);
    --emsg_off;

    lnum = get_tv_lnum_buf(&argvars[1], buf);
    if (argvars[2].v_type == VAR_UNKNOWN)
	end = lnum;
    else
	end = get_tv_lnum_buf(&argvars[2], buf);

    get_buffer_lines(buf, lnum, end, TRUE, rettv);
}

/*
 * "getbufvar()" function
 */
    static void
f_getbufvar(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    buf_T	*buf;
    buf_T	*save_curbuf;
    char_u	*varname;
    dictitem_T	*v;

    (void)get_tv_number(&argvars[0]);	    /* issue errmsg if type error */
    varname = get_tv_string_chk(&argvars[1]);
    ++emsg_off;
    buf = get_buf_tv(&argvars[0]);

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (buf != NULL && varname != NULL)
    {
	/* set curbuf to be our buf, temporarily */
	save_curbuf = curbuf;
	curbuf = buf;

	if (*varname == '&')	/* buffer-local-option */
	    get_option_tv(&varname, rettv, TRUE);
	else
	{
	    if (*varname == NUL)
		/* let getbufvar({nr}, "") return the "b:" dictionary.  The
		 * scope prefix before the NUL byte is required by
		 * find_var_in_ht(). */
		varname = (char_u *)"b:" + 2;
	    /* look up the variable */
	    v = find_var_in_ht(&curbuf->b_vars.dv_hashtab, varname, FALSE);
	    if (v != NULL)
		copy_tv(&v->di_tv, rettv);
	}

	/* restore previous notion of curbuf */
	curbuf = save_curbuf;
    }

    --emsg_off;
}

/*
 * "getchar()" function
 */
    static void
f_getchar(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    varnumber_T		n;
    int			error = FALSE;

    /* Position the cursor.  Needed after a message that ends in a space. */
    windgoto(msg_row, msg_col);

    ++no_mapping;
    ++allow_keys;
    for (;;)
    {
	if (argvars[0].v_type == VAR_UNKNOWN)
	    /* getchar(): blocking wait. */
	    n = safe_vgetc();
	else if (get_tv_number_chk(&argvars[0], &error) == 1)
	    /* getchar(1): only check if char avail */
	    n = vpeekc();
	else if (error || vpeekc() == NUL)
	    /* illegal argument or getchar(0) and no char avail: return zero */
	    n = 0;
	else
	    /* getchar(0) and char avail: return char */
	    n = safe_vgetc();
	if (n == K_IGNORE)
	    continue;
	break;
    }
    --no_mapping;
    --allow_keys;

    vimvars[VV_MOUSE_WIN].vv_nr = 0;
    vimvars[VV_MOUSE_LNUM].vv_nr = 0;
    vimvars[VV_MOUSE_COL].vv_nr = 0;

    rettv->vval.v_number = n;
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
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = vim_strsave(temp);

#ifdef FEAT_MOUSE
	if (n == K_LEFTMOUSE
		|| n == K_LEFTMOUSE_NM
		|| n == K_LEFTDRAG
		|| n == K_LEFTRELEASE
		|| n == K_LEFTRELEASE_NM
		|| n == K_MIDDLEMOUSE
		|| n == K_MIDDLEDRAG
		|| n == K_MIDDLERELEASE
		|| n == K_RIGHTMOUSE
		|| n == K_RIGHTDRAG
		|| n == K_RIGHTRELEASE
		|| n == K_X1MOUSE
		|| n == K_X1DRAG
		|| n == K_X1RELEASE
		|| n == K_X2MOUSE
		|| n == K_X2DRAG
		|| n == K_X2RELEASE
		|| n == K_MOUSEDOWN
		|| n == K_MOUSEUP)
	{
	    int		row = mouse_row;
	    int		col = mouse_col;
	    win_T	*win;
	    linenr_T	lnum;
# ifdef FEAT_WINDOWS
	    win_T	*wp;
# endif
	    int		winnr = 1;

	    if (row >= 0 && col >= 0)
	    {
		/* Find the window at the mouse coordinates and compute the
		 * text position. */
		win = mouse_find_win(&row, &col);
		(void)mouse_comp_pos(win, &row, &col, &lnum);
# ifdef FEAT_WINDOWS
		for (wp = firstwin; wp != win; wp = wp->w_next)
		    ++winnr;
# endif
		vimvars[VV_MOUSE_WIN].vv_nr = winnr;
		vimvars[VV_MOUSE_LNUM].vv_nr = lnum;
		vimvars[VV_MOUSE_COL].vv_nr = col + 1;
	    }
	}
#endif
    }
}

/*
 * "getcharmod()" function
 */
/*ARGSUSED*/
    static void
f_getcharmod(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = mod_mask;
}

/*
 * "getcmdline()" function
 */
/*ARGSUSED*/
    static void
f_getcmdline(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = get_cmdline_str();
}

/*
 * "getcmdpos()" function
 */
/*ARGSUSED*/
    static void
f_getcmdpos(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = get_cmdline_pos() + 1;
}

/*
 * "getcmdtype()" function
 */
/*ARGSUSED*/
    static void
f_getcmdtype(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = alloc(2);
    if (rettv->vval.v_string != NULL)
    {
	rettv->vval.v_string[0] = get_cmdline_type();
	rettv->vval.v_string[1] = NUL;
    }
}

/*
 * "getcwd()" function
 */
/*ARGSUSED*/
    static void
f_getcwd(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	cwd[MAXPATHL];

    rettv->v_type = VAR_STRING;
    if (mch_dirname(cwd, MAXPATHL) == FAIL)
	rettv->vval.v_string = NULL;
    else
    {
	rettv->vval.v_string = vim_strsave(cwd);
#ifdef BACKSLASH_IN_FILENAME
	if (rettv->vval.v_string != NULL)
	    slash_adjust(rettv->vval.v_string);
#endif
    }
}

/*
 * "getfontname()" function
 */
/*ARGSUSED*/
    static void
f_getfontname(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	GuiFont font;
	char_u	*name = NULL;

	if (argvars[0].v_type == VAR_UNKNOWN)
	{
	    /* Get the "Normal" font.  Either the name saved by
	     * hl_set_font_name() or from the font ID. */
	    font = gui.norm_font;
	    name = hl_get_font_name();
	}
	else
	{
	    name = get_tv_string(&argvars[0]);
	    if (STRCMP(name, "*") == 0)	    /* don't use font dialog */
		return;
	    font = gui_mch_get_font(name, FALSE);
	    if (font == NOFONT)
		return;	    /* Invalid font name, return empty string. */
	}
	rettv->vval.v_string = gui_mch_get_fontname(font, name);
	if (argvars[0].v_type != VAR_UNKNOWN)
	    gui_mch_free_font(font);
    }
#endif
}

/*
 * "getfperm({fname})" function
 */
    static void
f_getfperm(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*fname;
    struct stat st;
    char_u	*perm = NULL;
    char_u	flags[] = "rwx";
    int		i;

    fname = get_tv_string(&argvars[0]);

    rettv->v_type = VAR_STRING;
    if (mch_stat((char *)fname, &st) >= 0)
    {
	perm = vim_strsave((char_u *)"---------");
	if (perm != NULL)
	{
	    for (i = 0; i < 9; i++)
	    {
		if (st.st_mode & (1 << (8 - i)))
		    perm[i] = flags[i % 3];
	    }
	}
    }
    rettv->vval.v_string = perm;
}

/*
 * "getfsize({fname})" function
 */
    static void
f_getfsize(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*fname;
    struct stat	st;

    fname = get_tv_string(&argvars[0]);

    rettv->v_type = VAR_NUMBER;

    if (mch_stat((char *)fname, &st) >= 0)
    {
	if (mch_isdir(fname))
	    rettv->vval.v_number = 0;
	else
	{
	    rettv->vval.v_number = (varnumber_T)st.st_size;

	    /* non-perfect check for overflow */
	    if ((off_t)rettv->vval.v_number != (off_t)st.st_size)
		rettv->vval.v_number = -2;
	}
    }
    else
	  rettv->vval.v_number = -1;
}

/*
 * "getftime({fname})" function
 */
    static void
f_getftime(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*fname;
    struct stat	st;

    fname = get_tv_string(&argvars[0]);

    if (mch_stat((char *)fname, &st) >= 0)
	rettv->vval.v_number = (varnumber_T)st.st_mtime;
    else
	rettv->vval.v_number = -1;
}

/*
 * "getftype({fname})" function
 */
    static void
f_getftype(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*fname;
    struct stat st;
    char_u	*type = NULL;
    char	*t;

    fname = get_tv_string(&argvars[0]);

    rettv->v_type = VAR_STRING;
    if (mch_lstat((char *)fname, &st) >= 0)
    {
#ifdef S_ISREG
	if (S_ISREG(st.st_mode))
	    t = "file";
	else if (S_ISDIR(st.st_mode))
	    t = "dir";
# ifdef S_ISLNK
	else if (S_ISLNK(st.st_mode))
	    t = "link";
# endif
# ifdef S_ISBLK
	else if (S_ISBLK(st.st_mode))
	    t = "bdev";
# endif
# ifdef S_ISCHR
	else if (S_ISCHR(st.st_mode))
	    t = "cdev";
# endif
# ifdef S_ISFIFO
	else if (S_ISFIFO(st.st_mode))
	    t = "fifo";
# endif
# ifdef S_ISSOCK
	else if (S_ISSOCK(st.st_mode))
	    t = "fifo";
# endif
	else
	    t = "other";
#else
# ifdef S_IFMT
	switch (st.st_mode & S_IFMT)
	{
	    case S_IFREG: t = "file"; break;
	    case S_IFDIR: t = "dir"; break;
#  ifdef S_IFLNK
	    case S_IFLNK: t = "link"; break;
#  endif
#  ifdef S_IFBLK
	    case S_IFBLK: t = "bdev"; break;
#  endif
#  ifdef S_IFCHR
	    case S_IFCHR: t = "cdev"; break;
#  endif
#  ifdef S_IFIFO
	    case S_IFIFO: t = "fifo"; break;
#  endif
#  ifdef S_IFSOCK
	    case S_IFSOCK: t = "socket"; break;
#  endif
	    default: t = "other";
	}
# else
	if (mch_isdir(fname))
	    t = "dir";
	else
	    t = "file";
# endif
#endif
	type = vim_strsave((char_u *)t);
    }
    rettv->vval.v_string = type;
}

/*
 * "getline(lnum, [end])" function
 */
    static void
f_getline(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    linenr_T	lnum;
    linenr_T	end;
    int		retlist;

    lnum = get_tv_lnum(argvars);
    if (argvars[1].v_type == VAR_UNKNOWN)
    {
	end = 0;
	retlist = FALSE;
    }
    else
    {
	end = get_tv_lnum(&argvars[1]);
	retlist = TRUE;
    }

    get_buffer_lines(curbuf, lnum, end, retlist, rettv);
}

/*
 * "getmatches()" function
 */
/*ARGSUSED*/
    static void
f_getmatches(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_SEARCH_EXTRA
    dict_T	*dict;
    matchitem_T	*cur = curwin->w_match_head;

    rettv->vval.v_number = 0;

    if (rettv_list_alloc(rettv) == OK)
    {
	while (cur != NULL)
	{
	    dict = dict_alloc();
	    if (dict == NULL)
		return;
	    dict_add_nr_str(dict, "group", 0L, syn_id2name(cur->hlg_id));
	    dict_add_nr_str(dict, "pattern", 0L, cur->pattern);
	    dict_add_nr_str(dict, "priority", (long)cur->priority, NULL);
	    dict_add_nr_str(dict, "id", (long)cur->id, NULL);
	    list_append_dict(rettv->vval.v_list, dict);
	    cur = cur->next;
	}
    }
#endif
}

/*
 * "getpid()" function
 */
/*ARGSUSED*/
    static void
f_getpid(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = mch_get_pid();
}

/*
 * "getpos(string)" function
 */
    static void
f_getpos(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    pos_T	*fp;
    list_T	*l;
    int		fnum = -1;

    if (rettv_list_alloc(rettv) == OK)
    {
	l = rettv->vval.v_list;
	fp = var2fpos(&argvars[0], TRUE, &fnum);
	if (fnum != -1)
	    list_append_number(l, (varnumber_T)fnum);
	else
	    list_append_number(l, (varnumber_T)0);
	list_append_number(l, (fp != NULL) ? (varnumber_T)fp->lnum
							    : (varnumber_T)0);
	list_append_number(l, (fp != NULL)
		     ? (varnumber_T)(fp->col == MAXCOL ? MAXCOL : fp->col + 1)
							    : (varnumber_T)0);
	list_append_number(l,
#ifdef FEAT_VIRTUALEDIT
				(fp != NULL) ? (varnumber_T)fp->coladd :
#endif
							      (varnumber_T)0);
    }
    else
	rettv->vval.v_number = FALSE;
}

/*
 * "getqflist()" and "getloclist()" functions
 */
/*ARGSUSED*/
    static void
f_getqflist(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_QUICKFIX
    win_T	*wp;
#endif

    rettv->vval.v_number = 0;
#ifdef FEAT_QUICKFIX
    if (rettv_list_alloc(rettv) == OK)
    {
	wp = NULL;
	if (argvars[0].v_type != VAR_UNKNOWN)	/* getloclist() */
	{
	    wp = find_win_by_nr(&argvars[0], NULL);
	    if (wp == NULL)
		return;
	}

	(void)get_errorlist(wp, rettv->vval.v_list);
    }
#endif
}

/*
 * "getreg()" function
 */
    static void
f_getreg(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*strregname;
    int		regname;
    int		arg2 = FALSE;
    int		error = FALSE;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	strregname = get_tv_string_chk(&argvars[0]);
	error = strregname == NULL;
	if (argvars[1].v_type != VAR_UNKNOWN)
	    arg2 = get_tv_number_chk(&argvars[1], &error);
    }
    else
	strregname = vimvars[VV_REG].vv_str;
    regname = (strregname == NULL ? '"' : *strregname);
    if (regname == 0)
	regname = '"';

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = error ? NULL :
				    get_reg_contents(regname, TRUE, arg2);
}

/*
 * "getregtype()" function
 */
    static void
f_getregtype(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*strregname;
    int		regname;
    char_u	buf[NUMBUFLEN + 2];
    long	reglen = 0;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	strregname = get_tv_string_chk(&argvars[0]);
	if (strregname == NULL)	    /* type error; errmsg already given */
	{
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = NULL;
	    return;
	}
    }
    else
	/* Default to v:register */
	strregname = vimvars[VV_REG].vv_str;

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
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave(buf);
}

/*
 * "gettabwinvar()" function
 */
    static void
f_gettabwinvar(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    getwinvar(argvars, rettv, 1);
}

/*
 * "getwinposx()" function
 */
/*ARGSUSED*/
    static void
f_getwinposx(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = -1;
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    rettv->vval.v_number = x;
    }
#endif
}

/*
 * "getwinposy()" function
 */
/*ARGSUSED*/
    static void
f_getwinposy(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = -1;
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    rettv->vval.v_number = y;
    }
#endif
}

/*
 * Find window specified by "vp" in tabpage "tp".
 */
    static win_T *
find_win_by_nr(vp, tp)
    typval_T	*vp;
    tabpage_T	*tp;	    /* NULL for current tab page */
{
#ifdef FEAT_WINDOWS
    win_T	*wp;
#endif
    int		nr;

    nr = get_tv_number_chk(vp, NULL);

#ifdef FEAT_WINDOWS
    if (nr < 0)
	return NULL;
    if (nr == 0)
	return curwin;

    for (wp = (tp == NULL || tp == curtab) ? firstwin : tp->tp_firstwin;
						  wp != NULL; wp = wp->w_next)
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
 * "getwinvar()" function
 */
    static void
f_getwinvar(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    getwinvar(argvars, rettv, 0);
}

/*
 * getwinvar() and gettabwinvar()
 */
    static void
getwinvar(argvars, rettv, off)
    typval_T	*argvars;
    typval_T	*rettv;
    int		off;	    /* 1 for gettabwinvar() */
{
    win_T	*win, *oldcurwin;
    char_u	*varname;
    dictitem_T	*v;
    tabpage_T	*tp;

#ifdef FEAT_WINDOWS
    if (off == 1)
	tp = find_tabpage((int)get_tv_number_chk(&argvars[0], NULL));
    else
	tp = curtab;
#endif
    win = find_win_by_nr(&argvars[off], tp);
    varname = get_tv_string_chk(&argvars[off + 1]);
    ++emsg_off;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (win != NULL && varname != NULL)
    {
	/* Set curwin to be our win, temporarily.  Also set curbuf, so
	 * that we can get buffer-local options. */
	oldcurwin = curwin;
	curwin = win;
	curbuf = win->w_buffer;

	if (*varname == '&')	/* window-local-option */
	    get_option_tv(&varname, rettv, 1);
	else
	{
	    if (*varname == NUL)
		/* let getwinvar({nr}, "") return the "w:" dictionary.  The
		 * scope prefix before the NUL byte is required by
		 * find_var_in_ht(). */
		varname = (char_u *)"w:" + 2;
	    /* look up the variable */
	    v = find_var_in_ht(&win->w_vars.dv_hashtab, varname, FALSE);
	    if (v != NULL)
		copy_tv(&v->di_tv, rettv);
	}

	/* restore previous notion of curwin */
	curwin = oldcurwin;
	curbuf = curwin->w_buffer;
    }

    --emsg_off;
}

/*
 * "glob()" function
 */
    static void
f_glob(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		flags = WILD_SILENT|WILD_USE_NL;
    expand_T	xpc;
    int		error = FALSE;

    /* When the optional second argument is non-zero, don't remove matches
    * for 'wildignore' and don't put matches for 'suffixes' at the end. */
    if (argvars[1].v_type != VAR_UNKNOWN
				&& get_tv_number_chk(&argvars[1], &error))
	flags |= WILD_KEEP_ALL;
    rettv->v_type = VAR_STRING;
    if (!error)
    {
	ExpandInit(&xpc);
	xpc.xp_context = EXPAND_FILES;
	rettv->vval.v_string = ExpandOne(&xpc, get_tv_string(&argvars[0]),
						       NULL, flags, WILD_ALL);
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * "globpath()" function
 */
    static void
f_globpath(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		flags = 0;
    char_u	buf1[NUMBUFLEN];
    char_u	*file = get_tv_string_buf_chk(&argvars[1], buf1);
    int		error = FALSE;

    /* When the optional second argument is non-zero, don't remove matches
    * for 'wildignore' and don't put matches for 'suffixes' at the end. */
    if (argvars[2].v_type != VAR_UNKNOWN
				&& get_tv_number_chk(&argvars[2], &error))
	flags |= WILD_KEEP_ALL;
    rettv->v_type = VAR_STRING;
    if (file == NULL || error)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = globpath(get_tv_string(&argvars[0]), file,
								       flags);
}

/*
 * "has()" function
 */
    static void
f_has(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
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
#ifdef MACOS
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
# ifndef FEAT_GUI_W32 /* other GUIs always have multiline balloons */
	"balloon_multiline",
# endif
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
#ifdef CURSOR_SHAPE
	"cursorshape",
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
#if defined(UNIX) && !defined(USE_SYSTEM)
	"filterpipe",
#endif
#ifdef FEAT_FIND_ID
	"find_in_path",
#endif
#ifdef FEAT_FLOAT
	"float",
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
#ifdef FEAT_GUI_GTK
	"gui_gtk",
# ifdef HAVE_GTK2
	"gui_gtk2",
# endif
#endif
#ifdef FEAT_GUI_GNOME
	"gui_gnome",
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
# ifdef FEAT_SYSMOUSE
	"mouse_sysmouse",
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
#ifdef FEAT_MZSCHEME
#ifndef DYNAMIC_MZSCHEME
	"mzscheme",
#endif
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
#ifdef FEAT_PROFILE
	"profile",
#endif
#ifdef FEAT_RELTIME
	"reltime",
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
#ifdef FEAT_SPELL
	"spell",
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

    name = get_tv_string(&argvars[0]);
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
#ifdef FEAT_MBYTE
	else if (STRICMP(name, "multi_byte_encoding") == 0)
	    n = has_mbyte;
#endif
#if defined(FEAT_BEVAL) && defined(FEAT_GUI_W32)
	else if (STRICMP(name, "balloon_multiline") == 0)
	    n = multiline_balloon_available();
#endif
#ifdef DYNAMIC_TCL
	else if (STRICMP(name, "tcl") == 0)
	    n = tcl_enabled(FALSE);
#endif
#if defined(USE_ICONV) && defined(DYNAMIC_ICONV)
	else if (STRICMP(name, "iconv") == 0)
	    n = iconv_enabled(FALSE);
#endif
#ifdef DYNAMIC_MZSCHEME
	else if (STRICMP(name, "mzscheme") == 0)
	    n = mzscheme_enabled(FALSE);
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
#ifdef FEAT_NETBEANS_INTG
	else if (STRICMP(name, "netbeans_enabled") == 0)
	    n = usingNetbeans;
#endif
    }

    rettv->vval.v_number = n;
}

/*
 * "has_key()" function
 */
    static void
f_has_key(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = 0;
    if (argvars[0].v_type != VAR_DICT)
    {
	EMSG(_(e_dictreq));
	return;
    }
    if (argvars[0].vval.v_dict == NULL)
	return;

    rettv->vval.v_number = dict_find(argvars[0].vval.v_dict,
				      get_tv_string(&argvars[1]), -1) != NULL;
}

/*
 * "haslocaldir()" function
 */
/*ARGSUSED*/
    static void
f_haslocaldir(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = (curwin->w_localdir != NULL);
}

/*
 * "hasmapto()" function
 */
    static void
f_hasmapto(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*name;
    char_u	*mode;
    char_u	buf[NUMBUFLEN];
    int		abbr = FALSE;

    name = get_tv_string(&argvars[0]);
    if (argvars[1].v_type == VAR_UNKNOWN)
	mode = (char_u *)"nvo";
    else
    {
	mode = get_tv_string_buf(&argvars[1], buf);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    abbr = get_tv_number(&argvars[2]);
    }

    if (map_to_exists(name, mode, abbr))
	rettv->vval.v_number = TRUE;
    else
	rettv->vval.v_number = FALSE;
}

/*
 * "histadd()" function
 */
/*ARGSUSED*/
    static void
f_histadd(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_CMDHIST
    int		histype;
    char_u	*str;
    char_u	buf[NUMBUFLEN];
#endif

    rettv->vval.v_number = FALSE;
    if (check_restricted() || check_secure())
	return;
#ifdef FEAT_CMDHIST
    str = get_tv_string_chk(&argvars[0]);	/* NULL on type error */
    histype = str != NULL ? get_histtype(str) : -1;
    if (histype >= 0)
    {
	str = get_tv_string_buf(&argvars[1], buf);
	if (*str != NUL)
	{
	    add_to_history(histype, str, FALSE, NUL);
	    rettv->vval.v_number = TRUE;
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
f_histdel(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_CMDHIST
    int		n;
    char_u	buf[NUMBUFLEN];
    char_u	*str;

    str = get_tv_string_chk(&argvars[0]);	/* NULL on type error */
    if (str == NULL)
	n = 0;
    else if (argvars[1].v_type == VAR_UNKNOWN)
	/* only one argument: clear entire history */
	n = clr_history(get_histtype(str));
    else if (argvars[1].v_type == VAR_NUMBER)
	/* index given: remove that entry */
	n = del_history_idx(get_histtype(str),
					  (int)get_tv_number(&argvars[1]));
    else
	/* string given: remove all matching entries */
	n = del_history_entry(get_histtype(str),
				      get_tv_string_buf(&argvars[1], buf));
    rettv->vval.v_number = n;
#else
    rettv->vval.v_number = 0;
#endif
}

/*
 * "histget()" function
 */
/*ARGSUSED*/
    static void
f_histget(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_CMDHIST
    int		type;
    int		idx;
    char_u	*str;

    str = get_tv_string_chk(&argvars[0]);	/* NULL on type error */
    if (str == NULL)
	rettv->vval.v_string = NULL;
    else
    {
	type = get_histtype(str);
	if (argvars[1].v_type == VAR_UNKNOWN)
	    idx = get_history_idx(type);
	else
	    idx = (int)get_tv_number_chk(&argvars[1], NULL);
						    /* -1 on type error */
	rettv->vval.v_string = vim_strsave(get_history_entry(type, idx));
    }
#else
    rettv->vval.v_string = NULL;
#endif
    rettv->v_type = VAR_STRING;
}

/*
 * "histnr()" function
 */
/*ARGSUSED*/
    static void
f_histnr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		i;

#ifdef FEAT_CMDHIST
    char_u	*history = get_tv_string_chk(&argvars[0]);

    i = history == NULL ? HIST_CMD - 1 : get_histtype(history);
    if (i >= HIST_CMD && i < HIST_COUNT)
	i = get_history_idx(i);
    else
#endif
	i = -1;
    rettv->vval.v_number = i;
}

/*
 * "highlightID(name)" function
 */
    static void
f_hlID(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = syn_name2id(get_tv_string(&argvars[0]));
}

/*
 * "highlight_exists()" function
 */
    static void
f_hlexists(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = highlight_exists(get_tv_string(&argvars[0]));
}

/*
 * "hostname()" function
 */
/*ARGSUSED*/
    static void
f_hostname(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u hostname[256];

    mch_get_host_name(hostname, 256);
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave(hostname);
}

/*
 * iconv() function
 */
/*ARGSUSED*/
    static void
f_iconv(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_MBYTE
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*from, *to, *str;
    vimconv_T	vimconv;
#endif

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

#ifdef FEAT_MBYTE
    str = get_tv_string(&argvars[0]);
    from = enc_canonize(enc_skip(get_tv_string_buf(&argvars[1], buf1)));
    to = enc_canonize(enc_skip(get_tv_string_buf(&argvars[2], buf2)));
    vimconv.vc_type = CONV_NONE;
    convert_setup(&vimconv, from, to);

    /* If the encodings are equal, no conversion needed. */
    if (vimconv.vc_type == CONV_NONE)
	rettv->vval.v_string = vim_strsave(str);
    else
	rettv->vval.v_string = string_convert(&vimconv, str, NULL);

    convert_setup(&vimconv, NULL, NULL);
    vim_free(from);
    vim_free(to);
#endif
}

/*
 * "indent()" function
 */
    static void
f_indent(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    linenr_T	lnum;

    lnum = get_tv_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	rettv->vval.v_number = get_indent_lnum(lnum);
    else
	rettv->vval.v_number = -1;
}

/*
 * "index()" function
 */
    static void
f_index(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    list_T	*l;
    listitem_T	*item;
    long	idx = 0;
    int		ic = FALSE;

    rettv->vval.v_number = -1;
    if (argvars[0].v_type != VAR_LIST)
    {
	EMSG(_(e_listreq));
	return;
    }
    l = argvars[0].vval.v_list;
    if (l != NULL)
    {
	item = l->lv_first;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    int		error = FALSE;

	    /* Start at specified item.  Use the cached index that list_find()
	     * sets, so that a negative number also works. */
	    item = list_find(l, get_tv_number_chk(&argvars[2], &error));
	    idx = l->lv_idx;
	    if (argvars[3].v_type != VAR_UNKNOWN)
		ic = get_tv_number_chk(&argvars[3], &error);
	    if (error)
		item = NULL;
	}

	for ( ; item != NULL; item = item->li_next, ++idx)
	    if (tv_equal(&item->li_tv, &argvars[1], ic))
	    {
		rettv->vval.v_number = idx;
		break;
	    }
    }
}

static int inputsecret_flag = 0;

static void get_user_input __ARGS((typval_T *argvars, typval_T *rettv, int inputdialog));

/*
 * This function is used by f_input() and f_inputdialog() functions. The third
 * argument to f_input() specifies the type of completion to use at the
 * prompt. The third argument to f_inputdialog() specifies the value to return
 * when the user cancels the prompt.
 */
    static void
get_user_input(argvars, rettv, inputdialog)
    typval_T	*argvars;
    typval_T	*rettv;
    int		inputdialog;
{
    char_u	*prompt = get_tv_string_chk(&argvars[0]);
    char_u	*p = NULL;
    int		c;
    char_u	buf[NUMBUFLEN];
    int		cmd_silent_save = cmd_silent;
    char_u	*defstr = (char_u *)"";
    int		xp_type = EXPAND_NOTHING;
    char_u	*xp_arg = NULL;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

#ifdef NO_CONSOLE_INPUT
    /* While starting up, there is no place to enter text. */
    if (no_console_input())
	return;
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

	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    defstr = get_tv_string_buf_chk(&argvars[1], buf);
	    if (defstr != NULL)
		stuffReadbuffSpec(defstr);

	    if (!inputdialog && argvars[2].v_type != VAR_UNKNOWN)
	    {
		char_u	*xp_name;
		int	xp_namelen;
		long	argt;

		rettv->vval.v_string = NULL;

		xp_name = get_tv_string_buf_chk(&argvars[2], buf);
		if (xp_name == NULL)
		    return;

		xp_namelen = (int)STRLEN(xp_name);

		if (parse_compl_arg(xp_name, xp_namelen, &xp_type, &argt,
							     &xp_arg) == FAIL)
		    return;
	    }
	}

	if (defstr != NULL)
	    rettv->vval.v_string =
		getcmdline_prompt(inputsecret_flag ? NUL : '@', p, echo_attr,
				  xp_type, xp_arg);

	vim_free(xp_arg);

	/* since the user typed this, no need to wait for return */
	need_wait_return = FALSE;
	msg_didout = FALSE;
    }
    cmd_silent = cmd_silent_save;
}

/*
 * "input()" function
 *     Also handles inputsecret() when inputsecret is set.
 */
    static void
f_input(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    get_user_input(argvars, rettv, FALSE);
}

/*
 * "inputdialog()" function
 */
    static void
f_inputdialog(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#if defined(FEAT_GUI_TEXTDIALOG)
    /* Use a GUI dialog if the GUI is running and 'c' is not in 'guioptions' */
    if (gui.in_use && vim_strchr(p_go, GO_CONDIALOG) == NULL)
    {
	char_u	*message;
	char_u	buf[NUMBUFLEN];
	char_u	*defstr = (char_u *)"";

	message = get_tv_string_chk(&argvars[0]);
	if (argvars[1].v_type != VAR_UNKNOWN
		&& (defstr = get_tv_string_buf_chk(&argvars[1], buf)) != NULL)
	    vim_strncpy(IObuff, defstr, IOSIZE - 1);
	else
	    IObuff[0] = NUL;
	if (message != NULL && defstr != NULL
		&& do_dialog(VIM_QUESTION, NULL, message,
				(char_u *)_("&OK\n&Cancel"), 1, IObuff) == 1)
	    rettv->vval.v_string = vim_strsave(IObuff);
	else
	{
	    if (message != NULL && defstr != NULL
					&& argvars[1].v_type != VAR_UNKNOWN
					&& argvars[2].v_type != VAR_UNKNOWN)
		rettv->vval.v_string = vim_strsave(
				      get_tv_string_buf(&argvars[2], buf));
	    else
		rettv->vval.v_string = NULL;
	}
	rettv->v_type = VAR_STRING;
    }
    else
#endif
	get_user_input(argvars, rettv, TRUE);
}

/*
 * "inputlist()" function
 */
    static void
f_inputlist(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    listitem_T	*li;
    int		selected;
    int		mouse_used;

    rettv->vval.v_number = 0;
#ifdef NO_CONSOLE_INPUT
    /* While starting up, there is no place to enter text. */
    if (no_console_input())
	return;
#endif
    if (argvars[0].v_type != VAR_LIST || argvars[0].vval.v_list == NULL)
    {
	EMSG2(_(e_listarg), "inputlist()");
	return;
    }

    msg_start();
    msg_row = Rows - 1;	/* for when 'cmdheight' > 1 */
    lines_left = Rows;	/* avoid more prompt */
    msg_scroll = TRUE;
    msg_clr_eos();

    for (li = argvars[0].vval.v_list->lv_first; li != NULL; li = li->li_next)
    {
	msg_puts(get_tv_string(&li->li_tv));
	msg_putchar('\n');
    }

    /* Ask for choice. */
    selected = prompt_for_number(&mouse_used);
    if (mouse_used)
	selected -= lines_left;

    rettv->vval.v_number = selected;
}


static garray_T	    ga_userinput = {0, 0, sizeof(tasave_T), 4, NULL};

/*
 * "inputrestore()" function
 */
/*ARGSUSED*/
    static void
f_inputrestore(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    if (ga_userinput.ga_len > 0)
    {
	--ga_userinput.ga_len;
	restore_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	rettv->vval.v_number = 0; /* OK */
    }
    else if (p_verbose > 1)
    {
	verb_msg((char_u *)_("called inputrestore() more often than inputsave()"));
	rettv->vval.v_number = 1; /* Failed */
    }
}

/*
 * "inputsave()" function
 */
/*ARGSUSED*/
    static void
f_inputsave(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    /* Add an entry to the stack of typeahead storage. */
    if (ga_grow(&ga_userinput, 1) == OK)
    {
	save_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	++ga_userinput.ga_len;
	rettv->vval.v_number = 0; /* OK */
    }
    else
	rettv->vval.v_number = 1; /* Failed */
}

/*
 * "inputsecret()" function
 */
    static void
f_inputsecret(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    ++cmdline_star;
    ++inputsecret_flag;
    f_input(argvars, rettv);
    --cmdline_star;
    --inputsecret_flag;
}

/*
 * "insert()" function
 */
    static void
f_insert(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    long	before = 0;
    listitem_T	*item;
    list_T	*l;
    int		error = FALSE;

    rettv->vval.v_number = 0;
    if (argvars[0].v_type != VAR_LIST)
	EMSG2(_(e_listarg), "insert()");
    else if ((l = argvars[0].vval.v_list) != NULL
	    && !tv_check_lock(l->lv_lock, (char_u *)"insert()"))
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    before = get_tv_number_chk(&argvars[2], &error);
	if (error)
	    return;		/* type error; errmsg already given */

	if (before == l->lv_len)
	    item = NULL;
	else
	{
	    item = list_find(l, before);
	    if (item == NULL)
	    {
		EMSGN(_(e_listidx), before);
		l = NULL;
	    }
	}
	if (l != NULL)
	{
	    list_insert_tv(l, &argvars[1], item);
	    copy_tv(&argvars[0], rettv);
	}
    }
}

/*
 * "isdirectory()" function
 */
    static void
f_isdirectory(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = mch_isdir(get_tv_string(&argvars[0]));
}

/*
 * "islocked()" function
 */
    static void
f_islocked(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    lval_T	lv;
    char_u	*end;
    dictitem_T	*di;

    rettv->vval.v_number = -1;
    end = get_lval(get_tv_string(&argvars[0]), NULL, &lv, FALSE, FALSE, FALSE,
							     FNE_CHECK_START);
    if (end != NULL && lv.ll_name != NULL)
    {
	if (*end != NUL)
	    EMSG(_(e_trailing));
	else
	{
	    if (lv.ll_tv == NULL)
	    {
		if (check_changedtick(lv.ll_name))
		    rettv->vval.v_number = 1;	    /* always locked */
		else
		{
		    di = find_var(lv.ll_name, NULL);
		    if (di != NULL)
		    {
			/* Consider a variable locked when:
			 * 1. the variable itself is locked
			 * 2. the value of the variable is locked.
			 * 3. the List or Dict value is locked.
			 */
			rettv->vval.v_number = ((di->di_flags & DI_FLAGS_LOCK)
						  || tv_islocked(&di->di_tv));
		    }
		}
	    }
	    else if (lv.ll_range)
		EMSG(_("E786: Range not allowed"));
	    else if (lv.ll_newkey != NULL)
		EMSG2(_(e_dictkey), lv.ll_newkey);
	    else if (lv.ll_list != NULL)
		/* List item. */
		rettv->vval.v_number = tv_islocked(&lv.ll_li->li_tv);
	    else
		/* Dictionary item. */
		rettv->vval.v_number = tv_islocked(&lv.ll_di->di_tv);
	}
    }

    clear_lval(&lv);
}

static void dict_list __ARGS((typval_T *argvars, typval_T *rettv, int what));

/*
 * Turn a dict into a list:
 * "what" == 0: list of keys
 * "what" == 1: list of values
 * "what" == 2: list of items
 */
    static void
dict_list(argvars, rettv, what)
    typval_T	*argvars;
    typval_T	*rettv;
    int		what;
{
    list_T	*l2;
    dictitem_T	*di;
    hashitem_T	*hi;
    listitem_T	*li;
    listitem_T	*li2;
    dict_T	*d;
    int		todo;

    rettv->vval.v_number = 0;
    if (argvars[0].v_type != VAR_DICT)
    {
	EMSG(_(e_dictreq));
	return;
    }
    if ((d = argvars[0].vval.v_dict) == NULL)
	return;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    todo = (int)d->dv_hashtab.ht_used;
    for (hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    di = HI2DI(hi);

	    li = listitem_alloc();
	    if (li == NULL)
		break;
	    list_append(rettv->vval.v_list, li);

	    if (what == 0)
	    {
		/* keys() */
		li->li_tv.v_type = VAR_STRING;
		li->li_tv.v_lock = 0;
		li->li_tv.vval.v_string = vim_strsave(di->di_key);
	    }
	    else if (what == 1)
	    {
		/* values() */
		copy_tv(&di->di_tv, &li->li_tv);
	    }
	    else
	    {
		/* items() */
		l2 = list_alloc();
		li->li_tv.v_type = VAR_LIST;
		li->li_tv.v_lock = 0;
		li->li_tv.vval.v_list = l2;
		if (l2 == NULL)
		    break;
		++l2->lv_refcount;

		li2 = listitem_alloc();
		if (li2 == NULL)
		    break;
		list_append(l2, li2);
		li2->li_tv.v_type = VAR_STRING;
		li2->li_tv.v_lock = 0;
		li2->li_tv.vval.v_string = vim_strsave(di->di_key);

		li2 = listitem_alloc();
		if (li2 == NULL)
		    break;
		list_append(l2, li2);
		copy_tv(&di->di_tv, &li2->li_tv);
	    }
	}
    }
}

/*
 * "items(dict)" function
 */
    static void
f_items(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    dict_list(argvars, rettv, 2);
}

/*
 * "join()" function
 */
    static void
f_join(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    garray_T	ga;
    char_u	*sep;

    rettv->vval.v_number = 0;
    if (argvars[0].v_type != VAR_LIST)
    {
	EMSG(_(e_listreq));
	return;
    }
    if (argvars[0].vval.v_list == NULL)
	return;
    if (argvars[1].v_type == VAR_UNKNOWN)
	sep = (char_u *)" ";
    else
	sep = get_tv_string_chk(&argvars[1]);

    rettv->v_type = VAR_STRING;

    if (sep != NULL)
    {
	ga_init2(&ga, (int)sizeof(char), 80);
	list_join(&ga, argvars[0].vval.v_list, sep, TRUE, 0);
	ga_append(&ga, NUL);
	rettv->vval.v_string = (char_u *)ga.ga_data;
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * "keys()" function
 */
    static void
f_keys(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    dict_list(argvars, rettv, 0);
}

/*
 * "last_buffer_nr()" function.
 */
/*ARGSUSED*/
    static void
f_last_buffer_nr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		n = 0;
    buf_T	*buf;

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (n < buf->b_fnum)
	    n = buf->b_fnum;

    rettv->vval.v_number = n;
}

/*
 * "len()" function
 */
    static void
f_len(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    switch (argvars[0].v_type)
    {
	case VAR_STRING:
	case VAR_NUMBER:
	    rettv->vval.v_number = (varnumber_T)STRLEN(
					       get_tv_string(&argvars[0]));
	    break;
	case VAR_LIST:
	    rettv->vval.v_number = list_len(argvars[0].vval.v_list);
	    break;
	case VAR_DICT:
	    rettv->vval.v_number = dict_len(argvars[0].vval.v_dict);
	    break;
	default:
	    EMSG(_("E701: Invalid type for len()"));
	    break;
    }
}

static void libcall_common __ARGS((typval_T *argvars, typval_T *rettv, int type));

    static void
libcall_common(argvars, rettv, type)
    typval_T	*argvars;
    typval_T	*rettv;
    int		type;
{
#ifdef FEAT_LIBCALL
    char_u		*string_in;
    char_u		**string_result;
    int			nr_result;
#endif

    rettv->v_type = type;
    if (type == VAR_NUMBER)
	rettv->vval.v_number = 0;
    else
	rettv->vval.v_string = NULL;

    if (check_restricted() || check_secure())
	return;

#ifdef FEAT_LIBCALL
    /* The first two args must be strings, otherwise its meaningless */
    if (argvars[0].v_type == VAR_STRING && argvars[1].v_type == VAR_STRING)
    {
	string_in = NULL;
	if (argvars[2].v_type == VAR_STRING)
	    string_in = argvars[2].vval.v_string;
	if (type == VAR_NUMBER)
	    string_result = NULL;
	else
	    string_result = &rettv->vval.v_string;
	if (mch_libcall(argvars[0].vval.v_string,
			     argvars[1].vval.v_string,
			     string_in,
			     argvars[2].vval.v_number,
			     string_result,
			     &nr_result) == OK
		&& type == VAR_NUMBER)
	    rettv->vval.v_number = nr_result;
    }
#endif
}

/*
 * "libcall()" function
 */
    static void
f_libcall(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    libcall_common(argvars, rettv, VAR_STRING);
}

/*
 * "libcallnr()" function
 */
    static void
f_libcallnr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    libcall_common(argvars, rettv, VAR_NUMBER);
}

/*
 * "line(string)" function
 */
    static void
f_line(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    linenr_T	lnum = 0;
    pos_T	*fp;
    int		fnum;

    fp = var2fpos(&argvars[0], TRUE, &fnum);
    if (fp != NULL)
	lnum = fp->lnum;
    rettv->vval.v_number = lnum;
}

/*
 * "line2byte(lnum)" function
 */
/*ARGSUSED*/
    static void
f_line2byte(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifndef FEAT_BYTEOFF
    rettv->vval.v_number = -1;
#else
    linenr_T	lnum;

    lnum = get_tv_lnum(argvars);
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count + 1)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = ml_find_line_or_offset(curbuf, lnum, NULL);
    if (rettv->vval.v_number >= 0)
	++rettv->vval.v_number;
#endif
}

/*
 * "lispindent(lnum)" function
 */
    static void
f_lispindent(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_LISP
    pos_T	pos;
    linenr_T	lnum;

    pos = curwin->w_cursor;
    lnum = get_tv_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
	curwin->w_cursor.lnum = lnum;
	rettv->vval.v_number = get_lisp_indent();
	curwin->w_cursor = pos;
    }
    else
#endif
	rettv->vval.v_number = -1;
}

/*
 * "localtime()" function
 */
/*ARGSUSED*/
    static void
f_localtime(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = (varnumber_T)time(NULL);
}

static void get_maparg __ARGS((typval_T *argvars, typval_T *rettv, int exact));

    static void
get_maparg(argvars, rettv, exact)
    typval_T	*argvars;
    typval_T	*rettv;
    int		exact;
{
    char_u	*keys;
    char_u	*which;
    char_u	buf[NUMBUFLEN];
    char_u	*keys_buf = NULL;
    char_u	*rhs;
    int		mode;
    garray_T	ga;
    int		abbr = FALSE;

    /* return empty string for failure */
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    keys = get_tv_string(&argvars[0]);
    if (*keys == NUL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	which = get_tv_string_buf_chk(&argvars[1], buf);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    abbr = get_tv_number(&argvars[2]);
    }
    else
	which = (char_u *)"";
    if (which == NULL)
	return;

    mode = get_map_mode(&which, 0);

    keys = replace_termcodes(keys, &keys_buf, TRUE, TRUE, FALSE);
    rhs = check_map(keys, mode, exact, FALSE, abbr);
    vim_free(keys_buf);
    if (rhs != NULL)
    {
	ga_init(&ga);
	ga.ga_itemsize = 1;
	ga.ga_growsize = 40;

	while (*rhs != NUL)
	    ga_concat(&ga, str2special(&rhs, FALSE));

	ga_append(&ga, NUL);
	rettv->vval.v_string = (char_u *)ga.ga_data;
    }
}

#ifdef FEAT_FLOAT
/*
 * "log10()" function
 */
    static void
f_log10(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = log10(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "map()" function
 */
    static void
f_map(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    filter_map(argvars, rettv, TRUE);
}

/*
 * "maparg()" function
 */
    static void
f_maparg(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    get_maparg(argvars, rettv, TRUE);
}

/*
 * "mapcheck()" function
 */
    static void
f_mapcheck(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    get_maparg(argvars, rettv, FALSE);
}

static void find_some_match __ARGS((typval_T *argvars, typval_T *rettv, int start));

    static void
find_some_match(argvars, rettv, type)
    typval_T	*argvars;
    typval_T	*rettv;
    int		type;
{
    char_u	*str = NULL;
    char_u	*expr = NULL;
    char_u	*pat;
    regmatch_T	regmatch;
    char_u	patbuf[NUMBUFLEN];
    char_u	strbuf[NUMBUFLEN];
    char_u	*save_cpo;
    long	start = 0;
    long	nth = 1;
    colnr_T	startcol = 0;
    int		match = 0;
    list_T	*l = NULL;
    listitem_T	*li = NULL;
    long	idx = 0;
    char_u	*tofree = NULL;

    /* Make 'cpoptions' empty, the 'l' flag should not be used here. */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    rettv->vval.v_number = -1;
    if (type == 3)
    {
	/* return empty list when there are no matches */
	if (rettv_list_alloc(rettv) == FAIL)
	    goto theend;
    }
    else if (type == 2)
    {
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = NULL;
    }

    if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) == NULL)
	    goto theend;
	li = l->lv_first;
    }
    else
	expr = str = get_tv_string(&argvars[0]);

    pat = get_tv_string_buf_chk(&argvars[1], patbuf);
    if (pat == NULL)
	goto theend;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	int	    error = FALSE;

	start = get_tv_number_chk(&argvars[2], &error);
	if (error)
	    goto theend;
	if (l != NULL)
	{
	    li = list_find(l, start);
	    if (li == NULL)
		goto theend;
	    idx = l->lv_idx;	/* use the cached index */
	}
	else
	{
	    if (start < 0)
		start = 0;
	    if (start > (long)STRLEN(str))
		goto theend;
	    /* When "count" argument is there ignore matches before "start",
	     * otherwise skip part of the string.  Differs when pattern is "^"
	     * or "\<". */
	    if (argvars[3].v_type != VAR_UNKNOWN)
		startcol = start;
	    else
		str += start;
	}

	if (argvars[3].v_type != VAR_UNKNOWN)
	    nth = get_tv_number_chk(&argvars[3], &error);
	if (error)
	    goto theend;
    }

    regmatch.regprog = vim_regcomp(pat, RE_MAGIC + RE_STRING);
    if (regmatch.regprog != NULL)
    {
	regmatch.rm_ic = p_ic;

	for (;;)
	{
	    if (l != NULL)
	    {
		if (li == NULL)
		{
		    match = FALSE;
		    break;
		}
		vim_free(tofree);
		str = echo_string(&li->li_tv, &tofree, strbuf, 0);
		if (str == NULL)
		    break;
	    }

	    match = vim_regexec_nl(&regmatch, str, (colnr_T)startcol);

	    if (match && --nth <= 0)
		break;
	    if (l == NULL && !match)
		break;

	    /* Advance to just after the match. */
	    if (l != NULL)
	    {
		li = li->li_next;
		++idx;
	    }
	    else
	    {
#ifdef FEAT_MBYTE
		startcol = (colnr_T)(regmatch.startp[0]
				    + (*mb_ptr2len)(regmatch.startp[0]) - str);
#else
		startcol = regmatch.startp[0] + 1 - str;
#endif
	    }
	}

	if (match)
	{
	    if (type == 3)
	    {
		int i;

		/* return list with matched string and submatches */
		for (i = 0; i < NSUBEXP; ++i)
		{
		    if (regmatch.endp[i] == NULL)
		    {
			if (list_append_string(rettv->vval.v_list,
						     (char_u *)"", 0) == FAIL)
			    break;
		    }
		    else if (list_append_string(rettv->vval.v_list,
				regmatch.startp[i],
				(int)(regmatch.endp[i] - regmatch.startp[i]))
			    == FAIL)
			break;
		}
	    }
	    else if (type == 2)
	    {
		/* return matched string */
		if (l != NULL)
		    copy_tv(&li->li_tv, rettv);
		else
		    rettv->vval.v_string = vim_strnsave(regmatch.startp[0],
				(int)(regmatch.endp[0] - regmatch.startp[0]));
	    }
	    else if (l != NULL)
		rettv->vval.v_number = idx;
	    else
	    {
		if (type != 0)
		    rettv->vval.v_number =
				      (varnumber_T)(regmatch.startp[0] - str);
		else
		    rettv->vval.v_number =
					(varnumber_T)(regmatch.endp[0] - str);
		rettv->vval.v_number += (varnumber_T)(str - expr);
	    }
	}
	vim_free(regmatch.regprog);
    }

theend:
    vim_free(tofree);
    p_cpo = save_cpo;
}

/*
 * "match()" function
 */
    static void
f_match(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    find_some_match(argvars, rettv, 1);
}

/*
 * "matchadd()" function
 */
    static void
f_matchadd(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_SEARCH_EXTRA
    char_u	buf[NUMBUFLEN];
    char_u	*grp = get_tv_string_buf_chk(&argvars[0], buf);	/* group */
    char_u	*pat = get_tv_string_buf_chk(&argvars[1], buf);	/* pattern */
    int		prio = 10;	/* default priority */
    int		id = -1;
    int		error = FALSE;

    rettv->vval.v_number = -1;

    if (grp == NULL || pat == NULL)
	return;
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	prio = get_tv_number_chk(&argvars[2], &error);
	if (argvars[3].v_type != VAR_UNKNOWN)
	    id = get_tv_number_chk(&argvars[3], &error);
    }
    if (error == TRUE)
	return;
    if (id >= 1 && id <= 3)
    {
	EMSGN("E798: ID is reserved for \":match\": %ld", id);
	return;
    }

    rettv->vval.v_number = match_add(curwin, grp, pat, prio, id);
#endif
}

/*
 * "matcharg()" function
 */
    static void
f_matcharg(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    if (rettv_list_alloc(rettv) == OK)
    {
#ifdef FEAT_SEARCH_EXTRA
	int	    id = get_tv_number(&argvars[0]);
	matchitem_T *m;

	if (id >= 1 && id <= 3)
	{
	    if ((m = (matchitem_T *)get_match(curwin, id)) != NULL)
	    {
		list_append_string(rettv->vval.v_list,
						syn_id2name(m->hlg_id), -1);
		list_append_string(rettv->vval.v_list, m->pattern, -1);
	    }
	    else
	    {
		list_append_string(rettv->vval.v_list, NUL, -1);
		list_append_string(rettv->vval.v_list, NUL, -1);
	    }
	}
#endif
    }
}

/*
 * "matchdelete()" function
 */
    static void
f_matchdelete(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_SEARCH_EXTRA
    rettv->vval.v_number = match_delete(curwin,
				       (int)get_tv_number(&argvars[0]), TRUE);
#endif
}

/*
 * "matchend()" function
 */
    static void
f_matchend(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    find_some_match(argvars, rettv, 0);
}

/*
 * "matchlist()" function
 */
    static void
f_matchlist(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    find_some_match(argvars, rettv, 3);
}

/*
 * "matchstr()" function
 */
    static void
f_matchstr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    find_some_match(argvars, rettv, 2);
}

static void max_min __ARGS((typval_T *argvars, typval_T *rettv, int domax));

    static void
max_min(argvars, rettv, domax)
    typval_T	*argvars;
    typval_T	*rettv;
    int		domax;
{
    long	n = 0;
    long	i;
    int		error = FALSE;

    if (argvars[0].v_type == VAR_LIST)
    {
	list_T		*l;
	listitem_T	*li;

	l = argvars[0].vval.v_list;
	if (l != NULL)
	{
	    li = l->lv_first;
	    if (li != NULL)
	    {
		n = get_tv_number_chk(&li->li_tv, &error);
		for (;;)
		{
		    li = li->li_next;
		    if (li == NULL)
			break;
		    i = get_tv_number_chk(&li->li_tv, &error);
		    if (domax ? i > n : i < n)
			n = i;
		}
	    }
	}
    }
    else if (argvars[0].v_type == VAR_DICT)
    {
	dict_T		*d;
	int		first = TRUE;
	hashitem_T	*hi;
	int		todo;

	d = argvars[0].vval.v_dict;
	if (d != NULL)
	{
	    todo = (int)d->dv_hashtab.ht_used;
	    for (hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;
		    i = get_tv_number_chk(&HI2DI(hi)->di_tv, &error);
		    if (first)
		    {
			n = i;
			first = FALSE;
		    }
		    else if (domax ? i > n : i < n)
			n = i;
		}
	    }
	}
    }
    else
	EMSG(_(e_listdictarg));
    rettv->vval.v_number = error ? 0 : n;
}

/*
 * "max()" function
 */
    static void
f_max(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    max_min(argvars, rettv, TRUE);
}

/*
 * "min()" function
 */
    static void
f_min(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    max_min(argvars, rettv, FALSE);
}

static int mkdir_recurse __ARGS((char_u *dir, int prot));

/*
 * Create the directory in which "dir" is located, and higher levels when
 * needed.
 */
    static int
mkdir_recurse(dir, prot)
    char_u	*dir;
    int		prot;
{
    char_u	*p;
    char_u	*updir;
    int		r = FAIL;

    /* Get end of directory name in "dir".
     * We're done when it's "/" or "c:/". */
    p = gettail_sep(dir);
    if (p <= get_past_head(dir))
	return OK;

    /* If the directory exists we're done.  Otherwise: create it.*/
    updir = vim_strnsave(dir, (int)(p - dir));
    if (updir == NULL)
	return FAIL;
    if (mch_isdir(updir))
	r = OK;
    else if (mkdir_recurse(updir, prot) == OK)
	r = vim_mkdir_emsg(updir, prot);
    vim_free(updir);
    return r;
}

#ifdef vim_mkdir
/*
 * "mkdir()" function
 */
    static void
f_mkdir(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*dir;
    char_u	buf[NUMBUFLEN];
    int		prot = 0755;

    rettv->vval.v_number = FAIL;
    if (check_restricted() || check_secure())
	return;

    dir = get_tv_string_buf(&argvars[0], buf);
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    prot = get_tv_number_chk(&argvars[2], NULL);
	if (prot != -1 && STRCMP(get_tv_string(&argvars[1]), "p") == 0)
	    mkdir_recurse(dir, prot);
    }
    rettv->vval.v_number = prot != -1 ? vim_mkdir_emsg(dir, prot) : 0;
}
#endif

/*
 * "mode()" function
 */
/*ARGSUSED*/
    static void
f_mode(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	buf[3];

    buf[1] = NUL;
    buf[2] = NUL;

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
	if (State == HITRETURN || State == ASKMORE || State == SETWSIZE
		|| State == CONFIRM)
    {
	buf[0] = 'r';
	if (State == ASKMORE)
	    buf[1] = 'm';
	else if (State == CONFIRM)
	    buf[1] = '?';
    }
    else if (State == EXTERNCMD)
	buf[0] = '!';
    else if (State & INSERT)
    {
#ifdef FEAT_VREPLACE
	if (State & VREPLACE_FLAG)
	{
	    buf[0] = 'R';
	    buf[1] = 'v';
	}
	else
#endif
	if (State & REPLACE_FLAG)
	    buf[0] = 'R';
	else
	    buf[0] = 'i';
    }
    else if (State & CMDLINE)
    {
	buf[0] = 'c';
	if (exmode_active)
	    buf[1] = 'v';
    }
    else if (exmode_active)
    {
	buf[0] = 'c';
	buf[1] = 'e';
    }
    else
    {
	buf[0] = 'n';
	if (finish_op)
	    buf[1] = 'o';
    }

    /* Clear out the minor mode when the argument is not a non-zero number or
     * non-empty string.  */
    if (!non_zero_arg(&argvars[0]))
	buf[1] = NUL;

    rettv->vval.v_string = vim_strsave(buf);
    rettv->v_type = VAR_STRING;
}

/*
 * "nextnonblank()" function
 */
    static void
f_nextnonblank(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    linenr_T	lnum;

    for (lnum = get_tv_lnum(argvars); ; ++lnum)
    {
	if (lnum < 0 || lnum > curbuf->b_ml.ml_line_count)
	{
	    lnum = 0;
	    break;
	}
	if (*skipwhite(ml_get(lnum)) != NUL)
	    break;
    }
    rettv->vval.v_number = lnum;
}

/*
 * "nr2char()" function
 */
    static void
f_nr2char(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	buf[NUMBUFLEN];

#ifdef FEAT_MBYTE
    if (has_mbyte)
	buf[(*mb_char2bytes)((int)get_tv_number(&argvars[0]), buf)] = NUL;
    else
#endif
    {
	buf[0] = (char_u)get_tv_number(&argvars[0]);
	buf[1] = NUL;
    }
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave(buf);
}

/*
 * "pathshorten()" function
 */
    static void
f_pathshorten(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p;

    rettv->v_type = VAR_STRING;
    p = get_tv_string_chk(&argvars[0]);
    if (p == NULL)
	rettv->vval.v_string = NULL;
    else
    {
	p = vim_strsave(p);
	rettv->vval.v_string = p;
	if (p != NULL)
	    shorten_dir(p);
    }
}

#ifdef FEAT_FLOAT
/*
 * "pow()" function
 */
    static void
f_pow(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	fx, fy;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &fx) == OK
				     && get_float_arg(&argvars[1], &fy) == OK)
	rettv->vval.v_float = pow(fx, fy);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "prevnonblank()" function
 */
    static void
f_prevnonblank(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    linenr_T	lnum;

    lnum = get_tv_lnum(argvars);
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count)
	lnum = 0;
    else
	while (lnum >= 1 && *skipwhite(ml_get(lnum)) == NUL)
	    --lnum;
    rettv->vval.v_number = lnum;
}

#ifdef HAVE_STDARG_H
/* This dummy va_list is here because:
 * - passing a NULL pointer doesn't work when va_list isn't a pointer
 * - locally in the function results in a "used before set" warning
 * - using va_start() to initialize it gives "function with fixed args" error */
static va_list	ap;
#endif

/*
 * "printf()" function
 */
    static void
f_printf(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef HAVE_STDARG_H	    /* only very old compilers can't do this */
    {
	char_u	buf[NUMBUFLEN];
	int	len;
	char_u	*s;
	int	saved_did_emsg = did_emsg;
	char	*fmt;

	/* Get the required length, allocate the buffer and do it for real. */
	did_emsg = FALSE;
	fmt = (char *)get_tv_string_buf(&argvars[0], buf);
	len = vim_vsnprintf(NULL, 0, fmt, ap, argvars + 1);
	if (!did_emsg)
	{
	    s = alloc(len + 1);
	    if (s != NULL)
	    {
		rettv->vval.v_string = s;
		(void)vim_vsnprintf((char *)s, len + 1, fmt, ap, argvars + 1);
	    }
	}
	did_emsg |= saved_did_emsg;
    }
#endif
}

/*
 * "pumvisible()" function
 */
/*ARGSUSED*/
    static void
f_pumvisible(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = 0;
#ifdef FEAT_INS_EXPAND
    if (pum_visible())
	rettv->vval.v_number = 1;
#endif
}

/*
 * "range()" function
 */
    static void
f_range(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    long	start;
    long	end;
    long	stride = 1;
    long	i;
    int		error = FALSE;

    start = get_tv_number_chk(&argvars[0], &error);
    if (argvars[1].v_type == VAR_UNKNOWN)
    {
	end = start - 1;
	start = 0;
    }
    else
    {
	end = get_tv_number_chk(&argvars[1], &error);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    stride = get_tv_number_chk(&argvars[2], &error);
    }

    rettv->vval.v_number = 0;
    if (error)
	return;		/* type error; errmsg already given */
    if (stride == 0)
	EMSG(_("E726: Stride is zero"));
    else if (stride > 0 ? end + 1 < start : end - 1 > start)
	EMSG(_("E727: Start past end"));
    else
    {
	if (rettv_list_alloc(rettv) == OK)
	    for (i = start; stride > 0 ? i <= end : i >= end; i += stride)
		if (list_append_number(rettv->vval.v_list,
						      (varnumber_T)i) == FAIL)
		    break;
    }
}

/*
 * "readfile()" function
 */
    static void
f_readfile(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		binary = FALSE;
    char_u	*fname;
    FILE	*fd;
    listitem_T	*li;
#define FREAD_SIZE 200	    /* optimized for text lines */
    char_u	buf[FREAD_SIZE];
    int		readlen;    /* size of last fread() */
    int		buflen;	    /* nr of valid chars in buf[] */
    int		filtd;	    /* how much in buf[] was NUL -> '\n' filtered */
    int		tolist;	    /* first byte in buf[] still to be put in list */
    int		chop;	    /* how many CR to chop off */
    char_u	*prev = NULL;	/* previously read bytes, if any */
    int		prevlen = 0;    /* length of "prev" if not NULL */
    char_u	*s;
    int		len;
    long	maxline = MAXLNUM;
    long	cnt = 0;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (STRCMP(get_tv_string(&argvars[1]), "b") == 0)
	    binary = TRUE;
	if (argvars[2].v_type != VAR_UNKNOWN)
	    maxline = get_tv_number(&argvars[2]);
    }

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    /* Always open the file in binary mode, library functions have a mind of
     * their own about CR-LF conversion. */
    fname = get_tv_string(&argvars[0]);
    if (*fname == NUL || (fd = mch_fopen((char *)fname, READBIN)) == NULL)
    {
	EMSG2(_(e_notopen), *fname == NUL ? (char_u *)_("<empty>") : fname);
	return;
    }

    filtd = 0;
    while (cnt < maxline || maxline < 0)
    {
	readlen = (int)fread(buf + filtd, 1, FREAD_SIZE - filtd, fd);
	buflen = filtd + readlen;
	tolist = 0;
	for ( ; filtd < buflen || readlen <= 0; ++filtd)
	{
	    if (buf[filtd] == '\n' || readlen <= 0)
	    {
		/* Only when in binary mode add an empty list item when the
		 * last line ends in a '\n'. */
		if (!binary && readlen == 0 && filtd == 0)
		    break;

		/* Found end-of-line or end-of-file: add a text line to the
		 * list. */
		chop = 0;
		if (!binary)
		    while (filtd - chop - 1 >= tolist
					  && buf[filtd - chop - 1] == '\r')
			++chop;
		len = filtd - tolist - chop;
		if (prev == NULL)
		    s = vim_strnsave(buf + tolist, len);
		else
		{
		    s = alloc((unsigned)(prevlen + len + 1));
		    if (s != NULL)
		    {
			mch_memmove(s, prev, prevlen);
			vim_free(prev);
			prev = NULL;
			mch_memmove(s + prevlen, buf + tolist, len);
			s[prevlen + len] = NUL;
		    }
		}
		tolist = filtd + 1;

		li = listitem_alloc();
		if (li == NULL)
		{
		    vim_free(s);
		    break;
		}
		li->li_tv.v_type = VAR_STRING;
		li->li_tv.v_lock = 0;
		li->li_tv.vval.v_string = s;
		list_append(rettv->vval.v_list, li);

		if (++cnt >= maxline && maxline >= 0)
		    break;
		if (readlen <= 0)
		    break;
	    }
	    else if (buf[filtd] == NUL)
		buf[filtd] = '\n';
	}
	if (readlen <= 0)
	    break;

	if (tolist == 0)
	{
	    /* "buf" is full, need to move text to an allocated buffer */
	    if (prev == NULL)
	    {
		prev = vim_strnsave(buf, buflen);
		prevlen = buflen;
	    }
	    else
	    {
		s = alloc((unsigned)(prevlen + buflen));
		if (s != NULL)
		{
		    mch_memmove(s, prev, prevlen);
		    mch_memmove(s + prevlen, buf, buflen);
		    vim_free(prev);
		    prev = s;
		    prevlen += buflen;
		}
	    }
	    filtd = 0;
	}
	else
	{
	    mch_memmove(buf, buf + tolist, buflen - tolist);
	    filtd -= tolist;
	}
    }

    /*
     * For a negative line count use only the lines at the end of the file,
     * free the rest.
     */
    if (maxline < 0)
	while (cnt > -maxline)
	{
	    listitem_remove(rettv->vval.v_list, rettv->vval.v_list->lv_first);
	    --cnt;
	}

    vim_free(prev);
    fclose(fd);
}

#if defined(FEAT_RELTIME)
static int list2proftime __ARGS((typval_T *arg, proftime_T *tm));

/*
 * Convert a List to proftime_T.
 * Return FAIL when there is something wrong.
 */
    static int
list2proftime(arg, tm)
    typval_T	*arg;
    proftime_T  *tm;
{
    long	n1, n2;
    int	error = FALSE;

    if (arg->v_type != VAR_LIST || arg->vval.v_list == NULL
					     || arg->vval.v_list->lv_len != 2)
	return FAIL;
    n1 = list_find_nr(arg->vval.v_list, 0L, &error);
    n2 = list_find_nr(arg->vval.v_list, 1L, &error);
# ifdef WIN3264
    tm->HighPart = n1;
    tm->LowPart = n2;
# else
    tm->tv_sec = n1;
    tm->tv_usec = n2;
# endif
    return error ? FAIL : OK;
}
#endif /* FEAT_RELTIME */

/*
 * "reltime()" function
 */
    static void
f_reltime(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_RELTIME
    proftime_T	res;
    proftime_T	start;

    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	/* No arguments: get current time. */
	profile_start(&res);
    }
    else if (argvars[1].v_type == VAR_UNKNOWN)
    {
	if (list2proftime(&argvars[0], &res) == FAIL)
	    return;
	profile_end(&res);
    }
    else
    {
	/* Two arguments: compute the difference. */
	if (list2proftime(&argvars[0], &start) == FAIL
		|| list2proftime(&argvars[1], &res) == FAIL)
	    return;
	profile_sub(&res, &start);
    }

    if (rettv_list_alloc(rettv) == OK)
    {
	long	n1, n2;

# ifdef WIN3264
	n1 = res.HighPart;
	n2 = res.LowPart;
# else
	n1 = res.tv_sec;
	n2 = res.tv_usec;
# endif
	list_append_number(rettv->vval.v_list, (varnumber_T)n1);
	list_append_number(rettv->vval.v_list, (varnumber_T)n2);
    }
#endif
}

/*
 * "reltimestr()" function
 */
    static void
f_reltimestr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_RELTIME
    proftime_T	tm;
#endif

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_RELTIME
    if (list2proftime(&argvars[0], &tm) == OK)
	rettv->vval.v_string = vim_strsave((char_u *)profile_msg(&tm));
#endif
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

#ifdef FEAT_CLIENTSERVER
static void remote_common __ARGS((typval_T *argvars, typval_T *rettv, int expr));

    static void
remote_common(argvars, rettv, expr)
    typval_T	*argvars;
    typval_T	*rettv;
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

    server_name = get_tv_string_chk(&argvars[0]);
    if (server_name == NULL)
	return;		/* type error; errmsg already given */
    keys = get_tv_string_buf(&argvars[1], buf);
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

    rettv->vval.v_string = r;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	dictitem_T	v;
	char_u		str[30];
	char_u		*idvar;

	sprintf((char *)str, PRINTF_HEX_LONG_U, (long_u)w);
	v.di_tv.v_type = VAR_STRING;
	v.di_tv.vval.v_string = vim_strsave(str);
	idvar = get_tv_string_chk(&argvars[2]);
	if (idvar != NULL)
	    set_var(idvar, &v.di_tv, FALSE);
	vim_free(v.di_tv.vval.v_string);
    }
}
#endif

/*
 * "remote_expr()" function
 */
/*ARGSUSED*/
    static void
f_remote_expr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_CLIENTSERVER
    remote_common(argvars, rettv, TRUE);
#endif
}

/*
 * "remote_foreground()" function
 */
/*ARGSUSED*/
    static void
f_remote_foreground(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = 0;
#ifdef FEAT_CLIENTSERVER
# ifdef WIN32
    /* On Win32 it's done in this application. */
    {
	char_u	*server_name = get_tv_string_chk(&argvars[0]);

	if (server_name != NULL)
	    serverForeground(server_name);
    }
# else
    /* Send a foreground() expression to the server. */
    argvars[1].v_type = VAR_STRING;
    argvars[1].vval.v_string = vim_strsave((char_u *)"foreground()");
    argvars[2].v_type = VAR_UNKNOWN;
    remote_common(argvars, rettv, TRUE);
    vim_free(argvars[1].vval.v_string);
# endif
#endif
}

/*ARGSUSED*/
    static void
f_remote_peek(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_CLIENTSERVER
    dictitem_T	v;
    char_u	*s = NULL;
# ifdef WIN32
    long_u	n = 0;
# endif
    char_u	*serverid;

    if (check_restricted() || check_secure())
    {
	rettv->vval.v_number = -1;
	return;
    }
    serverid = get_tv_string_chk(&argvars[0]);
    if (serverid == NULL)
    {
	rettv->vval.v_number = -1;
	return;		/* type error; errmsg already given */
    }
# ifdef WIN32
    sscanf(serverid, SCANF_HEX_LONG_U, &n);
    if (n == 0)
	rettv->vval.v_number = -1;
    else
    {
	s = serverGetReply((HWND)n, FALSE, FALSE, FALSE);
	rettv->vval.v_number = (s != NULL);
    }
# else
    rettv->vval.v_number = 0;
    if (check_connection() == FAIL)
	return;

    rettv->vval.v_number = serverPeekReply(X_DISPLAY,
						serverStrToWin(serverid), &s);
# endif

    if (argvars[1].v_type != VAR_UNKNOWN && rettv->vval.v_number > 0)
    {
	char_u		*retvar;

	v.di_tv.v_type = VAR_STRING;
	v.di_tv.vval.v_string = vim_strsave(s);
	retvar = get_tv_string_chk(&argvars[1]);
	if (retvar != NULL)
	    set_var(retvar, &v.di_tv, FALSE);
	vim_free(v.di_tv.vval.v_string);
    }
#else
    rettv->vval.v_number = -1;
#endif
}

/*ARGSUSED*/
    static void
f_remote_read(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*r = NULL;

#ifdef FEAT_CLIENTSERVER
    char_u	*serverid = get_tv_string_chk(&argvars[0]);

    if (serverid != NULL && !check_restricted() && !check_secure())
    {
# ifdef WIN32
	/* The server's HWND is encoded in the 'id' parameter */
	long_u		n = 0;

	sscanf(serverid, SCANF_HEX_LONG_U, &n);
	if (n != 0)
	    r = serverGetReply((HWND)n, FALSE, TRUE, TRUE);
	if (r == NULL)
# else
	if (check_connection() == FAIL || serverReadReply(X_DISPLAY,
		serverStrToWin(serverid), &r, FALSE) < 0)
# endif
	    EMSG(_("E277: Unable to read a server reply"));
    }
#endif
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = r;
}

/*
 * "remote_send()" function
 */
/*ARGSUSED*/
    static void
f_remote_send(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_CLIENTSERVER
    remote_common(argvars, rettv, FALSE);
#endif
}

/*
 * "remove()" function
 */
    static void
f_remove(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    list_T	*l;
    listitem_T	*item, *item2;
    listitem_T	*li;
    long	idx;
    long	end;
    char_u	*key;
    dict_T	*d;
    dictitem_T	*di;

    rettv->vval.v_number = 0;
    if (argvars[0].v_type == VAR_DICT)
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    EMSG2(_(e_toomanyarg), "remove()");
	else if ((d = argvars[0].vval.v_dict) != NULL
		&& !tv_check_lock(d->dv_lock, (char_u *)"remove() argument"))
	{
	    key = get_tv_string_chk(&argvars[1]);
	    if (key != NULL)
	    {
		di = dict_find(d, key, -1);
		if (di == NULL)
		    EMSG2(_(e_dictkey), key);
		else
		{
		    *rettv = di->di_tv;
		    init_tv(&di->di_tv);
		    dictitem_remove(d, di);
		}
	    }
	}
    }
    else if (argvars[0].v_type != VAR_LIST)
	EMSG2(_(e_listdictarg), "remove()");
    else if ((l = argvars[0].vval.v_list) != NULL
	    && !tv_check_lock(l->lv_lock, (char_u *)"remove() argument"))
    {
	int	    error = FALSE;

	idx = get_tv_number_chk(&argvars[1], &error);
	if (error)
	    ;		/* type error: do nothing, errmsg already given */
	else if ((item = list_find(l, idx)) == NULL)
	    EMSGN(_(e_listidx), idx);
	else
	{
	    if (argvars[2].v_type == VAR_UNKNOWN)
	    {
		/* Remove one item, return its value. */
		list_remove(l, item, item);
		*rettv = item->li_tv;
		vim_free(item);
	    }
	    else
	    {
		/* Remove range of items, return list with values. */
		end = get_tv_number_chk(&argvars[2], &error);
		if (error)
		    ;		/* type error: do nothing */
		else if ((item2 = list_find(l, end)) == NULL)
		    EMSGN(_(e_listidx), end);
		else
		{
		    int	    cnt = 0;

		    for (li = item; li != NULL; li = li->li_next)
		    {
			++cnt;
			if (li == item2)
			    break;
		    }
		    if (li == NULL)  /* didn't find "item2" after "item" */
			EMSG(_(e_invrange));
		    else
		    {
			list_remove(l, item, item2);
			if (rettv_list_alloc(rettv) == OK)
			{
			    l = rettv->vval.v_list;
			    l->lv_first = item;
			    l->lv_last = item2;
			    item->li_prev = NULL;
			    item2->li_next = NULL;
			    l->lv_len = cnt;
			}
		    }
		}
	    }
	}
    }
}

/*
 * "rename({from}, {to})" function
 */
    static void
f_rename(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	buf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = vim_rename(get_tv_string(&argvars[0]),
				      get_tv_string_buf(&argvars[1], buf));
}

/*
 * "repeat()" function
 */
/*ARGSUSED*/
    static void
f_repeat(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p;
    int		n;
    int		slen;
    int		len;
    char_u	*r;
    int		i;

    n = get_tv_number(&argvars[1]);
    if (argvars[0].v_type == VAR_LIST)
    {
	if (rettv_list_alloc(rettv) == OK && argvars[0].vval.v_list != NULL)
	    while (n-- > 0)
		if (list_extend(rettv->vval.v_list,
					argvars[0].vval.v_list, NULL) == FAIL)
		    break;
    }
    else
    {
	p = get_tv_string(&argvars[0]);
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = NULL;

	slen = (int)STRLEN(p);
	len = slen * n;
	if (len <= 0)
	    return;

	r = alloc(len + 1);
	if (r != NULL)
	{
	    for (i = 0; i < n; i++)
		mch_memmove(r + i * slen, p, (size_t)slen);
	    r[len] = NUL;
	}

	rettv->vval.v_string = r;
    }
}

/*
 * "resolve()" function
 */
    static void
f_resolve(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p;

    p = get_tv_string(&argvars[0]);
#ifdef FEAT_SHORTCUT
    {
	char_u	*v = NULL;

	v = mch_resolve_shortcut(p);
	if (v != NULL)
	    rettv->vval.v_string = v;
	else
	    rettv->vval.v_string = vim_strsave(p);
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
	if (len > 0 && after_pathsep(p, p + len))
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
		    rettv->vval.v_string = NULL;
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
			cpy = concat_str(q - 1, remain);
			if (cpy != NULL)
			{
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
		STRMOVE(remain, q - 1);
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
		cpy = concat_str((char_u *)"./", p);
		if (cpy != NULL)
		{
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
		    STRMOVE(p, p + 2);
	    }
	}

	/* Ensure that the result will have no trailing path separator
	 * if the argument had none.  But keep "/" or "//". */
	if (!has_trailing_pathsep)
	{
	    q = p + STRLEN(p);
	    if (after_pathsep(p, q))
		*gettail_sep(p) = NUL;
	}

	rettv->vval.v_string = p;
    }
# else
    rettv->vval.v_string = vim_strsave(p);
# endif
#endif

    simplify_filename(rettv->vval.v_string);

#ifdef HAVE_READLINK
fail:
#endif
    rettv->v_type = VAR_STRING;
}

/*
 * "reverse({list})" function
 */
    static void
f_reverse(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    list_T	*l;
    listitem_T	*li, *ni;

    rettv->vval.v_number = 0;
    if (argvars[0].v_type != VAR_LIST)
	EMSG2(_(e_listarg), "reverse()");
    else if ((l = argvars[0].vval.v_list) != NULL
	    && !tv_check_lock(l->lv_lock, (char_u *)"reverse()"))
    {
	li = l->lv_last;
	l->lv_first = l->lv_last = NULL;
	l->lv_len = 0;
	while (li != NULL)
	{
	    ni = li->li_prev;
	    list_append(l, li);
	    li = ni;
	}
	rettv->vval.v_list = l;
	rettv->v_type = VAR_LIST;
	++l->lv_refcount;
	l->lv_idx = l->lv_len - l->lv_idx - 1;
    }
}

#define SP_NOMOVE	0x01	    /* don't move cursor */
#define SP_REPEAT	0x02	    /* repeat to find outer pair */
#define SP_RETCOUNT	0x04	    /* return matchcount */
#define SP_SETPCMARK	0x08	    /* set previous context mark */
#define SP_START	0x10	    /* accept match at start position */
#define SP_SUBPAT	0x20	    /* return nr of matching sub-pattern */
#define SP_END		0x40	    /* leave cursor at end of match */

static int get_search_arg __ARGS((typval_T *varp, int *flagsp));

/*
 * Get flags for a search function.
 * Possibly sets "p_ws".
 * Returns BACKWARD, FORWARD or zero (for an error).
 */
    static int
get_search_arg(varp, flagsp)
    typval_T	*varp;
    int		*flagsp;
{
    int		dir = FORWARD;
    char_u	*flags;
    char_u	nbuf[NUMBUFLEN];
    int		mask;

    if (varp->v_type != VAR_UNKNOWN)
    {
	flags = get_tv_string_buf_chk(varp, nbuf);
	if (flags == NULL)
	    return 0;		/* type error; errmsg already given */
	while (*flags != NUL)
	{
	    switch (*flags)
	    {
		case 'b': dir = BACKWARD; break;
		case 'w': p_ws = TRUE; break;
		case 'W': p_ws = FALSE; break;
		default:  mask = 0;
			  if (flagsp != NULL)
			     switch (*flags)
			     {
				 case 'c': mask = SP_START; break;
				 case 'e': mask = SP_END; break;
				 case 'm': mask = SP_RETCOUNT; break;
				 case 'n': mask = SP_NOMOVE; break;
				 case 'p': mask = SP_SUBPAT; break;
				 case 'r': mask = SP_REPEAT; break;
				 case 's': mask = SP_SETPCMARK; break;
			     }
			  if (mask == 0)
			  {
			      EMSG2(_(e_invarg2), flags);
			      dir = 0;
			  }
			  else
			      *flagsp |= mask;
	    }
	    if (dir == 0)
		break;
	    ++flags;
	}
    }
    return dir;
}

/*
 * Shared by search() and searchpos() functions
 */
    static int
search_cmn(argvars, match_pos, flagsp)
    typval_T	*argvars;
    pos_T	*match_pos;
    int		*flagsp;
{
    int		flags;
    char_u	*pat;
    pos_T	pos;
    pos_T	save_cursor;
    int		save_p_ws = p_ws;
    int		dir;
    int		retval = 0;	/* default: FAIL */
    long	lnum_stop = 0;
    proftime_T	tm;
#ifdef FEAT_RELTIME
    long	time_limit = 0;
#endif
    int		options = SEARCH_KEEP;
    int		subpatnum;

    pat = get_tv_string(&argvars[0]);
    dir = get_search_arg(&argvars[1], flagsp);	/* may set p_ws */
    if (dir == 0)
	goto theend;
    flags = *flagsp;
    if (flags & SP_START)
	options |= SEARCH_START;
    if (flags & SP_END)
	options |= SEARCH_END;

    /* Optional arguments: line number to stop searching and timeout. */
    if (argvars[1].v_type != VAR_UNKNOWN && argvars[2].v_type != VAR_UNKNOWN)
    {
	lnum_stop = get_tv_number_chk(&argvars[2], NULL);
	if (lnum_stop < 0)
	    goto theend;
#ifdef FEAT_RELTIME
	if (argvars[3].v_type != VAR_UNKNOWN)
	{
	    time_limit = get_tv_number_chk(&argvars[3], NULL);
	    if (time_limit < 0)
		goto theend;
	}
#endif
    }

#ifdef FEAT_RELTIME
    /* Set the time limit, if there is one. */
    profile_setlimit(time_limit, &tm);
#endif

    /*
     * This function does not accept SP_REPEAT and SP_RETCOUNT flags.
     * Check to make sure only those flags are set.
     * Also, Only the SP_NOMOVE or the SP_SETPCMARK flag can be set. Both
     * flags cannot be set. Check for that condition also.
     */
    if (((flags & (SP_REPEAT | SP_RETCOUNT)) != 0)
	    || ((flags & SP_NOMOVE) && (flags & SP_SETPCMARK)))
    {
	EMSG2(_(e_invarg2), get_tv_string(&argvars[1]));
	goto theend;
    }

    pos = save_cursor = curwin->w_cursor;
    subpatnum = searchit(curwin, curbuf, &pos, dir, pat, 1L,
				options, RE_SEARCH, (linenr_T)lnum_stop, &tm);
    if (subpatnum != FAIL)
    {
	if (flags & SP_SUBPAT)
	    retval = subpatnum;
	else
	    retval = pos.lnum;
	if (flags & SP_SETPCMARK)
	    setpcmark();
	curwin->w_cursor = pos;
	if (match_pos != NULL)
	{
	    /* Store the match cursor position */
	    match_pos->lnum = pos.lnum;
	    match_pos->col = pos.col + 1;
	}
	/* "/$" will put the cursor after the end of the line, may need to
	 * correct that here */
	check_cursor();
    }

    /* If 'n' flag is used: restore cursor position. */
    if (flags & SP_NOMOVE)
	curwin->w_cursor = save_cursor;
    else
	curwin->w_set_curswant = TRUE;
theend:
    p_ws = save_p_ws;

    return retval;
}

#ifdef FEAT_FLOAT
/*
 * "round({float})" function
 */
    static void
f_round(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	/* round() is not in C90, use ceil() or floor() instead. */
	rettv->vval.v_float = f > 0 ? floor(f + 0.5) : ceil(f - 0.5);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "search()" function
 */
    static void
f_search(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		flags = 0;

    rettv->vval.v_number = search_cmn(argvars, NULL, &flags);
}

/*
 * "searchdecl()" function
 */
    static void
f_searchdecl(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		locally = 1;
    int		thisblock = 0;
    int		error = FALSE;
    char_u	*name;

    rettv->vval.v_number = 1;	/* default: FAIL */

    name = get_tv_string_chk(&argvars[0]);
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	locally = get_tv_number_chk(&argvars[1], &error) == 0;
	if (!error && argvars[2].v_type != VAR_UNKNOWN)
	    thisblock = get_tv_number_chk(&argvars[2], &error) != 0;
    }
    if (!error && name != NULL)
	rettv->vval.v_number = find_decl(name, (int)STRLEN(name),
				     locally, thisblock, SEARCH_KEEP) == FAIL;
}

/*
 * Used by searchpair() and searchpairpos()
 */
    static int
searchpair_cmn(argvars, match_pos)
    typval_T	*argvars;
    pos_T	*match_pos;
{
    char_u	*spat, *mpat, *epat;
    char_u	*skip;
    int		save_p_ws = p_ws;
    int		dir;
    int		flags = 0;
    char_u	nbuf1[NUMBUFLEN];
    char_u	nbuf2[NUMBUFLEN];
    char_u	nbuf3[NUMBUFLEN];
    int		retval = 0;		/* default: FAIL */
    long	lnum_stop = 0;
    long	time_limit = 0;

    /* Get the three pattern arguments: start, middle, end. */
    spat = get_tv_string_chk(&argvars[0]);
    mpat = get_tv_string_buf_chk(&argvars[1], nbuf1);
    epat = get_tv_string_buf_chk(&argvars[2], nbuf2);
    if (spat == NULL || mpat == NULL || epat == NULL)
	goto theend;	    /* type error */

    /* Handle the optional fourth argument: flags */
    dir = get_search_arg(&argvars[3], &flags); /* may set p_ws */
    if (dir == 0)
	goto theend;

    /* Don't accept SP_END or SP_SUBPAT.
     * Only one of the SP_NOMOVE or SP_SETPCMARK flags can be set.
     */
    if ((flags & (SP_END | SP_SUBPAT)) != 0
	    || ((flags & SP_NOMOVE) && (flags & SP_SETPCMARK)))
    {
	EMSG2(_(e_invarg2), get_tv_string(&argvars[3]));
	goto theend;
    }

    /* Using 'r' implies 'W', otherwise it doesn't work. */
    if (flags & SP_REPEAT)
	p_ws = FALSE;

    /* Optional fifth argument: skip expression */
    if (argvars[3].v_type == VAR_UNKNOWN
	    || argvars[4].v_type == VAR_UNKNOWN)
	skip = (char_u *)"";
    else
    {
	skip = get_tv_string_buf_chk(&argvars[4], nbuf3);
	if (argvars[5].v_type != VAR_UNKNOWN)
	{
	    lnum_stop = get_tv_number_chk(&argvars[5], NULL);
	    if (lnum_stop < 0)
		goto theend;
#ifdef FEAT_RELTIME
	    if (argvars[6].v_type != VAR_UNKNOWN)
	    {
		time_limit = get_tv_number_chk(&argvars[6], NULL);
		if (time_limit < 0)
		    goto theend;
	    }
#endif
	}
    }
    if (skip == NULL)
	goto theend;	    /* type error */

    retval = do_searchpair(spat, mpat, epat, dir, skip, flags,
					    match_pos, lnum_stop, time_limit);

theend:
    p_ws = save_p_ws;

    return retval;
}

/*
 * "searchpair()" function
 */
    static void
f_searchpair(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = searchpair_cmn(argvars, NULL);
}

/*
 * "searchpairpos()" function
 */
    static void
f_searchpairpos(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    pos_T	match_pos;
    int		lnum = 0;
    int		col = 0;

    rettv->vval.v_number = 0;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    if (searchpair_cmn(argvars, &match_pos) > 0)
    {
	lnum = match_pos.lnum;
	col = match_pos.col;
    }

    list_append_number(rettv->vval.v_list, (varnumber_T)lnum);
    list_append_number(rettv->vval.v_list, (varnumber_T)col);
}

/*
 * Search for a start/middle/end thing.
 * Used by searchpair(), see its documentation for the details.
 * Returns 0 or -1 for no match,
 */
    long
do_searchpair(spat, mpat, epat, dir, skip, flags, match_pos,
							lnum_stop, time_limit)
    char_u	*spat;	    /* start pattern */
    char_u	*mpat;	    /* middle pattern */
    char_u	*epat;	    /* end pattern */
    int		dir;	    /* BACKWARD or FORWARD */
    char_u	*skip;	    /* skip expression */
    int		flags;	    /* SP_SETPCMARK and other SP_ values */
    pos_T	*match_pos;
    linenr_T	lnum_stop;  /* stop at this line if not zero */
    long	time_limit; /* stop after this many msec */
{
    char_u	*save_cpo;
    char_u	*pat, *pat2 = NULL, *pat3 = NULL;
    long	retval = 0;
    pos_T	pos;
    pos_T	firstpos;
    pos_T	foundpos;
    pos_T	save_cursor;
    pos_T	save_pos;
    int		n;
    int		r;
    int		nest = 1;
    int		err;
    int		options = SEARCH_KEEP;
    proftime_T	tm;

    /* Make 'cpoptions' empty, the 'l' flag should not be used here. */
    save_cpo = p_cpo;
    p_cpo = empty_option;

#ifdef FEAT_RELTIME
    /* Set the time limit, if there is one. */
    profile_setlimit(time_limit, &tm);
#endif

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
    if (flags & SP_START)
	options |= SEARCH_START;

    save_cursor = curwin->w_cursor;
    pos = curwin->w_cursor;
    clearpos(&firstpos);
    clearpos(&foundpos);
    pat = pat3;
    for (;;)
    {
	n = searchit(curwin, curbuf, &pos, dir, pat, 1L,
					   options, RE_SEARCH, lnum_stop, &tm);
	if (n == FAIL || (firstpos.lnum != 0 && equalpos(pos, firstpos)))
	    /* didn't find it or found the first match again: FAIL */
	    break;

	if (firstpos.lnum == 0)
	    firstpos = pos;
	if (equalpos(pos, foundpos))
	{
	    /* Found the same position again.  Can happen with a pattern that
	     * has "\zs" at the end and searching backwards.  Advance one
	     * character and try again. */
	    if (dir == BACKWARD)
		decl(&pos);
	    else
		incl(&pos);
	}
	foundpos = pos;

	/* clear the start flag to avoid getting stuck here */
	options &= ~SEARCH_START;

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
		retval = -1;
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
		++retval;
	    else
		retval = pos.lnum;
	    if (flags & SP_SETPCMARK)
		setpcmark();
	    curwin->w_cursor = pos;
	    if (!(flags & SP_REPEAT))
		break;
	    nest = 1;	    /* search for next unmatched */
	}
    }

    if (match_pos != NULL)
    {
	/* Store the match cursor position */
	match_pos->lnum = curwin->w_cursor.lnum;
	match_pos->col = curwin->w_cursor.col + 1;
    }

    /* If 'n' flag is used or search failed: restore cursor position. */
    if ((flags & SP_NOMOVE) || retval == 0)
	curwin->w_cursor = save_cursor;

theend:
    vim_free(pat2);
    vim_free(pat3);
    if (p_cpo == empty_option)
	p_cpo = save_cpo;
    else
	/* Darn, evaluating the {skip} expression changed the value. */
	free_string_option(save_cpo);

    return retval;
}

/*
 * "searchpos()" function
 */
    static void
f_searchpos(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    pos_T	match_pos;
    int		lnum = 0;
    int		col = 0;
    int		n;
    int		flags = 0;

    rettv->vval.v_number = 0;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    n = search_cmn(argvars, &match_pos, &flags);
    if (n > 0)
    {
	lnum = match_pos.lnum;
	col = match_pos.col;
    }

    list_append_number(rettv->vval.v_list, (varnumber_T)lnum);
    list_append_number(rettv->vval.v_list, (varnumber_T)col);
    if (flags & SP_SUBPAT)
	list_append_number(rettv->vval.v_list, (varnumber_T)n);
}


/*ARGSUSED*/
    static void
f_server2client(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_CLIENTSERVER
    char_u	buf[NUMBUFLEN];
    char_u	*server = get_tv_string_chk(&argvars[0]);
    char_u	*reply = get_tv_string_buf_chk(&argvars[1], buf);

    rettv->vval.v_number = -1;
    if (server == NULL || reply == NULL)
	return;
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
    rettv->vval.v_number = 0;
#else
    rettv->vval.v_number = -1;
#endif
}

/*ARGSUSED*/
    static void
f_serverlist(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
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
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = r;
}

/*
 * "setbufvar()" function
 */
/*ARGSUSED*/
    static void
f_setbufvar(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    buf_T	*buf;
    aco_save_T	aco;
    char_u	*varname, *bufvarname;
    typval_T	*varp;
    char_u	nbuf[NUMBUFLEN];

    rettv->vval.v_number = 0;

    if (check_restricted() || check_secure())
	return;
    (void)get_tv_number(&argvars[0]);	    /* issue errmsg if type error */
    varname = get_tv_string_chk(&argvars[1]);
    buf = get_buf_tv(&argvars[0]);
    varp = &argvars[2];

    if (buf != NULL && varname != NULL && varp != NULL)
    {
	/* set curbuf to be our buf, temporarily */
	aucmd_prepbuf(&aco, buf);

	if (*varname == '&')
	{
	    long	numval;
	    char_u	*strval;
	    int		error = FALSE;

	    ++varname;
	    numval = get_tv_number_chk(varp, &error);
	    strval = get_tv_string_buf_chk(varp, nbuf);
	    if (!error && strval != NULL)
		set_option_value(varname, numval, strval, OPT_LOCAL);
	}
	else
	{
	    bufvarname = alloc((unsigned)STRLEN(varname) + 3);
	    if (bufvarname != NULL)
	    {
		STRCPY(bufvarname, "b:");
		STRCPY(bufvarname + 2, varname);
		set_var(bufvarname, varp, TRUE);
		vim_free(bufvarname);
	    }
	}

	/* reset notion of buffer */
	aucmd_restbuf(&aco);
    }
}

/*
 * "setcmdpos()" function
 */
    static void
f_setcmdpos(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		pos = (int)get_tv_number(&argvars[0]) - 1;

    if (pos >= 0)
	rettv->vval.v_number = set_cmdline_pos(pos);
}

/*
 * "setline()" function
 */
    static void
f_setline(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    linenr_T	lnum;
    char_u	*line = NULL;
    list_T	*l = NULL;
    listitem_T	*li = NULL;
    long	added = 0;
    linenr_T	lcount = curbuf->b_ml.ml_line_count;

    lnum = get_tv_lnum(&argvars[0]);
    if (argvars[1].v_type == VAR_LIST)
    {
	l = argvars[1].vval.v_list;
	li = l->lv_first;
    }
    else
	line = get_tv_string_chk(&argvars[1]);

    rettv->vval.v_number = 0;		/* OK */
    for (;;)
    {
	if (l != NULL)
	{
	    /* list argument, get next string */
	    if (li == NULL)
		break;
	    line = get_tv_string_chk(&li->li_tv);
	    li = li->li_next;
	}

	rettv->vval.v_number = 1;	/* FAIL */
	if (line == NULL || lnum < 1 || lnum > curbuf->b_ml.ml_line_count + 1)
	    break;
	if (lnum <= curbuf->b_ml.ml_line_count)
	{
	    /* existing line, replace it */
	    if (u_savesub(lnum) == OK && ml_replace(lnum, line, TRUE) == OK)
	    {
		changed_bytes(lnum, 0);
		if (lnum == curwin->w_cursor.lnum)
		    check_cursor_col();
		rettv->vval.v_number = 0;	/* OK */
	    }
	}
	else if (added > 0 || u_save(lnum - 1, lnum) == OK)
	{
	    /* lnum is one past the last line, append the line */
	    ++added;
	    if (ml_append(lnum - 1, line, (colnr_T)0, FALSE) == OK)
		rettv->vval.v_number = 0;	/* OK */
	}

	if (l == NULL)			/* only one string argument */
	    break;
	++lnum;
    }

    if (added > 0)
	appended_lines_mark(lcount, added);
}

static void set_qf_ll_list __ARGS((win_T *wp, typval_T *list_arg, typval_T *action_arg, typval_T *rettv));

/*
 * Used by "setqflist()" and "setloclist()" functions
 */
/*ARGSUSED*/
    static void
set_qf_ll_list(wp, list_arg, action_arg, rettv)
    win_T	*wp;
    typval_T	*list_arg;
    typval_T	*action_arg;
    typval_T	*rettv;
{
#ifdef FEAT_QUICKFIX
    char_u	*act;
    int		action = ' ';
#endif

    rettv->vval.v_number = -1;

#ifdef FEAT_QUICKFIX
    if (list_arg->v_type != VAR_LIST)
	EMSG(_(e_listreq));
    else
    {
	list_T  *l = list_arg->vval.v_list;

	if (action_arg->v_type == VAR_STRING)
	{
	    act = get_tv_string_chk(action_arg);
	    if (act == NULL)
		return;		/* type error; errmsg already given */
	    if (*act == 'a' || *act == 'r')
		action = *act;
	}

	if (l != NULL && set_errorlist(wp, l, action) == OK)
	    rettv->vval.v_number = 0;
    }
#endif
}

/*
 * "setloclist()" function
 */
/*ARGSUSED*/
    static void
f_setloclist(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    win_T	*win;

    rettv->vval.v_number = -1;

    win = find_win_by_nr(&argvars[0], NULL);
    if (win != NULL)
	set_qf_ll_list(win, &argvars[1], &argvars[2], rettv);
}

/*
 * "setmatches()" function
 */
    static void
f_setmatches(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_SEARCH_EXTRA
    list_T	*l;
    listitem_T	*li;
    dict_T	*d;

    rettv->vval.v_number = -1;
    if (argvars[0].v_type != VAR_LIST)
    {
	EMSG(_(e_listreq));
	return;
    }
    if ((l = argvars[0].vval.v_list) != NULL)
    {

	/* To some extent make sure that we are dealing with a list from
	 * "getmatches()". */
	li = l->lv_first;
	while (li != NULL)
	{
	    if (li->li_tv.v_type != VAR_DICT
		    || (d = li->li_tv.vval.v_dict) == NULL)
	    {
		EMSG(_(e_invarg));
		return;
	    }
	    if (!(dict_find(d, (char_u *)"group", -1) != NULL
			&& dict_find(d, (char_u *)"pattern", -1) != NULL
			&& dict_find(d, (char_u *)"priority", -1) != NULL
			&& dict_find(d, (char_u *)"id", -1) != NULL))
	    {
		EMSG(_(e_invarg));
		return;
	    }
	    li = li->li_next;
	}

	clear_matches(curwin);
	li = l->lv_first;
	while (li != NULL)
	{
	    d = li->li_tv.vval.v_dict;
	    match_add(curwin, get_dict_string(d, (char_u *)"group", FALSE),
		    get_dict_string(d, (char_u *)"pattern", FALSE),
		    (int)get_dict_number(d, (char_u *)"priority"),
		    (int)get_dict_number(d, (char_u *)"id"));
	    li = li->li_next;
	}
	rettv->vval.v_number = 0;
    }
#endif
}

/*
 * "setpos()" function
 */
/*ARGSUSED*/
    static void
f_setpos(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    pos_T	pos;
    int		fnum;
    char_u	*name;

    rettv->vval.v_number = -1;
    name = get_tv_string_chk(argvars);
    if (name != NULL)
    {
	if (list2fpos(&argvars[1], &pos, &fnum) == OK)
	{
	    --pos.col;
	    if (name[0] == '.' && name[1] == NUL)
	    {
		/* set cursor */
		if (fnum == curbuf->b_fnum)
		{
		    curwin->w_cursor = pos;
		    check_cursor();
		    rettv->vval.v_number = 0;
		}
		else
		    EMSG(_(e_invarg));
	    }
	    else if (name[0] == '\'' && name[1] != NUL && name[2] == NUL)
	    {
		/* set mark */
		if (setmark_pos(name[1], &pos, fnum) == OK)
		    rettv->vval.v_number = 0;
	    }
	    else
		EMSG(_(e_invarg));
	}
    }
}

/*
 * "setqflist()" function
 */
/*ARGSUSED*/
    static void
f_setqflist(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    set_qf_ll_list(NULL, &argvars[0], &argvars[1], rettv);
}

/*
 * "setreg()" function
 */
    static void
f_setreg(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		regname;
    char_u	*strregname;
    char_u	*stropt;
    char_u	*strval;
    int		append;
    char_u	yank_type;
    long	block_len;

    block_len = -1;
    yank_type = MAUTO;
    append = FALSE;

    strregname = get_tv_string_chk(argvars);
    rettv->vval.v_number = 1;		/* FAIL is default */

    if (strregname == NULL)
	return;		/* type error; errmsg already given */
    regname = *strregname;
    if (regname == 0 || regname == '@')
	regname = '"';
    else if (regname == '=')
	return;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	stropt = get_tv_string_chk(&argvars[2]);
	if (stropt == NULL)
	    return;		/* type error */
	for (; *stropt != NUL; ++stropt)
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

    strval = get_tv_string_chk(&argvars[1]);
    if (strval != NULL)
	write_reg_contents_ex(regname, strval, -1,
						append, yank_type, block_len);
    rettv->vval.v_number = 0;
}

/*
 * "settabwinvar()" function
 */
    static void
f_settabwinvar(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    setwinvar(argvars, rettv, 1);
}

/*
 * "setwinvar()" function
 */
    static void
f_setwinvar(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    setwinvar(argvars, rettv, 0);
}

/*
 * "setwinvar()" and "settabwinvar()" functions
 */
    static void
setwinvar(argvars, rettv, off)
    typval_T	*argvars;
    typval_T	*rettv;
    int		off;
{
    win_T	*win;
#ifdef FEAT_WINDOWS
    win_T	*save_curwin;
    tabpage_T	*save_curtab;
#endif
    char_u	*varname, *winvarname;
    typval_T	*varp;
    char_u	nbuf[NUMBUFLEN];
    tabpage_T	*tp;

    rettv->vval.v_number = 0;

    if (check_restricted() || check_secure())
	return;

#ifdef FEAT_WINDOWS
    if (off == 1)
	tp = find_tabpage((int)get_tv_number_chk(&argvars[0], NULL));
    else
	tp = curtab;
#endif
    win = find_win_by_nr(&argvars[off], tp);
    varname = get_tv_string_chk(&argvars[off + 1]);
    varp = &argvars[off + 2];

    if (win != NULL && varname != NULL && varp != NULL)
    {
#ifdef FEAT_WINDOWS
	/* set curwin to be our win, temporarily */
	save_curwin = curwin;
	save_curtab = curtab;
	goto_tabpage_tp(tp);
	if (!win_valid(win))
	    return;
	curwin = win;
	curbuf = curwin->w_buffer;
#endif

	if (*varname == '&')
	{
	    long	numval;
	    char_u	*strval;
	    int		error = FALSE;

	    ++varname;
	    numval = get_tv_number_chk(varp, &error);
	    strval = get_tv_string_buf_chk(varp, nbuf);
	    if (!error && strval != NULL)
		set_option_value(varname, numval, strval, OPT_LOCAL);
	}
	else
	{
	    winvarname = alloc((unsigned)STRLEN(varname) + 3);
	    if (winvarname != NULL)
	    {
		STRCPY(winvarname, "w:");
		STRCPY(winvarname + 2, varname);
		set_var(winvarname, varp, TRUE);
		vim_free(winvarname);
	    }
	}

#ifdef FEAT_WINDOWS
	/* Restore current tabpage and window, if still valid (autocomands can
	 * make them invalid). */
	if (valid_tabpage(save_curtab))
	    goto_tabpage_tp(save_curtab);
	if (win_valid(save_curwin))
	{
	    curwin = save_curwin;
	    curbuf = curwin->w_buffer;
	}
#endif
    }
}

/*
 * "shellescape({string})" function
 */
    static void
f_shellescape(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_string = vim_strsave_shellescape(
		       get_tv_string(&argvars[0]), non_zero_arg(&argvars[1]));
    rettv->v_type = VAR_STRING;
}

/*
 * "simplify()" function
 */
    static void
f_simplify(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p;

    p = get_tv_string(&argvars[0]);
    rettv->vval.v_string = vim_strsave(p);
    simplify_filename(rettv->vval.v_string);	/* simplify in place */
    rettv->v_type = VAR_STRING;
}

#ifdef FEAT_FLOAT
/*
 * "sin()" function
 */
    static void
f_sin(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = sin(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

static int
#ifdef __BORLANDC__
    _RTLENTRYF
#endif
	item_compare __ARGS((const void *s1, const void *s2));
static int
#ifdef __BORLANDC__
    _RTLENTRYF
#endif
	item_compare2 __ARGS((const void *s1, const void *s2));

static int	item_compare_ic;
static char_u	*item_compare_func;
static int	item_compare_func_err;
#define ITEM_COMPARE_FAIL 999

/*
 * Compare functions for f_sort() below.
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
item_compare(s1, s2)
    const void	*s1;
    const void	*s2;
{
    char_u	*p1, *p2;
    char_u	*tofree1, *tofree2;
    int		res;
    char_u	numbuf1[NUMBUFLEN];
    char_u	numbuf2[NUMBUFLEN];

    p1 = tv2string(&(*(listitem_T **)s1)->li_tv, &tofree1, numbuf1, 0);
    p2 = tv2string(&(*(listitem_T **)s2)->li_tv, &tofree2, numbuf2, 0);
    if (p1 == NULL)
	p1 = (char_u *)"";
    if (p2 == NULL)
	p2 = (char_u *)"";
    if (item_compare_ic)
	res = STRICMP(p1, p2);
    else
	res = STRCMP(p1, p2);
    vim_free(tofree1);
    vim_free(tofree2);
    return res;
}

    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
item_compare2(s1, s2)
    const void	*s1;
    const void	*s2;
{
    int		res;
    typval_T	rettv;
    typval_T	argv[3];
    int		dummy;

    /* shortcut after failure in previous call; compare all items equal */
    if (item_compare_func_err)
	return 0;

    /* copy the values.  This is needed to be able to set v_lock to VAR_FIXED
     * in the copy without changing the original list items. */
    copy_tv(&(*(listitem_T **)s1)->li_tv, &argv[0]);
    copy_tv(&(*(listitem_T **)s2)->li_tv, &argv[1]);

    rettv.v_type = VAR_UNKNOWN;		/* clear_tv() uses this */
    res = call_func(item_compare_func, (int)STRLEN(item_compare_func),
				 &rettv, 2, argv, 0L, 0L, &dummy, TRUE, NULL);
    clear_tv(&argv[0]);
    clear_tv(&argv[1]);

    if (res == FAIL)
	res = ITEM_COMPARE_FAIL;
    else
	res = get_tv_number_chk(&rettv, &item_compare_func_err);
    if (item_compare_func_err)
	res = ITEM_COMPARE_FAIL;  /* return value has wrong type */
    clear_tv(&rettv);
    return res;
}

/*
 * "sort({list})" function
 */
    static void
f_sort(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    list_T	*l;
    listitem_T	*li;
    listitem_T	**ptrs;
    long	len;
    long	i;

    rettv->vval.v_number = 0;
    if (argvars[0].v_type != VAR_LIST)
	EMSG2(_(e_listarg), "sort()");
    else
    {
	l = argvars[0].vval.v_list;
	if (l == NULL || tv_check_lock(l->lv_lock, (char_u *)"sort()"))
	    return;
	rettv->vval.v_list = l;
	rettv->v_type = VAR_LIST;
	++l->lv_refcount;

	len = list_len(l);
	if (len <= 1)
	    return;	/* short list sorts pretty quickly */

	item_compare_ic = FALSE;
	item_compare_func = NULL;
	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    if (argvars[1].v_type == VAR_FUNC)
		item_compare_func = argvars[1].vval.v_string;
	    else
	    {
		int	    error = FALSE;

		i = get_tv_number_chk(&argvars[1], &error);
		if (error)
		    return;		/* type error; errmsg already given */
		if (i == 1)
		    item_compare_ic = TRUE;
		else
		    item_compare_func = get_tv_string(&argvars[1]);
	    }
	}

	/* Make an array with each entry pointing to an item in the List. */
	ptrs = (listitem_T **)alloc((int)(len * sizeof(listitem_T *)));
	if (ptrs == NULL)
	    return;
	i = 0;
	for (li = l->lv_first; li != NULL; li = li->li_next)
	    ptrs[i++] = li;

	item_compare_func_err = FALSE;
	/* test the compare function */
	if (item_compare_func != NULL
		&& item_compare2((void *)&ptrs[0], (void *)&ptrs[1])
							 == ITEM_COMPARE_FAIL)
	    EMSG(_("E702: Sort compare function failed"));
	else
	{
	    /* Sort the array with item pointers. */
	    qsort((void *)ptrs, (size_t)len, sizeof(listitem_T *),
		    item_compare_func == NULL ? item_compare : item_compare2);

	    if (!item_compare_func_err)
	    {
		/* Clear the List and append the items in the sorted order. */
		l->lv_first = l->lv_last = l->lv_idx_item = NULL;
		l->lv_len = 0;
		for (i = 0; i < len; ++i)
		    list_append(l, ptrs[i]);
	    }
	}

	vim_free(ptrs);
    }
}

/*
 * "soundfold({word})" function
 */
    static void
f_soundfold(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*s;

    rettv->v_type = VAR_STRING;
    s = get_tv_string(&argvars[0]);
#ifdef FEAT_SPELL
    rettv->vval.v_string = eval_soundfold(s);
#else
    rettv->vval.v_string = vim_strsave(s);
#endif
}

/*
 * "spellbadword()" function
 */
/* ARGSUSED */
    static void
f_spellbadword(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*word = (char_u *)"";
    hlf_T	attr = HLF_COUNT;
    int		len = 0;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

#ifdef FEAT_SPELL
    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	/* Find the start and length of the badly spelled word. */
	len = spell_move_to(curwin, FORWARD, TRUE, TRUE, &attr);
	if (len != 0)
	    word = ml_get_cursor();
    }
    else if (curwin->w_p_spell && *curbuf->b_p_spl != NUL)
    {
	char_u	*str = get_tv_string_chk(&argvars[0]);
	int	capcol = -1;

	if (str != NULL)
	{
	    /* Check the argument for spelling. */
	    while (*str != NUL)
	    {
		len = spell_check(curwin, str, &attr, &capcol, FALSE);
		if (attr != HLF_COUNT)
		{
		    word = str;
		    break;
		}
		str += len;
	    }
	}
    }
#endif

    list_append_string(rettv->vval.v_list, word, len);
    list_append_string(rettv->vval.v_list, (char_u *)(
			attr == HLF_SPB ? "bad" :
			attr == HLF_SPR ? "rare" :
			attr == HLF_SPL ? "local" :
			attr == HLF_SPC ? "caps" :
			""), -1);
}

/*
 * "spellsuggest()" function
 */
/*ARGSUSED*/
    static void
f_spellsuggest(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_SPELL
    char_u	*str;
    int		typeerr = FALSE;
    int		maxcount;
    garray_T	ga;
    int		i;
    listitem_T	*li;
    int		need_capital = FALSE;
#endif

    if (rettv_list_alloc(rettv) == FAIL)
	return;

#ifdef FEAT_SPELL
    if (curwin->w_p_spell && *curbuf->b_p_spl != NUL)
    {
	str = get_tv_string(&argvars[0]);
	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    maxcount = get_tv_number_chk(&argvars[1], &typeerr);
	    if (maxcount <= 0)
		return;
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		need_capital = get_tv_number_chk(&argvars[2], &typeerr);
		if (typeerr)
		    return;
	    }
	}
	else
	    maxcount = 25;

	spell_suggest_list(&ga, str, maxcount, need_capital, FALSE);

	for (i = 0; i < ga.ga_len; ++i)
	{
	    str = ((char_u **)ga.ga_data)[i];

	    li = listitem_alloc();
	    if (li == NULL)
		vim_free(str);
	    else
	    {
		li->li_tv.v_type = VAR_STRING;
		li->li_tv.v_lock = 0;
		li->li_tv.vval.v_string = str;
		list_append(rettv->vval.v_list, li);
	    }
	}
	ga_clear(&ga);
    }
#endif
}

    static void
f_split(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*str;
    char_u	*end;
    char_u	*pat = NULL;
    regmatch_T	regmatch;
    char_u	patbuf[NUMBUFLEN];
    char_u	*save_cpo;
    int		match;
    colnr_T	col = 0;
    int		keepempty = FALSE;
    int		typeerr = FALSE;

    /* Make 'cpoptions' empty, the 'l' flag should not be used here. */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    str = get_tv_string(&argvars[0]);
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	pat = get_tv_string_buf_chk(&argvars[1], patbuf);
	if (pat == NULL)
	    typeerr = TRUE;
	if (argvars[2].v_type != VAR_UNKNOWN)
	    keepempty = get_tv_number_chk(&argvars[2], &typeerr);
    }
    if (pat == NULL || *pat == NUL)
	pat = (char_u *)"[\\x01- ]\\+";

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    if (typeerr)
	return;

    regmatch.regprog = vim_regcomp(pat, RE_MAGIC + RE_STRING);
    if (regmatch.regprog != NULL)
    {
	regmatch.rm_ic = FALSE;
	while (*str != NUL || keepempty)
	{
	    if (*str == NUL)
		match = FALSE;	/* empty item at the end */
	    else
		match = vim_regexec_nl(&regmatch, str, col);
	    if (match)
		end = regmatch.startp[0];
	    else
		end = str + STRLEN(str);
	    if (keepempty || end > str || (rettv->vval.v_list->lv_len > 0
			   && *str != NUL && match && end < regmatch.endp[0]))
	    {
		if (list_append_string(rettv->vval.v_list, str,
						    (int)(end - str)) == FAIL)
		    break;
	    }
	    if (!match)
		break;
	    /* Advance to just after the match. */
	    if (regmatch.endp[0] > str)
		col = 0;
	    else
	    {
		/* Don't get stuck at the same match. */
#ifdef FEAT_MBYTE
		col = (*mb_ptr2len)(regmatch.endp[0]);
#else
		col = 1;
#endif
	    }
	    str = regmatch.endp[0];
	}

	vim_free(regmatch.regprog);
    }

    p_cpo = save_cpo;
}

#ifdef FEAT_FLOAT
/*
 * "sqrt()" function
 */
    static void
f_sqrt(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = sqrt(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "str2float()" function
 */
    static void
f_str2float(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u *p = skipwhite(get_tv_string(&argvars[0]));

    if (*p == '+')
	p = skipwhite(p + 1);
    (void)string2float(p, &rettv->vval.v_float);
    rettv->v_type = VAR_FLOAT;
}
#endif

/*
 * "str2nr()" function
 */
    static void
f_str2nr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		base = 10;
    char_u	*p;
    long	n;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	base = get_tv_number(&argvars[1]);
	if (base != 8 && base != 10 && base != 16)
	{
	    EMSG(_(e_invarg));
	    return;
	}
    }

    p = skipwhite(get_tv_string(&argvars[0]));
    if (*p == '+')
	p = skipwhite(p + 1);
    vim_str2nr(p, NULL, NULL, base == 8 ? 2 : 0, base == 16 ? 2 : 0, &n, NULL);
    rettv->vval.v_number = n;
}

#ifdef HAVE_STRFTIME
/*
 * "strftime({format}[, {time}])" function
 */
    static void
f_strftime(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	result_buf[256];
    struct tm	*curtime;
    time_t	seconds;
    char_u	*p;

    rettv->v_type = VAR_STRING;

    p = get_tv_string(&argvars[0]);
    if (argvars[1].v_type == VAR_UNKNOWN)
	seconds = time(NULL);
    else
	seconds = (time_t)get_tv_number(&argvars[1]);
    curtime = localtime(&seconds);
    /* MSVC returns NULL for an invalid value of seconds. */
    if (curtime == NULL)
	rettv->vval.v_string = vim_strsave((char_u *)_("(Invalid)"));
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
	    rettv->vval.v_string = string_convert(&conv, result_buf, NULL);
	else
# endif
	    rettv->vval.v_string = vim_strsave(result_buf);

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
f_stridx(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*save_haystack;
    char_u	*pos;
    int		start_idx;

    needle = get_tv_string_chk(&argvars[1]);
    save_haystack = haystack = get_tv_string_buf_chk(&argvars[0], buf);
    rettv->vval.v_number = -1;
    if (needle == NULL || haystack == NULL)
	return;		/* type error; errmsg already given */

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	int	    error = FALSE;

	start_idx = get_tv_number_chk(&argvars[2], &error);
	if (error || start_idx >= (int)STRLEN(haystack))
	    return;
	if (start_idx >= 0)
	    haystack += start_idx;
    }

    pos	= (char_u *)strstr((char *)haystack, (char *)needle);
    if (pos != NULL)
	rettv->vval.v_number = (varnumber_T)(pos - save_haystack);
}

/*
 * "string()" function
 */
    static void
f_string(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = tv2string(&argvars[0], &tofree, numbuf, 0);
    /* Make a copy if we have a value but it's not in allocated memory. */
    if (rettv->vval.v_string != NULL && tofree == NULL)
	rettv->vval.v_string = vim_strsave(rettv->vval.v_string);
}

/*
 * "strlen()" function
 */
    static void
f_strlen(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->vval.v_number = (varnumber_T)(STRLEN(
					      get_tv_string(&argvars[0])));
}

/*
 * "strpart()" function
 */
    static void
f_strpart(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p;
    int		n;
    int		len;
    int		slen;
    int		error = FALSE;

    p = get_tv_string(&argvars[0]);
    slen = (int)STRLEN(p);

    n = get_tv_number_chk(&argvars[1], &error);
    if (error)
	len = 0;
    else if (argvars[2].v_type != VAR_UNKNOWN)
	len = get_tv_number(&argvars[2]);
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

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strnsave(p + n, len);
}

/*
 * "strridx()" function
 */
    static void
f_strridx(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*rest;
    char_u	*lastmatch = NULL;
    int		haystack_len, end_idx;

    needle = get_tv_string_chk(&argvars[1]);
    haystack = get_tv_string_buf_chk(&argvars[0], buf);

    rettv->vval.v_number = -1;
    if (needle == NULL || haystack == NULL)
	return;		/* type error; errmsg already given */

    haystack_len = (int)STRLEN(haystack);
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	/* Third argument: upper limit for index */
	end_idx = get_tv_number_chk(&argvars[2], NULL);
	if (end_idx < 0)
	    return;	/* can never find a match */
    }
    else
	end_idx = haystack_len;

    if (*needle == NUL)
    {
	/* Empty string matches past the end. */
	lastmatch = haystack + end_idx;
    }
    else
    {
	for (rest = haystack; *rest != '\0'; ++rest)
	{
	    rest = (char_u *)strstr((char *)rest, (char *)needle);
	    if (rest == NULL || rest > haystack + end_idx)
		break;
	    lastmatch = rest;
	}
    }

    if (lastmatch == NULL)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = (varnumber_T)(lastmatch - haystack);
}

/*
 * "strtrans()" function
 */
    static void
f_strtrans(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = transstr(get_tv_string(&argvars[0]));
}

/*
 * "submatch()" function
 */
    static void
f_submatch(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string =
		    reg_submatch((int)get_tv_number_chk(&argvars[0], NULL));
}

/*
 * "substitute()" function
 */
    static void
f_substitute(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	patbuf[NUMBUFLEN];
    char_u	subbuf[NUMBUFLEN];
    char_u	flagsbuf[NUMBUFLEN];

    char_u	*str = get_tv_string_chk(&argvars[0]);
    char_u	*pat = get_tv_string_buf_chk(&argvars[1], patbuf);
    char_u	*sub = get_tv_string_buf_chk(&argvars[2], subbuf);
    char_u	*flg = get_tv_string_buf_chk(&argvars[3], flagsbuf);

    rettv->v_type = VAR_STRING;
    if (str == NULL || pat == NULL || sub == NULL || flg == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = do_string_sub(str, pat, sub, flg);
}

/*
 * "synID(lnum, col, trans)" function
 */
/*ARGSUSED*/
    static void
f_synID(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		id = 0;
#ifdef FEAT_SYN_HL
    long	lnum;
    long	col;
    int		trans;
    int		transerr = FALSE;

    lnum = get_tv_lnum(argvars);		/* -1 on type error */
    col = get_tv_number(&argvars[1]) - 1;	/* -1 on type error */
    trans = get_tv_number_chk(&argvars[2], &transerr);

    if (!transerr && lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count
	    && col >= 0 && col < (long)STRLEN(ml_get(lnum)))
	id = syn_get_id(curwin, lnum, (colnr_T)col, trans, NULL, FALSE);
#endif

    rettv->vval.v_number = id;
}

/*
 * "synIDattr(id, what [, mode])" function
 */
/*ARGSUSED*/
    static void
f_synIDattr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p = NULL;
#ifdef FEAT_SYN_HL
    int		id;
    char_u	*what;
    char_u	*mode;
    char_u	modebuf[NUMBUFLEN];
    int		modec;

    id = get_tv_number(&argvars[0]);
    what = get_tv_string(&argvars[1]);
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	mode = get_tv_string_buf(&argvars[2], modebuf);
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

	case 's':
		if (TOLOWER_ASC(what[1]) == 'p')	/* sp[#] */
		    p = highlight_color(id, what, modec);
		else					/* standout */
		    p = highlight_has_attr(id, HL_STANDOUT, modec);
		break;

	case 'u':
		if (STRLEN(what) <= 5 || TOLOWER_ASC(what[5]) != 'c')
							/* underline */
		    p = highlight_has_attr(id, HL_UNDERLINE, modec);
		else
							/* undercurl */
		    p = highlight_has_attr(id, HL_UNDERCURL, modec);
		break;
    }

    if (p != NULL)
	p = vim_strsave(p);
#endif
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = p;
}

/*
 * "synIDtrans(id)" function
 */
/*ARGSUSED*/
    static void
f_synIDtrans(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		id;

#ifdef FEAT_SYN_HL
    id = get_tv_number(&argvars[0]);

    if (id > 0)
	id = syn_get_final_id(id);
    else
#endif
	id = 0;

    rettv->vval.v_number = id;
}

/*
 * "synstack(lnum, col)" function
 */
/*ARGSUSED*/
    static void
f_synstack(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_SYN_HL
    long	lnum;
    long	col;
    int		i;
    int		id;
#endif

    rettv->v_type = VAR_LIST;
    rettv->vval.v_list = NULL;

#ifdef FEAT_SYN_HL
    lnum = get_tv_lnum(argvars);		/* -1 on type error */
    col = get_tv_number(&argvars[1]) - 1;	/* -1 on type error */

    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count
	    && col >= 0 && (col == 0 || col < (long)STRLEN(ml_get(lnum)))
	    && rettv_list_alloc(rettv) != FAIL)
    {
	(void)syn_get_id(curwin, lnum, (colnr_T)col, FALSE, NULL, TRUE);
	for (i = 0; ; ++i)
	{
	    id = syn_get_stack_item(i);
	    if (id < 0)
		break;
	    if (list_append_number(rettv->vval.v_list, id) == FAIL)
		break;
	}
    }
#endif
}

/*
 * "system()" function
 */
    static void
f_system(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*res = NULL;
    char_u	*p;
    char_u	*infile = NULL;
    char_u	buf[NUMBUFLEN];
    int		err = FALSE;
    FILE	*fd;

    if (check_restricted() || check_secure())
	goto done;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	/*
	 * Write the string to a temp file, to be used for input of the shell
	 * command.
	 */
	if ((infile = vim_tempname('i')) == NULL)
	{
	    EMSG(_(e_notmp));
	    goto done;
	}

	fd = mch_fopen((char *)infile, WRITEBIN);
	if (fd == NULL)
	{
	    EMSG2(_(e_notopen), infile);
	    goto done;
	}
	p = get_tv_string_buf_chk(&argvars[1], buf);
	if (p == NULL)
	{
	    fclose(fd);
	    goto done;		/* type error; errmsg already given */
	}
	if (fwrite(p, STRLEN(p), 1, fd) != 1)
	    err = TRUE;
	if (fclose(fd) != 0)
	    err = TRUE;
	if (err)
	{
	    EMSG(_("E677: Error writing temp file"));
	    goto done;
	}
    }

    res = get_cmd_output(get_tv_string(&argvars[0]), infile,
						 SHELL_SILENT | SHELL_COOKED);

#ifdef USE_CR
    /* translate <CR> into <NL> */
    if (res != NULL)
    {
	char_u	*s;

	for (s = res; *s; ++s)
	{
	    if (*s == CAR)
		*s = NL;
	}
    }
#else
# ifdef USE_CRNL
    /* translate <CR><NL> into <NL> */
    if (res != NULL)
    {
	char_u	*s, *d;

	d = res;
	for (s = res; *s; ++s)
	{
	    if (s[0] == CAR && s[1] == NL)
		++s;
	    *d++ = *s;
	}
	*d = NUL;
    }
# endif
#endif

done:
    if (infile != NULL)
    {
	mch_remove(infile);
	vim_free(infile);
    }
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = res;
}

/*
 * "tabpagebuflist()" function
 */
/* ARGSUSED */
    static void
f_tabpagebuflist(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifndef FEAT_WINDOWS
    rettv->vval.v_number = 0;
#else
    tabpage_T	*tp;
    win_T	*wp = NULL;

    if (argvars[0].v_type == VAR_UNKNOWN)
	wp = firstwin;
    else
    {
	tp = find_tabpage((int)get_tv_number(&argvars[0]));
	if (tp != NULL)
	    wp = (tp == curtab) ? firstwin : tp->tp_firstwin;
    }
    if (wp == NULL)
	rettv->vval.v_number = 0;
    else
    {
	if (rettv_list_alloc(rettv) == FAIL)
	    rettv->vval.v_number = 0;
	else
	{
	    for (; wp != NULL; wp = wp->w_next)
		if (list_append_number(rettv->vval.v_list,
						wp->w_buffer->b_fnum) == FAIL)
		    break;
	}
    }
#endif
}


/*
 * "tabpagenr()" function
 */
/* ARGSUSED */
    static void
f_tabpagenr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		nr = 1;
#ifdef FEAT_WINDOWS
    char_u	*arg;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	arg = get_tv_string_chk(&argvars[0]);
	nr = 0;
	if (arg != NULL)
	{
	    if (STRCMP(arg, "$") == 0)
		nr = tabpage_index(NULL) - 1;
	    else
		EMSG2(_(e_invexpr2), arg);
	}
    }
    else
	nr = tabpage_index(curtab);
#endif
    rettv->vval.v_number = nr;
}


#ifdef FEAT_WINDOWS
static int get_winnr __ARGS((tabpage_T *tp, typval_T *argvar));

/*
 * Common code for tabpagewinnr() and winnr().
 */
    static int
get_winnr(tp, argvar)
    tabpage_T	*tp;
    typval_T	*argvar;
{
    win_T	*twin;
    int		nr = 1;
    win_T	*wp;
    char_u	*arg;

    twin = (tp == curtab) ? curwin : tp->tp_curwin;
    if (argvar->v_type != VAR_UNKNOWN)
    {
	arg = get_tv_string_chk(argvar);
	if (arg == NULL)
	    nr = 0;		/* type error; errmsg already given */
	else if (STRCMP(arg, "$") == 0)
	    twin = (tp == curtab) ? lastwin : tp->tp_lastwin;
	else if (STRCMP(arg, "#") == 0)
	{
	    twin = (tp == curtab) ? prevwin : tp->tp_prevwin;
	    if (twin == NULL)
		nr = 0;
	}
	else
	{
	    EMSG2(_(e_invexpr2), arg);
	    nr = 0;
	}
    }

    if (nr > 0)
	for (wp = (tp == curtab) ? firstwin : tp->tp_firstwin;
					      wp != twin; wp = wp->w_next)
	{
	    if (wp == NULL)
	    {
		/* didn't find it in this tabpage */
		nr = 0;
		break;
	    }
	    ++nr;
	}
    return nr;
}
#endif

/*
 * "tabpagewinnr()" function
 */
/* ARGSUSED */
    static void
f_tabpagewinnr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		nr = 1;
#ifdef FEAT_WINDOWS
    tabpage_T	*tp;

    tp = find_tabpage((int)get_tv_number(&argvars[0]));
    if (tp == NULL)
	nr = 0;
    else
	nr = get_winnr(tp, &argvars[1]);
#endif
    rettv->vval.v_number = nr;
}


/*
 * "tagfiles()" function
 */
/*ARGSUSED*/
    static void
f_tagfiles(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	fname[MAXPATHL + 1];
    tagname_T	tn;
    int		first;

    if (rettv_list_alloc(rettv) == FAIL)
    {
	rettv->vval.v_number = 0;
	return;
    }

    for (first = TRUE; ; first = FALSE)
	if (get_tagfname(&tn, first, fname) == FAIL
		|| list_append_string(rettv->vval.v_list, fname, -1) == FAIL)
	    break;
    tagname_free(&tn);
}

/*
 * "taglist()" function
 */
    static void
f_taglist(argvars, rettv)
    typval_T  *argvars;
    typval_T  *rettv;
{
    char_u  *tag_pattern;

    tag_pattern = get_tv_string(&argvars[0]);

    rettv->vval.v_number = FALSE;
    if (*tag_pattern == NUL)
	return;

    if (rettv_list_alloc(rettv) == OK)
	(void)get_tags(rettv->vval.v_list, tag_pattern);
}

/*
 * "tempname()" function
 */
/*ARGSUSED*/
    static void
f_tempname(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    static int	x = 'A';

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_tempname(x);

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
 * "test(list)" function: Just checking the walls...
 */
/*ARGSUSED*/
    static void
f_test(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    /* Used for unit testing.  Change the code below to your liking. */
#if 0
    listitem_T	*li;
    list_T	*l;
    char_u	*bad, *good;

    if (argvars[0].v_type != VAR_LIST)
	return;
    l = argvars[0].vval.v_list;
    if (l == NULL)
	return;
    li = l->lv_first;
    if (li == NULL)
	return;
    bad = get_tv_string(&li->li_tv);
    li = li->li_next;
    if (li == NULL)
	return;
    good = get_tv_string(&li->li_tv);
    rettv->vval.v_number = test_edit_score(bad, good);
#endif
}

/*
 * "tolower(string)" function
 */
    static void
f_tolower(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*p;

    p = vim_strsave(get_tv_string(&argvars[0]));
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = p;

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
		l = utf_ptr2len(p);
		/* TODO: reallocate string when byte count changes. */
		if (utf_char2len(lc) == l)
		    utf_char2bytes(lc, p);
		p += l;
	    }
	    else if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
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
f_toupper(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = strup_save(get_tv_string(&argvars[0]));
}

/*
 * "tr(string, fromstr, tostr)" function
 */
    static void
f_tr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    char_u	*instr;
    char_u	*fromstr;
    char_u	*tostr;
    char_u	*p;
#ifdef FEAT_MBYTE
    int		inlen;
    int		fromlen;
    int		tolen;
    int		idx;
    char_u	*cpstr;
    int		cplen;
    int		first = TRUE;
#endif
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    garray_T	ga;

    instr = get_tv_string(&argvars[0]);
    fromstr = get_tv_string_buf_chk(&argvars[1], buf);
    tostr = get_tv_string_buf_chk(&argvars[2], buf2);

    /* Default return value: empty string. */
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    if (fromstr == NULL || tostr == NULL)
	    return;		/* type error; errmsg already given */
    ga_init2(&ga, (int)sizeof(char), 80);

#ifdef FEAT_MBYTE
    if (!has_mbyte)
#endif
	/* not multi-byte: fromstr and tostr must be the same length */
	if (STRLEN(fromstr) != STRLEN(tostr))
	{
#ifdef FEAT_MBYTE
error:
#endif
	    EMSG2(_(e_invarg2), fromstr);
	    ga_clear(&ga);
	    return;
	}

    /* fromstr and tostr have to contain the same number of chars */
    while (*instr != NUL)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    inlen = (*mb_ptr2len)(instr);
	    cpstr = instr;
	    cplen = inlen;
	    idx = 0;
	    for (p = fromstr; *p != NUL; p += fromlen)
	    {
		fromlen = (*mb_ptr2len)(p);
		if (fromlen == inlen && STRNCMP(instr, p, inlen) == 0)
		{
		    for (p = tostr; *p != NUL; p += tolen)
		    {
			tolen = (*mb_ptr2len)(p);
			if (idx-- == 0)
			{
			    cplen = tolen;
			    cpstr = p;
			    break;
			}
		    }
		    if (*p == NUL)	/* tostr is shorter than fromstr */
			goto error;
		    break;
		}
		++idx;
	    }

	    if (first && cpstr == instr)
	    {
		/* Check that fromstr and tostr have the same number of
		 * (multi-byte) characters.  Done only once when a character
		 * of instr doesn't appear in fromstr. */
		first = FALSE;
		for (p = tostr; *p != NUL; p += tolen)
		{
		    tolen = (*mb_ptr2len)(p);
		    --idx;
		}
		if (idx != 0)
		    goto error;
	    }

	    ga_grow(&ga, cplen);
	    mch_memmove((char *)ga.ga_data + ga.ga_len, cpstr, (size_t)cplen);
	    ga.ga_len += cplen;

	    instr += inlen;
	}
	else
#endif
	{
	    /* When not using multi-byte chars we can do it faster. */
	    p = vim_strchr(fromstr, *instr);
	    if (p != NULL)
		ga_append(&ga, tostr[p - fromstr]);
	    else
		ga_append(&ga, *instr);
	    ++instr;
	}
    }

    /* add a terminating NUL */
    ga_grow(&ga, 1);
    ga_append(&ga, NUL);

    rettv->vval.v_string = ga.ga_data;
}

#ifdef FEAT_FLOAT
/*
 * "trunc({float})" function
 */
    static void
f_trunc(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    float_T	f;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	/* trunc() is not in C90, use floor() or ceil() instead. */
	rettv->vval.v_float = f > 0 ? floor(f) : ceil(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "type(expr)" function
 */
    static void
f_type(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int n;

    switch (argvars[0].v_type)
    {
	case VAR_NUMBER: n = 0; break;
	case VAR_STRING: n = 1; break;
	case VAR_FUNC:   n = 2; break;
	case VAR_LIST:   n = 3; break;
	case VAR_DICT:   n = 4; break;
#ifdef FEAT_FLOAT
	case VAR_FLOAT:  n = 5; break;
#endif
	default: EMSG2(_(e_intern2), "f_type()"); n = 0; break;
    }
    rettv->vval.v_number = n;
}

/*
 * "values(dict)" function
 */
    static void
f_values(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    dict_list(argvars, rettv, 1);
}

/*
 * "virtcol(string)" function
 */
    static void
f_virtcol(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    colnr_T	vcol = 0;
    pos_T	*fp;
    int		fnum = curbuf->b_fnum;

    fp = var2fpos(&argvars[0], FALSE, &fnum);
    if (fp != NULL && fp->lnum <= curbuf->b_ml.ml_line_count
						    && fnum == curbuf->b_fnum)
    {
	getvvcol(curwin, fp, NULL, NULL, &vcol);
	++vcol;
    }

    rettv->vval.v_number = vcol;
}

/*
 * "visualmode()" function
 */
/*ARGSUSED*/
    static void
f_visualmode(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
#ifdef FEAT_VISUAL
    char_u	str[2];

    rettv->v_type = VAR_STRING;
    str[0] = curbuf->b_visual_mode_eval;
    str[1] = NUL;
    rettv->vval.v_string = vim_strsave(str);

    /* A non-zero number or non-empty string argument: reset mode. */
    if (non_zero_arg(&argvars[0]))
	curbuf->b_visual_mode_eval = NUL;
#else
    rettv->vval.v_number = 0; /* return anything, it won't work anyway */
#endif
}

/*
 * "winbufnr(nr)" function
 */
    static void
f_winbufnr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    win_T	*wp;

    wp = find_win_by_nr(&argvars[0], NULL);
    if (wp == NULL)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = wp->w_buffer->b_fnum;
}

/*
 * "wincol()" function
 */
/*ARGSUSED*/
    static void
f_wincol(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    validate_cursor();
    rettv->vval.v_number = curwin->w_wcol + 1;
}

/*
 * "winheight(nr)" function
 */
    static void
f_winheight(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    win_T	*wp;

    wp = find_win_by_nr(&argvars[0], NULL);
    if (wp == NULL)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = wp->w_height;
}

/*
 * "winline()" function
 */
/*ARGSUSED*/
    static void
f_winline(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    validate_cursor();
    rettv->vval.v_number = curwin->w_wrow + 1;
}

/*
 * "winnr()" function
 */
/* ARGSUSED */
    static void
f_winnr(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		nr = 1;

#ifdef FEAT_WINDOWS
    nr = get_winnr(curtab, &argvars[0]);
#endif
    rettv->vval.v_number = nr;
}

/*
 * "winrestcmd()" function
 */
/* ARGSUSED */
    static void
f_winrestcmd(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
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
    ga_append(&ga, NUL);

    rettv->vval.v_string = ga.ga_data;
#else
    rettv->vval.v_string = NULL;
#endif
    rettv->v_type = VAR_STRING;
}

/*
 * "winrestview()" function
 */
/* ARGSUSED */
    static void
f_winrestview(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    dict_T	*dict;

    if (argvars[0].v_type != VAR_DICT
	    || (dict = argvars[0].vval.v_dict) == NULL)
	EMSG(_(e_invarg));
    else
    {
	curwin->w_cursor.lnum = get_dict_number(dict, (char_u *)"lnum");
	curwin->w_cursor.col = get_dict_number(dict, (char_u *)"col");
#ifdef FEAT_VIRTUALEDIT
	curwin->w_cursor.coladd = get_dict_number(dict, (char_u *)"coladd");
#endif
	curwin->w_curswant = get_dict_number(dict, (char_u *)"curswant");
	curwin->w_set_curswant = FALSE;

	set_topline(curwin, get_dict_number(dict, (char_u *)"topline"));
#ifdef FEAT_DIFF
	curwin->w_topfill = get_dict_number(dict, (char_u *)"topfill");
#endif
	curwin->w_leftcol = get_dict_number(dict, (char_u *)"leftcol");
	curwin->w_skipcol = get_dict_number(dict, (char_u *)"skipcol");

	check_cursor();
	changed_cline_bef_curs();
	invalidate_botline();
	redraw_later(VALID);

	if (curwin->w_topline == 0)
	    curwin->w_topline = 1;
	if (curwin->w_topline > curbuf->b_ml.ml_line_count)
	    curwin->w_topline = curbuf->b_ml.ml_line_count;
#ifdef FEAT_DIFF
	check_topfill(curwin, TRUE);
#endif
    }
}

/*
 * "winsaveview()" function
 */
/* ARGSUSED */
    static void
f_winsaveview(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    dict_T	*dict;

    dict = dict_alloc();
    if (dict == NULL)
	return;
    rettv->v_type = VAR_DICT;
    rettv->vval.v_dict = dict;
    ++dict->dv_refcount;

    dict_add_nr_str(dict, "lnum", (long)curwin->w_cursor.lnum, NULL);
    dict_add_nr_str(dict, "col", (long)curwin->w_cursor.col, NULL);
#ifdef FEAT_VIRTUALEDIT
    dict_add_nr_str(dict, "coladd", (long)curwin->w_cursor.coladd, NULL);
#endif
    update_curswant();
    dict_add_nr_str(dict, "curswant", (long)curwin->w_curswant, NULL);

    dict_add_nr_str(dict, "topline", (long)curwin->w_topline, NULL);
#ifdef FEAT_DIFF
    dict_add_nr_str(dict, "topfill", (long)curwin->w_topfill, NULL);
#endif
    dict_add_nr_str(dict, "leftcol", (long)curwin->w_leftcol, NULL);
    dict_add_nr_str(dict, "skipcol", (long)curwin->w_skipcol, NULL);
}

/*
 * "winwidth(nr)" function
 */
    static void
f_winwidth(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    win_T	*wp;

    wp = find_win_by_nr(&argvars[0], NULL);
    if (wp == NULL)
	rettv->vval.v_number = -1;
    else
#ifdef FEAT_VERTSPLIT
	rettv->vval.v_number = wp->w_width;
#else
	rettv->vval.v_number = Columns;
#endif
}

/*
 * "writefile()" function
 */
    static void
f_writefile(argvars, rettv)
    typval_T	*argvars;
    typval_T	*rettv;
{
    int		binary = FALSE;
    char_u	*fname;
    FILE	*fd;
    listitem_T	*li;
    char_u	*s;
    int		ret = 0;
    int		c;

    if (check_restricted() || check_secure())
	return;

    if (argvars[0].v_type != VAR_LIST)
    {
	EMSG2(_(e_listarg), "writefile()");
	return;
    }
    if (argvars[0].vval.v_list == NULL)
	return;

    if (argvars[2].v_type != VAR_UNKNOWN
			      && STRCMP(get_tv_string(&argvars[2]), "b") == 0)
	binary = TRUE;

    /* Always open the file in binary mode, library functions have a mind of
     * their own about CR-LF conversion. */
    fname = get_tv_string(&argvars[1]);
    if (*fname == NUL || (fd = mch_fopen((char *)fname, WRITEBIN)) == NULL)
    {
	EMSG2(_(e_notcreate), *fname == NUL ? (char_u *)_("<empty>") : fname);
	ret = -1;
    }
    else
    {
	for (li = argvars[0].vval.v_list->lv_first; li != NULL;
							     li = li->li_next)
	{
	    for (s = get_tv_string(&li->li_tv); *s != NUL; ++s)
	    {
		if (*s == '\n')
		    c = putc(NUL, fd);
		else
		    c = putc(*s, fd);
		if (c == EOF)
		{
		    ret = -1;
		    break;
		}
	    }
	    if (!binary || li->li_next != NULL)
		if (putc('\n', fd) == EOF)
		{
		    ret = -1;
		    break;
		}
	    if (ret < 0)
	    {
		EMSG(_(e_write));
		break;
	    }
	}
	fclose(fd);
    }

    rettv->vval.v_number = ret;
}

/*
 * Translate a String variable into a position.
 * Returns NULL when there is an error.
 */
    static pos_T *
var2fpos(varp, dollar_lnum, fnum)
    typval_T	*varp;
    int		dollar_lnum;	/* TRUE when $ is last line */
    int		*fnum;		/* set to fnum for '0, 'A, etc. */
{
    char_u		*name;
    static pos_T	pos;
    pos_T		*pp;

    /* Argument can be [lnum, col, coladd]. */
    if (varp->v_type == VAR_LIST)
    {
	list_T		*l;
	int		len;
	int		error = FALSE;
	listitem_T	*li;

	l = varp->vval.v_list;
	if (l == NULL)
	    return NULL;

	/* Get the line number */
	pos.lnum = list_find_nr(l, 0L, &error);
	if (error || pos.lnum <= 0 || pos.lnum > curbuf->b_ml.ml_line_count)
	    return NULL;	/* invalid line number */

	/* Get the column number */
	pos.col = list_find_nr(l, 1L, &error);
	if (error)
	    return NULL;
	len = (long)STRLEN(ml_get(pos.lnum));

	/* We accept "$" for the column number: last column. */
	li = list_find(l, 1L);
	if (li != NULL && li->li_tv.v_type == VAR_STRING
		&& li->li_tv.vval.v_string != NULL
		&& STRCMP(li->li_tv.vval.v_string, "$") == 0)
	    pos.col = len + 1;

	/* Accept a position up to the NUL after the line. */
	if (pos.col == 0 || (int)pos.col > len + 1)
	    return NULL;	/* invalid column number */
	--pos.col;

#ifdef FEAT_VIRTUALEDIT
	/* Get the virtual offset.  Defaults to zero. */
	pos.coladd = list_find_nr(l, 2L, &error);
	if (error)
	    pos.coladd = 0;
#endif

	return &pos;
    }

    name = get_tv_string_chk(varp);
    if (name == NULL)
	return NULL;
    if (name[0] == '.')				/* cursor */
	return &curwin->w_cursor;
#ifdef FEAT_VISUAL
    if (name[0] == 'v' && name[1] == NUL)	/* Visual start */
    {
	if (VIsual_active)
	    return &VIsual;
	return &curwin->w_cursor;
    }
#endif
    if (name[0] == '\'')			/* mark */
    {
	pp = getmark_fnum(name[1], FALSE, fnum);
	if (pp == NULL || pp == (pos_T *)-1 || pp->lnum <= 0)
	    return NULL;
	return pp;
    }

#ifdef FEAT_VIRTUALEDIT
    pos.coladd = 0;
#endif

    if (name[0] == 'w' && dollar_lnum)
    {
	pos.col = 0;
	if (name[1] == '0')		/* "w0": first visible line */
	{
	    update_topline();
	    pos.lnum = curwin->w_topline;
	    return &pos;
	}
	else if (name[1] == '$')	/* "w$": last visible line */
	{
	    validate_botline();
	    pos.lnum = curwin->w_botline - 1;
	    return &pos;
	}
    }
    else if (name[0] == '$')		/* last column or line */
    {
	if (dollar_lnum)
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
 * Convert list in "arg" into a position and optional file number.
 * When "fnump" is NULL there is no file number, only 3 items.
 * Note that the column is passed on as-is, the caller may want to decrement
 * it to use 1 for the first column.
 * Return FAIL when conversion is not possible, doesn't check the position for
 * validity.
 */
    static int
list2fpos(arg, posp, fnump)
    typval_T	*arg;
    pos_T	*posp;
    int		*fnump;
{
    list_T	*l = arg->vval.v_list;
    long	i = 0;
    long	n;

    /* List must be: [fnum, lnum, col, coladd], where "fnum" is only there
     * when "fnump" isn't NULL and "coladd" is optional. */
    if (arg->v_type != VAR_LIST
	    || l == NULL
	    || l->lv_len < (fnump == NULL ? 2 : 3)
	    || l->lv_len > (fnump == NULL ? 3 : 4))
	return FAIL;

    if (fnump != NULL)
    {
	n = list_find_nr(l, i++, NULL);	/* fnum */
	if (n < 0)
	    return FAIL;
	if (n == 0)
	    n = curbuf->b_fnum;		/* current buffer */
	*fnump = n;
    }

    n = list_find_nr(l, i++, NULL);	/* lnum */
    if (n < 0)
	return FAIL;
    posp->lnum = n;

    n = list_find_nr(l, i++, NULL);	/* col */
    if (n < 0)
	return FAIL;
    posp->col = n;

#ifdef FEAT_VIRTUALEDIT
    n = list_find_nr(l, i, NULL);
    if (n < 0)
	posp->coladd = 0;
    else
	posp->coladd = n;
#endif

    return OK;
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
 * Get the length of the name of a variable or function.
 * Only the name is recognized, does not handle ".key" or "[idx]".
 * "arg" is advanced to the first non-white character after the name.
 * Return -1 if curly braces expansion failed.
 * Return 0 if something else is wrong.
 * If the name contains 'magic' {}'s, expand them and return the
 * expanded name in an allocated string via 'alias' - caller must free.
 */
    static int
get_name_len(arg, alias, evaluate, verbose)
    char_u	**arg;
    char_u	**alias;
    int		evaluate;
    int		verbose;
{
    int		len;
    char_u	*p;
    char_u	*expr_start;
    char_u	*expr_end;

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

    /*
     * Find the end of the name; check for {} construction.
     */
    p = find_name_end(*arg, &expr_start, &expr_end,
					       len > 0 ? 0 : FNE_CHECK_START);
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
	    return -1;
	*alias = temp_string;
	*arg = skipwhite(p);
	return (int)STRLEN(temp_string);
    }

    len += get_id_len(arg);
    if (len == 0 && verbose)
	EMSG2(_(e_invexpr2), *arg);

    return len;
}

/*
 * Find the end of a variable or function name, taking care of magic braces.
 * If "expr_start" is not NULL then "expr_start" and "expr_end" are set to the
 * start and end of the first magic braces item.
 * "flags" can have FNE_INCL_BR and FNE_CHECK_START.
 * Return a pointer to just after the name.  Equal to "arg" if there is no
 * valid name.
 */
    static char_u *
find_name_end(arg, expr_start, expr_end, flags)
    char_u	*arg;
    char_u	**expr_start;
    char_u	**expr_end;
    int		flags;
{
    int		mb_nest = 0;
    int		br_nest = 0;
    char_u	*p;

    if (expr_start != NULL)
    {
	*expr_start = NULL;
	*expr_end = NULL;
    }

    /* Quick check for valid starting character. */
    if ((flags & FNE_CHECK_START) && !eval_isnamec1(*arg) && *arg != '{')
	return arg;

    for (p = arg; *p != NUL
		    && (eval_isnamec(*p)
			|| *p == '{'
			|| ((flags & FNE_INCL_BR) && (*p == '[' || *p == '.'))
			|| mb_nest != 0
			|| br_nest != 0); mb_ptr_adv(p))
    {
	if (*p == '\'')
	{
	    /* skip over 'string' to avoid counting [ and ] inside it. */
	    for (p = p + 1; *p != NUL && *p != '\''; mb_ptr_adv(p))
		;
	    if (*p == NUL)
		break;
	}
	else if (*p == '"')
	{
	    /* skip over "str\"ing" to avoid counting [ and ] inside it. */
	    for (p = p + 1; *p != NUL && *p != '"'; mb_ptr_adv(p))
		if (*p == '\\' && p[1] != NUL)
		    ++p;
	    if (*p == NUL)
		break;
	}

	if (mb_nest == 0)
	{
	    if (*p == '[')
		++br_nest;
	    else if (*p == ']')
		--br_nest;
	}

	if (br_nest == 0)
	{
	    if (*p == '{')
	    {
		mb_nest++;
		if (expr_start != NULL && *expr_start == NULL)
		    *expr_start = p;
	    }
	    else if (*p == '}')
	    {
		mb_nest--;
		if (expr_start != NULL && mb_nest == 0 && *expr_end == NULL)
		    *expr_end = p;
	    }
	}
    }

    return p;
}

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

    temp_result = eval_to_string(expr_start + 1, &nextcmd, FALSE);
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
	temp_result = find_name_end(retval, &expr_start, &expr_end, 0);
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

/*
 * Return TRUE if character "c" can be used in a variable or function name.
 * Does not include '{' or '}' for magic braces.
 */
    static int
eval_isnamec(c)
    int	    c;
{
    return (ASCII_ISALNUM(c) || c == '_' || c == ':' || c == AUTOLOAD_CHAR);
}

/*
 * Return TRUE if character "c" can be used as the first character in a
 * variable or function name (excluding '{' and '}').
 */
    static int
eval_isnamec1(c)
    int	    c;
{
    return (ASCII_ISALPHA(c) || c == '_');
}

/*
 * Set number v: variable to "val".
 */
    void
set_vim_var_nr(idx, val)
    int		idx;
    long	val;
{
    vimvars[idx].vv_nr = val;
}

/*
 * Get number v: variable value.
 */
    long
get_vim_var_nr(idx)
    int		idx;
{
    return vimvars[idx].vv_nr;
}

/*
 * Get string v: variable value.  Uses a static buffer, can only be used once.
 */
    char_u *
get_vim_var_str(idx)
    int		idx;
{
    return get_tv_string(&vimvars[idx].vv_tv);
}

/*
 * Get List v: variable value.  Caller must take care of reference count when
 * needed.
 */
    list_T *
get_vim_var_list(idx)
    int		idx;
{
    return vimvars[idx].vv_list;
}

/*
 * Set v:count to "count" and v:count1 to "count1".
 * When "set_prevcount" is TRUE first set v:prevcount from v:count.
 */
    void
set_vcount(count, count1, set_prevcount)
    long	count;
    long	count1;
    int		set_prevcount;
{
    if (set_prevcount)
	vimvars[VV_PREVCOUNT].vv_nr = vimvars[VV_COUNT].vv_nr;
    vimvars[VV_COUNT].vv_nr = count;
    vimvars[VV_COUNT1].vv_nr = count1;
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
    /* Need to do this (at least) once, since we can't initialize a union.
     * Will always be invoked when "v:progname" is set. */
    vimvars[VV_VERSION].vv_nr = VIM_VERSION_100;

    vim_free(vimvars[idx].vv_str);
    if (val == NULL)
	vimvars[idx].vv_str = NULL;
    else if (len == -1)
	vimvars[idx].vv_str = vim_strsave(val);
    else
	vimvars[idx].vv_str = vim_strnsave(val, len);
}

/*
 * Set List v: variable to "val".
 */
    void
set_vim_var_list(idx, val)
    int		idx;
    list_T	*val;
{
    list_unref(vimvars[idx].vv_list);
    vimvars[idx].vv_list = val;
    if (val != NULL)
	++val->lv_refcount;
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
    if (vimvars[VV_REG].vv_str == NULL || vimvars[VV_REG].vv_str[0] != c)
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
	return vimvars[VV_EXCEPTION].vv_str;

    vimvars[VV_EXCEPTION].vv_str = oldval;
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
	return vimvars[VV_THROWPOINT].vv_str;

    vimvars[VV_THROWPOINT].vv_str = oldval;
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

    oldval = vimvars[VV_CMDARG].vv_str;
    if (eap == NULL)
    {
	vim_free(oldval);
	vimvars[VV_CMDARG].vv_str = oldarg;
	return NULL;
    }

    if (eap->force_bin == FORCE_BIN)
	len = 6;
    else if (eap->force_bin == FORCE_NOBIN)
	len = 8;
    else
	len = 0;

    if (eap->read_edit)
	len += 7;

    if (eap->force_ff != 0)
	len += (unsigned)STRLEN(eap->cmd + eap->force_ff) + 6;
# ifdef FEAT_MBYTE
    if (eap->force_enc != 0)
	len += (unsigned)STRLEN(eap->cmd + eap->force_enc) + 7;
    if (eap->bad_char != 0)
	len += (unsigned)STRLEN(eap->cmd + eap->bad_char) + 7;
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

    if (eap->read_edit)
	STRCAT(newval, " ++edit");

    if (eap->force_ff != 0)
	sprintf((char *)newval + STRLEN(newval), " ++ff=%s",
						eap->cmd + eap->force_ff);
# ifdef FEAT_MBYTE
    if (eap->force_enc != 0)
	sprintf((char *)newval + STRLEN(newval), " ++enc=%s",
					       eap->cmd + eap->force_enc);
    if (eap->bad_char != 0)
	sprintf((char *)newval + STRLEN(newval), " ++bad=%s",
					       eap->cmd + eap->bad_char);
# endif
    vimvars[VV_CMDARG].vv_str = newval;
    return oldval;
}
#endif

/*
 * Get the value of internal variable "name".
 * Return OK or FAIL.
 */
    static int
get_var_tv(name, len, rettv, verbose)
    char_u	*name;
    int		len;		/* length of "name" */
    typval_T	*rettv;		/* NULL when only checking existence */
    int		verbose;	/* may give error message */
{
    int		ret = OK;
    typval_T	*tv = NULL;
    typval_T	atv;
    dictitem_T	*v;
    int		cc;

    /* truncate the name, so that we can use strcmp() */
    cc = name[len];
    name[len] = NUL;

    /*
     * Check for "b:changedtick".
     */
    if (STRCMP(name, "b:changedtick") == 0)
    {
	atv.v_type = VAR_NUMBER;
	atv.vval.v_number = curbuf->b_changedtick;
	tv = &atv;
    }

    /*
     * Check for user-defined variables.
     */
    else
    {
	v = find_var(name, NULL);
	if (v != NULL)
	    tv = &v->di_tv;
    }

    if (tv == NULL)
    {
	if (rettv != NULL && verbose)
	    EMSG2(_(e_undefvar), name);
	ret = FAIL;
    }
    else if (rettv != NULL)
	copy_tv(tv, rettv);

    name[len] = cc;

    return ret;
}

/*
 * Handle expr[expr], expr[expr:expr] subscript and .name lookup.
 * Also handle function call with Funcref variable: func(expr)
 * Can all be combined: dict.func(expr)[idx]['func'](expr)
 */
    static int
handle_subscript(arg, rettv, evaluate, verbose)
    char_u	**arg;
    typval_T	*rettv;
    int		evaluate;	/* do more than finding the end */
    int		verbose;	/* give error messages */
{
    int		ret = OK;
    dict_T	*selfdict = NULL;
    char_u	*s;
    int		len;
    typval_T	functv;

    while (ret == OK
	    && (**arg == '['
		|| (**arg == '.' && rettv->v_type == VAR_DICT)
		|| (**arg == '(' && rettv->v_type == VAR_FUNC))
	    && !vim_iswhite(*(*arg - 1)))
    {
	if (**arg == '(')
	{
	    /* need to copy the funcref so that we can clear rettv */
	    functv = *rettv;
	    rettv->v_type = VAR_UNKNOWN;

	    /* Invoke the function.  Recursive! */
	    s = functv.vval.v_string;
	    ret = get_func_tv(s, (int)STRLEN(s), rettv, arg,
			curwin->w_cursor.lnum, curwin->w_cursor.lnum,
			&len, evaluate, selfdict);

	    /* Clear the funcref afterwards, so that deleting it while
	     * evaluating the arguments is possible (see test55). */
	    clear_tv(&functv);

	    /* Stop the expression evaluation when immediately aborting on
	     * error, or when an interrupt occurred or an exception was thrown
	     * but not caught. */
	    if (aborting())
	    {
		if (ret == OK)
		    clear_tv(rettv);
		ret = FAIL;
	    }
	    dict_unref(selfdict);
	    selfdict = NULL;
	}
	else /* **arg == '[' || **arg == '.' */
	{
	    dict_unref(selfdict);
	    if (rettv->v_type == VAR_DICT)
	    {
		selfdict = rettv->vval.v_dict;
		if (selfdict != NULL)
		    ++selfdict->dv_refcount;
	    }
	    else
		selfdict = NULL;
	    if (eval_index(arg, rettv, evaluate, verbose) == FAIL)
	    {
		clear_tv(rettv);
		ret = FAIL;
	    }
	}
    }
    dict_unref(selfdict);
    return ret;
}

/*
 * Allocate memory for a variable type-value, and make it empty (0 or NULL
 * value).
 */
    static typval_T *
alloc_tv()
{
    return (typval_T *)alloc_clear((unsigned)sizeof(typval_T));
}

/*
 * Allocate memory for a variable type-value, and assign a string to it.
 * The string "s" must have been allocated, it is consumed.
 * Return NULL for out of memory, the variable otherwise.
 */
    static typval_T *
alloc_string_tv(s)
    char_u	*s;
{
    typval_T	*rettv;

    rettv = alloc_tv();
    if (rettv != NULL)
    {
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = s;
    }
    else
	vim_free(s);
    return rettv;
}

/*
 * Free the memory for a variable type-value.
 */
    void
free_tv(varp)
    typval_T *varp;
{
    if (varp != NULL)
    {
	switch (varp->v_type)
	{
	    case VAR_FUNC:
		func_unref(varp->vval.v_string);
		/*FALLTHROUGH*/
	    case VAR_STRING:
		vim_free(varp->vval.v_string);
		break;
	    case VAR_LIST:
		list_unref(varp->vval.v_list);
		break;
	    case VAR_DICT:
		dict_unref(varp->vval.v_dict);
		break;
	    case VAR_NUMBER:
#ifdef FEAT_FLOAT
	    case VAR_FLOAT:
#endif
	    case VAR_UNKNOWN:
		break;
	    default:
		EMSG2(_(e_intern2), "free_tv()");
		break;
	}
	vim_free(varp);
    }
}

/*
 * Free the memory for a variable value and set the value to NULL or 0.
 */
    void
clear_tv(varp)
    typval_T *varp;
{
    if (varp != NULL)
    {
	switch (varp->v_type)
	{
	    case VAR_FUNC:
		func_unref(varp->vval.v_string);
		/*FALLTHROUGH*/
	    case VAR_STRING:
		vim_free(varp->vval.v_string);
		varp->vval.v_string = NULL;
		break;
	    case VAR_LIST:
		list_unref(varp->vval.v_list);
		varp->vval.v_list = NULL;
		break;
	    case VAR_DICT:
		dict_unref(varp->vval.v_dict);
		varp->vval.v_dict = NULL;
		break;
	    case VAR_NUMBER:
		varp->vval.v_number = 0;
		break;
#ifdef FEAT_FLOAT
	    case VAR_FLOAT:
		varp->vval.v_float = 0.0;
		break;
#endif
	    case VAR_UNKNOWN:
		break;
	    default:
		EMSG2(_(e_intern2), "clear_tv()");
	}
	varp->v_lock = 0;
    }
}

/*
 * Set the value of a variable to NULL without freeing items.
 */
    static void
init_tv(varp)
    typval_T *varp;
{
    if (varp != NULL)
	vim_memset(varp, 0, sizeof(typval_T));
}

/*
 * Get the number value of a variable.
 * If it is a String variable, uses vim_str2nr().
 * For incompatible types, return 0.
 * get_tv_number_chk() is similar to get_tv_number(), but informs the
 * caller of incompatible types: it sets *denote to TRUE if "denote"
 * is not NULL or returns -1 otherwise.
 */
    static long
get_tv_number(varp)
    typval_T	*varp;
{
    int		error = FALSE;

    return get_tv_number_chk(varp, &error);	/* return 0L on error */
}

    long
get_tv_number_chk(varp, denote)
    typval_T	*varp;
    int		*denote;
{
    long	n = 0L;

    switch (varp->v_type)
    {
	case VAR_NUMBER:
	    return (long)(varp->vval.v_number);
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    EMSG(_("E805: Using a Float as a Number"));
	    break;
#endif
	case VAR_FUNC:
	    EMSG(_("E703: Using a Funcref as a Number"));
	    break;
	case VAR_STRING:
	    if (varp->vval.v_string != NULL)
		vim_str2nr(varp->vval.v_string, NULL, NULL,
							TRUE, TRUE, &n, NULL);
	    return n;
	case VAR_LIST:
	    EMSG(_("E745: Using a List as a Number"));
	    break;
	case VAR_DICT:
	    EMSG(_("E728: Using a Dictionary as a Number"));
	    break;
	default:
	    EMSG2(_(e_intern2), "get_tv_number()");
	    break;
    }
    if (denote == NULL)		/* useful for values that must be unsigned */
	n = -1;
    else
	*denote = TRUE;
    return n;
}

/*
 * Get the lnum from the first argument.
 * Also accepts ".", "$", etc., but that only works for the current buffer.
 * Returns -1 on error.
 */
    static linenr_T
get_tv_lnum(argvars)
    typval_T	*argvars;
{
    typval_T	rettv;
    linenr_T	lnum;

    lnum = get_tv_number_chk(&argvars[0], NULL);
    if (lnum == 0)  /* no valid number, try using line() */
    {
	rettv.v_type = VAR_NUMBER;
	f_line(argvars, &rettv);
	lnum = rettv.vval.v_number;
	clear_tv(&rettv);
    }
    return lnum;
}

/*
 * Get the lnum from the first argument.
 * Also accepts "$", then "buf" is used.
 * Returns 0 on error.
 */
    static linenr_T
get_tv_lnum_buf(argvars, buf)
    typval_T	*argvars;
    buf_T	*buf;
{
    if (argvars[0].v_type == VAR_STRING
	    && argvars[0].vval.v_string != NULL
	    && argvars[0].vval.v_string[0] == '$'
	    && buf != NULL)
	return buf->b_ml.ml_line_count;
    return get_tv_number_chk(&argvars[0], NULL);
}

/*
 * Get the string value of a variable.
 * If it is a Number variable, the number is converted into a string.
 * get_tv_string() uses a single, static buffer.  YOU CAN ONLY USE IT ONCE!
 * get_tv_string_buf() uses a given buffer.
 * If the String variable has never been set, return an empty string.
 * Never returns NULL;
 * get_tv_string_chk() and get_tv_string_buf_chk() are similar, but return
 * NULL on error.
 */
    static char_u *
get_tv_string(varp)
    typval_T	*varp;
{
    static char_u   mybuf[NUMBUFLEN];

    return get_tv_string_buf(varp, mybuf);
}

    static char_u *
get_tv_string_buf(varp, buf)
    typval_T	*varp;
    char_u	*buf;
{
    char_u	*res =  get_tv_string_buf_chk(varp, buf);

    return res != NULL ? res : (char_u *)"";
}

    char_u *
get_tv_string_chk(varp)
    typval_T	*varp;
{
    static char_u   mybuf[NUMBUFLEN];

    return get_tv_string_buf_chk(varp, mybuf);
}

    static char_u *
get_tv_string_buf_chk(varp, buf)
    typval_T	*varp;
    char_u	*buf;
{
    switch (varp->v_type)
    {
	case VAR_NUMBER:
	    sprintf((char *)buf, "%ld", (long)varp->vval.v_number);
	    return buf;
	case VAR_FUNC:
	    EMSG(_("E729: using Funcref as a String"));
	    break;
	case VAR_LIST:
	    EMSG(_("E730: using List as a String"));
	    break;
	case VAR_DICT:
	    EMSG(_("E731: using Dictionary as a String"));
	    break;
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    EMSG(_("E806: using Float as a String"));
	    break;
#endif
	case VAR_STRING:
	    if (varp->vval.v_string != NULL)
		return varp->vval.v_string;
	    return (char_u *)"";
	default:
	    EMSG2(_(e_intern2), "get_tv_string_buf()");
	    break;
    }
    return NULL;
}

/*
 * Find variable "name" in the list of variables.
 * Return a pointer to it if found, NULL if not found.
 * Careful: "a:0" variables don't have a name.
 * When "htp" is not NULL we are writing to the variable, set "htp" to the
 * hashtab_T used.
 */
    static dictitem_T *
find_var(name, htp)
    char_u	*name;
    hashtab_T	**htp;
{
    char_u	*varname;
    hashtab_T	*ht;

    ht = find_var_ht(name, &varname);
    if (htp != NULL)
	*htp = ht;
    if (ht == NULL)
	return NULL;
    return find_var_in_ht(ht, varname, htp != NULL);
}

/*
 * Find variable "varname" in hashtab "ht".
 * Returns NULL if not found.
 */
    static dictitem_T *
find_var_in_ht(ht, varname, writing)
    hashtab_T	*ht;
    char_u	*varname;
    int		writing;
{
    hashitem_T	*hi;

    if (*varname == NUL)
    {
	/* Must be something like "s:", otherwise "ht" would be NULL. */
	switch (varname[-2])
	{
	    case 's': return &SCRIPT_SV(current_SID).sv_var;
	    case 'g': return &globvars_var;
	    case 'v': return &vimvars_var;
	    case 'b': return &curbuf->b_bufvar;
	    case 'w': return &curwin->w_winvar;
#ifdef FEAT_WINDOWS
	    case 't': return &curtab->tp_winvar;
#endif
	    case 'l': return current_funccal == NULL
					? NULL : &current_funccal->l_vars_var;
	    case 'a': return current_funccal == NULL
				       ? NULL : &current_funccal->l_avars_var;
	}
	return NULL;
    }

    hi = hash_find(ht, varname);
    if (HASHITEM_EMPTY(hi))
    {
	/* For global variables we may try auto-loading the script.  If it
	 * worked find the variable again.  Don't auto-load a script if it was
	 * loaded already, otherwise it would be loaded every time when
	 * checking if a function name is a Funcref variable. */
	if (ht == &globvarht && !writing
			    && script_autoload(varname, FALSE) && !aborting())
	    hi = hash_find(ht, varname);
	if (HASHITEM_EMPTY(hi))
	    return NULL;
    }
    return HI2DI(hi);
}

/*
 * Find the hashtab used for a variable name.
 * Set "varname" to the start of name without ':'.
 */
    static hashtab_T *
find_var_ht(name, varname)
    char_u  *name;
    char_u  **varname;
{
    hashitem_T	*hi;

    if (name[1] != ':')
    {
	/* The name must not start with a colon or #. */
	if (name[0] == ':' || name[0] == AUTOLOAD_CHAR)
	    return NULL;
	*varname = name;

	/* "version" is "v:version" in all scopes */
	hi = hash_find(&compat_hashtab, name);
	if (!HASHITEM_EMPTY(hi))
	    return &compat_hashtab;

	if (current_funccal == NULL)
	    return &globvarht;			/* global variable */
	return &current_funccal->l_vars.dv_hashtab; /* l: variable */
    }
    *varname = name + 2;
    if (*name == 'g')				/* global variable */
	return &globvarht;
    /* There must be no ':' or '#' in the rest of the name, unless g: is used
     */
    if (vim_strchr(name + 2, ':') != NULL
			       || vim_strchr(name + 2, AUTOLOAD_CHAR) != NULL)
	return NULL;
    if (*name == 'b')				/* buffer variable */
	return &curbuf->b_vars.dv_hashtab;
    if (*name == 'w')				/* window variable */
	return &curwin->w_vars.dv_hashtab;
#ifdef FEAT_WINDOWS
    if (*name == 't')				/* tab page variable */
	return &curtab->tp_vars.dv_hashtab;
#endif
    if (*name == 'v')				/* v: variable */
	return &vimvarht;
    if (*name == 'a' && current_funccal != NULL) /* function argument */
	return &current_funccal->l_avars.dv_hashtab;
    if (*name == 'l' && current_funccal != NULL) /* local function variable */
	return &current_funccal->l_vars.dv_hashtab;
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
    dictitem_T	*v;

    v = find_var(name, NULL);
    if (v == NULL)
	return NULL;
    return get_tv_string(&v->di_tv);
}

/*
 * Allocate a new hashtab for a sourced script.  It will be used while
 * sourcing this script and when executing functions defined in the script.
 */
    void
new_script_vars(id)
    scid_T id;
{
    int		i;
    hashtab_T	*ht;
    scriptvar_T *sv;

    if (ga_grow(&ga_scripts, (int)(id - ga_scripts.ga_len)) == OK)
    {
	/* Re-allocating ga_data means that an ht_array pointing to
	 * ht_smallarray becomes invalid.  We can recognize this: ht_mask is
	 * at its init value.  Also reset "v_dict", it's always the same. */
	for (i = 1; i <= ga_scripts.ga_len; ++i)
	{
	    ht = &SCRIPT_VARS(i);
	    if (ht->ht_mask == HT_INIT_SIZE - 1)
		ht->ht_array = ht->ht_smallarray;
	    sv = &SCRIPT_SV(i);
	    sv->sv_var.di_tv.vval.v_dict = &sv->sv_dict;
	}

	while (ga_scripts.ga_len < id)
	{
	    sv = &SCRIPT_SV(ga_scripts.ga_len + 1);
	    init_var_dict(&sv->sv_dict, &sv->sv_var);
	    ++ga_scripts.ga_len;
	}
    }
}

/*
 * Initialize dictionary "dict" as a scope and set variable "dict_var" to
 * point to it.
 */
    void
init_var_dict(dict, dict_var)
    dict_T	*dict;
    dictitem_T	*dict_var;
{
    hash_init(&dict->dv_hashtab);
    dict->dv_refcount = DO_NOT_FREE_CNT;
    dict_var->di_tv.vval.v_dict = dict;
    dict_var->di_tv.v_type = VAR_DICT;
    dict_var->di_tv.v_lock = VAR_FIXED;
    dict_var->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    dict_var->di_key[0] = NUL;
}

/*
 * Clean up a list of internal variables.
 * Frees all allocated variables and the value they contain.
 * Clears hashtab "ht", does not free it.
 */
    void
vars_clear(ht)
    hashtab_T *ht;
{
    vars_clear_ext(ht, TRUE);
}

/*
 * Like vars_clear(), but only free the value if "free_val" is TRUE.
 */
    static void
vars_clear_ext(ht, free_val)
    hashtab_T	*ht;
    int		free_val;
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

	    /* Free the variable.  Don't remove it from the hashtab,
	     * ht_array might change then.  hash_clear() takes care of it
	     * later. */
	    v = HI2DI(hi);
	    if (free_val)
		clear_tv(&v->di_tv);
	    if ((v->di_flags & DI_FLAGS_FIX) == 0)
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
    static void
delete_var(ht, hi)
    hashtab_T	*ht;
    hashitem_T	*hi;
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
list_one_var(v, prefix, first)
    dictitem_T	*v;
    char_u	*prefix;
    int		*first;
{
    char_u	*tofree;
    char_u	*s;
    char_u	numbuf[NUMBUFLEN];

    s = echo_string(&v->di_tv, &tofree, numbuf, ++current_copyID);
    list_one_var_a(prefix, v->di_key, v->di_tv.v_type,
					 s == NULL ? (char_u *)"" : s, first);
    vim_free(tofree);
}

    static void
list_one_var_a(prefix, name, type, string, first)
    char_u	*prefix;
    char_u	*name;
    int		type;
    char_u	*string;
    int		*first;  /* when TRUE clear rest of screen and set to FALSE */
{
    /* don't use msg() or msg_attr() to avoid overwriting "v:statusmsg" */
    msg_start();
    msg_puts(prefix);
    if (name != NULL)	/* "a:" vars don't have a name stored */
	msg_puts(name);
    msg_putchar(' ');
    msg_advance(22);
    if (type == VAR_NUMBER)
	msg_putchar('#');
    else if (type == VAR_FUNC)
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

    if (type == VAR_FUNC)
	msg_puts((char_u *)"()");
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
    static void
set_var(name, tv, copy)
    char_u	*name;
    typval_T	*tv;
    int		copy;	    /* make copy of value in "tv" */
{
    dictitem_T	*v;
    char_u	*varname;
    hashtab_T	*ht;
    char_u	*p;

    if (tv->v_type == VAR_FUNC)
    {
	if (!(vim_strchr((char_u *)"wbs", name[0]) != NULL && name[1] == ':')
		&& !ASCII_ISUPPER((name[0] != NUL && name[1] == ':')
							 ? name[2] : name[0]))
	{
	    EMSG2(_("E704: Funcref variable name must start with a capital: %s"), name);
	    return;
	}
	if (function_exists(name))
	{
	    EMSG2(_("E705: Variable name conflicts with existing function: %s"),
									name);
	    return;
	}
    }

    ht = find_var_ht(name, &varname);
    if (ht == NULL || *varname == NUL)
    {
	EMSG2(_(e_illvar), name);
	return;
    }

    v = find_var_in_ht(ht, varname, TRUE);
    if (v != NULL)
    {
	/* existing variable, need to clear the value */
	if (var_check_ro(v->di_flags, name)
				      || tv_check_lock(v->di_tv.v_lock, name))
	    return;
	if (v->di_tv.v_type != tv->v_type
		&& !((v->di_tv.v_type == VAR_STRING
			|| v->di_tv.v_type == VAR_NUMBER)
		    && (tv->v_type == VAR_STRING
			|| tv->v_type == VAR_NUMBER))
#ifdef FEAT_FLOAT
		&& !((v->di_tv.v_type == VAR_NUMBER
			|| v->di_tv.v_type == VAR_FLOAT)
		    && (tv->v_type == VAR_NUMBER
			|| tv->v_type == VAR_FLOAT))
#endif
		)
	{
	    EMSG2(_("E706: Variable type mismatch for: %s"), name);
	    return;
	}

	/*
	 * Handle setting internal v: variables separately: we don't change
	 * the type.
	 */
	if (ht == &vimvarht)
	{
	    if (v->di_tv.v_type == VAR_STRING)
	    {
		vim_free(v->di_tv.vval.v_string);
		if (copy || tv->v_type != VAR_STRING)
		    v->di_tv.vval.v_string = vim_strsave(get_tv_string(tv));
		else
		{
		    /* Take over the string to avoid an extra alloc/free. */
		    v->di_tv.vval.v_string = tv->vval.v_string;
		    tv->vval.v_string = NULL;
		}
	    }
	    else if (v->di_tv.v_type != VAR_NUMBER)
		EMSG2(_(e_intern2), "set_var()");
	    else
	    {
		v->di_tv.vval.v_number = get_tv_number(tv);
		if (STRCMP(varname, "searchforward") == 0)
		    set_search_direction(v->di_tv.vval.v_number ? '/' : '?');
	    }
	    return;
	}

	clear_tv(&v->di_tv);
    }
    else		    /* add a new variable */
    {
	/* Can't add "v:" variable. */
	if (ht == &vimvarht)
	{
	    EMSG2(_(e_illvar), name);
	    return;
	}

	/* Make sure the variable name is valid. */
	for (p = varname; *p != NUL; ++p)
	    if (!eval_isnamec1(*p) && (p == varname || !VIM_ISDIGIT(*p))
						       && *p != AUTOLOAD_CHAR)
	    {
		EMSG2(_(e_illvar), varname);
		return;
	    }

	v = (dictitem_T *)alloc((unsigned)(sizeof(dictitem_T)
							  + STRLEN(varname)));
	if (v == NULL)
	    return;
	STRCPY(v->di_key, varname);
	if (hash_add(ht, DI2HIKEY(v)) == FAIL)
	{
	    vim_free(v);
	    return;
	}
	v->di_flags = 0;
    }

    if (copy || tv->v_type == VAR_NUMBER || tv->v_type == VAR_FLOAT)
	copy_tv(tv, &v->di_tv);
    else
    {
	v->di_tv = *tv;
	v->di_tv.v_lock = 0;
	init_tv(tv);
    }
}

/*
 * Return TRUE if di_flags "flags" indicates variable "name" is read-only.
 * Also give an error message.
 */
    static int
var_check_ro(flags, name)
    int		flags;
    char_u	*name;
{
    if (flags & DI_FLAGS_RO)
    {
	EMSG2(_(e_readonlyvar), name);
	return TRUE;
    }
    if ((flags & DI_FLAGS_RO_SBX) && sandbox)
    {
	EMSG2(_(e_readonlysbx), name);
	return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if di_flags "flags" indicates variable "name" is fixed.
 * Also give an error message.
 */
    static int
var_check_fixed(flags, name)
    int		flags;
    char_u	*name;
{
    if (flags & DI_FLAGS_FIX)
    {
	EMSG2(_("E795: Cannot delete variable %s"), name);
	return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if typeval "tv" is set to be locked (immutable).
 * Also give an error message, using "name".
 */
    static int
tv_check_lock(lock, name)
    int		lock;
    char_u	*name;
{
    if (lock & VAR_LOCKED)
    {
	EMSG2(_("E741: Value is locked: %s"),
				name == NULL ? (char_u *)_("Unknown") : name);
	return TRUE;
    }
    if (lock & VAR_FIXED)
    {
	EMSG2(_("E742: Cannot change value of %s"),
				name == NULL ? (char_u *)_("Unknown") : name);
	return TRUE;
    }
    return FALSE;
}

/*
 * Copy the values from typval_T "from" to typval_T "to".
 * When needed allocates string or increases reference count.
 * Does not make a copy of a list or dict but copies the reference!
 * It is OK for "from" and "to" to point to the same item.  This is used to
 * make a copy later.
 */
    static void
copy_tv(from, to)
    typval_T *from;
    typval_T *to;
{
    to->v_type = from->v_type;
    to->v_lock = 0;
    switch (from->v_type)
    {
	case VAR_NUMBER:
	    to->vval.v_number = from->vval.v_number;
	    break;
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    to->vval.v_float = from->vval.v_float;
	    break;
#endif
	case VAR_STRING:
	case VAR_FUNC:
	    if (from->vval.v_string == NULL)
		to->vval.v_string = NULL;
	    else
	    {
		to->vval.v_string = vim_strsave(from->vval.v_string);
		if (from->v_type == VAR_FUNC)
		    func_ref(to->vval.v_string);
	    }
	    break;
	case VAR_LIST:
	    if (from->vval.v_list == NULL)
		to->vval.v_list = NULL;
	    else
	    {
		to->vval.v_list = from->vval.v_list;
		++to->vval.v_list->lv_refcount;
	    }
	    break;
	case VAR_DICT:
	    if (from->vval.v_dict == NULL)
		to->vval.v_dict = NULL;
	    else
	    {
		to->vval.v_dict = from->vval.v_dict;
		++to->vval.v_dict->dv_refcount;
	    }
	    break;
	default:
	    EMSG2(_(e_intern2), "copy_tv()");
	    break;
    }
}

/*
 * Make a copy of an item.
 * Lists and Dictionaries are also copied.  A deep copy if "deep" is set.
 * For deepcopy() "copyID" is zero for a full copy or the ID for when a
 * reference to an already copied list/dict can be used.
 * Returns FAIL or OK.
 */
    static int
item_copy(from, to, deep, copyID)
    typval_T	*from;
    typval_T	*to;
    int		deep;
    int		copyID;
{
    static int	recurse = 0;
    int		ret = OK;

    if (recurse >= DICT_MAXNEST)
    {
	EMSG(_("E698: variable nested too deep for making a copy"));
	return FAIL;
    }
    ++recurse;

    switch (from->v_type)
    {
	case VAR_NUMBER:
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
#endif
	case VAR_STRING:
	case VAR_FUNC:
	    copy_tv(from, to);
	    break;
	case VAR_LIST:
	    to->v_type = VAR_LIST;
	    to->v_lock = 0;
	    if (from->vval.v_list == NULL)
		to->vval.v_list = NULL;
	    else if (copyID != 0 && from->vval.v_list->lv_copyID == copyID)
	    {
		/* use the copy made earlier */
		to->vval.v_list = from->vval.v_list->lv_copylist;
		++to->vval.v_list->lv_refcount;
	    }
	    else
		to->vval.v_list = list_copy(from->vval.v_list, deep, copyID);
	    if (to->vval.v_list == NULL)
		ret = FAIL;
	    break;
	case VAR_DICT:
	    to->v_type = VAR_DICT;
	    to->v_lock = 0;
	    if (from->vval.v_dict == NULL)
		to->vval.v_dict = NULL;
	    else if (copyID != 0 && from->vval.v_dict->dv_copyID == copyID)
	    {
		/* use the copy made earlier */
		to->vval.v_dict = from->vval.v_dict->dv_copydict;
		++to->vval.v_dict->dv_refcount;
	    }
	    else
		to->vval.v_dict = dict_copy(from->vval.v_dict, deep, copyID);
	    if (to->vval.v_dict == NULL)
		ret = FAIL;
	    break;
	default:
	    EMSG2(_(e_intern2), "item_copy()");
	    ret = FAIL;
    }
    --recurse;
    return ret;
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
    typval_T	rettv;
    char_u	*tofree;
    char_u	*p;
    int		needclr = TRUE;
    int		atstart = TRUE;
    char_u	numbuf[NUMBUFLEN];

    if (eap->skip)
	++emsg_skip;
    while (*arg != NUL && *arg != '|' && *arg != '\n' && !got_int)
    {
	/* If eval1() causes an error message the text from the command may
	 * still need to be cleared. E.g., "echo 22,44". */
	need_clr_eos = needclr;

	p = arg;
	if (eval1(&arg, &rettv, !eap->skip) == FAIL)
	{
	    /*
	     * Report the invalid expression unless the expression evaluation
	     * has been cancelled due to an aborting error, an interrupt, or an
	     * exception.
	     */
	    if (!aborting())
		EMSG2(_(e_invexpr2), p);
	    need_clr_eos = FALSE;
	    break;
	}
	need_clr_eos = FALSE;

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
	    p = echo_string(&rettv, &tofree, numbuf, ++current_copyID);
	    if (p != NULL)
		for ( ; *p != NUL && !got_int; ++p)
		{
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
			    int i = (*mb_ptr2len)(p);

			    (void)msg_outtrans_len_attr(p, i, echo_attr);
			    p += i - 1;
			}
			else
#endif
			    (void)msg_outtrans_len_attr(p, 1, echo_attr);
		    }
		}
	    vim_free(tofree);
	}
	clear_tv(&rettv);
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
    typval_T	rettv;
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
	if (eval1(&arg, &rettv, !eap->skip) == FAIL)
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
	    p = get_tv_string(&rettv);
	    len = (int)STRLEN(p);
	    if (ga_grow(&ga, len + 2) == FAIL)
	    {
		clear_tv(&rettv);
		ret = FAIL;
		break;
	    }
	    if (ga.ga_len)
		((char_u *)(ga.ga_data))[ga.ga_len++] = ' ';
	    STRCPY((char_u *)(ga.ga_data) + ga.ga_len, p);
	    ga.ga_len += len;
	}

	clear_tv(&rettv);
	arg = skipwhite(arg);
    }

    if (ret != FAIL && ga.ga_data != NULL)
    {
	if (eap->cmdidx == CMD_echomsg)
	{
	    MSG_ATTR(ga.ga_data, echo_attr);
	    out_flush();
	}
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
    int		saved_did_emsg;
    char_u	*name = NULL;
    char_u	*p;
    char_u	*arg;
    char_u	*line_arg = NULL;
    garray_T	newargs;
    garray_T	newlines;
    int		varargs = FALSE;
    int		mustend = FALSE;
    int		flags = 0;
    ufunc_T	*fp;
    int		indent;
    int		nesting;
    char_u	*skip_until = NULL;
    dictitem_T	*v;
    funcdict_T	fudi;
    static int	func_nr = 0;	    /* number for nameless function */
    int		paren;
    hashtab_T	*ht;
    int		todo;
    hashitem_T	*hi;
    int		sourcing_lnum_off;

    /*
     * ":function" without argument: list functions.
     */
    if (ends_excmd(*eap->arg))
    {
	if (!eap->skip)
	{
	    todo = (int)func_hashtab.ht_used;
	    for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;
		    fp = HI2UF(hi);
		    if (!isdigit(*fp->uf_name))
			list_func_head(fp, FALSE);
		}
	    }
	}
	eap->nextcmd = check_nextcmd(eap->arg);
	return;
    }

    /*
     * ":function /pat": list functions matching pattern.
     */
    if (*eap->arg == '/')
    {
	p = skip_regexp(eap->arg + 1, '/', TRUE, NULL);
	if (!eap->skip)
	{
	    regmatch_T	regmatch;

	    c = *p;
	    *p = NUL;
	    regmatch.regprog = vim_regcomp(eap->arg + 1, RE_MAGIC);
	    *p = c;
	    if (regmatch.regprog != NULL)
	    {
		regmatch.rm_ic = p_ic;

		todo = (int)func_hashtab.ht_used;
		for (hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
		{
		    if (!HASHITEM_EMPTY(hi))
		    {
			--todo;
			fp = HI2UF(hi);
			if (!isdigit(*fp->uf_name)
				    && vim_regexec(&regmatch, fp->uf_name, 0))
			    list_func_head(fp, FALSE);
		    }
		}
	    }
	}
	if (*p == '/')
	    ++p;
	eap->nextcmd = check_nextcmd(p);
	return;
    }

    /*
     * Get the function name.  There are these situations:
     * func	    normal function name
     *		    "name" == func, "fudi.fd_dict" == NULL
     * dict.func    new dictionary entry
     *		    "name" == NULL, "fudi.fd_dict" set,
     *		    "fudi.fd_di" == NULL, "fudi.fd_newkey" == func
     * dict.func    existing dict entry with a Funcref
     *		    "name" == func, "fudi.fd_dict" set,
     *		    "fudi.fd_di" set, "fudi.fd_newkey" == NULL
     * dict.func    existing dict entry that's not a Funcref
     *		    "name" == NULL, "fudi.fd_dict" set,
     *		    "fudi.fd_di" set, "fudi.fd_newkey" == NULL
     */
    p = eap->arg;
    name = trans_function_name(&p, eap->skip, 0, &fudi);
    paren = (vim_strchr(p, '(') != NULL);
    if (name == NULL && (fudi.fd_dict == NULL || !paren) && !eap->skip)
    {
	/*
	 * Return on an invalid expression in braces, unless the expression
	 * evaluation has been cancelled due to an aborting error, an
	 * interrupt, or an exception.
	 */
	if (!aborting())
	{
	    if (!eap->skip && fudi.fd_newkey != NULL)
		EMSG2(_(e_dictkey), fudi.fd_newkey);
	    vim_free(fudi.fd_newkey);
	    return;
	}
	else
	    eap->skip = TRUE;
    }

    /* An error in a function call during evaluation of an expression in magic
     * braces should not cause the function not to be defined. */
    saved_did_emsg = did_emsg;
    did_emsg = FALSE;

    /*
     * ":function func" with only function name: list function.
     */
    if (!paren)
    {
	if (!ends_excmd(*skipwhite(p)))
	{
	    EMSG(_(e_trailing));
	    goto ret_free;
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
		for (j = 0; j < fp->uf_lines.ga_len && !got_int; ++j)
		{
		    if (FUNCLINE(fp, j) == NULL)
			continue;
		    msg_putchar('\n');
		    msg_outnum((long)(j + 1));
		    if (j < 9)
			msg_putchar(' ');
		    if (j < 99)
			msg_putchar(' ');
		    msg_prt_line(FUNCLINE(fp, j), FALSE);
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
		emsg_funcname(N_("E123: Undefined function: %s"), name);
	}
	goto ret_free;
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
	    goto ret_free;
	}
	/* attempt to continue by skipping some text */
	if (vim_strchr(p, '(') != NULL)
	    p = vim_strchr(p, '(');
    }
    p = skipwhite(p + 1);

    ga_init2(&newargs, (int)sizeof(char_u *), 3);
    ga_init2(&newlines, (int)sizeof(char_u *), 3);

    if (!eap->skip)
    {
	/* Check the name of the function.  Unless it's a dictionary function
	 * (that we are overwriting). */
	if (name != NULL)
	    arg = name;
	else
	    arg = fudi.fd_newkey;
	if (arg != NULL && (fudi.fd_di == NULL
				     || fudi.fd_di->di_tv.v_type != VAR_FUNC))
	{
	    if (*arg == K_SPECIAL)
		j = 3;
	    else
		j = 0;
	    while (arg[j] != NUL && (j == 0 ? eval_isnamec1(arg[j])
						      : eval_isnamec(arg[j])))
		++j;
	    if (arg[j] != NUL)
		emsg_funcname((char *)e_invarg2, arg);
	}
    }

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

    /* find extra arguments "range", "dict" and "abort" */
    for (;;)
    {
	p = skipwhite(p);
	if (STRNCMP(p, "range", 5) == 0)
	{
	    flags |= FC_RANGE;
	    p += 5;
	}
	else if (STRNCMP(p, "dict", 4) == 0)
	{
	    flags |= FC_DICT;
	    p += 4;
	}
	else if (STRNCMP(p, "abort", 5) == 0)
	{
	    flags |= FC_ABORT;
	    p += 5;
	}
	else
	    break;
    }

    /* When there is a line break use what follows for the function body.
     * Makes 'exe "func Test()\n...\nendfunc"' work. */
    if (*p == '\n')
	line_arg = p + 1;
    else if (*p != NUL && *p != '"' && !eap->skip && !did_emsg)
	EMSG(_(e_trailing));

    /*
     * Read the body of the function, until ":endfunction" is found.
     */
    if (KeyTyped)
    {
	/* Check if the function already exists, don't let the user type the
	 * whole function before telling him it doesn't work!  For a script we
	 * need to skip the body to be able to find what follows. */
	if (!eap->skip && !eap->forceit)
	{
	    if (fudi.fd_dict != NULL && fudi.fd_newkey == NULL)
		EMSG(_(e_funcdict));
	    else if (name != NULL && find_func(name) != NULL)
		emsg_funcname(e_funcexts, name);
	}

	if (!eap->skip && did_emsg)
	    goto erret;

	msg_putchar('\n');	    /* don't overwrite the function name */
	cmdline_row = msg_row;
    }

    indent = 2;
    nesting = 0;
    for (;;)
    {
	msg_scroll = TRUE;
	need_wait_return = FALSE;
	sourcing_lnum_off = sourcing_lnum;

	if (line_arg != NULL)
	{
	    /* Use eap->arg, split up in parts by line breaks. */
	    theline = line_arg;
	    p = vim_strchr(theline, '\n');
	    if (p == NULL)
		line_arg += STRLEN(line_arg);
	    else
	    {
		*p = NUL;
		line_arg = p + 1;
	    }
	}
	else if (eap->getline == NULL)
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

	/* Detect line continuation: sourcing_lnum increased more than one. */
	if (sourcing_lnum > sourcing_lnum_off + 1)
	    sourcing_lnum_off = sourcing_lnum - sourcing_lnum_off - 1;
	else
	    sourcing_lnum_off = 0;

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

	    /* Check for "endfunction". */
	    if (checkforcmd(&p, "endfunction", 4) && nesting-- == 0)
	    {
		if (line_arg == NULL)
		    vim_free(theline);
		break;
	    }

	    /* Increase indent inside "if", "while", "for" and "try", decrease
	     * at "end". */
	    if (indent > 2 && STRNCMP(p, "end", 3) == 0)
		indent -= 2;
	    else if (STRNCMP(p, "if", 2) == 0
		    || STRNCMP(p, "wh", 2) == 0
		    || STRNCMP(p, "for", 3) == 0
		    || STRNCMP(p, "try", 3) == 0)
		indent += 2;

	    /* Check for defining a function inside this function. */
	    if (checkforcmd(&p, "function", 2))
	    {
		if (*p == '!')
		    p = skipwhite(p + 1);
		p += eval_fname_script(p);
		if (ASCII_ISALPHA(*p))
		{
		    vim_free(trans_function_name(&p, TRUE, 0, NULL));
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
			|| (p[0] == 'm' && p[1] == 'z'
				    && (!ASCII_ISALPHA(p[2]) || p[2] == 's'))
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
	if (ga_grow(&newlines, 1 + sourcing_lnum_off) == FAIL)
	{
	    if (line_arg == NULL)
		vim_free(theline);
	    goto erret;
	}

	/* Copy the line to newly allocated memory.  get_one_sourceline()
	 * allocates 250 bytes per line, this saves 80% on average.  The cost
	 * is an extra alloc/free. */
	p = vim_strsave(theline);
	if (p != NULL)
	{
	    if (line_arg == NULL)
		vim_free(theline);
	    theline = p;
	}

	((char_u **)(newlines.ga_data))[newlines.ga_len++] = theline;

	/* Add NULL lines for continuation lines, so that the line count is
	 * equal to the index in the growarray.   */
	while (sourcing_lnum_off-- > 0)
	    ((char_u **)(newlines.ga_data))[newlines.ga_len++] = NULL;

	/* Check for end of eap->arg. */
	if (line_arg != NULL && *line_arg == NUL)
	    line_arg = NULL;
    }

    /* Don't define the function when skipping commands or when an error was
     * detected. */
    if (eap->skip || did_emsg)
	goto erret;

    /*
     * If there are no errors, add the function
     */
    if (fudi.fd_dict == NULL)
    {
	v = find_var(name, &ht);
	if (v != NULL && v->di_tv.v_type == VAR_FUNC)
	{
	    emsg_funcname(N_("E707: Function name conflicts with variable: %s"),
									name);
	    goto erret;
	}

	fp = find_func(name);
	if (fp != NULL)
	{
	    if (!eap->forceit)
	    {
		emsg_funcname(e_funcexts, name);
		goto erret;
	    }
	    if (fp->uf_calls > 0)
	    {
		emsg_funcname(N_("E127: Cannot redefine function %s: It is in use"),
									name);
		goto erret;
	    }
	    /* redefine existing function */
	    ga_clear_strings(&(fp->uf_args));
	    ga_clear_strings(&(fp->uf_lines));
	    vim_free(name);
	    name = NULL;
	}
    }
    else
    {
	char	numbuf[20];

	fp = NULL;
	if (fudi.fd_newkey == NULL && !eap->forceit)
	{
	    EMSG(_(e_funcdict));
	    goto erret;
	}
	if (fudi.fd_di == NULL)
	{
	    /* Can't add a function to a locked dictionary */
	    if (tv_check_lock(fudi.fd_dict->dv_lock, eap->arg))
		goto erret;
	}
	    /* Can't change an existing function if it is locked */
	else if (tv_check_lock(fudi.fd_di->di_tv.v_lock, eap->arg))
	    goto erret;

	/* Give the function a sequential number.  Can only be used with a
	 * Funcref! */
	vim_free(name);
	sprintf(numbuf, "%d", ++func_nr);
	name = vim_strsave((char_u *)numbuf);
	if (name == NULL)
	    goto erret;
    }

    if (fp == NULL)
    {
	if (fudi.fd_dict == NULL && vim_strchr(name, AUTOLOAD_CHAR) != NULL)
	{
	    int	    slen, plen;
	    char_u  *scriptname;

	    /* Check that the autoload name matches the script name. */
	    j = FAIL;
	    if (sourcing_name != NULL)
	    {
		scriptname = autoload_name(name);
		if (scriptname != NULL)
		{
		    p = vim_strchr(scriptname, '/');
		    plen = (int)STRLEN(p);
		    slen = (int)STRLEN(sourcing_name);
		    if (slen > plen && fnamecmp(p,
					    sourcing_name + slen - plen) == 0)
			j = OK;
		    vim_free(scriptname);
		}
	    }
	    if (j == FAIL)
	    {
		EMSG2(_("E746: Function name does not match script file name: %s"), name);
		goto erret;
	    }
	}

	fp = (ufunc_T *)alloc((unsigned)(sizeof(ufunc_T) + STRLEN(name)));
	if (fp == NULL)
	    goto erret;

	if (fudi.fd_dict != NULL)
	{
	    if (fudi.fd_di == NULL)
	    {
		/* add new dict entry */
		fudi.fd_di = dictitem_alloc(fudi.fd_newkey);
		if (fudi.fd_di == NULL)
		{
		    vim_free(fp);
		    goto erret;
		}
		if (dict_add(fudi.fd_dict, fudi.fd_di) == FAIL)
		{
		    vim_free(fudi.fd_di);
		    vim_free(fp);
		    goto erret;
		}
	    }
	    else
		/* overwrite existing dict entry */
		clear_tv(&fudi.fd_di->di_tv);
	    fudi.fd_di->di_tv.v_type = VAR_FUNC;
	    fudi.fd_di->di_tv.v_lock = 0;
	    fudi.fd_di->di_tv.vval.v_string = vim_strsave(name);
	    fp->uf_refcount = 1;

	    /* behave like "dict" was used */
	    flags |= FC_DICT;
	}

	/* insert the new function in the function list */
	STRCPY(fp->uf_name, name);
	hash_add(&func_hashtab, UF2HIKEY(fp));
    }
    fp->uf_args = newargs;
    fp->uf_lines = newlines;
#ifdef FEAT_PROFILE
    fp->uf_tml_count = NULL;
    fp->uf_tml_total = NULL;
    fp->uf_tml_self = NULL;
    fp->uf_profiling = FALSE;
    if (prof_def_func())
	func_do_profile(fp);
#endif
    fp->uf_varargs = varargs;
    fp->uf_flags = flags;
    fp->uf_calls = 0;
    fp->uf_script_ID = current_SID;
    goto ret_free;

erret:
    ga_clear_strings(&newargs);
    ga_clear_strings(&newlines);
ret_free:
    vim_free(skip_until);
    vim_free(fudi.fd_newkey);
    vim_free(name);
    did_emsg |= saved_did_emsg;
}

/*
 * Get a function name, translating "<SID>" and "<SNR>".
 * Also handles a Funcref in a List or Dictionary.
 * Returns the function name in allocated memory, or NULL for failure.
 * flags:
 * TFN_INT:   internal function name OK
 * TFN_QUIET: be quiet
 * Advances "pp" to just after the function name (if no error).
 */
    static char_u *
trans_function_name(pp, skip, flags, fdp)
    char_u	**pp;
    int		skip;		/* only find the end, don't evaluate */
    int		flags;
    funcdict_T	*fdp;		/* return: info about dictionary used */
{
    char_u	*name = NULL;
    char_u	*start;
    char_u	*end;
    int		lead;
    char_u	sid_buf[20];
    int		len;
    lval_T	lv;

    if (fdp != NULL)
	vim_memset(fdp, 0, sizeof(funcdict_T));
    start = *pp;

    /* Check for hard coded <SNR>: already translated function ID (from a user
     * command). */
    if ((*pp)[0] == K_SPECIAL && (*pp)[1] == KS_EXTRA
						   && (*pp)[2] == (int)KE_SNR)
    {
	*pp += 3;
	len = get_id_len(pp) + 3;
	return vim_strnsave(start, len);
    }

    /* A name starting with "<SID>" or "<SNR>" is local to a script.  But
     * don't skip over "s:", get_lval() needs it for "s:dict.func". */
    lead = eval_fname_script(start);
    if (lead > 2)
	start += lead;

    end = get_lval(start, NULL, &lv, FALSE, skip, flags & TFN_QUIET,
					      lead > 2 ? 0 : FNE_CHECK_START);
    if (end == start)
    {
	if (!skip)
	    EMSG(_("E129: Function name required"));
	goto theend;
    }
    if (end == NULL || (lv.ll_tv != NULL && (lead > 2 || lv.ll_range)))
    {
	/*
	 * Report an invalid expression in braces, unless the expression
	 * evaluation has been cancelled due to an aborting error, an
	 * interrupt, or an exception.
	 */
	if (!aborting())
	{
	    if (end != NULL)
		EMSG2(_(e_invarg2), start);
	}
	else
	    *pp = find_name_end(start, NULL, NULL, FNE_INCL_BR);
	goto theend;
    }

    if (lv.ll_tv != NULL)
    {
	if (fdp != NULL)
	{
	    fdp->fd_dict = lv.ll_dict;
	    fdp->fd_newkey = lv.ll_newkey;
	    lv.ll_newkey = NULL;
	    fdp->fd_di = lv.ll_di;
	}
	if (lv.ll_tv->v_type == VAR_FUNC && lv.ll_tv->vval.v_string != NULL)
	{
	    name = vim_strsave(lv.ll_tv->vval.v_string);
	    *pp = end;
	}
	else
	{
	    if (!skip && !(flags & TFN_QUIET) && (fdp == NULL
			     || lv.ll_dict == NULL || fdp->fd_newkey == NULL))
		EMSG(_(e_funcref));
	    else
		*pp = end;
	    name = NULL;
	}
	goto theend;
    }

    if (lv.ll_name == NULL)
    {
	/* Error found, but continue after the function name. */
	*pp = end;
	goto theend;
    }

    /* Check if the name is a Funcref.  If so, use the value. */
    if (lv.ll_exp_name != NULL)
    {
	len = (int)STRLEN(lv.ll_exp_name);
	name = deref_func_name(lv.ll_exp_name, &len);
	if (name == lv.ll_exp_name)
	    name = NULL;
    }
    else
    {
	len = (int)(end - *pp);
	name = deref_func_name(*pp, &len);
	if (name == *pp)
	    name = NULL;
    }
    if (name != NULL)
    {
	name = vim_strsave(name);
	*pp = end;
	goto theend;
    }

    if (lv.ll_exp_name != NULL)
    {
	len = (int)STRLEN(lv.ll_exp_name);
	if (lead <= 2 && lv.ll_name == lv.ll_exp_name
					 && STRNCMP(lv.ll_name, "s:", 2) == 0)
	{
	    /* When there was "s:" already or the name expanded to get a
	     * leading "s:" then remove it. */
	    lv.ll_name += 2;
	    len -= 2;
	    lead = 2;
	}
    }
    else
    {
	if (lead == 2)	/* skip over "s:" */
	    lv.ll_name += 2;
	len = (int)(end - lv.ll_name);
    }

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
	if ((lv.ll_exp_name != NULL && eval_fname_sid(lv.ll_exp_name))
						       || eval_fname_sid(*pp))
	{
	    /* It's "s:" or "<SID>" */
	    if (current_SID <= 0)
	    {
		EMSG(_(e_usingsid));
		goto theend;
	    }
	    sprintf((char *)sid_buf, "%ld_", (long)current_SID);
	    lead += (int)STRLEN(sid_buf);
	}
    }
    else if (!(flags & TFN_INT) && builtin_function(lv.ll_name))
    {
	EMSG2(_("E128: Function name must start with a capital or contain a colon: %s"), lv.ll_name);
	goto theend;
    }
    name = alloc((unsigned)(len + lead + 1));
    if (name != NULL)
    {
	if (lead > 0)
	{
	    name[0] = K_SPECIAL;
	    name[1] = KS_EXTRA;
	    name[2] = (int)KE_SNR;
	    if (lead > 3)	/* If it's "<SID>" */
		STRCPY(name + 3, sid_buf);
	}
	mch_memmove(name + lead, lv.ll_name, (size_t)len);
	name[len + lead] = NUL;
    }
    *pp = end;

theend:
    clear_lval(&lv);
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
    if (fp->uf_name[0] == K_SPECIAL)
    {
	MSG_PUTS_ATTR("<SNR>", hl_attr(HLF_8));
	msg_puts(fp->uf_name + 3);
    }
    else
	msg_puts(fp->uf_name);
    msg_putchar('(');
    for (j = 0; j < fp->uf_args.ga_len; ++j)
    {
	if (j)
	    MSG_PUTS(", ");
	msg_puts(FUNCARG(fp, j));
    }
    if (fp->uf_varargs)
    {
	if (j)
	    MSG_PUTS(", ");
	MSG_PUTS("...");
    }
    msg_putchar(')');
    msg_clr_eos();
    if (p_verbose > 0)
	last_set_msg(fp->uf_script_ID);
}

/*
 * Find a function by name, return pointer to it in ufuncs.
 * Return NULL for unknown function.
 */
    static ufunc_T *
find_func(name)
    char_u	*name;
{
    hashitem_T	*hi;

    hi = hash_find(&func_hashtab, name);
    if (!HASHITEM_EMPTY(hi))
	return HI2UF(hi);
    return NULL;
}

#if defined(EXITFREE) || defined(PROTO)
    void
free_all_functions()
{
    hashitem_T	*hi;

    /* Need to start all over every time, because func_free() may change the
     * hash table. */
    while (func_hashtab.ht_used > 0)
	for (hi = func_hashtab.ht_array; ; ++hi)
	    if (!HASHITEM_EMPTY(hi))
	    {
		func_free(HI2UF(hi));
		break;
	    }
}
#endif

/*
 * Return TRUE if a function "name" exists.
 */
    static int
function_exists(name)
    char_u *name;
{
    char_u  *nm = name;
    char_u  *p;
    int	    n = FALSE;

    p = trans_function_name(&nm, FALSE, TFN_INT|TFN_QUIET, NULL);
    nm = skipwhite(nm);

    /* Only accept "funcname", "funcname ", "funcname (..." and
     * "funcname(...", not "funcname!...". */
    if (p != NULL && (*nm == NUL || *nm == '('))
    {
	if (builtin_function(p))
	    n = (find_internal_func(p) >= 0);
	else
	    n = (find_func(p) != NULL);
    }
    vim_free(p);
    return n;
}

/*
 * Return TRUE if "name" looks like a builtin function name: starts with a
 * lower case letter and doesn't contain a ':' or AUTOLOAD_CHAR.
 */
    static int
builtin_function(name)
    char_u *name;
{
    return ASCII_ISLOWER(name[0]) && vim_strchr(name, ':') == NULL
				   && vim_strchr(name, AUTOLOAD_CHAR) == NULL;
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Start profiling function "fp".
 */
    static void
func_do_profile(fp)
    ufunc_T	*fp;
{
    fp->uf_tm_count = 0;
    profile_zero(&fp->uf_tm_self);
    profile_zero(&fp->uf_tm_total);
    if (fp->uf_tml_count == NULL)
	fp->uf_tml_count = (int *)alloc_clear((unsigned)
					 (sizeof(int) * fp->uf_lines.ga_len));
    if (fp->uf_tml_total == NULL)
	fp->uf_tml_total = (proftime_T *)alloc_clear((unsigned)
				  (sizeof(proftime_T) * fp->uf_lines.ga_len));
    if (fp->uf_tml_self == NULL)
	fp->uf_tml_self = (proftime_T *)alloc_clear((unsigned)
				  (sizeof(proftime_T) * fp->uf_lines.ga_len));
    fp->uf_tml_idx = -1;
    if (fp->uf_tml_count == NULL || fp->uf_tml_total == NULL
						   || fp->uf_tml_self == NULL)
	return;	    /* out of memory */

    fp->uf_profiling = TRUE;
}

/*
 * Dump the profiling results for all functions in file "fd".
 */
    void
func_dump_profile(fd)
    FILE    *fd;
{
    hashitem_T	*hi;
    int		todo;
    ufunc_T	*fp;
    int		i;
    ufunc_T	**sorttab;
    int		st_len = 0;

    todo = (int)func_hashtab.ht_used;
    if (todo == 0)
	return;     /* nothing to dump */

    sorttab = (ufunc_T **)alloc((unsigned)(sizeof(ufunc_T) * todo));

    for (hi = func_hashtab.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    fp = HI2UF(hi);
	    if (fp->uf_profiling)
	    {
		if (sorttab != NULL)
		    sorttab[st_len++] = fp;

		if (fp->uf_name[0] == K_SPECIAL)
		    fprintf(fd, "FUNCTION  <SNR>%s()\n", fp->uf_name + 3);
		else
		    fprintf(fd, "FUNCTION  %s()\n", fp->uf_name);
		if (fp->uf_tm_count == 1)
		    fprintf(fd, "Called 1 time\n");
		else
		    fprintf(fd, "Called %d times\n", fp->uf_tm_count);
		fprintf(fd, "Total time: %s\n", profile_msg(&fp->uf_tm_total));
		fprintf(fd, " Self time: %s\n", profile_msg(&fp->uf_tm_self));
		fprintf(fd, "\n");
		fprintf(fd, "count  total (s)   self (s)\n");

		for (i = 0; i < fp->uf_lines.ga_len; ++i)
		{
		    if (FUNCLINE(fp, i) == NULL)
			continue;
		    prof_func_line(fd, fp->uf_tml_count[i],
			     &fp->uf_tml_total[i], &fp->uf_tml_self[i], TRUE);
		    fprintf(fd, "%s\n", FUNCLINE(fp, i));
		}
		fprintf(fd, "\n");
	    }
	}
    }

    if (sorttab != NULL && st_len > 0)
    {
	qsort((void *)sorttab, (size_t)st_len, sizeof(ufunc_T *),
							      prof_total_cmp);
	prof_sort_list(fd, sorttab, st_len, "TOTAL", FALSE);
	qsort((void *)sorttab, (size_t)st_len, sizeof(ufunc_T *),
							      prof_self_cmp);
	prof_sort_list(fd, sorttab, st_len, "SELF", TRUE);
    }

    vim_free(sorttab);
}

    static void
prof_sort_list(fd, sorttab, st_len, title, prefer_self)
    FILE	*fd;
    ufunc_T	**sorttab;
    int		st_len;
    char	*title;
    int		prefer_self;	/* when equal print only self time */
{
    int		i;
    ufunc_T	*fp;

    fprintf(fd, "FUNCTIONS SORTED ON %s TIME\n", title);
    fprintf(fd, "count  total (s)   self (s)  function\n");
    for (i = 0; i < 20 && i < st_len; ++i)
    {
	fp = sorttab[i];
	prof_func_line(fd, fp->uf_tm_count, &fp->uf_tm_total, &fp->uf_tm_self,
								 prefer_self);
	if (fp->uf_name[0] == K_SPECIAL)
	    fprintf(fd, " <SNR>%s()\n", fp->uf_name + 3);
	else
	    fprintf(fd, " %s()\n", fp->uf_name);
    }
    fprintf(fd, "\n");
}

/*
 * Print the count and times for one function or function line.
 */
    static void
prof_func_line(fd, count, total, self, prefer_self)
    FILE	*fd;
    int		count;
    proftime_T	*total;
    proftime_T	*self;
    int		prefer_self;	/* when equal print only self time */
{
    if (count > 0)
    {
	fprintf(fd, "%5d ", count);
	if (prefer_self && profile_equal(total, self))
	    fprintf(fd, "           ");
	else
	    fprintf(fd, "%s ", profile_msg(total));
	if (!prefer_self && profile_equal(total, self))
	    fprintf(fd, "           ");
	else
	    fprintf(fd, "%s ", profile_msg(self));
    }
    else
	fprintf(fd, "                            ");
}

/*
 * Compare function for total time sorting.
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
prof_total_cmp(s1, s2)
    const void	*s1;
    const void	*s2;
{
    ufunc_T	*p1, *p2;

    p1 = *(ufunc_T **)s1;
    p2 = *(ufunc_T **)s2;
    return profile_cmp(&p1->uf_tm_total, &p2->uf_tm_total);
}

/*
 * Compare function for self time sorting.
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
prof_self_cmp(s1, s2)
    const void	*s1;
    const void	*s2;
{
    ufunc_T	*p1, *p2;

    p1 = *(ufunc_T **)s1;
    p2 = *(ufunc_T **)s2;
    return profile_cmp(&p1->uf_tm_self, &p2->uf_tm_self);
}

#endif

/*
 * If "name" has a package name try autoloading the script for it.
 * Return TRUE if a package was loaded.
 */
    static int
script_autoload(name, reload)
    char_u	*name;
    int		reload;	    /* load script again when already loaded */
{
    char_u	*p;
    char_u	*scriptname, *tofree;
    int		ret = FALSE;
    int		i;

    /* If there is no '#' after name[0] there is no package name. */
    p = vim_strchr(name, AUTOLOAD_CHAR);
    if (p == NULL || p == name)
	return FALSE;

    tofree = scriptname = autoload_name(name);

    /* Find the name in the list of previously loaded package names.  Skip
     * "autoload/", it's always the same. */
    for (i = 0; i < ga_loaded.ga_len; ++i)
	if (STRCMP(((char_u **)ga_loaded.ga_data)[i] + 9, scriptname + 9) == 0)
	    break;
    if (!reload && i < ga_loaded.ga_len)
	ret = FALSE;	    /* was loaded already */
    else
    {
	/* Remember the name if it wasn't loaded already. */
	if (i == ga_loaded.ga_len && ga_grow(&ga_loaded, 1) == OK)
	{
	    ((char_u **)ga_loaded.ga_data)[ga_loaded.ga_len++] = scriptname;
	    tofree = NULL;
	}

	/* Try loading the package from $VIMRUNTIME/autoload/<name>.vim */
	if (source_runtime(scriptname, FALSE) == OK)
	    ret = TRUE;
    }

    vim_free(tofree);
    return ret;
}

/*
 * Return the autoload script name for a function or variable name.
 * Returns NULL when out of memory.
 */
    static char_u *
autoload_name(name)
    char_u	*name;
{
    char_u	*p;
    char_u	*scriptname;

    /* Get the script file name: replace '#' with '/', append ".vim". */
    scriptname = alloc((unsigned)(STRLEN(name) + 14));
    if (scriptname == NULL)
	return FALSE;
    STRCPY(scriptname, "autoload/");
    STRCAT(scriptname, name);
    *vim_strrchr(scriptname, AUTOLOAD_CHAR) = NUL;
    STRCAT(scriptname, ".vim");
    while ((p = vim_strchr(scriptname, AUTOLOAD_CHAR)) != NULL)
	*p = '/';
    return scriptname;
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
    static long_u	done;
    static hashitem_T	*hi;
    ufunc_T		*fp;

    if (idx == 0)
    {
	done = 0;
	hi = func_hashtab.ht_array;
    }
    if (done < func_hashtab.ht_used)
    {
	if (done++ > 0)
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	fp = HI2UF(hi);

	if (STRLEN(fp->uf_name) + 4 >= IOSIZE)
	    return fp->uf_name;	/* prevents overflow */

	cat_func_name(IObuff, fp);
	if (xp->xp_context != EXPAND_USER_FUNC)
	{
	    STRCAT(IObuff, "(");
	    if (!fp->uf_varargs && fp->uf_args.ga_len == 0)
		STRCAT(IObuff, ")");
	}
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
    if (fp->uf_name[0] == K_SPECIAL)
    {
	STRCPY(buf, "<SNR>");
	STRCAT(buf, fp->uf_name + 3);
    }
    else
	STRCPY(buf, fp->uf_name);
}

/*
 * ":delfunction {name}"
 */
    void
ex_delfunction(eap)
    exarg_T	*eap;
{
    ufunc_T	*fp = NULL;
    char_u	*p;
    char_u	*name;
    funcdict_T	fudi;

    p = eap->arg;
    name = trans_function_name(&p, eap->skip, 0, &fudi);
    vim_free(fudi.fd_newkey);
    if (name == NULL)
    {
	if (fudi.fd_dict != NULL && !eap->skip)
	    EMSG(_(e_funcref));
	return;
    }
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
	    EMSG2(_(e_nofunc), eap->arg);
	    return;
	}
	if (fp->uf_calls > 0)
	{
	    EMSG2(_("E131: Cannot delete function %s: It is in use"), eap->arg);
	    return;
	}

	if (fudi.fd_dict != NULL)
	{
	    /* Delete the dict item that refers to the function, it will
	     * invoke func_unref() and possibly delete the function. */
	    dictitem_remove(fudi.fd_dict, fudi.fd_di);
	}
	else
	    func_free(fp);
    }
}

/*
 * Free a function and remove it from the list of functions.
 */
    static void
func_free(fp)
    ufunc_T *fp;
{
    hashitem_T	*hi;

    /* clear this function */
    ga_clear_strings(&(fp->uf_args));
    ga_clear_strings(&(fp->uf_lines));
#ifdef FEAT_PROFILE
    vim_free(fp->uf_tml_count);
    vim_free(fp->uf_tml_total);
    vim_free(fp->uf_tml_self);
#endif

    /* remove the function from the function hashtable */
    hi = hash_find(&func_hashtab, UF2HIKEY(fp));
    if (HASHITEM_EMPTY(hi))
	EMSG2(_(e_intern2), "func_free()");
    else
	hash_remove(&func_hashtab, hi);

    vim_free(fp);
}

/*
 * Unreference a Function: decrement the reference count and free it when it
 * becomes zero.  Only for numbered functions.
 */
    static void
func_unref(name)
    char_u	*name;
{
    ufunc_T *fp;

    if (name != NULL && isdigit(*name))
    {
	fp = find_func(name);
	if (fp == NULL)
	    EMSG2(_(e_intern2), "func_unref()");
	else if (--fp->uf_refcount <= 0)
	{
	    /* Only delete it when it's not being used.  Otherwise it's done
	     * when "uf_calls" becomes zero. */
	    if (fp->uf_calls == 0)
		func_free(fp);
	}
    }
}

/*
 * Count a reference to a Function.
 */
    static void
func_ref(name)
    char_u	*name;
{
    ufunc_T *fp;

    if (name != NULL && isdigit(*name))
    {
	fp = find_func(name);
	if (fp == NULL)
	    EMSG2(_(e_intern2), "func_ref()");
	else
	    ++fp->uf_refcount;
    }
}

/*
 * Call a user function.
 */
    static void
call_user_func(fp, argcount, argvars, rettv, firstline, lastline, selfdict)
    ufunc_T	*fp;		/* pointer to function */
    int		argcount;	/* nr of args */
    typval_T	*argvars;	/* arguments */
    typval_T	*rettv;		/* return value */
    linenr_T	firstline;	/* first line of range */
    linenr_T	lastline;	/* last line of range */
    dict_T	*selfdict;	/* Dictionary for "self" */
{
    char_u	*save_sourcing_name;
    linenr_T	save_sourcing_lnum;
    scid_T	save_current_SID;
    funccall_T	*fc;
    int		save_did_emsg;
    static int	depth = 0;
    dictitem_T	*v;
    int		fixvar_idx = 0;	/* index in fixvar[] */
    int		i;
    int		ai;
    char_u	numbuf[NUMBUFLEN];
    char_u	*name;
#ifdef FEAT_PROFILE
    proftime_T	wait_start;
    proftime_T	call_start;
#endif

    /* If depth of calling is getting too high, don't execute the function */
    if (depth >= p_mfd)
    {
	EMSG(_("E132: Function call depth is higher than 'maxfuncdepth'"));
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = -1;
	return;
    }
    ++depth;

    line_breakcheck();		/* check for CTRL-C hit */

    fc = (funccall_T *)alloc(sizeof(funccall_T));
    fc->caller = current_funccal;
    current_funccal = fc;
    fc->func = fp;
    fc->rettv = rettv;
    rettv->vval.v_number = 0;
    fc->linenr = 0;
    fc->returned = FALSE;
    fc->level = ex_nesting_level;
    /* Check if this function has a breakpoint. */
    fc->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name, (linenr_T)0);
    fc->dbg_tick = debug_tick;

    /*
     * Note about using fc->fixvar[]: This is an array of FIXVAR_CNT variables
     * with names up to VAR_SHORT_LEN long.  This avoids having to alloc/free
     * each argument variable and saves a lot of time.
     */
    /*
     * Init l: variables.
     */
    init_var_dict(&fc->l_vars, &fc->l_vars_var);
    if (selfdict != NULL)
    {
	/* Set l:self to "selfdict".  Use "name" to avoid a warning from
	 * some compiler that checks the destination size. */
	v = &fc->fixvar[fixvar_idx++].var;
	name = v->di_key;
	STRCPY(name, "self");
	v->di_flags = DI_FLAGS_RO + DI_FLAGS_FIX;
	hash_add(&fc->l_vars.dv_hashtab, DI2HIKEY(v));
	v->di_tv.v_type = VAR_DICT;
	v->di_tv.v_lock = 0;
	v->di_tv.vval.v_dict = selfdict;
	++selfdict->dv_refcount;
    }

    /*
     * Init a: variables.
     * Set a:0 to "argcount".
     * Set a:000 to a list with room for the "..." arguments.
     */
    init_var_dict(&fc->l_avars, &fc->l_avars_var);
    add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "0",
				(varnumber_T)(argcount - fp->uf_args.ga_len));
    /* Use "name" to avoid a warning from some compiler that checks the
     * destination size. */
    v = &fc->fixvar[fixvar_idx++].var;
    name = v->di_key;
    STRCPY(name, "000");
    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    hash_add(&fc->l_avars.dv_hashtab, DI2HIKEY(v));
    v->di_tv.v_type = VAR_LIST;
    v->di_tv.v_lock = VAR_FIXED;
    v->di_tv.vval.v_list = &fc->l_varlist;
    vim_memset(&fc->l_varlist, 0, sizeof(list_T));
    fc->l_varlist.lv_refcount = DO_NOT_FREE_CNT;
    fc->l_varlist.lv_lock = VAR_FIXED;

    /*
     * Set a:firstline to "firstline" and a:lastline to "lastline".
     * Set a:name to named arguments.
     * Set a:N to the "..." arguments.
     */
    add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "firstline",
						      (varnumber_T)firstline);
    add_nr_var(&fc->l_avars, &fc->fixvar[fixvar_idx++].var, "lastline",
						       (varnumber_T)lastline);
    for (i = 0; i < argcount; ++i)
    {
	ai = i - fp->uf_args.ga_len;
	if (ai < 0)
	    /* named argument a:name */
	    name = FUNCARG(fp, i);
	else
	{
	    /* "..." argument a:1, a:2, etc. */
	    sprintf((char *)numbuf, "%d", ai + 1);
	    name = numbuf;
	}
	if (fixvar_idx < FIXVAR_CNT && STRLEN(name) <= VAR_SHORT_LEN)
	{
	    v = &fc->fixvar[fixvar_idx++].var;
	    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
	}
	else
	{
	    v = (dictitem_T *)alloc((unsigned)(sizeof(dictitem_T)
							     + STRLEN(name)));
	    if (v == NULL)
		break;
	    v->di_flags = DI_FLAGS_RO;
	}
	STRCPY(v->di_key, name);
	hash_add(&fc->l_avars.dv_hashtab, DI2HIKEY(v));

	/* Note: the values are copied directly to avoid alloc/free.
	 * "argvars" must have VAR_FIXED for v_lock. */
	v->di_tv = argvars[i];
	v->di_tv.v_lock = VAR_FIXED;

	if (ai >= 0 && ai < MAX_FUNC_ARGS)
	{
	    list_append(&fc->l_varlist, &fc->l_listitems[ai]);
	    fc->l_listitems[ai].li_tv = argvars[i];
	    fc->l_listitems[ai].li_tv.v_lock = VAR_FIXED;
	}
    }

    /* Don't redraw while executing the function. */
    ++RedrawingDisabled;
    save_sourcing_name = sourcing_name;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 1;
    sourcing_name = alloc((unsigned)((save_sourcing_name == NULL ? 0
		: STRLEN(save_sourcing_name)) + STRLEN(fp->uf_name) + 13));
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
	    verbose_enter_scroll();

	    smsg((char_u *)_("calling %s"), sourcing_name);
	    if (p_verbose >= 14)
	    {
		char_u	buf[MSG_BUF_LEN];
		char_u	numbuf2[NUMBUFLEN];
		char_u	*tofree;
		char_u	*s;

		msg_puts((char_u *)"(");
		for (i = 0; i < argcount; ++i)
		{
		    if (i > 0)
			msg_puts((char_u *)", ");
		    if (argvars[i].v_type == VAR_NUMBER)
			msg_outnum((long)argvars[i].vval.v_number);
		    else
		    {
			s = tv2string(&argvars[i], &tofree, numbuf2, 0);
			if (s != NULL)
			{
			    trunc_string(s, buf, MSG_BUF_CLEN);
			    msg_puts(buf);
			    vim_free(tofree);
			}
		    }
		}
		msg_puts((char_u *)")");
	    }
	    msg_puts((char_u *)"\n");   /* don't overwrite this either */

	    verbose_leave_scroll();
	    --no_wait_return;
	}
    }
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
    {
	if (!fp->uf_profiling && has_profiling(FALSE, fp->uf_name, NULL))
	    func_do_profile(fp);
	if (fp->uf_profiling
		    || (fc->caller != NULL && fc->caller->func->uf_profiling))
	{
	    ++fp->uf_tm_count;
	    profile_start(&call_start);
	    profile_zero(&fp->uf_tm_children);
	}
	script_prof_save(&wait_start);
    }
#endif

    save_current_SID = current_SID;
    current_SID = fp->uf_script_ID;
    save_did_emsg = did_emsg;
    did_emsg = FALSE;

    /* call do_cmdline() to execute the lines */
    do_cmdline(NULL, get_func_line, (void *)fc,
				     DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT);

    --RedrawingDisabled;

    /* when the function was aborted because of an error, return -1 */
    if ((did_emsg && (fp->uf_flags & FC_ABORT)) || rettv->v_type == VAR_UNKNOWN)
    {
	clear_tv(rettv);
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = -1;
    }

#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES && (fp->uf_profiling
		    || (fc->caller != NULL && fc->caller->func->uf_profiling)))
    {
	profile_end(&call_start);
	profile_sub_wait(&wait_start, &call_start);
	profile_add(&fp->uf_tm_total, &call_start);
	profile_self(&fp->uf_tm_self, &call_start, &fp->uf_tm_children);
	if (fc->caller != NULL && fc->caller->func->uf_profiling)
	{
	    profile_add(&fc->caller->func->uf_tm_children, &call_start);
	    profile_add(&fc->caller->func->uf_tml_children, &call_start);
	}
    }
#endif

    /* when being verbose, mention the return value */
    if (p_verbose >= 12)
    {
	++no_wait_return;
	verbose_enter_scroll();

	if (aborting())
	    smsg((char_u *)_("%s aborted"), sourcing_name);
	else if (fc->rettv->v_type == VAR_NUMBER)
	    smsg((char_u *)_("%s returning #%ld"), sourcing_name,
					       (long)fc->rettv->vval.v_number);
	else
	{
	    char_u	buf[MSG_BUF_LEN];
	    char_u	numbuf2[NUMBUFLEN];
	    char_u	*tofree;
	    char_u	*s;

	    /* The value may be very long.  Skip the middle part, so that we
	     * have some idea how it starts and ends. smsg() would always
	     * truncate it at the end. */
	    s = tv2string(fc->rettv, &tofree, numbuf2, 0);
	    if (s != NULL)
	    {
		trunc_string(s, buf, MSG_BUF_CLEN);
		smsg((char_u *)_("%s returning %s"), sourcing_name, buf);
		vim_free(tofree);
	    }
	}
	msg_puts((char_u *)"\n");   /* don't overwrite this either */

	verbose_leave_scroll();
	--no_wait_return;
    }

    vim_free(sourcing_name);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    current_SID = save_current_SID;
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	script_prof_restore(&wait_start);
#endif

    if (p_verbose >= 12 && sourcing_name != NULL)
    {
	++no_wait_return;
	verbose_enter_scroll();

	smsg((char_u *)_("continuing in %s"), sourcing_name);
	msg_puts((char_u *)"\n");   /* don't overwrite this either */

	verbose_leave_scroll();
	--no_wait_return;
    }

    did_emsg |= save_did_emsg;
    current_funccal = fc->caller;
    --depth;

    /* if the a:000 list and the a: dict are not referenced we can free the
     * funccall_T and what's in it. */
    if (fc->l_varlist.lv_refcount == DO_NOT_FREE_CNT
	    && fc->l_vars.dv_refcount == DO_NOT_FREE_CNT
	    && fc->l_avars.dv_refcount == DO_NOT_FREE_CNT)
    {
	free_funccal(fc, FALSE);
    }
    else
    {
	hashitem_T	*hi;
	listitem_T	*li;
	int		todo;

	/* "fc" is still in use.  This can happen when returning "a:000" or
	 * assigning "l:" to a global variable.
	 * Link "fc" in the list for garbage collection later. */
	fc->caller = previous_funccal;
	previous_funccal = fc;

	/* Make a copy of the a: variables, since we didn't do that above. */
	todo = (int)fc->l_avars.dv_hashtab.ht_used;
	for (hi = fc->l_avars.dv_hashtab.ht_array; todo > 0; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		v = HI2DI(hi);
		copy_tv(&v->di_tv, &v->di_tv);
	    }
	}

	/* Make a copy of the a:000 items, since we didn't do that above. */
	for (li = fc->l_varlist.lv_first; li != NULL; li = li->li_next)
	    copy_tv(&li->li_tv, &li->li_tv);
    }
}

/*
 * Return TRUE if items in "fc" do not have "copyID".  That means they are not
 * referenced from anywhere.
 */
    static int
can_free_funccal(fc, copyID)
    funccall_T	*fc;
    int		copyID;
{
    return (fc->l_varlist.lv_copyID != copyID
	    && fc->l_vars.dv_copyID != copyID
	    && fc->l_avars.dv_copyID != copyID);
}

/*
 * Free "fc" and what it contains.
 */
   static void
free_funccal(fc, free_val)
    funccall_T	*fc;
    int		free_val;  /* a: vars were allocated */
{
    listitem_T	*li;

    /* The a: variables typevals may not have been allocated, only free the
     * allocated variables. */
    vars_clear_ext(&fc->l_avars.dv_hashtab, free_val);

    /* free all l: variables */
    vars_clear(&fc->l_vars.dv_hashtab);

    /* Free the a:000 variables if they were allocated. */
    if (free_val)
	for (li = fc->l_varlist.lv_first; li != NULL; li = li->li_next)
	    clear_tv(&li->li_tv);

    vim_free(fc);
}

/*
 * Add a number variable "name" to dict "dp" with value "nr".
 */
    static void
add_nr_var(dp, v, name, nr)
    dict_T	*dp;
    dictitem_T	*v;
    char	*name;
    varnumber_T nr;
{
    STRCPY(v->di_key, name);
    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    hash_add(&dp->dv_hashtab, DI2HIKEY(v));
    v->di_tv.v_type = VAR_NUMBER;
    v->di_tv.v_lock = VAR_FIXED;
    v->di_tv.vval.v_number = nr;
}

/*
 * ":return [expr]"
 */
    void
ex_return(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    typval_T	rettv;
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
	    && eval0(arg, &rettv, &eap->nextcmd, !eap->skip) != FAIL)
    {
	if (!eap->skip)
	    returning = do_return(eap, FALSE, TRUE, &rettv);
	else
	    clear_tv(&rettv);
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
 * when called due to a ":return" command.  "rettv" may point to a typval_T
 * with the return rettv.  Returns TRUE when the return can be carried out,
 * FALSE when the return gets pending.
 */
    int
do_return(eap, reanimate, is_cmd, rettv)
    exarg_T	*eap;
    int		reanimate;
    int		is_cmd;
    void	*rettv;
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
	    /* A pending return again gets pending.  "rettv" points to an
	     * allocated variable with the rettv of the original ":return"'s
	     * argument if present or is NULL else. */
	    cstack->cs_rettv[idx] = rettv;
	else
	{
	    /* When undoing a return in order to make it pending, get the stored
	     * return rettv. */
	    if (reanimate)
		rettv = current_funccal->rettv;

	    if (rettv != NULL)
	    {
		/* Store the value of the pending return. */
		if ((cstack->cs_rettv[idx] = alloc_tv()) != NULL)
		    *(typval_T *)cstack->cs_rettv[idx] = *(typval_T *)rettv;
		else
		    EMSG(_(e_outofmem));
	    }
	    else
		cstack->cs_rettv[idx] = NULL;

	    if (reanimate)
	    {
		/* The pending return value could be overwritten by a ":return"
		 * without argument in a finally clause; reset the default
		 * return value. */
		current_funccal->rettv->v_type = VAR_NUMBER;
		current_funccal->rettv->vval.v_number = 0;
	    }
	}
	report_make_pending(CSTP_RETURN, rettv);
    }
    else
    {
	current_funccal->returned = TRUE;

	/* If the return is carried out now, store the return value.  For
	 * a return immediately after reanimation, the value is already
	 * there. */
	if (!reanimate && rettv != NULL)
	{
	    clear_tv(current_funccal->rettv);
	    *current_funccal->rettv = *(typval_T *)rettv;
	    if (!is_cmd)
		vim_free(rettv);
	}
    }

    return idx < 0;
}

/*
 * Free the variable with a pending return value.
 */
    void
discard_pending_return(rettv)
    void	*rettv;
{
    free_tv((typval_T *)rettv);
}

/*
 * Generate a return command for producing the value of "rettv".  The result
 * is an allocated string.  Used by report_pending() for verbose messages.
 */
    char_u *
get_return_cmd(rettv)
    void	*rettv;
{
    char_u	*s = NULL;
    char_u	*tofree = NULL;
    char_u	numbuf[NUMBUFLEN];

    if (rettv != NULL)
	s = echo_string((typval_T *)rettv, &tofree, numbuf, 0);
    if (s == NULL)
	s = (char_u *)"";

    STRCPY(IObuff, ":return ");
    STRNCPY(IObuff + 8, s, IOSIZE - 8);
    if (STRLEN(s) + 8 >= IOSIZE)
	STRCPY(IObuff + IOSIZE - 4, "...");
    vim_free(tofree);
    return vim_strsave(IObuff);
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
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;
    char_u	*retval;
    garray_T	*gap;  /* growarray with function lines */

    /* If breakpoints have been added/deleted need to check for it. */
    if (fcp->dbg_tick != debug_tick)
    {
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name,
							       sourcing_lnum);
	fcp->dbg_tick = debug_tick;
    }
#ifdef FEAT_PROFILE
    if (do_profiling == PROF_YES)
	func_line_end(cookie);
#endif

    gap = &fp->uf_lines;
    if (((fp->uf_flags & FC_ABORT) && did_emsg && !aborted_in_try())
	    || fcp->returned)
	retval = NULL;
    else
    {
	/* Skip NULL lines (continuation lines). */
	while (fcp->linenr < gap->ga_len
			  && ((char_u **)(gap->ga_data))[fcp->linenr] == NULL)
	    ++fcp->linenr;
	if (fcp->linenr >= gap->ga_len)
	    retval = NULL;
	else
	{
	    retval = vim_strsave(((char_u **)(gap->ga_data))[fcp->linenr++]);
	    sourcing_lnum = fcp->linenr;
#ifdef FEAT_PROFILE
	    if (do_profiling == PROF_YES)
		func_line_start(cookie);
#endif
	}
    }

    /* Did we encounter a breakpoint? */
    if (fcp->breakpoint != 0 && fcp->breakpoint <= sourcing_lnum)
    {
	dbg_breakpoint(fp->uf_name, sourcing_lnum);
	/* Find next breakpoint. */
	fcp->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name,
							       sourcing_lnum);
	fcp->dbg_tick = debug_tick;
    }

    return retval;
}

#if defined(FEAT_PROFILE) || defined(PROTO)
/*
 * Called when starting to read a function line.
 * "sourcing_lnum" must be correct!
 * When skipping lines it may not actually be executed, but we won't find out
 * until later and we need to store the time now.
 */
    void
func_line_start(cookie)
    void    *cookie;
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;

    if (fp->uf_profiling && sourcing_lnum >= 1
				      && sourcing_lnum <= fp->uf_lines.ga_len)
    {
	fp->uf_tml_idx = sourcing_lnum - 1;
	/* Skip continuation lines. */
	while (fp->uf_tml_idx > 0 && FUNCLINE(fp, fp->uf_tml_idx) == NULL)
	    --fp->uf_tml_idx;
	fp->uf_tml_execed = FALSE;
	profile_start(&fp->uf_tml_start);
	profile_zero(&fp->uf_tml_children);
	profile_get_wait(&fp->uf_tml_wait);
    }
}

/*
 * Called when actually executing a function line.
 */
    void
func_line_exec(cookie)
    void    *cookie;
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;

    if (fp->uf_profiling && fp->uf_tml_idx >= 0)
	fp->uf_tml_execed = TRUE;
}

/*
 * Called when done with a function line.
 */
    void
func_line_end(cookie)
    void    *cookie;
{
    funccall_T	*fcp = (funccall_T *)cookie;
    ufunc_T	*fp = fcp->func;

    if (fp->uf_profiling && fp->uf_tml_idx >= 0)
    {
	if (fp->uf_tml_execed)
	{
	    ++fp->uf_tml_count[fp->uf_tml_idx];
	    profile_end(&fp->uf_tml_start);
	    profile_sub_wait(&fp->uf_tml_wait, &fp->uf_tml_start);
	    profile_add(&fp->uf_tml_total[fp->uf_tml_idx], &fp->uf_tml_start);
	    profile_self(&fp->uf_tml_self[fp->uf_tml_idx], &fp->uf_tml_start,
							&fp->uf_tml_children);
	}
	fp->uf_tml_idx = -1;
    }
}
#endif

/*
 * Return TRUE if the currently active function should be ended, because a
 * return was encountered or an error occurred.  Used inside a ":while".
 */
    int
func_has_ended(cookie)
    void    *cookie;
{
    funccall_T  *fcp = (funccall_T *)cookie;

    /* Ignore the "abort" flag if the abortion behavior has been changed due to
     * an error inside a try conditional. */
    return (((fcp->func->uf_flags & FC_ABORT) && did_emsg && !aborted_in_try())
	    || fcp->returned);
}

/*
 * return TRUE if cookie indicates a function which "abort"s on errors.
 */
    int
func_has_abort(cookie)
    void    *cookie;
{
    return ((funccall_T *)cookie)->func->uf_flags & FC_ABORT;
}

#if defined(FEAT_VIMINFO) || defined(FEAT_SESSION)
typedef enum
{
    VAR_FLAVOUR_DEFAULT,	/* doesn't start with uppercase */
    VAR_FLAVOUR_SESSION,	/* starts with uppercase, some lower */
    VAR_FLAVOUR_VIMINFO		/* all uppercase */
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
    int		type = VAR_NUMBER;
    typval_T	tv;

    if (!writing && (find_viminfo_parameter('!') != NULL))
    {
	tab = vim_strchr(virp->vir_line + 1, '\t');
	if (tab != NULL)
	{
	    *tab++ = '\0';	/* isolate the variable name */
	    if (*tab == 'S')	/* string var */
		type = VAR_STRING;
#ifdef FEAT_FLOAT
	    else if (*tab == 'F')
		type = VAR_FLOAT;
#endif

	    tab = vim_strchr(tab, '\t');
	    if (tab != NULL)
	    {
		tv.v_type = type;
		if (type == VAR_STRING)
		    tv.vval.v_string = viminfo_readstring(virp,
				       (int)(tab - virp->vir_line + 1), TRUE);
#ifdef FEAT_FLOAT
		else if (type == VAR_FLOAT)
		    (void)string2float(tab + 1, &tv.vval.v_float);
#endif
		else
		    tv.vval.v_number = atol((char *)tab + 1);
		set_var(virp->vir_line + 1, &tv, FALSE);
		if (type == VAR_STRING)
		    vim_free(tv.vval.v_string);
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
    hashitem_T	*hi;
    dictitem_T	*this_var;
    int		todo;
    char	*s;
    char_u	*p;
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];

    if (find_viminfo_parameter('!') == NULL)
	return;

    fprintf(fp, _("\n# global variables:\n"));

    todo = (int)globvarht.ht_used;
    for (hi = globvarht.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    this_var = HI2DI(hi);
	    if (var_flavour(this_var->di_key) == VAR_FLAVOUR_VIMINFO)
	    {
		switch (this_var->di_tv.v_type)
		{
		    case VAR_STRING: s = "STR"; break;
		    case VAR_NUMBER: s = "NUM"; break;
#ifdef FEAT_FLOAT
		    case VAR_FLOAT: s = "FLO"; break;
#endif
		    default: continue;
		}
		fprintf(fp, "!%s\t%s\t", this_var->di_key, s);
		p = echo_string(&this_var->di_tv, &tofree, numbuf, 0);
		if (p != NULL)
		    viminfo_writestring(fp, p);
		vim_free(tofree);
	    }
	}
    }
}
#endif

#if defined(FEAT_SESSION) || defined(PROTO)
    int
store_session_globals(fd)
    FILE	*fd;
{
    hashitem_T	*hi;
    dictitem_T	*this_var;
    int		todo;
    char_u	*p, *t;

    todo = (int)globvarht.ht_used;
    for (hi = globvarht.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    this_var = HI2DI(hi);
	    if ((this_var->di_tv.v_type == VAR_NUMBER
			|| this_var->di_tv.v_type == VAR_STRING)
		    && var_flavour(this_var->di_key) == VAR_FLAVOUR_SESSION)
	    {
		/* Escape special characters with a backslash.  Turn a LF and
		 * CR into \n and \r. */
		p = vim_strsave_escaped(get_tv_string(&this_var->di_tv),
							(char_u *)"\\\"\n\r");
		if (p == NULL)	    /* out of memory */
		    break;
		for (t = p; *t != NUL; ++t)
		    if (*t == '\n')
			*t = 'n';
		    else if (*t == '\r')
			*t = 'r';
		if ((fprintf(fd, "let %s = %c%s%c",
				this_var->di_key,
				(this_var->di_tv.v_type == VAR_STRING) ? '"'
									: ' ',
				p,
				(this_var->di_tv.v_type == VAR_STRING) ? '"'
								   : ' ') < 0)
			|| put_eol(fd) == FAIL)
		{
		    vim_free(p);
		    return FAIL;
		}
		vim_free(p);
	    }
#ifdef FEAT_FLOAT
	    else if (this_var->di_tv.v_type == VAR_FLOAT
		    && var_flavour(this_var->di_key) == VAR_FLAVOUR_SESSION)
	    {
		float_T f = this_var->di_tv.vval.v_float;
		int sign = ' ';

		if (f < 0)
		{
		    f = -f;
		    sign = '-';
		}
		if ((fprintf(fd, "let %s = %c&%f",
					       this_var->di_key, sign, f) < 0)
			|| put_eol(fd) == FAIL)
		    return FAIL;
	    }
#endif
	}
    }
    return OK;
}
#endif

/*
 * Display script name where an item was last set.
 * Should only be invoked when 'verbose' is non-zero.
 */
    void
last_set_msg(scriptID)
    scid_T scriptID;
{
    char_u *p;

    if (scriptID != 0)
    {
	p = home_replace_save(NULL, get_scriptname(scriptID));
	if (p != NULL)
	{
	    verbose_enter();
	    MSG_PUTS(_("\n\tLast set from "));
	    MSG_PUTS(p);
	    vim_free(p);
	    verbose_leave();
	}
    }
}

/*
 * List v:oldfiles in a nice way.
 */
/*ARGSUSED*/
    void
ex_oldfiles(eap)
    exarg_T	*eap;
{
    list_T	*l = vimvars[VV_OLDFILES].vv_list;
    listitem_T	*li;
    int		nr = 0;

    if (l == NULL)
	msg((char_u *)_("No old files"));
    else
    {
	msg_start();
	msg_scroll = TRUE;
	for (li = l->lv_first; li != NULL && !got_int; li = li->li_next)
	{
	    msg_outnum((long)++nr);
	    MSG_PUTS(": ");
	    msg_outtrans(get_tv_string(&li->li_tv));
	    msg_putchar('\n');
	    out_flush();	    /* output one line at a time */
	    ui_breakcheck();
	}
	/* Assume "got_int" was set to truncate the listing. */
	got_int = FALSE;

#ifdef FEAT_BROWSE_CMD
	if (cmdmod.browse)
	{
	    quit_more = FALSE;
	    nr = prompt_for_number(FALSE);
	    msg_starthere();
	    if (nr > 0)
	    {
		char_u *p = list_find_str(get_vim_var_list(VV_OLDFILES),
								    (long)nr);

		if (p != NULL)
		{
		    p = expand_env_save(p);
		    eap->arg = p;
		    eap->cmdidx = CMD_edit;
		    cmdmod.browse = FALSE;
		    do_exedit(eap, NULL);
		    vim_free(p);
		}
	    }
	}
#endif
    }
}

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
 * Get the short path (8.3) for the filename in "fnamep".
 * Only works for a valid file name.
 * When the path gets longer "fnamep" is changed and the allocated buffer
 * is put in "bufp".
 * *fnamelen is the length of "fnamep" and set to 0 for a nonexistent path.
 * Returns OK on success, FAIL on failure.
 */
    static int
get_short_pathname(fnamep, bufp, fnamelen)
    char_u	**fnamep;
    char_u	**bufp;
    int		*fnamelen;
{
    int		l, len;
    char_u	*newbuf;

    len = *fnamelen;
    l = GetShortPathName(*fnamep, *fnamep, len);
    if (l > len - 1)
    {
	/* If that doesn't work (not enough space), then save the string
	 * and try again with a new buffer big enough. */
	newbuf = vim_strnsave(*fnamep, l);
	if (newbuf == NULL)
	    return FAIL;

	vim_free(*bufp);
	*fnamep = *bufp = newbuf;

	/* Really should always succeed, as the buffer is big enough. */
	l = GetShortPathName(*fnamep, *fnamep, l+1);
    }

    *fnamelen = l;
    return OK;
}

/*
 * Get the short path (8.3) for the filename in "fname". The converted
 * path is returned in "bufp".
 *
 * Some of the directories specified in "fname" may not exist. This function
 * will shorten the existing directories at the beginning of the path and then
 * append the remaining non-existing path.
 *
 * fname - Pointer to the filename to shorten.  On return, contains the
 *         pointer to the shortened pathname
 * bufp -  Pointer to an allocated buffer for the filename.
 * fnamelen - Length of the filename pointed to by fname
 *
 * Returns OK on success (or nothing done) and FAIL on failure (out of memory).
 */
    static int
shortpath_for_invalid_fname(fname, bufp, fnamelen)
    char_u	**fname;
    char_u	**bufp;
    int		*fnamelen;
{
    char_u	*short_fname, *save_fname, *pbuf_unused;
    char_u	*endp, *save_endp;
    char_u	ch;
    int		old_len, len;
    int		new_len, sfx_len;
    int		retval = OK;

    /* Make a copy */
    old_len = *fnamelen;
    save_fname = vim_strnsave(*fname, old_len);
    pbuf_unused = NULL;
    short_fname = NULL;

    endp = save_fname + old_len - 1; /* Find the end of the copy */
    save_endp = endp;

    /*
     * Try shortening the supplied path till it succeeds by removing one
     * directory at a time from the tail of the path.
     */
    len = 0;
    for (;;)
    {
	/* go back one path-separator */
	while (endp > save_fname && !after_pathsep(save_fname, endp + 1))
	    --endp;
	if (endp <= save_fname)
	    break;		/* processed the complete path */

	/*
	 * Replace the path separator with a NUL and try to shorten the
	 * resulting path.
	 */
	ch = *endp;
	*endp = 0;
	short_fname = save_fname;
	len = (int)STRLEN(short_fname) + 1;
	if (get_short_pathname(&short_fname, &pbuf_unused, &len) == FAIL)
	{
	    retval = FAIL;
	    goto theend;
	}
	*endp = ch;	/* preserve the string */

	if (len > 0)
	    break;	/* successfully shortened the path */

	/* failed to shorten the path. Skip the path separator */
	--endp;
    }

    if (len > 0)
    {
	/*
	 * Succeeded in shortening the path. Now concatenate the shortened
	 * path with the remaining path at the tail.
	 */

	/* Compute the length of the new path. */
	sfx_len = (int)(save_endp - endp) + 1;
	new_len = len + sfx_len;

	*fnamelen = new_len;
	vim_free(*bufp);
	if (new_len > old_len)
	{
	    /* There is not enough space in the currently allocated string,
	     * copy it to a buffer big enough. */
	    *fname = *bufp = vim_strnsave(short_fname, new_len);
	    if (*fname == NULL)
	    {
		retval = FAIL;
		goto theend;
	    }
	}
	else
	{
	    /* Transfer short_fname to the main buffer (it's big enough),
	     * unless get_short_pathname() did its work in-place. */
	    *fname = *bufp = save_fname;
	    if (short_fname != save_fname)
		vim_strncpy(save_fname, short_fname, len);
	    save_fname = NULL;
	}

	/* concat the not-shortened part of the path */
	vim_strncpy(*fname + len, endp, sfx_len);
	(*fname)[new_len] = NUL;
    }

theend:
    vim_free(pbuf_unused);
    vim_free(save_fname);

    return retval;
}

/*
 * Get a pathname for a partial path.
 * Returns OK for success, FAIL for failure.
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

    /* Count up the path separators from the RHS.. so we know which part
     * of the path to return. */
    sepcount = 0;
    for (p = *fnamep; p < *fnamep + *fnamelen; mb_ptr_adv(p))
	if (vim_ispathsep(*p))
	    ++sepcount;

    /* Need full path first (use expand_env() to remove a "~/") */
    hasTilde = (**fnamep == '~');
    if (hasTilde)
	pbuf = tfname = expand_env_save(*fnamep);
    else
	pbuf = tfname = FullName_save(*fnamep, FALSE);

    len = tflen = (int)STRLEN(tfname);

    if (get_short_pathname(&tfname, &pbuf, &len) == FAIL)
	return FAIL;

    if (len == 0)
    {
	/* Don't have a valid filename, so shorten the rest of the
	 * path if we can. This CAN give us invalid 8.3 filenames, but
	 * there's not a lot of point in guessing what it might be.
	 */
	len = tflen;
	if (shortpath_for_invalid_fname(&tfname, &pbuf, &len) == FAIL)
	    return FAIL;
    }

    /* Count the paths backward to find the beginning of the desired string. */
    for (p = tfname + len - 1; p >= tfname; --p)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    p -= mb_head_off(tfname, p);
#endif
	if (vim_ispathsep(*p))
	{
	    if (sepcount == 0 || (hasTilde && sepcount == 1))
		break;
	    else
		sepcount --;
	}
    }
    if (hasTilde)
    {
	--p;
	if (p >= tfname)
	    *p = '~';
	else
	    return FAIL;
    }
    else
	++p;

    /* Copy in the string - p indexes into tfname - allocated at pbuf */
    vim_free(*bufp);
    *fnamelen = (int)STRLEN(p);
    *bufp = pbuf;
    *fnamep = p;

    return OK;
}
#endif /* WIN3264 */

/*
 * Adjust a filename, according to a string of modifiers.
 * *fnamep must be NUL terminated when called.  When returning, the length is
 * determined by *fnamelen.
 * Returns VALID_ flags or -1 for failure.
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
	for (p = *fnamep; *p != NUL; mb_ptr_adv(p))
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
	while (tail > s && after_pathsep(s, tail))
	    mb_ptr_back(*fnamep, tail);
	*fnamelen = (int)(tail - *fnamep);
#ifdef VMS
	if (*fnamelen > 0)
	    *fnamelen += 1; /* the path separator is part of the path */
#endif
	if (*fnamelen == 0)
	{
	    /* Result is empty.  Turn it into "." to make ":cd %:h" work. */
	    p = vim_strsave((char_u *)".");
	    if (p == NULL)
		return -1;
	    vim_free(*bufp);
	    *bufp = *fnamep = tail = p;
	    *fnamelen = 1;
	}
	else
	{
	    while (tail > s && !after_pathsep(s, tail))
		mb_ptr_back(*fnamep, tail);
	}
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
	    if (shortpath_for_partial(fnamep, bufp, fnamelen) == FAIL)
		return -1;
	}
	else
	{
	    int		l;

	    /* Simple case, already have the full-name
	     * Nearly always shorter, so try first time. */
	    l = *fnamelen;
	    if (get_short_pathname(fnamep, bufp, &l) == FAIL)
		return -1;

	    if (l == 0)
	    {
		/* Couldn't find the filename.. search the paths.
		 */
		l = *fnamelen;
		if (shortpath_for_invalid_fname(fnamep, bufp, &l) == FAIL)
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
    p_cpo = empty_option;

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
	    /* avoid getting stuck on a match with an empty string */
	    if (tail == regmatch.endp[0])
	    {
		if (*tail == NUL)
		    break;
		*((char_u *)ga.ga_data + ga.ga_len) = *tail++;
		++ga.ga_len;
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
    if (p_cpo == empty_option)
	p_cpo = save_cpo;
    else
	/* Darn, evaluating {sub} expression changed the value. */
	free_string_option(save_cpo);

    return ret;
}

#endif /* defined(FEAT_MODIFY_FNAME) || defined(FEAT_EVAL) */
