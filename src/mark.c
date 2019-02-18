/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * mark.c: functions for setting marks and jumping to them
 */

#include "vim.h"

/*
 * This file contains routines to maintain and manipulate marks.
 */

/*
 * If a named file mark's lnum is non-zero, it is valid.
 * If a named file mark's fnum is non-zero, it is for an existing buffer,
 * otherwise it is from .viminfo and namedfm[n].fname is the file name.
 * There are marks 'A - 'Z (set by user) and '0 to '9 (set when writing
 * viminfo).
 */
#define EXTRA_MARKS 10					/* marks 0-9 */
static xfmark_T namedfm[NMARKS + EXTRA_MARKS];		/* marks with file nr */

static void fmarks_check_one(xfmark_T *fm, char_u *name, buf_T *buf);
static char_u *mark_line(pos_T *mp, int lead_len);
static void show_one_mark(int, char_u *, pos_T *, char_u *, int current);
#ifdef FEAT_VIMINFO
static void write_one_filemark(FILE *fp, xfmark_T *fm, int c1, int c2);
#endif
static void mark_adjust_internal(linenr_T line1, linenr_T line2, long amount,
    long amount_after, int adjust_folds);

/*
 * Set named mark "c" at current cursor position.
 * Returns OK on success, FAIL if bad name given.
 */
    int
setmark(int c)
{
    return setmark_pos(c, &curwin->w_cursor, curbuf->b_fnum);
}

/*
 * Set named mark "c" to position "pos".
 * When "c" is upper case use file "fnum".
 * Returns OK on success, FAIL if bad name given.
 */
    int
setmark_pos(int c, pos_T *pos, int fnum)
{
    int		i;
    buf_T	*buf;

    /* Check for a special key (may cause islower() to crash). */
    if (c < 0)
	return FAIL;

    if (c == '\'' || c == '`')
    {
	if (pos == &curwin->w_cursor)
	{
	    setpcmark();
	    /* keep it even when the cursor doesn't move */
	    curwin->w_prev_pcmark = curwin->w_pcmark;
	}
	else
	    curwin->w_pcmark = *pos;
	return OK;
    }

    buf = buflist_findnr(fnum);
    if (buf == NULL)
	return FAIL;

    if (c == '"')
    {
	buf->b_last_cursor = *pos;
	return OK;
    }

    /* Allow setting '[ and '] for an autocommand that simulates reading a
     * file. */
    if (c == '[')
    {
	buf->b_op_start = *pos;
	return OK;
    }
    if (c == ']')
    {
	buf->b_op_end = *pos;
	return OK;
    }

    if (c == '<' || c == '>')
    {
	if (c == '<')
	    buf->b_visual.vi_start = *pos;
	else
	    buf->b_visual.vi_end = *pos;
	if (buf->b_visual.vi_mode == NUL)
	    /* Visual_mode has not yet been set, use a sane default. */
	    buf->b_visual.vi_mode = 'v';
	return OK;
    }

    if (ASCII_ISLOWER(c))
    {
	i = c - 'a';
	buf->b_namedm[i] = *pos;
	return OK;
    }
    if (ASCII_ISUPPER(c) || VIM_ISDIGIT(c))
    {
	if (VIM_ISDIGIT(c))
	    i = c - '0' + NMARKS;
	else
	    i = c - 'A';
	namedfm[i].fmark.mark = *pos;
	namedfm[i].fmark.fnum = fnum;
	VIM_CLEAR(namedfm[i].fname);
#ifdef FEAT_VIMINFO
	namedfm[i].time_set = vim_time();
#endif
	return OK;
    }
    return FAIL;
}

/*
 * Set the previous context mark to the current position and add it to the
 * jump list.
 */
    void
setpcmark(void)
{
#ifdef FEAT_JUMPLIST
    int		i;
    xfmark_T	*fm;
#endif
#ifdef JUMPLIST_ROTATE
    xfmark_T	tempmark;
#endif

    /* for :global the mark is set only once */
    if (global_busy || listcmd_busy || cmdmod.keepjumps)
	return;

    curwin->w_prev_pcmark = curwin->w_pcmark;
    curwin->w_pcmark = curwin->w_cursor;

#ifdef FEAT_JUMPLIST
# ifdef JUMPLIST_ROTATE
    /*
     * If last used entry is not at the top, put it at the top by rotating
     * the stack until it is (the newer entries will be at the bottom).
     * Keep one entry (the last used one) at the top.
     */
    if (curwin->w_jumplistidx < curwin->w_jumplistlen)
	++curwin->w_jumplistidx;
    while (curwin->w_jumplistidx < curwin->w_jumplistlen)
    {
	tempmark = curwin->w_jumplist[curwin->w_jumplistlen - 1];
	for (i = curwin->w_jumplistlen - 1; i > 0; --i)
	    curwin->w_jumplist[i] = curwin->w_jumplist[i - 1];
	curwin->w_jumplist[0] = tempmark;
	++curwin->w_jumplistidx;
    }
# endif

    /* If jumplist is full: remove oldest entry */
    if (++curwin->w_jumplistlen > JUMPLISTSIZE)
    {
	curwin->w_jumplistlen = JUMPLISTSIZE;
	vim_free(curwin->w_jumplist[0].fname);
	for (i = 1; i < JUMPLISTSIZE; ++i)
	    curwin->w_jumplist[i - 1] = curwin->w_jumplist[i];
    }
    curwin->w_jumplistidx = curwin->w_jumplistlen;
    fm = &curwin->w_jumplist[curwin->w_jumplistlen - 1];

    fm->fmark.mark = curwin->w_pcmark;
    fm->fmark.fnum = curbuf->b_fnum;
    fm->fname = NULL;
# ifdef FEAT_VIMINFO
    fm->time_set = vim_time();
# endif
#endif
}

/*
 * To change context, call setpcmark(), then move the current position to
 * where ever, then call checkpcmark().  This ensures that the previous
 * context will only be changed if the cursor moved to a different line.
 * If pcmark was deleted (with "dG") the previous mark is restored.
 */
    void
checkpcmark(void)
{
    if (curwin->w_prev_pcmark.lnum != 0
	    && (EQUAL_POS(curwin->w_pcmark, curwin->w_cursor)
		|| curwin->w_pcmark.lnum == 0))
    {
	curwin->w_pcmark = curwin->w_prev_pcmark;
	curwin->w_prev_pcmark.lnum = 0;		/* Show it has been checked */
    }
}

#if defined(FEAT_JUMPLIST) || defined(PROTO)
/*
 * move "count" positions in the jump list (count may be negative)
 */
    pos_T *
movemark(int count)
{
    pos_T	*pos;
    xfmark_T	*jmp;

    cleanup_jumplist(curwin, TRUE);

    if (curwin->w_jumplistlen == 0)	    /* nothing to jump to */
	return (pos_T *)NULL;

    for (;;)
    {
	if (curwin->w_jumplistidx + count < 0
		|| curwin->w_jumplistidx + count >= curwin->w_jumplistlen)
	    return (pos_T *)NULL;

	/*
	 * if first CTRL-O or CTRL-I command after a jump, add cursor position
	 * to list.  Careful: If there are duplicates (CTRL-O immediately after
	 * starting Vim on a file), another entry may have been removed.
	 */
	if (curwin->w_jumplistidx == curwin->w_jumplistlen)
	{
	    setpcmark();
	    --curwin->w_jumplistidx;	/* skip the new entry */
	    if (curwin->w_jumplistidx + count < 0)
		return (pos_T *)NULL;
	}

	curwin->w_jumplistidx += count;

	jmp = curwin->w_jumplist + curwin->w_jumplistidx;
	if (jmp->fmark.fnum == 0)
	    fname2fnum(jmp);
	if (jmp->fmark.fnum != curbuf->b_fnum)
	{
	    /* jump to other file */
	    if (buflist_findnr(jmp->fmark.fnum) == NULL)
	    {					     /* Skip this one .. */
		count += count < 0 ? -1 : 1;
		continue;
	    }
	    if (buflist_getfile(jmp->fmark.fnum, jmp->fmark.mark.lnum,
							    0, FALSE) == FAIL)
		return (pos_T *)NULL;
	    /* Set lnum again, autocommands my have changed it */
	    curwin->w_cursor = jmp->fmark.mark;
	    pos = (pos_T *)-1;
	}
	else
	    pos = &(jmp->fmark.mark);
	return pos;
    }
}

/*
 * Move "count" positions in the changelist (count may be negative).
 */
    pos_T *
