/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * undo.c: multi level undo facility
 *
 * The saved lines are stored in a list of lists (one for each buffer):
 *
 * b_u_oldhead------------------------------------------------+
 *							      |
 *							      V
 *		  +--------------+    +--------------+	  +--------------+
 * b_u_newhead--->| u_header	 |    | u_header     |	  | u_header	 |
 *		  |	uh_next------>|     uh_next------>|	uh_next---->NULL
 *	   NULL<--------uh_prev  |<---------uh_prev  |<---------uh_prev  |
 *		  |	uh_entry |    |     uh_entry |	  |	uh_entry |
 *		  +--------|-----+    +--------|-----+	  +--------|-----+
 *			   |		       |		   |
 *			   V		       V		   V
 *		  +--------------+    +--------------+	  +--------------+
 *		  | u_entry	 |    | u_entry      |	  | u_entry	 |
 *		  |	ue_next  |    |     ue_next  |	  |	ue_next  |
 *		  +--------|-----+    +--------|-----+	  +--------|-----+
 *			   |		       |		   |
 *			   V		       V		   V
 *		  +--------------+	      NULL		  NULL
 *		  | u_entry	 |
 *		  |	ue_next  |
 *		  +--------|-----+
 *			   |
 *			   V
 *			  etc.
 *
 * Each u_entry list contains the information for one undo or redo.
 * curbuf->b_u_curhead points to the header of the last undo (the next redo),
 * or is NULL if nothing has been undone.
 *
 * All data is allocated with u_alloc_line(), thus it will be freed as soon as
 * we switch files!
 */

#include "vim.h"

static u_entry_T *u_get_headentry __ARGS((void));
static void u_getbot __ARGS((void));
static int u_savecommon __ARGS((linenr_T, linenr_T, linenr_T));
static void u_doit __ARGS((int count));
static void u_undoredo __ARGS((void));
static void u_undo_end __ARGS((void));
static void u_freelist __ARGS((struct u_header *));
static void u_freeentry __ARGS((u_entry_T *, long));

static char_u *u_blockalloc __ARGS((long_u));
static void u_free_line __ARGS((char_u *, int keep));
static char_u *u_alloc_line __ARGS((unsigned));
static char_u *u_save_line __ARGS((linenr_T));

static long	u_newcount, u_oldcount;

/*
 * When 'u' flag included in 'cpoptions', we behave like vi.  Need to remember
 * the action that "u" should do.
 */
static int	undo_undoes = FALSE;

/*
 * save the current line for both the "u" and "U" command
 */
    int
u_save_cursor()
{
    return (u_save((linenr_T)(curwin->w_cursor.lnum - 1),
				      (linenr_T)(curwin->w_cursor.lnum + 1)));
}

/*
 * Save the lines between "top" and "bot" for both the "u" and "U" command.
 * "top" may be 0 and bot may be curbuf->b_ml.ml_line_count + 1.
 * Returns FAIL when lines could not be saved, OK otherwise.
 */
    int
u_save(top, bot)
    linenr_T top, bot;
{
    if (undo_off)
	return OK;

    if (top > curbuf->b_ml.ml_line_count ||
			    top >= bot || bot > curbuf->b_ml.ml_line_count + 1)
	return FALSE;	/* rely on caller to do error messages */

    if (top + 2 == bot)
	u_saveline((linenr_T)(top + 1));

    return (u_savecommon(top, bot, (linenr_T)0));
}

/*
 * save the line "lnum" (used by ":s" and "~" command)
 * The line is replaced, so the new bottom line is lnum + 1.
 */
    int
u_savesub(lnum)
    linenr_T	lnum;
{
    if (undo_off)
	return OK;

    return (u_savecommon(lnum - 1, lnum + 1, lnum + 1));
}

/*
 * a new line is inserted before line "lnum" (used by :s command)
 * The line is inserted, so the new bottom line is lnum + 1.
 */
    int
u_inssub(lnum)
    linenr_T	lnum;
{
    if (undo_off)
	return OK;

    return (u_savecommon(lnum - 1, lnum, lnum + 1));
}

/*
 * save the lines "lnum" - "lnum" + nlines (used by delete command)
 * The lines are deleted, so the new bottom line is lnum, unless the buffer
 * becomes empty.
 */
    int
u_savedel(lnum, nlines)
    linenr_T	lnum;
    long	nlines;
{
    if (undo_off)
	return OK;

    return (u_savecommon(lnum - 1, lnum + nlines,
			nlines == curbuf->b_ml.ml_line_count ? 2 : lnum));
}

    static int
