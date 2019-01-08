/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Ruby interface by Shugo Maeda
 *   with improvements by SegPhault (Ryan Paul)
 *   with improvements by Jon Maken
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "protodef.h"
#ifdef HAVE_CONFIG_H
# include "auto/config.h"
#endif

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
# if !defined(DYNAMIC_RUBY_VER) || (DYNAMIC_RUBY_VER < 18)
#   define NT
# endif
# ifndef DYNAMIC_RUBY
#  define IMPORT /* For static dll usage __declspec(dllimport) */
#  define RUBYEXTERN __declspec(dllimport)
# endif
#endif
#ifndef RUBYEXTERN
# define RUBYEXTERN extern
#endif

#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 24
# define USE_RUBY_INTEGER
#endif

#ifdef DYNAMIC_RUBY
/*
 * This is tricky.  In ruby.h there is (inline) function rb_class_of()
 * definition.  This function use these variables.  But we want function to
 * use dll_* variables.
 */
# define rb_cFalseClass		(*dll_rb_cFalseClass)
# define rb_cFixnum		(*dll_rb_cFixnum)
# if defined(USE_RUBY_INTEGER)
#  define rb_cInteger		(*dll_rb_cInteger)
# endif
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 20
#  define rb_cFloat		(*dll_rb_cFloat)
# endif
# define rb_cNilClass		(*dll_rb_cNilClass)
# define rb_cSymbol		(*dll_rb_cSymbol)
# define rb_cTrueClass		(*dll_rb_cTrueClass)
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
/*
 * On ver 1.8, all Ruby functions are exported with "__declspec(dllimport)"
 * in ruby.h.  But it causes trouble for these variables, because it is
 * defined in this file.  When defined this RUBY_EXPORT it modified to
 * "extern" and be able to avoid this problem.
 */
#  define RUBY_EXPORT
# endif

#if !(defined(WIN32) || defined(_WIN64))
# include <dlfcn.h>
# define HINSTANCE void*
# define RUBY_PROC void*
# define load_dll(n) dlopen((n), RTLD_LAZY|RTLD_GLOBAL)
# define symbol_from_dll dlsym
# define close_dll dlclose
#else
# define RUBY_PROC FARPROC
# define load_dll vimLoadLib
# define symbol_from_dll GetProcAddress
# define close_dll FreeLibrary
#endif

#endif  /* ifdef DYNAMIC_RUBY */

/* suggested by Ariya Mizutani */
#if (_MSC_VER == 1200)
# undef _WIN32_WINNT
#endif

#if (defined(RUBY_VERSION) && RUBY_VERSION >= 19) \
    || (defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 19)
# define RUBY19_OR_LATER 1
#endif

#if (defined(RUBY_VERSION) && RUBY_VERSION >= 20) \
    || (defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 20)
# define RUBY20_OR_LATER 1
#endif

#if (defined(RUBY_VERSION) && RUBY_VERSION >= 21) \
    || (defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 21)
# define RUBY21_OR_LATER 1
#endif

#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 19
/* Ruby 1.9 defines a number of static functions which use rb_num2long and
 * rb_int2big */
# define rb_num2long rb_num2long_stub
# define rb_int2big rb_int2big_stub
#endif

#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 19 \
	&& VIM_SIZEOF_INT < VIM_SIZEOF_LONG
/* Ruby 1.9 defines a number of static functions which use rb_fix2int and
 * rb_num2int if VIM_SIZEOF_INT < VIM_SIZEOF_LONG (64bit) */
# define rb_fix2int rb_fix2int_stub
# define rb_num2int rb_num2int_stub
#endif

#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER == 21
/* Ruby 2.1 adds new GC called RGenGC and RARRAY_PTR uses
 * rb_gc_writebarrier_unprotect_promoted if USE_RGENGC  */
# define rb_gc_writebarrier_unprotect_promoted rb_gc_writebarrier_unprotect_promoted_stub
#endif
#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 22
# define rb_gc_writebarrier_unprotect rb_gc_writebarrier_unprotect_stub
#endif

#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 26
# define rb_ary_detransient (*dll_rb_ary_detransient)
#endif

#include <ruby.h>
#ifdef RUBY19_OR_LATER
# include <ruby/encoding.h>
#endif

#undef off_t	/* ruby defines off_t as _int64, Mingw uses long */
#undef EXTERN
#undef _

/* T_DATA defined both by Ruby and Mac header files, hack around it... */
#if defined(MACOS_X)
# define __OPENTRANSPORT__
# define __OPENTRANSPORTPROTOCOL__
# define __OPENTRANSPORTPROVIDERS__
#endif

/*
 * The TypedData_XXX macro family can be used since Ruby 1.9.2 but
 * rb_data_type_t changed in 1.9.3, therefore require at least 2.0.
 * The old Data_XXX macro family was deprecated on Ruby 2.2.
 * Use TypedData_XXX if available.
 */
#if defined(TypedData_Wrap_Struct) && defined(RUBY20_OR_LATER)
# define USE_TYPEDDATA	1
#endif

/*
 * Backward compatibility for Ruby 1.8 and earlier.
 * Ruby 1.9 does not provide STR2CSTR, instead StringValuePtr is provided.
 * Ruby 1.9 does not provide RXXX(s)->len and RXXX(s)->ptr, instead
 * RXXX_LEN(s) and RXXX_PTR(s) are provided.
 */
#ifndef StringValuePtr
# define StringValuePtr(s) STR2CSTR(s)
#endif
#ifndef RARRAY_LEN
# define RARRAY_LEN(s) RARRAY(s)->len
#endif
#ifndef RARRAY_PTR
# define RARRAY_PTR(s) RARRAY(s)->ptr
#endif
#ifndef RSTRING_LEN
# define RSTRING_LEN(s) RSTRING(s)->len
#endif
#ifndef RSTRING_PTR
# define RSTRING_PTR(s) RSTRING(s)->ptr
#endif

#ifdef HAVE_DUP
# undef HAVE_DUP
#endif

#include "vim.h"
#include "version.h"

#if defined(PROTO) && !defined(FEAT_RUBY)
/* Define these to be able to generate the function prototypes. */
# define VALUE int
# define RUBY_DATA_FUNC int
#endif

static int ruby_initialized = 0;
static void *ruby_stack_start;
static VALUE objtbl;

static VALUE mVIM;
static VALUE cBuffer;
static VALUE cVimWindow;
static VALUE eDeletedBufferError;
static VALUE eDeletedWindowError;

static int ensure_ruby_initialized(void);
static void error_print(int);
static void ruby_io_init(void);
static void ruby_vim_init(void);

#if defined(RUBY19_OR_LATER) || defined(RUBY_INIT_STACK)
# if defined(__ia64) && !defined(ruby_init_stack)
#  define ruby_init_stack(addr) ruby_init_stack((addr), rb_ia64_bsp())
# endif
#endif

#if defined(DYNAMIC_RUBY) || defined(PROTO)
# if defined(PROTO) && !defined(HINSTANCE)
#  define HINSTANCE int		/* for generating prototypes */
# endif

/*
 * Wrapper defines
 */
