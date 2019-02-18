/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * ex_cmds.c: some functions for command line commands
 */

#include "vim.h"
#include "version.h"

#ifdef FEAT_FLOAT
# include <float.h>
#endif

static int linelen(int *has_tab);
static void do_filter(linenr_T line1, linenr_T line2, exarg_T *eap, char_u *cmd, int do_in, int do_out);
#ifdef FEAT_VIMINFO
static char_u *viminfo_filename(char_u	*);
static void do_viminfo(FILE *fp_in, FILE *fp_out, int flags);
static int viminfo_encoding(vir_T *virp);
static int read_viminfo_up_to_marks(vir_T *virp, int forceit, int writing);
#endif

static int check_readonly(int *forceit, buf_T *buf);
static void delbuf_msg(char_u *name);
static int
#ifdef __BORLANDC__
    _RTLENTRYF
#endif
	help_compare(const void *s1, const void *s2);
static void prepare_help_buffer(void);

/*
 * ":ascii" and "ga".
 */
    void
do_ascii(exarg_T *eap UNUSED)
{
    int		c;
    int		cval;
    char	buf1[20];
    char	buf2[20];
    char_u	buf3[7];
#ifdef FEAT_DIGRAPHS
    char_u      *dig;
#endif
    int		cc[MAX_MCO];
    int		ci = 0;
    int		len;

    if (enc_utf8)
	c = utfc_ptr2char(ml_get_cursor(), cc);
    else
	c = gchar_cursor();
    if (c == NUL)
    {
	msg("NUL");
	return;
    }

    IObuff[0] = NUL;
    if (!has_mbyte || (enc_dbcs != 0 && c < 0x100) || c < 0x80)
    {
	if (c == NL)	    /* NUL is stored as NL */
	    c = NUL;
	if (c == CAR && get_fileformat(curbuf) == EOL_MAC)
	    cval = NL;	    /* NL is stored as CR */
	else
	    cval = c;
	if (vim_isprintc_strict(c) && (c < ' '
#ifndef EBCDIC
		    || c > '~'
#endif
			       ))
	{
	    transchar_nonprint(buf3, c);
	    vim_snprintf(buf1, sizeof(buf1), "  <%s>", (char *)buf3);
	}
	else
	    buf1[0] = NUL;
#ifndef EBCDIC
	if (c >= 0x80)
	    vim_snprintf(buf2, sizeof(buf2), "  <M-%s>",
						 (char *)transchar(c & 0x7f));
	else
#endif
	    buf2[0] = NUL;
#ifdef FEAT_DIGRAPHS
	dig = get_digraph_for_char(cval);
	if (dig != NULL)
	    vim_snprintf((char *)IObuff, IOSIZE,
		_("<%s>%s%s  %d,  Hex %02x,  Oct %03o, Digr %s"),
			      transchar(c), buf1, buf2, cval, cval, cval, dig);
	else
#endif
	    vim_snprintf((char *)IObuff, IOSIZE,
		_("<%s>%s%s  %d,  Hex %02x,  Octal %03o"),
				  transchar(c), buf1, buf2, cval, cval, cval);
	if (enc_utf8)
	    c = cc[ci++];
	else
	    c = 0;
    }

    /* Repeat for combining characters. */
    while (has_mbyte && (c >= 0x100 || (enc_utf8 && c >= 0x80)))
    {
	len = (int)STRLEN(IObuff);
	/* This assumes every multi-byte char is printable... */
	if (len > 0)
	    IObuff[len++] = ' ';
	IObuff[len++] = '<';
	if (enc_utf8 && utf_iscomposing(c)
# ifdef USE_GUI
		&& !gui.in_use
# endif
		)
	    IObuff[len++] = ' '; /* draw composing char on top of a space */
	len += (*mb_char2bytes)(c, IObuff + len);
#ifdef FEAT_DIGRAPHS
	dig = get_digraph_for_char(c);
	if (dig != NULL)
	    vim_snprintf((char *)IObuff + len, IOSIZE - len,
			c < 0x10000 ? _("> %d, Hex %04x, Oct %o, Digr %s")
				    : _("> %d, Hex %08x, Oct %o, Digr %s"),
					c, c, c, dig);
	else
#endif
	    vim_snprintf((char *)IObuff + len, IOSIZE - len,
			 c < 0x10000 ? _("> %d, Hex %04x, Octal %o")
				     : _("> %d, Hex %08x, Octal %o"),
				     c, c, c);
	if (ci == MAX_MCO)
	    break;
	if (enc_utf8)
	    c = cc[ci++];
	else
	    c = 0;
    }

    msg((char *)IObuff);
}

/*
 * ":left", ":center" and ":right": align text.
 */
    void
ex_align(exarg_T *eap)
{
    pos_T	save_curpos;
    int		len;
    int		indent = 0;
    int		new_indent;
    int		has_tab;
    int		width;

#ifdef FEAT_RIGHTLEFT
    if (curwin->w_p_rl)
    {
	/* switch left and right aligning */
	if (eap->cmdidx == CMD_right)
	    eap->cmdidx = CMD_left;
	else if (eap->cmdidx == CMD_left)
	    eap->cmdidx = CMD_right;
    }
#endif

    width = atoi((char *)eap->arg);
    save_curpos = curwin->w_cursor;
    if (eap->cmdidx == CMD_left)    /* width is used for new indent */
    {
	if (width >= 0)
	    indent = width;
    }
    else
    {
	/*
	 * if 'textwidth' set, use it
	 * else if 'wrapmargin' set, use it
	 * if invalid value, use 80
	 */
	if (width <= 0)
	    width = curbuf->b_p_tw;
	if (width == 0 && curbuf->b_p_wm > 0)
	    width = curwin->w_width - curbuf->b_p_wm;
	if (width <= 0)
	    width = 80;
    }

    if (u_save((linenr_T)(eap->line1 - 1), (linenr_T)(eap->line2 + 1)) == FAIL)
	return;

    for (curwin->w_cursor.lnum = eap->line1;
		 curwin->w_cursor.lnum <= eap->line2; ++curwin->w_cursor.lnum)
    {
	if (eap->cmdidx == CMD_left)		/* left align */
	    new_indent = indent;
	else
	{
	    has_tab = FALSE;	/* avoid uninit warnings */
	    len = linelen(eap->cmdidx == CMD_right ? &has_tab
						   : NULL) - get_indent();

	    if (len <= 0)			/* skip blank lines */
		continue;

	    if (eap->cmdidx == CMD_center)
		new_indent = (width - len) / 2;
	    else
	    {
		new_indent = width - len;	/* right align */

		/*
		 * Make sure that embedded TABs don't make the text go too far
		 * to the right.
		 */
		if (has_tab)
		    while (new_indent > 0)
		    {
			(void)set_indent(new_indent, 0);
			if (linelen(NULL) <= width)
			{
			    /*
			     * Now try to move the line as much as possible to
			     * the right.  Stop when it moves too far.
			     */
			    do
				(void)set_indent(++new_indent, 0);
			    while (linelen(NULL) <= width);
			    --new_indent;
			    break;
			}
			--new_indent;
		    }
	    }
	}
	if (new_indent < 0)
	    new_indent = 0;
	(void)set_indent(new_indent, 0);		/* set indent */
    }
    changed_lines(eap->line1, 0, eap->line2 + 1, 0L);
    curwin->w_cursor = save_curpos;
    beginline(BL_WHITE | BL_FIX);
}

/*
 * Get the length of the current line, excluding trailing white space.
 */
    static int
linelen(int *has_tab)
{
    char_u  *line;
    char_u  *first;
    char_u  *last;
    int	    save;
    int	    len;

    /* find the first non-blank character */
    line = ml_get_curline();
    first = skipwhite(line);

    /* find the character after the last non-blank character */
    for (last = first + STRLEN(first);
				last > first && VIM_ISWHITE(last[-1]); --last)
	;
    save = *last;
    *last = NUL;
    len = linetabsize(line);		/* get line length */
    if (has_tab != NULL)		/* check for embedded TAB */
	*has_tab = (vim_strchr(first, TAB) != NULL);
    *last = save;

    return len;
}

/* Buffer for two lines used during sorting.  They are allocated to
 * contain the longest line being sorted. */
static char_u	*sortbuf1;
static char_u	*sortbuf2;

static int	sort_ic;	/* ignore case */
static int	sort_nr;	/* sort on number */
static int	sort_rx;	/* sort on regex instead of skipping it */
#ifdef FEAT_FLOAT
static int	sort_flt;	/* sort on floating number */
#endif

static int	sort_abort;	/* flag to indicate if sorting has been interrupted */

/* Struct to store info to be sorted. */
typedef struct
{
    linenr_T	lnum;			/* line number */
    union {
	struct
	{
	    varnumber_T	start_col_nr;		/* starting column number */
	    varnumber_T	end_col_nr;		/* ending column number */
	} line;
	varnumber_T	value;		/* value if sorting by integer */
#ifdef FEAT_FLOAT
	float_T value_flt;	/* value if sorting by float */
#endif
    } st_u;
} sorti_T;

static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
sort_compare(const void *s1, const void *s2);

    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
sort_compare(const void *s1, const void *s2)
{
    sorti_T	l1 = *(sorti_T *)s1;
    sorti_T	l2 = *(sorti_T *)s2;
    int		result = 0;

    /* If the user interrupts, there's no way to stop qsort() immediately, but
     * if we return 0 every time, qsort will assume it's done sorting and
     * exit. */
    if (sort_abort)
	return 0;
    fast_breakcheck();
    if (got_int)
	sort_abort = TRUE;

    /* When sorting numbers "start_col_nr" is the number, not the column
     * number. */
    if (sort_nr)
	result = l1.st_u.value == l2.st_u.value ? 0
				 : l1.st_u.value > l2.st_u.value ? 1 : -1;
#ifdef FEAT_FLOAT
    else if (sort_flt)
	result = l1.st_u.value_flt == l2.st_u.value_flt ? 0
			     : l1.st_u.value_flt > l2.st_u.value_flt ? 1 : -1;
#endif
    else
    {
	/* We need to copy one line into "sortbuf1", because there is no
	 * guarantee that the first pointer becomes invalid when obtaining the
	 * second one. */
	STRNCPY(sortbuf1, ml_get(l1.lnum) + l1.st_u.line.start_col_nr,
		     l1.st_u.line.end_col_nr - l1.st_u.line.start_col_nr + 1);
	sortbuf1[l1.st_u.line.end_col_nr - l1.st_u.line.start_col_nr] = 0;
	STRNCPY(sortbuf2, ml_get(l2.lnum) + l2.st_u.line.start_col_nr,
		     l2.st_u.line.end_col_nr - l2.st_u.line.start_col_nr + 1);
	sortbuf2[l2.st_u.line.end_col_nr - l2.st_u.line.start_col_nr] = 0;

	result = sort_ic ? STRICMP(sortbuf1, sortbuf2)
						 : STRCMP(sortbuf1, sortbuf2);
    }

    /* If two lines have the same value, preserve the original line order. */
    if (result == 0)
	return (int)(l1.lnum - l2.lnum);
    return result;
}

/*
 * ":sort".
 */
    void
ex_sort(exarg_T *eap)
{
    regmatch_T	regmatch;
    int		len;
    linenr_T	lnum;
    long	maxlen = 0;
    sorti_T	*nrs;
    size_t	count = (size_t)(eap->line2 - eap->line1 + 1);
    size_t	i;
    char_u	*p;
    char_u	*s;
    char_u	*s2;
    char_u	c;			/* temporary character storage */
    int		unique = FALSE;
    long	deleted;
    colnr_T	start_col;
    colnr_T	end_col;
    int		sort_what = 0;
    int		format_found = 0;
    int		change_occurred = FALSE; // Buffer contents changed.

    /* Sorting one line is really quick! */
    if (count <= 1)
	return;

    if (u_save((linenr_T)(eap->line1 - 1), (linenr_T)(eap->line2 + 1)) == FAIL)
	return;
    sortbuf1 = NULL;
    sortbuf2 = NULL;
    regmatch.regprog = NULL;
    nrs = (sorti_T *)lalloc((long_u)(count * sizeof(sorti_T)), TRUE);
    if (nrs == NULL)
	goto sortend;

    sort_abort = sort_ic = sort_rx = sort_nr = 0;
#ifdef FEAT_FLOAT
    sort_flt = 0;
#endif

    for (p = eap->arg; *p != NUL; ++p)
    {
	if (VIM_ISWHITE(*p))
	    ;
	else if (*p == 'i')
	    sort_ic = TRUE;
	else if (*p == 'r')
	    sort_rx = TRUE;
	else if (*p == 'n')
	{
	    sort_nr = 1;
	    ++format_found;
	}
#ifdef FEAT_FLOAT
	else if (*p == 'f')
	{
	    sort_flt = 1;
	    ++format_found;
	}
#endif
	else if (*p == 'b')
	{
	    sort_what = STR2NR_BIN + STR2NR_FORCE;
	    ++format_found;
	}
	else if (*p == 'o')
	{
	    sort_what = STR2NR_OCT + STR2NR_FORCE;
	    ++format_found;
	}
	else if (*p == 'x')
	{
	    sort_what = STR2NR_HEX + STR2NR_FORCE;
	    ++format_found;
	}
	else if (*p == 'u')
	    unique = TRUE;
	else if (*p == '"')	/* comment start */
	    break;
	else if (check_nextcmd(p) != NULL)
	{
	    eap->nextcmd = check_nextcmd(p);
	    break;
	}
	else if (!ASCII_ISALPHA(*p) && regmatch.regprog == NULL)
	{
	    s = skip_regexp(p + 1, *p, TRUE, NULL);
	    if (*s != *p)
	    {
		emsg(_(e_invalpat));
		goto sortend;
	    }
	    *s = NUL;
	    /* Use last search pattern if sort pattern is empty. */
	    if (s == p + 1)
	    {
		if (last_search_pat() == NULL)
		{
		    emsg(_(e_noprevre));
		    goto sortend;
		}
		regmatch.regprog = vim_regcomp(last_search_pat(), RE_MAGIC);
	    }
	    else
		regmatch.regprog = vim_regcomp(p + 1, RE_MAGIC);
	    if (regmatch.regprog == NULL)
		goto sortend;
	    p = s;		/* continue after the regexp */
	    regmatch.rm_ic = p_ic;
	}
	else
	{
	    semsg(_(e_invarg2), p);
	    goto sortend;
	}
    }

    /* Can only have one of 'n', 'b', 'o' and 'x'. */
    if (format_found > 1)
    {
	emsg(_(e_invarg));
	goto sortend;
    }

    /* From here on "sort_nr" is used as a flag for any integer number
     * sorting. */
    sort_nr += sort_what;

    /*
     * Make an array with all line numbers.  This avoids having to copy all
     * the lines into allocated memory.
     * When sorting on strings "start_col_nr" is the offset in the line, for
     * numbers sorting it's the number to sort on.  This means the pattern
     * matching and number conversion only has to be done once per line.
     * Also get the longest line length for allocating "sortbuf".
     */
    for (lnum = eap->line1; lnum <= eap->line2; ++lnum)
    {
	s = ml_get(lnum);
	len = (int)STRLEN(s);
	if (maxlen < len)
	    maxlen = len;

	start_col = 0;
	end_col = len;
	if (regmatch.regprog != NULL && vim_regexec(&regmatch, s, 0))
	{
	    if (sort_rx)
	    {
		start_col = (colnr_T)(regmatch.startp[0] - s);
		end_col = (colnr_T)(regmatch.endp[0] - s);
	    }
	    else
		start_col = (colnr_T)(regmatch.endp[0] - s);
	}
	else
	    if (regmatch.regprog != NULL)
		end_col = 0;

	if (sort_nr
#ifdef FEAT_FLOAT
		|| sort_flt
#endif
		)
	{
	    /* Make sure vim_str2nr doesn't read any digits past the end
	     * of the match, by temporarily terminating the string there */
	    s2 = s + end_col;
	    c = *s2;
	    *s2 = NUL;
	    /* Sorting on number: Store the number itself. */
	    p = s + start_col;
	    if (sort_nr)
	    {
		if (sort_what & STR2NR_HEX)
		    s = skiptohex(p);
		else if (sort_what & STR2NR_BIN)
		    s = skiptobin(p);
		else
		    s = skiptodigit(p);
		if (s > p && s[-1] == '-')
		    --s;  /* include preceding negative sign */
		if (*s == NUL)
		    /* empty line should sort before any number */
		    nrs[lnum - eap->line1].st_u.value = -MAXLNUM;
		else
		    vim_str2nr(s, NULL, NULL, sort_what,
			       &nrs[lnum - eap->line1].st_u.value, NULL, 0);
	    }
#ifdef FEAT_FLOAT
	    else
	    {
		s = skipwhite(p);
		if (*s == '+')
		    s = skipwhite(s + 1);

		if (*s == NUL)
		    /* empty line should sort before any number */
		    nrs[lnum - eap->line1].st_u.value_flt = -DBL_MAX;
		else
		    nrs[lnum - eap->line1].st_u.value_flt =
						      strtod((char *)s, NULL);
	    }
#endif
	    *s2 = c;
	}
	else
	{
	    /* Store the column to sort at. */
	    nrs[lnum - eap->line1].st_u.line.start_col_nr = start_col;
	    nrs[lnum - eap->line1].st_u.line.end_col_nr = end_col;
	}

	nrs[lnum - eap->line1].lnum = lnum;

	if (regmatch.regprog != NULL)
	    fast_breakcheck();
	if (got_int)
	    goto sortend;
    }

    /* Allocate a buffer that can hold the longest line. */
    sortbuf1 = alloc((unsigned)maxlen + 1);
    if (sortbuf1 == NULL)
	goto sortend;
    sortbuf2 = alloc((unsigned)maxlen + 1);
    if (sortbuf2 == NULL)
	goto sortend;

    /* Sort the array of line numbers.  Note: can't be interrupted! */
    qsort((void *)nrs, count, sizeof(sorti_T), sort_compare);

    if (sort_abort)
	goto sortend;

    /* Insert the lines in the sorted order below the last one. */
    lnum = eap->line2;
    for (i = 0; i < count; ++i)
    {
	linenr_T get_lnum = nrs[eap->forceit ? count - i - 1 : i].lnum;

	// If the original line number of the line being placed is not the same
	// as "lnum" (accounting for offset), we know that the buffer changed.
	if (get_lnum + ((linenr_T)count - 1) != lnum)
	    change_occurred = TRUE;

	s = ml_get(get_lnum);
	if (!unique || i == 0
		|| (sort_ic ? STRICMP(s, sortbuf1) : STRCMP(s, sortbuf1)) != 0)
	{
	    // Copy the line into a buffer, it may become invalid in
	    // ml_append(). And it's needed for "unique".
	    STRCPY(sortbuf1, s);
	    if (ml_append(lnum++, sortbuf1, (colnr_T)0, FALSE) == FAIL)
		break;
	}
	fast_breakcheck();
	if (got_int)
	    goto sortend;
    }

    /* delete the original lines if appending worked */
    if (i == count)
	for (i = 0; i < count; ++i)
	    ml_delete(eap->line1, FALSE);
    else
	count = 0;

    /* Adjust marks for deleted (or added) lines and prepare for displaying. */
    deleted = (long)(count - (lnum - eap->line2));
    if (deleted > 0)
    {
	mark_adjust(eap->line2 - deleted, eap->line2, (long)MAXLNUM, -deleted);
	msgmore(-deleted);
    }
    else if (deleted < 0)
	mark_adjust(eap->line2, MAXLNUM, -deleted, 0L);

    if (change_occurred || deleted != 0)
	changed_lines(eap->line1, 0, eap->line2 + 1, -deleted);

    curwin->w_cursor.lnum = eap->line1;
    beginline(BL_WHITE | BL_FIX);

sortend:
    vim_free(nrs);
    vim_free(sortbuf1);
    vim_free(sortbuf2);
    vim_regfree(regmatch.regprog);
    if (got_int)
	emsg(_(e_interr));
}

/*
 * ":retab".
 */
    void
ex_retab(exarg_T *eap)
{
    linenr_T	lnum;
    int		got_tab = FALSE;
    long	num_spaces = 0;
    long	num_tabs;
    long	len;
    long	col;
    long	vcol;
    long	start_col = 0;		/* For start of white-space string */
    long	start_vcol = 0;		/* For start of white-space string */
    long	old_len;
    char_u	*ptr;
    char_u	*new_line = (char_u *)1;    /* init to non-NULL */
    int		did_undo;		/* called u_save for current line */
#ifdef FEAT_VARTABS
    int		*new_vts_array = NULL;
    char_u	*new_ts_str;		/* string value of tab argument */
#else
    int		temp;
    int		new_ts;
#endif
    int		save_list;
    linenr_T	first_line = 0;		/* first changed line */
    linenr_T	last_line = 0;		/* last changed line */

    save_list = curwin->w_p_list;
    curwin->w_p_list = 0;	    /* don't want list mode here */

#ifdef FEAT_VARTABS
    new_ts_str = eap->arg;
    if (!tabstop_set(eap->arg, &new_vts_array))
	return;
    while (vim_isdigit(*(eap->arg)) || *(eap->arg) == ',')
	++(eap->arg);

    // This ensures that either new_vts_array and new_ts_str are freshly
    // allocated, or new_vts_array points to an existing array and new_ts_str
    // is null.
    if (new_vts_array == NULL)
    {
	new_vts_array = curbuf->b_p_vts_array;
	new_ts_str = NULL;
    }
    else
	new_ts_str = vim_strnsave(new_ts_str, eap->arg - new_ts_str);
#else
    new_ts = getdigits(&(eap->arg));
    if (new_ts < 0)
    {
	emsg(_(e_positive));
	return;
    }
    if (new_ts == 0)
	new_ts = curbuf->b_p_ts;
#endif
    for (lnum = eap->line1; !got_int && lnum <= eap->line2; ++lnum)
    {
	ptr = ml_get(lnum);
	col = 0;
	vcol = 0;
	did_undo = FALSE;
	for (;;)
	{
	    if (VIM_ISWHITE(ptr[col]))
	    {
		if (!got_tab && num_spaces == 0)
		{
		    /* First consecutive white-space */
		    start_vcol = vcol;
		    start_col = col;
		}
		if (ptr[col] == ' ')
		    num_spaces++;
		else
		    got_tab = TRUE;
	    }
	    else
	    {
		if (got_tab || (eap->forceit && num_spaces > 1))
		{
		    /* Retabulate this string of white-space */

		    /* len is virtual length of white string */
		    len = num_spaces = vcol - start_vcol;
		    num_tabs = 0;
		    if (!curbuf->b_p_et)
		    {
#ifdef FEAT_VARTABS
			int t, s;

			tabstop_fromto(start_vcol, vcol,
					curbuf->b_p_ts, new_vts_array, &t, &s);
			num_tabs = t;
			num_spaces = s;
#else
			temp = new_ts - (start_vcol % new_ts);
			if (num_spaces >= temp)
			{
			    num_spaces -= temp;
			    num_tabs++;
			}
			num_tabs += num_spaces / new_ts;
			num_spaces -= (num_spaces / new_ts) * new_ts;
#endif
		    }
		    if (curbuf->b_p_et || got_tab ||
					(num_spaces + num_tabs < len))
		    {
			if (did_undo == FALSE)
			{
			    did_undo = TRUE;
			    if (u_save((linenr_T)(lnum - 1),
						(linenr_T)(lnum + 1)) == FAIL)
			    {
				new_line = NULL;	/* flag out-of-memory */
				break;
			    }
			}

			/* len is actual number of white characters used */
			len = num_spaces + num_tabs;
			old_len = (long)STRLEN(ptr);
			new_line = lalloc(old_len - col + start_col + len + 1,
									TRUE);
			if (new_line == NULL)
			    break;
			if (start_col > 0)
			    mch_memmove(new_line, ptr, (size_t)start_col);
			mch_memmove(new_line + start_col + len,
				      ptr + col, (size_t)(old_len - col + 1));
			ptr = new_line + start_col;
			for (col = 0; col < len; col++)
			    ptr[col] = (col < num_tabs) ? '\t' : ' ';
			ml_replace(lnum, new_line, FALSE);
			if (first_line == 0)
			    first_line = lnum;
			last_line = lnum;
			ptr = new_line;
			col = start_col + len;
		    }
		}
		got_tab = FALSE;
		num_spaces = 0;
	    }
	    if (ptr[col] == NUL)
		break;
	    vcol += chartabsize(ptr + col, (colnr_T)vcol);
	    if (has_mbyte)
		col += (*mb_ptr2len)(ptr + col);
	    else
		++col;
	}
	if (new_line == NULL)		    /* out of memory */
	    break;
	line_breakcheck();
    }
    if (got_int)
	emsg(_(e_interr));

#ifdef FEAT_VARTABS
    // If a single value was given then it can be considered equal to
    // either the value of 'tabstop' or the value of 'vartabstop'.
    if (tabstop_count(curbuf->b_p_vts_array) == 0
	&& tabstop_count(new_vts_array) == 1
	&& curbuf->b_p_ts == tabstop_first(new_vts_array))
	; /* not changed */
    else if (tabstop_count(curbuf->b_p_vts_array) > 0
        && tabstop_eq(curbuf->b_p_vts_array, new_vts_array))
	; /* not changed */
    else
	redraw_curbuf_later(NOT_VALID);
#else
    if (curbuf->b_p_ts != new_ts)
	redraw_curbuf_later(NOT_VALID);
#endif
    if (first_line != 0)
	changed_lines(first_line, 0, last_line + 1, 0L);

    curwin->w_p_list = save_list;	/* restore 'list' */

#ifdef FEAT_VARTABS
    if (new_ts_str != NULL)		/* set the new tabstop */
    {
	// If 'vartabstop' is in use or if the value given to retab has more
	// than one tabstop then update 'vartabstop'.
	int *old_vts_ary = curbuf->b_p_vts_array;

	if (tabstop_count(old_vts_ary) > 0 || tabstop_count(new_vts_array) > 1)
	{
	    set_string_option_direct((char_u *)"vts", -1, new_ts_str,
							OPT_FREE|OPT_LOCAL, 0);
	    curbuf->b_p_vts_array = new_vts_array;
	    vim_free(old_vts_ary);
	}
	else
	{
	    // 'vartabstop' wasn't in use and a single value was given to
	    // retab then update 'tabstop'.
	    curbuf->b_p_ts = tabstop_first(new_vts_array);
	    vim_free(new_vts_array);
	}
	vim_free(new_ts_str);
    }
#else
    curbuf->b_p_ts = new_ts;
#endif
    coladvance(curwin->w_curswant);

    u_clearline();
}

/*
 * :move command - move lines line1-line2 to line dest
 *
 * return FAIL for failure, OK otherwise
 */
    int
