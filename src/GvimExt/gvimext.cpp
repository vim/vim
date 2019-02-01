/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	gvimext by Tianmiao Hu
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * gvimext is a DLL which is used for the "Edit with Vim" context menu
 * extension.  It implements a MS defined interface with the Shell.
 *
 * If you have any questions or any suggestions concerning gvimext, please
 * contact Tianmiao Hu: tianmiao@acm.org.
 */

#include "gvimext.h"

#ifdef __BORLANDC__
# include <dir.h>
# ifndef _strnicmp
#  define _strnicmp(a, b, c) strnicmp((a), (b), (c))
# endif
#else
static char *searchpath(char *name);
#endif

// Always get an error while putting the following stuff to the
// gvimext.h file as class protected variables, give up and
// declare them as global stuff
FORMATETC fmte = {CF_HDROP,
		  (DVTARGETDEVICE FAR *)NULL,
		  DVASPECT_CONTENT,
		  -1,
		  TYMED_HGLOBAL
		 };
STGMEDIUM medium;
HRESULT hres = 0;
UINT cbFiles = 0;

/* The buffers size used to be MAX_PATH (260 bytes), but that's not always
 * enough */
#define BUFSIZE 1100

//
// Get the name of the Gvim executable to use, with the path.
// When "runtime" is non-zero, we were called to find the runtime directory.
// Returns the path in name[BUFSIZE].  It's empty when it fails.
//
    static void
getGvimName(char *name, int runtime)
{
    HKEY	keyhandle;
    DWORD	hlen;

    // Get the location of gvim from the registry.
    name[0] = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Vim\\Gvim", 0,
				       KEY_READ, &keyhandle) == ERROR_SUCCESS)
    {
	hlen = BUFSIZE;
	if (RegQueryValueEx(keyhandle, "path", 0, NULL, (BYTE *)name, &hlen)
							     != ERROR_SUCCESS)
	    name[0] = 0;
	else
	    name[hlen] = 0;
	RegCloseKey(keyhandle);
    }

    // Registry didn't work, use the search path.
    if (name[0] == 0)
	strcpy(name, searchpath((char *)"gvim.exe"));

    if (!runtime)
    {
	// Only when looking for the executable, not the runtime dir, we can
	// search for the batch file or a name without a path.
	if (name[0] == 0)
	    strcpy(name, searchpath((char *)"gvim.bat"));
	if (name[0] == 0)
	    strcpy(name, "gvim");	// finds gvim.bat or gvim.exe
    }
}

    static void
getGvimInvocation(char *name, int runtime)
{
    getGvimName(name, runtime);
    // avoid that Vim tries to expand wildcards in the file names
    strcat(name, " --literal");
}

    static void
getGvimInvocationW(wchar_t *nameW)
{
    char *name;

    name = (char *)malloc(BUFSIZE);
    getGvimInvocation(name, 0);
    mbstowcs(nameW, name, BUFSIZE);
    free(name);
}

//
// Get the Vim runtime directory into buf[BUFSIZE].
// The result is empty when it failed.
// When it works, the path ends in a slash or backslash.
//
    static void
getRuntimeDir(char *buf)
{
    int		idx;

    getGvimName(buf, 1);
    if (buf[0] != 0)
    {
	// When no path found, use the search path to expand it.
	if (strchr(buf, '/') == NULL && strchr(buf, '\\') == NULL)
	    strcpy(buf, searchpath(buf));

	// remove "gvim.exe" from the end
	for (idx = (int)strlen(buf) - 1; idx >= 0; idx--)
	    if (buf[idx] == '\\' || buf[idx] == '/')
	    {
		buf[idx + 1] = 0;
		break;
	    }
    }
}

HBITMAP IconToBitmap(HICON hIcon, HBRUSH hBackground, int width, int height)
{
	HDC hDC = GetDC(NULL);
	HDC hMemDC = CreateCompatibleDC(hDC);
	HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, width, height);
	HBITMAP hResultBmp = NULL;
	HGDIOBJ hOrgBMP = SelectObject(hMemDC, hMemBmp);

	DrawIconEx(hMemDC, 0, 0, hIcon, width, height, 0, hBackground, DI_NORMAL);

	hResultBmp = hMemBmp;
	hMemBmp = NULL;

	SelectObject(hMemDC, hOrgBMP);
	DeleteDC(hMemDC);
	ReleaseDC(NULL, hDC);
	DestroyIcon(hIcon);
	return hResultBmp;
}

