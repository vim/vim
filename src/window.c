/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef HAVE_FCNTL_H
# include <fcntl.h>	    /* for chdir() */
#endif

static int path_is_url __ARGS((char_u *p));
#if defined(FEAT_WINDOWS) || defined(PROTO)
static int win_split_ins __ARGS((int size, int flags, win_T *newwin, int dir));
static int win_comp_pos __ARGS((void));
static void frame_comp_pos __ARGS((frame_T *topfrp, int *row, int *col));
static void frame_setheight __ARGS((frame_T *curfrp, int height));
#ifdef FEAT_VERTSPLIT
static void frame_setwidth __ARGS((frame_T *curfrp, int width));
#endif
static void win_exchange __ARGS((long));
static void win_rotate __ARGS((int, int));
static void win_totop __ARGS((int size, int flags));
static void win_equal_rec __ARGS((win_T *next_curwin, int current, frame_T *topfr, int dir, int col, int row, int width, int height));
static win_T *winframe_remove __ARGS((win_T *win, int *dirp));
static frame_T *win_altframe __ARGS((win_T *win));
static win_T *frame2win __ARGS((frame_T *frp));
static int frame_has_win __ARGS((frame_T *frp, win_T *wp));
static void frame_new_height __ARGS((frame_T *topfrp, int height, int topfirst, int wfh));
static int frame_fixed_height __ARGS((frame_T *frp));
#ifdef FEAT_VERTSPLIT
static void frame_add_statusline __ARGS((frame_T *frp));
static void frame_new_width __ARGS((frame_T *topfrp, int width, int leftfirst));
static void frame_add_vsep __ARGS((frame_T *frp));
static int frame_minwidth __ARGS((frame_T *topfrp, win_T *next_curwin));
static void frame_fix_width __ARGS((win_T *wp));
#endif
static void frame_fix_height __ARGS((win_T *wp));
static int frame_minheight __ARGS((frame_T *topfrp, win_T *next_curwin));
static void win_enter_ext __ARGS((win_T *wp, int undo_sync, int no_curwin));
static void win_free __ARGS((win_T *wp));
static void win_append __ARGS((win_T *, win_T *));
static void win_remove __ARGS((win_T *));
static void frame_append __ARGS((frame_T *after, frame_T *frp));
static void frame_insert __ARGS((frame_T *before, frame_T *frp));
static void frame_remove __ARGS((frame_T *frp));
#ifdef FEAT_VERTSPLIT
static void win_new_width __ARGS((win_T *wp, int width));
static int win_minheight __ARGS((win_T *wp));
static void win_goto_ver __ARGS((int up, long count));
static void win_goto_hor __ARGS((int left, long count));
#endif
static void frame_add_height __ARGS((frame_T *frp, int n));
static void last_status_rec __ARGS((frame_T *fr, int statusline));

static void make_snapshot __ARGS((void));
static void make_snapshot_rec __ARGS((frame_T *fr, frame_T **frp));
static void clear_snapshot __ARGS((void));
static void clear_snapshot_rec __ARGS((frame_T *fr));
static void restore_snapshot __ARGS((int close_curwin));
static int check_snapshot_rec __ARGS((frame_T *sn, frame_T *fr));
static win_T *restore_snapshot_rec __ARGS((frame_T *sn, frame_T *fr));

#endif /* FEAT_WINDOWS */
static win_T *win_alloc __ARGS((win_T *after));
static void win_new_height __ARGS((win_T *, int));

#define URL_SLASH	1		/* path_is_url() has found "://" */
#define URL_BACKSLASH	2		/* path_is_url() has found ":\\" */

#define NOWIN		(win_T *)-1	/* non-exisiting window */

#if defined(FEAT_WINDOWS) || defined(PROTO)
/*
 * all CTRL-W window commands are handled here, called from normal_cmd().
 */
    void
do_window(nchar, Prenum, xchar)
    int		nchar;
    long	Prenum;
    int		xchar;	    /* extra char from ":wincmd gx" or NUL */
{
    long	Prenum1;
    win_T	*wp;
#if defined(FEAT_SEARCHPATH) || defined(FEAT_FIND_ID)
    char_u	*ptr;
#endif
#ifdef FEAT_FIND_ID
    int		type = FIND_DEFINE;
    int		len;
#endif
    char_u	cbuf[40];

    if (Prenum == 0)
	Prenum1 = 1;
    else
	Prenum1 = Prenum;

#ifdef FEAT_CMDWIN
# define CHECK_CMDWIN if (cmdwin_type != 0) { EMSG(_(e_cmdwin)); break; }
#else
# define CHECK_CMDWIN
#endif

    switch (nchar)
    {
/* split current window in two parts, horizontally */
    case 'S':
    case Ctrl_S:
    case 's':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_split((int)Prenum, 0);
		break;

#ifdef FEAT_VERTSPLIT
/* split current window in two parts, vertically */
    case Ctrl_V:
    case 'v':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_split((int)Prenum, WSP_VERT);
		break;
#endif

/* split current window and edit alternate file */
    case Ctrl_HAT:
    case '^':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		STRCPY(cbuf, "split #");
		if (Prenum)
		    sprintf((char *)cbuf + 7, "%ld", Prenum);
		do_cmdline_cmd(cbuf);
		break;

/* open new window */
    case Ctrl_N:
    case 'n':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		if (Prenum)
		    sprintf((char *)cbuf, "%ld", Prenum); /* window height */
		else
		    cbuf[0] = NUL;
		STRCAT(cbuf, "new");
		do_cmdline_cmd(cbuf);
		break;

/* quit current window */
    case Ctrl_Q:
    case 'q':
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		do_cmdline_cmd((char_u *)"quit");
		break;

/* close current window */
    case Ctrl_C:
    case 'c':
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		do_cmdline_cmd((char_u *)"close");
		break;

#if defined(FEAT_WINDOWS) && defined(FEAT_QUICKFIX)
/* close preview window */
    case Ctrl_Z:
    case 'z':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		do_cmdline_cmd((char_u *)"pclose");
		break;

/* cursor to preview window */
    case 'P':
		for (wp = firstwin; wp != NULL; wp = wp->w_next)
		    if (wp->w_p_pvw)
			break;
		if (wp == NULL)
		    EMSG(_("E441: There is no preview window"));
		else
		    win_goto(wp);
		break;
#endif

/* close all but current window */
    case Ctrl_O:
    case 'o':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		do_cmdline_cmd((char_u *)"only");
		break;

/* cursor to next window with wrap around */
    case Ctrl_W:
    case 'w':
/* cursor to previous window with wrap around */
    case 'W':
		CHECK_CMDWIN
		if (lastwin == firstwin && Prenum != 1)	/* just one window */
		    beep_flush();
		else
		{
		    if (Prenum)			/* go to specified window */
		    {
			for (wp = firstwin; --Prenum > 0; )
			{
			    if (wp->w_next == NULL)
				break;
			    else
				wp = wp->w_next;
			}
		    }
		    else
		    {
			if (nchar == 'W')	    /* go to previous window */
			{
			    wp = curwin->w_prev;
			    if (wp == NULL)
				wp = lastwin;	    /* wrap around */
			}
			else			    /* go to next window */
			{
			    wp = curwin->w_next;
			    if (wp == NULL)
				wp = firstwin;	    /* wrap around */
			}
		    }
		    win_goto(wp);
		}
		break;

/* cursor to window below */
    case 'j':
    case K_DOWN:
    case Ctrl_J:
		CHECK_CMDWIN
#ifdef FEAT_VERTSPLIT
		win_goto_ver(FALSE, Prenum1);
#else
		for (wp = curwin; wp->w_next != NULL && Prenum1-- > 0;
							    wp = wp->w_next)
		    ;
		win_goto(wp);
#endif
		break;

/* cursor to window above */
    case 'k':
    case K_UP:
    case Ctrl_K:
		CHECK_CMDWIN
#ifdef FEAT_VERTSPLIT
		win_goto_ver(TRUE, Prenum1);
#else
		for (wp = curwin; wp->w_prev != NULL && Prenum1-- > 0;
							    wp = wp->w_prev)
		    ;
		win_goto(wp);
#endif
		break;

#ifdef FEAT_VERTSPLIT
/* cursor to left window */
    case 'h':
    case K_LEFT:
    case Ctrl_H:
    case K_BS:
		CHECK_CMDWIN
		win_goto_hor(TRUE, Prenum1);
		break;

/* cursor to right window */
    case 'l':
    case K_RIGHT:
    case Ctrl_L:
		CHECK_CMDWIN
		win_goto_hor(FALSE, Prenum1);
		break;
#endif

/* cursor to top-left window */
    case 't':
    case Ctrl_T:
		win_goto(firstwin);
		break;

/* cursor to bottom-right window */
    case 'b':
    case Ctrl_B:
		win_goto(lastwin);
		break;

/* cursor to last accessed (previous) window */
    case 'p':
    case Ctrl_P:
		if (prevwin == NULL)
		    beep_flush();
		else
		    win_goto(prevwin);
		break;

/* exchange current and next window */
    case 'x':
    case Ctrl_X:
		CHECK_CMDWIN
		win_exchange(Prenum);
		break;

/* rotate windows downwards */
    case Ctrl_R:
    case 'r':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		win_rotate(FALSE, (int)Prenum1);    /* downwards */
		break;

/* rotate windows upwards */
    case 'R':
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		win_rotate(TRUE, (int)Prenum1);	    /* upwards */
		break;

/* move window to the very top/bottom/left/right */
    case 'K':
    case 'J':
#ifdef FEAT_VERTSPLIT
    case 'H':
    case 'L':
#endif
		CHECK_CMDWIN
		win_totop((int)Prenum,
			((nchar == 'H' || nchar == 'L') ? WSP_VERT : 0)
			| ((nchar == 'H' || nchar == 'K') ? WSP_TOP : WSP_BOT));
		break;

/* make all windows the same height */
    case '=':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_equal(NULL, FALSE, 'b');
		break;

/* increase current window height */
    case '+':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(curwin->w_height + (int)Prenum1);
		break;

/* decrease current window height */
    case '-':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(curwin->w_height - (int)Prenum1);
		break;

/* set current window height */
    case Ctrl__:
    case '_':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(Prenum ? (int)Prenum : 9999);
		break;

#ifdef FEAT_VERTSPLIT
/* increase current window width */
    case '>':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setwidth(curwin->w_width + (int)Prenum1);
		break;

/* decrease current window width */
    case '<':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setwidth(curwin->w_width - (int)Prenum1);
		break;

/* set current window width */
    case '|':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setwidth(Prenum != 0 ? (int)Prenum : 9999);
		break;
#endif

/* jump to tag and split window if tag exists (in preview window) */
#if defined(FEAT_QUICKFIX)
    case '}':
		CHECK_CMDWIN
		if (Prenum)
		    g_do_tagpreview = Prenum;
		else
		    g_do_tagpreview = p_pvh;
		/*FALLTHROUGH*/
#endif
    case ']':
    case Ctrl_RSB:
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		if (Prenum)
		    postponed_split = Prenum;
		else
		    postponed_split = -1;

		/* Execute the command right here, required when
		 * "wincmd ]" was used in a function. */
		do_nv_ident(Ctrl_RSB, NUL);
		break;

#ifdef FEAT_SEARCHPATH
/* edit file name under cursor in a new window */
    case 'f':
    case Ctrl_F:
		CHECK_CMDWIN
#ifdef FEAT_VISUAL
		reset_VIsual_and_resel();	/* stop Visual mode */
#endif
		ptr = file_name_at_cursor(FNAME_MESS|FNAME_HYP|FNAME_EXP,
								     Prenum1);
		if (ptr != NULL)
		{
#ifdef FEAT_GUI
		    need_mouse_correct = TRUE;
#endif
		    setpcmark();
		    if (win_split(0, 0) == OK)
		    {
# ifdef FEAT_SCROLLBIND
			curwin->w_p_scb = FALSE;
# endif
			(void)do_ecmd(0, ptr, NULL, NULL, ECMD_LASTL,
								   ECMD_HIDE);
		    }
		    vim_free(ptr);
		}
		break;
#endif

#ifdef FEAT_FIND_ID
/* Go to the first occurence of the identifier under cursor along path in a
 * new window -- webb
 */
    case 'i':			    /* Go to any match */
    case Ctrl_I:
		type = FIND_ANY;
		/* FALLTHROUGH */
    case 'd':			    /* Go to definition, using 'define' */
    case Ctrl_D:
		CHECK_CMDWIN
		if ((len = find_ident_under_cursor(&ptr, FIND_IDENT)) == 0)
		    break;
		find_pattern_in_path(ptr, 0, len, TRUE,
			Prenum == 0 ? TRUE : FALSE, type,
			Prenum1, ACTION_SPLIT, (linenr_T)1, (linenr_T)MAXLNUM);
		curwin->w_set_curswant = TRUE;
		break;
#endif

/* CTRL-W g  extended commands */
    case 'g':
    case Ctrl_G:
		CHECK_CMDWIN
#ifdef USE_ON_FLY_SCROLL
		dont_scroll = TRUE;		/* disallow scrolling here */
#endif
		++no_mapping;
		++allow_keys;   /* no mapping for xchar, but allow key codes */
		if (xchar == NUL)
		    xchar = safe_vgetc();
#ifdef FEAT_LANGMAP
		LANGMAP_ADJUST(xchar, TRUE);
#endif
		--no_mapping;
		--allow_keys;
#ifdef FEAT_CMDL_INFO
		(void)add_to_showcmd(xchar);
#endif
		switch (xchar)
		{
#if defined(FEAT_QUICKFIX)
		    case '}':
			xchar = Ctrl_RSB;
			if (Prenum)
			    g_do_tagpreview = Prenum;
			else
			    g_do_tagpreview = p_pvh;
			/*FALLTHROUGH*/
#endif
		    case ']':
		    case Ctrl_RSB:
#ifdef FEAT_VISUAL
			reset_VIsual_and_resel();	/* stop Visual mode */
#endif
			if (Prenum)
			    postponed_split = Prenum;
			else
			    postponed_split = -1;

			/* Execute the command right here, required when
			 * "wincmd g}" was used in a function. */
			do_nv_ident('g', xchar);
			break;

		    default:
			beep_flush();
			break;
		}
		break;

    default:	beep_flush();
		break;
    }
}

/*
 * split the current window, implements CTRL-W s and :split
 *
 * "size" is the height or width for the new window, 0 to use half of current
 * height or width.
 *
 * "flags":
 * WSP_ROOM: require enough room for new window
 * WSP_VERT: vertical split.
 * WSP_TOP:  open window at the top-left of the shell (help window).
 * WSP_BOT:  open window at the bottom-right of the shell (quickfix window).
 * WSP_HELP: creating the help window, keep layout snapshot
 *
 * return FAIL for failure, OK otherwise
 */
    int
win_split(size, flags)
    int		size;
    int		flags;
{
    /* Add flags from ":vertical", ":topleft" and ":botright". */
    flags |= cmdmod.split;
    if ((flags & WSP_TOP) && (flags & WSP_BOT))
    {
	EMSG(_("E442: Can't split topleft and botright at the same time"));
	return FAIL;
    }

    /* When creating the help window make a snapshot of the window layout.
     * Otherwise clear the snapshot, it's now invalid. */
    if (flags & WSP_HELP)
	make_snapshot();
    else
	clear_snapshot();

    return win_split_ins(size, flags, NULL, 0);
}

/*
 * When "newwin" is NULL: split a window in two.
 * When "newwin" is not NULL: insert this window at the far
 * top/left/right/bottom.
 * return FAIL for failure, OK otherwise
 */
    static int