u_savecommon(top, bot, newbot)
    linenr_T	top, bot;
    linenr_T	newbot;
{
    linenr_T		lnum;
    long		i;
    struct u_header	*uhp;
    u_entry_T		*uep;
    u_entry_T		*prev_uep;
    long		size;

    /*
     * Don't allow changes when 'modifiable' is off.  Letting the
     * undo fail is a crude way to make all change commands fail.
     */
    if (!curbuf->b_p_ma)
    {
	EMSG(_(e_modifiable));
	return FAIL;
    }

#ifdef HAVE_SANDBOX
    /*
     * In the sandbox it's not allowed to change the text.  Letting the
     * undo fail is a crude way to make all change commands fail.
     */
    if (sandbox != 0)
    {
	EMSG(_(e_sandbox));
	return FAIL;
    }
#endif

#ifdef FEAT_NETBEANS_INTG
    /*
     * Netbeans defines areas that cannot be modified.  Bail out here when
     * trying to change text in a guarded area.
     */
    if (usingNetbeans && netbeans_is_guarded(top, bot))
    {
	EMSG(_(e_guarded));
	return FAIL;
    }
#endif

#ifdef FEAT_AUTOCMD
    /*
     * Saving text for undo means we are going to make a change.  Give a
     * warning for a read-only file before making the change, so that the
     * FileChangedRO event can replace the buffer with a read-write version
     * (e.g., obtained from a source control system).
     */
    change_warning(0);
#endif

    size = bot - top - 1;

    /*
     * if curbuf->b_u_synced == TRUE make a new header
     */
    if (curbuf->b_u_synced)
    {
#ifdef FEAT_JUMPLIST
	/* Need to create new entry in b_changelist. */
	curbuf->b_new_change = TRUE;
#endif

	/*
	 * if we undid more than we redid, free the entry lists before and
	 * including curbuf->b_u_curhead
	 */
	while (curbuf->b_u_curhead != NULL)
	    u_freelist(curbuf->b_u_newhead);

	/*
	 * free headers to keep the size right
	 */
	while (curbuf->b_u_numhead > p_ul && curbuf->b_u_oldhead != NULL)
	    u_freelist(curbuf->b_u_oldhead);

	if (p_ul < 0)		/* no undo at all */
	{
	    curbuf->b_u_synced = FALSE;
	    return OK;
	}

	/*
	 * make a new header entry
	 */
	uhp = (struct u_header *)u_alloc_line((unsigned)
						     sizeof(struct u_header));
	if (uhp == NULL)
	    goto nomem;
	uhp->uh_prev = NULL;
	uhp->uh_next = curbuf->b_u_newhead;
	if (curbuf->b_u_newhead != NULL)
	    curbuf->b_u_newhead->uh_prev = uhp;
	uhp->uh_entry = NULL;
	uhp->uh_getbot_entry = NULL;
	uhp->uh_cursor = curwin->w_cursor;	/* save cursor pos. for undo */
#ifdef FEAT_VIRTUALEDIT
	if (virtual_active() && curwin->w_cursor.coladd > 0)
	    uhp->uh_cursor_vcol = getviscol();
	else
	    uhp->uh_cursor_vcol = -1;
#endif

	/* save changed and buffer empty flag for undo */
	uhp->uh_flags = (curbuf->b_changed ? UH_CHANGED : 0) +
		       ((curbuf->b_ml.ml_flags & ML_EMPTY) ? UH_EMPTYBUF : 0);

	/* save named marks for undo */
	mch_memmove(uhp->uh_namedm, curbuf->b_namedm, sizeof(pos_T) * NMARKS);
	curbuf->b_u_newhead = uhp;
	if (curbuf->b_u_oldhead == NULL)
	    curbuf->b_u_oldhead = uhp;
	++curbuf->b_u_numhead;
    }
    else
    {
	if (p_ul < 0)		/* no undo at all */
	    return OK;

	/*
	 * When saving a single line, and it has been saved just before, it
	 * doesn't make sense saving it again.  Saves a lot of memory when
	 * making lots of changes inside the same line.
	 * This is only possible if the previous change didn't increase or
	 * decrease the number of lines.
	 * Check the ten last changes.  More doesn't make sense and takes too
	 * long.
	 */
	if (size == 1)
	{
	    uep = u_get_headentry();
	    prev_uep = NULL;
	    for (i = 0; i < 10; ++i)
	    {
		if (uep == NULL)
		    break;

		/* If lines have been inserted/deleted we give up.
		 * Also when the line was included in a multi-line save. */
		if ((curbuf->b_u_newhead->uh_getbot_entry != uep
			    ? (uep->ue_top + uep->ue_size + 1
				!= (uep->ue_bot == 0
				    ? curbuf->b_ml.ml_line_count + 1
				    : uep->ue_bot))
			    : uep->ue_lcount != curbuf->b_ml.ml_line_count)
			|| (uep->ue_size > 1
			    && top >= uep->ue_top
			    && top + 2 <= uep->ue_top + uep->ue_size + 1))
		    break;

		/* If it's the same line we can skip saving it again. */
		if (uep->ue_size == 1 && uep->ue_top == top)
		{
		    if (i > 0)
		    {
			/* It's not the last entry: get ue_bot for the last
			 * entry now.  Following deleted/inserted lines go to
			 * the re-used entry. */
			u_getbot();
			curbuf->b_u_synced = FALSE;

			/* Move the found entry to become the last entry.  The
			 * order of undo/redo doesn't matter for the entries
			 * we move it over, since they don't change the line
			 * count and don't include this line.  It does matter
			 * for the found entry if the line count is changed by
			 * the executed command. */
			prev_uep->ue_next = uep->ue_next;
			uep->ue_next = curbuf->b_u_newhead->uh_entry;
			curbuf->b_u_newhead->uh_entry = uep;
		    }

		    /* The executed command may change the line count. */
		    if (newbot != 0)
			uep->ue_bot = newbot;
		    else if (bot > curbuf->b_ml.ml_line_count)
			uep->ue_bot = 0;
		    else
		    {
			uep->ue_lcount = curbuf->b_ml.ml_line_count;
			curbuf->b_u_newhead->uh_getbot_entry = uep;
		    }
		    return OK;
		}
		prev_uep = uep;
		uep = uep->ue_next;
	    }
	}

	/* find line number for ue_bot for previous u_save() */
	u_getbot();
    }

#if !defined(UNIX) && !defined(DJGPP) && !defined(WIN32) && !defined(__EMX__)
	/*
	 * With Amiga and MSDOS 16 bit we can't handle big undo's, because
	 * then u_alloc_line would have to allocate a block larger than 32K
	 */
    if (size >= 8000)
	goto nomem;
#endif

    /*
     * add lines in front of entry list
     */
    uep = (u_entry_T *)u_alloc_line((unsigned)sizeof(u_entry_T));
    if (uep == NULL)
	goto nomem;

    uep->ue_size = size;
    uep->ue_top = top;
    if (newbot != 0)
	uep->ue_bot = newbot;
    /*
     * Use 0 for ue_bot if bot is below last line.
     * Otherwise we have to compute ue_bot later.
     */
    else if (bot > curbuf->b_ml.ml_line_count)
	uep->ue_bot = 0;
    else
    {
	uep->ue_lcount = curbuf->b_ml.ml_line_count;
	curbuf->b_u_newhead->uh_getbot_entry = uep;
    }

    if (size)
    {
	if ((uep->ue_array = (char_u **)u_alloc_line(
				(unsigned)(sizeof(char_u *) * size))) == NULL)
	{
	    u_freeentry(uep, 0L);
	    goto nomem;
	}
	for (i = 0, lnum = top + 1; i < size; ++i)
	{
	    if ((uep->ue_array[i] = u_save_line(lnum++)) == NULL)
	    {
		u_freeentry(uep, i);
		goto nomem;
	    }
	}
    }
    uep->ue_next = curbuf->b_u_newhead->uh_entry;
    curbuf->b_u_newhead->uh_entry = uep;
    curbuf->b_u_synced = FALSE;
    undo_undoes = FALSE;

    return OK;

nomem:
    msg_silent = 0;	/* must display the prompt */
    if (ask_yesno((char_u *)_("No undo possible; continue anyway"), TRUE)
								       == 'y')
    {
	undo_off = TRUE;	    /* will be reset when character typed */
	return OK;
    }
    do_outofmem_msg((long_u)0);
    return FAIL;
}