//
// GETTEXT: translated messages and menu entries
//
#ifndef FEAT_GETTEXT
# define _(x)  x
#else
# define _(x)  (*dyn_libintl_gettext)(x)
# define VIMPACKAGE "vim"
# ifndef GETTEXT_DLL
#  define GETTEXT_DLL "libintl.dll"
#  define GETTEXT_DLL_ALT "libintl-8.dll"
# endif

// Dummy functions
static char *null_libintl_gettext(const char *);
static char *null_libintl_textdomain(const char *);
static char *null_libintl_bindtextdomain(const char *, const char *);
static int dyn_libintl_init(char *dir);
static void dyn_libintl_end(void);

static wchar_t *oldenv = NULL;
static HINSTANCE hLibintlDLL = 0;
static char *(*dyn_libintl_gettext)(const char *) = null_libintl_gettext;
static char *(*dyn_libintl_textdomain)(const char *) = null_libintl_textdomain;
static char *(*dyn_libintl_bindtextdomain)(const char *, const char *)
						= null_libintl_bindtextdomain;

//
// Attempt to load libintl.dll.  If it doesn't work, use dummy functions.
// "dir" is the directory where the libintl.dll might be.
// Return 1 for success, 0 for failure.
//
    static int
dyn_libintl_init(char *dir)
{
    int		i;
    static struct
    {
	char	    *name;
	FARPROC	    *ptr;
    } libintl_entry[] =
    {
	{(char *)"gettext",		(FARPROC*)&dyn_libintl_gettext},
	{(char *)"textdomain",		(FARPROC*)&dyn_libintl_textdomain},
	{(char *)"bindtextdomain",	(FARPROC*)&dyn_libintl_bindtextdomain},
	{NULL, NULL}
    };
    DWORD	len, len2;
    LPWSTR	buf = NULL;
    LPWSTR	buf2 = NULL;

    // No need to initialize twice.
    if (hLibintlDLL)
	return 1;

    // Load gettext library from $VIMRUNTIME\GvimExt{64,32} directory.
    // Add the directory to $PATH temporarily.
    len = GetEnvironmentVariableW(L"PATH", NULL, 0);
    len2 = MAX_PATH + 1 + len;
    buf = (LPWSTR)malloc(len * sizeof(WCHAR));
    buf2 = (LPWSTR)malloc(len2 * sizeof(WCHAR));
    if (buf != NULL && buf2 != NULL)
    {
	GetEnvironmentVariableW(L"PATH", buf, len);
#ifdef _WIN64
	_snwprintf(buf2, len2, L"%S\\GvimExt64;%s", dir, buf);
#else
	_snwprintf(buf2, len2, L"%S\\GvimExt32;%s", dir, buf);
#endif
	SetEnvironmentVariableW(L"PATH", buf2);
	hLibintlDLL = LoadLibrary(GETTEXT_DLL);
#ifdef GETTEXT_DLL_ALT
	if (!hLibintlDLL)
	    hLibintlDLL = LoadLibrary(GETTEXT_DLL_ALT);
#endif
	SetEnvironmentVariableW(L"PATH", buf);
    }
    free(buf);
    free(buf2);
    if (!hLibintlDLL)
	return 0;

    // Get the addresses of the functions we need.
    for (i = 0; libintl_entry[i].name != NULL
					 && libintl_entry[i].ptr != NULL; ++i)
    {
	if ((*libintl_entry[i].ptr = GetProcAddress(hLibintlDLL,
					      libintl_entry[i].name)) == NULL)
	{
	    dyn_libintl_end();
	    return 0;
	}
    }
    return 1;
}

    static void
dyn_libintl_end(void)
{
    if (hLibintlDLL)
	FreeLibrary(hLibintlDLL);
    hLibintlDLL			= NULL;
    dyn_libintl_gettext		= null_libintl_gettext;
    dyn_libintl_textdomain	= null_libintl_textdomain;
    dyn_libintl_bindtextdomain	= null_libintl_bindtextdomain;
}

    static char *
null_libintl_gettext(const char *msgid)
{
    return (char *)msgid;
}

    static char *
null_libintl_bindtextdomain(const char * /* domainname */, const char * /* dirname */)
{
    return NULL;
}

    static char *
null_libintl_textdomain(const char*  /* domainname */)
{
    return NULL;
}

