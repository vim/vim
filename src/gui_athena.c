/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *				Athena port by Bill Foster
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#ifdef FEAT_GUI_NEXTAW
# include <X11/neXtaw/Form.h>
# include <X11/neXtaw/SimpleMenu.h>
# include <X11/neXtaw/MenuButton.h>
# include <X11/neXtaw/SmeBSB.h>
# include <X11/neXtaw/SmeLine.h>
# include <X11/neXtaw/Box.h>
# include <X11/neXtaw/Dialog.h>
# include <X11/neXtaw/Text.h>
# include <X11/neXtaw/AsciiText.h>
# include <X11/neXtaw/Scrollbar.h>
#else
# include <X11/Xaw/Form.h>
# include <X11/Xaw/SimpleMenu.h>
# include <X11/Xaw/MenuButton.h>
# include <X11/Xaw/SmeBSB.h>
# include <X11/Xaw/SmeLine.h>
# include <X11/Xaw/Box.h>
# include <X11/Xaw/Dialog.h>
# include <X11/Xaw/Text.h>
# include <X11/Xaw/AsciiText.h>
#endif /* FEAT_GUI_NEXTAW */

#include "vim.h"
#ifndef FEAT_GUI_NEXTAW
# include "gui_at_sb.h"
#endif

extern Widget vimShell;

static Widget vimForm = (Widget)0;
Widget textArea = (Widget)0;
#ifdef FEAT_MENU
static Widget menuBar = (Widget)0;
static XtIntervalId timer = 0;	    /* 0 = expired, otherwise active */

/* Used to figure out menu ordering */
static vimmenu_T *a_cur_menu = NULL;
static Cardinal	athena_calculate_ins_pos __ARGS((Widget));

static Pixmap gui_athena_create_pullright_pixmap __ARGS((Widget));
static void gui_athena_menu_timeout __ARGS((XtPointer, XtIntervalId *));
static void gui_athena_popup_callback __ARGS((Widget, XtPointer, XtPointer));
static void gui_athena_delayed_arm_action __ARGS((Widget, XEvent *, String *,
						 Cardinal *));
static void gui_athena_popdown_submenus_action __ARGS((Widget, XEvent *,
						      String *, Cardinal *));
static XtActionsRec	pullAction[2] = {
    { "menu-delayedpopup", (XtActionProc)gui_athena_delayed_arm_action},
    { "menu-popdownsubmenus", (XtActionProc)gui_athena_popdown_submenus_action}
};
#endif

#ifdef FEAT_TOOLBAR
static void gui_mch_reset_focus __ARGS((void));
static Widget toolBar = (Widget)0;
#endif

static void gui_athena_scroll_cb_jump	__ARGS((Widget, XtPointer, XtPointer));
static void gui_athena_scroll_cb_scroll __ARGS((Widget, XtPointer, XtPointer));
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_MENU)
static void gui_athena_menu_colors __ARGS((Widget id));
#endif
static void gui_athena_scroll_colors __ARGS((Widget id));

#ifdef FEAT_MENU
static XtTranslations	popupTrans, parentTrans, menuTrans, supermenuTrans;
static Pixmap		pullerBitmap = None;
static int		puller_width = 0;
#endif

/*
 * Scrollbar callback (XtNjumpProc) for when the scrollbar is dragged with the
 * left or middle mouse button.
 */
/* ARGSUSED */
    static void
