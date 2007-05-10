//
// Class for creating OLE automation controllers.
//
// CreateObject() creates an automation object
// Invoke() will call a property or method of the automation object.
// GetProperty() returns a property
// SetProperty() changes a property
// Method() invokes a method
//
// For example, the following VB code will control Microsoft Word:
//
//    Private Sub Form_Load()
//    Dim wb As Object
//    Set wb = CreateObject("Word.Basic")
//    wb.AppShow
//    wb.FileNewDefault
//    wb.Insert "This is a test"
//    wb.FileSaveAs "c:\sample.doc)"
//    End Sub
//
// A C++ automation controller that does the same can be written as follows:
// the helper functions:
//
//   Void FormLoad ()
//   {
//       COleAutomationControl Aut;
//       Aut.CreateObject("Word.Basic");
//       Aut.Method ("AppShow");
//       Aut.Method ("FileNewDefault");
//       Aut.Method ("Insert", "s", (LPOLESTR) OLESTR ("This is a test"));
//       Aut.Method ("FileSaveAs", "s", OLESTR ("c:\\sample.doc"));
//   }
//
//

#include "stdafx.h"
#include <stdarg.h>
#include "oleaut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static bool CountArgsInFormat (LPCTSTR Format, UINT* nArgs);
static LPCTSTR GetNextVarType (LPCTSTR Format, VARTYPE* pVarType);


COleAutomationControl::COleAutomationControl ()
{
	m_pDispatch = NULL;
	m_hResult = NOERROR;
	m_nErrArg = 0;
	VariantInit (&m_VariantResult);
}

COleAutomationControl::~COleAutomationControl ()
{
	DeleteObject ();
}

void COleAutomationControl::DeleteObject ()
{
	if (m_pDispatch)
	{
		m_pDispatch->Release ();
		m_pDispatch = NULL;
	}
}

// Creates an instance of the Automation object and
// obtains it's IDispatch interface.
//
// Parameters:
// ProgId	  ProgID of Automation object
//
bool COleAutomationControl::CreateObject (char* ProgId)
{
	CLSID ClsId;			// CLSID of automation object
	LPUNKNOWN pUnknown = NULL;	// IUnknown of automation object

	// Retrieve CLSID from the progID that the user specified
	LPOLESTR OleProgId = TO_OLE_STR (ProgId);
	m_hResult = CLSIDFromProgID (OleProgId, &ClsId);
	if (FAILED (m_hResult))
		goto error;

	// Create an instance of the automation object and ask for the
	// IDispatch interface
	m_hResult = CoCreateInstance (ClsId, NULL, CLSCTX_SERVER,
			       IID_IUnknown, (void**) &pUnknown);
	if (FAILED (m_hResult))
		goto error;

	m_hResult = pUnknown->QueryInterface (IID_IDispatch, (void**) &m_pDispatch);
	if (FAILED (m_hResult))
		goto error;

	pUnknown->Release ();
	return true;

error:
	if (pUnknown)
		pUnknown->Release ();
	if (m_pDispatch)
		m_pDispatch->Release ();
	return false;
}

// Return the dispatch id of a named service
// This id can be used in subsequent calls to GetProperty (), SetProperty () and
// Method (). This is the preferred method when performance is important.
//
DISPID COleAutomationControl::GetDispatchId (char* Name)
{
	DISPID DispatchId;

	ASSERT (m_pDispatch);

	// Get DISPID of property/method
	LPOLESTR OleName = TO_OLE_STR (Name);
	m_hResult = m_pDispatch->GetIDsOfNames (IID_NULL, &OleName, 1,
						LOCALE_USER_DEFAULT, &DispatchId);
	if (FAILED (m_hResult))
		return NULL;
	return DispatchId;
}

