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

#define PyErr_SetVim(str) PyErr_SetString(VimError, str)

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

#ifdef FEAT_EVAL
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
#endif

    static PyObject *
VimEval(PyObject *self UNUSED, PyObject *args UNUSED)
{
#ifdef FEAT_EVAL
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
#else
    PyErr_SetVim(_("expressions disabled at compile time"));
    return NULL;
#endif
}

/*
 * Vim module - Definitions
 */

static struct PyMethodDef VimMethods[] = {
    /* name,	     function,		calling,    documentation */
    {"command",	     VimCommand,	1,	    "Execute a Vim ex-mode command" },
    {"eval",	     VimEval,		1,	    "Evaluate an expression using Vim evaluator" },
    { NULL,	     NULL,		0,	    NULL }
};

typedef struct
{
    PyObject_HEAD
    buf_T *buf;
}
BufferObject;

#define INVALID_BUFFER_VALUE ((buf_T *)(-1))

/*
 * Buffer list object - Implementation
 */

    static PyInt
BufListLength(PyObject *self UNUSED)
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
BufListItem(PyObject *self UNUSED, PyInt n)
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

typedef struct
{
    PyObject_HEAD
    win_T	*win;
} WindowObject;

#define INVALID_WINDOW_VALUE ((win_T *)(-1))

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

static int WindowSetattr(PyObject *, char *, PyObject *);
static PyObject *WindowRepr(PyObject *);

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
	long len;

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

	/* When column is out of range silently correct it. */
	len = (long)STRLEN(ml_get_buf(this->win->w_buffer, lnum, FALSE));
	if (col > len)
	    col = len;

	this->win->w_cursor.lnum = lnum;
	this->win->w_cursor.col = col;
#ifdef FEAT_VIRTUALEDIT
	this->win->w_cursor.coladd = 0;
#endif
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
	int	i = 0;
	win_T	*w;

	for (w = firstwin; w != NULL && w != this->win; w = W_NEXT(w))
	    ++i;

	if (w == NULL)
	    vim_snprintf(repr, 100, _("<window object (unknown) at %p>"),
								      (self));
	else
	    vim_snprintf(repr, 100, _("<window %d>"), i);

	return PyString_FromString(repr);
    }
}

/*
 * Window list object - Implementation
 */
    static PyInt
WinListLength(PyObject *self UNUSED)
{
    win_T	*w = firstwin;
    PyInt	n = 0;

    while (w != NULL)
    {
	++n;
	w = W_NEXT(w);
    }

    return n;
}

    static PyObject *
WinListItem(PyObject *self UNUSED, PyInt n)
{
    win_T *w;

    for (w = firstwin; w != NULL; w = W_NEXT(w), --n)
	if (n == 0)
	    return WindowNew(w);

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
    PyInt len;
    PyInt i;
    char *p;

    if (obj == NULL || !PyString_Check(obj))
    {
	PyErr_BadArgument();
	return NULL;
    }

    str = PyString_AsString(obj);
    len = PyString_Size(obj);

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

/* Replace a line in the specified buffer. The line number is
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
    /* First of all, we check the thpe of the supplied Python object.
     * There are three cases:
     *	  1. NULL, or None - this is a deletion.
     *	  2. A string	   - this is a replacement.
     *	  3. Anything else - this is an error.
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
	    if (buf == curwin->w_buffer)
		py_fix_cursor((linenr_T)n, (linenr_T)n + 1, (linenr_T)-1);
	    deleted_lines_mark((linenr_T)n, 1L);
	}

	curbuf = savebuf;

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = -1;

	return OK;
    }
    else if (PyString_Check(line))
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

/*
 * Common routines for buffers and line ranges
 * -------------------------------------------
 */

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


    static PyObject *
RBAppend(BufferObject *self, PyObject *args, PyInt start, PyInt end, PyInt *new_end)
{
    PyObject *lines;
    PyInt len_change;
    PyInt max;
    PyInt n;

    if (CheckBuffer(self))
	return NULL;

    max = n = end - start + 1;

    if (!PyArg_ParseTuple(args, "O|n", &lines, &n))
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


/* Buffer object - Definitions
 */

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

    static PyObject *
BufferAppend(PyObject *self, PyObject *args)
{
    return RBAppend((BufferObject *)(self), args, 1,
		    (PyInt)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		    NULL);
}

    static PyObject *
BufferMark(PyObject *self, PyObject *args)
{
    pos_T	*posp;
    char	*pmark;
    char	mark;
    buf_T	*curbuf_save;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    if (!PyArg_ParseTuple(args, "s", &pmark))
	return NULL;
    mark = *pmark;

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

static struct PyMethodDef BufferMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    BufferAppend,	1,	    "Append data to Vim buffer" },
    {"mark",	    BufferMark,		1,	    "Return (row,col) representing position of named mark" },
    {"range",	    BufferRange,	1,	    "Return a range object which represents the part of the given buffer between line numbers s and e" },
    { NULL,	    NULL,		0,	    NULL }
};

    static PyObject *
RangeAppend(PyObject *self, PyObject *args)
{
    return RBAppend(((RangeObject *)(self))->buf, args,
		    ((RangeObject *)(self))->start,
		    ((RangeObject *)(self))->end,
		    &((RangeObject *)(self))->end);
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

    static PyObject *
RangeSlice(PyObject *self, PyInt lo, PyInt hi)
{
    return RBSlice(((RangeObject *)(self))->buf, lo, hi,
		   ((RangeObject *)(self))->start,
		   ((RangeObject *)(self))->end);
}

/*
 * Line range object - Definitions
 */

static struct PyMethodDef RangeMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    RangeAppend,	1,	    "Append data to the Vim range" },
    { NULL,	    NULL,		0,	    NULL }
};

