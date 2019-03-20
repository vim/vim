/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * os_win32.c
 *
 * Used for both the console version and the Win32 GUI.  A lot of code is for
 * the console version only, so there is a lot of "#ifndef FEAT_GUI_MSWIN".
 *
 * Win32 (Windows NT and Windows 95) system-dependent routines.
 * Portions lifted from the Win32 SDK samples, the MSDOS-dependent code,
 * NetHack 3.1.3, GNU Emacs 19.30, and Vile 5.5.
 *
 * George V. Reilly <george@reilly.org> wrote most of this.
 * Roger Knobbe <rogerk@wonderware.com> did the initial port of Vim 3.0.
 */

#include "vim.h"

#ifdef FEAT_MZSCHEME
# include "if_mzsch.h"
#endif

#include <sys/types.h>
#include <signal.h>
#include <limits.h>

/* cproto fails on missing include files */
#ifndef PROTO
# include <process.h>
#endif

#undef chdir
#ifdef __GNUC__
# ifndef __MINGW32__
#  include <dirent.h>
# endif
#else
# include <direct.h>
#endif

#ifndef PROTO
# if defined(FEAT_TITLE) && !defined(FEAT_GUI_MSWIN)
#  include <shellapi.h>
# endif
#endif

#ifdef FEAT_JOB_CHANNEL
# include <tlhelp32.h>
#endif

#ifdef __MINGW32__
# ifndef FROM_LEFT_1ST_BUTTON_PRESSED
#  define FROM_LEFT_1ST_BUTTON_PRESSED    0x0001
# endif
# ifndef RIGHTMOST_BUTTON_PRESSED
#  define RIGHTMOST_BUTTON_PRESSED	  0x0002
# endif
# ifndef FROM_LEFT_2ND_BUTTON_PRESSED
#  define FROM_LEFT_2ND_BUTTON_PRESSED    0x0004
# endif
# ifndef FROM_LEFT_3RD_BUTTON_PRESSED
#  define FROM_LEFT_3RD_BUTTON_PRESSED    0x0008
# endif
# ifndef FROM_LEFT_4TH_BUTTON_PRESSED
#  define FROM_LEFT_4TH_BUTTON_PRESSED    0x0010
# endif

/*
 * EventFlags
 */
# ifndef MOUSE_MOVED
#  define MOUSE_MOVED   0x0001
# endif
# ifndef DOUBLE_CLICK
#  define DOUBLE_CLICK  0x0002
# endif
#endif

/* Record all output and all keyboard & mouse input */
/* #define MCH_WRITE_DUMP */

#ifdef MCH_WRITE_DUMP
FILE* fdDump = NULL;
#endif

/*
 * When generating prototypes for Win32 on Unix, these lines make the syntax
 * errors disappear.  They do not need to be correct.
 */
#ifdef PROTO
#define WINAPI
typedef char * LPCSTR;
typedef char * LPWSTR;
typedef int ACCESS_MASK;
typedef int BOOL;
typedef int COLORREF;
typedef int CONSOLE_CURSOR_INFO;
typedef int COORD;
typedef int DWORD;
typedef int HANDLE;
typedef int LPHANDLE;
typedef int HDC;
typedef int HFONT;
typedef int HICON;
typedef int HINSTANCE;
typedef int HWND;
typedef int INPUT_RECORD;
typedef int INT;
typedef int KEY_EVENT_RECORD;
typedef int LOGFONT;
typedef int LPBOOL;
typedef int LPCTSTR;
typedef int LPDWORD;
typedef int LPSTR;
typedef int LPTSTR;
typedef int LPVOID;
typedef int MOUSE_EVENT_RECORD;
typedef int PACL;
typedef int PDWORD;
typedef int PHANDLE;
typedef int PRINTDLG;
typedef int PSECURITY_DESCRIPTOR;
typedef int PSID;
typedef int SECURITY_INFORMATION;
typedef int SHORT;
typedef int SMALL_RECT;
typedef int TEXTMETRIC;
typedef int TOKEN_INFORMATION_CLASS;
typedef int TRUSTEE;
typedef int WORD;
typedef int WCHAR;
typedef void VOID;
typedef int BY_HANDLE_FILE_INFORMATION;
typedef int SE_OBJECT_TYPE;
typedef int PSNSECINFO;
typedef int PSNSECINFOW;
typedef int STARTUPINFO;
typedef int PROCESS_INFORMATION;
typedef int LPSECURITY_ATTRIBUTES;
# define __stdcall /* empty */
#endif

#if defined(__BORLANDC__)
/* Strangely Borland uses a non-standard name. */
# define wcsicmp(a, b) wcscmpi((a), (b))
#endif

#ifndef FEAT_GUI_MSWIN
/* Win32 Console handles for input and output */
static HANDLE g_hConIn  = INVALID_HANDLE_VALUE;
static HANDLE g_hConOut = INVALID_HANDLE_VALUE;

/* Win32 Screen buffer,coordinate,console I/O information */
static SMALL_RECT g_srScrollRegion;
static COORD	  g_coord;  /* 0-based, but external coords are 1-based */

/* The attribute of the screen when the editor was started */
static WORD  g_attrDefault = 7;  /* lightgray text on black background */
static WORD  g_attrCurrent;

static int g_fCBrkPressed = FALSE;  /* set by ctrl-break interrupt */
static int g_fCtrlCPressed = FALSE; /* set when ctrl-C or ctrl-break detected */
static int g_fForceExit = FALSE;    /* set when forcefully exiting */

static void scroll(unsigned cLines);
static void set_scroll_region(unsigned left, unsigned top,
			      unsigned right, unsigned bottom);
static void set_scroll_region_tb(unsigned top, unsigned bottom);
static void set_scroll_region_lr(unsigned left, unsigned right);
static void insert_lines(unsigned cLines);
static void delete_lines(unsigned cLines);
static void gotoxy(unsigned x, unsigned y);
static void standout(void);
static int s_cursor_visible = TRUE;
static int did_create_conin = FALSE;
#else
static int s_dont_use_vimrun = TRUE;
static int need_vimrun_warning = FALSE;
static char *vimrun_path = "vimrun ";
#endif

static int win32_getattrs(char_u *name);
static int win32_setattrs(char_u *name, int attrs);
static int win32_set_archive(char_u *name);

static int conpty_working = 0;
static int conpty_stable = 0;
static void vtp_flag_init();

#ifndef FEAT_GUI_MSWIN
static int vtp_working = 0;
static void vtp_init();
static void vtp_exit();
static int vtp_printf(char *format, ...);
static void vtp_sgr_bulk(int arg);
static void vtp_sgr_bulks(int argc, int *argv);

static guicolor_T save_console_bg_rgb;
static guicolor_T save_console_fg_rgb;

static int g_color_index_bg = 0;
static int g_color_index_fg = 7;

# ifdef FEAT_TERMGUICOLORS
static int default_console_color_bg = 0x000000; // black
static int default_console_color_fg = 0xc0c0c0; // white
# endif

# ifdef FEAT_TERMGUICOLORS
#  define USE_VTP		(vtp_working && is_term_win32() && (p_tgc || (!p_tgc && t_colors >= 256)))
# else
#  define USE_VTP		0
# endif

static void set_console_color_rgb(void);
static void reset_console_color_rgb(void);
#endif

/* This flag is newly created from Windows 10 */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
# define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#ifndef FEAT_GUI_MSWIN
static int suppress_winsize = 1;	/* don't fiddle with console */
#endif

static char_u *exe_path = NULL;

static BOOL win8_or_later = FALSE;

#ifndef FEAT_GUI_MSWIN
/* Dynamic loading for portability */
typedef struct _DYN_CONSOLE_SCREEN_BUFFER_INFOEX
{
    ULONG cbSize;
    COORD dwSize;
    COORD dwCursorPosition;
    WORD wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
    WORD wPopupAttributes;
    BOOL bFullscreenSupported;
    COLORREF ColorTable[16];
} DYN_CONSOLE_SCREEN_BUFFER_INFOEX, *PDYN_CONSOLE_SCREEN_BUFFER_INFOEX;
typedef BOOL (WINAPI *PfnGetConsoleScreenBufferInfoEx)(HANDLE, PDYN_CONSOLE_SCREEN_BUFFER_INFOEX);
static PfnGetConsoleScreenBufferInfoEx pGetConsoleScreenBufferInfoEx;
typedef BOOL (WINAPI *PfnSetConsoleScreenBufferInfoEx)(HANDLE, PDYN_CONSOLE_SCREEN_BUFFER_INFOEX);
static PfnSetConsoleScreenBufferInfoEx pSetConsoleScreenBufferInfoEx;
static BOOL has_csbiex = FALSE;
#endif

/*
 * Get version number including build number
 */
typedef BOOL (WINAPI *PfnRtlGetVersion)(LPOSVERSIONINFOW);
# define MAKE_VER(major, minor, build) \
    (((major) << 24) | ((minor) << 16) | (build))

    static DWORD
get_build_number(void)
{
    OSVERSIONINFOW	osver = {sizeof(OSVERSIONINFOW)};
    HMODULE		hNtdll;
    PfnRtlGetVersion	pRtlGetVersion;
    DWORD		ver = MAKE_VER(0, 0, 0);

    hNtdll = GetModuleHandle("ntdll.dll");
    if (hNtdll != NULL)
    {
	pRtlGetVersion =
	    (PfnRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
	pRtlGetVersion(&osver);
	ver = MAKE_VER(min(osver.dwMajorVersion, 255),
		       min(osver.dwMinorVersion, 255),
		       min(osver.dwBuildNumber, 32767));
    }
    return ver;
}

#ifndef FEAT_GUI_MSWIN
/*
 * Version of ReadConsoleInput() that works with IME.
 * Works around problems on Windows 8.
 */
    static BOOL
read_console_input(
    HANDLE	    hInput,
    INPUT_RECORD    *lpBuffer,
    DWORD	    nLength,
    LPDWORD	    lpEvents)
{
    enum
    {
	IRSIZE = 10
    };
    static INPUT_RECORD s_irCache[IRSIZE];
    static DWORD s_dwIndex = 0;
    static DWORD s_dwMax = 0;
    DWORD dwEvents;
    int head;
    int tail;
    int i;

    if (nLength == -2)
	return (s_dwMax > 0) ? TRUE : FALSE;

    if (!win8_or_later)
    {
	if (nLength == -1)
	    return PeekConsoleInputW(hInput, lpBuffer, 1, lpEvents);
	return ReadConsoleInputW(hInput, lpBuffer, 1, &dwEvents);
    }

    if (s_dwMax == 0)
    {
	if (nLength == -1)
	    return PeekConsoleInputW(hInput, lpBuffer, 1, lpEvents);
	if (!ReadConsoleInputW(hInput, s_irCache, IRSIZE, &dwEvents))
	    return FALSE;
	s_dwIndex = 0;
	s_dwMax = dwEvents;
	if (dwEvents == 0)
	{
	    *lpEvents = 0;
	    return TRUE;
	}

	if (s_dwMax > 1)
	{
	    head = 0;
	    tail = s_dwMax - 1;
	    while (head != tail)
	    {
		if (s_irCache[head].EventType == WINDOW_BUFFER_SIZE_EVENT
			&& s_irCache[head + 1].EventType
						  == WINDOW_BUFFER_SIZE_EVENT)
		{
		    /* Remove duplicate event to avoid flicker. */
		    for (i = head; i < tail; ++i)
			s_irCache[i] = s_irCache[i + 1];
		    --tail;
		    continue;
		}
		head++;
	    }
	    s_dwMax = tail + 1;
	}
    }

    *lpBuffer = s_irCache[s_dwIndex];
    if (!(nLength == -1 || nLength == -2) && ++s_dwIndex >= s_dwMax)
	s_dwMax = 0;
    *lpEvents = 1;
    return TRUE;
}

/*
 * Version of PeekConsoleInput() that works with IME.
 */
    static BOOL
peek_console_input(
    HANDLE	    hInput,
    INPUT_RECORD    *lpBuffer,
    DWORD	    nLength,
    LPDWORD	    lpEvents)
{
    return read_console_input(hInput, lpBuffer, -1, lpEvents);
}

# ifdef FEAT_CLIENTSERVER
    static DWORD
msg_wait_for_multiple_objects(
    DWORD    nCount,
    LPHANDLE pHandles,
    BOOL     fWaitAll,
    DWORD    dwMilliseconds,
    DWORD    dwWakeMask)
{
    if (read_console_input(NULL, NULL, -2, NULL))
	return WAIT_OBJECT_0;
    return MsgWaitForMultipleObjects(nCount, pHandles, fWaitAll,
				     dwMilliseconds, dwWakeMask);
}
# endif

# ifndef FEAT_CLIENTSERVER
    static DWORD
wait_for_single_object(
    HANDLE hHandle,
    DWORD dwMilliseconds)
{
    if (read_console_input(NULL, NULL, -2, NULL))
	return WAIT_OBJECT_0;
    return WaitForSingleObject(hHandle, dwMilliseconds);
}
# endif
#endif

    static void
get_exe_name(void)
{
    /* Maximum length of $PATH is more than MAXPATHL.  8191 is often mentioned
     * as the maximum length that works (plus a NUL byte). */
#define MAX_ENV_PATH_LEN 8192
    char	temp[MAX_ENV_PATH_LEN];
    char_u	*p;

    if (exe_name == NULL)
    {
	/* store the name of the executable, may be used for $VIM */
	GetModuleFileName(NULL, temp, MAX_ENV_PATH_LEN - 1);
	if (*temp != NUL)
	    exe_name = FullName_save((char_u *)temp, FALSE);
    }

    if (exe_path == NULL && exe_name != NULL)
    {
	exe_path = vim_strnsave(exe_name,
				     (int)(gettail_sep(exe_name) - exe_name));
	if (exe_path != NULL)
	{
	    /* Append our starting directory to $PATH, so that when doing
	     * "!xxd" it's found in our starting directory.  Needed because
	     * SearchPath() also looks there. */
	    p = mch_getenv("PATH");
	    if (p == NULL
		       || STRLEN(p) + STRLEN(exe_path) + 2 < MAX_ENV_PATH_LEN)
	    {
		if (p == NULL || *p == NUL)
		    temp[0] = NUL;
		else
		{
		    STRCPY(temp, p);
		    STRCAT(temp, ";");
		}
		STRCAT(temp, exe_path);
		vim_setenv((char_u *)"PATH", (char_u *)temp);
	    }
	}
    }
}

/*
 * Unescape characters in "p" that appear in "escaped".
 */
    static void
unescape_shellxquote(char_u *p, char_u *escaped)
{
    int	    l = (int)STRLEN(p);
    int	    n;

    while (*p != NUL)
    {
	if (*p == '^' && vim_strchr(escaped, p[1]) != NULL)
	    mch_memmove(p, p + 1, l--);
	n = (*mb_ptr2len)(p);
	p += n;
	l -= n;
    }
}

/*
 * Load library "name".
 */
    HINSTANCE
vimLoadLib(char *name)
{
    HINSTANCE	dll = NULL;

    /* NOTE: Do not use mch_dirname() and mch_chdir() here, they may call
     * vimLoadLib() recursively, which causes a stack overflow. */
    if (exe_path == NULL)
	get_exe_name();
    if (exe_path != NULL)
    {
	WCHAR old_dirw[MAXPATHL];

	if (GetCurrentDirectoryW(MAXPATHL, old_dirw) != 0)
	{
	    /* Change directory to where the executable is, both to make
	     * sure we find a .dll there and to avoid looking for a .dll
	     * in the current directory. */
	    SetCurrentDirectory((LPCSTR)exe_path);
	    dll = LoadLibrary(name);
	    SetCurrentDirectoryW(old_dirw);
	    return dll;
	}
    }
    return dll;
}

#if defined(DYNAMIC_ICONV) || defined(DYNAMIC_GETTEXT) || defined(PROTO)
/*
 * Get related information about 'funcname' which is imported by 'hInst'.
 * If 'info' is 0, return the function address.
 * If 'info' is 1, return the module name which the function is imported from.
 */
    static void *
get_imported_func_info(HINSTANCE hInst, const char *funcname, int info)
{
    PBYTE			pImage = (PBYTE)hInst;
    PIMAGE_DOS_HEADER		pDOS = (PIMAGE_DOS_HEADER)hInst;
    PIMAGE_NT_HEADERS		pPE;
    PIMAGE_IMPORT_DESCRIPTOR	pImpDesc;
    PIMAGE_THUNK_DATA		pIAT;	    /* Import Address Table */
    PIMAGE_THUNK_DATA		pINT;	    /* Import Name Table */
    PIMAGE_IMPORT_BY_NAME	pImpName;

    if (pDOS->e_magic != IMAGE_DOS_SIGNATURE)
	return NULL;
    pPE = (PIMAGE_NT_HEADERS)(pImage + pDOS->e_lfanew);
    if (pPE->Signature != IMAGE_NT_SIGNATURE)
	return NULL;
    pImpDesc = (PIMAGE_IMPORT_DESCRIPTOR)(pImage
	    + pPE->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
							    .VirtualAddress);
    for (; pImpDesc->FirstThunk; ++pImpDesc)
    {
	if (!pImpDesc->OriginalFirstThunk)
	    continue;
	pIAT = (PIMAGE_THUNK_DATA)(pImage + pImpDesc->FirstThunk);
	pINT = (PIMAGE_THUNK_DATA)(pImage + pImpDesc->OriginalFirstThunk);
	for (; pIAT->u1.Function; ++pIAT, ++pINT)
	{
	    if (IMAGE_SNAP_BY_ORDINAL(pINT->u1.Ordinal))
		continue;
	    pImpName = (PIMAGE_IMPORT_BY_NAME)(pImage
					+ (UINT_PTR)(pINT->u1.AddressOfData));
	    if (strcmp((char *)pImpName->Name, funcname) == 0)
	    {
		switch (info)
		{
		    case 0:
			return (void *)pIAT->u1.Function;
		    case 1:
			return (void *)(pImage + pImpDesc->Name);
		    default:
			return NULL;
		}
	    }
	}
    }
    return NULL;
}

/*
 * Get the module handle which 'funcname' in 'hInst' is imported from.
 */
    HINSTANCE
find_imported_module_by_funcname(HINSTANCE hInst, const char *funcname)
{
    char    *modulename;

    modulename = (char *)get_imported_func_info(hInst, funcname, 1);
    if (modulename != NULL)
	return GetModuleHandleA(modulename);
    return NULL;
}

/*
 * Get the address of 'funcname' which is imported by 'hInst' DLL.
 */
    void *
get_dll_import_func(HINSTANCE hInst, const char *funcname)
{
    return get_imported_func_info(hInst, funcname, 0);
}
#endif

#if defined(DYNAMIC_GETTEXT) || defined(PROTO)
# ifndef GETTEXT_DLL
#  define GETTEXT_DLL "libintl.dll"
#  define GETTEXT_DLL_ALT1 "libintl-8.dll"
#  define GETTEXT_DLL_ALT2 "intl.dll"
# endif
/* Dummy functions */
static char *null_libintl_gettext(const char *);
static char *null_libintl_ngettext(const char *, const char *, unsigned long n);
static char *null_libintl_textdomain(const char *);
static char *null_libintl_bindtextdomain(const char *, const char *);
static char *null_libintl_bind_textdomain_codeset(const char *, const char *);
static int null_libintl_putenv(const char *);
static int null_libintl_wputenv(const wchar_t *);

static HINSTANCE hLibintlDLL = NULL;
char *(*dyn_libintl_gettext)(const char *) = null_libintl_gettext;
char *(*dyn_libintl_ngettext)(const char *, const char *, unsigned long n)
						= null_libintl_ngettext;
char *(*dyn_libintl_textdomain)(const char *) = null_libintl_textdomain;
char *(*dyn_libintl_bindtextdomain)(const char *, const char *)
						= null_libintl_bindtextdomain;
char *(*dyn_libintl_bind_textdomain_codeset)(const char *, const char *)
				       = null_libintl_bind_textdomain_codeset;
int (*dyn_libintl_putenv)(const char *) = null_libintl_putenv;
int (*dyn_libintl_wputenv)(const wchar_t *) = null_libintl_wputenv;

    int
dyn_libintl_init(void)
{
    int i;
    static struct
    {
	char	    *name;
	FARPROC	    *ptr;
    } libintl_entry[] =
    {
	{"gettext", (FARPROC*)&dyn_libintl_gettext},
	{"ngettext", (FARPROC*)&dyn_libintl_ngettext},
	{"textdomain", (FARPROC*)&dyn_libintl_textdomain},
	{"bindtextdomain", (FARPROC*)&dyn_libintl_bindtextdomain},
	{NULL, NULL}
    };
    HINSTANCE hmsvcrt;

    // No need to initialize twice.
    if (hLibintlDLL != NULL)
	return 1;
    // Load gettext library (libintl.dll and other names).
    hLibintlDLL = vimLoadLib(GETTEXT_DLL);
#ifdef GETTEXT_DLL_ALT1
    if (!hLibintlDLL)
	hLibintlDLL = vimLoadLib(GETTEXT_DLL_ALT1);
#endif
#ifdef GETTEXT_DLL_ALT2
    if (!hLibintlDLL)
	hLibintlDLL = vimLoadLib(GETTEXT_DLL_ALT2);
#endif
    if (!hLibintlDLL)
    {
	if (p_verbose > 0)
	{
	    verbose_enter();
	    semsg(_(e_loadlib), GETTEXT_DLL);
	    verbose_leave();
	}
	return 0;
    }
    for (i = 0; libintl_entry[i].name != NULL
					 && libintl_entry[i].ptr != NULL; ++i)
    {
	if ((*libintl_entry[i].ptr = (FARPROC)GetProcAddress(hLibintlDLL,
					      libintl_entry[i].name)) == NULL)
	{
	    dyn_libintl_end();
	    if (p_verbose > 0)
	    {
		verbose_enter();
		semsg(_(e_loadfunc), libintl_entry[i].name);
		verbose_leave();
	    }
	    return 0;
	}
    }

    /* The bind_textdomain_codeset() function is optional. */
    dyn_libintl_bind_textdomain_codeset = (void *)GetProcAddress(hLibintlDLL,
						   "bind_textdomain_codeset");
    if (dyn_libintl_bind_textdomain_codeset == NULL)
	dyn_libintl_bind_textdomain_codeset =
					 null_libintl_bind_textdomain_codeset;

    /* _putenv() function for the libintl.dll is optional. */
    hmsvcrt = find_imported_module_by_funcname(hLibintlDLL, "getenv");
    if (hmsvcrt != NULL)
    {
	dyn_libintl_putenv = (void *)GetProcAddress(hmsvcrt, "_putenv");
	dyn_libintl_wputenv = (void *)GetProcAddress(hmsvcrt, "_wputenv");
    }
    if (dyn_libintl_putenv == NULL || dyn_libintl_putenv == _putenv)
	dyn_libintl_putenv = null_libintl_putenv;
    if (dyn_libintl_wputenv == NULL || dyn_libintl_wputenv == _wputenv)
	dyn_libintl_wputenv = null_libintl_wputenv;

    return 1;
}

    void
dyn_libintl_end(void)
{
    if (hLibintlDLL)
	FreeLibrary(hLibintlDLL);
    hLibintlDLL			= NULL;
    dyn_libintl_gettext		= null_libintl_gettext;
    dyn_libintl_ngettext	= null_libintl_ngettext;
    dyn_libintl_textdomain	= null_libintl_textdomain;
    dyn_libintl_bindtextdomain	= null_libintl_bindtextdomain;
    dyn_libintl_bind_textdomain_codeset = null_libintl_bind_textdomain_codeset;
    dyn_libintl_putenv		= null_libintl_putenv;
    dyn_libintl_wputenv		= null_libintl_wputenv;
}

    static char *
null_libintl_gettext(const char *msgid)
{
    return (char*)msgid;
}

    static char *
null_libintl_ngettext(
	const char *msgid,
	const char *msgid_plural,
	unsigned long n)
{
    return (char *)(n == 1 ? msgid : msgid_plural);
}

    static char *
null_libintl_bindtextdomain(
	const char *domainname UNUSED,
	const char *dirname UNUSED)
{
    return NULL;
}

    static char *
null_libintl_bind_textdomain_codeset(
	const char *domainname UNUSED,
	const char *codeset UNUSED)
{
    return NULL;
}

    static char *
null_libintl_textdomain(const char *domainname UNUSED)
{
    return NULL;
}

    static int
null_libintl_putenv(const char *envstring UNUSED)
{
    return 0;
}

    static int
null_libintl_wputenv(const wchar_t *envstring UNUSED)
{
    return 0;
}

#endif /* DYNAMIC_GETTEXT */

/* This symbol is not defined in older versions of the SDK or Visual C++ */

#ifndef VER_PLATFORM_WIN32_WINDOWS
# define VER_PLATFORM_WIN32_WINDOWS 1
#endif

DWORD g_PlatformId;

#ifdef HAVE_ACL
# ifndef PROTO
#  include <aclapi.h>
# endif
# ifndef PROTECTED_DACL_SECURITY_INFORMATION
#  define PROTECTED_DACL_SECURITY_INFORMATION	0x80000000L
# endif
#endif

#ifdef HAVE_ACL
/*
 * Enables or disables the specified privilege.
 */
    static BOOL
win32_enable_privilege(LPTSTR lpszPrivilege, BOOL bEnable)
{
    BOOL		bResult;
    LUID		luid;
    HANDLE		hToken;
    TOKEN_PRIVILEGES	tokenPrivileges;

    if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	return FALSE;

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
    {
	CloseHandle(hToken);
	return FALSE;
    }

    tokenPrivileges.PrivilegeCount	     = 1;
    tokenPrivileges.Privileges[0].Luid       = luid;
    tokenPrivileges.Privileges[0].Attributes = bEnable ?
						    SE_PRIVILEGE_ENABLED : 0;

    bResult = AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges,
	    sizeof(TOKEN_PRIVILEGES), NULL, NULL);

    CloseHandle(hToken);

    return bResult && GetLastError() == ERROR_SUCCESS;
}
#endif

/*
 * Set g_PlatformId to VER_PLATFORM_WIN32_NT (NT) or
 * VER_PLATFORM_WIN32_WINDOWS (Win95).
 */
    void
PlatformId(void)
{
    static int done = FALSE;

    if (!done)
    {
	OSVERSIONINFO ovi;

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	GetVersionEx(&ovi);

	g_PlatformId = ovi.dwPlatformId;

	if ((ovi.dwMajorVersion == 6 && ovi.dwMinorVersion >= 2)
		|| ovi.dwMajorVersion > 6)
	    win8_or_later = TRUE;

#ifdef HAVE_ACL
	/* Enable privilege for getting or setting SACLs. */
	win32_enable_privilege(SE_SECURITY_NAME, TRUE);
#endif
	done = TRUE;
    }
}

#ifndef FEAT_GUI_MSWIN

#define SHIFT  (SHIFT_PRESSED)
#define CTRL   (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)
#define ALT    (RIGHT_ALT_PRESSED  | LEFT_ALT_PRESSED)
#define ALT_GR (RIGHT_ALT_PRESSED  | LEFT_CTRL_PRESSED)


/* When uChar.AsciiChar is 0, then we need to look at wVirtualKeyCode.
 * We map function keys to their ANSI terminal equivalents, as produced
 * by ANSI.SYS, for compatibility with the MS-DOS version of Vim.  Any
 * ANSI key with a value >= '\300' is nonstandard, but provided anyway
 * so that the user can have access to all SHIFT-, CTRL-, and ALT-
 * combinations of function/arrow/etc keys.
 */

