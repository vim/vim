/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
/*
 * if_perl.xs: Main code for Perl interface support.
 *		Mostly written by Sven Verdoolaege.
 */

#define _memory_h	/* avoid memset redeclaration */
#define IN_PERL_FILE	/* don't include if_perl.pro from proto.h */

#include "vim.h"


/*
 * Work around clashes between Perl and Vim namespace.	proto.h doesn't
 * include if_perl.pro and perlsfio.pro when IN_PERL_FILE is defined, because
 * we need the CV typedef.  proto.h can't be moved to after including
 * if_perl.h, because we get all sorts of name clashes then.
 */
#ifndef PROTO
#ifndef __MINGW32__
# include "proto/if_perl.pro"
# include "proto/if_perlsfio.pro"
#endif
#endif

/* Perl compatibility stuff. This should ensure compatibility with older
 * versions of Perl.
 */

#ifndef PERL_VERSION
#    include <patchlevel.h>
#    define PERL_REVISION   5
#    define PERL_VERSION    PATCHLEVEL
#    define PERL_SUBVERSION SUBVERSION
#endif

/*
 * Quoting Jan Dubois of Active State:
 *    ActivePerl build 822 still identifies itself as 5.8.8 but already
 *    contains many of the changes from the upcoming Perl 5.8.9 release.
 *
 * The changes include addition of two symbols (Perl_sv_2iv_flags,
 * Perl_newXS_flags) not present in earlier releases.
 *
 * Jan Dubois suggested the following guarding scheme.
 *
 * Active State defined ACTIVEPERL_VERSION as a string in versions before
 * 5.8.8; and so the comparison to 822 below needs to be guarded.
 */
#if (PERL_REVISION == 5) && (PERL_VERSION == 8) && (PERL_SUBVERSION >= 8)
# if (ACTIVEPERL_VERSION >= 822) || (PERL_SUBVERSION >= 9)
#  define PERL589_OR_LATER
# endif
#endif
#if (PERL_REVISION == 5) && (PERL_VERSION >= 9)
# define PERL589_OR_LATER
#endif

#if (PERL_REVISION == 5) && ((PERL_VERSION > 10) || \
    (PERL_VERSION == 10) && (PERL_SUBVERSION >= 1))
# define PERL5101_OR_LATER
#endif

#ifndef pTHX
#    define pTHX void
#    define pTHX_
#endif

#ifndef EXTERN_C
# define EXTERN_C
#endif

/* Compatibility hacks over */

static PerlInterpreter *perl_interp = NULL;
static void xs_init __ARGS((pTHX));
static void VIM_init __ARGS((void));
EXTERN_C void boot_DynaLoader __ARGS((pTHX_ CV*));

/*
 * For dynamic linked perl.
 */
#if defined(DYNAMIC_PERL) || defined(PROTO)

#ifndef DYNAMIC_PERL /* just generating prototypes */
#ifdef WIN3264
typedef int HANDLE;
#endif
typedef int XSINIT_t;
typedef int XSUBADDR_t;
typedef int perl_key;
#endif

#ifndef WIN3264
#include <dlfcn.h>
#define HANDLE void*
#define PERL_PROC void*
#define load_dll(n) dlopen((n), RTLD_LAZY|RTLD_GLOBAL)
#define symbol_from_dll dlsym
#define close_dll dlclose
#else
#define PERL_PROC FARPROC
#define load_dll LoadLibrary
#define symbol_from_dll GetProcAddress
#define close_dll FreeLibrary
#endif
/*
 * Wrapper defines
 */
# define perl_alloc dll_perl_alloc
# define perl_construct dll_perl_construct
# define perl_parse dll_perl_parse
# define perl_run dll_perl_run
# define perl_destruct dll_perl_destruct
# define perl_free dll_perl_free
# define Perl_get_context dll_Perl_get_context
# define Perl_croak dll_Perl_croak
# ifdef PERL5101_OR_LATER
#  define Perl_croak_xs_usage dll_Perl_croak_xs_usage
# endif
# ifndef PROTO
#  define Perl_croak_nocontext dll_Perl_croak_nocontext
#  define Perl_call_argv dll_Perl_call_argv
#  define Perl_call_pv dll_Perl_call_pv
#  define Perl_eval_sv dll_Perl_eval_sv
#  define Perl_get_sv dll_Perl_get_sv
#  define Perl_eval_pv dll_Perl_eval_pv
#  define Perl_call_method dll_Perl_call_method
# endif
# define Perl_dowantarray dll_Perl_dowantarray
# define Perl_free_tmps dll_Perl_free_tmps
# define Perl_gv_stashpv dll_Perl_gv_stashpv
# define Perl_markstack_grow dll_Perl_markstack_grow
# define Perl_mg_find dll_Perl_mg_find
# define Perl_newXS dll_Perl_newXS
# define Perl_newSV dll_Perl_newSV
# define Perl_newSViv dll_Perl_newSViv
# define Perl_newSVpv dll_Perl_newSVpv
# define Perl_pop_scope dll_Perl_pop_scope
# define Perl_push_scope dll_Perl_push_scope
# define Perl_save_int dll_Perl_save_int
# define Perl_stack_grow dll_Perl_stack_grow
# define Perl_set_context dll_Perl_set_context
# define Perl_sv_2bool dll_Perl_sv_2bool
# define Perl_sv_2iv dll_Perl_sv_2iv
# define Perl_sv_2mortal dll_Perl_sv_2mortal
# if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
#  define Perl_sv_2pv_flags dll_Perl_sv_2pv_flags
#  define Perl_sv_2pv_nolen dll_Perl_sv_2pv_nolen
# else
#  define Perl_sv_2pv dll_Perl_sv_2pv
# endif
# define Perl_sv_bless dll_Perl_sv_bless
# if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
#  define Perl_sv_catpvn_flags dll_Perl_sv_catpvn_flags
# else
#  define Perl_sv_catpvn dll_Perl_sv_catpvn
# endif
#ifdef PERL589_OR_LATER
#  define Perl_sv_2iv_flags dll_Perl_sv_2iv_flags
#  define Perl_newXS_flags dll_Perl_newXS_flags
#endif
# define Perl_sv_free dll_Perl_sv_free
# if (PERL_REVISION == 5) && (PERL_VERSION >= 10)
#  define Perl_sv_free2 dll_Perl_sv_free2
# endif
# define Perl_sv_isa dll_Perl_sv_isa
# define Perl_sv_magic dll_Perl_sv_magic
# define Perl_sv_setiv dll_Perl_sv_setiv
# define Perl_sv_setpv dll_Perl_sv_setpv
# define Perl_sv_setpvn dll_Perl_sv_setpvn
# if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
#  define Perl_sv_setsv_flags dll_Perl_sv_setsv_flags
# else
#  define Perl_sv_setsv dll_Perl_sv_setsv
# endif
# define Perl_sv_upgrade dll_Perl_sv_upgrade
# define Perl_Tstack_sp_ptr dll_Perl_Tstack_sp_ptr
# define Perl_Top_ptr dll_Perl_Top_ptr
# define Perl_Tstack_base_ptr dll_Perl_Tstack_base_ptr
# define Perl_Tstack_max_ptr dll_Perl_Tstack_max_ptr
# define Perl_Ttmps_ix_ptr dll_Perl_Ttmps_ix_ptr
# define Perl_Ttmps_floor_ptr dll_Perl_Ttmps_floor_ptr
# define Perl_Tmarkstack_ptr_ptr dll_Perl_Tmarkstack_ptr_ptr
# define Perl_Tmarkstack_max_ptr dll_Perl_Tmarkstack_max_ptr
# define Perl_TSv_ptr dll_Perl_TSv_ptr
# define Perl_TXpv_ptr dll_Perl_TXpv_ptr
# define Perl_Tna_ptr dll_Perl_Tna_ptr
# define Perl_Idefgv_ptr dll_Perl_Idefgv_ptr
# define Perl_Ierrgv_ptr dll_Perl_Ierrgv_ptr
# define Perl_Isv_yes_ptr dll_Perl_Isv_yes_ptr
# define boot_DynaLoader dll_boot_DynaLoader
# define Perl_Gthr_key_ptr dll_Perl_Gthr_key_ptr

