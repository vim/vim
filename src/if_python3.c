/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * Python extensions by Paul Moore.
 * Changes for Unix by David Leonard.
 *
 * This consists of four parts:
 * 1. Python interpreter main program
 * 2. Python output stream: writes output via [e]msg().
 * 3. Implementation of the Vim module for Python
 * 4. Utility functions for handling the interface between Vim and Python.
 */

/*
 * Roland Puntaier 2009/sept/16:
 * Adaptations to support both python3.x and python2.x
 */

/* uncomment this if used with the debug version of python */
/* #define Py_DEBUG */

#include "vim.h"

#include <limits.h>

/* Python.h defines _POSIX_THREADS itself (if needed) */
#ifdef _POSIX_THREADS
# undef _POSIX_THREADS
#endif

#if defined(_WIN32) && defined(HAVE_FCNTL_H)
# undef HAVE_FCNTL_H
#endif

#ifdef _DEBUG
# undef _DEBUG
#endif

#define PY_SSIZE_T_CLEAN

#ifdef F_BLANK
# undef F_BLANK
#endif

#ifdef HAVE_STDARG_H
# undef HAVE_STDARG_H   /* Python's config.h defines it as well. */
#endif
#ifdef _POSIX_C_SOURCE  /* defined in feature.h */
# undef _POSIX_C_SOURCE
#endif
#ifdef _XOPEN_SOURCE
# undef _XOPEN_SOURCE	/* pyconfig.h defines it as well. */
#endif

#include <Python.h>
#if defined(MACOS) && !defined(MACOS_X_UNIX)
# include "macglue.h"
# include <CodeFragments.h>
#endif
#undef main /* Defined in python.h - aargh */
#undef HAVE_FCNTL_H /* Clash with os_win32.h */

static void init_structs(void);

#define PyInt Py_ssize_t
#define PyString_Check(obj) PyUnicode_Check(obj)
#define PyString_AsString(obj) _PyUnicode_AsString(obj)
#define PyString_Size(obj) PyUnicode_GET_SIZE(obj)
#define PyString_FromString(repr) PyUnicode_FromString(repr)

#if defined(DYNAMIC_PYTHON3) || defined(PROTO)

# ifndef WIN3264
#  include <dlfcn.h>
#  define FARPROC void*
#  define HINSTANCE void*
#  if defined(PY_NO_RTLD_GLOBAL) && defined(PY3_NO_RTLD_GLOBAL)
#   define load_dll(n) dlopen((n), RTLD_LAZY)
#  else
#   define load_dll(n) dlopen((n), RTLD_LAZY|RTLD_GLOBAL)
#  endif
#  define close_dll dlclose
#  define symbol_from_dll dlsym
# else
#  define load_dll vimLoadLib
#  define close_dll FreeLibrary
#  define symbol_from_dll GetProcAddress
# endif
/*
 * Wrapper defines
 */
# undef PyArg_Parse
# define PyArg_Parse py3_PyArg_Parse
# undef PyArg_ParseTuple
# define PyArg_ParseTuple py3_PyArg_ParseTuple
# define PyDict_SetItemString py3_PyDict_SetItemString
# define PyErr_BadArgument py3_PyErr_BadArgument
# define PyErr_Clear py3_PyErr_Clear
# define PyErr_NoMemory py3_PyErr_NoMemory
# define PyErr_Occurred py3_PyErr_Occurred
# define PyErr_SetNone py3_PyErr_SetNone
# define PyErr_SetString py3_PyErr_SetString
# define PyEval_InitThreads py3_PyEval_InitThreads
# define PyEval_RestoreThread py3_PyEval_RestoreThread
# define PyEval_SaveThread py3_PyEval_SaveThread
# define PyGILState_Ensure py3_PyGILState_Ensure
# define PyGILState_Release py3_PyGILState_Release
# define PyLong_AsLong py3_PyLong_AsLong
# define PyLong_FromLong py3_PyLong_FromLong
# define PyList_GetItem py3_PyList_GetItem
# define PyList_Append py3_PyList_Append
# define PyList_New py3_PyList_New
# define PyList_SetItem py3_PyList_SetItem
# define PyList_Size py3_PyList_Size
# define PySlice_GetIndicesEx py3_PySlice_GetIndicesEx
# define PyImport_ImportModule py3_PyImport_ImportModule
# define PyObject_Init py3__PyObject_Init
# define PyDict_New py3_PyDict_New
# define PyDict_GetItemString py3_PyDict_GetItemString
# define PyModule_GetDict py3_PyModule_GetDict
#undef PyRun_SimpleString
# define PyRun_SimpleString py3_PyRun_SimpleString
# define PySys_SetObject py3_PySys_SetObject
# define PySys_SetArgv py3_PySys_SetArgv
# define PyType_Type (*py3_PyType_Type)
# define PyType_Ready py3_PyType_Ready
#undef Py_BuildValue
# define Py_BuildValue py3_Py_BuildValue
# define Py_SetPythonHome py3_Py_SetPythonHome
# define Py_Initialize py3_Py_Initialize
# define Py_Finalize py3_Py_Finalize
# define Py_IsInitialized py3_Py_IsInitialized
# define _Py_NoneStruct (*py3__Py_NoneStruct)
# define PyModule_AddObject py3_PyModule_AddObject
# define PyImport_AppendInittab py3_PyImport_AppendInittab
# define _PyUnicode_AsString py3__PyUnicode_AsString
# define PyObject_GenericGetAttr py3_PyObject_GenericGetAttr
# define PySlice_Type (*py3_PySlice_Type)
# ifdef Py_DEBUG
#  define _Py_NegativeRefcount py3__Py_NegativeRefcount
#  define _Py_RefTotal (*py3__Py_RefTotal)
#  define _Py_Dealloc py3__Py_Dealloc
#  define _PyObject_DebugMalloc py3__PyObject_DebugMalloc
#  define _PyObject_DebugFree py3__PyObject_DebugFree
# else
#  define PyObject_Malloc py3_PyObject_Malloc
#  define PyObject_Free py3_PyObject_Free
# endif
# define PyType_GenericAlloc py3_PyType_GenericAlloc
# define PyType_GenericNew py3_PyType_GenericNew
# define PyModule_Create2 py3_PyModule_Create2
# undef PyUnicode_FromString
# define PyUnicode_FromString py3_PyUnicode_FromString
# undef PyUnicode_FromStringAndSize
# define PyUnicode_FromStringAndSize py3_PyUnicode_FromStringAndSize

