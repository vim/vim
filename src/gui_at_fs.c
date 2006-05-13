/* vi:set ts=8 sts=4 sw=4: */

/*
 * Copyright 1989 Software Research Associates, Inc., Tokyo, Japan
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Software Research Associates not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Software Research Associates
 * makes no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * SOFTWARE RESEARCH ASSOCIATES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL SOFTWARE RESEARCH ASSOCIATES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Erik M. van der Poel
 *	   Software Research Associates, Inc., Tokyo, Japan
 *	   erik@sra.co.jp
 */
/*
 * Author's addresses:
 *	erik@sra.co.jp
 *	erik%sra.co.jp@uunet.uu.net
 *	erik%sra.co.jp@mcvax.uucp
 *	try junet instead of co.jp
 *	Erik M. van der Poel
 *	Software Research Associates, Inc.
 *	1-1-1 Hirakawa-cho, Chiyoda-ku
 *	Tokyo 102 Japan. TEL +81-3-234-2692
 */

/*
 * Heavely modified for Vim by Bram Moolenaar
 */

#include "vim.h"

/* Only include this when using the file browser */

#ifdef FEAT_BROWSE

/* Weird complication: for "make lint" Text.h doesn't combine with Xm.h */
#if defined(FEAT_GUI_MOTIF) && defined(FMT8BIT)
# undef FMT8BIT
#endif

#ifndef FEAT_GUI_NEXTAW
# include "gui_at_sb.h"
#endif

/***************** SFinternal.h */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#ifdef FEAT_GUI_NEXTAW
# include <X11/neXtaw/Text.h>
# include <X11/neXtaw/AsciiText.h>
# include <X11/neXtaw/Scrollbar.h>
#else
# include <X11/Xaw/Text.h>
# include <X11/Xaw/AsciiText.h>
#endif

#define SEL_FILE_CANCEL		-1
#define SEL_FILE_OK		0
#define SEL_FILE_NULL		1
#define SEL_FILE_TEXT		2

#define SF_DO_SCROLL		1
#define SF_DO_NOT_SCROLL	0

typedef struct
{
    int		statDone;
    char	*real;
    char	*shown;
} SFEntry;

typedef struct
{
    char	*dir;
    char	*path;
    SFEntry	*entries;
    int		nEntries;
    int		vOrigin;
    int		nChars;
    int		hOrigin;
    int		changed;
    int		beginSelection;
    int		endSelection;
    time_t	mtime;
} SFDir;

static char	SFstartDir[MAXPATHL],
		SFcurrentPath[MAXPATHL],
		SFcurrentDir[MAXPATHL];

static Widget	selFile,
		selFileField,
		selFileForm,
		selFileHScroll,
		selFileHScrolls[3],
		selFileLists[3],
		selFileOK,
		selFileCancel,
		selFilePrompt,
		selFileVScrolls[3];

static Display	*SFdisplay;

static int	SFcharWidth, SFcharAscent, SFcharHeight;

static SFDir	*SFdirs = NULL;

static int	SFdirEnd;
static int	SFdirPtr;

static Pixel	SFfore, SFback;

static Atom	SFwmDeleteWindow;

static XSegment SFsegs[2], SFcompletionSegs[2];

static XawTextPosition SFtextPos;

static int	SFupperX, SFlowerY, SFupperY;

static int	SFtextX, SFtextYoffset;

static int	SFentryWidth, SFentryHeight;

static int	SFlineToTextH = 3;
static int	SFlineToTextV = 3;

static int	SFbesideText = 3;
static int	SFaboveAndBelowText = 2;

static int	SFcharsPerEntry = 15;

static int	SFlistSize = 10;

static int	SFcurrentInvert[3] = { -1, -1, -1 };

static int	SFworkProcAdded = 0;

static XtAppContext SFapp;

static int	SFpathScrollWidth, SFvScrollHeight, SFhScrollWidth;

#ifdef FEAT_XFONTSET
static char	SFtextBuffer[MAXPATHL*sizeof(wchar_t)];
#else
static char	SFtextBuffer[MAXPATHL];
#endif

static int	SFbuttonPressed = 0;

static XtIntervalId SFdirModTimerId;

static int	(*SFfunc)();

static int	SFstatus = SEL_FILE_NULL;

/***************** static functions */

static void SFsetText __ARGS((char *path));
static void SFtextChanged __ARGS((void));
static char *SFgetText __ARGS((void));
static void SFupdatePath __ARGS((void));
static int SFgetDir __ARGS((SFDir *dir));
static void SFdrawLists __ARGS((int doScroll));
static void SFdrawList __ARGS((int n, int doScroll));
static void SFclearList __ARGS((int n, int doScroll));
static void SFbuttonPressList __ARGS((Widget w, int n, XButtonPressedEvent *event));
static void SFbuttonReleaseList __ARGS((Widget w, int n, XButtonReleasedEvent *event));
static void SFdirModTimer __ARGS((XtPointer cl, XtIntervalId *id));
static char SFstatChar __ARGS((struct stat *statBuf));
static void SFdrawStrings __ARGS((Window w, SFDir *dir, int from, int to));
static int SFnewInvertEntry __ARGS((int n, XMotionEvent *event));
static void SFinvertEntry __ARGS((int n));
static void SFenterList __ARGS((Widget w, int n, XEnterWindowEvent *event));
static void SFleaveList __ARGS((Widget w, int n, XEvent *event));
static void SFmotionList __ARGS((Widget w, int n, XMotionEvent *event));
static void SFvFloatSliderMovedCallback __ARGS((Widget w, XtPointer n, XtPointer fnew));
static void SFvSliderMovedCallback __ARGS((Widget w, int n, int nw));
static void SFvAreaSelectedCallback __ARGS((Widget w, XtPointer n, XtPointer pnew));
static void SFhSliderMovedCallback __ARGS((Widget w, XtPointer n, XtPointer nw));
static void SFhAreaSelectedCallback __ARGS((Widget w, XtPointer n, XtPointer pnew));
static void SFpathSliderMovedCallback __ARGS((Widget w, XtPointer client_data, XtPointer nw));
static void SFpathAreaSelectedCallback __ARGS((Widget w, XtPointer client_data, XtPointer pnew));
static Boolean SFworkProc __ARGS((void));
static int SFcompareEntries __ARGS((const void *p, const void *q));
static void SFprepareToReturn __ARGS((void));
static void SFcreateWidgets __ARGS((Widget toplevel, char *prompt, char *ok, char *cancel));
static void SFsetColors __ARGS((guicolor_T bg, guicolor_T fg, guicolor_T scroll_bg, guicolor_T scrollfg));

/***************** xstat.h */

#ifndef S_IXUSR
# define S_IXUSR 0100
#endif
#ifndef S_IXGRP
# define S_IXGRP 0010
#endif
#ifndef S_IXOTH
# define S_IXOTH 0001
#endif

#define S_ISXXX(m) ((m) & (S_IXUSR | S_IXGRP | S_IXOTH))

/***************** Path.c */

#include <pwd.h>

typedef struct
{
    char	*name;
    char	*dir;
} SFLogin;

static int	SFdoNotTouchDirPtr = 0;

static int	SFdoNotTouchVorigin = 0;

static SFDir	SFrootDir, SFhomeDir;

static SFLogin	*SFlogins;

static int	SFtwiddle = 0;

static int SFchdir __ARGS((char *path));

    static int
SFchdir(path)
    char	*path;
{
    int		result;

    result = 0;

    if (strcmp(path, SFcurrentDir))
    {
	result = mch_chdir(path);
	if (!result)
	    (void) strcpy(SFcurrentDir, path);
    }

    return result;
}

static void SFfree __ARGS((int i));

    static void
SFfree(i)
    int	i;
{
    SFDir	*dir;
    int		j;

    dir = &(SFdirs[i]);

    for (j = dir->nEntries - 1; j >= 0; j--)
    {
	if (dir->entries[j].shown != dir->entries[j].real)
	    XtFree(dir->entries[j].shown);
	XtFree(dir->entries[j].real);
    }

    XtFree((char *)dir->entries);
    XtFree(dir->dir);

    dir->dir = NULL;
}

static void SFstrdup __ARGS((char **s1, char *s2));

    static void
SFstrdup(s1, s2)
    char	**s1;
    char	*s2;
{
    *s1 = strcpy(XtMalloc((unsigned)(strlen(s2) + 1)), s2);
}

static void SFunreadableDir __ARGS((SFDir *dir));

    static void
SFunreadableDir(dir)
    SFDir	*dir;
{
    char	*cannotOpen = _("<cannot open> ");

    dir->entries = (SFEntry *) XtMalloc(sizeof(SFEntry));
    dir->entries[0].statDone = 1;
    SFstrdup(&dir->entries[0].real, cannotOpen);
    dir->entries[0].shown = dir->entries[0].real;
    dir->nEntries = 1;
    dir->nChars = strlen(cannotOpen);
}

static void SFreplaceText __ARGS((SFDir *dir, char *str));

    static void
SFreplaceText(dir, str)
    SFDir	*dir;
    char	*str;
{
    int	len;

    *(dir->path) = 0;
    len = strlen(str);
    if (str[len - 1] == '/')
	(void) strcat(SFcurrentPath, str);
    else
	(void) strncat(SFcurrentPath, str, len - 1);
    if (strncmp(SFcurrentPath, SFstartDir, strlen(SFstartDir)))
	SFsetText(SFcurrentPath);
    else
	SFsetText(&(SFcurrentPath[strlen(SFstartDir)]));

    SFtextChanged();
}

static void SFexpand __ARGS((char *str));

    static void
SFexpand(str)
    char	*str;
{
    int		len;
    int		cmp;
    char	*name, *growing;
    SFDir	*dir;
    SFEntry	*entry, *max;

    len = strlen(str);

    dir = &(SFdirs[SFdirEnd - 1]);

    if (dir->beginSelection == -1)
    {
	SFstrdup(&str, str);
	SFreplaceText(dir, str);
	XtFree(str);
	return;
    }
    else if (dir->beginSelection == dir->endSelection)
    {
	SFreplaceText(dir, dir->entries[dir->beginSelection].shown);
	return;
    }

    max = &(dir->entries[dir->endSelection + 1]);

    name = dir->entries[dir->beginSelection].shown;
    SFstrdup(&growing, name);

    cmp = 0;
    while (!cmp)
    {
	entry = &(dir->entries[dir->beginSelection]);
	while (entry < max)
	{
	    if ((cmp = strncmp(growing, entry->shown, len)))
		break;
	    entry++;
	}
	len++;
    }

    /*
     * SFreplaceText() expects filename
     */
    growing[len - 2] = ' ';

    growing[len - 1] = 0;
    SFreplaceText(dir, growing);
    XtFree(growing);
}

