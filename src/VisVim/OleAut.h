#ifndef __OLEAUT_H__
#define __OLEAUT_H__

class COleAutomationControl : public CObject
{
    public:
	COleAutomationControl ();
	~COleAutomationControl ();
	bool CreateObject (char* ProgId);
	DISPID GetDispatchId (char* Name);
	bool GetProperty (char* Name);
	bool GetProperty (DISPID DispatchId);
	bool PutProperty (char* Name, LPCTSTR Format, ...);
	bool PutProperty (DISPID DispatchId, LPCTSTR Format, ...);
	bool Method (char* Name, LPCTSTR Format = NULL, ...);
	bool Method (DISPID DispatchId, LPCTSTR Format = NULL, ...);
	void DeleteObject ();
	void ErrDiag ();
	bool IsCreated ()
	{
		return m_pDispatch ? true : false;
	}
	bool IsAlive ();
	HRESULT GetResult ()
	{
		return m_hResult;
	}
	UINT GetErrArgNr ()
	{
		return m_nErrArg;
	}
	EXCEPINFO* GetExceptionInfo ()
	{
		return &m_ExceptionInfo;
	}
	LPVARIANT GetResultVariant ()
	{
		return &m_VariantResult;
	}

    protected:
	bool Invoke (WORD Flags, char* Name, LPCTSTR Format, va_list ArgList);
	bool Invoke (WORD Flags, DISPID DispatchId, LPCTSTR Format, va_list ArgList);

    protected:
	IDispatch*	m_pDispatch;
	HRESULT		m_hResult;
	UINT		m_nErrArg;
	EXCEPINFO	m_ExceptionInfo;
	VARIANTARG	m_VariantResult;
};

#ifdef UNICODE
    #define FROM_OLE_STRING(str)		str
    #define FROM_OLE_STRING_BUF(str,buf)	str
    #define TO_OLE_STR(str)			str
    #define TO_OLE_STR_BUF(str,buf)		str
    #define MAX_OLE_STR				1
#else
    #define FROM_OLE_STR(str)			ConvertToAnsi(str)
    #define FROM_OLE_STR_BUF(str,buf)		ConvertToAnsiBuf(str,buf)
    char* ConvertToAnsi (OLECHAR* sUnicode);
    char* ConvertToAnsiBuf (OLECHAR* sUnicode, char* Buf);
    #define TO_OLE_STR(str)			ConvertToUnicode(str)
    #define TO_OLE_STR_BUF(str,buf)		ConvertToUnicodeBuf(str,buf)
    OLECHAR* ConvertToUnicode (char* sAscii);
    OLECHAR* ConvertToUnicodeBuf (char* sAscii, OLECHAR* Buf);
    // Maximum length of string that can be converted between Ansi & Unicode
    #define MAX_OLE_STR				500
#endif


#endif		 // __OLEAUT_H__
