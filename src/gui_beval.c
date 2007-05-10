/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#if defined(FEAT_BEVAL) || defined(PROTO)

/*
 * Common code, invoked when the mouse is resting for a moment.
 */
/*ARGSUSED*/
    void
general_beval_cb(beval, state)
    BalloonEval *beval;
    int state;
{
    win_T	*wp;
    int		col;
    int		use_sandbox;
    linenr_T	lnum;
    char_u	*text;
    static char_u  *result = NULL;
    long	winnr = 0;
    char_u	*bexpr;
    buf_T	*save_curbuf;
#ifdef FEAT_WINDOWS
    win_T	*cw;
#endif
    static int	recursive = FALSE;

    /* Don't do anything when 'ballooneval' is off, messages scrolled the
     * windows up or we have no beval area. */
    if (!p_beval || balloonEval == NULL || msg_scrolled > 0)
	return;

    /* Don't do this recursively.  Happens when the expression evaluation
     * takes a long time and invokes something that checks for CTRL-C typed. */
    if (recursive)
	return;
    recursive = TRUE;

#ifdef FEAT_EVAL
    if (get_beval_info(balloonEval, TRUE, &wp, &lnum, &text, &col) == OK)
    {
	bexpr = (*wp->w_buffer->b_p_bexpr == NUL) ? p_bexpr
						    : wp->w_buffer->b_p_bexpr;
	if (*bexpr != NUL)
	{
# ifdef FEAT_WINDOWS
	    /* Convert window pointer to number. */
	    for (cw = firstwin; cw != wp; cw = cw->w_next)
		++winnr;
# endif

	    set_vim_var_nr(VV_BEVAL_BUFNR, (long)wp->w_buffer->b_fnum);
	    set_vim_var_nr(VV_BEVAL_WINNR, winnr);
	    set_vim_var_nr(VV_BEVAL_LNUM, (long)lnum);
	    set_vim_var_nr(VV_BEVAL_COL, (long)(col + 1));
	    set_vim_var_string(VV_BEVAL_TEXT, text, -1);
	    vim_free(text);

	    /*
	     * Temporarily change the curbuf, so that we can determine whether
	     * the buffer-local balloonexpr option was set insecurely.
	     */
	    save_curbuf = curbuf;
	    curbuf = wp->w_buffer;
	    use_sandbox = was_set_insecurely((char_u *)"balloonexpr",
				 *curbuf->b_p_bexpr == NUL ? 0 : OPT_LOCAL);
	    curbuf = save_curbuf;
	    if (use_sandbox)
		++sandbox;
	    ++textlock;

	    vim_free(result);
	    result = eval_to_string(bexpr, NULL, TRUE);

	    if (use_sandbox)
		--sandbox;
	    --textlock;

	    set_vim_var_string(VV_BEVAL_TEXT, NULL, -1);
	    if (result != NULL && result[0] != NUL)
	    {
		gui_mch_post_balloon(beval, result);
		recursive = FALSE;
		return;
	    }
	}
    }
#endif
#ifdef FEAT_NETBEANS_INTG
    if (bevalServers & BEVAL_NETBEANS)
	netbeans_beval_cb(beval, state);
#endif
#ifdef FEAT_SUN_WORKSHOP
    if (bevalServers & BEVAL_WORKSHOP)
	workshop_beval_cb(beval, state);
#endif

    recursive = FALSE;
}

/* on Win32 only get_beval_info() is required */
#if !defined(FEAT_GUI_W32) || defined(PROTO)

#ifdef FEAT_GUI_GTK
# include <gdk/gdkkeysyms.h>
# include <gtk/gtk.h>
#else
# include <X11/keysym.h>
# ifdef FEAT_GUI_MOTIF
#  include <Xm/PushB.h>
#  include <Xm/Separator.h>
#  include <Xm/List.h>
#  include <Xm/Label.h>
#  include <Xm/AtomMgr.h>
#  include <Xm/Protocols.h>
# else
   /* Assume Athena */
#  include <X11/Shell.h>
#  ifdef FEAT_GUI_NEXTAW
#   include <X11/neXtaw/Label.h>
#  else
#   include <X11/Xaw/Label.h>
#  endif
# endif
#endif

#include "gui_beval.h"

#ifndef FEAT_GUI_GTK
extern Widget vimShell;

/*
 * Currently, we assume that there can be only one BalloonEval showing
 * on-screen at any given moment.  This variable will hold the currently
 * showing BalloonEval or NULL if none is showing.
 */
static BalloonEval *current_beval = NULL;
#endif

#ifdef FEAT_GUI_GTK
static void addEventHandler __ARGS((GtkWidget *, BalloonEval *));
static void removeEventHandler __ARGS((BalloonEval *));
static gint target_event_cb __ARGS((GtkWidget *, GdkEvent *, gpointer));
static gint mainwin_event_cb __ARGS((GtkWidget *, GdkEvent *, gpointer));
static void pointer_event __ARGS((BalloonEval *, int, int, unsigned));
static void key_event __ARGS((BalloonEval *, unsigned, int));
static gint timeout_cb __ARGS((gpointer));
static gint balloon_expose_event_cb __ARGS((GtkWidget *, GdkEventExpose *, gpointer));
# ifndef HAVE_GTK2
static void balloon_draw_cb __ARGS((GtkWidget *, GdkRectangle *, gpointer));
# endif
#else
static void addEventHandler __ARGS((Widget, BalloonEval *));
static void removeEventHandler __ARGS((BalloonEval *));
static void pointerEventEH __ARGS((Widget, XtPointer, XEvent *, Boolean *));
static void pointerEvent __ARGS((BalloonEval *, XEvent *));
static void timerRoutine __ARGS((XtPointer, XtIntervalId *));
#endif
static void cancelBalloon __ARGS((BalloonEval *));
static void requestBalloon __ARGS((BalloonEval *));
static void drawBalloon __ARGS((BalloonEval *));
static void undrawBalloon __ARGS((BalloonEval *beval));
static void createBalloonEvalWindow __ARGS((BalloonEval *));