win_split_ins(size, flags, newwin, dir)
    int		size;
    int		flags;
    win_T	*newwin;
    int		dir;
{
    win_T	*wp = newwin;
    win_T	*oldwin;
    int		new_size = size;
    int		i;
    int		need_status = 0;
    int		do_equal = FALSE;
    int		needed;
    int		available;
    int		oldwin_height = 0;
    int		layout;
    frame_T	*frp, *curfrp;
    int		before;

    if (flags & WSP_TOP)
	oldwin = firstwin;
    else if (flags & WSP_BOT)
	oldwin = lastwin;
    else
	oldwin = curwin;

    /* add a status line when p_ls == 1 and splitting the first window */
    if (lastwin == firstwin && p_ls == 1 && oldwin->w_status_height == 0)
    {
	if (oldwin->w_height <= p_wmh && newwin == NULL)
	{
	    EMSG(_(e_noroom));
	    return FAIL;
	}
	need_status = STATUS_HEIGHT;
    }

#ifdef FEAT_VERTSPLIT
    if (flags & WSP_VERT)
    {
	layout = FR_ROW;
	do_equal = (p_ea && new_size == 0 && *p_ead != 'v');

	/*
	 * Check if we are able to split the current window and compute its
	 * width.
	 */
	needed = p_wmw + 1;
	if (flags & WSP_ROOM)
	    needed += p_wiw - p_wmw;
	if (p_ea || (flags & (WSP_BOT | WSP_TOP)))
	{
	    available = topframe->fr_width;
	    needed += frame_minwidth(topframe, NULL);
	}
	else
	    available = oldwin->w_width;
	if (available < needed && newwin == NULL)
	{
	    EMSG(_(e_noroom));
	    return FAIL;
	}
	if (new_size == 0)
	    new_size = oldwin->w_width / 2;
	if (new_size > oldwin->w_width - p_wmw - 1)
	    new_size = oldwin->w_width - p_wmw - 1;
	if (new_size < p_wmw)
	    new_size = p_wmw;

	/* if it doesn't fit in the current window, need win_equal() */
	if (oldwin->w_width - new_size - 1 < p_wmw)
	    do_equal = TRUE;
    }
    else
#endif
    {
	layout = FR_COL;
	do_equal = (p_ea && new_size == 0
#ifdef FEAT_VERTSPLIT
		&& *p_ead != 'h'
#endif
		);

	/*
	 * Check if we are able to split the current window and compute its
	 * height.
	 */
	needed = p_wmh + STATUS_HEIGHT + need_status;
	if (flags & WSP_ROOM)
	    needed += p_wh - p_wmh;
	if (p_ea || (flags & (WSP_BOT | WSP_TOP)))
	{
	    available = topframe->fr_height;
	    needed += frame_minheight(topframe, NULL);
	}
	else
	{
	    available = oldwin->w_height;
	    needed += p_wmh;
	}
	if (available < needed && newwin == NULL)
	{
	    EMSG(_(e_noroom));
	    return FAIL;
	}
	oldwin_height = oldwin->w_height;
	if (need_status)
	{
	    oldwin->w_status_height = STATUS_HEIGHT;
	    oldwin_height -= STATUS_HEIGHT;
	}
	if (new_size == 0)
	    new_size = oldwin_height / 2;

	if (new_size > oldwin_height - p_wmh - STATUS_HEIGHT)
	    new_size = oldwin_height - p_wmh - STATUS_HEIGHT;
	if (new_size < p_wmh)
	    new_size = p_wmh;

	/* if it doesn't fit in the current window, need win_equal() */
	if (oldwin_height - new_size - STATUS_HEIGHT < p_wmh)
	    do_equal = TRUE;

	/* We don't like to take lines for the new window from a
	 * 'winfixheight' window.  Take them from a window above or below
	 * instead, if possible. */
	if (oldwin->w_p_wfh)
	{
	    win_setheight_win(oldwin->w_height + new_size + STATUS_HEIGHT,
								      oldwin);
	    oldwin_height = oldwin->w_height;
	    if (need_status)
		oldwin_height -= STATUS_HEIGHT;
	}
    }

    /*
     * allocate new window structure and link it in the window list
     */
    if ((flags & WSP_TOP) == 0
	    && ((flags & WSP_BOT)
		|| (flags & WSP_BELOW)
		|| (!(flags & WSP_ABOVE)
		    && (
#ifdef FEAT_VERTSPLIT
			(flags & WSP_VERT) ? p_spr :
#endif
			p_sb))))
    {
	/* new window below/right of current one */
	if (newwin == NULL)
	    wp = win_alloc(oldwin);
	else
	    win_append(oldwin, wp);
    }
    else
    {
	if (newwin == NULL)
	    wp = win_alloc(oldwin->w_prev);
	else
	    win_append(oldwin->w_prev, wp);
    }

    if (newwin == NULL)
    {
	if (wp == NULL)
	    return FAIL;

	/*
	 * make the contents of the new window the same as the current one
	 */
	wp->w_buffer = curbuf;
	curbuf->b_nwindows++;
	wp->w_cursor = curwin->w_cursor;
	wp->w_valid = 0;
	wp->w_curswant = curwin->w_curswant;
	wp->w_set_curswant = curwin->w_set_curswant;
	wp->w_topline = curwin->w_topline;
#ifdef FEAT_DIFF
	wp->w_topfill = curwin->w_topfill;
#endif
	wp->w_leftcol = curwin->w_leftcol;
	wp->w_pcmark = curwin->w_pcmark;
	wp->w_prev_pcmark = curwin->w_prev_pcmark;
	wp->w_alt_fnum = curwin->w_alt_fnum;
	wp->w_fraction = curwin->w_fraction;
	wp->w_prev_fraction_row = curwin->w_prev_fraction_row;
#ifdef FEAT_JUMPLIST
	copy_jumplist(curwin, wp);
#endif
	if (curwin->w_localdir != NULL)
	    wp->w_localdir = vim_strsave(curwin->w_localdir);

	/* Use the same argument list. */
	wp->w_alist = curwin->w_alist;
	++wp->w_alist->al_refcount;
	wp->w_arg_idx = curwin->w_arg_idx;

	/*
	 * copy tagstack and options from existing window
	 */
	for (i = 0; i < curwin->w_tagstacklen; i++)
	{
	    wp->w_tagstack[i] = curwin->w_tagstack[i];
	    if (wp->w_tagstack[i].tagname != NULL)
		wp->w_tagstack[i].tagname =
				       vim_strsave(wp->w_tagstack[i].tagname);
	}
	wp->w_tagstackidx = curwin->w_tagstackidx;
	wp->w_tagstacklen = curwin->w_tagstacklen;
	win_copy_options(curwin, wp);
#ifdef FEAT_FOLDING
	copyFoldingState(curwin, wp);
#endif
    }

    /*
     * Reorganise the tree of frames to insert the new window.
     */
    if (flags & (WSP_TOP | WSP_BOT))
    {
#ifdef FEAT_VERTSPLIT
	if ((topframe->fr_layout == FR_COL && (flags & WSP_VERT) == 0)
	    || (topframe->fr_layout == FR_ROW && (flags & WSP_VERT) != 0))
#else
	if (topframe->fr_layout == FR_COL)
#endif
	{
	    curfrp = topframe->fr_child;
	    if (flags & WSP_BOT)
		while (curfrp->fr_next != NULL)
		    curfrp = curfrp->fr_next;
	}
	else
	    curfrp = topframe;
	before = (flags & WSP_TOP);
    }
    else
    {
	curfrp = oldwin->w_frame;
	if (flags & WSP_BELOW)
	    before = FALSE;
	else if (flags & WSP_ABOVE)
	    before = TRUE;
	else
#ifdef FEAT_VERTSPLIT
	if (flags & WSP_VERT)
	    before = !p_spr;
	else
#endif
	    before = !p_sb;
    }
    if (curfrp->fr_parent == NULL || curfrp->fr_parent->fr_layout != layout)
    {
	/* Need to create a new frame in the tree to make a branch. */
	frp = (frame_T *)alloc_clear((unsigned)sizeof(frame_T));
	*frp = *curfrp;
	curfrp->fr_layout = layout;
	frp->fr_parent = curfrp;
	frp->fr_next = NULL;
	frp->fr_prev = NULL;
	curfrp->fr_child = frp;
	curfrp->fr_win = NULL;
	curfrp = frp;
	if (frp->fr_win != NULL)
	    oldwin->w_frame = frp;
	else
	    for (frp = frp->fr_child; frp != NULL; frp = frp->fr_next)
		frp->fr_parent = curfrp;
    }

    if (newwin == NULL)
    {
	/* Create a frame for the new window. */
	frp = (frame_T *)alloc_clear((unsigned)sizeof(frame_T));
	frp->fr_layout = FR_LEAF;
	frp->fr_win = wp;
	wp->w_frame = frp;
    }
    else
	frp = newwin->w_frame;
    frp->fr_parent = curfrp->fr_parent;

    /* Insert the new frame at the right place in the frame list. */
    if (before)
	frame_insert(curfrp, frp);
    else
	frame_append(curfrp, frp);

#ifdef FEAT_VERTSPLIT
    if (flags & WSP_VERT)
    {
	wp->w_p_scr = curwin->w_p_scr;
	if (need_status)
	{
	    --oldwin->w_height;
	    oldwin->w_status_height = need_status;
	}
	if (flags & (WSP_TOP | WSP_BOT))
	{
	    /* set height and row of new window to full height */
	    wp->w_winrow = 0;
	    wp->w_height = curfrp->fr_height - (p_ls > 0);
	    wp->w_status_height = (p_ls > 0);
	}
	else
	{
	    /* height and row of new window is same as current window */
	    wp->w_winrow = oldwin->w_winrow;
	    wp->w_height = oldwin->w_height;
	    wp->w_status_height = oldwin->w_status_height;
	}
	frp->fr_height = curfrp->fr_height;

	/* "new_size" of the current window goes to the new window, use
	 * one column for the vertical separator */
	wp->w_width = new_size;
	if (before)
	    wp->w_vsep_width = 1;
	else
	{
	    wp->w_vsep_width = oldwin->w_vsep_width;
	    oldwin->w_vsep_width = 1;
	}
	if (flags & (WSP_TOP | WSP_BOT))
	{
	    if (flags & WSP_BOT)
		frame_add_vsep(curfrp);
	    /* Set width of neighbor frame */
	    frame_new_width(curfrp, curfrp->fr_width
		    - (new_size + ((flags & WSP_TOP) != 0)), flags & WSP_TOP);
	}
	else
	    oldwin->w_width -= new_size + 1;
	if (before)	/* new window left of current one */
	{
	    wp->w_wincol = oldwin->w_wincol;
	    oldwin->w_wincol += new_size + 1;
	}
	else		/* new window right of current one */
	    wp->w_wincol = oldwin->w_wincol + oldwin->w_width + 1;
	frame_fix_width(oldwin);
	frame_fix_width(wp);
    }
    else
#endif
    {
	/* width and column of new window is same as current window */
#ifdef FEAT_VERTSPLIT
	if (flags & (WSP_TOP | WSP_BOT))
	{
	    wp->w_wincol = 0;
	    wp->w_width = Columns;
	    wp->w_vsep_width = 0;
	}
	else
	{
	    wp->w_wincol = oldwin->w_wincol;
	    wp->w_width = oldwin->w_width;
	    wp->w_vsep_width = oldwin->w_vsep_width;
	}
	frp->fr_width = curfrp->fr_width;
#endif

	/* "new_size" of the current window goes to the new window, use
	 * one row for the status line */
	win_new_height(wp, new_size);
	if (flags & (WSP_TOP | WSP_BOT))
	    frame_new_height(curfrp, curfrp->fr_height
			- (new_size + STATUS_HEIGHT), flags & WSP_TOP, FALSE);
	else
	    win_new_height(oldwin, oldwin_height - (new_size + STATUS_HEIGHT));
	if (before)	/* new window above current one */
	{
	    wp->w_winrow = oldwin->w_winrow;
	    wp->w_status_height = STATUS_HEIGHT;
	    oldwin->w_winrow += wp->w_height + STATUS_HEIGHT;
	}
	else		/* new window below current one */
	{
	    wp->w_winrow = oldwin->w_winrow + oldwin->w_height + STATUS_HEIGHT;
	    wp->w_status_height = oldwin->w_status_height;
	    oldwin->w_status_height = STATUS_HEIGHT;
	}
#ifdef FEAT_VERTSPLIT
	if (flags & WSP_BOT)
	    frame_add_statusline(curfrp);
#endif
	frame_fix_height(wp);
	frame_fix_height(oldwin);
    }

    if (flags & (WSP_TOP | WSP_BOT))
	(void)win_comp_pos();

    /*
     * Both windows need redrawing
     */
    redraw_win_later(wp, NOT_VALID);
    wp->w_redr_status = TRUE;
    redraw_win_later(oldwin, NOT_VALID);
    oldwin->w_redr_status = TRUE;

    if (need_status)
    {
	msg_row = Rows - 1;
	msg_col = sc_col;
	msg_clr_eos_force();	/* Old command/ruler may still be there */
	comp_col();
	msg_row = Rows - 1;
	msg_col = 0;	/* put position back at start of line */
    }

    /*
     * make the new window the current window and redraw
     */
    if (do_equal || dir != 0)
	win_equal(wp, TRUE,
#ifdef FEAT_VERTSPLIT
		(flags & WSP_VERT) ? (dir == 'v' ? 'b' : 'h')
		: dir == 'h' ? 'b' :
#endif
		'v');

    /* Don't change the window height/width to 'winheight' / 'winwidth' if a
     * size was given. */
#ifdef FEAT_VERTSPLIT
    if (flags & WSP_VERT)
    {
	i = p_wiw;
	if (size != 0)
	    p_wiw = size;

# ifdef FEAT_GUI
	/* When 'guioptions' includes 'L' or 'R' may have to add scrollbars. */
	if (gui.in_use)
	    gui_init_which_components(NULL);
# endif
    }
    else
#endif
    {
	i = p_wh;
	if (size != 0)
	    p_wh = size;
    }
    win_enter(wp, FALSE);
#ifdef FEAT_VERTSPLIT
    if (flags & WSP_VERT)
	p_wiw = i;
    else
#endif
	p_wh = i;

    return OK;
}

#endif /* FEAT_WINDOWS */

#ifdef FEAT_VERTSPLIT
/*
 * Return minimal height for window "wp" and windows east of it.
 * Takes into account the eastbound windws can be split, each of them
 * requireing p_wmh lines.  Doesn't count status lines.
 */
    static int
win_minheight(wp)
    win_T	*wp;
{
    int		minheight = p_wmh;
    int		n;
    win_T	*wp1, *wp2;

    wp1 = wp;
    for (;;)
    {
	wp1 = wp1->w_next;
	if (wp1 == NULL)
	    break;
	n = p_wmh;
	wp2 = wp1;
	for (;;)
	{
	    wp2 = wp2->w_next;
	    if (wp2 == NULL)
		break;
	    n += win_minheight(wp2);
	}
	if (n > minheight)
	    minheight = n;
    }
    return minheight;
}

#endif

#if defined(FEAT_WINDOWS) || defined(PROTO)
/*
 * Check if "win" is a pointer to an existing window.
 */
    int
win_valid(win)
    win_T	*win;
{
    win_T	*wp;

    if (win == NULL)
	return FALSE;
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (wp == win)
	    return TRUE;
    return FALSE;
}

/*
 * Return the number of windows.
 */
    int
win_count()
{
    win_T	*wp;
    int		count = 0;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	++count;
    return count;
}

/*
 * Make "count" windows on the screen.
 * Return actual number of windows on the screen.
 * Must be called when there is just one window, filling the whole screen
 * (excluding the command line).
 */
/*ARGSUSED*/
    int
make_windows(count, vertical)
    int		count;
    int		vertical;	/* split windows vertically if TRUE */
{
    int		maxcount;
    int		todo;

#ifdef FEAT_VERTSPLIT
    if (vertical)
    {
	/* Each windows needs at least 'winminwidth' lines and a separator
	 * column. */
	maxcount = (curwin->w_width + curwin->w_vsep_width
					     - (p_wiw - p_wmw)) / (p_wmw + 1);
    }
    else
#endif
    {
	/* Each window needs at least 'winminheight' lines and a status line. */
	maxcount = (curwin->w_height + curwin->w_status_height
				  - (p_wh - p_wmh)) / (p_wmh + STATUS_HEIGHT);
    }

    if (maxcount < 2)
	maxcount = 2;
    if (count > maxcount)
	count = maxcount;

    /*
     * add status line now, otherwise first window will be too big
     */
    if (count > 1)
	last_status(TRUE);

#ifdef FEAT_AUTOCMD
    /*
     * Don't execute autocommands while creating the windows.  Must do that
     * when putting the buffers in the windows.
     */
    ++autocmd_block;
#endif

    /* todo is number of windows left to create */
    for (todo = count - 1; todo > 0; --todo)
#ifdef FEAT_VERTSPLIT
	if (vertical)
	{
	    if (win_split(curwin->w_width - (curwin->w_width - todo)
			/ (todo + 1) - 1, WSP_VERT | WSP_ABOVE) == FAIL)
		break;
	}
	else
#endif
	{
	    if (win_split(curwin->w_height - (curwin->w_height - todo
			    * STATUS_HEIGHT) / (todo + 1)
			- STATUS_HEIGHT, WSP_ABOVE) == FAIL)
		break;
	}

#ifdef FEAT_AUTOCMD
    --autocmd_block;
#endif

    /* return actual number of windows */
    return (count - todo);
}

/*
 * Exchange current and next window
 */
    static void
