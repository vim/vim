/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * viminfo.c: viminfo related functions
 */

#include "vim.h"
#include "version.h"

#if defined(FEAT_VIMINFO) || defined(PROTO)

static int  viminfo_errcnt;

/*
 * Get the viminfo file name to use.
 * If "file" is given and not empty, use it (has already been expanded by
 * cmdline functions).
 * Otherwise use "-i file_name", value from 'viminfo' or the default, and
 * expand environment variables.
 * Returns an allocated string.  NULL when out of memory.
 */
    static char_u *
viminfo_filename(char_u *file)
{
    if (file == NULL || *file == NUL)
    {
	if (*p_viminfofile != NUL)
	    file = p_viminfofile;
	else if ((file = find_viminfo_parameter('n')) == NULL || *file == NUL)
	{
#ifdef VIMINFO_FILE2
# ifdef VMS
	    if (mch_getenv((char_u *)"SYS$LOGIN") == NULL)
# else
#  ifdef MSWIN
	    // Use $VIM only if $HOME is the default "C:/".
	    if (STRCMP(vim_getenv((char_u *)"HOME", NULL), "C:/") == 0
		    && mch_getenv((char_u *)"HOME") == NULL)
#  else
	    if (mch_getenv((char_u *)"HOME") == NULL)
#  endif
# endif
	    {
		// don't use $VIM when not available.
		expand_env((char_u *)"$VIM", NameBuff, MAXPATHL);
		if (STRCMP("$VIM", NameBuff) != 0)  // $VIM was expanded
		    file = (char_u *)VIMINFO_FILE2;
		else
		    file = (char_u *)VIMINFO_FILE;
	    }
	    else
#endif
		file = (char_u *)VIMINFO_FILE;
	}
	expand_env(file, NameBuff, MAXPATHL);
	file = NameBuff;
    }
    return vim_strsave(file);
}

    static int
read_viminfo_bufferlist(
    vir_T	*virp,
    int		writing)
{
    char_u	*tab;
    linenr_T	lnum;
    colnr_T	col;
    buf_T	*buf;
    char_u	*sfname;
    char_u	*xline;

    // Handle long line and escaped characters.
    xline = viminfo_readstring(virp, 1, FALSE);

    // don't read in if there are files on the command-line or if writing:
    if (xline != NULL && !writing && ARGCOUNT == 0
				       && find_viminfo_parameter('%') != NULL)
    {
	// Format is: <fname> Tab <lnum> Tab <col>.
	// Watch out for a Tab in the file name, work from the end.
	lnum = 0;
	col = 0;
	tab = vim_strrchr(xline, '\t');
	if (tab != NULL)
	{
	    *tab++ = '\0';
	    col = (colnr_T)atoi((char *)tab);
	    tab = vim_strrchr(xline, '\t');
	    if (tab != NULL)
	    {
		*tab++ = '\0';
		lnum = atol((char *)tab);
	    }
	}

	// Expand "~/" in the file name at "line + 1" to a full path.
	// Then try shortening it by comparing with the current directory
	expand_env(xline, NameBuff, MAXPATHL);
	sfname = shorten_fname1(NameBuff);

	buf = buflist_new(NameBuff, sfname, (linenr_T)0, BLN_LISTED);
	if (buf != NULL)	// just in case...
	{
	    buf->b_last_cursor.lnum = lnum;
	    buf->b_last_cursor.col = col;
	    buflist_setfpos(buf, curwin, lnum, col, FALSE);
	}
    }
    vim_free(xline);

    return viminfo_readline(virp);
}

    static void
write_viminfo_bufferlist(FILE *fp)
{
    buf_T	*buf;
    win_T	*win;
    tabpage_T	*tp;
    char_u	*line;
    int		max_buffers;

    if (find_viminfo_parameter('%') == NULL)
	return;

    // Without a number -1 is returned: do all buffers.
    max_buffers = get_viminfo_parameter('%');

    // Allocate room for the file name, lnum and col.
#define LINE_BUF_LEN (MAXPATHL + 40)
    line = alloc(LINE_BUF_LEN);
    if (line == NULL)
	return;

    FOR_ALL_TAB_WINDOWS(tp, win)
	set_last_cursor(win);

    fputs(_("\n# Buffer list:\n"), fp);
    FOR_ALL_BUFFERS(buf)
    {
	if (buf->b_fname == NULL
		|| !buf->b_p_bl
#ifdef FEAT_QUICKFIX
		|| bt_quickfix(buf)
#endif
#ifdef FEAT_TERMINAL
		|| bt_terminal(buf)
#endif
		|| removable(buf->b_ffname))
	    continue;

	if (max_buffers-- == 0)
	    break;
	putc('%', fp);
	home_replace(NULL, buf->b_ffname, line, MAXPATHL, TRUE);
	vim_snprintf_add((char *)line, LINE_BUF_LEN, "\t%ld\t%d",
			(long)buf->b_last_cursor.lnum,
			buf->b_last_cursor.col);
	viminfo_writestring(fp, line);
    }
    vim_free(line);
}

    static void
