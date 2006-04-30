/* vi:set ts=8 sts=4 sw=4:
 *
 * MzScheme interface by Sergey Khorev <sergey.khorev@gmail.com>
 * Original work by Brent Fulgham <bfulgham@debian.org>
 * (Based on lots of help from Matthew Flatt)
 *
 * This consists of six parts:
 * 1. MzScheme interpreter main program
 * 2. Routines that handle the external interface between MzScheme and
 *    Vim.
 * 3. MzScheme input/output handlers: writes output via [e]msg().
 * 4. Implementation of the Vim Features for MzScheme
 * 5. Vim Window-related Manipulation Functions.
 * 6. Vim Buffer-related Manipulation Functions
 *
 * NOTES
 * 1. Memory, allocated with scheme_malloc*, need not to be freed explicitly,
 *    garbage collector will do it self
 * 2. Requires at least NORMAL features. I can't imagine why one may want
 *    to build with SMALL or TINY features but with MzScheme interface.
 * 3. I don't use K&R-style functions. Anyway, MzScheme headers are ANSI.
 */

#include "vim.h"
#include "if_mzsch.h"

/* Only do the following when the feature is enabled.  Needed for "make
 * depend". */
#if defined(FEAT_MZSCHEME) || defined(PROTO)

/* Base data structures */
#define SCHEME_VIMBUFFERP(obj)  SAME_TYPE(SCHEME_TYPE(obj), mz_buffer_type)
#define SCHEME_VIMWINDOWP(obj)  SAME_TYPE(SCHEME_TYPE(obj), mz_window_type)

typedef struct
{
    Scheme_Type	    tag;
    Scheme_Env	    *env;
    buf_T	    *buf;
} vim_mz_buffer;

#define INVALID_BUFFER_VALUE ((buf_T *)(-1))

typedef struct
{
    Scheme_Type	    tag;
    win_T	    *win;
} vim_mz_window;

#define INVALID_WINDOW_VALUE ((win_T *)(-1))

/*
 * Prims that form MzScheme Vim interface
 */
typedef struct
{
    Scheme_Closed_Prim	*prim;
    char	*name;
    int		mina;	/* arity information */
    int		maxa;
} Vim_Prim;

typedef struct
{
    char	    *name;
    Scheme_Object   *port;
} Port_Info;

/* info for closed prim */
/*
 * data have different means:
 * for do_eval it is char*
 * for do_apply is Apply_Onfo*
 * for do_load is Port_Info*
 */
typedef struct
{
    void	*data;
    Scheme_Env	*env;
} Cmd_Info;

/* info for do_apply */
typedef struct
{
    Scheme_Object   *proc;
    int		    argc;
    Scheme_Object   **argv;
} Apply_Info;

/*
 *========================================================================
 *  Vim-Control Commands
 *========================================================================
 */
/*
 *========================================================================
 *  Utility functions for the vim/mzscheme interface
 *========================================================================
 */
