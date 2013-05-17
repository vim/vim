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

#define PyErr_SetVim(str) PyErr_SetString(VimError, str)

#define INVALID_BUFFER_VALUE ((buf_T *)(-1))
#define INVALID_WINDOW_VALUE ((win_T *)(-1))
#define INVALID_TABPAGE_VALUE ((tabpage_T *)(-1))

static int ConvertFromPyObject(PyObject *, typval_T *);
static int _ConvertFromPyObject(PyObject *, typval_T *, PyObject *);
static PyObject *WindowNew(win_T *, tabpage_T *);
static PyObject *BufferNew (buf_T *);
static PyObject *LineToString(const char *);

static PyInt RangeStart;
static PyInt RangeEnd;

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

    static int
OutputSetattr(PyObject *self, char *name, PyObject *val)
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

	((OutputObject *)(self))->softspace = PyInt_AsLong(val);
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
OutputWrite(PyObject *self, PyObject *args)
{
    Py_ssize_t len = 0;
    char *str = NULL;
    int error = ((OutputObject *)(self))->error;

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
OutputWritelines(PyObject *self, PyObject *args)
{
    PyInt n;
    PyInt i;
    PyObject *list;
    int error = ((OutputObject *)(self))->error;

    if (!PyArg_ParseTuple(args, "O", &list))
	return NULL;
    Py_INCREF(list);

    if (!PyList_Check(list))
    {
	PyErr_SetString(PyExc_TypeError, _("writelines() requires list of strings"));
	Py_DECREF(list);
	return NULL;
    }

    n = PyList_Size(list);

    for (i = 0; i < n; ++i)
    {
	PyObject *line = PyList_GetItem(list, i);
	char *str = NULL;
	PyInt len;

	if (!PyArg_Parse(line, "et#", ENC_OPT, &str, &len))
	{
	    PyErr_SetString(PyExc_TypeError, _("writelines() requires list of strings"));
	    Py_DECREF(list);
	    return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	Python_Lock_Vim();
	writer((writefn)(error ? emsg : msg), (char_u *)str, len);
	Python_Release_Vim();
	Py_END_ALLOW_THREADS
	PyMem_Free(str);
    }

    Py_DECREF(list);
    Py_INCREF(Py_None);
    return Py_None;
}

    static PyObject *
OutputFlush(PyObject *self UNUSED, PyObject *args UNUSED)
{
    /* do nothing */
    Py_INCREF(Py_None);
    return Py_None;
}

/***************/

static struct PyMethodDef OutputMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"write",	    OutputWrite,	1,	    ""},
    {"writelines",  OutputWritelines,	1,	    ""},
    {"flush",	    OutputFlush,	1,	    ""},
    { NULL,	    NULL,		0,	    NULL}
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
    PySys_SetObject("stdout", (PyObject *)(void *)&Output);
    PySys_SetObject("stderr", (PyObject *)(void *)&Error);

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
    static int
VimErrorCheck(void)
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

/*
 * Function to translate a typval_T into a PyObject; this will recursively
 * translate lists/dictionaries into their Python equivalents.
 *
 * The depth parameter is to avoid infinite recursion, set it to 1 when
 * you call VimToPython.
 */
    static PyObject *
VimToPython(typval_T *our_tv, int depth, PyObject *lookupDict)
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

    /* Check if we run into a recursive loop.  The item must be in lookupDict
     * then and we can use it again. */
    if ((our_tv->v_type == VAR_LIST && our_tv->vval.v_list != NULL)
	    || (our_tv->v_type == VAR_DICT && our_tv->vval.v_dict != NULL))
    {
	sprintf(ptrBuf, "%p",
		our_tv->v_type == VAR_LIST ? (void *)our_tv->vval.v_list
					   : (void *)our_tv->vval.v_dict);
	result = PyDict_GetItemString(lookupDict, ptrBuf);
	if (result != NULL)
	{
	    Py_INCREF(result);
	    return result;
	}
    }

    if (our_tv->v_type == VAR_STRING)
    {
	result = Py_BuildValue("s", our_tv->vval.v_string == NULL
					? "" : (char *)our_tv->vval.v_string);
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
	    hashtab_T	*ht = &our_tv->vval.v_dict->dv_hashtab;
	    long_u	todo = ht->ht_used;
	    hashitem_T	*hi;
	    dictitem_T	*di;

	    PyDict_SetItemString(lookupDict, ptrBuf, result);

	    for (hi = ht->ht_array; todo > 0; ++hi)
	    {
		if (!HASHITEM_EMPTY(hi))
		{
		    --todo;

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

    static PyObject *
VimEval(PyObject *self UNUSED, PyObject *args UNUSED)
{
    char	*expr;
    typval_T	*our_tv;
    PyObject	*result;
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
    our_tv = eval_expr((char_u *)expr, NULL);

    Python_Release_Vim();
    Py_END_ALLOW_THREADS

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

/*
 * Vim module - Definitions
 */

static struct PyMethodDef VimMethods[] = {
    /* name,	     function,		calling,    documentation */
    {"command",	     VimCommand,	1,	    "Execute a Vim ex-mode command" },
    {"eval",	     VimEval,		1,	    "Evaluate an expression using Vim evaluator" },
    {"bindeval",     VimEvalPy,		1,	    "Like eval(), but returns objects attached to vim ones"},
    {"strwidth",     VimStrwidth,	1,	    "Screen string width, counts <Tab> as having width 1"},
    { NULL,	     NULL,		0,	    NULL }
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

    self = PyObject_NEW(IterObject, &IterType);
    self->cur = start;
    self->next = next;
    self->destruct = destruct;
    self->traverse = traverse;
    self->clear = clear;

    return (PyObject *)(self);
}

    static void
IterDestructor(PyObject *self)
{
    IterObject *this = (IterObject *)(self);

    this->destruct(this->cur);

    DESTRUCTOR_FINISH(self);
}

    static int
IterTraverse(PyObject *self, visitproc visit, void *arg)
{
    IterObject *this = (IterObject *)(self);

    if (this->traverse != NULL)
	return this->traverse(this->cur, visit, arg);
    else
	return 0;
}

/* Mac OSX defines clear() somewhere. */
#ifdef clear
# undef clear
#endif

    static int
IterClear(PyObject *self)
{
    IterObject *this = (IterObject *)(self);

    if (this->clear != NULL)
	return this->clear(&this->cur);
    else
	return 0;
}

    static PyObject *
IterNext(PyObject *self)
{
    IterObject *this = (IterObject *)(self);

    return this->next(&this->cur);
}

    static PyObject *
IterIter(PyObject *self)
{
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

#define DICTKEY_GET_NOTEMPTY(err) \
    DICTKEY_GET(err) \
    if (*key == NUL) \
    { \
	PyErr_SetString(PyExc_ValueError, _("empty keys are not allowed")); \
	return err; \
    }

typedef struct
{
    PyObject_HEAD
    dict_T	*dict;
    pylinkedlist_T	ref;
} DictionaryObject;

    static PyObject *
DictionaryNew(dict_T *dict)
{
    DictionaryObject	*self;

    self = PyObject_NEW(DictionaryObject, &DictionaryType);
    if (self == NULL)
	return NULL;
    self->dict = dict;
    ++dict->dv_refcount;

    pyll_add((PyObject *)(self), &self->ref, &lastdict);

    return (PyObject *)(self);
}

    static void
DictionaryDestructor(PyObject *self)
{
    DictionaryObject	*this = ((DictionaryObject *) (self));

    pyll_remove(&this->ref, &lastdict);
    dict_unref(this->dict);

    DESTRUCTOR_FINISH(self);
}

    static int
DictionarySetattr(PyObject *self, char *name, PyObject *val)
{
    DictionaryObject *this = (DictionaryObject *)(self);

    if (val == NULL)
    {
	PyErr_SetString(PyExc_AttributeError, _("Cannot delete DictionaryObject attributes"));
	return -1;
    }

    if (strcmp(name, "locked") == 0)
    {
	if (this->dict->dv_lock == VAR_FIXED)
	{
	    PyErr_SetString(PyExc_TypeError, _("Cannot modify fixed dictionary"));
	    return -1;
	}
	else
	{
	    int		istrue = PyObject_IsTrue(val);
	    if (istrue == -1)
		return -1;
	    else if (istrue)
		this->dict->dv_lock = VAR_LOCKED;
	    else
		this->dict->dv_lock = 0;
	}
	return 0;
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, _("Cannot set this attribute"));
	return -1;
    }
}

    static PyInt
DictionaryLength(PyObject *self)
{
    return ((PyInt) ((((DictionaryObject *)(self))->dict->dv_hashtab.ht_used)));
}

    static PyObject *
DictionaryItem(PyObject *self, PyObject *keyObject)
{
    char_u	*key;
    dictitem_T	*di;
    DICTKEY_DECL

    DICTKEY_GET_NOTEMPTY(NULL)

    di = dict_find(((DictionaryObject *) (self))->dict, key, -1);

    DICTKEY_UNREF

    if (di == NULL)
    {
	PyErr_SetObject(PyExc_KeyError, keyObject);
	return NULL;
    }

    return ConvertToPyObject(&di->di_tv);
}

    static PyInt
DictionaryAssItem(PyObject *self, PyObject *keyObject, PyObject *valObject)
{
    char_u	*key;
    typval_T	tv;
    dict_T	*d = ((DictionaryObject *)(self))->dict;
    dictitem_T	*di;
    DICTKEY_DECL

    if (d->dv_lock)
    {
	PyErr_SetVim(_("dict is locked"));
	return -1;
    }

    DICTKEY_GET_NOTEMPTY(-1)

    di = dict_find(d, key, -1);

    if (valObject == NULL)
    {
	hashitem_T	*hi;

	if (di == NULL)
	{
	    DICTKEY_UNREF
	    PyErr_SetObject(PyExc_KeyError, keyObject);
	    return -1;
	}
	hi = hash_find(&d->dv_hashtab, di->di_key);
	hash_remove(&d->dv_hashtab, hi);
	dictitem_free(di);
	return 0;
    }

    if (ConvertFromPyObject(valObject, &tv) == -1)
	return -1;

    if (di == NULL)
    {
	di = dictitem_alloc(key);
	if (di == NULL)
	{
	    PyErr_NoMemory();
	    return -1;
	}
	di->di_tv.v_lock = 0;

	if (dict_add(d, di) == FAIL)
	{
	    DICTKEY_UNREF
	    vim_free(di);
	    PyErr_SetVim(_("failed to add key to dictionary"));
	    return -1;
	}
    }
    else
	clear_tv(&di->di_tv);

    DICTKEY_UNREF

    copy_tv(&tv, &di->di_tv);
    return 0;
}

    static PyObject *
DictionaryListKeys(PyObject *self UNUSED)
{
    dict_T	*dict = ((DictionaryObject *)(self))->dict;
    long_u	todo = dict->dv_hashtab.ht_used;
    Py_ssize_t	i = 0;
    PyObject	*r;
    hashitem_T	*hi;

    r = PyList_New(todo);
    for (hi = dict->dv_hashtab.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    PyList_SetItem(r, i, PyBytes_FromString((char *)(hi->hi_key)));
	    --todo;
	    ++i;
	}
    }
    return r;
}

static PyMappingMethods DictionaryAsMapping = {
    (lenfunc)       DictionaryLength,
    (binaryfunc)    DictionaryItem,
    (objobjargproc) DictionaryAssItem,
};

static struct PyMethodDef DictionaryMethods[] = {
    {"keys", (PyCFunction)DictionaryListKeys, METH_NOARGS, ""},
    { NULL,	    NULL,		0,	    NULL }
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

    static PyObject *
ListNew(list_T *list)
{
    ListObject	*self;

    self = PyObject_NEW(ListObject, &ListType);
    if (self == NULL)
	return NULL;
    self->list = list;
    ++list->lv_refcount;

    pyll_add((PyObject *)(self), &self->ref, &lastlist);

    return (PyObject *)(self);
}

    static void
ListDestructor(PyObject *self)
{
    ListObject *this = (ListObject *)(self);

    pyll_remove(&this->ref, &lastlist);
    list_unref(this->list);

    DESTRUCTOR_FINISH(self);
}

    static int
list_py_concat(list_T *l, PyObject *obj, PyObject *lookupDict)
{
    Py_ssize_t	i;
    Py_ssize_t	lsize = PySequence_Size(obj);
    PyObject	*litem;
    listitem_T	*li;

    for(i=0; i<lsize; i++)
    {
	li = listitem_alloc();
	if (li == NULL)
	{
	    PyErr_NoMemory();
	    return -1;
	}
	li->li_tv.v_lock = 0;

	litem = PySequence_GetItem(obj, i);
	if (litem == NULL)
	    return -1;
	if (_ConvertFromPyObject(litem, &li->li_tv, lookupDict) == -1)
	    return -1;

	list_append(l, li);
    }
    return 0;
}

    static PyInt
ListLength(PyObject *self)
{
    return ((PyInt) (((ListObject *) (self))->list->lv_len));
}

    static PyObject *
ListItem(PyObject *self, Py_ssize_t index)
{
    listitem_T	*li;

    if (index>=ListLength(self))
    {
	PyErr_SetString(PyExc_IndexError, _("list index out of range"));
	return NULL;
    }
    li = list_find(((ListObject *) (self))->list, (long) index);
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
ListSlice(PyObject *self, Py_ssize_t first, Py_ssize_t last)
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

	if ((PyList_SetItem(list, ((reversed)?(n-i-1):(i)), item)))
	{
	    Py_DECREF(item);
	    Py_DECREF(list);
	    return NULL;
	}
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
ListIter(PyObject *self)
{
    listiterinfo_T	*lii;
    list_T	*l = ((ListObject *) (self))->list;

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
ListAssItem(PyObject *self, Py_ssize_t index, PyObject *obj)
{
    typval_T	tv;
    list_T	*l = ((ListObject *) (self))->list;
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
	    PyErr_SetVim(_("Failed to add item to list"));
	    return -1;
	}
    }
    else
    {
	li = list_find(l, (long) index);
	clear_tv(&li->li_tv);
	copy_tv(&tv, &li->li_tv);
    }
    return 0;
}

    static int
ListAssSlice(PyObject *self, Py_ssize_t first, Py_ssize_t last, PyObject *obj)
{
    PyInt	size = ListLength(self);
    Py_ssize_t	i;
    Py_ssize_t	lsize;
    PyObject	*litem;
    listitem_T	*li;
    listitem_T	*next;
    typval_T	v;
    list_T	*l = ((ListObject *) (self))->list;

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

    if (!PyList_Check(obj))
    {
	PyErr_SetString(PyExc_TypeError, _("can only assign lists to slice"));
	return -1;
    }

    lsize = PyList_Size(obj);

    for(i=0; i<lsize; i++)
    {
	litem = PyList_GetItem(obj, i);
	if (litem == NULL)
	    return -1;
	if (ConvertFromPyObject(litem, &v) == -1)
	    return -1;
	if (list_insert_tv(l, &v, li) == FAIL)
	{
	    PyErr_SetVim(_("internal error: failed to add item to list"));
	    return -1;
	}
    }
    return 0;
}

    static PyObject *
ListConcatInPlace(PyObject *self, PyObject *obj)
{
    list_T	*l = ((ListObject *) (self))->list;
    PyObject	*lookup_dict;

    if (l->lv_lock)
    {
	PyErr_SetVim(_("list is locked"));
	return NULL;
    }

    if (!PySequence_Check(obj))
    {
	PyErr_SetString(PyExc_TypeError, _("can only concatenate with lists"));
	return NULL;
    }

    lookup_dict = PyDict_New();
    if (list_py_concat(l, obj, lookup_dict) == -1)
    {
	Py_DECREF(lookup_dict);
	return NULL;
    }
    Py_DECREF(lookup_dict);

    Py_INCREF(self);
    return self;
}

    static int
ListSetattr(PyObject *self, char *name, PyObject *val)
{
    ListObject *this = (ListObject *)(self);

    if (val == NULL)
    {
	PyErr_SetString(PyExc_AttributeError,
		_("cannot delete vim.dictionary attributes"));
	return -1;
    }

    if (strcmp(name, "locked") == 0)
    {
	if (this->list->lv_lock == VAR_FIXED)
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
		this->list->lv_lock = VAR_LOCKED;
	    else
		this->list->lv_lock = 0;
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
    {"extend", (PyCFunction)ListConcatInPlace, METH_O, ""},
    { NULL,	    NULL,		0,	    NULL }
};

typedef struct
{
    PyObject_HEAD
    char_u	*name;
} FunctionObject;

static PyTypeObject FunctionType;

    static PyObject *
FunctionNew(char_u *name)
{
    FunctionObject	*self;

    self = PyObject_NEW(FunctionObject, &FunctionType);
    if (self == NULL)
	return NULL;
    self->name = PyMem_New(char_u, STRLEN(name) + 1);
    if (self->name == NULL)
    {
	PyErr_NoMemory();
	return NULL;
    }
    STRCPY(self->name, name);
    func_ref(name);
    return (PyObject *)(self);
}

    static void
FunctionDestructor(PyObject *self)
{
    FunctionObject	*this = (FunctionObject *) (self);

    func_unref(this->name);
    PyMem_Del(this->name);

    DESTRUCTOR_FINISH(self);
}

    static PyObject *
FunctionCall(PyObject *self, PyObject *argsObject, PyObject *kwargs)
{
    FunctionObject	*this = (FunctionObject *)(self);
    char_u	*name = this->name;
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
	    if (!PyMapping_Check(selfdictObject))
	    {
		PyErr_SetString(PyExc_TypeError,
				   _("'self' argument must be a dictionary"));
		clear_tv(&args);
		return NULL;
	    }
	    if (ConvertFromPyObject(selfdictObject, &selfdicttv) == -1)
		return NULL;
	    selfdict = selfdicttv.vval.v_dict;
	}
    }

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();

    error = func_call(name, &args, selfdict, &rettv);

    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (error != OK)
    {
	result = NULL;
	PyErr_SetVim(_("failed to run function"));
    }
    else
	result = ConvertToPyObject(&rettv);

    /* FIXME Check what should really be cleared. */
    clear_tv(&args);
    clear_tv(&rettv);
    /*
     * if (selfdict!=NULL)
     *     clear_tv(selfdicttv);
     */

    return result;
}

static struct PyMethodDef FunctionMethods[] = {
    {"__call__",    (PyCFunction)FunctionCall, METH_VARARGS|METH_KEYWORDS, ""},
    { NULL,	    NULL,		0,	    NULL }
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

    self = PyObject_NEW(OptionsObject, &OptionsType);
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
OptionsDestructor(PyObject *self)
{
    if (((OptionsObject *)(self))->fromObj)
	Py_DECREF(((OptionsObject *)(self))->fromObj);
    DESTRUCTOR_FINISH(self);
}

    static int
OptionsTraverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((OptionsObject *)(self))->fromObj);
    return 0;
}

    static int
OptionsClear(PyObject *self)
{
    Py_CLEAR(((OptionsObject *)(self))->fromObj);
    return 0;
}

    static PyObject *
OptionsItem(OptionsObject *this, PyObject *keyObject)
{
    char_u	*key;
    int		flags;
    long	numval;
    char_u	*stringval;
    DICTKEY_DECL

    if (this->Check(this->from))
	return NULL;

    DICTKEY_GET_NOTEMPTY(NULL)

    flags = get_option_value_strict(key, &numval, &stringval,
				    this->opt_type, this->from);

    DICTKEY_UNREF

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
	PyObject *r;
	r = numval ? Py_True : Py_False;
	Py_INCREF(r);
	return r;
    }
    else if (flags & SOPT_NUM)
	return PyInt_FromLong(numval);
    else if (flags & SOPT_STRING)
    {
	if (stringval)
	    return PyBytes_FromString((char *) stringval);
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
set_option_value_for(key, numval, stringval, opt_flags, opt_type, from)
    char_u	*key;
    int		numval;
    char_u	*stringval;
    int		opt_flags;
    int		opt_type;
    void	*from;
{
    win_T	*save_curwin;
    tabpage_T	*save_curtab;
    buf_T	*save_curbuf;
    int		r = 0;

    switch (opt_type)
    {
	case SREQ_WIN:
	    if (switch_win(&save_curwin, &save_curtab, (win_T *)from,
				     win_find_tabpage((win_T *)from)) == FAIL)
	    {
		PyErr_SetVim("Problem while switching windows.");
		return -1;
	    }
	    set_option_value(key, numval, stringval, opt_flags);
	    restore_win(save_curwin, save_curtab);
	    break;
	case SREQ_BUF:
	    switch_buffer(&save_curbuf, (buf_T *)from);
	    set_option_value(key, numval, stringval, opt_flags);
	    restore_buffer(save_curbuf);
	    break;
	case SREQ_GLOBAL:
	    set_option_value(key, numval, stringval, opt_flags);
	    break;
    }
    return r;
}

    static int
OptionsAssItem(OptionsObject *this, PyObject *keyObject, PyObject *valObject)
{
    char_u	*key;
    int		flags;
    int		opt_flags;
    int		r = 0;
    DICTKEY_DECL

    if (this->Check(this->from))
	return -1;

    DICTKEY_GET_NOTEMPTY(-1)

    flags = get_option_value_strict(key, NULL, NULL,
				    this->opt_type, this->from);

    DICTKEY_UNREF

    if (flags == 0)
    {
	PyErr_SetObject(PyExc_KeyError, keyObject);
	return -1;
    }

    if (valObject == NULL)
    {
	if (this->opt_type == SREQ_GLOBAL)
	{
	    PyErr_SetString(PyExc_ValueError,
		    _("unable to unset global option"));
	    return -1;
	}
	else if (!(flags & SOPT_GLOBAL))
	{
	    PyErr_SetString(PyExc_ValueError, _("unable to unset option "
						"without global value"));
	    return -1;
	}
	else
	{
	    unset_global_local_option(key, this->from);
	    return 0;
	}
    }

    opt_flags = (this->opt_type ? OPT_LOCAL : OPT_GLOBAL);

    if (flags & SOPT_BOOL)
    {
	int	istrue = PyObject_IsTrue(valObject);
	if (istrue == -1)
	    return -1;
	r = set_option_value_for(key, istrue, NULL,
				opt_flags, this->opt_type, this->from);
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
	    return -1;
	}

	r = set_option_value_for(key, val, NULL, opt_flags,
				this->opt_type, this->from);
    }
    else
    {
	char_u	*val;
	if (PyBytes_Check(valObject))
	{

	    if (PyString_AsStringAndSize(valObject, (char **) &val, NULL) == -1)
		return -1;
	    if (val == NULL)
		return -1;

	    val = vim_strsave(val);
	}
	else if (PyUnicode_Check(valObject))
	{
	    PyObject	*bytes;

	    bytes = PyUnicode_AsEncodedString(valObject, (char *)ENC_OPT, NULL);
	    if (bytes == NULL)
		return -1;

	    if(PyString_AsStringAndSize(bytes, (char **) &val, NULL) == -1)
		return -1;
	    if (val == NULL)
		return -1;

	    val = vim_strsave(val);
	    Py_XDECREF(bytes);
	}
	else
	{
	    PyErr_SetString(PyExc_TypeError, _("object must be string"));
	    return -1;
	}

	r = set_option_value_for(key, 0, val, opt_flags,
				this->opt_type, this->from);
	vim_free(val);
    }

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
CheckTabPage(TabPageObject *this)
{
    if (this->tab == INVALID_TABPAGE_VALUE)
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
TabPageDestructor(PyObject *self)
{
    TabPageObject *this = (TabPageObject *)(self);

    if (this->tab && this->tab != INVALID_TABPAGE_VALUE)
	TAB_PYTHON_REF(this->tab) = NULL;

    DESTRUCTOR_FINISH(self);
}

    static PyObject *
TabPageAttr(TabPageObject *this, char *name)
{
    if (strcmp(name, "windows") == 0)
	return WinListNew(this);
    else if (strcmp(name, "number") == 0)
	return PyLong_FromLong((long) get_tab_number(this->tab));
    else if (strcmp(name, "vars") == 0)
	return DictionaryNew(this->tab->tp_vars);
    else if (strcmp(name, "window") == 0)
    {
	/* For current tab window.c does not bother to set or update tp_curwin
	 */
	if (this->tab == curtab)
	    return WindowNew(curwin, curtab);
	else
	    return WindowNew(this->tab->tp_curwin, this->tab);
    }
    return NULL;
}

    static PyObject *
TabPageRepr(PyObject *self)
{
    static char repr[100];
    TabPageObject *this = (TabPageObject *)(self);

    if (this->tab == INVALID_TABPAGE_VALUE)
    {
	vim_snprintf(repr, 100, _("<tabpage object (deleted) at %p>"), (self));
	return PyString_FromString(repr);
    }
    else
    {
	int	t = get_tab_number(this->tab);

	if (t == 0)
	    vim_snprintf(repr, 100, _("<tabpage object (unknown) at %p>"),
								      (self));
	else
	    vim_snprintf(repr, 100, _("<tabpage %d>"), t - 1);

	return PyString_FromString(repr);
    }
}

static struct PyMethodDef TabPageMethods[] = {
    /* name,	    function,		calling,    documentation */
    { NULL,	    NULL,		0,	    NULL }
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
CheckWindow(WindowObject *this)
{
    if (this->win == INVALID_WINDOW_VALUE)
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
	self = PyObject_NEW(WindowObject, &WindowType);
	if (self == NULL)
	    return NULL;
	self->win = win;
	WIN_PYTHON_REF(win) = self;
    }

    self->tabObject = ((TabPageObject *)(TabPageNew(tab)));

    return (PyObject *)(self);
}

    static void
WindowDestructor(PyObject *self)
{
    WindowObject *this = (WindowObject *)(self);

    if (this->win && this->win != INVALID_WINDOW_VALUE)
	WIN_PYTHON_REF(this->win) = NULL;

    Py_DECREF(((PyObject *)(this->tabObject)));

    DESTRUCTOR_FINISH(self);
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
    static int
WindowTraverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((PyObject *)(((WindowObject *)(self))->tabObject)));
    return 0;
}

    static int
WindowClear(PyObject *self)
{
    Py_CLEAR((((WindowObject *)(self))->tabObject));
    return 0;
}

    static PyObject *
WindowAttr(WindowObject *this, char *name)
{
    if (strcmp(name, "buffer") == 0)
	return (PyObject *)BufferNew(this->win->w_buffer);
    else if (strcmp(name, "cursor") == 0)
    {
	pos_T *pos = &this->win->w_cursor;

	return Py_BuildValue("(ll)", (long)(pos->lnum), (long)(pos->col));
    }
    else if (strcmp(name, "height") == 0)
	return PyLong_FromLong((long)(this->win->w_height));
#ifdef FEAT_WINDOWS
    else if (strcmp(name, "row") == 0)
	return PyLong_FromLong((long)(this->win->w_winrow));
#endif
#ifdef FEAT_VERTSPLIT
    else if (strcmp(name, "width") == 0)
	return PyLong_FromLong((long)(W_WIDTH(this->win)));
    else if (strcmp(name, "col") == 0)
	return PyLong_FromLong((long)(W_WINCOL(this->win)));
#endif
    else if (strcmp(name, "vars") == 0)
	return DictionaryNew(this->win->w_vars);
    else if (strcmp(name, "options") == 0)
	return OptionsNew(SREQ_WIN, this->win, (checkfun) CheckWindow,
			(PyObject *) this);
    else if (strcmp(name, "number") == 0)
    {
	if (CheckTabPage(this->tabObject))
	    return NULL;
	return PyLong_FromLong((long)
		get_win_number(this->win, get_firstwin(this->tabObject)));
    }
    else if (strcmp(name, "tabpage") == 0)
    {
	Py_INCREF(this->tabObject);
	return (PyObject *)(this->tabObject);
    }
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[sssssssss]", "buffer", "cursor", "height",
		"vars", "options", "number", "row", "col", "tabpage");
    else
	return NULL;
}

    static int
WindowSetattr(PyObject *self, char *name, PyObject *val)
{
    WindowObject *this = (WindowObject *)(self);

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

	this->win->w_cursor.lnum = lnum;
	this->win->w_cursor.col = col;
#ifdef FEAT_VIRTUALEDIT
	this->win->w_cursor.coladd = 0;
#endif
	/* When column is out of range silently correct it. */
	check_cursor_col_win(this->win);

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
	int	width;
	win_T	*savewin;

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

    static PyObject *
WindowRepr(PyObject *self)
{
    static char repr[100];
    WindowObject *this = (WindowObject *)(self);

    if (this->win == INVALID_WINDOW_VALUE)
    {
	vim_snprintf(repr, 100, _("<window object (deleted) at %p>"), (self));
	return PyString_FromString(repr);
    }
    else
    {
	int	w = get_win_number(this->win, firstwin);

	if (w == 0)
	    vim_snprintf(repr, 100, _("<window object (unknown) at %p>"),
								      (self));
	else
	    vim_snprintf(repr, 100, _("<window %d>"), w - 1);

	return PyString_FromString(repr);
    }
}

static struct PyMethodDef WindowMethods[] = {
    /* name,	    function,		calling,    documentation */
    { NULL,	    NULL,		0,	    NULL }
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
WinListDestructor(PyObject *self)
{
    TabPageObject	*tabObject = ((WinListObject *)(self))->tabObject;

    if (tabObject)
	Py_DECREF((PyObject *)(tabObject));

    DESTRUCTOR_FINISH(self);
}

    static PyInt
WinListLength(PyObject *self)
{
    win_T	*w;
    PyInt	n = 0;

    if (!(w = get_firstwin(((WinListObject *)(self))->tabObject)))
	return -1;

    while (w != NULL)
    {
	++n;
	w = W_NEXT(w);
    }

    return n;
}

    static PyObject *
WinListItem(PyObject *self, PyInt n)
{
    WinListObject	*this = ((WinListObject *)(self));
    win_T *w;

    if (!(w = get_firstwin(this->tabObject)))
	return NULL;

    for (; w != NULL; w = W_NEXT(w), --n)
	if (n == 0)
	    return WindowNew(w, this->tabObject? this->tabObject->tab: curtab);

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

	if (PyErr_Occurred() || VimErrorCheck())
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

	if (PyErr_Occurred() || VimErrorCheck())
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
	    array = (char **)alloc((unsigned)(new_len * sizeof(char *)));
	    if (array == NULL)
	    {
		PyErr_NoMemory();
		return FAIL;
	    }
	}

	for (i = 0; i < new_len; ++i)
	{
	    PyObject *line = PyList_GetItem(list, i);

	    array[i] = StringToLine(line);
	    if (array[i] == NULL)
	    {
		while (i)
		    vim_free(array[--i]);
		vim_free(array);
		return FAIL;
	    }
	}

	PyErr_Clear();

	// START of region without "return".  Must call restore_buffer()!
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
	vim_free(array);

	/* Adjust marks. Invalidate any which lie in the
	 * changed range, and move any in the remainder of the buffer.
	 */
	mark_adjust((linenr_T)lo, (linenr_T)(hi - 1),
						  (long)MAXLNUM, (long)extra);
	changed_lines((linenr_T)lo, 0, (linenr_T)hi, (long)extra);

	if (buf == savebuf)
	    py_fix_cursor((linenr_T)lo, (linenr_T)hi, (linenr_T)extra);

	// END of region without "return".
	restore_buffer(savebuf);

	if (PyErr_Occurred() || VimErrorCheck())
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

	if (PyErr_Occurred() || VimErrorCheck())
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

	PyErr_Clear();
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
	vim_free(array);

	restore_buffer(savebuf);
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
CheckBuffer(BufferObject *this)
{
    if (this->buf == INVALID_BUFFER_VALUE)
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

    static void
RangeDestructor(PyObject *self)
{
    Py_DECREF(((RangeObject *)(self))->buf);
    DESTRUCTOR_FINISH(self);
}

    static PyInt
RangeLength(PyObject *self)
{
    /* HOW DO WE SIGNAL AN ERROR FROM THIS FUNCTION? */
    if (CheckBuffer(((RangeObject *)(self))->buf))
	return -1; /* ??? */

    return (((RangeObject *)(self))->end - ((RangeObject *)(self))->start + 1);
}

    static PyObject *
RangeItem(PyObject *self, PyInt n)
{
    return RBItem(((RangeObject *)(self))->buf, n,
		  ((RangeObject *)(self))->start,
		  ((RangeObject *)(self))->end);
}

    static PyObject *
RangeSlice(PyObject *self, PyInt lo, PyInt hi)
{
    return RBSlice(((RangeObject *)(self))->buf, lo, hi,
		   ((RangeObject *)(self))->start,
		   ((RangeObject *)(self))->end);
}

    static PyObject *
RangeAppend(PyObject *self, PyObject *args)
{
    return RBAppend(((RangeObject *)(self))->buf, args,
		    ((RangeObject *)(self))->start,
		    ((RangeObject *)(self))->end,
		    &((RangeObject *)(self))->end);
}

    static PyObject *
RangeRepr(PyObject *self)
{
    static char repr[100];
    RangeObject *this = (RangeObject *)(self);

    if (this->buf->buf == INVALID_BUFFER_VALUE)
    {
	vim_snprintf(repr, 100, "<range object (for deleted buffer) at %p>",
								      (self));
	return PyString_FromString(repr);
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

	return PyString_FromString(repr);
    }
}

static struct PyMethodDef RangeMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    RangeAppend,	1,	    "Append data to the Vim range" },
    { NULL,	    NULL,		0,	    NULL }
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
BufferDestructor(PyObject *self)
{
    BufferObject *this = (BufferObject *)(self);

    if (this->buf && this->buf != INVALID_BUFFER_VALUE)
	BUF_PYTHON_REF(this->buf) = NULL;

    DESTRUCTOR_FINISH(self);
}

    static PyInt
BufferLength(PyObject *self)
{
    /* HOW DO WE SIGNAL AN ERROR FROM THIS FUNCTION? */
    if (CheckBuffer((BufferObject *)(self)))
	return -1; /* ??? */

    return (PyInt)(((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

    static PyObject *
BufferItem(PyObject *self, PyInt n)
{
    return RBItem((BufferObject *)(self), n, 1, -1);
}

    static PyObject *
BufferSlice(PyObject *self, PyInt lo, PyInt hi)
{
    return RBSlice((BufferObject *)(self), lo, hi, 1, -1);
}

    static PyObject *
BufferAttr(BufferObject *this, char *name)
{
    if (strcmp(name, "name") == 0)
	return Py_BuildValue("s", this->buf->b_ffname);
    else if (strcmp(name, "number") == 0)
	return Py_BuildValue(Py_ssize_t_fmt, this->buf->b_fnum);
    else if (strcmp(name, "vars") == 0)
	return DictionaryNew(this->buf->b_vars);
    else if (strcmp(name, "options") == 0)
	return OptionsNew(SREQ_BUF, this->buf, (checkfun) CheckBuffer,
			(PyObject *) this);
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[ssss]", "name", "number", "vars", "options");
    else
	return NULL;
}

    static PyObject *
BufferAppend(PyObject *self, PyObject *args)
{
    return RBAppend((BufferObject *)(self), args, 1, -1, NULL);
}

    static PyObject *
BufferMark(PyObject *self, PyObject *args)
{
    pos_T	*posp;
    char	*pmark;
    char	mark;
    buf_T	*savebuf;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    if (!PyArg_ParseTuple(args, "s", &pmark))
	return NULL;
    mark = *pmark;

    switch_buffer(&savebuf, ((BufferObject *)(self))->buf);
    posp = getmark(mark, FALSE);
    restore_buffer(savebuf);

    if (posp == NULL)
    {
	PyErr_SetVim(_("invalid mark name"));
	return NULL;
    }

    /* Check for keyboard interrupt */
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

    static PyObject *
BufferRange(PyObject *self, PyObject *args)
{
    PyInt start;
    PyInt end;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    if (!PyArg_ParseTuple(args, "nn", &start, &end))
	return NULL;

    return RangeNew(((BufferObject *)(self))->buf, start, end);
}

    static PyObject *
BufferRepr(PyObject *self)
{
    static char repr[100];
    BufferObject *this = (BufferObject *)(self);

    if (this->buf == INVALID_BUFFER_VALUE)
    {
	vim_snprintf(repr, 100, _("<buffer object (deleted) at %p>"), (self));
	return PyString_FromString(repr);
    }
    else
    {
	char *name = (char *)this->buf->b_fname;
	PyInt len;

	if (name == NULL)
	    name = "";
	len = strlen(name);

	if (len > 35)
	    name = name + (35 - len);

	vim_snprintf(repr, 100, "<buffer %s%s>", len > 35 ? "..." : "", name);

	return PyString_FromString(repr);
    }
}

static struct PyMethodDef BufferMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    BufferAppend,	1,	    "Append data to Vim buffer" },
    {"mark",	    BufferMark,		1,	    "Return (row,col) representing position of named mark" },
    {"range",	    BufferRange,	1,	    "Return a range object which represents the part of the given buffer between line numbers s and e" },
#if PY_VERSION_HEX >= 0x03000000
    {"__dir__",	    BufferDir,		4,	    "List its attributes" },
#endif
    { NULL,	    NULL,		0,	    NULL }
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
    Py_VISIT(buffer);
    return 0;
}

    static int
BufMapIterClear(PyObject **buffer)
{
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
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[sssss]", "buffer", "window", "line", "range",
		"tabpage");
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
    }
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
	    PyErr_SetString(PyExc_TypeError, _("expected vim.buffer object"));
	    return -1;
	}

	if (CheckBuffer((BufferObject *)(value)))
	    return -1;
	count = ((BufferObject *)(value))->buf->b_fnum;

	if (do_buffer(DOBUF_GOTO, DOBUF_FIRST, FORWARD, count, 0) == FAIL)
	{
	    PyErr_SetVim(_("failed to switch to given buffer"));
	    return -1;
	}

	return 0;
    }
    else if (strcmp(name, "window") == 0)
    {
	int count;

	if (value->ob_type != &WindowType)
	{
	    PyErr_SetString(PyExc_TypeError, _("expected vim.window object"));
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

	win_goto(((WindowObject *)(value))->win);
	if (((WindowObject *)(value))->win != curwin)
	{
	    PyErr_SetString(PyExc_RuntimeError,
		    _("did not switch to the specified window"));
	    return -1;
	}

	return 0;
    }
    else if (strcmp(name, "tabpage") == 0)
    {
	if (value->ob_type != &TabPageType)
	{
	    PyErr_SetString(PyExc_TypeError, _("expected vim.tabpage object"));
	    return -1;
	}

	if (CheckTabPage((TabPageObject *)(value)))
	    return -1;

	goto_tabpage_tp(((TabPageObject *)(value))->tab, TRUE, TRUE);
	if (((TabPageObject *)(value))->tab != curtab)
	{
	    PyErr_SetString(PyExc_RuntimeError,
		    _("did not switch to the specified tab page"));
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
pydict_to_tv(PyObject *obj, typval_T *tv, PyObject *lookupDict)
{
    dict_T	*d;
    char_u	*key;
    dictitem_T	*di;
    PyObject	*keyObject;
    PyObject	*valObject;
    Py_ssize_t	iter = 0;

    d = dict_alloc();
    if (d == NULL)
    {
	PyErr_NoMemory();
	return -1;
    }

    tv->v_type = VAR_DICT;
    tv->vval.v_dict = d;

    while (PyDict_Next(obj, &iter, &keyObject, &valObject))
    {
	DICTKEY_DECL

	if (keyObject == NULL)
	    return -1;
	if (valObject == NULL)
	    return -1;

	DICTKEY_GET_NOTEMPTY(-1)

	di = dictitem_alloc(key);

	DICTKEY_UNREF

	if (di == NULL)
	{
	    PyErr_NoMemory();
	    return -1;
	}
	di->di_tv.v_lock = 0;

	if (_ConvertFromPyObject(valObject, &di->di_tv, lookupDict) == -1)
	{
	    vim_free(di);
	    return -1;
	}
	if (dict_add(d, di) == FAIL)
	{
	    vim_free(di);
	    PyErr_SetVim(_("failed to add key to dictionary"));
	    return -1;
	}
    }
    return 0;
}

    static int
pymap_to_tv(PyObject *obj, typval_T *tv, PyObject *lookupDict)
{
    dict_T	*d;
    char_u	*key;
    dictitem_T	*di;
    PyObject	*list;
    PyObject	*litem;
    PyObject	*keyObject;
    PyObject	*valObject;
    Py_ssize_t	lsize;

    d = dict_alloc();
    if (d == NULL)
    {
	PyErr_NoMemory();
	return -1;
    }

    tv->v_type = VAR_DICT;
    tv->vval.v_dict = d;

    list = PyMapping_Items(obj);
    if (list == NULL)
	return -1;
    lsize = PyList_Size(list);
    while (lsize--)
    {
	DICTKEY_DECL

	litem = PyList_GetItem(list, lsize);
	if (litem == NULL)
	{
	    Py_DECREF(list);
	    return -1;
	}

	keyObject = PyTuple_GetItem(litem, 0);
	if (keyObject == NULL)
	{
	    Py_DECREF(list);
	    Py_DECREF(litem);
	    return -1;
	}

	DICTKEY_GET_NOTEMPTY(-1)

	valObject = PyTuple_GetItem(litem, 1);
	if (valObject == NULL)
	{
	    Py_DECREF(list);
	    Py_DECREF(litem);
	    return -1;
	}

	di = dictitem_alloc(key);

	DICTKEY_UNREF

	if (di == NULL)
	{
	    Py_DECREF(list);
	    Py_DECREF(litem);
	    PyErr_NoMemory();
	    return -1;
	}
	di->di_tv.v_lock = 0;

	if (_ConvertFromPyObject(valObject, &di->di_tv, lookupDict) == -1)
	{
	    vim_free(di);
	    Py_DECREF(list);
	    Py_DECREF(litem);
	    return -1;
	}
	if (dict_add(d, di) == FAIL)
	{
	    vim_free(di);
	    Py_DECREF(list);
	    Py_DECREF(litem);
	    PyErr_SetVim(_("failed to add key to dictionary"));
	    return -1;
	}
	Py_DECREF(litem);
    }
    Py_DECREF(list);
    return 0;
}

    static int
pyseq_to_tv(PyObject *obj, typval_T *tv, PyObject *lookupDict)
{
    list_T	*l;

    l = list_alloc();
    if (l == NULL)
    {
	PyErr_NoMemory();
	return -1;
    }

    tv->v_type = VAR_LIST;
    tv->vval.v_list = l;

    if (list_py_concat(l, obj, lookupDict) == -1)
	return -1;

    return 0;
}

    static int
pyiter_to_tv(PyObject *obj, typval_T *tv, PyObject *lookupDict)
{
    PyObject	*iterator = PyObject_GetIter(obj);
    PyObject	*item;
    list_T	*l;
    listitem_T	*li;

    l = list_alloc();

    if (l == NULL)
    {
	PyErr_NoMemory();
	return -1;
    }

    tv->vval.v_list = l;
    tv->v_type = VAR_LIST;


    if (iterator == NULL)
	return -1;

    while ((item = PyIter_Next(obj)))
    {
	li = listitem_alloc();
	if (li == NULL)
	{
	    PyErr_NoMemory();
	    return -1;
	}
	li->li_tv.v_lock = 0;

	if (_ConvertFromPyObject(item, &li->li_tv, lookupDict) == -1)
	    return -1;

	list_append(l, li);

	Py_DECREF(item);
    }

    Py_DECREF(iterator);
    return 0;
}

typedef int (*pytotvfunc)(PyObject *, typval_T *, PyObject *);

    static int
convert_dl(PyObject *obj, typval_T *tv,
				    pytotvfunc py_to_tv, PyObject *lookupDict)
{
    PyObject	*capsule;
    char	hexBuf[sizeof(void *) * 2 + 3];

    sprintf(hexBuf, "%p", obj);

# ifdef PY_USE_CAPSULE
    capsule = PyDict_GetItemString(lookupDict, hexBuf);
# else
    capsule = (PyObject *)PyDict_GetItemString(lookupDict, hexBuf);
# endif
    if (capsule == NULL)
    {
# ifdef PY_USE_CAPSULE
	capsule = PyCapsule_New(tv, NULL, NULL);
# else
	capsule = PyCObject_FromVoidPtr(tv, NULL);
# endif
	PyDict_SetItemString(lookupDict, hexBuf, capsule);
	Py_DECREF(capsule);
	if (py_to_tv(obj, tv, lookupDict) == -1)
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
ConvertFromPyObject(PyObject *obj, typval_T *tv)
{
    PyObject	*lookup_dict;
    int		r;

    lookup_dict = PyDict_New();
    r = _ConvertFromPyObject(obj, tv, lookup_dict);
    Py_DECREF(lookup_dict);
    return r;
}

    static int
_ConvertFromPyObject(PyObject *obj, typval_T *tv, PyObject *lookupDict)
{
    if (obj->ob_type == &DictionaryType)
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

	if (set_string_copy(result, tv) == -1)
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
	return convert_dl(obj, tv, pydict_to_tv, lookupDict);
#ifdef FEAT_FLOAT
    else if (PyFloat_Check(obj))
    {
	tv->v_type = VAR_FLOAT;
	tv->vval.v_float = (float_T) PyFloat_AsDouble(obj);
    }
#endif
    else if (PyIter_Check(obj))
	return convert_dl(obj, tv, pyiter_to_tv, lookupDict);
    else if (PySequence_Check(obj))
	return convert_dl(obj, tv, pyseq_to_tv, lookupDict);
    else if (PyMapping_Check(obj))
	return convert_dl(obj, tv, pymap_to_tv, lookupDict);
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
	    return ListNew(tv->vval.v_list);
	case VAR_DICT:
	    return DictionaryNew(tv->vval.v_dict);
	case VAR_FUNC:
	    return FunctionNew(tv->vval.v_string == NULL
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
    OutputType.tp_getattro = OutputGetattro;
    OutputType.tp_setattro = OutputSetattro;
    OutputType.tp_alloc = call_PyType_GenericAlloc;
    OutputType.tp_new = call_PyType_GenericNew;
    OutputType.tp_free = call_PyObject_Free;
#else
    OutputType.tp_getattr = OutputGetattr;
    OutputType.tp_setattr = OutputSetattr;
#endif

    vim_memset(&IterType, 0, sizeof(IterType));
    IterType.tp_name = "vim.iter";
    IterType.tp_basicsize = sizeof(IterObject);
    IterType.tp_flags = Py_TPFLAGS_DEFAULT;
    IterType.tp_doc = "generic iterator object";
    IterType.tp_iter = IterIter;
    IterType.tp_iternext = IterNext;
    IterType.tp_dealloc = IterDestructor;
    IterType.tp_traverse = IterTraverse;
    IterType.tp_clear = IterClear;

    vim_memset(&BufferType, 0, sizeof(BufferType));
    BufferType.tp_name = "vim.buffer";
    BufferType.tp_basicsize = sizeof(BufferType);
    BufferType.tp_dealloc = BufferDestructor;
    BufferType.tp_repr = BufferRepr;
    BufferType.tp_as_sequence = &BufferAsSeq;
    BufferType.tp_as_mapping = &BufferAsMapping;
    BufferType.tp_flags = Py_TPFLAGS_DEFAULT;
    BufferType.tp_doc = "vim buffer object";
    BufferType.tp_methods = BufferMethods;
#if PY_MAJOR_VERSION >= 3
    BufferType.tp_getattro = BufferGetattro;
    BufferType.tp_alloc = call_PyType_GenericAlloc;
    BufferType.tp_new = call_PyType_GenericNew;
    BufferType.tp_free = call_PyObject_Free;
#else
    BufferType.tp_getattr = BufferGetattr;
#endif

    vim_memset(&WindowType, 0, sizeof(WindowType));
    WindowType.tp_name = "vim.window";
    WindowType.tp_basicsize = sizeof(WindowObject);
    WindowType.tp_dealloc = WindowDestructor;
    WindowType.tp_repr = WindowRepr;
    WindowType.tp_flags = Py_TPFLAGS_DEFAULT;
    WindowType.tp_doc = "vim Window object";
    WindowType.tp_methods = WindowMethods;
    WindowType.tp_traverse = WindowTraverse;
    WindowType.tp_clear = WindowClear;
#if PY_MAJOR_VERSION >= 3
    WindowType.tp_getattro = WindowGetattro;
    WindowType.tp_setattro = WindowSetattro;
    WindowType.tp_alloc = call_PyType_GenericAlloc;
    WindowType.tp_new = call_PyType_GenericNew;
    WindowType.tp_free = call_PyObject_Free;
#else
    WindowType.tp_getattr = WindowGetattr;
    WindowType.tp_setattr = WindowSetattr;
#endif

    vim_memset(&TabPageType, 0, sizeof(TabPageType));
    TabPageType.tp_name = "vim.tabpage";
    TabPageType.tp_basicsize = sizeof(TabPageObject);
    TabPageType.tp_dealloc = TabPageDestructor;
    TabPageType.tp_repr = TabPageRepr;
    TabPageType.tp_flags = Py_TPFLAGS_DEFAULT;
    TabPageType.tp_doc = "vim tab page object";
    TabPageType.tp_methods = TabPageMethods;
#if PY_MAJOR_VERSION >= 3
    TabPageType.tp_getattro = TabPageGetattro;
    TabPageType.tp_alloc = call_PyType_GenericAlloc;
    TabPageType.tp_new = call_PyType_GenericNew;
    TabPageType.tp_free = call_PyObject_Free;
#else
    TabPageType.tp_getattr = TabPageGetattr;
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
    WinListType.tp_dealloc = WinListDestructor;

    vim_memset(&TabListType, 0, sizeof(TabListType));
    TabListType.tp_name = "vim.tabpagelist";
    TabListType.tp_basicsize = sizeof(TabListType);
    TabListType.tp_as_sequence = &TabListAsSeq;
    TabListType.tp_flags = Py_TPFLAGS_DEFAULT;
    TabListType.tp_doc = "vim tab page list";

    vim_memset(&RangeType, 0, sizeof(RangeType));
    RangeType.tp_name = "vim.range";
    RangeType.tp_basicsize = sizeof(RangeObject);
    RangeType.tp_dealloc = RangeDestructor;
    RangeType.tp_repr = RangeRepr;
    RangeType.tp_as_sequence = &RangeAsSeq;
    RangeType.tp_as_mapping = &RangeAsMapping;
    RangeType.tp_flags = Py_TPFLAGS_DEFAULT;
    RangeType.tp_doc = "vim Range object";
    RangeType.tp_methods = RangeMethods;
#if PY_MAJOR_VERSION >= 3
    RangeType.tp_getattro = RangeGetattro;
    RangeType.tp_alloc = call_PyType_GenericAlloc;
    RangeType.tp_new = call_PyType_GenericNew;
    RangeType.tp_free = call_PyObject_Free;
#else
    RangeType.tp_getattr = RangeGetattr;
#endif

    vim_memset(&CurrentType, 0, sizeof(CurrentType));
    CurrentType.tp_name = "vim.currentdata";
    CurrentType.tp_basicsize = sizeof(CurrentObject);
    CurrentType.tp_flags = Py_TPFLAGS_DEFAULT;
    CurrentType.tp_doc = "vim current object";
#if PY_MAJOR_VERSION >= 3
    CurrentType.tp_getattro = CurrentGetattro;
    CurrentType.tp_setattro = CurrentSetattro;
#else
    CurrentType.tp_getattr = CurrentGetattr;
    CurrentType.tp_setattr = CurrentSetattr;
#endif

    vim_memset(&DictionaryType, 0, sizeof(DictionaryType));
    DictionaryType.tp_name = "vim.dictionary";
    DictionaryType.tp_basicsize = sizeof(DictionaryObject);
    DictionaryType.tp_dealloc = DictionaryDestructor;
    DictionaryType.tp_as_mapping = &DictionaryAsMapping;
    DictionaryType.tp_flags = Py_TPFLAGS_DEFAULT;
    DictionaryType.tp_doc = "dictionary pushing modifications to vim structure";
    DictionaryType.tp_methods = DictionaryMethods;
#if PY_MAJOR_VERSION >= 3
    DictionaryType.tp_getattro = DictionaryGetattro;
    DictionaryType.tp_setattro = DictionarySetattro;
#else
    DictionaryType.tp_getattr = DictionaryGetattr;
    DictionaryType.tp_setattr = DictionarySetattr;
#endif

    vim_memset(&ListType, 0, sizeof(ListType));
    ListType.tp_name = "vim.list";
    ListType.tp_dealloc = ListDestructor;
    ListType.tp_basicsize = sizeof(ListObject);
    ListType.tp_as_sequence = &ListAsSeq;
    ListType.tp_as_mapping = &ListAsMapping;
    ListType.tp_flags = Py_TPFLAGS_DEFAULT;
    ListType.tp_doc = "list pushing modifications to vim structure";
    ListType.tp_methods = ListMethods;
    ListType.tp_iter = ListIter;
#if PY_MAJOR_VERSION >= 3
    ListType.tp_getattro = ListGetattro;
    ListType.tp_setattro = ListSetattro;
#else
    ListType.tp_getattr = ListGetattr;
    ListType.tp_setattr = ListSetattr;
#endif

    vim_memset(&FunctionType, 0, sizeof(FunctionType));
    FunctionType.tp_name = "vim.function";
    FunctionType.tp_basicsize = sizeof(FunctionObject);
    FunctionType.tp_dealloc = FunctionDestructor;
    FunctionType.tp_call = FunctionCall;
    FunctionType.tp_flags = Py_TPFLAGS_DEFAULT;
    FunctionType.tp_doc = "object that calls vim function";
    FunctionType.tp_methods = FunctionMethods;
#if PY_MAJOR_VERSION >= 3
    FunctionType.tp_getattro = FunctionGetattro;
#else
    FunctionType.tp_getattr = FunctionGetattr;
#endif

    vim_memset(&OptionsType, 0, sizeof(OptionsType));
    OptionsType.tp_name = "vim.options";
    OptionsType.tp_basicsize = sizeof(OptionsObject);
    OptionsType.tp_flags = Py_TPFLAGS_DEFAULT;
    OptionsType.tp_doc = "object for manipulating options";
    OptionsType.tp_as_mapping = &OptionsAsMapping;
    OptionsType.tp_dealloc = OptionsDestructor;
    OptionsType.tp_traverse = OptionsTraverse;
    OptionsType.tp_clear = OptionsClear;

#if PY_MAJOR_VERSION >= 3
    vim_memset(&vimmodule, 0, sizeof(vimmodule));
    vimmodule.m_name = "vim";
    vimmodule.m_doc = "Vim Python interface\n";
    vimmodule.m_size = -1;
    vimmodule.m_methods = VimMethods;
#endif
}