/*
 * Create a balloon-evaluation area for a Widget.
 * There can be either a "mesg" for a fixed string or "mesgCB" to generate a
 * message by calling this callback function.
 * When "mesg" is not NULL it must remain valid for as long as the balloon is
 * used.  It is not freed here.
 * Returns a pointer to the resulting object (NULL when out of memory).
 */
    BalloonEval *
gui_mch_create_beval_area(target, mesg, mesgCB, clientData)
    void	*target;
    char_u	*mesg;
    void	(*mesgCB)__ARGS((BalloonEval *, int));
    void	*clientData;
{
#ifndef FEAT_GUI_GTK
    char	*display_name;	    /* get from gui.dpy */
    int		screen_num;
    char	*p;
#endif
    BalloonEval	*beval;

    if (mesg != NULL && mesgCB != NULL)
    {
	EMSG(_("E232: Cannot create BalloonEval with both message and callback"));
	return NULL;
    }

    beval = (BalloonEval *)alloc(sizeof(BalloonEval));
    if (beval != NULL)
    {
#ifdef FEAT_GUI_GTK
	beval->target = GTK_WIDGET(target);
	beval->balloonShell = NULL;
	beval->timerID = 0;
#else
	beval->target = (Widget)target;
	beval->balloonShell = NULL;
	beval->timerID = (XtIntervalId)NULL;
	beval->appContext = XtWidgetToApplicationContext((Widget)target);
#endif
	beval->showState = ShS_NEUTRAL;
	beval->x = 0;
	beval->y = 0;
	beval->msg = mesg;
	beval->msgCB = mesgCB;
	beval->clientData = clientData;

	/*
	 * Set up event handler which will keep its eyes on the pointer,
	 * and when the pointer rests in a certain spot for a given time
	 * interval, show the beval.
	 */
	addEventHandler(beval->target, beval);
	createBalloonEvalWindow(beval);

#ifndef FEAT_GUI_GTK
	/*
	 * Now create and save the screen width and height. Used in drawing.
	 */
	display_name = DisplayString(gui.dpy);
	p = strrchr(display_name, '.');
	if (p != NULL)
	    screen_num = atoi(++p);
	else
	    screen_num = 0;
	beval->screen_width = DisplayWidth(gui.dpy, screen_num);
	beval->screen_height = DisplayHeight(gui.dpy, screen_num);
#endif
    }

    return beval;
}

#if defined(FEAT_BEVAL_TIP) || defined(PROTO)
/*
 * Destroy a balloon-eval and free its associated memory.
 */
    void
gui_mch_destroy_beval_area(beval)
    BalloonEval	*beval;
{
    cancelBalloon(beval);
    removeEventHandler(beval);
    /* Children will automatically be destroyed */
# ifdef FEAT_GUI_GTK
    gtk_widget_destroy(beval->balloonShell);
# else
    XtDestroyWidget(beval->balloonShell);
# endif
    vim_free(beval);
}
#endif

    void
gui_mch_enable_beval_area(beval)
    BalloonEval	*beval;
{
    if (beval != NULL)
	addEventHandler(beval->target, beval);
}

    void
gui_mch_disable_beval_area(beval)
    BalloonEval	*beval;
{
    if (beval != NULL)
	removeEventHandler(beval);
}

#if defined(FEAT_BEVAL_TIP) || defined(PROTO)
/*
 * This function returns the BalloonEval * associated with the currently
 * displayed tooltip.  Returns NULL if there is no tooltip currently showing.
 *
 * Assumption: Only one tooltip can be shown at a time.
 */
    BalloonEval *
gui_mch_currently_showing_beval()
{
    return current_beval;
}
#endif
#endif /* !FEAT_GUI_W32 */

#if defined(FEAT_SUN_WORKSHOP) || defined(FEAT_NETBEANS_INTG) \
    || defined(FEAT_EVAL) || defined(PROTO)
/*
 * Get the text and position to be evaluated for "beval".
 * If "getword" is true the returned text is not the whole line but the
 * relevant word in allocated memory.
 * Returns OK or FAIL.
 */
    int