do_move(linenr_T line1, linenr_T line2, linenr_T dest)
{
    char_u	*str;
    linenr_T	l;
    linenr_T	extra;	    // Num lines added before line1
    linenr_T	num_lines;  // Num lines moved
    linenr_T	last_line;  // Last line in file after adding new text
#ifdef FEAT_FOLDING
    win_T	*win;
    tabpage_T	*tp;
#endif

    if (dest >= line1 && dest < line2)
    {
	emsg(_("E134: Cannot move a range of lines into itself"));
	return FAIL;
    }

    // Do nothing if we are not actually moving any lines.  This will prevent
    // the 'modified' flag from being set without cause.
    if (dest == line1 - 1 || dest == line2)
    {
	// Move the cursor as if lines were moved (see below) to be backwards
	// compatible.
	if (dest >= line1)
	    curwin->w_cursor.lnum = dest;
	else
	    curwin->w_cursor.lnum = dest + (line2 - line1) + 1;

	return OK;
    }

    num_lines = line2 - line1 + 1;

    /*
     * First we copy the old text to its new location -- webb
     * Also copy the flag that ":global" command uses.
     */
    if (u_save(dest, dest + 1) == FAIL)
	return FAIL;
    for (extra = 0, l = line1; l <= line2; l++)
    {
	str = vim_strsave(ml_get(l + extra));
	if (str != NULL)
	{
	    ml_append(dest + l - line1, str, (colnr_T)0, FALSE);
	    vim_free(str);
	    if (dest < line1)
		extra++;
	}
    }

    /*
     * Now we must be careful adjusting our marks so that we don't overlap our
     * mark_adjust() calls.
     *
     * We adjust the marks within the old text so that they refer to the
     * last lines of the file (temporarily), because we know no other marks
     * will be set there since these line numbers did not exist until we added
     * our new lines.
     *
     * Then we adjust the marks on lines between the old and new text positions
     * (either forwards or backwards).
     *
     * And Finally we adjust the marks we put at the end of the file back to
     * their final destination at the new text position -- webb
     */
    last_line = curbuf->b_ml.ml_line_count;
    mark_adjust_nofold(line1, line2, last_line - line2, 0L);
    if (dest >= line2)
    {
	mark_adjust_nofold(line2 + 1, dest, -num_lines, 0L);
#ifdef FEAT_FOLDING
	FOR_ALL_TAB_WINDOWS(tp, win) {
	    if (win->w_buffer == curbuf)
		foldMoveRange(&win->w_folds, line1, line2, dest);
	}
#endif
	curbuf->b_op_start.lnum = dest - num_lines + 1;
	curbuf->b_op_end.lnum = dest;
    }
    else
    {
	mark_adjust_nofold(dest + 1, line1 - 1, num_lines, 0L);
#ifdef FEAT_FOLDING
	FOR_ALL_TAB_WINDOWS(tp, win) {
	    if (win->w_buffer == curbuf)
		foldMoveRange(&win->w_folds, dest + 1, line1 - 1, line2);
	}
#endif
	curbuf->b_op_start.lnum = dest + 1;
	curbuf->b_op_end.lnum = dest + num_lines;
    }
    curbuf->b_op_start.col = curbuf->b_op_end.col = 0;
    mark_adjust_nofold(last_line - num_lines + 1, last_line,
					     -(last_line - dest - extra), 0L);

    /*
     * Now we delete the original text -- webb
     */
    if (u_save(line1 + extra - 1, line2 + extra + 1) == FAIL)
	return FAIL;

    for (l = line1; l <= line2; l++)
	ml_delete(line1 + extra, TRUE);

    if (!global_busy && num_lines > p_report)
	smsg(NGETTEXT("%ld line moved", "%ld lines moved", num_lines),
			(long)num_lines);

    /*
     * Leave the cursor on the last of the moved lines.
     */
    if (dest >= line1)
	curwin->w_cursor.lnum = dest;
    else
	curwin->w_cursor.lnum = dest + (line2 - line1) + 1;

    if (line1 < dest)
    {
	dest += num_lines + 1;
	last_line = curbuf->b_ml.ml_line_count;
	if (dest > last_line + 1)
	    dest = last_line + 1;
	changed_lines(line1, 0, dest, 0L);
    }
    else
	changed_lines(dest + 1, 0, line1 + num_lines, 0L);

    return OK;
}

/*
 * ":copy"
 */
    void
ex_copy(linenr_T line1, linenr_T line2, linenr_T n)
{
    linenr_T	count;
    char_u	*p;

    count = line2 - line1 + 1;
    curbuf->b_op_start.lnum = n + 1;
    curbuf->b_op_end.lnum = n + count;
    curbuf->b_op_start.col = curbuf->b_op_end.col = 0;

    /*
     * there are three situations:
     * 1. destination is above line1
     * 2. destination is between line1 and line2
     * 3. destination is below line2
     *
     * n = destination (when starting)
     * curwin->w_cursor.lnum = destination (while copying)
     * line1 = start of source (while copying)
     * line2 = end of source (while copying)
     */
    if (u_save(n, n + 1) == FAIL)
	return;

    curwin->w_cursor.lnum = n;
    while (line1 <= line2)
    {
	/* need to use vim_strsave() because the line will be unlocked within
	 * ml_append() */
	p = vim_strsave(ml_get(line1));
	if (p != NULL)
	{
	    ml_append(curwin->w_cursor.lnum, p, (colnr_T)0, FALSE);
	    vim_free(p);
	}
	/* situation 2: skip already copied lines */
	if (line1 == n)
	    line1 = curwin->w_cursor.lnum;
	++line1;
	if (curwin->w_cursor.lnum < line1)
	    ++line1;
	if (curwin->w_cursor.lnum < line2)
	    ++line2;
	++curwin->w_cursor.lnum;
    }

    appended_lines_mark(n, count);

    msgmore((long)count);
}

static char_u	*prevcmd = NULL;	/* the previous command */

#if defined(EXITFREE) || defined(PROTO)
    void
free_prev_shellcmd(void)
{
    vim_free(prevcmd);
}
#endif

/*
 * Handle the ":!cmd" command.	Also for ":r !cmd" and ":w !cmd"
 * Bangs in the argument are replaced with the previously entered command.
 * Remember the argument.
 */
    void
do_bang(
    int		addr_count,
    exarg_T	*eap,
    int		forceit,
    int		do_in,
    int		do_out)
{
    char_u		*arg = eap->arg;	/* command */
    linenr_T		line1 = eap->line1;	/* start of range */
    linenr_T		line2 = eap->line2;	/* end of range */
    char_u		*newcmd = NULL;		/* the new command */
    int			free_newcmd = FALSE;    /* need to free() newcmd */
    int			ins_prevcmd;
    char_u		*t;
    char_u		*p;
    char_u		*trailarg;
    int			len;
    int			scroll_save = msg_scroll;

    /*
     * Disallow shell commands for "rvim".
     * Disallow shell commands from .exrc and .vimrc in current directory for
     * security reasons.
     */
    if (check_restricted() || check_secure())
	return;

    if (addr_count == 0)		/* :! */
    {
	msg_scroll = FALSE;	    /* don't scroll here */
	autowrite_all();
	msg_scroll = scroll_save;
    }

    /*
     * Try to find an embedded bang, like in :!<cmd> ! [args]
     * (:!! is indicated by the 'forceit' variable)
     */
    ins_prevcmd = forceit;
    trailarg = arg;
    do
    {
	len = (int)STRLEN(trailarg) + 1;
	if (newcmd != NULL)
	    len += (int)STRLEN(newcmd);
	if (ins_prevcmd)
	{
	    if (prevcmd == NULL)
	    {
		emsg(_(e_noprev));
		vim_free(newcmd);
		return;
	    }
	    len += (int)STRLEN(prevcmd);
	}
	if ((t = alloc((unsigned)len)) == NULL)
	{
	    vim_free(newcmd);
	    return;
	}
	*t = NUL;
	if (newcmd != NULL)
	    STRCAT(t, newcmd);
	if (ins_prevcmd)
	    STRCAT(t, prevcmd);
	p = t + STRLEN(t);
	STRCAT(t, trailarg);
	vim_free(newcmd);
	newcmd = t;

	/*
	 * Scan the rest of the argument for '!', which is replaced by the
	 * previous command.  "\!" is replaced by "!" (this is vi compatible).
	 */
	trailarg = NULL;
	while (*p)
	{
	    if (*p == '!')
	    {
		if (p > newcmd && p[-1] == '\\')
		    STRMOVE(p - 1, p);
		else
		{
		    trailarg = p;
		    *trailarg++ = NUL;
		    ins_prevcmd = TRUE;
		    break;
		}
	    }
	    ++p;
	}
    } while (trailarg != NULL);

    vim_free(prevcmd);
    prevcmd = newcmd;

    if (bangredo)	    /* put cmd in redo buffer for ! command */
    {
	/* If % or # appears in the command, it must have been escaped.
	 * Reescape them, so that redoing them does not substitute them by the
	 * buffername. */
	char_u *cmd = vim_strsave_escaped(prevcmd, (char_u *)"%#");

	if (cmd != NULL)
	{
	    AppendToRedobuffLit(cmd, -1);
	    vim_free(cmd);
	}
	else
	    AppendToRedobuffLit(prevcmd, -1);
	AppendToRedobuff((char_u *)"\n");
	bangredo = FALSE;
    }
    /*
     * Add quotes around the command, for shells that need them.
     */
    if (*p_shq != NUL)
    {
	newcmd = alloc((unsigned)(STRLEN(prevcmd) + 2 * STRLEN(p_shq) + 1));
	if (newcmd == NULL)
	    return;
	STRCPY(newcmd, p_shq);
	STRCAT(newcmd, prevcmd);
	STRCAT(newcmd, p_shq);
	free_newcmd = TRUE;
    }
    if (addr_count == 0)		/* :! */
    {
	/* echo the command */
	msg_start();
	msg_putchar(':');
	msg_putchar('!');
	msg_outtrans(newcmd);
	msg_clr_eos();
	windgoto(msg_row, msg_col);

	do_shell(newcmd, 0);
    }
    else				/* :range! */
    {
	/* Careful: This may recursively call do_bang() again! (because of
	 * autocommands) */
	do_filter(line1, line2, eap, newcmd, do_in, do_out);
	apply_autocmds(EVENT_SHELLFILTERPOST, NULL, NULL, FALSE, curbuf);
    }
    if (free_newcmd)
	vim_free(newcmd);
}

/*
 * do_filter: filter lines through a command given by the user
 *
 * We mostly use temp files and the call_shell() routine here. This would
 * normally be done using pipes on a UNIX machine, but this is more portable
 * to non-unix machines. The call_shell() routine needs to be able
 * to deal with redirection somehow, and should handle things like looking
 * at the PATH env. variable, and adding reasonable extensions to the
 * command name given by the user. All reasonable versions of call_shell()
 * do this.
 * Alternatively, if on Unix and redirecting input or output, but not both,
 * and the 'shelltemp' option isn't set, use pipes.
 * We use input redirection if do_in is TRUE.
 * We use output redirection if do_out is TRUE.
 */
    static void
do_filter(
    linenr_T	line1,
    linenr_T	line2,
    exarg_T	*eap,		/* for forced 'ff' and 'fenc' */
    char_u	*cmd,
    int		do_in,
    int		do_out)
{
    char_u	*itmp = NULL;
    char_u	*otmp = NULL;
    linenr_T	linecount;
    linenr_T	read_linecount;
    pos_T	cursor_save;
    char_u	*cmd_buf;
    buf_T	*old_curbuf = curbuf;
    int		shell_flags = 0;

    if (*cmd == NUL)	    /* no filter command */
	return;

    cursor_save = curwin->w_cursor;
    linecount = line2 - line1 + 1;
    curwin->w_cursor.lnum = line1;
    curwin->w_cursor.col = 0;
    changed_line_abv_curs();
    invalidate_botline();

    /*
     * When using temp files:
     * 1. * Form temp file names
     * 2. * Write the lines to a temp file
     * 3.   Run the filter command on the temp file
     * 4. * Read the output of the command into the buffer
     * 5. * Delete the original lines to be filtered
     * 6. * Remove the temp files
     *
     * When writing the input with a pipe or when catching the output with a
     * pipe only need to do 3.
     */

    if (do_out)
	shell_flags |= SHELL_DOOUT;

#ifdef FEAT_FILTERPIPE
    if (!do_in && do_out && !p_stmp)
    {
	/* Use a pipe to fetch stdout of the command, do not use a temp file. */
	shell_flags |= SHELL_READ;
	curwin->w_cursor.lnum = line2;
    }
    else if (do_in && !do_out && !p_stmp)
    {
	/* Use a pipe to write stdin of the command, do not use a temp file. */
	shell_flags |= SHELL_WRITE;
	curbuf->b_op_start.lnum = line1;
	curbuf->b_op_end.lnum = line2;
    }
    else if (do_in && do_out && !p_stmp)
    {
	/* Use a pipe to write stdin and fetch stdout of the command, do not
	 * use a temp file. */
	shell_flags |= SHELL_READ|SHELL_WRITE;
	curbuf->b_op_start.lnum = line1;
	curbuf->b_op_end.lnum = line2;
	curwin->w_cursor.lnum = line2;
    }
    else
#endif
	if ((do_in && (itmp = vim_tempname('i', FALSE)) == NULL)
		|| (do_out && (otmp = vim_tempname('o', FALSE)) == NULL))
	{
	    emsg(_(e_notmp));
	    goto filterend;
	}

/*
 * The writing and reading of temp files will not be shown.
 * Vi also doesn't do this and the messages are not very informative.
 */
    ++no_wait_return;		/* don't call wait_return() while busy */
    if (itmp != NULL && buf_write(curbuf, itmp, NULL, line1, line2, eap,
					   FALSE, FALSE, FALSE, TRUE) == FAIL)
    {
	msg_putchar('\n');		/* keep message from buf_write() */
	--no_wait_return;
#if defined(FEAT_EVAL)
	if (!aborting())
#endif
	    (void)semsg(_(e_notcreate), itmp);	/* will call wait_return */
	goto filterend;
    }
    if (curbuf != old_curbuf)
	goto filterend;

    if (!do_out)
	msg_putchar('\n');

    /* Create the shell command in allocated memory. */
    cmd_buf = make_filter_cmd(cmd, itmp, otmp);
    if (cmd_buf == NULL)
	goto filterend;

    windgoto((int)Rows - 1, 0);
    cursor_on();

    /*
     * When not redirecting the output the command can write anything to the
     * screen. If 'shellredir' is equal to ">", screen may be messed up by
     * stderr output of external command. Clear the screen later.
     * If do_in is FALSE, this could be something like ":r !cat", which may
     * also mess up the screen, clear it later.
     */
    if (!do_out || STRCMP(p_srr, ">") == 0 || !do_in)
	redraw_later_clear();

    if (do_out)
    {
	if (u_save((linenr_T)(line2), (linenr_T)(line2 + 1)) == FAIL)
	{
	    vim_free(cmd_buf);
	    goto error;
	}
	redraw_curbuf_later(VALID);
    }
    read_linecount = curbuf->b_ml.ml_line_count;

    /*
     * When call_shell() fails wait_return() is called to give the user a
     * chance to read the error messages. Otherwise errors are ignored, so you
     * can see the error messages from the command that appear on stdout; use
     * 'u' to fix the text
     * Switch to cooked mode when not redirecting stdin, avoids that something
     * like ":r !cat" hangs.
     * Pass on the SHELL_DOOUT flag when the output is being redirected.
     */
    if (call_shell(cmd_buf, SHELL_FILTER | SHELL_COOKED | shell_flags))
    {
	redraw_later_clear();
	wait_return(FALSE);
    }
    vim_free(cmd_buf);

    did_check_timestamps = FALSE;
    need_check_timestamps = TRUE;

    /* When interrupting the shell command, it may still have produced some
     * useful output.  Reset got_int here, so that readfile() won't cancel
     * reading. */
    ui_breakcheck();
    got_int = FALSE;

    if (do_out)
    {
	if (otmp != NULL)
	{
	    if (readfile(otmp, NULL, line2, (linenr_T)0, (linenr_T)MAXLNUM,
						    eap, READ_FILTER) != OK)
	    {
#if defined(FEAT_EVAL)
		if (!aborting())
#endif
		{
		    msg_putchar('\n');
		    semsg(_(e_notread), otmp);
		}
		goto error;
	    }
	    if (curbuf != old_curbuf)
		goto filterend;
	}

	read_linecount = curbuf->b_ml.ml_line_count - read_linecount;

	if (shell_flags & SHELL_READ)
	{
	    curbuf->b_op_start.lnum = line2 + 1;
	    curbuf->b_op_end.lnum = curwin->w_cursor.lnum;
	    appended_lines_mark(line2, read_linecount);
	}

	if (do_in)
	{
	    if (cmdmod.keepmarks || vim_strchr(p_cpo, CPO_REMMARK) == NULL)
	    {
		if (read_linecount >= linecount)
		    /* move all marks from old lines to new lines */
		    mark_adjust(line1, line2, linecount, 0L);
		else
		{
		    /* move marks from old lines to new lines, delete marks
		     * that are in deleted lines */
		    mark_adjust(line1, line1 + read_linecount - 1,
								linecount, 0L);
		    mark_adjust(line1 + read_linecount, line2, MAXLNUM, 0L);
		}
	    }

	    /*
	     * Put cursor on first filtered line for ":range!cmd".
	     * Adjust '[ and '] (set by buf_write()).
	     */
	    curwin->w_cursor.lnum = line1;
	    del_lines(linecount, TRUE);
	    curbuf->b_op_start.lnum -= linecount;	/* adjust '[ */
	    curbuf->b_op_end.lnum -= linecount;		/* adjust '] */
	    write_lnum_adjust(-linecount);		/* adjust last line
							   for next write */
#ifdef FEAT_FOLDING
	    foldUpdate(curwin, curbuf->b_op_start.lnum, curbuf->b_op_end.lnum);
#endif
	}
	else
	{
	    /*
	     * Put cursor on last new line for ":r !cmd".
	     */
	    linecount = curbuf->b_op_end.lnum - curbuf->b_op_start.lnum + 1;
	    curwin->w_cursor.lnum = curbuf->b_op_end.lnum;
	}

	beginline(BL_WHITE | BL_FIX);	    /* cursor on first non-blank */
	--no_wait_return;

	if (linecount > p_report)
	{
	    if (do_in)
	    {
		vim_snprintf(msg_buf, sizeof(msg_buf),
				    _("%ld lines filtered"), (long)linecount);
		if (msg(msg_buf) && !msg_scroll)
		    /* save message to display it after redraw */
		    set_keep_msg((char_u *)msg_buf, 0);
	    }
	    else
		msgmore((long)linecount);
	}
    }
    else
    {
error:
	/* put cursor back in same position for ":w !cmd" */
	curwin->w_cursor = cursor_save;
	--no_wait_return;
	wait_return(FALSE);
    }

filterend:

    if (curbuf != old_curbuf)
    {
	--no_wait_return;
	emsg(_("E135: *Filter* Autocommands must not change current buffer"));
    }
    if (itmp != NULL)
	mch_remove(itmp);
    if (otmp != NULL)
	mch_remove(otmp);
    vim_free(itmp);
    vim_free(otmp);
}

/*
 * Call a shell to execute a command.
 * When "cmd" is NULL start an interactive shell.
 */
    void
do_shell(
    char_u	*cmd,
    int		flags)	/* may be SHELL_DOOUT when output is redirected */
{
    buf_T	*buf;
#ifndef FEAT_GUI_MSWIN
    int		save_nwr;
#endif
#ifdef MSWIN
    int		winstart = FALSE;
#endif

    /*
     * Disallow shell commands for "rvim".
     * Disallow shell commands from .exrc and .vimrc in current directory for
     * security reasons.
     */
    if (check_restricted() || check_secure())
    {
	msg_end();
	return;
    }

#ifdef MSWIN
    /*
     * Check if ":!start" is used.
     */
    if (cmd != NULL)
	winstart = (STRNICMP(cmd, "start ", 6) == 0);
#endif

    /*
     * For autocommands we want to get the output on the current screen, to
     * avoid having to type return below.
     */
    msg_putchar('\r');			/* put cursor at start of line */
    if (!autocmd_busy)
    {
#ifdef MSWIN
	if (!winstart)
#endif
	    stoptermcap();
    }
#ifdef MSWIN
    if (!winstart)
#endif
	msg_putchar('\n');		/* may shift screen one line up */

    /* warning message before calling the shell */
    if (p_warn && !autocmd_busy && msg_silent == 0)
	FOR_ALL_BUFFERS(buf)
	    if (bufIsChangedNotTerm(buf))
	    {
#ifdef FEAT_GUI_MSWIN
		if (!winstart)
		    starttermcap();	/* don't want a message box here */
#endif
		msg_puts(_("[No write since last change]\n"));
#ifdef FEAT_GUI_MSWIN
		if (!winstart)
		    stoptermcap();
#endif
		break;
	    }

    /* This windgoto is required for when the '\n' resulted in a "delete line
     * 1" command to the terminal. */
    if (!swapping_screen())
	windgoto(msg_row, msg_col);
    cursor_on();
    (void)call_shell(cmd, SHELL_COOKED | flags);
    did_check_timestamps = FALSE;
    need_check_timestamps = TRUE;

    /*
     * put the message cursor at the end of the screen, avoids wait_return()
     * to overwrite the text that the external command showed
     */
    if (!swapping_screen())
    {
	msg_row = Rows - 1;
	msg_col = 0;
    }

    if (autocmd_busy)
    {
	if (msg_silent == 0)
	    redraw_later_clear();
    }
    else
    {
	/*
	 * For ":sh" there is no need to call wait_return(), just redraw.
	 * Also for the Win32 GUI (the output is in a console window).
	 * Otherwise there is probably text on the screen that the user wants
	 * to read before redrawing, so call wait_return().
	 */
#ifndef FEAT_GUI_MSWIN
	if (cmd == NULL
# ifdef MSWIN
		|| (winstart && !need_wait_return)
# endif
	   )
	{
	    if (msg_silent == 0)
		redraw_later_clear();
	    need_wait_return = FALSE;
	}
	else
	{
	    /*
	     * If we switch screens when starttermcap() is called, we really
	     * want to wait for "hit return to continue".
	     */
	    save_nwr = no_wait_return;
	    if (swapping_screen())
		no_wait_return = FALSE;
# ifdef AMIGA
	    wait_return(term_console ? -1 : msg_silent == 0);	/* see below */
# else
	    wait_return(msg_silent == 0);
# endif
	    no_wait_return = save_nwr;
	}
#endif /* FEAT_GUI_MSWIN */

#ifdef MSWIN
	if (!winstart) /* if winstart==TRUE, never stopped termcap! */
#endif
	    starttermcap();	/* start termcap if not done by wait_return() */

	/*
	 * In an Amiga window redrawing is caused by asking the window size.
	 * If we got an interrupt this will not work. The chance that the
	 * window size is wrong is very small, but we need to redraw the
	 * screen.  Don't do this if ':' hit in wait_return().	THIS IS UGLY
	 * but it saves an extra redraw.
	 */
#ifdef AMIGA
	if (skip_redraw)		/* ':' hit in wait_return() */
	{
	    if (msg_silent == 0)
		redraw_later_clear();
	}
	else if (term_console)
	{
	    OUT_STR(IF_EB("\033[0 q", ESC_STR "[0 q"));	/* get window size */
	    if (got_int && msg_silent == 0)
		redraw_later_clear();	/* if got_int is TRUE, redraw needed */
	    else
		must_redraw = 0;	/* no extra redraw needed */
	}
#endif
    }

    /* display any error messages now */
    display_errors();

    apply_autocmds(EVENT_SHELLCMDPOST, NULL, NULL, FALSE, curbuf);
}

#if !defined(UNIX)
    static char_u *
find_pipe(char_u *cmd)
{
    char_u  *p;
    int	    inquote = FALSE;

    for (p = cmd; *p != NUL; ++p)
    {
	if (!inquote && *p == '|')
	    return p;
	if (*p == '"')
	    inquote = !inquote;
	else if (rem_backslash(p))
	    ++p;
    }
    return NULL;
}
#endif

/*
 * Create a shell command from a command string, input redirection file and
 * output redirection file.
 * Returns an allocated string with the shell command, or NULL for failure.
 */
    char_u *
make_filter_cmd(
    char_u	*cmd,		/* command */
    char_u	*itmp,		/* NULL or name of input file */
    char_u	*otmp)		/* NULL or name of output file */
{
    char_u	*buf;
    long_u	len;

#if defined(UNIX)
    int		is_fish_shell;
    char_u	*shell_name = get_isolated_shell_name();

    /* Account for fish's different syntax for subshells */
    is_fish_shell = (fnamecmp(shell_name, "fish") == 0);
    vim_free(shell_name);
    if (is_fish_shell)
	len = (long_u)STRLEN(cmd) + 13;		/* "begin; " + "; end" + NUL */
    else
#endif
	len = (long_u)STRLEN(cmd) + 3;			/* "()" + NUL */
    if (itmp != NULL)
	len += (long_u)STRLEN(itmp) + 9;		/* " { < " + " } " */
    if (otmp != NULL)
	len += (long_u)STRLEN(otmp) + (long_u)STRLEN(p_srr) + 2; /* "  " */
    buf = lalloc(len, TRUE);
    if (buf == NULL)
	return NULL;

#if defined(UNIX)
    /*
     * Put braces around the command (for concatenated commands) when
     * redirecting input and/or output.
     */
    if (itmp != NULL || otmp != NULL)
    {
	if (is_fish_shell)
	    vim_snprintf((char *)buf, len, "begin; %s; end", (char *)cmd);
	else
	    vim_snprintf((char *)buf, len, "(%s)", (char *)cmd);
    }
    else
	STRCPY(buf, cmd);
    if (itmp != NULL)
    {
	STRCAT(buf, " < ");
	STRCAT(buf, itmp);
    }
#else
    /*
     * For shells that don't understand braces around commands, at least allow
     * the use of commands in a pipe.
     */
    STRCPY(buf, cmd);
    if (itmp != NULL)
    {
	char_u	*p;

	/*
	 * If there is a pipe, we have to put the '<' in front of it.
	 * Don't do this when 'shellquote' is not empty, otherwise the
	 * redirection would be inside the quotes.
	 */
	if (*p_shq == NUL)
	{
	    p = find_pipe(buf);
	    if (p != NULL)
		*p = NUL;
	}
	STRCAT(buf, " <");	/* " < " causes problems on Amiga */
	STRCAT(buf, itmp);
	if (*p_shq == NUL)
	{
	    p = find_pipe(cmd);
	    if (p != NULL)
	    {
		STRCAT(buf, " ");   /* insert a space before the '|' for DOS */
		STRCAT(buf, p);
	    }
	}
    }
#endif
    if (otmp != NULL)
	append_redir(buf, (int)len, p_srr, otmp);

    return buf;
}

