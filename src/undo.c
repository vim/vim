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
 * or is NULL if nothing has been undone (end of the branch).
 *
 * For keeping alternate undo/redo branches the uh_alt field is used.  Thus at
 * each point in the list a branch may appear for an alternate to redo.  The
 * uh_seq field is numbered sequentially to be able to find a newer or older
 * branch.
 *
 *		   +---------------+	+---------------+
 * b_u_oldhead --->| u_header	   |	| u_header	|
 *		   |   uh_alt_next ---->|   uh_alt_next ----> NULL
 *	   NULL <----- uh_alt_prev |<------ uh_alt_prev |
 *		   |   uh_prev	   |	|   uh_prev	|
 *		   +-----|---------+	+-----|---------+
 *			 |		      |
 *			 V		      V
 *		   +---------------+	+---------------+
 *		   | u_header	   |	| u_header	|
 *		   |   uh_alt_next |	|   uh_alt_next |
 * b_u_newhead --->|   uh_alt_prev |	|   uh_alt_prev |
 *		   |   uh_prev	   |	|   uh_prev	|
 *		   +-----|---------+	+-----|---------+
 *			 |		      |
 *			 V		      V
 *		       NULL		+---------------+    +---------------+
 *					| u_header	|    | u_header      |
 *					|   uh_alt_next ---->|	 uh_alt_next |
 *					|   uh_alt_prev |<------ uh_alt_prev |
 *					|   uh_prev	|    |	 uh_prev     |
 *					+-----|---------+    +-----|---------+
 *					      |			   |
 *					     etc.		  etc.
 *
 *
 * All data is allocated with U_ALLOC_LINE(), it will be freed as soon as the
 * buffer is unloaded.
 */

/* Uncomment the next line for including the u_check() function.  This warns
 * for errors in the debug information. */
/* #define U_DEBUG 1 */
#define UH_MAGIC 0x18dade	/* value for uh_magic when in use */
#define UE_MAGIC 0xabc123	/* value for ue_magic when in use */

#include "vim.h"

/* See below: use malloc()/free() for memory management. */
#define U_USE_MALLOC 1

static void u_unch_branch __ARGS((u_header_T *uhp));
static u_entry_T *u_get_headentry __ARGS((void));
static void u_getbot __ARGS((void));
static int u_savecommon __ARGS((linenr_T, linenr_T, linenr_T));
static void u_doit __ARGS((int count));
static void u_undoredo __ARGS((int undo));
static void u_undo_end __ARGS((int did_undo, int absolute));
static void u_add_time __ARGS((char_u *buf, size_t buflen, time_t tt));
static void u_freeheader __ARGS((buf_T *buf, u_header_T *uhp, u_header_T **uhpp));
static void u_freebranch __ARGS((buf_T *buf, u_header_T *uhp, u_header_T **uhpp));
static void u_freeentries __ARGS((buf_T *buf, u_header_T *uhp, u_header_T **uhpp));
static void u_freeentry __ARGS((u_entry_T *, long));

#ifdef U_USE_MALLOC
# define U_FREE_LINE(ptr) vim_free(ptr)
# define U_ALLOC_LINE(size) lalloc((long_u)((size) + 1), FALSE)
#else
static void u_free_line __ARGS((char_u *ptr, int keep));
static char_u *u_alloc_line __ARGS((unsigned size));
# define U_FREE_LINE(ptr) u_free_line((ptr), FALSE)
# define U_ALLOC_LINE(size) u_alloc_line(size)
#endif
static char_u *u_save_line __ARGS((linenr_T));

static long	u_newcount, u_oldcount;

/*
 * When 'u' flag included in 'cpoptions', we behave like vi.  Need to remember
 * the action that "u" should do.
 */
static int	undo_undoes = FALSE;

#ifdef U_DEBUG
/*
 * Check the undo structures for being valid.  Print a warning when something
 * looks wrong.
 */
static int seen_b_u_curhead;
static int seen_b_u_newhead;
static int header_count;

    static void
u_check_tree(u_header_T *uhp,
	u_header_T *exp_uh_next,
	u_header_T *exp_uh_alt_prev)
{
    u_entry_T *uep;

    if (uhp == NULL)
	return;
    ++header_count;
    if (uhp == curbuf->b_u_curhead && ++seen_b_u_curhead > 1)
    {
	EMSG("b_u_curhead found twice (looping?)");
	return;
    }
    if (uhp == curbuf->b_u_newhead && ++seen_b_u_newhead > 1)
    {
	EMSG("b_u_newhead found twice (looping?)");
	return;
    }

    if (uhp->uh_magic != UH_MAGIC)
	EMSG("uh_magic wrong (may be using freed memory)");
    else
    {
	/* Check pointers back are correct. */
	if (uhp->uh_next != exp_uh_next)
	{
	    EMSG("uh_next wrong");
	    smsg((char_u *)"expected: 0x%x, actual: 0x%x",
						   exp_uh_next, uhp->uh_next);
	}
	if (uhp->uh_alt_prev != exp_uh_alt_prev)
	{
	    EMSG("uh_alt_prev wrong");
	    smsg((char_u *)"expected: 0x%x, actual: 0x%x",
					   exp_uh_alt_prev, uhp->uh_alt_prev);
	}

	/* Check the undo tree at this header. */
	for (uep = uhp->uh_entry; uep != NULL; uep = uep->ue_next)
	{
	    if (uep->ue_magic != UE_MAGIC)
	    {
		EMSG("ue_magic wrong (may be using freed memory)");
		break;
	    }
	}

	/* Check the next alt tree. */
	u_check_tree(uhp->uh_alt_next, uhp->uh_next, uhp);

	/* Check the next header in this branch. */
	u_check_tree(uhp->uh_prev, uhp, NULL);
    }
}

    void