write_viminfo_barlines(vir_T *virp, FILE *fp_out)
{
    int		i;
    garray_T	*gap = &virp->vir_barlines;
    int		seen_useful = FALSE;
    char	*line;

    if (gap->ga_len > 0)
    {
	fputs(_("\n# Bar lines, copied verbatim:\n"), fp_out);

	// Skip over continuation lines until seeing a useful line.
	for (i = 0; i < gap->ga_len; ++i)
	{
	    line = ((char **)(gap->ga_data))[i];
	    if (seen_useful || line[1] != '<')
	    {
		fputs(line, fp_out);
		seen_useful = TRUE;
	    }
	}
    }
}

/*
 * Parse a viminfo line starting with '|'.
 * Add each decoded value to "values".
 * Returns TRUE if the next line is to be read after using the parsed values.
 */
    static int
barline_parse(vir_T *virp, char_u *text, garray_T *values)
{
    char_u  *p = text;
    char_u  *nextp = NULL;
    char_u  *buf = NULL;
    bval_T  *value;
    int	    i;
    int	    allocated = FALSE;
    int	    eof;
    char_u  *sconv;
    int	    converted;

    while (*p == ',')
    {
	++p;
	if (ga_grow(values, 1) == FAIL)
	    break;
	value = (bval_T *)(values->ga_data) + values->ga_len;

	if (*p == '>')
	{
	    // Need to read a continuation line.  Put strings in allocated
	    // memory, because virp->vir_line is overwritten.
	    if (!allocated)
	    {
		for (i = 0; i < values->ga_len; ++i)
		{
		    bval_T  *vp = (bval_T *)(values->ga_data) + i;

		    if (vp->bv_type == BVAL_STRING && !vp->bv_allocated)
		    {
			vp->bv_string = vim_strnsave(vp->bv_string, vp->bv_len);
			vp->bv_allocated = TRUE;
		    }
		}
		allocated = TRUE;
	    }

	    if (vim_isdigit(p[1]))
	    {
		size_t len;
		size_t todo;
		size_t n;

		// String value was split into lines that are each shorter
		// than LSIZE:
		//     |{bartype},>{length of "{text}{text2}"}
		//     |<"{text1}
		//     |<{text2}",{value}
		// Length includes the quotes.
		++p;
		len = getdigits(&p);
		buf = alloc((int)(len + 1));
		if (buf == NULL)
		    return TRUE;
		p = buf;
		for (todo = len; todo > 0; todo -= n)
		{
		    eof = viminfo_readline(virp);
		    if (eof || virp->vir_line[0] != '|'
						  || virp->vir_line[1] != '<')
		    {
			// File was truncated or garbled. Read another line if
			// this one starts with '|'.
			vim_free(buf);
			return eof || virp->vir_line[0] == '|';
		    }
		    // Get length of text, excluding |< and NL chars.
		    n = STRLEN(virp->vir_line);
		    while (n > 0 && (virp->vir_line[n - 1] == NL
					     || virp->vir_line[n - 1] == CAR))
			--n;
		    n -= 2;
		    if (n > todo)
		    {
			// more values follow after the string
			nextp = virp->vir_line + 2 + todo;
			n = todo;
		    }
		    mch_memmove(p, virp->vir_line + 2, n);
		    p += n;
		}
		*p = NUL;
		p = buf;
	    }
	    else
	    {
		// Line ending in ">" continues in the next line:
		//     |{bartype},{lots of values},>
		//     |<{value},{value}
		eof = viminfo_readline(virp);
		if (eof || virp->vir_line[0] != '|'
					      || virp->vir_line[1] != '<')
		    // File was truncated or garbled. Read another line if
		    // this one starts with '|'.
		    return eof || virp->vir_line[0] == '|';
		p = virp->vir_line + 2;
	    }
	}

	if (isdigit(*p))
	{
	    value->bv_type = BVAL_NR;
	    value->bv_nr = getdigits(&p);
	    ++values->ga_len;
	}
	else if (*p == '"')
	{
	    int	    len = 0;
	    char_u  *s = p;

	    // Unescape special characters in-place.
	    ++p;
	    while (*p != '"')
	    {
		if (*p == NL || *p == NUL)
		    return TRUE;  // syntax error, drop the value
		if (*p == '\\')
		{
		    ++p;
		    if (*p == 'n')
			s[len++] = '\n';
		    else
			s[len++] = *p;
		    ++p;
		}
		else
		    s[len++] = *p++;
	    }
	    ++p;
	    s[len] = NUL;

	    converted = FALSE;
	    if (virp->vir_conv.vc_type != CONV_NONE && *s != NUL)
	    {
		sconv = string_convert(&virp->vir_conv, s, NULL);
		if (sconv != NULL)
		{
		    if (s == buf)
			vim_free(s);
		    s = sconv;
		    buf = s;
		    converted = TRUE;
		}
	    }

	    // Need to copy in allocated memory if the string wasn't allocated
	    // above and we did allocate before, thus vir_line may change.
	    if (s != buf && allocated)
		s = vim_strsave(s);
	    value->bv_string = s;
	    value->bv_type = BVAL_STRING;
	    value->bv_len = len;
	    value->bv_allocated = allocated || converted;
	    ++values->ga_len;
	    if (nextp != NULL)
	    {
		// values following a long string
		p = nextp;
		nextp = NULL;
	    }
	}
	else if (*p == ',')
	{
	    value->bv_type = BVAL_EMPTY;
	    ++values->ga_len;
	}
	else
	    break;
    }
    return TRUE;
}

    static int
