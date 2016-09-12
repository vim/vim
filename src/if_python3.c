/* vi:set ts=8 sts=4 sw=4 noet:
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
/* Note: most of time you can add -DPy_DEBUG to CFLAGS in place of uncommenting
 */
/* uncomment this if used with the debug version of python, but without its
 * allocator */
/* #define Py_DEBUG_NO_PYMALLOC */

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

#ifdef F_BLANK
# undef F_BLANK
#endif

#ifdef HAVE_STRFTIME
# undef HAVE_STRFTIME
#endif
#ifdef HAVE_STRING_H
# undef HAVE_STRING_H
#endif
#ifdef HAVE_PUTENV
# undef HAVE_PUTENV
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

#define PY_SSIZE_T_CLEAN

#include <Python.h>

#if defined(MACOS) && !defined(MACOS_X_UNIX)
# include "macglue.h"
# include <CodeFragments.h>
#endif
#undef main /* Defined in python.h - aargh */
#undef HAVE_FCNTL_H /* Clash with os_win32.h */

/* The "surrogateescape" error handler is new in Python 3.1 */
#if PY_VERSION_HEX >= 0x030100f0
# define CODEC_ERROR_HANDLER "surrogateescape"
#else
# define CODEC_ERROR_HANDLER NULL
#endif

/* Python 3 does not support CObjects, always use Capsules */
#define PY_USE_CAPSULE

#define PyInt Py_ssize_t
#ifndef PyString_Check
# define PyString_Check(obj) PyUnicode_Check(obj)
#endif
#define PyString_FromString(repr) \
    PyUnicode_Decode(repr, STRLEN(repr), ENC_OPT, NULL)
#define PyString_FromFormat PyUnicode_FromFormat
#ifndef PyInt_Check
# define PyInt_Check(obj) PyLong_Check(obj)
#endif
#define PyInt_FromLong(i) PyLong_FromLong(i)
#define PyInt_AsLong(obj) PyLong_AsLong(obj)
#define Py_ssize_t_fmt "n"
#define Py_bytes_fmt "y"

#define PyIntArgFunc	ssizeargfunc
#define PyIntObjArgProc	ssizeobjargproc

/*
 * PySlice_GetIndicesEx(): first argument type changed from PySliceObject
 * to PyObject in Python 3.2 or later.
 */
#if PY_VERSION_HEX >= 0x030200f0
typedef PyObject PySliceObject_T;
#else
typedef PySliceObject PySliceObject_T;
#endif

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
# define PyMem_Free py3_PyMem_Free
# define PyMem_Malloc py3_PyMem_Malloc
# define PyDict_SetItemString py3_PyDict_SetItemString
# define PyErr_BadArgument py3_PyErr_BadArgument
# define PyErr_Clear py3_PyErr_Clear
# define PyErr_Format py3_PyErr_Format
# define PyErr_PrintEx py3_PyErr_PrintEx
# define PyErr_NoMemory py3_PyErr_NoMemory
# define PyErr_Occurred py3_PyErr_Occurred
# define PyErr_SetNone py3_PyErr_SetNone
# define PyErr_SetString py3_PyErr_SetString
# define PyErr_SetObject py3_PyErr_SetObject
# define PyErr_ExceptionMatches py3_PyErr_ExceptionMatches
# define PyEval_InitThreads py3_PyEval_InitThreads
# define PyEval_RestoreThread py3_PyEval_RestoreThread
# define PyEval_SaveThread py3_PyEval_SaveThread
# define PyGILState_Ensure py3_PyGILState_Ensure
# define PyGILState_Release py3_PyGILState_Release
# define PyLong_AsLong py3_PyLong_AsLong
# define PyLong_FromLong py3_PyLong_FromLong
# define PyList_GetItem py3_PyList_GetItem
# define PyList_Append py3_PyList_Append
# define PyList_Insert py3_PyList_Insert
# define PyList_New py3_PyList_New
# define PyList_SetItem py3_PyList_SetItem
# define PyList_Size py3_PyList_Size
# define PySequence_Check py3_PySequence_Check
# define PySequence_Size py3_PySequence_Size
# define PySequence_GetItem py3_PySequence_GetItem
# define PySequence_Fast py3_PySequence_Fast
# define PyTuple_Size py3_PyTuple_Size
# define PyTuple_GetItem py3_PyTuple_GetItem
# define PySlice_GetIndicesEx py3_PySlice_GetIndicesEx
# define PyImport_ImportModule py3_PyImport_ImportModule
# define PyObject_Init py3__PyObject_Init
# define PyDict_New py3_PyDict_New
# define PyDict_GetItemString py3_PyDict_GetItemString
# define PyDict_Next py3_PyDict_Next
# define PyMapping_Check py3_PyMapping_Check
# ifndef PyMapping_Keys
#  define PyMapping_Keys py3_PyMapping_Keys
# endif
# define PyIter_Next py3_PyIter_Next
# define PyObject_GetIter py3_PyObject_GetIter
# define PyObject_Repr py3_PyObject_Repr
# define PyObject_GetItem py3_PyObject_GetItem
# define PyObject_IsTrue py3_PyObject_IsTrue
# define PyModule_GetDict py3_PyModule_GetDict
#undef PyRun_SimpleString
# define PyRun_SimpleString py3_PyRun_SimpleString
#undef PyRun_String
# define PyRun_String py3_PyRun_String
# define PyObject_GetAttrString py3_PyObject_GetAttrString
# define PyObject_HasAttrString py3_PyObject_HasAttrString
# define PyObject_SetAttrString py3_PyObject_SetAttrString
# define PyObject_CallFunctionObjArgs py3_PyObject_CallFunctionObjArgs
# define _PyObject_CallFunction_SizeT py3__PyObject_CallFunction_SizeT
# define PyObject_Call py3_PyObject_Call
# define PyEval_GetLocals py3_PyEval_GetLocals
# define PyEval_GetGlobals py3_PyEval_GetGlobals
# define PySys_SetObject py3_PySys_SetObject
# define PySys_GetObject py3_PySys_GetObject
# define PySys_SetArgv py3_PySys_SetArgv
# define PyType_Ready py3_PyType_Ready
#undef Py_BuildValue
# define Py_BuildValue py3_Py_BuildValue
# define Py_SetPythonHome py3_Py_SetPythonHome
# define Py_Initialize py3_Py_Initialize
# define Py_Finalize py3_Py_Finalize
# define Py_IsInitialized py3_Py_IsInitialized
# define _Py_NoneStruct (*py3__Py_NoneStruct)
# define _Py_FalseStruct (*py3__Py_FalseStruct)
# define _Py_TrueStruct (*py3__Py_TrueStruct)
# define _PyObject_NextNotImplemented (*py3__PyObject_NextNotImplemented)
# define PyModule_AddObject py3_PyModule_AddObject
# define PyImport_AppendInittab py3_PyImport_AppendInittab
# define PyImport_AddModule py3_PyImport_AddModule
# if PY_VERSION_HEX >= 0x030300f0
#  undef _PyUnicode_AsString
#  define _PyUnicode_AsString py3_PyUnicode_AsUTF8
# else
#  define _PyUnicode_AsString py3__PyUnicode_AsString
# endif
# undef PyUnicode_AsEncodedString
# define PyUnicode_AsEncodedString py3_PyUnicode_AsEncodedString
# undef PyBytes_AsString
# define PyBytes_AsString py3_PyBytes_AsString
# ifndef PyBytes_AsStringAndSize
#  define PyBytes_AsStringAndSize py3_PyBytes_AsStringAndSize
# endif
# undef PyBytes_FromString
# define PyBytes_FromString py3_PyBytes_FromString
# define PyFloat_FromDouble py3_PyFloat_FromDouble
# define PyFloat_AsDouble py3_PyFloat_AsDouble
# define PyObject_GenericGetAttr py3_PyObject_GenericGetAttr
# define PyType_Type (*py3_PyType_Type)
# define PySlice_Type (*py3_PySlice_Type)
# define PyFloat_Type (*py3_PyFloat_Type)
# define PyNumber_Check (*py3_PyNumber_Check)
# define PyNumber_Long (*py3_PyNumber_Long)
# define PyBool_Type (*py3_PyBool_Type)
# define PyErr_NewException py3_PyErr_NewException
# ifdef Py_DEBUG
#  define _Py_NegativeRefcount py3__Py_NegativeRefcount
#  define _Py_RefTotal (*py3__Py_RefTotal)
#  define _Py_Dealloc py3__Py_Dealloc
#  define PyModule_Create2TraceRefs py3_PyModule_Create2TraceRefs
# else
#  define PyModule_Create2 py3_PyModule_Create2
# endif
# if defined(Py_DEBUG) && !defined(Py_DEBUG_NO_PYMALLOC)
#  define _PyObject_DebugMalloc py3__PyObject_DebugMalloc
#  define _PyObject_DebugFree py3__PyObject_DebugFree
# else
#  define PyObject_Malloc py3_PyObject_Malloc
#  define PyObject_Free py3_PyObject_Free
# endif
# define _PyObject_GC_New py3__PyObject_GC_New
# define PyObject_GC_Del py3_PyObject_GC_Del
# define PyObject_GC_UnTrack py3_PyObject_GC_UnTrack
# define PyType_GenericAlloc py3_PyType_GenericAlloc
# define PyType_GenericNew py3_PyType_GenericNew
# undef PyUnicode_FromString
# define PyUnicode_FromString py3_PyUnicode_FromString
# ifndef PyUnicode_FromFormat
#  define PyUnicode_FromFormat py3_PyUnicode_FromFormat
# else
#  define Py_UNICODE_USE_UCS_FUNCTIONS
#  ifdef Py_UNICODE_WIDE
#   define PyUnicodeUCS4_FromFormat py3_PyUnicodeUCS4_FromFormat
#  else
#   define PyUnicodeUCS2_FromFormat py3_PyUnicodeUCS2_FromFormat
#  endif
# endif
# undef PyUnicode_Decode
# define PyUnicode_Decode py3_PyUnicode_Decode
# define PyType_IsSubtype py3_PyType_IsSubtype
# define PyCapsule_New py3_PyCapsule_New
# define PyCapsule_GetPointer py3_PyCapsule_GetPointer