movechangelist(int count)
{
    int		n;

    if (curbuf->b_changelistlen == 0)	    /* nothing to jump to */
	return (pos_T *)NULL;

    n = curwin->w_changelistidx;
    if (n + count < 0)
    {
	if (n == 0)
	    return (pos_T *)NULL;
	n = 0;
    }
    else if (n + count >= curbuf->b_changelistlen)
    {
	if (n == curbuf->b_changelistlen - 1)
	    return (pos_T *)NULL;
	n = curbuf->b_changelistlen - 1;
    }
    else
	n += count;
    curwin->w_changelistidx = n;
    return curbuf->b_changelist + n;
}
#endif

/*
 * Find mark "c" in buffer pointed to by "buf".
 * If "changefile" is TRUE it's allowed to edit another file for '0, 'A, etc.
 * If "fnum" is not NULL store the fnum there for '0, 'A etc., don't edit
 * another file.
 * Returns:
 * - pointer to pos_T if found.  lnum is 0 when mark not set, -1 when mark is
 *   in another file which can't be gotten. (caller needs to check lnum!)
 * - NULL if there is no mark called 'c'.
 * - -1 if mark is in other file and jumped there (only if changefile is TRUE)
 */
    pos_T *
getmark_buf(buf_T *buf, int c, int changefile)
{
    return getmark_buf_fnum(buf, c, changefile, NULL);
}

    pos_T *
getmark(int c, int changefile)
{
    return getmark_buf_fnum(curbuf, c, changefile, NULL);
}

    pos_T *
getmark_buf_fnum(
    buf_T	*buf,
    int		c,
    int		changefile,
    int		*fnum)
{
    pos_T		*posp;
    pos_T		*startp, *endp;
    static pos_T	pos_copy;

    posp = NULL;

    /* Check for special key, can't be a mark name and might cause islower()
     * to crash. */
    if (c < 0)
	return posp;
#ifndef EBCDIC
    if (c > '~')			/* check for islower()/isupper() */
	;
    else
#endif
	if (c == '\'' || c == '`')	/* previous context mark */
    {
	pos_copy = curwin->w_pcmark;	/* need to make a copy because */
	posp = &pos_copy;		/*   w_pcmark may be changed soon */
    }
    else if (c == '"')			/* to pos when leaving buffer */
	posp = &(buf->b_last_cursor);
    else if (c == '^')			/* to where Insert mode stopped */
	posp = &(buf->b_last_insert);
    else if (c == '.')			/* to where last change was made */
	posp = &(buf->b_last_change);
    else if (c == '[')			/* to start of previous operator */
	posp = &(buf->b_op_start);
    else if (c == ']')			/* to end of previous operator */
	posp = &(buf->b_op_end);
    else if (c == '{' || c == '}')	/* to previous/next paragraph */
    {
	pos_T	pos;
	oparg_T	oa;
	int	slcb = listcmd_busy;

	pos = curwin->w_cursor;
	listcmd_busy = TRUE;	    /* avoid that '' is changed */
	if (findpar(&oa.inclusive,
			       c == '}' ? FORWARD : BACKWARD, 1L, NUL, FALSE))
	{
	    pos_copy = curwin->w_cursor;
	    posp = &pos_copy;
	}
	curwin->w_cursor = pos;
	listcmd_busy = slcb;
    }
    else if (c == '(' || c == ')')	/* to previous/next sentence */
    {
	pos_T	pos;
	int	slcb = listcmd_busy;

	pos = curwin->w_cursor;
	listcmd_busy = TRUE;	    /* avoid that '' is changed */
	if (findsent(c == ')' ? FORWARD : BACKWARD, 1L))
	{
	    pos_copy = curwin->w_cursor;
	    posp = &pos_copy;
	}
	curwin->w_cursor = pos;
	listcmd_busy = slcb;
    }
    else if (c == '<' || c == '>')	/* start/end of visual area */
    {
	startp = &buf->b_visual.vi_start;
	endp = &buf->b_visual.vi_end;
	if (((c == '<') == LT_POS(*startp, *endp) || endp->lnum == 0)
							  && startp->lnum != 0)
	    posp = startp;
	else
	    posp = endp;
	/*
	 * For Visual line mode, set mark at begin or end of line
	 */
	if (buf->b_visual.vi_mode == 'V')
	{
	    pos_copy = *posp;
	    posp = &pos_copy;
	    if (c == '<')
		pos_copy.col = 0;
	    else
		pos_copy.col = MAXCOL;
	    pos_copy.coladd = 0;
	}
    }
    else if (ASCII_ISLOWER(c))		/* normal named mark */
    {
	posp = &(buf->b_namedm[c - 'a']);
    }
    else if (ASCII_ISUPPER(c) || VIM_ISDIGIT(c))	/* named file mark */
    {
	if (VIM_ISDIGIT(c))
	    c = c - '0' + NMARKS;
	else
	    c -= 'A';
	posp = &(namedfm[c].fmark.mark);

	if (namedfm[c].fmark.fnum == 0)
	    fname2fnum(&namedfm[c]);

	if (fnum != NULL)
	    *fnum = namedfm[c].fmark.fnum;
	else if (namedfm[c].fmark.fnum != buf->b_fnum)
	{
	    /* mark is in another file */
	    posp = &pos_copy;

	    if (namedfm[c].fmark.mark.lnum != 0
				       && changefile && namedfm[c].fmark.fnum)
	    {
		if (buflist_getfile(namedfm[c].fmark.fnum,
				      (linenr_T)1, GETF_SETMARK, FALSE) == OK)
		{
		    /* Set the lnum now, autocommands could have changed it */
		    curwin->w_cursor = namedfm[c].fmark.mark;
		    return (pos_T *)-1;
		}
		pos_copy.lnum = -1;	/* can't get file */
	    }
	    else
		pos_copy.lnum = 0;	/* mark exists, but is not valid in
					   current buffer */
	}
    }

    return posp;
}

/*
 * Search for the next named mark in the current file.
 *
 * Returns pointer to pos_T of the next mark or NULL if no mark is found.
 */
    pos_T *
getnextmark(
    pos_T	*startpos,	/* where to start */
    int		dir,	/* direction for search */
    int		begin_line)
{
    int		i;
    pos_T	*result = NULL;
    pos_T	pos;

    pos = *startpos;

    /* When searching backward and leaving the cursor on the first non-blank,
     * position must be in a previous line.
     * When searching forward and leaving the cursor on the first non-blank,
     * position must be in a next line. */
    if (dir == BACKWARD && begin_line)
	pos.col = 0;
    else if (dir == FORWARD && begin_line)
	pos.col = MAXCOL;

    for (i = 0; i < NMARKS; i++)
    {
	if (curbuf->b_namedm[i].lnum > 0)
	{
	    if (dir == FORWARD)
	    {
		if ((result == NULL || LT_POS(curbuf->b_namedm[i], *result))
			&& LT_POS(pos, curbuf->b_namedm[i]))
		    result = &curbuf->b_namedm[i];
	    }
	    else
	    {
		if ((result == NULL || LT_POS(*result, curbuf->b_namedm[i]))
			&& LT_POS(curbuf->b_namedm[i], pos))
		    result = &curbuf->b_namedm[i];
	    }
	}
    }

    return result;
}

/*
 * For an xtended filemark: set the fnum from the fname.
 * This is used for marks obtained from the .viminfo file.  It's postponed
 * until the mark is used to avoid a long startup delay.
 */
    void
fname2fnum(xfmark_T *fm)
{
    char_u	*p;

    if (fm->fname != NULL)
    {
	/*
	 * First expand "~/" in the file name to the home directory.
	 * Don't expand the whole name, it may contain other '~' chars.
	 */
	if (fm->fname[0] == '~' && (fm->fname[1] == '/'
#ifdef BACKSLASH_IN_FILENAME
		    || fm->fname[1] == '\\'
#endif
		    ))
	{
	    int len;

	    expand_env((char_u *)"~/", NameBuff, MAXPATHL);
	    len = (int)STRLEN(NameBuff);
	    vim_strncpy(NameBuff + len, fm->fname + 2, MAXPATHL - len - 1);
	}
	else
	    vim_strncpy(NameBuff, fm->fname, MAXPATHL - 1);

	/* Try to shorten the file name. */
	mch_dirname(IObuff, IOSIZE);
	p = shorten_fname(NameBuff, IObuff);

	/* buflist_new() will call fmarks_check_names() */
	(void)buflist_new(NameBuff, p, (linenr_T)1, 0);
    }
}

/*
 * Check all file marks for a name that matches the file name in buf.
 * May replace the name with an fnum.
 * Used for marks that come from the .viminfo file.
 */
    void