static int SFfindFile __ARGS((SFDir *dir, char *str));

    static int
SFfindFile(dir, str)
    SFDir	*dir;
    char	*str;
{
    int		i, last, max;
    char	*name, save;
    SFEntry	*entries;
    int		len;
    int		begin, end;
    int		result;

    len = strlen(str);

    if (str[len - 1] == ' ')
    {
	SFexpand(str);
	return 1;
    }
    else if (str[len - 1] == '/')
	len--;

    max = dir->nEntries;

    entries = dir->entries;

    i = 0;
    while (i < max)
    {
	name = entries[i].shown;
	last = strlen(name) - 1;
	save = name[last];
	name[last] = 0;

	result = strncmp(str, name, len);

	name[last] = save;
	if (result <= 0)
	    break;
	i++;
    }
    begin = i;
    while (i < max)
    {
	name = entries[i].shown;
	last = strlen(name) - 1;
	save = name[last];
	name[last] = 0;

	result = strncmp(str, name, len);

	name[last] = save;
	if (result)
	    break;
	i++;
    }
    end = i;

    if (begin != end)
    {
	if ((dir->beginSelection != begin) || (dir->endSelection != end - 1))
	{
	    dir->changed = 1;
	    dir->beginSelection = begin;
	    if (str[strlen(str) - 1] == '/')
		dir->endSelection = begin;
	    else
		dir->endSelection = end - 1;
	}
    }
    else if (dir->beginSelection != -1)
    {
	dir->changed = 1;
	dir->beginSelection = -1;
	dir->endSelection = -1;
    }

    if (SFdoNotTouchVorigin
	    || ((begin > dir->vOrigin) && (end < dir->vOrigin + SFlistSize)))
    {
	SFdoNotTouchVorigin = 0;
	return 0;
    }

    i = begin - 1;
    if (i > max - SFlistSize)
	i = max - SFlistSize;
    if (i < 0)
	i = 0;

    if (dir->vOrigin != i)
    {
	dir->vOrigin = i;
	dir->changed = 1;
    }

    return 0;
}

static void SFunselect __ARGS((void));

    static void
SFunselect()
{
    SFDir	*dir;

    dir = &(SFdirs[SFdirEnd - 1]);
    if (dir->beginSelection != -1)
	dir->changed = 1;
    dir->beginSelection = -1;
    dir->endSelection = -1;
}

static int SFcompareLogins __ARGS((const void *p, const void *q));

    static int
SFcompareLogins(p, q)
    const void *p, *q;
{
    return strcmp(((SFLogin *)p)->name, ((SFLogin *)q)->name);
}

static void SFgetHomeDirs __ARGS((void));

    static void
SFgetHomeDirs()
{
    struct	passwd	*pw;
    int		Alloc;
    int		i;
    SFEntry	*entries = NULL;
    int		len;
    int		maxChars;

    Alloc = 1;
    i = 1;
    entries = (SFEntry *)XtMalloc(sizeof(SFEntry));
    SFlogins = (SFLogin *)XtMalloc(sizeof(SFLogin));
    entries[0].real = XtMalloc(3);
    (void) strcpy(entries[0].real, "~");
    entries[0].shown = entries[0].real;
    entries[0].statDone = 1;
    SFlogins[0].name = "";
    pw = getpwuid((int) getuid());
    SFstrdup(&SFlogins[0].dir, pw ? pw->pw_dir : "/");
    maxChars = 0;

    (void) setpwent();

    while ((pw = getpwent()) && (*(pw->pw_name)))
    {
	if (i >= Alloc)
	{
	    Alloc *= 2;
	    entries = (SFEntry *) XtRealloc((char *)entries,
					 (unsigned)(Alloc * sizeof(SFEntry)));
	    SFlogins = (SFLogin *) XtRealloc((char *)SFlogins,
					 (unsigned)(Alloc * sizeof(SFLogin)));
	}
	len = strlen(pw->pw_name);
	entries[i].real = XtMalloc((unsigned) (len + 3));
	(void) strcat(strcpy(entries[i].real, "~"), pw->pw_name);
	entries[i].shown = entries[i].real;
	entries[i].statDone = 1;
	if (len > maxChars)
	    maxChars = len;
	SFstrdup(&SFlogins[i].name, pw->pw_name);
	SFstrdup(&SFlogins[i].dir, pw->pw_dir);
	i++;
    }

    SFhomeDir.dir		= XtMalloc(1);
    SFhomeDir.dir[0]		= 0;
    SFhomeDir.path		= SFcurrentPath;
    SFhomeDir.entries		= entries;
    SFhomeDir.nEntries		= i;
    SFhomeDir.vOrigin		= 0;	/* :-) */
    SFhomeDir.nChars		= maxChars + 2;
    SFhomeDir.hOrigin		= 0;
    SFhomeDir.changed		= 1;
    SFhomeDir.beginSelection	= -1;
    SFhomeDir.endSelection	= -1;

    qsort((char *)entries, (size_t)i, sizeof(SFEntry), SFcompareEntries);
    qsort((char *)SFlogins, (size_t)i, sizeof(SFLogin), SFcompareLogins);

    for (i--; i >= 0; i--)
	(void)strcat(entries[i].real, "/");
}

static int SFfindHomeDir __ARGS((char *begin, char *end));

    static int
SFfindHomeDir(begin, end)
    char	*begin, *end;
{
    char	save;
    char	*theRest;
    int	i;

    save = *end;
    *end = 0;

    for (i = SFhomeDir.nEntries - 1; i >= 0; i--)
    {
	if (!strcmp(SFhomeDir.entries[i].real, begin))
	{
	    *end = save;
	    SFstrdup(&theRest, end);
	    (void) strcat(strcat(strcpy(SFcurrentPath,
					SFlogins[i].dir), "/"), theRest);
	    XtFree(theRest);
	    SFsetText(SFcurrentPath);
	    SFtextChanged();
	    return 1;
	}
    }

    *end = save;

    return 0;
}

    static void
SFupdatePath()
{
    static int	Alloc;
    static int	wasTwiddle = 0;
    char	*begin, *end;
    int		i, j;
    int		prevChange;
    int		SFdirPtrSave, SFdirEndSave;
    SFDir	*dir;

    if (!SFdirs)
    {
	SFdirs = (SFDir *) XtMalloc((Alloc = 10) * sizeof(SFDir));
	dir = &(SFdirs[0]);
	SFstrdup(&dir->dir, "/");
	(void) SFchdir("/");
	(void) SFgetDir(dir);
	for (j = 1; j < Alloc; j++)
	    SFdirs[j].dir = NULL;
	dir->path = SFcurrentPath + 1;
	dir->vOrigin = 0;
	dir->hOrigin = 0;
	dir->changed = 1;
	dir->beginSelection = -1;
	dir->endSelection = -1;
	SFhomeDir.dir = NULL;
    }

    SFdirEndSave = SFdirEnd;
    SFdirEnd = 1;

    SFdirPtrSave = SFdirPtr;
    SFdirPtr = 0;

    begin = NULL;

    if (SFcurrentPath[0] == '~')
    {
	if (!SFtwiddle)
	{
	    SFtwiddle = 1;
	    dir = &(SFdirs[0]);
	    SFrootDir = *dir;
	    if (!SFhomeDir.dir)
		SFgetHomeDirs();
	    *dir = SFhomeDir;
	    dir->changed = 1;
	}
	end = SFcurrentPath;
	SFdoNotTouchDirPtr = 1;
	wasTwiddle = 1;
    }
    else
    {
	if (SFtwiddle)
	{
	    SFtwiddle = 0;
	    dir = &(SFdirs[0]);
	    *dir = SFrootDir;
	    dir->changed = 1;
	}
	end = SFcurrentPath + 1;
    }

    i = 0;

    prevChange = 0;

    while (*end)
    {
	while (*end++ == '/')
	    ;
	end--;
	begin = end;
	while ((*end) && (*end++ != '/'))
	    ;
	if ((end - SFcurrentPath <= SFtextPos) && (*(end - 1) == '/'))
	{
	    SFdirPtr = i - 1;
	    if (SFdirPtr < 0)
		SFdirPtr = 0;
	}
	if (*begin)
	{
	    if (*(end - 1) == '/')
	    {
		char save = *end;

		if (SFtwiddle)
		{
		    if (SFfindHomeDir(begin, end))
			return;
		}
		*end = 0;
		i++;
		SFdirEnd++;
		if (i >= Alloc)
		{
		    SFdirs = (SFDir *) XtRealloc((char *) SFdirs,
				    (unsigned)((Alloc *= 2) * sizeof(SFDir)));
		    for (j = Alloc / 2; j < Alloc; j++)
			SFdirs[j].dir = NULL;
		}
		dir = &(SFdirs[i]);
		if ((!(dir->dir)) || prevChange || strcmp(dir->dir, begin))
		{
		    if (dir->dir)
			SFfree(i);
		    prevChange = 1;
		    SFstrdup(&dir->dir, begin);
		    dir->path = end;
		    dir->vOrigin = 0;
		    dir->hOrigin = 0;
		    dir->changed = 1;
		    dir->beginSelection = -1;
		    dir->endSelection = -1;
		    (void)SFfindFile(dir - 1, begin);
		    if (SFchdir(SFcurrentPath) || SFgetDir(dir))
		    {
			SFunreadableDir(dir);
			break;
		    }
		}
		*end = save;
		if (!save)
		    SFunselect();
	    }
	    else
	    {
		if (SFfindFile(&(SFdirs[SFdirEnd-1]), begin))
		    return;
	    }
	}
	else
	    SFunselect();
    }

    if ((end == SFcurrentPath + 1) && (!SFtwiddle))
	SFunselect();

    for (i = SFdirEnd; i < Alloc; i++)
	if (SFdirs[i].dir)
	    SFfree(i);

    if (SFdoNotTouchDirPtr)
    {
	if (wasTwiddle)
	{
	    wasTwiddle = 0;
	    SFdirPtr = SFdirEnd - 2;
	    if (SFdirPtr < 0)
		SFdirPtr = 0;
	}
	else
	    SFdirPtr = SFdirPtrSave;
	SFdoNotTouchDirPtr = 0;
    }

    if ((SFdirPtr != SFdirPtrSave) || (SFdirEnd != SFdirEndSave))
    {
#ifdef FEAT_GUI_NEXTAW
	XawScrollbarSetThumb( selFileHScroll,
		(float) (((double) SFdirPtr) / SFdirEnd),
		(float) (((double) ((SFdirEnd < 3) ? SFdirEnd : 3)) /
			 SFdirEnd));
#else
	vim_XawScrollbarSetThumb( selFileHScroll,
		(float) (((double) SFdirPtr) / SFdirEnd),
		(float) (((double) ((SFdirEnd < 3) ? SFdirEnd : 3)) /
			 SFdirEnd),
		(double)SFdirEnd);
#endif
    }

    if (SFdirPtr != SFdirPtrSave)
	SFdrawLists(SF_DO_SCROLL);
    else
	for (i = 0; i < 3; i++)
	{
	    if (SFdirPtr + i < SFdirEnd)
	    {
		if (SFdirs[SFdirPtr + i].changed)
		{
		    SFdirs[SFdirPtr + i].changed = 0;
		    SFdrawList(i, SF_DO_SCROLL);
		}
	    }
	    else
		SFclearList(i, SF_DO_SCROLL);
	}
}