u_check(int newhead_may_be_NULL)
{
    seen_b_u_newhead = 0;
    seen_b_u_curhead = 0;
    header_count = 0;

    u_check_tree(curbuf->b_u_oldhead, NULL, NULL);

    if (seen_b_u_newhead == 0 && curbuf->b_u_oldhead != NULL
	    && !(newhead_may_be_NULL && curbuf->b_u_newhead == NULL))
	EMSGN("b_u_newhead invalid: 0x%x", curbuf->b_u_newhead);
    if (curbuf->b_u_curhead != NULL && seen_b_u_curhead == 0)
	EMSGN("b_u_curhead invalid: 0x%x", curbuf->b_u_curhead);
    if (header_count != curbuf->b_u_numhead)
    {
	EMSG("b_u_numhead invalid");
	smsg((char_u *)"expected: %ld, actual: %ld",
			       (long)header_count, (long)curbuf->b_u_numhead);
    }
}
#endif

/*
 * Save the current line for both the "u" and "U" command.
 * Returns OK or FAIL.
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

/*
 * Return TRUE when undo is allowed.  Otherwise give an error message and
 * return FALSE.
 */
    int
undo_allowed()
{
    /* Don't allow changes when 'modifiable' is off.  */
    if (!curbuf->b_p_ma)
    {
	EMSG(_(e_modifiable));
	return FALSE;
    }

#ifdef HAVE_SANDBOX
    /* In the sandbox it's not allowed to change the text. */
    if (sandbox != 0)
    {
	EMSG(_(e_sandbox));
	return FALSE;
    }
#endif

    /* Don't allow changes in the buffer while editing the cmdline.  The
     * caller of getcmdline() may get confused. */
    if (textlock != 0)
    {
	EMSG(_(e_secure));
	return FALSE;
    }

    return TRUE;
}

    static int