fmarks_check_names(buf_T *buf)
{
    char_u	*name;
    int		i;
#ifdef FEAT_JUMPLIST
    win_T	*wp;
#endif

    if (buf->b_ffname == NULL)
	return;

    name = home_replace_save(buf, buf->b_ffname);
    if (name == NULL)
	return;

    for (i = 0; i < NMARKS + EXTRA_MARKS; ++i)
	fmarks_check_one(&namedfm[i], name, buf);

#ifdef FEAT_JUMPLIST
    FOR_ALL_WINDOWS(wp)
    {
	for (i = 0; i < wp->w_jumplistlen; ++i)
	    fmarks_check_one(&wp->w_jumplist[i], name, buf);
    }
#endif

    vim_free(name);
}

    static void
fmarks_check_one(xfmark_T *fm, char_u *name, buf_T *buf)
{
    if (fm->fmark.fnum == 0
	    && fm->fname != NULL
	    && fnamecmp(name, fm->fname) == 0)
    {
	fm->fmark.fnum = buf->b_fnum;
	VIM_CLEAR(fm->fname);
    }
}

/*
 * Check a if a position from a mark is valid.
 * Give and error message and return FAIL if not.
 */
    int
check_mark(pos_T *pos)
{
    if (pos == NULL)
    {
	emsg(_(e_umark));
	return FAIL;
    }
    if (pos->lnum <= 0)
    {
	/* lnum is negative if mark is in another file can can't get that
	 * file, error message already give then. */
	if (pos->lnum == 0)
	    emsg(_(e_marknotset));
	return FAIL;
    }
    if (pos->lnum > curbuf->b_ml.ml_line_count)
    {
	emsg(_(e_markinval));
	return FAIL;
    }
    return OK;
}

/*
 * clrallmarks() - clear all marks in the buffer 'buf'
 *
 * Used mainly when trashing the entire buffer during ":e" type commands
 */
    void
clrallmarks(buf_T *buf)
{
    static int		i = -1;

    if (i == -1)	/* first call ever: initialize */
	for (i = 0; i < NMARKS + 1; i++)
	{
	    namedfm[i].fmark.mark.lnum = 0;
	    namedfm[i].fname = NULL;
#ifdef FEAT_VIMINFO
	    namedfm[i].time_set = 0;
#endif
	}

    for (i = 0; i < NMARKS; i++)
	buf->b_namedm[i].lnum = 0;
    buf->b_op_start.lnum = 0;		/* start/end op mark cleared */
    buf->b_op_end.lnum = 0;
    buf->b_last_cursor.lnum = 1;	/* '" mark cleared */
    buf->b_last_cursor.col = 0;
    buf->b_last_cursor.coladd = 0;
    buf->b_last_insert.lnum = 0;	/* '^ mark cleared */
    buf->b_last_change.lnum = 0;	/* '. mark cleared */
#ifdef FEAT_JUMPLIST
    buf->b_changelistlen = 0;
#endif
}

/*
 * Get name of file from a filemark.
 * When it's in the current buffer, return the text at the mark.
 * Returns an allocated string.
 */
    char_u *
fm_getname(fmark_T *fmark, int lead_len)
{
    if (fmark->fnum == curbuf->b_fnum)		    /* current buffer */
	return mark_line(&(fmark->mark), lead_len);
    return buflist_nr2name(fmark->fnum, FALSE, TRUE);
}

/*
 * Return the line at mark "mp".  Truncate to fit in window.
 * The returned string has been allocated.
 */
    static char_u *
mark_line(pos_T *mp, int lead_len)
{
    char_u	*s, *p;
    int		len;

    if (mp->lnum == 0 || mp->lnum > curbuf->b_ml.ml_line_count)
	return vim_strsave((char_u *)"-invalid-");
    // Allow for up to 5 bytes per character.
    s = vim_strnsave(skipwhite(ml_get(mp->lnum)), (int)Columns * 5);
    if (s == NULL)
	return NULL;
    // Truncate the line to fit it in the window.
    len = 0;
    for (p = s; *p != NUL; MB_PTR_ADV(p))
    {
	len += ptr2cells(p);
	if (len >= Columns - lead_len)
	    break;
    }
    *p = NUL;
    return s;
}

/*
 * print the marks
 */
    void
do_marks(exarg_T *eap)
{
    char_u	*arg = eap->arg;
    int		i;
    char_u	*name;

    if (arg != NULL && *arg == NUL)
	arg = NULL;

    show_one_mark('\'', arg, &curwin->w_pcmark, NULL, TRUE);
    for (i = 0; i < NMARKS; ++i)
	show_one_mark(i + 'a', arg, &curbuf->b_namedm[i], NULL, TRUE);
    for (i = 0; i < NMARKS + EXTRA_MARKS; ++i)
    {
	if (namedfm[i].fmark.fnum != 0)
	    name = fm_getname(&namedfm[i].fmark, 15);
	else
	    name = namedfm[i].fname;
	if (name != NULL)
	{
	    show_one_mark(i >= NMARKS ? i - NMARKS + '0' : i + 'A',
		    arg, &namedfm[i].fmark.mark, name,
		    namedfm[i].fmark.fnum == curbuf->b_fnum);
	    if (namedfm[i].fmark.fnum != 0)
		vim_free(name);
	}
    }
    show_one_mark('"', arg, &curbuf->b_last_cursor, NULL, TRUE);
    show_one_mark('[', arg, &curbuf->b_op_start, NULL, TRUE);
    show_one_mark(']', arg, &curbuf->b_op_end, NULL, TRUE);
    show_one_mark('^', arg, &curbuf->b_last_insert, NULL, TRUE);
    show_one_mark('.', arg, &curbuf->b_last_change, NULL, TRUE);
    show_one_mark('<', arg, &curbuf->b_visual.vi_start, NULL, TRUE);
    show_one_mark('>', arg, &curbuf->b_visual.vi_end, NULL, TRUE);
    show_one_mark(-1, arg, NULL, NULL, FALSE);
}

    static void
show_one_mark(
    int		c,
    char_u	*arg,
    pos_T	*p,
    char_u	*name,
    int		current)	/* in current file */
{
    static int	did_title = FALSE;
    int		mustfree = FALSE;

    if (c == -1)			    /* finish up */
    {
	if (did_title)
	    did_title = FALSE;
	else
	{
	    if (arg == NULL)
		msg(_("No marks set"));
	    else
		semsg(_("E283: No marks matching \"%s\""), arg);
	}
    }
    /* don't output anything if 'q' typed at --more-- prompt */
    else if (!got_int
	    && (arg == NULL || vim_strchr(arg, c) != NULL)
	    && p->lnum != 0)
    {
	if (!did_title)
	{
	    /* Highlight title */
	    msg_puts_title(_("\nmark line  col file/text"));
	    did_title = TRUE;
	}
	msg_putchar('\n');
	if (!got_int)
	{
	    sprintf((char *)IObuff, " %c %6ld %4d ", c, p->lnum, p->col);
	    msg_outtrans(IObuff);
	    if (name == NULL && current)
	    {
		name = mark_line(p, 15);
		mustfree = TRUE;
	    }
	    if (name != NULL)
	    {
		msg_outtrans_attr(name, current ? HL_ATTR(HLF_D) : 0);
		if (mustfree)
		    vim_free(name);
	    }
	}
	out_flush();		    /* show one line at a time */
    }
}

/*
 * ":delmarks[!] [marks]"
 */
    void
ex_delmarks(exarg_T *eap)
{
    char_u	*p;
    int		from, to;
    int		i;
    int		lower;
    int		digit;
    int		n;

    if (*eap->arg == NUL && eap->forceit)
	/* clear all marks */
	clrallmarks(curbuf);
    else if (eap->forceit)
	emsg(_(e_invarg));
    else if (*eap->arg == NUL)
	emsg(_(e_argreq));
    else
    {
	/* clear specified marks only */
	for (p = eap->arg; *p != NUL; ++p)
	{
	    lower = ASCII_ISLOWER(*p);
	    digit = VIM_ISDIGIT(*p);
	    if (lower || digit || ASCII_ISUPPER(*p))
	    {
		if (p[1] == '-')
		{
		    /* clear range of marks */
		    from = *p;
		    to = p[2];
		    if (!(lower ? ASCII_ISLOWER(p[2])
				: (digit ? VIM_ISDIGIT(p[2])
				    : ASCII_ISUPPER(p[2])))
			    || to < from)
		    {
			semsg(_(e_invarg2), p);
			return;
		    }
		    p += 2;
		}
		else
		    /* clear one lower case mark */
		    from = to = *p;

		for (i = from; i <= to; ++i)
		{
		    if (lower)
			curbuf->b_namedm[i - 'a'].lnum = 0;
		    else
		    {
			if (digit)
			    n = i - '0' + NMARKS;
			else
			    n = i - 'A';
			namedfm[n].fmark.mark.lnum = 0;
			VIM_CLEAR(namedfm[n].fname);
#ifdef FEAT_VIMINFO
			namedfm[n].time_set = 0;
#endif
		    }
		}
	    }
	    else
		switch (*p)
		{
		    case '"': curbuf->b_last_cursor.lnum = 0; break;
		    case '^': curbuf->b_last_insert.lnum = 0; break;
		    case '.': curbuf->b_last_change.lnum = 0; break;
		    case '[': curbuf->b_op_start.lnum    = 0; break;
		    case ']': curbuf->b_op_end.lnum      = 0; break;
		    case '<': curbuf->b_visual.vi_start.lnum = 0; break;
		    case '>': curbuf->b_visual.vi_end.lnum   = 0; break;
		    case ' ': break;
		    default:  semsg(_(e_invarg2), p);
			      return;
		}
	}
    }
}

