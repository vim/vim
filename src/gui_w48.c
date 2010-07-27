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
 * gui_w48.c:  This file is included in gui_w16.c and gui_w32.c.
 *
 * GUI support for Microsoft Windows (Win16 + Win32 = Win48 :-)
 *
 * The combined efforts of:
 * George V. Reilly <george@reilly.org>
 * Robert Webb
 * Vince Negri
 * ...and contributions from many others
 *
 */

#include "vim.h"
#include "version.h"	/* used by dialog box routine for default title */
#ifdef DEBUG
# include <tchar.h>
#endif
#ifndef __MINGW32__
# include <shellapi.h>
#endif
#if defined(FEAT_TOOLBAR) || defined(FEAT_BEVAL) || defined(FEAT_GUI_TABLINE)
# include <commctrl.h>
#endif
#ifdef WIN16
# include <commdlg.h>
# include <shellapi.h>
# ifdef WIN16_3DLOOK
#  include <ctl3d.h>
# endif
#endif
#include <windowsx.h>

#ifdef GLOBAL_IME
# include "glbl_ime.h"
#endif

#ifdef FEAT_MENU
# define MENUHINTS		/* show menu hints in command line */
#endif

/* Some parameters for dialog boxes.  All in pixels. */
#define DLG_PADDING_X		10
#define DLG_PADDING_Y		10
#define DLG_OLD_STYLE_PADDING_X	5
#define DLG_OLD_STYLE_PADDING_Y	5
#define DLG_VERT_PADDING_X	4	/* For vertical buttons */
#define DLG_VERT_PADDING_Y	4
#define DLG_ICON_WIDTH		34
#define DLG_ICON_HEIGHT		34
#define DLG_MIN_WIDTH		150
#define DLG_FONT_NAME		"MS Sans Serif"
#define DLG_FONT_POINT_SIZE	8
#define DLG_MIN_MAX_WIDTH	400
#define DLG_MIN_MAX_HEIGHT	400

#define DLG_NONBUTTON_CONTROL	5000	/* First ID of non-button controls */

#ifndef WM_XBUTTONDOWN /* For Win2K / winME ONLY */
# define WM_XBUTTONDOWN		0x020B
# define WM_XBUTTONUP		0x020C
# define WM_XBUTTONDBLCLK	0x020D
# define MK_XBUTTON1		0x0020
# define MK_XBUTTON2		0x0040
#endif

#ifdef PROTO
/*
 * Define a few things for generating prototypes.  This is just to avoid
 * syntax errors, the defines do not need to be correct.
 */
# define APIENTRY
# define CALLBACK
# define CONST
# define FAR
# define NEAR
# define _cdecl
typedef int BOOL;
typedef int BYTE;
typedef int DWORD;
typedef int WCHAR;
typedef int ENUMLOGFONT;
typedef int FINDREPLACE;
typedef int HANDLE;
typedef int HBITMAP;
typedef int HBRUSH;
typedef int HDROP;
typedef int INT;
typedef int LOGFONT[];
typedef int LPARAM;
typedef int LPCREATESTRUCT;
typedef int LPCSTR;
typedef int LPCTSTR;
typedef int LPRECT;
typedef int LPSTR;
typedef int LPWINDOWPOS;
typedef int LPWORD;
typedef int LRESULT;
typedef int HRESULT;
# undef MSG
typedef int MSG;
typedef int NEWTEXTMETRIC;
typedef int OSVERSIONINFO;
typedef int PWORD;
typedef int RECT;
typedef int UINT;
typedef int WORD;
typedef int WPARAM;
typedef int POINT;
typedef void *HINSTANCE;
typedef void *HMENU;
typedef void *HWND;
typedef void *HDC;
typedef void VOID;
typedef int LPNMHDR;
typedef int LONG;
#endif

#ifndef GET_X_LPARAM
# define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

static void _OnPaint( HWND hwnd);
static void clear_rect(RECT *rcp);
static int gui_mswin_get_menu_height(int fix_window);

static WORD		s_dlgfntheight;		/* height of the dialog font */
static WORD		s_dlgfntwidth;		/* width of the dialog font */

#ifdef FEAT_MENU
static HMENU		s_menuBar = NULL;
#endif
#ifdef FEAT_TEAROFF
static void rebuild_tearoff(vimmenu_T *menu);
static HBITMAP	s_htearbitmap;	    /* bitmap used to indicate tearoff */
#endif

/* Flag that is set while processing a message that must not be interrupted by
 * processing another message. */
static int		s_busy_processing = FALSE;

static int		destroying = FALSE;	/* call DestroyWindow() ourselves */

#ifdef MSWIN_FIND_REPLACE
static UINT		s_findrep_msg = 0;	/* set in gui_w[16/32].c */
static FINDREPLACE	s_findrep_struct;
# if defined(FEAT_MBYTE) && defined(WIN3264)
static FINDREPLACEW	s_findrep_struct_w;
# endif
static HWND		s_findrep_hwnd = NULL;
static int		s_findrep_is_find;	/* TRUE for find dialog, FALSE
						   for find/replace dialog */
#endif

static HINSTANCE	s_hinst = NULL;
#if !defined(FEAT_SNIFF) && !defined(FEAT_GUI)
static
#endif
HWND			s_hwnd = NULL;
static HDC		s_hdc = NULL;
static HBRUSH	s_brush = NULL;

#ifdef FEAT_TOOLBAR
static HWND		s_toolbarhwnd = NULL;
#endif

#ifdef FEAT_GUI_TABLINE
static HWND		s_tabhwnd = NULL;
static int		showing_tabline = 0;
#endif

static WPARAM		s_wParam = 0;
static LPARAM		s_lParam = 0;

static HWND		s_textArea = NULL;
static UINT		s_uMsg = 0;

static char_u		*s_textfield; /* Used by dialogs to pass back strings */

static int		s_need_activate = FALSE;

/* This variable is set when waiting for an event, which is the only moment
 * scrollbar dragging can be done directly.  It's not allowed while commands
 * are executed, because it may move the cursor and that may cause unexpected
 * problems (e.g., while ":s" is working).
 */
static int allow_scrollbar = FALSE;

#ifdef GLOBAL_IME
# define MyTranslateMessage(x) global_ime_TranslateMessage(x)
#else
# define MyTranslateMessage(x) TranslateMessage(x)
#endif

#if (defined(WIN3264) && defined(FEAT_MBYTE)) || defined(GLOBAL_IME)
  /* use of WindowProc depends on wide_WindowProc */
# define MyWindowProc vim_WindowProc
#else
  /* use ordinary WindowProc */
# define MyWindowProc DefWindowProc
#endif

extern int current_font_height;	    /* this is in os_mswin.c */

static struct
{
    UINT    key_sym;
    char_u  vim_code0;
    char_u  vim_code1;
} special_keys[] =
{
    {VK_UP,		'k', 'u'},
    {VK_DOWN,		'k', 'd'},
    {VK_LEFT,		'k', 'l'},
    {VK_RIGHT,		'k', 'r'},

    {VK_F1,		'k', '1'},
    {VK_F2,		'k', '2'},
    {VK_F3,		'k', '3'},
    {VK_F4,		'k', '4'},
    {VK_F5,		'k', '5'},
    {VK_F6,		'k', '6'},
    {VK_F7,		'k', '7'},
    {VK_F8,		'k', '8'},
    {VK_F9,		'k', '9'},
    {VK_F10,		'k', ';'},

    {VK_F11,		'F', '1'},
    {VK_F12,		'F', '2'},
    {VK_F13,		'F', '3'},
    {VK_F14,		'F', '4'},
    {VK_F15,		'F', '5'},
    {VK_F16,		'F', '6'},
    {VK_F17,		'F', '7'},
    {VK_F18,		'F', '8'},
    {VK_F19,		'F', '9'},
    {VK_F20,		'F', 'A'},

    {VK_F21,		'F', 'B'},
#ifdef FEAT_NETBEANS_INTG
    {VK_PAUSE,		'F', 'B'},	/* Pause == F21 (see gui_gtk_x11.c) */
#endif
    {VK_F22,		'F', 'C'},
    {VK_F23,		'F', 'D'},
    {VK_F24,		'F', 'E'},	/* winuser.h defines up to F24 */

    {VK_HELP,		'%', '1'},
    {VK_BACK,		'k', 'b'},
    {VK_INSERT,		'k', 'I'},
    {VK_DELETE,		'k', 'D'},
    {VK_HOME,		'k', 'h'},
    {VK_END,		'@', '7'},
    {VK_PRIOR,		'k', 'P'},
    {VK_NEXT,		'k', 'N'},
    {VK_PRINT,		'%', '9'},
    {VK_ADD,		'K', '6'},
    {VK_SUBTRACT,	'K', '7'},
    {VK_DIVIDE,		'K', '8'},
    {VK_MULTIPLY,	'K', '9'},
    {VK_SEPARATOR,	'K', 'A'},	/* Keypad Enter */
    {VK_DECIMAL,	'K', 'B'},

    {VK_NUMPAD0,	'K', 'C'},
    {VK_NUMPAD1,	'K', 'D'},
    {VK_NUMPAD2,	'K', 'E'},
    {VK_NUMPAD3,	'K', 'F'},
    {VK_NUMPAD4,	'K', 'G'},
    {VK_NUMPAD5,	'K', 'H'},
    {VK_NUMPAD6,	'K', 'I'},
    {VK_NUMPAD7,	'K', 'J'},
    {VK_NUMPAD8,	'K', 'K'},
    {VK_NUMPAD9,	'K', 'L'},

    /* Keys that we want to be able to use any modifier with: */
    {VK_SPACE,		' ', NUL},
    {VK_TAB,		TAB, NUL},
    {VK_ESCAPE,		ESC, NUL},
    {NL,		NL, NUL},
    {CAR,		CAR, NUL},

    /* End of list marker: */
    {0,			0, 0}
};

/* Local variables */
static int		s_button_pending = -1;

/* s_getting_focus is set when we got focus but didn't see mouse-up event yet,
 * so don't reset s_button_pending. */
static int		s_getting_focus = FALSE;

static int		s_x_pending;
static int		s_y_pending;
static UINT		s_kFlags_pending;
static UINT		s_wait_timer = 0;   /* Timer for get char from user */
static int		s_timed_out = FALSE;
static int		dead_key = 0;	/* 0 - no dead key, 1 - dead key pressed */

#ifdef WIN3264
static OSVERSIONINFO os_version;    /* like it says.  Init in gui_mch_init() */
#endif

#ifdef FEAT_BEVAL
/* balloon-eval WM_NOTIFY_HANDLER */
static void Handle_WM_Notify __ARGS((HWND hwnd, LPNMHDR pnmh));
static void TrackUserActivity __ARGS((UINT uMsg));
#endif

/*
 * For control IME.
 */
#ifdef FEAT_MBYTE
# ifdef USE_IM_CONTROL
static LOGFONT norm_logfont;
# endif
#endif

#ifdef FEAT_MBYTE_IME
static LRESULT _OnImeNotify(HWND hWnd, DWORD dwCommand, DWORD dwData);
#endif

#ifdef DEBUG_PRINT_ERROR
/*
 * Print out the last Windows error message
 */
    static void
print_windows_error(void)
{
    LPVOID  lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL, GetLastError(),
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPTSTR) &lpMsgBuf, 0, NULL);
    TRACE1("Error: %s\n", lpMsgBuf);
    LocalFree(lpMsgBuf);
}
#endif

/*
 * Cursor blink functions.
 *
 * This is a simple state machine:
 * BLINK_NONE	not blinking at all
 * BLINK_OFF	blinking, cursor is not shown
 * BLINK_ON	blinking, cursor is shown
 */

#define BLINK_NONE  0
#define BLINK_OFF   1
#define BLINK_ON    2

static int		blink_state = BLINK_NONE;
static long_u		blink_waittime = 700;
static long_u		blink_ontime = 400;
static long_u		blink_offtime = 250;
static UINT		blink_timer = 0;

    void
gui_mch_set_blinking(long wait, long on, long off)
{
    blink_waittime = wait;
    blink_ontime = on;
    blink_offtime = off;
}

/* ARGSUSED */
    static VOID CALLBACK
_OnBlinkTimer(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime)
{
    MSG msg;

    /*
    TRACE2("Got timer event, id %d, blink_timer %d\n", idEvent, blink_timer);
    */

    KillTimer(NULL, idEvent);

    /* Eat spurious WM_TIMER messages */
    while (PeekMessage(&msg, hwnd, WM_TIMER, WM_TIMER, PM_REMOVE))
	;

    if (blink_state == BLINK_ON)
    {
	gui_undraw_cursor();
	blink_state = BLINK_OFF;
	blink_timer = (UINT) SetTimer(NULL, 0, (UINT)blink_offtime,
						    (TIMERPROC)_OnBlinkTimer);
    }
    else
    {
	gui_update_cursor(TRUE, FALSE);
	blink_state = BLINK_ON;
	blink_timer = (UINT) SetTimer(NULL, 0, (UINT)blink_ontime,
							 (TIMERPROC)_OnBlinkTimer);
    }
}

    static void
gui_mswin_rm_blink_timer(void)
{
    MSG msg;

    if (blink_timer != 0)
    {
	KillTimer(NULL, blink_timer);
	/* Eat spurious WM_TIMER messages */
	while (PeekMessage(&msg, s_hwnd, WM_TIMER, WM_TIMER, PM_REMOVE))
	    ;
	blink_timer = 0;
    }
}

/*
 * Stop the cursor blinking.  Show the cursor if it wasn't shown.
 */
    void
gui_mch_stop_blink(void)
{
    gui_mswin_rm_blink_timer();
    if (blink_state == BLINK_OFF)
	gui_update_cursor(TRUE, FALSE);
    blink_state = BLINK_NONE;
}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
    void
gui_mch_start_blink(void)
{
    gui_mswin_rm_blink_timer();

    /* Only switch blinking on if none of the times is zero */
    if (blink_waittime && blink_ontime && blink_offtime && gui.in_focus)
    {
	blink_timer = (UINT)SetTimer(NULL, 0, (UINT)blink_waittime,
						    (TIMERPROC)_OnBlinkTimer);
	blink_state = BLINK_ON;
	gui_update_cursor(TRUE, FALSE);
    }
}

/*
 * Call-back routines.
 */

/*ARGSUSED*/
    static VOID CALLBACK
