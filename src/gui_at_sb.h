/* vi:set ts=8 sts=4 sw=4 noet: */
/* MODIFIED ATHENA SCROLLBAR (USING ARROWHEADS AT ENDS OF TRAVEL) */
/* Modifications Copyright 1992 by Mitch Trachtenberg		  */
/* Rights, permissions, and disclaimer of warranty are as in the  */
/* DEC and MIT notice below.  See usage warning in .c file.	  */
/*
 * $XConsortium: ScrollbarP.h,v 1.3 94/04/17 20:12:42 jim Exp $
 */


/***********************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef _Scrollbar_h
#define _Scrollbar_h

/****************************************************************
 *
 * Scrollbar Widget
 *
 ****************************************************************/

#include <X11/IntrinsicP.h>
#include <X11/Xaw/SimpleP.h>
#include <X11/Xmu/Converters.h>

/*
 * Most things we need are in StringDefs.h
 */
#define XtCMinimumThumb		"MinimumThumb"
#define XtCShown		"Shown"
#define XtCTopOfThumb		"TopOfThumb"
#define XtCMaxOfThumb		"MaxOfThumb"
#define XtCShadowWidth		"ShadowWidth"
#define XtCTopShadowPixel	"TopShadowPixel"
#define XtCBottomShadowPixel	"BottomShadowPixel"
#define XtCLimitThumb		"LimitThumb"

#define XtNminimumThumb		"minimumThumb"
#define XtNtopOfThumb		"topOfThumb"
#define XtNmaxOfThumb		"maxOfThumb"
#define XtNshadowWidth		"shadowWidth"
#define XtNtopShadowPixel	"topShadowPixel"
#define XtNbottomShadowPixel	"bottomShadowPixel"
#define XtNlimitThumb		"limitThumb"

typedef struct _ScrollbarRec	  *ScrollbarWidget;
typedef struct _ScrollbarClassRec *ScrollbarWidgetClass;

extern WidgetClass vim_scrollbarWidgetClass;

extern void vim_XawScrollbarSetThumb(Widget, double, double, double);

typedef struct
{
     /* public */
    Pixel	  foreground;	/* thumb foreground color */
    XtOrientation orientation;	/* horizontal or vertical */
    XtCallbackList scrollProc;	/* proportional scroll */
    XtCallbackList thumbProc;	/* jump (to position) scroll */
    XtCallbackList jumpProc;	/* same as thumbProc but pass data by ref */
    Pixmap	  thumb;	/* thumb color */
    float	  top;		/* What percent is above the win's top */
    float	  shown;	/* What percent is shown in the win */
    float	  max;		/* Maximum value for top */
    Dimension	  length;	/* either height or width */
    Dimension	  thickness;	/* either width or height */
    Dimension	  min_thumb;	/* minimum size for the thumb. */

     /* private */
    XtIntervalId  timer_id;	/* autorepeat timer; remove on destruction */
    char	  scroll_mode;	/* see below */
    float	  scroll_off;	/* offset from event to top of thumb */
    GC		  gc;		/* a (shared) gc */
    Position	  topLoc;	/* Pixel that corresponds to top */
    Dimension	  shownLength;	/* Num pixels corresponding to shown */

    /* From 3d widget */
    Dimension	shadow_width;
    Pixel	top_shadow_pixel;
    Pixel	bot_shadow_pixel;
    Bool	limit_thumb;	/* limit thumb to inside scrollbar */
    int		top_shadow_contrast;
    int		bot_shadow_contrast;
    GC		top_shadow_GC;
    GC		bot_shadow_GC;
} ScrollbarPart;

#define SMODE_NONE		0
#define SMODE_CONT		1
#define SMODE_PAGE_UP		2
#define SMODE_PAGE_DOWN		3
#define SMODE_LINE_UP		4
#define SMODE_LINE_DOWN		5

#define ONE_LINE_DATA		1
#define ONE_PAGE_DATA		10
#define END_PAGE_DATA		9999

typedef struct _ScrollbarRec {
    CorePart		core;
    SimplePart		simple;
    ScrollbarPart	scrollbar;
} ScrollbarRec;

typedef struct {int empty;} ScrollbarClassPart;

typedef struct _ScrollbarClassRec {
    CoreClassPart		core_class;
    SimpleClassPart		simple_class;
    ScrollbarClassPart		scrollbar_class;
} ScrollbarClassRec;

extern ScrollbarClassRec vim_scrollbarClassRec;

#endif /* _Scrollbar_h */