u_savecommon(top, bot, newbot)
    linenr_T	top, bot;
    linenr_T	newbot;
{
    linenr_T	lnum;
    long	i;
    u_header_T	*uhp;
    u_header_T	*old_curhead;
    u_entry_T	*uep;
    u_entry_T	*prev_uep;
    long	size;

    /* When making changes is not allowed return FAIL.  It's a crude way to
     * make all change commands fail. */
    if (!undo_allowed())
	return FAIL;

#ifdef U_DEBUG
    u_check(FALSE);
#endif
#ifdef FEAT_NETBEANS_INTG
    /*
     * Netbeans defines areas that cannot be modified.  Bail out here when
     * trying to change text in a guarded area.
     */
    if (usingNetbeans)
    {
	if (netbeans_is_guarded(top, bot))
	{
	    EMSG(_(e_guarded));
	    return FAIL;
	}
	if (curbuf->b_p_ro)
	{
	    EMSG(_(e_nbreadonly));
	    return FAIL;
	}
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

	if (p_ul >= 0)
	{
	    /*
	     * Make a new header entry.  Do this first so that we don't mess
	     * up the undo info when out of memory.
	     */
	    uhp = (u_header_T *)U_ALLOC_LINE((unsigned)sizeof(u_header_T));
	    if (uhp == NULL)
		goto nomem;
#ifdef U_DEBUG
	    uhp->uh_magic = UH_MAGIC;
#endif
	}
	else
	    uhp = NULL;

	/*
	 * If we undid more than we redid, move the entry lists before and
	 * including curbuf->b_u_curhead to an alternate branch.
	 */
	old_curhead = curbuf->b_u_curhead;
	if (old_curhead != NULL)
	{
	    curbuf->b_u_newhead = old_curhead->uh_next;
	    curbuf->b_u_curhead = NULL;
	}

	/*
	 * free headers to keep the size right
	 */
	while (curbuf->b_u_numhead > p_ul && curbuf->b_u_oldhead != NULL)
	{
	    u_header_T	    *uhfree = curbuf->b_u_oldhead;

	    if (uhfree == old_curhead)
		/* Can't reconnect the branch, delete all of it. */
		u_freebranch(curbuf, uhfree, &old_curhead);
	    else if (uhfree->uh_alt_next == NULL)
		/* There is no branch, only free one header. */
		u_freeheader(curbuf, uhfree, &old_curhead);
	    else
	    {
		/* Free the oldest alternate branch as a whole. */
		while (uhfree->uh_alt_next != NULL)
		    uhfree = uhfree->uh_alt_next;
		u_freebranch(curbuf, uhfree, &old_curhead);
	    }
#ifdef U_DEBUG
	    u_check(TRUE);
#endif
	}

	if (uhp == NULL)		/* no undo at all */
	{
	    if (old_curhead != NULL)
		u_freebranch(curbuf, old_curhead, NULL);
	    curbuf->b_u_synced = FALSE;
	    return OK;
	}

	uhp->uh_prev = NULL;
	uhp->uh_next = curbuf->b_u_newhead;
	uhp->uh_alt_next = old_curhead;
	if (old_curhead != NULL)
	{
	    uhp->uh_alt_prev = old_curhead->uh_alt_prev;
	    if (uhp->uh_alt_prev != NULL)
		uhp->uh_alt_prev->uh_alt_next = uhp;
	    old_curhead->uh_alt_prev = uhp;
	    if (curbuf->b_u_oldhead == old_curhead)
		curbuf->b_u_oldhead = uhp;
	}
	else
	    uhp->uh_alt_prev = NULL;
	if (curbuf->b_u_newhead != NULL)
	    curbuf->b_u_newhead->uh_prev = uhp;

	uhp->uh_seq = ++curbuf->b_u_seq_last;
	curbuf->b_u_seq_cur = uhp->uh_seq;
	uhp->uh_time = time(NULL);
	curbuf->b_u_seq_time = uhp->uh_time + 1;

	uhp->uh_walk = 0;
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

	/* save named marks and Visual marks for undo */
	mch_memmove(uhp->uh_namedm, curbuf->b_namedm, sizeof(pos_T) * NMARKS);
#ifdef FEAT_VISUAL
	uhp->uh_visual = curbuf->b_visual;
#endif

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
    uep = (u_entry_T *)U_ALLOC_LINE((unsigned)sizeof(u_entry_T));
    if (uep == NULL)
	goto nomem;
#ifdef U_DEBUG
    uep->ue_magic = UE_MAGIC;
#endif

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

    if (size > 0)
    {
	if ((uep->ue_array = (char_u **)U_ALLOC_LINE(
				(unsigned)(sizeof(char_u *) * size))) == NULL)
	{
	    u_freeentry(uep, 0L);
	    goto nomem;
	}
	for (i = 0, lnum = top + 1; i < size; ++i)
	{
	    fast_breakcheck();
	    if (got_int)
	    {
		u_freeentry(uep, i);
		return FAIL;
	    }
	    if ((uep->ue_array[i] = u_save_line(lnum++)) == NULL)
	    {
		u_freeentry(uep, i);
		goto nomem;
	    }
	}
    }
    else
	uep->ue_array = NULL;
    uep->ue_next = curbuf->b_u_newhead->uh_entry;
    curbuf->b_u_newhead->uh_entry = uep;
    curbuf->b_u_synced = FALSE;
    undo_undoes = FALSE;

#ifdef U_DEBUG
    u_check(FALSE);
#endif
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
	u_sync(TRUE);
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
u_doit(startcount)
    int startcount;
{
    int count = startcount;

    if (!undo_allowed())
	return;

    u_newcount = 0;
    u_oldcount = 0;
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
	u_oldcount = -1;
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
		if (count == startcount - 1)
		{
		    MSG(_("Already at oldest change"));
		    return;
		}
		break;
	    }

	    u_undoredo(TRUE);
	}
	else
	{
	    if (curbuf->b_u_curhead == NULL || p_ul <= 0)
	    {
		beep_flush();	/* nothing to redo */
		if (count == startcount - 1)
		{
		    MSG(_("Already at newest change"));
		    return;
		}
		break;
	    }

	    u_undoredo(FALSE);

	    /* Advance for next redo.  Set "newhead" when at the end of the
	     * redoable changes. */
	    if (curbuf->b_u_curhead->uh_prev == NULL)
		curbuf->b_u_newhead = curbuf->b_u_curhead;
	    curbuf->b_u_curhead = curbuf->b_u_curhead->uh_prev;
	}
    }
    u_undo_end(undo_undoes, FALSE);
}

static int lastmark = 0;

/*
 * Undo or redo over the timeline.
 * When "step" is negative go back in time, otherwise goes forward in time.
 * When "sec" is FALSE make "step" steps, when "sec" is TRUE use "step" as
 * seconds.
 * When "absolute" is TRUE use "step" as the sequence number to jump to.
 * "sec" must be FALSE then.
 */
    void