# if defined(Py_DEBUG) && !defined(Py_DEBUG_NO_PYMALLOC)
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
static PyObject* (*py3_PySys_GetObject)(char *);
static int (*py3_PyList_Append)(PyObject *, PyObject *);
static int (*py3_PyList_Insert)(PyObject *, int, PyObject *);
static Py_ssize_t (*py3_PyList_Size)(PyObject *);
static int (*py3_PySequence_Check)(PyObject *);
static Py_ssize_t (*py3_PySequence_Size)(PyObject *);
static PyObject* (*py3_PySequence_GetItem)(PyObject *, Py_ssize_t);
static PyObject* (*py3_PySequence_Fast)(PyObject *, const char *);
static Py_ssize_t (*py3_PyTuple_Size)(PyObject *);
static PyObject* (*py3_PyTuple_GetItem)(PyObject *, Py_ssize_t);
static int (*py3_PyMapping_Check)(PyObject *);
static PyObject* (*py3_PyMapping_Keys)(PyObject *);
static int (*py3_PySlice_GetIndicesEx)(PySliceObject_T *r, Py_ssize_t length,
		     Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step,
		     Py_ssize_t *slicelen);
static PyObject* (*py3_PyErr_NoMemory)(void);
static void (*py3_Py_Finalize)(void);
static void (*py3_PyErr_SetString)(PyObject *, const char *);
static void (*py3_PyErr_SetObject)(PyObject *, PyObject *);
static int (*py3_PyErr_ExceptionMatches)(PyObject *);
static int (*py3_PyRun_SimpleString)(char *);
static PyObject* (*py3_PyRun_String)(char *, int, PyObject *, PyObject *);
static PyObject* (*py3_PyObject_GetAttrString)(PyObject *, const char *);
static int (*py3_PyObject_HasAttrString)(PyObject *, const char *);
static int (*py3_PyObject_SetAttrString)(PyObject *, const char *, PyObject *);
static PyObject* (*py3_PyObject_CallFunctionObjArgs)(PyObject *, ...);
static PyObject* (*py3__PyObject_CallFunction_SizeT)(PyObject *, char *, ...);
static PyObject* (*py3_PyObject_Call)(PyObject *, PyObject *, PyObject *);
static PyObject* (*py3_PyEval_GetGlobals)();
static PyObject* (*py3_PyEval_GetLocals)();
static PyObject* (*py3_PyList_GetItem)(PyObject *, Py_ssize_t);
static PyObject* (*py3_PyImport_ImportModule)(const char *);
static PyObject* (*py3_PyImport_AddModule)(const char *);
static int (*py3_PyErr_BadArgument)(void);
static PyObject* (*py3_PyErr_Occurred)(void);
static PyObject* (*py3_PyModule_GetDict)(PyObject *);
static int (*py3_PyList_SetItem)(PyObject *, Py_ssize_t, PyObject *);
static PyObject* (*py3_PyDict_GetItemString)(PyObject *, const char *);
static int (*py3_PyDict_Next)(PyObject *, Py_ssize_t *, PyObject **, PyObject **);
static PyObject* (*py3_PyLong_FromLong)(long);
static PyObject* (*py3_PyDict_New)(void);
static PyObject* (*py3_PyIter_Next)(PyObject *);
static PyObject* (*py3_PyObject_GetIter)(PyObject *);
static PyObject* (*py3_PyObject_Repr)(PyObject *);
static PyObject* (*py3_PyObject_GetItem)(PyObject *, PyObject *);
static int (*py3_PyObject_IsTrue)(PyObject *);
static PyObject* (*py3_Py_BuildValue)(char *, ...);
static int (*py3_PyType_Ready)(PyTypeObject *type);
static int (*py3_PyDict_SetItemString)(PyObject *dp, char *key, PyObject *item);
static PyObject* (*py3_PyUnicode_FromString)(const char *u);
# ifndef Py_UNICODE_USE_UCS_FUNCTIONS
static PyObject* (*py3_PyUnicode_FromFormat)(const char *u, ...);
# else
#  ifdef Py_UNICODE_WIDE
static PyObject* (*py3_PyUnicodeUCS4_FromFormat)(const char *u, ...);
#  else
static PyObject* (*py3_PyUnicodeUCS2_FromFormat)(const char *u, ...);
#  endif
# endif
static PyObject* (*py3_PyUnicode_Decode)(const char *u, Py_ssize_t size,
	const char *encoding, const char *errors);
