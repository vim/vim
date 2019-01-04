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

#ifdef AMIGA
# include <time.h>	/* for strftime() */
#endif

#ifdef VMS
# include <float.h>
#endif

#ifdef MACOS_X
# include <time.h>	/* for time_t */
#endif

static char *e_listarg = N_("E686: Argument of %s must be a List");
static char *e_stringreq = N_("E928: String required");

#ifdef FEAT_FLOAT
static void f_abs(typval_T *argvars, typval_T *rettv);
static void f_acos(typval_T *argvars, typval_T *rettv);
#endif
static void f_add(typval_T *argvars, typval_T *rettv);
static void f_and(typval_T *argvars, typval_T *rettv);
static void f_append(typval_T *argvars, typval_T *rettv);
static void f_appendbufline(typval_T *argvars, typval_T *rettv);
static void f_argc(typval_T *argvars, typval_T *rettv);
static void f_argidx(typval_T *argvars, typval_T *rettv);
static void f_arglistid(typval_T *argvars, typval_T *rettv);
static void f_argv(typval_T *argvars, typval_T *rettv);
static void f_assert_beeps(typval_T *argvars, typval_T *rettv);
static void f_assert_equal(typval_T *argvars, typval_T *rettv);
static void f_assert_equalfile(typval_T *argvars, typval_T *rettv);
static void f_assert_exception(typval_T *argvars, typval_T *rettv);
static void f_assert_fails(typval_T *argvars, typval_T *rettv);
static void f_assert_false(typval_T *argvars, typval_T *rettv);
static void f_assert_inrange(typval_T *argvars, typval_T *rettv);
static void f_assert_match(typval_T *argvars, typval_T *rettv);
static void f_assert_notequal(typval_T *argvars, typval_T *rettv);
static void f_assert_notmatch(typval_T *argvars, typval_T *rettv);
static void f_assert_report(typval_T *argvars, typval_T *rettv);
static void f_assert_true(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_asin(typval_T *argvars, typval_T *rettv);
static void f_atan(typval_T *argvars, typval_T *rettv);
static void f_atan2(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_BEVAL
static void f_balloon_show(typval_T *argvars, typval_T *rettv);
# if defined(FEAT_BEVAL_TERM)
static void f_balloon_split(typval_T *argvars, typval_T *rettv);
# endif
#endif
static void f_browse(typval_T *argvars, typval_T *rettv);
static void f_browsedir(typval_T *argvars, typval_T *rettv);
static void f_bufexists(typval_T *argvars, typval_T *rettv);
static void f_buflisted(typval_T *argvars, typval_T *rettv);
static void f_bufloaded(typval_T *argvars, typval_T *rettv);
static void f_bufname(typval_T *argvars, typval_T *rettv);
static void f_bufnr(typval_T *argvars, typval_T *rettv);
static void f_bufwinid(typval_T *argvars, typval_T *rettv);
static void f_bufwinnr(typval_T *argvars, typval_T *rettv);
static void f_byte2line(typval_T *argvars, typval_T *rettv);
static void byteidx(typval_T *argvars, typval_T *rettv, int comp);
static void f_byteidx(typval_T *argvars, typval_T *rettv);
static void f_byteidxcomp(typval_T *argvars, typval_T *rettv);
static void f_call(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_ceil(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_JOB_CHANNEL
static void f_ch_canread(typval_T *argvars, typval_T *rettv);
static void f_ch_close(typval_T *argvars, typval_T *rettv);
static void f_ch_close_in(typval_T *argvars, typval_T *rettv);
static void f_ch_evalexpr(typval_T *argvars, typval_T *rettv);
static void f_ch_evalraw(typval_T *argvars, typval_T *rettv);
static void f_ch_getbufnr(typval_T *argvars, typval_T *rettv);
static void f_ch_getjob(typval_T *argvars, typval_T *rettv);
static void f_ch_info(typval_T *argvars, typval_T *rettv);
static void f_ch_log(typval_T *argvars, typval_T *rettv);
static void f_ch_logfile(typval_T *argvars, typval_T *rettv);
static void f_ch_open(typval_T *argvars, typval_T *rettv);
static void f_ch_read(typval_T *argvars, typval_T *rettv);
static void f_ch_readraw(typval_T *argvars, typval_T *rettv);
static void f_ch_sendexpr(typval_T *argvars, typval_T *rettv);
static void f_ch_sendraw(typval_T *argvars, typval_T *rettv);
static void f_ch_setoptions(typval_T *argvars, typval_T *rettv);
static void f_ch_status(typval_T *argvars, typval_T *rettv);
#endif
static void f_changenr(typval_T *argvars, typval_T *rettv);
static void f_char2nr(typval_T *argvars, typval_T *rettv);
static void f_cindent(typval_T *argvars, typval_T *rettv);
static void f_clearmatches(typval_T *argvars, typval_T *rettv);
static void f_col(typval_T *argvars, typval_T *rettv);
#if defined(FEAT_INS_EXPAND)
static void f_complete(typval_T *argvars, typval_T *rettv);
static void f_complete_add(typval_T *argvars, typval_T *rettv);
static void f_complete_check(typval_T *argvars, typval_T *rettv);
#endif
static void f_confirm(typval_T *argvars, typval_T *rettv);
static void f_copy(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_cos(typval_T *argvars, typval_T *rettv);
static void f_cosh(typval_T *argvars, typval_T *rettv);
#endif
static void f_count(typval_T *argvars, typval_T *rettv);
static void f_cscope_connection(typval_T *argvars, typval_T *rettv);
static void f_cursor(typval_T *argsvars, typval_T *rettv);
#ifdef WIN3264
static void f_debugbreak(typval_T *argvars, typval_T *rettv);
#endif
static void f_deepcopy(typval_T *argvars, typval_T *rettv);
static void f_delete(typval_T *argvars, typval_T *rettv);
static void f_deletebufline(typval_T *argvars, typval_T *rettv);
static void f_did_filetype(typval_T *argvars, typval_T *rettv);
static void f_diff_filler(typval_T *argvars, typval_T *rettv);
static void f_diff_hlID(typval_T *argvars, typval_T *rettv);
static void f_empty(typval_T *argvars, typval_T *rettv);
static void f_escape(typval_T *argvars, typval_T *rettv);
static void f_eval(typval_T *argvars, typval_T *rettv);
static void f_eventhandler(typval_T *argvars, typval_T *rettv);
static void f_executable(typval_T *argvars, typval_T *rettv);
static void f_execute(typval_T *argvars, typval_T *rettv);
static void f_exepath(typval_T *argvars, typval_T *rettv);
static void f_exists(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_exp(typval_T *argvars, typval_T *rettv);
#endif
static void f_expand(typval_T *argvars, typval_T *rettv);
static void f_extend(typval_T *argvars, typval_T *rettv);
static void f_feedkeys(typval_T *argvars, typval_T *rettv);
static void f_filereadable(typval_T *argvars, typval_T *rettv);
static void f_filewritable(typval_T *argvars, typval_T *rettv);
static void f_filter(typval_T *argvars, typval_T *rettv);
static void f_finddir(typval_T *argvars, typval_T *rettv);
static void f_findfile(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_float2nr(typval_T *argvars, typval_T *rettv);
static void f_floor(typval_T *argvars, typval_T *rettv);
static void f_fmod(typval_T *argvars, typval_T *rettv);
#endif
static void f_fnameescape(typval_T *argvars, typval_T *rettv);
static void f_fnamemodify(typval_T *argvars, typval_T *rettv);
static void f_foldclosed(typval_T *argvars, typval_T *rettv);
static void f_foldclosedend(typval_T *argvars, typval_T *rettv);
static void f_foldlevel(typval_T *argvars, typval_T *rettv);
static void f_foldtext(typval_T *argvars, typval_T *rettv);
static void f_foldtextresult(typval_T *argvars, typval_T *rettv);
static void f_foreground(typval_T *argvars, typval_T *rettv);
static void f_funcref(typval_T *argvars, typval_T *rettv);
static void f_function(typval_T *argvars, typval_T *rettv);
static void f_garbagecollect(typval_T *argvars, typval_T *rettv);
static void f_get(typval_T *argvars, typval_T *rettv);
static void f_getbufinfo(typval_T *argvars, typval_T *rettv);
static void f_getbufline(typval_T *argvars, typval_T *rettv);
static void f_getbufvar(typval_T *argvars, typval_T *rettv);
static void f_getchangelist(typval_T *argvars, typval_T *rettv);
static void f_getchar(typval_T *argvars, typval_T *rettv);
static void f_getcharmod(typval_T *argvars, typval_T *rettv);
static void f_getcharsearch(typval_T *argvars, typval_T *rettv);
static void f_getcmdline(typval_T *argvars, typval_T *rettv);
#if defined(FEAT_CMDL_COMPL)
static void f_getcompletion(typval_T *argvars, typval_T *rettv);
#endif
static void f_getcmdpos(typval_T *argvars, typval_T *rettv);
static void f_getcmdtype(typval_T *argvars, typval_T *rettv);
static void f_getcmdwintype(typval_T *argvars, typval_T *rettv);
static void f_getcwd(typval_T *argvars, typval_T *rettv);
static void f_getfontname(typval_T *argvars, typval_T *rettv);
static void f_getfperm(typval_T *argvars, typval_T *rettv);
static void f_getfsize(typval_T *argvars, typval_T *rettv);
static void f_getftime(typval_T *argvars, typval_T *rettv);
static void f_getftype(typval_T *argvars, typval_T *rettv);
static void f_getjumplist(typval_T *argvars, typval_T *rettv);
static void f_getline(typval_T *argvars, typval_T *rettv);
static void f_getloclist(typval_T *argvars UNUSED, typval_T *rettv UNUSED);
static void f_getmatches(typval_T *argvars, typval_T *rettv);
static void f_getpid(typval_T *argvars, typval_T *rettv);
static void f_getcurpos(typval_T *argvars, typval_T *rettv);
static void f_getpos(typval_T *argvars, typval_T *rettv);
static void f_getqflist(typval_T *argvars, typval_T *rettv);
static void f_getreg(typval_T *argvars, typval_T *rettv);
static void f_getregtype(typval_T *argvars, typval_T *rettv);
static void f_gettabinfo(typval_T *argvars, typval_T *rettv);
static void f_gettabvar(typval_T *argvars, typval_T *rettv);
static void f_gettabwinvar(typval_T *argvars, typval_T *rettv);
static void f_gettagstack(typval_T *argvars, typval_T *rettv);
static void f_getwininfo(typval_T *argvars, typval_T *rettv);
static void f_getwinpos(typval_T *argvars, typval_T *rettv);
static void f_getwinposx(typval_T *argvars, typval_T *rettv);
static void f_getwinposy(typval_T *argvars, typval_T *rettv);
static void f_getwinvar(typval_T *argvars, typval_T *rettv);
static void f_glob(typval_T *argvars, typval_T *rettv);
static void f_globpath(typval_T *argvars, typval_T *rettv);
static void f_glob2regpat(typval_T *argvars, typval_T *rettv);
static void f_has(typval_T *argvars, typval_T *rettv);
static void f_has_key(typval_T *argvars, typval_T *rettv);
static void f_haslocaldir(typval_T *argvars, typval_T *rettv);
static void f_hasmapto(typval_T *argvars, typval_T *rettv);
static void f_histadd(typval_T *argvars, typval_T *rettv);
static void f_histdel(typval_T *argvars, typval_T *rettv);
static void f_histget(typval_T *argvars, typval_T *rettv);
static void f_histnr(typval_T *argvars, typval_T *rettv);
static void f_hlID(typval_T *argvars, typval_T *rettv);
static void f_hlexists(typval_T *argvars, typval_T *rettv);
static void f_hostname(typval_T *argvars, typval_T *rettv);
static void f_iconv(typval_T *argvars, typval_T *rettv);
static void f_indent(typval_T *argvars, typval_T *rettv);
static void f_index(typval_T *argvars, typval_T *rettv);
static void f_input(typval_T *argvars, typval_T *rettv);
static void f_inputdialog(typval_T *argvars, typval_T *rettv);
static void f_inputlist(typval_T *argvars, typval_T *rettv);
static void f_inputrestore(typval_T *argvars, typval_T *rettv);
static void f_inputsave(typval_T *argvars, typval_T *rettv);
static void f_inputsecret(typval_T *argvars, typval_T *rettv);
static void f_insert(typval_T *argvars, typval_T *rettv);
static void f_invert(typval_T *argvars, typval_T *rettv);
static void f_isdirectory(typval_T *argvars, typval_T *rettv);
static void f_islocked(typval_T *argvars, typval_T *rettv);
#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
static void f_isnan(typval_T *argvars, typval_T *rettv);
#endif
static void f_items(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_JOB_CHANNEL
static void f_job_getchannel(typval_T *argvars, typval_T *rettv);
static void f_job_info(typval_T *argvars, typval_T *rettv);
static void f_job_setoptions(typval_T *argvars, typval_T *rettv);
static void f_job_start(typval_T *argvars, typval_T *rettv);
static void f_job_stop(typval_T *argvars, typval_T *rettv);
static void f_job_status(typval_T *argvars, typval_T *rettv);
#endif
static void f_join(typval_T *argvars, typval_T *rettv);
static void f_js_decode(typval_T *argvars, typval_T *rettv);
static void f_js_encode(typval_T *argvars, typval_T *rettv);
static void f_json_decode(typval_T *argvars, typval_T *rettv);
static void f_json_encode(typval_T *argvars, typval_T *rettv);
static void f_keys(typval_T *argvars, typval_T *rettv);
static void f_last_buffer_nr(typval_T *argvars, typval_T *rettv);
static void f_len(typval_T *argvars, typval_T *rettv);
static void f_libcall(typval_T *argvars, typval_T *rettv);
static void f_libcallnr(typval_T *argvars, typval_T *rettv);
static void f_line(typval_T *argvars, typval_T *rettv);
static void f_line2byte(typval_T *argvars, typval_T *rettv);
static void f_lispindent(typval_T *argvars, typval_T *rettv);
static void f_localtime(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_log(typval_T *argvars, typval_T *rettv);
static void f_log10(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_LUA
static void f_luaeval(typval_T *argvars, typval_T *rettv);
#endif
static void f_map(typval_T *argvars, typval_T *rettv);
static void f_maparg(typval_T *argvars, typval_T *rettv);
static void f_mapcheck(typval_T *argvars, typval_T *rettv);
static void f_match(typval_T *argvars, typval_T *rettv);
static void f_matchadd(typval_T *argvars, typval_T *rettv);
static void f_matchaddpos(typval_T *argvars, typval_T *rettv);
static void f_matcharg(typval_T *argvars, typval_T *rettv);
static void f_matchdelete(typval_T *argvars, typval_T *rettv);
static void f_matchend(typval_T *argvars, typval_T *rettv);
static void f_matchlist(typval_T *argvars, typval_T *rettv);
static void f_matchstr(typval_T *argvars, typval_T *rettv);
static void f_matchstrpos(typval_T *argvars, typval_T *rettv);
static void f_max(typval_T *argvars, typval_T *rettv);
static void f_min(typval_T *argvars, typval_T *rettv);
#ifdef vim_mkdir
static void f_mkdir(typval_T *argvars, typval_T *rettv);
#endif
static void f_mode(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_MZSCHEME
static void f_mzeval(typval_T *argvars, typval_T *rettv);
#endif
static void f_nextnonblank(typval_T *argvars, typval_T *rettv);
static void f_nr2char(typval_T *argvars, typval_T *rettv);
static void f_or(typval_T *argvars, typval_T *rettv);
static void f_pathshorten(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_PERL
static void f_perleval(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_FLOAT
static void f_pow(typval_T *argvars, typval_T *rettv);
#endif
static void f_prevnonblank(typval_T *argvars, typval_T *rettv);
static void f_printf(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_JOB_CHANNEL
static void f_prompt_setcallback(typval_T *argvars, typval_T *rettv);
static void f_prompt_setinterrupt(typval_T *argvars, typval_T *rettv);
static void f_prompt_setprompt(typval_T *argvars, typval_T *rettv);
#endif
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
static void f_range(typval_T *argvars, typval_T *rettv);
static void f_readfile(typval_T *argvars, typval_T *rettv);
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
static void f_remove(typval_T *argvars, typval_T *rettv);
static void f_rename(typval_T *argvars, typval_T *rettv);
static void f_repeat(typval_T *argvars, typval_T *rettv);
static void f_resolve(typval_T *argvars, typval_T *rettv);
static void f_reverse(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_round(typval_T *argvars, typval_T *rettv);
#endif
static void f_screenattr(typval_T *argvars, typval_T *rettv);
static void f_screenchar(typval_T *argvars, typval_T *rettv);
static void f_screencol(typval_T *argvars, typval_T *rettv);
static void f_screenrow(typval_T *argvars, typval_T *rettv);
static void f_search(typval_T *argvars, typval_T *rettv);
static void f_searchdecl(typval_T *argvars, typval_T *rettv);
static void f_searchpair(typval_T *argvars, typval_T *rettv);
static void f_searchpairpos(typval_T *argvars, typval_T *rettv);
static void f_searchpos(typval_T *argvars, typval_T *rettv);
static void f_server2client(typval_T *argvars, typval_T *rettv);
static void f_serverlist(typval_T *argvars, typval_T *rettv);
static void f_setbufline(typval_T *argvars, typval_T *rettv);
static void f_setbufvar(typval_T *argvars, typval_T *rettv);
static void f_setcharsearch(typval_T *argvars, typval_T *rettv);
static void f_setcmdpos(typval_T *argvars, typval_T *rettv);
static void f_setfperm(typval_T *argvars, typval_T *rettv);
static void f_setline(typval_T *argvars, typval_T *rettv);
static void f_setloclist(typval_T *argvars, typval_T *rettv);
static void f_setmatches(typval_T *argvars, typval_T *rettv);
static void f_setpos(typval_T *argvars, typval_T *rettv);
static void f_setqflist(typval_T *argvars, typval_T *rettv);
static void f_setreg(typval_T *argvars, typval_T *rettv);
static void f_settabvar(typval_T *argvars, typval_T *rettv);
static void f_settabwinvar(typval_T *argvars, typval_T *rettv);
static void f_settagstack(typval_T *argvars, typval_T *rettv);
static void f_setwinvar(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_CRYPT
static void f_sha256(typval_T *argvars, typval_T *rettv);
#endif /* FEAT_CRYPT */
static void f_shellescape(typval_T *argvars, typval_T *rettv);
static void f_shiftwidth(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_SIGNS
static void f_sign_define(typval_T *argvars, typval_T *rettv);
static void f_sign_getdefined(typval_T *argvars, typval_T *rettv);
static void f_sign_getplaced(typval_T *argvars, typval_T *rettv);
static void f_sign_place(typval_T *argvars, typval_T *rettv);
static void f_sign_undefine(typval_T *argvars, typval_T *rettv);
static void f_sign_unplace(typval_T *argvars, typval_T *rettv);
#endif
static void f_simplify(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_sin(typval_T *argvars, typval_T *rettv);
static void f_sinh(typval_T *argvars, typval_T *rettv);
#endif
static void f_sort(typval_T *argvars, typval_T *rettv);
static void f_soundfold(typval_T *argvars, typval_T *rettv);
static void f_spellbadword(typval_T *argvars, typval_T *rettv);
static void f_spellsuggest(typval_T *argvars, typval_T *rettv);
static void f_split(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_sqrt(typval_T *argvars, typval_T *rettv);
static void f_str2float(typval_T *argvars, typval_T *rettv);
#endif
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
static void f_system(typval_T *argvars, typval_T *rettv);
static void f_systemlist(typval_T *argvars, typval_T *rettv);
static void f_tabpagebuflist(typval_T *argvars, typval_T *rettv);
static void f_tabpagenr(typval_T *argvars, typval_T *rettv);
static void f_tabpagewinnr(typval_T *argvars, typval_T *rettv);
static void f_taglist(typval_T *argvars, typval_T *rettv);
static void f_tagfiles(typval_T *argvars, typval_T *rettv);
static void f_tempname(typval_T *argvars, typval_T *rettv);
static void f_test_alloc_fail(typval_T *argvars, typval_T *rettv);
static void f_test_autochdir(typval_T *argvars, typval_T *rettv);
static void f_test_feedinput(typval_T *argvars, typval_T *rettv);
static void f_test_option_not_set(typval_T *argvars, typval_T *rettv);
static void f_test_override(typval_T *argvars, typval_T *rettv);
static void f_test_garbagecollect_now(typval_T *argvars, typval_T *rettv);
static void f_test_ignore_error(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_JOB_CHANNEL
static void f_test_null_channel(typval_T *argvars, typval_T *rettv);
#endif
static void f_test_null_dict(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_JOB_CHANNEL
static void f_test_null_job(typval_T *argvars, typval_T *rettv);
#endif
static void f_test_null_list(typval_T *argvars, typval_T *rettv);
static void f_test_null_partial(typval_T *argvars, typval_T *rettv);
static void f_test_null_string(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_GUI
static void f_test_scrollbar(typval_T *argvars, typval_T *rettv);
#endif
static void f_test_settime(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_tan(typval_T *argvars, typval_T *rettv);
static void f_tanh(typval_T *argvars, typval_T *rettv);
#endif
#ifdef FEAT_TIMERS
static void f_timer_info(typval_T *argvars, typval_T *rettv);
static void f_timer_pause(typval_T *argvars, typval_T *rettv);
static void f_timer_start(typval_T *argvars, typval_T *rettv);
static void f_timer_stop(typval_T *argvars, typval_T *rettv);
static void f_timer_stopall(typval_T *argvars, typval_T *rettv);
#endif
static void f_tolower(typval_T *argvars, typval_T *rettv);
static void f_toupper(typval_T *argvars, typval_T *rettv);
static void f_tr(typval_T *argvars, typval_T *rettv);
static void f_trim(typval_T *argvars, typval_T *rettv);
#ifdef FEAT_FLOAT
static void f_trunc(typval_T *argvars, typval_T *rettv);
#endif
static void f_type(typval_T *argvars, typval_T *rettv);
static void f_undofile(typval_T *argvars, typval_T *rettv);
static void f_undotree(typval_T *argvars, typval_T *rettv);
static void f_uniq(typval_T *argvars, typval_T *rettv);
static void f_values(typval_T *argvars, typval_T *rettv);
static void f_virtcol(typval_T *argvars, typval_T *rettv);
static void f_visualmode(typval_T *argvars, typval_T *rettv);
static void f_wildmenumode(typval_T *argvars, typval_T *rettv);
static void f_win_findbuf(typval_T *argvars, typval_T *rettv);
static void f_win_getid(typval_T *argvars, typval_T *rettv);
static void f_win_gotoid(typval_T *argvars, typval_T *rettv);
static void f_win_id2tabwin(typval_T *argvars, typval_T *rettv);
static void f_win_id2win(typval_T *argvars, typval_T *rettv);
static void f_win_screenpos(typval_T *argvars, typval_T *rettv);
static void f_winbufnr(typval_T *argvars, typval_T *rettv);
static void f_wincol(typval_T *argvars, typval_T *rettv);
static void f_winheight(typval_T *argvars, typval_T *rettv);
static void f_winlayout(typval_T *argvars, typval_T *rettv);
static void f_winline(typval_T *argvars, typval_T *rettv);
static void f_winnr(typval_T *argvars, typval_T *rettv);
static void f_winrestcmd(typval_T *argvars, typval_T *rettv);
static void f_winrestview(typval_T *argvars, typval_T *rettv);
static void f_winsaveview(typval_T *argvars, typval_T *rettv);
static void f_winwidth(typval_T *argvars, typval_T *rettv);
static void f_writefile(typval_T *argvars, typval_T *rettv);
static void f_wordcount(typval_T *argvars, typval_T *rettv);
static void f_xor(typval_T *argvars, typval_T *rettv);

/*
 * Array with names and number of arguments of all internal functions
 * MUST BE KEPT SORTED IN strcmp() ORDER FOR BINARY SEARCH!
 */
static struct fst
{
    char	*f_name;	/* function name */
    char	f_min_argc;	/* minimal number of arguments */
    char	f_max_argc;	/* maximal number of arguments */
    void	(*f_func)(typval_T *args, typval_T *rvar);
				/* implementation of function */
} functions[] =
{
#ifdef FEAT_FLOAT
    {"abs",		1, 1, f_abs},
    {"acos",		1, 1, f_acos},	/* WJMc */
#endif
    {"add",		2, 2, f_add},
    {"and",		2, 2, f_and},
    {"append",		2, 2, f_append},
    {"appendbufline",	3, 3, f_appendbufline},
    {"argc",		0, 1, f_argc},
    {"argidx",		0, 0, f_argidx},
    {"arglistid",	0, 2, f_arglistid},
    {"argv",		0, 2, f_argv},
#ifdef FEAT_FLOAT
    {"asin",		1, 1, f_asin},	/* WJMc */
#endif
    {"assert_beeps",	1, 2, f_assert_beeps},
    {"assert_equal",	2, 3, f_assert_equal},
    {"assert_equalfile", 2, 2, f_assert_equalfile},
    {"assert_exception", 1, 2, f_assert_exception},
    {"assert_fails",	1, 3, f_assert_fails},
    {"assert_false",	1, 2, f_assert_false},
    {"assert_inrange",	3, 4, f_assert_inrange},
    {"assert_match",	2, 3, f_assert_match},
    {"assert_notequal",	2, 3, f_assert_notequal},
    {"assert_notmatch",	2, 3, f_assert_notmatch},
    {"assert_report",	1, 1, f_assert_report},
    {"assert_true",	1, 2, f_assert_true},
#ifdef FEAT_FLOAT
    {"atan",		1, 1, f_atan},
    {"atan2",		2, 2, f_atan2},
#endif
#ifdef FEAT_BEVAL
    {"balloon_show",	1, 1, f_balloon_show},
# if defined(FEAT_BEVAL_TERM)
    {"balloon_split",	1, 1, f_balloon_split},
# endif
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
    {"bufwinid",	1, 1, f_bufwinid},
    {"bufwinnr",	1, 1, f_bufwinnr},
    {"byte2line",	1, 1, f_byte2line},
    {"byteidx",		2, 2, f_byteidx},
    {"byteidxcomp",	2, 2, f_byteidxcomp},
    {"call",		2, 3, f_call},
#ifdef FEAT_FLOAT
    {"ceil",		1, 1, f_ceil},
#endif
#ifdef FEAT_JOB_CHANNEL
    {"ch_canread",	1, 1, f_ch_canread},
    {"ch_close",	1, 1, f_ch_close},
    {"ch_close_in",	1, 1, f_ch_close_in},
    {"ch_evalexpr",	2, 3, f_ch_evalexpr},
    {"ch_evalraw",	2, 3, f_ch_evalraw},
    {"ch_getbufnr",	2, 2, f_ch_getbufnr},
    {"ch_getjob",	1, 1, f_ch_getjob},
    {"ch_info",		1, 1, f_ch_info},
    {"ch_log",		1, 2, f_ch_log},
    {"ch_logfile",	1, 2, f_ch_logfile},
    {"ch_open",		1, 2, f_ch_open},
    {"ch_read",		1, 2, f_ch_read},
    {"ch_readraw",	1, 2, f_ch_readraw},
    {"ch_sendexpr",	2, 3, f_ch_sendexpr},
    {"ch_sendraw",	2, 3, f_ch_sendraw},
    {"ch_setoptions",	2, 2, f_ch_setoptions},
    {"ch_status",	1, 2, f_ch_status},
#endif
    {"changenr",	0, 0, f_changenr},
    {"char2nr",		1, 2, f_char2nr},
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
    {"cosh",		1, 1, f_cosh},
#endif
    {"count",		2, 4, f_count},
    {"cscope_connection",0,3, f_cscope_connection},
    {"cursor",		1, 3, f_cursor},
#ifdef WIN3264
    {"debugbreak",	1, 1, f_debugbreak},
#endif
    {"deepcopy",	1, 2, f_deepcopy},
    {"delete",		1, 2, f_delete},
    {"deletebufline",	2, 3, f_deletebufline},
    {"did_filetype",	0, 0, f_did_filetype},
    {"diff_filler",	1, 1, f_diff_filler},
    {"diff_hlID",	2, 2, f_diff_hlID},
    {"empty",		1, 1, f_empty},
    {"escape",		2, 2, f_escape},
    {"eval",		1, 1, f_eval},
    {"eventhandler",	0, 0, f_eventhandler},
    {"executable",	1, 1, f_executable},
    {"execute",		1, 2, f_execute},
    {"exepath",		1, 1, f_exepath},
    {"exists",		1, 1, f_exists},
#ifdef FEAT_FLOAT
    {"exp",		1, 1, f_exp},
#endif
    {"expand",		1, 3, f_expand},
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
    {"fmod",		2, 2, f_fmod},
#endif
    {"fnameescape",	1, 1, f_fnameescape},
    {"fnamemodify",	2, 2, f_fnamemodify},
    {"foldclosed",	1, 1, f_foldclosed},
    {"foldclosedend",	1, 1, f_foldclosedend},
    {"foldlevel",	1, 1, f_foldlevel},
    {"foldtext",	0, 0, f_foldtext},
    {"foldtextresult",	1, 1, f_foldtextresult},
    {"foreground",	0, 0, f_foreground},
    {"funcref",		1, 3, f_funcref},
    {"function",	1, 3, f_function},
    {"garbagecollect",	0, 1, f_garbagecollect},
    {"get",		2, 3, f_get},
    {"getbufinfo",	0, 1, f_getbufinfo},
    {"getbufline",	2, 3, f_getbufline},
    {"getbufvar",	2, 3, f_getbufvar},
    {"getchangelist",	1, 1, f_getchangelist},
    {"getchar",		0, 1, f_getchar},
    {"getcharmod",	0, 0, f_getcharmod},
    {"getcharsearch",	0, 0, f_getcharsearch},
    {"getcmdline",	0, 0, f_getcmdline},
    {"getcmdpos",	0, 0, f_getcmdpos},
    {"getcmdtype",	0, 0, f_getcmdtype},
    {"getcmdwintype",	0, 0, f_getcmdwintype},
#if defined(FEAT_CMDL_COMPL)
    {"getcompletion",	2, 3, f_getcompletion},
#endif
    {"getcurpos",	0, 0, f_getcurpos},
    {"getcwd",		0, 2, f_getcwd},
    {"getfontname",	0, 1, f_getfontname},
    {"getfperm",	1, 1, f_getfperm},
    {"getfsize",	1, 1, f_getfsize},
    {"getftime",	1, 1, f_getftime},
    {"getftype",	1, 1, f_getftype},
    {"getjumplist",	0, 2, f_getjumplist},
    {"getline",		1, 2, f_getline},
    {"getloclist",	1, 2, f_getloclist},
    {"getmatches",	0, 0, f_getmatches},
    {"getpid",		0, 0, f_getpid},
    {"getpos",		1, 1, f_getpos},
    {"getqflist",	0, 1, f_getqflist},
    {"getreg",		0, 3, f_getreg},
    {"getregtype",	0, 1, f_getregtype},
    {"gettabinfo",	0, 1, f_gettabinfo},
    {"gettabvar",	2, 3, f_gettabvar},
    {"gettabwinvar",	3, 4, f_gettabwinvar},
    {"gettagstack",	0, 1, f_gettagstack},
    {"getwininfo",	0, 1, f_getwininfo},
    {"getwinpos",	0, 1, f_getwinpos},
    {"getwinposx",	0, 0, f_getwinposx},
    {"getwinposy",	0, 0, f_getwinposy},
    {"getwinvar",	2, 3, f_getwinvar},
    {"glob",		1, 4, f_glob},
    {"glob2regpat",	1, 1, f_glob2regpat},
    {"globpath",	2, 5, f_globpath},
    {"has",		1, 1, f_has},
    {"has_key",		2, 2, f_has_key},
    {"haslocaldir",	0, 2, f_haslocaldir},
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
    {"invert",		1, 1, f_invert},
    {"isdirectory",	1, 1, f_isdirectory},
    {"islocked",	1, 1, f_islocked},
#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
    {"isnan",		1, 1, f_isnan},
#endif
    {"items",		1, 1, f_items},
#ifdef FEAT_JOB_CHANNEL
    {"job_getchannel",	1, 1, f_job_getchannel},
    {"job_info",	0, 1, f_job_info},
    {"job_setoptions",	2, 2, f_job_setoptions},
    {"job_start",	1, 2, f_job_start},
    {"job_status",	1, 1, f_job_status},
    {"job_stop",	1, 2, f_job_stop},
#endif
    {"join",		1, 2, f_join},
    {"js_decode",	1, 1, f_js_decode},
    {"js_encode",	1, 1, f_js_encode},
    {"json_decode",	1, 1, f_json_decode},
    {"json_encode",	1, 1, f_json_encode},
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
    {"log",		1, 1, f_log},
    {"log10",		1, 1, f_log10},
#endif
#ifdef FEAT_LUA
    {"luaeval",		1, 2, f_luaeval},
#endif
    {"map",		2, 2, f_map},
    {"maparg",		1, 4, f_maparg},
    {"mapcheck",	1, 3, f_mapcheck},
    {"match",		2, 4, f_match},
    {"matchadd",	2, 5, f_matchadd},
    {"matchaddpos",	2, 5, f_matchaddpos},
    {"matcharg",	1, 1, f_matcharg},
    {"matchdelete",	1, 1, f_matchdelete},
    {"matchend",	2, 4, f_matchend},
    {"matchlist",	2, 4, f_matchlist},
    {"matchstr",	2, 4, f_matchstr},
    {"matchstrpos",	2, 4, f_matchstrpos},
    {"max",		1, 1, f_max},
    {"min",		1, 1, f_min},
#ifdef vim_mkdir
    {"mkdir",		1, 3, f_mkdir},
#endif
    {"mode",		0, 1, f_mode},
#ifdef FEAT_MZSCHEME
    {"mzeval",		1, 1, f_mzeval},
#endif
    {"nextnonblank",	1, 1, f_nextnonblank},
    {"nr2char",		1, 2, f_nr2char},
    {"or",		2, 2, f_or},
    {"pathshorten",	1, 1, f_pathshorten},
#ifdef FEAT_PERL
    {"perleval",	1, 1, f_perleval},
#endif
#ifdef FEAT_FLOAT
    {"pow",		2, 2, f_pow},
#endif
    {"prevnonblank",	1, 1, f_prevnonblank},
    {"printf",		1, 19, f_printf},
#ifdef FEAT_JOB_CHANNEL
    {"prompt_setcallback", 2, 2, f_prompt_setcallback},
    {"prompt_setinterrupt", 2, 2, f_prompt_setinterrupt},
    {"prompt_setprompt", 2, 2, f_prompt_setprompt},
#endif
#ifdef FEAT_TEXT_PROP
    {"prop_add",	3, 3, f_prop_add},
    {"prop_clear",	1, 3, f_prop_clear},
    {"prop_list",	1, 2, f_prop_list},
    {"prop_remove",	2, 3, f_prop_remove},
    {"prop_type_add",	2, 2, f_prop_type_add},
    {"prop_type_change", 2, 2, f_prop_type_change},
    {"prop_type_delete", 1, 2, f_prop_type_delete},
    {"prop_type_get",	1, 2, f_prop_type_get},
    {"prop_type_list",	0, 1, f_prop_type_list},
#endif
    {"pumvisible",	0, 0, f_pumvisible},
#ifdef FEAT_PYTHON3
    {"py3eval",		1, 1, f_py3eval},
#endif
#ifdef FEAT_PYTHON
    {"pyeval",		1, 1, f_pyeval},
#endif
#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
    {"pyxeval",		1, 1, f_pyxeval},
#endif
    {"range",		1, 3, f_range},
    {"readfile",	1, 3, f_readfile},
    {"reg_executing",	0, 0, f_reg_executing},
    {"reg_recording",	0, 0, f_reg_recording},
    {"reltime",		0, 2, f_reltime},
#ifdef FEAT_FLOAT
    {"reltimefloat",	1, 1, f_reltimefloat},
#endif
    {"reltimestr",	1, 1, f_reltimestr},
    {"remote_expr",	2, 4, f_remote_expr},
    {"remote_foreground", 1, 1, f_remote_foreground},
    {"remote_peek",	1, 2, f_remote_peek},
    {"remote_read",	1, 2, f_remote_read},
    {"remote_send",	2, 3, f_remote_send},
    {"remote_startserver", 1, 1, f_remote_startserver},
    {"remove",		2, 3, f_remove},
    {"rename",		2, 2, f_rename},
    {"repeat",		2, 2, f_repeat},
    {"resolve",		1, 1, f_resolve},
    {"reverse",		1, 1, f_reverse},
#ifdef FEAT_FLOAT
    {"round",		1, 1, f_round},
#endif
    {"screenattr",	2, 2, f_screenattr},
    {"screenchar",	2, 2, f_screenchar},
    {"screencol",	0, 0, f_screencol},
    {"screenrow",	0, 0, f_screenrow},
    {"search",		1, 4, f_search},
    {"searchdecl",	1, 3, f_searchdecl},
    {"searchpair",	3, 7, f_searchpair},
    {"searchpairpos",	3, 7, f_searchpairpos},
    {"searchpos",	1, 4, f_searchpos},
    {"server2client",	2, 2, f_server2client},
    {"serverlist",	0, 0, f_serverlist},
    {"setbufline",	3, 3, f_setbufline},
    {"setbufvar",	3, 3, f_setbufvar},
    {"setcharsearch",	1, 1, f_setcharsearch},
    {"setcmdpos",	1, 1, f_setcmdpos},
    {"setfperm",	2, 2, f_setfperm},
    {"setline",		2, 2, f_setline},
    {"setloclist",	2, 4, f_setloclist},
    {"setmatches",	1, 1, f_setmatches},
    {"setpos",		2, 2, f_setpos},
    {"setqflist",	1, 3, f_setqflist},
    {"setreg",		2, 3, f_setreg},
    {"settabvar",	3, 3, f_settabvar},
    {"settabwinvar",	4, 4, f_settabwinvar},
    {"settagstack",	2, 3, f_settagstack},
    {"setwinvar",	3, 3, f_setwinvar},
#ifdef FEAT_CRYPT
    {"sha256",		1, 1, f_sha256},
#endif
    {"shellescape",	1, 2, f_shellescape},
    {"shiftwidth",	0, 1, f_shiftwidth},
#ifdef FEAT_SIGNS
    {"sign_define",	1, 2, f_sign_define},
    {"sign_getdefined",	0, 1, f_sign_getdefined},
    {"sign_getplaced",	0, 2, f_sign_getplaced},
    {"sign_place",	4, 5, f_sign_place},
    {"sign_undefine",	0, 1, f_sign_undefine},
    {"sign_unplace",	1, 2, f_sign_unplace},
#endif
    {"simplify",	1, 1, f_simplify},
#ifdef FEAT_FLOAT
    {"sin",		1, 1, f_sin},
    {"sinh",		1, 1, f_sinh},
#endif
    {"sort",		1, 3, f_sort},
    {"soundfold",	1, 1, f_soundfold},
    {"spellbadword",	0, 1, f_spellbadword},
    {"spellsuggest",	1, 3, f_spellsuggest},
    {"split",		1, 3, f_split},
#ifdef FEAT_FLOAT
    {"sqrt",		1, 1, f_sqrt},
    {"str2float",	1, 1, f_str2float},
#endif
    {"str2nr",		1, 2, f_str2nr},
    {"strcharpart",	2, 3, f_strcharpart},
    {"strchars",	1, 2, f_strchars},
    {"strdisplaywidth",	1, 2, f_strdisplaywidth},
#ifdef HAVE_STRFTIME
    {"strftime",	1, 2, f_strftime},
#endif
    {"strgetchar",	2, 2, f_strgetchar},
    {"stridx",		2, 3, f_stridx},
    {"string",		1, 1, f_string},
    {"strlen",		1, 1, f_strlen},
    {"strpart",		2, 3, f_strpart},
    {"strridx",		2, 3, f_strridx},
    {"strtrans",	1, 1, f_strtrans},
    {"strwidth",	1, 1, f_strwidth},
    {"submatch",	1, 2, f_submatch},
    {"substitute",	4, 4, f_substitute},
    {"swapinfo",	1, 1, f_swapinfo},
    {"swapname",	1, 1, f_swapname},
    {"synID",		3, 3, f_synID},
    {"synIDattr",	2, 3, f_synIDattr},
    {"synIDtrans",	1, 1, f_synIDtrans},
    {"synconcealed",	2, 2, f_synconcealed},
    {"synstack",	2, 2, f_synstack},
    {"system",		1, 2, f_system},
    {"systemlist",	1, 2, f_systemlist},
    {"tabpagebuflist",	0, 1, f_tabpagebuflist},
    {"tabpagenr",	0, 1, f_tabpagenr},
    {"tabpagewinnr",	1, 2, f_tabpagewinnr},
    {"tagfiles",	0, 0, f_tagfiles},
    {"taglist",		1, 2, f_taglist},
#ifdef FEAT_FLOAT
    {"tan",		1, 1, f_tan},
    {"tanh",		1, 1, f_tanh},
#endif
    {"tempname",	0, 0, f_tempname},
#ifdef FEAT_TERMINAL
    {"term_dumpdiff",	2, 3, f_term_dumpdiff},
    {"term_dumpload",	1, 2, f_term_dumpload},
    {"term_dumpwrite",	2, 3, f_term_dumpwrite},
    {"term_getaltscreen", 1, 1, f_term_getaltscreen},
# if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
    {"term_getansicolors", 1, 1, f_term_getansicolors},
# endif
    {"term_getattr",	2, 2, f_term_getattr},
    {"term_getcursor",	1, 1, f_term_getcursor},
    {"term_getjob",	1, 1, f_term_getjob},
    {"term_getline",	2, 2, f_term_getline},
    {"term_getscrolled", 1, 1, f_term_getscrolled},
    {"term_getsize",	1, 1, f_term_getsize},
    {"term_getstatus",	1, 1, f_term_getstatus},
    {"term_gettitle",	1, 1, f_term_gettitle},
    {"term_gettty",	1, 2, f_term_gettty},
    {"term_list",	0, 0, f_term_list},
    {"term_scrape",	2, 2, f_term_scrape},
    {"term_sendkeys",	2, 2, f_term_sendkeys},
# if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
    {"term_setansicolors", 2, 2, f_term_setansicolors},
# endif
    {"term_setkill",	2, 2, f_term_setkill},
    {"term_setrestore",	2, 2, f_term_setrestore},
    {"term_setsize",	3, 3, f_term_setsize},
    {"term_start",	1, 2, f_term_start},
    {"term_wait",	1, 2, f_term_wait},
#endif
    {"test_alloc_fail",	3, 3, f_test_alloc_fail},
    {"test_autochdir",	0, 0, f_test_autochdir},
    {"test_feedinput",	1, 1, f_test_feedinput},
    {"test_garbagecollect_now",	0, 0, f_test_garbagecollect_now},
    {"test_ignore_error",	1, 1, f_test_ignore_error},
#ifdef FEAT_JOB_CHANNEL
    {"test_null_channel", 0, 0, f_test_null_channel},
#endif
    {"test_null_dict",	0, 0, f_test_null_dict},
#ifdef FEAT_JOB_CHANNEL
    {"test_null_job",	0, 0, f_test_null_job},
#endif
    {"test_null_list",	0, 0, f_test_null_list},
    {"test_null_partial", 0, 0, f_test_null_partial},
    {"test_null_string", 0, 0, f_test_null_string},
    {"test_option_not_set", 1, 1, f_test_option_not_set},
    {"test_override",    2, 2, f_test_override},
#ifdef FEAT_GUI
    {"test_scrollbar",	3, 3, f_test_scrollbar},
#endif
    {"test_settime",	1, 1, f_test_settime},
#ifdef FEAT_TIMERS
    {"timer_info",	0, 1, f_timer_info},
    {"timer_pause",	2, 2, f_timer_pause},
    {"timer_start",	2, 3, f_timer_start},
    {"timer_stop",	1, 1, f_timer_stop},
    {"timer_stopall",	0, 0, f_timer_stopall},
#endif
    {"tolower",		1, 1, f_tolower},
    {"toupper",		1, 1, f_toupper},
    {"tr",		3, 3, f_tr},
    {"trim",		1, 2, f_trim},
#ifdef FEAT_FLOAT
    {"trunc",		1, 1, f_trunc},
#endif
    {"type",		1, 1, f_type},
    {"undofile",	1, 1, f_undofile},
    {"undotree",	0, 0, f_undotree},
    {"uniq",		1, 3, f_uniq},
    {"values",		1, 1, f_values},
    {"virtcol",		1, 1, f_virtcol},
    {"visualmode",	0, 1, f_visualmode},
    {"wildmenumode",	0, 0, f_wildmenumode},
    {"win_findbuf",	1, 1, f_win_findbuf},
    {"win_getid",	0, 2, f_win_getid},
    {"win_gotoid",	1, 1, f_win_gotoid},
    {"win_id2tabwin",	1, 1, f_win_id2tabwin},
    {"win_id2win",	1, 1, f_win_id2win},
    {"win_screenpos",	1, 1, f_win_screenpos},
    {"winbufnr",	1, 1, f_winbufnr},
    {"wincol",		0, 0, f_wincol},
    {"winheight",	1, 1, f_winheight},
    {"winlayout",	0, 1, f_winlayout},
    {"winline",		0, 0, f_winline},
    {"winnr",		0, 1, f_winnr},
    {"winrestcmd",	0, 0, f_winrestcmd},
    {"winrestview",	1, 1, f_winrestview},
    {"winsaveview",	0, 0, f_winsaveview},
    {"winwidth",	1, 1, f_winwidth},
    {"wordcount",	0, 0, f_wordcount},
    {"writefile",	2, 3, f_writefile},
    {"xor",		2, 2, f_xor},
};

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)

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

#endif /* FEAT_CMDL_COMPL */

/*
 * Find internal function in table above.
 * Return index, or -1 if not found
 */
    int
find_internal_func(
    char_u	*name)		/* name of the function */
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
	return ERROR_UNKNOWN;
    if (argcount < functions[i].f_min_argc)
	return ERROR_TOOFEW;
    if (argcount > functions[i].f_max_argc)
	return ERROR_TOOMANY;
    argvars[argcount].v_type = VAR_UNKNOWN;
    functions[i].f_func(argvars, rettv);
    return ERROR_NONE;
}

/*
 * Return TRUE for a non-zero Number and a non-empty String.
 */
    static int
non_zero_arg(typval_T *argvars)
{
    return ((argvars[0].v_type == VAR_NUMBER
		&& argvars[0].vval.v_number != 0)
	    || (argvars[0].v_type == VAR_SPECIAL
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
    static linenr_T
tv_get_lnum(typval_T *argvars)
{
    typval_T	rettv;
    linenr_T	lnum;

    lnum = (linenr_T)tv_get_number_chk(&argvars[0], NULL);
    if (lnum == 0)  /* no valid number, try using line() */
    {
	rettv.v_type = VAR_NUMBER;
	f_line(argvars, &rettv);
	lnum = (linenr_T)rettv.vval.v_number;
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
    EMSG(_("E808: Number or Float required"));
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
 * "add(list, item)" function
 */
    static void
f_add(typval_T *argvars, typval_T *rettv)
{
    list_T	*l;

    rettv->vval.v_number = 1; /* Default: Failed */
    if (argvars[0].v_type == VAR_LIST)
    {
	if ((l = argvars[0].vval.v_list) != NULL
		&& !tv_check_lock(l->lv_lock,
					 (char_u *)N_("add() argument"), TRUE)
		&& list_append_tv(l, &argvars[1]) == OK)
	    copy_tv(&argvars[0], rettv);
    }
    else
	EMSG(_(e_listreq));
}

/*
 * "and(expr, expr)" function
 */
    static void
f_and(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = tv_get_number_chk(&argvars[0], NULL)
					& tv_get_number_chk(&argvars[1], NULL);
}

/*
 * If there is a window for "curbuf", make it the current window.
 */
    static void
find_win_for_curbuf(void)
{
    wininfo_T *wip;

    for (wip = curbuf->b_wininfo; wip != NULL; wip = wip->wi_next)
    {
	if (wip->wi_win != NULL)
	{
	    curwin = wip->wi_win;
	    break;
	}
    }
}

/*
 * Set line or list of lines in buffer "buf".
 */
    static void
set_buffer_lines(
	buf_T	    *buf,
	linenr_T    lnum_arg,
	int	    append,
	typval_T    *lines,
	typval_T    *rettv)
{
    linenr_T    lnum = lnum_arg + (append ? 1 : 0);
    char_u	*line = NULL;
    list_T	*l = NULL;
    listitem_T	*li = NULL;
    long	added = 0;
    linenr_T	append_lnum;
    buf_T	*curbuf_save = NULL;
    win_T	*curwin_save = NULL;
    int		is_curbuf = buf == curbuf;

    /* When using the current buffer ml_mfp will be set if needed.  Useful when
     * setline() is used on startup.  For other buffers the buffer must be
     * loaded. */
    if (buf == NULL || (!is_curbuf && buf->b_ml.ml_mfp == NULL) || lnum < 1)
    {
	rettv->vval.v_number = 1;	/* FAIL */
	return;
    }

    if (!is_curbuf)
    {
	curbuf_save = curbuf;
	curwin_save = curwin;
	curbuf = buf;
	find_win_for_curbuf();
    }

    if (append)
	// appendbufline() uses the line number below which we insert
	append_lnum = lnum - 1;
    else
	// setbufline() uses the line number above which we insert, we only
	// append if it's below the last line
	append_lnum = curbuf->b_ml.ml_line_count;

    if (lines->v_type == VAR_LIST)
    {
	l = lines->vval.v_list;
	li = l->lv_first;
    }
    else
	line = tv_get_string_chk(lines);

    /* default result is zero == OK */
    for (;;)
    {
	if (l != NULL)
	{
	    /* list argument, get next string */
	    if (li == NULL)
		break;
	    line = tv_get_string_chk(&li->li_tv);
	    li = li->li_next;
	}

	rettv->vval.v_number = 1;	/* FAIL */
	if (line == NULL || lnum > curbuf->b_ml.ml_line_count + 1)
	    break;

	/* When coming here from Insert mode, sync undo, so that this can be
	 * undone separately from what was previously inserted. */
	if (u_sync_once == 2)
	{
	    u_sync_once = 1; /* notify that u_sync() was called */
	    u_sync(TRUE);
	}

	if (!append && lnum <= curbuf->b_ml.ml_line_count)
	{
	    // Existing line, replace it.
	    // Removes any existing text properties.
	    if (u_savesub(lnum) == OK && ml_replace_len(
		      lnum, line, (colnr_T)STRLEN(line) + 1, TRUE, TRUE) == OK)
	    {
		changed_bytes(lnum, 0);
		if (is_curbuf && lnum == curwin->w_cursor.lnum)
		    check_cursor_col();
		rettv->vval.v_number = 0;	/* OK */
	    }
	}
	else if (added > 0 || u_save(lnum - 1, lnum) == OK)
	{
	    /* append the line */
	    ++added;
	    if (ml_append(lnum - 1, line, (colnr_T)0, FALSE) == OK)
		rettv->vval.v_number = 0;	/* OK */
	}

	if (l == NULL)			/* only one string argument */
	    break;
	++lnum;
    }

    if (added > 0)
    {
	win_T	    *wp;
	tabpage_T   *tp;

	appended_lines_mark(append_lnum, added);
	FOR_ALL_TAB_WINDOWS(tp, wp)
	    if (wp->w_buffer == buf && wp->w_cursor.lnum > append_lnum)
		wp->w_cursor.lnum += added;
	check_cursor_col();

#ifdef FEAT_JOB_CHANNEL
	if (bt_prompt(curbuf) && (State & INSERT))
	    // show the line with the prompt
	    update_topline();
#endif
    }

    if (!is_curbuf)
    {
	curbuf = curbuf_save;
	curwin = curwin_save;
    }
}

/*
 * "append(lnum, string/list)" function
 */
    static void
f_append(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum = tv_get_lnum(&argvars[0]);

    set_buffer_lines(curbuf, lnum, TRUE, &argvars[1], rettv);
}

/*
 * "appendbufline(buf, lnum, string/list)" function
 */
    static void
f_appendbufline(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;
    buf_T	*buf;

    buf = tv_get_buf(&argvars[0], FALSE);
    if (buf == NULL)
	rettv->vval.v_number = 1; /* FAIL */
    else
    {
	lnum = tv_get_lnum_buf(&argvars[1], buf);
	set_buffer_lines(buf, lnum, TRUE, &argvars[2], rettv);
    }
}

/*
 * "argc([window id])" function
 */
    static void
f_argc(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp;

    if (argvars[0].v_type == VAR_UNKNOWN)
	// use the current window
	rettv->vval.v_number = ARGCOUNT;
    else if (argvars[0].v_type == VAR_NUMBER
					   && tv_get_number(&argvars[0]) == -1)
	// use the global argument list
	rettv->vval.v_number = GARGCOUNT;
    else
    {
	// use the argument list of the specified window
	wp = find_win_by_nr_or_id(&argvars[0]);
	if (wp != NULL)
	    rettv->vval.v_number = WARGCOUNT(wp);
	else
	    rettv->vval.v_number = -1;
    }
}

/*
 * "argidx()" function
 */
    static void
f_argidx(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = curwin->w_arg_idx;
}

/*
 * "arglistid()" function
 */
    static void
f_arglistid(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp;

    rettv->vval.v_number = -1;
    wp = find_tabwin(&argvars[0], &argvars[1]);
    if (wp != NULL)
	rettv->vval.v_number = wp->w_alist->id;
}

/*
 * Get the argument list for a given window
 */
    static void
get_arglist_as_rettv(aentry_T *arglist, int argcount, typval_T *rettv)
{
    int		idx;

    if (rettv_list_alloc(rettv) == OK && arglist != NULL)
	for (idx = 0; idx < argcount; ++idx)
	    list_append_string(rettv->vval.v_list,
						alist_name(&arglist[idx]), -1);
}

/*
 * "argv(nr)" function
 */
    static void
f_argv(typval_T *argvars, typval_T *rettv)
{
    int		idx;
    aentry_T	*arglist = NULL;
    int		argcount = -1;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	if (argvars[1].v_type == VAR_UNKNOWN)
	{
	    arglist = ARGLIST;
	    argcount = ARGCOUNT;
	}
	else if (argvars[1].v_type == VAR_NUMBER
					   && tv_get_number(&argvars[1]) == -1)
	{
	    arglist = GARGLIST;
	    argcount = GARGCOUNT;
	}
	else
	{
	    win_T	*wp = find_win_by_nr_or_id(&argvars[1]);

	    if (wp != NULL)
	    {
		/* Use the argument list of the specified window */
		arglist = WARGLIST(wp);
		argcount = WARGCOUNT(wp);
	    }
	}

	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = NULL;
	idx = tv_get_number_chk(&argvars[0], NULL);
	if (arglist != NULL && idx >= 0 && idx < argcount)
	    rettv->vval.v_string = vim_strsave(alist_name(&arglist[idx]));
	else if (idx == -1)
	    get_arglist_as_rettv(arglist, argcount, rettv);
    }
    else
	get_arglist_as_rettv(ARGLIST, ARGCOUNT, rettv);
}

/*
 * "assert_beeps(cmd [, error])" function
 */
    static void
f_assert_beeps(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_beeps(argvars);
}

/*
 * "assert_equal(expected, actual[, msg])" function
 */
    static void
f_assert_equal(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_equal_common(argvars, ASSERT_EQUAL);
}

/*
 * "assert_equalfile(fname-one, fname-two)" function
 */
    static void
f_assert_equalfile(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_equalfile(argvars);
}

/*
 * "assert_notequal(expected, actual[, msg])" function
 */
    static void
f_assert_notequal(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_equal_common(argvars, ASSERT_NOTEQUAL);
}

/*
 * "assert_exception(string[, msg])" function
 */
    static void
f_assert_exception(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_exception(argvars);
}

/*
 * "assert_fails(cmd [, error[, msg]])" function
 */
    static void
f_assert_fails(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_fails(argvars);
}

/*
 * "assert_false(actual[, msg])" function
 */
    static void
f_assert_false(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_bool(argvars, FALSE);
}

/*
 * "assert_inrange(lower, upper[, msg])" function
 */
    static void
f_assert_inrange(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_inrange(argvars);
}

/*
 * "assert_match(pattern, actual[, msg])" function
 */
    static void
f_assert_match(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_match_common(argvars, ASSERT_MATCH);
}

/*
 * "assert_notmatch(pattern, actual[, msg])" function
 */
    static void
f_assert_notmatch(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_match_common(argvars, ASSERT_NOTMATCH);
}

/*
 * "assert_report(msg)" function
 */
    static void
f_assert_report(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_report(argvars);
}

/*
 * "assert_true(actual[, msg])" function
 */
    static void
f_assert_true(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = assert_bool(argvars, TRUE);
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
f_balloon_show(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (balloonEval != NULL)
    {
	if (argvars[0].v_type == VAR_LIST
# ifdef FEAT_GUI
		&& !gui.in_use
# endif
	   )
	    post_balloon(balloonEval, NULL, argvars[0].vval.v_list);
	else
	    post_balloon(balloonEval, tv_get_string_chk(&argvars[0]), NULL);
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

	    /* Skip the first and last item, they are always empty. */
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
 * "browse(save, title, initdir, default)" function
 */
    static void
f_browse(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_BROWSE
    int		save;
    char_u	*title;
    char_u	*initdir;
    char_u	*defname;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    int		error = FALSE;

    save = (int)tv_get_number_chk(&argvars[0], &error);
    title = tv_get_string_chk(&argvars[1]);
    initdir = tv_get_string_buf_chk(&argvars[2], buf);
    defname = tv_get_string_buf_chk(&argvars[3], buf2);

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
    static void
f_browsedir(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_BROWSE
    char_u	*title;
    char_u	*initdir;
    char_u	buf[NUMBUFLEN];

    title = tv_get_string_chk(&argvars[0]);
    initdir = tv_get_string_buf_chk(&argvars[1], buf);

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

/*
 * Find a buffer by number or exact name.
 */
    static buf_T *
find_buffer(typval_T *avar)
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
	    FOR_ALL_BUFFERS(buf)
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
f_bufexists(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = (find_buffer(&argvars[0]) != NULL);
}

/*
 * "buflisted(expr)" function
 */
    static void
f_buflisted(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf;

    buf = find_buffer(&argvars[0]);
    rettv->vval.v_number = (buf != NULL && buf->b_p_bl);
}

/*
 * "bufloaded(expr)" function
 */
    static void
f_bufloaded(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf;

    buf = find_buffer(&argvars[0]);
    rettv->vval.v_number = (buf != NULL && buf->b_ml.ml_mfp != NULL);
}

    buf_T *
buflist_find_by_name(char_u *name, int curtab_only)
{
    int		save_magic;
    char_u	*save_cpo;
    buf_T	*buf;

    /* Ignore 'magic' and 'cpoptions' here to make scripts portable */
    save_magic = p_magic;
    p_magic = TRUE;
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    buf = buflist_findnr(buflist_findpat(name, name + STRLEN(name),
						    TRUE, FALSE, curtab_only));

    p_magic = save_magic;
    p_cpo = save_cpo;
    return buf;
}

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

    /* If not found, try expanding the name, like done for bufexists(). */
    if (buf == NULL)
	buf = find_buffer(tv);

    return buf;
}

/*
 * "bufname(expr)" function
 */
    static void
f_bufname(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf;

    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = tv_get_buf(&argvars[0], FALSE);
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
f_bufnr(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf;
    int		error = FALSE;
    char_u	*name;

    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = tv_get_buf(&argvars[0], FALSE);
    --emsg_off;

    /* If the buffer isn't found and the second argument is not zero create a
     * new buffer. */
    if (buf == NULL
	    && argvars[1].v_type != VAR_UNKNOWN
	    && tv_get_number_chk(&argvars[1], &error) != 0
	    && !error
	    && (name = tv_get_string_chk(&argvars[0])) != NULL
	    && !error)
	buf = buflist_new(name, NULL, (linenr_T)1, 0);

    if (buf != NULL)
	rettv->vval.v_number = buf->b_fnum;
    else
	rettv->vval.v_number = -1;
}

    static void
buf_win_common(typval_T *argvars, typval_T *rettv, int get_nr)
{
    win_T	*wp;
    int		winnr = 0;
    buf_T	*buf;

    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = tv_get_buf(&argvars[0], TRUE);
    FOR_ALL_WINDOWS(wp)
    {
	++winnr;
	if (wp->w_buffer == buf)
	    break;
    }
    rettv->vval.v_number = (wp != NULL ? (get_nr ? winnr : wp->w_id) : -1);
    --emsg_off;
}

/*
 * "bufwinid(nr)" function
 */
    static void
f_bufwinid(typval_T *argvars, typval_T *rettv)
{
    buf_win_common(argvars, rettv, FALSE);
}

/*
 * "bufwinnr(nr)" function
 */
    static void
f_bufwinnr(typval_T *argvars, typval_T *rettv)
{
    buf_win_common(argvars, rettv, TRUE);
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

    boff = tv_get_number(&argvars[0]) - 1;  /* boff gets -1 on type error */
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
#ifdef FEAT_MBYTE
    char_u	*t;
#endif
    char_u	*str;
    varnumber_T	idx;

    str = tv_get_string_chk(&argvars[0]);
    idx = tv_get_number_chk(&argvars[1], NULL);
    rettv->vval.v_number = -1;
    if (str == NULL || idx < 0)
	return;

#ifdef FEAT_MBYTE
    t = str;
    for ( ; idx > 0; idx--)
    {
	if (*t == NUL)		/* EOL reached */
	    return;
	if (enc_utf8 && comp)
	    t += utf_ptr2len(t);
	else
	    t += (*mb_ptr2len)(t);
    }
    rettv->vval.v_number = (varnumber_T)(t - str);
#else
    if ((size_t)idx <= STRLEN(str))
	rettv->vval.v_number = idx;
#endif
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
	EMSG(_(e_listreq));
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

#ifdef FEAT_JOB_CHANNEL
/*
 * "ch_canread()" function
 */
    static void
f_ch_canread(typval_T *argvars, typval_T *rettv)
{
    channel_T *channel = get_channel_arg(&argvars[0], FALSE, FALSE, 0);

    rettv->vval.v_number = 0;
    if (channel != NULL)
	rettv->vval.v_number = channel_has_readahead(channel, PART_SOCK)
			    || channel_has_readahead(channel, PART_OUT)
			    || channel_has_readahead(channel, PART_ERR);
}

/*
 * "ch_close()" function
 */
    static void
f_ch_close(typval_T *argvars, typval_T *rettv UNUSED)
{
    channel_T *channel = get_channel_arg(&argvars[0], TRUE, FALSE, 0);

    if (channel != NULL)
    {
	channel_close(channel, FALSE);
	channel_clear(channel);
    }
}

/*
 * "ch_close()" function
 */
    static void
f_ch_close_in(typval_T *argvars, typval_T *rettv UNUSED)
{
    channel_T *channel = get_channel_arg(&argvars[0], TRUE, FALSE, 0);

    if (channel != NULL)
	channel_close_in(channel);
}

/*
 * "ch_getbufnr()" function
 */
    static void
f_ch_getbufnr(typval_T *argvars, typval_T *rettv)
{
    channel_T *channel = get_channel_arg(&argvars[0], FALSE, FALSE, 0);

    rettv->vval.v_number = -1;
    if (channel != NULL)
    {
	char_u	*what = tv_get_string(&argvars[1]);
	int	part;

	if (STRCMP(what, "err") == 0)
	    part = PART_ERR;
	else if (STRCMP(what, "out") == 0)
	    part = PART_OUT;
	else if (STRCMP(what, "in") == 0)
	    part = PART_IN;
	else
	    part = PART_SOCK;
	if (channel->ch_part[part].ch_bufref.br_buf != NULL)
	    rettv->vval.v_number =
			      channel->ch_part[part].ch_bufref.br_buf->b_fnum;
    }
}

/*
 * "ch_getjob()" function
 */
    static void
f_ch_getjob(typval_T *argvars, typval_T *rettv)
{
    channel_T *channel = get_channel_arg(&argvars[0], FALSE, FALSE, 0);

    if (channel != NULL)
    {
	rettv->v_type = VAR_JOB;
	rettv->vval.v_job = channel->ch_job;
	if (channel->ch_job != NULL)
	    ++channel->ch_job->jv_refcount;
    }
}

/*
 * "ch_info()" function
 */
    static void
f_ch_info(typval_T *argvars, typval_T *rettv UNUSED)
{
    channel_T *channel = get_channel_arg(&argvars[0], FALSE, FALSE, 0);

    if (channel != NULL && rettv_dict_alloc(rettv) != FAIL)
	channel_info(channel, rettv->vval.v_dict);
}

/*
 * "ch_log()" function
 */
    static void
f_ch_log(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u	*msg = tv_get_string(&argvars[0]);
    channel_T	*channel = NULL;

    if (argvars[1].v_type != VAR_UNKNOWN)
	channel = get_channel_arg(&argvars[1], FALSE, FALSE, 0);

    ch_log(channel, "%s", msg);
}

/*
 * "ch_logfile()" function
 */
    static void
f_ch_logfile(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u *fname;
    char_u *opt = (char_u *)"";
    char_u buf[NUMBUFLEN];

    /* Don't open a file in restricted mode. */
    if (check_restricted() || check_secure())
	return;
    fname = tv_get_string(&argvars[0]);
    if (argvars[1].v_type == VAR_STRING)
	opt = tv_get_string_buf(&argvars[1], buf);
    ch_logfile(fname, opt);
}

/*
 * "ch_open()" function
 */
    static void
f_ch_open(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_CHANNEL;
    if (check_restricted() || check_secure())
	return;
    rettv->vval.v_channel = channel_open_func(argvars);
}

/*
 * "ch_read()" function
 */
    static void
f_ch_read(typval_T *argvars, typval_T *rettv)
{
    common_channel_read(argvars, rettv, FALSE);
}

/*
 * "ch_readraw()" function
 */
    static void
f_ch_readraw(typval_T *argvars, typval_T *rettv)
{
    common_channel_read(argvars, rettv, TRUE);
}

/*
 * "ch_evalexpr()" function
 */
    static void
f_ch_evalexpr(typval_T *argvars, typval_T *rettv)
{
    ch_expr_common(argvars, rettv, TRUE);
}

/*
 * "ch_sendexpr()" function
 */
    static void
f_ch_sendexpr(typval_T *argvars, typval_T *rettv)
{
    ch_expr_common(argvars, rettv, FALSE);
}

/*
 * "ch_evalraw()" function
 */
    static void
f_ch_evalraw(typval_T *argvars, typval_T *rettv)
{
    ch_raw_common(argvars, rettv, TRUE);
}

/*
 * "ch_sendraw()" function
 */
    static void
f_ch_sendraw(typval_T *argvars, typval_T *rettv)
{
    ch_raw_common(argvars, rettv, FALSE);
}

/*
 * "ch_setoptions()" function
 */
    static void
f_ch_setoptions(typval_T *argvars, typval_T *rettv UNUSED)
{
    channel_T	*channel;
    jobopt_T	opt;

    channel = get_channel_arg(&argvars[0], FALSE, FALSE, 0);
    if (channel == NULL)
	return;
    clear_job_options(&opt);
    if (get_job_options(&argvars[1], &opt,
			    JO_CB_ALL + JO_TIMEOUT_ALL + JO_MODE_ALL, 0) == OK)
	channel_set_options(channel, &opt);
    free_job_options(&opt);
}

/*
 * "ch_status()" function
 */
    static void
f_ch_status(typval_T *argvars, typval_T *rettv)
{
    channel_T	*channel;
    jobopt_T	opt;
    int		part = -1;

    /* return an empty string by default */
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    channel = get_channel_arg(&argvars[0], FALSE, FALSE, 0);

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	clear_job_options(&opt);
	if (get_job_options(&argvars[1], &opt, JO_PART, 0) == OK
						     && (opt.jo_set & JO_PART))
	    part = opt.jo_part;
    }

    rettv->vval.v_string = vim_strsave((char_u *)channel_status(channel, part));
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
#ifdef FEAT_MBYTE
    if (has_mbyte)
    {
	int	utf8 = 0;

	if (argvars[1].v_type != VAR_UNKNOWN)
	    utf8 = (int)tv_get_number_chk(&argvars[1], NULL);

	if (utf8)
	    rettv->vval.v_number = (*utf_ptr2char)(tv_get_string(&argvars[0]));
	else
	    rettv->vval.v_number = (*mb_ptr2char)(tv_get_string(&argvars[0]));
    }
    else
#endif
    rettv->vval.v_number = tv_get_string(&argvars[0])[0];
}

/*
 * "cindent(lnum)" function
 */
    static void
f_cindent(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_CINDENT
    pos_T	pos;
    linenr_T	lnum;

    pos = curwin->w_cursor;
    lnum = tv_get_lnum(argvars);
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
    static void
f_clearmatches(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_SEARCH_EXTRA
    clear_matches(curwin);
#endif
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
    static void
f_complete(typval_T *argvars, typval_T *rettv UNUSED)
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

    startcol = (int)tv_get_number_chk(&argvars[0], NULL);
    if (startcol <= 0)
	return;

    set_completion(startcol - 1, argvars[1].vval.v_list);
}

/*
 * "complete_add()" function
 */
    static void
f_complete_add(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = ins_compl_add_tv(&argvars[0], 0);
}

/*
 * "complete_check()" function
 */
    static void
f_complete_check(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		saved = RedrawingDisabled;

    RedrawingDisabled = 0;
    ins_compl_check_keys(0, TRUE);
    rettv->vval.v_number = compl_interrupted;
    RedrawingDisabled = saved;
}
#endif

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
 * "count()" function
 */
    static void
f_count(typval_T *argvars, typval_T *rettv)
{
    long	n = 0;
    int		ic = FALSE;
    int		error = FALSE;

    if (argvars[2].v_type != VAR_UNKNOWN)
	ic = (int)tv_get_number_chk(&argvars[2], &error);

    if (argvars[0].v_type == VAR_STRING)
    {
	char_u *expr = tv_get_string_chk(&argvars[1]);
	char_u *p = argvars[0].vval.v_string;
	char_u *next;

	if (!error && expr != NULL && *expr != NUL && p != NULL)
	{
	    if (ic)
	    {
		size_t len = STRLEN(expr);

		while (*p != NUL)
		{
		    if (MB_STRNICMP(p, expr, len) == 0)
		    {
			++n;
			p += len;
		    }
		    else
			MB_PTR_ADV(p);
		}
	    }
	    else
		while ((next = (char_u *)strstr((char *)p, (char *)expr))
								       != NULL)
		{
		    ++n;
		    p = next + STRLEN(expr);
		}
	}

    }
    else if (argvars[0].v_type == VAR_LIST)
    {
	listitem_T	*li;
	list_T		*l;
	long		idx;

	if ((l = argvars[0].vval.v_list) != NULL)
	{
	    li = l->lv_first;
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		if (argvars[3].v_type != VAR_UNKNOWN)
		{
		    idx = (long)tv_get_number_chk(&argvars[3], &error);
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
		if (tv_equal(&li->li_tv, &argvars[1], ic, FALSE))
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
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		if (argvars[3].v_type != VAR_UNKNOWN)
		    EMSG(_(e_invarg));
	    }

	    todo = error ? 0 : (int)d->dv_hashtab.ht_used;
	    for (hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;
		    if (tv_equal(&HI2DI(hi)->di_tv, &argvars[1], ic, FALSE))
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
    static void
f_cscope_connection(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_CSCOPE
    int		num = 0;
    char_u	*dbpath = NULL;
    char_u	*prepend = NULL;
    char_u	buf[NUMBUFLEN];

    if (argvars[0].v_type != VAR_UNKNOWN
	    && argvars[1].v_type != VAR_UNKNOWN)
    {
	num = (int)tv_get_number(&argvars[0]);
	dbpath = tv_get_string(&argvars[1]);
	if (argvars[2].v_type != VAR_UNKNOWN)
	    prepend = tv_get_string_buf(&argvars[2], buf);
    }

    rettv->vval.v_number = cs_connection(num, dbpath, prepend);
#endif
}

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
#ifdef FEAT_VIRTUALEDIT
    long	coladd = 0;
#endif
    int		set_curswant = TRUE;

    rettv->vval.v_number = -1;
    if (argvars[1].v_type == VAR_UNKNOWN)
    {
	pos_T	    pos;
	colnr_T	    curswant = -1;

	if (list2fpos(argvars, &pos, NULL, &curswant) == FAIL)
	{
	    EMSG(_(e_invarg));
	    return;
	}
	line = pos.lnum;
	col = pos.col;
#ifdef FEAT_VIRTUALEDIT
	coladd = pos.coladd;
#endif
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
#ifdef FEAT_VIRTUALEDIT
	if (argvars[2].v_type != VAR_UNKNOWN)
	    coladd = (long)tv_get_number_chk(&argvars[2], NULL);
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

    curwin->w_set_curswant = set_curswant;
    rettv->vval.v_number = 0;
}

#ifdef WIN3264
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
	EMSG(_(e_invarg));
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
	EMSG(_(e_invarg));
    else
    {
	copyID = get_copyID();
	item_copy(&argvars[0], rettv, TRUE, noref == 0 ? copyID : 0);
    }
}

/*
 * "delete()" function
 */
    static void
f_delete(typval_T *argvars, typval_T *rettv)
{
    char_u	nbuf[NUMBUFLEN];
    char_u	*name;
    char_u	*flags;

    rettv->vval.v_number = -1;
    if (check_restricted() || check_secure())
	return;

    name = tv_get_string(&argvars[0]);
    if (name == NULL || *name == NUL)
    {
	EMSG(_(e_invarg));
	return;
    }

    if (argvars[1].v_type != VAR_UNKNOWN)
	flags = tv_get_string_buf(&argvars[1], nbuf);
    else
	flags = (char_u *)"";

    if (*flags == NUL)
	/* delete a file */
	rettv->vval.v_number = mch_remove(name) == 0 ? 0 : -1;
    else if (STRCMP(flags, "d") == 0)
	/* delete an empty directory */
	rettv->vval.v_number = mch_rmdir(name) == 0 ? 0 : -1;
    else if (STRCMP(flags, "rf") == 0)
	/* delete a directory recursively */
	rettv->vval.v_number = delete_recursive(name);
    else
	EMSG2(_(e_invexpr2), flags);
}

/*
 * "deletebufline()" function
 */
    static void
f_deletebufline(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf;
    linenr_T	first, last;
    linenr_T	lnum;
    long	count;
    int		is_curbuf;
    buf_T	*curbuf_save = NULL;
    win_T	*curwin_save = NULL;
    tabpage_T	*tp;
    win_T	*wp;

    buf = tv_get_buf(&argvars[0], FALSE);
    if (buf == NULL)
    {
	rettv->vval.v_number = 1; /* FAIL */
	return;
    }
    is_curbuf = buf == curbuf;

    first = tv_get_lnum_buf(&argvars[1], buf);
    if (argvars[2].v_type != VAR_UNKNOWN)
	last = tv_get_lnum_buf(&argvars[2], buf);
    else
	last = first;

    if (buf->b_ml.ml_mfp == NULL || first < 1
			   || first > buf->b_ml.ml_line_count || last < first)
    {
	rettv->vval.v_number = 1;	/* FAIL */
	return;
    }

    if (!is_curbuf)
    {
	curbuf_save = curbuf;
	curwin_save = curwin;
	curbuf = buf;
	find_win_for_curbuf();
    }
    if (last > curbuf->b_ml.ml_line_count)
	last = curbuf->b_ml.ml_line_count;
    count = last - first + 1;

    // When coming here from Insert mode, sync undo, so that this can be
    // undone separately from what was previously inserted.
    if (u_sync_once == 2)
    {
	u_sync_once = 1; // notify that u_sync() was called
	u_sync(TRUE);
    }

    if (u_save(first - 1, last + 1) == FAIL)
    {
	rettv->vval.v_number = 1;	/* FAIL */
	return;
    }

    for (lnum = first; lnum <= last; ++lnum)
	ml_delete(first, TRUE);

    FOR_ALL_TAB_WINDOWS(tp, wp)
	if (wp->w_buffer == buf)
	{
	    if (wp->w_cursor.lnum > last)
		wp->w_cursor.lnum -= count;
	    else if (wp->w_cursor.lnum> first)
		wp->w_cursor.lnum = first;
	    if (wp->w_cursor.lnum > wp->w_buffer->b_ml.ml_line_count)
		wp->w_cursor.lnum = wp->w_buffer->b_ml.ml_line_count;
	}
    check_cursor_col();
    deleted_lines_mark(first, count);

    if (!is_curbuf)
    {
	curbuf = curbuf_save;
	curwin = curwin_save;
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
 * "diff_filler()" function
 */
    static void
f_diff_filler(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_DIFF
    rettv->vval.v_number = diff_check_fill(curwin, tv_get_lnum(argvars));
#endif
}

/*
 * "diff_hlID()" function
 */
    static void
f_diff_hlID(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_DIFF
    linenr_T		lnum = tv_get_lnum(argvars);
    static linenr_T	prev_lnum = 0;
    static varnumber_T	changedtick = 0;
    static int		fnum = 0;
    static int		change_start = 0;
    static int		change_end = 0;
    static hlf_T	hlID = (hlf_T)0;
    int			filler_lines;
    int			col;

    if (lnum < 0)	/* ignore type error in {lnum} arg */
	lnum = 0;
    if (lnum != prev_lnum
	    || changedtick != CHANGEDTICK(curbuf)
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
	changedtick = CHANGEDTICK(curbuf);
	fnum = curbuf->b_fnum;
    }

    if (hlID == HLF_CHD || hlID == HLF_TXD)
    {
	col = tv_get_number(&argvars[1]) - 1; /* ignore type error in {col} */
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
	case VAR_SPECIAL:
	    n = argvars[0].vval.v_number != VVAL_TRUE;
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
	    internal_error("f_empty(UNKNOWN)");
	    n = TRUE;
	    break;
    }

    rettv->vval.v_number = n;
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
	    EMSG2(_(e_invexpr2), p);
	need_clr_eos = FALSE;
	rettv->v_type = VAR_NUMBER;
	rettv->vval.v_number = 0;
    }
    else if (*s != NUL)
	EMSG(_(e_trailing));
}

/*
 * "eventhandler()" function
 */
    static void
f_eventhandler(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = vgetc_busy;
}

/*
 * "executable()" function
 */
    static void
f_executable(typval_T *argvars, typval_T *rettv)
{
    char_u *name = tv_get_string(&argvars[0]);

    /* Check in $PATH and also check directly if there is a directory name. */
    rettv->vval.v_number = mch_can_exe(name, NULL, TRUE)
		 || (gettail(name) != name && mch_can_exe(name, NULL, FALSE));
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
	len = (int)STRLEN(value);	/* Append the entire string */
    else
	len = value_len;		/* Append only "value_len" characters */
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
    int	    indent UNUSED)
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
    static void
f_execute(typval_T *argvars, typval_T *rettv)
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

    if (argvars[0].v_type == VAR_LIST)
    {
	list = argvars[0].vval.v_list;
	if (list == NULL || list->lv_first == NULL)
	    /* empty list, no commands, empty output */
	    return;
	++list->lv_refcount;
    }
    else
    {
	cmd = tv_get_string_chk(&argvars[0]);
	if (cmd == NULL)
	    return;
    }

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	char_u	buf[NUMBUFLEN];
	char_u  *s = tv_get_string_buf_chk(&argvars[1], buf);

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

    /* Need to append a NUL to the result. */
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
 * "exepath()" function
 */
    static void
f_exepath(typval_T *argvars, typval_T *rettv)
{
    char_u *p = NULL;

    (void)mch_can_exe(tv_get_string(&argvars[0]), &p, TRUE);
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = p;
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
    else				/* internal variable */
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
    char_u	*errormsg;
    int		options = WILD_SILENT|WILD_USE_NL|WILD_LIST_NOTFOUND;
    expand_T	xpc;
    int		error = FALSE;
    char_u	*result;

    rettv->v_type = VAR_STRING;
    if (argvars[1].v_type != VAR_UNKNOWN
	    && argvars[2].v_type != VAR_UNKNOWN
	    && tv_get_number_chk(&argvars[2], &error)
	    && !error)
    {
	rettv_list_set(rettv, NULL);
    }

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
	    else
		vim_free(result);
	}
	else
	    rettv->vval.v_string = result;
    }
    else
    {
	/* When the optional second argument is non-zero, don't remove matches
	 * for 'wildignore' and don't put matches for 'suffixes' at the end. */
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
 * "extend(list, list [, idx])" function
 * "extend(dict, dict [, action])" function
 */
    static void
f_extend(typval_T *argvars, typval_T *rettv)
{
    char_u      *arg_errmsg = (char_u *)N_("extend() argument");

    if (argvars[0].v_type == VAR_LIST && argvars[1].v_type == VAR_LIST)
    {
	list_T		*l1, *l2;
	listitem_T	*item;
	long		before;
	int		error = FALSE;

	l1 = argvars[0].vval.v_list;
	l2 = argvars[1].vval.v_list;
	if (l1 != NULL && !tv_check_lock(l1->lv_lock, arg_errmsg, TRUE)
		&& l2 != NULL)
	{
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		before = (long)tv_get_number_chk(&argvars[2], &error);
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
	dict_T	*d1, *d2;
	char_u	*action;
	int	i;

	d1 = argvars[0].vval.v_dict;
	d2 = argvars[1].vval.v_dict;
	if (d1 != NULL && !tv_check_lock(d1->dv_lock, arg_errmsg, TRUE)
		&& d2 != NULL)
	{
	    /* Check the third argument. */
	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		static char *(av[]) = {"keep", "force", "error"};

		action = tv_get_string_chk(&argvars[2]);
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

	    dict_extend(d1, d2, action);

	    copy_tv(&argvars[0], rettv);
	}
    }
    else
	EMSG2(_(e_listdictarg), "extend()");
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
    char_u	*keys_esc;

    /* This is not allowed in the sandbox.  If the commands would still be
     * executed in the sandbox it would be OK, but it probably happens later,
     * when "sandbox" is no longer set. */
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
	    }
	}
    }

    if (*keys != NUL || execute)
    {
	/* Need to escape K_SPECIAL and CSI before putting the string in the
	 * typeahead buffer. */
	keys_esc = vim_strsave_escape_csi(keys);
	if (keys_esc != NULL)
	{
	    ins_typebuf(keys_esc, (remap ? REMAP_YES : REMAP_NONE),
				  insert ? 0 : typebuf.tb_len, !typed, FALSE);
	    vim_free(keys_esc);
	    if (vgetc_busy
#ifdef FEAT_TIMERS
		    || timer_busy
#endif
		    )
		typebuf_was_filled = TRUE;
	    if (execute)
	    {
		int save_msg_scroll = msg_scroll;

		/* Avoid a 1 second delay when the keys start Insert mode. */
		msg_scroll = FALSE;

		if (!dangerous)
		    ++ex_normal_busy;
		exec_normal(TRUE, FALSE, TRUE);
		if (!dangerous)
		    --ex_normal_busy;

		msg_scroll |= save_msg_scroll;
	    }
	}
    }
}

/*
 * "filereadable()" function
 */
    static void
f_filereadable(typval_T *argvars, typval_T *rettv)
{
    int		fd;
    char_u	*p;
    int		n;

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif
    p = tv_get_string(&argvars[0]);
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
f_filewritable(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = filewritable(tv_get_string(&argvars[0]));
}

    static void
findfilendir(
    typval_T	*argvars UNUSED,
    typval_T	*rettv,
    int		find_what UNUSED)
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
    fname = tv_get_string(&argvars[0]);

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	p = tv_get_string_buf_chk(&argvars[1], pathbuf);
	if (p == NULL)
	    error = TRUE;
	else
	{
	    if (*p != NUL)
		path = p;

	    if (argvars[2].v_type != VAR_UNKNOWN)
		count = (int)tv_get_number_chk(&argvars[2], &error);
	}
    }

    if (count < 0 && rettv_list_alloc(rettv) == FAIL)
	error = TRUE;

    if (*fname != NUL && !error)
    {
	do
	{
	    if (rettv->v_type == VAR_STRING || rettv->v_type == VAR_LIST)
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

/*
 * "filter()" function
 */
    static void
f_filter(typval_T *argvars, typval_T *rettv)
{
    filter_map(argvars, rettv, FALSE);
}

/*
 * "finddir({fname}[, {path}[, {count}]])" function
 */
    static void
f_finddir(typval_T *argvars, typval_T *rettv)
{
    findfilendir(argvars, rettv, FINDFILE_DIR);
}

/*
 * "findfile({fname}[, {path}[, {count}]])" function
 */
    static void
f_findfile(typval_T *argvars, typval_T *rettv)
{
    findfilendir(argvars, rettv, FINDFILE_FILE);
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
 * "fnamemodify({fname}, {mods})" function
 */
    static void
f_fnamemodify(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    char_u	*mods;
    int		usedlen = 0;
    int		len;
    char_u	*fbuf = NULL;
    char_u	buf[NUMBUFLEN];

    fname = tv_get_string_chk(&argvars[0]);
    mods = tv_get_string_buf_chk(&argvars[1], buf);
    if (fname == NULL || mods == NULL)
	fname = NULL;
    else
    {
	len = (int)STRLEN(fname);
	(void)modify_fname(mods, FALSE, &usedlen, &fname, &fbuf, &len);
    }

    rettv->v_type = VAR_STRING;
    if (fname == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = vim_strnsave(fname, len);
    vim_free(fbuf);
}

/*
 * "foldclosed()" function
 */
    static void
foldclosed_both(
    typval_T	*argvars UNUSED,
    typval_T	*rettv,
    int		end UNUSED)
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;
    linenr_T	first, last;

    lnum = tv_get_lnum(argvars);
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
f_foldclosed(typval_T *argvars, typval_T *rettv)
{
    foldclosed_both(argvars, rettv, FALSE);
}

/*
 * "foldclosedend()" function
 */
    static void
f_foldclosedend(typval_T *argvars, typval_T *rettv)
{
    foldclosed_both(argvars, rettv, TRUE);
}

/*
 * "foldlevel()" function
 */
    static void
f_foldlevel(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;

    lnum = tv_get_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	rettv->vval.v_number = foldLevel(lnum);
#endif
}

/*
 * "foldtext()" function
 */
    static void
f_foldtext(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_FOLDING
    linenr_T	foldstart;
    linenr_T	foldend;
    char_u	*dashes;
    linenr_T	lnum;
    char_u	*s;
    char_u	*r;
    int		len;
    char	*txt;
    long	count;
#endif

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_FOLDING
    foldstart = (linenr_T)get_vim_var_nr(VV_FOLDSTART);
    foldend = (linenr_T)get_vim_var_nr(VV_FOLDEND);
    dashes = get_vim_var_str(VV_FOLDDASHES);
    if (foldstart > 0 && foldend <= curbuf->b_ml.ml_line_count
	    && dashes != NULL)
    {
	/* Find first non-empty line in the fold. */
	for (lnum = foldstart; lnum < foldend; ++lnum)
	    if (!linewhite(lnum))
		break;

	/* Find interesting text in this line. */
	s = skipwhite(ml_get(lnum));
	/* skip C comment-start */
	if (s[0] == '/' && (s[1] == '*' || s[1] == '/'))
	{
	    s = skipwhite(s + 2);
	    if (*skipwhite(s) == NUL
			    && lnum + 1 < (linenr_T)get_vim_var_nr(VV_FOLDEND))
	    {
		s = skipwhite(ml_get(lnum + 1));
		if (*s == '*')
		    s = skipwhite(s + 1);
	    }
	}
	count = (long)(foldend - foldstart + 1);
	txt = NGETTEXT("+-%s%3ld line: ", "+-%s%3ld lines: ", count);
	r = alloc((unsigned)(STRLEN(txt)
		    + STRLEN(dashes)	    /* for %s */
		    + 20		    /* for %3ld */
		    + STRLEN(s)));	    /* concatenated */
	if (r != NULL)
	{
	    sprintf((char *)r, txt, dashes, count);
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
    static void
f_foldtextresult(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_FOLDING
    linenr_T	lnum;
    char_u	*text;
    char_u	buf[FOLD_TEXT_LEN];
    foldinfo_T  foldinfo;
    int		fold_count;
    static int	entered = FALSE;
#endif

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
#ifdef FEAT_FOLDING
    if (entered)
	return; /* reject recursive use */
    entered = TRUE;

    lnum = tv_get_lnum(argvars);
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

    entered = FALSE;
#endif
}

/*
 * "foreground()" function
 */
    static void
f_foreground(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_GUI
    if (gui.in_use)
	gui_mch_set_foreground();
#else
# ifdef WIN32
    win32_set_foreground();
# endif
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
	/* function(MyFunc, [arg], dict) */
	s = argvars[0].vval.v_string;
    }
    else if (argvars[0].v_type == VAR_PARTIAL
					 && argvars[0].vval.v_partial != NULL)
    {
	/* function(dict.MyFunc, [arg]) */
	arg_pt = argvars[0].vval.v_partial;
	s = partial_name(arg_pt);
    }
    else
    {
	/* function('MyFunc', [arg], dict) */
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
	EMSG2(_(e_invarg2), use_string ? tv_get_string(&argvars[0]) : s);
    /* Don't check an autoload name for existence here. */
    else if (trans_name != NULL && (is_funcref
				? find_func(trans_name) == NULL
				: !translated_function_exists(trans_name)))
	EMSG2(_("E700: Unknown function: %s"), s);
    else
    {
	int	dict_idx = 0;
	int	arg_idx = 0;
	list_T	*list = NULL;

	if (STRNCMP(s, "s:", 2) == 0 || STRNCMP(s, "<SID>", 5) == 0)
	{
	    char	sid_buf[25];
	    int		off = *s == 's' ? 2 : 5;

	    /* Expand s: and <SID> into <SNR>nr_, so that the function can
	     * also be called from another script. Using trans_function_name()
	     * would also work, but some plugins depend on the name being
	     * printable text. */
	    sprintf(sid_buf, "<SNR>%ld_", (long)current_sctx.sc_sid);
	    name = alloc((int)(STRLEN(sid_buf) + STRLEN(s + off) + 1));
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
		/* function(name, [args], dict) */
		arg_idx = 1;
		dict_idx = 2;
	    }
	    else if (argvars[1].v_type == VAR_DICT)
		/* function(name, dict) */
		dict_idx = 1;
	    else
		/* function(name, [args]) */
		arg_idx = 1;
	    if (dict_idx > 0)
	    {
		if (argvars[dict_idx].v_type != VAR_DICT)
		{
		    EMSG(_("E922: expected a dict"));
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
		    EMSG(_("E923: Second argument of function() must be a list or a dict"));
		    vim_free(name);
		    goto theend;
		}
		list = argvars[arg_idx].vval.v_list;
		if (list == NULL || list->lv_len == 0)
		    arg_idx = 0;
	    }
	}
	if (dict_idx > 0 || arg_idx > 0 || arg_pt != NULL || is_funcref)
	{
	    partial_T	*pt = (partial_T *)alloc_clear(sizeof(partial_T));

	    /* result is a VAR_PARTIAL */
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
		    pt->pt_argv = (typval_T *)alloc(
					      sizeof(typval_T) * pt->pt_argc);
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

		/* For "function(dict.func, [], dict)" and "func" is a partial
		 * use "dict".  That is backwards compatible. */
		if (dict_idx > 0)
		{
		    /* The dict is bound explicitly, pt_auto is FALSE. */
		    pt->pt_dict = argvars[dict_idx].vval.v_dict;
		    ++pt->pt_dict->dv_refcount;
		}
		else if (arg_pt != NULL)
		{
		    /* If the dict was bound automatically the result is also
		     * bound automatically. */
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
		    pt->pt_func = find_func(trans_name);
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
	    /* result is a VAR_FUNC */
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
    /* This is postponed until we are back at the toplevel, because we may be
     * using Lists and Dicts internally.  E.g.: ":echo [garbagecollect()]". */
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

    if (argvars[0].v_type == VAR_LIST)
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
		rettv_dict_set(rettv, pt->pt_dict);
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
		EMSG2(_(e_invarg2), what);
	    return;
	}
    }
    else
	EMSG2(_(e_listdictarg), "get()");

    if (tv == NULL)
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    copy_tv(&argvars[2], rettv);
    }
    else
	copy_tv(tv, rettv);
}

/*
 * Returns buffer options, variables and other attributes in a dictionary.
 */
    static dict_T *
get_buffer_info(buf_T *buf)
{
    dict_T	*dict;
    tabpage_T	*tp;
    win_T	*wp;
    list_T	*windows;

    dict = dict_alloc();
    if (dict == NULL)
	return NULL;

    dict_add_number(dict, "bufnr", buf->b_fnum);
    dict_add_string(dict, "name", buf->b_ffname);
    dict_add_number(dict, "lnum", buf == curbuf ? curwin->w_cursor.lnum
						     : buflist_findlnum(buf));
    dict_add_number(dict, "loaded", buf->b_ml.ml_mfp != NULL);
    dict_add_number(dict, "listed", buf->b_p_bl);
    dict_add_number(dict, "changed", bufIsChanged(buf));
    dict_add_number(dict, "changedtick", CHANGEDTICK(buf));
    dict_add_number(dict, "hidden",
			    buf->b_ml.ml_mfp != NULL && buf->b_nwindows == 0);

    /* Get a reference to buffer variables */
    dict_add_dict(dict, "variables", buf->b_vars);

    /* List of windows displaying this buffer */
    windows = list_alloc();
    if (windows != NULL)
    {
	FOR_ALL_TAB_WINDOWS(tp, wp)
	    if (wp->w_buffer == buf)
		list_append_number(windows, (varnumber_T)wp->w_id);
	dict_add_list(dict, "windows", windows);
    }

#ifdef FEAT_SIGNS
    if (buf->b_signlist != NULL)
    {
	/* List of signs placed in this buffer */
	list_T	*signs = list_alloc();
	if (signs != NULL)
	{
	    get_buffer_signs(buf, signs);
	    dict_add_list(dict, "signs", signs);
	}
    }
#endif

    return dict;
}

/*
 * "getbufinfo()" function
 */
    static void
f_getbufinfo(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = NULL;
    buf_T	*argbuf = NULL;
    dict_T	*d;
    int		filtered = FALSE;
    int		sel_buflisted = FALSE;
    int		sel_bufloaded = FALSE;
    int		sel_bufmodified = FALSE;

    if (rettv_list_alloc(rettv) != OK)
	return;

    /* List of all the buffers or selected buffers */
    if (argvars[0].v_type == VAR_DICT)
    {
	dict_T	*sel_d = argvars[0].vval.v_dict;

	if (sel_d != NULL)
	{
	    dictitem_T	*di;

	    filtered = TRUE;

	    di = dict_find(sel_d, (char_u *)"buflisted", -1);
	    if (di != NULL && tv_get_number(&di->di_tv))
		sel_buflisted = TRUE;

	    di = dict_find(sel_d, (char_u *)"bufloaded", -1);
	    if (di != NULL && tv_get_number(&di->di_tv))
		sel_bufloaded = TRUE;

	    di = dict_find(sel_d, (char_u *)"bufmodified", -1);
	    if (di != NULL && tv_get_number(&di->di_tv))
		sel_bufmodified = TRUE;
	}
    }
    else if (argvars[0].v_type != VAR_UNKNOWN)
    {
	/* Information about one buffer.  Argument specifies the buffer */
	(void)tv_get_number(&argvars[0]);   /* issue errmsg if type error */
	++emsg_off;
	argbuf = tv_get_buf(&argvars[0], FALSE);
	--emsg_off;
	if (argbuf == NULL)
	    return;
    }

    /* Return information about all the buffers or a specified buffer */
    FOR_ALL_BUFFERS(buf)
    {
	if (argbuf != NULL && argbuf != buf)
	    continue;
	if (filtered && ((sel_bufloaded && buf->b_ml.ml_mfp == NULL)
			|| (sel_buflisted && !buf->b_p_bl)
			|| (sel_bufmodified && !buf->b_changed)))
	    continue;

	d = get_buffer_info(buf);
	if (d != NULL)
	    list_append_dict(rettv->vval.v_list, d);
	if (argbuf != NULL)
	    return;
    }
}

/*
 * Get line or list of lines from buffer "buf" into "rettv".
 * Return a range (from start to end) of lines in rettv from the specified
 * buffer.
 * If 'retlist' is TRUE, then the lines are returned as a Vim List.
 */
    static void
get_buffer_lines(
    buf_T	*buf,
    linenr_T	start,
    linenr_T	end,
    int		retlist,
    typval_T	*rettv)
{
    char_u	*p;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    if (retlist && rettv_list_alloc(rettv) == FAIL)
	return;

    if (buf == NULL || buf->b_ml.ml_mfp == NULL || start < 0)
	return;

    if (!retlist)
    {
	if (start >= 1 && start <= buf->b_ml.ml_line_count)
	    p = ml_get_buf(buf, start, FALSE);
	else
	    p = (char_u *)"";
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
f_getbufline(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;
    linenr_T	end;
    buf_T	*buf;

    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = tv_get_buf(&argvars[0], FALSE);
    --emsg_off;

    lnum = tv_get_lnum_buf(&argvars[1], buf);
    if (argvars[2].v_type == VAR_UNKNOWN)
	end = lnum;
    else
	end = tv_get_lnum_buf(&argvars[2], buf);

    get_buffer_lines(buf, lnum, end, TRUE, rettv);
}

/*
 * "getbufvar()" function
 */
    static void
f_getbufvar(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf;
    buf_T	*save_curbuf;
    char_u	*varname;
    dictitem_T	*v;
    int		done = FALSE;

    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    varname = tv_get_string_chk(&argvars[1]);
    ++emsg_off;
    buf = tv_get_buf(&argvars[0], FALSE);

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (buf != NULL && varname != NULL)
    {
	/* set curbuf to be our buf, temporarily */
	save_curbuf = curbuf;
	curbuf = buf;

	if (*varname == '&')
	{
	    if (varname[1] == NUL)
	    {
		/* get all buffer-local options in a dict */
		dict_T	*opts = get_winbuf_options(TRUE);

		if (opts != NULL)
		{
		    rettv_dict_set(rettv, opts);
		    done = TRUE;
		}
	    }
	    else if (get_option_tv(&varname, rettv, TRUE) == OK)
		/* buffer-local-option */
		done = TRUE;
	}
	else
	{
	    /* Look up the variable. */
	    /* Let getbufvar({nr}, "") return the "b:" dictionary. */
	    v = find_var_in_ht(&curbuf->b_vars->dv_hashtab,
							 'b', varname, FALSE);
	    if (v != NULL)
	    {
		copy_tv(&v->di_tv, rettv);
		done = TRUE;
	    }
	}

	/* restore previous notion of curbuf */
	curbuf = save_curbuf;
    }

    if (!done && argvars[2].v_type != VAR_UNKNOWN)
	/* use the default value */
	copy_tv(&argvars[2], rettv);

    --emsg_off;
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
    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    ++emsg_off;
    buf = tv_get_buf(&argvars[0], FALSE);
    --emsg_off;
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
# ifdef FEAT_VIRTUALEDIT
	dict_add_number(d, "coladd", (long)buf->b_changelist[i].coladd);
# endif
    }
#endif
}
/*
 * "getchar()" function
 */
    static void
f_getchar(typval_T *argvars, typval_T *rettv)
{
    varnumber_T		n;
    int			error = FALSE;

#ifdef MESSAGE_QUEUE
    // vpeekc() used to check for messages, but that caused problems, invoking
    // a callback where it was not expected.  Some plugins use getchar(1) in a
    // loop to await a message, therefore make sure we check for messages here.
    parse_queued_messages();
#endif

    /* Position the cursor.  Needed after a message that ends in a space. */
    windgoto(msg_row, msg_col);

    ++no_mapping;
    ++allow_keys;
    for (;;)
    {
	if (argvars[0].v_type == VAR_UNKNOWN)
	    /* getchar(): blocking wait. */
	    n = plain_vgetc();
	else if (tv_get_number_chk(&argvars[0], &error) == 1)
	    /* getchar(1): only check if char avail */
	    n = vpeekc_any();
	else if (error || vpeekc_any() == NUL)
	    /* illegal argument or getchar(0) and no char avail: return zero */
	    n = 0;
	else
	    /* getchar(0) and char avail: return char */
	    n = plain_vgetc();

	if (n == K_IGNORE)
	    continue;
	break;
    }
    --no_mapping;
    --allow_keys;

    set_vim_var_nr(VV_MOUSE_WIN, 0);
    set_vim_var_nr(VV_MOUSE_WINID, 0);
    set_vim_var_nr(VV_MOUSE_LNUM, 0);
    set_vim_var_nr(VV_MOUSE_COL, 0);

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
	if (is_mouse_key(n))
	{
	    int		row = mouse_row;
	    int		col = mouse_col;
	    win_T	*win;
	    linenr_T	lnum;
	    win_T	*wp;
	    int		winnr = 1;

	    if (row >= 0 && col >= 0)
	    {
		/* Find the window at the mouse coordinates and compute the
		 * text position. */
		win = mouse_find_win(&row, &col);
		if (win == NULL)
		    return;
		(void)mouse_comp_pos(win, &row, &col, &lnum);
		for (wp = firstwin; wp != win; wp = wp->w_next)
		    ++winnr;
		set_vim_var_nr(VV_MOUSE_WIN, winnr);
		set_vim_var_nr(VV_MOUSE_WINID, win->w_id);
		set_vim_var_nr(VV_MOUSE_LNUM, lnum);
		set_vim_var_nr(VV_MOUSE_COL, col + 1);
	    }
	}
#endif
    }
}

/*
 * "getcharmod()" function
 */
    static void
f_getcharmod(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = mod_mask;
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
 * "getcmdline()" function
 */
    static void
f_getcmdline(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = get_cmdline_str();
}

/*
 * "getcmdpos()" function
 */
    static void
f_getcmdpos(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = get_cmdline_pos() + 1;
}

/*
 * "getcmdtype()" function
 */
    static void
f_getcmdtype(typval_T *argvars UNUSED, typval_T *rettv)
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

#if defined(FEAT_CMDL_COMPL)
/*
 * "getcompletion()" function
 */
    static void
f_getcompletion(typval_T *argvars, typval_T *rettv)
{
    char_u	*pat;
    expand_T	xpc;
    int		filtered = FALSE;
    int		options = WILD_SILENT | WILD_USE_NL | WILD_ADD_SLASH
					| WILD_NO_BEEP;

    if (argvars[2].v_type != VAR_UNKNOWN)
	filtered = tv_get_number_chk(&argvars[2], NULL);

    if (p_wic)
	options |= WILD_ICASE;

    /* For filtered results, 'wildignore' is used */
    if (!filtered)
	options |= WILD_KEEP_ALL;

    ExpandInit(&xpc);
    xpc.xp_pattern = tv_get_string(&argvars[0]);
    xpc.xp_pattern_len = (int)STRLEN(xpc.xp_pattern);
    xpc.xp_context = cmdcomplete_str_to_type(tv_get_string(&argvars[1]));
    if (xpc.xp_context == EXPAND_NOTHING)
    {
	if (argvars[1].v_type == VAR_STRING)
	    EMSG2(_(e_invarg2), argvars[1].vval.v_string);
	else
	    EMSG(_(e_invarg));
	return;
    }

# if defined(FEAT_MENU)
    if (xpc.xp_context == EXPAND_MENUS)
    {
	set_context_in_menu_cmd(&xpc, (char_u *)"menu", xpc.xp_pattern, FALSE);
	xpc.xp_pattern_len = (int)STRLEN(xpc.xp_pattern);
    }
# endif
#ifdef FEAT_CSCOPE
    if (xpc.xp_context == EXPAND_CSCOPE)
    {
	set_context_in_cscope_cmd(&xpc, xpc.xp_pattern, CMD_cscope);
	xpc.xp_pattern_len = (int)STRLEN(xpc.xp_pattern);
    }
#endif
#ifdef FEAT_SIGNS
    if (xpc.xp_context == EXPAND_SIGN)
    {
	set_context_in_sign_cmd(&xpc, xpc.xp_pattern);
	xpc.xp_pattern_len = (int)STRLEN(xpc.xp_pattern);
    }
#endif

    pat = addstar(xpc.xp_pattern, xpc.xp_pattern_len, xpc.xp_context);
    if ((rettv_list_alloc(rettv) != FAIL) && (pat != NULL))
    {
	int	i;

	ExpandOne(&xpc, pat, NULL, options, WILD_ALL_KEEP);

	for (i = 0; i < xpc.xp_numfiles; i++)
	    list_append_string(rettv->vval.v_list, xpc.xp_files[i], -1);
    }
    vim_free(pat);
    ExpandCleanup(&xpc);
}
#endif

/*
 * "getcwd()" function
 */
    static void
f_getcwd(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp = NULL;
    char_u	*cwd;
    int		global = FALSE;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (argvars[0].v_type == VAR_NUMBER && argvars[0].vval.v_number == -1)
	global = TRUE;
    else
	wp = find_tabwin(&argvars[0], &argvars[1]);

    if (wp != NULL && wp->w_localdir != NULL)
	rettv->vval.v_string = vim_strsave(wp->w_localdir);
    else if (wp != NULL || global)
    {
	if (globaldir != NULL)
	    rettv->vval.v_string = vim_strsave(globaldir);
	else
	{
	    cwd = alloc(MAXPATHL);
	    if (cwd != NULL)
	    {
		if (mch_dirname(cwd, MAXPATHL) != FAIL)
		    rettv->vval.v_string = vim_strsave(cwd);
		vim_free(cwd);
	    }
	}
    }
#ifdef BACKSLASH_IN_FILENAME
    if (rettv->vval.v_string != NULL)
	slash_adjust(rettv->vval.v_string);
#endif
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
	    /* Get the "Normal" font.  Either the name saved by
	     * hl_set_font_name() or from the font ID. */
	    font = gui.norm_font;
	    name = hl_get_font_name();
	}
	else
	{
	    name = tv_get_string(&argvars[0]);
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
f_getfperm(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;
    char_u	*perm = NULL;
    char_u	flags[] = "rwx";
    int		i;

    fname = tv_get_string(&argvars[0]);

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
f_getfsize(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;

    fname = tv_get_string(&argvars[0]);

    rettv->v_type = VAR_NUMBER;

    if (mch_stat((char *)fname, &st) >= 0)
    {
	if (mch_isdir(fname))
	    rettv->vval.v_number = 0;
	else
	{
	    rettv->vval.v_number = (varnumber_T)st.st_size;

	    /* non-perfect check for overflow */
	    if ((off_T)rettv->vval.v_number != (off_T)st.st_size)
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
f_getftime(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;

    fname = tv_get_string(&argvars[0]);

    if (mch_stat((char *)fname, &st) >= 0)
	rettv->vval.v_number = (varnumber_T)st.st_mtime;
    else
	rettv->vval.v_number = -1;
}

/*
 * "getftype({fname})" function
 */
    static void
f_getftype(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;
    char_u	*type = NULL;
    char	*t;

    fname = tv_get_string(&argvars[0]);

    rettv->v_type = VAR_STRING;
    if (mch_lstat((char *)fname, &st) >= 0)
    {
	if (S_ISREG(st.st_mode))
	    t = "file";
	else if (S_ISDIR(st.st_mode))
	    t = "dir";
	else if (S_ISLNK(st.st_mode))
	    t = "link";
	else if (S_ISBLK(st.st_mode))
	    t = "bdev";
	else if (S_ISCHR(st.st_mode))
	    t = "cdev";
	else if (S_ISFIFO(st.st_mode))
	    t = "fifo";
	else if (S_ISSOCK(st.st_mode))
	    t = "socket";
	else
	    t = "other";
	type = vim_strsave((char_u *)t);
    }
    rettv->vval.v_string = type;
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
    wp = find_tabwin(&argvars[0], &argvars[1]);
    if (wp == NULL)
	return;

    l = list_alloc();
    if (l == NULL)
	return;

    if (list_append_list(rettv->vval.v_list, l) == FAIL)
	return;
    list_append_number(rettv->vval.v_list, (varnumber_T)wp->w_jumplistidx);

    cleanup_jumplist(wp, TRUE);

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
# ifdef FEAT_VIRTUALEDIT
	dict_add_number(d, "coladd", (long)wp->w_jumplist[i].fmark.mark.coladd);
# endif
	dict_add_number(d, "bufnr", (long)wp->w_jumplist[i].fmark.fnum);
	if (wp->w_jumplist[i].fname != NULL)
	    dict_add_string(d, "filename", wp->w_jumplist[i].fname);
    }
#endif
}

/*
 * "getline(lnum, [end])" function
 */
    static void
f_getline(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;
    linenr_T	end;
    int		retlist;

    lnum = tv_get_lnum(argvars);
    if (argvars[1].v_type == VAR_UNKNOWN)
    {
	end = 0;
	retlist = FALSE;
    }
    else
    {
	end = tv_get_lnum(&argvars[1]);
	retlist = TRUE;
    }

    get_buffer_lines(curbuf, lnum, end, retlist, rettv);
}

#ifdef FEAT_QUICKFIX
    static void
get_qf_loc_list(int is_qf, win_T *wp, typval_T *what_arg, typval_T *rettv)
{
    if (what_arg->v_type == VAR_UNKNOWN)
    {
	if (rettv_list_alloc(rettv) == OK)
	    if (is_qf || wp != NULL)
		(void)get_errorlist(NULL, wp, -1, rettv->vval.v_list);
    }
    else
    {
	if (rettv_dict_alloc(rettv) == OK)
	    if (is_qf || (wp != NULL))
	    {
		if (what_arg->v_type == VAR_DICT)
		{
		    dict_T	*d = what_arg->vval.v_dict;

		    if (d != NULL)
			qf_get_properties(wp, d, rettv->vval.v_dict);
		}
		else
		    EMSG(_(e_dictreq));
	    }
    }
}
#endif

/*
 * "getloclist()" function
 */
    static void
f_getloclist(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_QUICKFIX
    win_T	*wp;

    wp = find_win_by_nr_or_id(&argvars[0]);
    get_qf_loc_list(FALSE, wp, &argvars[1], rettv);
#endif
}

/*
 * "getmatches()" function
 */
    static void
f_getmatches(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_SEARCH_EXTRA
    dict_T	*dict;
    matchitem_T	*cur = curwin->w_match_head;
    int		i;

    if (rettv_list_alloc(rettv) == OK)
    {
	while (cur != NULL)
	{
	    dict = dict_alloc();
	    if (dict == NULL)
		return;
	    if (cur->match.regprog == NULL)
	    {
		/* match added with matchaddpos() */
		for (i = 0; i < MAXPOSMATCH; ++i)
		{
		    llpos_T	*llpos;
		    char	buf[6];
		    list_T	*l;

		    llpos = &cur->pos.pos[i];
		    if (llpos->lnum == 0)
			break;
		    l = list_alloc();
		    if (l == NULL)
			break;
		    list_append_number(l, (varnumber_T)llpos->lnum);
		    if (llpos->col > 0)
		    {
			list_append_number(l, (varnumber_T)llpos->col);
			list_append_number(l, (varnumber_T)llpos->len);
		    }
		    sprintf(buf, "pos%d", i + 1);
		    dict_add_list(dict, buf, l);
		}
	    }
	    else
	    {
		dict_add_string(dict, "pattern", cur->pattern);
	    }
	    dict_add_string(dict, "group", syn_id2name(cur->hlg_id));
	    dict_add_number(dict, "priority", (long)cur->priority);
	    dict_add_number(dict, "id", (long)cur->id);
# if defined(FEAT_CONCEAL) && defined(FEAT_MBYTE)
	    if (cur->conceal_char)
	    {
		char_u buf[MB_MAXBYTES + 1];

		buf[(*mb_char2bytes)((int)cur->conceal_char, buf)] = NUL;
		dict_add_string(dict, "conceal", (char_u *)&buf);
	    }
# endif
	    list_append_dict(rettv->vval.v_list, dict);
	    cur = cur->next;
	}
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
	list_append_number(l,
#ifdef FEAT_VIRTUALEDIT
				(fp != NULL) ? (varnumber_T)fp->coladd :
#endif
							      (varnumber_T)0);
	if (getcurpos)
	{
	    update_curswant();
	    list_append_number(l, curwin->w_curswant == MAXCOL ?
		    (varnumber_T)MAXCOL : (varnumber_T)curwin->w_curswant + 1);
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
 * "getqflist()" function
 */
    static void
f_getqflist(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_QUICKFIX
    get_qf_loc_list(TRUE, NULL, &argvars[0], rettv);
#endif
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
	if (strregname == NULL)	    /* type error; errmsg already given */
	{
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = NULL;
	    return;
	}
    }
    else
	/* Default to v:register */
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
 * Returns information (variables, options, etc.) about a tab page
 * as a dictionary.
 */
    static dict_T *
get_tabpage_info(tabpage_T *tp, int tp_idx)
{
    win_T	*wp;
    dict_T	*dict;
    list_T	*l;

    dict = dict_alloc();
    if (dict == NULL)
	return NULL;

    dict_add_number(dict, "tabnr", tp_idx);

    l = list_alloc();
    if (l != NULL)
    {
	for (wp = (tp == curtab) ? firstwin : tp->tp_firstwin;
		wp; wp = wp->w_next)
	    list_append_number(l, (varnumber_T)wp->w_id);
	dict_add_list(dict, "windows", l);
    }

    /* Make a reference to tabpage variables */
    dict_add_dict(dict, "variables", tp->tp_vars);

    return dict;
}

/*
 * "gettabinfo()" function
 */
    static void
f_gettabinfo(typval_T *argvars, typval_T *rettv)
{
    tabpage_T	*tp, *tparg = NULL;
    dict_T	*d;
    int		tpnr = 0;

    if (rettv_list_alloc(rettv) != OK)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	/* Information about one tab page */
	tparg = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
	if (tparg == NULL)
	    return;
    }

    /* Get information about a specific tab page or all tab pages */
    FOR_ALL_TABPAGES(tp)
    {
	tpnr++;
	if (tparg != NULL && tp != tparg)
	    continue;
	d = get_tabpage_info(tp, tpnr);
	if (d != NULL)
	    list_append_dict(rettv->vval.v_list, d);
	if (tparg != NULL)
	    return;
    }
}

/*
 * "gettabvar()" function
 */
    static void
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
	/* Set tp to be our tabpage, temporarily.  Also set the window to the
	 * first window in the tabpage, otherwise the window is not valid. */
	if (switch_win(&oldcurwin, &oldtabpage,
		tp == curtab || tp->tp_firstwin == NULL ? firstwin
					    : tp->tp_firstwin, tp, TRUE) == OK)
	{
	    /* look up the variable */
	    /* Let gettabvar({nr}, "") return the "t:" dictionary. */
	    v = find_var_in_ht(&tp->tp_vars->dv_hashtab, 't', varname, FALSE);
	    if (v != NULL)
	    {
		copy_tv(&v->di_tv, rettv);
		done = TRUE;
	    }
	}

	/* restore previous notion of curwin */
	restore_win(oldcurwin, oldtabpage, TRUE);
    }

    if (!done && argvars[2].v_type != VAR_UNKNOWN)
	copy_tv(&argvars[2], rettv);
}

/*
 * "gettabwinvar()" function
 */
    static void
f_gettabwinvar(typval_T *argvars, typval_T *rettv)
{
    getwinvar(argvars, rettv, 1);
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

/*
 * Returns information about a window as a dictionary.
 */
    static dict_T *
get_win_info(win_T *wp, short tpnr, short winnr)
{
    dict_T	*dict;

    dict = dict_alloc();
    if (dict == NULL)
	return NULL;

    dict_add_number(dict, "tabnr", tpnr);
    dict_add_number(dict, "winnr", winnr);
    dict_add_number(dict, "winid", wp->w_id);
    dict_add_number(dict, "height", wp->w_height);
    dict_add_number(dict, "winrow", wp->w_winrow + 1);
#ifdef FEAT_MENU
    dict_add_number(dict, "winbar", wp->w_winbar_height);
#endif
    dict_add_number(dict, "width", wp->w_width);
    dict_add_number(dict, "wincol", wp->w_wincol + 1);
    dict_add_number(dict, "bufnr", wp->w_buffer->b_fnum);

#ifdef FEAT_TERMINAL
    dict_add_number(dict, "terminal", bt_terminal(wp->w_buffer));
#endif
#ifdef FEAT_QUICKFIX
    dict_add_number(dict, "quickfix", bt_quickfix(wp->w_buffer));
    dict_add_number(dict, "loclist",
		      (bt_quickfix(wp->w_buffer) && wp->w_llist_ref != NULL));
#endif

    /* Add a reference to window variables */
    dict_add_dict(dict, "variables", wp->w_vars);

    return dict;
}

/*
 * "getwininfo()" function
 */
    static void
f_getwininfo(typval_T *argvars, typval_T *rettv)
{
    tabpage_T	*tp;
    win_T	*wp = NULL, *wparg = NULL;
    dict_T	*d;
    short	tabnr = 0, winnr;

    if (rettv_list_alloc(rettv) != OK)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	wparg = win_id2wp(argvars);
	if (wparg == NULL)
	    return;
    }

    /* Collect information about either all the windows across all the tab
     * pages or one particular window.
     */
    FOR_ALL_TABPAGES(tp)
    {
	tabnr++;
	winnr = 0;
	FOR_ALL_WINDOWS_IN_TAB(tp, wp)
	{
	    winnr++;
	    if (wparg != NULL && wp != wparg)
		continue;
	    d = get_win_info(wp, tabnr, winnr);
	    if (d != NULL)
		list_append_dict(rettv->vval.v_list, d);
	    if (wparg != NULL)
		/* found information about a specific window */
		return;
	}
    }
}

/*
 * "win_findbuf()" function
 */
    static void
f_win_findbuf(typval_T *argvars, typval_T *rettv)
{
    if (rettv_list_alloc(rettv) != FAIL)
	win_findbuf(argvars, rettv->vval.v_list);
}

/*
 * "win_getid()" function
 */
    static void
f_win_getid(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = win_getid(argvars);
}

/*
 * "win_gotoid()" function
 */
    static void
f_win_gotoid(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = win_gotoid(argvars);
}

/*
 * "win_id2tabwin()" function
 */
    static void
f_win_id2tabwin(typval_T *argvars, typval_T *rettv)
{
    if (rettv_list_alloc(rettv) != FAIL)
	win_id2tabwin(argvars, rettv->vval.v_list);
}

/*
 * "win_id2win()" function
 */
    static void
f_win_id2win(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = win_id2win(argvars);
}

/*
 * "win_screenpos()" function
 */
    static void
f_win_screenpos(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    wp = find_win_by_nr_or_id(&argvars[0]);
    list_append_number(rettv->vval.v_list, wp == NULL ? 0 : wp->w_winrow + 1);
    list_append_number(rettv->vval.v_list, wp == NULL ? 0 : wp->w_wincol + 1);
}

/*
 * "getwinpos({timeout})" function
 */
    static void
f_getwinpos(typval_T *argvars UNUSED, typval_T *rettv)
{
    int x = -1;
    int y = -1;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
#ifdef FEAT_GUI
    if (gui.in_use)
	(void)gui_mch_get_winpos(&x, &y);
# if defined(HAVE_TGETENT) && defined(FEAT_TERMRESPONSE)
    else
# endif
#endif
#if defined(HAVE_TGETENT) && defined(FEAT_TERMRESPONSE)
    {
	varnumber_T timeout = 100;

	if (argvars[0].v_type != VAR_UNKNOWN)
	    timeout = tv_get_number(&argvars[0]);
	term_get_winpos(&x, &y, timeout);
    }
#endif
    list_append_number(rettv->vval.v_list, (varnumber_T)x);
    list_append_number(rettv->vval.v_list, (varnumber_T)y);
}


/*
 * "getwinposx()" function
 */
    static void
f_getwinposx(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = -1;
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    rettv->vval.v_number = x;
	return;
    }
#endif
#if defined(HAVE_TGETENT) && defined(FEAT_TERMRESPONSE)
    {
	int	    x, y;

	if (term_get_winpos(&x, &y, (varnumber_T)100) == OK)
	    rettv->vval.v_number = x;
    }
#endif
}

/*
 * "getwinposy()" function
 */
    static void
f_getwinposy(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = -1;
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    rettv->vval.v_number = y;
	return;
    }
#endif
#if defined(HAVE_TGETENT) && defined(FEAT_TERMRESPONSE)
    {
	int	    x, y;

	if (term_get_winpos(&x, &y, (varnumber_T)100) == OK)
	    rettv->vval.v_number = y;
    }
#endif
}

/*
 * "getwinvar()" function
 */
    static void
f_getwinvar(typval_T *argvars, typval_T *rettv)
{
    getwinvar(argvars, rettv, 0);
}

/*
 * "glob()" function
 */
    static void
f_glob(typval_T *argvars, typval_T *rettv)
{
    int		options = WILD_SILENT|WILD_USE_NL;
    expand_T	xpc;
    int		error = FALSE;

    /* When the optional second argument is non-zero, don't remove matches
     * for 'wildignore' and don't put matches for 'suffixes' at the end. */
    rettv->v_type = VAR_STRING;
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (tv_get_number_chk(&argvars[1], &error))
	    options |= WILD_KEEP_ALL;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    if (tv_get_number_chk(&argvars[2], &error))
	    {
		rettv_list_set(rettv, NULL);
	    }
	    if (argvars[3].v_type != VAR_UNKNOWN
				    && tv_get_number_chk(&argvars[3], &error))
		options |= WILD_ALLLINKS;
	}
    }
    if (!error)
    {
	ExpandInit(&xpc);
	xpc.xp_context = EXPAND_FILES;
	if (p_wic)
	    options += WILD_ICASE;
	if (rettv->v_type == VAR_STRING)
	    rettv->vval.v_string = ExpandOne(&xpc, tv_get_string(&argvars[0]),
						     NULL, options, WILD_ALL);
	else if (rettv_list_alloc(rettv) != FAIL)
	{
	  int i;

	  ExpandOne(&xpc, tv_get_string(&argvars[0]),
						NULL, options, WILD_ALL_KEEP);
	  for (i = 0; i < xpc.xp_numfiles; i++)
	      list_append_string(rettv->vval.v_list, xpc.xp_files[i], -1);

	  ExpandCleanup(&xpc);
	}
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * "globpath()" function
 */
    static void
f_globpath(typval_T *argvars, typval_T *rettv)
{
    int		flags = 0;
    char_u	buf1[NUMBUFLEN];
    char_u	*file = tv_get_string_buf_chk(&argvars[1], buf1);
    int		error = FALSE;
    garray_T	ga;
    int		i;

    /* When the optional second argument is non-zero, don't remove matches
    * for 'wildignore' and don't put matches for 'suffixes' at the end. */
    rettv->v_type = VAR_STRING;
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	if (tv_get_number_chk(&argvars[2], &error))
	    flags |= WILD_KEEP_ALL;
	if (argvars[3].v_type != VAR_UNKNOWN)
	{
	    if (tv_get_number_chk(&argvars[3], &error))
	    {
		rettv_list_set(rettv, NULL);
	    }
	    if (argvars[4].v_type != VAR_UNKNOWN
				    && tv_get_number_chk(&argvars[4], &error))
		flags |= WILD_ALLLINKS;
	}
    }
    if (file != NULL && !error)
    {
	ga_init2(&ga, (int)sizeof(char_u *), 10);
	globpath(tv_get_string(&argvars[0]), file, &ga, flags);
	if (rettv->v_type == VAR_STRING)
	    rettv->vval.v_string = ga_concat_strings(&ga, "\n");
	else if (rettv_list_alloc(rettv) != FAIL)
	    for (i = 0; i < ga.ga_len; ++i)
		list_append_string(rettv->vval.v_list,
					    ((char_u **)(ga.ga_data))[i], -1);
	ga_clear_strings(&ga);
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * "glob2regpat()" function
 */
    static void
f_glob2regpat(typval_T *argvars, typval_T *rettv)
{
    char_u	*pat = tv_get_string_chk(&argvars[0]);

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = (pat == NULL)
			 ? NULL : file_pat_to_reg_pat(pat, NULL, NULL, FALSE);
}

/* for VIM_VERSION_ defines */
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
#ifdef MACOS_X
	"mac",		/* Mac OS X (and, once, Mac OS Classic) */
	"osx",		/* Mac OS X */
# ifdef MACOS_X_DARWIN
	"macunix",	/* Mac OS X, with the darwin feature */
	"osxdarwin",	/* synonym for macunix */
# endif
#endif
#ifdef __QNX__
	"qnx",
#endif
#ifdef UNIX
	"unix",
#endif
#ifdef VMS
	"vms",
#endif
#ifdef WIN32
	"win32",
#endif
#if defined(UNIX) && (defined(__CYGWIN32__) || defined(__CYGWIN__))
	"win32unix",
#endif
#if defined(WIN64) || defined(_WIN64)
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
# ifndef FEAT_GUI_W32 /* other GUIs always have multiline balloons */
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
	|| defined(FEAT_GUI_W32) \
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
#ifdef FEAT_CMDL_COMPL
	"cmdline_compl",
#endif
#ifdef FEAT_CMDHIST
	"cmdline_hist",
#endif
#ifdef FEAT_COMMENTS
	"comments",
#endif
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
	"eval",	    /* always present, of course! */
	"ex_extra", /* graduated feature */
#ifdef FEAT_SEARCH_EXTRA
	"extra_search",
#endif
#ifdef FEAT_FKMAP
	"farsi",
#endif
#ifdef FEAT_SEARCHPATH
	"file_in_path",
#endif
#ifdef FEAT_FILTERPIPE
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
#ifdef FEAT_JOB_CHANNEL
	"job",
#endif
#ifdef FEAT_JUMPLIST
	"jumplist",
#endif
#ifdef FEAT_KEYMAP
	"keymap",
#endif
	"lambda", /* always with FEAT_EVAL, since 7.4.2120 with closure */
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
#ifdef FEAT_LOCALMAP
	"localmap",
#endif
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
# ifdef FEAT_MOUSE_SGR
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
#ifdef FEAT_TERMGUICOLORS
	"termguicolors",
#endif
#if defined(FEAT_TERMINAL) && !defined(WIN3264)
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
#ifdef FEAT_TEXT_PROP
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
#ifdef FEAT_USR_CMDS
	"user-commands",    /* was accidentally included in 5.4 */
	"user_commands",
#endif
#ifdef FEAT_VARTABS
	"vartabs",
#endif
#ifdef FEAT_VIMINFO
	"viminfo",
#endif
	"vertsplit",
#ifdef FEAT_VIRTUALEDIT
	"virtualedit",
#endif
	"visual",
#ifdef FEAT_VISUALEXTRA
	"visualextra",
#endif
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
	"xpm_w32",	/* for backward compatibility */
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

		/* Expect "patch-9.9.01234". */
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
	    n = gui.in_use;	/* gui_mch_browse() works when GUI is running */
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
#if defined(FEAT_TERMINAL) && defined(WIN3264)
	else if (STRICMP(name, "terminal") == 0)
	    n = terminal_enabled();
#endif
    }

    rettv->vval.v_number = n;
}

/*
 * "has_key()" function
 */
    static void
f_has_key(typval_T *argvars, typval_T *rettv)
{
    if (argvars[0].v_type != VAR_DICT)
    {
	EMSG(_(e_dictreq));
	return;
    }
    if (argvars[0].vval.v_dict == NULL)
	return;

    rettv->vval.v_number = dict_find(argvars[0].vval.v_dict,
				      tv_get_string(&argvars[1]), -1) != NULL;
}

/*
 * "haslocaldir()" function
 */
    static void
f_haslocaldir(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp = NULL;

    wp = find_tabwin(&argvars[0], &argvars[1]);
    rettv->vval.v_number = (wp != NULL && wp->w_localdir != NULL);
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
 * "histadd()" function
 */
    static void
f_histadd(typval_T *argvars UNUSED, typval_T *rettv)
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
    str = tv_get_string_chk(&argvars[0]);	/* NULL on type error */
    histype = str != NULL ? get_histtype(str) : -1;
    if (histype >= 0)
    {
	str = tv_get_string_buf(&argvars[1], buf);
	if (*str != NUL)
	{
	    init_history();
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
    static void
f_histdel(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_CMDHIST
    int		n;
    char_u	buf[NUMBUFLEN];
    char_u	*str;

    str = tv_get_string_chk(&argvars[0]);	/* NULL on type error */
    if (str == NULL)
	n = 0;
    else if (argvars[1].v_type == VAR_UNKNOWN)
	/* only one argument: clear entire history */
	n = clr_history(get_histtype(str));
    else if (argvars[1].v_type == VAR_NUMBER)
	/* index given: remove that entry */
	n = del_history_idx(get_histtype(str),
					  (int)tv_get_number(&argvars[1]));
    else
	/* string given: remove all matching entries */
	n = del_history_entry(get_histtype(str),
				      tv_get_string_buf(&argvars[1], buf));
    rettv->vval.v_number = n;
#endif
}

/*
 * "histget()" function
 */
    static void
f_histget(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_CMDHIST
    int		type;
    int		idx;
    char_u	*str;

    str = tv_get_string_chk(&argvars[0]);	/* NULL on type error */
    if (str == NULL)
	rettv->vval.v_string = NULL;
    else
    {
	type = get_histtype(str);
	if (argvars[1].v_type == VAR_UNKNOWN)
	    idx = get_history_idx(type);
	else
	    idx = (int)tv_get_number_chk(&argvars[1], NULL);
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
    static void
f_histnr(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		i;

#ifdef FEAT_CMDHIST
    char_u	*history = tv_get_string_chk(&argvars[0]);

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
#ifdef FEAT_MBYTE
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*from, *to, *str;
    vimconv_T	vimconv;
#endif

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

#ifdef FEAT_MBYTE
    str = tv_get_string(&argvars[0]);
    from = enc_canonize(enc_skip(tv_get_string_buf(&argvars[1], buf1)));
    to = enc_canonize(enc_skip(tv_get_string_buf(&argvars[2], buf2)));
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
f_indent(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;

    lnum = tv_get_lnum(argvars);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	rettv->vval.v_number = get_indent_lnum(lnum);
    else
	rettv->vval.v_number = -1;
}

/*
 * "index()" function
 */
    static void
f_index(typval_T *argvars, typval_T *rettv)
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
    /* Use a GUI dialog if the GUI is running and 'c' is not in 'guioptions' */
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
    /* While starting up, there is no place to enter text. When running tests
     * with --not-a-term we assume feedkeys() will be used. */
    if (no_console_input() && !is_not_a_term())
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
	msg_puts(tv_get_string(&li->li_tv));
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
    static void
f_inputrestore(typval_T *argvars UNUSED, typval_T *rettv)
{
    if (ga_userinput.ga_len > 0)
    {
	--ga_userinput.ga_len;
	restore_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	/* default return is zero == OK */
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
    static void
f_inputsave(typval_T *argvars UNUSED, typval_T *rettv)
{
    /* Add an entry to the stack of typeahead storage. */
    if (ga_grow(&ga_userinput, 1) == OK)
    {
	save_typeahead((tasave_T *)(ga_userinput.ga_data)
						       + ga_userinput.ga_len);
	++ga_userinput.ga_len;
	/* default return is zero == OK */
    }
    else
	rettv->vval.v_number = 1; /* Failed */
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
 * "insert()" function
 */
    static void
f_insert(typval_T *argvars, typval_T *rettv)
{
    long	before = 0;
    listitem_T	*item;
    list_T	*l;
    int		error = FALSE;

    if (argvars[0].v_type != VAR_LIST)
	EMSG2(_(e_listarg), "insert()");
    else if ((l = argvars[0].vval.v_list) != NULL
	    && !tv_check_lock(l->lv_lock, (char_u *)N_("insert() argument"), TRUE))
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    before = (long)tv_get_number_chk(&argvars[2], &error);
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
 * "invert(expr)" function
 */
    static void
f_invert(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = ~tv_get_number_chk(&argvars[0], NULL);
}

/*
 * "isdirectory()" function
 */
    static void
f_isdirectory(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = mch_isdir(tv_get_string(&argvars[0]));
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
	    EMSG(_(e_trailing));
	else
	{
	    if (lv.ll_tv == NULL)
	    {
		di = find_var(lv.ll_name, NULL, TRUE);
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

#if defined(FEAT_FLOAT) && defined(HAVE_MATH_H)
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
 * "items(dict)" function
 */
    static void
f_items(typval_T *argvars, typval_T *rettv)
{
    dict_list(argvars, rettv, 2);
}

#if defined(FEAT_JOB_CHANNEL) || defined(PROTO)
/*
 * Get the job from the argument.
 * Returns NULL if the job is invalid.
 */
    static job_T *
get_job_arg(typval_T *tv)
{
    job_T *job;

    if (tv->v_type != VAR_JOB)
    {
	EMSG2(_(e_invarg2), tv_get_string(tv));
	return NULL;
    }
    job = tv->vval.v_job;

    if (job == NULL)
	EMSG(_("E916: not a valid job"));
    return job;
}

/*
 * "job_getchannel()" function
 */
    static void
f_job_getchannel(typval_T *argvars, typval_T *rettv)
{
    job_T	*job = get_job_arg(&argvars[0]);

    if (job != NULL)
    {
	rettv->v_type = VAR_CHANNEL;
	rettv->vval.v_channel = job->jv_channel;
	if (job->jv_channel != NULL)
	    ++job->jv_channel->ch_refcount;
    }
}

/*
 * "job_info()" function
 */
    static void
f_job_info(typval_T *argvars, typval_T *rettv)
{
    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	job_T	*job = get_job_arg(&argvars[0]);

	if (job != NULL && rettv_dict_alloc(rettv) != FAIL)
	    job_info(job, rettv->vval.v_dict);
    }
    else if (rettv_list_alloc(rettv) == OK)
	job_info_all(rettv->vval.v_list);
}

/*
 * "job_setoptions()" function
 */
    static void
f_job_setoptions(typval_T *argvars, typval_T *rettv UNUSED)
{
    job_T	*job = get_job_arg(&argvars[0]);
    jobopt_T	opt;

    if (job == NULL)
	return;
    clear_job_options(&opt);
    if (get_job_options(&argvars[1], &opt, JO_STOPONEXIT + JO_EXIT_CB, 0) == OK)
	job_set_options(job, &opt);
    free_job_options(&opt);
}

/*
 * "job_start()" function
 */
    static void
f_job_start(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_JOB;
    if (check_restricted() || check_secure())
	return;
    rettv->vval.v_job = job_start(argvars, NULL, NULL, FALSE);
}

/*
 * "job_status()" function
 */
    static void
f_job_status(typval_T *argvars, typval_T *rettv)
{
    job_T	*job = get_job_arg(&argvars[0]);

    if (job != NULL)
    {
	rettv->v_type = VAR_STRING;
	rettv->vval.v_string = vim_strsave((char_u *)job_status(job));
    }
}

/*
 * "job_stop()" function
 */
    static void
f_job_stop(typval_T *argvars, typval_T *rettv)
{
    job_T	*job = get_job_arg(&argvars[0]);

    if (job != NULL)
	rettv->vval.v_number = job_stop(job, argvars, NULL);
}
#endif

/*
 * "join()" function
 */
    static void
f_join(typval_T *argvars, typval_T *rettv)
{
    garray_T	ga;
    char_u	*sep;

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
	sep = tv_get_string_chk(&argvars[1]);

    rettv->v_type = VAR_STRING;

    if (sep != NULL)
    {
	ga_init2(&ga, (int)sizeof(char), 80);
	list_join(&ga, argvars[0].vval.v_list, sep, TRUE, FALSE, 0);
	ga_append(&ga, NUL);
	rettv->vval.v_string = (char_u *)ga.ga_data;
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * "js_decode()" function
 */
    static void
f_js_decode(typval_T *argvars, typval_T *rettv)
{
    js_read_T	reader;

    reader.js_buf = tv_get_string(&argvars[0]);
    reader.js_fill = NULL;
    reader.js_used = 0;
    if (json_decode_all(&reader, rettv, JSON_JS) != OK)
	EMSG(_(e_invarg));
}

/*
 * "js_encode()" function
 */
    static void
f_js_encode(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = json_encode(&argvars[0], JSON_JS);
}

/*
 * "json_decode()" function
 */
    static void
f_json_decode(typval_T *argvars, typval_T *rettv)
{
    js_read_T	reader;

    reader.js_buf = tv_get_string(&argvars[0]);
    reader.js_fill = NULL;
    reader.js_used = 0;
    json_decode_all(&reader, rettv, 0);
}

/*
 * "json_encode()" function
 */
    static void
f_json_encode(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = json_encode(&argvars[0], 0);
}

/*
 * "keys()" function
 */
    static void
f_keys(typval_T *argvars, typval_T *rettv)
{
    dict_list(argvars, rettv, 0);
}

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
	case VAR_LIST:
	    rettv->vval.v_number = list_len(argvars[0].vval.v_list);
	    break;
	case VAR_DICT:
	    rettv->vval.v_number = dict_len(argvars[0].vval.v_dict);
	    break;
	case VAR_UNKNOWN:
	case VAR_SPECIAL:
	case VAR_FLOAT:
	case VAR_FUNC:
	case VAR_PARTIAL:
	case VAR_JOB:
	case VAR_CHANNEL:
	    EMSG(_("E701: Invalid type for len()"));
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
    /* The first two args must be strings, otherwise it's meaningless */
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
 * "line(string)" function
 */
    static void
f_line(typval_T *argvars, typval_T *rettv)
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
 * "lispindent(lnum)" function
 */
    static void
f_lispindent(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_LISP
    pos_T	pos;
    linenr_T	lnum;

    pos = curwin->w_cursor;
    lnum = tv_get_lnum(argvars);
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
    static void
f_localtime(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->vval.v_number = (varnumber_T)time(NULL);
}

    static void
get_maparg(typval_T *argvars, typval_T *rettv, int exact)
{
    char_u	*keys;
    char_u	*which;
    char_u	buf[NUMBUFLEN];
    char_u	*keys_buf = NULL;
    char_u	*rhs;
    int		mode;
    int		abbr = FALSE;
    int		get_dict = FALSE;
    mapblock_T	*mp;
    int		buffer_local;

    /* return empty string for failure */
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    keys = tv_get_string(&argvars[0]);
    if (*keys == NUL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	which = tv_get_string_buf_chk(&argvars[1], buf);
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    abbr = (int)tv_get_number(&argvars[2]);
	    if (argvars[3].v_type != VAR_UNKNOWN)
		get_dict = (int)tv_get_number(&argvars[3]);
	}
    }
    else
	which = (char_u *)"";
    if (which == NULL)
	return;

    mode = get_map_mode(&which, 0);

    keys = replace_termcodes(keys, &keys_buf, TRUE, TRUE, FALSE);
    rhs = check_map(keys, mode, exact, FALSE, abbr, &mp, &buffer_local);
    vim_free(keys_buf);

    if (!get_dict)
    {
	/* Return a string. */
	if (rhs != NULL)
	{
	    if (*rhs == NUL)
		rettv->vval.v_string = vim_strsave((char_u *)"<Nop>");
	    else
		rettv->vval.v_string = str2special_save(rhs, FALSE);
	}

    }
    else if (rettv_dict_alloc(rettv) != FAIL && rhs != NULL)
    {
	/* Return a dictionary. */
	char_u	    *lhs = str2special_save(mp->m_keys, TRUE);
	char_u	    *mapmode = map_mode_to_chars(mp->m_mode);
	dict_T	    *dict = rettv->vval.v_dict;

	dict_add_string(dict, "lhs", lhs);
	dict_add_string(dict, "rhs", mp->m_orig_str);
	dict_add_number(dict, "noremap", mp->m_noremap ? 1L : 0L);
	dict_add_number(dict, "expr", mp->m_expr ? 1L : 0L);
	dict_add_number(dict, "silent", mp->m_silent ? 1L : 0L);
	dict_add_number(dict, "sid", (long)mp->m_script_ctx.sc_sid);
	dict_add_number(dict, "lnum", (long)mp->m_script_ctx.sc_lnum);
	dict_add_number(dict, "buffer", (long)buffer_local);
	dict_add_number(dict, "nowait", mp->m_nowait ? 1L : 0L);
	dict_add_string(dict, "mode", mapmode);

	vim_free(lhs);
	vim_free(mapmode);
    }
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

    str = tv_get_string_buf(&argvars[0], buf);
    do_luaeval(str, argvars + 1, rettv);
}
#endif

/*
 * "map()" function
 */
    static void
f_map(typval_T *argvars, typval_T *rettv)
{
    filter_map(argvars, rettv, TRUE);
}

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
    MATCH_END,	    /* matchend() */
    MATCH_MATCH,    /* match() */
    MATCH_STR,	    /* matchstr() */
    MATCH_LIST,	    /* matchlist() */
    MATCH_POS	    /* matchstrpos() */
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

    /* Make 'cpoptions' empty, the 'l' flag should not be used here. */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    rettv->vval.v_number = -1;
    if (type == MATCH_LIST || type == MATCH_POS)
    {
	/* type MATCH_LIST: return empty list when there are no matches.
	 * type MATCH_POS: return ["", -1, -1, -1] */
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
	    idx = l->lv_idx;	/* use the cached index */
	}
	else
	{
	    if (start < 0)
		start = 0;
	    if (start > len)
		goto theend;
	    /* When "count" argument is there ignore matches before "start",
	     * otherwise skip part of the string.  Differs when pattern is "^"
	     * or "\<". */
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
		startcol = (colnr_T)(regmatch.startp[0] + 1 - str);
#endif
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
	    else if (type == MATCH_STR)
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
	/* matchstrpos() without a list: drop the second item. */
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

#ifdef FEAT_SEARCH_EXTRA
    static int
matchadd_dict_arg(typval_T *tv, char_u **conceal_char, win_T **win)
{
    dictitem_T *di;

    if (tv->v_type != VAR_DICT)
    {
	EMSG(_(e_dictreq));
	return FAIL;
    }

    if (dict_find(tv->vval.v_dict, (char_u *)"conceal", -1) != NULL)
	*conceal_char = dict_get_string(tv->vval.v_dict,
						   (char_u *)"conceal", FALSE);

    if ((di = dict_find(tv->vval.v_dict, (char_u *)"window", -1)) != NULL)
    {
	*win = find_win_by_nr_or_id(&di->di_tv);
	if (*win == NULL)
	{
	    EMSG(_("E957: Invalid window number"));
	    return FAIL;
	}
    }

    return OK;
}
#endif

/*
 * "matchadd()" function
 */
    static void
f_matchadd(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_SEARCH_EXTRA
    char_u	buf[NUMBUFLEN];
    char_u	*grp = tv_get_string_buf_chk(&argvars[0], buf);	/* group */
    char_u	*pat = tv_get_string_buf_chk(&argvars[1], buf);	/* pattern */
    int		prio = 10;	/* default priority */
    int		id = -1;
    int		error = FALSE;
    char_u	*conceal_char = NULL;
    win_T	*win = curwin;

    rettv->vval.v_number = -1;

    if (grp == NULL || pat == NULL)
	return;
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	prio = (int)tv_get_number_chk(&argvars[2], &error);
	if (argvars[3].v_type != VAR_UNKNOWN)
	{
	    id = (int)tv_get_number_chk(&argvars[3], &error);
	    if (argvars[4].v_type != VAR_UNKNOWN
		&& matchadd_dict_arg(&argvars[4], &conceal_char, &win) == FAIL)
		return;
	}
    }
    if (error == TRUE)
	return;
    if (id >= 1 && id <= 3)
    {
	EMSGN(_("E798: ID is reserved for \":match\": %ld"), id);
	return;
    }

    rettv->vval.v_number = match_add(win, grp, pat, prio, id, NULL,
								conceal_char);
#endif
}

/*
 * "matchaddpos()" function
 */
    static void
f_matchaddpos(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_SEARCH_EXTRA
    char_u	buf[NUMBUFLEN];
    char_u	*group;
    int		prio = 10;
    int		id = -1;
    int		error = FALSE;
    list_T	*l;
    char_u	*conceal_char = NULL;
    win_T	*win = curwin;

    rettv->vval.v_number = -1;

    group = tv_get_string_buf_chk(&argvars[0], buf);
    if (group == NULL)
	return;

    if (argvars[1].v_type != VAR_LIST)
    {
	EMSG2(_(e_listarg), "matchaddpos()");
	return;
    }
    l = argvars[1].vval.v_list;
    if (l == NULL)
	return;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	prio = (int)tv_get_number_chk(&argvars[2], &error);
	if (argvars[3].v_type != VAR_UNKNOWN)
	{
	    id = (int)tv_get_number_chk(&argvars[3], &error);

	    if (argvars[4].v_type != VAR_UNKNOWN
		&& matchadd_dict_arg(&argvars[4], &conceal_char, &win) == FAIL)
		return;
	}
    }
    if (error == TRUE)
	return;

    /* id == 3 is ok because matchaddpos() is supposed to substitute :3match */
    if (id == 1 || id == 2)
    {
	EMSGN(_("E798: ID is reserved for \":match\": %ld"), id);
	return;
    }

    rettv->vval.v_number = match_add(win, group, NULL, prio, id, l,
								conceal_char);
#endif
}

/*
 * "matcharg()" function
 */
    static void
f_matcharg(typval_T *argvars UNUSED, typval_T *rettv)
{
    if (rettv_list_alloc(rettv) == OK)
    {
#ifdef FEAT_SEARCH_EXTRA
	int	    id = (int)tv_get_number(&argvars[0]);
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
		list_append_string(rettv->vval.v_list, NULL, -1);
		list_append_string(rettv->vval.v_list, NULL, -1);
	    }
	}
#endif
    }
}

/*
 * "matchdelete()" function
 */
    static void
f_matchdelete(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_SEARCH_EXTRA
    rettv->vval.v_number = match_delete(curwin,
				       (int)tv_get_number(&argvars[0]), TRUE);
#endif
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
	EMSG2(_(e_listdictarg), domax ? "max()" : "min()");
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

/*
 * Create the directory in which "dir" is located, and higher levels when
 * needed.
 * Return OK or FAIL.
 */
    static int
mkdir_recurse(char_u *dir, int prot)
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
f_mkdir(typval_T *argvars, typval_T *rettv)
{
    char_u	*dir;
    char_u	buf[NUMBUFLEN];
    int		prot = 0755;

    rettv->vval.v_number = FAIL;
    if (check_restricted() || check_secure())
	return;

    dir = tv_get_string_buf(&argvars[0], buf);
    if (*dir == NUL)
	return;

    if (*gettail(dir) == NUL)
	/* remove trailing slashes */
	*gettail_sep(dir) = NUL;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    prot = (int)tv_get_number_chk(&argvars[2], NULL);
	    if (prot == -1)
		return;
	}
	if (STRCMP(tv_get_string(&argvars[1]), "p") == 0)
	{
	    if (mch_isdir(dir))
	    {
		/* With the "p" flag it's OK if the dir already exists. */
		rettv->vval.v_number = OK;
		return;
	    }
	    mkdir_recurse(dir, prot);
	}
    }
    rettv->vval.v_number = vim_mkdir_emsg(dir, prot);
}
#endif

/*
 * "mode()" function
 */
    static void
f_mode(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[4];

    vim_memset(buf, 0, sizeof(buf));

    if (time_for_testing == 93784)
    {
	/* Testing the two-character code. */
	buf[0] = 'x';
	buf[1] = '!';
    }
#ifdef FEAT_TERMINAL
    else if (term_use_loop())
	buf[0] = 't';
#endif
    else if (VIsual_active)
    {
	if (VIsual_select)
	    buf[0] = VIsual_mode + 's' - 'v';
	else
	    buf[0] = VIsual_mode;
    }
    else if (State == HITRETURN || State == ASKMORE || State == SETWSIZE
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
	if (State & VREPLACE_FLAG)
	{
	    buf[0] = 'R';
	    buf[1] = 'v';
	}
	else
	{
	    if (State & REPLACE_FLAG)
		buf[0] = 'R';
	    else
		buf[0] = 'i';
#ifdef FEAT_INS_EXPAND
	    if (ins_compl_active())
		buf[1] = 'c';
	    else if (ctrl_x_mode_not_defined_yet())
		buf[1] = 'x';
#endif
	}
    }
    else if ((State & CMDLINE) || exmode_active)
    {
	buf[0] = 'c';
	if (exmode_active == EXMODE_VIM)
	    buf[1] = 'v';
	else if (exmode_active == EXMODE_NORMAL)
	    buf[1] = 'e';
    }
    else
    {
	buf[0] = 'n';
	if (finish_op)
	{
	    buf[1] = 'o';
	    // to be able to detect force-linewise/blockwise/characterwise operations
	    buf[2] = motion_force;
	}
	else if (restart_edit == 'I' || restart_edit == 'R'
							|| restart_edit == 'V')
	{
	    buf[1] = 'i';
	    buf[2] = restart_edit;
	}
    }

    /* Clear out the minor mode when the argument is not a non-zero number or
     * non-empty string.  */
    if (!non_zero_arg(&argvars[0]))
	buf[1] = NUL;

    rettv->vval.v_string = vim_strsave(buf);
    rettv->v_type = VAR_STRING;
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

#ifdef FEAT_MBYTE
    if (has_mbyte)
    {
	int	utf8 = 0;

	if (argvars[1].v_type != VAR_UNKNOWN)
	    utf8 = (int)tv_get_number_chk(&argvars[1], NULL);
	if (utf8)
	    buf[(*utf_char2bytes)((int)tv_get_number(&argvars[0]), buf)] = NUL;
	else
	    buf[(*mb_char2bytes)((int)tv_get_number(&argvars[0]), buf)] = NUL;
    }
    else
#endif
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

/*
 * "pathshorten()" function
 */
    static void
f_pathshorten(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;

    rettv->v_type = VAR_STRING;
    p = tv_get_string_chk(&argvars[0]);
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

/* This dummy va_list is here because:
 * - passing a NULL pointer doesn't work when va_list isn't a pointer
 * - locally in the function results in a "used before set" warning
 * - using va_start() to initialize it gives "function with fixed args" error */
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

    /* Get the required length, allocate the buffer and do it for real. */
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

#ifdef FEAT_JOB_CHANNEL
/*
 * "prompt_setcallback({buffer}, {callback})" function
 */
    static void
f_prompt_setcallback(typval_T *argvars, typval_T *rettv UNUSED)
{
    buf_T	*buf;
    char_u	*callback;
    partial_T	*partial;

    if (check_secure())
	return;
    buf = tv_get_buf(&argvars[0], FALSE);
    if (buf == NULL)
	return;

    callback = get_callback(&argvars[1], &partial);
    if (callback == NULL)
	return;

    free_callback(buf->b_prompt_callback, buf->b_prompt_partial);
    if (partial == NULL)
	buf->b_prompt_callback = vim_strsave(callback);
    else
	/* pointer into the partial */
	buf->b_prompt_callback = callback;
    buf->b_prompt_partial = partial;
}

/*
 * "prompt_setinterrupt({buffer}, {callback})" function
 */
    static void
f_prompt_setinterrupt(typval_T *argvars, typval_T *rettv UNUSED)
{
    buf_T	*buf;
    char_u	*callback;
    partial_T	*partial;

    if (check_secure())
	return;
    buf = tv_get_buf(&argvars[0], FALSE);
    if (buf == NULL)
	return;

    callback = get_callback(&argvars[1], &partial);
    if (callback == NULL)
	return;

    free_callback(buf->b_prompt_interrupt, buf->b_prompt_int_partial);
    if (partial == NULL)
	buf->b_prompt_interrupt = vim_strsave(callback);
    else
	/* pointer into the partial */
	buf->b_prompt_interrupt = callback;
    buf->b_prompt_int_partial = partial;
}

/*
 * "prompt_setprompt({buffer}, {text})" function
 */
    static void
f_prompt_setprompt(typval_T *argvars, typval_T *rettv UNUSED)
{
    buf_T	*buf;
    char_u	*text;

    if (check_secure())
	return;
    buf = tv_get_buf(&argvars[0], FALSE);
    if (buf == NULL)
	return;

    text = tv_get_string(&argvars[1]);
    vim_free(buf->b_prompt_text);
    buf->b_prompt_text = vim_strsave(text);
}
#endif

/*
 * "pumvisible()" function
 */
    static void
f_pumvisible(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_INS_EXPAND
    if (pum_visible())
	rettv->vval.v_number = 1;
#endif
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
 * "range()" function
 */
    static void
f_range(typval_T *argvars, typval_T *rettv)
{
    varnumber_T	start;
    varnumber_T	end;
    varnumber_T	stride = 1;
    varnumber_T	i;
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
f_readfile(typval_T *argvars, typval_T *rettv)
{
    int		binary = FALSE;
    int		failed = FALSE;
    char_u	*fname;
    FILE	*fd;
    char_u	buf[(IOSIZE/256)*256];	/* rounded to avoid odd + 1 */
    int		io_size = sizeof(buf);
    int		readlen;		/* size of last fread() */
    char_u	*prev	 = NULL;	/* previously read bytes, if any */
    long	prevlen  = 0;		/* length of data in prev */
    long	prevsize = 0;		/* size of prev buffer */
    long	maxline  = MAXLNUM;
    long	cnt	 = 0;
    char_u	*p;			/* position in buf */
    char_u	*start;			/* start of current line */

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (STRCMP(tv_get_string(&argvars[1]), "b") == 0)
	    binary = TRUE;
	if (argvars[2].v_type != VAR_UNKNOWN)
	    maxline = (long)tv_get_number(&argvars[2]);
    }

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    /* Always open the file in binary mode, library functions have a mind of
     * their own about CR-LF conversion. */
    fname = tv_get_string(&argvars[0]);
    if (*fname == NUL || (fd = mch_fopen((char *)fname, READBIN)) == NULL)
    {
	EMSG2(_(e_notopen), *fname == NUL ? (char_u *)_("<empty>") : fname);
	return;
    }

    while (cnt < maxline || maxline < 0)
    {
	readlen = (int)fread(buf, 1, io_size, fd);

	/* This for loop processes what was read, but is also entered at end
	 * of file so that either:
	 * - an incomplete line gets written
	 * - a "binary" file gets an empty line at the end if it ends in a
	 *   newline.  */
	for (p = buf, start = buf;
		p < buf + readlen || (readlen <= 0 && (prevlen > 0 || binary));
		++p)
	{
	    if (*p == '\n' || readlen <= 0)
	    {
		listitem_T  *li;
		char_u	    *s	= NULL;
		long_u	    len = p - start;

		/* Finished a line.  Remove CRs before NL. */
		if (readlen > 0 && !binary)
		{
		    while (len > 0 && start[len - 1] == '\r')
			--len;
		    /* removal may cross back to the "prev" string */
		    if (len == 0)
			while (prevlen > 0 && prev[prevlen - 1] == '\r')
			    --prevlen;
		}
		if (prevlen == 0)
		    s = vim_strnsave(start, (int)len);
		else
		{
		    /* Change "prev" buffer to be the right size.  This way
		     * the bytes are only copied once, and very long lines are
		     * allocated only once.  */
		    if ((s = vim_realloc(prev, prevlen + len + 1)) != NULL)
		    {
			mch_memmove(s + prevlen, start, len);
			s[prevlen + len] = NUL;
			prev = NULL; /* the list will own the string */
			prevlen = prevsize = 0;
		    }
		}
		if (s == NULL)
		{
		    do_outofmem_msg((long_u) prevlen + len + 1);
		    failed = TRUE;
		    break;
		}

		if ((li = listitem_alloc()) == NULL)
		{
		    vim_free(s);
		    failed = TRUE;
		    break;
		}
		li->li_tv.v_type = VAR_STRING;
		li->li_tv.v_lock = 0;
		li->li_tv.vval.v_string = s;
		list_append(rettv->vval.v_list, li);

		start = p + 1; /* step over newline */
		if ((++cnt >= maxline && maxline >= 0) || readlen <= 0)
		    break;
	    }
	    else if (*p == NUL)
		*p = '\n';
#ifdef FEAT_MBYTE
	    /* Check for utf8 "bom"; U+FEFF is encoded as EF BB BF.  Do this
	     * when finding the BF and check the previous two bytes. */
	    else if (*p == 0xbf && enc_utf8 && !binary)
	    {
		/* Find the two bytes before the 0xbf.	If p is at buf, or buf
		 * + 1, these may be in the "prev" string. */
		char_u back1 = p >= buf + 1 ? p[-1]
				     : prevlen >= 1 ? prev[prevlen - 1] : NUL;
		char_u back2 = p >= buf + 2 ? p[-2]
			  : p == buf + 1 && prevlen >= 1 ? prev[prevlen - 1]
			  : prevlen >= 2 ? prev[prevlen - 2] : NUL;

		if (back2 == 0xef && back1 == 0xbb)
		{
		    char_u *dest = p - 2;

		    /* Usually a BOM is at the beginning of a file, and so at
		     * the beginning of a line; then we can just step over it.
		     */
		    if (start == dest)
			start = p + 1;
		    else
		    {
			/* have to shuffle buf to close gap */
			int adjust_prevlen = 0;

			if (dest < buf)
			{
			    adjust_prevlen = (int)(buf - dest); /* must be 1 or 2 */
			    dest = buf;
			}
			if (readlen > p - buf + 1)
			    mch_memmove(dest, p + 1, readlen - (p - buf) - 1);
			readlen -= 3 - adjust_prevlen;
			prevlen -= adjust_prevlen;
			p = dest - 1;
		    }
		}
	    }
#endif
	} /* for */

	if (failed || (cnt >= maxline && maxline >= 0) || readlen <= 0)
	    break;
	if (start < p)
	{
	    /* There's part of a line in buf, store it in "prev". */
	    if (p - start + prevlen >= prevsize)
	    {
		/* need bigger "prev" buffer */
		char_u *newprev;

		/* A common use case is ordinary text files and "prev" gets a
		 * fragment of a line, so the first allocation is made
		 * small, to avoid repeatedly 'allocing' large and
		 * 'reallocing' small. */
		if (prevsize == 0)
		    prevsize = (long)(p - start);
		else
		{
		    long grow50pc = (prevsize * 3) / 2;
		    long growmin  = (long)((p - start) * 2 + prevlen);
		    prevsize = grow50pc > growmin ? grow50pc : growmin;
		}
		newprev = prev == NULL ? alloc(prevsize)
						: vim_realloc(prev, prevsize);
		if (newprev == NULL)
		{
		    do_outofmem_msg((long_u)prevsize);
		    failed = TRUE;
		    break;
		}
		prev = newprev;
	    }
	    /* Add the line part to end of "prev". */
	    mch_memmove(prev + prevlen, start, p - start);
	    prevlen += (long)(p - start);
	}
    } /* while */

    /*
     * For a negative line count use only the lines at the end of the file,
     * free the rest.
     */
    if (!failed && maxline < 0)
	while (cnt > -maxline)
	{
	    listitem_remove(rettv->vval.v_list, rettv->vval.v_list->lv_first);
	    --cnt;
	}

    if (failed)
    {
	list_free(rettv->vval.v_list);
	/* readfile doc says an empty list is returned on error */
	rettv->vval.v_list = list_alloc();
    }

    vim_free(prev);
    fclose(fd);
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
f_reltime(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
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
	EMSG(_("E240: No connection to the X server"));
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
    if (argvars[2].v_type != VAR_UNKNOWN
	    && argvars[3].v_type != VAR_UNKNOWN)
	timeout = tv_get_number(&argvars[3]);

    server_name = tv_get_string_chk(&argvars[0]);
    if (server_name == NULL)
	return;		/* type error; errmsg already given */
    keys = tv_get_string_buf(&argvars[1], buf);
# ifdef WIN32
    if (serverSendToVim(server_name, keys, &r, &w, expr, timeout, TRUE) < 0)
# else
    if (serverSendToVim(X_DISPLAY, server_name, keys, &r, &w, expr, timeout,
								  0, TRUE) < 0)
# endif
    {
	if (r != NULL)
	{
	    EMSG(r);		/* sending worked but evaluation failed */
	    vim_free(r);
	}
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
# ifdef WIN32
    /* On Win32 it's done in this application. */
    {
	char_u	*server_name = tv_get_string_chk(&argvars[0]);

	if (server_name != NULL)
	    serverForeground(server_name);
    }
# else
    /* Send a foreground() expression to the server. */
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
# ifdef WIN32
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
	return;		/* type error; errmsg already given */
    }
# ifdef WIN32
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
# ifdef WIN32
	/* The server's HWND is encoded in the 'id' parameter */
	long_u		n = 0;
# endif

	if (argvars[1].v_type != VAR_UNKNOWN)
	    timeout = tv_get_number(&argvars[1]);

# ifdef WIN32
	sscanf((char *)serverid, SCANF_HEX_LONG_U, &n);
	if (n != 0)
	    r = serverGetReply((HWND)n, FALSE, TRUE, TRUE, timeout);
	if (r == NULL)
# else
	if (check_connection() == FAIL
		|| serverReadReply(X_DISPLAY, serverStrToWin(serverid),
						       &r, FALSE, timeout) < 0)
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
	return;		/* type error; errmsg already given */
    if (serverName != NULL)
	EMSG(_("E941: already started a server"));
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
    EMSG(_("E942: +clientserver feature not available"));
#endif
}

/*
 * "remove()" function
 */
    static void
f_remove(typval_T *argvars, typval_T *rettv)
{
    list_T	*l;
    listitem_T	*item, *item2;
    listitem_T	*li;
    long	idx;
    long	end;
    char_u	*key;
    dict_T	*d;
    dictitem_T	*di;
    char_u	*arg_errmsg = (char_u *)N_("remove() argument");

    if (argvars[0].v_type == VAR_DICT)
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	    EMSG2(_(e_toomanyarg), "remove()");
	else if ((d = argvars[0].vval.v_dict) != NULL
		&& !tv_check_lock(d->dv_lock, arg_errmsg, TRUE))
	{
	    key = tv_get_string_chk(&argvars[1]);
	    if (key != NULL)
	    {
		di = dict_find(d, key, -1);
		if (di == NULL)
		    EMSG2(_(e_dictkey), key);
		else if (!var_check_fixed(di->di_flags, arg_errmsg, TRUE)
			    && !var_check_ro(di->di_flags, arg_errmsg, TRUE))
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
	    && !tv_check_lock(l->lv_lock, arg_errmsg, TRUE))
    {
	int	    error = FALSE;

	idx = (long)tv_get_number_chk(&argvars[1], &error);
	if (error)
	    ;		/* type error: do nothing, errmsg already given */
	else if ((item = list_find(l, idx)) == NULL)
	    EMSGN(_(e_listidx), idx);
	else
	{
	    if (argvars[2].v_type == VAR_UNKNOWN)
	    {
		/* Remove one item, return its value. */
		vimlist_remove(l, item, item);
		*rettv = item->li_tv;
		vim_free(item);
	    }
	    else
	    {
		/* Remove range of items, return list with values. */
		end = (long)tv_get_number_chk(&argvars[2], &error);
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
			vimlist_remove(l, item, item2);
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

/*
 * "resolve()" function
 */
    static void
f_resolve(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
#ifdef HAVE_READLINK
    char_u	*buf = NULL;
#endif

    p = tv_get_string(&argvars[0]);
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
	{
	    has_trailing_pathsep = TRUE;
	    p[len - 1] = NUL; /* the trailing slash breaks readlink() */
	}

	q = getnextcomp(p);
	if (*q != NUL)
	{
	    /* Separate the first path component in "p", and keep the
	     * remainder (beginning with the path separator). */
	    remain = vim_strsave(q - 1);
	    q[-1] = NUL;
	}

	buf = alloc(MAXPATHL + 1);
	if (buf == NULL)
	    goto fail;

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
		VIM_CLEAR(remain);
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
    vim_free(buf);
#endif
    rettv->v_type = VAR_STRING;
}

/*
 * "reverse({list})" function
 */
    static void
f_reverse(typval_T *argvars, typval_T *rettv)
{
    list_T	*l;
    listitem_T	*li, *ni;

    if (argvars[0].v_type != VAR_LIST)
	EMSG2(_(e_listarg), "reverse()");
    else if ((l = argvars[0].vval.v_list) != NULL
	    && !tv_check_lock(l->lv_lock,
				    (char_u *)N_("reverse() argument"), TRUE))
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
	rettv_list_set(rettv, l);
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
#define SP_COLUMN	0x80	    /* start at cursor column */

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
				 case 'z': mask = SP_COLUMN; break;
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
    int		retval = 0;	/* default: FAIL */
    long	lnum_stop = 0;
    proftime_T	tm;
#ifdef FEAT_RELTIME
    long	time_limit = 0;
#endif
    int		options = SEARCH_KEEP;
    int		subpatnum;

    pat = tv_get_string(&argvars[0]);
    dir = get_search_arg(&argvars[1], flagsp);	/* may set p_ws */
    if (dir == 0)
	goto theend;
    flags = *flagsp;
    if (flags & SP_START)
	options |= SEARCH_START;
    if (flags & SP_END)
	options |= SEARCH_END;
    if (flags & SP_COLUMN)
	options |= SEARCH_COL;

    /* Optional arguments: line number to stop searching and timeout. */
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
	EMSG2(_(e_invarg2), tv_get_string(&argvars[1]));
	goto theend;
    }

    pos = save_cursor = curwin->w_cursor;
    subpatnum = searchit(curwin, curbuf, &pos, NULL, dir, pat, 1L,
			   options, RE_SEARCH, (linenr_T)lnum_stop, &tm, NULL);
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
    if (row < 0 || row >= screen_Rows
	    || col < 0 || col >= screen_Columns)
	c = -1;
    else
    {
	off = LineOffset[row] + col;
#ifdef FEAT_MBYTE
	if (enc_utf8 && ScreenLinesUC[off] != 0)
	    c = ScreenLinesUC[off];
	else
#endif
	    c = ScreenLines[off];
    }
    rettv->vval.v_number = c;
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

    rettv->vval.v_number = 1;	/* default: FAIL */

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
    int		retval = 0;		/* default: FAIL */
    long	lnum_stop = 0;
    long	time_limit = 0;

    /* Get the three pattern arguments: start, middle, end. Will result in an
     * error if not a valid argument. */
    spat = tv_get_string_chk(&argvars[0]);
    mpat = tv_get_string_buf_chk(&argvars[1], nbuf1);
    epat = tv_get_string_buf_chk(&argvars[2], nbuf2);
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
	EMSG2(_(e_invarg2), tv_get_string(&argvars[3]));
	goto theend;
    }

    /* Using 'r' implies 'W', otherwise it doesn't work. */
    if (flags & SP_REPEAT)
	p_ws = FALSE;

    /* Optional fifth argument: skip expression */
    if (argvars[3].v_type == VAR_UNKNOWN
	    || argvars[4].v_type == VAR_UNKNOWN)
	skip = NULL;
    else
    {
	skip = &argvars[4];
	if (skip->v_type != VAR_FUNC && skip->v_type != VAR_PARTIAL
	    && skip->v_type != VAR_STRING)
	{
	    /* Type error */
	    EMSG2(_(e_invarg2), tv_get_string(&argvars[4]));
	    goto theend;
	}
	if (argvars[5].v_type != VAR_UNKNOWN)
	{
	    lnum_stop = (long)tv_get_number_chk(&argvars[5], NULL);
	    if (lnum_stop < 0)
	    {
		EMSG2(_(e_invarg2), tv_get_string(&argvars[5]));
		goto theend;
	    }
#ifdef FEAT_RELTIME
	    if (argvars[6].v_type != VAR_UNKNOWN)
	    {
		time_limit = (long)tv_get_number_chk(&argvars[6], NULL);
		if (time_limit < 0)
		{
		    EMSG2(_(e_invarg2), tv_get_string(&argvars[6]));
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
    char_u	*spat,	    /* start pattern */
    char_u	*mpat,	    /* middle pattern */
    char_u	*epat,	    /* end pattern */
    int		dir,	    /* BACKWARD or FORWARD */
    typval_T	*skip,	    /* skip expression */
    int		flags,	    /* SP_SETPCMARK and other SP_ values */
    pos_T	*match_pos,
    linenr_T	lnum_stop,  /* stop at this line if not zero */
    long	time_limit UNUSED) /* stop after this many msec */
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
    pat2 = alloc((unsigned)(STRLEN(spat) + STRLEN(epat) + 17));
    pat3 = alloc((unsigned)(STRLEN(spat) + STRLEN(mpat) + STRLEN(epat) + 25));
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
	/* Empty string means to not use the skip expression. */
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
	n = searchit(curwin, curbuf, &pos, NULL, dir, pat, 1L,
				     options, RE_SEARCH, lnum_stop, &tm, NULL);
	if (n == FAIL || (firstpos.lnum != 0 && EQUAL_POS(pos, firstpos)))
	    /* didn't find it or found the first match again: FAIL */
	    break;

	if (firstpos.lnum == 0)
	    firstpos = pos;
	if (EQUAL_POS(pos, foundpos))
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
	if (use_skip)
	{
	    save_pos = curwin->w_cursor;
	    curwin->w_cursor = pos;
	    err = FALSE;
	    r = eval_expr_to_bool(skip, &err);
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
	EMSG(_("E258: Unable to send to client"));
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
 * "setbufline()" function
 */
    static void
f_setbufline(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum;
    buf_T	*buf;

    buf = tv_get_buf(&argvars[0], FALSE);
    if (buf == NULL)
	rettv->vval.v_number = 1; /* FAIL */
    else
    {
	lnum = tv_get_lnum_buf(&argvars[1], buf);
	set_buffer_lines(buf, lnum, FALSE, &argvars[2], rettv);
    }
}

/*
 * "setbufvar()" function
 */
    static void
f_setbufvar(typval_T *argvars, typval_T *rettv UNUSED)
{
    buf_T	*buf;
    char_u	*varname, *bufvarname;
    typval_T	*varp;
    char_u	nbuf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;
    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    varname = tv_get_string_chk(&argvars[1]);
    buf = tv_get_buf(&argvars[0], FALSE);
    varp = &argvars[2];

    if (buf != NULL && varname != NULL && varp != NULL)
    {
	if (*varname == '&')
	{
	    long	numval;
	    char_u	*strval;
	    int		error = FALSE;
	    aco_save_T	aco;

	    /* set curbuf to be our buf, temporarily */
	    aucmd_prepbuf(&aco, buf);

	    ++varname;
	    numval = (long)tv_get_number_chk(varp, &error);
	    strval = tv_get_string_buf_chk(varp, nbuf);
	    if (!error && strval != NULL)
		set_option_value(varname, numval, strval, OPT_LOCAL);

	    /* reset notion of buffer */
	    aucmd_restbuf(&aco);
	}
	else
	{
	    buf_T *save_curbuf = curbuf;

	    bufvarname = alloc((unsigned)STRLEN(varname) + 3);
	    if (bufvarname != NULL)
	    {
		curbuf = buf;
		STRCPY(bufvarname, "b:");
		STRCPY(bufvarname + 2, varname);
		set_var(bufvarname, varp, TRUE);
		vim_free(bufvarname);
		curbuf = save_curbuf;
	    }
	}
    }
}

    static void
f_setcharsearch(typval_T *argvars, typval_T *rettv UNUSED)
{
    dict_T	*d;
    dictitem_T	*di;
    char_u	*csearch;

    if (argvars[0].v_type != VAR_DICT)
    {
	EMSG(_(e_dictreq));
	return;
    }

    if ((d = argvars[0].vval.v_dict) != NULL)
    {
	csearch = dict_get_string(d, (char_u *)"char", FALSE);
	if (csearch != NULL)
	{
#ifdef FEAT_MBYTE
	    if (enc_utf8)
	    {
		int pcc[MAX_MCO];
		int c = utfc_ptr2char(csearch, pcc);

		set_last_csearch(c, csearch, utfc_ptr2len(csearch));
	    }
	    else
#endif
		set_last_csearch(PTR2CHAR(csearch),
						csearch, MB_PTR2LEN(csearch));
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
 * "setcmdpos()" function
 */
    static void
f_setcmdpos(typval_T *argvars, typval_T *rettv)
{
    int		pos = (int)tv_get_number(&argvars[0]) - 1;

    if (pos >= 0)
	rettv->vval.v_number = set_cmdline_pos(pos);
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
	EMSG2(_(e_invarg2), mode_str);
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
 * "setline()" function
 */
    static void
f_setline(typval_T *argvars, typval_T *rettv)
{
    linenr_T	lnum = tv_get_lnum(&argvars[0]);

    set_buffer_lines(curbuf, lnum, FALSE, &argvars[1], rettv);
}

/*
 * Used by "setqflist()" and "setloclist()" functions
 */
    static void
set_qf_ll_list(
    win_T	*wp UNUSED,
    typval_T	*list_arg UNUSED,
    typval_T	*action_arg UNUSED,
    typval_T	*what_arg UNUSED,
    typval_T	*rettv)
{
#ifdef FEAT_QUICKFIX
    static char *e_invact = N_("E927: Invalid action: '%s'");
    char_u	*act;
    int		action = 0;
    static int	recursive = 0;
#endif

    rettv->vval.v_number = -1;

#ifdef FEAT_QUICKFIX
    if (list_arg->v_type != VAR_LIST)
	EMSG(_(e_listreq));
    else if (recursive != 0)
	EMSG(_(e_au_recursive));
    else
    {
	list_T  *l = list_arg->vval.v_list;
	dict_T	*d = NULL;
	int	valid_dict = TRUE;

	if (action_arg->v_type == VAR_STRING)
	{
	    act = tv_get_string_chk(action_arg);
	    if (act == NULL)
		return;		/* type error; errmsg already given */
	    if ((*act == 'a' || *act == 'r' || *act == ' ' || *act == 'f') &&
		    act[1] == NUL)
		action = *act;
	    else
		EMSG2(_(e_invact), act);
	}
	else if (action_arg->v_type == VAR_UNKNOWN)
	    action = ' ';
	else
	    EMSG(_(e_stringreq));

	if (action_arg->v_type != VAR_UNKNOWN
		&& what_arg->v_type != VAR_UNKNOWN)
	{
	    if (what_arg->v_type == VAR_DICT)
		d = what_arg->vval.v_dict;
	    else
	    {
		EMSG(_(e_dictreq));
		valid_dict = FALSE;
	    }
	}

	++recursive;
	if (l != NULL && action && valid_dict && set_errorlist(wp, l, action,
		     (char_u *)(wp == NULL ? ":setqflist()" : ":setloclist()"),
		     d) == OK)
	    rettv->vval.v_number = 0;
	--recursive;
    }
#endif
}

/*
 * "setloclist()" function
 */
    static void
f_setloclist(typval_T *argvars, typval_T *rettv)
{
    win_T	*win;

    rettv->vval.v_number = -1;

    win = find_win_by_nr_or_id(&argvars[0]);
    if (win != NULL)
	set_qf_ll_list(win, &argvars[1], &argvars[2], &argvars[3], rettv);
}

/*
 * "setmatches()" function
 */
    static void
f_setmatches(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#ifdef FEAT_SEARCH_EXTRA
    list_T	*l;
    listitem_T	*li;
    dict_T	*d;
    list_T	*s = NULL;

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
			&& (dict_find(d, (char_u *)"pattern", -1) != NULL
			    || dict_find(d, (char_u *)"pos1", -1) != NULL)
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
	    int		i = 0;
	    char_u	buf[5];
	    dictitem_T  *di;
	    char_u	*group;
	    int		priority;
	    int		id;
	    char_u	*conceal;

	    d = li->li_tv.vval.v_dict;
	    if (dict_find(d, (char_u *)"pattern", -1) == NULL)
	    {
		if (s == NULL)
		{
		    s = list_alloc();
		    if (s == NULL)
			return;
		}

		/* match from matchaddpos() */
		for (i = 1; i < 9; i++)
		{
		    sprintf((char *)buf, (char *)"pos%d", i);
		    if ((di = dict_find(d, (char_u *)buf, -1)) != NULL)
		    {
			if (di->di_tv.v_type != VAR_LIST)
			    return;

			list_append_tv(s, &di->di_tv);
			s->lv_refcount++;
		    }
		    else
			break;
		}
	    }

	    group = dict_get_string(d, (char_u *)"group", TRUE);
	    priority = (int)dict_get_number(d, (char_u *)"priority");
	    id = (int)dict_get_number(d, (char_u *)"id");
	    conceal = dict_find(d, (char_u *)"conceal", -1) != NULL
			      ? dict_get_string(d, (char_u *)"conceal", TRUE)
			      : NULL;
	    if (i == 0)
	    {
		match_add(curwin, group,
		    dict_get_string(d, (char_u *)"pattern", FALSE),
		    priority, id, NULL, conceal);
	    }
	    else
	    {
		match_add(curwin, group, NULL, priority, id, s, conceal);
		list_unref(s);
		s = NULL;
	    }
	    vim_free(group);
	    vim_free(conceal);

	    li = li->li_next;
	}
	rettv->vval.v_number = 0;
    }
#endif
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
	    if (--pos.col < 0)
		pos.col = 0;
	    if (name[0] == '.' && name[1] == NUL)
	    {
		/* set cursor; "fnum" is ignored */
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
    static void
f_setqflist(typval_T *argvars, typval_T *rettv)
{
    set_qf_ll_list(NULL, &argvars[0], &argvars[1], &argvars[2], rettv);
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
    rettv->vval.v_number = 1;		/* FAIL is default */

    if (strregname == NULL)
	return;		/* type error; errmsg already given */
    regname = *strregname;
    if (regname == 0 || regname == '@')
	regname = '"';

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	stropt = tv_get_string_chk(&argvars[2]);
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
		case 'b': case Ctrl_V:	/* block-wise selection */
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

	/* If the list is NULL handle like an empty list. */
	len = ll == NULL ? 0 : ll->lv_len;

	/* First half: use for pointers to result lines; second half: use for
	 * pointers to allocated copies. */
	lstval = (char_u **)alloc(sizeof(char_u *) * ((len + 1) * 2));
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
		/* Need to make a copy, next tv_get_string_buf_chk() will
		 * overwrite the string. */
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
 * "settabvar()" function
 */
    static void
f_settabvar(typval_T *argvars, typval_T *rettv)
{
    tabpage_T	*save_curtab;
    tabpage_T	*tp;
    char_u	*varname, *tabvarname;
    typval_T	*varp;

    rettv->vval.v_number = 0;

    if (check_restricted() || check_secure())
	return;

    tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    varname = tv_get_string_chk(&argvars[1]);
    varp = &argvars[2];

    if (varname != NULL && varp != NULL && tp != NULL)
    {
	save_curtab = curtab;
	goto_tabpage_tp(tp, FALSE, FALSE);

	tabvarname = alloc((unsigned)STRLEN(varname) + 3);
	if (tabvarname != NULL)
	{
	    STRCPY(tabvarname, "t:");
	    STRCPY(tabvarname + 2, varname);
	    set_var(tabvarname, varp, TRUE);
	    vim_free(tabvarname);
	}

	/* Restore current tabpage */
	if (valid_tabpage(save_curtab))
	    goto_tabpage_tp(save_curtab, FALSE, FALSE);
    }
}

/*
 * "settabwinvar()" function
 */
    static void
f_settabwinvar(typval_T *argvars, typval_T *rettv)
{
    setwinvar(argvars, rettv, 1);
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
	EMSG(_(e_dictreq));
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
	if ((*actstr == 'r' || *actstr == 'a') && actstr[1] == NUL)
	    action = *actstr;
	else
	{
	    EMSG2(_(e_invact2), actstr);
	    return;
	}
    }
    else
    {
	EMSG(_(e_stringreq));
	return;
    }

    if (set_tagstack(wp, d, action) == OK)
	rettv->vval.v_number = 0;
}

/*
 * "setwinvar()" function
 */
    static void
f_setwinvar(typval_T *argvars, typval_T *rettv)
{
    setwinvar(argvars, rettv, 0);
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
#endif /* FEAT_CRYPT */

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

#ifdef FEAT_SIGNS
/*
 * "sign_define()" function
 */
    static void
f_sign_define(typval_T *argvars, typval_T *rettv)
{
    char_u	*name;
    dict_T	*dict;
    char_u	*icon = NULL;
    char_u	*linehl = NULL;
    char_u	*text = NULL;
    char_u	*texthl = NULL;

    rettv->vval.v_number = -1;

    name = tv_get_string_chk(&argvars[0]);
    if (name == NULL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (argvars[1].v_type != VAR_DICT)
	{
	    EMSG(_(e_dictreq));
	    return;
	}

	// sign attributes
	dict = argvars[1].vval.v_dict;
	if (dict_find(dict, (char_u *)"icon", -1) != NULL)
	    icon = dict_get_string(dict, (char_u *)"icon", TRUE);
	if (dict_find(dict, (char_u *)"linehl", -1) != NULL)
	    linehl = dict_get_string(dict, (char_u *)"linehl", TRUE);
	if (dict_find(dict, (char_u *)"text", -1) != NULL)
	    text = dict_get_string(dict, (char_u *)"text", TRUE);
	if (dict_find(dict, (char_u *)"texthl", -1) != NULL)
	    texthl = dict_get_string(dict, (char_u *)"texthl", TRUE);
    }

    if (sign_define_by_name(name, icon, linehl, text, texthl) == OK)
	rettv->vval.v_number = 0;

    vim_free(icon);
    vim_free(linehl);
    vim_free(text);
    vim_free(texthl);
}

/*
 * "sign_getdefined()" function
 */
    static void
f_sign_getdefined(typval_T *argvars, typval_T *rettv)
{
    char_u	*name = NULL;

    if (rettv_list_alloc_id(rettv, aid_sign_getdefined) != OK)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
	name = tv_get_string(&argvars[0]);

    sign_getlist(name, rettv->vval.v_list);
}

/*
 * "sign_getplaced()" function
 */
    static void
f_sign_getplaced(typval_T *argvars, typval_T *rettv)
{
    buf_T	*buf = NULL;
    dict_T	*dict;
    dictitem_T	*di;
    linenr_T	lnum = 0;
    int		sign_id = 0;
    char_u	*group = NULL;
    int		notanum = FALSE;

    if (rettv_list_alloc_id(rettv, aid_sign_getplaced) != OK)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	// get signs placed in this buffer
	buf = tv_get_buf(&argvars[0], FALSE);
	if (buf == NULL)
	{
	    EMSG2(_("E158: Invalid buffer name: %s"),
						tv_get_string(&argvars[0]));
	    return;
	}

	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    if (argvars[1].v_type != VAR_DICT ||
				((dict = argvars[1].vval.v_dict) == NULL))
	    {
		EMSG(_(e_dictreq));
		return;
	    }
	    if ((di = dict_find(dict, (char_u *)"lnum", -1)) != NULL)
	    {
		// get signs placed at this line
		(void)tv_get_number_chk(&di->di_tv, &notanum);
		if (notanum)
		    return;
		lnum = tv_get_lnum(&di->di_tv);
	    }
	    if ((di = dict_find(dict, (char_u *)"id", -1)) != NULL)
	    {
		// get sign placed with this identifier
		sign_id = (int)tv_get_number_chk(&di->di_tv, &notanum);
		if (notanum)
		    return;
	    }
	    if ((di = dict_find(dict, (char_u *)"group", -1)) != NULL)
	    {
		group = tv_get_string_chk(&di->di_tv);
		if (group == NULL)
		    return;
		if (*group == '\0')	// empty string means global group
		    group = NULL;
	    }
	}
    }

    sign_get_placed(buf, lnum, sign_id, group, rettv->vval.v_list);
}

/*
 * "sign_place()" function
 */
    static void
f_sign_place(typval_T *argvars, typval_T *rettv)
{
    int		sign_id;
    char_u	*group = NULL;
    char_u	*sign_name;
    buf_T	*buf;
    dict_T	*dict;
    dictitem_T	*di;
    linenr_T	lnum = 0;
    int		prio = SIGN_DEF_PRIO;
    int		notanum = FALSE;

    rettv->vval.v_number = -1;

    // Sign identifer
    sign_id = (int)tv_get_number_chk(&argvars[0], &notanum);
    if (notanum)
	return;
    if (sign_id < 0)
    {
	EMSG(_(e_invarg));
	return;
    }

    // Sign group
    group = tv_get_string_chk(&argvars[1]);
    if (group == NULL)
	return;
    if (group[0] == '\0')
	group = NULL;			// global sign group
    else
    {
	group = vim_strsave(group);
	if (group == NULL)
	    return;
    }

    // Sign name
    sign_name = tv_get_string_chk(&argvars[2]);
    if (sign_name == NULL)
	goto cleanup;

    // Buffer to place the sign
    buf = tv_get_buf(&argvars[3], FALSE);
    if (buf == NULL)
    {
	EMSG2(_("E158: Invalid buffer name: %s"), tv_get_string(&argvars[2]));
	goto cleanup;
    }

    if (argvars[4].v_type != VAR_UNKNOWN)
    {
	if (argvars[4].v_type != VAR_DICT ||
				((dict = argvars[4].vval.v_dict) == NULL))
	{
	    EMSG(_(e_dictreq));
	    goto cleanup;
	}

	// Line number where the sign is to be placed
	if ((di = dict_find(dict, (char_u *)"lnum", -1)) != NULL)
	{
	    (void)tv_get_number_chk(&di->di_tv, &notanum);
	    if (notanum)
		goto cleanup;
	    lnum = tv_get_lnum(&di->di_tv);
	}
	if ((di = dict_find(dict, (char_u *)"priority", -1)) != NULL)
	{
	    // Sign priority
	    prio = (int)tv_get_number_chk(&di->di_tv, &notanum);
	    if (notanum)
		goto cleanup;
	}
    }

    if (sign_place(&sign_id, group, sign_name, buf, lnum, prio) == OK)
	rettv->vval.v_number = sign_id;

cleanup:
    vim_free(group);
}

/*
 * "sign_undefine()" function
 */
    static void
f_sign_undefine(typval_T *argvars, typval_T *rettv)
{
    char_u *name;

    rettv->vval.v_number = -1;

    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	// Free all the signs
	free_signs();
	rettv->vval.v_number = 0;
    }
    else
    {
	// Free only the specified sign
	name = tv_get_string_chk(&argvars[0]);
	if (name == NULL)
	    return;

	if (sign_undefine_by_name(name) == OK)
	    rettv->vval.v_number = 0;
    }
}

/*
 * "sign_unplace()" function
 */
    static void
f_sign_unplace(typval_T *argvars, typval_T *rettv)
{
    dict_T	*dict;
    dictitem_T	*di;
    int		sign_id = 0;
    buf_T	*buf = NULL;
    char_u	*group = NULL;

    rettv->vval.v_number = -1;

    if (argvars[0].v_type != VAR_STRING)
    {
	EMSG(_(e_invarg));
	return;
    }

    group = tv_get_string(&argvars[0]);
    if (group[0] == '\0')
	group = NULL;			// global sign group
    else
    {
	group = vim_strsave(group);
	if (group == NULL)
	    return;
    }

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (argvars[1].v_type != VAR_DICT)
	{
	    EMSG(_(e_dictreq));
	    goto cleanup;
	}
	dict = argvars[1].vval.v_dict;

	if ((di = dict_find(dict, (char_u *)"buffer", -1)) != NULL)
	{
	    buf = tv_get_buf(&di->di_tv, FALSE);
	    if (buf == NULL)
	    {
		EMSG2(_("E158: Invalid buffer name: %s"),
						tv_get_string(&di->di_tv));
		goto cleanup;
	    }
	}
	if (dict_find(dict, (char_u *)"id", -1) != NULL)
	    sign_id = dict_get_number(dict, (char_u *)"id");
    }

    if (buf == NULL)
    {
	// Delete the sign in all the buffers
	FOR_ALL_BUFFERS(buf)
	    if (sign_unplace(sign_id, group, buf, 0) == OK)
		rettv->vval.v_number = 0;
    }
    else
    {
	if (sign_unplace(sign_id, group, buf, 0) == OK)
	    rettv->vval.v_number = 0;
    }

cleanup:
    vim_free(group);
}
#endif

/*
 * "simplify()" function
 */
    static void
f_simplify(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;

    p = tv_get_string(&argvars[0]);
    rettv->vval.v_string = vim_strsave(p);
    simplify_filename(rettv->vval.v_string);	/* simplify in place */
    rettv->v_type = VAR_STRING;
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

static int
#ifdef __BORLANDC__
    _RTLENTRYF
#endif
	item_compare(const void *s1, const void *s2);
static int
#ifdef __BORLANDC__
    _RTLENTRYF
#endif
	item_compare2(const void *s1, const void *s2);

/* struct used in the array that's given to qsort() */
typedef struct
{
    listitem_T	*item;
    int		idx;
} sortItem_T;

/* struct storing information about current sort */
typedef struct
{
    int		item_compare_ic;
    int		item_compare_numeric;
    int		item_compare_numbers;
#ifdef FEAT_FLOAT
    int		item_compare_float;
#endif
    char_u	*item_compare_func;
    partial_T	*item_compare_partial;
    dict_T	*item_compare_selfdict;
    int		item_compare_func_err;
    int		item_compare_keep_zero;
} sortinfo_T;
static sortinfo_T	*sortinfo = NULL;
#define ITEM_COMPARE_FAIL 999

/*
 * Compare functions for f_sort() and f_uniq() below.
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
item_compare(const void *s1, const void *s2)
{
    sortItem_T  *si1, *si2;
    typval_T	*tv1, *tv2;
    char_u	*p1, *p2;
    char_u	*tofree1 = NULL, *tofree2 = NULL;
    int		res;
    char_u	numbuf1[NUMBUFLEN];
    char_u	numbuf2[NUMBUFLEN];

    si1 = (sortItem_T *)s1;
    si2 = (sortItem_T *)s2;
    tv1 = &si1->item->li_tv;
    tv2 = &si2->item->li_tv;

    if (sortinfo->item_compare_numbers)
    {
	varnumber_T	v1 = tv_get_number(tv1);
	varnumber_T	v2 = tv_get_number(tv2);

	return v1 == v2 ? 0 : v1 > v2 ? 1 : -1;
    }

#ifdef FEAT_FLOAT
    if (sortinfo->item_compare_float)
    {
	float_T	v1 = tv_get_float(tv1);
	float_T	v2 = tv_get_float(tv2);

	return v1 == v2 ? 0 : v1 > v2 ? 1 : -1;
    }
#endif

    /* tv2string() puts quotes around a string and allocates memory.  Don't do
     * that for string variables. Use a single quote when comparing with a
     * non-string to do what the docs promise. */
    if (tv1->v_type == VAR_STRING)
    {
	if (tv2->v_type != VAR_STRING || sortinfo->item_compare_numeric)
	    p1 = (char_u *)"'";
	else
	    p1 = tv1->vval.v_string;
    }
    else
	p1 = tv2string(tv1, &tofree1, numbuf1, 0);
    if (tv2->v_type == VAR_STRING)
    {
	if (tv1->v_type != VAR_STRING || sortinfo->item_compare_numeric)
	    p2 = (char_u *)"'";
	else
	    p2 = tv2->vval.v_string;
    }
    else
	p2 = tv2string(tv2, &tofree2, numbuf2, 0);
    if (p1 == NULL)
	p1 = (char_u *)"";
    if (p2 == NULL)
	p2 = (char_u *)"";
    if (!sortinfo->item_compare_numeric)
    {
	if (sortinfo->item_compare_ic)
	    res = STRICMP(p1, p2);
	else
	    res = STRCMP(p1, p2);
    }
    else
    {
	double n1, n2;
	n1 = strtod((char *)p1, (char **)&p1);
	n2 = strtod((char *)p2, (char **)&p2);
	res = n1 == n2 ? 0 : n1 > n2 ? 1 : -1;
    }

    /* When the result would be zero, compare the item indexes.  Makes the
     * sort stable. */
    if (res == 0 && !sortinfo->item_compare_keep_zero)
	res = si1->idx > si2->idx ? 1 : -1;

    vim_free(tofree1);
    vim_free(tofree2);
    return res;
}

    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
item_compare2(const void *s1, const void *s2)
{
    sortItem_T  *si1, *si2;
    int		res;
    typval_T	rettv;
    typval_T	argv[3];
    int		dummy;
    char_u	*func_name;
    partial_T	*partial = sortinfo->item_compare_partial;

    /* shortcut after failure in previous call; compare all items equal */
    if (sortinfo->item_compare_func_err)
	return 0;

    si1 = (sortItem_T *)s1;
    si2 = (sortItem_T *)s2;

    if (partial == NULL)
	func_name = sortinfo->item_compare_func;
    else
	func_name = partial_name(partial);

    /* Copy the values.  This is needed to be able to set v_lock to VAR_FIXED
     * in the copy without changing the original list items. */
    copy_tv(&si1->item->li_tv, &argv[0]);
    copy_tv(&si2->item->li_tv, &argv[1]);

    rettv.v_type = VAR_UNKNOWN;		/* clear_tv() uses this */
    res = call_func(func_name, (int)STRLEN(func_name),
				 &rettv, 2, argv, NULL, 0L, 0L, &dummy, TRUE,
				 partial, sortinfo->item_compare_selfdict);
    clear_tv(&argv[0]);
    clear_tv(&argv[1]);

    if (res == FAIL)
	res = ITEM_COMPARE_FAIL;
    else
	res = (int)tv_get_number_chk(&rettv, &sortinfo->item_compare_func_err);
    if (sortinfo->item_compare_func_err)
	res = ITEM_COMPARE_FAIL;  /* return value has wrong type */
    clear_tv(&rettv);

    /* When the result would be zero, compare the pointers themselves.  Makes
     * the sort stable. */
    if (res == 0 && !sortinfo->item_compare_keep_zero)
	res = si1->idx > si2->idx ? 1 : -1;

    return res;
}

/*
 * "sort({list})" function
 */
    static void
do_sort_uniq(typval_T *argvars, typval_T *rettv, int sort)
{
    list_T	*l;
    listitem_T	*li;
    sortItem_T	*ptrs;
    sortinfo_T	*old_sortinfo;
    sortinfo_T	info;
    long	len;
    long	i;

    /* Pointer to current info struct used in compare function. Save and
     * restore the current one for nested calls. */
    old_sortinfo = sortinfo;
    sortinfo = &info;

    if (argvars[0].v_type != VAR_LIST)
	EMSG2(_(e_listarg), sort ? "sort()" : "uniq()");
    else
    {
	l = argvars[0].vval.v_list;
	if (l == NULL || tv_check_lock(l->lv_lock,
	     (char_u *)(sort ? N_("sort() argument") : N_("uniq() argument")),
									TRUE))
	    goto theend;
	rettv_list_set(rettv, l);

	len = list_len(l);
	if (len <= 1)
	    goto theend;	/* short list sorts pretty quickly */

	info.item_compare_ic = FALSE;
	info.item_compare_numeric = FALSE;
	info.item_compare_numbers = FALSE;
#ifdef FEAT_FLOAT
	info.item_compare_float = FALSE;
#endif
	info.item_compare_func = NULL;
	info.item_compare_partial = NULL;
	info.item_compare_selfdict = NULL;
	if (argvars[1].v_type != VAR_UNKNOWN)
	{
	    /* optional second argument: {func} */
	    if (argvars[1].v_type == VAR_FUNC)
		info.item_compare_func = argvars[1].vval.v_string;
	    else if (argvars[1].v_type == VAR_PARTIAL)
		info.item_compare_partial = argvars[1].vval.v_partial;
	    else
	    {
		int	    error = FALSE;

		i = (long)tv_get_number_chk(&argvars[1], &error);
		if (error)
		    goto theend;	/* type error; errmsg already given */
		if (i == 1)
		    info.item_compare_ic = TRUE;
		else if (argvars[1].v_type != VAR_NUMBER)
		    info.item_compare_func = tv_get_string(&argvars[1]);
		else if (i != 0)
		{
		    EMSG(_(e_invarg));
		    goto theend;
		}
		if (info.item_compare_func != NULL)
		{
		    if (*info.item_compare_func == NUL)
		    {
			/* empty string means default sort */
			info.item_compare_func = NULL;
		    }
		    else if (STRCMP(info.item_compare_func, "n") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_numeric = TRUE;
		    }
		    else if (STRCMP(info.item_compare_func, "N") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_numbers = TRUE;
		    }
#ifdef FEAT_FLOAT
		    else if (STRCMP(info.item_compare_func, "f") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_float = TRUE;
		    }
#endif
		    else if (STRCMP(info.item_compare_func, "i") == 0)
		    {
			info.item_compare_func = NULL;
			info.item_compare_ic = TRUE;
		    }
		}
	    }

	    if (argvars[2].v_type != VAR_UNKNOWN)
	    {
		/* optional third argument: {dict} */
		if (argvars[2].v_type != VAR_DICT)
		{
		    EMSG(_(e_dictreq));
		    goto theend;
		}
		info.item_compare_selfdict = argvars[2].vval.v_dict;
	    }
	}

	/* Make an array with each entry pointing to an item in the List. */
	ptrs = (sortItem_T *)alloc((int)(len * sizeof(sortItem_T)));
	if (ptrs == NULL)
	    goto theend;

	i = 0;
	if (sort)
	{
	    /* sort(): ptrs will be the list to sort */
	    for (li = l->lv_first; li != NULL; li = li->li_next)
	    {
		ptrs[i].item = li;
		ptrs[i].idx = i;
		++i;
	    }

	    info.item_compare_func_err = FALSE;
	    info.item_compare_keep_zero = FALSE;
	    /* test the compare function */
	    if ((info.item_compare_func != NULL
					 || info.item_compare_partial != NULL)
		    && item_compare2((void *)&ptrs[0], (void *)&ptrs[1])
							 == ITEM_COMPARE_FAIL)
		EMSG(_("E702: Sort compare function failed"));
	    else
	    {
		/* Sort the array with item pointers. */
		qsort((void *)ptrs, (size_t)len, sizeof(sortItem_T),
		    info.item_compare_func == NULL
					  && info.item_compare_partial == NULL
					       ? item_compare : item_compare2);

		if (!info.item_compare_func_err)
		{
		    /* Clear the List and append the items in sorted order. */
		    l->lv_first = l->lv_last = l->lv_idx_item = NULL;
		    l->lv_len = 0;
		    for (i = 0; i < len; ++i)
			list_append(l, ptrs[i].item);
		}
	    }
	}
	else
	{
	    int	(*item_compare_func_ptr)(const void *, const void *);

	    /* f_uniq(): ptrs will be a stack of items to remove */
	    info.item_compare_func_err = FALSE;
	    info.item_compare_keep_zero = TRUE;
	    item_compare_func_ptr = info.item_compare_func != NULL
					  || info.item_compare_partial != NULL
					       ? item_compare2 : item_compare;

	    for (li = l->lv_first; li != NULL && li->li_next != NULL;
							     li = li->li_next)
	    {
		if (item_compare_func_ptr((void *)&li, (void *)&li->li_next)
									 == 0)
		    ptrs[i++].item = li;
		if (info.item_compare_func_err)
		{
		    EMSG(_("E882: Uniq compare function failed"));
		    break;
		}
	    }

	    if (!info.item_compare_func_err)
	    {
		while (--i >= 0)
		{
		    li = ptrs[i].item->li_next;
		    ptrs[i].item->li_next = li->li_next;
		    if (li->li_next != NULL)
			li->li_next->li_prev = ptrs[i].item;
		    else
			l->lv_last = ptrs[i].item;
		    list_fix_watch(l, li);
		    listitem_free(li);
		    l->lv_len--;
		}
	    }
	}

	vim_free(ptrs);
    }
theend:
    sortinfo = old_sortinfo;
}

/*
 * "sort({list})" function
 */
    static void
f_sort(typval_T *argvars, typval_T *rettv)
{
    do_sort_uniq(argvars, rettv, TRUE);
}

/*
 * "uniq({list})" function
 */
    static void
f_uniq(typval_T *argvars, typval_T *rettv)
{
    do_sort_uniq(argvars, rettv, FALSE);
}

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
	/* Find the start and length of the badly spelled word. */
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
		capcol -= len;
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

    /* Make 'cpoptions' empty, the 'l' flag should not be used here. */
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
 * "str2nr()" function
 */
    static void
f_str2nr(typval_T *argvars, typval_T *rettv)
{
    int		base = 10;
    char_u	*p;
    varnumber_T	n;
    int		what;
    int		isneg;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	base = (int)tv_get_number(&argvars[1]);
	if (base != 2 && base != 8 && base != 10 && base != 16)
	{
	    EMSG(_(e_invarg));
	    return;
	}
    }

    p = skipwhite(tv_get_string(&argvars[0]));
    isneg = (*p == '-');
    if (*p == '+' || *p == '-')
	p = skipwhite(p + 1);
    switch (base)
    {
	case 2: what = STR2NR_BIN + STR2NR_FORCE; break;
	case 8: what = STR2NR_OCT + STR2NR_FORCE; break;
	case 16: what = STR2NR_HEX + STR2NR_FORCE; break;
	default: what = 0;
    }
    vim_str2nr(p, NULL, NULL, what, &n, NULL, 0);
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
    struct tm	*curtime;
    time_t	seconds;
    char_u	*p;

    rettv->v_type = VAR_STRING;

    p = tv_get_string(&argvars[0]);
    if (argvars[1].v_type == VAR_UNKNOWN)
	seconds = time(NULL);
    else
	seconds = (time_t)tv_get_number(&argvars[1]);
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
 * "strgetchar()" function
 */
    static void
f_strgetchar(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    int		len;
    int		error = FALSE;
    int		charidx;

    rettv->vval.v_number = -1;
    str = tv_get_string_chk(&argvars[0]);
    if (str == NULL)
	return;
    len = (int)STRLEN(str);
    charidx = (int)tv_get_number_chk(&argvars[1], &error);
    if (error)
	return;
#ifdef FEAT_MBYTE
    {
	int	byteidx = 0;

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
#else
    if (charidx < len)
	rettv->vval.v_number = str[charidx];
#endif
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
	return;		/* type error; errmsg already given */

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
    /* Make a copy if we have a value but it's not in allocated memory. */
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
#ifdef FEAT_MBYTE
    varnumber_T		len = 0;
    int			(*func_mb_ptr2char_adv)(char_u **pp);
#endif

    if (argvars[1].v_type != VAR_UNKNOWN)
	skipcc = (int)tv_get_number_chk(&argvars[1], NULL);
    if (skipcc < 0 || skipcc > 1)
	EMSG(_(e_invarg));
    else
    {
#ifdef FEAT_MBYTE
	func_mb_ptr2char_adv = skipcc ? mb_ptr2char_adv : mb_cptr2char_adv;
	while (*s != NUL)
	{
	    func_mb_ptr2char_adv(&s);
	    ++len;
	}
	rettv->vval.v_number = len;
#else
	rettv->vval.v_number = (varnumber_T)(STRLEN(s));
#endif
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

    rettv->vval.v_number = (varnumber_T)(
#ifdef FEAT_MBYTE
	    mb_string2cells(s, -1)
#else
	    STRLEN(s)
#endif
	    );
}

/*
 * "strcharpart()" function
 */
    static void
f_strcharpart(typval_T *argvars, typval_T *rettv)
{
#ifdef FEAT_MBYTE
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
	    len = slen - nbyte;    /* default: all bytes that are available. */
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
#else
    f_strpart(argvars, rettv);
#endif
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
	return;		/* type error; errmsg already given */

    haystack_len = (int)STRLEN(haystack);
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	/* Third argument: upper limit for index */
	end_idx = (int)tv_get_number_chk(&argvars[2], NULL);
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
	EMSGN(_("E935: invalid submatch number: %d"), no);
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

    lnum = tv_get_lnum(argvars);		/* -1 on type error */
    col = (linenr_T)tv_get_number(&argvars[1]) - 1;	/* -1 on type error */
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
	    modec = 0;	/* replace invalid with current */
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
		if (TOLOWER_ASC(what[1]) == 'g')	/* bg[#] */
		    p = highlight_color(id, what, modec);
		else					/* bold */
		    p = highlight_has_attr(id, HL_BOLD, modec);
		break;

	case 'f':					/* fg[#] or font */
		p = highlight_color(id, what, modec);
		break;

	case 'i':
		if (TOLOWER_ASC(what[1]) == 'n')	/* inverse */
		    p = highlight_has_attr(id, HL_INVERSE, modec);
		else					/* italic */
		    p = highlight_has_attr(id, HL_ITALIC, modec);
		break;

	case 'n':					/* name */
		p = get_highlight_name_ext(NULL, id - 1, FALSE);
		break;

	case 'r':					/* reverse */
		p = highlight_has_attr(id, HL_INVERSE, modec);
		break;

	case 's':
		if (TOLOWER_ASC(what[1]) == 'p')	/* sp[#] */
		    p = highlight_color(id, what, modec);
							/* strikeout */
		else if (TOLOWER_ASC(what[1]) == 't' &&
			TOLOWER_ASC(what[2]) == 'r')
		    p = highlight_has_attr(id, HL_STRIKETHROUGH, modec);
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
    lnum = tv_get_lnum(argvars);		/* -1 on type error */
    col = (colnr_T)tv_get_number(&argvars[1]) - 1;	/* -1 on type error */

    vim_memset(str, NUL, sizeof(str));

    if (rettv_list_alloc(rettv) != FAIL)
    {
	if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count
	    && col >= 0 && col <= (long)STRLEN(ml_get(lnum))
	    && curwin->w_p_cole > 0)
	{
	    (void)syn_get_id(curwin, lnum, col, FALSE, NULL, FALSE);
	    syntax_flags = get_syntax_info(&matchid);

	    /* get the conceal character */
	    if ((syntax_flags & HL_CONCEAL) && curwin->w_p_cole < 3)
	    {
		cchar = syn_get_sub_char();
		if (cchar == NUL && curwin->w_p_cole == 1)
		    cchar = (lcs_conceal == NUL) ? ' ' : lcs_conceal;
		if (cchar != NUL)
		{
# ifdef FEAT_MBYTE
		    if (has_mbyte)
			(*mb_char2bytes)(cchar, str);
		    else
# endif
			str[0] = cchar;
		}
	    }
	}

	list_append_number(rettv->vval.v_list,
					    (syntax_flags & HL_CONCEAL) != 0);
	/* -1 to auto-determine strlen */
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
    lnum = tv_get_lnum(argvars);		/* -1 on type error */
    col = (colnr_T)tv_get_number(&argvars[1]) - 1;	/* -1 on type error */

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

    static void
get_cmd_output_as_rettv(
    typval_T	*argvars,
    typval_T	*rettv,
    int		retlist)
{
    char_u	*res = NULL;
    char_u	*p;
    char_u	*infile = NULL;
    int		err = FALSE;
    FILE	*fd;
    list_T	*list = NULL;
    int		flags = SHELL_SILENT;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    if (check_restricted() || check_secure())
	goto errret;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	/*
	 * Write the text to a temp file, to be used for input of the shell
	 * command.
	 */
	if ((infile = vim_tempname('i', TRUE)) == NULL)
	{
	    EMSG(_(e_notmp));
	    goto errret;
	}

	fd = mch_fopen((char *)infile, WRITEBIN);
	if (fd == NULL)
	{
	    EMSG2(_(e_notopen), infile);
	    goto errret;
	}
	if (argvars[1].v_type == VAR_NUMBER)
	{
	    linenr_T	lnum;
	    buf_T	*buf;

	    buf = buflist_findnr(argvars[1].vval.v_number);
	    if (buf == NULL)
	    {
		EMSGN(_(e_nobufnr), argvars[1].vval.v_number);
		fclose(fd);
		goto errret;
	    }

	    for (lnum = 1; lnum <= buf->b_ml.ml_line_count; lnum++)
	    {
		for (p = ml_get_buf(buf, lnum, FALSE); *p != NUL; ++p)
		    if (putc(*p == '\n' ? NUL : *p, fd) == EOF)
		    {
			err = TRUE;
			break;
		    }
		if (putc(NL, fd) == EOF)
		{
		    err = TRUE;
		    break;
		}
	    }
	}
	else if (argvars[1].v_type == VAR_LIST)
	{
	    if (write_list(fd, argvars[1].vval.v_list, TRUE) == FAIL)
		err = TRUE;
	}
	else
	{
	    size_t	len;
	    char_u	buf[NUMBUFLEN];

	    p = tv_get_string_buf_chk(&argvars[1], buf);
	    if (p == NULL)
	    {
		fclose(fd);
		goto errret;		/* type error; errmsg already given */
	    }
	    len = STRLEN(p);
	    if (len > 0 && fwrite(p, len, 1, fd) != 1)
		err = TRUE;
	}
	if (fclose(fd) != 0)
	    err = TRUE;
	if (err)
	{
	    EMSG(_("E677: Error writing temp file"));
	    goto errret;
	}
    }

    /* Omit SHELL_COOKED when invoked with ":silent".  Avoids that the shell
     * echoes typeahead, that messes up the display. */
    if (!msg_silent)
	flags += SHELL_COOKED;

    if (retlist)
    {
	int		len;
	listitem_T	*li;
	char_u		*s = NULL;
	char_u		*start;
	char_u		*end;
	int		i;

	res = get_cmd_output(tv_get_string(&argvars[0]), infile, flags, &len);
	if (res == NULL)
	    goto errret;

	list = list_alloc();
	if (list == NULL)
	    goto errret;

	for (i = 0; i < len; ++i)
	{
	    start = res + i;
	    while (i < len && res[i] != NL)
		++i;
	    end = res + i;

	    s = alloc((unsigned)(end - start + 1));
	    if (s == NULL)
		goto errret;

	    for (p = s; start < end; ++p, ++start)
		*p = *start == NUL ? NL : *start;
	    *p = NUL;

	    li = listitem_alloc();
	    if (li == NULL)
	    {
		vim_free(s);
		goto errret;
	    }
	    li->li_tv.v_type = VAR_STRING;
	    li->li_tv.v_lock = 0;
	    li->li_tv.vval.v_string = s;
	    list_append(list, li);
	}

	rettv_list_set(rettv, list);
	list = NULL;
    }
    else
    {
	res = get_cmd_output(tv_get_string(&argvars[0]), infile, flags, NULL);
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
	rettv->vval.v_string = res;
	res = NULL;
    }

errret:
    if (infile != NULL)
    {
	mch_remove(infile);
	vim_free(infile);
    }
    if (res != NULL)
	vim_free(res);
    if (list != NULL)
	list_free(list);
}

/*
 * "system()" function
 */
    static void
f_system(typval_T *argvars, typval_T *rettv)
{
    get_cmd_output_as_rettv(argvars, rettv, FALSE);
}

/*
 * "systemlist()" function
 */
    static void
f_systemlist(typval_T *argvars, typval_T *rettv)
{
    get_cmd_output_as_rettv(argvars, rettv, TRUE);
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
 * "tabpagenr()" function
 */
    static void
f_tabpagenr(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		nr = 1;
    char_u	*arg;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	arg = tv_get_string_chk(&argvars[0]);
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
    rettv->vval.v_number = nr;
}


/*
 * Common code for tabpagewinnr() and winnr().
 */
    static int
get_winnr(tabpage_T *tp, typval_T *argvar)
{
    win_T	*twin;
    int		nr = 1;
    win_T	*wp;
    char_u	*arg;

    twin = (tp == curtab) ? curwin : tp->tp_curwin;
    if (argvar->v_type != VAR_UNKNOWN)
    {
	arg = tv_get_string_chk(argvar);
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

/*
 * "tabpagewinnr()" function
 */
    static void
f_tabpagewinnr(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		nr = 1;
    tabpage_T	*tp;

    tp = find_tabpage((int)tv_get_number(&argvars[0]));
    if (tp == NULL)
	nr = 0;
    else
	nr = get_winnr(tp, &argvars[1]);
    rettv->vval.v_number = nr;
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

/*
 * "tempname()" function
 */
    static void
f_tempname(typval_T *argvars UNUSED, typval_T *rettv)
{
    static int	x = 'A';

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_tempname(x, FALSE);

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
 * "test_alloc_fail(id, countdown, repeat)" function
 */
    static void
f_test_alloc_fail(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (argvars[0].v_type != VAR_NUMBER
	    || argvars[0].vval.v_number <= 0
	    || argvars[1].v_type != VAR_NUMBER
	    || argvars[1].vval.v_number < 0
	    || argvars[2].v_type != VAR_NUMBER)
	EMSG(_(e_invarg));
    else
    {
	alloc_fail_id = argvars[0].vval.v_number;
	if (alloc_fail_id >= aid_last)
	    EMSG(_(e_invarg));
	alloc_fail_countdown = argvars[1].vval.v_number;
	alloc_fail_repeat = argvars[2].vval.v_number;
	did_outofmem_msg = FALSE;
    }
}

/*
 * "test_autochdir()"
 */
    static void
f_test_autochdir(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
#if defined(FEAT_AUTOCHDIR)
    test_autochdir = TRUE;
#endif
}

/*
 * "test_feedinput()"
 */
    static void
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
 * "test_option_not_set({name})" function
 */
    static void
f_test_option_not_set(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u *name = (char_u *)"";

    if (argvars[0].v_type != VAR_STRING)
	EMSG(_(e_invarg));
    else
    {
	name = tv_get_string(&argvars[0]);
	if (reset_option_was_set(name) == FAIL)
	    EMSG2(_(e_invarg2), name);
    }
}

/*
 * "test_override({name}, {val})" function
 */
    static void
f_test_override(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u *name = (char_u *)"";
    int     val;
    static int save_starting = -1;

    if (argvars[0].v_type != VAR_STRING
	    || (argvars[1].v_type) != VAR_NUMBER)
	EMSG(_(e_invarg));
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
	else if (STRCMP(name, (char_u *)"ALL") == 0)
	{
	    disable_char_avail_for_testing = FALSE;
	    disable_redraw_for_testing = FALSE;
	    ignore_redraw_flag_for_testing = FALSE;
	    nfa_fail_for_testing = FALSE;
	    if (save_starting >= 0)
	    {
		starting = save_starting;
		save_starting = -1;
	    }
	}
	else
	    EMSG2(_(e_invarg2), name);
    }
}

/*
 * "test_garbagecollect_now()" function
 */
    static void
f_test_garbagecollect_now(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    /* This is dangerous, any Lists and Dicts used internally may be freed
     * while still in use. */
    garbage_collect(TRUE);
}

/*
 * "test_ignore_error()" function
 */
    static void
f_test_ignore_error(typval_T *argvars, typval_T *rettv UNUSED)
{
     ignore_error_for_testing(tv_get_string(&argvars[0]));
}

#ifdef FEAT_JOB_CHANNEL
    static void
f_test_null_channel(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_CHANNEL;
    rettv->vval.v_channel = NULL;
}
#endif

    static void
f_test_null_dict(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv_dict_set(rettv, NULL);
}

#ifdef FEAT_JOB_CHANNEL
    static void
f_test_null_job(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_JOB;
    rettv->vval.v_job = NULL;
}
#endif

    static void
f_test_null_list(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv_list_set(rettv, NULL);
}

    static void
f_test_null_partial(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_PARTIAL;
    rettv->vval.v_partial = NULL;
}

    static void
f_test_null_string(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
}

#ifdef FEAT_GUI
    static void
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
	EMSG(_(e_invarg));
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
	EMSG2(_(e_invarg2), which);
	return;
    }
    gui_drag_scrollbar(sb, value, dragging);
# ifndef USE_ON_FLY_SCROLL
    // need to loop through normal_cmd() to handle the scroll events
    exec_normal(FALSE, TRUE, FALSE);
# endif
}
#endif

    static void
f_test_settime(typval_T *argvars, typval_T *rettv UNUSED)
{
    time_for_testing = (time_t)tv_get_number(&argvars[0]);
}

#if defined(FEAT_JOB_CHANNEL) || defined(FEAT_TIMERS) || defined(PROTO)
/*
 * Get a callback from "arg".  It can be a Funcref or a function name.
 * When "arg" is zero return an empty string.
 * Return NULL for an invalid argument.
 */
    char_u *
get_callback(typval_T *arg, partial_T **pp)
{
    if (arg->v_type == VAR_PARTIAL && arg->vval.v_partial != NULL)
    {
	*pp = arg->vval.v_partial;
	++(*pp)->pt_refcount;
	return partial_name(*pp);
    }
    *pp = NULL;
    if (arg->v_type == VAR_FUNC || arg->v_type == VAR_STRING)
    {
	func_ref(arg->vval.v_string);
	return arg->vval.v_string;
    }
    if (arg->v_type == VAR_NUMBER && arg->vval.v_number == 0)
	return (char_u *)"";
    EMSG(_("E921: Invalid callback argument"));
    return NULL;
}

/*
 * Unref/free "callback" and "partial" returned by get_callback().
 */
    void
free_callback(char_u *callback, partial_T *partial)
{
    if (partial != NULL)
	partial_unref(partial);
    else if (callback != NULL)
    {
	func_unref(callback);
	vim_free(callback);
    }
}
#endif

#ifdef FEAT_TIMERS
/*
 * "timer_info([timer])" function
 */
    static void
f_timer_info(typval_T *argvars, typval_T *rettv)
{
    timer_T *timer = NULL;

    if (rettv_list_alloc(rettv) != OK)
	return;
    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	if (argvars[0].v_type != VAR_NUMBER)
	    EMSG(_(e_number_exp));
	else
	{
	    timer = find_timer((int)tv_get_number(&argvars[0]));
	    if (timer != NULL)
		add_timer_info(rettv, timer);
	}
    }
    else
	add_timer_info_all(rettv);
}

/*
 * "timer_pause(timer, paused)" function
 */
    static void
f_timer_pause(typval_T *argvars, typval_T *rettv UNUSED)
{
    timer_T	*timer = NULL;
    int		paused = (int)tv_get_number(&argvars[1]);

    if (argvars[0].v_type != VAR_NUMBER)
	EMSG(_(e_number_exp));
    else
    {
	timer = find_timer((int)tv_get_number(&argvars[0]));
	if (timer != NULL)
	    timer->tr_paused = paused;
    }
}

/*
 * "timer_start(time, callback [, options])" function
 */
    static void
f_timer_start(typval_T *argvars, typval_T *rettv)
{
    long	msec = (long)tv_get_number(&argvars[0]);
    timer_T	*timer;
    int		repeat = 0;
    char_u	*callback;
    dict_T	*dict;
    partial_T	*partial;

    rettv->vval.v_number = -1;
    if (check_secure())
	return;
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	if (argvars[2].v_type != VAR_DICT
				   || (dict = argvars[2].vval.v_dict) == NULL)
	{
	    EMSG2(_(e_invarg2), tv_get_string(&argvars[2]));
	    return;
	}
	if (dict_find(dict, (char_u *)"repeat", -1) != NULL)
	    repeat = dict_get_number(dict, (char_u *)"repeat");
    }

    callback = get_callback(&argvars[1], &partial);
    if (callback == NULL)
	return;

    timer = create_timer(msec, repeat);
    if (timer == NULL)
	free_callback(callback, partial);
    else
    {
	if (partial == NULL)
	    timer->tr_callback = vim_strsave(callback);
	else
	    /* pointer into the partial */
	    timer->tr_callback = callback;
	timer->tr_partial = partial;
	rettv->vval.v_number = (varnumber_T)timer->tr_id;
    }
}

/*
 * "timer_stop(timer)" function
 */
    static void
f_timer_stop(typval_T *argvars, typval_T *rettv UNUSED)
{
    timer_T *timer;

    if (argvars[0].v_type != VAR_NUMBER)
    {
	EMSG(_(e_number_exp));
	return;
    }
    timer = find_timer((int)tv_get_number(&argvars[0]));
    if (timer != NULL)
	stop_timer(timer);
}

/*
 * "timer_stopall()" function
 */
    static void
f_timer_stopall(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    stop_all_timers();
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

    in_str = tv_get_string(&argvars[0]);
    fromstr = tv_get_string_buf_chk(&argvars[1], buf);
    tostr = tv_get_string_buf_chk(&argvars[2], buf2);

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
    while (*in_str != NUL)
    {
#ifdef FEAT_MBYTE
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
		    if (*p == NUL)	/* tostr is shorter than fromstr */
			goto error;
		    break;
		}
		++idx;
	    }

	    if (first && cpstr == in_str)
	    {
		/* Check that fromstr and tostr have the same number of
		 * (multi-byte) characters.  Done only once when a character
		 * of in_str doesn't appear in fromstr. */
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
#endif
	{
	    /* When not using multi-byte chars we can do it faster. */
	    p = vim_strchr(fromstr, *in_str);
	    if (p != NULL)
		ga_append(&ga, tostr[p - fromstr]);
	    else
		ga_append(&ga, *in_str);
	    ++in_str;
	}
    }

    /* add a terminating NUL */
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
f_type(typval_T *argvars, typval_T *rettv)
{
    int n = -1;

    switch (argvars[0].v_type)
    {
	case VAR_NUMBER: n = VAR_TYPE_NUMBER; break;
	case VAR_STRING: n = VAR_TYPE_STRING; break;
	case VAR_PARTIAL:
	case VAR_FUNC:   n = VAR_TYPE_FUNC; break;
	case VAR_LIST:   n = VAR_TYPE_LIST; break;
	case VAR_DICT:   n = VAR_TYPE_DICT; break;
	case VAR_FLOAT:  n = VAR_TYPE_FLOAT; break;
	case VAR_SPECIAL:
	     if (argvars[0].vval.v_number == VVAL_FALSE
		     || argvars[0].vval.v_number == VVAL_TRUE)
		 n = VAR_TYPE_BOOL;
	     else
		 n = VAR_TYPE_NONE;
	     break;
	case VAR_JOB:     n = VAR_TYPE_JOB; break;
	case VAR_CHANNEL: n = VAR_TYPE_CHANNEL; break;
	case VAR_UNKNOWN:
	     internal_error("f_type(UNKNOWN)");
	     n = -1;
	     break;
    }
    rettv->vval.v_number = n;
}

/*
 * "undofile(name)" function
 */
    static void
f_undofile(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
#ifdef FEAT_PERSISTENT_UNDO
    {
	char_u *fname = tv_get_string(&argvars[0]);

	if (*fname == NUL)
	{
	    /* If there is no file name there will be no undo file. */
	    rettv->vval.v_string = NULL;
	}
	else
	{
	    char_u *ffname = FullName_save(fname, FALSE);

	    if (ffname != NULL)
		rettv->vval.v_string = u_get_undo_file_name(ffname, FALSE);
	    vim_free(ffname);
	}
    }
#else
    rettv->vval.v_string = NULL;
#endif
}

/*
 * "undotree()" function
 */
    static void
f_undotree(typval_T *argvars UNUSED, typval_T *rettv)
{
    if (rettv_dict_alloc(rettv) == OK)
    {
	dict_T *dict = rettv->vval.v_dict;
	list_T *list;

	dict_add_number(dict, "synced", (long)curbuf->b_u_synced);
	dict_add_number(dict, "seq_last", curbuf->b_u_seq_last);
	dict_add_number(dict, "save_last", (long)curbuf->b_u_save_nr_last);
	dict_add_number(dict, "seq_cur", curbuf->b_u_seq_cur);
	dict_add_number(dict, "time_cur", (long)curbuf->b_u_time_cur);
	dict_add_number(dict, "save_cur", (long)curbuf->b_u_save_nr_cur);

	list = list_alloc();
	if (list != NULL)
	{
	    u_eval_tree(curbuf->b_u_oldhead, list);
	    dict_add_list(dict, "entries", list);
	}
    }
}

/*
 * "values(dict)" function
 */
    static void
f_values(typval_T *argvars, typval_T *rettv)
{
    dict_list(argvars, rettv, 1);
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
    static void
f_visualmode(typval_T *argvars, typval_T *rettv)
{
    char_u	str[2];

    rettv->v_type = VAR_STRING;
    str[0] = curbuf->b_visual_mode_eval;
    str[1] = NUL;
    rettv->vval.v_string = vim_strsave(str);

    /* A non-zero number or non-empty string argument: reset mode. */
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
 * "winbufnr(nr)" function
 */
    static void
f_winbufnr(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp;

    wp = find_win_by_nr_or_id(&argvars[0]);
    if (wp == NULL)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = wp->w_buffer->b_fnum;
}

/*
 * "wincol()" function
 */
    static void
f_wincol(typval_T *argvars UNUSED, typval_T *rettv)
{
    validate_cursor();
    rettv->vval.v_number = curwin->w_wcol + 1;
}

/*
 * "winheight(nr)" function
 */
    static void
f_winheight(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp;

    wp = find_win_by_nr_or_id(&argvars[0]);
    if (wp == NULL)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = wp->w_height;
}

/*
 * "winlayout()" function
 */
    static void
f_winlayout(typval_T *argvars, typval_T *rettv)
{
    tabpage_T	*tp;

    if (rettv_list_alloc(rettv) != OK)
	return;

    if (argvars[0].v_type == VAR_UNKNOWN)
	tp = curtab;
    else
    {
	tp = find_tabpage((int)tv_get_number(&argvars[0]));
	if (tp == NULL)
	    return;
    }

    get_framelayout(tp->tp_topframe, rettv->vval.v_list, TRUE);
}

/*
 * "winline()" function
 */
    static void
f_winline(typval_T *argvars UNUSED, typval_T *rettv)
{
    validate_cursor();
    rettv->vval.v_number = curwin->w_wrow + 1;
}

/*
 * "winnr()" function
 */
    static void
f_winnr(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		nr = 1;

    nr = get_winnr(curtab, &argvars[0]);
    rettv->vval.v_number = nr;
}

/*
 * "winrestcmd()" function
 */
    static void
f_winrestcmd(typval_T *argvars UNUSED, typval_T *rettv)
{
    win_T	*wp;
    int		winnr = 1;
    garray_T	ga;
    char_u	buf[50];

    ga_init2(&ga, (int)sizeof(char), 70);
    FOR_ALL_WINDOWS(wp)
    {
	sprintf((char *)buf, "%dresize %d|", winnr, wp->w_height);
	ga_concat(&ga, buf);
	sprintf((char *)buf, "vert %dresize %d|", winnr, wp->w_width);
	ga_concat(&ga, buf);
	++winnr;
    }
    ga_append(&ga, NUL);

    rettv->vval.v_string = ga.ga_data;
    rettv->v_type = VAR_STRING;
}

/*
 * "winrestview()" function
 */
    static void
f_winrestview(typval_T *argvars, typval_T *rettv UNUSED)
{
    dict_T	*dict;

    if (argvars[0].v_type != VAR_DICT
	    || (dict = argvars[0].vval.v_dict) == NULL)
	EMSG(_(e_invarg));
    else
    {
	if (dict_find(dict, (char_u *)"lnum", -1) != NULL)
	    curwin->w_cursor.lnum = (linenr_T)dict_get_number(dict, (char_u *)"lnum");
	if (dict_find(dict, (char_u *)"col", -1) != NULL)
	    curwin->w_cursor.col = (colnr_T)dict_get_number(dict, (char_u *)"col");
#ifdef FEAT_VIRTUALEDIT
	if (dict_find(dict, (char_u *)"coladd", -1) != NULL)
	    curwin->w_cursor.coladd = (colnr_T)dict_get_number(dict, (char_u *)"coladd");
#endif
	if (dict_find(dict, (char_u *)"curswant", -1) != NULL)
	{
	    curwin->w_curswant = (colnr_T)dict_get_number(dict, (char_u *)"curswant");
	    curwin->w_set_curswant = FALSE;
	}

	if (dict_find(dict, (char_u *)"topline", -1) != NULL)
	    set_topline(curwin, (linenr_T)dict_get_number(dict, (char_u *)"topline"));
#ifdef FEAT_DIFF
	if (dict_find(dict, (char_u *)"topfill", -1) != NULL)
	    curwin->w_topfill = (int)dict_get_number(dict, (char_u *)"topfill");
#endif
	if (dict_find(dict, (char_u *)"leftcol", -1) != NULL)
	    curwin->w_leftcol = (colnr_T)dict_get_number(dict, (char_u *)"leftcol");
	if (dict_find(dict, (char_u *)"skipcol", -1) != NULL)
	    curwin->w_skipcol = (colnr_T)dict_get_number(dict, (char_u *)"skipcol");

	check_cursor();
	win_new_height(curwin, curwin->w_height);
	win_new_width(curwin, curwin->w_width);
	changed_window_setting();

	if (curwin->w_topline <= 0)
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
    static void
f_winsaveview(typval_T *argvars UNUSED, typval_T *rettv)
{
    dict_T	*dict;

    if (rettv_dict_alloc(rettv) == FAIL)
	return;
    dict = rettv->vval.v_dict;

    dict_add_number(dict, "lnum", (long)curwin->w_cursor.lnum);
    dict_add_number(dict, "col", (long)curwin->w_cursor.col);
#ifdef FEAT_VIRTUALEDIT
    dict_add_number(dict, "coladd", (long)curwin->w_cursor.coladd);
#endif
    update_curswant();
    dict_add_number(dict, "curswant", (long)curwin->w_curswant);

    dict_add_number(dict, "topline", (long)curwin->w_topline);
#ifdef FEAT_DIFF
    dict_add_number(dict, "topfill", (long)curwin->w_topfill);
#endif
    dict_add_number(dict, "leftcol", (long)curwin->w_leftcol);
    dict_add_number(dict, "skipcol", (long)curwin->w_skipcol);
}

/*
 * "winwidth(nr)" function
 */
    static void
f_winwidth(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp;

    wp = find_win_by_nr_or_id(&argvars[0]);
    if (wp == NULL)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = wp->w_width;
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
 * "writefile()" function
 */
    static void
f_writefile(typval_T *argvars, typval_T *rettv)
{
    int		binary = FALSE;
    int		append = FALSE;
#ifdef HAVE_FSYNC
    int		do_fsync = p_fs;
#endif
    char_u	*fname;
    FILE	*fd;
    int		ret = 0;
    listitem_T	*li;
    list_T	*list;

    rettv->vval.v_number = -1;
    if (check_restricted() || check_secure())
	return;

    if (argvars[0].v_type != VAR_LIST)
    {
	EMSG2(_(e_listarg), "writefile()");
	return;
    }
    list = argvars[0].vval.v_list;
    if (list == NULL)
	return;
    for (li = list->lv_first; li != NULL; li = li->li_next)
	if (tv_get_string_chk(&li->li_tv) == NULL)
	    return;

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	char_u *arg2 = tv_get_string_chk(&argvars[2]);

	if (arg2 == NULL)
	    return;
	if (vim_strchr(arg2, 'b') != NULL)
	    binary = TRUE;
	if (vim_strchr(arg2, 'a') != NULL)
	    append = TRUE;
#ifdef HAVE_FSYNC
	if (vim_strchr(arg2, 's') != NULL)
	    do_fsync = TRUE;
	else if (vim_strchr(arg2, 'S') != NULL)
	    do_fsync = FALSE;
#endif
    }

    fname = tv_get_string_chk(&argvars[1]);
    if (fname == NULL)
	return;

    /* Always open the file in binary mode, library functions have a mind of
     * their own about CR-LF conversion. */
    if (*fname == NUL || (fd = mch_fopen((char *)fname,
				      append ? APPENDBIN : WRITEBIN)) == NULL)
    {
	EMSG2(_(e_notcreate), *fname == NUL ? (char_u *)_("<empty>") : fname);
	ret = -1;
    }
    else
    {
	if (write_list(fd, list, binary) == FAIL)
	    ret = -1;
#ifdef HAVE_FSYNC
	else if (do_fsync)
	    /* Ignore the error, the user wouldn't know what to do about it.
	     * May happen for a device. */
	    vim_ignored = fsync(fileno(fd));
#endif
	fclose(fd);
    }

    rettv->vval.v_number = ret;
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

#endif /* FEAT_EVAL */