# define rb_assoc_new			dll_rb_assoc_new
# define rb_cObject			(*dll_rb_cObject)
# define rb_check_type			dll_rb_check_type
# ifdef USE_TYPEDDATA
#  define rb_check_typeddata		dll_rb_check_typeddata
# endif
# define rb_class_path			dll_rb_class_path
# ifdef USE_TYPEDDATA
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 23
#   define rb_data_typed_object_wrap	dll_rb_data_typed_object_wrap
#  else
#   define rb_data_typed_object_alloc	dll_rb_data_typed_object_alloc
#  endif
# else
#  define rb_data_object_alloc		dll_rb_data_object_alloc
# endif
# define rb_define_class_under		dll_rb_define_class_under
# define rb_define_const			dll_rb_define_const
# define rb_define_global_function	dll_rb_define_global_function
# define rb_define_method		dll_rb_define_method
# define rb_define_module		dll_rb_define_module
# define rb_define_module_function	dll_rb_define_module_function
# define rb_define_singleton_method	dll_rb_define_singleton_method
# define rb_define_virtual_variable	dll_rb_define_virtual_variable
# define rb_stdout			(*dll_rb_stdout)
# define rb_stderr			(*dll_rb_stderr)
# define rb_eArgError			(*dll_rb_eArgError)
# define rb_eIndexError			(*dll_rb_eIndexError)
# define rb_eRuntimeError		(*dll_rb_eRuntimeError)
# define rb_eStandardError		(*dll_rb_eStandardError)
# define rb_eval_string_protect		dll_rb_eval_string_protect
# ifdef RUBY21_OR_LATER
#  define rb_funcallv			dll_rb_funcallv
# else
#  define rb_funcall2			dll_rb_funcall2
# endif
# define rb_global_variable		dll_rb_global_variable
# define rb_hash_aset			dll_rb_hash_aset
# define rb_hash_new			dll_rb_hash_new
# define rb_inspect			dll_rb_inspect
# define rb_int2inum			dll_rb_int2inum

// ruby.h may redefine rb_intern to use RUBY_CONST_ID_CACHE(), but that won't
// work.  Not using the cache appears to be the best solution.
# undef rb_intern
# define rb_intern			dll_rb_intern

# if VIM_SIZEOF_INT < VIM_SIZEOF_LONG /* 64 bits only */
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER <= 18
#   define rb_fix2int			dll_rb_fix2int
#   define rb_num2int			dll_rb_num2int
#  endif
#  define rb_num2uint			dll_rb_num2uint
# endif
# define rb_lastline_get			dll_rb_lastline_get
# define rb_lastline_set			dll_rb_lastline_set
# define rb_protect			dll_rb_protect
# define rb_load			dll_rb_load
# ifndef RUBY19_OR_LATER
#  define rb_num2long			dll_rb_num2long
# endif
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER <= 19
#  define rb_num2ulong			dll_rb_num2ulong
# endif
# define rb_obj_alloc			dll_rb_obj_alloc
# define rb_obj_as_string		dll_rb_obj_as_string
# define rb_obj_id			dll_rb_obj_id
# define rb_raise			dll_rb_raise
# define rb_str_cat			dll_rb_str_cat
# define rb_str_concat			dll_rb_str_concat
# undef rb_str_new
# define rb_str_new			dll_rb_str_new
# ifdef rb_str_new2
/* Ruby may #define rb_str_new2 to use rb_str_new_cstr. */
#  define need_rb_str_new_cstr 1
/* Ruby's headers #define rb_str_new_cstr to make use of GCC's
 * __builtin_constant_p extension. */
#  undef rb_str_new_cstr
#  define rb_str_new_cstr		dll_rb_str_new_cstr
# else
#  define rb_str_new2			dll_rb_str_new2
# endif
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
#  define rb_string_value		dll_rb_string_value
#  define rb_string_value_ptr		dll_rb_string_value_ptr
#  define rb_float_new			dll_rb_float_new
#  define rb_ary_new			dll_rb_ary_new
#  ifdef rb_ary_new4
#    define RB_ARY_NEW4_MACRO 1
#    undef rb_ary_new4
#  endif
#  define rb_ary_new4			dll_rb_ary_new4
#  define rb_ary_push			dll_rb_ary_push
#  if defined(RUBY19_OR_LATER) || defined(RUBY_INIT_STACK)
#   ifdef __ia64
#    define rb_ia64_bsp			dll_rb_ia64_bsp
#    undef ruby_init_stack
#    define ruby_init_stack(addr)	dll_ruby_init_stack((addr), rb_ia64_bsp())
#   else
#    define ruby_init_stack		dll_ruby_init_stack
#   endif
#  endif
# else
#  define rb_str2cstr			dll_rb_str2cstr
# endif
# ifdef RUBY19_OR_LATER
#  define rb_errinfo			dll_rb_errinfo
# else
#  define ruby_errinfo			(*dll_ruby_errinfo)
# endif
# define ruby_init			dll_ruby_init
# define ruby_init_loadpath		dll_ruby_init_loadpath
# ifdef WIN3264
#  ifdef RUBY19_OR_LATER
#   define ruby_sysinit			dll_ruby_sysinit
#  else
#   define NtInitialize			dll_NtInitialize
#  endif
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
#   define rb_w32_snprintf		dll_rb_w32_snprintf
#  endif
# endif

# ifdef RUBY19_OR_LATER
#  define ruby_script			dll_ruby_script
#  define rb_enc_find_index		dll_rb_enc_find_index
#  define rb_enc_find			dll_rb_enc_find
#  undef rb_enc_str_new
#  define rb_enc_str_new		dll_rb_enc_str_new
#  define rb_sprintf			dll_rb_sprintf
#  define rb_require			dll_rb_require
#  define ruby_options			dll_ruby_options
# endif

/*
 * Pointers for dynamic link
 */
