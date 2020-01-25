/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * evalfunc.c: Builtin functions
 */
#define USING_FLOAT_STUFF

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

#ifdef VMS
# include <float.h>
#endif

#if defined(MACOS_X)
# include <time.h>	// for time_t
#endif

#ifdef FEAT_FLOAT
static void f_abs(typval_T *argvars, typval_T *rettv);
static void f_acos(typval_T *argvars, typval_T *rettv);
#endif
static void f_and(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_asin(typval_T *argvars, typval_T *rettv);
static void f_atan(typval_T *argvars, typval_T *rettv);
static void f_atan2(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_BEVAL
static void f_balloon_gettext(typval_T *argvars, typval_T *rettv);
static void f_balloon_show(typval_T *argvars, typval_T *rettv);
# if defined(FEAT_BEVAL_TERM)
static void f_balloon_split(typval_T *argvars, typval_T *rettv);
# endif
#endif
static void f_byte2line(typval_T *argvars, typval_T *rettv);
static void byteidx(typval_T *argvars, typval_T *rettv, int comp);
static void f_byteidx(typval_T *argvars, typval_T *rettv);
static void f_byteidxcomp(typval_T *argvars, typval_T *rettv);
static void f_call(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_ceil(typval_T *argvars, typval_T *rettv);
#endif
static void f_changenr(typval_T *argvars, typval_T *rettv);
static void f_char2nr(typval_T *argvars, typval_T *rettv);
static void f_col(typval_T *argvars, typval_T *rettv);
static void f_confirm(typval_T *argvars, typval_T *rettv);
static void f_copy(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_cos(typval_T *argvars, typval_T *rettv);
static void f_cosh(typval_T *argvars, typval_T *rettv);
#endif
static void f_cursor(typval_T *argsvars, typval_T *rettv);
#ifdef MSWIN
static void f_debugbreak(typval_T *argvars, typval_T *rettv);
#endif
static void f_deepcopy(typval_T *argvars, typval_T *rettv);
static void f_did_filetype(typval_T *argvars, typval_T *rettv);
static void f_empty(typval_T *argvars, typval_T *rettv);
static void f_environ(typval_T *argvars, typval_T *rettv);
static void f_escape(typval_T *argvars, typval_T *rettv);
static void f_eval(typval_T *argvars, typval_T *rettv);
static void f_eventhandler(typval_T *argvars, typval_T *rettv);
static void f_execute(typval_T *argvars, typval_T *rettv);
static void f_exists(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_exp(typval_T *argvars, typval_T *rettv);
#endif
static void f_expand(typval_T *argvars, typval_T *rettv);
static void f_expandcmd(typval_T *argvars, typval_T *rettv);
static void f_feedkeys(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_float2nr(typval_T *argvars, typval_T *rettv);
static void f_floor(typval_T *argvars, typval_T *rettv);
static void f_fmod(typval_T *argvars, typval_T *rettv);
#endif
static void f_fnameescape(typval_T *argvars, typval_T *rettv);
static void f_foreground(typval_T *argvars, typval_T *rettv);
static void f_funcref(typval_T *argvars, typval_T *rettv);
static void f_function(typval_T *argvars, typval_T *rettv);
static void f_garbagecollect(typval_T *argvars, typval_T *rettv);
static void f_get(typval_T *argvars, typval_T *rettv);
static void f_getchangelist(typval_T *argvars, typval_T *rettv);
static void f_getcharsearch(typval_T *argvars, typval_T *rettv);
static void f_getcmdwintype(typval_T *argvars, typval_T *rettv);
static void f_getenv(typval_T *argvars, typval_T *rettv);
static void f_getfontname(typval_T *argvars, typval_T *rettv);
static void f_getjumplist(typval_T *argvars, typval_T *rettv);
static void f_getpid(typval_T *argvars, typval_T *rettv);
static void f_getcurpos(typval_T *argvars, typval_T *rettv);
static void f_getpos(typval_T *argvars, typval_T *rettv);
static void f_getreg(typval_T *argvars, typval_T *rettv);
static void f_getregtype(typval_T *argvars, typval_T *rettv);
static void f_gettagstack(typval_T *argvars, typval_T *rettv);
static void f_has(typval_T *argvars, typval_T *rettv);
static void f_haslocaldir(typval_T *argvars, typval_T *rettv);
static void f_hasmapto(typval_T *argvars, typval_T *rettv);
static void f_hlID(typval_T *argvars, typval_T *rettv);
static void f_hlexists(typval_T *argvars, typval_T *rettv);
static void f_hostname(typval_T *argvars, typval_T *rettv);
static void f_iconv(typval_T *argvars, typval_T *rettv);
static void f_index(typval_T *argvars, typval_T *rettv);
static void f_input(typval_T *argvars, typval_T *rettv);
static void f_inputdialog(typval_T *argvars, typval_T *rettv);
static void f_inputlist(typval_T *argvars, typval_T *rettv);
static void f_inputrestore(typval_T *argvars, typval_T *rettv);
static void f_inputsave(typval_T *argvars, typval_T *rettv);
static void f_inputsecret(typval_T *argvars, typval_T *rettv);
static void f_interrupt(typval_T *argvars, typval_T *rettv);
static void f_invert(typval_T *argvars, typval_T *rettv);
static void f_islocked(typval_T *argvars, typval_T *rettv);
#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
static void f_isinf(typval_T *argvars, typval_T *rettv);
static void f_isnan(typval_T *argvars, typval_T *rettv);
#endif
static void f_last_buffer_nr(typval_T *argvars, typval_T *rettv);
static void f_len(typval_T *argvars, typval_T *rettv);
static void f_libcall(typval_T *argvars, typval_T *rettv);
static void f_libcallnr(typval_T *argvars, typval_T *rettv);
static void f_line(typval_T *argvars, typval_T *rettv);
static void f_line2byte(typval_T *argvars, typval_T *rettv);
static void f_localtime(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_log(typval_T *argvars, typval_T *rettv);
static void f_log10(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_LUA
static void f_luaeval(typval_T *argvars, typval_T *rettv);
#endif
static void f_maparg(typval_T *argvars, typval_T *rettv);
static void f_mapcheck(typval_T *argvars, typval_T *rettv);
static void f_match(typval_T *argvars, typval_T *rettv);
static void f_matchend(typval_T *argvars, typval_T *rettv);
static void f_matchlist(typval_T *argvars, typval_T *rettv);
static void f_matchstr(typval_T *argvars, typval_T *rettv);
static void f_matchstrpos(typval_T *argvars, typval_T *rettv);
static void f_max(typval_T *argvars, typval_T *rettv);
static void f_min(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_MZSCHEME
static void f_mzeval(typval_T *argvars, typval_T *rettv);
#endif
static void f_nextnonblank(typval_T *argvars, typval_T *rettv);
static void f_nr2char(typval_T *argvars, typval_T *rettv);
static void f_or(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_PERL
static void f_perleval(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_FLOAT
static void f_pow(typval_T *argvars, typval_T *rettv);
#endif
static void f_prevnonblank(typval_T *argvars, typval_T *rettv);
static void f_printf(typval_T *argvars, typval_T *rettv);
static void f_pum_getpos(typval_T *argvars, typval_T *rettv);
static void f_pumvisible(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_PYTHON3
static void f_py3eval(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_PYTHON
static void f_pyeval(typval_T *argvars, typval_T *rettv);
#endif
#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
static void f_pyxeval(typval_T *argvars, typval_T *rettv);
#endif
static void f_rand(typval_T *argvars, typval_T *rettv);
static void f_range(typval_T *argvars, typval_T *rettv);
static void f_reg_executing(typval_T *argvars, typval_T *rettv);
static void f_reg_recording(typval_T *argvars, typval_T *rettv);
static void f_reltime(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_reltimefloat(typval_T *argvars, typval_T *rettv);
#endif
static void f_reltimestr(typval_T *argvars, typval_T *rettv);
static void f_remote_expr(typval_T *argvars, typval_T *rettv);
static void f_remote_foreground(typval_T *argvars, typval_T *rettv);
static void f_remote_peek(typval_T *argvars, typval_T *rettv);
static void f_remote_read(typval_T *argvars, typval_T *rettv);
static void f_remote_send(typval_T *argvars, typval_T *rettv);
static void f_remote_startserver(typval_T *argvars, typval_T *rettv);
static void f_rename(typval_T *argvars, typval_T *rettv);
static void f_repeat(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_round(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_RUBY
static void f_rubyeval(typval_T *argvars, typval_T *rettv);
#endif
static void f_screenattr(typval_T *argvars, typval_T *rettv);
static void f_screenchar(typval_T *argvars, typval_T *rettv);
static void f_screenchars(typval_T *argvars, typval_T *rettv);
static void f_screencol(typval_T *argvars, typval_T *rettv);
static void f_screenrow(typval_T *argvars, typval_T *rettv);
static void f_screenstring(typval_T *argvars, typval_T *rettv);
static void f_search(typval_T *argvars, typval_T *rettv);
static void f_searchdecl(typval_T *argvars, typval_T *rettv);
static void f_searchpair(typval_T *argvars, typval_T *rettv);
static void f_searchpairpos(typval_T *argvars, typval_T *rettv);
static void f_searchpos(typval_T *argvars, typval_T *rettv);
static void f_server2client(typval_T *argvars, typval_T *rettv);
static void f_serverlist(typval_T *argvars, typval_T *rettv);
static void f_setcharsearch(typval_T *argvars, typval_T *rettv);
static void f_setenv(typval_T *argvars, typval_T *rettv);
static void f_setfperm(typval_T *argvars, typval_T *rettv);
static void f_setpos(typval_T *argvars, typval_T *rettv);
static void f_setreg(typval_T *argvars, typval_T *rettv);
static void f_settagstack(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_CRYPT
static void f_sha256(typval_T *argvars, typval_T *rettv);
#endif
static void f_shellescape(typval_T *argvars, typval_T *rettv);
static void f_shiftwidth(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_sin(typval_T *argvars, typval_T *rettv);
static void f_sinh(typval_T *argvars, typval_T *rettv);
#endif
static void f_soundfold(typval_T *argvars, typval_T *rettv);
static void f_spellbadword(typval_T *argvars, typval_T *rettv);
static void f_spellsuggest(typval_T *argvars, typval_T *rettv);
static void f_split(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_sqrt(typval_T *argvars, typval_T *rettv);
#endif
static void f_srand(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_str2float(typval_T *argvars, typval_T *rettv);
#endif
static void f_str2list(typval_T *argvars, typval_T *rettv);
static void f_str2nr(typval_T *argvars, typval_T *rettv);
static void f_strchars(typval_T *argvars, typval_T *rettv);
#ifdef HAVE_STRFTIME
static void f_strftime(typval_T *argvars, typval_T *rettv);
#endif
static void f_strgetchar(typval_T *argvars, typval_T *rettv);
static void f_stridx(typval_T *argvars, typval_T *rettv);
static void f_strlen(typval_T *argvars, typval_T *rettv);
static void f_strcharpart(typval_T *argvars, typval_T *rettv);
static void f_strpart(typval_T *argvars, typval_T *rettv);
#ifdef HAVE_STRPTIME
static void f_strptime(typval_T *argvars, typval_T *rettv);
#endif
static void f_strridx(typval_T *argvars, typval_T *rettv);
static void f_strtrans(typval_T *argvars, typval_T *rettv);
static void f_strdisplaywidth(typval_T *argvars, typval_T *rettv);
static void f_strwidth(typval_T *argvars, typval_T *rettv);
static void f_submatch(typval_T *argvars, typval_T *rettv);
static void f_substitute(typval_T *argvars, typval_T *rettv);
static void f_swapinfo(typval_T *argvars, typval_T *rettv);
static void f_swapname(typval_T *argvars, typval_T *rettv);
static void f_synID(typval_T *argvars, typval_T *rettv);
static void f_synIDattr(typval_T *argvars, typval_T *rettv);
static void f_synIDtrans(typval_T *argvars, typval_T *rettv);
static void f_synstack(typval_T *argvars, typval_T *rettv);
static void f_synconcealed(typval_T *argvars, typval_T *rettv);
static void f_tabpagebuflist(typval_T *argvars, typval_T *rettv);
static void f_taglist(typval_T *argvars, typval_T *rettv);
static void f_tagfiles(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_tan(typval_T *argvars, typval_T *rettv);
static void f_tanh(typval_T *argvars, typval_T *rettv);
#endif
static void f_tolower(typval_T *argvars, typval_T *rettv);
static void f_toupper(typval_T *argvars, typval_T *rettv);
static void f_tr(typval_T *argvars, typval_T *rettv);
static void f_trim(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_trunc(typval_T *argvars, typval_T *rettv);
#endif
static void f_type(typval_T *argvars, typval_T *rettv);
static void f_virtcol(typval_T *argvars, typval_T *rettv);
static void f_visualmode(typval_T *argvars, typval_T *rettv);
static void f_wildmenumode(typval_T *argvars, typval_T *rettv);
static void f_windowsversion(typval_T *argvars, typval_T *rettv);
static void f_wordcount(typval_T *argvars, typval_T *rettv);
static void f_xor(typval_T *argvars, typval_T *rettv);

/*
 * Array with names and number of arguments of all internal functions
 * MUST BE KEPT SORTED IN strcmp() ORDER FOR BINARY SEARCH!
 */
typedef struct
{
    char	*f_name;	// function name
    char	f_min_argc;	// minimal number of arguments
    char	f_max_argc;	// maximal number of arguments
    char	f_argtype;	// for method: FEARG_ values
    type_T	*f_rettype;	// return type
    void	(*f_func)(typval_T *args, typval_T *rvar);
				// implementation of function
} funcentry_T;

// values for f_argtype; zero means it cannot be used as a method
#define FEARG_1    1	    // base is the first argument
#define FEARG_2    2	    // base is the second argument
#define FEARG_3    3	    // base is the third argument
#define FEARG_4    4	    // base is the fourth argument
#define FEARG_LAST 9	    // base is the last argument

static funcentry_T global_functions[] =
{
#ifdef FEAT_FLOAT
    {"abs",		1, 1, FEARG_1,	  &t_any,	f_abs},
    {"acos",		1, 1, FEARG_1,	  &t_float,	f_acos},	// WJMc
#endif
    {"add",		2, 2, FEARG_1,	  &t_any,	f_add},
    {"and",		2, 2, FEARG_1,	  &t_number,	f_and},
    {"append",		2, 2, FEARG_LAST, &t_number,	f_append},
    {"appendbufline",	3, 3, FEARG_LAST, &t_number,	f_appendbufline},
    {"argc",		0, 1, 0,	  &t_number,	f_argc},
    {"argidx",		0, 0, 0,	  &t_number,	f_argidx},
    {"arglistid",	0, 2, 0,	  &t_number,	f_arglistid},
    {"argv",		0, 2, 0,	  &t_any,	f_argv},
#ifdef FEAT_FLOAT
    {"asin",		1, 1, FEARG_1,	  &t_float,	f_asin},	// WJMc
#endif
    {"assert_beeps",	1, 2, FEARG_1,	  &t_number,	f_assert_beeps},
    {"assert_equal",	2, 3, FEARG_2,	  &t_number,	f_assert_equal},
    {"assert_equalfile", 2, 2, FEARG_1,	  &t_number,	f_assert_equalfile},
    {"assert_exception", 1, 2, 0,	  &t_number,	f_assert_exception},
    {"assert_fails",	1, 3, FEARG_1,	  &t_number,	f_assert_fails},
    {"assert_false",	1, 2, FEARG_1,	  &t_number,	f_assert_false},
    {"assert_inrange",	3, 4, FEARG_3,	  &t_number,	f_assert_inrange},
    {"assert_match",	2, 3, FEARG_2,	  &t_number,	f_assert_match},
    {"assert_notequal",	2, 3, FEARG_2,	  &t_number,	f_assert_notequal},
    {"assert_notmatch",	2, 3, FEARG_2,	  &t_number,	f_assert_notmatch},
    {"assert_report",	1, 1, FEARG_1,	  &t_number,	f_assert_report},
    {"assert_true",	1, 2, FEARG_1,	  &t_number,	f_assert_true},
#ifdef FEAT_FLOAT
    {"atan",		1, 1, FEARG_1,	  &t_float,	f_atan},
    {"atan2",		2, 2, FEARG_1,	  &t_float,	f_atan2},
#endif
#ifdef FEAT_BEVAL
    {"balloon_gettext",	0, 0, 0,	  &t_string,	f_balloon_gettext},
    {"balloon_show",	1, 1, FEARG_1,	  &t_void,	f_balloon_show},
# if defined(FEAT_BEVAL_TERM)
    {"balloon_split",	1, 1, FEARG_1,	  &t_list_string, f_balloon_split},
# endif
#endif
    {"browse",		4, 4, 0,	  &t_string,	f_browse},
    {"browsedir",	2, 2, 0,	  &t_string,	f_browsedir},
    {"bufadd",		1, 1, FEARG_1,	  &t_number,	f_bufadd},
    {"bufexists",	1, 1, FEARG_1,	  &t_number,	f_bufexists},
    {"buffer_exists",	1, 1, FEARG_1,	  &t_number,	f_bufexists},	// obsolete
    {"buffer_name",	0, 1, FEARG_1,	  &t_string,	f_bufname},	// obsolete
    {"buffer_number",	0, 1, FEARG_1,	  &t_number,	f_bufnr},	// obsolete
    {"buflisted",	1, 1, FEARG_1,	  &t_number,	f_buflisted},
    {"bufload",		1, 1, FEARG_1,	  &t_void,	f_bufload},
    {"bufloaded",	1, 1, FEARG_1,	  &t_number,	f_bufloaded},
    {"bufname",		0, 1, FEARG_1,	  &t_string,	f_bufname},
    {"bufnr",		0, 2, FEARG_1,	  &t_number,	f_bufnr},
    {"bufwinid",	1, 1, FEARG_1,	  &t_number,	f_bufwinid},
    {"bufwinnr",	1, 1, FEARG_1,	  &t_number,	f_bufwinnr},
    {"byte2line",	1, 1, FEARG_1,	  &t_number,	f_byte2line},
    {"byteidx",		2, 2, FEARG_1,	  &t_number,	f_byteidx},
    {"byteidxcomp",	2, 2, FEARG_1,	  &t_number,	f_byteidxcomp},
    {"call",		2, 3, FEARG_1,	  &t_any,	f_call},
#ifdef FEAT_FLOAT
    {"ceil",		1, 1, FEARG_1,	  &t_float,	f_ceil},
#endif
#ifdef FEAT_JOB_CHANNEL
    {"ch_canread",	1, 1, FEARG_1,	  &t_number,	f_ch_canread},
    {"ch_close",	1, 1, FEARG_1,	  &t_void,	f_ch_close},
    {"ch_close_in",	1, 1, FEARG_1,	  &t_void,	f_ch_close_in},
    {"ch_evalexpr",	2, 3, FEARG_1,	  &t_any,	f_ch_evalexpr},
    {"ch_evalraw",	2, 3, FEARG_1,	  &t_any,	f_ch_evalraw},
    {"ch_getbufnr",	2, 2, FEARG_1,	  &t_number,	f_ch_getbufnr},
    {"ch_getjob",	1, 1, FEARG_1,	  &t_job,	f_ch_getjob},
    {"ch_info",		1, 1, FEARG_1,	  &t_dict_any,	f_ch_info},
    {"ch_log",		1, 2, FEARG_1,	  &t_void,	f_ch_log},
    {"ch_logfile",	1, 2, FEARG_1,	  &t_void,	f_ch_logfile},
    {"ch_open",		1, 2, FEARG_1,	  &t_channel,	f_ch_open},
    {"ch_read",		1, 2, FEARG_1,	  &t_string,	f_ch_read},
    {"ch_readblob",	1, 2, FEARG_1,	  &t_blob,	f_ch_readblob},
    {"ch_readraw",	1, 2, FEARG_1,	  &t_string,	f_ch_readraw},
    {"ch_sendexpr",	2, 3, FEARG_1,	  &t_void,	f_ch_sendexpr},
    {"ch_sendraw",	2, 3, FEARG_1,	  &t_void,	f_ch_sendraw},
    {"ch_setoptions",	2, 2, FEARG_1,	  &t_void,	f_ch_setoptions},
    {"ch_status",	1, 2, FEARG_1,	  &t_string,	f_ch_status},
#endif
    {"changenr",	0, 0, 0,	  &t_number,	f_changenr},
    {"char2nr",		1, 2, FEARG_1,	  &t_number,	f_char2nr},
    {"chdir",		1, 1, FEARG_1,	  &t_string,	f_chdir},
    {"cindent",		1, 1, FEARG_1,	  &t_number,	f_cindent},
    {"clearmatches",	0, 1, FEARG_1,	  &t_void,	f_clearmatches},
    {"col",		1, 1, FEARG_1,	  &t_number,	f_col},
    {"complete",	2, 2, FEARG_2,	  &t_void,	f_complete},
    {"complete_add",	1, 1, FEARG_1,	  &t_number,	f_complete_add},
    {"complete_check",	0, 0, 0,	  &t_number,	f_complete_check},
    {"complete_info",	0, 1, FEARG_1,	  &t_dict_any,	f_complete_info},
    {"confirm",		1, 4, FEARG_1,	  &t_number,	f_confirm},
    {"copy",		1, 1, FEARG_1,	  &t_any,	f_copy},
#ifdef FEAT_FLOAT
    {"cos",		1, 1, FEARG_1,	  &t_float,	f_cos},
    {"cosh",		1, 1, FEARG_1,	  &t_float,	f_cosh},
#endif
    {"count",		2, 4, FEARG_1,	  &t_number,	f_count},
    {"cscope_connection",0,3, 0,	  &t_number,	f_cscope_connection},
    {"cursor",		1, 3, FEARG_1,	  &t_number,	f_cursor},
#ifdef MSWIN
    {"debugbreak",	1, 1, FEARG_1,	  &t_number,	f_debugbreak},
#endif
    {"deepcopy",	1, 2, FEARG_1,	  &t_any,	f_deepcopy},
    {"delete",		1, 2, FEARG_1,	  &t_number,	f_delete},
    {"deletebufline",	2, 3, FEARG_1,	  &t_number,	f_deletebufline},
    {"did_filetype",	0, 0, 0,	  &t_number,	f_did_filetype},
    {"diff_filler",	1, 1, FEARG_1,	  &t_number,	f_diff_filler},
    {"diff_hlID",	2, 2, FEARG_1,	  &t_number,	f_diff_hlID},
    {"empty",		1, 1, FEARG_1,	  &t_number,	f_empty},
    {"environ",		0, 0, 0,	  &t_dict_string, f_environ},
    {"escape",		2, 2, FEARG_1,	  &t_string,	f_escape},
    {"eval",		1, 1, FEARG_1,	  &t_any,	f_eval},
    {"eventhandler",	0, 0, 0,	  &t_number,	f_eventhandler},
    {"executable",	1, 1, FEARG_1,	  &t_number,	f_executable},
    {"execute",		1, 2, FEARG_1,	  &t_string,	f_execute},
    {"exepath",		1, 1, FEARG_1,	  &t_string,	f_exepath},
    {"exists",		1, 1, FEARG_1,	  &t_number,	f_exists},
#ifdef FEAT_FLOAT
    {"exp",		1, 1, FEARG_1,	  &t_float,	f_exp},
#endif
    {"expand",		1, 3, FEARG_1,	  &t_any,	f_expand},
    {"expandcmd",	1, 1, FEARG_1,	  &t_string,	f_expandcmd},
    {"extend",		2, 3, FEARG_1,	  &t_any,	f_extend},
    {"feedkeys",	1, 2, FEARG_1,	  &t_void,	f_feedkeys},
    {"file_readable",	1, 1, FEARG_1,	  &t_number,	f_filereadable}, // obsolete
    {"filereadable",	1, 1, FEARG_1,	  &t_number,	f_filereadable},
    {"filewritable",	1, 1, FEARG_1,	  &t_number,	f_filewritable},
    {"filter",		2, 2, FEARG_1,	  &t_any,	f_filter},
    {"finddir",		1, 3, FEARG_1,	  &t_string,	f_finddir},
    {"findfile",	1, 3, FEARG_1,	  &t_string,	f_findfile},
#ifdef FEAT_FLOAT
    {"float2nr",	1, 1, FEARG_1,	  &t_number,	f_float2nr},
    {"floor",		1, 1, FEARG_1,	  &t_float,	f_floor},
    {"fmod",		2, 2, FEARG_1,	  &t_float,	f_fmod},
#endif
    {"fnameescape",	1, 1, FEARG_1,	  &t_string,	f_fnameescape},
    {"fnamemodify",	2, 2, FEARG_1,	  &t_string,	f_fnamemodify},
    {"foldclosed",	1, 1, FEARG_1,	  &t_number,	f_foldclosed},
    {"foldclosedend",	1, 1, FEARG_1,	  &t_number,	f_foldclosedend},
    {"foldlevel",	1, 1, FEARG_1,	  &t_number,	f_foldlevel},
    {"foldtext",	0, 0, 0,	  &t_string,	f_foldtext},
    {"foldtextresult",	1, 1, FEARG_1,	  &t_string,	f_foldtextresult},
    {"foreground",	0, 0, 0,	  &t_void,	f_foreground},
    {"funcref",		1, 3, FEARG_1,	  &t_any,	f_funcref},
    {"function",	1, 3, FEARG_1,	  &t_any,	f_function},
    {"garbagecollect",	0, 1, 0,	  &t_void,	f_garbagecollect},
    {"get",		2, 3, FEARG_1,	  &t_any,	f_get},
    {"getbufinfo",	0, 1, 0,	  &t_list_dict_any, f_getbufinfo},
    {"getbufline",	2, 3, FEARG_1,	  &t_list_string, f_getbufline},
    {"getbufvar",	2, 3, FEARG_1,	  &t_any,	f_getbufvar},
    {"getchangelist",	0, 1, FEARG_1,	  &t_list_any,	f_getchangelist},
    {"getchar",		0, 1, 0,	  &t_number,	f_getchar},
    {"getcharmod",	0, 0, 0,	  &t_number,	f_getcharmod},
    {"getcharsearch",	0, 0, 0,	  &t_dict_any,	f_getcharsearch},
    {"getcmdline",	0, 0, 0,	  &t_string,	f_getcmdline},
    {"getcmdpos",	0, 0, 0,	  &t_number,	f_getcmdpos},
    {"getcmdtype",	0, 0, 0,	  &t_string,	f_getcmdtype},
    {"getcmdwintype",	0, 0, 0,	  &t_string,	f_getcmdwintype},
    {"getcompletion",	2, 3, FEARG_1,	  &t_list_string, f_getcompletion},
    {"getcurpos",	0, 0, 0,	  &t_list_number, f_getcurpos},
    {"getcwd",		0, 2, FEARG_1,	  &t_string,	f_getcwd},
    {"getenv",		1, 1, FEARG_1,	  &t_string,	f_getenv},
    {"getfontname",	0, 1, 0,	  &t_string,	f_getfontname},
    {"getfperm",	1, 1, FEARG_1,	  &t_string,	f_getfperm},
    {"getfsize",	1, 1, FEARG_1,	  &t_number,	f_getfsize},
    {"getftime",	1, 1, FEARG_1,	  &t_number,	f_getftime},
    {"getftype",	1, 1, FEARG_1,	  &t_string,	f_getftype},
    {"getimstatus",	0, 0, 0,	  &t_number,	f_getimstatus},
    {"getjumplist",	0, 2, FEARG_1,	  &t_list_any,	f_getjumplist},
    {"getline",		1, 2, FEARG_1,	  &t_string,	f_getline},
    {"getloclist",	1, 2, 0,	  &t_list_dict_any, f_getloclist},
    {"getmatches",	0, 1, 0,	  &t_list_dict_any, f_getmatches},
    {"getmousepos",	0, 0, 0,	  &t_dict_number, f_getmousepos},
    {"getpid",		0, 0, 0,	  &t_number,	f_getpid},
    {"getpos",		1, 1, FEARG_1,	  &t_list_number,	f_getpos},
    {"getqflist",	0, 1, 0,	  &t_list_dict_any,	f_getqflist},
    {"getreg",		0, 3, FEARG_1,	  &t_string,	f_getreg},
    {"getregtype",	0, 1, FEARG_1,	  &t_string,	f_getregtype},
    {"gettabinfo",	0, 1, FEARG_1,	  &t_list_dict_any,	f_gettabinfo},
    {"gettabvar",	2, 3, FEARG_1,	  &t_any,	f_gettabvar},
    {"gettabwinvar",	3, 4, FEARG_1,	  &t_any,	f_gettabwinvar},
    {"gettagstack",	0, 1, FEARG_1,	  &t_dict_any,	f_gettagstack},
    {"getwininfo",	0, 1, FEARG_1,	  &t_list_dict_any,	f_getwininfo},
    {"getwinpos",	0, 1, FEARG_1,	  &t_list_number,	f_getwinpos},
    {"getwinposx",	0, 0, 0,	  &t_number,	f_getwinposx},
    {"getwinposy",	0, 0, 0,	  &t_number,	f_getwinposy},
    {"getwinvar",	2, 3, FEARG_1,	  &t_any,	f_getwinvar},
    {"glob",		1, 4, FEARG_1,	  &t_any,	f_glob},
    {"glob2regpat",	1, 1, FEARG_1,	  &t_string,	f_glob2regpat},
    {"globpath",	2, 5, FEARG_2,	  &t_any,	f_globpath},
    {"has",		1, 1, 0,	  &t_number,	f_has},
    {"has_key",		2, 2, FEARG_1,	  &t_number,	f_has_key},
    {"haslocaldir",	0, 2, FEARG_1,	  &t_number,	f_haslocaldir},
    {"hasmapto",	1, 3, FEARG_1,	  &t_number,	f_hasmapto},
    {"highlightID",	1, 1, FEARG_1,	  &t_number,	f_hlID},	// obsolete
    {"highlight_exists",1, 1, FEARG_1,	  &t_number,	f_hlexists},	// obsolete
    {"histadd",		2, 2, FEARG_2,	  &t_number,	f_histadd},
    {"histdel",		1, 2, FEARG_1,	  &t_number,	f_histdel},
    {"histget",		1, 2, FEARG_1,	  &t_string,	f_histget},
    {"histnr",		1, 1, FEARG_1,	  &t_number,	f_histnr},
    {"hlID",		1, 1, FEARG_1,	  &t_number,	f_hlID},
    {"hlexists",	1, 1, FEARG_1,	  &t_number,	f_hlexists},
    {"hostname",	0, 0, 0,	  &t_string,	f_hostname},
    {"iconv",		3, 3, FEARG_1,	  &t_string,	f_iconv},
    {"indent",		1, 1, FEARG_1,	  &t_number,	f_indent},
    {"index",		2, 4, FEARG_1,	  &t_number,	f_index},
    {"input",		1, 3, FEARG_1,	  &t_string,	f_input},
    {"inputdialog",	1, 3, FEARG_1,	  &t_string,	f_inputdialog},
    {"inputlist",	1, 1, FEARG_1,	  &t_number,	f_inputlist},
    {"inputrestore",	0, 0, 0,	  &t_number,	f_inputrestore},
    {"inputsave",	0, 0, 0,	  &t_number,	f_inputsave},
    {"inputsecret",	1, 2, FEARG_1,	  &t_string,	f_inputsecret},
    {"insert",		2, 3, FEARG_1,	  &t_any,	f_insert},
    {"interrupt",	0, 0, 0,	  &t_void,	f_interrupt},
    {"invert",		1, 1, FEARG_1,	  &t_number,	f_invert},
    {"isdirectory",	1, 1, FEARG_1,	  &t_number,	f_isdirectory},
#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
    {"isinf",		1, 1, FEARG_1,	  &t_number,	f_isinf},
#endif
    {"islocked",	1, 1, FEARG_1,	  &t_number,	f_islocked},
#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
    {"isnan",		1, 1, FEARG_1,	  &t_number,	f_isnan},
#endif
    {"items",		1, 1, FEARG_1,	  &t_list_any,	f_items},
#ifdef FEAT_JOB_CHANNEL
    {"job_getchannel",	1, 1, FEARG_1,	  &t_channel,	f_job_getchannel},
    {"job_info",	0, 1, FEARG_1,	  &t_dict_any,	f_job_info},
    {"job_setoptions",	2, 2, FEARG_1,	  &t_void,	f_job_setoptions},
    {"job_start",	1, 2, FEARG_1,	  &t_job,	f_job_start},
    {"job_status",	1, 1, FEARG_1,	  &t_string,	f_job_status},
    {"job_stop",	1, 2, FEARG_1,	  &t_number,	f_job_stop},
#endif
    {"join",		1, 2, FEARG_1,	  &t_string,	f_join},
    {"js_decode",	1, 1, FEARG_1,	  &t_any,	f_js_decode},
    {"js_encode",	1, 1, FEARG_1,	  &t_string,	f_js_encode},
    {"json_decode",	1, 1, FEARG_1,	  &t_any,	f_json_decode},
    {"json_encode",	1, 1, FEARG_1,	  &t_string,	f_json_encode},
    {"keys",		1, 1, FEARG_1,	  &t_list_any,	f_keys},
    {"last_buffer_nr",	0, 0, 0,	  &t_number,	f_last_buffer_nr}, // obsolete
    {"len",		1, 1, FEARG_1,	  &t_number,	f_len},
    {"libcall",		3, 3, FEARG_3,	  &t_string,	f_libcall},
    {"libcallnr",	3, 3, FEARG_3,	  &t_number,	f_libcallnr},
    {"line",		1, 2, FEARG_1,	  &t_number,	f_line},
    {"line2byte",	1, 1, FEARG_1,	  &t_number,	f_line2byte},
    {"lispindent",	1, 1, FEARG_1,	  &t_number,	f_lispindent},
    {"list2str",	1, 2, FEARG_1,	  &t_string,	f_list2str},
    {"listener_add",	1, 2, FEARG_2,	  &t_number,	f_listener_add},
    {"listener_flush",	0, 1, FEARG_1,	  &t_void,	f_listener_flush},
    {"listener_remove",	1, 1, FEARG_1,	  &t_number,	f_listener_remove},
    {"localtime",	0, 0, 0,	  &t_number,	f_localtime},
#ifdef FEAT_FLOAT
    {"log",		1, 1, FEARG_1,	  &t_float,	f_log},
    {"log10",		1, 1, FEARG_1,	  &t_float,	f_log10},
#endif
#ifdef FEAT_LUA
    {"luaeval",		1, 2, FEARG_1,	  &t_any,	f_luaeval},
#endif
    {"map",		2, 2, FEARG_1,	  &t_any,	f_map},
    {"maparg",		1, 4, FEARG_1,	  &t_string,	f_maparg},
    {"mapcheck",	1, 3, FEARG_1,	  &t_string,	f_mapcheck},
    {"match",		2, 4, FEARG_1,	  &t_any,	f_match},
    {"matchadd",	2, 5, FEARG_1,	  &t_number,	f_matchadd},
    {"matchaddpos",	2, 5, FEARG_1,	  &t_number,	f_matchaddpos},
    {"matcharg",	1, 1, FEARG_1,	  &t_list_string, f_matcharg},
    {"matchdelete",	1, 2, FEARG_1,	  &t_number,	f_matchdelete},
    {"matchend",	2, 4, FEARG_1,	  &t_number,	f_matchend},
    {"matchlist",	2, 4, FEARG_1,	  &t_list_any,	f_matchlist},
    {"matchstr",	2, 4, FEARG_1,	  &t_string,	f_matchstr},
    {"matchstrpos",	2, 4, FEARG_1,	  &t_list_any,	f_matchstrpos},
    {"max",		1, 1, FEARG_1,	  &t_any,	f_max},
    {"min",		1, 1, FEARG_1,	  &t_any,	f_min},
    {"mkdir",		1, 3, FEARG_1,	  &t_number,	f_mkdir},
    {"mode",		0, 1, FEARG_1,	  &t_string,	f_mode},
#ifdef FEAT_MZSCHEME
    {"mzeval",		1, 1, FEARG_1,	  &t_any,	f_mzeval},
#endif
    {"nextnonblank",	1, 1, FEARG_1,	  &t_number,	f_nextnonblank},
    {"nr2char",		1, 2, FEARG_1,	  &t_string,	f_nr2char},
    {"or",		2, 2, FEARG_1,	  &t_number,	f_or},
    {"pathshorten",	1, 1, FEARG_1,	  &t_string,	f_pathshorten},
#ifdef FEAT_PERL
    {"perleval",	1, 1, FEARG_1,	  &t_any,	f_perleval},
#endif
#ifdef FEAT_PROP_POPUP
    {"popup_atcursor",	2, 2, FEARG_1,	  &t_number,	f_popup_atcursor},
    {"popup_beval",	2, 2, FEARG_1,	  &t_number,	f_popup_beval},
    {"popup_clear",	0, 0, 0,	  &t_void,	f_popup_clear},
    {"popup_close",	1, 2, FEARG_1,	  &t_void,	f_popup_close},
    {"popup_create",	2, 2, FEARG_1,	  &t_number,	f_popup_create},
    {"popup_dialog",	2, 2, FEARG_1,	  &t_number,	f_popup_dialog},
    {"popup_filter_menu", 2, 2, 0,	  &t_number,	f_popup_filter_menu},
    {"popup_filter_yesno", 2, 2, 0,	  &t_number,	f_popup_filter_yesno},
    {"popup_findinfo",	0, 0, 0,	  &t_number,	f_popup_findinfo},
    {"popup_findpreview", 0, 0, 0,	  &t_number,	f_popup_findpreview},
    {"popup_getoptions", 1, 1, FEARG_1,	  &t_dict_any,	f_popup_getoptions},
    {"popup_getpos",	1, 1, FEARG_1,	  &t_dict_any,	f_popup_getpos},
    {"popup_hide",	1, 1, FEARG_1,	  &t_void,	f_popup_hide},
    {"popup_locate",	2, 2, 0,	  &t_number,	f_popup_locate},
    {"popup_menu",	2, 2, FEARG_1,	  &t_number,	f_popup_menu},
    {"popup_move",	2, 2, FEARG_1,	  &t_void,	f_popup_move},
    {"popup_notification", 2, 2, FEARG_1, &t_number,	f_popup_notification},
    {"popup_setoptions", 2, 2, FEARG_1,	  &t_void,	f_popup_setoptions},
    {"popup_settext",	2, 2, FEARG_1,	  &t_void,	f_popup_settext},
    {"popup_show",	1, 1, FEARG_1,	  &t_void,	f_popup_show},
#endif
#ifdef FEAT_FLOAT
    {"pow",		2, 2, FEARG_1,	  &t_float,	f_pow},
#endif
    {"prevnonblank",	1, 1, FEARG_1,	  &t_number,	f_prevnonblank},
    {"printf",		1, 19, FEARG_2,	  &t_string,	f_printf},
#ifdef FEAT_JOB_CHANNEL
    {"prompt_setcallback", 2, 2, FEARG_1, &t_void,	 f_prompt_setcallback},
    {"prompt_setinterrupt", 2, 2, FEARG_1,&t_void,	 f_prompt_setinterrupt},
    {"prompt_setprompt", 2, 2, FEARG_1,	  &t_void,	 f_prompt_setprompt},
#endif
#ifdef FEAT_PROP_POPUP
    {"prop_add",	3, 3, FEARG_1,	  &t_void,	f_prop_add},
    {"prop_clear",	1, 3, FEARG_1,	  &t_void,	f_prop_clear},
    {"prop_find",	1, 2, FEARG_1,	  &t_dict_any,	f_prop_find},
    {"prop_list",	1, 2, FEARG_1,	  &t_list_any,	f_prop_list},
    {"prop_remove",	1, 3, FEARG_1,	  &t_number,	f_prop_remove},
    {"prop_type_add",	2, 2, FEARG_1,	  &t_void,	f_prop_type_add},
    {"prop_type_change", 2, 2, FEARG_1,	  &t_void,	f_prop_type_change},
    {"prop_type_delete", 1, 2, FEARG_1,	  &t_void,	f_prop_type_delete},
    {"prop_type_get",	1, 2, FEARG_1,	  &t_dict_any,	f_prop_type_get},
    {"prop_type_list",	0, 1, FEARG_1,	  &t_list_string, f_prop_type_list},
#endif
    {"pum_getpos",	0, 0, 0,	  &t_dict_number, f_pum_getpos},
    {"pumvisible",	0, 0, 0,	  &t_number,	f_pumvisible},
#ifdef FEAT_PYTHON3
    {"py3eval",		1, 1, FEARG_1,	  &t_any,	f_py3eval},
#endif
#ifdef FEAT_PYTHON
    {"pyeval",		1, 1, FEARG_1,	  &t_any,	f_pyeval},
#endif
#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
    {"pyxeval",		1, 1, FEARG_1,	  &t_any,	f_pyxeval},
#endif
    {"rand",		0, 1, FEARG_1,	  &t_number,	f_rand},
    {"range",		1, 3, FEARG_1,	  &t_list_number, f_range},
    {"readdir",		1, 2, FEARG_1,	  &t_list_string, f_readdir},
    {"readfile",	1, 3, FEARG_1,	  &t_any,	f_readfile},
    {"reg_executing",	0, 0, 0,	  &t_string,	f_reg_executing},
    {"reg_recording",	0, 0, 0,	  &t_string,	f_reg_recording},
    {"reltime",		0, 2, FEARG_1,	  &t_list_any,	f_reltime},
#ifdef FEAT_FLOAT
    {"reltimefloat",	1, 1, FEARG_1,	  &t_float,	f_reltimefloat},
#endif
    {"reltimestr",	1, 1, FEARG_1,	  &t_string,	f_reltimestr},
    {"remote_expr",	2, 4, FEARG_1,	  &t_string,	f_remote_expr},
    {"remote_foreground", 1, 1, FEARG_1,  &t_string,	f_remote_foreground},
    {"remote_peek",	1, 2, FEARG_1,	  &t_number,	f_remote_peek},
    {"remote_read",	1, 2, FEARG_1,	  &t_string,	f_remote_read},
    {"remote_send",	2, 3, FEARG_1,	  &t_string,	f_remote_send},
    {"remote_startserver", 1, 1, FEARG_1, &t_void,	 f_remote_startserver},
    {"remove",		2, 3, FEARG_1,	  &t_any,	f_remove},
    {"rename",		2, 2, FEARG_1,	  &t_number,	f_rename},
    {"repeat",		2, 2, FEARG_1,	  &t_any,	f_repeat},
    {"resolve",		1, 1, FEARG_1,	  &t_string,	f_resolve},
    {"reverse",		1, 1, FEARG_1,	  &t_any,	f_reverse},
#ifdef FEAT_FLOAT
    {"round",		1, 1, FEARG_1,	  &t_float,	f_round},
#endif
#ifdef FEAT_RUBY
    {"rubyeval",	1, 1, FEARG_1,	  &t_any,	f_rubyeval},
#endif
    {"screenattr",	2, 2, FEARG_1,	  &t_number,	f_screenattr},
    {"screenchar",	2, 2, FEARG_1,	  &t_number,	f_screenchar},
    {"screenchars",	2, 2, FEARG_1,	  &t_list_number, f_screenchars},
    {"screencol",	0, 0, 0,	  &t_number,	f_screencol},
    {"screenpos",	3, 3, FEARG_1,	  &t_dict_number, f_screenpos},
    {"screenrow",	0, 0, 0,	  &t_number,	f_screenrow},
    {"screenstring",	2, 2, FEARG_1,	  &t_string,	f_screenstring},
    {"search",		1, 4, FEARG_1,	  &t_number,	f_search},
    {"searchdecl",	1, 3, FEARG_1,	  &t_number,	f_searchdecl},
    {"searchpair",	3, 7, 0,	  &t_number,	f_searchpair},
    {"searchpairpos",	3, 7, 0,	  &t_list_number, f_searchpairpos},
    {"searchpos",	1, 4, FEARG_1,	  &t_list_number, f_searchpos},
    {"server2client",	2, 2, FEARG_1,	  &t_number,	f_server2client},
    {"serverlist",	0, 0, 0,	  &t_string,	f_serverlist},
    {"setbufline",	3, 3, FEARG_3,	  &t_number,	f_setbufline},
    {"setbufvar",	3, 3, FEARG_3,	  &t_void,	f_setbufvar},
    {"setcharsearch",	1, 1, FEARG_1,	  &t_void,	f_setcharsearch},
    {"setcmdpos",	1, 1, FEARG_1,	  &t_number,	f_setcmdpos},
    {"setenv",		2, 2, FEARG_2,	  &t_void,	f_setenv},
    {"setfperm",	2, 2, FEARG_1,	  &t_number,	f_setfperm},
    {"setline",		2, 2, FEARG_2,	  &t_number,	f_setline},
    {"setloclist",	2, 4, FEARG_2,	  &t_number,	f_setloclist},
    {"setmatches",	1, 2, FEARG_1,	  &t_number,	f_setmatches},
    {"setpos",		2, 2, FEARG_2,	  &t_number,	f_setpos},
    {"setqflist",	1, 3, FEARG_1,	  &t_number,	f_setqflist},
    {"setreg",		2, 3, FEARG_2,	  &t_number,	f_setreg},
    {"settabvar",	3, 3, FEARG_3,	  &t_void,	f_settabvar},
    {"settabwinvar",	4, 4, FEARG_4,	  &t_void,	f_settabwinvar},
    {"settagstack",	2, 3, FEARG_2,	  &t_number,	f_settagstack},
    {"setwinvar",	3, 3, FEARG_3,	  &t_void,	f_setwinvar},
#ifdef FEAT_CRYPT
    {"sha256",		1, 1, FEARG_1,	  &t_string,	f_sha256},
#endif
    {"shellescape",	1, 2, FEARG_1,	  &t_string,	f_shellescape},
    {"shiftwidth",	0, 1, FEARG_1,	  &t_number,	f_shiftwidth},
#ifdef FEAT_SIGNS
    {"sign_define",	1, 2, FEARG_1,	  &t_any,	f_sign_define},
    {"sign_getdefined",	0, 1, FEARG_1,	  &t_list_dict_any, f_sign_getdefined},
    {"sign_getplaced",	0, 2, FEARG_1,	  &t_list_dict_any, f_sign_getplaced},
    {"sign_jump",	3, 3, FEARG_1,	  &t_number,	f_sign_jump},
    {"sign_place",	4, 5, FEARG_1,	  &t_number,	f_sign_place},
    {"sign_placelist",	1, 1, FEARG_1,	  &t_list_number, f_sign_placelist},
    {"sign_undefine",	0, 1, FEARG_1,	  &t_number,	f_sign_undefine},
    {"sign_unplace",	1, 2, FEARG_1,	  &t_number,	f_sign_unplace},
    {"sign_unplacelist", 1, 2, FEARG_1,	  &t_list_number, f_sign_unplacelist},
#endif
    {"simplify",	1, 1, 0,	  &t_string,	f_simplify},
#ifdef FEAT_FLOAT
    {"sin",		1, 1, FEARG_1,	  &t_float,	f_sin},
    {"sinh",		1, 1, FEARG_1,	  &t_float,	f_sinh},
#endif
    {"sort",		1, 3, FEARG_1,	  &t_list_any,	f_sort},
#ifdef FEAT_SOUND
    {"sound_clear",	0, 0, 0,	  &t_void,	f_sound_clear},
    {"sound_playevent",	1, 2, FEARG_1,	  &t_number,	f_sound_playevent},
    {"sound_playfile",	1, 2, FEARG_1,	  &t_number,	f_sound_playfile},
    {"sound_stop",	1, 1, FEARG_1,	  &t_void,	f_sound_stop},
#endif
    {"soundfold",	1, 1, FEARG_1,	  &t_string,	f_soundfold},
    {"spellbadword",	0, 1, FEARG_1,	  &t_list_string, f_spellbadword},
    {"spellsuggest",	1, 3, FEARG_1,	  &t_list_string, f_spellsuggest},
    {"split",		1, 3, FEARG_1,	  &t_list_string, f_split},
#ifdef FEAT_FLOAT
    {"sqrt",		1, 1, FEARG_1,	  &t_float,	f_sqrt},
#endif
    {"srand",		0, 1, FEARG_1,	  &t_list_number, f_srand},
    {"state",		0, 1, FEARG_1,	  &t_string,	f_state},
#ifdef FEAT_FLOAT
    {"str2float",	1, 1, FEARG_1,	  &t_float,	f_str2float},
#endif
    {"str2list",	1, 2, FEARG_1,	  &t_list_number, f_str2list},
    {"str2nr",		1, 3, FEARG_1,	  &t_number,	f_str2nr},
    {"strcharpart",	2, 3, FEARG_1,	  &t_string,	f_strcharpart},
    {"strchars",	1, 2, FEARG_1,	  &t_number,	f_strchars},
    {"strdisplaywidth",	1, 2, FEARG_1,	  &t_number,	f_strdisplaywidth},
#ifdef HAVE_STRFTIME
    {"strftime",	1, 2, FEARG_1,	  &t_string,	f_strftime},
#endif
    {"strgetchar",	2, 2, FEARG_1,	  &t_number,	f_strgetchar},
    {"stridx",		2, 3, FEARG_1,	  &t_number,	f_stridx},
    {"string",		1, 1, FEARG_1,	  &t_string,	f_string},
    {"strlen",		1, 1, FEARG_1,	  &t_number,	f_strlen},
    {"strpart",		2, 3, FEARG_1,	  &t_string,	f_strpart},
#ifdef HAVE_STRPTIME
    {"strptime",	2, 2, FEARG_1,	  &t_number,	f_strptime},
#endif
    {"strridx",		2, 3, FEARG_1,	  &t_number,	f_strridx},
    {"strtrans",	1, 1, FEARG_1,	  &t_string,	f_strtrans},
    {"strwidth",	1, 1, FEARG_1,	  &t_number,	f_strwidth},
    {"submatch",	1, 2, FEARG_1,	  &t_string,	f_submatch},
    {"substitute",	4, 4, FEARG_1,	  &t_string,	f_substitute},
    {"swapinfo",	1, 1, FEARG_1,	  &t_dict_any,	f_swapinfo},
    {"swapname",	1, 1, FEARG_1,	  &t_string,	f_swapname},
    {"synID",		3, 3, 0,	  &t_number,	f_synID},
    {"synIDattr",	2, 3, FEARG_1,	  &t_string,	f_synIDattr},
    {"synIDtrans",	1, 1, FEARG_1,	  &t_number,	f_synIDtrans},
    {"synconcealed",	2, 2, 0,	  &t_list_any,	f_synconcealed},
    {"synstack",	2, 2, 0,	  &t_list_number, f_synstack},
    {"system",		1, 2, FEARG_1,	  &t_string,	f_system},
    {"systemlist",	1, 2, FEARG_1,	  &t_list_string, f_systemlist},
    {"tabpagebuflist",	0, 1, FEARG_1,	  &t_list_number, f_tabpagebuflist},
    {"tabpagenr",	0, 1, 0,	  &t_number,	f_tabpagenr},
    {"tabpagewinnr",	1, 2, FEARG_1,	  &t_number,	f_tabpagewinnr},
    {"tagfiles",	0, 0, 0,	  &t_list_string, f_tagfiles},
    {"taglist",		1, 2, FEARG_1,	  &t_list_dict_any, f_taglist},
#ifdef FEAT_FLOAT
    {"tan",		1, 1, FEARG_1,	  &t_float,	f_tan},
    {"tanh",		1, 1, FEARG_1,	  &t_float,	f_tanh},
#endif
    {"tempname",	0, 0, 0,	  &t_string,	f_tempname},
#ifdef FEAT_TERMINAL
    {"term_dumpdiff",	2, 3, FEARG_1,	  &t_number,	f_term_dumpdiff},
    {"term_dumpload",	1, 2, FEARG_1,	  &t_number,	f_term_dumpload},
    {"term_dumpwrite",	2, 3, FEARG_2,	  &t_void,	f_term_dumpwrite},
    {"term_getaltscreen", 1, 1, FEARG_1,  &t_number,	f_term_getaltscreen},
# if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
    {"term_getansicolors", 1, 1, FEARG_1, &t_list_string, f_term_getansicolors},
# endif
    {"term_getattr",	2, 2, FEARG_1,	  &t_number,	f_term_getattr},
    {"term_getcursor",	1, 1, FEARG_1,	  &t_list_any,	f_term_getcursor},
    {"term_getjob",	1, 1, FEARG_1,	  &t_job,	f_term_getjob},
    {"term_getline",	2, 2, FEARG_1,	  &t_string,	f_term_getline},
    {"term_getscrolled", 1, 1, FEARG_1,	  &t_number,	f_term_getscrolled},
    {"term_getsize",	1, 1, FEARG_1,	  &t_list_number, f_term_getsize},
    {"term_getstatus",	1, 1, FEARG_1,	  &t_string,	f_term_getstatus},
    {"term_gettitle",	1, 1, FEARG_1,	  &t_string,	f_term_gettitle},
    {"term_gettty",	1, 2, FEARG_1,	  &t_string,	f_term_gettty},
    {"term_list",	0, 0, 0,	  &t_list_number, f_term_list},
    {"term_scrape",	2, 2, FEARG_1,	  &t_list_dict_any, f_term_scrape},
    {"term_sendkeys",	2, 2, FEARG_1,	  &t_void,	f_term_sendkeys},
# if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
    {"term_setansicolors", 2, 2, FEARG_1, &t_void,	f_term_setansicolors},
# endif
    {"term_setapi",	2, 2, FEARG_1,	  &t_void,	f_term_setapi},
    {"term_setkill",	2, 2, FEARG_1,	  &t_void,	f_term_setkill},
    {"term_setrestore",	2, 2, FEARG_1,	  &t_void,	f_term_setrestore},
    {"term_setsize",	3, 3, FEARG_1,	  &t_void,	f_term_setsize},
    {"term_start",	1, 2, FEARG_1,	  &t_number,	f_term_start},
    {"term_wait",	1, 2, FEARG_1,	  &t_void,	f_term_wait},
#endif
    {"test_alloc_fail",	3, 3, FEARG_1,	  &t_void,	f_test_alloc_fail},
    {"test_autochdir",	0, 0, 0,	  &t_void,	f_test_autochdir},
    {"test_feedinput",	1, 1, FEARG_1,	  &t_void,	f_test_feedinput},
    {"test_garbagecollect_now",	0, 0, 0,  &t_void,	f_test_garbagecollect_now},
    {"test_garbagecollect_soon", 0, 0, 0, &t_void,	f_test_garbagecollect_soon},
    {"test_getvalue",	1, 1, FEARG_1,	  &t_number,	f_test_getvalue},
    {"test_ignore_error", 1, 1, FEARG_1,  &t_void,	f_test_ignore_error},
    {"test_null_blob",	0, 0, 0,	  &t_blob,	f_test_null_blob},
#ifdef FEAT_JOB_CHANNEL
    {"test_null_channel", 0, 0, 0,	  &t_channel,	f_test_null_channel},
#endif
    {"test_null_dict",	0, 0, 0,	  &t_dict_any,	f_test_null_dict},
#ifdef FEAT_JOB_CHANNEL
    {"test_null_job",	0, 0, 0,	  &t_job,	f_test_null_job},
#endif
    {"test_null_list",	0, 0, 0,	  &t_list_any,	f_test_null_list},
    {"test_null_partial", 0, 0, 0,	  &t_partial_void, f_test_null_partial},
    {"test_null_string", 0, 0, 0,	  &t_string,	f_test_null_string},
    {"test_option_not_set", 1, 1, FEARG_1,&t_void,	 f_test_option_not_set},
    {"test_override",	2, 2, FEARG_2,	  &t_void,	f_test_override},
    {"test_refcount",	1, 1, FEARG_1,	  &t_number,	f_test_refcount},
#ifdef FEAT_GUI
    {"test_scrollbar",	3, 3, FEARG_2,	  &t_void,	f_test_scrollbar},
#endif
    {"test_setmouse",	2, 2, 0,	  &t_void,	f_test_setmouse},
    {"test_settime",	1, 1, FEARG_1,	  &t_void,	f_test_settime},
#ifdef FEAT_TIMERS
    {"timer_info",	0, 1, FEARG_1,	  &t_list_dict_any, f_timer_info},
    {"timer_pause",	2, 2, FEARG_1,	  &t_void,	f_timer_pause},
    {"timer_start",	2, 3, FEARG_1,	  &t_number,	f_timer_start},
    {"timer_stop",	1, 1, FEARG_1,	  &t_void,	f_timer_stop},
    {"timer_stopall",	0, 0, 0,	  &t_void,	f_timer_stopall},
#endif
    {"tolower",		1, 1, FEARG_1,	  &t_string,	f_tolower},
    {"toupper",		1, 1, FEARG_1,	  &t_string,	f_toupper},
    {"tr",		3, 3, FEARG_1,	  &t_string,	f_tr},
    {"trim",		1, 2, FEARG_1,	  &t_string,	f_trim},
#ifdef FEAT_FLOAT
    {"trunc",		1, 1, FEARG_1,	  &t_float,	f_trunc},
#endif
    {"type",		1, 1, FEARG_1,	  &t_number,	f_type},
    {"undofile",	1, 1, FEARG_1,	  &t_string,	f_undofile},
    {"undotree",	0, 0, 0,	  &t_dict_any,	f_undotree},
    {"uniq",		1, 3, FEARG_1,	  &t_list_any,	f_uniq},
    {"values",		1, 1, FEARG_1,	  &t_list_any,	f_values},
    {"virtcol",		1, 1, FEARG_1,	  &t_number,	f_virtcol},
    {"visualmode",	0, 1, 0,	  &t_string,	f_visualmode},
    {"wildmenumode",	0, 0, 0,	  &t_number,	f_wildmenumode},
    {"win_execute",	2, 3, FEARG_2,	  &t_string,	f_win_execute},
    {"win_findbuf",	1, 1, FEARG_1,	  &t_list_number, f_win_findbuf},
    {"win_getid",	0, 2, FEARG_1,	  &t_number,	f_win_getid},
    {"win_gotoid",	1, 1, FEARG_1,	  &t_number,	f_win_gotoid},
    {"win_id2tabwin",	1, 1, FEARG_1,	  &t_list_number, f_win_id2tabwin},
    {"win_id2win",	1, 1, FEARG_1,	  &t_number,	f_win_id2win},
    {"win_screenpos",	1, 1, FEARG_1,	  &t_list_number, f_win_screenpos},
    {"win_splitmove",   2, 3, FEARG_1,    &t_number,	f_win_splitmove},
    {"winbufnr",	1, 1, FEARG_1,	  &t_number,	f_winbufnr},
    {"wincol",		0, 0, 0,	  &t_number,	f_wincol},
    {"windowsversion",	0, 0, 0,	  &t_string,	f_windowsversion},
    {"winheight",	1, 1, FEARG_1,	  &t_number,	f_winheight},
    {"winlayout",	0, 1, FEARG_1,	  &t_list_any,	f_winlayout},
    {"winline",		0, 0, 0,	  &t_number,	f_winline},
    {"winnr",		0, 1, FEARG_1,	  &t_number,	f_winnr},
    {"winrestcmd",	0, 0, 0,	  &t_string,	f_winrestcmd},
    {"winrestview",	1, 1, FEARG_1,	  &t_void,	f_winrestview},
    {"winsaveview",	0, 0, 0,	  &t_dict_any,	f_winsaveview},
    {"winwidth",	1, 1, FEARG_1,	  &t_number,	f_winwidth},
    {"wordcount",	0, 0, 0,	  &t_dict_number, f_wordcount},
    {"writefile",	2, 3, FEARG_1,	  &t_number,	f_writefile},
    {"xor",		2, 2, FEARG_1,	  &t_number,	f_xor},
};

/*
 * Function given to ExpandGeneric() to obtain the list of internal
 * or user defined function names.
 */
    char_u *
get_function_name(expand_T *xp, int idx)
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
    if (++intidx < (int)(sizeof(global_functions) / sizeof(funcentry_T)))
    {
	STRCPY(IObuff, global_functions[intidx].f_name);
	STRCAT(IObuff, "(");
	if (global_functions[intidx].f_max_argc == 0)
	    STRCAT(IObuff, ")");
	return IObuff;
    }

    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the list of internal or
 * user defined variable or function names.
 */
    char_u *
get_expr_name(expand_T *xp, int idx)
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

/*
 * Find internal function "name" in table "global_functions".
 * Return index, or -1 if not found
 */
    int
find_internal_func(char_u *name)
{
    int		first = 0;
    int		last;
    int		cmp;
    int		x;

    last = (int)(sizeof(global_functions) / sizeof(funcentry_T)) - 1;

    // Find the function name in the table. Binary search.
    while (first <= last)
    {
	x = first + ((unsigned)(last - first) >> 1);
	cmp = STRCMP(name, global_functions[x].f_name);
	if (cmp < 0)
	    last = x - 1;
	else if (cmp > 0)
	    first = x + 1;
	else
	    return x;
    }
    return -1;
}

    int
has_internal_func(char_u *name)
{
    return find_internal_func(name) >= 0;
}

    char *
internal_func_name(int idx)
{
    return global_functions[idx].f_name;
}

    type_T *
internal_func_ret_type(int idx, int argcount)
{
    funcentry_T *fe = &global_functions[idx];

    if (fe->f_func == f_getline)
	return argcount == 1 ? &t_string : &t_list_string;
    return fe->f_rettype;
}

/*
 * Check the argument count to use for internal function "idx".
 * Returns OK or FAIL;
 */
    int
check_internal_func(int idx, int argcount)
{
    int	    res;
    char    *name;

    if (argcount < global_functions[idx].f_min_argc)
	res = FCERR_TOOFEW;
    else if (argcount > global_functions[idx].f_max_argc)
	res = FCERR_TOOMANY;
    else
	return OK;

    name = internal_func_name(idx);
    if (res == FCERR_TOOMANY)
	semsg(_(e_toomanyarg), name);
    else
	semsg(_(e_toofewarg), name);
    return FAIL;
}

    int
call_internal_func(
	char_u	    *name,
	int	    argcount,
	typval_T    *argvars,
	typval_T    *rettv)
{
    int i;

    i = find_internal_func(name);
    if (i < 0)
	return FCERR_UNKNOWN;
    if (argcount < global_functions[i].f_min_argc)
	return FCERR_TOOFEW;
    if (argcount > global_functions[i].f_max_argc)
	return FCERR_TOOMANY;
    argvars[argcount].v_type = VAR_UNKNOWN;
    global_functions[i].f_func(argvars, rettv);
    return FCERR_NONE;
}

    void
call_internal_func_by_idx(
	int	    idx,
	typval_T    *argvars,
	typval_T    *rettv)
{
    global_functions[idx].f_func(argvars, rettv);
}

/*
 * Invoke a method for base->method().
 */
    int
call_internal_method(
	char_u	    *name,
	int	    argcount,
	typval_T    *argvars,
	typval_T    *rettv,
	typval_T    *basetv)
{
    int		i;
    int		fi;
    typval_T	argv[MAX_FUNC_ARGS + 1];

    fi = find_internal_func(name);
    if (fi < 0)
	return FCERR_UNKNOWN;
    if (global_functions[fi].f_argtype == 0)
	return FCERR_NOTMETHOD;
    if (argcount + 1 < global_functions[fi].f_min_argc)
	return FCERR_TOOFEW;
    if (argcount + 1 > global_functions[fi].f_max_argc)
	return FCERR_TOOMANY;

    if (global_functions[fi].f_argtype == FEARG_LAST)
    {
	// base value goes last
	for (i = 0; i < argcount; ++i)
	    argv[i] = argvars[i];
	argv[argcount] = *basetv;
    }
    else if (global_functions[fi].f_argtype == FEARG_2)
    {
	// base value goes second
	argv[0] = argvars[0];
	argv[1] = *basetv;
	for (i = 1; i < argcount; ++i)
	    argv[i + 1] = argvars[i];
    }
    else if (global_functions[fi].f_argtype == FEARG_3)
    {
	// base value goes third
	argv[0] = argvars[0];
	argv[1] = argvars[1];
	argv[2] = *basetv;
	for (i = 2; i < argcount; ++i)
	    argv[i + 1] = argvars[i];
    }
    else if (global_functions[fi].f_argtype == FEARG_4)
    {
	// base value goes fourth
	argv[0] = argvars[0];
	argv[1] = argvars[1];
	argv[2] = argvars[2];
	argv[3] = *basetv;
	for (i = 3; i < argcount; ++i)
	    argv[i + 1] = argvars[i];
    }
    else
    {
	// FEARG_1: base value goes first
	argv[0] = *basetv;
	for (i = 0; i < argcount; ++i)
	    argv[i + 1] = argvars[i];
    }
    argv[argcount + 1].v_type = VAR_UNKNOWN;

    global_functions[fi].f_func(argv, rettv);
    return FCERR_NONE;
}

/*
 * Return TRUE for a non-zero Number and a non-empty String.
 */
    int
non_zero_arg(typval_T *argvars)
{
    return ((argvars[0].v_type == VAR_NUMBER
		&& argvars[0].vval.v_number != 0)
	    || (argvars[0].v_type == VAR_BOOL
		&& argvars[0].vval.v_number == VVAL_TRUE)
	    || (argvars[0].v_type == VAR_STRING
		&& argvars[0].vval.v_string != NULL
		&& *argvars[0].vval.v_string != NUL));
}

/*
 * Get the lnum from the first argument.
 * Also accepts ".", "$", etc., but that only works for the current buffer.
 * Returns -1 on error.
 */
    linenr_T
tv_get_lnum(typval_T *argvars)
{
    linenr_T	lnum;

    lnum = (linenr_T)tv_get_number_chk(&argvars[0], NULL);
    if (lnum == 0)  // no valid number, try using arg like line()
    {
	int	fnum;
	pos_T	*fp = var2fpos(&argvars[0], TRUE, &fnum);

	if (fp != NULL)
	    lnum = fp->lnum;
    }
    return lnum;
}

/*
 * Get the lnum from the first argument.
 * Also accepts "$", then "buf" is used.
 * Returns 0 on error.
 */
    linenr_T
tv_get_lnum_buf(typval_T *argvars, buf_T *buf)
{
    if (argvars[0].v_type == VAR_STRING
	    && argvars[0].vval.v_string != NULL
	    && argvars[0].vval.v_string[0] == '$'
	    && buf != NULL)
	return buf->b_ml.ml_line_count;
    return (linenr_T)tv_get_number_chk(&argvars[0], NULL);
}

#ifdef FEAT_FLOAT
/*
 * Get the float value of "argvars[0]" into "f".
 * Returns FAIL when the argument is not a Number or Float.
 */
    static int
get_float_arg(typval_T *argvars, float_T *f)
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
    emsg(_("E808: Number or Float required"));
    return FAIL;
}

/*
 * "abs(expr)" function
 */
    static void
f_abs(typval_T *argvars, typval_T *rettv)
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

	n = tv_get_number_chk(&argvars[0], &error);
	if (error)
	    rettv->vval.v_number = -1;
	else if (n > 0)
	    rettv->vval.v_number = n;
	else
	    rettv->vval.v_number = -n;
    }
}

/*
 * "acos()" function
 */
    static void
f_acos(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = acos(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "and(expr, expr)" function
 */
    static void
f_and(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = tv_get_number_chk(&argvars[0], NULL)
					& tv_get_number_chk(&argvars[1], NULL);
}

#ifdef FEAT_FLOAT
/*
 * "asin()" function
 */
    static void
f_asin(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = asin(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "atan()" function
 */
    static void
f_atan(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = atan(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "atan2()" function
 */
    static void
f_atan2(typval_T *argvars, typval_T *rettv)
{
    float_T	fx = 0.0, fy = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &fx) == OK
				     && get_float_arg(&argvars[1], &fy) == OK)
	rettv->vval.v_float = atan2(fx, fy);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "balloon_show()" function
 */
#ifdef FEAT_BEVAL
    static void
f_balloon_gettext(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    if (balloonEval != NULL)
    {
	if (balloonEval->msg == NULL)
	    rettv->vval.v_string = NULL;
	else
	    rettv->vval.v_string = vim_strsave(balloonEval->msg);
    }
}

    static void
f_balloon_show(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (balloonEval != NULL)
    {
	if (argvars[0].v_type == VAR_LIST
# ifdef FEAT_GUI
		&& !gui.in_use
# endif
	   )
	{
	    list_T *l = argvars[0].vval.v_list;

	    // empty list removes the balloon
	    post_balloon(balloonEval, NULL,
				       l == NULL || l->lv_len == 0 ? NULL : l);
	}
	else
	{
	    char_u *mesg = tv_get_string_chk(&argvars[0]);

	    if (mesg != NULL)
		// empty string removes the balloon
		post_balloon(balloonEval, *mesg == NUL ? NULL : mesg, NULL);
	}
    }
}

# if defined(FEAT_BEVAL_TERM)
    static void
f_balloon_split(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (rettv_list_alloc(rettv) == OK)
    {
	char_u *msg = tv_get_string_chk(&argvars[0]);

	if (msg != NULL)
	{
	    pumitem_T	*array;
	    int		size = split_message(msg, &array);
	    int		i;

	    // Skip the first and last item, they are always empty.
	    for (i = 1; i < size - 1; ++i)
		list_append_string(rettv->vval.v_list, array[i].pum_text, -1);
	    while (size > 0)
		vim_free(array[--size].pum_text);
	    vim_free(array);
	}
    }
}
# endif
#endif

/*
 * Get buffer by number or pattern.
 */
    buf_T *
tv_get_buf(typval_T *tv, int curtab_only)
{
    char_u	*name = tv->vval.v_string;
    buf_T	*buf;

    if (tv->v_type == VAR_NUMBER)
	return buflist_findnr((int)tv->vval.v_number);
    if (tv->v_type != VAR_STRING)
	return NULL;
    if (name == NULL || *name == NUL)
	return curbuf;
    if (name[0] == '$' && name[1] == NUL)
	return lastbuf;

    buf = buflist_find_by_name(name, curtab_only);

    // If not found, try expanding the name, like done for bufexists().
    if (buf == NULL)
	buf = find_buffer(tv);

    return buf;
}

/*
 * Get the buffer from "arg" and give an error and return NULL if it is not
 * valid.
 */
    buf_T *
get_buf_arg(typval_T *arg)
{
    buf_T *buf;

    ++emsg_off;
    buf = tv_get_buf(arg, FALSE);
    --emsg_off;
    if (buf == NULL)
	semsg(_("E158: Invalid buffer name: %s"), tv_get_string(arg));
    return buf;
}

/*
 * "byte2line(byte)" function
 */
    static void
f_byte2line(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifndef FEAT_BYTEOFF
    rettv->vval.v_number = -1;
#else
    long	boff = 0;

    boff = tv_get_number(&argvars[0]) - 1;  // boff gets -1 on type error
    if (boff < 0)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = ml_find_line_or_offset(curbuf,
							  (linenr_T)0, &boff);
#endif
}

    static void
byteidx(typval_T *argvars, typval_T *rettv, int comp UNUSED)
{
    char_u	*t;
    char_u	*str;
    varnumber_T	idx;

    str = tv_get_string_chk(&argvars[0]);
    idx = tv_get_number_chk(&argvars[1], NULL);
    rettv->vval.v_number = -1;
    if (str == NULL || idx < 0)
	return;

    t = str;
    for ( ; idx > 0; idx--)
    {
	if (*t == NUL)		// EOL reached
	    return;
	if (enc_utf8 && comp)
	    t += utf_ptr2len(t);
	else
	    t += (*mb_ptr2len)(t);
    }
    rettv->vval.v_number = (varnumber_T)(t - str);
}

/*
 * "byteidx()" function
 */
    static void
f_byteidx(typval_T *argvars, typval_T *rettv)
{
    byteidx(argvars, rettv, FALSE);
}

/*
 * "byteidxcomp()" function
 */
    static void
f_byteidxcomp(typval_T *argvars, typval_T *rettv)
{
    byteidx(argvars, rettv, TRUE);
}

/*
 * "call(func, arglist [, dict])" function
 */
    static void
f_call(typval_T *argvars, typval_T *rettv)
{
    char_u	*func;
    partial_T   *partial = NULL;
    dict_T	*selfdict = NULL;

    if (argvars[1].v_type != VAR_LIST)
    {
	emsg(_(e_listreq));
	return;
    }
    if (argvars[1].vval.v_list == NULL)
	return;

    if (argvars[0].v_type == VAR_FUNC)
	func = argvars[0].vval.v_string;
    else if (argvars[0].v_type == VAR_PARTIAL)
    {
	partial = argvars[0].vval.v_partial;
	func = partial_name(partial);
    }
    else
	func = tv_get_string(&argvars[0]);
    if (*func == NUL)
	return;		// type error or empty name

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	if (argvars[2].v_type != VAR_DICT)
	{
	    emsg(_(e_dictreq));
	    return;
	}
	selfdict = argvars[2].vval.v_dict;
    }

    (void)func_call(func, &argvars[1], partial, selfdict, rettv);
}

#ifdef FEAT_FLOAT
/*
 * "ceil({float})" function
 */
    static void
f_ceil(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

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
    static void
f_changenr(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = curbuf->b_u_seq_cur;
}

/*
 * "char2nr(string)" function
 */
    static void
f_char2nr(typval_T *argvars, typval_T *rettv)
{
    if (has_mbyte)
    {
	int	utf8 = 0;

	if (argvars[1].v_type != VAR_UNKNOWN)
	    utf8 = (int)tv_get_number_chk(&argvars[1], NULL);

	if (utf8)
	    rettv->vval.v_number = utf_ptr2char(tv_get_string(&argvars[0]));
	else
	    rettv->vval.v_number = (*mb_ptr2char)(tv_get_string(&argvars[0]));
    }
    else
	rettv->vval.v_number = tv_get_string(&argvars[0])[0];
}

    win_T *
get_optional_window(typval_T *argvars, int idx)
{
    win_T   *win = curwin;

    if (argvars[idx].v_type != VAR_UNKNOWN)
    {
	win = find_win_by_nr_or_id(&argvars[idx]);
	if (win == NULL)
	{
	    emsg(_(e_invalwindow));
	    return NULL;
	}
    }
    return win;
}

/*
 * "col(string)" function
 */
    static void
f_col(typval_T *argvars, typval_T *rettv)
{
    colnr_T	col = 0;
    pos_T	*fp;
    int		fnum = curbuf->b_fnum;

    fp = var2fpos(&argvars[0], FALSE, &fnum);
    if (fp != NULL && fnum == curbuf->b_fnum)
    {
	if (fp->col == MAXCOL)
	{
	    // '> can be MAXCOL, get the length of the line then
	    if (fp->lnum <= curbuf->b_ml.ml_line_count)
		col = (colnr_T)STRLEN(ml_get(fp->lnum)) + 1;
	    else
		col = MAXCOL;
	}
	else
	{
	    col = fp->col + 1;
	    // col(".") when the cursor is on the NUL at the end of the line
	    // because of "coladd" can be seen as an extra column.
	    if (virtual_active() && fp == &curwin->w_cursor)
	    {
		char_u	*p = ml_get_cursor();

		if (curwin->w_cursor.coladd >= (colnr_T)chartabsize(p,
				 curwin->w_virtcol - curwin->w_cursor.coladd))
		{
		    int		l;

		    if (*p != NUL && p[(l = (*mb_ptr2len)(p))] == NUL)
			col += l;
		}
	    }
	}
    }
    rettv->vval.v_number = col;
}

/*
 * "confirm(message, buttons[, default [, type]])" function
 */
    static void
f_confirm(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
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

    message = tv_get_string_chk(&argvars[0]);
    if (message == NULL)
	error = TRUE;
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	buttons = tv_get_string_buf_chk(&argvars[1], buf);
	if (buttons == NULL)
	    error = TRUE;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    def = (int)tv_get_number_chk(&argvars[2], &error);
	    if (argvars[3].v_type != VAR_UNKNOWN)
	    {
		typestr = tv_get_string_buf_chk(&argvars[3], buf2);
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

    if (!error)
	rettv->vval.v_number = do_dialog(type, NULL, message, buttons,
							    def, NULL, FALSE);
#endif
}

/*
 * "copy()" function
 */
    static void
f_copy(typval_T *argvars, typval_T *rettv)
{
    item_copy(&argvars[0], rettv, FALSE, 0);
}

#ifdef FEAT_FLOAT
/*
 * "cos()" function
 */
    static void
f_cos(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = cos(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "cosh()" function
 */
    static void
f_cosh(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = cosh(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "cursor(lnum, col)" function, or
 * "cursor(list)"
 *
 * Moves the cursor to the specified line and column.
 * Returns 0 when the position could be set, -1 otherwise.
 */
    static void
f_cursor(typval_T *argvars, typval_T *rettv)
{
    long	line, col;
    long	coladd = 0;
    int		set_curswant = TRUE;

    rettv->vval.v_number = -1;
    if (argvars[1].v_type == VAR_UNKNOWN)
    {
	pos_T	    pos;
	colnr_T	    curswant = -1;

	if (list2fpos(argvars, &pos, NULL, &curswant) == FAIL)
	{
	    emsg(_(e_invarg));
	    return;
	}
	line = pos.lnum;
	col = pos.col;
	coladd = pos.coladd;
	if (curswant >= 0)
	{
	    curwin->w_curswant = curswant - 1;
	    set_curswant = FALSE;
	}
    }
    else
    {
	line = tv_get_lnum(argvars);
	col = (long)tv_get_number_chk(&argvars[1], NULL);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    coladd = (long)tv_get_number_chk(&argvars[2], NULL);
    }
    if (line < 0 || col < 0 || coladd < 0)
	return;		// type error; errmsg already given
    if (line > 0)
	curwin->w_cursor.lnum = line;
    if (col > 0)
	curwin->w_cursor.col = col - 1;
    curwin->w_cursor.coladd = coladd;

    // Make sure the cursor is in a valid position.
    check_cursor();
    // Correct cursor for multi-byte character.
    if (has_mbyte)
	mb_adjust_cursor();

    curwin->w_set_curswant = set_curswant;
    rettv->vval.v_number = 0;
}

#ifdef MSWIN
/*
 * "debugbreak()" function
 */
    static void
f_debugbreak(typval_T *argvars, typval_T *rettv)
{
    int		pid;

    rettv->vval.v_number = FAIL;
    pid = (int)tv_get_number(&argvars[0]);
    if (pid == 0)
	emsg(_(e_invarg));
    else
    {
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);

	if (hProcess != NULL)
	{
	    DebugBreakProcess(hProcess);
	    CloseHandle(hProcess);
	    rettv->vval.v_number = OK;
	}
    }
}
#endif

/*
 * "deepcopy()" function
 */
    static void
f_deepcopy(typval_T *argvars, typval_T *rettv)
{
    int		noref = 0;
    int		copyID;

    if (argvars[1].v_type != VAR_UNKNOWN)
	noref = (int)tv_get_number_chk(&argvars[1], NULL);
    if (noref < 0 || noref > 1)
	emsg(_(e_invarg));
    else
    {
	copyID = get_copyID();
	item_copy(&argvars[0], rettv, TRUE, noref == 0 ? copyID : 0);
    }
}

/*
 * "did_filetype()" function
 */
    static void
f_did_filetype(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    rettv->vval.v_number = did_filetype;
}

/*
 * "empty({expr})" function
 */
    static void
f_empty(typval_T *argvars, typval_T *rettv)
{
    int		n = FALSE;

    switch (argvars[0].v_type)
    {
	case VAR_STRING:
	case VAR_FUNC:
	    n = argvars[0].vval.v_string == NULL
					  || *argvars[0].vval.v_string == NUL;
	    break;
	case VAR_PARTIAL:
	    n = FALSE;
	    break;
	case VAR_NUMBER:
	    n = argvars[0].vval.v_number == 0;
	    break;
	case VAR_FLOAT:
#ifdef FEAT_FLOAT
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
	case VAR_BOOL:
	case VAR_SPECIAL:
	    n = argvars[0].vval.v_number != VVAL_TRUE;
	    break;

	case VAR_BLOB:
	    n = argvars[0].vval.v_blob == NULL
		|| argvars[0].vval.v_blob->bv_ga.ga_len == 0;
	    break;

	case VAR_JOB:
#ifdef FEAT_JOB_CHANNEL
	    n = argvars[0].vval.v_job == NULL
			   || argvars[0].vval.v_job->jv_status != JOB_STARTED;
	    break;
#endif
	case VAR_CHANNEL:
#ifdef FEAT_JOB_CHANNEL
	    n = argvars[0].vval.v_channel == NULL
			       || !channel_is_open(argvars[0].vval.v_channel);
	    break;
#endif
	case VAR_UNKNOWN:
	case VAR_VOID:
	    internal_error("f_empty(UNKNOWN)");
	    n = TRUE;
	    break;
    }

    rettv->vval.v_number = n;
}

/*
 * "environ()" function
 */
    static void
f_environ(typval_T *argvars UNUSED, typval_T *rettv)
{
#if !defined(AMIGA)
    int			i = 0;
    char_u		*entry, *value;
# ifdef MSWIN
    extern wchar_t	**_wenviron;
# else
    extern char		**environ;
# endif

    if (rettv_dict_alloc(rettv) != OK)
	return;

# ifdef MSWIN
    if (*_wenviron == NULL)
	return;
# else
    if (*environ == NULL)
	return;
# endif

    for (i = 0; ; ++i)
    {
# ifdef MSWIN
	short_u		*p;

	if ((p = (short_u *)_wenviron[i]) == NULL)
	    return;
	entry = utf16_to_enc(p, NULL);
# else
	if ((entry = (char_u *)environ[i]) == NULL)
	    return;
	entry = vim_strsave(entry);
# endif
	if (entry == NULL) // out of memory
	    return;
	if ((value = vim_strchr(entry, '=')) == NULL)
	{
	    vim_free(entry);
	    continue;
	}
	*value++ = NUL;
	dict_add_string(rettv->vval.v_dict, (char *)entry, value);
	vim_free(entry);
    }
#endif
}

/*
 * "escape({string}, {chars})" function
 */
    static void
f_escape(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];

    rettv->vval.v_string = vim_strsave_escaped(tv_get_string(&argvars[0]),
					 tv_get_string_buf(&argvars[1], buf));
    rettv->v_type = VAR_STRING;
}

/*
 * "eval()" function
 */
    static void
f_eval(typval_T *argvars, typval_T *rettv)
{
    char_u	*s, *p;

    s = tv_get_string_chk(&argvars[0]);
    if (s != NULL)
	s = skipwhite(s);

    p = s;
    if (s == NULL || eval1(&s, rettv, TRUE) == FAIL)
    {
	if (p != NULL && !aborting())
	    semsg(_(e_invexpr2), p);
	need_clr_eos = FALSE;
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = 0;
    }
    else if (*s != NUL)
	emsg(_(e_trailing));
}

/*
 * "eventhandler()" function
 */
    static void
f_eventhandler(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = vgetc_busy;
}

static garray_T	redir_execute_ga;

/*
 * Append "value[value_len]" to the execute() output.
 */
    void
execute_redir_str(char_u *value, int value_len)
{
    int		len;

    if (value_len == -1)
	len = (int)STRLEN(value);	// Append the entire string
    else
	len = value_len;		// Append only "value_len" characters
    if (ga_grow(&redir_execute_ga, len) == OK)
    {
	mch_memmove((char *)redir_execute_ga.ga_data
				       + redir_execute_ga.ga_len, value, len);
	redir_execute_ga.ga_len += len;
    }
}

/*
 * Get next line from a list.
 * Called by do_cmdline() to get the next line.
 * Returns allocated string, or NULL for end of function.
 */

    static char_u *
get_list_line(
    int	    c UNUSED,
    void    *cookie,
    int	    indent UNUSED,
    int	    do_concat UNUSED)
{
    listitem_T **p = (listitem_T **)cookie;
    listitem_T *item = *p;
    char_u	buf[NUMBUFLEN];
    char_u	*s;

    if (item == NULL)
	return NULL;
    s = tv_get_string_buf_chk(&item->li_tv, buf);
    *p = item->li_next;
    return s == NULL ? NULL : vim_strsave(s);
}

/*
 * "execute()" function
 */
    void
execute_common(typval_T *argvars, typval_T *rettv, int arg_off)
{
    char_u	*cmd = NULL;
    list_T	*list = NULL;
    int		save_msg_silent = msg_silent;
    int		save_emsg_silent = emsg_silent;
    int		save_emsg_noredir = emsg_noredir;
    int		save_redir_execute = redir_execute;
    int		save_redir_off = redir_off;
    garray_T	save_ga;
    int		save_msg_col = msg_col;
    int		echo_output = FALSE;

    rettv->vval.v_string = NULL;
    rettv->v_type = VAR_STRING;

    if (argvars[arg_off].v_type == VAR_LIST)
    {
	list = argvars[arg_off].vval.v_list;
	if (list == NULL || list->lv_first == NULL)
	    // empty list, no commands, empty output
	    return;
	++list->lv_refcount;
    }
    else if (argvars[arg_off].v_type == VAR_JOB
	    || argvars[arg_off].v_type == VAR_CHANNEL)
    {
	emsg(_(e_inval_string));
	return;
    }
    else
    {
	cmd = tv_get_string_chk(&argvars[arg_off]);
	if (cmd == NULL)
	    return;
    }

    if (argvars[arg_off + 1].v_type != VAR_UNKNOWN)
    {
	char_u	buf[NUMBUFLEN];
	char_u  *s = tv_get_string_buf_chk(&argvars[arg_off + 1], buf);

	if (s == NULL)
	    return;
	if (*s == NUL)
	    echo_output = TRUE;
	if (STRNCMP(s, "silent", 6) == 0)
	    ++msg_silent;
	if (STRCMP(s, "silent!") == 0)
	{
	    emsg_silent = TRUE;
	    emsg_noredir = TRUE;
	}
    }
    else
	++msg_silent;

    if (redir_execute)
	save_ga = redir_execute_ga;
    ga_init2(&redir_execute_ga, (int)sizeof(char), 500);
    redir_execute = TRUE;
    redir_off = FALSE;
    if (!echo_output)
	msg_col = 0;  // prevent leading spaces

    if (cmd != NULL)
	do_cmdline_cmd(cmd);
    else
    {
	listitem_T	*item = list->lv_first;

	do_cmdline(NULL, get_list_line, (void *)&item,
		      DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT|DOCMD_KEYTYPED);
	--list->lv_refcount;
    }

    // Need to append a NUL to the result.
    if (ga_grow(&redir_execute_ga, 1) == OK)
    {
	((char *)redir_execute_ga.ga_data)[redir_execute_ga.ga_len] = NUL;
	rettv->vval.v_string = redir_execute_ga.ga_data;
    }
    else
    {
	ga_clear(&redir_execute_ga);
	rettv->vval.v_string = NULL;
    }
    msg_silent = save_msg_silent;
    emsg_silent = save_emsg_silent;
    emsg_noredir = save_emsg_noredir;

    redir_execute = save_redir_execute;
    if (redir_execute)
	redir_execute_ga = save_ga;
    redir_off = save_redir_off;

    // "silent reg" or "silent echo x" leaves msg_col somewhere in the line.
    if (echo_output)
	// When not working silently: put it in column zero.  A following
	// "echon" will overwrite the message, unavoidably.
	msg_col = 0;
    else
	// When working silently: Put it back where it was, since nothing
	// should have been written.
	msg_col = save_msg_col;
}

/*
 * "execute()" function
 */
    static void
f_execute(typval_T *argvars, typval_T *rettv)
{
    execute_common(argvars, rettv, 0);
}

/*
 * "exists()" function
 */
    static void
f_exists(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		n = FALSE;

    p = tv_get_string(&argvars[0]);
    if (*p == '$')			// environment variable
    {
	// first try "normal" environment variables (fast)
	if (mch_getenv(p + 1) != NULL)
	    n = TRUE;
	else
	{
	    // try expanding things like $VIM and ${HOME}
	    p = expand_env_save(p);
	    if (p != NULL && *p != '$')
		n = TRUE;
	    vim_free(p);
	}
    }
    else if (*p == '&' || *p == '+')			// option
    {
	n = (get_option_tv(&p, NULL, TRUE) == OK);
	if (*skipwhite(p) != NUL)
	    n = FALSE;			// trailing garbage
    }
    else if (*p == '*')			// internal or user defined function
    {
	n = function_exists(p + 1, FALSE);
    }
    else if (*p == ':')
    {
	n = cmd_exists(p + 1);
    }
    else if (*p == '#')
    {
	if (p[1] == '#')
	    n = autocmd_supported(p + 2);
	else
	    n = au_exists(p + 1);
    }
    else				// internal variable
    {
	n = var_exists(p);
    }

    rettv->vval.v_number = n;
}

#ifdef FEAT_FLOAT
/*
 * "exp()" function
 */
    static void
f_exp(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = exp(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "expand()" function
 */
    static void
f_expand(typval_T *argvars, typval_T *rettv)
{
    char_u	*s;
    int		len;
    char	*errormsg;
    int		options = WILD_SILENT|WILD_USE_NL|WILD_LIST_NOTFOUND;
    expand_T	xpc;
    int		error = FALSE;
    char_u	*result;

    rettv->v_type = VAR_STRING;
    if (argvars[1].v_type != VAR_UNKNOWN
	    && argvars[2].v_type != VAR_UNKNOWN
	    && tv_get_number_chk(&argvars[2], &error)
	    && !error)
	rettv_list_set(rettv, NULL);

    s = tv_get_string(&argvars[0]);
    if (*s == '%' || *s == '#' || *s == '<')
    {
	++emsg_off;
	result = eval_vars(s, s, &len, NULL, &errormsg, NULL);
	--emsg_off;
	if (rettv->v_type == VAR_LIST)
	{
	    if (rettv_list_alloc(rettv) != FAIL && result != NULL)
		list_append_string(rettv->vval.v_list, result, -1);
	    vim_free(result);
	}
	else
	    rettv->vval.v_string = result;
    }
    else
    {
	// When the optional second argument is non-zero, don't remove matches
	// for 'wildignore' and don't put matches for 'suffixes' at the end.
	if (argvars[1].v_type != VAR_UNKNOWN
				    && tv_get_number_chk(&argvars[1], &error))
	    options |= WILD_KEEP_ALL;
	if (!error)
	{
	    ExpandInit(&xpc);
	    xpc.xp_context = EXPAND_FILES;
	    if (p_wic)
		options += WILD_ICASE;
	    if (rettv->v_type == VAR_STRING)
		rettv->vval.v_string = ExpandOne(&xpc, s, NULL,
							   options, WILD_ALL);
	    else if (rettv_list_alloc(rettv) != FAIL)
	    {
		int i;

		ExpandOne(&xpc, s, NULL, options, WILD_ALL_KEEP);
		for (i = 0; i < xpc.xp_numfiles; i++)
		    list_append_string(rettv->vval.v_list, xpc.xp_files[i], -1);
		ExpandCleanup(&xpc);
	    }
	}
	else
	    rettv->vval.v_string = NULL;
    }
}

/*
 * "expandcmd()" function
 * Expand all the special characters in a command string.
 */
    static void
f_expandcmd(typval_T *argvars, typval_T *rettv)
{
    exarg_T	eap;
    char_u	*cmdstr;
    char	*errormsg = NULL;

    rettv->v_type = VAR_STRING;
    cmdstr = vim_strsave(tv_get_string(&argvars[0]));

    memset(&eap, 0, sizeof(eap));
    eap.cmd = cmdstr;
    eap.arg = cmdstr;
    eap.argt |= EX_NOSPC;
    eap.usefilter = FALSE;
    eap.nextcmd = NULL;
    eap.cmdidx = CMD_USER;

    expand_filename(&eap, &cmdstr, &errormsg);
    if (errormsg != NULL && *errormsg != NUL)
	emsg(errormsg);

    rettv->vval.v_string = cmdstr;
}

/*
 * "feedkeys()" function
 */
    static void
f_feedkeys(typval_T *argvars, typval_T *rettv UNUSED)
{
    int		remap = TRUE;
    int		insert = FALSE;
    char_u	*keys, *flags;
    char_u	nbuf[NUMBUFLEN];
    int		typed = FALSE;
    int		execute = FALSE;
    int		dangerous = FALSE;
    int		lowlevel = FALSE;
    char_u	*keys_esc;

    // This is not allowed in the sandbox.  If the commands would still be
    // executed in the sandbox it would be OK, but it probably happens later,
    // when "sandbox" is no longer set.
    if (check_secure())
	return;

    keys = tv_get_string(&argvars[0]);

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	flags = tv_get_string_buf(&argvars[1], nbuf);
	for ( ; *flags != NUL; ++flags)
	{
	    switch (*flags)
	    {
		case 'n': remap = FALSE; break;
		case 'm': remap = TRUE; break;
		case 't': typed = TRUE; break;
		case 'i': insert = TRUE; break;
		case 'x': execute = TRUE; break;
		case '!': dangerous = TRUE; break;
		case 'L': lowlevel = TRUE; break;
	    }
	}
    }

    if (*keys != NUL || execute)
    {
	// Need to escape K_SPECIAL and CSI before putting the string in the
	// typeahead buffer.
	keys_esc = vim_strsave_escape_csi(keys);
	if (keys_esc != NULL)
	{
	    if (lowlevel)
	    {
#ifdef USE_INPUT_BUF
		add_to_input_buf(keys, (int)STRLEN(keys));
#else
		emsg(_("E980: lowlevel input not supported"));
#endif
	    }
	    else
	    {
		ins_typebuf(keys_esc, (remap ? REMAP_YES : REMAP_NONE),
				  insert ? 0 : typebuf.tb_len, !typed, FALSE);
		if (vgetc_busy
#ifdef FEAT_TIMERS
			|| timer_busy
#endif
			)
		    typebuf_was_filled = TRUE;
	    }
	    vim_free(keys_esc);

	    if (execute)
	    {
		int save_msg_scroll = msg_scroll;

		// Avoid a 1 second delay when the keys start Insert mode.
		msg_scroll = FALSE;

		if (!dangerous)
		    ++ex_normal_busy;
		exec_normal(TRUE, lowlevel, TRUE);
		if (!dangerous)
		    --ex_normal_busy;

		msg_scroll |= save_msg_scroll;
	    }
	}
    }
}

#ifdef FEAT_FLOAT
/*
 * "float2nr({float})" function
 */
    static void
f_float2nr(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    if (get_float_arg(argvars, &f) == OK)
    {
	if (f <= -VARNUM_MAX + DBL_EPSILON)
	    rettv->vval.v_number = -VARNUM_MAX;
	else if (f >= VARNUM_MAX - DBL_EPSILON)
	    rettv->vval.v_number = VARNUM_MAX;
	else
	    rettv->vval.v_number = (varnumber_T)f;
    }
}

/*
 * "floor({float})" function
 */
    static void
f_floor(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = floor(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "fmod()" function
 */
    static void
f_fmod(typval_T *argvars, typval_T *rettv)
{
    float_T	fx = 0.0, fy = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &fx) == OK
				     && get_float_arg(&argvars[1], &fy) == OK)
	rettv->vval.v_float = fmod(fx, fy);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "fnameescape({string})" function
 */
    static void
f_fnameescape(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_string = vim_strsave_fnameescape(
					   tv_get_string(&argvars[0]), FALSE);
    rettv->v_type = VAR_STRING;
}

/*
 * "foreground()" function
 */
    static void
f_foreground(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui_mch_set_foreground();
	return;
    }
#endif
#if defined(MSWIN) && (!defined(FEAT_GUI) || defined(VIMDLL))
    win32_set_foreground();
#endif
}

    static void
common_function(typval_T *argvars, typval_T *rettv, int is_funcref)
{
    char_u	*s;
    char_u	*name;
    int		use_string = FALSE;
    partial_T   *arg_pt = NULL;
    char_u	*trans_name = NULL;

    if (argvars[0].v_type == VAR_FUNC)
    {
	// function(MyFunc, [arg], dict)
	s = argvars[0].vval.v_string;
    }
    else if (argvars[0].v_type == VAR_PARTIAL
					 && argvars[0].vval.v_partial != NULL)
    {
	// function(dict.MyFunc, [arg])
	arg_pt = argvars[0].vval.v_partial;
	s = partial_name(arg_pt);
    }
    else
    {
	// function('MyFunc', [arg], dict)
	s = tv_get_string(&argvars[0]);
	use_string = TRUE;
    }

    if ((use_string && vim_strchr(s, AUTOLOAD_CHAR) == NULL) || is_funcref)
    {
	name = s;
	trans_name = trans_function_name(&name, FALSE,
	     TFN_INT | TFN_QUIET | TFN_NO_AUTOLOAD | TFN_NO_DEREF, NULL, NULL);
	if (*name != NUL)
	    s = NULL;
    }

    if (s == NULL || *s == NUL || (use_string && VIM_ISDIGIT(*s))
					 || (is_funcref && trans_name == NULL))
	semsg(_(e_invarg2), use_string ? tv_get_string(&argvars[0]) : s);
    // Don't check an autoload name for existence here.
    else if (trans_name != NULL && (is_funcref
				? find_func(trans_name, NULL) == NULL
				: !translated_function_exists(trans_name)))
	semsg(_("E700: Unknown function: %s"), s);
    else
    {
	int	dict_idx = 0;
	int	arg_idx = 0;
	list_T	*list = NULL;

	if (STRNCMP(s, "s:", 2) == 0 || STRNCMP(s, "<SID>", 5) == 0)
	{
	    char	sid_buf[25];
	    int		off = *s == 's' ? 2 : 5;

	    // Expand s: and <SID> into <SNR>nr_, so that the function can
	    // also be called from another script. Using trans_function_name()
	    // would also work, but some plugins depend on the name being
	    // printable text.
	    sprintf(sid_buf, "<SNR>%ld_", (long)current_sctx.sc_sid);
	    name = alloc(STRLEN(sid_buf) + STRLEN(s + off) + 1);
	    if (name != NULL)
	    {
		STRCPY(name, sid_buf);
		STRCAT(name, s + off);
	    }
	}
	else
	    name = vim_strsave(s);

	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		// function(name, [args], dict)
		arg_idx = 1;
		dict_idx = 2;
	    }
	    else if (argvars[1].v_type == VAR_DICT)
		// function(name, dict)
		dict_idx = 1;
	    else
		// function(name, [args])
		arg_idx = 1;
	    if (dict_idx > 0)
	    {
		if (argvars[dict_idx].v_type != VAR_DICT)
		{
		    emsg(_("E922: expected a dict"));
		    vim_free(name);
		    goto theend;
		}
		if (argvars[dict_idx].vval.v_dict == NULL)
		    dict_idx = 0;
	    }
	    if (arg_idx > 0)
	    {
		if (argvars[arg_idx].v_type != VAR_LIST)
		{
		    emsg(_("E923: Second argument of function() must be a list or a dict"));
		    vim_free(name);
		    goto theend;
		}
		list = argvars[arg_idx].vval.v_list;
		if (list == NULL || list->lv_len == 0)
		    arg_idx = 0;
		else if (list->lv_len > MAX_FUNC_ARGS)
		{
		    emsg_funcname((char *)e_toomanyarg, s);
		    vim_free(name);
		    goto theend;
		}
	    }
	}
	if (dict_idx > 0 || arg_idx > 0 || arg_pt != NULL || is_funcref)
	{
	    partial_T	*pt = ALLOC_CLEAR_ONE(partial_T);

	    // result is a VAR_PARTIAL
	    if (pt == NULL)
		vim_free(name);
	    else
	    {
		if (arg_idx > 0 || (arg_pt != NULL && arg_pt->pt_argc > 0))
		{
		    listitem_T	*li;
		    int		i = 0;
		    int		arg_len = 0;
		    int		lv_len = 0;

		    if (arg_pt != NULL)
			arg_len = arg_pt->pt_argc;
		    if (list != NULL)
			lv_len = list->lv_len;
		    pt->pt_argc = arg_len + lv_len;
		    pt->pt_argv = ALLOC_MULT(typval_T, pt->pt_argc);
		    if (pt->pt_argv == NULL)
		    {
			vim_free(pt);
			vim_free(name);
			goto theend;
		    }
		    for (i = 0; i < arg_len; i++)
			copy_tv(&arg_pt->pt_argv[i], &pt->pt_argv[i]);
		    if (lv_len > 0)
			for (li = list->lv_first; li != NULL;
							 li = li->li_next)
			    copy_tv(&li->li_tv, &pt->pt_argv[i++]);
		}

		// For "function(dict.func, [], dict)" and "func" is a partial
		// use "dict".  That is backwards compatible.
		if (dict_idx > 0)
		{
		    // The dict is bound explicitly, pt_auto is FALSE.
		    pt->pt_dict = argvars[dict_idx].vval.v_dict;
		    ++pt->pt_dict->dv_refcount;
		}
		else if (arg_pt != NULL)
		{
		    // If the dict was bound automatically the result is also
		    // bound automatically.
		    pt->pt_dict = arg_pt->pt_dict;
		    pt->pt_auto = arg_pt->pt_auto;
		    if (pt->pt_dict != NULL)
			++pt->pt_dict->dv_refcount;
		}

		pt->pt_refcount = 1;
		if (arg_pt != NULL && arg_pt->pt_func != NULL)
		{
		    pt->pt_func = arg_pt->pt_func;
		    func_ptr_ref(pt->pt_func);
		    vim_free(name);
		}
		else if (is_funcref)
		{
		    pt->pt_func = find_func(trans_name, NULL);
		    func_ptr_ref(pt->pt_func);
		    vim_free(name);
		}
		else
		{
		    pt->pt_name = name;
		    func_ref(name);
		}
	    }
	    rettv->v_type = VAR_PARTIAL;
	    rettv->vval.v_partial = pt;
	}
	else
	{
	    // result is a VAR_FUNC
	    rettv->v_type = VAR_FUNC;
	    rettv->vval.v_string = name;
	    func_ref(name);
	}
    }
theend:
    vim_free(trans_name);
}

/*
 * "funcref()" function
 */
    static void
f_funcref(typval_T *argvars, typval_T *rettv)
{
    common_function(argvars, rettv, TRUE);
}

/*
 * "function()" function
 */
    static void
f_function(typval_T *argvars, typval_T *rettv)
{
    common_function(argvars, rettv, FALSE);
}

/*
 * "garbagecollect()" function
 */
    static void
f_garbagecollect(typval_T *argvars, typval_T *rettv UNUSED)
{
    // This is postponed until we are back at the toplevel, because we may be
    // using Lists and Dicts internally.  E.g.: ":echo [garbagecollect()]".
    want_garbage_collect = TRUE;

    if (argvars[0].v_type != VAR_UNKNOWN && tv_get_number(&argvars[0]) == 1)
	garbage_collect_at_exit = TRUE;
}

/*
 * "get()" function
 */
    static void
f_get(typval_T *argvars, typval_T *rettv)
{
    listitem_T	*li;
    list_T	*l;
    dictitem_T	*di;
    dict_T	*d;
    typval_T	*tv = NULL;
    int		what_is_dict = FALSE;

    if (argvars[0].v_type == VAR_BLOB)
    {
	int error = FALSE;
	int idx = tv_get_number_chk(&argvars[1], &error);

	if (!error)
	{
	    rettv->v_type = VAR_NUMBER;
	    if (idx < 0)
		idx = blob_len(argvars[0].vval.v_blob) + idx;
	    if (idx < 0 || idx >= blob_len(argvars[0].vval.v_blob))
		rettv->vval.v_number = -1;
	    else
	    {
		rettv->vval.v_number = blob_get(argvars[0].vval.v_blob, idx);
		tv = rettv;
	    }
	}
    }
    else if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) != NULL)
	{
	    int		error = FALSE;

	    li = list_find(l, (long)tv_get_number_chk(&argvars[1], &error));
	    if (!error && li != NULL)
		tv = &li->li_tv;
	}
    }
    else if (argvars[0].v_type == VAR_DICT)
    {
	if ((d = argvars[0].vval.v_dict) != NULL)
	{
	    di = dict_find(d, tv_get_string(&argvars[1]), -1);
	    if (di != NULL)
		tv = &di->di_tv;
	}
    }
    else if (argvars[0].v_type == VAR_PARTIAL || argvars[0].v_type == VAR_FUNC)
    {
	partial_T	*pt;
	partial_T	fref_pt;

	if (argvars[0].v_type == VAR_PARTIAL)
	    pt = argvars[0].vval.v_partial;
	else
	{
	    vim_memset(&fref_pt, 0, sizeof(fref_pt));
	    fref_pt.pt_name = argvars[0].vval.v_string;
	    pt = &fref_pt;
	}

	if (pt != NULL)
	{
	    char_u *what = tv_get_string(&argvars[1]);
	    char_u *n;

	    if (STRCMP(what, "func") == 0 || STRCMP(what, "name") == 0)
	    {
		rettv->v_type = (*what == 'f' ? VAR_FUNC : VAR_STRING);
		n = partial_name(pt);
		if (n == NULL)
		    rettv->vval.v_string = NULL;
		else
		{
		    rettv->vval.v_string = vim_strsave(n);
		    if (rettv->v_type == VAR_FUNC)
			func_ref(rettv->vval.v_string);
		}
	    }
	    else if (STRCMP(what, "dict") == 0)
	    {
		what_is_dict = TRUE;
		if (pt->pt_dict != NULL)
		    rettv_dict_set(rettv, pt->pt_dict);
	    }
	    else if (STRCMP(what, "args") == 0)
	    {
		rettv->v_type = VAR_LIST;
		if (rettv_list_alloc(rettv) == OK)
		{
		    int i;

		    for (i = 0; i < pt->pt_argc; ++i)
			list_append_tv(rettv->vval.v_list, &pt->pt_argv[i]);
		}
	    }
	    else
		semsg(_(e_invarg2), what);

	    // When {what} == "dict" and pt->pt_dict == NULL, evaluate the
	    // third argument
	    if (!what_is_dict)
		return;
	}
    }
    else
	semsg(_(e_listdictblobarg), "get()");

    if (tv == NULL)
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    copy_tv(&argvars[2], rettv);
    }
    else
	copy_tv(tv, rettv);
}

/*
 * "getchangelist()" function
 */
    static void
f_getchangelist(typval_T *argvars, typval_T *rettv)
{
#ifdef FEAT_JUMPLIST
    buf_T	*buf;
    int		i;
    list_T	*l;
    dict_T	*d;
#endif

    if (rettv_list_alloc(rettv) != OK)
	return;

#ifdef FEAT_JUMPLIST
    if (argvars[0].v_type == VAR_UNKNOWN)
	buf = curbuf;
    else
    {
	(void)tv_get_number(&argvars[0]);    // issue errmsg if type error
	++emsg_off;
	buf = tv_get_buf(&argvars[0], FALSE);
	--emsg_off;
    }
    if (buf == NULL)
	return;

    l = list_alloc();
    if (l == NULL)
	return;

    if (list_append_list(rettv->vval.v_list, l) == FAIL)
	return;
    /*
     * The current window change list index tracks only the position in the
     * current buffer change list. For other buffers, use the change list
     * length as the current index.
     */
    list_append_number(rettv->vval.v_list,
	    (varnumber_T)((buf == curwin->w_buffer)
		? curwin->w_changelistidx : buf->b_changelistlen));

    for (i = 0; i < buf->b_changelistlen; ++i)
    {
	if (buf->b_changelist[i].lnum == 0)
	    continue;
	if ((d = dict_alloc()) == NULL)
	    return;
	if (list_append_dict(l, d) == FAIL)
	    return;
	dict_add_number(d, "lnum", (long)buf->b_changelist[i].lnum);
	dict_add_number(d, "col", (long)buf->b_changelist[i].col);
	dict_add_number(d, "coladd", (long)buf->b_changelist[i].coladd);
    }
#endif
}

/*
 * "getcharsearch()" function
 */
    static void
f_getcharsearch(typval_T *argvars UNUSED, typval_T *rettv)
{
    if (rettv_dict_alloc(rettv) != FAIL)
    {
	dict_T *dict = rettv->vval.v_dict;

	dict_add_string(dict, "char", last_csearch());
	dict_add_number(dict, "forward", last_csearch_forward());
	dict_add_number(dict, "until", last_csearch_until());
    }
}

/*
 * "getcmdwintype()" function
 */
    static void
f_getcmdwintype(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_CMDWIN
    rettv->vval.v_string = alloc(2);
    if (rettv->vval.v_string != NULL)
    {
	rettv->vval.v_string[0] = cmdwin_type;
	rettv->vval.v_string[1] = NUL;
    }
#endif
}

/*
 * "getenv()" function
 */
    static void
f_getenv(typval_T *argvars, typval_T *rettv)
{
    int	    mustfree = FALSE;
    char_u  *p = vim_getenv(tv_get_string(&argvars[0]), &mustfree);

    if (p == NULL)
    {
	rettv->v_type = VAR_SPECIAL;
	rettv->vval.v_number = VVAL_NULL;
	return;
    }
    if (!mustfree)
	p = vim_strsave(p);
    rettv->vval.v_string = p;
    rettv->v_type = VAR_STRING;
}

/*
 * "getfontname()" function
 */
    static void
f_getfontname(typval_T *argvars UNUSED, typval_T *rettv)
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
	    // Get the "Normal" font.  Either the name saved by
	    // hl_set_font_name() or from the font ID.
	    font = gui.norm_font;
	    name = hl_get_font_name();
	}
	else
	{
	    name = tv_get_string(&argvars[0]);
	    if (STRCMP(name, "*") == 0)	    // don't use font dialog
		return;
	    font = gui_mch_get_font(name, FALSE);
	    if (font == NOFONT)
		return;	    // Invalid font name, return empty string.
	}
	rettv->vval.v_string = gui_mch_get_fontname(font, name);
	if (argvars[0].v_type != VAR_UNKNOWN)
	    gui_mch_free_font(font);
    }
#endif
}

/*
 * "getjumplist()" function
 */
    static void
f_getjumplist(typval_T *argvars, typval_T *rettv)
{
#ifdef FEAT_JUMPLIST
    win_T	*wp;
    int		i;
    list_T	*l;
    dict_T	*d;
#endif

    if (rettv_list_alloc(rettv) != OK)
	return;

#ifdef FEAT_JUMPLIST
    wp = find_tabwin(&argvars[0], &argvars[1], NULL);
    if (wp == NULL)
	return;

    cleanup_jumplist(wp, TRUE);

    l = list_alloc();
    if (l == NULL)
	return;

    if (list_append_list(rettv->vval.v_list, l) == FAIL)
	return;
    list_append_number(rettv->vval.v_list, (varnumber_T)wp->w_jumplistidx);

    for (i = 0; i < wp->w_jumplistlen; ++i)
    {
	if (wp->w_jumplist[i].fmark.mark.lnum == 0)
	    continue;
	if ((d = dict_alloc()) == NULL)
	    return;
	if (list_append_dict(l, d) == FAIL)
	    return;
	dict_add_number(d, "lnum", (long)wp->w_jumplist[i].fmark.mark.lnum);
	dict_add_number(d, "col", (long)wp->w_jumplist[i].fmark.mark.col);
	dict_add_number(d, "coladd", (long)wp->w_jumplist[i].fmark.mark.coladd);
	dict_add_number(d, "bufnr", (long)wp->w_jumplist[i].fmark.fnum);
	if (wp->w_jumplist[i].fname != NULL)
	    dict_add_string(d, "filename", wp->w_jumplist[i].fname);
    }
#endif
}

/*
 * "getpid()" function
 */
    static void
f_getpid(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = mch_get_pid();
}

    static void
getpos_both(
    typval_T	*argvars,
    typval_T	*rettv,
    int		getcurpos)
{
    pos_T	*fp;
    list_T	*l;
    int		fnum = -1;

    if (rettv_list_alloc(rettv) == OK)
    {
	l = rettv->vval.v_list;
	if (getcurpos)
	    fp = &curwin->w_cursor;
	else
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
	list_append_number(l, (fp != NULL) ? (varnumber_T)fp->coladd :
							      (varnumber_T)0);
	if (getcurpos)
	{
	    int	    save_set_curswant = curwin->w_set_curswant;
	    colnr_T save_curswant = curwin->w_curswant;
	    colnr_T save_virtcol = curwin->w_virtcol;

	    update_curswant();
	    list_append_number(l, curwin->w_curswant == MAXCOL ?
		    (varnumber_T)MAXCOL : (varnumber_T)curwin->w_curswant + 1);

	    // Do not change "curswant", as it is unexpected that a get
	    // function has a side effect.
	    if (save_set_curswant)
	    {
		curwin->w_set_curswant = save_set_curswant;
		curwin->w_curswant = save_curswant;
		curwin->w_virtcol = save_virtcol;
		curwin->w_valid &= ~VALID_VIRTCOL;
	    }
	}
    }
    else
	rettv->vval.v_number = FALSE;
}

/*
 * "getcurpos()" function
 */
    static void
f_getcurpos(typval_T *argvars, typval_T *rettv)
{
    getpos_both(argvars, rettv, TRUE);
}

/*
 * "getpos(string)" function
 */
    static void
f_getpos(typval_T *argvars, typval_T *rettv)
{
    getpos_both(argvars, rettv, FALSE);
}

/*
 * "getreg()" function
 */
    static void
f_getreg(typval_T *argvars, typval_T *rettv)
{
    char_u	*strregname;
    int		regname;
    int		arg2 = FALSE;
    int		return_list = FALSE;
    int		error = FALSE;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	strregname = tv_get_string_chk(&argvars[0]);
	error = strregname == NULL;
	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    arg2 = (int)tv_get_number_chk(&argvars[1], &error);
	    if (!error && argvars[2].v_type != VAR_UNKNOWN)
		return_list = (int)tv_get_number_chk(&argvars[2], &error);
	}
    }
    else
	strregname = get_vim_var_str(VV_REG);

    if (error)
	return;

    regname = (strregname == NULL ? '"' : *strregname);
    if (regname == 0)
	regname = '"';

    if (return_list)
    {
	rettv->v_type = VAR_LIST;
	rettv->vval.v_list = (list_T *)get_reg_contents(regname,
				      (arg2 ? GREG_EXPR_SRC : 0) | GREG_LIST);
	if (rettv->vval.v_list == NULL)
	    (void)rettv_list_alloc(rettv);
	else
	    ++rettv->vval.v_list->lv_refcount;
    }
    else
    {
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = get_reg_contents(regname,
						    arg2 ? GREG_EXPR_SRC : 0);
    }
}

/*
 * "getregtype()" function
 */
    static void
f_getregtype(typval_T *argvars, typval_T *rettv)
{
    char_u	*strregname;
    int		regname;
    char_u	buf[NUMBUFLEN + 2];
    long	reglen = 0;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	strregname = tv_get_string_chk(&argvars[0]);
	if (strregname == NULL)	    // type error; errmsg already given
	{
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = NULL;
	    return;
	}
    }
    else
	// Default to v:register
	strregname = get_vim_var_str(VV_REG);

    regname = (strregname == NULL ? '"' : *strregname);
    if (regname == 0)
	regname = '"';

    buf[0] = NUL;
    buf[1] = NUL;
    switch (get_reg_type(regname, &reglen))
    {
	case MLINE: buf[0] = 'V'; break;
	case MCHAR: buf[0] = 'v'; break;
	case MBLOCK:
		buf[0] = Ctrl_V;
		sprintf((char *)buf + 1, "%ld", reglen + 1);
		break;
    }
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave(buf);
}

/*
 * "gettagstack()" function
 */
    static void
f_gettagstack(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp = curwin;			// default is current window

    if (rettv_dict_alloc(rettv) != OK)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	wp = find_win_by_nr_or_id(&argvars[0]);
	if (wp == NULL)
	    return;
    }

    get_tagstack(wp, rettv->vval.v_dict);
}

// for VIM_VERSION_ defines
#include "version.h"

/*
 * "has()" function
 */
    static void
f_has(typval_T *argvars, typval_T *rettv)
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
#if defined(BSD) && !defined(MACOS_X)
	"bsd",
#endif
#ifdef hpux
	"hpux",
#endif
#ifdef __linux__
	"linux",
#endif
#ifdef MACOS_X
	"mac",		// Mac OS X (and, once, Mac OS Classic)
	"osx",		// Mac OS X
# ifdef MACOS_X_DARWIN
	"macunix",	// Mac OS X, with the darwin feature
	"osxdarwin",	// synonym for macunix
# endif
#endif
#ifdef __QNX__
	"qnx",
#endif
#ifdef SUN_SYSTEM
	"sun",
#else
	"moon",
#endif
#ifdef UNIX
	"unix",
#endif
#ifdef VMS
	"vms",
#endif
#ifdef MSWIN
	"win32",
#endif
#if defined(UNIX) && defined(__CYGWIN__)
	"win32unix",
#endif
#ifdef _WIN64
	"win64",
#endif
#ifdef EBCDIC
	"ebcdic",
#endif
#ifndef CASE_INSENSITIVE_FILENAME
	"fname_case",
#endif
#ifdef HAVE_ACL
	"acl",
#endif
#ifdef FEAT_ARABIC
	"arabic",
#endif
	"autocmd",
#ifdef FEAT_AUTOCHDIR
	"autochdir",
#endif
#ifdef FEAT_AUTOSERVERNAME
	"autoservername",
#endif
#ifdef FEAT_BEVAL_GUI
	"balloon_eval",
# ifndef FEAT_GUI_MSWIN // other GUIs always have multiline balloons
	"balloon_multiline",
# endif
#endif
#ifdef FEAT_BEVAL_TERM
	"balloon_eval_term",
#endif
#if defined(SOME_BUILTIN_TCAPS) || defined(ALL_BUILTIN_TCAPS)
	"builtin_terms",
# ifdef ALL_BUILTIN_TCAPS
	"all_builtin_terms",
# endif
#endif
#if defined(FEAT_BROWSE) && (defined(USE_FILE_CHOOSER) \
	|| defined(FEAT_GUI_MSWIN) \
	|| defined(FEAT_GUI_MOTIF))
	"browsefilter",
#endif
#ifdef FEAT_BYTEOFF
	"byte_offset",
#endif
#ifdef FEAT_JOB_CHANNEL
	"channel",
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
	"cmdline_compl",
	"cmdline_hist",
	"comments",
#ifdef FEAT_CONCEAL
	"conceal",
#endif
#ifdef FEAT_CRYPT
	"cryptv",
	"crypt-blowfish",
	"crypt-blowfish2",
#endif
#ifdef FEAT_CSCOPE
	"cscope",
#endif
	"cursorbind",
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
#ifdef FEAT_DIRECTX
	"directx",
#endif
#ifdef FEAT_DND
	"dnd",
#endif
#ifdef FEAT_EMACS_TAGS
	"emacs_tags",
#endif
	"eval",	    // always present, of course!
	"ex_extra", // graduated feature
#ifdef FEAT_SEARCH_EXTRA
	"extra_search",
#endif
#ifdef FEAT_SEARCHPATH
	"file_in_path",
#endif
#if defined(FEAT_FILTERPIPE) && !defined(VIMDLL)
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
# ifdef USE_GTK3
	"gui_gtk3",
# else
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
#ifdef FEAT_GUI_MSWIN
	"gui_win32",
#endif
#if defined(HAVE_ICONV_H) && defined(USE_ICONV)
	"iconv",
#endif
	"insert_expand",
#ifdef FEAT_JOB_CHANNEL
	"job",
#endif
#ifdef FEAT_JUMPLIST
	"jumplist",
#endif
#ifdef FEAT_KEYMAP
	"keymap",
#endif
	"lambda", // always with FEAT_EVAL, since 7.4.2120 with closure
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
	"listcmds",
	"localmap",
#ifdef FEAT_LUA
# ifndef DYNAMIC_LUA
	"lua",
# endif
#endif
#ifdef FEAT_MENU
	"menu",
#endif
#ifdef FEAT_SESSION
	"mksession",
#endif
	"modify_fname",
	"mouse",
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
	"mouse_sgr",
# endif
# ifdef FEAT_SYSMOUSE
	"mouse_sysmouse",
# endif
# ifdef FEAT_MOUSE_URXVT
	"mouse_urxvt",
# endif
# ifdef FEAT_MOUSE_XTERM
	"mouse_xterm",
# endif
#endif
	"multi_byte",
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
#ifdef FEAT_NUM64
	"num64",
#endif
#ifdef FEAT_OLE
	"ole",
#endif
#ifdef FEAT_EVAL
	"packages",
#endif
#ifdef FEAT_PATH_EXTRA
	"path_extra",
#endif
#ifdef FEAT_PERL
#ifndef DYNAMIC_PERL
	"perl",
#endif
#endif
#ifdef FEAT_PERSISTENT_UNDO
	"persistent_undo",
#endif
#if defined(FEAT_PYTHON)
	"python_compiled",
# if defined(DYNAMIC_PYTHON)
	"python_dynamic",
# else
	"python",
	"pythonx",
# endif
#endif
#if defined(FEAT_PYTHON3)
	"python3_compiled",
# if defined(DYNAMIC_PYTHON3)
	"python3_dynamic",
# else
	"python3",
	"pythonx",
# endif
#endif
#ifdef FEAT_PROP_POPUP
	"popupwin",
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
	"scrollbind",
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
#ifdef STARTUPTIME
	"startuptime",
#endif
#ifdef FEAT_STL_OPT
	"statusline",
#endif
#ifdef FEAT_NETBEANS_INTG
	"netbeans_intg",
#endif
#ifdef FEAT_SOUND
	"sound",
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
#ifdef FEAT_TCL
# ifndef DYNAMIC_TCL
	"tcl",
# endif
#endif
#ifdef FEAT_TERMGUICOLORS
	"termguicolors",
#endif
#if defined(FEAT_TERMINAL) && !defined(MSWIN)
	"terminal",
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
#ifdef FEAT_PROP_POPUP
	"textprop",
#endif
#ifdef HAVE_TGETENT
	"tgetent",
#endif
#ifdef FEAT_TIMERS
	"timers",
#endif
#ifdef FEAT_TITLE
	"title",
#endif
#ifdef FEAT_TOOLBAR
	"toolbar",
#endif
#if defined(FEAT_CLIPBOARD) && defined(FEAT_X11)
	"unnamedplus",
#endif
	"user-commands",    // was accidentally included in 5.4
	"user_commands",
#ifdef FEAT_VARTABS
	"vartabs",
#endif
	"vertsplit",
#ifdef FEAT_VIMINFO
	"viminfo",
#endif
	"vimscript-1",
	"vimscript-2",
	"vimscript-3",
	"vimscript-4",
	"virtualedit",
	"visual",
	"visualextra",
	"vreplace",
#ifdef FEAT_VTP
	"vtp",
#endif
#ifdef FEAT_WILDIGN
	"wildignore",
#endif
#ifdef FEAT_WILDMENU
	"wildmenu",
#endif
	"windows",
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
#ifdef FEAT_XPM_W32
	"xpm",
	"xpm_w32",	// for backward compatibility
#else
# if defined(HAVE_XPM)
	"xpm",
# endif
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

    name = tv_get_string(&argvars[0]);
    for (i = 0; has_list[i] != NULL; ++i)
	if (STRICMP(name, has_list[i]) == 0)
	{
	    n = TRUE;
	    break;
	}

    if (n == FALSE)
    {
	if (STRNICMP(name, "patch", 5) == 0)
	{
	    if (name[5] == '-'
		    && STRLEN(name) >= 11
		    && vim_isdigit(name[6])
		    && vim_isdigit(name[8])
		    && vim_isdigit(name[10]))
	    {
		int major = atoi((char *)name + 6);
		int minor = atoi((char *)name + 8);

		// Expect "patch-9.9.01234".
		n = (major < VIM_VERSION_MAJOR
		     || (major == VIM_VERSION_MAJOR
			 && (minor < VIM_VERSION_MINOR
			     || (minor == VIM_VERSION_MINOR
				 && has_patch(atoi((char *)name + 10))))));
	    }
	    else
		n = has_patch(atoi((char *)name + 5));
	}
	else if (STRICMP(name, "vim_starting") == 0)
	    n = (starting != 0);
	else if (STRICMP(name, "ttyin") == 0)
	    n = mch_input_isatty();
	else if (STRICMP(name, "ttyout") == 0)
	    n = stdout_isatty;
	else if (STRICMP(name, "multi_byte_encoding") == 0)
	    n = has_mbyte;
#if defined(FEAT_BEVAL) && defined(FEAT_GUI_MSWIN)
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
#ifdef DYNAMIC_LUA
	else if (STRICMP(name, "lua") == 0)
	    n = lua_enabled(FALSE);
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
#ifdef DYNAMIC_PYTHON3
	else if (STRICMP(name, "python3") == 0)
	    n = python3_enabled(FALSE);
#endif
#if defined(DYNAMIC_PYTHON) || defined(DYNAMIC_PYTHON3)
	else if (STRICMP(name, "pythonx") == 0)
	{
# if defined(DYNAMIC_PYTHON) && defined(DYNAMIC_PYTHON3)
	    if (p_pyx == 0)
		n = python3_enabled(FALSE) || python_enabled(FALSE);
	    else if (p_pyx == 3)
		n = python3_enabled(FALSE);
	    else if (p_pyx == 2)
		n = python_enabled(FALSE);
# elif defined(DYNAMIC_PYTHON)
	    n = python_enabled(FALSE);
# elif defined(DYNAMIC_PYTHON3)
	    n = python3_enabled(FALSE);
# endif
	}
#endif
#ifdef DYNAMIC_PERL
	else if (STRICMP(name, "perl") == 0)
	    n = perl_enabled(FALSE);
#endif
#ifdef FEAT_GUI
	else if (STRICMP(name, "gui_running") == 0)
	    n = (gui.in_use || gui.starting);
# ifdef FEAT_BROWSE
	else if (STRICMP(name, "browse") == 0)
	    n = gui.in_use;	// gui_mch_browse() works when GUI is running
# endif
#endif
#ifdef FEAT_SYN_HL
	else if (STRICMP(name, "syntax_items") == 0)
	    n = syntax_present(curwin);
#endif
#ifdef FEAT_VTP
	else if (STRICMP(name, "vcon") == 0)
	    n = is_term_win32() && has_vtp_working();
#endif
#ifdef FEAT_NETBEANS_INTG
	else if (STRICMP(name, "netbeans_enabled") == 0)
	    n = netbeans_active();
#endif
#ifdef FEAT_MOUSE_GPM
	else if (STRICMP(name, "mouse_gpm_enabled") == 0)
	    n = gpm_enabled();
#endif
#if defined(FEAT_TERMINAL) && defined(MSWIN)
	else if (STRICMP(name, "terminal") == 0)
	    n = terminal_enabled();
#endif
#if defined(FEAT_TERMINAL) && defined(MSWIN)
	else if (STRICMP(name, "conpty") == 0)
	    n = use_conpty();
#endif
#ifdef FEAT_CLIPBOARD
	else if (STRICMP(name, "clipboard_working") == 0)
	    n = clip_star.available;
#endif
#ifdef VIMDLL
	else if (STRICMP(name, "filterpipe") == 0)
	    n = gui.in_use || gui.starting;
#endif
    }

    rettv->vval.v_number = n;
}

/*
 * "haslocaldir()" function
 */
    static void
f_haslocaldir(typval_T *argvars, typval_T *rettv)
{
    tabpage_T	*tp = NULL;
    win_T	*wp = NULL;

    wp = find_tabwin(&argvars[0], &argvars[1], &tp);

    // Check for window-local and tab-local directories
    if (wp != NULL && wp->w_localdir != NULL)
	rettv->vval.v_number = 1;
    else if (tp != NULL && tp->tp_localdir != NULL)
	rettv->vval.v_number = 2;
    else
	rettv->vval.v_number = 0;
}

/*
 * "hasmapto()" function
 */
    static void
f_hasmapto(typval_T *argvars, typval_T *rettv)
{
    char_u	*name;
    char_u	*mode;
    char_u	buf[NUMBUFLEN];
    int		abbr = FALSE;

    name = tv_get_string(&argvars[0]);
    if (argvars[1].v_type == VAR_UNKNOWN)
	mode = (char_u *)"nvo";
    else
    {
	mode = tv_get_string_buf(&argvars[1], buf);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    abbr = (int)tv_get_number(&argvars[2]);
    }

    if (map_to_exists(name, mode, abbr))
	rettv->vval.v_number = TRUE;
    else
	rettv->vval.v_number = FALSE;
}

/*
 * "highlightID(name)" function
 */
    static void
f_hlID(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = syn_name2id(tv_get_string(&argvars[0]));
}

/*
 * "highlight_exists()" function
 */
    static void
f_hlexists(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = highlight_exists(tv_get_string(&argvars[0]));
}

/*
 * "hostname()" function
 */
    static void
f_hostname(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u hostname[256];

    mch_get_host_name(hostname, 256);
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave(hostname);
}

/*
 * iconv() function
 */
    static void
f_iconv(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*from, *to, *str;
    vimconv_T	vimconv;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    str = tv_get_string(&argvars[0]);
    from = enc_canonize(enc_skip(tv_get_string_buf(&argvars[1], buf1)));
    to = enc_canonize(enc_skip(tv_get_string_buf(&argvars[2], buf2)));
    vimconv.vc_type = CONV_NONE;
    convert_setup(&vimconv, from, to);

    // If the encodings are equal, no conversion needed.
    if (vimconv.vc_type == CONV_NONE)
	rettv->vval.v_string = vim_strsave(str);
    else
	rettv->vval.v_string = string_convert(&vimconv, str, NULL);

    convert_setup(&vimconv, NULL, NULL);
    vim_free(from);
    vim_free(to);
}

/*
 * "index()" function
 */
    static void
f_index(typval_T *argvars, typval_T *rettv)
{
    list_T	*l;
    listitem_T	*item;
    blob_T	*b;
    long	idx = 0;
    int		ic = FALSE;
    int		error = FALSE;

    rettv->vval.v_number = -1;
    if (argvars[0].v_type == VAR_BLOB)
    {
	typval_T	tv;
	int		start = 0;

	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    start = tv_get_number_chk(&argvars[2], &error);
	    if (error)
		return;
	}
	b = argvars[0].vval.v_blob;
	if (b == NULL)
	    return;
	if (start < 0)
	{
	    start = blob_len(b) + start;
	    if (start < 0)
		start = 0;
	}

	for (idx = start; idx < blob_len(b); ++idx)
	{
	    tv.v_type = VAR_NUMBER;
	    tv.vval.v_number = blob_get(b, idx);
	    if (tv_equal(&tv, &argvars[1], ic, FALSE))
	    {
		rettv->vval.v_number = idx;
		return;
	    }
	}
	return;
    }
    else if (argvars[0].v_type != VAR_LIST)
    {
	emsg(_(e_listblobreq));
	return;
    }

    l = argvars[0].vval.v_list;
    if (l != NULL)
    {
	item = l->lv_first;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    // Start at specified item.  Use the cached index that list_find()
	    // sets, so that a negative number also works.
	    item = list_find(l, (long)tv_get_number_chk(&argvars[2], &error));
	    idx = l->lv_idx;
	    if (argvars[3].v_type != VAR_UNKNOWN)
		ic = (int)tv_get_number_chk(&argvars[3], &error);
	    if (error)
		item = NULL;
	}

	for ( ; item != NULL; item = item->li_next, ++idx)
	    if (tv_equal(&item->li_tv, &argvars[1], ic, FALSE))
	    {
		rettv->vval.v_number = idx;
		break;
	    }
    }
}

static int inputsecret_flag = 0;

/*
 * "input()" function
 *     Also handles inputsecret() when inputsecret is set.
 */
    static void
f_input(typval_T *argvars, typval_T *rettv)
{
    get_user_input(argvars, rettv, FALSE, inputsecret_flag);
}

/*
 * "inputdialog()" function
 */
    static void
f_inputdialog(typval_T *argvars, typval_T *rettv)
{
#if defined(FEAT_GUI_TEXTDIALOG)
    // Use a GUI dialog if the GUI is running and 'c' is not in 'guioptions'
    if (gui.in_use && vim_strchr(p_go, GO_CONDIALOG) == NULL)
    {
	char_u	*message;
	char_u	buf[NUMBUFLEN];
	char_u	*defstr = (char_u *)"";

	message = tv_get_string_chk(&argvars[0]);
	if (argvars[1].v_type != VAR_UNKNOWN
		&& (defstr = tv_get_string_buf_chk(&argvars[1], buf)) != NULL)
	    vim_strncpy(IObuff, defstr, IOSIZE - 1);
	else
	    IObuff[0] = NUL;
	if (message != NULL && defstr != NULL
		&& do_dialog(VIM_QUESTION, NULL, message,
			  (char_u *)_("&OK\n&Cancel"), 1, IObuff, FALSE) == 1)
	    rettv->vval.v_string = vim_strsave(IObuff);
	else
	{
	    if (message != NULL && defstr != NULL
					&& argvars[1].v_type != VAR_UNKNOWN
					&& argvars[2].v_type != VAR_UNKNOWN)
		rettv->vval.v_string = vim_strsave(
				      tv_get_string_buf(&argvars[2], buf));
	    else
		rettv->vval.v_string = NULL;
	}
	rettv->v_type = VAR_STRING;
    }
    else
#endif
	get_user_input(argvars, rettv, TRUE, inputsecret_flag);
}

/*
 * "inputlist()" function
 */
    static void
f_inputlist(typval_T *argvars, typval_T *rettv)
{
    listitem_T	*li;
    int		selected;
    int		mouse_used;

#ifdef NO_CONSOLE_INPUT
    // While starting up, there is no place to enter text. When running tests
    // with --not-a-term we assume feedkeys() will be used.
    if (no_console_input() && !is_not_a_term())
	return;
#endif
    if (argvars[0].v_type != VAR_LIST || argvars[0].vval.v_list == NULL)
    {
	semsg(_(e_listarg), "inputlist()");
	return;
    }

    msg_start();
    msg_row = Rows - 1;	// for when 'cmdheight' > 1
    lines_left = Rows;	// avoid more prompt
    msg_scroll = TRUE;
    msg_clr_eos();

    for (li = argvars[0].vval.v_list->lv_first; li != NULL; li = li->li_next)
    {
	msg_puts((char *)tv_get_string(&li->li_tv));
	msg_putchar('\n');
    }

    // Ask for choice.
    selected = prompt_for_number(&mouse_used);
    if (mouse_used)
	selected -= lines_left;

    rettv->vval.v_number = selected;
}

static garray_T	    ga_userinput = {0, 0, sizeof(tasave_T), 4, NULL};

/*
 * "inputrestore()" function
 */
    static void
f_inputrestore(typval_T *argvars UNUSED, typval_T *rettv)
{
    if (ga_userinput.ga_len > 0)
    {
	--ga_userinput.ga_len;
	restore_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	// default return is zero == OK
    }
    else if (p_verbose > 1)
    {
	verb_msg(_("called inputrestore() more often than inputsave()"));
	rettv->vval.v_number = 1; // Failed
    }
}

/*
 * "inputsave()" function
 */
    static void
f_inputsave(typval_T *argvars UNUSED, typval_T *rettv)
{
    // Add an entry to the stack of typeahead storage.
    if (ga_grow(&ga_userinput, 1) == OK)
    {
	save_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	++ga_userinput.ga_len;
	// default return is zero == OK
    }
    else
	rettv->vval.v_number = 1; // Failed
}

/*
 * "inputsecret()" function
 */
    static void
f_inputsecret(typval_T *argvars, typval_T *rettv)
{
    ++cmdline_star;
    ++inputsecret_flag;
    f_input(argvars, rettv);
    --cmdline_star;
    --inputsecret_flag;
}

/*
 * "interrupt()" function
 */
    static void
f_interrupt(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    got_int = TRUE;
}

/*
 * "invert(expr)" function
 */
    static void
f_invert(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = ~tv_get_number_chk(&argvars[0], NULL);
}

/*
 * Return TRUE if typeval "tv" is locked: Either that value is locked itself
 * or it refers to a List or Dictionary that is locked.
 */
    static int
tv_islocked(typval_T *tv)
{
    return (tv->v_lock & VAR_LOCKED)
	|| (tv->v_type == VAR_LIST
		&& tv->vval.v_list != NULL
		&& (tv->vval.v_list->lv_lock & VAR_LOCKED))
	|| (tv->v_type == VAR_DICT
		&& tv->vval.v_dict != NULL
		&& (tv->vval.v_dict->dv_lock & VAR_LOCKED));
}

/*
 * "islocked()" function
 */
    static void
f_islocked(typval_T *argvars, typval_T *rettv)
{
    lval_T	lv;
    char_u	*end;
    dictitem_T	*di;

    rettv->vval.v_number = -1;
    end = get_lval(tv_get_string(&argvars[0]), NULL, &lv, FALSE, FALSE,
			     GLV_NO_AUTOLOAD | GLV_READ_ONLY, FNE_CHECK_START);
    if (end != NULL && lv.ll_name != NULL)
    {
	if (*end != NUL)
	    emsg(_(e_trailing));
	else
	{
	    if (lv.ll_tv == NULL)
	    {
		di = find_var(lv.ll_name, NULL, TRUE);
		if (di != NULL)
		{
		    // Consider a variable locked when:
		    // 1. the variable itself is locked
		    // 2. the value of the variable is locked.
		    // 3. the List or Dict value is locked.
		    rettv->vval.v_number = ((di->di_flags & DI_FLAGS_LOCK)
						   || tv_islocked(&di->di_tv));
		}
	    }
	    else if (lv.ll_range)
		emsg(_("E786: Range not allowed"));
	    else if (lv.ll_newkey != NULL)
		semsg(_(e_dictkey), lv.ll_newkey);
	    else if (lv.ll_list != NULL)
		// List item.
		rettv->vval.v_number = tv_islocked(&lv.ll_li->li_tv);
	    else
		// Dictionary item.
		rettv->vval.v_number = tv_islocked(&lv.ll_di->di_tv);
	}
    }

    clear_lval(&lv);
}

#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
/*
 * "isinf()" function
 */
    static void
f_isinf(typval_T *argvars, typval_T *rettv)
{
    if (argvars[0].v_type == VAR_FLOAT && isinf(argvars[0].vval.v_float))
	rettv->vval.v_number = argvars[0].vval.v_float > 0.0 ? 1 : -1;
}

/*
 * "isnan()" function
 */
    static void
f_isnan(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = argvars[0].v_type == VAR_FLOAT
					    && isnan(argvars[0].vval.v_float);
}
#endif

/*
 * "last_buffer_nr()" function.
 */
    static void
f_last_buffer_nr(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		n = 0;
    buf_T	*buf;

    FOR_ALL_BUFFERS(buf)
	if (n < buf->b_fnum)
	    n = buf->b_fnum;

    rettv->vval.v_number = n;
}

/*
 * "len()" function
 */
    static void
f_len(typval_T *argvars, typval_T *rettv)
{
    switch (argvars[0].v_type)
    {
	case VAR_STRING:
	case VAR_NUMBER:
	    rettv->vval.v_number = (varnumber_T)STRLEN(
					       tv_get_string(&argvars[0]));
	    break;
	case VAR_BLOB:
	    rettv->vval.v_number = blob_len(argvars[0].vval.v_blob);
	    break;
	case VAR_LIST:
	    rettv->vval.v_number = list_len(argvars[0].vval.v_list);
	    break;
	case VAR_DICT:
	    rettv->vval.v_number = dict_len(argvars[0].vval.v_dict);
	    break;
	case VAR_UNKNOWN:
	case VAR_VOID:
	case VAR_BOOL:
	case VAR_SPECIAL:
	case VAR_FLOAT:
	case VAR_FUNC:
	case VAR_PARTIAL:
	case VAR_JOB:
	case VAR_CHANNEL:
	    emsg(_("E701: Invalid type for len()"));
	    break;
    }
}

    static void
libcall_common(typval_T *argvars UNUSED, typval_T *rettv, int type)
{
#ifdef FEAT_LIBCALL
    char_u		*string_in;
    char_u		**string_result;
    int			nr_result;
#endif

    rettv->v_type = type;
    if (type != VAR_NUMBER)
	rettv->vval.v_string = NULL;

    if (check_restricted() || check_secure())
	return;

#ifdef FEAT_LIBCALL
    // The first two args must be strings, otherwise it's meaningless
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
f_libcall(typval_T *argvars, typval_T *rettv)
{
    libcall_common(argvars, rettv, VAR_STRING);
}

/*
 * "libcallnr()" function
 */
    static void
f_libcallnr(typval_T *argvars, typval_T *rettv)
{
    libcall_common(argvars, rettv, VAR_NUMBER);
}

/*
 * "line(string, [winid])" function
 */
    static void
f_line(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum = 0;
    pos_T	*fp = NULL;
    int		fnum;
    int		id;
    tabpage_T	*tp;
    win_T	*wp;
    win_T	*save_curwin;
    tabpage_T	*save_curtab;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	// use window specified in the second argument
	id = (int)tv_get_number(&argvars[1]);
	wp = win_id2wp_tp(id, &tp);
	if (wp != NULL && tp != NULL)
	{
	    if (switch_win_noblock(&save_curwin, &save_curtab, wp, tp, TRUE)
									 == OK)
	    {
		check_cursor();
		fp = var2fpos(&argvars[0], TRUE, &fnum);
	    }
	    restore_win_noblock(save_curwin, save_curtab, TRUE);
	}
    }
    else
	// use current window
	fp = var2fpos(&argvars[0], TRUE, &fnum);

    if (fp != NULL)
	lnum = fp->lnum;
    rettv->vval.v_number = lnum;
}

/*
 * "line2byte(lnum)" function
 */
    static void
f_line2byte(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifndef FEAT_BYTEOFF
    rettv->vval.v_number = -1;
#else
    linenr_T	lnum;

    lnum = tv_get_lnum(argvars);
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count + 1)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = ml_find_line_or_offset(curbuf, lnum, NULL);
    if (rettv->vval.v_number >= 0)
	++rettv->vval.v_number;
#endif
}

/*
 * "localtime()" function
 */
    static void
f_localtime(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = (varnumber_T)time(NULL);
}

#ifdef FEAT_FLOAT
/*
 * "log()" function
 */
    static void
f_log(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = log(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "log10()" function
 */
    static void
f_log10(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = log10(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

#ifdef FEAT_LUA
/*
 * "luaeval()" function
 */
    static void
f_luaeval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;

    str = tv_get_string_buf(&argvars[0], buf);
    do_luaeval(str, argvars + 1, rettv);
}
#endif

/*
 * "maparg()" function
 */
    static void
f_maparg(typval_T *argvars, typval_T *rettv)
{
    get_maparg(argvars, rettv, TRUE);
}

/*
 * "mapcheck()" function
 */
    static void
f_mapcheck(typval_T *argvars, typval_T *rettv)
{
    get_maparg(argvars, rettv, FALSE);
}

typedef enum
{
    MATCH_END,	    // matchend()
    MATCH_MATCH,    // match()
    MATCH_STR,	    // matchstr()
    MATCH_LIST,	    // matchlist()
    MATCH_POS	    // matchstrpos()
} matchtype_T;

    static void
find_some_match(typval_T *argvars, typval_T *rettv, matchtype_T type)
{
    char_u	*str = NULL;
    long	len = 0;
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

    // Make 'cpoptions' empty, the 'l' flag should not be used here.
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    rettv->vval.v_number = -1;
    if (type == MATCH_LIST || type == MATCH_POS)
    {
	// type MATCH_LIST: return empty list when there are no matches.
	// type MATCH_POS: return ["", -1, -1, -1]
	if (rettv_list_alloc(rettv) == FAIL)
	    goto theend;
	if (type == MATCH_POS
		&& (list_append_string(rettv->vval.v_list,
					    (char_u *)"", 0) == FAIL
		    || list_append_number(rettv->vval.v_list,
					    (varnumber_T)-1) == FAIL
		    || list_append_number(rettv->vval.v_list,
					    (varnumber_T)-1) == FAIL
		    || list_append_number(rettv->vval.v_list,
					    (varnumber_T)-1) == FAIL))
	{
		list_free(rettv->vval.v_list);
		rettv->vval.v_list = NULL;
		goto theend;
	}
    }
    else if (type == MATCH_STR)
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
    {
	expr = str = tv_get_string(&argvars[0]);
	len = (long)STRLEN(str);
    }

    pat = tv_get_string_buf_chk(&argvars[1], patbuf);
    if (pat == NULL)
	goto theend;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	int	    error = FALSE;

	start = (long)tv_get_number_chk(&argvars[2], &error);
	if (error)
	    goto theend;
	if (l != NULL)
	{
	    li = list_find(l, start);
	    if (li == NULL)
		goto theend;
	    idx = l->lv_idx;	// use the cached index
	}
	else
	{
	    if (start < 0)
		start = 0;
	    if (start > len)
		goto theend;
	    // When "count" argument is there ignore matches before "start",
	    // otherwise skip part of the string.  Differs when pattern is "^"
	    // or "\<".
	    if (argvars[3].v_type != VAR_UNKNOWN)
		startcol = start;
	    else
	    {
		str += start;
		len -= start;
	    }
	}

	if (argvars[3].v_type != VAR_UNKNOWN)
	    nth = (long)tv_get_number_chk(&argvars[3], &error);
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
		expr = str = echo_string(&li->li_tv, &tofree, strbuf, 0);
		if (str == NULL)
		    break;
	    }

	    match = vim_regexec_nl(&regmatch, str, (colnr_T)startcol);

	    if (match && --nth <= 0)
		break;
	    if (l == NULL && !match)
		break;

	    // Advance to just after the match.
	    if (l != NULL)
	    {
		li = li->li_next;
		++idx;
	    }
	    else
	    {
		startcol = (colnr_T)(regmatch.startp[0]
				    + (*mb_ptr2len)(regmatch.startp[0]) - str);
		if (startcol > (colnr_T)len
				      || str + startcol <= regmatch.startp[0])
		{
		    match = FALSE;
		    break;
		}
	    }
	}

	if (match)
	{
	    if (type == MATCH_POS)
	    {
		listitem_T *li1 = rettv->vval.v_list->lv_first;
		listitem_T *li2 = li1->li_next;
		listitem_T *li3 = li2->li_next;
		listitem_T *li4 = li3->li_next;

		vim_free(li1->li_tv.vval.v_string);
		li1->li_tv.vval.v_string = vim_strnsave(regmatch.startp[0],
				(int)(regmatch.endp[0] - regmatch.startp[0]));
		li3->li_tv.vval.v_number =
				      (varnumber_T)(regmatch.startp[0] - expr);
		li4->li_tv.vval.v_number =
					(varnumber_T)(regmatch.endp[0] - expr);
		if (l != NULL)
		    li2->li_tv.vval.v_number = (varnumber_T)idx;
	    }
	    else if (type == MATCH_LIST)
	    {
		int i;

		// return list with matched string and submatches
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
	    else if (type == MATCH_STR)
	    {
		// return matched string
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
		if (type != MATCH_END)
		    rettv->vval.v_number =
				      (varnumber_T)(regmatch.startp[0] - str);
		else
		    rettv->vval.v_number =
					(varnumber_T)(regmatch.endp[0] - str);
		rettv->vval.v_number += (varnumber_T)(str - expr);
	    }
	}
	vim_regfree(regmatch.regprog);
    }

theend:
    if (type == MATCH_POS && l == NULL && rettv->vval.v_list != NULL)
	// matchstrpos() without a list: drop the second item.
	listitem_remove(rettv->vval.v_list,
				       rettv->vval.v_list->lv_first->li_next);
    vim_free(tofree);
    p_cpo = save_cpo;
}

/*
 * "match()" function
 */
    static void
f_match(typval_T *argvars, typval_T *rettv)
{
    find_some_match(argvars, rettv, MATCH_MATCH);
}

/*
 * "matchend()" function
 */
    static void
f_matchend(typval_T *argvars, typval_T *rettv)
{
    find_some_match(argvars, rettv, MATCH_END);
}

/*
 * "matchlist()" function
 */
    static void
f_matchlist(typval_T *argvars, typval_T *rettv)
{
    find_some_match(argvars, rettv, MATCH_LIST);
}

/*
 * "matchstr()" function
 */
    static void
f_matchstr(typval_T *argvars, typval_T *rettv)
{
    find_some_match(argvars, rettv, MATCH_STR);
}

/*
 * "matchstrpos()" function
 */
    static void
f_matchstrpos(typval_T *argvars, typval_T *rettv)
{
    find_some_match(argvars, rettv, MATCH_POS);
}

    static void
max_min(typval_T *argvars, typval_T *rettv, int domax)
{
    varnumber_T	n = 0;
    varnumber_T	i;
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
		n = tv_get_number_chk(&li->li_tv, &error);
		for (;;)
		{
		    li = li->li_next;
		    if (li == NULL)
			break;
		    i = tv_get_number_chk(&li->li_tv, &error);
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
		    i = tv_get_number_chk(&HI2DI(hi)->di_tv, &error);
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
	semsg(_(e_listdictarg), domax ? "max()" : "min()");
    rettv->vval.v_number = error ? 0 : n;
}

/*
 * "max()" function
 */
    static void
f_max(typval_T *argvars, typval_T *rettv)
{
    max_min(argvars, rettv, TRUE);
}

/*
 * "min()" function
 */
    static void
f_min(typval_T *argvars, typval_T *rettv)
{
    max_min(argvars, rettv, FALSE);
}

#if defined(FEAT_MZSCHEME) || defined(PROTO)
/*
 * "mzeval()" function
 */
    static void
f_mzeval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;
    str = tv_get_string_buf(&argvars[0], buf);
    do_mzeval(str, rettv);
}

    void
mzscheme_call_vim(char_u *name, typval_T *args, typval_T *rettv)
{
    typval_T argvars[3];

    argvars[0].v_type = VAR_STRING;
    argvars[0].vval.v_string = name;
    copy_tv(args, &argvars[1]);
    argvars[2].v_type = VAR_UNKNOWN;
    f_call(argvars, rettv);
    clear_tv(&argvars[1]);
}
#endif

/*
 * "nextnonblank()" function
 */
    static void
f_nextnonblank(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;

    for (lnum = tv_get_lnum(argvars); ; ++lnum)
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
f_nr2char(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];

    if (has_mbyte)
    {
	int	utf8 = 0;

	if (argvars[1].v_type != VAR_UNKNOWN)
	    utf8 = (int)tv_get_number_chk(&argvars[1], NULL);
	if (utf8)
	    buf[utf_char2bytes((int)tv_get_number(&argvars[0]), buf)] = NUL;
	else
	    buf[(*mb_char2bytes)((int)tv_get_number(&argvars[0]), buf)] = NUL;
    }
    else
    {
	buf[0] = (char_u)tv_get_number(&argvars[0]);
	buf[1] = NUL;
    }
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave(buf);
}

/*
 * "or(expr, expr)" function
 */
    static void
f_or(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = tv_get_number_chk(&argvars[0], NULL)
					| tv_get_number_chk(&argvars[1], NULL);
}

#ifdef FEAT_PERL
/*
 * "perleval()" function
 */
    static void
f_perleval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    str = tv_get_string_buf(&argvars[0], buf);
    do_perleval(str, rettv);
}
#endif

#ifdef FEAT_FLOAT
/*
 * "pow()" function
 */
    static void
f_pow(typval_T *argvars, typval_T *rettv)
{
    float_T	fx = 0.0, fy = 0.0;

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
f_prevnonblank(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;

    lnum = tv_get_lnum(argvars);
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count)
	lnum = 0;
    else
	while (lnum >= 1 && *skipwhite(ml_get(lnum)) == NUL)
	    --lnum;
    rettv->vval.v_number = lnum;
}

// This dummy va_list is here because:
// - passing a NULL pointer doesn't work when va_list isn't a pointer
// - locally in the function results in a "used before set" warning
// - using va_start() to initialize it gives "function with fixed args" error
static va_list	ap;

/*
 * "printf()" function
 */
    static void
f_printf(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];
    int		len;
    char_u	*s;
    int		saved_did_emsg = did_emsg;
    char	*fmt;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    // Get the required length, allocate the buffer and do it for real.
    did_emsg = FALSE;
    fmt = (char *)tv_get_string_buf(&argvars[0], buf);
    len = vim_vsnprintf_typval(NULL, 0, fmt, ap, argvars + 1);
    if (!did_emsg)
    {
	s = alloc(len + 1);
	if (s != NULL)
	{
	    rettv->vval.v_string = s;
	    (void)vim_vsnprintf_typval((char *)s, len + 1, fmt,
							      ap, argvars + 1);
	}
    }
    did_emsg |= saved_did_emsg;
}

/*
 * "pum_getpos()" function
 */
    static void
f_pum_getpos(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    if (rettv_dict_alloc(rettv) != OK)
	return;
    pum_set_event_info(rettv->vval.v_dict);
}

/*
 * "pumvisible()" function
 */
    static void
f_pumvisible(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    if (pum_visible())
	rettv->vval.v_number = 1;
}

#ifdef FEAT_PYTHON3
/*
 * "py3eval()" function
 */
    static void
f_py3eval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;

    if (p_pyx == 0)
	p_pyx = 3;

    str = tv_get_string_buf(&argvars[0], buf);
    do_py3eval(str, rettv);
}
#endif

#ifdef FEAT_PYTHON
/*
 * "pyeval()" function
 */
    static void
f_pyeval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;

    if (p_pyx == 0)
	p_pyx = 2;

    str = tv_get_string_buf(&argvars[0], buf);
    do_pyeval(str, rettv);
}
#endif

#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
/*
 * "pyxeval()" function
 */
    static void
f_pyxeval(typval_T *argvars, typval_T *rettv)
{
    if (check_restricted() || check_secure())
	return;

# if defined(FEAT_PYTHON) && defined(FEAT_PYTHON3)
    init_pyxversion();
    if (p_pyx == 2)
	f_pyeval(argvars, rettv);
    else
	f_py3eval(argvars, rettv);
# elif defined(FEAT_PYTHON)
    f_pyeval(argvars, rettv);
# elif defined(FEAT_PYTHON3)
    f_py3eval(argvars, rettv);
# endif
}
#endif

/*
 * "rand()" function
 */
    static void
f_rand(typval_T *argvars, typval_T *rettv)
{
    list_T	*l = NULL;
    static list_T *globl = NULL;
    UINT32_T	x, y, z, w, t, result;
    listitem_T	*lx, *ly, *lz, *lw;

    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	// When no argument is given use the global seed list.
	if (globl == NULL)
	{
	    // Initialize the global seed list.
	    f_srand(argvars, rettv);
	    l = rettv->vval.v_list;
	    if (l == NULL || list_len(l) != 4)
	    {
		clear_tv(rettv);
		goto theend;
	    }
	    globl = l;
	}
	else
	    l = globl;
    }
    else if (argvars[0].v_type == VAR_LIST)
    {
	l = argvars[0].vval.v_list;
	if (l == NULL || list_len(l) != 4)
	    goto theend;
    }
    else
	goto theend;

    lx = list_find(l, 0L);
    ly = list_find(l, 1L);
    lz = list_find(l, 2L);
    lw = list_find(l, 3L);
    if (lx->li_tv.v_type != VAR_NUMBER) goto theend;
    if (ly->li_tv.v_type != VAR_NUMBER) goto theend;
    if (lz->li_tv.v_type != VAR_NUMBER) goto theend;
    if (lw->li_tv.v_type != VAR_NUMBER) goto theend;
    x = (UINT32_T)lx->li_tv.vval.v_number;
    y = (UINT32_T)ly->li_tv.vval.v_number;
    z = (UINT32_T)lz->li_tv.vval.v_number;
    w = (UINT32_T)lw->li_tv.vval.v_number;

    // SHUFFLE_XOSHIRO128STARSTAR
#define ROTL(x, k) ((x << k) | (x >> (32 - k)))
    result = ROTL(y * 5, 7) * 9;
    t = y << 9;
    z ^= x;
    w ^= y;
    y ^= z, x ^= w;
    z ^= t;
    w = ROTL(w, 11);
#undef ROTL

    lx->li_tv.vval.v_number = (varnumber_T)x;
    ly->li_tv.vval.v_number = (varnumber_T)y;
    lz->li_tv.vval.v_number = (varnumber_T)z;
    lw->li_tv.vval.v_number = (varnumber_T)w;

    rettv->v_type = VAR_NUMBER;
    rettv->vval.v_number = (varnumber_T)result;
    return;

theend:
    semsg(_(e_invarg2), tv_get_string(&argvars[0]));
    rettv->v_type = VAR_NUMBER;
    rettv->vval.v_number = -1;
}

/*
 * "range()" function
 */
    static void
f_range(typval_T *argvars, typval_T *rettv)
{
    varnumber_T	start;
    varnumber_T	end;
    varnumber_T	stride = 1;
    int		error = FALSE;

    start = tv_get_number_chk(&argvars[0], &error);
    if (argvars[1].v_type == VAR_UNKNOWN)
    {
	end = start - 1;
	start = 0;
    }
    else
    {
	end = tv_get_number_chk(&argvars[1], &error);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    stride = tv_get_number_chk(&argvars[2], &error);
    }

    if (error)
	return;		// type error; errmsg already given
    if (stride == 0)
	emsg(_("E726: Stride is zero"));
    else if (stride > 0 ? end + 1 < start : end - 1 > start)
	emsg(_("E727: Start past end"));
    else if (rettv_list_alloc(rettv) == OK)
    {
	list_T *list = rettv->vval.v_list;

	// Create a non-materialized list.  This is much more efficient and
	// works with ":for".  If used otherwise range_list_materialize() must
	// be called.
	list->lv_first = &range_list_item;
	list->lv_start = start;
	list->lv_end = end;
	list->lv_stride = stride;
	list->lv_len = (end - start + 1) / stride;
    }
}

/*
 * If "list" is a non-materialized list then materialize it now.
 */
    void
range_list_materialize(list_T *list)
{
    if (list->lv_first == &range_list_item)
    {
	varnumber_T start = list->lv_start;
	varnumber_T end = list->lv_end;
	int	    stride = list->lv_stride;
	varnumber_T i;

	list->lv_first = NULL;
	list->lv_last = NULL;
	list->lv_len = 0;
	list->lv_idx_item = NULL;
	for (i = start; stride > 0 ? i <= end : i >= end; i += stride)
	    if (list_append_number(list, (varnumber_T)i) == FAIL)
		break;
    }
}

    static void
return_register(int regname, typval_T *rettv)
{
    char_u buf[2] = {0, 0};

    buf[0] = (char_u)regname;
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave(buf);
}

/*
 * "reg_executing()" function
 */
    static void
f_reg_executing(typval_T *argvars UNUSED, typval_T *rettv)
{
    return_register(reg_executing, rettv);
}

/*
 * "reg_recording()" function
 */
    static void
f_reg_recording(typval_T *argvars UNUSED, typval_T *rettv)
{
    return_register(reg_recording, rettv);
}

#if defined(FEAT_RELTIME)
/*
 * Convert a List to proftime_T.
 * Return FAIL when there is something wrong.
 */
    static int
list2proftime(typval_T *arg, proftime_T *tm)
{
    long	n1, n2;
    int	error = FALSE;

    if (arg->v_type != VAR_LIST || arg->vval.v_list == NULL
					     || arg->vval.v_list->lv_len != 2)
	return FAIL;
    n1 = list_find_nr(arg->vval.v_list, 0L, &error);
    n2 = list_find_nr(arg->vval.v_list, 1L, &error);
# ifdef MSWIN
    tm->HighPart = n1;
    tm->LowPart = n2;
# else
    tm->tv_sec = n1;
    tm->tv_usec = n2;
# endif
    return error ? FAIL : OK;
}
#endif // FEAT_RELTIME

/*
 * "reltime()" function
 */
    static void
f_reltime(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_RELTIME
    proftime_T	res;
    proftime_T	start;

    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	// No arguments: get current time.
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
	// Two arguments: compute the difference.
	if (list2proftime(&argvars[0], &start) == FAIL
		|| list2proftime(&argvars[1], &res) == FAIL)
	    return;
	profile_sub(&res, &start);
    }

    if (rettv_list_alloc(rettv) == OK)
    {
	long	n1, n2;

# ifdef MSWIN
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

#ifdef FEAT_FLOAT
/*
 * "reltimefloat()" function
 */
    static void
f_reltimefloat(typval_T *argvars UNUSED, typval_T *rettv)
{
# ifdef FEAT_RELTIME
    proftime_T	tm;
# endif

    rettv->v_type = VAR_FLOAT;
    rettv->vval.v_float = 0;
# ifdef FEAT_RELTIME
    if (list2proftime(&argvars[0], &tm) == OK)
	rettv->vval.v_float = profile_float(&tm);
# endif
}
#endif

/*
 * "reltimestr()" function
 */
    static void
f_reltimestr(typval_T *argvars UNUSED, typval_T *rettv)
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
    static void
make_connection(void)
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
check_connection(void)
{
    make_connection();
    if (X_DISPLAY == NULL)
    {
	emsg(_("E240: No connection to the X server"));
	return FAIL;
    }
    return OK;
}
#endif

#ifdef FEAT_CLIENTSERVER
    static void
remote_common(typval_T *argvars, typval_T *rettv, int expr)
{
    char_u	*server_name;
    char_u	*keys;
    char_u	*r = NULL;
    char_u	buf[NUMBUFLEN];
    int		timeout = 0;
# ifdef MSWIN
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
    if (argvars[2].v_type != VAR_UNKNOWN
	    && argvars[3].v_type != VAR_UNKNOWN)
	timeout = tv_get_number(&argvars[3]);

    server_name = tv_get_string_chk(&argvars[0]);
    if (server_name == NULL)
	return;		// type error; errmsg already given
    keys = tv_get_string_buf(&argvars[1], buf);
# ifdef MSWIN
    if (serverSendToVim(server_name, keys, &r, &w, expr, timeout, TRUE) < 0)
# else
    if (serverSendToVim(X_DISPLAY, server_name, keys, &r, &w, expr, timeout,
								  0, TRUE) < 0)
# endif
    {
	if (r != NULL)
	{
	    emsg((char *)r);	// sending worked but evaluation failed
	    vim_free(r);
	}
	else
	    semsg(_("E241: Unable to send to %s"), server_name);
	return;
    }

    rettv->vval.v_string = r;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	dictitem_T	v;
	char_u		str[30];
	char_u		*idvar;

	idvar = tv_get_string_chk(&argvars[2]);
	if (idvar != NULL && *idvar != NUL)
	{
	    sprintf((char *)str, PRINTF_HEX_LONG_U, (long_u)w);
	    v.di_tv.v_type = VAR_STRING;
	    v.di_tv.vval.v_string = vim_strsave(str);
	    set_var(idvar, &v.di_tv, FALSE);
	    vim_free(v.di_tv.vval.v_string);
	}
    }
}
#endif

/*
 * "remote_expr()" function
 */
    static void
f_remote_expr(typval_T *argvars UNUSED, typval_T *rettv)
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
    static void
f_remote_foreground(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_CLIENTSERVER
# ifdef MSWIN
    // On Win32 it's done in this application.
    {
	char_u	*server_name = tv_get_string_chk(&argvars[0]);

	if (server_name != NULL)
	    serverForeground(server_name);
    }
# else
    // Send a foreground() expression to the server.
    argvars[1].v_type = VAR_STRING;
    argvars[1].vval.v_string = vim_strsave((char_u *)"foreground()");
    argvars[2].v_type = VAR_UNKNOWN;
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    remote_common(argvars, rettv, TRUE);
    vim_free(argvars[1].vval.v_string);
# endif
#endif
}

    static void
f_remote_peek(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_CLIENTSERVER
    dictitem_T	v;
    char_u	*s = NULL;
# ifdef MSWIN
    long_u	n = 0;
# endif
    char_u	*serverid;

    if (check_restricted() || check_secure())
    {
	rettv->vval.v_number = -1;
	return;
    }
    serverid = tv_get_string_chk(&argvars[0]);
    if (serverid == NULL)
    {
	rettv->vval.v_number = -1;
	return;		// type error; errmsg already given
    }
# ifdef MSWIN
    sscanf((const char *)serverid, SCANF_HEX_LONG_U, &n);
    if (n == 0)
	rettv->vval.v_number = -1;
    else
    {
	s = serverGetReply((HWND)n, FALSE, FALSE, FALSE, 0);
	rettv->vval.v_number = (s != NULL);
    }
# else
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
	retvar = tv_get_string_chk(&argvars[1]);
	if (retvar != NULL)
	    set_var(retvar, &v.di_tv, FALSE);
	vim_free(v.di_tv.vval.v_string);
    }
#else
    rettv->vval.v_number = -1;
#endif
}

    static void
f_remote_read(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	*r = NULL;

#ifdef FEAT_CLIENTSERVER
    char_u	*serverid = tv_get_string_chk(&argvars[0]);

    if (serverid != NULL && !check_restricted() && !check_secure())
    {
	int timeout = 0;
# ifdef MSWIN
	// The server's HWND is encoded in the 'id' parameter
	long_u		n = 0;
# endif

	if (argvars[1].v_type != VAR_UNKNOWN)
	    timeout = tv_get_number(&argvars[1]);

# ifdef MSWIN
	sscanf((char *)serverid, SCANF_HEX_LONG_U, &n);
	if (n != 0)
	    r = serverGetReply((HWND)n, FALSE, TRUE, TRUE, timeout);
	if (r == NULL)
# else
	if (check_connection() == FAIL
		|| serverReadReply(X_DISPLAY, serverStrToWin(serverid),
						       &r, FALSE, timeout) < 0)
# endif
	    emsg(_("E277: Unable to read a server reply"));
    }
#endif
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = r;
}

/*
 * "remote_send()" function
 */
    static void
f_remote_send(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_CLIENTSERVER
    remote_common(argvars, rettv, FALSE);
#endif
}

/*
 * "remote_startserver()" function
 */
    static void
f_remote_startserver(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_CLIENTSERVER
    char_u	*server = tv_get_string_chk(&argvars[0]);

    if (server == NULL)
	return;		// type error; errmsg already given
    if (serverName != NULL)
	emsg(_("E941: already started a server"));
    else
    {
# ifdef FEAT_X11
	if (check_connection() == OK)
	    serverRegisterName(X_DISPLAY, server);
# else
	serverSetName(server);
# endif
    }
#else
    emsg(_("E942: +clientserver feature not available"));
#endif
}

/*
 * "rename({from}, {to})" function
 */
    static void
f_rename(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = vim_rename(tv_get_string(&argvars[0]),
				      tv_get_string_buf(&argvars[1], buf));
}

/*
 * "repeat()" function
 */
    static void
f_repeat(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		n;
    int		slen;
    int		len;
    char_u	*r;
    int		i;

    n = (int)tv_get_number(&argvars[1]);
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
	p = tv_get_string(&argvars[0]);
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

#define SP_NOMOVE	0x01	    // don't move cursor
#define SP_REPEAT	0x02	    // repeat to find outer pair
#define SP_RETCOUNT	0x04	    // return matchcount
#define SP_SETPCMARK	0x08	    // set previous context mark
#define SP_START	0x10	    // accept match at start position
#define SP_SUBPAT	0x20	    // return nr of matching sub-pattern
#define SP_END		0x40	    // leave cursor at end of match
#define SP_COLUMN	0x80	    // start at cursor column

/*
 * Get flags for a search function.
 * Possibly sets "p_ws".
 * Returns BACKWARD, FORWARD or zero (for an error).
 */
    static int
get_search_arg(typval_T *varp, int *flagsp)
{
    int		dir = FORWARD;
    char_u	*flags;
    char_u	nbuf[NUMBUFLEN];
    int		mask;

    if (varp->v_type != VAR_UNKNOWN)
    {
	flags = tv_get_string_buf_chk(varp, nbuf);
	if (flags == NULL)
	    return 0;		// type error; errmsg already given
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
				 case 'z': mask = SP_COLUMN; break;
			     }
			  if (mask == 0)
			  {
			      semsg(_(e_invarg2), flags);
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
 * Shared by search() and searchpos() functions.
 */
    static int
search_cmn(typval_T *argvars, pos_T *match_pos, int *flagsp)
{
    int		flags;
    char_u	*pat;
    pos_T	pos;
    pos_T	save_cursor;
    int		save_p_ws = p_ws;
    int		dir;
    int		retval = 0;	// default: FAIL
    long	lnum_stop = 0;
#ifdef FEAT_RELTIME
    proftime_T	tm;
    long	time_limit = 0;
#endif
    int		options = SEARCH_KEEP;
    int		subpatnum;
    searchit_arg_T sia;

    pat = tv_get_string(&argvars[0]);
    dir = get_search_arg(&argvars[1], flagsp);	// may set p_ws
    if (dir == 0)
	goto theend;
    flags = *flagsp;
    if (flags & SP_START)
	options |= SEARCH_START;
    if (flags & SP_END)
	options |= SEARCH_END;
    if (flags & SP_COLUMN)
	options |= SEARCH_COL;

    // Optional arguments: line number to stop searching and timeout.
    if (argvars[1].v_type != VAR_UNKNOWN && argvars[2].v_type != VAR_UNKNOWN)
    {
	lnum_stop = (long)tv_get_number_chk(&argvars[2], NULL);
	if (lnum_stop < 0)
	    goto theend;
#ifdef FEAT_RELTIME
	if (argvars[3].v_type != VAR_UNKNOWN)
	{
	    time_limit = (long)tv_get_number_chk(&argvars[3], NULL);
	    if (time_limit < 0)
		goto theend;
	}
#endif
    }

#ifdef FEAT_RELTIME
    // Set the time limit, if there is one.
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
	semsg(_(e_invarg2), tv_get_string(&argvars[1]));
	goto theend;
    }

    pos = save_cursor = curwin->w_cursor;
    vim_memset(&sia, 0, sizeof(sia));
    sia.sa_stop_lnum = (linenr_T)lnum_stop;
#ifdef FEAT_RELTIME
    sia.sa_tm = &tm;
#endif
    subpatnum = searchit(curwin, curbuf, &pos, NULL, dir, pat, 1L,
						     options, RE_SEARCH, &sia);
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
	    // Store the match cursor position
	    match_pos->lnum = pos.lnum;
	    match_pos->col = pos.col + 1;
	}
	// "/$" will put the cursor after the end of the line, may need to
	// correct that here
	check_cursor();
    }

    // If 'n' flag is used: restore cursor position.
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
 * round() is not in C90, use ceil() or floor() instead.
 */
    float_T
vim_round(float_T f)
{
    return f > 0 ? floor(f + 0.5) : ceil(f - 0.5);
}

/*
 * "round({float})" function
 */
    static void
f_round(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = vim_round(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

#ifdef FEAT_RUBY
/*
 * "rubyeval()" function
 */
    static void
f_rubyeval(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    char_u	buf[NUMBUFLEN];

    str = tv_get_string_buf(&argvars[0], buf);
    do_rubyeval(str, rettv);
}
#endif

/*
 * "screenattr()" function
 */
    static void
f_screenattr(typval_T *argvars, typval_T *rettv)
{
    int		row;
    int		col;
    int		c;

    row = (int)tv_get_number_chk(&argvars[0], NULL) - 1;
    col = (int)tv_get_number_chk(&argvars[1], NULL) - 1;
    if (row < 0 || row >= screen_Rows
	    || col < 0 || col >= screen_Columns)
	c = -1;
    else
	c = ScreenAttrs[LineOffset[row] + col];
    rettv->vval.v_number = c;
}

/*
 * "screenchar()" function
 */
    static void
f_screenchar(typval_T *argvars, typval_T *rettv)
{
    int		row;
    int		col;
    int		off;
    int		c;

    row = (int)tv_get_number_chk(&argvars[0], NULL) - 1;
    col = (int)tv_get_number_chk(&argvars[1], NULL) - 1;
    if (row < 0 || row >= screen_Rows || col < 0 || col >= screen_Columns)
	c = -1;
    else
    {
	off = LineOffset[row] + col;
	if (enc_utf8 && ScreenLinesUC[off] != 0)
	    c = ScreenLinesUC[off];
	else
	    c = ScreenLines[off];
    }
    rettv->vval.v_number = c;
}

/*
 * "screenchars()" function
 */
    static void
f_screenchars(typval_T *argvars, typval_T *rettv)
{
    int		row;
    int		col;
    int		off;
    int		c;
    int		i;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    row = (int)tv_get_number_chk(&argvars[0], NULL) - 1;
    col = (int)tv_get_number_chk(&argvars[1], NULL) - 1;
    if (row < 0 || row >= screen_Rows || col < 0 || col >= screen_Columns)
	return;

    off = LineOffset[row] + col;
    if (enc_utf8 && ScreenLinesUC[off] != 0)
	c = ScreenLinesUC[off];
    else
	c = ScreenLines[off];
    list_append_number(rettv->vval.v_list, (varnumber_T)c);

    if (enc_utf8)

	for (i = 0; i < Screen_mco && ScreenLinesC[i][off] != 0; ++i)
	    list_append_number(rettv->vval.v_list,
				       (varnumber_T)ScreenLinesC[i][off]);
}

/*
 * "screencol()" function
 *
 * First column is 1 to be consistent with virtcol().
 */
    static void
f_screencol(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = screen_screencol() + 1;
}

/*
 * "screenrow()" function
 */
    static void
f_screenrow(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = screen_screenrow() + 1;
}

/*
 * "screenstring()" function
 */
    static void
f_screenstring(typval_T *argvars, typval_T *rettv)
{
    int		row;
    int		col;
    int		off;
    int		c;
    int		i;
    char_u	buf[MB_MAXBYTES + 1];
    int		buflen = 0;

    rettv->vval.v_string = NULL;
    rettv->v_type = VAR_STRING;

    row = (int)tv_get_number_chk(&argvars[0], NULL) - 1;
    col = (int)tv_get_number_chk(&argvars[1], NULL) - 1;
    if (row < 0 || row >= screen_Rows || col < 0 || col >= screen_Columns)
	return;

    off = LineOffset[row] + col;
    if (enc_utf8 && ScreenLinesUC[off] != 0)
	c = ScreenLinesUC[off];
    else
	c = ScreenLines[off];
    buflen += mb_char2bytes(c, buf);

    if (enc_utf8)
	for (i = 0; i < Screen_mco && ScreenLinesC[i][off] != 0; ++i)
	    buflen += mb_char2bytes(ScreenLinesC[i][off], buf + buflen);

    buf[buflen] = NUL;
    rettv->vval.v_string = vim_strsave(buf);
}

/*
 * "search()" function
 */
    static void
f_search(typval_T *argvars, typval_T *rettv)
{
    int		flags = 0;

    rettv->vval.v_number = search_cmn(argvars, NULL, &flags);
}

/*
 * "searchdecl()" function
 */
    static void
f_searchdecl(typval_T *argvars, typval_T *rettv)
{
    int		locally = 1;
    int		thisblock = 0;
    int		error = FALSE;
    char_u	*name;

    rettv->vval.v_number = 1;	// default: FAIL

    name = tv_get_string_chk(&argvars[0]);
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	locally = (int)tv_get_number_chk(&argvars[1], &error) == 0;
	if (!error && argvars[2].v_type != VAR_UNKNOWN)
	    thisblock = (int)tv_get_number_chk(&argvars[2], &error) != 0;
    }
    if (!error && name != NULL)
	rettv->vval.v_number = find_decl(name, (int)STRLEN(name),
				     locally, thisblock, SEARCH_KEEP) == FAIL;
}

/*
 * Used by searchpair() and searchpairpos()
 */
    static int
searchpair_cmn(typval_T *argvars, pos_T *match_pos)
{
    char_u	*spat, *mpat, *epat;
    typval_T	*skip;
    int		save_p_ws = p_ws;
    int		dir;
    int		flags = 0;
    char_u	nbuf1[NUMBUFLEN];
    char_u	nbuf2[NUMBUFLEN];
    int		retval = 0;		// default: FAIL
    long	lnum_stop = 0;
    long	time_limit = 0;

    // Get the three pattern arguments: start, middle, end. Will result in an
    // error if not a valid argument.
    spat = tv_get_string_chk(&argvars[0]);
    mpat = tv_get_string_buf_chk(&argvars[1], nbuf1);
    epat = tv_get_string_buf_chk(&argvars[2], nbuf2);
    if (spat == NULL || mpat == NULL || epat == NULL)
	goto theend;	    // type error

    // Handle the optional fourth argument: flags
    dir = get_search_arg(&argvars[3], &flags); // may set p_ws
    if (dir == 0)
	goto theend;

    // Don't accept SP_END or SP_SUBPAT.
    // Only one of the SP_NOMOVE or SP_SETPCMARK flags can be set.
    if ((flags & (SP_END | SP_SUBPAT)) != 0
	    || ((flags & SP_NOMOVE) && (flags & SP_SETPCMARK)))
    {
	semsg(_(e_invarg2), tv_get_string(&argvars[3]));
	goto theend;
    }

    // Using 'r' implies 'W', otherwise it doesn't work.
    if (flags & SP_REPEAT)
	p_ws = FALSE;

    // Optional fifth argument: skip expression
    if (argvars[3].v_type == VAR_UNKNOWN
	    || argvars[4].v_type == VAR_UNKNOWN)
	skip = NULL;
    else
    {
	skip = &argvars[4];
	if (skip->v_type != VAR_FUNC && skip->v_type != VAR_PARTIAL
	    && skip->v_type != VAR_STRING)
	{
	    // Type error
	    semsg(_(e_invarg2), tv_get_string(&argvars[4]));
	    goto theend;
	}
	if (argvars[5].v_type != VAR_UNKNOWN)
	{
	    lnum_stop = (long)tv_get_number_chk(&argvars[5], NULL);
	    if (lnum_stop < 0)
	    {
		semsg(_(e_invarg2), tv_get_string(&argvars[5]));
		goto theend;
	    }
#ifdef FEAT_RELTIME
	    if (argvars[6].v_type != VAR_UNKNOWN)
	    {
		time_limit = (long)tv_get_number_chk(&argvars[6], NULL);
		if (time_limit < 0)
		{
		    semsg(_(e_invarg2), tv_get_string(&argvars[6]));
		    goto theend;
		}
	    }
#endif
	}
    }

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
f_searchpair(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = searchpair_cmn(argvars, NULL);
}

/*
 * "searchpairpos()" function
 */
    static void
f_searchpairpos(typval_T *argvars, typval_T *rettv)
{
    pos_T	match_pos;
    int		lnum = 0;
    int		col = 0;

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
do_searchpair(
    char_u	*spat,	    // start pattern
    char_u	*mpat,	    // middle pattern
    char_u	*epat,	    // end pattern
    int		dir,	    // BACKWARD or FORWARD
    typval_T	*skip,	    // skip expression
    int		flags,	    // SP_SETPCMARK and other SP_ values
    pos_T	*match_pos,
    linenr_T	lnum_stop,  // stop at this line if not zero
    long	time_limit UNUSED) // stop after this many msec
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
    int		use_skip = FALSE;
    int		err;
    int		options = SEARCH_KEEP;
#ifdef FEAT_RELTIME
    proftime_T	tm;
#endif

    // Make 'cpoptions' empty, the 'l' flag should not be used here.
    save_cpo = p_cpo;
    p_cpo = empty_option;

#ifdef FEAT_RELTIME
    // Set the time limit, if there is one.
    profile_setlimit(time_limit, &tm);
#endif

    // Make two search patterns: start/end (pat2, for in nested pairs) and
    // start/middle/end (pat3, for the top pair).
    pat2 = alloc(STRLEN(spat) + STRLEN(epat) + 17);
    pat3 = alloc(STRLEN(spat) + STRLEN(mpat) + STRLEN(epat) + 25);
    if (pat2 == NULL || pat3 == NULL)
	goto theend;
    sprintf((char *)pat2, "\\m\\(%s\\m\\)\\|\\(%s\\m\\)", spat, epat);
    if (*mpat == NUL)
	STRCPY(pat3, pat2);
    else
	sprintf((char *)pat3, "\\m\\(%s\\m\\)\\|\\(%s\\m\\)\\|\\(%s\\m\\)",
							    spat, epat, mpat);
    if (flags & SP_START)
	options |= SEARCH_START;

    if (skip != NULL)
    {
	// Empty string means to not use the skip expression.
	if (skip->v_type == VAR_STRING || skip->v_type == VAR_FUNC)
	    use_skip = skip->vval.v_string != NULL
						&& *skip->vval.v_string != NUL;
    }

    save_cursor = curwin->w_cursor;
    pos = curwin->w_cursor;
    CLEAR_POS(&firstpos);
    CLEAR_POS(&foundpos);
    pat = pat3;
    for (;;)
    {
	searchit_arg_T sia;

	vim_memset(&sia, 0, sizeof(sia));
	sia.sa_stop_lnum = lnum_stop;
#ifdef FEAT_RELTIME
	sia.sa_tm = &tm;
#endif
	n = searchit(curwin, curbuf, &pos, NULL, dir, pat, 1L,
						     options, RE_SEARCH, &sia);
	if (n == FAIL || (firstpos.lnum != 0 && EQUAL_POS(pos, firstpos)))
	    // didn't find it or found the first match again: FAIL
	    break;

	if (firstpos.lnum == 0)
	    firstpos = pos;
	if (EQUAL_POS(pos, foundpos))
	{
	    // Found the same position again.  Can happen with a pattern that
	    // has "\zs" at the end and searching backwards.  Advance one
	    // character and try again.
	    if (dir == BACKWARD)
		decl(&pos);
	    else
		incl(&pos);
	}
	foundpos = pos;

	// clear the start flag to avoid getting stuck here
	options &= ~SEARCH_START;

	// If the skip pattern matches, ignore this match.
	if (use_skip)
	{
	    save_pos = curwin->w_cursor;
	    curwin->w_cursor = pos;
	    err = FALSE;
	    r = eval_expr_to_bool(skip, &err);
	    curwin->w_cursor = save_pos;
	    if (err)
	    {
		// Evaluating {skip} caused an error, break here.
		curwin->w_cursor = save_cursor;
		retval = -1;
		break;
	    }
	    if (r)
		continue;
	}

	if ((dir == BACKWARD && n == 3) || (dir == FORWARD && n == 2))
	{
	    // Found end when searching backwards or start when searching
	    // forward: nested pair.
	    ++nest;
	    pat = pat2;		// nested, don't search for middle
	}
	else
	{
	    // Found end when searching forward or start when searching
	    // backward: end of (nested) pair; or found middle in outer pair.
	    if (--nest == 1)
		pat = pat3;	// outer level, search for middle
	}

	if (nest == 0)
	{
	    // Found the match: return matchcount or line number.
	    if (flags & SP_RETCOUNT)
		++retval;
	    else
		retval = pos.lnum;
	    if (flags & SP_SETPCMARK)
		setpcmark();
	    curwin->w_cursor = pos;
	    if (!(flags & SP_REPEAT))
		break;
	    nest = 1;	    // search for next unmatched
	}
    }

    if (match_pos != NULL)
    {
	// Store the match cursor position
	match_pos->lnum = curwin->w_cursor.lnum;
	match_pos->col = curwin->w_cursor.col + 1;
    }

    // If 'n' flag is used or search failed: restore cursor position.
    if ((flags & SP_NOMOVE) || retval == 0)
	curwin->w_cursor = save_cursor;

theend:
    vim_free(pat2);
    vim_free(pat3);
    if (p_cpo == empty_option)
	p_cpo = save_cpo;
    else
	// Darn, evaluating the {skip} expression changed the value.
	free_string_option(save_cpo);

    return retval;
}

/*
 * "searchpos()" function
 */
    static void
f_searchpos(typval_T *argvars, typval_T *rettv)
{
    pos_T	match_pos;
    int		lnum = 0;
    int		col = 0;
    int		n;
    int		flags = 0;

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

    static void
f_server2client(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_CLIENTSERVER
    char_u	buf[NUMBUFLEN];
    char_u	*server = tv_get_string_chk(&argvars[0]);
    char_u	*reply = tv_get_string_buf_chk(&argvars[1], buf);

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
	emsg(_("E258: Unable to send to client"));
	return;
    }
    rettv->vval.v_number = 0;
#else
    rettv->vval.v_number = -1;
#endif
}

    static void
f_serverlist(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	*r = NULL;

#ifdef FEAT_CLIENTSERVER
# ifdef MSWIN
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

    static void
f_setcharsearch(typval_T *argvars, typval_T *rettv UNUSED)
{
    dict_T	*d;
    dictitem_T	*di;
    char_u	*csearch;

    if (argvars[0].v_type != VAR_DICT)
    {
	emsg(_(e_dictreq));
	return;
    }

    if ((d = argvars[0].vval.v_dict) != NULL)
    {
	csearch = dict_get_string(d, (char_u *)"char", FALSE);
	if (csearch != NULL)
	{
	    if (enc_utf8)
	    {
		int pcc[MAX_MCO];
		int c = utfc_ptr2char(csearch, pcc);

		set_last_csearch(c, csearch, utfc_ptr2len(csearch));
	    }
	    else
		set_last_csearch(PTR2CHAR(csearch),
						csearch, mb_ptr2len(csearch));
	}

	di = dict_find(d, (char_u *)"forward", -1);
	if (di != NULL)
	    set_csearch_direction((int)tv_get_number(&di->di_tv)
							? FORWARD : BACKWARD);

	di = dict_find(d, (char_u *)"until", -1);
	if (di != NULL)
	    set_csearch_until(!!tv_get_number(&di->di_tv));
    }
}

/*
 * "setenv()" function
 */
    static void
f_setenv(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u   namebuf[NUMBUFLEN];
    char_u   valbuf[NUMBUFLEN];
    char_u  *name = tv_get_string_buf(&argvars[0], namebuf);

    if (argvars[1].v_type == VAR_SPECIAL
				      && argvars[1].vval.v_number == VVAL_NULL)
	vim_unsetenv(name);
    else
	vim_setenv(name, tv_get_string_buf(&argvars[1], valbuf));
}

/*
 * "setfperm({fname}, {mode})" function
 */
    static void
f_setfperm(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    char_u	modebuf[NUMBUFLEN];
    char_u	*mode_str;
    int		i;
    int		mask;
    int		mode = 0;

    rettv->vval.v_number = 0;
    fname = tv_get_string_chk(&argvars[0]);
    if (fname == NULL)
	return;
    mode_str = tv_get_string_buf_chk(&argvars[1], modebuf);
    if (mode_str == NULL)
	return;
    if (STRLEN(mode_str) != 9)
    {
	semsg(_(e_invarg2), mode_str);
	return;
    }

    mask = 1;
    for (i = 8; i >= 0; --i)
    {
	if (mode_str[i] != '-')
	    mode |= mask;
	mask = mask << 1;
    }
    rettv->vval.v_number = mch_setperm(fname, mode) == OK;
}

/*
 * "setpos()" function
 */
    static void
f_setpos(typval_T *argvars, typval_T *rettv)
{
    pos_T	pos;
    int		fnum;
    char_u	*name;
    colnr_T	curswant = -1;

    rettv->vval.v_number = -1;
    name = tv_get_string_chk(argvars);
    if (name != NULL)
    {
	if (list2fpos(&argvars[1], &pos, &fnum, &curswant) == OK)
	{
	    if (pos.col != MAXCOL && --pos.col < 0)
		pos.col = 0;
	    if (name[0] == '.' && name[1] == NUL)
	    {
		// set cursor; "fnum" is ignored
		curwin->w_cursor = pos;
		if (curswant >= 0)
		{
		    curwin->w_curswant = curswant - 1;
		    curwin->w_set_curswant = FALSE;
		}
		check_cursor();
		rettv->vval.v_number = 0;
	    }
	    else if (name[0] == '\'' && name[1] != NUL && name[2] == NUL)
	    {
		// set mark
		if (setmark_pos(name[1], &pos, fnum) == OK)
		    rettv->vval.v_number = 0;
	    }
	    else
		emsg(_(e_invarg));
	}
    }
}

/*
 * "setreg()" function
 */
    static void
f_setreg(typval_T *argvars, typval_T *rettv)
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

    strregname = tv_get_string_chk(argvars);
    rettv->vval.v_number = 1;		// FAIL is default

    if (strregname == NULL)
	return;		// type error; errmsg already given
    regname = *strregname;
    if (regname == 0 || regname == '@')
	regname = '"';

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	stropt = tv_get_string_chk(&argvars[2]);
	if (stropt == NULL)
	    return;		// type error
	for (; *stropt != NUL; ++stropt)
	    switch (*stropt)
	    {
		case 'a': case 'A':	// append
		    append = TRUE;
		    break;
		case 'v': case 'c':	// character-wise selection
		    yank_type = MCHAR;
		    break;
		case 'V': case 'l':	// line-wise selection
		    yank_type = MLINE;
		    break;
		case 'b': case Ctrl_V:	// block-wise selection
		    yank_type = MBLOCK;
		    if (VIM_ISDIGIT(stropt[1]))
		    {
			++stropt;
			block_len = getdigits(&stropt) - 1;
			--stropt;
		    }
		    break;
	    }
    }

    if (argvars[1].v_type == VAR_LIST)
    {
	char_u		**lstval;
	char_u		**allocval;
	char_u		buf[NUMBUFLEN];
	char_u		**curval;
	char_u		**curallocval;
	list_T		*ll = argvars[1].vval.v_list;
	listitem_T	*li;
	int		len;

	// If the list is NULL handle like an empty list.
	len = ll == NULL ? 0 : ll->lv_len;

	// First half: use for pointers to result lines; second half: use for
	// pointers to allocated copies.
	lstval = ALLOC_MULT(char_u *, (len + 1) * 2);
	if (lstval == NULL)
	    return;
	curval = lstval;
	allocval = lstval + len + 2;
	curallocval = allocval;

	for (li = ll == NULL ? NULL : ll->lv_first; li != NULL;
							     li = li->li_next)
	{
	    strval = tv_get_string_buf_chk(&li->li_tv, buf);
	    if (strval == NULL)
		goto free_lstval;
	    if (strval == buf)
	    {
		// Need to make a copy, next tv_get_string_buf_chk() will
		// overwrite the string.
		strval = vim_strsave(buf);
		if (strval == NULL)
		    goto free_lstval;
		*curallocval++ = strval;
	    }
	    *curval++ = strval;
	}
	*curval++ = NULL;

	write_reg_contents_lst(regname, lstval, -1,
						append, yank_type, block_len);
free_lstval:
	while (curallocval > allocval)
	    vim_free(*--curallocval);
	vim_free(lstval);
    }
    else
    {
	strval = tv_get_string_chk(&argvars[1]);
	if (strval == NULL)
	    return;
	write_reg_contents_ex(regname, strval, -1,
						append, yank_type, block_len);
    }
    rettv->vval.v_number = 0;
}

/*
 * "settagstack()" function
 */
    static void
f_settagstack(typval_T *argvars, typval_T *rettv)
{
    static char *e_invact2 = N_("E962: Invalid action: '%s'");
    win_T	*wp;
    dict_T	*d;
    int		action = 'r';

    rettv->vval.v_number = -1;

    // first argument: window number or id
    wp = find_win_by_nr_or_id(&argvars[0]);
    if (wp == NULL)
	return;

    // second argument: dict with items to set in the tag stack
    if (argvars[1].v_type != VAR_DICT)
    {
	emsg(_(e_dictreq));
	return;
    }
    d = argvars[1].vval.v_dict;
    if (d == NULL)
	return;

    // third argument: action - 'a' for append and 'r' for replace.
    // default is to replace the stack.
    if (argvars[2].v_type == VAR_UNKNOWN)
	action = 'r';
    else if (argvars[2].v_type == VAR_STRING)
    {
	char_u	*actstr;
	actstr = tv_get_string_chk(&argvars[2]);
	if (actstr == NULL)
	    return;
	if ((*actstr == 'r' || *actstr == 'a' || *actstr == 't')
		&& actstr[1] == NUL)
	    action = *actstr;
	else
	{
	    semsg(_(e_invact2), actstr);
	    return;
	}
    }
    else
    {
	emsg(_(e_stringreq));
	return;
    }

    if (set_tagstack(wp, d, action) == OK)
	rettv->vval.v_number = 0;
}

#ifdef FEAT_CRYPT
/*
 * "sha256({string})" function
 */
    static void
f_sha256(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;

    p = tv_get_string(&argvars[0]);
    rettv->vval.v_string = vim_strsave(
				    sha256_bytes(p, (int)STRLEN(p), NULL, 0));
    rettv->v_type = VAR_STRING;
}
#endif // FEAT_CRYPT

/*
 * "shellescape({string})" function
 */
    static void
f_shellescape(typval_T *argvars, typval_T *rettv)
{
    int do_special = non_zero_arg(&argvars[1]);

    rettv->vval.v_string = vim_strsave_shellescape(
			   tv_get_string(&argvars[0]), do_special, do_special);
    rettv->v_type = VAR_STRING;
}

/*
 * shiftwidth() function
 */
    static void
f_shiftwidth(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = 0;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	long	col;

	col = (long)tv_get_number_chk(argvars, NULL);
	if (col < 0)
	    return;	// type error; errmsg already given
#ifdef FEAT_VARTABS
	rettv->vval.v_number = get_sw_value_col(curbuf, col);
	return;
#endif
    }

    rettv->vval.v_number = get_sw_value(curbuf);
}

#ifdef FEAT_FLOAT
/*
 * "sin()" function
 */
    static void
f_sin(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = sin(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "sinh()" function
 */
    static void
f_sinh(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = sinh(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "soundfold({word})" function
 */
    static void
f_soundfold(typval_T *argvars, typval_T *rettv)
{
    char_u	*s;

    rettv->v_type = VAR_STRING;
    s = tv_get_string(&argvars[0]);
#ifdef FEAT_SPELL
    rettv->vval.v_string = eval_soundfold(s);
#else
    rettv->vval.v_string = vim_strsave(s);
#endif
}

/*
 * "spellbadword()" function
 */
    static void
f_spellbadword(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	*word = (char_u *)"";
    hlf_T	attr = HLF_COUNT;
    int		len = 0;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

#ifdef FEAT_SPELL
    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	// Find the start and length of the badly spelled word.
	len = spell_move_to(curwin, FORWARD, TRUE, TRUE, &attr);
	if (len != 0)
	{
	    word = ml_get_cursor();
	    curwin->w_set_curswant = TRUE;
	}
    }
    else if (curwin->w_p_spell && *curbuf->b_s.b_p_spl != NUL)
    {
	char_u	*str = tv_get_string_chk(&argvars[0]);
	int	capcol = -1;

	if (str != NULL)
	{
	    // Check the argument for spelling.
	    while (*str != NUL)
	    {
		len = spell_check(curwin, str, &attr, &capcol, FALSE);
		if (attr != HLF_COUNT)
		{
		    word = str;
		    break;
		}
		str += len;
		capcol -= len;
		len = 0;
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
    static void
f_spellsuggest(typval_T *argvars UNUSED, typval_T *rettv)
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
    if (curwin->w_p_spell && *curwin->w_s->b_p_spl != NUL)
    {
	str = tv_get_string(&argvars[0]);
	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    maxcount = (int)tv_get_number_chk(&argvars[1], &typeerr);
	    if (maxcount <= 0)
		return;
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		need_capital = (int)tv_get_number_chk(&argvars[2], &typeerr);
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
f_split(typval_T *argvars, typval_T *rettv)
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

    // Make 'cpoptions' empty, the 'l' flag should not be used here.
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    str = tv_get_string(&argvars[0]);
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	pat = tv_get_string_buf_chk(&argvars[1], patbuf);
	if (pat == NULL)
	    typeerr = TRUE;
	if (argvars[2].v_type != VAR_UNKNOWN)
	    keepempty = (int)tv_get_number_chk(&argvars[2], &typeerr);
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
		match = FALSE;	// empty item at the end
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
	    // Advance to just after the match.
	    if (regmatch.endp[0] > str)
		col = 0;
	    else
		// Don't get stuck at the same match.
		col = (*mb_ptr2len)(regmatch.endp[0]);
	    str = regmatch.endp[0];
	}

	vim_regfree(regmatch.regprog);
    }

    p_cpo = save_cpo;
}

#ifdef FEAT_FLOAT
/*
 * "sqrt()" function
 */
    static void
f_sqrt(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = sqrt(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "srand()" function
 */
    static void
f_srand(typval_T *argvars, typval_T *rettv)
{
    static int dev_urandom_state = -1;  // FAIL or OK once tried
    UINT32_T x = 0, z;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	if (dev_urandom_state != FAIL)
	{
	    int  fd = open("/dev/urandom", O_RDONLY);
	    struct {
		union {
		    UINT32_T number;
		    char     bytes[sizeof(UINT32_T)];
		} cont;
	    } buf;

	    // Attempt reading /dev/urandom.
	    if (fd == -1)
		dev_urandom_state = FAIL;
	    else
	    {
		buf.cont.number = 0;
		if (read(fd, buf.cont.bytes, sizeof(UINT32_T))
							   != sizeof(UINT32_T))
		    dev_urandom_state = FAIL;
		else
		{
		    dev_urandom_state = OK;
		    x = buf.cont.number;
		}
		close(fd);
	    }

	}
	if (dev_urandom_state != OK)
	    // Reading /dev/urandom doesn't work, fall back to time().
	    x = vim_time();
    }
    else
    {
	int	    error = FALSE;

	x = (UINT32_T)tv_get_number_chk(&argvars[0], &error);
	if (error)
	    return;
    }

#define SPLITMIX32 ( \
    z = (x += 0x9e3779b9), \
    z = (z ^ (z >> 16)) * 0x85ebca6b, \
    z = (z ^ (z >> 13)) * 0xc2b2ae35, \
    z ^ (z >> 16) \
    )

    list_append_number(rettv->vval.v_list, (varnumber_T)SPLITMIX32);
    list_append_number(rettv->vval.v_list, (varnumber_T)SPLITMIX32);
    list_append_number(rettv->vval.v_list, (varnumber_T)SPLITMIX32);
    list_append_number(rettv->vval.v_list, (varnumber_T)SPLITMIX32);
}

#ifdef FEAT_FLOAT
/*
 * "str2float()" function
 */
    static void
f_str2float(typval_T *argvars, typval_T *rettv)
{
    char_u *p = skipwhite(tv_get_string(&argvars[0]));
    int     isneg = (*p == '-');

    if (*p == '+' || *p == '-')
	p = skipwhite(p + 1);
    (void)string2float(p, &rettv->vval.v_float);
    if (isneg)
	rettv->vval.v_float *= -1;
    rettv->v_type = VAR_FLOAT;
}
#endif

/*
 * "str2list()" function
 */
    static void
f_str2list(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		utf8 = FALSE;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
	utf8 = (int)tv_get_number_chk(&argvars[1], NULL);

    p = tv_get_string(&argvars[0]);

    if (has_mbyte || utf8)
    {
	int (*ptr2len)(char_u *);
	int (*ptr2char)(char_u *);

	if (utf8 || enc_utf8)
	{
	    ptr2len = utf_ptr2len;
	    ptr2char = utf_ptr2char;
	}
	else
	{
	    ptr2len = mb_ptr2len;
	    ptr2char = mb_ptr2char;
	}

	for ( ; *p != NUL; p += (*ptr2len)(p))
	    list_append_number(rettv->vval.v_list, (*ptr2char)(p));
    }
    else
	for ( ; *p != NUL; ++p)
	    list_append_number(rettv->vval.v_list, *p);
}

/*
 * "str2nr()" function
 */
    static void
f_str2nr(typval_T *argvars, typval_T *rettv)
{
    int		base = 10;
    char_u	*p;
    varnumber_T	n;
    int		what = 0;
    int		isneg;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	base = (int)tv_get_number(&argvars[1]);
	if (base != 2 && base != 8 && base != 10 && base != 16)
	{
	    emsg(_(e_invarg));
	    return;
	}
	if (argvars[2].v_type != VAR_UNKNOWN && tv_get_number(&argvars[2]))
	    what |= STR2NR_QUOTE;
    }

    p = skipwhite(tv_get_string(&argvars[0]));
    isneg = (*p == '-');
    if (*p == '+' || *p == '-')
	p = skipwhite(p + 1);
    switch (base)
    {
	case 2: what |= STR2NR_BIN + STR2NR_FORCE; break;
	case 8: what |= STR2NR_OCT + STR2NR_FORCE; break;
	case 16: what |= STR2NR_HEX + STR2NR_FORCE; break;
    }
    vim_str2nr(p, NULL, NULL, what, &n, NULL, 0, FALSE);
    // Text after the number is silently ignored.
    if (isneg)
	rettv->vval.v_number = -n;
    else
	rettv->vval.v_number = n;

}

#ifdef HAVE_STRFTIME
/*
 * "strftime({format}[, {time}])" function
 */
    static void
f_strftime(typval_T *argvars, typval_T *rettv)
{
    char_u	result_buf[256];
    struct tm	tmval;
    struct tm	*curtime;
    time_t	seconds;
    char_u	*p;

    rettv->v_type = VAR_STRING;

    p = tv_get_string(&argvars[0]);
    if (argvars[1].v_type == VAR_UNKNOWN)
	seconds = time(NULL);
    else
	seconds = (time_t)tv_get_number(&argvars[1]);
    curtime = vim_localtime(&seconds, &tmval);
    // MSVC returns NULL for an invalid value of seconds.
    if (curtime == NULL)
	rettv->vval.v_string = vim_strsave((char_u *)_("(Invalid)"));
    else
    {
	vimconv_T   conv;
	char_u	    *enc;

	conv.vc_type = CONV_NONE;
	enc = enc_locale();
	convert_setup(&conv, p_enc, enc);
	if (conv.vc_type != CONV_NONE)
	    p = string_convert(&conv, p, NULL);
	if (p != NULL)
	    (void)strftime((char *)result_buf, sizeof(result_buf),
							  (char *)p, curtime);
	else
	    result_buf[0] = NUL;

	if (conv.vc_type != CONV_NONE)
	    vim_free(p);
	convert_setup(&conv, enc, p_enc);
	if (conv.vc_type != CONV_NONE)
	    rettv->vval.v_string = string_convert(&conv, result_buf, NULL);
	else
	    rettv->vval.v_string = vim_strsave(result_buf);

	// Release conversion descriptors
	convert_setup(&conv, NULL, NULL);
	vim_free(enc);
    }
}
#endif

/*
 * "strgetchar()" function
 */
    static void
f_strgetchar(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    int		len;
    int		error = FALSE;
    int		charidx;
    int		byteidx = 0;

    rettv->vval.v_number = -1;
    str = tv_get_string_chk(&argvars[0]);
    if (str == NULL)
	return;
    len = (int)STRLEN(str);
    charidx = (int)tv_get_number_chk(&argvars[1], &error);
    if (error)
	return;

    while (charidx >= 0 && byteidx < len)
    {
	if (charidx == 0)
	{
	    rettv->vval.v_number = mb_ptr2char(str + byteidx);
	    break;
	}
	--charidx;
	byteidx += MB_CPTR2LEN(str + byteidx);
    }
}

/*
 * "stridx()" function
 */
    static void
f_stridx(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*save_haystack;
    char_u	*pos;
    int		start_idx;

    needle = tv_get_string_chk(&argvars[1]);
    save_haystack = haystack = tv_get_string_buf_chk(&argvars[0], buf);
    rettv->vval.v_number = -1;
    if (needle == NULL || haystack == NULL)
	return;		// type error; errmsg already given

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	int	    error = FALSE;

	start_idx = (int)tv_get_number_chk(&argvars[2], &error);
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
    void
f_string(typval_T *argvars, typval_T *rettv)
{
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = tv2string(&argvars[0], &tofree, numbuf,
								get_copyID());
    // Make a copy if we have a value but it's not in allocated memory.
    if (rettv->vval.v_string != NULL && tofree == NULL)
	rettv->vval.v_string = vim_strsave(rettv->vval.v_string);
}

/*
 * "strlen()" function
 */
    static void
f_strlen(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = (varnumber_T)(STRLEN(
					      tv_get_string(&argvars[0])));
}

/*
 * "strchars()" function
 */
    static void
f_strchars(typval_T *argvars, typval_T *rettv)
{
    char_u		*s = tv_get_string(&argvars[0]);
    int			skipcc = 0;
    varnumber_T		len = 0;
    int			(*func_mb_ptr2char_adv)(char_u **pp);

    if (argvars[1].v_type != VAR_UNKNOWN)
	skipcc = (int)tv_get_number_chk(&argvars[1], NULL);
    if (skipcc < 0 || skipcc > 1)
	emsg(_(e_invarg));
    else
    {
	func_mb_ptr2char_adv = skipcc ? mb_ptr2char_adv : mb_cptr2char_adv;
	while (*s != NUL)
	{
	    func_mb_ptr2char_adv(&s);
	    ++len;
	}
	rettv->vval.v_number = len;
    }
}

/*
 * "strdisplaywidth()" function
 */
    static void
f_strdisplaywidth(typval_T *argvars, typval_T *rettv)
{
    char_u	*s = tv_get_string(&argvars[0]);
    int		col = 0;

    if (argvars[1].v_type != VAR_UNKNOWN)
	col = (int)tv_get_number(&argvars[1]);

    rettv->vval.v_number = (varnumber_T)(linetabsize_col(col, s) - col);
}

/*
 * "strwidth()" function
 */
    static void
f_strwidth(typval_T *argvars, typval_T *rettv)
{
    char_u	*s = tv_get_string(&argvars[0]);

    rettv->vval.v_number = (varnumber_T)(mb_string2cells(s, -1));
}

/*
 * "strcharpart()" function
 */
    static void
f_strcharpart(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		nchar;
    int		nbyte = 0;
    int		charlen;
    int		len = 0;
    int		slen;
    int		error = FALSE;

    p = tv_get_string(&argvars[0]);
    slen = (int)STRLEN(p);

    nchar = (int)tv_get_number_chk(&argvars[1], &error);
    if (!error)
    {
	if (nchar > 0)
	    while (nchar > 0 && nbyte < slen)
	    {
		nbyte += MB_CPTR2LEN(p + nbyte);
		--nchar;
	    }
	else
	    nbyte = nchar;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    charlen = (int)tv_get_number(&argvars[2]);
	    while (charlen > 0 && nbyte + len < slen)
	    {
		int off = nbyte + len;

		if (off < 0)
		    len += 1;
		else
		    len += MB_CPTR2LEN(p + off);
		--charlen;
	    }
	}
	else
	    len = slen - nbyte;    // default: all bytes that are available.
    }

    /*
     * Only return the overlap between the specified part and the actual
     * string.
     */
    if (nbyte < 0)
    {
	len += nbyte;
	nbyte = 0;
    }
    else if (nbyte > slen)
	nbyte = slen;
    if (len < 0)
	len = 0;
    else if (nbyte + len > slen)
	len = slen - nbyte;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strnsave(p + nbyte, len);
}

/*
 * "strpart()" function
 */
    static void
f_strpart(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		n;
    int		len;
    int		slen;
    int		error = FALSE;

    p = tv_get_string(&argvars[0]);
    slen = (int)STRLEN(p);

    n = (int)tv_get_number_chk(&argvars[1], &error);
    if (error)
	len = 0;
    else if (argvars[2].v_type != VAR_UNKNOWN)
	len = (int)tv_get_number(&argvars[2]);
    else
	len = slen - n;	    // default len: all bytes that are available.

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

#ifdef HAVE_STRPTIME
/*
 * "strptime({format}, {timestring})" function
 */
    static void
f_strptime(typval_T *argvars, typval_T *rettv)
{
    struct tm	tmval;
    char_u	*fmt;
    char_u	*str;
    vimconv_T   conv;
    char_u	*enc;

    vim_memset(&tmval, NUL, sizeof(tmval));
    fmt = tv_get_string(&argvars[0]);
    str = tv_get_string(&argvars[1]);

    conv.vc_type = CONV_NONE;
    enc = enc_locale();
    convert_setup(&conv, p_enc, enc);
    if (conv.vc_type != CONV_NONE)
	fmt = string_convert(&conv, fmt, NULL);
    if (fmt == NULL
	    || strptime((char *)str, (char *)fmt, &tmval) == NULL
	    || (rettv->vval.v_number = mktime(&tmval)) == -1)
	rettv->vval.v_number = 0;

    if (conv.vc_type != CONV_NONE)
	vim_free(fmt);
    convert_setup(&conv, NULL, NULL);
    vim_free(enc);
}
#endif

/*
 * "strridx()" function
 */
    static void
f_strridx(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*rest;
    char_u	*lastmatch = NULL;
    int		haystack_len, end_idx;

    needle = tv_get_string_chk(&argvars[1]);
    haystack = tv_get_string_buf_chk(&argvars[0], buf);

    rettv->vval.v_number = -1;
    if (needle == NULL || haystack == NULL)
	return;		// type error; errmsg already given

    haystack_len = (int)STRLEN(haystack);
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	// Third argument: upper limit for index
	end_idx = (int)tv_get_number_chk(&argvars[2], NULL);
	if (end_idx < 0)
	    return;	// can never find a match
    }
    else
	end_idx = haystack_len;

    if (*needle == NUL)
    {
	// Empty string matches past the end.
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
f_strtrans(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = transstr(tv_get_string(&argvars[0]));
}

/*
 * "submatch()" function
 */
    static void
f_submatch(typval_T *argvars, typval_T *rettv)
{
    int		error = FALSE;
    int		no;
    int		retList = 0;

    no = (int)tv_get_number_chk(&argvars[0], &error);
    if (error)
	return;
    if (no < 0 || no >= NSUBEXP)
    {
	semsg(_("E935: invalid submatch number: %d"), no);
	return;
    }
    if (argvars[1].v_type != VAR_UNKNOWN)
	retList = (int)tv_get_number_chk(&argvars[1], &error);
    if (error)
	return;

    if (retList == 0)
    {
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = reg_submatch(no);
    }
    else
    {
	rettv->v_type = VAR_LIST;
	rettv->vval.v_list = reg_submatch_list(no);
    }
}

/*
 * "substitute()" function
 */
    static void
f_substitute(typval_T *argvars, typval_T *rettv)
{
    char_u	patbuf[NUMBUFLEN];
    char_u	subbuf[NUMBUFLEN];
    char_u	flagsbuf[NUMBUFLEN];

    char_u	*str = tv_get_string_chk(&argvars[0]);
    char_u	*pat = tv_get_string_buf_chk(&argvars[1], patbuf);
    char_u	*sub = NULL;
    typval_T	*expr = NULL;
    char_u	*flg = tv_get_string_buf_chk(&argvars[3], flagsbuf);

    if (argvars[2].v_type == VAR_FUNC || argvars[2].v_type == VAR_PARTIAL)
	expr = &argvars[2];
    else
	sub = tv_get_string_buf_chk(&argvars[2], subbuf);

    rettv->v_type = VAR_STRING;
    if (str == NULL || pat == NULL || (sub == NULL && expr == NULL)
								|| flg == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = do_string_sub(str, pat, sub, expr, flg);
}

/*
 * "swapinfo(swap_filename)" function
 */
    static void
f_swapinfo(typval_T *argvars, typval_T *rettv)
{
    if (rettv_dict_alloc(rettv) == OK)
	get_b0_dict(tv_get_string(argvars), rettv->vval.v_dict);
}

/*
 * "swapname(expr)" function
 */
    static void
f_swapname(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf;

    rettv->v_type = VAR_STRING;
    buf = tv_get_buf(&argvars[0], FALSE);
    if (buf == NULL || buf->b_ml.ml_mfp == NULL
					|| buf->b_ml.ml_mfp->mf_fname == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = vim_strsave(buf->b_ml.ml_mfp->mf_fname);
}

/*
 * "synID(lnum, col, trans)" function
 */
    static void
f_synID(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		id = 0;
#ifdef FEAT_SYN_HL
    linenr_T	lnum;
    colnr_T	col;
    int		trans;
    int		transerr = FALSE;

    lnum = tv_get_lnum(argvars);		// -1 on type error
    col = (linenr_T)tv_get_number(&argvars[1]) - 1;	// -1 on type error
    trans = (int)tv_get_number_chk(&argvars[2], &transerr);

    if (!transerr && lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count
	    && col >= 0 && col < (long)STRLEN(ml_get(lnum)))
	id = syn_get_id(curwin, lnum, (colnr_T)col, trans, NULL, FALSE);
#endif

    rettv->vval.v_number = id;
}

/*
 * "synIDattr(id, what [, mode])" function
 */
    static void
f_synIDattr(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	*p = NULL;
#ifdef FEAT_SYN_HL
    int		id;
    char_u	*what;
    char_u	*mode;
    char_u	modebuf[NUMBUFLEN];
    int		modec;

    id = (int)tv_get_number(&argvars[0]);
    what = tv_get_string(&argvars[1]);
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	mode = tv_get_string_buf(&argvars[2], modebuf);
	modec = TOLOWER_ASC(mode[0]);
	if (modec != 't' && modec != 'c' && modec != 'g')
	    modec = 0;	// replace invalid with current
    }
    else
    {
#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
	if (USE_24BIT)
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
		if (TOLOWER_ASC(what[1]) == 'g')	// bg[#]
		    p = highlight_color(id, what, modec);
		else					// bold
		    p = highlight_has_attr(id, HL_BOLD, modec);
		break;

	case 'f':					// fg[#] or font
		p = highlight_color(id, what, modec);
		break;

	case 'i':
		if (TOLOWER_ASC(what[1]) == 'n')	// inverse
		    p = highlight_has_attr(id, HL_INVERSE, modec);
		else					// italic
		    p = highlight_has_attr(id, HL_ITALIC, modec);
		break;

	case 'n':					// name
		p = get_highlight_name_ext(NULL, id - 1, FALSE);
		break;

	case 'r':					// reverse
		p = highlight_has_attr(id, HL_INVERSE, modec);
		break;

	case 's':
		if (TOLOWER_ASC(what[1]) == 'p')	// sp[#]
		    p = highlight_color(id, what, modec);
							// strikeout
		else if (TOLOWER_ASC(what[1]) == 't' &&
			TOLOWER_ASC(what[2]) == 'r')
		    p = highlight_has_attr(id, HL_STRIKETHROUGH, modec);
		else					// standout
		    p = highlight_has_attr(id, HL_STANDOUT, modec);
		break;

	case 'u':
		if (STRLEN(what) <= 5 || TOLOWER_ASC(what[5]) != 'c')
							// underline
		    p = highlight_has_attr(id, HL_UNDERLINE, modec);
		else
							// undercurl
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
    static void
f_synIDtrans(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		id;

#ifdef FEAT_SYN_HL
    id = (int)tv_get_number(&argvars[0]);

    if (id > 0)
	id = syn_get_final_id(id);
    else
#endif
	id = 0;

    rettv->vval.v_number = id;
}

/*
 * "synconcealed(lnum, col)" function
 */
    static void
f_synconcealed(typval_T *argvars UNUSED, typval_T *rettv)
{
#if defined(FEAT_SYN_HL) && defined(FEAT_CONCEAL)
    linenr_T	lnum;
    colnr_T	col;
    int		syntax_flags = 0;
    int		cchar;
    int		matchid = 0;
    char_u	str[NUMBUFLEN];
#endif

    rettv_list_set(rettv, NULL);

#if defined(FEAT_SYN_HL) && defined(FEAT_CONCEAL)
    lnum = tv_get_lnum(argvars);		// -1 on type error
    col = (colnr_T)tv_get_number(&argvars[1]) - 1;	// -1 on type error

    vim_memset(str, NUL, sizeof(str));

    if (rettv_list_alloc(rettv) != FAIL)
    {
	if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count
	    && col >= 0 && col <= (long)STRLEN(ml_get(lnum))
	    && curwin->w_p_cole > 0)
	{
	    (void)syn_get_id(curwin, lnum, col, FALSE, NULL, FALSE);
	    syntax_flags = get_syntax_info(&matchid);

	    // get the conceal character
	    if ((syntax_flags & HL_CONCEAL) && curwin->w_p_cole < 3)
	    {
		cchar = syn_get_sub_char();
		if (cchar == NUL && curwin->w_p_cole == 1)
		    cchar = (lcs_conceal == NUL) ? ' ' : lcs_conceal;
		if (cchar != NUL)
		{
		    if (has_mbyte)
			(*mb_char2bytes)(cchar, str);
		    else
			str[0] = cchar;
		}
	    }
	}

	list_append_number(rettv->vval.v_list,
					    (syntax_flags & HL_CONCEAL) != 0);
	// -1 to auto-determine strlen
	list_append_string(rettv->vval.v_list, str, -1);
	list_append_number(rettv->vval.v_list, matchid);
    }
#endif
}

/*
 * "synstack(lnum, col)" function
 */
    static void
f_synstack(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_SYN_HL
    linenr_T	lnum;
    colnr_T	col;
    int		i;
    int		id;
#endif

    rettv_list_set(rettv, NULL);

#ifdef FEAT_SYN_HL
    lnum = tv_get_lnum(argvars);		// -1 on type error
    col = (colnr_T)tv_get_number(&argvars[1]) - 1;	// -1 on type error

    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count
	    && col >= 0 && col <= (long)STRLEN(ml_get(lnum))
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
 * "tabpagebuflist()" function
 */
    static void
f_tabpagebuflist(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    tabpage_T	*tp;
    win_T	*wp = NULL;

    if (argvars[0].v_type == VAR_UNKNOWN)
	wp = firstwin;
    else
    {
	tp = find_tabpage((int)tv_get_number(&argvars[0]));
	if (tp != NULL)
	    wp = (tp == curtab) ? firstwin : tp->tp_firstwin;
    }
    if (wp != NULL && rettv_list_alloc(rettv) != FAIL)
    {
	for (; wp != NULL; wp = wp->w_next)
	    if (list_append_number(rettv->vval.v_list,
						wp->w_buffer->b_fnum) == FAIL)
		break;
    }
}

/*
 * "tagfiles()" function
 */
    static void
f_tagfiles(typval_T *argvars UNUSED, typval_T *rettv)
{
    char_u	*fname;
    tagname_T	tn;
    int		first;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    fname = alloc(MAXPATHL);
    if (fname == NULL)
	return;

    for (first = TRUE; ; first = FALSE)
	if (get_tagfname(&tn, first, fname) == FAIL
		|| list_append_string(rettv->vval.v_list, fname, -1) == FAIL)
	    break;
    tagname_free(&tn);
    vim_free(fname);
}

/*
 * "taglist()" function
 */
    static void
f_taglist(typval_T *argvars, typval_T *rettv)
{
    char_u  *fname = NULL;
    char_u  *tag_pattern;

    tag_pattern = tv_get_string(&argvars[0]);

    rettv->vval.v_number = FALSE;
    if (*tag_pattern == NUL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
	fname = tv_get_string(&argvars[1]);
    if (rettv_list_alloc(rettv) == OK)
	(void)get_tags(rettv->vval.v_list, tag_pattern, fname);
}

#ifdef FEAT_FLOAT
/*
 * "tan()" function
 */
    static void
f_tan(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = tan(f);
    else
	rettv->vval.v_float = 0.0;
}

/*
 * "tanh()" function
 */
    static void
f_tanh(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	rettv->vval.v_float = tanh(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "tolower(string)" function
 */
    static void
f_tolower(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = strlow_save(tv_get_string(&argvars[0]));
}

/*
 * "toupper(string)" function
 */
    static void
f_toupper(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = strup_save(tv_get_string(&argvars[0]));
}

/*
 * "tr(string, fromstr, tostr)" function
 */
    static void
f_tr(typval_T *argvars, typval_T *rettv)
{
    char_u	*in_str;
    char_u	*fromstr;
    char_u	*tostr;
    char_u	*p;
    int		inlen;
    int		fromlen;
    int		tolen;
    int		idx;
    char_u	*cpstr;
    int		cplen;
    int		first = TRUE;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    garray_T	ga;

    in_str = tv_get_string(&argvars[0]);
    fromstr = tv_get_string_buf_chk(&argvars[1], buf);
    tostr = tv_get_string_buf_chk(&argvars[2], buf2);

    // Default return value: empty string.
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    if (fromstr == NULL || tostr == NULL)
	    return;		// type error; errmsg already given
    ga_init2(&ga, (int)sizeof(char), 80);

    if (!has_mbyte)
	// not multi-byte: fromstr and tostr must be the same length
	if (STRLEN(fromstr) != STRLEN(tostr))
	{
error:
	    semsg(_(e_invarg2), fromstr);
	    ga_clear(&ga);
	    return;
	}

    // fromstr and tostr have to contain the same number of chars
    while (*in_str != NUL)
    {
	if (has_mbyte)
	{
	    inlen = (*mb_ptr2len)(in_str);
	    cpstr = in_str;
	    cplen = inlen;
	    idx = 0;
	    for (p = fromstr; *p != NUL; p += fromlen)
	    {
		fromlen = (*mb_ptr2len)(p);
		if (fromlen == inlen && STRNCMP(in_str, p, inlen) == 0)
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
		    if (*p == NUL)	// tostr is shorter than fromstr
			goto error;
		    break;
		}
		++idx;
	    }

	    if (first && cpstr == in_str)
	    {
		// Check that fromstr and tostr have the same number of
		// (multi-byte) characters.  Done only once when a character
		// of in_str doesn't appear in fromstr.
		first = FALSE;
		for (p = tostr; *p != NUL; p += tolen)
		{
		    tolen = (*mb_ptr2len)(p);
		    --idx;
		}
		if (idx != 0)
		    goto error;
	    }

	    (void)ga_grow(&ga, cplen);
	    mch_memmove((char *)ga.ga_data + ga.ga_len, cpstr, (size_t)cplen);
	    ga.ga_len += cplen;

	    in_str += inlen;
	}
	else
	{
	    // When not using multi-byte chars we can do it faster.
	    p = vim_strchr(fromstr, *in_str);
	    if (p != NULL)
		ga_append(&ga, tostr[p - fromstr]);
	    else
		ga_append(&ga, *in_str);
	    ++in_str;
	}
    }

    // add a terminating NUL
    (void)ga_grow(&ga, 1);
    ga_append(&ga, NUL);

    rettv->vval.v_string = ga.ga_data;
}

/*
 * "trim({expr})" function
 */
    static void
f_trim(typval_T *argvars, typval_T *rettv)
{
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*head = tv_get_string_buf_chk(&argvars[0], buf1);
    char_u	*mask = NULL;
    char_u	*tail;
    char_u	*prev;
    char_u	*p;
    int		c1;

    rettv->v_type = VAR_STRING;
    if (head == NULL)
    {
	rettv->vval.v_string = NULL;
	return;
    }

    if (argvars[1].v_type == VAR_STRING)
	mask = tv_get_string_buf_chk(&argvars[1], buf2);

    while (*head != NUL)
    {
	c1 = PTR2CHAR(head);
	if (mask == NULL)
	{
	    if (c1 > ' ' && c1 != 0xa0)
		break;
	}
	else
	{
	    for (p = mask; *p != NUL; MB_PTR_ADV(p))
		if (c1 == PTR2CHAR(p))
		    break;
	    if (*p == NUL)
		break;
	}
	MB_PTR_ADV(head);
    }

    for (tail = head + STRLEN(head); tail > head; tail = prev)
    {
	prev = tail;
	MB_PTR_BACK(head, prev);
	c1 = PTR2CHAR(prev);
	if (mask == NULL)
	{
	    if (c1 > ' ' && c1 != 0xa0)
		break;
	}
	else
	{
	    for (p = mask; *p != NUL; MB_PTR_ADV(p))
		if (c1 == PTR2CHAR(p))
		    break;
	    if (*p == NUL)
		break;
	}
    }
    rettv->vval.v_string = vim_strnsave(head, (int)(tail - head));
}

#ifdef FEAT_FLOAT
/*
 * "trunc({float})" function
 */
    static void
f_trunc(typval_T *argvars, typval_T *rettv)
{
    float_T	f = 0.0;

    rettv->v_type = VAR_FLOAT;
    if (get_float_arg(argvars, &f) == OK)
	// trunc() is not in C90, use floor() or ceil() instead.
	rettv->vval.v_float = f > 0 ? floor(f) : ceil(f);
    else
	rettv->vval.v_float = 0.0;
}
#endif

/*
 * "type(expr)" function
 */
    static void
f_type(typval_T *argvars, typval_T *rettv)
{
    int n = -1;

    switch (argvars[0].v_type)
    {
	case VAR_NUMBER:  n = VAR_TYPE_NUMBER; break;
	case VAR_STRING:  n = VAR_TYPE_STRING; break;
	case VAR_PARTIAL:
	case VAR_FUNC:    n = VAR_TYPE_FUNC; break;
	case VAR_LIST:    n = VAR_TYPE_LIST; break;
	case VAR_DICT:    n = VAR_TYPE_DICT; break;
	case VAR_FLOAT:   n = VAR_TYPE_FLOAT; break;
	case VAR_BOOL:	  n = VAR_TYPE_BOOL; break;
	case VAR_SPECIAL: n = VAR_TYPE_NONE; break;
	case VAR_JOB:     n = VAR_TYPE_JOB; break;
	case VAR_CHANNEL: n = VAR_TYPE_CHANNEL; break;
	case VAR_BLOB:    n = VAR_TYPE_BLOB; break;
	case VAR_UNKNOWN:
	case VAR_VOID:
	     internal_error("f_type(UNKNOWN)");
	     n = -1;
	     break;
    }
    rettv->vval.v_number = n;
}

/*
 * "virtcol(string)" function
 */
    static void
f_virtcol(typval_T *argvars, typval_T *rettv)
{
    colnr_T	vcol = 0;
    pos_T	*fp;
    int		fnum = curbuf->b_fnum;
    int		len;

    fp = var2fpos(&argvars[0], FALSE, &fnum);
    if (fp != NULL && fp->lnum <= curbuf->b_ml.ml_line_count
						    && fnum == curbuf->b_fnum)
    {
	// Limit the column to a valid value, getvvcol() doesn't check.
	if (fp->col < 0)
	    fp->col = 0;
	else
	{
	    len = (int)STRLEN(ml_get(fp->lnum));
	    if (fp->col > len)
		fp->col = len;
	}
	getvvcol(curwin, fp, NULL, NULL, &vcol);
	++vcol;
    }

    rettv->vval.v_number = vcol;
}

/*
 * "visualmode()" function
 */
    static void
f_visualmode(typval_T *argvars, typval_T *rettv)
{
    char_u	str[2];

    rettv->v_type = VAR_STRING;
    str[0] = curbuf->b_visual_mode_eval;
    str[1] = NUL;
    rettv->vval.v_string = vim_strsave(str);

    // A non-zero number or non-empty string argument: reset mode.
    if (non_zero_arg(&argvars[0]))
	curbuf->b_visual_mode_eval = NUL;
}

/*
 * "wildmenumode()" function
 */
    static void
f_wildmenumode(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_WILDMENU
    if (wild_menu_showing)
	rettv->vval.v_number = 1;
#endif
}

/*
 * "windowsversion()" function
 */
    static void
f_windowsversion(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strsave((char_u *)windowsVersion);
}

/*
 * "wordcount()" function
 */
    static void
f_wordcount(typval_T *argvars UNUSED, typval_T *rettv)
{
    if (rettv_dict_alloc(rettv) == FAIL)
	return;
    cursor_pos_info(rettv->vval.v_dict);
}

/*
 * "xor(expr, expr)" function
 */
    static void
f_xor(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = tv_get_number_chk(&argvars[0], NULL)
					^ tv_get_number_chk(&argvars[1], NULL);
}

#endif // FEAT_EVAL