static const struct
{
    WORD    wVirtKey;
    BOOL    fAnsiKey;
    int	    chAlone;
    int	    chShift;
    int	    chCtrl;
    int	    chAlt;
} VirtKeyMap[] =
{
//    Key	ANSI	alone	shift	ctrl	    alt
    { VK_ESCAPE,FALSE,	ESC,	ESC,	ESC,	    ESC,    },

    { VK_F1,	TRUE,	';',	'T',	'^',	    'h', },
    { VK_F2,	TRUE,	'<',	'U',	'_',	    'i', },
    { VK_F3,	TRUE,	'=',	'V',	'`',	    'j', },
    { VK_F4,	TRUE,	'>',	'W',	'a',	    'k', },
    { VK_F5,	TRUE,	'?',	'X',	'b',	    'l', },
    { VK_F6,	TRUE,	'@',	'Y',	'c',	    'm', },
    { VK_F7,	TRUE,	'A',	'Z',	'd',	    'n', },
    { VK_F8,	TRUE,	'B',	'[',	'e',	    'o', },
    { VK_F9,	TRUE,	'C',	'\\',	'f',	    'p', },
    { VK_F10,	TRUE,	'D',	']',	'g',	    'q', },
    { VK_F11,	TRUE,	'\205',	'\207',	'\211',	    '\213', },
    { VK_F12,	TRUE,	'\206',	'\210',	'\212',	    '\214', },

    { VK_HOME,	TRUE,	'G',	'\302',	'w',	    '\303', },
    { VK_UP,	TRUE,	'H',	'\304',	'\305',	    '\306', },
    { VK_PRIOR,	TRUE,	'I',	'\307',	'\204',	    '\310', }, // PgUp
    { VK_LEFT,	TRUE,	'K',	'\311',	's',	    '\312', },
    { VK_RIGHT,	TRUE,	'M',	'\313',	't',	    '\314', },
    { VK_END,	TRUE,	'O',	'\315',	'u',	    '\316', },
    { VK_DOWN,	TRUE,	'P',	'\317',	'\320',	    '\321', },
    { VK_NEXT,	TRUE,	'Q',	'\322',	'v',	    '\323', }, // PgDn
    { VK_INSERT,TRUE,	'R',	'\324',	'\325',	    '\326', },
    { VK_DELETE,TRUE,	'S',	'\327',	'\330',	    '\331', },

    { VK_SNAPSHOT,TRUE,	0,	0,	0,	    'r', }, // PrtScrn

#if 0
    // Most people don't have F13-F20, but what the hell...
    { VK_F13,	TRUE,	'\332',	'\333',	'\334',	    '\335', },
    { VK_F14,	TRUE,	'\336',	'\337',	'\340',	    '\341', },
    { VK_F15,	TRUE,	'\342',	'\343',	'\344',	    '\345', },
    { VK_F16,	TRUE,	'\346',	'\347',	'\350',	    '\351', },
    { VK_F17,	TRUE,	'\352',	'\353',	'\354',	    '\355', },
    { VK_F18,	TRUE,	'\356',	'\357',	'\360',	    '\361', },
    { VK_F19,	TRUE,	'\362',	'\363',	'\364',	    '\365', },
    { VK_F20,	TRUE,	'\366',	'\367',	'\370',	    '\371', },
#endif
    { VK_ADD,	TRUE,   'N',    'N',    'N',	'N',	}, // keyp '+'
    { VK_SUBTRACT, TRUE,'J',	'J',    'J',	'J',	}, // keyp '-'
 // { VK_DIVIDE,   TRUE,'N',	'N',    'N',	'N',	}, // keyp '/'
    { VK_MULTIPLY, TRUE,'7',	'7',    '7',	'7',	}, // keyp '*'

    { VK_NUMPAD0,TRUE,  '\332',	'\333',	'\334',	    '\335', },
    { VK_NUMPAD1,TRUE,  '\336',	'\337',	'\340',	    '\341', },
    { VK_NUMPAD2,TRUE,  '\342',	'\343',	'\344',	    '\345', },
    { VK_NUMPAD3,TRUE,  '\346',	'\347',	'\350',	    '\351', },
    { VK_NUMPAD4,TRUE,  '\352',	'\353',	'\354',	    '\355', },
    { VK_NUMPAD5,TRUE,  '\356',	'\357',	'\360',	    '\361', },
    { VK_NUMPAD6,TRUE,  '\362',	'\363',	'\364',	    '\365', },
    { VK_NUMPAD7,TRUE,  '\366',	'\367',	'\370',	    '\371', },
    { VK_NUMPAD8,TRUE,  '\372',	'\373',	'\374',	    '\375', },
    // Sorry, out of number space! <negri>
    { VK_NUMPAD9,TRUE,  '\376',	'\377',	'\377',	    '\367', },

};


#ifdef _MSC_VER
// The ToAscii bug destroys several registers.	Need to turn off optimization
// or the GetConsoleKeyboardLayoutName hack will fail in non-debug versions
# pragma warning(push)
# pragma warning(disable: 4748)
# pragma optimize("", off)
#endif

#if defined(__GNUC__) && !defined(__MINGW32__)  && !defined(__CYGWIN__)
# define UChar UnicodeChar
#else
# define UChar uChar.UnicodeChar
#endif

/* The return code indicates key code size. */
    static int
#ifdef __BORLANDC__
    __stdcall
#endif
win32_kbd_patch_key(
    KEY_EVENT_RECORD *pker)
{
    UINT uMods = pker->dwControlKeyState;
    static int s_iIsDead = 0;
    static WORD awAnsiCode[2];
    static BYTE abKeystate[256];


    if (s_iIsDead == 2)
    {
	pker->UChar = (WCHAR) awAnsiCode[1];
	s_iIsDead = 0;
	return 1;
    }

    if (pker->UChar != 0)
	return 1;

    vim_memset(abKeystate, 0, sizeof (abKeystate));

    /* Clear any pending dead keys */
    ToUnicode(VK_SPACE, MapVirtualKey(VK_SPACE, 0), abKeystate, awAnsiCode, 2, 0);

    if (uMods & SHIFT_PRESSED)
	abKeystate[VK_SHIFT] = 0x80;
    if (uMods & CAPSLOCK_ON)
	abKeystate[VK_CAPITAL] = 1;

    if ((uMods & ALT_GR) == ALT_GR)
    {
	abKeystate[VK_CONTROL] = abKeystate[VK_LCONTROL] =
	    abKeystate[VK_MENU] = abKeystate[VK_RMENU] = 0x80;
    }

    s_iIsDead = ToUnicode(pker->wVirtualKeyCode, pker->wVirtualScanCode,
			abKeystate, awAnsiCode, 2, 0);

    if (s_iIsDead > 0)
	pker->UChar = (WCHAR) awAnsiCode[0];

    return s_iIsDead;
}

#ifdef _MSC_VER
/* MUST switch optimization on again here, otherwise a call to
 * decode_key_event() may crash (e.g. when hitting caps-lock) */
# pragma optimize("", on)
# pragma warning(pop)

# if (_MSC_VER < 1100)
/* MUST turn off global optimisation for this next function, or
 * pressing ctrl-minus in insert mode crashes Vim when built with
 * VC4.1. -- negri. */
#  pragma optimize("g", off)
# endif
#endif

static BOOL g_fJustGotFocus = FALSE;

/*
 * Decode a KEY_EVENT into one or two keystrokes
 */
    static BOOL
decode_key_event(
    KEY_EVENT_RECORD	*pker,
    WCHAR		*pch,
    WCHAR		*pch2,
    int			*pmodifiers,
    BOOL		fDoPost)
{
    int i;
    const int nModifs = pker->dwControlKeyState & (SHIFT | ALT | CTRL);

    *pch = *pch2 = NUL;
    g_fJustGotFocus = FALSE;

    /* ignore key up events */
    if (!pker->bKeyDown)
	return FALSE;

    /* ignore some keystrokes */
    switch (pker->wVirtualKeyCode)
    {
    /* modifiers */
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:   /* Alt key */
	return FALSE;

    default:
	break;
    }

    /* special cases */
    if ((nModifs & CTRL) != 0 && (nModifs & ~CTRL) == 0 && pker->UChar == NUL)
    {
	/* Ctrl-6 is Ctrl-^ */
	if (pker->wVirtualKeyCode == '6')
	{
	    *pch = Ctrl_HAT;
	    return TRUE;
	}
	/* Ctrl-2 is Ctrl-@ */
	else if (pker->wVirtualKeyCode == '2')
	{
	    *pch = NUL;
	    return TRUE;
	}
	/* Ctrl-- is Ctrl-_ */
	else if (pker->wVirtualKeyCode == 0xBD)
	{
	    *pch = Ctrl__;
	    return TRUE;
	}
    }

    /* Shift-TAB */
    if (pker->wVirtualKeyCode == VK_TAB && (nModifs & SHIFT_PRESSED))
    {
	*pch = K_NUL;
	*pch2 = '\017';
	return TRUE;
    }

    for (i = sizeof(VirtKeyMap) / sizeof(VirtKeyMap[0]);  --i >= 0;  )
    {
	if (VirtKeyMap[i].wVirtKey == pker->wVirtualKeyCode)
	{
	    if (nModifs == 0)
		*pch = VirtKeyMap[i].chAlone;
	    else if ((nModifs & SHIFT) != 0 && (nModifs & ~SHIFT) == 0)
		*pch = VirtKeyMap[i].chShift;
	    else if ((nModifs & CTRL) != 0 && (nModifs & ~CTRL) == 0)
		*pch = VirtKeyMap[i].chCtrl;
	    else if ((nModifs & ALT) != 0 && (nModifs & ~ALT) == 0)
		*pch = VirtKeyMap[i].chAlt;

	    if (*pch != 0)
	    {
		if (VirtKeyMap[i].fAnsiKey)
		{
		    *pch2 = *pch;
		    *pch = K_NUL;
		}

		return TRUE;
	    }
	}
    }

    i = win32_kbd_patch_key(pker);

    if (i < 0)
	*pch = NUL;
    else
    {
	*pch = (i > 0) ? pker->UChar : NUL;

	if (pmodifiers != NULL)
	{
	    /* Pass on the ALT key as a modifier, but only when not combined
	     * with CTRL (which is ALTGR). */
	    if ((nModifs & ALT) != 0 && (nModifs & CTRL) == 0)
		*pmodifiers |= MOD_MASK_ALT;

	    /* Pass on SHIFT only for special keys, because we don't know when
	     * it's already included with the character. */
	    if ((nModifs & SHIFT) != 0 && *pch <= 0x20)
		*pmodifiers |= MOD_MASK_SHIFT;

	    /* Pass on CTRL only for non-special keys, because we don't know
	     * when it's already included with the character.  And not when
	     * combined with ALT (which is ALTGR). */
	    if ((nModifs & CTRL) != 0 && (nModifs & ALT) == 0
					       && *pch >= 0x20 && *pch < 0x80)
		*pmodifiers |= MOD_MASK_CTRL;
	}
    }

    return (*pch != NUL);
}

#ifdef _MSC_VER
# pragma optimize("", on)
#endif

#endif /* FEAT_GUI_MSWIN */


#ifdef FEAT_MOUSE

/*
 * For the GUI the mouse handling is in gui_w32.c.
 */
# ifdef FEAT_GUI_MSWIN
    void
mch_setmouse(int on UNUSED)
{
}
# else
static int g_fMouseAvail = FALSE;   /* mouse present */
static int g_fMouseActive = FALSE;  /* mouse enabled */
static int g_nMouseClick = -1;	    /* mouse status */
static int g_xMouse;		    /* mouse x coordinate */
static int g_yMouse;		    /* mouse y coordinate */

/*
 * Enable or disable mouse input
 */
    void
mch_setmouse(int on)
{
    DWORD cmodein;

    if (!g_fMouseAvail)
	return;

    g_fMouseActive = on;
    GetConsoleMode(g_hConIn, &cmodein);

    if (g_fMouseActive)
	cmodein |= ENABLE_MOUSE_INPUT;
    else
	cmodein &= ~ENABLE_MOUSE_INPUT;

    SetConsoleMode(g_hConIn, cmodein);
}


#if defined(FEAT_BEVAL_TERM) || defined(PROTO)
/*
 * Called when 'balloonevalterm' changed.
 */
    void
mch_bevalterm_changed(void)
{
    mch_setmouse(g_fMouseActive);
}
#endif

/*
 * Decode a MOUSE_EVENT.  If it's a valid event, return MOUSE_LEFT,
 * MOUSE_MIDDLE, or MOUSE_RIGHT for a click; MOUSE_DRAG for a mouse
 * move with a button held down; and MOUSE_RELEASE after a MOUSE_DRAG
 * or a MOUSE_LEFT, _MIDDLE, or _RIGHT.  We encode the button type,
 * the number of clicks, and the Shift/Ctrl/Alt modifiers in g_nMouseClick,
 * and we return the mouse position in g_xMouse and g_yMouse.
 *
 * Every MOUSE_LEFT, _MIDDLE, or _RIGHT will be followed by zero or more
 * MOUSE_DRAGs and one MOUSE_RELEASE.  MOUSE_RELEASE will be followed only
 * by MOUSE_LEFT, _MIDDLE, or _RIGHT.
 *
 * For multiple clicks, we send, say, MOUSE_LEFT/1 click, MOUSE_RELEASE,
 * MOUSE_LEFT/2 clicks, MOUSE_RELEASE, MOUSE_LEFT/3 clicks, MOUSE_RELEASE, ....
 *
 * Windows will send us MOUSE_MOVED notifications whenever the mouse
 * moves, even if it stays within the same character cell.  We ignore
 * all MOUSE_MOVED messages if the position hasn't really changed, and
 * we ignore all MOUSE_MOVED messages where no button is held down (i.e.,
 * we're only interested in MOUSE_DRAG).
 *
 * All of this is complicated by the code that fakes MOUSE_MIDDLE on
 * 2-button mouses by pressing the left & right buttons simultaneously.
 * In practice, it's almost impossible to click both at the same time,
 * so we need to delay a little.  Also, we tend not to get MOUSE_RELEASE
 * in such cases, if the user is clicking quickly.
 */
    static BOOL
decode_mouse_event(
    MOUSE_EVENT_RECORD *pmer)
{
    static int s_nOldButton = -1;
    static int s_nOldMouseClick = -1;
    static int s_xOldMouse = -1;
    static int s_yOldMouse = -1;
    static linenr_T s_old_topline = 0;
#ifdef FEAT_DIFF
    static int s_old_topfill = 0;
#endif
    static int s_cClicks = 1;
    static BOOL s_fReleased = TRUE;
    static DWORD s_dwLastClickTime = 0;
    static BOOL s_fNextIsMiddle = FALSE;

    static DWORD cButtons = 0;	/* number of buttons supported */

    const DWORD LEFT = FROM_LEFT_1ST_BUTTON_PRESSED;
    const DWORD MIDDLE = FROM_LEFT_2ND_BUTTON_PRESSED;
    const DWORD RIGHT = RIGHTMOST_BUTTON_PRESSED;
    const DWORD LEFT_RIGHT = LEFT | RIGHT;

    int nButton;

    if (cButtons == 0 && !GetNumberOfConsoleMouseButtons(&cButtons))
	cButtons = 2;

    if (!g_fMouseAvail || !g_fMouseActive)
    {
	g_nMouseClick = -1;
	return FALSE;
    }

    /* get a spurious MOUSE_EVENT immediately after receiving focus; ignore */
    if (g_fJustGotFocus)
    {
	g_fJustGotFocus = FALSE;
	return FALSE;
    }

    /* unprocessed mouse click? */
    if (g_nMouseClick != -1)
	return TRUE;

    nButton = -1;
    g_xMouse = pmer->dwMousePosition.X;
    g_yMouse = pmer->dwMousePosition.Y;

    if (pmer->dwEventFlags == MOUSE_MOVED)
    {
	/* Ignore MOUSE_MOVED events if (x, y) hasn't changed.	(We get these
	 * events even when the mouse moves only within a char cell.) */
	if (s_xOldMouse == g_xMouse && s_yOldMouse == g_yMouse)
	    return FALSE;
    }

    /* If no buttons are pressed... */
    if ((pmer->dwButtonState & ((1 << cButtons) - 1)) == 0)
    {
	nButton = MOUSE_RELEASE;

	/* If the last thing returned was MOUSE_RELEASE, ignore this */
	if (s_fReleased)
	{
#ifdef FEAT_BEVAL_TERM
	    /* do return mouse move events when we want them */
	    if (p_bevalterm)
		nButton = MOUSE_DRAG;
	    else
#endif
		return FALSE;
	}

	s_fReleased = TRUE;
    }
    else    /* one or more buttons pressed */
    {
	/* on a 2-button mouse, hold down left and right buttons
	 * simultaneously to get MIDDLE. */

	if (cButtons == 2 && s_nOldButton != MOUSE_DRAG)
	{
	    DWORD dwLR = (pmer->dwButtonState & LEFT_RIGHT);

	    /* if either left or right button only is pressed, see if the
	     * next mouse event has both of them pressed */
	    if (dwLR == LEFT || dwLR == RIGHT)
	    {
		for (;;)
		{
		    /* wait a short time for next input event */
		    if (WaitForSingleObject(g_hConIn, p_mouset / 3)
							     != WAIT_OBJECT_0)
			break;
		    else
		    {
			DWORD cRecords = 0;
			INPUT_RECORD ir;
			MOUSE_EVENT_RECORD* pmer2 = &ir.Event.MouseEvent;

			peek_console_input(g_hConIn, &ir, 1, &cRecords);

			if (cRecords == 0 || ir.EventType != MOUSE_EVENT
				|| !(pmer2->dwButtonState & LEFT_RIGHT))
			    break;
			else
			{
			    if (pmer2->dwEventFlags != MOUSE_MOVED)
			    {
				read_console_input(g_hConIn, &ir, 1, &cRecords);

				return decode_mouse_event(pmer2);
			    }
			    else if (s_xOldMouse == pmer2->dwMousePosition.X &&
				     s_yOldMouse == pmer2->dwMousePosition.Y)
			    {
				/* throw away spurious mouse move */
				read_console_input(g_hConIn, &ir, 1, &cRecords);

				/* are there any more mouse events in queue? */
				peek_console_input(g_hConIn, &ir, 1, &cRecords);

				if (cRecords==0 || ir.EventType != MOUSE_EVENT)
				    break;
			    }
			    else
				break;
			}
		    }
		}
	    }
	}

	if (s_fNextIsMiddle)
	{
	    nButton = (pmer->dwEventFlags == MOUSE_MOVED)
		? MOUSE_DRAG : MOUSE_MIDDLE;
	    s_fNextIsMiddle = FALSE;
	}
	else if (cButtons == 2	&&
	    ((pmer->dwButtonState & LEFT_RIGHT) == LEFT_RIGHT))
	{
	    nButton = MOUSE_MIDDLE;

	    if (! s_fReleased && pmer->dwEventFlags != MOUSE_MOVED)
	    {
		s_fNextIsMiddle = TRUE;
		nButton = MOUSE_RELEASE;
	    }
	}
	else if ((pmer->dwButtonState & LEFT) == LEFT)
	    nButton = MOUSE_LEFT;
	else if ((pmer->dwButtonState & MIDDLE) == MIDDLE)
	    nButton = MOUSE_MIDDLE;
	else if ((pmer->dwButtonState & RIGHT) == RIGHT)
	    nButton = MOUSE_RIGHT;

	if (! s_fReleased && ! s_fNextIsMiddle
		&& nButton != s_nOldButton && s_nOldButton != MOUSE_DRAG)
	    return FALSE;

	s_fReleased = s_fNextIsMiddle;
    }

    if (pmer->dwEventFlags == 0 || pmer->dwEventFlags == DOUBLE_CLICK)
    {
	/* button pressed or released, without mouse moving */
	if (nButton != -1 && nButton != MOUSE_RELEASE)
	{
	    DWORD dwCurrentTime = GetTickCount();

	    if (s_xOldMouse != g_xMouse
		    || s_yOldMouse != g_yMouse
		    || s_nOldButton != nButton
		    || s_old_topline != curwin->w_topline
#ifdef FEAT_DIFF
		    || s_old_topfill != curwin->w_topfill
#endif
		    || (int)(dwCurrentTime - s_dwLastClickTime) > p_mouset)
	    {
		s_cClicks = 1;
	    }
	    else if (++s_cClicks > 4)
	    {
		s_cClicks = 1;
	    }

	    s_dwLastClickTime = dwCurrentTime;
	}
    }
    else if (pmer->dwEventFlags == MOUSE_MOVED)
    {
	if (nButton != -1 && nButton != MOUSE_RELEASE)
	    nButton = MOUSE_DRAG;

	s_cClicks = 1;
    }

    if (nButton == -1)
	return FALSE;

    if (nButton != MOUSE_RELEASE)
	s_nOldButton = nButton;

    g_nMouseClick = nButton;

    if (pmer->dwControlKeyState & SHIFT_PRESSED)
	g_nMouseClick |= MOUSE_SHIFT;
    if (pmer->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
	g_nMouseClick |= MOUSE_CTRL;
    if (pmer->dwControlKeyState & (RIGHT_ALT_PRESSED  | LEFT_ALT_PRESSED))
	g_nMouseClick |= MOUSE_ALT;

    if (nButton != MOUSE_DRAG && nButton != MOUSE_RELEASE)
	SET_NUM_MOUSE_CLICKS(g_nMouseClick, s_cClicks);

    /* only pass on interesting (i.e., different) mouse events */
    if (s_xOldMouse == g_xMouse
	    && s_yOldMouse == g_yMouse
	    && s_nOldMouseClick == g_nMouseClick)
    {
	g_nMouseClick = -1;
	return FALSE;
    }

    s_xOldMouse = g_xMouse;
    s_yOldMouse = g_yMouse;
    s_old_topline = curwin->w_topline;
#ifdef FEAT_DIFF
    s_old_topfill = curwin->w_topfill;
#endif
    s_nOldMouseClick = g_nMouseClick;

    return TRUE;
}

# endif /* FEAT_GUI_MSWIN */
#endif /* FEAT_MOUSE */


#ifdef MCH_CURSOR_SHAPE
/*
 * Set the shape of the cursor.
 * 'thickness' can be from 1 (thin) to 99 (block)
 */
    static void
mch_set_cursor_shape(int thickness)
{
    CONSOLE_CURSOR_INFO ConsoleCursorInfo;
    ConsoleCursorInfo.dwSize = thickness;
    ConsoleCursorInfo.bVisible = s_cursor_visible;

    SetConsoleCursorInfo(g_hConOut, &ConsoleCursorInfo);
    if (s_cursor_visible)
	SetConsoleCursorPosition(g_hConOut, g_coord);
}

    void
mch_update_cursor(void)
{
    int		idx;
    int		thickness;

    /*
     * How the cursor is drawn depends on the current mode.
     */
    idx = get_shape_idx(FALSE);

    if (shape_table[idx].shape == SHAPE_BLOCK)
	thickness = 99;	/* 100 doesn't work on W95 */
    else
	thickness = shape_table[idx].percentage;
    mch_set_cursor_shape(thickness);
}
#endif

#ifndef FEAT_GUI_MSWIN	    /* this isn't used for the GUI */
/*
 * Handle FOCUS_EVENT.
 */
    static void
handle_focus_event(INPUT_RECORD ir)
{
    g_fJustGotFocus = ir.Event.FocusEvent.bSetFocus;
    ui_focus_change((int)g_fJustGotFocus);
}

static void ResizeConBuf(HANDLE hConsole, COORD coordScreen);

/*
 * Wait until console input from keyboard or mouse is available,
 * or the time is up.
 * When "ignore_input" is TRUE even wait when input is available.
 * Return TRUE if something is available FALSE if not.
 */
    static int
WaitForChar(long msec, int ignore_input)
{
    DWORD	    dwNow = 0, dwEndTime = 0;
    INPUT_RECORD    ir;
    DWORD	    cRecords;
    WCHAR	    ch, ch2;
#ifdef FEAT_TIMERS
    int		    tb_change_cnt = typebuf.tb_change_cnt;
#endif

    if (msec > 0)
	/* Wait until the specified time has elapsed. */
	dwEndTime = GetTickCount() + msec;
    else if (msec < 0)
	/* Wait forever. */
	dwEndTime = INFINITE;

    // We need to loop until the end of the time period, because
    // we might get multiple unusable mouse events in that time.
    for (;;)
    {
	// Only process messages when waiting.
	if (msec != 0)
	{
#ifdef MESSAGE_QUEUE
	    parse_queued_messages();
#endif
#ifdef FEAT_MZSCHEME
	    mzvim_check_threads();
#endif
#ifdef FEAT_CLIENTSERVER
	    serverProcessPendingMessages();
#endif
	}

	if (0
#ifdef FEAT_MOUSE
		|| g_nMouseClick != -1
#endif
#ifdef FEAT_CLIENTSERVER
		|| (!ignore_input && input_available())
#endif
	   )
	    return TRUE;

	if (msec > 0)
	{
	    /* If the specified wait time has passed, return.  Beware that
	     * GetTickCount() may wrap around (overflow). */
	    dwNow = GetTickCount();
	    if ((int)(dwNow - dwEndTime) >= 0)
		break;
	}
	if (msec != 0)
	{
	    DWORD dwWaitTime = dwEndTime - dwNow;

#ifdef FEAT_JOB_CHANNEL
	    /* Check channel while waiting for input. */
	    if (dwWaitTime > 100)
	    {
		dwWaitTime = 100;
		/* If there is readahead then parse_queued_messages() timed out
		 * and we should call it again soon. */
		if (channel_any_readahead())
		    dwWaitTime = 10;
	    }
#endif
#ifdef FEAT_BEVAL_GUI
	    if (p_beval && dwWaitTime > 100)
		/* The 'balloonexpr' may indirectly invoke a callback while
		 * waiting for a character, need to check often. */
		dwWaitTime = 100;
#endif
#ifdef FEAT_MZSCHEME
	    if (mzthreads_allowed() && p_mzq > 0
				    && (msec < 0 || (long)dwWaitTime > p_mzq))
		dwWaitTime = p_mzq; /* don't wait longer than 'mzquantum' */
#endif
#ifdef FEAT_TIMERS
	    // When waiting very briefly don't trigger timers.
	    if (dwWaitTime > 10)
	    {
		long	due_time;

		// Trigger timers and then get the time in msec until the next
		// one is due.  Wait up to that time.
		due_time = check_due_timer();
		if (typebuf.tb_change_cnt != tb_change_cnt)
		{
		    // timer may have used feedkeys().
		    return FALSE;
		}
		if (due_time > 0 && dwWaitTime > (DWORD)due_time)
		    dwWaitTime = due_time;
	    }
#endif
	    if (
#ifdef FEAT_CLIENTSERVER
		    // Wait for either an event on the console input or a
		    // message in the client-server window.
		    msg_wait_for_multiple_objects(1, &g_hConIn, FALSE,
				  dwWaitTime, QS_SENDMESSAGE) != WAIT_OBJECT_0
#else
		    wait_for_single_object(g_hConIn, dwWaitTime)
							      != WAIT_OBJECT_0
#endif
		    )
		continue;
	}

	cRecords = 0;
	peek_console_input(g_hConIn, &ir, 1, &cRecords);

#ifdef FEAT_MBYTE_IME
	if (State & CMDLINE && msg_row == Rows - 1)
	{
	    CONSOLE_SCREEN_BUFFER_INFO csbi;

	    if (GetConsoleScreenBufferInfo(g_hConOut, &csbi))
	    {
		if (csbi.dwCursorPosition.Y != msg_row)
		{
		    /* The screen is now messed up, must redraw the
		     * command line and later all the windows. */
		    redraw_all_later(CLEAR);
		    cmdline_row -= (msg_row - csbi.dwCursorPosition.Y);
		    redrawcmd();
		}
	    }
	}
#endif

	if (cRecords > 0)
	{
	    if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
	    {
#ifdef FEAT_MBYTE_IME
		/* Windows IME sends two '\n's with only one 'ENTER'.  First:
		 * wVirtualKeyCode == 13. second: wVirtualKeyCode == 0 */
		if (ir.Event.KeyEvent.UChar == 0
			&& ir.Event.KeyEvent.wVirtualKeyCode == 13)
		{
		    read_console_input(g_hConIn, &ir, 1, &cRecords);
		    continue;
		}
#endif
		if (decode_key_event(&ir.Event.KeyEvent, &ch, &ch2,
								 NULL, FALSE))
		    return TRUE;
	    }

	    read_console_input(g_hConIn, &ir, 1, &cRecords);

	    if (ir.EventType == FOCUS_EVENT)
		handle_focus_event(ir);
	    else if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
	    {
		COORD dwSize = ir.Event.WindowBufferSizeEvent.dwSize;

		// Only call shell_resized() when the size actually change to
		// avoid the screen is cleard.
		if (dwSize.X != Columns || dwSize.Y != Rows)
		{
		    CONSOLE_SCREEN_BUFFER_INFO csbi;
		    GetConsoleScreenBufferInfo(g_hConOut, &csbi);
		    dwSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		    ResizeConBuf(g_hConOut, dwSize);
		    shell_resized();
		}
	    }
#ifdef FEAT_MOUSE
	    else if (ir.EventType == MOUSE_EVENT
		    && decode_mouse_event(&ir.Event.MouseEvent))
		return TRUE;
#endif
	}
	else if (msec == 0)
	    break;
    }

#ifdef FEAT_CLIENTSERVER
    /* Something might have been received while we were waiting. */
    if (input_available())
	return TRUE;
#endif

    return FALSE;
}

#ifndef FEAT_GUI_MSWIN
/*
 * return non-zero if a character is available
 */
    int
mch_char_avail(void)
{
    return WaitForChar(0L, FALSE);
}

# if defined(FEAT_TERMINAL) || defined(PROTO)
/*
 * Check for any pending input or messages.
 */
    int
mch_check_messages(void)
{
    return WaitForChar(0L, TRUE);
}
# endif
#endif

/*
 * Create the console input.  Used when reading stdin doesn't work.
 */
    static void
create_conin(void)
{
    g_hConIn =	CreateFile("CONIN$", GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			(LPSECURITY_ATTRIBUTES) NULL,
			OPEN_EXISTING, 0, (HANDLE)NULL);
    did_create_conin = TRUE;
}

/*
 * Get a keystroke or a mouse event, use a blocking wait.
 */
    static WCHAR
tgetch(int *pmodifiers, WCHAR *pch2)
{
    WCHAR ch;

    for (;;)
    {
	INPUT_RECORD ir;
	DWORD cRecords = 0;

#ifdef FEAT_CLIENTSERVER
	(void)WaitForChar(-1L, FALSE);
	if (input_available())
	    return 0;
# ifdef FEAT_MOUSE
	if (g_nMouseClick != -1)
	    return 0;
# endif
#endif
	if (read_console_input(g_hConIn, &ir, 1, &cRecords) == 0)
	{
	    if (did_create_conin)
		read_error_exit();
	    create_conin();
	    continue;
	}

	if (ir.EventType == KEY_EVENT)
	{
	    if (decode_key_event(&ir.Event.KeyEvent, &ch, pch2,
							    pmodifiers, TRUE))
		return ch;
	}
	else if (ir.EventType == FOCUS_EVENT)
	    handle_focus_event(ir);
	else if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
	    shell_resized();
#ifdef FEAT_MOUSE
	else if (ir.EventType == MOUSE_EVENT)
	{
	    if (decode_mouse_event(&ir.Event.MouseEvent))
		return 0;
	}
#endif
    }
}
#endif /* !FEAT_GUI_MSWIN */


/*
 * mch_inchar(): low-level input function.
 * Get one or more characters from the keyboard or the mouse.
 * If time == 0, do not wait for characters.
 * If time == n, wait a short time for characters.
 * If time == -1, wait forever for characters.
 * Returns the number of characters read into buf.
 */
    int
mch_inchar(
    char_u	*buf UNUSED,
    int		maxlen UNUSED,
    long	time UNUSED,
    int		tb_change_cnt UNUSED)
{
#ifndef FEAT_GUI_MSWIN	    /* this isn't used for the GUI */

    int		len;
    int		c;
#define TYPEAHEADLEN 20
    static char_u   typeahead[TYPEAHEADLEN];	/* previously typed bytes. */
    static int	    typeaheadlen = 0;

    /* First use any typeahead that was kept because "buf" was too small. */
    if (typeaheadlen > 0)
	goto theend;

    if (time >= 0)
    {
	if (!WaitForChar(time, FALSE))     /* no character available */
	    return 0;
    }
    else    /* time == -1, wait forever */
    {
	mch_set_winsize_now();	/* Allow winsize changes from now on */

	/*
	 * If there is no character available within 2 seconds (default)
	 * write the autoscript file to disk.  Or cause the CursorHold event
	 * to be triggered.
	 */
	if (!WaitForChar(p_ut, FALSE))
	{
	    if (trigger_cursorhold() && maxlen >= 3)
	    {
		buf[0] = K_SPECIAL;
		buf[1] = KS_EXTRA;
		buf[2] = (int)KE_CURSORHOLD;
		return 3;
	    }
	    before_blocking();
	}
    }

    /*
     * Try to read as many characters as there are, until the buffer is full.
     */

    /* we will get at least one key. Get more if they are available. */
    g_fCBrkPressed = FALSE;

#ifdef MCH_WRITE_DUMP
    if (fdDump)
	fputc('[', fdDump);
#endif

    /* Keep looping until there is something in the typeahead buffer and more
     * to get and still room in the buffer (up to two bytes for a char and
     * three bytes for a modifier). */
    while ((typeaheadlen == 0 || WaitForChar(0L, FALSE))
					  && typeaheadlen + 5 <= TYPEAHEADLEN)
    {
	if (typebuf_changed(tb_change_cnt))
	{
	    /* "buf" may be invalid now if a client put something in the
	     * typeahead buffer and "buf" is in the typeahead buffer. */
	    typeaheadlen = 0;
	    break;
	}
#ifdef FEAT_MOUSE
	if (g_nMouseClick != -1)
	{
# ifdef MCH_WRITE_DUMP
	    if (fdDump)
		fprintf(fdDump, "{%02x @ %d, %d}",
			g_nMouseClick, g_xMouse, g_yMouse);
# endif
	    typeahead[typeaheadlen++] = ESC + 128;
	    typeahead[typeaheadlen++] = 'M';
	    typeahead[typeaheadlen++] = g_nMouseClick;
	    typeahead[typeaheadlen++] = g_xMouse + '!';
	    typeahead[typeaheadlen++] = g_yMouse + '!';
	    g_nMouseClick = -1;
	}
	else
#endif
	{
	    WCHAR	ch2 = NUL;
	    int		modifiers = 0;

	    c = tgetch(&modifiers, &ch2);

	    if (typebuf_changed(tb_change_cnt))
	    {
		/* "buf" may be invalid now if a client put something in the
		 * typeahead buffer and "buf" is in the typeahead buffer. */
		typeaheadlen = 0;
		break;
	    }

	    if (c == Ctrl_C && ctrl_c_interrupts)
	    {
#if defined(FEAT_CLIENTSERVER)
		trash_input_buf();
#endif
		got_int = TRUE;
	    }

#ifdef FEAT_MOUSE
	    if (g_nMouseClick == -1)
#endif
	    {
		int	n = 1;

		if (ch2 == NUL)
		{
		    int	    i;
		    char_u  *p;
		    WCHAR   ch[2];

		    ch[0] = c;
		    if (c >= 0xD800 && c <= 0xDBFF)	/* High surrogate */
		    {
			ch[1] = tgetch(&modifiers, &ch2);
			n++;
		    }
		    p = utf16_to_enc(ch, &n);
		    if (p != NULL)
		    {
			for (i = 0; i < n; i++)
			    typeahead[typeaheadlen + i] = p[i];
			vim_free(p);
		    }
		}
		else
		    typeahead[typeaheadlen] = c;
		if (ch2 != NUL)
		{
		    if (c == K_NUL)
		    {
			switch (ch2)
			{
			case (WCHAR)'\324': // SHIFT+Insert
			case (WCHAR)'\325': // CTRL+Insert
			case (WCHAR)'\327': // SHIFT+Delete
			case (WCHAR)'\330': // CTRL+Delete
			    typeahead[typeaheadlen + n] = (char_u)ch2;
			    n++;
			    break;

			default:
			    typeahead[typeaheadlen + n] = 3;
			    typeahead[typeaheadlen + n + 1] = (char_u)ch2;
			    n += 2;
			    break;
			}
		    }
		    else
		    {
			typeahead[typeaheadlen + n] = 3;
			typeahead[typeaheadlen + n + 1] = (char_u)ch2;
			n += 2;
		    }
		}

		/* Use the ALT key to set the 8th bit of the character
		 * when it's one byte, the 8th bit isn't set yet and not
		 * using a double-byte encoding (would become a lead
		 * byte). */
		if ((modifiers & MOD_MASK_ALT)
			&& n == 1
			&& (typeahead[typeaheadlen] & 0x80) == 0
			&& !enc_dbcs
		   )
		{
		    n = (*mb_char2bytes)(typeahead[typeaheadlen] | 0x80,
						    typeahead + typeaheadlen);
		    modifiers &= ~MOD_MASK_ALT;
		}

		if (modifiers != 0)
		{
		    /* Prepend modifiers to the character. */
		    mch_memmove(typeahead + typeaheadlen + 3,
						 typeahead + typeaheadlen, n);
		    typeahead[typeaheadlen++] = K_SPECIAL;
		    typeahead[typeaheadlen++] = (char_u)KS_MODIFIER;
		    typeahead[typeaheadlen++] =  modifiers;
		}

		typeaheadlen += n;

#ifdef MCH_WRITE_DUMP
		if (fdDump)
		    fputc(c, fdDump);
#endif
	    }
	}
    }

#ifdef MCH_WRITE_DUMP
    if (fdDump)
    {
	fputs("]\n", fdDump);
	fflush(fdDump);
    }
#endif

theend:
    /* Move typeahead to "buf", as much as fits. */
    len = 0;
    while (len < maxlen && typeaheadlen > 0)
    {
	buf[len++] = typeahead[0];
	mch_memmove(typeahead, typeahead + 1, --typeaheadlen);
    }
    return len;

#else /* FEAT_GUI_MSWIN */
    return 0;
#endif /* FEAT_GUI_MSWIN */
}

#ifndef PROTO
# ifndef __MINGW32__
#  include <shellapi.h>	/* required for FindExecutable() */
# endif
#endif

/*
 * If "use_path" is TRUE: Return TRUE if "name" is in $PATH.
 * If "use_path" is FALSE: Return TRUE if "name" exists.
 * When returning TRUE and "path" is not NULL save the path and set "*path" to
 * the allocated memory.
 * TODO: Should somehow check if it's really executable.
 */
    static int
executable_exists(char *name, char_u **path, int use_path)
{
    char	*dum;
    char	fname[_MAX_PATH];
    char	*curpath, *newpath;
    long	n;

    if (!use_path)
    {
	if (mch_getperm((char_u *)name) != -1 && !mch_isdir((char_u *)name))
	{
	    if (path != NULL)
	    {
		if (mch_isFullName((char_u *)name))
		    *path = vim_strsave((char_u *)name);
		else
		    *path = FullName_save((char_u *)name, FALSE);
	    }
	    return TRUE;
	}
	return FALSE;
    }

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR	*p = enc_to_utf16((char_u *)name, NULL);
	WCHAR	fnamew[_MAX_PATH];
	WCHAR	*dumw;
	WCHAR	*wcurpath, *wnewpath;

	if (p != NULL)
	{
	    wcurpath = _wgetenv(L"PATH");
	    wnewpath = (WCHAR*)alloc((unsigned)(wcslen(wcurpath) + 3)
							    * sizeof(WCHAR));
	    if (wnewpath == NULL)
		return FALSE;
	    wcscpy(wnewpath, L".;");
	    wcscat(wnewpath, wcurpath);
	    n = (long)SearchPathW(wnewpath, p, NULL, _MAX_PATH, fnamew, &dumw);
	    vim_free(wnewpath);
	    vim_free(p);
	    if (n == 0)
		return FALSE;
	    if (GetFileAttributesW(fnamew) & FILE_ATTRIBUTE_DIRECTORY)
		return FALSE;
	    if (path != NULL)
		*path = utf16_to_enc(fnamew, NULL);
	    return TRUE;
	}
    }

    curpath = getenv("PATH");
    newpath = (char*)alloc((unsigned)(STRLEN(curpath) + 3));
    if (newpath == NULL)
	return FALSE;
    STRCPY(newpath, ".;");
    STRCAT(newpath, curpath);
    n = (long)SearchPath(newpath, name, NULL, _MAX_PATH, fname, &dum);
    vim_free(newpath);
    if (n == 0)
	return FALSE;
    if (mch_isdir((char_u *)fname))
	return FALSE;
    if (path != NULL)
	*path = vim_strsave((char_u *)fname);
    return TRUE;
}