get_beval_info(beval, getword, winp, lnump, textp, colp)
    BalloonEval	*beval;
    int		getword;
    win_T	**winp;
    linenr_T	*lnump;
    char_u	**textp;
    int		*colp;
{
    win_T	*wp;
    int		row, col;
    char_u	*lbuf;
    linenr_T	lnum;

    *textp = NULL;
    row = Y_2_ROW(beval->y);
    col = X_2_COL(beval->x);
#ifdef FEAT_WINDOWS
    wp = mouse_find_win(&row, &col);
#else
    wp = firstwin;
#endif
    if (wp != NULL && row < wp->w_height && col < W_WIDTH(wp))
    {
	/* Found a window and the cursor is in the text.  Now find the line
	 * number. */
	if (!mouse_comp_pos(wp, &row, &col, &lnum))
	{
	    /* Not past end of the file. */
	    lbuf = ml_get_buf(wp->w_buffer, lnum, FALSE);
	    if (col <= win_linetabsize(wp, lbuf, (colnr_T)MAXCOL))
	    {
		/* Not past end of line. */
		if (getword)
		{
		    /* For Netbeans we get the relevant part of the line
		     * instead of the whole line. */
		    int		len;
		    pos_T	*spos = NULL, *epos = NULL;

		    if (VIsual_active)
		    {
			if (lt(VIsual, curwin->w_cursor))
			{
			    spos = &VIsual;
			    epos = &curwin->w_cursor;
			}
			else
			{
			    spos = &curwin->w_cursor;
			    epos = &VIsual;
			}
		    }

		    col = vcol2col(wp, lnum, col) - 1;

		    if (VIsual_active
			    && wp->w_buffer == curwin->w_buffer
			    && (lnum == spos->lnum
				? col >= (int)spos->col
				: lnum > spos->lnum)
			    && (lnum == epos->lnum
				? col <= (int)epos->col
				: lnum < epos->lnum))
		    {
			/* Visual mode and pointing to the line with the
			 * Visual selection: return selected text, with a
			 * maximum of one line. */
			if (spos->lnum != epos->lnum || spos->col == epos->col)
			    return FAIL;

			lbuf = ml_get_buf(curwin->w_buffer, VIsual.lnum, FALSE);
			lbuf = vim_strnsave(lbuf + spos->col,
				     epos->col - spos->col + (*p_sel != 'e'));
			lnum = spos->lnum;
			col = spos->col;
		    }
		    else
		    {
			/* Find the word under the cursor. */
			++emsg_off;
			len = find_ident_at_pos(wp, lnum, (colnr_T)col, &lbuf,
					FIND_IDENT + FIND_STRING + FIND_EVAL);
			--emsg_off;
			if (len == 0)
			    return FAIL;
			lbuf = vim_strnsave(lbuf, len);
		    }
		}

		*winp = wp;
		*lnump = lnum;
		*textp = lbuf;
		*colp = col;
		beval->ts = wp->w_buffer->b_p_ts;
		return OK;
	    }
	}
    }

    return FAIL;
}

# if !defined(FEAT_GUI_W32) || defined(PROTO)

/*
 * Show a balloon with "mesg".
 */
    void
gui_mch_post_balloon(beval, mesg)
    BalloonEval	*beval;
    char_u	*mesg;
{
    beval->msg = mesg;
    if (mesg != NULL)
	drawBalloon(beval);
    else
	undrawBalloon(beval);
}
# endif /* FEAT_GUI_W32 */
#endif /* FEAT_SUN_WORKSHOP || FEAT_NETBEANS_INTG || PROTO */

#if !defined(FEAT_GUI_W32) || defined(PROTO)
#if defined(FEAT_BEVAL_TIP) || defined(PROTO)
/*
 * Hide the given balloon.
 */
    void
gui_mch_unpost_balloon(beval)
    BalloonEval	*beval;
{
    undrawBalloon(beval);
}
#endif

#ifdef FEAT_GUI_GTK
/*
 * We can unconditionally use ANSI-style prototypes here since
 * GTK+ requires an ANSI C compiler anyway.
 */
    static void
addEventHandler(GtkWidget *target, BalloonEval *beval)
{
    /*
     * Connect to the generic "event" signal instead of the individual
     * signals for each event type, because the former is emitted earlier.
     * This allows us to catch events independently of the signal handlers
     * in gui_gtk_x11.c.
     */
    /* Should use GTK_OBJECT() here, but that causes a lint warning... */
    gtk_signal_connect((GtkObject*)(target), "event",
		       GTK_SIGNAL_FUNC(target_event_cb),
		       beval);
    /*
     * Nasty:  Key press events go to the main window thus the drawing area
     * will never see them.  This means we have to connect to the main window
     * as well in order to catch those events.
     */
    if (gtk_socket_id == 0 && gui.mainwin != NULL
	    && gtk_widget_is_ancestor(target, gui.mainwin))
    {
	gtk_signal_connect((GtkObject*)(gui.mainwin), "event",
			   GTK_SIGNAL_FUNC(mainwin_event_cb),
			   beval);
    }
}

    static void
removeEventHandler(BalloonEval *beval)
{
    /* LINTED: avoid warning: dubious operation on enum */
    gtk_signal_disconnect_by_func((GtkObject*)(beval->target),
				  GTK_SIGNAL_FUNC(target_event_cb),
				  beval);

    if (gtk_socket_id == 0 && gui.mainwin != NULL
	    && gtk_widget_is_ancestor(beval->target, gui.mainwin))
    {
	/* LINTED: avoid warning: dubious operation on enum */
	gtk_signal_disconnect_by_func((GtkObject*)(gui.mainwin),
				      GTK_SIGNAL_FUNC(mainwin_event_cb),
				      beval);
    }
}

    static gint
