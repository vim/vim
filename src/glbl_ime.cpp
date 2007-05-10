/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * DESCRIPTION:
 * This module produces Global IME for Vim, on Windows with Internet
 * Explorer 5.01 or higher.  You need three files "dimm.idl", "dimm.h", and
 * "dimm_i.c" when compile this module at your self.  "dimm.h", and
 * "dimm_i.c" are generated from "dimm.idl" by using MIDL.EXE as like
 * "if_ole.h".  You can get "dimm.idl" in MSDN web site.  I got it below
 * URL.
 *
 * WHAT IS THE GLOBAL IME?:
 * Global IME makes capability input Chinese, Japanese, and Korean text into
 * Vim buffer on any language version of Windows 98, Windows 95, and Windows
 * NT 4.0.  See below URL for detail of Global IME.  You can also find
 * various language version of Global IME at same place.
 *
 * RUNTIME REQUIREMENTS:
 * - Internet Explorer 5.01 or higher.
 * - Global IME (with language pack?).
 * - Of course Vim for Windows.
 *
 * URLS:
 * - Where you can probably get "dimm.idl".
 * http://msdn.microsoft.com/downloads/samples/internet/libraries/ie5_lib/sample.asp
 * - Global IME detailed information.
 * http://www.microsoft.com/windows/ie/features/ime.asp
 */

#ifdef GLOBAL_IME

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
extern "C" {
#include "vim.h"
}
#include "dimm.h"
#include "glbl_ime.h"

static IActiveIMMApp *pIApp = NULL;
static IActiveIMMMessagePumpOwner *pIMsg = NULL;
static HWND s_hWnd = NULL;
static BOOL s_bStatus = FALSE; /* for evacuate */

/*
 * Initialize Global IME.
 * "atom" must be return value of RegisterClass(Ex).
 */
    void
global_ime_init(ATOM atom, HWND hWnd)
{
    IUnknown *pI;
    HRESULT hr;

    if (pIApp != NULL || pIMsg != NULL)
	return;
    OleInitialize(NULL);

    /*
     * Get interface IUnknown
     */
    hr = CoCreateInstance(CLSID_CActiveIMM, NULL, CLSCTX_SERVER,
	    IID_IUnknown, (void**)&pI);
    if (FAILED(hr) || !pI)
	return;

    /*
     * Get interface IActiveIMMApp
     */
    hr = pI->QueryInterface(IID_IActiveIMMApp, (void**)&pIApp);
    if (FAILED(hr))
	pIApp = NULL;

    /*
     * Get interface IActiveIMMMessagePumpOwner
     */
    hr = pI->QueryInterface(IID_IActiveIMMMessagePumpOwner, (void**)&pIMsg);
    if (FAILED(hr))
	pIMsg = NULL;

    if (pIApp != NULL)
    {
	pIApp->Activate(TRUE);
	pIApp->FilterClientWindows(&atom, 1);
    }
    if (pIMsg != NULL)
	pIMsg->Start();

    pI->Release();
    s_hWnd = hWnd;
}

/*
 * Reset and clear Global IME.
 */
    void
global_ime_end()
{
    if (pIApp != NULL)
    {
	IActiveIMMApp *p = pIApp;

	pIApp = NULL;
	p->FilterClientWindows(NULL, 0);
	p->Deactivate();
	p->Release();
    }
    if (pIMsg != NULL)
    {
	IActiveIMMMessagePumpOwner *p = pIMsg;

	pIMsg = NULL;
	p->End();
	p->Release();
    }
    OleUninitialize();
}

/*
 * Replacement for DefWindowProc().
 */
    LRESULT WINAPI
global_ime_DefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;

    if (pIApp == NULL || pIApp->OnDefWindowProc(hWnd, Msg,
					    wParam, lParam, &lResult) != S_OK)
    {
#if defined(WIN3264) && defined(FEAT_MBYTE)
	if (wide_WindowProc)
	    lResult = DefWindowProcW(hWnd, Msg, wParam, lParam);
	else
#endif
	    lResult = DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return lResult;
}

/*
 * Replace with TranslateMessage()
 */
    BOOL WINAPI
global_ime_TranslateMessage(CONST MSG *lpMsg)
{
    if (pIMsg == NULL || pIMsg->OnTranslateMessage(lpMsg) == S_FALSE)
	return TranslateMessage(lpMsg);
    return TRUE;
}

/*
 * Set position of IME compotision window.
 *
 * You have to call this before starting composition.  If once composition
 * started, this can take no effect until that composition have finished.  So
 * you should handle WM_IME_STARTCOMPOSITION and call this function.
 */
    void WINAPI
global_ime_set_position(POINT *pPoint)
{
    HIMC hImc = NULL;

    if (pIApp == NULL || pPoint == NULL)
	return;

    if (SUCCEEDED(pIApp->GetContext(s_hWnd, &hImc)))
    {
	COMPOSITIONFORM CompForm;

	CompForm.dwStyle = CFS_POINT;
	CompForm.ptCurrentPos = *pPoint;
	pIApp->SetCompositionWindow(hImc, &CompForm);
	pIApp->ReleaseContext(s_hWnd, hImc);
    }
}

/*
 * Set font to Global IME
 */
/* GIME_TEST */
    void WINAPI
global_ime_set_font(LOGFONT *pFont)
{
    HIMC hImc = NULL;

    if (pIApp == NULL || pFont == NULL)
	return;

    if (SUCCEEDED(pIApp->GetContext(s_hWnd, &hImc)))
    {
	pIApp->SetCompositionFontA(hImc, pFont);
	pIApp->ReleaseContext(s_hWnd, hImc);
    }
}

#if 0
/*
 * for IME control.  Save current status of IME, and set force new-status to
 * English (turn off).
 */
    void WINAPI
global_ime_status_evacuate()
{
    HIMC    hImc;

    if (pIApp != NULL && SUCCEEDED(pIApp->GetContext(s_hWnd, &hImc)))
    {
	s_bStatus = (pIApp->GetOpenStatus(hImc) == 0) ? TRUE : FALSE;
	pIApp->SetOpenStatus(hImc, FALSE);
	pIApp->ReleaseContext(s_hWnd, hImc);
    }
}

/*
 * for IME control.  Change IME status to last saved one.
 */
    void WINAPI
global_ime_status_restore()
{
    HIMC    hImc;

    if (pIApp != NULL && SUCCEEDED(pIApp->GetContext(s_hWnd, &hImc)))
    {
	pIApp->SetOpenStatus(hImc, s_bStatus);
	pIApp->ReleaseContext(s_hWnd, hImc);
    }
}
#endif

    void WINAPI
global_ime_set_status(int status)
{
    HIMC    hImc;

    if (pIApp != NULL && SUCCEEDED(pIApp->GetContext(s_hWnd, &hImc)))
    {
	pIApp->SetOpenStatus(hImc, status ? TRUE : FALSE);
	pIApp->ReleaseContext(s_hWnd, hImc);
    }
}

    int WINAPI
global_ime_get_status()
{
    int status = 0;
    HIMC    hImc;

    if (pIApp != NULL && SUCCEEDED(pIApp->GetContext(s_hWnd, &hImc)))
    {
	status = pIApp->GetOpenStatus(hImc) ? 1 : 0;
	pIApp->ReleaseContext(s_hWnd, hImc);
    }
    return status;
}

#endif /* GLOBAL_IME */
