// DSAddIn.h : header file
//

#if !defined(AFX_DSADDIN_H__AC726715_2977_11D1_B2F3_006008040780__INCLUDED_)
#define AFX_DSADDIN_H__AC726715_2977_11D1_B2F3_006008040780__INCLUDED_

#include "commands.h"

// {4F9E01C0-406B-11d2-8006-00001C405077}
DEFINE_GUID (CLSID_DSAddIn,
	0x4f9e01c0, 0x406b, 0x11d2, 0x80, 0x6, 0x0, 0x0, 0x1c, 0x40, 0x50, 0x77);

/////////////////////////////////////////////////////////////////////////////
// CDSAddIn

class CDSAddIn :
	public   IDSAddIn,
	public   CComObjectRoot,
	public   CComCoClass < CDSAddIn,
		 &CLSID_DSAddIn >
{
    public:
	DECLARE_REGISTRY (CDSAddIn, "VisVim.DSAddIn.1",
			  "VisVim Developer Studio Add-in", IDS_VISVIM_LONGNAME,
			  THREADFLAGS_BOTH)

	CDSAddIn ()
	{
	}

	BEGIN_COM_MAP (CDSAddIn)
	COM_INTERFACE_ENTRY (IDSAddIn)
	END_COM_MAP ()
	DECLARE_NOT_AGGREGATABLE (CDSAddIn)

	// IDSAddIns
    public:
	STDMETHOD (OnConnection) (THIS_ IApplication * pApp, VARIANT_BOOL bFirstTime,
				  long dwCookie, VARIANT_BOOL * OnConnection);
	STDMETHOD (OnDisconnection) (THIS_ VARIANT_BOOL bLastTime);

    protected:
	bool AddCommand (IApplication* pApp, char* MethodName, char* CmdName,
			 UINT StrResId, UINT GlyphIndex, VARIANT_BOOL bFirstTime);

    protected:
	CCommandsObj * m_pCommands;
	DWORD m_dwCookie;
};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_DSADDIN_H__AC726715_2977_11D1_B2F3_006008040780__INCLUDED)