# define Perl_sys_init dll_Perl_sys_init
# define Perl_sys_term dll_Perl_sys_term
# define Perl_ISv_ptr dll_Perl_ISv_ptr
# define Perl_Istack_max_ptr dll_Perl_Istack_max_ptr
# define Perl_Istack_base_ptr dll_Perl_Istack_base_ptr
# define Perl_Itmps_ix_ptr dll_Perl_Itmps_ix_ptr
# define Perl_Itmps_floor_ptr dll_Perl_Itmps_floor_ptr
# define Perl_IXpv_ptr dll_Perl_IXpv_ptr
# define Perl_Ina_ptr dll_Perl_Ina_ptr
# define Perl_Imarkstack_ptr_ptr dll_Perl_Imarkstack_ptr_ptr
# define Perl_Imarkstack_max_ptr dll_Perl_Imarkstack_max_ptr
# define Perl_Istack_sp_ptr dll_Perl_Istack_sp_ptr
# define Perl_Iop_ptr dll_Perl_Iop_ptr
# define Perl_call_list dll_Perl_call_list
# define Perl_Iscopestack_ix_ptr dll_Perl_Iscopestack_ix_ptr
# define Perl_Iunitcheckav_ptr dll_Perl_Iunitcheckav_ptr

/*
 * Declare HANDLE for perl.dll and function pointers.
 */
static HANDLE hPerlLib = NULL;