win_exchange(Prenum)
    long	Prenum;
{
    frame_T	*frp;
    frame_T	*frp2;
    win_T	*wp;
    win_T	*wp2;
    int		temp;

    if (lastwin == firstwin)	    /* just one window */
    {
	beep_flush();
	return;
    }

#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

    /*
     * find window to exchange with
     */
    if (Prenum)
    {
	frp = curwin->w_frame->fr_parent->fr_child;
	while (frp != NULL && --Prenum > 0)
	    frp = frp->fr_next;
    }
    else if (curwin->w_frame->fr_next != NULL)	/* Swap with next */
	frp = curwin->w_frame->fr_next;
    else    /* Swap last window in row/col with previous */
	frp = curwin->w_frame->fr_prev;

    /* We can only exchange a window with another window, not with a frame
     * containing windows. */
    if (frp == NULL || frp->fr_win == NULL || frp->fr_win == curwin)
	return;
    wp = frp->fr_win;

/*
 * 1. remove curwin from the list. Remember after which window it was in wp2
 * 2. insert curwin before wp in the list
 * if wp != wp2
 *    3. remove wp from the list
 *    4. insert wp after wp2
 * 5. exchange the status line height and vsep width.
 */
    wp2 = curwin->w_prev;
    frp2 = curwin->w_frame->fr_prev;
    if (wp->w_prev != curwin)
    {
	win_remove(curwin);
	frame_remove(curwin->w_frame);
	win_append(wp->w_prev, curwin);
	frame_insert(frp, curwin->w_frame);
    }
    if (wp != wp2)
    {
	win_remove(wp);
	frame_remove(wp->w_frame);
	win_append(wp2, wp);
	if (frp2 == NULL)
	    frame_insert(wp->w_frame->fr_parent->fr_child, wp->w_frame);
	else
	    frame_append(frp2, wp->w_frame);
    }
    temp = curwin->w_status_height;
    curwin->w_status_height = wp->w_status_height;
    wp->w_status_height = temp;
#ifdef FEAT_VERTSPLIT
    temp = curwin->w_vsep_width;
    curwin->w_vsep_width = wp->w_vsep_width;
    wp->w_vsep_width = temp;

    /* If the windows are not in the same frame, exchange the sizes to avoid
     * messing up the window layout.  Otherwise fix the frame sizes. */
    if (curwin->w_frame->fr_parent != wp->w_frame->fr_parent)
    {
	temp = curwin->w_height;
	curwin->w_height = wp->w_height;
	wp->w_height = temp;
	temp = curwin->w_width;
	curwin->w_width = wp->w_width;
	wp->w_width = temp;
    }
    else
    {
	frame_fix_height(curwin);
	frame_fix_height(wp);
	frame_fix_width(curwin);
	frame_fix_width(wp);
    }
#endif

    (void)win_comp_pos();		/* recompute window positions */

    win_enter(wp, TRUE);
    redraw_later(CLEAR);
}

/*
 * rotate windows: if upwards TRUE the second window becomes the first one
 *		   if upwards FALSE the first window becomes the second one
 */
    static void
win_rotate(upwards, count)
    int		upwards;
    int		count;
{
    win_T	*wp1;
    win_T	*wp2;
    frame_T	*frp;
    int		n;

    if (firstwin == lastwin)		/* nothing to do */
    {
	beep_flush();
	return;
    }

#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

#ifdef FEAT_VERTSPLIT
    /* Check if all frames in this row/col have one window. */
    for (frp = curwin->w_frame->fr_parent->fr_child; frp != NULL;
							   frp = frp->fr_next)
	if (frp->fr_win == NULL)
	{
	    EMSG(_("E443: Cannot rotate when another window is split"));
	    return;
	}
#endif

    while (count--)
    {
	if (upwards)		/* first window becomes last window */
	{
	    /* remove first window/frame from the list */
	    frp = curwin->w_frame->fr_parent->fr_child;
	    wp1 = frp->fr_win;
	    win_remove(wp1);
	    frame_remove(frp);

	    /* find last frame and append removed window/frame after it */
	    for ( ; frp->fr_next != NULL; frp = frp->fr_next)
		;
	    win_append(frp->fr_win, wp1);
	    frame_append(frp, wp1->w_frame);

	    wp2 = frp->fr_win;		/* previously last window */
	}
	else			/* last window becomes first window */
	{
	    /* find last window/frame in the list and remove it */
	    for (frp = curwin->w_frame; frp->fr_next != NULL;
							   frp = frp->fr_next)
		;
	    wp1 = frp->fr_win;
	    wp2 = wp1->w_prev;		    /* will become last window */
	    win_remove(wp1);
	    frame_remove(frp);

	    /* append the removed window/frame before the first in the list */
	    win_append(frp->fr_parent->fr_child->fr_win->w_prev, wp1);
	    frame_insert(frp->fr_parent->fr_child, frp);
	}

	/* exchange status height and vsep width of old and new last window */
	n = wp2->w_status_height;
	wp2->w_status_height = wp1->w_status_height;
	wp1->w_status_height = n;
	frame_fix_height(wp1);
	frame_fix_height(wp2);
#ifdef FEAT_VERTSPLIT
	n = wp2->w_vsep_width;
	wp2->w_vsep_width = wp1->w_vsep_width;
	wp1->w_vsep_width = n;
	frame_fix_width(wp1);
	frame_fix_width(wp2);
#endif

	    /* recompute w_winrow and w_wincol for all windows */
	(void)win_comp_pos();
    }

    redraw_later(CLEAR);
}

/*
 * Move the current window to the very top/bottom/left/right of the screen.
 */
    static void
win_totop(size, flags)
    int		size;
    int		flags;
{
    int		dir;
    int		height = curwin->w_height;

    if (lastwin == firstwin)
    {
	beep_flush();
	return;
    }

    /* Remove the window and frame from the tree of frames. */
    (void)winframe_remove(curwin, &dir);
    win_remove(curwin);
    last_status(FALSE);	    /* may need to remove last status line */
    (void)win_comp_pos();   /* recompute window positions */

    /* Split a window on the desired side and put the window there. */
    (void)win_split_ins(size, flags, curwin, dir);
    if (!(flags & WSP_VERT))
    {
	win_setheight(height);
	if (p_ea)
	    win_equal(curwin, TRUE, 'v');
    }

#if defined(FEAT_GUI) && defined(FEAT_VERTSPLIT)
    /* When 'guioptions' includes 'L' or 'R' may have to remove or add
     * scrollbars.  Have to update them anyway. */
    if (gui.in_use)
    {
	out_flush();
	gui_init_which_components(NULL);
	gui_update_scrollbars(TRUE);
    }
    need_mouse_correct = TRUE;
#endif

}

/*
 * Move window "win1" to below/right of "win2" and make "win1" the current
 * window.  Only works within the same frame!
 */
    void
win_move_after(win1, win2)
    win_T	*win1, *win2;
{
    int		height;

    /* check if the arguments are reasonable */
    if (win1 == win2)
	return;

    /* check if there is something to do */
    if (win2->w_next != win1)
    {
	/* may need move the status line/vertical separator of the last window
	 * */
	if (win1 == lastwin)
	{
	    height = win1->w_prev->w_status_height;
	    win1->w_prev->w_status_height = win1->w_status_height;
	    win1->w_status_height = height;
#ifdef FEAT_VERTSPLIT
	    win1->w_prev->w_vsep_width = 0;
	    win1->w_vsep_width = 1;
#endif
	}
	else if (win2 == lastwin)
	{
	    height = win1->w_status_height;
	    win1->w_status_height = win2->w_status_height;
	    win2->w_status_height = height;
#ifdef FEAT_VERTSPLIT
	    win2->w_vsep_width = 1;
	    win1->w_vsep_width = 0;
#endif
	}
	win_remove(win1);
	frame_remove(win1->w_frame);
	win_append(win2, win1);
	frame_append(win2->w_frame, win1->w_frame);

	(void)win_comp_pos();	/* recompute w_winrow for all windows */
	redraw_later(NOT_VALID);
    }
    win_enter(win1, FALSE);
}

/*
 * Make all windows the same height.
 * 'next_curwin' will soon be the current window, make sure it has enough
 * rows.
 */
    void
win_equal(next_curwin, current, dir)
    win_T	*next_curwin;	/* pointer to current window to be or NULL */
    int		current;	/* do only frame with current window */
    int		dir;		/* 'v' for vertically, 'h' for horizontally,
				   'b' for both, 0 for using p_ead */
{
    if (dir == 0)
#ifdef FEAT_VERTSPLIT
	dir = *p_ead;
#else
	dir = 'b';
#endif
    win_equal_rec(next_curwin == NULL ? curwin : next_curwin, current,
		      topframe, dir, 0, 0, (int)Columns, topframe->fr_height);
}

/*
 * Set a frame to a new position and height, spreading the available room
 * equally over contained frames.
 * The window "next_curwin" (if not NULL) should at least get the size from
 * 'winheight' and 'winwidth' if possible.
 */
    static void
win_equal_rec(next_curwin, current, topfr, dir, col, row, width, height)
    win_T	*next_curwin;	/* pointer to current window to be or NULL */
    int		current;	/* do only frame with current window */
    frame_T	*topfr;		/* frame to set size off */
    int		dir;		/* 'v', 'h' or 'b', see win_equal() */
    int		col;		/* horizontal position for frame */
    int		row;		/* vertical position for frame */
    int		width;		/* new width of frame */
    int		height;		/* new height of frame */
{
    int		n, m;
    int		extra_sep = 0;
    int		wincount, totwincount = 0;
    frame_T	*fr;
    int		next_curwin_size = 0;
    int		room = 0;
    int		new_size;
    int		has_next_curwin = 0;
    int		hnc;

    if (topfr->fr_layout == FR_LEAF)
    {
	/* Set the width/height of this frame.
	 * Redraw when size or position changes */
	if (topfr->fr_height != height || topfr->fr_win->w_winrow != row
#ifdef FEAT_VERTSPLIT
		|| topfr->fr_width != width || topfr->fr_win->w_wincol != col
#endif
	   )
	{
	    topfr->fr_win->w_winrow = row;
	    frame_new_height(topfr, height, FALSE, FALSE);
#ifdef FEAT_VERTSPLIT
	    topfr->fr_win->w_wincol = col;
	    frame_new_width(topfr, width, FALSE);
#endif
	    redraw_all_later(CLEAR);
	}
    }
#ifdef FEAT_VERTSPLIT
    else if (topfr->fr_layout == FR_ROW)
    {
	topfr->fr_width = width;
	topfr->fr_height = height;

	if (dir != 'v')			/* equalize frame widths */
	{
	    /* Compute the maximum number of windows horizontally in this
	     * frame. */
	    n = frame_minwidth(topfr, NOWIN);
	    /* add one for the rightmost window, it doesn't have a separator */
	    if (col + width == Columns)
		extra_sep = 1;
	    else
		extra_sep = 0;
	    totwincount = (n + extra_sep) / (p_wmw + 1);

	    /* Compute room available for windows other than "next_curwin" */
	    m = frame_minwidth(topfr, next_curwin);
	    room = width - m;
	    if (room < 0)
	    {
		next_curwin_size = p_wiw + room;
		room = 0;
	    }
	    else if (n == m)		/* doesn't contain curwin */
		next_curwin_size = 0;
	    else
	    {
		next_curwin_size = (room + p_wiw + (totwincount - 1) * p_wmw
					   + (totwincount - 1)) / totwincount;
		if (next_curwin_size  > p_wiw)
		    room -= next_curwin_size - p_wiw;
		else
		    next_curwin_size = p_wiw;
	    }
	    if (n != m)
		--totwincount;		/* don't count curwin */
	}

	for (fr = topfr->fr_child; fr != NULL; fr = fr->fr_next)
	{
	    n = m = 0;
	    wincount = 1;
	    if (fr->fr_next == NULL)
		/* last frame gets all that remains (avoid roundoff error) */
		new_size = width;
	    else if (dir == 'v')
		new_size = fr->fr_width;
	    else
	    {
		/* Compute the maximum number of windows horiz. in "fr". */
		n = frame_minwidth(fr, NOWIN);
		wincount = (n + (fr->fr_next == NULL ? extra_sep : 0))
								/ (p_wmw + 1);
		m = frame_minwidth(fr, next_curwin);
		if (n != m)	    /* don't count next_curwin */
		    --wincount;
		new_size = (wincount * room + ((unsigned)totwincount >> 1))
								/ totwincount;
		if (n != m)	    /* add next_curwin size */
		{
		    next_curwin_size -= p_wiw - (m - n);
		    new_size += next_curwin_size;
		}
	    }

	    /* Skip frame that is full height when splitting or closing a
	     * window, unless equalizing all frames. */
	    if (!current || dir != 'v' || topfr->fr_parent != NULL
		    || (new_size != fr->fr_width)
		    || frame_has_win(fr, next_curwin))
		win_equal_rec(next_curwin, current, fr, dir, col, row,
							new_size + n, height);
	    col += new_size + n;
	    width -= new_size + n;
	    if (n != m)	    /* contains curwin */
		room -= new_size - next_curwin_size;
	    else
		room -= new_size;
	    totwincount -= wincount;
	}
    }
#endif
    else /* topfr->fr_layout == FR_COL */
    {
#ifdef FEAT_VERTSPLIT
	topfr->fr_width = width;
#endif
	topfr->fr_height = height;

	if (dir != 'h')			/* equalize frame heights */
	{
	    /* Compute maximum number of windows vertically in this frame. */
	    n = frame_minheight(topfr, NOWIN);
	    /* add one for the bottom window if it doesn't have a statusline */
	    if (row + height == cmdline_row && p_ls == 0)
		extra_sep = 1;
	    else
		extra_sep = 0;
	    totwincount = (n + extra_sep) / (p_wmh + 1);
	    has_next_curwin = frame_has_win(topfr, next_curwin);

	    /*
	     * Compute height for "next_curwin" window and room available for
	     * other windows.
	     * "m" is the minimal height when counting p_wh for "next_curwin".
	     */
	    m = frame_minheight(topfr, next_curwin);
	    room = height - m;
	    if (room < 0)
	    {
		/* The room is less then 'winheight', use all space for the
		 * current window. */
		next_curwin_size = p_wh + room;
		room = 0;
	    }
	    else
	    {
		next_curwin_size = -1;
		for (fr = topfr->fr_child; fr != NULL; fr = fr->fr_next)
		{
		    /* If 'winfixheight' set keep the window height if
		     * possible.
		     * Watch out for this window being the next_curwin. */
		    if (frame_fixed_height(fr))
		    {
			n = frame_minheight(fr, NOWIN);
			new_size = fr->fr_height;
			if (frame_has_win(fr, next_curwin))
			{
			    room += p_wh - p_wmh;
			    next_curwin_size = 0;
			    if (new_size < p_wh)
				new_size = p_wh;
			}
			else
			    /* These windows don't use up room. */
			    totwincount -= (n + (fr->fr_next == NULL
					      ? extra_sep : 0)) / (p_wmh + 1);
			room -= new_size - n;
			if (room < 0)
			{
			    new_size += room;
			    room = 0;
			}
			fr->fr_newheight = new_size;
		    }
		}
		if (next_curwin_size == -1)
		{
		    if (!has_next_curwin)
			next_curwin_size = 0;
		    else if (totwincount > 1
			    && (room + (totwincount - 2))
						   / (totwincount - 1) > p_wh)
		    {
			next_curwin_size = (room + p_wh + totwincount * p_wmh
					   + (totwincount - 1)) / totwincount;
			room -= next_curwin_size - p_wh;
		    }
		    else
			next_curwin_size = p_wh;
		}
	    }

	    if (has_next_curwin)
		--totwincount;		/* don't count curwin */
	}

	for (fr = topfr->fr_child; fr != NULL; fr = fr->fr_next)
	{
	    n = m = 0;
	    wincount = 1;
	    if (fr->fr_next == NULL)
		/* last frame gets all that remains (avoid roundoff error) */
		new_size = height;
	    else if (dir == 'h')
		new_size = fr->fr_height;
	    else if (frame_fixed_height(fr))
	    {
		new_size = fr->fr_newheight;
		wincount = 0;	    /* doesn't count as a sizeable window */
	    }
	    else
	    {
		/* Compute the maximum number of windows vert. in "fr". */
		n = frame_minheight(fr, NOWIN);
		wincount = (n + (fr->fr_next == NULL ? extra_sep : 0))
								/ (p_wmh + 1);
		m = frame_minheight(fr, next_curwin);
		if (has_next_curwin)
		    hnc = frame_has_win(fr, next_curwin);
		else
		    hnc = FALSE;
		if (hnc)	    /* don't count next_curwin */
		    --wincount;
		if (totwincount == 0)
		    new_size = room;
		else
		    new_size = (wincount * room + ((unsigned)totwincount >> 1))
								/ totwincount;
		if (hnc)	    /* add next_curwin size */
		{
		    next_curwin_size -= p_wh - (m - n);
		    new_size += next_curwin_size;
		    room -= new_size - next_curwin_size;
		}
		else
		    room -= new_size;
		new_size += n;
	    }
	    /* Skip frame that is full width when splitting or closing a
	     * window, unless equalizing all frames. */
	    if (!current || dir != 'h' || topfr->fr_parent != NULL
		    || (new_size != fr->fr_height)
		    || frame_has_win(fr, next_curwin))
		win_equal_rec(next_curwin, current, fr, dir, col, row,
							     width, new_size);
	    row += new_size;
	    height -= new_size;
	    totwincount -= wincount;
	}
    }
}

/*
 * close all windows for buffer 'buf'
 */
    void
close_windows(buf)
    buf_T	*buf;
{
    win_T	*win;

    ++RedrawingDisabled;
    for (win = firstwin; win != NULL && lastwin != firstwin; )
    {
	if (win->w_buffer == buf)
	{
	    win_close(win, FALSE);
	    win = firstwin;	    /* go back to the start */
	}
	else
	    win = win->w_next;
    }
    --RedrawingDisabled;
}

