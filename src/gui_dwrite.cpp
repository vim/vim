/* vi:set ts=8 sts=4 sw=4 noet: */
/*
 * Author: MURAOKA Taro <koron.kaoriya@gmail.com>
 *
 * Contributors:
 *  - Ken Takata
 *
 * Copyright (C) 2013 MURAOKA Taro <koron.kaoriya@gmail.com>
 * THIS FILE IS DISTRIBUTED UNDER THE VIM LICENSE.
 */

#define WIN32_LEAN_AND_MEAN

#ifndef DYNAMIC_DIRECTX
# if WINVER < 0x0600
#  error WINVER must be 0x0600 or above to use DirectWrite(DirectX)
# endif
#endif

#include <windows.h>
#include <crtdbg.h>
#include <assert.h>
#include <math.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

#include "gui_dwrite.h"

#ifdef __MINGW32__
# define __maybenull	SAL__maybenull
# define __in		SAL__in
# define __out		SAL__out
#endif

#if (defined(_MSC_VER) && (_MSC_VER >= 1700)) || (__cplusplus >= 201103L)
# define FINAL final
#else
# define FINAL
#endif

#ifdef DYNAMIC_DIRECTX
extern "C" HINSTANCE vimLoadLib(char *name);

typedef int (WINAPI *PGETUSERDEFAULTLOCALENAME)(LPWSTR, int);
typedef HRESULT (WINAPI *PD2D1CREATEFACTORY)(D2D1_FACTORY_TYPE,
	REFIID, const D2D1_FACTORY_OPTIONS *, void **);
typedef HRESULT (WINAPI *PDWRITECREATEFACTORY)(DWRITE_FACTORY_TYPE,
	REFIID, IUnknown **);

static HINSTANCE hD2D1DLL = NULL;
static HINSTANCE hDWriteDLL = NULL;

static PGETUSERDEFAULTLOCALENAME pGetUserDefaultLocaleName = NULL;
static PD2D1CREATEFACTORY pD2D1CreateFactory = NULL;
static PDWRITECREATEFACTORY pDWriteCreateFactory = NULL;

#define GetUserDefaultLocaleName	(*pGetUserDefaultLocaleName)
#define D2D1CreateFactory		(*pD2D1CreateFactory)
#define DWriteCreateFactory		(*pDWriteCreateFactory)

    static void
unload(HINSTANCE &hinst)
{
    if (hinst != NULL)
    {
	FreeLibrary(hinst);
	hinst = NULL;
    }
}
#endif // DYNAMIC_DIRECTX

template <class T> inline void SafeRelease(T **ppT)
{
    if (*ppT)
    {
	(*ppT)->Release();
	*ppT = NULL;
    }
}

struct GdiTextRendererContext
{
    // const fields.
    COLORREF color;
    FLOAT cellWidth;

    // working fields.
    FLOAT offsetX;
};

    static DWRITE_PIXEL_GEOMETRY
ToPixelGeometry(int value)
{
    switch (value)
    {
	default:
	case 0:
	    return DWRITE_PIXEL_GEOMETRY_FLAT;
	case 1:
	    return DWRITE_PIXEL_GEOMETRY_RGB;
	case 2:
	    return DWRITE_PIXEL_GEOMETRY_BGR;
    }
}

    static int
ToInt(DWRITE_PIXEL_GEOMETRY value)
{
    switch (value)
    {
	case DWRITE_PIXEL_GEOMETRY_FLAT:
	    return 0;
	case DWRITE_PIXEL_GEOMETRY_RGB:
	    return 1;
	case DWRITE_PIXEL_GEOMETRY_BGR:
	    return 2;
	default:
	    return -1;
    }
}

    static DWRITE_RENDERING_MODE