static VALUE (*dll_rb_assoc_new) (VALUE, VALUE);
VALUE *dll_rb_cFalseClass;
VALUE *dll_rb_cFixnum;
# if defined(USE_RUBY_INTEGER)
VALUE *dll_rb_cInteger;
# endif
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 20
VALUE *dll_rb_cFloat;
# endif
VALUE *dll_rb_cNilClass;
static VALUE *dll_rb_cObject;
VALUE *dll_rb_cSymbol;
VALUE *dll_rb_cTrueClass;
static void (*dll_rb_check_type) (VALUE,int);
# ifdef USE_TYPEDDATA
static void *(*dll_rb_check_typeddata) (VALUE,const rb_data_type_t *);
# endif
static VALUE (*dll_rb_class_path) (VALUE);
# ifdef USE_TYPEDDATA
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 23
static VALUE (*dll_rb_data_typed_object_wrap) (VALUE, void*, const rb_data_type_t *);
#  else
static VALUE (*dll_rb_data_typed_object_alloc) (VALUE, void*, const rb_data_type_t *);
#  endif
# else
static VALUE (*dll_rb_data_object_alloc) (VALUE, void*, RUBY_DATA_FUNC, RUBY_DATA_FUNC);
# endif
static VALUE (*dll_rb_define_class_under) (VALUE, const char*, VALUE);
static void (*dll_rb_define_const) (VALUE,const char*,VALUE);
static void (*dll_rb_define_global_function) (const char*,VALUE(*)(),int);
static void (*dll_rb_define_method) (VALUE,const char*,VALUE(*)(),int);
static VALUE (*dll_rb_define_module) (const char*);
static void (*dll_rb_define_module_function) (VALUE,const char*,VALUE(*)(),int);
static void (*dll_rb_define_singleton_method) (VALUE,const char*,VALUE(*)(),int);
static void (*dll_rb_define_virtual_variable) (const char*,VALUE(*)(),void(*)());
static VALUE *dll_rb_stdout;
static VALUE *dll_rb_stderr;
static VALUE *dll_rb_eArgError;
static VALUE *dll_rb_eIndexError;
static VALUE *dll_rb_eRuntimeError;
static VALUE *dll_rb_eStandardError;
static VALUE (*dll_rb_eval_string_protect) (const char*, int*);
# ifdef RUBY21_OR_LATER
static VALUE (*dll_rb_funcallv) (VALUE, ID, int, const VALUE*);
# else
static VALUE (*dll_rb_funcall2) (VALUE, ID, int, const VALUE*);
# endif
static void (*dll_rb_global_variable) (VALUE*);
static VALUE (*dll_rb_hash_aset) (VALUE, VALUE, VALUE);
static VALUE (*dll_rb_hash_new) (void);
static VALUE (*dll_rb_inspect) (VALUE);
static VALUE (*dll_rb_int2inum) (long);
static ID (*dll_rb_intern) (const char*);
# if VIM_SIZEOF_INT < VIM_SIZEOF_LONG /* 64 bits only */
static long (*dll_rb_fix2int) (VALUE);
static long (*dll_rb_num2int) (VALUE);
static unsigned long (*dll_rb_num2uint) (VALUE);
# endif
static VALUE (*dll_rb_lastline_get) (void);
static void (*dll_rb_lastline_set) (VALUE);
static VALUE (*dll_rb_protect) (VALUE (*)(VALUE), VALUE, int*);
static void (*dll_rb_load) (VALUE, int);
static long (*dll_rb_num2long) (VALUE);
static unsigned long (*dll_rb_num2ulong) (VALUE);
static VALUE (*dll_rb_obj_alloc) (VALUE);
static VALUE (*dll_rb_obj_as_string) (VALUE);
static VALUE (*dll_rb_obj_id) (VALUE);
static void (*dll_rb_raise) (VALUE, const char*, ...);
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
static VALUE (*dll_rb_string_value) (volatile VALUE*);
# else
static char *(*dll_rb_str2cstr) (VALUE,int*);
# endif
static VALUE (*dll_rb_str_cat) (VALUE, const char*, long);
static VALUE (*dll_rb_str_concat) (VALUE, VALUE);
static VALUE (*dll_rb_str_new) (const char*, long);
# ifdef need_rb_str_new_cstr
/* Ruby may #define rb_str_new2 to use rb_str_new_cstr. */
static VALUE (*dll_rb_str_new_cstr) (const char*);
# else
static VALUE (*dll_rb_str_new2) (const char*);
# endif
# ifdef RUBY19_OR_LATER
static VALUE (*dll_rb_errinfo) (void);
# else
static VALUE *dll_ruby_errinfo;
# endif
static void (*dll_ruby_init) (void);
static void (*dll_ruby_init_loadpath) (void);
# ifdef WIN3264
#  ifdef RUBY19_OR_LATER
static void (*dll_ruby_sysinit) (int*, char***);
#  else
static void (*dll_NtInitialize) (int*, char***);
#  endif
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
static int (*dll_rb_w32_snprintf)(char*, size_t, const char*, ...);
#  endif
# endif
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
static char * (*dll_rb_string_value_ptr) (volatile VALUE*);
static VALUE (*dll_rb_float_new) (double);
static VALUE (*dll_rb_ary_new) (void);
static VALUE (*dll_rb_ary_new4) (long n, const VALUE *elts);
static VALUE (*dll_rb_ary_push) (VALUE, VALUE);
#  if DYNAMIC_RUBY_VER >= 26
static void (*dll_rb_ary_detransient) (VALUE);
#  endif
#  if defined(RUBY19_OR_LATER) || defined(RUBY_INIT_STACK)
#   ifdef __ia64
static void * (*dll_rb_ia64_bsp) (void);
static void (*dll_ruby_init_stack)(VALUE*, void*);
#   else
static void (*dll_ruby_init_stack)(VALUE*);
#   endif
#  endif
# endif
# ifdef RUBY19_OR_LATER
static VALUE (*dll_rb_int2big)(SIGNED_VALUE);
# endif

# ifdef RUBY19_OR_LATER
static void (*dll_ruby_script) (const char*);
static int (*dll_rb_enc_find_index) (const char*);
static rb_encoding* (*dll_rb_enc_find) (const char*);
static VALUE (*dll_rb_enc_str_new) (const char*, long, rb_encoding*);
static VALUE (*dll_rb_sprintf) (const char*, ...);
static VALUE (*dll_rb_require) (const char*);
static void* (*ruby_options)(int, char**);
# endif

# if defined(USE_RGENGC) && USE_RGENGC
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER == 21
static void (*dll_rb_gc_writebarrier_unprotect_promoted)(VALUE);
#  else
static void (*dll_rb_gc_writebarrier_unprotect)(VALUE obj);
#  endif
# endif

# if defined(RUBY19_OR_LATER) && !defined(PROTO)
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 22
long rb_num2long_stub(VALUE x)
#  else
SIGNED_VALUE rb_num2long_stub(VALUE x)
#  endif
{
    return dll_rb_num2long(x);
}
VALUE rb_int2big_stub(SIGNED_VALUE x)
{
    return dll_rb_int2big(x);
}
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 19 \
	&& VIM_SIZEOF_INT < VIM_SIZEOF_LONG
long rb_fix2int_stub(VALUE x)
{
    return dll_rb_fix2int(x);
}
long rb_num2int_stub(VALUE x)
{
    return dll_rb_num2int(x);
}
#  endif
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 20
VALUE
rb_float_new_in_heap(double d)
{
    return dll_rb_float_new(d);
}
#   if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 22
unsigned long rb_num2ulong(VALUE x)
#   else
VALUE rb_num2ulong(VALUE x)
#   endif
{
    return (long)RSHIFT((SIGNED_VALUE)(x),1);
}
#  endif
# endif

   /* Do not generate a prototype here, VALUE isn't always defined. */
# if defined(USE_RGENGC) && USE_RGENGC && !defined(PROTO)
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER == 21
void rb_gc_writebarrier_unprotect_promoted_stub(VALUE obj)
{
    dll_rb_gc_writebarrier_unprotect_promoted(obj);
}
#  else
void rb_gc_writebarrier_unprotect_stub(VALUE obj)
{
    dll_rb_gc_writebarrier_unprotect(obj);
}
#  endif
# endif

static HINSTANCE hinstRuby = NULL; /* Instance of ruby.dll */

/*
 * Table of name to function pointer of ruby.
 */