//
// Setup for translating strings.
//
    static void
dyn_gettext_load(void)
{
    char    szBuff[BUFSIZE];
    char    szLang[BUFSIZE];
    DWORD   len;
    HKEY    keyhandle;
    int	    gotlang = 0;

    strcpy(szLang, "LANG=");

    // First try getting the language from the registry, this can be
    // used to overrule the system language.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Vim\\Gvim", 0,
				       KEY_READ, &keyhandle) == ERROR_SUCCESS)
    {
	len = BUFSIZE;
	if (RegQueryValueEx(keyhandle, "lang", 0, NULL, (BYTE*)szBuff, &len)
							     == ERROR_SUCCESS)
	{
	    szBuff[len] = 0;
	    strcat(szLang, szBuff);
	    gotlang = 1;
	}
	RegCloseKey(keyhandle);
    }

    if (!gotlang && getenv("LANG") == NULL)
    {
	// Get the language from the system.
	// Could use LOCALE_SISO639LANGNAME, but it's not in Win95.
	// LOCALE_SABBREVLANGNAME gives us three letters, like "enu", we use
	// only the first two.
	len = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME,
						    (LPTSTR)szBuff, BUFSIZE);
	if (len >= 2 && _strnicmp(szBuff, "en", 2) != 0)
	{
	    // There are a few exceptions (probably more)
	    if (_strnicmp(szBuff, "cht", 3) == 0
					  || _strnicmp(szBuff, "zht", 3) == 0)
		strcpy(szBuff, "zh_TW");
	    else if (_strnicmp(szBuff, "chs", 3) == 0
					  || _strnicmp(szBuff, "zhc", 3) == 0)
		strcpy(szBuff, "zh_CN");
	    else if (_strnicmp(szBuff, "jp", 2) == 0)
		strcpy(szBuff, "ja");
	    else
		szBuff[2] = 0;	// truncate to two-letter code
	    strcat(szLang, szBuff);
	    gotlang = 1;
	}
    }
    if (gotlang)
	putenv(szLang);

    // Try to locate the runtime files.  The path is used to find libintl.dll
    // and the vim.mo files.
    getRuntimeDir(szBuff);
    if (szBuff[0] != 0)
    {
	len = (DWORD)strlen(szBuff);
	if (dyn_libintl_init(szBuff))
	{
	    strcpy(szBuff + len, "lang");

	    (*dyn_libintl_bindtextdomain)(VIMPACKAGE, szBuff);
	    (*dyn_libintl_textdomain)(VIMPACKAGE);
	}
    }
}

    static void
dyn_gettext_free(void)
{
    dyn_libintl_end();
}
#endif // FEAT_GETTEXT

//
// Global variables
//
UINT      g_cRefThisDll = 0;    // Reference count of this DLL.
HINSTANCE g_hmodThisDll = NULL;	// Handle to this DLL itself.


//---------------------------------------------------------------------------
// DllMain
//---------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID  /* lpReserved */)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
	// Extension DLL one-time initialization
	g_hmodThisDll = hInstance;
	break;

    case DLL_PROCESS_DETACH:
	break;
    }

    return 1;   // ok
}

    static void
inc_cRefThisDLL()
{
#ifdef FEAT_GETTEXT
    if (g_cRefThisDll == 0) {
	dyn_gettext_load();
	oldenv = GetEnvironmentStringsW();
    }
#endif
    InterlockedIncrement((LPLONG)&g_cRefThisDll);
}

    static void
dec_cRefThisDLL()
{
#ifdef FEAT_GETTEXT
    if (InterlockedDecrement((LPLONG)&g_cRefThisDll) == 0) {
	dyn_gettext_free();
	if (oldenv != NULL) {
	    FreeEnvironmentStringsW(oldenv);
	    oldenv = NULL;
	}
    }
#else
    InterlockedDecrement((LPLONG)&g_cRefThisDll);
#endif
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    if (IsEqualIID(rclsid, CLSID_ShellExtension))
    {
	CShellExtClassFactory *pcf = new CShellExtClassFactory;

	return pcf->QueryInterface(riid, ppvOut);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

CShellExtClassFactory::CShellExtClassFactory()
{
    m_cRef = 0L;

    inc_cRefThisDLL();
}

CShellExtClassFactory::~CShellExtClassFactory()
{
    dec_cRefThisDLL();
}

STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID riid,
						   LPVOID FAR *ppv)
{
    *ppv = NULL;

    // any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
	*ppv = (LPCLASSFACTORY)this;

	AddRef();

	return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
    if (InterlockedDecrement((LPLONG)&m_cRef))
	return m_cRef;

    delete this;

    return 0L;
}

STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
						      REFIID riid,
						      LPVOID *ppvObj)
{
    *ppvObj = NULL;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
	return CLASS_E_NOAGGREGATION;

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    LPCSHELLEXT pShellExt = new CShellExt();  // create the CShellExt object

    if (NULL == pShellExt)
	return E_OUTOFMEMORY;

    return pShellExt->QueryInterface(riid, ppvObj);
}


