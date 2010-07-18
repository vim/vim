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

// uncomment this if used with the debug version of python
// #define Py_DEBUG

#include "vim.h"

#include <limits.h>

/* Python.h defines _POSIX_THREADS itself (if needed) */
#ifdef _POSIX_THREADS
# undef _POSIX_THREADS
#endif

#if defined(_WIN32) && defined (HAVE_FCNTL_H)
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

#if defined(DYNAMIC_PYTHON3)

#ifndef _WIN32
#include <dlfcn.h>
#define FARPROC void*
#define HINSTANCE void*
#define load_dll(n) dlopen((n),RTLD_LAZY)
#define close_dll dlclose
#define symbol_from_dll dlsym
#else
#define load_dll LoadLibrary
#define close_dll FreeLibrary
#define symbol_from_dll GetProcAddress
#endif
/*
 * Wrapper defines
 */
#undef PyArg_Parse
# define PyArg_Parse py3_PyArg_Parse
#undef PyArg_ParseTuple
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
# define Py_Initialize py3_Py_Initialize
# define Py_Finalize py3_Py_Finalize
# define Py_IsInitialized py3_Py_IsInitialized
# define _Py_NoneStruct (*py3__Py_NoneStruct)
# define PyModule_AddObject py3_PyModule_AddObject
# define PyImport_AppendInittab py3_PyImport_AppendInittab
# define _PyUnicode_AsString py3__PyUnicode_AsString
# define PyObject_GenericGetAttr py3_PyObject_GenericGetAttr
# define PySlice_Type (*py3_PySlice_Type)
#ifdef Py_DEBUG
    # define _Py_NegativeRefcount py3__Py_NegativeRefcount
    # define _Py_RefTotal (*py3__Py_RefTotal)
    # define _Py_Dealloc py3__Py_Dealloc
    # define _PyObject_DebugMalloc py3__PyObject_DebugMalloc
    # define _PyObject_DebugFree py3__PyObject_DebugFree
#else
    # define PyObject_Malloc py3_PyObject_Malloc
    # define PyObject_Free py3_PyObject_Free
#endif
# define PyType_GenericAlloc py3_PyType_GenericAlloc
# define PyType_GenericNew py3_PyType_GenericNew
# define PyModule_Create2 py3_PyModule_Create2
#undef PyUnicode_FromString
# define PyUnicode_FromString py3_PyUnicode_FromString
#undef PyUnicode_FromStringAndSize
# define PyUnicode_FromStringAndSize py3_PyUnicode_FromStringAndSize

#ifdef Py_DEBUG
#undef PyObject_NEW
#define PyObject_NEW(type, typeobj) \
( (type *) PyObject_Init( \
	(PyObject *) _PyObject_DebugMalloc( _PyObject_SIZE(typeobj) ), (typeobj)) )
#endif
/*
 * Pointers for dynamic link
 */
static int (*py3_PySys_SetArgv)(int, wchar_t **);
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
#ifdef Py_DEBUG
    static void (*py3__Py_NegativeRefcount)(const char *fname, int lineno, PyObject *op);
    static Py_ssize_t* py3__Py_RefTotal;
    static void (*py3__Py_Dealloc)(PyObject *obj);
    static void (*py3__PyObject_DebugFree)(void*);
    static void* (*py3__PyObject_DebugMalloc)(size_t);
#else
    static void (*py3_PyObject_Free)(void*);
    static void* (*py3_PyObject_Malloc)(size_t);
#endif

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
#ifdef Py_DEBUG
    {"_Py_NegativeRefcount", (PYTHON_PROC*)&py3__Py_NegativeRefcount},
    {"_Py_RefTotal", (PYTHON_PROC*)&py3__Py_RefTotal},
    {"_Py_Dealloc", (PYTHON_PROC*)&py3__Py_Dealloc},
    {"_PyObject_DebugFree", (PYTHON_PROC*)&py3__PyObject_DebugFree},
    {"_PyObject_DebugMalloc", (PYTHON_PROC*)&py3__PyObject_DebugMalloc},
#else
    {"PyObject_Malloc", (PYTHON_PROC*)&py3_PyObject_Malloc},
    {"PyObject_Free", (PYTHON_PROC*)&py3_PyObject_Free},
#endif
    {"", NULL},
};

/*
 * Free python.dll
 */
static void end_dynamic_python3(void)
{
    if (hinstPy3)
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
static int py3_runtime_link_init(char *libname, int verbose)
{
    int i;

    if (hinstPy3)
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

    /* load unicode functions separately as only the ucs2 or the ucs4 functions
     * will be present in the library
     */
    void *ucs_from_string, *ucs_from_string_and_size;

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
int python3_enabled(int verbose)
{
    return py3_runtime_link_init(DYNAMIC_PYTHON3_DLL, verbose) == OK;
}

/* Load the standard Python exceptions - don't import the symbols from the
 * DLL, as this can cause errors (importing data symbols is not reliable).
 */
static void get_py3_exceptions __ARGS((void));

static void get_py3_exceptions()
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

static void call_PyObject_Free(void *p)
{
#ifdef Py_DEBUG
    _PyObject_DebugFree(p);
#else
    PyObject_Free(p);
#endif
}
static PyObject* call_PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return PyType_GenericNew(type,args,kwds);
}
static PyObject* call_PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)
{
    return PyType_GenericAlloc(type,nitems);
}

/******************************************************
 * Internal function prototypes.
 */

static void DoPy3Command(exarg_T *, const char *);
static Py_ssize_t RangeStart;
static Py_ssize_t RangeEnd;

