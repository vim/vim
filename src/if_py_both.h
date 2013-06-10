/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * Python extensions by Paul Moore, David Leonard, Roland Puntaier, Nikolay
 * Pavlov.
 *
 * Common code for if_python.c and if_python3.c.
 */

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;  /* Python 2.4 and earlier don't have this type. */
#endif

#ifdef FEAT_MBYTE
# define ENC_OPT p_enc
#else
# define ENC_OPT "latin1"
#endif
#define DOPY_FUNC "_vim_pydo"

static const char *vim_special_path = "_vim_path_";

#define PyErr_SetVim(str) PyErr_SetString(VimError, str)

#define RAISE_NO_EMPTY_KEYS PyErr_SetString(PyExc_ValueError, \
						_("empty keys are not allowed"))

#define INVALID_BUFFER_VALUE ((buf_T *)(-1))
#define INVALID_WINDOW_VALUE ((win_T *)(-1))
#define INVALID_TABPAGE_VALUE ((tabpage_T *)(-1))

typedef void (*rangeinitializer)(void *);
typedef void (*runner)(const char *, void *
#ifdef PY_CAN_RECURSE
	, PyGILState_STATE *
#endif
	);

static int ConvertFromPyObject(PyObject *, typval_T *);
static int _ConvertFromPyObject(PyObject *, typval_T *, PyObject *);
static int ConvertFromPyMapping(PyObject *, typval_T *);
static PyObject *WindowNew(win_T *, tabpage_T *);
static PyObject *BufferNew (buf_T *);
static PyObject *LineToString(const char *);

static PyInt RangeStart;
static PyInt RangeEnd;

static PyObject *globals;

static PyObject *py_chdir;
static PyObject *py_fchdir;
static PyObject *py_getcwd;
static PyObject *vim_module;
static PyObject *vim_special_path_object;

/*
 * obtain a lock on the Vim data structures
 */
    static void
Python_Lock_Vim(void)
{
}

/*
 * release a lock on the Vim data structures
 */
    static void
Python_Release_Vim(void)
{
}

/*
 * The "todecref" argument holds a pointer to PyObject * that must be
 * DECREF'ed after returned char_u * is no longer needed or NULL if all what
 * was needed to generate returned value is object.
 *
 * Use Py_XDECREF to decrement reference count.
 */
    static char_u *
StringToChars(PyObject *object, PyObject **todecref)
{
    char_u	*p;
    PyObject	*bytes = NULL;

    if (PyBytes_Check(object))
    {

	if (PyString_AsStringAndSize(object, (char **) &p, NULL) == -1)
	    return NULL;
	if (p == NULL)
	    return NULL;

	*todecref = NULL;
    }
    else if (PyUnicode_Check(object))
    {
	bytes = PyUnicode_AsEncodedString(object, (char *)ENC_OPT, NULL);
	if (bytes == NULL)
	    return NULL;

	if(PyString_AsStringAndSize(bytes, (char **) &p, NULL) == -1)
	    return NULL;
	if (p == NULL)
	    return NULL;

	*todecref = bytes;
    }
    else
    {
	PyErr_SetString(PyExc_TypeError, _("object must be string"));
	return NULL;
    }

    return (char_u *) p;
}

    static int
add_string(PyObject *list, char *s)
{
    PyObject	*string;

    if (!(string = PyString_FromString(s)))
	return -1;
    if (PyList_Append(list, string))
    {
	Py_DECREF(string);
	return -1;
    }

    Py_DECREF(string);
    return 0;
}

    static PyObject *
ObjectDir(PyObject *self, char **attributes)
{
    PyMethodDef	*method;
    char	**attr;
    PyObject	*r;

    if (!(r = PyList_New(0)))
	return NULL;

    if (self)
	for (method = self->ob_type->tp_methods ; method->ml_name != NULL ; ++method)
	    if (add_string(r, (char *) method->ml_name))
	    {
		Py_DECREF(r);
		return NULL;
	    }

    for (attr = attributes ; *attr ; ++attr)
	if (add_string(r, *attr))
	{
	    Py_DECREF(r);
	    return NULL;
	}

#if PY_MAJOR_VERSION < 3
    if (add_string(r, "__members__"))
    {
	Py_DECREF(r);
	return NULL;
    }
#endif

    return r;
}

/* Output buffer management
 */

/* Function to write a line, points to either msg() or emsg(). */
typedef void (*writefn)(char_u *);

static PyTypeObject OutputType;

typedef struct
{
    PyObject_HEAD
    long softspace;
    long error;
} OutputObject;

static char *OutputAttrs[] = {
    "softspace",
    NULL
};

    static PyObject *
OutputDir(PyObject *self)
{
    return ObjectDir(self, OutputAttrs);
}

    static int
OutputSetattr(OutputObject *self, char *name, PyObject *val)
{
    if (val == NULL)
    {
	PyErr_SetString(PyExc_AttributeError,
		_("can't delete OutputObject attributes"));
	return -1;
    }

    if (strcmp(name, "softspace") == 0)
    {
	if (!PyInt_Check(val))
	{
	    PyErr_SetString(PyExc_TypeError, _("softspace must be an integer"));
	    return -1;
	}

	self->softspace = PyInt_AsLong(val);
	return 0;
    }

    PyErr_SetString(PyExc_AttributeError, _("invalid attribute"));
    return -1;
}

/* Buffer IO, we write one whole line at a time. */
static garray_T io_ga = {0, 0, 1, 80, NULL};
static writefn old_fn = NULL;

    static void
PythonIO_Flush(void)
{
    if (old_fn != NULL && io_ga.ga_len > 0)
    {
	((char_u *)io_ga.ga_data)[io_ga.ga_len] = NUL;
	old_fn((char_u *)io_ga.ga_data);
    }
    io_ga.ga_len = 0;
}

    static void
writer(writefn fn, char_u *str, PyInt n)
{
    char_u *ptr;

    /* Flush when switching output function. */
    if (fn != old_fn)
	PythonIO_Flush();
    old_fn = fn;

    /* Write each NL separated line.  Text after the last NL is kept for
     * writing later. */
    while (n > 0 && (ptr = memchr(str, '\n', n)) != NULL)
    {
	PyInt len = ptr - str;

	if (ga_grow(&io_ga, (int)(len + 1)) == FAIL)
	    break;

	mch_memmove(((char *)io_ga.ga_data) + io_ga.ga_len, str, (size_t)len);
	((char *)io_ga.ga_data)[io_ga.ga_len + len] = NUL;
	fn((char_u *)io_ga.ga_data);
	str = ptr + 1;
	n -= len + 1;
	io_ga.ga_len = 0;
    }

    /* Put the remaining text into io_ga for later printing. */
    if (n > 0 && ga_grow(&io_ga, (int)(n + 1)) == OK)
    {
	mch_memmove(((char *)io_ga.ga_data) + io_ga.ga_len, str, (size_t)n);
	io_ga.ga_len += (int)n;
    }
}

    static PyObject *
OutputWrite(OutputObject *self, PyObject *args)
{
    Py_ssize_t len = 0;
    char *str = NULL;
    int error = self->error;

    if (!PyArg_ParseTuple(args, "et#", ENC_OPT, &str, &len))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    writer((writefn)(error ? emsg : msg), (char_u *)str, len);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS
    PyMem_Free(str);

    Py_INCREF(Py_None);
    return Py_None;
}

    static PyObject *