read_viminfo_barline(vir_T *virp, int got_encoding, int force, int writing)
{
    char_u	*p = virp->vir_line + 1;
    int		bartype;
    garray_T	values;
    bval_T	*vp;
    int		i;
    int		read_next = TRUE;

    /*
     * The format is: |{bartype},{value},...
     * For a very long string:
     *     |{bartype},>{length of "{text}{text2}"}
     *     |<{text1}
     *     |<{text2},{value}
     * For a long line not using a string
     *     |{bartype},{lots of values},>
     *     |<{value},{value}
     */
    if (*p == '<')
    {
	// Continuation line of an unrecognized item.
	if (writing)
	    ga_add_string(&virp->vir_barlines, virp->vir_line);
    }
    else
    {
	ga_init2(&values, sizeof(bval_T), 20);
	bartype = getdigits(&p);
	switch (bartype)
	{
	    case BARTYPE_VERSION:
		// Only use the version when it comes before the encoding.
		// If it comes later it was copied by a Vim version that
		// doesn't understand the version.
		if (!got_encoding)
		{
		    read_next = barline_parse(virp, p, &values);
		    vp = (bval_T *)values.ga_data;
		    if (values.ga_len > 0 && vp->bv_type == BVAL_NR)
			virp->vir_version = vp->bv_nr;
		}
		break;

	    case BARTYPE_HISTORY:
		read_next = barline_parse(virp, p, &values);
		handle_viminfo_history(&values, writing);
		break;

	    case BARTYPE_REGISTER:
		read_next = barline_parse(virp, p, &values);
		handle_viminfo_register(&values, force);
		break;

	    case BARTYPE_MARK:
		read_next = barline_parse(virp, p, &values);
		handle_viminfo_mark(&values, force);
		break;

	    default:
		// copy unrecognized line (for future use)
		if (writing)
		    ga_add_string(&virp->vir_barlines, virp->vir_line);
	}
	for (i = 0; i < values.ga_len; ++i)
	{
	    vp = (bval_T *)values.ga_data + i;
	    if (vp->bv_type == BVAL_STRING && vp->bv_allocated)
		vim_free(vp->bv_string);
	}
	ga_clear(&values);
    }

    if (read_next)
	return viminfo_readline(virp);
    return FALSE;
}

    static void
write_viminfo_version(FILE *fp_out)
{
    fprintf(fp_out, "# Viminfo version\n|%d,%d\n\n",
					    BARTYPE_VERSION, VIMINFO_VERSION);
}

    static int
no_viminfo(void)
{
    // "vim -i NONE" does not read or write a viminfo file
    return STRCMP(p_viminfofile, "NONE") == 0;
}

/*
 * Report an error for reading a viminfo file.
 * Count the number of errors.	When there are more than 10, return TRUE.
 */
    int
viminfo_error(char *errnum, char *message, char_u *line)
{
    vim_snprintf((char *)IObuff, IOSIZE, _("%sviminfo: %s in line: "),
							     errnum, message);
    STRNCAT(IObuff, line, IOSIZE - STRLEN(IObuff) - 1);
    if (IObuff[STRLEN(IObuff) - 1] == '\n')
	IObuff[STRLEN(IObuff) - 1] = NUL;
    emsg((char *)IObuff);
    if (++viminfo_errcnt >= 10)
    {
	emsg(_("E136: viminfo: Too many errors, skipping rest of file"));
	return TRUE;
    }
    return FALSE;
}

/*
 * Compare the 'encoding' value in the viminfo file with the current value of
 * 'encoding'.  If different and the 'c' flag is in 'viminfo', setup for
 * conversion of text with iconv() in viminfo_readstring().
 */
    static int