/*
 * close window "win"
 * If "free_buf" is TRUE related buffer may be unloaded.
 *
 * called by :quit, :close, :xit, :wq and findtag()
 */
    void
win_close(win, free_buf)
    win_T	*win;
    int		free_buf;
{
    win_T	*wp;
#ifdef FEAT_AUTOCMD
    int		other_buffer = FALSE;
#endif
    int		close_curwin = FALSE;
    frame_T	*frp;
    int		dir;
    int		help_window = FALSE;

    if (lastwin == firstwin)
    {
	EMSG(_("E444: Cannot close last window"));
	return;
    }

    /* When closing the help window, try restoring a snapshot after closing
     * the window.  Otherwise clear the snapshot, it's now invalid. */
    if (win->w_buffer->b_help)
	help_window = TRUE;
    else
	clear_snapshot();

#ifdef FEAT_AUTOCMD
    if (win == curwin)
    {
	/*
	 * Guess which window is going to be the new current window.
	 * This may change because of the autocommands (sigh).
	 */
	wp = frame2win(win_altframe(win));

	/*
	 * Be careful: If autocommands delete the window, return now.
	 */
	if (wp->w_buffer != curbuf)
	{
	    other_buffer = TRUE;
	    apply_autocmds(EVENT_BUFLEAVE, NULL, NULL, FALSE, curbuf);
	    if (!win_valid(win) || firstwin == lastwin)
		return;
	}
	apply_autocmds(EVENT_WINLEAVE, NULL, NULL, FALSE, curbuf);
	if (!win_valid(win) || firstwin == lastwin)
	    return;
# ifdef FEAT_EVAL
	/* autocmds may abort script processing */
	if (aborting())
	    return;
# endif
    }
#endif

    /*
     * Close the link to the buffer.
     */
    close_buffer(win, win->w_buffer, free_buf ? DOBUF_UNLOAD : 0);
    /* Autocommands may have closed the window already, or closed the only
     * other window. */
    if (!win_valid(win) || firstwin == lastwin)
	return;

    /* reduce the reference count to the argument list. */
    alist_unlink(win->w_alist);

    /* remove the window and its frame from the tree of frames. */
    frp = win->w_frame;
    wp = winframe_remove(win, &dir);
    vim_free(frp);
    win_free(win);

    /* Make sure curwin isn't invalid.  It can cause severe trouble when
     * printing an error message.  For win_equal() curbuf needs to be valid
     * too. */
    if (win == curwin)
    {
	curwin = wp;
#ifdef FEAT_QUICKFIX
	if (wp->w_p_pvw || bt_quickfix(wp->w_buffer))
	{
	    /*
	     * The cursor goes to the preview or the quickfix window, try
	     * finding another window to go to.
	     */
	    for (;;)
	    {
		if (wp->w_next == NULL)
		    wp = firstwin;
		else
		    wp = wp->w_next;
		if (wp == curwin)
		    break;
		if (!wp->w_p_pvw && !bt_quickfix(wp->w_buffer))
		{
		    curwin = wp;
		    break;
		}
	    }
	}
#endif
	curbuf = curwin->w_buffer;
	close_curwin = TRUE;
    }
    if (p_ea)
	win_equal(curwin, TRUE,
#ifdef FEAT_VERTSPLIT
		dir
#else
		0
#endif
		);
    else
	win_comp_pos();
    if (close_curwin)
    {
	win_enter_ext(wp, FALSE, TRUE);
#ifdef FEAT_AUTOCMD
	if (other_buffer)
	    /* careful: after this wp and win may be invalid! */
	    apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
#endif
    }

    /*
     * if last window has a status line now and we don't want one,
     * remove the status line
     */
    last_status(FALSE);

    /* After closing the help window, try restoring the window layout from
     * before it was opened. */
    if (help_window)
	restore_snapshot(close_curwin);

#if defined(FEAT_GUI) && defined(FEAT_VERTSPLIT)
    /* When 'guioptions' includes 'L' or 'R' may have to remove scrollbars. */
    if (gui.in_use && !win_hasvertsplit())
	gui_init_which_components(NULL);
#endif

    redraw_all_later(NOT_VALID);
}

/*
 * Remove a window and its frame from the tree of frames.
 * Returns a pointer to the window that got the freed up space.
 */
/*ARGSUSED*/
    static win_T *
winframe_remove(win, dirp)
    win_T	*win;
    int		*dirp;		/* set to 'v' or 'h' for direction if 'ea' */
{
    frame_T	*frp, *frp2, *frp3;
    frame_T	*frp_close = win->w_frame;
    win_T	*wp;
    int		old_height = 0;

    /*
     * Remove the window from its frame.
     */
    frp2 = win_altframe(win);
    wp = frame2win(frp2);

    /* Remove this frame from the list of frames. */
    frame_remove(frp_close);

#ifdef FEAT_VERTSPLIT
    if (frp_close->fr_parent->fr_layout == FR_COL)
    {
#endif
	/* When 'winfixheight' is set, remember its old size and restore
	 * it later (it's a simplistic solution...).  Don't do this if the
	 * window will occupy the full height of the screen. */
	if (frp2->fr_win != NULL
		&& (frp2->fr_next != NULL || frp2->fr_prev != NULL)
		&& frp2->fr_win->w_p_wfh)
	    old_height = frp2->fr_win->w_height;
	frame_new_height(frp2, frp2->fr_height + frp_close->fr_height,
			    frp2 == frp_close->fr_next ? TRUE : FALSE, FALSE);
	if (old_height != 0)
	    win_setheight_win(old_height, frp2->fr_win);
#ifdef FEAT_VERTSPLIT
	*dirp = 'v';
    }
    else
    {
	frame_new_width(frp2, frp2->fr_width + frp_close->fr_width,
				   frp2 == frp_close->fr_next ? TRUE : FALSE);
	*dirp = 'h';
    }
#endif

    /* If rows/columns go to a window below/right its positions need to be
     * updated.  Can only be done after the sizes have been updated. */
    if (frp2 == frp_close->fr_next)
    {
	int row = win->w_winrow;
	int col = W_WINCOL(win);

	frame_comp_pos(frp2, &row, &col);
    }

    if (frp2->fr_next == NULL && frp2->fr_prev == NULL)
    {
	/* There is no other frame in this list, move its info to the parent
	 * and remove it. */
	frp2->fr_parent->fr_layout = frp2->fr_layout;
	frp2->fr_parent->fr_child = frp2->fr_child;
	for (frp = frp2->fr_child; frp != NULL; frp = frp->fr_next)
	    frp->fr_parent = frp2->fr_parent;
	frp2->fr_parent->fr_win = frp2->fr_win;
	if (frp2->fr_win != NULL)
	    frp2->fr_win->w_frame = frp2->fr_parent;
	frp = frp2->fr_parent;
	vim_free(frp2);

	frp2 = frp->fr_parent;
	if (frp2 != NULL && frp2->fr_layout == frp->fr_layout)
	{
	    /* The frame above the parent has the same layout, have to merge
	     * the frames into this list. */
	    if (frp2->fr_child == frp)
		frp2->fr_child = frp->fr_child;
	    frp->fr_child->fr_prev = frp->fr_prev;
	    if (frp->fr_prev != NULL)
		frp->fr_prev->fr_next = frp->fr_child;
	    for (frp3 = frp->fr_child; ; frp3 = frp3->fr_next)
	    {
		frp3->fr_parent = frp2;
		if (frp3->fr_next == NULL)
		{
		    frp3->fr_next = frp->fr_next;
		    if (frp->fr_next != NULL)
			frp->fr_next->fr_prev = frp3;
		    break;
		}
	    }
	    vim_free(frp);
	}
    }

    return wp;
}

/*
 * Find out which frame is going to get the freed up space when "win" is
 * closed.
 * if 'splitbelow'/'splitleft' the space goes to the window above/left.
 * if 'nosplitbelow'/'nosplitleft' the space goes to the window below/right.
 * This makes opening a window and closing it immediately keep the same window
 * layout.
 */
    static frame_T *
win_altframe(win)
    win_T	*win;
{
    frame_T	*frp;
    int		b;

    frp = win->w_frame;
#ifdef FEAT_VERTSPLIT
    if (frp->fr_parent->fr_layout == FR_ROW)
	b = p_spr;
    else
#endif
	b = p_sb;
    if ((!b && frp->fr_next != NULL) || frp->fr_prev == NULL)
	return frp->fr_next;
    return frp->fr_prev;
}

/*
 * Find the left-upper window in frame "frp".
 */
    static win_T *
frame2win(frp)
    frame_T	*frp;
{
    while (frp->fr_win == NULL)
	frp = frp->fr_child;
    return frp->fr_win;
}

/*
 * Return TRUE if frame "frp" contains window "wp".
 */
    static int
frame_has_win(frp, wp)
    frame_T	*frp;
    win_T	*wp;
{
    frame_T	*p;

    if (frp->fr_layout == FR_LEAF)
	return frp->fr_win == wp;

    for (p = frp->fr_child; p != NULL; p = p->fr_next)
	if (frame_has_win(p, wp))
	    return TRUE;
    return FALSE;
}

/*
 * Set a new height for a frame.  Recursively sets the height for contained
 * frames and windows.  Caller must take care of positions.
 */
    static void
frame_new_height(topfrp, height, topfirst, wfh)
    frame_T	*topfrp;
    int		height;
    int		topfirst;	/* resize topmost contained frame first */
    int		wfh;		/* obey 'winfixheight' when there is a choice;
				   may cause the height not to be set */
{
    frame_T	*frp;
    int		extra_lines;
    int		h;

    if (topfrp->fr_win != NULL)
    {
	/* Simple case: just one window. */
	win_new_height(topfrp->fr_win,
				    height - topfrp->fr_win->w_status_height);
    }
#ifdef FEAT_VERTSPLIT
    else if (topfrp->fr_layout == FR_ROW)
    {
	do
	{
	    /* All frames in this row get the same new height. */
	    for (frp = topfrp->fr_child; frp != NULL; frp = frp->fr_next)
	    {
		frame_new_height(frp, height, topfirst, wfh);
		if (frp->fr_height > height)
		{
		    /* Could not fit the windows, make the whole row higher. */
		    height = frp->fr_height;
		    break;
		}
	    }
	}
	while (frp != NULL);
    }
#endif
    else
    {
	/* Complicated case: Resize a column of frames.  Resize the bottom
	 * frame first, frames above that when needed. */

	frp = topfrp->fr_child;
	if (wfh)
	    /* Advance past frames with one window with 'wfh' set. */
	    while (frame_fixed_height(frp))
	    {
		frp = frp->fr_next;
		if (frp == NULL)
		    return;	    /* no frame without 'wfh', give up */
	    }
	if (!topfirst)
	{
	    /* Find the bottom frame of this column */
	    while (frp->fr_next != NULL)
		frp = frp->fr_next;
	    if (wfh)
		/* Advance back for frames with one window with 'wfh' set. */
		while (frame_fixed_height(frp))
		    frp = frp->fr_prev;
	}

	extra_lines = height - topfrp->fr_height;
	if (extra_lines < 0)
	{
	    /* reduce height of contained frames, bottom or top frame first */
	    while (frp != NULL)
	    {
		h = frame_minheight(frp, NULL);
		if (frp->fr_height + extra_lines < h)
		{
		    extra_lines += frp->fr_height - h;
		    frame_new_height(frp, h, topfirst, wfh);
		}
		else
		{
		    frame_new_height(frp, frp->fr_height + extra_lines,
							       topfirst, wfh);
		    break;
		}
		if (topfirst)
		{
		    do
			frp = frp->fr_next;
		    while (wfh && frp != NULL && frame_fixed_height(frp));
		}
		else
		{
		    do
			frp = frp->fr_prev;
		    while (wfh && frp != NULL && frame_fixed_height(frp));
		}
		/* Increase "height" if we could not reduce enough frames. */
		if (frp == NULL)
		    height -= extra_lines;
	    }
	}
	else if (extra_lines > 0)
	{
	    /* increase height of bottom or top frame */
	    frame_new_height(frp, frp->fr_height + extra_lines, topfirst, wfh);
	}
    }
    topfrp->fr_height = height;
}

/*
 * Return TRUE if height of frame "frp" should not be changed because of
 * the 'winfixheight' option.
 */
    static int
frame_fixed_height(frp)
    frame_T	*frp;
{
    /* frame with one window: fixed height if 'winfixheight' set. */
    if (frp->fr_win != NULL)
	return frp->fr_win->w_p_wfh;

    if (frp->fr_layout == FR_ROW)
    {
	/* The frame is fixed height if one of the frames in the row is fixed
	 * height. */
	for (frp = frp->fr_child; frp != NULL; frp = frp->fr_next)
	    if (frame_fixed_height(frp))
		return TRUE;
	return FALSE;
    }

    /* frp->fr_layout == FR_COL: The frame is fixed height if all of the
     * frames in the row are fixed height. */
    for (frp = frp->fr_child; frp != NULL; frp = frp->fr_next)
	if (!frame_fixed_height(frp))
	    return FALSE;
    return TRUE;
}

#ifdef FEAT_VERTSPLIT
/*
 * Add a status line to windows at the bottom of "frp".
 * Note: Does not check if there is room!
 */
    static void
frame_add_statusline(frp)
    frame_T	*frp;
{
    win_T	*wp;

    if (frp->fr_layout == FR_LEAF)
    {
	wp = frp->fr_win;
	if (wp->w_status_height == 0)
	{
	    if (wp->w_height > 0)	/* don't make it negative */
		--wp->w_height;
	    wp->w_status_height = STATUS_HEIGHT;
	}
    }
    else if (frp->fr_layout == FR_ROW)
    {
	/* Handle all the frames in the row. */
	for (frp = frp->fr_child; frp != NULL; frp = frp->fr_next)
	    frame_add_statusline(frp);
    }
    else /* frp->fr_layout == FR_COL */
    {
	/* Only need to handle the last frame in the column. */
	for (frp = frp->fr_child; frp->fr_next != NULL; frp = frp->fr_next)
	    ;
	frame_add_statusline(frp);
    }
}

/*
 * Set width of a frame.  Handles recursively going through contained frames.
 * May remove separator line for windows at the right side (for win_close()).
 */
    static void
frame_new_width(topfrp, width, leftfirst)
    frame_T	*topfrp;
    int		width;
    int		leftfirst;	/* resize leftmost contained frame first */
{
    frame_T	*frp;
    int		extra_cols;
    int		w;
    win_T	*wp;

    if (topfrp->fr_layout == FR_LEAF)
    {
	/* Simple case: just one window. */
	wp = topfrp->fr_win;
	/* Find out if there are any windows right of this one. */
	for (frp = topfrp; frp->fr_parent != NULL; frp = frp->fr_parent)
	    if (frp->fr_parent->fr_layout == FR_ROW && frp->fr_next != NULL)
		break;
	if (frp->fr_parent == NULL)
	    wp->w_vsep_width = 0;
	win_new_width(wp, width - wp->w_vsep_width);
    }
    else if (topfrp->fr_layout == FR_COL)
    {
	/* All frames in this column get the same new width. */
	for (frp = topfrp->fr_child; frp != NULL; frp = frp->fr_next)
	    frame_new_width(frp, width, leftfirst);
    }
    else    /* fr_layout == FR_ROW */
    {
	/* Complicated case: Resize a row of frames.  Resize the rightmost
	 * frame first, frames left of it when needed. */

	/* Find the rightmost frame of this row */
	frp = topfrp->fr_child;
	if (!leftfirst)
	    while (frp->fr_next != NULL)
		frp = frp->fr_next;

	extra_cols = width - topfrp->fr_width;
	if (extra_cols < 0)
	{
	    /* reduce frame width, rightmost frame first */
	    while (frp != NULL)
	    {
		w = frame_minwidth(frp, NULL);
		if (frp->fr_width + extra_cols < w)
		{
		    extra_cols += frp->fr_width - w;
		    frame_new_width(frp, w, leftfirst);
		}
		else
		{
		    frame_new_width(frp, frp->fr_width + extra_cols, leftfirst);
		    break;
		}
		if (leftfirst)
		    frp = frp->fr_next;
		else
		    frp = frp->fr_prev;
	    }
	}
	else if (extra_cols > 0)
	{
	    /* increase width of rightmost frame */
	    frame_new_width(frp, frp->fr_width + extra_cols, leftfirst);
	}
    }
    topfrp->fr_width = width;
}

/*
 * Add the vertical separator to windows at the right side of "frp".
 * Note: Does not check if there is room!
 */
    static void
frame_add_vsep(frp)
    frame_T	*frp;
{
    win_T	*wp;

    if (frp->fr_layout == FR_LEAF)
    {
	wp = frp->fr_win;
	if (wp->w_vsep_width == 0)
	{
	    if (wp->w_width > 0)	/* don't make it negative */
		--wp->w_width;
	    wp->w_vsep_width = 1;
	}
    }
    else if (frp->fr_layout == FR_COL)
    {
	/* Handle all the frames in the column. */
	for (frp = frp->fr_child; frp != NULL; frp = frp->fr_next)
	    frame_add_vsep(frp);
    }
    else /* frp->fr_layout == FR_ROW */
    {
	/* Only need to handle the last frame in the row. */
	frp = frp->fr_child;
	while (frp->fr_next != NULL)
	    frp = frp->fr_next;
	frame_add_vsep(frp);
    }
}