target_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    BalloonEval *beval = (BalloonEval *)data;

    switch (event->type)
    {
	case GDK_ENTER_NOTIFY:
	    pointer_event(beval, (int)event->crossing.x,
				 (int)event->crossing.y,
				 event->crossing.state);
	    break;
	case GDK_MOTION_NOTIFY:
	    if (event->motion.is_hint)
	    {
		int		x;
		int		y;
		GdkModifierType	state;
		/*
		 * GDK_POINTER_MOTION_HINT_MASK is set, thus we cannot obtain
		 * the coordinates from the GdkEventMotion struct directly.
		 */
		gdk_window_get_pointer(widget->window, &x, &y, &state);
		pointer_event(beval, x, y, (unsigned int)state);
	    }
	    else
	    {
		pointer_event(beval, (int)event->motion.x,
				     (int)event->motion.y,
				     event->motion.state);
	    }
	    break;
	case GDK_LEAVE_NOTIFY:
	    /*
	     * Ignore LeaveNotify events that are not "normal".
	     * Apparently we also get it when somebody else grabs focus.
	     */
	    if (event->crossing.mode == GDK_CROSSING_NORMAL)
		cancelBalloon(beval);
	    break;
	case GDK_BUTTON_PRESS:
# ifdef HAVE_GTK2
	case GDK_SCROLL:
# endif
	    cancelBalloon(beval);
	    break;
	case GDK_KEY_PRESS:
	    key_event(beval, event->key.keyval, TRUE);
	    break;
	case GDK_KEY_RELEASE:
	    key_event(beval, event->key.keyval, FALSE);
	    break;
	default:
	    break;
    }

    return FALSE; /* continue emission */
}

/*ARGSUSED*/
    static gint
mainwin_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    BalloonEval *beval = (BalloonEval *)data;

    switch (event->type)
    {
	case GDK_KEY_PRESS:
	    key_event(beval, event->key.keyval, TRUE);
	    break;
	case GDK_KEY_RELEASE:
	    key_event(beval, event->key.keyval, FALSE);
	    break;
	default:
	    break;
    }

    return FALSE; /* continue emission */
}

    static void
pointer_event(BalloonEval *beval, int x, int y, unsigned state)
{
    int distance;

    distance = ABS(x - beval->x) + ABS(y - beval->y);

    if (distance > 4)
    {
	/*
	 * Moved out of the balloon location: cancel it.
	 * Remember button state
	 */
	beval->state = state;
	cancelBalloon(beval);

	/* Mouse buttons are pressed - no balloon now */
	if (!(state & ((int)GDK_BUTTON1_MASK | (int)GDK_BUTTON2_MASK
						    | (int)GDK_BUTTON3_MASK)))
	{
	    beval->x = x;
	    beval->y = y;

	    if (state & (int)GDK_MOD1_MASK)
	    {
		/*
		 * Alt is pressed -- enter super-evaluate-mode,
		 * where there is no time delay
		 */
		if (beval->msgCB != NULL)
		{
		    beval->showState = ShS_PENDING;
		    (*beval->msgCB)(beval, state);
		}
	    }
	    else
	    {
		beval->timerID = gtk_timeout_add((guint32)p_bdlay,
						 &timeout_cb, beval);
	    }
	}
    }
}

    static void
key_event(BalloonEval *beval, unsigned keyval, int is_keypress)
{
    if (beval->showState == ShS_SHOWING && beval->msgCB != NULL)
    {
	switch (keyval)
	{
	    case GDK_Shift_L:
	    case GDK_Shift_R:
		beval->showState = ShS_UPDATE_PENDING;
		(*beval->msgCB)(beval, (is_keypress)
						   ? (int)GDK_SHIFT_MASK : 0);
		break;
	    case GDK_Control_L:
	    case GDK_Control_R:
		beval->showState = ShS_UPDATE_PENDING;
		(*beval->msgCB)(beval, (is_keypress)
						 ? (int)GDK_CONTROL_MASK : 0);
		break;
	    default:
		/* Don't do this for key release, we apparently get these with
		 * focus changes in some GTK version. */
		if (is_keypress)
		    cancelBalloon(beval);
		break;
	}
    }
    else
	cancelBalloon(beval);
}

    static gint
timeout_cb(gpointer data)
{
    BalloonEval *beval = (BalloonEval *)data;

    beval->timerID = 0;
    /*
     * If the timer event happens then the mouse has stopped long enough for
     * a request to be started. The request will only send to the debugger if
     * there the mouse is pointing at real data.
     */
    requestBalloon(beval);

    return FALSE; /* don't call me again */
}

/*ARGSUSED2*/
    static gint
balloon_expose_event_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    gtk_paint_flat_box(widget->style, widget->window,
		       GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		       &event->area, widget, "tooltip",
		       0, 0, -1, -1);

    return FALSE; /* continue emission */
}

# ifndef HAVE_GTK2
/*ARGSUSED2*/
    static void
balloon_draw_cb(GtkWidget *widget, GdkRectangle *area, gpointer data)
{
    GtkWidget	    *child;
    GdkRectangle    child_area;

    gtk_paint_flat_box(widget->style, widget->window,
		       GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		       area, widget, "tooltip",
		       0, 0, -1, -1);

    child = GTK_BIN(widget)->child;

    if (gtk_widget_intersect(child, area, &child_area))
	gtk_widget_draw(child, &child_area);
}
# endif

#else /* !FEAT_GUI_GTK */

    static void