#if (defined(__MINGW32__) && __MSVCRT_VERSION__ >= 0x800) || \
	(defined(_MSC_VER) && _MSC_VER >= 1400)
/*
 * Bad parameter handler.
 *
 * Certain MS CRT functions will intentionally crash when passed invalid
 * parameters to highlight possible security holes.  Setting this function as
 * the bad parameter handler will prevent the crash.
 *
 * In debug builds the parameters contain CRT information that might help track
 * down the source of a problem, but in non-debug builds the arguments are all
 * NULL/0.  Debug builds will also produce assert dialogs from the CRT, it is
 * worth allowing these to make debugging of issues easier.
 */
    static void
bad_param_handler(const wchar_t *expression,
    const wchar_t *function,
    const wchar_t *file,
    unsigned int line,
    uintptr_t pReserved)
{
}

# define SET_INVALID_PARAM_HANDLER \
	((void)_set_invalid_parameter_handler(bad_param_handler))
#else
# define SET_INVALID_PARAM_HANDLER
#endif

#ifdef FEAT_GUI_MSWIN

/*
 * GUI version of mch_init().
 */
    void
mch_init(void)
{
#ifndef __MINGW32__
    extern int _fmode;
#endif

    /* Silently handle invalid parameters to CRT functions */
    SET_INVALID_PARAM_HANDLER;

    /* Let critical errors result in a failure, not in a dialog box.  Required
     * for the timestamp test to work on removed floppies. */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    _fmode = O_BINARY;		/* we do our own CR-LF translation */

    /* Specify window size.  Is there a place to get the default from? */
    Rows = 25;
    Columns = 80;

    /* Look for 'vimrun' */
    {
	char_u vimrun_location[_MAX_PATH + 4];

	/* First try in same directory as gvim.exe */
	STRCPY(vimrun_location, exe_name);
	STRCPY(gettail(vimrun_location), "vimrun.exe");
	if (mch_getperm(vimrun_location) >= 0)
	{
	    if (*skiptowhite(vimrun_location) != NUL)
	    {
		/* Enclose path with white space in double quotes. */
		mch_memmove(vimrun_location + 1, vimrun_location,
						 STRLEN(vimrun_location) + 1);
		*vimrun_location = '"';
		STRCPY(gettail(vimrun_location), "vimrun\" ");
	    }
	    else
		STRCPY(gettail(vimrun_location), "vimrun ");

	    vimrun_path = (char *)vim_strsave(vimrun_location);
	    s_dont_use_vimrun = FALSE;
	}
	else if (executable_exists("vimrun.exe", NULL, TRUE))
	    s_dont_use_vimrun = FALSE;

	/* Don't give the warning for a missing vimrun.exe right now, but only
	 * when vimrun was supposed to be used.  Don't bother people that do
	 * not need vimrun.exe. */
	if (s_dont_use_vimrun)
	    need_vimrun_warning = TRUE;
    }

    /*
     * If "finstr.exe" doesn't exist, use "grep -n" for 'grepprg'.
     * Otherwise the default "findstr /n" is used.
     */
    if (!executable_exists("findstr.exe", NULL, TRUE))
	set_option_value((char_u *)"grepprg", 0, (char_u *)"grep -n", 0);

#ifdef FEAT_CLIPBOARD
    win_clip_init();
#endif

    vtp_flag_init();
}


#else /* FEAT_GUI_MSWIN */

#define SRWIDTH(sr) ((sr).Right - (sr).Left + 1)
#define SRHEIGHT(sr) ((sr).Bottom - (sr).Top + 1)

/*
 * ClearConsoleBuffer()
 * Description:
 *  Clears the entire contents of the console screen buffer, using the
 *  specified attribute.
 * Returns:
 *  TRUE on success
 */
    static BOOL
ClearConsoleBuffer(WORD wAttribute)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coord;
    DWORD NumCells, dummy;

    if (!GetConsoleScreenBufferInfo(g_hConOut, &csbi))
	return FALSE;

    NumCells = csbi.dwSize.X * csbi.dwSize.Y;
    coord.X = 0;
    coord.Y = 0;
    if (!FillConsoleOutputCharacter(g_hConOut, ' ', NumCells,
	    coord, &dummy))
    {
	return FALSE;
    }
    if (!FillConsoleOutputAttribute(g_hConOut, wAttribute, NumCells,
	    coord, &dummy))
    {
	return FALSE;
    }

    return TRUE;
}

/*
 * FitConsoleWindow()
 * Description:
 *  Checks if the console window will fit within given buffer dimensions.
 *  Also, if requested, will shrink the window to fit.
 * Returns:
 *  TRUE on success
 */
    static BOOL
FitConsoleWindow(
    COORD dwBufferSize,
    BOOL WantAdjust)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD dwWindowSize;
    BOOL NeedAdjust = FALSE;

    if (GetConsoleScreenBufferInfo(g_hConOut, &csbi))
    {
	/*
	 * A buffer resize will fail if the current console window does
	 * not lie completely within that buffer.  To avoid this, we might
	 * have to move and possibly shrink the window.
	 */
	if (csbi.srWindow.Right >= dwBufferSize.X)
	{
	    dwWindowSize.X = SRWIDTH(csbi.srWindow);
	    if (dwWindowSize.X > dwBufferSize.X)
		dwWindowSize.X = dwBufferSize.X;
	    csbi.srWindow.Right = dwBufferSize.X - 1;
	    csbi.srWindow.Left = dwBufferSize.X - dwWindowSize.X;
	    NeedAdjust = TRUE;
	}
	if (csbi.srWindow.Bottom >= dwBufferSize.Y)
	{
	    dwWindowSize.Y = SRHEIGHT(csbi.srWindow);
	    if (dwWindowSize.Y > dwBufferSize.Y)
		dwWindowSize.Y = dwBufferSize.Y;
	    csbi.srWindow.Bottom = dwBufferSize.Y - 1;
	    csbi.srWindow.Top = dwBufferSize.Y - dwWindowSize.Y;
	    NeedAdjust = TRUE;
	}
	if (NeedAdjust && WantAdjust)
	{
	    if (!SetConsoleWindowInfo(g_hConOut, TRUE, &csbi.srWindow))
		return FALSE;
	}
	return TRUE;
    }

    return FALSE;
}

typedef struct ConsoleBufferStruct
{
    BOOL			IsValid;
    CONSOLE_SCREEN_BUFFER_INFO	Info;
    PCHAR_INFO			Buffer;
    COORD			BufferSize;
    PSMALL_RECT			Regions;
    int				NumRegions;
} ConsoleBuffer;

/*
 * SaveConsoleBuffer()
 * Description:
 *  Saves important information about the console buffer, including the
 *  actual buffer contents.  The saved information is suitable for later
 *  restoration by RestoreConsoleBuffer().
 * Returns:
 *  TRUE if all information was saved; FALSE otherwise
 *  If FALSE, still sets cb->IsValid if buffer characteristics were saved.
 */
    static BOOL
SaveConsoleBuffer(
    ConsoleBuffer *cb)
{
    DWORD NumCells;
    COORD BufferCoord;
    SMALL_RECT ReadRegion;
    WORD Y, Y_incr;
    int i, numregions;

    if (cb == NULL)
	return FALSE;

    if (!GetConsoleScreenBufferInfo(g_hConOut, &cb->Info))
    {
	cb->IsValid = FALSE;
	return FALSE;
    }
    cb->IsValid = TRUE;

    /*
     * Allocate a buffer large enough to hold the entire console screen
     * buffer.  If this ConsoleBuffer structure has already been initialized
     * with a buffer of the correct size, then just use that one.
     */
    if (!cb->IsValid || cb->Buffer == NULL ||
	    cb->BufferSize.X != cb->Info.dwSize.X ||
	    cb->BufferSize.Y != cb->Info.dwSize.Y)
    {
	cb->BufferSize.X = cb->Info.dwSize.X;
	cb->BufferSize.Y = cb->Info.dwSize.Y;
	NumCells = cb->BufferSize.X * cb->BufferSize.Y;
	vim_free(cb->Buffer);
	cb->Buffer = (PCHAR_INFO)alloc(NumCells * sizeof(CHAR_INFO));
	if (cb->Buffer == NULL)
	    return FALSE;
    }

    /*
     * We will now copy the console screen buffer into our buffer.
     * ReadConsoleOutput() seems to be limited as far as how much you
     * can read at a time.  Empirically, this number seems to be about
     * 12000 cells (rows * columns).  Start at position (0, 0) and copy
     * in chunks until it is all copied.  The chunks will all have the
     * same horizontal characteristics, so initialize them now.  The
     * height of each chunk will be (12000 / width).
     */
    BufferCoord.X = 0;
    ReadRegion.Left = 0;
    ReadRegion.Right = cb->Info.dwSize.X - 1;
    Y_incr = 12000 / cb->Info.dwSize.X;

    numregions = (cb->Info.dwSize.Y + Y_incr - 1) / Y_incr;
    if (cb->Regions == NULL || numregions != cb->NumRegions)
    {
	cb->NumRegions = numregions;
	vim_free(cb->Regions);
	cb->Regions = (PSMALL_RECT)alloc(cb->NumRegions * sizeof(SMALL_RECT));
	if (cb->Regions == NULL)
	{
	    VIM_CLEAR(cb->Buffer);
	    return FALSE;
	}
    }

    for (i = 0, Y = 0; i < cb->NumRegions; i++, Y += Y_incr)
    {
	/*
	 * Read into position (0, Y) in our buffer.
	 */
	BufferCoord.Y = Y;
	/*
	 * Read the region whose top left corner is (0, Y) and whose bottom
	 * right corner is (width - 1, Y + Y_incr - 1).  This should define
	 * a region of size width by Y_incr.  Don't worry if this region is
	 * too large for the remaining buffer; it will be cropped.
	 */
	ReadRegion.Top = Y;
	ReadRegion.Bottom = Y + Y_incr - 1;
	if (!ReadConsoleOutputW(g_hConOut,	/* output handle */
		cb->Buffer,			/* our buffer */
		cb->BufferSize,			/* dimensions of our buffer */
		BufferCoord,			/* offset in our buffer */
		&ReadRegion))			/* region to save */
	{
	    VIM_CLEAR(cb->Buffer);
	    VIM_CLEAR(cb->Regions);
	    return FALSE;
	}
	cb->Regions[i] = ReadRegion;
    }

    return TRUE;
}

/*
 * RestoreConsoleBuffer()
 * Description:
 *  Restores important information about the console buffer, including the
 *  actual buffer contents, if desired.  The information to restore is in
 *  the same format used by SaveConsoleBuffer().
 * Returns:
 *  TRUE on success
 */
    static BOOL
RestoreConsoleBuffer(
    ConsoleBuffer   *cb,
    BOOL	    RestoreScreen)
{
    COORD BufferCoord;
    SMALL_RECT WriteRegion;
    int i;

    if (cb == NULL || !cb->IsValid)
	return FALSE;

    /*
     * Before restoring the buffer contents, clear the current buffer, and
     * restore the cursor position and window information.  Doing this now
     * prevents old buffer contents from "flashing" onto the screen.
     */
    if (RestoreScreen)
	ClearConsoleBuffer(cb->Info.wAttributes);

    FitConsoleWindow(cb->Info.dwSize, TRUE);
    if (!SetConsoleScreenBufferSize(g_hConOut, cb->Info.dwSize))
	return FALSE;
    if (!SetConsoleTextAttribute(g_hConOut, cb->Info.wAttributes))
	return FALSE;

    if (!RestoreScreen)
    {
	/*
	 * No need to restore the screen buffer contents, so we're done.
	 */
	return TRUE;
    }

    if (!SetConsoleCursorPosition(g_hConOut, cb->Info.dwCursorPosition))
	return FALSE;
    if (!SetConsoleWindowInfo(g_hConOut, TRUE, &cb->Info.srWindow))
	return FALSE;

    /*
     * Restore the screen buffer contents.
     */
    if (cb->Buffer != NULL)
    {
	for (i = 0; i < cb->NumRegions; i++)
	{
	    BufferCoord.X = cb->Regions[i].Left;
	    BufferCoord.Y = cb->Regions[i].Top;
	    WriteRegion = cb->Regions[i];
	    if (!WriteConsoleOutputW(g_hConOut,	/* output handle */
			cb->Buffer,		/* our buffer */
			cb->BufferSize,		/* dimensions of our buffer */
			BufferCoord,		/* offset in our buffer */
			&WriteRegion))		/* region to restore */
	    {
		return FALSE;
	    }
	}
    }

    return TRUE;
}

#define FEAT_RESTORE_ORIG_SCREEN
#ifdef FEAT_RESTORE_ORIG_SCREEN
static ConsoleBuffer g_cbOrig = { 0 };
#endif
static ConsoleBuffer g_cbNonTermcap = { 0 };
static ConsoleBuffer g_cbTermcap = { 0 };

#ifdef FEAT_TITLE
#ifdef __BORLANDC__
typedef HWND (__stdcall *GETCONSOLEWINDOWPROC)(VOID);
#else
typedef HWND (WINAPI *GETCONSOLEWINDOWPROC)(VOID);
#endif
char g_szOrigTitle[256] = { 0 };
HWND g_hWnd = NULL;	/* also used in os_mswin.c */
static HICON g_hOrigIconSmall = NULL;
static HICON g_hOrigIcon = NULL;
static HICON g_hVimIcon = NULL;
static BOOL g_fCanChangeIcon = FALSE;

/* ICON* are not defined in VC++ 4.0 */
#ifndef ICON_SMALL
#define ICON_SMALL 0
#endif
#ifndef ICON_BIG
#define ICON_BIG 1
#endif
/*
 * GetConsoleIcon()
 * Description:
 *  Attempts to retrieve the small icon and/or the big icon currently in
 *  use by a given window.
 * Returns:
 *  TRUE on success
 */
    static BOOL
GetConsoleIcon(
    HWND	hWnd,
    HICON	*phIconSmall,
    HICON	*phIcon)
{
    if (hWnd == NULL)
	return FALSE;

    if (phIconSmall != NULL)
	*phIconSmall = (HICON)SendMessage(hWnd, WM_GETICON,
					       (WPARAM)ICON_SMALL, (LPARAM)0);
    if (phIcon != NULL)
	*phIcon = (HICON)SendMessage(hWnd, WM_GETICON,
						 (WPARAM)ICON_BIG, (LPARAM)0);
    return TRUE;
}

/*
 * SetConsoleIcon()
 * Description:
 *  Attempts to change the small icon and/or the big icon currently in
 *  use by a given window.
 * Returns:
 *  TRUE on success
 */
    static BOOL
SetConsoleIcon(
    HWND    hWnd,
    HICON   hIconSmall,
    HICON   hIcon)
{
    if (hWnd == NULL)
	return FALSE;

    if (hIconSmall != NULL)
	SendMessage(hWnd, WM_SETICON,
			    (WPARAM)ICON_SMALL, (LPARAM)hIconSmall);
    if (hIcon != NULL)
	SendMessage(hWnd, WM_SETICON,
			    (WPARAM)ICON_BIG, (LPARAM) hIcon);
    return TRUE;
}

/*
 * SaveConsoleTitleAndIcon()
 * Description:
 *  Saves the current console window title in g_szOrigTitle, for later
 *  restoration.  Also, attempts to obtain a handle to the console window,
 *  and use it to save the small and big icons currently in use by the
 *  console window.  This is not always possible on some versions of Windows;
 *  nor is it possible when running Vim remotely using Telnet (since the
 *  console window the user sees is owned by a remote process).
 */
    static void
SaveConsoleTitleAndIcon(void)
{
    /* Save the original title. */
    if (!GetConsoleTitle(g_szOrigTitle, sizeof(g_szOrigTitle)))
	return;

    /*
     * Obtain a handle to the console window using GetConsoleWindow() from
     * KERNEL32.DLL; we need to handle in order to change the window icon.
     * This function only exists on NT-based Windows, starting with Windows
     * 2000.  On older operating systems, we can't change the window icon
     * anyway.
     */
    g_hWnd = GetConsoleWindow();
    if (g_hWnd == NULL)
	return;

    /* Save the original console window icon. */
    GetConsoleIcon(g_hWnd, &g_hOrigIconSmall, &g_hOrigIcon);
    if (g_hOrigIconSmall == NULL || g_hOrigIcon == NULL)
	return;

    /* Extract the first icon contained in the Vim executable. */
    if (mch_icon_load((HANDLE *)&g_hVimIcon) == FAIL || g_hVimIcon == NULL)
	g_hVimIcon = ExtractIcon(NULL, (LPCSTR)exe_name, 0);
    if (g_hVimIcon != NULL)
	g_fCanChangeIcon = TRUE;
}
#endif

static int g_fWindInitCalled = FALSE;
static int g_fTermcapMode = FALSE;
static CONSOLE_CURSOR_INFO g_cci;
static DWORD g_cmodein = 0;
static DWORD g_cmodeout = 0;

/*
 * non-GUI version of mch_init().
 */
    void
mch_init(void)
{
#ifndef FEAT_RESTORE_ORIG_SCREEN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
#endif
#ifndef __MINGW32__
    extern int _fmode;
#endif

    /* Silently handle invalid parameters to CRT functions */
    SET_INVALID_PARAM_HANDLER;

    /* Let critical errors result in a failure, not in a dialog box.  Required
     * for the timestamp test to work on removed floppies. */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    _fmode = O_BINARY;		/* we do our own CR-LF translation */
    out_flush();

    /* Obtain handles for the standard Console I/O devices */
    if (read_cmd_fd == 0)
	g_hConIn =  GetStdHandle(STD_INPUT_HANDLE);
    else
	create_conin();
    g_hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

#ifdef FEAT_RESTORE_ORIG_SCREEN
    /* Save the initial console buffer for later restoration */
    SaveConsoleBuffer(&g_cbOrig);
    g_attrCurrent = g_attrDefault = g_cbOrig.Info.wAttributes;
#else
    /* Get current text attributes */
    GetConsoleScreenBufferInfo(g_hConOut, &csbi);
    g_attrCurrent = g_attrDefault = csbi.wAttributes;
#endif
    if (cterm_normal_fg_color == 0)
	cterm_normal_fg_color = (g_attrCurrent & 0xf) + 1;
    if (cterm_normal_bg_color == 0)
	cterm_normal_bg_color = ((g_attrCurrent >> 4) & 0xf) + 1;

    // Fg and Bg color index number at startup
    g_color_index_fg = g_attrDefault & 0xf;
    g_color_index_bg = (g_attrDefault >> 4) & 0xf;

    /* set termcap codes to current text attributes */
    update_tcap(g_attrCurrent);

    GetConsoleCursorInfo(g_hConOut, &g_cci);
    GetConsoleMode(g_hConIn,  &g_cmodein);
    GetConsoleMode(g_hConOut, &g_cmodeout);

#ifdef FEAT_TITLE
    SaveConsoleTitleAndIcon();
    /*
     * Set both the small and big icons of the console window to Vim's icon.
     * Note that Vim presently only has one size of icon (32x32), but it
     * automatically gets scaled down to 16x16 when setting the small icon.
     */
    if (g_fCanChangeIcon)
	SetConsoleIcon(g_hWnd, g_hVimIcon, g_hVimIcon);
#endif

    ui_get_shellsize();

#ifdef MCH_WRITE_DUMP
    fdDump = fopen("dump", "wt");

    if (fdDump)
    {
	time_t t;

	time(&t);
	fputs(ctime(&t), fdDump);
	fflush(fdDump);
    }
#endif

    g_fWindInitCalled = TRUE;

#ifdef FEAT_MOUSE
    g_fMouseAvail = GetSystemMetrics(SM_MOUSEPRESENT);
#endif

#ifdef FEAT_CLIPBOARD
    win_clip_init();
#endif

    vtp_flag_init();
    vtp_init();
}

/*
 * non-GUI version of mch_exit().
 * Shut down and exit with status `r'
 * Careful: mch_exit() may be called before mch_init()!
 */
    void