/*
 * Set frame width from the window it contains.
 */
    static void
frame_fix_width(wp)
    win_T	*wp;
{
    wp->w_frame->fr_width = wp->w_width + wp->w_vsep_width;
}
#endif

/*
 * Set frame height from the window it contains.
 */
    static void
frame_fix_height(wp)
    win_T	*wp;
{
    wp->w_frame->fr_height = wp->w_height + wp->w_status_height;
}

/*
 * Compute the minimal height for frame "topfrp".
 * Uses the 'winminheight' option.
 * When "next_curwin" isn't NULL, use p_wh for this window.
 * When "next_curwin" is NOWIN, don't use at least one line for the current
 * window.
 */
    static int
frame_minheight(topfrp, next_curwin)
    frame_T	*topfrp;
    win_T	*next_curwin;
{
    frame_T	*frp;
    int		m;
#ifdef FEAT_VERTSPLIT
    int		n;
#endif

    if (topfrp->fr_win != NULL)
    {
	if (topfrp->fr_win == next_curwin)
	    m = p_wh + topfrp->fr_win->w_status_height;
	else
	{
	    /* window: minimal height of the window plus status line */
	    m = p_wmh + topfrp->fr_win->w_status_height;
	    /* Current window is minimal one line high */
	    if (p_wmh == 0 && topfrp->fr_win == curwin && next_curwin == NULL)
		++m;
	}
    }
#ifdef FEAT_VERTSPLIT
    else if (topfrp->fr_layout == FR_ROW)
    {
	/* get the minimal height from each frame in this row */
	m = 0;
	for (frp = topfrp->fr_child; frp != NULL; frp = frp->fr_next)
	{
	    n = frame_minheight(frp, next_curwin);
	    if (n > m)
		m = n;
	}
    }
#endif
    else
    {
	/* Add up the minimal heights for all frames in this column. */
	m = 0;
	for (frp = topfrp->fr_child; frp != NULL; frp = frp->fr_next)
	    m += frame_minheight(frp, next_curwin);
    }

    return m;
}

#ifdef FEAT_VERTSPLIT
/*
 * Compute the minimal width for frame "topfrp".
 * When "next_curwin" isn't NULL, use p_wiw for this window.
 * When "next_curwin" is NOWIN, don't use at least one column for the current
 * window.
 */
    static int
frame_minwidth(topfrp, next_curwin)
    frame_T	*topfrp;
    win_T	*next_curwin;	/* use p_wh and p_wiw for next_curwin */
{
    frame_T	*frp;
    int		m, n;

    if (topfrp->fr_win != NULL)
    {
	if (topfrp->fr_win == next_curwin)
	    m = p_wiw + topfrp->fr_win->w_vsep_width;
	else
	{
	    /* window: minimal width of the window plus separator column */
	    m = p_wmw + topfrp->fr_win->w_vsep_width;
	    /* Current window is minimal one column wide */
	    if (p_wmw == 0 && topfrp->fr_win == curwin && next_curwin == NULL)
		++m;
	}
    }
    else if (topfrp->fr_layout == FR_COL)
    {
	/* get the minimal width from each frame in this column */
	m = 0;
	for (frp = topfrp->fr_child; frp != NULL; frp = frp->fr_next)
	{
	    n = frame_minwidth(frp, next_curwin);
	    if (n > m)
		m = n;
	}
    }
    else
    {
	/* Add up the minimal widths for all frames in this row. */
	m = 0;
	for (frp = topfrp->fr_child; frp != NULL; frp = frp->fr_next)
	    m += frame_minwidth(frp, next_curwin);
    }

    return m;
}
#endif


/*
 * Try to close all windows except current one.
 * Buffers in the other windows become hidden if 'hidden' is set, or '!' is
 * used and the buffer was modified.
 *
 * Used by ":bdel" and ":only".
 */
    void
close_others(message, forceit)
    int		message;
    int		forceit;	    /* always hide all other windows */
{
    win_T	*wp;
    win_T	*nextwp;
    int		r;

    if (lastwin == firstwin)
    {
	if (message
#ifdef FEAT_AUTOCMD
		    && !autocmd_busy
#endif
				    )
	    MSG(_("Already only one window"));
	return;
    }

    /* Be very careful here: autocommands may change the window layout. */
    for (wp = firstwin; win_valid(wp); wp = nextwp)
    {
	nextwp = wp->w_next;
	if (wp != curwin)		/* don't close current window */
	{

	    /* Check if it's allowed to abandon this window */
	    r = can_abandon(wp->w_buffer, forceit);
#ifdef FEAT_AUTOCMD
	    if (!win_valid(wp))		/* autocommands messed wp up */
	    {
		nextwp = firstwin;
		continue;
	    }
#endif
	    if (!r)
	    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
		if (message && (p_confirm || cmdmod.confirm) && p_write)
		{
		    dialog_changed(wp->w_buffer, FALSE);
# ifdef FEAT_AUTOCMD
		    if (!win_valid(wp))		/* autocommands messed wp up */
		    {
			nextwp = firstwin;
			continue;
		    }
# endif
		}
		if (bufIsChanged(wp->w_buffer))
#endif
		    continue;
	    }
	    win_close(wp, !P_HID(wp->w_buffer) && !bufIsChanged(wp->w_buffer));
	}
    }

    /*
     * If current window has a status line and we don't want one,
     * remove the status line.
     */
    if (lastwin != firstwin)
	EMSG(_("E445: Other window contains changes"));
}

#endif /* FEAT_WINDOWS */

/*
 * init the cursor in the window
 *
 * called when a new file is being edited
 */
    void
win_init(wp)
    win_T	*wp;
{
    redraw_win_later(wp, NOT_VALID);
    wp->w_lines_valid = 0;
    wp->w_cursor.lnum = 1;
    wp->w_curswant = wp->w_cursor.col = 0;
#ifdef FEAT_VIRTUALEDIT
    wp->w_cursor.coladd = 0;
#endif
    wp->w_pcmark.lnum = 1;	/* pcmark not cleared but set to line 1 */
    wp->w_pcmark.col = 0;
    wp->w_prev_pcmark.lnum = 0;
    wp->w_prev_pcmark.col = 0;
    wp->w_topline = 1;
#ifdef FEAT_DIFF
    wp->w_topfill = 0;
#endif
    wp->w_botline = 2;
#ifdef FEAT_FKMAP
    if (curwin->w_p_rl)
	wp->w_farsi = W_CONV + W_R_L;
    else
	wp->w_farsi = W_CONV;
#endif
}

/*
 * Allocate the first window and put an empty buffer in it.
 * Called from main().
 * When this fails we can't do anything: exit.
 */
    void
win_alloc_first()
{
    curwin = win_alloc(NULL);
    curbuf = buflist_new(NULL, NULL, 1L, BLN_LISTED);
    if (curwin == NULL || curbuf == NULL)
	mch_exit(0);
    curwin->w_buffer = curbuf;
    curbuf->b_nwindows = 1;	/* there is one window */
#ifdef FEAT_WINDOWS
    curwin->w_alist = &global_alist;
#endif
    win_init(curwin);		/* init current window */

    topframe = (frame_T *)alloc_clear((unsigned)sizeof(frame_T));
    if (topframe == NULL)
	mch_exit(0);
    topframe->fr_layout = FR_LEAF;
#ifdef FEAT_VERTSPLIT
    topframe->fr_width = Columns;
#endif
    topframe->fr_height = Rows - p_ch;
    topframe->fr_win = curwin;
    curwin->w_frame = topframe;
}

#if defined(FEAT_WINDOWS) || defined(PROTO)

/*
 * Go to another window.
 * When jumping to another buffer, stop Visual mode.  Do this before
 * changing windows so we can yank the selection into the '*' register.
 * When jumping to another window on the same buffer, adjust its cursor
 * position to keep the same Visual area.
 */
    void
win_goto(wp)
    win_T	*wp;
{
#ifdef FEAT_CMDWIN
    if (cmdwin_type != 0)
    {
	beep_flush();
	return;
    }
#endif
#ifdef FEAT_VISUAL
    if (wp->w_buffer != curbuf)
	reset_VIsual_and_resel();
    else if (VIsual_active)
	wp->w_cursor = curwin->w_cursor;
#endif

#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif
    win_enter(wp, TRUE);
}

#if defined(FEAT_PERL) || defined(PROTO)
/*
 * Find window number "winnr" (counting top to bottom).
 */
    win_T *
win_find_nr(winnr)
    int		winnr;
{
    win_T	*wp;

# ifdef FEAT_WINDOWS
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (--winnr == 0)
	    break;
    return wp;
# else
    return curwin;
# endif
}
#endif

#ifdef FEAT_VERTSPLIT
/*
 * Move to window above or below "count" times.
 */
    static void
win_goto_ver(up, count)
    int		up;		/* TRUE to go to win above */
    long	count;
{
    frame_T	*fr;
    frame_T	*nfr;
    frame_T	*foundfr;

    foundfr = curwin->w_frame;
    while (count--)
    {
	/*
	 * First go upwards in the tree of frames until we find a upwards or
	 * downwards neighbor.
	 */
	fr = foundfr;
	for (;;)
	{
	    if (fr == topframe)
		goto end;
	    if (up)
		nfr = fr->fr_prev;
	    else
		nfr = fr->fr_next;
	    if (fr->fr_parent->fr_layout == FR_COL && nfr != NULL)
		break;
	    fr = fr->fr_parent;
	}

	/*
	 * Now go downwards to find the bottom or top frame in it.
	 */
	for (;;)
	{
	    if (nfr->fr_layout == FR_LEAF)
	    {
		foundfr = nfr;
		break;
	    }
	    fr = nfr->fr_child;
	    if (nfr->fr_layout == FR_ROW)
	    {
		/* Find the frame at the cursor row. */
		while (fr->fr_next != NULL
			&& frame2win(fr)->w_wincol + fr->fr_width
					 <= curwin->w_wincol + curwin->w_wcol)
		    fr = fr->fr_next;
	    }
	    if (nfr->fr_layout == FR_COL && up)
		while (fr->fr_next != NULL)
		    fr = fr->fr_next;
	    nfr = fr;
	}
    }
end:
    if (foundfr != NULL)
	win_goto(foundfr->fr_win);
}

/*
 * Move to left or right window.
 */
    static void
win_goto_hor(left, count)
    int		left;		/* TRUE to go to left win */
    long	count;
{
    frame_T	*fr;
    frame_T	*nfr;
    frame_T	*foundfr;

    foundfr = curwin->w_frame;
    while (count--)
    {
	/*
	 * First go upwards in the tree of frames until we find a left or
	 * right neighbor.
	 */
	fr = foundfr;
	for (;;)
	{
	    if (fr == topframe)
		goto end;
	    if (left)
		nfr = fr->fr_prev;
	    else
		nfr = fr->fr_next;
	    if (fr->fr_parent->fr_layout == FR_ROW && nfr != NULL)
		break;
	    fr = fr->fr_parent;
	}

	/*
	 * Now go downwards to find the leftmost or rightmost frame in it.
	 */
	for (;;)
	{
	    if (nfr->fr_layout == FR_LEAF)
	    {
		foundfr = nfr;
		break;
	    }
	    fr = nfr->fr_child;
	    if (nfr->fr_layout == FR_COL)
	    {
		/* Find the frame at the cursor row. */
		while (fr->fr_next != NULL
			&& frame2win(fr)->w_winrow + fr->fr_height
					 <= curwin->w_winrow + curwin->w_wrow)
		    fr = fr->fr_next;
	    }
	    if (nfr->fr_layout == FR_ROW && left)
		while (fr->fr_next != NULL)
		    fr = fr->fr_next;
	    nfr = fr;
	}
    }
end:
    if (foundfr != NULL)
	win_goto(foundfr->fr_win);
}
#endif

/*
 * Make window "wp" the current window.
 */
    void
win_enter(wp, undo_sync)
    win_T	*wp;
    int		undo_sync;
{
    win_enter_ext(wp, undo_sync, FALSE);
}

/*
 * Make window wp the current window.
 * Can be called with "curwin_invalid" TRUE, which means that curwin has just
 * been closed and isn't valid.
 */
    static void
win_enter_ext(wp, undo_sync, curwin_invalid)
    win_T	*wp;
    int		undo_sync;
    int		curwin_invalid;
{
#ifdef FEAT_AUTOCMD
    int		other_buffer = FALSE;
#endif

    if (wp == curwin && !curwin_invalid)	/* nothing to do */
	return;

#ifdef FEAT_AUTOCMD
    if (!curwin_invalid)
    {
	/*
	 * Be careful: If autocommands delete the window, return now.
	 */
	if (wp->w_buffer != curbuf)
	{
	    apply_autocmds(EVENT_BUFLEAVE, NULL, NULL, FALSE, curbuf);
	    other_buffer = TRUE;
	    if (!win_valid(wp))
		return;
	}
	apply_autocmds(EVENT_WINLEAVE, NULL, NULL, FALSE, curbuf);
	if (!win_valid(wp))
	    return;
# ifdef FEAT_EVAL
	/* autocmds may abort script processing */
	if (aborting())
	    return;
# endif
    }
#endif

    /* sync undo before leaving the current buffer */
    if (undo_sync && curbuf != wp->w_buffer)
	u_sync();
    /* may have to copy the buffer options when 'cpo' contains 'S' */
    if (wp->w_buffer != curbuf)
	buf_copy_options(wp->w_buffer, BCO_ENTER | BCO_NOHELP);
    if (!curwin_invalid)
    {
	prevwin = curwin;	/* remember for CTRL-W p */
	curwin->w_redr_status = TRUE;
    }
    curwin = wp;
    curbuf = wp->w_buffer;
    check_cursor();
#ifdef FEAT_VIRTUALEDIT
    if (!virtual_active())
	curwin->w_cursor.coladd = 0;
#endif
    changed_line_abv_curs();	/* assume cursor position needs updating */

    if (curwin->w_localdir != NULL)
    {
	/* Window has a local directory: Save current directory as global
	 * directory (unless that was done already) and change to the local
	 * directory. */
	if (globaldir == NULL)
	{
	    char_u	cwd[MAXPATHL];

	    if (mch_dirname(cwd, MAXPATHL) == OK)
		globaldir = vim_strsave(cwd);
	}
	mch_chdir((char *)curwin->w_localdir);
	shorten_fnames(TRUE);
    }
    else if (globaldir != NULL)
    {
	/* Window doesn't have a local directory and we are not in the global
	 * directory: Change to the global directory. */
	mch_chdir((char *)globaldir);
	vim_free(globaldir);
	globaldir = NULL;
	shorten_fnames(TRUE);
    }

#ifdef FEAT_AUTOCMD
    apply_autocmds(EVENT_WINENTER, NULL, NULL, FALSE, curbuf);
    if (other_buffer)
	apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
#endif

#ifdef FEAT_TITLE
    maketitle();
#endif
    curwin->w_redr_status = TRUE;
    if (restart_edit)
	redraw_later(VALID);	/* causes status line redraw */

    /* set window height to desired minimal value */
    if (curwin->w_height < p_wh && !curwin->w_p_wfh)
	win_setheight((int)p_wh);
    else if (curwin->w_height == 0)
	win_setheight(1);

#ifdef FEAT_VERTSPLIT
    /* set window width to desired minimal value */
    if (curwin->w_width < p_wiw)
	win_setwidth((int)p_wiw);
#endif

#ifdef FEAT_MOUSE
    setmouse();			/* in case jumped to/from help buffer */
#endif

#if defined(FEAT_NETBEANS_INTG) || defined(FEAT_SUN_WORKSHOP)
    /* Change directories when the acd option is set on and after
     * switching windows. */
    if (p_acd && curbuf->b_ffname != NULL
				     && vim_chdirfile(curbuf->b_ffname) == OK)
	shorten_fnames(TRUE);
#endif
}

#endif /* FEAT_WINDOWS */

#if defined(FEAT_WINDOWS) || defined(FEAT_SIGNS) || defined(PROTO)
/*
 * Jump to the first open window that contains buffer buf if one exists
 * TODO: Alternatively jump to last open window? Dependent from 'splitbelow'?
 * Returns pointer to window if it exists, otherwise NULL.
 */
    win_T *
buf_jump_open_win(buf)
    buf_T	*buf;
{
# ifdef FEAT_WINDOWS
    win_T	*wp;

    for (wp = firstwin; wp; wp = wp->w_next)
	if (wp->w_buffer == buf)
	    break;
    if (wp != NULL)
	win_enter(wp, FALSE);
    return wp;
# else
    if (curwin->w_buffer == buf)
	return curwin;
    return NULL;
# endif
}
#endif

/*
 * allocate a window structure and link it in the window list
 */