#ifdef HAVE_SANDBOX
static Scheme_Object *sandbox_file_guard(int, Scheme_Object **);
static Scheme_Object *sandbox_network_guard(int, Scheme_Object **);
static void sandbox_check();
#endif
/*  Buffer-related commands */
static Scheme_Object *buffer_new(buf_T *buf);
static Scheme_Object *get_buffer_by_name(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_by_num(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_count(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_line(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_line_list(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_name(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_num(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_size(void *, int, Scheme_Object **);
static Scheme_Object *get_curr_buffer(void *, int, Scheme_Object **);
static Scheme_Object *get_next_buffer(void *, int, Scheme_Object **);
static Scheme_Object *get_prev_buffer(void *, int, Scheme_Object **);
static Scheme_Object *mzscheme_open_buffer(void *, int, Scheme_Object **);
static Scheme_Object *set_buffer_line(void *, int, Scheme_Object **);
static Scheme_Object *set_buffer_line_list(void *, int, Scheme_Object **);
static Scheme_Object *insert_buffer_line_list(void *, int, Scheme_Object **);
static Scheme_Object *get_range_start(void *, int, Scheme_Object **);
static Scheme_Object *get_range_end(void *, int, Scheme_Object **);
static Scheme_Object *get_buffer_namespace(void *, int, Scheme_Object **);
static vim_mz_buffer *get_vim_curr_buffer(void);

/*  Window-related commands */
static Scheme_Object *window_new(win_T *win);
static Scheme_Object *get_curr_win(void *, int, Scheme_Object **);
static Scheme_Object *get_window_count(void *, int, Scheme_Object **);
static Scheme_Object *get_window_by_num(void *, int, Scheme_Object **);
static Scheme_Object *get_window_num(void *, int, Scheme_Object **);
static Scheme_Object *get_window_buffer(void *, int, Scheme_Object **);
static Scheme_Object *get_window_height(void *, int, Scheme_Object **);
static Scheme_Object *set_window_height(void *, int, Scheme_Object **);
#ifdef FEAT_VERTSPLIT
static Scheme_Object *get_window_width(void *, int, Scheme_Object **);
static Scheme_Object *set_window_width(void *, int, Scheme_Object **);
#endif
static Scheme_Object *get_cursor(void *, int, Scheme_Object **);
static Scheme_Object *set_cursor(void *, int, Scheme_Object **);
static Scheme_Object *get_window_list(void *, int, Scheme_Object **);
static vim_mz_window *get_vim_curr_window(void);

/*  Vim-related commands */
static Scheme_Object *mzscheme_beep(void *, int, Scheme_Object **);
static Scheme_Object *get_option(void *, int, Scheme_Object **);
static Scheme_Object *set_option(void *, int, Scheme_Object **);
static Scheme_Object *vim_command(void *, int, Scheme_Object **);
static Scheme_Object *vim_eval(void *, int, Scheme_Object **);
static Scheme_Object *vim_bufferp(void *data, int, Scheme_Object **);
static Scheme_Object *vim_windowp(void *data, int, Scheme_Object **);
static Scheme_Object *vim_buffer_validp(void *data, int, Scheme_Object **);
static Scheme_Object *vim_window_validp(void *data, int, Scheme_Object **);

/*
 *========================================================================
 *  Internal Function Prototypes
 *========================================================================
 */
static int vim_error_check(void);
static int do_mzscheme_command(exarg_T *, void *, Scheme_Closed_Prim *what);
static void startup_mzscheme(void);
static char *string_to_line(Scheme_Object *obj);
static int mzscheme_io_init(void);
static void mzscheme_interface_init(vim_mz_buffer *self);
static void do_output(char *mesg, long len);
static void do_printf(char *format, ...);
static void do_flush(void);
static Scheme_Object *_apply_thunk_catch_exceptions(
	Scheme_Object *, Scheme_Object **);
static Scheme_Object *extract_exn_message(Scheme_Object *v);
static Scheme_Object *do_eval(void *, int noargc, Scheme_Object **noargv);
static Scheme_Object *do_load(void *, int noargc, Scheme_Object **noargv);
static Scheme_Object *do_apply(void *, int noargc, Scheme_Object **noargv);
static void register_vim_exn(Scheme_Env *env);
static vim_mz_buffer *get_buffer_arg(const char *fname, int argnum,
	int argc, Scheme_Object **argv);
static vim_mz_window *get_window_arg(const char *fname, int argnum,
	int argc, Scheme_Object **argv);
static void add_vim_exn(Scheme_Env *env);
static int line_in_range(linenr_T, buf_T *);
static void check_line_range(linenr_T, buf_T *);
static void mz_fix_cursor(int lo, int hi, int extra);

static int eval_in_namespace(void *, Scheme_Closed_Prim *, Scheme_Env *,
		Scheme_Object **ret);
static void make_modules(Scheme_Env *);

#ifdef DYNAMIC_MZSCHEME

static Scheme_Object *dll_scheme_eof;
static Scheme_Object *dll_scheme_false;
static Scheme_Object *dll_scheme_void;
static Scheme_Object *dll_scheme_null;
static Scheme_Object *dll_scheme_true;

static Scheme_Thread **dll_scheme_current_thread_ptr;

static void (**dll_scheme_console_printf_ptr)(char *str, ...);
static void (**dll_scheme_console_output_ptr)(char *str, long len);
static void (**dll_scheme_notify_multithread_ptr)(int on);

static void *(*dll_GC_malloc)(size_t size_in_bytes);
static void *(*dll_GC_malloc_atomic)(size_t size_in_bytes);
static Scheme_Env *(*dll_scheme_basic_env)(void);
static void (*dll_scheme_check_threads)(void);
static void (*dll_scheme_register_static)(void *ptr, long size);
static void (*dll_scheme_set_stack_base)(void *base, int no_auto_statics);
static void (*dll_scheme_add_global)(const char *name, Scheme_Object *val,
	Scheme_Env *env);
static void (*dll_scheme_add_global_symbol)(Scheme_Object *name,
	Scheme_Object *val, Scheme_Env *env);
static Scheme_Object *(*dll_scheme_apply)(Scheme_Object *rator, int num_rands,
	Scheme_Object **rands);
static Scheme_Object *(*dll_scheme_builtin_value)(const char *name);
# if MZSCHEME_VERSION_MAJOR >= 299
static Scheme_Object *(*dll_scheme_byte_string_to_char_string)(Scheme_Object *s);
# endif
static void (*dll_scheme_close_input_port)(Scheme_Object *port);
static void (*dll_scheme_count_lines)(Scheme_Object *port);
static Scheme_Object *(*dll_scheme_current_continuation_marks)(void);
static void (*dll_scheme_display)(Scheme_Object *obj, Scheme_Object *port);
static char *(*dll_scheme_display_to_string)(Scheme_Object *obj, long *len);
static int (*dll_scheme_eq)(Scheme_Object *obj1, Scheme_Object *obj2);
static Scheme_Object *(*dll_scheme_do_eval)(Scheme_Object *obj,
	int _num_rands, Scheme_Object **rands, int val);
static void (*dll_scheme_dont_gc_ptr)(void *p);
static Scheme_Object *(*dll_scheme_eval)(Scheme_Object *obj, Scheme_Env *env);
static Scheme_Object *(*dll_scheme_eval_string)(const char *str,
	Scheme_Env *env);
static Scheme_Object *(*dll_scheme_eval_string_all)(const char *str,
	Scheme_Env *env, int all);
static void (*dll_scheme_finish_primitive_module)(Scheme_Env *env);
# if MZSCHEME_VERSION_MAJOR < 299
static char *(*dll_scheme_format)(char *format, int flen, int argc,
	Scheme_Object **argv, long *rlen);
# else
static char *(*dll_scheme_format_utf8)(char *format, int flen, int argc,
	Scheme_Object **argv, long *rlen);
static Scheme_Object *(*dll_scheme_get_param)(Scheme_Config *c, int pos);
# endif
static void (*dll_scheme_gc_ptr_ok)(void *p);
# if MZSCHEME_VERSION_MAJOR < 299
static char *(*dll_scheme_get_sized_string_output)(Scheme_Object *,
	long *len);
# else
static char *(*dll_scheme_get_sized_byte_string_output)(Scheme_Object *,
	long *len);
# endif
static Scheme_Object *(*dll_scheme_intern_symbol)(const char *name);
static Scheme_Object *(*dll_scheme_lookup_global)(Scheme_Object *symbol,
	Scheme_Env *env);
static Scheme_Object *(*dll_scheme_make_closed_prim_w_arity)
    (Scheme_Closed_Prim *prim, void *data, const char *name, mzshort mina,
     mzshort maxa);
static Scheme_Object *(*dll_scheme_make_integer_value)(long i);
static Scheme_Object *(*dll_scheme_make_namespace)(int argc,
	Scheme_Object *argv[]);
static Scheme_Object *(*dll_scheme_make_pair)(Scheme_Object *car,
	Scheme_Object *cdr);
static Scheme_Object *(*dll_scheme_make_prim_w_arity)(Scheme_Prim *prim,
	const char *name, mzshort mina, mzshort maxa);
# if MZSCHEME_VERSION_MAJOR < 299
static Scheme_Object *(*dll_scheme_make_string)(const char *chars);
static Scheme_Object *(*dll_scheme_make_string_output_port)();
# else
static Scheme_Object *(*dll_scheme_make_byte_string)(const char *chars);
static Scheme_Object *(*dll_scheme_make_byte_string_output_port)();
# endif
static Scheme_Object *(*dll_scheme_make_struct_instance)(Scheme_Object *stype,
	int argc, Scheme_Object **argv);
static Scheme_Object **(*dll_scheme_make_struct_names)(Scheme_Object *base,
	Scheme_Object *field_names, int flags, int *count_out);
static Scheme_Object *(*dll_scheme_make_struct_type)(Scheme_Object *base,
	Scheme_Object *parent, Scheme_Object *inspector, int num_fields,
	int num_uninit_fields, Scheme_Object *uninit_val,
	Scheme_Object *properties
# if MZSCHEME_VERSION_MAJOR >= 299
	, Scheme_Object *guard
# endif
	);
static Scheme_Object **(*dll_scheme_make_struct_values)(
	Scheme_Object *struct_type, Scheme_Object **names, int count,
	int flags);
static Scheme_Type (*dll_scheme_make_type)(const char *name);
static Scheme_Object *(*dll_scheme_make_vector)(int size,
	Scheme_Object *fill);
static void *(*dll_scheme_malloc_fail_ok)(void *(*f)(size_t), size_t);
static Scheme_Object *(*dll_scheme_open_input_file)(const char *name,
	const char *who);
static Scheme_Env *(*dll_scheme_primitive_module)(Scheme_Object *name,
	Scheme_Env *for_env);
static int (*dll_scheme_proper_list_length)(Scheme_Object *list);
static void (*dll_scheme_raise)(Scheme_Object *exn);
static Scheme_Object *(*dll_scheme_read)(Scheme_Object *port);
static void (*dll_scheme_signal_error)(const char *msg, ...);
static void (*dll_scheme_wrong_type)(const char *name, const char *expected,
	int which, int argc, Scheme_Object **argv);
# if MZSCHEME_VERSION_MAJOR >= 299
static void (*dll_scheme_set_param)(Scheme_Config *c, int pos,
	Scheme_Object *o);
static Scheme_Config *(*dll_scheme_current_config)(void);
static Scheme_Object *(*dll_scheme_char_string_to_byte_string)
    (Scheme_Object *s);
# endif

/* arrays are imported directly */
# define scheme_eof dll_scheme_eof
# define scheme_false dll_scheme_false
# define scheme_void dll_scheme_void
# define scheme_null dll_scheme_null
# define scheme_true dll_scheme_true

/* pointers are GetProceAddress'ed as pointers to pointer */
# define scheme_current_thread (*dll_scheme_current_thread_ptr)
# define scheme_console_printf (*dll_scheme_console_printf_ptr)
# define scheme_console_output (*dll_scheme_console_output_ptr)
# define scheme_notify_multithread (*dll_scheme_notify_multithread_ptr)

/* and functions in a usual way */
# define GC_malloc dll_GC_malloc
# define GC_malloc_atomic dll_GC_malloc_atomic

# define scheme_add_global dll_scheme_add_global
# define scheme_add_global_symbol dll_scheme_add_global_symbol
# define scheme_apply dll_scheme_apply
# define scheme_basic_env dll_scheme_basic_env
# define scheme_builtin_value dll_scheme_builtin_value
# if MZSCHEME_VERSION_MAJOR >= 299
#  define scheme_byte_string_to_char_string dll_scheme_byte_string_to_char_string
# endif
# define scheme_check_threads dll_scheme_check_threads
# define scheme_close_input_port dll_scheme_close_input_port
# define scheme_count_lines dll_scheme_count_lines
# define scheme_current_continuation_marks \
    dll_scheme_current_continuation_marks
# define scheme_display dll_scheme_display
# define scheme_display_to_string dll_scheme_display_to_string
# define scheme_do_eval dll_scheme_do_eval
# define scheme_dont_gc_ptr dll_scheme_dont_gc_ptr
# define scheme_eq dll_scheme_eq
# define scheme_eval dll_scheme_eval
# define scheme_eval_string dll_scheme_eval_string
# define scheme_eval_string_all dll_scheme_eval_string_all
# define scheme_finish_primitive_module dll_scheme_finish_primitive_module
# if MZSCHEME_VERSION_MAJOR < 299
#  define scheme_format dll_scheme_format
# else
#  define scheme_format_utf8 dll_scheme_format_utf8
# endif
# define scheme_gc_ptr_ok dll_scheme_gc_ptr_ok
# if MZSCHEME_VERSION_MAJOR < 299
#  define scheme_get_sized_string_output dll_scheme_get_sized_string_output
# else
#  define scheme_get_sized_byte_string_output \
    dll_scheme_get_sized_byte_string_output
# define scheme_get_param dll_scheme_get_param
# endif
# define scheme_intern_symbol dll_scheme_intern_symbol
# define scheme_lookup_global dll_scheme_lookup_global
# define scheme_make_closed_prim_w_arity dll_scheme_make_closed_prim_w_arity
# define scheme_make_integer_value dll_scheme_make_integer_value
# define scheme_make_namespace dll_scheme_make_namespace
# define scheme_make_pair dll_scheme_make_pair
# define scheme_make_prim_w_arity dll_scheme_make_prim_w_arity
# if MZSCHEME_VERSION_MAJOR < 299
#  define scheme_make_string dll_scheme_make_string
#  define scheme_make_string_output_port dll_scheme_make_string_output_port
# else
#  define scheme_make_byte_string dll_scheme_make_byte_string
#  define scheme_make_byte_string_output_port \
    dll_scheme_make_byte_string_output_port
# endif
# define scheme_make_struct_instance dll_scheme_make_struct_instance
# define scheme_make_struct_names dll_scheme_make_struct_names
# define scheme_make_struct_type dll_scheme_make_struct_type
# define scheme_make_struct_values dll_scheme_make_struct_values
# define scheme_make_type dll_scheme_make_type
# define scheme_make_vector dll_scheme_make_vector
# define scheme_malloc_fail_ok dll_scheme_malloc_fail_ok
# define scheme_open_input_file dll_scheme_open_input_file
# define scheme_primitive_module dll_scheme_primitive_module
# define scheme_proper_list_length dll_scheme_proper_list_length
# define scheme_raise dll_scheme_raise
# define scheme_read dll_scheme_read
# define scheme_register_static dll_scheme_register_static
# define scheme_set_stack_base dll_scheme_set_stack_base
# define scheme_signal_error dll_scheme_signal_error
# define scheme_wrong_type dll_scheme_wrong_type
# if MZSCHEME_VERSION_MAJOR >= 299
#  define scheme_set_param dll_scheme_set_param
#  define scheme_current_config dll_scheme_current_config
#  define scheme_char_string_to_byte_string \
    dll_scheme_char_string_to_byte_string
# endif

typedef struct
{
    char    *name;
    void    **ptr;
} Thunk_Info;

static Thunk_Info mzgc_imports[] = {
    {"GC_malloc", (void **)&dll_GC_malloc},
    {"GC_malloc_atomic", (void **)&dll_GC_malloc_atomic},
    {NULL, NULL}};

static Thunk_Info mzsch_imports[] = {
    {"scheme_eof", (void **)&dll_scheme_eof},
    {"scheme_false", (void **)&dll_scheme_false},
    {"scheme_void", (void **)&dll_scheme_void},
    {"scheme_null", (void **)&dll_scheme_null},
    {"scheme_true", (void **)&dll_scheme_true},
    {"scheme_current_thread", (void **)&dll_scheme_current_thread_ptr},
    {"scheme_console_printf", (void **)&dll_scheme_console_printf_ptr},
    {"scheme_console_output", (void **)&dll_scheme_console_output_ptr},
    {"scheme_notify_multithread",
	(void **)&dll_scheme_notify_multithread_ptr},
    {"scheme_add_global", (void **)&dll_scheme_add_global},
    {"scheme_add_global_symbol", (void **)&dll_scheme_add_global_symbol},
    {"scheme_apply", (void **)&dll_scheme_apply},
    {"scheme_basic_env", (void **)&dll_scheme_basic_env},
# if MZSCHEME_VERSION_MAJOR >= 299
    {"scheme_byte_string_to_char_string", (void **)&dll_scheme_byte_string_to_char_string},
# endif
    {"scheme_builtin_value", (void **)&dll_scheme_builtin_value},
    {"scheme_check_threads", (void **)&dll_scheme_check_threads},
    {"scheme_close_input_port", (void **)&dll_scheme_close_input_port},
    {"scheme_count_lines", (void **)&dll_scheme_count_lines},
    {"scheme_current_continuation_marks",
	(void **)&dll_scheme_current_continuation_marks},
    {"scheme_display", (void **)&dll_scheme_display},
    {"scheme_display_to_string", (void **)&dll_scheme_display_to_string},
    {"scheme_do_eval", (void **)&dll_scheme_do_eval},
    {"scheme_dont_gc_ptr", (void **)&dll_scheme_dont_gc_ptr},
    {"scheme_eq", (void **)&dll_scheme_eq},
    {"scheme_eval", (void **)&dll_scheme_eval},
    {"scheme_eval_string", (void **)&dll_scheme_eval_string},
    {"scheme_eval_string_all", (void **)&dll_scheme_eval_string_all},
    {"scheme_finish_primitive_module",
	(void **)&dll_scheme_finish_primitive_module},
# if MZSCHEME_VERSION_MAJOR < 299
    {"scheme_format", (void **)&dll_scheme_format},
# else
    {"scheme_format_utf8", (void **)&dll_scheme_format_utf8},
    {"scheme_get_param", (void **)&dll_scheme_get_param},
#endif
    {"scheme_gc_ptr_ok", (void **)&dll_scheme_gc_ptr_ok},
# if MZSCHEME_VERSION_MAJOR < 299
    {"scheme_get_sized_string_output",
	(void **)&dll_scheme_get_sized_string_output},
# else
    {"scheme_get_sized_byte_string_output",
	(void **)&dll_scheme_get_sized_byte_string_output},
#endif
    {"scheme_intern_symbol", (void **)&dll_scheme_intern_symbol},
    {"scheme_lookup_global", (void **)&dll_scheme_lookup_global},
    {"scheme_make_closed_prim_w_arity",
	(void **)&dll_scheme_make_closed_prim_w_arity},
    {"scheme_make_integer_value", (void **)&dll_scheme_make_integer_value},
    {"scheme_make_namespace", (void **)&dll_scheme_make_namespace},
    {"scheme_make_pair", (void **)&dll_scheme_make_pair},
    {"scheme_make_prim_w_arity", (void **)&dll_scheme_make_prim_w_arity},
# if MZSCHEME_VERSION_MAJOR < 299
    {"scheme_make_string", (void **)&dll_scheme_make_string},
    {"scheme_make_string_output_port",
	(void **)&dll_scheme_make_string_output_port},
# else
    {"scheme_make_byte_string", (void **)&dll_scheme_make_byte_string},
    {"scheme_make_byte_string_output_port",
	(void **)&dll_scheme_make_byte_string_output_port},
# endif
    {"scheme_make_struct_instance",
	(void **)&dll_scheme_make_struct_instance},
    {"scheme_make_struct_names", (void **)&dll_scheme_make_struct_names},
    {"scheme_make_struct_type", (void **)&dll_scheme_make_struct_type},
    {"scheme_make_struct_values", (void **)&dll_scheme_make_struct_values},
    {"scheme_make_type", (void **)&dll_scheme_make_type},
    {"scheme_make_vector", (void **)&dll_scheme_make_vector},
    {"scheme_malloc_fail_ok", (void **)&dll_scheme_malloc_fail_ok},
    {"scheme_open_input_file", (void **)&dll_scheme_open_input_file},
    {"scheme_primitive_module", (void **)&dll_scheme_primitive_module},
    {"scheme_proper_list_length", (void **)&dll_scheme_proper_list_length},
    {"scheme_raise", (void **)&dll_scheme_raise},
    {"scheme_read", (void **)&dll_scheme_read},
    {"scheme_register_static", (void **)&dll_scheme_register_static},
    {"scheme_set_stack_base", (void **)&dll_scheme_set_stack_base},
    {"scheme_signal_error", (void **)&dll_scheme_signal_error},
    {"scheme_wrong_type", (void **)&dll_scheme_wrong_type},
# if MZSCHEME_VERSION_MAJOR >= 299
    {"scheme_set_param", (void **)&dll_scheme_set_param},
    {"scheme_current_config", (void **)&dll_scheme_current_config},
    {"scheme_char_string_to_byte_string",
	(void **)&dll_scheme_char_string_to_byte_string},
# endif
    {NULL, NULL}};

static HINSTANCE hMzGC = 0;
static HINSTANCE hMzSch = 0;

static void dynamic_mzscheme_end(void);
static int mzscheme_runtime_link_init(char *sch_dll, char *gc_dll,
	int verbose);

    static int
mzscheme_runtime_link_init(char *sch_dll, char *gc_dll, int verbose)
{
    Thunk_Info *thunk = NULL;

    if (hMzGC && hMzSch)
	return OK;
    hMzSch = LoadLibrary(sch_dll);
    hMzGC = LoadLibrary(gc_dll);

    if (!hMzSch)
    {
	if (verbose)
	    EMSG2(_(e_loadlib), sch_dll);
	return FAIL;
    }

    if (!hMzGC)
    {
	if (verbose)
	    EMSG2(_(e_loadlib), gc_dll);
	return FAIL;
    }

    for (thunk = mzsch_imports; thunk->name; thunk++)
    {
	if ((*thunk->ptr =
		    (void *)GetProcAddress(hMzSch, thunk->name)) == NULL)
	{
	    FreeLibrary(hMzSch);
	    hMzSch = 0;
	    FreeLibrary(hMzGC);
	    hMzGC = 0;
	    if (verbose)
		EMSG2(_(e_loadfunc), thunk->name);
	    return FAIL;
	}
    }
    for (thunk = mzgc_imports; thunk->name; thunk++)
    {
	if ((*thunk->ptr =
		    (void *)GetProcAddress(hMzGC, thunk->name)) == NULL)
	{
	    FreeLibrary(hMzSch);
	    hMzSch = 0;
	    FreeLibrary(hMzGC);
	    hMzGC = 0;
	    if (verbose)
		EMSG2(_(e_loadfunc), thunk->name);
	    return FAIL;
	}
    }
    return OK;
}

    int
mzscheme_enabled(int verbose)
{
    return mzscheme_runtime_link_init(
	    DYNAMIC_MZSCH_DLL, DYNAMIC_MZGC_DLL, verbose) == OK;
}

    static void
dynamic_mzscheme_end(void)
{
    if (hMzSch)
    {
	FreeLibrary(hMzSch);
	hMzSch = 0;
    }
    if (hMzGC)
    {
	FreeLibrary(hMzGC);
	hMzGC = 0;
    }
}
#endif /* DYNAMIC_MZSCHEME */

/*
 *========================================================================
 *  1. MzScheme interpreter startup
 *========================================================================
 */

static Scheme_Type mz_buffer_type;
static Scheme_Type mz_window_type;

static int initialized = 0;

/* global environment */
static Scheme_Env    *environment = NULL;
/* output/error handlers */
static Scheme_Object *curout = NULL;
static Scheme_Object *curerr = NULL;
/* vim:exn exception */
static Scheme_Object *exn_catching_apply = NULL;
static Scheme_Object *exn_p = NULL;
static Scheme_Object *exn_message = NULL;
static Scheme_Object *vim_exn = NULL; /* Vim Error exception */
 /* values for exn:vim - constructor, predicate, accessors etc */
static Scheme_Object *vim_exn_names = NULL;
static Scheme_Object *vim_exn_values = NULL;

static long range_start;
static long range_end;

/* MzScheme threads scheduling stuff */
static int mz_threads_allow = 0;

#if defined(FEAT_GUI_W32)
static void CALLBACK timer_proc(HWND, UINT, UINT, DWORD);
static UINT timer_id = 0;
#elif defined(FEAT_GUI_GTK)
static gint timer_proc(gpointer);
static guint timer_id = 0;
#elif defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)
static void timer_proc(XtPointer, XtIntervalId *);
static XtIntervalId timer_id = (XtIntervalId)0;
#elif defined(FEAT_GUI_MAC)
pascal void timer_proc(EventLoopTimerRef, void *);
static EventLoopTimerRef timer_id = NULL;
static EventLoopTimerUPP timerUPP;
#endif

#ifndef FEAT_GUI_W32 /* Win32 console and Unix */
    void
mzvim_check_threads(void)
{
    /* Last time MzScheme threads were scheduled */
    static time_t mz_last_time = 0;

    if (mz_threads_allow && p_mzq > 0)
    {
	time_t now = time(NULL);

	if ((now - mz_last_time) * 1000 > p_mzq)
	{
	    mz_last_time = now;
	    scheme_check_threads();
	}
    }
}
#endif

#ifdef MZSCHEME_GUI_THREADS
static void setup_timer(void);
static void remove_timer(void);

/* timers are presented in GUI only */
# if defined(FEAT_GUI_W32)
    static void CALLBACK
timer_proc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
# elif defined(FEAT_GUI_GTK)
/*ARGSUSED*/
    static gint
timer_proc(gpointer data)
# elif defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)
/* ARGSUSED */
    static void
timer_proc(XtPointer timed_out, XtIntervalId *interval_id)
# elif defined(FEAT_GUI_MAC)
    pascal void
timer_proc(EventLoopTimerRef theTimer, void *userData)
# endif
{
    scheme_check_threads();
# if defined(FEAT_GUI_GTK)
    return TRUE; /* continue receiving notifications */
# elif defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)
    /* renew timeout */
    if (mz_threads_allow && p_mzq > 0)
	timer_id = XtAppAddTimeOut(app_context, p_mzq,
		timer_proc, NULL);
# endif
}

    static void
setup_timer(void)
{
# if defined(FEAT_GUI_W32)
    timer_id = SetTimer(NULL, 0, p_mzq, timer_proc);
# elif defined(FEAT_GUI_GTK)
    timer_id = gtk_timeout_add((guint32)p_mzq, (GtkFunction)timer_proc, NULL);
# elif defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)
    timer_id = XtAppAddTimeOut(app_context, p_mzq, timer_proc, NULL);
# elif defined(FEAT_GUI_MAC)
    timerUPP = NewEventLoopTimerUPP(timer_proc);
    InstallEventLoopTimer(GetMainEventLoop(), p_mzq * kEventDurationMillisecond,
		p_mzq * kEventDurationMillisecond, timerUPP, NULL, &timer_id);
# endif
}

    static void
remove_timer(void)
{
# if defined(FEAT_GUI_W32)
    KillTimer(NULL, timer_id);
# elif defined(FEAT_GUI_GTK)
    gtk_timeout_remove(timer_id);
# elif defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA)
    XtRemoveTimeOut(timer_id);
# elif defined(FEAT_GUI_MAC)
    RemoveEventLoopTimer(timer_id);
    DisposeEventLoopTimerUPP(timerUPP);
# endif
    timer_id = 0;
}

    void
mzvim_reset_timer(void)
{
    if (timer_id != 0)
	remove_timer();
    if (mz_threads_allow && p_mzq > 0 && gui.in_use)
	setup_timer();
}

#endif /* MZSCHEME_GUI_THREADS */

    static void
notify_multithread(int on)
{
    mz_threads_allow = on;
#ifdef MZSCHEME_GUI_THREADS
    if (on && timer_id == 0 && p_mzq > 0 && gui.in_use)
	setup_timer();
    if (!on && timer_id != 0)
	remove_timer();
#endif
}

    void
mzscheme_end(void)
{
#ifdef DYNAMIC_MZSCHEME
    dynamic_mzscheme_end();
#endif
}

    static void
startup_mzscheme(void)
{
    Scheme_Object *proc_make_security_guard;

    scheme_set_stack_base(NULL, 1);

    MZ_REGISTER_STATIC(environment);
    MZ_REGISTER_STATIC(curout);
    MZ_REGISTER_STATIC(curerr);
    MZ_REGISTER_STATIC(exn_catching_apply);
    MZ_REGISTER_STATIC(exn_p);
    MZ_REGISTER_STATIC(exn_message);
    MZ_REGISTER_STATIC(vim_exn);
    MZ_REGISTER_STATIC(vim_exn_names);
    MZ_REGISTER_STATIC(vim_exn_values);

    environment = scheme_basic_env();

    /* redirect output */
    scheme_console_output = do_output;
    scheme_console_printf = do_printf;

#ifdef MZSCHEME_COLLECTS
    /* setup 'current-library-collection-paths' parameter */
    scheme_set_param(scheme_config, MZCONFIG_COLLECTION_PATHS,
	    scheme_make_pair(scheme_make_string(MZSCHEME_COLLECTS),
		scheme_null));
#endif
#ifdef HAVE_SANDBOX
    /* setup sandbox guards */
    proc_make_security_guard = scheme_lookup_global(
	    scheme_intern_symbol("make-security-guard"),
	    environment);
    if (proc_make_security_guard != NULL)
    {
	Scheme_Object *args[3];
	Scheme_Object *guard;
	args[0] = scheme_get_param(scheme_config, MZCONFIG_SECURITY_GUARD);
	args[1] = scheme_make_prim_w_arity(sandbox_file_guard,
		"sandbox-file-guard", 3, 3);
	args[2] = scheme_make_prim_w_arity(sandbox_network_guard,
		"sandbox-network-guard", 4, 4);
	guard = scheme_apply(proc_make_security_guard, 3, args);
	scheme_set_param(scheme_config, MZCONFIG_SECURITY_GUARD, guard);
    }
#endif
    /* Create buffer and window types for use in Scheme code */
    mz_buffer_type = scheme_make_type("<vim-buffer>");
    mz_window_type = scheme_make_type("<vim-window>");

    register_vim_exn(environment);
    make_modules(environment);

    /*
     * setup callback to receive notifications
     * whether thread scheduling is (or not) required
     */
    scheme_notify_multithread = notify_multithread;
    initialized = 1;
}

/*
 * This routine is called for each new invocation of MzScheme
 * to make sure things are properly initialized.
 */
    static int
mzscheme_init(void)
{
    int do_require = FALSE;

    if (!initialized)
    {
	do_require = TRUE;
#ifdef DYNAMIC_MZSCHEME
	if (!mzscheme_enabled(TRUE))
	{
	    EMSG(_("???: Sorry, this command is disabled, the MzScheme library could not be loaded."));
	    return -1;
	}
#endif
	startup_mzscheme();

	if (mzscheme_io_init())
	    return -1;

    }
    /* recreate ports each call effectivelly clearing these ones */
    curout = scheme_make_string_output_port();
    curerr = scheme_make_string_output_port();
    scheme_set_param(scheme_config, MZCONFIG_OUTPUT_PORT, curout);
    scheme_set_param(scheme_config, MZCONFIG_ERROR_PORT, curerr);

    if (do_require)
    {
	/* auto-instantiate in basic env */
	eval_in_namespace("(require (prefix vimext: vimext))", do_eval,
		environment, NULL);
    }

    return 0;
}

/*
 * This routine fills the namespace with various important routines that can
 * be used within MzScheme.
 */
    static void
mzscheme_interface_init(vim_mz_buffer *mzbuff)
{
    Scheme_Object   *attach;

    mzbuff->env = (Scheme_Env *)scheme_make_namespace(0, NULL);

    /*
     * attach instantiated modules from global namespace
     * so they can be easily instantiated in the buffer namespace
     */
    attach = scheme_lookup_global(
	    scheme_intern_symbol("namespace-attach-module"),
	    environment);

    if (attach != NULL)
    {
	Scheme_Object   *ret;
	Scheme_Object	*args[2];

	args[0] = (Scheme_Object *)environment;
	args[1] = scheme_intern_symbol("vimext");

	ret = (Scheme_Object *)mzvim_apply(attach, 2, args);
    }

    add_vim_exn(mzbuff->env);
}

/*
 *========================================================================
 *  2.  External Interface
 *========================================================================
 */

/*
 * Evaluate command in namespace with exception handling
 */
    static int
eval_in_namespace(void *data, Scheme_Closed_Prim *what, Scheme_Env *env,
		Scheme_Object **ret)
{
    Scheme_Object   *value;
    Scheme_Object   *exn;
    Cmd_Info	    info;   /* closure info */

    info.data = data;
    info.env = env;

    scheme_set_param(scheme_config, MZCONFIG_ENV,
	    (Scheme_Object *) env);
    /*
     * ensure all evaluations will be in current buffer namespace,
     * the second argument to scheme_eval_string isn't enough!
     */
    value = _apply_thunk_catch_exceptions(
	    scheme_make_closed_prim_w_arity(what, &info, "mzvim", 0, 0),
	    &exn);

    if (!value)
    {
	value = extract_exn_message(exn);
	/* Got an exn? */
	if (value)
	{
	    scheme_display(value, curerr);  /*  Send to stderr-vim */
	    do_flush();
	}
	/* `raise' was called on some arbitrary value */
	return FAIL;
    }

    if (ret != NULL)	/* if pointer to retval supported give it up */
	*ret = value;
    /* Print any result, as long as it's not a void */
    else if (!SCHEME_VOIDP(value))
	scheme_display(value, curout);  /* Send to stdout-vim */

    do_flush();
    return OK;
}

/* :mzscheme */
    static int
do_mzscheme_command(exarg_T *eap, void *data, Scheme_Closed_Prim *what)
{
    if (mzscheme_init())
	return FAIL;

    range_start = eap->line1;
    range_end = eap->line2;

    return eval_in_namespace(data, what, get_vim_curr_buffer()->env, NULL);
}

/*
 * Routine called by VIM when deleting a buffer
 */
    void
mzscheme_buffer_free(buf_T *buf)
{
    if (buf->b_mzscheme_ref)
    {
	vim_mz_buffer *bp;

	bp = buf->b_mzscheme_ref;
	bp->buf = INVALID_BUFFER_VALUE;
	buf->b_mzscheme_ref = NULL;
	scheme_gc_ptr_ok(bp);
    }
}

/*
 * Routine called by VIM when deleting a Window
 */
    void
mzscheme_window_free(win_T *win)
{
    if (win->w_mzscheme_ref)
    {
	vim_mz_window *wp;
	wp = win->w_mzscheme_ref;
	wp->win = INVALID_WINDOW_VALUE;
	win->w_mzscheme_ref = NULL;
	scheme_gc_ptr_ok(wp);
    }
}

/*
 * ":mzscheme" (or ":mz")
 */
    void
ex_mzscheme(exarg_T *eap)
{
    char_u	*script;

    script = script_get(eap, eap->arg);
    if (!eap->skip)
    {
	if (script == NULL)
	    do_mzscheme_command(eap, eap->arg, do_eval);
	else
	{
	    do_mzscheme_command(eap, script, do_eval);
	    vim_free(script);
	}
    }
}

/* eval MzScheme string */
    void *
mzvim_eval_string(char_u *str)
{
    Scheme_Object *ret = NULL;
    if (mzscheme_init())
	return FAIL;

    eval_in_namespace(str, do_eval, get_vim_curr_buffer()->env, &ret);
    return ret;
}

/*
 * apply MzScheme procedure with arguments,
 * handling errors
 */
    Scheme_Object *
mzvim_apply(Scheme_Object *proc, int argc, Scheme_Object **argv)
{
    Apply_Info	data;
    Scheme_Object *ret = NULL;

    if (mzscheme_init())
	return FAIL;

    data.proc = proc;
    data.argc = argc;
    data.argv = argv;

    eval_in_namespace(&data, do_apply, get_vim_curr_buffer()->env, &ret);
    return ret;
}

    static Scheme_Object *
do_load(void *data, int noargc, Scheme_Object **noargv)
{
    Cmd_Info	    *info = (Cmd_Info *)data;
    Scheme_Object   *result = scheme_void;
    Scheme_Object   *expr;
    char_u	    *file = scheme_malloc_fail_ok(
					  scheme_malloc_atomic, MAXPATHL + 1);
    Port_Info	    *pinfo = (Port_Info *)(info->data);

    /* make Vim expansion */
    expand_env((char_u *)pinfo->name, file, MAXPATHL);
    /* scheme_load looks strange working with namespaces and error handling*/
    pinfo->port = scheme_open_input_file(file, "mzfile");
    scheme_count_lines(pinfo->port); /* to get accurate read error location*/

    /* Like REPL but print only last result */
    while (!SCHEME_EOFP(expr = scheme_read(pinfo->port)))
	result = scheme_eval(expr, info->env);

    /* errors will be caught in do_mzscheme_comamnd and ex_mzfile */
    scheme_close_input_port(pinfo->port);
    pinfo->port = NULL;
    return result;
}

/* :mzfile */
    void
ex_mzfile(exarg_T *eap)
{
    Port_Info	pinfo;

    pinfo.name = (char *)eap->arg;
    pinfo.port = NULL;
    if (do_mzscheme_command(eap, &pinfo, do_load) != OK
	    && pinfo.port != NULL)	/* looks like port was not closed */
	scheme_close_input_port(pinfo.port);
}


/*
 *========================================================================
 * Exception handling code -- cribbed form the MzScheme sources and
 * Matthew Flatt's "Inside PLT MzScheme" document.
 *========================================================================
 */
    static void
init_exn_catching_apply(void)
{
    if (!exn_catching_apply)
    {
	char *e =
	    "(lambda (thunk) "
		"(with-handlers ([void (lambda (exn) (cons #f exn))]) "
		"(cons #t (thunk))))";

	/* make sure we have a namespace with the standard syntax: */
	Scheme_Env *env = (Scheme_Env *)scheme_make_namespace(0, NULL);
	add_vim_exn(env);

	exn_catching_apply = scheme_eval_string(e, env);
	exn_p = scheme_lookup_global(scheme_intern_symbol("exn?"), env);
	exn_message = scheme_lookup_global(
		scheme_intern_symbol("exn-message"), env);
    }
}

/*
 * This function applies a thunk, returning the Scheme value if there's
 * no exception, otherwise returning NULL and setting *exn to the raised
 * value (usually an exn structure).
 */
    static Scheme_Object *
_apply_thunk_catch_exceptions(Scheme_Object *f, Scheme_Object **exn)
{
    Scheme_Object *v;

    init_exn_catching_apply();

    v = _scheme_apply(exn_catching_apply, 1, &f);
    /* v is a pair: (cons #t value) or (cons #f exn) */

    if (SCHEME_TRUEP(SCHEME_CAR(v)))
	return SCHEME_CDR(v);
    else
    {
	*exn = SCHEME_CDR(v);
	return NULL;
    }
}

    static Scheme_Object *
extract_exn_message(Scheme_Object *v)
{
    init_exn_catching_apply();

    if (SCHEME_TRUEP(_scheme_apply(exn_p, 1, &v)))
	return _scheme_apply(exn_message, 1, &v);
    else
	return NULL; /* Not an exn structure */
}

    static Scheme_Object *
do_eval(void *s, int noargc, Scheme_Object **noargv)
{
    Cmd_Info	*info = (Cmd_Info *)s;

    return scheme_eval_string_all((char *)(info->data), info->env, TRUE);
}

    static Scheme_Object *
do_apply(void *a, int noargc, Scheme_Object **noargv)
{
    Apply_Info	*info = (Apply_Info *)(((Cmd_Info *)a)->data);

    return scheme_apply(info->proc, info->argc, info->argv);
}

/*
 *========================================================================
 *  3.  MzScheme I/O Handlers
 *========================================================================
 */
    static void
do_intrnl_output(char *mesg, long len, int error)
{
    char *p, *prev;

    prev = mesg;
    p = strchr(prev, '\n');
    while (p)
    {
	*p = '\0';
	if (error)
	    EMSG(prev);
	else
	    MSG(prev);
	prev = p + 1;
	p = strchr(prev, '\n');
    }

    if (error)
	EMSG(prev);
    else
	MSG(prev);
}

    static void
do_output(char *mesg, long len)
{
    do_intrnl_output(mesg, len, 0);
}

    static void
do_err_output(char *mesg, long len)
{
    do_intrnl_output(mesg, len, 1);
}

    static void
do_printf(char *format, ...)
{
    do_intrnl_output(format, STRLEN(format), 1);
}

    static void
do_flush(void)
{
    char *buff;
    long length;

    buff = scheme_get_sized_string_output(curerr, &length);
    if (length)
    {
	do_err_output(buff, length);
	return;
    }

    buff = scheme_get_sized_string_output(curout, &length);
    if (length)
	do_output(buff, length);
}

    static int
mzscheme_io_init(void)
{
    /* Nothing needed so far... */
    return 0;
}

/*
 *========================================================================
 *  4. Implementation of the Vim Features for MzScheme
 *========================================================================
 */

/* (command {command-string}) */
    static Scheme_Object *
vim_command(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;
    char	*cmd = SCHEME_STR_VAL(GUARANTEE_STRING(prim->name, 0));

    /* may be use do_cmdline_cmd? */
    do_cmdline((char_u *)cmd, NULL, NULL, DOCMD_NOWAIT|DOCMD_VERBOSE);
    update_screen(VALID);

    raise_if_error();
    return scheme_void;
}

/* (eval {expr-string}) */
    static Scheme_Object *
vim_eval(void *data, int argc, Scheme_Object **argv)
{
#ifdef FEAT_EVAL
    Vim_Prim	    *prim = (Vim_Prim *)data;
    char	    *expr;
    char	    *str;
    Scheme_Object   *result;

    expr = SCHEME_STR_VAL(GUARANTEE_STRING(prim->name, 0));

    str = (char *)eval_to_string((char_u *)expr, NULL, TRUE);

    if (str == NULL)
	raise_vim_exn(_("invalid expression"));

    result = scheme_make_string(str);

    vim_free(str);

    return result;
#else
    raise_vim_exn(_("expressions disabled at compile time"));
    /* unreachable */
    return scheme_false;
#endif
}

/* (range-start) */
    static Scheme_Object *
get_range_start(void *data, int argc, Scheme_Object **argv)
{
    return scheme_make_integer(range_start);
}

/* (range-end) */
    static Scheme_Object *
get_range_end(void *data, int argc, Scheme_Object **argv)
{
    return scheme_make_integer(range_end);
}

/* (beep) */
    static Scheme_Object *
mzscheme_beep(void *data, int argc, Scheme_Object **argv)
{
    vim_beep();
    return scheme_void;
}

static Scheme_Object *M_global = NULL;

/* (get-option {option-name}) [buffer/window] */
    static Scheme_Object *
get_option(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    char_u	    *name;
    long	    value;
    char_u	    *strval;
    int		    rc;
    Scheme_Object   *rval;
    int		    opt_flags = 0;
    buf_T	    *save_curb = curbuf;
    win_T	    *save_curw = curwin;

    name = (char_u *)SCHEME_STR_VAL(GUARANTEE_STRING(prim->name, 0));

    if (argc > 1)
    {
	if (M_global == NULL)
	{
	    MZ_REGISTER_STATIC(M_global);
	    M_global = scheme_intern_symbol("global");
	}

	if (argv[1] == M_global)
	    opt_flags = OPT_GLOBAL;
	else if (SCHEME_VIMBUFFERP(argv[1]))
	{
	    curbuf = get_valid_buffer(argv[1]);
	    opt_flags = OPT_LOCAL;
	}
	else if (SCHEME_VIMWINDOWP(argv[1]))
	{
	    win_T *win = get_valid_window(argv[1]);

	    curwin = win;
	    curbuf = win->w_buffer;
	    opt_flags = OPT_LOCAL;
	}
	else
	    scheme_wrong_type(prim->name, "vim-buffer/window", 1, argc, argv);
    }

    rc = get_option_value(name, &value, &strval, opt_flags);
    curbuf = save_curb;
    curwin = save_curw;

    switch (rc)
    {
    case 1:
	return scheme_make_integer_value(value);
    case 0:
	rval = scheme_make_string(strval);
	vim_free(strval);
	return rval;
    case -1:
    case -2:
	raise_vim_exn(_("hidden option"));
    case -3:
	raise_vim_exn(_("unknown option"));
    }
    /* unreachable */
    return scheme_void;
}

/* (set-option {option-changing-string} [buffer/window]) */
    static Scheme_Object *
set_option(void *data, int argc, Scheme_Object **argv)
{
    char_u	*cmd;
    int		opt_flags = 0;
    buf_T	*save_curb = curbuf;
    win_T	*save_curw = curwin;
    Vim_Prim	*prim = (Vim_Prim *)data;

    GUARANTEE_STRING(prim->name, 0);
    if (argc > 1)
    {
	if (M_global == NULL)
	{
	    MZ_REGISTER_STATIC(M_global);
	    M_global = scheme_intern_symbol("global");
	}

	if (argv[1] == M_global)
	    opt_flags = OPT_GLOBAL;
	else if (SCHEME_VIMBUFFERP(argv[1]))
	{
	    curbuf = get_valid_buffer(argv[1]);
	    opt_flags = OPT_LOCAL;
	}
	else if (SCHEME_VIMWINDOWP(argv[1]))
	{
	    win_T *win = get_valid_window(argv[1]);
	    curwin = win;
	    curbuf = win->w_buffer;
	    opt_flags = OPT_LOCAL;
	}
	else
	    scheme_wrong_type(prim->name, "vim-buffer/window", 1, argc, argv);
    }

    /* do_set can modify cmd, make copy */
    cmd = vim_strsave((char_u *)SCHEME_STR_VAL(argv[0]));
    do_set(cmd, opt_flags);
    vim_free(cmd);
    update_screen(NOT_VALID);
    curbuf = save_curb;
    curwin = save_curw;
    raise_if_error();
    return scheme_void;
}

/*
 *===========================================================================
 *  5. Vim Window-related Manipulation Functions
 *===========================================================================
 */

/* (curr-win) */
    static Scheme_Object *
get_curr_win(void *data, int argc, Scheme_Object **argv)
{
    return (Scheme_Object *)get_vim_curr_window();
}

/* (win-count) */
    static Scheme_Object *
get_window_count(void *data, int argc, Scheme_Object **argv)
{
    win_T   *w;
    int	    n = 0;

    for (w = firstwin; w != NULL; w = w->w_next)
	++n;
    return scheme_make_integer(n);
}

/* (get-win-list [buffer]) */
    static Scheme_Object *
get_window_list(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf;
    Scheme_Object   *list;
    win_T	    *w;

    buf = get_buffer_arg(prim->name, 0, argc, argv);
    list = scheme_null;

    for (w = firstwin; w != NULL; w = w->w_next)
	if (w->w_buffer == buf->buf)
	    list = scheme_make_pair(window_new(w), list);

    return list;
}

    static Scheme_Object *
window_new(win_T *win)
{
    vim_mz_window *self;

    /* We need to handle deletion of windows underneath us.
     * If we add a "w_mzscheme_ref" field to the win_T structure,
     * then we can get at it in win_free() in vim.
     *
     * On a win_free() we set the Scheme object's win_T *field
     * to an invalid value. We trap all uses of a window
     * object, and reject them if the win_T *field is invalid.
     */
    if (win->w_mzscheme_ref != NULL)
	return win->w_mzscheme_ref;

    self = scheme_malloc_fail_ok(scheme_malloc, sizeof(vim_mz_window));

    vim_memset(self, 0, sizeof(vim_mz_window));
    scheme_dont_gc_ptr(self);	/* because win isn't visible to GC */
    win->w_mzscheme_ref = self;
    self->win = win;
    self->tag = mz_window_type;

    return (Scheme_Object *)(self);
}

/* (get-win-num [window]) */
    static Scheme_Object *
get_window_num(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;
    win_T	*win = get_window_arg(prim->name, 0, argc, argv)->win;
    int		nr = 1;
    win_T	*wp;

    for (wp = firstwin; wp != win; wp = wp->w_next)
	++nr;

    return scheme_make_integer(nr);
}

/* (get-win-by-num {windownum}) */
    static Scheme_Object *
get_window_by_num(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;
    win_T	*win;
    int		fnum;

    fnum = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));
    if (fnum < 1)
	scheme_signal_error(_("window index is out of range"));

    for (win = firstwin; win != NULL; win = win->w_next, --fnum)
	if (fnum == 1)	    /* to be 1-based */
	    return window_new(win);

    return scheme_false;
}

/* (get-win-buffer [window]) */
    static Scheme_Object *
get_window_buffer(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_window   *win = get_window_arg(prim->name, 0, argc, argv);

    return buffer_new(win->win->w_buffer);
}

/* (get-win-height [window]) */
    static Scheme_Object *
get_window_height(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_window   *win = get_window_arg(prim->name, 0, argc, argv);

    return scheme_make_integer(win->win->w_height);
}

/* (set-win-height {height} [window]) */
    static Scheme_Object *
set_window_height(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_window   *win;
    win_T	    *savewin;
    int		    height;

    win = get_window_arg(prim->name, 1, argc, argv);
    height = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));

#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

    savewin = curwin;
    curwin = win->win;
    win_setheight(height);
    curwin = savewin;

    raise_if_error();
    return scheme_void;
}

#ifdef FEAT_VERTSPLIT
/* (get-win-width [window]) */
    static Scheme_Object *
get_window_width(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_window   *win = get_window_arg(prim->name, 0, argc, argv);

    return scheme_make_integer(W_WIDTH(win->win));
}

/* (set-win-width {width} [window]) */
    static Scheme_Object *
set_window_width(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_window   *win;
    win_T	    *savewin;
    int		    width = 0;

    win = get_window_arg(prim->name, 1, argc, argv);
    width = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));

# ifdef FEAT_GUI
    need_mouse_correct = TRUE;
# endif

    savewin = curwin;
    curwin = win->win;
    win_setwidth(width);
    curwin = savewin;

    raise_if_error();
    return scheme_void;
}
#endif

/* (get-cursor [window]) -> (line . col) */
    static Scheme_Object *
get_cursor(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_window   *win;
    pos_T	    pos;

    win = get_window_arg(prim->name, 0, argc, argv);
    pos = win->win->w_cursor;
    return scheme_make_pair(scheme_make_integer_value((long)pos.lnum),
		    scheme_make_integer_value((long)pos.col + 1));
}

/* (set-cursor (line . col) [window]) */
    static Scheme_Object *
set_cursor(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_window   *win;
    long	    lnum = 0;
    long	    col = 0;

#ifdef HAVE_SANDBOX
    sandbox_check();
#endif
    win = get_window_arg(prim->name, 1, argc, argv);
    GUARANTEE_PAIR(prim->name, 0);

    if (!SCHEME_INTP(SCHEME_CAR(argv[0]))
	    || !SCHEME_INTP(SCHEME_CDR(argv[0])))
	scheme_wrong_type(prim->name, "integer pair", 0, argc, argv);

    lnum = SCHEME_INT_VAL(SCHEME_CAR(argv[0]));
    col = SCHEME_INT_VAL(SCHEME_CDR(argv[0])) - 1;

    check_line_range(lnum, win->win->w_buffer);
    /* don't know how to catch invalid column value */

    win->win->w_cursor.lnum = lnum;
    win->win->w_cursor.col = col;
    update_screen(VALID);

    raise_if_error();
    return scheme_void;
}
/*
 *===========================================================================
 *  6. Vim Buffer-related Manipulation Functions
 *     Note that each buffer should have its own private namespace.
 *===========================================================================
 */

/* (open-buff {filename}) */
    static Scheme_Object *
mzscheme_open_buffer(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    char	    *fname;
    int		    num = 0;
    Scheme_Object   *onum;

#ifdef HAVE_SANDBOX
    sandbox_check();
#endif
    fname = SCHEME_STR_VAL(GUARANTEE_STRING(prim->name, 0));
    /* TODO make open existing file */
    num = buflist_add(fname, BLN_LISTED | BLN_CURBUF);

    if (num == 0)
	raise_vim_exn(_("couldn't open buffer"));

    onum = scheme_make_integer(num);
    return get_buffer_by_num(data, 1, &onum);
}

/* (get-buff-by-num {buffernum}) */
    static Scheme_Object *
get_buffer_by_num(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;
    buf_T	*buf;
    int		fnum;

    fnum = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));

    for (buf = firstbuf; buf; buf = buf->b_next)
	if (buf->b_fnum == fnum)
	    return buffer_new(buf);

    return scheme_false;
}