STDMETHODIMP CShellExtClassFactory::LockServer(BOOL  /* fLock */)
{
    return NOERROR;
}

// *********************** CShellExt *************************
CShellExt::CShellExt()
{
    m_cRef = 0L;
    m_pDataObj = NULL;

    inc_cRefThisDLL();

    LoadMenuIcon();
}

CShellExt::~CShellExt()
{
    if (m_pDataObj)
	m_pDataObj->Release();

    dec_cRefThisDLL();

    if (m_hVimIconBitmap)
	DeleteObject(m_hVimIconBitmap);
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
	*ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
	*ppv = (LPCONTEXTMENU)this;
    }

    if (*ppv)
    {
	AddRef();

	return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{

    if (InterlockedDecrement((LPLONG)&m_cRef))
	return m_cRef;

    delete this;

    return 0L;
}


//
//  FUNCTION: CShellExt::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
//
//  PURPOSE: Called by the shell when initializing a context menu or property
//	     sheet extension.
//
//  PARAMETERS:
//    pIDFolder - Specifies the parent folder
//    pDataObj  - Specifies the set of items selected in that folder.
//    hRegKey   - Specifies the type of the focused item in the selection.
//
//  RETURN VALUE:
//
//    NOERROR in all cases.
//
//  COMMENTS:   Note that at the time this function is called, we don't know
//		(or care) what type of shell extension is being initialized.
//		It could be a context menu or a property sheet.
//

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST  /* pIDFolder */,
				   LPDATAOBJECT pDataObj,
				   HKEY  /* hRegKey */)
{
    // Initialize can be called more than once
    if (m_pDataObj)
	m_pDataObj->Release();

    // duplicate the object pointer and registry handle

    if (pDataObj)
    {
	m_pDataObj = pDataObj;
	pDataObj->AddRef();
    }

    return NOERROR;
}