/*
 * If 'cpoptions' contains 'u': Undo the previous undo or redo (vi compatible).
 * If 'cpoptions' does not contain 'u': Always undo.
 */
    void
u_undo(count)
    int count;
{
    /*
     * If we get an undo command while executing a macro, we behave like the
     * original vi. If this happens twice in one macro the result will not
     * be compatible.
     */
    if (curbuf->b_u_synced == FALSE)
    {
	u_sync();
	count = 1;
    }

    if (vim_strchr(p_cpo, CPO_UNDO) == NULL)
	undo_undoes = TRUE;
    else
	undo_undoes = !undo_undoes;
    u_doit(count);
}

/*
 * If 'cpoptions' contains 'u': Repeat the previous undo or redo.
 * If 'cpoptions' does not contain 'u': Always redo.
 */
    void
u_redo(count)
    int count;
{
    if (vim_strchr(p_cpo, CPO_UNDO) == NULL)
	undo_undoes = FALSE;
    u_doit(count);
}

/*
 * Undo or redo, depending on 'undo_undoes', 'count' times.
 */
    static void
u_doit(count)
    int count;
{
    /* Don't allow changes when 'modifiable' is off. */
    if (!curbuf->b_p_ma)
    {
	EMSG(_(e_modifiable));
	return;
    }
#ifdef HAVE_SANDBOX
    /* In the sandbox it's not allowed to change the text. */
    if (sandbox != 0)
    {
	EMSG(_(e_sandbox));
	return;
    }
#endif

    u_newcount = 0;
    u_oldcount = 0;
    while (count--)
    {
	if (undo_undoes)
	{
	    if (curbuf->b_u_curhead == NULL)		/* first undo */
		curbuf->b_u_curhead = curbuf->b_u_newhead;
	    else if (p_ul > 0)				/* multi level undo */
		/* get next undo */
		curbuf->b_u_curhead = curbuf->b_u_curhead->uh_next;
	    /* nothing to undo */
	    if (curbuf->b_u_numhead == 0 || curbuf->b_u_curhead == NULL)
	    {
		/* stick curbuf->b_u_curhead at end */
		curbuf->b_u_curhead = curbuf->b_u_oldhead;
		beep_flush();
		break;
	    }

	    u_undoredo();
	}
	else
	{
	    if (curbuf->b_u_curhead == NULL || p_ul <= 0)
	    {
		beep_flush();	/* nothing to redo */
		break;
	    }

	    u_undoredo();
	    /* advance for next redo */
	    curbuf->b_u_curhead = curbuf->b_u_curhead->uh_prev;
	}
    }
    u_undo_end();
}

/*
 * u_undoredo: common code for undo and redo
 *
 * The lines in the file are replaced by the lines in the entry list at
 * curbuf->b_u_curhead. The replaced lines in the file are saved in the entry
 * list for the next undo/redo.
 */
    static void
