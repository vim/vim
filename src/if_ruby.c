/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Ruby interface by Shugo Maeda
 *   with improvements by SegPhault (Ryan Paul)
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

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

/*
 * This is tricky.  In ruby.h there is (inline) function rb_class_of()
 * definition.  This function use these variables.  But we want function to
 * use dll_* variables.
 */
#ifdef DYNAMIC_RUBY
# define rb_cFalseClass		(*dll_rb_cFalseClass)
# define rb_cFixnum		(*dll_rb_cFixnum)
# define rb_cNilClass		(*dll_rb_cNilClass)
# define rb_cSymbol		(*dll_rb_cSymbol)
# define rb_cTrueClass		(*dll_rb_cTrueClass)
# if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
/*
 * On ver 1.8, all Ruby functions are exported with "__declspce(dllimport)"
 * in ruby.h.  But it cause trouble for these variables, because it is
 * defined in this file.  When defined this RUBY_EXPORT it modified to
 * "extern" and be able to avoid this problem.
 */
#  define RUBY_EXPORT
# endif
#endif

#include <ruby.h>

#undef EXTERN
#undef _

/* T_DATA defined both by Ruby and Mac header files, hack around it... */
#if defined(MACOS_X_UNIX) || defined(macintosh)
# define __OPENTRANSPORT__
# define __OPENTRANSPORTPROTOCOL__
# define __OPENTRANSPORTPROVIDERS__
#endif

#include "vim.h"
#include "version.h"

#if defined(PROTO) && !defined(FEAT_RUBY)
/* Define these to be able to generate the function prototypes. */
# define VALUE int
# define RUBY_DATA_FUNC int
#endif

static int ruby_initialized = 0;
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

#if defined(DYNAMIC_RUBY) || defined(PROTO)
#ifdef PROTO
# define HINSTANCE int		/* for generating prototypes */
#endif

/*
 * Wrapper defines
 */
#define rb_assoc_new			dll_rb_assoc_new
#define rb_cObject			(*dll_rb_cObject)
#define rb_check_type			dll_rb_check_type
#define rb_class_path			dll_rb_class_path
#define rb_data_object_alloc		dll_rb_data_object_alloc
#define rb_define_class_under		dll_rb_define_class_under
#define rb_define_const			dll_rb_define_const
#define rb_define_global_function	dll_rb_define_global_function
#define rb_define_method		dll_rb_define_method
#define rb_define_module		dll_rb_define_module
#define rb_define_module_function	dll_rb_define_module_function
#define rb_define_singleton_method	dll_rb_define_singleton_method
#define rb_define_virtual_variable	dll_rb_define_virtual_variable
#define rb_stdout			(*dll_rb_stdout)
#define rb_eArgError			(*dll_rb_eArgError)
#define rb_eIndexError			(*dll_rb_eIndexError)
#define rb_eRuntimeError		(*dll_rb_eRuntimeError)
#define rb_eStandardError		(*dll_rb_eStandardError)
#define rb_eval_string_protect		dll_rb_eval_string_protect
#define rb_global_variable		dll_rb_global_variable
#define rb_hash_aset			dll_rb_hash_aset
#define rb_hash_new			dll_rb_hash_new
#define rb_inspect			dll_rb_inspect
#define rb_int2inum			dll_rb_int2inum
#define rb_lastline_get			dll_rb_lastline_get
#define rb_lastline_set			dll_rb_lastline_set
#define rb_load_protect			dll_rb_load_protect
#define rb_num2long			dll_rb_num2long
#define rb_num2ulong			dll_rb_num2ulong
#define rb_obj_alloc			dll_rb_obj_alloc
#define rb_obj_as_string		dll_rb_obj_as_string
#define rb_obj_id			dll_rb_obj_id
#define rb_raise			dll_rb_raise
#define rb_str2cstr			dll_rb_str2cstr
#define rb_str_cat			dll_rb_str_cat
#define rb_str_concat			dll_rb_str_concat
#define rb_str_new			dll_rb_str_new
#define rb_str_new2			dll_rb_str_new2
#define ruby_errinfo			(*dll_ruby_errinfo)
#define ruby_init			dll_ruby_init
#define ruby_init_loadpath		dll_ruby_init_loadpath
#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
# define rb_w32_snprintf		dll_rb_w32_snprintf
#endif

/*
 * Pointers for dynamic link
 */