static struct
{
    char *name;
    RUBY_PROC *ptr;
} ruby_funcname_table[] =
{
    {"rb_assoc_new", (RUBY_PROC*)&dll_rb_assoc_new},
    {"rb_cFalseClass", (RUBY_PROC*)&dll_rb_cFalseClass},
# if defined(USE_RUBY_INTEGER)
    {"rb_cInteger", (RUBY_PROC*)&dll_rb_cInteger},
# else
    {"rb_cFixnum", (RUBY_PROC*)&dll_rb_cFixnum},
# endif
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 20
    {"rb_cFloat", (RUBY_PROC*)&dll_rb_cFloat},
# endif
    {"rb_cNilClass", (RUBY_PROC*)&dll_rb_cNilClass},
    {"rb_cObject", (RUBY_PROC*)&dll_rb_cObject},
    {"rb_cSymbol", (RUBY_PROC*)&dll_rb_cSymbol},
    {"rb_cTrueClass", (RUBY_PROC*)&dll_rb_cTrueClass},
    {"rb_check_type", (RUBY_PROC*)&dll_rb_check_type},
# ifdef USE_TYPEDDATA
    {"rb_check_typeddata", (RUBY_PROC*)&dll_rb_check_typeddata},
# endif
    {"rb_class_path", (RUBY_PROC*)&dll_rb_class_path},
# ifdef USE_TYPEDDATA
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 23
    {"rb_data_typed_object_wrap", (RUBY_PROC*)&dll_rb_data_typed_object_wrap},
#  else
    {"rb_data_typed_object_alloc", (RUBY_PROC*)&dll_rb_data_typed_object_alloc},
#  endif
# else
    {"rb_data_object_alloc", (RUBY_PROC*)&dll_rb_data_object_alloc},
# endif
    {"rb_define_class_under", (RUBY_PROC*)&dll_rb_define_class_under},
    {"rb_define_const", (RUBY_PROC*)&dll_rb_define_const},
    {"rb_define_global_function", (RUBY_PROC*)&dll_rb_define_global_function},
    {"rb_define_method", (RUBY_PROC*)&dll_rb_define_method},
    {"rb_define_module", (RUBY_PROC*)&dll_rb_define_module},
    {"rb_define_module_function", (RUBY_PROC*)&dll_rb_define_module_function},
    {"rb_define_singleton_method", (RUBY_PROC*)&dll_rb_define_singleton_method},
    {"rb_define_virtual_variable", (RUBY_PROC*)&dll_rb_define_virtual_variable},
    {"rb_stdout", (RUBY_PROC*)&dll_rb_stdout},
    {"rb_stderr", (RUBY_PROC*)&dll_rb_stderr},
    {"rb_eArgError", (RUBY_PROC*)&dll_rb_eArgError},
    {"rb_eIndexError", (RUBY_PROC*)&dll_rb_eIndexError},
    {"rb_eRuntimeError", (RUBY_PROC*)&dll_rb_eRuntimeError},
    {"rb_eStandardError", (RUBY_PROC*)&dll_rb_eStandardError},
    {"rb_eval_string_protect", (RUBY_PROC*)&dll_rb_eval_string_protect},
# ifdef RUBY21_OR_LATER
    {"rb_funcallv", (RUBY_PROC*)&dll_rb_funcallv},
# else
    {"rb_funcall2", (RUBY_PROC*)&dll_rb_funcall2},
# endif
    {"rb_global_variable", (RUBY_PROC*)&dll_rb_global_variable},
    {"rb_hash_aset", (RUBY_PROC*)&dll_rb_hash_aset},
    {"rb_hash_new", (RUBY_PROC*)&dll_rb_hash_new},
    {"rb_inspect", (RUBY_PROC*)&dll_rb_inspect},
    {"rb_int2inum", (RUBY_PROC*)&dll_rb_int2inum},
    {"rb_intern", (RUBY_PROC*)&dll_rb_intern},
# if VIM_SIZEOF_INT < VIM_SIZEOF_LONG /* 64 bits only */
    {"rb_fix2int", (RUBY_PROC*)&dll_rb_fix2int},
    {"rb_num2int", (RUBY_PROC*)&dll_rb_num2int},
    {"rb_num2uint", (RUBY_PROC*)&dll_rb_num2uint},
# endif
    {"rb_lastline_get", (RUBY_PROC*)&dll_rb_lastline_get},
    {"rb_lastline_set", (RUBY_PROC*)&dll_rb_lastline_set},
    {"rb_protect", (RUBY_PROC*)&dll_rb_protect},
    {"rb_load", (RUBY_PROC*)&dll_rb_load},
    {"rb_num2long", (RUBY_PROC*)&dll_rb_num2long},
    {"rb_num2ulong", (RUBY_PROC*)&dll_rb_num2ulong},
    {"rb_obj_alloc", (RUBY_PROC*)&dll_rb_obj_alloc},
    {"rb_obj_as_string", (RUBY_PROC*)&dll_rb_obj_as_string},
    {"rb_obj_id", (RUBY_PROC*)&dll_rb_obj_id},
    {"rb_raise", (RUBY_PROC*)&dll_rb_raise},
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
    {"rb_string_value", (RUBY_PROC*)&dll_rb_string_value},
# else
    {"rb_str2cstr", (RUBY_PROC*)&dll_rb_str2cstr},
# endif
    {"rb_str_cat", (RUBY_PROC*)&dll_rb_str_cat},
    {"rb_str_concat", (RUBY_PROC*)&dll_rb_str_concat},
    {"rb_str_new", (RUBY_PROC*)&dll_rb_str_new},
# ifdef need_rb_str_new_cstr
    {"rb_str_new_cstr", (RUBY_PROC*)&dll_rb_str_new_cstr},
# else
    {"rb_str_new2", (RUBY_PROC*)&dll_rb_str_new2},
# endif
# ifdef RUBY19_OR_LATER
    {"rb_errinfo", (RUBY_PROC*)&dll_rb_errinfo},
# else
    {"ruby_errinfo", (RUBY_PROC*)&dll_ruby_errinfo},
# endif
    {"ruby_init", (RUBY_PROC*)&dll_ruby_init},
    {"ruby_init_loadpath", (RUBY_PROC*)&dll_ruby_init_loadpath},
# ifdef WIN3264
#  ifdef RUBY19_OR_LATER
    {"ruby_sysinit", (RUBY_PROC*)&dll_ruby_sysinit},
#  else
    {"NtInitialize", (RUBY_PROC*)&dll_NtInitialize},
#  endif
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
    {"rb_w32_snprintf", (RUBY_PROC*)&dll_rb_w32_snprintf},
#  endif
# endif
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
    {"rb_string_value_ptr", (RUBY_PROC*)&dll_rb_string_value_ptr},
#  if DYNAMIC_RUBY_VER <= 19
    {"rb_float_new", (RUBY_PROC*)&dll_rb_float_new},
#  else
    {"rb_float_new_in_heap", (RUBY_PROC*)&dll_rb_float_new},
#  endif
    {"rb_ary_new", (RUBY_PROC*)&dll_rb_ary_new},
#  ifdef RB_ARY_NEW4_MACRO
    {"rb_ary_new_from_values", (RUBY_PROC*)&dll_rb_ary_new4},
#  else
    {"rb_ary_new4", (RUBY_PROC*)&dll_rb_ary_new4},
#  endif
    {"rb_ary_push", (RUBY_PROC*)&dll_rb_ary_push},
#  if DYNAMIC_RUBY_VER >= 26
    {"rb_ary_detransient", (RUBY_PROC*)&dll_rb_ary_detransient},
#  endif
# endif
# ifdef RUBY19_OR_LATER
    {"rb_int2big", (RUBY_PROC*)&dll_rb_int2big},
    {"ruby_script", (RUBY_PROC*)&dll_ruby_script},
    {"rb_enc_find_index", (RUBY_PROC*)&dll_rb_enc_find_index},
    {"rb_enc_find", (RUBY_PROC*)&dll_rb_enc_find},
    {"rb_enc_str_new", (RUBY_PROC*)&dll_rb_enc_str_new},
    {"rb_sprintf", (RUBY_PROC*)&dll_rb_sprintf},
    {"rb_require", (RUBY_PROC*)&dll_rb_require},
    {"ruby_options", (RUBY_PROC*)&dll_ruby_options},
# endif
# if defined(RUBY19_OR_LATER) || defined(RUBY_INIT_STACK)
#  ifdef __ia64
    {"rb_ia64_bsp", (RUBY_PROC*)&dll_rb_ia64_bsp},
#  endif
    {"ruby_init_stack", (RUBY_PROC*)&dll_ruby_init_stack},
# endif
# if defined(USE_RGENGC) && USE_RGENGC
#  if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER == 21
    {"rb_gc_writebarrier_unprotect_promoted", (RUBY_PROC*)&dll_rb_gc_writebarrier_unprotect_promoted},
#  else
    {"rb_gc_writebarrier_unprotect", (RUBY_PROC*)&dll_rb_gc_writebarrier_unprotect},
#  endif
# endif
    {"", NULL},
};

