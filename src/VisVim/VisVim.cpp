// VisVim.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <initguid.h>
#include "VisVim.h"
#include "DSAddIn.h"
#include "Commands.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif

CComModule _Module;

BEGIN_OBJECT_MAP (ObjectMap)
OBJECT_ENTRY (CLSID_DSAddIn, CDSAddIn)
END_OBJECT_MAP ()

class CVisVimApp : public CWinApp
{
    public:
	CVisVimApp ();

	//{{AFX_VIRTUAL(CVisVimApp)
    public:
	virtual BOOL InitInstance ();
	virtual int ExitInstance ();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CVisVimApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP ()
};

BEGIN_MESSAGE_MAP (CVisVimApp, CWinApp)
//{{AFX_MSG_MAP(CVisVimApp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP ()

// The one and only CVisVimApp object
CVisVimApp theApp;

CVisVimApp::CVisVimApp ()
{
}

BOOL CVisVimApp::InitInstance ()
{
	_Module.Init (ObjectMap, m_hInstance);
	return CWinApp::InitInstance ();
}

int CVisVimApp::ExitInstance ()
{
	_Module.Term ();
	return CWinApp::ExitInstance ();
}

// Special entry points required for inproc servers
//

STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, LPVOID * ppv)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	return _Module.GetClassObject (rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow (void)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	return (AfxDllCanUnloadNow () == S_OK && _Module.GetLockCount () == 0)
		? S_OK : S_FALSE;
}

// By exporting DllRegisterServer, you can use regsvr32.exe
//
STDAPI DllRegisterServer (void)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HRESULT hRes;

	// Registers object, typelib and all interfaces in typelib
	hRes = _Module.RegisterServer (TRUE);
	if (FAILED (hRes))
		// Hack: When this fails we might be a normal user, while the
		// admin already registered the module.  Returning S_OK then
		// makes it work.  When the module was never registered it
		// will soon fail in another way.
		// old code: return hRes;
		return S_OK;

	_ATL_OBJMAP_ENTRY *pEntry = _Module.m_pObjMap;
	CRegKey key;
	LONG lRes = key.Open (HKEY_CLASSES_ROOT, _T ("CLSID"));

	if (lRes == ERROR_SUCCESS)
	{
		USES_CONVERSION;
		LPOLESTR lpOleStr;

		StringFromCLSID (*pEntry->pclsid, &lpOleStr);
		LPTSTR lpsz = OLE2T (lpOleStr);

		lRes = key.Open (key, lpsz);
		if (lRes == ERROR_SUCCESS)
		{
			CString strDescription;

			strDescription.LoadString (IDS_VISVIM_DESCRIPTION);
			key.SetKeyValue (_T ("Description"), strDescription);
		}
		CoTaskMemFree (lpOleStr);
	}

	if (lRes != ERROR_SUCCESS)
		hRes = HRESULT_FROM_WIN32 (lRes);

	return hRes;

}

// DllUnregisterServer - Removes entries from the system registry
//
STDAPI DllUnregisterServer (void)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());

	HRESULT hRes = S_OK;
	_Module.UnregisterServer ();
	return hRes;
}


// Debugging support

// GetLastErrorDescription is used in the implementation of the VERIFY_OK
//  macro, defined in stdafx.h.

#ifdef _DEBUG

void GetLastErrorDescription (CComBSTR & bstr)
{
	CComPtr < IErrorInfo > pErrorInfo;
	if (GetErrorInfo (0, &pErrorInfo) == S_OK)
		pErrorInfo->GetDescription (&bstr);
}

#endif //_DEBUG