/*ARGSUSED*/
    static win_T *
win_alloc(after)
    win_T	*after;
{
    win_T	*newwin;

    /*
     * allocate window structure and linesizes arrays
     */
    newwin = (win_T *)alloc_clear((unsigned)sizeof(win_T));
    if (newwin != NULL && win_alloc_lines(newwin) == FAIL)
    {
	vim_free(newwin);
	newwin = NULL;
    }

    if (newwin != NULL)
    {
	/*
	 * link the window in the window list
	 */
#ifdef FEAT_WINDOWS
	win_append(after, newwin);
#endif
#ifdef FEAT_VERTSPLIT
	newwin->w_wincol = 0;
	newwin->w_width = Columns;
#endif

	/* position the display and the cursor at the top of the file. */
	newwin->w_topline = 1;
#ifdef FEAT_DIFF
	newwin->w_topfill = 0;
#endif
	newwin->w_botline = 2;
	newwin->w_cursor.lnum = 1;
#ifdef FEAT_SCROLLBIND
	newwin->w_scbind_pos = 1;
#endif

	/* We won't calculate w_fraction until resizing the window */
	newwin->w_fraction = 0;
	newwin->w_prev_fraction_row = -1;

#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    out_flush();
	    gui_create_scrollbar(&newwin->w_scrollbars[SBAR_LEFT],
		    SBAR_LEFT, newwin);
	    gui_create_scrollbar(&newwin->w_scrollbars[SBAR_RIGHT],
		    SBAR_RIGHT, newwin);
	}
#endif
#ifdef FEAT_EVAL
	var_init(&newwin->w_vars);	    /* init internal variables */
#endif
#ifdef FEAT_FOLDING
	foldInitWin(newwin);
#endif
    }
    return newwin;
}

#if defined(FEAT_WINDOWS) || defined(PROTO)

/*
 * remove window 'wp' from the window list and free the structure
 */
    static void
win_free(wp)
    win_T	*wp;
{
    int		i;

#ifdef FEAT_PERL
    perl_win_free(wp);
#endif

#ifdef FEAT_PYTHON
    python_window_free(wp);
#endif

#ifdef FEAT_TCL
    tcl_window_free(wp);
#endif

#ifdef FEAT_RUBY
    ruby_window_free(wp);
#endif

    clear_winopt(&wp->w_onebuf_opt);
    clear_winopt(&wp->w_allbuf_opt);

#ifdef FEAT_EVAL
    var_clear(&wp->w_vars);	    /* free all internal variables */
#endif

    if (prevwin == wp)
	prevwin = NULL;
    win_free_lsize(wp);

    for (i = 0; i < wp->w_tagstacklen; ++i)
	vim_free(wp->w_tagstack[i].tagname);

    vim_free(wp->w_localdir);
#ifdef FEAT_SEARCH_EXTRA
    vim_free(wp->w_match.regprog);
#endif
#ifdef FEAT_JUMPLIST
    free_jumplist(wp);
#endif

#ifdef FEAT_GUI
    if (gui.in_use)
    {
	out_flush();
	gui_mch_destroy_scrollbar(&wp->w_scrollbars[SBAR_LEFT]);
	gui_mch_destroy_scrollbar(&wp->w_scrollbars[SBAR_RIGHT]);
    }
#endif /* FEAT_GUI */

    win_remove(wp);
    vim_free(wp);
}

/*
 * Append window "wp" in the window list after window "after".
 */
    static void
win_append(after, wp)
    win_T	*after, *wp;
{
    win_T	*before;

    if (after == NULL)	    /* after NULL is in front of the first */
	before = firstwin;
    else
	before = after->w_next;

    wp->w_next = before;
    wp->w_prev = after;
    if (after == NULL)
	firstwin = wp;
    else
	after->w_next = wp;
    if (before == NULL)
	lastwin = wp;
    else
	before->w_prev = wp;
}

/*
 * Remove a window from the window list.
 */
    static void
win_remove(wp)
    win_T	*wp;
{
    if (wp->w_prev != NULL)
	wp->w_prev->w_next = wp->w_next;
    else
	firstwin = wp->w_next;
    if (wp->w_next != NULL)
	wp->w_next->w_prev = wp->w_prev;
    else
	lastwin = wp->w_prev;
}

/*
 * Append frame "frp" in a frame list after frame "after".
 */
    static void
frame_append(after, frp)
    frame_T	*after, *frp;
{
    frp->fr_next = after->fr_next;
    after->fr_next = frp;
    if (frp->fr_next != NULL)
	frp->fr_next->fr_prev = frp;
    frp->fr_prev = after;
}

/*
 * Insert frame "frp" in a frame list before frame "before".
 */
    static void
frame_insert(before, frp)
    frame_T	*before, *frp;
{
    frp->fr_next = before;
    frp->fr_prev = before->fr_prev;
    before->fr_prev = frp;
    if (frp->fr_prev != NULL)
	frp->fr_prev->fr_next = frp;
    else
	frp->fr_parent->fr_child = frp;
}

/*
 * Remove a frame from a frame list.
 */
    static void
frame_remove(frp)
    frame_T	*frp;
{
    if (frp->fr_prev != NULL)
	frp->fr_prev->fr_next = frp->fr_next;
    else
	frp->fr_parent->fr_child = frp->fr_next;
    if (frp->fr_next != NULL)
	frp->fr_next->fr_prev = frp->fr_prev;
}

#endif /* FEAT_WINDOWS */

/*
 * Allocate w_lines[] for window "wp".
 * Return FAIL for failure, OK for success.
 */
    int
win_alloc_lines(wp)
    win_T	*wp;
{
    wp->w_lines_valid = 0;
    wp->w_lines = (wline_T *)alloc((unsigned)(Rows * sizeof(wline_T)));
    if (wp->w_lines == NULL)
	return FAIL;
    return OK;
}

/*
 * free lsize arrays for a window
 */
    void
win_free_lsize(wp)
    win_T	*wp;
{
    vim_free(wp->w_lines);
    wp->w_lines = NULL;
}

/*
 * Called from win_new_shellsize() after Rows changed.
 */
    void
shell_new_rows()
{
    int		h = (int)(Rows - p_ch);

    if (firstwin == NULL)	/* not initialized yet */
	return;
#ifdef FEAT_WINDOWS
    if (h < frame_minheight(topframe, NULL))
	h = frame_minheight(topframe, NULL);
    /* First try setting the heights of windows without 'winfixheight'.  If
     * that doesn't result in the right height, forget about that option. */
    frame_new_height(topframe, h, FALSE, TRUE);
    if (topframe->fr_height != h)
	frame_new_height(topframe, h, FALSE, FALSE);

    (void)win_comp_pos();		/* recompute w_winrow and w_wincol */
#else
    if (h < 1)
	h = 1;
    win_new_height(firstwin, h);
#endif
    compute_cmdrow();
#if 0
    /* Disabled: don't want making the screen smaller make a window larger. */
    if (p_ea)
	win_equal(curwin, FALSE, 'v');
#endif
}

#if defined(FEAT_VERTSPLIT) || defined(PROTO)
/*
 * Called from win_new_shellsize() after Columns changed.
 */
    void
shell_new_columns()
{
    if (firstwin == NULL)	/* not initialized yet */
	return;
    frame_new_width(topframe, (int)Columns, FALSE);
    (void)win_comp_pos();		/* recompute w_winrow and w_wincol */
#if 0
    /* Disabled: don't want making the screen smaller make a window larger. */
    if (p_ea)
	win_equal(curwin, FALSE, 'h');
#endif
}
#endif

#if defined(FEAT_CMDWIN) || defined(PROTO)
/*
 * Save the size of all windows in "gap".
 */
    void
win_size_save(gap)
    garray_T	*gap;

{
    win_T	*wp;

    ga_init2(gap, (int)sizeof(int), 1);
    if (ga_grow(gap, win_count() * 2) == OK)
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	{
	    ((int *)gap->ga_data)[gap->ga_len++] =
					       wp->w_width + wp->w_vsep_width;
	    ((int *)gap->ga_data)[gap->ga_len++] = wp->w_height;
	}
}

/*
 * Restore window sizes, but only if the number of windows is still the same.
 * Does not free the growarray.
 */
    void
win_size_restore(gap)
    garray_T	*gap;
{
    win_T	*wp;
    int		i;

    if (win_count() * 2 == gap->ga_len)
    {
	i = 0;
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	{
	    frame_setwidth(wp->w_frame, ((int *)gap->ga_data)[i++]);
	    win_setheight_win(((int *)gap->ga_data)[i++], wp);
	}
	/* recompute the window positions */
	(void)win_comp_pos();
    }
}
#endif /* FEAT_CMDWIN */

#if defined(FEAT_WINDOWS) || defined(PROTO)
/*
 * Update the position for all windows, using the width and height of the
 * frames.
 * Returns the row just after the last window.
 */
    static int
win_comp_pos()
{
    int		row = 0;
    int		col = 0;

    frame_comp_pos(topframe, &row, &col);
    return row;
}

/*
 * Update the position of the windows in frame "topfrp", using the width and
 * height of the frames.
 * "*row" and "*col" are the top-left position of the frame.  They are updated
 * to the bottom-right position plus one.
 */
    static void
frame_comp_pos(topfrp, row, col)
    frame_T	*topfrp;
    int		*row;
    int		*col;
{
    win_T	*wp;
    frame_T	*frp;
#ifdef FEAT_VERTSPLIT
    int		startcol;
    int		startrow;
#endif

    wp = topfrp->fr_win;
    if (wp != NULL)
    {
	if (wp->w_winrow != *row
#ifdef FEAT_VERTSPLIT
		|| wp->w_wincol != *col
#endif
		)
	{
	    /* position changed, redraw */
	    wp->w_winrow = *row;
#ifdef FEAT_VERTSPLIT
	    wp->w_wincol = *col;
#endif
	    redraw_win_later(wp, NOT_VALID);
	    wp->w_redr_status = TRUE;
	}
	*row += wp->w_height + wp->w_status_height;
#ifdef FEAT_VERTSPLIT
	*col += wp->w_width + wp->w_vsep_width;
#endif
    }
    else
    {
#ifdef FEAT_VERTSPLIT
	startrow = *row;
	startcol = *col;
#endif
	for (frp = topfrp->fr_child; frp != NULL; frp = frp->fr_next)
	{
#ifdef FEAT_VERTSPLIT
	    if (topfrp->fr_layout == FR_ROW)
		*row = startrow;	/* all frames are at the same row */
	    else
		*col = startcol;	/* all frames are at the same col */
#endif
	    frame_comp_pos(frp, row, col);
	}
    }
}

#endif /* FEAT_WINDOWS */

/*
 * Set current window height and take care of repositioning other windows to
 * fit around it.
 */
    void
win_setheight(height)
    int		height;
{
    win_setheight_win(height, curwin);
}

/*
 * Set the window height of window "win" and take care of repositioning other
 * windows to fit around it.
 */
    void
win_setheight_win(height, win)
    int		height;
    win_T	*win;
{
    int		row;

    if (win == curwin)
    {
	/* Always keep current window at least one line high, even when
	 * 'winminheight' is zero. */
#ifdef FEAT_WINDOWS
	if (height < p_wmh)
	    height = p_wmh;
#endif
	if (height == 0)
	    height = 1;
    }

#ifdef FEAT_WINDOWS
    frame_setheight(win->w_frame, height + win->w_status_height);

    /* recompute the window positions */
    row = win_comp_pos();
#else
    if (height > topframe->fr_height)
	height = topframe->fr_height;
    win->w_height = height;
    row = height;
#endif

    /*
     * If there is extra space created between the last window and the command
     * line, clear it.
     */
    if (full_screen && msg_scrolled == 0 && row < cmdline_row)
	screen_fill(row, cmdline_row, 0, (int)Columns, ' ', ' ', 0);
    cmdline_row = row;
    msg_row = row;
    msg_col = 0;

    redraw_all_later(NOT_VALID);
}

#if defined(FEAT_WINDOWS) || defined(PROTO)

/*
 * Set the height of a frame to "height" and take care that all frames and
 * windows inside it are resized.  Also resize frames on the left and right if
 * the are in the same FR_ROW frame.
 *
 * Strategy:
 * If the frame is part of a FR_COL frame, try fitting the frame in that
 * frame.  If that doesn't work (the FR_COL frame is too small), recursively
 * go to containing frames to resize them and make room.
 * If the frame is part of a FR_ROW frame, all frames must be resized as well.
 * Check for the minimal height of the FR_ROW frame.
 * At the top level we can also use change the command line height.
 */
    static void
frame_setheight(curfrp, height)
    frame_T	*curfrp;
    int		height;
{
    int		room;		/* total number of lines available */
    int		take;		/* number of lines taken from other windows */
    int		room_cmdline;	/* lines available from cmdline */
    int		run;
    frame_T	*frp;
    int		h;
    int		room_reserved;

    /* If the height already is the desired value, nothing to do. */
    if (curfrp->fr_height == height)
	return;

    if (curfrp->fr_parent == NULL)
    {
	/* topframe: can only change the command line */
	if (height > Rows - p_ch)
	    height = Rows - p_ch;
	if (height > 0)
	    frame_new_height(curfrp, height, FALSE, FALSE);
    }
    else if (curfrp->fr_parent->fr_layout == FR_ROW)
    {
	/* Row of frames: Also need to resize frames left and right of this
	 * one.  First check for the minimal height of these. */
	h = frame_minheight(curfrp->fr_parent, NULL);
	if (height < h)
	    height = h;
	frame_setheight(curfrp->fr_parent, height);
    }
    else
    {
	/*
	 * Column of frames: try to change only frames in this column.
	 */
#ifdef FEAT_VERTSPLIT
	/*
	 * Do this twice:
	 * 1: compute room available, if it's not enough try resizing the
	 *    containing frame.
	 * 2: compute the room available and adjust the height to it.
	 * Try not to reduce the height of a window with 'winfixheight' set.
	 */
	for (run = 1; run <= 2; ++run)
#else
	for (;;)
#endif
	{
	    room = 0;
	    room_reserved = 0;
	    for (frp = curfrp->fr_parent->fr_child; frp != NULL;
							   frp = frp->fr_next)
	    {
		if (frp != curfrp
			&& frp->fr_win != NULL
			&& frp->fr_win->w_p_wfh)
		    room_reserved += frp->fr_height;
		room += frp->fr_height;
		if (frp != curfrp)
		    room -= frame_minheight(frp, NULL);
	    }
#ifdef FEAT_VERTSPLIT
	    if (curfrp->fr_width != Columns)
		room_cmdline = 0;
	    else
#endif
	    {
		room_cmdline = Rows - p_ch - (lastwin->w_winrow
			       + lastwin->w_height + lastwin->w_status_height);
		if (room_cmdline < 0)
		    room_cmdline = 0;
	    }

	    if (height <= room + room_cmdline)
		break;
#ifdef FEAT_VERTSPLIT
	    if (run == 2 || curfrp->fr_width == Columns)
#endif
	    {
		if (height > room + room_cmdline)
		    height = room + room_cmdline;
		break;
	    }
#ifdef FEAT_VERTSPLIT
	    frame_setheight(curfrp->fr_parent, height
		+ frame_minheight(curfrp->fr_parent, NOWIN) - (int)p_wmh - 1);
#endif
	    /*NOTREACHED*/
	}

	/*
	 * Compute the number of lines we will take from others frames (can be
	 * negative!).
	 */
	take = height - curfrp->fr_height;

	/* If there is not enough room, also reduce the height of a window
	 * with 'winfixheight' set. */
	if (height > room + room_cmdline - room_reserved)
	    room_reserved = room + room_cmdline - height;
	/* If there is only a 'winfixheight' window and making the
	 * window smaller, need to make the other window taller. */
	if (take < 0 && room - curfrp->fr_height < room_reserved)
	    room_reserved = 0;

	if (take > 0 && room_cmdline > 0)
	{
	    /* use lines from cmdline first */
	    if (take < room_cmdline)
		room_cmdline = take;
	    take -= room_cmdline;
	    topframe->fr_height += room_cmdline;
	}

	/*
	 * set the current frame to the new height
	 */
	frame_new_height(curfrp, height, FALSE, FALSE);

	/*
	 * First take lines from the frames after the current frame.  If
	 * that is not enough, takes lines from frames above the current
	 * frame.
	 */
	for (run = 0; run < 2; ++run)
	{
	    if (run == 0)
		frp = curfrp->fr_next;	/* 1st run: start with next window */
	    else
		frp = curfrp->fr_prev;	/* 2nd run: start with prev window */
	    while (frp != NULL && take != 0)
	    {
		h = frame_minheight(frp, NULL);
		if (room_reserved > 0
			&& frp->fr_win != NULL
			&& frp->fr_win->w_p_wfh)
		{
		    if (room_reserved >= frp->fr_height)
			room_reserved -= frp->fr_height;
		    else
		    {
			if (frp->fr_height - room_reserved > take)
			    room_reserved = frp->fr_height - take;
			take -= frp->fr_height - room_reserved;
			frame_new_height(frp, room_reserved, FALSE, FALSE);
			room_reserved = 0;
		    }
		}
		else
		{
		    if (frp->fr_height - take < h)
		    {
			take -= frp->fr_height - h;
			frame_new_height(frp, h, FALSE, FALSE);
		    }
		    else
		    {
			frame_new_height(frp, frp->fr_height - take,
								FALSE, FALSE);
			take = 0;
		    }
		}
		if (run == 0)
		    frp = frp->fr_next;
		else
		    frp = frp->fr_prev;
	    }
	}
    }
}