/* (get-buff-by-name {buffername}) */
    static Scheme_Object *
get_buffer_by_name(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;
    buf_T	*buf;
    char_u	*fname;

    fname = SCHEME_STR_VAL(GUARANTEE_STRING(prim->name, 0));

    for (buf = firstbuf; buf; buf = buf->b_next)
	if (buf->b_ffname == NULL || buf->b_sfname == NULL)
	    /* empty string */
	{
	    if (fname[0] == NUL)
		return buffer_new(buf);
	}
	else if (!fnamecmp(buf->b_ffname, fname)
		|| !fnamecmp(buf->b_sfname, fname))
	    /* either short or long filename matches */
	    return buffer_new(buf);

    return scheme_false;
}

/* (get-next-buff [buffer]) */
    static Scheme_Object *
get_next_buffer(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;
    buf_T	*buf = get_buffer_arg(prim->name, 0, argc, argv)->buf;

    if (buf->b_next == NULL)
	return scheme_false;
    else
	return buffer_new(buf->b_next);
}

/* (get-prev-buff [buffer]) */
    static Scheme_Object *
get_prev_buffer(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;
    buf_T	*buf = get_buffer_arg(prim->name, 0, argc, argv)->buf;

    if (buf->b_prev == NULL)
	return scheme_false;
    else
	return buffer_new(buf->b_prev);
}