/*
 * Free ruby.dll
 */
    static void
end_dynamic_ruby(void)
{
    if (hinstRuby)
    {
	close_dll(hinstRuby);
	hinstRuby = NULL;
    }
}

/*
 * Load library and get all pointers.
 * Parameter 'libname' provides name of DLL.
 * Return OK or FAIL.
 */
    static int
ruby_runtime_link_init(char *libname, int verbose)
{
    int i;

    if (hinstRuby)
	return OK;
    hinstRuby = load_dll(libname);
    if (!hinstRuby)
    {
	if (verbose)
	    EMSG2(_(e_loadlib), libname);
	return FAIL;
    }

    for (i = 0; ruby_funcname_table[i].ptr; ++i)
    {
	if (!(*ruby_funcname_table[i].ptr = symbol_from_dll(hinstRuby,
			ruby_funcname_table[i].name)))
	{
	    close_dll(hinstRuby);
	    hinstRuby = NULL;
	    if (verbose)
		EMSG2(_(e_loadfunc), ruby_funcname_table[i].name);
	    return FAIL;
	}
    }
    return OK;
}

/*
 * If ruby is enabled (there is installed ruby on Windows system) return TRUE,
 * else FALSE.
 */
    int
ruby_enabled(int verbose)
{
    return ruby_runtime_link_init((char *)p_rubydll, verbose) == OK;
}
#endif /* defined(DYNAMIC_RUBY) || defined(PROTO) */

    void
ruby_end(void)
{
#ifdef DYNAMIC_RUBY
    end_dynamic_ruby();
#endif
}

void ex_ruby(exarg_T *eap)
{
    int state;
    char *script = NULL;

    script = (char *)script_get(eap, eap->arg);
    if (!eap->skip && ensure_ruby_initialized())
    {
	if (script == NULL)
	    rb_eval_string_protect((char *)eap->arg, &state);
	else
	    rb_eval_string_protect(script, &state);
	if (state)
	    error_print(state);
    }
    vim_free(script);
}

/*
 *  In Ruby 1.9 or later, ruby String object has encoding.
 *  conversion buffer string of vim to ruby String object using
 *  VIM encoding option.
 */
    static VALUE
vim_str2rb_enc_str(const char *s)
{
#ifdef RUBY19_OR_LATER
    int isnum;
    long lval;
    char_u *sval;
    rb_encoding *enc;

    isnum = get_option_value((char_u *)"enc", &lval, &sval, 0);
    if (isnum == 0)
    {
	enc = rb_enc_find((char *)sval);
	vim_free(sval);
	if (enc)
	{
	    return rb_enc_str_new(s, (long)strlen(s), enc);
	}
    }
#endif
    return rb_str_new2(s);
}

    static VALUE
eval_enc_string_protect(const char *str, int *state)
{
#ifdef RUBY19_OR_LATER
    int isnum;
    long lval;
    char_u *sval;
    rb_encoding *enc;
    VALUE v;

    isnum = get_option_value((char_u *)"enc", &lval, &sval, 0);
    if (isnum == 0)
    {
	enc = rb_enc_find((char *)sval);
	vim_free(sval);
	if (enc)
	{
	    v = rb_sprintf("#-*- coding:%s -*-\n%s", rb_enc_name(enc), str);
	    return rb_eval_string_protect(StringValuePtr(v), state);
	}
    }
#endif
    return rb_eval_string_protect(str, state);
}

void ex_rubydo(exarg_T *eap)
{
    int state;
    linenr_T i;
    buf_T   *was_curbuf = curbuf;

    if (ensure_ruby_initialized())
    {
	if (u_save(eap->line1 - 1, eap->line2 + 1) != OK)
	    return;
	for (i = eap->line1; i <= eap->line2; i++)
	{
	    VALUE line;

	    if (i > curbuf->b_ml.ml_line_count)
		break;
	    line = vim_str2rb_enc_str((char *)ml_get(i));
	    rb_lastline_set(line);
	    eval_enc_string_protect((char *) eap->arg, &state);
	    if (state)
	    {
		error_print(state);
		break;
	    }
	    if (was_curbuf != curbuf)
		break;
	    line = rb_lastline_get();
	    if (!NIL_P(line))
	    {
		if (TYPE(line) != T_STRING)
		{
		    EMSG(_("E265: $_ must be an instance of String"));
		    return;
		}
		ml_replace(i, (char_u *) StringValuePtr(line), 1);
		changed();
#ifdef SYNTAX_HL
		syn_changed(i); /* recompute syntax hl. for this line */
#endif
	    }
	}
	check_cursor();
	update_curbuf(NOT_VALID);
    }
}

static VALUE rb_load_wrap(VALUE file_to_load)
{
    rb_load(file_to_load, 0);
    return Qnil;
}

void ex_rubyfile(exarg_T *eap)
{
    int state;

    if (ensure_ruby_initialized())
    {
	VALUE file_to_load = rb_str_new2((const char *)eap->arg);
	rb_protect(rb_load_wrap, file_to_load, &state);
	if (state)
	    error_print(state);
    }
}

void ruby_buffer_free(buf_T *buf)
{
    if (buf->b_ruby_ref)
    {
	rb_hash_aset(objtbl, rb_obj_id((VALUE) buf->b_ruby_ref), Qnil);
	RDATA(buf->b_ruby_ref)->data = NULL;
    }
}

void ruby_window_free(win_T *win)
{
    if (win->w_ruby_ref)
    {
	rb_hash_aset(objtbl, rb_obj_id((VALUE) win->w_ruby_ref), Qnil);
	RDATA(win->w_ruby_ref)->data = NULL;
    }
}

static int ensure_ruby_initialized(void)
{
    if (!ruby_initialized)
    {
#ifdef DYNAMIC_RUBY
	if (ruby_enabled(TRUE))
	{
#endif
#ifdef _WIN32
	    /* suggested by Ariya Mizutani */
	    int argc = 1;
	    char *argv[] = {"gvim.exe"};
	    char **argvp = argv;
# ifdef RUBY19_OR_LATER
	    ruby_sysinit(&argc, &argvp);
# else
	    NtInitialize(&argc, &argvp);
# endif
#endif
	    {
#if defined(RUBY19_OR_LATER) || defined(RUBY_INIT_STACK)
		ruby_init_stack(ruby_stack_start);
#endif
		ruby_init();
	    }
#ifdef RUBY19_OR_LATER
	    {
		int dummy_argc = 2;
		char *dummy_argv[] = {"vim-ruby", "-e_=0"};
		ruby_options(dummy_argc, dummy_argv);
	    }
	    ruby_script("vim-ruby");
#else
	    ruby_init_loadpath();
#endif
	    ruby_io_init();
	    ruby_vim_init();
	    ruby_initialized = 1;
#ifdef DYNAMIC_RUBY
	}
	else
	{
	    EMSG(_("E266: Sorry, this command is disabled, the Ruby library could not be loaded."));
	    return 0;
	}
#endif
    }
    return ruby_initialized;
}