#ifdef XtNinternational
    static int
WcsLen(p)
    wchar_t *p;
{
    int i = 0;
    while (*p++ != 0)
	i++;
    return i;
}
#endif

    static void
SFsetText(path)
    char	*path;
{
    XawTextBlock	text;

    text.firstPos = 0;
    text.length = strlen(path);
    text.ptr = path;
    text.format = FMT8BIT;

#ifdef XtNinternational
    if (_XawTextFormat((TextWidget)selFileField) == XawFmtWide)
    {
	XawTextReplace(selFileField, (XawTextPosition)0,
				    (XawTextPosition)WcsLen((wchar_t *)&SFtextBuffer[0]), &text);
	XawTextSetInsertionPoint(selFileField,
					   (XawTextPosition)WcsLen((wchar_t *)&SFtextBuffer[0]));
    }
    else
    {
	XawTextReplace(selFileField, (XawTextPosition)0,
				    (XawTextPosition)strlen(SFtextBuffer), &text);
	XawTextSetInsertionPoint(selFileField,
					   (XawTextPosition)strlen(SFtextBuffer));
    }
#else
    XawTextReplace(selFileField, (XawTextPosition)0,
				(XawTextPosition)strlen(SFtextBuffer), &text);
    XawTextSetInsertionPoint(selFileField,
				       (XawTextPosition)strlen(SFtextBuffer));
#endif
}

/* ARGSUSED */
    static void
SFbuttonPressList(w, n, event)
    Widget		w;
    int			n;
    XButtonPressedEvent	*event;
{
    SFbuttonPressed = 1;
}

/* ARGSUSED */
    static void
SFbuttonReleaseList(w, n, event)
    Widget		 w;
    int			 n;
    XButtonReleasedEvent *event;
{
    SFDir	*dir;

    SFbuttonPressed = 0;

    if (SFcurrentInvert[n] != -1)
    {
	if (n < 2)
	    SFdoNotTouchDirPtr = 1;
	SFdoNotTouchVorigin = 1;
	dir = &(SFdirs[SFdirPtr + n]);
	SFreplaceText(dir,
		       dir->entries[dir->vOrigin + SFcurrentInvert[n]].shown);
	SFmotionList(w, n, (XMotionEvent *) event);
    }
}

static int SFcheckDir __ARGS((int n, SFDir *dir));

    static int
SFcheckDir(n, dir)
    int		n;
    SFDir		*dir;
{
    struct stat	statBuf;
    int		i;

    if ((!mch_stat(".", &statBuf)) && (statBuf.st_mtime != dir->mtime))
    {
	/*
	 * If the pointer is currently in the window that we are about
	 * to update, we must warp it to prevent the user from
	 * accidentally selecting the wrong file.
	 */
	if (SFcurrentInvert[n] != -1)
	{
	    XWarpPointer(
		    SFdisplay,
		    None,
		    XtWindow(selFileLists[n]),
		    0,
		    0,
		    0,
		    0,
		    0,
		    0);
	}

	for (i = dir->nEntries - 1; i >= 0; i--)
	{
	    if (dir->entries[i].shown != dir->entries[i].real)
		XtFree(dir->entries[i].shown);
	    XtFree(dir->entries[i].real);
	}
	XtFree((char *) dir->entries);
	if (SFgetDir(dir))
	    SFunreadableDir(dir);
	if (dir->vOrigin > dir->nEntries - SFlistSize)
	    dir->vOrigin = dir->nEntries - SFlistSize;
	if (dir->vOrigin < 0)
	    dir->vOrigin = 0;
	if (dir->hOrigin > dir->nChars - SFcharsPerEntry)
	    dir->hOrigin = dir->nChars - SFcharsPerEntry;
	if (dir->hOrigin < 0)
	    dir->hOrigin = 0;
	dir->beginSelection = -1;
	dir->endSelection = -1;
	SFdoNotTouchVorigin = 1;
	if ((dir + 1)->dir)
	    (void) SFfindFile(dir, (dir + 1)->dir);
	else
	    (void) SFfindFile(dir, dir->path);

	if (!SFworkProcAdded)
	{
	    (void) XtAppAddWorkProc(SFapp, (XtWorkProc)SFworkProc, NULL);
	    SFworkProcAdded = 1;
	}
	return 1;
    }
    return 0;
}

static int SFcheckFiles __ARGS((SFDir *dir));

    static int
SFcheckFiles(dir)
	SFDir	*dir;
{
    int		from, to;
    int		result;
    char	oldc, newc;
    int		i;
    char	*str;
    int		last;
    struct stat	statBuf;

    result = 0;

    from = dir->vOrigin;
    to = dir->vOrigin + SFlistSize;
    if (to > dir->nEntries)
	to = dir->nEntries;

    for (i = from; i < to; i++)
    {
	str = dir->entries[i].real;
	last = strlen(str) - 1;
	oldc = str[last];
	str[last] = 0;
	if (mch_stat(str, &statBuf))
	    newc = ' ';
	else
	    newc = SFstatChar(&statBuf);
	str[last] = newc;
	if (newc != oldc)
	    result = 1;
    }

    return result;
}

/* ARGSUSED */
    static void
SFdirModTimer(cl, id)
    XtPointer		cl;
    XtIntervalId	*id;
{
    static int		n = -1;
    static int		f = 0;
    char		save;
    SFDir		*dir;

    if ((!SFtwiddle) && (SFdirPtr < SFdirEnd))
    {
	n++;
	if ((n > 2) || (SFdirPtr + n >= SFdirEnd))
	{
	    n = 0;
	    f++;
	    if ((f > 2) || (SFdirPtr + f >= SFdirEnd))
		f = 0;
	}
	dir = &(SFdirs[SFdirPtr + n]);
	save = *(dir->path);
	*(dir->path) = 0;
	if (SFchdir(SFcurrentPath))
	{
	    *(dir->path) = save;

	    /*
	     * force a re-read
	     */
	    *(dir->dir) = 0;

	    SFupdatePath();
	}
	else
	{
	    *(dir->path) = save;
	    if (SFcheckDir(n, dir) || ((f == n) && SFcheckFiles(dir)))
		SFdrawList(n, SF_DO_SCROLL);
	}
    }

    SFdirModTimerId = XtAppAddTimeOut(SFapp, (unsigned long) 1000,
	    SFdirModTimer, (XtPointer) NULL);
}

/* Return a single character describing what kind of file STATBUF is.  */

    static char
SFstatChar(statBuf)
    struct stat *statBuf;
{
    if (S_ISDIR (statBuf->st_mode))
	return '/';
    if (S_ISREG (statBuf->st_mode))
	return S_ISXXX (statBuf->st_mode) ? '*' : ' ';
#ifdef S_ISSOCK
    if (S_ISSOCK (statBuf->st_mode))
	return '=';
#endif /* S_ISSOCK */
    return ' ';
}

/***************** Draw.c */

#ifdef FEAT_GUI_NEXTAW
# include <X11/neXtaw/Cardinals.h>
#else
# include <X11/Xaw/Cardinals.h>
#endif

#ifdef FEAT_XFONTSET
# define SF_DEFAULT_FONT "-misc-fixed-medium-r-normal--14-*"
#else
# define SF_DEFAULT_FONT "9x15"
#endif

#ifdef ABS
# undef ABS
#endif
#define ABS(x) (((x) < 0) ? (-(x)) : (x))

typedef struct
{
    char *fontname;
} TextData;

static GC SFlineGC, SFscrollGC, SFinvertGC, SFtextGC;

static XtResource textResources[] =
{
#ifdef FEAT_XFONTSET
	{XtNfontSet, XtCFontSet, XtRString, sizeof (char *),
		XtOffsetOf(TextData, fontname), XtRString, SF_DEFAULT_FONT},
#else
	{XtNfont, XtCFont, XtRString, sizeof (char *),
		XtOffsetOf(TextData, fontname), XtRString, SF_DEFAULT_FONT},
#endif
};

#ifdef FEAT_XFONTSET
static XFontSet SFfont;
#else
static XFontStruct *SFfont;
#endif

static int SFcurrentListY;

static XtIntervalId SFscrollTimerId;

static void SFinitFont __ARGS((void));

    static void