/*
 * Append output redirection for file "fname" to the end of string buffer
 * "buf[buflen]"
 * Works with the 'shellredir' and 'shellpipe' options.
 * The caller should make sure that there is enough room:
 *	STRLEN(opt) + STRLEN(fname) + 3
 */
    void
append_redir(
    char_u	*buf,
    int		buflen,
    char_u	*opt,
    char_u	*fname)
{
    char_u	*p;
    char_u	*end;

    end = buf + STRLEN(buf);
    /* find "%s" */
    for (p = opt; (p = vim_strchr(p, '%')) != NULL; ++p)
    {
	if (p[1] == 's') /* found %s */
	    break;
	if (p[1] == '%') /* skip %% */
	    ++p;
    }
    if (p != NULL)
    {
	*end = ' '; /* not really needed? Not with sh, ksh or bash */
	vim_snprintf((char *)end + 1, (size_t)(buflen - (end + 1 - buf)),
						  (char *)opt, (char *)fname);
    }
    else
	vim_snprintf((char *)end, (size_t)(buflen - (end - buf)),
#ifdef FEAT_QUICKFIX
		" %s %s",
#else
		" %s%s",	/* " > %s" causes problems on Amiga */
#endif
		(char *)opt, (char *)fname);
}

#if defined(FEAT_VIMINFO) || defined(PROTO)

static int read_viminfo_barline(vir_T *virp, int got_encoding, int force, int writing);
static void write_viminfo_version(FILE *fp_out);
static void write_viminfo_barlines(vir_T *virp, FILE *fp_out);
static int  viminfo_errcnt;

    static int
no_viminfo(void)
{
    /* "vim -i NONE" does not read or write a viminfo file */
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
 * read_viminfo() -- Read the viminfo file.  Registers etc. which are already
 * set are not over-written unless "flags" includes VIF_FORCEIT. -- webb
 */
    int
read_viminfo(
    char_u	*file,	    /* file name or NULL to use default name */
    int		flags)	    /* VIF_WANT_INFO et al. */
{
    FILE	*fp;
    char_u	*fname;

    if (no_viminfo())
	return FAIL;

    fname = viminfo_filename(file);	/* get file name in allocated buffer */
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
    FILE	*fp_in = NULL;	/* input viminfo file, if any */
    FILE	*fp_out = NULL;	/* output viminfo file */
    char_u	*tempname = NULL;	/* name of temp viminfo file */
    stat_T	st_new;		/* mch_stat() of potential new file */
#if defined(UNIX) || defined(VMS)
    mode_t	umask_save;
#endif
#ifdef UNIX
    int		shortname = FALSE;	/* use 8.3 file name */
    stat_T	st_old;		/* mch_stat() of existing viminfo file */
#endif
#ifdef MSWIN
    int		hidden = FALSE;
#endif

    if (no_viminfo())
	return;

    fname = viminfo_filename(file);	/* may set to default if NULL */
    if (fname == NULL)
	return;

    fp_in = mch_fopen((char *)fname, READBIN);
    if (fp_in == NULL)
    {
	int fd;

	/* if it does exist, but we can't read it, don't try writing */
	if (mch_stat((char *)fname, &st_new) == 0)
	    goto end;

	/* Create the new .viminfo non-accessible for others, because it may
	 * contain text from non-accessible documents. It is up to the user to
	 * widen access (e.g. to a group). This may also fail if there is a
	 * race condition, then just give up. */
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

	    /* avoid a wait_return for this message, it's annoying */
	    semsg(_("E137: Viminfo file is not writable: %s"), fname);
	    msg_didany = tt;
	    fclose(fp_in);
	    goto end;
	}
#endif
#ifdef MSWIN
	/* Get the file attributes of the existing viminfo file. */
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
	    if (tempname == NULL)		/* out of memory */
		break;

	    /*
	     * Try a series of names.  Change one character, just before
	     * the extension.  This should also work for an 8.3
	     * file name, when after adding the extension it still is
	     * the same file as the original.
	     */
	    wp = tempname + STRLEN(tempname) - 5;
	    if (wp < gettail(tempname))	    /* empty file name? */
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
		    /* Try creating the file exclusively.  This may fail if
		     * another Vim tries to do it at the same time. */
#ifdef VMS
		    /* fdopen() fails for some reason */
		    umask_save = umask(077);
		    fp_out = mch_fopen((char *)tempname, WRITEBIN);
		    (void)umask(umask_save);
#else
		    int	fd;

		    /* Use mch_open() to be able to use O_NOFOLLOW and set file
		     * protection:
		     * Unix: same as original file, but strip s-bit.  Reset
		     * umask to avoid it getting in the way.
		     * Others: r&w for user only. */
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
			/* Avoid trying lots of names while the problem is lack
			 * of permission, only retry if the file already
			 * exists. */
			if (errno != EEXIST)
			    break;
# endif
		    }
		    else
			fp_out = fdopen(fd, WRITEBIN);
#endif /* VMS */
		    if (fp_out != NULL)
			break;
		}

		/* Assume file exists, try again with another name. */
		if (next_char == 'a' - 1)
		{
		    /* They all exist?  Must be something wrong! Don't write
		     * the viminfo file then. */
		    semsg(_("E929: Too many viminfo temp files, like %s!"),
								     tempname);
		    break;
		}
		*wp = next_char;
		--next_char;
	    }

	    if (tempname != NULL)
		break;
	    /* continue if shortname was set */
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
		    /* Changing the owner might fail, in which case the
		     * file will now owned by the current user, oh well. */
		    vim_ignored = fchown(fileno(fp_out), st_old.st_uid, -1);
		if (st_old.st_gid != tmp_st.st_gid
			&& fchown(fileno(fp_out), -1, st_old.st_gid) == -1)
		    /* can't set the group to what it should be, remove
		     * group permissions */
		    (void)mch_setperm(tempname, 0600);
	    }
	    else
		/* can't stat the file, set conservative permissions */
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

	/* In case of an error keep the original viminfo file.  Otherwise
	 * rename the newly written file.  Give an error if that fails. */
	if (viminfo_errcnt == 0)
	{
	    if (vim_rename(tempname, fname) == -1)
	    {
		++viminfo_errcnt;
		semsg(_("E886: Can't rename viminfo file to %s!"), fname);
	    }
# ifdef MSWIN
	    /* If the viminfo file was hidden then also hide the new file. */
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
	    /* Use $VIM only if $HOME is the default "C:/". */
	    if (STRCMP(vim_getenv((char_u *)"HOME", NULL), "C:/") == 0
		    && mch_getenv((char_u *)"HOME") == NULL)
#  else
	    if (mch_getenv((char_u *)"HOME") == NULL)
#  endif
# endif
	    {
		/* don't use $VIM when not available. */
		expand_env((char_u *)"$VIM", NameBuff, MAXPATHL);
		if (STRCMP("$VIM", NameBuff) != 0)  /* $VIM was expanded */
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
		/* Registers and marks are read and kept separate from what
		 * this Vim is using.  They are merged when writing. */
		prepare_viminfo_registers();
		prepare_viminfo_marks();
	    }

	    eof = read_viminfo_up_to_marks(&vir,
					 flags & VIF_FORCEIT, fp_out != NULL);
	    merge = TRUE;
	}
	else if (flags != 0)
	    /* Skip info, find start of marks */
	    while (!(eof = viminfo_readline(&vir))
		    && vir.vir_line[0] != '>')
		;

	do_copy_marks = (flags &
			   (VIF_WANT_MARKS | VIF_GET_OLDFILES | VIF_FORCEIT));
    }

    if (fp_out != NULL)
    {
	/* Write the info: */
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
		/* Characters reserved for future expansion, ignored now */
	    case '+': /* "+40 /path/dir file", for running vim without args */
	    case '^': /* to be defined */
	    case '<': /* long line - ignored */
		/* A comment or empty line. */
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
	    case '*': /* "*encoding=value" */
		got_encoding = TRUE;
		eof = viminfo_encoding(virp);
		break;
	    case '!': /* global variable */
#ifdef FEAT_EVAL
		eof = read_viminfo_varlist(virp, writing);
#else
		eof = viminfo_readline(virp);
#endif
		break;
	    case '%': /* entry for buffer list */
		eof = read_viminfo_bufferlist(virp, writing);
		break;
	    case '"':
		/* When registers are in bar lines skip the old style register
		 * lines. */
		if (virp->vir_version < VIMINFO_VERSION_WITH_REGISTERS)
		    eof = read_viminfo_register(virp, forceit);
		else
		    do {
			eof = viminfo_readline(virp);
		    } while (!eof && (virp->vir_line[0] == TAB
						|| virp->vir_line[0] == '<'));
		break;
	    case '/':	    /* Search string */
	    case '&':	    /* Substitute search string */
	    case '~':	    /* Last search string, followed by '/' or '&' */
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
		/* When history is in bar lines skip the old style history
		 * lines. */
		if (virp->vir_version < VIMINFO_VERSION_WITH_HISTORY)
		    eof = read_viminfo_history(virp, writing);
		else
#endif
		    eof = viminfo_readline(virp);
		break;
	    case '-':
	    case '\'':
		/* When file marks are in bar lines skip the old style lines. */
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
    /* Finish reading history items. */
    if (!writing)
	finish_viminfo_history(virp);
#endif

    /* Change file names to buffer numbers for fmarks. */
    FOR_ALL_BUFFERS(buf)
	fmarks_check_names(buf);

    return eof;
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
	    /* remove trailing newline */
	    ++p;
	    for (i = 0; vim_isprintc(p[i]); ++i)
		;
	    p[i] = NUL;

	    convert_setup(&virp->vir_conv, p, p_enc);
	}
    }
    return viminfo_readline(virp);
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
    int		off,		    /* offset for virp->vir_line */
    int		convert UNUSED)	    /* convert the string */
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
	    /* Line too long?  File messed up?  Skip next line. */
	    (void)vim_fgets(virp->vir_line, 10, virp->vir_fd);
	    return NULL;
	}
	(void)vim_fgets(retval, (int)len, virp->vir_fd);
	s = retval + 1;	    /* Skip the leading '<' */
    }
    else
    {
	retval = vim_strsave(virp->vir_line + off);
	if (retval == NULL)
	    return NULL;
	s = retval;
    }

    /* Change CTRL-V CTRL-V to CTRL-V and CTRL-V n to \n in-place. */
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

    /* If the string will be too long, write its length and put it in the next
     * line.  Take into account that some room is needed for what comes before
     * the string (e.g., variable name).  Add something to the length for the
     * '<', NL and trailing NUL. */
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

    /* Count the number of characters produced, including quotes. */
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
	    /* Leave enough space for another continuation. */
	    remaining = LSIZE - 20;
	}
    }
    putc('"', fd);
    return remaining - 2;
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
	    /* Need to read a continuation line.  Put strings in allocated
	     * memory, because virp->vir_line is overwritten. */
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

		/* String value was split into lines that are each shorter
		 * than LSIZE:
		 *     |{bartype},>{length of "{text}{text2}"}
		 *     |<"{text1}
		 *     |<{text2}",{value}
		 * Length includes the quotes.
		 */
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
			/* File was truncated or garbled. Read another line if
			 * this one starts with '|'. */
			vim_free(buf);
			return eof || virp->vir_line[0] == '|';
		    }
		    /* Get length of text, excluding |< and NL chars. */
		    n = STRLEN(virp->vir_line);
		    while (n > 0 && (virp->vir_line[n - 1] == NL
					     || virp->vir_line[n - 1] == CAR))
			--n;
		    n -= 2;
		    if (n > todo)
		    {
			/* more values follow after the string */
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
		/* Line ending in ">" continues in the next line:
		 *     |{bartype},{lots of values},>
		 *     |<{value},{value}
		 */
		eof = viminfo_readline(virp);
		if (eof || virp->vir_line[0] != '|'
					      || virp->vir_line[1] != '<')
		    /* File was truncated or garbled. Read another line if
		     * this one starts with '|'. */
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

	    /* Unescape special characters in-place. */
	    ++p;
	    while (*p != '"')
	    {
		if (*p == NL || *p == NUL)
		    return TRUE;  /* syntax error, drop the value */
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

	    /* Need to copy in allocated memory if the string wasn't allocated
	     * above and we did allocate before, thus vir_line may change. */
	    if (s != buf && allocated)
		s = vim_strsave(s);
	    value->bv_string = s;
	    value->bv_type = BVAL_STRING;
	    value->bv_len = len;
	    value->bv_allocated = allocated || converted;
	    ++values->ga_len;
	    if (nextp != NULL)
	    {
		/* values following a long string */
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

    /* The format is: |{bartype},{value},...
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
	/* Continuation line of an unrecognized item. */
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
		/* Only use the version when it comes before the encoding.
		 * If it comes later it was copied by a Vim version that
		 * doesn't understand the version. */
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
		/* copy unrecognized line (for future use) */
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

	/* Skip over continuation lines until seeing a useful line. */
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
#endif /* FEAT_VIMINFO */

/*
 * Return the current time in seconds.  Calls time(), unless test_settime()
 * was used.
 */
    time_T
vim_time(void)
{
# ifdef FEAT_EVAL
    return time_for_testing == 0 ? time(NULL) : time_for_testing;
# else
    return time(NULL);
# endif
}

/*
 * Implementation of ":fixdel", also used by get_stty().
 *  <BS>    resulting <Del>
 *   ^?		^H
 * not ^?	^?
 */
    void
do_fixdel(exarg_T *eap UNUSED)
{
    char_u  *p;

    p = find_termcode((char_u *)"kb");
    add_termcode((char_u *)"kD", p != NULL
	    && *p == DEL ? (char_u *)CTRL_H_STR : DEL_STR, FALSE);
}

    void
print_line_no_prefix(
    linenr_T	lnum,
    int		use_number,
    int		list)
{
    char	numbuf[30];

    if (curwin->w_p_nu || use_number)
    {
	vim_snprintf(numbuf, sizeof(numbuf),
				   "%*ld ", number_width(curwin), (long)lnum);
	msg_puts_attr(numbuf, HL_ATTR(HLF_N));	/* Highlight line nrs */
    }
    msg_prt_line(ml_get(lnum), list);
}

/*
 * Print a text line.  Also in silent mode ("ex -s").
 */
    void
print_line(linenr_T lnum, int use_number, int list)
{
    int		save_silent = silent_mode;

    /* apply :filter /pat/ */
    if (message_filtered(ml_get(lnum)))
	return;

    msg_start();
    silent_mode = FALSE;
    info_message = TRUE;	/* use mch_msg(), not mch_errmsg() */
    print_line_no_prefix(lnum, use_number, list);
    if (save_silent)
    {
	msg_putchar('\n');
	cursor_on();		/* msg_start() switches it off */
	out_flush();
	silent_mode = save_silent;
    }
    info_message = FALSE;
}

    int
rename_buffer(char_u *new_fname)
{
    char_u	*fname, *sfname, *xfname;
    buf_T	*buf;

    buf = curbuf;
    apply_autocmds(EVENT_BUFFILEPRE, NULL, NULL, FALSE, curbuf);
    /* buffer changed, don't change name now */
    if (buf != curbuf)
	return FAIL;
#ifdef FEAT_EVAL
    if (aborting())	    /* autocmds may abort script processing */
	return FAIL;
#endif
    /*
     * The name of the current buffer will be changed.
     * A new (unlisted) buffer entry needs to be made to hold the old file
     * name, which will become the alternate file name.
     * But don't set the alternate file name if the buffer didn't have a
     * name.
     */
    fname = curbuf->b_ffname;
    sfname = curbuf->b_sfname;
    xfname = curbuf->b_fname;
    curbuf->b_ffname = NULL;
    curbuf->b_sfname = NULL;
    if (setfname(curbuf, new_fname, NULL, TRUE) == FAIL)
    {
	curbuf->b_ffname = fname;
	curbuf->b_sfname = sfname;
	return FAIL;
    }
    curbuf->b_flags |= BF_NOTEDITED;
    if (xfname != NULL && *xfname != NUL)
    {
	buf = buflist_new(fname, xfname, curwin->w_cursor.lnum, 0);
	if (buf != NULL && !cmdmod.keepalt)
	    curwin->w_alt_fnum = buf->b_fnum;
    }
    vim_free(fname);
    vim_free(sfname);
    apply_autocmds(EVENT_BUFFILEPOST, NULL, NULL, FALSE, curbuf);

    /* Change directories when the 'acd' option is set. */
    DO_AUTOCHDIR;
    return OK;
}

/*
 * ":file[!] [fname]".
 */
    void
ex_file(exarg_T *eap)
{
    /* ":0file" removes the file name.  Check for illegal uses ":3file",
     * "0file name", etc. */
    if (eap->addr_count > 0
	    && (*eap->arg != NUL
		|| eap->line2 > 0
		|| eap->addr_count > 1))
    {
	emsg(_(e_invarg));
	return;
    }

    if (*eap->arg != NUL || eap->addr_count == 1)
    {
	if (rename_buffer(eap->arg) == FAIL)
	    return;
	redraw_tabline = TRUE;
    }

    // print file name if no argument or 'F' is not in 'shortmess'
    if (*eap->arg == NUL || !shortmess(SHM_FILEINFO))
	fileinfo(FALSE, FALSE, eap->forceit);
}

/*
 * ":update".
 */
    void
ex_update(exarg_T *eap)
{
    if (curbufIsChanged())
	(void)do_write(eap);
}

/*
 * ":write" and ":saveas".
 */
    void
ex_write(exarg_T *eap)
{
    if (eap->usefilter)		/* input lines to shell command */
	do_bang(1, eap, FALSE, TRUE, FALSE);
    else
	(void)do_write(eap);
}

/*
 * write current buffer to file 'eap->arg'
 * if 'eap->append' is TRUE, append to the file
 *
 * if *eap->arg == NUL write to current file
 *
 * return FAIL for failure, OK otherwise
 */
    int
do_write(exarg_T *eap)
{
    int		other;
    char_u	*fname = NULL;		/* init to shut up gcc */
    char_u	*ffname;
    int		retval = FAIL;
    char_u	*free_fname = NULL;
#ifdef FEAT_BROWSE
    char_u	*browse_file = NULL;
#endif
    buf_T	*alt_buf = NULL;
    int		name_was_missing;

    if (not_writing())		/* check 'write' option */
	return FAIL;

    ffname = eap->arg;
#ifdef FEAT_BROWSE
    if (cmdmod.browse)
    {
	browse_file = do_browse(BROWSE_SAVE, (char_u *)_("Save As"), ffname,
						    NULL, NULL, NULL, curbuf);
	if (browse_file == NULL)
	    goto theend;
	ffname = browse_file;
    }
#endif
    if (*ffname == NUL)
    {
	if (eap->cmdidx == CMD_saveas)
	{
	    emsg(_(e_argreq));
	    goto theend;
	}
	other = FALSE;
    }
    else
    {
	fname = ffname;
	free_fname = fix_fname(ffname);
	/*
	 * When out-of-memory, keep unexpanded file name, because we MUST be
	 * able to write the file in this situation.
	 */
	if (free_fname != NULL)
	    ffname = free_fname;
	other = otherfile(ffname);
    }

    /*
     * If we have a new file, put its name in the list of alternate file names.
     */
    if (other)
    {
	if (vim_strchr(p_cpo, CPO_ALTWRITE) != NULL
						 || eap->cmdidx == CMD_saveas)
	    alt_buf = setaltfname(ffname, fname, (linenr_T)1);
	else
	    alt_buf = buflist_findname(ffname);
	if (alt_buf != NULL && alt_buf->b_ml.ml_mfp != NULL)
	{
	    /* Overwriting a file that is loaded in another buffer is not a
	     * good idea. */
	    emsg(_(e_bufloaded));
	    goto theend;
	}
    }

    /*
     * Writing to the current file is not allowed in readonly mode
     * and a file name is required.
     * "nofile" and "nowrite" buffers cannot be written implicitly either.
     */
    if (!other && (
#ifdef FEAT_QUICKFIX
		bt_dontwrite_msg(curbuf) ||
#endif
		check_fname() == FAIL || check_readonly(&eap->forceit, curbuf)))
	goto theend;

    if (!other)
    {
	ffname = curbuf->b_ffname;
	fname = curbuf->b_fname;
	/*
	 * Not writing the whole file is only allowed with '!'.
	 */
	if (	   (eap->line1 != 1
		    || eap->line2 != curbuf->b_ml.ml_line_count)
		&& !eap->forceit
		&& !eap->append
		&& !p_wa)
	{
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	    if (p_confirm || cmdmod.confirm)
	    {
		if (vim_dialog_yesno(VIM_QUESTION, NULL,
			       (char_u *)_("Write partial file?"), 2) != VIM_YES)
		    goto theend;
		eap->forceit = TRUE;
	    }
	    else
#endif
	    {
		emsg(_("E140: Use ! to write partial buffer"));
		goto theend;
	    }
	}
    }

    if (check_overwrite(eap, curbuf, fname, ffname, other) == OK)
    {
	if (eap->cmdidx == CMD_saveas && alt_buf != NULL)
	{
	    buf_T	*was_curbuf = curbuf;

	    apply_autocmds(EVENT_BUFFILEPRE, NULL, NULL, FALSE, curbuf);
	    apply_autocmds(EVENT_BUFFILEPRE, NULL, NULL, FALSE, alt_buf);
#ifdef FEAT_EVAL
	    if (curbuf != was_curbuf || aborting())
#else
	    if (curbuf != was_curbuf)
#endif
	    {
		/* buffer changed, don't change name now */
		retval = FAIL;
		goto theend;
	    }
	    /* Exchange the file names for the current and the alternate
	     * buffer.  This makes it look like we are now editing the buffer
	     * under the new name.  Must be done before buf_write(), because
	     * if there is no file name and 'cpo' contains 'F', it will set
	     * the file name. */
	    fname = alt_buf->b_fname;
	    alt_buf->b_fname = curbuf->b_fname;
	    curbuf->b_fname = fname;
	    fname = alt_buf->b_ffname;
	    alt_buf->b_ffname = curbuf->b_ffname;
	    curbuf->b_ffname = fname;
	    fname = alt_buf->b_sfname;
	    alt_buf->b_sfname = curbuf->b_sfname;
	    curbuf->b_sfname = fname;
	    buf_name_changed(curbuf);

	    apply_autocmds(EVENT_BUFFILEPOST, NULL, NULL, FALSE, curbuf);
	    apply_autocmds(EVENT_BUFFILEPOST, NULL, NULL, FALSE, alt_buf);
	    if (!alt_buf->b_p_bl)
	    {
		alt_buf->b_p_bl = TRUE;
		apply_autocmds(EVENT_BUFADD, NULL, NULL, FALSE, alt_buf);
	    }
#ifdef FEAT_EVAL
	    if (curbuf != was_curbuf || aborting())
#else
	    if (curbuf != was_curbuf)
#endif
	    {
		/* buffer changed, don't write the file */
		retval = FAIL;
		goto theend;
	    }

	    /* If 'filetype' was empty try detecting it now. */
	    if (*curbuf->b_p_ft == NUL)
	    {
		if (au_has_group((char_u *)"filetypedetect"))
		    (void)do_doautocmd((char_u *)"filetypedetect BufRead",
								  TRUE, NULL);
		do_modelines(0);
	    }

	    /* Autocommands may have changed buffer names, esp. when
	     * 'autochdir' is set. */
	    fname = curbuf->b_sfname;
	}

	name_was_missing = curbuf->b_ffname == NULL;

	retval = buf_write(curbuf, ffname, fname, eap->line1, eap->line2,
				 eap, eap->append, eap->forceit, TRUE, FALSE);

	/* After ":saveas fname" reset 'readonly'. */
	if (eap->cmdidx == CMD_saveas)
	{
	    if (retval == OK)
	    {
		curbuf->b_p_ro = FALSE;
		redraw_tabline = TRUE;
	    }
	}

	/* Change directories when the 'acd' option is set and the file name
	 * got changed or set. */
	if (eap->cmdidx == CMD_saveas || name_was_missing)
	{
	    DO_AUTOCHDIR;
	}
    }

theend:
#ifdef FEAT_BROWSE
    vim_free(browse_file);
#endif
    vim_free(free_fname);
    return retval;
}

/*
 * Check if it is allowed to overwrite a file.  If b_flags has BF_NOTEDITED,
 * BF_NEW or BF_READERR, check for overwriting current file.
 * May set eap->forceit if a dialog says it's OK to overwrite.
 * Return OK if it's OK, FAIL if it is not.
 */
    int
check_overwrite(
    exarg_T	*eap,
    buf_T	*buf,
    char_u	*fname,	    /* file name to be used (can differ from
			       buf->ffname) */
    char_u	*ffname,    /* full path version of fname */
    int		other)	    /* writing under other name */
{
    /*
     * write to other file or b_flags set or not writing the whole file:
     * overwriting only allowed with '!'
     */
    if (       (other
		|| (buf->b_flags & BF_NOTEDITED)
		|| ((buf->b_flags & BF_NEW)
		    && vim_strchr(p_cpo, CPO_OVERNEW) == NULL)
		|| (buf->b_flags & BF_READERR))
	    && !p_wa
#ifdef FEAT_QUICKFIX
	    && !bt_nofile(buf)
#endif
	    && vim_fexists(ffname))
    {
	if (!eap->forceit && !eap->append)
	{
#ifdef UNIX
	    /* with UNIX it is possible to open a directory */
	    if (mch_isdir(ffname))
	    {
		semsg(_(e_isadir2), ffname);
		return FAIL;
	    }
#endif
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	    if (p_confirm || cmdmod.confirm)
	    {
		char_u	buff[DIALOG_MSG_SIZE];

		dialog_msg(buff, _("Overwrite existing file \"%s\"?"), fname);
		if (vim_dialog_yesno(VIM_QUESTION, NULL, buff, 2) != VIM_YES)
		    return FAIL;
		eap->forceit = TRUE;
	    }
	    else
#endif
	    {
		emsg(_(e_exists));
		return FAIL;
	    }
	}

	/* For ":w! filename" check that no swap file exists for "filename". */
	if (other && !emsg_silent)
	{
	    char_u	*dir;
	    char_u	*p;
	    int		r;
	    char_u	*swapname;

	    /* We only try the first entry in 'directory', without checking if
	     * it's writable.  If the "." directory is not writable the write
	     * will probably fail anyway.
	     * Use 'shortname' of the current buffer, since there is no buffer
	     * for the written file. */
	    if (*p_dir == NUL)
	    {
		dir = alloc(5);
		if (dir == NULL)
		    return FAIL;
		STRCPY(dir, ".");
	    }
	    else
	    {
		dir = alloc(MAXPATHL);
		if (dir == NULL)
		    return FAIL;
		p = p_dir;
		copy_option_part(&p, dir, MAXPATHL, ",");
	    }
	    swapname = makeswapname(fname, ffname, curbuf, dir);
	    vim_free(dir);
	    r = vim_fexists(swapname);
	    if (r)
	    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
		if (p_confirm || cmdmod.confirm)
		{
		    char_u	buff[DIALOG_MSG_SIZE];

		    dialog_msg(buff,
			    _("Swap file \"%s\" exists, overwrite anyway?"),
								    swapname);
		    if (vim_dialog_yesno(VIM_QUESTION, NULL, buff, 2)
								   != VIM_YES)
		    {
			vim_free(swapname);
			return FAIL;
		    }
		    eap->forceit = TRUE;
		}
		else
#endif
		{
		    semsg(_("E768: Swap file exists: %s (:silent! overrides)"),
								    swapname);
		    vim_free(swapname);
		    return FAIL;
		}
	    }
	    vim_free(swapname);
	}
    }
    return OK;
}

/*
 * Handle ":wnext", ":wNext" and ":wprevious" commands.
 */
    void
ex_wnext(exarg_T *eap)
{
    int		i;

    if (eap->cmd[1] == 'n')
	i = curwin->w_arg_idx + (int)eap->line2;
    else
	i = curwin->w_arg_idx - (int)eap->line2;
    eap->line1 = 1;
    eap->line2 = curbuf->b_ml.ml_line_count;
    if (do_write(eap) != FAIL)
	do_argfile(eap, i);
}

/*
 * ":wall", ":wqall" and ":xall": Write all changed files (and exit).
 */
    void
do_wqall(exarg_T *eap)
{
    buf_T	*buf;
    int		error = 0;
    int		save_forceit = eap->forceit;

    if (eap->cmdidx == CMD_xall || eap->cmdidx == CMD_wqall)
	exiting = TRUE;

    FOR_ALL_BUFFERS(buf)
    {
#ifdef FEAT_TERMINAL
	if (exiting && term_job_running(buf->b_term))
	{
	    no_write_message_nobang(buf);
	    ++error;
	}
	else
#endif
	if (bufIsChanged(buf) && !bt_dontwrite(buf))
	{
	    /*
	     * Check if there is a reason the buffer cannot be written:
	     * 1. if the 'write' option is set
	     * 2. if there is no file name (even after browsing)
	     * 3. if the 'readonly' is set (even after a dialog)
	     * 4. if overwriting is allowed (even after a dialog)
	     */
	    if (not_writing())
	    {
		++error;
		break;
	    }
#ifdef FEAT_BROWSE
	    /* ":browse wall": ask for file name if there isn't one */
	    if (buf->b_ffname == NULL && cmdmod.browse)
		browse_save_fname(buf);
#endif
	    if (buf->b_ffname == NULL)
	    {
		semsg(_("E141: No file name for buffer %ld"), (long)buf->b_fnum);
		++error;
	    }
	    else if (check_readonly(&eap->forceit, buf)
		    || check_overwrite(eap, buf, buf->b_fname, buf->b_ffname,
							       FALSE) == FAIL)
	    {
		++error;
	    }
	    else
	    {
		bufref_T bufref;

		set_bufref(&bufref, buf);
		if (buf_write_all(buf, eap->forceit) == FAIL)
		    ++error;
		/* an autocommand may have deleted the buffer */
		if (!bufref_valid(&bufref))
		    buf = firstbuf;
	    }
	    eap->forceit = save_forceit;    /* check_overwrite() may set it */
	}
    }
    if (exiting)
    {
	if (!error)
	    getout(0);		/* exit Vim */
	not_exiting();
    }
}

/*
 * Check the 'write' option.
 * Return TRUE and give a message when it's not st.
 */
    int
not_writing(void)
{
    if (p_write)
	return FALSE;
    emsg(_("E142: File not written: Writing is disabled by 'write' option"));
    return TRUE;
}

/*
 * Check if a buffer is read-only (either 'readonly' option is set or file is
 * read-only). Ask for overruling in a dialog. Return TRUE and give an error
 * message when the buffer is readonly.
 */
    static int
check_readonly(int *forceit, buf_T *buf)
{
    stat_T	st;

    /* Handle a file being readonly when the 'readonly' option is set or when
     * the file exists and permissions are read-only.
     * We will send 0777 to check_file_readonly(), as the "perm" variable is
     * important for device checks but not here. */
    if (!*forceit && (buf->b_p_ro
		|| (mch_stat((char *)buf->b_ffname, &st) >= 0
		    && check_file_readonly(buf->b_ffname, 0777))))
    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	if ((p_confirm || cmdmod.confirm) && buf->b_fname != NULL)
	{
	    char_u	buff[DIALOG_MSG_SIZE];

	    if (buf->b_p_ro)
		dialog_msg(buff, _("'readonly' option is set for \"%s\".\nDo you wish to write anyway?"),
		    buf->b_fname);
	    else
		dialog_msg(buff, _("File permissions of \"%s\" are read-only.\nIt may still be possible to write it.\nDo you wish to try?"),
		    buf->b_fname);

	    if (vim_dialog_yesno(VIM_QUESTION, NULL, buff, 2) == VIM_YES)
	    {
		/* Set forceit, to force the writing of a readonly file */
		*forceit = TRUE;
		return FALSE;
	    }
	    else
		return TRUE;
	}
	else
#endif
	if (buf->b_p_ro)
	    emsg(_(e_readonly));
	else
	    semsg(_("E505: \"%s\" is read-only (add ! to override)"),
		    buf->b_fname);
	return TRUE;
    }

    return FALSE;
}

/*
 * Try to abandon the current file and edit a new or existing file.
 * "fnum" is the number of the file, if zero use "ffname_arg"/"sfname_arg".
 * "lnum" is the line number for the cursor in the new file (if non-zero).
 *
 * Return:
 * GETFILE_ERROR for "normal" error,
 * GETFILE_NOT_WRITTEN for "not written" error,
 * GETFILE_SAME_FILE for success
 * GETFILE_OPEN_OTHER for successfully opening another file.
 */
    int
getfile(
    int		fnum,
    char_u	*ffname_arg,
    char_u	*sfname_arg,
    int		setpm,
    linenr_T	lnum,
    int		forceit)
{
    char_u	*ffname = ffname_arg;
    char_u	*sfname = sfname_arg;
    int		other;
    int		retval;
    char_u	*free_me = NULL;

    if (text_locked())
	return GETFILE_ERROR;
    if (curbuf_locked())
	return GETFILE_ERROR;

    if (fnum == 0)
    {
					/* make ffname full path, set sfname */
	fname_expand(curbuf, &ffname, &sfname);
	other = otherfile(ffname);
	free_me = ffname;		/* has been allocated, free() later */
    }
    else
	other = (fnum != curbuf->b_fnum);

    if (other)
	++no_wait_return;	    /* don't wait for autowrite message */
    if (other && !forceit && curbuf->b_nwindows == 1 && !buf_hide(curbuf)
		   && curbufIsChanged() && autowrite(curbuf, forceit) == FAIL)
    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	if (p_confirm && p_write)
	    dialog_changed(curbuf, FALSE);
	if (curbufIsChanged())
#endif
	{
	    if (other)
		--no_wait_return;
	    no_write_message();
	    retval = GETFILE_NOT_WRITTEN;	/* file has been changed */
	    goto theend;
	}
    }
    if (other)
	--no_wait_return;
    if (setpm)
	setpcmark();
    if (!other)
    {
	if (lnum != 0)
	    curwin->w_cursor.lnum = lnum;
	check_cursor_lnum();
	beginline(BL_SOL | BL_FIX);
	retval = GETFILE_SAME_FILE;	/* it's in the same file */
    }
    else if (do_ecmd(fnum, ffname, sfname, NULL, lnum,
	     (buf_hide(curbuf) ? ECMD_HIDE : 0) + (forceit ? ECMD_FORCEIT : 0),
		curwin) == OK)
	retval = GETFILE_OPEN_OTHER;	/* opened another file */
    else
	retval = GETFILE_ERROR;		/* error encountered */

theend:
    vim_free(free_me);
    return retval;
}