static void error_print(int state)
{
#if !defined(DYNAMIC_RUBY) && !defined(RUBY19_OR_LATER)
    RUBYEXTERN VALUE ruby_errinfo;
#endif
    VALUE error;
    VALUE eclass;
    VALUE einfo;
    VALUE bt;
    int attr;
    char buff[BUFSIZ];
    long i;

#define TAG_RETURN	0x1
#define TAG_BREAK	0x2
#define TAG_NEXT	0x3
#define TAG_RETRY	0x4
#define TAG_REDO	0x5
#define TAG_RAISE	0x6
#define TAG_THROW	0x7
#define TAG_FATAL	0x8
#define TAG_MASK	0xf

    switch (state)
    {
    case TAG_RETURN:
	EMSG(_("E267: unexpected return"));
	break;
    case TAG_NEXT:
	EMSG(_("E268: unexpected next"));
	break;
    case TAG_BREAK:
	EMSG(_("E269: unexpected break"));
	break;
    case TAG_REDO:
	EMSG(_("E270: unexpected redo"));
	break;
    case TAG_RETRY:
	EMSG(_("E271: retry outside of rescue clause"));
	break;
    case TAG_RAISE:
    case TAG_FATAL:
#ifdef RUBY19_OR_LATER
	error = rb_errinfo();
#else
	error = ruby_errinfo;
#endif
	eclass = CLASS_OF(error);
	einfo = rb_obj_as_string(error);
	if (eclass == rb_eRuntimeError && RSTRING_LEN(einfo) == 0)
	{
	    EMSG(_("E272: unhandled exception"));
	}
	else
	{
	    VALUE epath;
	    char *p;

	    epath = rb_class_path(eclass);
	    vim_snprintf(buff, BUFSIZ, "%s: %s",
		     RSTRING_PTR(epath), RSTRING_PTR(einfo));
	    p = strchr(buff, '\n');
	    if (p) *p = '\0';
	    EMSG(buff);
	}

	attr = syn_name2attr((char_u *)"Error");
# ifdef RUBY21_OR_LATER
	bt = rb_funcallv(error, rb_intern("backtrace"), 0, 0);
	for (i = 0; i < RARRAY_LEN(bt); i++)
	    msg_attr((char_u *)RSTRING_PTR(RARRAY_AREF(bt, i)), attr);
# else
	bt = rb_funcall2(error, rb_intern("backtrace"), 0, 0);
	for (i = 0; i < RARRAY_LEN(bt); i++)
	    msg_attr((char_u *)RSTRING_PTR(RARRAY_PTR(bt)[i]), attr);
# endif
	break;
    default:
	vim_snprintf(buff, BUFSIZ, _("E273: unknown longjmp status %d"), state);
	EMSG(buff);
	break;
    }
}

static VALUE vim_message(VALUE self UNUSED, VALUE str)
{
    char *buff, *p;

    str = rb_obj_as_string(str);
    if (RSTRING_LEN(str) > 0)
    {
	/* Only do this when the string isn't empty, alloc(0) causes trouble. */
	buff = ALLOCA_N(char, RSTRING_LEN(str) + 1);
	strcpy(buff, RSTRING_PTR(str));
	p = strchr(buff, '\n');
	if (p) *p = '\0';
	MSG(buff);
    }
    else
    {
	MSG("");
    }
    return Qnil;
}

static VALUE vim_set_option(VALUE self UNUSED, VALUE str)
{
    do_set((char_u *)StringValuePtr(str), 0);
    update_screen(NOT_VALID);
    return Qnil;
}

static VALUE vim_command(VALUE self UNUSED, VALUE str)
{
    do_cmdline_cmd((char_u *)StringValuePtr(str));
    return Qnil;
}

#ifdef FEAT_EVAL
static VALUE vim_to_ruby(typval_T *tv)
{
    VALUE result = Qnil;

    if (tv->v_type == VAR_STRING)
    {
	result = rb_str_new2(tv->vval.v_string == NULL
					  ? "" : (char *)(tv->vval.v_string));
    }
    else if (tv->v_type == VAR_NUMBER)
    {
	result = INT2NUM(tv->vval.v_number);
    }
# ifdef FEAT_FLOAT
    else if (tv->v_type == VAR_FLOAT)
    {
	result = rb_float_new(tv->vval.v_float);
    }
# endif
    else if (tv->v_type == VAR_LIST)
    {
	list_T      *list = tv->vval.v_list;
	listitem_T  *curr;

	result = rb_ary_new();

	if (list != NULL)
	{
	    for (curr = list->lv_first; curr != NULL; curr = curr->li_next)
	    {
		rb_ary_push(result, vim_to_ruby(&curr->li_tv));
	    }
	}
    }
    else if (tv->v_type == VAR_DICT)
    {
	result = rb_hash_new();

	if (tv->vval.v_dict != NULL)
	{
	    hashtab_T   *ht = &tv->vval.v_dict->dv_hashtab;
	    long_u      todo = ht->ht_used;
	    hashitem_T  *hi;
	    dictitem_T  *di;

	    for (hi = ht->ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;

		    di = dict_lookup(hi);
		    rb_hash_aset(result, rb_str_new2((char *)hi->hi_key),
						     vim_to_ruby(&di->di_tv));
		}
	    }
	}
    }
    else if (tv->v_type == VAR_SPECIAL)
    {
	if (tv->vval.v_number == VVAL_TRUE)
	    result = Qtrue;
	else if (tv->vval.v_number == VVAL_FALSE)
	    result = Qfalse;
    } /* else return Qnil; */

    return result;
}
#endif

static VALUE vim_evaluate(VALUE self UNUSED, VALUE str)
{
#ifdef FEAT_EVAL
    typval_T    *tv;
    VALUE       result;

    tv = eval_expr((char_u *)StringValuePtr(str), NULL);
    if (tv == NULL)
    {
	return Qnil;
    }
    result = vim_to_ruby(tv);

    free_tv(tv);

    return result;
#else
    return Qnil;
#endif
}

#ifdef USE_TYPEDDATA
static size_t buffer_dsize(const void *buf);

static const rb_data_type_t buffer_type = {
    "vim_buffer",
    {0, 0, buffer_dsize, {0, 0}},
    0, 0,
# ifdef RUBY_TYPED_FREE_IMMEDIATELY
    0,
# endif
};

static size_t buffer_dsize(const void *buf UNUSED)
{
    return sizeof(buf_T);
}
#endif

static VALUE buffer_new(buf_T *buf)
{
    if (buf->b_ruby_ref)
    {
	return (VALUE) buf->b_ruby_ref;
    }
    else
    {
#ifdef USE_TYPEDDATA
	VALUE obj = TypedData_Wrap_Struct(cBuffer, &buffer_type, buf);
#else
	VALUE obj = Data_Wrap_Struct(cBuffer, 0, 0, buf);
#endif
	buf->b_ruby_ref = (void *) obj;
	rb_hash_aset(objtbl, rb_obj_id(obj), obj);
	return obj;
    }
}

static buf_T *get_buf(VALUE obj)
{
    buf_T *buf;

#ifdef USE_TYPEDDATA
    TypedData_Get_Struct(obj, buf_T, &buffer_type, buf);
#else
    Data_Get_Struct(obj, buf_T, buf);
#endif
    if (buf == NULL)
	rb_raise(eDeletedBufferError, "attempt to refer to deleted buffer");
    return buf;
}