static long (*py3_PyLong_AsLong)(PyObject *);
static void (*py3_PyErr_SetNone)(PyObject *);
static void (*py3_PyEval_InitThreads)(void);
static void(*py3_PyEval_RestoreThread)(PyThreadState *);
static PyThreadState*(*py3_PyEval_SaveThread)(void);
static int (*py3_PyArg_Parse)(PyObject *, char *, ...);
static int (*py3_PyArg_ParseTuple)(PyObject *, char *, ...);
static int (*py3_PyMem_Free)(void *);
static void* (*py3_PyMem_Malloc)(size_t);
static int (*py3_Py_IsInitialized)(void);
static void (*py3_PyErr_Clear)(void);
static PyObject* (*py3_PyErr_Format)(PyObject *, const char *, ...);
static void (*py3_PyErr_PrintEx)(int);
static PyObject*(*py3__PyObject_Init)(PyObject *, PyTypeObject *);
static iternextfunc py3__PyObject_NextNotImplemented;
static PyObject* py3__Py_NoneStruct;
static PyObject* py3__Py_FalseStruct;
static PyObject* py3__Py_TrueStruct;
static int (*py3_PyModule_AddObject)(PyObject *m, const char *name, PyObject *o);
static int (*py3_PyImport_AppendInittab)(const char *name, PyObject* (*initfunc)(void));
# if PY_VERSION_HEX >= 0x030300f0
static char* (*py3_PyUnicode_AsUTF8)(PyObject *unicode);
# else
static char* (*py3__PyUnicode_AsString)(PyObject *unicode);
# endif
static PyObject* (*py3_PyUnicode_AsEncodedString)(PyObject *unicode, const char* encoding, const char* errors);
static char* (*py3_PyBytes_AsString)(PyObject *bytes);
static int (*py3_PyBytes_AsStringAndSize)(PyObject *bytes, char **buffer, Py_ssize_t *length);
static PyObject* (*py3_PyBytes_FromString)(char *str);
static PyObject* (*py3_PyFloat_FromDouble)(double num);
static double (*py3_PyFloat_AsDouble)(PyObject *);
static PyObject* (*py3_PyObject_GenericGetAttr)(PyObject *obj, PyObject *name);
static PyObject* (*py3_PyType_GenericAlloc)(PyTypeObject *type, Py_ssize_t nitems);
static PyObject* (*py3_PyType_GenericNew)(PyTypeObject *type, PyObject *args, PyObject *kwds);
static PyTypeObject* py3_PyType_Type;
static PyTypeObject* py3_PySlice_Type;
static PyTypeObject* py3_PyFloat_Type;
static PyTypeObject* py3_PyBool_Type;
static int (*py3_PyNumber_Check)(PyObject *);
static PyObject* (*py3_PyNumber_Long)(PyObject *);
static PyObject* (*py3_PyErr_NewException)(char *name, PyObject *base, PyObject *dict);
static PyObject* (*py3_PyCapsule_New)(void *, char *, PyCapsule_Destructor);
static void* (*py3_PyCapsule_GetPointer)(PyObject *, char *);
# ifdef Py_DEBUG
static void (*py3__Py_NegativeRefcount)(const char *fname, int lineno, PyObject *op);
static Py_ssize_t* py3__Py_RefTotal;
static void (*py3__Py_Dealloc)(PyObject *obj);
static PyObject* (*py3_PyModule_Create2TraceRefs)(struct PyModuleDef* module, int module_api_version);
# else
static PyObject* (*py3_PyModule_Create2)(struct PyModuleDef* module, int module_api_version);
# endif
# if defined(Py_DEBUG) && !defined(Py_DEBUG_NO_PYMALLOC)
static void (*py3__PyObject_DebugFree)(void*);
static void* (*py3__PyObject_DebugMalloc)(size_t);
# else
static void (*py3_PyObject_Free)(void*);
static void* (*py3_PyObject_Malloc)(size_t);
# endif
static PyObject*(*py3__PyObject_GC_New)(PyTypeObject *);
static void(*py3_PyObject_GC_Del)(void *);
static void(*py3_PyObject_GC_UnTrack)(void *);
static int (*py3_PyType_IsSubtype)(PyTypeObject *, PyTypeObject *);

static HINSTANCE hinstPy3 = 0; /* Instance of python.dll */

/* Imported exception objects */
static PyObject *p3imp_PyExc_AttributeError;
static PyObject *p3imp_PyExc_IndexError;
static PyObject *p3imp_PyExc_KeyError;
static PyObject *p3imp_PyExc_KeyboardInterrupt;
static PyObject *p3imp_PyExc_TypeError;
static PyObject *p3imp_PyExc_ValueError;
static PyObject *p3imp_PyExc_SystemExit;
static PyObject *p3imp_PyExc_RuntimeError;
static PyObject *p3imp_PyExc_ImportError;
static PyObject *p3imp_PyExc_OverflowError;