viminfo_encoding(vir_T *virp)
{
    char_u	*p;
    int		i;

    if (get_viminfo_parameter('c') != 0)
    {
	p = vim_strchr(virp->vir_line, '=');
	if (p != NULL)
	{
	    // remove trailing newline
	    ++p;
	    for (i = 0; vim_isprintc(p[i]); ++i)
		;
	    p[i] = NUL;

	    convert_setup(&virp->vir_conv, p, p_enc);
	}
    }
    return viminfo_readline(virp);
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Restore global vars that start with a capital from the viminfo file
 */
    static int
read_viminfo_varlist(vir_T *virp, int writing)
{
    char_u	*tab;
    int		type = VAR_NUMBER;
    typval_T	tv;
    funccal_entry_T funccal_entry;

    if (!writing && (find_viminfo_parameter('!') != NULL))
    {
	tab = vim_strchr(virp->vir_line + 1, '\t');
	if (tab != NULL)
	{
	    *tab++ = '\0';	// isolate the variable name
	    switch (*tab)
	    {
		case 'S': type = VAR_STRING; break;
#ifdef FEAT_FLOAT
		case 'F': type = VAR_FLOAT; break;
#endif
		case 'D': type = VAR_DICT; break;
		case 'L': type = VAR_LIST; break;
		case 'B': type = VAR_BLOB; break;
		case 'X': type = VAR_SPECIAL; break;
	    }

	    tab = vim_strchr(tab, '\t');
	    if (tab != NULL)
	    {
		tv.v_type = type;
		if (type == VAR_STRING || type == VAR_DICT
			|| type == VAR_LIST || type == VAR_BLOB)
		    tv.vval.v_string = viminfo_readstring(virp,
				       (int)(tab - virp->vir_line + 1), TRUE);
#ifdef FEAT_FLOAT
		else if (type == VAR_FLOAT)
		    (void)string2float(tab + 1, &tv.vval.v_float);
#endif
		else
		    tv.vval.v_number = atol((char *)tab + 1);
		if (type == VAR_DICT || type == VAR_LIST)
		{
		    typval_T *etv = eval_expr(tv.vval.v_string, NULL);

		    if (etv == NULL)
			// Failed to parse back the dict or list, use it as a
			// string.
			tv.v_type = VAR_STRING;
		    else
		    {
			vim_free(tv.vval.v_string);
			tv = *etv;
			vim_free(etv);
		    }
		}
		else if (type == VAR_BLOB)
		{
		    blob_T *blob = string2blob(tv.vval.v_string);

		    if (blob == NULL)
			// Failed to parse back the blob, use it as a string.
			tv.v_type = VAR_STRING;
		    else
		    {
			vim_free(tv.vval.v_string);
			tv.v_type = VAR_BLOB;
			tv.vval.v_blob = blob;
		    }
		}

		// when in a function use global variables
		save_funccal(&funccal_entry);
		set_var(virp->vir_line + 1, &tv, FALSE);
		restore_funccal();

		if (tv.v_type == VAR_STRING)
		    vim_free(tv.vval.v_string);
		else if (tv.v_type == VAR_DICT || tv.v_type == VAR_LIST ||
			tv.v_type == VAR_BLOB)
		    clear_tv(&tv);
	    }
	}
    }

    return viminfo_readline(virp);
}

/*
 * Write global vars that start with a capital to the viminfo file
 */
    static void
write_viminfo_varlist(FILE *fp)
{
    hashitem_T	*hi;
    dictitem_T	*this_var;
    int		todo;
    char	*s = "";
    char_u	*p;
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];

    if (find_viminfo_parameter('!') == NULL)
	return;

    fputs(_("\n# global variables:\n"), fp);

    todo = (int)globvarht.ht_used;
    for (hi = globvarht.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    this_var = HI2DI(hi);
	    if (var_flavour(this_var->di_key) == VAR_FLAVOUR_VIMINFO)
	    {
		switch (this_var->di_tv.v_type)
		{
		    case VAR_STRING: s = "STR"; break;
		    case VAR_NUMBER: s = "NUM"; break;
		    case VAR_FLOAT:  s = "FLO"; break;
		    case VAR_DICT:   s = "DIC"; break;
		    case VAR_LIST:   s = "LIS"; break;
		    case VAR_BLOB:   s = "BLO"; break;
		    case VAR_SPECIAL: s = "XPL"; break;

		    case VAR_UNKNOWN:
		    case VAR_FUNC:
		    case VAR_PARTIAL:
		    case VAR_JOB:
		    case VAR_CHANNEL:
				     continue;
		}
		fprintf(fp, "!%s\t%s\t", this_var->di_key, s);
		if (this_var->di_tv.v_type == VAR_SPECIAL)
		{
		    sprintf((char *)numbuf, "%ld",
					  (long)this_var->di_tv.vval.v_number);
		    p = numbuf;
		    tofree = NULL;
		}
		else
		    p = echo_string(&this_var->di_tv, &tofree, numbuf, 0);
		if (p != NULL)
		    viminfo_writestring(fp, p);
		vim_free(tofree);
	    }
	}
    }
}
#endif // FEAT_EVAL

/*
 * read_viminfo_up_to_marks() -- Only called from do_viminfo().  Reads in the
 * first part of the viminfo file which contains everything but the marks that
 * are local to a file.  Returns TRUE when end-of-file is reached. -- webb
 */
    static int