static VALUE (*dll_rb_assoc_new) (VALUE, VALUE);
static VALUE *dll_rb_cFalseClass;
static VALUE *dll_rb_cFixnum;
static VALUE *dll_rb_cNilClass;
static VALUE *dll_rb_cObject;
static VALUE *dll_rb_cSymbol;
static VALUE *dll_rb_cTrueClass;
static void (*dll_rb_check_type) (VALUE,int);
static VALUE (*dll_rb_class_path) (VALUE);
static VALUE (*dll_rb_data_object_alloc) (VALUE, void*, RUBY_DATA_FUNC, RUBY_DATA_FUNC);
static VALUE (*dll_rb_define_class_under) (VALUE, const char*, VALUE);
static void (*dll_rb_define_const) (VALUE,const char*,VALUE);
static void (*dll_rb_define_global_function) (const char*,VALUE(*)(),int);
static void (*dll_rb_define_method) (VALUE,const char*,VALUE(*)(),int);
static VALUE (*dll_rb_define_module) (const char*);
static void (*dll_rb_define_module_function) (VALUE,const char*,VALUE(*)(),int);
static void (*dll_rb_define_singleton_method) (VALUE,const char*,VALUE(*)(),int);
static void (*dll_rb_define_virtual_variable) (const char*,VALUE(*)(),void(*)());
static VALUE *dll_rb_stdout;
static VALUE *dll_rb_eArgError;
static VALUE *dll_rb_eIndexError;
static VALUE *dll_rb_eRuntimeError;
static VALUE *dll_rb_eStandardError;
static VALUE (*dll_rb_eval_string_protect) (const char*, int*);
static void (*dll_rb_global_variable) (VALUE*);
static VALUE (*dll_rb_hash_aset) (VALUE, VALUE, VALUE);
static VALUE (*dll_rb_hash_new) (void);
static VALUE (*dll_rb_inspect) (VALUE);
static VALUE (*dll_rb_int2inum) (long);
static VALUE (*dll_rb_int2inum) (long);
static VALUE (*dll_rb_lastline_get) (void);
static void (*dll_rb_lastline_set) (VALUE);
static void (*dll_rb_load_protect) (VALUE, int, int*);
static long (*dll_rb_num2long) (VALUE);
static unsigned long (*dll_rb_num2ulong) (VALUE);
static VALUE (*dll_rb_obj_alloc) (VALUE);
static VALUE (*dll_rb_obj_as_string) (VALUE);
static VALUE (*dll_rb_obj_id) (VALUE);
static void (*dll_rb_raise) (VALUE, const char*, ...);
static char *(*dll_rb_str2cstr) (VALUE,int*);
static VALUE (*dll_rb_str_cat) (VALUE, const char*, long);
static VALUE (*dll_rb_str_concat) (VALUE, VALUE);
static VALUE (*dll_rb_str_new) (const char*, long);
static VALUE (*dll_rb_str_new2) (const char*);
static VALUE *dll_ruby_errinfo;
static void (*dll_ruby_init) (void);
static void (*dll_ruby_init_loadpath) (void);
#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
static int (*dll_rb_w32_snprintf)(char*, size_t, const char*, ...);
#endif

static HINSTANCE hinstRuby = 0; /* Instance of ruby.dll */

/*
 * Table of name to function pointer of ruby.
 */