# ifdef Py_DEBUG
#  undef PyObject_NEW
#  define PyObject_NEW(type, typeobj) \
( (type *) PyObject_Init( \
	(PyObject *) _PyObject_DebugMalloc( _PyObject_SIZE(typeobj) ), (typeobj)) )
# endif

/*
 * Pointers for dynamic link
 */
static int (*py3_PySys_SetArgv)(int, wchar_t **);
static void (*py3_Py_SetPythonHome)(wchar_t *home);
static void (*py3_Py_Initialize)(void);
static PyObject* (*py3_PyList_New)(Py_ssize_t size);
static PyGILState_STATE (*py3_PyGILState_Ensure)(void);
static void (*py3_PyGILState_Release)(PyGILState_STATE);
static int (*py3_PySys_SetObject)(char *, PyObject *);
static PyObject* (*py3_PyList_Append)(PyObject *, PyObject *);
static Py_ssize_t (*py3_PyList_Size)(PyObject *);
static int (*py3_PySlice_GetIndicesEx)(PySliceObject *r, Py_ssize_t length,
		     Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step, Py_ssize_t *slicelength);
static PyObject* (*py3_PyErr_NoMemory)(void);
static void (*py3_Py_Finalize)(void);
static void (*py3_PyErr_SetString)(PyObject *, const char *);
static int (*py3_PyRun_SimpleString)(char *);
static PyObject* (*py3_PyList_GetItem)(PyObject *, Py_ssize_t);
static PyObject* (*py3_PyImport_ImportModule)(const char *);
static int (*py3_PyErr_BadArgument)(void);
static PyTypeObject* py3_PyType_Type;
static PyObject* (*py3_PyErr_Occurred)(void);
static PyObject* (*py3_PyModule_GetDict)(PyObject *);
static int (*py3_PyList_SetItem)(PyObject *, Py_ssize_t, PyObject *);
static PyObject* (*py3_PyDict_GetItemString)(PyObject *, const char *);
static PyObject* (*py3_PyLong_FromLong)(long);
static PyObject* (*py3_PyDict_New)(void);
static PyObject* (*py3_Py_BuildValue)(char *, ...);
static int (*py3_PyType_Ready)(PyTypeObject *type);
static int (*py3_PyDict_SetItemString)(PyObject *dp, char *key, PyObject *item);
static PyObject* (*py3_PyUnicode_FromString)(const char *u);
static PyObject* (*py3_PyUnicode_FromStringAndSize)(const char *u, Py_ssize_t size);
static long (*py3_PyLong_AsLong)(PyObject *);
static void (*py3_PyErr_SetNone)(PyObject *);
static void (*py3_PyEval_InitThreads)(void);
static void(*py3_PyEval_RestoreThread)(PyThreadState *);
static PyThreadState*(*py3_PyEval_SaveThread)(void);
static int (*py3_PyArg_Parse)(PyObject *, char *, ...);
static int (*py3_PyArg_ParseTuple)(PyObject *, char *, ...);
static int (*py3_Py_IsInitialized)(void);
static void (*py3_PyErr_Clear)(void);
static PyObject*(*py3__PyObject_Init)(PyObject *, PyTypeObject *);
static PyObject* py3__Py_NoneStruct;
static int (*py3_PyModule_AddObject)(PyObject *m, const char *name, PyObject *o);
static int (*py3_PyImport_AppendInittab)(const char *name, PyObject* (*initfunc)(void));
static char* (*py3__PyUnicode_AsString)(PyObject *unicode);
static PyObject* (*py3_PyObject_GenericGetAttr)(PyObject *obj, PyObject *name);
static PyObject* (*py3_PyModule_Create2)(struct PyModuleDef* module, int module_api_version);
static PyObject* (*py3_PyType_GenericAlloc)(PyTypeObject *type, Py_ssize_t nitems);
static PyObject* (*py3_PyType_GenericNew)(PyTypeObject *type, PyObject *args, PyObject *kwds);
static PyTypeObject* py3_PySlice_Type;
# ifdef Py_DEBUG
    static void (*py3__Py_NegativeRefcount)(const char *fname, int lineno, PyObject *op);
    static Py_ssize_t* py3__Py_RefTotal;
    static void (*py3__Py_Dealloc)(PyObject *obj);
    static void (*py3__PyObject_DebugFree)(void*);
    static void* (*py3__PyObject_DebugMalloc)(size_t);
# else
    static void (*py3_PyObject_Free)(void*);
    static void* (*py3_PyObject_Malloc)(size_t);
# endif

static HINSTANCE hinstPy3 = 0; /* Instance of python.dll */

/* Imported exception objects */
static PyObject *p3imp_PyExc_AttributeError;
static PyObject *p3imp_PyExc_IndexError;
static PyObject *p3imp_PyExc_KeyboardInterrupt;
static PyObject *p3imp_PyExc_TypeError;
static PyObject *p3imp_PyExc_ValueError;

# define PyExc_AttributeError p3imp_PyExc_AttributeError
# define PyExc_IndexError p3imp_PyExc_IndexError
# define PyExc_KeyboardInterrupt p3imp_PyExc_KeyboardInterrupt
# define PyExc_TypeError p3imp_PyExc_TypeError
# define PyExc_ValueError p3imp_PyExc_ValueError

/*
 * Table of name to function pointer of python.
 */