/* (get-buff-num [buffer]) */
    static Scheme_Object *
get_buffer_num(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf = get_buffer_arg(prim->name, 0, argc, argv);

    return scheme_make_integer(buf->buf->b_fnum);
}

/* (buff-count) */
    static Scheme_Object *
get_buffer_count(void *data, int argc, Scheme_Object **argv)
{
    buf_T   *b;
    int	    n = 0;

    for (b = firstbuf; b; b = b->b_next) ++n;
    return scheme_make_integer(n);
}

/* (get-buff-name [buffer]) */
    static Scheme_Object *
get_buffer_name(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf = get_buffer_arg(prim->name, 0, argc, argv);

    return scheme_make_string(buf->buf->b_ffname);
}

/* (curr-buff) */
    static Scheme_Object *
get_curr_buffer(void *data, int argc, Scheme_Object **argv)
{
    return (Scheme_Object *)get_vim_curr_buffer();
}

    static Scheme_Object *
buffer_new(buf_T *buf)
{
    vim_mz_buffer *self;

    /* We need to handle deletion of buffers underneath us.
     * If we add a "b_mzscheme_ref" field to the buf_T structure,
     * then we can get at it in buf_freeall() in vim.
     */
    if (buf->b_mzscheme_ref)
	return buf->b_mzscheme_ref;

    self = scheme_malloc_fail_ok(scheme_malloc, sizeof(vim_mz_buffer));

    vim_memset(self, 0, sizeof(vim_mz_buffer));
    scheme_dont_gc_ptr(self);	/* because buf isn't visible to GC */
    buf->b_mzscheme_ref = self;
    self->buf = buf;
    self->tag = mz_buffer_type;

    mzscheme_interface_init(self);	/* Set up namespace */

    return (Scheme_Object *)(self);
}