SFinitFont()
{
    TextData	*data;
#ifdef FEAT_XFONTSET
    XFontSetExtents *extents;
    char **missing, *def_str;
    int  num_missing;
#endif

    data = XtNew(TextData);

    XtGetApplicationResources(selFileForm, (XtPointer) data, textResources,
	    XtNumber(textResources), (Arg *) NULL, ZERO);

#ifdef FEAT_XFONTSET
    SFfont = XCreateFontSet(SFdisplay, data->fontname,
			    &missing, &num_missing, &def_str);
#else
    SFfont = XLoadQueryFont(SFdisplay, data->fontname);
#endif
    if (!SFfont)
    {
#ifdef FEAT_XFONTSET
	SFfont = XCreateFontSet(SFdisplay, SF_DEFAULT_FONT,
					    &missing, &num_missing, &def_str);
#else
	SFfont = XLoadQueryFont(SFdisplay, SF_DEFAULT_FONT);
#endif
	if (!SFfont)
	{
	    EMSG2(_("E616: vim_SelFile: can't get font %s"), SF_DEFAULT_FONT);
	    SFstatus = SEL_FILE_CANCEL;
	    return;
	}
    }

#ifdef FEAT_XFONTSET
    extents = XExtentsOfFontSet(SFfont);
    SFcharWidth = extents->max_logical_extent.width;
    SFcharAscent = -extents->max_logical_extent.y;
    SFcharHeight = extents->max_logical_extent.height;
#else
    SFcharWidth = (SFfont->max_bounds.width + SFfont->min_bounds.width) / 2;
    SFcharAscent = SFfont->max_bounds.ascent;
    SFcharHeight = SFcharAscent + SFfont->max_bounds.descent;
#endif
}

static void SFcreateGC __ARGS((void));

    static void
SFcreateGC()
{
    XGCValues	gcValues;
    XRectangle	rectangles[1];

    gcValues.foreground = SFfore;

    SFlineGC = XtGetGC(
	    selFileLists[0],
	    (XtGCMask)GCForeground,
	    &gcValues);

    SFscrollGC = XtGetGC(
	    selFileLists[0],
	    (XtGCMask)0,
	    &gcValues);

    gcValues.function = GXxor;
    gcValues.foreground = SFfore ^ SFback;
    gcValues.background = SFfore ^ SFback;

    SFinvertGC = XtGetGC(
	    selFileLists[0],
	    (XtGCMask)GCFunction | GCForeground | GCBackground,
	    &gcValues);

    gcValues.foreground = SFfore;
    gcValues.background = SFback;
#ifndef FEAT_XFONTSET
    gcValues.font = SFfont->fid;
#endif

    SFtextGC = XCreateGC(
	    SFdisplay,
	    XtWindow(selFileLists[0]),
#ifdef FEAT_XFONTSET
	    (unsigned long)GCForeground | GCBackground,
#else
	    (unsigned long)GCForeground | GCBackground | GCFont,
#endif
	    &gcValues);

    rectangles[0].x = SFlineToTextH + SFbesideText;
    rectangles[0].y = 0;
    rectangles[0].width = SFcharsPerEntry * SFcharWidth;
    rectangles[0].height = SFupperY + 1;

    XSetClipRectangles(
	    SFdisplay,
	    SFtextGC,
	    0,
	    0,
	    rectangles,
	    1,
	    Unsorted);
}

    static void
SFclearList(n, doScroll)
    int	n;
    int	doScroll;
{
    SFDir	*dir;

    SFcurrentInvert[n] = -1;

    XClearWindow(SFdisplay, XtWindow(selFileLists[n]));

    XDrawSegments(SFdisplay, XtWindow(selFileLists[n]), SFlineGC, SFsegs, 2);

    if (doScroll)
    {
	dir = &(SFdirs[SFdirPtr + n]);

	if ((SFdirPtr + n < SFdirEnd) && dir->nEntries && dir->nChars)
	{
#ifdef FEAT_GUI_NEXTAW
	    XawScrollbarSetThumb(
		    selFileVScrolls[n],
		    (float) (((double) dir->vOrigin) /
			     dir->nEntries),
		    (float) (((double) ((dir->nEntries < SFlistSize)
					? dir->nEntries : SFlistSize)) /
			     dir->nEntries));
#else
	    vim_XawScrollbarSetThumb(
		    selFileVScrolls[n],
		    (float) (((double) dir->vOrigin) /
			     dir->nEntries),
		    (float) (((double) ((dir->nEntries < SFlistSize)
					? dir->nEntries : SFlistSize)) /
			     dir->nEntries),
		    (double)dir->nEntries);
#endif

#ifdef FEAT_GUI_NEXTAW
	    XawScrollbarSetThumb(
		    selFileHScrolls[n],
		    (float) (((double) dir->hOrigin) / dir->nChars),
		    (float) (((double) ((dir->nChars <
					 SFcharsPerEntry) ? dir->nChars :
					SFcharsPerEntry)) / dir->nChars));
#else
	    vim_XawScrollbarSetThumb(
		    selFileHScrolls[n],
		    (float) (((double) dir->hOrigin) / dir->nChars),
		    (float) (((double) ((dir->nChars <
					 SFcharsPerEntry) ? dir->nChars :
					SFcharsPerEntry)) / dir->nChars),
		    (double)dir->nChars);
#endif
	}
	else
	{
#ifdef FEAT_GUI_NEXTAW
	    XawScrollbarSetThumb(selFileVScrolls[n], (float) 0.0,
		    (float) 1.0);
#else
	    vim_XawScrollbarSetThumb(selFileVScrolls[n], (float) 0.0,
		    (float) 1.0, 1.0);
#endif
#ifdef FEAT_GUI_NEXTAW
	    XawScrollbarSetThumb(selFileHScrolls[n], (float) 0.0,
		    (float) 1.0);
#else
	    vim_XawScrollbarSetThumb(selFileHScrolls[n], (float) 0.0,
		    (float) 1.0, 1.0);
#endif
	}
    }
}

static void SFdeleteEntry __ARGS((SFDir *dir, SFEntry *entry));

    static void
SFdeleteEntry(dir, entry)
    SFDir	*dir;
    SFEntry	*entry;
{
    SFEntry	*e;
    SFEntry	*end;
    int		n;
    int		idx;

    idx = entry - dir->entries;

    if (idx < dir->beginSelection)
	dir->beginSelection--;
    if (idx <= dir->endSelection)
	dir->endSelection--;
    if (dir->beginSelection > dir->endSelection)
	dir->beginSelection = dir->endSelection = -1;

    if (idx < dir->vOrigin)
	dir->vOrigin--;

    XtFree(entry->real);

    end = &(dir->entries[dir->nEntries - 1]);

    for (e = entry; e < end; e++)
	*e = *(e + 1);

    if (!(--dir->nEntries))
	return;

    n = dir - &(SFdirs[SFdirPtr]);
    if ((n < 0) || (n > 2))
	return;

#ifdef FEAT_GUI_NEXTAW
    XawScrollbarSetThumb(
	    selFileVScrolls[n],
	    (float) (((double) dir->vOrigin) / dir->nEntries),
	    (float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries));
#else
    vim_XawScrollbarSetThumb(
	    selFileVScrolls[n],
	    (float) (((double) dir->vOrigin) / dir->nEntries),
	    (float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries),
	    (double)dir->nEntries);
#endif
}

static void SFwriteStatChar __ARGS((char *name, int last, struct stat *statBuf));

    static void
SFwriteStatChar(name, last, statBuf)
    char	*name;
    int		last;
    struct stat	*statBuf;
{
    name[last] = SFstatChar(statBuf);
}

static int SFstatAndCheck __ARGS((SFDir *dir, SFEntry *entry));

    static int
SFstatAndCheck(dir, entry)
    SFDir	*dir;
    SFEntry	*entry;
{
    struct stat	statBuf;
    char	save;
    int		last;

    /*
     * must be restored before returning
     */
    save = *(dir->path);
    *(dir->path) = 0;

    if (!SFchdir(SFcurrentPath))
    {
	last = strlen(entry->real) - 1;
	entry->real[last] = 0;
	entry->statDone = 1;
	if ((!mch_stat(entry->real, &statBuf))
#ifdef S_IFLNK
		|| (!mch_lstat(entry->real, &statBuf))
#endif
	   )
	{
	    if (SFfunc)
	    {
		char *shown;

		shown = NULL;
		if (SFfunc(entry->real, &shown, &statBuf))
		{
		    if (shown)
		    {
			int len;

			len = strlen(shown);
			entry->shown = XtMalloc((unsigned) (len + 2));
			(void) strcpy(entry->shown, shown);
			SFwriteStatChar(entry->shown, len, &statBuf);
			entry->shown[len + 1] = 0;
		    }
		}
		else
		{
		    SFdeleteEntry(dir, entry);

		    *(dir->path) = save;
		    return 1;
		}
	    }
	    SFwriteStatChar(entry->real, last, &statBuf);
	}
	else
	    entry->real[last] = ' ';
    }

    *(dir->path) = save;
    return 0;
}


    static void
SFdrawStrings(w, dir, from, to)
    Window	w;
    SFDir	*dir;
    int		from;
    int		to;
{
    int		i;
    SFEntry	*entry;
    int		x;

    x = SFtextX - dir->hOrigin * SFcharWidth;

    if (dir->vOrigin + to >= dir->nEntries)
	to = dir->nEntries - dir->vOrigin - 1;
    for (i = from; i <= to; i++)
    {
	entry = &(dir->entries[dir->vOrigin + i]);
	if (!(entry->statDone))
	{
	    if (SFstatAndCheck(dir, entry))
	    {
		if (dir->vOrigin + to >= dir->nEntries)
		    to = dir->nEntries - dir->vOrigin - 1;
		i--;
		continue;
	    }
	}
#ifdef FEAT_XFONTSET
	XmbDrawImageString(
		SFdisplay,
		w,
		SFfont,
		SFtextGC,
		x,
		SFtextYoffset + i * SFentryHeight,
		entry->shown,
		strlen(entry->shown));
#else
	XDrawImageString(
		SFdisplay,
		w,
		SFtextGC,
		x,
		SFtextYoffset + i * SFentryHeight,
		entry->shown,
		strlen(entry->shown));
#endif
	if (dir->vOrigin + i == dir->beginSelection)
	{
	    XDrawLine(
		    SFdisplay,
		    w,
		    SFlineGC,
		    SFlineToTextH + 1,
		    SFlowerY + i * SFentryHeight,
		    SFlineToTextH + SFentryWidth - 2,
		    SFlowerY + i * SFentryHeight);
	}
	if ((dir->vOrigin + i >= dir->beginSelection) &&
		(dir->vOrigin + i <= dir->endSelection))
	{
	    SFcompletionSegs[0].y1 = SFcompletionSegs[1].y1 =
		SFlowerY + i * SFentryHeight;
	    SFcompletionSegs[0].y2 = SFcompletionSegs[1].y2 =
		SFlowerY + (i + 1) * SFentryHeight - 1;
	    XDrawSegments(
		    SFdisplay,
		    w,
		    SFlineGC,
		    SFcompletionSegs,
		    2);
	}
	if (dir->vOrigin + i == dir->endSelection)
	{
	    XDrawLine(
		    SFdisplay,
		    w,
		    SFlineGC,
		    SFlineToTextH + 1,
		    SFlowerY + (i + 1) * SFentryHeight - 1,
		    SFlineToTextH + SFentryWidth - 2,
		    SFlowerY + (i + 1) * SFentryHeight - 1);
	}
    }
}

    static void