ToRenderingMode(int value)
{
    switch (value)
    {
	default:
	case 0:
	    return DWRITE_RENDERING_MODE_DEFAULT;
	case 1:
	    return DWRITE_RENDERING_MODE_ALIASED;
	case 2:
	    return DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC;
	case 3:
	    return DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL;
	case 4:
	    return DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL;
	case 5:
	    return DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC;
	case 6:
	    return DWRITE_RENDERING_MODE_OUTLINE;
    }
}

    static D2D1_TEXT_ANTIALIAS_MODE
ToTextAntialiasMode(int value)
{
    switch (value)
    {
	default:
	case 0:
	    return D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
	case 1:
	    return D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;
	case 2:
	    return D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE;
	case 3:
	    return D2D1_TEXT_ANTIALIAS_MODE_ALIASED;
    }
}

    static int
ToInt(DWRITE_RENDERING_MODE value)
{
    switch (value)
    {
	case DWRITE_RENDERING_MODE_DEFAULT:
	    return 0;
	case DWRITE_RENDERING_MODE_ALIASED:
	    return 1;
	case DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC:
	    return 2;
	case DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL:
	    return 3;
	case DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL:
	    return 4;
	case DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC:
	    return 5;
	case DWRITE_RENDERING_MODE_OUTLINE:
	    return 6;
	default:
	    return -1;
    }
}

class AdjustedGlyphRun : public DWRITE_GLYPH_RUN
{
private:
    FLOAT mDelta;
    FLOAT *mAdjustedAdvances;

public:
    AdjustedGlyphRun(
	    const DWRITE_GLYPH_RUN *glyphRun,
	    FLOAT cellWidth) :
	DWRITE_GLYPH_RUN(*glyphRun),
	mDelta(0.0f),
	mAdjustedAdvances(new FLOAT[glyphRun->glyphCount])
    {
	assert(cellWidth != 0.0f);
	for (UINT32 i = 0; i < glyphRun->glyphCount; ++i)
	{
	    FLOAT orig = glyphRun->glyphAdvances[i];
	    FLOAT adjusted = adjustToCell(orig, cellWidth);
	    mAdjustedAdvances[i] = adjusted;
	    mDelta += adjusted - orig;
	}
	glyphAdvances = mAdjustedAdvances;
    }

    ~AdjustedGlyphRun(void)
    {
	delete[] mAdjustedAdvances;
    }

    FLOAT getDelta(void) const
    {
	return mDelta;
    }

    static FLOAT adjustToCell(FLOAT value, FLOAT cellWidth)
    {
	int cellCount = (int)floor(value / cellWidth + 0.5f);
	if (cellCount < 1)
	    cellCount = 1;
	return cellCount * cellWidth;
    }
};

class GdiTextRenderer FINAL : public IDWriteTextRenderer
{
public:
    GdiTextRenderer(
	    IDWriteBitmapRenderTarget* bitmapRenderTarget,
	    IDWriteRenderingParams* renderingParams) :
	cRefCount_(0),
	pRenderTarget_(bitmapRenderTarget),
	pRenderingParams_(renderingParams)
    {
	pRenderTarget_->AddRef();
	pRenderingParams_->AddRef();
	AddRef();
    }

    // add "virtual" to avoid a compiler warning
    virtual ~GdiTextRenderer()
    {
	SafeRelease(&pRenderTarget_);
	SafeRelease(&pRenderingParams_);
    }

    IFACEMETHOD(IsPixelSnappingDisabled)(
	__maybenull void* clientDrawingContext,
	__out BOOL* isDisabled)
    {
	*isDisabled = FALSE;
	return S_OK;
    }

    IFACEMETHOD(GetCurrentTransform)(
	__maybenull void* clientDrawingContext,
	__out DWRITE_MATRIX* transform)
    {
	// forward the render target's transform
	pRenderTarget_->GetCurrentTransform(transform);
	return S_OK;
    }