//
//  FUNCTION: CShellExt::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//  PURPOSE: Called by the shell just before the context menu is displayed.
//	     This is where you add your specific menu items.
//
//  PARAMETERS:
//    hMenu      - Handle to the context menu
//    indexMenu  - Index of where to begin inserting menu items
//    idCmdFirst - Lowest value for new menu ID's
//    idCmtLast  - Highest value for new menu ID's
//    uFlags     - Specifies the context of the menu event
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
					 UINT indexMenu,
					 UINT idCmdFirst,
					 UINT  /* idCmdLast */,
					 UINT  /* uFlags */)
{
    UINT idCmd = idCmdFirst;

    hres = m_pDataObj->GetData(&fmte, &medium);
    if (medium.hGlobal)
	cbFiles = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, 0, 0);

    // InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

    // Initialize m_cntOfHWnd to 0
    m_cntOfHWnd = 0;

    HKEY keyhandle;
    bool showExisting = true;
    bool showIcons = true;

    // Check whether "Edit with existing Vim" entries are disabled.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Vim\\Gvim", 0,
				       KEY_READ, &keyhandle) == ERROR_SUCCESS)
    {
	if (RegQueryValueEx(keyhandle, "DisableEditWithExisting", 0, NULL,
						 NULL, NULL) == ERROR_SUCCESS)
	    showExisting = false;
	if (RegQueryValueEx(keyhandle, "DisableContextMenuIcons", 0, NULL,
						 NULL, NULL) == ERROR_SUCCESS)
	    showIcons = false;
	RegCloseKey(keyhandle);
    }

    // Retrieve all the vim instances, unless disabled.
    if (showExisting)
	EnumWindows(EnumWindowsProc, (LPARAM)this);

    MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
    mii.fMask = MIIM_STRING | MIIM_ID;
    if (showIcons)
    {
	mii.fMask |= MIIM_BITMAP;
	mii.hbmpItem = m_hVimIconBitmap;
    }

    if (cbFiles > 1)
    {
	mii.wID = idCmd++;
	mii.dwTypeData = _("Edit with &multiple Vims");
	mii.cch = lstrlen(mii.dwTypeData);
	InsertMenuItem(hMenu, indexMenu++, TRUE, &mii);

	mii.wID = idCmd++;
	mii.dwTypeData = _("Edit with single &Vim");
	mii.cch = lstrlen(mii.dwTypeData);
	InsertMenuItem(hMenu, indexMenu++, TRUE, &mii);

	if (cbFiles <= 4)
	{
	    // Can edit up to 4 files in diff mode
	    mii.wID = idCmd++;
	    mii.dwTypeData = _("Diff with Vim");
	    mii.cch = lstrlen(mii.dwTypeData);
	    InsertMenuItem(hMenu, indexMenu++, TRUE, &mii);
	    m_edit_existing_off = 3;
	}
	else
	    m_edit_existing_off = 2;

    }
    else
    {
	mii.wID = idCmd++;
	mii.dwTypeData = _("Edit with &Vim");
	mii.cch = lstrlen(mii.dwTypeData);
	InsertMenuItem(hMenu, indexMenu++, TRUE, &mii);
	m_edit_existing_off = 1;
    }

    HMENU hSubMenu = NULL;
    if (m_cntOfHWnd > 1)
    {
	hSubMenu = CreatePopupMenu();
	mii.fMask |= MIIM_SUBMENU;
	mii.wID = idCmd;
	mii.dwTypeData = _("Edit with existing Vim");
	mii.cch = lstrlen(mii.dwTypeData);
	mii.hSubMenu = hSubMenu;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &mii);
	mii.fMask = mii.fMask & ~MIIM_SUBMENU;
	mii.hSubMenu = NULL;
    }
    // Now display all the vim instances
    for (int i = 0; i < m_cntOfHWnd; i++)
    {
	char title[BUFSIZE];
	char temp[BUFSIZE];
	int index;
	HMENU hmenu;

	// Obtain window title, continue if can not
	if (GetWindowText(m_hWnd[i], title, BUFSIZE - 1) == 0)
	    continue;
	// Truncate the title before the path, keep the file name
	char *pos = strchr(title, '(');
	if (pos != NULL)
	{
	    if (pos > title && pos[-1] == ' ')
		--pos;
	    *pos = 0;
	}
	// Now concatenate
	if (m_cntOfHWnd > 1)
	    temp[0] = '\0';
	else
	{
	    strncpy(temp, _("Edit with existing Vim - "), BUFSIZE - 1);
	    temp[BUFSIZE - 1] = '\0';
	}
	strncat(temp, title, BUFSIZE - 1 - strlen(temp));
	temp[BUFSIZE - 1] = '\0';

	mii.wID = idCmd++;
	mii.dwTypeData = temp;
	mii.cch = lstrlen(mii.dwTypeData);
	if (m_cntOfHWnd > 1)
	{
	    hmenu = hSubMenu;
	    index = i;
	}
	else
	{
	    hmenu = hMenu;
	    index = indexMenu++;
	}
	InsertMenuItem(hmenu, index, TRUE, &mii);
    }
    // InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

    // Must return number of menu items we added.
    return ResultFromShort(idCmd-idCmdFirst);
}

//
//  FUNCTION: CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//	     menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr = E_INVALIDARG;

    // If HIWORD(lpcmi->lpVerb) then we have been called programmatically
    // and lpVerb is a command that should be invoked.  Otherwise, the shell
    // has called us, and LOWORD(lpcmi->lpVerb) is the menu ID the user has
    // selected.  Actually, it's (menu ID - idCmdFirst) from QueryContextMenu().
    if (!HIWORD(lpcmi->lpVerb))
    {
	UINT idCmd = LOWORD(lpcmi->lpVerb);

	if (idCmd >= m_edit_existing_off)
	{
	    // Existing with vim instance
	    hr = PushToWindow(lpcmi->hwnd,
		    lpcmi->lpDirectory,
		    lpcmi->lpVerb,
		    lpcmi->lpParameters,
		    lpcmi->nShow,
		    idCmd - m_edit_existing_off);
	}
	else
	{
	    switch (idCmd)
	    {
		case 0:
		    hr = InvokeGvim(lpcmi->hwnd,
			    lpcmi->lpDirectory,
			    lpcmi->lpVerb,
			    lpcmi->lpParameters,
			    lpcmi->nShow);
		    break;
		case 1:
		    hr = InvokeSingleGvim(lpcmi->hwnd,
			    lpcmi->lpDirectory,
			    lpcmi->lpVerb,
			    lpcmi->lpParameters,
			    lpcmi->nShow,
			    0);
		    break;
		case 2:
		    hr = InvokeSingleGvim(lpcmi->hwnd,
			    lpcmi->lpDirectory,
			    lpcmi->lpVerb,
			    lpcmi->lpParameters,
			    lpcmi->nShow,
			    1);
		    break;
	    }
	}
    }
    return hr;
}

