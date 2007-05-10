/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI support by Robert Webb
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * gui_w16.c
 *
 * GUI support for Microsoft Windows 3.1x
 *
 * George V. Reilly <george@reilly.org> wrote the original Win32 GUI.
 * Robert Webb reworked it to use the existing GUI stuff and added menu,
 * scrollbars, etc.
 *
 * Vince Negri then butchered the code to get it compiling for
 * 16-bit windows.
 *
 */

/*
 * Include the common stuff for MS-Windows GUI.
 */
#include "gui_w48.c"

#include "guiw16rc.h"

/* Undocumented Windows Message - not even defined in some SDK headers */
#define WM_EXITSIZEMOVE			0x0232


#ifdef FEAT_TOOLBAR
# define CMD_TB_BASE (99)
# include <vimtbar.h>
#endif

#ifdef PROTO
# define WINAPI
#endif

#define HANDLE_WM_DROPFILES(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HDROP)(wParam)), 0L)


/* Local variables: */

#ifdef FEAT_MENU
static UINT	s_menu_id = 100;
#endif


#define VIM_NAME	"vim"
#define VIM_CLASS	"Vim"

#define DLG_ALLOC_SIZE 16 * 1024

/*
 * stuff for dialogs, menus, tearoffs etc.
 */
#if defined(FEAT_GUI_DIALOG) || defined(PROTO)
static BOOL CALLBACK dialog_callback(HWND, UINT, WPARAM, LPARAM);

static LPWORD
add_dialog_element(
	LPWORD p,
	DWORD lStyle,
	WORD x,
	WORD y,
	WORD w,
	WORD h,
	WORD Id,
	BYTE clss,
	const char *caption);

static int dialog_default_button = -1;
#endif

static void get_dialog_font_metrics(void);

#ifdef FEAT_TOOLBAR
static void initialise_toolbar(void);
#endif


#ifdef FEAT_MENU
/*
 * Figure out how high the menu bar is at the moment.
 */
    static int
gui_mswin_get_menu_height(
    int	    fix_window)	    /* If TRUE, resize window if menu height changed */
{
    static int	old_menu_height = -1;

    int	    num;
    int	    menu_height;

    if (gui.menu_is_active)
	num = GetMenuItemCount(s_menuBar);
    else
	num = 0;

    if (num == 0)
	menu_height = 0;
    else if (gui.starting)
	menu_height = GetSystemMetrics(SM_CYMENU);
    else
    {
	RECT r1, r2;
	int frameht = GetSystemMetrics(SM_CYFRAME);
	int capht = GetSystemMetrics(SM_CYCAPTION);

	/*	get window rect of s_hwnd
		 * get client rect of s_hwnd
		 * get cap height
		 * subtract from window rect, the sum of client height,
		 * (if not maximized)frame thickness, and caption height.
	 */
	GetWindowRect(s_hwnd, &r1);
	GetClientRect(s_hwnd, &r2);
	menu_height = r1.bottom - r1.top - (r2.bottom-r2.top +
			       2 * frameht * (!IsZoomed(s_hwnd)) + capht);
    }

    if (fix_window && menu_height != old_menu_height)
    {
	old_menu_height = menu_height;
	gui_set_shellsize(FALSE, FALSE, RESIZE_VERT);
    }

    return menu_height;
}
#endif /*FEAT_MENU*/


/*
 * Even though we have _DuringSizing() which makes the rubber band a valid
 * size, we need this for when the user maximises the window.
 * TODO: Doesn't seem to adjust the width though for some reason.
 */
    static BOOL
_OnWindowPosChanging(
    HWND hwnd,
    LPWINDOWPOS lpwpos)
{

    if (!IsIconic(hwnd) && !(lpwpos->flags & SWP_NOSIZE))
    {
	gui_mswin_get_valid_dimensions(lpwpos->cx, lpwpos->cy,
				     &lpwpos->cx, &lpwpos->cy);
    }
    return 0;
}





    static LRESULT CALLBACK
_WndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    /*
    TRACE("WndProc: hwnd = %08x, msg = %x, wParam = %x, lParam = %x\n",
	  hwnd, uMsg, wParam, lParam);
    */

    HandleMouseHide(uMsg, lParam);

    s_uMsg = uMsg;
    s_wParam = wParam;
    s_lParam = lParam;

    switch (uMsg)
    {
	HANDLE_MSG(hwnd, WM_DEADCHAR,	_OnDeadChar);
	HANDLE_MSG(hwnd, WM_SYSDEADCHAR, _OnDeadChar);
	/* HANDLE_MSG(hwnd, WM_ACTIVATE,    _OnActivate); */
	HANDLE_MSG(hwnd, WM_CHAR,	_OnChar);
	HANDLE_MSG(hwnd, WM_CLOSE,	_OnClose);
	/* HANDLE_MSG(hwnd, WM_COMMAND,	_OnCommand); */
	HANDLE_MSG(hwnd, WM_DESTROY,	_OnDestroy);
	HANDLE_MSG(hwnd, WM_DROPFILES,	_OnDropFiles);
	HANDLE_MSG(hwnd, WM_HSCROLL,	_OnScroll);
	HANDLE_MSG(hwnd, WM_KILLFOCUS,	_OnKillFocus);
#ifdef FEAT_MENU
	HANDLE_MSG(hwnd, WM_COMMAND,	_OnMenu);
#endif
	/* HANDLE_MSG(hwnd, WM_MOVE,	    _OnMove); */
	/* HANDLE_MSG(hwnd, WM_NCACTIVATE,  _OnNCActivate); */
	HANDLE_MSG(hwnd, WM_SETFOCUS,	_OnSetFocus);
	HANDLE_MSG(hwnd, WM_SIZE,	_OnSize);
	/* HANDLE_MSG(hwnd, WM_SYSCOMMAND,  _OnSysCommand); */
	/* HANDLE_MSG(hwnd, WM_SYSKEYDOWN,  _OnAltKey); */
	HANDLE_MSG(hwnd, WM_VSCROLL,	_OnScroll);
	HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING,	_OnWindowPosChanging);
	HANDLE_MSG(hwnd, WM_ACTIVATEAPP, _OnActivateApp);

    case WM_QUERYENDSESSION:	/* System wants to go down. */
	gui_shell_closed();	/* Will exit when no changed buffers. */
	return FALSE;		/* Do NOT allow system to go down. */

    case WM_ENDSESSION:
	if (wParam)	/* system only really goes down when wParam is TRUE */
	    _OnEndSession();
	break;

    case WM_SYSCHAR:
	/*
	 * if 'winaltkeys' is "no", or it's "menu" and it's not a menu
	 * shortcut key, handle like a typed ALT key, otherwise call Windows
	 * ALT key handling.
	 */