read_viminfo_up_to_marks(
    vir_T	*virp,
    int		forceit,
    int		writing)
{
    int		eof;
    buf_T	*buf;
    int		got_encoding = FALSE;

#ifdef FEAT_CMDHIST
    prepare_viminfo_history(forceit ? 9999 : 0, writing);
#endif

    eof = viminfo_readline(virp);
    while (!eof && virp->vir_line[0] != '>')
    {
	switch (virp->vir_line[0])
	{
		// Characters reserved for future expansion, ignored now
	    case '+': // "+40 /path/dir file", for running vim without args
	    case '^': // to be defined
	    case '<': // long line - ignored
		// A comment or empty line.
	    case NUL:
	    case '\r':
	    case '\n':
	    case '#':
		eof = viminfo_readline(virp);
		break;
	    case '|':
		eof = read_viminfo_barline(virp, got_encoding,
							    forceit, writing);
		break;
	    case '*': // "*encoding=value"
		got_encoding = TRUE;
		eof = viminfo_encoding(virp);
		break;
	    case '!': // global variable
#ifdef FEAT_EVAL
		eof = read_viminfo_varlist(virp, writing);
#else
		eof = viminfo_readline(virp);
#endif
		break;
	    case '%': // entry for buffer list
		eof = read_viminfo_bufferlist(virp, writing);
		break;
	    case '"':
		// When registers are in bar lines skip the old style register
		// lines.
		if (virp->vir_version < VIMINFO_VERSION_WITH_REGISTERS)
		    eof = read_viminfo_register(virp, forceit);
		else
		    do {
			eof = viminfo_readline(virp);
		    } while (!eof && (virp->vir_line[0] == TAB
						|| virp->vir_line[0] == '<'));
		break;
	    case '/':	    // Search string
	    case '&':	    // Substitute search string
	    case '~':	    // Last search string, followed by '/' or '&'
		eof = read_viminfo_search_pattern(virp, forceit);
		break;
	    case '$':
		eof = read_viminfo_sub_string(virp, forceit);
		break;
	    case ':':
	    case '?':
	    case '=':
	    case '@':
#ifdef FEAT_CMDHIST
		// When history is in bar lines skip the old style history
		// lines.
		if (virp->vir_version < VIMINFO_VERSION_WITH_HISTORY)
		    eof = read_viminfo_history(virp, writing);
		else
#endif
		    eof = viminfo_readline(virp);
		break;
	    case '-':
	    case '\'':
		// When file marks are in bar lines skip the old style lines.
		if (virp->vir_version < VIMINFO_VERSION_WITH_MARKS)
		    eof = read_viminfo_filemark(virp, forceit);
		else
		    eof = viminfo_readline(virp);
		break;
	    default:
		if (viminfo_error("E575: ", _("Illegal starting char"),
			    virp->vir_line))
		    eof = TRUE;
		else
		    eof = viminfo_readline(virp);
		break;
	}
    }

#ifdef FEAT_CMDHIST
    // Finish reading history items.
    if (!writing)
	finish_viminfo_history(virp);
#endif

    // Change file names to buffer numbers for fmarks.
    FOR_ALL_BUFFERS(buf)
	fmarks_check_names(buf);

    return eof;
}

/*
 * do_viminfo() -- Should only be called from read_viminfo() & write_viminfo().
 */
    static void
do_viminfo(FILE *fp_in, FILE *fp_out, int flags)
{
    int		eof = FALSE;
    vir_T	vir;
    int		merge = FALSE;
    int		do_copy_marks = FALSE;
    garray_T	buflist;

    if ((vir.vir_line = alloc(LSIZE)) == NULL)
	return;
    vir.vir_fd = fp_in;
    vir.vir_conv.vc_type = CONV_NONE;
    ga_init2(&vir.vir_barlines, (int)sizeof(char_u *), 100);
    vir.vir_version = -1;

    if (fp_in != NULL)
    {
	if (flags & VIF_WANT_INFO)
	{
	    if (fp_out != NULL)
	    {
		// Registers and marks are read and kept separate from what
		// this Vim is using.  They are merged when writing.
		prepare_viminfo_registers();
		prepare_viminfo_marks();
	    }

	    eof = read_viminfo_up_to_marks(&vir,
					 flags & VIF_FORCEIT, fp_out != NULL);
	    merge = TRUE;
	}
	else if (flags != 0)
	    // Skip info, find start of marks
	    while (!(eof = viminfo_readline(&vir))
		    && vir.vir_line[0] != '>')
		;

	do_copy_marks = (flags &
			   (VIF_WANT_MARKS | VIF_GET_OLDFILES | VIF_FORCEIT));
    }

    if (fp_out != NULL)
    {
	// Write the info:
	fprintf(fp_out, _("# This viminfo file was generated by Vim %s.\n"),
							  VIM_VERSION_MEDIUM);
	fputs(_("# You may edit it if you're careful!\n\n"), fp_out);
	write_viminfo_version(fp_out);
	fputs(_("# Value of 'encoding' when this file was written\n"), fp_out);
	fprintf(fp_out, "*encoding=%s\n\n", p_enc);
	write_viminfo_search_pattern(fp_out);
	write_viminfo_sub_string(fp_out);
#ifdef FEAT_CMDHIST
	write_viminfo_history(fp_out, merge);
#endif
	write_viminfo_registers(fp_out);
	finish_viminfo_registers();
#ifdef FEAT_EVAL
	write_viminfo_varlist(fp_out);
#endif
	write_viminfo_filemarks(fp_out);
	finish_viminfo_marks();
	write_viminfo_bufferlist(fp_out);
	write_viminfo_barlines(&vir, fp_out);

	if (do_copy_marks)
	    ga_init2(&buflist, sizeof(buf_T *), 50);
	write_viminfo_marks(fp_out, do_copy_marks ? &buflist : NULL);
    }

    if (do_copy_marks)
    {
	copy_viminfo_marks(&vir, fp_out, &buflist, eof, flags);
	if (fp_out != NULL)
	    ga_clear(&buflist);
    }

    vim_free(vir.vir_line);
    if (vir.vir_conv.vc_type != CONV_NONE)
	convert_setup(&vir.vir_conv, NULL, NULL);
    ga_clear_strings(&vir.vir_barlines);
}

/*
 * read_viminfo() -- Read the viminfo file.  Registers etc. which are already
 * set are not over-written unless "flags" includes VIF_FORCEIT. -- webb
 */
    int