static PerlInterpreter* (*perl_alloc)();
static void (*perl_construct)(PerlInterpreter*);
static void (*perl_destruct)(PerlInterpreter*);
static void (*perl_free)(PerlInterpreter*);
static int (*perl_run)(PerlInterpreter*);
static int (*perl_parse)(PerlInterpreter*, XSINIT_t, int, char**, char**);
static void* (*Perl_get_context)(void);
static void (*Perl_croak)(pTHX_ const char*, ...);
#ifdef PERL5101_OR_LATER
static void (*Perl_croak_xs_usage)(pTHX_ const CV *const, const char *const params);
#endif
static void (*Perl_croak_nocontext)(const char*, ...);
static I32 (*Perl_dowantarray)(pTHX);
static void (*Perl_free_tmps)(pTHX);
static HV* (*Perl_gv_stashpv)(pTHX_ const char*, I32);
static void (*Perl_markstack_grow)(pTHX);
static MAGIC* (*Perl_mg_find)(pTHX_ SV*, int);
static CV* (*Perl_newXS)(pTHX_ char*, XSUBADDR_t, char*);
static SV* (*Perl_newSV)(pTHX_ STRLEN);
static SV* (*Perl_newSViv)(pTHX_ IV);
static SV* (*Perl_newSVpv)(pTHX_ const char*, STRLEN);
static I32 (*Perl_call_argv)(pTHX_ const char*, I32, char**);
static I32 (*Perl_call_pv)(pTHX_ const char*, I32);
static I32 (*Perl_eval_sv)(pTHX_ SV*, I32);
static SV* (*Perl_get_sv)(pTHX_ const char*, I32);
static SV* (*Perl_eval_pv)(pTHX_ const char*, I32);
static SV* (*Perl_call_method)(pTHX_ const char*, I32);
static void (*Perl_pop_scope)(pTHX);
static void (*Perl_push_scope)(pTHX);
static void (*Perl_save_int)(pTHX_ int*);
static SV** (*Perl_stack_grow)(pTHX_ SV**, SV**p, int);
static SV** (*Perl_set_context)(void*);
static bool (*Perl_sv_2bool)(pTHX_ SV*);
static IV (*Perl_sv_2iv)(pTHX_ SV*);
static SV* (*Perl_sv_2mortal)(pTHX_ SV*);
#if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
static char* (*Perl_sv_2pv_flags)(pTHX_ SV*, STRLEN*, I32);
static char* (*Perl_sv_2pv_nolen)(pTHX_ SV*);
#else
static char* (*Perl_sv_2pv)(pTHX_ SV*, STRLEN*);
#endif
static SV* (*Perl_sv_bless)(pTHX_ SV*, HV*);
#if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
static void (*Perl_sv_catpvn_flags)(pTHX_ SV* , const char*, STRLEN, I32);
#else
static void (*Perl_sv_catpvn)(pTHX_ SV*, const char*, STRLEN);
#endif
#ifdef PERL589_OR_LATER
static IV (*Perl_sv_2iv_flags)(pTHX_ SV* sv, I32 flags);
static CV * (*Perl_newXS_flags)(pTHX_ const char *name, XSUBADDR_t subaddr, const char *const filename, const char *const proto, U32 flags);
#endif
static void (*Perl_sv_free)(pTHX_ SV*);
static int (*Perl_sv_isa)(pTHX_ SV*, const char*);
static void (*Perl_sv_magic)(pTHX_ SV*, SV*, int, const char*, I32);
static void (*Perl_sv_setiv)(pTHX_ SV*, IV);
static void (*Perl_sv_setpv)(pTHX_ SV*, const char*);
static void (*Perl_sv_setpvn)(pTHX_ SV*, const char*, STRLEN);
#if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
static void (*Perl_sv_setsv_flags)(pTHX_ SV*, SV*, I32);
#else
static void (*Perl_sv_setsv)(pTHX_ SV*, SV*);
#endif
static bool (*Perl_sv_upgrade)(pTHX_ SV*, U32);
#if (PERL_REVISION == 5) && (PERL_VERSION < 10)
static SV*** (*Perl_Tstack_sp_ptr)(register PerlInterpreter*);
static OP** (*Perl_Top_ptr)(register PerlInterpreter*);
static SV*** (*Perl_Tstack_base_ptr)(register PerlInterpreter*);
static SV*** (*Perl_Tstack_max_ptr)(register PerlInterpreter*);
static I32* (*Perl_Ttmps_ix_ptr)(register PerlInterpreter*);
static I32* (*Perl_Ttmps_floor_ptr)(register PerlInterpreter*);
static I32** (*Perl_Tmarkstack_ptr_ptr)(register PerlInterpreter*);
static I32** (*Perl_Tmarkstack_max_ptr)(register PerlInterpreter*);
static SV** (*Perl_TSv_ptr)(register PerlInterpreter*);
static XPV** (*Perl_TXpv_ptr)(register PerlInterpreter*);
static STRLEN* (*Perl_Tna_ptr)(register PerlInterpreter*);
#else
static void (*Perl_sv_free2)(pTHX_ SV*);
static void (*Perl_sys_init)(int* argc, char*** argv);
static void (*Perl_sys_term)(void);
static SV** (*Perl_ISv_ptr)(register PerlInterpreter*);
static SV*** (*Perl_Istack_max_ptr)(register PerlInterpreter*);
static SV*** (*Perl_Istack_base_ptr)(register PerlInterpreter*);
static XPV** (*Perl_IXpv_ptr)(register PerlInterpreter*);
static I32* (*Perl_Itmps_ix_ptr)(register PerlInterpreter*);
static I32* (*Perl_Itmps_floor_ptr)(register PerlInterpreter*);
static STRLEN* (*Perl_Ina_ptr)(register PerlInterpreter*);
static I32** (*Perl_Imarkstack_ptr_ptr)(register PerlInterpreter*);
static I32** (*Perl_Imarkstack_max_ptr)(register PerlInterpreter*);
static SV*** (*Perl_Istack_sp_ptr)(register PerlInterpreter*);
static OP** (*Perl_Iop_ptr)(register PerlInterpreter*);
static void (*Perl_call_list)(pTHX_ I32, AV*);
static I32* (*Perl_Iscopestack_ix_ptr)(register PerlInterpreter*);
static AV** (*Perl_Iunitcheckav_ptr)(register PerlInterpreter*);
#endif

static GV** (*Perl_Idefgv_ptr)(register PerlInterpreter*);
static GV** (*Perl_Ierrgv_ptr)(register PerlInterpreter*);
static SV* (*Perl_Isv_yes_ptr)(register PerlInterpreter*);
static void (*boot_DynaLoader)_((pTHX_ CV*));
static perl_key* (*Perl_Gthr_key_ptr)_((pTHX));

/*
 * Table of name to function pointer of perl.
 */