/*
 * (get-buff-size [buffer])
 *
 * Get the size (number of lines) in the current buffer.
 */
    static Scheme_Object *
get_buffer_size(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf = get_buffer_arg(prim->name, 0, argc, argv);

    return scheme_make_integer(buf->buf->b_ml.ml_line_count);
}

/*
 * (get-buff-line {linenr} [buffer])
 *
 * Get a line from the specified buffer. The line number is
 * in Vim format (1-based). The line is returned as a MzScheme
 * string object.
 */
    static Scheme_Object *
get_buffer_line(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf;
    int		    linenr;
    char	    *line;

    buf = get_buffer_arg(prim->name, 1, argc, argv);
    linenr = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));
    line = ml_get_buf(buf->buf, (linenr_T)linenr, FALSE);

    raise_if_error();
    return scheme_make_string(line);
}


/*
 * (get-buff-line-list {start} {end} [buffer])
 *
 * Get a list of lines from the specified buffer. The line numbers
 * are in Vim format (1-based). The range is from lo up to, but not
 * including, hi. The list is returned as a list of string objects.
 */
    static Scheme_Object *
get_buffer_line_list(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf;
    int		    i, hi, lo, n;
    Scheme_Object   *list;

    buf = get_buffer_arg(prim->name, 2, argc, argv);
    list = scheme_null;
    hi = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 1));
    lo = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));

    /*
     * Handle some error conditions
     */
    if (lo < 0)
	lo = 0;

    if (hi < 0)
	hi = 0;
    if (hi < lo)
	hi = lo;

    n = hi - lo;

    for (i = n; i >= 0; --i)
    {
	Scheme_Object *str = scheme_make_string(
		       (char *)ml_get_buf(buf->buf, (linenr_T)(lo+i), FALSE));
	raise_if_error();

	/* Set the list item */
	list = scheme_make_pair(str, list);
    }

    return list;
}