static VALUE buffer_s_current(void)
{
    return buffer_new(curbuf);
}

static VALUE buffer_s_count(void)
{
    buf_T *b;
    int n = 0;

    FOR_ALL_BUFFERS(b)
    {
	/*  Deleted buffers should not be counted
	 *    SegPhault - 01/07/05 */
	if (b->b_p_bl)
	    n++;
    }

    return INT2NUM(n);
}

static VALUE buffer_s_aref(VALUE self UNUSED, VALUE num)
{
    buf_T *b;
    int n = NUM2INT(num);

    FOR_ALL_BUFFERS(b)
    {
	/*  Deleted buffers should not be counted
	 *    SegPhault - 01/07/05 */
	if (!b->b_p_bl)
	    continue;

	if (n == 0)
	    return buffer_new(b);

	n--;
    }
    return Qnil;
}

static VALUE buffer_name(VALUE self)
{
    buf_T *buf = get_buf(self);

    return buf->b_ffname ? rb_str_new2((char *)buf->b_ffname) : Qnil;
}

static VALUE buffer_number(VALUE self)
{
    buf_T *buf = get_buf(self);

    return INT2NUM(buf->b_fnum);
}

static VALUE buffer_count(VALUE self)
{
    buf_T *buf = get_buf(self);

    return INT2NUM(buf->b_ml.ml_line_count);
}

static VALUE get_buffer_line(buf_T *buf, linenr_T n)
{
    if (n <= 0 || n > buf->b_ml.ml_line_count)
	rb_raise(rb_eIndexError, "line number %ld out of range", (long)n);
    return vim_str2rb_enc_str((char *)ml_get_buf(buf, n, FALSE));
}

static VALUE buffer_aref(VALUE self, VALUE num)
{
    buf_T *buf = get_buf(self);

    if (buf != NULL)
	return get_buffer_line(buf, (linenr_T)NUM2LONG(num));
    return Qnil; /* For stop warning */
}

static VALUE set_buffer_line(buf_T *buf, linenr_T n, VALUE str)
{
    char	*line = StringValuePtr(str);
    aco_save_T	aco;

    if (n > 0 && n <= buf->b_ml.ml_line_count && line != NULL)
    {
	/* set curwin/curbuf for "buf" and save some things */
	aucmd_prepbuf(&aco, buf);

	if (u_savesub(n) == OK)
	{
	    ml_replace(n, (char_u *)line, TRUE);
	    changed();
#ifdef SYNTAX_HL
	    syn_changed(n); /* recompute syntax hl. for this line */
#endif
	}

	/* restore curwin/curbuf and a few other things */
	aucmd_restbuf(&aco);
	/* Careful: autocommands may have made "buf" invalid! */

	update_curbuf(NOT_VALID);
    }
    else
    {
	rb_raise(rb_eIndexError, "line number %ld out of range", (long)n);
    }
    return str;
}

static VALUE buffer_aset(VALUE self, VALUE num, VALUE str)
{
    buf_T *buf = get_buf(self);

    if (buf != NULL)
	return set_buffer_line(buf, (linenr_T)NUM2LONG(num), str);
    return str;
}

static VALUE buffer_delete(VALUE self, VALUE num)
{
    buf_T	*buf = get_buf(self);
    long	n = NUM2LONG(num);
    aco_save_T	aco;

    if (n > 0 && n <= buf->b_ml.ml_line_count)
    {
	/* set curwin/curbuf for "buf" and save some things */
	aucmd_prepbuf(&aco, buf);

	if (u_savedel(n, 1) == OK)
	{
	    ml_delete(n, 0);

	    /* Changes to non-active buffers should properly refresh
	     *   SegPhault - 01/09/05 */
	    deleted_lines_mark(n, 1L);

	    changed();
	}

	/* restore curwin/curbuf and a few other things */
	aucmd_restbuf(&aco);
	/* Careful: autocommands may have made "buf" invalid! */

	update_curbuf(NOT_VALID);
    }
    else
    {
	rb_raise(rb_eIndexError, "line number %ld out of range", n);
    }
    return Qnil;
}

static VALUE buffer_append(VALUE self, VALUE num, VALUE str)
{
    buf_T	*buf = get_buf(self);
    char	*line = StringValuePtr(str);
    long	n = NUM2LONG(num);
    aco_save_T	aco;

    if (line == NULL)
    {
	rb_raise(rb_eIndexError, "NULL line");
    }
    else if (n >= 0 && n <= buf->b_ml.ml_line_count)
    {
	/* set curwin/curbuf for "buf" and save some things */
	aucmd_prepbuf(&aco, buf);

	if (u_inssub(n + 1) == OK)
	{
	    ml_append(n, (char_u *) line, (colnr_T) 0, FALSE);

	    /*  Changes to non-active buffers should properly refresh screen
	     *    SegPhault - 12/20/04 */
	    appended_lines_mark(n, 1L);

	    changed();
	}

	/* restore curwin/curbuf and a few other things */
	aucmd_restbuf(&aco);
	/* Careful: autocommands may have made "buf" invalid! */

	update_curbuf(NOT_VALID);
    }
    else
    {
	rb_raise(rb_eIndexError, "line number %ld out of range", n);
    }
    return str;
}

#ifdef USE_TYPEDDATA
static size_t window_dsize(const void *buf);

static const rb_data_type_t window_type = {
    "vim_window",
    {0, 0, window_dsize, {0, 0}},
    0, 0,
# ifdef RUBY_TYPED_FREE_IMMEDIATELY
    0,
# endif
};

static size_t window_dsize(const void *win UNUSED)
{
    return sizeof(win_T);
}
#endif

static VALUE window_new(win_T *win)
{
    if (win->w_ruby_ref)
    {
	return (VALUE) win->w_ruby_ref;
    }
    else
    {
#ifdef USE_TYPEDDATA
	VALUE obj = TypedData_Wrap_Struct(cVimWindow, &window_type, win);
#else
	VALUE obj = Data_Wrap_Struct(cVimWindow, 0, 0, win);
#endif
	win->w_ruby_ref = (void *) obj;
	rb_hash_aset(objtbl, rb_obj_id(obj), obj);
	return obj;
    }
}

static win_T *get_win(VALUE obj)
{
    win_T *win;

#ifdef USE_TYPEDDATA
    TypedData_Get_Struct(obj, win_T, &window_type, win);
#else
    Data_Get_Struct(obj, win_T, win);
#endif
    if (win == NULL)
	rb_raise(eDeletedWindowError, "attempt to refer to deleted window");
    return win;
}

static VALUE window_s_current(void)
{
    return window_new(curwin);
}

/*
 * Added line manipulation functions
 *    SegPhault - 03/07/05
 */
static VALUE line_s_current(void)
{
    return get_buffer_line(curbuf, curwin->w_cursor.lnum);
}

static VALUE set_current_line(VALUE self UNUSED, VALUE str)
{
    return set_buffer_line(curbuf, curwin->w_cursor.lnum, str);
}

static VALUE current_line_number(void)
{
    return INT2FIX((int)curwin->w_cursor.lnum);
}



static VALUE window_s_count(void)
{
    win_T	*w;
    int n = 0;

    FOR_ALL_WINDOWS(w)
	n++;
    return INT2NUM(n);
}