static void PythonIO_Flush(void);
static int PythonIO_Init(void);
static void PythonIO_Fini(void);
static PyMODINIT_FUNC Py3Init_vim(void);

/* Utility functions for the vim/python interface
 * ----------------------------------------------
 */
static PyObject *GetBufferLine(buf_T *, Py_ssize_t);

static int SetBufferLine(buf_T *, Py_ssize_t, PyObject *, Py_ssize_t*);
static int InsertBufferLines(buf_T *, Py_ssize_t, PyObject *, Py_ssize_t*);
static PyObject *GetBufferLineList(buf_T *buf, Py_ssize_t lo, Py_ssize_t hi);

static PyObject *LineToString(const char *);
static char *StringToLine(PyObject *);

static int VimErrorCheck(void);

#define PyErr_SetVim(str) PyErr_SetString(VimError, str)

/******************************************************
 * 1. Python interpreter main program.
 */

static int py3initialised = 0;


static PyGILState_STATE pygilstate = PyGILState_UNLOCKED;

/*
 * obtain a lock on the Vim data structures
 */
static void Python_Lock_Vim(void)
{
}

/*
 * release a lock on the Vim data structures
 */
static void Python_Release_Vim(void)
{
}

void python3_end()
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

static int Python3_Init(void)
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

	/* initialise threads */
	PyEval_InitThreads();

#if !defined(MACOS) || defined(MACOS_X_UNIX)
	Py_Initialize();
#else
	PyMac_Initialize();
#endif

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
static void DoPy3Command(exarg_T *eap, const char *cmd)
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
 * ":python3"
 */
void ex_python3(exarg_T *eap)
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

static PyObject *OutputGetattro(PyObject *, PyObject *);
static int OutputSetattro(PyObject *, PyObject *, PyObject *);

static PyObject *OutputWrite(PyObject *, PyObject *);
static PyObject *OutputWritelines(PyObject *, PyObject *);

typedef void (*writefn)(char_u *);
static void writer(writefn fn, char_u *str, Py_ssize_t n);

/* Output object definition
 */

typedef struct
{
    PyObject_HEAD
    long softspace;
    long error;
} OutputObject;

static struct PyMethodDef OutputMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"write",	    OutputWrite,	1,	    "" },
    {"writelines",  OutputWritelines,	1,	    "" },
    { NULL,	    NULL,		0,	    NULL }
};

static PyTypeObject OutputType;

/*************/

static PyObject * OutputGetattro(PyObject *self, PyObject *nameobj)
{
    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);

    if (strcmp(name, "softspace") == 0)
	return PyLong_FromLong(((OutputObject *)(self))->softspace);

    return PyObject_GenericGetAttr(self, nameobj);
}

static int OutputSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
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

/*************/