mch_exit(int r)
{
    exiting = TRUE;

    vtp_exit();

    stoptermcap();
    if (g_fWindInitCalled)
	settmode(TMODE_COOK);

    ml_close_all(TRUE);		/* remove all memfiles */

    if (g_fWindInitCalled)
    {
#ifdef FEAT_TITLE
	mch_restore_title(SAVE_RESTORE_BOTH);
	/*
	 * Restore both the small and big icons of the console window to
	 * what they were at startup.  Don't do this when the window is
	 * closed, Vim would hang here.
	 */
	if (g_fCanChangeIcon && !g_fForceExit)
	    SetConsoleIcon(g_hWnd, g_hOrigIconSmall, g_hOrigIcon);
#endif

#ifdef MCH_WRITE_DUMP
	if (fdDump)
	{
	    time_t t;

	    time(&t);
	    fputs(ctime(&t), fdDump);
	    fclose(fdDump);
	}
	fdDump = NULL;
#endif
    }

    SetConsoleCursorInfo(g_hConOut, &g_cci);
    SetConsoleMode(g_hConIn,  g_cmodein);
    SetConsoleMode(g_hConOut, g_cmodeout);

#ifdef DYNAMIC_GETTEXT
    dyn_libintl_end();
#endif

    exit(r);
}
#endif /* !FEAT_GUI_MSWIN */

/*
 * Do we have an interactive window?
 */
    int
mch_check_win(
    int argc UNUSED,
    char **argv UNUSED)
{
    get_exe_name();

#ifdef FEAT_GUI_MSWIN
    return OK;	    /* GUI always has a tty */
#else
    if (isatty(1))
	return OK;
    return FAIL;
#endif
}


/*
 * fname_casew(): Wide version of fname_case().  Set the case of the file name,
 * if it already exists.  When "len" is > 0, also expand short to long
 * filenames.
 * Return FAIL if wide functions are not available, OK otherwise.
 * NOTE: much of this is identical to fname_case(), keep in sync!
 */
    static int
fname_casew(
    WCHAR	*name,
    int		len)
{
    WCHAR		szTrueName[_MAX_PATH + 2];
    WCHAR		szTrueNameTemp[_MAX_PATH + 2];
    WCHAR		*ptrue, *ptruePrev;
    WCHAR		*porig, *porigPrev;
    int			flen;
    WIN32_FIND_DATAW	fb;
    HANDLE		hFind = INVALID_HANDLE_VALUE;
    int			c;
    int			slen;

    flen = (int)wcslen(name);
    if (flen > _MAX_PATH)
	return OK;

    /* slash_adjust(name) not needed, already adjusted by fname_case(). */

    /* Build the new name in szTrueName[] one component at a time. */
    porig = name;
    ptrue = szTrueName;

    if (iswalpha(porig[0]) && porig[1] == L':')
    {
	/* copy leading drive letter */
	*ptrue++ = *porig++;
	*ptrue++ = *porig++;
    }
    *ptrue = NUL;	    /* in case nothing follows */

    while (*porig != NUL)
    {
	/* copy \ characters */
	while (*porig == psepc)
	    *ptrue++ = *porig++;

	ptruePrev = ptrue;
	porigPrev = porig;
	while (*porig != NUL && *porig != psepc)
	{
	    *ptrue++ = *porig++;
	}
	*ptrue = NUL;

	/* To avoid a slow failure append "\*" when searching a directory,
	 * server or network share. */
	wcscpy(szTrueNameTemp, szTrueName);
	slen = (int)wcslen(szTrueNameTemp);
	if (*porig == psepc && slen + 2 < _MAX_PATH)
	    wcscpy(szTrueNameTemp + slen, L"\\*");

	/* Skip "", "." and "..". */
	if (ptrue > ptruePrev
		&& (ptruePrev[0] != L'.'
		    || (ptruePrev[1] != NUL
			&& (ptruePrev[1] != L'.' || ptruePrev[2] != NUL)))
		&& (hFind = FindFirstFileW(szTrueNameTemp, &fb))
						      != INVALID_HANDLE_VALUE)
	{
	    c = *porig;
	    *porig = NUL;

	    /* Only use the match when it's the same name (ignoring case) or
	     * expansion is allowed and there is a match with the short name
	     * and there is enough room. */
	    if (_wcsicoll(porigPrev, fb.cFileName) == 0
		    || (len > 0
			&& (_wcsicoll(porigPrev, fb.cAlternateFileName) == 0
			    && (int)(ptruePrev - szTrueName)
					   + (int)wcslen(fb.cFileName) < len)))
	    {
		wcscpy(ptruePrev, fb.cFileName);

		/* Look for exact match and prefer it if found.  Must be a
		 * long name, otherwise there would be only one match. */
		while (FindNextFileW(hFind, &fb))
		{
		    if (*fb.cAlternateFileName != NUL
			    && (wcscoll(porigPrev, fb.cFileName) == 0
				|| (len > 0
				    && (_wcsicoll(porigPrev,
						   fb.cAlternateFileName) == 0
				    && (int)(ptruePrev - szTrueName)
					 + (int)wcslen(fb.cFileName) < len))))
		    {
			wcscpy(ptruePrev, fb.cFileName);
			break;
		    }
		}
	    }
	    FindClose(hFind);
	    *porig = c;
	    ptrue = ptruePrev + wcslen(ptruePrev);
	}
    }

    wcscpy(name, szTrueName);
    return OK;
}

/*
 * fname_case(): Set the case of the file name, if it already exists.
 * When "len" is > 0, also expand short to long filenames.
 * NOTE: much of this is identical to fname_casew(), keep in sync!
 */
    void
fname_case(
    char_u	*name,
    int		len)
{
    char		szTrueName[_MAX_PATH + 2];
    char		szTrueNameTemp[_MAX_PATH + 2];
    char		*ptrue, *ptruePrev;
    char		*porig, *porigPrev;
    int			flen;
    WIN32_FIND_DATA	fb;
    HANDLE		hFind;
    int			c;
    int			slen;

    flen = (int)STRLEN(name);
    if (flen == 0)
	return;

    slash_adjust(name);

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR	*p = enc_to_utf16(name, NULL);

	if (p != NULL)
	{
	    char_u	*q;
	    WCHAR	buf[_MAX_PATH + 1];

	    wcsncpy(buf, p, _MAX_PATH);
	    buf[_MAX_PATH] = L'\0';
	    vim_free(p);

	    if (fname_casew(buf, (len > 0) ? _MAX_PATH : 0) == OK)
	    {
		q = utf16_to_enc(buf, NULL);
		if (q != NULL)
		{
		    vim_strncpy(name, q, (len > 0) ? len - 1 : flen);
		    vim_free(q);
		    return;
		}
	    }
	}
	return;
    }

    /* If 'enc' is utf-8, flen can be larger than _MAX_PATH.
     * So we should check this after calling wide function. */
    if (flen > _MAX_PATH)
	return;

    /* Build the new name in szTrueName[] one component at a time. */
    porig = (char *)name;
    ptrue = szTrueName;

    if (isalpha(porig[0]) && porig[1] == ':')
    {
	/* copy leading drive letter */
	*ptrue++ = *porig++;
	*ptrue++ = *porig++;
    }
    *ptrue = NUL;	    /* in case nothing follows */

    while (*porig != NUL)
    {
	/* copy \ characters */
	while (*porig == psepc)
	    *ptrue++ = *porig++;

	ptruePrev = ptrue;
	porigPrev = porig;
	while (*porig != NUL && *porig != psepc)
	{
	    int l;

	    if (enc_dbcs)
	    {
		l = (*mb_ptr2len)((char_u *)porig);
		while (--l >= 0)
		    *ptrue++ = *porig++;
	    }
	    else
		*ptrue++ = *porig++;
	}
	*ptrue = NUL;

	/* To avoid a slow failure append "\*" when searching a directory,
	 * server or network share. */
	STRCPY(szTrueNameTemp, szTrueName);
	slen = (int)strlen(szTrueNameTemp);
	if (*porig == psepc && slen + 2 < _MAX_PATH)
	    STRCPY(szTrueNameTemp + slen, "\\*");

	/* Skip "", "." and "..". */
	if (ptrue > ptruePrev
		&& (ptruePrev[0] != '.'
		    || (ptruePrev[1] != NUL
			&& (ptruePrev[1] != '.' || ptruePrev[2] != NUL)))
		&& (hFind = FindFirstFile(szTrueNameTemp, &fb))
						      != INVALID_HANDLE_VALUE)
	{
	    c = *porig;
	    *porig = NUL;

	    /* Only use the match when it's the same name (ignoring case) or
	     * expansion is allowed and there is a match with the short name
	     * and there is enough room. */
	    if (_stricoll(porigPrev, fb.cFileName) == 0
		    || (len > 0
			&& (_stricoll(porigPrev, fb.cAlternateFileName) == 0
			    && (int)(ptruePrev - szTrueName)
					   + (int)strlen(fb.cFileName) < len)))
	    {
		STRCPY(ptruePrev, fb.cFileName);

		/* Look for exact match and prefer it if found.  Must be a
		 * long name, otherwise there would be only one match. */
		while (FindNextFile(hFind, &fb))
		{
		    if (*fb.cAlternateFileName != NUL
			    && (strcoll(porigPrev, fb.cFileName) == 0
				|| (len > 0
				    && (_stricoll(porigPrev,
						   fb.cAlternateFileName) == 0
				    && (int)(ptruePrev - szTrueName)
					 + (int)strlen(fb.cFileName) < len))))
		    {
			STRCPY(ptruePrev, fb.cFileName);
			break;
		    }
		}
	    }
	    FindClose(hFind);
	    *porig = c;
	    ptrue = ptruePrev + strlen(ptruePrev);
	}
    }

    STRCPY(name, szTrueName);
}


/*
 * Insert user name in s[len].
 */
    int
mch_get_user_name(
    char_u  *s,
    int	    len)
{
    char szUserName[256 + 1];	/* UNLEN is 256 */
    DWORD cch = sizeof szUserName;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR wszUserName[256 + 1];	/* UNLEN is 256 */
	DWORD wcch = sizeof(wszUserName) / sizeof(WCHAR);

	if (GetUserNameW(wszUserName, &wcch))
	{
	    char_u  *p = utf16_to_enc(wszUserName, NULL);

	    if (p != NULL)
	    {
		vim_strncpy(s, p, len - 1);
		vim_free(p);
		return OK;
	    }
	}
    }
    if (GetUserName(szUserName, &cch))
    {
	vim_strncpy(s, (char_u *)szUserName, len - 1);
	return OK;
    }
    s[0] = NUL;
    return FAIL;
}


/*
 * Insert host name in s[len].
 */
    void
mch_get_host_name(
    char_u	*s,
    int		len)
{
    DWORD cch = len;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR wszHostName[256 + 1];
	DWORD wcch = sizeof(wszHostName) / sizeof(WCHAR);

	if (GetComputerNameW(wszHostName, &wcch))
	{
	    char_u  *p = utf16_to_enc(wszHostName, NULL);

	    if (p != NULL)
	    {
		vim_strncpy(s, p, len - 1);
		vim_free(p);
		return;
	    }
	}
    }
    if (!GetComputerName((LPSTR)s, &cch))
	vim_strncpy(s, (char_u *)"PC (Win32 Vim)", len - 1);
}


/*
 * return process ID
 */
    long
mch_get_pid(void)
{
    return (long)GetCurrentProcessId();
}


/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return OK for success, FAIL for failure.
 */
    int
mch_dirname(
    char_u	*buf,
    int		len)
{
    char_u  abuf[_MAX_PATH + 1];
    DWORD   lfnlen;

    /*
     * Originally this was:
     *    return (getcwd(buf, len) != NULL ? OK : FAIL);
     * But the Win32s known bug list says that getcwd() doesn't work
     * so use the Win32 system call instead. <Negri>
     */
    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR	wbuf[_MAX_PATH + 1];

	if (GetCurrentDirectoryW(_MAX_PATH, wbuf) != 0)
	{
	    WCHAR   wcbuf[_MAX_PATH + 1];
	    char_u  *p = NULL;

	    if (GetLongPathNameW(wbuf, wcbuf, _MAX_PATH) != 0)
	    {
		p = utf16_to_enc(wcbuf, NULL);
		if (STRLEN(p) >= (size_t)len)
		{
		    // long path name is too long, fall back to short one
		    vim_free(p);
		    p = NULL;
		}
	    }
	    if (p == NULL)
		p = utf16_to_enc(wbuf, NULL);

	    if (p != NULL)
	    {
		vim_strncpy(buf, p, len - 1);
		vim_free(p);
		return OK;
	    }
	}
	return FAIL;
    }
    if (GetCurrentDirectory(len, (LPSTR)buf) == 0)
	return FAIL;
    lfnlen = GetLongPathNameA((LPCSTR)buf, (LPSTR)abuf, _MAX_PATH);
    if (lfnlen == 0 || lfnlen >= (DWORD)len)
	// Failed to get long path name or it's too long: fall back to the
	// short path name.
	return OK;

    STRCPY(buf, abuf);
    return OK;
}

/*
 * Get file permissions for "name".
 * Return mode_t or -1 for error.
 */
    long
mch_getperm(char_u *name)
{
    stat_T	st;
    int		n;

    n = mch_stat((char *)name, &st);
    return n == 0 ? (long)(unsigned short)st.st_mode : -1L;
}


/*
 * Set file permission for "name" to "perm".
 *
 * Return FAIL for failure, OK otherwise.
 */
    int
mch_setperm(char_u *name, long perm)
{
    long	n = -1;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR *p = enc_to_utf16(name, NULL);

	if (p != NULL)
	{
	    n = _wchmod(p, perm);
	    vim_free(p);
	    if (n == -1)
		return FAIL;
	}
    }
    if (n == -1)
	n = _chmod((const char *)name, perm);
    if (n == -1)
	return FAIL;

    win32_set_archive(name);

    return OK;
}

/*
 * Set hidden flag for "name".
 */
    void
mch_hide(char_u *name)
{
    int attrs = win32_getattrs(name);
    if (attrs == -1)
	return;

    attrs |= FILE_ATTRIBUTE_HIDDEN;
    win32_setattrs(name, attrs);
}

/*
 * Return TRUE if file "name" exists and is hidden.
 */
    int
mch_ishidden(char_u *name)
{
    int f = win32_getattrs(name);

    if (f == -1)
	return FALSE;		    /* file does not exist at all */

    return (f & FILE_ATTRIBUTE_HIDDEN) != 0;
}

/*
 * return TRUE if "name" is a directory
 * return FALSE if "name" is not a directory or upon error
 */
    int
mch_isdir(char_u *name)
{
    int f = win32_getattrs(name);

    if (f == -1)
	return FALSE;		    /* file does not exist at all */

    return (f & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

/*
 * return TRUE if "name" is a directory, NOT a symlink to a directory
 * return FALSE if "name" is not a directory
 * return FALSE for error
 */
    int
mch_isrealdir(char_u *name)
{
    return mch_isdir(name) && !mch_is_symbolic_link(name);
}

/*
 * Create directory "name".
 * Return 0 on success, -1 on error.
 */
    int
mch_mkdir(char_u *name)
{
    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR	*p;
	int	retval;

	p = enc_to_utf16(name, NULL);
	if (p == NULL)
	    return -1;
	retval = _wmkdir(p);
	vim_free(p);
	return retval;
    }
    return _mkdir((const char *)name);
}

/*
 * Delete directory "name".
 * Return 0 on success, -1 on error.
 */
    int
mch_rmdir(char_u *name)
{
    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR	*p;
	int	retval;

	p = enc_to_utf16(name, NULL);
	if (p == NULL)
	    return -1;
	retval = _wrmdir(p);
	vim_free(p);
	return retval;
    }
    return _rmdir((const char *)name);
}

/*
 * Return TRUE if file "fname" has more than one link.
 */
    int
mch_is_hard_link(char_u *fname)
{
    BY_HANDLE_FILE_INFORMATION info;

    return win32_fileinfo(fname, &info) == FILEINFO_OK
						   && info.nNumberOfLinks > 1;
}

/*
 * Return TRUE if "name" is a symbolic link (or a junction).
 */
    int
mch_is_symbolic_link(char_u *name)
{
    HANDLE		hFind;
    int			res = FALSE;
    WIN32_FIND_DATAA	findDataA;
    DWORD		fileFlags = 0, reparseTag = 0;
    WCHAR		*wn = NULL;
    WIN32_FIND_DATAW	findDataW;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	wn = enc_to_utf16(name, NULL);
    if (wn != NULL)
    {
	hFind = FindFirstFileW(wn, &findDataW);
	vim_free(wn);
	if (hFind != INVALID_HANDLE_VALUE)
	{
	    fileFlags = findDataW.dwFileAttributes;
	    reparseTag = findDataW.dwReserved0;
	}
    }
    else
    {
	hFind = FindFirstFile((LPCSTR)name, &findDataA);
	if (hFind != INVALID_HANDLE_VALUE)
	{
	    fileFlags = findDataA.dwFileAttributes;
	    reparseTag = findDataA.dwReserved0;
	}
    }

    if (hFind != INVALID_HANDLE_VALUE)
	FindClose(hFind);

    if ((fileFlags & FILE_ATTRIBUTE_REPARSE_POINT)
	    && (reparseTag == IO_REPARSE_TAG_SYMLINK
		|| reparseTag == IO_REPARSE_TAG_MOUNT_POINT))
	res = TRUE;

    return res;
}

/*
 * Return TRUE if file "fname" has more than one link or if it is a symbolic
 * link.
 */
    int
mch_is_linked(char_u *fname)
{
    if (mch_is_hard_link(fname) || mch_is_symbolic_link(fname))
	return TRUE;
    return FALSE;
}

/*
 * Get the by-handle-file-information for "fname".
 * Returns FILEINFO_OK when OK.
 * returns FILEINFO_ENC_FAIL when enc_to_utf16() failed.
 * Returns FILEINFO_READ_FAIL when CreateFile() failed.
 * Returns FILEINFO_INFO_FAIL when GetFileInformationByHandle() failed.
 */
    int
win32_fileinfo(char_u *fname, BY_HANDLE_FILE_INFORMATION *info)
{
    HANDLE	hFile;
    int		res = FILEINFO_READ_FAIL;
    WCHAR	*wn = NULL;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	wn = enc_to_utf16(fname, NULL);
	if (wn == NULL)
	    return FILEINFO_ENC_FAIL;
    }
    if (wn != NULL)
    {
	hFile = CreateFileW(wn,		/* file name */
		    GENERIC_READ,	/* access mode */
		    FILE_SHARE_READ | FILE_SHARE_WRITE,	/* share mode */
		    NULL,		/* security descriptor */
		    OPEN_EXISTING,	/* creation disposition */
		    FILE_FLAG_BACKUP_SEMANTICS,	/* file attributes */
		    NULL);		/* handle to template file */
	vim_free(wn);
    }
    else
	hFile = CreateFile((LPCSTR)fname,    /* file name */
		    GENERIC_READ,	    /* access mode */
		    FILE_SHARE_READ | FILE_SHARE_WRITE,	/* share mode */
		    NULL,		/* security descriptor */
		    OPEN_EXISTING,	/* creation disposition */
		    FILE_FLAG_BACKUP_SEMANTICS,	/* file attributes */
		    NULL);		/* handle to template file */

    if (hFile != INVALID_HANDLE_VALUE)
    {
	if (GetFileInformationByHandle(hFile, info) != 0)
	    res = FILEINFO_OK;
	else
	    res = FILEINFO_INFO_FAIL;
	CloseHandle(hFile);
    }

    return res;
}

/*
 * get file attributes for `name'
 * -1 : error
 * else FILE_ATTRIBUTE_* defined in winnt.h
 */
    static int
win32_getattrs(char_u *name)
{
    int		attr;
    WCHAR	*p = NULL;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	p = enc_to_utf16(name, NULL);

    if (p != NULL)
    {
	attr = GetFileAttributesW(p);
	vim_free(p);
    }
    else
	attr = GetFileAttributes((char *)name);

    return attr;
}

/*
 * set file attributes for `name' to `attrs'
 *
 * return -1 for failure, 0 otherwise
 */
    static int
win32_setattrs(char_u *name, int attrs)
{
    int res;
    WCHAR	*p = NULL;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	p = enc_to_utf16(name, NULL);

    if (p != NULL)
    {
	res = SetFileAttributesW(p, attrs);
	vim_free(p);
    }
    else
	res = SetFileAttributes((char *)name, attrs);

    return res ? 0 : -1;
}

/*
 * Set archive flag for "name".
 */
    static int
win32_set_archive(char_u *name)
{
    int attrs = win32_getattrs(name);
    if (attrs == -1)
	return -1;

    attrs |= FILE_ATTRIBUTE_ARCHIVE;
    return win32_setattrs(name, attrs);
}

/*
 * Return TRUE if file or directory "name" is writable (not readonly).
 * Strange semantics of Win32: a readonly directory is writable, but you can't
 * delete a file.  Let's say this means it is writable.
 */
    int
mch_writable(char_u *name)
{
    int attrs = win32_getattrs(name);

    return (attrs != -1 && (!(attrs & FILE_ATTRIBUTE_READONLY)
			  || (attrs & FILE_ATTRIBUTE_DIRECTORY)));
}

/*
 * Return TRUE if "name" can be executed, FALSE if not.
 * If "use_path" is FALSE only check if "name" is executable.
 * When returning TRUE and "path" is not NULL save the path and set "*path" to
 * the allocated memory.
 */
    int
mch_can_exe(char_u *name, char_u **path, int use_path)
{
    char_u	buf[_MAX_PATH];
    int		len = (int)STRLEN(name);
    char_u	*p, *saved;

    if (len >= _MAX_PATH)	/* safety check */
	return FALSE;

    /* Ty using the name directly when a Unix-shell like 'shell'. */
    if (strstr((char *)gettail(p_sh), "sh") != NULL)
	if (executable_exists((char *)name, path, use_path))
	    return TRUE;

    /*
     * Loop over all extensions in $PATHEXT.
     */
    p = mch_getenv("PATHEXT");
    if (p == NULL)
	p = (char_u *)".com;.exe;.bat;.cmd";
    saved = vim_strsave(p);
    if (saved == NULL)
	return FALSE;
    p = saved;
    while (*p)
    {
	char_u	*tmp = vim_strchr(p, ';');

	if (tmp != NULL)
	    *tmp = NUL;
	if (_stricoll((char *)name + len - STRLEN(p), (char *)p) == 0
			    && executable_exists((char *)name, path, use_path))
	{
	    vim_free(saved);
	    return TRUE;
	}
	if (tmp == NULL)
	    break;
	p = tmp + 1;
    }
    vim_free(saved);

    vim_strncpy(buf, name, _MAX_PATH - 1);
    p = mch_getenv("PATHEXT");
    if (p == NULL)
	p = (char_u *)".com;.exe;.bat;.cmd";
    while (*p)
    {
	if (p[0] == '.' && (p[1] == NUL || p[1] == ';'))
	{
	    /* A single "." means no extension is added. */
	    buf[len] = NUL;
	    ++p;
	    if (*p)
		++p;
	}
	else
	    copy_option_part(&p, buf + len, _MAX_PATH - len, ";");
	if (executable_exists((char *)buf, path, use_path))
	    return TRUE;
    }
    return FALSE;
}

/*
 * Check what "name" is:
 * NODE_NORMAL: file or directory (or doesn't exist)
 * NODE_WRITABLE: writable device, socket, fifo, etc.
 * NODE_OTHER: non-writable things
 */
    int
mch_nodetype(char_u *name)
{
    HANDLE	hFile;
    int		type;
    WCHAR	*wn = NULL;

    /* We can't open a file with a name "\\.\con" or "\\.\prn" and trying to
     * read from it later will cause Vim to hang.  Thus return NODE_WRITABLE
     * here. */
    if (STRNCMP(name, "\\\\.\\", 4) == 0)
	return NODE_WRITABLE;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	wn = enc_to_utf16(name, NULL);

    if (wn != NULL)
    {
	hFile = CreateFileW(wn,		    /* file name */
		    GENERIC_WRITE,	    /* access mode */
		    0,			    /* share mode */
		    NULL,		    /* security descriptor */
		    OPEN_EXISTING,	    /* creation disposition */
		    0,			    /* file attributes */
		    NULL);		    /* handle to template file */
	vim_free(wn);
    }
    else
	hFile = CreateFile((LPCSTR)name,    /* file name */
		    GENERIC_WRITE,	    /* access mode */
		    0,			    /* share mode */
		    NULL,		    /* security descriptor */
		    OPEN_EXISTING,	    /* creation disposition */
		    0,			    /* file attributes */
		    NULL);		    /* handle to template file */

    if (hFile == INVALID_HANDLE_VALUE)
	return NODE_NORMAL;

    type = GetFileType(hFile);
    CloseHandle(hFile);
    if (type == FILE_TYPE_CHAR)
	return NODE_WRITABLE;
    if (type == FILE_TYPE_DISK)
	return NODE_NORMAL;
    return NODE_OTHER;
}

#ifdef HAVE_ACL
struct my_acl
{
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;
    PSID		    pSidOwner;
    PSID		    pSidGroup;
    PACL		    pDacl;
    PACL		    pSacl;
};
#endif

/*
 * Return a pointer to the ACL of file "fname" in allocated memory.
 * Return NULL if the ACL is not available for whatever reason.
 */
    vim_acl_T
mch_get_acl(char_u *fname)
{
#ifndef HAVE_ACL
    return (vim_acl_T)NULL;
#else
    struct my_acl   *p = NULL;
    DWORD   err;

    p = (struct my_acl *)alloc_clear((unsigned)sizeof(struct my_acl));
    if (p != NULL)
    {
	WCHAR	*wn = NULL;

	if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	    wn = enc_to_utf16(fname, NULL);
	if (wn != NULL)
	{
	    /* Try to retrieve the entire security descriptor. */
	    err = GetNamedSecurityInfoW(
		    wn,			// Abstract filename
		    SE_FILE_OBJECT,	// File Object
		    OWNER_SECURITY_INFORMATION |
		    GROUP_SECURITY_INFORMATION |
		    DACL_SECURITY_INFORMATION |
		    SACL_SECURITY_INFORMATION,
		    &p->pSidOwner,	// Ownership information.
		    &p->pSidGroup,	// Group membership.
		    &p->pDacl,		// Discretionary information.
		    &p->pSacl,		// For auditing purposes.
		    &p->pSecurityDescriptor);
	    if (err == ERROR_ACCESS_DENIED ||
		    err == ERROR_PRIVILEGE_NOT_HELD)
	    {
		/* Retrieve only DACL. */
		(void)GetNamedSecurityInfoW(
			wn,
			SE_FILE_OBJECT,
			DACL_SECURITY_INFORMATION,
			NULL,
			NULL,
			&p->pDacl,
			NULL,
			&p->pSecurityDescriptor);
	    }
	    if (p->pSecurityDescriptor == NULL)
	    {
		mch_free_acl((vim_acl_T)p);
		p = NULL;
	    }
	    vim_free(wn);
	}
	else
	{
	    /* Try to retrieve the entire security descriptor. */
	    err = GetNamedSecurityInfo(
		    (LPSTR)fname,	// Abstract filename
		    SE_FILE_OBJECT,	// File Object
		    OWNER_SECURITY_INFORMATION |
		    GROUP_SECURITY_INFORMATION |
		    DACL_SECURITY_INFORMATION |
		    SACL_SECURITY_INFORMATION,
		    &p->pSidOwner,	// Ownership information.
		    &p->pSidGroup,	// Group membership.
		    &p->pDacl,		// Discretionary information.
		    &p->pSacl,		// For auditing purposes.
		    &p->pSecurityDescriptor);
	    if (err == ERROR_ACCESS_DENIED ||
		    err == ERROR_PRIVILEGE_NOT_HELD)
	    {
		/* Retrieve only DACL. */
		(void)GetNamedSecurityInfo(
			(LPSTR)fname,
			SE_FILE_OBJECT,
			DACL_SECURITY_INFORMATION,
			NULL,
			NULL,
			&p->pDacl,
			NULL,
			&p->pSecurityDescriptor);
	    }
	    if (p->pSecurityDescriptor == NULL)
	    {
		mch_free_acl((vim_acl_T)p);
		p = NULL;
	    }
	}
    }

    return (vim_acl_T)p;
#endif
}

#ifdef HAVE_ACL
/*
 * Check if "acl" contains inherited ACE.
 */
    static BOOL
is_acl_inherited(PACL acl)
{
    DWORD   i;
    ACL_SIZE_INFORMATION    acl_info;
    PACCESS_ALLOWED_ACE	    ace;

    acl_info.AceCount = 0;
    GetAclInformation(acl, &acl_info, sizeof(acl_info), AclSizeInformation);
    for (i = 0; i < acl_info.AceCount; i++)
    {
	GetAce(acl, i, (LPVOID *)&ace);
	if (ace->Header.AceFlags & INHERITED_ACE)
	    return TRUE;
    }
    return FALSE;
}
#endif

/*
 * Set the ACL of file "fname" to "acl" (unless it's NULL).
 * Errors are ignored.
 * This must only be called with "acl" equal to what mch_get_acl() returned.
 */
    void
mch_set_acl(char_u *fname, vim_acl_T acl)
{
#ifdef HAVE_ACL
    struct my_acl   *p = (struct my_acl *)acl;
    SECURITY_INFORMATION    sec_info = 0;

    if (p != NULL)
    {
	WCHAR	*wn = NULL;

	/* Set security flags */
	if (p->pSidOwner)
	    sec_info |= OWNER_SECURITY_INFORMATION;
	if (p->pSidGroup)
	    sec_info |= GROUP_SECURITY_INFORMATION;
	if (p->pDacl)
	{
	    sec_info |= DACL_SECURITY_INFORMATION;
	    /* Do not inherit its parent's DACL.
	     * If the DACL is inherited, Cygwin permissions would be changed.
	     */
	    if (!is_acl_inherited(p->pDacl))
		sec_info |= PROTECTED_DACL_SECURITY_INFORMATION;
	}
	if (p->pSacl)
	    sec_info |= SACL_SECURITY_INFORMATION;

	if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	    wn = enc_to_utf16(fname, NULL);
	if (wn != NULL)
	{
	    (void)SetNamedSecurityInfoW(
			wn,			// Abstract filename
			SE_FILE_OBJECT,		// File Object
			sec_info,
			p->pSidOwner,		// Ownership information.
			p->pSidGroup,		// Group membership.
			p->pDacl,		// Discretionary information.
			p->pSacl		// For auditing purposes.
			);
	    vim_free(wn);
	}
	else
	{
	    (void)SetNamedSecurityInfo(
			(LPSTR)fname,		// Abstract filename
			SE_FILE_OBJECT,		// File Object
			sec_info,
			p->pSidOwner,		// Ownership information.
			p->pSidGroup,		// Group membership.
			p->pDacl,		// Discretionary information.
			p->pSacl		// For auditing purposes.
			);
	}
    }
#endif
}

    void