OutputWritelines(OutputObject *self, PyObject *args)
{
    PyObject	*seq;
    PyObject	*iterator;
    PyObject	*item;
    int error = self->error;

    if (!PyArg_ParseTuple(args, "O", &seq))
	return NULL;

    if (!(iterator = PyObject_GetIter(seq)))
	return NULL;

    while ((item = PyIter_Next(iterator)))
    {
	char *str = NULL;
	PyInt len;

	if (!PyArg_Parse(item, "et#", ENC_OPT, &str, &len))
	{
	    PyErr_SetString(PyExc_TypeError, _("writelines() requires list of strings"));
	    Py_DECREF(iterator);
	    Py_DECREF(item);
	    return NULL;
	}
	Py_DECREF(item);

	Py_BEGIN_ALLOW_THREADS
	Python_Lock_Vim();
	writer((writefn)(error ? emsg : msg), (char_u *)str, len);
	Python_Release_Vim();
	Py_END_ALLOW_THREADS
	PyMem_Free(str);
    }

    Py_DECREF(iterator);

    /* Iterator may have finished due to an exception */
    if (PyErr_Occurred())
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

    static PyObject *
OutputFlush(PyObject *self UNUSED)
{
    /* do nothing */
    Py_INCREF(Py_None);
    return Py_None;
}

/***************/

static struct PyMethodDef OutputMethods[] = {
    /* name,	    function,				calling,	doc */
    {"write",	    (PyCFunction)OutputWrite,		METH_VARARGS,	""},
    {"writelines",  (PyCFunction)OutputWritelines,	METH_VARARGS,	""},
    {"flush",	    (PyCFunction)OutputFlush,		METH_NOARGS,	""},
    {"__dir__",	    (PyCFunction)OutputDir,		METH_NOARGS,	""},
    { NULL,	    NULL,				0,		NULL}
};

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

    static int
PythonIO_Init_io(void)
{
    if (PySys_SetObject("stdout", (PyObject *)(void *)&Output))
	return -1;
    if (PySys_SetObject("stderr", (PyObject *)(void *)&Error))
	return -1;

    if (PyErr_Occurred())
    {
	EMSG(_("E264: Python: Error initialising I/O objects"));
	return -1;
    }

    return 0;
}


static PyObject *VimError;

/* Check to see whether a Vim error has been reported, or a keyboard
 * interrupt has been detected.
 */

    static void
VimTryStart(void)
{
    ++trylevel;
}

    static int
VimTryEnd(void)
{
    --trylevel;
    if (got_int)
    {
	PyErr_SetNone(PyExc_KeyboardInterrupt);
	return 1;
    }
    else if (!did_throw)
	return 0;
    else if (PyErr_Occurred())
	return 1;
    else
    {
	PyErr_SetVim((char *) current_exception->value);
	discard_current_exception();
	return 1;
    }
}

    static int
VimCheckInterrupt(void)
{
    if (got_int)
    {
	PyErr_SetNone(PyExc_KeyboardInterrupt);
	return 1;
    }
    return 0;
}

/* Vim module - Implementation
 */

    static PyObject *
VimCommand(PyObject *self UNUSED, PyObject *args)
{
    char *cmd;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s", &cmd))
	return NULL;

    PyErr_Clear();

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();

    VimTryStart();
    do_cmdline_cmd((char_u *)cmd);
    update_screen(VALID);

    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (VimTryEnd())
	result = NULL;
    else
	result = Py_None;


    Py_XINCREF(result);
    return result;
}

/*
 * Function to translate a typval_T into a PyObject; this will recursively
 * translate lists/dictionaries into their Python equivalents.
 *
 * The depth parameter is to avoid infinite recursion, set it to 1 when
 * you call VimToPython.
 */
    static PyObject *
VimToPython(typval_T *our_tv, int depth, PyObject *lookup_dict)
{
    PyObject	*result;
    PyObject	*newObj;
    char	ptrBuf[sizeof(void *) * 2 + 3];

    /* Avoid infinite recursion */
    if (depth > 100)
    {
	Py_INCREF(Py_None);
	result = Py_None;
	return result;
    }

    /* Check if we run into a recursive loop.  The item must be in lookup_dict
     * then and we can use it again. */
    if ((our_tv->v_type == VAR_LIST && our_tv->vval.v_list != NULL)
	    || (our_tv->v_type == VAR_DICT && our_tv->vval.v_dict != NULL))
    {
	sprintf(ptrBuf, "%p",
		our_tv->v_type == VAR_LIST ? (void *)our_tv->vval.v_list
					   : (void *)our_tv->vval.v_dict);

	if ((result = PyDict_GetItemString(lookup_dict, ptrBuf)))
	{
	    Py_INCREF(result);
	    return result;
	}
    }

    if (our_tv->v_type == VAR_STRING)
    {
	result = PyString_FromString(our_tv->vval.v_string == NULL
					? "" : (char *)our_tv->vval.v_string);
    }
    else if (our_tv->v_type == VAR_NUMBER)
    {
	char buf[NUMBUFLEN];

	/* For backwards compatibility numbers are stored as strings. */
	sprintf(buf, "%ld", (long)our_tv->vval.v_number);
	result = PyString_FromString((char *) buf);
    }
# ifdef FEAT_FLOAT
    else if (our_tv->v_type == VAR_FLOAT)
    {
	char buf[NUMBUFLEN];

	sprintf(buf, "%f", our_tv->vval.v_float);
	result = PyString_FromString((char *) buf);
    }
# endif
    else if (our_tv->v_type == VAR_LIST)
    {
	list_T		*list = our_tv->vval.v_list;
	listitem_T	*curr;

	if (list == NULL)
	    return NULL;

	if (!(result = PyList_New(0)))
	    return NULL;

	if (PyDict_SetItemString(lookup_dict, ptrBuf, result))
	{
	    Py_DECREF(result);
	    return NULL;
	}

	for (curr = list->lv_first; curr != NULL; curr = curr->li_next)
	{
	    if (!(newObj = VimToPython(&curr->li_tv, depth + 1, lookup_dict)))
	    {
		Py_DECREF(result);
		return NULL;
	    }
	    if (PyList_Append(result, newObj))
	    {
		Py_DECREF(newObj);
		Py_DECREF(result);
		return NULL;
	    }
	    Py_DECREF(newObj);
	}
    }
    else if (our_tv->v_type == VAR_DICT)
    {

	hashtab_T	*ht = &our_tv->vval.v_dict->dv_hashtab;
	long_u	todo = ht->ht_used;
	hashitem_T	*hi;
	dictitem_T	*di;
	if (our_tv->vval.v_dict == NULL)
	    return NULL;

	if (!(result = PyDict_New()))
	    return NULL;

	if (PyDict_SetItemString(lookup_dict, ptrBuf, result))
	{
	    Py_DECREF(result);
	    return NULL;
	}

	for (hi = ht->ht_array; todo > 0; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;

		di = dict_lookup(hi);
		if (!(newObj = VimToPython(&di->di_tv, depth + 1, lookup_dict)))
		{
		    Py_DECREF(result);
		    return NULL;
		}
		if (PyDict_SetItemString(result, (char *)hi->hi_key, newObj))
		{
		    Py_DECREF(result);
		    Py_DECREF(newObj);
		    return NULL;
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

    static PyObject *
VimEval(PyObject *self UNUSED, PyObject *args)
{
    char	*expr;
    typval_T	*our_tv;
    PyObject	*result;
    PyObject    *lookup_dict;

    if (!PyArg_ParseTuple(args, "s", &expr))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    VimTryStart();
    our_tv = eval_expr((char_u *)expr, NULL);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (VimTryEnd())
	return NULL;

    if (our_tv == NULL)
    {
	PyErr_SetVim(_("invalid expression"));
	return NULL;
    }

    /* Convert the Vim type into a Python type.  Create a dictionary that's
     * used to check for recursive loops. */
    if (!(lookup_dict = PyDict_New()))
	result = NULL;
    else
    {
	result = VimToPython(our_tv, 1, lookup_dict);
	Py_DECREF(lookup_dict);
    }


    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    free_tv(our_tv);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *ConvertToPyObject(typval_T *);

    static PyObject *
VimEvalPy(PyObject *self UNUSED, PyObject *args)
{
    char	*expr;
    typval_T	*our_tv;
    PyObject	*result;

    if (!PyArg_ParseTuple(args, "s", &expr))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    VimTryStart();
    our_tv = eval_expr((char_u *)expr, NULL);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (VimTryEnd())
	return NULL;

    if (our_tv == NULL)
    {
	PyErr_SetVim(_("invalid expression"));
	return NULL;
    }

    result = ConvertToPyObject(our_tv);
    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    free_tv(our_tv);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    return result;
}

    static PyObject *
VimStrwidth(PyObject *self UNUSED, PyObject *args)
{
    char	*expr;

    if (!PyArg_ParseTuple(args, "s", &expr))
	return NULL;

    return PyLong_FromLong(
#ifdef FEAT_MBYTE
	    mb_string2cells((char_u *)expr, (int)STRLEN(expr))
#else
	    STRLEN(expr)
#endif
	    );
}

    static PyObject *
_VimChdir(PyObject *_chdir, PyObject *args, PyObject *kwargs)
{
    PyObject	*r;
    PyObject	*newwd;
    PyObject	*todecref;
    char_u	*new_dir;

    if (_chdir == NULL)
	return NULL;
    if (!(r = PyObject_Call(_chdir, args, kwargs)))
	return NULL;

    if (!(newwd = PyObject_CallFunctionObjArgs(py_getcwd, NULL)))
    {
	Py_DECREF(r);
	return NULL;
    }

    if (!(new_dir = StringToChars(newwd, &todecref)))
    {
	Py_DECREF(r);
	Py_DECREF(newwd);
	return NULL;
    }

    VimTryStart();

    if (vim_chdir(new_dir))
    {
	Py_DECREF(r);
	Py_DECREF(newwd);
	Py_XDECREF(todecref);

	if (VimTryEnd())
	    return NULL;

	PyErr_SetVim(_("failed to change directory"));
	return NULL;
    }

    Py_DECREF(newwd);
    Py_XDECREF(todecref);

    post_chdir(FALSE);

    if (VimTryEnd())
    {
	Py_DECREF(r);
	return NULL;
    }

    return r;
}

    static PyObject *
VimChdir(PyObject *self UNUSED, PyObject *args, PyObject *kwargs)
{
    return _VimChdir(py_chdir, args, kwargs);
}

    static PyObject *
VimFchdir(PyObject *self UNUSED, PyObject *args, PyObject *kwargs)
{
    return _VimChdir(py_fchdir, args, kwargs);
}

typedef struct {
    PyObject	*callable;
    PyObject	*result;
} map_rtp_data;

    static void
map_rtp_callback(char_u *path, void *_data)
{
    void	**data = (void **) _data;
    PyObject	*pathObject;
    map_rtp_data	*mr_data = *((map_rtp_data **) data);

    if (!(pathObject = PyString_FromString((char *) path)))
    {
	*data = NULL;
	return;
    }

    mr_data->result = PyObject_CallFunctionObjArgs(mr_data->callable,
						   pathObject, NULL);

    Py_DECREF(pathObject);

    if (!mr_data->result || mr_data->result != Py_None)
	*data = NULL;
    else
    {
	Py_DECREF(mr_data->result);
	mr_data->result = NULL;
    }
}

    static PyObject *
VimForeachRTP(PyObject *self UNUSED, PyObject *args)
{
    map_rtp_data	data;

    if (!PyArg_ParseTuple(args, "O", &data.callable))
	return NULL;

    data.result = NULL;

    do_in_runtimepath(NULL, FALSE, &map_rtp_callback, &data);

    if (data.result == NULL)
    {
	if (PyErr_Occurred())
	    return NULL;
	else
	{
	    Py_INCREF(Py_None);
	    return Py_None;
	}
    }
    return data.result;
}

/*
 * _vim_runtimepath_ special path implementation.
 */

    static void
map_finder_callback(char_u *path, void *_data)
{
    void	**data = (void **) _data;
    PyObject	*list = *((PyObject **) data);
    PyObject	*pathObject1, *pathObject2;
    char	*pathbuf;
    size_t	pathlen;

    pathlen = STRLEN(path);

#if PY_MAJOR_VERSION < 3
# define PY_MAIN_DIR_STRING "python2"
#else
# define PY_MAIN_DIR_STRING "python3"
#endif
#define PY_ALTERNATE_DIR_STRING "pythonx"

#define PYTHONX_STRING_LENGTH 7 /* STRLEN("pythonx") */
    if (!(pathbuf = PyMem_New(char,
		    pathlen + STRLEN(PATHSEPSTR) + PYTHONX_STRING_LENGTH + 1)))
    {
	PyErr_NoMemory();
	*data = NULL;
	return;
    }

    mch_memmove(pathbuf, path, pathlen + 1);
    add_pathsep((char_u *) pathbuf);

    pathlen = STRLEN(pathbuf);
    mch_memmove(pathbuf + pathlen, PY_MAIN_DIR_STRING,
	    PYTHONX_STRING_LENGTH + 1);

    if (!(pathObject1 = PyString_FromString(pathbuf)))
    {
	*data = NULL;
	PyMem_Free(pathbuf);
	return;
    }

    mch_memmove(pathbuf + pathlen, PY_ALTERNATE_DIR_STRING,
	    PYTHONX_STRING_LENGTH + 1);

    if (!(pathObject2 = PyString_FromString(pathbuf)))
    {
	Py_DECREF(pathObject1);
	PyMem_Free(pathbuf);
	*data = NULL;
	return;
    }

    PyMem_Free(pathbuf);

    if (PyList_Append(list, pathObject1)
	    || PyList_Append(list, pathObject2))
	*data = NULL;

    Py_DECREF(pathObject1);
    Py_DECREF(pathObject2);
}

    static PyObject *
Vim_GetPaths(PyObject *self UNUSED)
{
    PyObject	*r;

    if (!(r = PyList_New(0)))
	return NULL;

    do_in_runtimepath(NULL, FALSE, &map_finder_callback, r);

    if (PyErr_Occurred())
    {
	Py_DECREF(r);
	return NULL;
    }

    return r;
}

/*
 * Vim module - Definitions
 */

static struct PyMethodDef VimMethods[] = {
    /* name,	    function,			calling,			documentation */
    {"command",	    VimCommand,			METH_VARARGS,			"Execute a Vim ex-mode command" },
    {"eval",	    VimEval,			METH_VARARGS,			"Evaluate an expression using Vim evaluator" },
    {"bindeval",    VimEvalPy,			METH_VARARGS,			"Like eval(), but returns objects attached to vim ones"},
    {"strwidth",    VimStrwidth,		METH_VARARGS,			"Screen string width, counts <Tab> as having width 1"},
    {"chdir",	    (PyCFunction)VimChdir,	METH_VARARGS|METH_KEYWORDS,	"Change directory"},
    {"fchdir",	    (PyCFunction)VimFchdir,	METH_VARARGS|METH_KEYWORDS,	"Change directory"},
    {"foreach_rtp", VimForeachRTP,		METH_VARARGS,			"Call given callable for each path in &rtp"},
#if PY_MAJOR_VERSION < 3
    {"find_module", FinderFindModule,		METH_VARARGS,			"Internal use only, returns loader object for any input it receives"},
    {"load_module", LoaderLoadModule,		METH_VARARGS,			"Internal use only, tries importing the given module from &rtp by temporary mocking sys.path (to an rtp-based one) and unsetting sys.meta_path and sys.path_hooks"},
#endif
    {"path_hook",   VimPathHook,		METH_VARARGS,			"Hook function to install in sys.path_hooks"},
    {"_get_paths",  (PyCFunction)Vim_GetPaths,	METH_NOARGS,			"Get &rtp-based additions to sys.path"},
    { NULL,	    NULL,			0,				NULL}
};

/*
 * Generic iterator object
 */

static PyTypeObject IterType;

typedef PyObject *(*nextfun)(void **);
typedef void (*destructorfun)(void *);
typedef int (*traversefun)(void *, visitproc, void *);
typedef int (*clearfun)(void **);

/* Main purpose of this object is removing the need for do python
 * initialization (i.e. PyType_Ready and setting type attributes) for a big
 * bunch of objects. */

typedef struct
{
    PyObject_HEAD
    void *cur;
    nextfun next;
    destructorfun destruct;
    traversefun traverse;
    clearfun clear;
} IterObject;

    static PyObject *
IterNew(void *start, destructorfun destruct, nextfun next, traversefun traverse,
	clearfun clear)
{
    IterObject *self;

    self = PyObject_GC_New(IterObject, &IterType);
    self->cur = start;
    self->next = next;
    self->destruct = destruct;
    self->traverse = traverse;
    self->clear = clear;

    return (PyObject *)(self);
}

    static void
IterDestructor(IterObject *self)
{
    PyObject_GC_UnTrack((void *)(self));
    self->destruct(self->cur);
    PyObject_GC_Del((void *)(self));
}

    static int
IterTraverse(IterObject *self, visitproc visit, void *arg)
{
    if (self->traverse != NULL)
	return self->traverse(self->cur, visit, arg);
    else
	return 0;
}

/* Mac OSX defines clear() somewhere. */
#ifdef clear
# undef clear
#endif

    static int
IterClear(IterObject *self)
{
    if (self->clear != NULL)
	return self->clear(&self->cur);
    else
	return 0;
}

    static PyObject *
IterNext(IterObject *self)
{
    return self->next(&self->cur);
}

    static PyObject *
IterIter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

typedef struct pylinkedlist_S {
    struct pylinkedlist_S	*pll_next;
    struct pylinkedlist_S	*pll_prev;
    PyObject			*pll_obj;
} pylinkedlist_T;

static pylinkedlist_T *lastdict = NULL;
static pylinkedlist_T *lastlist = NULL;

    static void
pyll_remove(pylinkedlist_T *ref, pylinkedlist_T **last)
{
    if (ref->pll_prev == NULL)
    {
	if (ref->pll_next == NULL)
	{
	    *last = NULL;
	    return;
	}
    }
    else
	ref->pll_prev->pll_next = ref->pll_next;

    if (ref->pll_next == NULL)
	*last = ref->pll_prev;
    else
	ref->pll_next->pll_prev = ref->pll_prev;
}

    static void
pyll_add(PyObject *self, pylinkedlist_T *ref, pylinkedlist_T **last)
{
    if (*last == NULL)
	ref->pll_prev = NULL;
    else
    {
	(*last)->pll_next = ref;
	ref->pll_prev = *last;
    }
    ref->pll_next = NULL;
    ref->pll_obj = self;
    *last = ref;
}

static PyTypeObject DictionaryType;

typedef struct
{
    PyObject_HEAD
    dict_T	*dict;
    pylinkedlist_T	ref;
} DictionaryObject;

static PyObject *DictionaryUpdate(DictionaryObject *, PyObject *, PyObject *);

#define NEW_DICTIONARY(dict) DictionaryNew(&DictionaryType, dict)

    static PyObject *
DictionaryNew(PyTypeObject *subtype, dict_T *dict)
{
    DictionaryObject	*self;

    self = (DictionaryObject *) subtype->tp_alloc(subtype, 0);
    if (self == NULL)
	return NULL;
    self->dict = dict;
    ++dict->dv_refcount;

    pyll_add((PyObject *)(self), &self->ref, &lastdict);

    return (PyObject *)(self);
}

    static dict_T *
py_dict_alloc()
{
    dict_T	*r;

    if (!(r = dict_alloc()))
    {
	PyErr_NoMemory();
	return NULL;
    }
    ++r->dv_refcount;

    return r;
}

    static PyObject *
DictionaryConstructor(PyTypeObject *subtype, PyObject *args, PyObject *kwargs)
{
    DictionaryObject	*self;
    dict_T	*dict;

    if (!(dict = py_dict_alloc()))
	return NULL;

    self = (DictionaryObject *) DictionaryNew(subtype, dict);

    --dict->dv_refcount;

    if (kwargs || PyTuple_Size(args))
    {
	PyObject	*tmp;
	if (!(tmp = DictionaryUpdate(self, args, kwargs)))
	{
	    Py_DECREF(self);
	    return NULL;
	}

	Py_DECREF(tmp);
    }

    return (PyObject *)(self);
}

    static void
DictionaryDestructor(DictionaryObject *self)
{
    pyll_remove(&self->ref, &lastdict);
    dict_unref(self->dict);

    DESTRUCTOR_FINISH(self);
}

static char *DictionaryAttrs[] = {
    "locked", "scope",
    NULL
};

    static PyObject *
DictionaryDir(PyObject *self)
{
    return ObjectDir(self, DictionaryAttrs);
}

    static int
DictionarySetattr(DictionaryObject *self, char *name, PyObject *val)
{
    if (val == NULL)
    {
	PyErr_SetString(PyExc_AttributeError,
		_("cannot delete vim.Dictionary attributes"));
	return -1;
    }

    if (strcmp(name, "locked") == 0)
    {
	if (self->dict->dv_lock == VAR_FIXED)
	{
	    PyErr_SetString(PyExc_TypeError, _("cannot modify fixed dictionary"));
	    return -1;
	}
	else
	{
	    int		istrue = PyObject_IsTrue(val);
	    if (istrue == -1)
		return -1;
	    else if (istrue)
		self->dict->dv_lock = VAR_LOCKED;
	    else
		self->dict->dv_lock = 0;
	}
	return 0;
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, _("cannot set this attribute"));
	return -1;
    }
}

    static PyInt
DictionaryLength(DictionaryObject *self)
{
    return ((PyInt) (self->dict->dv_hashtab.ht_used));
}

#define DICT_FLAG_HAS_DEFAULT	0x01
#define DICT_FLAG_POP		0x02
#define DICT_FLAG_NONE_DEFAULT	0x04
#define DICT_FLAG_RETURN_BOOL	0x08 /* Incompatible with DICT_FLAG_POP */
#define DICT_FLAG_RETURN_PAIR	0x10

    static PyObject *
_DictionaryItem(DictionaryObject *self, PyObject *args, int flags)
{
    PyObject	*keyObject;
    PyObject	*defObject = ((flags & DICT_FLAG_NONE_DEFAULT)? Py_None : NULL);
    PyObject	*r;
    char_u	*key;
    dictitem_T	*di;
    dict_T	*dict = self->dict;
    hashitem_T	*hi;
    PyObject	*todecref;

    if (flags & DICT_FLAG_HAS_DEFAULT)
    {
	if (!PyArg_ParseTuple(args, "O|O", &keyObject, &defObject))
	    return NULL;
    }
    else
	keyObject = args;

    if (flags & DICT_FLAG_RETURN_BOOL)
	defObject = Py_False;

    if (!(key = StringToChars(keyObject, &todecref)))
	return NULL;

    if (*key == NUL)
    {
	RAISE_NO_EMPTY_KEYS;
	Py_XDECREF(todecref);
	return NULL;
    }

    hi = hash_find(&dict->dv_hashtab, key);

    Py_XDECREF(todecref);

    if (HASHITEM_EMPTY(hi))
    {
	if (defObject)
	{
	    Py_INCREF(defObject);
	    return defObject;
	}
	else
	{
	    PyErr_SetObject(PyExc_KeyError, keyObject);
	    return NULL;
	}
    }
    else if (flags & DICT_FLAG_RETURN_BOOL)
    {
	Py_INCREF(Py_True);
	return Py_True;
    }

    di = dict_lookup(hi);

    if (!(r = ConvertToPyObject(&di->di_tv)))
	return NULL;

    if (flags & DICT_FLAG_POP)
    {
	if (dict->dv_lock)
	{
	    PyErr_SetVim(_("dict is locked"));
	    Py_DECREF(r);
	    return NULL;
	}

	hash_remove(&dict->dv_hashtab, hi);
	dictitem_free(di);
    }

    return r;
}

    static PyObject *
DictionaryItem(DictionaryObject *self, PyObject *keyObject)
{
    return _DictionaryItem(self, keyObject, 0);
}

    static int
DictionaryContains(DictionaryObject *self, PyObject *keyObject)
{
    PyObject	*rObj = _DictionaryItem(self, keyObject, DICT_FLAG_RETURN_BOOL);
    int		r;

    r = (rObj == Py_True);

    Py_DECREF(Py_True);

    return r;
}

typedef struct
{
    hashitem_T	*ht_array;
    long_u	ht_used;
    hashtab_T	*ht;
    hashitem_T	*hi;
    long_u	todo;
} dictiterinfo_T;

    static PyObject *
DictionaryIterNext(dictiterinfo_T **dii)
{
    PyObject	*r;

    if (!(*dii)->todo)
	return NULL;

    if ((*dii)->ht->ht_array != (*dii)->ht_array ||
	    (*dii)->ht->ht_used != (*dii)->ht_used)
    {
	PyErr_SetString(PyExc_RuntimeError,
		_("hashtab changed during iteration"));
	return NULL;
    }

    while (((*dii)->todo) && HASHITEM_EMPTY((*dii)->hi))
	++((*dii)->hi);

    --((*dii)->todo);

    if (!(r = PyBytes_FromString((char *) (*dii)->hi->hi_key)))
	return NULL;

    return r;
}

    static PyObject *
DictionaryIter(DictionaryObject *self)
{
    dictiterinfo_T	*dii;
    hashtab_T		*ht;

    if (!(dii = PyMem_New(dictiterinfo_T, 1)))
    {
	PyErr_NoMemory();
	return NULL;
    }

    ht = &self->dict->dv_hashtab;
    dii->ht_array = ht->ht_array;
    dii->ht_used = ht->ht_used;
    dii->ht = ht;
    dii->hi = dii->ht_array;
    dii->todo = dii->ht_used;

    return IterNew(dii,
	    (destructorfun) PyMem_Free, (nextfun) DictionaryIterNext,
	    NULL, NULL);
}

    static PyInt
DictionaryAssItem(DictionaryObject *self, PyObject *keyObject, PyObject *valObject)
{
    char_u	*key;
    typval_T	tv;
    dict_T	*dict = self->dict;
    dictitem_T	*di;
    PyObject	*todecref;

    if (dict->dv_lock)
    {
	PyErr_SetVim(_("dict is locked"));
	return -1;
    }

    if (!(key = StringToChars(keyObject, &todecref)))
	return -1;

    if (*key == NUL)
    {
	RAISE_NO_EMPTY_KEYS;
	Py_XDECREF(todecref);
	return -1;
    }

    di = dict_find(dict, key, -1);

    if (valObject == NULL)
    {
	hashitem_T	*hi;

	if (di == NULL)
	{
	    Py_XDECREF(todecref);
	    PyErr_SetObject(PyExc_KeyError, keyObject);
	    return -1;
	}
	hi = hash_find(&dict->dv_hashtab, di->di_key);
	hash_remove(&dict->dv_hashtab, hi);
	dictitem_free(di);
	Py_XDECREF(todecref);
	return 0;
    }

    if (ConvertFromPyObject(valObject, &tv) == -1)
    {
	Py_XDECREF(todecref);
	return -1;
    }

    if (di == NULL)
    {
	if (!(di = dictitem_alloc(key)))
	{
	    Py_XDECREF(todecref);
	    PyErr_NoMemory();
	    return -1;
	}
	di->di_tv.v_lock = 0;
	di->di_tv.v_type = VAR_UNKNOWN;

	if (dict_add(dict, di) == FAIL)
	{
	    Py_XDECREF(todecref);
	    vim_free(di);
	    dictitem_free(di);
	    PyErr_SetVim(_("failed to add key to dictionary"));
	    return -1;
	}
    }
    else
	clear_tv(&di->di_tv);

    Py_XDECREF(todecref);

    copy_tv(&tv, &di->di_tv);
    clear_tv(&tv);
    return 0;
}

typedef PyObject *(*hi_to_py)(hashitem_T *);

    static PyObject *
DictionaryListObjects(DictionaryObject *self, hi_to_py hiconvert)
{
    dict_T	*dict = self->dict;
    long_u	todo = dict->dv_hashtab.ht_used;
    Py_ssize_t	i = 0;
    PyObject	*r;
    hashitem_T	*hi;
    PyObject	*newObj;

    r = PyList_New(todo);
    for (hi = dict->dv_hashtab.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    if (!(newObj = hiconvert(hi)))
	    {
		Py_DECREF(r);
		return NULL;
	    }
	    PyList_SET_ITEM(r, i, newObj);
	    --todo;
	    ++i;
	}
    }
    return r;
}

    static PyObject *
dict_key(hashitem_T *hi)
{
    return PyBytes_FromString((char *)(hi->hi_key));
}

    static PyObject *
DictionaryListKeys(DictionaryObject *self)
{
    return DictionaryListObjects(self, dict_key);
}

    static PyObject *
dict_val(hashitem_T *hi)
{
    dictitem_T	*di;

    di = dict_lookup(hi);
    return ConvertToPyObject(&di->di_tv);
}

    static PyObject *
DictionaryListValues(DictionaryObject *self)
{
    return DictionaryListObjects(self, dict_val);
}

    static PyObject *
dict_item(hashitem_T *hi)
{
    PyObject	*keyObject;
    PyObject	*valObject;
    PyObject	*r;

    if (!(keyObject = dict_key(hi)))
	return NULL;

    if (!(valObject = dict_val(hi)))
    {
	Py_DECREF(keyObject);
	return NULL;
    }

    r = Py_BuildValue("(OO)", keyObject, valObject);

    Py_DECREF(keyObject);
    Py_DECREF(valObject);

    return r;
}

    static PyObject *
DictionaryListItems(DictionaryObject *self)
{
    return DictionaryListObjects(self, dict_item);
}

    static PyObject *
DictionaryUpdate(DictionaryObject *self, PyObject *args, PyObject *kwargs)
{
    dict_T	*dict = self->dict;

    if (dict->dv_lock)
    {
	PyErr_SetVim(_("dict is locked"));
	return NULL;
    }

    if (kwargs)
    {
	typval_T	tv;

	if (ConvertFromPyMapping(kwargs, &tv) == -1)
	    return NULL;

	VimTryStart();
	dict_extend(self->dict, tv.vval.v_dict, (char_u *) "force");
	clear_tv(&tv);
	if (VimTryEnd())
	    return NULL;
    }
    else
    {
	PyObject	*object;

	if (!PyArg_Parse(args, "(O)", &object))
	    return NULL;

	if (PyObject_HasAttrString(object, "keys"))
	    return DictionaryUpdate(self, NULL, object);
	else
	{
	    PyObject	*iterator;
	    PyObject	*item;

	    if (!(iterator = PyObject_GetIter(object)))
		return NULL;

	    while ((item = PyIter_Next(iterator)))
	    {
		PyObject	*fast;
		PyObject	*keyObject;
		PyObject	*valObject;
		PyObject	*todecref;
		char_u		*key;
		dictitem_T	*di;

		if (!(fast = PySequence_Fast(item, "")))
		{
		    Py_DECREF(iterator);
		    Py_DECREF(item);
		    return NULL;
		}

		Py_DECREF(item);

		if (PySequence_Fast_GET_SIZE(fast) != 2)
		{
		    Py_DECREF(iterator);
		    Py_DECREF(fast);
		    PyErr_SetString(PyExc_ValueError,
			    _("expected sequence element of size 2"));
		    return NULL;
		}

		keyObject = PySequence_Fast_GET_ITEM(fast, 0);

		if (!(key = StringToChars(keyObject, &todecref)))
		{
		    Py_DECREF(iterator);
		    Py_DECREF(fast);
		    return NULL;
		}

		di = dictitem_alloc(key);

		Py_XDECREF(todecref);

		if (di == NULL)
		{
		    Py_DECREF(fast);
		    Py_DECREF(iterator);
		    PyErr_NoMemory();
		    return NULL;
		}
		di->di_tv.v_lock = 0;
		di->di_tv.v_type = VAR_UNKNOWN;

		valObject = PySequence_Fast_GET_ITEM(fast, 1);

		if (ConvertFromPyObject(valObject, &di->di_tv) == -1)
		{
		    Py_DECREF(iterator);
		    Py_DECREF(fast);
		    dictitem_free(di);
		    return NULL;
		}

		Py_DECREF(fast);

		if (dict_add(dict, di) == FAIL)
		{
		    Py_DECREF(iterator);
		    dictitem_free(di);
		    PyErr_SetVim(_("failed to add key to dictionary"));
		    return NULL;
		}
	    }

	    Py_DECREF(iterator);

	    /* Iterator may have finished due to an exception */
	    if (PyErr_Occurred())
		return NULL;
	}
    }
    Py_INCREF(Py_None);
    return Py_None;
}

    static PyObject *
DictionaryGet(DictionaryObject *self, PyObject *args)
{
    return _DictionaryItem(self, args,
			    DICT_FLAG_HAS_DEFAULT|DICT_FLAG_NONE_DEFAULT);
}

    static PyObject *
DictionaryPop(DictionaryObject *self, PyObject *args)
{
    return _DictionaryItem(self, args, DICT_FLAG_HAS_DEFAULT|DICT_FLAG_POP);
}

    static PyObject *
DictionaryPopItem(DictionaryObject *self)
{
    hashitem_T	*hi;
    PyObject	*r;
    PyObject	*valObject;
    dictitem_T	*di;

    if (self->dict->dv_hashtab.ht_used == 0)
    {
	PyErr_SetNone(PyExc_KeyError);
	return NULL;
    }

    hi = self->dict->dv_hashtab.ht_array;
    while (HASHITEM_EMPTY(hi))
	++hi;

    di = dict_lookup(hi);

    if (!(valObject = ConvertToPyObject(&di->di_tv)))
	return NULL;

    if (!(r = Py_BuildValue("(" Py_bytes_fmt "O)", hi->hi_key, valObject)))
    {
	Py_DECREF(valObject);
	return NULL;
    }

    hash_remove(&self->dict->dv_hashtab, hi);
    dictitem_free(di);

    return r;
}

    static PyObject *
DictionaryHasKey(DictionaryObject *self, PyObject *args)
{
    PyObject	*keyObject;

    if (!PyArg_ParseTuple(args, "O", &keyObject))
	return NULL;

    return _DictionaryItem(self, keyObject, DICT_FLAG_RETURN_BOOL);
}

static PySequenceMethods DictionaryAsSeq = {
    0,					/* sq_length */
    0,					/* sq_concat */
    0,					/* sq_repeat */
    0,					/* sq_item */
    0,					/* sq_slice */
    0,					/* sq_ass_item */
    0,					/* sq_ass_slice */
    (objobjproc) DictionaryContains,	/* sq_contains */
    0,					/* sq_inplace_concat */
    0,					/* sq_inplace_repeat */
};

static PyMappingMethods DictionaryAsMapping = {
    (lenfunc)       DictionaryLength,
    (binaryfunc)    DictionaryItem,
    (objobjargproc) DictionaryAssItem,
};

static struct PyMethodDef DictionaryMethods[] = {
    {"keys",	(PyCFunction)DictionaryListKeys,	METH_NOARGS,	""},
    {"values",	(PyCFunction)DictionaryListValues,	METH_NOARGS,	""},
    {"items",	(PyCFunction)DictionaryListItems,	METH_NOARGS,	""},
    {"update",	(PyCFunction)DictionaryUpdate,		METH_VARARGS|METH_KEYWORDS, ""},
    {"get",	(PyCFunction)DictionaryGet,		METH_VARARGS,	""},
    {"pop",	(PyCFunction)DictionaryPop,		METH_VARARGS,	""},
    {"popitem",	(PyCFunction)DictionaryPopItem,		METH_NOARGS,	""},
    {"has_key",	(PyCFunction)DictionaryHasKey,		METH_VARARGS,	""},
    {"__dir__",	(PyCFunction)DictionaryDir,		METH_NOARGS,	""},
    { NULL,	NULL,					0,		NULL}
};

static PyTypeObject ListType;
static PySequenceMethods ListAsSeq;
static PyMappingMethods ListAsMapping;

typedef struct
{
    PyObject_HEAD
    list_T	*list;
    pylinkedlist_T	ref;
} ListObject;

#define NEW_LIST(list) ListNew(&ListType, list)

    static PyObject *
ListNew(PyTypeObject *subtype, list_T *list)
{
    ListObject	*self;

    self = (ListObject *) subtype->tp_alloc(subtype, 0);
    if (self == NULL)
	return NULL;
    self->list = list;
    ++list->lv_refcount;

    pyll_add((PyObject *)(self), &self->ref, &lastlist);

    return (PyObject *)(self);
}

    static list_T *
py_list_alloc()
{
    list_T	*r;

    if (!(r = list_alloc()))
    {
	PyErr_NoMemory();
	return NULL;
    }
    ++r->lv_refcount;

    return r;
}

    static int
list_py_concat(list_T *l, PyObject *obj, PyObject *lookup_dict)
{
    PyObject	*iterator;
    PyObject	*item;
    listitem_T	*li;

    if (!(iterator = PyObject_GetIter(obj)))
	return -1;

    while ((item = PyIter_Next(iterator)))
    {
	if (!(li = listitem_alloc()))
	{
	    PyErr_NoMemory();
	    Py_DECREF(item);
	    Py_DECREF(iterator);
	    return -1;
	}
	li->li_tv.v_lock = 0;
	li->li_tv.v_type = VAR_UNKNOWN;

	if (_ConvertFromPyObject(item, &li->li_tv, lookup_dict) == -1)
	{
	    Py_DECREF(item);
	    Py_DECREF(iterator);
	    listitem_free(li);
	    return -1;
	}

	Py_DECREF(item);

	list_append(l, li);
    }

    Py_DECREF(iterator);

    /* Iterator may have finished due to an exception */
    if (PyErr_Occurred())
	return -1;

    return 0;
}

    static PyObject *
ListConstructor(PyTypeObject *subtype, PyObject *args, PyObject *kwargs)
{
    list_T	*list;
    PyObject	*obj = NULL;

    if (kwargs)
    {
	PyErr_SetString(PyExc_TypeError,
		_("list constructor does not accept keyword arguments"));
	return NULL;
    }

    if (!PyArg_ParseTuple(args, "|O", &obj))
	return NULL;

    if (!(list = py_list_alloc()))
	return NULL;

    if (obj)
    {
	PyObject	*lookup_dict;

	if (!(lookup_dict = PyDict_New()))
	{
	    list_unref(list);
	    return NULL;
	}

	if (list_py_concat(list, obj, lookup_dict) == -1)
	{
	    Py_DECREF(lookup_dict);
	    list_unref(list);
	    return NULL;
	}

	Py_DECREF(lookup_dict);
    }

    return ListNew(subtype, list);
}

    static void
ListDestructor(ListObject *self)
{
    pyll_remove(&self->ref, &lastlist);
    list_unref(self->list);

    DESTRUCTOR_FINISH(self);
}

    static PyInt
ListLength(ListObject *self)
{
    return ((PyInt) (self->list->lv_len));
}

    static PyObject *
ListItem(ListObject *self, Py_ssize_t index)
{
    listitem_T	*li;

    if (index >= ListLength(self))
    {
	PyErr_SetString(PyExc_IndexError, _("list index out of range"));
	return NULL;
    }
    li = list_find(self->list, (long) index);
    if (li == NULL)
    {
	PyErr_SetVim(_("internal error: failed to get vim list item"));
	return NULL;
    }
    return ConvertToPyObject(&li->li_tv);
}

#define PROC_RANGE \
    if (last < 0) {\
	if (last < -size) \
	    last = 0; \
	else \
	    last += size; \
    } \
    if (first < 0) \
	first = 0; \
    if (first > size) \
	first = size; \
    if (last > size) \
	last = size;

    static PyObject *
ListSlice(ListObject *self, Py_ssize_t first, Py_ssize_t last)
{
    PyInt	i;
    PyInt	size = ListLength(self);
    PyInt	n;
    PyObject	*list;
    int		reversed = 0;

    PROC_RANGE
    if (first >= last)
	first = last;

    n = last-first;
    list = PyList_New(n);
    if (list == NULL)
	return NULL;

    for (i = 0; i < n; ++i)
    {
	PyObject	*item = ListItem(self, first + i);
	if (item == NULL)
	{
	    Py_DECREF(list);
	    return NULL;
	}

	PyList_SET_ITEM(list, ((reversed)?(n-i-1):(i)), item);
    }

    return list;
}

typedef struct
{
    listwatch_T	lw;
    list_T	*list;
} listiterinfo_T;

    static void
ListIterDestruct(listiterinfo_T *lii)
{
    list_rem_watch(lii->list, &lii->lw);
    PyMem_Free(lii);
}

    static PyObject *
ListIterNext(listiterinfo_T **lii)
{
    PyObject	*r;

    if (!((*lii)->lw.lw_item))
	return NULL;

    if (!(r = ConvertToPyObject(&((*lii)->lw.lw_item->li_tv))))
	return NULL;

    (*lii)->lw.lw_item = (*lii)->lw.lw_item->li_next;

    return r;
}

    static PyObject *
ListIter(ListObject *self)
{
    listiterinfo_T	*lii;
    list_T	*l = self->list;

    if (!(lii = PyMem_New(listiterinfo_T, 1)))
    {
	PyErr_NoMemory();
	return NULL;
    }

    list_add_watch(l, &lii->lw);
    lii->lw.lw_item = l->lv_first;
    lii->list = l;

    return IterNew(lii,
	    (destructorfun) ListIterDestruct, (nextfun) ListIterNext,
	    NULL, NULL);
}

    static int
ListAssItem(ListObject *self, Py_ssize_t index, PyObject *obj)
{
    typval_T	tv;
    list_T	*l = self->list;
    listitem_T	*li;
    Py_ssize_t	length = ListLength(self);

    if (l->lv_lock)
    {
	PyErr_SetVim(_("list is locked"));
	return -1;
    }
    if (index>length || (index==length && obj==NULL))
    {
	PyErr_SetString(PyExc_IndexError, _("list index out of range"));
	return -1;
    }

    if (obj == NULL)
    {
	li = list_find(l, (long) index);
	list_remove(l, li, li);
	clear_tv(&li->li_tv);
	vim_free(li);
	return 0;
    }

    if (ConvertFromPyObject(obj, &tv) == -1)
	return -1;

    if (index == length)
    {
	if (list_append_tv(l, &tv) == FAIL)
	{
	    clear_tv(&tv);
	    PyErr_SetVim(_("failed to add item to list"));
	    return -1;
	}
    }
    else
    {
	li = list_find(l, (long) index);
	clear_tv(&li->li_tv);
	copy_tv(&tv, &li->li_tv);
	clear_tv(&tv);
    }
    return 0;
}

    static int
ListAssSlice(ListObject *self, Py_ssize_t first, Py_ssize_t last, PyObject *obj)
{
    PyInt	size = ListLength(self);
    PyObject	*iterator;
    PyObject	*item;
    listitem_T	*li;
    listitem_T	*next;
    typval_T	v;
    list_T	*l = self->list;
    PyInt	i;

    if (l->lv_lock)
    {
	PyErr_SetVim(_("list is locked"));
	return -1;
    }

    PROC_RANGE

    if (first == size)
	li = NULL;
    else
    {
	li = list_find(l, (long) first);
	if (li == NULL)
	{
	    PyErr_SetVim(_("internal error: no vim list item"));
	    return -1;
	}
	if (last > first)
	{
	    i = last - first;
	    while (i-- && li != NULL)
	    {
		next = li->li_next;
		listitem_remove(l, li);
		li = next;
	    }
	}
    }

    if (obj == NULL)
	return 0;

    if (!(iterator = PyObject_GetIter(obj)))
	return -1;

    while ((item = PyIter_Next(iterator)))
    {
	if (ConvertFromPyObject(item, &v) == -1)
	{
	    Py_DECREF(iterator);
	    Py_DECREF(item);
	    return -1;
	}
	Py_DECREF(item);
	if (list_insert_tv(l, &v, li) == FAIL)
	{
	    clear_tv(&v);
	    PyErr_SetVim(_("internal error: failed to add item to list"));
	    return -1;
	}
	clear_tv(&v);
    }
    Py_DECREF(iterator);
    return 0;
}

    static PyObject *
ListConcatInPlace(ListObject *self, PyObject *obj)
{
    list_T	*l = self->list;
    PyObject	*lookup_dict;

    if (l->lv_lock)
    {
	PyErr_SetVim(_("list is locked"));
	return NULL;
    }

    if (!(lookup_dict = PyDict_New()))
	return NULL;

    if (list_py_concat(l, obj, lookup_dict) == -1)
    {
	Py_DECREF(lookup_dict);
	return NULL;
    }
    Py_DECREF(lookup_dict);

    Py_INCREF(self);
    return (PyObject *)(self);
}

static char *ListAttrs[] = {
    "locked",
    NULL
};

    static PyObject *
ListDir(PyObject *self)
{
    return ObjectDir(self, ListAttrs);
}

    static int
ListSetattr(ListObject *self, char *name, PyObject *val)
{
    if (val == NULL)
    {
	PyErr_SetString(PyExc_AttributeError,
		_("cannot delete vim.List attributes"));
	return -1;
    }

    if (strcmp(name, "locked") == 0)
    {
	if (self->list->lv_lock == VAR_FIXED)
	{
	    PyErr_SetString(PyExc_TypeError, _("cannot modify fixed list"));
	    return -1;
	}
	else
	{
	    int		istrue = PyObject_IsTrue(val);
	    if (istrue == -1)
		return -1;
	    else if (istrue)
		self->list->lv_lock = VAR_LOCKED;
	    else
		self->list->lv_lock = 0;
	}
	return 0;
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, _("cannot set this attribute"));
	return -1;
    }
}

static struct PyMethodDef ListMethods[] = {
    {"extend",	(PyCFunction)ListConcatInPlace,	METH_O,		""},
    {"__dir__",	(PyCFunction)ListDir,		METH_NOARGS,	""},
    { NULL,	NULL,				0,		NULL}
};

typedef struct
{
    PyObject_HEAD
    char_u	*name;
} FunctionObject;

static PyTypeObject FunctionType;

#define NEW_FUNCTION(name) FunctionNew(&FunctionType, name)

    static PyObject *
FunctionNew(PyTypeObject *subtype, char_u *name)
{
    FunctionObject	*self;

    self = (FunctionObject *) subtype->tp_alloc(subtype, 0);

    if (self == NULL)
	return NULL;

    if (isdigit(*name))
    {
	if (!translated_function_exists(name))
	{
	    PyErr_SetString(PyExc_ValueError,
		    _("unnamed function does not exist"));
	    return NULL;
	}
	self->name = vim_strsave(name);
	func_ref(self->name);
    }
    else
	if ((self->name = get_expanded_name(name,
				    vim_strchr(name, AUTOLOAD_CHAR) == NULL))
		== NULL)
	{
	    PyErr_SetString(PyExc_ValueError, _("function does not exist"));
	    return NULL;
	}

    return (PyObject *)(self);
}

    static PyObject *
FunctionConstructor(PyTypeObject *subtype, PyObject *args, PyObject *kwargs)
{
    PyObject	*self;
    char_u	*name;

    if (kwargs)
    {
	PyErr_SetString(PyExc_TypeError,
		_("function constructor does not accept keyword arguments"));
	return NULL;
    }

    if (!PyArg_ParseTuple(args, "s", &name))
	return NULL;

    self = FunctionNew(subtype, name);

    return self;
}

    static void
FunctionDestructor(FunctionObject *self)
{
    func_unref(self->name);
    vim_free(self->name);

    DESTRUCTOR_FINISH(self);
}

static char *FunctionAttrs[] = {
    "softspace",
    NULL
};

    static PyObject *
FunctionDir(PyObject *self)
{
    return ObjectDir(self, FunctionAttrs);
}

    static PyObject *
FunctionCall(FunctionObject *self, PyObject *argsObject, PyObject *kwargs)
{
    char_u	*name = self->name;
    typval_T	args;
    typval_T	selfdicttv;
    typval_T	rettv;
    dict_T	*selfdict = NULL;
    PyObject	*selfdictObject;
    PyObject	*result;
    int		error;

    if (ConvertFromPyObject(argsObject, &args) == -1)
	return NULL;

    if (kwargs != NULL)
    {
	selfdictObject = PyDict_GetItemString(kwargs, "self");
	if (selfdictObject != NULL)
	{
	    if (ConvertFromPyMapping(selfdictObject, &selfdicttv) == -1)
	    {
		clear_tv(&args);
		return NULL;
	    }
	    selfdict = selfdicttv.vval.v_dict;
	}
    }

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();

    VimTryStart();
    error = func_call(name, &args, selfdict, &rettv);

    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (VimTryEnd())
	result = NULL;
    else if (error != OK)
    {
	result = NULL;
	PyErr_SetVim(_("failed to run function"));
    }
    else
	result = ConvertToPyObject(&rettv);

    clear_tv(&args);
    clear_tv(&rettv);
    if (selfdict != NULL)
	clear_tv(&selfdicttv);

    return result;
}

    static PyObject *
FunctionRepr(FunctionObject *self)
{
    return PyString_FromFormat("<vim.Function '%s'>", self->name);
}

static struct PyMethodDef FunctionMethods[] = {
    {"__dir__",	(PyCFunction)FunctionDir,   METH_NOARGS,		""},
    { NULL,	NULL,			0,				NULL}
};

/*
 * Options object
 */

static PyTypeObject OptionsType;

typedef int (*checkfun)(void *);

typedef struct
{
    PyObject_HEAD
    int opt_type;
    void *from;
    checkfun Check;
    PyObject *fromObj;
} OptionsObject;

    static int
dummy_check(void *arg UNUSED)
{
    return 0;
}

    static PyObject *
OptionsNew(int opt_type, void *from, checkfun Check, PyObject *fromObj)
{
    OptionsObject	*self;

    self = PyObject_GC_New(OptionsObject, &OptionsType);
    if (self == NULL)
	return NULL;

    self->opt_type = opt_type;
    self->from = from;
    self->Check = Check;
    self->fromObj = fromObj;
    if (fromObj)
	Py_INCREF(fromObj);

    return (PyObject *)(self);
}

    static void
OptionsDestructor(OptionsObject *self)
{
    PyObject_GC_UnTrack((void *)(self));
    Py_XDECREF(self->fromObj);
    PyObject_GC_Del((void *)(self));
}

    static int
OptionsTraverse(OptionsObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->fromObj);
    return 0;
}

    static int
OptionsClear(OptionsObject *self)
{
    Py_CLEAR(self->fromObj);
    return 0;
}

    static PyObject *
OptionsItem(OptionsObject *self, PyObject *keyObject)
{
    char_u	*key;
    int		flags;
    long	numval;
    char_u	*stringval;
    PyObject	*todecref;

    if (self->Check(self->from))
	return NULL;

    if (!(key = StringToChars(keyObject, &todecref)))
	return NULL;

    if (*key == NUL)
    {
	RAISE_NO_EMPTY_KEYS;
	Py_XDECREF(todecref);
	return NULL;
    }

    flags = get_option_value_strict(key, &numval, &stringval,
				    self->opt_type, self->from);

    Py_XDECREF(todecref);

    if (flags == 0)
    {
	PyErr_SetObject(PyExc_KeyError, keyObject);
	return NULL;
    }

    if (flags & SOPT_UNSET)
    {
	Py_INCREF(Py_None);
	return Py_None;
    }
    else if (flags & SOPT_BOOL)
    {
	PyObject	*r;
	r = numval ? Py_True : Py_False;
	Py_INCREF(r);
	return r;
    }
    else if (flags & SOPT_NUM)
	return PyInt_FromLong(numval);
    else if (flags & SOPT_STRING)
    {
	if (stringval)
	{
	    PyObject	*r = PyBytes_FromString((char *) stringval);
	    vim_free(stringval);
	    return r;
	}
	else
	{
	    PyErr_SetString(PyExc_RuntimeError,
		    _("unable to get option value"));
	    return NULL;
	}
    }
    else
    {
	PyErr_SetVim("Internal error: unknown option type. Should not happen");
	return NULL;
    }
}

    static int
set_option_value_err(key, numval, stringval, opt_flags)
    char_u	*key;
    int		numval;
    char_u	*stringval;
    int		opt_flags;
{
    char_u	*errmsg;

    if ((errmsg = set_option_value(key, numval, stringval, opt_flags)))
    {
	if (VimTryEnd())
	    return FAIL;
	PyErr_SetVim((char *)errmsg);
	return FAIL;
    }
    return OK;
}

    static int
set_option_value_for(key, numval, stringval, opt_flags, opt_type, from)
    char_u	*key;
    int		numval;
    char_u	*stringval;
    int		opt_flags;
    int		opt_type;
    void	*from;
{
    win_T	*save_curwin = NULL;
    tabpage_T	*save_curtab = NULL;
    buf_T	*save_curbuf = NULL;
    int		r = 0;

    VimTryStart();
    switch (opt_type)
    {
	case SREQ_WIN:
	    if (switch_win(&save_curwin, &save_curtab, (win_T *)from,
				     win_find_tabpage((win_T *)from)) == FAIL)
	    {
		if (VimTryEnd())
		    return -1;
		PyErr_SetVim("Problem while switching windows.");
		return -1;
	    }
	    r = set_option_value_err(key, numval, stringval, opt_flags);
	    restore_win(save_curwin, save_curtab);
	    if (r == FAIL)
		return -1;
	    break;
	case SREQ_BUF:
	    switch_buffer(&save_curbuf, (buf_T *)from);
	    r = set_option_value_err(key, numval, stringval, opt_flags);
	    restore_buffer(save_curbuf);
	    if (r == FAIL)
		return -1;
	    break;
	case SREQ_GLOBAL:
	    r = set_option_value_err(key, numval, stringval, opt_flags);
	    if (r == FAIL)
		return -1;
	    break;
    }
    return VimTryEnd();
}

    static int
OptionsAssItem(OptionsObject *self, PyObject *keyObject, PyObject *valObject)
{
    char_u	*key;
    int		flags;
    int		opt_flags;
    int		r = 0;
    PyObject	*todecref;

    if (self->Check(self->from))
	return -1;

    if (!(key = StringToChars(keyObject, &todecref)))
	return -1;

    if (*key == NUL)
    {
	RAISE_NO_EMPTY_KEYS;
	Py_XDECREF(todecref);
	return -1;
    }

    flags = get_option_value_strict(key, NULL, NULL,
				    self->opt_type, self->from);

    if (flags == 0)
    {
	PyErr_SetObject(PyExc_KeyError, keyObject);
	Py_XDECREF(todecref);
	return -1;
    }

    if (valObject == NULL)
    {
	if (self->opt_type == SREQ_GLOBAL)
	{
	    PyErr_SetString(PyExc_ValueError,
		    _("unable to unset global option"));
	    Py_XDECREF(todecref);
	    return -1;
	}
	else if (!(flags & SOPT_GLOBAL))
	{
	    PyErr_SetString(PyExc_ValueError, _("unable to unset option "
						"without global value"));
	    Py_XDECREF(todecref);
	    return -1;
	}
	else
	{
	    unset_global_local_option(key, self->from);
	    Py_XDECREF(todecref);
	    return 0;
	}
    }

    opt_flags = (self->opt_type ? OPT_LOCAL : OPT_GLOBAL);

    if (flags & SOPT_BOOL)
    {
	int	istrue = PyObject_IsTrue(valObject);

	if (istrue == -1)
	    r = -1;
	else
	    r = set_option_value_for(key, istrue, NULL,
				    opt_flags, self->opt_type, self->from);
    }
    else if (flags & SOPT_NUM)
    {
	int val;

#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(valObject))
	    val = PyInt_AsLong(valObject);
	else
#endif
	if (PyLong_Check(valObject))
	    val = PyLong_AsLong(valObject);
	else
	{
	    PyErr_SetString(PyExc_TypeError, _("object must be integer"));
	    Py_XDECREF(todecref);
	    return -1;
	}

	r = set_option_value_for(key, val, NULL, opt_flags,
				self->opt_type, self->from);
    }
    else
    {
	char_u	*val;
	PyObject	*todecref;

	if ((val = StringToChars(valObject, &todecref)))
	    r = set_option_value_for(key, 0, val, opt_flags,
				    self->opt_type, self->from);
	else
	    r = -1;
    }

    Py_XDECREF(todecref);

    return r;
}

static PyMappingMethods OptionsAsMapping = {
    (lenfunc)       NULL,
    (binaryfunc)    OptionsItem,
    (objobjargproc) OptionsAssItem,
};

/* Tabpage object
 */

typedef struct
{
    PyObject_HEAD
    tabpage_T	*tab;
} TabPageObject;

static PyObject *WinListNew(TabPageObject *tabObject);

static PyTypeObject TabPageType;

    static int
CheckTabPage(TabPageObject *self)
{
    if (self->tab == INVALID_TABPAGE_VALUE)
    {
	PyErr_SetVim(_("attempt to refer to deleted tab page"));
	return -1;
    }

    return 0;
}

    static PyObject *
TabPageNew(tabpage_T *tab)
{
    TabPageObject *self;

    if (TAB_PYTHON_REF(tab))
    {
	self = TAB_PYTHON_REF(tab);
	Py_INCREF(self);
    }
    else
    {
	self = PyObject_NEW(TabPageObject, &TabPageType);
	if (self == NULL)
	    return NULL;
	self->tab = tab;
	TAB_PYTHON_REF(tab) = self;
    }

    return (PyObject *)(self);
}

    static void
TabPageDestructor(TabPageObject *self)
{
    if (self->tab && self->tab != INVALID_TABPAGE_VALUE)
	TAB_PYTHON_REF(self->tab) = NULL;

    DESTRUCTOR_FINISH(self);
}

static char *TabPageAttrs[] = {
    "windows", "number", "vars", "window", "valid",
    NULL
};

    static PyObject *
TabPageDir(PyObject *self)
{
    return ObjectDir(self, TabPageAttrs);
}

    static PyObject *
TabPageAttrValid(TabPageObject *self, char *name)
{
    PyObject *r;

    if (strcmp(name, "valid") != 0)
	return NULL;

    r = ((self->tab == INVALID_TABPAGE_VALUE) ? Py_False : Py_True);
    Py_INCREF(r);
    return r;
}

    static PyObject *
TabPageAttr(TabPageObject *self, char *name)
{
    if (strcmp(name, "windows") == 0)
	return WinListNew(self);
    else if (strcmp(name, "number") == 0)
	return PyLong_FromLong((long) get_tab_number(self->tab));
    else if (strcmp(name, "vars") == 0)
	return NEW_DICTIONARY(self->tab->tp_vars);
    else if (strcmp(name, "window") == 0)
    {
	/* For current tab window.c does not bother to set or update tp_curwin
	 */
	if (self->tab == curtab)
	    return WindowNew(curwin, curtab);
	else
	    return WindowNew(self->tab->tp_curwin, self->tab);
    }
    else if (strcmp(name, "__members__") == 0)
	return ObjectDir(NULL, TabPageAttrs);
    return NULL;
}

    static PyObject *
TabPageRepr(TabPageObject *self)
{
    if (self->tab == INVALID_TABPAGE_VALUE)
	return PyString_FromFormat("<tabpage object (deleted) at %p>", (self));
    else
    {
	int	t = get_tab_number(self->tab);

	if (t == 0)
	    return PyString_FromFormat("<tabpage object (unknown) at %p>",
					(self));
	else
	    return PyString_FromFormat("<tabpage %d>", t - 1);
    }
}

static struct PyMethodDef TabPageMethods[] = {
    /* name,	    function,			calling,	documentation */
    {"__dir__",	    (PyCFunction)TabPageDir,	METH_NOARGS,	""},
    { NULL,	    NULL,			0,		NULL}
};

/*
 * Window list object
 */

static PyTypeObject TabListType;
static PySequenceMethods TabListAsSeq;

typedef struct
{
    PyObject_HEAD
} TabListObject;

    static PyInt
TabListLength(PyObject *self UNUSED)
{
    tabpage_T	*tp = first_tabpage;
    PyInt	n = 0;

    while (tp != NULL)
    {
	++n;
	tp = tp->tp_next;
    }

    return n;
}

    static PyObject *
TabListItem(PyObject *self UNUSED, PyInt n)
{
    tabpage_T	*tp;

    for (tp = first_tabpage; tp != NULL; tp = tp->tp_next, --n)
	if (n == 0)
	    return TabPageNew(tp);

    PyErr_SetString(PyExc_IndexError, _("no such tab page"));
    return NULL;
}

/* Window object
 */

typedef struct
{
    PyObject_HEAD
    win_T	*win;
    TabPageObject	*tabObject;
} WindowObject;

static PyTypeObject WindowType;

    static int
CheckWindow(WindowObject *self)
{
    if (self->win == INVALID_WINDOW_VALUE)
    {
	PyErr_SetVim(_("attempt to refer to deleted window"));
	return -1;
    }

    return 0;
}

    static PyObject *
WindowNew(win_T *win, tabpage_T *tab)
{
    /* We need to handle deletion of windows underneath us.
     * If we add a "w_python*_ref" field to the win_T structure,
     * then we can get at it in win_free() in vim. We then
     * need to create only ONE Python object per window - if
     * we try to create a second, just INCREF the existing one
     * and return it. The (single) Python object referring to
     * the window is stored in "w_python*_ref".
     * On a win_free() we set the Python object's win_T* field
     * to an invalid value. We trap all uses of a window
     * object, and reject them if the win_T* field is invalid.
     *
     * Python2 and Python3 get different fields and different objects:
     * w_python_ref and w_python3_ref fields respectively.
     */

    WindowObject *self;

    if (WIN_PYTHON_REF(win))
    {
	self = WIN_PYTHON_REF(win);
	Py_INCREF(self);
    }
    else
    {
	self = PyObject_GC_New(WindowObject, &WindowType);
	if (self == NULL)
	    return NULL;
	self->win = win;
	WIN_PYTHON_REF(win) = self;
    }

    self->tabObject = ((TabPageObject *)(TabPageNew(tab)));

    return (PyObject *)(self);
}

    static void
WindowDestructor(WindowObject *self)
{
    PyObject_GC_UnTrack((void *)(self));
    if (self->win && self->win != INVALID_WINDOW_VALUE)
	WIN_PYTHON_REF(self->win) = NULL;
     Py_XDECREF(((PyObject *)(self->tabObject)));
    PyObject_GC_Del((void *)(self));
}

    static int
WindowTraverse(WindowObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((PyObject *)(self->tabObject)));
    return 0;
}

    static int
WindowClear(WindowObject *self)
{
    Py_CLEAR(self->tabObject);
    return 0;
}

    static win_T *
get_firstwin(TabPageObject *tabObject)
{
    if (tabObject)
    {
	if (CheckTabPage(tabObject))
	    return NULL;
	/* For current tab window.c does not bother to set or update tp_firstwin
	 */
	else if (tabObject->tab == curtab)
	    return firstwin;
	else
	    return tabObject->tab->tp_firstwin;
    }
    else
	return firstwin;
}
static char *WindowAttrs[] = {
    "buffer", "cursor", "height", "vars", "options", "number", "row", "col",
    "tabpage", "valid",
    NULL
};

    static PyObject *
WindowDir(PyObject *self)
{
    return ObjectDir(self, WindowAttrs);
}

    static PyObject *
WindowAttrValid(WindowObject *self, char *name)
{
    PyObject *r;

    if (strcmp(name, "valid") != 0)
	return NULL;

    r = ((self->win == INVALID_WINDOW_VALUE) ? Py_False : Py_True);
    Py_INCREF(r);
    return r;
}

    static PyObject *
WindowAttr(WindowObject *self, char *name)
{
    if (strcmp(name, "buffer") == 0)
	return (PyObject *)BufferNew(self->win->w_buffer);
    else if (strcmp(name, "cursor") == 0)
    {
	pos_T *pos = &self->win->w_cursor;

	return Py_BuildValue("(ll)", (long)(pos->lnum), (long)(pos->col));
    }
    else if (strcmp(name, "height") == 0)
	return PyLong_FromLong((long)(self->win->w_height));
#ifdef FEAT_WINDOWS
    else if (strcmp(name, "row") == 0)
	return PyLong_FromLong((long)(self->win->w_winrow));
#endif
#ifdef FEAT_VERTSPLIT
    else if (strcmp(name, "width") == 0)
	return PyLong_FromLong((long)(W_WIDTH(self->win)));
    else if (strcmp(name, "col") == 0)
	return PyLong_FromLong((long)(W_WINCOL(self->win)));
#endif
    else if (strcmp(name, "vars") == 0)
	return NEW_DICTIONARY(self->win->w_vars);
    else if (strcmp(name, "options") == 0)
	return OptionsNew(SREQ_WIN, self->win, (checkfun) CheckWindow,
			(PyObject *) self);
    else if (strcmp(name, "number") == 0)
    {
	if (CheckTabPage(self->tabObject))
	    return NULL;
	return PyLong_FromLong((long)
		get_win_number(self->win, get_firstwin(self->tabObject)));
    }
    else if (strcmp(name, "tabpage") == 0)
    {
	Py_INCREF(self->tabObject);
	return (PyObject *)(self->tabObject);
    }
    else if (strcmp(name, "__members__") == 0)
	return ObjectDir(NULL, WindowAttrs);
    else
	return NULL;
}

    static int
WindowSetattr(WindowObject *self, char *name, PyObject *val)
{
    if (CheckWindow(self))
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

	if (lnum <= 0 || lnum > self->win->w_buffer->b_ml.ml_line_count)
	{
	    PyErr_SetVim(_("cursor position outside buffer"));
	    return -1;
	}

	/* Check for keyboard interrupts */
	if (VimCheckInterrupt())
	    return -1;

	self->win->w_cursor.lnum = lnum;
	self->win->w_cursor.col = col;
#ifdef FEAT_VIRTUALEDIT
	self->win->w_cursor.coladd = 0;
#endif
	/* When column is out of range silently correct it. */
	check_cursor_col_win(self->win);

	update_screen(VALID);
	return 0;
    }
    else if (strcmp(name, "height") == 0)
    {
	int	height;
	win_T	*savewin;

	if (!PyArg_Parse(val, "i", &height))
	    return -1;

#ifdef FEAT_GUI
	need_mouse_correct = TRUE;
#endif
	savewin = curwin;
	curwin = self->win;

	VimTryStart();
	win_setheight(height);
	curwin = savewin;
	if (VimTryEnd())
	    return -1;

	return 0;
    }
#ifdef FEAT_VERTSPLIT
    else if (strcmp(name, "width") == 0)
    {
	int	width;
	win_T	*savewin;

	if (!PyArg_Parse(val, "i", &width))
	    return -1;

#ifdef FEAT_GUI
	need_mouse_correct = TRUE;
#endif
	savewin = curwin;
	curwin = self->win;

	VimTryStart();
	win_setwidth(width);
	curwin = savewin;
	if (VimTryEnd())
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

    static PyObject *
WindowRepr(WindowObject *self)
{
    if (self->win == INVALID_WINDOW_VALUE)
	return PyString_FromFormat("<window object (deleted) at %p>", (self));
    else
    {
	int	w = get_win_number(self->win, firstwin);

	if (w == 0)
	    return PyString_FromFormat("<window object (unknown) at %p>",
								      (self));
	else
	    return PyString_FromFormat("<window %d>", w - 1);
    }
}

static struct PyMethodDef WindowMethods[] = {
    /* name,	    function,			calling,	documentation */
    {"__dir__",	    (PyCFunction)WindowDir,	METH_NOARGS,	""},
    { NULL,	    NULL,			0,		NULL}
};

/*
 * Window list object
 */

static PyTypeObject WinListType;
static PySequenceMethods WinListAsSeq;

typedef struct
{
    PyObject_HEAD
    TabPageObject	*tabObject;
} WinListObject;

    static PyObject *
WinListNew(TabPageObject *tabObject)
{
    WinListObject	*self;

    self = PyObject_NEW(WinListObject, &WinListType);
    self->tabObject = tabObject;
    Py_INCREF(tabObject);

    return (PyObject *)(self);
}

    static void
WinListDestructor(WinListObject *self)
{
    TabPageObject	*tabObject = self->tabObject;

    if (tabObject)
    {
	Py_DECREF((PyObject *)(tabObject));
    }

    DESTRUCTOR_FINISH(self);
}

    static PyInt
WinListLength(WinListObject *self)
{
    win_T	*w;
    PyInt	n = 0;

    if (!(w = get_firstwin(self->tabObject)))
	return -1;

    while (w != NULL)
    {
	++n;
	w = W_NEXT(w);
    }

    return n;
}

    static PyObject *
WinListItem(WinListObject *self, PyInt n)
{
    win_T *w;

    if (!(w = get_firstwin(self->tabObject)))
	return NULL;

    for (; w != NULL; w = W_NEXT(w), --n)
	if (n == 0)
	    return WindowNew(w, self->tabObject? self->tabObject->tab: curtab);

    PyErr_SetString(PyExc_IndexError, _("no such window"));
    return NULL;
}

/* Convert a Python string into a Vim line.
 *
 * The result is in allocated memory. All internal nulls are replaced by
 * newline characters. It is an error for the string to contain newline
 * characters.
 *
 * On errors, the Python exception data is set, and NULL is returned.
 */
    static char *
StringToLine(PyObject *obj)
{
    const char *str;
    char *save;
    PyObject *bytes;
    PyInt len;
    PyInt i;
    char *p;

    if (obj == NULL || !PyString_Check(obj))
    {
	PyErr_BadArgument();
	return NULL;
    }

    bytes = PyString_AsBytes(obj);  /* for Python 2 this does nothing */
    str = PyString_AsString(bytes);
    len = PyString_Size(bytes);

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
    PyString_FreeBytes(bytes);  /* Python 2 does nothing here */

    return save;
}

/* Get a line from the specified buffer. The line number is
 * in Vim format (1-based). The line is returned as a Python
 * string object.
 */
    static PyObject *
GetBufferLine(buf_T *buf, PyInt n)
{
    return LineToString((char *)ml_get_buf(buf, (linenr_T)n, FALSE));
}


/* Get a list of lines from the specified buffer. The line numbers
 * are in Vim format (1-based). The range is from lo up to, but not
 * including, hi. The list is returned as a Python list of string objects.
 */
    static PyObject *
GetBufferLineList(buf_T *buf, PyInt lo, PyInt hi)
{
    PyInt i;
    PyInt n = hi - lo;
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

	PyList_SET_ITEM(list, i, str);
    }

    /* The ownership of the Python list is passed to the caller (ie,
     * the caller should Py_DECREF() the object when it is finished
     * with it).
     */

    return list;
}

/*
 * Check if deleting lines made the cursor position invalid.
 * Changed the lines from "lo" to "hi" and added "extra" lines (negative if
 * deleted).
 */
    static void
py_fix_cursor(linenr_T lo, linenr_T hi, linenr_T extra)
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

/*
 * Replace a line in the specified buffer. The line number is
 * in Vim format (1-based). The replacement line is given as
 * a Python string object. The object is checked for validity
 * and correct format. Errors are returned as a value of FAIL.
 * The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
    static int
SetBufferLine(buf_T *buf, PyInt n, PyObject *line, PyInt *len_change)
{
    /* First of all, we check the type of the supplied Python object.
     * There are three cases:
     *	  1. NULL, or None - this is a deletion.
     *	  2. A string	   - this is a replacement.
     *	  3. Anything else - this is an error.
     */
    if (line == Py_None || line == NULL)
    {
	buf_T *savebuf;

	PyErr_Clear();
	switch_buffer(&savebuf, buf);

	VimTryStart();

	if (u_savedel((linenr_T)n, 1L) == FAIL)
	    PyErr_SetVim(_("cannot save undo information"));
	else if (ml_delete((linenr_T)n, FALSE) == FAIL)
	    PyErr_SetVim(_("cannot delete line"));
	else
	{
	    if (buf == savebuf)
		py_fix_cursor((linenr_T)n, (linenr_T)n + 1, (linenr_T)-1);
	    deleted_lines_mark((linenr_T)n, 1L);
	}

	restore_buffer(savebuf);

	if (VimTryEnd())
	    return FAIL;

	if (len_change)
	    *len_change = -1;

	return OK;
    }
    else if (PyString_Check(line))
    {
	char *save = StringToLine(line);
	buf_T *savebuf;

	if (save == NULL)
	    return FAIL;

	VimTryStart();

	/* We do not need to free "save" if ml_replace() consumes it. */
	PyErr_Clear();
	switch_buffer(&savebuf, buf);

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

	restore_buffer(savebuf);

	/* Check that the cursor is not beyond the end of the line now. */
	if (buf == savebuf)
	    check_cursor_col();

	if (VimTryEnd())
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

/* Replace a range of lines in the specified buffer. The line numbers are in
 * Vim format (1-based). The range is from lo up to, but not including, hi.
 * The replacement lines are given as a Python list of string objects. The
 * list is checked for validity and correct format. Errors are returned as a
 * value of FAIL.  The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
    static int
SetBufferLineList(buf_T *buf, PyInt lo, PyInt hi, PyObject *list, PyInt *len_change)
{
    /* First of all, we check the type of the supplied Python object.
     * There are three cases:
     *	  1. NULL, or None - this is a deletion.
     *	  2. A list	   - this is a replacement.
     *	  3. Anything else - this is an error.
     */
    if (list == Py_None || list == NULL)
    {
	PyInt	i;
	PyInt	n = (int)(hi - lo);
	buf_T	*savebuf;

	PyErr_Clear();
	VimTryStart();
	switch_buffer(&savebuf, buf);

	if (u_savedel((linenr_T)lo, (long)n) == FAIL)
	    PyErr_SetVim(_("cannot save undo information"));
	else
	{
	    for (i = 0; i < n; ++i)
	    {
		if (ml_delete((linenr_T)lo, FALSE) == FAIL)
		{
		    PyErr_SetVim(_("cannot delete line"));
		    break;
		}
	    }
	    if (buf == savebuf)
		py_fix_cursor((linenr_T)lo, (linenr_T)hi, (linenr_T)-n);
	    deleted_lines_mark((linenr_T)lo, (long)i);
	}

	restore_buffer(savebuf);

	if (VimTryEnd())
	    return FAIL;

	if (len_change)
	    *len_change = -n;

	return OK;
    }
    else if (PyList_Check(list))
    {
	PyInt	i;
	PyInt	new_len = PyList_Size(list);
	PyInt	old_len = hi - lo;
	PyInt	extra = 0;	/* lines added to text, can be negative */
	char	**array;
	buf_T	*savebuf;

	if (new_len == 0)	/* avoid allocating zero bytes */
	    array = NULL;
	else
	{
	    array = PyMem_New(char *, new_len);
	    if (array == NULL)
	    {
		PyErr_NoMemory();
		return FAIL;
	    }
	}

	for (i = 0; i < new_len; ++i)
	{
	    PyObject *line;

	    if (!(line = PyList_GetItem(list, i)) ||
		!(array[i] = StringToLine(line)))
	    {
		while (i)
		    vim_free(array[--i]);
		PyMem_Free(array);
		return FAIL;
	    }
	}

	VimTryStart();
	PyErr_Clear();

	/* START of region without "return".  Must call restore_buffer()! */
	switch_buffer(&savebuf, buf);

	if (u_save((linenr_T)(lo-1), (linenr_T)hi) == FAIL)
	    PyErr_SetVim(_("cannot save undo information"));

	/* If the size of the range is reducing (ie, new_len < old_len) we
	 * need to delete some old_len. We do this at the start, by
	 * repeatedly deleting line "lo".
	 */
	if (!PyErr_Occurred())
	{
	    for (i = 0; i < old_len - new_len; ++i)
		if (ml_delete((linenr_T)lo, FALSE) == FAIL)
		{
		    PyErr_SetVim(_("cannot delete line"));
		    break;
		}
	    extra -= i;
	}

	/* For as long as possible, replace the existing old_len with the
	 * new old_len. This is a more efficient operation, as it requires
	 * less memory allocation and freeing.
	 */
	if (!PyErr_Occurred())
	{
	    for (i = 0; i < old_len && i < new_len; ++i)
		if (ml_replace((linenr_T)(lo+i), (char_u *)array[i], FALSE)
								      == FAIL)
		{
		    PyErr_SetVim(_("cannot replace line"));
		    break;
		}
	}
	else
	    i = 0;

	/* Now we may need to insert the remaining new old_len. If we do, we
	 * must free the strings as we finish with them (we can't pass the
	 * responsibility to vim in this case).
	 */
	if (!PyErr_Occurred())
	{
	    while (i < new_len)
	    {
		if (ml_append((linenr_T)(lo + i - 1),
					(char_u *)array[i], 0, FALSE) == FAIL)
		{
		    PyErr_SetVim(_("cannot insert line"));
		    break;
		}
		vim_free(array[i]);
		++i;
		++extra;
	    }
	}

	/* Free any left-over old_len, as a result of an error */
	while (i < new_len)
	{
	    vim_free(array[i]);
	    ++i;
	}

	/* Free the array of old_len. All of its contents have now
	 * been dealt with (either freed, or the responsibility passed
	 * to vim.
	 */
	PyMem_Free(array);

	/* Adjust marks. Invalidate any which lie in the
	 * changed range, and move any in the remainder of the buffer.
	 */
	mark_adjust((linenr_T)lo, (linenr_T)(hi - 1),
						  (long)MAXLNUM, (long)extra);
	changed_lines((linenr_T)lo, 0, (linenr_T)hi, (long)extra);

	if (buf == savebuf)
	    py_fix_cursor((linenr_T)lo, (linenr_T)hi, (linenr_T)extra);

	/* END of region without "return". */
	restore_buffer(savebuf);

	if (VimTryEnd())
	    return FAIL;

	if (len_change)
	    *len_change = new_len - old_len;

	return OK;
    }
    else
    {
	PyErr_BadArgument();
	return FAIL;
    }
}

/* Insert a number of lines into the specified buffer after the specified line.
 * The line number is in Vim format (1-based). The lines to be inserted are
 * given as a Python list of string objects or as a single string. The lines
 * to be added are checked for validity and correct format. Errors are
 * returned as a value of FAIL.  The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
    static int
InsertBufferLines(buf_T *buf, PyInt n, PyObject *lines, PyInt *len_change)
{
    /* First of all, we check the type of the supplied Python object.
     * It must be a string or a list, or the call is in error.
     */
    if (PyString_Check(lines))
    {
	char	*str = StringToLine(lines);
	buf_T	*savebuf;

	if (str == NULL)
	    return FAIL;

	PyErr_Clear();
	VimTryStart();
	switch_buffer(&savebuf, buf);

	if (u_save((linenr_T)n, (linenr_T)(n+1)) == FAIL)
	    PyErr_SetVim(_("cannot save undo information"));
	else if (ml_append((linenr_T)n, (char_u *)str, 0, FALSE) == FAIL)
	    PyErr_SetVim(_("cannot insert line"));
	else
	    appended_lines_mark((linenr_T)n, 1L);

	vim_free(str);
	restore_buffer(savebuf);
	update_screen(VALID);

	if (VimTryEnd())
	    return FAIL;

	if (len_change)
	    *len_change = 1;

	return OK;
    }
    else if (PyList_Check(lines))
    {
	PyInt	i;
	PyInt	size = PyList_Size(lines);
	char	**array;
	buf_T	*savebuf;

	array = PyMem_New(char *, size);
	if (array == NULL)
	{
	    PyErr_NoMemory();
	    return FAIL;
	}

	for (i = 0; i < size; ++i)
	{
	    PyObject *line;

	    if (!(line = PyList_GetItem(lines, i)) ||
		!(array[i] = StringToLine(line)))
	    {
		while (i)
		    vim_free(array[--i]);
		PyMem_Free(array);
		return FAIL;
	    }
	}

	PyErr_Clear();
	VimTryStart();
	switch_buffer(&savebuf, buf);

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
	PyMem_Free(array);

	restore_buffer(savebuf);
	update_screen(VALID);

	if (VimTryEnd())
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

/*
 * Common routines for buffers and line ranges
 * -------------------------------------------
 */

typedef struct
{
    PyObject_HEAD
    buf_T *buf;
} BufferObject;

    static int
CheckBuffer(BufferObject *self)
{
    if (self->buf == INVALID_BUFFER_VALUE)
    {
	PyErr_SetVim(_("attempt to refer to deleted buffer"));
	return -1;
    }

    return 0;
}

    static PyObject *
RBItem(BufferObject *self, PyInt n, PyInt start, PyInt end)
{
    if (CheckBuffer(self))
	return NULL;

    if (end == -1)
	end = self->buf->b_ml.ml_line_count;

    if (n < 0)
	n += end - start + 1;

    if (n < 0 || n > end - start)
    {
	PyErr_SetString(PyExc_IndexError, _("line number out of range"));
	return NULL;
    }

    return GetBufferLine(self->buf, n+start);
}

    static PyObject *
RBSlice(BufferObject *self, PyInt lo, PyInt hi, PyInt start, PyInt end)
{
    PyInt size;

    if (CheckBuffer(self))
	return NULL;

    if (end == -1)
	end = self->buf->b_ml.ml_line_count;

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

    static PyInt
RBAsItem(BufferObject *self, PyInt n, PyObject *val, PyInt start, PyInt end, PyInt *new_end)
{
    PyInt len_change;

    if (CheckBuffer(self))
	return -1;

    if (end == -1)
	end = self->buf->b_ml.ml_line_count;

    if (n < 0)
	n += end - start + 1;

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

    static PyInt
RBAsSlice(BufferObject *self, PyInt lo, PyInt hi, PyObject *val, PyInt start, PyInt end, PyInt *new_end)
{
    PyInt size;
    PyInt len_change;

    /* Self must be a valid buffer */
    if (CheckBuffer(self))
	return -1;

    if (end == -1)
	end = self->buf->b_ml.ml_line_count;

    /* Sort out the slice range */
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

    if (SetBufferLineList(self->buf, lo + start, hi + start,
						    val, &len_change) == FAIL)
	return -1;

    if (new_end)
	*new_end = end + len_change;

    return 0;
}


    static PyObject *
RBAppend(BufferObject *self, PyObject *args, PyInt start, PyInt end, PyInt *new_end)
{
    PyObject *lines;
    PyInt len_change;
    PyInt max;
    PyInt n;

    if (CheckBuffer(self))
	return NULL;

    if (end == -1)
	end = self->buf->b_ml.ml_line_count;

    max = n = end - start + 1;

    if (!PyArg_ParseTuple(args, "O|n", &lines, &n))
	return NULL;

    if (n < 0 || n > max)
    {
	PyErr_SetString(PyExc_IndexError, _("line number out of range"));
	return NULL;
    }

    if (InsertBufferLines(self->buf, n + start - 1, lines, &len_change) == FAIL)
	return NULL;

    if (new_end)
	*new_end = end + len_change;

    Py_INCREF(Py_None);
    return Py_None;
}

/* Range object
 */

static PyTypeObject RangeType;
static PySequenceMethods RangeAsSeq;
static PyMappingMethods RangeAsMapping;

typedef struct
{
    PyObject_HEAD
    BufferObject *buf;
    PyInt start;
    PyInt end;
} RangeObject;

    static PyObject *
RangeNew(buf_T *buf, PyInt start, PyInt end)
{
    BufferObject *bufr;
    RangeObject *self;
    self = PyObject_GC_New(RangeObject, &RangeType);
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

    static void
RangeDestructor(RangeObject *self)
{
    PyObject_GC_UnTrack((void *)(self));
    Py_XDECREF(self->buf);
    PyObject_GC_Del((void *)(self));
}

    static int
RangeTraverse(RangeObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((PyObject *)(self->buf)));
    return 0;
}

    static int
RangeClear(RangeObject *self)
{
    Py_CLEAR(self->buf);
    return 0;
}

    static PyInt
RangeLength(RangeObject *self)
{
    /* HOW DO WE SIGNAL AN ERROR FROM THIS FUNCTION? */
    if (CheckBuffer(self->buf))
	return -1; /* ??? */

    return (self->end - self->start + 1);
}

    static PyObject *
RangeItem(RangeObject *self, PyInt n)
{
    return RBItem(self->buf, n, self->start, self->end);
}

    static PyObject *
RangeSlice(RangeObject *self, PyInt lo, PyInt hi)
{
    return RBSlice(self->buf, lo, hi, self->start, self->end);
}

static char *RangeAttrs[] = {
    "start", "end",
    NULL
};

    static PyObject *
RangeDir(PyObject *self)
{
    return ObjectDir(self, RangeAttrs);
}

    static PyObject *
RangeAppend(RangeObject *self, PyObject *args)
{
    return RBAppend(self->buf, args, self->start, self->end, &self->end);
}

    static PyObject *
RangeRepr(RangeObject *self)
{
    if (self->buf->buf == INVALID_BUFFER_VALUE)
	return PyString_FromFormat("<range object (for deleted buffer) at %p>",
				    (self));
    else
    {
	char *name = (char *)self->buf->buf->b_fname;

	if (name == NULL)
	    name = "";

	return PyString_FromFormat("<range %s (%d:%d)>",
				    name, (int)self->start, (int)self->end);
    }
}

static struct PyMethodDef RangeMethods[] = {
    /* name,	function,			calling,	documentation */
    {"append",	(PyCFunction)RangeAppend,	METH_VARARGS,	"Append data to the Vim range" },
    {"__dir__",	(PyCFunction)RangeDir,		METH_NOARGS,	""},
    { NULL,	NULL,				0,		NULL}
};

static PyTypeObject BufferType;
static PySequenceMethods BufferAsSeq;
static PyMappingMethods BufferAsMapping;

    static PyObject *
BufferNew(buf_T *buf)
{
    /* We need to handle deletion of buffers underneath us.
     * If we add a "b_python*_ref" field to the buf_T structure,
     * then we can get at it in buf_freeall() in vim. We then
     * need to create only ONE Python object per buffer - if
     * we try to create a second, just INCREF the existing one
     * and return it. The (single) Python object referring to
     * the buffer is stored in "b_python*_ref".
     * Question: what to do on a buf_freeall(). We'll probably
     * have to either delete the Python object (DECREF it to
     * zero - a bad idea, as it leaves dangling refs!) or
     * set the buf_T * value to an invalid value (-1?), which
     * means we need checks in all access functions... Bah.
     *
     * Python2 and Python3 get different fields and different objects:
     * b_python_ref and b_python3_ref fields respectively.
     */

    BufferObject *self;

    if (BUF_PYTHON_REF(buf) != NULL)
    {
	self = BUF_PYTHON_REF(buf);
	Py_INCREF(self);
    }
    else
    {
	self = PyObject_NEW(BufferObject, &BufferType);
	if (self == NULL)
	    return NULL;
	self->buf = buf;
	BUF_PYTHON_REF(buf) = self;
    }

    return (PyObject *)(self);
}

    static void
BufferDestructor(BufferObject *self)
{
    if (self->buf && self->buf != INVALID_BUFFER_VALUE)
	BUF_PYTHON_REF(self->buf) = NULL;

    DESTRUCTOR_FINISH(self);
}

    static PyInt
BufferLength(BufferObject *self)
{
    /* HOW DO WE SIGNAL AN ERROR FROM THIS FUNCTION? */
    if (CheckBuffer(self))
	return -1; /* ??? */

    return (PyInt)(self->buf->b_ml.ml_line_count);
}

    static PyObject *
BufferItem(BufferObject *self, PyInt n)
{
    return RBItem(self, n, 1, -1);
}

    static PyObject *
BufferSlice(BufferObject *self, PyInt lo, PyInt hi)
{
    return RBSlice(self, lo, hi, 1, -1);
}

static char *BufferAttrs[] = {
    "name", "number", "vars", "options", "valid",
    NULL
};

    static PyObject *
BufferDir(PyObject *self)
{
    return ObjectDir(self, BufferAttrs);
}

    static PyObject *
BufferAttrValid(BufferObject *self, char *name)
{
    PyObject *r;

    if (strcmp(name, "valid") != 0)
	return NULL;

    r = ((self->buf == INVALID_BUFFER_VALUE) ? Py_False : Py_True);
    Py_INCREF(r);
    return r;
}

    static PyObject *
BufferAttr(BufferObject *self, char *name)
{
    if (strcmp(name, "name") == 0)
	return PyString_FromString((self->buf->b_ffname == NULL
				    ? "" : (char *) self->buf->b_ffname));
    else if (strcmp(name, "number") == 0)
	return Py_BuildValue(Py_ssize_t_fmt, self->buf->b_fnum);
    else if (strcmp(name, "vars") == 0)
	return NEW_DICTIONARY(self->buf->b_vars);
    else if (strcmp(name, "options") == 0)
	return OptionsNew(SREQ_BUF, self->buf, (checkfun) CheckBuffer,
			(PyObject *) self);
    else if (strcmp(name, "__members__") == 0)
	return ObjectDir(NULL, BufferAttrs);
    else
	return NULL;
}

    static int
BufferSetattr(BufferObject *self, char *name, PyObject *valObject)
{
    if (CheckBuffer(self))
	return -1;

    if (strcmp(name, "name") == 0)
    {
	char_u	*val;
	aco_save_T	aco;
	int	r;
	PyObject	*todecref;

	if (!(val = StringToChars(valObject, &todecref)))
	    return -1;

	VimTryStart();
	/* Using aucmd_*: autocommands will be executed by rename_buffer */
	aucmd_prepbuf(&aco, self->buf);
	r = rename_buffer(val);
	aucmd_restbuf(&aco);
	Py_XDECREF(todecref);
	if (VimTryEnd())
	    return -1;

	if (r == FAIL)
	{
	    PyErr_SetVim(_("failed to rename buffer"));
	    return -1;
	}
	return 0;
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
    }
}

    static PyObject *
BufferAppend(BufferObject *self, PyObject *args)
{
    return RBAppend(self, args, 1, -1, NULL);
}

    static PyObject *
BufferMark(BufferObject *self, PyObject *args)
{
    pos_T	*posp;
    char	*pmark;
    char	mark;
    buf_T	*savebuf;

    if (CheckBuffer(self))
	return NULL;

    if (!PyArg_ParseTuple(args, "s", &pmark))
	return NULL;

    if (STRLEN(pmark) != 1)
    {
	PyErr_SetString(PyExc_ValueError,
		_("mark name must be a single character"));
	return NULL;
    }

    mark = *pmark;
    VimTryStart();
    switch_buffer(&savebuf, self->buf);
    posp = getmark(mark, FALSE);
    restore_buffer(savebuf);
    if (VimTryEnd())
	return NULL;

    if (posp == NULL)
    {
	PyErr_SetVim(_("invalid mark name"));
	return NULL;
    }

    if (posp->lnum <= 0)
    {
	/* Or raise an error? */
	Py_INCREF(Py_None);
	return Py_None;
    }

    return Py_BuildValue("(ll)", (long)(posp->lnum), (long)(posp->col));
}

    static PyObject *
BufferRange(BufferObject *self, PyObject *args)
{
    PyInt start;
    PyInt end;

    if (CheckBuffer(self))
	return NULL;

    if (!PyArg_ParseTuple(args, "nn", &start, &end))
	return NULL;

    return RangeNew(self->buf, start, end);
}

    static PyObject *
BufferRepr(BufferObject *self)
{
    if (self->buf == INVALID_BUFFER_VALUE)
	return PyString_FromFormat("<buffer object (deleted) at %p>", self);
    else
    {
	char	*name = (char *)self->buf->b_fname;

	if (name == NULL)
	    name = "";

	return PyString_FromFormat("<buffer %s>", name);
    }
}

static struct PyMethodDef BufferMethods[] = {
    /* name,	    function,			calling,	documentation */
    {"append",	    (PyCFunction)BufferAppend,	METH_VARARGS,	"Append data to Vim buffer" },
    {"mark",	    (PyCFunction)BufferMark,	METH_VARARGS,	"Return (row,col) representing position of named mark" },
    {"range",	    (PyCFunction)BufferRange,	METH_VARARGS,	"Return a range object which represents the part of the given buffer between line numbers s and e" },
    {"__dir__",	    (PyCFunction)BufferDir,	METH_NOARGS,	""},
    { NULL,	    NULL,			0,		NULL}
};

/*
 * Buffer list object - Implementation
 */

static PyTypeObject BufMapType;

typedef struct
{
    PyObject_HEAD
} BufMapObject;

    static PyInt
BufMapLength(PyObject *self UNUSED)
{
    buf_T	*b = firstbuf;
    PyInt	n = 0;

    while (b)
    {
	++n;
	b = b->b_next;
    }

    return n;
}

    static PyObject *
BufMapItem(PyObject *self UNUSED, PyObject *keyObject)
{
    buf_T	*b;
    int		bnr;

#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(keyObject))
	bnr = PyInt_AsLong(keyObject);
    else
#endif
    if (PyLong_Check(keyObject))
	bnr = PyLong_AsLong(keyObject);
    else
    {
	PyErr_SetString(PyExc_TypeError, _("key must be integer"));
	return NULL;
    }

    b = buflist_findnr(bnr);

    if (b)
	return BufferNew(b);
    else
    {
	PyErr_SetObject(PyExc_KeyError, keyObject);
	return NULL;
    }
}

    static void
BufMapIterDestruct(PyObject *buffer)
{
    /* Iteration was stopped before all buffers were processed */
    if (buffer)
    {
	Py_DECREF(buffer);
    }
}

    static int
BufMapIterTraverse(PyObject *buffer, visitproc visit, void *arg)
{
    if (buffer)
	Py_VISIT(buffer);
    return 0;
}

    static int
BufMapIterClear(PyObject **buffer)
{
    if (*buffer)
	Py_CLEAR(*buffer);
    return 0;
}

    static PyObject *
BufMapIterNext(PyObject **buffer)
{
    PyObject	*next;
    PyObject	*r;

    if (!*buffer)
	return NULL;

    r = *buffer;

    if (CheckBuffer((BufferObject *)(r)))
    {
	*buffer = NULL;
	return NULL;
    }

    if (!((BufferObject *)(r))->buf->b_next)
	next = NULL;
    else if (!(next = BufferNew(((BufferObject *)(r))->buf->b_next)))
	return NULL;
    *buffer = next;
    /* Do not increment reference: we no longer hold it (decref), but whoever
     * on other side will hold (incref). Decref+incref = nothing. */
    return r;
}

    static PyObject *
BufMapIter(PyObject *self UNUSED)
{
    PyObject *buffer;

    buffer = BufferNew(firstbuf);
    return IterNew(buffer,
	    (destructorfun) BufMapIterDestruct, (nextfun) BufMapIterNext,
	    (traversefun) BufMapIterTraverse, (clearfun) BufMapIterClear);
}

static PyMappingMethods BufMapAsMapping = {
    (lenfunc)       BufMapLength,
    (binaryfunc)    BufMapItem,
    (objobjargproc) 0,
};

/* Current items object
 */

static char *CurrentAttrs[] = {
    "buffer", "window", "line", "range", "tabpage",
    NULL
};

    static PyObject *
CurrentDir(PyObject *self)
{
    return ObjectDir(self, CurrentAttrs);
}

    static PyObject *
CurrentGetattr(PyObject *self UNUSED, char *name)
{
    if (strcmp(name, "buffer") == 0)
	return (PyObject *)BufferNew(curbuf);
    else if (strcmp(name, "window") == 0)
	return (PyObject *)WindowNew(curwin, curtab);
    else if (strcmp(name, "tabpage") == 0)
	return (PyObject *)TabPageNew(curtab);
    else if (strcmp(name, "line") == 0)
	return GetBufferLine(curbuf, (PyInt)curwin->w_cursor.lnum);
    else if (strcmp(name, "range") == 0)
	return RangeNew(curbuf, RangeStart, RangeEnd);
    else if (strcmp(name, "__members__") == 0)
	return ObjectDir(NULL, CurrentAttrs);
    else
#if PY_MAJOR_VERSION < 3
	return Py_FindMethod(WindowMethods, self, name);
#else
	return NULL;
#endif
}

    static int
CurrentSetattr(PyObject *self UNUSED, char *name, PyObject *value)
{
    if (strcmp(name, "line") == 0)
    {
	if (SetBufferLine(curbuf, (PyInt)curwin->w_cursor.lnum, value, NULL) == FAIL)
	    return -1;

	return 0;
    }
    else if (strcmp(name, "buffer") == 0)
    {
	int count;

	if (value->ob_type != &BufferType)
	{
	    PyErr_SetString(PyExc_TypeError, _("expected vim.Buffer object"));
	    return -1;
	}

	if (CheckBuffer((BufferObject *)(value)))
	    return -1;
	count = ((BufferObject *)(value))->buf->b_fnum;

	VimTryStart();
	if (do_buffer(DOBUF_GOTO, DOBUF_FIRST, FORWARD, count, 0) == FAIL)
	{
	    if (VimTryEnd())
		return -1;
	    PyErr_SetVim(_("failed to switch to given buffer"));
	    return -1;
	}

	return VimTryEnd();
    }
    else if (strcmp(name, "window") == 0)
    {
	int count;

	if (value->ob_type != &WindowType)
	{
	    PyErr_SetString(PyExc_TypeError, _("expected vim.Window object"));
	    return -1;
	}

	if (CheckWindow((WindowObject *)(value)))
	    return -1;
	count = get_win_number(((WindowObject *)(value))->win, firstwin);

	if (!count)
	{
	    PyErr_SetString(PyExc_ValueError,
		    _("failed to find window in the current tab page"));
	    return -1;
	}

	VimTryStart();
	win_goto(((WindowObject *)(value))->win);
	if (((WindowObject *)(value))->win != curwin)
	{
	    if (VimTryEnd())
		return -1;
	    PyErr_SetString(PyExc_RuntimeError,
		    _("did not switch to the specified window"));
	    return -1;
	}

	return VimTryEnd();
    }
    else if (strcmp(name, "tabpage") == 0)
    {
	if (value->ob_type != &TabPageType)
	{
	    PyErr_SetString(PyExc_TypeError, _("expected vim.TabPage object"));
	    return -1;
	}

	if (CheckTabPage((TabPageObject *)(value)))
	    return -1;

	VimTryStart();
	goto_tabpage_tp(((TabPageObject *)(value))->tab, TRUE, TRUE);
	if (((TabPageObject *)(value))->tab != curtab)
	{
	    if (VimTryEnd())
		return -1;
	    PyErr_SetString(PyExc_RuntimeError,
		    _("did not switch to the specified tab page"));
	    return -1;
	}

	return VimTryEnd();
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
    }
}

static struct PyMethodDef CurrentMethods[] = {
    /* name,	    function,			calling,	documentation */
    {"__dir__",	    (PyCFunction)CurrentDir,	METH_NOARGS,	""},
    { NULL,	    NULL,			0,		NULL}
};

    static void
init_range_cmd(exarg_T *eap)
{
    RangeStart = eap->line1;
    RangeEnd = eap->line2;
}

    static void
init_range_eval(typval_T *rettv UNUSED)
{
    RangeStart = (PyInt) curwin->w_cursor.lnum;
    RangeEnd = RangeStart;
}

    static void
run_cmd(const char *cmd, void *arg UNUSED
#ifdef PY_CAN_RECURSE
	, PyGILState_STATE *pygilstate UNUSED
#endif
	)
{
    PyRun_SimpleString((char *) cmd);
}

static const char	*code_hdr = "def " DOPY_FUNC "(line, linenr):\n ";
static int		code_hdr_len = 30;

    static void
run_do(const char *cmd, void *arg UNUSED
#ifdef PY_CAN_RECURSE
	, PyGILState_STATE *pygilstate
#endif
	)
{
    PyInt	lnum;
    size_t	len;
    char	*code;
    int		status;
    PyObject	*pyfunc, *pymain;

    if (u_save((linenr_T)RangeStart - 1, (linenr_T)RangeEnd + 1) != OK)
    {
	EMSG(_("cannot save undo information"));
	return;
    }

    len = code_hdr_len + STRLEN(cmd);
    code = PyMem_New(char, len + 1);
    memcpy(code, code_hdr, code_hdr_len);
    STRCPY(code + code_hdr_len, cmd);
    status = PyRun_SimpleString(code);
    PyMem_Free(code);

    if (status)
    {
	EMSG(_("failed to run the code"));
	return;
    }

    status = 0;
    pymain = PyImport_AddModule("__main__");
    pyfunc = PyObject_GetAttrString(pymain, DOPY_FUNC);
#ifdef PY_CAN_RECURSE
    PyGILState_Release(*pygilstate);
#endif

    for (lnum = RangeStart; lnum <= RangeEnd; ++lnum)
    {
	PyObject	*line, *linenr, *ret;

#ifdef PY_CAN_RECURSE
	*pygilstate = PyGILState_Ensure();
#endif
	if (!(line = GetBufferLine(curbuf, lnum)))
	    goto err;
	if (!(linenr = PyInt_FromLong((long) lnum)))
	{
	    Py_DECREF(line);
	    goto err;
	}
	ret = PyObject_CallFunctionObjArgs(pyfunc, line, linenr, NULL);
	Py_DECREF(line);
	Py_DECREF(linenr);
	if (!ret)
	    goto err;

	if (ret != Py_None)
	    if (SetBufferLine(curbuf, lnum, ret, NULL) == FAIL)
		goto err;

	Py_XDECREF(ret);
	PythonIO_Flush();
#ifdef PY_CAN_RECURSE
	PyGILState_Release(*pygilstate);
#endif
    }
    goto out;
err:
#ifdef PY_CAN_RECURSE
    *pygilstate = PyGILState_Ensure();
#endif
    PyErr_PrintEx(0);
    PythonIO_Flush();
    status = 1;
out:
#ifdef PY_CAN_RECURSE
    if (!status)
	*pygilstate = PyGILState_Ensure();
#endif
    Py_DECREF(pyfunc);
    PyObject_SetAttrString(pymain, DOPY_FUNC, NULL);
    if (status)
	return;
    check_cursor();
    update_curbuf(NOT_VALID);
}

    static void
run_eval(const char *cmd, typval_T *rettv
#ifdef PY_CAN_RECURSE
	, PyGILState_STATE *pygilstate UNUSED
#endif
	)
{
    PyObject	*r;

    r = PyRun_String((char *) cmd, Py_eval_input, globals, globals);
    if (r == NULL)
    {
	if (PyErr_Occurred() && !msg_silent)
	    PyErr_PrintEx(0);
	EMSG(_("E858: Eval did not return a valid python object"));
    }
    else
    {
	if (ConvertFromPyObject(r, rettv) == -1)
	    EMSG(_("E859: Failed to convert returned python object to vim value"));
	Py_DECREF(r);
    }
    PyErr_Clear();
}

    static void
set_ref_in_py(const int copyID)
{
    pylinkedlist_T	*cur;
    dict_T	*dd;
    list_T	*ll;

    if (lastdict != NULL)
	for(cur = lastdict ; cur != NULL ; cur = cur->pll_prev)
	{
	    dd = ((DictionaryObject *) (cur->pll_obj))->dict;
	    if (dd->dv_copyID != copyID)
	    {
		dd->dv_copyID = copyID;
		set_ref_in_ht(&dd->dv_hashtab, copyID);
	    }
	}

    if (lastlist != NULL)
	for(cur = lastlist ; cur != NULL ; cur = cur->pll_prev)
	{
	    ll = ((ListObject *) (cur->pll_obj))->list;
	    if (ll->lv_copyID != copyID)
	    {
		ll->lv_copyID = copyID;
		set_ref_in_list(ll, copyID);
	    }
	}
}

    static int
set_string_copy(char_u *str, typval_T *tv)
{
    tv->vval.v_string = vim_strsave(str);
    if (tv->vval.v_string == NULL)
    {
	PyErr_NoMemory();
	return -1;
    }
    return 0;
}

    static int
pydict_to_tv(PyObject *obj, typval_T *tv, PyObject *lookup_dict)
{
    dict_T	*dict;
    char_u	*key;
    dictitem_T	*di;
    PyObject	*keyObject;
    PyObject	*valObject;
    Py_ssize_t	iter = 0;

    if (!(dict = py_dict_alloc()))
	return -1;

    tv->v_type = VAR_DICT;
    tv->vval.v_dict = dict;

    while (PyDict_Next(obj, &iter, &keyObject, &valObject))
    {
	PyObject	*todecref = NULL;

	if (keyObject == NULL || valObject == NULL)
	{
	    dict_unref(dict);
	    return -1;
	}

	if (!(key = StringToChars(keyObject, &todecref)))
	{
	    dict_unref(dict);
	    return -1;
	}

	if (*key == NUL)
	{
	    dict_unref(dict);
	    Py_XDECREF(todecref);
	    RAISE_NO_EMPTY_KEYS;
	    return -1;
	}

	di = dictitem_alloc(key);

	Py_XDECREF(todecref);

	if (di == NULL)
	{
	    PyErr_NoMemory();
	    dict_unref(dict);
	    return -1;
	}
	di->di_tv.v_lock = 0;

	if (_ConvertFromPyObject(valObject, &di->di_tv, lookup_dict) == -1)
	{
	    vim_free(di);
	    dict_unref(dict);
	    return -1;
	}

	if (dict_add(dict, di) == FAIL)
	{
	    clear_tv(&di->di_tv);
	    vim_free(di);
	    dict_unref(dict);
	    PyErr_SetVim(_("failed to add key to dictionary"));
	    return -1;
	}
    }

    --dict->dv_refcount;
    return 0;
}

    static int
pymap_to_tv(PyObject *obj, typval_T *tv, PyObject *lookup_dict)
{
    dict_T	*dict;
    char_u	*key;
    dictitem_T	*di;
    PyObject	*list;
    PyObject	*iterator;
    PyObject	*keyObject;
    PyObject	*valObject;

    if (!(dict = py_dict_alloc()))
	return -1;

    tv->v_type = VAR_DICT;
    tv->vval.v_dict = dict;

    if (!(list = PyMapping_Keys(obj)))
    {
	dict_unref(dict);
	return -1;
    }

    if (!(iterator = PyObject_GetIter(list)))
    {
	dict_unref(dict);
	Py_DECREF(list);
	return -1;
    }
    Py_DECREF(list);

    while ((keyObject = PyIter_Next(iterator)))
    {
	PyObject	*todecref;

	if (!(key = StringToChars(keyObject, &todecref)))
	{
	    Py_DECREF(keyObject);
	    Py_DECREF(iterator);
	    dict_unref(dict);
	    return -1;
	}

	if (*key == NUL)
	{
	    Py_DECREF(keyObject);
	    Py_DECREF(iterator);
	    Py_XDECREF(todecref);
	    dict_unref(dict);
	    RAISE_NO_EMPTY_KEYS;
	    return -1;
	}

	if (!(valObject = PyObject_GetItem(obj, keyObject)))
	{
	    Py_DECREF(keyObject);
	    Py_DECREF(iterator);
	    Py_XDECREF(todecref);
	    dict_unref(dict);
	    return -1;
	}

	di = dictitem_alloc(key);

	Py_DECREF(keyObject);
	Py_XDECREF(todecref);

	if (di == NULL)
	{
	    Py_DECREF(iterator);
	    Py_DECREF(valObject);
	    dict_unref(dict);
	    PyErr_NoMemory();
	    return -1;
	}
	di->di_tv.v_lock = 0;

	if (_ConvertFromPyObject(valObject, &di->di_tv, lookup_dict) == -1)
	{
	    Py_DECREF(iterator);
	    Py_DECREF(valObject);
	    vim_free(di);
	    dict_unref(dict);
	    return -1;
	}

	Py_DECREF(valObject);

	if (dict_add(dict, di) == FAIL)
	{
	    Py_DECREF(iterator);
	    dictitem_free(di);
	    dict_unref(dict);
	    PyErr_SetVim(_("failed to add key to dictionary"));
	    return -1;
	}
    }
    Py_DECREF(iterator);
    --dict->dv_refcount;
    return 0;
}

    static int
pyseq_to_tv(PyObject *obj, typval_T *tv, PyObject *lookup_dict)
{
    list_T	*l;

    if (!(l = py_list_alloc()))
	return -1;

    tv->v_type = VAR_LIST;
    tv->vval.v_list = l;

    if (list_py_concat(l, obj, lookup_dict) == -1)
    {
	list_unref(l);
	return -1;
    }

    --l->lv_refcount;
    return 0;
}

typedef int (*pytotvfunc)(PyObject *, typval_T *, PyObject *);

    static int
convert_dl(PyObject *obj, typval_T *tv,
				    pytotvfunc py_to_tv, PyObject *lookup_dict)
{
    PyObject	*capsule;
    char	hexBuf[sizeof(void *) * 2 + 3];

    sprintf(hexBuf, "%p", obj);

# ifdef PY_USE_CAPSULE
    capsule = PyDict_GetItemString(lookup_dict, hexBuf);
# else
    capsule = (PyObject *)PyDict_GetItemString(lookup_dict, hexBuf);
# endif
    if (capsule == NULL)
    {
# ifdef PY_USE_CAPSULE
	capsule = PyCapsule_New(tv, NULL, NULL);
# else
	capsule = PyCObject_FromVoidPtr(tv, NULL);
# endif
	if (PyDict_SetItemString(lookup_dict, hexBuf, capsule))
	{
	    Py_DECREF(capsule);
	    tv->v_type = VAR_UNKNOWN;
	    return -1;
	}
	if (py_to_tv(obj, tv, lookup_dict) == -1)
	{
	    tv->v_type = VAR_UNKNOWN;
	    return -1;
	}
	/* As we are not using copy_tv which increments reference count we must
	 * do it ourself. */
	switch(tv->v_type)
	{
	    case VAR_DICT: ++tv->vval.v_dict->dv_refcount; break;
	    case VAR_LIST: ++tv->vval.v_list->lv_refcount; break;
	}
    }
    else
    {
	typval_T	*v;

# ifdef PY_USE_CAPSULE
	v = PyCapsule_GetPointer(capsule, NULL);
# else
	v = PyCObject_AsVoidPtr(capsule);
# endif
	copy_tv(v, tv);
    }
    return 0;
}

    static int
ConvertFromPyMapping(PyObject *obj, typval_T *tv)
{
    PyObject	*lookup_dict;
    int		r;

    if (!(lookup_dict = PyDict_New()))
	return -1;

    if (PyType_IsSubtype(obj->ob_type, &DictionaryType))
    {
	tv->v_type = VAR_DICT;
	tv->vval.v_dict = (((DictionaryObject *)(obj))->dict);
	++tv->vval.v_dict->dv_refcount;
	r = 0;
    }
    else if (PyDict_Check(obj))
	r = convert_dl(obj, tv, pydict_to_tv, lookup_dict);
    else if (PyMapping_Check(obj))
	r = convert_dl(obj, tv, pymap_to_tv, lookup_dict);
    else
    {
	PyErr_SetString(PyExc_TypeError,
		_("unable to convert object to vim dictionary"));
	r = -1;
    }
    Py_DECREF(lookup_dict);
    return r;
}

    static int
ConvertFromPyObject(PyObject *obj, typval_T *tv)
{
    PyObject	*lookup_dict;
    int		r;

    if (!(lookup_dict = PyDict_New()))
	return -1;
    r = _ConvertFromPyObject(obj, tv, lookup_dict);
    Py_DECREF(lookup_dict);
    return r;
}

    static int
_ConvertFromPyObject(PyObject *obj, typval_T *tv, PyObject *lookup_dict)
{
    if (PyType_IsSubtype(obj->ob_type, &DictionaryType))
    {
	tv->v_type = VAR_DICT;
	tv->vval.v_dict = (((DictionaryObject *)(obj))->dict);
	++tv->vval.v_dict->dv_refcount;
    }
    else if (obj->ob_type == &ListType)
    {
	tv->v_type = VAR_LIST;
	tv->vval.v_list = (((ListObject *)(obj))->list);
	++tv->vval.v_list->lv_refcount;
    }
    else if (obj->ob_type == &FunctionType)
    {
	if (set_string_copy(((FunctionObject *) (obj))->name, tv) == -1)
	    return -1;

	tv->v_type = VAR_FUNC;
	func_ref(tv->vval.v_string);
    }
    else if (PyBytes_Check(obj))
    {
	char_u	*result;

	if (PyString_AsStringAndSize(obj, (char **) &result, NULL) == -1)
	    return -1;
	if (result == NULL)
	    return -1;

	if (set_string_copy(result, tv) == -1)
	    return -1;

	tv->v_type = VAR_STRING;
    }
    else if (PyUnicode_Check(obj))
    {
	PyObject	*bytes;
	char_u	*result;

	bytes = PyUnicode_AsEncodedString(obj, (char *)ENC_OPT, NULL);
	if (bytes == NULL)
	    return -1;

	if(PyString_AsStringAndSize(bytes, (char **) &result, NULL) == -1)
	    return -1;
	if (result == NULL)
	    return -1;

	if (set_string_copy(result, tv))
	{
	    Py_XDECREF(bytes);
	    return -1;
	}
	Py_XDECREF(bytes);

	tv->v_type = VAR_STRING;
    }
#if PY_MAJOR_VERSION < 3
    else if (PyInt_Check(obj))
    {
	tv->v_type = VAR_NUMBER;
	tv->vval.v_number = (varnumber_T) PyInt_AsLong(obj);
    }
#endif
    else if (PyLong_Check(obj))
    {
	tv->v_type = VAR_NUMBER;
	tv->vval.v_number = (varnumber_T) PyLong_AsLong(obj);
    }
    else if (PyDict_Check(obj))
	return convert_dl(obj, tv, pydict_to_tv, lookup_dict);
#ifdef FEAT_FLOAT
    else if (PyFloat_Check(obj))
    {
	tv->v_type = VAR_FLOAT;
	tv->vval.v_float = (float_T) PyFloat_AsDouble(obj);
    }
#endif
    else if (PyObject_HasAttrString(obj, "keys"))
	return convert_dl(obj, tv, pymap_to_tv, lookup_dict);
    else if (PyIter_Check(obj) || PySequence_Check(obj))
	return convert_dl(obj, tv, pyseq_to_tv, lookup_dict);
    else if (PyMapping_Check(obj))
	return convert_dl(obj, tv, pymap_to_tv, lookup_dict);
    else
    {
	PyErr_SetString(PyExc_TypeError,
		_("unable to convert to vim structure"));
	return -1;
    }
    return 0;
}

    static PyObject *
ConvertToPyObject(typval_T *tv)
{
    if (tv == NULL)
    {
	PyErr_SetVim(_("NULL reference passed"));
	return NULL;
    }
    switch (tv->v_type)
    {
	case VAR_STRING:
	    return PyBytes_FromString(tv->vval.v_string == NULL
					    ? "" : (char *)tv->vval.v_string);
	case VAR_NUMBER:
	    return PyLong_FromLong((long) tv->vval.v_number);
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    return PyFloat_FromDouble((double) tv->vval.v_float);
#endif
	case VAR_LIST:
	    return NEW_LIST(tv->vval.v_list);
	case VAR_DICT:
	    return NEW_DICTIONARY(tv->vval.v_dict);
	case VAR_FUNC:
	    return NEW_FUNCTION(tv->vval.v_string == NULL
					  ? (char_u *)"" : tv->vval.v_string);
	case VAR_UNKNOWN:
	    Py_INCREF(Py_None);
	    return Py_None;
	default:
	    PyErr_SetVim(_("internal error: invalid value type"));
	    return NULL;
    }
}

typedef struct
{
    PyObject_HEAD
} CurrentObject;
static PyTypeObject CurrentType;

#if PY_MAJOR_VERSION >= 3
typedef struct
{
    PyObject_HEAD
} FinderObject;
static PyTypeObject FinderType;
#endif

    static void
init_structs(void)
{
    vim_memset(&OutputType, 0, sizeof(OutputType));
    OutputType.tp_name = "vim.message";
    OutputType.tp_basicsize = sizeof(OutputObject);
    OutputType.tp_flags = Py_TPFLAGS_DEFAULT;
    OutputType.tp_doc = "vim message object";
    OutputType.tp_methods = OutputMethods;
#if PY_MAJOR_VERSION >= 3
    OutputType.tp_getattro = (getattrofunc)OutputGetattro;
    OutputType.tp_setattro = (setattrofunc)OutputSetattro;
    OutputType.tp_alloc = call_PyType_GenericAlloc;
    OutputType.tp_new = call_PyType_GenericNew;
    OutputType.tp_free = call_PyObject_Free;
#else
    OutputType.tp_getattr = (getattrfunc)OutputGetattr;
    OutputType.tp_setattr = (setattrfunc)OutputSetattr;
#endif

    vim_memset(&IterType, 0, sizeof(IterType));
    IterType.tp_name = "vim.iter";
    IterType.tp_basicsize = sizeof(IterObject);
    IterType.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC;
    IterType.tp_doc = "generic iterator object";
    IterType.tp_iter = (getiterfunc)IterIter;
    IterType.tp_iternext = (iternextfunc)IterNext;
    IterType.tp_dealloc = (destructor)IterDestructor;
    IterType.tp_traverse = (traverseproc)IterTraverse;
    IterType.tp_clear = (inquiry)IterClear;

    vim_memset(&BufferType, 0, sizeof(BufferType));
    BufferType.tp_name = "vim.buffer";
    BufferType.tp_basicsize = sizeof(BufferType);
    BufferType.tp_dealloc = (destructor)BufferDestructor;
    BufferType.tp_repr = (reprfunc)BufferRepr;
    BufferType.tp_as_sequence = &BufferAsSeq;
    BufferType.tp_as_mapping = &BufferAsMapping;
    BufferType.tp_flags = Py_TPFLAGS_DEFAULT;
    BufferType.tp_doc = "vim buffer object";
    BufferType.tp_methods = BufferMethods;
#if PY_MAJOR_VERSION >= 3
    BufferType.tp_getattro = (getattrofunc)BufferGetattro;
    BufferType.tp_setattro = (setattrofunc)BufferSetattro;
    BufferType.tp_alloc = call_PyType_GenericAlloc;
    BufferType.tp_new = call_PyType_GenericNew;
    BufferType.tp_free = call_PyObject_Free;
#else
    BufferType.tp_getattr = (getattrfunc)BufferGetattr;
    BufferType.tp_setattr = (setattrfunc)BufferSetattr;
#endif

    vim_memset(&WindowType, 0, sizeof(WindowType));
    WindowType.tp_name = "vim.window";
    WindowType.tp_basicsize = sizeof(WindowObject);
    WindowType.tp_dealloc = (destructor)WindowDestructor;
    WindowType.tp_repr = (reprfunc)WindowRepr;
    WindowType.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC;
    WindowType.tp_doc = "vim Window object";
    WindowType.tp_methods = WindowMethods;
    WindowType.tp_traverse = (traverseproc)WindowTraverse;
    WindowType.tp_clear = (inquiry)WindowClear;
#if PY_MAJOR_VERSION >= 3
    WindowType.tp_getattro = (getattrofunc)WindowGetattro;
    WindowType.tp_setattro = (setattrofunc)WindowSetattro;
    WindowType.tp_alloc = call_PyType_GenericAlloc;
    WindowType.tp_new = call_PyType_GenericNew;
    WindowType.tp_free = call_PyObject_Free;
#else
    WindowType.tp_getattr = (getattrfunc)WindowGetattr;
    WindowType.tp_setattr = (setattrfunc)WindowSetattr;
#endif

    vim_memset(&TabPageType, 0, sizeof(TabPageType));
    TabPageType.tp_name = "vim.tabpage";
    TabPageType.tp_basicsize = sizeof(TabPageObject);
    TabPageType.tp_dealloc = (destructor)TabPageDestructor;
    TabPageType.tp_repr = (reprfunc)TabPageRepr;
    TabPageType.tp_flags = Py_TPFLAGS_DEFAULT;
    TabPageType.tp_doc = "vim tab page object";
    TabPageType.tp_methods = TabPageMethods;
#if PY_MAJOR_VERSION >= 3
    TabPageType.tp_getattro = (getattrofunc)TabPageGetattro;
    TabPageType.tp_alloc = call_PyType_GenericAlloc;
    TabPageType.tp_new = call_PyType_GenericNew;
    TabPageType.tp_free = call_PyObject_Free;
#else
    TabPageType.tp_getattr = (getattrfunc)TabPageGetattr;
#endif

    vim_memset(&BufMapType, 0, sizeof(BufMapType));
    BufMapType.tp_name = "vim.bufferlist";
    BufMapType.tp_basicsize = sizeof(BufMapObject);
    BufMapType.tp_as_mapping = &BufMapAsMapping;
    BufMapType.tp_flags = Py_TPFLAGS_DEFAULT;
    BufMapType.tp_iter = BufMapIter;
    BufferType.tp_doc = "vim buffer list";

    vim_memset(&WinListType, 0, sizeof(WinListType));
    WinListType.tp_name = "vim.windowlist";
    WinListType.tp_basicsize = sizeof(WinListType);
    WinListType.tp_as_sequence = &WinListAsSeq;
    WinListType.tp_flags = Py_TPFLAGS_DEFAULT;
    WinListType.tp_doc = "vim window list";
    WinListType.tp_dealloc = (destructor)WinListDestructor;

    vim_memset(&TabListType, 0, sizeof(TabListType));
    TabListType.tp_name = "vim.tabpagelist";
    TabListType.tp_basicsize = sizeof(TabListType);
    TabListType.tp_as_sequence = &TabListAsSeq;
    TabListType.tp_flags = Py_TPFLAGS_DEFAULT;
    TabListType.tp_doc = "vim tab page list";

    vim_memset(&RangeType, 0, sizeof(RangeType));
    RangeType.tp_name = "vim.range";
    RangeType.tp_basicsize = sizeof(RangeObject);
    RangeType.tp_dealloc = (destructor)RangeDestructor;
    RangeType.tp_repr = (reprfunc)RangeRepr;
    RangeType.tp_as_sequence = &RangeAsSeq;
    RangeType.tp_as_mapping = &RangeAsMapping;
    RangeType.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC;
    RangeType.tp_doc = "vim Range object";
    RangeType.tp_methods = RangeMethods;
    RangeType.tp_traverse = (traverseproc)RangeTraverse;
    RangeType.tp_clear = (inquiry)RangeClear;
#if PY_MAJOR_VERSION >= 3
    RangeType.tp_getattro = (getattrofunc)RangeGetattro;
    RangeType.tp_alloc = call_PyType_GenericAlloc;
    RangeType.tp_new = call_PyType_GenericNew;
    RangeType.tp_free = call_PyObject_Free;
#else
    RangeType.tp_getattr = (getattrfunc)RangeGetattr;
#endif

    vim_memset(&CurrentType, 0, sizeof(CurrentType));
    CurrentType.tp_name = "vim.currentdata";
    CurrentType.tp_basicsize = sizeof(CurrentObject);
    CurrentType.tp_flags = Py_TPFLAGS_DEFAULT;
    CurrentType.tp_doc = "vim current object";
    CurrentType.tp_methods = CurrentMethods;
#if PY_MAJOR_VERSION >= 3
    CurrentType.tp_getattro = (getattrofunc)CurrentGetattro;
    CurrentType.tp_setattro = (setattrofunc)CurrentSetattro;
#else
    CurrentType.tp_getattr = (getattrfunc)CurrentGetattr;
    CurrentType.tp_setattr = (setattrfunc)CurrentSetattr;
#endif

    vim_memset(&DictionaryType, 0, sizeof(DictionaryType));
    DictionaryType.tp_name = "vim.dictionary";
    DictionaryType.tp_basicsize = sizeof(DictionaryObject);
    DictionaryType.tp_dealloc = (destructor)DictionaryDestructor;
    DictionaryType.tp_as_sequence = &DictionaryAsSeq;
    DictionaryType.tp_as_mapping = &DictionaryAsMapping;
    DictionaryType.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE;
    DictionaryType.tp_doc = "dictionary pushing modifications to vim structure";
    DictionaryType.tp_methods = DictionaryMethods;
    DictionaryType.tp_iter = (getiterfunc)DictionaryIter;
    DictionaryType.tp_new = (newfunc)DictionaryConstructor;
    DictionaryType.tp_alloc = (allocfunc)PyType_GenericAlloc;
#if PY_MAJOR_VERSION >= 3
    DictionaryType.tp_getattro = (getattrofunc)DictionaryGetattro;
    DictionaryType.tp_setattro = (setattrofunc)DictionarySetattro;
#else
    DictionaryType.tp_getattr = (getattrfunc)DictionaryGetattr;
    DictionaryType.tp_setattr = (setattrfunc)DictionarySetattr;
#endif

    vim_memset(&ListType, 0, sizeof(ListType));
    ListType.tp_name = "vim.list";
    ListType.tp_dealloc = (destructor)ListDestructor;
    ListType.tp_basicsize = sizeof(ListObject);
    ListType.tp_as_sequence = &ListAsSeq;
    ListType.tp_as_mapping = &ListAsMapping;
    ListType.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE;
    ListType.tp_doc = "list pushing modifications to vim structure";
    ListType.tp_methods = ListMethods;
    ListType.tp_iter = (getiterfunc)ListIter;
    ListType.tp_new = (newfunc)ListConstructor;
    ListType.tp_alloc = (allocfunc)PyType_GenericAlloc;
#if PY_MAJOR_VERSION >= 3
    ListType.tp_getattro = (getattrofunc)ListGetattro;
    ListType.tp_setattro = (setattrofunc)ListSetattro;
#else
    ListType.tp_getattr = (getattrfunc)ListGetattr;
    ListType.tp_setattr = (setattrfunc)ListSetattr;
#endif

    vim_memset(&FunctionType, 0, sizeof(FunctionType));
    FunctionType.tp_name = "vim.function";
    FunctionType.tp_basicsize = sizeof(FunctionObject);
    FunctionType.tp_dealloc = (destructor)FunctionDestructor;
    FunctionType.tp_call = (ternaryfunc)FunctionCall;
    FunctionType.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE;
    FunctionType.tp_doc = "object that calls vim function";
    FunctionType.tp_methods = FunctionMethods;
    FunctionType.tp_repr = (reprfunc)FunctionRepr;
    FunctionType.tp_new = (newfunc)FunctionConstructor;
    FunctionType.tp_alloc = (allocfunc)PyType_GenericAlloc;
#if PY_MAJOR_VERSION >= 3
    FunctionType.tp_getattro = (getattrofunc)FunctionGetattro;
#else
    FunctionType.tp_getattr = (getattrfunc)FunctionGetattr;
#endif

    vim_memset(&OptionsType, 0, sizeof(OptionsType));
    OptionsType.tp_name = "vim.options";
    OptionsType.tp_basicsize = sizeof(OptionsObject);
    OptionsType.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC;
    OptionsType.tp_doc = "object for manipulating options";
    OptionsType.tp_as_mapping = &OptionsAsMapping;
    OptionsType.tp_dealloc = (destructor)OptionsDestructor;
    OptionsType.tp_traverse = (traverseproc)OptionsTraverse;
    OptionsType.tp_clear = (inquiry)OptionsClear;

#if PY_MAJOR_VERSION >= 3
    vim_memset(&vimmodule, 0, sizeof(vimmodule));
    vimmodule.m_name = "vim";
    vimmodule.m_doc = "Vim Python interface\n";
    vimmodule.m_size = -1;
    vimmodule.m_methods = VimMethods;
#endif
}

#define PYTYPE_READY(type) \
    if (PyType_Ready(&type)) \
	return -1;

    static int
init_types()
{
    PYTYPE_READY(IterType);
    PYTYPE_READY(BufferType);
    PYTYPE_READY(RangeType);
    PYTYPE_READY(WindowType);
    PYTYPE_READY(TabPageType);
    PYTYPE_READY(BufMapType);
    PYTYPE_READY(WinListType);
    PYTYPE_READY(TabListType);
    PYTYPE_READY(CurrentType);
    PYTYPE_READY(DictionaryType);
    PYTYPE_READY(ListType);
    PYTYPE_READY(FunctionType);
    PYTYPE_READY(OptionsType);
    PYTYPE_READY(OutputType);
#if PY_MAJOR_VERSION >= 3
    PYTYPE_READY(FinderType);
#endif
    return 0;
}

    static int
init_sys_path()
{
    PyObject	*path;
    PyObject	*path_hook;
    PyObject	*path_hooks;

    if (!(path_hook = PyObject_GetAttrString(vim_module, "path_hook")))
	return -1;

    if (!(path_hooks = PySys_GetObject("path_hooks")))
    {
	PyErr_Clear();
	path_hooks = PyList_New(1);
	PyList_SET_ITEM(path_hooks, 0, path_hook);
	if (PySys_SetObject("path_hooks", path_hooks))
	{
	    Py_DECREF(path_hooks);
	    return -1;
	}
	Py_DECREF(path_hooks);
    }
    else if (PyList_Check(path_hooks))
    {
	if (PyList_Append(path_hooks, path_hook))
	{
	    Py_DECREF(path_hook);
	    return -1;
	}
	Py_DECREF(path_hook);
    }
    else
    {
	VimTryStart();
	EMSG(_("Failed to set path hook: sys.path_hooks is not a list\n"
	       "You should now do the following:\n"
	       "- append vim.path_hook to sys.path_hooks\n"
	       "- append vim.VIM_SPECIAL_PATH to sys.path\n"));
	VimTryEnd(); /* Discard the error */
	Py_DECREF(path_hook);
	return 0;
    }

    if (!(path = PySys_GetObject("path")))
    {
	PyErr_Clear();
	path = PyList_New(1);
	Py_INCREF(vim_special_path_object);
	PyList_SET_ITEM(path, 0, vim_special_path_object);
	if (PySys_SetObject("path", path))
	{
	    Py_DECREF(path);
	    return -1;
	}
	Py_DECREF(path);
    }
    else if (PyList_Check(path))
    {
	if (PyList_Append(path, vim_special_path_object))
	    return -1;
    }
    else
    {
	VimTryStart();
	EMSG(_("Failed to set path: sys.path is not a list\n"
	       "You should now append vim.VIM_SPECIAL_PATH to sys.path"));
	VimTryEnd(); /* Discard the error */
    }

    return 0;
}

static BufMapObject TheBufferMap =
{
    PyObject_HEAD_INIT(&BufMapType)
};

static WinListObject TheWindowList =
{
    PyObject_HEAD_INIT(&WinListType)
    NULL
};

static CurrentObject TheCurrent =
{
    PyObject_HEAD_INIT(&CurrentType)
};

static TabListObject TheTabPageList =
{
    PyObject_HEAD_INIT(&TabListType)
};

static struct numeric_constant {
    char	*name;
    int		value;
} numeric_constants[] = {
    {"VAR_LOCKED",	VAR_LOCKED},
    {"VAR_FIXED",	VAR_FIXED},
    {"VAR_SCOPE",	VAR_SCOPE},
    {"VAR_DEF_SCOPE",	VAR_DEF_SCOPE},
};

static struct object_constant {
    char	*name;
    PyObject	*value;
} object_constants[] = {
    {"buffers",  (PyObject *)(void *)&TheBufferMap},
    {"windows",  (PyObject *)(void *)&TheWindowList},
    {"tabpages", (PyObject *)(void *)&TheTabPageList},
    {"current",  (PyObject *)(void *)&TheCurrent},

    {"Buffer",     (PyObject *)&BufferType},
    {"Range",      (PyObject *)&RangeType},
    {"Window",     (PyObject *)&WindowType},
    {"TabPage",    (PyObject *)&TabPageType},
    {"Dictionary", (PyObject *)&DictionaryType},
    {"List",       (PyObject *)&ListType},
    {"Function",   (PyObject *)&FunctionType},
    {"Options",    (PyObject *)&OptionsType},
#if PY_MAJOR_VERSION >= 3
    {"Finder",     (PyObject *)&FinderType},
#endif
};

typedef int (*object_adder)(PyObject *, const char *, PyObject *);
typedef PyObject *(*attr_getter)(PyObject *, const char *);

#define ADD_OBJECT(m, name, obj) \
    if (add_object(m, name, obj)) \
	return -1;

#define ADD_CHECKED_OBJECT(m, name, obj) \
    { \
	PyObject	*value = obj; \
	if (!value) \
	    return -1; \
	ADD_OBJECT(m, name, value); \
    }

    static int
populate_module(PyObject *m, object_adder add_object, attr_getter get_attr)
{
    int		i;
    PyObject	*other_module;
    PyObject	*attr;

    for (i = 0; i < (int)(sizeof(numeric_constants)
					   / sizeof(struct numeric_constant));
	    ++i)
	ADD_CHECKED_OBJECT(m, numeric_constants[i].name,
		PyInt_FromLong(numeric_constants[i].value));

    for (i = 0; i < (int)(sizeof(object_constants)
					    / sizeof(struct object_constant));
	    ++i)
    {
	PyObject	*value;

	value = object_constants[i].value;
	Py_INCREF(value);
	ADD_OBJECT(m, object_constants[i].name, value);
    }

    if (!(VimError = PyErr_NewException("vim.error", NULL, NULL)))
	return -1;
    ADD_OBJECT(m, "error", VimError);

    ADD_CHECKED_OBJECT(m, "vars",  NEW_DICTIONARY(&globvardict));
    ADD_CHECKED_OBJECT(m, "vvars", NEW_DICTIONARY(&vimvardict));
    ADD_CHECKED_OBJECT(m, "options",
	    OptionsNew(SREQ_GLOBAL, NULL, dummy_check, NULL));

    if (!(other_module = PyImport_ImportModule("os")))
	return -1;
    ADD_OBJECT(m, "os", other_module);

    if (!(py_getcwd = PyObject_GetAttrString(other_module, "getcwd")))
	return -1;
    ADD_OBJECT(m, "_getcwd", py_getcwd)

    if (!(py_chdir = PyObject_GetAttrString(other_module, "chdir")))
	return -1;
    ADD_OBJECT(m, "_chdir", py_chdir);
    if (!(attr = get_attr(m, "chdir")))
	return -1;
    if (PyObject_SetAttrString(other_module, "chdir", attr))
    {
	Py_DECREF(attr);
	return -1;
    }
    Py_DECREF(attr);

    if ((py_fchdir = PyObject_GetAttrString(other_module, "fchdir")))
    {
	ADD_OBJECT(m, "_fchdir", py_fchdir);
	if (!(attr = get_attr(m, "fchdir")))
	    return -1;
	if (PyObject_SetAttrString(other_module, "fchdir", attr))
	{
	    Py_DECREF(attr);
	    return -1;
	}
	Py_DECREF(attr);
    }
    else
	PyErr_Clear();

    if (!(vim_special_path_object = PyString_FromString(vim_special_path)))
	return -1;

    ADD_OBJECT(m, "VIM_SPECIAL_PATH", vim_special_path_object);

#if PY_MAJOR_VERSION >= 3
    ADD_OBJECT(m, "_PathFinder", path_finder);
    ADD_CHECKED_OBJECT(m, "_find_module",
	    (py_find_module = PyObject_GetAttrString(path_finder,
						     "find_module")));
#endif

    return 0;
}