#ifdef FEAT_MENU
	if (	!gui.menu_is_active
		|| p_wak[0] == 'n'
		|| (p_wak[0] == 'm' && !gui_is_menu_shortcut((int)wParam))
		)
#endif
	    return HANDLE_WM_SYSCHAR((hwnd), (wParam), (lParam), (_OnSysChar));
#ifdef FEAT_MENU
	else
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
#endif

    case WM_SYSKEYUP:
#ifdef FEAT_MENU
	/* Only when menu is active, ALT key is used for that. */
	if (gui.menu_is_active)
	{
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
	}
	else
#endif
	    return 0;

#if defined(MENUHINTS) && defined(FEAT_MENU)
    case WM_MENUSELECT:
	if (((UINT) LOWORD(lParam)
		    & (0xffff ^ (MF_MOUSESELECT + MF_BITMAP + MF_POPUP)))
		== MF_HILITE
		&& (State & CMDLINE) == 0)
	{
	    UINT idButton;
	    int	idx;
	    vimmenu_T *pMenu;

	    idButton = (UINT)LOWORD(wParam);
	    pMenu = gui_mswin_find_menu(root_menu, idButton);
	    if (pMenu)
	    {
		idx = MENU_INDEX_TIP;
		msg_clr_cmdline();
		if (pMenu->strings[idx])
		    msg(pMenu->strings[idx]);
		else
		    msg("");
		setcursor();
		out_flush();
	    }
	}
	break;
#endif
    case WM_NCHITTEST:
	{
	    LRESULT	result;
	    int x, y;
	    int xPos = GET_X_LPARAM(lParam);

	    result = MyWindowProc(hwnd, uMsg, wParam, lParam);
	    if (result == HTCLIENT)
	    {
		gui_mch_get_winpos(&x, &y);
		xPos -= x;

		if (xPos < 48) /*<VN> TODO should use system metric?*/
		    return HTBOTTOMLEFT;
		else
		    return HTBOTTOMRIGHT;
		}
	    else
		return result;
	}
	/* break; */
    default:
#ifdef MSWIN_FIND_REPLACE
	if (uMsg == s_findrep_msg && s_findrep_msg != 0)
	{
	    _OnFindRepl();
	}
#endif
	return MyWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 1;
}



/*
 * End of call-back routines
 */


/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(int *argc, char **argv)
{
    /* No special args for win16 GUI at the moment. */

}

/*
 * Initialise the GUI.	Create all the windows, set up all the call-backs
 * etc.
 */
    int
gui_mch_init(void)
{
    const char szVimWndClass[] = VIM_CLASS;
    const char szTextAreaClass[] = "VimTextArea";
    WNDCLASS wndclass;

#ifdef WIN16_3DLOOK
    Ctl3dRegister(s_hinst);
    Ctl3dAutoSubclass(s_hinst);
#endif

    /* Display any pending error messages */
    display_errors();

    gui.scrollbar_width = GetSystemMetrics(SM_CXVSCROLL);
    gui.scrollbar_height = GetSystemMetrics(SM_CYHSCROLL);
#ifdef FEAT_MENU
    gui.menu_height = 0;	/* Windows takes care of this */
#endif
    gui.border_width = 0;

    gui.currBgColor = INVALCOLOR;

    s_brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    if (GetClassInfo(s_hinst, szVimWndClass, &wndclass) == 0) {
	wndclass.style = 0;
	wndclass.lpfnWndProc = _WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = s_hinst;
	wndclass.hIcon = LoadIcon(wndclass.hInstance, MAKEINTRESOURCE(IDR_VIM));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = s_brush;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szVimWndClass;

    if ((
#ifdef GLOBAL_IME
	atom =
#endif
		RegisterClass(&wndclass)) == 0)
	    return FAIL;
    }

    s_hwnd = CreateWindow(
	szVimWndClass, "Vim MSWindows GUI",
	WS_OVERLAPPEDWINDOW,
	gui_win_x == -1 ? CW_USEDEFAULT : gui_win_x,
	gui_win_y == -1 ? CW_USEDEFAULT : gui_win_y,
	100,				/* Any value will do */
	100,				/* Any value will do */
	NULL, NULL,
	s_hinst, NULL);

    if (s_hwnd == NULL)
	return FAIL;

#ifdef GLOBAL_IME
    global_ime_init(atom, s_hwnd);
#endif

    /* Create the text area window */
    if (GetClassInfo(s_hinst, szTextAreaClass, &wndclass) == 0) {
	wndclass.style = CS_OWNDC;
	wndclass.lpfnWndProc = _TextAreaWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = s_hinst;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szTextAreaClass;

	if (RegisterClass(&wndclass) == 0)
	    return FAIL;
    }
    s_textArea = CreateWindow(
	szTextAreaClass, "Vim text area",
	WS_CHILD | WS_VISIBLE, 0, 0,
	100,				/* Any value will do for now */
	100,				/* Any value will do for now */
	s_hwnd, NULL,
	s_hinst, NULL);

    if (s_textArea == NULL)
	return FAIL;

#ifdef FEAT_MENU
    s_menuBar = CreateMenu();
#endif
    s_hdc = GetDC(s_textArea);

#ifdef MSWIN16_FASTTEXT
    SetBkMode(s_hdc, OPAQUE);
#endif

    DragAcceptFiles(s_hwnd, TRUE);

    /* Do we need to bother with this? */
    /* m_fMouseAvail = GetSystemMetrics(SM_MOUSEPRESENT); */

    /* Get background/foreground colors from the system */
    gui_mch_def_colors();

    /* Get the colors from the "Normal" group (set in syntax.c or in a vimrc
     * file) */
    set_normal_colors();

    /*
     * Check that none of the colors are the same as the background color.
     * Then store the current values as the defaults.
     */
    gui_check_colors();
    gui.def_norm_pixel = gui.norm_pixel;
    gui.def_back_pixel = gui.back_pixel;

    /* Get the colors for the highlight groups (gui_check_colors() might have
     * changed them) */
    highlight_gui_started();

    /*
     * Start out by adding the configured border width into the border offset
     */
    gui.border_offset = gui.border_width;


    /*
     * compute a couple of metrics used for the dialogs
     */
    get_dialog_font_metrics();
#ifdef FEAT_TOOLBAR
    /*
     * Create the toolbar
     */
    initialise_toolbar();
#endif
#ifdef MSWIN_FIND_REPLACE
    /*
     * Initialise the dialog box stuff
     */
    s_findrep_msg = RegisterWindowMessage(FINDMSGSTRING);

    /* Initialise the struct */
    s_findrep_struct.lStructSize = sizeof(s_findrep_struct);
    s_findrep_struct.lpstrFindWhat = alloc(MSWIN_FR_BUFSIZE);
    s_findrep_struct.lpstrFindWhat[0] = NUL;
    s_findrep_struct.lpstrReplaceWith = alloc(MSWIN_FR_BUFSIZE);
    s_findrep_struct.lpstrReplaceWith[0] = NUL;
    s_findrep_struct.wFindWhatLen = MSWIN_FR_BUFSIZE;
    s_findrep_struct.wReplaceWithLen = MSWIN_FR_BUFSIZE;
#endif

    return OK;
}