static struct {
    char* name;
    PERL_PROC* ptr;
} perl_funcname_table[] = {
    {"perl_alloc", (PERL_PROC*)&perl_alloc},
    {"perl_construct", (PERL_PROC*)&perl_construct},
    {"perl_destruct", (PERL_PROC*)&perl_destruct},
    {"perl_free", (PERL_PROC*)&perl_free},
    {"perl_run", (PERL_PROC*)&perl_run},
    {"perl_parse", (PERL_PROC*)&perl_parse},
    {"Perl_get_context", (PERL_PROC*)&Perl_get_context},
    {"Perl_croak", (PERL_PROC*)&Perl_croak},
#ifdef PERL5101_OR_LATER
    {"Perl_croak_xs_usage", (PERL_PROC*)&Perl_croak_xs_usage},
#endif
    {"Perl_croak_nocontext", (PERL_PROC*)&Perl_croak_nocontext},
    {"Perl_dowantarray", (PERL_PROC*)&Perl_dowantarray},
    {"Perl_free_tmps", (PERL_PROC*)&Perl_free_tmps},
    {"Perl_gv_stashpv", (PERL_PROC*)&Perl_gv_stashpv},
    {"Perl_markstack_grow", (PERL_PROC*)&Perl_markstack_grow},
    {"Perl_mg_find", (PERL_PROC*)&Perl_mg_find},
    {"Perl_newXS", (PERL_PROC*)&Perl_newXS},
    {"Perl_newSV", (PERL_PROC*)&Perl_newSV},
    {"Perl_newSViv", (PERL_PROC*)&Perl_newSViv},
    {"Perl_newSVpv", (PERL_PROC*)&Perl_newSVpv},
    {"Perl_call_argv", (PERL_PROC*)&Perl_call_argv},
    {"Perl_call_pv", (PERL_PROC*)&Perl_call_pv},
    {"Perl_eval_sv", (PERL_PROC*)&Perl_eval_sv},
    {"Perl_get_sv", (PERL_PROC*)&Perl_get_sv},
    {"Perl_eval_pv", (PERL_PROC*)&Perl_eval_pv},
    {"Perl_call_method", (PERL_PROC*)&Perl_call_method},
    {"Perl_pop_scope", (PERL_PROC*)&Perl_pop_scope},
    {"Perl_push_scope", (PERL_PROC*)&Perl_push_scope},
    {"Perl_save_int", (PERL_PROC*)&Perl_save_int},
    {"Perl_stack_grow", (PERL_PROC*)&Perl_stack_grow},
    {"Perl_set_context", (PERL_PROC*)&Perl_set_context},
    {"Perl_sv_2bool", (PERL_PROC*)&Perl_sv_2bool},
    {"Perl_sv_2iv", (PERL_PROC*)&Perl_sv_2iv},
    {"Perl_sv_2mortal", (PERL_PROC*)&Perl_sv_2mortal},
#if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
    {"Perl_sv_2pv_flags", (PERL_PROC*)&Perl_sv_2pv_flags},
    {"Perl_sv_2pv_nolen", (PERL_PROC*)&Perl_sv_2pv_nolen},
#else
    {"Perl_sv_2pv", (PERL_PROC*)&Perl_sv_2pv},
#endif
#ifdef PERL589_OR_LATER
    {"Perl_sv_2iv_flags", (PERL_PROC*)&Perl_sv_2iv_flags},
    {"Perl_newXS_flags", (PERL_PROC*)&Perl_newXS_flags},
#endif
    {"Perl_sv_bless", (PERL_PROC*)&Perl_sv_bless},
#if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
    {"Perl_sv_catpvn_flags", (PERL_PROC*)&Perl_sv_catpvn_flags},
#else
    {"Perl_sv_catpvn", (PERL_PROC*)&Perl_sv_catpvn},
#endif
    {"Perl_sv_free", (PERL_PROC*)&Perl_sv_free},
    {"Perl_sv_isa", (PERL_PROC*)&Perl_sv_isa},
    {"Perl_sv_magic", (PERL_PROC*)&Perl_sv_magic},
    {"Perl_sv_setiv", (PERL_PROC*)&Perl_sv_setiv},
    {"Perl_sv_setpv", (PERL_PROC*)&Perl_sv_setpv},
    {"Perl_sv_setpvn", (PERL_PROC*)&Perl_sv_setpvn},
#if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
    {"Perl_sv_setsv_flags", (PERL_PROC*)&Perl_sv_setsv_flags},
#else
    {"Perl_sv_setsv", (PERL_PROC*)&Perl_sv_setsv},
#endif
    {"Perl_sv_upgrade", (PERL_PROC*)&Perl_sv_upgrade},
#if (PERL_REVISION == 5) && (PERL_VERSION < 10)
    {"Perl_Tstack_sp_ptr", (PERL_PROC*)&Perl_Tstack_sp_ptr},
    {"Perl_Top_ptr", (PERL_PROC*)&Perl_Top_ptr},
    {"Perl_Tstack_base_ptr", (PERL_PROC*)&Perl_Tstack_base_ptr},
    {"Perl_Tstack_max_ptr", (PERL_PROC*)&Perl_Tstack_max_ptr},
    {"Perl_Ttmps_ix_ptr", (PERL_PROC*)&Perl_Ttmps_ix_ptr},
    {"Perl_Ttmps_floor_ptr", (PERL_PROC*)&Perl_Ttmps_floor_ptr},
    {"Perl_Tmarkstack_ptr_ptr", (PERL_PROC*)&Perl_Tmarkstack_ptr_ptr},
    {"Perl_Tmarkstack_max_ptr", (PERL_PROC*)&Perl_Tmarkstack_max_ptr},
    {"Perl_TSv_ptr", (PERL_PROC*)&Perl_TSv_ptr},
    {"Perl_TXpv_ptr", (PERL_PROC*)&Perl_TXpv_ptr},
    {"Perl_Tna_ptr", (PERL_PROC*)&Perl_Tna_ptr},
#else
    {"Perl_sv_free2", (PERL_PROC*)&Perl_sv_free2},
    {"Perl_sys_init", (PERL_PROC*)&Perl_sys_init},
    {"Perl_sys_term", (PERL_PROC*)&Perl_sys_term},
    {"Perl_ISv_ptr", (PERL_PROC*)&Perl_ISv_ptr},
    {"Perl_Istack_max_ptr", (PERL_PROC*)&Perl_Istack_max_ptr},
    {"Perl_Istack_base_ptr", (PERL_PROC*)&Perl_Istack_base_ptr},
    {"Perl_IXpv_ptr", (PERL_PROC*)&Perl_IXpv_ptr},
    {"Perl_Itmps_ix_ptr", (PERL_PROC*)&Perl_Itmps_ix_ptr},
    {"Perl_Itmps_floor_ptr", (PERL_PROC*)&Perl_Itmps_floor_ptr},
    {"Perl_Ina_ptr", (PERL_PROC*)&Perl_Ina_ptr},
    {"Perl_Imarkstack_ptr_ptr", (PERL_PROC*)&Perl_Imarkstack_ptr_ptr},
    {"Perl_Imarkstack_max_ptr", (PERL_PROC*)&Perl_Imarkstack_max_ptr},
    {"Perl_Istack_sp_ptr", (PERL_PROC*)&Perl_Istack_sp_ptr},
    {"Perl_Iop_ptr", (PERL_PROC*)&Perl_Iop_ptr},
    {"Perl_call_list", (PERL_PROC*)&Perl_call_list},
    {"Perl_Iscopestack_ix_ptr", (PERL_PROC*)&Perl_Iscopestack_ix_ptr},
    {"Perl_Iunitcheckav_ptr", (PERL_PROC*)&Perl_Iunitcheckav_ptr},
#endif
    {"Perl_Idefgv_ptr", (PERL_PROC*)&Perl_Idefgv_ptr},
    {"Perl_Ierrgv_ptr", (PERL_PROC*)&Perl_Ierrgv_ptr},
    {"Perl_Isv_yes_ptr", (PERL_PROC*)&Perl_Isv_yes_ptr},
    {"boot_DynaLoader", (PERL_PROC*)&boot_DynaLoader},
    {"Perl_Gthr_key_ptr", (PERL_PROC*)&Perl_Gthr_key_ptr},
    {"", NULL},
};