u_undoredo()
{
    char_u	**newarray = NULL;
    linenr_T	oldsize;
    linenr_T	newsize;
    linenr_T	top, bot;
    linenr_T	lnum;
    linenr_T	newlnum = MAXLNUM;
    long	i;
    u_entry_T	*uep, *nuep;
    u_entry_T	*newlist = NULL;
    int		old_flags;
    int		new_flags;
    pos_T	namedm[NMARKS];
    int		empty_buffer;		    /* buffer became empty */

    old_flags = curbuf->b_u_curhead->uh_flags;
    new_flags = (curbuf->b_changed ? UH_CHANGED : 0) +
	       ((curbuf->b_ml.ml_flags & ML_EMPTY) ? UH_EMPTYBUF : 0);
    setpcmark();

    /*
     * save marks before undo/redo
     */
    mch_memmove(namedm, curbuf->b_namedm, sizeof(pos_T) * NMARKS);
    curbuf->b_op_start.lnum = curbuf->b_ml.ml_line_count;
    curbuf->b_op_start.col = 0;
    curbuf->b_op_end.lnum = 0;
    curbuf->b_op_end.col = 0;

    for (uep = curbuf->b_u_curhead->uh_entry; uep != NULL; uep = nuep)
    {
	top = uep->ue_top;
	bot = uep->ue_bot;
	if (bot == 0)
	    bot = curbuf->b_ml.ml_line_count + 1;
	if (top > curbuf->b_ml.ml_line_count || top >= bot
				      || bot > curbuf->b_ml.ml_line_count + 1)
	{
	    EMSG(_("E438: u_undo: line numbers wrong"));
	    changed();		/* don't want UNCHANGED now */
	    return;
	}

	oldsize = bot - top - 1;    /* number of lines before undo */
	newsize = uep->ue_size;	    /* number of lines after undo */

	if (top < newlnum)
	{
	    /* If the saved cursor is somewhere in this undo block, move it to
	     * the remembered position.  Makes "gwap" put the cursor back
	     * where it was. */
	    lnum = curbuf->b_u_curhead->uh_cursor.lnum;
	    if (lnum >= top && lnum <= top + newsize + 1)
	    {
		curwin->w_cursor = curbuf->b_u_curhead->uh_cursor;
		newlnum = curwin->w_cursor.lnum - 1;
	    }
	    else
	    {
		/* Use the first line that actually changed.  Avoids that
		 * undoing auto-formatting puts the cursor in the previous
		 * line. */
		for (i = 0; i < newsize && i < oldsize; ++i)
		    if (STRCMP(uep->ue_array[i], ml_get(top + 1 + i)) != 0)
			break;
		if (i == newsize && newlnum == MAXLNUM && uep->ue_next == NULL)
		{
		    newlnum = top;
		    curwin->w_cursor.lnum = newlnum + 1;
		}
		else if (i < newsize)
		{
		    newlnum = top + i;
		    curwin->w_cursor.lnum = newlnum + 1;
		}
	    }
	}

	empty_buffer = FALSE;

	/* delete the lines between top and bot and save them in newarray */
	if (oldsize)
	{
	    if ((newarray = (char_u **)u_alloc_line(
			    (unsigned)(sizeof(char_u *) * oldsize))) == NULL)
	    {
		do_outofmem_msg((long_u)(sizeof(char_u *) * oldsize));
		/*
		 * We have messed up the entry list, repair is impossible.
		 * we have to free the rest of the list.
		 */
		while (uep != NULL)
		{
		    nuep = uep->ue_next;
		    u_freeentry(uep, uep->ue_size);
		    uep = nuep;
		}
		break;
	    }
	    /* delete backwards, it goes faster in most cases */
	    for (lnum = bot - 1, i = oldsize; --i >= 0; --lnum)
	    {
		/* what can we do when we run out of memory? */
		if ((newarray[i] = u_save_line(lnum)) == NULL)
		    do_outofmem_msg((long_u)0);
		/* remember we deleted the last line in the buffer, and a
		 * dummy empty line will be inserted */
		if (curbuf->b_ml.ml_line_count == 1)
		    empty_buffer = TRUE;
		ml_delete(lnum, FALSE);
	    }
	}

	/* insert the lines in u_array between top and bot */
	if (newsize)
	{
	    for (lnum = top, i = 0; i < newsize; ++i, ++lnum)
	    {
		/*
		 * If the file is empty, there is an empty line 1 that we
		 * should get rid of, by replacing it with the new line
		 */
		if (empty_buffer && lnum == 0)
		    ml_replace((linenr_T)1, uep->ue_array[i], TRUE);
		else
		    ml_append(lnum, uep->ue_array[i], (colnr_T)0, FALSE);
		u_free_line(uep->ue_array[i], FALSE);
	    }
	    u_free_line((char_u *)uep->ue_array, FALSE);
	}

	/* adjust marks */
	if (oldsize != newsize)
	{
	    mark_adjust(top + 1, top + oldsize, (long)MAXLNUM,
					       (long)newsize - (long)oldsize);
	    if (curbuf->b_op_start.lnum > top + oldsize)
		curbuf->b_op_start.lnum += newsize - oldsize;
	    if (curbuf->b_op_end.lnum > top + oldsize)
		curbuf->b_op_end.lnum += newsize - oldsize;
	}

	changed_lines(top + 1, 0, bot, newsize - oldsize);

	/* set '[ and '] mark */
	if (top + 1 < curbuf->b_op_start.lnum)
	    curbuf->b_op_start.lnum = top + 1;
	if (newsize == 0 && top + 1 > curbuf->b_op_end.lnum)
	    curbuf->b_op_end.lnum = top + 1;
	else if (top + newsize > curbuf->b_op_end.lnum)
	    curbuf->b_op_end.lnum = top + newsize;

	u_newcount += newsize;
	u_oldcount += oldsize;
	uep->ue_size = oldsize;
	uep->ue_array = newarray;
	uep->ue_bot = top + newsize + 1;

	/*
	 * insert this entry in front of the new entry list
	 */
	nuep = uep->ue_next;
	uep->ue_next = newlist;
	newlist = uep;
    }

    curbuf->b_u_curhead->uh_entry = newlist;
    curbuf->b_u_curhead->uh_flags = new_flags;
    if ((old_flags & UH_EMPTYBUF) && bufempty())
	curbuf->b_ml.ml_flags |= ML_EMPTY;
    if (old_flags & UH_CHANGED)
	changed();
    else
	unchanged(curbuf, FALSE);

    /*
     * restore marks from before undo/redo
     */
    for (i = 0; i < NMARKS; ++i)
	if (curbuf->b_u_curhead->uh_namedm[i].lnum)
	{
	    curbuf->b_namedm[i] = curbuf->b_u_curhead->uh_namedm[i];
	    curbuf->b_u_curhead->uh_namedm[i] = namedm[i];
	}

    /*
     * If the cursor is only off by one line, put it at the same position as
     * before starting the change (for the "o" command).
     * Otherwise the cursor should go to the first undone line.
     */
    if (curbuf->b_u_curhead->uh_cursor.lnum + 1 == curwin->w_cursor.lnum
						 && curwin->w_cursor.lnum > 1)
	--curwin->w_cursor.lnum;
    if (curbuf->b_u_curhead->uh_cursor.lnum == curwin->w_cursor.lnum)
    {
	curwin->w_cursor.col = curbuf->b_u_curhead->uh_cursor.col;
#ifdef FEAT_VIRTUALEDIT
	if (virtual_active() && curbuf->b_u_curhead->uh_cursor_vcol >= 0)
	    coladvance((colnr_T)curbuf->b_u_curhead->uh_cursor_vcol);
	else
	    curwin->w_cursor.coladd = 0;
#endif
    }
    else if (curwin->w_cursor.lnum <= curbuf->b_ml.ml_line_count)
	beginline(BL_SOL | BL_FIX);
    else
    {
	/* We get here with the current cursor line being past the end (eg
	 * after adding lines at the end of the file, and then undoing it).
	 * check_cursor() will move the cursor to the last line.  Move it to
	 * the first column here. */
	curwin->w_cursor.col = 0;
#ifdef FEAT_VIRTUALEDIT
	curwin->w_cursor.coladd = 0;
#endif
    }

    /* Make sure the cursor is on an existing line and column. */
    check_cursor();
}