/*
 * Set the size of the window to the given width and height in pixels.
 */
    void
gui_mch_set_shellsize(int width, int height,
	int min_width, int min_height, int base_width, int base_height,
	int direction)
{
    RECT	workarea_rect;
    int		win_width, win_height;
    int		win_xpos, win_ypos;
    WINDOWPLACEMENT wndpl;

    /* try to keep window completely on screen */
    /* get size of the screen work area - use SM_CYFULLSCREEN
     * instead of SM_CYSCREEN so that we don't overlap the
     * taskbar if someone fires us up on Win95/NT */
    workarea_rect.left = 0;
    workarea_rect.top = 0;
    workarea_rect.right = GetSystemMetrics(SM_CXSCREEN);
    workarea_rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);

    /* get current posision of our window */
    wndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(s_hwnd, &wndpl);
    if (wndpl.showCmd == SW_SHOWNORMAL)
    {
	win_xpos = wndpl.rcNormalPosition.left;
	win_ypos = wndpl.rcNormalPosition.top;
    }
    else
    {
	win_xpos = workarea_rect.left;
	win_ypos = workarea_rect.top;
    }

    /* compute the size of the outside of the window */
    win_width = width + GetSystemMetrics(SM_CXFRAME) * 2;
    win_height = height + GetSystemMetrics(SM_CYFRAME) * 2
			+ GetSystemMetrics(SM_CYCAPTION)
#ifdef FEAT_MENU
			+ gui_mswin_get_menu_height(FALSE)
#endif
			;

    /* if the window is going off the screen, move it on to the screen */
    if ((direction & RESIZE_HOR) && win_xpos + win_width > workarea_rect.right)
	win_xpos = workarea_rect.right - win_width;

    if ((direction & RESIZE_HOR) && win_xpos < workarea_rect.left)
	win_xpos = workarea_rect.left;

    if ((direction & RESIZE_VERT)
			       && win_ypos + win_height > workarea_rect.bottom)
	win_ypos = workarea_rect.bottom - win_height;

    if ((direction & RESIZE_VERT) && win_ypos < workarea_rect.top)
	win_ypos = workarea_rect.top;

    /* set window position */
    SetWindowPos(s_hwnd, NULL, win_xpos, win_ypos, win_width, win_height,
		 SWP_NOZORDER | SWP_NOACTIVATE);

#ifdef FEAT_MENU
    /* Menu may wrap differently now */
    gui_mswin_get_menu_height(!gui.starting);
#endif
}

    void
gui_mch_set_scrollbar_thumb(
    scrollbar_T     *sb,
    long	    val,
    long	    size,
    long	    max)
{
    sb->scroll_shift = 0;
    while (max > 32767)
    {
	max = (max + 1) >> 1;
	val  >>= 1;
	size >>= 1;
	++sb->scroll_shift;
    }

    if (sb->scroll_shift > 0)
	++size;

    SetScrollRange(sb->id, SB_CTL, 0, (int) max, FALSE);
    SetScrollPos(sb->id, SB_CTL, (int) val, TRUE);
}


/*
 * Set the current text font.
 */
    void
gui_mch_set_font(GuiFont font)
{
    gui.currFont = font;
    SelectFont(s_hdc, gui.currFont);
}

/*
 * Set the current text foreground color.
 */
    void
gui_mch_set_fg_color(guicolor_T color)
{
    gui.currFgColor = color;
    SetTextColor(s_hdc, gui.currFgColor);
}

/*
 * Set the current text background color.
 */
    void
gui_mch_set_bg_color(guicolor_T color)
{
    if (gui.currBgColor == color)
	return;

    gui.currBgColor = color;
    SetBkColor(s_hdc, gui.currBgColor);
}

/*
 * Set the current text special color.
 */
    void
gui_mch_set_sp_color(guicolor_T color)
{
    /* TODO */
}



    void