//  The following functions use these parameters:
//
// Parameters:
//
//  Name      Name of property or method.
//
//  Format    Format string that describes the variable list of parameters that
//	      follows. The format string can contain the following characters.
//	      & = mark the following format character as VT_BYREF
//	      B = VT_BOOL
//	      i = VT_I2
//	      I = VT_I4
//	      r = VT_R2
//	      R = VT_R4
//	      c = VT_CY
//	      s = VT_BSTR (string pointer can be passed,
//			BSTR will be allocated by this function).
//	      e = VT_ERROR
//	      d = VT_DATE
//	      v = VT_VARIANT. Use this to pass data types that are not described
//			in the format string. (For example SafeArrays).
//	      D = VT_DISPATCH
//	      U = VT_UNKNOWN
//
//  ...       Arguments of the property or method.
//	      Arguments are described by Format.
//

bool COleAutomationControl::GetProperty (char* Name)
{
	return Invoke (DISPATCH_PROPERTYGET, Name, NULL, NULL);
}

bool COleAutomationControl::GetProperty (DISPID DispatchId)
{
	return Invoke (DISPATCH_PROPERTYGET, DispatchId, NULL, NULL);
}

bool COleAutomationControl::PutProperty (char* Name, LPCTSTR Format, ...)
{
	va_list ArgList;

	va_start (ArgList, Format);
	bool bRet = Invoke (DISPATCH_PROPERTYPUT, Name, Format, ArgList);
	va_end (ArgList);
	return bRet;
}

bool COleAutomationControl::PutProperty (DISPID DispatchId, LPCTSTR Format, ...)
{
	va_list ArgList;

	va_start (ArgList, Format);
	bool bRet = Invoke (DISPATCH_PROPERTYPUT, DispatchId, Format, ArgList);
	va_end (ArgList);
	return bRet;
}

bool COleAutomationControl::Method (char* Name, LPCTSTR Format, ...)
{
	va_list ArgList;

	va_start (ArgList, Format);
	bool bRet = Invoke (DISPATCH_METHOD, Name, Format, ArgList);
	va_end (ArgList);
	return bRet;
}

bool COleAutomationControl::Method (DISPID DispatchId, LPCTSTR Format, ...)
{
	va_list ArgList;

	va_start (ArgList, Format);
	bool bRet = Invoke (DISPATCH_METHOD, DispatchId, Format, ArgList);
	va_end (ArgList);
	return bRet;
}

bool COleAutomationControl::Invoke (WORD Flags, char* Name,
				    LPCTSTR Format, va_list ArgList)
{
	DISPID DispatchId = GetDispatchId (Name);
	if (! DispatchId)
		return false;
	return Invoke (Flags, DispatchId, Format, ArgList);
}