_OnTimer(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime)
{
    MSG msg;

    /*
    TRACE2("Got timer event, id %d, s_wait_timer %d\n", idEvent, s_wait_timer);
    */
    KillTimer(NULL, idEvent);
    s_timed_out = TRUE;

    /* Eat spurious WM_TIMER messages */
    while (PeekMessage(&msg, hwnd, WM_TIMER, WM_TIMER, PM_REMOVE))
	;
    if (idEvent == s_wait_timer)
	s_wait_timer = 0;
}

/*ARGSUSED*/
    static void
_OnDeadChar(
    HWND hwnd,
    UINT ch,
    int cRepeat)
{
    dead_key = 1;
}

/*
 * Convert Unicode character "ch" to bytes in "string[slen]".
 * When "had_alt" is TRUE the ALT key was included in "ch".
 * Return the length.
 */
    static int
char_to_string(int ch, char_u *string, int slen, int had_alt)
{
    int		len;
    int		i;
#ifdef FEAT_MBYTE
    WCHAR	wstring[2];
    char_u	*ws = NULL;;

    if (os_version.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
	/* On Windows 95/98 we apparently get the character in the active
	 * codepage, not in UCS-2.  If conversion is needed convert it to
	 * UCS-2 first. */
	if ((int)GetACP() == enc_codepage)
	    len = 0;	    /* no conversion required */
	else
	{
	    string[0] = ch;
	    len = MultiByteToWideChar(GetACP(), 0, string, 1, wstring, 2);
	}
    }
    else
    {
	wstring[0] = ch;
	len = 1;
    }

    if (len > 0)
    {
	/* "ch" is a UTF-16 character.  Convert it to a string of bytes.  When
	 * "enc_codepage" is non-zero use the standard Win32 function,
	 * otherwise use our own conversion function (e.g., for UTF-8). */
	if (enc_codepage > 0)
	{
	    len = WideCharToMultiByte(enc_codepage, 0, wstring, len,
						       string, slen, 0, NULL);
	    /* If we had included the ALT key into the character but now the
	     * upper bit is no longer set, that probably means the conversion
	     * failed.  Convert the original character and set the upper bit
	     * afterwards. */
	    if (had_alt && len == 1 && ch >= 0x80 && string[0] < 0x80)
	    {
		wstring[0] = ch & 0x7f;
		len = WideCharToMultiByte(enc_codepage, 0, wstring, len,
						       string, slen, 0, NULL);
		if (len == 1) /* safety check */
		    string[0] |= 0x80;
	    }
	}
	else
	{
	    len = 1;
	    ws = utf16_to_enc(wstring, &len);
	    if (ws == NULL)
		len = 0;
	    else
	    {
		if (len > slen)	/* just in case */
		    len = slen;
		mch_memmove(string, ws, len);
		vim_free(ws);
	    }
	}
    }

    if (len == 0)
#endif
    {
	string[0] = ch;
	len = 1;
    }

    for (i = 0; i < len; ++i)
	if (string[i] == CSI && len <= slen - 2)
	{
	    /* Insert CSI as K_CSI. */
	    mch_memmove(string + i + 3, string + i + 1, len - i - 1);
	    string[++i] = KS_EXTRA;
	    string[++i] = (int)KE_CSI;
	    len += 2;
	}

    return len;
}

/*
 * Key hit, add it to the input buffer.
 */
/*ARGSUSED*/
    static void
_OnChar(
    HWND hwnd,
    UINT ch,
    int cRepeat)
{
    char_u	string[40];
    int		len = 0;

    len = char_to_string(ch, string, 40, FALSE);
    if (len == 1 && string[0] == Ctrl_C && ctrl_c_interrupts)
    {
	trash_input_buf();
	got_int = TRUE;
    }

    add_to_input_buf(string, len);
}

/*
 * Alt-Key hit, add it to the input buffer.
 */
/*ARGSUSED*/
    static void
_OnSysChar(
    HWND hwnd,
    UINT cch,
    int cRepeat)
{
    char_u	string[40]; /* Enough for multibyte character */
    int		len;
    int		modifiers;
    int		ch = cch;   /* special keys are negative */

    /* TRACE("OnSysChar(%d, %c)\n", ch, ch); */

    /* OK, we have a character key (given by ch) which was entered with the
     * ALT key pressed. Eg, if the user presses Alt-A, then ch == 'A'. Note
     * that the system distinguishes Alt-a and Alt-A (Alt-Shift-a unless
     * CAPSLOCK is pressed) at this point.
     */
    modifiers = MOD_MASK_ALT;
    if (GetKeyState(VK_SHIFT) & 0x8000)
	modifiers |= MOD_MASK_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000)
	modifiers |= MOD_MASK_CTRL;

    ch = simplify_key(ch, &modifiers);
    /* remove the SHIFT modifier for keys where it's already included, e.g.,
     * '(' and '*' */
    if (ch < 0x100 && !isalpha(ch) && isprint(ch))
	modifiers &= ~MOD_MASK_SHIFT;

    /* Interpret the ALT key as making the key META, include SHIFT, etc. */
    ch = extract_modifiers(ch, &modifiers);
    if (ch == CSI)
	ch = K_CSI;

    len = 0;
    if (modifiers)
    {
	string[len++] = CSI;
	string[len++] = KS_MODIFIER;
	string[len++] = modifiers;
    }

    if (IS_SPECIAL((int)ch))
    {
	string[len++] = CSI;
	string[len++] = K_SECOND((int)ch);
	string[len++] = K_THIRD((int)ch);
    }
    else
    {
	/* Although the documentation isn't clear about it, we assume "ch" is
	 * a Unicode character. */
	len += char_to_string(ch, string + len, 40 - len, TRUE);
    }

    add_to_input_buf(string, len);
}

    static void
_OnMouseEvent(
    int button,
    int x,
    int y,
    int repeated_click,
    UINT keyFlags)
{
    int vim_modifiers = 0x0;

    s_getting_focus = FALSE;

    if (keyFlags & MK_SHIFT)
	vim_modifiers |= MOUSE_SHIFT;
    if (keyFlags & MK_CONTROL)
	vim_modifiers |= MOUSE_CTRL;
    if (GetKeyState(VK_MENU) & 0x8000)
	vim_modifiers |= MOUSE_ALT;

    gui_send_mouse_event(button, x, y, repeated_click, vim_modifiers);
}

/*ARGSUSED*/
    static void
_OnMouseButtonDown(
    HWND hwnd,
    BOOL fDoubleClick,
    int x,
    int y,
    UINT keyFlags)
{
    static LONG	s_prevTime = 0;

    LONG    currentTime = GetMessageTime();
    int	    button = -1;
    int	    repeated_click;

    /* Give main window the focus: this is so the cursor isn't hollow. */
    (void)SetFocus(s_hwnd);

    if (s_uMsg == WM_LBUTTONDOWN || s_uMsg == WM_LBUTTONDBLCLK)
	button = MOUSE_LEFT;
    else if (s_uMsg == WM_MBUTTONDOWN || s_uMsg == WM_MBUTTONDBLCLK)
	button = MOUSE_MIDDLE;
    else if (s_uMsg == WM_RBUTTONDOWN || s_uMsg == WM_RBUTTONDBLCLK)
	button = MOUSE_RIGHT;
#ifndef WIN16 /*<VN>*/
    else if (s_uMsg == WM_XBUTTONDOWN || s_uMsg == WM_XBUTTONDBLCLK)
    {
#ifndef GET_XBUTTON_WPARAM
# define GET_XBUTTON_WPARAM(wParam)	(HIWORD(wParam))
#endif
	button = ((GET_XBUTTON_WPARAM(s_wParam) == 1) ? MOUSE_X1 : MOUSE_X2);
    }
    else if (s_uMsg == WM_CAPTURECHANGED)
    {
	/* on W95/NT4, somehow you get in here with an odd Msg
	 * if you press one button while holding down the other..*/
	if (s_button_pending == MOUSE_LEFT)
	    button = MOUSE_RIGHT;
	else
	    button = MOUSE_LEFT;
    }
#endif
    if (button >= 0)
    {
	repeated_click = ((int)(currentTime - s_prevTime) < p_mouset);

	/*
	 * Holding down the left and right buttons simulates pushing the middle
	 * button.
	 */
	if (repeated_click
		&& ((button == MOUSE_LEFT && s_button_pending == MOUSE_RIGHT)
		    || (button == MOUSE_RIGHT
					  && s_button_pending == MOUSE_LEFT)))
	{
	    /*
	     * Hmm, gui.c will ignore more than one button down at a time, so
	     * pretend we let go of it first.
	     */
	    gui_send_mouse_event(MOUSE_RELEASE, x, y, FALSE, 0x0);
	    button = MOUSE_MIDDLE;
	    repeated_click = FALSE;
	    s_button_pending = -1;
	    _OnMouseEvent(button, x, y, repeated_click, keyFlags);
	}
	else if ((repeated_click)
		|| (mouse_model_popup() && (button == MOUSE_RIGHT)))
	{
	    if (s_button_pending > -1)
	    {
		    _OnMouseEvent(s_button_pending, x, y, FALSE, keyFlags);
		    s_button_pending = -1;
	    }
	    /* TRACE("Button down at x %d, y %d\n", x, y); */
	    _OnMouseEvent(button, x, y, repeated_click, keyFlags);
	}
	else
	{
	    /*
	     * If this is the first press (i.e. not a multiple click) don't
	     * action immediately, but store and wait for:
	     * i) button-up
	     * ii) mouse move
	     * iii) another button press
	     * before using it.
	     * This enables us to make left+right simulate middle button,
	     * without left or right being actioned first.  The side-effect is
	     * that if you click and hold the mouse without dragging, the
	     * cursor doesn't move until you release the button. In practice
	     * this is hardly a problem.
	     */
	    s_button_pending = button;
	    s_x_pending = x;
	    s_y_pending = y;
	    s_kFlags_pending = keyFlags;
	}

	s_prevTime = currentTime;
    }
}

/*ARGSUSED*/
    static void
_OnMouseMoveOrRelease(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags)
{
    int button;

    s_getting_focus = FALSE;
    if (s_button_pending > -1)
    {
	/* Delayed action for mouse down event */
	_OnMouseEvent(s_button_pending, s_x_pending,
					s_y_pending, FALSE, s_kFlags_pending);
	s_button_pending = -1;
    }
    if (s_uMsg == WM_MOUSEMOVE)
    {
	/*
	 * It's only a MOUSE_DRAG if one or more mouse buttons are being held
	 * down.
	 */
	if (!(keyFlags & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON
						| MK_XBUTTON1 | MK_XBUTTON2)))
	{
	    gui_mouse_moved(x, y);
	    return;
	}

	/*
	 * While button is down, keep grabbing mouse move events when
	 * the mouse goes outside the window
	 */
	SetCapture(s_textArea);
	button = MOUSE_DRAG;
	/* TRACE("  move at x %d, y %d\n", x, y); */
    }
    else
    {
	ReleaseCapture();
	button = MOUSE_RELEASE;
	/* TRACE("  up at x %d, y %d\n", x, y); */
    }

    _OnMouseEvent(button, x, y, FALSE, keyFlags);
}

#ifdef FEAT_MENU
/*
 * Find the vimmenu_T with the given id
 */
    static vimmenu_T *
gui_mswin_find_menu(
    vimmenu_T	*pMenu,
    int		id)
{
    vimmenu_T	*pChildMenu;

    while (pMenu)
    {
	if (pMenu->id == (UINT)id)
	    break;
	if (pMenu->children != NULL)
	{
	    pChildMenu = gui_mswin_find_menu(pMenu->children, id);
	    if (pChildMenu)
	    {
		pMenu = pChildMenu;
		break;
	    }
	}
	pMenu = pMenu->next;
    }
    return pMenu;
}

/*ARGSUSED*/
    static void
_OnMenu(
    HWND	hwnd,
    int		id,
    HWND	hwndCtl,
    UINT	codeNotify)
{
    vimmenu_T	*pMenu;

    pMenu = gui_mswin_find_menu(root_menu, id);
    if (pMenu)
	gui_menu_cb(pMenu);
}
#endif

#ifdef MSWIN_FIND_REPLACE
# if defined(FEAT_MBYTE) && defined(WIN3264)
/*
 * copy useful data from structure LPFINDREPLACE to structure LPFINDREPLACEW
 */
    static void
findrep_atow(LPFINDREPLACEW lpfrw, LPFINDREPLACE lpfr)
{
    WCHAR *wp;

    lpfrw->hwndOwner = lpfr->hwndOwner;
    lpfrw->Flags = lpfr->Flags;

    wp = enc_to_utf16(lpfr->lpstrFindWhat, NULL);
    wcsncpy(lpfrw->lpstrFindWhat, wp, lpfrw->wFindWhatLen - 1);
    vim_free(wp);

    /* the field "lpstrReplaceWith" doesn't need to be copied */
}

/*
 * copy useful data from structure LPFINDREPLACEW to structure LPFINDREPLACE
 */
    static void
findrep_wtoa(LPFINDREPLACE lpfr, LPFINDREPLACEW lpfrw)
{
    char_u *p;

    lpfr->Flags = lpfrw->Flags;

    p = utf16_to_enc(lpfrw->lpstrFindWhat, NULL);
    vim_strncpy(lpfr->lpstrFindWhat, p, lpfr->wFindWhatLen - 1);
    vim_free(p);

    p = utf16_to_enc(lpfrw->lpstrReplaceWith, NULL);
    vim_strncpy(lpfr->lpstrReplaceWith, p, lpfr->wReplaceWithLen - 1);
    vim_free(p);
}
# endif

/*
 * Handle a Find/Replace window message.
 */
    static void
_OnFindRepl(void)
{
    int	    flags = 0;
    int	    down;

# if defined(FEAT_MBYTE) && defined(WIN3264)
    /* If the OS is Windows NT, and 'encoding' differs from active codepage:
     * convert text from wide string. */
    if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
			&& enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	findrep_wtoa(&s_findrep_struct, &s_findrep_struct_w);
    }