STDMETHODIMP CShellExt::PushToWindow(HWND  /* hParent */,
				   LPCSTR  /* pszWorkingDir */,
				   LPCSTR  /* pszCmd */,
				   LPCSTR  /* pszParam */,
				   int  /* iShowCmd */,
				   int idHWnd)
{
    HWND hWnd = m_hWnd[idHWnd];

    // Show and bring vim instance to foreground
    if (IsIconic(hWnd) != 0)
	ShowWindow(hWnd, SW_RESTORE);
    else
	ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);

    // Post the selected files to the vim instance
    PostMessage(hWnd, WM_DROPFILES, (WPARAM)medium.hGlobal, 0);

    return NOERROR;
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR  /* idCmd */,
					 UINT uFlags,
					 UINT FAR * /* reserved */,
					 LPSTR pszName,
					 UINT cchMax)
{
    if (uFlags == GCS_HELPTEXT && cchMax > 35)
	lstrcpy(pszName, _("Edits the selected file(s) with Vim"));

    return NOERROR;
}

BOOL CALLBACK CShellExt::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    char temp[BUFSIZE];

    // First do a bunch of check
    // No invisible window
    if (!IsWindowVisible(hWnd)) return TRUE;
    // No child window ???
    // if (GetParent(hWnd)) return TRUE;
    // Class name should be Vim, if failed to get class name, return
    if (GetClassName(hWnd, temp, sizeof(temp)) == 0)
	return TRUE;
    // Compare class name to that of vim, if not, return
    if (_strnicmp(temp, "vim", sizeof("vim")) != 0)
	return TRUE;
    // First check if the number of vim instance exceeds MAX_HWND
    CShellExt *cs = (CShellExt*) lParam;
    if (cs->m_cntOfHWnd >= MAX_HWND) return TRUE;
    // Now we get the vim window, put it into some kind of array
    cs->m_hWnd[cs->m_cntOfHWnd] = hWnd;
    cs->m_cntOfHWnd ++;

    return TRUE; // continue enumeration (otherwise this would be false)
}

BOOL CShellExt::LoadMenuIcon()
{
	char vimExeFile[BUFSIZE];
	getGvimName(vimExeFile, 1);
	if (vimExeFile[0] == '\0')
		return FALSE;
	HICON hVimIcon;
	if (ExtractIconEx(vimExeFile, 0, NULL, &hVimIcon, 1) == 0)
		return FALSE;
	m_hVimIconBitmap = IconToBitmap(hVimIcon,
		GetSysColorBrush(COLOR_MENU),
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON));
	return TRUE;
}

#ifndef __BORLANDC__
    static char *
searchpath(char *name)
{
    static char widename[2 * BUFSIZE];
    static char location[2 * BUFSIZE + 2];

    // There appears to be a bug in FindExecutableA() on Windows NT.
    // Use FindExecutableW() instead...
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)name, -1,
	    (LPWSTR)widename, BUFSIZE);
    if (FindExecutableW((LPCWSTR)widename, (LPCWSTR)"",
		(LPWSTR)location) > (HINSTANCE)32)
    {
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)location, -1,
		(LPSTR)widename, 2 * BUFSIZE, NULL, NULL);
	return widename;
    }
    return (char *)"";
}
#endif