read_viminfo(
    char_u	*file,	    // file name or NULL to use default name
    int		flags)	    // VIF_WANT_INFO et al.
{
    FILE	*fp;
    char_u	*fname;

    if (no_viminfo())
	return FAIL;

    fname = viminfo_filename(file);	// get file name in allocated buffer
    if (fname == NULL)
	return FAIL;
    fp = mch_fopen((char *)fname, READBIN);

    if (p_verbose > 0)
    {
	verbose_enter();
	smsg(_("Reading viminfo file \"%s\"%s%s%s"),
		fname,
		(flags & VIF_WANT_INFO) ? _(" info") : "",
		(flags & VIF_WANT_MARKS) ? _(" marks") : "",
		(flags & VIF_GET_OLDFILES) ? _(" oldfiles") : "",
		fp == NULL ? _(" FAILED") : "");
	verbose_leave();
    }

    vim_free(fname);
    if (fp == NULL)
	return FAIL;

    viminfo_errcnt = 0;
    do_viminfo(fp, NULL, flags);

    fclose(fp);
    return OK;
}

/*
 * Write the viminfo file.  The old one is read in first so that effectively a
 * merge of current info and old info is done.  This allows multiple vims to
 * run simultaneously, without losing any marks etc.
 * If "forceit" is TRUE, then the old file is not read in, and only internal
 * info is written to the file.
 */
    void