# endif

    if (s_findrep_struct.Flags & FR_DIALOGTERM)
	/* Give main window the focus back. */
	(void)SetFocus(s_hwnd);

    if (s_findrep_struct.Flags & FR_FINDNEXT)
    {
	flags = FRD_FINDNEXT;

	/* Give main window the focus back: this is so the cursor isn't
	 * hollow. */
	(void)SetFocus(s_hwnd);
    }
    else if (s_findrep_struct.Flags & FR_REPLACE)
    {
	flags = FRD_REPLACE;

	/* Give main window the focus back: this is so the cursor isn't
	 * hollow. */
	(void)SetFocus(s_hwnd);
    }
    else if (s_findrep_struct.Flags & FR_REPLACEALL)
    {
	flags = FRD_REPLACEALL;
    }

    if (flags != 0)
    {
	/* Call the generic GUI function to do the actual work. */
	if (s_findrep_struct.Flags & FR_WHOLEWORD)
	    flags |= FRD_WHOLE_WORD;
	if (s_findrep_struct.Flags & FR_MATCHCASE)
	    flags |= FRD_MATCH_CASE;
	down = (s_findrep_struct.Flags & FR_DOWN) != 0;
	gui_do_findrepl(flags, s_findrep_struct.lpstrFindWhat,
				     s_findrep_struct.lpstrReplaceWith, down);
    }
}
#endif

    static void
HandleMouseHide(UINT uMsg, LPARAM lParam)
{
    static LPARAM last_lParam = 0L;

    /* We sometimes get a mousemove when the mouse didn't move... */
    if (uMsg == WM_MOUSEMOVE)
    {
	if (lParam == last_lParam)
	    return;
	last_lParam = lParam;
    }

    /* Handle specially, to centralise coding. We need to be sure we catch all
     * possible events which should cause us to restore the cursor (as it is a
     * shared resource, we take full responsibility for it).
     */
    switch (uMsg)
    {
    case WM_KEYUP:
    case WM_CHAR:
	/*
	 * blank out the pointer if necessary
	 */
	if (p_mh)
	    gui_mch_mousehide(TRUE);
	break;

    case WM_SYSKEYUP:	 /* show the pointer when a system-key is pressed */
    case WM_SYSCHAR:
    case WM_MOUSEMOVE:	 /* show the pointer on any mouse action */
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_NCMOUSEMOVE:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_KILLFOCUS:
	/*
	 * if the pointer is currently hidden, then we should show it.
	 */
	gui_mch_mousehide(FALSE);
	break;
    }
}

    static LRESULT CALLBACK
_TextAreaWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    /*
    TRACE("TextAreaWndProc: hwnd = %08x, msg = %x, wParam = %x, lParam = %x\n",
	  hwnd, uMsg, wParam, lParam);
    */

    HandleMouseHide(uMsg, lParam);

    s_uMsg = uMsg;
    s_wParam = wParam;
    s_lParam = lParam;

#ifdef FEAT_BEVAL
    TrackUserActivity(uMsg);
#endif

    switch (uMsg)
    {
	HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_LBUTTONDOWN,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_LBUTTONUP,	_OnMouseMoveOrRelease);
	HANDLE_MSG(hwnd, WM_MBUTTONDBLCLK,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_MBUTTONDOWN,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_MBUTTONUP,	_OnMouseMoveOrRelease);
	HANDLE_MSG(hwnd, WM_MOUSEMOVE,	_OnMouseMoveOrRelease);
	HANDLE_MSG(hwnd, WM_PAINT,	_OnPaint);
	HANDLE_MSG(hwnd, WM_RBUTTONDBLCLK,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_RBUTTONDOWN,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_RBUTTONUP,	_OnMouseMoveOrRelease);
#ifndef WIN16 /*<VN>*/
	HANDLE_MSG(hwnd, WM_XBUTTONDBLCLK,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_XBUTTONDOWN,_OnMouseButtonDown);
	HANDLE_MSG(hwnd, WM_XBUTTONUP,	_OnMouseMoveOrRelease);
#endif

#ifdef FEAT_BEVAL
	case WM_NOTIFY: Handle_WM_Notify(hwnd, (LPNMHDR)lParam);
	    return TRUE;
#endif
	default:
	    return MyWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

#if (defined(WIN3264) && defined(FEAT_MBYTE)) \
	|| defined(GLOBAL_IME) \
	|| defined(PROTO)
# ifdef PROTO
typedef int WINAPI;
# endif

    LRESULT WINAPI
vim_WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
# ifdef GLOBAL_IME
    return global_ime_DefWindowProc(hwnd, message, wParam, lParam);
# else
    if (wide_WindowProc)
	return DefWindowProcW(hwnd, message, wParam, lParam);
    return DefWindowProc(hwnd, message, wParam, lParam);
#endif
}
#endif

/*
 * Called when the foreground or background color has been changed.
 */
    void
gui_mch_new_colors(void)
{
    /* nothing to do? */
}

/*
 * Set the colors to their default values.
 */
    void