/*
 * start editing a new file
 *
 *     fnum: file number; if zero use ffname/sfname
 *   ffname: the file name
 *		- full path if sfname used,
 *		- any file name if sfname is NULL
 *		- empty string to re-edit with the same file name (but may be
 *		    in a different directory)
 *		- NULL to start an empty buffer
 *   sfname: the short file name (or NULL)
 *	eap: contains the command to be executed after loading the file and
 *	     forced 'ff' and 'fenc'
 *  newlnum: if > 0: put cursor on this line number (if possible)
 *	     if ECMD_LASTL: use last position in loaded file
 *	     if ECMD_LAST: use last position in all files
 *	     if ECMD_ONE: use first line
 *    flags:
 *	   ECMD_HIDE: if TRUE don't free the current buffer
 *     ECMD_SET_HELP: set b_help flag of (new) buffer before opening file
 *	 ECMD_OLDBUF: use existing buffer if it exists
 *	ECMD_FORCEIT: ! used for Ex command
 *	 ECMD_ADDBUF: don't edit, just add to buffer list
 *   oldwin: Should be "curwin" when editing a new buffer in the current
 *	     window, NULL when splitting the window first.  When not NULL info
 *	     of the previous buffer for "oldwin" is stored.
 *
 * return FAIL for failure, OK otherwise
 */
    int
do_ecmd(
    int		fnum,
    char_u	*ffname,
    char_u	*sfname,
    exarg_T	*eap,			/* can be NULL! */
    linenr_T	newlnum,
    int		flags,
    win_T	*oldwin)
{
    int		other_file;		/* TRUE if editing another file */
    int		oldbuf;			/* TRUE if using existing buffer */
    int		auto_buf = FALSE;	/* TRUE if autocommands brought us
					   into the buffer unexpectedly */
    char_u	*new_name = NULL;
#if defined(FEAT_EVAL)
    int		did_set_swapcommand = FALSE;
#endif
    buf_T	*buf;
    bufref_T	bufref;
    bufref_T	old_curbuf;
    char_u	*free_fname = NULL;
#ifdef FEAT_BROWSE
    char_u	*browse_file = NULL;
#endif
    int		retval = FAIL;
    long	n;
    pos_T	orig_pos;
    linenr_T	topline = 0;
    int		newcol = -1;
    int		solcol = -1;
    pos_T	*pos;
    char_u	*command = NULL;
#ifdef FEAT_SPELL
    int		did_get_winopts = FALSE;
#endif
    int		readfile_flags = 0;
    int		did_inc_redrawing_disabled = FALSE;
    long        *so_ptr = curwin->w_p_so >= 0 ? &curwin->w_p_so : &p_so;

    if (eap != NULL)
	command = eap->do_ecmd_cmd;
    set_bufref(&old_curbuf, curbuf);

    if (fnum != 0)
    {
	if (fnum == curbuf->b_fnum)	/* file is already being edited */
	    return OK;			/* nothing to do */
	other_file = TRUE;
    }
    else
    {
#ifdef FEAT_BROWSE
	if (cmdmod.browse)
	{
	    if (
# ifdef FEAT_GUI
		!gui.in_use &&
# endif
		    au_has_group((char_u *)"FileExplorer"))
	    {
		/* No browsing supported but we do have the file explorer:
		 * Edit the directory. */
		if (ffname == NULL || !mch_isdir(ffname))
		    ffname = (char_u *)".";
	    }
	    else
	    {
		browse_file = do_browse(0, (char_u *)_("Edit File"), ffname,
						    NULL, NULL, NULL, curbuf);
		if (browse_file == NULL)
		    goto theend;
		ffname = browse_file;
	    }
	}
#endif
	/* if no short name given, use ffname for short name */
	if (sfname == NULL)
	    sfname = ffname;
#ifdef USE_FNAME_CASE
	if (sfname != NULL)
	    fname_case(sfname, 0);   /* set correct case for sfname */
#endif

	if ((flags & ECMD_ADDBUF) && (ffname == NULL || *ffname == NUL))
	    goto theend;

	if (ffname == NULL)
	    other_file = TRUE;
					    /* there is no file name */
	else if (*ffname == NUL && curbuf->b_ffname == NULL)
	    other_file = FALSE;
	else
	{
	    if (*ffname == NUL)		    /* re-edit with same file name */
	    {
		ffname = curbuf->b_ffname;
		sfname = curbuf->b_fname;
	    }
	    free_fname = fix_fname(ffname); /* may expand to full path name */
	    if (free_fname != NULL)
		ffname = free_fname;
	    other_file = otherfile(ffname);
	}
    }

    /*
     * if the file was changed we may not be allowed to abandon it
     * - if we are going to re-edit the same file
     * - or if we are the only window on this file and if ECMD_HIDE is FALSE
     */
    if (  ((!other_file && !(flags & ECMD_OLDBUF))
	    || (curbuf->b_nwindows == 1
		&& !(flags & (ECMD_HIDE | ECMD_ADDBUF))))
	&& check_changed(curbuf, (p_awa ? CCGD_AW : 0)
			       | (other_file ? 0 : CCGD_MULTWIN)
			       | ((flags & ECMD_FORCEIT) ? CCGD_FORCEIT : 0)
			       | (eap == NULL ? 0 : CCGD_EXCMD)))
    {
	if (fnum == 0 && other_file && ffname != NULL)
	    (void)setaltfname(ffname, sfname, newlnum < 0 ? 0 : newlnum);
	goto theend;
    }

    /*
     * End Visual mode before switching to another buffer, so the text can be
     * copied into the GUI selection buffer.
     */
    reset_VIsual();

#if defined(FEAT_EVAL)
    if ((command != NULL || newlnum > (linenr_T)0)
	    && *get_vim_var_str(VV_SWAPCOMMAND) == NUL)
    {
	int	len;
	char_u	*p;

	/* Set v:swapcommand for the SwapExists autocommands. */
	if (command != NULL)
	    len = (int)STRLEN(command) + 3;
	else
	    len = 30;
	p = alloc((unsigned)len);
	if (p != NULL)
	{
	    if (command != NULL)
		vim_snprintf((char *)p, len, ":%s\r", command);
	    else
		vim_snprintf((char *)p, len, "%ldG", (long)newlnum);
	    set_vim_var_string(VV_SWAPCOMMAND, p, -1);
	    did_set_swapcommand = TRUE;
	    vim_free(p);
	}
    }
#endif

    /*
     * If we are starting to edit another file, open a (new) buffer.
     * Otherwise we re-use the current buffer.
     */
    if (other_file)
    {
	if (!(flags & ECMD_ADDBUF))
	{
	    if (!cmdmod.keepalt)
		curwin->w_alt_fnum = curbuf->b_fnum;
	    if (oldwin != NULL)
		buflist_altfpos(oldwin);
	}

	if (fnum)
	    buf = buflist_findnr(fnum);
	else
	{
	    if (flags & ECMD_ADDBUF)
	    {
		linenr_T	tlnum = 1L;

		if (command != NULL)
		{
		    tlnum = atol((char *)command);
		    if (tlnum <= 0)
			tlnum = 1L;
		}
		(void)buflist_new(ffname, sfname, tlnum, BLN_LISTED);
		goto theend;
	    }
	    buf = buflist_new(ffname, sfname, 0L,
		    BLN_CURBUF | ((flags & ECMD_SET_HELP) ? 0 : BLN_LISTED));

	    /* autocommands may change curwin and curbuf */
	    if (oldwin != NULL)
		oldwin = curwin;
	    set_bufref(&old_curbuf, curbuf);
	}
	if (buf == NULL)
	    goto theend;
	if (buf->b_ml.ml_mfp == NULL)		/* no memfile yet */
	{
	    oldbuf = FALSE;
	}
	else					/* existing memfile */
	{
	    oldbuf = TRUE;
	    set_bufref(&bufref, buf);
	    (void)buf_check_timestamp(buf, FALSE);
	    /* Check if autocommands made the buffer invalid or changed the
	     * current buffer. */
	    if (!bufref_valid(&bufref) || curbuf != old_curbuf.br_buf)
		goto theend;
#ifdef FEAT_EVAL
	    if (aborting())	    /* autocmds may abort script processing */
		goto theend;
#endif
	}

	/* May jump to last used line number for a loaded buffer or when asked
	 * for explicitly */
	if ((oldbuf && newlnum == ECMD_LASTL) || newlnum == ECMD_LAST)
	{
	    pos = buflist_findfpos(buf);
	    newlnum = pos->lnum;
	    solcol = pos->col;
	}

	/*
	 * Make the (new) buffer the one used by the current window.
	 * If the old buffer becomes unused, free it if ECMD_HIDE is FALSE.
	 * If the current buffer was empty and has no file name, curbuf
	 * is returned by buflist_new(), nothing to do here.
	 */
	if (buf != curbuf)
	{
	    /*
	     * Be careful: The autocommands may delete any buffer and change
	     * the current buffer.
	     * - If the buffer we are going to edit is deleted, give up.
	     * - If the current buffer is deleted, prefer to load the new
	     *   buffer when loading a buffer is required.  This avoids
	     *   loading another buffer which then must be closed again.
	     * - If we ended up in the new buffer already, need to skip a few
	     *	 things, set auto_buf.
	     */
	    if (buf->b_fname != NULL)
		new_name = vim_strsave(buf->b_fname);
	    set_bufref(&au_new_curbuf, buf);
	    apply_autocmds(EVENT_BUFLEAVE, NULL, NULL, FALSE, curbuf);
	    if (!bufref_valid(&au_new_curbuf))
	    {
		/* new buffer has been deleted */
		delbuf_msg(new_name);	/* frees new_name */
		goto theend;
	    }
#ifdef FEAT_EVAL
	    if (aborting())	    /* autocmds may abort script processing */
	    {
		vim_free(new_name);
		goto theend;
	    }
#endif
	    if (buf == curbuf)		/* already in new buffer */
		auto_buf = TRUE;
	    else
	    {
		win_T	    *the_curwin = curwin;

		/* Set the w_closing flag to avoid that autocommands close the
		 * window.  And set b_locked for the same reason. */
		the_curwin->w_closing = TRUE;
		++buf->b_locked;

		if (curbuf == old_curbuf.br_buf)
		    buf_copy_options(buf, BCO_ENTER);

		/* Close the link to the current buffer. This will set
		 * oldwin->w_buffer to NULL. */
		u_sync(FALSE);
		close_buffer(oldwin, curbuf,
			       (flags & ECMD_HIDE) ? 0 : DOBUF_UNLOAD, FALSE);

		the_curwin->w_closing = FALSE;
		--buf->b_locked;

#ifdef FEAT_EVAL
		/* autocmds may abort script processing */
		if (aborting() && curwin->w_buffer != NULL)
		{
		    vim_free(new_name);
		    goto theend;
		}
#endif
		/* Be careful again, like above. */
		if (!bufref_valid(&au_new_curbuf))
		{
		    /* new buffer has been deleted */
		    delbuf_msg(new_name);	/* frees new_name */
		    goto theend;
		}
		if (buf == curbuf)		/* already in new buffer */
		    auto_buf = TRUE;
		else
		{
#ifdef FEAT_SYN_HL
		    /*
		     * <VN> We could instead free the synblock
		     * and re-attach to buffer, perhaps.
		     */
		    if (curwin->w_buffer == NULL
			    || curwin->w_s == &(curwin->w_buffer->b_s))
			curwin->w_s = &(buf->b_s);
#endif
		    curwin->w_buffer = buf;
		    curbuf = buf;
		    ++curbuf->b_nwindows;

		    /* Set 'fileformat', 'binary' and 'fenc' when forced. */
		    if (!oldbuf && eap != NULL)
		    {
			set_file_options(TRUE, eap);
			set_forced_fenc(eap);
		    }
		}

		/* May get the window options from the last time this buffer
		 * was in this window (or another window).  If not used
		 * before, reset the local window options to the global
		 * values.  Also restores old folding stuff. */
		get_winopts(curbuf);
#ifdef FEAT_SPELL
		did_get_winopts = TRUE;
#endif
	    }
	    vim_free(new_name);
	    au_new_curbuf.br_buf = NULL;
	    au_new_curbuf.br_buf_free_count = 0;
	}

	curwin->w_pcmark.lnum = 1;
	curwin->w_pcmark.col = 0;
    }
    else /* !other_file */
    {
	if ((flags & ECMD_ADDBUF) || check_fname() == FAIL)
	    goto theend;

	oldbuf = (flags & ECMD_OLDBUF);
    }

    /* Don't redraw until the cursor is in the right line, otherwise
     * autocommands may cause ml_get errors. */
    ++RedrawingDisabled;
    did_inc_redrawing_disabled = TRUE;

    buf = curbuf;
    if ((flags & ECMD_SET_HELP) || keep_help_flag)
    {
	prepare_help_buffer();
    }
    else
    {
	/* Don't make a buffer listed if it's a help buffer.  Useful when
	 * using CTRL-O to go back to a help file. */
	if (!curbuf->b_help)
	    set_buflisted(TRUE);
    }

    /* If autocommands change buffers under our fingers, forget about
     * editing the file. */
    if (buf != curbuf)
	goto theend;
#ifdef FEAT_EVAL
    if (aborting())	    /* autocmds may abort script processing */
	goto theend;
#endif

    /* Since we are starting to edit a file, consider the filetype to be
     * unset.  Helps for when an autocommand changes files and expects syntax
     * highlighting to work in the other file. */
    did_filetype = FALSE;

/*
 * other_file	oldbuf
 *  FALSE	FALSE	    re-edit same file, buffer is re-used
 *  FALSE	TRUE	    re-edit same file, nothing changes
 *  TRUE	FALSE	    start editing new file, new buffer
 *  TRUE	TRUE	    start editing in existing buffer (nothing to do)
 */
    if (!other_file && !oldbuf)		/* re-use the buffer */
    {
	set_last_cursor(curwin);	/* may set b_last_cursor */
	if (newlnum == ECMD_LAST || newlnum == ECMD_LASTL)
	{
	    newlnum = curwin->w_cursor.lnum;
	    solcol = curwin->w_cursor.col;
	}
	buf = curbuf;
	if (buf->b_fname != NULL)
	    new_name = vim_strsave(buf->b_fname);
	else
	    new_name = NULL;
	set_bufref(&bufref, buf);

	if (p_ur < 0 || curbuf->b_ml.ml_line_count <= p_ur)
	{
	    /* Save all the text, so that the reload can be undone.
	     * Sync first so that this is a separate undo-able action. */
	    u_sync(FALSE);
	    if (u_savecommon(0, curbuf->b_ml.ml_line_count + 1, 0, TRUE)
								     == FAIL)
	    {
		vim_free(new_name);
		goto theend;
	    }
	    u_unchanged(curbuf);
	    buf_freeall(curbuf, BFA_KEEP_UNDO);

	    /* tell readfile() not to clear or reload undo info */
	    readfile_flags = READ_KEEP_UNDO;
	}
	else
	    buf_freeall(curbuf, 0);   /* free all things for buffer */

	/* If autocommands deleted the buffer we were going to re-edit, give
	 * up and jump to the end. */
	if (!bufref_valid(&bufref))
	{
	    delbuf_msg(new_name);	/* frees new_name */
	    goto theend;
	}
	vim_free(new_name);

	/* If autocommands change buffers under our fingers, forget about
	 * re-editing the file.  Should do the buf_clear_file(), but perhaps
	 * the autocommands changed the buffer... */
	if (buf != curbuf)
	    goto theend;
#ifdef FEAT_EVAL
	if (aborting())	    /* autocmds may abort script processing */
	    goto theend;
#endif
	buf_clear_file(curbuf);
	curbuf->b_op_start.lnum = 0;	/* clear '[ and '] marks */
	curbuf->b_op_end.lnum = 0;
    }

/*
 * If we get here we are sure to start editing
 */
    /* Assume success now */
    retval = OK;

    /*
     * Check if we are editing the w_arg_idx file in the argument list.
     */
    check_arg_idx(curwin);

    if (!auto_buf)
    {
	/*
	 * Set cursor and init window before reading the file and executing
	 * autocommands.  This allows for the autocommands to position the
	 * cursor.
	 */
	curwin_init();

#ifdef FEAT_FOLDING
	/* It's possible that all lines in the buffer changed.  Need to update
	 * automatic folding for all windows where it's used. */
	{
	    win_T	    *win;
	    tabpage_T	    *tp;

	    FOR_ALL_TAB_WINDOWS(tp, win)
		if (win->w_buffer == curbuf)
		    foldUpdateAll(win);
	}
#endif

	/* Change directories when the 'acd' option is set. */
	DO_AUTOCHDIR;

	/*
	 * Careful: open_buffer() and apply_autocmds() may change the current
	 * buffer and window.
	 */
	orig_pos = curwin->w_cursor;
	topline = curwin->w_topline;
	if (!oldbuf)			    /* need to read the file */
	{
#if defined(HAS_SWAP_EXISTS_ACTION)
	    swap_exists_action = SEA_DIALOG;
#endif
	    curbuf->b_flags |= BF_CHECK_RO; /* set/reset 'ro' flag */

	    /*
	     * Open the buffer and read the file.
	     */
#if defined(FEAT_EVAL)
	    if (should_abort(open_buffer(FALSE, eap, readfile_flags)))
		retval = FAIL;
#else
	    (void)open_buffer(FALSE, eap, readfile_flags);
#endif

#if defined(HAS_SWAP_EXISTS_ACTION)
	    if (swap_exists_action == SEA_QUIT)
		retval = FAIL;
	    handle_swap_exists(&old_curbuf);
#endif
	}
	else
	{
	    /* Read the modelines, but only to set window-local options.  Any
	     * buffer-local options have already been set and may have been
	     * changed by the user. */
	    do_modelines(OPT_WINONLY);

	    apply_autocmds_retval(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf,
								    &retval);
	    apply_autocmds_retval(EVENT_BUFWINENTER, NULL, NULL, FALSE, curbuf,
								    &retval);
	}
	check_arg_idx(curwin);

	/* If autocommands change the cursor position or topline, we should
	 * keep it.  Also when it moves within a line. But not when it moves
	 * to the first non-blank. */
	if (!EQUAL_POS(curwin->w_cursor, orig_pos))
	{
	    char_u *text = ml_get_curline();

	    if (curwin->w_cursor.lnum != orig_pos.lnum
		    || curwin->w_cursor.col != (int)(skipwhite(text) - text))
	    {
		newlnum = curwin->w_cursor.lnum;
		newcol = curwin->w_cursor.col;
	    }
	}
	if (curwin->w_topline == topline)
	    topline = 0;

	/* Even when cursor didn't move we need to recompute topline. */
	changed_line_abv_curs();

#ifdef FEAT_TITLE
	maketitle();
#endif
    }

#ifdef FEAT_DIFF
    /* Tell the diff stuff that this buffer is new and/or needs updating.
     * Also needed when re-editing the same buffer, because unloading will
     * have removed it as a diff buffer. */
    if (curwin->w_p_diff)
    {
	diff_buf_add(curbuf);
	diff_invalidate(curbuf);
    }
#endif

#ifdef FEAT_SPELL
    /* If the window options were changed may need to set the spell language.
     * Can only do this after the buffer has been properly setup. */
    if (did_get_winopts && curwin->w_p_spell && *curwin->w_s->b_p_spl != NUL)
	(void)did_set_spelllang(curwin);
#endif

    if (command == NULL)
    {
	if (newcol >= 0)	/* position set by autocommands */
	{
	    curwin->w_cursor.lnum = newlnum;
	    curwin->w_cursor.col = newcol;
	    check_cursor();
	}
	else if (newlnum > 0)	/* line number from caller or old position */
	{
	    curwin->w_cursor.lnum = newlnum;
	    check_cursor_lnum();
	    if (solcol >= 0 && !p_sol)
	    {
		/* 'sol' is off: Use last known column. */
		curwin->w_cursor.col = solcol;
		check_cursor_col();
		curwin->w_cursor.coladd = 0;
		curwin->w_set_curswant = TRUE;
	    }
	    else
		beginline(BL_SOL | BL_FIX);
	}
	else			/* no line number, go to last line in Ex mode */
	{
	    if (exmode_active)
		curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	    beginline(BL_WHITE | BL_FIX);
	}
    }

    /* Check if cursors in other windows on the same buffer are still valid */
    check_lnums(FALSE);

    /*
     * Did not read the file, need to show some info about the file.
     * Do this after setting the cursor.
     */
    if (oldbuf && !auto_buf)
    {
	int	msg_scroll_save = msg_scroll;

	/* Obey the 'O' flag in 'cpoptions': overwrite any previous file
	 * message. */
	if (shortmess(SHM_OVERALL) && !exiting && p_verbose == 0)
	    msg_scroll = FALSE;
	if (!msg_scroll)	/* wait a bit when overwriting an error msg */
	    check_for_delay(FALSE);
	msg_start();
	msg_scroll = msg_scroll_save;
	msg_scrolled_ign = TRUE;

	if (!shortmess(SHM_FILEINFO))
	    fileinfo(FALSE, TRUE, FALSE);

	msg_scrolled_ign = FALSE;
    }

#ifdef FEAT_VIMINFO
    curbuf->b_last_used = vim_time();
#endif

    if (command != NULL)
	do_cmdline(command, NULL, NULL, DOCMD_VERBOSE);

#ifdef FEAT_KEYMAP
    if (curbuf->b_kmap_state & KEYMAP_INIT)
	(void)keymap_init();
#endif

    --RedrawingDisabled;
    did_inc_redrawing_disabled = FALSE;
    if (!skip_redraw)
    {
	n = *so_ptr;
	if (topline == 0 && command == NULL)
	    *so_ptr = 9999;		// force cursor halfway the window
	update_topline();
	curwin->w_scbind_pos = curwin->w_topline;
	*so_ptr = n;
	redraw_curbuf_later(NOT_VALID);	/* redraw this buffer later */
    }

    if (p_im)
	need_start_insertmode = TRUE;

#ifdef FEAT_AUTOCHDIR
    /* Change directories when the 'acd' option is set and we aren't already in
     * that directory (should already be done above). Expect getcwd() to be
     * faster than calling shorten_fnames() unnecessarily. */
    if (p_acd && curbuf->b_ffname != NULL)
    {
	char_u	curdir[MAXPATHL];
	char_u	filedir[MAXPATHL];

	vim_strncpy(filedir, curbuf->b_ffname, MAXPATHL - 1);
	*gettail_sep(filedir) = NUL;
	if (mch_dirname(curdir, MAXPATHL) != FAIL
		&& vim_fnamecmp(curdir, filedir) != 0)
	    do_autochdir();
    }
#endif

#if defined(FEAT_NETBEANS_INTG)
    if (curbuf->b_ffname != NULL)
    {
# ifdef FEAT_NETBEANS_INTG
	if ((flags & ECMD_SET_HELP) != ECMD_SET_HELP)
	    netbeans_file_opened(curbuf);
# endif
    }
#endif

theend:
    if (did_inc_redrawing_disabled)
	--RedrawingDisabled;
#if defined(FEAT_EVAL)
    if (did_set_swapcommand)
	set_vim_var_string(VV_SWAPCOMMAND, NULL, -1);
#endif
#ifdef FEAT_BROWSE
    vim_free(browse_file);
#endif
    vim_free(free_fname);
    return retval;
}

    static void