#if defined(FEAT_JUMPLIST) || defined(PROTO)
/*
 * print the jumplist
 */
    void
ex_jumps(exarg_T *eap UNUSED)
{
    int		i;
    char_u	*name;

    cleanup_jumplist(curwin, TRUE);

    /* Highlight title */
    msg_puts_title(_("\n jump line  col file/text"));
    for (i = 0; i < curwin->w_jumplistlen && !got_int; ++i)
    {
	if (curwin->w_jumplist[i].fmark.mark.lnum != 0)
	{
	    name = fm_getname(&curwin->w_jumplist[i].fmark, 16);

	    // apply :filter /pat/ or file name not available
	    if (name == NULL || message_filtered(name))
	    {
		vim_free(name);
		continue;
	    }

	    msg_putchar('\n');
	    if (got_int)
	    {
		vim_free(name);
		break;
	    }
	    sprintf((char *)IObuff, "%c %2d %5ld %4d ",
		i == curwin->w_jumplistidx ? '>' : ' ',
		i > curwin->w_jumplistidx ? i - curwin->w_jumplistidx
					  : curwin->w_jumplistidx - i,
		curwin->w_jumplist[i].fmark.mark.lnum,
		curwin->w_jumplist[i].fmark.mark.col);
	    msg_outtrans(IObuff);
	    msg_outtrans_attr(name,
			    curwin->w_jumplist[i].fmark.fnum == curbuf->b_fnum
							? HL_ATTR(HLF_D) : 0);
	    vim_free(name);
	    ui_breakcheck();
	}
	out_flush();
    }
    if (curwin->w_jumplistidx == curwin->w_jumplistlen)
	msg_puts("\n>");
}

    void
ex_clearjumps(exarg_T *eap UNUSED)
{
    free_jumplist(curwin);
    curwin->w_jumplistlen = 0;
    curwin->w_jumplistidx = 0;
}

/*
 * print the changelist
 */
    void
ex_changes(exarg_T *eap UNUSED)
{
    int		i;
    char_u	*name;

    /* Highlight title */
    msg_puts_title(_("\nchange line  col text"));

    for (i = 0; i < curbuf->b_changelistlen && !got_int; ++i)
    {
	if (curbuf->b_changelist[i].lnum != 0)
	{
	    msg_putchar('\n');
	    if (got_int)
		break;
	    sprintf((char *)IObuff, "%c %3d %5ld %4d ",
		    i == curwin->w_changelistidx ? '>' : ' ',
		    i > curwin->w_changelistidx ? i - curwin->w_changelistidx
						: curwin->w_changelistidx - i,
		    (long)curbuf->b_changelist[i].lnum,
		    curbuf->b_changelist[i].col);
	    msg_outtrans(IObuff);
	    name = mark_line(&curbuf->b_changelist[i], 17);
	    if (name == NULL)
		break;
	    msg_outtrans_attr(name, HL_ATTR(HLF_D));
	    vim_free(name);
	    ui_breakcheck();
	}
	out_flush();
    }
    if (curwin->w_changelistidx == curbuf->b_changelistlen)
	msg_puts("\n>");
}
#endif

#define one_adjust(add) \
    { \
	lp = add; \
	if (*lp >= line1 && *lp <= line2) \
	{ \
	    if (amount == MAXLNUM) \
		*lp = 0; \
	    else \
		*lp += amount; \
	} \
	else if (amount_after && *lp > line2) \
	    *lp += amount_after; \
    }

/* don't delete the line, just put at first deleted line */
#define one_adjust_nodel(add) \
    { \
	lp = add; \
	if (*lp >= line1 && *lp <= line2) \
	{ \
	    if (amount == MAXLNUM) \
		*lp = line1; \
	    else \
		*lp += amount; \
	} \
	else if (amount_after && *lp > line2) \
	    *lp += amount_after; \
    }

/*
 * Adjust marks between line1 and line2 (inclusive) to move 'amount' lines.
 * Must be called before changed_*(), appended_lines() or deleted_lines().
 * May be called before or after changing the text.
 * When deleting lines line1 to line2, use an 'amount' of MAXLNUM: The marks
 * within this range are made invalid.
 * If 'amount_after' is non-zero adjust marks after line2.
 * Example: Delete lines 34 and 35: mark_adjust(34, 35, MAXLNUM, -2);
 * Example: Insert two lines below 55: mark_adjust(56, MAXLNUM, 2, 0);
 *				   or: mark_adjust(56, 55, MAXLNUM, 2);
 */
    void
mark_adjust(
    linenr_T	line1,
    linenr_T	line2,
    long	amount,
    long	amount_after)
{
    mark_adjust_internal(line1, line2, amount, amount_after, TRUE);
}

    void
mark_adjust_nofold(
    linenr_T line1,
    linenr_T line2,
    long amount,
    long amount_after)
{
    mark_adjust_internal(line1, line2, amount, amount_after, FALSE);
}

    static void
mark_adjust_internal(
    linenr_T line1,
    linenr_T line2,
    long amount,
    long amount_after,
    int adjust_folds UNUSED)
{
    int		i;
    int		fnum = curbuf->b_fnum;
    linenr_T	*lp;
    win_T	*win;
    tabpage_T	*tab;
    static pos_T initpos = {1, 0, 0};

    if (line2 < line1 && amount_after == 0L)	    /* nothing to do */
	return;

    if (!cmdmod.lockmarks)
    {
	/* named marks, lower case and upper case */
	for (i = 0; i < NMARKS; i++)
	{
	    one_adjust(&(curbuf->b_namedm[i].lnum));
	    if (namedfm[i].fmark.fnum == fnum)
		one_adjust_nodel(&(namedfm[i].fmark.mark.lnum));
	}
	for (i = NMARKS; i < NMARKS + EXTRA_MARKS; i++)
	{
	    if (namedfm[i].fmark.fnum == fnum)
		one_adjust_nodel(&(namedfm[i].fmark.mark.lnum));
	}

	/* last Insert position */
	one_adjust(&(curbuf->b_last_insert.lnum));

	/* last change position */
	one_adjust(&(curbuf->b_last_change.lnum));

	/* last cursor position, if it was set */
	if (!EQUAL_POS(curbuf->b_last_cursor, initpos))
	    one_adjust(&(curbuf->b_last_cursor.lnum));


#ifdef FEAT_JUMPLIST
	/* list of change positions */
	for (i = 0; i < curbuf->b_changelistlen; ++i)
	    one_adjust_nodel(&(curbuf->b_changelist[i].lnum));
#endif

	/* Visual area */
	one_adjust_nodel(&(curbuf->b_visual.vi_start.lnum));
	one_adjust_nodel(&(curbuf->b_visual.vi_end.lnum));

#ifdef FEAT_QUICKFIX
	/* quickfix marks */
	qf_mark_adjust(NULL, line1, line2, amount, amount_after);
	/* location lists */
	FOR_ALL_TAB_WINDOWS(tab, win)
	    qf_mark_adjust(win, line1, line2, amount, amount_after);
#endif

#ifdef FEAT_SIGNS
	sign_mark_adjust(line1, line2, amount, amount_after);
#endif
    }

    /* previous context mark */
    one_adjust(&(curwin->w_pcmark.lnum));

    /* previous pcmark */
    one_adjust(&(curwin->w_prev_pcmark.lnum));

    /* saved cursor for formatting */
    if (saved_cursor.lnum != 0)
	one_adjust_nodel(&(saved_cursor.lnum));

    /*
     * Adjust items in all windows related to the current buffer.
     */
    FOR_ALL_TAB_WINDOWS(tab, win)
    {
#ifdef FEAT_JUMPLIST
	if (!cmdmod.lockmarks)
	    /* Marks in the jumplist.  When deleting lines, this may create
	     * duplicate marks in the jumplist, they will be removed later. */
	    for (i = 0; i < win->w_jumplistlen; ++i)
		if (win->w_jumplist[i].fmark.fnum == fnum)
		    one_adjust_nodel(&(win->w_jumplist[i].fmark.mark.lnum));
#endif

	if (win->w_buffer == curbuf)
	{
	    if (!cmdmod.lockmarks)
		/* marks in the tag stack */
		for (i = 0; i < win->w_tagstacklen; i++)
		    if (win->w_tagstack[i].fmark.fnum == fnum)
			one_adjust_nodel(&(win->w_tagstack[i].fmark.mark.lnum));

	    /* the displayed Visual area */
	    if (win->w_old_cursor_lnum != 0)
	    {
		one_adjust_nodel(&(win->w_old_cursor_lnum));
		one_adjust_nodel(&(win->w_old_visual_lnum));
	    }

	    /* topline and cursor position for windows with the same buffer
	     * other than the current window */
	    if (win != curwin)
	    {
		if (win->w_topline >= line1 && win->w_topline <= line2)
		{
		    if (amount == MAXLNUM)	    /* topline is deleted */
		    {
			if (line1 <= 1)
			    win->w_topline = 1;
			else
			    win->w_topline = line1 - 1;
		    }
		    else		/* keep topline on the same line */
			win->w_topline += amount;
#ifdef FEAT_DIFF
		    win->w_topfill = 0;
#endif
		}
		else if (amount_after && win->w_topline > line2)
		{
		    win->w_topline += amount_after;
#ifdef FEAT_DIFF
		    win->w_topfill = 0;
#endif
		}
		if (win->w_cursor.lnum >= line1 && win->w_cursor.lnum <= line2)
		{
		    if (amount == MAXLNUM) /* line with cursor is deleted */
		    {
			if (line1 <= 1)
			    win->w_cursor.lnum = 1;
			else
			    win->w_cursor.lnum = line1 - 1;
			win->w_cursor.col = 0;
		    }
		    else		/* keep cursor on the same line */
			win->w_cursor.lnum += amount;
		}
		else if (amount_after && win->w_cursor.lnum > line2)
		    win->w_cursor.lnum += amount_after;
	    }

#ifdef FEAT_FOLDING
	    /* adjust folds */
	    if (adjust_folds)
		foldMarkAdjust(win, line1, line2, amount, amount_after);
#endif
	}
    }

#ifdef FEAT_DIFF
    /* adjust diffs */
    diff_mark_adjust(line1, line2, amount, amount_after);
#endif
}