/*
 * If we deleted or added lines, report the number of less/more lines.
 * Otherwise, report the number of changes (this may be incorrect
 * in some cases, but it's better than nothing).
 */
    static void
u_undo_end()
{
    if ((u_oldcount -= u_newcount) != 0)
	msgmore(-u_oldcount);
    else if (u_newcount > p_report)
    {
	if (u_newcount == 1)
	    MSG(_("1 change"));
	else
	    smsg((char_u *)_("%ld changes"), u_newcount);
    }
#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_UNDO) && KeyTyped)
	foldOpenCursor();
#endif
}

/*
 * u_sync: stop adding to the current entry list
 */
    void
u_sync()
{
    if (curbuf->b_u_synced)
	return;		    /* already synced */
#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
    if (im_is_preediting())
	return;		    /* XIM is busy, don't break an undo sequence */
#endif
    if (p_ul < 0)
	curbuf->b_u_synced = TRUE;  /* no entries, nothing to do */
    else
    {
	u_getbot();		    /* compute ue_bot of previous u_save */
	curbuf->b_u_curhead = NULL;
    }
}

/*
 * Called after writing the file and setting b_changed to FALSE.
 * Now an undo means that the buffer is modified.
 */
    void
u_unchanged(buf)
    buf_T	*buf;
{
    struct u_header	*uh;

    for (uh = buf->b_u_newhead; uh; uh = uh->uh_next)
	uh->uh_flags |= UH_CHANGED;
    buf->b_did_warn = FALSE;
}

/*
 * Get pointer to last added entry.
 * If it's not valid, give an error message and return NULL.
 */
    static u_entry_T *
u_get_headentry()
{
    if (curbuf->b_u_newhead == NULL || curbuf->b_u_newhead->uh_entry == NULL)
    {
	EMSG(_("E439: undo list corrupt"));
	return NULL;
    }
    return curbuf->b_u_newhead->uh_entry;
}

/*
 * u_getbot(): compute the line number of the previous u_save
 *		It is called only when b_u_synced is FALSE.
 */
    static void
u_getbot()
{
    u_entry_T	*uep;
    linenr_T	extra;

    uep = u_get_headentry();	/* check for corrupt undo list */
    if (uep == NULL)
	return;

    uep = curbuf->b_u_newhead->uh_getbot_entry;
    if (uep != NULL)
    {
	/*
	 * the new ue_bot is computed from the number of lines that has been
	 * inserted (0 - deleted) since calling u_save. This is equal to the
	 * old line count subtracted from the current line count.
	 */
	extra = curbuf->b_ml.ml_line_count - uep->ue_lcount;
	uep->ue_bot = uep->ue_top + uep->ue_size + 1 + extra;
	if (uep->ue_bot < 1 || uep->ue_bot > curbuf->b_ml.ml_line_count)
	{
	    EMSG(_("E440: undo line missing"));
	    uep->ue_bot = uep->ue_top + 1;  /* assume all lines deleted, will
					     * get all the old lines back
					     * without deleting the current
					     * ones */
	}

	curbuf->b_u_newhead->uh_getbot_entry = NULL;
    }

    curbuf->b_u_synced = TRUE;
}