# define PyExc_AttributeError p3imp_PyExc_AttributeError
# define PyExc_IndexError p3imp_PyExc_IndexError
# define PyExc_KeyError p3imp_PyExc_KeyError
# define PyExc_KeyboardInterrupt p3imp_PyExc_KeyboardInterrupt
# define PyExc_TypeError p3imp_PyExc_TypeError
# define PyExc_ValueError p3imp_PyExc_ValueError
# define PyExc_SystemExit p3imp_PyExc_SystemExit
# define PyExc_RuntimeError p3imp_PyExc_RuntimeError
# define PyExc_ImportError p3imp_PyExc_ImportError
# define PyExc_OverflowError p3imp_PyExc_OverflowError

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
    {"_PyArg_ParseTuple_SizeT", (PYTHON_PROC*)&py3_PyArg_ParseTuple},
    {"_Py_BuildValue_SizeT", (PYTHON_PROC*)&py3_Py_BuildValue},
    {"PyMem_Free", (PYTHON_PROC*)&py3_PyMem_Free},
    {"PyMem_Malloc", (PYTHON_PROC*)&py3_PyMem_Malloc},
    {"PyList_New", (PYTHON_PROC*)&py3_PyList_New},
    {"PyGILState_Ensure", (PYTHON_PROC*)&py3_PyGILState_Ensure},
    {"PyGILState_Release", (PYTHON_PROC*)&py3_PyGILState_Release},
    {"PySys_SetObject", (PYTHON_PROC*)&py3_PySys_SetObject},
    {"PySys_GetObject", (PYTHON_PROC*)&py3_PySys_GetObject},
    {"PyList_Append", (PYTHON_PROC*)&py3_PyList_Append},
    {"PyList_Insert", (PYTHON_PROC*)&py3_PyList_Insert},
    {"PyList_Size", (PYTHON_PROC*)&py3_PyList_Size},
    {"PySequence_Check", (PYTHON_PROC*)&py3_PySequence_Check},
    {"PySequence_Size", (PYTHON_PROC*)&py3_PySequence_Size},
    {"PySequence_GetItem", (PYTHON_PROC*)&py3_PySequence_GetItem},
    {"PySequence_Fast", (PYTHON_PROC*)&py3_PySequence_Fast},
    {"PyTuple_Size", (PYTHON_PROC*)&py3_PyTuple_Size},
    {"PyTuple_GetItem", (PYTHON_PROC*)&py3_PyTuple_GetItem},
    {"PySlice_GetIndicesEx", (PYTHON_PROC*)&py3_PySlice_GetIndicesEx},
    {"PyErr_NoMemory", (PYTHON_PROC*)&py3_PyErr_NoMemory},
    {"Py_Finalize", (PYTHON_PROC*)&py3_Py_Finalize},
    {"PyErr_SetString", (PYTHON_PROC*)&py3_PyErr_SetString},
    {"PyErr_SetObject", (PYTHON_PROC*)&py3_PyErr_SetObject},
    {"PyErr_ExceptionMatches", (PYTHON_PROC*)&py3_PyErr_ExceptionMatches},
    {"PyRun_SimpleString", (PYTHON_PROC*)&py3_PyRun_SimpleString},
    {"PyRun_String", (PYTHON_PROC*)&py3_PyRun_String},
    {"PyObject_GetAttrString", (PYTHON_PROC*)&py3_PyObject_GetAttrString},
    {"PyObject_HasAttrString", (PYTHON_PROC*)&py3_PyObject_HasAttrString},
    {"PyObject_SetAttrString", (PYTHON_PROC*)&py3_PyObject_SetAttrString},
    {"PyObject_CallFunctionObjArgs", (PYTHON_PROC*)&py3_PyObject_CallFunctionObjArgs},
    {"_PyObject_CallFunction_SizeT", (PYTHON_PROC*)&py3__PyObject_CallFunction_SizeT},
    {"PyObject_Call", (PYTHON_PROC*)&py3_PyObject_Call},
    {"PyEval_GetGlobals", (PYTHON_PROC*)&py3_PyEval_GetGlobals},
    {"PyEval_GetLocals", (PYTHON_PROC*)&py3_PyEval_GetLocals},
    {"PyList_GetItem", (PYTHON_PROC*)&py3_PyList_GetItem},
    {"PyImport_ImportModule", (PYTHON_PROC*)&py3_PyImport_ImportModule},
    {"PyImport_AddModule", (PYTHON_PROC*)&py3_PyImport_AddModule},
    {"PyErr_BadArgument", (PYTHON_PROC*)&py3_PyErr_BadArgument},
    {"PyErr_Occurred", (PYTHON_PROC*)&py3_PyErr_Occurred},
    {"PyModule_GetDict", (PYTHON_PROC*)&py3_PyModule_GetDict},
    {"PyList_SetItem", (PYTHON_PROC*)&py3_PyList_SetItem},
    {"PyDict_GetItemString", (PYTHON_PROC*)&py3_PyDict_GetItemString},
    {"PyDict_Next", (PYTHON_PROC*)&py3_PyDict_Next},
    {"PyMapping_Check", (PYTHON_PROC*)&py3_PyMapping_Check},
    {"PyMapping_Keys", (PYTHON_PROC*)&py3_PyMapping_Keys},
    {"PyIter_Next", (PYTHON_PROC*)&py3_PyIter_Next},
    {"PyObject_GetIter", (PYTHON_PROC*)&py3_PyObject_GetIter},
    {"PyObject_Repr", (PYTHON_PROC*)&py3_PyObject_Repr},
    {"PyObject_GetItem", (PYTHON_PROC*)&py3_PyObject_GetItem},
    {"PyObject_IsTrue", (PYTHON_PROC*)&py3_PyObject_IsTrue},
    {"PyLong_FromLong", (PYTHON_PROC*)&py3_PyLong_FromLong},
    {"PyDict_New", (PYTHON_PROC*)&py3_PyDict_New},
    {"PyType_Ready", (PYTHON_PROC*)&py3_PyType_Ready},
    {"PyDict_SetItemString", (PYTHON_PROC*)&py3_PyDict_SetItemString},
    {"PyLong_AsLong", (PYTHON_PROC*)&py3_PyLong_AsLong},
    {"PyErr_SetNone", (PYTHON_PROC*)&py3_PyErr_SetNone},
    {"PyEval_InitThreads", (PYTHON_PROC*)&py3_PyEval_InitThreads},
    {"PyEval_RestoreThread", (PYTHON_PROC*)&py3_PyEval_RestoreThread},
    {"PyEval_SaveThread", (PYTHON_PROC*)&py3_PyEval_SaveThread},
    {"_PyArg_Parse_SizeT", (PYTHON_PROC*)&py3_PyArg_Parse},
    {"Py_IsInitialized", (PYTHON_PROC*)&py3_Py_IsInitialized},
    {"_PyObject_NextNotImplemented", (PYTHON_PROC*)&py3__PyObject_NextNotImplemented},
    {"_Py_NoneStruct", (PYTHON_PROC*)&py3__Py_NoneStruct},
    {"_Py_FalseStruct", (PYTHON_PROC*)&py3__Py_FalseStruct},
    {"_Py_TrueStruct", (PYTHON_PROC*)&py3__Py_TrueStruct},
    {"PyErr_Clear", (PYTHON_PROC*)&py3_PyErr_Clear},
    {"PyErr_Format", (PYTHON_PROC*)&py3_PyErr_Format},
    {"PyErr_PrintEx", (PYTHON_PROC*)&py3_PyErr_PrintEx},
    {"PyObject_Init", (PYTHON_PROC*)&py3__PyObject_Init},
    {"PyModule_AddObject", (PYTHON_PROC*)&py3_PyModule_AddObject},
    {"PyImport_AppendInittab", (PYTHON_PROC*)&py3_PyImport_AppendInittab},
# if PY_VERSION_HEX >= 0x030300f0
    {"PyUnicode_AsUTF8", (PYTHON_PROC*)&py3_PyUnicode_AsUTF8},
# else
    {"_PyUnicode_AsString", (PYTHON_PROC*)&py3__PyUnicode_AsString},
# endif
# ifndef Py_UNICODE_USE_UCS_FUNCTIONS
    {"PyUnicode_FromFormat", (PYTHON_PROC*)&py3_PyUnicode_FromFormat},
# else
#  ifdef Py_UNICODE_WIDE
    {"PyUnicodeUCS4_FromFormat", (PYTHON_PROC*)&py3_PyUnicodeUCS4_FromFormat},
#  else
    {"PyUnicodeUCS2_FromFormat", (PYTHON_PROC*)&py3_PyUnicodeUCS2_FromFormat},