/*
 * (set-buff-line {linenr} {string/#f} [buffer])
 *
 * Replace a line in the specified buffer. The line number is
 * in Vim format (1-based). The replacement line is given as
 * an MzScheme string object. The object is checked for validity
 * and correct format. An exception is thrown if the values are not
 * the correct format.
 *
 * It returns a Scheme Object that indicates the length of the
 * string changed.
 */
    static Scheme_Object *
set_buffer_line(void *data, int argc, Scheme_Object **argv)
{
    /* First of all, we check the the of the supplied MzScheme object.
     * There are three cases:
     *	  1. #f - this is a deletion.
     *	  2. A string	   - this is a replacement.
     *	  3. Anything else - this is an error.
     */
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf;
    Scheme_Object   *line;
    char	    *save;
    buf_T	    *savebuf;
    int		    n;

#ifdef HAVE_SANDBOX
    sandbox_check();
#endif
    n = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));
    if (!SCHEME_STRINGP(argv[1]) && !SCHEME_FALSEP(argv[1]))
	scheme_wrong_type(prim->name, "string or #f", 1, argc, argv);
    line = argv[1];
    buf = get_buffer_arg(prim->name, 2, argc, argv);

    check_line_range(n, buf->buf);

    if (SCHEME_FALSEP(line))
    {
	savebuf = curbuf;
	curbuf = buf->buf;

	if (u_savedel((linenr_T)n, 1L) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot save undo information"));
	}
	else if (ml_delete((linenr_T)n, FALSE) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot delete line"));
	}
	deleted_lines_mark((linenr_T)n, 1L);
	if (buf->buf == curwin->w_buffer)
	    mz_fix_cursor(n, n + 1, -1);

	curbuf = savebuf;

	raise_if_error();
	return scheme_void;
    }

    /* Otherwise it's a line */
    save = string_to_line(line);
    savebuf = curbuf;

    curbuf = buf->buf;

    if (u_savesub((linenr_T)n) == FAIL)
    {
	curbuf = savebuf;
	raise_vim_exn(_("cannot save undo information"));
    }
    else if (ml_replace((linenr_T)n, (char_u *)save, TRUE) == FAIL)
    {
	curbuf = savebuf;
	raise_vim_exn(_("cannot replace line"));
    }
    else
	changed_bytes((linenr_T)n, 0);

    curbuf = savebuf;

    raise_if_error();
    return scheme_void;
}

/*
 * (set-buff-line-list {start} {end} {string-list/#f/null} [buffer])
 *
 * Replace a range of lines in the specified buffer. The line numbers are in
 * Vim format (1-based). The range is from lo up to, but not including, hi.
 * The replacement lines are given as a Scheme list of string objects. The
 * list is checked for validity and correct format.
 *
 * Errors are returned as a value of FAIL. The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change is set to the
 * change in the buffer length.
 */
    static Scheme_Object *