delbuf_msg(char_u *name)
{
    semsg(_("E143: Autocommands unexpectedly deleted new buffer %s"),
	    name == NULL ? (char_u *)"" : name);
    vim_free(name);
    au_new_curbuf.br_buf = NULL;
    au_new_curbuf.br_buf_free_count = 0;
}

static int append_indent = 0;	    /* autoindent for first line */

/*
 * ":insert" and ":append", also used by ":change"
 */
    void
ex_append(exarg_T *eap)
{
    char_u	*theline;
    int		did_undo = FALSE;
    linenr_T	lnum = eap->line2;
    int		indent = 0;
    char_u	*p;
    int		vcol;
    int		empty = (curbuf->b_ml.ml_flags & ML_EMPTY);

    /* the ! flag toggles autoindent */
    if (eap->forceit)
	curbuf->b_p_ai = !curbuf->b_p_ai;

    /* First autoindent comes from the line we start on */
    if (eap->cmdidx != CMD_change && curbuf->b_p_ai && lnum > 0)
	append_indent = get_indent_lnum(lnum);

    if (eap->cmdidx != CMD_append)
	--lnum;

    /* when the buffer is empty need to delete the dummy line */
    if (empty && lnum == 1)
	lnum = 0;

    State = INSERT;		    /* behave like in Insert mode */
    if (curbuf->b_p_iminsert == B_IMODE_LMAP)
	State |= LANGMAP;

    for (;;)
    {
	msg_scroll = TRUE;
	need_wait_return = FALSE;
	if (curbuf->b_p_ai)
	{
	    if (append_indent >= 0)
	    {
		indent = append_indent;
		append_indent = -1;
	    }
	    else if (lnum > 0)
		indent = get_indent_lnum(lnum);
	}
	ex_keep_indent = FALSE;
	if (eap->getline == NULL)
	{
	    /* No getline() function, use the lines that follow. This ends
	     * when there is no more. */
	    if (eap->nextcmd == NULL || *eap->nextcmd == NUL)
		break;
	    p = vim_strchr(eap->nextcmd, NL);
	    if (p == NULL)
		p = eap->nextcmd + STRLEN(eap->nextcmd);
	    theline = vim_strnsave(eap->nextcmd, (int)(p - eap->nextcmd));
	    if (*p != NUL)
		++p;
	    eap->nextcmd = p;
	}
	else
	{
	    int save_State = State;

	    /* Set State to avoid the cursor shape to be set to INSERT mode
	     * when getline() returns. */
	    State = CMDLINE;
	    theline = eap->getline(
#ifdef FEAT_EVAL
		    eap->cstack->cs_looplevel > 0 ? -1 :
#endif
		    NUL, eap->cookie, indent);
	    State = save_State;
	}
	lines_left = Rows - 1;
	if (theline == NULL)
	    break;

	/* Using ^ CTRL-D in getexmodeline() makes us repeat the indent. */
	if (ex_keep_indent)
	    append_indent = indent;

	/* Look for the "." after automatic indent. */
	vcol = 0;
	for (p = theline; indent > vcol; ++p)
	{
	    if (*p == ' ')
		++vcol;
	    else if (*p == TAB)
		vcol += 8 - vcol % 8;
	    else
		break;
	}
	if ((p[0] == '.' && p[1] == NUL)
		|| (!did_undo && u_save(lnum, lnum + 1 + (empty ? 1 : 0))
								     == FAIL))
	{
	    vim_free(theline);
	    break;
	}

	/* don't use autoindent if nothing was typed. */
	if (p[0] == NUL)
	    theline[0] = NUL;

	did_undo = TRUE;
	ml_append(lnum, theline, (colnr_T)0, FALSE);
	appended_lines_mark(lnum + (empty ? 1 : 0), 1L);

	vim_free(theline);
	++lnum;

	if (empty)
	{
	    ml_delete(2L, FALSE);
	    empty = FALSE;
	}
    }
    State = NORMAL;

    if (eap->forceit)
	curbuf->b_p_ai = !curbuf->b_p_ai;

    /* "start" is set to eap->line2+1 unless that position is invalid (when
     * eap->line2 pointed to the end of the buffer and nothing was appended)
     * "end" is set to lnum when something has been appended, otherwise
     * it is the same than "start"  -- Acevedo */
    curbuf->b_op_start.lnum = (eap->line2 < curbuf->b_ml.ml_line_count) ?
	eap->line2 + 1 : curbuf->b_ml.ml_line_count;
    if (eap->cmdidx != CMD_append)
	--curbuf->b_op_start.lnum;
    curbuf->b_op_end.lnum = (eap->line2 < lnum)
					     ? lnum : curbuf->b_op_start.lnum;
    curbuf->b_op_start.col = curbuf->b_op_end.col = 0;
    curwin->w_cursor.lnum = lnum;
    check_cursor_lnum();
    beginline(BL_SOL | BL_FIX);

    need_wait_return = FALSE;	/* don't use wait_return() now */
    ex_no_reprint = TRUE;
}

/*
 * ":change"
 */
    void
ex_change(exarg_T *eap)
{
    linenr_T	lnum;

    if (eap->line2 >= eap->line1
	    && u_save(eap->line1 - 1, eap->line2 + 1) == FAIL)
	return;

    /* the ! flag toggles autoindent */
    if (eap->forceit ? !curbuf->b_p_ai : curbuf->b_p_ai)
	append_indent = get_indent_lnum(eap->line1);

    for (lnum = eap->line2; lnum >= eap->line1; --lnum)
    {
	if (curbuf->b_ml.ml_flags & ML_EMPTY)	    /* nothing to delete */
	    break;
	ml_delete(eap->line1, FALSE);
    }

    /* make sure the cursor is not beyond the end of the file now */
    check_cursor_lnum();
    deleted_lines_mark(eap->line1, (long)(eap->line2 - lnum));

    /* ":append" on the line above the deleted lines. */
    eap->line2 = eap->line1;
    ex_append(eap);
}

    void
ex_z(exarg_T *eap)
{
    char_u	*x;
    long	bigness;
    char_u	*kind;
    int		minus = 0;
    linenr_T	start, end, curs, i;
    int		j;
    linenr_T	lnum = eap->line2;

    /* Vi compatible: ":z!" uses display height, without a count uses
     * 'scroll' */
    if (eap->forceit)
	bigness = curwin->w_height;
    else if (!ONE_WINDOW)
	bigness = curwin->w_height - 3;
    else
	bigness = curwin->w_p_scr * 2;
    if (bigness < 1)
	bigness = 1;

    x = eap->arg;
    kind = x;
    if (*kind == '-' || *kind == '+' || *kind == '='
					      || *kind == '^' || *kind == '.')
	++x;
    while (*x == '-' || *x == '+')
	++x;

    if (*x != 0)
    {
	if (!VIM_ISDIGIT(*x))
	{
	    emsg(_("E144: non-numeric argument to :z"));
	    return;
	}
	else
	{
	    bigness = atol((char *)x);

	    /* bigness could be < 0 if atol(x) overflows. */
	    if (bigness > 2 * curbuf->b_ml.ml_line_count || bigness < 0)
		bigness = 2 * curbuf->b_ml.ml_line_count;

	    p_window = bigness;
	    if (*kind == '=')
		bigness += 2;
	}
    }

    /* the number of '-' and '+' multiplies the distance */
    if (*kind == '-' || *kind == '+')
	for (x = kind + 1; *x == *kind; ++x)
	    ;

    switch (*kind)
    {
	case '-':
	    start = lnum - bigness * (linenr_T)(x - kind) + 1;
	    end = start + bigness - 1;
	    curs = end;
	    break;

	case '=':
	    start = lnum - (bigness + 1) / 2 + 1;
	    end = lnum + (bigness + 1) / 2 - 1;
	    curs = lnum;
	    minus = 1;
	    break;

	case '^':
	    start = lnum - bigness * 2;
	    end = lnum - bigness;
	    curs = lnum - bigness;
	    break;

	case '.':
	    start = lnum - (bigness + 1) / 2 + 1;
	    end = lnum + (bigness + 1) / 2 - 1;
	    curs = end;
	    break;

	default:  /* '+' */
	    start = lnum;
	    if (*kind == '+')
		start += bigness * (linenr_T)(x - kind - 1) + 1;
	    else if (eap->addr_count == 0)
		++start;
	    end = start + bigness - 1;
	    curs = end;
	    break;
    }

    if (start < 1)
	start = 1;

    if (end > curbuf->b_ml.ml_line_count)
	end = curbuf->b_ml.ml_line_count;

    if (curs > curbuf->b_ml.ml_line_count)
	curs = curbuf->b_ml.ml_line_count;
    else if (curs < 1)
	curs = 1;

    for (i = start; i <= end; i++)
    {
	if (minus && i == lnum)
	{
	    msg_putchar('\n');

	    for (j = 1; j < Columns; j++)
		msg_putchar('-');
	}

	print_line(i, eap->flags & EXFLAG_NR, eap->flags & EXFLAG_LIST);

	if (minus && i == lnum)
	{
	    msg_putchar('\n');

	    for (j = 1; j < Columns; j++)
		msg_putchar('-');
	}
    }

    if (curwin->w_cursor.lnum != curs)
    {
	curwin->w_cursor.lnum = curs;
	curwin->w_cursor.col = 0;
    }
    ex_no_reprint = TRUE;
}

/*
 * Check if the restricted flag is set.
 * If so, give an error message and return TRUE.
 * Otherwise, return FALSE.
 */
    int
check_restricted(void)
{
    if (restricted)
    {
	emsg(_("E145: Shell commands and some functionality not allowed in rvim"));
	return TRUE;
    }
    return FALSE;
}

/*
 * Check if the secure flag is set (.exrc or .vimrc in current directory).
 * If so, give an error message and return TRUE.
 * Otherwise, return FALSE.
 */
    int
check_secure(void)
{
    if (secure)
    {
	secure = 2;
	emsg(_(e_curdir));
	return TRUE;
    }
#ifdef HAVE_SANDBOX
    /*
     * In the sandbox more things are not allowed, including the things
     * disallowed in secure mode.
     */
    if (sandbox != 0)
    {
	emsg(_(e_sandbox));
	return TRUE;
    }
#endif
    return FALSE;
}

static char_u	*old_sub = NULL;	/* previous substitute pattern */
static int	global_need_beginline;	/* call beginline() after ":g" */

/*
 * Flags that are kept between calls to :substitute.
 */
typedef struct {
    int	do_all;		/* do multiple substitutions per line */
    int	do_ask;		/* ask for confirmation */
    int	do_count;	/* count only */
    int	do_error;	/* if false, ignore errors */
    int	do_print;	/* print last line with subs. */
    int	do_list;	/* list last line with subs. */
    int	do_number;	/* list last line with line nr*/
    int	do_ic;		/* ignore case flag */
} subflags_T;

/* do_sub()
 *
 * Perform a substitution from line eap->line1 to line eap->line2 using the
 * command pointed to by eap->arg which should be of the form:
 *
 * /pattern/substitution/{flags}
 *
 * The usual escapes are supported as described in the regexp docs.
 */
    void