/* This code is used often, needs to be fast. */
#define col_adjust(pp) \
    { \
	posp = pp; \
	if (posp->lnum == lnum && posp->col >= mincol) \
	{ \
	    posp->lnum += lnum_amount; \
	    if (col_amount < 0 && posp->col <= (colnr_T)-col_amount) \
		posp->col = 0; \
	    else if (posp->col < spaces_removed) \
		posp->col = col_amount + spaces_removed; \
	    else \
		posp->col += col_amount; \
	} \
    }

/*
 * Adjust marks in line "lnum" at column "mincol" and further: add
 * "lnum_amount" to the line number and add "col_amount" to the column
 * position.
 * "spaces_removed" is the number of spaces that were removed, matters when the
 * cursor is inside them.
 */
    void
mark_col_adjust(
    linenr_T	lnum,
    colnr_T	mincol,
    long	lnum_amount,
    long	col_amount,
    int		spaces_removed)
{
    int		i;
    int		fnum = curbuf->b_fnum;
    win_T	*win;
    pos_T	*posp;

    if ((col_amount == 0L && lnum_amount == 0L) || cmdmod.lockmarks)
	return; /* nothing to do */

    /* named marks, lower case and upper case */
    for (i = 0; i < NMARKS; i++)
    {
	col_adjust(&(curbuf->b_namedm[i]));
	if (namedfm[i].fmark.fnum == fnum)
	    col_adjust(&(namedfm[i].fmark.mark));
    }
    for (i = NMARKS; i < NMARKS + EXTRA_MARKS; i++)
    {
	if (namedfm[i].fmark.fnum == fnum)
	    col_adjust(&(namedfm[i].fmark.mark));
    }

    /* last Insert position */
    col_adjust(&(curbuf->b_last_insert));

    /* last change position */
    col_adjust(&(curbuf->b_last_change));

#ifdef FEAT_JUMPLIST
    /* list of change positions */
    for (i = 0; i < curbuf->b_changelistlen; ++i)
	col_adjust(&(curbuf->b_changelist[i]));
#endif

    /* Visual area */
    col_adjust(&(curbuf->b_visual.vi_start));
    col_adjust(&(curbuf->b_visual.vi_end));

    /* previous context mark */
    col_adjust(&(curwin->w_pcmark));

    /* previous pcmark */
    col_adjust(&(curwin->w_prev_pcmark));

    /* saved cursor for formatting */
    col_adjust(&saved_cursor);

    /*
     * Adjust items in all windows related to the current buffer.
     */
    FOR_ALL_WINDOWS(win)
    {
#ifdef FEAT_JUMPLIST
	/* marks in the jumplist */
	for (i = 0; i < win->w_jumplistlen; ++i)
	    if (win->w_jumplist[i].fmark.fnum == fnum)
		col_adjust(&(win->w_jumplist[i].fmark.mark));
#endif

	if (win->w_buffer == curbuf)
	{
	    /* marks in the tag stack */
	    for (i = 0; i < win->w_tagstacklen; i++)
		if (win->w_tagstack[i].fmark.fnum == fnum)
		    col_adjust(&(win->w_tagstack[i].fmark.mark));

	    /* cursor position for other windows with the same buffer */
	    if (win != curwin)
		col_adjust(&win->w_cursor);
	}
    }
}

#ifdef FEAT_JUMPLIST
/*
 * When deleting lines, this may create duplicate marks in the
 * jumplist. They will be removed here for the specified window.
 * When "loadfiles" is TRUE first ensure entries have the "fnum" field set
 * (this may be a bit slow).
 */
    void
cleanup_jumplist(win_T *wp, int loadfiles)
{
    int	    i;
    int	    from, to;

    if (loadfiles)
    {
	/* If specified, load all the files from the jump list. This is
	 * needed to properly clean up duplicate entries, but will take some
	 * time. */
	for (i = 0; i < wp->w_jumplistlen; ++i)
	{
	    if ((wp->w_jumplist[i].fmark.fnum == 0) &&
		    (wp->w_jumplist[i].fmark.mark.lnum != 0))
		fname2fnum(&wp->w_jumplist[i]);
	}
    }

    to = 0;
    for (from = 0; from < wp->w_jumplistlen; ++from)
    {
	if (wp->w_jumplistidx == from)
	    wp->w_jumplistidx = to;
	for (i = from + 1; i < wp->w_jumplistlen; ++i)
	    if (wp->w_jumplist[i].fmark.fnum
					== wp->w_jumplist[from].fmark.fnum
		    && wp->w_jumplist[from].fmark.fnum != 0
		    && wp->w_jumplist[i].fmark.mark.lnum
				  == wp->w_jumplist[from].fmark.mark.lnum)
		break;
	if (i >= wp->w_jumplistlen)	    /* no duplicate */
	    wp->w_jumplist[to++] = wp->w_jumplist[from];
	else
	    vim_free(wp->w_jumplist[from].fname);
    }
    if (wp->w_jumplistidx == wp->w_jumplistlen)
	wp->w_jumplistidx = to;
    wp->w_jumplistlen = to;
}

/*
 * Copy the jumplist from window "from" to window "to".
 */
    void
copy_jumplist(win_T *from, win_T *to)
{
    int		i;

    for (i = 0; i < from->w_jumplistlen; ++i)
    {
	to->w_jumplist[i] = from->w_jumplist[i];
	if (from->w_jumplist[i].fname != NULL)
	    to->w_jumplist[i].fname = vim_strsave(from->w_jumplist[i].fname);
    }
    to->w_jumplistlen = from->w_jumplistlen;
    to->w_jumplistidx = from->w_jumplistidx;
}

/*
 * Free items in the jumplist of window "wp".
 */
    void
free_jumplist(win_T *wp)
{
    int		i;

    for (i = 0; i < wp->w_jumplistlen; ++i)
	vim_free(wp->w_jumplist[i].fname);
}
#endif /* FEAT_JUMPLIST */

    void
set_last_cursor(win_T *win)
{
    if (win->w_buffer != NULL)
	win->w_buffer->b_last_cursor = win->w_cursor;
}

#if defined(EXITFREE) || defined(PROTO)
    void