write_viminfo(char_u *file, int forceit)
{
    char_u	*fname;
    FILE	*fp_in = NULL;	// input viminfo file, if any
    FILE	*fp_out = NULL;	// output viminfo file
    char_u	*tempname = NULL;	// name of temp viminfo file
    stat_T	st_new;		// mch_stat() of potential new file
#if defined(UNIX) || defined(VMS)
    mode_t	umask_save;
#endif
#ifdef UNIX
    int		shortname = FALSE;	// use 8.3 file name
    stat_T	st_old;		// mch_stat() of existing viminfo file
#endif
#ifdef MSWIN
    int		hidden = FALSE;
#endif

    if (no_viminfo())
	return;

    fname = viminfo_filename(file);	// may set to default if NULL
    if (fname == NULL)
	return;

    fp_in = mch_fopen((char *)fname, READBIN);
    if (fp_in == NULL)
    {
	int fd;

	// if it does exist, but we can't read it, don't try writing
	if (mch_stat((char *)fname, &st_new) == 0)
	    goto end;

	// Create the new .viminfo non-accessible for others, because it may
	// contain text from non-accessible documents. It is up to the user to
	// widen access (e.g. to a group). This may also fail if there is a
	// race condition, then just give up.
	fd = mch_open((char *)fname,
			    O_CREAT|O_EXTRA|O_EXCL|O_WRONLY|O_NOFOLLOW, 0600);
	if (fd < 0)
	    goto end;
	fp_out = fdopen(fd, WRITEBIN);
    }
    else
    {
	/*
	 * There is an existing viminfo file.  Create a temporary file to
	 * write the new viminfo into, in the same directory as the
	 * existing viminfo file, which will be renamed once all writing is
	 * successful.
	 */
#ifdef UNIX
	/*
	 * For Unix we check the owner of the file.  It's not very nice to
	 * overwrite a user's viminfo file after a "su root", with a
	 * viminfo file that the user can't read.
	 */
	st_old.st_dev = (dev_t)0;
	st_old.st_ino = 0;
	st_old.st_mode = 0600;
	if (mch_stat((char *)fname, &st_old) == 0
		&& getuid() != ROOT_UID
		&& !(st_old.st_uid == getuid()
			? (st_old.st_mode & 0200)
			: (st_old.st_gid == getgid()
				? (st_old.st_mode & 0020)
				: (st_old.st_mode & 0002))))
	{
	    int	tt = msg_didany;

	    // avoid a wait_return for this message, it's annoying
	    semsg(_("E137: Viminfo file is not writable: %s"), fname);
	    msg_didany = tt;
	    fclose(fp_in);
	    goto end;
	}
#endif
#ifdef MSWIN
	// Get the file attributes of the existing viminfo file.
	hidden = mch_ishidden(fname);
#endif

	/*
	 * Make tempname, find one that does not exist yet.
	 * Beware of a race condition: If someone logs out and all Vim
	 * instances exit at the same time a temp file might be created between
	 * stat() and open().  Use mch_open() with O_EXCL to avoid that.
	 * May try twice: Once normal and once with shortname set, just in
	 * case somebody puts his viminfo file in an 8.3 filesystem.
	 */
	for (;;)
	{
	    int		next_char = 'z';
	    char_u	*wp;

	    tempname = buf_modname(
#ifdef UNIX
				    shortname,
#else
				    FALSE,
#endif
				    fname,
#ifdef VMS
				    (char_u *)"-tmp",
#else
				    (char_u *)".tmp",
#endif
				    FALSE);
	    if (tempname == NULL)		// out of memory
		break;

	    /*
	     * Try a series of names.  Change one character, just before
	     * the extension.  This should also work for an 8.3
	     * file name, when after adding the extension it still is
	     * the same file as the original.
	     */
	    wp = tempname + STRLEN(tempname) - 5;
	    if (wp < gettail(tempname))	    // empty file name?
		wp = gettail(tempname);
	    for (;;)
	    {
		/*
		 * Check if tempfile already exists.  Never overwrite an
		 * existing file!
		 */
		if (mch_stat((char *)tempname, &st_new) == 0)
		{
#ifdef UNIX
		    /*
		     * Check if tempfile is same as original file.  May happen
		     * when modname() gave the same file back.  E.g.  silly
		     * link, or file name-length reached.  Try again with
		     * shortname set.
		     */
		    if (!shortname && st_new.st_dev == st_old.st_dev
						&& st_new.st_ino == st_old.st_ino)
		    {
			VIM_CLEAR(tempname);
			shortname = TRUE;
			break;
		    }
#endif
		}
		else
		{
		    // Try creating the file exclusively.  This may fail if
		    // another Vim tries to do it at the same time.
#ifdef VMS
		    // fdopen() fails for some reason
		    umask_save = umask(077);
		    fp_out = mch_fopen((char *)tempname, WRITEBIN);
		    (void)umask(umask_save);
#else
		    int	fd;

		    // Use mch_open() to be able to use O_NOFOLLOW and set file
		    // protection:
		    // Unix: same as original file, but strip s-bit.  Reset
		    // umask to avoid it getting in the way.
		    // Others: r&w for user only.
# ifdef UNIX
		    umask_save = umask(0);
		    fd = mch_open((char *)tempname,
			    O_CREAT|O_EXTRA|O_EXCL|O_WRONLY|O_NOFOLLOW,
					(int)((st_old.st_mode & 0777) | 0600));
		    (void)umask(umask_save);
# else
		    fd = mch_open((char *)tempname,
			     O_CREAT|O_EXTRA|O_EXCL|O_WRONLY|O_NOFOLLOW, 0600);
# endif
		    if (fd < 0)
		    {
			fp_out = NULL;
# ifdef EEXIST
			// Avoid trying lots of names while the problem is lack
			// of permission, only retry if the file already
			// exists.
			if (errno != EEXIST)
			    break;
# endif
		    }
		    else
			fp_out = fdopen(fd, WRITEBIN);
#endif // VMS
		    if (fp_out != NULL)
			break;
		}

		// Assume file exists, try again with another name.
		if (next_char == 'a' - 1)
		{
		    // They all exist?  Must be something wrong! Don't write
		    // the viminfo file then.
		    semsg(_("E929: Too many viminfo temp files, like %s!"),
								     tempname);
		    break;
		}
		*wp = next_char;
		--next_char;
	    }

	    if (tempname != NULL)
		break;
	    // continue if shortname was set
	}

#if defined(UNIX) && defined(HAVE_FCHOWN)
	if (tempname != NULL && fp_out != NULL)
	{
		stat_T	tmp_st;

	    /*
	     * Make sure the original owner can read/write the tempfile and
	     * otherwise preserve permissions, making sure the group matches.
	     */
	    if (mch_stat((char *)tempname, &tmp_st) >= 0)
	    {
		if (st_old.st_uid != tmp_st.st_uid)
		    // Changing the owner might fail, in which case the
		    // file will now owned by the current user, oh well.
		    vim_ignored = fchown(fileno(fp_out), st_old.st_uid, -1);
		if (st_old.st_gid != tmp_st.st_gid
			&& fchown(fileno(fp_out), -1, st_old.st_gid) == -1)
		    // can't set the group to what it should be, remove
		    // group permissions
		    (void)mch_setperm(tempname, 0600);
	    }
	    else
		// can't stat the file, set conservative permissions
		(void)mch_setperm(tempname, 0600);
	}
#endif
    }

    /*
     * Check if the new viminfo file can be written to.
     */
    if (fp_out == NULL)
    {
	semsg(_("E138: Can't write viminfo file %s!"),
		       (fp_in == NULL || tempname == NULL) ? fname : tempname);
	if (fp_in != NULL)
	    fclose(fp_in);
	goto end;
    }

    if (p_verbose > 0)
    {
	verbose_enter();
	smsg(_("Writing viminfo file \"%s\""), fname);
	verbose_leave();
    }

    viminfo_errcnt = 0;
    do_viminfo(fp_in, fp_out, forceit ? 0 : (VIF_WANT_INFO | VIF_WANT_MARKS));

    if (fclose(fp_out) == EOF)
	++viminfo_errcnt;

    if (fp_in != NULL)
    {
	fclose(fp_in);

	// In case of an error keep the original viminfo file.  Otherwise
	// rename the newly written file.  Give an error if that fails.
	if (viminfo_errcnt == 0)
	{
	    if (vim_rename(tempname, fname) == -1)
	    {
		++viminfo_errcnt;
		semsg(_("E886: Can't rename viminfo file to %s!"), fname);
	    }
# ifdef MSWIN
	    // If the viminfo file was hidden then also hide the new file.
	    else if (hidden)
		mch_hide(fname);
# endif
	}
	if (viminfo_errcnt > 0)
	    mch_remove(tempname);
    }

end:
    vim_free(fname);
    vim_free(tempname);
}

/*
 * Read a line from the viminfo file.
 * Returns TRUE for end-of-file;
 */
    int
viminfo_readline(vir_T *virp)
{
    return vim_fgets(virp->vir_line, LSIZE, virp->vir_fd);
}