mch_free_acl(vim_acl_T acl)
{
#ifdef HAVE_ACL
    struct my_acl   *p = (struct my_acl *)acl;

    if (p != NULL)
    {
	LocalFree(p->pSecurityDescriptor);	// Free the memory just in case
	vim_free(p);
    }
#endif
}

#ifndef FEAT_GUI_MSWIN

/*
 * handler for ctrl-break, ctrl-c interrupts, and fatal events.
 */
    static BOOL WINAPI
handler_routine(
    DWORD dwCtrlType)
{
    INPUT_RECORD ir;
    DWORD out;

    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
	if (ctrl_c_interrupts)
	    g_fCtrlCPressed = TRUE;
	return TRUE;

    case CTRL_BREAK_EVENT:
	g_fCBrkPressed	= TRUE;
	ctrl_break_was_pressed = TRUE;
	/* ReadConsoleInput is blocking, send a key event to continue. */
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = TRUE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.wVirtualKeyCode = VK_CANCEL;
	ir.Event.KeyEvent.wVirtualScanCode = 0;
	ir.Event.KeyEvent.dwControlKeyState = 0;
	ir.Event.KeyEvent.uChar.UnicodeChar = 0;
	WriteConsoleInput(g_hConIn, &ir, 1, &out);
	return TRUE;

    /* fatal events: shut down gracefully */
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
	windgoto((int)Rows - 1, 0);
	g_fForceExit = TRUE;

	vim_snprintf((char *)IObuff, IOSIZE, _("Vim: Caught %s event\n"),
		(dwCtrlType == CTRL_CLOSE_EVENT
		     ? _("close")
		     : dwCtrlType == CTRL_LOGOFF_EVENT
			 ? _("logoff")
			 : _("shutdown")));
#ifdef DEBUG
	OutputDebugString(IObuff);
#endif

	preserve_exit();	/* output IObuff, preserve files and exit */

	return TRUE;		/* not reached */

    default:
	return FALSE;
    }
}


/*
 * set the tty in (raw) ? "raw" : "cooked" mode
 */
    void
mch_settmode(int tmode)
{
    DWORD cmodein;
    DWORD cmodeout;
    BOOL bEnableHandler;

    GetConsoleMode(g_hConIn, &cmodein);
    GetConsoleMode(g_hConOut, &cmodeout);
    if (tmode == TMODE_RAW)
    {
	cmodein &= ~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT |
		     ENABLE_ECHO_INPUT);
#ifdef FEAT_MOUSE
	if (g_fMouseActive)
	    cmodein |= ENABLE_MOUSE_INPUT;
#endif
	cmodeout &= ~(
#ifdef FEAT_TERMGUICOLORS
	    /* Do not turn off the ENABLE_PROCESSRD_OUTPUT flag when using
	     * VTP. */
	    ((vtp_working) ? 0 : ENABLE_PROCESSED_OUTPUT) |
#else
	    ENABLE_PROCESSED_OUTPUT |
#endif
	    ENABLE_WRAP_AT_EOL_OUTPUT);
	bEnableHandler = TRUE;
    }
    else /* cooked */
    {
	cmodein |= (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT |
		    ENABLE_ECHO_INPUT);
	cmodeout |= (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	bEnableHandler = FALSE;
    }
    SetConsoleMode(g_hConIn, cmodein);
    SetConsoleMode(g_hConOut, cmodeout);
    SetConsoleCtrlHandler(handler_routine, bEnableHandler);

#ifdef MCH_WRITE_DUMP
    if (fdDump)
    {
	fprintf(fdDump, "mch_settmode(%s, in = %x, out = %x)\n",
		tmode == TMODE_RAW ? "raw" :
				    tmode == TMODE_COOK ? "cooked" : "normal",
		cmodein, cmodeout);
	fflush(fdDump);
    }
#endif
}


/*
 * Get the size of the current window in `Rows' and `Columns'
 * Return OK when size could be determined, FAIL otherwise.
 */
    int
mch_get_shellsize(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (!g_fTermcapMode && g_cbTermcap.IsValid)
    {
	/*
	 * For some reason, we are trying to get the screen dimensions
	 * even though we are not in termcap mode.  The 'Rows' and 'Columns'
	 * variables are really intended to mean the size of Vim screen
	 * while in termcap mode.
	 */
	Rows = g_cbTermcap.Info.dwSize.Y;
	Columns = g_cbTermcap.Info.dwSize.X;
    }
    else if (GetConsoleScreenBufferInfo(g_hConOut, &csbi))
    {
	Rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	Columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    else
    {
	Rows = 25;
	Columns = 80;
    }
    return OK;
}

/*
 * Resize console buffer to 'COORD'
 */
    static void
ResizeConBuf(
    HANDLE  hConsole,
    COORD   coordScreen)
{
    if (!SetConsoleScreenBufferSize(hConsole, coordScreen))
    {
#ifdef MCH_WRITE_DUMP
	if (fdDump)
	{
	    fprintf(fdDump, "SetConsoleScreenBufferSize failed: %lx\n",
		    GetLastError());
	    fflush(fdDump);
	}
#endif
    }
}

/*
 * Resize console window size to 'srWindowRect'
 */
    static void
ResizeWindow(
    HANDLE     hConsole,
    SMALL_RECT srWindowRect)
{
    if (!SetConsoleWindowInfo(hConsole, TRUE, &srWindowRect))
    {
#ifdef MCH_WRITE_DUMP
	if (fdDump)
	{
	    fprintf(fdDump, "SetConsoleWindowInfo failed: %lx\n",
		    GetLastError());
	    fflush(fdDump);
	}
#endif
    }
}

/*
 * Set a console window to `xSize' * `ySize'
 */
    static void
ResizeConBufAndWindow(
    HANDLE  hConsole,
    int	    xSize,
    int	    ySize)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;	/* hold current console buffer info */
    SMALL_RECT	    srWindowRect;	/* hold the new console size */
    COORD	    coordScreen;
    static int	    resized = FALSE;

#ifdef MCH_WRITE_DUMP
    if (fdDump)
    {
	fprintf(fdDump, "ResizeConBufAndWindow(%d, %d)\n", xSize, ySize);
	fflush(fdDump);
    }
#endif

    /* get the largest size we can size the console window to */
    coordScreen = GetLargestConsoleWindowSize(hConsole);

    /* define the new console window size and scroll position */
    srWindowRect.Left = srWindowRect.Top = (SHORT) 0;
    srWindowRect.Right =  (SHORT) (min(xSize, coordScreen.X) - 1);
    srWindowRect.Bottom = (SHORT) (min(ySize, coordScreen.Y) - 1);

    if (GetConsoleScreenBufferInfo(g_hConOut, &csbi))
    {
	int sx, sy;

	sx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	sy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	if (sy < ySize || sx < xSize)
	{
	    /*
	     * Increasing number of lines/columns, do buffer first.
	     * Use the maximal size in x and y direction.
	     */
	    if (sy < ySize)
		coordScreen.Y = ySize;
	    else
		coordScreen.Y = sy;
	    if (sx < xSize)
		coordScreen.X = xSize;
	    else
		coordScreen.X = sx;
	    SetConsoleScreenBufferSize(hConsole, coordScreen);
	}
    }

    // define the new console buffer size
    coordScreen.X = xSize;
    coordScreen.Y = ySize;

    // In the new console call API, only the first time in reverse order
    if (!vtp_working || resized)
    {
	ResizeWindow(hConsole, srWindowRect);
	ResizeConBuf(hConsole, coordScreen);
    }
    else
    {
	ResizeConBuf(hConsole, coordScreen);
	ResizeWindow(hConsole, srWindowRect);
	resized = TRUE;
    }
}


/*
 * Set the console window to `Rows' * `Columns'
 */
    void
mch_set_shellsize(void)
{
    COORD coordScreen;

    /* Don't change window size while still starting up */
    if (suppress_winsize != 0)
    {
	suppress_winsize = 2;
	return;
    }

    if (term_console)
    {
	coordScreen = GetLargestConsoleWindowSize(g_hConOut);

	/* Clamp Rows and Columns to reasonable values */
	if (Rows > coordScreen.Y)
	    Rows = coordScreen.Y;
	if (Columns > coordScreen.X)
	    Columns = coordScreen.X;

	ResizeConBufAndWindow(g_hConOut, Columns, Rows);
    }
}

/*
 * Rows and/or Columns has changed.
 */
    void
mch_new_shellsize(void)
{
    set_scroll_region(0, 0, Columns - 1, Rows - 1);
}


/*
 * Called when started up, to set the winsize that was delayed.
 */
    void
mch_set_winsize_now(void)
{
    if (suppress_winsize == 2)
    {
	suppress_winsize = 0;
	mch_set_shellsize();
	shell_resized();
    }
    suppress_winsize = 0;
}
#endif /* FEAT_GUI_MSWIN */

    static BOOL
vim_create_process(
    char		*cmd,
    BOOL		inherit_handles,
    DWORD		flags,
    STARTUPINFO		*si,
    PROCESS_INFORMATION *pi,
    LPVOID		*env,
    char		*cwd)
{
    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	BOOL	ret;
	WCHAR	*wcmd, *wcwd = NULL;

	wcmd = enc_to_utf16((char_u *)cmd, NULL);
	if (wcmd == NULL)
	    goto fallback;
	if (cwd != NULL)
	{
	    wcwd = enc_to_utf16((char_u *)cwd, NULL);
	    if (wcwd == NULL)
	    {
		vim_free(wcmd);
		goto fallback;
	    }
	}

	ret = CreateProcessW(
	    NULL,			/* Executable name */
	    wcmd,			/* Command to execute */
	    NULL,			/* Process security attributes */
	    NULL,			/* Thread security attributes */
	    inherit_handles,	/* Inherit handles */
	    flags,			/* Creation flags */
	    env,			/* Environment */
	    wcwd,			/* Current directory */
	    (LPSTARTUPINFOW)si,	/* Startup information */
	    pi);			/* Process information */
	vim_free(wcmd);
	vim_free(wcwd);
	return ret;
    }
fallback:
    return CreateProcess(
	NULL,			/* Executable name */
	cmd,			/* Command to execute */
	NULL,			/* Process security attributes */
	NULL,			/* Thread security attributes */
	inherit_handles,	/* Inherit handles */
	flags,			/* Creation flags */
	env,			/* Environment */
	cwd,			/* Current directory */
	si,			/* Startup information */
	pi);			/* Process information */
}


    static HINSTANCE
vim_shell_execute(
    char *cmd,
    INT	 n_show_cmd)
{
    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR *wcmd = enc_to_utf16((char_u *)cmd, NULL);
	if (wcmd != NULL)
	{
	    HINSTANCE ret;
	    ret = ShellExecuteW(NULL, NULL, wcmd, NULL, NULL, n_show_cmd);
	    vim_free(wcmd);
	    return ret;
	}
    }
    return ShellExecute(NULL, NULL, cmd, NULL, NULL, n_show_cmd);
}


#if defined(FEAT_GUI_MSWIN) || defined(PROTO)

/*
 * Specialised version of system() for Win32 GUI mode.
 * This version proceeds as follows:
 *    1. Create a console window for use by the subprocess
 *    2. Run the subprocess (it gets the allocated console by default)
 *    3. Wait for the subprocess to terminate and get its exit code
 *    4. Prompt the user to press a key to close the console window
 */
    static int
mch_system_classic(char *cmd, int options)
{
    STARTUPINFO		si;
    PROCESS_INFORMATION pi;
    DWORD		ret = 0;
    HWND		hwnd = GetFocus();

    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwFlags = STARTF_USESHOWWINDOW;
    /*
     * It's nicer to run a filter command in a minimized window.
     * Don't activate the window to keep focus on Vim.
     */
    if (options & SHELL_DOOUT)
	si.wShowWindow = SW_SHOWMINNOACTIVE;
    else
	si.wShowWindow = SW_SHOWNORMAL;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;

    /* Now, run the command */
    vim_create_process(cmd, FALSE,
	    CREATE_DEFAULT_ERROR_MODE |	CREATE_NEW_CONSOLE,
	    &si, &pi, NULL, NULL);

    /* Wait for the command to terminate before continuing */
    {
#ifdef FEAT_GUI
	int	    delay = 1;

	/* Keep updating the window while waiting for the shell to finish. */
	for (;;)
	{
	    MSG	msg;

	    if (pPeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE))
	    {
		TranslateMessage(&msg);
		pDispatchMessage(&msg);
		delay = 1;
		continue;
	    }
	    if (WaitForSingleObject(pi.hProcess, delay) != WAIT_TIMEOUT)
		break;

	    /* We start waiting for a very short time and then increase it, so
	     * that we respond quickly when the process is quick, and don't
	     * consume too much overhead when it's slow. */
	    if (delay < 50)
		delay += 10;
	}
#else
	WaitForSingleObject(pi.hProcess, INFINITE);
#endif

	/* Get the command exit code */
	GetExitCodeProcess(pi.hProcess, &ret);
    }

    /* Close the handles to the subprocess, so that it goes away */
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* Try to get input focus back.  Doesn't always work though. */
    PostMessage(hwnd, WM_SETFOCUS, 0, 0);

    return ret;
}

/*
 * Thread launched by the gui to send the current buffer data to the
 * process. This way avoid to hang up vim totally if the children
 * process take a long time to process the lines.
 */
    static unsigned int __stdcall
sub_process_writer(LPVOID param)
{
    HANDLE	    g_hChildStd_IN_Wr = param;
    linenr_T	    lnum = curbuf->b_op_start.lnum;
    DWORD	    len = 0;
    DWORD	    l;
    char_u	    *lp = ml_get(lnum);
    char_u	    *s;
    int		    written = 0;

    for (;;)
    {
	l = (DWORD)STRLEN(lp + written);
	if (l == 0)
	    len = 0;
	else if (lp[written] == NL)
	{
	    /* NL -> NUL translation */
	    WriteFile(g_hChildStd_IN_Wr, "", 1, &len, NULL);
	}
	else
	{
	    s = vim_strchr(lp + written, NL);
	    WriteFile(g_hChildStd_IN_Wr, (char *)lp + written,
		      s == NULL ? l : (DWORD)(s - (lp + written)),
		      &len, NULL);
	}
	if (len == (int)l)
	{
	    /* Finished a line, add a NL, unless this line should not have
	     * one. */
	    if (lnum != curbuf->b_op_end.lnum
		|| (!curbuf->b_p_bin
		    && curbuf->b_p_fixeol)
		|| (lnum != curbuf->b_no_eol_lnum
		    && (lnum != curbuf->b_ml.ml_line_count
			|| curbuf->b_p_eol)))
	    {
		WriteFile(g_hChildStd_IN_Wr, "\n", 1,
						  (LPDWORD)&vim_ignored, NULL);
	    }

	    ++lnum;
	    if (lnum > curbuf->b_op_end.lnum)
		break;

	    lp = ml_get(lnum);
	    written = 0;
	}
	else if (len > 0)
	    written += len;
    }

    /* finished all the lines, close pipe */
    CloseHandle(g_hChildStd_IN_Wr);
    return 0;
}


# define BUFLEN 100	/* length for buffer, stolen from unix version */

/*
 * This function read from the children's stdout and write the
 * data on screen or in the buffer accordingly.
 */
    static void
dump_pipe(int	    options,
	  HANDLE    g_hChildStd_OUT_Rd,
	  garray_T  *ga,
	  char_u    buffer[],
	  DWORD	    *buffer_off)
{
    DWORD	availableBytes = 0;
    DWORD	i;
    int		ret;
    DWORD	len;
    DWORD	toRead;
    int		repeatCount;

    /* we query the pipe to see if there is any data to read
     * to avoid to perform a blocking read */
    ret = PeekNamedPipe(g_hChildStd_OUT_Rd, /* pipe to query */
			NULL,		    /* optional buffer */
			0,		    /* buffer size */
			NULL,		    /* number of read bytes */
			&availableBytes,    /* available bytes total */
			NULL);		    /* byteLeft */

    repeatCount = 0;
    /* We got real data in the pipe, read it */
    while (ret != 0 && availableBytes > 0)
    {
	repeatCount++;
	toRead = (DWORD)(BUFLEN - *buffer_off);
	toRead = availableBytes < toRead ? availableBytes : toRead;
	ReadFile(g_hChildStd_OUT_Rd, buffer + *buffer_off, toRead , &len, NULL);

	/* If we haven't read anything, there is a problem */
	if (len == 0)
	    break;

	availableBytes -= len;

	if (options & SHELL_READ)
	{
	    /* Do NUL -> NL translation, append NL separated
	     * lines to the current buffer. */
	    for (i = 0; i < len; ++i)
	    {
		if (buffer[i] == NL)
		    append_ga_line(ga);
		else if (buffer[i] == NUL)
		    ga_append(ga, NL);
		else
		    ga_append(ga, buffer[i]);
	    }
	}
	else if (has_mbyte)
	{
	    int		l;
	    int		c;
	    char_u	*p;

	    len += *buffer_off;
	    buffer[len] = NUL;

	    /* Check if the last character in buffer[] is
	     * incomplete, keep these bytes for the next
	     * round. */
	    for (p = buffer; p < buffer + len; p += l)
	    {
		l = MB_CPTR2LEN(p);
		if (l == 0)
		    l = 1;  /* NUL byte? */
		else if (MB_BYTE2LEN(*p) != l)
		    break;
	    }
	    if (p == buffer)	/* no complete character */
	    {
		/* avoid getting stuck at an illegal byte */
		if (len >= 12)
		    ++p;
		else
		{
		    *buffer_off = len;
		    return;
		}
	    }
	    c = *p;
	    *p = NUL;
	    msg_puts((char *)buffer);
	    if (p < buffer + len)
	    {
		*p = c;
		*buffer_off = (DWORD)((buffer + len) - p);
		mch_memmove(buffer, p, *buffer_off);
		return;
	    }
	    *buffer_off = 0;
	}
	else
	{
	    buffer[len] = NUL;
	    msg_puts((char *)buffer);
	}

	windgoto(msg_row, msg_col);
	cursor_on();
	out_flush();
    }
}

/*
 * Version of system to use for windows NT > 5.0 (Win2K), use pipe
 * for communication and doesn't open any new window.
 */
    static int
mch_system_piped(char *cmd, int options)
{
    STARTUPINFO		si;
    PROCESS_INFORMATION pi;
    DWORD		ret = 0;

    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    char_u	buffer[BUFLEN + 1]; /* reading buffer + size */
    DWORD	len;

    /* buffer used to receive keys */
    char_u	ta_buf[BUFLEN + 1];	/* TypeAHead */
    int		ta_len = 0;		/* valid bytes in ta_buf[] */

    DWORD	i;
    int		c;
    int		noread_cnt = 0;
    garray_T	ga;
    int		delay = 1;
    DWORD	buffer_off = 0;	/* valid bytes in buffer[] */
    char	*p = NULL;

    SECURITY_ATTRIBUTES saAttr;

    /* Set the bInheritHandle flag so pipe handles are inherited. */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)
	/* Ensure the read handle to the pipe for STDOUT is not inherited. */
       || ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)
	/* Create a pipe for the child process's STDIN. */
       || ! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)
	/* Ensure the write handle to the pipe for STDIN is not inherited. */
       || ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
    {
	CloseHandle(g_hChildStd_IN_Rd);
	CloseHandle(g_hChildStd_IN_Wr);
	CloseHandle(g_hChildStd_OUT_Rd);
	CloseHandle(g_hChildStd_OUT_Wr);
	msg_puts(_("\nCannot create pipes\n"));
    }

    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

    /* set-up our file redirection */
    si.hStdError = g_hChildStd_OUT_Wr;
    si.hStdOutput = g_hChildStd_OUT_Wr;
    si.hStdInput = g_hChildStd_IN_Rd;
    si.wShowWindow = SW_HIDE;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;

    if (options & SHELL_READ)
	ga_init2(&ga, 1, BUFLEN);

    if (cmd != NULL)
    {
	p = (char *)vim_strsave((char_u *)cmd);
	if (p != NULL)
	    unescape_shellxquote((char_u *)p, p_sxe);
	else
	    p = cmd;
    }

    /* Now, run the command.
     * About "Inherit handles" being TRUE: this command can be litigious,
     * handle inheritance was deactivated for pending temp file, but, if we
     * deactivate it, the pipes don't work for some reason. */
     vim_create_process(p, TRUE, CREATE_DEFAULT_ERROR_MODE,
	     &si, &pi, NULL, NULL);

    if (p != cmd)
	vim_free(p);

    /* Close our unused side of the pipes */
    CloseHandle(g_hChildStd_IN_Rd);
    CloseHandle(g_hChildStd_OUT_Wr);

    if (options & SHELL_WRITE)
    {
	HANDLE thread = (HANDLE)
	 _beginthreadex(NULL,  /* security attributes */
			0,     /* default stack size */
			sub_process_writer, /* function to be executed */
			g_hChildStd_IN_Wr,  /* parameter */
			0,		 /* creation flag, start immediately */
			NULL);		    /* we don't care about thread id */
	CloseHandle(thread);
	g_hChildStd_IN_Wr = NULL;
    }

    /* Keep updating the window while waiting for the shell to finish. */
    for (;;)
    {
	MSG	msg;

	if (pPeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE))
	{
	    TranslateMessage(&msg);
	    pDispatchMessage(&msg);
	}

	/* write pipe information in the window */
	if ((options & (SHELL_READ|SHELL_WRITE))
# ifdef FEAT_GUI
		|| gui.in_use
# endif
	    )
	{
	    len = 0;
	    if (!(options & SHELL_EXPAND)
		&& ((options &
			(SHELL_READ|SHELL_WRITE|SHELL_COOKED))
		    != (SHELL_READ|SHELL_WRITE|SHELL_COOKED)
# ifdef FEAT_GUI
		    || gui.in_use
# endif
		    )
		&& (ta_len > 0 || noread_cnt > 4))
	    {
		if (ta_len == 0)
		{
		    /* Get extra characters when we don't have any.  Reset the
		     * counter and timer. */
		    noread_cnt = 0;
		    len = ui_inchar(ta_buf, BUFLEN, 10L, 0);
		}
		if (ta_len > 0 || len > 0)
		{
		    /*
		     * For pipes: Check for CTRL-C: send interrupt signal to
		     * child.  Check for CTRL-D: EOF, close pipe to child.
		     */
		    if (len == 1 && cmd != NULL)
		    {
			if (ta_buf[ta_len] == Ctrl_C)
			{
			    /* Learn what exit code is expected, for
				* now put 9 as SIGKILL */
			    TerminateProcess(pi.hProcess, 9);
			}
			if (ta_buf[ta_len] == Ctrl_D)
			{
			    CloseHandle(g_hChildStd_IN_Wr);
			    g_hChildStd_IN_Wr = NULL;
			}
		    }

		    /* replace K_BS by <BS> and K_DEL by <DEL> */
		    for (i = ta_len; i < ta_len + len; ++i)
		    {
			if (ta_buf[i] == CSI && len - i > 2)
			{
			    c = TERMCAP2KEY(ta_buf[i + 1], ta_buf[i + 2]);
			    if (c == K_DEL || c == K_KDEL || c == K_BS)
			    {
				mch_memmove(ta_buf + i + 1, ta_buf + i + 3,
					    (size_t)(len - i - 2));
				if (c == K_DEL || c == K_KDEL)
				    ta_buf[i] = DEL;
				else
				    ta_buf[i] = Ctrl_H;
				len -= 2;
			    }
			}
			else if (ta_buf[i] == '\r')
			    ta_buf[i] = '\n';
			if (has_mbyte)
			    i += (*mb_ptr2len_len)(ta_buf + i,
						    ta_len + len - i) - 1;
		    }

		    /*
		     * For pipes: echo the typed characters.  For a pty this
		     * does not seem to work.
		     */
		    for (i = ta_len; i < ta_len + len; ++i)
		    {
			if (ta_buf[i] == '\n' || ta_buf[i] == '\b')
			    msg_putchar(ta_buf[i]);
			else if (has_mbyte)
			{
			    int l = (*mb_ptr2len)(ta_buf + i);

			    msg_outtrans_len(ta_buf + i, l);
			    i += l - 1;
			}
			else
			    msg_outtrans_len(ta_buf + i, 1);
		    }
		    windgoto(msg_row, msg_col);
		    out_flush();

		    ta_len += len;

		    /*
		     * Write the characters to the child, unless EOF has been
		     * typed for pipes.  Write one character at a time, to
		     * avoid losing too much typeahead.  When writing buffer
		     * lines, drop the typed characters (only check for
		     * CTRL-C).
		     */
		    if (options & SHELL_WRITE)
			ta_len = 0;
		    else if (g_hChildStd_IN_Wr != NULL)
		    {
			WriteFile(g_hChildStd_IN_Wr, (char*)ta_buf,
				    1, &len, NULL);
			// if we are typing in, we want to keep things reactive
			delay = 1;
			if (len > 0)
			{
			    ta_len -= len;
			    mch_memmove(ta_buf, ta_buf + len, ta_len);
			}
		    }
		}
	    }
	}

	if (ta_len)
	    ui_inchar_undo(ta_buf, ta_len);

	if (WaitForSingleObject(pi.hProcess, delay) != WAIT_TIMEOUT)
	{
	    dump_pipe(options, g_hChildStd_OUT_Rd, &ga, buffer, &buffer_off);
	    break;
	}

	++noread_cnt;
	dump_pipe(options, g_hChildStd_OUT_Rd, &ga, buffer, &buffer_off);

	/* We start waiting for a very short time and then increase it, so
	 * that we respond quickly when the process is quick, and don't
	 * consume too much overhead when it's slow. */
	if (delay < 50)
	    delay += 10;
    }

    /* Close the pipe */
    CloseHandle(g_hChildStd_OUT_Rd);
    if (g_hChildStd_IN_Wr != NULL)
	CloseHandle(g_hChildStd_IN_Wr);

    WaitForSingleObject(pi.hProcess, INFINITE);

    /* Get the command exit code */
    GetExitCodeProcess(pi.hProcess, &ret);

    if (options & SHELL_READ)
    {
	if (ga.ga_len > 0)
	{
	    append_ga_line(&ga);
	    /* remember that the NL was missing */
	    curbuf->b_no_eol_lnum = curwin->w_cursor.lnum;
	}
	else
	    curbuf->b_no_eol_lnum = 0;
	ga_clear(&ga);
    }

    /* Close the handles to the subprocess, so that it goes away */
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return ret;
}

    static int
mch_system(char *cmd, int options)
{
    /* if we can pipe and the shelltemp option is off */
    if (!p_stmp)
	return mch_system_piped(cmd, options);
    else
	return mch_system_classic(cmd, options);
}
#else

    static int
mch_system(char *cmd, int options)
{
    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR	*wcmd = enc_to_utf16((char_u *)cmd, NULL);
	if (wcmd != NULL)
	{
	    int ret = _wsystem(wcmd);
	    vim_free(wcmd);
	    return ret;
	}
    }
    return system(cmd);
}

#endif

#if defined(FEAT_GUI) && defined(FEAT_TERMINAL)
/*
 * Use a terminal window to run a shell command in.
 */
    static int
mch_call_shell_terminal(
    char_u	*cmd,
    int		options UNUSED)	/* SHELL_*, see vim.h */
{
    jobopt_T	opt;
    char_u	*newcmd = NULL;
    typval_T	argvar[2];
    long_u	cmdlen;
    int		retval = -1;
    buf_T	*buf;
    job_T	*job;
    aco_save_T	aco;
    oparg_T	oa;		/* operator arguments */

    if (cmd == NULL)
	cmdlen = STRLEN(p_sh) + 1;
    else
	cmdlen = STRLEN(p_sh) + STRLEN(p_shcf) + STRLEN(cmd) + 10;
    newcmd = lalloc(cmdlen, TRUE);
    if (newcmd == NULL)
	return 255;
    if (cmd == NULL)
    {
	STRCPY(newcmd, p_sh);
	ch_log(NULL, "starting terminal to run a shell");
    }
    else
    {
	vim_snprintf((char *)newcmd, cmdlen, "%s %s %s", p_sh, p_shcf, cmd);
	ch_log(NULL, "starting terminal for system command '%s'", cmd);
    }

    init_job_options(&opt);

    argvar[0].v_type = VAR_STRING;
    argvar[0].vval.v_string = newcmd;
    argvar[1].v_type = VAR_UNKNOWN;
    buf = term_start(argvar, NULL, &opt, TERM_START_SYSTEM);
    if (buf == NULL)
    {
	vim_free(newcmd);
	return 255;
    }

    job = term_getjob(buf->b_term);
    ++job->jv_refcount;

    /* Find a window to make "buf" curbuf. */
    aucmd_prepbuf(&aco, buf);

    clear_oparg(&oa);
    while (term_use_loop())
    {
	if (oa.op_type == OP_NOP && oa.regname == NUL && !VIsual_active)
	{
	    /* If terminal_loop() returns OK we got a key that is handled
	     * in Normal model. We don't do redrawing anyway. */
	    if (terminal_loop(TRUE) == OK)
		normal_cmd(&oa, TRUE);
	}
	else
	    normal_cmd(&oa, TRUE);
    }
    retval = job->jv_exitval;
    ch_log(NULL, "system command finished");

    job_unref(job);

    /* restore curwin/curbuf and a few other things */
    aucmd_restbuf(&aco);

    wait_return(TRUE);
    do_buffer(DOBUF_WIPE, DOBUF_FIRST, FORWARD, buf->b_fnum, TRUE);

    vim_free(newcmd);
    return retval;
}
#endif

