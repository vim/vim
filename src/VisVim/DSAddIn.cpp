#include "stdafx.h"
#include "VisVim.h"
#include "DSAddIn.h"
#include "Commands.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif

// This is called when the user first loads the add-in, and on start-up
//  of each subsequent Developer Studio session
STDMETHODIMP CDSAddIn::OnConnection (IApplication * pApp, VARIANT_BOOL bFirstTime,
				     long dwCookie, VARIANT_BOOL * OnConnection)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	*OnConnection = VARIANT_FALSE;

	// Store info passed to us
	IApplication *pApplication = NULL;
	HRESULT hr;

	hr = pApp->QueryInterface (IID_IApplication, (void **) &pApplication);
	if (FAILED (hr))
	{
		ReportLastError (hr);
		return E_UNEXPECTED;
	}
	if (pApplication == NULL)
	{
		ReportInternalError ("IApplication::QueryInterface");
		return E_UNEXPECTED;
	}

	m_dwCookie = dwCookie;

	// Create command dispatch, send info back to DevStudio
	CCommandsObj::CreateInstance (&m_pCommands);
	if (! m_pCommands)
	{
		ReportInternalError ("CCommandsObj::CreateInstance");
		return E_UNEXPECTED;
	}
	m_pCommands->AddRef ();

	// The QueryInterface above AddRef'd the Application object.  It will
	// be Release'd in CCommand's destructor.
	m_pCommands->SetApplicationObject (pApplication);

	hr = pApplication->SetAddInInfo ((long) AfxGetInstanceHandle (),
		(LPDISPATCH) m_pCommands, IDR_TOOLBAR_MEDIUM, IDR_TOOLBAR_LARGE,
		m_dwCookie);
	if (FAILED (hr))
	{
		ReportLastError (hr);
		return E_UNEXPECTED;
	}

	// Inform DevStudio of the commands we implement
	if (! AddCommand (pApplication, "VisVimDialog", "VisVimDialogCmd",
			  IDS_CMD_DIALOG, 0, bFirstTime))
		return E_UNEXPECTED;
	if (! AddCommand (pApplication, "VisVimEnable", "VisVimEnableCmd",
			  IDS_CMD_ENABLE, 1, bFirstTime))
		return E_UNEXPECTED;
	if (! AddCommand (pApplication, "VisVimDisable", "VisVimDisableCmd",
			  IDS_CMD_DISABLE, 2, bFirstTime))
		return E_UNEXPECTED;
	if (! AddCommand (pApplication, "VisVimToggle", "VisVimToggleCmd",
			  IDS_CMD_TOGGLE, 3, bFirstTime))
		return E_UNEXPECTED;
	if (! AddCommand (pApplication, "VisVimLoad", "VisVimLoadCmd",
			  IDS_CMD_LOAD, 4, bFirstTime))
		return E_UNEXPECTED;

	*OnConnection = VARIANT_TRUE;
	return S_OK;
}

// This is called on shut-down, and also when the user unloads the add-in
STDMETHODIMP CDSAddIn::OnDisconnection (VARIANT_BOOL bLastTime)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());

	m_pCommands->UnadviseFromEvents ();
	m_pCommands->Release ();
	m_pCommands = NULL;

	return S_OK;
}

// Add a command to DevStudio
// Creates a toolbar button for the command also.
// 'MethodName' is the name of the methode specified in the .odl file
// 'StrResId' the resource id of the descriptive string
// 'GlyphIndex' the image index into the command buttons bitmap
// Return true on success
//
bool CDSAddIn::AddCommand (IApplication* pApp, char* MethodName, char* CmdName,
			   UINT StrResId, UINT GlyphIndex, VARIANT_BOOL bFirstTime)
{
	CString CmdString;
	CString CmdText;

	CmdText.LoadString (StrResId);
	CmdString = CmdName;
	CmdString += CmdText;

	CComBSTR bszCmdString (CmdString);
	CComBSTR bszMethod (MethodName);
	CComBSTR bszCmdName (CmdName);

	// (see stdafx.h for the definition of VERIFY_OK)

	VARIANT_BOOL bRet;
	VERIFY_OK (pApp->AddCommand (bszCmdString, bszMethod, GlyphIndex,
				     m_dwCookie, &bRet));
	if (bRet == VARIANT_FALSE)
	{
		// AddCommand failed because a command with this name already exists.
		ReportInternalError ("IApplication::AddCommand");
		return FALSE;
	}

	// Add toolbar buttons only if this is the first time the add-in
	// is being loaded.  Toolbar buttons are automatically remembered
	// by Developer Studio from session to session, so we should only
	// add the toolbar buttons once.
	if (bFirstTime == VARIANT_TRUE)
		VERIFY_OK (pApp->AddCommandBarButton (dsGlyph, bszCmdName, m_dwCookie));

	return TRUE;
}

void ReportLastError (HRESULT Err)
{
	char *Buf = NULL;
	char Msg[512];

	FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		       NULL, Err,
		       MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		       Buf, 400, NULL);
	sprintf (Msg, "Unexpected error (Error code: %lx)\n%s", Err, Buf);

	::MessageBox (NULL, Msg, "VisVim", MB_OK | MB_ICONSTOP);
	if (Buf)
		LocalFree (Buf);
}

void ReportInternalError (char* Fct)
{
	char Msg[512];

	sprintf (Msg, "Unexpected error\n%s failed", Fct);
	::MessageBox (NULL, Msg, "VisVim", MB_OK | MB_ICONSTOP);
}