#if defined(FEAT_VERTSPLIT) || defined(PROTO)
/*
 * Set current window width and take care of repositioning other windows to
 * fit around it.
 */
    void
win_setwidth(width)
    int		width;
{
    win_setwidth_win(width, curwin);
}

    void
win_setwidth_win(width, wp)
    int		width;
    win_T	*wp;
{
    /* Always keep current window at least one column wide, even when
     * 'winminwidth' is zero. */
    if (wp == curwin)
    {
	if (width < p_wmw)
	    width = p_wmw;
	if (width == 0)
	    width = 1;
    }

    frame_setwidth(wp->w_frame, width + wp->w_vsep_width);

    /* recompute the window positions */
    (void)win_comp_pos();

    redraw_all_later(NOT_VALID);
}

/*
 * Set the width of a frame to "width" and take care that all frames and
 * windows inside it are resized.  Also resize frames above and below if the
 * are in the same FR_ROW frame.
 *
 * Strategy is similar to frame_setheight().
 */
    static void
frame_setwidth(curfrp, width)
    frame_T	*curfrp;
    int		width;
{
    int		room;		/* total number of lines available */
    int		take;		/* number of lines taken from other windows */
    int		run;
    frame_T	*frp;
    int		w;

    /* If the width already is the desired value, nothing to do. */
    if (curfrp->fr_width == width)
	return;

    if (curfrp->fr_parent == NULL)
	/* topframe: can't change width */
	return;

    if (curfrp->fr_parent->fr_layout == FR_COL)
    {
	/* Column of frames: Also need to resize frames above and below of
	 * this one.  First check for the minimal width of these. */
	w = frame_minwidth(curfrp->fr_parent, NULL);
	if (width < w)
	    width = w;
	frame_setwidth(curfrp->fr_parent, width);
    }
    else
    {
	/*
	 * Row of frames: try to change only frames in this row.
	 *
	 * Do this twice:
	 * 1: compute room available, if it's not enough try resizing the
	 *    containing frame.
	 * 2: compute the room available and adjust the width to it.
	 */
	for (run = 1; run <= 2; ++run)
	{
	    room = 0;
	    for (frp = curfrp->fr_parent->fr_child; frp != NULL;
							   frp = frp->fr_next)
	    {
		room += frp->fr_width;
		if (frp != curfrp)
		    room -= frame_minwidth(frp, NULL);
	    }

	    if (width <= room)
		break;
	    if (run == 2 || curfrp->fr_height >= Rows - p_ch)
	    {
		if (width > room)
		    width = room;
		break;
	    }
	    frame_setwidth(curfrp->fr_parent, width
		 + frame_minwidth(curfrp->fr_parent, NOWIN) - (int)p_wmw - 1);
	}


	/*
	 * Compute the number of lines we will take from others frames (can be
	 * negative!).
	 */
	take = width - curfrp->fr_width;

	/*
	 * set the current frame to the new width
	 */
	frame_new_width(curfrp, width, FALSE);

	/*
	 * First take lines from the frames right of the current frame.  If
	 * that is not enough, takes lines from frames left of the current
	 * frame.
	 */
	for (run = 0; run < 2; ++run)
	{
	    if (run == 0)
		frp = curfrp->fr_next;	/* 1st run: start with next window */
	    else
		frp = curfrp->fr_prev;	/* 2nd run: start with prev window */
	    while (frp != NULL && take != 0)
	    {
		w = frame_minwidth(frp, NULL);
		if (frp->fr_width - take < w)
		{
		    take -= frp->fr_width - w;
		    frame_new_width(frp, w, FALSE);
		}
		else
		{
		    frame_new_width(frp, frp->fr_width - take, FALSE);
		    take = 0;
		}
		if (run == 0)
		    frp = frp->fr_next;
		else
		    frp = frp->fr_prev;
	    }
	}
    }
}
#endif /* FEAT_VERTSPLIT */

/*
 * Check 'winminheight' for a valid value.
 */
    void
win_setminheight()
{
    int		room;
    int		first = TRUE;
    win_T	*wp;

    /* loop until there is a 'winminheight' that is possible */
    while (p_wmh > 0)
    {
	/* TODO: handle vertical splits */
	room = -p_wh;
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	    room += wp->w_height - p_wmh;
	if (room >= 0)
	    break;
	--p_wmh;
	if (first)
	{
	    EMSG(_(e_noroom));
	    first = FALSE;
	}
    }
}

#ifdef FEAT_MOUSE

/*
 * Status line of dragwin is dragged "offset" lines down (negative is up).
 */
    void
win_drag_status_line(dragwin, offset)
    win_T	*dragwin;
    int		offset;
{
    frame_T	*curfr;
    frame_T	*fr;
    int		room;
    int		row;
    int		up;	/* if TRUE, drag status line up, otherwise down */
    int		n;

    fr = dragwin->w_frame;
    curfr = fr;
    if (fr != topframe)		/* more than one window */
    {
	fr = fr->fr_parent;
	/* When the parent frame is not a column of frames, its parent should
	 * be. */
	if (fr->fr_layout != FR_COL)
	{
	    curfr = fr;
	    if (fr != topframe)	/* only a row of windows, may drag statusline */
		fr = fr->fr_parent;
	}
    }

    /* If this is the last frame in a column, may want to resize the parent
     * frame instead (go two up to skip a row of frames). */
    while (curfr != topframe && curfr->fr_next == NULL)
    {
	if (fr != topframe)
	    fr = fr->fr_parent;
	curfr = fr;
	if (fr != topframe)
	    fr = fr->fr_parent;
    }

    if (offset < 0) /* drag up */
    {
	up = TRUE;
	offset = -offset;
	/* sum up the room of the current frame and above it */
	if (fr == curfr)
	{
	    /* only one window */
	    room = fr->fr_height - frame_minheight(fr, NULL);
	}
	else
	{
	    room = 0;
	    for (fr = fr->fr_child; ; fr = fr->fr_next)
	    {
		room += fr->fr_height - frame_minheight(fr, NULL);
		if (fr == curfr)
		    break;
	    }
	}
	fr = curfr->fr_next;		/* put fr at frame that grows */
    }
    else    /* drag down */
    {
	up = FALSE;
	/*
	 * Only dragging the last status line can reduce p_ch.
	 */
	room = Rows - cmdline_row;
	if (curfr->fr_next == NULL)
	    room -= 1;
	else
	    room -= p_ch;
	if (room < 0)
	    room = 0;
	/* sum up the room of frames below of the current one */
	for (fr = curfr->fr_next; fr != NULL; fr = fr->fr_next)
	    room += fr->fr_height - frame_minheight(fr, NULL);
	fr = curfr;			/* put fr at window that grows */
    }

    if (room < offset)		/* Not enough room */
	offset = room;		/* Move as far as we can */
    if (offset <= 0)
	return;

    /*
     * Grow frame fr by "offset" lines.
     * Doesn't happen when dragging the last status line up.
     */
    if (fr != NULL)
	frame_new_height(fr, fr->fr_height + offset, up, FALSE);

    if (up)
	fr = curfr;		/* current frame gets smaller */
    else
	fr = curfr->fr_next;	/* next frame gets smaller */

    /*
     * Now make the other frames smaller.
     */
    while (fr != NULL && offset > 0)
    {
	n = frame_minheight(fr, NULL);
	if (fr->fr_height - offset <= n)
	{
	    offset -= fr->fr_height - n;
	    frame_new_height(fr, n, !up, FALSE);
	}
	else
	{
	    frame_new_height(fr, fr->fr_height - offset, !up, FALSE);
	    break;
	}
	if (up)
	    fr = fr->fr_prev;
	else
	    fr = fr->fr_next;
    }
    row = win_comp_pos();
    screen_fill(row, cmdline_row, 0, (int)Columns, ' ', ' ', 0);
    cmdline_row = row;
    p_ch = Rows - cmdline_row;
    if (p_ch < 1)
	p_ch = 1;
    redraw_all_later(NOT_VALID);
    showmode();
}

#ifdef FEAT_VERTSPLIT
/*
 * Separator line of dragwin is dragged "offset" lines right (negative is left).
 */
    void
win_drag_vsep_line(dragwin, offset)
    win_T	*dragwin;
    int		offset;
{
    frame_T	*curfr;
    frame_T	*fr;
    int		room;
    int		left;	/* if TRUE, drag separator line left, otherwise right */
    int		n;

    fr = dragwin->w_frame;
    if (fr == topframe)		/* only one window (cannot happe?) */
	return;
    curfr = fr;
    fr = fr->fr_parent;
    /* When the parent frame is not a row of frames, its parent should be. */
    if (fr->fr_layout != FR_ROW)
    {
	if (fr == topframe)	/* only a column of windows (cannot happen?) */
	    return;
	curfr = fr;
	fr = fr->fr_parent;
    }

    /* If this is the last frame in a row, may want to resize a parent
     * frame instead. */
    while (curfr->fr_next == NULL)
    {
	if (fr == topframe)
	    break;
	curfr = fr;
	fr = fr->fr_parent;
	if (fr != topframe)
	{
	    curfr = fr;
	    fr = fr->fr_parent;
	}
    }

    if (offset < 0) /* drag left */
    {
	left = TRUE;
	offset = -offset;
	/* sum up the room of the current frame and left of it */
	room = 0;
	for (fr = fr->fr_child; ; fr = fr->fr_next)
	{
	    room += fr->fr_width - frame_minwidth(fr, NULL);
	    if (fr == curfr)
		break;
	}
	fr = curfr->fr_next;		/* put fr at frame that grows */
    }
    else    /* drag right */
    {
	left = FALSE;
	/* sum up the room of frames right of the current one */
	room = 0;
	for (fr = curfr->fr_next; fr != NULL; fr = fr->fr_next)
	    room += fr->fr_width - frame_minwidth(fr, NULL);
	fr = curfr;			/* put fr at window that grows */
    }

    if (room < offset)		/* Not enough room */
	offset = room;		/* Move as far as we can */
    if (offset <= 0)		/* No room at all, quit. */
	return;

    /* grow frame fr by offset lines */
    frame_new_width(fr, fr->fr_width + offset, left);

    /* shrink other frames: current and at the left or at the right */
    if (left)
	fr = curfr;		/* current frame gets smaller */
    else
	fr = curfr->fr_next;	/* next frame gets smaller */

    while (fr != NULL && offset > 0)
    {
	n = frame_minwidth(fr, NULL);
	if (fr->fr_width - offset <= n)
	{
	    offset -= fr->fr_width - n;
	    frame_new_width(fr, n, !left);
	}
	else
	{
	    frame_new_width(fr, fr->fr_width - offset, !left);
	    break;
	}
	if (left)
	    fr = fr->fr_prev;
	else
	    fr = fr->fr_next;
    }
    (void)win_comp_pos();
    redraw_all_later(NOT_VALID);
}
#endif /* FEAT_VERTSPLIT */
#endif /* FEAT_MOUSE */

#endif /* FEAT_WINDOWS */

/*
 * Set the height of a window.
 * This takes care of the things inside the window, not what happens to the
 * window position, the frame or to other windows.
 */
    static void
win_new_height(wp, height)
    win_T	*wp;
    int		height;
{
    linenr_T	lnum;
    int		sline, line_size;
#define FRACTION_MULT	16384L

    /* Don't want a negative height.  Happens when splitting a tiny window.
     * Will equalize heights soon to fix it. */
    if (height < 0)
	height = 0;

    if (wp->w_wrow != wp->w_prev_fraction_row && wp->w_height > 0)
	wp->w_fraction = ((long)wp->w_wrow * FRACTION_MULT
				    + FRACTION_MULT / 2) / (long)wp->w_height;

    wp->w_height = height;
    wp->w_skipcol = 0;

    /* Don't change w_topline when height is zero.  Don't set w_topline when
     * 'scrollbind' is set and this isn't the current window. */
    if (height > 0
#ifdef FEAT_SCROLLBIND
	    && (!wp->w_p_scb || wp == curwin)
#endif
       )
    {
	lnum = wp->w_cursor.lnum;
	if (lnum < 1)		/* can happen when starting up */
	    lnum = 1;
	wp->w_wrow = ((long)wp->w_fraction * (long)height - 1L) / FRACTION_MULT;
	line_size = plines_win_col(wp, lnum, (long)(wp->w_cursor.col)) - 1;
	sline = wp->w_wrow - line_size;
	if (sline < 0)
	{
	    /*
	     * Cursor line would go off top of screen if w_wrow was this high.
	     */
	    wp->w_wrow = line_size;
	}
	else
	{
	    while (sline > 0 && lnum > 1)
	    {
#ifdef FEAT_FOLDING
		hasFoldingWin(wp, lnum, &lnum, NULL, TRUE, NULL);
		if (lnum == 1)
		{
		    /* first line in buffer is folded */
		    line_size = 1;
		    --sline;
		    break;
		}
#endif
		--lnum;
#ifdef FEAT_DIFF
		if (lnum == wp->w_topline)
		    line_size = plines_win_nofill(wp, lnum, TRUE)
							      + wp->w_topfill;
		else
#endif
		    line_size = plines_win(wp, lnum, TRUE);
		sline -= line_size;
	    }
	    if (sline < 0)
	    {
		/*
		 * Line we want at top would go off top of screen.  Use next
		 * line instead.
		 */
#ifdef FEAT_FOLDING
		hasFoldingWin(wp, lnum, NULL, &lnum, TRUE, NULL);
#endif
		lnum++;
		wp->w_wrow -= line_size + sline;
	    }
	    else if (sline > 0)
	    {
		/* First line of file reached, use that as topline. */
		lnum = 1;
		wp->w_wrow -= sline;
	    }
	}
	set_topline(wp, lnum);
    }

    if (wp == curwin)
    {
	if (p_so)
	    update_topline();
	curs_columns(FALSE);	/* validate w_wrow */
    }
    wp->w_prev_fraction_row = wp->w_wrow;

    win_comp_scroll(wp);
    redraw_win_later(wp, NOT_VALID);
#ifdef FEAT_WINDOWS
    wp->w_redr_status = TRUE;
#endif
    invalidate_botline_win(wp);
}

#ifdef FEAT_VERTSPLIT
/*
 * Set the width of a window.
 */
    static void
win_new_width(wp, width)
    win_T	*wp;
    int		width;
{
    wp->w_width = width;
    wp->w_lines_valid = 0;
    changed_line_abv_curs_win(wp);
    invalidate_botline_win(wp);
    if (wp == curwin)
    {
	update_topline();
	curs_columns(TRUE);	/* validate w_wrow */
    }
    redraw_win_later(wp, NOT_VALID);
    wp->w_redr_status = TRUE;
}
#endif

    void
win_comp_scroll(wp)
    win_T	*wp;
{
    wp->w_p_scr = ((unsigned)wp->w_height >> 1);
    if (wp->w_p_scr == 0)
	wp->w_p_scr = 1;
}

/*
 * command_height: called whenever p_ch has been changed
 */
    void