addEventHandler(target, beval)
    Widget	target;
    BalloonEval	*beval;
{
    XtAddEventHandler(target,
			PointerMotionMask | EnterWindowMask |
			LeaveWindowMask | ButtonPressMask | KeyPressMask |
			KeyReleaseMask,
			False,
			pointerEventEH, (XtPointer)beval);
}

    static void
removeEventHandler(beval)
    BalloonEval	*beval;
{
    XtRemoveEventHandler(beval->target,
			PointerMotionMask | EnterWindowMask |
			LeaveWindowMask | ButtonPressMask | KeyPressMask |
			KeyReleaseMask,
			False,
			pointerEventEH, (XtPointer)beval);
}


/*
 * The X event handler. All it does is call the real event handler.
 */
/*ARGSUSED*/
    static void
pointerEventEH(w, client_data, event, unused)
    Widget	w;
    XtPointer	client_data;
    XEvent	*event;
    Boolean	*unused;
{
    BalloonEval *beval = (BalloonEval *)client_data;
    pointerEvent(beval, event);
}


/*
 * The real event handler. Called by pointerEventEH() whenever an event we are
 * interested in occurs.
 */

    static void
pointerEvent(beval, event)
    BalloonEval	*beval;
    XEvent	*event;
{
    Position	distance;	    /* a measure of how much the ponter moved */
    Position	delta;		    /* used to compute distance */

    switch (event->type)
    {
	case EnterNotify:
	case MotionNotify:
	    delta = event->xmotion.x - beval->x;
	    if (delta < 0)
		delta = -delta;
	    distance = delta;
	    delta = event->xmotion.y - beval->y;
	    if (delta < 0)
		delta = -delta;
	    distance += delta;
	    if (distance > 4)
	    {
		/*
		 * Moved out of the balloon location: cancel it.
		 * Remember button state
		 */
		beval->state = event->xmotion.state;
		if (beval->state & (Button1Mask|Button2Mask|Button3Mask))
		{
		    /* Mouse buttons are pressed - no balloon now */
		    cancelBalloon(beval);
		}
		else if (beval->state & (Mod1Mask|Mod2Mask|Mod3Mask))
		{
		    /*
		     * Alt is pressed -- enter super-evaluate-mode,
		     * where there is no time delay
		     */
		    beval->x = event->xmotion.x;
		    beval->y = event->xmotion.y;
		    beval->x_root = event->xmotion.x_root;
		    beval->y_root = event->xmotion.y_root;
		    cancelBalloon(beval);
		    if (beval->msgCB != NULL)
		    {
			beval->showState = ShS_PENDING;
			(*beval->msgCB)(beval, beval->state);
		    }
		}
		else
		{
		    beval->x = event->xmotion.x;
		    beval->y = event->xmotion.y;
		    beval->x_root = event->xmotion.x_root;
		    beval->y_root = event->xmotion.y_root;
		    cancelBalloon(beval);
		    beval->timerID = XtAppAddTimeOut( beval->appContext,
					(long_u)p_bdlay, timerRoutine, beval);
		}
	    }
	    break;

	/*
	 * Motif and Athena version: Keystrokes will be caught by the
	 * "textArea" widget, and handled in gui_x11_key_hit_cb().
	 */
	case KeyPress:
	    if (beval->showState == ShS_SHOWING && beval->msgCB != NULL)
	    {
		Modifiers   modifier;
		KeySym	    keysym;

		XtTranslateKeycode(gui.dpy,
				       event->xkey.keycode, event->xkey.state,
				       &modifier, &keysym);
		if (keysym == XK_Shift_L || keysym == XK_Shift_R)
		{
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, ShiftMask);
		}
		else if (keysym == XK_Control_L || keysym == XK_Control_R)
		{
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, ControlMask);
		}
		else
		    cancelBalloon(beval);
	    }
	    else
		cancelBalloon(beval);
	    break;

	case KeyRelease:
	    if (beval->showState == ShS_SHOWING && beval->msgCB != NULL)
	    {
		Modifiers modifier;
		KeySym keysym;

		XtTranslateKeycode(gui.dpy, event->xkey.keycode,
				event->xkey.state, &modifier, &keysym);
		if ((keysym == XK_Shift_L) || (keysym == XK_Shift_R)) {
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, 0);
		}
		else if ((keysym == XK_Control_L) || (keysym == XK_Control_R))
		{
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, 0);
		}
		else
		    cancelBalloon(beval);
	    }
	    else
		cancelBalloon(beval);
	    break;

	case LeaveNotify:
		/* Ignore LeaveNotify events that are not "normal".
		 * Apparently we also get it when somebody else grabs focus.
		 * Happens for me every two seconds (some clipboard tool?) */
		if (event->xcrossing.mode == NotifyNormal)
		    cancelBalloon(beval);
		break;

	case ButtonPress:
		cancelBalloon(beval);
		break;

	default:
	    break;
    }
}

/*ARGSUSED*/
    static void
timerRoutine(dx, id)
    XtPointer	    dx;
    XtIntervalId    *id;
{
    BalloonEval *beval = (BalloonEval *)dx;

    beval->timerID = (XtIntervalId)NULL;

    /*
     * If the timer event happens then the mouse has stopped long enough for
     * a request to be started. The request will only send to the debugger if
     * there the mouse is pointing at real data.
     */
    requestBalloon(beval);
}

#endif /* !FEAT_GUI_GTK */

    static void
