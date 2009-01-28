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
 * Windows GUI.
 *
 * GUI support for Microsoft Windows.  Win32 initially; maybe Win16 later
 *
 * George V. Reilly <george@reilly.org> wrote the original Win32 GUI.
 * Robert Webb reworked it to use the existing GUI stuff and added menu,
 * scrollbars, etc.
 *
 * Note: Clipboard stuff, for cutting and pasting text to other windows, is in
 * os_win32.c.	(It can also be done from the terminal version).
 *
 * TODO: Some of the function signatures ought to be updated for Win64;
 * e.g., replace LONG with LONG_PTR, etc.
 */

#include "vim.h"

/*
 * These are new in Windows ME/XP, only defined in recent compilers.
 */
#ifndef HANDLE_WM_XBUTTONUP
# define HANDLE_WM_XBUTTONUP(hwnd, wParam, lParam, fn) \
   ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
#endif
#ifndef HANDLE_WM_XBUTTONDOWN
# define HANDLE_WM_XBUTTONDOWN(hwnd, wParam, lParam, fn) \
   ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
#endif
#ifndef HANDLE_WM_XBUTTONDBLCLK
# define HANDLE_WM_XBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
   ((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
#endif

/*
 * Include the common stuff for MS-Windows GUI.
 */
#include "gui_w48.c"

#ifdef FEAT_XPM_W32
# include "xpm_w32.h"
#endif

#ifdef PROTO
# define WINAPI
#endif

#ifdef __MINGW32__
/*
 * Add a lot of missing defines.
 * They are not always missing, we need the #ifndef's.
 */
# ifndef _cdecl
#  define _cdecl
# endif
# ifndef IsMinimized
#  define     IsMinimized(hwnd)		IsIconic(hwnd)
# endif
# ifndef IsMaximized
#  define     IsMaximized(hwnd)		IsZoomed(hwnd)
# endif
# ifndef SelectFont
#  define     SelectFont(hdc, hfont)  ((HFONT)SelectObject((hdc), (HGDIOBJ)(HFONT)(hfont)))
# endif
# ifndef GetStockBrush
#  define     GetStockBrush(i)     ((HBRUSH)GetStockObject(i))
# endif
# ifndef DeleteBrush
#  define     DeleteBrush(hbr)     DeleteObject((HGDIOBJ)(HBRUSH)(hbr))
# endif

# ifndef HANDLE_WM_RBUTTONDBLCLK
#  define HANDLE_WM_RBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_MBUTTONUP
#  define HANDLE_WM_MBUTTONUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_MBUTTONDBLCLK
#  define HANDLE_WM_MBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_LBUTTONDBLCLK
#  define HANDLE_WM_LBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_RBUTTONDOWN
#  define HANDLE_WM_RBUTTONDOWN(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_MOUSEMOVE
#  define HANDLE_WM_MOUSEMOVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_RBUTTONUP
#  define HANDLE_WM_RBUTTONUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_MBUTTONDOWN
#  define HANDLE_WM_MBUTTONDOWN(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_LBUTTONUP
#  define HANDLE_WM_LBUTTONUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_LBUTTONDOWN
#  define HANDLE_WM_LBUTTONDOWN(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_SYSCHAR
#  define HANDLE_WM_SYSCHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)
# endif
# ifndef HANDLE_WM_ACTIVATEAPP
#  define HANDLE_WM_ACTIVATEAPP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (DWORD)(lParam)), 0L)
# endif
# ifndef HANDLE_WM_WINDOWPOSCHANGING
#  define HANDLE_WM_WINDOWPOSCHANGING(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (LPWINDOWPOS)(lParam))
# endif
# ifndef HANDLE_WM_VSCROLL
#  define HANDLE_WM_VSCROLL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(lParam), (UINT)(LOWORD(wParam)),  (int)(short)HIWORD(wParam)), 0L)
# endif
# ifndef HANDLE_WM_SETFOCUS
#  define HANDLE_WM_SETFOCUS(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_KILLFOCUS
#  define HANDLE_WM_KILLFOCUS(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_HSCROLL
#  define HANDLE_WM_HSCROLL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(lParam), (UINT)(LOWORD(wParam)), (int)(short)HIWORD(wParam)), 0L)
# endif
# ifndef HANDLE_WM_DROPFILES
#  define HANDLE_WM_DROPFILES(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HDROP)(wParam)), 0L)
# endif
# ifndef HANDLE_WM_CHAR
#  define HANDLE_WM_CHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)
# endif
# ifndef HANDLE_WM_SYSDEADCHAR
#  define HANDLE_WM_SYSDEADCHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)
# endif
# ifndef HANDLE_WM_DEADCHAR
#  define HANDLE_WM_DEADCHAR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)
# endif
#endif /* __MINGW32__ */


/* Some parameters for tearoff menus.  All in pixels. */
#define TEAROFF_PADDING_X	2
#define TEAROFF_BUTTON_PAD_X	8
#define TEAROFF_MIN_WIDTH	200
#define TEAROFF_SUBMENU_LABEL	">>"
#define TEAROFF_COLUMN_PADDING	3	// # spaces to pad column with.


/* For the Intellimouse: */
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x20a
#endif


#ifdef FEAT_BEVAL
# define ID_BEVAL_TOOLTIP   200
# define BEVAL_TEXT_LEN	    MAXPATHL

#if _MSC_VER < 1300
/* Work around old versions of basetsd.h which wrongly declares
 * UINT_PTR as unsigned long. */
# define UINT_PTR UINT
#endif

static void make_tooltip __ARGS((BalloonEval *beval, char *text, POINT pt));
static void delete_tooltip __ARGS((BalloonEval *beval));
static VOID CALLBACK BevalTimerProc __ARGS((HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime));

static BalloonEval  *cur_beval = NULL;
static UINT_PTR	    BevalTimerId = 0;
static DWORD	    LastActivity = 0;

/*
 * excerpts from headers since this may not be presented
 * in the extremely old compilers
 */
#include <pshpack1.h>

typedef struct _DllVersionInfo
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;

typedef struct tagTOOLINFOA_NEW
{
	UINT cbSize;
	UINT uFlags;
	HWND hwnd;
	UINT uId;
	RECT rect;
	HINSTANCE hinst;
	LPSTR lpszText;
	LPARAM lParam;
} TOOLINFO_NEW;

typedef struct tagNMTTDISPINFO_NEW
{
    NMHDR      hdr;
    LPTSTR     lpszText;
    char       szText[80];
    HINSTANCE  hinst;
    UINT       uFlags;
    LPARAM     lParam;
} NMTTDISPINFO_NEW;

#include <poppack.h>

typedef HRESULT (WINAPI* DLLGETVERSIONPROC)(DLLVERSIONINFO *);
#ifndef TTM_SETMAXTIPWIDTH
# define TTM_SETMAXTIPWIDTH	(WM_USER+24)
#endif

#ifndef TTF_DI_SETITEM
# define TTF_DI_SETITEM		0x8000
#endif

#ifndef TTN_GETDISPINFO
# define TTN_GETDISPINFO	(TTN_FIRST - 0)
#endif

#endif /* defined(FEAT_BEVAL) */

#if defined(FEAT_TOOLBAR) || defined(FEAT_GUI_TABLINE)
/* Older MSVC compilers don't have LPNMTTDISPINFO[AW] thus we need to define
 * it here if LPNMTTDISPINFO isn't defined.
 * MingW doesn't define LPNMTTDISPINFO but typedefs it.  Thus we need to check
 * _MSC_VER. */
# if !defined(LPNMTTDISPINFO) && defined(_MSC_VER)
typedef struct tagNMTTDISPINFOA {
    NMHDR	hdr;
    LPSTR	lpszText;
    char	szText[80];
    HINSTANCE	hinst;
    UINT	uFlags;
    LPARAM	lParam;
} NMTTDISPINFOA, *LPNMTTDISPINFOA;
#  define LPNMTTDISPINFO LPNMTTDISPINFOA

#  ifdef FEAT_MBYTE
typedef struct tagNMTTDISPINFOW {
    NMHDR	hdr;
    LPWSTR	lpszText;
    WCHAR	szText[80];
    HINSTANCE	hinst;
    UINT	uFlags;
    LPARAM	lParam;
} NMTTDISPINFOW, *LPNMTTDISPINFOW;
#  endif
# endif
#endif

#ifndef TTN_GETDISPINFOW
# define TTN_GETDISPINFOW	(TTN_FIRST - 10)
#endif

/* Local variables: */

#ifdef FEAT_MENU
static UINT	s_menu_id = 100;

/*
 * Use the system font for dialogs and tear-off menus.  Remove this line to
 * use DLG_FONT_NAME.
 */
# define USE_SYSMENU_FONT
#endif

#define VIM_NAME	"vim"
#define VIM_CLASS	"Vim"
#define VIM_CLASSW	L"Vim"

/* Initial size for the dialog template.  For gui_mch_dialog() it's fixed,
 * thus there should be room for every dialog.  For tearoffs it's made bigger
 * when needed. */
#define DLG_ALLOC_SIZE 16 * 1024

/*
 * stuff for dialogs, menus, tearoffs etc.
 */
static LRESULT APIENTRY dialog_callback(HWND, UINT, WPARAM, LPARAM);
static LRESULT APIENTRY tearoff_callback(HWND, UINT, WPARAM, LPARAM);
static PWORD
add_dialog_element(
	PWORD p,
	DWORD lStyle,
	WORD x,
	WORD y,
	WORD w,
	WORD h,
	WORD Id,
	WORD clss,
	const char *caption);
static LPWORD lpwAlign(LPWORD);
static int nCopyAnsiToWideChar(LPWORD, LPSTR);
static void gui_mch_tearoff(char_u *title, vimmenu_T *menu, int initX, int initY);
static void get_dialog_font_metrics(void);

static int dialog_default_button = -1;

/* Intellimouse support */
static int mouse_scroll_lines = 0;
static UINT msh_msgmousewheel = 0;

static int	s_usenewlook;	    /* emulate W95/NT4 non-bold dialogs */
#ifdef FEAT_TOOLBAR
static void initialise_toolbar(void);
static int get_toolbar_bitmap(vimmenu_T *menu);
#endif

#ifdef FEAT_GUI_TABLINE
static void initialise_tabline(void);
#endif

#ifdef FEAT_MBYTE_IME
static LRESULT _OnImeComposition(HWND hwnd, WPARAM dbcs, LPARAM param);
static char_u *GetResultStr(HWND hwnd, int GCS, int *lenp);
#endif
#if defined(FEAT_MBYTE_IME) && defined(DYNAMIC_IME)
# ifdef NOIME
typedef struct tagCOMPOSITIONFORM {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} COMPOSITIONFORM, *PCOMPOSITIONFORM, NEAR *NPCOMPOSITIONFORM, FAR *LPCOMPOSITIONFORM;
typedef HANDLE HIMC;
# endif

static HINSTANCE hLibImm = NULL;
static LONG (WINAPI *pImmGetCompositionStringA)(HIMC, DWORD, LPVOID, DWORD);
static LONG (WINAPI *pImmGetCompositionStringW)(HIMC, DWORD, LPVOID, DWORD);
static HIMC (WINAPI *pImmGetContext)(HWND);
static HIMC (WINAPI *pImmAssociateContext)(HWND, HIMC);
static BOOL (WINAPI *pImmReleaseContext)(HWND, HIMC);
static BOOL (WINAPI *pImmGetOpenStatus)(HIMC);
static BOOL (WINAPI *pImmSetOpenStatus)(HIMC, BOOL);
static BOOL (WINAPI *pImmGetCompositionFont)(HIMC, LPLOGFONTA);
static BOOL (WINAPI *pImmSetCompositionFont)(HIMC, LPLOGFONTA);
static BOOL (WINAPI *pImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
static BOOL (WINAPI *pImmGetConversionStatus)(HIMC, LPDWORD, LPDWORD);
static BOOL (WINAPI *pImmSetConversionStatus)(HIMC, DWORD, DWORD);
static void dyn_imm_load(void);
#else
# define pImmGetCompositionStringA ImmGetCompositionStringA
# define pImmGetCompositionStringW ImmGetCompositionStringW
# define pImmGetContext		  ImmGetContext
# define pImmAssociateContext	  ImmAssociateContext
# define pImmReleaseContext	  ImmReleaseContext
# define pImmGetOpenStatus	  ImmGetOpenStatus
# define pImmSetOpenStatus	  ImmSetOpenStatus
# define pImmGetCompositionFont   ImmGetCompositionFontA
# define pImmSetCompositionFont   ImmSetCompositionFontA
# define pImmSetCompositionWindow ImmSetCompositionWindow
# define pImmGetConversionStatus  ImmGetConversionStatus
# define pImmSetConversionStatus  ImmSetConversionStatus
#endif

#ifndef ETO_IGNORELANGUAGE
# define ETO_IGNORELANGUAGE  0x1000
#endif

/* multi monitor support */
typedef struct _MONITORINFOstruct
{
    DWORD cbSize;
    RECT rcMonitor;
    RECT rcWork;
    DWORD dwFlags;
} _MONITORINFO;

typedef HANDLE _HMONITOR;
typedef _HMONITOR (WINAPI *TMonitorFromWindow)(HWND, DWORD);
typedef BOOL (WINAPI *TGetMonitorInfo)(_HMONITOR, _MONITORINFO *);

static TMonitorFromWindow   pMonitorFromWindow = NULL;
static TGetMonitorInfo	    pGetMonitorInfo = NULL;
static HANDLE		    user32_lib = NULL;
#ifdef FEAT_NETBEANS_INTG
int WSInitialized = FALSE; /* WinSock is initialized */
#endif
/*
 * Return TRUE when running under Windows NT 3.x or Win32s, both of which have
 * less fancy GUI APIs.
 */
    static int
is_winnt_3(void)
{
    return ((os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
		&& os_version.dwMajorVersion == 3)
	    || (os_version.dwPlatformId == VER_PLATFORM_WIN32s));
}

/*
 * Return TRUE when running under Win32s.
 */
    int
gui_is_win32s(void)
{
    return (os_version.dwPlatformId == VER_PLATFORM_WIN32s);
}

#ifdef FEAT_MENU
/*
 * Figure out how high the menu bar is at the moment.
 */
    static int
gui_mswin_get_menu_height(
    int	    fix_window)	    /* If TRUE, resize window if menu height changed */
{
    static int	old_menu_height = -1;

    RECT    rc1, rc2;
    int	    num;
    int	    menu_height;

    if (gui.menu_is_active)
	num = GetMenuItemCount(s_menuBar);
    else
	num = 0;

    if (num == 0)
	menu_height = 0;
    else
    {
	if (is_winnt_3())	/* for NT 3.xx */
	{
	    if (gui.starting)
		menu_height = GetSystemMetrics(SM_CYMENU);
	    else
	    {
		RECT r1, r2;
		int frameht = GetSystemMetrics(SM_CYFRAME);
		int capht = GetSystemMetrics(SM_CYCAPTION);

		/* get window rect of s_hwnd
		 * get client rect of s_hwnd
		 * get cap height
		 * subtract from window rect, the sum of client height,
		 * (if not maximized)frame thickness, and caption height.
		 */
		GetWindowRect(s_hwnd, &r1);
		GetClientRect(s_hwnd, &r2);
		menu_height = r1.bottom - r1.top - (r2.bottom - r2.top
				 + 2 * frameht * (!IsZoomed(s_hwnd)) + capht);
	    }
	}
	else			/* win95 and variants (NT 4.0, I guess) */
	{
	    /*
	     * In case 'lines' is set in _vimrc/_gvimrc window width doesn't
	     * seem to have been set yet, so menu wraps in default window
	     * width which is very narrow.  Instead just return height of a
	     * single menu item.  Will still be wrong when the menu really
	     * should wrap over more than one line.
	     */
	    GetMenuItemRect(s_hwnd, s_menuBar, 0, &rc1);
	    if (gui.starting)
		menu_height = rc1.bottom - rc1.top + 1;
	    else
	    {
		GetMenuItemRect(s_hwnd, s_menuBar, num - 1, &rc2);
		menu_height = rc2.bottom - rc1.top + 1;
	    }
	}
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
 * Setup for the Intellimouse
 */
    static void
init_mouse_wheel(void)
{

#ifndef SPI_GETWHEELSCROLLLINES
# define SPI_GETWHEELSCROLLLINES    104
#endif
#ifndef SPI_SETWHEELSCROLLLINES
# define SPI_SETWHEELSCROLLLINES    105
#endif

#define VMOUSEZ_CLASSNAME  "MouseZ"		/* hidden wheel window class */
#define VMOUSEZ_TITLE      "Magellan MSWHEEL"	/* hidden wheel window title */
#define VMSH_MOUSEWHEEL    "MSWHEEL_ROLLMSG"
#define VMSH_SCROLL_LINES  "MSH_SCROLL_LINES_MSG"

    HWND hdl_mswheel;
    UINT msh_msgscrolllines;

    msh_msgmousewheel = 0;
    mouse_scroll_lines = 3;	/* reasonable default */

    if ((os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
		&& os_version.dwMajorVersion >= 4)
	    || (os_version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
		&& ((os_version.dwMajorVersion == 4
			&& os_version.dwMinorVersion >= 10)
		    || os_version.dwMajorVersion >= 5)))
    {
	/* if NT 4.0+ (or Win98) get scroll lines directly from system */
	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
		&mouse_scroll_lines, 0);
    }
    else if (os_version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
	    || (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
		&& os_version.dwMajorVersion < 4))
    {	/*
	 * If Win95 or NT 3.51,
	 * try to find the hidden point32 window.
	 */
	hdl_mswheel = FindWindow(VMOUSEZ_CLASSNAME, VMOUSEZ_TITLE);
	if (hdl_mswheel)
	{
	    msh_msgscrolllines = RegisterWindowMessage(VMSH_SCROLL_LINES);
	    if (msh_msgscrolllines)
	    {
		mouse_scroll_lines = (int)SendMessage(hdl_mswheel,
			msh_msgscrolllines, 0, 0);
		msh_msgmousewheel  = RegisterWindowMessage(VMSH_MOUSEWHEEL);
	    }
	}
    }
}


/* Intellimouse wheel handler */
    static void
_OnMouseWheel(
    HWND hwnd,
    short zDelta)
{
/* Treat a mouse wheel event as if it were a scroll request */
    int i;
    int size;
    HWND hwndCtl;

    if (curwin->w_scrollbars[SBAR_RIGHT].id != 0)
    {
	hwndCtl = curwin->w_scrollbars[SBAR_RIGHT].id;
	size = curwin->w_scrollbars[SBAR_RIGHT].size;
    }
    else if (curwin->w_scrollbars[SBAR_LEFT].id != 0)
    {
	hwndCtl = curwin->w_scrollbars[SBAR_LEFT].id;
	size = curwin->w_scrollbars[SBAR_LEFT].size;
    }
    else
	return;

    size = curwin->w_height;
    if (mouse_scroll_lines == 0)
	init_mouse_wheel();

    if (mouse_scroll_lines > 0
	    && mouse_scroll_lines < (size > 2 ? size - 2 : 1))
    {
	for (i = mouse_scroll_lines; i > 0; --i)
	    _OnScroll(hwnd, hwndCtl, zDelta >= 0 ? SB_LINEUP : SB_LINEDOWN, 0);
    }
    else
	_OnScroll(hwnd, hwndCtl, zDelta >= 0 ? SB_PAGEUP : SB_PAGEDOWN, 0);
}

#ifdef USE_SYSMENU_FONT
/*
 * Get Menu Font.
 * Return OK or FAIL.
 */
    static int
gui_w32_get_menu_font(LOGFONT *lf)
{
    NONCLIENTMETRICS nm;

    nm.cbSize = sizeof(NONCLIENTMETRICS);
    if (!SystemParametersInfo(
	    SPI_GETNONCLIENTMETRICS,
	    sizeof(NONCLIENTMETRICS),
	    &nm,
	    0))
	return FAIL;
    *lf = nm.lfMenuFont;
    return OK;
}
#endif


#if defined(FEAT_GUI_TABLINE) && defined(USE_SYSMENU_FONT)
/*
 * Set the GUI tabline font to the system menu font
 */
    static void
set_tabline_font(void)
{
    LOGFONT	lfSysmenu;
    HFONT	font;
    HWND	hwnd;
    HDC		hdc;
    HFONT	hfntOld;
    TEXTMETRIC	tm;

    if (gui_w32_get_menu_font(&lfSysmenu) != OK)
	return;

    font = CreateFontIndirect(&lfSysmenu);

    SendMessage(s_tabhwnd, WM_SETFONT, (WPARAM)font, TRUE);

    /*
     * Compute the height of the font used for the tab text
     */
    hwnd = GetDesktopWindow();
    hdc = GetWindowDC(hwnd);
    hfntOld = SelectFont(hdc, font);

    GetTextMetrics(hdc, &tm);

    SelectFont(hdc, hfntOld);
    ReleaseDC(hwnd, hdc);

    /*
     * The space used by the tab border and the space between the tab label
     * and the tab border is included as 7.
     */
    gui.tabline_height = tm.tmHeight + tm.tmInternalLeading + 7;
}
#endif

/*
 * Invoked when a setting was changed.
 */
    static LRESULT CALLBACK
_OnSettingChange(UINT n)
{
    if (n == SPI_SETWHEELSCROLLLINES)
	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
		&mouse_scroll_lines, 0);
#if defined(FEAT_GUI_TABLINE) && defined(USE_SYSMENU_FONT)
    if (n == SPI_SETNONCLIENTMETRICS)
	set_tabline_font();
#endif
    return 0;
}

#if 0	/* disabled, a gap appears below and beside the window, and the window
	   can be moved (in a strange way) */
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
    RECT    workarea_rect;

    if (!(lpwpos->flags & SWP_NOSIZE))
    {
	if (IsMaximized(hwnd)
		&& (os_version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
		    || (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
			&& os_version.dwMajorVersion >= 4)))
	{
	    SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea_rect, 0);
	    lpwpos->x = workarea_rect.left;
	    lpwpos->y = workarea_rect.top;
	    lpwpos->cx = workarea_rect.right - workarea_rect.left;
	    lpwpos->cy = workarea_rect.bottom - workarea_rect.top;
	}
	gui_mswin_get_valid_dimensions(lpwpos->cx, lpwpos->cy,
				     &lpwpos->cx, &lpwpos->cy);
    }
    return 0;
}
#endif

#ifdef FEAT_NETBEANS_INTG
    static void
_OnWindowPosChanged(
    HWND hwnd,
    const LPWINDOWPOS lpwpos)
{
    static int x = 0, y = 0, cx = 0, cy = 0;

    if (WSInitialized && (lpwpos->x != x || lpwpos->y != y
				     || lpwpos->cx != cx || lpwpos->cy != cy))
    {
	x = lpwpos->x;
	y = lpwpos->y;
	cx = lpwpos->cx;
	cy = lpwpos->cy;
	netbeans_frame_moved(x, y);
    }
    /* Allow to send WM_SIZE and WM_MOVE */
    FORWARD_WM_WINDOWPOSCHANGED(hwnd, lpwpos, MyWindowProc);
}
#endif

    static int
_DuringSizing(
    UINT fwSide,
    LPRECT lprc)
{
    int	    w, h;
    int	    valid_w, valid_h;
    int	    w_offset, h_offset;

    w = lprc->right - lprc->left;
    h = lprc->bottom - lprc->top;
    gui_mswin_get_valid_dimensions(w, h, &valid_w, &valid_h);
    w_offset = w - valid_w;
    h_offset = h - valid_h;

    if (fwSide == WMSZ_LEFT || fwSide == WMSZ_TOPLEFT
			    || fwSide == WMSZ_BOTTOMLEFT)
	lprc->left += w_offset;
    else if (fwSide == WMSZ_RIGHT || fwSide == WMSZ_TOPRIGHT
			    || fwSide == WMSZ_BOTTOMRIGHT)
	lprc->right -= w_offset;

    if (fwSide == WMSZ_TOP || fwSide == WMSZ_TOPLEFT
			    || fwSide == WMSZ_TOPRIGHT)
	lprc->top += h_offset;
    else if (fwSide == WMSZ_BOTTOM || fwSide == WMSZ_BOTTOMLEFT
			    || fwSide == WMSZ_BOTTOMRIGHT)
	lprc->bottom -= h_offset;
    return TRUE;
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
	// HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING,	_OnWindowPosChanging);
	HANDLE_MSG(hwnd, WM_ACTIVATEAPP, _OnActivateApp);
#ifdef FEAT_NETBEANS_INTG
	HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, _OnWindowPosChanged);
#endif

#ifdef FEAT_GUI_TABLINE
	case WM_RBUTTONUP:
	{
	    if (gui_mch_showing_tabline())
	    {
		POINT pt;
		RECT rect;

		/*
		 * If the cursor is on the tabline, display the tab menu
		 */
		GetCursorPos((LPPOINT)&pt);
		GetWindowRect(s_textArea, &rect);
		if (pt.y < rect.top)
		{
		    show_tabline_popup_menu();
		    return 0;
		}
	    }
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
	}
	case WM_LBUTTONDBLCLK:
	{
	    /*
	     * If the user double clicked the tabline, create a new tab
	     */
	    if (gui_mch_showing_tabline())
	    {
		POINT pt;
		RECT rect;

		GetCursorPos((LPPOINT)&pt);
		GetWindowRect(s_textArea, &rect);
		if (pt.y < rect.top)
		    send_tabline_menu_event(0, TABLINE_MENU_NEW);
	    }
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
	}
#endif

    case WM_QUERYENDSESSION:	/* System wants to go down. */
	gui_shell_closed();	/* Will exit when no changed buffers. */
	return FALSE;		/* Do NOT allow system to go down. */

    case WM_ENDSESSION:
	if (wParam)	/* system only really goes down when wParam is TRUE */
	    _OnEndSession();
	break;

    case WM_CHAR:
	/* Don't use HANDLE_MSG() for WM_CHAR, it truncates wParam to a single
	 * byte while we want the UTF-16 character value. */
	_OnChar(hwnd, (UINT)wParam, (int)(short)LOWORD(lParam));
	return 0L;

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
	{
	    _OnSysChar(hwnd, (UINT)wParam, (int)(short)LOWORD(lParam));
	    return 0L;
	}
#ifdef FEAT_MENU
	else
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
#endif

    case WM_SYSKEYUP:
#ifdef FEAT_MENU
	/* This used to be done only when menu is active: ALT key is used for
	 * that.  But that caused problems when menu is disabled and using
	 * Alt-Tab-Esc: get into a strange state where no mouse-moved events
	 * are received, mouse pointer remains hidden. */
	return MyWindowProc(hwnd, uMsg, wParam, lParam);
#else
	return 0;
#endif

    case WM_SIZING:	/* HANDLE_MSG doesn't seem to handle this one */
	return _DuringSizing((UINT)wParam, (LPRECT)lParam);

    case WM_MOUSEWHEEL:
	_OnMouseWheel(hwnd, HIWORD(wParam));
	break;

	/* Notification for change in SystemParametersInfo() */
    case WM_SETTINGCHANGE:
	return _OnSettingChange((UINT)wParam);

#if defined(FEAT_TOOLBAR) || defined(FEAT_GUI_TABLINE)
    case WM_NOTIFY:
	switch (((LPNMHDR) lParam)->code)
	{
# ifdef FEAT_MBYTE
	    case TTN_GETDISPINFOW:
# endif
	    case TTN_GETDISPINFO:
		{
		    LPNMHDR		hdr = (LPNMHDR)lParam;
		    char_u		*str = NULL;
		    static void		*tt_text = NULL;

		    vim_free(tt_text);
		    tt_text = NULL;

# ifdef FEAT_GUI_TABLINE
		    if (gui_mch_showing_tabline()
			   && hdr->hwndFrom == TabCtrl_GetToolTips(s_tabhwnd))
		    {
			POINT		pt;
			/*
			 * Mouse is over the GUI tabline. Display the
			 * tooltip for the tab under the cursor
			 *
			 * Get the cursor position within the tab control
			 */
			GetCursorPos(&pt);
			if (ScreenToClient(s_tabhwnd, &pt) != 0)
			{
			    TCHITTESTINFO htinfo;
			    int idx;

			    /*
			     * Get the tab under the cursor
			     */
			    htinfo.pt.x = pt.x;
			    htinfo.pt.y = pt.y;
			    idx = TabCtrl_HitTest(s_tabhwnd, &htinfo);
			    if (idx != -1)
			    {
				tabpage_T *tp;

				tp = find_tabpage(idx + 1);
				if (tp != NULL)
				{
				    get_tabline_label(tp, TRUE);
				    str = NameBuff;
				}
			    }
			}
		    }
# endif
# ifdef FEAT_TOOLBAR
#  ifdef FEAT_GUI_TABLINE
		    else
#  endif
		    {
			UINT		idButton;
			vimmenu_T	*pMenu;

			idButton = (UINT) hdr->idFrom;
			pMenu = gui_mswin_find_menu(root_menu, idButton);
			if (pMenu)
			    str = pMenu->strings[MENU_INDEX_TIP];
		    }
# endif
		    if (str != NULL)
		    {
# ifdef FEAT_MBYTE
			if (hdr->code == TTN_GETDISPINFOW)
			{
			    LPNMTTDISPINFOW	lpdi = (LPNMTTDISPINFOW)lParam;

			    /* Set the maximum width, this also enables using
			     * \n for line break. */
			    SendMessage(lpdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH,
								      0, 500);

			    tt_text = enc_to_utf16(str, NULL);
			    lpdi->lpszText = tt_text;
			    /* can't show tooltip if failed */
			}
			else
# endif
			{
			    LPNMTTDISPINFO	lpdi = (LPNMTTDISPINFO)lParam;

			    /* Set the maximum width, this also enables using
			     * \n for line break. */
			    SendMessage(lpdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH,
								      0, 500);

			    if (STRLEN(str) < sizeof(lpdi->szText)
				    || ((tt_text = vim_strsave(str)) == NULL))
				vim_strncpy(lpdi->szText, str,
						sizeof(lpdi->szText) - 1);
			    else
				lpdi->lpszText = tt_text;
			}
		    }
		}
		break;
# ifdef FEAT_GUI_TABLINE
	    case TCN_SELCHANGE:
		if (gui_mch_showing_tabline()
				  && ((LPNMHDR)lParam)->hwndFrom == s_tabhwnd)
		    send_tabline_event(TabCtrl_GetCurSel(s_tabhwnd) + 1);
		break;

	    case NM_RCLICK:
		if (gui_mch_showing_tabline()
			&& ((LPNMHDR)lParam)->hwndFrom == s_tabhwnd)
		    show_tabline_popup_menu();
		break;
# endif
	    default:
# ifdef FEAT_GUI_TABLINE
		if (gui_mch_showing_tabline()
				  && ((LPNMHDR)lParam)->hwndFrom == s_tabhwnd)
		    return MyWindowProc(hwnd, uMsg, wParam, lParam);
# endif
		break;
	}
	break;
#endif
#if defined(MENUHINTS) && defined(FEAT_MENU)
    case WM_MENUSELECT:
	if (((UINT) HIWORD(wParam)
		    & (0xffff ^ (MF_MOUSESELECT + MF_BITMAP + MF_POPUP)))
		== MF_HILITE
		&& (State & CMDLINE) == 0)
	{
	    UINT	idButton;
	    vimmenu_T	*pMenu;
	    static int	did_menu_tip = FALSE;

	    if (did_menu_tip)
	    {
		msg_clr_cmdline();
		setcursor();
		out_flush();
		did_menu_tip = FALSE;
	    }

	    idButton = (UINT)LOWORD(wParam);
	    pMenu = gui_mswin_find_menu(root_menu, idButton);
	    if (pMenu != NULL && pMenu->strings[MENU_INDEX_TIP] != 0
		    && GetMenuState(s_menuBar, pMenu->id, MF_BYCOMMAND) != -1)
	    {
		++msg_hist_off;
		msg(pMenu->strings[MENU_INDEX_TIP]);
		--msg_hist_off;
		setcursor();
		out_flush();
		did_menu_tip = TRUE;
	    }
	}
	break;
#endif
    case WM_NCHITTEST:
	{
	    LRESULT	result;
	    int		x, y;
	    int		xPos = GET_X_LPARAM(lParam);

	    result = MyWindowProc(hwnd, uMsg, wParam, lParam);
	    if (result == HTCLIENT)
	    {
#ifdef FEAT_GUI_TABLINE
		if (gui_mch_showing_tabline())
		{
		    int  yPos = GET_Y_LPARAM(lParam);
		    RECT rct;

		    /* If the cursor is on the GUI tabline, don't process this
		     * event */
		    GetWindowRect(s_textArea, &rct);
		    if (yPos < rct.top)
			return result;
		}
#endif
		gui_mch_get_winpos(&x, &y);
		xPos -= x;

		if (xPos < 48) /* <VN> TODO should use system metric? */
		    return HTBOTTOMLEFT;
		else
		    return HTBOTTOMRIGHT;
	    }
	    else
		return result;
	}
	/* break; notreached */

#ifdef FEAT_MBYTE_IME
    case WM_IME_NOTIFY:
	if (!_OnImeNotify(hwnd, (DWORD)wParam, (DWORD)lParam))
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
	break;
    case WM_IME_COMPOSITION:
	if (!_OnImeComposition(hwnd, wParam, lParam))
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
	break;
#endif

    default:
	if (uMsg == msh_msgmousewheel && msh_msgmousewheel != 0)
	{   /* handle MSH_MOUSEWHEEL messages for Intellimouse */
	    _OnMouseWheel(hwnd, HIWORD(wParam));
	    break;
	}
#ifdef MSWIN_FIND_REPLACE
	else if (uMsg == s_findrep_msg && s_findrep_msg != 0)
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

/* parent window, if specified with -P */
HWND vim_parent_hwnd = NULL;

    static BOOL CALLBACK
FindWindowTitle(HWND hwnd, LPARAM lParam)
{
    char	buf[2048];
    char	*title = (char *)lParam;

    if (GetWindowText(hwnd, buf, sizeof(buf)))
    {
	if (strstr(buf, title) != NULL)
	{
	    /* Found it.  Store the window ref. and quit searching if MDI
	     * works. */
	    vim_parent_hwnd = FindWindowEx(hwnd, NULL, "MDIClient", NULL);
	    if (vim_parent_hwnd != NULL)
		return FALSE;
	}
    }
    return TRUE;	/* continue searching */
}

/*
 * Invoked for '-P "title"' argument: search for parent application to open
 * our window in.
 */
    void
gui_mch_set_parent(char *title)
{
    EnumWindows(FindWindowTitle, (LPARAM)title);
    if (vim_parent_hwnd == NULL)
    {
	EMSG2(_("E671: Cannot find window title \"%s\""), title);
	mch_exit(2);
    }
}

#ifndef FEAT_OLE
    static void
ole_error(char *arg)
{
    char buf[IOSIZE];

    /* Can't use EMSG() here, we have not finished initialisation yet. */
    vim_snprintf(buf, IOSIZE,
	    _("E243: Argument not supported: \"-%s\"; Use the OLE version."),
	    arg);
    mch_errmsg(buf);
}
#endif

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(int *argc, char **argv)
{
    int		silent = FALSE;
    int		idx;

    /* Check for special OLE command line parameters */
    if ((*argc == 2 || *argc == 3) && (argv[1][0] == '-' || argv[1][0] == '/'))
    {
	/* Check for a "-silent" argument first. */
	if (*argc == 3 && STRICMP(argv[1] + 1, "silent") == 0
		&& (argv[2][0] == '-' || argv[2][0] == '/'))
	{
	    silent = TRUE;
	    idx = 2;
	}
	else
	    idx = 1;

	/* Register Vim as an OLE Automation server */
	if (STRICMP(argv[idx] + 1, "register") == 0)
	{
#ifdef FEAT_OLE
	    RegisterMe(silent);
	    mch_exit(0);
#else
	    if (!silent)
		ole_error("register");
	    mch_exit(2);
#endif
	}

	/* Unregister Vim as an OLE Automation server */
	if (STRICMP(argv[idx] + 1, "unregister") == 0)
	{
#ifdef FEAT_OLE
	    UnregisterMe(!silent);
	    mch_exit(0);
#else
	    if (!silent)
		ole_error("unregister");
	    mch_exit(2);
#endif
	}

	/* Ignore an -embedding argument. It is only relevant if the
	 * application wants to treat the case when it is started manually
	 * differently from the case where it is started via automation (and
	 * we don't).
	 */
	if (STRICMP(argv[idx] + 1, "embedding") == 0)
	{
#ifdef FEAT_OLE
	    *argc = 1;
#else
	    ole_error("embedding");
	    mch_exit(2);
#endif
	}
    }

#ifdef FEAT_OLE
    {
	int	bDoRestart = FALSE;

	InitOLE(&bDoRestart);
	/* automatically exit after registering */
	if (bDoRestart)
	    mch_exit(0);
    }
#endif

#ifdef FEAT_NETBEANS_INTG
    {
	/* stolen from gui_x11.x */
	int arg;

	for (arg = 1; arg < *argc; arg++)
	    if (strncmp("-nb", argv[arg], 3) == 0)
	    {
		usingNetbeans++;
		netbeansArg = argv[arg];
		mch_memmove(&argv[arg], &argv[arg + 1],
					    (--*argc - arg) * sizeof(char *));
		argv[*argc] = NULL;
		break;	/* enough? */
	    }

	if (usingNetbeans)
	{
	    WSADATA wsaData;
	    int wsaerr;

	    /* Init WinSock */
	    wsaerr = WSAStartup(MAKEWORD(2, 2), &wsaData);
	    if (wsaerr == 0)
		WSInitialized = TRUE;
	}
    }
#endif

    /* get the OS version info */
    os_version.dwOSVersionInfoSize = sizeof(os_version);
    GetVersionEx(&os_version); /* this call works on Win32s, Win95 and WinNT */

    /* try and load the user32.dll library and get the entry points for
     * multi-monitor-support. */
    if ((user32_lib = LoadLibrary("User32.dll")) != NULL)
    {
	pMonitorFromWindow = (TMonitorFromWindow)GetProcAddress(user32_lib,
							 "MonitorFromWindow");

	/* there are ...A and ...W version of GetMonitorInfo - looking at
	 * winuser.h, they have exactly the same declaration. */
	pGetMonitorInfo = (TGetMonitorInfo)GetProcAddress(user32_lib,
							  "GetMonitorInfoA");
    }
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
#ifdef FEAT_MBYTE
    const WCHAR szVimWndClassW[] = VIM_CLASSW;
    WNDCLASSW wndclassw;
#endif
#ifdef GLOBAL_IME
    ATOM	atom;
#endif

    /* Return here if the window was already opened (happens when
     * gui_mch_dialog() is called early). */
    if (s_hwnd != NULL)
	goto theend;

    /*
     * Load the tearoff bitmap
     */
#ifdef FEAT_TEAROFF
    s_htearbitmap = LoadBitmap(s_hinst, "IDB_TEAROFF");
#endif

    gui.scrollbar_width = GetSystemMetrics(SM_CXVSCROLL);
    gui.scrollbar_height = GetSystemMetrics(SM_CYHSCROLL);
#ifdef FEAT_MENU
    gui.menu_height = 0;	/* Windows takes care of this */
#endif
    gui.border_width = 0;

    s_brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

#ifdef FEAT_MBYTE
    /* First try using the wide version, so that we can use any title.
     * Otherwise only characters in the active codepage will work. */
    if (GetClassInfoW(s_hinst, szVimWndClassW, &wndclassw) == 0)
    {
	wndclassw.style = CS_DBLCLKS;
	wndclassw.lpfnWndProc = _WndProc;
	wndclassw.cbClsExtra = 0;
	wndclassw.cbWndExtra = 0;
	wndclassw.hInstance = s_hinst;
	wndclassw.hIcon = LoadIcon(wndclassw.hInstance, "IDR_VIM");
	wndclassw.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclassw.hbrBackground = s_brush;
	wndclassw.lpszMenuName = NULL;
	wndclassw.lpszClassName = szVimWndClassW;

	if ((
#ifdef GLOBAL_IME
		    atom =
#endif
		    RegisterClassW(&wndclassw)) == 0)
	{
	    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
		return FAIL;

	    /* Must be Windows 98, fall back to non-wide function. */
	}
	else
	    wide_WindowProc = TRUE;
    }

    if (!wide_WindowProc)
#endif

    if (GetClassInfo(s_hinst, szVimWndClass, &wndclass) == 0)
    {
	wndclass.style = CS_DBLCLKS;
	wndclass.lpfnWndProc = _WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = s_hinst;
	wndclass.hIcon = LoadIcon(wndclass.hInstance, "IDR_VIM");
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

    if (vim_parent_hwnd != NULL)
    {
#ifdef HAVE_TRY_EXCEPT
	__try
	{
#endif
	    /* Open inside the specified parent window.
	     * TODO: last argument should point to a CLIENTCREATESTRUCT
	     * structure. */
	    s_hwnd = CreateWindowEx(
		WS_EX_MDICHILD,
		szVimWndClass, "Vim MSWindows GUI",
		WS_OVERLAPPEDWINDOW | WS_CHILD | WS_CLIPSIBLINGS | 0xC000,
		gui_win_x == -1 ? CW_USEDEFAULT : gui_win_x,
		gui_win_y == -1 ? CW_USEDEFAULT : gui_win_y,
		100,				/* Any value will do */
		100,				/* Any value will do */
		vim_parent_hwnd, NULL,
		s_hinst, NULL);
#ifdef HAVE_TRY_EXCEPT
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	    /* NOP */
	}
#endif
	if (s_hwnd == NULL)
	{
	    EMSG(_("E672: Unable to open window inside MDI application"));
	    mch_exit(2);
	}
    }
    else
    {
	/* If the provided windowid is not valid reset it to zero, so that it
	 * is ignored and we open our own window. */
	if (IsWindow((HWND)win_socket_id) <= 0)
	    win_socket_id = 0;

	/* Create a window.  If win_socket_id is not zero without border and
	 * titlebar, it will be reparented below. */
	s_hwnd = CreateWindow(
		szVimWndClass, "Vim MSWindows GUI",
		win_socket_id == 0 ? WS_OVERLAPPEDWINDOW : WS_POPUP,
		gui_win_x == -1 ? CW_USEDEFAULT : gui_win_x,
		gui_win_y == -1 ? CW_USEDEFAULT : gui_win_y,
		100,				/* Any value will do */
		100,				/* Any value will do */
		NULL, NULL,
		s_hinst, NULL);
	if (s_hwnd != NULL && win_socket_id != 0)
	{
	    SetParent(s_hwnd, (HWND)win_socket_id);
	    ShowWindow(s_hwnd, SW_SHOWMAXIMIZED);
	}
    }

    if (s_hwnd == NULL)
	return FAIL;

#ifdef GLOBAL_IME
    global_ime_init(atom, s_hwnd);
#endif
#if defined(FEAT_MBYTE_IME) && defined(DYNAMIC_IME)
    dyn_imm_load();
#endif

    /* Create the text area window */
    if (GetClassInfo(s_hinst, szTextAreaClass, &wndclass) == 0)
    {
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
    s_textArea = CreateWindowEx(
	WS_EX_CLIENTEDGE,
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

#ifdef FEAT_WINDOWS
    DragAcceptFiles(s_hwnd, TRUE);
#endif

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
    gui.border_offset = gui.border_width + 2;	/*CLIENT EDGE*/

    /*
     * Set up for Intellimouse processing
     */
    init_mouse_wheel();

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
#ifdef FEAT_GUI_TABLINE
    /*
     * Create the tabline
     */
    initialise_tabline();
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
# if defined(FEAT_MBYTE) && defined(WIN3264)
    s_findrep_struct_w.lStructSize = sizeof(s_findrep_struct_w);
    s_findrep_struct_w.lpstrFindWhat =
			      (LPWSTR)alloc(MSWIN_FR_BUFSIZE * sizeof(WCHAR));
    s_findrep_struct_w.lpstrFindWhat[0] = NUL;
    s_findrep_struct_w.lpstrReplaceWith =
			      (LPWSTR)alloc(MSWIN_FR_BUFSIZE * sizeof(WCHAR));
    s_findrep_struct_w.lpstrReplaceWith[0] = NUL;
    s_findrep_struct_w.wFindWhatLen = MSWIN_FR_BUFSIZE;
    s_findrep_struct_w.wReplaceWithLen = MSWIN_FR_BUFSIZE;
# endif
#endif

theend:
    /* Display any pending error messages */
    display_errors();

    return OK;
}

/*
 * Get the size of the screen, taking position on multiple monitors into
 * account (if supported).
 */
    static void
get_work_area(RECT *spi_rect)
{
    _HMONITOR	    mon;
    _MONITORINFO    moninfo;

    /* use these functions only if available */
    if (pMonitorFromWindow != NULL && pGetMonitorInfo != NULL)
    {
	/* work out which monitor the window is on, and get *it's* work area */
	mon = pMonitorFromWindow(s_hwnd, 1 /*MONITOR_DEFAULTTOPRIMARY*/);
	if (mon != NULL)
	{
	    moninfo.cbSize = sizeof(_MONITORINFO);
	    if (pGetMonitorInfo(mon, &moninfo))
	    {
		*spi_rect = moninfo.rcWork;
		return;
	    }
	}
    }
    /* this is the old method... */
    SystemParametersInfo(SPI_GETWORKAREA, 0, spi_rect, 0);
}

/*
 * Set the size of the window to the given width and height in pixels.
 */
/*ARGSUSED*/
    void
gui_mch_set_shellsize(int width, int height,
	int min_width, int min_height, int base_width, int base_height,
	int direction)
{
    RECT	workarea_rect;
    int		win_width, win_height;
    int		win_xpos, win_ypos;
    WINDOWPLACEMENT wndpl;
    int		workarea_left;

    /* Try to keep window completely on screen. */
    /* Get position of the screen work area.  This is the part that is not
     * used by the taskbar or appbars. */
    get_work_area(&workarea_rect);

    /* Get current posision of our window.  Note that the .left and .top are
     * relative to the work area.  */
    wndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(s_hwnd, &wndpl);

    /* Resizing a maximized window looks very strange, unzoom it first.
     * But don't do it when still starting up, it may have been requested in
     * the shortcut. */
    if (wndpl.showCmd == SW_SHOWMAXIMIZED && starting == 0)
    {
	ShowWindow(s_hwnd, SW_SHOWNORMAL);
	/* Need to get the settings of the normal window. */
	GetWindowPlacement(s_hwnd, &wndpl);
    }

    win_xpos = wndpl.rcNormalPosition.left;
    win_ypos = wndpl.rcNormalPosition.top;

    /* compute the size of the outside of the window */
    win_width = width + GetSystemMetrics(SM_CXFRAME) * 2;
    win_height = height + GetSystemMetrics(SM_CYFRAME) * 2
			+ GetSystemMetrics(SM_CYCAPTION)
#ifdef FEAT_MENU
			+ gui_mswin_get_menu_height(FALSE)
#endif
			;

    /* There is an inconsistency when using two monitors and Vim is on the
     * second (right) one: win_xpos will be the offset from the workarea of
     * the left monitor.  While with one monitor it's the offset from the
     * workarea (including a possible taskbar on the left).  Detect the second
     * monitor by checking for the left offset to be quite big. */
    if (workarea_rect.left > 300)
	workarea_left = 0;
    else
	workarea_left = workarea_rect.left;

    /* If the window is going off the screen, move it on to the screen.
     * win_xpos and win_ypos are relative to the workarea. */
    if ((direction & RESIZE_HOR)
	    && workarea_left + win_xpos + win_width > workarea_rect.right)
	win_xpos = workarea_rect.right - win_width - workarea_left;

    if ((direction & RESIZE_HOR) && win_xpos < 0)
	win_xpos = 0;

    if ((direction & RESIZE_VERT)
	  && workarea_rect.top + win_ypos + win_height > workarea_rect.bottom)
	win_ypos = workarea_rect.bottom - win_height - workarea_rect.top;

    if ((direction & RESIZE_VERT) && win_ypos < 0)
	win_ypos = 0;

    wndpl.rcNormalPosition.left = win_xpos;
    wndpl.rcNormalPosition.right = win_xpos + win_width;
    wndpl.rcNormalPosition.top = win_ypos;
    wndpl.rcNormalPosition.bottom = win_ypos + win_height;

    /* set window position - we should use SetWindowPlacement rather than
     * SetWindowPos as the MSDN docs say the coord systems returned by
     * these two are not compatible. */
    SetWindowPlacement(s_hwnd, &wndpl);

    SetActiveWindow(s_hwnd);
    SetFocus(s_hwnd);

#ifdef FEAT_MENU
    /* Menu may wrap differently now */
    gui_mswin_get_menu_height(!gui.starting);
#endif
}


    void
gui_mch_set_scrollbar_thumb(
    scrollbar_T *sb,
    long	val,
    long	size,
    long	max)
{
    SCROLLINFO	info;

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

    info.cbSize = sizeof(info);
    info.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    info.nPos = val;
    info.nMin = 0;
    info.nMax = max;
    info.nPage = size;
    SetScrollInfo(sb->id, SB_CTL, &info, TRUE);
}


/*
 * Set the current text font.
 */
    void
gui_mch_set_font(GuiFont font)
{
    gui.currFont = font;
}


/*
 * Set the current text foreground color.
 */
    void
gui_mch_set_fg_color(guicolor_T color)
{
    gui.currFgColor = color;
}

/*
 * Set the current text background color.
 */
    void
gui_mch_set_bg_color(guicolor_T color)
{
    gui.currBgColor = color;
}

/*
 * Set the current text special color.
 */
    void
gui_mch_set_sp_color(guicolor_T color)
{
    gui.currSpColor = color;
}

#if defined(FEAT_MBYTE) && defined(FEAT_MBYTE_IME)
/*
 * Multi-byte handling, originally by Sung-Hoon Baek.
 * First static functions (no prototypes generated).
 */
#ifdef _MSC_VER
# include <ime.h>   /* Apparently not needed for Cygwin, MingW or Borland. */
#endif
#include <imm.h>

/*
 * handle WM_IME_NOTIFY message
 */
/*ARGSUSED*/
    static LRESULT
_OnImeNotify(HWND hWnd, DWORD dwCommand, DWORD dwData)
{
    LRESULT lResult = 0;
    HIMC hImc;

    if (!pImmGetContext || (hImc = pImmGetContext(hWnd)) == (HIMC)0)
	return lResult;
    switch (dwCommand)
    {
	case IMN_SETOPENSTATUS:
	    if (pImmGetOpenStatus(hImc))
	    {
		pImmSetCompositionFont(hImc, &norm_logfont);
		im_set_position(gui.row, gui.col);

		/* Disable langmap */
		State &= ~LANGMAP;
		if (State & INSERT)
		{
#if defined(FEAT_WINDOWS) && defined(FEAT_KEYMAP)
		    /* Unshown 'keymap' in status lines */
		    if (curbuf->b_p_iminsert == B_IMODE_LMAP)
		    {
			/* Save cursor position */
			int old_row = gui.row;
			int old_col = gui.col;

			// This must be called here before
			// status_redraw_curbuf(), otherwise the mode
			// message may appear in the wrong position.
			showmode();
			status_redraw_curbuf();
			update_screen(0);
			/* Restore cursor position */
			gui.row = old_row;
			gui.col = old_col;
		    }
#endif
		}
	    }
	    gui_update_cursor(TRUE, FALSE);
	    lResult = 0;
	    break;
    }
    pImmReleaseContext(hWnd, hImc);
    return lResult;
}

/*ARGSUSED*/
    static LRESULT
_OnImeComposition(HWND hwnd, WPARAM dbcs, LPARAM param)
{
    char_u	*ret;
    int		len;

    if ((param & GCS_RESULTSTR) == 0) /* Composition unfinished. */
	return 0;

    ret = GetResultStr(hwnd, GCS_RESULTSTR, &len);
    if (ret != NULL)
    {
	add_to_input_buf_csi(ret, len);
	vim_free(ret);
	return 1;
    }
    return 0;
}

/*
 * get the current composition string, in UCS-2; *lenp is the number of
 * *lenp is the number of Unicode characters.
 */
    static short_u *
GetCompositionString_inUCS2(HIMC hIMC, DWORD GCS, int *lenp)
{
    LONG	    ret;
    LPWSTR	    wbuf = NULL;
    char_u	    *buf;

    if (!pImmGetContext)
	return NULL; /* no imm32.dll */

    /* Try Unicode; this'll always work on NT regardless of codepage. */
    ret = pImmGetCompositionStringW(hIMC, GCS, NULL, 0);
    if (ret == 0)
	return NULL; /* empty */

    if (ret > 0)
    {
	/* Allocate the requested buffer plus space for the NUL character. */
	wbuf = (LPWSTR)alloc(ret + sizeof(WCHAR));
	if (wbuf != NULL)
	{
	    pImmGetCompositionStringW(hIMC, GCS, wbuf, ret);
	    *lenp = ret / sizeof(WCHAR);
	}
	return (short_u *)wbuf;
    }

    /* ret < 0; we got an error, so try the ANSI version.  This'll work
     * on 9x/ME, but only if the codepage happens to be set to whatever
     * we're inputting. */
    ret = pImmGetCompositionStringA(hIMC, GCS, NULL, 0);
    if (ret <= 0)
	return NULL; /* empty or error */

    buf = alloc(ret);
    if (buf == NULL)
	return NULL;
    pImmGetCompositionStringA(hIMC, GCS, buf, ret);

    /* convert from codepage to UCS-2 */
    MultiByteToWideChar_alloc(GetACP(), 0, buf, ret, &wbuf, lenp);
    vim_free(buf);

    return (short_u *)wbuf;
}

/*
 * void GetResultStr()
 *
 * This handles WM_IME_COMPOSITION with GCS_RESULTSTR flag on.
 * get complete composition string
 */
    static char_u *
GetResultStr(HWND hwnd, int GCS, int *lenp)
{
    HIMC	hIMC;		/* Input context handle. */
    short_u	*buf = NULL;
    char_u	*convbuf = NULL;

    if (!pImmGetContext || (hIMC = pImmGetContext(hwnd)) == (HIMC)0)
	return NULL;

    /* Reads in the composition string. */
    buf = GetCompositionString_inUCS2(hIMC, GCS, lenp);
    if (buf == NULL)
	return NULL;

    convbuf = utf16_to_enc(buf, lenp);
    pImmReleaseContext(hwnd, hIMC);
    vim_free(buf);
    return convbuf;
}
#endif

/* For global functions we need prototypes. */
#if (defined(FEAT_MBYTE) && defined(FEAT_MBYTE_IME)) || defined(PROTO)

/*
 * set font to IM.
 */
    void
im_set_font(LOGFONT *lf)
{
    HIMC hImc;

    if (pImmGetContext && (hImc = pImmGetContext(s_hwnd)) != (HIMC)0)
    {
	pImmSetCompositionFont(hImc, lf);
	pImmReleaseContext(s_hwnd, hImc);
    }
}

/*
 * Notify cursor position to IM.
 */
    void
im_set_position(int row, int col)
{
    HIMC hImc;

    if (pImmGetContext && (hImc = pImmGetContext(s_hwnd)) != (HIMC)0)
    {
	COMPOSITIONFORM	cfs;

	cfs.dwStyle = CFS_POINT;
	cfs.ptCurrentPos.x = FILL_X(col);
	cfs.ptCurrentPos.y = FILL_Y(row);
	MapWindowPoints(s_textArea, s_hwnd, &cfs.ptCurrentPos, 1);
	pImmSetCompositionWindow(hImc, &cfs);

	pImmReleaseContext(s_hwnd, hImc);
    }
}

/*
 * Set IM status on ("active" is TRUE) or off ("active" is FALSE).
 */
    void
im_set_active(int active)
{
    HIMC	hImc;
    static HIMC	hImcOld = (HIMC)0;

    if (pImmGetContext)	    /* if NULL imm32.dll wasn't loaded (yet) */
    {
	if (p_imdisable)
	{
	    if (hImcOld == (HIMC)0)
	    {
		hImcOld = pImmGetContext(s_hwnd);
		if (hImcOld)
		    pImmAssociateContext(s_hwnd, (HIMC)0);
	    }
	    active = FALSE;
	}
	else if (hImcOld != (HIMC)0)
	{
	    pImmAssociateContext(s_hwnd, hImcOld);
	    hImcOld = (HIMC)0;
	}

	hImc = pImmGetContext(s_hwnd);
	if (hImc)
	{
	    /*
	     * for Korean ime
	     */
	    HKL hKL = GetKeyboardLayout(0);

	    if (LOWORD(hKL) == MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN))
	    {
		static DWORD dwConversionSaved = 0, dwSentenceSaved = 0;
		static BOOL bSaved = FALSE;

		if (active)
		{
		    /* if we have a saved conversion status, restore it */
		    if (bSaved)
			pImmSetConversionStatus(hImc, dwConversionSaved,
							     dwSentenceSaved);
		    bSaved = FALSE;
		}
		else
		{
		    /* save conversion status and disable korean */
		    if (pImmGetConversionStatus(hImc, &dwConversionSaved,
							    &dwSentenceSaved))
		    {
			bSaved = TRUE;
			pImmSetConversionStatus(hImc,
				dwConversionSaved & ~(IME_CMODE_NATIVE
						       | IME_CMODE_FULLSHAPE),
				dwSentenceSaved);
		    }
		}
	    }

	    pImmSetOpenStatus(hImc, active);
	    pImmReleaseContext(s_hwnd, hImc);
	}
    }
}

/*
 * Get IM status.  When IM is on, return not 0.  Else return 0.
 */
    int
im_get_status()
{
    int		status = 0;
    HIMC	hImc;

    if (pImmGetContext && (hImc = pImmGetContext(s_hwnd)) != (HIMC)0)
    {
	status = pImmGetOpenStatus(hImc) ? 1 : 0;
	pImmReleaseContext(s_hwnd, hImc);
    }
    return status;
}

#endif /* FEAT_MBYTE && FEAT_MBYTE_IME */

#if defined(FEAT_MBYTE) && !defined(FEAT_MBYTE_IME) && defined(GLOBAL_IME)
/* Win32 with GLOBAL IME */

/*
 * Notify cursor position to IM.
 */
    void
im_set_position(int row, int col)
{
    /* Win32 with GLOBAL IME */
    POINT p;

    p.x = FILL_X(col);
    p.y = FILL_Y(row);
    MapWindowPoints(s_textArea, s_hwnd, &p, 1);
    global_ime_set_position(&p);
}

/*
 * Set IM status on ("active" is TRUE) or off ("active" is FALSE).
 */
    void
im_set_active(int active)
{
    global_ime_set_status(active);
}

/*
 * Get IM status.  When IM is on, return not 0.  Else return 0.
 */
    int
im_get_status()
{
    return global_ime_get_status();
}
#endif

#ifdef FEAT_MBYTE
/*
 * Convert latin9 text "text[len]" to ucs-2 in "unicodebuf".
 */
    static void
latin9_to_ucs(char_u *text, int len, WCHAR *unicodebuf)
{
    int		c;

    while (--len >= 0)
    {
	c = *text++;
	switch (c)
	{
	    case 0xa4: c = 0x20ac; break;   /* euro */
	    case 0xa6: c = 0x0160; break;   /* S hat */
	    case 0xa8: c = 0x0161; break;   /* S -hat */
	    case 0xb4: c = 0x017d; break;   /* Z hat */
	    case 0xb8: c = 0x017e; break;   /* Z -hat */
	    case 0xbc: c = 0x0152; break;   /* OE */
	    case 0xbd: c = 0x0153; break;   /* oe */
	    case 0xbe: c = 0x0178; break;   /* Y */
	}
	*unicodebuf++ = c;
    }
}
#endif

#ifdef FEAT_RIGHTLEFT
/*
 * What is this for?  In the case where you are using Win98 or Win2K or later,
 * and you are using a Hebrew font (or Arabic!), Windows does you a favor and
 * reverses the string sent to the TextOut... family.  This sucks, because we
 * go to a lot of effort to do the right thing, and there doesn't seem to be a
 * way to tell Windblows not to do this!
 *
 * The short of it is that this 'RevOut' only gets called if you are running
 * one of the new, "improved" MS OSes, and only if you are running in
 * 'rightleft' mode.  It makes display take *slightly* longer, but not
 * noticeably so.
 */
    static void
RevOut( HDC s_hdc,
	int col,
	int row,
	UINT foptions,
	CONST RECT *pcliprect,
	LPCTSTR text,
	UINT len,
	CONST INT *padding)
{
    int		ix;
    static int	special = -1;

    if (special == -1)
    {
	/* Check windows version: special treatment is needed if it is NT 5 or
	 * Win98 or higher. */
	if  ((os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
		    && os_version.dwMajorVersion >= 5)
		|| (os_version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
		    && (os_version.dwMajorVersion > 4
			|| (os_version.dwMajorVersion == 4
			    && os_version.dwMinorVersion > 0))))
	    special = 1;
	else
	    special = 0;
    }

    if (special)
	for (ix = 0; ix < (int)len; ++ix)
	    ExtTextOut(s_hdc, col + TEXT_X(ix), row, foptions,
					    pcliprect, text + ix, 1, padding);
    else
	ExtTextOut(s_hdc, col, row, foptions, pcliprect, text, len, padding);
}
#endif

    void
gui_mch_draw_string(
    int		row,
    int		col,
    char_u	*text,
    int		len,
    int		flags)
{
    static int	*padding = NULL;
    static int	pad_size = 0;
    int		i;
    const RECT	*pcliprect = NULL;
    UINT	foptions = 0;
#ifdef FEAT_MBYTE
    static WCHAR *unicodebuf = NULL;
    static int   *unicodepdy = NULL;
    static int	unibuflen = 0;
    int		n = 0;
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
    static HBRUSH	hbr_cache[2] = {NULL, NULL};
    static guicolor_T	brush_color[2] = {INVALCOLOR, INVALCOLOR};
    static int		brush_lru = 0;
    HBRUSH		hbr;
    RECT		rc;

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

	/* Cache the created brush, that saves a lot of time.  We need two:
	 * one for cursor background and one for the normal background. */
	if (gui.currBgColor == brush_color[0])
	{
	    hbr = hbr_cache[0];
	    brush_lru = 1;
	}
	else if (gui.currBgColor == brush_color[1])
	{
	    hbr = hbr_cache[1];
	    brush_lru = 0;
	}
	else
	{
	    if (hbr_cache[brush_lru] != NULL)
		DeleteBrush(hbr_cache[brush_lru]);
	    hbr_cache[brush_lru] = CreateSolidBrush(gui.currBgColor);
	    brush_color[brush_lru] = gui.currBgColor;
	    hbr = hbr_cache[brush_lru];
	    brush_lru = !brush_lru;
	}
	FillRect(s_hdc, &rc, hbr);

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
     * The alternative would be to write the characters in opaque mode, but
     * when the text is not exactly the same proportions as normal text, too
     * big or too little a rectangle gets drawn for the background.
     */
    SetBkMode(s_hdc, OPAQUE);
    SetBkColor(s_hdc, gui.currBgColor);
#endif
    SetTextColor(s_hdc, gui.currFgColor);
    SelectFont(s_hdc, gui.currFont);

    if (pad_size != Columns || padding == NULL || padding[0] != gui.char_width)
    {
	vim_free(padding);
	pad_size = Columns;

	/* Don't give an out-of-memory message here, it would call us
	 * recursively. */
	padding = (int *)lalloc(pad_size * sizeof(int), FALSE);
	if (padding != NULL)
	    for (i = 0; i < pad_size; i++)
		padding[i] = gui.char_width;
    }

    /* On NT, tell the font renderer not to "help" us with Hebrew and Arabic
     * text.  This doesn't work in 9x, so we have to deal with it manually on
     * those systems. */
    if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	foptions |= ETO_IGNORELANGUAGE;

    /*
     * We have to provide the padding argument because italic and bold versions
     * of fixed-width fonts are often one pixel or so wider than their normal
     * versions.
     * No check for DRAW_BOLD, Windows will have done it already.
     */

#ifdef FEAT_MBYTE
    /* Check if there are any UTF-8 characters.  If not, use normal text
     * output to speed up output. */
    if (enc_utf8)
	for (n = 0; n < len; ++n)
	    if (text[n] >= 0x80)
		break;

    /* Check if the Unicode buffer exists and is big enough.  Create it
     * with the same length as the multi-byte string, the number of wide
     * characters is always equal or smaller. */
    if ((enc_utf8
		|| (enc_codepage > 0 && (int)GetACP() != enc_codepage)
		|| enc_latin9)
	    && (unicodebuf == NULL || len > unibuflen))
    {
	vim_free(unicodebuf);
	unicodebuf = (WCHAR *)lalloc(len * sizeof(WCHAR), FALSE);

	vim_free(unicodepdy);
	unicodepdy = (int *)lalloc(len * sizeof(int), FALSE);

	unibuflen = len;
    }

    if (enc_utf8 && n < len && unicodebuf != NULL)
    {
	/* Output UTF-8 characters.  Caller has already separated
	 * composing characters. */
	int		i;
	int		wlen;	/* string length in words */
	int		clen;	/* string length in characters */
	int		cells;	/* cell width of string up to composing char */
	int		cw;	/* width of current cell */
	int		c;

	wlen = 0;
	clen = 0;
	cells = 0;
	for (i = 0; i < len; )
	{
	    c = utf_ptr2char(text + i);
	    if (c >= 0x10000)
	    {
		/* Turn into UTF-16 encoding. */
		unicodebuf[wlen++] = ((c - 0x10000) >> 10) + 0xD800;
		unicodebuf[wlen++] = ((c - 0x10000) & 0x3ff) + 0xDC00;
	    }
	    else
	    {
		unicodebuf[wlen++] = c;
	    }
	    cw = utf_char2cells(c);
	    if (cw > 2)		/* don't use 4 for unprintable char */
		cw = 1;
	    if (unicodepdy != NULL)
	    {
		/* Use unicodepdy to make characters fit as we expect, even
		 * when the font uses different widths (e.g., bold character
		 * is wider).  */
		unicodepdy[clen] = cw * gui.char_width;
	    }
	    cells += cw;
	    i += utfc_ptr2len_len(text + i, len - i);
	    ++clen;
	}
	ExtTextOutW(s_hdc, TEXT_X(col), TEXT_Y(row),
			   foptions, pcliprect, unicodebuf, wlen, unicodepdy);
	len = cells;	/* used for underlining */
    }
    else if ((enc_codepage > 0 && (int)GetACP() != enc_codepage) || enc_latin9)
    {
	/* If we want to display codepage data, and the current CP is not the
	 * ANSI one, we need to go via Unicode. */
	if (unicodebuf != NULL)
	{
	    if (enc_latin9)
		latin9_to_ucs(text, len, unicodebuf);
	    else
		len = MultiByteToWideChar(enc_codepage,
			MB_PRECOMPOSED,
			(char *)text, len,
			(LPWSTR)unicodebuf, unibuflen);
	    if (len != 0)
	    {
		/* Use unicodepdy to make characters fit as we expect, even
		 * when the font uses different widths (e.g., bold character
		 * is wider). */
		if (unicodepdy != NULL)
		{
		    int i;
		    int cw;

		    for (i = 0; i < len; ++i)
		    {
			cw = utf_char2cells(unicodebuf[i]);
			if (cw > 2)
			    cw = 1;
			unicodepdy[i] = cw * gui.char_width;
		    }
		}
		ExtTextOutW(s_hdc, TEXT_X(col), TEXT_Y(row),
			    foptions, pcliprect, unicodebuf, len, unicodepdy);
	    }
	}
    }
    else
#endif
    {
#ifdef FEAT_RIGHTLEFT
	/* If we can't use ETO_IGNORELANGUAGE, we can't tell Windows not to
	 * mess up RL text, so we have to draw it character-by-character.
	 * Only do this if RL is on, since it's slow. */
	if (curwin->w_p_rl && !(foptions & ETO_IGNORELANGUAGE))
	    RevOut(s_hdc, TEXT_X(col), TEXT_Y(row),
			 foptions, pcliprect, (char *)text, len, padding);
	else
#endif
	    ExtTextOut(s_hdc, TEXT_X(col), TEXT_Y(row),
			 foptions, pcliprect, (char *)text, len, padding);
    }

    /* Underline */
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

    /* Undercurl */
    if (flags & DRAW_UNDERC)
    {
	int			x;
	int			offset;
	static const int	val[8] = {1, 0, 0, 0, 1, 2, 2, 2 };

	y = FILL_Y(row + 1) - 1;
	for (x = FILL_X(col); x < FILL_X(col + len); ++x)
	{
	    offset = val[x % 8];
	    SetPixel(s_hdc, x, y - offset, gui.currSpColor);
	}
    }
}


/*
 * Output routines.
 */

/* Flush any output to the screen */
    void
gui_mch_flush(void)
{
#   if defined(__BORLANDC__)
    /*
     * The GdiFlush declaration (in Borland C 5.01 <wingdi.h>) is not a
     * prototype declaration.
     * The compiler complains if __stdcall is not used in both declarations.
     */
    BOOL  __stdcall GdiFlush(void);
#   endif

    GdiFlush();
}

    static void
clear_rect(RECT *rcp)
{
    HBRUSH  hbr;

    hbr = CreateSolidBrush(gui.back_pixel);
    FillRect(s_hdc, rcp, hbr);
    DeleteBrush(hbr);
}


    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
    RECT	workarea_rect;

    get_work_area(&workarea_rect);

    *screen_w = workarea_rect.right - workarea_rect.left
		- GetSystemMetrics(SM_CXFRAME) * 2;

    /* FIXME: dirty trick: Because the gui_get_base_height() doesn't include
     * the menubar for MSwin, we subtract it from the screen height, so that
     * the window size can be made to fit on the screen. */
    *screen_h = workarea_rect.bottom - workarea_rect.top
		- GetSystemMetrics(SM_CYFRAME) * 2
		- GetSystemMetrics(SM_CYCAPTION)
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
	if (is_winnt_3())
	{
	    InsertMenu((parent == NULL) ? s_menuBar : parent->submenu_id,
		    (UINT)pos, MF_POPUP | MF_STRING | MF_BYPOSITION,
		    (long_u)menu->submenu_id, (LPCTSTR) menu->name);
	}
	else
	{
#ifdef FEAT_MBYTE
	    WCHAR	*wn = NULL;
	    int		n;

	    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	    {
		/* 'encoding' differs from active codepage: convert menu name
		 * and use wide function */
		wn = enc_to_utf16(menu->name, NULL);
		if (wn != NULL)
		{
		    MENUITEMINFOW	infow;

		    infow.cbSize = sizeof(infow);
		    infow.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID
							       | MIIM_SUBMENU;
		    infow.dwItemData = (long_u)menu;
		    infow.wID = menu->id;
		    infow.fType = MFT_STRING;
		    infow.dwTypeData = wn;
		    infow.cch = (UINT)wcslen(wn);
		    infow.hSubMenu = menu->submenu_id;
		    n = InsertMenuItemW((parent == NULL)
					    ? s_menuBar : parent->submenu_id,
					    (UINT)pos, TRUE, &infow);
		    vim_free(wn);
		    if (n == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			/* Failed, try using non-wide function. */
			wn = NULL;
		}
	    }

	    if (wn == NULL)
#endif
	    {
		MENUITEMINFO	info;

		info.cbSize = sizeof(info);
		info.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
		info.dwItemData = (long_u)menu;
		info.wID = menu->id;
		info.fType = MFT_STRING;
		info.dwTypeData = (LPTSTR)menu->name;
		info.cch = (UINT)STRLEN(menu->name);
		info.hSubMenu = menu->submenu_id;
		InsertMenuItem((parent == NULL)
					? s_menuBar : parent->submenu_id,
					(UINT)pos, TRUE, &info);
	    }
	}
    }

    /* Fix window size if menu may have wrapped */
    if (parent == NULL)
	gui_mswin_get_menu_height(!gui.starting);
#ifdef FEAT_TEAROFF
    else if (IsWindow(parent->tearoff_handle))
	rebuild_tearoff(parent);
#endif
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
	POINT	p;

	/* Find the position of the current cursor */
	GetDCOrgEx(s_hdc, &p);
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

#if defined(FEAT_TEAROFF) || defined(PROTO)
/*
 * Given a menu descriptor, e.g. "File.New", find it in the menu hierarchy and
 * create it as a pseudo-"tearoff menu".
 */
    void
gui_make_tearoff(char_u *path_name)
{
    vimmenu_T	*menu = gui_find_menu(path_name);

    /* Found the menu, so tear it off. */
    if (menu != NULL)
	gui_mch_tearoff(menu->dname, menu, 0xffffL, 0xffffL);
}
#endif

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

#ifdef FEAT_TEAROFF
    if (STRNCMP(menu->name, TEAR_STRING, TEAR_LEN) == 0)
    {
	InsertMenu(parent->submenu_id, (UINT)idx, MF_BITMAP|MF_BYPOSITION,
		(UINT)menu->id, (LPCTSTR) s_htearbitmap);
    }
    else
#endif
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
	    newtb.iBitmap = get_toolbar_bitmap(menu);
	    newtb.fsStyle = TBSTYLE_BUTTON;
	}
	newtb.idCommand = menu->id;
	newtb.fsState = TBSTATE_ENABLED;
	newtb.iString = 0;
	SendMessage(s_toolbarhwnd, TB_INSERTBUTTON, (WPARAM)idx,
							     (LPARAM)&newtb);
	menu->submenu_id = (HMENU)-1;
    }
    else
#endif
    {
#ifdef FEAT_MBYTE
	WCHAR	*wn = NULL;
	int	n;

	if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	{
	    /* 'encoding' differs from active codepage: convert menu item name
	     * and use wide function */
	    wn = enc_to_utf16(menu->name, NULL);
	    if (wn != NULL)
	    {
		n = InsertMenuW(parent->submenu_id, (UINT)idx,
			(menu_is_separator(menu->name)
				 ? MF_SEPARATOR : MF_STRING) | MF_BYPOSITION,
			(UINT)menu->id, wn);
		vim_free(wn);
		if (n == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
		    /* Failed, try using non-wide function. */
		    wn = NULL;
	    }
	}
	if (wn == NULL)
#endif
	    InsertMenu(parent->submenu_id, (UINT)idx,
		(menu_is_separator(menu->name) ? MF_SEPARATOR : MF_STRING)
							      | MF_BYPOSITION,
		(UINT)menu->id, (LPCTSTR)menu->name);
#ifdef FEAT_TEAROFF
	if (IsWindow(parent->tearoff_handle))
	    rebuild_tearoff(parent);
#endif
    }
}

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(vimmenu_T *menu)
{
#ifdef FEAT_TOOLBAR
    /*
     * is this a toolbar button?
     */
    if (menu->submenu_id == (HMENU)-1)
    {
	int iButton;

	iButton = (int)SendMessage(s_toolbarhwnd, TB_COMMANDTOINDEX,
							 (WPARAM)menu->id, 0);
	SendMessage(s_toolbarhwnd, TB_DELETEBUTTON, (WPARAM)iButton, 0);
    }
    else
#endif
    {
	if (menu->parent != NULL
		&& menu_is_popup(menu->parent->dname)
		&& menu->parent->submenu_id != NULL)
	    RemoveMenu(menu->parent->submenu_id, menu->id, MF_BYCOMMAND);
	else
	    RemoveMenu(s_menuBar, menu->id, MF_BYCOMMAND);
	if (menu->submenu_id != NULL)
	    DestroyMenu(menu->submenu_id);
#ifdef FEAT_TEAROFF
	if (IsWindow(menu->tearoff_handle))
	    DestroyWindow(menu->tearoff_handle);
	if (menu->parent != NULL
		&& menu->parent->children != NULL
		&& IsWindow(menu->parent->tearoff_handle))
	{
	    /* This menu must not show up when rebuilding the tearoff window. */
	    menu->modes = 0;
	    rebuild_tearoff(menu->parent);
	}
#endif
    }
}

#ifdef FEAT_TEAROFF
    static void
rebuild_tearoff(vimmenu_T *menu)
{
    /*hackish*/
    char_u	tbuf[128];
    RECT	trect;
    RECT	rct;
    RECT	roct;
    int		x, y;

    HWND thwnd = menu->tearoff_handle;

    GetWindowText(thwnd, tbuf, 127);
    if (GetWindowRect(thwnd, &trect)
	    && GetWindowRect(s_hwnd, &rct)
	    && GetClientRect(s_hwnd, &roct))
    {
	x = trect.left - rct.left;
	y = (trect.top -  rct.bottom  + roct.bottom);
    }
    else
    {
	x = y = 0xffffL;
    }
    DestroyWindow(thwnd);
    if (menu->children != NULL)
    {
	gui_mch_tearoff(tbuf, menu, x, y);
	if (IsWindow(menu->tearoff_handle))
	    (void) SetWindowPos(menu->tearoff_handle,
				NULL,
				(int)trect.left,
				(int)trect.top,
				0, 0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
#endif /* FEAT_TEAROFF */

/*
 * Make a menu either grey or not grey.
 */
    void
gui_mch_menu_grey(
    vimmenu_T	*menu,
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

#ifdef FEAT_TEAROFF
    if ((menu->parent != NULL) && (IsWindow(menu->parent->tearoff_handle)))
    {
	WORD menuID;
	HWND menuHandle;

	/*
	 * A tearoff button has changed state.
	 */
	if (menu->children == NULL)
	    menuID = (WORD)(menu->id);
	else
	    menuID = (WORD)((long_u)(menu->submenu_id) | (DWORD)0x8000);
	menuHandle = GetDlgItem(menu->parent->tearoff_handle, menuID);
	if (menuHandle)
	    EnableWindow(menuHandle, !grey);

    }
#endif
}

#endif /* FEAT_MENU */


/* define some macros used to make the dialogue creation more readable */

#define add_string(s) strcpy((LPSTR)p, s); (LPSTR)p += (strlen((LPSTR)p) + 1)
#define add_word(x)		*p++ = (x)
#define add_long(x)		dwp = (DWORD *)p; *dwp++ = (x); p = (WORD *)dwp

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
/*ARGSUSED*/
    static LRESULT CALLBACK
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
	else
	    /* We don't have a default, set focus on another element of the
	     * dialog window, probably the icon */
	    (void)SetFocus(GetDlgItem(hwnd, DLG_NONBUTTON_CONTROL));
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
	{
# if defined(FEAT_MBYTE) && defined(WIN3264)
	    /* If the OS is Windows NT, and 'encoding' differs from active
	     * codepage: use wide function and convert text. */
	    if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
		    && enc_codepage >= 0 && (int)GetACP() != enc_codepage)
            {
	       WCHAR  *wp = (WCHAR *)alloc(IOSIZE * sizeof(WCHAR));
	       char_u *p;

	       GetDlgItemTextW(hwnd, DLG_NONBUTTON_CONTROL + 2, wp, IOSIZE);
	       p = utf16_to_enc(wp, NULL);
	       vim_strncpy(s_textfield, p, IOSIZE);
	       vim_free(p);
	       vim_free(wp);
	    }
	    else
# endif
		GetDlgItemText(hwnd, DLG_NONBUTTON_CONTROL + 2,
							 s_textfield, IOSIZE);
	}

	/*
	 * Need to check for IDOK because if the user just hits Return to
	 * accept the default value, some reason this is what we get.
	 */
	if (button == IDOK)
	{
	    if (dialog_default_button > IDCANCEL)
		EndDialog(hwnd, dialog_default_button);
	}
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

static const char_u *dlg_icons[] = /* must match names in resource file */
{
    "IDR_VIM",
    "IDR_VIM_ERROR",
    "IDR_VIM_ALERT",
    "IDR_VIM_INFO",
    "IDR_VIM_QUESTION"
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
    WORD	*p, *pdlgtemplate, *pnumitems;
    DWORD	*dwp;
    int		numButtons;
    int		*buttonWidths, *buttonPositions;
    int		buttonYpos;
    int		nchar, i;
    DWORD	lStyle;
    int		dlgwidth = 0;
    int		dlgheight;
    int		editboxheight;
    int		horizWidth = 0;
    int		msgheight;
    char_u	*pstart;
    char_u	*pend;
    char_u	*last_white;
    char_u	*tbuffer;
    RECT	rect;
    HWND	hwnd;
    HDC		hdc;
    HFONT	font, oldFont;
    TEXTMETRIC	fontInfo;
    int		fontHeight;
    int		textWidth, minButtonWidth, messageWidth;
    int		maxDialogWidth;
    int		maxDialogHeight;
    int		scroll_flag = 0;
    int		vertical;
    int		dlgPaddingX;
    int		dlgPaddingY;
#ifdef USE_SYSMENU_FONT
    LOGFONT	lfSysmenu;
    int		use_lfSysmenu = FALSE;
#endif
    garray_T	ga;
    int		l;

#ifndef NO_CONSOLE
    /* Don't output anything in silent mode ("ex -s") */
    if (silent_mode)
	return dfltbutton;   /* return default option */
#endif

#if 0
    /* If there is no window yet, open it. */
    if (s_hwnd == NULL && gui_mch_init() == FAIL)
	return dfltbutton;
#else
    if (s_hwnd == NULL)
	get_dialog_font_metrics();
#endif

    if ((type < 0) || (type > VIM_LAST_TYPE))
	type = 0;

    /* allocate some memory for dialog template */
    /* TODO should compute this really */
    pdlgtemplate = p = (PWORD)LocalAlloc(LPTR,
					DLG_ALLOC_SIZE + STRLEN(message) * 2);

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
	dfltbutton = -1;

    /* Allocate array to hold the width of each button */
    buttonWidths = (int *)lalloc(numButtons * sizeof(int), TRUE);
    if (buttonWidths == NULL)
	return -1;

    /* Allocate array to hold the X position of each button */
    buttonPositions = (int *)lalloc(numButtons * sizeof(int), TRUE);
    if (buttonPositions == NULL)
	return -1;

    /*
     * Calculate how big the dialog must be.
     */
    hwnd = GetDesktopWindow();
    hdc = GetWindowDC(hwnd);
#ifdef USE_SYSMENU_FONT
    if (gui_w32_get_menu_font(&lfSysmenu) == OK)
    {
	font = CreateFontIndirect(&lfSysmenu);
	use_lfSysmenu = TRUE;
    }
    else
#endif
    font = CreateFont(-DLG_FONT_POINT_SIZE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		      VARIABLE_PITCH , DLG_FONT_NAME);
    if (s_usenewlook)
    {
	oldFont = SelectFont(hdc, font);
	dlgPaddingX = DLG_PADDING_X;
	dlgPaddingY = DLG_PADDING_Y;
    }
    else
    {
	oldFont = SelectFont(hdc, GetStockObject(SYSTEM_FONT));
	dlgPaddingX = DLG_OLD_STYLE_PADDING_X;
	dlgPaddingY = DLG_OLD_STYLE_PADDING_Y;
    }
    GetTextMetrics(hdc, &fontInfo);
    fontHeight = fontInfo.tmHeight;

    /* Minimum width for horizontal button */
    minButtonWidth = GetTextWidth(hdc, "Cancel", 6);

    /* Maximum width of a dialog, if possible */
    if (s_hwnd == NULL)
    {
	RECT	workarea_rect;

	/* We don't have a window, use the desktop area. */
	get_work_area(&workarea_rect);
	maxDialogWidth = workarea_rect.right - workarea_rect.left - 100;
	if (maxDialogWidth > 600)
	    maxDialogWidth = 600;
	maxDialogHeight = workarea_rect.bottom - workarea_rect.top - 100;
    }
    else
    {
	/* Use our own window for the size, unless it's very small. */
	GetWindowRect(s_hwnd, &rect);
	maxDialogWidth = rect.right - rect.left
					   - GetSystemMetrics(SM_CXFRAME) * 2;
	if (maxDialogWidth < DLG_MIN_MAX_WIDTH)
	    maxDialogWidth = DLG_MIN_MAX_WIDTH;

	maxDialogHeight = rect.bottom - rect.top
					   - GetSystemMetrics(SM_CXFRAME) * 2;
	if (maxDialogHeight < DLG_MIN_MAX_HEIGHT)
	    maxDialogHeight = DLG_MIN_MAX_HEIGHT;
    }

    /* Set dlgwidth to width of message.
     * Copy the message into "ga", changing NL to CR-NL and inserting line
     * breaks where needed. */
    pstart = message;
    messageWidth = 0;
    msgheight = 0;
    ga_init2(&ga, sizeof(char), 500);
    do
    {
	msgheight += fontHeight;    /* at least one line */

	/* Need to figure out where to break the string.  The system does it
	 * at a word boundary, which would mean we can't compute the number of
	 * wrapped lines. */
	textWidth = 0;
	last_white = NULL;
	for (pend = pstart; *pend != NUL && *pend != '\n'; )
	{
#ifdef FEAT_MBYTE
	    l = (*mb_ptr2len)(pend);
#else
	    l = 1;
#endif
	    if (l == 1 && vim_iswhite(*pend)
					&& textWidth > maxDialogWidth * 3 / 4)
		last_white = pend;
	    textWidth += GetTextWidth(hdc, pend, l);
	    if (textWidth >= maxDialogWidth)
	    {
		/* Line will wrap. */
		messageWidth = maxDialogWidth;
		msgheight += fontHeight;
		textWidth = 0;

		if (last_white != NULL)
		{
		    /* break the line just after a space */
		    ga.ga_len -= (int)(pend - (last_white + 1));
		    pend = last_white + 1;
		    last_white = NULL;
		}
		ga_append(&ga, '\r');
		ga_append(&ga, '\n');
		continue;
	    }

	    while (--l >= 0)
		ga_append(&ga, *pend++);
	}
	if (textWidth > messageWidth)
	    messageWidth = textWidth;

	ga_append(&ga, '\r');
	ga_append(&ga, '\n');
	pstart = pend + 1;
    } while (*pend != NUL);

    if (ga.ga_data != NULL)
	message = ga.ga_data;

    messageWidth += 10;		/* roundoff space */

    /* Restrict the size to a maximum.  Causes a scrollbar to show up. */
    if (msgheight > maxDialogHeight)
    {
	msgheight = maxDialogHeight;
	scroll_flag = WS_VSCROLL;
	messageWidth += GetSystemMetrics(SM_CXVSCROLL);
    }

    /* Add width of icon to dlgwidth, and some space */
    dlgwidth = messageWidth + DLG_ICON_WIDTH + 3 * dlgPaddingX;

    if (msgheight < DLG_ICON_HEIGHT)
	msgheight = DLG_ICON_HEIGHT;

    /*
     * Check button names.  A long one will make the dialog wider.
     * When called early (-register error message) p_go isn't initialized.
     */
    vertical = (p_go != NULL && vim_strchr(p_go, GO_VERTICAL) != NULL);
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
	    textWidth = GetTextWidth(hdc, pstart, (int)(pend - pstart));
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
	    textWidth = GetTextWidth(hdc, pstart, (int)(pend - pstart));
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
    if (s_usenewlook)
	lStyle = DS_MODALFRAME | WS_CAPTION |DS_3DLOOK| WS_VISIBLE |DS_SETFONT;
    else
	lStyle = DS_MODALFRAME | WS_CAPTION |DS_3DLOOK| WS_VISIBLE;

    add_long(lStyle);
    add_long(0);	// (lExtendedStyle)
    pnumitems = p;	/*save where the number of items must be stored*/
    add_word(0);	// NumberOfItems(will change later)
    add_word(10);	// x
    add_word(10);	// y
    add_word(PixelToDialogX(dlgwidth));	// cx

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

    add_word(0);	// Menu
    add_word(0);	// Class

    /* copy the title of the dialog */
    nchar = nCopyAnsiToWideChar(p, (title ?
				    (LPSTR)title :
				    (LPSTR)("Vim "VIM_VERSION_MEDIUM)));
    p += nchar;

    if (s_usenewlook)
    {
	/* do the font, since DS_3DLOOK doesn't work properly */
#ifdef USE_SYSMENU_FONT
	if (use_lfSysmenu)
	{
	    /* point size */
	    *p++ = -MulDiv(lfSysmenu.lfHeight, 72,
		    GetDeviceCaps(hdc, LOGPIXELSY));
	    nchar = nCopyAnsiToWideChar(p, TEXT(lfSysmenu.lfFaceName));
	}
	else
#endif
	{
	    *p++ = DLG_FONT_POINT_SIZE;		// point size
	    nchar = nCopyAnsiToWideChar(p, TEXT(DLG_FONT_NAME));
	}
	p += nchar;
    }

    buttonYpos = msgheight + 2 * dlgPaddingY;

    if (textfield != NULL)
	buttonYpos += editboxheight;

    pstart = tbuffer;
    if (!vertical)
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
	 * old NOTE:
	 * setting the BS_DEFPUSHBUTTON style doesn't work because Windows sets
	 * the focus to the first tab-able button and in so doing makes that
	 * the default!! Grrr.  Workaround: Make the default button the only
	 * one with WS_TABSTOP style. Means user can't tab between buttons, but
	 * he/she can use arrow keys.
	 *
	 * new NOTE: BS_DEFPUSHBUTTON is required to be able to select the
	 * right button when hitting <Enter>.  E.g., for the ":confirm quit"
	 * dialog.  Also needed for when the textfield is the default control.
	 * It appears to work now (perhaps not on Win95?).
	 */
	if (vertical)
	{
	    p = add_dialog_element(p,
		    (i == dfltbutton
			    ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON) | WS_TABSTOP,
		    PixelToDialogX(DLG_VERT_PADDING_X),
		    PixelToDialogY(buttonYpos /* TBK */
				   + 2 * fontHeight * i),
		    PixelToDialogX(dlgwidth - 2 * DLG_VERT_PADDING_X),
		    (WORD)(PixelToDialogY(2 * fontHeight) - 1),
		    (WORD)(IDCANCEL + 1 + i), (WORD)0x0080, pstart);
	}
	else
	{
	    p = add_dialog_element(p,
		    (i == dfltbutton
			    ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON) | WS_TABSTOP,
		    PixelToDialogX(horizWidth + buttonPositions[i]),
		    PixelToDialogY(buttonYpos), /* TBK */
		    PixelToDialogX(buttonWidths[i]),
		    (WORD)(PixelToDialogY(2 * fontHeight) - 1),
		    (WORD)(IDCANCEL + 1 + i), (WORD)0x0080, pstart);
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
	    DLG_NONBUTTON_CONTROL + 0, (WORD)0x0082,
	    dlg_icons[type]);

#if 0
    /* Dialog message */
    p = add_dialog_element(p, SS_LEFT,
	    PixelToDialogX(2 * dlgPaddingX + DLG_ICON_WIDTH),
	    PixelToDialogY(dlgPaddingY),
	    (WORD)(PixelToDialogX(messageWidth) + 1),
	    PixelToDialogY(msgheight),
	    DLG_NONBUTTON_CONTROL + 1, (WORD)0x0082, message);
#else
    /* Dialog message */
    p = add_dialog_element(p, ES_LEFT|scroll_flag|ES_MULTILINE|ES_READONLY,
	    PixelToDialogX(2 * dlgPaddingX + DLG_ICON_WIDTH),
	    PixelToDialogY(dlgPaddingY),
	    (WORD)(PixelToDialogX(messageWidth) + 1),
	    PixelToDialogY(msgheight),
	    DLG_NONBUTTON_CONTROL + 1, (WORD)0x0081, message);
#endif

    /* Edit box */
    if (textfield != NULL)
    {
	p = add_dialog_element(p, ES_LEFT|ES_AUTOHSCROLL|WS_TABSTOP|WS_BORDER,
		PixelToDialogX(2 * dlgPaddingX),
		PixelToDialogY(2 * dlgPaddingY + msgheight),
		PixelToDialogX(dlgwidth - 4 * dlgPaddingX),
		PixelToDialogY(fontHeight + dlgPaddingY),
		DLG_NONBUTTON_CONTROL + 2, (WORD)0x0081, textfield);
	*pnumitems += 1;
    }

    *pnumitems += 2;

    SelectFont(hdc, oldFont);
    DeleteObject(font);
    ReleaseDC(hwnd, hdc);

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

    /* show the dialog box modally and get a return value */
    nchar = (int)DialogBoxIndirect(
	    s_hinst,
	    (LPDLGTEMPLATE)pdlgtemplate,
	    s_hwnd,
	    (DLGPROC)dialog_callback);

    LocalFree(LocalHandle(pdlgtemplate));
    vim_free(tbuffer);
    vim_free(buttonWidths);
    vim_free(buttonPositions);
    vim_free(ga.ga_data);

    /* Focus back to our window (for when MDI is used). */
    (void)SetFocus(s_hwnd);

    return nchar;
}

#endif /* FEAT_GUI_DIALOG */

/*
 * Put a simple element (basic class) onto a dialog template in memory.
 * return a pointer to where the next item should be added.
 *
 * parameters:
 *  lStyle = additional style flags
 *		(be careful, NT3.51 & Win32s will ignore the new ones)
 *  x,y = x & y positions IN DIALOG UNITS
 *  w,h = width and height IN DIALOG UNITS
 *  Id  = ID used in messages
 *  clss  = class ID, e.g 0x0080 for a button, 0x0082 for a static
 *  caption = usually text or resource name
 *
 *  TODO: use the length information noted here to enable the dialog creation
 *  routines to work out more exactly how much memory they need to alloc.
 */
    static PWORD
add_dialog_element(
    PWORD p,
    DWORD lStyle,
    WORD x,
    WORD y,
    WORD w,
    WORD h,
    WORD Id,
    WORD clss,
    const char *caption)
{
    int nchar;

    p = lpwAlign(p);	/* Align to dword boundary*/
    lStyle = lStyle | WS_VISIBLE | WS_CHILD;
    *p++ = LOWORD(lStyle);
    *p++ = HIWORD(lStyle);
    *p++ = 0;		// LOWORD (lExtendedStyle)
    *p++ = 0;		// HIWORD (lExtendedStyle)
    *p++ = x;
    *p++ = y;
    *p++ = w;
    *p++ = h;
    *p++ = Id;		//9 or 10 words in all

    *p++ = (WORD)0xffff;
    *p++ = clss;			//2 more here

    nchar = nCopyAnsiToWideChar(p, (LPSTR)caption); //strlen(caption)+1
    p += nchar;

    *p++ = 0;  // advance pointer over nExtraStuff WORD   - 2 more

    return p;	//total = 15+ (strlen(caption)) words
		//	   = 30 + 2(strlen(caption) bytes reqd
}


/*
 * Helper routine.  Take an input pointer, return closest pointer that is
 * aligned on a DWORD (4 byte) boundary.  Taken from the Win32SDK samples.
 */
    static LPWORD
lpwAlign(
    LPWORD lpIn)
{
    long_u ul;

    ul = (long_u)lpIn;
    ul += 3;
    ul >>= 2;
    ul <<= 2;
    return (LPWORD)ul;
}

/*
 * Helper routine.  Takes second parameter as Ansi string, copies it to first
 * parameter as wide character (16-bits / char) string, and returns integer
 * number of wide characters (words) in string (including the trailing wide
 * char NULL).  Partly taken from the Win32SDK samples.
 */
    static int
nCopyAnsiToWideChar(
    LPWORD lpWCStr,
    LPSTR lpAnsiIn)
{
    int		nChar = 0;
#ifdef FEAT_MBYTE
    int		len = lstrlen(lpAnsiIn) + 1;	/* include NUL character */
    int		i;
    WCHAR	*wn;

    if (enc_codepage == 0 && (int)GetACP() != enc_codepage)
    {
	/* Not a codepage, use our own conversion function. */
	wn = enc_to_utf16(lpAnsiIn, NULL);
	if (wn != NULL)
	{
	    wcscpy(lpWCStr, wn);
	    nChar = (int)wcslen(wn) + 1;
	    vim_free(wn);
	}
    }
    if (nChar == 0)
	/* Use Win32 conversion function. */
	nChar = MultiByteToWideChar(
		enc_codepage > 0 ? enc_codepage : CP_ACP,
		MB_PRECOMPOSED,
		lpAnsiIn, len,
		lpWCStr, len);
    for (i = 0; i < nChar; ++i)
	if (lpWCStr[i] == (WORD)'\t')	/* replace tabs with spaces */
	    lpWCStr[i] = (WORD)' ';
#else
    do
    {
	if (*lpAnsiIn == '\t')
	    *lpWCStr++ = (WORD)' ';
	else
	    *lpWCStr++ = (WORD)*lpAnsiIn;
	nChar++;
    } while (*lpAnsiIn++);
#endif

    return nChar;
}


#ifdef FEAT_TEAROFF
/*
 * The callback function for all the modeless dialogs that make up the
 * "tearoff menus" Very simple - forward button presses (to fool Vim into
 * thinking its menus have been clicked), and go away when closed.
 */
    static LRESULT CALLBACK
tearoff_callback(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (message == WM_INITDIALOG)
	return (TRUE);

    /* May show the mouse pointer again. */
    HandleMouseHide(message, lParam);

    if (message == WM_COMMAND)
    {
	if ((WORD)(LOWORD(wParam)) & 0x8000)
	{
	    POINT   mp;
	    RECT    rect;

	    if (GetCursorPos(&mp) && GetWindowRect(hwnd, &rect))
	    {
		(void)TrackPopupMenu(
			 (HMENU)(long_u)(LOWORD(wParam) ^ 0x8000),
			 TPM_LEFTALIGN | TPM_LEFTBUTTON,
			 (int)rect.right - 8,
			 (int)mp.y,
			 (int)0,	    /*reserved param*/
			 s_hwnd,
			 NULL);
		/*
		 * NOTE: The pop-up menu can eat the mouse up event.
		 * We deal with this in normal.c.
		 */
	    }
	}
	else
	    /* Pass on messages to the main Vim window */
	    PostMessage(s_hwnd, WM_COMMAND, LOWORD(wParam), 0);
	/*
	 * Give main window the focus back: this is so after
	 * choosing a tearoff button you can start typing again
	 * straight away.
	 */
	(void)SetFocus(s_hwnd);
	return TRUE;
    }
    if ((message == WM_SYSCOMMAND) && (wParam == SC_CLOSE))
    {
	DestroyWindow(hwnd);
	return TRUE;
    }

    /* When moved around, give main window the focus back. */
    if (message == WM_EXITSIZEMOVE)
	(void)SetActiveWindow(s_hwnd);

    return FALSE;
}
#endif


/*
 * Decide whether to use the "new look" (small, non-bold font) or the "old
 * look" (big, clanky font) for dialogs, and work out a few values for use
 * later accordingly.
 */
    static void
get_dialog_font_metrics(void)
{
    HDC		    hdc;
    HFONT	    hfontTools = 0;
    DWORD	    dlgFontSize;
    SIZE	    size;
#ifdef USE_SYSMENU_FONT
    LOGFONT	    lfSysmenu;
#endif

    s_usenewlook = FALSE;

    /*
     * For NT3.51 and Win32s, we stick with the old look
     * because it matches everything else.
     */
    if (!is_winnt_3())
    {
#ifdef USE_SYSMENU_FONT
	if (gui_w32_get_menu_font(&lfSysmenu) == OK)
	    hfontTools = CreateFontIndirect(&lfSysmenu);
	else
#endif
	hfontTools = CreateFont(-DLG_FONT_POINT_SIZE, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, VARIABLE_PITCH , DLG_FONT_NAME);

	if (hfontTools)
	{
	    hdc = GetDC(s_hwnd);
	    SelectObject(hdc, hfontTools);
	    /*
	     * GetTextMetrics() doesn't return the right value in
	     * tmAveCharWidth, so we have to figure out the dialog base units
	     * ourselves.
	     */
	    GetTextExtentPoint(hdc,
		    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
		    52, &size);
	    ReleaseDC(s_hwnd, hdc);

	    s_dlgfntwidth = (WORD)((size.cx / 26 + 1) / 2);
	    s_dlgfntheight = (WORD)size.cy;
	    s_usenewlook = TRUE;
	}
    }

    if (!s_usenewlook)
    {
	dlgFontSize = GetDialogBaseUnits();	/* fall back to big old system*/
	s_dlgfntwidth = LOWORD(dlgFontSize);
	s_dlgfntheight = HIWORD(dlgFontSize);
    }
}

#if defined(FEAT_MENU) && defined(FEAT_TEAROFF)
/*
 * Create a pseudo-"tearoff menu" based on the child
 * items of a given menu pointer.
 */
    static void
gui_mch_tearoff(
    char_u	*title,
    vimmenu_T	*menu,
    int		initX,
    int		initY)
{
    WORD	*p, *pdlgtemplate, *pnumitems, *ptrueheight;
    int		template_len;
    int		nchar, textWidth, submenuWidth;
    DWORD	lStyle;
    DWORD	lExtendedStyle;
    WORD	dlgwidth;
    WORD	menuID;
    vimmenu_T	*pmenu;
    vimmenu_T	*the_menu = menu;
    HWND	hwnd;
    HDC		hdc;
    HFONT	font, oldFont;
    int		col, spaceWidth, len;
    int		columnWidths[2];
    char_u	*label, *text;
    int		acLen = 0;
    int		nameLen;
    int		padding0, padding1, padding2 = 0;
    int		sepPadding=0;
    int		x;
    int		y;
#ifdef USE_SYSMENU_FONT
    LOGFONT	lfSysmenu;
    int		use_lfSysmenu = FALSE;
#endif

    /*
     * If this menu is already torn off, move it to the mouse position.
     */
    if (IsWindow(menu->tearoff_handle))
    {
	POINT mp;
	if (GetCursorPos((LPPOINT)&mp))
	{
	    SetWindowPos(menu->tearoff_handle, NULL, mp.x, mp.y, 0, 0,
		    SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	}
	return;
    }

    /*
     * Create a new tearoff.
     */
    if (*title == MNU_HIDDEN_CHAR)
	title++;

    /* Allocate memory to store the dialog template.  It's made bigger when
     * needed. */
    template_len = DLG_ALLOC_SIZE;
    pdlgtemplate = p = (WORD *)LocalAlloc(LPTR, template_len);
    if (p == NULL)
	return;

    hwnd = GetDesktopWindow();
    hdc = GetWindowDC(hwnd);
#ifdef USE_SYSMENU_FONT
    if (gui_w32_get_menu_font(&lfSysmenu) == OK)
    {
	font = CreateFontIndirect(&lfSysmenu);
	use_lfSysmenu = TRUE;
    }
    else
#endif
    font = CreateFont(-DLG_FONT_POINT_SIZE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		      VARIABLE_PITCH , DLG_FONT_NAME);
    if (s_usenewlook)
	oldFont = SelectFont(hdc, font);
    else
	oldFont = SelectFont(hdc, GetStockObject(SYSTEM_FONT));

    /* Calculate width of a single space.  Used for padding columns to the
     * right width. */
    spaceWidth = GetTextWidth(hdc, " ", 1);

    /* Figure out max width of the text column, the accelerator column and the
     * optional submenu column. */
    submenuWidth = 0;
    for (col = 0; col < 2; col++)
    {
	columnWidths[col] = 0;
	for (pmenu = menu->children; pmenu != NULL; pmenu = pmenu->next)
	{
	    /* Use "dname" here to compute the width of the visible text. */
	    text = (col == 0) ? pmenu->dname : pmenu->actext;
	    if (text != NULL && *text != NUL)
	    {
		textWidth = GetTextWidthEnc(hdc, text, (int)STRLEN(text));
		if (textWidth > columnWidths[col])
		    columnWidths[col] = textWidth;
	    }
	    if (pmenu->children != NULL)
		submenuWidth = TEAROFF_COLUMN_PADDING * spaceWidth;
	}
    }
    if (columnWidths[1] == 0)
    {
	/* no accelerators */
	if (submenuWidth != 0)
	    columnWidths[0] += submenuWidth;
	else
	    columnWidths[0] += spaceWidth;
    }
    else
    {
	/* there is an accelerator column */
	columnWidths[0] += TEAROFF_COLUMN_PADDING * spaceWidth;
	columnWidths[1] += submenuWidth;
    }

    /*
     * Now find the total width of our 'menu'.
     */
    textWidth = columnWidths[0] + columnWidths[1];
    if (submenuWidth != 0)
    {
	submenuWidth = GetTextWidth(hdc, TEAROFF_SUBMENU_LABEL,
					  (int)STRLEN(TEAROFF_SUBMENU_LABEL));
	textWidth += submenuWidth;
    }
    dlgwidth = GetTextWidthEnc(hdc, title, (int)STRLEN(title));
    if (textWidth > dlgwidth)
	dlgwidth = textWidth;
    dlgwidth += 2 * TEAROFF_PADDING_X + TEAROFF_BUTTON_PAD_X;

    /* W95 can't do thin dialogs, they look v. weird! */
    if (mch_windows95() && dlgwidth < TEAROFF_MIN_WIDTH)
	dlgwidth = TEAROFF_MIN_WIDTH;

    /* start to fill in the dlgtemplate information.  addressing by WORDs */
    if (s_usenewlook)
	lStyle = DS_MODALFRAME | WS_CAPTION| WS_SYSMENU |DS_SETFONT| WS_VISIBLE;
    else
	lStyle = DS_MODALFRAME | WS_CAPTION| WS_SYSMENU | WS_VISIBLE;

    lExtendedStyle = WS_EX_TOOLWINDOW|WS_EX_STATICEDGE;
    *p++ = LOWORD(lStyle);
    *p++ = HIWORD(lStyle);
    *p++ = LOWORD(lExtendedStyle);
    *p++ = HIWORD(lExtendedStyle);
    pnumitems = p;	/* save where the number of items must be stored */
    *p++ = 0;		// NumberOfItems(will change later)
    gui_mch_getmouse(&x, &y);
    if (initX == 0xffffL)
	*p++ = PixelToDialogX(x); // x
    else
	*p++ = PixelToDialogX(initX); // x
    if (initY == 0xffffL)
	*p++ = PixelToDialogY(y); // y
    else
	*p++ = PixelToDialogY(initY); // y
    *p++ = PixelToDialogX(dlgwidth);    // cx
    ptrueheight = p;
    *p++ = 0;		// dialog height: changed later anyway
    *p++ = 0;		// Menu
    *p++ = 0;		// Class

    /* copy the title of the dialog */
    nchar = nCopyAnsiToWideChar(p, ((*title)
				    ? (LPSTR)title
				    : (LPSTR)("Vim "VIM_VERSION_MEDIUM)));
    p += nchar;

    if (s_usenewlook)
    {
	/* do the font, since DS_3DLOOK doesn't work properly */
#ifdef USE_SYSMENU_FONT
	if (use_lfSysmenu)
	{
	    /* point size */
	    *p++ = -MulDiv(lfSysmenu.lfHeight, 72,
		    GetDeviceCaps(hdc, LOGPIXELSY));
	    nchar = nCopyAnsiToWideChar(p, TEXT(lfSysmenu.lfFaceName));
	}
	else
#endif
	{
	    *p++ = DLG_FONT_POINT_SIZE;		// point size
	    nchar = nCopyAnsiToWideChar (p, TEXT(DLG_FONT_NAME));
	}
	p += nchar;
    }

    /*
     * Loop over all the items in the menu.
     * But skip over the tearbar.
     */
    if (STRCMP(menu->children->name, TEAR_STRING) == 0)
	menu = menu->children->next;
    else
	menu = menu->children;
    for ( ; menu != NULL; menu = menu->next)
    {
	if (menu->modes == 0)	/* this menu has just been deleted */
	    continue;
	if (menu_is_separator(menu->dname))
	{
	    sepPadding += 3;
	    continue;
	}

	/* Check if there still is plenty of room in the template.  Make it
	 * larger when needed. */
	if (((char *)p - (char *)pdlgtemplate) + 1000 > template_len)
	{
	    WORD    *newp;

	    newp = (WORD *)LocalAlloc(LPTR, template_len + 4096);
	    if (newp != NULL)
	    {
		template_len += 4096;
		mch_memmove(newp, pdlgtemplate,
					    (char *)p - (char *)pdlgtemplate);
		p = newp + (p - pdlgtemplate);
		pnumitems = newp + (pnumitems - pdlgtemplate);
		ptrueheight = newp + (ptrueheight - pdlgtemplate);
		LocalFree(LocalHandle(pdlgtemplate));
		pdlgtemplate = newp;
	    }
	}

	/* Figure out minimal length of this menu label.  Use "name" for the
	 * actual text, "dname" for estimating the displayed size.  "name"
	 * has "&a" for mnemonic and includes the accelerator. */
	len = nameLen = (int)STRLEN(menu->name);
	padding0 = (columnWidths[0] - GetTextWidthEnc(hdc, menu->dname,
				      (int)STRLEN(menu->dname))) / spaceWidth;
	len += padding0;

	if (menu->actext != NULL)
	{
	    acLen = (int)STRLEN(menu->actext);
	    len += acLen;
	    textWidth = GetTextWidthEnc(hdc, menu->actext, acLen);
	}
	else
	    textWidth = 0;
	padding1 = (columnWidths[1] - textWidth) / spaceWidth;
	len += padding1;

	if (menu->children == NULL)
	{
	    padding2 = submenuWidth / spaceWidth;
	    len += padding2;
	    menuID = (WORD)(menu->id);
	}
	else
	{
	    len += (int)STRLEN(TEAROFF_SUBMENU_LABEL);
	    menuID = (WORD)((long_u)(menu->submenu_id) | (DWORD)0x8000);
	}

	/* Allocate menu label and fill it in */
	text = label = alloc((unsigned)len + 1);
	if (label == NULL)
	    break;

	vim_strncpy(text, menu->name, nameLen);
	text = vim_strchr(text, TAB);	    /* stop at TAB before actext */
	if (text == NULL)
	    text = label + nameLen;	    /* no actext, use whole name */
	while (padding0-- > 0)
	    *text++ = ' ';
	if (menu->actext != NULL)
	{
	    STRNCPY(text, menu->actext, acLen);
	    text += acLen;
	}
	while (padding1-- > 0)
	    *text++ = ' ';
	if (menu->children != NULL)
	{
	    STRCPY(text, TEAROFF_SUBMENU_LABEL);
	    text += STRLEN(TEAROFF_SUBMENU_LABEL);
	}
	else
	{
	    while (padding2-- > 0)
		*text++ = ' ';
	}
	*text = NUL;

	/*
	 * BS_LEFT will just be ignored on Win32s/NT3.5x - on
	 * W95/NT4 it makes the tear-off look more like a menu.
	 */
	p = add_dialog_element(p,
		BS_PUSHBUTTON|BS_LEFT,
		(WORD)PixelToDialogX(TEAROFF_PADDING_X),
		(WORD)(sepPadding + 1 + 13 * (*pnumitems)),
		(WORD)PixelToDialogX(dlgwidth - 2 * TEAROFF_PADDING_X),
		(WORD)12,
		menuID, (WORD)0x0080, label);
	vim_free(label);
	(*pnumitems)++;
    }

    *ptrueheight = (WORD)(sepPadding + 1 + 13 * (*pnumitems));


    /* show modelessly */
    the_menu->tearoff_handle = CreateDialogIndirect(
	    s_hinst,
	    (LPDLGTEMPLATE)pdlgtemplate,
	    s_hwnd,
	    (DLGPROC)tearoff_callback);

    LocalFree(LocalHandle(pdlgtemplate));
    SelectFont(hdc, oldFont);
    DeleteObject(font);
    ReleaseDC(hwnd, hdc);

    /*
     * Reassert ourselves as the active window.  This is so that after creating
     * a tearoff, the user doesn't have to click with the mouse just to start
     * typing again!
     */
    (void)SetActiveWindow(s_hwnd);

    /* make sure the right buttons are enabled */
    force_menu_update = TRUE;
}
#endif

#if defined(FEAT_TOOLBAR) || defined(PROTO)
#include "gui_w32_rc.h"

/* This not defined in older SDKs */
# ifndef TBSTYLE_FLAT
#  define TBSTYLE_FLAT		0x0800
# endif

/*
 * Create the toolbar, initially unpopulated.
 *  (just like the menu, there are no defaults, it's all
 *  set up through menu.vim)
 */
    static void
initialise_toolbar(void)
{
    InitCommonControls();
    s_toolbarhwnd = CreateToolbarEx(
		    s_hwnd,
		    WS_CHILD | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
		    4000,		//any old big number
		    31,			//number of images in initial bitmap
		    s_hinst,
		    IDR_TOOLBAR1,	// id of initial bitmap
		    NULL,
		    0,			// initial number of buttons
		    TOOLBAR_BUTTON_WIDTH, //api guide is wrong!
		    TOOLBAR_BUTTON_HEIGHT,
		    TOOLBAR_BUTTON_WIDTH,
		    TOOLBAR_BUTTON_HEIGHT,
		    sizeof(TBBUTTON)
		    );

    gui_mch_show_toolbar(vim_strchr(p_go, GO_TOOLBAR) != NULL);
}

    static int
get_toolbar_bitmap(vimmenu_T *menu)
{
    int i = -1;

    /*
     * Check user bitmaps first, unless builtin is specified.
     */
    if (!is_winnt_3() && !menu->icon_builtin)
    {
	char_u fname[MAXPATHL];
	HANDLE hbitmap = NULL;

	if (menu->iconfile != NULL)
	{
	    gui_find_iconfile(menu->iconfile, fname, "bmp");
	    hbitmap = LoadImage(
			NULL,
			fname,
			IMAGE_BITMAP,
			TOOLBAR_BUTTON_WIDTH,
			TOOLBAR_BUTTON_HEIGHT,
			LR_LOADFROMFILE |
			LR_LOADMAP3DCOLORS
			);
	}

	/*
	 * If the LoadImage call failed, or the "icon=" file
	 * didn't exist or wasn't specified, try the menu name
	 */
	if (hbitmap == NULL
		&& (gui_find_bitmap(menu->name, fname, "bmp") == OK))
	    hbitmap = LoadImage(
		    NULL,
		    fname,
		    IMAGE_BITMAP,
		    TOOLBAR_BUTTON_WIDTH,
		    TOOLBAR_BUTTON_HEIGHT,
		    LR_LOADFROMFILE |
		    LR_LOADMAP3DCOLORS
		);

	if (hbitmap != NULL)
	{
	    TBADDBITMAP tbAddBitmap;

	    tbAddBitmap.hInst = NULL;
	    tbAddBitmap.nID = (long_u)hbitmap;

	    i = (int)SendMessage(s_toolbarhwnd, TB_ADDBITMAP,
			    (WPARAM)1, (LPARAM)&tbAddBitmap);
	    /* i will be set to -1 if it fails */
	}
    }
    if (i == -1 && menu->iconidx >= 0 && menu->iconidx < TOOLBAR_BITMAP_COUNT)
	i = menu->iconidx;

    return i;
}
#endif

#if defined(FEAT_GUI_TABLINE) || defined(PROTO)
    static void
initialise_tabline(void)
{
    InitCommonControls();

    s_tabhwnd = CreateWindow(WC_TABCONTROL, "Vim tabline",
	    WS_CHILD|TCS_FOCUSNEVER|TCS_TOOLTIPS,
	    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	    CW_USEDEFAULT, s_hwnd, NULL, s_hinst, NULL);

    gui.tabline_height = TABLINE_HEIGHT;

# ifdef USE_SYSMENU_FONT
    set_tabline_font();
# endif
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
    SetForegroundWindow(s_hwnd);
}
#endif

#if defined(FEAT_MBYTE_IME) && defined(DYNAMIC_IME)
    static void
dyn_imm_load(void)
{
    hLibImm = LoadLibrary("imm32.dll");
    if (hLibImm == NULL)
	return;

    pImmGetCompositionStringA
	    = (void *)GetProcAddress(hLibImm, "ImmGetCompositionStringA");
    pImmGetCompositionStringW
	    = (void *)GetProcAddress(hLibImm, "ImmGetCompositionStringW");
    pImmGetContext
	    = (void *)GetProcAddress(hLibImm, "ImmGetContext");
    pImmAssociateContext
	    = (void *)GetProcAddress(hLibImm, "ImmAssociateContext");
    pImmReleaseContext
	    = (void *)GetProcAddress(hLibImm, "ImmReleaseContext");
    pImmGetOpenStatus
	    = (void *)GetProcAddress(hLibImm, "ImmGetOpenStatus");
    pImmSetOpenStatus
	    = (void *)GetProcAddress(hLibImm, "ImmSetOpenStatus");
    pImmGetCompositionFont
	    = (void *)GetProcAddress(hLibImm, "ImmGetCompositionFontA");
    pImmSetCompositionFont
	    = (void *)GetProcAddress(hLibImm, "ImmSetCompositionFontA");
    pImmSetCompositionWindow
	    = (void *)GetProcAddress(hLibImm, "ImmSetCompositionWindow");
    pImmGetConversionStatus
	    = (void *)GetProcAddress(hLibImm, "ImmGetConversionStatus");
    pImmSetConversionStatus
	    = (void *)GetProcAddress(hLibImm, "ImmSetConversionStatus");

    if (       pImmGetCompositionStringA == NULL
	    || pImmGetCompositionStringW == NULL
	    || pImmGetContext == NULL
	    || pImmAssociateContext == NULL
	    || pImmReleaseContext == NULL
	    || pImmGetOpenStatus == NULL
	    || pImmSetOpenStatus == NULL
	    || pImmGetCompositionFont == NULL
	    || pImmSetCompositionFont == NULL
	    || pImmSetCompositionWindow == NULL
	    || pImmGetConversionStatus == NULL
	    || pImmSetConversionStatus == NULL)
    {
	FreeLibrary(hLibImm);
	hLibImm = NULL;
	pImmGetContext = NULL;
	return;
    }

    return;
}

# if 0	/* not used */
    int
dyn_imm_unload(void)
{
    if (!hLibImm)
	return FALSE;
    FreeLibrary(hLibImm);
    hLibImm = NULL;
    return TRUE;
}
# endif

#endif

#if defined(FEAT_SIGN_ICONS) || defined(PROTO)

# ifdef FEAT_XPM_W32
#  define IMAGE_XPM   100
# endif

typedef struct _signicon_t
{
    HANDLE	hImage;
    UINT	uType;
#ifdef FEAT_XPM_W32
    HANDLE	hShape;	/* Mask bitmap handle */
#endif
} signicon_t;

    void
gui_mch_drawsign(row, col, typenr)
    int		row;
    int		col;
    int		typenr;
{
    signicon_t *sign;
    int x, y, w, h;

    if (!gui.in_use || (sign = (signicon_t *)sign_get_image(typenr)) == NULL)
	return;

    x = TEXT_X(col);
    y = TEXT_Y(row);
    w = gui.char_width * 2;
    h = gui.char_height;
    switch (sign->uType)
    {
	case IMAGE_BITMAP:
	    {
		HDC hdcMem;
		HBITMAP hbmpOld;

		hdcMem = CreateCompatibleDC(s_hdc);
		hbmpOld = (HBITMAP)SelectObject(hdcMem, sign->hImage);
		BitBlt(s_hdc, x, y, w, h, hdcMem, 0, 0, SRCCOPY);
		SelectObject(hdcMem, hbmpOld);
		DeleteDC(hdcMem);
	    }
	    break;
	case IMAGE_ICON:
	case IMAGE_CURSOR:
	    DrawIconEx(s_hdc, x, y, (HICON)sign->hImage, w, h, 0, NULL, DI_NORMAL);
	    break;
#ifdef FEAT_XPM_W32
	case IMAGE_XPM:
	    {
		HDC hdcMem;
		HBITMAP hbmpOld;

		hdcMem = CreateCompatibleDC(s_hdc);
		hbmpOld = (HBITMAP)SelectObject(hdcMem, sign->hShape);
		/* Make hole */
		BitBlt(s_hdc, x, y, w, h, hdcMem, 0, 0, SRCAND);

		SelectObject(hdcMem, sign->hImage);
		/* Paint sign */
		BitBlt(s_hdc, x, y, w, h, hdcMem, 0, 0, SRCPAINT);
		SelectObject(hdcMem, hbmpOld);
		DeleteDC(hdcMem);
	    }
	    break;
#endif
    }
}

    static void
close_signicon_image(signicon_t *sign)
{
    if (sign)
	switch (sign->uType)
	{
	    case IMAGE_BITMAP:
		DeleteObject((HGDIOBJ)sign->hImage);
		break;
	    case IMAGE_CURSOR:
		DestroyCursor((HCURSOR)sign->hImage);
		break;
	    case IMAGE_ICON:
		DestroyIcon((HICON)sign->hImage);
		break;
#ifdef FEAT_XPM_W32
	    case IMAGE_XPM:
		DeleteObject((HBITMAP)sign->hImage);
		DeleteObject((HBITMAP)sign->hShape);
		break;
#endif
	}
}

    void *
gui_mch_register_sign(signfile)
    char_u	*signfile;
{
    signicon_t	sign, *psign;
    char_u	*ext;

    if (is_winnt_3())
    {
	EMSG(_(e_signdata));
	return NULL;
    }

    sign.hImage = NULL;
    ext = signfile + STRLEN(signfile) - 4; /* get extention */
    if (ext > signfile)
    {
	int do_load = 1;

	if (!STRICMP(ext, ".bmp"))
	    sign.uType =  IMAGE_BITMAP;
	else if (!STRICMP(ext, ".ico"))
	    sign.uType =  IMAGE_ICON;
	else if (!STRICMP(ext, ".cur") || !STRICMP(ext, ".ani"))
	    sign.uType =  IMAGE_CURSOR;
	else
	    do_load = 0;

	if (do_load)
	    sign.hImage = (HANDLE)LoadImage(NULL, signfile, sign.uType,
		    gui.char_width * 2, gui.char_height,
		    LR_LOADFROMFILE | LR_CREATEDIBSECTION);
#ifdef FEAT_XPM_W32
	if (!STRICMP(ext, ".xpm"))
	{
	    sign.uType = IMAGE_XPM;
	    LoadXpmImage(signfile, (HBITMAP *)&sign.hImage, (HBITMAP *)&sign.hShape);
	}
#endif
    }

    psign = NULL;
    if (sign.hImage && (psign = (signicon_t *)alloc(sizeof(signicon_t)))
								      != NULL)
	*psign = sign;

    if (!psign)
    {
	if (sign.hImage)
	    close_signicon_image(&sign);
	EMSG(_(e_signdata));
    }
    return (void *)psign;

}

    void
gui_mch_destroy_sign(sign)
    void *sign;
{
    if (sign)
    {
	close_signicon_image((signicon_t *)sign);
	vim_free(sign);
    }
}
#endif

#if defined(FEAT_BEVAL) || defined(PROTO)

/* BALLOON-EVAL IMPLEMENTATION FOR WINDOWS.
 *  Added by Sergey Khorev <sergey.khorev@gmail.com>
 *
 * The only reused thing is gui_beval.h and get_beval_info()
 * from gui_beval.c (note it uses x and y of the BalloonEval struct
 * to get current mouse position).
 *
 * Trying to use as more Windows services as possible, and as less
 * IE version as possible :)).
 *
 * 1) Don't create ToolTip in gui_mch_create_beval_area, only initialize
 * BalloonEval struct.
 * 2) Enable/Disable simply create/kill BalloonEval Timer
 * 3) When there was enough inactivity, timer procedure posts
 * async request to debugger
 * 4) gui_mch_post_balloon (invoked from netbeans.c) creates tooltip control
 * and performs some actions to show it ASAP
 * 5) WM_NOTIFY:TTN_POP destroys created tooltip
 */

/*
 * determine whether installed Common Controls support multiline tooltips
 * (i.e. their version is >= 4.70
 */
    int
multiline_balloon_available(void)
{
    HINSTANCE hDll;
    static char comctl_dll[] = "comctl32.dll";
    static int multiline_tip = MAYBE;

    if (multiline_tip != MAYBE)
	return multiline_tip;

    hDll = GetModuleHandle(comctl_dll);
    if (hDll != NULL)
    {
	DLLGETVERSIONPROC pGetVer;
	pGetVer = (DLLGETVERSIONPROC)GetProcAddress(hDll, "DllGetVersion");

	if (pGetVer != NULL)
	{
	    DLLVERSIONINFO dvi;
	    HRESULT hr;

	    ZeroMemory(&dvi, sizeof(dvi));
	    dvi.cbSize = sizeof(dvi);

	    hr = (*pGetVer)(&dvi);

	    if (SUCCEEDED(hr)
		    && (dvi.dwMajorVersion > 4
			|| (dvi.dwMajorVersion == 4
						&& dvi.dwMinorVersion >= 70)))
	    {
		multiline_tip = TRUE;
		return multiline_tip;
	    }
	}
	else
	{
	    /* there is chance we have ancient CommCtl 4.70
	       which doesn't export DllGetVersion */
	    DWORD dwHandle = 0;
	    DWORD len = GetFileVersionInfoSize(comctl_dll, &dwHandle);
	    if (len > 0)
	    {
		VS_FIXEDFILEINFO *ver;
		UINT vlen = 0;
		void *data = alloc(len);

		if (data != NULL
			&& GetFileVersionInfo(comctl_dll, 0, len, data)
			&& VerQueryValue(data, "\\", (void **)&ver, &vlen)
			&& vlen
			&& HIWORD(ver->dwFileVersionMS) > 4
			|| (HIWORD(ver->dwFileVersionMS) == 4
			    && LOWORD(ver->dwFileVersionMS) >= 70))
		{
		    vim_free(data);
		    multiline_tip = TRUE;
		    return multiline_tip;
		}
		vim_free(data);
	    }
	}
    }
    multiline_tip = FALSE;
    return multiline_tip;
}

    static void
make_tooltip(beval, text, pt)
    BalloonEval *beval;
    char *text;
    POINT pt;
{
    TOOLINFO	*pti;
    int		ToolInfoSize;

    if (multiline_balloon_available() == TRUE)
	ToolInfoSize = sizeof(TOOLINFO_NEW);
    else
	ToolInfoSize = sizeof(TOOLINFO);

    pti = (TOOLINFO *)alloc(ToolInfoSize);
    if (pti == NULL)
	return;

    beval->balloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,
	    NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
	    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	    beval->target, NULL, s_hinst, NULL);

    SetWindowPos(beval->balloon, HWND_TOPMOST, 0, 0, 0, 0,
	    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    pti->cbSize = ToolInfoSize;
    pti->uFlags = TTF_SUBCLASS;
    pti->hwnd = beval->target;
    pti->hinst = 0; /* Don't use string resources */
    pti->uId = ID_BEVAL_TOOLTIP;

    if (multiline_balloon_available() == TRUE)
    {
	RECT rect;
	TOOLINFO_NEW *ptin = (TOOLINFO_NEW *)pti;
	pti->lpszText = LPSTR_TEXTCALLBACK;
	ptin->lParam = (LPARAM)text;
	if (GetClientRect(s_textArea, &rect)) /* switch multiline tooltips on */
	    SendMessage(beval->balloon, TTM_SETMAXTIPWIDTH, 0,
		    (LPARAM)rect.right);
    }
    else
	pti->lpszText = text; /* do this old way */

    /* Limit ballooneval bounding rect to CursorPos neighbourhood */
    pti->rect.left = pt.x - 3;
    pti->rect.top = pt.y - 3;
    pti->rect.right = pt.x + 3;
    pti->rect.bottom = pt.y + 3;

    SendMessage(beval->balloon, TTM_ADDTOOL, 0, (LPARAM)pti);
    /* Make tooltip appear sooner */
    SendMessage(beval->balloon, TTM_SETDELAYTIME, TTDT_INITIAL, 10);
    /* I've performed some tests and it seems the longest possible life time
     * of tooltip is 30 seconds */
    SendMessage(beval->balloon, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
    /*
     * HACK: force tooltip to appear, because it'll not appear until
     * first mouse move. D*mn M$
     * Amazingly moving (2, 2) and then (-1, -1) the mouse doesn't move.
     */
    mouse_event(MOUSEEVENTF_MOVE, 2, 2, 0, 0);
    mouse_event(MOUSEEVENTF_MOVE, (DWORD)-1, (DWORD)-1, 0, 0);
    vim_free(pti);
}

    static void
delete_tooltip(beval)
    BalloonEval	*beval;
{
    DestroyWindow(beval->balloon);
}

/*ARGSUSED*/
    static VOID CALLBACK
BevalTimerProc(hwnd, uMsg, idEvent, dwTime)
    HWND    hwnd;
    UINT    uMsg;
    UINT_PTR    idEvent;
    DWORD   dwTime;
{
    POINT	pt;
    RECT	rect;

    if (cur_beval == NULL || cur_beval->showState == ShS_SHOWING || !p_beval)
	return;

    GetCursorPos(&pt);
    if (WindowFromPoint(pt) != s_textArea)
	return;

    ScreenToClient(s_textArea, &pt);
    GetClientRect(s_textArea, &rect);
    if (!PtInRect(&rect, pt))
	return;

    if (LastActivity > 0
	    && (dwTime - LastActivity) >= (DWORD)p_bdlay
	    && (cur_beval->showState != ShS_PENDING
		|| abs(cur_beval->x - pt.x) > 3
		|| abs(cur_beval->y - pt.y) > 3))
    {
	/* Pointer resting in one place long enough, it's time to show
	 * the tooltip. */
	cur_beval->showState = ShS_PENDING;
	cur_beval->x = pt.x;
	cur_beval->y = pt.y;

	// TRACE0("BevalTimerProc: sending request");

	if (cur_beval->msgCB != NULL)
	    (*cur_beval->msgCB)(cur_beval, 0);
    }
}

/*ARGSUSED*/
    void
gui_mch_disable_beval_area(beval)
    BalloonEval	*beval;
{
    // TRACE0("gui_mch_disable_beval_area {{{");
    KillTimer(s_textArea, BevalTimerId);
    // TRACE0("gui_mch_disable_beval_area }}}");
}

/*ARGSUSED*/
    void
gui_mch_enable_beval_area(beval)
    BalloonEval	*beval;
{
    // TRACE0("gui_mch_enable_beval_area |||");
    if (beval == NULL)
	return;
    // TRACE0("gui_mch_enable_beval_area {{{");
    BevalTimerId = SetTimer(s_textArea, 0, p_bdlay / 2, BevalTimerProc);
    // TRACE0("gui_mch_enable_beval_area }}}");
}

    void
gui_mch_post_balloon(beval, mesg)
    BalloonEval	*beval;
    char_u	*mesg;
{
    POINT   pt;
    // TRACE0("gui_mch_post_balloon {{{");
    if (beval->showState == ShS_SHOWING)
	return;
    GetCursorPos(&pt);
    ScreenToClient(s_textArea, &pt);

    if (abs(beval->x - pt.x) < 3 && abs(beval->y - pt.y) < 3)
	/* cursor is still here */
    {
	gui_mch_disable_beval_area(cur_beval);
	beval->showState = ShS_SHOWING;
	make_tooltip(beval, mesg, pt);
    }
    // TRACE0("gui_mch_post_balloon }}}");
}

/*ARGSUSED*/
    BalloonEval *
gui_mch_create_beval_area(target, mesg, mesgCB, clientData)
    void	*target;	/* ignored, always use s_textArea */
    char_u	*mesg;
    void	(*mesgCB)__ARGS((BalloonEval *, int));
    void	*clientData;
{
    /* partially stolen from gui_beval.c */
    BalloonEval	*beval;

    if (mesg != NULL && mesgCB != NULL)
    {
	EMSG(_("E232: Cannot create BalloonEval with both message and callback"));
	return NULL;
    }

    beval = (BalloonEval *)alloc(sizeof(BalloonEval));
    if (beval != NULL)
    {
	beval->target = s_textArea;
	beval->balloon = NULL;

	beval->showState = ShS_NEUTRAL;
	beval->x = 0;
	beval->y = 0;
	beval->msg = mesg;
	beval->msgCB = mesgCB;
	beval->clientData = clientData;

	InitCommonControls();
	cur_beval = beval;

	if (p_beval)
	    gui_mch_enable_beval_area(beval);

    }
    return beval;
}

/*ARGSUSED*/
    static void
Handle_WM_Notify(hwnd, pnmh)
    HWND hwnd;
    LPNMHDR pnmh;
{
    if (pnmh->idFrom != ID_BEVAL_TOOLTIP) /* it is not our tooltip */
	return;

    if (cur_beval != NULL)
    {
	switch (pnmh->code)
	{
	case TTN_SHOW:
	    // TRACE0("TTN_SHOW {{{");
	    // TRACE0("TTN_SHOW }}}");
	    break;
	case TTN_POP: /* Before tooltip disappear */
	    // TRACE0("TTN_POP {{{");
	    delete_tooltip(cur_beval);
	    gui_mch_enable_beval_area(cur_beval);
	    // TRACE0("TTN_POP }}}");

	    cur_beval->showState = ShS_NEUTRAL;
	    break;
	case TTN_GETDISPINFO:
	    {
		/* if you get there then we have new common controls */
		NMTTDISPINFO_NEW *info = (NMTTDISPINFO_NEW *)pnmh;
		info->lpszText = (LPSTR)info->lParam;
		info->uFlags |= TTF_DI_SETITEM;
	    }
	    break;
	}
    }
}

    static void
TrackUserActivity(UINT uMsg)
{
    if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
	    || (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST))
	LastActivity = GetTickCount();
}

    void
gui_mch_destroy_beval_area(beval)
    BalloonEval	*beval;
{
    vim_free(beval);
}
#endif /* FEAT_BEVAL */

#if defined(FEAT_NETBEANS_INTG) || defined(PROTO)
/*
 * We have multiple signs to draw at the same location. Draw the
 * multi-sign indicator (down-arrow) instead. This is the Win32 version.
 */
    void
netbeans_draw_multisign_indicator(int row)
{
    int i;
    int y;
    int x;

    x = 0;
    y = TEXT_Y(row);

    for (i = 0; i < gui.char_height - 3; i++)
	SetPixel(s_hdc, x+2, y++, gui.currFgColor);

    SetPixel(s_hdc, x+0, y, gui.currFgColor);
    SetPixel(s_hdc, x+2, y, gui.currFgColor);
    SetPixel(s_hdc, x+4, y++, gui.currFgColor);
    SetPixel(s_hdc, x+1, y, gui.currFgColor);
    SetPixel(s_hdc, x+2, y, gui.currFgColor);
    SetPixel(s_hdc, x+3, y++, gui.currFgColor);
    SetPixel(s_hdc, x+2, y, gui.currFgColor);
}
#endif