    IFACEMETHOD(GetPixelsPerDip)(
	__maybenull void* clientDrawingContext,
	__out FLOAT* pixelsPerDip)
    {
	*pixelsPerDip = pRenderTarget_->GetPixelsPerDip();
	return S_OK;
    }

    IFACEMETHOD(DrawGlyphRun)(
	__maybenull void* clientDrawingContext,
	FLOAT baselineOriginX,
	FLOAT baselineOriginY,
	DWRITE_MEASURING_MODE measuringMode,
	__in DWRITE_GLYPH_RUN const* glyphRun,
	__in DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
	IUnknown* clientDrawingEffect)
    {
	HRESULT hr = S_OK;

	GdiTextRendererContext *context =
	    reinterpret_cast<GdiTextRendererContext*>(clientDrawingContext);

	AdjustedGlyphRun adjustedGlyphRun(glyphRun, context->cellWidth);

	// Pass on the drawing call to the render target to do the real work.
	RECT dirtyRect = {0};

	hr = pRenderTarget_->DrawGlyphRun(
		baselineOriginX + context->offsetX,
		baselineOriginY,
		measuringMode,
		&adjustedGlyphRun,
		pRenderingParams_,
		context->color,
		&dirtyRect);

	context->offsetX += adjustedGlyphRun.getDelta();

	return hr;
    }

    IFACEMETHOD(DrawUnderline)(
	__maybenull void* clientDrawingContext,
	FLOAT baselineOriginX,
	FLOAT baselineOriginY,
	__in DWRITE_UNDERLINE const* underline,
	IUnknown* clientDrawingEffect)
    {
	return E_NOTIMPL;
    }

    IFACEMETHOD(DrawStrikethrough)(
	__maybenull void* clientDrawingContext,
	FLOAT baselineOriginX,
	FLOAT baselineOriginY,
	__in DWRITE_STRIKETHROUGH const* strikethrough,
	IUnknown* clientDrawingEffect)
    {
	return E_NOTIMPL;
    }

    IFACEMETHOD(DrawInlineObject)(
	__maybenull void* clientDrawingContext,
	FLOAT originX,
	FLOAT originY,
	IDWriteInlineObject* inlineObject,
	BOOL isSideways,
	BOOL isRightToLeft,
	IUnknown* clientDrawingEffect)
    {
	return E_NOTIMPL;
    }

public:
    IFACEMETHOD_(unsigned long, AddRef) ()
    {
	return InterlockedIncrement(&cRefCount_);
    }

    IFACEMETHOD_(unsigned long, Release) ()
    {
	long newCount = InterlockedDecrement(&cRefCount_);

	if (newCount == 0)
	{
	    delete this;
	    return 0;
	}
	return newCount;
    }

    IFACEMETHOD(QueryInterface)(
	IID const& riid,
	void** ppvObject)
    {
	if (__uuidof(IDWriteTextRenderer) == riid)
	{
	    *ppvObject = this;
	}
	else if (__uuidof(IDWritePixelSnapping) == riid)
	{
	    *ppvObject = this;
	}
	else if (__uuidof(IUnknown) == riid)
	{
	    *ppvObject = this;
	}
	else
	{
	    *ppvObject = NULL;
	    return E_FAIL;
	}

	return S_OK;
    }

private:
    long cRefCount_;
    IDWriteBitmapRenderTarget* pRenderTarget_;
    IDWriteRenderingParams* pRenderingParams_;
};

struct DWriteContext {
    FLOAT mDpiScaleX;
    FLOAT mDpiScaleY;
    bool mDrawing;

    ID2D1Factory *mD2D1Factory;

    ID2D1DCRenderTarget *mRT;
    ID2D1SolidColorBrush *mBrush;

    IDWriteFactory *mDWriteFactory;
    IDWriteGdiInterop *mGdiInterop;
    IDWriteRenderingParams *mRenderingParams;
    IDWriteTextFormat *mTextFormat;