do_sub(exarg_T *eap)
{
    linenr_T	lnum;
    long	i = 0;
    regmmatch_T regmatch;
    static subflags_T subflags = {FALSE, FALSE, FALSE, TRUE, FALSE,
							      FALSE, FALSE, 0};
#ifdef FEAT_EVAL
    subflags_T	subflags_save;
#endif
    int		save_do_all;		/* remember user specified 'g' flag */
    int		save_do_ask;		/* remember user specified 'c' flag */
    char_u	*pat = NULL, *sub = NULL;	/* init for GCC */
    int		delimiter;
    int		sublen;
    int		got_quit = FALSE;
    int		got_match = FALSE;
    int		temp;
    int		which_pat;
    char_u	*cmd;
    int		save_State;
    linenr_T	first_line = 0;		/* first changed line */
    linenr_T	last_line= 0;		/* below last changed line AFTER the
					 * change */
    linenr_T	old_line_count = curbuf->b_ml.ml_line_count;
    linenr_T	line2;
    long	nmatch;			/* number of lines in match */
    char_u	*sub_firstline;		/* allocated copy of first sub line */
    int		endcolumn = FALSE;	/* cursor in last column when done */
    pos_T	old_cursor = curwin->w_cursor;
    int		start_nsubs;
#ifdef FEAT_EVAL
    int		save_ma = 0;
#endif

    cmd = eap->arg;
    if (!global_busy)
    {
	sub_nsubs = 0;
	sub_nlines = 0;
    }
    start_nsubs = sub_nsubs;

    if (eap->cmdidx == CMD_tilde)
	which_pat = RE_LAST;	/* use last used regexp */
    else
	which_pat = RE_SUBST;	/* use last substitute regexp */

				/* new pattern and substitution */
    if (eap->cmd[0] == 's' && *cmd != NUL && !VIM_ISWHITE(*cmd)
		&& vim_strchr((char_u *)"0123456789cegriIp|\"", *cmd) == NULL)
    {
				/* don't accept alphanumeric for separator */
	if (isalpha(*cmd))
	{
	    emsg(_("E146: Regular expressions can't be delimited by letters"));
	    return;
	}
	/*
	 * undocumented vi feature:
	 *  "\/sub/" and "\?sub?" use last used search pattern (almost like
	 *  //sub/r).  "\&sub&" use last substitute pattern (like //sub/).
	 */
	if (*cmd == '\\')
	{
	    ++cmd;
	    if (vim_strchr((char_u *)"/?&", *cmd) == NULL)
	    {
		emsg(_(e_backslash));
		return;
	    }
	    if (*cmd != '&')
		which_pat = RE_SEARCH;	    /* use last '/' pattern */
	    pat = (char_u *)"";		    /* empty search pattern */
	    delimiter = *cmd++;		    /* remember delimiter character */
	}
	else		/* find the end of the regexp */
	{
	    which_pat = RE_LAST;	    /* use last used regexp */
	    delimiter = *cmd++;		    /* remember delimiter character */
	    pat = cmd;			    /* remember start of search pat */
	    cmd = skip_regexp(cmd, delimiter, p_magic, &eap->arg);
	    if (cmd[0] == delimiter)	    /* end delimiter found */
		*cmd++ = NUL;		    /* replace it with a NUL */
	}

	/*
	 * Small incompatibility: vi sees '\n' as end of the command, but in
	 * Vim we want to use '\n' to find/substitute a NUL.
	 */
	sub = cmd;	    /* remember the start of the substitution */

	while (cmd[0])
	{
	    if (cmd[0] == delimiter)		/* end delimiter found */
	    {
		*cmd++ = NUL;			/* replace it with a NUL */
		break;
	    }
	    if (cmd[0] == '\\' && cmd[1] != 0)	/* skip escaped characters */
		++cmd;
	    MB_PTR_ADV(cmd);
	}

	if (!eap->skip)
	{
	    /* In POSIX vi ":s/pat/%/" uses the previous subst. string. */
	    if (STRCMP(sub, "%") == 0
				 && vim_strchr(p_cpo, CPO_SUBPERCENT) != NULL)
	    {
		if (old_sub == NULL)	/* there is no previous command */
		{
		    emsg(_(e_nopresub));
		    return;
		}
		sub = old_sub;
	    }
	    else
	    {
		vim_free(old_sub);
		old_sub = vim_strsave(sub);
	    }
	}
    }
    else if (!eap->skip)	/* use previous pattern and substitution */
    {
	if (old_sub == NULL)	/* there is no previous command */
	{
	    emsg(_(e_nopresub));
	    return;
	}
	pat = NULL;		/* search_regcomp() will use previous pattern */
	sub = old_sub;

	/* Vi compatibility quirk: repeating with ":s" keeps the cursor in the
	 * last column after using "$". */
	endcolumn = (curwin->w_curswant == MAXCOL);
    }

    /* Recognize ":%s/\n//" and turn it into a join command, which is much
     * more efficient.
     * TODO: find a generic solution to make line-joining operations more
     * efficient, avoid allocating a string that grows in size.
     */
    if (pat != NULL && STRCMP(pat, "\\n") == 0
	    && *sub == NUL
	    && (*cmd == NUL || (cmd[1] == NUL && (*cmd == 'g' || *cmd == 'l'
					     || *cmd == 'p' || *cmd == '#'))))
    {
	linenr_T    joined_lines_count;

	curwin->w_cursor.lnum = eap->line1;
	if (*cmd == 'l')
	    eap->flags = EXFLAG_LIST;
	else if (*cmd == '#')
	    eap->flags = EXFLAG_NR;
	else if (*cmd == 'p')
	    eap->flags = EXFLAG_PRINT;

	/* The number of lines joined is the number of lines in the range plus
	 * one.  One less when the last line is included. */
	joined_lines_count = eap->line2 - eap->line1 + 1;
	if (eap->line2 < curbuf->b_ml.ml_line_count)
	    ++joined_lines_count;
	if (joined_lines_count > 1)
	{
	    (void)do_join(joined_lines_count, FALSE, TRUE, FALSE, TRUE);
	    sub_nsubs = joined_lines_count - 1;
	    sub_nlines = 1;
	    (void)do_sub_msg(FALSE);
	    ex_may_print(eap);
	}

	if (!cmdmod.keeppatterns)
	    save_re_pat(RE_SUBST, pat, p_magic);
#ifdef FEAT_CMDHIST
	/* put pattern in history */
	add_to_history(HIST_SEARCH, pat, TRUE, NUL);
#endif

	return;
    }

    /*
     * Find trailing options.  When '&' is used, keep old options.
     */
    if (*cmd == '&')
	++cmd;
    else
    {
	if (!p_ed)
	{
	    if (p_gd)		/* default is global on */
		subflags.do_all = TRUE;
	    else
		subflags.do_all = FALSE;
	    subflags.do_ask = FALSE;
	}
	subflags.do_error = TRUE;
	subflags.do_print = FALSE;
	subflags.do_list = FALSE;
	subflags.do_count = FALSE;
	subflags.do_number = FALSE;
	subflags.do_ic = 0;
    }
    while (*cmd)
    {
	/*
	 * Note that 'g' and 'c' are always inverted, also when p_ed is off.
	 * 'r' is never inverted.
	 */
	if (*cmd == 'g')
	    subflags.do_all = !subflags.do_all;
	else if (*cmd == 'c')
	    subflags.do_ask = !subflags.do_ask;
	else if (*cmd == 'n')
	    subflags.do_count = TRUE;
	else if (*cmd == 'e')
	    subflags.do_error = !subflags.do_error;
	else if (*cmd == 'r')	    /* use last used regexp */
	    which_pat = RE_LAST;
	else if (*cmd == 'p')
	    subflags.do_print = TRUE;
	else if (*cmd == '#')
	{
	    subflags.do_print = TRUE;
	    subflags.do_number = TRUE;
	}
	else if (*cmd == 'l')
	{
	    subflags.do_print = TRUE;
	    subflags.do_list = TRUE;
	}
	else if (*cmd == 'i')	    /* ignore case */
	    subflags.do_ic = 'i';
	else if (*cmd == 'I')	    /* don't ignore case */
	    subflags.do_ic = 'I';
	else
	    break;
	++cmd;
    }
    if (subflags.do_count)
	subflags.do_ask = FALSE;

    save_do_all = subflags.do_all;
    save_do_ask = subflags.do_ask;

    /*
     * check for a trailing count
     */
    cmd = skipwhite(cmd);
    if (VIM_ISDIGIT(*cmd))
    {
	i = getdigits(&cmd);
	if (i <= 0 && !eap->skip && subflags.do_error)
	{
	    emsg(_(e_zerocount));
	    return;
	}
	eap->line1 = eap->line2;
	eap->line2 += i - 1;
	if (eap->line2 > curbuf->b_ml.ml_line_count)
	    eap->line2 = curbuf->b_ml.ml_line_count;
    }

    /*
     * check for trailing command or garbage
     */
    cmd = skipwhite(cmd);
    if (*cmd && *cmd != '"')	    /* if not end-of-line or comment */
    {
	eap->nextcmd = check_nextcmd(cmd);
	if (eap->nextcmd == NULL)
	{
	    emsg(_(e_trailing));
	    return;
	}
    }

    if (eap->skip)	    /* not executing commands, only parsing */
	return;

    if (!subflags.do_count && !curbuf->b_p_ma)
    {
	/* Substitution is not allowed in non-'modifiable' buffer */
	emsg(_(e_modifiable));
	return;
    }

    if (search_regcomp(pat, RE_SUBST, which_pat, SEARCH_HIS, &regmatch) == FAIL)
    {
	if (subflags.do_error)
	    emsg(_(e_invcmd));
	return;
    }

    /* the 'i' or 'I' flag overrules 'ignorecase' and 'smartcase' */
    if (subflags.do_ic == 'i')
	regmatch.rmm_ic = TRUE;
    else if (subflags.do_ic == 'I')
	regmatch.rmm_ic = FALSE;

    sub_firstline = NULL;

    /*
     * ~ in the substitute pattern is replaced with the old pattern.
     * We do it here once to avoid it to be replaced over and over again.
     * But don't do it when it starts with "\=", then it's an expression.
     */
    if (!(sub[0] == '\\' && sub[1] == '='))
	sub = regtilde(sub, p_magic);

    /*
     * Check for a match on each line.
     */
    line2 = eap->line2;
    for (lnum = eap->line1; lnum <= line2 && !(got_quit
#if defined(FEAT_EVAL)
		|| aborting()
#endif
		); ++lnum)
    {
	nmatch = vim_regexec_multi(&regmatch, curwin, curbuf, lnum,
						       (colnr_T)0, NULL, NULL);
	if (nmatch)
	{
	    colnr_T	copycol;
	    colnr_T	matchcol;
	    colnr_T	prev_matchcol = MAXCOL;
	    char_u	*new_end, *new_start = NULL;
	    unsigned	new_start_len = 0;
	    char_u	*p1;
	    int		did_sub = FALSE;
	    int		lastone;
	    int		len, copy_len, needed_len;
	    long	nmatch_tl = 0;	/* nr of lines matched below lnum */
	    int		do_again;	/* do it again after joining lines */
	    int		skip_match = FALSE;
	    linenr_T	sub_firstlnum;	/* nr of first sub line */

	    /*
	     * The new text is build up step by step, to avoid too much
	     * copying.  There are these pieces:
	     * sub_firstline	The old text, unmodified.
	     * copycol		Column in the old text where we started
	     *			looking for a match; from here old text still
	     *			needs to be copied to the new text.
	     * matchcol		Column number of the old text where to look
	     *			for the next match.  It's just after the
	     *			previous match or one further.
	     * prev_matchcol	Column just after the previous match (if any).
	     *			Mostly equal to matchcol, except for the first
	     *			match and after skipping an empty match.
	     * regmatch.*pos	Where the pattern matched in the old text.
	     * new_start	The new text, all that has been produced so
	     *			far.
	     * new_end		The new text, where to append new text.
	     *
	     * lnum		The line number where we found the start of
	     *			the match.  Can be below the line we searched
	     *			when there is a \n before a \zs in the
	     *			pattern.
	     * sub_firstlnum	The line number in the buffer where to look
	     *			for a match.  Can be different from "lnum"
	     *			when the pattern or substitute string contains
	     *			line breaks.
	     *
	     * Special situations:
	     * - When the substitute string contains a line break, the part up
	     *   to the line break is inserted in the text, but the copy of
	     *   the original line is kept.  "sub_firstlnum" is adjusted for
	     *   the inserted lines.
	     * - When the matched pattern contains a line break, the old line
	     *   is taken from the line at the end of the pattern.  The lines
	     *   in the match are deleted later, "sub_firstlnum" is adjusted
	     *   accordingly.
	     *
	     * The new text is built up in new_start[].  It has some extra
	     * room to avoid using alloc()/free() too often.  new_start_len is
	     * the length of the allocated memory at new_start.
	     *
	     * Make a copy of the old line, so it won't be taken away when
	     * updating the screen or handling a multi-line match.  The "old_"
	     * pointers point into this copy.
	     */
	    sub_firstlnum = lnum;
	    copycol = 0;
	    matchcol = 0;

	    /* At first match, remember current cursor position. */
	    if (!got_match)
	    {
		setpcmark();
		got_match = TRUE;
	    }

	    /*
	     * Loop until nothing more to replace in this line.
	     * 1. Handle match with empty string.
	     * 2. If do_ask is set, ask for confirmation.
	     * 3. substitute the string.
	     * 4. if do_all is set, find next match
	     * 5. break if there isn't another match in this line
	     */
	    for (;;)
	    {
		/* Advance "lnum" to the line where the match starts.  The
		 * match does not start in the first line when there is a line
		 * break before \zs. */
		if (regmatch.startpos[0].lnum > 0)
		{
		    lnum += regmatch.startpos[0].lnum;
		    sub_firstlnum += regmatch.startpos[0].lnum;
		    nmatch -= regmatch.startpos[0].lnum;
		    VIM_CLEAR(sub_firstline);
		}

		if (sub_firstline == NULL)
		{
		    sub_firstline = vim_strsave(ml_get(sub_firstlnum));
		    if (sub_firstline == NULL)
		    {
			vim_free(new_start);
			goto outofmem;
		    }
		}

		/* Save the line number of the last change for the final
		 * cursor position (just like Vi). */
		curwin->w_cursor.lnum = lnum;
		do_again = FALSE;

		/*
		 * 1. Match empty string does not count, except for first
		 * match.  This reproduces the strange vi behaviour.
		 * This also catches endless loops.
		 */
		if (matchcol == prev_matchcol
			&& regmatch.endpos[0].lnum == 0
			&& matchcol == regmatch.endpos[0].col)
		{
		    if (sub_firstline[matchcol] == NUL)
			/* We already were at the end of the line.  Don't look
			 * for a match in this line again. */
			skip_match = TRUE;
		    else
		    {
			 /* search for a match at next column */
			if (has_mbyte)
			    matchcol += mb_ptr2len(sub_firstline + matchcol);
			else
			    ++matchcol;
		    }
		    goto skip;
		}

		/* Normally we continue searching for a match just after the
		 * previous match. */
		matchcol = regmatch.endpos[0].col;
		prev_matchcol = matchcol;

		/*
		 * 2. If do_count is set only increase the counter.
		 *    If do_ask is set, ask for confirmation.
		 */
		if (subflags.do_count)
		{
		    /* For a multi-line match, put matchcol at the NUL at
		     * the end of the line and set nmatch to one, so that
		     * we continue looking for a match on the next line.
		     * Avoids that ":s/\nB\@=//gc" get stuck. */
		    if (nmatch > 1)
		    {
			matchcol = (colnr_T)STRLEN(sub_firstline);
			nmatch = 1;
			skip_match = TRUE;
		    }
		    sub_nsubs++;
		    did_sub = TRUE;
#ifdef FEAT_EVAL
		    /* Skip the substitution, unless an expression is used,
		     * then it is evaluated in the sandbox. */
		    if (!(sub[0] == '\\' && sub[1] == '='))
#endif
			goto skip;
		}

		if (subflags.do_ask)
		{
		    int typed = 0;

		    /* change State to CONFIRM, so that the mouse works
		     * properly */
		    save_State = State;
		    State = CONFIRM;
#ifdef FEAT_MOUSE
		    setmouse();		/* disable mouse in xterm */
#endif
		    curwin->w_cursor.col = regmatch.startpos[0].col;
		    if (curwin->w_p_crb)
			do_check_cursorbind();

		    /* When 'cpoptions' contains "u" don't sync undo when
		     * asking for confirmation. */
		    if (vim_strchr(p_cpo, CPO_UNDO) != NULL)
			++no_u_sync;

		    /*
		     * Loop until 'y', 'n', 'q', CTRL-E or CTRL-Y typed.
		     */
		    while (subflags.do_ask)
		    {
			if (exmode_active)
			{
			    char_u	*resp;
			    colnr_T	sc, ec;

			    print_line_no_prefix(lnum,
					 subflags.do_number, subflags.do_list);

			    getvcol(curwin, &curwin->w_cursor, &sc, NULL, NULL);
			    curwin->w_cursor.col = regmatch.endpos[0].col - 1;
			    if (curwin->w_cursor.col < 0)
				curwin->w_cursor.col = 0;
			    getvcol(curwin, &curwin->w_cursor, NULL, NULL, &ec);
			    if (subflags.do_number || curwin->w_p_nu)
			    {
				int numw = number_width(curwin) + 1;
				sc += numw;
				ec += numw;
			    }
			    msg_start();
			    for (i = 0; i < (long)sc; ++i)
				msg_putchar(' ');
			    for ( ; i <= (long)ec; ++i)
				msg_putchar('^');

			    resp = getexmodeline('?', NULL, 0);
			    if (resp != NULL)
			    {
				typed = *resp;
				vim_free(resp);
			    }
			}
			else
			{
			    char_u *orig_line = NULL;
			    int    len_change = 0;
#ifdef FEAT_FOLDING
			    int save_p_fen = curwin->w_p_fen;

			    curwin->w_p_fen = FALSE;
#endif
			    /* Invert the matched string.
			     * Remove the inversion afterwards. */
			    temp = RedrawingDisabled;
			    RedrawingDisabled = 0;

			    if (new_start != NULL)
			    {
				/* There already was a substitution, we would
				 * like to show this to the user.  We cannot
				 * really update the line, it would change
				 * what matches.  Temporarily replace the line
				 * and change it back afterwards. */
				orig_line = vim_strsave(ml_get(lnum));
				if (orig_line != NULL)
				{
				    char_u *new_line = concat_str(new_start,
						     sub_firstline + copycol);

				    if (new_line == NULL)
					VIM_CLEAR(orig_line);
				    else
				    {
					/* Position the cursor relative to the
					 * end of the line, the previous
					 * substitute may have inserted or
					 * deleted characters before the
					 * cursor. */
					len_change = (int)STRLEN(new_line)
						     - (int)STRLEN(orig_line);
					curwin->w_cursor.col += len_change;
					ml_replace(lnum, new_line, FALSE);
				    }
				}
			    }

			    search_match_lines = regmatch.endpos[0].lnum
						  - regmatch.startpos[0].lnum;
			    search_match_endcol = regmatch.endpos[0].col
								 + len_change;
			    highlight_match = TRUE;

			    update_topline();
			    validate_cursor();
			    update_screen(SOME_VALID);
			    highlight_match = FALSE;
			    redraw_later(SOME_VALID);

#ifdef FEAT_FOLDING
			    curwin->w_p_fen = save_p_fen;
#endif
			    if (msg_row == Rows - 1)
				msg_didout = FALSE;	/* avoid a scroll-up */
			    msg_starthere();
			    i = msg_scroll;
			    msg_scroll = 0;		/* truncate msg when
							   needed */
			    msg_no_more = TRUE;
			    /* write message same highlighting as for
			     * wait_return */
			    smsg_attr(HL_ATTR(HLF_R),
				_("replace with %s (y/n/a/q/l/^E/^Y)?"), sub);
			    msg_no_more = FALSE;
			    msg_scroll = i;
			    showruler(TRUE);
			    windgoto(msg_row, msg_col);
			    RedrawingDisabled = temp;

#ifdef USE_ON_FLY_SCROLL
			    dont_scroll = FALSE; /* allow scrolling here */
#endif
			    ++no_mapping;	/* don't map this key */
			    ++allow_keys;	/* allow special keys */
			    typed = plain_vgetc();
			    --allow_keys;
			    --no_mapping;

			    /* clear the question */
			    msg_didout = FALSE;	/* don't scroll up */
			    msg_col = 0;
			    gotocmdline(TRUE);

			    /* restore the line */
			    if (orig_line != NULL)
				ml_replace(lnum, orig_line, FALSE);
			}

			need_wait_return = FALSE; /* no hit-return prompt */
			if (typed == 'q' || typed == ESC || typed == Ctrl_C
#ifdef UNIX
				|| typed == intr_char
#endif
				)
			{
			    got_quit = TRUE;
			    break;
			}
			if (typed == 'n')
			    break;
			if (typed == 'y')
			    break;
			if (typed == 'l')
			{
			    /* last: replace and then stop */
			    subflags.do_all = FALSE;
			    line2 = lnum;
			    break;
			}
			if (typed == 'a')
			{
			    subflags.do_ask = FALSE;
			    break;
			}
#ifdef FEAT_INS_EXPAND
			if (typed == Ctrl_E)
			    scrollup_clamp();
			else if (typed == Ctrl_Y)
			    scrolldown_clamp();
#endif
		    }
		    State = save_State;
#ifdef FEAT_MOUSE
		    setmouse();
#endif
		    if (vim_strchr(p_cpo, CPO_UNDO) != NULL)
			--no_u_sync;

		    if (typed == 'n')
		    {
			/* For a multi-line match, put matchcol at the NUL at
			 * the end of the line and set nmatch to one, so that
			 * we continue looking for a match on the next line.
			 * Avoids that ":%s/\nB\@=//gc" and ":%s/\n/,\r/gc"
			 * get stuck when pressing 'n'. */
			if (nmatch > 1)
			{
			    matchcol = (colnr_T)STRLEN(sub_firstline);
			    skip_match = TRUE;
			}
			goto skip;
		    }
		    if (got_quit)
			goto skip;
		}

		/* Move the cursor to the start of the match, so that we can
		 * use "\=col("."). */
		curwin->w_cursor.col = regmatch.startpos[0].col;

		/*
		 * 3. substitute the string.
		 */
#ifdef FEAT_EVAL
		if (subflags.do_count)
		{
		    /* prevent accidentally changing the buffer by a function */
		    save_ma = curbuf->b_p_ma;
		    curbuf->b_p_ma = FALSE;
		    sandbox++;
		}
		/* Save flags for recursion.  They can change for e.g.
		 * :s/^/\=execute("s#^##gn") */
		subflags_save = subflags;
#endif
		/* get length of substitution part */
		sublen = vim_regsub_multi(&regmatch,
				    sub_firstlnum - regmatch.startpos[0].lnum,
				    sub, sub_firstline, FALSE, p_magic, TRUE);
#ifdef FEAT_EVAL
		/* Don't keep flags set by a recursive call. */
		subflags = subflags_save;
		if (subflags.do_count)
		{
		    curbuf->b_p_ma = save_ma;
		    if (sandbox > 0)
			sandbox--;
		    goto skip;
		}
#endif

		/* When the match included the "$" of the last line it may
		 * go beyond the last line of the buffer. */
		if (nmatch > curbuf->b_ml.ml_line_count - sub_firstlnum + 1)
		{
		    nmatch = curbuf->b_ml.ml_line_count - sub_firstlnum + 1;
		    skip_match = TRUE;
		}

		/* Need room for:
		 * - result so far in new_start (not for first sub in line)
		 * - original text up to match
		 * - length of substituted part
		 * - original text after match
		 * Adjust text properties here, since we have all information
		 * needed.
		 */
		if (nmatch == 1)
		{
		    p1 = sub_firstline;
#ifdef FEAT_TEXT_PROP
		    if (curbuf->b_has_textprop)
			adjust_prop_columns(lnum, regmatch.startpos[0].col,
			      sublen - 1 - (regmatch.endpos[0].col
						  - regmatch.startpos[0].col));
#endif
		}
		else
		{
		    p1 = ml_get(sub_firstlnum + nmatch - 1);
		    nmatch_tl += nmatch - 1;
		}
		copy_len = regmatch.startpos[0].col - copycol;
		needed_len = copy_len + ((unsigned)STRLEN(p1)
				       - regmatch.endpos[0].col) + sublen + 1;
		if (new_start == NULL)
		{
		    /*
		     * Get some space for a temporary buffer to do the
		     * substitution into (and some extra space to avoid
		     * too many calls to alloc()/free()).
		     */
		    new_start_len = needed_len + 50;
		    if ((new_start = alloc_check(new_start_len)) == NULL)
			goto outofmem;
		    *new_start = NUL;
		    new_end = new_start;
		}
		else
		{
		    /*
		     * Check if the temporary buffer is long enough to do the
		     * substitution into.  If not, make it larger (with a bit
		     * extra to avoid too many calls to alloc()/free()).
		     */
		    len = (unsigned)STRLEN(new_start);
		    needed_len += len;
		    if (needed_len > (int)new_start_len)
		    {
			new_start_len = needed_len + 50;
			if ((p1 = alloc_check(new_start_len)) == NULL)
			{
			    vim_free(new_start);
			    goto outofmem;
			}
			mch_memmove(p1, new_start, (size_t)(len + 1));
			vim_free(new_start);
			new_start = p1;
		    }
		    new_end = new_start + len;
		}

		/*
		 * copy the text up to the part that matched
		 */
		mch_memmove(new_end, sub_firstline + copycol, (size_t)copy_len);
		new_end += copy_len;

		(void)vim_regsub_multi(&regmatch,
				    sub_firstlnum - regmatch.startpos[0].lnum,
					   sub, new_end, TRUE, p_magic, TRUE);
		sub_nsubs++;
		did_sub = TRUE;

		/* Move the cursor to the start of the line, to avoid that it
		 * is beyond the end of the line after the substitution. */
		curwin->w_cursor.col = 0;

		/* For a multi-line match, make a copy of the last matched
		 * line and continue in that one. */
		if (nmatch > 1)
		{
		    sub_firstlnum += nmatch - 1;
		    vim_free(sub_firstline);
		    sub_firstline = vim_strsave(ml_get(sub_firstlnum));
		    /* When going beyond the last line, stop substituting. */
		    if (sub_firstlnum <= line2)
			do_again = TRUE;
		    else
			subflags.do_all = FALSE;
		}

		/* Remember next character to be copied. */
		copycol = regmatch.endpos[0].col;

		if (skip_match)
		{
		    /* Already hit end of the buffer, sub_firstlnum is one
		     * less than what it ought to be. */
		    vim_free(sub_firstline);
		    sub_firstline = vim_strsave((char_u *)"");
		    copycol = 0;
		}

		/*
		 * Now the trick is to replace CTRL-M chars with a real line
		 * break.  This would make it impossible to insert a CTRL-M in
		 * the text.  The line break can be avoided by preceding the
		 * CTRL-M with a backslash.  To be able to insert a backslash,
		 * they must be doubled in the string and are halved here.
		 * That is Vi compatible.
		 */
		for (p1 = new_end; *p1; ++p1)
		{
		    if (p1[0] == '\\' && p1[1] != NUL)  /* remove backslash */
			STRMOVE(p1, p1 + 1);
		    else if (*p1 == CAR)
		    {
			if (u_inssub(lnum) == OK)   // prepare for undo
			{
			    colnr_T	plen = (colnr_T)(p1 - new_start + 1);

			    *p1 = NUL;		    // truncate up to the CR
			    ml_append(lnum - 1, new_start, plen, FALSE);
			    mark_adjust(lnum + 1, (linenr_T)MAXLNUM, 1L, 0L);
			    if (subflags.do_ask)
				appended_lines(lnum - 1, 1L);
			    else
			    {
				if (first_line == 0)
				    first_line = lnum;
				last_line = lnum + 1;
			    }
#ifdef FEAT_TEXT_PROP
			    adjust_props_for_split(lnum, plen, 1);
#endif
			    // all line numbers increase
			    ++sub_firstlnum;
			    ++lnum;
			    ++line2;
			    // move the cursor to the new line, like Vi
			    ++curwin->w_cursor.lnum;
			    // copy the rest
			    STRMOVE(new_start, p1 + 1);
			    p1 = new_start - 1;
			}
		    }
		    else if (has_mbyte)
			p1 += (*mb_ptr2len)(p1) - 1;
		}

		/*
		 * 4. If do_all is set, find next match.
		 * Prevent endless loop with patterns that match empty
		 * strings, e.g. :s/$/pat/g or :s/[a-z]* /(&)/g.
		 * But ":s/\n/#/" is OK.
		 */
skip:
		/* We already know that we did the last subst when we are at
		 * the end of the line, except that a pattern like
		 * "bar\|\nfoo" may match at the NUL.  "lnum" can be below
		 * "line2" when there is a \zs in the pattern after a line
		 * break. */
		lastone = (skip_match
			|| got_int
			|| got_quit
			|| lnum > line2
			|| !(subflags.do_all || do_again)
			|| (sub_firstline[matchcol] == NUL && nmatch <= 1
					 && !re_multiline(regmatch.regprog)));
		nmatch = -1;

		/*
		 * Replace the line in the buffer when needed.  This is
		 * skipped when there are more matches.
		 * The check for nmatch_tl is needed for when multi-line
		 * matching must replace the lines before trying to do another
		 * match, otherwise "\@<=" won't work.
		 * When the match starts below where we start searching also
		 * need to replace the line first (using \zs after \n).
		 */
		if (lastone
			|| nmatch_tl > 0
			|| (nmatch = vim_regexec_multi(&regmatch, curwin,
							curbuf, sub_firstlnum,
						    matchcol, NULL, NULL)) == 0
			|| regmatch.startpos[0].lnum > 0)
		{
		    if (new_start != NULL)
		    {
			/*
			 * Copy the rest of the line, that didn't match.
			 * "matchcol" has to be adjusted, we use the end of
			 * the line as reference, because the substitute may
			 * have changed the number of characters.  Same for
			 * "prev_matchcol".
			 */
			STRCAT(new_start, sub_firstline + copycol);
			matchcol = (colnr_T)STRLEN(sub_firstline) - matchcol;
			prev_matchcol = (colnr_T)STRLEN(sub_firstline)
							      - prev_matchcol;

			if (u_savesub(lnum) != OK)
			    break;
			ml_replace(lnum, new_start, TRUE);

			if (nmatch_tl > 0)
			{
			    /*
			     * Matched lines have now been substituted and are
			     * useless, delete them.  The part after the match
			     * has been appended to new_start, we don't need
			     * it in the buffer.
			     */
			    ++lnum;
			    if (u_savedel(lnum, nmatch_tl) != OK)
				break;
			    for (i = 0; i < nmatch_tl; ++i)
				ml_delete(lnum, (int)FALSE);
			    mark_adjust(lnum, lnum + nmatch_tl - 1,
						   (long)MAXLNUM, -nmatch_tl);
			    if (subflags.do_ask)
				deleted_lines(lnum, nmatch_tl);
			    --lnum;
			    line2 -= nmatch_tl; /* nr of lines decreases */
			    nmatch_tl = 0;
			}

			/* When asking, undo is saved each time, must also set
			 * changed flag each time. */
			if (subflags.do_ask)
			    changed_bytes(lnum, 0);
			else
			{
			    if (first_line == 0)
				first_line = lnum;
			    last_line = lnum + 1;
			}

			sub_firstlnum = lnum;
			vim_free(sub_firstline);    /* free the temp buffer */
			sub_firstline = new_start;
			new_start = NULL;
			matchcol = (colnr_T)STRLEN(sub_firstline) - matchcol;
			prev_matchcol = (colnr_T)STRLEN(sub_firstline)
							      - prev_matchcol;
			copycol = 0;
		    }
		    if (nmatch == -1 && !lastone)
			nmatch = vim_regexec_multi(&regmatch, curwin, curbuf,
					  sub_firstlnum, matchcol, NULL, NULL);

		    /*
		     * 5. break if there isn't another match in this line
		     */
		    if (nmatch <= 0)
		    {
			/* If the match found didn't start where we were
			 * searching, do the next search in the line where we
			 * found the match. */
			if (nmatch == -1)
			    lnum -= regmatch.startpos[0].lnum;
			break;
		    }
		}

		line_breakcheck();
	    }

	    if (did_sub)
		++sub_nlines;
	    vim_free(new_start);	/* for when substitute was cancelled */
	    VIM_CLEAR(sub_firstline);	/* free the copy of the original line */
	}

	line_breakcheck();
    }

    if (first_line != 0)
    {
	/* Need to subtract the number of added lines from "last_line" to get
	 * the line number before the change (same as adding the number of
	 * deleted lines). */
	i = curbuf->b_ml.ml_line_count - old_line_count;
	changed_lines(first_line, 0, last_line - i, i);
    }

outofmem:
    vim_free(sub_firstline); /* may have to free allocated copy of the line */

    /* ":s/pat//n" doesn't move the cursor */
    if (subflags.do_count)
	curwin->w_cursor = old_cursor;

    if (sub_nsubs > start_nsubs)
    {
	/* Set the '[ and '] marks. */
	curbuf->b_op_start.lnum = eap->line1;
	curbuf->b_op_end.lnum = line2;
	curbuf->b_op_start.col = curbuf->b_op_end.col = 0;

	if (!global_busy)
	{
	    /* when interactive leave cursor on the match */
	    if (!subflags.do_ask)
	    {
		if (endcolumn)
		    coladvance((colnr_T)MAXCOL);
		else
		    beginline(BL_WHITE | BL_FIX);
	    }
	    if (!do_sub_msg(subflags.do_count) && subflags.do_ask)
		msg("");
	}
	else
	    global_need_beginline = TRUE;
	if (subflags.do_print)
	    print_line(curwin->w_cursor.lnum,
					 subflags.do_number, subflags.do_list);
    }
    else if (!global_busy)
    {
	if (got_int)		/* interrupted */
	    emsg(_(e_interr));
	else if (got_match)	/* did find something but nothing substituted */
	    msg("");
	else if (subflags.do_error)	/* nothing found */
	    semsg(_(e_patnotf2), get_search_pat());
    }

#ifdef FEAT_FOLDING
    if (subflags.do_ask && hasAnyFolding(curwin))
	/* Cursor position may require updating */
	changed_window_setting();
#endif

    vim_regfree(regmatch.regprog);

    /* Restore the flag values, they can be used for ":&&". */
    subflags.do_all = save_do_all;
    subflags.do_ask = save_do_ask;
}

/*
 * Give message for number of substitutions.
 * Can also be used after a ":global" command.
 * Return TRUE if a message was given.
 */
    int
do_sub_msg(
    int	    count_only)		/* used 'n' flag for ":s" */
{
    /*
     * Only report substitutions when:
     * - more than 'report' substitutions
     * - command was typed by user, or number of changed lines > 'report'
     * - giving messages is not disabled by 'lazyredraw'
     */
    if (((sub_nsubs > p_report && (KeyTyped || sub_nlines > 1 || p_report < 1))
		|| count_only)
	    && messaging())
    {
	char	*msg_single;
	char	*msg_plural;

	if (got_int)
	    STRCPY(msg_buf, _("(Interrupted) "));
	else
	    *msg_buf = NUL;

	msg_single = count_only
		    ? NGETTEXT("%ld match on %ld line",
					  "%ld matches on %ld line", sub_nsubs)
		    : NGETTEXT("%ld substitution on %ld line",
				   "%ld substitutions on %ld line", sub_nsubs);
	msg_plural = count_only
		    ? NGETTEXT("%ld match on %ld lines",
					 "%ld matches on %ld lines", sub_nsubs)
		    : NGETTEXT("%ld substitution on %ld lines",
				  "%ld substitutions on %ld lines", sub_nsubs);

	vim_snprintf_add(msg_buf, sizeof(msg_buf),
				 NGETTEXT(msg_single, msg_plural, sub_nlines),
				 sub_nsubs, (long)sub_nlines);

	if (msg(msg_buf))
	    /* save message to display it after redraw */
	    set_keep_msg((char_u *)msg_buf, 0);
	return TRUE;
    }
    if (got_int)
    {
	emsg(_(e_interr));
	return TRUE;
    }
    return FALSE;
}

    static void
global_exe_one(char_u *cmd, linenr_T lnum)
{
    curwin->w_cursor.lnum = lnum;
    curwin->w_cursor.col = 0;
    if (*cmd == NUL || *cmd == '\n')
	do_cmdline((char_u *)"p", NULL, NULL, DOCMD_NOWAIT);
    else
	do_cmdline(cmd, NULL, NULL, DOCMD_NOWAIT);
}

/*
 * Execute a global command of the form:
 *
 * g/pattern/X : execute X on all lines where pattern matches
 * v/pattern/X : execute X on all lines where pattern does not match
 *
 * where 'X' is an EX command
 *
 * The command character (as well as the trailing slash) is optional, and
 * is assumed to be 'p' if missing.
 *
 * This is implemented in two passes: first we scan the file for the pattern and
 * set a mark for each line that (not) matches. Secondly we execute the command
 * for each line that has a mark. This is required because after deleting
 * lines we do not know where to search for the next match.
 */
    void