# define PYTHON_PROC FARPROC
static struct
{
    char *name;
    PYTHON_PROC *ptr;
} py3_funcname_table[] =
{
    {"PySys_SetArgv", (PYTHON_PROC*)&py3_PySys_SetArgv},
    {"Py_SetPythonHome", (PYTHON_PROC*)&py3_Py_SetPythonHome},
    {"Py_Initialize", (PYTHON_PROC*)&py3_Py_Initialize},
    {"PyArg_ParseTuple", (PYTHON_PROC*)&py3_PyArg_ParseTuple},
    {"PyList_New", (PYTHON_PROC*)&py3_PyList_New},
    {"PyGILState_Ensure", (PYTHON_PROC*)&py3_PyGILState_Ensure},
    {"PyGILState_Release", (PYTHON_PROC*)&py3_PyGILState_Release},
    {"PySys_SetObject", (PYTHON_PROC*)&py3_PySys_SetObject},
    {"PyList_Append", (PYTHON_PROC*)&py3_PyList_Append},
    {"PyList_Size", (PYTHON_PROC*)&py3_PyList_Size},
    {"PySlice_GetIndicesEx", (PYTHON_PROC*)&py3_PySlice_GetIndicesEx},
    {"PyErr_NoMemory", (PYTHON_PROC*)&py3_PyErr_NoMemory},
    {"Py_Finalize", (PYTHON_PROC*)&py3_Py_Finalize},
    {"PyErr_SetString", (PYTHON_PROC*)&py3_PyErr_SetString},
    {"PyRun_SimpleString", (PYTHON_PROC*)&py3_PyRun_SimpleString},
    {"PyList_GetItem", (PYTHON_PROC*)&py3_PyList_GetItem},
    {"PyImport_ImportModule", (PYTHON_PROC*)&py3_PyImport_ImportModule},
    {"PyErr_BadArgument", (PYTHON_PROC*)&py3_PyErr_BadArgument},
    {"PyType_Type", (PYTHON_PROC*)&py3_PyType_Type},
    {"PyErr_Occurred", (PYTHON_PROC*)&py3_PyErr_Occurred},
    {"PyModule_GetDict", (PYTHON_PROC*)&py3_PyModule_GetDict},
    {"PyList_SetItem", (PYTHON_PROC*)&py3_PyList_SetItem},
    {"PyDict_GetItemString", (PYTHON_PROC*)&py3_PyDict_GetItemString},
    {"PyLong_FromLong", (PYTHON_PROC*)&py3_PyLong_FromLong},
    {"PyDict_New", (PYTHON_PROC*)&py3_PyDict_New},
    {"Py_BuildValue", (PYTHON_PROC*)&py3_Py_BuildValue},
    {"PyType_Ready", (PYTHON_PROC*)&py3_PyType_Ready},
    {"PyDict_SetItemString", (PYTHON_PROC*)&py3_PyDict_SetItemString},
    {"PyLong_AsLong", (PYTHON_PROC*)&py3_PyLong_AsLong},
    {"PyErr_SetNone", (PYTHON_PROC*)&py3_PyErr_SetNone},
    {"PyEval_InitThreads", (PYTHON_PROC*)&py3_PyEval_InitThreads},
    {"PyEval_RestoreThread", (PYTHON_PROC*)&py3_PyEval_RestoreThread},
    {"PyEval_SaveThread", (PYTHON_PROC*)&py3_PyEval_SaveThread},
    {"PyArg_Parse", (PYTHON_PROC*)&py3_PyArg_Parse},
    {"PyArg_ParseTuple", (PYTHON_PROC*)&py3_PyArg_ParseTuple},
    {"Py_IsInitialized", (PYTHON_PROC*)&py3_Py_IsInitialized},
    {"_Py_NoneStruct", (PYTHON_PROC*)&py3__Py_NoneStruct},
    {"PyErr_Clear", (PYTHON_PROC*)&py3_PyErr_Clear},
    {"PyObject_Init", (PYTHON_PROC*)&py3__PyObject_Init},
    {"PyModule_AddObject", (PYTHON_PROC*)&py3_PyModule_AddObject},
    {"PyImport_AppendInittab", (PYTHON_PROC*)&py3_PyImport_AppendInittab},
    {"_PyUnicode_AsString", (PYTHON_PROC*)&py3__PyUnicode_AsString},
    {"PyObject_GenericGetAttr", (PYTHON_PROC*)&py3_PyObject_GenericGetAttr},
    {"PyModule_Create2", (PYTHON_PROC*)&py3_PyModule_Create2},
    {"PyType_GenericAlloc", (PYTHON_PROC*)&py3_PyType_GenericAlloc},
    {"PyType_GenericNew", (PYTHON_PROC*)&py3_PyType_GenericNew},
    {"PySlice_Type", (PYTHON_PROC*)&py3_PySlice_Type},
# ifdef Py_DEBUG
    {"_Py_NegativeRefcount", (PYTHON_PROC*)&py3__Py_NegativeRefcount},
    {"_Py_RefTotal", (PYTHON_PROC*)&py3__Py_RefTotal},
    {"_Py_Dealloc", (PYTHON_PROC*)&py3__Py_Dealloc},
    {"_PyObject_DebugFree", (PYTHON_PROC*)&py3__PyObject_DebugFree},
    {"_PyObject_DebugMalloc", (PYTHON_PROC*)&py3__PyObject_DebugMalloc},
# else
    {"PyObject_Malloc", (PYTHON_PROC*)&py3_PyObject_Malloc},
    {"PyObject_Free", (PYTHON_PROC*)&py3_PyObject_Free},
# endif
    {"", NULL},
};

/*
 * Free python.dll
 */
    static void
end_dynamic_python3(void)
{
    if (hinstPy3 != 0)
    {
	close_dll(hinstPy3);
	hinstPy3 = 0;
    }
}

/*
 * Load library and get all pointers.
 * Parameter 'libname' provides name of DLL.
 * Return OK or FAIL.
 */
    static int
py3_runtime_link_init(char *libname, int verbose)
{
    int i;
    void *ucs_from_string, *ucs_from_string_and_size;

# if !(defined(PY_NO_RTLD_GLOBAL) && defined(PY3_NO_RTLD_GLOBAL)) && defined(UNIX) && defined(FEAT_PYTHON)
    /* Can't have Python and Python3 loaded at the same time.
     * It cause a crash, because RTLD_GLOBAL is needed for
     * standard C extension libraries of one or both python versions. */
    if (python_loaded())
    {
	EMSG(_("E837: This Vim cannot execute :py3 after using :python"));
	return FAIL;
    }
# endif

    if (hinstPy3 != 0)
	return OK;
    hinstPy3 = load_dll(libname);

    if (!hinstPy3)
    {
	if (verbose)
	    EMSG2(_(e_loadlib), libname);
	return FAIL;
    }

    for (i = 0; py3_funcname_table[i].ptr; ++i)
    {
	if ((*py3_funcname_table[i].ptr = symbol_from_dll(hinstPy3,
			py3_funcname_table[i].name)) == NULL)
	{
	    close_dll(hinstPy3);
	    hinstPy3 = 0;
	    if (verbose)
		EMSG2(_(e_loadfunc), py3_funcname_table[i].name);
	    return FAIL;
	}
    }

    /* Load unicode functions separately as only the ucs2 or the ucs4 functions
     * will be present in the library. */
    ucs_from_string = symbol_from_dll(hinstPy3, "PyUnicodeUCS2_FromString");
    ucs_from_string_and_size = symbol_from_dll(hinstPy3,
	    "PyUnicodeUCS2_FromStringAndSize");
    if (!ucs_from_string || !ucs_from_string_and_size)
    {
	ucs_from_string = symbol_from_dll(hinstPy3,
		"PyUnicodeUCS4_FromString");
	ucs_from_string_and_size = symbol_from_dll(hinstPy3,
		"PyUnicodeUCS4_FromStringAndSize");
    }
    if (ucs_from_string && ucs_from_string_and_size)
    {
	py3_PyUnicode_FromString = ucs_from_string;
	py3_PyUnicode_FromStringAndSize = ucs_from_string_and_size;
    }
    else
    {
	close_dll(hinstPy3);
	hinstPy3 = 0;
	if (verbose)
	    EMSG2(_(e_loadfunc), "PyUnicode_UCSX_*");
	return FAIL;
    }

    return OK;
}