bool COleAutomationControl::Invoke (WORD Flags, DISPID DispatchId,
				    LPCTSTR Format, va_list ArgList)
{
	UINT ArgCount = 0;
	VARIANTARG* ArgVector = NULL;

	ASSERT (m_pDispatch);

	DISPPARAMS DispatchParams;
	memset (&DispatchParams, 0, sizeof (DispatchParams));

	// Determine number of arguments
	if (Format)
		CountArgsInFormat (Format, &ArgCount);

	// Property puts have a named argument that represents the value that
	// the property is being assigned.
	DISPID DispIdNamed = DISPID_PROPERTYPUT;
	if (Flags & DISPATCH_PROPERTYPUT)
	{
		if (ArgCount == 0)
		{
			m_hResult = ResultFromScode (E_INVALIDARG);
			return false;
		}
		DispatchParams.cNamedArgs = 1;
		DispatchParams.rgdispidNamedArgs = &DispIdNamed;
	}

	if (ArgCount)
	{
		// Allocate memory for all VARIANTARG parameters
		ArgVector = (VARIANTARG*) CoTaskMemAlloc (
				ArgCount * sizeof (VARIANTARG));
		if (! ArgVector)
		{
			m_hResult = ResultFromScode (E_OUTOFMEMORY);
			return false;
		}
		memset (ArgVector, 0, sizeof (VARIANTARG) * ArgCount);

		// Get ready to walk vararg list
		LPCTSTR s = Format;

		VARIANTARG *p = ArgVector + ArgCount - 1;  // Params go in opposite order

		for (;;)
		{
			VariantInit (p);
			if (! (s = GetNextVarType (s, &p->vt)))
				break;

			if (p < ArgVector)
			{
				m_hResult = ResultFromScode (E_INVALIDARG);
				goto Cleanup;
			}
			switch (p->vt)
			{
			    case VT_I2:
				V_I2 (p) = va_arg (ArgList, short);
				break;
			    case VT_I4:
				V_I4 (p) = va_arg (ArgList, long);
				break;
			    case VT_R4:
				V_R4 (p) = va_arg (ArgList, float);
				break;
			    case VT_DATE:
			    case VT_R8:
				V_R8 (p) = va_arg (ArgList, double);
				break;
			    case VT_CY:
				V_CY (p) = va_arg (ArgList, CY);
				break;
			    case VT_BSTR:
				V_BSTR (p) = SysAllocString (va_arg (ArgList,
								     OLECHAR*));
				if (! p->bstrVal)
				{
					m_hResult = ResultFromScode (E_OUTOFMEMORY);
					p->vt = VT_EMPTY;
					goto Cleanup;
				}
				break;
			    case VT_DISPATCH:
				V_DISPATCH (p) = va_arg (ArgList, LPDISPATCH);
				break;
			    case VT_ERROR:
				V_ERROR (p) = va_arg (ArgList, SCODE);
				break;
			    case VT_BOOL:
				V_BOOL (p) = va_arg (ArgList, BOOL) ? -1 : 0;
				break;
			    case VT_VARIANT:
				*p = va_arg (ArgList, VARIANTARG);
				break;
			    case VT_UNKNOWN:
				V_UNKNOWN (p) = va_arg (ArgList, LPUNKNOWN);
				break;

			    case VT_I2 | VT_BYREF:
				V_I2REF (p) = va_arg (ArgList, short*);
				break;
			    case VT_I4 | VT_BYREF:
				V_I4REF (p) = va_arg (ArgList, long*);
				break;
			    case VT_R4 | VT_BYREF:
				V_R4REF (p) = va_arg (ArgList, float*);
				break;
			    case VT_R8 | VT_BYREF:
				V_R8REF (p) = va_arg (ArgList, double*);
				break;
			    case VT_DATE | VT_BYREF:
				V_DATEREF (p) = va_arg (ArgList, DATE*);
				break;
			    case VT_CY | VT_BYREF:
				V_CYREF (p) = va_arg (ArgList, CY*);
				break;
			    case VT_BSTR | VT_BYREF:
				V_BSTRREF (p) = va_arg (ArgList, BSTR*);
				break;
			    case VT_DISPATCH | VT_BYREF:
				V_DISPATCHREF (p) = va_arg (ArgList, LPDISPATCH*);
				break;
			    case VT_ERROR | VT_BYREF:
				V_ERRORREF (p) = va_arg (ArgList, SCODE*);
				break;
			    case VT_BOOL | VT_BYREF:
				{
					BOOL* pBool = va_arg (ArgList, BOOL*);

					*pBool = 0;
					V_BOOLREF (p) = (VARIANT_BOOL*) pBool;
				}
				break;
			    case VT_VARIANT | VT_BYREF:
				V_VARIANTREF (p) = va_arg (ArgList, VARIANTARG*);
				break;
			    case VT_UNKNOWN | VT_BYREF:
				V_UNKNOWNREF (p) = va_arg (ArgList, LPUNKNOWN*);
				break;

			    default:
				{
					m_hResult = ResultFromScode (E_INVALIDARG);
					goto Cleanup;
				}
				break;
			}

			--p;	// Get ready to fill next argument
		}
	}

	DispatchParams.cArgs = ArgCount;
	DispatchParams.rgvarg = ArgVector;

	// Initialize return variant, in case caller forgot. Caller can pass
	// NULL if return value is not expected.
	VariantInit (&m_VariantResult);

	// Make the call
	m_hResult = m_pDispatch->Invoke (DispatchId, IID_NULL, LOCALE_USER_DEFAULT,
					 Flags, &DispatchParams, &m_VariantResult,
					 &m_ExceptionInfo, &m_nErrArg);

    Cleanup:
	// Cleanup any arguments that need cleanup
	if (ArgCount)
	{
		VARIANTARG* p = ArgVector;

		while (ArgCount--)
		{
			switch (p->vt)
			{
			    case VT_BSTR:
				VariantClear (p);
				break;
			}
			++p;
		}
		CoTaskMemFree (ArgVector);
	}

	return FAILED (m_hResult) ? false : true;
}