gui_mch_draw_string(
    int		row,
    int		col,
    char_u	*text,
    int		len,
    int		flags)
{
#ifndef MSWIN16_FASTTEXT
    static int	*padding = NULL;
    static int	pad_size = 0;
    int		i;
#endif
    HPEN	hpen, old_pen;
    int		y;

#ifndef MSWIN16_FASTTEXT
    /*
     * Italic and bold text seems to have an extra row of pixels at the bottom
     * (below where the bottom of the character should be).  If we draw the
     * characters with a solid background, the top row of pixels in the
     * character below will be overwritten.  We can fix this by filling in the
     * background ourselves, to the correct character proportions, and then
     * writing the character in transparent mode.  Still have a problem when
     * the character is "_", which gets written on to the character below.
     * New fix: set gui.char_ascent to -1.  This shifts all characters up one
     * pixel in their slots, which fixes the problem with the bottom row of
     * pixels.	We still need this code because otherwise the top row of pixels
     * becomes a problem. - webb.
     */
    HBRUSH	hbr;
    RECT	rc;

    if (!(flags & DRAW_TRANSP))
    {
	/*
	 * Clear background first.
	 * Note: FillRect() excludes right and bottom of rectangle.
	 */
	rc.left = FILL_X(col);
	rc.top = FILL_Y(row);
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    int cell_len = 0;

	    /* Compute the length in display cells. */
	    for (n = 0; n < len; n += MB_BYTE2LEN(text[n]))
		cell_len += (*mb_ptr2cells)(text + n);
	    rc.right = FILL_X(col + cell_len);
	}
	else
#endif
	    rc.right = FILL_X(col + len);
	rc.bottom = FILL_Y(row + 1);
	hbr = CreateSolidBrush(gui.currBgColor);
	FillRect(s_hdc, &rc, hbr);
	DeleteBrush(hbr);

	SetBkMode(s_hdc, TRANSPARENT);

	/*
	 * When drawing block cursor, prevent inverted character spilling
	 * over character cell (can happen with bold/italic)
	 */
	if (flags & DRAW_CURSOR)
	{
	    pcliprect = &rc;
	    foptions = ETO_CLIPPED;
	}
    }
#else
    /*
     * Alternative: write the characters in opaque mode, since we have blocked
     * bold or italic fonts.
     */
    /* The OPAQUE mode and backcolour have already been set */
#endif
    /* The forecolor and font have already been set */

#ifndef MSWIN16_FASTTEXT

    if (pad_size != Columns || padding == NULL || padding[0] != gui.char_width)
    {
	vim_free(padding);
	pad_size = Columns;

	padding = (int *)alloc(pad_size * sizeof(int));
	if (padding != NULL)
	    for (i = 0; i < pad_size; i++)
		padding[i] = gui.char_width;
    }
#endif

    /*
     * We have to provide the padding argument because italic and bold versions
     * of fixed-width fonts are often one pixel or so wider than their normal
     * versions.
     * No check for DRAW_BOLD, Windows will have done it already.
     */
#ifndef MSWIN16_FASTTEXT
    ExtTextOut(s_hdc, TEXT_X(col), TEXT_Y(row), 0, NULL,
						     (char *)text, len, padding);
#else
    TextOut(s_hdc, TEXT_X(col), TEXT_Y(row), (char *)text, len);
#endif

    if (flags & DRAW_UNDERL)
    {
	hpen = CreatePen(PS_SOLID, 1, gui.currFgColor);
	old_pen = SelectObject(s_hdc, hpen);
	/* When p_linespace is 0, overwrite the bottom row of pixels.
	 * Otherwise put the line just below the character. */
	y = FILL_Y(row + 1) - 1;
#ifndef MSWIN16_FASTTEXT
	if (p_linespace > 1)
	    y -= p_linespace - 1;
#endif
	MoveToEx(s_hdc, FILL_X(col), y, NULL);
	/* Note: LineTo() excludes the last pixel in the line. */
	LineTo(s_hdc, FILL_X(col + len), y);
	DeleteObject(SelectObject(s_hdc, old_pen));
    }
}


/*
 * Output routines.
 */

/* Flush any output to the screen */
    void
gui_mch_flush(void)
{
    /* Is anything needed here? */
}

    static void
clear_rect(RECT *rcp)
{
    /* Use trick for fast rect clear */
    gui_mch_set_bg_color(gui.back_pixel);
    ExtTextOut(s_hdc, 0, 0, ETO_CLIPPED | ETO_OPAQUE, rcp, NULL, 0, NULL);
}


    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{

    *screen_w = GetSystemMetrics(SM_CXFULLSCREEN)
	      - GetSystemMetrics(SM_CXFRAME) * 2;
    /* FIXME: dirty trick: Because the gui_get_base_height() doesn't include
     * the menubar for MSwin, we subtract it from the screen height, so that
     * the window size can be made to fit on the screen. */
    *screen_h = GetSystemMetrics(SM_CYFULLSCREEN)
	      - GetSystemMetrics(SM_CYFRAME) * 2
#ifdef FEAT_MENU
	      - gui_mswin_get_menu_height(FALSE)
#endif
	      ;
}


#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Add a sub menu to the menu bar.
 */
    void
gui_mch_add_menu(
    vimmenu_T	*menu,
    int		pos)
{
    vimmenu_T	*parent = menu->parent;

    menu->submenu_id = CreatePopupMenu();
    menu->id = s_menu_id++;

    if (menu_is_menubar(menu->name))
    {
	InsertMenu((parent == NULL) ? s_menuBar : parent->submenu_id,
		(UINT)pos, MF_POPUP | MF_STRING | MF_BYPOSITION,
		(UINT)menu->submenu_id,  menu->name);
    }

    /* Fix window size if menu may have wrapped */
    if (parent == NULL)
	gui_mswin_get_menu_height(!gui.starting);
}

    void
gui_mch_show_popupmenu(vimmenu_T *menu)
{
    POINT mp;

    (void)GetCursorPos((LPPOINT)&mp);
    gui_mch_show_popupmenu_at(menu, (int)mp.x, (int)mp.y);
}

    void