/*
 * Check string read from viminfo file.
 * Remove '\n' at the end of the line.
 * - replace CTRL-V CTRL-V with CTRL-V
 * - replace CTRL-V 'n'    with '\n'
 *
 * Check for a long line as written by viminfo_writestring().
 *
 * Return the string in allocated memory (NULL when out of memory).
 */
    char_u *
viminfo_readstring(
    vir_T	*virp,
    int		off,		    // offset for virp->vir_line
    int		convert UNUSED)	    // convert the string
{
    char_u	*retval;
    char_u	*s, *d;
    long	len;

    if (virp->vir_line[off] == Ctrl_V && vim_isdigit(virp->vir_line[off + 1]))
    {
	len = atol((char *)virp->vir_line + off + 1);
	retval = lalloc(len, TRUE);
	if (retval == NULL)
	{
	    // Line too long?  File messed up?  Skip next line.
	    (void)vim_fgets(virp->vir_line, 10, virp->vir_fd);
	    return NULL;
	}
	(void)vim_fgets(retval, (int)len, virp->vir_fd);
	s = retval + 1;	    // Skip the leading '<'
    }
    else
    {
	retval = vim_strsave(virp->vir_line + off);
	if (retval == NULL)
	    return NULL;
	s = retval;
    }

    // Change CTRL-V CTRL-V to CTRL-V and CTRL-V n to \n in-place.
    d = retval;
    while (*s != NUL && *s != '\n')
    {
	if (s[0] == Ctrl_V && s[1] != NUL)
	{
	    if (s[1] == 'n')
		*d++ = '\n';
	    else
		*d++ = Ctrl_V;
	    s += 2;
	}
	else
	    *d++ = *s++;
    }
    *d = NUL;

    if (convert && virp->vir_conv.vc_type != CONV_NONE && *retval != NUL)
    {
	d = string_convert(&virp->vir_conv, retval, NULL);
	if (d != NULL)
	{
	    vim_free(retval);
	    retval = d;
	}
    }

    return retval;
}

/*
 * write string to viminfo file
 * - replace CTRL-V with CTRL-V CTRL-V
 * - replace '\n'   with CTRL-V 'n'
 * - add a '\n' at the end
 *
 * For a long line:
 * - write " CTRL-V <length> \n " in first line
 * - write " < <string> \n "	  in second line
 */
    void
viminfo_writestring(FILE *fd, char_u *p)
{
    int		c;
    char_u	*s;
    int		len = 0;

    for (s = p; *s != NUL; ++s)
    {
	if (*s == Ctrl_V || *s == '\n')
	    ++len;
	++len;
    }

    // If the string will be too long, write its length and put it in the next
    // line.  Take into account that some room is needed for what comes before
    // the string (e.g., variable name).  Add something to the length for the
    // '<', NL and trailing NUL.
    if (len > LSIZE / 2)
	fprintf(fd, IF_EB("\026%d\n<", CTRL_V_STR "%d\n<"), len + 3);

    while ((c = *p++) != NUL)
    {
	if (c == Ctrl_V || c == '\n')
	{
	    putc(Ctrl_V, fd);
	    if (c == '\n')
		c = 'n';
	}
	putc(c, fd);
    }
    putc('\n', fd);
}

/*
 * Write a string in quotes that barline_parse() can read back.
 * Breaks the line in less than LSIZE pieces when needed.
 * Returns remaining characters in the line.
 */
    int
barline_writestring(FILE *fd, char_u *s, int remaining_start)
{
    char_u *p;
    int	    remaining = remaining_start;
    int	    len = 2;

    // Count the number of characters produced, including quotes.
    for (p = s; *p != NUL; ++p)
    {
	if (*p == NL)
	    len += 2;
	else if (*p == '"' || *p == '\\')
	    len += 2;
	else
	    ++len;
    }
    if (len > remaining - 2)
    {
	fprintf(fd, ">%d\n|<", len);
	remaining = LSIZE - 20;
    }

    putc('"', fd);
    for (p = s; *p != NUL; ++p)
    {
	if (*p == NL)
	{
	    putc('\\', fd);
	    putc('n', fd);
	    --remaining;
	}
	else if (*p == '"' || *p == '\\')
	{
	    putc('\\', fd);
	    putc(*p, fd);
	    --remaining;
	}
	else
	    putc(*p, fd);
	--remaining;

	if (remaining < 3)
	{
	    putc('\n', fd);
	    putc('|', fd);
	    putc('<', fd);
	    // Leave enough space for another continuation.
	    remaining = LSIZE - 20;
	}
    }
    putc('"', fd);
    return remaining - 2;
}

/*
 * ":rviminfo" and ":wviminfo".
 */
    void
ex_viminfo(
    exarg_T	*eap)
{
    char_u	*save_viminfo;

    save_viminfo = p_viminfo;
    if (*p_viminfo == NUL)
	p_viminfo = (char_u *)"'100";
    if (eap->cmdidx == CMD_rviminfo)
    {
	if (read_viminfo(eap->arg, VIF_WANT_INFO | VIF_WANT_MARKS
				  | (eap->forceit ? VIF_FORCEIT : 0)) == FAIL)
	    emsg(_("E195: Cannot open viminfo file for reading"));
    }
    else
	write_viminfo(eap->arg, eap->forceit);
    p_viminfo = save_viminfo;
}

#endif // FEAT_VIMINFO