/*
 * u_freelist: free one entry list and adjust the pointers
 */
    static void
u_freelist(uhp)
    struct u_header *uhp;
{
    u_entry_T	*uep, *nuep;

    for (uep = uhp->uh_entry; uep != NULL; uep = nuep)
    {
	nuep = uep->ue_next;
	u_freeentry(uep, uep->ue_size);
    }

    if (curbuf->b_u_curhead == uhp)
	curbuf->b_u_curhead = NULL;

    if (uhp->uh_next == NULL)
	curbuf->b_u_oldhead = uhp->uh_prev;
    else
	uhp->uh_next->uh_prev = uhp->uh_prev;

    if (uhp->uh_prev == NULL)
	curbuf->b_u_newhead = uhp->uh_next;
    else
	uhp->uh_prev->uh_next = uhp->uh_next;

    u_free_line((char_u *)uhp, FALSE);
    --curbuf->b_u_numhead;
}

/*
 * free entry 'uep' and 'n' lines in uep->ue_array[]
 */
    static void
u_freeentry(uep, n)
    u_entry_T	*uep;
    long	    n;
{
    while (n)
	u_free_line(uep->ue_array[--n], FALSE);
    u_free_line((char_u *)uep, FALSE);
}

/*
 * invalidate the undo buffer; called when storage has already been released
 */
    void
u_clearall(buf)
    buf_T	*buf;
{
    buf->b_u_newhead = buf->b_u_oldhead = buf->b_u_curhead = NULL;
    buf->b_u_synced = TRUE;
    buf->b_u_numhead = 0;
    buf->b_u_line_ptr = NULL;
    buf->b_u_line_lnum = 0;
}

/*
 * save the line "lnum" for the "U" command
 */
    void
u_saveline(lnum)
    linenr_T lnum;
{
    if (lnum == curbuf->b_u_line_lnum)	    /* line is already saved */
	return;
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count)	/* should never happen */
	return;
    u_clearline();
    curbuf->b_u_line_lnum = lnum;
    if (curwin->w_cursor.lnum == lnum)
	curbuf->b_u_line_colnr = curwin->w_cursor.col;
    else
	curbuf->b_u_line_colnr = 0;
    if ((curbuf->b_u_line_ptr = u_save_line(lnum)) == NULL)
	do_outofmem_msg((long_u)0);
}

/*
 * clear the line saved for the "U" command
 * (this is used externally for crossing a line while in insert mode)
 */
    void
u_clearline()
{
    if (curbuf->b_u_line_ptr != NULL)
    {
	u_free_line(curbuf->b_u_line_ptr, FALSE);
	curbuf->b_u_line_ptr = NULL;
	curbuf->b_u_line_lnum = 0;
    }
}

/*
 * Implementation of the "U" command.
 * Differentiation from vi: "U" can be undone with the next "U".
 * We also allow the cursor to be in another line.
 */
    void
u_undoline()
{
    colnr_T t;
    char_u  *oldp;

    if (undo_off)
	return;

    if (curbuf->b_u_line_ptr == NULL ||
			curbuf->b_u_line_lnum > curbuf->b_ml.ml_line_count)
    {
	beep_flush();
	return;
    }
	/* first save the line for the 'u' command */
    if (u_savecommon(curbuf->b_u_line_lnum - 1,
				curbuf->b_u_line_lnum + 1, (linenr_T)0) == FAIL)
	return;
    oldp = u_save_line(curbuf->b_u_line_lnum);
    if (oldp == NULL)
    {
	do_outofmem_msg((long_u)0);
	return;
    }
    ml_replace(curbuf->b_u_line_lnum, curbuf->b_u_line_ptr, TRUE);
    changed_bytes(curbuf->b_u_line_lnum, 0);
    u_free_line(curbuf->b_u_line_ptr, FALSE);
    curbuf->b_u_line_ptr = oldp;

    t = curbuf->b_u_line_colnr;
    if (curwin->w_cursor.lnum == curbuf->b_u_line_lnum)
	curbuf->b_u_line_colnr = curwin->w_cursor.col;
    curwin->w_cursor.col = t;
    curwin->w_cursor.lnum = curbuf->b_u_line_lnum;
}

/*
 * storage allocation for the undo lines and blocks of the current file
 */

/*
 * Memory is allocated in relatively large blocks. These blocks are linked
 * in the allocated block list, headed by curbuf->b_block_head. They are all
 * freed when abandoning a file, so we don't have to free every single line.
 * The list is kept sorted on memory address.
 * block_alloc() allocates a block.
 * m_blockfree() frees all blocks.
 *
 * The available chunks of memory are kept in free chunk lists. There is
 * one free list for each block of allocated memory. The list is kept sorted
 * on memory address.
 * u_alloc_line() gets a chunk from the free lists.
 * u_free_line() returns a chunk to the free lists.
 * curbuf->b_m_search points to the chunk before the chunk that was
 * freed/allocated the last time.
 * curbuf->b_mb_current points to the b_head where curbuf->b_m_search
 * points into the free list.
 *
 *
 *  b_block_head     /---> block #1	/---> block #2
 *	 mb_next ---/	    mb_next ---/       mb_next ---> NULL
 *	 mb_info	    mb_info	       mb_info
 *	    |		       |		  |
 *	    V		       V		  V
 *	  NULL		free chunk #1.1      free chunk #2.1
 *			       |		  |
 *			       V		  V
 *			free chunk #1.2		 NULL
 *			       |
 *			       V
 *			      NULL
 *
 * When a single free chunk list would have been used, it could take a lot
 * of time in u_free_line() to find the correct place to insert a chunk in the
 * free list. The single free list would become very long when many lines are
 * changed (e.g. with :%s/^M$//).
 */

 /*
  * this blocksize is used when allocating new lines
  */