#  endif
# endif
    {"PyBytes_AsString", (PYTHON_PROC*)&py3_PyBytes_AsString},
    {"PyBytes_AsStringAndSize", (PYTHON_PROC*)&py3_PyBytes_AsStringAndSize},
    {"PyBytes_FromString", (PYTHON_PROC*)&py3_PyBytes_FromString},
    {"PyFloat_FromDouble", (PYTHON_PROC*)&py3_PyFloat_FromDouble},
    {"PyFloat_AsDouble", (PYTHON_PROC*)&py3_PyFloat_AsDouble},
    {"PyObject_GenericGetAttr", (PYTHON_PROC*)&py3_PyObject_GenericGetAttr},
    {"PyType_GenericAlloc", (PYTHON_PROC*)&py3_PyType_GenericAlloc},
    {"PyType_GenericNew", (PYTHON_PROC*)&py3_PyType_GenericNew},
    {"PyType_Type", (PYTHON_PROC*)&py3_PyType_Type},
    {"PySlice_Type", (PYTHON_PROC*)&py3_PySlice_Type},
    {"PyFloat_Type", (PYTHON_PROC*)&py3_PyFloat_Type},
    {"PyBool_Type", (PYTHON_PROC*)&py3_PyBool_Type},
    {"PyNumber_Check", (PYTHON_PROC*)&py3_PyNumber_Check},
    {"PyNumber_Long", (PYTHON_PROC*)&py3_PyNumber_Long},
    {"PyErr_NewException", (PYTHON_PROC*)&py3_PyErr_NewException},
# ifdef Py_DEBUG
    {"_Py_NegativeRefcount", (PYTHON_PROC*)&py3__Py_NegativeRefcount},
    {"_Py_RefTotal", (PYTHON_PROC*)&py3__Py_RefTotal},
    {"_Py_Dealloc", (PYTHON_PROC*)&py3__Py_Dealloc},
    {"PyModule_Create2TraceRefs", (PYTHON_PROC*)&py3_PyModule_Create2TraceRefs},
# else
    {"PyModule_Create2", (PYTHON_PROC*)&py3_PyModule_Create2},
# endif
# if defined(Py_DEBUG) && !defined(Py_DEBUG_NO_PYMALLOC)
    {"_PyObject_DebugFree", (PYTHON_PROC*)&py3__PyObject_DebugFree},
    {"_PyObject_DebugMalloc", (PYTHON_PROC*)&py3__PyObject_DebugMalloc},
# else
    {"PyObject_Malloc", (PYTHON_PROC*)&py3_PyObject_Malloc},
    {"PyObject_Free", (PYTHON_PROC*)&py3_PyObject_Free},
# endif
    {"_PyObject_GC_New", (PYTHON_PROC*)&py3__PyObject_GC_New},
    {"PyObject_GC_Del", (PYTHON_PROC*)&py3_PyObject_GC_Del},
    {"PyObject_GC_UnTrack", (PYTHON_PROC*)&py3_PyObject_GC_UnTrack},
    {"PyType_IsSubtype", (PYTHON_PROC*)&py3_PyType_IsSubtype},
    {"PyCapsule_New", (PYTHON_PROC*)&py3_PyCapsule_New},
    {"PyCapsule_GetPointer", (PYTHON_PROC*)&py3_PyCapsule_GetPointer},
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
    void *ucs_from_string, *ucs_decode, *ucs_as_encoded_string;

# if !(defined(PY_NO_RTLD_GLOBAL) && defined(PY3_NO_RTLD_GLOBAL)) && defined(UNIX) && defined(FEAT_PYTHON)
    /* Can't have Python and Python3 loaded at the same time.
     * It cause a crash, because RTLD_GLOBAL is needed for
     * standard C extension libraries of one or both python versions. */
    if (python_loaded())
    {
	if (verbose)
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
# if PY_VERSION_HEX >= 0x030300f0
    ucs_from_string = symbol_from_dll(hinstPy3, "PyUnicode_FromString");
    ucs_decode = symbol_from_dll(hinstPy3, "PyUnicode_Decode");
    ucs_as_encoded_string = symbol_from_dll(hinstPy3,
	    "PyUnicode_AsEncodedString");
# else
    ucs_from_string = symbol_from_dll(hinstPy3, "PyUnicodeUCS2_FromString");
    ucs_decode = symbol_from_dll(hinstPy3,
	    "PyUnicodeUCS2_Decode");
    ucs_as_encoded_string = symbol_from_dll(hinstPy3,
	    "PyUnicodeUCS2_AsEncodedString");
    if (!ucs_from_string || !ucs_decode || !ucs_as_encoded_string)
    {
	ucs_from_string = symbol_from_dll(hinstPy3,
		"PyUnicodeUCS4_FromString");
	ucs_decode = symbol_from_dll(hinstPy3,
		"PyUnicodeUCS4_Decode");
	ucs_as_encoded_string = symbol_from_dll(hinstPy3,
		"PyUnicodeUCS4_AsEncodedString");
    }
# endif
    if (ucs_from_string && ucs_decode && ucs_as_encoded_string)
    {
	py3_PyUnicode_FromString = ucs_from_string;
	py3_PyUnicode_Decode = ucs_decode;
	py3_PyUnicode_AsEncodedString = ucs_as_encoded_string;
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
    return py3_runtime_link_init((char *)p_py3dll, verbose) == OK;
}

/* Load the standard Python exceptions - don't import the symbols from the
 * DLL, as this can cause errors (importing data symbols is not reliable).
 */
static void get_py3_exceptions(void);

    static void
get_py3_exceptions(void)
{
    PyObject *exmod = PyImport_ImportModule("builtins");
    PyObject *exdict = PyModule_GetDict(exmod);
    p3imp_PyExc_AttributeError = PyDict_GetItemString(exdict, "AttributeError");
    p3imp_PyExc_IndexError = PyDict_GetItemString(exdict, "IndexError");
    p3imp_PyExc_KeyError = PyDict_GetItemString(exdict, "KeyError");
    p3imp_PyExc_KeyboardInterrupt = PyDict_GetItemString(exdict, "KeyboardInterrupt");
    p3imp_PyExc_TypeError = PyDict_GetItemString(exdict, "TypeError");
    p3imp_PyExc_ValueError = PyDict_GetItemString(exdict, "ValueError");
    p3imp_PyExc_SystemExit = PyDict_GetItemString(exdict, "SystemExit");
    p3imp_PyExc_RuntimeError = PyDict_GetItemString(exdict, "RuntimeError");
    p3imp_PyExc_ImportError = PyDict_GetItemString(exdict, "ImportError");
    p3imp_PyExc_OverflowError = PyDict_GetItemString(exdict, "OverflowError");
    Py_XINCREF(p3imp_PyExc_AttributeError);
    Py_XINCREF(p3imp_PyExc_IndexError);
    Py_XINCREF(p3imp_PyExc_KeyError);
    Py_XINCREF(p3imp_PyExc_KeyboardInterrupt);
    Py_XINCREF(p3imp_PyExc_TypeError);
    Py_XINCREF(p3imp_PyExc_ValueError);
    Py_XINCREF(p3imp_PyExc_SystemExit);
    Py_XINCREF(p3imp_PyExc_RuntimeError);
    Py_XINCREF(p3imp_PyExc_ImportError);
    Py_XINCREF(p3imp_PyExc_OverflowError);
    Py_XDECREF(exmod);
}
#endif /* DYNAMIC_PYTHON3 */

static int py3initialised = 0;

#define PYINITIALISED py3initialised

#define DESTRUCTOR_FINISH(self) Py_TYPE(self)->tp_free((PyObject*)self)

#define WIN_PYTHON_REF(win) win->w_python3_ref
#define BUF_PYTHON_REF(buf) buf->b_python3_ref
#define TAB_PYTHON_REF(tab) tab->tp_python3_ref

    static void
call_PyObject_Free(void *p)
{
#if defined(Py_DEBUG) && !defined(Py_DEBUG_NO_PYMALLOC)
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

static PyObject *OutputGetattro(PyObject *, PyObject *);
static int OutputSetattro(PyObject *, PyObject *, PyObject *);
static PyObject *BufferGetattro(PyObject *, PyObject *);
static int BufferSetattro(PyObject *, PyObject *, PyObject *);
static PyObject *TabPageGetattro(PyObject *, PyObject *);
static PyObject *WindowGetattro(PyObject *, PyObject *);
static int WindowSetattro(PyObject *, PyObject *, PyObject *);
static PyObject *RangeGetattro(PyObject *, PyObject *);
static PyObject *CurrentGetattro(PyObject *, PyObject *);
static int CurrentSetattro(PyObject *, PyObject *, PyObject *);
static PyObject *DictionaryGetattro(PyObject *, PyObject *);
static int DictionarySetattro(PyObject *, PyObject *, PyObject *);
static PyObject *ListGetattro(PyObject *, PyObject *);
static int ListSetattro(PyObject *, PyObject *, PyObject *);
static PyObject *FunctionGetattro(PyObject *, PyObject *);

static PyObject *VimPathHook(PyObject *, PyObject *);

static struct PyModuleDef vimmodule;

#define PY_CAN_RECURSE

/*
 * Include the code shared with if_python.c
 */
#include "if_py_both.h"

#define GET_ATTR_STRING(name, nameobj) \
    char	*name = ""; \
    if (PyUnicode_Check(nameobj)) \
	name = _PyUnicode_AsString(nameobj)

#define PY3OBJ_DELETED(obj) (obj->ob_base.ob_refcnt<=0)

/******************************************************
 * Internal function prototypes.
 */

static PyObject *Py3Init_vim(void);

/******************************************************
 * 1. Python interpreter main program.
 */

    void
python3_end(void)
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
	PyGILState_Ensure();

	Py_Finalize();
    }

#ifdef DYNAMIC_PYTHON3
    end_dynamic_python3();
#endif

    --recurse;
}