/*
 * Make all runtime-links of perl.
 *
 * 1. Get module handle using LoadLibraryEx.
 * 2. Get pointer to perl function by GetProcAddress.
 * 3. Repeat 2, until get all functions will be used.
 *
 * Parameter 'libname' provides name of DLL.
 * Return OK or FAIL.
 */
    static int
perl_runtime_link_init(char *libname, int verbose)
{
    int i;

    if (hPerlLib != NULL)
	return OK;
    if ((hPerlLib = load_dll(libname)) == NULL)
    {
	if (verbose)
	    EMSG2(_("E370: Could not load library %s"), libname);
	return FAIL;
    }
    for (i = 0; perl_funcname_table[i].ptr; ++i)
    {
	if (!(*perl_funcname_table[i].ptr = symbol_from_dll(hPerlLib,
			perl_funcname_table[i].name)))
	{
	    close_dll(hPerlLib);
	    hPerlLib = NULL;
	    if (verbose)
		EMSG2(_(e_loadfunc), perl_funcname_table[i].name);
	    return FAIL;
	}
    }
    return OK;
}

/*
 * If runtime-link-perl(DLL) was loaded successfully, return TRUE.
 * There were no DLL loaded, return FALSE.
 */
    int
perl_enabled(verbose)
    int		verbose;
{
    return perl_runtime_link_init(DYNAMIC_PERL_DLL, verbose) == OK;
}
#endif /* DYNAMIC_PERL */

/*
 * perl_init(): initialize perl interpreter
 * We have to call perl_parse to initialize some structures,
 * there's nothing to actually parse.
 */
    static void
perl_init()
{
    char *bootargs[] = { "VI", NULL };
    int argc = 3;
    static char *argv[] = { "", "-e", "" };

#if (PERL_REVISION == 5) && (PERL_VERSION >= 10)
    Perl_sys_init(&argc, (char***)&argv);
#endif
    perl_interp = perl_alloc();
    perl_construct(perl_interp);
    perl_parse(perl_interp, xs_init, argc, argv, 0);
    perl_call_argv("VIM::bootstrap", (long)G_DISCARD, bootargs);
    VIM_init();
#ifdef USE_SFIO
    sfdisc(PerlIO_stdout(), sfdcnewvim());
    sfdisc(PerlIO_stderr(), sfdcnewvim());
    sfsetbuf(PerlIO_stdout(), NULL, 0);
    sfsetbuf(PerlIO_stderr(), NULL, 0);
#endif
}

/*
 * perl_end(): clean up after ourselves
 */
    void
perl_end()
{
    if (perl_interp)
    {
	perl_run(perl_interp);
	perl_destruct(perl_interp);
	perl_free(perl_interp);
	perl_interp = NULL;
#if (PERL_REVISION == 5) && (PERL_VERSION >= 10)
        Perl_sys_term();
#endif
    }
#ifdef DYNAMIC_PERL
    if (hPerlLib)
    {
	close_dll(hPerlLib);
	hPerlLib = NULL;
    }
#endif
}

/*
 * msg_split(): send a message to the message handling routines
 * split at '\n' first though.
 */
    void
msg_split(s, attr)
    char_u	*s;
    int		attr;	/* highlighting attributes */
{
    char *next;
    char *token = (char *)s;

    while ((next = strchr(token, '\n')) && !got_int)
    {
	*next++ = '\0';			/* replace \n with \0 */
	msg_attr((char_u *)token, attr);
	token = next;
    }
    if (*token && !got_int)
	msg_attr((char_u *)token, attr);
}

#ifndef FEAT_EVAL
/*
 * This stub is needed because an "#ifdef FEAT_EVAL" around Eval() doesn't
 * work properly.
 */
    char_u *
eval_to_string(arg, nextcmd, dolist)
    char_u	*arg;
    char_u	**nextcmd;
    int		dolist;
{
    return NULL;
}
#endif

/*
 * Create a new reference to an SV pointing to the SCR structure
 * The b_perl_private/w_perl_private part of the SCR structure points to the
 * SV, so there can only be one such SV for a particular SCR structure.  When
 * the last reference has gone (DESTROY is called),
 * b_perl_private/w_perl_private is reset; When the screen goes away before
 * all references are gone, the value of the SV is reset;
 * any subsequent use of any of those reference will produce
 * a warning. (see typemap)
 */

    static SV *
newWINrv(rv, ptr)
    SV	    *rv;
    win_T   *ptr;
{
    sv_upgrade(rv, SVt_RV);
    if (ptr->w_perl_private == NULL)
    {
	ptr->w_perl_private = newSV(0);
	sv_setiv(ptr->w_perl_private, (IV)ptr);
    }
    else
	SvREFCNT_inc(ptr->w_perl_private);
    SvRV(rv) = ptr->w_perl_private;
    SvROK_on(rv);
    return sv_bless(rv, gv_stashpv("VIWIN", TRUE));
}

    static SV *
newBUFrv(rv, ptr)
    SV	    *rv;
    buf_T   *ptr;
{
    sv_upgrade(rv, SVt_RV);
    if (ptr->b_perl_private == NULL)
    {
	ptr->b_perl_private = newSV(0);
	sv_setiv(ptr->b_perl_private, (IV)ptr);
    }
    else
	SvREFCNT_inc(ptr->b_perl_private);
    SvRV(rv) = ptr->b_perl_private;
    SvROK_on(rv);
    return sv_bless(rv, gv_stashpv("VIBUF", TRUE));
}