STDMETHODIMP CShellExt::InvokeGvim(HWND hParent,
				   LPCSTR  /* pszWorkingDir */,
				   LPCSTR  /* pszCmd */,
				   LPCSTR  /* pszParam */,
				   int  /* iShowCmd */)
{
    wchar_t m_szFileUserClickedOn[BUFSIZE];
    wchar_t cmdStrW[BUFSIZE];
    UINT i;

    for (i = 0; i < cbFiles; i++)
    {
	DragQueryFileW((HDROP)medium.hGlobal,
		i,
		m_szFileUserClickedOn,
		sizeof(m_szFileUserClickedOn));

	getGvimInvocationW(cmdStrW);
	wcscat(cmdStrW, L" \"");

	if ((wcslen(cmdStrW) + wcslen(m_szFileUserClickedOn) + 2) < BUFSIZE)
	{
	    wcscat(cmdStrW, m_szFileUserClickedOn);
	    wcscat(cmdStrW, L"\"");

	    STARTUPINFOW si;
	    PROCESS_INFORMATION pi;

	    ZeroMemory(&si, sizeof(si));
	    si.cb = sizeof(si);

	    // Start the child process.
	    if (!CreateProcessW(NULL,	// No module name (use command line).
			cmdStrW,	// Command line.
			NULL,		// Process handle not inheritable.
			NULL,		// Thread handle not inheritable.
			FALSE,		// Set handle inheritance to FALSE.
			oldenv == NULL ? 0 : CREATE_UNICODE_ENVIRONMENT,
			oldenv,		// Use unmodified environment block.
			NULL,		// Use parent's starting directory.
			&si,		// Pointer to STARTUPINFO structure.
			&pi)		// Pointer to PROCESS_INFORMATION structure.
	       )
	    {
		MessageBox(
		    hParent,
		    _("Error creating process: Check if gvim is in your path!"),
		    _("gvimext.dll error"),
		    MB_OK);
	    }
	    else
	    {
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	    }
	}
	else
	{
	    MessageBox(
		hParent,
		_("Path length too long!"),
		_("gvimext.dll error"),
		MB_OK);
	}
    }

    return NOERROR;
}


STDMETHODIMP CShellExt::InvokeSingleGvim(HWND hParent,
				   LPCSTR  /* pszWorkingDir */,
				   LPCSTR  /* pszCmd */,
				   LPCSTR  /* pszParam */,
				   int  /* iShowCmd */,
				   int useDiff)
{
    wchar_t	m_szFileUserClickedOn[BUFSIZE];
    wchar_t	*cmdStrW;
    size_t	cmdlen;
    size_t	len;
    UINT i;

    cmdlen = BUFSIZE;
    cmdStrW  = (wchar_t *) malloc(cmdlen * sizeof(wchar_t));
    if (cmdStrW == NULL)
	return E_FAIL;
    getGvimInvocationW(cmdStrW);

    if (useDiff)
	wcscat(cmdStrW, L" -d");
    for (i = 0; i < cbFiles; i++)
    {
	DragQueryFileW((HDROP)medium.hGlobal,
		i,
		m_szFileUserClickedOn,
		sizeof(m_szFileUserClickedOn));

	len = wcslen(cmdStrW) + wcslen(m_szFileUserClickedOn) + 4;
	if (len > cmdlen)
	{
	    cmdlen = len + BUFSIZE;
	    wchar_t *cmdStrW_new = (wchar_t *)realloc(cmdStrW, cmdlen * sizeof(wchar_t));
	    if (cmdStrW_new == NULL)
	    {
		free(cmdStrW);
		return E_FAIL;
	    }
	    cmdStrW = cmdStrW_new;
	}
	wcscat(cmdStrW, L" \"");
	wcscat(cmdStrW, m_szFileUserClickedOn);
	wcscat(cmdStrW, L"\"");
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    // Start the child process.
    if (!CreateProcessW(NULL,	// No module name (use command line).
		cmdStrW,	// Command line.
		NULL,		// Process handle not inheritable.
		NULL,		// Thread handle not inheritable.
		FALSE,		// Set handle inheritance to FALSE.
		oldenv == NULL ? 0 : CREATE_UNICODE_ENVIRONMENT,
		oldenv,		// Use unmodified environment block.
		NULL,		// Use parent's starting directory.
		&si,		// Pointer to STARTUPINFO structure.
		&pi)		// Pointer to PROCESS_INFORMATION structure.
       )
    {
	MessageBox(
	    hParent,
	    _("Error creating process: Check if gvim is in your path!"),
	    _("gvimext.dll error"),
	    MB_OK);
    }
    else
    {
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
    }
    free(cmdStrW);

    return NOERROR;
}