command_height(old_p_ch)
    long	old_p_ch;
{
#ifdef FEAT_WINDOWS
    int		h;
    frame_T	*frp;

    /* Find bottom frame with width of screen. */
    frp = lastwin->w_frame;
# ifdef FEAT_VERTSPLIT
    while (frp->fr_width != Columns && frp->fr_parent != NULL)
	frp = frp->fr_parent;
# endif

    /* Avoid changing the height of a window with 'winfixheight' set. */
    while (frp->fr_prev != NULL && frp->fr_layout == FR_LEAF
						      && frp->fr_win->w_p_wfh)
	frp = frp->fr_prev;

    if (starting != NO_SCREEN)
    {
	cmdline_row = Rows - p_ch;

	if (p_ch > old_p_ch)		    /* p_ch got bigger */
	{
	    while (p_ch > old_p_ch)
	    {
		if (frp == NULL)
		{
		    EMSG(_(e_noroom));
		    p_ch = old_p_ch;
		    cmdline_row = Rows - p_ch;
		    break;
		}
		h = frp->fr_height - frame_minheight(frp, NULL);
		if (h > p_ch - old_p_ch)
		    h = p_ch - old_p_ch;
		old_p_ch += h;
		frame_add_height(frp, -h);
		frp = frp->fr_prev;
	    }

	    /* Recompute window positions. */
	    (void)win_comp_pos();

	    /* clear the lines added to cmdline */
	    if (full_screen)
		screen_fill((int)(cmdline_row), (int)Rows, 0,
						   (int)Columns, ' ', ' ', 0);
	    msg_row = cmdline_row;
	    redraw_cmdline = TRUE;
	    return;
	}

	if (msg_row < cmdline_row)
	    msg_row = cmdline_row;
	redraw_cmdline = TRUE;
    }
    frame_add_height(frp, (int)(old_p_ch - p_ch));

    /* Recompute window positions. */
    if (frp != lastwin->w_frame)
	(void)win_comp_pos();
#else
    win_setheight((int)(firstwin->w_height + old_p_ch - p_ch));
    cmdline_row = Rows - p_ch;
#endif
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
/*
 * Resize frame "frp" to be "n" lines higher (negative for less high).
 * Also resize the frames it is contained in.
 */
    static void
frame_add_height(frp, n)
    frame_T	*frp;
    int		n;
{
    frame_new_height(frp, frp->fr_height + n, FALSE, FALSE);
    for (;;)
    {
	frp = frp->fr_parent;
	if (frp == NULL)
	    break;
	frp->fr_height += n;
    }
}

/*
 * Add or remove a status line for the bottom window(s), according to the
 * value of 'laststatus'.
 */
    void
last_status(morewin)
    int		morewin;	/* pretend there are two or more windows */
{
    /* Don't make a difference between horizontal or vertical split. */
    last_status_rec(topframe, (p_ls == 2
			  || (p_ls == 1 && (morewin || lastwin != firstwin))));
}

    static void
last_status_rec(fr, statusline)
    frame_T	*fr;
    int		statusline;
{
    frame_T	*fp;
    win_T	*wp;

    if (fr->fr_layout == FR_LEAF)
    {
	wp = fr->fr_win;
	if (wp->w_status_height != 0 && !statusline)
	{
	    /* remove status line */
	    win_new_height(wp, wp->w_height + 1);
	    wp->w_status_height = 0;
	    comp_col();
	}
	else if (wp->w_status_height == 0 && statusline)
	{
	    /* Find a frame to take a line from. */
	    fp = fr;
	    while (fp->fr_height <= frame_minheight(fp, NULL))
	    {
		if (fp == topframe)
		{
		    EMSG(_(e_noroom));
		    return;
		}
		/* In a column of frames: go to frame above.  If already at
		 * the top or in a row of frames: go to parent. */
		if (fp->fr_parent->fr_layout == FR_COL && fp->fr_prev != NULL)
		    fp = fp->fr_prev;
		else
		    fp = fp->fr_parent;
	    }
	    wp->w_status_height = 1;
	    if (fp != fr)
	    {
		frame_new_height(fp, fp->fr_height - 1, FALSE, FALSE);
		frame_fix_height(wp);
		(void)win_comp_pos();
	    }
	    else
		win_new_height(wp, wp->w_height - 1);
	    comp_col();
	    redraw_all_later(NOT_VALID);
	}
    }
#ifdef FEAT_VERTSPLIT
    else if (fr->fr_layout == FR_ROW)
    {
	/* vertically split windows, set status line for each one */
	for (fp = fr->fr_child; fp != NULL; fp = fp->fr_next)
	    last_status_rec(fp, statusline);
    }
#endif
    else
    {
	/* horizontally split window, set status line for last one */
	for (fp = fr->fr_child; fp->fr_next != NULL; fp = fp->fr_next)
	    ;
	last_status_rec(fp, statusline);
    }
}

#endif /* FEAT_WINDOWS */

#if defined(FEAT_SEARCHPATH) || defined(PROTO)
/*
 * Return the file name under or after the cursor.
 *
 * The 'path' option is searched if the file name is not absolute.
 * The string returned has been alloc'ed and should be freed by the caller.
 * NULL is returned if the file name or file is not found.
 *
 * options:
 * FNAME_MESS	    give error messages
 * FNAME_EXP	    expand to path
 * FNAME_HYP	    check for hypertext link
 * FNAME_INCL	    apply "includeexpr"
 */
    char_u *
file_name_at_cursor(options, count)
    int	    options;
    long    count;
{
    return file_name_in_line(ml_get_curline(),
		      curwin->w_cursor.col, options, count, curbuf->b_ffname);
}

/*
 * Return the name of the file under or after ptr[col].
 * Otherwise like file_name_at_cursor().
 */
    char_u *
file_name_in_line(line, col, options, count, rel_fname)
    char_u	*line;
    int		col;
    int		options;
    long	count;
    char_u	*rel_fname;	/* file we are searching relative to */
{
    char_u	*ptr;
    int		len;

    /*
     * search forward for what could be the start of a file name
     */
    ptr = line + col;
    while (*ptr != NUL && !vim_isfilec(*ptr))
	++ptr;
    if (*ptr == NUL)		/* nothing found */
    {
	if (options & FNAME_MESS)
	    EMSG(_("E446: No file name under cursor"));
	return NULL;
    }

    /*
     * Search backward for first char of the file name.
     * Go one char back to ":" before "//" even when ':' is not in 'isfname'.
     */
    while (ptr > line)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte && (len = (*mb_head_off)(line, ptr - 1)) > 0)
	    ptr -= len + 1;
	else
#endif
	if (vim_isfilec(ptr[-1])
		|| ((options & FNAME_HYP) && path_is_url(ptr - 1)))
	    --ptr;
	else
	    break;
    }

    /*
     * Search forward for the last char of the file name.
     * Also allow "://" when ':' is not in 'isfname'.
     */
    len = 0;
    while (vim_isfilec(ptr[len])
			 || ((options & FNAME_HYP) && path_is_url(ptr + len)))
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    len += (*mb_ptr2len_check)(ptr + len);
	else
#endif
	    ++len;

    /*
     * If there is trailing punctuation, remove it.
     * But don't remove "..", could be a directory name.
     */
    if (len > 2 && vim_strchr((char_u *)".,:;!", ptr[len - 1]) != NULL
						       && ptr[len - 2] != '.')
	--len;

    return find_file_name_in_path(ptr, len, options, count, rel_fname);
}

# if defined(FEAT_FIND_ID) && defined(FEAT_EVAL)
static char_u *eval_includeexpr __ARGS((char_u *ptr, int len));

    static char_u *
eval_includeexpr(ptr, len)
    char_u	*ptr;
    int		len;
{
    char_u	*res;

    set_vim_var_string(VV_FNAME, ptr, len);
    res = eval_to_string_safe(curbuf->b_p_inex, NULL);
    set_vim_var_string(VV_FNAME, NULL, 0);
    return res;
}
#endif

/*
 * Return the name of the file ptr[len] in 'path'.
 * Otherwise like file_name_at_cursor().
 */
    char_u *
find_file_name_in_path(ptr, len, options, count, rel_fname)
    char_u	*ptr;
    int		len;
    int		options;
    long	count;
    char_u	*rel_fname;	/* file we are searching relative to */
{
    char_u	*file_name;
    int		c;
# if defined(FEAT_FIND_ID) && defined(FEAT_EVAL)
    char_u	*tofree = NULL;

    if ((options & FNAME_INCL) && *curbuf->b_p_inex != NUL)
    {
	tofree = eval_includeexpr(ptr, len);
	if (tofree != NULL)
	{
	    ptr = tofree;
	    len = (int)STRLEN(ptr);
	}
    }
# endif

    if (options & FNAME_EXP)
    {
	file_name = find_file_in_path(ptr, len, options & ~FNAME_MESS,
							     TRUE, rel_fname);

# if defined(FEAT_FIND_ID) && defined(FEAT_EVAL)
	/*
	 * If the file could not be found in a normal way, try applying
	 * 'includeexpr' (unless done already).
	 */
	if (file_name == NULL
		&& !(options & FNAME_INCL) && *curbuf->b_p_inex != NUL)
	{
	    tofree = eval_includeexpr(ptr, len);
	    if (tofree != NULL)
	    {
		ptr = tofree;
		len = (int)STRLEN(ptr);
		file_name = find_file_in_path(ptr, len, options & ~FNAME_MESS,
							     TRUE, rel_fname);
	    }
	}
# endif
	if (file_name == NULL && (options & FNAME_MESS))
	{
	    c = ptr[len];
	    ptr[len] = NUL;
	    EMSG2(_("E447: Can't find file \"%s\" in path"), ptr);
	    ptr[len] = c;
	}

	/* Repeat finding the file "count" times.  This matters when it
	 * appears several times in the path. */
	while (file_name != NULL && --count > 0)
	{
	    vim_free(file_name);
	    file_name = find_file_in_path(ptr, len, options, FALSE, rel_fname);
	}
    }
    else
	file_name = vim_strnsave(ptr, len);

# if defined(FEAT_FIND_ID) && defined(FEAT_EVAL)
    vim_free(tofree);
# endif

    return file_name;
}
#endif /* FEAT_SEARCHPATH */

/*
 * Check if the "://" of a URL is at the pointer, return URL_SLASH.
 * Also check for ":\\", which MS Internet Explorer accepts, return
 * URL_BACKSLASH.
 */
    static int
path_is_url(p)
    char_u  *p;
{
    if (STRNCMP(p, "://", (size_t)3) == 0)
	return URL_SLASH;
    else if (STRNCMP(p, ":\\\\", (size_t)3) == 0)
	return URL_BACKSLASH;
    return 0;
}

/*
 * Check if "fname" starts with "name://".  Return URL_SLASH if it does.
 * Return URL_BACKSLASH for "name:\\".
 * Return zero otherwise.
 */
    int
path_with_url(fname)
    char_u *fname;
{
    char_u *p;

    for (p = fname; isalpha(*p); ++p)
	;
    return path_is_url(p);
}

/*
 * Return TRUE if "name" is a full (absolute) path name or URL.
 */
    int
vim_isAbsName(name)
    char_u	*name;
{
    return (path_with_url(name) != 0 || mch_isFullName(name));
}

/*
 * Get absolute file name into buffer 'buf' of length 'len' bytes.
 *
 * return FAIL for failure, OK otherwise
 */
    int
vim_FullName(fname, buf, len, force)
    char_u	*fname, *buf;
    int		len;
    int		force;
{
    int		retval = OK;
    int		url;

    *buf = NUL;
    if (fname == NULL)
	return FAIL;

    url = path_with_url(fname);
    if (!url)
	retval = mch_FullName(fname, buf, len, force);
    if (url || retval == FAIL)
    {
	/* something failed; use the file name (truncate when too long) */
	STRNCPY(buf, fname, len);
	buf[len - 1] = NUL;
    }
#if defined(MACOS_CLASSIC) || defined(OS2) || defined(MSDOS) || defined(MSWIN)
    slash_adjust(buf);
#endif
    return retval;
}

/*
 * Return the minimal number of rows that is needed on the screen to display
 * the current number of windows.
 */
    int
min_rows()
{
    int		total;

    if (firstwin == NULL)	/* not initialized yet */
	return MIN_LINES;

    total = 1;		/* count the room for the command line */
#ifdef FEAT_WINDOWS
    total += frame_minheight(topframe, NULL);
#else
    total += 1;		/* at least one window should have a line! */
#endif
    return total;
}

/*
 * Return TRUE if there is only one window, not counting a help or preview
 * window, unless it is the current window.
 */
    int
only_one_window()
{
#ifdef FEAT_WINDOWS
    int		count = 0;
    win_T	*wp;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (!(wp->w_buffer->b_help
# ifdef FEAT_QUICKFIX
		    || wp->w_p_pvw
# endif
	     ) || wp == curwin)
	    ++count;
    return (count <= 1);
#else
    return TRUE;
#endif
}

#if defined(FEAT_WINDOWS) || defined(FEAT_AUTOCMD) || defined(PROTO)
/*
 * Correct the cursor line number in other windows.  Used after changing the
 * current buffer, and before applying autocommands.
 * When "do_curwin" is TRUE, also check current window.
 */
    void
check_lnums(do_curwin)
    int		do_curwin;
{
    win_T	*wp;

#ifdef FEAT_WINDOWS
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if ((do_curwin || wp != curwin) && wp->w_buffer == curbuf)
#else
    wp = curwin;
    if (do_curwin)
#endif
	{
	    if (wp->w_cursor.lnum > curbuf->b_ml.ml_line_count)
		wp->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	    if (wp->w_topline > curbuf->b_ml.ml_line_count)
		wp->w_topline = curbuf->b_ml.ml_line_count;
	}
}
#endif

#if defined(FEAT_WINDOWS) || defined(PROTO)

/*
 * A snapshot of the window sizes, to restore them after closing the help
 * window.
 * Only these fields are used:
 * fr_layout
 * fr_width
 * fr_height
 * fr_next
 * fr_child
 * fr_win (only valid for the old curwin, NULL otherwise)
 */
static frame_T *snapshot = NULL;

/*
 * Create a snapshot of the current frame sizes.
 */
    static void
make_snapshot()
{
    clear_snapshot();
    make_snapshot_rec(topframe, &snapshot);
}

    static void
make_snapshot_rec(fr, frp)
    frame_T	*fr;
    frame_T	**frp;
{
    *frp = (frame_T *)alloc_clear((unsigned)sizeof(frame_T));
    if (*frp == NULL)
	return;
    (*frp)->fr_layout = fr->fr_layout;
# ifdef FEAT_VERTSPLIT
    (*frp)->fr_width = fr->fr_width;
# endif
    (*frp)->fr_height = fr->fr_height;
    if (fr->fr_next != NULL)
	make_snapshot_rec(fr->fr_next, &((*frp)->fr_next));
    if (fr->fr_child != NULL)
	make_snapshot_rec(fr->fr_child, &((*frp)->fr_child));
    if (fr->fr_layout == FR_LEAF && fr->fr_win == curwin)
	(*frp)->fr_win = curwin;
}

/*
 * Remove any existing snapshot.
 */
    static void
clear_snapshot()
{
    clear_snapshot_rec(snapshot);
    snapshot = NULL;
}

    static void
clear_snapshot_rec(fr)
    frame_T	*fr;
{
    if (fr != NULL)
    {
	clear_snapshot_rec(fr->fr_next);
	clear_snapshot_rec(fr->fr_child);
	vim_free(fr);
    }
}

/*
 * Restore a previously created snapshot, if there is any.
 * This is only done if the screen size didn't change and the window layout is
 * still the same.
 */
    static void
restore_snapshot(close_curwin)
    int		close_curwin;	    /* closing current window */
{
    win_T	*wp;

    if (snapshot != NULL
# ifdef FEAT_VERTSPLIT
	    && snapshot->fr_width == topframe->fr_width
# endif
	    && snapshot->fr_height == topframe->fr_height
	    && check_snapshot_rec(snapshot, topframe) == OK)
    {
	wp = restore_snapshot_rec(snapshot, topframe);
	win_comp_pos();
	if (wp != NULL && close_curwin)
	    win_goto(wp);
	redraw_all_later(CLEAR);
    }
    clear_snapshot();
}

/*
 * Check if frames "sn" and "fr" have the same layout, same following frames
 * and same children.
 */
    static int
check_snapshot_rec(sn, fr)
    frame_T	*sn;
    frame_T	*fr;
{
    if (sn->fr_layout != fr->fr_layout
	    || (sn->fr_next == NULL) != (fr->fr_next == NULL)
	    || (sn->fr_child == NULL) != (fr->fr_child == NULL)
	    || (sn->fr_next != NULL
		&& check_snapshot_rec(sn->fr_next, fr->fr_next) == FAIL)
	    || (sn->fr_child != NULL
		&& check_snapshot_rec(sn->fr_child, fr->fr_child) == FAIL))
	return FAIL;
    return OK;
}

/*
 * Copy the size of snapshot frame "sn" to frame "fr".  Do the same for all
 * following frames and children.
 * Returns a pointer to the old current window, or NULL.
 */
    static win_T *
restore_snapshot_rec(sn, fr)
    frame_T	*sn;
    frame_T	*fr;
{
    win_T	*wp = NULL;
    win_T	*wp2;

    fr->fr_height = sn->fr_height;
# ifdef FEAT_VERTSPLIT
    fr->fr_width = sn->fr_width;
# endif
    if (fr->fr_layout == FR_LEAF)
    {
	frame_new_height(fr, fr->fr_height, FALSE, FALSE);
# ifdef FEAT_VERTSPLIT
	frame_new_width(fr, fr->fr_width, FALSE);
# endif
	wp = sn->fr_win;
    }
    if (sn->fr_next != NULL)
    {
	wp2 = restore_snapshot_rec(sn->fr_next, fr->fr_next);
	if (wp2 != NULL)
	    wp = wp2;
    }
    if (sn->fr_child != NULL)
    {
	wp2 = restore_snapshot_rec(sn->fr_child, fr->fr_child);
	if (wp2 != NULL)
	    wp = wp2;
    }
    return wp;
}

#endif

#if (defined(FEAT_GUI) && defined(FEAT_VERTSPLIT)) || defined(PROTO)
/*
 * Return TRUE if there is any vertically split window.
 */
    int
win_hasvertsplit()
{
    frame_T	*fr;

    if (topframe->fr_layout == FR_ROW)
	return TRUE;

    if (topframe->fr_layout == FR_COL)
	for (fr = topframe->fr_child; fr != NULL; fr = fr->fr_next)
	    if (fr->fr_layout == FR_ROW)
		return TRUE;

    return FALSE;
}
#endif