    HFONT mLastHFont;
    DWRITE_FONT_WEIGHT mFontWeight;
    DWRITE_FONT_STYLE mFontStyle;

    D2D1_TEXT_ANTIALIAS_MODE mTextAntialiasMode;

    // METHODS

    DWriteContext();

    virtual ~DWriteContext();

    HRESULT SetLOGFONT(const LOGFONTW &logFont, float fontSize);

    void SetFont(HFONT hFont);

    void SetFont(const LOGFONTW &logFont);

    void DrawText(HDC hdc, const WCHAR* text, int len,
	int x, int y, int w, int h, int cellWidth, COLORREF color);

    float PixelsToDipsX(int x);

    float PixelsToDipsY(int y);

    void SetRenderingParams(
	    const DWriteRenderingParams *params);

    DWriteRenderingParams *GetRenderingParams(
	    DWriteRenderingParams *params);
};

DWriteContext::DWriteContext() :
    mDpiScaleX(1.f),
    mDpiScaleY(1.f),
    mDrawing(false),
    mD2D1Factory(NULL),
    mRT(NULL),
    mBrush(NULL),
    mDWriteFactory(NULL),
    mGdiInterop(NULL),
    mRenderingParams(NULL),
    mTextFormat(NULL),
    mLastHFont(NULL),
    mFontWeight(DWRITE_FONT_WEIGHT_NORMAL),
    mFontStyle(DWRITE_FONT_STYLE_NORMAL),
    mTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT)
{
    HRESULT hr;

    HDC screen = ::GetDC(0);
    mDpiScaleX = ::GetDeviceCaps(screen, LOGPIXELSX) / 96.0f;
    mDpiScaleY = ::GetDeviceCaps(screen, LOGPIXELSY) / 96.0f;
    ::ReleaseDC(0, screen);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
	    __uuidof(ID2D1Factory), NULL,
	    reinterpret_cast<void**>(&mD2D1Factory));
    _RPT2(_CRT_WARN, "D2D1CreateFactory: hr=%p p=%p\n", hr, mD2D1Factory);

    if (SUCCEEDED(hr))
    {
	D2D1_RENDER_TARGET_PROPERTIES props = {
	    D2D1_RENDER_TARGET_TYPE_DEFAULT,
	    { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
	    0, 0,
	    D2D1_RENDER_TARGET_USAGE_NONE,
	    D2D1_FEATURE_LEVEL_DEFAULT
	};
	hr = mD2D1Factory->CreateDCRenderTarget(&props, &mRT);
	_RPT2(_CRT_WARN, "CreateDCRenderTarget: hr=%p p=%p\n", hr, mRT);
    }

    if (SUCCEEDED(hr))
    {
	hr = mRT->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Black),
		&mBrush);
	_RPT2(_CRT_WARN, "CreateSolidColorBrush: hr=%p p=%p\n", hr, mBrush);
    }

    if (SUCCEEDED(hr))
    {
	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&mDWriteFactory));
	_RPT2(_CRT_WARN, "DWriteCreateFactory: hr=%p p=%p\n", hr,
		mDWriteFactory);
    }

    if (SUCCEEDED(hr))
    {
	hr = mDWriteFactory->GetGdiInterop(&mGdiInterop);
	_RPT2(_CRT_WARN, "GetGdiInterop: hr=%p p=%p\n", hr, mGdiInterop);
    }

    if (SUCCEEDED(hr))
    {
	hr = mDWriteFactory->CreateRenderingParams(&mRenderingParams);
	_RPT2(_CRT_WARN, "CreateRenderingParams: hr=%p p=%p\n", hr,
		mRenderingParams);
    }
}

DWriteContext::~DWriteContext()
{
    SafeRelease(&mTextFormat);
    SafeRelease(&mRenderingParams);
    SafeRelease(&mGdiInterop);
    SafeRelease(&mDWriteFactory);
    SafeRelease(&mBrush);
    SafeRelease(&mRT);
    SafeRelease(&mD2D1Factory);
}

    HRESULT