/*
 * perl_win_free
 *	Remove all refences to the window to be destroyed
 */
    void
perl_win_free(wp)
    win_T *wp;
{
    if (wp->w_perl_private)
	sv_setiv((SV *)wp->w_perl_private, 0);
    return;
}

    void
perl_buf_free(bp)
    buf_T *bp;
{
    if (bp->b_perl_private)
	sv_setiv((SV *)bp->b_perl_private, 0);
    return;
}

#ifndef PROTO
# if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
I32 cur_val(pTHX_ IV iv, SV *sv);
# else
I32 cur_val(IV iv, SV *sv);
#endif

/*
 * Handler for the magic variables $main::curwin and $main::curbuf.
 * The handler is put into the magic vtbl for these variables.
 * (This is effectively a C-level equivalent of a tied variable).
 * There is no "set" function as the variables are read-only.
 */
# if (PERL_REVISION == 5) && (PERL_VERSION >= 8)
I32 cur_val(pTHX_ IV iv, SV *sv)
# else
I32 cur_val(IV iv, SV *sv)
# endif
{
    SV *rv;
    if (iv == 0)
	rv = newWINrv(newSV(0), curwin);
    else
	rv = newBUFrv(newSV(0), curbuf);
    sv_setsv(sv, rv);
    return 0;
}
#endif /* !PROTO */

struct ufuncs cw_funcs = { cur_val, 0, 0 };
struct ufuncs cb_funcs = { cur_val, 0, 1 };

/*
 * VIM_init(): Vim-specific initialisation.
 * Make the magical main::curwin and main::curbuf variables
 */
    static void
VIM_init()
{
    static char cw[] = "main::curwin";
    static char cb[] = "main::curbuf";
    SV *sv;

    sv = perl_get_sv(cw, TRUE);
    sv_magic(sv, NULL, 'U', (char *)&cw_funcs, sizeof(cw_funcs));
    SvREADONLY_on(sv);

    sv = perl_get_sv(cb, TRUE);
    sv_magic(sv, NULL, 'U', (char *)&cb_funcs, sizeof(cb_funcs));
    SvREADONLY_on(sv);

    /*
     * Setup the Safe compartment.
     * It shouldn't be a fatal error if the Safe module is missing.
     * XXX: Only shares the 'Msg' routine (which has to be called
     * like 'Msg(...)').
     */
    (void)perl_eval_pv( "if ( eval( 'require Safe' ) ) { $VIM::safe = Safe->new(); $VIM::safe->share_from( 'VIM', ['Msg'] ); }", G_DISCARD | G_VOID );

}

#ifdef DYNAMIC_PERL
static char *e_noperl = N_("Sorry, this command is disabled: the Perl library could not be loaded.");
#endif

/*
 * ":perl"
 */
    void
ex_perl(eap)
    exarg_T	*eap;
{
    char	*err;
    char	*script;
    STRLEN	length;
    SV		*sv;
#ifdef HAVE_SANDBOX
    SV		*safe;
#endif

    script = (char *)script_get(eap, eap->arg);
    if (eap->skip)
    {
	vim_free(script);
	return;
    }

    if (perl_interp == NULL)
    {
#ifdef DYNAMIC_PERL
	if (!perl_enabled(TRUE))
	{
	    EMSG(_(e_noperl));
	    vim_free(script);
	    return;
	}
#endif
	perl_init();
    }

    {
    dSP;
    ENTER;
    SAVETMPS;

    if (script == NULL)
	sv = newSVpv((char *)eap->arg, 0);
    else
    {
	sv = newSVpv(script, 0);
	vim_free(script);
    }

#ifdef HAVE_SANDBOX
    if (sandbox)
    {
	safe = perl_get_sv( "VIM::safe", FALSE );
# ifndef MAKE_TEST  /* avoid a warning for unreachable code */
	if (safe == NULL || !SvTRUE(safe))
	    EMSG(_("E299: Perl evaluation forbidden in sandbox without the Safe module"));
	else
# endif
	{
	    PUSHMARK(SP);
	    XPUSHs(safe);
	    XPUSHs(sv);
	    PUTBACK;
	    perl_call_method("reval", G_DISCARD);
	}
    }
    else
#endif
	perl_eval_sv(sv, G_DISCARD | G_NOARGS);

    SvREFCNT_dec(sv);

    err = SvPV(GvSV(PL_errgv), length);

    FREETMPS;
    LEAVE;

    if (!length)
	return;

    msg_split((char_u *)err, highlight_attr[HLF_E]);
    return;
    }
}

    static int
replace_line(line, end)
    linenr_T	*line, *end;
{
    char *str;

    if (SvOK(GvSV(PL_defgv)))
    {
	str = SvPV(GvSV(PL_defgv), PL_na);
	ml_replace(*line, (char_u *)str, 1);
	changed_bytes(*line, 0);
    }
    else
    {
	ml_delete(*line, FALSE);
	deleted_lines_mark(*line, 1L);
	--(*end);
	--(*line);
    }
    return OK;
}

/*
 * ":perldo".
 */
    void
ex_perldo(eap)
    exarg_T	*eap;
{
    STRLEN	length;
    SV		*sv;
    char	*str;
    linenr_T	i;

    if (bufempty())
	return;

    if (perl_interp == NULL)
    {
#ifdef DYNAMIC_PERL
	if (!perl_enabled(TRUE))
	{
	    EMSG(_(e_noperl));
	    return;
	}
#endif
	perl_init();
    }
    {
    dSP;
    length = strlen((char *)eap->arg);
    sv = newSV(length + sizeof("sub VIM::perldo {") - 1 + 1);
    sv_setpvn(sv, "sub VIM::perldo {", sizeof("sub VIM::perldo {") - 1);
    sv_catpvn(sv, (char *)eap->arg, length);
    sv_catpvn(sv, "}", 1);
    perl_eval_sv(sv, G_DISCARD | G_NOARGS);
    SvREFCNT_dec(sv);
    str = SvPV(GvSV(PL_errgv), length);
    if (length)
	goto err;

    if (u_save(eap->line1 - 1, eap->line2 + 1) != OK)
	return;

    ENTER;
    SAVETMPS;
    for (i = eap->line1; i <= eap->line2; i++)
    {
	sv_setpv(GvSV(PL_defgv), (char *)ml_get(i));
	PUSHMARK(sp);
	perl_call_pv("VIM::perldo", G_SCALAR | G_EVAL);
	str = SvPV(GvSV(PL_errgv), length);
	if (length)
	    break;
	SPAGAIN;
	if (SvTRUEx(POPs))
	{
	    if (replace_line(&i, &eap->line2) != OK)
	    {
		PUTBACK;
		break;
	    }
	}
	PUTBACK;
    }
    FREETMPS;
    LEAVE;
    check_cursor();
    update_screen(NOT_VALID);
    if (!length)
	return;

err:
    msg_split((char_u *)str, highlight_attr[HLF_E]);
    return;
    }
}