#define CASE_SCODE(sc)  \
	case sc: \
	lstrcpy((char*)ErrName, (char*)#sc); \
	break;

void COleAutomationControl::ErrDiag ()
{
	char ErrName[200];

	SCODE sc = GetScode (m_hResult);
	switch (sc)
	{
	    // SCODE's defined in SCODE.H
	    CASE_SCODE (S_OK)
	    CASE_SCODE (S_FALSE)
	    CASE_SCODE (E_UNEXPECTED)
	    CASE_SCODE (E_OUTOFMEMORY)
	    CASE_SCODE (E_INVALIDARG)
	    CASE_SCODE (E_NOINTERFACE)
	    CASE_SCODE (E_POINTER)
	    CASE_SCODE (E_HANDLE)
	    CASE_SCODE (E_ABORT)
	    CASE_SCODE (E_FAIL)
	    CASE_SCODE (E_ACCESSDENIED)

	    // SCODE's defined in OLE2.H
	    CASE_SCODE (OLE_E_OLEVERB)
	    CASE_SCODE (OLE_E_ADVF)
	    CASE_SCODE (OLE_E_ENUM_NOMORE)
	    CASE_SCODE (OLE_E_ADVISENOTSUPPORTED)
	    CASE_SCODE (OLE_E_NOCONNECTION)
	    CASE_SCODE (OLE_E_NOTRUNNING)
	    CASE_SCODE (OLE_E_NOCACHE)
	    CASE_SCODE (OLE_E_BLANK)
	    CASE_SCODE (OLE_E_CLASSDIFF)
	    CASE_SCODE (OLE_E_CANT_GETMONIKER)
	    CASE_SCODE (OLE_E_CANT_BINDTOSOURCE)
	    CASE_SCODE (OLE_E_STATIC)
	    CASE_SCODE (OLE_E_PROMPTSAVECANCELLED)
	    CASE_SCODE (OLE_E_INVALIDRECT)
	    CASE_SCODE (OLE_E_WRONGCOMPOBJ)
	    CASE_SCODE (OLE_E_INVALIDHWND)
	    CASE_SCODE (OLE_E_NOT_INPLACEACTIVE)
	    CASE_SCODE (OLE_E_CANTCONVERT)
	    CASE_SCODE (OLE_E_NOSTORAGE)

	    CASE_SCODE (DV_E_FORMATETC)
	    CASE_SCODE (DV_E_DVTARGETDEVICE)
	    CASE_SCODE (DV_E_STGMEDIUM)
	    CASE_SCODE (DV_E_STATDATA)
	    CASE_SCODE (DV_E_LINDEX)
	    CASE_SCODE (DV_E_TYMED)
	    CASE_SCODE (DV_E_CLIPFORMAT)
	    CASE_SCODE (DV_E_DVASPECT)
	    CASE_SCODE (DV_E_DVTARGETDEVICE_SIZE)
	    CASE_SCODE (DV_E_NOIVIEWOBJECT)

	    CASE_SCODE (OLE_S_USEREG)
	    CASE_SCODE (OLE_S_STATIC)
	    CASE_SCODE (OLE_S_MAC_CLIPFORMAT)

	    CASE_SCODE (CONVERT10_E_OLESTREAM_GET)
	    CASE_SCODE (CONVERT10_E_OLESTREAM_PUT)
	    CASE_SCODE (CONVERT10_E_OLESTREAM_FMT)
	    CASE_SCODE (CONVERT10_E_OLESTREAM_BITMAP_TO_DIB)
	    CASE_SCODE (CONVERT10_E_STG_FMT)
	    CASE_SCODE (CONVERT10_E_STG_NO_STD_STREAM)
	    CASE_SCODE (CONVERT10_E_STG_DIB_TO_BITMAP)
	    CASE_SCODE (CONVERT10_S_NO_PRESENTATION)

	    CASE_SCODE (CLIPBRD_E_CANT_OPEN)
	    CASE_SCODE (CLIPBRD_E_CANT_EMPTY)
	    CASE_SCODE (CLIPBRD_E_CANT_SET)
	    CASE_SCODE (CLIPBRD_E_BAD_DATA)
	    CASE_SCODE (CLIPBRD_E_CANT_CLOSE)

	    CASE_SCODE (DRAGDROP_E_NOTREGISTERED)
	    CASE_SCODE (DRAGDROP_E_ALREADYREGISTERED)
	    CASE_SCODE (DRAGDROP_E_INVALIDHWND)
	    CASE_SCODE (DRAGDROP_S_DROP)
	    CASE_SCODE (DRAGDROP_S_CANCEL)
	    CASE_SCODE (DRAGDROP_S_USEDEFAULTCURSORS)

	    CASE_SCODE (OLEOBJ_E_NOVERBS)
	    CASE_SCODE (OLEOBJ_E_INVALIDVERB)
	    CASE_SCODE (OLEOBJ_S_INVALIDVERB)
	    CASE_SCODE (OLEOBJ_S_CANNOT_DOVERB_NOW)
	    CASE_SCODE (OLEOBJ_S_INVALIDHWND)
	    CASE_SCODE (INPLACE_E_NOTUNDOABLE)
	    CASE_SCODE (INPLACE_E_NOTOOLSPACE)
	    CASE_SCODE (INPLACE_S_TRUNCATED)

	    // SCODE's defined in COMPOBJ.H
	    CASE_SCODE (CO_E_NOTINITIALIZED)
	    CASE_SCODE (CO_E_ALREADYINITIALIZED)
	    CASE_SCODE (CO_E_CANTDETERMINECLASS)
	    CASE_SCODE (CO_E_CLASSSTRING)
	    CASE_SCODE (CO_E_IIDSTRING)
	    CASE_SCODE (CO_E_APPNOTFOUND)
	    CASE_SCODE (CO_E_APPSINGLEUSE)
	    CASE_SCODE (CO_E_ERRORINAPP)
	    CASE_SCODE (CO_E_DLLNOTFOUND)
	    CASE_SCODE (CO_E_ERRORINDLL)
	    CASE_SCODE (CO_E_WRONGOSFORAPP)
	    CASE_SCODE (CO_E_OBJNOTREG)
	    CASE_SCODE (CO_E_OBJISREG)
	    CASE_SCODE (CO_E_OBJNOTCONNECTED)
	    CASE_SCODE (CO_E_APPDIDNTREG)
	    CASE_SCODE (CLASS_E_NOAGGREGATION)
	    CASE_SCODE (CLASS_E_CLASSNOTAVAILABLE)
	    CASE_SCODE (REGDB_E_READREGDB)
	    CASE_SCODE (REGDB_E_WRITEREGDB)
	    CASE_SCODE (REGDB_E_KEYMISSING)
	    CASE_SCODE (REGDB_E_INVALIDVALUE)
	    CASE_SCODE (REGDB_E_CLASSNOTREG)
	    CASE_SCODE (REGDB_E_IIDNOTREG)
	    CASE_SCODE (RPC_E_CALL_REJECTED)
	    CASE_SCODE (RPC_E_CALL_CANCELED)
	    CASE_SCODE (RPC_E_CANTPOST_INSENDCALL)
	    CASE_SCODE (RPC_E_CANTCALLOUT_INASYNCCALL)
	    CASE_SCODE (RPC_E_CANTCALLOUT_INEXTERNALCALL)
	    CASE_SCODE (RPC_E_CONNECTION_TERMINATED)
	    CASE_SCODE (RPC_E_SERVER_DIED)
	    CASE_SCODE (RPC_E_CLIENT_DIED)
	    CASE_SCODE (RPC_E_INVALID_DATAPACKET)
	    CASE_SCODE (RPC_E_CANTTRANSMIT_CALL)
	    CASE_SCODE (RPC_E_CLIENT_CANTMARSHAL_DATA)
	    CASE_SCODE (RPC_E_CLIENT_CANTUNMARSHAL_DATA)
	    CASE_SCODE (RPC_E_SERVER_CANTMARSHAL_DATA)
	    CASE_SCODE (RPC_E_SERVER_CANTUNMARSHAL_DATA)
	    CASE_SCODE (RPC_E_INVALID_DATA)
	    CASE_SCODE (RPC_E_INVALID_PARAMETER)
	    CASE_SCODE (RPC_E_CANTCALLOUT_AGAIN)
	    CASE_SCODE (RPC_E_UNEXPECTED)

	    // SCODE's defined in DVOBJ.H
	    CASE_SCODE (DATA_S_SAMEFORMATETC)
	    CASE_SCODE (VIEW_E_DRAW)
	    CASE_SCODE (VIEW_S_ALREADY_FROZEN)
	    CASE_SCODE (CACHE_E_NOCACHE_UPDATED)
	    CASE_SCODE (CACHE_S_FORMATETC_NOTSUPPORTED)
	    CASE_SCODE (CACHE_S_SAMECACHE)
	    CASE_SCODE (CACHE_S_SOMECACHES_NOTUPDATED)

	    // SCODE's defined in STORAGE.H
	    CASE_SCODE (STG_E_INVALIDFUNCTION)
	    CASE_SCODE (STG_E_FILENOTFOUND)
	    CASE_SCODE (STG_E_PATHNOTFOUND)
	    CASE_SCODE (STG_E_TOOMANYOPENFILES)
	    CASE_SCODE (STG_E_ACCESSDENIED)
	    CASE_SCODE (STG_E_INVALIDHANDLE)
	    CASE_SCODE (STG_E_INSUFFICIENTMEMORY)
	    CASE_SCODE (STG_E_INVALIDPOINTER)
	    CASE_SCODE (STG_E_NOMOREFILES)
	    CASE_SCODE (STG_E_DISKISWRITEPROTECTED)
	    CASE_SCODE (STG_E_SEEKERROR)
	    CASE_SCODE (STG_E_WRITEFAULT)
	    CASE_SCODE (STG_E_READFAULT)
	    CASE_SCODE (STG_E_SHAREVIOLATION)
	    CASE_SCODE (STG_E_LOCKVIOLATION)
	    CASE_SCODE (STG_E_FILEALREADYEXISTS)
	    CASE_SCODE (STG_E_INVALIDPARAMETER)
	    CASE_SCODE (STG_E_MEDIUMFULL)
	    CASE_SCODE (STG_E_ABNORMALAPIEXIT)
	    CASE_SCODE (STG_E_INVALIDHEADER)
	    CASE_SCODE (STG_E_INVALIDNAME)
	    CASE_SCODE (STG_E_UNKNOWN)
	    CASE_SCODE (STG_E_UNIMPLEMENTEDFUNCTION)
	    CASE_SCODE (STG_E_INVALIDFLAG)
	    CASE_SCODE (STG_E_INUSE)
	    CASE_SCODE (STG_E_NOTCURRENT)
	    CASE_SCODE (STG_E_REVERTED)
	    CASE_SCODE (STG_E_CANTSAVE)
	    CASE_SCODE (STG_E_OLDFORMAT)
	    CASE_SCODE (STG_E_OLDDLL)
	    CASE_SCODE (STG_E_SHAREREQUIRED)
	    CASE_SCODE (STG_E_NOTFILEBASEDSTORAGE)
	    CASE_SCODE (STG_E_EXTANTMARSHALLINGS)
	    CASE_SCODE (STG_S_CONVERTED)

	    // SCODE's defined in STORAGE.H
	    CASE_SCODE (MK_E_CONNECTMANUALLY)
	    CASE_SCODE (MK_E_EXCEEDEDDEADLINE)
	    CASE_SCODE (MK_E_NEEDGENERIC)
	    CASE_SCODE (MK_E_UNAVAILABLE)
	    CASE_SCODE (MK_E_SYNTAX)
	    CASE_SCODE (MK_E_NOOBJECT)
	    CASE_SCODE (MK_E_INVALIDEXTENSION)
	    CASE_SCODE (MK_E_INTERMEDIATEINTERFACENOTSUPPORTED)
	    CASE_SCODE (MK_E_NOTBINDABLE)
	    CASE_SCODE (MK_E_NOTBOUND)
	    CASE_SCODE (MK_E_CANTOPENFILE)
	    CASE_SCODE (MK_E_MUSTBOTHERUSER)
	    CASE_SCODE (MK_E_NOINVERSE)
	    CASE_SCODE (MK_E_NOSTORAGE)
	    CASE_SCODE (MK_E_NOPREFIX)
	    CASE_SCODE (MK_S_REDUCED_TO_SELF)
	    CASE_SCODE (MK_S_ME)
	    CASE_SCODE (MK_S_HIM)
	    CASE_SCODE (MK_S_US)
	    CASE_SCODE (MK_S_MONIKERALREADYREGISTERED)

	    // SCODE's defined in DISPATCH.H
	    CASE_SCODE (DISP_E_UNKNOWNINTERFACE)
	    CASE_SCODE (DISP_E_MEMBERNOTFOUND)
	    CASE_SCODE (DISP_E_PARAMNOTFOUND)
	    CASE_SCODE (DISP_E_TYPEMISMATCH)
	    CASE_SCODE (DISP_E_UNKNOWNNAME)
	    CASE_SCODE (DISP_E_NONAMEDARGS)
	    CASE_SCODE (DISP_E_BADVARTYPE)
	    CASE_SCODE (DISP_E_EXCEPTION)
	    CASE_SCODE (DISP_E_OVERFLOW)
	    CASE_SCODE (DISP_E_BADINDEX)
	    CASE_SCODE (DISP_E_UNKNOWNLCID)
	    CASE_SCODE (DISP_E_ARRAYISLOCKED)
	    CASE_SCODE (DISP_E_BADPARAMCOUNT)
	    CASE_SCODE (DISP_E_PARAMNOTOPTIONAL)
	    CASE_SCODE (DISP_E_BADCALLEE)
	    CASE_SCODE (DISP_E_NOTACOLLECTION)

	    CASE_SCODE (TYPE_E_BUFFERTOOSMALL)
	    CASE_SCODE (TYPE_E_INVDATAREAD)
	    CASE_SCODE (TYPE_E_UNSUPFORMAT)
	    CASE_SCODE (TYPE_E_REGISTRYACCESS)
	    CASE_SCODE (TYPE_E_LIBNOTREGISTERED)
	    CASE_SCODE (TYPE_E_UNDEFINEDTYPE)
	    CASE_SCODE (TYPE_E_QUALIFIEDNAMEDISALLOWED)
	    CASE_SCODE (TYPE_E_INVALIDSTATE)
	    CASE_SCODE (TYPE_E_WRONGTYPEKIND)
	    CASE_SCODE (TYPE_E_ELEMENTNOTFOUND)
	    CASE_SCODE (TYPE_E_AMBIGUOUSNAME)
	    CASE_SCODE (TYPE_E_NAMECONFLICT)
	    CASE_SCODE (TYPE_E_UNKNOWNLCID)
	    CASE_SCODE (TYPE_E_DLLFUNCTIONNOTFOUND)
	    CASE_SCODE (TYPE_E_BADMODULEKIND)
	    CASE_SCODE (TYPE_E_SIZETOOBIG)
	    CASE_SCODE (TYPE_E_DUPLICATEID)
	    CASE_SCODE (TYPE_E_TYPEMISMATCH)
	    CASE_SCODE (TYPE_E_OUTOFBOUNDS)
	    CASE_SCODE (TYPE_E_IOERROR)
	    CASE_SCODE (TYPE_E_CANTCREATETMPFILE)
	    CASE_SCODE (TYPE_E_CANTLOADLIBRARY)
	    CASE_SCODE (TYPE_E_INCONSISTENTPROPFUNCS)
	    CASE_SCODE (TYPE_E_CIRCULARTYPE)

	    default:
		lstrcpy (ErrName, "UNKNOWN SCODE");
	}

	char Buf[256];
	sprintf (Buf, "An OLE error occured:\r\nCode = %s\r\nResult = %lx.",
		 (char*) ErrName, m_hResult);
	MessageBox (NULL, Buf, "OLE Error", MB_OK);
}


static bool CountArgsInFormat (LPCTSTR Format, UINT* pArgCount)
{
	*pArgCount = 0;

	if (! Format)
		return true;

	while (*Format)
	{
		if (*Format == '&')
			Format++;

		switch (*Format)
		{
		    case 'b':
		    case 'i':
		    case 'I':
		    case 'r':
		    case 'R':
		    case 'c':
		    case 's':
		    case 'e':
		    case 'd':
		    case 'v':
		    case 'D':
		    case 'U':
			++ (*pArgCount);
			Format++;
			break;
		    case '\0':
		    default:
			return false;
		}
	}
	return true;
}

static LPCTSTR GetNextVarType (LPCTSTR Format, VARTYPE* pVarType)
{
	*pVarType = 0;
	if (*Format == '&')
	{
		*pVarType = VT_BYREF;
		Format++;
		if (!*Format)
			return NULL;
	}
	switch (*Format)
	{
	    case 'b':
		*pVarType |= VT_BOOL;
		break;
	    case 'i':
		*pVarType |= VT_I2;
		break;
	    case 'I':
		*pVarType |= VT_I4;
		break;
	    case 'r':
		*pVarType |= VT_R4;
		break;
	    case 'R':
		*pVarType |= VT_R8;
		break;
	    case 'c':
		*pVarType |= VT_CY;
		break;
	    case 's':
		*pVarType |= VT_BSTR;
		break;
	    case 'e':
		*pVarType |= VT_ERROR;
		break;
	    case 'd':
		*pVarType |= VT_DATE;
		break;
	    case 'v':
		*pVarType |= VT_VARIANT;
		break;
	    case 'U':
		*pVarType |= VT_UNKNOWN;
		break;
	    case 'D':
		*pVarType |= VT_DISPATCH;
		break;
	    case '\0':
		return NULL;	// End of Format string
	    default:
		return NULL;
	}
	return ++Format;
}

#ifndef UNICODE
char* ConvertToAnsi (OLECHAR* sUnicode)
{
	static char BufAscii[MAX_OLE_STR];
	return ConvertToAnsiBuf (sUnicode, BufAscii);
}

char* ConvertToAnsiBuf (OLECHAR* sUnicode, char* BufAscii)
{
	WideCharToMultiByte (CP_ACP, 0, sUnicode, -1, BufAscii, MAX_OLE_STR, NULL, NULL);
	return BufAscii;
}

OLECHAR* ConvertToUnicode (char* sAscii)
{
	static OLECHAR BufUnicode[MAX_OLE_STR];
	return ConvertToUnicodeBuf (sAscii, BufUnicode);
}

OLECHAR* ConvertToUnicodeBuf (char* sAscii, OLECHAR* BufUnicode)
{
	MultiByteToWideChar (CP_ACP, 0, sAscii, -1, BufUnicode, MAX_OLE_STR);
	return BufUnicode;
}
#endif