gui_make_popup(char_u *path_name, int mouse_pos)
{
    vimmenu_T	*menu = gui_find_menu(path_name);

    if (menu != NULL)
    {
	/* Find the position of the current cursor */
	DWORD	temp_p;
	POINT	p;
	temp_p = GetDCOrg(s_hdc);
	p.x = LOWORD(temp_p);
	p.y = HIWORD(temp_p);
	if (mouse_pos)
	{
	    int	mx, my;

	    gui_mch_getmouse(&mx, &my);
	    p.x += mx;
	    p.y += my;
	}
	else if (curwin != NULL)
	{
	    p.x += TEXT_X(W_WINCOL(curwin) + curwin->w_wcol + 1);
	    p.y += TEXT_Y(W_WINROW(curwin) + curwin->w_wrow + 1);
	}
	msg_scroll = FALSE;
	gui_mch_show_popupmenu_at(menu, (int)p.x, (int)p.y);
    }
}

/*
 * Add a menu item to a menu
 */
    void
gui_mch_add_menu_item(
    vimmenu_T	*menu,
    int		idx)
{
    vimmenu_T	*parent = menu->parent;

    menu->id = s_menu_id++;
    menu->submenu_id = NULL;

#ifdef FEAT_TOOLBAR
    if (menu_is_toolbar(parent->name))
    {
	TBBUTTON newtb;

	vim_memset(&newtb, 0, sizeof(newtb));
	if (menu_is_separator(menu->name))
	{
	    newtb.iBitmap = 0;
	    newtb.fsStyle = TBSTYLE_SEP;
	}
	else
	{
	    if (menu->iconidx >= TOOLBAR_BITMAP_COUNT)
		newtb.iBitmap = -1;
	    else
		newtb.iBitmap = menu->iconidx;
	    newtb.fsStyle = TBSTYLE_BUTTON;
	}
	newtb.idCommand = menu->id;
	newtb.fsState = TBSTATE_ENABLED;
	SendMessage(s_toolbarhwnd, TB_INSERTBUTTON, (WPARAM)idx,
							     (LPARAM)&newtb);
	menu->submenu_id = (HMENU)-1;
    }
    else
#endif
    {
	InsertMenu(parent->submenu_id, (UINT)idx,
		(menu_is_separator(menu->name) ? MF_SEPARATOR : MF_STRING)
							      | MF_BYPOSITION,
		(UINT)menu->id, menu->name);
    }
}

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(vimmenu_T *menu)
{
    UINT i, j;
    char pants[80]; /*<VN> hack*/
#ifdef FEAT_TOOLBAR
    /*
     * is this a toolbar button?
     */
    if (menu->submenu_id == (HMENU)-1)
    {
	int iButton;

	iButton = SendMessage(s_toolbarhwnd, TB_COMMANDTOINDEX, (WPARAM)menu->id, 0);
	SendMessage(s_toolbarhwnd, TB_DELETEBUTTON, (WPARAM)iButton, 0);
    }
    else
#endif
    {
	/*
	 * negri: horrible API bug when running 16-bit programs under Win9x or
	 * NT means that we can't use MF_BYCOMMAND for menu items which have
	 * submenus, including the top-level headings. We have to find the menu
	 * item and use MF_BYPOSITION instead. :-p
	 */
    if (menu->parent != NULL
	    && menu_is_popup(menu->parent->dname)
	    && menu->parent->submenu_id != NULL)
	RemoveMenu(menu->parent->submenu_id, menu->id, MF_BYCOMMAND);
    else if (menu->submenu_id == NULL)
	RemoveMenu(s_menuBar, menu->id, MF_BYCOMMAND);
    else if (menu->parent != NULL)
    {
	i = GetMenuItemCount(menu->parent->submenu_id);
	for (j = 0; j < i; ++j)
	{
	    GetMenuString(menu->parent->submenu_id, j,
		    pants, 80, MF_BYPOSITION);
	    if (strcmp(pants, menu->name) == 0)
	    {
		RemoveMenu(menu->parent->submenu_id, j, MF_BYPOSITION);
		break;
	    }
	}
    }
    else
    {
	i = GetMenuItemCount(s_menuBar);
	for (j = 0; j < i; ++j)
	{
	    GetMenuString(s_menuBar, j, pants, 80, MF_BYPOSITION);
	    if (strcmp(pants, menu->name) == 0)
	    {
		RemoveMenu(s_menuBar, j, MF_BYPOSITION);
		break;
	    }
	}
    }

    if (menu->submenu_id != NULL)
	DestroyMenu(menu->submenu_id);
    }
    DrawMenuBar(s_hwnd);
}


/*
 * Make a menu either grey or not grey.
 */
    void
gui_mch_menu_grey(
    vimmenu_T *menu,
    int	    grey)
{
#ifdef FEAT_TOOLBAR
    /*
     * is this a toolbar button?
     */
    if (menu->submenu_id == (HMENU)-1)
    {
	SendMessage(s_toolbarhwnd, TB_ENABLEBUTTON,
	    (WPARAM)menu->id, (LPARAM) MAKELONG((grey ? FALSE : TRUE), 0) );
    }
    else
#endif
    if (grey)
	EnableMenuItem(s_menuBar, menu->id, MF_BYCOMMAND | MF_GRAYED);
    else
	EnableMenuItem(s_menuBar, menu->id, MF_BYCOMMAND | MF_ENABLED);

}


#endif /*FEAT_MENU*/


/* define some macros used to make the dialogue creation more readable */

#define add_string(s) strcpy((LPSTR)p, s); (LPSTR)p += (strlen((LPSTR)p) + 1)
#define add_word(x)		*p++ = (x)
#define add_byte(x)		*((LPSTR)p)++ = (x)
#define add_long(x)		*((LPDWORD)p)++ = (x)

#if defined(FEAT_GUI_DIALOG) || defined(PROTO)
/*
 * stuff for dialogs
 */

/*
 * The callback routine used by all the dialogs.  Very simple.  First,
 * acknowledges the INITDIALOG message so that Windows knows to do standard
 * dialog stuff (Return = default, Esc = cancel....) Second, if a button is
 * pressed, return that button's ID - IDCANCEL (2), which is the button's
 * number.
 */
	 static BOOL CALLBACK