#if (defined(DYNAMIC_PYTHON3) && defined(DYNAMIC_PYTHON) && defined(FEAT_PYTHON) && defined(UNIX)) || defined(PROTO)
    int
python3_loaded(void)
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
# ifdef DYNAMIC_PYTHON3
	if (mch_getenv((char_u *)"PYTHONHOME") == NULL)
# endif
	    Py_SetPythonHome(PYTHON3_HOME);
#endif

	PyImport_AppendInittab("vim", Py3Init_vim);

#if !defined(MACOS) || defined(MACOS_X_UNIX)
	Py_Initialize();
#else
	PyMac_Initialize();
#endif
	/* Initialise threads, and below save the state using
	 * PyEval_SaveThread.  Without the call to PyEval_SaveThread, thread
	 * specific state (such as the system trace hook), will be lost
	 * between invocations of Python code. */
	PyEval_InitThreads();
#ifdef DYNAMIC_PYTHON3
	get_py3_exceptions();
#endif

	if (PythonIO_Init_io())
	    goto fail;

	globals = PyModule_GetDict(PyImport_AddModule("__main__"));

	/* Remove the element from sys.path that was added because of our
	 * argv[0] value in Py3Init_vim().  Previously we used an empty
	 * string, but depending on the OS we then get an empty entry or
	 * the current directory in sys.path.
	 * Only after vim has been imported, the element does exist in
	 * sys.path.
	 */
	PyRun_SimpleString("import vim; import sys; sys.path = list(filter(lambda x: not x.endswith('must>not&exist'), sys.path))");

	/* lock is created and acquired in PyEval_InitThreads() and thread
	 * state is created in Py_Initialize()
	 * there _PyGILState_NoteThreadState() also sets gilcounter to 1
	 * (python must have threads enabled!)
	 * so the following does both: unlock GIL and save thread state in TLS
	 * without deleting thread state
	 */
	PyEval_SaveThread();

	py3initialised = 1;
    }

    return 0;

fail:
    /* We call PythonIO_Flush() here to print any Python errors.
     * This is OK, as it is possible to call this function even
     * if PythonIO_Init_io() has not completed successfully (it will
     * not do anything in this case).
     */
    PythonIO_Flush();
    return -1;
}

/*
 * External interface
 */
    static void
DoPyCommand(const char *cmd, rangeinitializer init_range, runner run, void *arg)
{
#if defined(MACOS) && !defined(MACOS_X_UNIX)
    GrafPtr		oldPort;
#endif
#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    char		*saved_locale;
#endif
    PyObject		*cmdstr;
    PyObject		*cmdbytes;
    PyGILState_STATE	pygilstate;

#if defined(MACOS) && !defined(MACOS_X_UNIX)
    GetPort(&oldPort);
    /* Check if the Python library is available */
    if ((Ptr)PyMac_Initialize == (Ptr)kUnresolvedCFragSymbolAddress)
	goto theend;
#endif
    if (Python3_Init())
	goto theend;

    init_range(arg);

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

    /* PyRun_SimpleString expects a UTF-8 string. Wrong encoding may cause
     * SyntaxError (unicode error). */
    cmdstr = PyUnicode_Decode(cmd, strlen(cmd),
					(char *)ENC_OPT, CODEC_ERROR_HANDLER);
    cmdbytes = PyUnicode_AsEncodedString(cmdstr, "utf-8", CODEC_ERROR_HANDLER);
    Py_XDECREF(cmdstr);

    run(PyBytes_AsString(cmdbytes), arg, &pygilstate);
    Py_XDECREF(cmdbytes);

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
	DoPyCommand(script == NULL ? (char *) eap->arg : (char *) script,
		(rangeinitializer) init_range_cmd,
		(runner) run_cmd,
		(void *) eap);
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
     * construct: exec(compile(open('a_filename', 'rb').read(), 'a_filename', 'exec'))
     *
     * Using bytes so that Python can detect the source encoding as it normally
     * does. The doc does not say "compile" accept bytes, though.
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
	    strcpy(p,"','rb').read(),'");
	    p += 16;
	}
	else
	{
	    strcpy(p,"','exec'))");
	    p += 10;
	}
    }


    /* Execute the file */
    DoPyCommand(buffer,
	    (rangeinitializer) init_range_cmd,
	    (runner) run_cmd,
	    (void *) eap);
}

    void
ex_py3do(exarg_T *eap)
{
    DoPyCommand((char *)eap->arg,
	    (rangeinitializer)init_range_cmd,
	    (runner)run_do,
	    (void *)eap);
}