requestBalloon(beval)
    BalloonEval	*beval;
{
    if (beval->showState != ShS_PENDING)
    {
	/* Determine the beval to display */
	if (beval->msgCB != NULL)
	{
	    beval->showState = ShS_PENDING;
	    (*beval->msgCB)(beval, beval->state);
	}
	else if (beval->msg != NULL)
	    drawBalloon(beval);
    }
}

#ifdef FEAT_GUI_GTK

# ifdef HAVE_GTK2
/*
 * Convert the string to UTF-8 if 'encoding' is not "utf-8".
 * Replace any non-printable characters and invalid bytes sequences with
 * "^X" or "<xx>" escapes, and apply SpecialKey highlighting to them.
 * TAB and NL are passed through unscathed.
 */
#  define IS_NONPRINTABLE(c) (((c) < 0x20 && (c) != TAB && (c) != NL) \
			      || (c) == DEL)
    static void
set_printable_label_text(GtkLabel *label, char_u *text)
{
    char_u	    *convbuf = NULL;
    char_u	    *buf;
    char_u	    *p;
    char_u	    *pdest;
    unsigned int    len;
    int		    charlen;
    int		    uc;
    PangoAttrList   *attr_list;

    /* Convert to UTF-8 if it isn't already */
    if (output_conv.vc_type != CONV_NONE)
    {
	convbuf = string_convert(&output_conv, text, NULL);
	if (convbuf != NULL)
	    text = convbuf;
    }

    /* First let's see how much we need to allocate */
    len = 0;
    for (p = text; *p != NUL; p += charlen)
    {
	if ((*p & 0x80) == 0)	/* be quick for ASCII */
	{
	    charlen = 1;
	    len += IS_NONPRINTABLE(*p) ? 2 : 1;	/* nonprintable: ^X */
	}
	else
	{
	    charlen = utf_ptr2len(p);
	    uc = utf_ptr2char(p);

	    if (charlen != utf_char2len(uc))
		charlen = 1; /* reject overlong sequences */

	    if (charlen == 1 || uc < 0xa0)	/* illegal byte or    */
		len += 4;			/* control char: <xx> */
	    else if (!utf_printable(uc))
		/* Note: we assume here that utf_printable() doesn't
		 * care about characters outside the BMP. */
		len += 6;			/* nonprintable: <xxxx> */
	    else
		len += charlen;
	}
    }

    attr_list = pango_attr_list_new();
    buf = alloc(len + 1);

    /* Now go for the real work */
    if (buf != NULL)
    {
	attrentry_T	*aep;
	PangoAttribute	*attr;
	guicolor_T	pixel;
	GdkColor	color = { 0, 0, 0, 0 };

	/* Look up the RGB values of the SpecialKey foreground color. */
	aep = syn_gui_attr2entry(hl_attr(HLF_8));
	pixel = (aep != NULL) ? aep->ae_u.gui.fg_color : INVALCOLOR;
	if (pixel != INVALCOLOR)
	    gdk_colormap_query_color(gtk_widget_get_colormap(gui.drawarea),
				     (unsigned long)pixel, &color);

	pdest = buf;
	p = text;
	while (*p != NUL)
	{
	    /* Be quick for ASCII */
	    if ((*p & 0x80) == 0 && !IS_NONPRINTABLE(*p))
	    {
		*pdest++ = *p++;
	    }
	    else
	    {
		charlen = utf_ptr2len(p);
		uc = utf_ptr2char(p);

		if (charlen != utf_char2len(uc))
		    charlen = 1; /* reject overlong sequences */

		if (charlen == 1 || uc < 0xa0 || !utf_printable(uc))
		{
		    int	outlen;

		    /* Careful: we can't just use transchar_byte() here,
		     * since 'encoding' is not necessarily set to "utf-8". */
		    if (*p & 0x80 && charlen == 1)
		    {
			transchar_hex(pdest, *p);	/* <xx> */
			outlen = 4;
		    }
		    else if (uc >= 0x80)
		    {
			/* Note: we assume here that utf_printable() doesn't
			 * care about characters outside the BMP. */
			transchar_hex(pdest, uc);	/* <xx> or <xxxx> */
			outlen = (uc < 0x100) ? 4 : 6;
		    }
		    else
		    {
			transchar_nonprint(pdest, *p);	/* ^X */
			outlen = 2;
		    }
		    if (pixel != INVALCOLOR)
		    {
			attr = pango_attr_foreground_new(
				color.red, color.green, color.blue);
			attr->start_index = pdest - buf;
			attr->end_index   = pdest - buf + outlen;
			pango_attr_list_insert(attr_list, attr);
		    }
		    pdest += outlen;
		    p += charlen;
		}
		else
		{
		    do
			*pdest++ = *p++;
		    while (--charlen != 0);
		}
	    }
	}
	*pdest = NUL;
    }

    vim_free(convbuf);

    gtk_label_set_text(label, (const char *)buf);
    vim_free(buf);

    gtk_label_set_attributes(label, attr_list);
    pango_attr_list_unref(attr_list);
}
#  undef IS_NONPRINTABLE
# endif /* HAVE_GTK2 */

/*
 * Draw a balloon.
 */
    static void