#define RUBY_PROC FARPROC
static struct
{
    char *name;
    RUBY_PROC *ptr;
} ruby_funcname_table[] =
{
    {"rb_assoc_new", (RUBY_PROC*)&dll_rb_assoc_new},
    {"rb_cFalseClass", (RUBY_PROC*)&dll_rb_cFalseClass},
    {"rb_cFixnum", (RUBY_PROC*)&dll_rb_cFixnum},
    {"rb_cNilClass", (RUBY_PROC*)&dll_rb_cNilClass},
    {"rb_cObject", (RUBY_PROC*)&dll_rb_cObject},
    {"rb_cSymbol", (RUBY_PROC*)&dll_rb_cSymbol},
    {"rb_cTrueClass", (RUBY_PROC*)&dll_rb_cTrueClass},
    {"rb_check_type", (RUBY_PROC*)&dll_rb_check_type},
    {"rb_class_path", (RUBY_PROC*)&dll_rb_class_path},
    {"rb_data_object_alloc", (RUBY_PROC*)&dll_rb_data_object_alloc},
    {"rb_define_class_under", (RUBY_PROC*)&dll_rb_define_class_under},
    {"rb_define_const", (RUBY_PROC*)&dll_rb_define_const},
    {"rb_define_global_function", (RUBY_PROC*)&dll_rb_define_global_function},
    {"rb_define_method", (RUBY_PROC*)&dll_rb_define_method},
    {"rb_define_module", (RUBY_PROC*)&dll_rb_define_module},
    {"rb_define_module_function", (RUBY_PROC*)&dll_rb_define_module_function},
    {"rb_define_singleton_method", (RUBY_PROC*)&dll_rb_define_singleton_method},
    {"rb_define_virtual_variable", (RUBY_PROC*)&dll_rb_define_virtual_variable},
    {"rb_stdout", (RUBY_PROC*)&dll_rb_stdout},
    {"rb_eArgError", (RUBY_PROC*)&dll_rb_eArgError},
    {"rb_eIndexError", (RUBY_PROC*)&dll_rb_eIndexError},
    {"rb_eRuntimeError", (RUBY_PROC*)&dll_rb_eRuntimeError},
    {"rb_eStandardError", (RUBY_PROC*)&dll_rb_eStandardError},
    {"rb_eval_string_protect", (RUBY_PROC*)&dll_rb_eval_string_protect},
    {"rb_global_variable", (RUBY_PROC*)&dll_rb_global_variable},
    {"rb_hash_aset", (RUBY_PROC*)&dll_rb_hash_aset},
    {"rb_hash_new", (RUBY_PROC*)&dll_rb_hash_new},
    {"rb_inspect", (RUBY_PROC*)&dll_rb_inspect},
    {"rb_int2inum", (RUBY_PROC*)&dll_rb_int2inum},
    {"rb_lastline_get", (RUBY_PROC*)&dll_rb_lastline_get},
    {"rb_lastline_set", (RUBY_PROC*)&dll_rb_lastline_set},
    {"rb_load_protect", (RUBY_PROC*)&dll_rb_load_protect},
    {"rb_num2long", (RUBY_PROC*)&dll_rb_num2long},
    {"rb_num2ulong", (RUBY_PROC*)&dll_rb_num2ulong},
    {"rb_obj_alloc", (RUBY_PROC*)&dll_rb_obj_alloc},
    {"rb_obj_as_string", (RUBY_PROC*)&dll_rb_obj_as_string},
    {"rb_obj_id", (RUBY_PROC*)&dll_rb_obj_id},
    {"rb_raise", (RUBY_PROC*)&dll_rb_raise},
    {"rb_str2cstr", (RUBY_PROC*)&dll_rb_str2cstr},
    {"rb_str_cat", (RUBY_PROC*)&dll_rb_str_cat},
    {"rb_str_concat", (RUBY_PROC*)&dll_rb_str_concat},
    {"rb_str_new", (RUBY_PROC*)&dll_rb_str_new},
    {"rb_str_new2", (RUBY_PROC*)&dll_rb_str_new2},
    {"ruby_errinfo", (RUBY_PROC*)&dll_ruby_errinfo},
    {"ruby_init", (RUBY_PROC*)&dll_ruby_init},
    {"ruby_init_loadpath", (RUBY_PROC*)&dll_ruby_init_loadpath},
#if defined(DYNAMIC_RUBY_VER) && DYNAMIC_RUBY_VER >= 18
    {"rb_w32_snprintf", (RUBY_PROC*)&dll_rb_w32_snprintf},
#endif
    {"", NULL},
};

/*
 * Free ruby.dll
 */
    static void