set_buffer_line_list(void *data, int argc, Scheme_Object **argv)
{
    /* First of all, we check the type of the supplied MzScheme object.
     * There are three cases:
     *	  1. #f - this is a deletion.
     *	  2. A list	   - this is a replacement.
     *	  3. Anything else - this is an error.
     */
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf;
    Scheme_Object   *line_list;
    Scheme_Object   *line;
    Scheme_Object   *rest;
    char	    **array;
    buf_T	    *savebuf;
    int		    i, old_len, new_len, hi, lo;
    long	    extra;

#ifdef HAVE_SANDBOX
    sandbox_check();
#endif
    lo = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));
    hi = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 1));
    if (!SCHEME_PAIRP(argv[2])
	    && !SCHEME_FALSEP(argv[2]) && !SCHEME_NULLP(argv[2]))
	scheme_wrong_type(prim->name, "list or #f", 2, argc, argv);
    line_list = argv[2];
    buf = get_buffer_arg(prim->name, 3, argc, argv);
    old_len = hi - lo;
    if (old_len < 0) /* process inverse values wisely */
    {
	i = lo;
	lo = hi;
	hi = i;
	old_len = -old_len;
    }
    extra = 0;

    check_line_range(lo, buf->buf);	    /* inclusive */
    check_line_range(hi - 1, buf->buf);  /* exclisive */

    if (SCHEME_FALSEP(line_list) || SCHEME_NULLP(line_list))
    {
	savebuf = curbuf;
	curbuf = buf->buf;

	if (u_savedel((linenr_T)lo, (long)old_len) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot save undo information"));
	}
	else
	{
	    for (i = 0; i < old_len; i++)
		if (ml_delete((linenr_T)lo, FALSE) == FAIL)
		{
		    curbuf = savebuf;
		    raise_vim_exn(_("cannot delete line"));
		}
	    deleted_lines_mark((linenr_T)lo, (long)old_len);
	    if (buf->buf == curwin->w_buffer)
		mz_fix_cursor(lo, hi, -old_len);
	}

	curbuf = savebuf;

	raise_if_error();
	return scheme_void;
    }

    /* List */
    new_len = scheme_proper_list_length(line_list);
    if (new_len < 0)	/* improper or cyclic list */
	scheme_wrong_type(prim->name, "proper list",
		2, argc, argv);

    /* Using MzScheme allocator, so we don't need to free this and
     * can safely keep pointers to GC collected strings
     */
    array = (char **)scheme_malloc_fail_ok(scheme_malloc,
		(unsigned)(new_len * sizeof(char *)));

    rest = line_list;
    for (i = 0; i < new_len; ++i)
    {
	line = SCHEME_CAR(rest);
	rest = SCHEME_CDR(rest);
	if (!SCHEME_STRINGP(line))
	    scheme_wrong_type(prim->name, "string-list", 2, argc, argv);
	array[i] = string_to_line(line);
    }

    savebuf = curbuf;
    curbuf = buf->buf;

    if (u_save((linenr_T)(lo-1), (linenr_T)hi) == FAIL)
    {
	curbuf = savebuf;
	raise_vim_exn(_("cannot save undo information"));
    }

    /*
     * If the size of the range is reducing (ie, new_len < old_len) we
     * need to delete some old_len. We do this at the start, by
     * repeatedly deleting line "lo".
     */
    for (i = 0; i < old_len - new_len; ++i)
    {
	if (ml_delete((linenr_T)lo, FALSE) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot delete line"));
	}
	extra--;
    }

    /*
     * For as long as possible, replace the existing old_len with the
     * new old_len. This is a more efficient operation, as it requires
     * less memory allocation and freeing.
     */
    for (i = 0; i < old_len && i < new_len; i++)
	if (ml_replace((linenr_T)(lo+i), (char_u *)array[i], TRUE) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot replace line"));
	}

    /*
     * Now we may need to insert the remaining new_len.  We don't need to
     * free the string passed back because MzScheme has control of that
     * memory.
     */
    while (i < new_len)
    {
	if (ml_append((linenr_T)(lo + i - 1),
		(char_u *)array[i], 0, FALSE) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot insert line"));
	}
	++i;
	++extra;
    }

    /*
     * Adjust marks. Invalidate any which lie in the
     * changed range, and move any in the remainder of the buffer.
     */
    mark_adjust((linenr_T)lo, (linenr_T)(hi - 1), (long)MAXLNUM, (long)extra);
    changed_lines((linenr_T)lo, 0, (linenr_T)hi, (long)extra);

    if (buf->buf == curwin->w_buffer)
	mz_fix_cursor(lo, hi, extra);
    curbuf = savebuf;

    raise_if_error();
    return scheme_void;
}

/*
 * (insert-buff-line-list {linenr} {string/string-list} [buffer])
 *
 * Insert a number of lines into the specified buffer after the specifed line.
 * The line number is in Vim format (1-based). The lines to be inserted are
 * given as an MzScheme list of string objects or as a single string. The lines
 * to be added are checked for validity and correct format. Errors are
 * returned as a value of FAIL.  The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
    static Scheme_Object *
insert_buffer_line_list(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	    *prim = (Vim_Prim *)data;
    vim_mz_buffer   *buf;
    Scheme_Object   *list;
    Scheme_Object   *line;
    Scheme_Object   *rest;
    char	    **array;
    char	    *str;
    buf_T	    *savebuf;
    int		    i, n, size;

#ifdef HAVE_SANDBOX
    sandbox_check();
#endif
    /*
     * First of all, we check the type of the supplied MzScheme object.
     * It must be a string or a list, or the call is in error.
     */
    n = SCHEME_INT_VAL(GUARANTEE_INTEGER(prim->name, 0));
    list = argv[1];

    if (!SCHEME_STRINGP(list) && !SCHEME_PAIRP(list))
	scheme_wrong_type(prim->name, "string or list", 1, argc, argv);
    buf = get_buffer_arg(prim->name, 2, argc, argv);

    if (n != 0)	    /* 0 can be used in insert */
	check_line_range(n, buf->buf);
    if (SCHEME_STRINGP(list))
    {
	str = string_to_line(list);

	savebuf = curbuf;
	curbuf = buf->buf;

	if (u_save((linenr_T)n, (linenr_T)(n+1)) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot save undo information"));
	}
	else if (ml_append((linenr_T)n, (char_u *)str, 0, FALSE) == FAIL)
	{
	    curbuf = savebuf;
	    raise_vim_exn(_("cannot insert line"));
	}
	else
	    appended_lines_mark((linenr_T)n, 1L);

	curbuf = savebuf;
	update_screen(VALID);

	raise_if_error();
	return scheme_void;
    }

    /* List */
    size = scheme_proper_list_length(list);
    if (size < 0)	/* improper or cyclic list */
	scheme_wrong_type(prim->name, "proper list",
		2, argc, argv);

    /* Using MzScheme allocator, so we don't need to free this and
     * can safely keep pointers to GC collected strings
     */
    array = (char **)scheme_malloc_fail_ok(
	    scheme_malloc, (unsigned)(size * sizeof(char *)));

    rest = list;
    for (i = 0; i < size; ++i)
    {
	line = SCHEME_CAR(rest);
	rest = SCHEME_CDR(rest);
	array[i] = string_to_line(line);
    }

    savebuf = curbuf;
    curbuf = buf->buf;

    if (u_save((linenr_T)n, (linenr_T)(n + 1)) == FAIL)
    {
	curbuf = savebuf;
	raise_vim_exn(_("cannot save undo information"));
    }
    else
    {
	for (i = 0; i < size; ++i)
	    if (ml_append((linenr_T)(n + i), (char_u *)array[i],
			0, FALSE) == FAIL)
	    {
		curbuf = savebuf;
		raise_vim_exn(_("cannot insert line"));
	    }

	if (i > 0)
	    appended_lines_mark((linenr_T)n, (long)i);
    }

    curbuf = savebuf;
    update_screen(VALID);

    raise_if_error();
    return scheme_void;
}

/* (get-buff-namespace [buffer]) */
    static Scheme_Object *
get_buffer_namespace(void *data, int argc, Scheme_Object **argv)
{
    Vim_Prim	*prim = (Vim_Prim *)data;

    return (Scheme_Object *)get_buffer_arg(prim->name, 0, argc, argv)->env;
}

/*
 * Predicates
 */
/* (buff? obj) */
    static Scheme_Object *
vim_bufferp(void *data, int argc, Scheme_Object **argv)
{
    if (SCHEME_VIMBUFFERP(argv[0]))
	return scheme_true;
    else
	return scheme_false;
}

/* (win? obj) */
    static Scheme_Object *
vim_windowp(void *data, int argc, Scheme_Object **argv)
{
    if (SCHEME_VIMWINDOWP(argv[0]))
	return scheme_true;
    else
	return scheme_false;
}

/* (buff-valid? obj) */
    static Scheme_Object *
vim_buffer_validp(void *data, int argc, Scheme_Object **argv)
{
    if (SCHEME_VIMBUFFERP(argv[0])
	    && ((vim_mz_buffer *)argv[0])->buf != INVALID_BUFFER_VALUE)
	return scheme_true;
    else
	return scheme_false;
}

/* (win-valid? obj) */
    static Scheme_Object *
vim_window_validp(void *data, int argc, Scheme_Object **argv)
{
    if (SCHEME_VIMWINDOWP(argv[0])
	    && ((vim_mz_window *)argv[0])->win != INVALID_WINDOW_VALUE)
	return scheme_true;
    else
	return scheme_false;
}

/*
 *===========================================================================
 * Utilities
 *===========================================================================
 */

/*
 * Convert an MzScheme string into a Vim line.
 *
 * The result is in allocated memory. All internal nulls are replaced by
 * newline characters. It is an error for the string to contain newline
 * characters.
 *
 */
    static char *
string_to_line(Scheme_Object *obj)
{
    char	*str;
    long	len;
    int		i;

    str = scheme_display_to_string(obj, &len);

    /* Error checking: String must not contain newlines, as we
     * are replacing a single line, and we must replace it with
     * a single line.
     */
    if (memchr(str, '\n', len))
	scheme_signal_error(_("string cannot contain newlines"));

    /* Create a copy of the string, with internal nulls replaced by
     * newline characters, as is the vim convention.
     */
    for (i = 0; i < len; ++i)
    {
	if (str[i] == '\0')
	    str[i] = '\n';
    }

    str[i] = '\0';

    return str;
}

/*
 * Check to see whether a Vim error has been reported, or a keyboard
 * interrupt (from vim --> got_int) has been detected.
 */
    static int
vim_error_check(void)
{
    return (got_int || did_emsg);
}

/*
 * register Scheme exn:vim
 */
    static void