SFdrawList(n, doScroll)
    int	n;
    int	doScroll;
{
    SFDir	*dir;
    Window	w;

    SFclearList(n, doScroll);

    if (SFdirPtr + n < SFdirEnd)
    {
	dir = &(SFdirs[SFdirPtr + n]);
	w = XtWindow(selFileLists[n]);
#ifdef FEAT_XFONTSET
	XmbDrawImageString(
		SFdisplay,
		w,
		SFfont,
		SFtextGC,
		SFtextX - dir->hOrigin * SFcharWidth,
		SFlineToTextV + SFaboveAndBelowText + SFcharAscent,
		dir->dir,
		strlen(dir->dir));
#else
	XDrawImageString(
		SFdisplay,
		w,
		SFtextGC,
		SFtextX - dir->hOrigin * SFcharWidth,
		SFlineToTextV + SFaboveAndBelowText + SFcharAscent,
		dir->dir,
		strlen(dir->dir));
#endif
	SFdrawStrings(w, dir, 0, SFlistSize - 1);
    }
}

    static void
SFdrawLists(doScroll)
    int	doScroll;
{
    int	i;

    for (i = 0; i < 3; i++)
	SFdrawList(i, doScroll);
}

    static void
SFinvertEntry(n)
    int		n;
{
    XFillRectangle(
	    SFdisplay,
	    XtWindow(selFileLists[n]),
	    SFinvertGC,
	    SFlineToTextH,
	    SFcurrentInvert[n] * SFentryHeight + SFlowerY,
	    SFentryWidth,
	    SFentryHeight);
}

static unsigned long SFscrollTimerInterval __ARGS((void));

    static unsigned long
SFscrollTimerInterval()
{
    static int	maxVal = 200;
    static int	varyDist = 50;
    static int	minDist = 50;
    int		t;
    int		dist;

    if (SFcurrentListY < SFlowerY)
	dist = SFlowerY - SFcurrentListY;
    else if (SFcurrentListY > SFupperY)
	dist = SFcurrentListY - SFupperY;
    else
	return (unsigned long) 1;

    t = maxVal - ((maxVal / varyDist) * (dist - minDist));

    if (t < 1)
	t = 1;

    if (t > maxVal)
	t = maxVal;

    return (unsigned long)t;
}

static void SFscrollTimer __ARGS((XtPointer p, XtIntervalId *id));

/* ARGSUSED */
    static void
SFscrollTimer(p, id)
    XtPointer		p;
    XtIntervalId	*id;
{
    SFDir	*dir;
    int		save;
    int		n;

    n = (long)p;

    dir = &(SFdirs[SFdirPtr + n]);
    save = dir->vOrigin;

    if (SFcurrentListY < SFlowerY)
    {
	if (dir->vOrigin > 0)
	    SFvSliderMovedCallback(selFileVScrolls[n], n, dir->vOrigin - 1);
    }
    else if (SFcurrentListY > SFupperY)
    {
	if (dir->vOrigin < dir->nEntries - SFlistSize)
	    SFvSliderMovedCallback(selFileVScrolls[n], n, dir->vOrigin + 1);
    }

    if (dir->vOrigin != save)
    {
	if (dir->nEntries)
	{
#ifdef FEAT_GUI_NEXTAW
	    XawScrollbarSetThumb(
		    selFileVScrolls[n],
		    (float) (((double) dir->vOrigin) / dir->nEntries),
		    (float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries));
#else
	    vim_XawScrollbarSetThumb(
		    selFileVScrolls[n],
		    (float) (((double) dir->vOrigin) / dir->nEntries),
		    (float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries),
		    (double)dir->nEntries);
#endif
	}
    }

    if (SFbuttonPressed)
	SFscrollTimerId = XtAppAddTimeOut(SFapp,
		       SFscrollTimerInterval(), SFscrollTimer, (XtPointer) n);
}

    static int
SFnewInvertEntry(n, event)
    int			n;
    XMotionEvent	*event;
{
    int			x, y;
    int			nw;
    static int		SFscrollTimerAdded = 0;

    x = event->x;
    y = event->y;

    if (SFdirPtr + n >= SFdirEnd)
	return -1;

    if ((x >= 0) && (x <= SFupperX) && (y >= SFlowerY) && (y <= SFupperY))
    {
	SFDir *dir = &(SFdirs[SFdirPtr + n]);

	if (SFscrollTimerAdded)
	{
	    SFscrollTimerAdded = 0;
	    XtRemoveTimeOut(SFscrollTimerId);
	}

	nw = (y - SFlowerY) / SFentryHeight;
	if (dir->vOrigin + nw >= dir->nEntries)
	    return -1;
	return nw;
    }
    else
    {
	if (SFbuttonPressed)
	{
	    SFcurrentListY = y;
	    if (!SFscrollTimerAdded)
	    {
		SFscrollTimerAdded = 1;
		SFscrollTimerId = XtAppAddTimeOut(SFapp,
			SFscrollTimerInterval(), SFscrollTimer,
			(XtPointer) n);
	    }
	}
	return -1;
    }
}

/* ARGSUSED */
    static void
SFenterList(w, n, event)
    Widget		w;
    int			n;
    XEnterWindowEvent	*event;
{
    int			nw;

    /* sanity */
    if (SFcurrentInvert[n] != -1)
    {
	SFinvertEntry(n);
	SFcurrentInvert[n] = -1;
    }

    nw = SFnewInvertEntry(n, (XMotionEvent *) event);
    if (nw != -1)
    {
	SFcurrentInvert[n] = nw;
	SFinvertEntry(n);
    }
}

/* ARGSUSED */
    static void
SFleaveList(w, n, event)
    Widget	w;
    int		n;
    XEvent	*event;
{
    if (SFcurrentInvert[n] != -1)
    {
	SFinvertEntry(n);
	SFcurrentInvert[n] = -1;
    }
}

/* ARGSUSED */
    static void
SFmotionList(w, n, event)
    Widget		w;
    int			n;
    XMotionEvent	*event;
{
    int		nw;

    nw = SFnewInvertEntry(n, event);

    if (nw != SFcurrentInvert[n])
    {
	if (SFcurrentInvert[n] != -1)
	    SFinvertEntry(n);
	SFcurrentInvert[n] = nw;
	if (nw != -1)
	    SFinvertEntry(n);
    }
}

/* ARGSUSED */
    static void
SFvFloatSliderMovedCallback(w, n, fnew)
    Widget	w;
    XtPointer	n;
    XtPointer	fnew;
{
    int		nw;

    nw = (*(float *)fnew) * SFdirs[SFdirPtr + (int)(long)n].nEntries;
    SFvSliderMovedCallback(w, (int)(long)n, nw);
}

/* ARGSUSED */
    static void
SFvSliderMovedCallback(w, n, nw)
    Widget	w;
    int		n;
    int		nw;
{
    int		old;
    Window	win;
    SFDir	*dir;

    dir = &(SFdirs[SFdirPtr + n]);

    old = dir->vOrigin;
    dir->vOrigin = nw;

    if (old == nw)
	return;

    win = XtWindow(selFileLists[n]);

    if (ABS(nw - old) < SFlistSize)
    {
	if (nw > old)
	{
	    XCopyArea(
		    SFdisplay,
		    win,
		    win,
		    SFscrollGC,
		    SFlineToTextH,
		    SFlowerY + (nw - old) * SFentryHeight,
		    SFentryWidth + SFlineToTextH,
		    (SFlistSize - (nw - old)) * SFentryHeight,
		    SFlineToTextH,
		    SFlowerY);
	    XClearArea(
		    SFdisplay,
		    win,
		    SFlineToTextH,
		    SFlowerY + (SFlistSize - (nw - old)) *
		    SFentryHeight,
		    SFentryWidth + SFlineToTextH,
		    (nw - old) * SFentryHeight,
		    False);
	    SFdrawStrings(win, dir, SFlistSize - (nw - old),
		    SFlistSize - 1);
	}
	else
	{
	    XCopyArea(
		    SFdisplay,
		    win,
		    win,
		    SFscrollGC,
		    SFlineToTextH,
		    SFlowerY,
		    SFentryWidth + SFlineToTextH,
		    (SFlistSize - (old - nw)) * SFentryHeight,
		    SFlineToTextH,
		    SFlowerY + (old - nw) * SFentryHeight);
	    XClearArea(
		    SFdisplay,
		    win,
		    SFlineToTextH,
		    SFlowerY,
		    SFentryWidth + SFlineToTextH,
		    (old - nw) * SFentryHeight,
		    False);
	    SFdrawStrings(win, dir, 0, old - nw);
	}
    }
    else
    {
	XClearArea(
		SFdisplay,
		win,
		SFlineToTextH,
		SFlowerY,
		SFentryWidth + SFlineToTextH,
		SFlistSize * SFentryHeight,
		False);
	SFdrawStrings(win, dir, 0, SFlistSize - 1);
    }
}

/* ARGSUSED */
    static void