#define MEMBLOCKSIZE 2044

/*
 * The size field contains the size of the chunk, including the size field
 * itself.
 *
 * When the chunk is not in-use it is preceded with the m_info structure.
 * The m_next field links it in one of the free chunk lists.
 *
 * On most unix systems structures have to be longword (32 or 64 bit) aligned.
 * On most other systems they are short (16 bit) aligned.
 */

/* the structure definitions are now in structs.h */

#ifdef ALIGN_LONG
    /* size of m_size */
# define M_OFFSET (sizeof(long_u))
#else
    /* size of m_size */
# define M_OFFSET (sizeof(short_u))
#endif

/*
 * Allocate a block of memory and link it in the allocated block list.
 */
    static char_u *
u_blockalloc(size)
    long_u	size;
{
    mblock_T	*p;
    mblock_T	*mp, *next;

    p = (mblock_T *)lalloc(size + sizeof(mblock_T), FALSE);
    if (p != NULL)
    {
	 /* Insert the block into the allocated block list, keeping it
		    sorted on address. */
	for (mp = &curbuf->b_block_head;
		(next = mp->mb_next) != NULL && next < p;
			mp = next)
	    ;
	p->mb_next = next;		/* link in block list */
	p->mb_size = size;
	mp->mb_next = p;
	p->mb_info.m_next = NULL;	/* clear free list */
	p->mb_info.m_size = 0;
	curbuf->b_mb_current = p;	/* remember current block */
	curbuf->b_m_search = NULL;
	p++;				/* return usable memory */
    }
    return (char_u *)p;
}

/*
 * free all allocated memory blocks for the buffer 'buf'
 */
    void
u_blockfree(buf)
    buf_T	*buf;
{
    mblock_T	*p, *np;

    for (p = buf->b_block_head.mb_next; p != NULL; p = np)
    {
	np = p->mb_next;
	vim_free(p);
    }
    buf->b_block_head.mb_next = NULL;
    buf->b_m_search = NULL;
    buf->b_mb_current = NULL;
}

/*
 * Free a chunk of memory for the current buffer.
 * Insert the chunk into the correct free list, keeping it sorted on address.
 */
    static void
u_free_line(ptr, keep)
    char_u	*ptr;
    int		keep;	/* don't free the block when it's empty */
{
    minfo_T	*next;
    minfo_T	*prev, *curr;
    minfo_T	*mp;
    mblock_T	*nextb;
    mblock_T	*prevb;

    if (ptr == NULL || ptr == IObuff)
	return;	/* illegal address can happen in out-of-memory situations */

    mp = (minfo_T *)(ptr - M_OFFSET);

    /* find block where chunk could be a part off */
    /* if we change curbuf->b_mb_current, curbuf->b_m_search is set to NULL */
    if (curbuf->b_mb_current == NULL || mp < (minfo_T *)curbuf->b_mb_current)
    {
	curbuf->b_mb_current = curbuf->b_block_head.mb_next;
	curbuf->b_m_search = NULL;
    }
    if ((nextb = curbuf->b_mb_current->mb_next) != NULL
						     && (minfo_T *)nextb < mp)
    {
	curbuf->b_mb_current = nextb;
	curbuf->b_m_search = NULL;
    }
    while ((nextb = curbuf->b_mb_current->mb_next) != NULL
						     && (minfo_T *)nextb < mp)
	curbuf->b_mb_current = nextb;

    curr = NULL;
    /*
     * If mp is smaller than curbuf->b_m_search->m_next go to the start of
     * the free list
     */
    if (curbuf->b_m_search == NULL || mp < (curbuf->b_m_search->m_next))
	next = &(curbuf->b_mb_current->mb_info);
    else
	next = curbuf->b_m_search;
    /*
     * The following loop is executed very often.
     * Therefore it has been optimized at the cost of readability.
     * Keep it fast!
     */
#ifdef SLOW_BUT_EASY_TO_READ
    do
    {
	prev = curr;
	curr = next;
	next = next->m_next;
    }
    while (mp > next && next != NULL);
#else
    do					    /* first, middle, last */
    {
	prev = next->m_next;		    /* curr, next, prev */
	if (prev == NULL || mp <= prev)
	{
	    prev = curr;
	    curr = next;
	    next = next->m_next;
	    break;
	}
	curr = prev->m_next;		    /* next, prev, curr */
	if (curr == NULL || mp <= curr)
	{
	    prev = next;
	    curr = prev->m_next;
	    next = curr->m_next;
	    break;
	}
	next = curr->m_next;		    /* prev, curr, next */
    }
    while (mp > next && next != NULL);
#endif

    /* if *mp and *next are concatenated, join them into one chunk */
    if ((char_u *)mp + mp->m_size == (char_u *)next)
    {
	mp->m_size += next->m_size;
	mp->m_next = next->m_next;
    }
    else
	mp->m_next = next;

    /* if *curr and *mp are concatenated, join them */
    if (prev != NULL && (char_u *)curr + curr->m_size == (char_u *)mp)
    {
	curr->m_size += mp->m_size;
	curr->m_next = mp->m_next;
	curbuf->b_m_search = prev;
    }
    else
    {
	curr->m_next = mp;
	curbuf->b_m_search = curr;  /* put curbuf->b_m_search before freed
				       chunk */
    }

    /*
     * If the block only containes free memory now, release it.
     */
    if (!keep && curbuf->b_mb_current->mb_size
			      == curbuf->b_mb_current->mb_info.m_next->m_size)
    {
	/* Find the block before the current one to be able to unlink it from
	 * the list of blocks. */
	prevb = &curbuf->b_block_head;
	for (nextb = prevb->mb_next; nextb != curbuf->b_mb_current;
						       nextb = nextb->mb_next)
	    prevb = nextb;
	prevb->mb_next = nextb->mb_next;
	vim_free(nextb);
	curbuf->b_mb_current = NULL;
	curbuf->b_m_search = NULL;
    }
}