ex_global(exarg_T *eap)
{
    linenr_T	lnum;		/* line number according to old situation */
    int		ndone = 0;
    int		type;		/* first char of cmd: 'v' or 'g' */
    char_u	*cmd;		/* command argument */

    char_u	delim;		/* delimiter, normally '/' */
    char_u	*pat;
    regmmatch_T	regmatch;
    int		match;
    int		which_pat;

    /* When nesting the command works on one line.  This allows for
     * ":g/found/v/notfound/command". */
    if (global_busy && (eap->line1 != 1
				  || eap->line2 != curbuf->b_ml.ml_line_count))
    {
	/* will increment global_busy to break out of the loop */
	emsg(_("E147: Cannot do :global recursive with a range"));
	return;
    }

    if (eap->forceit)		    /* ":global!" is like ":vglobal" */
	type = 'v';
    else
	type = *eap->cmd;
    cmd = eap->arg;
    which_pat = RE_LAST;	    /* default: use last used regexp */

    /*
     * undocumented vi feature:
     *	"\/" and "\?": use previous search pattern.
     *		 "\&": use previous substitute pattern.
     */
    if (*cmd == '\\')
    {
	++cmd;
	if (vim_strchr((char_u *)"/?&", *cmd) == NULL)
	{
	    emsg(_(e_backslash));
	    return;
	}
	if (*cmd == '&')
	    which_pat = RE_SUBST;	/* use previous substitute pattern */
	else
	    which_pat = RE_SEARCH;	/* use previous search pattern */
	++cmd;
	pat = (char_u *)"";
    }
    else if (*cmd == NUL)
    {
	emsg(_("E148: Regular expression missing from global"));
	return;
    }
    else
    {
	delim = *cmd;		/* get the delimiter */
	if (delim)
	    ++cmd;		/* skip delimiter if there is one */
	pat = cmd;		/* remember start of pattern */
	cmd = skip_regexp(cmd, delim, p_magic, &eap->arg);
	if (cmd[0] == delim)		    /* end delimiter found */
	    *cmd++ = NUL;		    /* replace it with a NUL */
    }

    if (search_regcomp(pat, RE_BOTH, which_pat, SEARCH_HIS, &regmatch) == FAIL)
    {
	emsg(_(e_invcmd));
	return;
    }

    if (global_busy)
    {
	lnum = curwin->w_cursor.lnum;
	match = vim_regexec_multi(&regmatch, curwin, curbuf, lnum,
						       (colnr_T)0, NULL, NULL);
	if ((type == 'g' && match) || (type == 'v' && !match))
	    global_exe_one(cmd, lnum);
    }
    else
    {
	/*
	 * pass 1: set marks for each (not) matching line
	 */
	for (lnum = eap->line1; lnum <= eap->line2 && !got_int; ++lnum)
	{
	    /* a match on this line? */
	    match = vim_regexec_multi(&regmatch, curwin, curbuf, lnum,
						       (colnr_T)0, NULL, NULL);
	    if ((type == 'g' && match) || (type == 'v' && !match))
	    {
		ml_setmarked(lnum);
		ndone++;
	    }
	    line_breakcheck();
	}

	/*
	 * pass 2: execute the command for each line that has been marked
	 */
	if (got_int)
	    msg(_(e_interr));
	else if (ndone == 0)
	{
	    if (type == 'v')
		smsg(_("Pattern found in every line: %s"), pat);
	    else
		smsg(_("Pattern not found: %s"), pat);
	}
	else
	{
#ifdef FEAT_CLIPBOARD
	    start_global_changes();
#endif
	    global_exe(cmd);
#ifdef FEAT_CLIPBOARD
	    end_global_changes();
#endif
	}

	ml_clearmarked();	   /* clear rest of the marks */
    }

    vim_regfree(regmatch.regprog);
}

/*
 * Execute "cmd" on lines marked with ml_setmarked().
 */
    void
global_exe(char_u *cmd)
{
    linenr_T old_lcount;	/* b_ml.ml_line_count before the command */
    buf_T    *old_buf = curbuf;	/* remember what buffer we started in */
    linenr_T lnum;		/* line number according to old situation */

    /*
     * Set current position only once for a global command.
     * If global_busy is set, setpcmark() will not do anything.
     * If there is an error, global_busy will be incremented.
     */
    setpcmark();

    /* When the command writes a message, don't overwrite the command. */
    msg_didout = TRUE;

    sub_nsubs = 0;
    sub_nlines = 0;
    global_need_beginline = FALSE;
    global_busy = 1;
    old_lcount = curbuf->b_ml.ml_line_count;
    while (!got_int && (lnum = ml_firstmarked()) != 0 && global_busy == 1)
    {
	global_exe_one(cmd, lnum);
	ui_breakcheck();
    }

    global_busy = 0;
    if (global_need_beginline)
	beginline(BL_WHITE | BL_FIX);
    else
	check_cursor();	/* cursor may be beyond the end of the line */

    /* the cursor may not have moved in the text but a change in a previous
     * line may move it on the screen */
    changed_line_abv_curs();

    /* If it looks like no message was written, allow overwriting the
     * command with the report for number of changes. */
    if (msg_col == 0 && msg_scrolled == 0)
	msg_didout = FALSE;

    /* If substitutes done, report number of substitutes, otherwise report
     * number of extra or deleted lines.
     * Don't report extra or deleted lines in the edge case where the buffer
     * we are in after execution is different from the buffer we started in. */
    if (!do_sub_msg(FALSE) && curbuf == old_buf)
	msgmore(curbuf->b_ml.ml_line_count - old_lcount);
}

#ifdef FEAT_VIMINFO
    int
read_viminfo_sub_string(vir_T *virp, int force)
{
    if (force)
	vim_free(old_sub);
    if (force || old_sub == NULL)
	old_sub = viminfo_readstring(virp, 1, TRUE);
    return viminfo_readline(virp);
}

    void
write_viminfo_sub_string(FILE *fp)
{
    if (get_viminfo_parameter('/') != 0 && old_sub != NULL)
    {
	fputs(_("\n# Last Substitute String:\n$"), fp);
	viminfo_writestring(fp, old_sub);
    }
}
#endif /* FEAT_VIMINFO */

#if defined(EXITFREE) || defined(PROTO)
    void
free_old_sub(void)
{
    vim_free(old_sub);
}
#endif

#if defined(FEAT_QUICKFIX) || defined(PROTO)
/*
 * Set up for a tagpreview.
 * Return TRUE when it was created.
 */
    int
prepare_tagpreview(
    int		undo_sync)	/* sync undo when leaving the window */
{
    win_T	*wp;

# ifdef FEAT_GUI
    need_mouse_correct = TRUE;
# endif

    /*
     * If there is already a preview window open, use that one.
     */
    if (!curwin->w_p_pvw)
    {
	FOR_ALL_WINDOWS(wp)
	    if (wp->w_p_pvw)
		break;
	if (wp != NULL)
	    win_enter(wp, undo_sync);
	else
	{
	    /*
	     * There is no preview window open yet.  Create one.
	     */
	    if (win_split(g_do_tagpreview > 0 ? g_do_tagpreview : 0, 0)
								      == FAIL)
		return FALSE;
	    curwin->w_p_pvw = TRUE;
	    curwin->w_p_wfh = TRUE;
	    RESET_BINDING(curwin);	    /* don't take over 'scrollbind'
					       and 'cursorbind' */
# ifdef FEAT_DIFF
	    curwin->w_p_diff = FALSE;	    /* no 'diff' */
# endif
# ifdef FEAT_FOLDING
	    curwin->w_p_fdc = 0;	    /* no 'foldcolumn' */
# endif
	    return TRUE;
	}
    }
    return FALSE;
}

#endif


/*
 * ":help": open a read-only window on a help file
 */
    void
ex_help(exarg_T *eap)
{
    char_u	*arg;
    char_u	*tag;
    FILE	*helpfd;	/* file descriptor of help file */
    int		n;
    int		i;
    win_T	*wp;
    int		num_matches;
    char_u	**matches;
    char_u	*p;
    int		empty_fnum = 0;
    int		alt_fnum = 0;
    buf_T	*buf;
#ifdef FEAT_MULTI_LANG
    int		len;
    char_u	*lang;
#endif
#ifdef FEAT_FOLDING
    int		old_KeyTyped = KeyTyped;
#endif

    if (eap != NULL)
    {
	/*
	 * A ":help" command ends at the first LF, or at a '|' that is
	 * followed by some text.  Set nextcmd to the following command.
	 */
	for (arg = eap->arg; *arg; ++arg)
	{
	    if (*arg == '\n' || *arg == '\r'
		    || (*arg == '|' && arg[1] != NUL && arg[1] != '|'))
	    {
		*arg++ = NUL;
		eap->nextcmd = arg;
		break;
	    }
	}
	arg = eap->arg;

	if (eap->forceit && *arg == NUL && !curbuf->b_help)
	{
	    emsg(_("E478: Don't panic!"));
	    return;
	}

	if (eap->skip)	    /* not executing commands */
	    return;
    }
    else
	arg = (char_u *)"";

    /* remove trailing blanks */
    p = arg + STRLEN(arg) - 1;
    while (p > arg && VIM_ISWHITE(*p) && p[-1] != '\\')
	*p-- = NUL;

#ifdef FEAT_MULTI_LANG
    /* Check for a specified language */
    lang = check_help_lang(arg);
#endif

    /* When no argument given go to the index. */
    if (*arg == NUL)
	arg = (char_u *)"help.txt";

    /*
     * Check if there is a match for the argument.
     */
    n = find_help_tags(arg, &num_matches, &matches,
						 eap != NULL && eap->forceit);

    i = 0;
#ifdef FEAT_MULTI_LANG
    if (n != FAIL && lang != NULL)
	/* Find first item with the requested language. */
	for (i = 0; i < num_matches; ++i)
	{
	    len = (int)STRLEN(matches[i]);
	    if (len > 3 && matches[i][len - 3] == '@'
				  && STRICMP(matches[i] + len - 2, lang) == 0)
		break;
	}
#endif
    if (i >= num_matches || n == FAIL)
    {
#ifdef FEAT_MULTI_LANG
	if (lang != NULL)
	    semsg(_("E661: Sorry, no '%s' help for %s"), lang, arg);
	else
#endif
	    semsg(_("E149: Sorry, no help for %s"), arg);
	if (n != FAIL)
	    FreeWild(num_matches, matches);
	return;
    }

    /* The first match (in the requested language) is the best match. */
    tag = vim_strsave(matches[i]);
    FreeWild(num_matches, matches);

#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

    /*
     * Re-use an existing help window or open a new one.
     * Always open a new one for ":tab help".
     */
    if (!bt_help(curwin->w_buffer) || cmdmod.tab != 0)
    {
	if (cmdmod.tab != 0)
	    wp = NULL;
	else
	    FOR_ALL_WINDOWS(wp)
		if (bt_help(wp->w_buffer))
		    break;
	if (wp != NULL && wp->w_buffer->b_nwindows > 0)
	    win_enter(wp, TRUE);
	else
	{
	    /*
	     * There is no help window yet.
	     * Try to open the file specified by the "helpfile" option.
	     */
	    if ((helpfd = mch_fopen((char *)p_hf, READBIN)) == NULL)
	    {
		smsg(_("Sorry, help file \"%s\" not found"), p_hf);
		goto erret;
	    }
	    fclose(helpfd);

	    /* Split off help window; put it at far top if no position
	     * specified, the current window is vertically split and
	     * narrow. */
	    n = WSP_HELP;
	    if (cmdmod.split == 0 && curwin->w_width != Columns
						  && curwin->w_width < 80)
		n |= WSP_TOP;
	    if (win_split(0, n) == FAIL)
		goto erret;

	    if (curwin->w_height < p_hh)
		win_setheight((int)p_hh);

	    /*
	     * Open help file (do_ecmd() will set b_help flag, readfile() will
	     * set b_p_ro flag).
	     * Set the alternate file to the previously edited file.
	     */
	    alt_fnum = curbuf->b_fnum;
	    (void)do_ecmd(0, NULL, NULL, NULL, ECMD_LASTL,
			  ECMD_HIDE + ECMD_SET_HELP,
			  NULL);  /* buffer is still open, don't store info */
	    if (!cmdmod.keepalt)
		curwin->w_alt_fnum = alt_fnum;
	    empty_fnum = curbuf->b_fnum;
	}
    }

    if (!p_im)
	restart_edit = 0;	    /* don't want insert mode in help file */

#ifdef FEAT_FOLDING
    /* Restore KeyTyped, setting 'filetype=help' may reset it.
     * It is needed for do_tag top open folds under the cursor. */
    KeyTyped = old_KeyTyped;
#endif

    if (tag != NULL)
	do_tag(tag, DT_HELP, 1, FALSE, TRUE);

    /* Delete the empty buffer if we're not using it.  Careful: autocommands
     * may have jumped to another window, check that the buffer is not in a
     * window. */
    if (empty_fnum != 0 && curbuf->b_fnum != empty_fnum)
    {
	buf = buflist_findnr(empty_fnum);
	if (buf != NULL && buf->b_nwindows == 0)
	    wipe_buffer(buf, TRUE);
    }

    /* keep the previous alternate file */
    if (alt_fnum != 0 && curwin->w_alt_fnum == empty_fnum && !cmdmod.keepalt)
	curwin->w_alt_fnum = alt_fnum;

erret:
    vim_free(tag);
}

/*
 * ":helpclose": Close one help window
 */
    void
ex_helpclose(exarg_T *eap UNUSED)
{
    win_T *win;

    FOR_ALL_WINDOWS(win)
    {
	if (bt_help(win->w_buffer))
	{
	    win_close(win, FALSE);
	    return;
	}
    }
}

#if defined(FEAT_MULTI_LANG) || defined(PROTO)
/*
 * In an argument search for a language specifiers in the form "@xx".
 * Changes the "@" to NUL if found, and returns a pointer to "xx".
 * Returns NULL if not found.
 */
    char_u *
check_help_lang(char_u *arg)
{
    int len = (int)STRLEN(arg);

    if (len >= 3 && arg[len - 3] == '@' && ASCII_ISALPHA(arg[len - 2])
					       && ASCII_ISALPHA(arg[len - 1]))
    {
	arg[len - 3] = NUL;		/* remove the '@' */
	return arg + len - 2;
    }
    return NULL;
}
#endif

/*
 * Return a heuristic indicating how well the given string matches.  The
 * smaller the number, the better the match.  This is the order of priorities,
 * from best match to worst match:
 *	- Match with least alpha-numeric characters is better.
 *	- Match with least total characters is better.
 *	- Match towards the start is better.
 *	- Match starting with "+" is worse (feature instead of command)
 * Assumption is made that the matched_string passed has already been found to
 * match some string for which help is requested.  webb.
 */
    int
help_heuristic(
    char_u	*matched_string,
    int		offset,			/* offset for match */
    int		wrong_case)		/* no matching case */
{
    int		num_letters;
    char_u	*p;

    num_letters = 0;
    for (p = matched_string; *p; p++)
	if (ASCII_ISALNUM(*p))
	    num_letters++;

    /*
     * Multiply the number of letters by 100 to give it a much bigger
     * weighting than the number of characters.
     * If there only is a match while ignoring case, add 5000.
     * If the match starts in the middle of a word, add 10000 to put it
     * somewhere in the last half.
     * If the match is more than 2 chars from the start, multiply by 200 to
     * put it after matches at the start.
     */
    if (ASCII_ISALNUM(matched_string[offset]) && offset > 0
				 && ASCII_ISALNUM(matched_string[offset - 1]))
	offset += 10000;
    else if (offset > 2)
	offset *= 200;
    if (wrong_case)
	offset += 5000;
    /* Features are less interesting than the subjects themselves, but "+"
     * alone is not a feature. */
    if (matched_string[0] == '+' && matched_string[1] != NUL)
	offset += 100;
    return (int)(100 * num_letters + STRLEN(matched_string) + offset);
}

/*
 * Compare functions for qsort() below, that checks the help heuristics number
 * that has been put after the tagname by find_tags().
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
help_compare(const void *s1, const void *s2)
{
    char    *p1;
    char    *p2;

    p1 = *(char **)s1 + strlen(*(char **)s1) + 1;
    p2 = *(char **)s2 + strlen(*(char **)s2) + 1;
    return strcmp(p1, p2);
}

/*
 * Find all help tags matching "arg", sort them and return in matches[], with
 * the number of matches in num_matches.
 * The matches will be sorted with a "best" match algorithm.
 * When "keep_lang" is TRUE try keeping the language of the current buffer.
 */
    int
find_help_tags(
    char_u	*arg,
    int		*num_matches,
    char_u	***matches,
    int		keep_lang)
{
    char_u	*s, *d;
    int		i;
    static char *(mtable[]) = {"*", "g*", "[*", "]*", ":*",
			       "/*", "/\\*", "\"*", "**",
			       "cpo-*", "/\\(\\)", "/\\%(\\)",
			       "?", ":?", "?<CR>", "g?", "g?g?", "g??",
			       "-?", "q?", "v_g?",
			       "/\\?", "/\\z(\\)", "\\=", ":s\\=",
			       "[count]", "[quotex]",
			       "[range]", ":[range]",
			       "[pattern]", "\\|", "\\%$",
			       "s/\\~", "s/\\U", "s/\\L",
			       "s/\\1", "s/\\2", "s/\\3", "s/\\9"};
    static char *(rtable[]) = {"star", "gstar", "[star", "]star", ":star",
			       "/star", "/\\\\star", "quotestar", "starstar",
			       "cpo-star", "/\\\\(\\\\)", "/\\\\%(\\\\)",
			       "?", ":?", "?<CR>", "g?", "g?g?", "g??",
			       "-?", "q?", "v_g?",
			       "/\\\\?", "/\\\\z(\\\\)", "\\\\=", ":s\\\\=",
			       "\\[count]", "\\[quotex]",
			       "\\[range]", ":\\[range]",
			       "\\[pattern]", "\\\\bar", "/\\\\%\\$",
			       "s/\\\\\\~", "s/\\\\U", "s/\\\\L",
			       "s/\\\\1", "s/\\\\2", "s/\\\\3", "s/\\\\9"};
    static char *(expr_table[]) = {"!=?", "!~?", "<=?", "<?", "==?", "=~?",
				">=?", ">?", "is?", "isnot?"};
    int flags;

    d = IObuff;		    /* assume IObuff is long enough! */

    if (STRNICMP(arg, "expr-", 5) == 0)
    {
	// When the string starting with "expr-" and containing '?' and matches
	// the table, it is taken literally.  Otherwise '?' is recognized as a
	// wildcard.
	for (i = (int)(sizeof(expr_table) / sizeof(char *)); --i >= 0; )
	    if (STRCMP(arg + 5, expr_table[i]) == 0)
	    {
		STRCPY(d, arg);
		break;
	    }
    }
    else
    {
	// Recognize a few exceptions to the rule.  Some strings that contain
	// '*' with "star".  Otherwise '*' is recognized as a wildcard.
	for (i = (int)(sizeof(mtable) / sizeof(char *)); --i >= 0; )
	    if (STRCMP(arg, mtable[i]) == 0)
	    {
		STRCPY(d, rtable[i]);
		break;
	    }
    }

    if (i < 0)	/* no match in table */
    {
	/* Replace "\S" with "/\\S", etc.  Otherwise every tag is matched.
	 * Also replace "\%^" and "\%(", they match every tag too.
	 * Also "\zs", "\z1", etc.
	 * Also "\@<", "\@=", "\@<=", etc.
	 * And also "\_$" and "\_^". */
	if (arg[0] == '\\'
		&& ((arg[1] != NUL && arg[2] == NUL)
		    || (vim_strchr((char_u *)"%_z@", arg[1]) != NULL
							   && arg[2] != NUL)))
	{
	    STRCPY(d, "/\\\\");
	    STRCPY(d + 3, arg + 1);
	    /* Check for "/\\_$", should be "/\\_\$" */
	    if (d[3] == '_' && d[4] == '$')
		STRCPY(d + 4, "\\$");
	}
	else
	{
	  /* Replace:
	   * "[:...:]" with "\[:...:]"
	   * "[++...]" with "\[++...]"
	   * "\{" with "\\{"		   -- matching "} \}"
	   */
	    if ((arg[0] == '[' && (arg[1] == ':'
			 || (arg[1] == '+' && arg[2] == '+')))
		    || (arg[0] == '\\' && arg[1] == '{'))
	      *d++ = '\\';

	  /*
	   * If tag starts with "('", skip the "(". Fixes CTRL-] on ('option'.
	   */
	  if (*arg == '(' && arg[1] == '\'')
	      arg++;
	  for (s = arg; *s; ++s)
	  {
	    /*
	     * Replace "|" with "bar" and '"' with "quote" to match the name of
	     * the tags for these commands.
	     * Replace "*" with ".*" and "?" with "." to match command line
	     * completion.
	     * Insert a backslash before '~', '$' and '.' to avoid their
	     * special meaning.
	     */
	    if (d - IObuff > IOSIZE - 10)	/* getting too long!? */
		break;
	    switch (*s)
	    {
		case '|':   STRCPY(d, "bar");
			    d += 3;
			    continue;
		case '"':   STRCPY(d, "quote");
			    d += 5;
			    continue;
		case '*':   *d++ = '.';
			    break;
		case '?':   *d++ = '.';
			    continue;
		case '$':
		case '.':
		case '~':   *d++ = '\\';
			    break;
	    }

	    /*
	     * Replace "^x" by "CTRL-X". Don't do this for "^_" to make
	     * ":help i_^_CTRL-D" work.
	     * Insert '-' before and after "CTRL-X" when applicable.
	     */
	    if (*s < ' ' || (*s == '^' && s[1] && (ASCII_ISALPHA(s[1])
			   || vim_strchr((char_u *)"?@[\\]^", s[1]) != NULL)))
	    {
		if (d > IObuff && d[-1] != '_' && d[-1] != '\\')
		    *d++ = '_';		/* prepend a '_' to make x_CTRL-x */
		STRCPY(d, "CTRL-");
		d += 5;
		if (*s < ' ')
		{
#ifdef EBCDIC
		    *d++ = CtrlChar(*s);
#else
		    *d++ = *s + '@';
#endif
		    if (d[-1] == '\\')
			*d++ = '\\';	/* double a backslash */
		}
		else
		    *d++ = *++s;
		if (s[1] != NUL && s[1] != '_')
		    *d++ = '_';		/* append a '_' */
		continue;
	    }
	    else if (*s == '^')		/* "^" or "CTRL-^" or "^_" */
		*d++ = '\\';

	    /*
	     * Insert a backslash before a backslash after a slash, for search
	     * pattern tags: "/\|" --> "/\\|".
	     */
	    else if (s[0] == '\\' && s[1] != '\\'
					       && *arg == '/' && s == arg + 1)
		*d++ = '\\';

	    /* "CTRL-\_" -> "CTRL-\\_" to avoid the special meaning of "\_" in
	     * "CTRL-\_CTRL-N" */
	    if (STRNICMP(s, "CTRL-\\_", 7) == 0)
	    {
		STRCPY(d, "CTRL-\\\\");
		d += 7;
		s += 6;
	    }

	    *d++ = *s;

	    /*
	     * If tag contains "({" or "([", tag terminates at the "(".
	     * This is for help on functions, e.g.: abs({expr}).
	     */
	    if (*s == '(' && (s[1] == '{' || s[1] =='['))
		break;

	    /*
	     * If tag starts with ', toss everything after a second '. Fixes
	     * CTRL-] on 'option'. (would include the trailing '.').
	     */
	    if (*s == '\'' && s > arg && *arg == '\'')
		break;
	    /* Also '{' and '}'. */
	    if (*s == '}' && s > arg && *arg == '{')
		break;
	  }
	  *d = NUL;

	  if (*IObuff == '`')
	  {
	      if (d > IObuff + 2 && d[-1] == '`')
	      {
		  /* remove the backticks from `command` */
		  mch_memmove(IObuff, IObuff + 1, STRLEN(IObuff));
		  d[-2] = NUL;
	      }
	      else if (d > IObuff + 3 && d[-2] == '`' && d[-1] == ',')
	      {
		  /* remove the backticks and comma from `command`, */
		  mch_memmove(IObuff, IObuff + 1, STRLEN(IObuff));
		  d[-3] = NUL;
	      }
	      else if (d > IObuff + 4 && d[-3] == '`'
					     && d[-2] == '\\' && d[-1] == '.')
	      {
		  /* remove the backticks and dot from `command`\. */
		  mch_memmove(IObuff, IObuff + 1, STRLEN(IObuff));
		  d[-4] = NUL;
	      }
	  }
	}
    }

    *matches = (char_u **)"";
    *num_matches = 0;
    flags = TAG_HELP | TAG_REGEXP | TAG_NAMES | TAG_VERBOSE;
    if (keep_lang)
	flags |= TAG_KEEP_LANG;
    if (find_tags(IObuff, num_matches, matches, flags, (int)MAXCOL, NULL) == OK
	    && *num_matches > 0)
    {
	/* Sort the matches found on the heuristic number that is after the
	 * tag name. */
	qsort((void *)*matches, (size_t)*num_matches,
					      sizeof(char_u *), help_compare);
	/* Delete more than TAG_MANY to reduce the size of the listing. */
	while (*num_matches > TAG_MANY)
	    vim_free((*matches)[--*num_matches]);
    }
    return OK;
}

/*
 * Called when starting to edit a buffer for a help file.
 */
    static void
prepare_help_buffer(void)
{
    char_u	*p;

    curbuf->b_help = TRUE;
#ifdef FEAT_QUICKFIX
    set_string_option_direct((char_u *)"buftype", -1,
				     (char_u *)"help", OPT_FREE|OPT_LOCAL, 0);
#endif

    /*
     * Always set these options after jumping to a help tag, because the
     * user may have an autocommand that gets in the way.
     * Accept all ASCII chars for keywords, except ' ', '*', '"', '|', and
     * latin1 word characters (for translated help files).
     * Only set it when needed, buf_init_chartab() is some work.
     */
    p =
#ifdef EBCDIC
	    (char_u *)"65-255,^*,^|,^\"";
#else
	    (char_u *)"!-~,^*,^|,^\",192-255";
#endif
    if (STRCMP(curbuf->b_p_isk, p) != 0)
    {
	set_string_option_direct((char_u *)"isk", -1, p, OPT_FREE|OPT_LOCAL, 0);
	check_buf_options(curbuf);
	(void)buf_init_chartab(curbuf, FALSE);
    }

#ifdef FEAT_FOLDING
    /* Don't use the global foldmethod.*/
    set_string_option_direct((char_u *)"fdm", -1, (char_u *)"manual",
						       OPT_FREE|OPT_LOCAL, 0);
#endif

    curbuf->b_p_ts = 8;		/* 'tabstop' is 8 */
    curwin->w_p_list = FALSE;	/* no list mode */

    curbuf->b_p_ma = FALSE;		/* not modifiable */
    curbuf->b_p_bin = FALSE;	/* reset 'bin' before reading file */
    curwin->w_p_nu = 0;		/* no line numbers */
    curwin->w_p_rnu = 0;		/* no relative line numbers */
    RESET_BINDING(curwin);		/* no scroll or cursor binding */
#ifdef FEAT_ARABIC
    curwin->w_p_arab = FALSE;	/* no arabic mode */
#endif
#ifdef FEAT_RIGHTLEFT
    curwin->w_p_rl  = FALSE;	/* help window is left-to-right */
#endif
#ifdef FEAT_FOLDING
    curwin->w_p_fen = FALSE;	/* No folding in the help window */
#endif
#ifdef FEAT_DIFF
    curwin->w_p_diff = FALSE;	/* No 'diff' */
#endif
#ifdef FEAT_SPELL
    curwin->w_p_spell = FALSE;	/* No spell checking */
#endif

    set_buflisted(FALSE);
}

/*
 * After reading a help file: May cleanup a help buffer when syntax
 * highlighting is not used.
 */
    void