register_vim_exn(Scheme_Env *env)
{
    Scheme_Object   *exn_name = scheme_intern_symbol("exn:vim");

    if (vim_exn == NULL)
	vim_exn = scheme_make_struct_type(exn_name,
		scheme_builtin_value("struct:exn"), NULL, 0, 0, NULL, NULL
#if MZSCHEME_VERSION_MAJOR >= 299
		, NULL
#endif
		);

    if (vim_exn_values == NULL)
    {
	int	nc = 0;

	Scheme_Object   **exn_names = scheme_make_struct_names(
		exn_name, scheme_null, 0, &nc);
	Scheme_Object   **exn_values = scheme_make_struct_values(
		vim_exn, exn_names, nc, 0);

	vim_exn_names = scheme_make_vector(nc, scheme_false);
	vim_exn_values = scheme_make_vector(nc, scheme_false);
	/* remember names and values */
	mch_memmove(SCHEME_VEC_ELS(vim_exn_names), exn_names,
		nc * sizeof(Scheme_Object *));
	mch_memmove(SCHEME_VEC_ELS(vim_exn_values), exn_values,
		nc * sizeof(Scheme_Object *));
    }

    add_vim_exn(env);
}

/*
 * Add stuff of exn:vim to env
 */
    static void
add_vim_exn(Scheme_Env *env)
{
    int i;

    for (i = 0; i < SCHEME_VEC_SIZE(vim_exn_values); i++)
	scheme_add_global_symbol(SCHEME_VEC_ELS(vim_exn_names)[i],
		SCHEME_VEC_ELS(vim_exn_values)[i], env);
}

/*
 * raise exn:vim, may be with additional info string
 */
    void
raise_vim_exn(const char *add_info)
{
    Scheme_Object   *argv[2];
    char_u	    *fmt = _("Vim error: ~a");

    if (add_info != NULL)
    {
	Scheme_Object   *info = scheme_make_string(add_info);
	argv[0] = scheme_byte_string_to_char_string(scheme_make_string(
		scheme_format(fmt, strlen(fmt), 1, &info, NULL)));
	SCHEME_SET_IMMUTABLE(argv[0]);
    }
    else
	argv[0] = scheme_make_string(_("Vim error"));

    argv[1] = scheme_current_continuation_marks();

    scheme_raise(scheme_make_struct_instance(vim_exn, 2, argv));
}

    void
raise_if_error(void)
{
    if (vim_error_check())
	raise_vim_exn(NULL);
}

/* get buffer:
 * either current
 * or passed as argv[argnum] with checks
 */
    static vim_mz_buffer *
get_buffer_arg(const char *fname, int argnum, int argc, Scheme_Object **argv)
{
    vim_mz_buffer *b;

    if (argc < argnum + 1)
	return get_vim_curr_buffer();
    if (!SCHEME_VIMBUFFERP(argv[argnum]))
	scheme_wrong_type(fname, "vim-buffer", argnum, argc, argv);
    b = (vim_mz_buffer *)argv[argnum];
    (void)get_valid_buffer(argv[argnum]);
    return b;
}

/* get window:
 * either current
 * or passed as argv[argnum] with checks
 */
    static vim_mz_window *
get_window_arg(const char *fname, int argnum, int argc, Scheme_Object **argv)
{
    vim_mz_window *w;

    if (argc < argnum + 1)
	return get_vim_curr_window();
    w = (vim_mz_window *)argv[argnum];
    if (!SCHEME_VIMWINDOWP(argv[argnum]))
	scheme_wrong_type(fname, "vim-window", argnum, argc, argv);
    (void)get_valid_window(argv[argnum]);
    return w;
}

/* get valid Vim buffer from Scheme_Object* */
buf_T *get_valid_buffer(void *obj)
{
    buf_T *buf = ((vim_mz_buffer *)obj)->buf;

    if (buf == INVALID_BUFFER_VALUE)
	scheme_signal_error(_("buffer is invalid"));
    return buf;
}

/* get valid Vim window from Scheme_Object* */
win_T *get_valid_window(void *obj)
{
    win_T *win = ((vim_mz_window *)obj)->win;
    if (win == INVALID_WINDOW_VALUE)
	scheme_signal_error(_("window is invalid"));
    return win;
}

    int
mzthreads_allowed(void)
{
    return mz_threads_allow;
}

    static int
line_in_range(linenr_T lnum, buf_T *buf)
{
    return (lnum > 0 && lnum <= buf->b_ml.ml_line_count);
}

    static void
check_line_range(linenr_T lnum, buf_T *buf)
{
    if (!line_in_range(lnum, buf))
	scheme_signal_error(_("linenr out of range"));
}

/*
 * Check if deleting lines made the cursor position invalid
 * (or you'll get msg from Vim about invalid linenr).
 * Changed the lines from "lo" to "hi" and added "extra" lines (negative if
 * deleted). Got from if_python.c
 */
    static void
mz_fix_cursor(int lo, int hi, int extra)
{
    if (curwin->w_cursor.lnum >= lo)
    {
	/* Adjust the cursor position if it's in/after the changed
	 * lines. */
	if (curwin->w_cursor.lnum >= hi)
	{
	    curwin->w_cursor.lnum += extra;
	    check_cursor_col();
	}
	else if (extra < 0)
	{
	    curwin->w_cursor.lnum = lo;
	    check_cursor();
	}
	changed_cline_bef_curs();
    }
    invalidate_botline();
}

static Vim_Prim prims[]=
{
    /*
     * Buffer-related commands
     */
    {get_buffer_line, "get-buff-line", 1, 2},
    {set_buffer_line, "set-buff-line", 2, 3},
    {get_buffer_line_list, "get-buff-line-list", 2, 3},
    {get_buffer_name, "get-buff-name", 0, 1},
    {get_buffer_num, "get-buff-num", 0, 1},
    {get_buffer_size, "get-buff-size", 0, 1},
    {set_buffer_line_list, "set-buff-line-list", 3, 4},
    {insert_buffer_line_list, "insert-buff-line-list", 2, 3},
    {get_curr_buffer, "curr-buff", 0, 0},
    {get_buffer_count, "buff-count", 0, 0},
    {get_next_buffer, "get-next-buff", 0, 1},
    {get_prev_buffer, "get-prev-buff", 0, 1},
    {mzscheme_open_buffer, "open-buff", 1, 1},
    {get_buffer_by_name, "get-buff-by-name", 1, 1},
    {get_buffer_by_num, "get-buff-by-num", 1, 1},
    {get_buffer_namespace, "get-buff-namespace", 0, 1},
    /*
     * Window-related commands
     */
    {get_curr_win, "curr-win", 0, 0},
    {get_window_count, "win-count", 0, 0},
    {get_window_by_num, "get-win-by-num", 1, 1},
    {get_window_num, "get-win-num", 0, 1},
    {get_window_buffer, "get-win-buffer", 0, 1},
    {get_window_height, "get-win-height", 0, 1},
    {set_window_height, "set-win-height", 1, 2},
#ifdef FEAT_VERTSPLIT
    {get_window_width, "get-win-width", 0, 1},
    {set_window_width, "set-win-width", 1, 2},
#endif
    {get_cursor, "get-cursor", 0, 1},
    {set_cursor, "set-cursor", 1, 2},
    {get_window_list, "get-win-list", 0, 1},
    /*
     * Vim-related commands
     */
    {vim_command, "command", 1, 1},
    {vim_eval, "eval", 1, 1},
    {get_range_start, "range-start", 0, 0},
    {get_range_end, "range-end", 0, 0},
    {mzscheme_beep, "beep", 0, 0},
    {get_option, "get-option", 1, 2},
    {set_option, "set-option", 1, 2},
    /*
     * small utilities
     */
    {vim_bufferp, "buff?", 1, 1},
    {vim_windowp, "win?", 1, 1},
    {vim_buffer_validp, "buff-valid?", 1, 1},
    {vim_window_validp, "win-valid?", 1, 1}
};

/* return MzScheme wrapper for curbuf */
    static vim_mz_buffer *
get_vim_curr_buffer(void)
{
    if (curbuf->b_mzscheme_ref == NULL)
	return (vim_mz_buffer *)buffer_new(curbuf);
    else
	return (vim_mz_buffer *)curbuf->b_mzscheme_ref;
}

/* return MzScheme wrapper for curwin */
    static vim_mz_window *
get_vim_curr_window(void)
{
    if (curwin->w_mzscheme_ref == NULL)
	return (vim_mz_window *)window_new(curwin);
    else
	return (vim_mz_window *)curwin->w_mzscheme_ref;
}

    static void
make_modules(Scheme_Env *env)
{
    int		i;
    Scheme_Env	*mod;

    mod = scheme_primitive_module(scheme_intern_symbol("vimext"), env);
    /* all prims made closed so they can access their own names */
    for (i = 0; i < sizeof(prims)/sizeof(prims[0]); i++)
    {
	Vim_Prim *prim = prims + i;
	scheme_add_global(prim->name,
		scheme_make_closed_prim_w_arity(prim->prim, prim, prim->name,
		    prim->mina, prim->maxa),
		mod);
    }
    scheme_add_global("global-namespace", (Scheme_Object *)environment, mod);
    scheme_finish_primitive_module(mod);
}

#ifdef HAVE_SANDBOX
static Scheme_Object *M_write = NULL;
static Scheme_Object *M_read = NULL;
static Scheme_Object *M_execute = NULL;
static Scheme_Object *M_delete = NULL;

    static void
sandbox_check()
{
    if (sandbox)
	raise_vim_exn(_("not allowed in the Vim sandbox"));
}

/* security guards to force Vim's sandbox restrictions on MzScheme level */
    static Scheme_Object *
sandbox_file_guard(int argc, Scheme_Object **argv)
{
    if (sandbox)
    {
	Scheme_Object *requested_access = argv[2];

	if (M_write == NULL)
	{
	    MZ_REGISTER_STATIC(M_write);
	    M_write = scheme_intern_symbol("write");
	}
	if (M_read == NULL)
	{
	    MZ_REGISTER_STATIC(M_read);
	    M_read = scheme_intern_symbol("read");
	}
	if (M_execute == NULL)
	{
	    MZ_REGISTER_STATIC(M_execute);
	    M_execute = scheme_intern_symbol("execute");
	}
	if (M_delete == NULL)
	{
	    MZ_REGISTER_STATIC(M_delete);
	    M_delete = scheme_intern_symbol("delete");
	}

	while (!SCHEME_NULLP(requested_access))
	{
	    Scheme_Object *item = SCHEME_CAR(requested_access);
	    if (scheme_eq(item, M_write) || scheme_eq(item, M_read)
		    || scheme_eq(item, M_execute) || scheme_eq(item, M_delete))
	    {
		raise_vim_exn(_("not allowed in the Vim sandbox"));
	    }
	    requested_access = SCHEME_CDR(requested_access);
	}
    }
    return scheme_void;
}

    static Scheme_Object *
sandbox_network_guard(int argc, Scheme_Object **argv)
{
    return scheme_void;
}
#endif

#endif