gui_athena_scroll_cb_jump(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data, call_data;
{
    scrollbar_T *sb, *sb_info;
    long	value;

    sb = gui_find_scrollbar((long)client_data);

    if (sb == NULL)
	return;
    else if (sb->wp != NULL)	    /* Left or right scrollbar */
    {
	/*
	 * Careful: need to get scrollbar info out of first (left) scrollbar
	 * for window, but keep real scrollbar too because we must pass it to
	 * gui_drag_scrollbar().
	 */
	sb_info = &sb->wp->w_scrollbars[0];
    }
    else	    /* Bottom scrollbar */
	sb_info = sb;

    value = (long)(*((float *)call_data) * (float)(sb_info->max + 1) + 0.001);
    if (value > sb_info->max)
	value = sb_info->max;

    gui_drag_scrollbar(sb, value, TRUE);
}

/*
 * Scrollbar callback (XtNscrollProc) for paging up or down with the left or
 * right mouse buttons.
 */
/* ARGSUSED */
    static void
gui_athena_scroll_cb_scroll(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data, call_data;
{
    scrollbar_T *sb, *sb_info;
    long	value;
    int		data = (int)(long)call_data;
    int		page;

    sb = gui_find_scrollbar((long)client_data);

    if (sb == NULL)
	return;
    if (sb->wp != NULL)		/* Left or right scrollbar */
    {
	/*
	 * Careful: need to get scrollbar info out of first (left) scrollbar
	 * for window, but keep real scrollbar too because we must pass it to
	 * gui_drag_scrollbar().
	 */
	sb_info = &sb->wp->w_scrollbars[0];

	if (sb_info->size > 5)
	    page = sb_info->size - 2;	    /* use two lines of context */
	else
	    page = sb_info->size;
#ifdef FEAT_GUI_NEXTAW
	if (data < 0)
	{
	    data = (data - gui.char_height + 1) / gui.char_height;
	    if (data > -sb_info->size)
		data = -1;
	    else
		data = -page;
	}
	else if (data > 0)
	{
	    data = (data + gui.char_height - 1) / gui.char_height;
	    if (data < sb_info->size)
		data = 1;
	    else
		data = page;
	}
#else
	switch (data)
	{
	    case  ONE_LINE_DATA: data = 1; break;
	    case -ONE_LINE_DATA: data = -1; break;
	    case  ONE_PAGE_DATA: data = page; break;
	    case -ONE_PAGE_DATA: data = -page; break;
	    case  END_PAGE_DATA: data = sb_info->max; break;
	    case -END_PAGE_DATA: data = -sb_info->max; break;
			default: data = 0; break;
	}
#endif
    }
    else			/* Bottom scrollbar */
    {
	sb_info = sb;
#ifdef FEAT_GUI_NEXTAW
	if (data < 0)
	{
	    data = (data - gui.char_width + 1) / gui.char_width;
	    if (data > -sb->size)
		data = -1;
	}
	else if (data > 0)
	{
	    data = (data + gui.char_width - 1) / gui.char_width;
	    if (data < sb->size)
		data = 1;
	}
#endif
	if (data < -1)		/* page-width left */
	{
	    if (sb->size > 8)
		data = -(sb->size - 5);
	    else
		data = -sb->size;
	}
	else if (data > 1)	/* page-width right */
	{
	    if (sb->size > 8)
		data = (sb->size - 5);
	    else
		data = sb->size;
	}
    }

    value = sb_info->value + data;
    if (value > sb_info->max)
	value = sb_info->max;
    else if (value < 0)
	value = 0;

    /* Update the bottom scrollbar an extra time (why is this needed?? */
    if (sb->wp == NULL)		/* Bottom scrollbar */
	gui_mch_set_scrollbar_thumb(sb, value, sb->size, sb->max);

    gui_drag_scrollbar(sb, value, FALSE);
}

/*
 * Create all the Athena widgets necessary.
 */
    void
gui_x11_create_widgets()
{
    /*
     * We don't have any borders handled internally by the textArea to worry
     * about so only skip over the configured border width.
     */
    gui.border_offset = gui.border_width;

#if 0 /* not needed? */
    XtInitializeWidgetClass(formWidgetClass);
    XtInitializeWidgetClass(boxWidgetClass);
    XtInitializeWidgetClass(coreWidgetClass);
#ifdef FEAT_MENU
    XtInitializeWidgetClass(menuButtonWidgetClass);
#endif
    XtInitializeWidgetClass(simpleMenuWidgetClass);
#ifdef FEAT_GUI_NEXTAW
    XtInitializeWidgetClass(scrollbarWidgetClass);
#else
    XtInitializeWidgetClass(vim_scrollbarWidgetClass);
#endif
#endif

    /* The form containing all the other widgets */
    vimForm = XtVaCreateManagedWidget("vimForm",
	formWidgetClass,	vimShell,
	XtNborderWidth,		0,
	NULL);
    gui_athena_scroll_colors(vimForm);

#ifdef FEAT_MENU
    /* The top menu bar */
    menuBar = XtVaCreateManagedWidget("menuBar",
	boxWidgetClass,		vimForm,
	XtNresizable,		True,
	XtNtop,			XtChainTop,
	XtNbottom,		XtChainTop,
	XtNleft,		XtChainLeft,
	XtNright,		XtChainRight,
	XtNinsertPosition,	athena_calculate_ins_pos,
	NULL);
    gui_athena_menu_colors(menuBar);
    if (gui.menu_fg_pixel != INVALCOLOR)
	XtVaSetValues(menuBar, XtNborderColor, gui.menu_fg_pixel, NULL);
#endif

#ifdef FEAT_TOOLBAR
    /* Don't create it Managed, it will be managed when creating the first
     * item.  Otherwise an empty toolbar shows up. */
    toolBar = XtVaCreateWidget("toolBar",
	boxWidgetClass,		vimForm,
	XtNresizable,		True,
	XtNtop,			XtChainTop,
	XtNbottom,		XtChainTop,
	XtNleft,		XtChainLeft,
	XtNright,		XtChainRight,
	XtNorientation,		XtorientHorizontal,
	XtNhSpace,		1,
	XtNvSpace,		3,
	XtNinsertPosition,	athena_calculate_ins_pos,
	NULL);
    gui_athena_menu_colors(toolBar);
#endif

    /* The text area. */
    textArea = XtVaCreateManagedWidget("textArea",
	coreWidgetClass,	vimForm,
	XtNresizable,		True,
	XtNtop,			XtChainTop,
	XtNbottom,		XtChainTop,
	XtNleft,		XtChainLeft,
	XtNright,		XtChainLeft,
	XtNbackground,		gui.back_pixel,
	XtNborderWidth,		0,
	NULL);

    /*
     * Install the callbacks.
     */
    gui_x11_callbacks(textArea, vimForm);

#ifdef FEAT_MENU
    popupTrans = XtParseTranslationTable(
	    "<EnterWindow>: menu-popdownsubmenus() highlight() menu-delayedpopup()\n"
	    "<LeaveWindow>: unhighlight()\n"
	    "<BtnUp>: menu-popdownsubmenus() XtMenuPopdown() notify() unhighlight()\n"
	    "<Motion>: highlight() menu-delayedpopup()");
    parentTrans = XtParseTranslationTable("<LeaveWindow>: unhighlight()");
    menuTrans = XtParseTranslationTable(
	    "<EnterWindow>: menu-popdownsubmenus() highlight() menu-delayedpopup()\n"
	    "<LeaveWindow>: menu-popdownsubmenus() XtMenuPopdown() unhighlight()\n"
	    "<BtnUp>: notify() unhighlight()\n"
	    "<BtnMotion>: highlight() menu-delayedpopup()");
    supermenuTrans = XtParseTranslationTable(
	    "<EnterWindow>: menu-popdownsubmenus() highlight() menu-delayedpopup()\n"
	    "<LeaveWindow>: unhighlight()\n"
	    "<BtnUp>: menu-popdownsubmenus() XtMenuPopdown() notify() unhighlight()\n"
	    "<BtnMotion>: highlight() menu-delayedpopup()");

    XtAppAddActions(XtWidgetToApplicationContext(vimForm), pullAction,
		    XtNumber(pullAction));
#endif

    /* Pretend we don't have input focus, we will get an event if we do. */
    gui.in_focus = FALSE;
}

#ifdef FEAT_MENU
/*
 * Calculates the Pixmap based on the size of the current menu font.
 */
    static Pixmap
gui_athena_create_pullright_pixmap(w)
    Widget  w;
{
    Pixmap  retval;
#ifdef FONTSET_ALWAYS
    XFontSet	font = None;
#else
    XFontStruct	*font = NULL;
#endif

#ifdef FONTSET_ALWAYS
    if (gui.menu_fontset == NOFONTSET)
#else
    if (gui.menu_font == NOFONT)
#endif
    {
	XrmValue from, to;
	WidgetList  children;
	Cardinal    num_children;

#ifdef FONTSET_ALWAYS
	from.size = strlen(from.addr = XtDefaultFontSet);
	to.addr = (XtPointer)&font;
	to.size = sizeof(XFontSet);
#else
	from.size = strlen(from.addr = XtDefaultFont);
	to.addr = (XtPointer)&font;
	to.size = sizeof(XFontStruct *);
#endif
	/* Assumption: The menuBar children will use the same font as the
	 *	       pulldown menu items AND they will all be of type
	 *	       XtNfont.
	 */
	XtVaGetValues(menuBar, XtNchildren, &children,
			       XtNnumChildren, &num_children,
			       NULL);
	if (XtConvertAndStore(w ? w :
				(num_children > 0) ? children[0] : menuBar,
			      XtRString, &from,
#ifdef FONTSET_ALWAYS
			      XtRFontSet, &to
#else
			      XtRFontStruct, &to
#endif
		    ) == False)
	    return None;
	/* "font" should now contain data */
    }
    else
#ifdef FONTSET_ALWAYS
	font = (XFontSet)gui.menu_fontset;
#else
	font = (XFontStruct *)gui.menu_font;
#endif

    {
	int	    width, height;
	GC	    draw_gc, undraw_gc;
	XGCValues   gc_values;
	XPoint	    points[3];

#ifdef FONTSET_ALWAYS
	height = fontset_height2(font);
#else
	height = font->max_bounds.ascent + font->max_bounds.descent;
#endif
	width = height - 2;
	puller_width = width + 4;
	retval = XCreatePixmap(gui.dpy,DefaultRootWindow(gui.dpy),width,
			       height, 1);
	gc_values.foreground = 1;
	gc_values.background = 0;
	draw_gc = XCreateGC(gui.dpy, retval,
		    GCForeground | GCBackground,
		    &gc_values);
	gc_values.foreground = 0;
	gc_values.background = 1;
	undraw_gc = XCreateGC(gui.dpy, retval,
			      GCForeground | GCBackground,
			      &gc_values);
	points[0].x = 0;
	points[0].y = 0;
	points[1].x = width - 1;
	points[1].y = (height - 1) / 2;
	points[2].x = 0;
	points[2].y = height - 1;
	XFillRectangle(gui.dpy, retval, undraw_gc, 0, 0, height, height);
	XFillPolygon(gui.dpy, retval, draw_gc, points, XtNumber(points),
		     Convex, CoordModeOrigin);
	XFreeGC(gui.dpy, draw_gc);
	XFreeGC(gui.dpy, undraw_gc);
    }
    return retval;
}
#endif

/*
 * Called when the GUI is not going to start after all.
 */
    void
gui_x11_destroy_widgets()
{
    textArea = NULL;
#ifdef FEAT_MENU
    menuBar = NULL;
#endif
#ifdef FEAT_TOOLBAR
    toolBar = NULL;
#endif
}

#if defined(FEAT_TOOLBAR) || defined(PROTO)
# include "gui_x11_pm.h"
# ifdef HAVE_X11_XPM_H
#  include <X11/xpm.h>
# endif

static void createXpmImages __ARGS((char_u *path, char **xpm, Pixmap *sen));
static void get_toolbar_pixmap __ARGS((vimmenu_T *menu, Pixmap *sen));

/*
 * Allocated a pixmap for toolbar menu "menu".
 * Return in "sen".
 */
    static void
get_toolbar_pixmap(menu, sen)
    vimmenu_T	*menu;
    Pixmap	*sen;
{
    char_u	buf[MAXPATHL];		/* buffer storing expanded pathname */
    char	**xpm = NULL;		/* xpm array */

    buf[0] = NUL;			/* start with NULL path */

    if (menu->iconfile != NULL)
    {
	/* Use the "icon="  argument. */
	gui_find_iconfile(menu->iconfile, buf, "xpm");
	createXpmImages(buf, NULL, sen);

	/* If it failed, try using the menu name. */
	if (*sen == (Pixmap)0 && gui_find_bitmap(menu->name, buf, "xpm") == OK)
	    createXpmImages(buf, NULL, sen);
	if (*sen != (Pixmap)0)
	    return;
    }

    if (menu->icon_builtin || gui_find_bitmap(menu->name, buf, "xpm") == FAIL)
    {
	if (menu->iconidx >= 0 && menu->iconidx
		   < (sizeof(built_in_pixmaps) / sizeof(built_in_pixmaps[0])))
	    xpm = built_in_pixmaps[menu->iconidx];
	else
	    xpm = tb_blank_xpm;
    }

    if (xpm != NULL || buf[0] != NUL)
	createXpmImages(buf, xpm, sen);
}

/*
 * Read an Xpm file, doing color substitutions for the foreground and
 * background colors. If there is an error reading a color xpm file,
 * drop back and read the monochrome file. If successful, create the
 * insensitive Pixmap too.
 */
    static void
createXpmImages(path, xpm, sen)
    char_u	*path;
    char	**xpm;
    Pixmap	*sen;
{
    Window	rootWindow;
    XpmAttributes attrs;
    XpmColorSymbol color[5] =
    {
	{"none", "none", 0},
	{"iconColor1", NULL, 0},
	{"bottomShadowColor", NULL, 0},
	{"topShadowColor", NULL, 0},
	{"selectColor", NULL, 0}
    };
    int		screenNum;
    int		status;
    Pixmap	mask;
    Pixmap	map;

    gui_mch_get_toolbar_colors(
	    &color[BACKGROUND].pixel,
	    &color[FOREGROUND].pixel,
	    &color[BOTTOM_SHADOW].pixel,
	    &color[TOP_SHADOW].pixel,
	    &color[HIGHLIGHT].pixel);

    /* Setup the color subsititution table */
    attrs.valuemask = XpmColorSymbols;
    attrs.colorsymbols = color;
    attrs.numsymbols = 5;

    screenNum = DefaultScreen(gui.dpy);
    rootWindow = RootWindow(gui.dpy, screenNum);

    /* Create the "sensitive" pixmap */
    if (xpm != NULL)
	status = XpmCreatePixmapFromData(gui.dpy, rootWindow, xpm,
							 &map, &mask, &attrs);
    else
	status = XpmReadFileToPixmap(gui.dpy, rootWindow, (char *)path,
							 &map, &mask, &attrs);
    if (status == XpmSuccess && map != 0)
    {
	XGCValues   gcvalues;
	GC	    back_gc;
	GC	    mask_gc;

	/* Need to create new Pixmaps with the mask applied. */
	gcvalues.foreground = color[BACKGROUND].pixel;
	back_gc = XCreateGC(gui.dpy, map, GCForeground, &gcvalues);
	mask_gc = XCreateGC(gui.dpy, map, GCForeground, &gcvalues);
	XSetClipMask(gui.dpy, mask_gc, mask);

	/* Create the "sensitive" pixmap. */
	*sen = XCreatePixmap(gui.dpy, rootWindow,
		 attrs.width, attrs.height,
		 DefaultDepth(gui.dpy, screenNum));
	XFillRectangle(gui.dpy, *sen, back_gc, 0, 0,
		attrs.width, attrs.height);
	XCopyArea(gui.dpy, map, *sen, mask_gc, 0, 0,
		attrs.width, attrs.height, 0, 0);

	XFreeGC(gui.dpy, back_gc);
	XFreeGC(gui.dpy, mask_gc);
	XFreePixmap(gui.dpy, map);
    }
    else
	*sen = 0;

    XpmFreeAttributes(&attrs);
}

    void
gui_mch_set_toolbar_pos(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    Dimension	border;
    int		height;

    if (!XtIsManaged(toolBar))	/* nothing to do */
	return;
    XtUnmanageChild(toolBar);
    XtVaGetValues(toolBar,
		XtNborderWidth, &border,
		NULL);
    height = h - 2 * border;
    if (height < 0)
	height = 1;
    XtVaSetValues(toolBar,
		  XtNhorizDistance, x,
		  XtNvertDistance, y,
		  XtNwidth, w - 2 * border,
		  XtNheight,	height,
		  NULL);
    XtManageChild(toolBar);
}
#endif

    void
gui_mch_set_text_area_pos(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    XtUnmanageChild(textArea);
    XtVaSetValues(textArea,
		  XtNhorizDistance, x,
		  XtNvertDistance, y,
		  XtNwidth, w,
		  XtNheight, h,
		  NULL);
    XtManageChild(textArea);
#ifdef FEAT_TOOLBAR
    /* Give keyboard focus to the textArea instead of the toolbar. */
    gui_mch_reset_focus();
#endif
}

#ifdef FEAT_TOOLBAR
/*
 * A toolbar button has been pushed; now reset the input focus
 * such that the user can type page up/down etc. and have the
 * input go to the editor window, not the button
 */
    static void
gui_mch_reset_focus()
{
    XtSetKeyboardFocus(vimForm, textArea);
}
#endif


    void
gui_x11_set_back_color()
{
    if (textArea != NULL)
	XtVaSetValues(textArea,
		  XtNbackground, gui.back_pixel,
		  NULL);
}

#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Menu stuff.
 */

static char_u	*make_pull_name __ARGS((char_u * name));
static Widget	get_popup_entry __ARGS((Widget w));
static Widget	submenu_widget __ARGS((Widget));
static Boolean	has_submenu __ARGS((Widget));
static void gui_mch_submenu_change __ARGS((vimmenu_T *mp, int colors));
static void gui_athena_menu_font __ARGS((Widget id));
static Boolean	gui_athena_menu_has_submenus __ARGS((Widget, Widget));

    void
gui_mch_enable_menu(flag)
    int	    flag;
{
    if (flag)
    {
	XtManageChild(menuBar);
# ifdef FEAT_TOOLBAR
	if (XtIsManaged(toolBar))
	{
	    XtVaSetValues(toolBar,
		XtNvertDistance,    gui.menu_height,
		NULL);
	    XtVaSetValues(textArea,
		XtNvertDistance,    gui.menu_height + gui.toolbar_height,
		NULL);
	}
# endif
    }
    else
    {
	XtUnmanageChild(menuBar);
# ifdef FEAT_TOOLBAR
	if (XtIsManaged(toolBar))
	{
	    XtVaSetValues(toolBar,
		XtNvertDistance,    0,
		NULL);
	}
# endif
    }
}

    void
gui_mch_set_menu_pos(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    Dimension	border;
    int		height;

    XtUnmanageChild(menuBar);
    XtVaGetValues(menuBar, XtNborderWidth, &border, NULL);
    /* avoid trouble when there are no menu items, and h is 1 */
    height = h - 2 * border;
    if (height < 0)
	height = 1;
    XtVaSetValues(menuBar,
		XtNhorizDistance, x,
		XtNvertDistance, y,
		XtNwidth, w - 2 * border,
		XtNheight, height,
		NULL);
    XtManageChild(menuBar);
}

/*
 * Used to calculate the insertion position of a widget with respect to its
 * neighbors.
 *
 * Valid range of return values is: 0 (beginning of children) to
 *				    numChildren (end of children).
 */
    static Cardinal
athena_calculate_ins_pos(widget)
    Widget	widget;
{
    /* Assume that if the parent of the vimmenu_T is NULL, then we can get
     * to this menu by traversing "next", starting at "root_menu".
     *
     * This holds true for popup menus, toolbar, and toplevel menu items.
     */

    /* Popup menus:  "id" is NULL. Only submenu_id is valid */

    /* Menus that are not toplevel: "parent" will be non-NULL, "id" &
     * "submenu_id" will be non-NULL.
     */

    /* Toplevel menus: "parent" is NULL, id is the widget of the menu item */

    WidgetList	children;
    Cardinal	num_children = 0;
    int		retval;
    Arg		args[2];
    int		n = 0;
    int		i;

    XtSetArg(args[n], XtNchildren, &children); n++;
    XtSetArg(args[n], XtNnumChildren, &num_children); n++;
    XtGetValues(XtParent(widget), args, n);

    retval = num_children;
    for (i = 0; i < num_children; ++i)
    {
	Widget	current = children[i];
	vimmenu_T	*menu = NULL;

	for (menu = (a_cur_menu->parent == NULL)
			       ? root_menu : a_cur_menu->parent->children;
			       menu != NULL;
			       menu = menu->next)
	    if (current == menu->id
		    && a_cur_menu->priority < menu->priority
		    && i < retval)
		retval = i;
    }
    return retval;
}

/* ARGSUSED */
    void
gui_mch_add_menu(menu, idx)
    vimmenu_T	*menu;
    int		idx;
{
    char_u	*pullright_name;
    Dimension	height, space, border;
    vimmenu_T	*parent = menu->parent;

    a_cur_menu = menu;
    if (parent == NULL)
    {
	if (menu_is_popup(menu->dname))
	{
	    menu->submenu_id = XtVaCreatePopupShell((char *)menu->dname,
		simpleMenuWidgetClass,	vimShell,
		XtNinsertPosition,	athena_calculate_ins_pos,
		XtNtranslations,	popupTrans,
		NULL);
	    gui_athena_menu_colors(menu->submenu_id);
	}
	else if (menu_is_menubar(menu->dname))
	{
	    menu->id = XtVaCreateManagedWidget((char *)menu->dname,
		menuButtonWidgetClass, menuBar,
		XtNmenuName,	    menu->dname,
#ifdef FONTSET_ALWAYS
		XtNinternational,   True,
#endif
		NULL);
	    if (menu->id == (Widget)0)
		return;
	    gui_athena_menu_colors(menu->id);
	    gui_athena_menu_font(menu->id);

	    menu->submenu_id = XtVaCreatePopupShell((char *)menu->dname,
		simpleMenuWidgetClass, menu->id,
		XtNinsertPosition,	athena_calculate_ins_pos,
		XtNtranslations,	supermenuTrans,
		NULL);
	    gui_athena_menu_colors(menu->submenu_id);
	    gui_athena_menu_font(menu->submenu_id);

	    /* Don't update the menu height when it was set at a fixed value */
	    if (!gui.menu_height_fixed)
	    {
		/*
		 * When we add a top-level item to the menu bar, we can figure
		 * out how high the menu bar should be.
		 */
		XtVaGetValues(menuBar,
			XtNvSpace,	&space,
			XtNborderWidth, &border,
			NULL);
		XtVaGetValues(menu->id,
			XtNheight,	&height,
			NULL);
		gui.menu_height = height + 2 * (space + border);
	    }
	}
    }
    else if (parent->submenu_id != (Widget)0)
    {
	menu->id = XtVaCreateManagedWidget((char *)menu->dname,
	    smeBSBObjectClass, parent->submenu_id,
	    XtNlabel, menu->dname,
#ifdef FONTSET_ALWAYS
	    XtNinternational,	True,
#endif
	    NULL);
	if (menu->id == (Widget)0)
	    return;
	if (pullerBitmap == None)
	    pullerBitmap = gui_athena_create_pullright_pixmap(menu->id);

	XtVaSetValues(menu->id, XtNrightBitmap, pullerBitmap,
				NULL);
	/* If there are other menu items that are not pulldown menus,
	 * we need to adjust the right margins of those, too.
	 */
	{
	    WidgetList	children;
	    Cardinal	num_children;
	    int		i;

	    XtVaGetValues(parent->submenu_id, XtNchildren, &children,
					      XtNnumChildren, &num_children,
					      NULL);
	    for (i = 0; i < num_children; ++i)
	    {
		XtVaSetValues(children[i],
			      XtNrightMargin, puller_width,
			      NULL);
	    }
	}
	gui_athena_menu_colors(menu->id);
	gui_athena_menu_font(menu->id);

	pullright_name = make_pull_name(menu->dname);
	menu->submenu_id = XtVaCreatePopupShell((char *)pullright_name,
	    simpleMenuWidgetClass, parent->submenu_id,
	    XtNtranslations, menuTrans,
	    NULL);
	gui_athena_menu_colors(menu->submenu_id);
	gui_athena_menu_font(menu->submenu_id);
	vim_free(pullright_name);
	XtAddCallback(menu->submenu_id, XtNpopupCallback,
		      gui_athena_popup_callback, (XtPointer)menu);

	if (parent->parent != NULL)
	    XtOverrideTranslations(parent->submenu_id, parentTrans);
    }
    a_cur_menu = NULL;
}

/* Used to determine whether a SimpleMenu has pulldown entries.
 *
 * "id" is the parent of the menu items.
 * Ignore widget "ignore" in the pane.
 */
    static Boolean
gui_athena_menu_has_submenus(id, ignore)
    Widget	id;
    Widget	ignore;
{
    WidgetList	children;
    Cardinal	num_children;
    int		i;

    XtVaGetValues(id, XtNchildren, &children,
		      XtNnumChildren, &num_children,
		      NULL);
    for (i = 0; i < num_children; ++i)
    {
	if (children[i] == ignore)
	    continue;
	if (has_submenu(children[i]))
	    return True;
    }
    return False;
}

    static void
gui_athena_menu_font(id)
    Widget	id;
{
#ifdef FONTSET_ALWAYS
    if (gui.menu_fontset != NOFONTSET)
    {
	if (XtIsManaged(id))
	{
	    XtUnmanageChild(id);
	    XtVaSetValues(id, XtNfontSet, gui.menu_fontset, NULL);
	    /* We should force the widget to recalculate it's
	     * geometry now. */
	    XtManageChild(id);
	}
	else
	    XtVaSetValues(id, XtNfontSet, gui.menu_fontset, NULL);
	if (has_submenu(id))
	    XtVaSetValues(id, XtNrightBitmap, pullerBitmap, NULL);
    }
#else
    int		managed = FALSE;

    if (gui.menu_font != NOFONT)
    {
	if (XtIsManaged(id))
	{
	    XtUnmanageChild(id);
	    managed = TRUE;
	}

# ifdef FEAT_XFONTSET
	if (gui.fontset != NOFONTSET)
	    XtVaSetValues(id, XtNfontSet, gui.menu_font, NULL);
	else
# endif
	    XtVaSetValues(id, XtNfont, gui.menu_font, NULL);
	if (has_submenu(id))
	    XtVaSetValues(id, XtNrightBitmap, pullerBitmap, NULL);

	/* Force the widget to recalculate it's geometry now. */
	if (managed)
	    XtManageChild(id);
    }
#endif
}


    void
gui_mch_new_menu_font()
{
    Pixmap oldpuller = None;

    if (menuBar == (Widget)0)
	return;

    if (pullerBitmap != None)
    {
	oldpuller = pullerBitmap;
	pullerBitmap = gui_athena_create_pullright_pixmap(NULL);
    }
    gui_mch_submenu_change(root_menu, FALSE);

    {
	/* Iterate through the menubar menu items and get the height of
	 * each one.  The menu bar height is set to the maximum of all
	 * the heights.
	 */
	vimmenu_T *mp;
	int max_height = 9999;

	for (mp = root_menu; mp != NULL; mp = mp->next)
	{
	    if (menu_is_menubar(mp->dname))
	    {
		Dimension height;

		XtVaGetValues(mp->id,
			XtNheight, &height,
			NULL);
		if (height < max_height)
		    max_height = height;
	    }
	}
	if (max_height != 9999)
	{
	    /* Don't update the menu height when it was set at a fixed value */
	    if (!gui.menu_height_fixed)
	    {
		Dimension   space, border;

		XtVaGetValues(menuBar,
			XtNvSpace,	&space,
			XtNborderWidth, &border,
			NULL);
		gui.menu_height = max_height + 2 * (space + border);
	    }
	}
    }
    /* Now, to simulate the window being resized.  Only, this
     * will resize the window to it's current state.
     *
     * There has to be a better way, but I do not see one at this time.
     * (David Harrison)
     */
    {
	Position w, h;

	XtVaGetValues(vimShell,
		XtNwidth, &w,
		XtNheight, &h,
		NULL);
	gui_resize_shell(w, h
#ifdef FEAT_XIM
						- xim_get_status_area_height()
#endif
		     );
    }
    gui_set_shellsize(FALSE, TRUE, RESIZE_VERT);
    ui_new_shellsize();
    if (oldpuller != None)
	XFreePixmap(gui.dpy, oldpuller);
}

#if defined(FEAT_BEVAL) || defined(PROTO)
    void
gui_mch_new_tooltip_font()
{
#  ifdef FEAT_TOOLBAR
    vimmenu_T   *menu;

    if (toolBar == (Widget)0)
	return;

    menu = gui_find_menu((char_u *)"ToolBar");
    if (menu != NULL)
	gui_mch_submenu_change(menu, FALSE);
#  endif
}

    void
gui_mch_new_tooltip_colors()
{
# ifdef FEAT_TOOLBAR
    vimmenu_T   *menu;

    if (toolBar == (Widget)0)
	return;

    menu = gui_find_menu((char_u *)"ToolBar");
    if (menu != NULL)
	gui_mch_submenu_change(menu, TRUE);
# endif
}
#endif

    static void
gui_mch_submenu_change(menu, colors)
    vimmenu_T	*menu;
    int		colors;		/* TRUE for colors, FALSE for font */
{
    vimmenu_T	*mp;

    for (mp = menu; mp != NULL; mp = mp->next)
    {
	if (mp->id != (Widget)0)
	{
	    if (colors)
	    {
		gui_athena_menu_colors(mp->id);
#ifdef FEAT_TOOLBAR
		/* For a toolbar item: Free the pixmap and allocate a new one,
		 * so that the background color is right. */
		if (mp->image != (Pixmap)0)
		{
		    XFreePixmap(gui.dpy, mp->image);
		    get_toolbar_pixmap(mp, &mp->image);
		    if (mp->image != (Pixmap)0)
			XtVaSetValues(mp->id, XtNbitmap, mp->image, NULL);
		}

# ifdef FEAT_BEVAL
		/* If we have a tooltip, then we need to change it's colors */
		if (mp->tip != NULL)
		{
		    Arg args[2];

		    args[0].name = XtNbackground;
		    args[0].value = gui.tooltip_bg_pixel;
		    args[1].name = XtNforeground;
		    args[1].value = gui.tooltip_fg_pixel;
		    XtSetValues(mp->tip->balloonLabel, &args[0], XtNumber(args));
		}
# endif
#endif
	    }
	    else
	    {
		gui_athena_menu_font(mp->id);
#ifdef FEAT_BEVAL
		/* If we have a tooltip, then we need to change it's font */
		/* Assume XtNinternational == True (in createBalloonEvalWindow)
		 */
		if (mp->tip != NULL)
		{
		    Arg args[1];

		    args[0].name = XtNfontSet;
		    args[0].value = (XtArgVal)gui.tooltip_fontset;
		    XtSetValues(mp->tip->balloonLabel, &args[0], XtNumber(args));
		}
#endif
	    }
	}

	if (mp->children != NULL)
	{
	    /* Set the colors/font for the tear off widget */
	    if (mp->submenu_id != (Widget)0)
	    {
		if (colors)
		    gui_athena_menu_colors(mp->submenu_id);
		else
		    gui_athena_menu_font(mp->submenu_id);
	    }
	    /* Set the colors for the children */
	    gui_mch_submenu_change(mp->children, colors);
	}
    }
}

/*
 * Make a submenu name into a pullright name.
 * Replace '.' by '_', can't include '.' in the submenu name.
 */
    static char_u *
make_pull_name(name)
    char_u * name;
{
    char_u  *pname;
    char_u  *p;

    pname = vim_strnsave(name, STRLEN(name) + strlen("-pullright"));
    if (pname != NULL)
    {
	strcat((char *)pname, "-pullright");
	while ((p = vim_strchr(pname, '.')) != NULL)
	    *p = '_';
    }
    return pname;
}

/* ARGSUSED */
    void
gui_mch_add_menu_item(menu, idx)
    vimmenu_T	*menu;
    int		idx;
{
    vimmenu_T	*parent = menu->parent;

    a_cur_menu = menu;
# ifdef FEAT_TOOLBAR
    if (menu_is_toolbar(parent->name))
    {
	WidgetClass	type;
	int		n;
	Arg		args[21];

	n = 0;
	if (menu_is_separator(menu->name))
	{
	    XtSetArg(args[n], XtNlabel, ""); n++;
	    XtSetArg(args[n], XtNborderWidth, 0); n++;
	}
	else
	{
	    get_toolbar_pixmap(menu, &menu->image);
	    XtSetArg(args[n], XtNlabel, menu->dname); n++;
	    XtSetArg(args[n], XtNinternalHeight, 1); n++;
	    XtSetArg(args[n], XtNinternalWidth, 1); n++;
	    XtSetArg(args[n], XtNborderWidth, 1); n++;
	    if (menu->image != 0)
		XtSetArg(args[n], XtNbitmap, menu->image); n++;
	}
	XtSetArg(args[n], XtNhighlightThickness, 0); n++;
	type = commandWidgetClass;
	/* TODO: figure out the position in the toolbar?
	 *       This currently works fine for the default toolbar, but
	 *       what if we add/remove items during later runtime?
	 */

	/* NOTE: "idx" isn't used here.  The position is calculated by
	 *       athena_calculate_ins_pos().  The position it calculates
	 *       should be equal to "idx".
	 */
	/* TODO: Could we just store "idx" and use that as the child
	 * placement?
	 */

	if (menu->id == NULL)
	{
	    menu->id = XtCreateManagedWidget((char *)menu->dname,
			type, toolBar, args, n);
	    XtAddCallback(menu->id,
		    XtNcallback, gui_x11_menu_cb, menu);
	}
	else
	    XtSetValues(menu->id, args, n);
	gui_athena_menu_colors(menu->id);

#ifdef FEAT_BEVAL
	gui_mch_menu_set_tip(menu);
#endif

	menu->parent = parent;
	menu->submenu_id = NULL;
	if (!XtIsManaged(toolBar)
		    && vim_strchr(p_go, GO_TOOLBAR) != NULL)
	    gui_mch_show_toolbar(TRUE);
	gui.toolbar_height = gui_mch_compute_toolbar_height();
	return;
    } /* toolbar menu item */
# endif

    /* Add menu separator */
    if (menu_is_separator(menu->name))
    {
	menu->submenu_id = (Widget)0;
	menu->id = XtVaCreateManagedWidget((char *)menu->dname,
		smeLineObjectClass, parent->submenu_id,
		NULL);
	if (menu->id == (Widget)0)
	    return;
	gui_athena_menu_colors(menu->id);
    }
    else
    {
	if (parent != NULL && parent->submenu_id != (Widget)0)
	{
	    menu->submenu_id = (Widget)0;
	    menu->id = XtVaCreateManagedWidget((char *)menu->dname,
		    smeBSBObjectClass, parent->submenu_id,
		    XtNlabel, menu->dname,
#ifdef FONTSET_ALWAYS
		    XtNinternational,	True,
#endif
		    NULL);
	    if (menu->id == (Widget)0)
		return;

	    /* If there are other "pulldown" items in this pane, then adjust
	     * the right margin to accomodate the arrow pixmap, otherwise
	     * the right margin will be the same as the left margin.
	     */
	    {
		Dimension   left_margin;

		XtVaGetValues(menu->id, XtNleftMargin, &left_margin, NULL);
		XtVaSetValues(menu->id, XtNrightMargin,
			gui_athena_menu_has_submenus(parent->submenu_id, NULL) ?
			    puller_width :
			    left_margin,
			NULL);
	    }

	    gui_athena_menu_colors(menu->id);
	    gui_athena_menu_font(menu->id);
	    XtAddCallback(menu->id, XtNcallback, gui_x11_menu_cb,
		    (XtPointer)menu);
	}
    }
    a_cur_menu = NULL;
}

#if defined(FEAT_TOOLBAR) || defined(PROTO)
    void
gui_mch_show_toolbar(int showit)
{
    Cardinal	numChildren;	    /* how many children toolBar has */

    if (toolBar == (Widget)0)
	return;
    XtVaGetValues(toolBar, XtNnumChildren, &numChildren, NULL);
    if (showit && numChildren > 0)
    {
	/* Assume that we want to show the toolbar if p_toolbar contains valid
	 * option settings, therefore p_toolbar must not be NULL.
	 */
	WidgetList  children;

	XtVaGetValues(toolBar, XtNchildren, &children, NULL);
	{
	    void    (*action)(BalloonEval *);
	    int	    text = 0;

	    if (strstr((const char *)p_toolbar, "tooltips"))
		action = &gui_mch_enable_beval_area;
	    else
		action = &gui_mch_disable_beval_area;
	    if (strstr((const char *)p_toolbar, "text"))
		text = 1;
	    else if (strstr((const char *)p_toolbar, "icons"))
		text = -1;
	    if (text != 0)
	    {
		vimmenu_T   *toolbar;
		vimmenu_T   *cur;

		for (toolbar = root_menu; toolbar; toolbar = toolbar->next)
		    if (menu_is_toolbar(toolbar->dname))
			break;
		/* Assumption: toolbar is NULL if there is no toolbar,
		 *	       otherwise it contains the toolbar menu structure.
		 *
		 * Assumption: "numChildren" == the number of items in the list
		 *	       of items beginning with toolbar->children.
		 */
		if (toolbar)
		{
		    for (cur = toolbar->children; cur; cur = cur->next)
		    {
			Arg	    args[2];
			int	    n = 0;

			/* Enable/Disable tooltip (OK to enable while currently
			 * enabled)
			 */
			if (cur->tip != NULL)
			    (*action)(cur->tip);
			if (text == 1)
			{
			    XtSetArg(args[n], XtNbitmap, None);
			    n++;
			    XtSetArg(args[n], XtNlabel,
				    menu_is_separator(cur->name) ? "" :
					(char *)cur->dname);
			    n++;
			}
			else
			{
			    XtSetArg(args[n], XtNbitmap, cur->image);
			    n++;
			    XtSetArg(args[n], XtNlabel, (cur->image == None) ?
				    menu_is_separator(cur->name) ?
					"" :
					(char *)cur->dname
				    :
				    (char *)None);
			    n++;
			}
			if (cur->id != NULL)
			{
			    XtUnmanageChild(cur->id);
			    XtSetValues(cur->id, args, n);
			    XtManageChild(cur->id);
			}
		    }
		}
	    }
	}
	gui.toolbar_height = gui_mch_compute_toolbar_height();
	XtManageChild(toolBar);
	if (XtIsManaged(menuBar))
	{
	    XtVaSetValues(textArea,
		    XtNvertDistance,    gui.toolbar_height + gui.menu_height,
		    NULL);
	    XtVaSetValues(toolBar,
		    XtNvertDistance,    gui.menu_height,
		    NULL);
	}
	else
	{
	    XtVaSetValues(textArea,
		    XtNvertDistance,    gui.toolbar_height,
		    NULL);
	    XtVaSetValues(toolBar,
		    XtNvertDistance,    0,
		    NULL);
	}
    }
    else
    {
	gui.toolbar_height = 0;
	if (XtIsManaged(menuBar))
	    XtVaSetValues(textArea,
		XtNvertDistance,    gui.menu_height,
		NULL);
	else
	    XtVaSetValues(textArea,
		XtNvertDistance,    0,
		NULL);

	XtUnmanageChild(toolBar);
    }
    gui_set_shellsize(FALSE, FALSE, RESIZE_VERT);
}


    int
gui_mch_compute_toolbar_height()
{
    Dimension	height;		    /* total Toolbar height */
    Dimension	whgt;		    /* height of each widget */
    Dimension	marginHeight;	    /* XmNmarginHeight of toolBar */
    Dimension	shadowThickness;    /* thickness of Xtparent(toolBar) */
    WidgetList	children;	    /* list of toolBar's children */
    Cardinal	numChildren;	    /* how many children toolBar has */
    int		i;

    height = 0;
    shadowThickness = 0;
    marginHeight = 0;
    if (toolBar != (Widget)0)
    {
	XtVaGetValues(toolBar,
		XtNborderWidth,	    &shadowThickness,
		XtNvSpace,	    &marginHeight,
		XtNchildren,	    &children,
		XtNnumChildren,	    &numChildren,
		NULL);
	for (i = 0; i < numChildren; i++)
	{
	    whgt = 0;

	    XtVaGetValues(children[i], XtNheight, &whgt, NULL);
	    if (height < whgt)
		height = whgt;
	}
    }

    return (int)(height + (marginHeight << 1) + (shadowThickness << 1));
}

    void
gui_mch_get_toolbar_colors(bgp, fgp, bsp, tsp, hsp)
    Pixel	*bgp;
    Pixel	*fgp;
    Pixel       *bsp;
    Pixel	*tsp;
    Pixel	*hsp;
{
    XtVaGetValues(toolBar, XtNbackground, bgp, XtNborderColor, fgp, NULL);
    *bsp = *bgp;
    *tsp = *fgp;
    *hsp = *tsp;
}
#endif


/* ARGSUSED */
    void
gui_mch_toggle_tearoffs(enable)
    int		enable;
{
    /* no tearoff menus */
}

    void
gui_mch_new_menu_colors()
{
    if (menuBar == (Widget)0)
	return;
    if (gui.menu_fg_pixel != INVALCOLOR)
	XtVaSetValues(menuBar, XtNborderColor,	gui.menu_fg_pixel, NULL);
    gui_athena_menu_colors(menuBar);
#ifdef FEAT_TOOLBAR
    gui_athena_menu_colors(toolBar);
#endif

    gui_mch_submenu_change(root_menu, TRUE);
}

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(menu)
    vimmenu_T *menu;
{
    Widget	parent;

    /* There is no item for the toolbar. */
    if (menu->id == (Widget)0)
	return;

    parent = XtParent(menu->id);

    /* When removing the last "pulldown" menu item from a pane, adjust the
     * right margins of the remaining widgets.
     */
    if (menu->submenu_id != (Widget)0)
    {
	/* Go through the menu items in the parent of this item and
	 * adjust their margins, if necessary.
	 * This takes care of the case when we delete the last menu item in a
	 * pane that has a submenu.  In this case, there will be no arrow
	 * pixmaps shown anymore.
	 */
	{
	    WidgetList  children;
	    Cardinal    num_children;
	    int		i;
	    Dimension	right_margin = 0;
	    Boolean	get_left_margin = False;

	    XtVaGetValues(parent, XtNchildren, &children,
				  XtNnumChildren, &num_children,
				  NULL);
	    if (gui_athena_menu_has_submenus(parent, menu->id))
		right_margin = puller_width;
	    else
		get_left_margin = True;

	    for (i = 0; i < num_children; ++i)
	    {
		if (children[i] == menu->id)
		    continue;
		if (get_left_margin == True)
		{
		    Dimension left_margin;

		    XtVaGetValues(children[i], XtNleftMargin, &left_margin,
				  NULL);
		    XtVaSetValues(children[i], XtNrightMargin, left_margin,
				  NULL);
		}
		else
		    XtVaSetValues(children[i], XtNrightMargin, right_margin,
				  NULL);
	    }
	}
    }
    /* Please be sure to destroy the parent widget first (i.e. menu->id).
     *
     * This code should be basically identical to that in the file gui_motif.c
     * because they are both Xt based.
     */
    if (menu->id != (Widget)0)
    {
	Cardinal    num_children;
	Dimension   height, space, border;

	XtVaGetValues(menuBar,
		XtNvSpace,	&space,
		XtNborderWidth, &border,
		NULL);
	XtVaGetValues(menu->id,
		XtNheight,	&height,
		NULL);
#if defined(FEAT_TOOLBAR) && defined(FEAT_BEVAL)
	if (parent == toolBar && menu->tip != NULL)
	{
	    /* We try to destroy this before the actual menu, because there are
	     * callbacks, etc. that will be unregistered during the tooltip
	     * destruction.
	     *
	     * If you call "gui_mch_destroy_beval_area()" after destroying
	     * menu->id, then the tooltip's window will have already been
	     * deallocated by Xt, and unknown behaviour will ensue (probably
	     * a core dump).
	     */
	    gui_mch_destroy_beval_area(menu->tip);
	    menu->tip = NULL;
	}
#endif
	/*
	 * This is a hack to stop the Athena simpleMenuWidget from getting a
	 * BadValue error when a menu's last child is destroyed. We check to
	 * see if this is the last child and if so, don't delete it. The parent
	 * will be deleted soon anyway, and it will delete it's children like
	 * all good widgets do.
	 */
	/* NOTE: The cause of the BadValue X Protocol Error is because when the
	 * last child is destroyed, it is first unmanaged, thus causing a
	 * geometry resize request from the parent Shell widget.
	 * Since the Shell widget has no more children, it is resized to have
	 * width/height of 0.  XConfigureWindow() is then called with the
	 * width/height of 0, which generates the BadValue.
	 *
	 * This happens in phase two of the widget destruction process.
	 */
	{
	    if (parent != menuBar
#ifdef FEAT_TOOLBAR
		    && parent != toolBar
#endif
		    )
	    {
		XtVaGetValues(parent, XtNnumChildren, &num_children, NULL);
		if (num_children > 1)
		    XtDestroyWidget(menu->id);
	    }
	    else
		XtDestroyWidget(menu->id);
	    menu->id = (Widget)0;
	}

	if (parent == menuBar)
	{
	    if (!gui.menu_height_fixed)
		gui.menu_height = height + 2 * (space + border);
	}
#ifdef FEAT_TOOLBAR
	else if (parent == toolBar)
	{
	    /* When removing last toolbar item, don't display the toolbar. */
	    XtVaGetValues(toolBar, XtNnumChildren, &num_children, NULL);
	    if (num_children == 0)
		gui_mch_show_toolbar(FALSE);
	    else
		gui.toolbar_height = gui_mch_compute_toolbar_height();
	}
#endif
    }
    if (menu->submenu_id != (Widget)0)
    {
	XtDestroyWidget(menu->submenu_id);
	menu->submenu_id = (Widget)0;
    }
}

/*ARGSUSED*/
    static void
gui_athena_menu_timeout(client_data, id)
    XtPointer	    client_data;
    XtIntervalId    *id;
{
    Widget  w = (Widget)client_data;
    Widget  popup;

    timer = 0;
    if (XtIsSubclass(w,smeBSBObjectClass))
    {
	Pixmap p;

	XtVaGetValues(w, XtNrightBitmap, &p, NULL);
	if ((p != None) && (p != XtUnspecifiedPixmap))
	{
	    /* We are dealing with an item that has a submenu */
	    popup = get_popup_entry(XtParent(w));
	    if (popup == (Widget)0)
		return;
	    XtPopup(popup, XtGrabNonexclusive);
	}
    }
}

/* This routine is used to calculate the position (in screen coordinates)
 * where a submenu should appear relative to the menu entry that popped it
 * up.  It should appear even with and just slightly to the left of the
 * rightmost end of the menu entry that caused the popup.
 *
 * This is called when XtPopup() is called.
 */
/*ARGSUSED*/
    static void
gui_athena_popup_callback(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data;
    XtPointer	call_data;
{
    /* Assumption: XtIsSubclass(XtParent(w),simpleMenuWidgetClass) */
    vimmenu_T	*menu = (vimmenu_T *)client_data;
    Dimension	width;
    Position	root_x, root_y;

    /* First, popdown any siblings that may have menus popped up */
    {
	vimmenu_T *i;

	for (i = menu->parent->children; i != NULL; i = i->next)
	{
	    if (i->submenu_id != NULL && XtIsManaged(i->submenu_id))
		XtPopdown(i->submenu_id);
	}
    }
    XtVaGetValues(XtParent(w),
		  XtNwidth,   &width,
		  NULL);
    /* Assumption: XawSimpleMenuGetActiveEntry(XtParent(w)) == menu->id */
    /* i.e. This IS the active entry */
    XtTranslateCoords(menu->id,width - 5, 0, &root_x, &root_y);
    XtVaSetValues(w, XtNx, root_x,
		     XtNy, root_y,
		     NULL);
}

/* ARGSUSED */
    static void
gui_athena_popdown_submenus_action(w, event, args, nargs)
    Widget	w;
    XEvent	*event;
    String	*args;
    Cardinal	*nargs;
{
    WidgetList	children;
    Cardinal	num_children;

    XtVaGetValues(w, XtNchildren, &children,
		     XtNnumChildren, &num_children,
		     NULL);
    for (; num_children > 0; --num_children)
    {
	Widget child = children[num_children - 1];

	if (has_submenu(child))
	{
	    Widget temp_w;

	    temp_w = submenu_widget(child);
	    gui_athena_popdown_submenus_action(temp_w,event,args,nargs);
	    XtPopdown(temp_w);
	}
    }
}

/* Used to determine if the given widget has a submenu that can be popped up. */
    static Boolean
has_submenu(widget)
    Widget  widget;
{
    if ((widget != NULL) && XtIsSubclass(widget,smeBSBObjectClass))
    {
	Pixmap p;

	XtVaGetValues(widget, XtNrightBitmap, &p, NULL);
	if ((p != None) && (p != XtUnspecifiedPixmap))
	    return True;
    }
    return False;
}

/* ARGSUSED */
    static void
gui_athena_delayed_arm_action(w, event, args, nargs)
    Widget	w;
    XEvent	*event;
    String	*args;
    Cardinal	*nargs;
{
    Dimension	width, height;

    if (event->type != MotionNotify)
	return;

    XtVaGetValues(w,
	XtNwidth,   &width,
	XtNheight,  &height,
	NULL);

    if (event->xmotion.x >= (int)width || event->xmotion.y >= (int)height)
	return;

    {
	static Widget	    previous_active_widget = NULL;
	Widget		    current;

	current = XawSimpleMenuGetActiveEntry(w);
	if (current != previous_active_widget)
	{
	    if (timer)
	    {
		/* If the timeout hasn't been triggered, remove it */
		XtRemoveTimeOut(timer);
	    }
	    gui_athena_popdown_submenus_action(w,event,args,nargs);
	    if (has_submenu(current))
	    {
		XtAppAddTimeOut(XtWidgetToApplicationContext(w), 600L,
				gui_athena_menu_timeout,
				(XtPointer)current);
	    }
	    previous_active_widget = current;
	}
    }
}

    static Widget
get_popup_entry(w)
    Widget  w;
{
    Widget	menuw;

    /* Get the active entry for the current menu */
    if ((menuw = XawSimpleMenuGetActiveEntry(w)) == (Widget)0)
	return NULL;

    return submenu_widget(menuw);
}

/* Given the widget that has been determined to have a submenu, return the submenu widget
 * that is to be popped up.
 */
    static Widget
submenu_widget(widget)
    Widget  widget;
{
    /* Precondition: has_submenu(widget) == True
     *	    XtIsSubclass(XtParent(widget),simpleMenuWidgetClass) == True
     */

    char_u	*pullright_name;
    Widget	popup;

    pullright_name = make_pull_name((char_u *)XtName(widget));
    popup = XtNameToWidget(XtParent(widget), (char *)pullright_name);
    vim_free(pullright_name);

    return popup;
    /* Postcondition: (popup != NULL) implies
     * (XtIsSubclass(popup,simpleMenuWidgetClass) == True) */
}

/* ARGSUSED */
    void
gui_mch_show_popupmenu(menu)
    vimmenu_T *menu;
{
    int		rootx, rooty, winx, winy;
    Window	root, child;
    unsigned int mask;

    if (menu->submenu_id == (Widget)0)
	return;

    /* Position the popup menu at the pointer */
    if (XQueryPointer(gui.dpy, XtWindow(vimShell), &root, &child,
		&rootx, &rooty, &winx, &winy, &mask))
    {
	rootx -= 30;
	if (rootx < 0)
	    rootx = 0;
	rooty -= 5;
	if (rooty < 0)
	    rooty = 0;
	XtVaSetValues(menu->submenu_id,
		XtNx, rootx,
		XtNy, rooty,
		NULL);
    }

    XtOverrideTranslations(menu->submenu_id, popupTrans);
    XtPopupSpringLoaded(menu->submenu_id);
}

#endif /* FEAT_MENU */

/*
 * Set the menu and scrollbar colors to their default values.
 */
    void
gui_mch_def_colors()
{
    /*
     * Get the colors ourselves.  Using the automatic conversion doesn't
     * handle looking for approximate colors.
     */
    if (gui.in_use)
    {
	gui.menu_fg_pixel = gui_get_color((char_u *)gui.rsrc_menu_fg_name);
	gui.menu_bg_pixel = gui_get_color((char_u *)gui.rsrc_menu_bg_name);
	gui.scroll_fg_pixel = gui_get_color((char_u *)gui.rsrc_scroll_fg_name);
	gui.scroll_bg_pixel = gui_get_color((char_u *)gui.rsrc_scroll_bg_name);
#ifdef FEAT_BEVAL
	gui.tooltip_fg_pixel = gui_get_color((char_u *)gui.rsrc_tooltip_fg_name);
	gui.tooltip_bg_pixel = gui_get_color((char_u *)gui.rsrc_tooltip_bg_name);
#endif
    }
}


/*
 * Scrollbar stuff.
 */

    void
gui_mch_set_scrollbar_thumb(sb, val, size, max)
    scrollbar_T	*sb;
    long	val;
    long	size;
    long	max;
{
    double	v, s;

    if (sb->id == (Widget)0)
	return;

    /*
     * Athena scrollbar must go from 0.0 to 1.0.
     */
    if (max == 0)
    {
	/* So you can't scroll it at all (normally it scrolls past end) */
#ifdef FEAT_GUI_NEXTAW
	XawScrollbarSetThumb(sb->id, 0.0, 1.0);
#else
	vim_XawScrollbarSetThumb(sb->id, 0.0, 1.0, 0.0);
#endif
    }
    else
    {
	v = (double)val / (double)(max + 1);
	s = (double)size / (double)(max + 1);
#ifdef FEAT_GUI_NEXTAW
	XawScrollbarSetThumb(sb->id, v, s);
#else
	vim_XawScrollbarSetThumb(sb->id, v, s, 1.0);
#endif
    }
}

    void
gui_mch_set_scrollbar_pos(sb, x, y, w, h)
    scrollbar_T *sb;
    int		x;
    int		y;
    int		w;
    int		h;
{
    if (sb->id == (Widget)0)
	return;

    XtUnmanageChild(sb->id);
    XtVaSetValues(sb->id,
		  XtNhorizDistance, x,
		  XtNvertDistance, y,
		  XtNwidth, w,
		  XtNheight, h,
		  NULL);
    XtManageChild(sb->id);
}

    void
gui_mch_enable_scrollbar(sb, flag)
    scrollbar_T	*sb;
    int		flag;
{
    if (sb->id != (Widget)0)
    {
	if (flag)
	    XtManageChild(sb->id);
	else
	    XtUnmanageChild(sb->id);
    }
}

    void
gui_mch_create_scrollbar(sb, orient)
    scrollbar_T *sb;
    int		orient;	/* SBAR_VERT or SBAR_HORIZ */
{
    sb->id = XtVaCreateWidget("scrollBar",
#ifdef FEAT_GUI_NEXTAW
	    scrollbarWidgetClass, vimForm,
#else
	    vim_scrollbarWidgetClass, vimForm,
#endif
	    XtNresizable,   True,
	    XtNtop,	    XtChainTop,
	    XtNbottom,	    XtChainTop,
	    XtNleft,	    XtChainLeft,
	    XtNright,	    XtChainLeft,
	    XtNborderWidth, 0,
	    XtNorientation, (orient == SBAR_VERT) ? XtorientVertical
						  : XtorientHorizontal,
	    XtNforeground, gui.scroll_fg_pixel,
	    XtNbackground, gui.scroll_bg_pixel,
	    NULL);
    if (sb->id == (Widget)0)
	return;

    XtAddCallback(sb->id, XtNjumpProc,
		  gui_athena_scroll_cb_jump, (XtPointer)sb->ident);
    XtAddCallback(sb->id, XtNscrollProc,
		  gui_athena_scroll_cb_scroll, (XtPointer)sb->ident);

#ifdef FEAT_GUI_NEXTAW
    XawScrollbarSetThumb(sb->id, 0.0, 1.0);
#else
    vim_XawScrollbarSetThumb(sb->id, 0.0, 1.0, 0.0);
#endif
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
gui_mch_destroy_scrollbar(sb)
    scrollbar_T *sb;
{
    if (sb->id != (Widget)0)
	XtDestroyWidget(sb->id);
}
#endif

    void
gui_mch_set_scrollbar_colors(sb)
    scrollbar_T *sb;
{
    if (sb->id != (Widget)0)
	XtVaSetValues(sb->id,
	    XtNforeground, gui.scroll_fg_pixel,
	    XtNbackground, gui.scroll_bg_pixel,
	    NULL);

    /* This is needed for the rectangle below the vertical scrollbars. */
    if (sb == &gui.bottom_sbar && vimForm != (Widget)0)
	gui_athena_scroll_colors(vimForm);
}

/*
 * Miscellaneous stuff:
 */
    Window
gui_x11_get_wid()
{
    return XtWindow(textArea);
}

#if defined(FEAT_BROWSE) || defined(PROTO)
/*
 * Put up a file requester.
 * Returns the selected name in allocated memory, or NULL for Cancel.
 */
/* ARGSUSED */
    char_u *
gui_mch_browse(saving, title, dflt, ext, initdir, filter)
    int		saving;		/* select file to write */
    char_u	*title;		/* not used (title for the window) */
    char_u	*dflt;		/* not used (default name) */
    char_u	*ext;		/* not used (extension added) */
    char_u	*initdir;	/* initial directory, NULL for current dir */
    char_u	*filter;	/* not used (file name filter) */
{
    Position x, y;
    char_u	dirbuf[MAXPATHL];

    /* Concatenate "initdir" and "dflt". */
    if (initdir == NULL || *initdir == NUL)
	mch_dirname(dirbuf, MAXPATHL);
    else if (STRLEN(initdir) + 2 < MAXPATHL)
	STRCPY(dirbuf, initdir);
    else
	dirbuf[0] = NUL;
    if (dflt != NULL && *dflt != NUL
			      && STRLEN(dirbuf) + 2 + STRLEN(dflt) < MAXPATHL)
    {
	add_pathsep(dirbuf);
	STRCAT(dirbuf, dflt);
    }

    /* Position the file selector just below the menubar */
    XtTranslateCoords(vimShell, (Position)0, (Position)
#ifdef FEAT_MENU
	    gui.menu_height
#else
	    0
#endif
	    , &x, &y);
    return (char_u *)vim_SelFile(vimShell, (char *)title, (char *)dirbuf,
		  NULL, (int)x, (int)y, gui.menu_fg_pixel, gui.menu_bg_pixel,
		  gui.scroll_fg_pixel, gui.scroll_bg_pixel);
}
#endif

#if defined(FEAT_GUI_DIALOG) || defined(PROTO)

static int	dialogStatus;
static Atom	dialogatom;

static void keyhit_callback __ARGS((Widget w, XtPointer client_data, XEvent *event, Boolean *cont));
static void butproc __ARGS((Widget w, XtPointer client_data, XtPointer call_data));
static void dialog_wm_handler __ARGS((Widget w, XtPointer client_data, XEvent *event, Boolean *dum));

/*
 * Callback function for the textfield.  When CR is hit this works like
 * hitting the "OK" button, ESC like "Cancel".
 */
/* ARGSUSED */
    static void
keyhit_callback(w, client_data, event, cont)
    Widget		w;
    XtPointer		client_data;
    XEvent		*event;
    Boolean		*cont;
{
    char	buf[2];

    if (XLookupString(&(event->xkey), buf, 2, NULL, NULL) == 1)
    {
	if (*buf == CAR)
	    dialogStatus = 1;
	else if (*buf == ESC)
	    dialogStatus = 0;
    }
}

/* ARGSUSED */
    static void
butproc(w, client_data, call_data)
    Widget	w;
    XtPointer	client_data;
    XtPointer	call_data;
{
    dialogStatus = (int)(long)client_data + 1;
}

/*
 * Function called when dialog window closed.
 */
/*ARGSUSED*/
    static void
dialog_wm_handler(w, client_data, event, dum)
    Widget	w;
    XtPointer	client_data;
    XEvent	*event;
    Boolean	*dum;
{
    if (event->type == ClientMessage
	    && ((XClientMessageEvent *)event)->data.l[0] == dialogatom)
	dialogStatus = 0;
}

/* ARGSUSED */
    int
gui_mch_dialog(type, title, message, buttons, dfltbutton, textfield)
    int		type;
    char_u	*title;
    char_u	*message;
    char_u	*buttons;
    int		dfltbutton;
    char_u	*textfield;
{
    char_u		*buts;
    char_u		*p, *next;
    XtAppContext	app;
    XEvent		event;
    Position		wd, hd;
    Position		wv, hv;
    Position		x, y;
    Widget		dialog;
    Widget		dialogshell;
    Widget		dialogmessage;
    Widget		dialogtextfield = 0;
    Widget		dialogButton;
    Widget		prev_dialogButton = NULL;
    int			butcount;
    int			vertical;

    if (title == NULL)
	title = (char_u *)_("Vim dialog");
    dialogStatus = -1;

    /* if our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

    /* Check 'v' flag in 'guioptions': vertical button placement. */
    vertical = (vim_strchr(p_go, GO_VERTICAL) != NULL);

    /* The shell is created each time, to make sure it is resized properly */
    dialogshell = XtVaCreatePopupShell("dialogShell",
	    transientShellWidgetClass, vimShell,
	    XtNtitle, title,
	    NULL);
    if (dialogshell == (Widget)0)
	goto error;

    dialog = XtVaCreateManagedWidget("dialog",
	    formWidgetClass, dialogshell,
	    XtNdefaultDistance, 20,
	    NULL);
    if (dialog == (Widget)0)
	goto error;
    gui_athena_menu_colors(dialog);
    dialogmessage = XtVaCreateManagedWidget("dialogMessage",
	    labelWidgetClass, dialog,
	    XtNlabel, message,
	    XtNtop, XtChainTop,
	    XtNbottom, XtChainTop,
	    XtNleft, XtChainLeft,
	    XtNright, XtChainLeft,
	    XtNresizable, True,
	    XtNborderWidth, 0,
	    NULL);
    gui_athena_menu_colors(dialogmessage);

    if (textfield != NULL)
    {
	dialogtextfield = XtVaCreateManagedWidget("textfield",
		asciiTextWidgetClass, dialog,
		XtNwidth, 400,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainRight,
		XtNfromVert, dialogmessage,
		XtNresizable, True,
		XtNstring, textfield,
		XtNlength, IOSIZE,
		XtNuseStringInPlace, True,
		XtNeditType, XawtextEdit,
		XtNwrap, XawtextWrapNever,
		XtNresize, XawtextResizeHeight,
		NULL);
	XtManageChild(dialogtextfield);
	XtAddEventHandler(dialogtextfield, KeyPressMask, False,
			    (XtEventHandler)keyhit_callback, (XtPointer)NULL);
	XawTextSetInsertionPoint(dialogtextfield,
					  (XawTextPosition)STRLEN(textfield));
	XtSetKeyboardFocus(dialog, dialogtextfield);
    }

    /* make a copy, so that we can insert NULs */
    buts = vim_strsave(buttons);
    if (buts == NULL)
	return -1;

    p = buts;
    for (butcount = 0; *p; ++butcount)
    {
	for (next = p; *next; ++next)
	{
	    if (*next == DLG_HOTKEY_CHAR)
		mch_memmove(next, next + 1, STRLEN(next));
	    if (*next == DLG_BUTTON_SEP)
	    {
		*next++ = NUL;
		break;
	    }
	}
	dialogButton = XtVaCreateManagedWidget("button",
		commandWidgetClass, dialog,
		XtNlabel, p,
		XtNtop, XtChainBottom,
		XtNbottom, XtChainBottom,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		XtNfromVert, textfield == NULL ? dialogmessage : dialogtextfield,
		XtNvertDistance, vertical ? 4 : 20,
		XtNresizable, False,
		NULL);
	gui_athena_menu_colors(dialogButton);
	if (butcount > 0)
	    XtVaSetValues(dialogButton,
		    vertical ? XtNfromVert : XtNfromHoriz, prev_dialogButton,
		    NULL);

	XtAddCallback(dialogButton, XtNcallback, butproc, (XtPointer)butcount);
	p = next;
	prev_dialogButton = dialogButton;
    }
    vim_free(buts);

    XtRealizeWidget(dialogshell);

    /* Setup for catching the close-window event, don't let it close Vim! */
    dialogatom = XInternAtom(gui.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(gui.dpy, XtWindow(dialogshell), &dialogatom, 1);
    XtAddEventHandler(dialogshell, NoEventMask, True, dialog_wm_handler, NULL);

    XtVaGetValues(dialogshell,
	    XtNwidth, &wd,
	    XtNheight, &hd,
	    NULL);
    XtVaGetValues(vimShell,
	    XtNwidth, &wv,
	    XtNheight, &hv,
	    NULL);
    XtTranslateCoords(vimShell,
	    (Position)((wv - wd) / 2),
	    (Position)((hv - hd) / 2),
	    &x, &y);
    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;
    XtVaSetValues(dialogshell, XtNx, x, XtNy, y, NULL);

    /* Position the mouse pointer in the dialog, required for when focus
     * follows mouse. */
    XWarpPointer(gui.dpy, (Window)0, XtWindow(dialogshell), 0, 0, 0, 0, 20, 40);


    app = XtWidgetToApplicationContext(dialogshell);

    XtPopup(dialogshell, XtGrabNonexclusive);

    for (;;)
    {
	XtAppNextEvent(app, &event);
	XtDispatchEvent(&event);
	if (dialogStatus >= 0)
	    break;
    }

    XtPopdown(dialogshell);

    if (textfield != NULL && dialogStatus < 0)
	*textfield = NUL;

error:
    XtDestroyWidget(dialogshell);

    return dialogStatus;
}
#endif

#if defined(FEAT_GUI_DIALOG) || defined(FEAT_MENU)
/*
 * Set the colors of Widget "id" to the menu colors.
 */
    static void
gui_athena_menu_colors(id)
    Widget  id;
{
    if (gui.menu_bg_pixel != INVALCOLOR)
	XtVaSetValues(id, XtNbackground, gui.menu_bg_pixel, NULL);
    if (gui.menu_fg_pixel != INVALCOLOR)
	XtVaSetValues(id, XtNforeground, gui.menu_fg_pixel, NULL);
}
#endif

/*
 * Set the colors of Widget "id" to the scroll colors.
 */
    static void
gui_athena_scroll_colors(id)
    Widget  id;
{
    if (gui.scroll_bg_pixel != INVALCOLOR)
	XtVaSetValues(id, XtNbackground, gui.scroll_bg_pixel, NULL);
    if (gui.scroll_fg_pixel != INVALCOLOR)
	XtVaSetValues(id, XtNforeground, gui.scroll_fg_pixel, NULL);
}