undo_time(step, sec, absolute)
    long	step;
    int		sec;
    int		absolute;
{
    long	    target;
    long	    closest;
    long	    closest_start;
    long	    closest_seq = 0;
    long	    val;
    u_header_T	    *uhp;
    u_header_T	    *last;
    int		    mark;
    int		    nomark;
    int		    round;
    int		    dosec = sec;
    int		    above = FALSE;
    int		    did_undo = TRUE;

    /* First make sure the current undoable change is synced. */
    if (curbuf->b_u_synced == FALSE)
	u_sync(TRUE);

    u_newcount = 0;
    u_oldcount = 0;
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
	u_oldcount = -1;

    /* "target" is the node below which we want to be.
     * Init "closest" to a value we can't reach. */
    if (absolute)
    {
	target = step;
	closest = -1;
    }
    else
    {
	/* When doing computations with time_t subtract starttime, because
	 * time_t converted to a long may result in a wrong number. */
	if (sec)
	    target = (long)(curbuf->b_u_seq_time - starttime) + step;
	else
	    target = curbuf->b_u_seq_cur + step;
	if (step < 0)
	{
	    if (target < 0)
		target = 0;
	    closest = -1;
	}
	else
	{
	    if (sec)
		closest = (long)(time(NULL) - starttime + 1);
	    else
		closest = curbuf->b_u_seq_last + 2;
	    if (target >= closest)
		target = closest - 1;
	}
    }
    closest_start = closest;
    closest_seq = curbuf->b_u_seq_cur;

    /*
     * May do this twice:
     * 1. Search for "target", update "closest" to the best match found.
     * 2. If "target" not found search for "closest".
     *
     * When using the closest time we use the sequence number in the second
     * round, because there may be several entries with the same time.
     */
    for (round = 1; round <= 2; ++round)
    {
	/* Find the path from the current state to where we want to go.  The
	 * desired state can be anywhere in the undo tree, need to go all over
	 * it.  We put "nomark" in uh_walk where we have been without success,
	 * "mark" where it could possibly be. */
	mark = ++lastmark;
	nomark = ++lastmark;

	if (curbuf->b_u_curhead == NULL)	/* at leaf of the tree */
	    uhp = curbuf->b_u_newhead;
	else
	    uhp = curbuf->b_u_curhead;

	while (uhp != NULL)
	{
	    uhp->uh_walk = mark;
	    val = (long)(dosec ? (uhp->uh_time - starttime) : uhp->uh_seq);

	    if (round == 1)
	    {
		/* Remember the header that is closest to the target.
		 * It must be at least in the right direction (checked with
		 * "b_u_seq_cur").  When the timestamp is equal find the
		 * highest/lowest sequence number. */
		if ((step < 0 ? uhp->uh_seq <= curbuf->b_u_seq_cur
			      : uhp->uh_seq > curbuf->b_u_seq_cur)
			&& ((dosec && val == closest)
			    ? (step < 0
				? uhp->uh_seq < closest_seq
				: uhp->uh_seq > closest_seq)
			    : closest == closest_start
				|| (val > target
				    ? (closest > target
					? val - target <= closest - target
					: val - target <= target - closest)
				    : (closest > target
					? target - val <= closest - target
					: target - val <= target - closest))))
		{
		    closest = val;
		    closest_seq = uhp->uh_seq;
		}
	    }

	    /* Quit searching when we found a match.  But when searching for a
	     * time we need to continue looking for the best uh_seq. */
	    if (target == val && !dosec)
		break;

	    /* go down in the tree if we haven't been there */
	    if (uhp->uh_prev != NULL && uhp->uh_prev->uh_walk != nomark
					     && uhp->uh_prev->uh_walk != mark)
		uhp = uhp->uh_prev;

	    /* go to alternate branch if we haven't been there */
	    else if (uhp->uh_alt_next != NULL
		    && uhp->uh_alt_next->uh_walk != nomark
		    && uhp->uh_alt_next->uh_walk != mark)
		uhp = uhp->uh_alt_next;

	    /* go up in the tree if we haven't been there and we are at the
	     * start of alternate branches */
	    else if (uhp->uh_next != NULL && uhp->uh_alt_prev == NULL
		    && uhp->uh_next->uh_walk != nomark
		    && uhp->uh_next->uh_walk != mark)
	    {
		/* If still at the start we don't go through this change. */
		if (uhp == curbuf->b_u_curhead)
		    uhp->uh_walk = nomark;
		uhp = uhp->uh_next;
	    }

	    else
	    {
		/* need to backtrack; mark this node as useless */
		uhp->uh_walk = nomark;
		if (uhp->uh_alt_prev != NULL)
		    uhp = uhp->uh_alt_prev;
		else
		    uhp = uhp->uh_next;
	    }
	}

	if (uhp != NULL)    /* found it */
	    break;

	if (absolute)
	{
	    EMSGN(_("Undo number %ld not found"), step);
	    return;
	}

	if (closest == closest_start)
	{
	    if (step < 0)
		MSG(_("Already at oldest change"));
	    else
		MSG(_("Already at newest change"));
	    return;
	}

	target = closest_seq;
	dosec = FALSE;
	if (step < 0)
	    above = TRUE;	/* stop above the header */
    }

    /* If we found it: Follow the path to go to where we want to be. */
    if (uhp != NULL)
    {
	/*
	 * First go up the tree as much as needed.
	 */
	for (;;)
	{
	    uhp = curbuf->b_u_curhead;
	    if (uhp == NULL)
		uhp = curbuf->b_u_newhead;
	    else
		uhp = uhp->uh_next;
	    if (uhp == NULL || uhp->uh_walk != mark
					 || (uhp->uh_seq == target && !above))
		break;
	    curbuf->b_u_curhead = uhp;
	    u_undoredo(TRUE);
	    uhp->uh_walk = nomark;	/* don't go back down here */
	}

	/*
	 * And now go down the tree (redo), branching off where needed.
	 */
	uhp = curbuf->b_u_curhead;
	while (uhp != NULL)
	{
	    /* Go back to the first branch with a mark. */
	    while (uhp->uh_alt_prev != NULL
					&& uhp->uh_alt_prev->uh_walk == mark)
		uhp = uhp->uh_alt_prev;

	    /* Find the last branch with a mark, that's the one. */
	    last = uhp;
	    while (last->uh_alt_next != NULL
					&& last->uh_alt_next->uh_walk == mark)
		last = last->uh_alt_next;
	    if (last != uhp)
	    {
		/* Make the used branch the first entry in the list of
		 * alternatives to make "u" and CTRL-R take this branch. */
		while (uhp->uh_alt_prev != NULL)
		    uhp = uhp->uh_alt_prev;
		if (last->uh_alt_next != NULL)
		    last->uh_alt_next->uh_alt_prev = last->uh_alt_prev;
		last->uh_alt_prev->uh_alt_next = last->uh_alt_next;
		last->uh_alt_prev = NULL;
		last->uh_alt_next = uhp;
		uhp->uh_alt_prev = last;

		uhp = last;
		if (uhp->uh_next != NULL)
		    uhp->uh_next->uh_prev = uhp;
	    }
	    curbuf->b_u_curhead = uhp;

	    if (uhp->uh_walk != mark)
		break;	    /* must have reached the target */

	    /* Stop when going backwards in time and didn't find the exact
	     * header we were looking for. */
	    if (uhp->uh_seq == target && above)
	    {
		curbuf->b_u_seq_cur = target - 1;
		break;
	    }

	    u_undoredo(FALSE);

	    /* Advance "curhead" to below the header we last used.  If it
	     * becomes NULL then we need to set "newhead" to this leaf. */
	    if (uhp->uh_prev == NULL)
		curbuf->b_u_newhead = uhp;
	    curbuf->b_u_curhead = uhp->uh_prev;
	    did_undo = FALSE;

	    if (uhp->uh_seq == target)	/* found it! */
		break;

	    uhp = uhp->uh_prev;
	    if (uhp == NULL || uhp->uh_walk != mark)
	    {
		/* Need to redo more but can't find it... */
		EMSG2(_(e_intern2), "undo_time()");
		break;
	    }
	}
    }
    u_undo_end(did_undo, absolute);
}