dialog_callback(
	 HWND hwnd,
	 UINT message,
	 WPARAM wParam,
	 LPARAM lParam)
{
    if (message == WM_INITDIALOG)
    {
	CenterWindow(hwnd, GetWindow(hwnd, GW_OWNER));
	/* Set focus to the dialog.  Set the default button, if specified. */
	(void)SetFocus(hwnd);
	if (dialog_default_button > IDCANCEL)
	    (void)SetFocus(GetDlgItem(hwnd, dialog_default_button));
//	if (dialog_default_button > 0)
//	    (void)SetFocus(GetDlgItem(hwnd, dialog_default_button + IDCANCEL));
	return FALSE;
    }

    if (message == WM_COMMAND)
    {
	int	button = LOWORD(wParam);

	/* Don't end the dialog if something was selected that was
	 * not a button.
	 */
	if (button >= DLG_NONBUTTON_CONTROL)
	    return TRUE;

	/* If the edit box exists, copy the string. */
	if (s_textfield != NULL)
	    GetDlgItemText(hwnd, DLG_NONBUTTON_CONTROL + 2,
							 s_textfield, IOSIZE);

	/*
	 * Need to check for IDOK because if the user just hits Return to
	 * accept the default value, some reason this is what we get.
	 */
	if (button == IDOK)
	    EndDialog(hwnd, dialog_default_button);
	else
	    EndDialog(hwnd, button - IDCANCEL);
	return TRUE;
    }

    if ((message == WM_SYSCOMMAND) && (wParam == SC_CLOSE))
    {
	EndDialog(hwnd, 0);
	return TRUE;
    }
    return FALSE;
}

/*
 * Create a dialog dynamically from the parameter strings.
 * type		= type of dialog (question, alert, etc.)
 * title	= dialog title. may be NULL for default title.
 * message	= text to display. Dialog sizes to accommodate it.
 * buttons	= '\n' separated list of button captions, default first.
 * dfltbutton	= number of default button.
 *
 * This routine returns 1 if the first button is pressed,
 *			2 for the second, etc.
 *
 *			0 indicates Esc was pressed.
 *			-1 for unexpected error
 *
 * If stubbing out this fn, return 1.
 */

static const char_u dlg_icons[] = /* must match names in resource file */
{
    IDR_VIM,
    IDR_VIM_ERROR,
    IDR_VIM_ALERT,
    IDR_VIM_INFO,
    IDR_VIM_QUESTION
};

    int