static PyObject * OutputWrite(PyObject *self, PyObject *args)
{
    int len;
    char *str;
    int error = ((OutputObject *)(self))->error;

    if (!PyArg_ParseTuple(args, "s#", &str, &len))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    writer((writefn)(error ? emsg : msg), (char_u *)str, len);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * OutputWritelines(PyObject *self, PyObject *args)
{
    Py_ssize_t n;
    Py_ssize_t i;
    PyObject *list;
    int error = ((OutputObject *)(self))->error;

    if (!PyArg_ParseTuple(args, "O", &list))
	return NULL;
    Py_INCREF(list);

    if (!PyList_Check(list)) {
	PyErr_SetString(PyExc_TypeError, _("writelines() requires list of strings"));
	Py_DECREF(list);
	return NULL;
    }

    n = PyList_Size(list);

    for (i = 0; i < n; ++i)
    {
	PyObject *line = PyList_GetItem(list, i);
	char *str;
	Py_ssize_t len;

	if (!PyArg_Parse(line, "s#", &str, &len)) {
	    PyErr_SetString(PyExc_TypeError, _("writelines() requires list of strings"));
	    Py_DECREF(list);
	    return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	    Python_Lock_Vim();
	writer((writefn)(error ? emsg : msg), (char_u *)str, len);
	Python_Release_Vim();
	Py_END_ALLOW_THREADS
    }

    Py_DECREF(list);
    Py_INCREF(Py_None);
    return Py_None;
}

/* Output buffer management
 */

static char_u *buffer = NULL;
static Py_ssize_t buffer_len = 0;
static Py_ssize_t buffer_size = 0;

static writefn old_fn = NULL;

static void buffer_ensure(Py_ssize_t n)
{
    Py_ssize_t new_size;
    char_u *new_buffer;

    if (n < buffer_size)
	return;

    new_size = buffer_size;
    while (new_size < n)
	new_size += 80;

    if (new_size != buffer_size)
    {
	new_buffer = alloc((unsigned)new_size);
	if (new_buffer == NULL)
	    return;

	if (buffer)
	{
	    memcpy(new_buffer, buffer, buffer_len);
	    vim_free(buffer);
	}

	buffer = new_buffer;
	buffer_size = new_size;
    }
}

static void PythonIO_Flush(void)
{
    if (old_fn && buffer_len)
    {
	buffer[buffer_len] = 0;
	old_fn(buffer);
    }

    buffer_len = 0;
}

static void writer(writefn fn, char_u *str, Py_ssize_t n)
{
    char_u *ptr;

    if (fn != old_fn && old_fn != NULL)
	PythonIO_Flush();

    old_fn = fn;

    while (n > 0 && (ptr = memchr(str, '\n', n)) != NULL)
    {
	Py_ssize_t len = ptr - str;

	buffer_ensure(buffer_len + len + 1);

	memcpy(buffer + buffer_len, str, len);
	buffer_len += len;
	buffer[buffer_len] = 0;
	fn(buffer);
	str = ptr + 1;
	n -= len + 1;
	buffer_len = 0;
    }

    /* Put the remaining text into the buffer for later printing */
    buffer_ensure(buffer_len + n + 1);
    memcpy(buffer + buffer_len, str, n);
    buffer_len += n;
}

/***************/

static OutputObject Output =
{
    PyObject_HEAD_INIT(&OutputType)
    0,
    0
};

static OutputObject Error =
{
    PyObject_HEAD_INIT(&OutputType)
    0,
    1
};

static int PythonIO_Init(void)
{
    PyType_Ready(&OutputType);

    PySys_SetObject("stdout", (PyObject *)(void *)&Output);
    PySys_SetObject("stderr", (PyObject *)(void *)&Error);

    if (PyErr_Occurred())
    {
	EMSG(_("E264: Python: Error initialising I/O objects"));
	return -1;
    }

    return 0;
}
static void PythonIO_Fini(void)
{
    PySys_SetObject("stdout", NULL);
    PySys_SetObject("stderr", NULL);
}

/******************************************************
 * 3. Implementation of the Vim module for Python
 */

/* Vim module - Implementation functions
 * -------------------------------------
 */

static PyObject *VimError;

static PyObject *VimCommand(PyObject *, PyObject *);
static PyObject *VimEval(PyObject *, PyObject *);

/* Window type - Implementation functions
 * --------------------------------------
 */

typedef struct
{
    PyObject_HEAD
    win_T       *win;
}
WindowObject;

#define INVALID_WINDOW_VALUE ((win_T *)(-1))

#define WindowType_Check(obj) ((obj)->ob_base.ob_type == &WindowType)

static PyObject *WindowNew(win_T *);

static void WindowDestructor(PyObject *);
static PyObject *WindowGetattro(PyObject *, PyObject *);
static int WindowSetattro(PyObject *, PyObject *, PyObject *);
static PyObject *WindowRepr(PyObject *);

/* Buffer type - Implementation functions
 * --------------------------------------
 */

typedef struct
{
    PyObject_HEAD
    buf_T *buf;
}
BufferObject;

#define INVALID_BUFFER_VALUE ((buf_T *)(-1))

#define BufferType_Check(obj) ((obj)->ob_base.ob_type == &BufferType)

static PyObject *BufferNew (buf_T *);

static void BufferDestructor(PyObject *);

static PyObject *BufferGetattro(PyObject *, PyObject*);
static PyObject *BufferRepr(PyObject *);

static Py_ssize_t BufferLength(PyObject *);
static PyObject *BufferItem(PyObject *, Py_ssize_t);
static Py_ssize_t BufferAsItem(PyObject *, Py_ssize_t, PyObject *);
static PyObject* BufferSubscript(PyObject *self, PyObject* idx);

static PyObject *BufferAppend(PyObject *, PyObject *);
static PyObject *BufferMark(PyObject *, PyObject *);
static PyObject *BufferRange(PyObject *, PyObject *);

/* Line range type - Implementation functions
 * --------------------------------------
 */

typedef struct
{
    PyObject_HEAD
    BufferObject *buf;
    Py_ssize_t start;
    Py_ssize_t end;
}
RangeObject;

#define RangeType_Check(obj) ((obj)->ob_base.ob_type == &RangeType)

static PyObject *RangeNew(buf_T *, Py_ssize_t, Py_ssize_t);

static void RangeDestructor(PyObject *);
static PyObject *RangeGetattro(PyObject *, PyObject *);
static PyObject *RangeRepr(PyObject *);
static PyObject* RangeSubscript(PyObject *self, PyObject* idx);

static Py_ssize_t RangeLength(PyObject *);
static PyObject *RangeItem(PyObject *, Py_ssize_t);
static Py_ssize_t RangeAsItem(PyObject *, Py_ssize_t, PyObject *);

static PyObject *RangeAppend(PyObject *, PyObject *);

/* Window list type - Implementation functions
 * -------------------------------------------
 */

static Py_ssize_t WinListLength(PyObject *);
static PyObject *WinListItem(PyObject *, Py_ssize_t);

/* Buffer list type - Implementation functions
 * -------------------------------------------
 */

static Py_ssize_t BufListLength(PyObject *);
static PyObject *BufListItem(PyObject *, Py_ssize_t);

/* Current objects type - Implementation functions
 * -----------------------------------------------
 */

static PyObject *CurrentGetattro(PyObject *, PyObject *);
static int CurrentSetattro(PyObject *, PyObject *, PyObject *);

/* Vim module - Definitions
 */

static struct PyMethodDef VimMethods[] = {
    /* name,	     function,		calling,    documentation */
    {"command",      VimCommand,	1,	    "Execute a Vim ex-mode command" },
    {"eval",	     VimEval,		1,	    "Evaluate an expression using Vim evaluator" },
    { NULL,	     NULL,		0,	    NULL }
};

/* Vim module - Implementation
 */
/*ARGSUSED*/
static PyObject * VimCommand(PyObject *self UNUSED, PyObject *args)
{
    char *cmd;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s", &cmd))
	return NULL;

    PyErr_Clear();

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();

    do_cmdline_cmd((char_u *)cmd);
    update_screen(VALID);

    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (VimErrorCheck())
	result = NULL;
    else
	result = Py_None;

    Py_XINCREF(result);
    return result;
}

#ifdef FEAT_EVAL
/*
 * Function to translate a typval_T into a PyObject; this will recursively
 * translate lists/dictionaries into their Python equivalents.
 *
 * The depth parameter is to avoid infinite recursion, set it to 1 when
 * you call VimToPython.
 */
static PyObject * VimToPython(typval_T *our_tv, int depth, PyObject *lookupDict)
{
    PyObject    *result;
    PyObject    *newObj;
    char	ptrBuf[NUMBUFLEN];

    /* Avoid infinite recursion */
    if (depth > 100)
    {
	Py_INCREF(Py_None);
	result = Py_None;
	return result;
    }

    /* Check if we run into a recursive loop.  The item must be in lookupDict
     * then and we can use it again. */
    if ((our_tv->v_type == VAR_LIST && our_tv->vval.v_list != NULL)
	    || (our_tv->v_type == VAR_DICT && our_tv->vval.v_dict != NULL))
    {
	sprintf(ptrBuf, PRINTF_DECIMAL_LONG_U,
		our_tv->v_type == VAR_LIST ? (long_u)our_tv->vval.v_list
					   : (long_u)our_tv->vval.v_dict);
	result = PyDict_GetItemString(lookupDict, ptrBuf);
	if (result != NULL)
	{
	    Py_INCREF(result);
	    return result;
	}
    }

    if (our_tv->v_type == VAR_STRING)
    {
	result = Py_BuildValue("s", our_tv->vval.v_string);
    }
    else if (our_tv->v_type == VAR_NUMBER)
    {
	char buf[NUMBUFLEN];

	/* For backwards compatibility numbers are stored as strings. */
	sprintf(buf, "%ld", (long)our_tv->vval.v_number);
	result = Py_BuildValue("s", buf);
    }
# ifdef FEAT_FLOAT
    else if (our_tv->v_type == VAR_FLOAT)
    {
	char buf[NUMBUFLEN];

	sprintf(buf, "%f", our_tv->vval.v_float);
	result = Py_BuildValue("s", buf);
    }
# endif
    else if (our_tv->v_type == VAR_LIST)
    {
	list_T		*list = our_tv->vval.v_list;
	listitem_T	*curr;

	result = PyList_New(0);

	if (list != NULL)
	{
	    PyDict_SetItemString(lookupDict, ptrBuf, result);

	    for (curr = list->lv_first; curr != NULL; curr = curr->li_next)
	    {
		newObj = VimToPython(&curr->li_tv, depth + 1, lookupDict);
		PyList_Append(result, newObj);
		Py_DECREF(newObj);
	    }
	}
    }
    else if (our_tv->v_type == VAR_DICT)
    {
	result = PyDict_New();

	if (our_tv->vval.v_dict != NULL)
	{
	    hashtab_T   *ht = &our_tv->vval.v_dict->dv_hashtab;
	    long_u      t = ht->ht_used;
	    hashitem_T  *hi;
	    dictitem_T  *di;

	    PyDict_SetItemString(lookupDict, ptrBuf, result);

	    for (hi = ht->ht_array; t > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --t;

		    di = dict_lookup(hi);
		    newObj = VimToPython(&di->di_tv, depth + 1, lookupDict);
		    PyDict_SetItemString(result, (char *)hi->hi_key, newObj);
		    Py_DECREF(newObj);
		}
	    }
	}
    }
    else
    {
	Py_INCREF(Py_None);
	result = Py_None;
    }

    return result;
}
#endif

/*ARGSUSED*/
static PyObject * VimEval(PyObject *self UNUSED, PyObject *args)
{
#ifdef FEAT_EVAL
    char	*expr;
    typval_T    *our_tv;
    PyObject    *result;
    PyObject    *lookup_dict;

    if (!PyArg_ParseTuple(args, "s", &expr))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    our_tv = eval_expr((char_u *)expr, NULL);

    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (our_tv == NULL)
    {
	PyErr_SetVim(_("invalid expression"));
	return NULL;
    }

    /* Convert the Vim type into a Python type.  Create a dictionary that's
     * used to check for recursive loops. */
    lookup_dict = PyDict_New();
    result = VimToPython(our_tv, 1, lookup_dict);
    Py_DECREF(lookup_dict);


    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    free_tv(our_tv);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    return result;
#else
    PyErr_SetVim(_("expressions disabled at compile time"));
    return NULL;
#endif
}

/* Common routines for buffers and line ranges
 * -------------------------------------------
 */

static int CheckBuffer(BufferObject *this)
{
    if (this->buf == INVALID_BUFFER_VALUE)
    {
	PyErr_SetVim(_("attempt to refer to deleted buffer"));
	return -1;
    }

    return 0;
}

static PyObject * RBItem(BufferObject *self, Py_ssize_t n, Py_ssize_t start, Py_ssize_t end)
{
    if (CheckBuffer(self))
	return NULL;

    if (n < 0 || n > end - start)
    {
	PyErr_SetString(PyExc_IndexError, _("line number out of range"));
	return NULL;
    }

    return GetBufferLine(self->buf, n+start);
}

static Py_ssize_t RBAsItem(BufferObject *self, Py_ssize_t n, PyObject *val, Py_ssize_t start, Py_ssize_t end, Py_ssize_t *new_end)
{
    Py_ssize_t len_change;

    if (CheckBuffer(self))
	return -1;

    if (n < 0 || n > end - start)
    {
	PyErr_SetString(PyExc_IndexError, _("line number out of range"));
	return -1;
    }

    if (SetBufferLine(self->buf, n+start, val, &len_change) == FAIL)
	return -1;

    if (new_end)
	*new_end = end + len_change;

    return 0;
}

static PyObject * RBSlice(BufferObject *self, Py_ssize_t lo, Py_ssize_t hi, Py_ssize_t start, Py_ssize_t end)
{
    Py_ssize_t size;

    if (CheckBuffer(self))
	return NULL;

    size = end - start + 1;

    if (lo < 0)
	lo = 0;
    else if (lo > size)
	lo = size;
    if (hi < 0)
	hi = 0;
    if (hi < lo)
	hi = lo;
    else if (hi > size)
	hi = size;

    return GetBufferLineList(self->buf, lo+start, hi+start);
}

static PyObject * RBAppend(BufferObject *self, PyObject *args, Py_ssize_t start, Py_ssize_t end, Py_ssize_t *new_end)
{
    PyObject *lines;
    Py_ssize_t len_change;
    Py_ssize_t max;
    Py_ssize_t n;

    if (CheckBuffer(self))
	return NULL;

    max = n = end - start + 1;

    if (!PyArg_ParseTuple(args, "O|n" , &lines, &n))
	return NULL;

    if (n < 0 || n > max)
    {
	PyErr_SetString(PyExc_ValueError, _("line number out of range"));
	return NULL;
    }

    if (InsertBufferLines(self->buf, n + start - 1, lines, &len_change) == FAIL)
	return NULL;

    if (new_end)
	*new_end = end + len_change;

    Py_INCREF(Py_None);
    return Py_None;
}


static struct PyMethodDef BufferMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    BufferAppend,	1,	    "Append data to Vim buffer" },
    {"mark",	    BufferMark,		1,	    "Return (row,col) representing position of named mark" },
    {"range",	    BufferRange,	1,	    "Return a range object which represents the part of the given buffer between line numbers s and e" },
    { NULL,	    NULL,		0,	    NULL }
};

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