/*
 * Either execute a command by calling the shell or start a new shell
 */
    int
mch_call_shell(
    char_u  *cmd,
    int	    options)	/* SHELL_*, see vim.h */
{
    int		x = 0;
    int		tmode = cur_tmode;
#ifdef FEAT_TITLE
    char	szShellTitle[512];
    int		did_set_title = FALSE;

    /* Change the title to reflect that we are in a subshell. */
    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR szShellTitle[512];

	if (GetConsoleTitleW(szShellTitle,
				  sizeof(szShellTitle)/sizeof(WCHAR) - 4) > 0)
	{
	    if (cmd == NULL)
		wcscat(szShellTitle, L" :sh");
	    else
	    {
		WCHAR *wn = enc_to_utf16((char_u *)cmd, NULL);

		if (wn != NULL)
		{
		    wcscat(szShellTitle, L" - !");
		    if ((wcslen(szShellTitle) + wcslen(wn) <
					  sizeof(szShellTitle)/sizeof(WCHAR)))
			wcscat(szShellTitle, wn);
		    SetConsoleTitleW(szShellTitle);
		    vim_free(wn);
		    did_set_title = TRUE;
		}
	    }
	}
    }
    if (!did_set_title)
	/* Change the title to reflect that we are in a subshell. */
	if (GetConsoleTitle(szShellTitle, sizeof(szShellTitle) - 4) > 0)
	{
	    if (cmd == NULL)
		strcat(szShellTitle, " :sh");
	    else
	    {
		strcat(szShellTitle, " - !");
		if ((strlen(szShellTitle) + strlen((char *)cmd)
			    < sizeof(szShellTitle)))
		    strcat(szShellTitle, (char *)cmd);
	    }
	    SetConsoleTitle(szShellTitle);
	}
#endif

    out_flush();

#ifdef MCH_WRITE_DUMP
    if (fdDump)
    {
	fprintf(fdDump, "mch_call_shell(\"%s\", %d)\n", cmd, options);
	fflush(fdDump);
    }
#endif
#if defined(FEAT_GUI) && defined(FEAT_TERMINAL)
    /* TODO: make the terminal window work with input or output redirected. */
    if (vim_strchr(p_go, GO_TERMINAL) != NULL
	 && (options & (SHELL_FILTER|SHELL_DOOUT|SHELL_WRITE|SHELL_READ)) == 0)
    {
	/* Use a terminal window to run the command in. */
	x = mch_call_shell_terminal(cmd, options);
# ifdef FEAT_TITLE
	resettitle();
# endif
	return x;
    }
#endif

    /*
     * Catch all deadly signals while running the external command, because a
     * CTRL-C, Ctrl-Break or illegal instruction  might otherwise kill us.
     */
    signal(SIGINT, SIG_IGN);
#if defined(__GNUC__) && !defined(__MINGW32__)
    signal(SIGKILL, SIG_IGN);
#else
    signal(SIGBREAK, SIG_IGN);
#endif
    signal(SIGILL, SIG_IGN);
    signal(SIGFPE, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGABRT, SIG_IGN);

    if (options & SHELL_COOKED)
	settmode(TMODE_COOK);	/* set to normal mode */

    if (cmd == NULL)
    {
	x = mch_system((char *)p_sh, options);
    }
    else
    {
	/* we use "command" or "cmd" to start the shell; slow but easy */
	char_u	*newcmd = NULL;
	char_u	*cmdbase = cmd;
	long_u	cmdlen;

	/* Skip a leading ", ( and "(. */
	if (*cmdbase == '"' )
	    ++cmdbase;
	if (*cmdbase == '(')
	    ++cmdbase;

	if ((STRNICMP(cmdbase, "start", 5) == 0) && VIM_ISWHITE(cmdbase[5]))
	{
	    STARTUPINFO		si;
	    PROCESS_INFORMATION	pi;
	    DWORD		flags = CREATE_NEW_CONSOLE;
	    INT			n_show_cmd = SW_SHOWNORMAL;
	    char_u		*p;

	    ZeroMemory(&si, sizeof(si));
	    si.cb = sizeof(si);
	    si.lpReserved = NULL;
	    si.lpDesktop = NULL;
	    si.lpTitle = NULL;
	    si.dwFlags = 0;
	    si.cbReserved2 = 0;
	    si.lpReserved2 = NULL;

	    cmdbase = skipwhite(cmdbase + 5);
	    if ((STRNICMP(cmdbase, "/min", 4) == 0)
		    && VIM_ISWHITE(cmdbase[4]))
	    {
		cmdbase = skipwhite(cmdbase + 4);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWMINNOACTIVE;
		n_show_cmd = SW_SHOWMINNOACTIVE;
	    }
	    else if ((STRNICMP(cmdbase, "/b", 2) == 0)
		    && VIM_ISWHITE(cmdbase[2]))
	    {
		cmdbase = skipwhite(cmdbase + 2);
		flags = CREATE_NO_WINDOW;
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = CreateFile("\\\\.\\NUL",	// File name
		    GENERIC_READ,			// Access flags
		    0,					// Share flags
		    NULL,				// Security att.
		    OPEN_EXISTING,			// Open flags
		    FILE_ATTRIBUTE_NORMAL,		// File att.
		    NULL);				// Temp file
		si.hStdOutput = si.hStdInput;
		si.hStdError = si.hStdInput;
	    }

	    /* Remove a trailing ", ) and )" if they have a match
	     * at the start of the command. */
	    if (cmdbase > cmd)
	    {
		p = cmdbase + STRLEN(cmdbase);
		if (p > cmdbase && p[-1] == '"' && *cmd == '"')
		    *--p = NUL;
		if (p > cmdbase && p[-1] == ')'
			&& (*cmd =='(' || cmd[1] == '('))
		    *--p = NUL;
	    }

	    newcmd = cmdbase;
	    unescape_shellxquote(cmdbase, p_sxe);

	    /*
	     * If creating new console, arguments are passed to the
	     * 'cmd.exe' as-is. If it's not, arguments are not treated
	     * correctly for current 'cmd.exe'. So unescape characters in
	     * shellxescape except '|' for avoiding to be treated as
	     * argument to them. Pass the arguments to sub-shell.
	     */
	    if (flags != CREATE_NEW_CONSOLE)
	    {
		char_u	*subcmd;
		char_u	*cmd_shell = mch_getenv("COMSPEC");

		if (cmd_shell == NULL || *cmd_shell == NUL)
		    cmd_shell = (char_u *)default_shell();

		subcmd = vim_strsave_escaped_ext(cmdbase,
			(char_u *)"|", '^', FALSE);
		if (subcmd != NULL)
		{
		    /* make "cmd.exe /c arguments" */
		    cmdlen = STRLEN(cmd_shell) + STRLEN(subcmd) + 5;
		    newcmd = lalloc(cmdlen, TRUE);
		    if (newcmd != NULL)
			vim_snprintf((char *)newcmd, cmdlen, "%s /c %s",
						       cmd_shell, subcmd);
		    else
			newcmd = cmdbase;
		    vim_free(subcmd);
		}
	    }

	    /*
	     * Now, start the command as a process, so that it doesn't
	     * inherit our handles which causes unpleasant dangling swap
	     * files if we exit before the spawned process
	     */
	    if (vim_create_process((char *)newcmd, FALSE, flags,
			&si, &pi, NULL, NULL))
		x = 0;
	    else if (vim_shell_execute((char *)newcmd, n_show_cmd)
							       > (HINSTANCE)32)
		x = 0;
	    else
	    {
		x = -1;
#ifdef FEAT_GUI_MSWIN
		emsg(_("E371: Command not found"));
#endif
	    }

	    if (newcmd != cmdbase)
		vim_free(newcmd);

	    if (si.dwFlags == STARTF_USESTDHANDLES && si.hStdInput != NULL)
	    {
		/* Close the handle to \\.\NUL created above. */
		CloseHandle(si.hStdInput);
	    }
	    /* Close the handles to the subprocess, so that it goes away */
	    CloseHandle(pi.hThread);
	    CloseHandle(pi.hProcess);
	}
	else
	{
	    cmdlen = (
#ifdef FEAT_GUI_MSWIN
		(!p_stmp ? 0 : STRLEN(vimrun_path)) +
#endif
		STRLEN(p_sh) + STRLEN(p_shcf) + STRLEN(cmd) + 10);

	    newcmd = lalloc(cmdlen, TRUE);
	    if (newcmd != NULL)
	    {
#if defined(FEAT_GUI_MSWIN)
		if (need_vimrun_warning)
		{
		    char *msg = _("VIMRUN.EXE not found in your $PATH.\n"
			"External commands will not pause after completion.\n"
			"See  :help win32-vimrun  for more information.");
		    char *title = _("Vim Warning");
		    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
		    {
			WCHAR *wmsg = enc_to_utf16((char_u *)msg, NULL);
			WCHAR *wtitle = enc_to_utf16((char_u *)title, NULL);

			if (wmsg != NULL && wtitle != NULL)
			    MessageBoxW(NULL, wmsg, wtitle, MB_ICONWARNING);
			vim_free(wmsg);
			vim_free(wtitle);
		    }
		    else
			MessageBox(NULL, msg, title, MB_ICONWARNING);
		    need_vimrun_warning = FALSE;
		}
		if (!s_dont_use_vimrun && p_stmp)
		    /* Use vimrun to execute the command.  It opens a console
		     * window, which can be closed without killing Vim. */
		    vim_snprintf((char *)newcmd, cmdlen, "%s%s%s %s %s",
			    vimrun_path,
			    (msg_silent != 0 || (options & SHELL_DOOUT))
								 ? "-s " : "",
			    p_sh, p_shcf, cmd);
		else
#endif
		    vim_snprintf((char *)newcmd, cmdlen, "%s %s %s",
							   p_sh, p_shcf, cmd);
		x = mch_system((char *)newcmd, options);
		vim_free(newcmd);
	    }
	}
    }

    if (tmode == TMODE_RAW)
	settmode(TMODE_RAW);	/* set to raw mode */

    /* Print the return value, unless "vimrun" was used. */
    if (x != 0 && !(options & SHELL_SILENT) && !emsg_silent
#if defined(FEAT_GUI_MSWIN)
		&& ((options & SHELL_DOOUT) || s_dont_use_vimrun || !p_stmp)
#endif
	    )
    {
	smsg(_("shell returned %d"), x);
	msg_putchar('\n');
    }
#ifdef FEAT_TITLE
    resettitle();
#endif

    signal(SIGINT, SIG_DFL);
#if defined(__GNUC__) && !defined(__MINGW32__)
    signal(SIGKILL, SIG_DFL);
#else
    signal(SIGBREAK, SIG_DFL);
#endif
    signal(SIGILL, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGABRT, SIG_DFL);

    return x;
}

#if defined(FEAT_JOB_CHANNEL) || defined(PROTO)
    static HANDLE
job_io_file_open(
	char_u *fname,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes)
{
    HANDLE h;
    WCHAR *wn = NULL;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	wn = enc_to_utf16(fname, NULL);
	if (wn != NULL)
	{
	    h = CreateFileW(wn, dwDesiredAccess, dwShareMode,
		    lpSecurityAttributes, dwCreationDisposition,
		    dwFlagsAndAttributes, NULL);
	    vim_free(wn);
	}
    }
    if (wn == NULL)
	h = CreateFile((LPCSTR)fname, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition,
		dwFlagsAndAttributes, NULL);
    return h;
}

/*
 * Turn the dictionary "env" into a NUL separated list that can be used as the
 * environment argument of vim_create_process().
 */
    void
win32_build_env(dict_T *env, garray_T *gap, int is_terminal)
{
    hashitem_T	*hi;
    long_u	todo = env != NULL ? env->dv_hashtab.ht_used : 0;
    LPVOID	base = GetEnvironmentStringsW();

    /* for last \0 */
    if (ga_grow(gap, 1) == FAIL)
	return;

    if (base)
    {
	WCHAR	*p = (WCHAR*) base;

	/* for last \0 */
	if (ga_grow(gap, 1) == FAIL)
	    return;

	while (*p != 0 || *(p + 1) != 0)
	{
	    if (ga_grow(gap, 1) == OK)
		*((WCHAR*)gap->ga_data + gap->ga_len++) = *p;
	    p++;
	}
	FreeEnvironmentStrings(base);
	*((WCHAR*)gap->ga_data + gap->ga_len++) = L'\0';
    }

    if (env != NULL)
    {
	for (hi = env->dv_hashtab.ht_array; todo > 0; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		typval_T *item = &dict_lookup(hi)->di_tv;
		WCHAR   *wkey = enc_to_utf16((char_u *)hi->hi_key, NULL);
		WCHAR   *wval = enc_to_utf16(tv_get_string(item), NULL);
		--todo;
		if (wkey != NULL && wval != NULL)
		{
		    size_t	n;
		    size_t	lkey = wcslen(wkey);
		    size_t	lval = wcslen(wval);

		    if (ga_grow(gap, (int)(lkey + lval + 2)) != OK)
			continue;
		    for (n = 0; n < lkey; n++)
			*((WCHAR*)gap->ga_data + gap->ga_len++) = wkey[n];
		    *((WCHAR*)gap->ga_data + gap->ga_len++) = L'=';
		    for (n = 0; n < lval; n++)
			*((WCHAR*)gap->ga_data + gap->ga_len++) = wval[n];
		    *((WCHAR*)gap->ga_data + gap->ga_len++) = L'\0';
		}
		vim_free(wkey);
		vim_free(wval);
	    }
	}
    }

# if defined(FEAT_CLIENTSERVER) || defined(FEAT_TERMINAL)
    {
#  ifdef FEAT_CLIENTSERVER
	char_u	*servername = get_vim_var_str(VV_SEND_SERVER);
	size_t	servername_len = STRLEN(servername);
#  endif
#  ifdef FEAT_TERMINAL
	char_u	*version = get_vim_var_str(VV_VERSION);
	size_t	version_len = STRLEN(version);
#  endif
	// size of "VIM_SERVERNAME=" and value,
	// plus "VIM_TERMINAL=" and value,
	// plus two terminating NULs
	size_t	n = 0
#  ifdef FEAT_CLIENTSERVER
		    + 15 + servername_len
#  endif
#  ifdef FEAT_TERMINAL
		    + 13 + version_len + 2
#  endif
		    ;

	if (ga_grow(gap, (int)n) == OK)
	{
#  ifdef FEAT_CLIENTSERVER
	    for (n = 0; n < 15; n++)
		*((WCHAR*)gap->ga_data + gap->ga_len++) =
		    (WCHAR)"VIM_SERVERNAME="[n];
	    for (n = 0; n < servername_len; n++)
		*((WCHAR*)gap->ga_data + gap->ga_len++) =
		    (WCHAR)servername[n];
	    *((WCHAR*)gap->ga_data + gap->ga_len++) = L'\0';
#  endif
#  ifdef FEAT_TERMINAL
	    if (is_terminal)
	    {
		for (n = 0; n < 13; n++)
		    *((WCHAR*)gap->ga_data + gap->ga_len++) =
			(WCHAR)"VIM_TERMINAL="[n];
		for (n = 0; n < version_len; n++)
		    *((WCHAR*)gap->ga_data + gap->ga_len++) =
			(WCHAR)version[n];
		*((WCHAR*)gap->ga_data + gap->ga_len++) = L'\0';
	    }
#  endif
	}
    }
# endif
}

/*
 * Create a pair of pipes.
 * Return TRUE for success, FALSE for failure.
 */
    static BOOL