end_dynamic_ruby()
{
    if (hinstRuby)
    {
	FreeLibrary(hinstRuby);
	hinstRuby = 0;
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
    hinstRuby = LoadLibrary(libname);
    if (!hinstRuby)
    {
	if (verbose)
	    EMSG2(_(e_loadlib), libname);
	return FAIL;
    }

    for (i = 0; ruby_funcname_table[i].ptr; ++i)
    {
	if (!(*ruby_funcname_table[i].ptr = GetProcAddress(hinstRuby,
			ruby_funcname_table[i].name)))
	{
	    FreeLibrary(hinstRuby);
	    hinstRuby = 0;
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
ruby_enabled(verbose)
    int		verbose;
{
    return ruby_runtime_link_init(DYNAMIC_RUBY_DLL, verbose) == OK;
}
#endif /* defined(DYNAMIC_RUBY) || defined(PROTO) */

    void
ruby_end()
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

void ex_rubydo(exarg_T *eap)
{
    int state;
    linenr_T i;

    if (ensure_ruby_initialized())
    {
	if (u_save(eap->line1 - 1, eap->line2 + 1) != OK)
	    return;
	for (i = eap->line1; i <= eap->line2; i++) {
	    VALUE line, oldline;

	    line = oldline = rb_str_new2((char *)ml_get(i));
	    rb_lastline_set(line);
	    rb_eval_string_protect((char *) eap->arg, &state);
	    if (state) {
		error_print(state);
		break;
	    }
	    line = rb_lastline_get();
	    if (!NIL_P(line)) {
		if (TYPE(line) != T_STRING) {
		    EMSG(_("E265: $_ must be an instance of String"));
		    return;
		}
		ml_replace(i, (char_u *) STR2CSTR(line), 1);
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

void ex_rubyfile(exarg_T *eap)
{
    int state;

    if (ensure_ruby_initialized())
    {
	rb_load_protect(rb_str_new2((char *) eap->arg), 0, &state);
	if (state) error_print(state);
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
	    ruby_init();
	    ruby_init_loadpath();
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
#ifndef DYNAMIC_RUBY
    RUBYEXTERN VALUE ruby_errinfo;
#endif
    VALUE eclass;
    VALUE einfo;
    char buff[BUFSIZ];

#define TAG_RETURN	0x1
#define TAG_BREAK	0x2
#define TAG_NEXT	0x3
#define TAG_RETRY	0x4
#define TAG_REDO	0x5
#define TAG_RAISE	0x6
#define TAG_THROW	0x7
#define TAG_FATAL	0x8
#define TAG_MASK	0xf

    switch (state) {
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
	eclass = CLASS_OF(ruby_errinfo);
	einfo = rb_obj_as_string(ruby_errinfo);
	if (eclass == rb_eRuntimeError && RSTRING(einfo)->len == 0) {
	    EMSG(_("E272: unhandled exception"));
	}
	else {
	    VALUE epath;
	    char *p;

	    epath = rb_class_path(eclass);
	    vim_snprintf(buff, BUFSIZ, "%s: %s",
		     RSTRING(epath)->ptr, RSTRING(einfo)->ptr);
	    p = strchr(buff, '\n');
	    if (p) *p = '\0';
	    EMSG(buff);
	}
	break;
    default:
	vim_snprintf(buff, BUFSIZ, _("E273: unknown longjmp status %d"), state);
	EMSG(buff);
	break;
    }
}

static VALUE vim_message(VALUE self, VALUE str)
{
    char *buff, *p;

    str = rb_obj_as_string(str);
    buff = ALLOCA_N(char, RSTRING(str)->len);
    strcpy(buff, RSTRING(str)->ptr);
    p = strchr(buff, '\n');
    if (p) *p = '\0';
    MSG(buff);
    return Qnil;
}

static VALUE vim_set_option(VALUE self, VALUE str)
{
    do_set((char_u *)STR2CSTR(str), 0);
    update_screen(NOT_VALID);
    return Qnil;
}

static VALUE vim_command(VALUE self, VALUE str)
{
    do_cmdline_cmd((char_u *)STR2CSTR(str));
    return Qnil;
}

static VALUE vim_evaluate(VALUE self, VALUE str)
{
#ifdef FEAT_EVAL
    char_u *value = eval_to_string((char_u *)STR2CSTR(str), NULL, TRUE);

    if (value != NULL)
    {
	VALUE val = rb_str_new2((char *)value);
	vim_free(value);
	return val;
    }
    else
#endif
	return Qnil;
}

static VALUE buffer_new(buf_T *buf)
{
    if (buf->b_ruby_ref)
    {
	return (VALUE) buf->b_ruby_ref;
    }
    else
    {
	VALUE obj = Data_Wrap_Struct(cBuffer, 0, 0, buf);
	buf->b_ruby_ref = (void *) obj;
	rb_hash_aset(objtbl, rb_obj_id(obj), obj);
	return obj;
    }
}

static buf_T *get_buf(VALUE obj)
{
    buf_T *buf;

    Data_Get_Struct(obj, buf_T, buf);
    if (buf == NULL)
	rb_raise(eDeletedBufferError, "attempt to refer to deleted buffer");
    return buf;
}

static VALUE buffer_s_current()
{
    return buffer_new(curbuf);
}

static VALUE buffer_s_count()
{
    buf_T *b;
    int n = 0;

    for (b = firstbuf; b != NULL; b = b->b_next)
    {
	/*  Deleted buffers should not be counted
	 *    SegPhault - 01/07/05 */
	if (b->b_p_bl)
	    n++;
    }

    return INT2NUM(n);
}

static VALUE buffer_s_aref(VALUE self, VALUE num)
{
    buf_T *b;
    int n = NUM2INT(num);

    for (b = firstbuf; b != NULL; b = b->b_next)
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
    if (n > 0 && n <= buf->b_ml.ml_line_count)
    {
	char *line = (char *)ml_get_buf(buf, n, FALSE);
	return line ? rb_str_new2(line) : Qnil;
    }
    rb_raise(rb_eIndexError, "index %d out of buffer", n);
    return Qnil; /* For stop warning */
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
    char	*line = STR2CSTR(str);
    aco_save_T	aco;

    if (n > 0 && n <= buf->b_ml.ml_line_count && line != NULL)
    {
	/* set curwin/curbuf for "buf" and save some things */
	aucmd_prepbuf(&aco, buf);

	if (u_savesub(n) == OK) {
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
	rb_raise(rb_eIndexError, "index %d out of buffer", n);
	return Qnil; /* For stop warning */
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

	if (u_savedel(n, 1) == OK) {
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
	rb_raise(rb_eIndexError, "index %d out of buffer", n);
    }
    return Qnil;
}

static VALUE buffer_append(VALUE self, VALUE num, VALUE str)
{
    buf_T	*buf = get_buf(self);
    char	*line = STR2CSTR(str);
    long	n = NUM2LONG(num);
    aco_save_T	aco;

    if (n >= 0 && n <= buf->b_ml.ml_line_count && line != NULL)
    {
	/* set curwin/curbuf for "buf" and save some things */
	aucmd_prepbuf(&aco, buf);

	if (u_inssub(n + 1) == OK) {
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
    else {
	rb_raise(rb_eIndexError, "index %d out of buffer", n);
    }
    return str;
}

static VALUE window_new(win_T *win)
{
    if (win->w_ruby_ref)
    {
	return (VALUE) win->w_ruby_ref;
    }
    else
    {
	VALUE obj = Data_Wrap_Struct(cVimWindow, 0, 0, win);
	win->w_ruby_ref = (void *) obj;
	rb_hash_aset(objtbl, rb_obj_id(obj), obj);
	return obj;
    }
}

static win_T *get_win(VALUE obj)
{
    win_T *win;

    Data_Get_Struct(obj, win_T, win);
    if (win == NULL)
	rb_raise(eDeletedWindowError, "attempt to refer to deleted window");
    return win;
}

static VALUE window_s_current()
{
    return window_new(curwin);
}

/*
 * Added line manipulation functions
 *    SegPhault - 03/07/05
 */
static VALUE line_s_current()
{
    return get_buffer_line(curbuf, curwin->w_cursor.lnum);
}

static VALUE set_current_line(VALUE self, VALUE str)
{
    return set_buffer_line(curbuf, curwin->w_cursor.lnum, str);
}

static VALUE current_line_number()
{
    return INT2FIX((int)curwin->w_cursor.lnum);
}



static VALUE window_s_count()
{
#ifdef FEAT_WINDOWS
    win_T	*w;
    int n = 0;

    for (w = firstwin; w != NULL; w = w->w_next)
	n++;
    return INT2NUM(n);
#else
    return INT2NUM(1);
#endif
}

static VALUE window_s_aref(VALUE self, VALUE num)
{
    win_T *w;
    int n = NUM2INT(num);

#ifndef FEAT_WINDOWS
    w = curwin;
#else
    for (w = firstwin; w != NULL; w = w->w_next, --n)
#endif
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

static VALUE window_width(VALUE self)
{
    win_T *win = get_win(self);

    return INT2NUM(win->w_width);
}

static VALUE window_set_width(VALUE self, VALUE width)
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
    if (RARRAY(pos)->len != 2)
	rb_raise(rb_eArgError, "array length must be 2");
    lnum = RARRAY(pos)->ptr[0];
    col = RARRAY(pos)->ptr[1];
    win->w_cursor.lnum = NUM2LONG(lnum);
    win->w_cursor.col = NUM2UINT(col);
    check_cursor();		    /* put cursor on an existing line */
    update_screen(NOT_VALID);
    return Qnil;
}

static VALUE f_p(int argc, VALUE *argv, VALUE self)
{
    int i;
    VALUE str = rb_str_new("", 0);

    for (i = 0; i < argc; i++) {
	if (i > 0) rb_str_cat(str, ", ", 2);
	rb_str_concat(str, rb_inspect(argv[i]));
    }
    MSG(RSTRING(str)->ptr);
    return Qnil;
}

static void ruby_io_init(void)
{
#ifndef DYNAMIC_RUBY
    RUBYEXTERN VALUE rb_stdout;
#endif

    rb_stdout = rb_obj_alloc(rb_cObject);
    rb_define_singleton_method(rb_stdout, "write", vim_message, 1);
    rb_define_global_function("p", f_p, -1);
}

static void ruby_vim_init(void)
{
    objtbl = rb_hash_new();
    rb_global_variable(&objtbl);

    /* The Vim module used to be called "VIM", but "Vim" is better.  Make an
     * alias "VIM" for backwards compatiblity. */
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