/*
 * Allocate and initialize a new line structure with room for at least
 * 'size' characters plus a terminating NUL.
 */
    static char_u *
u_alloc_line(size)
    unsigned	size;
{
    minfo_T	*mp, *mprev, *mp2;
    mblock_T	*mbp;
    int		size_align;

    /*
     * Add room for size field and trailing NUL byte.
     * Adjust for minimal size (must be able to store minfo_T
     * plus a trailing NUL, so the chunk can be released again)
     */
    size += M_OFFSET + 1;
    if (size < sizeof(minfo_T) + 1)
	size = sizeof(minfo_T) + 1;

    /*
     * round size up for alignment
     */
    size_align = (size + ALIGN_MASK) & ~ALIGN_MASK;

    /*
     * If curbuf->b_m_search is NULL (uninitialized free list) start at
     * curbuf->b_block_head
     */
    if (curbuf->b_mb_current == NULL || curbuf->b_m_search == NULL)
    {
	curbuf->b_mb_current = &curbuf->b_block_head;
	curbuf->b_m_search = &(curbuf->b_block_head.mb_info);
    }

    /* search for space in free list */
    mprev = curbuf->b_m_search;
    mbp = curbuf->b_mb_current;
    mp = curbuf->b_m_search->m_next;
    if (mp == NULL)
    {
	if (mbp->mb_next)
	    mbp = mbp->mb_next;
	else
	    mbp = &curbuf->b_block_head;
	mp = curbuf->b_m_search = &(mbp->mb_info);
    }
    while (mp->m_size < size)
    {
	if (mp == curbuf->b_m_search)	    /* back where we started in free
					       chunk list */
	{
	    if (mbp->mb_next)
		mbp = mbp->mb_next;
	    else
		mbp = &curbuf->b_block_head;
	    mp = curbuf->b_m_search = &(mbp->mb_info);
	    if (mbp == curbuf->b_mb_current)	/* back where we started in
						   block list */
	    {
		int	n = (size_align > (MEMBLOCKSIZE / 4)
						 ? size_align : MEMBLOCKSIZE);

		mp = (minfo_T *)u_blockalloc((long_u)n);
		if (mp == NULL)
		    return (NULL);
		mp->m_size = n;
		u_free_line((char_u *)mp + M_OFFSET, TRUE);
		mp = curbuf->b_m_search;
		mbp = curbuf->b_mb_current;
	    }
	}
	mprev = mp;
	if ((mp = mp->m_next) == NULL)	    /* at end of the list */
	    mp = &(mbp->mb_info);	    /* wrap around to begin */
    }

    /* if the chunk we found is large enough, split it up in two */
    if ((long)mp->m_size - size_align >= (long)(sizeof(minfo_T) + 1))
    {
	mp2 = (minfo_T *)((char_u *)mp + size_align);
	mp2->m_size = mp->m_size - size_align;
	mp2->m_next = mp->m_next;
	mprev->m_next = mp2;
	mp->m_size = size_align;
    }
    else		    /* remove *mp from the free list */
    {
	mprev->m_next = mp->m_next;
    }
    curbuf->b_m_search = mprev;
    curbuf->b_mb_current = mbp;

    mp = (minfo_T *)((char_u *)mp + M_OFFSET);
    *(char_u *)mp = NUL;		    /* set the first byte to NUL */

    return ((char_u *)mp);
}

/*
 * u_save_line(): allocate memory with u_alloc_line() and copy line 'lnum'
 * into it.
 */
    static char_u *
u_save_line(lnum)
    linenr_T	lnum;
{
    char_u	*src;
    char_u	*dst;
    unsigned	len;

    src = ml_get(lnum);
    len = (unsigned)STRLEN(src);
    if ((dst = u_alloc_line(len)) != NULL)
	mch_memmove(dst, src, (size_t)(len + 1));
    return (dst);
}

/*
 * Check if the 'modified' flag is set, or 'ff' has changed (only need to
 * check the first character, because it can only be "dos", "unix" or "mac").
 * "nofile" and "scratch" type buffers are considered to always be unchanged.
 */
    int
bufIsChanged(buf)
    buf_T	*buf;
{
    return
#ifdef FEAT_QUICKFIX
	    !bt_dontwrite(buf) &&
#endif
	    (buf->b_changed || file_ff_differs(buf));
}

    int
curbufIsChanged()
{
    return
#ifdef FEAT_QUICKFIX
	!bt_dontwrite(curbuf) &&
#endif
	(curbuf->b_changed || file_ff_differs(curbuf));
}