create_pipe_pair(HANDLE handles[2])
{
    static LONG		s;
    char		name[64];
    SECURITY_ATTRIBUTES sa;

    sprintf(name, "\\\\?\\pipe\\vim-%08lx-%08lx",
	    GetCurrentProcessId(),
	    InterlockedIncrement(&s));

    // Create named pipe. Max size of named pipe is 65535.
    handles[1] = CreateNamedPipe(
	    name,
	    PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
	    PIPE_TYPE_BYTE | PIPE_NOWAIT,
	    1, MAX_NAMED_PIPE_SIZE, 0, 0, NULL);

    if (handles[1] == INVALID_HANDLE_VALUE)
	return FALSE;

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    handles[0] = CreateFile(name,
	    FILE_GENERIC_READ,
	    FILE_SHARE_READ, &sa,
	    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (handles[0] == INVALID_HANDLE_VALUE)
    {
	CloseHandle(handles[1]);
	return FALSE;
    }

    return TRUE;
}

    void
mch_job_start(char *cmd, job_T *job, jobopt_T *options)
{
    STARTUPINFO		si;
    PROCESS_INFORMATION	pi;
    HANDLE		jo;
    SECURITY_ATTRIBUTES saAttr;
    channel_T		*channel = NULL;
    HANDLE		ifd[2];
    HANDLE		ofd[2];
    HANDLE		efd[2];
    garray_T		ga;

    int		use_null_for_in = options->jo_io[PART_IN] == JIO_NULL;
    int		use_null_for_out = options->jo_io[PART_OUT] == JIO_NULL;
    int		use_null_for_err = options->jo_io[PART_ERR] == JIO_NULL;
    int		use_file_for_in = options->jo_io[PART_IN] == JIO_FILE;
    int		use_file_for_out = options->jo_io[PART_OUT] == JIO_FILE;
    int		use_file_for_err = options->jo_io[PART_ERR] == JIO_FILE;
    int		use_out_for_err = options->jo_io[PART_ERR] == JIO_OUT;

    if (use_out_for_err && use_null_for_out)
	use_null_for_err = TRUE;

    ifd[0] = INVALID_HANDLE_VALUE;
    ifd[1] = INVALID_HANDLE_VALUE;
    ofd[0] = INVALID_HANDLE_VALUE;
    ofd[1] = INVALID_HANDLE_VALUE;
    efd[0] = INVALID_HANDLE_VALUE;
    efd[1] = INVALID_HANDLE_VALUE;
    ga_init2(&ga, (int)sizeof(wchar_t), 500);

    jo = CreateJobObject(NULL, NULL);
    if (jo == NULL)
    {
	job->jv_status = JOB_FAILED;
	goto failed;
    }

    if (options->jo_env != NULL)
	win32_build_env(options->jo_env, &ga, FALSE);

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (use_file_for_in)
    {
	char_u *fname = options->jo_io_name[PART_IN];

	ifd[0] = job_io_file_open(fname, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&saAttr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
	if (ifd[0] == INVALID_HANDLE_VALUE)
	{
	    semsg(_(e_notopen), fname);
	    goto failed;
	}
    }
    else if (!use_null_for_in
	    && (!create_pipe_pair(ifd)
		|| !SetHandleInformation(ifd[1], HANDLE_FLAG_INHERIT, 0)))
	goto failed;

    if (use_file_for_out)
    {
	char_u *fname = options->jo_io_name[PART_OUT];

	ofd[1] = job_io_file_open(fname, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&saAttr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	if (ofd[1] == INVALID_HANDLE_VALUE)
	{
	    semsg(_(e_notopen), fname);
	    goto failed;
	}
    }
    else if (!use_null_for_out &&
	    (!CreatePipe(&ofd[0], &ofd[1], &saAttr, 0)
	    || !SetHandleInformation(ofd[0], HANDLE_FLAG_INHERIT, 0)))
	goto failed;

    if (use_file_for_err)
    {
	char_u *fname = options->jo_io_name[PART_ERR];

	efd[1] = job_io_file_open(fname, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&saAttr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	if (efd[1] == INVALID_HANDLE_VALUE)
	{
	    semsg(_(e_notopen), fname);
	    goto failed;
	}
    }
    else if (!use_out_for_err && !use_null_for_err &&
	    (!CreatePipe(&efd[0], &efd[1], &saAttr, 0)
	    || !SetHandleInformation(efd[0], HANDLE_FLAG_INHERIT, 0)))
	goto failed;

    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = ifd[0];
    si.hStdOutput = ofd[1];
    si.hStdError = use_out_for_err ? ofd[1] : efd[1];

    if (!use_null_for_in || !use_null_for_out || !use_null_for_err)
    {
	if (options->jo_set & JO_CHANNEL)
	{
	    channel = options->jo_channel;
	    if (channel != NULL)
		++channel->ch_refcount;
	}
	else
	    channel = add_channel();
	if (channel == NULL)
	    goto failed;
    }

    if (!vim_create_process(cmd, TRUE,
	    CREATE_SUSPENDED |
	    CREATE_DEFAULT_ERROR_MODE |
	    CREATE_NEW_PROCESS_GROUP |
	    CREATE_UNICODE_ENVIRONMENT |
	    CREATE_NEW_CONSOLE,
	    &si, &pi,
	    ga.ga_data,
	    (char *)options->jo_cwd))
    {
	CloseHandle(jo);
	job->jv_status = JOB_FAILED;
	goto failed;
    }

    ga_clear(&ga);

    if (!AssignProcessToJobObject(jo, pi.hProcess))
    {
	/* if failing, switch the way to terminate
	 * process with TerminateProcess. */
	CloseHandle(jo);
	jo = NULL;
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    job->jv_proc_info = pi;
    job->jv_job_object = jo;
    job->jv_status = JOB_STARTED;

    CloseHandle(ifd[0]);
    CloseHandle(ofd[1]);
    if (!use_out_for_err && !use_null_for_err)
	CloseHandle(efd[1]);

    job->jv_channel = channel;
    if (channel != NULL)
    {
	channel_set_pipes(channel,
		      use_file_for_in || use_null_for_in
					      ? INVALID_FD : (sock_T)ifd[1],
		      use_file_for_out || use_null_for_out
					     ? INVALID_FD : (sock_T)ofd[0],
		      use_out_for_err || use_file_for_err || use_null_for_err
					    ? INVALID_FD : (sock_T)efd[0]);
	channel_set_job(channel, job, options);
    }
    return;

failed:
    CloseHandle(ifd[0]);
    CloseHandle(ofd[0]);
    CloseHandle(efd[0]);
    CloseHandle(ifd[1]);
    CloseHandle(ofd[1]);
    CloseHandle(efd[1]);
    channel_unref(channel);
    ga_clear(&ga);
}

    char *
mch_job_status(job_T *job)
{
    DWORD dwExitCode = 0;

    if (!GetExitCodeProcess(job->jv_proc_info.hProcess, &dwExitCode)
	    || dwExitCode != STILL_ACTIVE)
    {
	job->jv_exitval = (int)dwExitCode;
	if (job->jv_status < JOB_ENDED)
	{
	    ch_log(job->jv_channel, "Job ended");
	    job->jv_status = JOB_ENDED;
	}
	return "dead";
    }
    return "run";
}

    job_T *
mch_detect_ended_job(job_T *job_list)
{
    HANDLE jobHandles[MAXIMUM_WAIT_OBJECTS];
    job_T *jobArray[MAXIMUM_WAIT_OBJECTS];
    job_T *job = job_list;

    while (job != NULL)
    {
	DWORD n;
	DWORD result;

	for (n = 0; n < MAXIMUM_WAIT_OBJECTS
				       && job != NULL; job = job->jv_next)
	{
	    if (job->jv_status == JOB_STARTED)
	    {
		jobHandles[n] = job->jv_proc_info.hProcess;
		jobArray[n] = job;
		++n;
	    }
	}
	if (n == 0)
	    continue;
	result = WaitForMultipleObjects(n, jobHandles, FALSE, 0);
	if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + n)
	{
	    job_T *wait_job = jobArray[result - WAIT_OBJECT_0];

	    if (STRCMP(mch_job_status(wait_job), "dead") == 0)
		return wait_job;
	}
    }
    return NULL;
}

    static BOOL
terminate_all(HANDLE process, int code)
{
    PROCESSENTRY32  pe;
    HANDLE	    h = INVALID_HANDLE_VALUE;
    DWORD	    pid = GetProcessId(process);

    if (pid != 0)
    {
	h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
	    pe.dwSize = sizeof(PROCESSENTRY32);
	    if (!Process32First(h, &pe))
		goto theend;

	    do
	    {
		if (pe.th32ParentProcessID == pid)
		{
		    HANDLE ph = OpenProcess(
				  PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
		    if (ph != NULL)
		    {
			terminate_all(ph, code);
			CloseHandle(ph);
		    }
		}
	    } while (Process32Next(h, &pe));

	    CloseHandle(h);
	}
    }

theend:
    return TerminateProcess(process, code);
}

/*
 * Send a (deadly) signal to "job".
 * Return FAIL if it didn't work.
 */
    int
mch_signal_job(job_T *job, char_u *how)
{
    int ret;

    if (STRCMP(how, "term") == 0 || STRCMP(how, "kill") == 0 || *how == NUL)
    {
	/* deadly signal */
	if (job->jv_job_object != NULL)
	{
	    if (job->jv_channel != NULL && job->jv_channel->ch_anonymous_pipe)
		job->jv_channel->ch_killing = TRUE;
	    return TerminateJobObject(job->jv_job_object, 0) ? OK : FAIL;
	}
	return terminate_all(job->jv_proc_info.hProcess, 0) ? OK : FAIL;
    }

    if (!AttachConsole(job->jv_proc_info.dwProcessId))
	return FAIL;
    ret = GenerateConsoleCtrlEvent(
		STRCMP(how, "int") == 0 ? CTRL_C_EVENT : CTRL_BREAK_EVENT,
		job->jv_proc_info.dwProcessId)
	    ? OK : FAIL;
    FreeConsole();
    return ret;
}

/*
 * Clear the data related to "job".
 */
    void
mch_clear_job(job_T *job)
{
    if (job->jv_status != JOB_FAILED)
    {
	if (job->jv_job_object != NULL)
	    CloseHandle(job->jv_job_object);
	CloseHandle(job->jv_proc_info.hProcess);
    }
}
#endif


#ifndef FEAT_GUI_MSWIN

/*
 * Start termcap mode
 */
    static void
termcap_mode_start(void)
{
    DWORD cmodein;

    if (g_fTermcapMode)
	return;

    SaveConsoleBuffer(&g_cbNonTermcap);

    if (g_cbTermcap.IsValid)
    {
	/*
	 * We've been in termcap mode before.  Restore certain screen
	 * characteristics, including the buffer size and the window
	 * size.  Since we will be redrawing the screen, we don't need
	 * to restore the actual contents of the buffer.
	 */
	RestoreConsoleBuffer(&g_cbTermcap, FALSE);
	reset_console_color_rgb();
	SetConsoleWindowInfo(g_hConOut, TRUE, &g_cbTermcap.Info.srWindow);
	Rows = g_cbTermcap.Info.dwSize.Y;
	Columns = g_cbTermcap.Info.dwSize.X;
    }
    else
    {
	/*
	 * This is our first time entering termcap mode.  Clear the console
	 * screen buffer, and resize the buffer to match the current window
	 * size.  We will use this as the size of our editing environment.
	 */
	ClearConsoleBuffer(g_attrCurrent);
	set_console_color_rgb();
	ResizeConBufAndWindow(g_hConOut, Columns, Rows);
    }

#ifdef FEAT_TITLE
    resettitle();
#endif

    GetConsoleMode(g_hConIn, &cmodein);
#ifdef FEAT_MOUSE
    if (g_fMouseActive)
	cmodein |= ENABLE_MOUSE_INPUT;
    else
	cmodein &= ~ENABLE_MOUSE_INPUT;
#endif
    cmodein |= ENABLE_WINDOW_INPUT;
    SetConsoleMode(g_hConIn, cmodein);

    redraw_later_clear();
    g_fTermcapMode = TRUE;
}


/*
 * End termcap mode
 */
    static void
termcap_mode_end(void)
{
    DWORD cmodein;
    ConsoleBuffer *cb;
    COORD coord;
    DWORD dwDummy;

    if (!g_fTermcapMode)
	return;

    SaveConsoleBuffer(&g_cbTermcap);

    GetConsoleMode(g_hConIn, &cmodein);
    cmodein &= ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
    SetConsoleMode(g_hConIn, cmodein);

#ifdef FEAT_RESTORE_ORIG_SCREEN
    cb = exiting ? &g_cbOrig : &g_cbNonTermcap;
#else
    cb = &g_cbNonTermcap;
#endif
    RestoreConsoleBuffer(cb, p_rs);
    reset_console_color_rgb();
    SetConsoleCursorInfo(g_hConOut, &g_cci);

    if (p_rs || exiting)
    {
	/*
	 * Clear anything that happens to be on the current line.
	 */
	coord.X = 0;
	coord.Y = (SHORT) (p_rs ? cb->Info.dwCursorPosition.Y : (Rows - 1));
	FillConsoleOutputCharacter(g_hConOut, ' ',
		cb->Info.dwSize.X, coord, &dwDummy);
	/*
	 * The following is just for aesthetics.  If we are exiting without
	 * restoring the screen, then we want to have a prompt string
	 * appear at the bottom line.  However, the command interpreter
	 * seems to always advance the cursor one line before displaying
	 * the prompt string, which causes the screen to scroll.  To
	 * counter this, move the cursor up one line before exiting.
	 */
	if (exiting && !p_rs)
	    coord.Y--;
	/*
	 * Position the cursor at the leftmost column of the desired row.
	 */
	SetConsoleCursorPosition(g_hConOut, coord);
    }

    g_fTermcapMode = FALSE;
}
#endif /* FEAT_GUI_MSWIN */


#ifdef FEAT_GUI_MSWIN
    void
mch_write(
    char_u  *s UNUSED,
    int	    len UNUSED)
{
    /* never used */
}

#else

/*
 * clear `n' chars, starting from `coord'
 */
    static void
clear_chars(
    COORD coord,
    DWORD n)
{
    DWORD dwDummy;

    FillConsoleOutputCharacter(g_hConOut, ' ', n, coord, &dwDummy);

    if (!USE_VTP)
	FillConsoleOutputAttribute(g_hConOut, g_attrCurrent, n, coord, &dwDummy);
    else
    {
	set_console_color_rgb();
	gotoxy(coord.X + 1, coord.Y + 1);
	vtp_printf("\033[%dX", n);
    }
}


/*
 * Clear the screen
 */
    static void
clear_screen(void)
{
    g_coord.X = g_coord.Y = 0;

    if (!USE_VTP)
	clear_chars(g_coord, Rows * Columns);
    else
    {
	set_console_color_rgb();
	gotoxy(1, 1);
	vtp_printf("\033[2J");
    }
}


/*
 * Clear to end of display
 */
    static void
clear_to_end_of_display(void)
{
    COORD save = g_coord;

    if (!USE_VTP)
	clear_chars(g_coord, (Rows - g_coord.Y - 1)
					   * Columns + (Columns - g_coord.X));
    else
    {
	set_console_color_rgb();
	gotoxy(g_coord.X + 1, g_coord.Y + 1);
	vtp_printf("\033[0J");

	gotoxy(save.X + 1, save.Y + 1);
	g_coord = save;
    }
}


/*
 * Clear to end of line
 */
    static void
clear_to_end_of_line(void)
{
    COORD save = g_coord;

    if (!USE_VTP)
	clear_chars(g_coord, Columns - g_coord.X);
    else
    {
	set_console_color_rgb();
	gotoxy(g_coord.X + 1, g_coord.Y + 1);
	vtp_printf("\033[0K");

	gotoxy(save.X + 1, save.Y + 1);
	g_coord = save;
    }
}


/*
 * Scroll the scroll region up by `cLines' lines
 */
    static void
scroll(unsigned cLines)
{
    COORD oldcoord = g_coord;

    gotoxy(g_srScrollRegion.Left + 1, g_srScrollRegion.Top + 1);
    delete_lines(cLines);

    g_coord = oldcoord;
}


/*
 * Set the scroll region
 */
    static void
set_scroll_region(
    unsigned left,
    unsigned top,
    unsigned right,
    unsigned bottom)
{
    if (left >= right
	    || top >= bottom
	    || right > (unsigned) Columns - 1
	    || bottom > (unsigned) Rows - 1)
	return;

    g_srScrollRegion.Left =   left;
    g_srScrollRegion.Top =    top;
    g_srScrollRegion.Right =  right;
    g_srScrollRegion.Bottom = bottom;
}

    static void
set_scroll_region_tb(
    unsigned top,
    unsigned bottom)
{
    if (top >= bottom || bottom > (unsigned)Rows - 1)
	return;

    g_srScrollRegion.Top = top;
    g_srScrollRegion.Bottom = bottom;
}

    static void
set_scroll_region_lr(
    unsigned left,
    unsigned right)
{
    if (left >= right || right > (unsigned)Columns - 1)
	return;

    g_srScrollRegion.Left = left;
    g_srScrollRegion.Right = right;
}


/*
 * Insert `cLines' lines at the current cursor position
 */
    static void
insert_lines(unsigned cLines)
{
    SMALL_RECT	    source, clip;
    COORD	    dest;
    CHAR_INFO	    fill;

    gotoxy(g_srScrollRegion.Left + 1, g_srScrollRegion.Top + 1);

    dest.X = g_srScrollRegion.Left;
    dest.Y = g_coord.Y + cLines;

    source.Left   = g_srScrollRegion.Left;
    source.Top	  = g_coord.Y;
    source.Right  = g_srScrollRegion.Right;
    source.Bottom = g_srScrollRegion.Bottom - cLines;

    clip.Left   = g_srScrollRegion.Left;
    clip.Top    = g_coord.Y;
    clip.Right  = g_srScrollRegion.Right;
    clip.Bottom = g_srScrollRegion.Bottom;

    fill.Char.AsciiChar = ' ';
    if (!USE_VTP)
	fill.Attributes = g_attrCurrent;
    else
	fill.Attributes = g_attrDefault;

    set_console_color_rgb();

    ScrollConsoleScreenBuffer(g_hConOut, &source, &clip, dest, &fill);

    // Here we have to deal with a win32 console flake: If the scroll
    // region looks like abc and we scroll c to a and fill with d we get
    // cbd... if we scroll block c one line at a time to a, we get cdd...
    // vim expects cdd consistently... So we have to deal with that
    // here... (this also occurs scrolling the same way in the other
    // direction).

    if (source.Bottom < dest.Y)
    {
	COORD coord;
	int   i;

	coord.X = source.Left;
	for (i = clip.Top; i < dest.Y; ++i)
	{
	    coord.Y = i;
	    clear_chars(coord, source.Right - source.Left + 1);
	}
    }
}


/*
 * Delete `cLines' lines at the current cursor position
 */
    static void
delete_lines(unsigned cLines)
{
    SMALL_RECT	    source, clip;
    COORD	    dest;
    CHAR_INFO	    fill;
    int		    nb;

    gotoxy(g_srScrollRegion.Left + 1, g_srScrollRegion.Top + 1);

    dest.X = g_srScrollRegion.Left;
    dest.Y = g_coord.Y;

    source.Left   = g_srScrollRegion.Left;
    source.Top	  = g_coord.Y + cLines;
    source.Right  = g_srScrollRegion.Right;
    source.Bottom = g_srScrollRegion.Bottom;

    clip.Left   = g_srScrollRegion.Left;
    clip.Top    = g_coord.Y;
    clip.Right  = g_srScrollRegion.Right;
    clip.Bottom = g_srScrollRegion.Bottom;

    fill.Char.AsciiChar = ' ';
    if (!USE_VTP)
	fill.Attributes = g_attrCurrent;
    else
	fill.Attributes = g_attrDefault;

    set_console_color_rgb();

    ScrollConsoleScreenBuffer(g_hConOut, &source, &clip, dest, &fill);

    // Here we have to deal with a win32 console flake; See insert_lines()
    // above.

    nb = dest.Y + (source.Bottom - source.Top) + 1;

    if (nb < source.Top)
    {
	COORD coord;
	int   i;

	coord.X = source.Left;
	for (i = nb; i < clip.Bottom; ++i)
	{
	    coord.Y = i;
	    clear_chars(coord, source.Right - source.Left + 1);
	}
    }
}


/*
 * Set the cursor position
 */
    static void
gotoxy(
    unsigned x,
    unsigned y)
{
    if (x < 1 || x > (unsigned)Columns || y < 1 || y > (unsigned)Rows)
	return;

    /* external cursor coords are 1-based; internal are 0-based */
    g_coord.X = x - 1;
    g_coord.Y = y - 1;

    if (!USE_VTP)
	SetConsoleCursorPosition(g_hConOut, g_coord);
    else
	vtp_printf("\033[%d;%dH", y, x);
}


/*
 * Set the current text attribute = (foreground | background)
 * See ../doc/os_win32.txt for the numbers.
 */
    static void
textattr(WORD wAttr)
{
    g_attrCurrent = wAttr & 0xff;

    SetConsoleTextAttribute(g_hConOut, wAttr);
}


    static void
textcolor(WORD wAttr)
{
    g_attrCurrent = (g_attrCurrent & 0xf0) + (wAttr & 0x0f);

    if (!USE_VTP)
	SetConsoleTextAttribute(g_hConOut, g_attrCurrent);
    else
	vtp_sgr_bulk(wAttr);
}


    static void
textbackground(WORD wAttr)
{
    g_attrCurrent = (g_attrCurrent & 0x0f) + ((wAttr & 0x0f) << 4);

    if (!USE_VTP)
	SetConsoleTextAttribute(g_hConOut, g_attrCurrent);
    else
	vtp_sgr_bulk(wAttr);
}


/*
 * restore the default text attribute (whatever we started with)
 */
    static void
normvideo(void)
{
    if (!USE_VTP)
	textattr(g_attrDefault);
    else
	vtp_sgr_bulk(0);
}


static WORD g_attrPreStandout = 0;

/*
 * Make the text standout, by brightening it
 */
    static void
standout(void)
{
    g_attrPreStandout = g_attrCurrent;

    textattr((WORD) (g_attrCurrent|FOREGROUND_INTENSITY|BACKGROUND_INTENSITY));
}


/*
 * Turn off standout mode
 */
    static void
standend(void)
{
    if (g_attrPreStandout)
	textattr(g_attrPreStandout);

    g_attrPreStandout = 0;
}


/*
 * Set normal fg/bg color, based on T_ME.  Called when t_me has been set.
 */
    void
mch_set_normal_colors(void)
{
    char_u	*p;
    int		n;

    cterm_normal_fg_color = (g_attrDefault & 0xf) + 1;
    cterm_normal_bg_color = ((g_attrDefault >> 4) & 0xf) + 1;
    if (
#ifdef FEAT_TERMGUICOLORS
	!p_tgc &&
#endif
	T_ME[0] == ESC && T_ME[1] == '|')
    {
	p = T_ME + 2;
	n = getdigits(&p);
	if (*p == 'm' && n > 0)
	{
	    cterm_normal_fg_color = (n & 0xf) + 1;
	    cterm_normal_bg_color = ((n >> 4) & 0xf) + 1;
	}
    }
#ifdef FEAT_TERMGUICOLORS
    cterm_normal_fg_gui_color = INVALCOLOR;
    cterm_normal_bg_gui_color = INVALCOLOR;
#endif
}


/*
 * visual bell: flash the screen
 */
    static void
visual_bell(void)
{
    COORD   coordOrigin = {0, 0};
    WORD    attrFlash = ~g_attrCurrent & 0xff;

    DWORD   dwDummy;
    LPWORD  oldattrs = (LPWORD)alloc(Rows * Columns * sizeof(WORD));

    if (oldattrs == NULL)
	return;
    ReadConsoleOutputAttribute(g_hConOut, oldattrs, Rows * Columns,
			       coordOrigin, &dwDummy);
    FillConsoleOutputAttribute(g_hConOut, attrFlash, Rows * Columns,
			       coordOrigin, &dwDummy);

    Sleep(15);	    /* wait for 15 msec */
    if (!USE_VTP)
	WriteConsoleOutputAttribute(g_hConOut, oldattrs, Rows * Columns,
				coordOrigin, &dwDummy);
    vim_free(oldattrs);
}


/*
 * Make the cursor visible or invisible
 */
    static void
cursor_visible(BOOL fVisible)
{
    s_cursor_visible = fVisible;
#ifdef MCH_CURSOR_SHAPE
    mch_update_cursor();
#endif
}


/*
 * Write "cbToWrite" bytes in `pchBuf' to the screen.
 * Returns the number of bytes actually written (at least one).
 */
    static DWORD
write_chars(
    char_u *pchBuf,
    DWORD  cbToWrite)
{
    COORD coord = g_coord;
    DWORD written;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	static WCHAR	*unicodebuf = NULL;
	static int	unibuflen = 0;
	int		length;
	DWORD		n, cchwritten, cells;

	length = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pchBuf, cbToWrite, 0, 0);
	if (unicodebuf == NULL || length > unibuflen)
	{
	    vim_free(unicodebuf);
	    unicodebuf = (WCHAR *)lalloc(length * sizeof(WCHAR), FALSE);
	    unibuflen = length;
	}
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pchBuf, cbToWrite,
			    unicodebuf, unibuflen);

	cells = mb_string2cells(pchBuf, cbToWrite);

	if (!USE_VTP)
	{
	    FillConsoleOutputAttribute(g_hConOut, g_attrCurrent, cells,
					coord, &written);
	    /* When writing fails or didn't write a single character, pretend one
	     * character was written, otherwise we get stuck. */
	    if (WriteConsoleOutputCharacterW(g_hConOut, unicodebuf, length,
			coord, &cchwritten) == 0
		    || cchwritten == 0 || cchwritten == (DWORD)-1)
		cchwritten = 1;
	}
	else
	{
	    if (WriteConsoleW(g_hConOut, unicodebuf, length, &cchwritten,
		    NULL) == 0 || cchwritten == 0)
		cchwritten = 1;
	}

	if (cchwritten == length)
	{
	    written = cbToWrite;
	    g_coord.X += (SHORT)cells;
	}
	else
	{
	    char_u *p = pchBuf;
	    for (n = 0; n < cchwritten; n++)
		MB_CPTR_ADV(p);
	    written = p - pchBuf;
	    g_coord.X += (SHORT)mb_string2cells(pchBuf, written);
	}
    }
    else
    {
	if (!USE_VTP)
	{
	    FillConsoleOutputAttribute(g_hConOut, g_attrCurrent, cbToWrite,
					coord, &written);
	    /* When writing fails or didn't write a single character, pretend one
	     * character was written, otherwise we get stuck. */
	    if (WriteConsoleOutputCharacter(g_hConOut, (LPCSTR)pchBuf, cbToWrite,
			coord, &written) == 0
		    || written == 0 || written == (DWORD)-1)
		written = 1;
	}
	else
	{
	    if (WriteConsole(g_hConOut, (LPCSTR)pchBuf, cbToWrite, &written,
		    NULL) == 0 || written == 0)
		written = 1;
	}

	g_coord.X += (SHORT) written;
    }

    while (g_coord.X > g_srScrollRegion.Right)
    {
	g_coord.X -= (SHORT) Columns;
	if (g_coord.Y < g_srScrollRegion.Bottom)
	    g_coord.Y++;
    }

    gotoxy(g_coord.X + 1, g_coord.Y + 1);

    return written;
}


/*
 * mch_write(): write the output buffer to the screen, translating ESC
 * sequences into calls to console output routines.
 */
    void
mch_write(
    char_u  *s,
    int	    len)
{
    s[len] = NUL;

    if (!term_console)
    {
	write(1, s, (unsigned)len);
	return;
    }

    /* translate ESC | sequences into faked bios calls */
    while (len--)
    {
	/* optimization: use one single write_chars for runs of text,
	 * rather than once per character  It ain't curses, but it helps. */
	DWORD  prefix = (DWORD)strcspn((char *)s, "\n\r\b\a\033");

	if (p_wd)
	{
	    WaitForChar(p_wd, FALSE);
	    if (prefix != 0)
		prefix = 1;
	}

	if (prefix != 0)
	{
	    DWORD nWritten;

	    nWritten = write_chars(s, prefix);
#ifdef MCH_WRITE_DUMP
	    if (fdDump)
	    {
		fputc('>', fdDump);
		fwrite(s, sizeof(char_u), nWritten, fdDump);
		fputs("<\n", fdDump);
	    }
#endif
	    len -= (nWritten - 1);
	    s += nWritten;
	}
	else if (s[0] == '\n')
	{
	    /* \n, newline: go to the beginning of the next line or scroll */
	    if (g_coord.Y == g_srScrollRegion.Bottom)
	    {
		scroll(1);
		gotoxy(g_srScrollRegion.Left + 1, g_srScrollRegion.Bottom + 1);
	    }
	    else
	    {
		gotoxy(g_srScrollRegion.Left + 1, g_coord.Y + 2);
	    }
#ifdef MCH_WRITE_DUMP
	    if (fdDump)
		fputs("\\n\n", fdDump);
#endif
	    s++;
	}
	else if (s[0] == '\r')
	{
	    /* \r, carriage return: go to beginning of line */
	    gotoxy(g_srScrollRegion.Left+1, g_coord.Y + 1);
#ifdef MCH_WRITE_DUMP
	    if (fdDump)
		fputs("\\r\n", fdDump);
#endif
	    s++;
	}
	else if (s[0] == '\b')
	{
	    /* \b, backspace: move cursor one position left */
	    if (g_coord.X > g_srScrollRegion.Left)
		g_coord.X--;
	    else if (g_coord.Y > g_srScrollRegion.Top)
	    {
		g_coord.X = g_srScrollRegion.Right;
		g_coord.Y--;
	    }
	    gotoxy(g_coord.X + 1, g_coord.Y + 1);
#ifdef MCH_WRITE_DUMP
	    if (fdDump)
		fputs("\\b\n", fdDump);
#endif
	    s++;
	}
	else if (s[0] == '\a')
	{
	    /* \a, bell */
	    MessageBeep(0xFFFFFFFF);
#ifdef MCH_WRITE_DUMP
	    if (fdDump)
		fputs("\\a\n", fdDump);
#endif
	    s++;
	}
	else if (s[0] == ESC && len >= 3-1 && s[1] == '|')
	{
#ifdef MCH_WRITE_DUMP
	    char_u  *old_s = s;
#endif
	    char_u  *p;
	    int	    arg1 = 0, arg2 = 0, argc = 0, args[16];

	    switch (s[2])
	    {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		p = s + 1;
		do
		{
		    ++p;
		    args[argc] = getdigits(&p);
		    argc += (argc < 15) ? 1 : 0;
		    if (p > s + len)
			break;
		} while (*p == ';');

		if (p > s + len)
		    break;

		arg1 = args[0];
		arg2 = args[1];
		if (*p == 'm')
		{
		    if (argc == 1 && args[0] == 0)
			normvideo();
		    else if (argc == 1)
		    {
			if (USE_VTP)
			    textcolor((WORD) arg1);
			else
			    textattr((WORD) arg1);
		    }
		    else if (USE_VTP)
			vtp_sgr_bulks(argc, args);
		}
		else if (argc == 2 && *p == 'H')
		{
		    gotoxy(arg2, arg1);
		}
		else if (argc == 2 && *p == 'r')
		{
		    set_scroll_region(0, arg1 - 1, Columns - 1, arg2 - 1);
		}
		else if (argc == 2 && *p == 'R')
		{
		    set_scroll_region_tb(arg1, arg2);
		}
		else if (argc == 2 && *p == 'V')
		{
		    set_scroll_region_lr(arg1, arg2);
		}
		else if (argc == 1 && *p == 'A')
		{
		    gotoxy(g_coord.X + 1,
			   max(g_srScrollRegion.Top, g_coord.Y - arg1) + 1);
		}
		else if (argc == 1 && *p == 'b')
		{
		    textbackground((WORD) arg1);
		}
		else if (argc == 1 && *p == 'C')
		{
		    gotoxy(min(g_srScrollRegion.Right, g_coord.X + arg1) + 1,
			   g_coord.Y + 1);
		}
		else if (argc == 1 && *p == 'f')
		{
		    textcolor((WORD) arg1);
		}
		else if (argc == 1 && *p == 'H')
		{
		    gotoxy(1, arg1);
		}
		else if (argc == 1 && *p == 'L')
		{
		    insert_lines(arg1);
		}
		else if (argc == 1 && *p == 'M')
		{
		    delete_lines(arg1);
		}

		len -= (int)(p - s);
		s = p + 1;
		break;

	    case 'A':
		gotoxy(g_coord.X + 1,
		       max(g_srScrollRegion.Top, g_coord.Y - 1) + 1);
		goto got3;

	    case 'B':
		visual_bell();
		goto got3;

	    case 'C':
		gotoxy(min(g_srScrollRegion.Right, g_coord.X + 1) + 1,
		       g_coord.Y + 1);
		goto got3;

	    case 'E':
		termcap_mode_end();
		goto got3;

	    case 'F':
		standout();
		goto got3;

	    case 'f':
		standend();
		goto got3;

	    case 'H':
		gotoxy(1, 1);
		goto got3;

	    case 'j':
		clear_to_end_of_display();
		goto got3;

	    case 'J':
		clear_screen();
		goto got3;

	    case 'K':
		clear_to_end_of_line();
		goto got3;

	    case 'L':
		insert_lines(1);
		goto got3;

	    case 'M':
		delete_lines(1);
		goto got3;

	    case 'S':
		termcap_mode_start();
		goto got3;

	    case 'V':
		cursor_visible(TRUE);
		goto got3;

	    case 'v':
		cursor_visible(FALSE);
		goto got3;

	    got3:
		s += 3;
		len -= 2;
	    }

#ifdef MCH_WRITE_DUMP
	    if (fdDump)
	    {
		fputs("ESC | ", fdDump);
		fwrite(old_s + 2, sizeof(char_u), s - old_s - 2, fdDump);
		fputc('\n', fdDump);
	    }
#endif
	}
	else
	{
	    /* Write a single character */
	    DWORD nWritten;

	    nWritten = write_chars(s, 1);
#ifdef MCH_WRITE_DUMP
	    if (fdDump)
	    {
		fputc('>', fdDump);
		fwrite(s, sizeof(char_u), nWritten, fdDump);
		fputs("<\n", fdDump);
	    }
#endif

	    len -= (nWritten - 1);
	    s += nWritten;
	}
    }

#ifdef MCH_WRITE_DUMP
    if (fdDump)
	fflush(fdDump);
#endif
}

#endif /* FEAT_GUI_MSWIN */


/*
 * Delay for "msec" milliseconds.
 */
    void
mch_delay(
    long    msec,
    int	    ignoreinput UNUSED)
{
#ifdef FEAT_GUI_MSWIN
    Sleep((int)msec);	    /* never wait for input */
#else /* Console */
    if (ignoreinput)
# ifdef FEAT_MZSCHEME
	if (mzthreads_allowed() && p_mzq > 0 && msec > p_mzq)
	{
	    int towait = p_mzq;

	    /* if msec is large enough, wait by portions in p_mzq */
	    while (msec > 0)
	    {
		mzvim_check_threads();
		if (msec < towait)
		    towait = msec;
		Sleep(towait);
		msec -= towait;
	    }
	}
	else
# endif
	    Sleep((int)msec);
    else
	WaitForChar(msec, FALSE);
#endif
}


/*
 * This version of remove is not scared by a readonly (backup) file.
 * This can also remove a symbolic link like Unix.
 * Return 0 for success, -1 for failure.
 */
    int
mch_remove(char_u *name)
{
    WCHAR	*wn = NULL;
    int		n;

    /*
     * On Windows, deleting a directory's symbolic link is done by
     * RemoveDirectory(): mch_rmdir.  It seems unnatural, but it is fact.
     */
    if (mch_isdir(name) && mch_is_symbolic_link(name))
	return mch_rmdir(name);

    win32_setattrs(name, FILE_ATTRIBUTE_NORMAL);

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	wn = enc_to_utf16(name, NULL);
	if (wn != NULL)
	{
	    n = DeleteFileW(wn) ? 0 : -1;
	    vim_free(wn);
	    return n;
	}
    }
    return DeleteFile((LPCSTR)name) ? 0 : -1;
}


/*
 * Check for an "interrupt signal": CTRL-break or CTRL-C.
 */
    void
mch_breakcheck(int force)
{
#ifndef FEAT_GUI_MSWIN	    /* never used */
    if (g_fCtrlCPressed || g_fCBrkPressed)
    {
	ctrl_break_was_pressed = g_fCBrkPressed;
	g_fCtrlCPressed = g_fCBrkPressed = FALSE;
	got_int = TRUE;
    }
#endif
}

/* physical RAM to leave for the OS */
#define WINNT_RESERVE_BYTES     (256*1024*1024)

/*
 * How much main memory in KiB that can be used by VIM.
 */
    long_u
mch_total_mem(int special UNUSED)
{
    MEMORYSTATUSEX  ms;

    PlatformId();
    /* Need to use GlobalMemoryStatusEx() when there is more memory than
     * what fits in 32 bits. But it's not always available. */
    ms.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&ms);
    if (ms.ullAvailVirtual < ms.ullTotalPhys)
    {
	/* Process address space fits in physical RAM, use all of it. */
	return (long_u)(ms.ullAvailVirtual / 1024);
    }
    if (ms.ullTotalPhys <= WINNT_RESERVE_BYTES)
    {
	/* Catch old NT box or perverse hardware setup. */
	return (long_u)((ms.ullTotalPhys / 2) / 1024);
    }
    /* Use physical RAM less reserve for OS + data. */
    return (long_u)((ms.ullTotalPhys - WINNT_RESERVE_BYTES) / 1024);
}

/*
 * Same code as below, but with wide functions and no comments.
 * Return 0 for success, non-zero for failure.
 */
    int