/*
 * If python is enabled (there is installed python on Windows system) return
 * TRUE, else FALSE.
 */
    int
python3_enabled(int verbose)
{
    return py3_runtime_link_init(DYNAMIC_PYTHON3_DLL, verbose) == OK;
}

/* Load the standard Python exceptions - don't import the symbols from the
 * DLL, as this can cause errors (importing data symbols is not reliable).
 */
static void get_py3_exceptions __ARGS((void));

    static void
get_py3_exceptions()
{
    PyObject *exmod = PyImport_ImportModule("builtins");
    PyObject *exdict = PyModule_GetDict(exmod);
    p3imp_PyExc_AttributeError = PyDict_GetItemString(exdict, "AttributeError");
    p3imp_PyExc_IndexError = PyDict_GetItemString(exdict, "IndexError");
    p3imp_PyExc_KeyboardInterrupt = PyDict_GetItemString(exdict, "KeyboardInterrupt");
    p3imp_PyExc_TypeError = PyDict_GetItemString(exdict, "TypeError");
    p3imp_PyExc_ValueError = PyDict_GetItemString(exdict, "ValueError");
    Py_XINCREF(p3imp_PyExc_AttributeError);
    Py_XINCREF(p3imp_PyExc_IndexError);
    Py_XINCREF(p3imp_PyExc_KeyboardInterrupt);
    Py_XINCREF(p3imp_PyExc_TypeError);
    Py_XINCREF(p3imp_PyExc_ValueError);
    Py_XDECREF(exmod);
}
#endif /* DYNAMIC_PYTHON3 */

static PyObject *BufferNew (buf_T *);
static PyObject *WindowNew(win_T *);
static PyObject *LineToString(const char *);

static PyTypeObject RangeType;

/*
 * Include the code shared with if_python.c
 */
#include "if_py_both.h"

    static void
call_PyObject_Free(void *p)
{
#ifdef Py_DEBUG
    _PyObject_DebugFree(p);
#else
    PyObject_Free(p);
#endif
}

    static PyObject *
call_PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return PyType_GenericNew(type,args,kwds);
}

    static PyObject *
call_PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)
{
    return PyType_GenericAlloc(type,nitems);
}

/******************************************************
 * Internal function prototypes.
 */

static Py_ssize_t RangeStart;
static Py_ssize_t RangeEnd;

static int PythonIO_Init(void);
static void PythonIO_Fini(void);
PyMODINIT_FUNC Py3Init_vim(void);

/******************************************************
 * 1. Python interpreter main program.
 */

static int py3initialised = 0;

static PyGILState_STATE pygilstate = PyGILState_UNLOCKED;

    void
python3_end()
{
    static int recurse = 0;

    /* If a crash occurs while doing this, don't try again. */
    if (recurse != 0)
	return;

    ++recurse;

#ifdef DYNAMIC_PYTHON3
    if (hinstPy3)
#endif
    if (Py_IsInitialized())
    {
	// acquire lock before finalizing
	pygilstate = PyGILState_Ensure();

	PythonIO_Fini();
	Py_Finalize();
    }

#ifdef DYNAMIC_PYTHON3
    end_dynamic_python3();
#endif

    --recurse;
}

#if (defined(DYNAMIC_PYTHON) && defined(FEAT_PYTHON)) || defined(PROTO)
    int
python3_loaded()
{
    return (hinstPy3 != 0);
}
#endif

    static int
Python3_Init(void)
{
    if (!py3initialised)
    {
#ifdef DYNAMIC_PYTHON3
	if (!python3_enabled(TRUE))
	{
	    EMSG(_("E263: Sorry, this command is disabled, the Python library could not be loaded."));
	    goto fail;
	}
#endif

	init_structs();


#ifdef PYTHON3_HOME
	Py_SetPythonHome(PYTHON3_HOME);
#endif

#if !defined(MACOS) || defined(MACOS_X_UNIX)
	Py_Initialize();
#else
	PyMac_Initialize();
#endif
	/* initialise threads, must be after Py_Initialize() */
	PyEval_InitThreads();

#ifdef DYNAMIC_PYTHON3
	get_py3_exceptions();
#endif

	if (PythonIO_Init())
	    goto fail;

	PyImport_AppendInittab("vim", Py3Init_vim);

	/* Remove the element from sys.path that was added because of our
	 * argv[0] value in Py3Init_vim().  Previously we used an empty
	 * string, but dependinding on the OS we then get an empty entry or
	 * the current directory in sys.path. */
	PyRun_SimpleString("import sys; sys.path = list(filter(lambda x: x != '/must>not&exist', sys.path))");

	// lock is created and acquired in PyEval_InitThreads() and thread
	// state is created in Py_Initialize()
	// there _PyGILState_NoteThreadState() also sets gilcounter to 1
	// (python must have threads enabled!)
	// so the following does both: unlock GIL and save thread state in TLS
	// without deleting thread state
	PyGILState_Release(pygilstate);

	py3initialised = 1;
    }

    return 0;

fail:
    /* We call PythonIO_Flush() here to print any Python errors.
     * This is OK, as it is possible to call this function even
     * if PythonIO_Init() has not completed successfully (it will
     * not do anything in this case).
     */
    PythonIO_Flush();
    return -1;
}

/*
 * External interface
 */
    static void