fix_help_buffer(void)
{
    linenr_T	lnum;
    char_u	*line;
    int		in_example = FALSE;
    int		len;
    char_u	*fname;
    char_u	*p;
    char_u	*rt;
    int		mustfree;

    /* Set filetype to "help" if still needed. */
    if (STRCMP(curbuf->b_p_ft, "help") != 0)
    {
	++curbuf_lock;
	set_option_value((char_u *)"ft", 0L, (char_u *)"help", OPT_LOCAL);
	--curbuf_lock;
    }

#ifdef FEAT_SYN_HL
    if (!syntax_present(curwin))
#endif
    {
	for (lnum = 1; lnum <= curbuf->b_ml.ml_line_count; ++lnum)
	{
	    line = ml_get_buf(curbuf, lnum, FALSE);
	    len = (int)STRLEN(line);
	    if (in_example && len > 0 && !VIM_ISWHITE(line[0]))
	    {
		/* End of example: non-white or '<' in first column. */
		if (line[0] == '<')
		{
		    /* blank-out a '<' in the first column */
		    line = ml_get_buf(curbuf, lnum, TRUE);
		    line[0] = ' ';
		}
		in_example = FALSE;
	    }
	    if (!in_example && len > 0)
	    {
		if (line[len - 1] == '>' && (len == 1 || line[len - 2] == ' '))
		{
		    /* blank-out a '>' in the last column (start of example) */
		    line = ml_get_buf(curbuf, lnum, TRUE);
		    line[len - 1] = ' ';
		    in_example = TRUE;
		}
		else if (line[len - 1] == '~')
		{
		    /* blank-out a '~' at the end of line (header marker) */
		    line = ml_get_buf(curbuf, lnum, TRUE);
		    line[len - 1] = ' ';
		}
	    }
	}
    }

    /*
     * In the "help.txt" and "help.abx" file, add the locally added help
     * files.  This uses the very first line in the help file.
     */
    fname = gettail(curbuf->b_fname);
    if (fnamecmp(fname, "help.txt") == 0
#ifdef FEAT_MULTI_LANG
	|| (fnamencmp(fname, "help.", 5) == 0
	    && ASCII_ISALPHA(fname[5])
	    && ASCII_ISALPHA(fname[6])
	    && TOLOWER_ASC(fname[7]) == 'x'
	    && fname[8] == NUL)
#endif
	)
    {
	for (lnum = 1; lnum < curbuf->b_ml.ml_line_count; ++lnum)
	{
	    line = ml_get_buf(curbuf, lnum, FALSE);
	    if (strstr((char *)line, "*local-additions*") == NULL)
		continue;

	    /* Go through all directories in 'runtimepath', skipping
	     * $VIMRUNTIME. */
	    p = p_rtp;
	    while (*p != NUL)
	    {
		copy_option_part(&p, NameBuff, MAXPATHL, ",");
		mustfree = FALSE;
		rt = vim_getenv((char_u *)"VIMRUNTIME", &mustfree);
		if (rt != NULL && fullpathcmp(rt, NameBuff, FALSE) != FPC_SAME)
		{
		    int		fcount;
		    char_u	**fnames;
		    FILE	*fd;
		    char_u	*s;
		    int		fi;
		    vimconv_T	vc;
		    char_u	*cp;

		    /* Find all "doc/ *.txt" files in this directory. */
		    add_pathsep(NameBuff);
#ifdef FEAT_MULTI_LANG
		    STRCAT(NameBuff, "doc/*.??[tx]");
#else
		    STRCAT(NameBuff, "doc/*.txt");
#endif
		    if (gen_expand_wildcards(1, &NameBuff, &fcount,
					 &fnames, EW_FILE|EW_SILENT) == OK
			    && fcount > 0)
		    {
#ifdef FEAT_MULTI_LANG
			int	i1, i2;
			char_u	*f1, *f2;
			char_u	*t1, *t2;
			char_u	*e1, *e2;

			/* If foo.abx is found use it instead of foo.txt in
			 * the same directory. */
			for (i1 = 0; i1 < fcount; ++i1)
			{
			    for (i2 = 0; i2 < fcount; ++i2)
			    {
				if (i1 == i2)
				    continue;
				if (fnames[i1] == NULL || fnames[i2] == NULL)
				    continue;
				f1 = fnames[i1];
				f2 = fnames[i2];
				t1 = gettail(f1);
				t2 = gettail(f2);
				e1 = vim_strrchr(t1, '.');
				e2 = vim_strrchr(t2, '.');
				if (e1 == NULL || e2 == NULL)
				    continue;
				if (fnamecmp(e1, ".txt") != 0
				    && fnamecmp(e1, fname + 4) != 0)
				{
				    /* Not .txt and not .abx, remove it. */
				    VIM_CLEAR(fnames[i1]);
				    continue;
				}
				if (e1 - f1 != e2 - f2
					    || fnamencmp(f1, f2, e1 - f1) != 0)
				    continue;
				if (fnamecmp(e1, ".txt") == 0
				    && fnamecmp(e2, fname + 4) == 0)
				    /* use .abx instead of .txt */
				    VIM_CLEAR(fnames[i1]);
			    }
			}
#endif
			for (fi = 0; fi < fcount; ++fi)
			{
			    if (fnames[fi] == NULL)
				continue;
			    fd = mch_fopen((char *)fnames[fi], "r");
			    if (fd != NULL)
			    {
				vim_fgets(IObuff, IOSIZE, fd);
				if (IObuff[0] == '*'
					&& (s = vim_strchr(IObuff + 1, '*'))
								  != NULL)
				{
				    int	this_utf = MAYBE;

				    /* Change tag definition to a
				     * reference and remove <CR>/<NL>. */
				    IObuff[0] = '|';
				    *s = '|';
				    while (*s != NUL)
				    {
					if (*s == '\r' || *s == '\n')
					    *s = NUL;
					/* The text is utf-8 when a byte
					 * above 127 is found and no
					 * illegal byte sequence is found.
					 */
					if (*s >= 0x80 && this_utf != FALSE)
					{
					    int	l;

					    this_utf = TRUE;
					    l = utf_ptr2len(s);
					    if (l == 1)
						this_utf = FALSE;
					    s += l - 1;
					}
					++s;
				    }

				    /* The help file is latin1 or utf-8;
				     * conversion to the current
				     * 'encoding' may be required. */
				    vc.vc_type = CONV_NONE;
				    convert_setup(&vc, (char_u *)(
						this_utf == TRUE ? "utf-8"
						      : "latin1"), p_enc);
				    if (vc.vc_type == CONV_NONE)
					/* No conversion needed. */
					cp = IObuff;
				    else
				    {
					/* Do the conversion.  If it fails
					 * use the unconverted text. */
					cp = string_convert(&vc, IObuff,
								    NULL);
					if (cp == NULL)
					    cp = IObuff;
				    }
				    convert_setup(&vc, NULL, NULL);

				    ml_append(lnum, cp, (colnr_T)0, FALSE);
				    if (cp != IObuff)
					vim_free(cp);
				    ++lnum;
				}
				fclose(fd);
			    }
			}
			FreeWild(fcount, fnames);
		    }
		}
		if (mustfree)
		    vim_free(rt);
	    }
	    break;
	}
    }
}

/*
 * ":exusage"
 */
    void
ex_exusage(exarg_T *eap UNUSED)
{
    do_cmdline_cmd((char_u *)"help ex-cmd-index");
}

/*
 * ":viusage"
 */
    void
ex_viusage(exarg_T *eap UNUSED)
{
    do_cmdline_cmd((char_u *)"help normal-index");
}

/*
 * Generate tags in one help directory.
 */
    static void
helptags_one(
    char_u	*dir,		/* doc directory */
    char_u	*ext,		/* suffix, ".txt", ".itx", ".frx", etc. */
    char_u	*tagfname,	/* "tags" for English, "tags-fr" for French. */
    int		add_help_tags)	/* add "help-tags" tag */
{
    FILE	*fd_tags;
    FILE	*fd;
    garray_T	ga;
    int		filecount;
    char_u	**files;
    char_u	*p1, *p2;
    int		fi;
    char_u	*s;
    int		i;
    char_u	*fname;
    int		dirlen;
    int		utf8 = MAYBE;
    int		this_utf8;
    int		firstline;
    int		mix = FALSE;	/* detected mixed encodings */

    /*
     * Find all *.txt files.
     */
    dirlen = (int)STRLEN(dir);
    STRCPY(NameBuff, dir);
    STRCAT(NameBuff, "/**/*");
    STRCAT(NameBuff, ext);
    if (gen_expand_wildcards(1, &NameBuff, &filecount, &files,
						    EW_FILE|EW_SILENT) == FAIL
	    || filecount == 0)
    {
	if (!got_int)
	    semsg(_("E151: No match: %s"), NameBuff);
	return;
    }

    /*
     * Open the tags file for writing.
     * Do this before scanning through all the files.
     */
    STRCPY(NameBuff, dir);
    add_pathsep(NameBuff);
    STRCAT(NameBuff, tagfname);
    fd_tags = mch_fopen((char *)NameBuff, "w");
    if (fd_tags == NULL)
    {
	semsg(_("E152: Cannot open %s for writing"), NameBuff);
	FreeWild(filecount, files);
	return;
    }

    /*
     * If using the "++t" argument or generating tags for "$VIMRUNTIME/doc"
     * add the "help-tags" tag.
     */
    ga_init2(&ga, (int)sizeof(char_u *), 100);
    if (add_help_tags || fullpathcmp((char_u *)"$VIMRUNTIME/doc",
						      dir, FALSE) == FPC_SAME)
    {
	if (ga_grow(&ga, 1) == FAIL)
	    got_int = TRUE;
	else
	{
	    s = alloc(18 + (unsigned)STRLEN(tagfname));
	    if (s == NULL)
		got_int = TRUE;
	    else
	    {
		sprintf((char *)s, "help-tags\t%s\t1\n", tagfname);
		((char_u **)ga.ga_data)[ga.ga_len] = s;
		++ga.ga_len;
	    }
	}
    }

    /*
     * Go over all the files and extract the tags.
     */
    for (fi = 0; fi < filecount && !got_int; ++fi)
    {
	fd = mch_fopen((char *)files[fi], "r");
	if (fd == NULL)
	{
	    semsg(_("E153: Unable to open %s for reading"), files[fi]);
	    continue;
	}
	fname = files[fi] + dirlen + 1;

	firstline = TRUE;
	while (!vim_fgets(IObuff, IOSIZE, fd) && !got_int)
	{
	    if (firstline)
	    {
		/* Detect utf-8 file by a non-ASCII char in the first line. */
		this_utf8 = MAYBE;
		for (s = IObuff; *s != NUL; ++s)
		    if (*s >= 0x80)
		    {
			int l;

			this_utf8 = TRUE;
			l = utf_ptr2len(s);
			if (l == 1)
			{
			    /* Illegal UTF-8 byte sequence. */
			    this_utf8 = FALSE;
			    break;
			}
			s += l - 1;
		    }
		if (this_utf8 == MAYBE)	    /* only ASCII characters found */
		    this_utf8 = FALSE;
		if (utf8 == MAYBE)	    /* first file */
		    utf8 = this_utf8;
		else if (utf8 != this_utf8)
		{
		    semsg(_("E670: Mix of help file encodings within a language: %s"), files[fi]);
		    mix = !got_int;
		    got_int = TRUE;
		}
		firstline = FALSE;
	    }
	    p1 = vim_strchr(IObuff, '*');	/* find first '*' */
	    while (p1 != NULL)
	    {
		/* Use vim_strbyte() instead of vim_strchr() so that when
		 * 'encoding' is dbcs it still works, don't find '*' in the
		 * second byte. */
		p2 = vim_strbyte(p1 + 1, '*');	/* find second '*' */
		if (p2 != NULL && p2 > p1 + 1)	/* skip "*" and "**" */
		{
		    for (s = p1 + 1; s < p2; ++s)
			if (*s == ' ' || *s == '\t' || *s == '|')
			    break;

		    /*
		     * Only accept a *tag* when it consists of valid
		     * characters, there is white space before it and is
		     * followed by a white character or end-of-line.
		     */
		    if (s == p2
			    && (p1 == IObuff || p1[-1] == ' ' || p1[-1] == '\t')
			    && (vim_strchr((char_u *)" \t\n\r", s[1]) != NULL
				|| s[1] == '\0'))
		    {
			*p2 = '\0';
			++p1;
			if (ga_grow(&ga, 1) == FAIL)
			{
			    got_int = TRUE;
			    break;
			}
			s = alloc((unsigned)(p2 - p1 + STRLEN(fname) + 2));
			if (s == NULL)
			{
			    got_int = TRUE;
			    break;
			}
			((char_u **)ga.ga_data)[ga.ga_len] = s;
			++ga.ga_len;
			sprintf((char *)s, "%s\t%s", p1, fname);

			/* find next '*' */
			p2 = vim_strchr(p2 + 1, '*');
		    }
		}
		p1 = p2;
	    }
	    line_breakcheck();
	}

	fclose(fd);
    }

    FreeWild(filecount, files);

    if (!got_int)
    {
	/*
	 * Sort the tags.
	 */
	if (ga.ga_data != NULL)
	    sort_strings((char_u **)ga.ga_data, ga.ga_len);

	/*
	 * Check for duplicates.
	 */
	for (i = 1; i < ga.ga_len; ++i)
	{
	    p1 = ((char_u **)ga.ga_data)[i - 1];
	    p2 = ((char_u **)ga.ga_data)[i];
	    while (*p1 == *p2)
	    {
		if (*p2 == '\t')
		{
		    *p2 = NUL;
		    vim_snprintf((char *)NameBuff, MAXPATHL,
			    _("E154: Duplicate tag \"%s\" in file %s/%s"),
				     ((char_u **)ga.ga_data)[i], dir, p2 + 1);
		    emsg((char *)NameBuff);
		    *p2 = '\t';
		    break;
		}
		++p1;
		++p2;
	    }
	}

	if (utf8 == TRUE)
	    fprintf(fd_tags, "!_TAG_FILE_ENCODING\tutf-8\t//\n");

	/*
	 * Write the tags into the file.
	 */
	for (i = 0; i < ga.ga_len; ++i)
	{
	    s = ((char_u **)ga.ga_data)[i];
	    if (STRNCMP(s, "help-tags\t", 10) == 0)
		/* help-tags entry was added in formatted form */
		fputs((char *)s, fd_tags);
	    else
	    {
		fprintf(fd_tags, "%s\t/*", s);
		for (p1 = s; *p1 != '\t'; ++p1)
		{
		    /* insert backslash before '\\' and '/' */
		    if (*p1 == '\\' || *p1 == '/')
			putc('\\', fd_tags);
		    putc(*p1, fd_tags);
		}
		fprintf(fd_tags, "*\n");
	    }
	}
    }
    if (mix)
	got_int = FALSE;    /* continue with other languages */

    for (i = 0; i < ga.ga_len; ++i)
	vim_free(((char_u **)ga.ga_data)[i]);
    ga_clear(&ga);
    fclose(fd_tags);	    /* there is no check for an error... */
}

/*
 * Generate tags in one help directory, taking care of translations.
 */
    static void
do_helptags(char_u *dirname, int add_help_tags)
{
#ifdef FEAT_MULTI_LANG
    int		len;
    int		i, j;
    garray_T	ga;
    char_u	lang[2];
    char_u	ext[5];
    char_u	fname[8];
    int		filecount;
    char_u	**files;

    /* Get a list of all files in the help directory and in subdirectories. */
    STRCPY(NameBuff, dirname);
    add_pathsep(NameBuff);
    STRCAT(NameBuff, "**");
    if (gen_expand_wildcards(1, &NameBuff, &filecount, &files,
						    EW_FILE|EW_SILENT) == FAIL
	    || filecount == 0)
    {
	semsg(_("E151: No match: %s"), NameBuff);
	return;
    }

    /* Go over all files in the directory to find out what languages are
     * present. */
    ga_init2(&ga, 1, 10);
    for (i = 0; i < filecount; ++i)
    {
	len = (int)STRLEN(files[i]);
	if (len > 4)
	{
	    if (STRICMP(files[i] + len - 4, ".txt") == 0)
	    {
		/* ".txt" -> language "en" */
		lang[0] = 'e';
		lang[1] = 'n';
	    }
	    else if (files[i][len - 4] == '.'
		    && ASCII_ISALPHA(files[i][len - 3])
		    && ASCII_ISALPHA(files[i][len - 2])
		    && TOLOWER_ASC(files[i][len - 1]) == 'x')
	    {
		/* ".abx" -> language "ab" */
		lang[0] = TOLOWER_ASC(files[i][len - 3]);
		lang[1] = TOLOWER_ASC(files[i][len - 2]);
	    }
	    else
		continue;

	    /* Did we find this language already? */
	    for (j = 0; j < ga.ga_len; j += 2)
		if (STRNCMP(lang, ((char_u *)ga.ga_data) + j, 2) == 0)
		    break;
	    if (j == ga.ga_len)
	    {
		/* New language, add it. */
		if (ga_grow(&ga, 2) == FAIL)
		    break;
		((char_u *)ga.ga_data)[ga.ga_len++] = lang[0];
		((char_u *)ga.ga_data)[ga.ga_len++] = lang[1];
	    }
	}
    }

    /*
     * Loop over the found languages to generate a tags file for each one.
     */
    for (j = 0; j < ga.ga_len; j += 2)
    {
	STRCPY(fname, "tags-xx");
	fname[5] = ((char_u *)ga.ga_data)[j];
	fname[6] = ((char_u *)ga.ga_data)[j + 1];
	if (fname[5] == 'e' && fname[6] == 'n')
	{
	    /* English is an exception: use ".txt" and "tags". */
	    fname[4] = NUL;
	    STRCPY(ext, ".txt");
	}
	else
	{
	    /* Language "ab" uses ".abx" and "tags-ab". */
	    STRCPY(ext, ".xxx");
	    ext[1] = fname[5];
	    ext[2] = fname[6];
	}
	helptags_one(dirname, ext, fname, add_help_tags);
    }

    ga_clear(&ga);
    FreeWild(filecount, files);

#else
    /* No language support, just use "*.txt" and "tags". */
    helptags_one(dirname, (char_u *)".txt", (char_u *)"tags", add_help_tags);
#endif
}

    static void
helptags_cb(char_u *fname, void *cookie)
{
    do_helptags(fname, *(int *)cookie);
}

/*
 * ":helptags"
 */
    void
ex_helptags(exarg_T *eap)
{
    expand_T	xpc;
    char_u	*dirname;
    int		add_help_tags = FALSE;

    /* Check for ":helptags ++t {dir}". */
    if (STRNCMP(eap->arg, "++t", 3) == 0 && VIM_ISWHITE(eap->arg[3]))
    {
	add_help_tags = TRUE;
	eap->arg = skipwhite(eap->arg + 3);
    }

    if (STRCMP(eap->arg, "ALL") == 0)
    {
	do_in_path(p_rtp, (char_u *)"doc", DIP_ALL + DIP_DIR,
						 helptags_cb, &add_help_tags);
    }
    else
    {
	ExpandInit(&xpc);
	xpc.xp_context = EXPAND_DIRECTORIES;
	dirname = ExpandOne(&xpc, eap->arg, NULL,
			    WILD_LIST_NOTFOUND|WILD_SILENT, WILD_EXPAND_FREE);
	if (dirname == NULL || !mch_isdir(dirname))
	    semsg(_("E150: Not a directory: %s"), eap->arg);
	else
	    do_helptags(dirname, add_help_tags);
	vim_free(dirname);
    }
}

/*
 * Make the user happy.
 */
    void
ex_smile(exarg_T *eap UNUSED)
{
    static char *code[] = {
	"\34 \4o\14$\4ox\30 \2o\30$\1ox\25 \2o\36$\1o\11 \1o\1$\3 \2$\1 \1o\1$x\5 \1o\1 \1$\1 \2o\10 \1o\44$\1o\7 \2$\1 \2$\1 \2$\1o\1$x\2 \2o\1 \1$\1 \1$\1 \1\"\1$\6 \1o\11$\4 \15$\4 \11$\1o\7 \3$\1o\2$\1o\1$x\2 \1\"\6$\1o\1$\5 \1o\11$\6 \13$\6 \12$\1o\4 \10$x\4 \7$\4 \13$\6 \13$\6 \27$x\4 \27$\4 \15$\4 \16$\2 \3\"\3$x\5 \1\"\3$\4\"\61$\5 \1\"\3$x\6 \3$\3 \1o\62$\5 \1\"\3$\1ox\5 \1o\2$\1\"\3 \63$\7 \3$\1ox\5 \3$\4 \55$\1\"\1 \1\"\6$",
	"\5o\4$\1ox\4 \1o\3$\4o\5$\2 \45$\3 \1o\21$x\4 \10$\1\"\4$\3 \42$\5 \4$\10\"x\3 \4\"\7 \4$\4 \1\"\34$\1\"\6 \1o\3$x\16 \1\"\3$\1o\5 \3\"\22$\1\"\2$\1\"\11 \3$x\20 \3$\1o\12 \1\"\2$\2\"\6$\4\"\13 \1o\3$x\21 \4$\1o\40 \1o\3$\1\"x\22 \1\"\4$\1o\6 \1o\6$\1o\1\"\4$\1o\10 \1o\4$x\24 \1\"\5$\2o\5 \2\"\4$\1o\5$\1o\3 \1o\4$\2\"x\27 \2\"\5$\4o\2 \1\"\3$\1o\11$\3\"x\32 \2\"\7$\2o\1 \12$x\42 \4\"\13$x\46 \14$x\47 \12$\1\"x\50 \1\"\3$\4\"x"
    };
    char *p;
    int n;
    int i;

    msg_start();
    msg_putchar('\n');
    for (i = 0; i < 2; ++i)
	for (p = code[i]; *p != NUL; ++p)
	    if (*p == 'x')
		msg_putchar('\n');
	    else
		for (n = *p++; n > 0; --n)
		    if (*p == 'o' || *p == '$')
			msg_putchar_attr(*p, HL_ATTR(HLF_L));
		    else
			msg_putchar(*p);
    msg_clr_eos();
}

/*
 * ":drop"
 * Opens the first argument in a window.  When there are two or more arguments
 * the argument list is redefined.
 */
    void
ex_drop(exarg_T *eap)
{
    int		split = FALSE;
    win_T	*wp;
    buf_T	*buf;
    tabpage_T	*tp;

    /*
     * Check if the first argument is already being edited in a window.  If
     * so, jump to that window.
     * We would actually need to check all arguments, but that's complicated
     * and mostly only one file is dropped.
     * This also ignores wildcards, since it is very unlikely the user is
     * editing a file name with a wildcard character.
     */
    set_arglist(eap->arg);

    /*
     * Expanding wildcards may result in an empty argument list.  E.g. when
     * editing "foo.pyc" and ".pyc" is in 'wildignore'.  Assume that we
     * already did an error message for this.
     */
    if (ARGCOUNT == 0)
	return;

    if (cmdmod.tab)
    {
	/* ":tab drop file ...": open a tab for each argument that isn't
	 * edited in a window yet.  It's like ":tab all" but without closing
	 * windows or tabs. */
	ex_all(eap);
    }
    else
    {
	/* ":drop file ...": Edit the first argument.  Jump to an existing
	 * window if possible, edit in current window if the current buffer
	 * can be abandoned, otherwise open a new window. */
	buf = buflist_findnr(ARGLIST[0].ae_fnum);

	FOR_ALL_TAB_WINDOWS(tp, wp)
	{
	    if (wp->w_buffer == buf)
	    {
		goto_tabpage_win(tp, wp);
		curwin->w_arg_idx = 0;
		return;
	    }
	}

	/*
	 * Check whether the current buffer is changed. If so, we will need
	 * to split the current window or data could be lost.
	 * Skip the check if the 'hidden' option is set, as in this case the
	 * buffer won't be lost.
	 */
	if (!buf_hide(curbuf))
	{
	    ++emsg_off;
	    split = check_changed(curbuf, CCGD_AW | CCGD_EXCMD);
	    --emsg_off;
	}

	/* Fake a ":sfirst" or ":first" command edit the first argument. */
	if (split)
	{
	    eap->cmdidx = CMD_sfirst;
	    eap->cmd[0] = 's';
	}
	else
	    eap->cmdidx = CMD_first;
	ex_rewind(eap);
    }
}

/*
 * Skip over the pattern argument of ":vimgrep /pat/[g][j]".
 * Put the start of the pattern in "*s", unless "s" is NULL.
 * If "flags" is not NULL put the flags in it: VGR_GLOBAL, VGR_NOJUMP.
 * If "s" is not NULL terminate the pattern with a NUL.
 * Return a pointer to the char just past the pattern plus flags.
 */
    char_u *
skip_vimgrep_pat(char_u *p, char_u **s, int *flags)
{
    int		c;

    if (vim_isIDc(*p))
    {
	/* ":vimgrep pattern fname" */
	if (s != NULL)
	    *s = p;
	p = skiptowhite(p);
	if (s != NULL && *p != NUL)
	    *p++ = NUL;
    }
    else
    {
	/* ":vimgrep /pattern/[g][j] fname" */
	if (s != NULL)
	    *s = p + 1;
	c = *p;
	p = skip_regexp(p + 1, c, TRUE, NULL);
	if (*p != c)
	    return NULL;

	/* Truncate the pattern. */
	if (s != NULL)
	    *p = NUL;
	++p;

	/* Find the flags */
	while (*p == 'g' || *p == 'j')
	{
	    if (flags != NULL)
	    {
		if (*p == 'g')
		    *flags |= VGR_GLOBAL;
		else
		    *flags |= VGR_NOJUMP;
	    }
	    ++p;
	}
    }
    return p;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * List v:oldfiles in a nice way.
 */
    void
ex_oldfiles(exarg_T *eap UNUSED)
{
    list_T	*l = get_vim_var_list(VV_OLDFILES);
    listitem_T	*li;
    int		nr = 0;
    char_u	*fname;

    if (l == NULL)
	msg(_("No old files"));
    else
    {
	msg_start();
	msg_scroll = TRUE;
	for (li = l->lv_first; li != NULL && !got_int; li = li->li_next)
	{
	    ++nr;
	    fname = tv_get_string(&li->li_tv);
	    if (!message_filtered(fname))
	    {
		msg_outnum((long)nr);
		msg_puts(": ");
		msg_outtrans(fname);
		msg_clr_eos();
		msg_putchar('\n');
		out_flush();	    /* output one line at a time */
		ui_breakcheck();
	    }
	}

	/* Assume "got_int" was set to truncate the listing. */
	got_int = FALSE;

# ifdef FEAT_BROWSE_CMD
	if (cmdmod.browse)
	{
	    quit_more = FALSE;
	    nr = prompt_for_number(FALSE);
	    msg_starthere();
	    if (nr > 0)
	    {
		char_u *p = list_find_str(get_vim_var_list(VV_OLDFILES),
								    (long)nr);

		if (p != NULL)
		{
		    p = expand_env_save(p);
		    eap->arg = p;
		    eap->cmdidx = CMD_edit;
		    cmdmod.browse = FALSE;
		    do_exedit(eap, NULL);
		    vim_free(p);
		}
	    }
	}
# endif
    }
}
#endif