/******************************************************
 * 2. Python output stream: writes output via [e]msg().
 */

/* Implementation functions
 */

    static PyObject *
OutputGetattro(PyObject *self, PyObject *nameobj)
{
    GET_ATTR_STRING(name, nameobj);

    if (strcmp(name, "softspace") == 0)
	return PyLong_FromLong(((OutputObject *)(self))->softspace);
    else if (strcmp(name, "errors") == 0)
	return PyString_FromString("strict");
    else if (strcmp(name, "encoding") == 0)
	return PyString_FromString(ENC_OPT);

    return PyObject_GenericGetAttr(self, nameobj);
}

    static int
OutputSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    GET_ATTR_STRING(name, nameobj);

    return OutputSetattr((OutputObject *)(self), name, val);
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

static PyObject* BufferSubscript(PyObject *self, PyObject *idx);
static Py_ssize_t BufferAsSubscript(PyObject *self, PyObject *idx, PyObject *val);

/* Line range type - Implementation functions
 * --------------------------------------
 */

#define RangeType_Check(obj) ((obj)->ob_base.ob_type == &RangeType)

static PyObject* RangeSubscript(PyObject *self, PyObject *idx);
static Py_ssize_t RangeAsItem(PyObject *, Py_ssize_t, PyObject *);
static Py_ssize_t RangeAsSubscript(PyObject *self, PyObject *idx, PyObject *val);

/* Current objects type - Implementation functions
 * -----------------------------------------------
 */

static PySequenceMethods BufferAsSeq = {
    (lenfunc)		BufferLength,	    /* sq_length,    len(x)   */
    (binaryfunc)	0,		    /* sq_concat,    x+y      */
    (ssizeargfunc)	0,		    /* sq_repeat,    x*n      */
    (ssizeargfunc)	BufferItem,	    /* sq_item,      x[i]     */
    0,					    /* was_sq_slice,	 x[i:j]   */
    0,					    /* sq_ass_item,  x[i]=v   */
    0,					    /* sq_ass_slice, x[i:j]=v */
    0,					    /* sq_contains */
    0,					    /* sq_inplace_concat */
    0,					    /* sq_inplace_repeat */
};

static PyMappingMethods BufferAsMapping = {
    /* mp_length	*/ (lenfunc)BufferLength,
    /* mp_subscript     */ (binaryfunc)BufferSubscript,
    /* mp_ass_subscript */ (objobjargproc)BufferAsSubscript,
};


/* Buffer object
 */

    static PyObject *
BufferGetattro(PyObject *self, PyObject *nameobj)
{
    PyObject *r;

    GET_ATTR_STRING(name, nameobj);

    if ((r = BufferAttrValid((BufferObject *)(self), name)))
	return r;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    r = BufferAttr((BufferObject *)(self), name);
    if (r || PyErr_Occurred())
	return r;
    else
	return PyObject_GenericGetAttr(self, nameobj);
}

    static int
BufferSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    GET_ATTR_STRING(name, nameobj);

    return BufferSetattr((BufferObject *)(self), name, val);
}

/******************/

    static PyObject *