static PyObject * BufferNew(buf_T *buf)
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

static void BufferDestructor(PyObject *self)
{
    BufferObject *this = (BufferObject *)(self);

    if (this->buf && this->buf != INVALID_BUFFER_VALUE)
	this->buf->b_python3_ref = NULL;
}

static PyObject * BufferGetattro(PyObject *self, PyObject*nameobj)
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

static PyObject * BufferRepr(PyObject *self)
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

static Py_ssize_t BufferLength(PyObject *self)
{
    if (CheckBuffer((BufferObject *)(self)))
	return -1;

    return (Py_ssize_t)(((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

static PyObject * BufferItem(PyObject *self, Py_ssize_t n)
{
    return RBItem((BufferObject *)(self), n, 1,
	       (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

static Py_ssize_t BufferAsItem(PyObject *self, Py_ssize_t n, PyObject *val)
{
    return RBAsItem((BufferObject *)(self), n, val, 1,
		(Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		NULL);
}

static PyObject * BufferSlice(PyObject *self, Py_ssize_t lo, Py_ssize_t hi)
{
    return RBSlice((BufferObject *)(self), lo, hi, 1,
	       (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count);
}


static PyObject* BufferSubscript(PyObject *self, PyObject* idx)
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

static PyObject * BufferAppend(PyObject *self, PyObject *args)
{
    return RBAppend((BufferObject *)(self), args, 1,
		(Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		NULL);
}

static PyObject * BufferMark(PyObject *self, PyObject *args)
{
    pos_T       *posp;
    char	*pmark;//test
    char	mark;
    buf_T       *curbuf_save;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    if (!PyArg_ParseTuple(args, "s", &pmark))//test: "c"->"s"
	return NULL;
    mark = *pmark;//test

    curbuf_save = curbuf;
    curbuf = ((BufferObject *)(self))->buf;
    posp = getmark(mark, FALSE);
    curbuf = curbuf_save;

    if (posp == NULL)
    {
	PyErr_SetVim(_("invalid mark name"));
	return NULL;
    }

    /* Ckeck for keyboard interrupt */
    if (VimErrorCheck())
	return NULL;

    if (posp->lnum <= 0)
    {
	/* Or raise an error? */
	Py_INCREF(Py_None);
	return Py_None;
    }

    return Py_BuildValue("(ll)", (long)(posp->lnum), (long)(posp->col));
}

static PyObject * BufferRange(PyObject *self, PyObject *args)
{
    Py_ssize_t start;
    Py_ssize_t end;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    if (!PyArg_ParseTuple(args, "nn", &start, &end))
	return NULL;

    return RangeNew(((BufferObject *)(self))->buf, start, end);
}

/* Line range object - Definitions
 */

static struct PyMethodDef RangeMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    RangeAppend,	1,	    "Append data to the Vim range" },
    { NULL,	    NULL,		0,	    NULL }
};

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

static PyTypeObject RangeType;

/* Line range object - Implementation
 */

static PyObject * RangeNew(buf_T *buf, Py_ssize_t start, Py_ssize_t end)
{
    BufferObject *bufr;
    RangeObject *self;
    self = PyObject_NEW(RangeObject, &RangeType);
    if (self == NULL)
	return NULL;

    bufr = (BufferObject *)BufferNew(buf);
    if (bufr == NULL)
    {
	Py_DECREF(self);
	return NULL;
    }
    Py_INCREF(bufr);

    self->buf = bufr;
    self->start = start;
    self->end = end;

    return (PyObject *)(self);
}

static void RangeDestructor(PyObject *self)
{
    Py_DECREF(((RangeObject *)(self))->buf);
}

static PyObject * RangeGetattro(PyObject *self, PyObject *nameobj)
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

static PyObject * RangeRepr(PyObject *self)
{
    static char repr[100];
    RangeObject *this = (RangeObject *)(self);

    if (this->buf->buf == INVALID_BUFFER_VALUE)
    {
	vim_snprintf(repr, 100, "<range object (for deleted buffer) at %p>",
								      (self));
	return PyUnicode_FromString(repr);
    }
    else
    {
	char *name = (char *)this->buf->buf->b_fname;
	int len;

	if (name == NULL)
	    name = "";
	len = (int)strlen(name);

	if (len > 45)
	    name = name + (45 - len);

	vim_snprintf(repr, 100, "<range %s%s (%d:%d)>",
		len > 45 ? "..." : "", name,
		this->start, this->end);

	return PyUnicode_FromString(repr);
    }
}

/****************/

static Py_ssize_t RangeLength(PyObject *self)
{
    /* HOW DO WE SIGNAL AN ERROR FROM THIS FUNCTION? */
    if (CheckBuffer(((RangeObject *)(self))->buf))
	return -1; /* ??? */

    return (((RangeObject *)(self))->end - ((RangeObject *)(self))->start + 1);
}

static PyObject * RangeItem(PyObject *self, Py_ssize_t n)
{
    return RBItem(((RangeObject *)(self))->buf, n,
		  ((RangeObject *)(self))->start,
		  ((RangeObject *)(self))->end);
}

static Py_ssize_t RangeAsItem(PyObject *self, Py_ssize_t n, PyObject *val)
{
    return RBAsItem(((RangeObject *)(self))->buf, n, val,
		    ((RangeObject *)(self))->start,
		    ((RangeObject *)(self))->end,
		    &((RangeObject *)(self))->end);
}

static PyObject * RangeSlice(PyObject *self, Py_ssize_t lo, Py_ssize_t hi)
{
    return RBSlice(((RangeObject *)(self))->buf, lo, hi,
		   ((RangeObject *)(self))->start,
		   ((RangeObject *)(self))->end);
}

static PyObject* RangeSubscript(PyObject *self, PyObject* idx)
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

static PyObject * RangeAppend(PyObject *self, PyObject *args)
{
    return RBAppend(((RangeObject *)(self))->buf, args,
		    ((RangeObject *)(self))->start,
		    ((RangeObject *)(self))->end,
		    &((RangeObject *)(self))->end);
}

/* Buffer list object - Definitions
 */

typedef struct
{
    PyObject_HEAD
}
BufListObject;

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

/* Buffer list object - Implementation
 */

/*ARGSUSED*/
static Py_ssize_t BufListLength(PyObject *self UNUSED)
{
    buf_T       *b = firstbuf;
    Py_ssize_t  n = 0;

    while (b)
    {
	++n;
	b = b->b_next;
    }

    return n;
}

/*ARGSUSED*/
static PyObject * BufListItem(PyObject *self UNUSED, Py_ssize_t n)
{
    buf_T *b;

    for (b = firstbuf; b; b = b->b_next, --n)
    {
	if (n == 0)
	    return BufferNew(b);
    }

    PyErr_SetString(PyExc_IndexError, _("no such buffer"));
    return NULL;
}

/* Window object - Definitions
 */

static struct PyMethodDef WindowMethods[] = {
    /* name,	    function,		calling,    documentation */
    { NULL,	    NULL,		0,	    NULL }
};

static PyTypeObject WindowType;

/* Window object - Implementation
 */

static PyObject * WindowNew(win_T *win)
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

static void WindowDestructor(PyObject *self)
{
    WindowObject *this = (WindowObject *)(self);

    if (this->win && this->win != INVALID_WINDOW_VALUE)
	this->win->w_python3_ref = NULL;
}

static int CheckWindow(WindowObject *this)
{
    if (this->win == INVALID_WINDOW_VALUE)
    {
	PyErr_SetVim(_("attempt to refer to deleted window"));
	return -1;
    }

    return 0;
}

static PyObject * WindowGetattro(PyObject *self, PyObject *nameobj)
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

static int WindowSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    WindowObject *this = (WindowObject *)(self);

    char *name = "";
    if (PyUnicode_Check(nameobj))
	name = _PyUnicode_AsString(nameobj);


    if (CheckWindow(this))
	return -1;

    if (strcmp(name, "buffer") == 0)
    {
	PyErr_SetString(PyExc_TypeError, _("readonly attribute"));
	return -1;
    }
    else if (strcmp(name, "cursor") == 0)
    {
	long lnum;
	long col;

	if (!PyArg_Parse(val, "(ll)", &lnum, &col))
	    return -1;

	if (lnum <= 0 || lnum > this->win->w_buffer->b_ml.ml_line_count)
	{
	    PyErr_SetVim(_("cursor position outside buffer"));
	    return -1;
	}

	/* Check for keyboard interrupts */
	if (VimErrorCheck())
	    return -1;

	/* NO CHECK ON COLUMN - SEEMS NOT TO MATTER */

	this->win->w_cursor.lnum = lnum;
	this->win->w_cursor.col = col;
	update_screen(VALID);

	return 0;
    }
    else if (strcmp(name, "height") == 0)
    {
	int     height;
	win_T   *savewin;

	if (!PyArg_Parse(val, "i", &height))
	    return -1;

#ifdef FEAT_GUI
	need_mouse_correct = TRUE;
#endif
	savewin = curwin;
	curwin = this->win;
	win_setheight(height);
	curwin = savewin;

	/* Check for keyboard interrupts */
	if (VimErrorCheck())
	    return -1;

	return 0;
    }
#ifdef FEAT_VERTSPLIT
    else if (strcmp(name, "width") == 0)
    {
	int     width;
	win_T   *savewin;

	if (!PyArg_Parse(val, "i", &width))
	    return -1;

#ifdef FEAT_GUI
	need_mouse_correct = TRUE;
#endif
	savewin = curwin;
	curwin = this->win;
	win_setwidth(width);
	curwin = savewin;

	/* Check for keyboard interrupts */
	if (VimErrorCheck())
	    return -1;

	return 0;
    }
#endif
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
    }
}

static PyObject * WindowRepr(PyObject *self)
{
    static char repr[100];
    WindowObject *this = (WindowObject *)(self);

    if (this->win == INVALID_WINDOW_VALUE)
    {
	vim_snprintf(repr, 100, _("<window object (deleted) at %p>"), (self));
	return PyUnicode_FromString(repr);
    }
    else
    {
	int     i = 0;
	win_T   *w;

	for (w = firstwin; w != NULL && w != this->win; w = W_NEXT(w))
	    ++i;

	if (w == NULL)
	    vim_snprintf(repr, 100, _("<window object (unknown) at %p>"),
								      (self));
	else
	    vim_snprintf(repr, 100, _("<window %d>"), i);

	return PyUnicode_FromString(repr);
    }
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

/* Window list object - Implementation
 */
/*ARGSUSED*/
static Py_ssize_t WinListLength(PyObject *self UNUSED)
{
    win_T       *w = firstwin;
    Py_ssize_t  n = 0;

    while (w != NULL)
    {
	++n;
	w = W_NEXT(w);
    }

    return n;
}

/*ARGSUSED*/
static PyObject * WinListItem(PyObject *self UNUSED, Py_ssize_t n)
{
    win_T *w;

    for (w = firstwin; w != NULL; w = W_NEXT(w), --n)
	if (n == 0)
	    return WindowNew(w);

    PyErr_SetString(PyExc_IndexError, _("no such window"));
    return NULL;
}

/* Current items object - Definitions
 */

typedef struct
{
    PyObject_HEAD
}
CurrentObject;

static PyTypeObject CurrentType;

/* Current items object - Implementation
 */
/*ARGSUSED*/
static PyObject * CurrentGetattro(PyObject *self UNUSED, PyObject *nameobj)
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

/*ARGSUSED*/
static int CurrentSetattro(PyObject *self UNUSED, PyObject *nameobj, PyObject *value)
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

static PyMODINIT_FUNC Py3Init_vim(void)
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

/*************************************************************************
 * 4. Utility functions for handling the interface between Vim and Python.
 */


/* Get a list of lines from the specified buffer. The line numbers
 * are in Vim format (1-based). The range is from lo up to, but not
 * including, hi. The list is returned as a Python list of string objects.
 */
static PyObject * GetBufferLineList(buf_T *buf, Py_ssize_t lo, Py_ssize_t hi)
{
    Py_ssize_t i;
    Py_ssize_t n = hi - lo;
    PyObject *list = PyList_New(n);

    if (list == NULL)
	return NULL;

    for (i = 0; i < n; ++i)
    {
	PyObject *str = LineToString((char *)ml_get_buf(buf, (linenr_T)(lo+i), FALSE));

	/* Error check - was the Python string creation OK? */
	if (str == NULL)
	{
	    Py_DECREF(list);
	    return NULL;
	}

	/* Set the list item */
	if (PyList_SetItem(list, i, str))
	{
	    Py_DECREF(str);
	    Py_DECREF(list);
	    return NULL;
	}
    }

    /* The ownership of the Python list is passed to the caller (ie,
     * the caller should Py_DECREF() the object when it is finished
     * with it).
     */

    return list;
}

/* Get a line from the specified buffer. The line number is
 * in Vim format (1-based). The line is returned as a Python
 * string object.
 */
static PyObject * GetBufferLine(buf_T *buf, Py_ssize_t n)
{
    return LineToString((char *)ml_get_buf(buf, (linenr_T)n, FALSE));
}

/*
 * Check if deleting lines made the cursor position invalid.
 * Changed the lines from "lo" to "hi" and added "extra" lines (negative if
 * deleted).
 */
static void py_fix_cursor(linenr_T lo, linenr_T hi, linenr_T extra)
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
	else
	    check_cursor_col();
	changed_cline_bef_curs();
    }
    invalidate_botline();
}

/* Replace a line in the specified buffer. The line number is
 * in Vim format (1-based). The replacement line is given as
 * a Python string object. The object is checked for validity
 * and correct format. Errors are returned as a value of FAIL.
 * The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
static int SetBufferLine(buf_T *buf, Py_ssize_t n, PyObject *line, Py_ssize_t *len_change)
{
    /* First of all, we check the thpe of the supplied Python object.
     * There are three cases:
     *    1. NULL, or None - this is a deletion.
     *    2. A string      - this is a replacement.
     *    3. Anything else - this is an error.
     */
    if (line == Py_None || line == NULL)
    {
	buf_T *savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_savedel((linenr_T)n, 1L) == FAIL)
	    PyErr_SetVim(_("cannot save undo information"));
	else if (ml_delete((linenr_T)n, FALSE) == FAIL)
	    PyErr_SetVim(_("cannot delete line"));
	else
	{
	    deleted_lines_mark((linenr_T)n, 1L);
	    if (buf == curwin->w_buffer)
		py_fix_cursor((linenr_T)n, (linenr_T)n + 1, (linenr_T)-1);
	}

	curbuf = savebuf;

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = -1;

	return OK;
    }
    else if (PyUnicode_Check(line))
    {
	char *save = StringToLine(line);
	buf_T *savebuf = curbuf;

	if (save == NULL)
	    return FAIL;

	/* We do not need to free "save" if ml_replace() consumes it. */
	PyErr_Clear();
	curbuf = buf;

	if (u_savesub((linenr_T)n) == FAIL)
	{
	    PyErr_SetVim(_("cannot save undo information"));
	    vim_free(save);
	}
	else if (ml_replace((linenr_T)n, (char_u *)save, FALSE) == FAIL)
	{
	    PyErr_SetVim(_("cannot replace line"));
	    vim_free(save);
	}
	else
	    changed_bytes((linenr_T)n, 0);

	curbuf = savebuf;

	/* Check that the cursor is not beyond the end of the line now. */
	if (buf == curwin->w_buffer)
	    check_cursor_col();

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = 0;

	return OK;
    }
    else
    {
	PyErr_BadArgument();
	return FAIL;
    }
}

/* Insert a number of lines into the specified buffer after the specifed line.
 * The line number is in Vim format (1-based). The lines to be inserted are
 * given as a Python list of string objects or as a single string. The lines
 * to be added are checked for validity and correct format. Errors are
 * returned as a value of FAIL.  The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
static int InsertBufferLines(buf_T *buf, Py_ssize_t n, PyObject *lines, Py_ssize_t *len_change)
{
    /* First of all, we check the type of the supplied Python object.
     * It must be a string or a list, or the call is in error.
     */
    if (PyUnicode_Check(lines))
    {
	char    *str = StringToLine(lines);
	buf_T   *savebuf;

	if (str == NULL)
	    return FAIL;

	savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_save((linenr_T)n, (linenr_T)(n+1)) == FAIL)
	    PyErr_SetVim(_("cannot save undo information"));
	else if (ml_append((linenr_T)n, (char_u *)str, 0, FALSE) == FAIL)
	    PyErr_SetVim(_("cannot insert line"));
	else
	    appended_lines_mark((linenr_T)n, 1L);

	vim_free(str);
	curbuf = savebuf;
	update_screen(VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = 1;

	return OK;
    }
    else if (PyList_Check(lines))
    {
	Py_ssize_t      i;
	Py_ssize_t      size = PyList_Size(lines);
	char    **array;
	buf_T   *savebuf;

	array = (char **)alloc((unsigned)(size * sizeof(char *)));
	if (array == NULL)
	{
	    PyErr_NoMemory();
	    return FAIL;
	}

	for (i = 0; i < size; ++i)
	{
	    PyObject *line = PyList_GetItem(lines, i);
	    array[i] = StringToLine(line);

	    if (array[i] == NULL)
	    {
		while (i)
		    vim_free(array[--i]);
		vim_free(array);
		return FAIL;
	    }
	}

	savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_save((linenr_T)n, (linenr_T)(n + 1)) == FAIL)
	    PyErr_SetVim(_("cannot save undo information"));
	else
	{
	    for (i = 0; i < size; ++i)
	    {
		if (ml_append((linenr_T)(n + i),
					(char_u *)array[i], 0, FALSE) == FAIL)
		{
		    PyErr_SetVim(_("cannot insert line"));

		    /* Free the rest of the lines */
		    while (i < size)
			vim_free(array[i++]);

		    break;
		}
		vim_free(array[i]);
	    }
	    if (i > 0)
		appended_lines_mark((linenr_T)n, (long)i);
	}

	/* Free the array of lines. All of its contents have now
	 * been freed.
	 */
	vim_free(array);

	curbuf = savebuf;
	update_screen(VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = size;

	return OK;
    }
    else
    {
	PyErr_BadArgument();
	return FAIL;
    }
}

/* Convert a Vim line into a Python string.
 * All internal newlines are replaced by null characters.
 *
 * On errors, the Python exception data is set, and NULL is returned.
 */
static PyObject * LineToString(const char *str)
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

/* Convert a Python string into a Vim line.
 *
 * The result is in allocated memory. All internal nulls are replaced by
 * newline characters. It is an error for the string to contain newline
 * characters.
 *
 * On errors, the Python exception data is set, and NULL is returned.
 */
static char * StringToLine(PyObject *obj)
{
    const char *str;
    char *save;
    Py_ssize_t len;
    Py_ssize_t i;
    char *p;

    if (obj == NULL || !PyUnicode_Check(obj))
    {
	PyErr_BadArgument();
	return NULL;
    }

    str = _PyUnicode_AsString(obj);
    len = PyUnicode_GET_SIZE(obj);

    /*
     * Error checking: String must not contain newlines, as we
     * are replacing a single line, and we must replace it with
     * a single line.
     * A trailing newline is removed, so that append(f.readlines()) works.
     */
    p = memchr(str, '\n', len);
    if (p != NULL)
    {
	if (p == str + len - 1)
	    --len;
	else
	{
	    PyErr_SetVim(_("string cannot contain newlines"));
	    return NULL;
	}
    }

    /* Create a copy of the string, with internal nulls replaced by
     * newline characters, as is the vim convention.
     */
    save = (char *)alloc((unsigned)(len+1));
    if (save == NULL)
    {
	PyErr_NoMemory();
	return NULL;
    }

    for (i = 0; i < len; ++i)
    {
	if (str[i] == '\0')
	    save[i] = '\n';
	else
	    save[i] = str[i];
    }

    save[i] = '\0';

    return save;
}

/* Check to see whether a Vim error has been reported, or a keyboard
 * interrupt has been detected.
 */
static int VimErrorCheck(void)
{
    if (got_int)
    {
	PyErr_SetNone(PyExc_KeyboardInterrupt);
	return 1;
    }
    else if (did_emsg && !PyErr_Occurred())
    {
	PyErr_SetNone(VimError);
	return 1;
    }

    return 0;
}

static void init_structs(void)
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