mch_wrename(WCHAR *wold, WCHAR *wnew)
{
    WCHAR	*p;
    int		i;
    WCHAR	szTempFile[_MAX_PATH + 1];
    WCHAR	szNewPath[_MAX_PATH + 1];
    HANDLE	hf;

    p = wold;
    for (i = 0; wold[i] != NUL; ++i)
	if ((wold[i] == '/' || wold[i] == '\\' || wold[i] == ':')
		&& wold[i + 1] != 0)
	    p = wold + i + 1;
    if ((int)(wold + i - p) < 8 || p[6] != '~')
	return (MoveFileW(wold, wnew) == 0);

    if (GetFullPathNameW(wnew, _MAX_PATH, szNewPath, &p) == 0 || p == NULL)
	return -1;
    *p = NUL;

    if (GetTempFileNameW(szNewPath, L"VIM", 0, szTempFile) == 0)
	return -2;

    if (!DeleteFileW(szTempFile))
	return -3;

    if (!MoveFileW(wold, szTempFile))
	return -4;

    if ((hf = CreateFileW(wold, GENERIC_WRITE, 0, NULL, CREATE_NEW,
		    FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	return -5;
    if (!CloseHandle(hf))
	return -6;

    if (!MoveFileW(szTempFile, wnew))
    {
	(void)MoveFileW(szTempFile, wold);
	return -7;
    }

    DeleteFileW(szTempFile);

    if (!DeleteFileW(wold))
	return -8;

    return 0;
}


/*
 * mch_rename() works around a bug in rename (aka MoveFile) in
 * Windows 95: rename("foo.bar", "foo.bar~") will generate a
 * file whose short file name is "FOO.BAR" (its long file name will
 * be correct: "foo.bar~").  Because a file can be accessed by
 * either its SFN or its LFN, "foo.bar" has effectively been
 * renamed to "foo.bar", which is not at all what was wanted.  This
 * seems to happen only when renaming files with three-character
 * extensions by appending a suffix that does not include ".".
 * Windows NT gets it right, however, with an SFN of "FOO~1.BAR".
 *
 * There is another problem, which isn't really a bug but isn't right either:
 * When renaming "abcdef~1.txt" to "abcdef~1.txt~", the short name can be
 * "abcdef~1.txt" again.  This has been reported on Windows NT 4.0 with
 * service pack 6.  Doesn't seem to happen on Windows 98.
 *
 * Like rename(), returns 0 upon success, non-zero upon failure.
 * Should probably set errno appropriately when errors occur.
 */
    int
mch_rename(
    const char	*pszOldFile,
    const char	*pszNewFile)
{
    char	szTempFile[_MAX_PATH+1];
    char	szNewPath[_MAX_PATH+1];
    char	*pszFilePart;
    HANDLE	hf;
    WCHAR	*wold = NULL;
    WCHAR	*wnew = NULL;
    int		retval = -1;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	wold = enc_to_utf16((char_u *)pszOldFile, NULL);
	wnew = enc_to_utf16((char_u *)pszNewFile, NULL);
	if (wold != NULL && wnew != NULL)
	    retval = mch_wrename(wold, wnew);
	vim_free(wold);
	vim_free(wnew);
	return retval;
    }

    /*
     * No need to play tricks unless the file name contains a "~" as the
     * seventh character.
     */
    pszFilePart = (char *)gettail((char_u *)pszOldFile);
    if (STRLEN(pszFilePart) < 8 || pszFilePart[6] != '~')
	return rename(pszOldFile, pszNewFile);

    /* Get base path of new file name.  Undocumented feature: If pszNewFile is
     * a directory, no error is returned and pszFilePart will be NULL. */
    if (GetFullPathName(pszNewFile, _MAX_PATH, szNewPath, &pszFilePart) == 0
	    || pszFilePart == NULL)
	return -1;
    *pszFilePart = NUL;

    /* Get (and create) a unique temporary file name in directory of new file */
    if (GetTempFileName(szNewPath, "VIM", 0, szTempFile) == 0)
	return -2;

    /* blow the temp file away */
    if (!DeleteFile(szTempFile))
	return -3;

    /* rename old file to the temp file */
    if (!MoveFile(pszOldFile, szTempFile))
	return -4;

    /* now create an empty file called pszOldFile; this prevents the operating
     * system using pszOldFile as an alias (SFN) if we're renaming within the
     * same directory.  For example, we're editing a file called
     * filename.asc.txt by its SFN, filena~1.txt.  If we rename filena~1.txt
     * to filena~1.txt~ (i.e., we're making a backup while writing it), the
     * SFN for filena~1.txt~ will be filena~1.txt, by default, which will
     * cause all sorts of problems later in buf_write().  So, we create an
     * empty file called filena~1.txt and the system will have to find some
     * other SFN for filena~1.txt~, such as filena~2.txt
     */
    if ((hf = CreateFile(pszOldFile, GENERIC_WRITE, 0, NULL, CREATE_NEW,
		    FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	return -5;
    if (!CloseHandle(hf))
	return -6;

    /* rename the temp file to the new file */
    if (!MoveFile(szTempFile, pszNewFile))
    {
	/* Renaming failed.  Rename the file back to its old name, so that it
	 * looks like nothing happened. */
	(void)MoveFile(szTempFile, pszOldFile);

	return -7;
    }

    /* Seems to be left around on Novell filesystems */
    DeleteFile(szTempFile);

    /* finally, remove the empty old file */
    if (!DeleteFile(pszOldFile))
	return -8;

    return 0;	/* success */
}

/*
 * Get the default shell for the current hardware platform
 */
    char *
default_shell(void)
{
    PlatformId();

    return "cmd.exe";
}

/*
 * mch_access() extends access() to do more detailed check on network drives.
 * Returns 0 if file "n" has access rights according to "p", -1 otherwise.
 */
    int
mch_access(char *n, int p)
{
    HANDLE	hFile;
    int		retval = -1;	    /* default: fail */
    WCHAR	*wn = NULL;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
	wn = enc_to_utf16((char_u *)n, NULL);

    if (mch_isdir((char_u *)n))
    {
	char TempName[_MAX_PATH + 16] = "";
	WCHAR TempNameW[_MAX_PATH + 16] = L"";

	if (p & R_OK)
	{
	    /* Read check is performed by seeing if we can do a find file on
	     * the directory for any file. */
	    if (wn != NULL)
	    {
		int		    i;
		WIN32_FIND_DATAW    d;

		for (i = 0; i < _MAX_PATH && wn[i] != 0; ++i)
		    TempNameW[i] = wn[i];
		if (TempNameW[i - 1] != '\\' && TempNameW[i - 1] != '/')
		    TempNameW[i++] = '\\';
		TempNameW[i++] = '*';
		TempNameW[i++] = 0;

		hFile = FindFirstFileW(TempNameW, &d);
		if (hFile == INVALID_HANDLE_VALUE)
		    goto getout;
		else
		    (void)FindClose(hFile);
	    }
	    else
	    {
		char		    *pch;
		WIN32_FIND_DATA	    d;

		vim_strncpy((char_u *)TempName, (char_u *)n, _MAX_PATH);
		pch = TempName + STRLEN(TempName) - 1;
		if (*pch != '\\' && *pch != '/')
		    *++pch = '\\';
		*++pch = '*';
		*++pch = NUL;

		hFile = FindFirstFile(TempName, &d);
		if (hFile == INVALID_HANDLE_VALUE)
		    goto getout;
		(void)FindClose(hFile);
	    }
	}

	if (p & W_OK)
	{
	    /* Trying to create a temporary file in the directory should catch
	     * directories on read-only network shares.  However, in
	     * directories whose ACL allows writes but denies deletes will end
	     * up keeping the temporary file :-(. */
	    if (wn != NULL)
	    {
		if (!GetTempFileNameW(wn, L"VIM", 0, TempNameW))
		    goto getout;
		else
		    DeleteFileW(TempNameW);
	    }
	    else
	    {
		if (!GetTempFileName(n, "VIM", 0, TempName))
		    goto getout;
		mch_remove((char_u *)TempName);
	    }
	}
    }
    else
    {
	// Don't consider a file read-only if another process has opened it.
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;

	/* Trying to open the file for the required access does ACL, read-only
	 * network share, and file attribute checks.  */
	DWORD access_mode = ((p & W_OK) ? GENERIC_WRITE : 0)
					     | ((p & R_OK) ? GENERIC_READ : 0);

	if (wn != NULL)
	    hFile = CreateFileW(wn, access_mode, share_mode,
						 NULL, OPEN_EXISTING, 0, NULL);
	else
	    hFile = CreateFile(n, access_mode, share_mode,
						 NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	    goto getout;
	CloseHandle(hFile);
    }

    retval = 0;	    /* success */
getout:
    vim_free(wn);
    return retval;
}

/*
 * Version of open() that may use UTF-16 file name.
 */
    int
mch_open(const char *name, int flags, int mode)
{
    /* _wopen() does not work with Borland C 5.5: creates a read-only file. */
#ifndef __BORLANDC__
    WCHAR	*wn;
    int		f;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	wn = enc_to_utf16((char_u *)name, NULL);
	if (wn != NULL)
	{
	    f = _wopen(wn, flags, mode);
	    vim_free(wn);
	    return f;
	}
    }
#endif

    /* open() can open a file which name is longer than _MAX_PATH bytes
     * and shorter than _MAX_PATH characters successfully, but sometimes it
     * causes unexpected error in another part. We make it an error explicitly
     * here. */
    if (strlen(name) >= _MAX_PATH)
	return -1;

    return open(name, flags, mode);
}

/*
 * Version of fopen() that may use UTF-16 file name.
 */
    FILE *
mch_fopen(const char *name, const char *mode)
{
    WCHAR	*wn, *wm;
    FILE	*f = NULL;

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
#if defined(DEBUG) && _MSC_VER >= 1400
	/* Work around an annoying assertion in the Microsoft debug CRT
	 * when mode's text/binary setting doesn't match _get_fmode(). */
	char newMode = mode[strlen(mode) - 1];
	int oldMode = 0;

	_get_fmode(&oldMode);
	if (newMode == 't')
	    _set_fmode(_O_TEXT);
	else if (newMode == 'b')
	    _set_fmode(_O_BINARY);
#endif
	wn = enc_to_utf16((char_u *)name, NULL);
	wm = enc_to_utf16((char_u *)mode, NULL);
	if (wn != NULL && wm != NULL)
	    f = _wfopen(wn, wm);
	vim_free(wn);
	vim_free(wm);

#if defined(DEBUG) && _MSC_VER >= 1400
	_set_fmode(oldMode);
#endif
	return f;
    }

    /* fopen() can open a file which name is longer than _MAX_PATH bytes
     * and shorter than _MAX_PATH characters successfully, but sometimes it
     * causes unexpected error in another part. We make it an error explicitly
     * here. */
    if (strlen(name) >= _MAX_PATH)
	return NULL;

    return fopen(name, mode);
}

/*
 * SUB STREAM (aka info stream) handling:
 *
 * NTFS can have sub streams for each file.  Normal contents of file is
 * stored in the main stream, and extra contents (author information and
 * title and so on) can be stored in sub stream.  After Windows 2000, user
 * can access and store those informations in sub streams via explorer's
 * property menuitem in right click menu.  Those informations in sub streams
 * were lost when copying only the main stream.  So we have to copy sub
 * streams.
 *
 * Incomplete explanation:
 *	http://msdn.microsoft.com/library/en-us/dnw2k/html/ntfs5.asp
 * More useful info and an example:
 *	http://www.sysinternals.com/ntw2k/source/misc.shtml#streams
 */

/*
 * Copy info stream data "substream".  Read from the file with BackupRead(sh)
 * and write to stream "substream" of file "to".
 * Errors are ignored.
 */
    static void
copy_substream(HANDLE sh, void *context, WCHAR *to, WCHAR *substream, long len)
{
    HANDLE  hTo;
    WCHAR   *to_name;

    to_name = malloc((wcslen(to) + wcslen(substream) + 1) * sizeof(WCHAR));
    wcscpy(to_name, to);
    wcscat(to_name, substream);

    hTo = CreateFileW(to_name, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTo != INVALID_HANDLE_VALUE)
    {
	long	done;
	DWORD	todo;
	DWORD	readcnt, written;
	char	buf[4096];

	/* Copy block of bytes at a time.  Abort when something goes wrong. */
	for (done = 0; done < len; done += written)
	{
	    /* (size_t) cast for Borland C 5.5 */
	    todo = (DWORD)((size_t)(len - done) > sizeof(buf) ? sizeof(buf)
						       : (size_t)(len - done));
	    if (!BackupRead(sh, (LPBYTE)buf, todo, &readcnt,
						       FALSE, FALSE, context)
		    || readcnt != todo
		    || !WriteFile(hTo, buf, todo, &written, NULL)
		    || written != todo)
		break;
	}
	CloseHandle(hTo);
    }

    free(to_name);
}

/*
 * Copy info streams from file "from" to file "to".
 */
    static void
copy_infostreams(char_u *from, char_u *to)
{
    WCHAR		*fromw;
    WCHAR		*tow;
    HANDLE		sh;
    WIN32_STREAM_ID	sid;
    int			headersize;
    WCHAR		streamname[_MAX_PATH];
    DWORD		readcount;
    void		*context = NULL;
    DWORD		lo, hi;
    int			len;

    /* Convert the file names to wide characters. */
    fromw = enc_to_utf16(from, NULL);
    tow = enc_to_utf16(to, NULL);
    if (fromw != NULL && tow != NULL)
    {
	/* Open the file for reading. */
	sh = CreateFileW(fromw, GENERIC_READ, FILE_SHARE_READ, NULL,
			     OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (sh != INVALID_HANDLE_VALUE)
	{
	    /* Use BackupRead() to find the info streams.  Repeat until we
	     * have done them all.*/
	    for (;;)
	    {
		/* Get the header to find the length of the stream name.  If
		 * the "readcount" is zero we have done all info streams. */
		ZeroMemory(&sid, sizeof(WIN32_STREAM_ID));
		headersize = (int)((char *)&sid.cStreamName - (char *)&sid.dwStreamId);
		if (!BackupRead(sh, (LPBYTE)&sid, headersize,
					   &readcount, FALSE, FALSE, &context)
			|| readcount == 0)
		    break;

		/* We only deal with streams that have a name.  The normal
		 * file data appears to be without a name, even though docs
		 * suggest it is called "::$DATA". */
		if (sid.dwStreamNameSize > 0)
		{
		    /* Read the stream name. */
		    if (!BackupRead(sh, (LPBYTE)streamname,
							 sid.dwStreamNameSize,
					  &readcount, FALSE, FALSE, &context))
			break;

		    /* Copy an info stream with a name ":anything:$DATA".
		     * Skip "::$DATA", it has no stream name (examples suggest
		     * it might be used for the normal file contents).
		     * Note that BackupRead() counts bytes, but the name is in
		     * wide characters. */
		    len = readcount / sizeof(WCHAR);
		    streamname[len] = 0;
		    if (len > 7 && wcsicmp(streamname + len - 6,
							      L":$DATA") == 0)
		    {
			streamname[len - 6] = 0;
			copy_substream(sh, &context, tow, streamname,
						    (long)sid.Size.u.LowPart);
		    }
		}

		/* Advance to the next stream.  We might try seeking too far,
		 * but BackupSeek() doesn't skip over stream borders, thus
		 * that's OK. */
		(void)BackupSeek(sh, sid.Size.u.LowPart, sid.Size.u.HighPart,
							  &lo, &hi, &context);
	    }

	    /* Clear the context. */
	    (void)BackupRead(sh, NULL, 0, &readcount, TRUE, FALSE, &context);

	    CloseHandle(sh);
	}
    }
    vim_free(fromw);
    vim_free(tow);
}

/*
 * Copy file attributes from file "from" to file "to".
 * For Windows NT and later we copy info streams.
 * Always returns zero, errors are ignored.
 */
    int
mch_copy_file_attribute(char_u *from, char_u *to)
{
    /* File streams only work on Windows NT and later. */
    PlatformId();
    copy_infostreams(from, to);
    return 0;
}

#if defined(MYRESETSTKOFLW) || defined(PROTO)
/*
 * Recreate a destroyed stack guard page in win32.
 * Written by Benjamin Peterson.
 */

/* These magic numbers are from the MS header files */
# define MIN_STACK_WINNT 2

/*
 * This function does the same thing as _resetstkoflw(), which is only
 * available in DevStudio .net and later.
 * Returns 0 for failure, 1 for success.
 */
    int
myresetstkoflw(void)
{
    BYTE	*pStackPtr;
    BYTE	*pGuardPage;
    BYTE	*pStackBase;
    BYTE	*pLowestPossiblePage;
    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_INFO si;
    DWORD	nPageSize;
    DWORD	dummy;

    PlatformId();

    /* We need to know the system page size. */
    GetSystemInfo(&si);
    nPageSize = si.dwPageSize;

    /* ...and the current stack pointer */
    pStackPtr = (BYTE*)_alloca(1);

    /* ...and the base of the stack. */
    if (VirtualQuery(pStackPtr, &mbi, sizeof mbi) == 0)
	return 0;
    pStackBase = (BYTE*)mbi.AllocationBase;

    /* ...and the page thats min_stack_req pages away from stack base; this is
     * the lowest page we could use. */
    pLowestPossiblePage = pStackBase + MIN_STACK_WINNT * nPageSize;

    {
	/* We want the first committed page in the stack Start at the stack
	 * base and move forward through memory until we find a committed block.
	 */
	BYTE *pBlock = pStackBase;

	for (;;)
	{
	    if (VirtualQuery(pBlock, &mbi, sizeof mbi) == 0)
		return 0;

	    pBlock += mbi.RegionSize;

	    if (mbi.State & MEM_COMMIT)
		break;
	}

	/* mbi now describes the first committed block in the stack. */
	if (mbi.Protect & PAGE_GUARD)
	    return 1;

	/* decide where the guard page should start */
	if ((long_u)(mbi.BaseAddress) < (long_u)pLowestPossiblePage)
	    pGuardPage = pLowestPossiblePage;
	else
	    pGuardPage = (BYTE*)mbi.BaseAddress;

	/* allocate the guard page */
	if (!VirtualAlloc(pGuardPage, nPageSize, MEM_COMMIT, PAGE_READWRITE))
	    return 0;

	/* apply the guard attribute to the page */
	if (!VirtualProtect(pGuardPage, nPageSize, PAGE_READWRITE | PAGE_GUARD,
								      &dummy))
	    return 0;
    }

    return 1;
}
#endif


/*
 * The command line arguments in UCS2
 */
static int	nArgsW = 0;
static LPWSTR	*ArglistW = NULL;
static int	global_argc = 0;
static char	**global_argv;

static int	used_file_argc = 0;	/* last argument in global_argv[] used
					   for the argument list. */
static int	*used_file_indexes = NULL; /* indexes in global_argv[] for
					      command line arguments added to
					      the argument list */
static int	used_file_count = 0;	/* nr of entries in used_file_indexes */
static int	used_file_literal = FALSE;  /* take file names literally */
static int	used_file_full_path = FALSE;  /* file name was full path */
static int	used_file_diff_mode = FALSE;  /* file name was with diff mode */
static int	used_alist_count = 0;


/*
 * Get the command line arguments.  Unicode version.
 * Returns argc.  Zero when something fails.
 */
    int
get_cmd_argsW(char ***argvp)
{
    char	**argv = NULL;
    int		argc = 0;
    int		i;

    free_cmd_argsW();
    ArglistW = CommandLineToArgvW(GetCommandLineW(), &nArgsW);
    if (ArglistW != NULL)
    {
	argv = malloc((nArgsW + 1) * sizeof(char *));
	if (argv != NULL)
	{
	    argc = nArgsW;
	    argv[argc] = NULL;
	    for (i = 0; i < argc; ++i)
	    {
		int	len;

		/* Convert each Unicode argument to the current codepage. */
		WideCharToMultiByte_alloc(GetACP(), 0,
				ArglistW[i], (int)wcslen(ArglistW[i]) + 1,
				(LPSTR *)&argv[i], &len, 0, 0);
		if (argv[i] == NULL)
		{
		    /* Out of memory, clear everything. */
		    while (i > 0)
			free(argv[--i]);
		    free(argv);
		    argv = NULL;
		    argc = 0;
		}
	    }
	}
    }

    global_argc = argc;
    global_argv = argv;
    if (argc > 0)
    {
	if (used_file_indexes != NULL)
	    free(used_file_indexes);
	used_file_indexes = malloc(argc * sizeof(int));
    }

    if (argvp != NULL)
	*argvp = argv;
    return argc;
}

    void
free_cmd_argsW(void)
{
    if (ArglistW != NULL)
    {
	GlobalFree(ArglistW);
	ArglistW = NULL;
    }
}

/*
 * Remember "name" is an argument that was added to the argument list.
 * This avoids that we have to re-parse the argument list when fix_arg_enc()
 * is called.
 */
    void
used_file_arg(char *name, int literal, int full_path, int diff_mode)
{
    int		i;

    if (used_file_indexes == NULL)
	return;
    for (i = used_file_argc + 1; i < global_argc; ++i)
	if (STRCMP(global_argv[i], name) == 0)
	{
	    used_file_argc = i;
	    used_file_indexes[used_file_count++] = i;
	    break;
	}
    used_file_literal = literal;
    used_file_full_path = full_path;
    used_file_diff_mode = diff_mode;
}

/*
 * Remember the length of the argument list as it was.  If it changes then we
 * leave it alone when 'encoding' is set.
 */
    void
set_alist_count(void)
{
    used_alist_count = GARGCOUNT;
}

/*
 * Fix the encoding of the command line arguments.  Invoked when 'encoding'
 * has been changed while starting up.  Use the UCS-2 command line arguments
 * and convert them to 'encoding'.
 */
    void
fix_arg_enc(void)
{
    int		i;
    int		idx;
    char_u	*str;
    int		*fnum_list;

    /* Safety checks:
     * - if argument count differs between the wide and non-wide argument
     *   list, something must be wrong.
     * - the file name arguments must have been located.
     * - the length of the argument list wasn't changed by the user.
     */
    if (global_argc != nArgsW
	    || ArglistW == NULL
	    || used_file_indexes == NULL
	    || used_file_count == 0
	    || used_alist_count != GARGCOUNT)
	return;

    /* Remember the buffer numbers for the arguments. */
    fnum_list = (int *)alloc((int)sizeof(int) * GARGCOUNT);
    if (fnum_list == NULL)
	return;		/* out of memory */
    for (i = 0; i < GARGCOUNT; ++i)
	fnum_list[i] = GARGLIST[i].ae_fnum;

    /* Clear the argument list.  Make room for the new arguments. */
    alist_clear(&global_alist);
    if (ga_grow(&global_alist.al_ga, used_file_count) == FAIL)
	return;		/* out of memory */

    for (i = 0; i < used_file_count; ++i)
    {
	idx = used_file_indexes[i];
	str = utf16_to_enc(ArglistW[idx], NULL);
	if (str != NULL)
	{
	    int literal = used_file_literal;

# ifdef FEAT_DIFF
	    /* When using diff mode may need to concatenate file name to
	     * directory name.  Just like it's done in main(). */
	    if (used_file_diff_mode && mch_isdir(str) && GARGCOUNT > 0
				      && !mch_isdir(alist_name(&GARGLIST[0])))
	    {
		char_u	    *r;

		r = concat_fnames(str, gettail(alist_name(&GARGLIST[0])), TRUE);
		if (r != NULL)
		{
		    vim_free(str);
		    str = r;
		}
	    }
# endif
	    /* Re-use the old buffer by renaming it.  When not using literal
	     * names it's done by alist_expand() below. */
	    if (used_file_literal)
		buf_set_name(fnum_list[i], str);

	    /* Check backtick literal. backtick literal is already expanded in
	     * main.c, so this part add str as literal. */
	    if (literal == FALSE)
	    {
		size_t len = STRLEN(str);

		if (len > 2 && *str == '`' && *(str + len - 1) == '`')
		    literal = TRUE;
	    }
	    alist_add(&global_alist, str, literal ? 2 : 0);
	}
    }

    if (!used_file_literal)
    {
	/* Now expand wildcards in the arguments. */
	/* Temporarily add '(' and ')' to 'isfname'.  These are valid
	 * filename characters but are excluded from 'isfname' to make
	 * "gf" work on a file name in parenthesis (e.g.: see vim.h).
	 * Also, unset wildignore to not be influenced by this option.
	 * The arguments specified in command-line should be kept even if
	 * encoding options were changed. */
	do_cmdline_cmd((char_u *)":let SaVe_ISF = &isf|set isf+=(,)");
	do_cmdline_cmd((char_u *)":let SaVe_WIG = &wig|set wig=");
	alist_expand(fnum_list, used_alist_count);
	do_cmdline_cmd((char_u *)":let &isf = SaVe_ISF|unlet SaVe_ISF");
	do_cmdline_cmd((char_u *)":let &wig = SaVe_WIG|unlet SaVe_WIG");
    }

    /* If wildcard expansion failed, we are editing the first file of the
     * arglist and there is no file name: Edit the first argument now. */
    if (curwin->w_arg_idx == 0 && curbuf->b_fname == NULL)
    {
	do_cmdline_cmd((char_u *)":rewind");
	if (GARGCOUNT == 1 && used_file_full_path)
	    (void)vim_chdirfile(alist_name(&GARGLIST[0]), "drop");
    }

    set_alist_count();
}

    int
mch_setenv(char *var, char *value, int x)
{
    char_u	*envbuf;

    envbuf = alloc((unsigned)(STRLEN(var) + STRLEN(value) + 2));
    if (envbuf == NULL)
	return -1;

    sprintf((char *)envbuf, "%s=%s", var, value);

    if (enc_codepage >= 0 && (int)GetACP() != enc_codepage)
    {
	WCHAR	    *p = enc_to_utf16(envbuf, NULL);

	vim_free(envbuf);
	if (p == NULL)
	    return -1;
	_wputenv(p);
#ifdef libintl_wputenv
	libintl_wputenv(p);
#endif
	/* Unlike Un*x systems, we can free the string for _wputenv(). */
	vim_free(p);
    }
    else
    {
	_putenv((char *)envbuf);
#ifdef libintl_putenv
	libintl_putenv((char *)envbuf);
#endif
	/* Unlike Un*x systems, we can free the string for _putenv(). */
	vim_free(envbuf);
    }

    return 0;
}

/*
 * Support for 256 colors and 24-bit colors was added in Windows 10
 * version 1703 (Creators update).
 */
#define VTP_FIRST_SUPPORT_BUILD MAKE_VER(10, 0, 15063)

/*
 * Support for pseudo-console (ConPTY) was added in windows 10
 * version 1809 (October 2018 update).  However, that version is unstable.
 */
#define CONPTY_FIRST_SUPPORT_BUILD  MAKE_VER(10, 0, 17763)
#define CONPTY_STABLE_BUILD	    MAKE_VER(10, 0, 32767)  // T.B.D.

    static void
vtp_flag_init(void)
{
    DWORD   ver = get_build_number();
#ifndef FEAT_GUI_MSWIN
    DWORD   mode;
    HANDLE  out;

    out = GetStdHandle(STD_OUTPUT_HANDLE);

    vtp_working = (ver >= VTP_FIRST_SUPPORT_BUILD) ? 1 : 0;
    GetConsoleMode(out, &mode);
    mode |= (ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    if (SetConsoleMode(out, mode) == 0)
	vtp_working = 0;
#endif

    if (ver >= CONPTY_FIRST_SUPPORT_BUILD)
	conpty_working = 1;
    if (ver >= CONPTY_STABLE_BUILD)
	conpty_stable = 1;

}

#if !defined(FEAT_GUI_MSWIN) || defined(PROTO)

    static void
vtp_init(void)
{
    HMODULE hKerneldll;
    DYN_CONSOLE_SCREEN_BUFFER_INFOEX csbi;
# ifdef FEAT_TERMGUICOLORS
    COLORREF fg, bg;
# endif

    /* Use functions supported from Vista */
    hKerneldll = GetModuleHandle("kernel32.dll");
    if (hKerneldll != NULL)
    {
	pGetConsoleScreenBufferInfoEx =
		(PfnGetConsoleScreenBufferInfoEx)GetProcAddress(
		hKerneldll, "GetConsoleScreenBufferInfoEx");
	pSetConsoleScreenBufferInfoEx =
		(PfnSetConsoleScreenBufferInfoEx)GetProcAddress(
		hKerneldll, "SetConsoleScreenBufferInfoEx");
	if (pGetConsoleScreenBufferInfoEx != NULL
		&& pSetConsoleScreenBufferInfoEx != NULL)
	    has_csbiex = TRUE;
    }

    csbi.cbSize = sizeof(csbi);
    if (has_csbiex)
	pGetConsoleScreenBufferInfoEx(g_hConOut, &csbi);
    save_console_bg_rgb = (guicolor_T)csbi.ColorTable[g_color_index_bg];
    save_console_fg_rgb = (guicolor_T)csbi.ColorTable[g_color_index_fg];

# ifdef FEAT_TERMGUICOLORS
    bg = (COLORREF)csbi.ColorTable[g_color_index_bg];
    fg = (COLORREF)csbi.ColorTable[g_color_index_fg];
    bg = (GetRValue(bg) << 16) | (GetGValue(bg) << 8) | GetBValue(bg);
    fg = (GetRValue(fg) << 16) | (GetGValue(fg) << 8) | GetBValue(fg);
    default_console_color_bg = bg;
    default_console_color_fg = fg;
# endif

    set_console_color_rgb();
}

    static void
vtp_exit(void)
{
    reset_console_color_rgb();
}

    static int
vtp_printf(
    char *format,
    ...)
{
    char_u  buf[100];
    va_list list;
    DWORD   result;

    va_start(list, format);
    vim_vsnprintf((char *)buf, 100, (char *)format, list);
    va_end(list);
    WriteConsoleA(g_hConOut, buf, (DWORD)STRLEN(buf), &result, NULL);
    return (int)result;
}

    static void
vtp_sgr_bulk(
    int arg)
{
    int args[1];

    args[0] = arg;
    vtp_sgr_bulks(1, args);
}

    static void
vtp_sgr_bulks(
    int argc,
    int *args
)
{
    /* 2('\033[') + 4('255.') * 16 + NUL */
    char_u buf[2 + (4 * 16) + 1];
    char_u *p;
    int    i;

    p = buf;
    *p++ = '\033';
    *p++ = '[';

    for (i = 0; i < argc; ++i)
    {
	p += vim_snprintf((char *)p, 4, "%d", args[i] & 0xff);
	*p++ = ';';
    }
    p--;
    *p++ = 'm';
    *p = NUL;
    vtp_printf((char *)buf);
}

# ifdef FEAT_TERMGUICOLORS
    static int
ctermtoxterm(
    int cterm)
{
    char_u r, g, b, idx;

    cterm_color2rgb(cterm, &r, &g, &b, &idx);
    return (((int)r << 16) | ((int)g << 8) | (int)b);
}
# endif

    static void
set_console_color_rgb(void)
{
# ifdef FEAT_TERMGUICOLORS
    DYN_CONSOLE_SCREEN_BUFFER_INFOEX csbi;
    int id;
    guicolor_T fg = INVALCOLOR;
    guicolor_T bg = INVALCOLOR;
    int ctermfg;
    int ctermbg;

    if (!USE_VTP)
	return;

    id = syn_name2id((char_u *)"Normal");
    if (id > 0 && p_tgc)
	syn_id2colors(id, &fg, &bg);
    if (fg == INVALCOLOR)
    {
	ctermfg = -1;
	if (id > 0)
	    syn_id2cterm_bg(id, &ctermfg, &ctermbg);
	fg = ctermfg != -1 ? ctermtoxterm(ctermfg) : default_console_color_fg;
	cterm_normal_fg_gui_color = fg;
    }
    if (bg == INVALCOLOR)
    {
	ctermbg = -1;
	if (id > 0)
	    syn_id2cterm_bg(id, &ctermfg, &ctermbg);
	bg = ctermbg != -1 ? ctermtoxterm(ctermbg) : default_console_color_bg;
	cterm_normal_bg_gui_color = bg;
    }
    fg = (GetRValue(fg) << 16) | (GetGValue(fg) << 8) | GetBValue(fg);
    bg = (GetRValue(bg) << 16) | (GetGValue(bg) << 8) | GetBValue(bg);

    csbi.cbSize = sizeof(csbi);
    if (has_csbiex)
	pGetConsoleScreenBufferInfoEx(g_hConOut, &csbi);

    csbi.cbSize = sizeof(csbi);
    csbi.srWindow.Right += 1;
    csbi.srWindow.Bottom += 1;
    csbi.ColorTable[g_color_index_bg] = (COLORREF)bg;
    csbi.ColorTable[g_color_index_fg] = (COLORREF)fg;
    if (has_csbiex)
	pSetConsoleScreenBufferInfoEx(g_hConOut, &csbi);
# endif
}

    static void
reset_console_color_rgb(void)
{
# ifdef FEAT_TERMGUICOLORS
    DYN_CONSOLE_SCREEN_BUFFER_INFOEX csbi;

    csbi.cbSize = sizeof(csbi);
    if (has_csbiex)
	pGetConsoleScreenBufferInfoEx(g_hConOut, &csbi);

    csbi.cbSize = sizeof(csbi);
    csbi.srWindow.Right += 1;
    csbi.srWindow.Bottom += 1;
    csbi.ColorTable[g_color_index_bg] = (COLORREF)save_console_bg_rgb;
    csbi.ColorTable[g_color_index_fg] = (COLORREF)save_console_fg_rgb;
    if (has_csbiex)
	pSetConsoleScreenBufferInfoEx(g_hConOut, &csbi);
# endif
}

    void
control_console_color_rgb(void)
{
    if (USE_VTP)
	set_console_color_rgb();
    else
	reset_console_color_rgb();
}

    int
use_vtp(void)
{
    return USE_VTP;
}

    int
is_term_win32(void)
{
    return T_NAME != NULL && STRCMP(T_NAME, "win32") == 0;
}

    int
has_vtp_working(void)
{
    return vtp_working;
}

#endif

    int
has_conpty_working(void)
{
    return conpty_working;
}

    int
is_conpty_stable(void)
{
    return conpty_stable;
}

#if !defined(FEAT_GUI_MSWIN) || defined(PROTO)
    void
resize_console_buf(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coord;
    SMALL_RECT newsize;

    if (GetConsoleScreenBufferInfo(g_hConOut, &csbi))
    {
	coord.X = SRWIDTH(csbi.srWindow);
	coord.Y = SRHEIGHT(csbi.srWindow);
	SetConsoleScreenBufferSize(g_hConOut, coord);

	newsize.Left = 0;
	newsize.Top = 0;
	newsize.Right = coord.X - 1;
	newsize.Bottom = coord.Y - 1;
	SetConsoleWindowInfo(g_hConOut, TRUE, &newsize);

	SetConsoleScreenBufferSize(g_hConOut, coord);
    }
}
#endif