SFvAreaSelectedCallback(w, n, pnew)
    Widget		w;
    XtPointer	n;
    XtPointer	pnew;
{
    SFDir	*dir;
    int		nw = (int)(long)pnew;

    dir = &(SFdirs[SFdirPtr + (int)(long)n]);

#ifdef FEAT_GUI_NEXTAW
    if (nw < 0)
    {
	if (nw > -SFvScrollHeight)
	    nw = -1;
	else
	    nw = -SFlistSize;
    }
    else if (nw > 0)
    {
	if (nw < SFvScrollHeight)
	    nw = 1;
	else
	    nw = SFlistSize;
    }
#endif
    nw += dir->vOrigin;

    if (nw > dir->nEntries - SFlistSize)
	nw = dir->nEntries - SFlistSize;

    if (nw < 0)
	nw = 0;

    if (dir->nEntries)
    {
	float	f;

	f = ((double) nw) / dir->nEntries;

#ifdef FEAT_GUI_NEXTAW
	XawScrollbarSetThumb(
		w,
		f,
		(float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries));
#else
	vim_XawScrollbarSetThumb(
		w,
		f,
		(float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries),
		(double)dir->nEntries);
#endif
    }

    SFvSliderMovedCallback(w, (int)(long)n, nw);
}

/* ARGSUSED */
    static void
SFhSliderMovedCallback(w, n, nw)
    Widget	w;
    XtPointer	n;
    XtPointer	nw;
{
    SFDir	*dir;
    int	save;

    dir = &(SFdirs[SFdirPtr + (int)(long)n]);
    save = dir->hOrigin;
    dir->hOrigin = (*(float *)nw) * dir->nChars;
    if (dir->hOrigin == save)
	return;

    SFdrawList((int)(long)n, SF_DO_NOT_SCROLL);
}

/* ARGSUSED */
    static void
SFhAreaSelectedCallback(w, n, pnew)
    Widget		w;
    XtPointer	n;
    XtPointer	pnew;
{
    SFDir	*dir;
    int		nw = (int)(long)pnew;

    dir = &(SFdirs[SFdirPtr + (int)(long)n]);

#ifdef FEAT_GUI_NEXTAW
    if (nw < 0)
    {
	if (nw > -SFhScrollWidth)
	    nw = -1;
	else
	    nw = -SFcharsPerEntry;
    }
    else if (nw > 0)
    {
	if (nw < SFhScrollWidth)
	    nw = 1;
	else
	    nw = SFcharsPerEntry;
    }
#endif
    nw += dir->hOrigin;

    if (nw > dir->nChars - SFcharsPerEntry)
	nw = dir->nChars - SFcharsPerEntry;

    if (nw < 0)
	nw = 0;

    if (dir->nChars)
    {
	float	f;

	f = ((double) nw) / dir->nChars;

#ifdef FEAT_GUI_NEXTAW
	XawScrollbarSetThumb(
		w,
		f,
		(float) (((double) ((dir->nChars < SFcharsPerEntry) ?
			       dir->nChars : SFcharsPerEntry)) / dir->nChars));
#else
	vim_XawScrollbarSetThumb(
		w,
		f,
		(float) (((double) ((dir->nChars < SFcharsPerEntry) ?
			       dir->nChars : SFcharsPerEntry)) / dir->nChars),
		(double)dir->nChars);
#endif

	SFhSliderMovedCallback(w, n, (XtPointer)&f);
    }
}

/* ARGSUSED */
    static void
SFpathSliderMovedCallback(w, client_data, nw)
    Widget		w;
    XtPointer	client_data;
    XtPointer	nw;
{
    SFDir		*dir;
    int			n;
    XawTextPosition	pos;
    int			SFdirPtrSave;

    SFdirPtrSave = SFdirPtr;
    SFdirPtr = (*(float *)nw) * SFdirEnd;
    if (SFdirPtr == SFdirPtrSave)
	return;

    SFdrawLists(SF_DO_SCROLL);

    n = 2;
    while (SFdirPtr + n >= SFdirEnd)
	n--;

    dir = &(SFdirs[SFdirPtr + n]);

    pos = dir->path - SFcurrentPath;

    if (!strncmp(SFcurrentPath, SFstartDir, strlen(SFstartDir)))
    {
	pos -= strlen(SFstartDir);
	if (pos < 0)
	    pos = 0;
    }

    XawTextSetInsertionPoint(selFileField, pos);
}

/* ARGSUSED */
    static void
SFpathAreaSelectedCallback(w, client_data, pnew)
    Widget	w;
    XtPointer	client_data;
    XtPointer	pnew;
{
    int		nw = (int)(long)pnew;
    float	f;

#ifdef FEAT_GUI_NEXTAW
    if (nw < 0)
    {
	if (nw > -SFpathScrollWidth)
	    nw = -1;
	else
	    nw = -3;
    }
    else if (nw > 0)
    {
	if (nw < SFpathScrollWidth)
	    nw = 1;
	else
	    nw = 3;
    }
#endif
    nw += SFdirPtr;

    if (nw > SFdirEnd - 3)
	nw = SFdirEnd - 3;

    if (nw < 0)
	nw = 0;

    f = ((double) nw) / SFdirEnd;

#ifdef FEAT_GUI_NEXTAW
    XawScrollbarSetThumb(
	    w,
	    f,
	    (float) (((double) ((SFdirEnd < 3) ? SFdirEnd : 3)) / SFdirEnd));
#else
    vim_XawScrollbarSetThumb(
	    w,
	    f,
	    (float) (((double) ((SFdirEnd < 3) ? SFdirEnd : 3)) / SFdirEnd),
	    (double)SFdirEnd);
#endif

    SFpathSliderMovedCallback(w, (XtPointer) NULL, (XtPointer)&f);
}

    static Boolean
SFworkProc()
{
    SFDir	*dir;
    SFEntry	*entry;

    for (dir = &(SFdirs[SFdirEnd - 1]); dir >= SFdirs; dir--)
    {
	if (!(dir->nEntries))
	    continue;
	for (entry = &(dir->entries[dir->nEntries - 1]);
		entry >= dir->entries;
		entry--)
	{
	    if (!(entry->statDone))
	    {
		(void)SFstatAndCheck(dir, entry);
		return False;
	    }
	}
    }

    SFworkProcAdded = 0;

    return True;
}

/***************** Dir.c */

    static int
SFcompareEntries(p, q)
    const void	*p;
    const void	*q;
{
    return strcmp(((SFEntry *)p)->real, ((SFEntry *)q)->real);
}

    static int
SFgetDir(dir)
    SFDir	*dir;
{
    SFEntry		*result = NULL;
    int			Alloc = 0;
    int			i;
    DIR			*dirp;
    struct dirent	*dp;
    char		*str;
    int			len;
    int			maxChars;
    struct stat		statBuf;

    maxChars = strlen(dir->dir) - 1;

    dir->entries = NULL;
    dir->nEntries = 0;
    dir->nChars = 0;

    result = NULL;
    i = 0;

    dirp = opendir(".");
    if (!dirp)
	return 1;

    (void)mch_stat(".", &statBuf);
    dir->mtime = statBuf.st_mtime;

    while ((dp = readdir(dirp)))
    {
	/* Ignore "." and ".." */
	if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
	    continue;
	if (i >= Alloc)
	{
	    Alloc = 2 * (Alloc + 1);
	    result = (SFEntry *) XtRealloc((char *) result,
		    (unsigned) (Alloc * sizeof(SFEntry)));
	}
	result[i].statDone = 0;
	str = dp->d_name;
	len = strlen(str);
	result[i].real = XtMalloc((unsigned) (len + 2));
	(void) strcat(strcpy(result[i].real, str), " ");
	if (len > maxChars)
	    maxChars = len;
	result[i].shown = result[i].real;
	i++;
    }

    qsort((char *) result, (size_t) i, sizeof(SFEntry), SFcompareEntries);

    dir->entries = result;
    dir->nEntries = i;
    dir->nChars = maxChars + 1;

    closedir(dirp);

    return 0;
}

/***************** SFinternal.h */

#include <sys/param.h>
#include <X11/cursorfont.h>
#include <X11/Composite.h>
#include <X11/Shell.h>
#ifdef FEAT_GUI_NEXTAW
# include <X11/neXtaw/Form.h>
# include <X11/neXtaw/Command.h>
# include <X11/neXtaw/Label.h>
#else
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#endif

static char *oneLineTextEditTranslations = "\
	<Key>Return:	redraw-display()\n\
	Ctrl<Key>M:	redraw-display()\n\
";

static void SFexposeList __ARGS((Widget w, XtPointer n, XEvent *event, Boolean *cont));

/* ARGSUSED */
    static void
SFexposeList(w, n, event, cont)
    Widget	w;
    XtPointer	n;
    XEvent	*event;
    Boolean	*cont;
{
    if ((event->type == NoExpose) || event->xexpose.count)
	return;

    SFdrawList((int)(long)n, SF_DO_NOT_SCROLL);
}

static void SFmodVerifyCallback __ARGS((Widget w, XtPointer client_data, XEvent *event, Boolean *cont));

/* ARGSUSED */
    static void
SFmodVerifyCallback(w, client_data, event, cont)
    Widget		w;
    XtPointer		client_data;
    XEvent		*event;
    Boolean		*cont;
{
    char	buf[2];

    if ((XLookupString(&(event->xkey), buf, 2, NULL, NULL) == 1) &&
	    ((*buf) == '\r'))
	SFstatus = SEL_FILE_OK;
    else
	SFstatus = SEL_FILE_TEXT;
}

static void SFokCallback __ARGS((Widget w, XtPointer cl, XtPointer cd));

/* ARGSUSED */
    static void
SFokCallback(w, cl, cd)
    Widget	w;
    XtPointer	cl, cd;
{
    SFstatus = SEL_FILE_OK;
}

static XtCallbackRec SFokSelect[] =
{
    { SFokCallback, (XtPointer) NULL },
    { NULL, (XtPointer) NULL },
};

static void SFcancelCallback __ARGS((Widget w, XtPointer cl, XtPointer cd));

/* ARGSUSED */
    static void
SFcancelCallback(w, cl, cd)
    Widget	w;
    XtPointer	cl, cd;
{
    SFstatus = SEL_FILE_CANCEL;
}

static XtCallbackRec SFcancelSelect[] =
{
    { SFcancelCallback, (XtPointer) NULL },
    { NULL, (XtPointer) NULL },
};

static void SFdismissAction __ARGS((Widget w, XEvent *event, String *params, Cardinal *num_params));

/* ARGSUSED */
    static void