BufferSubscript(PyObject *self, PyObject* idx)
{
    if (PyLong_Check(idx))
    {
	long _idx = PyLong_AsLong(idx);
	return BufferItem((BufferObject *)(self), _idx);
    } else if (PySlice_Check(idx))
    {
	Py_ssize_t start, stop, step, slicelen;

	if (CheckBuffer((BufferObject *) self))
	    return NULL;

	if (PySlice_GetIndicesEx((PySliceObject_T *)idx,
	      (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count,
	      &start, &stop,
	      &step, &slicelen) < 0)
	{
	    return NULL;
	}
	return BufferSlice((BufferObject *)(self), start, stop);
    }
    else
    {
	RAISE_INVALID_INDEX_TYPE(idx);
	return NULL;
    }
}

    static Py_ssize_t
BufferAsSubscript(PyObject *self, PyObject* idx, PyObject* val)
{
    if (PyLong_Check(idx))
    {
	long n = PyLong_AsLong(idx);
	return RBAsItem((BufferObject *)(self), n, val, 1,
		    (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		    NULL);
    } else if (PySlice_Check(idx))
    {
	Py_ssize_t start, stop, step, slicelen;

	if (CheckBuffer((BufferObject *) self))
	    return -1;

	if (PySlice_GetIndicesEx((PySliceObject_T *)idx,
	      (Py_ssize_t)((BufferObject *)(self))->buf->b_ml.ml_line_count,
	      &start, &stop,
	      &step, &slicelen) < 0)
	{
	    return -1;
	}
	return RBAsSlice((BufferObject *)(self), start, stop, val, 1,
			  (PyInt)((BufferObject *)(self))->buf->b_ml.ml_line_count,
			  NULL);
    }
    else
    {
	RAISE_INVALID_INDEX_TYPE(idx);
	return -1;
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

static PyMappingMethods RangeAsMapping = {
    /* mp_length	*/ (lenfunc)RangeLength,
    /* mp_subscript     */ (binaryfunc)RangeSubscript,
    /* mp_ass_subscript */ (objobjargproc)RangeAsSubscript,
};

/* Line range object - Implementation
 */

    static PyObject *
RangeGetattro(PyObject *self, PyObject *nameobj)
{
    GET_ATTR_STRING(name, nameobj);

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

    static Py_ssize_t
RangeAsSlice(PyObject *self, Py_ssize_t lo, Py_ssize_t hi, PyObject *val)
{
    return RBAsSlice(((RangeObject *)(self))->buf, lo, hi, val,
		    ((RangeObject *)(self))->start,
		    ((RangeObject *)(self))->end,
		    &((RangeObject *)(self))->end);
}

    static PyObject *
RangeSubscript(PyObject *self, PyObject* idx)
{
    if (PyLong_Check(idx))
    {
	long _idx = PyLong_AsLong(idx);
	return RangeItem((RangeObject *)(self), _idx);
    } else if (PySlice_Check(idx))
    {
	Py_ssize_t start, stop, step, slicelen;

	if (PySlice_GetIndicesEx((PySliceObject_T *)idx,
		((RangeObject *)(self))->end-((RangeObject *)(self))->start+1,
		&start, &stop,
		&step, &slicelen) < 0)
	{
	    return NULL;
	}
	return RangeSlice((RangeObject *)(self), start, stop);
    }
    else
    {
	RAISE_INVALID_INDEX_TYPE(idx);
	return NULL;
    }
}

    static Py_ssize_t
RangeAsSubscript(PyObject *self, PyObject *idx, PyObject *val)
{
    if (PyLong_Check(idx))
    {
	long n = PyLong_AsLong(idx);
	return RangeAsItem(self, n, val);
    } else if (PySlice_Check(idx))
    {
	Py_ssize_t start, stop, step, slicelen;

	if (PySlice_GetIndicesEx((PySliceObject_T *)idx,
		((RangeObject *)(self))->end-((RangeObject *)(self))->start+1,
		&start, &stop,
		&step, &slicelen) < 0)
	{
	    return -1;
	}
	return RangeAsSlice(self, start, stop, val);
    }
    else
    {
	RAISE_INVALID_INDEX_TYPE(idx);
	return -1;
    }
}

/* TabPage object - Implementation
 */

    static PyObject *
TabPageGetattro(PyObject *self, PyObject *nameobj)
{
    PyObject *r;

    GET_ATTR_STRING(name, nameobj);

    if ((r = TabPageAttrValid((TabPageObject *)(self), name)))
	return r;

    if (CheckTabPage((TabPageObject *)(self)))
	return NULL;

    r = TabPageAttr((TabPageObject *)(self), name);
    if (r || PyErr_Occurred())
	return r;
    else
	return PyObject_GenericGetAttr(self, nameobj);
}

/* Window object - Implementation
 */

    static PyObject *
WindowGetattro(PyObject *self, PyObject *nameobj)
{
    PyObject *r;

    GET_ATTR_STRING(name, nameobj);

    if ((r = WindowAttrValid((WindowObject *)(self), name)))
	return r;

    if (CheckWindow((WindowObject *)(self)))
	return NULL;

    r = WindowAttr((WindowObject *)(self), name);
    if (r || PyErr_Occurred())
	return r;
    else
	return PyObject_GenericGetAttr(self, nameobj);
}

    static int
WindowSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    GET_ATTR_STRING(name, nameobj);

    return WindowSetattr((WindowObject *)(self), name, val);
}

/* Tab page list object - Definitions
 */

static PySequenceMethods TabListAsSeq = {
    (lenfunc)	     TabListLength,	    /* sq_length,    len(x)   */
    (binaryfunc)     0,			    /* sq_concat,    x+y      */
    (ssizeargfunc)   0,			    /* sq_repeat,    x*n      */
    (ssizeargfunc)   TabListItem,	    /* sq_item,      x[i]     */
    0,					    /* sq_slice,     x[i:j]   */
    (ssizeobjargproc)0,			    /* sq_as_item,  x[i]=v   */
    0,					    /* sq_ass_slice, x[i:j]=v */
    0,					    /* sq_contains */
    0,					    /* sq_inplace_concat */
    0,					    /* sq_inplace_repeat */
};

/* Window list object - Definitions
 */

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

/* Current items object - Implementation
 */
    static PyObject *
CurrentGetattro(PyObject *self, PyObject *nameobj)
{
    PyObject	*r;
    GET_ATTR_STRING(name, nameobj);
    if (!(r = CurrentGetattr(self, name)))
	return PyObject_GenericGetAttr(self, nameobj);
    return r;
}

    static int
CurrentSetattro(PyObject *self, PyObject *nameobj, PyObject *value)
{
    GET_ATTR_STRING(name, nameobj);
    return CurrentSetattr(self, name, value);
}

/* Dictionary object - Definitions
 */

    static PyObject *
DictionaryGetattro(PyObject *self, PyObject *nameobj)
{
    DictionaryObject	*this = ((DictionaryObject *) (self));

    GET_ATTR_STRING(name, nameobj);

    if (strcmp(name, "locked") == 0)
	return PyLong_FromLong(this->dict->dv_lock);
    else if (strcmp(name, "scope") == 0)
	return PyLong_FromLong(this->dict->dv_scope);

    return PyObject_GenericGetAttr(self, nameobj);
}

    static int
DictionarySetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    GET_ATTR_STRING(name, nameobj);
    return DictionarySetattr((DictionaryObject *)(self), name, val);
}

/* List object - Definitions
 */

    static PyObject *
ListGetattro(PyObject *self, PyObject *nameobj)
{
    GET_ATTR_STRING(name, nameobj);

    if (strcmp(name, "locked") == 0)
	return PyLong_FromLong(((ListObject *) (self))->list->lv_lock);

    return PyObject_GenericGetAttr(self, nameobj);
}

    static int
ListSetattro(PyObject *self, PyObject *nameobj, PyObject *val)
{
    GET_ATTR_STRING(name, nameobj);
    return ListSetattr((ListObject *)(self), name, val);
}

/* Function object - Definitions
 */

    static PyObject *
FunctionGetattro(PyObject *self, PyObject *nameobj)
{
    PyObject		*r;
    FunctionObject	*this = (FunctionObject *)(self);

    GET_ATTR_STRING(name, nameobj);

    r = FunctionAttr(this, name);
    if (r || PyErr_Occurred())
	return r;
    else
	return PyObject_GenericGetAttr(self, nameobj);
}

/* External interface
 */

    void
python3_buffer_free(buf_T *buf)
{
    if (BUF_PYTHON_REF(buf) != NULL)
    {
	BufferObject *bp = BUF_PYTHON_REF(buf);
	bp->buf = INVALID_BUFFER_VALUE;
	BUF_PYTHON_REF(buf) = NULL;
    }
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
python3_window_free(win_T *win)
{
    if (WIN_PYTHON_REF(win) != NULL)
    {
	WindowObject *wp = WIN_PYTHON_REF(win);
	wp->win = INVALID_WINDOW_VALUE;
	WIN_PYTHON_REF(win) = NULL;
    }
}

    void
python3_tabpage_free(tabpage_T *tab)
{
    if (TAB_PYTHON_REF(tab) != NULL)
    {
	TabPageObject *tp = TAB_PYTHON_REF(tab);
	tp->tab = INVALID_TABPAGE_VALUE;
	TAB_PYTHON_REF(tab) = NULL;
    }
}
#endif

    static PyObject *
Py3Init_vim(void)
{
    /* The special value is removed from sys.path in Python3_Init(). */
    static wchar_t *(argv[2]) = {L"/must>not&exist/foo", NULL};

    if (init_types())
	return NULL;

    /* Set sys.argv[] to avoid a crash in warn(). */
    PySys_SetArgv(1, argv);

    if ((vim_module = PyModule_Create(&vimmodule)) == NULL)
	return NULL;

    if (populate_module(vim_module))
	return NULL;

    if (init_sys_path())
	return NULL;

    return vim_module;
}

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

    result = PyUnicode_Decode(tmp, len, (char *)ENC_OPT, CODEC_ERROR_HANDLER);

    vim_free(tmp);
    return result;
}

    void
do_py3eval (char_u *str, typval_T *rettv)
{
    DoPyCommand((char *) str,
	    (rangeinitializer) init_range_eval,
	    (runner) run_eval,
	    (void *) rettv);
    switch(rettv->v_type)
    {
	case VAR_DICT: ++rettv->vval.v_dict->dv_refcount; break;
	case VAR_LIST: ++rettv->vval.v_list->lv_refcount; break;
	case VAR_FUNC: func_ref(rettv->vval.v_string);    break;
	case VAR_PARTIAL: ++rettv->vval.v_partial->pt_refcount; break;
	case VAR_UNKNOWN:
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = 0;
	    break;
	case VAR_NUMBER:
	case VAR_STRING:
	case VAR_FLOAT:
	case VAR_SPECIAL:
	case VAR_JOB:
	case VAR_CHANNEL:
	    break;
    }
}

    int
set_ref_in_python3 (int copyID)
{
    return set_ref_in_py(copyID);
}