DWriteContext::SetLOGFONT(const LOGFONTW &logFont, float fontSize)
{
    // Most of this function is copy from: http://msdn.microsoft.com/en-us/library/windows/desktop/dd941783(v=vs.85).aspx
    HRESULT hr = S_OK;

    IDWriteFont *font = NULL;
    IDWriteFontFamily *fontFamily = NULL;
    IDWriteLocalizedStrings *localizedFamilyNames = NULL;

    if (SUCCEEDED(hr))
    {
	hr = mGdiInterop->CreateFontFromLOGFONT(&logFont, &font);
    }

    // Get the font family to which this font belongs.
    if (SUCCEEDED(hr))
    {
	hr = font->GetFontFamily(&fontFamily);
    }

    // Get the family names. This returns an object that encapsulates one or
    // more names with the same meaning but in different languages.
    if (SUCCEEDED(hr))
    {
	hr = fontFamily->GetFamilyNames(&localizedFamilyNames);
    }

    // Get the family name at index zero. If we were going to display the name
    // we'd want to try to find one that matched the use locale, but for
    // purposes of creating a text format object any language will do.

    wchar_t familyName[100];
    if (SUCCEEDED(hr))
    {
	hr = localizedFamilyNames->GetString(0, familyName,
		ARRAYSIZE(familyName));
    }

    if (SUCCEEDED(hr))
    {
	// If no font size was passed in use the lfHeight of the LOGFONT.
	if (fontSize == 0)
	{
	    // Convert from pixels to DIPs.
	    fontSize = PixelsToDipsY(logFont.lfHeight);
	    if (fontSize < 0)
	    {
		// Negative lfHeight represents the size of the em unit.
		fontSize = -fontSize;
	    }
	    else
	    {
		// Positive lfHeight represents the cell height (ascent +
		// descent).
		DWRITE_FONT_METRICS fontMetrics;
		font->GetMetrics(&fontMetrics);

		// Convert the cell height (ascent + descent) from design units
		// to ems.
		float cellHeight = static_cast<float>(
			fontMetrics.ascent + fontMetrics.descent)
					       / fontMetrics.designUnitsPerEm;

		// Divide the font size by the cell height to get the font em
		// size.
		fontSize /= cellHeight;
	    }
	}
    }

    // The text format includes a locale name. Ideally, this would be the
    // language of the text, which may or may not be the same as the primary
    // language of the user. However, for our purposes the user locale will do.
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    if (SUCCEEDED(hr))
    {
	if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) == 0)
	    hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
	// Create the text format object.
	hr = mDWriteFactory->CreateTextFormat(
		familyName,
		NULL, // no custom font collection
		font->GetWeight(),
		font->GetStyle(),
		font->GetStretch(),
		fontSize,
		localeName,
		&mTextFormat);
    }

    if (SUCCEEDED(hr))
    {
	mFontWeight = static_cast<DWRITE_FONT_WEIGHT>(logFont.lfWeight);
	mFontStyle = logFont.lfItalic ? DWRITE_FONT_STYLE_ITALIC
	    : DWRITE_FONT_STYLE_NORMAL;
    }

    SafeRelease(&localizedFamilyNames);
    SafeRelease(&fontFamily);
    SafeRelease(&font);

    return hr;
}

    void
DWriteContext::SetFont(HFONT hFont)
{
    if (mLastHFont != hFont)
    {
	LOGFONTW lf;
	if (GetObjectW(hFont, sizeof(lf), &lf))
	{
	    SetFont(lf);
	    mLastHFont = hFont;
	}
    }
}

    void