DoPy3Command(exarg_T *eap, const char *cmd)
{
#if defined(MACOS) && !defined(MACOS_X_UNIX)
    GrafPtr		oldPort;
#endif
#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    char		*saved_locale;
#endif

#if defined(MACOS) && !defined(MACOS_X_UNIX)
    GetPort(&oldPort);
    /* Check if the Python library is available */
    if ((Ptr)PyMac_Initialize == (Ptr)kUnresolvedCFragSymbolAddress)
	goto theend;
#endif
    if (Python3_Init())
	goto theend;

    RangeStart = eap->line1;
    RangeEnd = eap->line2;
    Python_Release_Vim();	    /* leave vim */

#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    /* Python only works properly when the LC_NUMERIC locale is "C". */
    saved_locale = setlocale(LC_NUMERIC, NULL);
    if (saved_locale == NULL || STRCMP(saved_locale, "C") == 0)
	saved_locale = NULL;
    else
    {
	/* Need to make a copy, value may change when setting new locale. */
	saved_locale = (char *)vim_strsave((char_u *)saved_locale);
	(void)setlocale(LC_NUMERIC, "C");
    }
#endif

    pygilstate = PyGILState_Ensure();

    PyRun_SimpleString((char *)(cmd));

    PyGILState_Release(pygilstate);

#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    if (saved_locale != NULL)
    {
	(void)setlocale(LC_NUMERIC, saved_locale);
	vim_free(saved_locale);
    }
#endif

    Python_Lock_Vim();		    /* enter vim */
    PythonIO_Flush();
#if defined(MACOS) && !defined(MACOS_X_UNIX)
    SetPort(oldPort);
#endif

theend:
    return;	    /* keeps lint happy */
}

/*
 * ":py3"
 */
    void
ex_py3(exarg_T *eap)
{
    char_u *script;

    script = script_get(eap, eap->arg);
    if (!eap->skip)
    {
	if (script == NULL)
	    DoPy3Command(eap, (char *)eap->arg);
	else
	    DoPy3Command(eap, (char *)script);
    }
    vim_free(script);
}

#define BUFFER_SIZE 2048

/*
 * ":py3file"
 */
    void
ex_py3file(exarg_T *eap)
{
    static char buffer[BUFFER_SIZE];
    const char *file;
    char *p;
    int i;

    /* Have to do it like this. PyRun_SimpleFile requires you to pass a
     * stdio file pointer, but Vim and the Python DLL are compiled with
     * different options under Windows, meaning that stdio pointers aren't
     * compatible between the two. Yuk.
     *
     * construct: exec(compile(open('a_filename').read(), 'a_filename', 'exec'))
     *
     * We need to escape any backslashes or single quotes in the file name, so that
     * Python won't mangle the file name.
     */

    strcpy(buffer, "exec(compile(open('");
    p = buffer + 19; /* size of "exec(compile(open('" */

    for (i=0; i<2; ++i)
    {
	file = (char *)eap->arg;
	while (*file && p < buffer + (BUFFER_SIZE - 3))
	{
	    if (*file == '\\' || *file == '\'')
		*p++ = '\\';
	    *p++ = *file++;
	}
	/* If we didn't finish the file name, we hit a buffer overflow */
	if (*file != '\0')
	    return;
	if (i==0)
	{
	    strcpy(p,"').read(),'");
	    p += 11;
	}
	else
	{
	    strcpy(p,"','exec'))");
	    p += 10;
	}
    }


    /* Execute the file */
    DoPy3Command(eap, buffer);
}

/******************************************************
 * 2. Python output stream: writes output via [e]msg().
 */

/* Implementation functions
 */

    static PyObject *
OutputGetattro(PyObject *self, PyObject *nameobj)
{
    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    if (strcmp(name, "softspace") == 0)
	return PyLong_FromLong(((OutputObject *)(self))->softspace);

    return PyObject_GenericGetAttr(self, nameobj);
}

    static int
OutputSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    if (val == NULL) {
	PyErr_SetString(PyExc_AttributeError, _("can't delete OutputObject attributes"));
	return -1;
    }

    if (strcmp(name, "softspace") == 0)
    {
	if (!PyLong_Check(val)) {
	    PyErr_SetString(PyExc_TypeError, _("softspace must be an integer"));
	    return -1;
	}

	((OutputObject *)(self))->softspace = PyLong_AsLong(val);
	return 0;
    }

    PyErr_SetString(PyExc_AttributeError, _("invalid attribute"));
    return -1;
}

/***************/

    static int
PythonIO_Init(void)
{
    PyType_Ready(&OutputType);
    return PythonIO_Init_io();
}

    static void
PythonIO_Fini(void)
{
    PySys_SetObject("stdout", NULL);
    PySys_SetObject("stderr", NULL);
}

/******************************************************
 * 3. Implementation of the Vim module for Python
 */

/* Window type - Implementation functions
 * --------------------------------------
 */

#define WindowType_Check(obj) ((obj)->ob_base.ob_type == &WindowType)

/* Buffer type - Implementation functions
 * --------------------------------------
 */

#define BufferType_Check(obj) ((obj)->ob_base.ob_type == &BufferType)

static Py_ssize_t BufferLength(PyObject *);
static PyObject *BufferItem(PyObject *, Py_ssize_t);
static Py_ssize_t BufferAsItem(PyObject *, Py_ssize_t, PyObject *);
static PyObject* BufferSubscript(PyObject *self, PyObject* idx);


/* Line range type - Implementation functions
 * --------------------------------------
 */

#define RangeType_Check(obj) ((obj)->ob_base.ob_type == &RangeType)

static PyObject* RangeSubscript(PyObject *self, PyObject* idx);
static Py_ssize_t RangeAsItem(PyObject *, Py_ssize_t, PyObject *);

/* Current objects type - Implementation functions
 * -----------------------------------------------
 */

static PySequenceMethods BufferAsSeq = {
    (lenfunc)		BufferLength,	    /* sq_length,    len(x)   */
    (binaryfunc)	0,		    /* sq_concat,    x+y      */
    (ssizeargfunc)	0,		    /* sq_repeat,    x*n      */
    (ssizeargfunc)	BufferItem,	    /* sq_item,      x[i]     */
    0,					    /* was_sq_slice,	 x[i:j]   */
    (ssizeobjargproc)	BufferAsItem,	    /* sq_ass_item,  x[i]=v   */
    0,					    /* sq_ass_slice, x[i:j]=v */
    0,					    /* sq_contains */
    0,					    /* sq_inplace_concat */
    0,					    /* sq_inplace_repeat */
};

PyMappingMethods BufferAsMapping = {
    /* mp_length	*/ (lenfunc)BufferLength,
    /* mp_subscript     */ (binaryfunc)BufferSubscript,
    /* mp_ass_subscript */ (objobjargproc)0,
};


/* Buffer object - Definitions
 */

static PyTypeObject BufferType;

    static PyObject *