free_all_marks(void)
{
    int		i;

    for (i = 0; i < NMARKS + EXTRA_MARKS; i++)
	if (namedfm[i].fmark.mark.lnum != 0)
	    vim_free(namedfm[i].fname);
}
#endif

#if defined(FEAT_VIMINFO) || defined(PROTO)
    int
read_viminfo_filemark(vir_T *virp, int force)
{
    char_u	*str;
    xfmark_T	*fm;
    int		i;

    /* We only get here if line[0] == '\'' or '-'.
     * Illegal mark names are ignored (for future expansion). */
    str = virp->vir_line + 1;
    if (
#ifndef EBCDIC
	    *str <= 127 &&
#endif
	    ((*virp->vir_line == '\'' && (VIM_ISDIGIT(*str) || isupper(*str)))
	     || (*virp->vir_line == '-' && *str == '\'')))
    {
	if (*str == '\'')
	{
#ifdef FEAT_JUMPLIST
	    /* If the jumplist isn't full insert fmark as oldest entry */
	    if (curwin->w_jumplistlen == JUMPLISTSIZE)
		fm = NULL;
	    else
	    {
		for (i = curwin->w_jumplistlen; i > 0; --i)
		    curwin->w_jumplist[i] = curwin->w_jumplist[i - 1];
		++curwin->w_jumplistidx;
		++curwin->w_jumplistlen;
		fm = &curwin->w_jumplist[0];
		fm->fmark.mark.lnum = 0;
		fm->fname = NULL;
	    }
#else
	    fm = NULL;
#endif
	}
	else if (VIM_ISDIGIT(*str))
	    fm = &namedfm[*str - '0' + NMARKS];
	else
	    fm = &namedfm[*str - 'A'];
	if (fm != NULL && (fm->fmark.mark.lnum == 0 || force))
	{
	    str = skipwhite(str + 1);
	    fm->fmark.mark.lnum = getdigits(&str);
	    str = skipwhite(str);
	    fm->fmark.mark.col = getdigits(&str);
	    fm->fmark.mark.coladd = 0;
	    fm->fmark.fnum = 0;
	    str = skipwhite(str);
	    vim_free(fm->fname);
	    fm->fname = viminfo_readstring(virp, (int)(str - virp->vir_line),
								       FALSE);
	    fm->time_set = 0;
	}
    }
    return vim_fgets(virp->vir_line, LSIZE, virp->vir_fd);
}

static xfmark_T *vi_namedfm = NULL;
#ifdef FEAT_JUMPLIST
static xfmark_T *vi_jumplist = NULL;
static int vi_jumplist_len = 0;
#endif

/*
 * Prepare for reading viminfo marks when writing viminfo later.
 */
    void
prepare_viminfo_marks(void)
{
    vi_namedfm = (xfmark_T *)alloc_clear((NMARKS + EXTRA_MARKS)
						     * (int)sizeof(xfmark_T));
#ifdef FEAT_JUMPLIST
    vi_jumplist = (xfmark_T *)alloc_clear(JUMPLISTSIZE
						     * (int)sizeof(xfmark_T));
    vi_jumplist_len = 0;
#endif
}

    void
finish_viminfo_marks(void)
{
    int		i;

    if (vi_namedfm != NULL)
    {
	for (i = 0; i < NMARKS + EXTRA_MARKS; ++i)
	    vim_free(vi_namedfm[i].fname);
	VIM_CLEAR(vi_namedfm);
    }
#ifdef FEAT_JUMPLIST
    if (vi_jumplist != NULL)
    {
	for (i = 0; i < vi_jumplist_len; ++i)
	    vim_free(vi_jumplist[i].fname);
	VIM_CLEAR(vi_jumplist);
    }
#endif
}

/*
 * Accept a new style mark line from the viminfo, store it when it's new.
 */
    void
handle_viminfo_mark(garray_T *values, int force)
{
    bval_T	*vp = (bval_T *)values->ga_data;
    int		name;
    linenr_T	lnum;
    colnr_T	col;
    time_t	timestamp;
    xfmark_T	*fm = NULL;

    /* Check the format:
     * |{bartype},{name},{lnum},{col},{timestamp},{filename} */
    if (values->ga_len < 5
	    || vp[0].bv_type != BVAL_NR
	    || vp[1].bv_type != BVAL_NR
	    || vp[2].bv_type != BVAL_NR
	    || vp[3].bv_type != BVAL_NR
	    || vp[4].bv_type != BVAL_STRING)
	return;

    name = vp[0].bv_nr;
    if (name != '\'' && !VIM_ISDIGIT(name) && !ASCII_ISUPPER(name))
	return;
    lnum = vp[1].bv_nr;
    col = vp[2].bv_nr;
    if (lnum <= 0 || col < 0)
	return;
    timestamp = (time_t)vp[3].bv_nr;

    if (name == '\'')
    {
#ifdef FEAT_JUMPLIST
	if (vi_jumplist != NULL)
	{
	    if (vi_jumplist_len < JUMPLISTSIZE)
		fm = &vi_jumplist[vi_jumplist_len++];
	}
	else
	{
	    int idx;
	    int i;

	    /* If we have a timestamp insert it in the right place. */
	    if (timestamp != 0)
	    {
		for (idx = curwin->w_jumplistlen - 1; idx >= 0; --idx)
		    if (curwin->w_jumplist[idx].time_set < timestamp)
		    {
			++idx;
			break;
		    }
		/* idx cannot be zero now */
		if (idx < 0 && curwin->w_jumplistlen < JUMPLISTSIZE)
		    /* insert as the oldest entry */
		    idx = 0;
	    }
	    else if (curwin->w_jumplistlen < JUMPLISTSIZE)
		/* insert as oldest entry */
		idx = 0;
	    else
		idx = -1;

	    if (idx >= 0)
	    {
		if (curwin->w_jumplistlen == JUMPLISTSIZE)
		{
		    /* Drop the oldest entry. */
		    --idx;
		    vim_free(curwin->w_jumplist[0].fname);
		    for (i = 0; i < idx; ++i)
			curwin->w_jumplist[i] = curwin->w_jumplist[i + 1];
		}
		else
		{
		    /* Move newer entries forward. */
		    for (i = curwin->w_jumplistlen; i > idx; --i)
			curwin->w_jumplist[i] = curwin->w_jumplist[i - 1];
		    ++curwin->w_jumplistidx;
		    ++curwin->w_jumplistlen;
		}
		fm = &curwin->w_jumplist[idx];
		fm->fmark.mark.lnum = 0;
		fm->fname = NULL;
		fm->time_set = 0;
	    }
	}
#endif
    }
    else
    {
	int idx;

	if (VIM_ISDIGIT(name))
	{
	    if (vi_namedfm != NULL)
		idx = name - '0' + NMARKS;
	    else
	    {
		int i;

		/* Do not use the name from the viminfo file, insert in time
		 * order. */
		for (idx = NMARKS; idx < NMARKS + EXTRA_MARKS; ++idx)
		    if (namedfm[idx].time_set < timestamp)
			break;
		if (idx == NMARKS + EXTRA_MARKS)
		    /* All existing entries are newer. */
		    return;
		i = NMARKS + EXTRA_MARKS - 1;

		vim_free(namedfm[i].fname);
		for ( ; i > idx; --i)
		    namedfm[i] = namedfm[i - 1];
		namedfm[idx].fname = NULL;
	    }
	}
	else
	    idx = name - 'A';
	if (vi_namedfm != NULL)
	    fm = &vi_namedfm[idx];
	else
	    fm = &namedfm[idx];
    }

    if (fm != NULL)
    {
	if (vi_namedfm != NULL || fm->fmark.mark.lnum == 0
					  || fm->time_set < timestamp || force)
	{
	    fm->fmark.mark.lnum = lnum;
	    fm->fmark.mark.col = col;
	    fm->fmark.mark.coladd = 0;
	    fm->fmark.fnum = 0;
	    vim_free(fm->fname);
	    if (vp[4].bv_allocated)
	    {
		fm->fname = vp[4].bv_string;
		vp[4].bv_string = NULL;
	    }
	    else
		fm->fname = vim_strsave(vp[4].bv_string);
	    fm->time_set = timestamp;
	}
    }
}

/*
 * Return TRUE if marks for "buf" should not be written.
 */
    static int
skip_for_viminfo(buf_T *buf)
{
    return
#ifdef FEAT_TERMINAL
	    bt_terminal(buf) ||
#endif
	    removable(buf->b_ffname);
}

    void