#ifndef FEAT_WINDOWS
int win_valid(win_T *w) { return TRUE; }
int win_count() { return 1; }
win_T *win_find_nr(int n) { return curwin; }
#endif

XS(XS_VIM_Msg);
XS(XS_VIM_SetOption);
XS(XS_VIM_DoCommand);
XS(XS_VIM_Eval);
XS(XS_VIM_Buffers);
XS(XS_VIM_Windows);
XS(XS_VIWIN_DESTROY);
XS(XS_VIWIN_Buffer);
XS(XS_VIWIN_SetHeight);
XS(XS_VIWIN_Cursor);
XS(XS_VIBUF_DESTROY);
XS(XS_VIBUF_Name);
XS(XS_VIBUF_Number);
XS(XS_VIBUF_Count);
XS(XS_VIBUF_Get);
XS(XS_VIBUF_Set);
XS(XS_VIBUF_Delete);
XS(XS_VIBUF_Append);
XS(boot_VIM);

    static void
xs_init(pTHX)
{
    char *file = __FILE__;

    /* DynaLoader is a special case */
    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
    newXS("VIM::bootstrap", boot_VIM, file);
}

typedef win_T *	VIWIN;
typedef buf_T *	VIBUF;

MODULE = VIM	    PACKAGE = VIM

void
Msg(text, hl=NULL)
    char	*text;
    char	*hl;

    PREINIT:
    int		attr;
    int		id;

    PPCODE:
    if (text != NULL)
    {
	attr = 0;
	if (hl != NULL)
	{
	    id = syn_name2id((char_u *)hl);
	    if (id != 0)
		attr = syn_id2attr(id);
	}
	msg_split((char_u *)text, attr);
    }

void
SetOption(line)
    char *line;

    PPCODE:
    if (line != NULL)
	do_set((char_u *)line, 0);
    update_screen(NOT_VALID);

void
DoCommand(line)
    char *line;

    PPCODE:
    if (line != NULL)
	do_cmdline_cmd((char_u *)line);

void
Eval(str)
    char *str;

    PREINIT:
	char_u *value;
    PPCODE:
	value = eval_to_string((char_u *)str, (char_u **)0, TRUE);
	if (value == NULL)
	{
	    XPUSHs(sv_2mortal(newSViv(0)));
	    XPUSHs(sv_2mortal(newSVpv("", 0)));
	}
	else
	{
	    XPUSHs(sv_2mortal(newSViv(1)));
	    XPUSHs(sv_2mortal(newSVpv((char *)value, 0)));
	    vim_free(value);
	}

void
Buffers(...)

    PREINIT:
    buf_T *vimbuf;
    int i, b;

    PPCODE:
    if (items == 0)
    {
	if (GIMME == G_SCALAR)
	{
	    i = 0;
	    for (vimbuf = firstbuf; vimbuf; vimbuf = vimbuf->b_next)
		++i;

	    XPUSHs(sv_2mortal(newSViv(i)));
	}
	else
	{
	    for (vimbuf = firstbuf; vimbuf; vimbuf = vimbuf->b_next)
		XPUSHs(newBUFrv(newSV(0), vimbuf));
	}
    }
    else
    {
	for (i = 0; i < items; i++)
	{
	    SV *sv = ST(i);
	    if (SvIOK(sv))
		b = SvIV(ST(i));
	    else
	    {
		char_u *pat;
		STRLEN len;

		pat = (char_u *)SvPV(sv, len);
		++emsg_off;
		b = buflist_findpat(pat, pat+len, FALSE, FALSE);
		--emsg_off;
	    }

	    if (b >= 0)
	    {
		vimbuf = buflist_findnr(b);
		if (vimbuf)
		    XPUSHs(newBUFrv(newSV(0), vimbuf));
	    }
	}
    }

void
Windows(...)

    PREINIT:
    win_T   *vimwin;
    int	    i, w;

    PPCODE:
    if (items == 0)
    {
	if (GIMME == G_SCALAR)
	    XPUSHs(sv_2mortal(newSViv(win_count())));
	else
	{
	    for (vimwin = firstwin; vimwin != NULL; vimwin = W_NEXT(vimwin))
		XPUSHs(newWINrv(newSV(0), vimwin));
	}
    }
    else
    {
	for (i = 0; i < items; i++)
	{
	    w = SvIV(ST(i));
	    vimwin = win_find_nr(w);
	    if (vimwin)
		XPUSHs(newWINrv(newSV(0), vimwin));
	}
    }

MODULE = VIM	    PACKAGE = VIWIN

void
DESTROY(win)
    VIWIN win

    CODE:
    if (win_valid(win))
	win->w_perl_private = 0;

SV *
Buffer(win)
    VIWIN win

    CODE:
    if (!win_valid(win))
	win = curwin;
    RETVAL = newBUFrv(newSV(0), win->w_buffer);
    OUTPUT:
    RETVAL

void
SetHeight(win, height)
    VIWIN win
    int height;

    PREINIT:
    win_T *savewin;

    PPCODE:
    if (!win_valid(win))
	win = curwin;
    savewin = curwin;
    curwin = win;
    win_setheight(height);
    curwin = savewin;