BufferNew(buf_T *buf)
{
    /* We need to handle deletion of buffers underneath us.
     * If we add a "b_python3_ref" field to the buf_T structure,
     * then we can get at it in buf_freeall() in vim. We then
     * need to create only ONE Python object per buffer - if
     * we try to create a second, just INCREF the existing one
     * and return it. The (single) Python object referring to
     * the buffer is stored in "b_python3_ref".
     * Question: what to do on a buf_freeall(). We'll probably
     * have to either delete the Python object (DECREF it to
     * zero - a bad idea, as it leaves dangling refs!) or
     * set the buf_T * value to an invalid value (-1?), which
     * means we need checks in all access functions... Bah.
     */

    BufferObject *self;

    if (buf->b_python3_ref != NULL)
    {
	self = buf->b_python3_ref;
	Py_INCREF(self);
    }
    else
    {
	self = PyObject_NEW(BufferObject, &BufferType);
	buf->b_python3_ref = self;
	if (self == NULL)
	    return NULL;
	self->buf = buf;
    }

    return (PyObject *)(self);
}

    static void
BufferDestructor(PyObject *self)
{
    BufferObject *this = (BufferObject *)(self);

    if (this->buf && this->buf != INVALID_BUFFER_VALUE)
	this->buf->b_python3_ref = NULL;
}

    static PyObject *
BufferGetattro(PyObject *self, PyObject*nameobj)
{
    BufferObject *this = (BufferObject *)(self);

    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    if (CheckBuffer(this))
	return NULL;

    if (strcmp(name, "name") == 0)
	return Py_BuildValue("s", this->buf->b_ffname);
    else if (strcmp(name, "number") == 0)
	return Py_BuildValue("n", this->buf->b_fnum);
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[ss]", "name", "number");
    else
	return PyObject_GenericGetAttr(self, nameobj);
}

    static PyObject *
BufferRepr(PyObject *self)
{
    static char repr[100];
    BufferObject *this = (BufferObject *)(self);

    if (this->buf == INVALID_BUFFER_VALUE)
    {
	vim_snprintf(repr, 100, _("<buffer object (deleted) at %p>"), (self));
	return PyUnicode_FromString(repr);
    }
    else
    {
	char *name = (char *)this->buf->b_fname;
	Py_ssize_t len;

	if (name == NULL)
	    name = "";
	len = strlen(name);

	if (len > 35)
	    name = name + (35 - len);

	vim_snprintf(repr, 100, "<buffer %s%s>", len > 35 ? "..." : "", name);

	return PyUnicode_FromString(repr);
    }
}

/******************/

    static Py_ssize_t