SFdismissAction(w, event, params, num_params)
    Widget	w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    if (event->type == ClientMessage &&
	    event->xclient.data.l[0] != SFwmDeleteWindow)
	return;

    SFstatus = SEL_FILE_CANCEL;
}

static char *wmDeleteWindowTranslation = "\
	<Message>WM_PROTOCOLS:	SelFileDismiss()\n\
";

static XtActionsRec actions[] =
{
    {"SelFileDismiss",	SFdismissAction},
};

    static void
SFsetColors(bg, fg, scroll_bg, scroll_fg)
    guicolor_T	bg;
    guicolor_T	fg;
    guicolor_T	scroll_bg;
    guicolor_T	scroll_fg;
{
    if (selFileForm)
    {
	XtVaSetValues(selFileForm, XtNbackground,  bg,
				   XtNforeground,  fg,
				   XtNborderColor, bg,
				   NULL);
    }
    {
	int i;

	for (i = 0; i < 3; ++i)
	{
	    if (selFileLists[i])
	    {
		XtVaSetValues(selFileLists[i], XtNbackground,  bg,
					       XtNforeground,  fg,
					       XtNborderColor, fg,
					       NULL);
	    }
	}
    }
    if (selFileOK)
    {
	XtVaSetValues(selFileOK, XtNbackground,  bg,
				 XtNforeground,  fg,
				 XtNborderColor, fg,
				 NULL);
    }
    if (selFileCancel)
    {
	XtVaSetValues(selFileCancel, XtNbackground, bg,
				     XtNforeground, fg,
				     XtNborderColor, fg,
				     NULL);
    }
    if (selFilePrompt)
    {
	XtVaSetValues(selFilePrompt, XtNbackground, bg,
				     XtNforeground, fg,
				     NULL);
    }
    if (gui.dpy)
    {
	XSetBackground(gui.dpy, SFtextGC, bg);
	XSetForeground(gui.dpy, SFtextGC, fg);
	XSetForeground(gui.dpy, SFlineGC, fg);

	/* This is an xor GC, so combine the fg and background */
	XSetBackground(gui.dpy, SFinvertGC, fg ^ bg);
	XSetForeground(gui.dpy, SFinvertGC, fg ^ bg);
    }
    if (selFileHScroll)
    {
	XtVaSetValues(selFileHScroll, XtNbackground, scroll_bg,
				      XtNforeground, scroll_fg,
				      XtNborderColor, fg,
				      NULL);
    }
    {
	int i;

	for (i = 0; i < 3; i++)
	{
	    XtVaSetValues(selFileVScrolls[i], XtNbackground, scroll_bg,
					      XtNforeground, scroll_fg,
					      XtNborderColor, fg,
					      NULL);
	    XtVaSetValues(selFileHScrolls[i], XtNbackground, scroll_bg,
					      XtNforeground, scroll_fg,
					      XtNborderColor, fg,
					      NULL);
	}
    }
}

    static void