drawBalloon(BalloonEval *beval)
{
    if (beval->msg != NULL)
    {
	GtkRequisition	requisition;
	int		screen_w;
	int		screen_h;
	int		x;
	int		y;
	int		x_offset = EVAL_OFFSET_X;
	int		y_offset = EVAL_OFFSET_Y;
# ifdef HAVE_GTK2
	PangoLayout	*layout;
# endif
# ifdef HAVE_GTK_MULTIHEAD
	GdkScreen	*screen;

	screen = gtk_widget_get_screen(beval->target);
	gtk_window_set_screen(GTK_WINDOW(beval->balloonShell), screen);
	screen_w = gdk_screen_get_width(screen);
	screen_h = gdk_screen_get_height(screen);
# else
	screen_w = gdk_screen_width();
	screen_h = gdk_screen_height();
# endif
	gtk_widget_ensure_style(beval->balloonShell);
	gtk_widget_ensure_style(beval->balloonLabel);

# ifdef HAVE_GTK2
	set_printable_label_text(GTK_LABEL(beval->balloonLabel), beval->msg);
	/*
	 * Dirty trick:  Enable wrapping mode on the label's layout behind its
	 * back.  This way GtkLabel won't try to constrain the wrap width to a
	 * builtin maximum value of about 65 Latin characters.
	 */
	layout = gtk_label_get_layout(GTK_LABEL(beval->balloonLabel));
#  ifdef PANGO_WRAP_WORD_CHAR
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
#  else
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
#  endif
	pango_layout_set_width(layout,
		/* try to come up with some reasonable width */
		PANGO_SCALE * CLAMP(gui.num_cols * gui.char_width,
				    screen_w / 2,
				    MAX(20, screen_w - 20)));

	/* Calculate the balloon's width and height. */
	gtk_widget_size_request(beval->balloonShell, &requisition);
# else
	gtk_label_set_line_wrap(GTK_LABEL(beval->balloonLabel), FALSE);
	gtk_label_set_text(GTK_LABEL(beval->balloonLabel),
			   (const char *)beval->msg);

	/* Calculate the balloon's width and height. */
	gtk_widget_size_request(beval->balloonShell, &requisition);
	/*
	 * Unfortunately, the dirty trick used above to get around the builtin
	 * maximum wrap width of GtkLabel doesn't work with GTK+ 1.  Thus if
	 * and only if it's absolutely necessary to avoid drawing off-screen,
	 * do enable wrapping now and recalculate the size request.
	 */
	if (requisition.width > screen_w)
	{
	    gtk_label_set_line_wrap(GTK_LABEL(beval->balloonLabel), TRUE);
	    gtk_widget_size_request(beval->balloonShell, &requisition);
	}
# endif

	/* Compute position of the balloon area */
	gdk_window_get_origin(beval->target->window, &x, &y);
	x += beval->x;
	y += beval->y;

	/* Get out of the way of the mouse pointer */
	if (x + x_offset + requisition.width > screen_w)
	    y_offset += 15;
	if (y + y_offset + requisition.height > screen_h)
	    y_offset = -requisition.height - EVAL_OFFSET_Y;

	/* Sanitize values */
	x = CLAMP(x + x_offset, 0, MAX(0, screen_w - requisition.width));
	y = CLAMP(y + y_offset, 0, MAX(0, screen_h - requisition.height));

	/* Show the balloon */
	gtk_widget_set_uposition(beval->balloonShell, x, y);
	gtk_widget_show(beval->balloonShell);

	beval->showState = ShS_SHOWING;
    }
}

/*
 * Undraw a balloon.
 */
    static void
undrawBalloon(BalloonEval *beval)
{
    if (beval->balloonShell != NULL)
	gtk_widget_hide(beval->balloonShell);
    beval->showState = ShS_NEUTRAL;
}

    static void
cancelBalloon(BalloonEval *beval)
{
    if (beval->showState == ShS_SHOWING
	    || beval->showState == ShS_UPDATE_PENDING)
	undrawBalloon(beval);

    if (beval->timerID != 0)
    {
	gtk_timeout_remove(beval->timerID);
	beval->timerID = 0;
    }
    beval->showState = ShS_NEUTRAL;
}

    static void
createBalloonEvalWindow(BalloonEval *beval)
{
    beval->balloonShell = gtk_window_new(GTK_WINDOW_POPUP);

    gtk_widget_set_app_paintable(beval->balloonShell, TRUE);
    gtk_window_set_policy(GTK_WINDOW(beval->balloonShell), FALSE, FALSE, TRUE);
    gtk_widget_set_name(beval->balloonShell, "gtk-tooltips");
    gtk_container_border_width(GTK_CONTAINER(beval->balloonShell), 4);

    gtk_signal_connect((GtkObject*)(beval->balloonShell), "expose_event",
		       GTK_SIGNAL_FUNC(balloon_expose_event_cb), NULL);
# ifndef HAVE_GTK2
    gtk_signal_connect((GtkObject*)(beval->balloonShell), "draw",
		       GTK_SIGNAL_FUNC(balloon_draw_cb), NULL);
# endif
    beval->balloonLabel = gtk_label_new(NULL);

    gtk_label_set_line_wrap(GTK_LABEL(beval->balloonLabel), FALSE);
    gtk_label_set_justify(GTK_LABEL(beval->balloonLabel), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(beval->balloonLabel), 0.5f, 0.5f);
    gtk_widget_set_name(beval->balloonLabel, "vim-balloon-label");
    gtk_widget_show(beval->balloonLabel);

    gtk_container_add(GTK_CONTAINER(beval->balloonShell), beval->balloonLabel);
}

#else /* !FEAT_GUI_GTK */