BufferLength(PyObject *self)
{
    if (CheckBuffer((BufferObject *)(self)))
	return -1;

    return (Py_ssize_t)(((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

    static PyObject *
BufferItem(PyObject *self, Py_ssize_t n)
{
    return RBItem((BufferObject *)(self), n, 1,
	       (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

    static PyObject *
BufferSlice(PyObject *self, Py_ssize_t lo, Py_ssize_t hi)
{
    return RBSlice((BufferObject *)(self), lo, hi, 1,
	       (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

    static Py_ssize_t
BufferAsItem(PyObject *self, Py_ssize_t n, PyObject *val)
{
    return RBAsItem((BufferObject *)(self), n, val, 1,
		(Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		NULL);
}


    static PyObject *
BufferSubscript(PyObject *self, PyObject* idx)
{
    if (PyLong_Check(idx)) {
	long _idx = PyLong_AsLong(idx);
	return BufferItem(self,_idx);
    } else if (PySlice_Check(idx)) {
	Py_ssize_t start, stop, step, slicelen;

	if (PySlice_GetIndicesEx((PySliceObject *)idx,
	      (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count+1,
	      &start, &stop,
	      &step, &slicelen) < 0) {
	    return NULL;
	}
	return BufferSlice(self,start,stop+1);
    } else {
	PyErr_SetString(PyExc_IndexError, "Index must be int or slice");
	return NULL;
    }
}

static PySequenceMethods RangeAsSeq = {
    (lenfunc)		RangeLength,	 /* sq_length,	  len(x)   */
    (binaryfunc)	0,		 /* RangeConcat, sq_concat,  x+y   */
    (ssizeargfunc)	0,		 /* RangeRepeat, sq_repeat,  x*n   */
    (ssizeargfunc)	RangeItem,	 /* sq_item,	  x[i]	   */
    0,					 /* was_sq_slice,     x[i:j]   */
    (ssizeobjargproc)	RangeAsItem,	 /* sq_as_item,  x[i]=v   */
    0,					 /* sq_ass_slice, x[i:j]=v */
    0,					 /* sq_contains */
    0,					 /* sq_inplace_concat */
    0,					 /* sq_inplace_repeat */
};

PyMappingMethods RangeAsMapping = {
    /* mp_length	*/ (lenfunc)RangeLength,
    /* mp_subscript     */ (binaryfunc)RangeSubscript,
    /* mp_ass_subscript */ (objobjargproc)0,
};

/* Line range object - Implementation
 */

    static void
RangeDestructor(PyObject *self)
{
    Py_DECREF(((RangeObject *)(self))->buf);
}

    static PyObject *
RangeGetattro(PyObject *self, PyObject *nameobj)
{
    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    if (strcmp(name, "start") == 0)
	return Py_BuildValue("n", ((RangeObject *)(self))->start - 1);
    else if (strcmp(name, "end") == 0)
	return Py_BuildValue("n", ((RangeObject *)(self))->end - 1);
    else
	return PyObject_GenericGetAttr(self, nameobj);
}

/****************/

    static Py_ssize_t
RangeAsItem(PyObject *self, Py_ssize_t n, PyObject *val)
{
    return RBAsItem(((RangeObject *)(self))->buf, n, val,
		    ((RangeObject *)(self))->start,
		    ((RangeObject *)(self))->end,
		    &((RangeObject *)(self))->end);
}

    static PyObject *
RangeSubscript(PyObject *self, PyObject* idx)
{
    if (PyLong_Check(idx)) {
	long _idx = PyLong_AsLong(idx);
	return RangeItem(self,_idx);
    } else if (PySlice_Check(idx)) {
	Py_ssize_t start, stop, step, slicelen;

	if (PySlice_GetIndicesEx((PySliceObject *)idx,
		((RangeObject *)(self))->end-((RangeObject *)(self))->start+1,
		&start, &stop,
		&step, &slicelen) < 0) {
	    return NULL;
	}
	return RangeSlice(self,start,stop+1);
    } else {
	PyErr_SetString(PyExc_IndexError, "Index must be int or slice");
	return NULL;
    }
}

/* Buffer list object - Definitions
 */

typedef struct
{
    PyObject_HEAD
} BufListObject;

static PySequenceMethods BufListAsSeq = {
    (lenfunc)		BufListLength,	    /* sq_length,    len(x)   */
    (binaryfunc)	0,		    /* sq_concat,    x+y      */
    (ssizeargfunc)	0,		    /* sq_repeat,    x*n      */
    (ssizeargfunc)	BufListItem,	    /* sq_item,      x[i]     */
    0,					    /* was_sq_slice,	 x[i:j]   */
    (ssizeobjargproc)	0,		    /* sq_as_item,  x[i]=v   */
    0,					    /* sq_ass_slice, x[i:j]=v */
    0,					    /* sq_contains */
    0,					    /* sq_inplace_concat */
    0,					    /* sq_inplace_repeat */
};

static PyTypeObject BufListType;

/* Window object - Definitions
 */

static struct PyMethodDef WindowMethods[] = {
    /* name,	    function,		calling,    documentation */
    { NULL,	    NULL,		0,	    NULL }
};

static PyTypeObject WindowType;

/* Window object - Implementation
 */

    static PyObject *
WindowNew(win_T *win)
{
    /* We need to handle deletion of windows underneath us.
     * If we add a "w_python3_ref" field to the win_T structure,
     * then we can get at it in win_free() in vim. We then
     * need to create only ONE Python object per window - if
     * we try to create a second, just INCREF the existing one
     * and return it. The (single) Python object referring to
     * the window is stored in "w_python3_ref".
     * On a win_free() we set the Python object's win_T* field
     * to an invalid value. We trap all uses of a window
     * object, and reject them if the win_T* field is invalid.
     */

    WindowObject *self;

    if (win->w_python3_ref)
    {
	self = win->w_python3_ref;
	Py_INCREF(self);
    }
    else
    {
	self = PyObject_NEW(WindowObject, &WindowType);
	if (self == NULL)
	    return NULL;
	self->win = win;
	win->w_python3_ref = self;
    }

    return (PyObject *)(self);
}

    static void
WindowDestructor(PyObject *self)
{
    WindowObject *this = (WindowObject *)(self);

    if (this->win && this->win != INVALID_WINDOW_VALUE)
	this->win->w_python3_ref = NULL;
}

    static PyObject *
WindowGetattro(PyObject *self, PyObject *nameobj)
{
    WindowObject *this = (WindowObject *)(self);

    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);


    if (CheckWindow(this))
	return NULL;

    if (strcmp(name, "buffer") == 0)
	return (PyObject *)BufferNew(this->win->w_buffer);
    else if (strcmp(name, "cursor") == 0)
    {
	pos_T *pos = &this->win->w_cursor;

	return Py_BuildValue("(ll)", (long)(pos->lnum), (long)(pos->col));
    }
    else if (strcmp(name, "height") == 0)
	return Py_BuildValue("l", (long)(this->win->w_height));
#ifdef FEAT_VERTSPLIT
    else if (strcmp(name, "width") == 0)
	return Py_BuildValue("l", (long)(W_WIDTH(this->win)));
#endif
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[sss]", "buffer", "cursor", "height");
    else
	return PyObject_GenericGetAttr(self, nameobj);
}

    static int
WindowSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    char *name = "";

    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    return WindowSetattr(self, name, val);
}

/* Window list object - Definitions
 */

typedef struct
{
    PyObject_HEAD
}
WinListObject;

static PySequenceMethods WinListAsSeq = {
    (lenfunc)	     WinListLength,	    /* sq_length,    len(x)   */
    (binaryfunc)     0,			    /* sq_concat,    x+y      */
    (ssizeargfunc)   0,			    /* sq_repeat,    x*n      */
    (ssizeargfunc)   WinListItem,	    /* sq_item,      x[i]     */
    0,					    /* sq_slice,     x[i:j]   */
    (ssizeobjargproc)0,			    /* sq_as_item,  x[i]=v   */
    0,					    /* sq_ass_slice, x[i:j]=v */
    0,					    /* sq_contains */
    0,					    /* sq_inplace_concat */
    0,					    /* sq_inplace_repeat */
};

static PyTypeObject WinListType;

/* Current items object - Definitions
 */

typedef struct
{
    PyObject_HEAD
} CurrentObject;

static PyTypeObject CurrentType;

/* Current items object - Implementation
 */
    static PyObject *
CurrentGetattro(PyObject *self UNUSED, PyObject *nameobj)
{
    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    if (strcmp(name, "buffer") == 0)
	return (PyObject *)BufferNew(curbuf);
    else if (strcmp(name, "window") == 0)
	return (PyObject *)WindowNew(curwin);
    else if (strcmp(name, "line") == 0)
	return GetBufferLine(curbuf, (Py_ssize_t)curwin->w_cursor.lnum);
    else if (strcmp(name, "range") == 0)
	return RangeNew(curbuf, RangeStart, RangeEnd);
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[ssss]", "buffer", "window", "line", "range");
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
    }
}

    static int
CurrentSetattro(PyObject *self UNUSED, PyObject *nameobj, PyObject *value)
{
    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    if (strcmp(name, "line") == 0)
    {
	if (SetBufferLine(curbuf, (Py_ssize_t)curwin->w_cursor.lnum, value, NULL) == FAIL)
	    return -1;

	return 0;
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
    }
}

/* External interface
 */

    void
python3_buffer_free(buf_T *buf)
{
    if (buf->b_python3_ref != NULL)
    {
	BufferObject *bp = buf->b_python3_ref;
	bp->buf = INVALID_BUFFER_VALUE;
	buf->b_python3_ref = NULL;
    }
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
python3_window_free(win_T *win)
{
    if (win->w_python3_ref != NULL)
    {
	WindowObject *wp = win->w_python3_ref;
	wp->win = INVALID_WINDOW_VALUE;
	win->w_python3_ref = NULL;
    }
}
#endif

static BufListObject TheBufferList =
{
    PyObject_HEAD_INIT(&BufListType)
};

static WinListObject TheWindowList =
{
    PyObject_HEAD_INIT(&WinListType)
};

static CurrentObject TheCurrent =
{
    PyObject_HEAD_INIT(&CurrentType)
};

PyDoc_STRVAR(vim_module_doc,"vim python interface\n");

static struct PyModuleDef vimmodule;

#ifndef PROTO
PyMODINIT_FUNC Py3Init_vim(void)
{
    PyObject *mod;
    /* The special value is removed from sys.path in Python3_Init(). */
    static wchar_t *(argv[2]) = {L"/must>not&exist/foo", NULL};

    PyType_Ready(&BufferType);
    PyType_Ready(&RangeType);
    PyType_Ready(&WindowType);
    PyType_Ready(&BufListType);
    PyType_Ready(&WinListType);
    PyType_Ready(&CurrentType);

    /* Set sys.argv[] to avoid a crash in warn(). */
    PySys_SetArgv(1, argv);

    mod = PyModule_Create(&vimmodule);

    VimError = Py_BuildValue("s", "vim.error");

    PyModule_AddObject(mod, "error", VimError);
    Py_INCREF((PyObject *)(void *)&TheBufferList);
    PyModule_AddObject(mod, "buffers", (PyObject *)(void *)&TheBufferList);
    Py_INCREF((PyObject *)(void *)&TheCurrent);
    PyModule_AddObject(mod, "current", (PyObject *)(void *)&TheCurrent);
    Py_INCREF((PyObject *)(void *)&TheWindowList);
    PyModule_AddObject(mod, "windows", (PyObject *)(void *)&TheWindowList);

    if (PyErr_Occurred())
	return NULL;

    return mod;
}
#endif

/*************************************************************************
 * 4. Utility functions for handling the interface between Vim and Python.
 */

/* Convert a Vim line into a Python string.
 * All internal newlines are replaced by null characters.
 *
 * On errors, the Python exception data is set, and NULL is returned.
 */
    static PyObject *
LineToString(const char *str)
{
    PyObject *result;
    Py_ssize_t len = strlen(str);
    char *tmp,*p;

    tmp = (char *)alloc((unsigned)(len+1));
    p = tmp;
    if (p == NULL)
    {
	PyErr_NoMemory();
	return NULL;
    }

    while (*str)
    {
	if (*str == '\n')
	    *p = '\0';
	else
	    *p = *str;

	++p;
	++str;
    }
    *p = '\0';

    result = PyUnicode_FromStringAndSize(tmp, len);

    vim_free(tmp);
    return result;
}

    static void
init_structs(void)
{
    vim_memset(&OutputType, 0, sizeof(OutputType));
    OutputType.tp_name = "vim.message";
    OutputType.tp_basicsize = sizeof(OutputObject);
    OutputType.tp_getattro = OutputGetattro;
    OutputType.tp_setattro = OutputSetattro;
    OutputType.tp_flags = Py_TPFLAGS_DEFAULT;
    OutputType.tp_doc = "vim message object";
    OutputType.tp_methods = OutputMethods;
    OutputType.tp_alloc = call_PyType_GenericAlloc;
    OutputType.tp_new = call_PyType_GenericNew;
    OutputType.tp_free = call_PyObject_Free;

    vim_memset(&BufferType, 0, sizeof(BufferType));
    BufferType.tp_name = "vim.buffer";
    BufferType.tp_basicsize = sizeof(BufferType);
    BufferType.tp_dealloc = BufferDestructor;
    BufferType.tp_repr = BufferRepr;
    BufferType.tp_as_sequence = &BufferAsSeq;
    BufferType.tp_as_mapping = &BufferAsMapping;
    BufferType.tp_getattro = BufferGetattro;
    BufferType.tp_flags = Py_TPFLAGS_DEFAULT;
    BufferType.tp_doc = "vim buffer object";
    BufferType.tp_methods = BufferMethods;
    BufferType.tp_alloc = call_PyType_GenericAlloc;
    BufferType.tp_new = call_PyType_GenericNew;
    BufferType.tp_free = call_PyObject_Free;

    vim_memset(&WindowType, 0, sizeof(WindowType));
    WindowType.tp_name = "vim.window";
    WindowType.tp_basicsize = sizeof(WindowObject);
    WindowType.tp_dealloc = WindowDestructor;
    WindowType.tp_repr = WindowRepr;
    WindowType.tp_getattro = WindowGetattro;
    WindowType.tp_setattro = WindowSetattro;
    WindowType.tp_flags = Py_TPFLAGS_DEFAULT;
    WindowType.tp_doc = "vim Window object";
    WindowType.tp_methods = WindowMethods;
    WindowType.tp_alloc = call_PyType_GenericAlloc;
    WindowType.tp_new = call_PyType_GenericNew;
    WindowType.tp_free = call_PyObject_Free;

    vim_memset(&BufListType, 0, sizeof(BufListType));
    BufListType.tp_name = "vim.bufferlist";
    BufListType.tp_basicsize = sizeof(BufListObject);
    BufListType.tp_as_sequence = &BufListAsSeq;
    BufListType.tp_flags = Py_TPFLAGS_DEFAULT;
    BufferType.tp_doc = "vim buffer list";

    vim_memset(&WinListType, 0, sizeof(WinListType));
    WinListType.tp_name = "vim.windowlist";
    WinListType.tp_basicsize = sizeof(WinListType);
    WinListType.tp_as_sequence = &WinListAsSeq;
    WinListType.tp_flags = Py_TPFLAGS_DEFAULT;
    WinListType.tp_doc = "vim window list";

    vim_memset(&RangeType, 0, sizeof(RangeType));
    RangeType.tp_name = "vim.range";
    RangeType.tp_basicsize = sizeof(RangeObject);
    RangeType.tp_dealloc = RangeDestructor;
    RangeType.tp_repr = RangeRepr;
    RangeType.tp_as_sequence = &RangeAsSeq;
    RangeType.tp_as_mapping = &RangeAsMapping;
    RangeType.tp_getattro = RangeGetattro;
    RangeType.tp_flags = Py_TPFLAGS_DEFAULT;
    RangeType.tp_doc = "vim Range object";
    RangeType.tp_methods = RangeMethods;
    RangeType.tp_alloc = call_PyType_GenericAlloc;
    RangeType.tp_new = call_PyType_GenericNew;
    RangeType.tp_free = call_PyObject_Free;

    vim_memset(&CurrentType, 0, sizeof(CurrentType));
    CurrentType.tp_name = "vim.currentdata";
    CurrentType.tp_basicsize = sizeof(CurrentObject);
    CurrentType.tp_getattro = CurrentGetattro;
    CurrentType.tp_setattro = CurrentSetattro;
    CurrentType.tp_flags = Py_TPFLAGS_DEFAULT;
    CurrentType.tp_doc = "vim current object";

    vim_memset(&vimmodule, 0, sizeof(vimmodule));
    vimmodule.m_name = "vim";
    vimmodule.m_doc = vim_module_doc;
    vimmodule.m_size = -1;
    vimmodule.m_methods = VimMethods;
}