/*
 * u_undoredo: common code for undo and redo
 *
 * The lines in the file are replaced by the lines in the entry list at
 * curbuf->b_u_curhead. The replaced lines in the file are saved in the entry
 * list for the next undo/redo.
 *
 * When "undo" is TRUE we go up in the tree, when FALSE we go down.
 */
    static void
u_undoredo(undo)
    int		undo;
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
#ifdef FEAT_VISUAL
    visualinfo_T visualinfo;
#endif
    int		empty_buffer;		    /* buffer became empty */
    u_header_T	*curhead = curbuf->b_u_curhead;

#ifdef U_DEBUG
    u_check(FALSE);
#endif
    old_flags = curhead->uh_flags;
    new_flags = (curbuf->b_changed ? UH_CHANGED : 0) +
	       ((curbuf->b_ml.ml_flags & ML_EMPTY) ? UH_EMPTYBUF : 0);
    setpcmark();

    /*
     * save marks before undo/redo
     */
    mch_memmove(namedm, curbuf->b_namedm, sizeof(pos_T) * NMARKS);
#ifdef FEAT_VISUAL
    visualinfo = curbuf->b_visual;
#endif
    curbuf->b_op_start.lnum = curbuf->b_ml.ml_line_count;
    curbuf->b_op_start.col = 0;
    curbuf->b_op_end.lnum = 0;
    curbuf->b_op_end.col = 0;

    for (uep = curhead->uh_entry; uep != NULL; uep = nuep)
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
	    lnum = curhead->uh_cursor.lnum;
	    if (lnum >= top && lnum <= top + newsize + 1)
	    {
		curwin->w_cursor = curhead->uh_cursor;
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
	if (oldsize > 0)
	{
	    if ((newarray = (char_u **)U_ALLOC_LINE(
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
	else
	    newarray = NULL;

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
		U_FREE_LINE(uep->ue_array[i]);
	    }
	    U_FREE_LINE((char_u *)uep->ue_array);
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

    curhead->uh_entry = newlist;
    curhead->uh_flags = new_flags;
    if ((old_flags & UH_EMPTYBUF) && bufempty())
	curbuf->b_ml.ml_flags |= ML_EMPTY;
    if (old_flags & UH_CHANGED)
	changed();
    else
#ifdef FEAT_NETBEANS_INTG
	/* per netbeans undo rules, keep it as modified */
	if (!isNetbeansModified(curbuf))
#endif
	unchanged(curbuf, FALSE);

    /*
     * restore marks from before undo/redo
     */
    for (i = 0; i < NMARKS; ++i)
	if (curhead->uh_namedm[i].lnum != 0)
	{
	    curbuf->b_namedm[i] = curhead->uh_namedm[i];
	    curhead->uh_namedm[i] = namedm[i];
	}
#ifdef FEAT_VISUAL
    if (curhead->uh_visual.vi_start.lnum != 0)
    {
	curbuf->b_visual = curhead->uh_visual;
	curhead->uh_visual = visualinfo;
    }
#endif

    /*
     * If the cursor is only off by one line, put it at the same position as
     * before starting the change (for the "o" command).
     * Otherwise the cursor should go to the first undone line.
     */
    if (curhead->uh_cursor.lnum + 1 == curwin->w_cursor.lnum
						 && curwin->w_cursor.lnum > 1)
	--curwin->w_cursor.lnum;
    if (curhead->uh_cursor.lnum == curwin->w_cursor.lnum)
    {
	curwin->w_cursor.col = curhead->uh_cursor.col;
#ifdef FEAT_VIRTUALEDIT
	if (virtual_active() && curhead->uh_cursor_vcol >= 0)
	    coladvance((colnr_T)curhead->uh_cursor_vcol);
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

    /* Remember where we are for "g-" and ":earlier 10s". */
    curbuf->b_u_seq_cur = curhead->uh_seq;
    if (undo)
	/* We are below the previous undo.  However, to make ":earlier 1s"
	 * work we compute this as being just above the just undone change. */
	--curbuf->b_u_seq_cur;

    /* The timestamp can be the same for multiple changes, just use the one of
     * the undone/redone change. */
    curbuf->b_u_seq_time = curhead->uh_time;
#ifdef U_DEBUG
    u_check(FALSE);
#endif
}

/*
 * If we deleted or added lines, report the number of less/more lines.
 * Otherwise, report the number of changes (this may be incorrect
 * in some cases, but it's better than nothing).
 */
    static void
u_undo_end(did_undo, absolute)
    int		did_undo;	/* just did an undo */
    int		absolute;	/* used ":undo N" */
{
    char	*msgstr;
    u_header_T	*uhp;
    char_u	msgbuf[80];

#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_UNDO) && KeyTyped)
	foldOpenCursor();
#endif

    if (global_busy	    /* no messages now, wait until global is finished */
	    || !messaging())  /* 'lazyredraw' set, don't do messages now */
	return;

    if (curbuf->b_ml.ml_flags & ML_EMPTY)
	--u_newcount;

    u_oldcount -= u_newcount;
    if (u_oldcount == -1)
	msgstr = N_("more line");
    else if (u_oldcount < 0)
	msgstr = N_("more lines");
    else if (u_oldcount == 1)
	msgstr = N_("line less");
    else if (u_oldcount > 1)
	msgstr = N_("fewer lines");
    else
    {
	u_oldcount = u_newcount;
	if (u_newcount == 1)
	    msgstr = N_("change");
	else
	    msgstr = N_("changes");
    }

    if (curbuf->b_u_curhead != NULL)
    {
	/* For ":undo N" we prefer a "after #N" message. */
	if (absolute && curbuf->b_u_curhead->uh_next != NULL)
	{
	    uhp = curbuf->b_u_curhead->uh_next;
	    did_undo = FALSE;
	}
	else if (did_undo)
	    uhp = curbuf->b_u_curhead;
	else
	    uhp = curbuf->b_u_curhead->uh_next;
    }
    else
	uhp = curbuf->b_u_newhead;

    if (uhp == NULL)
	*msgbuf = NUL;
    else
	u_add_time(msgbuf, sizeof(msgbuf), uhp->uh_time);

    smsg((char_u *)_("%ld %s; %s #%ld  %s"),
	    u_oldcount < 0 ? -u_oldcount : u_oldcount,
	    _(msgstr),
	    did_undo ? _("before") : _("after"),
	    uhp == NULL ? 0L : uhp->uh_seq,
	    msgbuf);
}

/*
 * u_sync: stop adding to the current entry list
 */
    void
u_sync(force)
    int	    force;	/* Also sync when no_u_sync is set. */
{
    /* Skip it when already synced or syncing is disabled. */
    if (curbuf->b_u_synced || (!force && no_u_sync > 0))
	return;
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
 * ":undolist": List the leafs of the undo tree
 */
/*ARGSUSED*/
    void
ex_undolist(eap)
    exarg_T *eap;
{
    garray_T	ga;
    u_header_T	*uhp;
    int		mark;
    int		nomark;
    int		changes = 1;
    int		i;

    /*
     * 1: walk the tree to find all leafs, put the info in "ga".
     * 2: sort the lines
     * 3: display the list
     */
    mark = ++lastmark;
    nomark = ++lastmark;
    ga_init2(&ga, (int)sizeof(char *), 20);

    uhp = curbuf->b_u_oldhead;
    while (uhp != NULL)
    {
	if (uhp->uh_prev == NULL && uhp->uh_walk != nomark
						      && uhp->uh_walk != mark)
	{
	    if (ga_grow(&ga, 1) == FAIL)
		break;
	    vim_snprintf((char *)IObuff, IOSIZE, "%6ld %7ld  ",
							uhp->uh_seq, changes);
	    u_add_time(IObuff + STRLEN(IObuff), IOSIZE - STRLEN(IObuff),
								uhp->uh_time);
	    ((char_u **)(ga.ga_data))[ga.ga_len++] = vim_strsave(IObuff);
	}

	uhp->uh_walk = mark;

	/* go down in the tree if we haven't been there */
	if (uhp->uh_prev != NULL && uhp->uh_prev->uh_walk != nomark
					 && uhp->uh_prev->uh_walk != mark)
	{
	    uhp = uhp->uh_prev;
	    ++changes;
	}

	/* go to alternate branch if we haven't been there */
	else if (uhp->uh_alt_next != NULL
		&& uhp->uh_alt_next->uh_walk != nomark
		&& uhp->uh_alt_next->uh_walk != mark)
	    uhp = uhp->uh_alt_next;

	/* go up in the tree if we haven't been there and we are at the
	 * start of alternate branches */
	else if (uhp->uh_next != NULL && uhp->uh_alt_prev == NULL
		&& uhp->uh_next->uh_walk != nomark
		&& uhp->uh_next->uh_walk != mark)
	{
	    uhp = uhp->uh_next;
	    --changes;
	}

	else
	{
	    /* need to backtrack; mark this node as done */
	    uhp->uh_walk = nomark;
	    if (uhp->uh_alt_prev != NULL)
		uhp = uhp->uh_alt_prev;
	    else
	    {
		uhp = uhp->uh_next;
		--changes;
	    }
	}
    }

    if (ga.ga_len == 0)
	MSG(_("Nothing to undo"));
    else
    {
	sort_strings((char_u **)ga.ga_data, ga.ga_len);

	msg_start();
	msg_puts_attr((char_u *)_("number changes  time"), hl_attr(HLF_T));
	for (i = 0; i < ga.ga_len && !got_int; ++i)
	{
	    msg_putchar('\n');
	    if (got_int)
		break;
	    msg_puts(((char_u **)ga.ga_data)[i]);
	}
	msg_end();

	ga_clear_strings(&ga);
    }
}

/*
 * Put the timestamp of an undo header in "buf[buflen]" in a nice format.
 */
    static void
u_add_time(buf, buflen, tt)
    char_u	*buf;
    size_t	buflen;
    time_t	tt;
{
#ifdef HAVE_STRFTIME
    struct tm	*curtime;

    if (time(NULL) - tt >= 100)
    {
	curtime = localtime(&tt);
	(void)strftime((char *)buf, buflen, "%H:%M:%S", curtime);
    }
    else
#endif
	vim_snprintf((char *)buf, buflen, _("%ld seconds ago"),
						     (long)(time(NULL) - tt));
}

/*
 * ":undojoin": continue adding to the last entry list
 */
/*ARGSUSED*/
    void
ex_undojoin(eap)
    exarg_T *eap;
{
    if (curbuf->b_u_newhead == NULL)
	return;		    /* nothing changed before */
    if (curbuf->b_u_curhead != NULL)
    {
	EMSG(_("E790: undojoin is not allowed after undo"));
	return;
    }
    if (!curbuf->b_u_synced)
	return;		    /* already unsynced */
    if (p_ul < 0)
	return;		    /* no entries, nothing to do */
    else
    {
	/* Go back to the last entry */
	curbuf->b_u_curhead = curbuf->b_u_newhead;
	curbuf->b_u_synced = FALSE;  /* no entries, nothing to do */
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
    u_unch_branch(buf->b_u_oldhead);
    buf->b_did_warn = FALSE;
}

    static void
u_unch_branch(uhp)
    u_header_T	*uhp;
{
    u_header_T	*uh;

    for (uh = uhp; uh != NULL; uh = uh->uh_prev)
    {
	uh->uh_flags |= UH_CHANGED;
	if (uh->uh_alt_next != NULL)
	    u_unch_branch(uh->uh_alt_next);	    /* recursive */
    }
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
 * Free one header "uhp" and its entry list and adjust the pointers.
 */
    static void
u_freeheader(buf, uhp, uhpp)
    buf_T	    *buf;
    u_header_T	    *uhp;
    u_header_T	    **uhpp;	/* if not NULL reset when freeing this header */
{
    u_header_T	    *uhap;

    /* When there is an alternate redo list free that branch completely,
     * because we can never go there. */
    if (uhp->uh_alt_next != NULL)
	u_freebranch(buf, uhp->uh_alt_next, uhpp);

    if (uhp->uh_alt_prev != NULL)
	uhp->uh_alt_prev->uh_alt_next = NULL;

    /* Update the links in the list to remove the header. */
    if (uhp->uh_next == NULL)
	buf->b_u_oldhead = uhp->uh_prev;
    else
	uhp->uh_next->uh_prev = uhp->uh_prev;

    if (uhp->uh_prev == NULL)
	buf->b_u_newhead = uhp->uh_next;
    else
	for (uhap = uhp->uh_prev; uhap != NULL; uhap = uhap->uh_alt_next)
	    uhap->uh_next = uhp->uh_next;

    u_freeentries(buf, uhp, uhpp);
}

/*
 * Free an alternate branch and any following alternate branches.
 */
    static void
u_freebranch(buf, uhp, uhpp)
    buf_T	    *buf;
    u_header_T	    *uhp;
    u_header_T	    **uhpp;	/* if not NULL reset when freeing this header */
{
    u_header_T	    *tofree, *next;

    /* If this is the top branch we may need to use u_freeheader() to update
     * all the pointers. */
    if (uhp == buf->b_u_oldhead)
    {
	u_freeheader(buf, uhp, uhpp);
	return;
    }

    if (uhp->uh_alt_prev != NULL)
	uhp->uh_alt_prev->uh_alt_next = NULL;

    next = uhp;
    while (next != NULL)
    {
	tofree = next;
	if (tofree->uh_alt_next != NULL)
	    u_freebranch(buf, tofree->uh_alt_next, uhpp);   /* recursive */
	next = tofree->uh_prev;
	u_freeentries(buf, tofree, uhpp);
    }
}

/*
 * Free all the undo entries for one header and the header itself.
 * This means that "uhp" is invalid when returning.
 */
    static void
u_freeentries(buf, uhp, uhpp)
    buf_T	    *buf;
    u_header_T	    *uhp;
    u_header_T	    **uhpp;	/* if not NULL reset when freeing this header */
{
    u_entry_T	    *uep, *nuep;

    /* Check for pointers to the header that become invalid now. */
    if (buf->b_u_curhead == uhp)
	buf->b_u_curhead = NULL;
    if (buf->b_u_newhead == uhp)
	buf->b_u_newhead = NULL;  /* freeing the newest entry */
    if (uhpp != NULL && uhp == *uhpp)
	*uhpp = NULL;

    for (uep = uhp->uh_entry; uep != NULL; uep = nuep)
    {
	nuep = uep->ue_next;
	u_freeentry(uep, uep->ue_size);
    }

#ifdef U_DEBUG
    uhp->uh_magic = 0;
#endif
    U_FREE_LINE((char_u *)uhp);
    --buf->b_u_numhead;
}

/*
 * free entry 'uep' and 'n' lines in uep->ue_array[]
 */
    static void
u_freeentry(uep, n)
    u_entry_T	*uep;
    long	    n;
{
    while (n > 0)
	U_FREE_LINE(uep->ue_array[--n]);
    U_FREE_LINE((char_u *)uep->ue_array);
#ifdef U_DEBUG
    uep->ue_magic = 0;
#endif
    U_FREE_LINE((char_u *)uep);
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
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count) /* should never happen */
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
	U_FREE_LINE(curbuf->b_u_line_ptr);
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

    if (curbuf->b_u_line_ptr == NULL
			|| curbuf->b_u_line_lnum > curbuf->b_ml.ml_line_count)
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
    U_FREE_LINE(curbuf->b_u_line_ptr);
    curbuf->b_u_line_ptr = oldp;

    t = curbuf->b_u_line_colnr;
    if (curwin->w_cursor.lnum == curbuf->b_u_line_lnum)
	curbuf->b_u_line_colnr = curwin->w_cursor.col;
    curwin->w_cursor.col = t;
    curwin->w_cursor.lnum = curbuf->b_u_line_lnum;
    check_cursor_col();
}

/*
 * There are two implementations of the memory management for undo:
 * 1. Use the standard malloc()/free() functions.
 *    This should be fast for allocating memory, but when a buffer is
 *    abandoned every single allocated chunk must be freed, which may be slow.
 * 2. Allocate larger blocks of memory and keep track of chunks ourselves.
 *    This is fast for abandoning, but the use of linked lists is slow for
 *    finding a free chunk.  Esp. when a lot of lines are changed or deleted.
 * A bit of profiling showed that the first method is faster, especially when
 * making a large number of changes, under the condition that malloc()/free()
 * is implemented efficiently.
 */
#ifdef U_USE_MALLOC
/*
 * Version of undo memory allocation using malloc()/free()
 *
 * U_FREE_LINE() and U_ALLOC_LINE() are macros that invoke vim_free() and
 * lalloc() directly.
 */

/*
 * Free all allocated memory blocks for the buffer 'buf'.
 */
    void
u_blockfree(buf)
    buf_T	*buf;
{
    while (buf->b_u_oldhead != NULL)
	u_freeheader(buf, buf->b_u_oldhead, NULL);
    U_FREE_LINE(buf->b_u_line_ptr);
}

#else
/*
 * Storage allocation for the undo lines and blocks of the current file.
 * Version where Vim keeps track of the available memory.
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

static char_u *u_blockalloc __ARGS((long_u));

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
	p->mb_maxsize = 0;		/* nothing free yet */
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
    long_u	maxsize;

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
    maxsize = mp->m_size;

    /* if *curr and *mp are concatenated, join them */
    if (prev != NULL && (char_u *)curr + curr->m_size == (char_u *)mp)
    {
	curr->m_size += mp->m_size;
	maxsize = curr->m_size;
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
     * If the block only contains free memory now, release it.
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
    else if (curbuf->b_mb_current->mb_maxsize < maxsize)
	curbuf->b_mb_current->mb_maxsize = maxsize;
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

    /* Search for a block with enough space. */
    mbp = curbuf->b_mb_current;
    while (mbp->mb_maxsize < size_align)
    {
	if (mbp->mb_next != NULL)
	    mbp = mbp->mb_next;
	else
	    mbp = &curbuf->b_block_head;
	if (mbp == curbuf->b_mb_current)
	{
	    int	n = (size_align > (MEMBLOCKSIZE / 4)
					     ? size_align : MEMBLOCKSIZE);

	    /* Back where we started in block list: need to add a new block
	     * with enough space. */
	    mp = (minfo_T *)u_blockalloc((long_u)n);
	    if (mp == NULL)
		return (NULL);
	    mp->m_size = n;
	    u_free_line((char_u *)mp + M_OFFSET, TRUE);
	    mbp = curbuf->b_mb_current;
	    break;
	}
    }
    if (mbp != curbuf->b_mb_current)
	curbuf->b_m_search = &(mbp->mb_info);

    /* In this block find a chunk with enough space. */
    mprev = curbuf->b_m_search;
    mp = curbuf->b_m_search->m_next;
    for (;;)
    {
	if (mp == NULL)			    /* at end of the list */
	    mp = &(mbp->mb_info);	    /* wrap around to begin */
	if (mp->m_size >= size)
	    break;
	if (mp == curbuf->b_m_search)
	{
	    /* back where we started in free chunk list: "cannot happen" */
	    EMSG2(_(e_intern2), "u_alloc_line()");
	    return NULL;
	}
	mprev = mp;
	mp = mp->m_next;
    }

    /* when using the largest chunk adjust mb_maxsize */
    if (mp->m_size >= mbp->mb_maxsize)
	mbp->mb_maxsize = 0;

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

    /* If using the largest chunk need to find the new largest chunk */
    if (mbp->mb_maxsize == 0)
	for (mp2 = &(mbp->mb_info); mp2 != NULL; mp2 = mp2->m_next)
	    if (mbp->mb_maxsize < mp2->m_size)
		mbp->mb_maxsize = mp2->m_size;

    mp = (minfo_T *)((char_u *)mp + M_OFFSET);
    *(char_u *)mp = NUL;		    /* set the first byte to NUL */

    return ((char_u *)mp);
}
#endif

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
    if ((dst = U_ALLOC_LINE(len)) != NULL)
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