void
Cursor(win, ...)
    VIWIN win

    PPCODE:
    if(items == 1)
    {
      EXTEND(sp, 2);
      if (!win_valid(win))
	  win = curwin;
      PUSHs(sv_2mortal(newSViv(win->w_cursor.lnum)));
      PUSHs(sv_2mortal(newSViv(win->w_cursor.col)));
    }
    else if(items == 3)
    {
      int lnum, col;

      if (!win_valid(win))
	  win = curwin;
      lnum = SvIV(ST(1));
      col = SvIV(ST(2));
      win->w_cursor.lnum = lnum;
      win->w_cursor.col = col;
      check_cursor();		    /* put cursor on an existing line */
      update_screen(NOT_VALID);
    }

MODULE = VIM	    PACKAGE = VIBUF

void
DESTROY(vimbuf)
    VIBUF vimbuf;

    CODE:
    if (buf_valid(vimbuf))
	vimbuf->b_perl_private = 0;

void
Name(vimbuf)
    VIBUF vimbuf;

    PPCODE:
    if (!buf_valid(vimbuf))
	vimbuf = curbuf;
    /* No file name returns an empty string */
    if (vimbuf->b_fname == NULL)
	XPUSHs(sv_2mortal(newSVpv("", 0)));
    else
	XPUSHs(sv_2mortal(newSVpv((char *)vimbuf->b_fname, 0)));

void
Number(vimbuf)
    VIBUF vimbuf;

    PPCODE:
    if (!buf_valid(vimbuf))
	vimbuf = curbuf;
    XPUSHs(sv_2mortal(newSViv(vimbuf->b_fnum)));

void
Count(vimbuf)
    VIBUF vimbuf;

    PPCODE:
    if (!buf_valid(vimbuf))
	vimbuf = curbuf;
    XPUSHs(sv_2mortal(newSViv(vimbuf->b_ml.ml_line_count)));

void
Get(vimbuf, ...)
    VIBUF vimbuf;

    PREINIT:
    char_u *line;
    int i;
    long lnum;
    PPCODE:
    if (buf_valid(vimbuf))
    {
	for (i = 1; i < items; i++)
	{
	    lnum = SvIV(ST(i));
	    if (lnum > 0 && lnum <= vimbuf->b_ml.ml_line_count)
	    {
		line = ml_get_buf(vimbuf, lnum, FALSE);
		XPUSHs(sv_2mortal(newSVpv((char *)line, 0)));
	    }
	}
    }

void
Set(vimbuf, ...)
    VIBUF vimbuf;

    PREINIT:
    int i;
    long lnum;
    char *line;
    PPCODE:
    if (buf_valid(vimbuf))
    {
	if (items < 3)
	    croak("Usage: VIBUF::Set(vimbuf, lnum, @lines)");

	lnum = SvIV(ST(1));
	for(i = 2; i < items; i++, lnum++)
	{
	    line = SvPV(ST(i),PL_na);
	    if (lnum > 0 && lnum <= vimbuf->b_ml.ml_line_count && line != NULL)
	    {
		aco_save_T	aco;

		/* set curwin/curbuf for "vimbuf" and save some things */
		aucmd_prepbuf(&aco, vimbuf);

		if (u_savesub(lnum) == OK)
		{
		    ml_replace(lnum, (char_u *)line, TRUE);
		    changed_bytes(lnum, 0);
		}

		/* restore curwin/curbuf and a few other things */
		aucmd_restbuf(&aco);
		/* Careful: autocommands may have made "vimbuf" invalid! */
	    }
	}
    }

void
Delete(vimbuf, ...)
    VIBUF vimbuf;

    PREINIT:
    long i, lnum = 0, count = 0;
    PPCODE:
    if (buf_valid(vimbuf))
    {
	if (items == 2)
	{
	    lnum = SvIV(ST(1));
	    count = 1;
	}
	else if (items == 3)
	{
	    lnum = SvIV(ST(1));
	    count = 1 + SvIV(ST(2)) - lnum;
	    if(count == 0)
		count = 1;
	    if(count < 0)
	    {
		lnum -= count;
		count = -count;
	    }
	}
	if (items >= 2)
	{
	    for (i = 0; i < count; i++)
	    {
		if (lnum > 0 && lnum <= vimbuf->b_ml.ml_line_count)
		{
		    aco_save_T	aco;

		    /* set curwin/curbuf for "vimbuf" and save some things */
		    aucmd_prepbuf(&aco, vimbuf);

		    if (u_savedel(lnum, 1) == OK)
		    {
			ml_delete(lnum, 0);
			check_cursor();
			deleted_lines_mark(lnum, 1L);
		    }

		    /* restore curwin/curbuf and a few other things */
		    aucmd_restbuf(&aco);
		    /* Careful: autocommands may have made "vimbuf" invalid! */

		    update_curbuf(VALID);
		}
	    }
	}
    }

void
Append(vimbuf, ...)
    VIBUF vimbuf;

    PREINIT:
    int		i;
    long	lnum;
    char	*line;
    PPCODE:
    if (buf_valid(vimbuf))
    {
	if (items < 3)
	    croak("Usage: VIBUF::Append(vimbuf, lnum, @lines)");

	lnum = SvIV(ST(1));
	for (i = 2; i < items; i++, lnum++)
	{
	    line = SvPV(ST(i),PL_na);
	    if (lnum >= 0 && lnum <= vimbuf->b_ml.ml_line_count && line != NULL)
	    {
		aco_save_T	aco;

		/* set curwin/curbuf for "vimbuf" and save some things */
		aucmd_prepbuf(&aco, vimbuf);

		if (u_inssub(lnum + 1) == OK)
		{
		    ml_append(lnum, (char_u *)line, (colnr_T)0, FALSE);
		    appended_lines_mark(lnum, 1L);
		}

		/* restore curwin/curbuf and a few other things */
		aucmd_restbuf(&aco);
		/* Careful: autocommands may have made "vimbuf" invalid! */

		update_curbuf(VALID);
	    }
	}
    }