gui_mch_dialog(
    int		 type,
    char_u	*title,
    char_u	*message,
    char_u	*buttons,
    int		 dfltbutton,
    char_u	*textfield)
{
    FARPROC	dp;
    LPWORD	p, pnumitems;
    int		numButtons;
    int		*buttonWidths, *buttonPositions;
    int		buttonYpos;
    int		nchar, i;
    DWORD	lStyle;
    int		dlgwidth = 0;
    int		dlgheight;
    int		editboxheight;
    int		horizWidth;
    int		msgheight;
    char_u	*pstart;
    char_u	*pend;
    char_u	*tbuffer;
    RECT	rect;
    HWND	hwnd;
    HDC		hdc;
    HFONT	oldFont;
    TEXTMETRIC	fontInfo;
    int		fontHeight;
    int		textWidth, minButtonWidth, messageWidth;
    int		maxDialogWidth;
    int		vertical;
    int		dlgPaddingX;
    int		dlgPaddingY;
    HGLOBAL	hglbDlgTemp;

#ifndef NO_CONSOLE
    /* Don't output anything in silent mode ("ex -s") */
    if (silent_mode)
	return dfltbutton;   /* return default option */
#endif

    /* If there is no window yet, open it. */
    if (s_hwnd == NULL && gui_mch_init() == FAIL)
	return dfltbutton;

    if ((type < 0) || (type > VIM_LAST_TYPE))
	type = 0;

    /* allocate some memory for dialog template */
    /* TODO should compute this really*/

    hglbDlgTemp = GlobalAlloc(GHND,  DLG_ALLOC_SIZE);
    if (hglbDlgTemp == NULL)
	return -1;

    p = (LPWORD) GlobalLock(hglbDlgTemp);

    if (p == NULL)
	return -1;

    /*
     * make a copy of 'buttons' to fiddle with it.  complier grizzles because
     * vim_strsave() doesn't take a const arg (why not?), so cast away the
     * const.
     */
    tbuffer = vim_strsave(buttons);
    if (tbuffer == NULL)
	return -1;

    --dfltbutton;   /* Change from one-based to zero-based */

    /* Count buttons */
    numButtons = 1;
    for (i = 0; tbuffer[i] != '\0'; i++)
    {
	if (tbuffer[i] == DLG_BUTTON_SEP)
	    numButtons++;
    }
    if (dfltbutton >= numButtons)
	dfltbutton = 0;

    /* Allocate array to hold the width of each button */
    buttonWidths = (int *) lalloc(numButtons * sizeof(int), TRUE);
    if (buttonWidths == NULL)
	return -1;

    /* Allocate array to hold the X position of each button */
    buttonPositions = (int *) lalloc(numButtons * sizeof(int), TRUE);
    if (buttonPositions == NULL)
	return -1;

    /*
     * Calculate how big the dialog must be.
     */
    hwnd = GetDesktopWindow();
    hdc = GetWindowDC(hwnd);
    oldFont = SelectFont(hdc, GetStockObject(SYSTEM_FONT));
    dlgPaddingX = DLG_OLD_STYLE_PADDING_X;
    dlgPaddingY = DLG_OLD_STYLE_PADDING_Y;

    GetTextMetrics(hdc, &fontInfo);
    fontHeight = fontInfo.tmHeight;

    /* Minimum width for horizontal button */
    minButtonWidth = GetTextWidth(hdc, "Cancel", 6);

    /* Maximum width of a dialog, if possible */
    GetWindowRect(s_hwnd, &rect);
    maxDialogWidth = rect.right - rect.left
		     - GetSystemMetrics(SM_CXFRAME) * 2;
    if (maxDialogWidth < DLG_MIN_MAX_WIDTH)
	maxDialogWidth = DLG_MIN_MAX_WIDTH;

    /* Set dlgwidth to width of message */
    pstart = message;
    messageWidth = 0;
    msgheight = 0;
    do
    {
	pend = vim_strchr(pstart, DLG_BUTTON_SEP);
	if (pend == NULL)
	    pend = pstart + STRLEN(pstart);	/* Last line of message. */
	msgheight += fontHeight;
	textWidth = GetTextWidth(hdc, pstart, pend - pstart);
	if (textWidth > messageWidth)
	    messageWidth = textWidth;
	pstart = pend + 1;
    } while (*pend != NUL);
    dlgwidth = messageWidth;

    /* Add width of icon to dlgwidth, and some space */
    dlgwidth += DLG_ICON_WIDTH + 3 * dlgPaddingX;

    if (msgheight < DLG_ICON_HEIGHT)
	msgheight = DLG_ICON_HEIGHT;

    /*
     * Check button names.  A long one will make the dialog wider.
     */
	 vertical = (vim_strchr(p_go, GO_VERTICAL) != NULL);
    if (!vertical)
    {
	// Place buttons horizontally if they fit.
	horizWidth = dlgPaddingX;
	pstart = tbuffer;
	i = 0;
	do
	{
	    pend = vim_strchr(pstart, DLG_BUTTON_SEP);
	    if (pend == NULL)
		pend = pstart + STRLEN(pstart);	// Last button name.
	    textWidth = GetTextWidth(hdc, pstart, pend - pstart);
	    if (textWidth < minButtonWidth)
		textWidth = minButtonWidth;
	    textWidth += dlgPaddingX;	    /* Padding within button */
	    buttonWidths[i] = textWidth;
	    buttonPositions[i++] = horizWidth;
	    horizWidth += textWidth + dlgPaddingX; /* Pad between buttons */
	    pstart = pend + 1;
	} while (*pend != NUL);

	if (horizWidth > maxDialogWidth)
	    vertical = TRUE;	// Too wide to fit on the screen.
	else if (horizWidth > dlgwidth)
	    dlgwidth = horizWidth;
    }

    if (vertical)
    {
	// Stack buttons vertically.
	pstart = tbuffer;
	do
	{
	    pend = vim_strchr(pstart, DLG_BUTTON_SEP);
	    if (pend == NULL)
		pend = pstart + STRLEN(pstart);	// Last button name.
	    textWidth = GetTextWidth(hdc, pstart, pend - pstart);
	    textWidth += dlgPaddingX;		/* Padding within button */
	    textWidth += DLG_VERT_PADDING_X * 2; /* Padding around button */
	    if (textWidth > dlgwidth)
		dlgwidth = textWidth;
	    pstart = pend + 1;
	} while (*pend != NUL);
    }

    if (dlgwidth < DLG_MIN_WIDTH)
	dlgwidth = DLG_MIN_WIDTH;	/* Don't allow a really thin dialog!*/

    /* start to fill in the dlgtemplate information.  addressing by WORDs */
    lStyle = DS_MODALFRAME | WS_CAPTION | WS_VISIBLE ;

    add_long(lStyle);
    pnumitems = p;	/*save where the number of items must be stored*/
    add_byte(0);	// NumberOfItems(will change later)
    add_word(10);	// x
    add_word(10);	// y
    add_word(PixelToDialogX(dlgwidth));

    // Dialog height.
    if (vertical)
	dlgheight = msgheight + 2 * dlgPaddingY +
			      DLG_VERT_PADDING_Y + 2 * fontHeight * numButtons;
    else
	dlgheight = msgheight + 3 * dlgPaddingY + 2 * fontHeight;

    // Dialog needs to be taller if contains an edit box.
    editboxheight = fontHeight + dlgPaddingY + 4 * DLG_VERT_PADDING_Y;
    if (textfield != NULL)
	dlgheight += editboxheight;

    add_word(PixelToDialogY(dlgheight));

    add_byte(0);	//menu
    add_byte(0);	//class

    /* copy the title of the dialog */
    add_string(title ? title : ("Vim"VIM_VERSION_MEDIUM));

    buttonYpos = msgheight + 2 * dlgPaddingY;

    if (textfield != NULL)
	buttonYpos += editboxheight;

    pstart = tbuffer; //dflt_text
    horizWidth = (dlgwidth - horizWidth) / 2;	/* Now it's X offset */
    for (i = 0; i < numButtons; i++)
    {
	/* get end of this button. */
	for (	pend = pstart;
		*pend && (*pend != DLG_BUTTON_SEP);
		pend++)
	    ;

	if (*pend)
	    *pend = '\0';

	/*
	 * NOTE:
	 * setting the BS_DEFPUSHBUTTON style doesn't work because Windows sets
	 * the focus to the first tab-able button and in so doing makes that
	 * the default!! Grrr.  Workaround: Make the default button the only
	 * one with WS_TABSTOP style. Means user can't tab between buttons, but
	 * he/she can use arrow keys.
	 *
	 * NOTE (Thore): Setting BS_DEFPUSHBUTTON works fine when it's the
	 * first one, so I changed the correct button to be this style. This
	 * is necessary because when an edit box is added, we need a button to
	 * be default.  The edit box will be the default control, and when the
	 * user presses enter from the edit box we want the default button to
	 * be pressed.
	 */
	if (vertical)
	{
	    p = add_dialog_element(p,
		    ((i == dfltbutton || dfltbutton < 0) && textfield != NULL
			    ?  BS_DEFPUSHBUTTON : BS_PUSHBUTTON) | WS_TABSTOP,
		    PixelToDialogX(DLG_VERT_PADDING_X),
		    PixelToDialogY(buttonYpos /* TBK */
				   + 2 * fontHeight * i),
		    PixelToDialogX(dlgwidth - 2 * DLG_VERT_PADDING_X),
		    (WORD)(PixelToDialogY(2 * fontHeight) - 1),
		    (WORD)(IDCANCEL + 1 + i), (BYTE)0x80, pstart);
	}
	else
	{
	    p = add_dialog_element(p,
		    ((i == dfltbutton || dfltbutton < 0) && textfield != NULL
			     ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON) | WS_TABSTOP,
		    PixelToDialogX(horizWidth + buttonPositions[i]),
		    PixelToDialogY(buttonYpos), /* TBK */
		    PixelToDialogX(buttonWidths[i]),
		    (WORD)(PixelToDialogY(2 * fontHeight) - 1),
		    (WORD)(IDCANCEL + 1 + i), (BYTE)0x80, pstart);
	}

	pstart = pend + 1;	/*next button*/

    }
    *pnumitems += numButtons;

    /* Vim icon */
    p = add_dialog_element(p, SS_ICON,
	    PixelToDialogX(dlgPaddingX),
	    PixelToDialogY(dlgPaddingY),
	    PixelToDialogX(DLG_ICON_WIDTH),
	    PixelToDialogY(DLG_ICON_HEIGHT),
	    DLG_NONBUTTON_CONTROL + 0, (BYTE)0x82,
	    &dlg_icons[type]);


    /* Dialog message */
    p = add_dialog_element(p, SS_LEFT,
	    PixelToDialogX(2 * dlgPaddingX + DLG_ICON_WIDTH),
	    PixelToDialogY(dlgPaddingY),
	    (WORD)(PixelToDialogX(messageWidth) + 1),
	    PixelToDialogY(msgheight),
	    DLG_NONBUTTON_CONTROL + 1, (BYTE)0x82, message);

    /* Edit box */
    if (textfield != NULL)
    {
	p = add_dialog_element(p, ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP | WS_BORDER,
		PixelToDialogX(2 * dlgPaddingX),
		PixelToDialogY(2 * dlgPaddingY + msgheight),
		PixelToDialogX(dlgwidth - 4 * dlgPaddingX),
		PixelToDialogY(fontHeight + dlgPaddingY),
		DLG_NONBUTTON_CONTROL + 2, (BYTE)0x81, textfield);
	*pnumitems += 1;
    }

    *pnumitems += 2;

    SelectFont(hdc, oldFont);
    ReleaseDC(hwnd, hdc);
    dp = MakeProcInstance((FARPROC)dialog_callback, s_hinst);


    /* Let the dialog_callback() function know which button to make default
     * If we have an edit box, make that the default. We also need to tell
     * dialog_callback() if this dialog contains an edit box or not. We do
     * this by setting s_textfield if it does.
     */
    if (textfield != NULL)
    {
	dialog_default_button = DLG_NONBUTTON_CONTROL + 2;
	s_textfield = textfield;
    }
    else
    {
	dialog_default_button = IDCANCEL + 1 + dfltbutton;
	s_textfield = NULL;
    }

    /*show the dialog box modally and get a return value*/
    nchar = DialogBoxIndirect(
	    s_hinst,
	    (HGLOBAL) hglbDlgTemp,
	    s_hwnd,
	    (DLGPROC)dp);

    FreeProcInstance( dp );
    GlobalUnlock(hglbDlgTemp);
    GlobalFree(hglbDlgTemp);
    vim_free(tbuffer);
    vim_free(buttonWidths);
    vim_free(buttonPositions);


    return nchar;
}

