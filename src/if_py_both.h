/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * Python extensions by Paul Moore, David Leonard, Roland Puntaier.
 *
 * Common code for if_python.c and if_python3.c.
 */

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

/* Output object definition
 */

static PyObject *OutputWrite(PyObject *, PyObject *);
static PyObject *OutputWritelines(PyObject *, PyObject *);

typedef void (*writefn)(char_u *);
static void writer(writefn fn, char_u *str, PyInt n);

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

/*************/

/* Output buffer management
 */

    static PyObject *
OutputWrite(PyObject *self, PyObject *args)
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
	PyInt len;

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

static char_u *buffer = NULL;
static PyInt buffer_len = 0;
static PyInt buffer_size = 0;

static writefn old_fn = NULL;

    static void
buffer_ensure(PyInt n)
{
    PyInt new_size;
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

    static void
PythonIO_Flush(void)
{
    if (old_fn && buffer_len)
    {
	buffer[buffer_len] = 0;
	old_fn(buffer);
    }

    buffer_len = 0;
}

    static void
writer(writefn fn, char_u *str, PyInt n)
{
    char_u *ptr;

    if (fn != old_fn && old_fn != NULL)
	PythonIO_Flush();

    old_fn = fn;

    while (n > 0 && (ptr = memchr(str, '\n', n)) != NULL)
    {
	PyInt len = ptr - str;

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

static PyTypeObject OutputType;

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