/*
 * Draw a balloon.
 */
    static void
drawBalloon(beval)
    BalloonEval	*beval;
{
    Dimension	w;
    Dimension	h;
    Position tx;
    Position ty;

    if (beval->msg != NULL)
    {
	/* Show the Balloon */

	/* Calculate the label's width and height */
#ifdef FEAT_GUI_MOTIF
	XmString s;

	/* For the callback function we parse NL characters to create a
	 * multi-line label.  This doesn't work for all languages, but
	 * XmStringCreateLocalized() doesn't do multi-line labels... */
	if (beval->msgCB != NULL)
	    s = XmStringCreateLtoR((char *)beval->msg, XmFONTLIST_DEFAULT_TAG);
	else
	    s = XmStringCreateLocalized((char *)beval->msg);
	{
	    XmFontList fl;

	    fl = gui_motif_fontset2fontlist(&gui.tooltip_fontset);
	    if (fl != NULL)
	    {
		XmStringExtent(fl, s, &w, &h);
		XmFontListFree(fl);
	    }
	}
	w += gui.border_offset << 1;
	h += gui.border_offset << 1;
	XtVaSetValues(beval->balloonLabel, XmNlabelString, s, NULL);
	XmStringFree(s);
#else /* Athena */
	/* Assume XtNinternational == True */
	XFontSet	fset;
	XFontSetExtents *ext;

	XtVaGetValues(beval->balloonLabel, XtNfontSet, &fset, NULL);
	ext = XExtentsOfFontSet(fset);
	h = ext->max_ink_extent.height;
	w = XmbTextEscapement(fset,
			      (char *)beval->msg,
			      (int)STRLEN(beval->msg));
	w += gui.border_offset << 1;
	h += gui.border_offset << 1;
	XtVaSetValues(beval->balloonLabel, XtNlabel, beval->msg, NULL);
#endif

	/* Compute position of the balloon area */
	tx = beval->x_root + EVAL_OFFSET_X;
	ty = beval->y_root + EVAL_OFFSET_Y;
	if ((tx + w) > beval->screen_width)
	    tx = beval->screen_width - w;
	if ((ty + h) > beval->screen_height)
	    ty = beval->screen_height - h;
#ifdef FEAT_GUI_MOTIF
	XtVaSetValues(beval->balloonShell,
		XmNx, tx,
		XmNy, ty,
		NULL);
#else
	/* Athena */
	XtVaSetValues(beval->balloonShell,
		XtNx, tx,
		XtNy, ty,
		NULL);
#endif

	XtPopup(beval->balloonShell, XtGrabNone);

	beval->showState = ShS_SHOWING;

	current_beval = beval;
    }
}

/*
 * Undraw a balloon.
 */
    static void
undrawBalloon(beval)
    BalloonEval *beval;
{
    if (beval->balloonShell != (Widget)0)
	XtPopdown(beval->balloonShell);
    beval->showState = ShS_NEUTRAL;

    current_beval = NULL;
}

    static void
cancelBalloon(beval)
    BalloonEval	*beval;
{
    if (beval->showState == ShS_SHOWING
	    || beval->showState == ShS_UPDATE_PENDING)
	undrawBalloon(beval);

    if (beval->timerID != (XtIntervalId)NULL)
    {
	XtRemoveTimeOut(beval->timerID);
	beval->timerID = (XtIntervalId)NULL;
    }
    beval->showState = ShS_NEUTRAL;
}


    static void
createBalloonEvalWindow(beval)
    BalloonEval	*beval;
{
    Arg		args[12];
    int		n;

    n = 0;
#ifdef FEAT_GUI_MOTIF
    XtSetArg(args[n], XmNallowShellResize, True); n++;
    beval->balloonShell = XtAppCreateShell("balloonEval", "BalloonEval",
		    overrideShellWidgetClass, gui.dpy, args, n);
#else
    /* Athena */
    XtSetArg(args[n], XtNallowShellResize, True); n++;
    beval->balloonShell = XtAppCreateShell("balloonEval", "BalloonEval",
		    overrideShellWidgetClass, gui.dpy, args, n);
#endif

    n = 0;
#ifdef FEAT_GUI_MOTIF
    {
	XmFontList fl;

	fl = gui_motif_fontset2fontlist(&gui.tooltip_fontset);
	XtSetArg(args[n], XmNforeground, gui.tooltip_fg_pixel); n++;
	XtSetArg(args[n], XmNbackground, gui.tooltip_bg_pixel); n++;
	XtSetArg(args[n], XmNfontList, fl); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	beval->balloonLabel = XtCreateManagedWidget("balloonLabel",
			xmLabelWidgetClass, beval->balloonShell, args, n);
    }
#else /* FEAT_GUI_ATHENA */
    XtSetArg(args[n], XtNforeground, gui.tooltip_fg_pixel); n++;
    XtSetArg(args[n], XtNbackground, gui.tooltip_bg_pixel); n++;
    XtSetArg(args[n], XtNinternational, True); n++;
    XtSetArg(args[n], XtNfontSet, gui.tooltip_fontset); n++;
    beval->balloonLabel = XtCreateManagedWidget("balloonLabel",
		    labelWidgetClass, beval->balloonShell, args, n);
#endif
}

#endif /* !FEAT_GUI_GTK */
#endif /* !FEAT_GUI_W32 */

#endif /* FEAT_BEVAL */