write_viminfo_filemarks(FILE *fp)
{
    int		i;
    char_u	*name;
    buf_T	*buf;
    xfmark_T	*fm;
    int		vi_idx;
    int		idx;

    if (get_viminfo_parameter('f') == 0)
	return;

    fputs(_("\n# File marks:\n"), fp);

    /* Write the filemarks 'A - 'Z */
    for (i = 0; i < NMARKS; i++)
    {
	if (vi_namedfm != NULL && (vi_namedfm[i].time_set > namedfm[i].time_set
					  || namedfm[i].fmark.mark.lnum == 0))
	    fm = &vi_namedfm[i];
	else
	    fm = &namedfm[i];
	write_one_filemark(fp, fm, '\'', i + 'A');
    }

    /*
     * Find a mark that is the same file and position as the cursor.
     * That one, or else the last one is deleted.
     * Move '0 to '1, '1 to '2, etc. until the matching one or '9
     * Set the '0 mark to current cursor position.
     */
    if (curbuf->b_ffname != NULL && !skip_for_viminfo(curbuf))
    {
	name = buflist_nr2name(curbuf->b_fnum, TRUE, FALSE);
	for (i = NMARKS; i < NMARKS + EXTRA_MARKS - 1; ++i)
	    if (namedfm[i].fmark.mark.lnum == curwin->w_cursor.lnum
		    && (namedfm[i].fname == NULL
			    ? namedfm[i].fmark.fnum == curbuf->b_fnum
			    : (name != NULL
				    && STRCMP(name, namedfm[i].fname) == 0)))
		break;
	vim_free(name);

	vim_free(namedfm[i].fname);
	for ( ; i > NMARKS; --i)
	    namedfm[i] = namedfm[i - 1];
	namedfm[NMARKS].fmark.mark = curwin->w_cursor;
	namedfm[NMARKS].fmark.fnum = curbuf->b_fnum;
	namedfm[NMARKS].fname = NULL;
	namedfm[NMARKS].time_set = vim_time();
    }

    /* Write the filemarks '0 - '9.  Newest (highest timestamp) first. */
    vi_idx = NMARKS;
    idx = NMARKS;
    for (i = NMARKS; i < NMARKS + EXTRA_MARKS; i++)
    {
	xfmark_T *vi_fm = vi_namedfm != NULL ? &vi_namedfm[vi_idx] : NULL;

	if (vi_fm != NULL
		&& vi_fm->fmark.mark.lnum != 0
		&& (vi_fm->time_set > namedfm[idx].time_set
		    || namedfm[idx].fmark.mark.lnum == 0))
	{
	    fm = vi_fm;
	    ++vi_idx;
	}
	else
	{
	    fm = &namedfm[idx++];
	    if (vi_fm != NULL
		  && vi_fm->fmark.mark.lnum == fm->fmark.mark.lnum
		  && vi_fm->time_set == fm->time_set
		  && ((vi_fm->fmark.fnum != 0
			  && vi_fm->fmark.fnum == fm->fmark.fnum)
		      || (vi_fm->fname != NULL
			  && fm->fname != NULL
			  && STRCMP(vi_fm->fname, fm->fname) == 0)))
		++vi_idx;  /* skip duplicate */
	}
	write_one_filemark(fp, fm, '\'', i - NMARKS + '0');
    }

#ifdef FEAT_JUMPLIST
    /* Write the jumplist with -' */
    fputs(_("\n# Jumplist (newest first):\n"), fp);
    setpcmark();	/* add current cursor position */
    cleanup_jumplist(curwin, FALSE);
    vi_idx = 0;
    idx = curwin->w_jumplistlen - 1;
    for (i = 0; i < JUMPLISTSIZE; ++i)
    {
	xfmark_T	*vi_fm;

	fm = idx >= 0 ? &curwin->w_jumplist[idx] : NULL;
	vi_fm = vi_idx < vi_jumplist_len ? &vi_jumplist[vi_idx] : NULL;
	if (fm == NULL && vi_fm == NULL)
	    break;
	if (fm == NULL || (vi_fm != NULL && fm->time_set < vi_fm->time_set))
	{
	    fm = vi_fm;
	    ++vi_idx;
	}
	else
	    --idx;
	if (fm->fmark.fnum == 0
		|| ((buf = buflist_findnr(fm->fmark.fnum)) != NULL
		    && !skip_for_viminfo(buf)))
	    write_one_filemark(fp, fm, '-', '\'');
    }
#endif
}

    static void
write_one_filemark(
    FILE	*fp,
    xfmark_T	*fm,
    int		c1,
    int		c2)
{
    char_u	*name;

    if (fm->fmark.mark.lnum == 0)	/* not set */
	return;

    if (fm->fmark.fnum != 0)		/* there is a buffer */
	name = buflist_nr2name(fm->fmark.fnum, TRUE, FALSE);
    else
	name = fm->fname;		/* use name from .viminfo */
    if (name != NULL && *name != NUL)
    {
	fprintf(fp, "%c%c  %ld  %ld  ", c1, c2, (long)fm->fmark.mark.lnum,
						    (long)fm->fmark.mark.col);
	viminfo_writestring(fp, name);

	/* Barline: |{bartype},{name},{lnum},{col},{timestamp},{filename}
	 * size up to filename: 8 + 3 * 20 */
	fprintf(fp, "|%d,%d,%ld,%ld,%ld,", BARTYPE_MARK, c2,
		(long)fm->fmark.mark.lnum, (long)fm->fmark.mark.col,
		(long)fm->time_set);
	barline_writestring(fp, name, LSIZE - 70);
	putc('\n', fp);
    }

    if (fm->fmark.fnum != 0)
	vim_free(name);
}

/*
 * Return TRUE if "name" is on removable media (depending on 'viminfo').
 */
    int
removable(char_u *name)
{
    char_u  *p;
    char_u  part[51];
    int	    retval = FALSE;
    size_t  n;

    name = home_replace_save(NULL, name);
    if (name != NULL)
    {
	for (p = p_viminfo; *p; )
	{
	    copy_option_part(&p, part, 51, ", ");
	    if (part[0] == 'r')
	    {
		n = STRLEN(part + 1);
		if (MB_STRNICMP(part + 1, name, n) == 0)
		{
		    retval = TRUE;
		    break;
		}
	    }
	}
	vim_free(name);
    }
    return retval;
}

    static void
write_one_mark(FILE *fp_out, int c, pos_T *pos)
{
    if (pos->lnum != 0)
	fprintf(fp_out, "\t%c\t%ld\t%d\n", c, (long)pos->lnum, (int)pos->col);
}


    static void
write_buffer_marks(buf_T *buf, FILE *fp_out)
{
    int		i;
    pos_T	pos;

    home_replace(NULL, buf->b_ffname, IObuff, IOSIZE, TRUE);
    fprintf(fp_out, "\n> ");
    viminfo_writestring(fp_out, IObuff);

    /* Write the last used timestamp as the lnum of the non-existing mark '*'.
     * Older Vims will ignore it and/or copy it. */
    pos.lnum = (linenr_T)buf->b_last_used;
    pos.col = 0;
    write_one_mark(fp_out, '*', &pos);

    write_one_mark(fp_out, '"', &buf->b_last_cursor);
    write_one_mark(fp_out, '^', &buf->b_last_insert);
    write_one_mark(fp_out, '.', &buf->b_last_change);
#ifdef FEAT_JUMPLIST
    /* changelist positions are stored oldest first */
    for (i = 0; i < buf->b_changelistlen; ++i)
    {
	/* skip duplicates */
	if (i == 0 || !EQUAL_POS(buf->b_changelist[i - 1],
							 buf->b_changelist[i]))
	    write_one_mark(fp_out, '+', &buf->b_changelist[i]);
    }
#endif
    for (i = 0; i < NMARKS; i++)
	write_one_mark(fp_out, 'a' + i, &buf->b_namedm[i]);
}

/*
 * Write all the named marks for all buffers.
 * When "buflist" is not NULL fill it with the buffers for which marks are to
 * be written.
 */
    void
write_viminfo_marks(FILE *fp_out, garray_T *buflist)
{
    buf_T	*buf;
    int		is_mark_set;
    int		i;
    win_T	*win;
    tabpage_T	*tp;

    /*
     * Set b_last_cursor for the all buffers that have a window.
     */
    FOR_ALL_TAB_WINDOWS(tp, win)
	set_last_cursor(win);

    fputs(_("\n# History of marks within files (newest to oldest):\n"), fp_out);
    FOR_ALL_BUFFERS(buf)
    {
	/*
	 * Only write something if buffer has been loaded and at least one
	 * mark is set.
	 */
	if (buf->b_marks_read)
	{
	    if (buf->b_last_cursor.lnum != 0)
		is_mark_set = TRUE;
	    else
	    {
		is_mark_set = FALSE;
		for (i = 0; i < NMARKS; i++)
		    if (buf->b_namedm[i].lnum != 0)
		    {
			is_mark_set = TRUE;
			break;
		    }
	    }
	    if (is_mark_set && buf->b_ffname != NULL
		      && buf->b_ffname[0] != NUL
		      && !skip_for_viminfo(buf))
	    {
		if (buflist == NULL)
		    write_buffer_marks(buf, fp_out);
		else if (ga_grow(buflist, 1) == OK)
		    ((buf_T **)buflist->ga_data)[buflist->ga_len++] = buf;
	    }
	}
    }
}