SFcreateWidgets(toplevel, prompt, ok, cancel)
    Widget	toplevel;
    char	*prompt;
    char	*ok;
    char	*cancel;
{
    Cardinal	n;
    int		listWidth, listHeight;
    int		listSpacing = 10;
    int		scrollThickness = 15;
    int		hScrollX, hScrollY;
    int		vScrollX, vScrollY;

    selFile = XtVaAppCreateShell("selFile", "SelFile",
		transientShellWidgetClass, SFdisplay,
		XtNtransientFor, toplevel,
		XtNtitle, prompt,
		NULL);

    /* Add WM_DELETE_WINDOW protocol */
    XtAppAddActions(XtWidgetToApplicationContext(selFile),
	    actions, XtNumber(actions));
    XtOverrideTranslations(selFile,
	    XtParseTranslationTable(wmDeleteWindowTranslation));

    selFileForm = XtVaCreateManagedWidget("selFileForm",
		formWidgetClass, selFile,
		XtNdefaultDistance, 30,
		XtNforeground, SFfore,
		XtNbackground, SFback,
		XtNborderColor, SFback,
		NULL);

    selFilePrompt = XtVaCreateManagedWidget("selFilePrompt",
		labelWidgetClass, selFileForm,
		XtNlabel, prompt,
		XtNresizable, True,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		XtNborderWidth, 0,
		XtNforeground, SFfore,
		XtNbackground, SFback,
		NULL);

    /*
    XtVaGetValues(selFilePrompt,
		XtNforeground, &SFfore,
		XtNbackground, &SFback,
		NULL);
    */

    SFinitFont();

    SFentryWidth = SFbesideText + SFcharsPerEntry * SFcharWidth +
	SFbesideText;
    SFentryHeight = SFaboveAndBelowText + SFcharHeight +
	SFaboveAndBelowText;

    listWidth = SFlineToTextH + SFentryWidth + SFlineToTextH + 1 +
	scrollThickness;
    listHeight = SFlineToTextV + SFentryHeight + SFlineToTextV + 1 +
	SFlineToTextV + SFlistSize * SFentryHeight +
	SFlineToTextV + 1 + scrollThickness;

    SFpathScrollWidth = 3 * listWidth + 2 * listSpacing + 4;

    hScrollX = -1;
    hScrollY = SFlineToTextV + SFentryHeight + SFlineToTextV + 1 +
	SFlineToTextV + SFlistSize * SFentryHeight +
	SFlineToTextV;
    SFhScrollWidth = SFlineToTextH + SFentryWidth + SFlineToTextH;

    vScrollX = SFlineToTextH + SFentryWidth + SFlineToTextH;
    vScrollY = SFlineToTextV + SFentryHeight + SFlineToTextV;
    SFvScrollHeight = SFlineToTextV + SFlistSize * SFentryHeight +
	SFlineToTextV;

    SFupperX = SFlineToTextH + SFentryWidth + SFlineToTextH - 1;
    SFlowerY = SFlineToTextV + SFentryHeight + SFlineToTextV + 1 +
	SFlineToTextV;
    SFupperY = SFlineToTextV + SFentryHeight + SFlineToTextV + 1 +
	SFlineToTextV + SFlistSize * SFentryHeight - 1;

    SFtextX = SFlineToTextH + SFbesideText;
    SFtextYoffset = SFlowerY + SFaboveAndBelowText + SFcharAscent;

    SFsegs[0].x1 = 0;
    SFsegs[0].y1 = vScrollY;
    SFsegs[0].x2 = vScrollX - 1;
    SFsegs[0].y2 = vScrollY;
    SFsegs[1].x1 = vScrollX;
    SFsegs[1].y1 = 0;
    SFsegs[1].x2 = vScrollX;
    SFsegs[1].y2 = vScrollY - 1;

    SFcompletionSegs[0].x1 = SFcompletionSegs[0].x2 = SFlineToTextH;
    SFcompletionSegs[1].x1 = SFcompletionSegs[1].x2 =
	SFlineToTextH + SFentryWidth - 1;

    selFileField = XtVaCreateManagedWidget("selFileField",
		asciiTextWidgetClass, selFileForm,
		XtNwidth, 3 * listWidth + 2 * listSpacing + 4,
		XtNborderColor, SFfore,
		XtNfromVert, selFilePrompt,
		XtNvertDistance, 10,
		XtNresizable, True,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		XtNstring, SFtextBuffer,
		XtNlength, MAXPATHL,
		XtNeditType, XawtextEdit,
		XtNwrap, XawtextWrapWord,
		XtNresize, XawtextResizeHeight,
		XtNuseStringInPlace, True,
		NULL);

    XtOverrideTranslations(selFileField,
	    XtParseTranslationTable(oneLineTextEditTranslations));
    XtSetKeyboardFocus(selFileForm, selFileField);

    selFileHScroll = XtVaCreateManagedWidget("selFileHScroll",
#ifdef FEAT_GUI_NEXTAW
		scrollbarWidgetClass, selFileForm,
#else
		vim_scrollbarWidgetClass, selFileForm,
#endif
		XtNorientation, XtorientHorizontal,
		XtNwidth, SFpathScrollWidth,
		XtNheight, scrollThickness,
		XtNborderColor, SFfore,
		XtNfromVert, selFileField,
		XtNvertDistance, 30,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		XtNforeground, gui.scroll_fg_pixel,
		XtNbackground, gui.scroll_bg_pixel,
#ifndef FEAT_GUI_NEXTAW
		XtNlimitThumb, 1,
#endif
		NULL);

    XtAddCallback(selFileHScroll, XtNjumpProc,
	    (XtCallbackProc) SFpathSliderMovedCallback, (XtPointer)NULL);
    XtAddCallback(selFileHScroll, XtNscrollProc,
	    (XtCallbackProc) SFpathAreaSelectedCallback, (XtPointer)NULL);

    selFileLists[0] = XtVaCreateManagedWidget("selFileList1",
		compositeWidgetClass, selFileForm,
		XtNwidth, listWidth,
		XtNheight, listHeight,
		XtNforeground,  SFfore,
		XtNbackground,  SFback,
		XtNborderColor, SFfore,
		XtNfromVert, selFileHScroll,
		XtNvertDistance, 10,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		NULL);

    selFileLists[1] = XtVaCreateManagedWidget("selFileList2",
		compositeWidgetClass, selFileForm,
		XtNwidth, listWidth,
		XtNheight, listHeight,
		XtNforeground,  SFfore,
		XtNbackground,  SFback,
		XtNborderColor, SFfore,
		XtNfromHoriz, selFileLists[0],
		XtNfromVert, selFileHScroll,
		XtNhorizDistance, listSpacing,
		XtNvertDistance, 10,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		NULL);

    selFileLists[2] = XtVaCreateManagedWidget("selFileList3",
		compositeWidgetClass, selFileForm,
		XtNwidth, listWidth,
		XtNheight, listHeight,
		XtNforeground,  SFfore,
		XtNbackground,  SFback,
		XtNborderColor, SFfore,
		XtNfromHoriz, selFileLists[1],
		XtNfromVert, selFileHScroll,
		XtNhorizDistance, listSpacing,
		XtNvertDistance, 10,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		NULL);

    for (n = 0; n < 3; n++)
    {
	selFileVScrolls[n] = XtVaCreateManagedWidget("selFileVScroll",
#ifdef FEAT_GUI_NEXTAW
		    scrollbarWidgetClass, selFileLists[n],
#else
		    vim_scrollbarWidgetClass, selFileLists[n],
#endif
		    XtNx, vScrollX,
		    XtNy, vScrollY,
		    XtNwidth, scrollThickness,
		    XtNheight, SFvScrollHeight,
		    XtNborderColor, SFfore,
		    XtNforeground, gui.scroll_fg_pixel,
		    XtNbackground, gui.scroll_bg_pixel,
#ifndef FEAT_GUI_NEXTAW
		    XtNlimitThumb, 1,
#endif
		    NULL);

	XtAddCallback(selFileVScrolls[n], XtNjumpProc,
		(XtCallbackProc)SFvFloatSliderMovedCallback, (XtPointer)n);
	XtAddCallback(selFileVScrolls[n], XtNscrollProc,
		(XtCallbackProc)SFvAreaSelectedCallback, (XtPointer)n);

	selFileHScrolls[n] = XtVaCreateManagedWidget("selFileHScroll",
#ifdef FEAT_GUI_NEXTAW
		    scrollbarWidgetClass, selFileLists[n],
#else
		    vim_scrollbarWidgetClass, selFileLists[n],
#endif
		    XtNorientation, XtorientHorizontal,
		    XtNx, hScrollX,
		    XtNy, hScrollY,
		    XtNwidth, SFhScrollWidth,
		    XtNheight, scrollThickness,
		    XtNborderColor, SFfore,
		    XtNforeground, gui.scroll_fg_pixel,
		    XtNbackground, gui.scroll_bg_pixel,
#ifndef FEAT_GUI_NEXTAW
		    XtNlimitThumb, 1,
#endif
		    NULL);

	XtAddCallback(selFileHScrolls[n], XtNjumpProc,
		(XtCallbackProc)SFhSliderMovedCallback, (XtPointer)n);
	XtAddCallback(selFileHScrolls[n], XtNscrollProc,
		(XtCallbackProc)SFhAreaSelectedCallback, (XtPointer)n);
    }

    selFileOK = XtVaCreateManagedWidget("selFileOK",
		commandWidgetClass, selFileForm,
		XtNlabel, ok,
		XtNresizable, True,
		XtNcallback, SFokSelect,
		XtNforeground,  SFfore,
		XtNbackground,  SFback,
		XtNborderColor, SFfore,
		XtNfromHoriz, selFileLists[0],
		XtNfromVert, selFileLists[0],
		XtNvertDistance, 30,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		NULL);

    selFileCancel = XtVaCreateManagedWidget("selFileCancel",
		commandWidgetClass, selFileForm,
		XtNlabel, cancel,
		XtNresizable, True,
		XtNcallback, SFcancelSelect,
		XtNforeground,  SFfore,
		XtNbackground,  SFback,
		XtNborderColor, SFfore,
		XtNfromHoriz, selFileOK,
		XtNfromVert, selFileLists[0],
		XtNhorizDistance, 30,
		XtNvertDistance, 30,
		XtNtop, XtChainTop,
		XtNbottom, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainLeft,
		NULL);

    XtSetMappedWhenManaged(selFile, False);
    XtRealizeWidget(selFile);

    /* Add WM_DELETE_WINDOW protocol */
    SFwmDeleteWindow = XInternAtom(SFdisplay, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(SFdisplay, XtWindow(selFile), &SFwmDeleteWindow, 1);

    SFcreateGC();

    for (n = 0; n < 3; n++)
    {
	XtAddEventHandler(selFileLists[n], ExposureMask, True,
		(XtEventHandler)SFexposeList, (XtPointer)n);
	XtAddEventHandler(selFileLists[n], EnterWindowMask, False,
		(XtEventHandler)SFenterList, (XtPointer)n);
	XtAddEventHandler(selFileLists[n], LeaveWindowMask, False,
		(XtEventHandler)SFleaveList, (XtPointer)n);
	XtAddEventHandler(selFileLists[n], PointerMotionMask, False,
		(XtEventHandler)SFmotionList, (XtPointer)n);
	XtAddEventHandler(selFileLists[n], ButtonPressMask, False,
		(XtEventHandler)SFbuttonPressList, (XtPointer)n);
	XtAddEventHandler(selFileLists[n], ButtonReleaseMask, False,
		(XtEventHandler)SFbuttonReleaseList, (XtPointer)n);
    }

    XtAddEventHandler(selFileField, KeyPressMask, False,
				       SFmodVerifyCallback, (XtPointer)NULL);

    SFapp = XtWidgetToApplicationContext(selFile);
}

    static void
SFtextChanged()
{
#if defined(FEAT_XFONTSET) && defined(XtNinternational)
    if (_XawTextFormat((TextWidget)selFileField) == XawFmtWide)
    {
	wchar_t *wcbuf=(wchar_t *)SFtextBuffer;

	if ((wcbuf[0] == L'/') || (wcbuf[0] == L'~'))
	{
	    (void) wcstombs(SFcurrentPath, wcbuf, MAXPATHL);
	    SFtextPos = XawTextGetInsertionPoint(selFileField);
	}
	else
	{
	    strcpy(SFcurrentPath, SFstartDir);
	    (void) wcstombs(SFcurrentPath + strlen(SFcurrentPath), wcbuf, MAXPATHL);

	    SFtextPos = XawTextGetInsertionPoint(selFileField) + strlen(SFstartDir);
	}
    }
    else
#endif
    if ((SFtextBuffer[0] == '/') || (SFtextBuffer[0] == '~'))
    {
	(void) strcpy(SFcurrentPath, SFtextBuffer);
	SFtextPos = XawTextGetInsertionPoint(selFileField);
    }
    else
    {
	(void) strcat(strcpy(SFcurrentPath, SFstartDir), SFtextBuffer);

	SFtextPos = XawTextGetInsertionPoint(selFileField) + strlen(SFstartDir);
    }

    if (!SFworkProcAdded)
    {
	(void) XtAppAddWorkProc(SFapp, (XtWorkProc)SFworkProc, NULL);
	SFworkProcAdded = 1;
    }

    SFupdatePath();
}

    static char *
SFgetText()
{
#if defined(FEAT_XFONTSET) && defined(XtNinternational)
    char *buf;

    if (_XawTextFormat((TextWidget)selFileField) == XawFmtWide)
    {
	wchar_t *wcbuf;
	int mbslength;

	XtVaGetValues(selFileField,
	    XtNstring, &wcbuf,
	NULL);
	mbslength = wcstombs(NULL, wcbuf, 0);
	/* Hack: some broken wcstombs() returns zero, just get a large buffer */
	if (mbslength == 0 && wcbuf != NULL && wcbuf[0] != 0)
	    mbslength = MAXPATHL;
	buf=(char *)XtMalloc(mbslength + 1);
	wcstombs(buf, wcbuf, mbslength +1);
	return buf;
    }
#endif
    return (char *)vim_strsave((char_u *)SFtextBuffer);
}

    static void
SFprepareToReturn()
{
    SFstatus = SEL_FILE_NULL;
    XtRemoveGrab(selFile);
    XtUnmapWidget(selFile);
    XtRemoveTimeOut(SFdirModTimerId);
    if (SFchdir(SFstartDir))
    {
	EMSG(_("E614: vim_SelFile: can't return to current directory"));
	SFstatus = SEL_FILE_CANCEL;
    }
}

    char *
vim_SelFile(toplevel, prompt, init_path, show_entry, x, y, fg, bg, scroll_fg, scroll_bg)
    Widget	toplevel;
    char	*prompt;
    char	*init_path;
    int		(*show_entry)();
    int		x, y;
    guicolor_T	fg, bg;
    guicolor_T	scroll_fg, scroll_bg; /* The "Scrollbar" group colors */
{
    static int	firstTime = 1;
    XEvent	event;
    char	*name_return;

    if (prompt == NULL)
	prompt = _("Pathname:");
    SFfore = fg;
    SFback = bg;

    if (mch_dirname((char_u *)SFstartDir, MAXPATHL) == FAIL)
    {
	EMSG(_("E615: vim_SelFile: can't get current directory"));
	return NULL;
    }

    if (firstTime)
    {
	firstTime = 0;
	SFdisplay = XtDisplay(toplevel);
	SFcreateWidgets(toplevel, prompt, _("OK"), _("Cancel"));
    }
    else
    {
	XtVaSetValues(selFilePrompt, XtNlabel, prompt, NULL);
	XtVaSetValues(selFile, XtNtitle, prompt, NULL);
	SFsetColors(bg, fg, scroll_bg, scroll_fg);
    }

    XtVaSetValues(selFile, XtNx, x, XtNy, y, NULL);
    XtMapWidget(selFile);

    (void)strcat(SFstartDir, "/");
    (void)strcpy(SFcurrentDir, SFstartDir);

    if (init_path)
    {
	if (init_path[0] == '/')
	{
	    (void)strcpy(SFcurrentPath, init_path);
	    if (strncmp(SFcurrentPath, SFstartDir, strlen(SFstartDir)))
		SFsetText(SFcurrentPath);
	    else
		SFsetText(&(SFcurrentPath[strlen(SFstartDir)]));
	}
	else
	{
	    (void)strcat(strcpy(SFcurrentPath, SFstartDir), init_path);
	    SFsetText(&(SFcurrentPath[strlen(SFstartDir)]));
	}
    }
    else
	(void)strcpy(SFcurrentPath, SFstartDir);

    SFfunc = show_entry;

    SFtextChanged();

    XtAddGrab(selFile, True, True);

    SFdirModTimerId = XtAppAddTimeOut(SFapp, (unsigned long) 1000,
	    SFdirModTimer, (XtPointer) NULL);

    for (;;)
    {
	XtAppNextEvent(SFapp, &event);
	XtDispatchEvent(&event);
	switch (SFstatus)
	{
	    case SEL_FILE_TEXT:
		SFstatus = SEL_FILE_NULL;
		SFtextChanged();
		break;
	    case SEL_FILE_OK:
		name_return = SFgetText();
		SFprepareToReturn();
		return name_return;
	    case SEL_FILE_CANCEL:
		SFprepareToReturn();
		return NULL;
	    case SEL_FILE_NULL:
		break;
	}
    }
}
#endif /* FEAT_BROWSE */