static VALUE window_s_aref(VALUE self UNUSED, VALUE num)
{
    win_T *w;
    int n = NUM2INT(num);

    for (w = firstwin; w != NULL; w = w->w_next, --n)
	if (n == 0)
	    return window_new(w);
    return Qnil;
}

static VALUE window_buffer(VALUE self)
{
    win_T *win = get_win(self);

    return buffer_new(win->w_buffer);
}

static VALUE window_height(VALUE self)
{
    win_T *win = get_win(self);

    return INT2NUM(win->w_height);
}

static VALUE window_set_height(VALUE self, VALUE height)
{
    win_T *win = get_win(self);
    win_T *savewin = curwin;

    curwin = win;
    win_setheight(NUM2INT(height));
    curwin = savewin;
    return height;
}

static VALUE window_width(VALUE self UNUSED)
{
    return INT2NUM(get_win(self)->w_width);
}

static VALUE window_set_width(VALUE self UNUSED, VALUE width)
{
    win_T *win = get_win(self);
    win_T *savewin = curwin;

    curwin = win;
    win_setwidth(NUM2INT(width));
    curwin = savewin;
    return width;
}

static VALUE window_cursor(VALUE self)
{
    win_T *win = get_win(self);

    return rb_assoc_new(INT2NUM(win->w_cursor.lnum), INT2NUM(win->w_cursor.col));
}

static VALUE window_set_cursor(VALUE self, VALUE pos)
{
    VALUE lnum, col;
    win_T *win = get_win(self);

    Check_Type(pos, T_ARRAY);
    if (RARRAY_LEN(pos) != 2)
	rb_raise(rb_eArgError, "array length must be 2");
    lnum = RARRAY_PTR(pos)[0];
    col = RARRAY_PTR(pos)[1];
    win->w_cursor.lnum = NUM2LONG(lnum);
    win->w_cursor.col = NUM2UINT(col);
    win->w_set_curswant = TRUE;
    check_cursor();		    /* put cursor on an existing line */
    update_screen(NOT_VALID);
    return Qnil;
}

static VALUE f_nop(VALUE self UNUSED)
{
    return Qnil;
}

static VALUE f_p(int argc, VALUE *argv, VALUE self UNUSED)
{
    int i;
    VALUE str = rb_str_new("", 0);
    VALUE ret = Qnil;

    for (i = 0; i < argc; i++)
    {
	if (i > 0) rb_str_cat(str, ", ", 2);
	rb_str_concat(str, rb_inspect(argv[i]));
    }
    MSG(RSTRING_PTR(str));

    if (argc == 1)
	ret = argv[0];
    else if (argc > 1)
	ret = rb_ary_new4(argc, argv);
    return ret;
}

static void ruby_io_init(void)
{
#ifndef DYNAMIC_RUBY
    RUBYEXTERN VALUE rb_stdout;
    RUBYEXTERN VALUE rb_stderr;
#endif

    rb_stdout = rb_obj_alloc(rb_cObject);
    rb_stderr = rb_obj_alloc(rb_cObject);
    rb_define_singleton_method(rb_stdout, "write", vim_message, 1);
    rb_define_singleton_method(rb_stdout, "flush", f_nop, 0);
    rb_define_singleton_method(rb_stderr, "write", vim_message, 1);
    rb_define_singleton_method(rb_stderr, "flush", f_nop, 0);
    rb_define_global_function("p", f_p, -1);
}

static void ruby_vim_init(void)
{
    objtbl = rb_hash_new();
    rb_global_variable(&objtbl);

    /* The Vim module used to be called "VIM", but "Vim" is better.  Make an
     * alias "VIM" for backwards compatibility. */
    mVIM = rb_define_module("Vim");
    rb_define_const(rb_cObject, "VIM", mVIM);
    rb_define_const(mVIM, "VERSION_MAJOR", INT2NUM(VIM_VERSION_MAJOR));
    rb_define_const(mVIM, "VERSION_MINOR", INT2NUM(VIM_VERSION_MINOR));
    rb_define_const(mVIM, "VERSION_BUILD", INT2NUM(VIM_VERSION_BUILD));
    rb_define_const(mVIM, "VERSION_PATCHLEVEL", INT2NUM(VIM_VERSION_PATCHLEVEL));
    rb_define_const(mVIM, "VERSION_SHORT", rb_str_new2(VIM_VERSION_SHORT));
    rb_define_const(mVIM, "VERSION_MEDIUM", rb_str_new2(VIM_VERSION_MEDIUM));
    rb_define_const(mVIM, "VERSION_LONG", rb_str_new2(VIM_VERSION_LONG));
    rb_define_const(mVIM, "VERSION_LONG_DATE", rb_str_new2(VIM_VERSION_LONG_DATE));
    rb_define_module_function(mVIM, "message", vim_message, 1);
    rb_define_module_function(mVIM, "set_option", vim_set_option, 1);
    rb_define_module_function(mVIM, "command", vim_command, 1);
    rb_define_module_function(mVIM, "evaluate", vim_evaluate, 1);

    eDeletedBufferError = rb_define_class_under(mVIM, "DeletedBufferError",
						rb_eStandardError);
    eDeletedWindowError = rb_define_class_under(mVIM, "DeletedWindowError",
						rb_eStandardError);

    cBuffer = rb_define_class_under(mVIM, "Buffer", rb_cObject);
    rb_define_singleton_method(cBuffer, "current", buffer_s_current, 0);
    rb_define_singleton_method(cBuffer, "count", buffer_s_count, 0);
    rb_define_singleton_method(cBuffer, "[]", buffer_s_aref, 1);
    rb_define_method(cBuffer, "name", buffer_name, 0);
    rb_define_method(cBuffer, "number", buffer_number, 0);
    rb_define_method(cBuffer, "count", buffer_count, 0);
    rb_define_method(cBuffer, "length", buffer_count, 0);
    rb_define_method(cBuffer, "[]", buffer_aref, 1);
    rb_define_method(cBuffer, "[]=", buffer_aset, 2);
    rb_define_method(cBuffer, "delete", buffer_delete, 1);
    rb_define_method(cBuffer, "append", buffer_append, 2);

    /* Added line manipulation functions
     *   SegPhault - 03/07/05 */
    rb_define_method(cBuffer, "line_number", current_line_number, 0);
    rb_define_method(cBuffer, "line", line_s_current, 0);
    rb_define_method(cBuffer, "line=", set_current_line, 1);


    cVimWindow = rb_define_class_under(mVIM, "Window", rb_cObject);
    rb_define_singleton_method(cVimWindow, "current", window_s_current, 0);
    rb_define_singleton_method(cVimWindow, "count", window_s_count, 0);
    rb_define_singleton_method(cVimWindow, "[]", window_s_aref, 1);
    rb_define_method(cVimWindow, "buffer", window_buffer, 0);
    rb_define_method(cVimWindow, "height", window_height, 0);
    rb_define_method(cVimWindow, "height=", window_set_height, 1);
    rb_define_method(cVimWindow, "width", window_width, 0);
    rb_define_method(cVimWindow, "width=", window_set_width, 1);
    rb_define_method(cVimWindow, "cursor", window_cursor, 0);
    rb_define_method(cVimWindow, "cursor=", window_set_cursor, 1);

    rb_define_virtual_variable("$curbuf", buffer_s_current, 0);
    rb_define_virtual_variable("$curwin", window_s_current, 0);
}

void vim_ruby_init(void *stack_start)
{
    /* should get machine stack start address early in main function */
    ruby_stack_start = stack_start;
}