/*
 * Compare functions for qsort() below, that compares b_last_used.
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
buf_compare(const void *s1, const void *s2)
{
    buf_T *buf1 = *(buf_T **)s1;
    buf_T *buf2 = *(buf_T **)s2;

    if (buf1->b_last_used == buf2->b_last_used)
	return 0;
    return buf1->b_last_used > buf2->b_last_used ? -1 : 1;
}

/*
 * Handle marks in the viminfo file:
 * fp_out != NULL: copy marks, in time order with buffers in "buflist".
 * fp_out == NULL && (flags & VIF_WANT_MARKS): read marks for curbuf only
 * fp_out == NULL && (flags & VIF_GET_OLDFILES | VIF_FORCEIT): fill v:oldfiles
 */
    void
copy_viminfo_marks(
    vir_T	*virp,
    FILE	*fp_out,
    garray_T	*buflist,
    int		eof,
    int		flags)
{
    char_u	*line = virp->vir_line;
    buf_T	*buf;
    int		num_marked_files;
    int		load_marks;
    int		copy_marks_out;
    char_u	*str;
    int		i;
    char_u	*p;
    char_u	*name_buf;
    pos_T	pos;
#ifdef FEAT_EVAL
    list_T	*list = NULL;
#endif
    int		count = 0;
    int		buflist_used = 0;
    buf_T	*buflist_buf = NULL;

    if ((name_buf = alloc(LSIZE)) == NULL)
	return;
    *name_buf = NUL;

    if (fp_out != NULL && buflist->ga_len > 0)
    {
	/* Sort the list of buffers on b_last_used. */
	qsort(buflist->ga_data, (size_t)buflist->ga_len,
						sizeof(buf_T *), buf_compare);
	buflist_buf = ((buf_T **)buflist->ga_data)[0];
    }

#ifdef FEAT_EVAL
    if (fp_out == NULL && (flags & (VIF_GET_OLDFILES | VIF_FORCEIT)))
    {
	list = list_alloc();
	if (list != NULL)
	    set_vim_var_list(VV_OLDFILES, list);
    }
#endif

    num_marked_files = get_viminfo_parameter('\'');
    while (!eof && (count < num_marked_files || fp_out == NULL))
    {
	if (line[0] != '>')
	{
	    if (line[0] != '\n' && line[0] != '\r' && line[0] != '#')
	    {
		if (viminfo_error("E576: ", _("Missing '>'"), line))
		    break;	/* too many errors, return now */
	    }
	    eof = vim_fgets(line, LSIZE, virp->vir_fd);
	    continue;		/* Skip this dud line */
	}

	/*
	 * Handle long line and translate escaped characters.
	 * Find file name, set str to start.
	 * Ignore leading and trailing white space.
	 */
	str = skipwhite(line + 1);
	str = viminfo_readstring(virp, (int)(str - virp->vir_line), FALSE);
	if (str == NULL)
	    continue;
	p = str + STRLEN(str);
	while (p != str && (*p == NUL || vim_isspace(*p)))
	    p--;
	if (*p)
	    p++;
	*p = NUL;

#ifdef FEAT_EVAL
	if (list != NULL)
	    list_append_string(list, str, -1);
#endif

	/*
	 * If fp_out == NULL, load marks for current buffer.
	 * If fp_out != NULL, copy marks for buffers not in buflist.
	 */
	load_marks = copy_marks_out = FALSE;
	if (fp_out == NULL)
	{
	    if ((flags & VIF_WANT_MARKS) && curbuf->b_ffname != NULL)
	    {
		if (*name_buf == NUL)	    /* only need to do this once */
		    home_replace(NULL, curbuf->b_ffname, name_buf, LSIZE, TRUE);
		if (fnamecmp(str, name_buf) == 0)
		    load_marks = TRUE;
	    }
	}
	else /* fp_out != NULL */
	{
	    /* This is slow if there are many buffers!! */
	    FOR_ALL_BUFFERS(buf)
		if (buf->b_ffname != NULL)
		{
		    home_replace(NULL, buf->b_ffname, name_buf, LSIZE, TRUE);
		    if (fnamecmp(str, name_buf) == 0)
			break;
		}

	    /*
	     * Copy marks if the buffer has not been loaded.
	     */
	    if (buf == NULL || !buf->b_marks_read)
	    {
		int	did_read_line = FALSE;

		if (buflist_buf != NULL)
		{
		    /* Read the next line.  If it has the "*" mark compare the
		     * time stamps.  Write entries from "buflist" that are
		     * newer. */
		    if (!(eof = viminfo_readline(virp)) && line[0] == TAB)
		    {
			did_read_line = TRUE;
			if (line[1] == '*')
			{
			    long	ltime;

			    sscanf((char *)line + 2, "%ld ", &ltime);
			    while ((time_T)ltime < buflist_buf->b_last_used)
			    {
				write_buffer_marks(buflist_buf, fp_out);
				if (++count >= num_marked_files)
				    break;
				if (++buflist_used == buflist->ga_len)
				{
				    buflist_buf = NULL;
				    break;
				}
				buflist_buf =
				   ((buf_T **)buflist->ga_data)[buflist_used];
			    }
			}
			else
			{
			    /* No timestamp, must be written by an older Vim.
			     * Assume all remaining buffers are older then
			     * ours.  */
			    while (count < num_marked_files
					    && buflist_used < buflist->ga_len)
			    {
				buflist_buf = ((buf_T **)buflist->ga_data)
							     [buflist_used++];
				write_buffer_marks(buflist_buf, fp_out);
				++count;
			    }
			    buflist_buf = NULL;
			}

			if (count >= num_marked_files)
			{
			    vim_free(str);
			    break;
			}
		    }
		}

		fputs("\n> ", fp_out);
		viminfo_writestring(fp_out, str);
		if (did_read_line)
		    fputs((char *)line, fp_out);

		count++;
		copy_marks_out = TRUE;
	    }
	}
	vim_free(str);

	pos.coladd = 0;
	while (!(eof = viminfo_readline(virp)) && line[0] == TAB)
	{
	    if (load_marks)
	    {
		if (line[1] != NUL)
		{
		    unsigned u;

		    sscanf((char *)line + 2, "%ld %u", &pos.lnum, &u);
		    pos.col = u;
		    switch (line[1])
		    {
			case '"': curbuf->b_last_cursor = pos; break;
			case '^': curbuf->b_last_insert = pos; break;
			case '.': curbuf->b_last_change = pos; break;
			case '+':
#ifdef FEAT_JUMPLIST
				  /* changelist positions are stored oldest
				   * first */
				  if (curbuf->b_changelistlen == JUMPLISTSIZE)
				      /* list is full, remove oldest entry */
				      mch_memmove(curbuf->b_changelist,
					    curbuf->b_changelist + 1,
					    sizeof(pos_T) * (JUMPLISTSIZE - 1));
				  else
				      ++curbuf->b_changelistlen;
				  curbuf->b_changelist[
					   curbuf->b_changelistlen - 1] = pos;
#endif
				  break;

				  /* Using the line number for the last-used
				   * timestamp. */
			case '*': curbuf->b_last_used = pos.lnum; break;

			default:  if ((i = line[1] - 'a') >= 0 && i < NMARKS)
				      curbuf->b_namedm[i] = pos;
		    }
		}
	    }
	    else if (copy_marks_out)
		fputs((char *)line, fp_out);
	}

	if (load_marks)
	{
#ifdef FEAT_JUMPLIST
	    win_T	*wp;

	    FOR_ALL_WINDOWS(wp)
	    {
		if (wp->w_buffer == curbuf)
		    wp->w_changelistidx = curbuf->b_changelistlen;
	    }
#endif
	    break;
	}
    }

    if (fp_out != NULL)
	/* Write any remaining entries from buflist. */
	while (count < num_marked_files && buflist_used < buflist->ga_len)
	{
	    buflist_buf = ((buf_T **)buflist->ga_data)[buflist_used++];
	    write_buffer_marks(buflist_buf, fp_out);
	    ++count;
	}

    vim_free(name_buf);
}
#endif /* FEAT_VIMINFO */