DWriteContext::SetFont(const LOGFONTW &logFont)
{
    SafeRelease(&mTextFormat);
    mLastHFont = NULL;

    HRESULT hr = SetLOGFONT(logFont, 0.f);

    if (SUCCEEDED(hr))
	hr = mTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    if (SUCCEEDED(hr))
	hr = mTextFormat->SetParagraphAlignment(
		DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (SUCCEEDED(hr))
	hr = mTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
}

    void
DWriteContext::DrawText(HDC hdc, const WCHAR* text, int len,
	int x, int y, int w, int h, int cellWidth, COLORREF color)
{
    HRESULT hr = S_OK;
    IDWriteBitmapRenderTarget *bmpRT = NULL;

    // Skip when any fonts are not set.
    if (mTextFormat == NULL)
	return;

    // Check possibility of zero divided error.
    if (cellWidth == 0 || mDpiScaleX == 0.0f || mDpiScaleY == 0.0f)
	return;

    if (SUCCEEDED(hr))
	hr = mGdiInterop->CreateBitmapRenderTarget(hdc, w, h, &bmpRT);

    if (SUCCEEDED(hr))
    {
	IDWriteTextLayout *textLayout = NULL;

	HDC memdc = bmpRT->GetMemoryDC();
	BitBlt(memdc, 0, 0, w, h, hdc, x, y, SRCCOPY);

	hr = mDWriteFactory->CreateGdiCompatibleTextLayout(
		text, len, mTextFormat, PixelsToDipsX(w),
		PixelsToDipsY(h), mDpiScaleX, NULL, TRUE, &textLayout);

	if (SUCCEEDED(hr))
	{
	    DWRITE_TEXT_RANGE textRange = { 0, (UINT32)len };
	    textLayout->SetFontWeight(mFontWeight, textRange);
	    textLayout->SetFontStyle(mFontStyle, textRange);
	}

	if (SUCCEEDED(hr))
	{
	    GdiTextRenderer *renderer = new GdiTextRenderer(bmpRT,
		    mRenderingParams);
	    GdiTextRendererContext data = {
		color,
		PixelsToDipsX(cellWidth),
		0.0f
	    };
	    textLayout->Draw(&data, renderer, 0, 0);
	    SafeRelease(&renderer);
	}

	BitBlt(hdc, x, y, w, h, memdc, 0, 0, SRCCOPY);

	SafeRelease(&textLayout);
    }

    SafeRelease(&bmpRT);
}

    float
DWriteContext::PixelsToDipsX(int x)
{
    return x / mDpiScaleX;
}

    float
DWriteContext::PixelsToDipsY(int y)
{
    return y / mDpiScaleY;
}

    void
DWriteContext::SetRenderingParams(
	const DWriteRenderingParams *params)
{
    if (mDWriteFactory == NULL)
	return;

    IDWriteRenderingParams *renderingParams = NULL;
    D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode =
	D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
    HRESULT hr;
    if (params != NULL)
    {
	hr = mDWriteFactory->CreateCustomRenderingParams(params->gamma,
		params->enhancedContrast, params->clearTypeLevel,
		ToPixelGeometry(params->pixelGeometry),
		ToRenderingMode(params->renderingMode), &renderingParams);
	textAntialiasMode = ToTextAntialiasMode(params->textAntialiasMode);
    }
    else
	hr = mDWriteFactory->CreateRenderingParams(&renderingParams);
    if (SUCCEEDED(hr) && renderingParams != NULL)
    {
	SafeRelease(&mRenderingParams);
	mRenderingParams = renderingParams;
	mTextAntialiasMode = textAntialiasMode;
    }
}

    DWriteRenderingParams *
DWriteContext::GetRenderingParams(
	DWriteRenderingParams *params)
{
    if (params != NULL && mRenderingParams != NULL)
    {
	params->gamma = mRenderingParams->GetGamma();
	params->enhancedContrast = mRenderingParams->GetEnhancedContrast();
	params->clearTypeLevel = mRenderingParams->GetClearTypeLevel();
	params->pixelGeometry = ToInt(mRenderingParams->GetPixelGeometry());
	params->renderingMode = ToInt(mRenderingParams->GetRenderingMode());
	params->textAntialiasMode = mTextAntialiasMode;
    }
    return params;
}

////////////////////////////////////////////////////////////////////////////
// PUBLIC C INTERFACES

    void
DWrite_Init(void)
{
#ifdef DYNAMIC_DIRECTX
    // Load libraries.
    hD2D1DLL = vimLoadLib(const_cast<char*>("d2d1.dll"));
    hDWriteDLL = vimLoadLib(const_cast<char*>("dwrite.dll"));
    if (hD2D1DLL == NULL || hDWriteDLL == NULL)
    {
	DWrite_Final();
	return;
    }
    // Get address of procedures.
    pGetUserDefaultLocaleName = (PGETUSERDEFAULTLOCALENAME)GetProcAddress(
	    GetModuleHandle("kernel32.dll"), "GetUserDefaultLocaleName");
    pD2D1CreateFactory = (PD2D1CREATEFACTORY)GetProcAddress(hD2D1DLL,
	    "D2D1CreateFactory");
    pDWriteCreateFactory = (PDWRITECREATEFACTORY)GetProcAddress(hDWriteDLL,
	    "DWriteCreateFactory");
#endif
}

    void
DWrite_Final(void)
{
#ifdef DYNAMIC_DIRECTX
    pGetUserDefaultLocaleName = NULL;
    pD2D1CreateFactory = NULL;
    pDWriteCreateFactory = NULL;
    unload(hDWriteDLL);
    unload(hD2D1DLL);
#endif
}

    DWriteContext *
DWriteContext_Open(void)
{
#ifdef DYNAMIC_DIRECTX
    if (pGetUserDefaultLocaleName == NULL || pD2D1CreateFactory == NULL
	    || pDWriteCreateFactory == NULL)
	return NULL;
#endif
    return new DWriteContext();
}

    void
DWriteContext_BeginDraw(DWriteContext *ctx)
{
    if (ctx != NULL && ctx->mRT != NULL)
    {
	ctx->mRT->BeginDraw();
	ctx->mRT->SetTransform(D2D1::IdentityMatrix());
	ctx->mDrawing = true;
    }
}

    void
DWriteContext_BindDC(DWriteContext *ctx, HDC hdc, RECT *rect)
{
    if (ctx != NULL && ctx->mRT != NULL)
    {
	ctx->mRT->BindDC(hdc, rect);
	ctx->mRT->SetTextAntialiasMode(ctx->mTextAntialiasMode);
    }
}

    void
DWriteContext_SetFont(DWriteContext *ctx, HFONT hFont)
{
    if (ctx != NULL)
    {
	ctx->SetFont(hFont);
    }
}

    void
DWriteContext_DrawText(
	DWriteContext *ctx,
	HDC hdc,
	const WCHAR* text,
	int len,
	int x,
	int y,
	int w,
	int h,
	int cellWidth,
	COLORREF color)
{
    if (ctx != NULL)
	ctx->DrawText(hdc, text, len, x, y, w, h, cellWidth, color);
}

    void
DWriteContext_EndDraw(DWriteContext *ctx)
{
    if (ctx != NULL && ctx->mRT != NULL)
    {
	ctx->mRT->EndDraw();
	ctx->mDrawing = false;
    }
}

    void
DWriteContext_Close(DWriteContext *ctx)
{
    delete ctx;
}

    void
DWriteContext_SetRenderingParams(
	DWriteContext *ctx,
	const DWriteRenderingParams *params)
{
    if (ctx != NULL)
	ctx->SetRenderingParams(params);
}

    DWriteRenderingParams *
DWriteContext_GetRenderingParams(
	DWriteContext *ctx,
	DWriteRenderingParams *params)
{
    if (ctx != NULL)
	return ctx->GetRenderingParams(params);
    else
	return NULL;
}