gui_mch_def_colors()
{
    gui.norm_pixel = GetSysColor(COLOR_WINDOWTEXT);
    gui.back_pixel = GetSysColor(COLOR_WINDOW);
    gui.def_norm_pixel = gui.norm_pixel;
    gui.def_back_pixel = gui.back_pixel;
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open(void)
{
#ifndef SW_SHOWDEFAULT
# define SW_SHOWDEFAULT 10	/* Borland 5.0 doesn't have it */
#endif
    /* Actually open the window, if not already visible
     * (may be done already in gui_mch_set_shellsize) */
    if (!IsWindowVisible(s_hwnd))
	ShowWindow(s_hwnd, SW_SHOWDEFAULT);

#ifdef MSWIN_FIND_REPLACE
    /* Init replace string here, so that we keep it when re-opening the
     * dialog. */
    s_findrep_struct.lpstrReplaceWith[0] = NUL;
#endif

    return OK;
}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(int *x, int *y)
{
    RECT    rect;

    GetWindowRect(s_hwnd, &rect);
    *x = rect.left;
    *y = rect.top;
    return OK;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(int x, int y)
{
    SetWindowPos(s_hwnd, NULL, x, y, 0, 0,
		 SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}
    void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
    static int oldx = 0;
    static int oldy = 0;

    SetWindowPos(s_textArea, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);

#ifdef FEAT_TOOLBAR
    if (vim_strchr(p_go, GO_TOOLBAR) != NULL)
	SendMessage(s_toolbarhwnd, WM_SIZE,
	      (WPARAM)0, (LPARAM)(w + ((long)(TOOLBAR_BUTTON_HEIGHT+8)<<16)));
#endif
#if defined(FEAT_GUI_TABLINE)
    if (showing_tabline)
    {
	int	top = 0;
	RECT	rect;

# ifdef FEAT_TOOLBAR
	if (vim_strchr(p_go, GO_TOOLBAR) != NULL)
	    top = TOOLBAR_BUTTON_HEIGHT + TOOLBAR_BORDER_HEIGHT;
# endif
	GetClientRect(s_hwnd, &rect);
	MoveWindow(s_tabhwnd, 0, top, rect.right, gui.tabline_height, TRUE);
    }
#endif

    /* When side scroll bar is unshown, the size of window will change.
     * then, the text area move left or right. thus client rect should be
     * forcely redraw. (Yasuhiro Matsumoto) */
    if (oldx != x || oldy != y)
    {
	InvalidateRect(s_hwnd, NULL, FALSE);
	oldx = x;
	oldy = y;
    }
}


/*
 * Scrollbar stuff:
 */

    void
gui_mch_enable_scrollbar(
    scrollbar_T     *sb,
    int		    flag)
{
    ShowScrollBar(sb->id, SB_CTL, flag);

    /* TODO: When the window is maximized, the size of the window stays the
     * same, thus the size of the text area changes.  On Win98 it's OK, on Win
     * NT 4.0 it's not... */
}

    void
gui_mch_set_scrollbar_pos(
    scrollbar_T *sb,
    int		x,
    int		y,
    int		w,
    int		h)
{
    SetWindowPos(sb->id, NULL, x, y, w, h,
			      SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

    void
gui_mch_create_scrollbar(
    scrollbar_T *sb,
    int		orient)	/* SBAR_VERT or SBAR_HORIZ */
{
    sb->id = CreateWindow(
	"SCROLLBAR", "Scrollbar",
	WS_CHILD | ((orient == SBAR_VERT) ? SBS_VERT : SBS_HORZ), 0, 0,
	10,				/* Any value will do for now */
	10,				/* Any value will do for now */
	s_hwnd, NULL,
	s_hinst, NULL);
}

/*
 * Find the scrollbar with the given hwnd.
 */
	 static scrollbar_T *
gui_mswin_find_scrollbar(HWND hwnd)
{
    win_T	*wp;

    if (gui.bottom_sbar.id == hwnd)
	return &gui.bottom_sbar;
    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_scrollbars[SBAR_LEFT].id == hwnd)
	    return &wp->w_scrollbars[SBAR_LEFT];
	if (wp->w_scrollbars[SBAR_RIGHT].id == hwnd)
	    return &wp->w_scrollbars[SBAR_RIGHT];
    }
    return NULL;
}

/*
 * Get the character size of a font.
 */
    static void
GetFontSize(GuiFont font)
{
    HWND    hwnd = GetDesktopWindow();
    HDC	    hdc = GetWindowDC(hwnd);
    HFONT   hfntOld = SelectFont(hdc, (HFONT)font);
    TEXTMETRIC tm;

    GetTextMetrics(hdc, &tm);
    gui.char_width = tm.tmAveCharWidth + tm.tmOverhang;

    gui.char_height = tm.tmHeight
#ifndef MSWIN16_FASTTEXT
				+ p_linespace
#endif
				;

    SelectFont(hdc, hfntOld);

    ReleaseDC(hwnd, hdc);
}

/*
 * Adjust gui.char_height (after 'linespace' was changed).
 */
    int
gui_mch_adjust_charheight(void)
{
    GetFontSize(gui.norm_font);
    return OK;
}

    static GuiFont
get_font_handle(LOGFONT *lf)
{
    HFONT   font = NULL;

    /* Load the font */
    font = CreateFontIndirect(lf);

    if (font == NULL)
	return NOFONT;

    return (GuiFont)font;
}

    static int
pixels_to_points(int pixels, int vertical)
{
    int		points;
    HWND	hwnd;
    HDC		hdc;

    hwnd = GetDesktopWindow();
    hdc = GetWindowDC(hwnd);

    points = MulDiv(pixels, 72,
		    GetDeviceCaps(hdc, vertical ? LOGPIXELSY : LOGPIXELSX));

    ReleaseDC(hwnd, hdc);

    return points;
}

    GuiFont
gui_mch_get_font(
    char_u	*name,
    int		giveErrorIfMissing)
{
    LOGFONT	lf;
    GuiFont	font = NOFONT;

    if (get_logfont(&lf, name, NULL, giveErrorIfMissing) == OK)
	font = get_font_handle(&lf);
    if (font == NOFONT && giveErrorIfMissing)
	EMSG2(_(e_font), name);
    return font;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return the name of font "font" in allocated memory.
 * Don't know how to get the actual name, thus use the provided name.
 */
/*ARGSUSED*/
    char_u *
gui_mch_get_fontname(font, name)
    GuiFont font;
    char_u  *name;
{
    if (name == NULL)
	return NULL;
    return vim_strsave(name);
}
#endif

    void
gui_mch_free_font(GuiFont font)
{
    if (font)
	DeleteObject((HFONT)font);
}

    static int
hex_digit(int c)
{
    if (VIM_ISDIGIT(c))
	return c - '0';
    c = TOLOWER_ASC(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 10;
    return -1000;
}
/*
 * Return the Pixel value (color) for the given color name.
 * Return INVALCOLOR for error.
 */
    guicolor_T
gui_mch_get_color(char_u *name)
{
    typedef struct guicolor_tTable
    {
	char	    *name;
	COLORREF    color;
    } guicolor_tTable;

    static guicolor_tTable table[] =
    {
	{"Black",		RGB(0x00, 0x00, 0x00)},
	{"DarkGray",		RGB(0x80, 0x80, 0x80)},
	{"DarkGrey",		RGB(0x80, 0x80, 0x80)},
	{"Gray",		RGB(0xC0, 0xC0, 0xC0)},
	{"Grey",		RGB(0xC0, 0xC0, 0xC0)},
	{"LightGray",		RGB(0xE0, 0xE0, 0xE0)},
	{"LightGrey",		RGB(0xE0, 0xE0, 0xE0)},
	{"Gray10",		RGB(0x1A, 0x1A, 0x1A)},
	{"Grey10",		RGB(0x1A, 0x1A, 0x1A)},
	{"Gray20",		RGB(0x33, 0x33, 0x33)},
	{"Grey20",		RGB(0x33, 0x33, 0x33)},
	{"Gray30",		RGB(0x4D, 0x4D, 0x4D)},
	{"Grey30",		RGB(0x4D, 0x4D, 0x4D)},
	{"Gray40",		RGB(0x66, 0x66, 0x66)},
	{"Grey40",		RGB(0x66, 0x66, 0x66)},
	{"Gray50",		RGB(0x7F, 0x7F, 0x7F)},
	{"Grey50",		RGB(0x7F, 0x7F, 0x7F)},
	{"Gray60",		RGB(0x99, 0x99, 0x99)},
	{"Grey60",		RGB(0x99, 0x99, 0x99)},
	{"Gray70",		RGB(0xB3, 0xB3, 0xB3)},
	{"Grey70",		RGB(0xB3, 0xB3, 0xB3)},
	{"Gray80",		RGB(0xCC, 0xCC, 0xCC)},
	{"Grey80",		RGB(0xCC, 0xCC, 0xCC)},
	{"Gray90",		RGB(0xE5, 0xE5, 0xE5)},
	{"Grey90",		RGB(0xE5, 0xE5, 0xE5)},
	{"White",		RGB(0xFF, 0xFF, 0xFF)},
	{"DarkRed",		RGB(0x80, 0x00, 0x00)},
	{"Red",			RGB(0xFF, 0x00, 0x00)},
	{"LightRed",		RGB(0xFF, 0xA0, 0xA0)},
	{"DarkBlue",		RGB(0x00, 0x00, 0x80)},
	{"Blue",		RGB(0x00, 0x00, 0xFF)},
	{"LightBlue",		RGB(0xA0, 0xA0, 0xFF)},
	{"DarkGreen",		RGB(0x00, 0x80, 0x00)},
	{"Green",		RGB(0x00, 0xFF, 0x00)},
	{"LightGreen",		RGB(0xA0, 0xFF, 0xA0)},
	{"DarkCyan",		RGB(0x00, 0x80, 0x80)},
	{"Cyan",		RGB(0x00, 0xFF, 0xFF)},
	{"LightCyan",		RGB(0xA0, 0xFF, 0xFF)},
	{"DarkMagenta",		RGB(0x80, 0x00, 0x80)},
	{"Magenta",		RGB(0xFF, 0x00, 0xFF)},
	{"LightMagenta",	RGB(0xFF, 0xA0, 0xFF)},
	{"Brown",		RGB(0x80, 0x40, 0x40)},
	{"Yellow",		RGB(0xFF, 0xFF, 0x00)},
	{"LightYellow",		RGB(0xFF, 0xFF, 0xA0)},
	{"DarkYellow",		RGB(0xBB, 0xBB, 0x00)},
	{"SeaGreen",		RGB(0x2E, 0x8B, 0x57)},
	{"Orange",		RGB(0xFF, 0xA5, 0x00)},
	{"Purple",		RGB(0xA0, 0x20, 0xF0)},
	{"SlateBlue",		RGB(0x6A, 0x5A, 0xCD)},
	{"Violet",		RGB(0xEE, 0x82, 0xEE)},
    };

    typedef struct SysColorTable
    {
	char	    *name;
	int	    color;
    } SysColorTable;

    static SysColorTable sys_table[] =
    {
#ifdef WIN3264
	{"SYS_3DDKSHADOW", COLOR_3DDKSHADOW},
	{"SYS_3DHILIGHT", COLOR_3DHILIGHT},
#ifndef __MINGW32__
	{"SYS_3DHIGHLIGHT", COLOR_3DHIGHLIGHT},
#endif
	{"SYS_BTNHILIGHT", COLOR_BTNHILIGHT},
	{"SYS_BTNHIGHLIGHT", COLOR_BTNHIGHLIGHT},
	{"SYS_3DLIGHT", COLOR_3DLIGHT},
	{"SYS_3DSHADOW", COLOR_3DSHADOW},
	{"SYS_DESKTOP", COLOR_DESKTOP},
	{"SYS_INFOBK", COLOR_INFOBK},
	{"SYS_INFOTEXT", COLOR_INFOTEXT},
	{"SYS_3DFACE", COLOR_3DFACE},
#endif
	{"SYS_BTNFACE", COLOR_BTNFACE},
	{"SYS_BTNSHADOW", COLOR_BTNSHADOW},
	{"SYS_ACTIVEBORDER", COLOR_ACTIVEBORDER},
	{"SYS_ACTIVECAPTION", COLOR_ACTIVECAPTION},
	{"SYS_APPWORKSPACE", COLOR_APPWORKSPACE},
	{"SYS_BACKGROUND", COLOR_BACKGROUND},
	{"SYS_BTNTEXT", COLOR_BTNTEXT},
	{"SYS_CAPTIONTEXT", COLOR_CAPTIONTEXT},
	{"SYS_GRAYTEXT", COLOR_GRAYTEXT},
	{"SYS_HIGHLIGHT", COLOR_HIGHLIGHT},
	{"SYS_HIGHLIGHTTEXT", COLOR_HIGHLIGHTTEXT},
	{"SYS_INACTIVEBORDER", COLOR_INACTIVEBORDER},
	{"SYS_INACTIVECAPTION", COLOR_INACTIVECAPTION},
	{"SYS_INACTIVECAPTIONTEXT", COLOR_INACTIVECAPTIONTEXT},
	{"SYS_MENU", COLOR_MENU},
	{"SYS_MENUTEXT", COLOR_MENUTEXT},
	{"SYS_SCROLLBAR", COLOR_SCROLLBAR},
	{"SYS_WINDOW", COLOR_WINDOW},
	{"SYS_WINDOWFRAME", COLOR_WINDOWFRAME},
	{"SYS_WINDOWTEXT", COLOR_WINDOWTEXT}
    };

    int		    r, g, b;
    int		    i;

    if (name[0] == '#' && strlen(name) == 7)
    {
	/* Name is in "#rrggbb" format */
	r = hex_digit(name[1]) * 16 + hex_digit(name[2]);
	g = hex_digit(name[3]) * 16 + hex_digit(name[4]);
	b = hex_digit(name[5]) * 16 + hex_digit(name[6]);
	if (r < 0 || g < 0 || b < 0)
	    return INVALCOLOR;
	return RGB(r, g, b);
    }
    else
    {
	/* Check if the name is one of the colors we know */
	for (i = 0; i < sizeof(table) / sizeof(table[0]); i++)
	    if (STRICMP(name, table[i].name) == 0)
		return table[i].color;
    }

    /*
     * Try to look up a system colour.
     */
    for (i = 0; i < sizeof(sys_table) / sizeof(sys_table[0]); i++)
	if (STRICMP(name, sys_table[i].name) == 0)
	    return GetSysColor(sys_table[i].color);

    /*
     * Last attempt. Look in the file "$VIMRUNTIME/rgb.txt".
     */
    {
#define LINE_LEN 100
	FILE	*fd;
	char	line[LINE_LEN];
	char_u	*fname;

	fname = expand_env_save((char_u *)"$VIMRUNTIME/rgb.txt");
	if (fname == NULL)
	    return INVALCOLOR;

	fd = mch_fopen((char *)fname, "rt");
	vim_free(fname);
	if (fd == NULL)
	    return INVALCOLOR;

	while (!feof(fd))
	{
	    int	    len;
	    int	    pos;
	    char    *color;

	    fgets(line, LINE_LEN, fd);
	    len = (int)STRLEN(line);

	    if (len <= 1 || line[len-1] != '\n')
		continue;

	    line[len-1] = '\0';

	    i = sscanf(line, "%d %d %d %n", &r, &g, &b, &pos);
	    if (i != 3)
		continue;

	    color = line + pos;

	    if (STRICMP(color, name) == 0)
	    {
		fclose(fd);
		return (guicolor_T) RGB(r, g, b);
	    }
	}

	fclose(fd);
    }

    return INVALCOLOR;
}
/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(char_u *name)
{
    int i;

    for (i = 0; special_keys[i].vim_code1 != NUL; i++)
	if (name[0] == special_keys[i].vim_code0 &&
					 name[1] == special_keys[i].vim_code1)
	    return OK;
    return FAIL;
}

    void
gui_mch_beep(void)
{
    MessageBeep(MB_OK);
}
/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
    void
gui_mch_invert_rectangle(
    int	    r,
    int	    c,
    int	    nr,
    int	    nc)
{
    RECT    rc;

    /*
     * Note: InvertRect() excludes right and bottom of rectangle.
     */
    rc.left = FILL_X(c);
    rc.top = FILL_Y(r);
    rc.right = rc.left + nc * gui.char_width;
    rc.bottom = rc.top + nr * gui.char_height;
    InvertRect(s_hdc, &rc);
}

/*
 * Iconify the GUI window.
 */
    void
gui_mch_iconify(void)
{
    ShowWindow(s_hwnd, SW_MINIMIZE);
}

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
    HBRUSH  hbr;
    RECT    rc;

    /*
     * Note: FrameRect() excludes right and bottom of rectangle.
     */
    rc.left = FILL_X(gui.col);
    rc.top = FILL_Y(gui.row);
    rc.right = rc.left + gui.char_width;
#ifdef FEAT_MBYTE
    if (mb_lefthalve(gui.row, gui.col))
	rc.right += gui.char_width;
#endif
    rc.bottom = rc.top + gui.char_height;
    hbr = CreateSolidBrush(color);
    FrameRect(s_hdc, &rc, hbr);
    DeleteBrush(hbr);
}
/*
 * Draw part of a cursor, "w" pixels wide, and "h" pixels high, using
 * color "color".
 */
    void
gui_mch_draw_part_cursor(
    int		w,
    int		h,
    guicolor_T	color)
{
    HBRUSH	hbr;
    RECT	rc;

    /*
     * Note: FillRect() excludes right and bottom of rectangle.
     */
    rc.left =
#ifdef FEAT_RIGHTLEFT
		/* vertical line should be on the right of current point */
		CURSOR_BAR_RIGHT ? FILL_X(gui.col + 1) - w :
#endif
		    FILL_X(gui.col);
    rc.top = FILL_Y(gui.row) + gui.char_height - h;
    rc.right = rc.left + w;
    rc.bottom = rc.top + h;
    hbr = CreateSolidBrush(color);
    FillRect(s_hdc, &rc, hbr);
    DeleteBrush(hbr);
}

/*
 * Process a single Windows message.
 * If one is not available we hang until one is.
 */
    static void
process_message(void)
{
    MSG		msg;
    UINT	vk = 0;		/* Virtual key */
    char_u	string[40];
    int		i;
    int		modifiers = 0;
    int		key;
#ifdef FEAT_MENU
    static char_u k10[] = {K_SPECIAL, 'k', ';', 0};
#endif

    GetMessage(&msg, NULL, 0, 0);

#ifdef FEAT_OLE
    /* Look after OLE Automation commands */
    if (msg.message == WM_OLE)
    {
	char_u *str = (char_u *)msg.lParam;
	if (str == NULL || *str == NUL)
	{
	    /* Message can't be ours, forward it.  Fixes problem with Ultramon
	     * 3.0.4 */
	    DispatchMessage(&msg);
	}
	else
	{
	    add_to_input_buf(str, (int)STRLEN(str));
	    vim_free(str);  /* was allocated in CVim::SendKeys() */
	}
	return;
    }
#endif

#ifdef FEAT_NETBEANS_INTG
    if (msg.message == WM_NETBEANS)
    {
	netbeans_read();
	return;
    }
#endif

#ifdef FEAT_SNIFF
    if (sniff_request_waiting && want_sniff_request)
    {
	static char_u bytes[3] = {CSI, (char_u)KS_EXTRA, (char_u)KE_SNIFF};
	add_to_input_buf(bytes, 3); /* K_SNIFF */
	sniff_request_waiting = 0;
	want_sniff_request = 0;
	/* request is handled in normal.c */
    }
    if (msg.message == WM_USER)
    {
	MyTranslateMessage(&msg);
	DispatchMessage(&msg);
	return;
    }
#endif

#ifdef MSWIN_FIND_REPLACE
    /* Don't process messages used by the dialog */
    if (s_findrep_hwnd != NULL && IsDialogMessage(s_findrep_hwnd, &msg))
    {
	HandleMouseHide(msg.message, msg.lParam);
	return;
    }
#endif

    /*
     * Check if it's a special key that we recognise.  If not, call
     * TranslateMessage().
     */
    if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
    {
	vk = (int) msg.wParam;
	/* handle key after dead key, but ignore shift, alt and control */
	if (dead_key && vk != VK_SHIFT && vk != VK_MENU && vk != VK_CONTROL)
	{
	    dead_key = 0;
	    /* handle non-alphabetic keys (ones that hopefully cannot generate
	     * umlaut-characters), unless when control is down */
	    if (vk < 'A' || vk > 'Z' || (GetKeyState(VK_CONTROL) & 0x8000))
	    {
		MSG dm;

		dm.message = msg.message;
		dm.hwnd = msg.hwnd;
		dm.wParam = VK_SPACE;
		MyTranslateMessage(&dm);	/* generate dead character */
		if (vk != VK_SPACE) /* and send current character once more */
		    PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
		return;
	    }
	}

	/* Check for CTRL-BREAK */
	if (vk == VK_CANCEL)
	{
	    trash_input_buf();
	    got_int = TRUE;
	    string[0] = Ctrl_C;
	    add_to_input_buf(string, 1);
	}

	for (i = 0; special_keys[i].key_sym != 0; i++)
	{
	    /* ignore VK_SPACE when ALT key pressed: system menu */
	    if (special_keys[i].key_sym == vk
		    && (vk != VK_SPACE || !(GetKeyState(VK_MENU) & 0x8000)))
	    {
#ifdef FEAT_MENU
		/* Check for <F10>: Windows selects the menu.  When <F10> is
		 * mapped we want to use the mapping instead. */
		if (vk == VK_F10
			&& gui.menu_is_active
			&& check_map(k10, State, FALSE, TRUE, FALSE) == NULL)
		    break;
#endif
		if (GetKeyState(VK_SHIFT) & 0x8000)
		    modifiers |= MOD_MASK_SHIFT;
		/*
		 * Don't use caps-lock as shift, because these are special keys
		 * being considered here, and we only want letters to get
		 * shifted -- webb
		 */
		/*
		if (GetKeyState(VK_CAPITAL) & 0x0001)
		    modifiers ^= MOD_MASK_SHIFT;
		*/
		if (GetKeyState(VK_CONTROL) & 0x8000)
		    modifiers |= MOD_MASK_CTRL;
		if (GetKeyState(VK_MENU) & 0x8000)
		    modifiers |= MOD_MASK_ALT;

		if (special_keys[i].vim_code1 == NUL)
		    key = special_keys[i].vim_code0;
		else
		    key = TO_SPECIAL(special_keys[i].vim_code0,
						   special_keys[i].vim_code1);
		key = simplify_key(key, &modifiers);
		if (key == CSI)
		    key = K_CSI;

		if (modifiers)
		{
		    string[0] = CSI;
		    string[1] = KS_MODIFIER;
		    string[2] = modifiers;
		    add_to_input_buf(string, 3);
		}

		if (IS_SPECIAL(key))
		{
		    string[0] = CSI;
		    string[1] = K_SECOND(key);
		    string[2] = K_THIRD(key);
		    add_to_input_buf(string, 3);
		}
		else
		{
		    int	len;

		    /* Handle "key" as a Unicode character. */
		    len = char_to_string(key, string, 40, FALSE);
		    add_to_input_buf(string, len);
		}
		break;
	    }
	}
	if (special_keys[i].key_sym == 0)
	{
	    /* Some keys need C-S- where they should only need C-.
	     * Ignore 0xff, Windows XP sends it when NUMLOCK has changed since
	     * system startup (Helmut Stiegler, 2003 Oct 3). */
	    if (vk != 0xff
		    && (GetKeyState(VK_CONTROL) & 0x8000)
		    && !(GetKeyState(VK_SHIFT) & 0x8000)
		    && !(GetKeyState(VK_MENU) & 0x8000))
	    {
		/* CTRL-6 is '^'; Japanese keyboard maps '^' to vk == 0xDE */
		if (vk == '6' || MapVirtualKey(vk, 2) == (UINT)'^')
		{
		    string[0] = Ctrl_HAT;
		    add_to_input_buf(string, 1);
		}
		/* vk == 0xBD AZERTY for CTRL-'-', but CTRL-[ for * QWERTY! */
		else if (vk == 0xBD)	/* QWERTY for CTRL-'-' */
		{
		    string[0] = Ctrl__;
		    add_to_input_buf(string, 1);
		}
		/* CTRL-2 is '@'; Japanese keyboard maps '@' to vk == 0xC0 */
		else if (vk == '2' || MapVirtualKey(vk, 2) == (UINT)'@')
		{
		    string[0] = Ctrl_AT;
		    add_to_input_buf(string, 1);
		}
		else
		    MyTranslateMessage(&msg);
	    }
	    else
		MyTranslateMessage(&msg);
	}
    }
#ifdef FEAT_MBYTE_IME
    else if (msg.message == WM_IME_NOTIFY)
	_OnImeNotify(msg.hwnd, (DWORD)msg.wParam, (DWORD)msg.lParam);
    else if (msg.message == WM_KEYUP && im_get_status())
	/* added for non-MS IME (Yasuhiro Matsumoto) */
	MyTranslateMessage(&msg);
#endif
#if !defined(FEAT_MBYTE_IME) && defined(GLOBAL_IME)
/* GIME_TEST */
    else if (msg.message == WM_IME_STARTCOMPOSITION)
    {
	POINT point;

	global_ime_set_font(&norm_logfont);
	point.x = FILL_X(gui.col);
	point.y = FILL_Y(gui.row);
	MapWindowPoints(s_textArea, s_hwnd, &point, 1);
	global_ime_set_position(&point);
    }
#endif

#ifdef FEAT_MENU
    /* Check for <F10>: Default effect is to select the menu.  When <F10> is
     * mapped we need to stop it here to avoid strange effects (e.g., for the
     * key-up event) */
    if (vk != VK_F10 || check_map(k10, State, FALSE, TRUE, FALSE) == NULL)
#endif
	DispatchMessage(&msg);
}

/*
 * Catch up with any queued events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the event queue (& no timers pending), then we return
 * immediately.
 */
    void
gui_mch_update(void)
{
    MSG	    msg;

    if (!s_busy_processing)
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)
						  && !vim_is_input_buf_full())
	    process_message();
}

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1	    Wait forever.
 *  wtime == 0	    This should never happen.
 *  wtime > 0	    Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
    int
gui_mch_wait_for_chars(int wtime)
{
    MSG		msg;
    int		focus;

    s_timed_out = FALSE;

    if (wtime > 0)
    {
	/* Don't do anything while processing a (scroll) message. */
	if (s_busy_processing)
	    return FAIL;
	s_wait_timer = (UINT)SetTimer(NULL, 0, (UINT)wtime,
							 (TIMERPROC)_OnTimer);
    }

    allow_scrollbar = TRUE;

    focus = gui.in_focus;
    while (!s_timed_out)
    {
	/* Stop or start blinking when focus changes */
	if (gui.in_focus != focus)
	{
	    if (gui.in_focus)
		gui_mch_start_blink();
	    else
		gui_mch_stop_blink();
	    focus = gui.in_focus;
	}

	if (s_need_activate)
	{
#ifdef WIN32
	    (void)SetForegroundWindow(s_hwnd);
#else
	    (void)SetActiveWindow(s_hwnd);
#endif
	    s_need_activate = FALSE;
	}

#ifdef FEAT_NETBEANS_INTG
	/* Process the queued netbeans messages. */
	netbeans_parse_messages();
#endif

	/*
	 * Don't use gui_mch_update() because then we will spin-lock until a
	 * char arrives, instead we use GetMessage() to hang until an
	 * event arrives.  No need to check for input_buf_full because we are
	 * returning as soon as it contains a single char -- webb
	 */
	process_message();

	if (input_available())
	{
	    if (s_wait_timer != 0 && !s_timed_out)
	    {
		KillTimer(NULL, s_wait_timer);

		/* Eat spurious WM_TIMER messages */
		while (PeekMessage(&msg, s_hwnd, WM_TIMER, WM_TIMER, PM_REMOVE))
		    ;
		s_wait_timer = 0;
	    }
	    allow_scrollbar = FALSE;

	    /* Clear pending mouse button, the release event may have been
	     * taken by the dialog window.  But don't do this when getting
	     * focus, we need the mouse-up event then. */
	    if (!s_getting_focus)
		s_button_pending = -1;

	    return OK;
	}
    }
    allow_scrollbar = FALSE;
    return FAIL;
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(
    int		row1,
    int		col1,
    int		row2,
    int		col2)
{
    RECT	rc;

    /*
     * Clear one extra pixel at the far right, for when bold characters have
     * spilled over to the window border.
     * Note: FillRect() excludes right and bottom of rectangle.
     */
    rc.left = FILL_X(col1);
    rc.top = FILL_Y(row1);
    rc.right = FILL_X(col2 + 1) + (col2 == Columns - 1);
    rc.bottom = FILL_Y(row2 + 1);
    clear_rect(&rc);
}

/*
 * Clear the whole text window.
 */
    void
gui_mch_clear_all(void)
{
    RECT    rc;

    rc.left = 0;
    rc.top = 0;
    rc.right = Columns * gui.char_width + 2 * gui.border_width;
    rc.bottom = Rows * gui.char_height + 2 * gui.border_width;
    clear_rect(&rc);
}
/*
 * Menu stuff.
 */

    void
gui_mch_enable_menu(int flag)
{
#ifdef FEAT_MENU
    SetMenu(s_hwnd, flag ? s_menuBar : NULL);
#endif
}

/*ARGSUSED*/
    void
gui_mch_set_menu_pos(
    int	    x,
    int	    y,
    int	    w,
    int	    h)
{
    /* It will be in the right place anyway */
}

#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Make menu item hidden or not hidden
 */
    void
gui_mch_menu_hidden(
    vimmenu_T	*menu,
    int		hidden)
{
    /*
     * This doesn't do what we want.  Hmm, just grey the menu items for now.
     */
    /*
    if (hidden)
	EnableMenuItem(s_menuBar, menu->id, MF_BYCOMMAND | MF_DISABLED);
    else
	EnableMenuItem(s_menuBar, menu->id, MF_BYCOMMAND | MF_ENABLED);
    */
    gui_mch_menu_grey(menu, hidden);
}

/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar(void)
{
    DrawMenuBar(s_hwnd);
}
#endif /*FEAT_MENU*/

#ifndef PROTO
void
#ifdef VIMDLL
_export
#endif
_cdecl
SaveInst(HINSTANCE hInst)
{
    s_hinst = hInst;
}
#endif

/*
 * Return the RGB value of a pixel as a long.
 */
    long_u
gui_mch_get_rgb(guicolor_T pixel)
{
    return (GetRValue(pixel) << 16) + (GetGValue(pixel) << 8)
							   + GetBValue(pixel);
}

#if defined(FEAT_GUI_DIALOG) || defined(PROTO)
/* Convert pixels in X to dialog units */
    static WORD
PixelToDialogX(int numPixels)
{
    return (WORD)((numPixels * 4) / s_dlgfntwidth);
}

/* Convert pixels in Y to dialog units */
    static WORD
PixelToDialogY(int numPixels)
{
    return (WORD)((numPixels * 8) / s_dlgfntheight);
}

/* Return the width in pixels of the given text in the given DC. */
    static int
GetTextWidth(HDC hdc, char_u *str, int len)
{
    SIZE    size;

    GetTextExtentPoint(hdc, str, len, &size);
    return size.cx;
}

#ifdef FEAT_MBYTE
/*
 * Return the width in pixels of the given text in the given DC, taking care
 * of 'encoding' to active codepage conversion.
 */
    static int
GetTextWidthEnc(HDC hdc, char_u *str, int len)
{
    SIZE	size;
    WCHAR	*wstr;
    int		n;
    int		wlen = len;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	/* 'encoding' differs from active codepage: convert text and use wide
	 * function */
	wstr = enc_to_utf16(str, &wlen);
	if (wstr != NULL)
	{
	    n = GetTextExtentPointW(hdc, wstr, wlen, &size);
	    vim_free(wstr);
	    if (n)
		return size.cx;
	}
    }

    return GetTextWidth(hdc, str, len);
}
#else
# define GetTextWidthEnc(h, s, l) GetTextWidth((h), (s), (l))
#endif

/*
 * A quick little routine that will center one window over another, handy for
 * dialog boxes.  Taken from the Win32SDK samples.
 */
    static BOOL
CenterWindow(
    HWND hwndChild,
    HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

    GetWindowRect(hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    /* If Vim is minimized put the window in the middle of the screen. */
    if (hwndParent == NULL || IsMinimized(hwndParent))
    {
#ifdef WIN16
	rParent.left = 0;
	rParent.top = 0;
	rParent.right = GetSystemMetrics(SM_CXSCREEN);
	rParent.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
#else
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rParent, 0);
#endif
    }
    else
	GetWindowRect(hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    hdc = GetDC(hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC(hwndChild, hdc);

    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0)
    {
	xNew = 0;
    }
    else if ((xNew+wChild) > wScreen)
    {
	xNew = wScreen - wChild;
    }

    yNew = rParent.top	+ ((hParent - hChild) /2);
    if (yNew < 0)
	yNew = 0;
    else if ((yNew+hChild) > hScreen)
	yNew = hScreen - hChild;

    return SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0,
						   SWP_NOSIZE | SWP_NOZORDER);
}
#endif /* FEAT_GUI_DIALOG */

void
gui_mch_activate_window(void)
{
    (void)SetActiveWindow(s_hwnd);
}

#if defined(FEAT_TOOLBAR) || defined(PROTO)
    void
gui_mch_show_toolbar(int showit)
{
    if (s_toolbarhwnd == NULL)
	return;

    if (showit)
    {
# ifdef FEAT_MBYTE
#  ifndef TB_SETUNICODEFORMAT
    /* For older compilers.  We assume this never changes. */
#   define TB_SETUNICODEFORMAT 0x2005
#  endif
	/* Enable/disable unicode support */
	int uu = (enc_codepage >= 0 && (int)GetACP() != enc_codepage);
	SendMessage(s_toolbarhwnd, TB_SETUNICODEFORMAT, (WPARAM)uu, (LPARAM)0);
# endif
	ShowWindow(s_toolbarhwnd, SW_SHOW);
    }
    else
	ShowWindow(s_toolbarhwnd, SW_HIDE);
}

/* Then number of bitmaps is fixed.  Exit is missing! */
#define TOOLBAR_BITMAP_COUNT 31

#endif

#if defined(FEAT_GUI_TABLINE) || defined(PROTO)
    static void
add_tabline_popup_menu_entry(HMENU pmenu, UINT item_id, char_u *item_text)
{
#ifdef FEAT_MBYTE
    WCHAR	*wn = NULL;
    int		n;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	/* 'encoding' differs from active codepage: convert menu name
	 * and use wide function */
	wn = enc_to_utf16(item_text, NULL);
	if (wn != NULL)
	{
	    MENUITEMINFOW	infow;

	    infow.cbSize = sizeof(infow);
	    infow.fMask = MIIM_TYPE | MIIM_ID;
	    infow.wID = item_id;
	    infow.fType = MFT_STRING;
	    infow.dwTypeData = wn;
	    infow.cch = (UINT)wcslen(wn);
	    n = InsertMenuItemW(pmenu, item_id, FALSE, &infow);
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
	info.fMask = MIIM_TYPE | MIIM_ID;
	info.wID = item_id;
	info.fType = MFT_STRING;
	info.dwTypeData = item_text;
	info.cch = (UINT)STRLEN(item_text);
	InsertMenuItem(pmenu, item_id, FALSE, &info);
    }
}

    static void
show_tabline_popup_menu(void)
{
    HMENU	    tab_pmenu;
    long	    rval;
    POINT	    pt;

    /* When ignoring events don't show the menu. */
    if (hold_gui_events
# ifdef FEAT_CMDWIN
	    || cmdwin_type != 0
# endif
       )
	return;

    tab_pmenu = CreatePopupMenu();
    if (tab_pmenu == NULL)
	return;

    add_tabline_popup_menu_entry(tab_pmenu, TABLINE_MENU_CLOSE, _("Close tab"));
    add_tabline_popup_menu_entry(tab_pmenu, TABLINE_MENU_NEW, _("New tab"));
    add_tabline_popup_menu_entry(tab_pmenu, TABLINE_MENU_OPEN,
				 _("Open tab..."));

    GetCursorPos(&pt);
    rval = TrackPopupMenuEx(tab_pmenu, TPM_RETURNCMD, pt.x, pt.y, s_tabhwnd,
									NULL);

    DestroyMenu(tab_pmenu);

    /* Add the string cmd into input buffer */
    if (rval > 0)
    {
	TCHITTESTINFO htinfo;
	int idx;

	if (ScreenToClient(s_tabhwnd, &pt) == 0)
	    return;

	htinfo.pt.x = pt.x;
	htinfo.pt.y = pt.y;
	idx = TabCtrl_HitTest(s_tabhwnd, &htinfo);
	if (idx == -1)
	    idx = 0;
	else
	    idx += 1;

	send_tabline_menu_event(idx, (int)rval);
    }
}

/*
 * Show or hide the tabline.
 */
    void
gui_mch_show_tabline(int showit)
{
    if (s_tabhwnd == NULL)
	return;

    if (!showit != !showing_tabline)
    {
	if (showit)
	    ShowWindow(s_tabhwnd, SW_SHOW);
	else
	    ShowWindow(s_tabhwnd, SW_HIDE);
	showing_tabline = showit;
    }
}

/*
 * Return TRUE when tabline is displayed.
 */
    int
gui_mch_showing_tabline(void)
{
    return s_tabhwnd != NULL && showing_tabline;
}

/*
 * Update the labels of the tabline.
 */
    void
gui_mch_update_tabline(void)
{
    tabpage_T	*tp;
    TCITEM	tie;
    int		nr = 0;
    int		curtabidx = 0;
    RECT	rc;
#ifdef FEAT_MBYTE
    static int	use_unicode = FALSE;
    int		uu;
    WCHAR	*wstr = NULL;
#endif

    if (s_tabhwnd == NULL)
	return;

#if defined(FEAT_MBYTE)
# ifndef CCM_SETUNICODEFORMAT
    /* For older compilers.  We assume this never changes. */
#  define CCM_SETUNICODEFORMAT 0x2005
# endif
    uu = (enc_codepage >= 0 && (int)GetACP() != enc_codepage);
    if (uu != use_unicode)
    {
	/* Enable/disable unicode support */
	SendMessage(s_tabhwnd, CCM_SETUNICODEFORMAT, (WPARAM)uu, (LPARAM)0);
	use_unicode = uu;
    }
#endif

    tie.mask = TCIF_TEXT;
    tie.iImage = -1;

    /* Add a label for each tab page.  They all contain the same text area. */
    for (tp = first_tabpage; tp != NULL; tp = tp->tp_next, ++nr)
    {
	if (tp == curtab)
	    curtabidx = nr;

	if (!TabCtrl_GetItemRect(s_tabhwnd, nr, &rc))
	{
	    /* Add the tab */
	    tie.pszText = "-Empty-";
	    TabCtrl_InsertItem(s_tabhwnd, nr, &tie);
	}

	get_tabline_label(tp, FALSE);
	tie.pszText = NameBuff;
#ifdef FEAT_MBYTE
	wstr = NULL;
	if (use_unicode)
	{
	    /* Need to go through Unicode. */
	    wstr = enc_to_utf16(NameBuff, NULL);
	    if (wstr != NULL)
	    {
		TCITEMW		tiw;

		tiw.mask = TCIF_TEXT;
		tiw.iImage = -1;
		tiw.pszText = wstr;
		SendMessage(s_tabhwnd, TCM_SETITEMW, (WPARAM)nr, (LPARAM)&tiw);
		vim_free(wstr);
	    }
	}
	if (wstr == NULL)
#endif
	{
	    TabCtrl_SetItem(s_tabhwnd, nr, &tie);
	}
    }

    /* Remove any old labels. */
    while (TabCtrl_GetItemRect(s_tabhwnd, nr, &rc))
	TabCtrl_DeleteItem(s_tabhwnd, nr);

    if (TabCtrl_GetCurSel(s_tabhwnd) != curtabidx)
	TabCtrl_SetCurSel(s_tabhwnd, curtabidx);
}

/*
 * Set the current tab to "nr".  First tab is 1.
 */
    void
gui_mch_set_curtab(nr)
    int		nr;
{
    if (s_tabhwnd == NULL)
	return;

    if (TabCtrl_GetCurSel(s_tabhwnd) != nr -1)
	TabCtrl_SetCurSel(s_tabhwnd, nr -1);
}

#endif

/*
 * ":simalt" command.
 */
    void
ex_simalt(exarg_T *eap)
{
    char_u *keys = eap->arg;

    PostMessage(s_hwnd, WM_SYSCOMMAND, (WPARAM)SC_KEYMENU, (LPARAM)0);
    while (*keys)
    {
	if (*keys == '~')
	    *keys = ' ';	    /* for showing system menu */
	PostMessage(s_hwnd, WM_CHAR, (WPARAM)*keys, (LPARAM)0);
	keys++;
    }
}

/*
 * Create the find & replace dialogs.
 * You can't have both at once: ":find" when replace is showing, destroys
 * the replace dialog first, and the other way around.
 */
#ifdef MSWIN_FIND_REPLACE
    static void
initialise_findrep(char_u *initial_string)
{
    int		wword = FALSE;
    int		mcase = !p_ic;
    char_u	*entry_text;

    /* Get the search string to use. */
    entry_text = get_find_dialog_text(initial_string, &wword, &mcase);

    s_findrep_struct.hwndOwner = s_hwnd;
    s_findrep_struct.Flags = FR_DOWN;
    if (mcase)
	s_findrep_struct.Flags |= FR_MATCHCASE;
    if (wword)
	s_findrep_struct.Flags |= FR_WHOLEWORD;
    if (entry_text != NULL && *entry_text != NUL)
	vim_strncpy(s_findrep_struct.lpstrFindWhat, entry_text,
					   s_findrep_struct.wFindWhatLen - 1);
    vim_free(entry_text);
}
#endif

    static void
set_window_title(HWND hwnd, char *title)
{
#ifdef FEAT_MBYTE
    if (title != NULL && enc_codepage >= 0 && enc_codepage != (int)GetACP())
    {
	WCHAR	*wbuf;
	int	n;

	/* Convert the title from 'encoding' to UTF-16. */
	wbuf = (WCHAR *)enc_to_utf16((char_u *)title, NULL);
	if (wbuf != NULL)
	{
	    n = SetWindowTextW(hwnd, wbuf);
	    vim_free(wbuf);
	    if (n != 0 || GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
		return;
	    /* Retry with non-wide function (for Windows 98). */
	}
    }
#endif
    (void)SetWindowText(hwnd, (LPCSTR)title);
}

    void
gui_mch_find_dialog(exarg_T *eap)
{
#ifdef MSWIN_FIND_REPLACE
    if (s_findrep_msg != 0)
    {
	if (IsWindow(s_findrep_hwnd) && !s_findrep_is_find)
	    DestroyWindow(s_findrep_hwnd);

	if (!IsWindow(s_findrep_hwnd))
	{
	    initialise_findrep(eap->arg);
# if defined(FEAT_MBYTE) && defined(WIN3264)
	    /* If the OS is Windows NT, and 'encoding' differs from active
	     * codepage: convert text and use wide function. */
	    if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
		    && enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	    {
		findrep_atow(&s_findrep_struct_w, &s_findrep_struct);
		s_findrep_hwnd = FindTextW(
					(LPFINDREPLACEW) &s_findrep_struct_w);
	    }
	    else
# endif
		s_findrep_hwnd = FindText((LPFINDREPLACE) &s_findrep_struct);
	}

	set_window_title(s_findrep_hwnd,
			       _("Find string (use '\\\\' to find  a '\\')"));
	(void)SetFocus(s_findrep_hwnd);

	s_findrep_is_find = TRUE;
    }
#endif
}


    void
gui_mch_replace_dialog(exarg_T *eap)
{
#ifdef MSWIN_FIND_REPLACE
    if (s_findrep_msg != 0)
    {
	if (IsWindow(s_findrep_hwnd) && s_findrep_is_find)
	    DestroyWindow(s_findrep_hwnd);

	if (!IsWindow(s_findrep_hwnd))
	{
	    initialise_findrep(eap->arg);
# if defined(FEAT_MBYTE) && defined(WIN3264)
	    if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT
		    && enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	    {
		findrep_atow(&s_findrep_struct_w, &s_findrep_struct);
		s_findrep_hwnd = ReplaceTextW(
					(LPFINDREPLACEW) &s_findrep_struct_w);
	    }
	    else
# endif
		s_findrep_hwnd = ReplaceText(
					   (LPFINDREPLACE) &s_findrep_struct);
	}

	set_window_title(s_findrep_hwnd,
			    _("Find & Replace (use '\\\\' to find  a '\\')"));
	(void)SetFocus(s_findrep_hwnd);

	s_findrep_is_find = FALSE;
    }
#endif
}


/*
 * Set visibility of the pointer.
 */
    void
gui_mch_mousehide(int hide)
{
    if (hide != gui.pointer_hidden)
    {
	ShowCursor(!hide);
	gui.pointer_hidden = hide;
    }
}

#ifdef FEAT_MENU
    static void
gui_mch_show_popupmenu_at(vimmenu_T *menu, int x, int y)
{
    /* Unhide the mouse, we don't get move events here. */
    gui_mch_mousehide(FALSE);

    (void)TrackPopupMenu(
	(HMENU)menu->submenu_id,
	TPM_LEFTALIGN | TPM_LEFTBUTTON,
	x, y,
	(int)0,	    /*reserved param*/
	s_hwnd,
	NULL);
    /*
     * NOTE: The pop-up menu can eat the mouse up event.
     * We deal with this in normal.c.
     */
}
#endif

/*
 * Got a message when the system will go down.
 */
    static void
_OnEndSession(void)
{
    getout_preserve_modified(1);
}

/*
 * Get this message when the user clicks on the cross in the top right corner
 * of a Windows95 window.
 */
/*ARGSUSED*/
    static void
_OnClose(
    HWND hwnd)
{
    gui_shell_closed();
}

/*
 * Get a message when the window is being destroyed.
 */
    static void
_OnDestroy(
    HWND hwnd)
{
#ifdef WIN16_3DLOOK
    Ctl3dUnregister(s_hinst);
#endif
    if (!destroying)
	_OnClose(hwnd);
}

    static void
_OnPaint(
    HWND hwnd)
{
    if (!IsMinimized(hwnd))
    {
	PAINTSTRUCT ps;

	out_flush();	    /* make sure all output has been processed */
	(void)BeginPaint(hwnd, &ps);

#ifdef FEAT_MBYTE
	/* prevent multi-byte characters from misprinting on an invalid
	 * rectangle */
	if (has_mbyte)
	{
	    RECT rect;

	    GetClientRect(hwnd, &rect);
	    ps.rcPaint.left = rect.left;
	    ps.rcPaint.right = rect.right;
	}
#endif

	if (!IsRectEmpty(&ps.rcPaint))
	    gui_redraw(ps.rcPaint.left, ps.rcPaint.top,
		    ps.rcPaint.right - ps.rcPaint.left + 1,
		    ps.rcPaint.bottom - ps.rcPaint.top + 1);
	EndPaint(hwnd, &ps);
    }
}

/*ARGSUSED*/
    static void
_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy)
{
    if (!IsMinimized(hwnd))
    {
	gui_resize_shell(cx, cy);

#ifdef FEAT_MENU
	/* Menu bar may wrap differently now */
	gui_mswin_get_menu_height(TRUE);
#endif
    }
}

    static void
_OnSetFocus(
    HWND hwnd,
    HWND hwndOldFocus)
{
    gui_focus_change(TRUE);
    s_getting_focus = TRUE;
    (void)MyWindowProc(hwnd, WM_SETFOCUS, (WPARAM)hwndOldFocus, 0);
}

    static void
_OnKillFocus(
    HWND hwnd,
    HWND hwndNewFocus)
{
    gui_focus_change(FALSE);
    s_getting_focus = FALSE;
    (void)MyWindowProc(hwnd, WM_KILLFOCUS, (WPARAM)hwndNewFocus, 0);
}

/*
 * Get a message when the user switches back to vim
 */
#ifdef WIN16
    static BOOL
#else
    static LRESULT
#endif
_OnActivateApp(
    HWND hwnd,
    BOOL fActivate,
#ifdef WIN16
    HTASK dwThreadId
#else
    DWORD dwThreadId
#endif
	)
{
    /* we call gui_focus_change() in _OnSetFocus() */
    /* gui_focus_change((int)fActivate); */
    return MyWindowProc(hwnd, WM_ACTIVATEAPP, fActivate, (DWORD)dwThreadId);
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
    DestroyWindow(sb->id);
}
#endif

/*
 * Get current mouse coordinates in text window.
 */
    void
gui_mch_getmouse(int *x, int *y)
{
    RECT rct;
    POINT mp;

    (void)GetWindowRect(s_textArea, &rct);
    (void)GetCursorPos((LPPOINT)&mp);
    *x = (int)(mp.x - rct.left);
    *y = (int)(mp.y - rct.top);
}

/*
 * Move mouse pointer to character at (x, y).
 */
    void
gui_mch_setmouse(int x, int y)
{
    RECT rct;

    (void)GetWindowRect(s_textArea, &rct);
    (void)SetCursorPos(x + gui.border_offset + rct.left,
		       y + gui.border_offset + rct.top);
}

    static void
gui_mswin_get_valid_dimensions(
    int w,
    int h,
    int *valid_w,
    int *valid_h)
{
    int	    base_width, base_height;

    base_width = gui_get_base_width()
	+ GetSystemMetrics(SM_CXFRAME) * 2;
    base_height = gui_get_base_height()
	+ GetSystemMetrics(SM_CYFRAME) * 2
	+ GetSystemMetrics(SM_CYCAPTION)
#ifdef FEAT_MENU
	+ gui_mswin_get_menu_height(FALSE)
#endif
	;
    *valid_w = base_width +
		    ((w - base_width) / gui.char_width) * gui.char_width;
    *valid_h = base_height +
		    ((h - base_height) / gui.char_height) * gui.char_height;
}

    void
gui_mch_flash(int msec)
{
    RECT    rc;

    /*
     * Note: InvertRect() excludes right and bottom of rectangle.
     */
    rc.left = 0;
    rc.top = 0;
    rc.right = gui.num_cols * gui.char_width;
    rc.bottom = gui.num_rows * gui.char_height;
    InvertRect(s_hdc, &rc);
    gui_mch_flush();			/* make sure it's displayed */

    ui_delay((long)msec, TRUE);	/* wait for a few msec */

    InvertRect(s_hdc, &rc);
}

/*
 * Return flags used for scrolling.
 * The SW_INVALIDATE is required when part of the window is covered or
 * off-screen. Refer to MS KB Q75236.
 */
    static int
get_scroll_flags(void)
{
    HWND	hwnd;
    RECT	rcVim, rcOther, rcDest;

    GetWindowRect(s_hwnd, &rcVim);

    /* Check if the window is partly above or below the screen.  We don't care
     * about partly left or right of the screen, it is not relevant when
     * scrolling up or down. */
    if (rcVim.top < 0 || rcVim.bottom > GetSystemMetrics(SM_CYFULLSCREEN))
	return SW_INVALIDATE;

    /* Check if there is an window (partly) on top of us. */
    for (hwnd = s_hwnd; (hwnd = GetWindow(hwnd, GW_HWNDPREV)) != (HWND)0; )
	if (IsWindowVisible(hwnd))
	{
	    GetWindowRect(hwnd, &rcOther);
	    if (IntersectRect(&rcDest, &rcVim, &rcOther))
		return SW_INVALIDATE;
	}
    return 0;
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
    void
gui_mch_delete_lines(
    int	    row,
    int	    num_lines)
{
    RECT	rc;

    rc.left = FILL_X(gui.scroll_region_left);
    rc.right = FILL_X(gui.scroll_region_right + 1);
    rc.top = FILL_Y(row);
    rc.bottom = FILL_Y(gui.scroll_region_bot + 1);

    ScrollWindowEx(s_textArea, 0, -num_lines * gui.char_height,
				    &rc, &rc, NULL, NULL, get_scroll_flags());

    UpdateWindow(s_textArea);
    /* This seems to be required to avoid the cursor disappearing when
     * scrolling such that the cursor ends up in the top-left character on
     * the screen...   But why?  (Webb) */
    /* It's probably fixed by disabling drawing the cursor while scrolling. */
    /* gui.cursor_is_valid = FALSE; */

    gui_clear_block(gui.scroll_region_bot - num_lines + 1,
						       gui.scroll_region_left,
	gui.scroll_region_bot, gui.scroll_region_right);
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
    void
gui_mch_insert_lines(
    int		row,
    int		num_lines)
{
    RECT	rc;

    rc.left = FILL_X(gui.scroll_region_left);
    rc.right = FILL_X(gui.scroll_region_right + 1);
    rc.top = FILL_Y(row);
    rc.bottom = FILL_Y(gui.scroll_region_bot + 1);
    /* The SW_INVALIDATE is required when part of the window is covered or
     * off-screen.  How do we avoid it when it's not needed? */
    ScrollWindowEx(s_textArea, 0, num_lines * gui.char_height,
				    &rc, &rc, NULL, NULL, get_scroll_flags());

    UpdateWindow(s_textArea);

    gui_clear_block(row, gui.scroll_region_left,
				row + num_lines - 1, gui.scroll_region_right);
}


/*ARGSUSED*/
    void
gui_mch_exit(int rc)
{
    ReleaseDC(s_textArea, s_hdc);
    DeleteObject(s_brush);

#ifdef FEAT_TEAROFF
    /* Unload the tearoff bitmap */
    (void)DeleteObject((HGDIOBJ)s_htearbitmap);
#endif

    /* Destroy our window (if we have one). */
    if (s_hwnd != NULL)
    {
	destroying = TRUE;	/* ignore WM_DESTROY message now */
	DestroyWindow(s_hwnd);
    }

#ifdef GLOBAL_IME
    global_ime_end();
#endif
}

    static char_u *
logfont2name(LOGFONT lf)
{
    char	*p;
    char	*res;
    char	*charset_name;

    charset_name = charset_id2name((int)lf.lfCharSet);
    res = alloc((unsigned)(strlen(lf.lfFaceName) + 20
		    + (charset_name == NULL ? 0 : strlen(charset_name) + 2)));
    if (res != NULL)
    {
	p = res;
	/* make a normal font string out of the lf thing:*/
	sprintf((char *)p, "%s:h%d", lf.lfFaceName, pixels_to_points(
			 lf.lfHeight < 0 ? -lf.lfHeight : lf.lfHeight, TRUE));
	while (*p)
	{
	    if (*p == ' ')
		*p = '_';
	    ++p;
	}
#ifndef MSWIN16_FASTTEXT
	if (lf.lfItalic)
	    STRCAT(p, ":i");
	if (lf.lfWeight >= FW_BOLD)
	    STRCAT(p, ":b");
#endif
	if (lf.lfUnderline)
	    STRCAT(p, ":u");
	if (lf.lfStrikeOut)
	    STRCAT(p, ":s");
	if (charset_name != NULL)
	{
	    STRCAT(p, ":c");
	    STRCAT(p, charset_name);
	}
    }

    return res;
}

/*
 * Initialise vim to use the font with the given name.
 * Return FAIL if the font could not be loaded, OK otherwise.
 */
/*ARGSUSED*/
    int
gui_mch_init_font(char_u *font_name, int fontset)
{
    LOGFONT	lf;
    GuiFont	font = NOFONT;
    char_u	*p;

    /* Load the font */
    if (get_logfont(&lf, font_name, NULL, TRUE) == OK)
	font = get_font_handle(&lf);
    if (font == NOFONT)
	return FAIL;

    if (font_name == NULL)
	font_name = lf.lfFaceName;
#if defined(FEAT_MBYTE_IME) || defined(GLOBAL_IME)
    norm_logfont = lf;
#endif
#ifdef FEAT_MBYTE_IME
    im_set_font(&lf);
#endif
    gui_mch_free_font(gui.norm_font);
    gui.norm_font = font;
    current_font_height = lf.lfHeight;
    GetFontSize(font);

    p = logfont2name(lf);
    if (p != NULL)
    {
	hl_set_font_name(p);

	/* When setting 'guifont' to "*" replace it with the actual font name.
	 * */
	if (STRCMP(font_name, "*") == 0 && STRCMP(p_guifont, "*") == 0)
	{
	    vim_free(p_guifont);
	    p_guifont = p;
	}
	else
	    vim_free(p);
    }

#ifndef MSWIN16_FASTTEXT
    gui_mch_free_font(gui.ital_font);
    gui.ital_font = NOFONT;
    gui_mch_free_font(gui.bold_font);
    gui.bold_font = NOFONT;
    gui_mch_free_font(gui.boldital_font);
    gui.boldital_font = NOFONT;

    if (!lf.lfItalic)
    {
	lf.lfItalic = TRUE;
	gui.ital_font = get_font_handle(&lf);
	lf.lfItalic = FALSE;
    }
    if (lf.lfWeight < FW_BOLD)
    {
	lf.lfWeight = FW_BOLD;
	gui.bold_font = get_font_handle(&lf);
	if (!lf.lfItalic)
	{
	    lf.lfItalic = TRUE;
	    gui.boldital_font = get_font_handle(&lf);
	}
    }
#endif

    return OK;
}

#ifndef WPF_RESTORETOMAXIMIZED
# define WPF_RESTORETOMAXIMIZED 2   /* just in case someone doesn't have it */
#endif

/*
 * Return TRUE if the GUI window is maximized, filling the whole screen.
 */
    int
gui_mch_maximized()
{
    WINDOWPLACEMENT wp;

    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(s_hwnd, &wp))
	return wp.showCmd == SW_SHOWMAXIMIZED
	    || (wp.showCmd == SW_SHOWMINIMIZED
		    && wp.flags == WPF_RESTORETOMAXIMIZED);

    return 0;
}

/*
 * Called when the font changed while the window is maximized.  Compute the
 * new Rows and Columns.  This is like resizing the window.
 */
    void
gui_mch_newfont()
{
    RECT	rect;

    GetWindowRect(s_hwnd, &rect);
    gui_resize_shell(rect.right - rect.left
			- GetSystemMetrics(SM_CXFRAME) * 2,
		     rect.bottom - rect.top
			- GetSystemMetrics(SM_CYFRAME) * 2
			- GetSystemMetrics(SM_CYCAPTION)
#ifdef FEAT_MENU
			- gui_mswin_get_menu_height(FALSE)
#endif
	    );
}

/*
 * Set the window title
 */
/*ARGSUSED*/
    void
gui_mch_settitle(
    char_u  *title,
    char_u  *icon)
{
    set_window_title(s_hwnd, (title == NULL ? "VIM" : (char *)title));
}

#ifdef FEAT_MOUSESHAPE
/* Table for shape IDCs.  Keep in sync with the mshape_names[] table in
 * misc2.c! */
static LPCSTR mshape_idcs[] =
{
    MAKEINTRESOURCE(IDC_ARROW),		/* arrow */
    MAKEINTRESOURCE(0),			/* blank */
    MAKEINTRESOURCE(IDC_IBEAM),		/* beam */
    MAKEINTRESOURCE(IDC_SIZENS),	/* updown */
    MAKEINTRESOURCE(IDC_SIZENS),	/* udsizing */
    MAKEINTRESOURCE(IDC_SIZEWE),	/* leftright */
    MAKEINTRESOURCE(IDC_SIZEWE),	/* lrsizing */
    MAKEINTRESOURCE(IDC_WAIT),		/* busy */
#ifdef WIN3264
    MAKEINTRESOURCE(IDC_NO),		/* no */
#else
    MAKEINTRESOURCE(IDC_ICON),		/* no */
#endif
    MAKEINTRESOURCE(IDC_ARROW),		/* crosshair */
    MAKEINTRESOURCE(IDC_ARROW),		/* hand1 */
    MAKEINTRESOURCE(IDC_ARROW),		/* hand2 */
    MAKEINTRESOURCE(IDC_ARROW),		/* pencil */
    MAKEINTRESOURCE(IDC_ARROW),		/* question */
    MAKEINTRESOURCE(IDC_ARROW),		/* right-arrow */
    MAKEINTRESOURCE(IDC_UPARROW),	/* up-arrow */
    MAKEINTRESOURCE(IDC_ARROW)		/* last one */
};

    void
mch_set_mouse_shape(int shape)
{
    LPCSTR idc;

    if (shape == MSHAPE_HIDE)
	ShowCursor(FALSE);
    else
    {
	if (shape >= MSHAPE_NUMBERED)
	    idc = MAKEINTRESOURCE(IDC_ARROW);
	else
	    idc = mshape_idcs[shape];
#ifdef SetClassLongPtr
	SetClassLongPtr(s_textArea, GCLP_HCURSOR, (__int3264)(LONG_PTR)LoadCursor(NULL, idc));
#else
# ifdef WIN32
	SetClassLong(s_textArea, GCL_HCURSOR, (long_u)LoadCursor(NULL, idc));
# else /* Win16 */
	SetClassWord(s_textArea, GCW_HCURSOR, (WORD)LoadCursor(NULL, idc));
# endif
#endif
	if (!p_mh)
	{
	    POINT mp;

	    /* Set the position to make it redrawn with the new shape. */
	    (void)GetCursorPos((LPPOINT)&mp);
	    (void)SetCursorPos(mp.x, mp.y);
	    ShowCursor(TRUE);
	}
    }
}
#endif

#ifdef FEAT_BROWSE
/*
 * The file browser exists in two versions: with "W" uses wide characters,
 * without "W" the current codepage.  When FEAT_MBYTE is defined and on
 * Windows NT/2000/XP the "W" functions are used.
 */

# if defined(FEAT_MBYTE) && defined(WIN3264)
/*
 * Wide version of convert_filter().  Keep in sync!
 */
    static WCHAR *
convert_filterW(char_u *s)
{
    WCHAR	*res;
    unsigned	s_len = (unsigned)STRLEN(s);
    unsigned	i;

    res = (WCHAR *)alloc((s_len + 3) * sizeof(WCHAR));
    if (res != NULL)
    {
	for (i = 0; i < s_len; ++i)
	    if (s[i] == '\t' || s[i] == '\n')
		res[i] = '\0';
	    else
		res[i] = s[i];
	res[s_len] = NUL;
	/* Add two extra NULs to make sure it's properly terminated. */
	res[s_len + 1] = NUL;
	res[s_len + 2] = NUL;
    }
    return res;
}

/*
 * Wide version of gui_mch_browse().  Keep in sync!
 */
    static char_u *
gui_mch_browseW(
	int saving,
	char_u *title,
	char_u *dflt,
	char_u *ext,
	char_u *initdir,
	char_u *filter)
{
    /* We always use the wide function.  This means enc_to_utf16() must work,
     * otherwise it fails miserably! */
    OPENFILENAMEW	fileStruct;
    WCHAR		fileBuf[MAXPATHL];
    WCHAR		*wp;
    int			i;
    WCHAR		*titlep = NULL;
    WCHAR		*extp = NULL;
    WCHAR		*initdirp = NULL;
    WCHAR		*filterp;
    char_u		*p;

    if (dflt == NULL)
	fileBuf[0] = NUL;
    else
    {
	wp = enc_to_utf16(dflt, NULL);
	if (wp == NULL)
	    fileBuf[0] = NUL;
	else
	{
	    for (i = 0; wp[i] != NUL && i < MAXPATHL - 1; ++i)
		fileBuf[i] = wp[i];
	    fileBuf[i] = NUL;
	    vim_free(wp);
	}
    }

    /* Convert the filter to Windows format. */
    filterp = convert_filterW(filter);

    vim_memset(&fileStruct, 0, sizeof(OPENFILENAMEW));
#ifdef OPENFILENAME_SIZE_VERSION_400
    /* be compatible with Windows NT 4.0 */
    /* TODO: what to use for OPENFILENAMEW??? */
    fileStruct.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
    fileStruct.lStructSize = sizeof(fileStruct);
#endif

    if (title != NULL)
	titlep = enc_to_utf16(title, NULL);
    fileStruct.lpstrTitle = titlep;

    if (ext != NULL)
	extp = enc_to_utf16(ext, NULL);
    fileStruct.lpstrDefExt = extp;

    fileStruct.lpstrFile = fileBuf;
    fileStruct.nMaxFile = MAXPATHL;
    fileStruct.lpstrFilter = filterp;
    fileStruct.hwndOwner = s_hwnd;		/* main Vim window is owner*/
    /* has an initial dir been specified? */
    if (initdir != NULL && *initdir != NUL)
    {
	/* Must have backslashes here, no matter what 'shellslash' says */
	initdirp = enc_to_utf16(initdir, NULL);
	if (initdirp != NULL)
	{
	    for (wp = initdirp; *wp != NUL; ++wp)
		if (*wp == '/')
		    *wp = '\\';
	}
	fileStruct.lpstrInitialDir = initdirp;
    }

    /*
     * TODO: Allow selection of multiple files.  Needs another arg to this
     * function to ask for it, and need to use OFN_ALLOWMULTISELECT below.
     * Also, should we use OFN_FILEMUSTEXIST when opening?  Vim can edit on
     * files that don't exist yet, so I haven't put it in.  What about
     * OFN_PATHMUSTEXIST?
     * Don't use OFN_OVERWRITEPROMPT, Vim has its own ":confirm" dialog.
     */
    fileStruct.Flags = (OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY);
#ifdef FEAT_SHORTCUT
    if (curbuf->b_p_bin)
	fileStruct.Flags |= OFN_NODEREFERENCELINKS;
#endif
    if (saving)
    {
	if (!GetSaveFileNameW(&fileStruct))
	    return NULL;
    }
    else
    {
	if (!GetOpenFileNameW(&fileStruct))
	    return NULL;
    }

    vim_free(filterp);
    vim_free(initdirp);
    vim_free(titlep);
    vim_free(extp);

    /* Convert from UCS2 to 'encoding'. */
    p = utf16_to_enc(fileBuf, NULL);
    if (p != NULL)
	/* when out of memory we get garbage for non-ASCII chars */
	STRCPY(fileBuf, p);
    vim_free(p);

    /* Give focus back to main window (when using MDI). */
    SetFocus(s_hwnd);

    /* Shorten the file name if possible */
    return vim_strsave(shorten_fname1((char_u *)fileBuf));
}
# endif /* FEAT_MBYTE */


/*
 * Convert the string s to the proper format for a filter string by replacing
 * the \t and \n delimiters with \0.
 * Returns the converted string in allocated memory.
 *
 * Keep in sync with convert_filterW() above!
 */
    static char_u *
convert_filter(char_u *s)
{
    char_u	*res;
    unsigned	s_len = (unsigned)STRLEN(s);
    unsigned	i;

    res = alloc(s_len + 3);
    if (res != NULL)
    {
	for (i = 0; i < s_len; ++i)
	    if (s[i] == '\t' || s[i] == '\n')
		res[i] = '\0';
	    else
		res[i] = s[i];
	res[s_len] = NUL;
	/* Add two extra NULs to make sure it's properly terminated. */
	res[s_len + 1] = NUL;
	res[s_len + 2] = NUL;
    }
    return res;
}

/*
 * Select a directory.
 */
    char_u *
gui_mch_browsedir(char_u *title, char_u *initdir)
{
    /* We fake this: Use a filter that doesn't select anything and a default
     * file name that won't be used. */
    return gui_mch_browse(0, title, (char_u *)_("Not Used"), NULL,
			      initdir, (char_u *)_("Directory\t*.nothing\n"));
}

/*
 * Pop open a file browser and return the file selected, in allocated memory,
 * or NULL if Cancel is hit.
 *  saving  - TRUE if the file will be saved to, FALSE if it will be opened.
 *  title   - Title message for the file browser dialog.
 *  dflt    - Default name of file.
 *  ext     - Default extension to be added to files without extensions.
 *  initdir - directory in which to open the browser (NULL = current dir)
 *  filter  - Filter for matched files to choose from.
 *
 * Keep in sync with gui_mch_browseW() above!
 */
    char_u *
gui_mch_browse(
	int saving,
	char_u *title,
	char_u *dflt,
	char_u *ext,
	char_u *initdir,
	char_u *filter)
{
    OPENFILENAME	fileStruct;
    char_u		fileBuf[MAXPATHL];
    char_u		*initdirp = NULL;
    char_u		*filterp;
    char_u		*p;

# if defined(FEAT_MBYTE) && defined(WIN3264)
    if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	return gui_mch_browseW(saving, title, dflt, ext, initdir, filter);
# endif

    if (dflt == NULL)
	fileBuf[0] = NUL;
    else
	vim_strncpy(fileBuf, dflt, MAXPATHL - 1);

    /* Convert the filter to Windows format. */
    filterp = convert_filter(filter);

    vim_memset(&fileStruct, 0, sizeof(OPENFILENAME));
#ifdef OPENFILENAME_SIZE_VERSION_400
    /* be compatible with Windows NT 4.0 */
    fileStruct.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
    fileStruct.lStructSize = sizeof(fileStruct);
#endif

    fileStruct.lpstrTitle = title;
    fileStruct.lpstrDefExt = ext;

    fileStruct.lpstrFile = fileBuf;
    fileStruct.nMaxFile = MAXPATHL;
    fileStruct.lpstrFilter = filterp;
    fileStruct.hwndOwner = s_hwnd;		/* main Vim window is owner*/
    /* has an initial dir been specified? */
    if (initdir != NULL && *initdir != NUL)
    {
	/* Must have backslashes here, no matter what 'shellslash' says */
	initdirp = vim_strsave(initdir);
	if (initdirp != NULL)
	    for (p = initdirp; *p != NUL; ++p)
		if (*p == '/')
		    *p = '\\';
	fileStruct.lpstrInitialDir = initdirp;
    }

    /*
     * TODO: Allow selection of multiple files.  Needs another arg to this
     * function to ask for it, and need to use OFN_ALLOWMULTISELECT below.
     * Also, should we use OFN_FILEMUSTEXIST when opening?  Vim can edit on
     * files that don't exist yet, so I haven't put it in.  What about
     * OFN_PATHMUSTEXIST?
     * Don't use OFN_OVERWRITEPROMPT, Vim has its own ":confirm" dialog.
     */
    fileStruct.Flags = (OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY);
#ifdef FEAT_SHORTCUT
    if (curbuf->b_p_bin)
	fileStruct.Flags |= OFN_NODEREFERENCELINKS;
#endif
    if (saving)
    {
	if (!GetSaveFileName(&fileStruct))
	    return NULL;
    }
    else
    {
	if (!GetOpenFileName(&fileStruct))
	    return NULL;
    }

    vim_free(filterp);
    vim_free(initdirp);

    /* Give focus back to main window (when using MDI). */
    SetFocus(s_hwnd);

    /* Shorten the file name if possible */
    return vim_strsave(shorten_fname1((char_u *)fileBuf));
}
#endif /* FEAT_BROWSE */

/*ARGSUSED*/
    static void
_OnDropFiles(
    HWND hwnd,
    HDROP hDrop)
{
#ifdef FEAT_WINDOWS
#ifdef WIN3264
# define BUFPATHLEN _MAX_PATH
# define DRAGQVAL 0xFFFFFFFF
#else
# define BUFPATHLEN MAXPATHL
# define DRAGQVAL 0xFFFF
#endif
#ifdef FEAT_MBYTE
    WCHAR   wszFile[BUFPATHLEN];
#endif
    char    szFile[BUFPATHLEN];
    UINT    cFiles = DragQueryFile(hDrop, DRAGQVAL, NULL, 0);
    UINT    i;
    char_u  **fnames;
    POINT   pt;
    int_u   modifiers = 0;

    /* TRACE("_OnDropFiles: %d files dropped\n", cFiles); */

    /* Obtain dropped position */
    DragQueryPoint(hDrop, &pt);
    MapWindowPoints(s_hwnd, s_textArea, &pt, 1);

# ifdef FEAT_VISUAL
    reset_VIsual();
# endif

    fnames = (char_u **)alloc(cFiles * sizeof(char_u *));

    if (fnames != NULL)
	for (i = 0; i < cFiles; ++i)
	{
#ifdef FEAT_MBYTE
	    if (DragQueryFileW(hDrop, i, wszFile, BUFPATHLEN) > 0)
		fnames[i] = utf16_to_enc(wszFile, NULL);
	    else
#endif
	    {
		DragQueryFile(hDrop, i, szFile, BUFPATHLEN);
		fnames[i] = vim_strsave(szFile);
	    }
	}

    DragFinish(hDrop);

    if (fnames != NULL)
    {
	if ((GetKeyState(VK_SHIFT) & 0x8000) != 0)
	    modifiers |= MOUSE_SHIFT;
	if ((GetKeyState(VK_CONTROL) & 0x8000) != 0)
	    modifiers |= MOUSE_CTRL;
	if ((GetKeyState(VK_MENU) & 0x8000) != 0)
	    modifiers |= MOUSE_ALT;

	gui_handle_drop(pt.x, pt.y, modifiers, fnames, cFiles);

	s_need_activate = TRUE;
    }
#endif
}

/*ARGSUSED*/
    static int
_OnScroll(
    HWND hwnd,
    HWND hwndCtl,
    UINT code,
    int pos)
{
    static UINT	prev_code = 0;   /* code of previous call */
    scrollbar_T *sb, *sb_info;
    long	val;
    int		dragging = FALSE;
    int		dont_scroll_save = dont_scroll;
#ifndef WIN3264
    int		nPos;
#else
    SCROLLINFO	si;

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
#endif

    sb = gui_mswin_find_scrollbar(hwndCtl);
    if (sb == NULL)
	return 0;

    if (sb->wp != NULL)		/* Left or right scrollbar */
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
    val = sb_info->value;

    switch (code)
    {
	case SB_THUMBTRACK:
	    val = pos;
	    dragging = TRUE;
	    if (sb->scroll_shift > 0)
		val <<= sb->scroll_shift;
	    break;
	case SB_LINEDOWN:
	    val++;
	    break;
	case SB_LINEUP:
	    val--;
	    break;
	case SB_PAGEDOWN:
	    val += (sb_info->size > 2 ? sb_info->size - 2 : 1);
	    break;
	case SB_PAGEUP:
	    val -= (sb_info->size > 2 ? sb_info->size - 2 : 1);
	    break;
	case SB_TOP:
	    val = 0;
	    break;
	case SB_BOTTOM:
	    val = sb_info->max;
	    break;
	case SB_ENDSCROLL:
	    if (prev_code == SB_THUMBTRACK)
	    {
		/*
		 * "pos" only gives us 16-bit data.  In case of large file,
		 * use GetScrollPos() which returns 32-bit.  Unfortunately it
		 * is not valid while the scrollbar is being dragged.
		 */
		val = GetScrollPos(hwndCtl, SB_CTL);
		if (sb->scroll_shift > 0)
		    val <<= sb->scroll_shift;
	    }
	    break;

	default:
	    /* TRACE("Unknown scrollbar event %d\n", code); */
	    return 0;
    }
    prev_code = code;

#ifdef WIN3264
    si.nPos = (sb->scroll_shift > 0) ? val >> sb->scroll_shift : val;
    SetScrollInfo(hwndCtl, SB_CTL, &si, TRUE);
#else
    nPos = (sb->scroll_shift > 0) ? val >> sb->scroll_shift : val;
    SetScrollPos(hwndCtl, SB_CTL, nPos, TRUE);
#endif

    /*
     * When moving a vertical scrollbar, move the other vertical scrollbar too.
     */
    if (sb->wp != NULL)
    {
	scrollbar_T *sba = sb->wp->w_scrollbars;
	HWND    id = sba[ (sb == sba + SBAR_LEFT) ? SBAR_RIGHT : SBAR_LEFT].id;

#ifdef WIN3264
	SetScrollInfo(id, SB_CTL, &si, TRUE);
#else
	SetScrollPos(id, SB_CTL, nPos, TRUE);
#endif
    }

    /* Don't let us be interrupted here by another message. */
    s_busy_processing = TRUE;

    /* When "allow_scrollbar" is FALSE still need to remember the new
     * position, but don't actually scroll by setting "dont_scroll". */
    dont_scroll = !allow_scrollbar;

    gui_drag_scrollbar(sb, val, dragging);

    s_busy_processing = FALSE;
    dont_scroll = dont_scroll_save;

    return 0;
}


/*
 * Get command line arguments.
 * Use "prog" as the name of the program and "cmdline" as the arguments.
 * Copy the arguments to allocated memory.
 * Return the number of arguments (including program name).
 * Return pointers to the arguments in "argvp".  Memory is allocated with
 * malloc(), use free() instead of vim_free().
 * Return pointer to buffer in "tofree".
 * Returns zero when out of memory.
 */
/*ARGSUSED*/
    int
get_cmd_args(char *prog, char *cmdline, char ***argvp, char **tofree)
{
    int		i;
    char	*p;
    char	*progp;
    char	*pnew = NULL;
    char	*newcmdline;
    int		inquote;
    int		argc;
    char	**argv = NULL;
    int		round;

    *tofree = NULL;

#ifdef FEAT_MBYTE
    /* Try using the Unicode version first, it takes care of conversion when
     * 'encoding' is changed. */
    argc = get_cmd_argsW(&argv);
    if (argc != 0)
	goto done;
#endif

    /* Handle the program name.  Remove the ".exe" extension, and find the 1st
     * non-space. */
    p = strrchr(prog, '.');
    if (p != NULL)
	*p = NUL;
    for (progp = prog; *progp == ' '; ++progp)
	;

    /* The command line is copied to allocated memory, so that we can change
     * it.  Add the size of the string, the separating NUL and a terminating
     * NUL. */
    newcmdline = malloc(STRLEN(cmdline) + STRLEN(progp) + 2);
    if (newcmdline == NULL)
	return 0;

    /*
     * First round: count the number of arguments ("pnew" == NULL).
     * Second round: produce the arguments.
     */
    for (round = 1; round <= 2; ++round)
    {
	/* First argument is the program name. */
	if (pnew != NULL)
	{
	    argv[0] = pnew;
	    strcpy(pnew, progp);
	    pnew += strlen(pnew);
	    *pnew++ = NUL;
	}

	/*
	 * Isolate each argument and put it in argv[].
	 */
	p = cmdline;
	argc = 1;
	while (*p != NUL)
	{
	    inquote = FALSE;
	    if (pnew != NULL)
		argv[argc] = pnew;
	    ++argc;
	    while (*p != NUL && (inquote || (*p != ' ' && *p != '\t')))
	    {
		/* Backslashes are only special when followed by a double
		 * quote. */
		i = (int)strspn(p, "\\");
		if (p[i] == '"')
		{
		    /* Halve the number of backslashes. */
		    if (i > 1 && pnew != NULL)
		    {
			vim_memset(pnew, '\\', i / 2);
			pnew += i / 2;
		    }

		    /* Even nr of backslashes toggles quoting, uneven copies
		     * the double quote. */
		    if ((i & 1) == 0)
			inquote = !inquote;
		    else if (pnew != NULL)
			*pnew++ = '"';
		    p += i + 1;
		}
		else if (i > 0)
		{
		    /* Copy span of backslashes unmodified. */
		    if (pnew != NULL)
		    {
			vim_memset(pnew, '\\', i);
			pnew += i;
		    }
		    p += i;
		}
		else
		{
		    if (pnew != NULL)
			*pnew++ = *p;
#ifdef FEAT_MBYTE
		    /* Can't use mb_* functions, because 'encoding' is not
		     * initialized yet here. */
		    if (IsDBCSLeadByte(*p))
		    {
			++p;
			if (pnew != NULL)
			    *pnew++ = *p;
		    }
#endif
		    ++p;
		}
	    }

	    if (pnew != NULL)
		*pnew++ = NUL;
	    while (*p == ' ' || *p == '\t')
		++p;		    /* advance until a non-space */
	}

	if (round == 1)
	{
	    argv = (char **)malloc((argc + 1) * sizeof(char *));
	    if (argv == NULL )
	    {
		free(newcmdline);
		return 0;		   /* malloc error */
	    }
	    pnew = newcmdline;
	    *tofree = newcmdline;
	}
    }

#ifdef FEAT_MBYTE
done:
#endif
    argv[argc] = NULL;		/* NULL-terminated list */
    *argvp = argv;
    return argc;
}