/*
 * Put a simple element (basic class) onto a dialog template in memory.
 * return a pointer to where the next item should be added.
 *
 * parameters:
 *  lStyle = additional style flags
 *  x,y = x & y positions IN DIALOG UNITS
 *  w,h = width and height IN DIALOG UNITS
 *  Id	= ID used in messages
 *  clss  = class ID, e.g 0x80 for a button, 0x82 for a static
 *  caption = usually text or resource name
 *
 *  TODO: use the length information noted here to enable the dialog creation
 *  routines to work out more exactly how much memory they need to alloc.
 */
    static LPWORD
add_dialog_element(
    LPWORD p,
    DWORD lStyle,
    WORD x,
    WORD y,
    WORD w,
    WORD h,
    WORD Id,
    BYTE clss,
    const char *caption)
{

    lStyle = lStyle | WS_VISIBLE | WS_CHILD;

    add_word(x);
    add_word(y);
    add_word(w);
    add_word(h);
    add_word(Id);
    add_long(lStyle);
    add_byte(clss);
    if (((lStyle & SS_ICON) != 0) && (clss == 0x82))
    {
	/* Use resource ID */
	add_byte(0xff);
	add_byte(*caption);
    }
    else
	add_string(caption);

    add_byte(0);    //# of extra bytes following


    return p;
}

#undef add_byte
#undef add_string
#undef add_long
#undef add_word

#endif /* FEAT_GUI_DIALOG */

    static void
get_dialog_font_metrics(void)
{
    DWORD	    dlgFontSize;
	dlgFontSize = GetDialogBaseUnits();	/* fall back to big old system*/
	s_dlgfntwidth = LOWORD(dlgFontSize);
	s_dlgfntheight = HIWORD(dlgFontSize);
}


#if defined(FEAT_TOOLBAR) || defined(PROTO)
#include "gui_w3~1.h"
/*
 * Create the toolbar, initially unpopulated.
 *  (just like the menu, there are no defaults, it's all
 *  set up through menu.vim)
 */
    static void
initialise_toolbar(void)
{
    s_toolbarhwnd = CreateToolbar(
		    s_hwnd,
		    WS_CHILD | WS_VISIBLE,
		    CMD_TB_BASE, /*<vn>*/
		    31,			//number of images in initial bitmap
		    s_hinst,
		    IDR_TOOLBAR1,	// id of initial bitmap
		    NULL,
		    0			// initial number of buttons
		    );

    gui_mch_show_toolbar(vim_strchr(p_go, GO_TOOLBAR) != NULL);
}
#endif

#if defined(FEAT_OLE) || defined(FEAT_EVAL) || defined(PROTO)
/*
 * Make the GUI window come to the foreground.
 */
    void
gui_mch_set_foreground(void)
{
    if (IsIconic(s_hwnd))
	 SendMessage(s_hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    SetActiveWindow(s_hwnd);
}
#endif
